#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTree/hkpBvTreeAgent.h"

#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeListAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTreeStream/hkpBvTreeStreamAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Null/hkpNullAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/hkpBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "xfnv/Compiler.h"

#include <xmmintrin.h>

template <typename KEY, typename INFO, typename HELPER>
void hkpAgentDispatchUtil<KEY, INFO, HELPER>::fastUpdate(
    hkArray<INFO>& partners,
    hkArray<KEY>& keys,
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner,
    HELPER& helper
) {
    // Each end is computed on the address, deliberately (an array of 0x80000000 entries ends at its start): a pointer
    // end lets clang test the count instead of comparing addresses.
    KEY* keyEnd = reinterpret_cast<KEY*>(reinterpret_cast<uintptr_t>(keys.m_data) + keys.m_size * sizeof(KEY));
    {
        // The partners whose key is gone are cleaned up and removed.
        INFO* entry = partners.m_data;
        INFO* entryEnd = reinterpret_cast<INFO*>(
            reinterpret_cast<uintptr_t>(partners.m_data) + partners.m_size * sizeof(INFO)
        );
        KEY* key = keys.m_data;
        for (; entry != entryEnd; ++entry) {
            if (key != keyEnd && entry->m_key == *key) {
                ++key;
                continue;
            }
            KEY* k = keys.m_data;
            while (k != keyEnd && entry->m_key != *k) {
                ++k;
            }
            if (k != keyEnd) {
                key = k + 1;
                continue;
            }
            entry->m_collisionAgent->cleanup(constraintOwner);
            partners.removeAtAndCopy(static_cast<int32_t>(entry - partners.m_data));
            key = k;
            --entry;
            --entryEnd;
        }
    }
    if (keys.m_size != partners.m_size) {
        // A partner is inserted for each new key, in the keys' order.
        INFO* entry = partners.m_data;
        INFO* entryEnd = reinterpret_cast<INFO*>(
            reinterpret_cast<uintptr_t>(partners.m_data) + partners.m_size * sizeof(INFO)
        );
        for (KEY* key = keys.m_data; key != keyEnd; ++key, ++entry) {
            if (entry != entryEnd && entry->m_key == *key) {
                continue;
            }
            entry = partners.expandAt(static_cast<int32_t>(key - keys.m_data), 1);

            helper.m_bodyB.m_shape = helper.m_container->getChildShape(*key, helper.m_shapeBuffer);
            helper.m_bodyB.m_shapeKey = *key;
            hkpCollisionAgent* agent;
            const hkpShapeCollectionFilter& filter = *input.m_filter;
            if (filter.isCollisionEnabled(input, bodyA, bodyB, *helper.m_container, *key).m_bool) {
                agent = input.m_dispatcher->getNewCollisionAgent(bodyA, helper.m_bodyB, input, mgr);
            }
            else {
                agent = hkpNullAgent::getNullAgent();
            }
            entry->m_collisionAgent = agent;
            entry->m_key = *key;
            entryEnd = reinterpret_cast<INFO*>(
                reinterpret_cast<uintptr_t>(partners.m_data) + partners.m_size * sizeof(INFO)
            );
        }
    }
}

