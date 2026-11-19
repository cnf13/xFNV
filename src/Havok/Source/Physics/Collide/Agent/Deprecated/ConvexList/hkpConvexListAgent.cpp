#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListAgent.h"

#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/List/hkpListAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/ShapeCollection/hkpShapeCollectionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGsk.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskManifold.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskfAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpPredGskfAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListAgentPenetrations.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListUtils.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/ConvexList3/hkpConvexListAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/hkpAgentMachineUtil.h"
#include "Havok/Source/Physics/Collide/Agent3/PredGskAgent3/hkpPredGskAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3Input.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpConvexListFilter.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpFlagCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpClosestCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpSimpleClosestContactCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexList/hkpConvexListShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include <x86intrin.h>

using TimerName11 = const char[11];
static TimerName11 s_szChildrenTimer{"Stchildren"};
using TimerName12 = const char[12];
static TimerName12 s_szCheckHullTimer{"StcheckHull"};
using TimerName10 = const char[10];
static TimerName10 s_szCvxListTimer{"LtCvxList"};

// Timer names that this file's HK_TIMER macros store. Each file that uses "lt" defines its own copy.
using TimerName3 = const char[3];
static TimerName3 s_szEndListTimer{"lt"};

// The SDK's constructor: hkpPredGskfAgent's, the input's dispatcher, GSK mode, and the GSK cache for the bodies'
// relative transform with no separating normal yet.
hkpConvexListAgent::hkpConvexListAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
)
  : hkpPredGskfAgent(bodyA, bodyB, mgr), m_dispatcher(input.m_dispatcher), m_inGskMode(true),
    m_processFunctionCalled(false) {
    hkTransform btoa;
    btoa.setMulInverseMul(*bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion);
    m_cache.init(
        static_cast<const hkpConvexShape*>(bodyA.m_shape), static_cast<const hkpConvexShape*>(bodyB.m_shape), btoa
    );
    m_separatingNormal.m_quad[3] = -1.0f;
    m_timeOfSeparatingNormal = -1.0f;
}

// In GSK mode the manifold holds the contacts and there are no child agents to report; in stream mode the track's
// agents report themselves, with the collector's client data. The class argument is not used.
void hkpConvexListAgent::calcContentStatistics(hkStatisticsCollector* collector, const hkClass*) const {
    if (m_inGskMode.m_bool) {
        return;
    }
    hkpAgentMachineUtil::calc1nStatistics(const_cast<hkpGskManifold*>(&m_manifold), collector->m_clientData, collector);
}

// The query does not use the agent's state: the static version does the work.
void hkpConvexListAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    staticGetPenetrations(bodyA, bodyB, input, collector);
}

// The query does not use the agent's state: the static version does the work.
void hkpConvexListAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    staticGetClosestPoints(bodyA, bodyB, input, collector);
}

void hkpConvexListAgent::linearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    hkpConvexListAgent::staticLinearCast(bodyA, bodyB, input, collector, startCollector);
}

