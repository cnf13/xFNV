#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/CapsuleTriangle/hkpCapsuleTriangleAgent.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideCapsuleUtil.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideTriangleUtil.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"

#include <xmmintrin.h>

// The capsule's two end points and the triangle's three vertices are taken to world space by their bodies' transforms
// (the points done last to first, as the game does, x*col0 + y*col1 added to z*col2 + translation), then
// hkCollideCapsuleUtilCapsVsTri is called with the shapes' radii and input's tolerance.
void calcContactPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCollideTriangleUtil::PointTriangleDistanceCache& cache,
    int32_t searchManifold,
    hkContactPoint* points
) {
    const hkpCapsuleShape* capsule = static_cast<const hkpCapsuleShape*>(bodyA.getShape());
    const hkpTriangleShape* triangle = static_cast<const hkpTriangleShape*>(bodyB.getShape());
    hkVector4 capsPoints[2];
    hkVector4 triVertices[3];
    const hkTransform transformA = *bodyA.m_motion.pMotion;
    for (int32_t i = 1; i >= 0; --i) {
        const __m128 p = (&capsule->m_vertexA)[i].m_quad;
        capsPoints[i].m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), transformA.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(p, p, 0), transformA.m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), transformA.m_rotation.m_col2.m_quad),
                transformA.m_translation.m_quad
            )
        );
    }
    const hkTransform transformB = *bodyB.m_motion.pMotion;
    for (int32_t i = 2; i >= 0; --i) {
        const __m128 p = triangle->getVertex(i).m_quad;
        triVertices[i].m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), transformB.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(p, p, 0), transformB.m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), transformB.m_rotation.m_col2.m_quad),
                transformB.m_translation.m_quad
            )
        );
    }
    hkCollideCapsuleUtilCapsVsTri(
        capsPoints, capsule->m_radius, triVertices, triangle->m_radius, cache, input.m_tolerance, searchManifold,
        points, nullptr
    );
}

hkpCapsuleTriangleAgent::hkpCapsuleTriangleAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
)
  : hkpIterativeLinearCastAgent(mgr) {
    m_contactPointId[0] = HK_INVALID_CONTACT_POINT;
    m_contactPointId[1] = HK_INVALID_CONTACT_POINT;
    m_contactPointId[2] = HK_INVALID_CONTACT_POINT;
    const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
    hkpCollideTriangleUtil::setupPointTriangleDistanceCache(&triB->m_vertexA, m_triangleCache);
}

// Nothing of its own to free.
hkpCapsuleTriangleAgent::~hkpCapsuleTriangleAgent() {}

