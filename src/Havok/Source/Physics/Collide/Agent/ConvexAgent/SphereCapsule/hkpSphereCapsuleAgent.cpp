#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereCapsule/hkpSphereCapsuleAgent.h"

#include "Havok/Source/Common/Base/Algorithm/Collide/LineSegment/hkLineSegmentUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"

#include <cmath>
#include <xmmintrin.h>

hkpSphereCapsuleAgent::hkpSphereCapsuleAgent(hkpContactMgr* mgr)
  : hkpIterativeLinearCastAgent(mgr), m_contactPointId(HK_INVALID_CONTACT_POINT) {}

// Nothing of its own to free.
hkpSphereCapsuleAgent::~hkpSphereCapsuleAgent() {}

// The agent keeps no state for the query: the static version does the work.
void hkpSphereCapsuleAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    staticGetPenetrations(bodyA, bodyB, input, collector);
}

// Tests the sphere's centre against the capsule's segment in world space, giving a point when nearer than the radii and
// the tolerance.
void hkpSphereCapsuleAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("SphereCapsule", nullptr);
    const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
    const hkpCapsuleShape* capsB = static_cast<const hkpCapsuleShape*>(bodyB.m_shape);
    const hkVector4& posA = bodyA.m_motion.pMotion->m_translation;

    hkpCdPoint event;
    event.m_cdBodyA = &bodyA;
    event.m_cdBodyB = &bodyB;

    // The capsule's two vertices in world space. The SDK's getVertices() is &m_vertexA, m_vertexB right after it.
    hkVector4 posB[2];
    hkVector4Util::transformPoints(*bodyB.m_motion.pMotion, &capsB->m_vertexA, 2, posB);

    hkLineSegmentUtil::ClosestPointLineSegResult result;
    hkLineSegmentUtil::closestPointLineSeg(posA, posB[0], posB[1], result);

    const float radiusSum = sphereA->m_radius + capsB->m_radius;
    hkVector4 aMinusB;
    aMinusB.m_quad = _mm_sub_ps(posA.m_quad, result.m_pointOnEdge.m_quad);
    const float refDist = input.m_tolerance + radiusSum;
    // lengthSquared3: z + (y + x), in SSE.
    const __m128 squares = _mm_mul_ps(aMinusB.m_quad, aMinusB.m_quad);
    const float distSquared = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(squares, squares, 0xAA), _mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares))
    );
    // Rounds the squared reference distance to a float before the compare.
    const float refDistSquared = refDist * refDist;
    if (!(distSquared >= refDistSquared)) {
        float dist;
        if (distSquared > 0.0f) {
            dist = std::sqrt(distSquared);
        }
        else {
            dist = 0.0f;
            hkVector4 edge;
            edge.m_quad = _mm_sub_ps(posB[1].m_quad, posB[0].m_quad);
            hkVector4Util::calculatePerpendicularVector(edge, aMinusB);
        }
        // normalize3: the broadcast inverse length (approximate reciprocal square root, one Newton step, 0 for a zero
        // length) times the vector.
        const __m128 normalSquares = _mm_mul_ps(aMinusB.m_quad, aMinusB.m_quad);
        const __m128 len2 = _mm_add_ss(
            _mm_shuffle_ps(normalSquares, normalSquares, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(normalSquares, normalSquares, 0x55), normalSquares)
        );
        const __m128 approx = _mm_rsqrt_ss(len2);
        const __m128 len2IsZero = _mm_cmpeq_ss(len2, _mm_set_ss(0.0f));
        const __m128 refined = _mm_mul_ss(
            _mm_mul_ss(_mm_set_ss(0.5f), approx),
            _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx))
        );
        const __m128 lenInv = _mm_andnot_ps(len2IsZero, refined);
        event.m_contact.m_separatingNormal.m_quad = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, 0), aMinusB.m_quad);
        // The point on the capsule's surface: posA + normal * (capsule radius - dist); the distance in w.
        const float fromCentre = capsB->m_radius - dist;
        const __m128 scale = _mm_set_ss(fromCentre);
        event.m_contact.m_position.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0), event.m_contact.m_separatingNormal.m_quad),
            posA.m_quad
        );
        event.m_contact.m_separatingNormal(3) = dist - radiusSum;
        collector.addCdPoint(event);
    }
    HK_TIMER_END();
}

