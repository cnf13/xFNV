#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/CapsuleCapsule/hkpCapsuleCapsuleAgent.h"

#include "Havok/Source/Common/Base/Algorithm/Collide/LineSegment/hkLineSegmentUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
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
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideCapsuleUtil.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"

#include <xmmintrin.h>

// Core of staticGetClosestPoints: transforms the end points of both capsules (y*c1 + x*c0, the end points from the last
// down), then computes the closest point.
hkResult getClosestPointCapsuleCapsule(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkContactPoint& cpoint
) {
    const hkpCapsuleShape* capsA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
    const hkpCapsuleShape* capsB = static_cast<const hkpCapsuleShape*>(bodyB.m_shape);

    alignas(16) hkVector4 pointsA[2];
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        const __m128 c0 = t.m_rotation.m_col0.m_quad;
        const __m128 c1 = t.m_rotation.m_col1.m_quad;
        const __m128 c2 = t.m_rotation.m_col2.m_quad;
        const __m128 c3 = t.m_translation.m_quad;
        const hkVector4* vertices = &capsA->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            const __m128 p = vertices[i].m_quad;
            // EXE-INLINED none: hkVector4::_setTransformedPos
            const __m128 r = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3)
            );
            pointsA[i].m_quad = r;
        }
    }
    alignas(16) hkVector4 pointsB[2];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        const __m128 c0 = t.m_rotation.m_col0.m_quad;
        const __m128 c1 = t.m_rotation.m_col1.m_quad;
        const __m128 c2 = t.m_rotation.m_col2.m_quad;
        const __m128 c3 = t.m_translation.m_quad;
        const hkVector4* vertices = &capsB->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            const __m128 p = vertices[i].m_quad;
            // EXE-INLINED none: hkVector4::_setTransformedPos
            const __m128 r = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3)
            );
            pointsB[i].m_quad = r;
        }
    }

    const float tolerance = input.m_tolerance;
    return hkCollideCapsuleUtilClostestPointCapsVsCaps(
        pointsA, capsA->m_radius, pointsB, capsB->m_radius, tolerance, cpoint
    );
}

// The tolerance is held as a float local and copied into each contact's distance; the radii are passed to the manifold
// function as float copies.
int32_t
getManifold(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input, hkContactPoint* points) {
    const hkpCapsuleShape* capsA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
    {
        const float tolerance = input.m_tolerance;
        points[0].m_separatingNormal(3) = tolerance;
        points[1].m_separatingNormal(3) = tolerance;
        points[2].m_separatingNormal(3) = tolerance;
    }
    const hkpCapsuleShape* capsB = static_cast<const hkpCapsuleShape*>(bodyB.m_shape);

    alignas(16) hkVector4 pointsA[2];
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        const __m128 c0 = t.m_rotation.m_col0.m_quad;
        const __m128 c1 = t.m_rotation.m_col1.m_quad;
        const __m128 c2 = t.m_rotation.m_col2.m_quad;
        const __m128 c3 = t.m_translation.m_quad;
        const hkVector4* vertices = &capsA->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            const __m128 p = vertices[i].m_quad;
            // EXE-INLINED none: hkVector4::_setTransformedPos
            const __m128 r = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3)
            );
            pointsA[i].m_quad = r;
        }
    }
    alignas(16) hkVector4 pointsB[2];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        const __m128 c0 = t.m_rotation.m_col0.m_quad;
        const __m128 c1 = t.m_rotation.m_col1.m_quad;
        const __m128 c2 = t.m_rotation.m_col2.m_quad;
        const __m128 c3 = t.m_translation.m_quad;
        const hkVector4* vertices = &capsB->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            const __m128 p = vertices[i].m_quad;
            // EXE-INLINED none: hkVector4::_setTransformedPos
            const __m128 r = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3)
            );
            pointsB[i].m_quad = r;
        }
    }

    return hkCollideCapsuleUtilManifoldCapsVsCaps(pointsA, capsA->m_radius, pointsB, capsB->m_radius, points);
}

