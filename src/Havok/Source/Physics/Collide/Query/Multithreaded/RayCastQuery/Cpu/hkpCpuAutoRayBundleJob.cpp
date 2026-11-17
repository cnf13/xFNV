#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuAutoRayBundleJob.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"

#include <cmath>
#include <xmmintrin.h>

// Each ray's direction is to - from; its squared length is the lane sum z + (y + x), and the inverse length is the
// reciprocal square root approximation of it with no Newton step, broadcast. The direction is written as delta times
// the inverse length on all four lanes; the length is the sum times the inverse length in lane 0, the other lanes
// keeping the squared z, and that quad is written first. The squared length is then pow(length, 2.0), stored as a float
// and written over the quad with lanes 1-3 zero. Each of the eight arrays is cleared of its count and reserved for
// every ray (twice the capacity unless the ray count is larger). A ray's octant is the sign mask of direction < 0 (NaN
// gives 0), bits 0-2.
void hkpAutoRayBundleJob_sortRaysByOctant(
    const hkVector4* from,
    const hkVector4* to,
    int32_t numRays,
    hkVector4* directionsOut,
    hkVector4* lengthsSquaredOut,
    hkArray<int32_t>* octantRaysOut,
    int32_t* octantCountsOut
) {
    for (int32_t i = 0; i < numRays; ++i) {
        const __m128 kDelta = _mm_sub_ps(
            _mm_load_ps(&reinterpret_cast<const float*>(&to[i].m_quad)[0]),
            _mm_load_ps(&reinterpret_cast<const float*>(&from[i].m_quad)[0])
        );
        const __m128 kSquares = _mm_mul_ps(kDelta, kDelta);
        const __m128 kSum = _mm_add_ss(
            _mm_shuffle_ps(kSquares, kSquares, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSquares, kSquares, 0x55), kSquares)
        );
        const __m128 kRsqrt = _mm_rsqrt_ss(kSum);
        const __m128 kInvLength = _mm_shuffle_ps(kRsqrt, kRsqrt, 0x00);
        const __m128 kLength = _mm_mul_ss(kSum, kInvLength);
        _mm_store_ps(&reinterpret_cast<float*>(&directionsOut[i].m_quad)[0], _mm_mul_ps(kDelta, kInvLength));
        _mm_store_ps(&reinterpret_cast<float*>(&lengthsSquaredOut[i].m_quad)[0], kLength);
        const float fLengthSquared = static_cast<float>(std::pow(_mm_cvtss_f32(kLength), 2.0));
        _mm_store_ps(&reinterpret_cast<float*>(&lengthsSquaredOut[i].m_quad)[0], _mm_set_ss(fLengthSquared));
    }
    for (int32_t j = 0; j < 8; ++j) {
        octantCountsOut[j] = 0;
        // EXE-INLINED none: hkArray<int>::reserve
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        const int32_t iCapacity =
            static_cast<int32_t>(octantRaysOut[j].m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK);
        if (iCapacity < numRays) {
            int32_t iReserve = 2 * iCapacity;
            if (numRays >= iReserve) {
                iReserve = numRays;
            }
            hkArrayUtil::_reserve(pHeap, &octantRaysOut[j], iReserve, sizeof(int32_t));
        }
    }
    for (int32_t i = 0; i < numRays; ++i) {
        const int32_t iMask = _mm_movemask_ps(
            _mm_cmplt_ps(_mm_load_ps(&reinterpret_cast<float*>(&directionsOut[i].m_quad)[0]), _mm_setzero_ps())
        );
        const int32_t iOctant = (iMask & 4) + (iMask & 2) + (iMask & 1);
        ++octantCountsOut[iOctant];
        hkArray<int32_t>& rRays = octantRaysOut[iOctant];
        rRays.m_data[rRays.m_size] = i;
        ++rRays.m_size;
    }
}

