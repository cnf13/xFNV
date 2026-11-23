#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListAgent.h"

#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
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
#include "xfnv/Compiler.h"

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
        static_cast<const hkpConvexShape*>(bodyA.m_shape),
        static_cast<const hkpConvexShape*>(bodyB.m_shape),
        btoa
    );
    XFNV_M128_F32(m_separatingNormal.m_quad, 3) = -1.0f;
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
// hkAgent1nMachine_Process copies the whole struct, an uninitialised read kept deliberately (as in
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
        hkSweptTransformUtil::calcTimInfo(*msA, *msB, input.m_stepInfo.m_deltaTime, timInfo);
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
                getStream().m_inStreamModeTimDist = -XFNV_M128_F32(m_separatingNormal.m_quad, 3);
                break;
            }
        }
        switchToGskMode(*result.m_constraintOwner.m_storage);
        resultBackup.rollbackOutput(bodyA, bodyB, result, m_contactMgr);

    gskMode:
        // The separating plane moves by the bodies' motion over the step: while the hull stays apart past the
        // tolerance, the manifold's points go and nothing is collided.
        if (XFNV_M128_F32(m_separatingNormal.m_quad, 3) > input.m_tolerance) {
            float fDistChange = timInfo.dot4xyz1(m_separatingNormal);
            XFNV_M128_F32(m_separatingNormal.m_quad, 3) -= fDistChange;
            if (XFNV_M128_F32(m_separatingNormal.m_quad, 3) > input.m_tolerance) {
                if (m_manifold.m_numContactPoints) {
                    hkGskManifold_cleanup(m_manifold, m_contactMgr, *result.m_constraintOwner.m_storage);
                }
                goto end;
            }
        }
        HK_TIMER_SPLIT_LIST("Gsk");
        {
            hkpMapPointsToSubShapeContactMgr mgr(m_contactMgr);
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
                if (!(XFNV_M128_F32(m_separatingNormal.m_quad, 3) < -twiceAllowed)) {
                    goto end;
                }
            }
        }
        resultBackup.rollbackOutput(bodyA, bodyB, result, m_contactMgr);
        switchToStreamMode(*result.m_constraintOwner.m_storage);
    }

    {
        StreamData& stream = getStream();
        stream.m_inStreamModeTimDist -= timInfo.length3();

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
            *reinterpret_cast<hkpAgent1nTrack*>(&m_manifold),
            m_dispatcher,
            m_contactMgr,
            constraintOwner
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
    ::new (pTrack) hkpAgent1nTrack;
    hkAgent1nMachine_Create(getStream().m_agentTrack);
    getStream().m_inStreamModeTimDist = 0.0f;
    m_inStreamModeCounter = 25;
}

// Checks the list's hull first: when the hull penetrates, or the closest hull point is not nearer than the shape's
// m_minDistanceToUseConvexHullForGetClosestPoints, the children are queried by the list agent instead. The collectors'
// destructors are empty and are not called.
void hkpConvexListAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN_LIST("CvxList", "checkHull");

    hkpFlagCdBodyPairCollector checker;
    hkpGskBaseAgent::staticGetPenetrations(bodyA, bodyB, input, checker);

    bool bQueryChildren;
    if (checker.hasHit()) {
        bQueryChildren = true;
    }
    else {
        hkpClosestCdPointCollector closest;
        hkpGskBaseAgent::staticGetClosestPoints(bodyA, bodyB, input, closest);

        if (!closest.hasHit()) {
            bQueryChildren = false;
        }
        else if (
            static_cast<const hkpConvexListShape*>(bodyB.m_shape)->m_minDistanceToUseConvexHullForGetClosestPoints <
            XFNV_M128_F32(closest.m_hitPoint.m_contact.m_separatingNormal.m_quad, 3)
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
        HK_TIMER_SPLIT_LIST("children");
        hkpSymmetricAgentLinearCast<hkpShapeCollectionAgent>::staticGetClosestPoints(bodyA, bodyB, input, collector);
    }

    HK_TIMER_END_LIST();
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
    hkpSimpleClosestContactCollector checkHullCollector;
    hkpIterativeLinearCastAgent::staticLinearCast(bodyA, bodyB, input, checkHullCollector, &checkHullCollector);
    if (checkHullCollector.m_hasHit.m_bool) {
        HK_TIMER_SPLIT_LIST("child");
        hkpSymmetricAgentLinearCast<hkpShapeCollectionAgent>::staticLinearCast(
            bodyA,
            bodyB,
            input,
            collector,
            startCollector
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
    if (checker.hasHit()) {
        HK_TIMER_SPLIT_LIST("children");
        staticGetPenetrationsFunc(bodyA, bodyB, input, collector);
    }
    HK_TIMER_END_LIST();
}

// Registers the agent for a convex list against a convex shape, the other way round and against another convex list;
// one AgentFuncs is filled three times. registerCollisionAgent copies f's flags, so the two padding bytes after
// m_isPredictive, never written here, land in the dispatcher's table.
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
        return new hkpSymmetricAgent<hkpShapeCollectionAgent>(bodyA, bodyB, input, nullptr);
    }
    switch (input.m_convexListFilter->getConvexListCollisionType(bodyB, bodyA, input)) {
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_NORMAL: {
            return new hkpConvexListAgent(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_LIST: {
            return new hkpSymmetricAgent<hkpShapeCollectionAgent>(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_CONVEX: {
            return new hkpPredGskfAgent(bodyA, bodyB, mgr);
        }
        default: return nullptr;
    }
}

// The SDK's create for a convex list A against a convex B: the convex list filter picks a predictive GSK agent
// (hkpGskfAgent's constructor, then hkpPredGskfAgent's vtable), a shape collection agent, or a convex list agent
// with the bodies swapped (hkpSymmetricAgent<hkpConvexListAgent>: its constructor, then the symmetric vtable).
// Without a contact manager it is always the shape collection agent.
hkpCollisionAgent* hkpConvexListAgent::createConvexListConvexAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    if (!mgr) {
        return new hkpShapeCollectionAgent(bodyA, bodyB, input, nullptr);
    }
    switch (input.m_convexListFilter->getConvexListCollisionType(bodyA, bodyB, input)) {
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_NORMAL: {
            return new hkpSymmetricAgent<hkpConvexListAgent>(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_LIST: {
            return new hkpShapeCollectionAgent(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_CONVEX: {
            return new hkpPredGskfAgent(bodyA, bodyB, mgr);
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
        return new hkpShapeCollectionAgent(bodyA, bodyB, input, nullptr);
    }
    switch (input.m_convexListFilter->getConvexListCollisionType(bodyA, bodyB, input)) {
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_NORMAL: {
            return new hkpSymmetricAgent<hkpConvexListAgent>(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_LIST: {
            return new hkpShapeCollectionAgent(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_CONVEX: break;
        default: return nullptr;
    }
    switch (input.m_convexListFilter->getConvexListCollisionType(bodyB, bodyA, input)) {
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_NORMAL: {
            return new hkpConvexListAgent(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_LIST: {
            return new hkpSymmetricAgent<hkpShapeCollectionAgent>(bodyA, bodyB, input, mgr);
        }
        case hkpConvexListFilter::TREAT_CONVEX_LIST_AS_CONVEX: {
            return new hkpPredGskfAgent(bodyA, bodyB, mgr);
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
    hkpSymmetricAgentFlipBodyCollector flip(collector);
    hkpShapeCollectionAgent::staticGetPenetrations(bodyB, bodyA, input, flip);
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
