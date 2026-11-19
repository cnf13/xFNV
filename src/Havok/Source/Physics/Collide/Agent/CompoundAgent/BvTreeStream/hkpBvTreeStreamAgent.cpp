#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTreeStream/hkpBvTreeStreamAgent.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeListAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTree/hkpBvTreeAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/ShapeCollection/hkpShapeCollectionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/BvTree3/hkpBvTreeAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/hkpAgentMachineUtil.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpConvexListFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/hkpBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include <xmmintrin.h>

// A Hoare partition around the middle key, then the two halves. The fourth argument is handed to the recursive calls
// and never read; the compares against the keys are unsigned, those between the indices signed.
void quickSortRecursive(uint32_t* keys, int32_t low, int32_t high, int32_t unused) {
    int32_t i = low;
    int32_t j = high;
    const uint32_t pivot = keys[(low + high) >> 1];
    do {
        while (keys[i] < pivot) {
            ++i;
        }
        while (pivot < keys[j]) {
            --j;
        }
        if (j < i) {
            break;
        }
        if (i != j) {
            const uint32_t key = keys[i];
            keys[i] = keys[j];
            keys[j] = key;
        }
        --j;
        ++i;
    }
    while (i <= j);
    if (low < j) {
        quickSortRecursive(keys, low, j, unused);
    }
    if (i < high) {
        quickSortRecursive(keys, i, high, unused);
    }
}

// The input's dispatcher, an empty cached box and a new agent track.
hkpBvTreeStreamAgent::hkpBvTreeStreamAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
)
  : hkpCollisionAgent(mgr), m_dispatcher(input.m_dispatcher) {
    m_cachedAabb.m_min.m_quad = _mm_setzero_ps();
    m_cachedAabb.m_max.m_quad = _mm_setzero_ps();
    hkAgent1nMachine_Create(m_agentTrack);
}

// The agent track frees its sectors in its own destructor.
hkpBvTreeStreamAgent::~hkpBvTreeStreamAgent() {}

void hkpBvTreeStreamAgent::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t m_capacityAndFlags = m_agentTrack.m_sectors.m_capacityAndFlags;
    const hkpAgent1nTrack* p_m_agentTrack = &m_agentTrack;
    if (m_capacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(
            p_m_agentTrack->m_sectors.m_data,
            static_cast<int32_t>(sizeof(*p_m_agentTrack->m_sectors.m_data)) * m_capacityAndFlags
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "SectorPtrs", p_m_agentTrack->m_sectors.m_data,
            static_cast<int32_t>(sizeof(*p_m_agentTrack->m_sectors.m_data)) * p_m_agentTrack->m_sectors.m_size,
            AllocatedSize
        );
    }
    hkpAgentMachineUtil::calc1nStatistics(p_m_agentTrack, collector->m_clientData, collector);
}

void hkpBvTreeStreamAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    hkpBvTreeAgent::staticGetPenetrations(bodyA, bodyB, input, collector);
}

void hkpBvTreeStreamAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    hkpBvTreeAgent::staticGetClosestPoints(bodyA, bodyB, input, collector);
}

void hkpBvTreeStreamAgent::linearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    hkpBvTreeAgent::staticLinearCast(bodyA, bodyB, input, collector, startCollector);
}

