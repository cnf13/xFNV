#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/Cpu/hkpCpuMoppAabbJob.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpMoppAabbJob.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppObbVirtualMachine.h"

// The SDK's hkCpuMoppAabbQueryJob: each command's box is queried against the job's MOPP code, the hits going into
// the command's results buffer, terminated by an invalid shape key.
hkJobQueue::JobStatus hkCpuMoppAabbQueryJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN("CollQueryMoppAabbQuery", nullptr);
    // The queue entry holds the job: both are hkJobs.
    const hkJob& baseJob = nextJobOut;
    const hkpMoppAabbJob& job = static_cast<const hkpMoppAabbJob&>(baseJob);
    // A MOPP code over the job's code bytes: only its info, its data array and its build type are set (no vtable or
    // reference count), and no destructor is run on it.
    alignas(16) uint8_t codeStorage[sizeof(hkpMoppCode)];
    hkpMoppCode& code = *reinterpret_cast<hkpMoppCode*>(codeStorage);
    code.m_info = job.m_moppCodeInfo;
    code.m_data.m_data = const_cast<uint8_t*>(job.m_moppCodeData);
    code.m_data.m_size = 512;
    code.m_data.m_capacityAndFlags = 512;
    // EXE-INLINED none: hkEnum<E, S>::operator=
    code.m_buildType.m_storage = static_cast<int8_t>(hkpMoppCode::BUILD_NOT_SET);
    for (int32_t i = 0; i < job.m_numCommands; i++) {
        const hkpMoppAabbCommand* command = &job.m_commandArray[i];
        // The results array over the command's buffer, which is treated as untyped memory: the command declares shape
        // keys, the query writes 4-byte primitive infos into it.
        hkArray<hkpMoppPrimitiveInfo, hkContainerHeapAllocator> results(
            reinterpret_cast<hkpMoppPrimitiveInfo*>(command->m_results), 0,
            hkpMoppAabbCommand::MAX_OUTPUT_KEYS_PER_QUERY
        );
        alignas(16) hkpMoppObbVirtualMachine machine;
        machine.queryAabb(&code, command->m_aabbInput, &results);
        // The terminator: an invalid shape key after the hits.
        // EXE-INLINED none: hkArray<hkpMoppPrimitiveInfo>::pushBack
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (results.m_size == results.getCapacity()) {
            hkArrayUtil::_reserveMore(router.heap(), &results, sizeof(hkpMoppPrimitiveInfo));
        }
        hkpMoppPrimitiveInfo* pTerminator = results.m_data + results.m_size;
        if (pTerminator) {
            pTerminator->ID = HK_INVALID_SHAPE_KEY;
        }
        results.m_size++;
    }
    HK_TIMER_END();
    return jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}
