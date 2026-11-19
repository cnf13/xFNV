#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpPredGskfAgent.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/BoxBox/hkpBoxBoxAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGsk.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskBaseAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskManifold.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Gjk/GskManifold/hkpGskManifoldUtil.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/Continuous/hkpContinuousGsk.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include <xmmintrin.h>

// Havok SDK: the predictive GSK agent. When the cached separating normal is not from this step's start, it is
// recomputed there first (continuous physics only). The normal's distance, less the bodies' worst-case approach over
// the step (the TIM), decides: far apart, the manifold is dropped; with continuous physics and closer than the
// minimum separation, the time of impact is searched; above the manifold TIM distance, only the manifold's points are
// moved to the bodies' new transforms; otherwise the full GSK runs (processCollisionNoTim).
void hkpPredGskfAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN("Gsk", nullptr);
    if (input.m_stepInfo.m_startTime != m_timeOfSeparatingNormal) {
        if (!input.m_collisionQualityInfo->m_useContinuousPhysics.m_storage) {
            m_timeOfSeparatingNormal = input.m_stepInfo.m_endTime;
            goto PROCESS_AT_T1;
        }
        {
            hkTransform tA;
            hkTransform tB;
            hkSweptTransformUtil::lerp2(bodyA.m_motion.pState->m_sweptTransform, input.m_stepInfo.m_startTime, tA);
            hkSweptTransformUtil::lerp2(bodyB.m_motion.pState->m_sweptTransform, input.m_stepInfo.m_startTime, tB);
            // EXE-INLINED none: hkpCdBody::hkpCdBody(const hkpCdBody* parent, const hkTransform* t)
            hkpCdBody bodyAt0;
            bodyAt0.m_shape = bodyA.m_shape;
            bodyAt0.m_shapeKey = bodyA.m_shapeKey;
            bodyAt0.m_motion.pMotion = &tA;
            bodyAt0.m_parent = &bodyA;
            hkpCdBody bodyBt0;
            bodyBt0.m_shape = bodyB.m_shape;
            bodyBt0.m_shapeKey = bodyB.m_shapeKey;
            bodyBt0.m_motion.pMotion = &tB;
            bodyBt0.m_parent = &bodyB;
            const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
            const hkpConvexShape* shapeB = static_cast<const hkpConvexShape*>(bodyB.m_shape);

            // hkpGsk::init, with hkPadSpu's stores and loads written as plain field accesses; this agent's copy clears
            // m_doNotHandlePenetration.
            hkpGsk gsk;
            gsk.m_dimA.m_storage = m_cache.m_dimA;
            gsk.m_dimB.m_storage = m_cache.m_dimB;
            gsk.m_maxDimA.m_storage = m_cache.m_maxDimA;
            gsk.m_maxDimB.m_storage = m_cache.m_maxDimB;
            gsk.m_doNotHandlePenetration.m_storage = 0;
            gsk.m_featureChange.m_storage = 0;
            shapeA->convertVertexIdsToVerticesImpl(&m_cache.m_vertices[0], gsk.m_dimA.m_storage, &gsk.m_verticesA[0]);
            shapeB->convertVertexIdsToVerticesImpl(
                &m_cache.m_vertices[gsk.m_dimA.m_storage], gsk.m_dimB.m_storage, &gsk.m_verticesBinB[0]
            );

            calcSeparatingNormal(
                bodyAt0, bodyBt0, input.m_collisionQualityInfo->m_keepContact, gsk, m_separatingNormal
            );
            if (gsk.m_featureChange.m_storage) {
                gsk.exitAndExportCacheImpl(m_cache);
            }
        }
    }
    m_timeOfSeparatingNormal = input.m_stepInfo.m_endTime;
    {
        const hkMotionState& msA = *bodyA.m_motion.pState;
        const hkMotionState& msB = *bodyB.m_motion.pState;
        hkVector4 timInfo;
        hkSweptTransformUtil::calcTimInfo(msA, msB, input.m_stepInfo.m_deltaTime, timInfo);

        const hkpCollisionQualityInfo& qi = *input.m_collisionQualityInfo;
        float timDot = timInfo.dot3(m_separatingNormal);
        // The projected distance is rounded to a float before the compares.
        float dist = m_separatingNormal(3) - timDot - timInfo(3);

        if (qi.m_keepContact < dist && m_allowedPenetration * 0.5 < dist) {
            m_separatingNormal(3) = dist;
            if (m_manifold.m_numContactPoints) {
                hkGskManifold_cleanup(m_manifold, m_contactMgr, *result.m_constraintOwner.m_storage);
            }
            goto END;
        }

        if (qi.m_useContinuousPhysics.m_storage) {
            hkpAgent3ProcessInput in3;
            in3.m_bodyA = &bodyA;
            in3.m_bodyB = &bodyB;
            in3.m_overrideBodyA = nullptr;
            in3.m_input = &input;
            in3.m_contactMgr = m_contactMgr;
            in3.m_distAtT1.m_storage = dist;
            in3.m_linearTimInfo = timInfo;
            float distAtT0 = m_separatingNormal(3);
            // hkMath::min2 semantics: the second unless it is the greater (a tie or a NaN gives it).
            float minSeparationAbs = qi.m_minSeparation * m_allowedPenetration;
            float minSeparationRel = distAtT0 + qi.m_minExtraSeparation * m_allowedPenetration;
            float minSeparation = minSeparationRel > minSeparationAbs ? minSeparationAbs : minSeparationRel;
            // The time of impact is also searched when the compare is unordered (NaN), as the game does.
            if (!(minSeparation <= dist)) {
                float toiSeparationAbs = qi.m_toiSeparation * m_allowedPenetration;
                float toiSeparationRel = distAtT0 + qi.m_toiExtraSeparation * m_allowedPenetration;
                float toiSeparation = toiSeparationRel > toiSeparationAbs ? toiSeparationAbs : toiSeparationRel;
                hk4dGskCollideCalcToi(
                    in3, m_allowedPenetration, minSeparation, toiSeparation, m_cache, m_separatingNormal, result
                );
                goto PROCESS_AT_T1;
            }
        }

        if (qi.m_manifoldTimDistance < dist) {
            m_separatingNormal(3) = dist;
            const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
            const hkpConvexShape* shapeB = static_cast<const hkpConvexShape*>(bodyB.m_shape);
            hkpGskManifoldWork work;
            work.m_keepContact.m_storage = input.m_tolerance;
            work.m_masterNormal = m_separatingNormal;
            work.m_radiusA.m_storage = shapeA->m_radius;
            work.m_radiusB.m_storage = shapeB->m_radius;
            // The radius sum is rounded to a float before it is squared.
            float radiusSum = work.m_radiusA.m_storage + work.m_keepContact.m_storage + work.m_radiusB.m_storage;
            work.m_radiusSumSqrd.m_storage = radiusSum * radiusSum;
            if (m_manifold.m_numContactPoints) {
                const uint16_t* vertexIds = m_manifold.getVertexIds();
                shapeA->convertVertexIdsToVerticesImpl(vertexIds, m_manifold.m_numVertsA, &work.m_vertices[0]);
                hkVector4Util::transformPoints(
                    *bodyA.m_motion.pMotion, work.m_vertices, m_manifold.m_numVertsA, work.m_vertices
                );
                hkpCdVertex* verticesB = &work.m_vertices[m_manifold.m_numVertsA];
                shapeB->convertVertexIdsToVerticesImpl(
                    vertexIds + m_manifold.m_numVertsA, m_manifold.m_numVertsB, verticesB
                );
                hkVector4Util::transformPoints(*bodyB.m_motion.pMotion, verticesB, m_manifold.m_numVertsB, verticesB);
            }
            hkGskManifold_verifyAndGetPoints(m_manifold, work, 0, result, m_contactMgr);
            goto END;
        }
    }