template <typename KEY, typename INFO, typename HELPER>
void hkpAgentDispatchUtil<KEY, INFO, HELPER>::update(
    hkArray<INFO>& partners,
    hkArray<KEY>& keys,
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner,
    HELPER& helper
) {
    // The comparator object (hkAlgorithm::less, an empty class), passed as its one byte.
    uint8_t lessCmp = 0;
    if (keys.m_size > 1) {
        quickSortRecursive(keys.m_data, 0, keys.m_size - 1, lessCmp);
    }
    INFO* entry = partners.m_data;
    // The ends on the addresses, as in fastUpdate.
    INFO* entryEnd = reinterpret_cast<INFO*>(
        reinterpret_cast<uintptr_t>(partners.m_data) + partners.m_size * sizeof(INFO)
    );
    KEY* key = keys.m_data;
    KEY* keyEnd = reinterpret_cast<KEY*>(reinterpret_cast<uintptr_t>(keys.m_data) + keys.m_size * sizeof(KEY));

    hkLocalArray<INFO> newEntries(keys.m_size);
    newEntries.reserve(keys.m_size);
    newEntries.setSizeUnchecked(keys.m_size);

    // The sorted keys and the sorted partners merged: a partner whose key is gone is cleaned up, a new key gets an
    // agent.
    INFO* out = newEntries.m_data;
    while (entry != entryEnd) {
        if (key == keyEnd) {
            do {
                if (entry->m_collisionAgent) {
                    entry->m_collisionAgent->cleanup(constraintOwner);
                }
                ++entry;
            }
            while (entry != entryEnd);
            break;
        }
        const KEY k = *key;
        if (k == entry->m_key) {
            *out = *entry;
            ++out;
            ++entry;
            ++key;
        }
        else if (k > entry->m_key) {
            if (entry->m_collisionAgent) {
                entry->m_collisionAgent->cleanup(constraintOwner);
            }
            ++entry;
        }
        else {
            helper.m_bodyB.m_shape = helper.m_container->getChildShape(k, helper.m_shapeBuffer);
            helper.m_bodyB.m_shapeKey = k;
            hkpCollisionAgent* agent;
            const hkpShapeCollectionFilter& filter = *input.m_filter;
            if (filter.isCollisionEnabled(input, bodyA, bodyB, *helper.m_container, *key).m_bool) {
                agent = input.m_dispatcher->getNewCollisionAgent(bodyA, helper.m_bodyB, input, mgr);
            }
            else {
                agent = hkpNullAgent::getNullAgent();
            }
            out->m_collisionAgent = agent;
            out->m_key = *key;
            ++out;
            ++key;
        }
    }
    for (; key != keyEnd; ++key, ++out) {
        const KEY k = *key;
        helper.m_bodyB.m_shape = helper.m_container->getChildShape(k, helper.m_shapeBuffer);
        helper.m_bodyB.m_shapeKey = k;
        hkpCollisionAgent* agent;
        const hkpShapeCollectionFilter& filter = *input.m_filter;
        if (filter.isCollisionEnabled(input, bodyA, bodyB, *helper.m_container, *key).m_bool) {
            agent = input.m_dispatcher->getNewCollisionAgent(bodyA, helper.m_bodyB, input, mgr);
        }
        else {
            agent = hkpNullAgent::getNullAgent();
        }
        out->m_collisionAgent = agent;
        out->m_key = *key;
    }

    partners = newEntries;
}

hkpCollisionAgent*
createShapeBvAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input, hkpContactMgr* mgr) {
    return new hkpBvTreeAgent(mgr);
}

hkpCollisionAgent* createBvTreeShapeAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    return new hkpSymmetricAgent<hkpBvTreeAgent>(mgr);
}

// No partners yet; both corners of the cached box are set to the same constant, so the first query misses it.
hkpBvTreeAgent::hkpBvTreeAgent(hkpContactMgr* mgr) : hkpCollisionAgent(mgr) {
    m_cachedAabb.m_max.m_quad = _mm_set1_ps(HK_REAL_MAX);
    m_cachedAabb.m_min.m_quad = m_cachedAabb.m_max.m_quad;
}

// The agent keeps no state for the query: the static version does the work. hkpMoppAgent's slot holds the same
// address.
void hkpBvTreeAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    staticGetPenetrations(bodyA, bodyB, input, collector);
}

void hkpBvTreeAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    hkpBvTreeAgent::staticGetClosestPoints(bodyA, bodyB, input, collector);
}

void hkpBvTreeAgent::linearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    hkpBvTreeAgent::staticLinearCast(bodyA, bodyB, input, collector, startCollector);
}

