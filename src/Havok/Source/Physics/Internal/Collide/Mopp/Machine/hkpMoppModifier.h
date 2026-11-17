#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// What a MOPP code walk asks, terminal by terminal, to remove terminals from the code. It holds no state; both
// callbacks are pure.
class hkpMoppModifier {
public:
    virtual ~hkpMoppModifier();

    // Whether the terminal with the id and properties is to be removed.
    virtual hkBool shouldTerminalBeRemoved(uint32_t id, const uint32_t* properties) = 0;
    // Tells the modifier the relative address in the MOPP code of a terminal to remove.
    virtual void addTerminalRemoveInfo(int32_t relativeMoppAddress) = 0;

    // DeletingDestruct's free (ourFree).
    static void operator delete(void* apMemory);
};
