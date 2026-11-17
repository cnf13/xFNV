#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereSphere/hkpSphereSphereAgent.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"

#include <cmath>
#include <xmmintrin.h>

hkpSphereSphereAgent::hkpSphereSphereAgent(hkpContactMgr* mgr)
  : hkpIterativeLinearCastAgent(mgr), m_contactPointId(HK_INVALID_CONTACT_POINT) {}

// The contact point is removed in cleanup(), so the destructor has nothing else to free.
hkpSphereSphereAgent::~hkpSphereSphereAgent() {}

void hkpSphereSphereAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    staticGetPenetrations(bodyA, bodyB, input, collector);
}

// The distance between the centres against the sum of the radii and the tolerance; a hit reports one point on sphere
// B's surface towards A, the normal from B to A (the x axis when the centres coincide). When the centres coincide only
// xyz of the point's normal is set, leaving its w uninitialised, and the position, radius B times that whole normal
// plus B's centre, carries the stale w into the reported point's m_position.w, as the game does.
void hkpSphereSphereAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("SphereSphere", nullptr);
    const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
    const hkpSphereShape* sphereB = static_cast<const hkpSphereShape*>(bodyB.m_shape);
    hkMotionState* pStateB = bodyB.m_motion.pState;
    hkpCdPoint point;
    point.m_cdBodyA = &bodyA;
    point.m_cdBodyB = &bodyB;
    const hkVector4* pPosB = &pStateB->m_transform.m_translation;
    __m128 vec = _mm_sub_ps(bodyA.m_motion.pState->m_transform.m_translation.m_quad, pPosB->m_quad);
    __m128 vecSq = _mm_mul_ps(vec, vec);
    float radiusSum = sphereA->m_radius + sphereB->m_radius + input.m_tolerance;
    float distSquared = _mm_shuffle_ps(vecSq, vecSq, 170)[0] + (_mm_shuffle_ps(vecSq, vecSq, 85)[0] + vecSq[0]);
    double distSquaredD = distSquared;
    if (distSquared < radiusSum * radiusSum) {
        double dist;
        if (!(distSquaredD > 0.0)) {
            dist = 0.0;
            point.m_contact.setNormal(hkTransformIdentity.m_rotation.m_col0);
        }
        else {
            float len = std::sqrt(distSquaredD);
            float invLen = 1.0 / len;
            dist = distSquared * invLen;
            point.m_contact.m_separatingNormal.m_quad = _mm_mul_ps(_mm_set1_ps(invLen), vec);
        }
        float distF = dist;
        const __m128 normal = point.m_contact.m_separatingNormal.m_quad;
        float radiusA = sphereA->m_radius;
        float radiusB = sphereB->m_radius;
        point.m_contact.m_position.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(radiusB), normal), pPosB->m_quad);
        point.m_contact.m_separatingNormal.m_quad[3] = distF - (radiusB + radiusA);
        collector.addCdPoint(point);
    }
    HK_TIMER_END();
}

void hkpSphereSphereAgent::linearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    hkpIterativeLinearCastAgent::linearCast(bodyA, bodyB, input, collector, startCollector);
}