// BodyA's box in bodyB's space, grown by the bodies' motion over the step, queries bodyB's tree (unless the box lies
// inside the cached one), the partners are brought up to date with the hit keys, and each partner's agent collides
// bodyA with its child shape.
void hkpBvTreeAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN_LIST("BvTree", "QueryTree");
    hkpConstraintOwner* constraintOwner = result.m_constraintOwner.m_storage;
    using hitList_t = hkInplaceArray<uint32_t, 128>;
    hitList_t hitList;
    {
        alignas(16) hkTransform bTa;
        bTa.setMulInverseMul(*bodyB.m_motion.pMotion, *bodyA.m_motion.pMotion);
        const hkMotionState* msA = bodyA.m_motion.pState;
        const hkMotionState* msB = bodyB.m_motion.pState;

        alignas(16) hkVector4 linearTimInfo;
        hkSweptTransformUtil::calcTimInfo(*msA, *msB, input.m_stepInfo.m_deltaTime, linearTimInfo);
        hkAabb* cachedAabb = hkpBvTreeAgent::m_useAabbCaching ? &m_cachedAabb : nullptr;

        // The time info into bodyB's space; its w is overwritten with the rotated vector's.
        const hkRotation& rotB = msB->m_transform.m_rotation;
        linearTimInfo._setRotatedInverseDir(rotB, linearTimInfo);

        hkAabb aabb;
        // The motion the box is grown by: the linear one, plus bodyB's rotation about bodyA's center where the
        // continuous path has it.
        hkQuadReal motion;
        // The box's extent before it is grown by the motion.
        hkQuadReal extent;
        if (input.m_collisionQualityInfo->m_useContinuousPhysics.m_storage) {
            const float epsB = XFNV_M128_F32(msB->m_deltaAngle.m_quad, 3) * XFNV_M128_F32(msB->m_deltaAngle.m_quad, 3) *
                msB->m_objectRadius;
            const float rotEps = (XFNV_M128_F32(msA->m_deltaAngle.m_quad, 3) +
                                  XFNV_M128_F32(msB->m_deltaAngle.m_quad, 3)) *
                msA->m_objectRadius;
            const float tolerance = rotEps + epsB + input.m_tolerance * 0.5;
            bodyA.m_shape->getAabbImpl(bTa, tolerance, aabb);

            // The box is clipped to the sphere around bodyA's center of mass, in bodyB's space.
            const float radius = input.m_tolerance * 0.5 + msA->m_objectRadius + epsB;
            const hkQuadReal r = _mm_set1_ps(radius);
            hkVector4 d;
            d.m_quad = _mm_sub_ps(msA->m_sweptTransform.m_centerOfMass1.m_quad, msB->m_transform.m_translation.m_quad);
            hkVector4 center;
            center._setRotatedInverseDir(rotB, d);
            aabb.m_min.m_quad = _mm_max_ps(aabb.m_min.m_quad, _mm_sub_ps(center.m_quad, r));
            aabb.m_max.m_quad = _mm_min_ps(aabb.m_max.m_quad, _mm_add_ps(center.m_quad, r));
            extent = _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad);

            if (XFNV_M128_F32(msB->m_deltaAngle.m_quad, 3) > 0.0f) {
                const hkQuadReal rel = _mm_sub_ps(center.m_quad, msB->m_sweptTransform.m_centerOfMassLocal.m_quad);
                const float scale = XFNV_M128_F32(msB->m_sweptTransform.m_centerOfMass1.m_quad, 3) *
                    input.m_stepInfo.m_deltaTime;
                const hkQuadReal dA = msB->m_deltaAngle.m_quad;
                const hkQuadReal cross = _mm_sub_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(dA, dA, _MM_SHUFFLE(3, 1, 0, 2)),
                        _mm_shuffle_ps(rel, rel, _MM_SHUFFLE(3, 0, 2, 1))
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(dA, dA, _MM_SHUFFLE(3, 0, 2, 1)),
                        _mm_shuffle_ps(rel, rel, _MM_SHUFFLE(3, 1, 0, 2))
                    )
                );
                motion = _mm_add_ps(_mm_mul_ps(cross, _mm_set1_ps(scale)), linearTimInfo.m_quad);
            }
            else {
                motion = linearTimInfo.m_quad;
            }
            const hkQuadReal zero = _mm_setzero_ps();
            aabb.m_min.m_quad = _mm_add_ps(_mm_min_ps(zero, motion), aabb.m_min.m_quad);
            aabb.m_max.m_quad = _mm_add_ps(_mm_max_ps(zero, motion), aabb.m_max.m_quad);
        }
        else {
            const float tolerance = input.m_tolerance * 0.5;
            bodyA.m_shape->getAabbImpl(bTa, tolerance, aabb);
            motion = linearTimInfo.m_quad;
            extent = _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad);
        }

        if (cachedAabb) {
            if (cachedAabb->contains(aabb).m_bool) {
                goto queryDone;
            }

            // The cached box: the box grown by half the tolerance, and further, against the motion, by twice the
            // motion, at most 0.4 times the extent.
            const float halfTolerance = input.m_tolerance * 0.5;
            const hkQuadReal t = _mm_set1_ps(halfTolerance);
            const hkQuadReal minT = _mm_sub_ps(aabb.m_min.m_quad, t);
            aabb.m_max.m_quad = _mm_add_ps(t, aabb.m_max.m_quad);
            const hkQuadReal zero = _mm_setzero_ps();
            const hkQuadReal factor = _mm_set1_ps(-2.0f);
            const hkQuadReal maxGrow = _mm_mul_ps(_mm_set1_ps(0.4f), extent);
            aabb.m_min.m_quad = _mm_add_ps(
                _mm_max_ps(_mm_mul_ps(factor, _mm_max_ps(zero, motion)), _mm_xor_ps(maxGrow, hkQuadRealNegateMask)),
                minT
            );
            aabb.m_max.m_quad = _mm_add_ps(
                _mm_min_ps(_mm_mul_ps(factor, _mm_min_ps(zero, motion)), maxGrow),
                aabb.m_max.m_quad
            );
            cachedAabb->m_min.m_quad = aabb.m_min.m_quad;
            cachedAabb->m_max.m_quad = aabb.m_max.m_quad;
        }

        static_cast<const hkpBvTreeShape*>(bodyB.m_shape)->queryAabb(aabb, hitList);

        hkAgentDispatchUtilHelper helper;
        helper.m_container = bodyB.m_shape->getContainer();
        helper.m_bodyB.m_parent = &bodyB;
        helper.m_bodyB.m_motion = bodyB.m_motion;
        using DispatchUtil = hkpAgentDispatchUtil<uint32_t, hkpBvAgentEntryInfo, hkAgentDispatchUtilHelper>;
        if (m_useFastUpdate) {
            DispatchUtil::fastUpdate(
                m_collisionPartners,
                hitList,
                bodyA,
                bodyB,
                input,
                m_contactMgr,
                *constraintOwner,
                helper
            );
        }
        else {
            DispatchUtil::update(
                m_collisionPartners,
                hitList,
                bodyA,
                bodyB,
                input,
                m_contactMgr,
                *constraintOwner,
                helper
            );
        }
    }
