#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/BoxBox/hkpBoxBoxAgent.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskfAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/BoxBox/hkpBoxBoxCollisionDetection.h"
#include "Havok/Source/Physics/Collide/BoxBox/hkpBoxBoxContactPoint.h"
#include "Havok/Source/Physics/Collide/BoxBox/hkpBoxBoxManifold.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"

#include <xmmintrin.h>

hkpBoxBoxAgent::hkpBoxBoxAgent(hkpContactMgr* contactMgr) : hkpIterativeLinearCastAgent(contactMgr) {}

// Nothing of its own to free.
hkpBoxBoxAgent::~hkpBoxBoxAgent() {}

// The agent keeps no state for the query: the static version does the work.
void hkpBoxBoxAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    staticGetPenetrations(bodyA, bodyB, input, collector);
}

void hkpBoxBoxAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    hkpBoxBoxAgent::staticGetClosestPoints(bodyA, bodyB, input, collector);
}

void hkpBoxBoxAgent::linearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    hkpIterativeLinearCastAgent::linearCast(bodyA, bodyB, input, collector, startCollector);
}

// The SDK's processCollision: as staticGetClosestPoints, but the detector gets the input, the agent's contact manager
// and the output, and calcManifold updates the agent's manifold.
void hkpBoxBoxAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN("BoxBox", nullptr);
    const hkpBoxShape* boxA = static_cast<const hkpBoxShape*>(bodyA.m_shape);
    const hkpBoxShape* boxB = static_cast<const hkpBoxShape*>(bodyB.m_shape);

    hkVector4 rA;
    // EXE-INLINED 0x4589C0
    // EXE-INLINED none: hkVector4::add4
    rA.m_quad = _mm_add_ps(_mm_set1_ps(boxA->m_radius), boxA->m_halfExtents.m_quad);
    hkVector4 rB;
    // EXE-INLINED 0x4589C0
    // EXE-INLINED none: hkVector4::add4
    rB.m_quad = _mm_add_ps(_mm_set1_ps(boxB->m_radius), boxB->m_halfExtents.m_quad);

    hkTransform aTb;
    aTb.setMulInverseMul(*bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion);

    const float tolerance = input.m_tolerance;
    hkpBoxBoxCollisionDetection detector(
        bodyA, bodyB, &input, m_contactMgr, &result, *bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion, aTb, rA, rB,
        tolerance
    );
    detector.calcManifold(m_manifold);
    HK_TIMER_END();
}

void hkpBoxBoxAgent::cleanup(hkpConstraintOwner& constraintOwner) {
    int32_t v3 = 0;
    if (m_manifold.m_numPoints) {
        uint16_t* p_m_contactPointId = &m_manifold.m_contactPoints[0].m_contactPointId;
        do {
            m_contactMgr->removeContactPointImpl(*p_m_contactPointId, constraintOwner);
            ++v3;
            p_m_contactPointId += 2;
        }
        while (v3 < m_manifold.m_numPoints);
    }
    delete this;
}

// Each box's radius vector is its half extents plus its convex radius. The detector is built on the stack with no
// environment, contact manager or output; when the boxes intersect within the tolerance, collector gets the pair
// through the virtual addCdBodyPair.
void hkpBoxBoxAgent::staticGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN("BoxBox", nullptr);
    const hkpBoxShape* boxA = static_cast<const hkpBoxShape*>(bodyA.m_shape);
    const hkpBoxShape* boxB = static_cast<const hkpBoxShape*>(bodyB.m_shape);

    hkVector4 rA;
    // EXE-INLINED 0x4589C0
    // EXE-INLINED none: hkVector4::add4
    rA.m_quad = _mm_add_ps(_mm_set1_ps(boxA->m_radius), boxA->m_halfExtents.m_quad);
    hkVector4 rB;
    // EXE-INLINED 0x4589C0
    // EXE-INLINED none: hkVector4::add4
    rB.m_quad = _mm_add_ps(_mm_set1_ps(boxB->m_radius), boxB->m_halfExtents.m_quad);

    const hkTransform& wTa = *bodyA.m_motion.pMotion;
    const hkTransform& wTb = *bodyB.m_motion.pMotion;
    hkTransform aTb;
    aTb.setMulInverseMul(wTa, wTb);

    const float tolerance = input.m_tolerance;
    // EXE-INLINED none: hkpBoxBoxCollisionDetection::hkpBoxBoxCollisionDetection
    hkpBoxBoxCollisionDetection detector(bodyA, bodyB, nullptr, nullptr, nullptr, wTa, wTb, aTb, rA, rB, tolerance);
    detector.initWorkVariables();
    if (detector.checkIntersection(detector.m_tolerance4) == HK_SUCCESS) {
        collector.addCdBodyPair(bodyA, bodyB);
    }
    HK_TIMER_END();
}