// The listed rays are grouped in a local array of one bundle per ray at most, taken from the thread's stack memory. The
// cosine of the angle is stored as a float and its absolute value is the minimum cosine. Each ray's direction, length
// and squared length are computed as in hkpAutoRayBundleJob_sortRaysByOctant. A ray joins the first bundle of fewer
// than four rays whose first ray passes the test with the squared length times maxDistanceFactor; otherwise it starts a
// new bundle (indices 1-3 zero, one ray). After the commands are written, the stack memory is returned. numRays is not
// read.
void hkpAutoRayBundleJob_bundleIndexedRays(
    const int32_t* rayIndices,
    int32_t numIndices,
    const hkVector4* from,
    const hkVector4* to,
    int32_t numRays,
    float maxAngle,
    float maxDistanceFactor,
    hkpWorldRayCastOutput* outputs,
    hkpWorldRayCastCommand* commandsOut,
    int32_t* numCommandsOut,
    hkpWorldRayCastBundleCommand* bundleCommandsOut,
    int32_t* numBundleCommandsOut
) {
    const float fCos = std::cos(maxAngle);
    // EXE-INLINED none: fabs
    const float fMinCos = __builtin_fabsf(fCos);
    alignas(16) hkLocalArray<hkpAutoRayBundle> bundles;
    bundles.m_size = 0;
    // EXE-INLINED none: hkAllocateStack<hkpAutoRayBundle>
    hkStackMemory& rStack = hkMemoryRouter::getInstance().m_stack;
    char* pBlock = rStack.m_next.m_storage;
    rStack.m_next.m_storage = pBlock + ((numIndices * static_cast<int32_t>(sizeof(hkpAutoRayBundle)) + 0x7F) & ~0x7F);
    hkpAutoRayBundle* pLocal = static_cast<hkpAutoRayBundle*>(static_cast<void*>(pBlock));
    bundles.m_data = pLocal;
    bundles.m_capacityAndFlags =
        static_cast<uint32_t>(numIndices) | hkArrayBase<hkpAutoRayBundle>::DONT_DEALLOCATE_FLAG;
    bundles.m_localMemory = pLocal;

    for (int32_t k = 0; k < numIndices; ++k) {
        const int32_t iRay = rayIndices[k];
        hkVector4 kFrom;
        hkVector4 kTo;
        _mm_store_ps(
            &reinterpret_cast<float*>(&kFrom.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<const float*>(&from[iRay].m_quad)[0])
        );
        _mm_store_ps(
            &reinterpret_cast<float*>(&kTo.m_quad)[0], _mm_load_ps(&reinterpret_cast<const float*>(&to[iRay].m_quad)[0])
        );
        const __m128 kDelta = _mm_sub_ps(
            _mm_load_ps(&reinterpret_cast<float*>(&kTo.m_quad)[0]),
            _mm_load_ps(&reinterpret_cast<float*>(&kFrom.m_quad)[0])
        );
        const __m128 kSquares = _mm_mul_ps(kDelta, kDelta);
        const __m128 kSum = _mm_add_ss(
            _mm_shuffle_ps(kSquares, kSquares, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSquares, kSquares, 0x55), kSquares)
        );
        const __m128 kRsqrt = _mm_rsqrt_ss(kSum);
        const __m128 kInvLength = _mm_shuffle_ps(kRsqrt, kRsqrt, 0x00);
        const float fLength = _mm_cvtss_f32(_mm_mul_ss(kSum, kInvLength));
        hkVector4 kDirection;
        _mm_store_ps(&reinterpret_cast<float*>(&kDirection.m_quad)[0], _mm_mul_ps(kInvLength, kDelta));
        const float fLengthSquared = static_cast<float>(std::pow(fLength, 2.0));

        hkpAutoRayBundle* pJoined = nullptr;
        for (int32_t j = 0; j < bundles.m_size && !pJoined; ++j) {
            hkpAutoRayBundle& rBundle = bundles.m_data[j];
            if (rBundle.m_numRays >= 4) {
                continue;
            }
            const float fMaxDistanceSquared = fLengthSquared * maxDistanceFactor;
            for (int32_t m = 0; m < 1; ++m) {
                const int32_t iOther = rBundle.m_rayIndices[m];
                hkVector4 kOtherFrom;
                hkVector4 kOtherTo;
                _mm_store_ps(
                    &reinterpret_cast<float*>(&kOtherFrom.m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<const float*>(&from[iOther].m_quad)[0])
                );
                _mm_store_ps(
                    &reinterpret_cast<float*>(&kOtherTo.m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<const float*>(&to[iOther].m_quad)[0])
                );
                const __m128 kOtherDelta = _mm_sub_ps(
                    _mm_load_ps(&reinterpret_cast<float*>(&kOtherTo.m_quad)[0]),
                    _mm_load_ps(&reinterpret_cast<float*>(&kOtherFrom.m_quad)[0])
                );
                const __m128 kOtherSquares = _mm_mul_ps(kOtherDelta, kOtherDelta);
                const __m128 kOtherSum = _mm_add_ss(
                    _mm_shuffle_ps(kOtherSquares, kOtherSquares, 0xAA),
                    _mm_add_ss(_mm_shuffle_ps(kOtherSquares, kOtherSquares, 0x55), kOtherSquares)
                );
                const __m128 kOtherRsqrt = _mm_rsqrt_ss(kOtherSum);
                hkVector4 kOtherDirection;
                _mm_store_ps(
                    &reinterpret_cast<float*>(&kOtherDirection.m_quad)[0],
                    _mm_mul_ps(_mm_shuffle_ps(kOtherRsqrt, kOtherRsqrt, 0x00), kOtherDelta)
                );
                if (hkpAutoRayBundleJob_canBundleRays(
                        kFrom, kTo, kDirection, kOtherFrom, kOtherTo, kOtherDirection, fMinCos, fMaxDistanceSquared
                    )) {
                    pJoined = &rBundle;
                    break;
                }
            }
        }
        if (pJoined) {
            pJoined->m_rayIndices[pJoined->m_numRays] = iRay;
            ++pJoined->m_numRays;
        }
        else {
            hkpAutoRayBundle* pNew = bundles.m_data + bundles.m_size;
            if (pNew) {
                pNew->m_rayIndices[0] = iRay;
                pNew->m_rayIndices[1] = 0;
                pNew->m_rayIndices[2] = 0;
                pNew->m_rayIndices[3] = 0;
                pNew->m_numRays = 1;
            }
            ++bundles.m_size;
        }
    }

    hkpAutoRayBundleJob_writeCommands(
        bundles, from, to, outputs, commandsOut, numCommandsOut, bundleCommandsOut, numBundleCommandsOut
    );
}