// As getClosestPoints but writing the point into the output's next free point: the sphere's centre against the
// capsule's segment in world space; when nearer than the radii and the tolerance the point is added to the contact
// manager once, otherwise the agent's contact point is removed. Builds the unnormalised normal in the output point and
// normalizes it there.
void hkpSphereCapsuleAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN("SphereCapsule", nullptr);
    hkpProcessCdPoint* point = result.m_firstFreeContactPoint.m_storage;
    const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
    const hkpCapsuleShape* capsB = static_cast<const hkpCapsuleShape*>(bodyB.m_shape);
    const hkVector4& posA = bodyA.m_motion.pState->m_transform.m_translation;

    // The capsule's two vertices in world space. The SDK's getVertices() is &m_vertexA, m_vertexB right after it.
    hkVector4 posB[2];
    hkVector4Util::transformPoints(bodyB.m_motion.pState->m_transform, &capsB->m_vertexA, 2, posB);

    hkLineSegmentUtil::ClosestPointLineSegResult lineResult;
    hkLineSegmentUtil::closestPointLineSeg(posA, posB[0], posB[1], lineResult);

    const float radiusSum = sphereA->m_radius + capsB->m_radius;
    hkVector4 aMinusB;
    aMinusB.m_quad = _mm_sub_ps(posA.m_quad, lineResult.m_pointOnEdge.m_quad);
    const float refDist = input.m_tolerance + radiusSum;
    // lengthSquared3: z + (y + x), in SSE.
    const __m128 squares = _mm_mul_ps(aMinusB.m_quad, aMinusB.m_quad);
    const float distSquared = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(squares, squares, 0xAA), _mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares))
    );
    // Rounds the squared reference distance to a float before the compare.
    const float refDistSquared = refDist * refDist;
    if (!(distSquared >= refDistSquared)) {
        hkVector4& normal = point->m_contact.m_separatingNormal;
        float dist;
        if (distSquared > 0.0f) {
            dist = std::sqrt(distSquared);
            normal = aMinusB;
        }
        else {
            dist = 0.0f;
            hkVector4 edge;
            edge.m_quad = _mm_sub_ps(posB[1].m_quad, posB[0].m_quad);
            hkVector4Util::calculatePerpendicularVector(edge, normal);
        }
        // normalize3: the broadcast inverse length (approximate reciprocal square root, one Newton step, 0 for a zero
        // length) times the vector.
        const __m128 unnormalised = normal.m_quad;
        const __m128 normalSquares = _mm_mul_ps(unnormalised, unnormalised);
        const __m128 len2 = _mm_add_ss(
            _mm_shuffle_ps(normalSquares, normalSquares, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(normalSquares, normalSquares, 0x55), normalSquares)
        );
        const __m128 approx = _mm_rsqrt_ss(len2);
        const __m128 len2IsZero = _mm_cmpeq_ss(len2, _mm_set_ss(0.0f));
        const __m128 refined = _mm_mul_ss(
            _mm_mul_ss(_mm_set_ss(0.5f), approx),
            _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx))
        );
        const __m128 lenInv = _mm_andnot_ps(len2IsZero, refined);
        normal.m_quad = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, 0), unnormalised);
        // The point on the capsule's surface: posA + normal * (capsule radius - dist); the distance in w.
        const float fromCentre = capsB->m_radius - dist;
        const __m128 scale = _mm_set_ss(fromCentre);
        point->m_contact.m_position.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0), normal.m_quad),
            posA.m_quad
        );
        normal(3) = dist - radiusSum;
        if (m_contactPointId == HK_INVALID_CONTACT_POINT) {
            m_contactPointId = m_contactMgr
                                   ->addContactPointImpl(bodyA, bodyB, input, result, nullptr, point->m_contact);
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

void hkpSphereCapsuleAgent::cleanup(hkpConstraintOwner& constraintOwner) {
    uint16_t m_contactPointId = this->m_contactPointId;
    if (m_contactPointId != 0xFFFF) {
        m_contactMgr->removeContactPointImpl(m_contactPointId, constraintOwner);
    }
    delete this;
}

// Takes the capsule's two end points into world space, finds the point of the segment closest to the sphere's centre,
// and reports the pair when that point is closer than the two radii.
void hkpSphereCapsuleAgent::staticGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN("SphereCapsule", nullptr);
    const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
    const hkpCapsuleShape* capsB = static_cast<const hkpCapsuleShape*>(bodyB.m_shape);
    const hkVector4& posA = bodyA.m_motion.pMotion->m_translation;

    alignas(16) hkVector4 posB[2];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        const hkVector4* vertices = &capsB->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            posB[i].setTransformedPos(t, vertices[i]);
        }
    }

    alignas(16) hkLineSegmentUtil::ClosestPointLineSegResult result;
    hkLineSegmentUtil::closestPointLineSeg(posA, posB[0], posB[1], result);

    // The radius sum is a float local.
    const float radiusSum = capsB->m_radius + sphereA->m_radius;
    hkVector4 d;
    d.setSub4(result.m_pointOnEdge, posA);
    const float distSquared = d.lengthSquared3();
    const float radiusSumSquared = radiusSum * radiusSum;
    // The pair is reported only when the squared radius sum is above the squared distance (not when equal, below or
    // NaN).
    if (distSquared < radiusSumSquared) {
        collector.addCdBodyPair(bodyA, bodyB);
    }
    HK_TIMER_END();
}