// In stream mode an hkpAgent3ProcessInput is filled without its m_distAtT1 or the padding before m_aTb, and
// hkAgent1nMachine_Process copies the whole struct, an uninitialised read kept deliberately as the game does (as in
// hkpListAgent::processCollision). The agent collides the list's hull as a predictive GSK agent while the hull is apart
// from the other shape, through a contact manager that maps each point to the child it belongs to; when a point spans
// several children, or the hull penetrates deeper than twice the bodies' allowed penetration, its output is rolled back
// and a stream of child agents takes over. In stream mode the hull is checked again every 26 frames, and GSK mode
// returns once it is apart.
void hkpConvexListAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN_LIST("CvxLst", "Tim");

    hkVector4 timInfo;
    {
        const hkMotionState* msA = bodyA.m_motion.pState;
        const hkMotionState* msB = bodyB.m_motion.pState;
        // EXE-INLINED none: hkSweptTransformUtil::calcTimInfo
        float deltaTime = input.m_stepInfo.m_deltaTime;
        float f0 = msA->m_sweptTransform.m_centerOfMass1.m_quad[3] * deltaTime;
        float f1 = deltaTime * msB->m_sweptTransform.m_centerOfMass1.m_quad[3];
        __m128 diff0 =
            _mm_sub_ps(msA->m_sweptTransform.m_centerOfMass0.m_quad, msA->m_sweptTransform.m_centerOfMass1.m_quad);
        __m128 diff1 =
            _mm_sub_ps(msB->m_sweptTransform.m_centerOfMass1.m_quad, msB->m_sweptTransform.m_centerOfMass0.m_quad);
        timInfo.m_quad = _mm_add_ps(_mm_mul_ps(diff1, _mm_set1_ps(f1)), _mm_mul_ps(diff0, _mm_set1_ps(f0)));
        timInfo.m_quad[3] = msA->m_deltaAngle.m_quad[3] * f0 * msA->m_objectRadius +
            msB->m_deltaAngle.m_quad[3] * f1 * msB->m_objectRadius;
    }

    hkpProcessCollisionOutputBackup resultBackup(result);

    if (m_inGskMode.m_bool) {
        goto gskMode;
    }
    for (;;) {
        HK_TIMER_SPLIT_LIST("Stream");
        {
            int16_t counter = m_inStreamModeCounter;
            m_inStreamModeCounter = static_cast<int16_t>(counter - 1);
            if (counter >= 0) {
                break;
            }
        }
        {
            // Time to check the hull: while it stays apart from the other shape, the stream goes on with the hull's
            // distance as its time distance.
            m_inStreamModeCounter = 25;
            hkTransform aTb;
            aTb.setMulInverseMul(*bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion);
            hkpGsk::GetClosesetPointInput gskInput;
            gskInput.m_shapeA.m_storage = static_cast<const hkpConvexShape*>(bodyA.m_shape);
            gskInput.m_aTb.m_storage = &aTb;
            gskInput.m_shapeB.m_storage = static_cast<const hkpConvexShape*>(bodyB.m_shape);
            gskInput.m_collisionTolerance.m_storage = input.m_tolerance;
            gskInput.m_transformA.m_storage = bodyA.m_motion.pMotion;
            hkVector4 pointOnB;
            if (hkpGsk::getClosestPoint(gskInput, m_cache, m_separatingNormal, pointOnB) == HK_SUCCESS) {
                getStream().m_inStreamModeTimDist = -m_separatingNormal.m_quad[3];
                break;
            }
        }
        switchToGskMode(*result.m_constraintOwner.m_storage);
        resultBackup.rollbackOutput(bodyA, bodyB, result, m_contactMgr);

    gskMode:
        // The separating plane moves by the bodies' motion over the step: while the hull stays apart past the
        // tolerance, the manifold's points go and nothing is collided.
        if (m_separatingNormal.m_quad[3] > input.m_tolerance) {
            // EXE-INLINED none: hkVector4::dot4xyz1
            __m128 x2 = _mm_mul_ps(m_separatingNormal.m_quad, timInfo.m_quad);
            __m128 xySum = _mm_add_ss(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(1, 1, 1, 1)), x2);
            __m128 zwSum = _mm_add_ss(
                _mm_shuffle_ps(x2, x2, _MM_SHUFFLE(2, 2, 2, 2)),
                _mm_shuffle_ps(timInfo.m_quad, timInfo.m_quad, _MM_SHUFFLE(3, 3, 3, 3))
            );
            float fDistChange = _mm_cvtss_f32(_mm_add_ss(zwSum, xySum));
            m_separatingNormal.m_quad[3] -= fDistChange;
            if (m_separatingNormal.m_quad[3] > input.m_tolerance) {
                if (m_manifold.m_numContactPoints) {
                    hkGskManifold_cleanup(m_manifold, m_contactMgr, *result.m_constraintOwner.m_storage);
                }
                goto end;
            }
        }
        HK_TIMER_SPLIT_LIST("Gsk");
        {
            hkpMapPointsToSubShapeContactMgr mgr;
            // EXE-INLINED none: hkpMapPointsToSubShapeContactMgr::hkpMapPointsToSubShapeContactMgr
            mgr.m_contactMgr = m_contactMgr;
            mgr.m_type = hkpContactMgr::TYPE_CONVEX_LIST_CONTACT_MGR;
            mgr.m_invalidPointHit.m_bool = false;
            m_contactMgr = &mgr;
            hkpPredGskfAgent::processCollision(bodyA, bodyB, input, result);
            m_contactMgr = mgr.m_contactMgr;
            if (!mgr.m_invalidPointHit.m_bool) {
                if (!m_manifold.m_numContactPoints || m_processFunctionCalled.m_bool) {
                    goto end;
                }
                // The smaller allowed penetration depth of the two root collidables.
                float allowedB = bodyB.getRootCollidable()->getAllowedPenetrationDepth();
                float allowedA = bodyA.getRootCollidable()->getAllowedPenetrationDepth();
                float allowedPenetration = allowedA < allowedB ? allowedA : allowedB;
                float twiceAllowed = allowedPenetration + allowedPenetration;
                if (!(m_separatingNormal.m_quad[3] < -twiceAllowed)) {
                    goto end;
                }
            }
        }
        resultBackup.rollbackOutput(bodyA, bodyB, result, m_contactMgr);
        switchToStreamMode(*result.m_constraintOwner.m_storage);
    }

    {
        StreamData& stream = getStream();
        // EXE-INLINED none: hkVector4::length3
        __m128 len2 = _mm_mul_ps(timInfo.m_quad, timInfo.m_quad);
        __m128 lenSum = _mm_add_ss(
            _mm_shuffle_ps(len2, len2, _MM_SHUFFLE(2, 2, 2, 2)),
            _mm_add_ss(_mm_shuffle_ps(len2, len2, _MM_SHUFFLE(1, 1, 1, 1)), len2)
        );
        stream.m_inStreamModeTimDist -= _mm_cvtss_f32(_mm_sqrt_ss(lenSum));

        hkpAgent3ProcessInput in3;
        in3.m_overrideBodyA = nullptr;
        in3.m_contactMgr = m_contactMgr;
        in3.m_bodyA = &bodyA;
        in3.m_bodyB = &bodyB;
        in3.m_input = &input;
        in3.m_linearTimInfo.m_quad = timInfo.m_quad;
        in3.m_aTb.setMulInverseMul(*bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion);

        const hkpConvexListShape* listShape = static_cast<const hkpConvexListShape*>(bodyB.m_shape);
        int32_t numChildren = listShape->m_childShapes.m_size;
        uint32_t* hitList = hkAllocateStack<uint32_t>(numChildren + 1, nullptr);
        for (int32_t i = 0; i < numChildren; i++) {
            hitList[i] = static_cast<uint32_t>(i);
        }
        hitList[numChildren] = HK_INVALID_SHAPE_KEY;
        hkAgent1nMachine_Process(stream.m_agentTrack, in3, listShape, hitList, result);
        hkDeallocateStack(hitList);
    }

