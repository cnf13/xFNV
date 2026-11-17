#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuFireJacobianSetupCallbackJob.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"

// Fires the contact and setup callbacks of the island's callback constraints the build jobs collected, frees their
// list and takes the next job.
hkJobQueue::JobStatus hkpSingleThreadedJobsOnIsland::cpuFireJacobianSetupCallbackJob(hkpMtThreadStructure& tl,
	hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& jobInOut) {
	// The queue entry holds the job in place: both derive from hkJob.
	const hkpFireJacobianSetupCallback& job =
		static_cast<const hkpFireJacobianSetupCallback&>(static_cast<const hkJob&>(jobInOut));
	hkpBuildJacobianTaskHeader* taskHeader = job.m_taskHeader;
	hkpConstraintQueryIn in = tl.m_constraintQueryIn;
	if (taskHeader->m_tasks.m_numCallbackConstraints > 0) {
		HK_TIMER_BEGIN_LIST("Integrate", "ConstraintCallbacks");
		for (int32_t i = 0; i < taskHeader->m_tasks.m_numCallbackConstraints; ++i) {
			const hkConstraintInternal* ci = taskHeader->m_tasks.m_callbackConstraints[i].m_callbackConstraints;
			in.m_constraintInstance = ci->m_constraint;
			in.m_constraintRuntime.m_storage = ci->m_runtime;
			// The accumulator buffer is raw memory: the entities' solver data are byte offsets from its start.
			in.m_bodyA =
				reinterpret_cast<hkpVelocityAccumulator*>(reinterpret_cast<uintptr_t>(taskHeader->m_accumulatorsBase) +
					ci->m_entities[0]->m_solverData);
			in.m_bodyB =
				reinterpret_cast<hkpVelocityAccumulator*>(reinterpret_cast<uintptr_t>(taskHeader->m_accumulatorsBase) +
					ci->m_entities[1]->m_solverData);
			in.m_transformA = ci->m_constraint->m_entities[0]->m_collidable.m_motion.pMotion;
			in.m_transformB = ci->m_constraint->m_entities[1]->m_collidable.m_motion.pMotion;
			if ((ci->m_callbackRequest &
					(hkpConstraintAtom::CALLBACK_REQUEST_NEW_CONTACT_POINT |
						hkpConstraintAtom::CALLBACK_REQUEST_CONTACT_POINT_CALLBACK)) != 0) {
				hkpSimpleContactConstraintData* contactData =
					static_cast<hkpSimpleContactConstraintData*>(ci->m_constraint->m_data);
				hkpModifierConstraintAtom* oldModifiers = ci->m_constraint->m_constraintModifiers;
				// EXE-INLINED none: hkpWorldConstraintUtil::getTerminalAtom
				hkpConstraintAtom* terminalAtom = ci->m_atoms;
				while (terminalAtom->m_type.m_storage >= hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT) {
					terminalAtom = static_cast<hkpModifierConstraintAtom*>(terminalAtom)->m_child;
				}
				// The terminal atom of a contact constraint is its simple contact atom.
				hkSimpleContactConstraintData_fireCallbacks(contactData, &in,
					static_cast<hkpSimpleContactConstraintAtom*>(terminalAtom), hkpContactPointEvent::TYPE_MANIFOLD);
				// A callback that added or removed a modifier changes the atoms the task builds from.
				hkpModifierConstraintAtom* newModifiers = ci->m_constraint->m_constraintModifiers;
				if (oldModifiers != newModifiers) {
					hkpBuildJacobianTask::AtomInfo* atomInfo = taskHeader->m_tasks.m_callbackConstraints[i].m_atomInfo;
					atomInfo->m_atoms = newModifiers;
					atomInfo->m_atomsSize = newModifiers->m_modifierAtomSize;
				}
			}
			if ((ci->m_callbackRequest & hkpConstraintAtom::CALLBACK_REQUEST_SETUP_CALLBACK) != 0) {
				ci->m_constraint->m_data->buildJacobianCallback(in);
			}
		}
		// EXE-INLINED none: hkDeallocateChunk<hkpBuildJacobianTaskCollection::CallbackPair>
		int32_t numCallbackConstraints = taskHeader->m_tasks.m_numCallbackConstraints;
		hkpBuildJacobianTaskCollection::CallbackPair* callbackConstraints = taskHeader->m_tasks.m_callbackConstraints;
		hkMemoryRouter::getInstance().heap().blockFree(callbackConstraints,
			numCallbackConstraints * static_cast<int32_t>(sizeof(hkpBuildJacobianTaskCollection::CallbackPair)));
		taskHeader->m_tasks.m_callbackConstraints = nullptr;
		taskHeader->m_tasks.m_numCallbackConstraints = 0;
		HK_TIMER_END_LIST();
	}
	return jobQueue.finishJobAndGetNextJob(&jobInOut, jobInOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}
