#include "Havok/Source/Physics/Collide/Agent3/BvTree3/hkpBvTreeAgent3.h"

#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTreeStream/hkpBvTreeStreamAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/List3/hkpListAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/hkpAgentMachineUtil.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/hkpBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/TriSampledHeightField/hkpTriSampledHeightFieldBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "SDK/MSVC/string.h"

#include <xmmintrin.h>

// A's AABB in B's space is queried against B's tree into a hit list on the Havok stack, and the 1n machine runs the
// hits (none when the cached AABB answered) against the track agentData holds. The agent's data is the track, rounded
// up to 16 bytes.
hkpAgentData* hkBvTreeAgent3::process(
    const hkpAgent3ProcessInput& input,
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkVector4* separatingNormal,
    hkpProcessCollisionOutput& output
) {
    HK_TIMER_BEGIN_LIST("BvTree3", "QueryTree");

    uint32_t* hitList = hkAllocateStack<uint32_t>(HK_MAX_NUM_HITS_PER_AABB_QUERY, nullptr);

    hkTransform bTa;
    bTa.setInverse(input.m_aTb);
    const int32_t numHits = hkBvTreeAgent3::calcAabbAndQueryTree(
        *input.m_bodyA, *input.m_bodyB, bTa, input.m_linearTimInfo, *input.m_input, nullptr, hitList,
        HK_MAX_NUM_HITS_PER_AABB_QUERY
    );
    // The shape buffer passed to getShapeContainerFrom is never read by the callee.
    alignas(16) char acContainerBuffer[HK_SHAPE_BUFFER_SIZE];
    const hkpShapeContainer* shapeContainer = hkBvTreeAgent3::getShapeContainerFrom(input.m_bodyB, acContainerBuffer);

    HK_TIMER_SPLIT_LIST("Narrow");
    hkAgent1nMachine_Process(
        *static_cast<hkpAgent1nTrack*>(agentData), input, shapeContainer, numHits < 0 ? nullptr : hitList, output
    );
    HK_TIMER_END_LIST();

    hkDeallocateStack(hitList);
    return static_cast<char*>(agentData) + ((sizeof(hkpAgent1nTrack) + 15) & ~static_cast<size_t>(15));
}

// The Agent3Funcs constructor clears the flags and the optional functions; initAgentFunc fills the table.
// registerAgent3 copies all of f, so the bytes after m_reusePreviousEntry, never written here, land in the dispatcher's
// table: an uninitialised read, kept deliberately as the game does.
void hkBvTreeAgent3::registerAgent3(hkpCollisionDispatcher* dispatcher) {
    alignas(16) hkpCollisionDispatcher::Agent3Funcs f;
    f.m_isPredictive.m_bool = false;
    f.m_ignoreSymmetricVersion.m_bool = false;
    f.m_reusePreviousEntry.m_bool = false;
    f.m_updateFilterFunc = nullptr;
    f.m_calcStatisticsFunc = nullptr;
    f.m_invalidateTimFunc = nullptr;
    f.m_warpTimeFunc = nullptr;
    f.m_sepNormalFunc = nullptr;
    hkBvTreeAgent3::initAgentFunc(f);
    dispatcher->registerAgent3(f, HK_SHAPE_ALL, HK_SHAPE_BV_TREE);
}

// The table holds the list agent's create and destroy and this agent's process and track functions; the agent is
// predictive.
void hkBvTreeAgent3::initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f) {
    f.m_createFunc = &hkListAgent3::create;
    f.m_processFunc = &hkBvTreeAgent3::process;
    f.m_sepNormalFunc = nullptr;
    f.m_cleanupFunc = nullptr;
    f.m_removePointFunc = nullptr;
    f.m_commitPotentialFunc = nullptr;
    f.m_createZombieFunc = nullptr;
    f.m_updateFilterFunc = &hkBvTreeAgent3::updateFilter;
    f.m_calcStatisticsFunc = &hkBvTreeAgent3::calcStatistics;
    f.m_invalidateTimFunc = &hkBvTreeAgent3::invalidateTim;
    f.m_warpTimeFunc = &hkBvTreeAgent3::warpTime;
    f.m_destroyFunc = &hkListAgent3::destroy;
    f.m_isPredictive.m_bool = true;
}

