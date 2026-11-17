#include "StackWalk.hpp"

#include "Facts.hpp"

#include <algorithm>
#include <unordered_set>

namespace exefacts {

namespace {

constexpr Families kCalleeSaved = kEbx | kEsi | kEdi | kEbp;

// the bytes a call pops where the exe shows them: a direct callee's `ret N`, an import's; nullopt for another
// indirect call
std::optional<int64_t> knownPops(Facts& facts, const Insn& call) {
    if (std::optional<uint32_t> target = call.target())
        return facts.calleePops(*target);
    std::optional<uint32_t> slot = call.iatSlot();
    std::optional<uint32_t> known = slot ? facts.program().importPops(*slot) : std::nullopt;
    return known ? std::optional<int64_t>(*known) : std::nullopt;
}

}  // namespace

StackWalk::StackWalk(Facts& facts, const Function& function, View view)
    : function_(function), view_(view) {
    walk(facts);
}

std::optional<int64_t> StackWalk::esp(uint32_t pc) const {
    auto found = frames_.find(pc);
    return found == frames_.end() ? std::nullopt : found->second.esp;
}

std::optional<int64_t> StackWalk::ebp(uint32_t pc) const {
    auto found = frames_.find(pc);
    return found == frames_.end() ? std::nullopt : found->second.ebp;
}

std::optional<int64_t> StackWalk::offset(uint32_t pc, const Operand& op) const {
    if (op.base == X86_REG_INVALID || op.index != X86_REG_INVALID)
        return std::nullopt;
    std::optional<int64_t> origin;
    if (op.base == X86_REG_ESP)
        origin = esp(pc);
    else if (op.base == X86_REG_EBP)
        origin = ebp(pc);
    if (!origin)
        return std::nullopt;
    return *origin + text::signed32(op.disp);
}

int64_t StackWalk::argumentsTaken(Facts& facts, uint32_t block, size_t call) {
    // the argument stack from an unknown call on, along one path (every path into a join has the same depth): a
    // known call or the caller's `add esp, N` cleanup takes N bytes, those pushed after the call first, then the
    // call's pending ones; another unknown call takes those pushed after the first that the code after it does not
    // take. 0xBB3E30's virtual `call edx` runs between the pushes of the constructor's (ret 0Ch) arguments, so it
    // pops none of them; 0xBADB90's `call edx` with `sub esp, 8` pending, a `jge` (an unsigned conversion's fixup)
    // before the call that takes them
    const uint32_t pc = function_.block(block)[call];
    if (auto found = argumentsTaken_.find(pc); found != argumentsTaken_.end())
        return found->second;
    argumentsTaken_[pc] = 0;  // a path back to the call takes nothing more
    int64_t taken = 0;
    int64_t above = 0;
    std::unordered_set<uint32_t> scanned{block};
    for (size_t later = call + 1;;) {
        const std::vector<uint32_t>& pcs = function_.block(block);
        if (later == pcs.size()) {
            const std::vector<uint32_t>& successors = function_.blockSuccessors(block);
            auto next = std::find_if(successors.begin(), successors.end(),
                                     [&](uint32_t successor) { return !scanned.contains(successor); });
            if (next == successors.end())
                break;
            block = *next;
            scanned.insert(block);
            later = 0;
            continue;
        }
        const Insn& next = function_.insn(pcs[later++]);
        const std::vector<std::string_view> parts = text::parts(next.ops);
        const bool cleanup = next.is("add") && parts[0] == "esp";
        std::optional<int64_t> consumed;
        if (next.is("push")) {
            above += 4;
        } else if (next.is("sub") && parts[0] == "esp") {
            std::optional<int64_t> amount = text::pyInt(parts[1]);
            if (!amount)
                break;
            above += *amount;
        } else if (cleanup) {
            // the frame's restore (`add esp, N; ret`) frees locals, no arguments
            consumed = text::pyInt(parts[1]);
            if (!consumed || (later < pcs.size() && function_.insn(pcs[later]).is("ret")))
                break;
        } else if (next.is("call")) {
            consumed = knownPops(facts, next);
            if (!consumed)
                above = std::min(above, argumentsTaken(facts, block, later - 1));
        } else if (next.is("pop") || next.is("ret") || next.is("leave") || parts[0] == "esp") {
            break;
        }
        if (consumed) {
            taken += std::max<int64_t>(0, *consumed - above);
            above = std::max<int64_t>(0, above - *consumed);
        }
        if (cleanup)
            break;
    }
    argumentsTaken_[pc] = taken;
    return taken;
}

void StackWalk::walk(Facts& facts) {
    Program& program = facts.program();
    // the walk state at a block's entry: esp, ebp, argument bytes pushed and not yet taken by a call, in the
    // prologue, the callee-saved registers touched (read or written) so far on the path
    struct State {
        std::optional<int64_t> esp;
        std::optional<int64_t> ebp;
        int64_t pending;
        bool prologue;
        Families touched;
    };
    std::unordered_map<uint32_t, State> stateIn{{function_.address(), {0, std::nullopt, 0, true, 0}}};
    std::vector<uint32_t> work{function_.address()};
    std::unordered_set<uint32_t> seen;
    while (!work.empty()) {
        const uint32_t start = work.back();
        work.pop_back();
        if (!seen.insert(start).second)
            continue;
        auto [esp, ebp, pending, prologue, touched] = stateIn.at(start);
        std::optional<int64_t> lastEaxImm;
        Families loaded = 0;  // the registers holding a dword loaded from the function's stack arguments
        const std::vector<uint32_t>& pcs = function_.block(start);
        for (size_t idx = 0; idx < pcs.size(); ++idx) {
            const Insn& insn = function_.insn(pcs[idx]);
            frames_[insn.address] = {esp, ebp};
            const std::string& m = insn.mnemonic;
            const std::vector<std::string_view> parts = text::parts(insn.ops);
            const std::string_view first = parts[0];
            // a callee-saved register pushed past the prologue before the function touched it still holds the
            // caller's value: a save on the path that uses the register, not an argument (0x9FC0E7 `push ebx; mov
            // ebx, [esp+1Ch]`, then `call ecx` pops only its own 8 bytes), unless the function never writes it
            // (Function::pushesInput: the push passes the caller's value on as an argument)
            const bool lateSave = m == "push" && (first == "ebx" || first == "esi" || first == "edi" ||
                                                  first == "ebp") && !(touched & text::family(first)) &&
                                  !function_.pushesInput(insn);
            const RegisterUse use = registerUse(insn);
            touched |= (use.reads | use.writes) & kCalleeSaved;
            // a push in the prologue of a register that holds one of the function's stack arguments passes it as a
            // call's argument, which ends the prologue (0xBA3F20 `push ebx; mov ebx, [esp+10h]; push esi; ...; push
            // edi; push ebx; call edx`, 0xBB8890 `mov ecx, [esp+10h]; ...; push edi; push ecx; call edx`)
            const Families pushed = m == "push" ? text::family(first) : 0;
            if (prologue && (loaded & pushed))
                prologue = false;
            loaded = Families(loaded & ~use.writes);
            if (m == "mov" && insn.operands.size() == 2 && insn.operands[0].type == X86_OP_REG &&
                insn.operands[0].size == 4 && insn.operands[1].type == X86_OP_MEM)
                if (std::optional<int64_t> at = offset(insn.address, insn.operands[1]); at && *at >= 4)
                    loaded |= text::family(first);
            if (!esp)
                continue;
            if (m == "push") {
                *esp -= 4;
                if (!prologue && !lateSave)
                    pending += 4;
            } else if (m == "pop") {
                *esp += 4;
                if (first == "ebp")
                    ebp.reset();
            } else if (m == "pushal") {
                *esp -= 32;
            } else if (m == "popal") {
                *esp += 32;
            } else if ((m == "sub" || m == "add") && first == "esp") {
                std::optional<int64_t> amount = parts.size() >= 2 ? text::pyInt(parts[1]) : std::nullopt;
                if (!amount) {
                    esp.reset();
                    continue;
                }
                if (m == "sub") {
                    *esp -= *amount;
                    if (!prologue)
                        pending += *amount;
                } else {
                    *esp += *amount;
                    pending = std::max<int64_t>(0, pending - *amount);
                }
            } else if (m == "and" && first == "esp") {
                if (view_ == View::kExact)
                    esp.reset();
            } else if (m == "mov" && parts.size() == 2 && parts[0] == "ebp" && parts[1] == "esp") {
                ebp = esp;
            } else if (m == "mov" && parts.size() == 2 && parts[0] == "esp" && parts[1] == "ebp") {
                esp = ebp;
            } else if (m == "leave") {
                esp = ebp ? std::optional(*ebp + 4) : std::nullopt;
                ebp.reset();
            } else if (m == "lea" && (first == "esp" || first == "ebp")) {
                const Operand& op = *insn.mem();
                std::optional<int64_t> origin = op.base == X86_REG_ESP ? esp
                                                : op.base == X86_REG_EBP ? ebp : std::nullopt;
                std::optional<int64_t> value;
                if (origin && op.index == X86_REG_INVALID)
                    value = *origin + text::signed32(op.disp);
                (first == "esp" ? esp : ebp) = value;
            } else if (m == "mov" && first == "eax") {
                lastEaxImm = parts.size() >= 2 ? text::pyInt(parts[1]) : std::nullopt;
            } else if (m == "call") {
                // the stack probe allocates the frame's locals and is part of the prologue: what the prologue
                // pushes after it (0xBE1750's security cookie) is not an argument
                std::optional<uint32_t> target = insn.target();
                if (target) {
                    const FunctionEntry* callee = program.at(*target);
                    if (callee != nullptr && (callee->name.find("alloca_probe") != std::string::npos ||
                                              callee->name.find("chkstk") != std::string::npos)) {
                        *esp -= lastEaxImm.value_or(0);
                        continue;
                    }
                }
                prologue = false;
                std::optional<int64_t> pops = knownPops(facts, insn);
                if (!pops)
                    pops = std::max<int64_t>(0, pending - argumentsTaken(facts, start, idx));
                *esp += *pops;
                pending = std::max<int64_t>(0, pending - *pops);
            } else if (view_ == View::kEhFrame && m == "mov" && first.find("fs:") != std::string_view::npos) {
                prologue = false;
            } else if (insn.startsWith("j") || m == "ret") {
                prologue = false;
            }
        }
        for (uint32_t next : function_.blockSuccessors(start)) {
            if (stateIn.try_emplace(next, State{esp, ebp, pending, false, touched}).second)
                work.push_back(next);
        }
    }
}

}  // namespace exefacts