queryDone:
    hkpBvAgentEntryInfo* partner = m_collisionPartners.m_data;
    // The end on the address, as above.
    hkpBvAgentEntryInfo* partnersEnd = reinterpret_cast<hkpBvAgentEntryInfo*>(
        reinterpret_cast<uintptr_t>(m_collisionPartners.m_data) +
        m_collisionPartners.m_size * sizeof(hkpBvAgentEntryInfo)
    );
    hkpCdBody modifiedBodyB;
    modifiedBodyB.m_parent = &bodyB;
    modifiedBodyB.m_motion = bodyB.m_motion;
    HK_TIMER_SPLIT_LIST("NarrowPhase");
    const hkpShapeContainer* container = bodyB.m_shape->getContainer();
    alignas(16) char shapeBuffer[512];
    for (; partner != partnersEnd; ++partner) {
        modifiedBodyB.m_shape = container->getChildShape(partner->m_key, shapeBuffer);
        modifiedBodyB.m_shapeKey = partner->m_key;
        partner->m_collisionAgent->processCollision(bodyA, modifiedBodyB, input, result);
    }
    HK_TIMER_END_LIST();
}

void hkpBvTreeAgent::cleanup(hkpConstraintOwner& info) {
    hkpBvTreeAgent::hkpBvAgentEntryInfo* m_data = m_collisionPartners.begin();
    for (hkpBvTreeAgent::hkpBvAgentEntryInfo* i = &m_data[m_collisionPartners.m_size]; m_data != i; ++m_data) {
        if (m_data->m_collisionAgent) {
            m_data->m_collisionAgent->cleanup(info);
        }
    }
    delete this;
}