end:
    m_processFunctionCalled.m_bool = true;
    HK_TIMER_END_LIST();
}

// Drops the manifold's (or the stream's) contacts and then deletes the agent itself.
void hkpConvexListAgent::cleanup(hkpConstraintOwner& constraintOwner) {
    if (m_inGskMode.m_bool) {
        hkGskManifold_cleanup(m_manifold, m_contactMgr, constraintOwner);
    }
    else {
        hkAgent1nMachine_Destroy(
            *reinterpret_cast<hkpAgent1nTrack*>(&m_manifold), m_dispatcher, m_contactMgr, constraintOwner
        );
    }
    delete this;
}

// In stream mode the child agents' track is visited with the new filter; in GSK mode there is nothing to filter.
void hkpConvexListAgent::updateShapeCollectionFilter(
    const hkpCdBody& bodyA,
    const hkpCdBody& listShapeBodyB,
    const hkpCollisionInput& input,
    hkpConstraintOwner& constraintOwner
) {
    if (!m_inGskMode.m_bool) {
        hkpAgent1nMachine_VisitorInput vin;
        hkpContactMgr* pMgr = m_contactMgr;
        vin.m_input = &input;
        vin.m_bodyA = &bodyA;
        vin.m_constraintOwner = &constraintOwner;
        vin.m_collectionBodyB = &listShapeBodyB;
        vin.m_contactMgr = pMgr;
        vin.m_containerShapeB = listShapeBodyB.m_shape->getContainer();
        hkAgent1nMachine_UpdateShapeCollectionFilter(getStream().m_agentTrack, vin);
    }
}

