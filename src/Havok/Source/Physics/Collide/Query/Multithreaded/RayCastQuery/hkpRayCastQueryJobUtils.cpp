#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobUtils.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeUtils.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpFixedBufferRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuKdTreeRayCastJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuWorldRaycastJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"

void hkKdTreeRayCastJobUtil::initializeBuildSetupJobs(
    const hkpCollidable** collidablesPtr,
    int32_t numCollidables,
    hkKdTreeProjectedEntry* entries,
    hkSemaphore* semaphore,
    int32_t numJobs,
    hkpKdTreeBuildSetupJob* jobsOut
) {
    if (numJobs > numCollidables) {
        numJobs = numCollidables;
    }
    const int32_t numPerJob = numCollidables / numJobs;
    jobsOut[0].m_semaphore = semaphore;
    jobsOut[0].m_collidables = collidablesPtr;
    jobsOut[0].m_entriesOut = entries;
    jobsOut[0].m_startIdx = 0;
    jobsOut[0].m_endIdx = numPerJob;
    int32_t endIdx = numPerJob;
    for (int32_t i = 1; i < numJobs; i++) {
        jobsOut[i] = jobsOut[0];
        jobsOut[i].m_startIdx = endIdx;
        endIdx += numPerJob;
        jobsOut[i].m_endIdx = endIdx;
    }
    jobsOut[numJobs - 1].m_endIdx = numCollidables;
}

void hkKdTreeRayCastJobUtil::makeBundleFromInputs(
    hkpWorldRayCastInput* inputs,
    hkKdTreeUtils::RayBundle& fromBundle,
    hkKdTreeUtils::RayBundle& toBundle
) {
    fromBundle.m_vec[0].m_quad = inputs[0].m_from.m_quad;
    toBundle.m_vec[0].m_quad = inputs[0].m_to.m_quad;
    fromBundle.m_vec[1].m_quad = inputs[1].m_from.m_quad;
    toBundle.m_vec[1].m_quad = inputs[1].m_to.m_quad;
    fromBundle.m_vec[2].m_quad = inputs[2].m_from.m_quad;
    toBundle.m_vec[2].m_quad = inputs[2].m_to.m_quad;
    // The SDK's 4x4 transpose of the three bundle vectors and the fourth ray's point (its w lane dropped).
    __m128 from3 = inputs[3].m_from.m_quad;
    __m128 from1 = fromBundle.m_vec[1].m_quad;
    __m128 from2 = fromBundle.m_vec[2].m_quad;
    __m128 to3 = inputs[3].m_to.m_quad;
    __m128 fromHi01 = _mm_shuffle_ps(fromBundle.m_vec[0].m_quad, from1, 238);
    __m128 fromLo23 = _mm_shuffle_ps(from2, from3, 68);
    __m128 fromLo01 = _mm_shuffle_ps(fromBundle.m_vec[0].m_quad, from1, 68);
    fromBundle.m_vec[1].m_quad = _mm_shuffle_ps(fromLo01, fromLo23, 221);
    fromBundle.m_vec[2].m_quad = _mm_shuffle_ps(fromHi01, _mm_shuffle_ps(from2, from3, 238), 136);
    fromBundle.m_vec[0].m_quad = _mm_shuffle_ps(fromLo01, fromLo23, 136);
    __m128 to1 = toBundle.m_vec[1].m_quad;
    __m128 toLo01 = _mm_shuffle_ps(toBundle.m_vec[0].m_quad, to1, 68);
    __m128 toLo23 = _mm_shuffle_ps(toBundle.m_vec[2].m_quad, to3, 68);
    __m128 toZ = _mm_shuffle_ps(
        _mm_shuffle_ps(toBundle.m_vec[0].m_quad, to1, 238),
        _mm_shuffle_ps(toBundle.m_vec[2].m_quad, to3, 238),
        136
    );
    toBundle.m_vec[0].m_quad = _mm_shuffle_ps(toLo01, toLo23, 136);
    toBundle.m_vec[1].m_quad = _mm_shuffle_ps(toLo01, toLo23, 221);
    toBundle.m_vec[2].m_quad = toZ;
}