// Each box's radius vector is its half extents plus its convex radius; the detector is built on the stack with no
// environment, contact manager or output, and its closest point, if any, goes to collector through the virtual
// addCdPoint. The timer macros write into the stream's raw buffer, which is treated as untyped (hence the casts), as in
// hkpSphereTriangleAgent.
void hkpBoxBoxAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("BoxBox", nullptr);
    const hkpBoxShape* boxA = static_cast<const hkpBoxShape*>(bodyA.m_shape);
    const hkpBoxShape* boxB = static_cast<const hkpBoxShape*>(bodyB.m_shape);

    hkVector4 rA;
    // EXE-INLINED 0x4589C0
    // EXE-INLINED none: hkVector4::add4
    _mm_store_ps(
        &reinterpret_cast<float*>(&rA.m_quad)[0],
        _mm_add_ps(
            _mm_set1_ps(boxA->m_radius), _mm_load_ps(&reinterpret_cast<const float*>(&boxA->m_halfExtents.m_quad)[0])
        )
    );
    hkVector4 rB;
    // EXE-INLINED 0x4589C0
    // EXE-INLINED none: hkVector4::add4
    _mm_store_ps(
        &reinterpret_cast<float*>(&rB.m_quad)[0],
        _mm_add_ps(
            _mm_set1_ps(boxB->m_radius), _mm_load_ps(&reinterpret_cast<const float*>(&boxB->m_halfExtents.m_quad)[0])
        )
    );

    const hkTransform& wTa = *bodyA.m_motion.pMotion;
    const hkTransform& wTb = *bodyB.m_motion.pMotion;
    hkTransform aTb;
    aTb.setMulInverseMul(wTa, wTb);

    const float tolerance = input.m_tolerance;
    alignas(hkpBoxBoxCollisionDetection) uint8_t kDetectorMemory[sizeof(hkpBoxBoxCollisionDetection)];
    const hkpBoxBoxCollisionDetection* pDetector = ::new (kDetectorMemory)
        hkpBoxBoxCollisionDetection(bodyA, bodyB, nullptr, nullptr, nullptr, wTa, wTb, aTb, rA, rB, tolerance);

    hkpCdPoint event;
    event.m_cdBodyA = &bodyA;
    event.m_cdBodyB = &bodyB;
    const hkBool result = pDetector->calculateClosestPoint(event.m_contact);
    if (result.m_bool) {
        collector.addCdPoint(event);
    }
    HK_TIMER_END();
}

// The table holds the agent's own create and penetration functions, the GSK closest points and the iterative linear
// cast; the agent is neither flipped nor predictive.
void hkpBoxBoxAgent::initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af) {
    af.m_createFunc = &hkpBoxBoxAgent::createBoxBoxAgent;
    af.m_getPenetrationsFunc = &hkpBoxBoxAgent::staticGetPenetrations;
    af.m_getClosestPointFunc = &hkpGskBaseAgent::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpIterativeLinearCastAgent::staticLinearCast;
    af.m_isFlipped.m_bool = 0;
    af.m_isPredictive.m_bool = 0;
}

// The AgentFuncs constructor clears both flags before initAgentFunc fills the table. Only m_isFlipped and
// m_isPredictive are written, and registerCollisionAgent copies the whole flags word, so the two padding bytes after
// m_isPredictive, never written, land in the dispatcher's table, as the game does.
void hkpBoxBoxAgent::registerAgent(hkpCollisionDispatcher* a1) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 0;
    hkpBoxBoxAgent::initAgentFunc(f);
    a1->registerCollisionAgent(f, HK_SHAPE_BOX, HK_SHAPE_BOX);
}

// Boxes whose half extents' w times 1.999 (a double constant) is within the tolerance get the GSK agent; the product is
// stored as a float before each compare. Otherwise a box-box agent is built.
hkpCollisionAgent* hkpBoxBoxAgent::createBoxBoxAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* contactMgr
) {
    const hkpBoxShape* boxA = static_cast<const hkpBoxShape*>(bodyA.m_shape);
    const float extentA = boxA->m_halfExtents(3) * 1.9989999532699585;
    if (extentA <= input.m_tolerance) {
        return hkpGskfAgent::createGskfAgent(bodyA, bodyB, input, contactMgr);
    }
    const hkpBoxShape* boxB = static_cast<const hkpBoxShape*>(bodyB.m_shape);
    const float extentB = 1.9989999532699585 * boxB->m_halfExtents(3);
    if (extentB <= input.m_tolerance) {
        return hkpGskfAgent::createGskfAgent(bodyA, bodyB, input, contactMgr);
    }
    hkpBoxBoxAgent* agent =
        static_cast<hkpBoxBoxAgent*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpBoxBoxAgent)));
    agent->m_memSizeAndFlags = sizeof(hkpBoxBoxAgent);
    return ::new (agent) hkpBoxBoxAgent(contactMgr);
}