// In GSK mode the cached separating normal's time goes; in stream mode the child agents' times do.
void hkpConvexListAgent::invalidateTim(const hkpCollisionInput& input) {
    if (m_inGskMode.m_bool) {
        hkpGskBaseAgent::invalidateTim(input);
    }
    else {
        hkAgent1nMachine_InvalidateTim(getStream().m_agentTrack, input);
    }
}

// In GSK mode the cached separating normal's time moves; in stream mode the child agents' times do.
void hkpConvexListAgent::warpTime(float oldTime, float newTime, const hkpCollisionInput& input) {
    if (m_inGskMode.m_bool) {
        hkpGskBaseAgent::warpTime(oldTime, newTime, input);
    }
    else {
        hkAgent1nMachine_WarpTime(*reinterpret_cast<hkpAgent1nTrack*>(&m_manifold), oldTime, newTime, input);
    }
}

void hkpConvexListAgent::removePoint(uint16_t idToRemove) {
    if (m_inGskMode.m_bool) {
        hkpGskfAgent::removePoint(idToRemove);
    }
}

// In GSK mode the manifold holds the contacts, so the base's copy gives the first free point the new id; the stream's
// child agents have none to give.
void hkpConvexListAgent::commitPotential(uint16_t idToCommit) {
    if (m_inGskMode.m_bool) {
        hkpGskfAgent::commitPotential(idToCommit);
    }
}

// As commitPotential: only the GSK manifold keeps points that can become zombies.
void hkpConvexListAgent::createZombie(uint16_t idTobecomeZombie) {
    if (m_inGskMode.m_bool) {
        hkpGskfAgent::createZombie(idTobecomeZombie);
    }
}

// The stream's child agents go away and the manifold starts empty before the GSK agent takes over.
void hkpConvexListAgent::switchToGskMode(hkpConstraintOwner& constraintOwner) {
    hkAgent1nMachine_Destroy(getStream().m_agentTrack, m_dispatcher, m_contactMgr, constraintOwner);
    m_manifold.init();
    m_inGskMode.m_bool = true;
}

// The manifold's contacts go and its storage becomes the stream's: an empty track started with one sector, the stream
// mode's time distance, and the stream mode's step counter.
void hkpConvexListAgent::switchToStreamMode(hkpConstraintOwner& constraintOwner) {
    hkGskManifold_cleanup(m_manifold, m_contactMgr, constraintOwner);
    m_inGskMode.m_bool = false;
    hkpAgent1nTrack* pTrack = &getStream().m_agentTrack;
    // EXE-INLINED none: hkpAgent1nTrack::hkpAgent1nTrack
    if (pTrack != nullptr) {
        pTrack->m_sectors.m_data = nullptr;
        pTrack->m_sectors.m_size = 0;
        pTrack->m_sectors.m_capacityAndFlags = hkArrayBase<hkpAgent1nSector*>::DONT_DEALLOCATE_FLAG;
    }
    hkAgent1nMachine_Create(getStream().m_agentTrack);
    getStream().m_inStreamModeTimDist = 0.0f;
    m_inStreamModeCounter = 25;
}

