// One function's code as draft.Func decodes it: recursively from the entry within the function's bounds, jump
// tables followed, jumps out of the bounds kept as tail calls, then cut into basic blocks.
#pragma once

#include "Program.hpp"

#include <map>
#include <utility>
#include <vector>

namespace exefacts {

class Function {
public:
    // Raised when a jump table read leaves the image (draft.Func raises there too)
    Function(Program& program, uint32_t address);

    uint32_t address() const { return address_; }
    uint32_t end() const { return end_; }
    // The decoded instructions, by address
    const std::map<uint32_t, const Insn*>& insns() const { return insns_; }
    // (jmp address, target) of each jump out of the bounds, in decode order
    const std::vector<std::pair<uint32_t, uint32_t>>& tailcalls() const { return tailcalls_; }
    // The block starting at start (its instruction addresses); Raised for none (draft's KeyError: an entry that
    // does not decode)
    const std::vector<uint32_t>& block(uint32_t start) const;
    // The blocks a block flows to, in draft's order
    const std::vector<uint32_t>& blockSuccessors(uint32_t start) const;
    const Insn& insn(uint32_t pc) const { return *insns_.at(pc); }
    // ebx, esi, edi written outside a push or pop
    Families clobbers() const { return clobbers_; }
    // ebx, esi, edi written outside a push or pop and never popped: what it leaves changed for its caller
    Families unrestored() const { return clobbers_ & ~popped_; }
    // A push of ebx, esi or edi the function never writes otherwise (clobbers()) nor restores (no `pop` of it)
    // passes the caller's value on: a read of that register (a register input), not a save. A save the function
    // never needed (an EH prologue's `push ebx; push esi; push edi`) is restored all the same
    bool pushesInput(const Insn& insn) const;

private:
    void decode();
    // pc lies in the function: its bounds, or a chunk of it decode reached (chunk)
    bool inside(uint32_t pc) const;
    // A jump target outside the bounds that starts a listed function named as an MSVC local label of another
    // (`::splitpath_s:::$error_einval$29241`): a piece of this function IDA split off, decoded as its own; its end
    std::optional<uint32_t> chunk(uint32_t target) const;
    std::vector<uint32_t> jumpTable(uint32_t pc, bool& found) const;
    void cutBlocks();

    Program& program_;
    uint32_t address_;
    uint32_t end_;
    std::vector<std::pair<uint32_t, uint32_t>> chunks_;  // [start, end) of the chunks decode reached
    std::map<uint32_t, const Insn*> insns_;
    Families clobbers_ = 0;
    Families popped_ = 0;  // ebx, esi, edi written by a pop
    // draft's succ: only the instructions it records successors for have an entry
    std::map<uint32_t, std::vector<uint32_t>> succ_;
    std::vector<std::pair<uint32_t, uint32_t>> tailcalls_;
    std::map<uint32_t, std::vector<uint32_t>> blocks_;
    std::map<uint32_t, std::vector<uint32_t>> blockSuccessors_;
};

}  // namespace exefacts
