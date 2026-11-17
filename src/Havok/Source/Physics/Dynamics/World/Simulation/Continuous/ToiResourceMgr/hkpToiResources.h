#pragma once

#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

// What one time of impact solve gets from its hkpToiResourceMgr: the priority below which events are dropped, the
// solver's entity and constraint limits, its iteration counts and the scratch pad it runs in.
// hkpDefaultToiResourceMgr::beginToiAndSetupResources fills it and endToiAndFreeResources frees the scratch pad.
// hkLs_localizedSolveToi takes it by const reference.
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