// Checks the list's hull first: when the hull penetrates, or the closest hull point is not nearer than the shape's
// m_minDistanceToUseConvexHullForGetClosestPoints, the children are queried by the list agent instead. The collectors'
// destructors are empty and are not called. The reinterpret_casts are on the monitor stream's untyped buffer.
void hkpConvexListAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    // EXE-INLINED none: HK_TIMER_BEGIN_LIST
    hkMonitorStream* pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
    if (pStream->m_end < pStream->m_capacityMinus16) {
        auto* pBegin = reinterpret_cast<hkMonitorStream::TimerBeginListCommand*>(pStream->m_end);
        pBegin->m_commandAndMonitor = s_szCvxListTimer;
        pBegin->m_nameOfFirstSplit = s_szCheckHullTimer;
        pBegin->m_time0 = static_cast<uint32_t>(__rdtsc());
        pStream->m_end = reinterpret_cast<char*>(pBegin + 1);
    }

    hkpFlagCdBodyPairCollector checker;
    hkpGskBaseAgent::staticGetPenetrations(bodyA, bodyB, input, checker);

    bool bQueryChildren;
    // EXE-INLINED none: hkpFlagCdBodyPairCollector::hasHit
    if (checker.m_earlyOut.m_bool) {
        bQueryChildren = true;
    }
    else {
        hkpClosestCdPointCollector closest;
        hkpGskBaseAgent::staticGetClosestPoints(bodyA, bodyB, input, closest);

        // EXE-INLINED none: hkpClosestCdPointCollector::hasHit
        if (closest.m_hitPoint.m_rootCollidableA == nullptr) {
            bQueryChildren = false;
        }
        else if (
            static_cast<const hkpConvexListShape*>(bodyB.m_shape)->m_minDistanceToUseConvexHullForGetClosestPoints <
            closest.m_hitPoint.m_contact.m_separatingNormal.m_quad[3]
        ) {
            hkpCdPoint event;
            event.m_contact = closest.m_hitPoint.m_contact;
            event.m_cdBodyA = &bodyA;
            event.m_cdBodyB = &bodyB;
            collector.addCdPoint(event);
            bQueryChildren = false;
        }
        else {
            bQueryChildren = true;
        }
    }

    if (bQueryChildren) {
        // EXE-INLINED none: HK_TIMER_SPLIT_LIST
        pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
        if (pStream->m_end < pStream->m_capacityMinus16) {
            auto* pSplit = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
            pSplit->m_commandAndMonitor = s_szChildrenTimer;
            pSplit->m_time0 = static_cast<uint32_t>(__rdtsc());
            pStream->m_end = reinterpret_cast<char*>(pSplit + 1);
        }
        hkpSymmetricAgentLinearCast<hkpShapeCollectionAgent>::staticGetClosestPoints(bodyA, bodyB, input, collector);
    }

    // EXE-INLINED none: HK_TIMER_END_LIST
    pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
    if (pStream->m_end < pStream->m_capacityMinus16) {
        auto* pEnd = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
        pEnd->m_commandAndMonitor = s_szEndListTimer;
        pEnd->m_time0 = static_cast<uint32_t>(__rdtsc());
        pStream->m_end = reinterpret_cast<char*>(pEnd + 1);
    }
}

// The hull is checked first with a closest-contact collector; the child shapes are cast only when the hull is hit.
void hkpConvexListAgent::staticLinearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    HK_TIMER_BEGIN_LIST("CvsListAgent", "checkHull");
    // EXE-INLINED none: hkpSimpleClosestContactCollector::hkpSimpleClosestContactCollector
    hkpSimpleClosestContactCollector checkHullCollector;
    hkpIterativeLinearCastAgent::staticLinearCast(bodyA, bodyB, input, checkHullCollector, &checkHullCollector);
    if (checkHullCollector.m_hasHit.m_bool) {
        HK_TIMER_SPLIT_LIST("child");
        hkpSymmetricAgentLinearCast<hkpShapeCollectionAgent>::staticLinearCast(
            bodyA, bodyB, input, collector, startCollector
        );
    }
    HK_TIMER_END_LIST();
}

// Checks the list's hull first: only when the hull penetrates are the children queried, through the list agent's
// function. The collector's destructor is empty.
void hkpConvexListAgent::staticGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN_LIST("CvxList", "checkHull");
    hkpFlagCdBodyPairCollector checker;
    hkpGskBaseAgent::staticGetPenetrations(bodyA, bodyB, input, checker);
    // EXE-INLINED none: hkpFlagCdBodyPairCollector::hasHit
    if (checker.m_earlyOut.m_bool) {
        HK_TIMER_SPLIT_LIST("children");
        staticGetPenetrationsFunc(bodyA, bodyB, input, collector);
    }
    HK_TIMER_END_LIST();
}