hkpCapsuleCapsuleAgent::hkpCapsuleCapsuleAgent(hkpContactMgr* mgr) : hkpIterativeLinearCastAgent(mgr) {
    m_contactPointId[0] = HK_INVALID_CONTACT_POINT;
    m_contactPointId[1] = HK_INVALID_CONTACT_POINT;
    m_contactPointId[2] = HK_INVALID_CONTACT_POINT;
}

// Nothing of its own to free.
hkpCapsuleCapsuleAgent::~hkpCapsuleCapsuleAgent() {}

// The tolerance is copied into each contact's distance. Each contact closer than the tolerance (tolerance above the
// distance, ordered compare) is kept or added; the others are removed.
void hkpCapsuleCapsuleAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN("CapsCaps", nullptr);

    const hkpCapsuleShape* capsA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
    const hkpCapsuleShape* capsB = static_cast<const hkpCapsuleShape*>(bodyB.m_shape);

    hkContactPoint contactPoints[3];
    {
        const float tolerance = input.m_tolerance;
        contactPoints[0].m_separatingNormal(3) = tolerance;
        contactPoints[1].m_separatingNormal(3) = tolerance;
        contactPoints[2].m_separatingNormal(3) = tolerance;
    }

    alignas(16) hkVector4 pointsA[2];
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        const __m128 c0 = t.m_rotation.m_col0.m_quad;
        const __m128 c1 = t.m_rotation.m_col1.m_quad;
        const __m128 c2 = t.m_rotation.m_col2.m_quad;
        const __m128 c3 = t.m_translation.m_quad;
        const hkVector4* vertices = &capsA->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            const __m128 p = vertices[i].m_quad;
            // EXE-INLINED none: hkVector4::_setTransformedPos
            const __m128 r = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3)
            );
            pointsA[i].m_quad = r;
        }
    }
    alignas(16) hkVector4 pointsB[2];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        const __m128 c0 = t.m_rotation.m_col0.m_quad;
        const __m128 c1 = t.m_rotation.m_col1.m_quad;
        const __m128 c2 = t.m_rotation.m_col2.m_quad;
        const __m128 c3 = t.m_translation.m_quad;
        const hkVector4* vertices = &capsB->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            const __m128 p = vertices[i].m_quad;
            // EXE-INLINED none: hkVector4::_setTransformedPos
            const __m128 r = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3)
            );
            pointsB[i].m_quad = r;
        }
    }

    hkCollideCapsuleUtilManifoldCapsVsCaps(pointsA, capsA->m_radius, pointsB, capsB->m_radius, contactPoints);

    hkContactPoint* cp = contactPoints;
    uint16_t* contactPointId = m_contactPointId;
    for (int32_t n = 3; n != 0; n--) {
        if (input.m_tolerance > cp->m_separatingNormal(3)) {
            if (*contactPointId == 0xFFFF) {
                *contactPointId = m_contactMgr->addContactPoint(bodyA, bodyB, input, result, nullptr, *cp);
            }
            if (*contactPointId != 0xFFFF) {
                hkpProcessCdPoint* point = result.m_firstFreeContactPoint.m_storage;
                result.m_firstFreeContactPoint.m_storage = point + 1;
                point->m_contact.m_position.m_quad = cp->m_position.m_quad;
                point->m_contact.m_separatingNormal.m_quad = cp->m_separatingNormal.m_quad;
                point->m_contactPointId.m_storage = *contactPointId;
            }
        }
        else if (*contactPointId != 0xFFFF) {
            m_contactMgr->removeContactPoint(*contactPointId, *result.m_constraintOwner.m_storage);
            *contactPointId = 0xFFFF;
        }
        ++contactPointId;
        ++cp;
    }

    HK_TIMER_END();
}

