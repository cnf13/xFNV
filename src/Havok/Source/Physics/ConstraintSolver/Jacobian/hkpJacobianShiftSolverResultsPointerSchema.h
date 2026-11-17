#pragma once

#include "Reconstruction.hpp"

// Tells the solver to skip m_numSolverResultsToSkip solver results (hkpconstraintsolver.pdb; the SDK ships no header).
// The schema's first byte is its type (hkpJacobianSchema); the exe's builder 0xD6F460 stores the count as a byte
// at 4 (`mov byte ptr [eax+4], cl`).
class hkpJacobianShiftSolverResultsPointerSchema {
public:
	uint32_t m_padding_1;
	uint8_t m_numSolverResultsToSkip;
	uint32_t m_padding2[2];
};