// Registers the agent for a convex list against a convex shape, the other way round and against another convex list;
// one AgentFuncs is filled three times. registerCollisionAgent copies f's flags, so the two padding bytes after
// m_isPredictive, never written here, land in the dispatcher's table, as the game does.
void hkpConvexListAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_createFunc = &hkpConvexListAgent::createConvexListConvexAgent;
    f.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpConvexListAgent>::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpConvexListAgent>::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpConvexListAgent>::staticLinearCast;
    f.m_isFlipped.m_bool = 1;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_CONVEX_LIST, HK_SHAPE_CONVEX);
    f.m_createFunc = &hkpConvexListAgent::createConvexConvexListAgent;
    f.m_getPenetrationsFunc = &hkpConvexListAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpConvexListAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpConvexListAgent::staticLinearCast;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_CONVEX, HK_SHAPE_CONVEX_LIST);
    f.m_createFunc = &hkpConvexListAgent::createConvexListConvexListAgent;
    f.m_getPenetrationsFunc = &hkpConvexListAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpConvexListAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpConvexListAgent::staticLinearCast;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_CONVEX_LIST, HK_SHAPE_CONVEX_LIST);
}

// The SDK's create for a convex A against a convex list B: the filter is asked with the list first (B, A); a
// convex list agent, a shape collection agent with the bodies swapped (hkpSymmetricAgent<hkpShapeCollectionAgent>)
// or a predictive GSK agent. Without a contact manager it is the swapped shape collection agent.
hkpCollisionAgent* hkpConvexListAgent::createConvexConvexListAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    if (!mgr) {
        auto* agent = static_cast<hkpSymmetricAgent<hkpShapeCollectionAgent>*>(
            hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpShapeCollectionAgent>))
        );
        agent->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpShapeCollectionAgent>);
        return ::new (agent) hkpSymmetricAgent<hkpShapeCollectionAgent>(bodyA, bodyB, input, nullptr);
    }
    switch (input.m_convexListFilter->getConvexListCollisionType(bodyB, bodyA, input)) {
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_NORMAL: {
            hkpConvexListAgent* agent = static_cast<hkpConvexListAgent*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpConvexListAgent))
            );
            agent->m_memSizeAndFlags = sizeof(hkpConvexListAgent);
            return ::new (agent) hkpConvexListAgent(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_LIST: {
            auto* agent = static_cast<hkpSymmetricAgent<hkpShapeCollectionAgent>*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpShapeCollectionAgent>))
            );
            agent->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpShapeCollectionAgent>);
            return ::new (agent) hkpSymmetricAgent<hkpShapeCollectionAgent>(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_CONVEX: {
            hkpPredGskfAgent* agent = static_cast<hkpPredGskfAgent*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpPredGskfAgent))
            );
            agent->m_memSizeAndFlags = sizeof(hkpPredGskfAgent);
            return ::new (agent) hkpPredGskfAgent(bodyA, bodyB, mgr);
        }
        default: return nullptr;
    }
}

// The SDK's create for a convex list A against a convex B: the convex list filter picks a predictive GSK agent
// (hkpGskfAgent's constructor, then hkpPredGskfAgent's vtable), a shape collection agent, or a convex list agent
// with the bodies swapped (hkpSymmetricAgent<hkpConvexListAgent>: its constructor, then the symmetric vtable).
// Without a contact manager it is always the shape collection agent. Each allocation stores its size, as Havok's
// class operator new does.
hkpCollisionAgent* hkpConvexListAgent::createConvexListConvexAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    if (!mgr) {
        hkpShapeCollectionAgent* agent = static_cast<hkpShapeCollectionAgent*>(
            hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpShapeCollectionAgent))
        );
        agent->m_memSizeAndFlags = sizeof(hkpShapeCollectionAgent);
        return ::new (agent) hkpShapeCollectionAgent(bodyA, bodyB, input, nullptr);
    }
    switch (input.m_convexListFilter->getConvexListCollisionType(bodyA, bodyB, input)) {
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_NORMAL: {
            auto* agent = static_cast<hkpSymmetricAgent<hkpConvexListAgent>*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpConvexListAgent>))
            );
            agent->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpConvexListAgent>);
            return ::new (agent) hkpSymmetricAgent<hkpConvexListAgent>(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_LIST: {
            hkpShapeCollectionAgent* agent = static_cast<hkpShapeCollectionAgent*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpShapeCollectionAgent))
            );
            agent->m_memSizeAndFlags = sizeof(hkpShapeCollectionAgent);
            return ::new (agent) hkpShapeCollectionAgent(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_CONVEX: {
            hkpPredGskfAgent* agent = static_cast<hkpPredGskfAgent*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpPredGskfAgent))
            );
            agent->m_memSizeAndFlags = sizeof(hkpPredGskfAgent);
            return ::new (agent) hkpPredGskfAgent(bodyA, bodyB, mgr);
        }
        default: return nullptr;
    }
}