// As hkpAutoRayBundleJob_bundleIndexedRays, over every ray: a ray is bundled only when the sign mask of its delta < 0
// (a less-than compare against zero: NaN gives 0), bits 0-2, equals octant. The local array holds one bundle per ray at
// most.
void hkpAutoRayBundleJob_bundleOctantRays(
    int32_t octant,
    const hkVector4* from,
    const hkVector4* to,
    int32_t numRays,
    float maxAngle,
    float maxDistanceFactor,
    hkpWorldRayCastOutput* outputs,
    hkpWorldRayCastCommand* commandsOut,
    int32_t* numCommandsOut,
    hkpWorldRayCastBundleCommand* bundleCommandsOut,
    int32_t* numBundleCommandsOut
) {
    const float fCos = std::cos(maxAngle);
    // EXE-INLINED none: fabs
    const float fMinCos = __builtin_fabsf(fCos);
    alignas(16) hkLocalArray<hkpAutoRayBundle> bundles;
    bundles.m_size = 0;
    // EXE-INLINED none: hkAllocateStack<hkpAutoRayBundle>
    hkStackMemory& rStack = hkMemoryRouter::getInstance().m_stack;
    char* pBlock = rStack.m_next.m_storage;
    rStack.m_next.m_storage = pBlock + ((numRays * static_cast<int32_t>(sizeof(hkpAutoRayBundle)) + 0x7F) & ~0x7F);
    hkpAutoRayBundle* pLocal = static_cast<hkpAutoRayBundle*>(static_cast<void*>(pBlock));
    bundles.m_data = pLocal;
    bundles.m_capacityAndFlags = static_cast<uint32_t>(numRays) | hkArrayBase<hkpAutoRayBundle>::DONT_DEALLOCATE_FLAG;
    bundles.m_localMemory = pLocal;

    for (int32_t iRay = 0; iRay < numRays; ++iRay) {
        hkVector4 kFrom;
        hkVector4 kTo;
        _mm_store_ps(
            &reinterpret_cast<float*>(&kFrom.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<const float*>(&from[iRay].m_quad)[0])
        );
        _mm_store_ps(
            &reinterpret_cast<float*>(&kTo.m_quad)[0], _mm_load_ps(&reinterpret_cast<const float*>(&to[iRay].m_quad)[0])
        );
        const __m128 kDelta = _mm_sub_ps(
            _mm_load_ps(&reinterpret_cast<float*>(&kTo.m_quad)[0]),
            _mm_load_ps(&reinterpret_cast<float*>(&kFrom.m_quad)[0])
        );
        const int32_t iMask = _mm_movemask_ps(_mm_cmplt_ps(kDelta, _mm_setzero_ps()));
        if ((iMask & 4) + (iMask & 2) + (iMask & 1) != octant) {
            continue;
        }
        const __m128 kSquares = _mm_mul_ps(kDelta, kDelta);
        const __m128 kSum = _mm_add_ss(
            _mm_shuffle_ps(kSquares, kSquares, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSquares, kSquares, 0x55), kSquares)
        );
        const __m128 kRsqrt = _mm_rsqrt_ss(kSum);
        const __m128 kInvLength = _mm_shuffle_ps(kRsqrt, kRsqrt, 0x00);
        const float fLength = _mm_cvtss_f32(_mm_mul_ss(kSum, kInvLength));
        hkVector4 kDirection;
        _mm_store_ps(&reinterpret_cast<float*>(&kDirection.m_quad)[0], _mm_mul_ps(kInvLength, kDelta));
        const float fLengthSquared = static_cast<float>(std::pow(fLength, 2.0));

        hkpAutoRayBundle* pJoined = nullptr;
        for (int32_t j = 0; j < bundles.m_size && !pJoined; ++j) {
            hkpAutoRayBundle& rBundle = bundles.m_data[j];
            if (rBundle.m_numRays >= 4) {
                continue;
            }
            const float fMaxDistanceSquared = fLengthSquared * maxDistanceFactor;
            for (int32_t m = 0; m < 1; ++m) {
                const int32_t iOther = rBundle.m_rayIndices[m];
                hkVector4 kOtherFrom;
                hkVector4 kOtherTo;
                _mm_store_ps(
                    &reinterpret_cast<float*>(&kOtherFrom.m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<const float*>(&from[iOther].m_quad)[0])
                );
                _mm_store_ps(
                    &reinterpret_cast<float*>(&kOtherTo.m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<const float*>(&to[iOther].m_quad)[0])
                );
                const __m128 kOtherDelta = _mm_sub_ps(
                    _mm_load_ps(&reinterpret_cast<float*>(&kOtherTo.m_quad)[0]),
                    _mm_load_ps(&reinterpret_cast<float*>(&kOtherFrom.m_quad)[0])
                );
                const __m128 kOtherSquares = _mm_mul_ps(kOtherDelta, kOtherDelta);
                const __m128 kOtherSum = _mm_add_ss(
                    _mm_shuffle_ps(kOtherSquares, kOtherSquares, 0xAA),
                    _mm_add_ss(_mm_shuffle_ps(kOtherSquares, kOtherSquares, 0x55), kOtherSquares)
                );
                const __m128 kOtherRsqrt = _mm_rsqrt_ss(kOtherSum);
                hkVector4 kOtherDirection;
                _mm_store_ps(
                    &reinterpret_cast<float*>(&kOtherDirection.m_quad)[0],
                    _mm_mul_ps(_mm_shuffle_ps(kOtherRsqrt, kOtherRsqrt, 0x00), kOtherDelta)
                );
                if (hkpAutoRayBundleJob_canBundleRays(
                        kFrom, kTo, kDirection, kOtherFrom, kOtherTo, kOtherDirection, fMinCos, fMaxDistanceSquared
                    )) {
                    pJoined = &rBundle;
                    break;
                }
            }
        }
        if (pJoined) {
            pJoined->m_rayIndices[pJoined->m_numRays] = iRay;
            ++pJoined->m_numRays;
        }
        else {
            hkpAutoRayBundle* pNew = bundles.m_data + bundles.m_size;
            if (pNew) {
                pNew->m_rayIndices[0] = iRay;
                pNew->m_rayIndices[1] = 0;
                pNew->m_rayIndices[2] = 0;
                pNew->m_rayIndices[3] = 0;
                pNew->m_numRays = 1;
            }
            ++bundles.m_size;
        }
    }

    hkpAutoRayBundleJob_writeCommands(
        bundles, from, to, outputs, commandsOut, numCommandsOut, bundleCommandsOut, numBundleCommandsOut
    );
}

