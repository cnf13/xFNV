#include "Facts.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <unordered_set>

namespace exefacts {

namespace {

std::string withoutSpaces(std::string_view s) {
    std::string out;
    for (char c : s)
        if (c != ' ')
            out += c;
    return out;
}

std::string withoutDwordPtr(std::string_view s) {
    std::string out;
    for (size_t at = 0;;) {
        const size_t found = s.find("dword ptr ", at);
        out += s.substr(at, found - at);
        if (found == std::string_view::npos)
            return out;
        at = found + 10;
    }
}

// Python's text.partition(", ")
std::pair<std::string_view, std::string_view> partition(std::string_view s) {
    const size_t cut = s.find(", ");
    if (cut == std::string_view::npos)
        return {s, {}};
    return {s.substr(0, cut), s.substr(cut + 2)};
}

// The families read before written on some path from the entry (a call writes callWrites: eax, ecx and edx unless
// given). exempt(block, i, use) drops the reads of the instruction at block[i] that are no use of the caller's value;
// visit(block, i, written) sees each instruction with the families written before it on the path walked.
struct NoVisit {
    void operator()(const std::vector<uint32_t>&, size_t, Families) const {}
};

template <typename Exempt, typename Visit = NoVisit>
Families readsBeforeWrite(const Function& f, Exempt exempt, Visit visit = {},
                          Families callWrites = kEax | kEcx | kEdx) {
    Families found = 0;
    std::unordered_map<uint32_t, Families> writtenIn{{f.address(), 0}};
    std::vector<uint32_t> work{f.address()};
    std::unordered_set<uint32_t> seen;
    while (!work.empty()) {
        const uint32_t start = work.back();
        work.pop_back();
        if (!seen.insert(start).second)
            continue;
        Families written = writtenIn.at(start);
        const std::vector<uint32_t>& pcs = f.block(start);
        for (size_t i = 0; i < pcs.size(); ++i) {
            const Insn& insn = f.insn(pcs[i]);
            RegisterUse use = registerUse(insn);
            exempt(pcs, i, use);
            visit(pcs, i, written);
            if (insn.is("call"))
                use.writes |= callWrites;
            found |= use.reads & ~written;
            written |= use.writes;
        }
        for (uint32_t next : f.blockSuccessors(start))
            if (writtenIn.try_emplace(next, written).second)
                work.push_back(next);
    }
    return found;
}

// draft._entry_pushes: the entry run of pushes (with `mov eax, fs:[0]`, `mov ebp, esp`, `mov edi, edi` between
// them: the EH and frame prologue)
std::unordered_set<uint32_t> entryPushes(const Function& f) {
    std::unordered_set<uint32_t> out;
    for (uint32_t pc : f.block(f.address())) {
        const Insn& insn = f.insn(pc);
        if (!insn.is("push") && !(insn.is("mov") && (insn.ops == "eax, dword ptr fs:[0]" || insn.ops == "ebp, esp" ||
                                                      insn.ops == "edi, edi")))
            break;
        out.insert(pc);
    }
    return out;
}

// draft.push_slot_overwritten: the `push reg` at block[i] only reserves an argument slot, overwritten whole by an
// x87 store before anything reads it or moves esp (`push ecx; fld [ebp+0x10]; fstp [esp]`)
bool pushSlotOverwritten(const Function& f, const std::vector<uint32_t>& block, size_t i) {
    for (size_t k = i + 1; k < block.size(); ++k) {
        const Insn& insn = f.insn(block[k]);
        const std::string ops = withoutSpaces(insn.ops);
        if ((insn.is("fstp") || insn.is("fst") || insn.is("fistp") || insn.is("fist")) && ops == "dwordptr[esp]")
            return true;
        // x87 instructions that do not name esp and register-only movs may sit between
        if (insn.startsWith("f") && ops.find("esp") == std::string::npos)
            continue;
        if (insn.is("mov") && ops.find("esp") == std::string::npos &&
            ops.substr(0, ops.find(',')).find('[') == std::string::npos)
            continue;
        return false;
    }
    return false;
}

// draft.dead_frame_spill: `mov dword ptr [ebp - X], reg` into a frame slot nothing else in the function touches
// (no overlapping or indexed ebp access, ebp not copied): the stored value is never read
bool deadFrameSpill(const Function& f, const Insn& insn) {
    if (!insn.is("mov") || !insn.ops.starts_with("dword ptr [ebp - ") || !text::family(text::parts(insn.ops)[1]))
        return false;
    const auto& insns = f.insns();
    if (insns.size() < 2)
        return false;
    auto it = insns.begin();
    const Insn& first = *it->second;
    const Insn& second = *(++it)->second;
    if (!(first.is("push") && first.ops == "ebp" && second.is("mov") && second.ops == "ebp, esp"))
        return false;
    const int64_t slot = insn.operands[0].disp;
    for (const auto& [pc, other] : insns) {
        if (pc == insn.address)
            continue;
        bool frameAccess = false;
        for (const Operand& op : other->operands) {
            if (op.type != X86_OP_MEM || (op.base != X86_REG_EBP && op.index != X86_REG_EBP))
                continue;
            if (op.base != X86_REG_INVALID && op.index != X86_REG_INVALID)
                return false;  // an indexed frame access
            frameAccess = true;
            if (op.base == X86_REG_EBP && !other->is("lea") && slot - 10 < op.disp && op.disp <= slot + 3)
                return false;  // overlaps the slot
        }
        if (!frameAccess) {
            const std::string plain = withoutSpaces(other->ops);
            const std::string shown = other->mnemonic + " " + plain;
            if (plain.find("ebp") != std::string::npos && shown != "push ebp" && shown != "pop ebp" &&
                shown != "mov ebp,esp" && shown != "mov esp,ebp")
                return false;  // ebp copied or computed with: a frame pointer could escape
        }
    }
    return true;
}

// abi's exemptions (ecx and edx): a `push ecx` near the entry, in its push run or whose slot is overwritten
// reserves a slot; an ecx or edx spilled into a frame slot nothing reads is no use
void abiExempt(const Function& f, const std::unordered_set<uint32_t>& entry, const std::vector<uint32_t>& block,
               size_t i, RegisterUse& use) {
    const Insn& insn = f.insn(block[i]);
    if (insn.is("push") && insn.ops == "ecx" &&
        (insn.address - f.address() <= 4 || entry.contains(insn.address) || pushSlotOverwritten(f, block, i)))
        use.reads &= ~kEcx;
    const std::string_view src = text::parts(insn.ops).back();
    if ((src == "ecx" || src == "edx") && deadFrameSpill(f, insn))
        use.reads &= ~text::family(src);
}

// regArgs' exemptions (eax, ebx, esi, edi): a push saves, except one passing on an input (Function::pushesInput);
// `sbb r, r` depends on CF only; `or r, -1` sets all ones in the part the function uses
void regArgsExempt(const Function& f, const std::vector<uint32_t>& block, size_t i, RegisterUse& use) {
    const Insn& insn = f.insn(block[i]);
    const std::vector<std::string_view> parts = text::parts(insn.ops);
    if ((insn.is("push") && !f.pushesInput(insn)) || insn.is("pop") ||
        (insn.is("sbb") && parts.size() == 2 && parts[0] == parts[1]))
        use.reads = 0;
    if (insn.is("or") && parts.size() == 2 &&
        (parts[1] == "-1" || parts[1] == "0xff" || parts[1] == "0xffff" || parts[1] == "0xffffffff"))
        use.reads &= ~text::family(parts[0]);
}

// The prologue's run from the entry: pushes and the frame's setup (`mov ebp, esp`, an aligned frame's `and esp, N`,
// `sub esp, N`, the EH frame's `mov eax, fs:[0]`, `mov edi, edi`)
std::unordered_set<uint32_t> prologueRun(const Function& f) {
    std::unordered_set<uint32_t> out;
    for (uint32_t pc : f.block(f.address())) {
        const Insn& insn = f.insn(pc);
        const bool setup = insn.is("mov") && (insn.ops == "eax, dword ptr fs:[0]" || insn.ops == "ebp, esp" ||
                                              insn.ops == "edi, edi");
        const bool frame = (insn.is("and") || insn.is("sub")) && insn.ops.starts_with("esp, ") &&
                           insn.operands.size() == 2 && insn.operands[1].type == X86_OP_IMM;
        if (!insn.is("push") && !setup && !frame)
            break;
        out.insert(pc);
    }
    return out;
}

// The `push ecx` at block[i] only reserves a slot: a store of at least a dword to [esp] overwrites it before
// anything moves esp, leaves the block, reads or takes the address of the slot (an esp-based operand below
// [esp + 4], an indexed one, esp copied) or of a frame slot that may be it (an ebp-based one below ebp)
bool pushSlotReserved(const Function& f, const std::vector<uint32_t>& block, size_t i) {
    for (size_t k = i + 1; k < block.size(); ++k) {
        const Insn& insn = f.insn(block[k]);
        const bool store = insn.is("mov") || insn.is("fstp") || insn.is("fst") || insn.is("fistp") ||
                           insn.is("fist") || insn.is("movss") || insn.is("movd");
        if (store && insn.operands.size() >= 1) {
            const Operand& to = insn.operands[0];
            if (to.type == X86_OP_MEM && to.base == X86_REG_ESP && to.index == X86_REG_INVALID && to.disp == 0 &&
                to.size >= 4)
                return true;
        }
        if ((insn.writes & kEsp) || insn.startsWith("j") || insn.is("call") || insn.is("ret"))
            return false;
        for (const Operand& op : insn.operands) {
            if (op.type == X86_OP_REG && op.reg == X86_REG_ESP)
                return false;
            if (op.type != X86_OP_MEM)
                continue;
            if (op.index == X86_REG_ESP || op.index == X86_REG_EBP || (op.base == X86_REG_ESP && op.disp < 4) ||
                (op.base == X86_REG_EBP && op.disp < 0))
                return false;
        }
    }
    return false;
}

// registerInputs' exemptions for ecx and edx: abi's, and a `push ecx` in the prologue's run or reserving a slot
void inputsExempt(const Function& f, const std::unordered_set<uint32_t>& entry,
                  const std::unordered_set<uint32_t>& prologue, const std::vector<uint32_t>& block, size_t i,
                  RegisterUse& use) {
    abiExempt(f, entry, block, i, use);
    const Insn& insn = f.insn(block[i]);
    if (insn.is("push") && insn.ops == "ecx" && (prologue.contains(insn.address) || pushSlotReserved(f, block, i)))
        use.reads &= ~kEcx;
}

// What an eax definition reaching a ret is (Facts::eaxReturns): none on some path, a call's, `this`, another value,
// or a temporary read again before the ret
enum EaxKind : uint8_t { kUnset = 1, kCall = 2, kSelf = 4, kValue = 8, kTemp = 16 };

}  // namespace

const Function& Facts::function(uint32_t address) {
    auto found = functions_.find(address);
    if (found == functions_.end())
        found = functions_.emplace(address, std::make_unique<Function>(program_, address)).first;
    return *found->second;
}

const StackWalk& Facts::stack(uint32_t address) {
    auto found = stacks_.find(address);
    if (found == stacks_.end()) {
        const Function& f = function(address);
        found = stacks_.emplace(address, std::make_unique<StackWalk>(*this, f)).first;
    }
    return *found->second;
}

void Facts::trim() {
    if (functions_.size() <= 4096)
        return;
    stacks_.clear();
    functions_.clear();
}

void Facts::trimInstructions(size_t maxInsns) {
    trim();
    if (program_.decoder().size() <= maxInsns)
        return;
    stacks_.clear();
    functions_.clear();
    program_.decoder().clear();
}

uint32_t Facts::calleePops(uint32_t target) {
    if (program_.at(target) == nullptr)
        return 0;
    if (auto found = calleePops_.find(target); found != calleePops_.end())
        return found->second;
    const Function& f = function(target);
    std::optional<uint32_t> pops;
    for (const auto& [pc, insn] : f.insns())
        if (insn->is("ret"))
            pops = std::max(pops.value_or(0), insn->imm().value_or(0));
    // a short thunk pops what its tail target pops
    if (!pops && !f.tailcalls().empty() && f.insns().size() <= 3)
        pops = calleePops(f.tailcalls()[0].second);
    // an import thunk, `jmp dword ptr [IAT slot]`
    if (!pops && f.insns().size() == 1 && f.insns().begin()->second->is("jmp"))
        if (std::optional<uint32_t> slot = f.insns().begin()->second->iatSlot())
            pops = program_.importPops(*slot).value_or(0);
    calleePops_[target] = pops.value_or(0);
    return pops.value_or(0);
}

const Abi& Facts::abi(uint32_t address) {
    auto found = abis_.find(address);
    if (found == abis_.end()) {
        std::optional<Abi> result;
        try {
            const Function& f = function(address);
            const StackWalk& st = stack(address);
            Abi info;
            for (const auto& [pc, insn] : f.insns()) {
                if (insn->is("ret")) {
                    info.returns = true;
                    info.retPops = std::max(info.retPops, insn->imm().value_or(0));
                }
            }
            const std::unordered_set<uint32_t> entry = entryPushes(f);
            const Families reads = readsBeforeWrite(f, [&](const std::vector<uint32_t>& block, size_t i,
                                                           RegisterUse& use) { abiExempt(f, entry, block, i, use); });
            info.ecx = reads & kEcx;
            info.edx = reads & kEdx;
            std::set<int32_t> slots;
            for (const auto& [pc, insn] : f.insns()) {
                const Operand* op = insn->mem();
                if (op == nullptr)
                    continue;
                std::optional<int64_t> off = st.offset(pc, *op);
                if (off && *off >= 4)
                    slots.insert(int32_t((*off - 4) / 4));
            }
            info.args.assign(slots.begin(), slots.end());
            result = std::move(info);
        } catch (const Raised&) {
        }
        found = abis_.emplace(address, std::move(result)).first;
    }
    if (!found->second)
        throw Raised("draft.abi raises");
    return *found->second;
}

namespace {

// An instruction's access of its memory operand op: a `lea`'s address, a store whose first operand is its
// destination, written whole, or a read
ArgumentAccess::Kind accessKind(const Insn& insn, const Operand* op) {
    static const std::unordered_set<std::string_view> stores = {
        "mov", "movss", "movsd", "movq", "movd", "movaps", "movups", "movdqa", "movdqu", "fstp", "fst",
        "fistp", "fist", "fisttp", "fnstcw", "fnstsw", "sete", "setne", "setb", "setae", "setbe", "seta",
        "setl", "setge", "setle", "setg", "sets", "setns", "setp", "setnp", "seto", "setno"};
    if (insn.is("lea"))
        return ArgumentAccess::Address;
    if (!insn.operands.empty() && &insn.operands[0] == op && stores.count(insn.mnemonic))
        return ArgumentAccess::Write;
    return ArgumentAccess::Read;
}

}  // namespace

std::vector<ArgumentAccess> Facts::argumentAccesses(uint32_t address) {
    const Function& f = function(address);
    const StackWalk& st = stack(address);
    std::vector<ArgumentAccess> out;
    for (const auto& [pc, insn] : f.insns()) {
        const Operand* op = insn->mem();
        if (op == nullptr)
            continue;
        std::optional<int64_t> off = st.offset(pc, *op);
        if (!off || *off < 4)
            continue;
        out.push_back(ArgumentAccess{pc, *off - 4, op->size, accessKind(*insn, op)});
    }
    return out;
}

namespace {

// A register's index as its Family bit's (eax 0 ... edi 7), for the 32-bit registers; -1 for any other
int fullRegister(x86_reg reg) {
    switch (reg) {
    case X86_REG_EAX: return 0;
    case X86_REG_ECX: return 1;
    case X86_REG_EDX: return 2;
    case X86_REG_EBX: return 3;
    case X86_REG_ESP: return 4;
    case X86_REG_EBP: return 5;
    case X86_REG_ESI: return 6;
    case X86_REG_EDI: return 7;
    default: return -1;
    }
}

// A value computed from the entry's (VtableJump's form): root (-1: ecx, k >= 0: stack argument slot k), each load's
// displacement from the value so far, then offset added
struct Value {
    int32_t root = -1;
    std::vector<int32_t> loads;
    int64_t offset = 0;
    bool operator==(const Value&) const = default;
};

// What the walk knows at a point: the registers' values, and the stack slots stored to (entry-relative offsets; a
// slot not stored to holds its entry value); nullopt: unknown
struct Values {
    std::array<std::optional<Value>, 8> registers;
    std::map<int64_t, std::optional<Value>> slots;
    bool operator==(const Values&) const = default;
};

// a's and b's common knowledge where two paths join
Values joined(const Values& a, const Values& b) {
    Values out;
    for (size_t r = 0; r < out.registers.size(); ++r)
        if (a.registers[r] == b.registers[r])
            out.registers[r] = a.registers[r];
    for (const Values* side : {&a, &b})
        for (const auto& [offset, value] : side->slots) {
            auto here = a.slots.find(offset);
            auto there = b.slots.find(offset);
            const bool same = here != a.slots.end() && there != b.slots.end() && here->second == there->second;
            out.slots[offset] = same ? value : std::nullopt;
        }
    return out;
}

// The value an operand holds: a 32-bit register's, a dword stack slot's (at an offset the stack walk knows), or the
// dword loaded from a known value plus the displacement (no index register)
std::optional<Value> operandValue(const Values& values, const StackWalk& st, uint32_t pc, const Operand& op) {
    if (op.type == X86_OP_REG) {
        const int r = fullRegister(op.reg);
        return r >= 0 ? values.registers[r] : std::nullopt;
    }
    if (op.type != X86_OP_MEM || op.size != 4 || op.index != X86_REG_INVALID)
        return std::nullopt;
    if (op.base == X86_REG_ESP || op.base == X86_REG_EBP) {
        const std::optional<int64_t> offset = st.offset(pc, op);
        if (!offset)
            return std::nullopt;
        if (auto stored = values.slots.find(*offset); stored != values.slots.end())
            return stored->second;
        if (*offset >= 4 && *offset % 4 == 0)
            return Value{int32_t((*offset - 4) / 4), {}, 0};
        return std::nullopt;
    }
    const int base = fullRegister(op.base);
    if (base < 0 || !values.registers[base])
        return std::nullopt;
    Value v = *values.registers[base];
    v.loads.push_back(int32_t(v.offset + text::signed32(op.disp)));
    v.offset = 0;
    return v;
}

// The instruction's effect on what the walk knows: `mov`, `lea`, and `add`/`sub` of an immediate compute values from
// known ones; any other write of a register, or of a stack slot, makes it unknown, as a call does eax, ecx and edx.
// pushes: a dword push stores its operand's value (passedArguments' argument dwords); else its slot is unknown
void step(Values& values, const StackWalk& st, uint32_t pc, const Insn& insn, bool pushes = false) {
    const Operand* to = insn.operands.empty() ? nullptr : &insn.operands[0];
    const int r = to && to->type == X86_OP_REG && to->size == 4 ? fullRegister(to->reg) : -1;
    if (to && insn.operands.size() == 2) {
        const Operand& from = insn.operands[1];
        if (insn.is("mov") && r >= 0) {
            values.registers[r] = operandValue(values, st, pc, from);
            return;
        }
        if (insn.is("lea") && r >= 0) {
            std::optional<Value> v;
            const int base = from.index == X86_REG_INVALID ? fullRegister(from.base) : -1;
            if (base >= 0 && base != 4 && base != 5 && values.registers[base]) {
                v = values.registers[base];
                v->offset += text::signed32(from.disp);
            }
            values.registers[r] = v;
            return;
        }
        if ((insn.is("add") || insn.is("sub")) && r >= 0 && r != 4 && from.type == X86_OP_IMM) {
            if (values.registers[r])
                values.registers[r]->offset += insn.is("add") ? from.imm : -from.imm;
            return;
        }
    }
    // a stack slot written: a mov's value, any other write's unknown (a push writes the slot below esp)
    std::optional<int64_t> written;
    if (insn.is("push")) {
        if (std::optional<int64_t> esp = st.esp(pc))
            written = *esp - 4;
    } else if (to && to->type == X86_OP_MEM && !insn.is("cmp") && !insn.is("test")) {
        written = st.offset(pc, *to);
    }
    if (written) {
        const bool whole = insn.is("mov") && to->size == 4 && *written % 4 == 0;
        std::optional<Value> v = whole ? operandValue(values, st, pc, insn.operands[1]) : std::nullopt;
        if (pushes && insn.is("push") && to && to->size == 4)
            v = operandValue(values, st, pc, *to);
        for (int64_t slot = *written & ~int64_t(3); slot < *written + (to ? std::max<int64_t>(to->size, 4) : 4);
             slot += 4)
            values.slots[slot] = slot == *written ? v : std::nullopt;
    }
    Families writes = registerUse(insn).writes;
    if (insn.is("call"))
        writes |= kEax | kEcx | kEdx;
    for (size_t k = 0; k < values.registers.size(); ++k)
        if (writes & (1u << k))
            values.registers[k].reset();
}

// The vtable slot a `jmp` reads its target from: [[receiver] + slot], the receiver a value of the entry's
std::optional<VtableJump> vtableJump(const Values& values, const StackWalk& st, const Insn& jmp) {
    if (jmp.operands.size() != 1)
        return std::nullopt;
    std::optional<Value> target = operandValue(values, st, jmp.address, jmp.operands[0]);
    if (!target || target->offset != 0 || target->loads.size() < 2)
        return std::nullopt;
    VtableJump out;
    out.root = target->root;
    out.slot = target->loads.back();
    out.receiver = target->loads[target->loads.size() - 2];
    out.loads.assign(target->loads.begin(), target->loads.end() - 2);
    return out;
}

// A value every proven part agrees on (pops' lattice: none yet, one value, a disagreement)
struct Agreement {
    std::optional<int64_t> value;
    bool conflict = false;