// The SDK's create for two convex lists: the filter on (A, B) first; where it treats A as a convex, the filter on
// (B, A) decides as in createConvexConvexListAgent. Without a contact manager it is the shape collection agent.
hkpCollisionAgent* hkpConvexListAgent::createConvexListConvexListAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    if (!mgr) {
        hkpShapeCollectionAgent* agent = static_cast<hkpShapeCollectionAgent*>(
            hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpShapeCollectionAgent))
        );
        agent->m_memSizeAndFlags = sizeof(hkpShapeCollectionAgent);
        return ::new (agent) hkpShapeCollectionAgent(bodyA, bodyB, input, nullptr);
    }
    switch (input.m_convexListFilter->getConvexListCollisionType(bodyA, bodyB, input)) {
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_NORMAL: {
            auto* agent = static_cast<hkpSymmetricAgent<hkpConvexListAgent>*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpConvexListAgent>))
            );
            agent->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpConvexListAgent>);
            return ::new (agent) hkpSymmetricAgent<hkpConvexListAgent>(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_LIST: {
            hkpShapeCollectionAgent* agent = static_cast<hkpShapeCollectionAgent*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpShapeCollectionAgent))
            );
            agent->m_memSizeAndFlags = sizeof(hkpShapeCollectionAgent);
            return ::new (agent) hkpShapeCollectionAgent(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_CONVEX: break;
        default: return nullptr;
    }
    switch (input.m_convexListFilter->getConvexListCollisionType(bodyB, bodyA, input)) {
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_NORMAL: {
            hkpConvexListAgent* agent = static_cast<hkpConvexListAgent*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpConvexListAgent))
            );
            agent->m_memSizeAndFlags = sizeof(hkpConvexListAgent);
            return ::new (agent) hkpConvexListAgent(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_LIST: {
            auto* agent = static_cast<hkpSymmetricAgent<hkpShapeCollectionAgent>*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpShapeCollectionAgent>))
            );
            agent->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpShapeCollectionAgent>);
            return ::new (agent) hkpSymmetricAgent<hkpShapeCollectionAgent>(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_CONVEX: {
            hkpPredGskfAgent* agent = static_cast<hkpPredGskfAgent*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpPredGskfAgent))
            );
            agent->m_memSizeAndFlags = sizeof(hkpPredGskfAgent);
            return ::new (agent) hkpPredGskfAgent(bodyA, bodyB, mgr);
        }
        default: return nullptr;
    }
}

void staticGetPenetrationsFunc(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    hkpSymmetricAgentFlipBodyCollector a4(collector);
    hkpShapeCollectionAgent::staticGetPenetrations(bodyB, bodyA, input, a4);
}

void hkpCollectionCollectionAgent3::calcStatistics(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    const hkpCollisionInput& input,
    hkStatisticsCollector* collector
) {
    collector->pushDir("hkpCollectionCollectionAgent3");
    hkpAgentMachineUtil::calc1nStatistics(agentData, &input, collector);
    collector->popDir();
}

void hkpCollectionCollectionAgent3::invalidateTim(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    const hkpCollisionInput& input
) {
    hkAgent1nMachine_InvalidateTim(*static_cast<hkpAgent1nTrack*>(agentData), input);
}

// The agent3 table's warp time for this agent: the entry is not used, the agent data is the track.
void hkpCollectionCollectionAgent3::warpTime(
    hkpAgentEntry*,
    hkpAgentData* agentData,
    float oldTime,
    float newTime,
    const hkpCollisionInput& input
) {
    hkAgent1nMachine_WarpTime(*static_cast<hkpAgent1nTrack*>(agentData), oldTime, newTime, input);
}