// The agent3 input is deliberately filled only partly, as the game does (the bodies, the input, the contact manager,
// m_aTb and the linear TIM information), never its padding nor m_distAtT1, so hkAgent1nMachine_Process copies
// uninitialised fields. The AABB of A in B's space is built as hkBvTreeAgent3::calcAabbAndQueryTree builds it, against
// the agent's cached AABB. On a cache hit the track is processed with no hit list (the child agents are kept);
// otherwise B's tree is queried into a 128-key inplace list whose first key is the invalid one (it sorts last and ends
// the list), and the track runs only when the heap can take a sector for every four hits it may add.
void hkpBvTreeStreamAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN_LIST("BvTree3", "QueryTree");

    alignas(16) hkpAgent3ProcessInput in3;
    in3.m_bodyA = &bodyA;
    in3.m_bodyB = &bodyB;
    in3.m_overrideBodyA = nullptr;
    in3.m_input = &input;
    in3.m_contactMgr = m_contactMgr;
    {
        const hkMotionState* msA = bodyA.m_motion.pState;
        const hkMotionState* msB = bodyB.m_motion.pState;
        // EXE-INLINED none: hkSweptTransformUtil::calcTimInfo
        const float fDeltaTime = input.m_stepInfo.m_deltaTime;
        const float fScaleA = msA->m_sweptTransform.m_centerOfMass1.m_quad[3] * fDeltaTime;
        const float fScaleB = fDeltaTime * msB->m_sweptTransform.m_centerOfMass1.m_quad[3];
        const __m128 kDiffA = _mm_mul_ps(
            _mm_sub_ps(msA->m_sweptTransform.m_centerOfMass0.m_quad, msA->m_sweptTransform.m_centerOfMass1.m_quad),
            _mm_set1_ps(fScaleA)
        );
        in3.m_linearTimInfo.m_quad = _mm_add_ps(
            _mm_mul_ps(
                _mm_set1_ps(fScaleB),
                _mm_sub_ps(msB->m_sweptTransform.m_centerOfMass1.m_quad, msB->m_sweptTransform.m_centerOfMass0.m_quad)
            ),
            kDiffA
        );
        in3.m_linearTimInfo.m_quad[3] = msB->m_deltaAngle.m_quad[3] * fScaleB * msB->m_objectRadius +
            msA->m_deltaAngle.m_quad[3] * fScaleA * msA->m_objectRadius;
        in3.m_aTb.setMulInverseMul(msA->m_transform, msB->m_transform);
    }

    // The SDK's inplace list over its own storage, holding the invalid shape key.
    alignas(16) hkInplaceArray<uint32_t, 128> hitList(1);
    hitList.m_storage[0] = 0xFFFFFFFF;

    alignas(16) hkTransform bTa;
    bTa.setInverse(in3.m_aTb);

    const hkMotionState* msA = bodyA.m_motion.pState;
    const hkMotionState* msB = bodyB.m_motion.pState;
    const hkRotation& rotB = msB->m_transform.m_rotation;
    const __m128 kCol0 = rotB.m_col0.m_quad;
    const __m128 kCol1 = rotB.m_col1.m_quad;
    const __m128 kCol2 = rotB.m_col2.m_quad;
    const __m128 kLo = _mm_unpacklo_ps(kCol0, kCol1);
    const __m128 kHi = _mm_unpackhi_ps(kCol0, kCol1);
    const __m128 kRow0 = _mm_movelh_ps(kLo, kCol2);
    const __m128 kRow1 = _mm_shuffle_ps(_mm_movehl_ps(kRow0, kLo), kCol2, 0xD4);
    const __m128 kRow2 = _mm_shuffle_ps(kHi, kCol2, 0xE4);

    // EXE-INLINED none: hkVector4::_setRotatedInverseDir
    const __m128 kTim = in3.m_linearTimInfo.m_quad;
    const __m128 kTimInB = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(kTim, kTim, 0x55), kRow1), _mm_mul_ps(_mm_shuffle_ps(kTim, kTim, 0x00), kRow0)
        ),
        _mm_mul_ps(_mm_shuffle_ps(kTim, kTim, 0xAA), kRow2)
    );

    float fSpill;
    hkAabb aabb;
    __m128 kMin;
    __m128 kMax;
    __m128 kExtent;
    __m128 kMotion = kTimInB;
    // EXE-INLINED none: hkPadSpu<int>::operator int
    if (input.m_collisionQualityInfo->m_useContinuousPhysics.m_storage != 0) {
        const float fRadiusB = msB->m_deltaAngle.m_quad[3] * msB->m_deltaAngle.m_quad[3] * msB->m_objectRadius;
        const float fRadiusA = (msA->m_deltaAngle.m_quad[3] + msB->m_deltaAngle.m_quad[3]) * msA->m_objectRadius;
        const float fTolerance = fRadiusA + fRadiusB + input.m_tolerance * 0.5;
        bodyA.m_shape->getAabbImpl(bTa, fTolerance, aabb);

        const float fRadius = input.m_tolerance * 0.5 + msA->m_objectRadius + fRadiusB;
        // EXE-INLINED none: hkVector4::setSub4
        const __m128 kOffset =
            _mm_sub_ps(msA->m_sweptTransform.m_centerOfMass1.m_quad, msB->m_transform.m_translation.m_quad);
        // EXE-INLINED none: hkVector4::_setRotatedInverseDir
        const __m128 kCenter = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(kOffset, kOffset, 0x55), kRow1),
                _mm_mul_ps(_mm_shuffle_ps(kOffset, kOffset, 0x00), kRow0)
            ),
            _mm_mul_ps(kRow2, _mm_shuffle_ps(kOffset, kOffset, 0xAA))
        );
        const __m128 kRadius = _mm_set1_ps(fRadius);
        kMin = _mm_max_ps(aabb.m_min.m_quad, _mm_sub_ps(kCenter, kRadius));
        kMax = _mm_min_ps(aabb.m_max.m_quad, _mm_add_ps(kCenter, kRadius));
        kExtent = _mm_sub_ps(kMax, kMin);
        fSpill = fRadius;

        if (0.0f < msB->m_deltaAngle.m_quad[3]) {
            const float fScale = msB->m_sweptTransform.m_centerOfMass1.m_quad[3] * input.m_stepInfo.m_deltaTime;
            const __m128 kArm = _mm_sub_ps(kCenter, msB->m_sweptTransform.m_centerOfMassLocal.m_quad);
            const __m128 kAngle = msB->m_deltaAngle.m_quad;
            // EXE-INLINED none: hkVector4::setCross
            const __m128 kCross = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(kAngle, kAngle, 0xD2), _mm_shuffle_ps(kArm, kArm, 0xC9)),
                _mm_mul_ps(_mm_shuffle_ps(kAngle, kAngle, 0xC9), _mm_shuffle_ps(kArm, kArm, 0xD2))
            );
            kMotion = _mm_add_ps(_mm_mul_ps(kCross, _mm_set1_ps(fScale)), kTimInB);
            fSpill = fScale;
        }
        const __m128 kZero = _mm_setzero_ps();
        kMin = _mm_add_ps(_mm_min_ps(kZero, kMotion), kMin);
        kMax = _mm_add_ps(_mm_max_ps(kZero, kMotion), kMax);
        aabb.m_min.m_quad = kMin;
        aabb.m_max.m_quad = kMax;
    }
    else {
        const float fTolerance = input.m_tolerance * 0.5;
        bodyA.m_shape->getAabbImpl(bTa, fTolerance, aabb);
        kMax = aabb.m_max.m_quad;
        kMin = aabb.m_min.m_quad;
        kExtent = _mm_sub_ps(kMax, kMin);
        fSpill = fTolerance;
    }

    bool bCacheHit = false;
    hkAabb* cachedAabb = &m_cachedAabb;
    if (cachedAabb) {
        // EXE-INLINED none: hkAabb::contains
        const __m128 kInside =
            _mm_and_ps(_mm_cmple_ps(cachedAabb->m_min.m_quad, kMin), _mm_cmple_ps(kMax, cachedAabb->m_max.m_quad));
        if ((_mm_movemask_ps(kInside) & 7) == 7) {
            bCacheHit = true;
        }
        else {
            const float fTolerance = input.m_tolerance * 0.5;
            const __m128 kTolerance = _mm_set1_ps(fTolerance);
            const __m128 kZero = _mm_setzero_ps();
            const __m128 kMargin = _mm_mul_ps(_mm_set1_ps(0.4f), kExtent);
            const __m128 kLowered = _mm_sub_ps(kMin, kTolerance);
            const __m128 kRaised = _mm_add_ps(kTolerance, kMax);
            // Flips the sign with the -0.0f mask.
            kMin = _mm_add_ps(
                _mm_max_ps(
                    _mm_mul_ps(_mm_set1_ps(-2.0f), _mm_max_ps(kZero, kMotion)), _mm_xor_ps(kMargin, _mm_set1_ps(-0.0f))
                ),
                kLowered
            );
            kMax = _mm_add_ps(_mm_min_ps(_mm_mul_ps(_mm_set1_ps(-2.0f), _mm_min_ps(kZero, kMotion)), kMargin), kRaised);
            aabb.m_min.m_quad = kMin;
            aabb.m_max.m_quad = kMax;
            cachedAabb->m_min.m_quad = kMin;
            cachedAabb->m_max.m_quad = kMax;
            fSpill = -2.0f;
        }
    }
    if (!bCacheHit) {
        static_cast<const hkpBvTreeShape*>(bodyB.m_shape)->queryAabb(aabb, hitList);
    }

    HK_TIMER_SPLIT_LIST("Narrow");

    if (bCacheHit) {
        hkAgent1nMachine_Process(m_agentTrack, in3, bodyB.m_shape->getContainer(), nullptr, result);
        HK_TIMER_END_LIST();
    }
    else if (!hkMemoryRouter::getInstance().heap().canAllocTotal(
                 (hitList.m_size / 4 - m_agentTrack.m_sectors.m_size + 1) *
                 static_cast<int32_t>(sizeof(hkpAgent1nSector))
             )) {
        hkSetOutOfMemoryState(hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY);
        HK_TIMER_END_LIST();
    }
    else {
        if (hitList.m_size > 1) {
            // The comparator is an empty functor passed by value: its low byte is zeroed while its upper three bytes
            // still hold the float fSpill, as the game does.
            const uint32_t uiComparator = __builtin_bit_cast(uint32_t, fSpill) & 0xFFFFFF00;
            quickSortRecursive(hitList.m_data, 0, hitList.m_size - 1, static_cast<int32_t>(uiComparator));
        }
        hkAgent1nMachine_Process(m_agentTrack, in3, bodyB.m_shape->getContainer(), hitList.m_data, result);
        HK_TIMER_END_LIST();
    }
}