// Transforms the capsule A's end points and the triangle B's vertices into world space, gets the utility's three
// candidate points with the input's tolerance and a manifold search, then per point: one closer than the tolerance is
// welded by the triangle when it has features (rejected: its id is removed), gets an id from the contact manager when
// it has none, and is written to the output with its id; a point that stops touching has its id removed. Unlike the
// SDK, this welds before the id is added and does not test the input's welding flag, as the game does. A point is kept
// only when the tolerance is above the distance and the distance is not NaN. The timer macros write into the stream's
// raw buffer, which is treated as untyped (hence the casts).
void hkpCapsuleTriangleAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN("CapsuleTri", nullptr);
    const hkpCapsuleShape* capsuleA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
    const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);

    alignas(16) hkVector4 endPoints[2];
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &capsuleA->m_vertexA, 2, endPoints);
    }
    alignas(16) hkVector4 triVertices[3];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &triB->m_vertexA, 3, triVertices);
    }

    alignas(16) hkContactPoint points[3];
    hkpFeatureOutput featureOutputs[3];
    hkCollideCapsuleUtilCapsVsTri(
        endPoints, capsuleA->m_radius, triVertices, triB->m_radius, m_triangleCache, input.m_tolerance, 1, points,
        featureOutputs
    );

    for (int32_t p = 0; p < 3; p++) {
        if (!(input.m_tolerance > points[p].m_separatingNormal.m_quad[3])) {
            if (m_contactPointId[p] != 0xFFFF) {
                m_contactMgr->removeContactPoint(m_contactPointId[p], *result.m_constraintOwner.m_storage);
                m_contactPointId[p] = 0xFFFF;
            }
            continue;
        }
        // numFeatures is copied to a byte local, which the triangle's weldContactPoint takes by reference.
        uint8_t numFeatures = featureOutputs[p].numFeatures;
        if (numFeatures != 0) {
            alignas(16) hkVector4 weldedNormal;
            weldedNormal.m_quad = points[p].m_separatingNormal.m_quad;
            const int32_t iWeldResult = triB->weldContactPointImpl(
                featureOutputs[p].featureIds, numFeatures, points[p].m_position, bodyB.m_motion.pMotion, capsuleA,
                bodyA.m_motion.pMotion, weldedNormal
            );
            if (iWeldResult == hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT) {
                if (m_contactPointId[p] != 0xFFFF) {
                    m_contactMgr->removeContactPoint(m_contactPointId[p], *result.m_constraintOwner.m_storage);
                    m_contactPointId[p] = 0xFFFF;
                }
                continue;
            }
            if (iWeldResult == hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED) {
                points[p].setNormal(weldedNormal);
            }
        }
        if (m_contactPointId[p] == 0xFFFF) {
            m_contactPointId[p] = m_contactMgr->addContactPoint(bodyA, bodyB, input, result, nullptr, points[p]);
            if (m_contactPointId[p] == 0xFFFF) {
                continue;
            }
        }
        hkpProcessCdPoint* cp = result.m_firstFreeContactPoint.m_storage;
        result.m_firstFreeContactPoint.m_storage = cp + 1;
        // EXE-INLINED none: hkContactPoint::operator=
        cp->m_contact.m_position.m_quad = points[p].m_position.m_quad;
        cp->m_contact.m_separatingNormal.m_quad = points[p].m_separatingNormal.m_quad;
        cp->m_contactPointId.m_storage = m_contactPointId[p];
    }
    HK_TIMER_END();
}

// getClosestPoint's steps with the agent's own cache: the capsule A's end points and the triangle B's vertices in world
// space ((y*c1 + x*c0) + (z*c2 + t)), the utility's two candidate points, the nearer one (point 1 unless point 1 is
// further, so NaN picks point 1) when closer than the tolerance, welded by the triangle when the input asks for welding
// and the point has features, then reported to collector. The timer macros write into the stream's raw buffer, which is
// treated as untyped (hence the casts), as in hkpSphereTriangleAgent.
void hkpCapsuleTriangleAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("CapsTriangle", nullptr);
    hkpCdPoint event;
    event.m_cdBodyA = &bodyA;
    event.m_cdBodyB = &bodyB;
    const hkpCapsuleShape* capsA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
    const hkpConvexShape* triB = static_cast<const hkpConvexShape*>(bodyB.m_shape);

    hkVector4 capsAPoints[2];
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &capsA->m_vertexA, 2, capsAPoints);
    }
    hkVector4 triVertices[3];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &static_cast<const hkpTriangleShape*>(triB)->m_vertexA, 3, triVertices);
    }

    hkContactPoint points[3];
    hkpFeatureOutput features[3];
    hkCollideCapsuleUtilCapsVsTri(
        capsAPoints, capsA->m_radius, triVertices, triB->m_radius, m_triangleCache, input.m_tolerance, 0, points,
        features
    );

    const int32_t iPoint = points[1].m_separatingNormal.m_quad[3] > points[0].m_separatingNormal.m_quad[3] ? 0 : 1;
    if (input.m_tolerance > points[iPoint].m_separatingNormal.m_quad[3]) {
        uint8_t numFeatures = features[iPoint].numFeatures;
        int32_t iWeldResult = hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_UNMODIFIED;
        if (input.m_weldClosestPoints.m_storage != 0 && numFeatures > 0) {
            hkVector4 weldedNormal;
            _mm_store_ps(
                &reinterpret_cast<float*>(&weldedNormal.m_quad)[0],
                _mm_load_ps(&reinterpret_cast<float*>(&points[iPoint].m_separatingNormal.m_quad)[0])
            );
            iWeldResult = triB->weldContactPointImpl(
                features[iPoint].featureIds, numFeatures, points[iPoint].m_position, bodyB.m_motion.pMotion, capsA,
                bodyA.m_motion.pMotion, weldedNormal
            );
            if (iWeldResult == hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED) {
                points[iPoint].setNormal(weldedNormal);
            }
        }
        if (iWeldResult != hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT) {
            // EXE-INLINED none: hkContactPoint::operator=
            _mm_store_ps(
                &reinterpret_cast<float*>(&event.m_contact.m_position.m_quad)[0],
                _mm_load_ps(&reinterpret_cast<float*>(&points[iPoint].m_position.m_quad)[0])
            );
            _mm_store_ps(
                &reinterpret_cast<float*>(&event.m_contact.m_separatingNormal.m_quad)[0],
                _mm_load_ps(&reinterpret_cast<float*>(&points[iPoint].m_separatingNormal.m_quad)[0])
            );
            collector.addCdPoint(event);
        }
    }
    HK_TIMER_END();
}

