#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskfAgent.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/BoxBox/hkpBoxBoxAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskBaseAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskManifold.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3Input.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Gjk/GskManifold/hkpGskManifoldUtil.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/Agent/hkpGskAgentUtil.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"
#include "SDK/Windows/winbase.h"

#include <xmmintrin.h>

// The edge-edge case of the vertices at offsets 4 and 5 (one side) and 6 and 7 (the other): out[2] gets the
// direction; returns 1 when the closest points of the two edges do not both lie inside them, else 0 with out[0] the
// closest point on the first edge and out[1] the difference to the closest point on the second.
int32_t calcEdgeEdgeClosestPoints(const uint8_t* simplex, hkVector4* out, const hkVector4* vertices) {
    const uint8_t* base = reinterpret_cast<const uint8_t*>(vertices);
    const __m128* v4 = reinterpret_cast<const __m128*>(base + simplex[4]);
    const __m128* v5 = reinterpret_cast<const __m128*>(base + simplex[5]);
    const __m128* v6 = reinterpret_cast<const __m128*>(base + simplex[6]);
    const __m128* v7 = reinterpret_cast<const __m128*>(base + simplex[7]);
    __m128 p7 = *v7;
    __m128 p6 = *v6;
    __m128 p4 = *v4;
    __m128 d54 = _mm_sub_ps(*v5, p4);
    __m128 d46 = _mm_sub_ps(p4, p6);
    __m128 d57 = _mm_sub_ps(*v5, p7);
    __m128 d76 = _mm_sub_ps(p7, p6);
    __m128 d76Yzx = _mm_shuffle_ps(d76, d76, 0xC9);
    __m128 d54Zxy = _mm_shuffle_ps(d54, d54, 0xD2);
    __m128 d76Zxy = _mm_shuffle_ps(d76, d76, 0xD2);
    __m128 d54Yzx = _mm_shuffle_ps(d54, d54, 0xC9);
    __m128 cross = _mm_sub_ps(_mm_mul_ps(d54Yzx, d76Zxy), _mm_mul_ps(d54Zxy, d76Yzx));
    __m128 crossYzx = _mm_shuffle_ps(cross, cross, 0xC9);
    __m128 crossZxy = _mm_shuffle_ps(cross, cross, 0xD2);
    __m128 t = _mm_mul_ps(crossZxy, d54Yzx);
    __m128 n76 = _mm_sub_ps(_mm_mul_ps(crossZxy, d76Yzx), _mm_mul_ps(crossYzx, d76Zxy));
    out[2].m_quad = vertices[16].m_quad;
    __m128 a = _mm_mul_ps(n76, d57);
    __m128 n54 = _mm_sub_ps(t, _mm_mul_ps(crossYzx, d54Zxy));
    __m128 b = _mm_mul_ps(n54, d57);
    __m128 cc = _mm_mul_ps(n76, d46);
    __m128 d = _mm_mul_ps(n54, d46);
    __m128 abLo = _mm_shuffle_ps(a, b, 0x44);
    __m128 abHi = _mm_shuffle_ps(a, b, 0xEE);
    __m128 cdLo = _mm_shuffle_ps(cc, d, 0x44);
    __m128 dots = _mm_add_ps(
        _mm_shuffle_ps(_mm_shuffle_ps(cc, d, 0xEE), abHi, 0x88),
        _mm_add_ps(_mm_shuffle_ps(cdLo, abLo, 0xDD), _mm_shuffle_ps(cdLo, abLo, 0x88))
    );
    int32_t mask = _mm_movemask_ps(_mm_cmplt_ps(dots, _mm_setzero_ps()));
    if (mask != 3 && mask != 12) {
        return 1;
    }
    __m128 f0 = _mm_div_ss(dots, _mm_sub_ss(dots, _mm_shuffle_ps(dots, dots, 0xAA)));
    f0 = _mm_shuffle_ps(f0, f0, 0);
    __m128 onFirst = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, f0), *v4), _mm_mul_ps(*v5, f0));
    __m128 dotsY = _mm_shuffle_ps(dots, dots, 0x55);
    out[0].m_quad = onFirst;
    __m128 f1 = _mm_div_ss(dotsY, _mm_sub_ss(dotsY, _mm_shuffle_ps(dots, dots, 0xFF)));
    f1 = _mm_shuffle_ps(f1, f1, 0);
    out[1].m_quad = _mm_sub_ps(
        onFirst,
        _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, f1), *v6), _mm_mul_ps(*v7, f1))
    );
    return 0;
}

hkpGskfAgent::hkpGskfAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, hkpContactMgr* mgr)
  : hkpGskBaseAgent(bodyA, bodyB, mgr) {
    m_manifold.init();
}

// Nothing of its own to free: the contacts are removed in cleanup().
hkpGskfAgent::~hkpGskfAgent() {}

