#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuBuildJacobiansJob.h"

#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpSimpleContactConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuSolveConstraintsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

// The goto schema's first word is read as a float, this double is added to it and the result is stored back as a float;
// its low byte is then overwritten with the goto type.
static const double kGotoSchemaTimeAdd = 1.1920928955078125e-07;

// Builds the Jacobians of one island's constraint batch and takes the next job. Copies the thread structure's
// constraint query into a local and fills in each constraint's two accumulators, its transforms, its instance and its
// runtime before calling the builder of its first atom's kind (the contact builder for a contact atom, the atoms
// builder otherwise), which appends schemas through the out parameter. The last schema slot then becomes a goto schema
// over the unused part of the batch, or is zeroed when the batch is not finished with a goto. The third argument is the
// queue entry, which holds the job from its first byte, as in hkCpuSolveConstraintBatchJob.
hkJobQueue::JobStatus
hkCpuBuildJacobiansJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& jobInOut) {
    HK_TIMER_BEGIN_LIST("Integrate", "BuildJacobians");
    hkpBuildJacobiansJob* job = reinterpret_cast<hkpBuildJacobiansJob*>(&jobInOut);
    hkpConstraintQueryIn queryIn = tl.m_constraintQueryIn;
    const hkpBuildJacobianTask* task = job->m_buildJacobianTask;
    hkpConstraintQueryOut out;
    out.m_jacobianSchemas = task->m_schemas;
    // Sets the flag when the job asks for it and the task has a successor (it tests the task's m_next).
    const bool finishSchemasWithGoto = job->m_finishSchemasWithGoto.m_bool != 0 && task->m_next != nullptr;
    for (int32_t i = 0; i < task->m_numAtomInfos; ++i) {
        const hkpBuildJacobianTask::AtomInfo& atomInfo = task->m_atomInfos[i];
        queryIn.m_transformA = atomInfo.m_transformA;
        queryIn.m_transformB = atomInfo.m_transformB;
        queryIn.m_bodyA = task->m_accumulators + atomInfo.m_accumulatorIndexA;
        queryIn.m_bodyB = task->m_accumulators + atomInfo.m_accumulatorIndexB;
        queryIn.m_accumulatorAIndex.m_storage = atomInfo.m_accumulatorIndexA;
        queryIn.m_accumulatorBIndex.m_storage = atomInfo.m_accumulatorIndexB;
        queryIn.m_constraintInstance = atomInfo.m_instance;
        queryIn.m_constraintRuntime.m_storage = atomInfo.m_runtime;
        queryIn.m_constraintRuntimeInMainMemory.m_storage = atomInfo.m_runtime;
        if (atomInfo.m_atoms->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
            hkBool contact = {true};
            hkSimpleContactConstraintDataBuildJacobian(
                static_cast<hkpSimpleContactConstraintAtom*>(atomInfo.m_atoms), queryIn, contact, out
            );
        }
        else {
            hkSolverBuildJacobianFromAtomsNotContact(atomInfo.m_atoms, atomInfo.m_atomsSize, queryIn, out);
        }
    }
    if (!finishSchemasWithGoto) {
        *reinterpret_cast<uint32_t*>(out.m_jacobianSchemas) = 0;
    }
    else {
        const int32_t size = static_cast<int32_t>(
            reinterpret_cast<uintptr_t>(task->m_schemasOfNextTask) - reinterpret_cast<uintptr_t>(out.m_jacobianSchemas)
        );
        if (size > 0) {
            float time = *reinterpret_cast<float*>(out.m_jacobianSchemas);
            *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(out.m_jacobianSchemas) + 4) =
                static_cast<uint32_t>(size);
            time = static_cast<float>(time + kGotoSchemaTimeAdd);
            *reinterpret_cast<float*>(out.m_jacobianSchemas) = time;
            *reinterpret_cast<uint8_t*>(out.m_jacobianSchemas) = 2;
        }
    }
    HK_TIMER_END_LIST();
    // Passes the job as both the finished job and the output entry, and does not wait.
    return jobQueue.finishJobAndGetNextJob(&jobInOut, jobInOut, static_cast<hkJobQueue::WaitStatus>(0));
}

// Builds the Jacobians of each of the task's atom infos into the task's schemas, then makes the last schema slot a goto
// over the unused part up to the next task's schemas, or zeroes it when the task is not finished with a goto.
void hkCpuBuildJacobiansFromTask(const hkBuildJacobiansFromTaskInput& input, hkpConstraintQueryIn& queryIn) {
    hkpConstraintQueryOut out;
    out.m_jacobianSchemas = input.m_schemas;
    const hkpBuildJacobianTask::AtomInfo* atomInfo = input.m_atomInfos;
    for (int32_t i = 0; i < input.m_numAtomInfos; ++i, ++atomInfo) {
        queryIn.m_transformA = atomInfo->m_transformA;
        queryIn.m_transformB = atomInfo->m_transformB;
        // The query's accumulator members are not const, so the const accumulators are cast.
        queryIn.m_bodyA = const_cast<hkpVelocityAccumulator*>(&input.m_accumulators[atomInfo->m_accumulatorIndexA]);
        queryIn.m_bodyB = const_cast<hkpVelocityAccumulator*>(&input.m_accumulators[atomInfo->m_accumulatorIndexB]);
        queryIn.m_accumulatorAIndex.m_storage = atomInfo->m_accumulatorIndexA;
        queryIn.m_accumulatorBIndex.m_storage = atomInfo->m_accumulatorIndexB;
        queryIn.m_constraintInstance = atomInfo->m_instance;
        queryIn.m_constraintRuntime.m_storage = atomInfo->m_runtime;
        queryIn.m_constraintRuntimeInMainMemory.m_storage = atomInfo->m_runtime;
        if (atomInfo->m_atoms->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
            hkBool contact = {true};
            hkSimpleContactConstraintDataBuildJacobian(
                static_cast<hkpSimpleContactConstraintAtom*>(atomInfo->m_atoms), queryIn, contact, out
            );
        }
        else {
            hkSolverBuildJacobianFromAtomsNotContact(atomInfo->m_atoms, atomInfo->m_atomsSize, queryIn, out);
        }
    }
    // The schema stream is untyped 16-byte units written through each schema's own view: the end schema is a zeroed
    // first 32-bit word, the goto schema's first lane is read and stored as a float before its type byte.
    if (!input.m_finishSchemasWithGoto.m_bool) {
        *reinterpret_cast<uint32_t*>(out.m_jacobianSchemas) = 0;
    }
    else {
        const int32_t size = static_cast<int32_t>(
            reinterpret_cast<uintptr_t>(input.m_schemasOfNextTask) - reinterpret_cast<uintptr_t>(out.m_jacobianSchemas)
        );
        if (size > 0) {
            float time = *reinterpret_cast<float*>(out.m_jacobianSchemas);
            reinterpret_cast<hkpJacobianGotoSchema*>(out.m_jacobianSchemas)->m_offset = static_cast<uint32_t>(size);
            time = static_cast<float>(time + kGotoSchemaTimeAdd);
            *reinterpret_cast<float*>(out.m_jacobianSchemas) = time;
            out.m_jacobianSchemas->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_GOTO;
        }
    }
}
