#include "Havok/Source/Physics/Collide/Agent/Deprecated/MultiSphereTriangle/hkpMultiSphereTriangleAgent.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
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
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/MultiSphere/hkpMultiSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"

#include <xmmintrin.h>

hkpMultiSphereTriangleAgent::hkpMultiSphereTriangleAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    hkpContactMgr* mgr
)
  : hkpIterativeLinearCastAgent(mgr) {
    for (uint16_t& id : m_contactPointId) {
        id = 0xFFFF;
    }
    const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
    hkpCollideTriangleUtil::setupClosestPointTriangleCache(&triB->m_vertexA, m_closestPointTriangleCache);
}

// The destructor is empty: the contact points are removed in cleanup(), nothing else to free.
hkpMultiSphereTriangleAgent::~hkpMultiSphereTriangleAgent() {}

// Reports the body pair once the first sphere overlaps the triangle (radius sum above the distance; not NaN). Uses the
// agent's own triangle cache without setting it up. The sphere count is checked before the loop, so with m_numSpheres
// <= 0 no sphere is read.
void hkpMultiSphereTriangleAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    const hkpMultiSphereShape* msShape = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
    const hkpTriangleShape* triShape = static_cast<const hkpTriangleShape*>(bodyB.m_shape);

    alignas(16) hkVector4 triVertices[3];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &triShape->m_vertexA, 3, triVertices);
    }

    alignas(16) hkVector4 worldSpheres[hkpMultiSphereShape::MAX_SPHERES];
    const int32_t numSpheres = msShape->m_numSpheres;
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        for (int32_t i = numSpheres - 1; i >= 0; i--) {
            worldSpheres[i]._setTransformedPos(t, msShape->m_spheres[i]);
        }
    }

    for (int32_t i = 0; i < numSpheres; i++) {
        const float radiusSum = triShape->m_radius + msShape->m_spheres[i].m_quad[3];
        hkpCollideTriangleUtil::ClosestPointTriangleResult result;
        hkpCollideTriangleUtil::closestPointTriangle(
            worldSpheres[i], triVertices, m_closestPointTriangleCache, result, nullptr
        );
        // The pair is reported when radiusSum > distance and neither is NaN.
        if (radiusSum > result.distance) {
            collector.addCdBodyPair(bodyA, bodyB);
            return;
        }
    }
}

// Does what staticGetClosestPoints does, but with the agent's own triangle cache, which the agent does not set up.
void hkpMultiSphereTriangleAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("MultiSphereTriangle", nullptr);
    const hkpMultiSphereShape* msShape = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
    const hkpTriangleShape* triShape = static_cast<const hkpTriangleShape*>(bodyB.m_shape);

    alignas(16) hkVector4 triVertices[3];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &triShape->m_vertexA, 3, triVertices);
    }

    alignas(16) hkVector4 worldSpheres[hkpMultiSphereShape::MAX_SPHERES];
    const int32_t numSpheres = msShape->m_numSpheres;
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        for (int32_t i = numSpheres - 1; i >= 0; i--) {
            worldSpheres[i]._setTransformedPos(t, msShape->m_spheres[i]);
        }
    }

    hkpCdPoint event;
    event.m_cdBodyA = &bodyA;
    event.m_cdBodyB = &bodyB;
    for (int32_t i = 0; i < numSpheres; i++) {
        const float radiusSum = triShape->m_radius + msShape->m_spheres[i].m_quad[3];
        hkpCollideTriangleUtil::ClosestPointTriangleResult result;
        hkpCollideTriangleUtil::closestPointTriangle(
            worldSpheres[i], triVertices, m_closestPointTriangleCache, result, nullptr
        );
        const float distance = result.distance;
        // The limit is stored as a float before the compare.
        const float limit = input.m_tolerance + radiusSum;
        // Only limit > distance (not NaN) reports the point.
        if (limit > distance) {
            const float scale = triShape->m_radius - distance;
            const __m128 hitDirection = _mm_load_ps(&reinterpret_cast<float*>(&result.hitDirection.m_quad)[0]);
            const __m128 position = _mm_add_ps(
                _mm_mul_ps(_mm_set1_ps(scale), hitDirection),
                _mm_load_ps(&reinterpret_cast<float*>(&worldSpheres[i].m_quad)[0])
            );
            _mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_separatingNormal.m_quad)[0], hitDirection);
            event.m_contact.m_separatingNormal.m_quad[3] = distance - radiusSum;
            _mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_position.m_quad)[0], position);
            collector.addCdPoint(event);
        }
    }
    HK_TIMER_END();
}

void hkpMultiSphereTriangleAgent::linearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    hkpIterativeLinearCastAgent::linearCast(bodyA, bodyB, input, collector, startCollector);
}