hkpAgentData* hkConvexListAgent3::create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData) {
    void* result = hkPredGskAgent3::create(input, entry, agentData);
    *(static_cast<uint8_t*>(agentData) + 11) = (*(static_cast<uint8_t*>(agentData) + 11) & 159) | 32;
    return result;
}

// The agent3 table's destroy for the flipped agent: a flipped entry is a predictive GSK agent's, so its destroy runs on
// the entry itself; otherwise the stream's own cleanup runs on the data behind the entry. The entry parameter is
// ignored and agentData is read as the agent's padded 1n entry.
void hkConvexListAgent3::destroy(
    hkpAgentEntry*,
    hkpAgentData* agentData,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner,
    hkpCollisionDispatcher* dispatcher
) {
    hkpAgent1nMachinePaddedEntry* pEntry = static_cast<hkpAgent1nMachinePaddedEntry*>(agentData);
    if (pEntry->m_shapeKeyPair.m_shapeKeyB & 0x20000000u) {
        hkPredGskAgent3::destroy(pEntry, pEntry, mgr, constraintOwner, dispatcher);
    }
    else {
        hkAgent1nMachine_Destroy(*reinterpret_cast<hkpAgent1nTrack*>(pEntry + 1), dispatcher, mgr, constraintOwner);
    }
}

// The agent3 table's warp time for the flipped agent: an entry whose second shape key carries the flipped flag (bit 29
// of the key) has no times of its own. agentData is read as the agent's padded 1n entry; entry is not used.
void hkConvexListAgent3::warpTime(
    hkpAgentEntry*,
    hkpAgentData* agentData,
    float oldTime,
    float newTime,
    const hkpCollisionInput& input
) {
    hkpAgent1nMachinePaddedEntry* pEntry = static_cast<hkpAgent1nMachinePaddedEntry*>(agentData);
    if ((pEntry->m_shapeKeyPair.m_shapeKeyB & 0x20000000u) == 0) {
        hkAgent1nMachine_WarpTime(*reinterpret_cast<hkpAgent1nTrack*>(pEntry + 1), oldTime, newTime, input);
    }
}

void hkConvexListAgent3::initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f) {
    f.m_createFunc = &hkConvexListAgent3::create;
    f.m_processFunc = &hkConvexListAgent3::process;
    f.m_sepNormalFunc = &hkConvexListAgent3::sepNormal;
    f.m_cleanupFunc = &hkConvexListAgent3::cleanup;
    f.m_removePointFunc = &hkConvexListAgent3::removePoint;
    f.m_commitPotentialFunc = &hkConvexListAgent3::commitPotential;
    f.m_createZombieFunc = &hkConvexListAgent3::createZombie;
    f.m_updateFilterFunc = &hkConvexListAgent3::updateFilter;
    f.m_calcStatisticsFunc = &hkConvexListAgent3::calcStatistics;
    f.m_invalidateTimFunc = &hkConvexListAgent3::invalidateTim;
    f.m_warpTimeFunc = &hkConvexListAgent3::warpTime;
    f.m_destroyFunc = &hkConvexListAgent3::destroy;
    f.m_isPredictive.m_bool = 1;
}

hkpFlagCdBodyPairCollector::~hkpFlagCdBodyPairCollector() {}

void hkpFlagCdBodyPairCollector::addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB) {
    m_earlyOut.m_bool = 1;
}

void hkpFlagCdBodyPairCollector::operator delete(void* apMemory) {
    if (apMemory) {
        hkMemoryRouter::getInstance().heap().blockFree(apMemory, sizeof(hkpFlagCdBodyPairCollector));
    }
}

hkpSimpleClosestContactCollector::~hkpSimpleClosestContactCollector() {}

void hkpSimpleClosestContactCollector::operator delete(void* apMemory) {
    if (apMemory) {
        hkMemoryRouter::getInstance().heap().blockFree(apMemory, sizeof(hkpSimpleClosestContactCollector));
    }
}
