#pragma once

#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Reconstruction.hpp"

// What one time of impact solve gets from its hkpToiResourceMgr: the priority below which events are dropped, the
// solver's entity and constraint limits, its iteration counts and the scratch pad it runs in. Members and offsets from
// the PDB (sizeof 32). hkpDefaultToiResourceMgr::beginToiAndSetupResources (0xD9FBB0) fills it: `mov [esi+0x1c], eax`
// (m_scratchpadSize), `mov [esi+0x18], eax` (the allocator's block), 0x3E8 at +4/+8/+0xC, 3 at +0x10, 4 at +0x14 and
// PRIORITY_TOI at +0; endToiAndFreeResources (0xD9FC30) frees the pointer at +0x18. hkLs_localizedSolveToi (0xD0F300)
// takes it by const reference (the PDB's `const hkpToiResources&`).
struct hkpToiResources {
	hkpConstraintInstance::ConstraintPriority m_minPriorityToProcess;
	int32_t m_maxNumEntities;
	int32_t m_maxNumActiveEntities;
	int32_t m_maxNumConstraints;
	int32_t m_numToiSolverIterations;
	int32_t m_numForcedToiFinalSolverIterations;
	char* m_scratchpad;
	int32_t m_scratchpadSize;
};