void hkBvTreeAgent3::updateFilter(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner
) {
    hkpAgent1nMachine_VisitorInput vin;
    const hkpShapeContainer* Container = bodyB.m_shape->getContainer();
    vin.m_bodyA = &bodyA;
    vin.m_containerShapeB = Container;
    vin.m_contactMgr = mgr;
    vin.m_input = &input;
    vin.m_collectionBodyB = &bodyB;
    vin.m_constraintOwner = &constraintOwner;
    hkAgent1nMachine_UpdateShapeCollectionFilter(*static_cast<hkpAgent1nTrack*>(agentData), vin);
}

void hkBvTreeAgent3::calcStatistics(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    const hkpCollisionInput& input,
    hkStatisticsCollector* collector
) {
    collector->pushDir("hkBvTreeAgent3");
    hkpAgentMachineUtil::calc1nStatistics(agentData, &input, collector);
    collector->popDir();
}

void hkBvTreeAgent3::invalidateTim(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input) {
    hkAgent1nMachine_InvalidateTim(*static_cast<hkpAgent1nTrack*>(agentData), input);
}

// The vector steps are single precision; the rotations into B's space are hkVector4::_setRotatedInverseDir's transpose,
// whose w lanes are left as they are. The radii and tolerances are each stored as a float. With continuous physics A's
// AABB is clipped to the box of the swept radius around A's centre of mass in B's space and grown by the linear and the
// angular motion (the angular part when B turns, 0 < its delta angle; NaN skips it). With a cache, an AABB inside it
// ends the query (-1); otherwise the cache becomes the AABB grown by half the tolerance and by the motion (twice,
// backwards), bounded by 0.4 of the extent. The hits past the capacity go to the filter, and are sorted by quicksort.
int32_t hkBvTreeAgent3::calcAabbAndQueryTree(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkTransform& bTa,
    const hkVector4& linearTimInfo,
    const hkpProcessCollisionInput& input,
    hkAabb* cachedAabb,
    uint32_t* hitListOut,
    int32_t hitListCapacity
) {
    const hkMotionState* msA = bodyA.m_motion.pState;
    const hkMotionState* msB = bodyB.m_motion.pState;
    const hkRotation& rotB = msB->m_transform.m_rotation;
    const __m128 kCol0 = _mm_load_ps(&reinterpret_cast<const float*>(&rotB.m_col0.m_quad)[0]);
    const __m128 kCol1 = _mm_load_ps(&reinterpret_cast<const float*>(&rotB.m_col1.m_quad)[0]);
    const __m128 kCol2 = _mm_load_ps(&reinterpret_cast<const float*>(&rotB.m_col2.m_quad)[0]);
    const __m128 kLo = _mm_unpacklo_ps(kCol0, kCol1);
    const __m128 kHi = _mm_unpackhi_ps(kCol0, kCol1);
    const __m128 kRow0 = _mm_movelh_ps(kLo, kCol2);
    const __m128 kRow1 = _mm_shuffle_ps(_mm_movehl_ps(kRow0, kLo), kCol2, 0xD4);
    const __m128 kRow2 = _mm_shuffle_ps(kHi, kCol2, 0xE4);

    // EXE-INLINED none: hkVector4::_setRotatedInverseDir
    const __m128 kTim = _mm_load_ps(&reinterpret_cast<const float*>(&linearTimInfo.m_quad)[0]);
    const __m128 kTimInB = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(kTim, kTim, 0x55), kRow1), _mm_mul_ps(_mm_shuffle_ps(kTim, kTim, 0x00), kRow0)
        ),
        _mm_mul_ps(_mm_shuffle_ps(kTim, kTim, 0xAA), kRow2)
    );

    float fSpill;
    // The hkAabb default constructor is deliberately not called for aabb.
    hkAabb aabb;
    __m128 kMin;
    __m128 kMax;
    __m128 kExtent;
    __m128 kMotion = kTimInB;
    if (input.m_collisionQualityInfo->m_useContinuousPhysics != 0) {
        const float fRadiusB = msB->m_deltaAngle.m_quad[3] * msB->m_deltaAngle.m_quad[3] * msB->m_objectRadius;
        const float fRadiusA = (msA->m_deltaAngle.m_quad[3] + msB->m_deltaAngle.m_quad[3]) * msA->m_objectRadius;
        const float fTolerance = fRadiusA + fRadiusB + input.m_tolerance * 0.5;
        bodyA.m_shape->getAabbImpl(bTa, fTolerance, aabb);

        const float fRadius = input.m_tolerance * 0.5 + msA->m_objectRadius + fRadiusB;
        // EXE-INLINED none: hkVector4::setSub4
        const __m128 kOffset = _mm_sub_ps(
            _mm_load_ps(&reinterpret_cast<const float*>(&msA->m_sweptTransform.m_centerOfMass1.m_quad)[0]),
            _mm_load_ps(&reinterpret_cast<const float*>(&msB->m_transform.m_translation.m_quad)[0])
        );
        // EXE-INLINED none: hkVector4::_setRotatedInverseDir
        const __m128 kCenter = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(kOffset, kOffset, 0x55), kRow1),
                _mm_mul_ps(_mm_shuffle_ps(kOffset, kOffset, 0x00), kRow0)
            ),
            _mm_mul_ps(kRow2, _mm_shuffle_ps(kOffset, kOffset, 0xAA))
        );
        const __m128 kRadius = _mm_set1_ps(fRadius);
        kMin = _mm_max_ps(_mm_load_ps(&reinterpret_cast<float*>(&aabb.m_min.m_quad)[0]), _mm_sub_ps(kCenter, kRadius));
        kMax = _mm_min_ps(_mm_load_ps(&reinterpret_cast<float*>(&aabb.m_max.m_quad)[0]), _mm_add_ps(kCenter, kRadius));
        kExtent = _mm_sub_ps(kMax, kMin);
        fSpill = fRadius;

        if (0.0f < msB->m_deltaAngle.m_quad[3]) {
            const float fScale = msB->m_sweptTransform.m_centerOfMass1.m_quad[3] * input.m_stepInfo.m_deltaTime;
            const __m128 kArm = _mm_sub_ps(
                kCenter,
                _mm_load_ps(&reinterpret_cast<const float*>(&msB->m_sweptTransform.m_centerOfMassLocal.m_quad)[0])
            );
            const __m128 kAngle = _mm_load_ps(&reinterpret_cast<const float*>(&msB->m_deltaAngle.m_quad)[0]);
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
        _mm_store_ps(&reinterpret_cast<float*>(&aabb.m_min.m_quad)[0], kMin);
        _mm_store_ps(&reinterpret_cast<float*>(&aabb.m_max.m_quad)[0], kMax);
    }
    else {
        const float fTolerance = input.m_tolerance * 0.5;
        bodyA.m_shape->getAabbImpl(bTa, fTolerance, aabb);
        kMax = _mm_load_ps(&reinterpret_cast<float*>(&aabb.m_max.m_quad)[0]);
        kMin = _mm_load_ps(&reinterpret_cast<float*>(&aabb.m_min.m_quad)[0]);
        kExtent = _mm_sub_ps(kMax, kMin);
        fSpill = fTolerance;
    }

    if (cachedAabb) {
        // EXE-INLINED none: hkAabb::contains
        const __m128 kInside = _mm_and_ps(
            _mm_cmple_ps(_mm_load_ps(&reinterpret_cast<float*>(&cachedAabb->m_min.m_quad)[0]), kMin),
            _mm_cmple_ps(kMax, _mm_load_ps(&reinterpret_cast<float*>(&cachedAabb->m_max.m_quad)[0]))
        );
        if ((_mm_movemask_ps(kInside) & 7) == 7) {
            return -1;
        }
        const float fTolerance = input.m_tolerance * 0.5;
        const __m128 kTolerance = _mm_set1_ps(fTolerance);
        const __m128 kZero = _mm_setzero_ps();
        const __m128 kMargin = _mm_mul_ps(_mm_set1_ps(0.4f), kExtent);
        const __m128 kLowered = _mm_sub_ps(kMin, kTolerance);
        const __m128 kRaised = _mm_add_ps(kTolerance, kMax);
        // The sign of kMargin is flipped with the -0.0f mask.
        kMin = _mm_add_ps(
            _mm_max_ps(
                _mm_mul_ps(_mm_set1_ps(-2.0f), _mm_max_ps(kZero, kMotion)), _mm_xor_ps(kMargin, _mm_set1_ps(-0.0f))
            ),
            kLowered
        );
        kMax = _mm_add_ps(_mm_min_ps(_mm_mul_ps(_mm_set1_ps(-2.0f), _mm_min_ps(kZero, kMotion)), kMargin), kRaised);
        _mm_store_ps(&reinterpret_cast<float*>(&aabb.m_min.m_quad)[0], kMin);
        _mm_store_ps(&reinterpret_cast<float*>(&aabb.m_max.m_quad)[0], kMax);
        _mm_store_ps(&reinterpret_cast<float*>(&cachedAabb->m_min.m_quad)[0], kMin);
        _mm_store_ps(&reinterpret_cast<float*>(&cachedAabb->m_max.m_quad)[0], kMax);
        fSpill = -2.0f;
    }

    int32_t numHits =
        static_cast<const hkpBvTreeShape*>(bodyB.m_shape)->queryAabbImpl(aabb, hitListOut, hitListCapacity);
    if (numHits >= hitListCapacity) {
        const hkpShapeCollectionFilter& filter = *input.m_filter;
        numHits = filter.numShapeKeyHitsLimitBreached(
            input, bodyA, bodyB, static_cast<const hkpBvTreeShape*>(bodyB.m_shape), aabb, hitListOut, hitListCapacity
        );
        if (numHits >= hitListCapacity - 1) {
            numHits = hitListCapacity - 1;
        }
    }
    if (numHits > 1) {
        // The comparator is an empty functor passed by value as a 32-bit value: its low byte is zeroed and its upper
        // three bytes deliberately still hold the bits of the float fSpill, as the game does.
        const uint32_t uiComparator = __builtin_bit_cast(uint32_t, fSpill) & 0xFFFFFF00;
        quickSortRecursive(hitListOut, 0, numHits - 1, static_cast<int32_t>(uiComparator));
    }
    // The invalid shape key ends the list.
    hitListOut[numHits] = 0xFFFFFFFF;
    return numHits;
}