// Adds or keeps a contact point per sphere that lies within the tolerance of the triangle, and removes the points of
// the others. Uses the agent's own triangle cache without setting it up. It walks the spheres (and their world
// positions) forward but the contact point ids backward, from m_contactPointId[numSpheres - 1]: sphere k uses id
// numSpheres - 1 - k.
void hkpMultiSphereTriangleAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN("MultiSphereTri", nullptr);
    const hkpTriangleShape* triShape = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
    const hkpMultiSphereShape* msShape = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);

    alignas(16) hkVector4 triVertices[3];
    {
        const hkTransform& t = bodyB.m_motion.pState->m_transform;
        hkVector4Util::transformPoints(t, &triShape->m_vertexA, 3, triVertices);
    }

    alignas(16) hkVector4 worldSpheres[hkpMultiSphereShape::MAX_SPHERES];
    const int32_t numSpheres = msShape->m_numSpheres;
    {
        const hkTransform& t = bodyA.m_motion.pState->m_transform;
        hkVector4Util::transformPoints(t, msShape->m_spheres, numSpheres, worldSpheres);
    }

    const hkVector4* position = worldSpheres;
    const hkVector4* sphere = msShape->m_spheres;
    uint16_t* contactPointId = &m_contactPointId[numSpheres - 1];
    for (int32_t i = numSpheres - 1; i >= 0; i--) {
        const float radiusSum = sphere->m_quad[3] + triShape->m_radius;
        hkpCollideTriangleUtil::ClosestPointTriangleResult cpt;
        hkpCollideTriangleUtil::closestPointTriangle(*position, triVertices, m_closestPointTriangleCache, cpt, nullptr);
        const float distance = cpt.distance;
        // The limit is stored as a float before the compare.
        const float limit = radiusSum + input.m_tolerance;
        // Only limit > distance (not NaN) makes a contact point.
        if (limit > distance) {
            hkpProcessCdPoint* point = result.m_firstFreeContactPoint.m_storage;
            const float scale = triShape->m_radius - distance;
            const __m128 hitDirection = cpt.hitDirection.m_quad;
            point->m_contact.m_position.m_quad =
                _mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), hitDirection), position->m_quad);
            point->m_contact.m_separatingNormal.m_quad = cpt.hitDirection.m_quad;
            point->m_contact.m_separatingNormal.m_quad[3] = distance - radiusSum;
            bool added = true;
            if (*contactPointId == 0xFFFF) {
                *contactPointId =
                    m_contactMgr->addContactPointImpl(bodyA, bodyB, input, result, nullptr, point->m_contact);
                added = *contactPointId != 0xFFFF;
            }
            if (added) {
                result.m_firstFreeContactPoint.m_storage++;
                point->m_contactPointId.m_storage = *contactPointId;
            }
        }
        else if (*contactPointId != 0xFFFF) {
            m_contactMgr->removeContactPointImpl(*contactPointId, *result.m_constraintOwner.m_storage);
            *contactPointId = 0xFFFF;
        }
        position++;
        sphere++;
        contactPointId--;
    }
    HK_TIMER_END();
}

void hkpMultiSphereTriangleAgent::cleanup(hkpConstraintOwner& constraintOwner) {
    uint16_t* m_contactPointId = this->m_contactPointId;
    int32_t v4 = 8;
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

// getPenetrations without an agent: the same test with a triangle cache of its own, set up from the triangle's local
// vertices (not the transformed ones).
void hkpMultiSphereTriangleAgent::staticGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    const hkpMultiSphereShape* msShape = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
    const hkpTriangleShape* triShape = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
    const hkVector4* vertices = &triShape->m_vertexA;

    alignas(16) hkVector4 triVertices[3];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        hkVector4Util::transformPoints(t, vertices, 3, triVertices);
    }

    alignas(16) hkVector4 worldSpheres[hkpMultiSphereShape::MAX_SPHERES];
    const int32_t numSpheres = msShape->m_numSpheres;
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        for (int32_t i = numSpheres - 1; i >= 0; i--) {
            worldSpheres[i]._setTransformedPos(t, msShape->m_spheres[i]);
        }
    }

    hkpCollideTriangleUtil::ClosestPointTriangleCache cache;
    hkpCollideTriangleUtil::setupClosestPointTriangleCache(vertices, cache);

    for (int32_t i = 0; i < numSpheres; i++) {
        const float radiusSum = triShape->m_radius + msShape->m_spheres[i].m_quad[3];
        hkpCollideTriangleUtil::ClosestPointTriangleResult result;
        hkpCollideTriangleUtil::closestPointTriangle(worldSpheres[i], triVertices, cache, result, nullptr);
        // The pair is reported when radiusSum > distance and neither is NaN.
        if (radiusSum > result.distance) {
            collector.addCdBodyPair(bodyA, bodyB);
            return;
        }
    }
}