// Uses the same world-space points as getClosestPoints (the triangle's _setTransformedPos here sums (x*c0 + y*c1) +
// (z*c2 + t)), the utility's two candidate points with the agent's cache and no features, and reports the pair when
// either point's distance is below zero (a NaN distance does not report). Timers as in getClosestPoints.
void hkpCapsuleTriangleAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN("CapsTriangle", nullptr);
    const hkpCapsuleShape* capsA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
    const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);

    hkVector4 capsAPoints[2];
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &capsA->m_vertexA, 2, capsAPoints);
    }
    hkVector4 triVertices[3];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &triB->m_vertexA, 3, triVertices);
    }

    hkContactPoint points[3];
    hkCollideCapsuleUtilCapsVsTri(
        capsAPoints, capsA->m_radius, triVertices, triB->m_radius, m_triangleCache, input.m_tolerance, 0, points,
        nullptr
    );
    if (0.0f > points[0].m_separatingNormal.m_quad[3] || 0.0f > points[1].m_separatingNormal.m_quad[3]) {
        collector.addCdBodyPair(bodyA, bodyB);
    }
    HK_TIMER_END();
}

void hkpCapsuleTriangleAgent::cleanup(hkpConstraintOwner& constraintOwner) {
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

// The capsule A's end points and the triangle B's vertices in world space, computed as (y*c1 + x*c0) + (z*c2 + t), with
// a local triangle cache, the utility's three candidate points, the nearer one (point 1 unless point 1 is further, so
// NaN picks point 1) when closer than the tolerance, welded by the triangle when the input asks for welding and the
// point has features, then reported to collector. The tolerance is compared against the chosen point in each arm of the
// distance test.
void hkpCapsuleTriangleAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("CapsTriangle", nullptr);
    const hkpCapsuleShape* capsA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
    const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
    hkpCollideTriangleUtil::PointTriangleDistanceCache cache;
    hkpCollideTriangleUtil::setupPointTriangleDistanceCache(&triB->m_vertexA, cache);

    hkpCdPoint event;
    event.m_cdBodyA = &bodyA;
    event.m_cdBodyB = &bodyB;

    alignas(16) hkVector4 capsAPoints[2];
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &capsA->m_vertexA, 2, capsAPoints);
    }
    alignas(16) hkVector4 triVertices[3];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &triB->m_vertexA, 3, triVertices);
    }

    alignas(16) hkContactPoint points[3];
    hkpFeatureOutput features[3];
    hkCollideCapsuleUtilCapsVsTri(
        capsAPoints, capsA->m_radius, triVertices, triB->m_radius, cache, input.m_tolerance, 0, points, features
    );

    const int32_t iPoint = points[1].m_separatingNormal.m_quad[3] > points[0].m_separatingNormal.m_quad[3] ? 0 : 1;
    if (input.m_tolerance > points[iPoint].m_separatingNormal.m_quad[3]) {
        uint8_t numFeatures = features[iPoint].numFeatures;
        bool reject = false;
        if (input.m_weldClosestPoints.m_storage != 0 && numFeatures != 0) {
            hkVector4 weldedNormal;
            _mm_store_ps(
                &reinterpret_cast<float*>(&weldedNormal.m_quad)[0],
                _mm_load_ps(&reinterpret_cast<float*>(&points[iPoint].m_separatingNormal.m_quad)[0])
            );
            const int32_t iWeldResult = triB->weldContactPointImpl(
                features[iPoint].featureIds, numFeatures, points[iPoint].m_position, bodyB.m_motion.pMotion, capsA,
                bodyA.m_motion.pMotion, weldedNormal
            );
            if (iWeldResult == hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT) {
                reject = true;
            }
            else if (iWeldResult == hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED) {
                points[iPoint].setNormal(weldedNormal);
            }
        }
        if (!reject) {
            // EXE-INLINED none: hkContactPoint::operator=
            _mm_store_ps(
                &reinterpret_cast<float*>(&event.m_contact.m_position.m_quad)[0],
                _mm_load_ps(&reinterpret_cast<float*>(&points[iPoint].m_position.m_quad)[0])
            );
            _mm_store_ps(
                &reinterpret_cast<float*>(&event.m_contact.m_separatingNormal.m_quad)[0],
                _mm_load_ps(&reinterpret_cast<float*>(&points[iPoint].m_separatingNormal.m_quad)[0])
            );
            collector.addCdPoint(event);
        }
    }
    HK_TIMER_END();
}