bool hkpAutoRayBundleJob_canBundleRays(
    const hkVector4& from0,
    const hkVector4& to0,
    const hkVector4& direction0,
    const hkVector4& from1,
    const hkVector4& to1,
    const hkVector4& direction1,
    float minCos,
    float maxDistanceSquared
) {
    __m128 v8 = _mm_mul_ps(direction0.m_quad, direction1.m_quad);
    bool result = false;
    if (!(minCos > static_cast<double>((_mm_shuffle_ps(v8, v8, 170)[0] + (_mm_shuffle_ps(v8, v8, 85)[0] + v8[0]))))) {
        __m128 v10 = _mm_sub_ps(from1.m_quad, from0.m_quad);
        __m128 v11 = _mm_mul_ps(v10, v10);
        __m128 v12 = _mm_sub_ps(to1.m_quad, to0.m_quad);
        if (!(maxDistanceSquared <
              static_cast<double>((_mm_shuffle_ps(v11, v11, 170)[0] + (_mm_shuffle_ps(v11, v11, 85)[0] + v11[0]))))) {
            __m128 v13 = _mm_mul_ps(v12, v12);
            if (!(maxDistanceSquared < static_cast<double>((
                                           _mm_shuffle_ps(v13, v13, 170)[0] + (_mm_shuffle_ps(v13, v13, 85)[0] + v13[0])
                                       )))) {
                return true;
            }
        }
    }
    return result;
}

