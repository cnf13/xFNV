#pragma once

#include <cstdint>

// Tells the solver to skip m_numSolverResultsToSkip solver results. The schema's first byte is its type
// (hkpJacobianSchema); the count is stored as a byte at offset 4.
class hkpJacobianShiftSolverResultsPointerSchema {
public:
    uint32_t m_padding_1;
    uint8_t m_numSolverResultsToSkip;
    uint32_t m_padding2[2];
};