// Re-runs the shape collection filter over every child of bodyB this agent tracks: a child the filter now enables and
// that has the null agent gets a new agent from the dispatcher, an
// enabled one with an agent passes the update on, and a disabled one has its agent cleaned up and replaced by the null
// agent.
void hkpBvTreeAgent::updateShapeCollectionFilter(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpConstraintOwner& constraintOwner
) {
    const hkpShapeContainer* shapeContainer = bodyB.m_shape->getContainer();
    hkpCdBody modifiedBodyB;
    alignas(16) char shapeBuffer[512];
    for (int32_t i = 0; i < m_collisionPartners.m_size; ++i) {
        const hkpShape* shape = shapeContainer->getChildShape(m_collisionPartners.m_data[i].m_key, shapeBuffer);
        modifiedBodyB.m_parent = &bodyB;
        modifiedBodyB.m_motion = bodyB.m_motion;
        modifiedBodyB.m_shapeKey = m_collisionPartners.m_data[i].m_key;
        modifiedBodyB.m_shape = shape;
        if (input.m_filter
                ->isCollisionEnabled(input, bodyA, bodyB, *shapeContainer, m_collisionPartners.m_data[i].m_key)
                .m_bool) {
            if (m_collisionPartners.m_data[i].m_collisionAgent == hkpNullAgent::getNullAgent()) {
                m_collisionPartners.m_data[i].m_collisionAgent = input.m_dispatcher->getNewCollisionAgent(
                    bodyA,
                    modifiedBodyB,
                    input,
                    m_contactMgr
                );
            }
            else {
                m_collisionPartners.m_data[i]
                    .m_collisionAgent->updateShapeCollectionFilter(bodyA, modifiedBodyB, input, constraintOwner);
            }
        }
        else if (m_collisionPartners.m_data[i].m_collisionAgent != hkpNullAgent::getNullAgent()) {
            m_collisionPartners.m_data[i].m_collisionAgent->cleanup(constraintOwner);
            m_collisionPartners.m_data[i].m_collisionAgent = hkpNullAgent::getNullAgent();
        }
    }
}

void hkpBvTreeAgent::invalidateTim(const hkpCollisionInput& input) {
    hkpBvTreeAgent::hkpBvAgentEntryInfo* m_data = m_collisionPartners.begin();
    for (hkpBvTreeAgent::hkpBvAgentEntryInfo* i = &m_data[m_collisionPartners.m_size]; m_data != i; ++m_data) {
        if (m_data->m_collisionAgent) {
            m_data->m_collisionAgent->invalidateTim(input);
        }
    }
}

void hkpBvTreeAgent::warpTime(float oldTime, float newTime, const hkpCollisionInput& input) {
    hkpBvTreeAgent::hkpBvAgentEntryInfo* m_data = m_collisionPartners.begin();
    for (hkpBvTreeAgent::hkpBvAgentEntryInfo* i = &m_data[m_collisionPartners.m_size]; m_data != i; ++m_data) {
        if (m_data->m_collisionAgent) {
            m_data->m_collisionAgent->warpTime(oldTime, newTime, input);
        }
    }
}

void hkpBvTreeAgent::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t m_capacityAndFlags = m_collisionPartners.m_capacityAndFlags;
    if (m_capacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(
            m_collisionPartners.m_data,
            static_cast<int32_t>(sizeof(*m_collisionPartners.m_data)) *
                (m_capacityAndFlags & hkArrayBase<hkpBvAgentEntryInfo>::CAPACITY_MASK)
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "AgentPtrs",
            m_collisionPartners.m_data,
            static_cast<int32_t>(sizeof(*m_collisionPartners.m_data)) * m_collisionPartners.m_size,
            AllocatedSize
        );
    }
    hkpBvTreeAgent::hkpBvAgentEntryInfo* m_data = m_collisionPartners.m_data;
    for (hkpBvTreeAgent::hkpBvAgentEntryInfo* i = &m_data[m_collisionPartners.m_size]; m_data != i; ++m_data) {
        hkReferencedObject* m_collisionAgent = m_data->m_collisionAgent;
        if (m_collisionAgent) {
            collector->addReferencedObject("Agent", m_collisionAgent, 0);
        }
    }
}

