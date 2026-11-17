#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Reconstruction.hpp"

class hkpJacobianSchema;
struct hkpMtThreadStructure;
class hkpVelocityAccumulator;

// What hkCpuBuildJacobiansFromTask builds from: a task's atom infos, accumulators and schema range. The PDB's
// `anonymous-namespace'::hkBuildJacobiansFromTaskInput of hkpCpuIntegrateJob.cpp (sizeof 28 in hkpdynamics.pdb),
// named at namespace scope here; 0xD949A0 reads +0x0..+0x18.
struct hkBuildJacobiansFromTaskInput {
	hkpBuildJacobianTask::AtomInfo* m_atomInfos;
	int32_t m_numAtomInfos;
	const hkpVelocityAccumulator* m_accumulators;
	hkpJacobianSchema* m_schemas;
	hkpJacobianSchema* m_schemasOfNextTask;
	void* m_nextTask;
	hkBool m_finishSchemasWithGoto;
};

// Builds the Jacobians of one island's constraint batch and takes the next job (PDB hkCpuBuildJacobiansJob,
// hkpCpuBuildJacobiansJob.cpp; 0xD94A80, cdecl, ret 0).
hkJobQueue::JobStatus hkCpuBuildJacobiansJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue,
	hkJobQueue::JobQueueEntry& jobInOut);
// The SDK's file-local hkCpuBuildJacobiansFromTask (the PDB's, hkpCpuBuildJacobiansJob.cpp), void; usercall
// (0xD949A0): in in eax, queryIn in esi, no caller. Per atom info it sets queryIn's bodies (+0x30, +0x34,
// accumulators at index * 0x80), transforms (+0x38, +0x3C) and constraint data and builds the Jacobians. Defined in
// hkpCpuBuildJacobiansJob.cpp.
void hkCpuBuildJacobiansFromTask(const hkBuildJacobiansFromTaskInput& input, hkpConstraintQueryIn& queryIn);
