#include "Function.hpp"

#include <algorithm>
#include <set>
#include <unordered_map>

namespace exefacts {

Function::Function(Program& program, uint32_t address) : program_(program), address_(address) {
    const FunctionEntry* f = program.at(address);
    if (f == nullptr)
        f = program.containing(address);
    end_ = address + (f != nullptr ? f->size : 0x400);
    decode();
    cutBlocks();
    for (const auto& [pc, insn] : insns_) {
        const Families written = registerUse(*insn).writes & (kEbx | kEsi | kEdi);
        if (insn->is("pop"))
            popped_ |= written;
        else if (!insn->is("push"))
            clobbers_ |= written;
    }
}

bool Function::pushesInput(const Insn& insn) const {
    return insn.is("push") && (insn.ops == "ebx" || insn.ops == "esi" || insn.ops == "edi") &&
           !((clobbers_ | popped_) & text::family(insn.ops));
}

const std::vector<uint32_t>& Function::block(uint32_t start) const {
    auto found = blocks_.find(start);
    if (found == blocks_.end())
        throw Raised("no block at the entry");
    return found->second;
}

const std::vector<uint32_t>& Function::blockSuccessors(uint32_t start) const {
    return blockSuccessors_.at(start);
}

bool Function::inside(uint32_t pc) const {
    if (address_ <= pc && pc < end_)
        return true;
    for (const auto& [start, end] : chunks_)
        if (start <= pc && pc < end)
            return true;
    return false;
}

std::optional<uint32_t> Function::chunk(uint32_t target) const {
    const FunctionEntry* f = program_.at(target);
    if (f == nullptr || f->size == 0 || f->name.find(":::$") == std::string::npos)
        return std::nullopt;
    return target + f->size;
}

void Function::decode() {
    std::vector<uint32_t> work{address_};
    // a jump's target inside the function, or the start of a chunk of it (taken in as it is reached)
    auto follows = [&](uint32_t target) {
        if (inside(target))
            return true;
        if (std::optional<uint32_t> end = chunk(target)) {
            chunks_.emplace_back(target, *end);
            return true;
        }
        return false;
    };
    while (!work.empty()) {
        uint32_t pc = work.back();
        work.pop_back();
        while (inside(pc) && !insns_.contains(pc)) {
            const Insn* insn = program_.decoder().at(pc);
            if (insn == nullptr)
                break;
            insns_[pc] = insn;
            const std::string& m = insn->mnemonic;
            if (m == "ret" || m == "retn" || m == "retf" || m == "int3" || m == "hlt" || m == "ud2")
                break;
            if (m == "jmp") {
                if (std::optional<uint32_t> target = insn->target()) {
                    if (follows(*target)) {
                        succ_[pc].push_back(*target);
                        work.push_back(*target);
                    } else {
                        tailcalls_.emplace_back(pc, *target);
                    }
                } else {
                    bool found = false;
                    std::vector<uint32_t> targets = jumpTable(pc, found);
                    if (found) {
                        std::vector<uint32_t>& out = succ_[pc];
                        out.insert(out.end(), targets.begin(), targets.end());
                        work.insert(work.end(), targets.begin(), targets.end());
                    }
                }
                break;
            }
            if (insn->startsWith("j") || insn->startsWith("loop")) {
                std::optional<uint32_t> target = insn->target();
                if (target && follows(*target)) {
                    succ_[pc].push_back(*target);
                    work.push_back(*target);
                }
            }
            succ_[pc].push_back(insn->next());
            pc = insn->next();
        }
    }
}

// `jmp [index*4 + table]`: the bound comes from the `cmp; ja` before it (count = the compare's immediate + 1),
// the entries from the table, through a byte index table when a `movzx` reads one. The cases' targets, ascending.
std::vector<uint32_t> Function::jumpTable(uint32_t pc, bool& found) const {
    const Operand* op = insns_.at(pc)->mem();
    if (op == nullptr || op->scale != 4 || op->index == X86_REG_INVALID || op->base != X86_REG_INVALID)
        return {};
    const Image& image = program_.image();
    const uint32_t table = uint32_t(op->disp);
    // up to 14 decoded instructions before the jmp, nearest first
    std::vector<const Insn*> previous;
    for (auto it = insns_.lower_bound(pc); it != insns_.begin() && previous.size() < 14;)
        previous.push_back((--it)->second);
    std::optional<uint32_t> byteTable;
    std::optional<int64_t> count;
    for (size_t i = 0; i < previous.size(); ++i) {
        const Insn& p = *previous[i];
        if (p.is("movzx") && !byteTable && p.ops.find("byte ptr [") != std::string::npos) {
            const Operand* byteOp = p.mem();
            if (byteOp != nullptr && byteOp->disp != 0 && image.valid(uint32_t(byteOp->disp)))
                byteTable = uint32_t(byteOp->disp);
            continue;
        }
        if (p.is("ja")) {
            if (i + 1 < previous.size() && previous[i + 1]->is("cmp")) {
                std::vector<std::string_view> parts = text::parts(previous[i + 1]->ops);
                if (parts.size() >= 2)
                    if (std::optional<int64_t> bound = text::pyInt(parts[1]))
                        count = *bound + 1;
            }
            break;
        }
    }
    if (!count || *count > 4096)
        return {};
    found = true;
    std::set<uint32_t> targets;
    for (int64_t i = 0; i < *count; ++i) {
        const int64_t entry = byteTable ? image.u8(int64_t(*byteTable) + i) : i;
        targets.insert(image.u32(int64_t(table) + 4 * entry));
    }
    return {targets.begin(), targets.end()};
}

void Function::cutBlocks() {
    std::set<uint32_t> leaders{address_};
    for (const auto& [pc, targets] : succ_) {
        const Insn& insn = *insns_.at(pc);
        if (targets.size() > 1 || insn.startsWith("j")) {
            leaders.insert(targets.begin(), targets.end());
            leaders.insert(insn.next());
        }
    }
    std::unordered_map<uint32_t, uint32_t> blockOf;
    std::vector<uint32_t>* current = nullptr;
    uint32_t currentStart = 0;
    for (const auto& [pc, insn] : insns_) {
        bool starts = current == nullptr || leaders.contains(pc);
        if (!starts) {
            // the block ends where control does not fall through to pc
            const Insn& last = *insns_.at(current->back());
            starts = last.is("ret") || last.is("jmp") || last.is("int3") || last.startsWith("j") || last.next() != pc;
        }
        if (starts) {
            currentStart = pc;
            current = &blocks_[pc];
        }
        current->push_back(pc);
        blockOf[pc] = currentStart;
    }
    for (const auto& [start, pcs] : blocks_) {
        std::vector<uint32_t>& out = blockSuccessors_[start];
        auto found = succ_.find(pcs.back());
        if (found == succ_.end())
            continue;
        for (uint32_t t : found->second)
            if (auto block = blockOf.find(t); block != blockOf.end())
                out.push_back(block->second);
    }
}

}  // namespace exefacts