// Follows the same steps as processCollision's below: bodyA's box in bodyB's space, grown by one and a half times the
// tolerance, queries bodyB's tree (unless the box lies inside the cached one), and the partners are brought up to date
// with the hit keys. Nothing is updated when the heap cannot take 128 bytes for each key more than there are partners.
void hkpBvTreeAgent::prepareCollisionPartners(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpConstraintOwner& constraintOwner
) {
    alignas(16) hkTransform bTa;
    bTa.setMulInverseMul(*bodyB.m_motion.pMotion, *bodyA.m_motion.pMotion);
    hkAabb aabb;
    {
        const float halfTolerance = input.m_tolerance * 0.5;
        const float tolerance = halfTolerance + input.m_tolerance;
        bodyA.m_shape->getAabbImpl(bTa, tolerance, aabb);
    }
    if (hkpBvTreeAgent::m_useAabbCaching) {
        if (m_cachedAabb.contains(aabb).m_bool) {
            return;
        }
        m_cachedAabb.m_min.m_quad = aabb.m_min.m_quad;
        m_cachedAabb.m_max.m_quad = aabb.m_max.m_quad;
    }

    using hitList_t = hkInplaceArray<uint32_t, 128>;
    alignas(16) hitList_t hitList;
    static_cast<const hkpBvTreeShape*>(bodyB.m_shape)->queryAabb(aabb, hitList);

    const int32_t numPartners = m_collisionPartners.m_size;
    if (!hkMemoryRouter::getInstance().heap().canAllocTotal((hitList.m_size - numPartners) * 128)) {
        hkSetOutOfMemoryState(hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY);
    }
    else {
        hkAgentDispatchUtilHelper helper;
        helper.m_container = bodyB.m_shape->getContainer();
        helper.m_bodyB.m_parent = &bodyB;
        helper.m_bodyB.m_motion = bodyB.m_motion;
        using DispatchUtil = hkpAgentDispatchUtil<uint32_t, hkpBvAgentEntryInfo, hkAgentDispatchUtilHelper>;
        if (m_useFastUpdate) {
            DispatchUtil::fastUpdate(
                m_collisionPartners,
                hitList,
                bodyA,
                bodyB,
                input,
                m_contactMgr,
                constraintOwner,
                helper
            );
        }
        else {
            DispatchUtil::update(
                m_collisionPartners,
                hitList,
                bodyA,
                bodyB,
                input,
                m_contactMgr,
                constraintOwner,
                helper
            );
        }
    }
}

// Registers the flipped agent for (bounding volume tree, multi-ray), then the plain one for (multi-ray, bounding volume
// tree); both are predictive. One AgentFuncs is reused for both. registerCollisionAgent copies af's flags, so the two
// padding bytes after m_isPredictive, never written here, land in the dispatcher's table.
void hkpBvTreeAgent::registerMultiRayAgent(hkpCollisionDispatcher* dispatcher) {
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

// Registers the flipped agent for (bounding volume tree, any shape), the plain one for (any shape, bounding volume
// tree) and the bounding volume tree pair; all are predictive. One AgentFuncs is reused for all three.
// registerCollisionAgent copies f's flags, so the two padding bytes after m_isPredictive, never written here, land in
// the dispatcher's table.
void hkpBvTreeAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_createFunc = &createBvTreeShapeAgent;
    f.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticLinearCast;
    f.m_isFlipped.m_bool = 1;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_BV_TREE, HK_SHAPE_ALL);

    f.m_createFunc = &createShapeBvAgent;
    f.m_getPenetrationsFunc = &hkpBvTreeAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpBvTreeAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpBvTreeAgent::staticLinearCast;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_ALL, HK_SHAPE_BV_TREE);

    f.m_createFunc = &hkpBvTreeAgent::createBvBvAgent;
    f.m_getPenetrationsFunc = &hkpBvTreeAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpBvTreeAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpBvTreeAgent::staticLinearCast;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_BV_TREE, HK_SHAPE_BV_TREE);
}