// EXE-INLINED 0xD3BD80
void hkpCapsuleCapsuleAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("CapsCaps", nullptr);

    const hkpCapsuleShape* capsA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
    const hkpCapsuleShape* capsB = static_cast<const hkpCapsuleShape*>(bodyB.m_shape);

    hkpCdPoint event;
    event.m_cdBodyA = &bodyA;
    event.m_cdBodyB = &bodyB;

    alignas(16) hkVector4 pointsA[2];
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        const __m128 c0 = t.m_rotation.m_col0.m_quad;
        const __m128 c1 = t.m_rotation.m_col1.m_quad;
        const __m128 c2 = t.m_rotation.m_col2.m_quad;
        const __m128 c3 = t.m_translation.m_quad;
        const hkVector4* vertices = &capsA->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            const __m128 p = vertices[i].m_quad;
            // EXE-INLINED none: hkVector4::_setTransformedPos
            const __m128 r = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3)
            );
            pointsA[i].m_quad = r;
        }
    }
    alignas(16) hkVector4 pointsB[2];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        const __m128 c0 = t.m_rotation.m_col0.m_quad;
        const __m128 c1 = t.m_rotation.m_col1.m_quad;
        const __m128 c2 = t.m_rotation.m_col2.m_quad;
        const __m128 c3 = t.m_translation.m_quad;
        const hkVector4* vertices = &capsB->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            const __m128 p = vertices[i].m_quad;
            // EXE-INLINED none: hkVector4::_setTransformedPos
            const __m128 r = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0), _mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3)
            );
            pointsB[i].m_quad = r;
        }
    }

    if (hkCollideCapsuleUtilClostestPointCapsVsCaps(
            pointsA, capsA->m_radius, pointsB, capsB->m_radius, input.m_tolerance, event.m_contact
        ) == HK_SUCCESS) {
        collector.addCdPoint(event);
    }

    HK_TIMER_END();
}

void hkpCapsuleCapsuleAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    hkpCapsuleCapsuleAgent::staticGetPenetrations(bodyA, bodyB, input, collector);
}

void hkpCapsuleCapsuleAgent::cleanup(hkpConstraintOwner& constraintOwner) {
    uint16_t* m_contactPointId = this->m_contactPointId;
    int32_t v4 = 3;
    do {
        if (*m_contactPointId != 0xFFFF) {
            m_contactMgr->removeContactPointImpl(*m_contactPointId, constraintOwner);
        }
        ++m_contactPointId;
        --v4;
    }
    while (v4);
    if (this) {
        delete this;
    }
}

// The table holds the agent's create, penetration and closest points functions and the iterative linear cast; the agent
// is neither flipped nor predictive. Only m_isFlipped and m_isPredictive are written, and registerCollisionAgent copies
// the whole flags word, so the two padding bytes after m_isPredictive, never written, land in the dispatcher's table:
// an uninitialised read, as the game does.
void hkpCapsuleCapsuleAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 0;
    f.m_createFunc = &hkpCapsuleCapsuleAgent::createCapsuleCapsuleAgent;
    f.m_getPenetrationsFunc = &hkpCapsuleCapsuleAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpCapsuleCapsuleAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpIterativeLinearCastAgent::staticLinearCast;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_CAPSULE, HK_SHAPE_CAPSULE);
}