// Drops the child agents' constraints and frees the agent: runs the track's cleanup and then its own deleting
// destructor.
void hkpBvTreeStreamAgent::cleanup(hkpConstraintOwner& info) {
    hkAgent1nMachine_Destroy(m_agentTrack, m_dispatcher, m_contactMgr, info);
    delete this;
}

void hkpBvTreeStreamAgent::updateShapeCollectionFilter(
    const hkpCdBody& bodyA,
    const hkpCdBody& collectionBodyB,
    const hkpCollisionInput& input,
    hkpConstraintOwner& constraintOwner
) {
    alignas(16) hkpAgent1nMachine_VisitorInput a2;
    m_cachedAabb.m_min.m_quad = _mm_setzero_ps();
    m_cachedAabb.m_max.m_quad = _mm_setzero_ps();
    a2.m_containerShapeB = collectionBodyB.m_shape->getContainer();
    a2.m_bodyA = &bodyA;
    hkpContactMgr* m_contactMgr = this->m_contactMgr;
    a2.m_input = &input;
    a2.m_collectionBodyB = &collectionBodyB;
    a2.m_contactMgr = m_contactMgr;
    a2.m_constraintOwner = &constraintOwner;
    hkAgent1nMachine_UpdateShapeCollectionFilter(m_agentTrack, a2);
}