    void add(int64_t v) {
        if (value && *value != v)
            conflict = true;
        value = v;
    }
    bool operator==(const Agreement&) const = default;
};

}  // namespace

Facts::PopsNode Facts::popsNode(uint32_t address) {
    PopsNode node;
    try {
        const Abi& info = abi(address);
        if (info.returns) {
            node.ret = info.retPops;
            return node;
        }
        const Function& f = function(address);
        const StackWalk& st = stack(address);
        Values entry;
        entry.registers[1] = Value{};
        std::map<uint32_t, Values> in{{f.address(), entry}};
        std::map<uint32_t, PopsExit> exits;
        std::vector<uint32_t> work{f.address()};
        while (!work.empty()) {
            const uint32_t start = work.back();
            work.pop_back();
            Values values = in.at(start);
            const std::vector<uint32_t>& pcs = f.block(start);
            for (uint32_t pc : pcs) {
                const Insn& insn = f.insn(pc);
                if (insn.is("jmp") && f.blockSuccessors(start).empty()) {
                    // a jump out of the function: what its target pops, past the bytes esp moved by here
                    PopsExit exit;
                    if (std::optional<uint32_t> target = insn.target())
                        exit.targets.push_back(*target);
                    else if (std::optional<uint32_t> slot = insn.iatSlot())
                        exit.constant = program_.importPops(*slot);
                    else if (std::optional<VtableJump> jump = vtableJump(values, st, insn); jump && slots_)
                        exit.targets = slots_->targets(address, *jump).value_or(std::vector<uint32_t>{});
                    std::optional<int64_t> esp = st.esp(pc);
                    exit.adjust = esp.value_or(0);
                    if (esp && (exit.constant || !exit.targets.empty()))
                        exits[pc] = std::move(exit);
                    else
                        exits.erase(pc);
                }
                step(values, st, pc, insn);
            }
            for (uint32_t next : f.blockSuccessors(start)) {
                auto [at, added] = in.try_emplace(next, values);
                if (!added) {
                    Values merged = joined(at->second, values);
                    if (merged == at->second)
                        continue;
                    at->second = std::move(merged);
                }
                work.push_back(next);
            }
        }
        for (auto& [pc, exit] : exits)
            node.exits.push_back(std::move(exit));
    } catch (const Raised&) {
        node.exits.clear();
    }
    return node;
}

namespace {

// The signedness a condition reads the flags with: -1 signed (l, le, g, ge), 1 unsigned (b, be, a, ae), 0 another
// (e, ne, s, p, o and their negations); nullopt for an instruction that reads no condition
std::optional<int8_t> conditionSign(const Insn& insn) {
    std::string_view m = insn.mnemonic, cc;
    if (m.starts_with("cmov"))
        cc = m.substr(4);
    else if (m.starts_with("set"))
        cc = m.substr(3);
    else if (m.size() > 1 && m[0] == 'j' && m != "jmp" && m != "jecxz" && m != "jcxz")
        cc = m.substr(1);
    else
        return std::nullopt;
    if (cc == "l" || cc == "le" || cc == "g" || cc == "ge" || cc == "nl" || cc == "nle" || cc == "ng" || cc == "nge")
        return int8_t(-1);
    if (cc == "b" || cc == "be" || cc == "a" || cc == "ae" || cc == "nb" || cc == "nbe" || cc == "na" || cc == "nae")
        return int8_t(1);
    return int8_t(0);
}

// The condition that reads the flags the compare at pcs[i] sets: the first instruction after it that is no move,
// `lea`, push or pop (none of which writes the flags); 0 where that is no condition
int8_t compareSign(const Function& f, const std::vector<uint32_t>& pcs, size_t i) {
    for (size_t k = i + 1; k < pcs.size(); ++k) {
        const Insn& next = f.insn(pcs[k]);
        if (next.is("mov") || next.is("movzx") || next.is("movsx") || next.is("lea") || next.is("push") ||
            next.is("pop"))
            continue;
        return conditionSign(next).value_or(0);
    }
    return 0;
}

// An x87 or SSE operand of a float (not an integer's fild/fist, not a control or status word)
bool realOperand(const Insn& insn) {
    std::string_view m = insn.mnemonic;
    if (insn.sets & kX87)
        return !m.starts_with("fi") && !m.starts_with("fn") && !m.starts_with("fldcw") && !m.starts_with("fldenv") &&
               !m.starts_with("fstcw") && !m.starts_with("fstsw") && !m.starts_with("fx") && !m.starts_with("frstor") &&
               !m.starts_with("fsave");
    if (insn.sets & (kSse1 | kSse2))
        return !m.starts_with("cvtsi") && (m.ends_with("ss") || m.ends_with("sd") || m.find("ss2") != m.npos ||
                                           m.find("sd2") != m.npos);
    return false;
}

// What each block starts with, to the fixpoint (popsNode's walk): the entry's ecx and stack arguments as values, each
// instruction's effect step's (pushes: step's)
std::map<uint32_t, Values> blockValues(const Function& f, const StackWalk& st, bool pushes) {
    Values entry;
    entry.registers[1] = Value{};
    std::map<uint32_t, Values> in{{f.address(), entry}};
    std::vector<uint32_t> work{f.address()};
    while (!work.empty()) {
        const uint32_t start = work.back();
        work.pop_back();
        Values values = in.at(start);
        for (uint32_t pc : f.block(start))
            step(values, st, pc, f.insn(pc), pushes);
        for (uint32_t next : f.blockSuccessors(start)) {
            auto [at, added] = in.try_emplace(next, values);
            if (!added) {
                Values merged = joined(at->second, values);
                if (merged == at->second)
                    continue;
                at->second = std::move(merged);
            }
            work.push_back(next);
        }
    }
    return in;
}

}  // namespace

std::vector<ThisAccess> Facts::thisAccesses(uint32_t address) {
    const Function& f = function(address);
    const StackWalk& st = stack(address);
    std::vector<ThisAccess> out;
    for (const auto& [start, first] : blockValues(f, st, false)) {
        Values values = first;
        const std::vector<uint32_t>& pcs = f.block(start);
        for (size_t i = 0; i < pcs.size(); ++i) {
            const Insn& insn = f.insn(pcs[i]);
            const Operand* op = insn.mem();
            const int base = op && op->index == X86_REG_INVALID ? fullRegister(op->base) : -1;
            if (base >= 0 && base != 4 && values.registers[base] && values.registers[base]->root == -1 &&
                values.registers[base]->loads.empty()) {
                ThisAccess a;
                a.pc = pcs[i];
                a.offset = values.registers[base]->offset + text::signed32(op->disp);
                a.size = op->size;
                a.kind = accessKind(insn, op);
                a.real = realOperand(insn);
                a.extension = insn.is("movzx") ? 1 : insn.is("movsx") ? -1 : 0;
                if (insn.is("cmp") || insn.is("test")) {
                    a.sign = compareSign(f, pcs, i);
                } else if (a.kind == ArgumentAccess::Read && !a.real && !insn.operands.empty() &&
                           insn.operands[0].type == X86_OP_REG && (insn.is("mov") || a.extension)) {
                    // the loaded value to its first use in the block: a compare, a shift, or the end of the trail
                    const Families loaded = insn.writes;
                    for (size_t k = i + 1; k < pcs.size() && loaded; ++k) {
                        const Insn& use = f.insn(pcs[k]);
                        if ((use.is("cmp") || use.is("test")) && (use.reads & loaded)) {
                            a.sign = compareSign(f, pcs, k);
                            break;
                        }
                        if ((use.is("sar") || use.is("shr")) && (use.writes & loaded)) {
                            a.sign = use.is("sar") ? -1 : 1;
                            break;
                        }
                        if ((use.reads | use.writes) & loaded || use.is("call"))
                            break;
                    }
                }
                out.push_back(a);
            }
            step(values, st, pcs[i], insn);
        }
    }
    std::sort(out.begin(), out.end(), [](const ThisAccess& a, const ThisAccess& b) { return a.pc < b.pc; });
    return out;
}

std::vector<PassedArgument> Facts::passedArguments(uint32_t address) {
    const Function& f = function(address);
    const StackWalk& st = stack(address);
    // a stack dword's value: the one stored there, else the entry's argument it is
    auto held = [](const Values& values, int64_t at) -> std::optional<Value> {
        if (auto stored = values.slots.find(at); stored != values.slots.end())
            return stored->second;
        if (at >= 4 && at % 4 == 0)
            return Value{int32_t((at - 4) / 4), {}, 0};
        return std::nullopt;
    };
    // the bytes past [esp + 4] the stack arguments a tail jump's target or the function itself reads or pops reach
    auto reach = [&](uint32_t of) -> int64_t {
        try {
            const Abi& a = abi(of);
            return std::max<int64_t>(a.retPops, a.args.empty() ? 0 : 4 * (int64_t(a.args.back()) + 1));
        } catch (const Raised&) {
            return 0;
        }
    };
    std::vector<PassedArgument> out;
    for (const auto& [start, first] : blockValues(f, st, true)) {
        Values values = first;
        const std::vector<uint32_t>& pcs = f.block(start);
        for (size_t i = 0; i < pcs.size(); ++i) {
            const Insn& insn = f.insn(pcs[i]);
            const std::optional<int64_t> esp = st.esp(pcs[i]);
            const bool leaves = insn.is("jmp") && f.blockSuccessors(start).empty();
            if (esp && (insn.is("call") || leaves)) {
                const std::optional<uint32_t> target = insn.target();
                // the argument dwords: from [esp] at a call, from [esp + 4] at a jump (its return address is the
                // caller's)
                int64_t from = *esp, bytes = 0;
                if (leaves) {
                    from += 4;
                    bytes = std::max(target ? std::max<int64_t>(calleePops(*target), reach(*target)) : 0,
                                     reach(address));
                } else if (f.insns().contains(insn.next())) {
                    if (std::optional<int64_t> after = st.esp(insn.next())) {
                        bytes = *after - *esp;
                        const Insn& next = f.insn(insn.next());
                        if (next.is("add") && next.operands.size() == 2 && next.operands[0].type == X86_OP_REG &&
                            next.operands[0].reg == X86_REG_ESP && next.operands[1].type == X86_OP_IMM)
                            bytes += next.operands[1].imm;
                    }
                }
                for (int64_t k = 0; 4 * k < bytes; ++k)
                    if (std::optional<Value> v = held(values, from + 4 * k);
                        v && v->root >= 0 && v->loads.empty() && v->offset == 0)
                        out.push_back(PassedArgument{pcs[i], target.value_or(0), 4 * int64_t(v->root), uint32_t(k)});
            }
            step(values, st, pcs[i], insn, true);
        }
    }
    std::sort(out.begin(), out.end(), [](const PassedArgument& a, const PassedArgument& b) { return a.pc < b.pc; });
    return out;
}

// Tarjan's walk over the exits' targets, as inputsWalk's: each cycle of tail jumps is finished together, at its least
// fixpoint (each member starts with no value; a target on the cycle with none yet proves nothing)
void Facts::popsWalk(uint32_t address) {
    PopsWalk& w = popsWalk_;
    const size_t index = w.counter++;
    w.index[address] = index;
    w.low[address] = index;
    w.stack.push_back(address);
    w.nodes[address] = popsNode(address);
    std::vector<uint32_t> targets;
    for (const PopsExit& exit : w.nodes[address].exits)
        targets.insert(targets.end(), exit.targets.begin(), exit.targets.end());
    for (uint32_t target : targets) {
        if (pops_.contains(target))
            continue;
        if (auto seen = w.index.find(target); seen == w.index.end()) {
            popsWalk(target);
            if (!pops_.contains(target))
                w.low[address] = std::min(w.low[address], w.low[target]);
        } else {
            w.low[address] = std::min(w.low[address], seen->second);
        }
    }
    if (w.low[address] != index)
        return;
    std::vector<uint32_t> members;
    for (uint32_t top = 0; top != address;) {
        top = w.stack.back();
        w.stack.pop_back();
        members.push_back(top);
    }
    std::unordered_map<uint32_t, Agreement> value;
    for (bool changed = true; changed;) {
        changed = false;
        for (uint32_t m : members) {
            const PopsNode& node = w.nodes.at(m);
            Agreement v;
            if (node.ret)
                v.add(*node.ret);
            for (const PopsExit& exit : node.exits) {
                // the exit's targets agree; a finished one with unknown pops, or one on the cycle with none yet,
                // proves nothing
                Agreement t;
                if (exit.constant)
                    t.add(*exit.constant);
                for (uint32_t target : exit.targets) {
                    if (auto done = pops_.find(target); done != pops_.end()) {
                        if (done->second)
                            t.add(*done->second);
                    } else if (const Agreement& onCycle = value[target]; onCycle.conflict) {
                        t.conflict = true;
                    } else if (onCycle.value) {
                        t.add(*onCycle.value);
                    }
                }
                if (t.conflict)
                    v.conflict = true;
                else if (t.value && *t.value + exit.adjust >= 0)
                    v.add(*t.value + exit.adjust);
            }
            if (!(v == value[m])) {
                value[m] = v;
                changed = true;
            }
        }
    }
    for (uint32_t m : members) {
        const Agreement& v = value[m];
        pops_[m] = v.value && !v.conflict ? std::optional<uint32_t>(uint32_t(*v.value)) : std::nullopt;
        w.nodes.erase(m);
        w.index.erase(m);
        w.low.erase(m);
    }
}

std::optional<uint32_t> Facts::pops(uint32_t address) {
    abi(address);  // Raised where its decode raises
    if (auto found = pops_.find(address); found == pops_.end())
        popsWalk(address);
    return pops_.at(address);
}

void Facts::setSlotTargets(SlotTargets* slots) {
    slots_ = slots;
    pops_.clear();
}

Families Facts::regArgs(uint32_t address) {
    const Function* f;
    try {
        f = &function(address);
    } catch (const Raised&) {
        return 0;
    }
    const Families reads = readsBeforeWrite(*f, [&](const std::vector<uint32_t>& block, size_t i, RegisterUse& use) {
        regArgsExempt(*f, block, i, use);
    });
    return reads & (kEax | kEbx | kEsi | kEdi);
}

Families Facts::registerInputs(uint32_t address) {
    if (auto found = inputs_.find(address); found != inputs_.end())
        return found->second;
    inputsWalk(address);
    return inputs_.at(address);
}

Facts::InputNode Facts::inputNode(uint32_t address) {
    InputNode node;
    try {
        const Function& f = function(address);
        // where the code leaves for other code with a register still unwritten: a direct call's or an outward
        // jump's target gets it (a site), an indirect call or jump (not an import's, not a jump table) may read
        // the registers a standard convention passes there, ecx and edx
        auto leaves = [&](Families mask) {
            return [&f, &node, mask](const std::vector<uint32_t>& block, size_t i, Families written) {
                const Insn& insn = f.insn(block[i]);
                if (!insn.is("call") && !insn.startsWith("j"))
                    return;
                const Families unwritten = Families(~written & mask);
                if (std::optional<uint32_t> target = insn.target()) {
                    if ((*target < f.address() || *target >= f.end()) && unwritten)
                        node.sites.emplace_back(*target, unwritten);
                } else if (!insn.iatSlot() && (insn.is("call") || f.blockSuccessors(block.front()).empty())) {
                    node.local |= unwritten & (kEcx | kEdx);
                }
            };
        };
        const std::unordered_set<uint32_t> entry = entryPushes(f);
        const std::unordered_set<uint32_t> prologue = prologueRun(f);
        // a call leaves ecx as it was for all the walk knows: no MSVC code reads ecx after a call before writing it
        // unless the callee keeps it (alloca_probe, the EH prologs: 0xCC0120 is a thiscall reading ecx after one);
        // eax and edx are results (edx:eax)
        const Families viaAbi = readsBeforeWrite(
            f,
            [&](const std::vector<uint32_t>& block, size_t i, RegisterUse& use) {
                inputsExempt(f, entry, prologue, block, i, use);
            },
            leaves(kEcx | kEdx), kEax | kEdx);
        const Families viaRegArgs = readsBeforeWrite(
            f,
            [&](const std::vector<uint32_t>& block, size_t i, RegisterUse& use) { regArgsExempt(f, block, i, use); },
            leaves(kEax | kEbx | kEsi | kEdi));
        node.local |= Families((viaAbi & (kEcx | kEdx)) | (viaRegArgs & (kEax | kEbx | kEsi | kEdi)));
    } catch (const Raised&) {
        node.local = kEax | kEcx | kEdx | kEbx | kEsi | kEdi;
        node.sites.clear();
    }
    return node;
}

// Tarjan's walk over the sites: each strongly connected set of functions (a cycle of calls) is finished together,
// at its least fixpoint: a register on a cycle is read only where some member reads it itself or through a callee
// off the cycle, and reaches the others unwritten; the cycle alone proves no read
void Facts::inputsWalk(uint32_t address) {
    InputWalk& w = inputsWalk_;
    const size_t index = w.counter++;
    w.index[address] = index;
    w.low[address] = index;
    w.stack.push_back(address);
    w.nodes[address] = inputNode(address);
    // copied: the walk below adds nodes
    const std::vector<std::pair<uint32_t, Families>> sites = w.nodes[address].sites;
    for (const auto& [target, mask] : sites) {
        if (inputs_.contains(target))
            continue;
        if (auto seen = w.index.find(target); seen == w.index.end()) {
            inputsWalk(target);
            if (!inputs_.contains(target))
                w.low[address] = std::min(w.low[address], w.low[target]);
        } else {
            w.low[address] = std::min(w.low[address], seen->second);
        }
    }
    if (w.low[address] != index)
        return;
    std::vector<uint32_t> members;
    for (uint32_t top = 0; top != address;) {
        top = w.stack.back();
        w.stack.pop_back();
        members.push_back(top);
    }
    std::unordered_map<uint32_t, Families> value;
    for (uint32_t m : members)
        value[m] = w.nodes[m].local;
    for (bool changed = true; changed;) {
        changed = false;
        for (uint32_t m : members) {
            Families v = value[m];
            for (const auto& [target, mask] : w.nodes[m].sites) {
                auto done = inputs_.find(target);
                v |= (done != inputs_.end() ? done->second : value.at(target)) & mask;
            }
            if (v != value[m]) {
                value[m] = v;
                changed = true;
            }
        }
    }
    for (uint32_t m : members) {
        inputs_[m] = value[m];
        w.nodes.erase(m);
        w.index.erase(m);
        w.low.erase(m);
    }
}

bool Facts::x87Input(uint32_t address) {
    const Function* f;
    try {
        f = &function(address);
    } catch (const Raised&) {
        return false;
    }
    // x87 instructions that push without reading the stack (`fld st(i)` reads it), and those that leave it alone
    static const std::set<std::string_view> pushes = {"fld", "fild", "fbld", "fldz", "fld1", "fldpi",
                                                      "fldl2e", "fldl2t", "fldlg2", "fldln2"};
    static const std::set<std::string_view> control = {"fnstsw", "fstsw", "fldcw", "fnstcw", "fstcw", "fninit",
                                                       "finit", "fwait", "fnclex", "fclex", "fnstenv", "fstenv",
                                                       "fldenv", "fnsave", "fsave", "frstor", "fxsave", "fxrstor",
                                                       "fnop", "ffree", "fincstp", "fdecstp"};
    std::vector<uint32_t> work{f->address()};
    std::unordered_set<uint32_t> seen;
    while (!work.empty()) {
        const uint32_t start = work.back();
        work.pop_back();
        if (!seen.insert(start).second)
            continue;
        bool done = false;
        for (uint32_t pc : f->block(start)) {
            const Insn& insn = f->insn(pc);
            if (insn.is("call")) {
                done = true;
                break;
            }
            if (!(insn.sets & kX87) || control.count(insn.mnemonic))
                continue;
            if (pushes.count(insn.mnemonic) && !insn.ops.starts_with("st")) {
                done = true;
                break;
            }
            return true;
        }
        if (!done)
            for (uint32_t next : f->blockSuccessors(start))
                work.push_back(next);
    }
    return false;
}

Families Facts::clobbers(uint32_t address) {
    try {
        return function(address).clobbers();
    } catch (const Raised&) {
        return kEbx | kEsi | kEdi;
    }
}

Families Facts::unrestored(uint32_t address) {
    try {
        return function(address).unrestored();
    } catch (const Raised&) {
        return kEbx | kEsi | kEdi;
    }
}

std::optional<int32_t> Facts::variadicFixedArgs(uint32_t address) {
    // the cleanups callers make right after their direct calls to it (`add esp, N`): at least two different ones
    std::set<int64_t> cleans;
    for (uint32_t caller : program_.callers(address)) {
        const Function* f;
        try {
            f = &function(caller);
        } catch (const Raised&) {
            continue;
        }
        for (const auto& [pc, insn] : f->insns()) {
            if (!insn->is("call") || insn->target() != address)
                continue;
            const Insn* next = program_.decoder().at(insn->next());
            if (next != nullptr && next->is("add") && next->ops.starts_with("esp, ")) {
                std::optional<int64_t> amount = text::pyInt(next->ops.substr(5));
                if (!amount)
                    throw Raised("cleanup by a register");
                cleans.insert(*amount);
            }
        }
    }
    if (cleans.size() < 2)
        return std::nullopt;
    const Function* f;
    const StackWalk* st;
    try {
        f = &function(address);
        st = &stack(address);
    } catch (const Raised&) {
        return std::nullopt;
    }
    // the function takes the address of its own argument area (va_start)
    for (const auto& [pc, insn] : f->insns()) {
        if (!insn->is("lea") || insn->ops.find('*') != std::string::npos)
            continue;
        const std::string_view src = text::strip(std::string_view(insn->ops).substr(insn->ops.find(',') + 1));
        for (const auto& [reg, base] : {std::pair{"esp", st->esp(pc)}, std::pair{"ebp", st->ebp(pc)}}) {
            if (src.find(std::string("[") + reg) == std::string_view::npos || !base)
                continue;
            // Python's src[src.index(reg) + 3:-1], spaces dropped
            const size_t from = src.find(reg) + 3;
            const std::string rest = from < src.size() ? withoutSpaces(src.substr(from, src.size() - 1 - from)) : "";
            std::optional<int64_t> value = rest.empty() ? std::optional<int64_t>(0) : text::pyInt(rest);
            if (!value)
                continue;
            const int64_t where = *base + *value;
            // past at least one fixed argument, and some caller passes only the fixed arguments
            if (where >= 8) {
                const int64_t fixed = (where - 4) / 4;
                return *cleans.begin() == 4 * fixed ? std::optional<int32_t>(int32_t(fixed)) : std::nullopt;
            }
        }
    }
    return std::nullopt;
}

uint32_t Facts::frameAlign(uint32_t address) {
    const Function* f;
    try {
        f = &function(address);
    } catch (const Raised&) {
        return 0;
    }
    int count = 0;
    for (const auto& [pc, insn] : f->insns()) {
        if (count++ == 8)
            break;
        if (insn->is("and") && insn->ops.starts_with("esp, ")) {
            std::optional<int64_t> value = text::pyInt(partition(insn->ops).second);
            if (!value)
                throw Raised("`and esp` by a register");
            const uint32_t mask = uint32_t(*value);
            return mask == 0xFFFFFFF0 || mask == 0xFFFFFFF8 || mask == 0xFFFFFFC0 || mask == 0xFFFFFFE0 ? -mask : 0;
        }
    }
    return 0;
}

InstructionSets Facts::instructionSets(uint32_t address) {
    const Function* f;
    try {
        f = &function(address);
    } catch (const Raised&) {
        return 0;
    }
    InstructionSets sets = 0;
    for (const auto& [pc, insn] : f->insns())
        sets |= insn->sets;
    return sets;
}

std::optional<Facts::EaxReturns> Facts::eaxReturns(uint32_t address, bool tempsReturn) {
    const Function* f;
    try {
        f = &function(address);
    } catch (const Raised&) {
        return std::nullopt;
    }
    std::vector<const Insn*> insns;
    for (const auto& [pc, insn] : f->insns())
        insns.push_back(insn);
    const int n = int(insns.size());
    // the jumps into each instruction
    std::map<int64_t, std::vector<int>> sources;
    for (int i = 0; i < n; ++i)
        if (insns[i]->startsWith("j"))
            if (std::optional<int64_t> to = text::pyInt(insns[i]->ops))
                sources[*to].push_back(i);
    auto dest = [](const Insn& insn) { return withoutDwordPtr(partition(insn.ops).first); };
    // ecx holds `this` until the first instruction that writes it, a call, or a jump target
    int ecxEnd = n;
    for (int i = 0; i < n; ++i) {
        const Insn& insn = *insns[i];
        const std::string d = dest(insn);
        if (insn.is("call") || sources.contains(insn.address) ||
            (!insn.is("cmp") && !insn.is("test") && !insn.is("push") && (d == "ecx" || d == "cx" || d == "cl"))) {
            ecxEnd = i;
            break;
        }
    }
    // `this` holders: registers / ebp slots a `mov` copies ecx (while it is `this`) or another holder into, written
    // once
    std::map<std::string, int> writes;
    for (const Insn* insn : insns)
        if (!insn->is("cmp") && !insn->is("test") && !insn->is("push") && !insn->is("pop") && !insn->ops.empty())
            ++writes[dest(*insn)];
    auto stable = [&](const std::string& place) {
        auto found = writes.find(place);
        if (found == writes.end() || found->second != 1)
            return false;
        if (place == "ebx" || place == "esi" || place == "edi" || place == "ebp")
            return true;
        // `[ebp - \w+]`
        if (!place.starts_with("[ebp - ") || !place.ends_with("]") || place.size() <= 8)
            return false;
        return std::all_of(place.begin() + 7, place.end() - 1,
                           [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; });
    };
    std::set<std::string> holders;
    for (int i = 0; i < n; ++i) {
        const auto [dstText, srcText] = partition(insns[i]->ops);
        const std::string dst = withoutDwordPtr(dstText), src = withoutDwordPtr(srcText);
        if (insns[i]->is("mov") && stable(dst) && ((src == "ecx" && i < ecxEnd) || holders.contains(src)))
            holders.insert(dst);
    }
    auto writesEax = [](const Insn& insn) {
        const std::string& m = insn.mnemonic;
        if (m == "cmp" || m == "test" || m == "push" || m == "out" || m == "call" || insn.startsWith("j"))
            return false;
        const std::string_view d = partition(insn.ops).first;
        if (d == "eax" || d == "ax" || d == "al" || d == "ah")
            return true;
        return m == "cwde" || m == "cbw" || m == "mul" || m == "div" || m == "idiv" || insn.startsWith("lods") ||
               (m == "imul" && insn.ops.find(',') == std::string::npos) ||
               (m == "xchg" && insn.ops.find("eax") != std::string::npos);
    };
    // eax (or a part) as an operand that is read: a source, a memory operand's register, or a destination the
    // instruction also reads (`add eax, 4`)
    auto namesEax = [](std::string_view text) {
        auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
        for (std::string_view name : {"eax", "ax", "al", "ah"})
            for (size_t at = text.find(name); at != std::string_view::npos; at = text.find(name, at + 1))
                if ((at == 0 || !word(text[at - 1])) &&
                    (at + name.size() == text.size() || !word(text[at + name.size()])))
                    return true;
        return false;
    };
    auto readsEax = [&](const Insn& insn) {
        const auto [dst, src] = partition(insn.ops);
        const bool pureWrite = insn.is("mov") || insn.is("lea") || insn.is("movzx") || insn.is("movsx") ||
                               insn.is("pop") || insn.startsWith("set");
        if (namesEax(src))
            return true;
        if (dst.find('[') != std::string_view::npos)
            return namesEax(dst);
        return !pureWrite && namesEax(dst);
    };
    auto classify = [&](int i) -> EaxKind {
        const Insn& insn = *insns[i];
        if (!insn.is("mov") || !insn.ops.starts_with("eax, "))
            return kValue;
        const std::string src = withoutDwordPtr(partition(insn.ops).second);
        return holders.contains(src) || (src == "ecx" && i < ecxEnd) ? kSelf : kValue;
    };
    auto preds = [&](int i) {
        std::vector<int> out;
        if (auto found = sources.find(insns[i]->address); found != sources.end())
            out = found->second;
        if (i == 0)
            out.push_back(-1);
        else if (!insns[i - 1]->is("jmp") && !insns[i - 1]->is("ret"))
            out.push_back(i - 1);
        return out;
    };
    // __security_check_cookie keeps eax
    const std::string cookieCheck = "0xec408c";
    uint8_t kinds = 0;
    std::vector<const Insn*> definitions;
    // a tail jump out of the function returns the target's eax
    for (const Insn* insn : insns)
        if (insn->is("jmp"))
            if (std::optional<int64_t> to = text::pyInt(insn->ops);
                to && (*to < 0 || *to > UINT32_MAX || !f->insns().contains(uint32_t(*to))))
                kinds |= kCall;
    // each ret's reaching eax definitions, searched backwards over fallthroughs and the jumps into each instruction
    // (with !tempsReturn, a definition whose value is read again before the ret, `mov eax, [ebp-4]; mov [eax+8],
    // ecx`, is a temporary: kTemp, not a return value)
    for (int k = 0; k < n; ++k) {
        if (!insns[k]->is("ret"))
            continue;
        std::vector<std::pair<int, bool>> stack;
        for (int p : preds(k))
            stack.push_back({p, false});
        std::set<std::pair<int, bool>> seen;
        while (!stack.empty()) {
            const auto [j, read] = stack.back();
            stack.pop_back();
            if (j < 0) {
                kinds |= kUnset;
                continue;
            }
            if (!seen.insert({j, read}).second)
                continue;
            const Insn& insn = *insns[j];
            if (insn.is("call") && insn.ops != cookieCheck) {
                kinds |= kCall;
            } else if (writesEax(insn)) {
                const EaxKind kind = read && !tempsReturn ? kTemp : classify(j);
                kinds |= kind;
                if (kind == kSelf || kind == kValue)
                    definitions.push_back(&insn);
            } else {
                const bool now = read || (!tempsReturn && readsEax(insn));
                for (int p : preds(j))
                    stack.push_back({p, now});
            }
        }
    }
    // the eax writes right before the epilogue (leavesEax's return values where some path returns no `this`)
    uint8_t near = 0;
    for (int k = 0; k < n; ++k) {
        if (!insns[k]->is("ret"))
            continue;
        std::vector<int> stack = preds(k);
        std::unordered_set<int> seen;
        while (!stack.empty()) {
            const int j = stack.back();
            stack.pop_back();
            if (j < 0 || !seen.insert(j).second) {
                near |= kUnset;
                continue;
            }
            const Insn& insn = *insns[j];
            const std::string& m = insn.mnemonic;
            const std::string& ops = insn.ops;
            const bool epilogue = (m == "pop" && ops != "eax") || m == "leave" || m == "jmp" || ops == "esp, ebp" ||
                                  ops == "ecx, ebp" || ops == "ecx, esp" || ops == "dword ptr fs:[0], ecx" ||
                                  (m == "add" && ops.starts_with("esp,")) ||
                                  (m == "mov" && ops.starts_with("ecx, ")) || (m == "call" && ops == cookieCheck);
            if (epilogue) {
                std::vector<int> more = preds(j);
                stack.insert(stack.end(), more.begin(), more.end());
            } else if (writesEax(insn)) {
                near |= classify(j);
            } else {
                near |= kUnset;
            }
        }
    }
    return EaxReturns{kinds, near, std::move(definitions)};
}

bool Facts::leavesEax(uint32_t address, bool tempsReturn) {
    std::optional<EaxReturns> returns = eaxReturns(address, tempsReturn);
    if (!returns)
        return false;
    // every path returns `this`
    if (returns->kinds == kSelf)
        return true;
    // else only an eax write right before the epilogue counts as a return value
    return (returns->near & ~kUnset) != 0;
}

bool Facts::returnsThis(uint32_t address) {
    const Function* f;
    const StackWalk* walk;
    // a realigned frame's slots by their offsets relative to each other, past its `and esp` (0xD56AB0 keeps this in
    // a local while ebx holds other values, and returns it from there): MSVC addresses such a frame's arguments
    // through an ebp set before the `and` (or through ebx, which the walk does not follow) and its locals through esp
    // or an ebp set after it, so the padding the two views differ by joins no slot of one to the other
    std::optional<StackWalk> relative;
    try {
        f = &function(address);
        if (frameAlign(address) != 0)
            relative.emplace(*this, *f, StackWalk::View::kRelative);
        walk = relative ? &*relative : &stack(address);
    } catch (const Raised&) {
        return false;
    }
    // the places holding the entry ecx: the general registers (ebp too, where it is no frame pointer), and the stack
    // dwords by their offset from the entry esp
    struct Holders {
        Families registers = kEcx;
        std::set<int64_t> slots;
        bool operator==(const Holders&) const = default;
    };
    auto bit = [](x86_reg reg) -> Families {
        switch (reg) {
        case X86_REG_EAX: return kEax;
        case X86_REG_ECX: return kEcx;
        case X86_REG_EDX: return kEdx;
        case X86_REG_EBX: return kEbx;
        case X86_REG_EBP: return kEbp;
        case X86_REG_ESI: return kEsi;
        case X86_REG_EDI: return kEdi;
        default: return 0;
        }
    };
    auto dword = [&](const Operand& op) { return op.type == X86_OP_REG && op.size == 4 && bit(op.reg) != 0; };
    // a direct callee whose code writes no ecx, calls nothing and leaves only by its rets keeps ecx: link-time code
    // generation lets the caller hold this in ecx across its call (0xD2C760 returns ecx after setMemory 0xD2C3A0)
    auto keepsEcx = [&](const Insn& call) {
        std::optional<uint32_t> target = call.target();
        if (!target)
            return false;
        try {
            const Function& callee = function(*target);
            if (!callee.tailcalls().empty())
                return false;
            for (const auto& [pc, insn] : callee.insns())
                if (insn->is("call") || (insn->is("jmp") && !insn->target()) || (registerUse(*insn).writes & kEcx))
                    return false;
            return true;
        } catch (const Raised&) {
            return false;
        }
    };
    // the holders after one instruction: a dword copy (mov, lea of [reg], xchg) between registers and stack slots
    // carries the value, any other write of a register or a stack slot ends it, a call clobbers eax, ecx and edx
    auto step = [&](Holders h, const Insn& insn) {
        const std::vector<Operand>& ops = insn.operands;
        auto slot = [&](const Operand& op) -> std::optional<int64_t> {
            if (op.type != X86_OP_MEM)
                return std::nullopt;
            return walk->offset(insn.address, op);
        };
        auto holds = [&](const Operand& op) {
            if (dword(op))
                return (h.registers & bit(op.reg)) != 0;
            std::optional<int64_t> at = op.size == 4 ? slot(op) : std::nullopt;
            return at && h.slots.contains(*at);
        };
        if (ops.size() == 2 && insn.is("xchg") && dword(ops[0]) && dword(ops[1])) {
            const bool a = holds(ops[0]), b = holds(ops[1]);
            h.registers = Families(h.registers & ~(bit(ops[0].reg) | bit(ops[1].reg)));
            h.registers |= Families((b ? bit(ops[0].reg) : 0) | (a ? bit(ops[1].reg) : 0));
            return h;
        }
        bool carried = false;
        if (ops.size() == 2 && insn.is("mov"))
            carried = holds(ops[1]);
        else if (ops.size() == 2 && insn.is("lea") && ops[1].type == X86_OP_MEM && ops[1].index == X86_REG_INVALID &&
                 ops[1].disp == 0 && bit(ops[1].base))
            carried = (h.registers & bit(ops[1].base)) != 0;
        RegisterUse use = registerUse(insn);
        // a call clobbers eax, ecx and edx, except __security_check_cookie, which keeps eax, and a callee that keeps
        // ecx
        if (insn.is("call"))
            use.writes |= insn.target() == 0xec408cu ? kEcx : keepsEcx(insn) ? kEax | kEdx : kEax | kEcx | kEdx;
        h.registers = Families(h.registers & ~use.writes);
        const bool writesFirst = !ops.empty() && !insn.is("cmp") && !insn.is("test") && !insn.is("push") &&
                                 !insn.startsWith("j") && !insn.is("call");
        std::optional<int64_t> written = writesFirst ? slot(ops[0]) : std::nullopt;
        if (insn.is("push"))
            if (std::optional<int64_t> esp = walk->esp(insn.address))
                written = *esp - 4;
        if (written)
            for (auto it = h.slots.begin(); it != h.slots.end();)  // any stack write over the slot
                it = *it < *written + ops[0].size && *written < *it + 4 ? h.slots.erase(it) : std::next(it);
        if (carried && dword(ops[0]))
            h.registers |= bit(ops[0].reg);
        else if (carried && written && ops[0].size == 4)
            h.slots.insert(*written);
        return h;
    };
    // forward over the blocks, a block's entry state the intersection of its predecessors' exits
    std::map<uint32_t, Holders> in{{f->address(), Holders{}}};
    std::vector<uint32_t> work{f->address()};
    bool sawRet = false;
    while (!work.empty()) {
        const uint32_t start = work.back();
        work.pop_back();
        Holders h = in.at(start);
        const std::vector<uint32_t>& pcs = f->block(start);
        for (uint32_t pc : pcs) {
            const Insn& insn = f->insn(pc);
            if (insn.is("ret")) {
                if (!(h.registers & kEax))
                    return false;
                sawRet = true;
            }
            h = step(std::move(h), insn);
        }
        // a jump out of the function (a tail call, or to an unknown target not a switch's) returns its target's eax
        const Insn& last = f->insn(pcs.back());
        if (last.is("jmp") && (last.target() ? !f->insns().contains(*last.target())
                                             : f->blockSuccessors(start).empty()))
            return false;
        for (uint32_t next : f->blockSuccessors(start)) {
            auto [found, fresh] = in.try_emplace(next, h);
            if (!fresh) {
                Holders merged{Families(found->second.registers & h.registers), {}};
                for (int64_t slot : found->second.slots)
                    if (h.slots.contains(slot))
                        merged.slots.insert(slot);
                if (merged == found->second)
                    continue;
                found->second = std::move(merged);
            }
            work.push_back(next);
        }
    }
    return sawRet;
}

std::optional<ReturnWidth> Facts::returnWidth(uint32_t address) {
    std::optional<EaxReturns> returns = eaxReturns(address, true);
    if (!returns || returns->definitions.empty() || (returns->kinds & ~(kSelf | kValue)) != 0)
        return std::nullopt;
    std::optional<ReturnWidth> out;
    bool zero = true, sign = true;
    uint32_t raw = 0;
    for (const Insn* insn : returns->definitions) {
        const std::string_view dest = partition(insn->ops).first;
        const std::string& m = insn->mnemonic;
        uint8_t width;
        if (m == "cbw")
            width = 2;
        else if (m == "cwde" || dest == "eax" || m == "mul" || m == "div" || m == "idiv" || m == "imul" ||
                 m == "xchg" || m == "lodsd")
            width = 4;
        else if (dest == "ax" || m == "lodsw")
            width = 2;
        else if (dest == "al" || m == "lodsb")
            width = 1;
        else
            return std::nullopt;  // `mov ah`: no low-bytes width
        zero = zero && m == "movzx";
        sign = sign && (m == "movsx" || m == "cwde" || m == "cbw");
        // a bool's 0 or 1: a setcc, a mov of 0 or 1, an xor of the register with itself, a load of a frame byte
        // that only such stores write (a debug build's bool local: 0x4570D0's `mov al, [ebp-1]`)
        const std::vector<Operand>& ops = insn->operands;
        auto zeroOrOne = [](const Insn& i) {
            return i.startsWith("set") || (i.is("mov") && i.operands.size() == 2 && i.operands[1].type == X86_OP_IMM &&
                                           (i.operands[1].imm == 0 || i.operands[1].imm == 1));
        };
        auto boolLocal = [&]() {
            if (!insn->is("mov") || ops.size() != 2 || ops[1].type != X86_OP_MEM || ops[1].size != 1)
                return false;
            try {
                const Function& f = function(address);
                const StackWalk& st = stack(address);
                const std::optional<int64_t> slot = st.offset(insn->address, ops[1]);
                if (!slot || *slot >= 0)
                    return false;
                for (const auto& [pc, other] : f.insns()) {
                    const Operand* op = other->mem();
                    const std::optional<int64_t> at = op ? st.offset(pc, *op) : std::nullopt;
                    if (!at || *at > *slot || *at + op->size <= *slot)
                        continue;
                    const ArgumentAccess::Kind kind = accessKind(*other, op);
                    if (kind == ArgumentAccess::Address ||
                        (kind == ArgumentAccess::Write && !(op->size == 1 && zeroOrOne(*other))))
                        return false;
                }
                return true;
            } catch (const Raised&) {
                return false;
            }
        };
        const bool boolean = zeroOrOne(*insn) ||
                             (m == "xor" && ops.size() == 2 && ops[0].type == X86_OP_REG &&
                              ops[1].type == X86_OP_REG && ops[0].reg == ops[1].reg) ||
                             boolLocal();
        if (!boolean && (!raw || insn->address < raw))
            raw = insn->address;
        if (!out || width < out->width)
            out = ReturnWidth{width, 0, 0};
    }
    out->extension = zero ? 1 : sign ? -1 : 0;
    out->raw = raw;
    return out;
}

bool Facts::throws(uint32_t target) {
    const FunctionEntry* f = program_.at(target);
    if (f == nullptr)
        return false;
    if (auto found = throws_.find(target); found != throws_.end())
        return found->second;
    throws_[target] = false;  // while being worked out (a call back into it): not known to throw
    // the CRT's _CxxThrowException, by its IDA name
    std::string_view name = f->name;
    for (int i = 0; i < 2 && name.starts_with("_"); ++i)
        name.remove_prefix(1);
    if (name == "CxxThrowException" || name == "CxxThrowException@8") {
        throws_[target] = true;
        return true;
    }
    auto imm = [](const Insn& insn) -> std::optional<uint32_t> {
        std::optional<int64_t> value = text::pyInt(insn.ops);
        return value ? std::optional(uint32_t(*value)) : std::nullopt;
    };
    // every path from the entry, within the bounds, reaches a throwing call or a trap before a `ret` or a jump the
    // walk cannot follow
    auto walk = [&] {
        std::vector<uint32_t> work{target};
        std::unordered_set<uint32_t> seen;
        while (!work.empty()) {
            uint32_t pc = work.back();
            work.pop_back();
            while (seen.insert(pc).second) {
                if (pc < target || pc >= target + f->size)
                    return false;
                const Insn* insn = program_.decoder().at(pc);
                if (insn == nullptr)
                    return false;
                const std::optional<uint32_t> to = imm(*insn);
                if (insn->is("ret") || (insn->is("jmp") && !to))
                    return false;
                if ((insn->is("call") && to && throws(*to)) || insn->is("int3") || insn->is("hlt") || insn->is("ud2"))
                    break;
                if (insn->startsWith("j") || insn->startsWith("loop")) {
                    if (!to)
                        return false;
                    work.push_back(*to);
                    if (insn->is("jmp"))
                        break;
                }
                pc += insn->size;
            }
        }
        return true;
    };
    const bool result = walk();
    throws_[target] = result;
    return result;
}

namespace {

// A value the trivial-accessor walk tracks: a register's entry value, entry esp plus an offset, an object pointer plus
// an offset, a field loaded from an object, a stack argument's low bytes, or anything else
struct Tracked {
    enum Kind : uint8_t { kOther, kEntry, kStack, kPointer, kField, kArgument } kind = kOther;
    // kEntry: the register's index; kPointer, kField: the object (-1 ecx, else the stack argument slot holding it);
    // kArgument: the stack argument slot
    int32_t base = 0;
    // kStack: from entry esp; kPointer: from the object; kField: the field's offset in the object
    int32_t offset = 0;
    // kField: as Accessor's; kArgument: the bytes read (1 or 2; a whole argument is a kPointer)
    uint8_t width = 0;
    int8_t extension = 0;