// The triangle and the spheres are transformed with the products summed as (y*c1 + x*c0) + (z*c2 + t). The triangle
// cache is set up from the shape's local vertices, not the transformed ones.
void hkpMultiSphereTriangleAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("MultiSphereTriangle", nullptr);
    const hkpMultiSphereShape* msShape = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
    const hkpTriangleShape* triShape = static_cast<const hkpTriangleShape*>(bodyB.m_shape);

    alignas(16) hkVector4 triVertices[3];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &triShape->m_vertexA, 3, triVertices);
    }

    alignas(16) hkVector4 worldSpheres[hkpMultiSphereShape::MAX_SPHERES];
    const int32_t numSpheres = msShape->m_numSpheres;
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        hkVector4Util::transformPoints(t, msShape->m_spheres, numSpheres, worldSpheres);
    }

    hkpCdPoint event;
    event.m_cdBodyA = &bodyA;
    event.m_cdBodyB = &bodyB;
    hkpCollideTriangleUtil::ClosestPointTriangleCache cache;
    hkpCollideTriangleUtil::setupClosestPointTriangleCache(&triShape->m_vertexA, cache);

    for (int32_t i = 0; i < numSpheres; i++) {
        const float radiusSum = msShape->m_spheres[i].m_quad[3] + triShape->m_radius;
        hkpCollideTriangleUtil::ClosestPointTriangleResult result;
        hkpCollideTriangleUtil::closestPointTriangle(worldSpheres[i], triVertices, cache, result, nullptr);
        const float distance = result.distance;
        // The limit is stored as a float before the compare.
        const float limit = input.m_tolerance + radiusSum;
        // Only limit > distance (not NaN) reports the point.
        if (limit > distance) {
            const float scale = triShape->m_radius - distance;
            const __m128 hitDirection = _mm_load_ps(&reinterpret_cast<float*>(&result.hitDirection.m_quad)[0]);
            const __m128 position = _mm_add_ps(
                _mm_mul_ps(_mm_set1_ps(scale), hitDirection),
                _mm_load_ps(&reinterpret_cast<float*>(&worldSpheres[i].m_quad)[0])
            );
            _mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_separatingNormal.m_quad)[0], hitDirection);
            event.m_contact.m_separatingNormal.m_quad[3] = distance - radiusSum;
            _mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_position.m_quad)[0], position);
            collector.addCdPoint(event);
        }
    }
    HK_TIMER_END();
}

// One AgentFuncs on the stack is filled twice: the flipped agent (triangle against multi-sphere), then the agent
// itself; neither predicts. The flipped table's linear cast is
// hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>::staticLinearCast, which the flipped agents share.
// registerCollisionAgent copies f's flags, so the two padding bytes after m_isPredictive, never written here, land in
// the dispatcher's table; this uninitialised read is deliberate, as the game does.
void hkpMultiSphereTriangleAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_createFunc = &hkpMultiSphereTriangleAgent::createTriangleMultiSphereAgent;
    f.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>::staticLinearCast;
    f.m_isFlipped.m_bool = 1;
    f.m_isPredictive.m_bool = 0;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_TRIANGLE, HK_SHAPE_MULTI_SPHERE);

    f.m_createFunc = &hkpMultiSphereTriangleAgent::createMultiSphereTriangleAgent;
    f.m_getPenetrationsFunc = &hkpMultiSphereTriangleAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpMultiSphereTriangleAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpIterativeLinearCastAgent::staticLinearCast;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 0;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_MULTI_SPHERE, HK_SHAPE_TRIANGLE);
}

// A new agent for a multi-sphere (bodyA) against a triangle (bodyB): the class allocator's operator new, then the
// constructor.
hkpCollisionAgent* hkpMultiSphereTriangleAgent::createMultiSphereTriangleAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* contactMgr
) {
    hkMemoryRouter* router = &hkMemoryRouter::getInstance();
    hkpMultiSphereTriangleAgent* agent =
        static_cast<hkpMultiSphereTriangleAgent*>(router->m_heap->blockAlloc(sizeof(hkpMultiSphereTriangleAgent)));
    agent->m_memSizeAndFlags = sizeof(hkpMultiSphereTriangleAgent);
    return ::new (agent) hkpMultiSphereTriangleAgent(bodyA, bodyB, contactMgr);
}

// The flipped agent for a triangle (bodyA) against a multi-sphere (bodyB): the agent's constructor on the swapped
// pair, then the flipped agent's vtable.
hkpCollisionAgent* hkpMultiSphereTriangleAgent::createTriangleMultiSphereAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* contactMgr
) {
    hkMemoryRouter* router = &hkMemoryRouter::getInstance();
    hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>* agent =
        static_cast<hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>*>(
            router->m_heap->blockAlloc(sizeof(hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>))
        );
    agent->m_memSizeAndFlags = sizeof(hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>);
    return ::new (agent) hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>(bodyA, bodyB, input, contactMgr);
}
