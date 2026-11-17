// The exe facts idakit's sync and emit read, each answered as today's Python does (draft.abi and draft's
// stack walk, checkbody's World.throws) or from the function's asm. A fact the Python raises for throws Raised.
#pragma once

#include "Function.hpp"
#include "StackWalk.hpp"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace exefacts {

// draft.abi, the parts sync and emit read
struct Abi {
    // ecx / edx read before written (thiscall / fastcall evidence)
    bool ecx = false;
    bool edx = false;
    // the stack argument slots read (0: [entry esp + 4]), ascending
    std::vector<int32_t> args;
    // a `ret`, and the largest `ret N`'s N (0 without one)
    bool returns = false;
    uint32_t retPops = 0;
};

// A tail jump through a vtable slot (Facts::pops): the jump's target is the dword at `slot` in the table the receiver
// points to at its offset 0, the receiver being `receiver` bytes past the value the loads reach. The loads start at
// root (-1: the entry's ecx; k >= 0: stack argument slot k, [entry esp + 4 + 4k], as the entry held it) and each reads
// the dword at its displacement from the value so far.
struct VtableJump {
    int32_t root = -1;
    std::vector<int32_t> loads;
    int32_t receiver = 0;
    int32_t slot = 0;
};

// The functions a vtable slot can hold for the class the receiver has (Facts::setSlotTargets), which needs the
// receiver's type: the user's (idakit's, from the database's types and the exe's RTTI tables). nullopt where the
// receiver's class or its tables are not known.
class SlotTargets {
public:
    virtual ~SlotTargets() = default;
    virtual std::optional<std::vector<uint32_t>> targets(uint32_t function, const VtableJump& jump) = 0;
};

// A trivial accessor (Facts::trivialAccessor): a function whose whole body, after an optional `push ebp; mov ebp, esp`
// frame and `this` spill, sets eax to an object pointer plus a constant, loads one field there into eax, loads one
// float field there into ST0 (`fld`), or stores one stack argument into the field there (a setter), then returns
struct Accessor {
    // eax = base + offset (`this`, `this + N`, `&field`), or the field at base + offset
    bool load = false;
    // a load into ST0 (a float getter); eax is then whatever the body left there
    bool x87 = false;
    // -1: the object is ecx (`this`), else the stack argument slot holding it (0: [entry esp + 4])
    int32_t base = -1;
    int32_t offset = 0;
    // a load's or store's width in bytes (1, 2 or 4; x87: 4, 8 or 10) and a load's extension: 1 movzx, -1 movsx, 0
    // none (a 4-byte load, an x87 one, or `mov al/ax`, which leaves eax's upper bytes as they were)
    uint8_t width = 0;
    int8_t extension = 0;
    // the `ret N`'s N
    uint32_t pops = 0;
    // a store (load false): the low width bytes of the stack argument in slot value go to the field at base + offset;
    // eax, ecx and edx are then whatever the body left there
    bool store = false;
    int32_t value = -1;
};

// A function's MSVC exception frame (Facts::ehFrame), from the prologue's `push -1; push handler; mov eax, fs:[0];
// push eax; ...; mov fs:[0], record` (after an `and esp` in a realigned frame). The registration record is three
// dwords on the stack: the next record (stateSlot - 8), the handler (stateSlot - 4) and the state (stateSlot). An SEH
// frame (a `__try` scope: __except or __finally) has a fourth, its scope table, between the handler and the state:
// `push -1|-2 (the initial try level); push scopetable; push handler; ...` (SEH3, SEH4), or `push N; push
// scopetable; call H` where H (__SEH_prolog4) pushes the handler and registers the record itself; its state is the
// try level, [ebp - 4] in either form.
struct EhFrame {
    // kNone: the function touches fs: but its prologue pushes no handler; kOther: a handler that is not a
    // __CxxFrameHandler thunk (`mov eax, funcinfo; jmp __CxxFrameHandler3`), with no scope table; kCxx: a C++ EH
    // frame; kSeh: an SEH frame, a scope table pushed right before the handler (or before the call of a helper that
    // pushes the handler and stores fs:[0]): MSVC builds one for every function with a `__try`, and for nothing else
    enum Kind : uint8_t { kNone, kOther, kCxx, kSeh } kind = kNone;
    // kCxx: the FuncInfo's try block count (0: a cleanup-only frame)
    uint32_t tryCount = 0;
    // kSeh: the scope table's address (one entry per `__try`)
    uint32_t scopeTable = 0;
    // kOther, kCxx, kSeh: the state slot, the `push -1`'s slot from entry esp (in a realigned frame IDA's offset, the
    // `and esp` ignored); kSeh: the try level's slot, entry - 8 in both forms
    int64_t stateSlot = 0;
    // kCxx, kSeh: the `mov` instructions that store the state (a dword or its low byte; kSeh: the try level, which
    // enters and leaves each `__try`), ascending
    std::vector<uint32_t> stores;
    // kOther, kCxx, kSeh: the registration's instructions (the -1 and handler pushes, the fs:[0] load and the push of
    // what it loads, the record's `lea`, the fs:[0] store; kSeh's scope table push, and in the helper form the pushes
    // and the call) and the unregistrations' (each later fs: access and each load of the next record's slot),
    // ascending
    std::vector<uint32_t> registration;
    std::vector<uint32_t> unregistration;
};