// Each bundle of one ray becomes a single-ray command: the ray, no shape collection filter (filter info 0), that ray's
// output as its one-result buffer, no hits yet, not stopping at the first hit. A larger bundle becomes a bundle
// command: each ray with no filter, its output and no hits yet, then the ray count, not stopping at the first hit.
// m_useCollector is not written. The counts are stored last; the return value is the single-ray count (0 when there are
// no bundles).
int32_t hkpAutoRayBundleJob_writeCommands(
    const hkArray<hkpAutoRayBundle>& bundles,
    const hkVector4* from,
    const hkVector4* to,
    hkpWorldRayCastOutput* outputs,
    hkpWorldRayCastCommand* commandsOut,
    int32_t* numCommandsOut,
    hkpWorldRayCastBundleCommand* bundleCommandsOut,
    int32_t* numBundleCommandsOut
) {
    if (bundles.m_size <= 0) {
        *numCommandsOut = 0;
        *numBundleCommandsOut = 0;
        return 0;
    }
    int32_t iNumCommands = 0;
    int32_t iNumBundleCommands = 0;
    hkpWorldRayCastCommand* pCommand = commandsOut;
    hkpWorldRayCastBundleCommand* pBundleCommand = bundleCommandsOut;
    const int32_t iNumBundles = bundles.m_size;
    for (int32_t i = 0; i < iNumBundles; ++i) {
        const hkpAutoRayBundle& rBundle = bundles.m_data[i];
        const int32_t iNumRays = rBundle.m_numRays;
        if (iNumRays == 1) {
            const int32_t iRay = rBundle.m_rayIndices[0];
            pCommand->m_rayInput.m_from = from[iRay];
            pCommand->m_rayInput.m_to = to[iRay];
            pCommand->m_rayInput.m_filterInfo = 0;
            pCommand->m_rayInput.m_enableShapeCollectionFilter = false;
            pCommand->m_results = &outputs[iRay];
            pCommand->m_resultsCapacity = 1;
            pCommand->m_numResultsOut = 0;
            pCommand->m_stopAfterFirstHit = false;
            ++pCommand;
            ++iNumCommands;
        }
        else {
            for (int32_t j = 0; j < iNumRays; ++j) {
                const int32_t iRay = rBundle.m_rayIndices[j];
                hkpWorldRayCastInput& rInput = pBundleCommand->m_rayInput[j];
                rInput.m_from = from[iRay];
                rInput.m_to = to[iRay];
                rInput.m_filterInfo = 0;
                rInput.m_enableShapeCollectionFilter = false;
                pBundleCommand->m_results[j] = &outputs[iRay];
                pBundleCommand->m_numResultsOut[j] = 0;
            }
            pBundleCommand->m_numActiveRays = iNumRays;
            pBundleCommand->m_stopAfterFirstHit = false;
            ++pBundleCommand;
            ++iNumBundleCommands;
        }
    }
    *numCommandsOut = iNumCommands;
    *numBundleCommandsOut = iNumBundleCommands;
    return iNumCommands;
}

