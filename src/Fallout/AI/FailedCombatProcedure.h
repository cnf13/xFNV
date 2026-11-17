#pragma once

#include <cstdint>

// A combat procedure that failed: its type, why it failed (a string the procedure gave) and when. The combat
// controller keeps the last five.
struct FailedCombatProcedure {
    int32_t iProcedureType;
    const char* pFailureReason;
    float fTime;
};