// The width of what a function returns in eax (Facts::returnWidth): the fewest bytes of eax every definition reaching
// a ret writes (`mov eax`, `movzx eax`, `movsx eax`, `xor eax, eax`: 4; `mov ax`: 2; `mov al`, `setcc al`: 1), and
// those definitions' extension: 1 when each is a movzx, -1 when each is a movsx, 0 otherwise; raw: the first
// definition whose value may be other than 0 and 1 (anything but a `setcc`, a `mov` of the constant 0 or 1, an `xor`
// of the register with itself, or a load of a frame byte only those stores write), 0 where every one is 0 or 1 (a bool)
struct ReturnWidth {
    uint8_t width = 0;
    int8_t extension = 0;
    uint32_t raw = 0;
};

// One instruction's access of the function's stack arguments (Facts::argumentAccesses): bytes [offset, offset + size)
// past [entry esp + 4], read, written (a store whose destination is the operand) or address-taken (`lea`)
struct ArgumentAccess {
    enum Kind : uint8_t { Read, Write, Address };
    uint32_t pc = 0;
    int64_t offset = 0;
    uint8_t size = 0;
    Kind kind = Read;
};

// One argument dword of a call or tail jump that holds one of the function's own stack arguments as the entry held it
// (Facts::passedArguments): the dword at `offset` past [entry esp + 4], passed whole as the `index`th argument dword
// ([esp + 4 index] at the call, [esp + 4 + 4 index] at the jump) of the call or jump at pc to target (0: indirect)
struct PassedArgument {
    uint32_t pc = 0;
    uint32_t target = 0;
    int64_t offset = 0;
    uint32_t index = 0;
};

// One instruction's access of the object the entry's ecx points to (Facts::thisAccesses): bytes [offset, offset +
// size) past it, read, written or address-taken (ArgumentAccess's kinds), and what the code shows of the value's
// type there: extension 1 for a movzx load, -1 for a movsx; sign 1 where an unsigned condition (jb/ja/setb/cmovb...,
// shr) reads it, -1 for a signed one (jl/jg/setl/cmovl..., sar), 0 none; real for an x87 or SSE float operand
struct ThisAccess {
    uint32_t pc = 0;
    int64_t offset = 0;
    uint8_t size = 0;
    ArgumentAccess::Kind kind = ArgumentAccess::Read;
    int8_t extension = 0;
    int8_t sign = 0;
    bool real = false;
};

class Facts {
public:
    explicit Facts(Program& program) : program_(program) {}

    Program& program() { return program_; }
    // draft.func / draft.stack, cached; Raised where draft raises
    const Function& function(uint32_t address);
    const StackWalk& stack(uint32_t address);
    // Drops the cached decodes once they hold more than 4096 functions (they are pure); call between queries only,
    // as it invalidates the references function() and stack() returned
    void trim();
    // trim, and also drops the decoded instructions (with the functions and stack walks built on them) once the
    // decoder holds more than maxInsns: a fact over every caller of a popular callee (variadicFixedArgs) decodes
    // hundreds of MB that no later query needs. Invalidates every Insn pointer the decoder returned; for callers that
    // keep none across queries (the C API)
    void trimInstructions(size_t maxInsns);