PROCESS_AT_T1:
    processCollisionNoTim(bodyA, bodyB, input, result);
END:
    HK_TIMER_END();
}

// KEEP: partcopy. Of f's flag dword the exe writes only the two bool bytes (0xD42000, 0xD42005), and
// registerCollisionAgent (0xCFBBE0) copies the whole AgentFuncs into the dispatcher, the two bytes nothing wrote
// included.
void hkpPredGskfAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_createFunc = &hkpPredGskfAgent::createPredGskfAgent;
    f.m_getPenetrationsFunc = &hkpGskBaseAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpGskBaseAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpIterativeLinearCastAgent::staticLinearCast;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_CONVEX, HK_SHAPE_CONVEX);
}

// With a contact manager the predictive agent (hkpGskfAgent's constructor, then this class's vtable), else a
// hkpGskBaseAgent with no manager; each allocation stores its size, as Havok's class operator new does.
hkpCollisionAgent* hkpPredGskfAgent::createPredGskfAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    if (mgr) {
        hkpPredGskfAgent* agent =
            static_cast<hkpPredGskfAgent*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpPredGskfAgent)));
        agent->m_memSizeAndFlags = sizeof(hkpPredGskfAgent);
        return ::new (agent) hkpPredGskfAgent(bodyA, bodyB, mgr);
    }

    hkpGskBaseAgent* agent =
        static_cast<hkpGskBaseAgent*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpGskBaseAgent)));
    agent->m_memSizeAndFlags = sizeof(hkpGskBaseAgent);
    return ::new (agent) hkpGskBaseAgent(bodyA, bodyB, nullptr);
}