void hkTreeRayCastJobUtil::castRayKdTree(
    const hkKdTree* kdTree,
    hkpWorldRayCastCommand* command,
    const hkpCollisionFilter* filter,
    hkpFixedBufferRayHitCollector* fixedBufferHitCollector,
    hkSpu4WayCache* collidableCache,
    hkSpu4WayCache* shapeCache,
    hkSpu4WayCache* transformCache,
    hkpWorldRayCastOutput* resultLocalArray
) {
    if (kdTree) {
        // The collector is not destroyed.
        hkCpuWorldRayCastCollector v13;
        hkpWorldRayCastOutput* m_results = command->m_results;
        v13.m_result = m_results;
        v13.m_nextFreeResult = m_results;
        v13.m_workInput.m_filterInfo = command->m_rayInput.m_filterInfo;
        const hkpCollisionFilter* v9 = command->m_rayInput.m_enableShapeCollectionFilter.m_bool != 0 ? filter : nullptr;
        v13.m_filter = filter;
        v13.m_originalInput = &command->m_rayInput;
        v13.m_command = command;
        if (v9) {
            v13.m_workInput.m_rayShapeCollectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(v9);
        }
        else {
            v13.m_workInput.m_rayShapeCollectionFilter = nullptr;
        }
        command->m_useCollector.m_bool;
        if (command->m_useCollector.m_bool) {
            if (fixedBufferHitCollector) {
                fixedBufferHitCollector->~hkpFixedBufferRayHitCollector();
                ::new (fixedBufferHitCollector)
                    hkpFixedBufferRayHitCollector(command->m_results, command->m_resultsCapacity);
                v13.m_fixedBufferRayHitCollector = fixedBufferHitCollector;
            }
            else {
                v13.m_fixedBufferRayHitCollector = nullptr;
            }
            g_FixedBufferRayHitCollectorAddRayHitCallbackFunc = &hkpFixedBufferRayHitCollector::addRayHitImplementation;
        }
        else {
            v13.m_fixedBufferRayHitCollector = nullptr;
        }
        hkKdTreeUtils::castRayIterative(kdTree, command->m_rayInput.m_from, command->m_rayInput.m_to, &v13);
    }
}

void hkTreeRayCastJobUtil::castRayBundleKdTree(
    const hkKdTree* kdTree,
    hkpWorldRayCastBundleCommand* command,
    const hkpCollisionFilter* filter,
    hkVector4& earlyHitFractionInOut,
    [[maybe_unused]] hkSpu4WayCache* collidableCache,
    [[maybe_unused]] hkSpu4WayCache* shapeCache,
    [[maybe_unused]] hkSpu4WayCache* transformCache,
    [[maybe_unused]] hkpWorldRayCastOutput* resultLocalArray,
    [[maybe_unused]] int32_t* numResultsInOut
) {
    if (!kdTree) {
        return;
    }
    const int32_t numActiveRays = command->m_numActiveRays;
    hkpWorldRayCastOutput localResults[4];
    for (int32_t i = 0; i < numActiveRays; ++i) {
        localResults[i] = *command->m_results[i];
    }

    hkKdTreeRayCastBundleCommand kdCurrentBundledCommand;
    kdCurrentBundledCommand.m_stopAfterFirstHit = command->m_stopAfterFirstHit;
    kdCurrentBundledCommand.m_results = localResults;
    for (int32_t i = 0; i < command->m_numActiveRays; ++i) {
        kdCurrentBundledCommand.m_rayInput[i] = command->m_rayInput[i];
        kdCurrentBundledCommand.m_results[i].m_hitFraction = earlyHitFractionInOut(i);
        kdCurrentBundledCommand.m_numResultsOut[i] = command->m_numResultsOut[i];
    }

    // The collector's destructor is deliberately never called.
    hkKdTreeCpuBroadphaseBundleCastCollector kBundleCollector;
    kBundleCollector.m_command = &kdCurrentBundledCommand;
    kBundleCollector.m_workInput.m_filterInfo = command->m_rayInput[0].m_filterInfo;
    kBundleCollector.m_filter = filter;
    kBundleCollector.m_numHits[0] = command->m_numResultsOut[0];
    kBundleCollector.m_numHits[1] = command->m_numResultsOut[1];
    kBundleCollector.m_numHits[2] = command->m_numResultsOut[2];
    kBundleCollector.m_numHits[3] = command->m_numResultsOut[3];
    kBundleCollector.m_originalInput = command->m_rayInput;

    hkKdTreeUtils::RayBundle kdFromBundle;
    hkKdTreeUtils::RayBundle kdToBundle;
    hkKdTreeRayCastJobUtil::makeBundleFromInputs(command->m_rayInput, kdFromBundle, kdToBundle);
    hkKdTreeUtils::castRayBundleIterative(
        kdTree,
        kdFromBundle,
        kdToBundle,
        &kBundleCollector,
        hkAabbTreeQueryUtilsData::s_numRaysToMask[command->m_numActiveRays]
    );

    for (int32_t i = 0; i < command->m_numActiveRays; ++i) {
        *command->m_results[i] = localResults[i];
        command->m_numResultsOut[i] = kdCurrentBundledCommand.m_numResultsOut[i];
    }
    kBundleCollector.hkKdTreeCpuBroadphaseBundleCastCollector::getHitFractions(earlyHitFractionInOut);
}