// The job's command counts are cleared and its rays are bundled into commands, the listed ones when m_rayIndices is
// set, else those of its octant; then the queue hands out the next job.
hkJobQueue::JobStatus hkCpuAutoRayBundleJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN("CpuAutoRayBundleJob", nullptr);
    // The queue entry holds the job: its hkJob base is the job's.
    hkpWorldRayCastWithAutoBundleJob& job =
        static_cast<hkpWorldRayCastWithAutoBundleJob&>(static_cast<hkJob&>(nextJobOut));
    job.m_numSingleCommands = 0;
    job.m_numBundledCommands = 0;
    if (job.m_rayIndices) {
        hkpAutoRayBundleJob_bundleIndexedRays(
            job.m_rayIndices, job.m_numIndices, job.m_rayFrom, job.m_rayTo, job.m_numRays, job.m_angleVariance,
            job.m_distanceVariance2, job.m_resultBuffer, job.m_singleCommands, &job.m_numSingleCommands,
            job.m_bundledCommands, &job.m_numBundledCommands
        );
    }
    else {
        hkpAutoRayBundleJob_bundleOctantRays(
            job.m_octantIdx, job.m_rayFrom, job.m_rayTo, job.m_numRays, job.m_angleVariance, job.m_distanceVariance2,
            job.m_resultBuffer, job.m_singleCommands, &job.m_numSingleCommands, job.m_bundledCommands,
            &job.m_numBundledCommands
        );
    }
    HK_TIMER_END();
    return jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

// The current time, also stored to apTime when it is not null.
int64_t time64(int64_t* apTime) {
    return std::time(apTime);
}

int32_t getCurrentTime() {
    return (int32_t)std::time(nullptr);
}