// As staticGetClosestPoints, with the pair collector's early out tested after each child's query, between the timer
// list macros; the hit list is an hkInplaceArray of 128 keys.
void hkpBvTreeAgent::staticGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN_LIST("BvTree", "QueryTree");
    hkAabb aabb;
    staticCalcAabb(bodyA, bodyB, input, aabb);

    const hkpBvTreeShape* bvB = static_cast<const hkpBvTreeShape*>(bodyB.m_shape);
    // 16-aligned, as queryAabb receives it.
    using hitList_t = hkInplaceArray<uint32_t, 128>;
    alignas(16) hitList_t hitList;
    bvB->queryAabb(aabb, hitList);
    HK_TIMER_SPLIT_LIST("NarrowPhase");

    const uint32_t typeA = bodyA.m_shape->m_type;
    const uint32_t* itr = hitList.m_data;
    const uint32_t* end = hitList.m_data + hitList.m_size;
    hkpCdBody modifiedBodyB;
    modifiedBodyB.m_parent = &bodyB;
    modifiedBodyB.m_motion = bodyB.m_motion;
    const hkpShapeContainer* shapeContainer = bvB->getContainer();
    alignas(16) char shapeBuffer[512];
    for (; itr != end; ++itr) {
        // Calls the filter through its hkpShapeCollectionFilter base; input.m_filter is deliberately not null-checked.
        const hkpShapeCollectionFilter& filter = *input.m_filter;
        if (!filter.isCollisionEnabled(input, bodyA, bodyB, *shapeContainer, *itr).m_bool) {
            continue;
        }
        const hkpShape* shape = shapeContainer->getChildShape(*itr, shapeBuffer);
        modifiedBodyB.m_shape = shape;
        modifiedBodyB.m_shapeKey = *itr;
        const hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
        dispatcher->m_agent2Func[dispatcher->m_agent2Types[typeA][shape->m_type]]
            .m_getPenetrationsFunc(bodyA, modifiedBodyB, input, collector);
        if (collector.m_earlyOut.m_bool) {
            break;
        }
    }
    HK_TIMER_END_LIST();
}

// As staticLinearCast, with bodyA's plain box, and each child key first goes through the input's collision filter
// (its hkpShapeCollectionFilter interface).
void hkpBvTreeAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN_LIST("BvTree", "QueryTree");
    alignas(16) uint8_t aAabbStorage[sizeof(hkAabb)];
    hkAabb& aabb = *reinterpret_cast<hkAabb*>(aAabbStorage);
    staticCalcAabb(bodyA, bodyB, input, aabb);

    const hkpBvTreeShape* bvB = static_cast<const hkpBvTreeShape*>(bodyB.m_shape);
    // 16-aligned, as queryAabb receives it.
    using hitList_t = hkInplaceArray<uint32_t, 128>;
    alignas(16) hitList_t hitList;
    bvB->queryAabb(aabb, hitList);
    HK_TIMER_SPLIT_LIST("NarrowPhase");

    const uint32_t typeA = bodyA.m_shape->m_type;
    const uint32_t* itr = hitList.m_data;
    const uint32_t* end = hitList.m_data + hitList.m_size;
    hkpCdBody modifiedBodyB;
    modifiedBodyB.m_parent = &bodyB;
    modifiedBodyB.m_motion = bodyB.m_motion;
    const hkpShapeContainer* shapeContainer = bvB->getContainer();
    alignas(16) char shapeBuffer[512];
    for (; itr != end; ++itr) {
        // Calls the filter through its hkpShapeCollectionFilter base; input.m_filter is deliberately not null-checked.
        const hkpShapeCollectionFilter& filter = *input.m_filter;
        if (!filter.isCollisionEnabled(input, bodyA, bodyB, *shapeContainer, *itr).m_bool) {
            continue;
        }
        const hkpShape* shape = shapeContainer->getChildShape(*itr, shapeBuffer);
        modifiedBodyB.m_shape = shape;
        modifiedBodyB.m_shapeKey = *itr;
        const hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
        dispatcher->m_agent2Func[dispatcher->m_agent2Types[typeA][shape->m_type]]
            .m_getClosestPointFunc(bodyA, modifiedBodyB, input, collector);
    }
    HK_TIMER_END_LIST();
}