    bool operator==(const Tracked&) const = default;
};

// A register operand's index (eax ecx edx ebx esp ebp esi edi) and width in bytes, width 0 for the registers the walk
// does not track (al, ax, cl, cx, dl and dx only: the low bytes a load or a setter's argument goes into)
std::pair<int, int> registerSlot(x86_reg reg) {
    switch (reg) {
    case X86_REG_EAX: return {0, 4};
    case X86_REG_AX: return {0, 2};
    case X86_REG_AL: return {0, 1};
    case X86_REG_ECX: return {1, 4};
    case X86_REG_CX: return {1, 2};
    case X86_REG_CL: return {1, 1};
    case X86_REG_EDX: return {2, 4};
    case X86_REG_DX: return {2, 2};
    case X86_REG_DL: return {2, 1};
    case X86_REG_EBX: return {3, 4};
    case X86_REG_ESP: return {4, 4};
    case X86_REG_EBP: return {5, 4};
    case X86_REG_ESI: return {6, 4};
    case X86_REG_EDI: return {7, 4};
    default: return {0, 0};
    }
}

}  // namespace

std::optional<Accessor> Facts::trivialAccessor(uint32_t address) {
    bool empty = false;
    std::optional<Accessor> accessor = trivialWalk(address, empty);
    return empty ? std::nullopt : accessor;
}

std::optional<uint32_t> Facts::emptyFunction(uint32_t address) {
    bool empty = false;
    std::optional<Accessor> body = trivialWalk(address, empty);
    return body && empty ? std::optional<uint32_t>(body->pops) : std::nullopt;
}

std::optional<Accessor> Facts::trivialWalk(uint32_t address, bool& empty) {
    if (program_.at(address) == nullptr)
        return std::nullopt;
    constexpr int kEaxSlot = 0, kEcxSlot = 1, kEspSlot = 4;
    Tracked regs[8];
    for (int r = 0; r < 8; ++r)
        regs[r] = {Tracked::kEntry, r};
    regs[kEcxSlot] = {Tracked::kPointer, -1};
    regs[kEspSlot] = {Tracked::kStack};
    // the dwords the body pushed or stored below its entry esp, by offset from it
    std::map<int32_t, Tracked> frame;
    int loads = 0;
    // the one float field `fld` pushes (a float getter's ST0 result)
    Tracked st0;
    // the one store of an argument into an object (a setter)
    std::optional<Accessor> store;
    // A memory operand's address: a stack offset or an object pointer plus offset (kOther for anything else: an
    // index, an absolute address, a base the walk does not know)
    auto locate = [&](const Operand& op) -> Tracked {
        const auto [r, width] = registerSlot(op.base);
        if (op.index != X86_REG_INVALID || width != 4)
            return {};
        Tracked at = regs[r];
        if (at.kind != Tracked::kStack && at.kind != Tracked::kPointer)
            return {};
        at.offset += int32_t(op.disp);
        return at;
    };
    // A read: a frame dword the body wrote, a stack argument (an object pointer, or a narrower argument's low
    // bytes), or the one field load
    auto read = [&](const Operand& op, int8_t extension) -> Tracked {
        const Tracked at = locate(op);
        if (at.kind == Tracked::kStack) {
            if (op.size != 4)
                return at.offset >= 4 && at.offset % 4 == 0 && !frame.contains(at.offset)
                           ? Tracked{Tracked::kArgument, (at.offset - 4) / 4, 0, uint8_t(op.size)}
                           : Tracked{};
            if (auto slot = frame.find(at.offset); slot != frame.end())
                return slot->second;
            if (at.offset >= 4 && at.offset % 4 == 0)
                return {Tracked::kPointer, (at.offset - 4) / 4};
            return {};
        }
        if (at.kind != Tracked::kPointer || ++loads > 1)
            return {};
        return {Tracked::kField, at.base, at.offset, op.size, extension};
    };
    for (uint32_t pc = address, count = 0; count < 16; ++count) {
        const Insn* insn = program_.decoder().at(pc);
        if (insn == nullptr)
            return std::nullopt;
        pc = insn->next();
        const std::vector<Operand>& ops = insn->operands;
        // the destination register (a whole register, or al/ax for a load), when the first operand is one
        const auto [d, dWidth] = !ops.empty() && ops[0].type == X86_OP_REG ? registerSlot(ops[0].reg)
                                                                            : std::pair<int, int>{0, 0};
        Tracked& esp = regs[kEspSlot];
        if (insn->is("ret")) {
            // a setter may leave eax, ecx and edx changed
            for (int r : {2, 3, 5, 6, 7})
                if ((r != 2 || !store) && regs[r] != Tracked{Tracked::kEntry, r})
                    return std::nullopt;
            const Tracked& eax = regs[kEaxSlot];
            if (esp != Tracked{Tracked::kStack} || (!store && regs[kEcxSlot] != Tracked{Tracked::kPointer, -1}))
                return std::nullopt;
            const uint32_t pops = insn->imm().value_or(0);
            if (store) {
                if (loads > 0 || st0.kind != Tracked::kOther)
                    return std::nullopt;
                store->pops = pops;
                return store;
            }
            // an empty body: nothing loaded, eax as it was at entry (the other registers are checked above)
            if (loads == 0 && st0.kind == Tracked::kOther && eax == Tracked{Tracked::kEntry, kEaxSlot}) {
                empty = true;
                return Accessor{.pops = pops};
            }
            if (st0.kind == Tracked::kField)
                return Accessor{true, true, st0.base, st0.offset, st0.width, 0, pops};
            if (eax.kind != Tracked::kPointer && eax.kind != Tracked::kField)
                return std::nullopt;
            return Accessor{eax.kind == Tracked::kField, false, eax.base, eax.offset, eax.width, eax.extension, pops};
        }
        if (insn->is("push") && dWidth == 4 && esp.kind == Tracked::kStack) {
            esp.offset -= 4;
            frame[esp.offset] = regs[d];
        } else if (insn->is("pop") && dWidth == 4 && esp.kind == Tracked::kStack && frame.contains(esp.offset)) {
            regs[d] = frame[esp.offset];
            esp.offset += 4;
        } else if (insn->is("mov") && ops.size() == 2 && dWidth == 4 && ops[1].type == X86_OP_REG &&
                   registerSlot(ops[1].reg).second == 4) {
            regs[d] = regs[registerSlot(ops[1].reg).first];
        } else if (insn->is("mov") && ops.size() == 2 && dWidth == 4 && ops[1].type == X86_OP_MEM) {
            regs[d] = read(ops[1], 0);
        } else if (insn->is("mov") && ops.size() == 2 && dWidth != 0 && dWidth < 4 && ops[1].type == X86_OP_MEM) {
            // `mov al/ax, [object + N]`: the field into eax's low bytes; `mov cl, [esp + N]`: a setter's argument
            regs[d] = read(ops[1], 0);
            if (regs[d].kind != Tracked::kField && regs[d].kind != Tracked::kArgument)
                return std::nullopt;
        } else if (insn->is("mov") && ops.size() == 2 && ops[0].type == X86_OP_MEM && ops[1].type == X86_OP_REG &&
                   registerSlot(ops[1].reg).second == ops[0].size) {
            // a dword store into the body's own frame (the `this` spill), or the one store of an argument (whole, or
            // the low bytes it was read as) into an object
            const Tracked at = locate(ops[0]);
            const Tracked value = regs[registerSlot(ops[1].reg).first];
            const bool argument = (value.kind == Tracked::kPointer && value.base >= 0 && value.offset == 0) ||
                                  (value.kind == Tracked::kArgument && value.width == ops[0].size);
            if (at.kind == Tracked::kStack && at.offset < 0 && ops[0].size == 4)
                frame[at.offset] = value;
            else if (at.kind == Tracked::kPointer && !store && argument)
                store = Accessor{false, false, at.base, at.offset, uint8_t(ops[0].size), 0, 0, true, value.base};
            else
                return std::nullopt;
        } else if ((insn->is("movzx") || insn->is("movsx")) && ops.size() == 2 && dWidth == 4 &&
                   ops[1].type == X86_OP_MEM) {
            regs[d] = read(ops[1], insn->is("movzx") ? 1 : -1);
            if (regs[d].kind != Tracked::kField)
                return std::nullopt;
        } else if (insn->is("fld") && ops.size() == 1 && ops[0].type == X86_OP_MEM && st0.kind == Tracked::kOther) {
            // `fld dword/qword/tbyte [object + N]`: the float field into ST0
            st0 = read(ops[0], 0);
            if (st0.kind != Tracked::kField)
                return std::nullopt;
        } else if (insn->is("lea") && ops.size() == 2 && dWidth == 4) {
            regs[d] = locate(ops[1]);
        } else if ((insn->is("add") || insn->is("sub")) && ops.size() == 2 && dWidth == 4 &&
                   ops[1].type == X86_OP_IMM &&
                   (regs[d].kind == Tracked::kStack || regs[d].kind == Tracked::kPointer)) {
            regs[d].offset += int32_t(insn->is("add") ? ops[1].imm : -ops[1].imm);
        } else {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

}  // namespace exefacts