// Havok SDK: the shape container of body's bounding volume tree shape. buffer, for a container built on the stack,
// is not used.
const hkpShapeContainer* hkBvTreeAgent3::getShapeContainerFrom(const hkpCdBody* body, char* buffer) {
    return body->m_shape->getContainer();
}

// Havok SDK: the shape collection a body's bounding volume tree queries by AABB: the child of a triangle sampled
// heightfield's tree or of a MOPP tree, else null. buffer is not used.
const hkpShapeCollection*
hkBvTreeAgent3::getShapeCollectionIfBvTreeSupportsAabbQueries(const hkpCdBody* body, char* buffer) {
    const hkpShape* pShape = body->m_shape;
    switch (pShape->m_type.m_storage) {
        case HK_SHAPE_BV_TREE:
            if (static_cast<const hkpBvTreeShape*>(pShape)->m_bvTreeType.m_storage ==
                hkpBvTreeShape::BVTREE_TRISAMPLED_HEIGHTFIELD) {
                return static_cast<const hkpShapeCollection*>(
                    static_cast<const hkpTriSampledHeightFieldBvTreeShape*>(pShape)->m_childContainer.m_childShape
                );
            }
            break;
        case HK_SHAPE_MOPP:
            return static_cast<const hkpShapeCollection*>(
                static_cast<const hkpMoppBvTreeShape*>(pShape)->m_child.m_childShape
            );
        default: break;
    }
    return nullptr;
}
