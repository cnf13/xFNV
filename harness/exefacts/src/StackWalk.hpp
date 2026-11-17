// draft.Stack: esp and ebp per instruction, relative to the entry's esp ([esp] at entry is the return address),
// walked over the blocks; a direct call moves esp by its callee's pops, an indirect one by its import's, else by the
// argument bytes pushed since the prologue that the code after it (later calls' pops, the caller's cleanup) does not
// take.
#pragma once

#include "Function.hpp"

#include <optional>
#include <unordered_map>

namespace exefacts {

class Facts;

class StackWalk {
public:
    // How the walk reads a realigned frame. kExact: an `and esp` leaves esp unknown, so that no esp offset past it is
    // taken for an entry-relative one. kRelative: an `and esp` leaves esp as it was, so that a realigned frame's
    // slots past it keep their offsets relative to each other (not to the entry: the alignment's padding is
    // unknown). kEhFrame, the view Facts::ehFrame takes, IDA's: kRelative, and the prologue ends at the exception
    // list's registration (`mov fs:[0], reg`), so that what is pushed after it is a call's arguments.
    enum class View { kExact, kRelative, kEhFrame };

    // Raised as draft.Stack is (an entry that does not decode).
    StackWalk(Facts& facts, const Function& function, View view = View::kExact);

    // The entry-relative stack offset a memory operand of the instruction at pc addresses (esp or ebp based, no
    // index register), else nullopt
    std::optional<int64_t> offset(uint32_t pc, const Operand& op) const;
    std::optional<int64_t> esp(uint32_t pc) const;
    std::optional<int64_t> ebp(uint32_t pc) const;

private:
    struct Frame {
        std::optional<int64_t> esp;
        std::optional<int64_t> ebp;
    };
    void walk(Facts& facts);
    // The pending argument bytes the code after a block's unknown call (its index there) takes (calls' pops, the
    // caller's cleanup) beyond what it pushes itself
    int64_t argumentsTaken(Facts& facts, uint32_t block, size_t call);

    const Function& function_;
    View view_;
    std::unordered_map<uint32_t, Frame> frames_;
    std::unordered_map<uint32_t, int64_t> argumentsTaken_;  // per unknown call's address
};

}  // namespace exefacts