    const Abi& abi(uint32_t address);
    // Every access of the stack arguments, in address order: the memory operand of each instruction the stack walk
    // places at or past [entry esp + 4] (esp or ebp based). Raised where the decode raises
    std::vector<ArgumentAccess> argumentAccesses(uint32_t address);
    // Every access of the object at the entry's ecx (a thiscall's `this`), in address order: the memory operand
    // (no index register) of each instruction whose base register holds that ecx, followed over register copies and
    // the function's stack slots on every path (calls write eax, ecx and edx); a read's value is followed within its
    // block to the first compare or shift of it and the condition that reads the compare's flags. Raised where the
    // decode raises
    std::vector<ThisAccess> thisAccesses(uint32_t address);
    // Every argument dword of a call or tail jump out of the function that holds one of its own stack arguments as the
    // entry held it (no write of the argument's slot on the way, the value followed over register copies and stack
    // slots as thisAccesses follows `this`), in address order: a call's argument dwords are those the walk takes off
    // the stack at it (its pops) and the caller's cleanup right after it (`add esp, N`); a tail jump's, the bytes
    // its direct target pops or reads, else the function's own `ret N` or the slots it reads. Raised where the
    // decode raises
    std::vector<PassedArgument> passedArguments(uint32_t address);
    // draft.callee_pops: what a direct callee pops (its `ret N`, through a short thunk or an import thunk)
    uint32_t calleePops(uint32_t target);
    // The bytes the function pops: its `ret N` (the largest); for a function with no `ret`, which leaves only by tail
    // jumps, the bytes every tail jump whose target is proven agrees on, each its target's pops plus the bytes the
    // body moved esp by before the jump (esp's entry-relative offset there): a direct target's own pops, an import's
    // (harness/tools/import_pops.tsv), a vtable slot's (VtableJump) agreement over the functions setSlotTargets'
    // resolver says it can hold. A cycle of tail jumps is decided at its least fixpoint, as registerInputs': the
    // cycle alone proves nothing. nullopt (unknown) where no jump is proven or the proven ones disagree; Raised where
    // the function's own decode raises
    std::optional<uint32_t> pops(uint32_t address);
    // The vtable-slot resolver pops reads (null: none, a vtable tail jump proves nothing); drops the pops known so far
    void setSlotTargets(SlotTargets* slots);
    // eax, ebx, esi, edi read before written (usercall register inputs)
    Families regArgs(uint32_t address);
    // eax, ecx, edx, ebx, esi, edi the function takes as register arguments: read before written on some path (ecx
    // and edx as abi reads them, a `push ecx` in the prologue's run or whose slot a store overwrites before any read
    // no read either; the others as regArgs does), or passed on: still unwritten where a direct call or a jump out of
    // the function reaches a target whose own register inputs hold them (a cycle of calls at its least fixpoint), or
    // where an indirect call or jump leaves (ecx and edx, which a standard convention passes there; not an import's,
    // nor a jump table's). All six where its decode raises
    Families registerInputs(uint32_t address);
    // Some path from the entry uses the x87 stack's top (ST0) before pushing onto it or calling: an x87 register input
    // no integer register convention passes (a usercall's). False where its decode raises
    bool x87Input(uint32_t address);
    // ebx, esi, edi written outside a push or pop
    Families clobbers(uint32_t address);
    // ebx, esi, edi written outside a push or pop and never popped: changed for its caller (a usercall whose C++ body,
    // which keeps them, cannot be). All three where its decode raises
    Families unrestored(uint32_t address);
    // The fixed stack arguments of a C-variadic cdecl function
    std::optional<int32_t> variadicFixedArgs(uint32_t address);
    // The alignment the prologue's `and esp` gives the frame, 0 for none
    uint32_t frameAlign(uint32_t address);
    // The instruction sets beyond the base one its decoded code uses (InstructionSet bits), 0 where its decode raises
    InstructionSets instructionSets(uint32_t address);
    // Some return path sets eax. tempsReturn false (checkbody): an eax definition read again before the ret is a
    // temporary, not a return value (a debug setter's `mov eax, [ebp-4]; mov [eax+X], ecx`)
    bool leavesEax(uint32_t address, bool tempsReturn = true);
    // Every path returns the entry ecx in eax (a constructor's `return this`); a call clobbers ecx unless its direct
    // callee writes no ecx, calls nothing and leaves only by its rets (LTCG's callers keep this in ecx across it)
    bool returnsThis(uint32_t address);
    // The bytes of eax every return path writes; nullopt where a path returns a call's eax, leaves eax unset or the
    // decode raises
    std::optional<ReturnWidth> returnWidth(uint32_t address);
    // every path of the function ends in a throw (memoised in query order)
    bool throws(uint32_t address);
    // The function as a trivial accessor, from its straight-line code up to the first `ret`; nullopt when it is not
    // one (any other instruction, a branch or call, a second load or store, a load and a store, a memory write outside
    // its own frame other than the one store, a register left changed other than eax, or eax, ecx and edx for a store)
    std::optional<Accessor> trivialAccessor(uint32_t address);
    // The function as an empty function, its `ret N`'s N; nullopt when it is not one. Empty: its whole body, after an
    // optional `push ebp; mov ebp, esp` frame and `this` spill, is the frame's teardown and the `ret`: no load, no
    // memory write outside its own frame, every register as it was at entry (trivialAccessor's walk)
    std::optional<uint32_t> emptyFunction(uint32_t address);
    // The function's exception frame, cached; nullopt when it never touches fs:. Raised where its decode raises
    const std::optional<EhFrame>& ehFrame(uint32_t address);

private:
    // The eax definitions reaching the function's rets (leavesEax's path walk)
    struct EaxReturns {
        uint8_t kinds = 0;  // Facts.cpp's EaxKind bits of every reaching definition
        uint8_t near = 0;   // the same of the definitions right before an epilogue
        std::vector<const Insn*> definitions;  // the reaching eax writes (kSelf, kValue)
    };
    std::optional<EaxReturns> eaxReturns(uint32_t address, bool tempsReturn);
    // registerInputs' parts: one function's own reads and the sites it leaves with registers unwritten (target,
    // those registers), and the walk over the sites, whose unfinished functions are inputsWalk_'s
    struct InputNode {
        Families local = 0;
        std::vector<std::pair<uint32_t, Families>> sites;
    };
    struct InputWalk {
        size_t counter = 0;
        std::unordered_map<uint32_t, size_t> index, low;
        std::vector<uint32_t> stack;
        std::unordered_map<uint32_t, InputNode> nodes;
    };
    InputNode inputNode(uint32_t address);
    void inputsWalk(uint32_t address);
    // pops' parts: one function's exits (its `ret N`, else each tail jump: a constant, an import's pops, or the
    // targets that must agree, with esp's offset at the jump), and Tarjan's walk over the targets, as inputsWalk's
    struct PopsExit {
        std::optional<uint32_t> constant;
        std::vector<uint32_t> targets;
        int64_t adjust = 0;
    };
    struct PopsNode {
        std::optional<uint32_t> ret;
        std::vector<PopsExit> exits;
    };
    struct PopsWalk {
        size_t counter = 0;
        std::unordered_map<uint32_t, size_t> index, low;
        std::vector<uint32_t> stack;
        std::unordered_map<uint32_t, PopsNode> nodes;
    };
    PopsNode popsNode(uint32_t address);
    void popsWalk(uint32_t address);
    // trivialAccessor's and emptyFunction's walk: the accessor, or for an empty body (empty set) an Accessor holding
    // only its pops
    std::optional<Accessor> trivialWalk(uint32_t address, bool& empty);
    // the handler a frame-registration helper (__SEH_prolog4) pushes for its caller, nullopt for another function
    std::optional<uint32_t> helperHandler(uint32_t address);
    std::optional<EhFrame> readEhFrame(uint32_t address);

    Program& program_;
    std::unordered_map<uint32_t, std::unique_ptr<Function>> functions_;
    std::unordered_map<uint32_t, std::unique_ptr<StackWalk>> stacks_;
    std::unordered_map<uint32_t, std::optional<Abi>> abis_;  // nullopt: raised
    std::unordered_map<uint32_t, uint32_t> calleePops_;
    std::unordered_map<uint32_t, bool> throws_;
    std::unordered_map<uint32_t, Families> inputs_;  // registerInputs
    InputWalk inputsWalk_;
    std::unordered_map<uint32_t, std::optional<uint32_t>> pops_;  // pops
    PopsWalk popsWalk_;
    SlotTargets* slots_ = nullptr;
    std::unordered_map<uint32_t, std::optional<EhFrame>> ehFrames_;
};

}  // namespace exefacts