// HK_TIMER_BEGIN_LIST, HK_TIMER_SPLIT_LIST and HK_TIMER_END_LIST write into the stream's raw buffer, which is treated
// as untyped (hence the casts), as in hkpSimulation::advanceTime. The hit list is an hkInplaceArray of 128 keys; the
// box and the shape buffer are 16-aligned locals.
void hkpBvTreeAgent::staticLinearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    HK_TIMER_BEGIN_LIST("BvTree", "QueryTree");
    alignas(16) uint8_t aAabbStorage[sizeof(hkAabb)];
    hkAabb& aabb = *reinterpret_cast<hkAabb*>(aAabbStorage);
    calcAabbLinearCast(bodyA, bodyB, input, aabb);

    const hkpBvTreeShape* bvB = static_cast<const hkpBvTreeShape*>(bodyB.m_shape);
    // 16-aligned, as queryAabb receives it.
    using hitList_t = hkInplaceArray<uint32_t, 128>;
    alignas(16) hitList_t hitList;
    bvB->queryAabb(aabb, hitList);
    HK_TIMER_SPLIT_LIST("NarrowPhase");

    const uint32_t typeA = bodyA.m_shape->m_type;
    const uint32_t* itr = hitList.m_data;
    const uint32_t* end = hitList.m_data + hitList.m_size;
    hkpCdBody modifiedBodyB;
    modifiedBodyB.m_parent = &bodyB;
    modifiedBodyB.m_motion = bodyB.m_motion;
    const hkpShapeContainer* shapeContainer = bvB->getContainer();
    alignas(16) char shapeBuffer[512];
    for (; itr != end; ++itr) {
        const hkpShape* shape = shapeContainer->getChildShape(*itr, shapeBuffer);
        modifiedBodyB.m_shape = shape;
        modifiedBodyB.m_shapeKey = *itr;
        const hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
        dispatcher->m_agent2Func[dispatcher->m_agent2Types[typeA][shape->m_type]]
            .m_linearCastFunc(bodyA, modifiedBodyB, input, collector, startCollector);
    }
    HK_TIMER_END_LIST();
}

void hkpBvTreeAgent::calcAabbLinearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkAabb& aabbOut
) {
    hkVector4 v5;
    hkTransform v6;
    v6.setMulInverseMul(*bodyB.m_motion.pMotion, *bodyA.m_motion.pMotion);
    bodyA.m_shape->getAabbImpl(v6, input.m_tolerance, aabbOut);
    v5.setRotatedInverseDir(bodyB.m_motion.pState->m_transform.m_rotation, input.m_path);
    __m128 m_quad = v5.m_quad;
    aabbOut.m_min.m_quad = _mm_add_ps(_mm_min_ps(_mm_setzero_ps(), v5.m_quad), aabbOut.m_min.m_quad);
    aabbOut.m_max.m_quad = _mm_add_ps(_mm_max_ps(_mm_setzero_ps(), m_quad), aabbOut.m_max.m_quad);
}

void hkpBvTreeAgent::staticCalcAabb(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkAabb& aabbOut
) {
    hkTransform v4;
    v4.setMulInverseMul(*bodyB.m_motion.pMotion, *bodyA.m_motion.pMotion);
    bodyA.m_shape->getAabbImpl(v4, input.m_tolerance, aabbOut);
}

hkpCollisionAgent* hkpBvTreeAgent::defaultAgentCreate(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    hkpCollisionDispatcher* m_dispatcher = input.m_dispatcher;
    uint8_t* v5 = input.m_dispatcher->m_agent2TypesPred[0];
    if (!input.m_createPredictiveAgents.m_storage) {
        v5 = m_dispatcher->m_agent2Types[0];
    }
    return m_dispatcher->m_agent2Func[v5[32 * bodyA.m_shape->m_type + bodyB.m_shape->m_type]]
        .m_createFunc(bodyA, bodyB, input, mgr);
}

hkpCollisionAgent* hkpBvTreeAgent::createBvBvAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    if (!(bodyB.m_motion.pState->m_objectRadius > static_cast<double>(bodyA.m_motion.pState->m_objectRadius))) {
        return new hkpSymmetricAgent<hkpBvTreeAgent>(mgr);
    }

    return new hkpBvTreeAgent(mgr);
}

uint8_t* hkpBvTreeAgent::getUseAabbCaching(uint8_t* result) {
    *result = static_cast<uint8_t>(hkpBvTreeAgent::m_useAabbCaching.m_bool);
    return result;
}

char hkpBvTreeAgent::setUseAabbCaching(char useIt) {
    hkpBvTreeAgent::m_useAabbCaching.m_bool = useIt;
    return useIt;
}
