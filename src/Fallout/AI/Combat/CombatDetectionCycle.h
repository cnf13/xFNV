#pragma once

#include <cstdint>

// The state of one detection pass of a combat controller: what it still has to check.
class CombatDetectionCycle {
public:
    uint32_t eState;
    bool bCheckCone;
    bool bRunDetectionEventCheck;
    bool bRunPlayerDetection;
};

// The detection passes a high process cycles through, indexed by its detection counter (modulo 12).
using CombatDetectionCycleArray = CombatDetectionCycle[12];
inline CombatDetectionCycleArray CombatDetectionCycles{
    {0, true, false, true},
    {1, false, true},
    {0, true},
    {},
    {1, true},
    {2, true},
    {0, false, false, true},
    {1},
    {0, true},
    {},
    {1, true},
    {2, false, true}
};