// When the centres are closer than the radii's sum plus the tolerance, one contact point on sphere B's surface
// towards A is written into the output's next free point (the normal from B to A, the x axis when the centres
// coincide) and added to the contact manager once; otherwise the agent's contact point is removed.
void hkpSphereSphereAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN("SphereSphere", nullptr);
    const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
    const hkpSphereShape* sphereB = static_cast<const hkpSphereShape*>(bodyB.m_shape);
    const hkVector4* pPosB = &bodyB.m_motion.pState->m_transform.m_translation;
    __m128 vec = _mm_sub_ps(bodyA.m_motion.pState->m_transform.m_translation.m_quad, pPosB->m_quad);
    __m128 vecSq = _mm_mul_ps(vec, vec);
    float radiusSum = sphereA->m_radius + sphereB->m_radius + input.m_tolerance;
    float distSquared = _mm_shuffle_ps(vecSq, vecSq, 170)[0] + (_mm_shuffle_ps(vecSq, vecSq, 85)[0] + vecSq[0]);
    double distSquaredD = distSquared;
    if (distSquared < radiusSum * radiusSum) {
        hkpProcessCdPoint* point = result.m_firstFreeContactPoint.m_storage;
        float dist;
        if (!(distSquaredD > 0.0)) {
            point->m_contact.m_separatingNormal = hkTransformIdentity.m_rotation.m_col0;
            dist = -(sphereA->m_radius + sphereB->m_radius);
        }
        else {
            float len = std::sqrt(distSquaredD);
            float invLen = 1.0 / len;
            point->m_contact.m_separatingNormal.m_quad = _mm_mul_ps(_mm_set1_ps(invLen), vec);
            dist = distSquared * invLen - (sphereA->m_radius + sphereB->m_radius);
        }
        float radiusB = sphereB->m_radius;
        point->m_contact.m_position.m_quad =
            _mm_add_ps(_mm_mul_ps(_mm_set1_ps(radiusB), point->m_contact.m_separatingNormal.m_quad), pPosB->m_quad);
        point->m_contact.m_separatingNormal.m_quad[3] = dist;
        if (m_contactPointId == HK_INVALID_CONTACT_POINT) {
            m_contactPointId =
                m_contactMgr->addContactPointImpl(bodyA, bodyB, input, result, nullptr, point->m_contact);
        }
        if (m_contactPointId != HK_INVALID_CONTACT_POINT) {
            point->m_contactPointId.m_storage = m_contactPointId;
            ++result.m_firstFreeContactPoint.m_storage;
        }
    }
    else if (m_contactPointId != HK_INVALID_CONTACT_POINT) {
        m_contactMgr->removeContactPointImpl(m_contactPointId, *result.m_constraintOwner.m_storage);
        m_contactPointId = HK_INVALID_CONTACT_POINT;
    }
    HK_TIMER_END();
}

void hkpSphereSphereAgent::cleanup(hkpConstraintOwner& constraintOwner) {
    uint16_t m_contactPointId = this->m_contactPointId;
    if (m_contactPointId != 0xFFFF) {
        m_contactMgr->removeContactPointImpl(m_contactPointId, constraintOwner);
        this->m_contactPointId = -1;
    }
    delete this;
}

void hkpSphereSphereAgent::initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af) {
    af.m_createFunc = &hkpSphereSphereAgent::createSphereSphereAgent;
    af.m_getPenetrationsFunc = &hkpSphereSphereAgent::staticGetPenetrations;
    af.m_getClosestPointFunc = &hkpSphereSphereAgent::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpIterativeLinearCastAgent::staticLinearCast;
}