// Both capsules' end points are transformed as (y*c1 + x*c0) + (z*c2 + t) using SSE intrinsics. The timer macros write
// into the stream's raw buffer, which is treated as untyped (hence the casts), as in hkpBvTreeAgent.
void hkpCapsuleCapsuleAgent::staticGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN("CapsCaps", nullptr);
    const hkpCapsuleShape* capsA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
    const hkpCapsuleShape* capsB = static_cast<const hkpCapsuleShape*>(bodyB.m_shape);

    alignas(16) hkVector4 pointsA[2];
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
        const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
        const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
        const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
        const hkVector4* vertices = &capsA->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&vertices[i].m_quad)[0]);
            // EXE-INLINED none: hkVector4::_setTransformedPos
            const __m128 r = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3)
            );
            _mm_store_ps(&reinterpret_cast<float*>(&pointsA[i].m_quad)[0], r);
        }
    }
    alignas(16) hkVector4 pointsB[2];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
        const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
        const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
        const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
        const hkVector4* vertices = &capsB->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&vertices[i].m_quad)[0]);
            // EXE-INLINED none: hkVector4::_setTransformedPos
            const __m128 r = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3)
            );
            _mm_store_ps(&reinterpret_cast<float*>(&pointsB[i].m_quad)[0], r);
        }
    }

    alignas(16) hkVector4 dA;
    dA.setSub4(pointsA[1], pointsA[0]);
    alignas(16) hkVector4 dB;
    dB.setSub4(pointsB[1], pointsB[0]);
    alignas(16) hkLineSegmentUtil::ClosestLineSegLineSegResult result;
    hkLineSegmentUtil::closestLineSegLineSeg(pointsA[0], dA, pointsB[0], dB, result);

    // The sum is a float local. The pair is reported only when the squared distance is below its square (not when
    // equal, above or NaN).
    const float radiusSum = capsB->m_radius + capsA->m_radius;
    if (result.m_distanceSquared < radiusSum * radiusSum) {
        collector.addCdBodyPair(bodyA, bodyB);
    }
    HK_TIMER_END();
}

hkpCollisionAgent* hkpCapsuleCapsuleAgent::createCapsuleCapsuleAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    hkpCapsuleCapsuleAgent* agent = static_cast<hkpCapsuleCapsuleAgent*>(
        hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpCapsuleCapsuleAgent))
    );
    agent->m_memSizeAndFlags = sizeof(hkpCapsuleCapsuleAgent);
    return ::new (agent) hkpCapsuleCapsuleAgent(mgr);
}

// Transforms both capsules' end points with hkVector4Util::transformPoints, and when the capsule test succeeds builds
// the hkpCdPoint from the two bodies and passes it to the collector's addCdPoint.
void hkpCapsuleCapsuleAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("CapsCaps", nullptr);

    const hkpCapsuleShape* capsA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
    const hkpCapsuleShape* capsB = static_cast<const hkpCapsuleShape*>(bodyB.m_shape);

    hkpCdPoint event;
    event.m_cdBodyA = &bodyA;
    event.m_cdBodyB = &bodyB;

    alignas(16) hkVector4 pointsA[2];
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        const __m128 c0 = t.m_rotation.m_col0.m_quad;
        const __m128 c1 = t.m_rotation.m_col1.m_quad;
        const __m128 c2 = t.m_rotation.m_col2.m_quad;
        const __m128 c3 = t.m_translation.m_quad;
        const hkVector4* vertices = &capsA->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            const __m128 p = vertices[i].m_quad;
            // EXE-INLINED none: hkVector4::_setTransformedPos
            const __m128 r = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3)
            );
            pointsA[i].m_quad = r;
        }
    }
    alignas(16) hkVector4 pointsB[2];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        const __m128 c0 = t.m_rotation.m_col0.m_quad;
        const __m128 c1 = t.m_rotation.m_col1.m_quad;
        const __m128 c2 = t.m_rotation.m_col2.m_quad;
        const __m128 c3 = t.m_translation.m_quad;
        const hkVector4* vertices = &capsB->m_vertexA;
        for (int32_t i = 1; i >= 0; i--) {
            const __m128 p = vertices[i].m_quad;
            // EXE-INLINED none: hkVector4::_setTransformedPos
            const __m128 r = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3)
            );
            pointsB[i].m_quad = r;
        }
    }

    if (hkCollideCapsuleUtilClostestPointCapsVsCaps(
            pointsA, capsA->m_radius, pointsB, capsB->m_radius, input.m_tolerance, event.m_contact
        ) == HK_SUCCESS) {
        collector.addCdPoint(event);
    }

    HK_TIMER_END();
}