void hkpBvTreeStreamAgent::invalidateTim(const hkpCollisionInput& input) {
    hkAgent1nMachine_InvalidateTim(m_agentTrack, input);
}

// The times of the child agents move with the agent's; forwards to the track's warp time.
void hkpBvTreeStreamAgent::warpTime(float oldTime, float newTime, const hkpCollisionInput& input) {
    hkAgent1nMachine_WarpTime(m_agentTrack, oldTime, newTime, input);
}

// Registers the flipped agent for (bounding volume tree, convex), then the plain one for (convex, bounding volume
// tree); both are predictive. One AgentFuncs is reused for both. registerCollisionAgent copies af's flags, so the two
// padding bytes after m_isPredictive, never written here, land in the dispatcher's table, as the game does.
void hkpBvTreeStreamAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs af;
    af.m_createFunc = &hkpBvTreeStreamAgent::createBvTreeShapeAgent;
    af.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticGetPenetrations;
    af.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticLinearCast;
    af.m_isFlipped.m_bool = 1;
    af.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(af, HK_SHAPE_BV_TREE, HK_SHAPE_CONVEX);

    af.m_createFunc = &hkpBvTreeStreamAgent::createShapeBvAgent;
    af.m_getPenetrationsFunc = &hkpBvTreeAgent::staticGetPenetrations;
    af.m_getClosestPointFunc = &hkpBvTreeAgent::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpBvTreeAgent::staticLinearCast;
    af.m_isFlipped.m_bool = 0;
    af.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(af, HK_SHAPE_CONVEX, HK_SHAPE_BV_TREE);
}