// The SDK's TIM early out: unless the separating normal was taken at this step's start, its distance (w) is moved by
// how far the bodies can have closed during the step; while it stays above the tolerance the GSK is skipped.
void hkpGskfAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN_LIST("GskAgent", "Tim");
    if (input.m_stepInfo.m_startTime == m_timeOfSeparatingNormal) {
        goto PROCESS_AT_T1;
    }
    {
        const hkMotionState* pStateA = bodyA.m_motion.pState;
        const hkMotionState* pStateB = bodyB.m_motion.pState;
        alignas(16) hkVector4 timInfo;
        hkSweptTransformUtil::calcTimInfo(*pStateA, *pStateB, input.m_stepInfo.m_deltaTime, timInfo);

        if (m_separatingNormal(3) > input.m_tolerance) {
            // timInfo's xyz against the normal's, plus timInfo's w.
            const float fDot = timInfo.dot4xyz1(m_separatingNormal);
            m_separatingNormal(3) = m_separatingNormal(3) - fDot;
            if (m_separatingNormal(3) > input.m_tolerance) {
                goto END;
            }
        }
    }
PROCESS_AT_T1:
    HK_TIMER_SPLIT_LIST("Gsk");
    m_timeOfSeparatingNormal = input.m_stepInfo.m_endTime;
    processCollisionNoTim(bodyA, bodyB, input, result);
END:
    HK_TIMER_END_LIST();
}

void hkpGskfAgent::cleanup(hkpConstraintOwner& constraintOwner) {
    hkGskManifold_cleanup(m_manifold, m_contactMgr, constraintOwner);
    delete this;
}

// Removes the contact point with the id, if the manifold has one.
void hkpGskfAgent::removePoint(uint16_t idToRemove) {
    const uint32_t uiCount = m_manifold.m_numContactPoints;
    for (uint32_t i = 0; i < uiCount; ++i) {
        if (m_manifold.m_contactPoints[i].m_id == idToRemove) {
            hkGskManifold_removePoint(m_manifold, static_cast<int32_t>(i));
            return;
        }
    }
}

// Gives the first unused manifold point (id 0xffff) the new id.
void hkpGskfAgent::commitPotential(uint16_t idToCommit) {
    uint16_t* i;
    int32_t result = 0;
    if (m_manifold.m_numContactPoints) {
        for (i = &m_manifold.m_contactPoints[0].m_id; (*i) != 0xffff; i += 4) {
            if ((++result) >= m_manifold.m_numContactPoints) {
                return;
            }
        }
        m_manifold.m_contactPoints[result].m_id = idToCommit;
    }
}

// Turns the manifold point with the id into a zombie (clears its two vertex dimensions).
void hkpGskfAgent::createZombie(uint16_t idTobecomeZombie) {
    int32_t m_numContactPoints = m_manifold.m_numContactPoints;
    int32_t v3 = 0;
    if (m_manifold.m_numContactPoints) {
        hkpGskManifold::ContactPoint* i;
        for (i = m_manifold.m_contactPoints; i->m_id != idTobecomeZombie; ++i) {
            if (++v3 >= m_numContactPoints) {
                return;
            }
        }
        i->m_dimA = 0;
        i->m_dimB = 0;
    }
}

// Processes the pair at the end of the step: the agent3 input over the two bodies with B's transform taken into A's
// space, and a dummy agent entry and agent data (a GSK cache, whose flags byte the callee clears and sets) on the
// stack, then the GSK collision with the agent's own cache, manifold and separating normal.
void hkpGskfAgent::processCollisionNoTim(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& output
) {
    hkpAgent3Input in3;
    hkpAgentEntry dummyEntry;
    // Starts zeroed (a departure): nothing else writes it, and hkGskAgentUtil_processCollisionNoTim clears and sets a
    // bit of its flags byte; the exe leaves the other bits unset.
    hkpGskCache dummyData{};
    in3.m_contactMgr = m_contactMgr;
    in3.m_bodyB = &bodyB;
    in3.m_input = &input;
    in3.m_overrideBodyA = nullptr;
    in3.m_bodyA = &bodyA;
    in3.m_aTb.setMulInverseMul(*bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion);
    hkGskAgentUtil_processCollisionNoTim(in3, &dummyEntry, &dummyData, m_cache, m_manifold, m_separatingNormal, output);
}

// The SDK's create: a GSK agent keeping a manifold where there is a contact manager, else the base agent alone (no
// manifold).
hkpCollisionAgent* hkpGskfAgent::createGskfAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    if (mgr) {
        return new hkpGskfAgent(bodyA, bodyB, mgr);
    }
    return new hkpGskBaseAgent(bodyA, bodyB, nullptr);
}

// Fills the agent functions: the gskf create, hkpGskBaseAgent's static queries and the iterative linear cast; neither
// flipped nor predictive.
void hkpGskfAgent::initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af) {
    af.m_createFunc = &hkpGskfAgent::createGskfAgent;
    af.m_getPenetrationsFunc = &hkpGskBaseAgent::staticGetPenetrations;
    af.m_getClosestPointFunc = &hkpGskBaseAgent::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpIterativeLinearCastAgent::staticLinearCast;
    af.m_isFlipped.m_bool = 0;
    af.m_isPredictive.m_bool = 0;
}

// KEEP: partcopy. The padding after f's two bools is never written (initAgentFunc writes only the bools too), and
// registerCollisionAgent copies the whole AgentFuncs into the dispatcher, the padding included.
void hkpGskfAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 0;
    hkpGskfAgent::initAgentFunc(f);
    dispatcher->registerCollisionAgent(f, HK_SHAPE_CONVEX, HK_SHAPE_CONVEX);
}