// Computes the capsule's two vertices in world space (last vertex first), the point of the segment closest to the
// sphere's centre, and reports a point when nearer than the radii and the tolerance. For a zero distance the edge's
// perpendicular is built in the event's normal and normalized there.
void hkpSphereCapsuleAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("SphereCapsule", nullptr);
    const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
    const hkpCapsuleShape* capsB = static_cast<const hkpCapsuleShape*>(bodyB.m_shape);
    const hkVector4& posA = bodyA.m_motion.pMotion->m_translation;

    alignas(16) hkpCdPoint event;
    event.m_cdBodyA = &bodyA;
    event.m_cdBodyB = &bodyB;

    alignas(16) hkVector4 posB[2];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &capsB->m_vertexA, 2, posB);
    }

    alignas(16) hkLineSegmentUtil::ClosestPointLineSegResult result;
    hkLineSegmentUtil::closestPointLineSeg(posA, posB[0], posB[1], result);

    const float radiusSum = sphereA->m_radius + capsB->m_radius;
    alignas(16) hkVector4 aMinusB;
    aMinusB.m_quad = _mm_sub_ps(posA.m_quad, result.m_pointOnEdge.m_quad);
    const float refDist = input.m_tolerance + radiusSum;
    const float distSquared = aMinusB.lengthSquared3();
    // The squared reference distance is rounded to a float before the compare.
    const float refDistSquared = refDist * refDist;
    // The point is skipped when the squared reference distance is below or equal to the squared distance, taken
    // otherwise (NaN included).
    if (!(refDistSquared <= distSquared)) {
        float dist;
        hkVector4 normal;
        // The square root is taken only for a squared distance above zero (not for NaN).
        if (distSquared > 0.0f) {
            dist = std::sqrt(distSquared);
            normal = aMinusB;
        }
        else {
            dist = 0.0f;
            alignas(16) hkVector4 edge;
            edge.m_quad = _mm_sub_ps(posB[1].m_quad, posB[0].m_quad);
            hkVector4Util::calculatePerpendicularVector(edge, event.m_contact.m_separatingNormal);
            normal = event.m_contact.m_separatingNormal;
        }
        normal.normalize3();
        const __m128 unitNormal = normal.m_quad;
        event.m_contact.m_separatingNormal.m_quad = unitNormal;
        // The point on the capsule's surface: posA + normal * (capsule radius - dist), from the normal before its w
        // takes the distance.
        const float fromCentre = capsB->m_radius - dist;
        const __m128 scale = _mm_set_ss(fromCentre);
        event.m_contact.m_separatingNormal(3) = dist - radiusSum;
        event.m_contact.m_position.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0), unitNormal),
            posA.m_quad
        );
        collector.addCdPoint(event);
    }
    HK_TIMER_END();
}

// The SDK's body: the flipped agent for a capsule against a sphere, then the agent for a sphere against a capsule.
// KEEP: the AgentFuncs local's padding after m_isPredictive is never written; registerCollisionAgent copies it
// whole, the padding included.
void hkpSphereCapsuleAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_createFunc = &hkpSphereCapsuleAgent::createCapsuleSphereAgent;
    f.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpSphereCapsuleAgent>::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpSphereCapsuleAgent>::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>::staticLinearCast;
    f.m_isFlipped.m_bool = true;
    f.m_isPredictive.m_bool = false;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_CAPSULE, HK_SHAPE_SPHERE);

    f.m_createFunc = &hkpSphereCapsuleAgent::createSphereCapsuleAgent;
    f.m_getPenetrationsFunc = &hkpSphereCapsuleAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpSphereCapsuleAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpIterativeLinearCastAgent::staticLinearCast;
    f.m_isFlipped.m_bool = false;
    f.m_isPredictive.m_bool = false;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_SPHERE, HK_SHAPE_CAPSULE);
}

