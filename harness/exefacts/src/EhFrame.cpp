// Facts::ehFrame: a function's MSVC exception frame, read from its code (Facts.hpp's EhFrame).
#include "Facts.hpp"

#include <algorithm>

namespace exefacts {

namespace {

// an instruction with an fs: operand (the TIB's exception list)
bool touchesFs(const Insn& insn) {
    return insn.ops.find("fs:") != std::string::npos;
}

// the first operand is fs: (a store to the exception list)
bool storesFs(const Insn& insn) {
    return insn.is("mov") && text::parts(insn.ops)[0].find("fs:") != std::string_view::npos;
}

// `push imm`'s value as a dword
std::optional<uint32_t> pushedImmediate(const Insn& insn) {
    if (!insn.is("push") || insn.operands.size() != 1 || insn.operands[0].type != X86_OP_IMM)
        return std::nullopt;
    return uint32_t(insn.operands[0].imm);
}

// an address inside the image (a handler, a scope table)
bool imageAddress(std::optional<uint32_t> value) {
    return value && *value >= 0x400000 && *value < 0x2000000;
}

// a `mov` to [ebp - 4], the try level of a frame __SEH_prolog4 built (ebp is its, not the caller's)
bool storesTryLevel(const Insn& insn) {
    if (!insn.is("mov") || insn.operands.empty())
        return false;
    const Operand& to = insn.operands[0];
    return to.type == X86_OP_MEM && to.base == X86_REG_EBP && to.index == X86_REG_INVALID && to.disp == -4;
}

}  // namespace

// The handler a frame-registration helper (__SEH_prolog4) pushes for its caller: its first instruction pushes an
// image address and it stores fs:[0] before its ret (decoded straight, at most 32 instructions); nullopt for any
// other function (_EH_prolog3 pushes eax, the caller's C++ handler)
std::optional<uint32_t> Facts::helperHandler(uint32_t address) {
    std::optional<uint32_t> handler;
    uint32_t pc = address;
    for (int k = 0; k < 32; ++k) {
        const Insn* insn = program_.decoder().at(pc);
        if (insn == nullptr || insn->is("ret") || insn->startsWith("j") || insn->is("call"))
            return std::nullopt;
        if (k == 0) {
            handler = pushedImmediate(*insn);
            if (!imageAddress(handler))
                return std::nullopt;
        }
        if (storesFs(*insn))
            return handler;
        pc = insn->next();
    }
    return std::nullopt;
}

const std::optional<EhFrame>& Facts::ehFrame(uint32_t address) {
    auto found = ehFrames_.find(address);
    if (found == ehFrames_.end())
        found = ehFrames_.emplace(address, readEhFrame(address)).first;
    return found->second;
}

std::optional<EhFrame> Facts::readEhFrame(uint32_t address) {
    const Function& f = function(address);
    std::vector<const Insn*> insns;
    for (const auto& [pc, insn] : f.insns())
        insns.push_back(insn);

    // an SEH frame a helper registers: `push N; push scopetable; call H` as the first call of the first 10
    // instructions, H pushing the handler and storing fs:[0] (helperHandler). The function itself need not touch fs:
    // (__SEH_epilog4 unregisters it); its try level is [ebp - 4], ebp the frame H builds (entry - 4)
    for (size_t k = 0; k < std::min<size_t>(insns.size(), 10); ++k) {
        if (!insns[k]->is("call"))
            continue;
        std::optional<uint32_t> target = insns[k]->target();
        std::optional<uint32_t> scope = k >= 2 ? pushedImmediate(*insns[k - 1]) : std::nullopt;
        if (!target || !imageAddress(scope) || !pushedImmediate(*insns[k - 2]) || !helperHandler(*target))
            break;
        EhFrame frame;
        frame.kind = EhFrame::kSeh;
        frame.scopeTable = *scope;
        frame.stateSlot = -8;
        frame.registration = {insns[k - 2]->address, insns[k - 1]->address, insns[k]->address};
        for (size_t j = k + 1; j < insns.size(); ++j)
            if (storesTryLevel(*insns[j]))
                frame.stores.push_back(insns[j]->address);
        return frame;
    }
    if (std::none_of(insns.begin(), insns.end(), [](const Insn* insn) { return touchesFs(*insn); }))
        return std::nullopt;

    // the prologue: up to the registration (`mov fs:[0], reg`) within 48 instructions, else the first 10. A
    // realigned frame's EH prologue follows its `and esp` (PlayerCharacter::Update 0x93E860: `push -1` is the 10th
    // instruction, its handler push the 11th)
    size_t prologue = 0;
    while (prologue < insns.size() && prologue < 48 && !storesFs(*insns[prologue]))
        ++prologue;
    prologue = prologue < insns.size() && prologue < 48 ? prologue + 1 : std::min<size_t>(insns.size(), 10);

    // the handler: an image address pushed right after a `push -1` (the previous immediate pushed); an SEH frame's
    // is the second of two pushed back to back after a `push -1` (SEH3) or `push -2` (SEH4), the first its scope
    // table
    const Insn* minusOne = nullptr;
    const Insn* handlerPush = nullptr;
    const Insn* scopePush = nullptr;
    std::optional<uint32_t> previous;
    for (size_t k = 0; k < prologue && !handlerPush; ++k) {
        std::optional<uint32_t> value = pushedImmediate(*insns[k]);
        if (!value)
            continue;
        if ((previous == 0xFFFFFFFFu || previous == 0xFFFFFFFEu) && imageAddress(value)) {
            if (k + 1 < prologue && imageAddress(pushedImmediate(*insns[k + 1]))) {
                scopePush = insns[k];
                handlerPush = insns[k + 1];
            } else if (previous == 0xFFFFFFFFu) {
                handlerPush = insns[k];
            } else {
                previous = value;
            }
        } else {
            previous = value;
        }
    }
    EhFrame frame;
    if (!handlerPush)
        return frame;
    for (size_t k = 0; k < prologue; ++k)
        if (std::optional<uint32_t> value = pushedImmediate(*insns[k]);
            value == 0xFFFFFFFFu || (scopePush && value == 0xFFFFFFFEu)) {
            minusOne = insns[k];
            break;
        }

    // the handler thunk: `mov eax, funcinfo; jmp __CxxFrameHandler3`
    frame.kind = EhFrame::kOther;
    std::optional<uint32_t> funcInfo;
    bool cxx = false;
    uint32_t pc = *handlerPush->imm();
    for (int k = 0; k < 16 && !scopePush; ++k) {
        const Insn* insn = program_.decoder().at(pc);
        if (insn == nullptr)
            break;
        const std::vector<Operand>& ops = insn->operands;
        if (insn->is("mov") && ops.size() == 2 && ops[0].type == X86_OP_REG && ops[0].reg == X86_REG_EAX &&
            ops[1].type == X86_OP_IMM)
            funcInfo = uint32_t(ops[1].imm);
        if (insn->is("jmp")) {
            std::optional<uint32_t> target = insn->target();
            const FunctionEntry* personality = target ? program_.at(*target) : nullptr;
            cxx = personality != nullptr && personality->name.find("CxxFrameHandler") != std::string::npos;
            break;
        }
        pc = insn->next();
    }
    if (cxx && funcInfo) {
        frame.kind = EhFrame::kCxx;
        frame.tryCount = program_.image().u32(*funcInfo + 12);
    }
    if (scopePush) {
        frame.kind = EhFrame::kSeh;
        frame.scopeTable = *scopePush->imm();
    }

    // the record's slots, from IDA's view of the stack (an `and esp` ignored): the state is where `push -1` went;
    // an SEH frame's scope table lies between it and the handler
    const StackWalk stack(*this, f, StackWalk::View::kEhFrame);
    frame.stateSlot = stack.esp(minusOne->address).value_or(0) - 4;
    const int64_t nextSlot = frame.stateSlot - (scopePush ? 12 : 8);
    auto slotOf = [&](const Insn& insn, size_t n) -> std::optional<int64_t> {
        if (insn.operands.size() <= n || insn.operands[n].type != X86_OP_MEM)
            return std::nullopt;
        return stack.offset(insn.address, insn.operands[n]);
    };

    std::optional<x86_reg> listHolder;  // the register the prologue loads fs:[0] into, until pushed
    for (size_t k = 0; k < insns.size(); ++k) {
        const Insn& insn = *insns[k];
        if ((frame.kind == EhFrame::kCxx || frame.kind == EhFrame::kSeh) && insn.is("mov") &&
            slotOf(insn, 0) == frame.stateSlot)
            frame.stores.push_back(insn.address);
        if (k < prologue) {
            const bool fsLoad = touchesFs(insn) && !storesFs(insn) && !insn.operands.empty() &&
                                insn.operands[0].type == X86_OP_REG;
            const bool listPush = listHolder && insn.is("push") && insn.operands.size() == 1 &&
                                  insn.operands[0].type == X86_OP_REG && insn.operands[0].reg == *listHolder;
            if (&insn == minusOne || &insn == handlerPush || &insn == scopePush || touchesFs(insn) || listPush ||
                (insn.is("lea") && slotOf(insn, 1) == nextSlot))
                frame.registration.push_back(insn.address);
            if (fsLoad)
                listHolder = insn.operands[0].reg;
            else if (listPush)
                listHolder.reset();
        } else if (touchesFs(insn) || (insn.is("mov") && slotOf(insn, 1) == nextSlot)) {
            frame.unregistration.push_back(insn.address);
        }
    }
    return frame;
}

}  // namespace exefacts