// Agent table entry, also called by the flipped agent. As getPenetrations, with a cache set up first from the
// triangle's local vertices.
void hkpCapsuleTriangleAgent::staticGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN("CapsTriangle", nullptr);
    hkpCollideTriangleUtil::PointTriangleDistanceCache cache;
    hkpCollideTriangleUtil::setupPointTriangleDistanceCache(
        &static_cast<const hkpTriangleShape*>(bodyB.m_shape)->m_vertexA, cache
    );
    const hkpCapsuleShape* capsA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
    const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);

    hkVector4 capsAPoints[2];
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &capsA->m_vertexA, 2, capsAPoints);
    }
    hkVector4 triVertices[3];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &triB->m_vertexA, 3, triVertices);
    }

    hkContactPoint points[3];
    hkCollideCapsuleUtilCapsVsTri(
        capsAPoints, capsA->m_radius, triVertices, triB->m_radius, cache, input.m_tolerance, 0, points, nullptr
    );
    if (0.0f > points[0].m_separatingNormal.m_quad[3] || 0.0f > points[1].m_separatingNormal.m_quad[3]) {
        collector.addCdBodyPair(bodyA, bodyB);
    }
    HK_TIMER_END();
}

// The flipped agent goes in first, for a triangle against a capsule; the two bool stores before each fill are the
// source's, though each init overwrites them.
// KEEP: partcopy. Of af's flag dword the exe writes only the two bool bytes, and registerCollisionAgent (0xCFBBE0)
// copies the whole AgentFuncs into the dispatcher, the two bytes nothing wrote included.
void hkpCapsuleTriangleAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs af;
    af.m_isFlipped.m_bool = false;
    af.m_isPredictive.m_bool = false;
    symmetricInitAgentFunc(af);
    dispatcher->registerCollisionAgent(af, HK_SHAPE_TRIANGLE, HK_SHAPE_CAPSULE);

    af.m_isFlipped.m_bool = false;
    af.m_isPredictive.m_bool = false;
    initAgentFunc(af);
    dispatcher->registerCollisionAgent(af, HK_SHAPE_CAPSULE, HK_SHAPE_TRIANGLE);
}

// The same two registrations, through the dispatcher's second agent table.
// KEEP: partcopy, as registerAgent: registerCollisionAgent2 copies the whole AgentFuncs as well.
void hkpCapsuleTriangleAgent::registerAgent2(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs af;
    af.m_isFlipped.m_bool = false;
    af.m_isPredictive.m_bool = false;
    symmetricInitAgentFunc(af);
    dispatcher->registerCollisionAgent2(af, HK_SHAPE_TRIANGLE, HK_SHAPE_CAPSULE);

    af.m_isFlipped.m_bool = false;
    af.m_isPredictive.m_bool = false;
    initAgentFunc(af);
    dispatcher->registerCollisionAgent2(af, HK_SHAPE_CAPSULE, HK_SHAPE_TRIANGLE);
}

void hkpCapsuleTriangleAgent::initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af) {
    af.m_createFunc = &hkpCapsuleTriangleAgent::createCapsuleTriangleAgent;
    af.m_getPenetrationsFunc = &hkpCapsuleTriangleAgent::staticGetPenetrations;
    af.m_getClosestPointFunc = &hkpCapsuleTriangleAgent::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpIterativeLinearCastAgent::staticLinearCast;
    af.m_isFlipped.m_bool = 0;
    af.m_isPredictive.m_bool = 0;
}