hkpCollisionAgent* hkpBvTreeStreamAgent::createShapeBvAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpBvTreeStreamAgent* v5 =
        static_cast<hkpBvTreeStreamAgent*>(Instance->m_heap->blockAlloc(sizeof(hkpBvTreeStreamAgent)));
    v5->m_memSizeAndFlags = sizeof(hkpBvTreeStreamAgent);
    return ::new (v5) hkpBvTreeStreamAgent(bodyA, bodyB, input, mgr);
}

hkpCollisionAgent* hkpBvTreeStreamAgent::dispatchConvexListBvTree(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    if (mgr) {
        hkpConvexListFilter::ConvexListCollisionType v4 =
            input.m_convexListFilter->getConvexListCollisionType(bodyA, bodyB, input);
        if (v4) {
            int32_t v5 = v4 - 1;
            if (!v5) {
                hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
                hkpShapeCollectionAgent* v8 = static_cast<hkpShapeCollectionAgent*>(
                    Instance->m_heap->blockAlloc(sizeof(hkpShapeCollectionAgent))
                );
                v8->m_memSizeAndFlags = sizeof(hkpShapeCollectionAgent);
                return ::new (v8) hkpShapeCollectionAgent(bodyA, bodyB, input, mgr);
            }
            if (v5 != 1) {
                return nullptr;
            }
        }
        hkMemoryRouter* v9 = &hkMemoryRouter::getInstance();
        hkpBvTreeStreamAgent* v10 =
            static_cast<hkpBvTreeStreamAgent*>(v9->m_heap->blockAlloc(sizeof(hkpBvTreeStreamAgent)));
        v10->m_memSizeAndFlags = sizeof(hkpBvTreeStreamAgent);
        return ::new (v10) hkpBvTreeStreamAgent(bodyA, bodyB, input, mgr);
    }

    hkMemoryRouter* v11 = &hkMemoryRouter::getInstance();
    hkpSymmetricAgent<hkpBvTreeStreamAgent>* v12 = static_cast<hkpSymmetricAgent<hkpBvTreeStreamAgent>*>(
        v11->m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpBvTreeStreamAgent>))
    );
    v12->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpBvTreeStreamAgent>);
    return ::new (v12) hkpSymmetricAgent<hkpBvTreeStreamAgent>(bodyA, bodyB, input, nullptr);
}

hkpCollisionAgent* hkpBvTreeStreamAgent::createBvTreeShapeAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpSymmetricAgent<hkpBvTreeStreamAgent>* v5 = static_cast<hkpSymmetricAgent<hkpBvTreeStreamAgent>*>(
        Instance->m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpBvTreeStreamAgent>))
    );
    v5->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpBvTreeStreamAgent>);
    return ::new (v5) hkpSymmetricAgent<hkpBvTreeStreamAgent>(bodyA, bodyB, input, mgr);
}