// As getClosestPoints: the distance between the centres against the sum of the radii and the tolerance; a hit reports
// one point on sphere B's surface towards A, the normal from B to A (the x axis when the centres coincide). As in
// getClosestPoints, the coinciding-centres path deliberately reads uninitialised data, as the game does: the normal's
// w, never written, goes into the reported position's w.
void hkpSphereSphereAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("SphereSphere", nullptr);
    const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
    const hkpSphereShape* sphereB = static_cast<const hkpSphereShape*>(bodyB.m_shape);
    hkMotionState* pStateB = bodyB.m_motion.pState;
    hkpCdPoint point;
    point.m_cdBodyA = &bodyA;
    point.m_cdBodyB = &bodyB;
    const hkVector4* pPosB = &pStateB->m_transform.m_translation;
    __m128 vec = _mm_sub_ps(bodyA.m_motion.pState->m_transform.m_translation.m_quad, pPosB->m_quad);
    __m128 vecSq = _mm_mul_ps(vec, vec);
    float radiusSum = sphereA->m_radius + sphereB->m_radius + input.m_tolerance;
    float distSquared = _mm_shuffle_ps(vecSq, vecSq, 170)[0] + (_mm_shuffle_ps(vecSq, vecSq, 85)[0] + vecSq[0]);
    double distSquaredD = distSquared;
    if (distSquared < radiusSum * radiusSum) {
        double dist;
        if (!(distSquaredD > 0.0)) {
            dist = 0.0;
            point.m_contact.setNormal(hkTransformIdentity.m_rotation.m_col0);
        }
        else {
            float len = std::sqrt(distSquaredD);
            float invLen = 1.0 / len;
            dist = distSquared * invLen;
            point.m_contact.m_separatingNormal.m_quad = _mm_mul_ps(_mm_set1_ps(invLen), vec);
        }
        float distF = dist;
        const __m128 normal = point.m_contact.m_separatingNormal.m_quad;
        float radiusA = sphereA->m_radius;
        float radiusB = sphereB->m_radius;
        point.m_contact.m_position.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(radiusB), normal), pPosB->m_quad);
        point.m_contact.m_separatingNormal.m_quad[3] = distF - (radiusB + radiusA);
        collector.addCdPoint(point);
    }
    HK_TIMER_END();
}

// The pair is reported when the squared distance between the centres is below the squared sum of the radii.
void hkpSphereSphereAgent::staticGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN("SphereSphere", nullptr);
    const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
    const hkpSphereShape* sphereB = static_cast<const hkpSphereShape*>(bodyB.m_shape);
    __m128 vec = _mm_sub_ps(
        bodyB.m_motion.pState->m_transform.m_translation.m_quad, bodyA.m_motion.pState->m_transform.m_translation.m_quad
    );
    __m128 vecSq = _mm_mul_ps(vec, vec);
    float radiusSum = sphereA->m_radius + sphereB->m_radius;
    float distSquared = _mm_shuffle_ps(vecSq, vecSq, 170)[0] + (_mm_shuffle_ps(vecSq, vecSq, 85)[0] + vecSq[0]);
    if (distSquared < radiusSum * radiusSum) {
        collector.addCdBodyPair(bodyA, bodyB);
    }
    HK_TIMER_END();
}

// Both flags are cleared before initAgentFunc fills the table; initAgentFunc writes neither. Only the two flag bytes
// are written, and registerCollisionAgent copies the whole flags word, so the two padding bytes after m_isPredictive,
// never written, land in the dispatcher's table: an uninitialised read, as the game does.
void hkpSphereSphereAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 0;
    hkpSphereSphereAgent::initAgentFunc(f);
    dispatcher->registerCollisionAgent(f, HK_SHAPE_SPHERE, HK_SHAPE_SPHERE);
}

// As registerAgent, through the dispatcher's second agent table. Only m_isFlipped and m_isPredictive of f's flags are
// written (initAgentFunc writes neither), and registerCollisionAgent2 copies the whole flags word, so the two padding
// bytes after m_isPredictive land in the dispatcher's table uninitialised, as the game does.
void hkpSphereSphereAgent::registerAgent2(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 0;
    hkpSphereSphereAgent::initAgentFunc(f);
    dispatcher->registerCollisionAgent2(f, HK_SHAPE_SPHERE, HK_SHAPE_SPHERE);
}

// The SDK's new hkpSphereSphereAgent(mgr): the class allocator's operator new (the thread's heap, the size kept in
// m_memSizeAndFlags), then the constructor.
hkpCollisionAgent* hkpSphereSphereAgent::createSphereSphereAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* contactMgr
) {
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    hkpSphereSphereAgent* agent =
        static_cast<hkpSphereSphereAgent*>(pRouter->m_heap->blockAlloc(sizeof(hkpSphereSphereAgent)));
    agent->m_memSizeAndFlags = sizeof(hkpSphereSphereAgent);
    return ::new (agent) hkpSphereSphereAgent(contactMgr);
}