// Tests the sphere's centre against the capsule's segment in world space; the point is written when nearer than the
// radii and the tolerance. The normal is built and normalised in cpoint itself.
hkpSphereCapsuleAgent::ClosestPointResult hkpSphereCapsuleAgent::getClosestPoint(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkContactPoint& cpoint
) {
    const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
    const hkVector4& posA = bodyA.m_motion.pMotion->m_translation;
    const hkpCapsuleShape* capsB = static_cast<const hkpCapsuleShape*>(bodyB.m_shape);

    // The capsule's two vertices in world space.
    hkVector4 posB[2];
    hkVector4Util::transformPoints(*bodyB.m_motion.pMotion, &capsB->m_vertexA, 2, posB);

    hkLineSegmentUtil::ClosestPointLineSegResult result;
    hkLineSegmentUtil::closestPointLineSeg(posA, posB[0], posB[1], result);

    const float radiusSum = sphereA->m_radius + capsB->m_radius;
    hkVector4 aMinusB;
    aMinusB.m_quad = _mm_sub_ps(posA.m_quad, result.m_pointOnEdge.m_quad);
    const float refDist = input.m_tolerance + radiusSum;
    // lengthSquared3: z + (y + x), in SSE.
    const __m128 squares = _mm_mul_ps(aMinusB.m_quad, aMinusB.m_quad);
    const float distSquared = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(squares, squares, 0xAA), _mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares))
    );
    // Rounds the squared reference distance to a float before the compare.
    const float refDistSquared = refDist * refDist;
    if (distSquared >= refDistSquared) {
        return ST_CP_MISS;
    }

    float dist;
    if (distSquared > 0.0f) {
        dist = std::sqrt(distSquared);
        cpoint.m_separatingNormal.m_quad = aMinusB.m_quad;
    }
    else {
        dist = 0.0f;
        hkVector4 edge;
        edge.m_quad = _mm_sub_ps(posB[1].m_quad, posB[0].m_quad);
        hkVector4Util::calculatePerpendicularVector(edge, cpoint.m_separatingNormal);
    }
    // normalize3: the broadcast inverse length (approximate reciprocal square root, one Newton step, 0 for a zero
    // length) times the vector.
    const __m128 normal = cpoint.m_separatingNormal.m_quad;
    const __m128 normalSquares = _mm_mul_ps(normal, normal);
    const __m128 len2 = _mm_add_ss(
        _mm_shuffle_ps(normalSquares, normalSquares, 0xAA),
        _mm_add_ss(_mm_shuffle_ps(normalSquares, normalSquares, 0x55), normalSquares)
    );
    const __m128 approx = _mm_rsqrt_ss(len2);
    const __m128 len2IsZero = _mm_cmpeq_ss(len2, _mm_set_ss(0.0f));
    const __m128 refined = _mm_mul_ss(
        _mm_mul_ss(_mm_set_ss(0.5f), approx),
        _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx))
    );
    const __m128 lenInv = _mm_andnot_ps(len2IsZero, refined);
    cpoint.m_separatingNormal.m_quad = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, 0), normal);
    // The point on the capsule's surface: posA + normal * (capsule radius - dist); the distance in w.
    const float fromCentre = capsB->m_radius - dist;
    const __m128 scale = _mm_set_ss(fromCentre);
    cpoint.m_position.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0), cpoint.m_separatingNormal.m_quad),
        posA.m_quad
    );
    cpoint.m_separatingNormal(3) = dist - radiusSum;
    return ST_CP_HIT;
}

hkpCollisionAgent* hkpSphereCapsuleAgent::createSphereCapsuleAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    return new hkpSphereCapsuleAgent(mgr);
}

hkpCollisionAgent* hkpSphereCapsuleAgent::createCapsuleSphereAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    return new hkpSymmetricAgentLinearCast<hkpSphereCapsuleAgent>(mgr);
}