hkpCollisionAgent* hkpBvTreeStreamAgent::dispatchBvTreeConvexList(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    if (mgr) {
        hkpConvexListFilter::ConvexListCollisionType v4 =
            input.m_convexListFilter->getConvexListCollisionType(bodyB, bodyA, input);
        if (v4) {
            int32_t v5 = v4 - 1;
            if (!v5) {
                hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
                hkpSymmetricAgent<hkpShapeCollectionAgent>* v8 =
                    static_cast<hkpSymmetricAgent<hkpShapeCollectionAgent>*>(
                        Instance->m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpShapeCollectionAgent>))
                    );
                v8->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpShapeCollectionAgent>);
                return ::new (v8) hkpSymmetricAgent<hkpShapeCollectionAgent>(bodyA, bodyB, input, mgr);
            }
            if (v5 != 1) {
                return nullptr;
            }
        }
        hkMemoryRouter* v9 = &hkMemoryRouter::getInstance();
        hkpSymmetricAgent<hkpBvTreeStreamAgent>* v10 = static_cast<hkpSymmetricAgent<hkpBvTreeStreamAgent>*>(
            v9->m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpBvTreeStreamAgent>))
        );
        v10->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpBvTreeStreamAgent>);
        return ::new (v10) hkpSymmetricAgent<hkpBvTreeStreamAgent>(bodyA, bodyB, input, mgr);
    }

    hkMemoryRouter* v11 = &hkMemoryRouter::getInstance();
    hkpSymmetricAgent<hkpBvTreeStreamAgent>* v12 = static_cast<hkpSymmetricAgent<hkpBvTreeStreamAgent>*>(
        v11->m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpBvTreeStreamAgent>))
    );
    v12->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpBvTreeStreamAgent>);
    return ::new (v12) hkpSymmetricAgent<hkpBvTreeStreamAgent>(bodyA, bodyB, input, nullptr);
}

// Registers the flipped agent for (bounding volume tree, multi-ray), then the plain one for (multi-ray, bounding volume
// tree); both are predictive. One AgentFuncs is reused for both registrations. registerCollisionAgent copies af's
// flags, so the two padding bytes after m_isPredictive, never written here, land in the dispatcher's table, as the game
// does.
void hkpBvTreeStreamAgent::registerMultiRayAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs af;
    af.m_createFunc = &hkpBvTreeStreamAgent::createBvTreeShapeAgent;
    af.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticGetPenetrations;
    af.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticLinearCast;
    af.m_isFlipped.m_bool = 1;
    af.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(af, HK_SHAPE_BV_TREE, HK_SHAPE_MULTI_RAY);

    af.m_createFunc = &hkpBvTreeStreamAgent::createShapeBvAgent;
    af.m_getPenetrationsFunc = &hkpBvTreeAgent::staticGetPenetrations;
    af.m_getClosestPointFunc = &hkpBvTreeAgent::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpBvTreeAgent::staticLinearCast;
    af.m_isFlipped.m_bool = 0;
    af.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(af, HK_SHAPE_MULTI_RAY, HK_SHAPE_BV_TREE);
}

// Registers the flipped dispatcher for (bounding volume tree, convex list), then the plain one for (convex list,
// bounding volume tree); both are predictive. One AgentFuncs is reused for both. registerCollisionAgent copies af's
// flags, so the two padding bytes after m_isPredictive, never written here, land in the dispatcher's table, as the game
// does.
void hkpBvTreeStreamAgent::registerConvexListAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs af;
    af.m_createFunc = &hkpBvTreeStreamAgent::dispatchBvTreeConvexList;
    af.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticGetPenetrations;
    af.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticLinearCast;
    af.m_isFlipped.m_bool = 1;
    af.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(af, HK_SHAPE_BV_TREE, HK_SHAPE_CONVEX_LIST);

    af.m_createFunc = &hkpBvTreeStreamAgent::dispatchConvexListBvTree;
    af.m_getPenetrationsFunc = &hkpBvTreeAgent::staticGetPenetrations;
    af.m_getClosestPointFunc = &hkpBvTreeAgent::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpBvTreeAgent::staticLinearCast;
    af.m_isFlipped.m_bool = 0;
    af.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(af, HK_SHAPE_CONVEX_LIST, HK_SHAPE_BV_TREE);
}

// The agent3 table's warp time for this agent: the entry is not used, the agent data is the track.
void hkBvTreeAgent3::warpTime(
    hkpAgentEntry*,
    hkpAgentData* agentData,
    float oldTime,
    float newTime,
    const hkpCollisionInput& input
) {
    hkAgent1nMachine_WarpTime(*static_cast<hkpAgent1nTrack*>(agentData), oldTime, newTime, input);
}