void hkpCapsuleTriangleAgent::symmetricInitAgentFunc(hkpCollisionDispatcher::AgentFuncs& af) {
    af.m_createFunc = &hkpCapsuleTriangleAgent::createTriangleCapsuleAgent;
    af.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>::staticGetPenetrations;
    af.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>::staticLinearCast;
    af.m_isFlipped.m_bool = 1;
    af.m_isPredictive.m_bool = 0;
}

// The capsule A's end points and the triangle B's vertices in world space, the capsule utility's two candidate points,
// the nearer one (point 1 unless point 1 is further, so NaN picks point 1) when it is closer than the collision
// tolerance, welded by the triangle when the input asks for welding and the point has features. The utility fills three
// feature entries.
hkpCapsuleTriangleAgent::ClosestPointResult hkpCapsuleTriangleAgent::getClosestPoint(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCollideTriangleUtil::PointTriangleDistanceCache& cache,
    hkContactPoint& cpoint
) {
    const hkpCapsuleShape* capsA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
    const hkpConvexShape* triB = static_cast<const hkpConvexShape*>(bodyB.m_shape);

    hkVector4 capsAPoints[2];
    {
        const hkTransform& t = *bodyA.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &capsA->m_vertexA, 2, capsAPoints);
    }
    hkVector4 triVertices[3];
    {
        const hkTransform& t = *bodyB.m_motion.pMotion;
        hkVector4Util::transformPoints(t, &static_cast<const hkpTriangleShape*>(triB)->m_vertexA, 3, triVertices);
    }

    hkContactPoint points[3];
    hkpFeatureOutput features[3];
    hkCollideCapsuleUtilCapsVsTri(
        capsAPoints, capsA->m_radius, triVertices, triB->m_radius, cache, input.m_tolerance, 0, points, features
    );

    const int32_t iPoint = points[1].m_separatingNormal.m_quad[3] > points[0].m_separatingNormal.m_quad[3] ? 0 : 1;
    if (!(input.m_tolerance > points[iPoint].m_separatingNormal.m_quad[3])) {
        return ST_CP_MISS;
    }

    uint8_t numFeatures = features[iPoint].numFeatures;
    if (input.m_weldClosestPoints.m_storage != 0 && numFeatures > 0) {
        hkVector4 weldedNormal;
        _mm_store_ps(
            &reinterpret_cast<float*>(&weldedNormal.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<float*>(&points[iPoint].m_separatingNormal.m_quad)[0])
        );
        const int32_t iWeldResult = triB->weldContactPointImpl(
            features[iPoint].featureIds, numFeatures, points[iPoint].m_position, bodyB.m_motion.pMotion, capsA,
            bodyA.m_motion.pMotion, weldedNormal
        );
        if (iWeldResult == hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT) {
            return ST_CP_MISS;
        }
        if (iWeldResult == hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED) {
            points[iPoint].setNormal(weldedNormal);
        }
    }
    // EXE-INLINED none: hkContactPoint::operator=
    _mm_store_ps(
        &reinterpret_cast<float*>(&cpoint.m_position.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<float*>(&points[iPoint].m_position.m_quad)[0])
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&cpoint.m_separatingNormal.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<float*>(&points[iPoint].m_separatingNormal.m_quad)[0])
    );
    return ST_CP_HIT;
}

// Does new hkpCapsuleTriangleAgent(bodyA, bodyB, input, mgr): the class allocator stores the size, then the constructor
// runs.
hkpCollisionAgent* hkpCapsuleTriangleAgent::createCapsuleTriangleAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    hkpCapsuleTriangleAgent* agent = static_cast<hkpCapsuleTriangleAgent*>(
        hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpCapsuleTriangleAgent))
    );
    agent->m_memSizeAndFlags = sizeof(hkpCapsuleTriangleAgent);
    return ::new (agent) hkpCapsuleTriangleAgent(bodyA, bodyB, input, mgr);
}

// new hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>(bodyA, bodyB, input, mgr): the agent's constructor on the
// swapped bodies (the cache from triangle A's vertices), then the flipped agent's vtable.
hkpCollisionAgent* hkpCapsuleTriangleAgent::createTriangleCapsuleAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    auto* agent = static_cast<hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>*>(
        hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>))
    );
    agent->m_memSizeAndFlags = sizeof(hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>);
    return ::new (agent) hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>(bodyA, bodyB, input, mgr);
}
