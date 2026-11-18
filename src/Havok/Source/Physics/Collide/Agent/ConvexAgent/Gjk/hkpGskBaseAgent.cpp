#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskBaseAgent.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGsk.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskfAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgentConfig.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpSimpleClosestContactCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"

#include <xmmintrin.h>

// The GSK cache for the pair (btoa places B in A's space), no separating normal yet (time and w -1), and the
// allowed penetration the smaller of the two root collidables'.
hkpGskBaseAgent::hkpGskBaseAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, hkpContactMgr* mgr)
  : hkpIterativeLinearCastAgent(mgr), m_timeOfSeparatingNormal(-1.0f) {
    const hkpConvexShape* shapeB = static_cast<const hkpConvexShape*>(bodyB.m_shape);
    const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
    hkTransform btoa;
    btoa.setMulInverseMul(*bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion);
    if (shapeB->m_type == HK_SHAPE_TRIANGLE) {
        m_cache.initTriangle(shapeA, static_cast<const hkpTriangleShape*>(shapeB), btoa);
    }
    else {
        m_cache.init(shapeA, shapeB, btoa);
    }
    m_separatingNormal.m_quad = _mm_setzero_ps();
    m_separatingNormal.m_quad[3] = -1.0f;
    const float penetrationA = bodyA.getRootCollidable()->getAllowedPenetrationDepth();
    const float penetrationB = bodyB.getRootCollidable()->getAllowedPenetrationDepth();
    m_allowedPenetration = penetrationB > penetrationA ? penetrationA : penetrationB;
}

// Nothing of its own to free.
hkpGskBaseAgent::~hkpGskBaseAgent() {}

// Havok SDK: runs GSK from the agent's cache (exporting the cache back when the simplex changed) and reports the pair
// when GSK fails (the cached normal then zero) or when the distance less both radii is negative; on success the
// cached normal is the separating normal in world space with that distance as w.
void hkpGskBaseAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN_LIST("Gsk", "tim");
    HK_TIMER_SPLIT_LIST("SepNormal");
    const hkpConvexShape* shapeB = static_cast<const hkpConvexShape*>(bodyB.m_shape);
    const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
    hkTransform aTb;
    aTb.setMulInverseMul(*bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion);

    // hkpGsk::init, with hkPadSpu's stores and loads written as plain field accesses.
    hkpGsk gsk;
    gsk.m_dimA.m_storage = m_cache.m_dimA;
    gsk.m_dimB.m_storage = m_cache.m_dimB;
    gsk.m_maxDimA.m_storage = m_cache.m_maxDimA;
    gsk.m_maxDimB.m_storage = m_cache.m_maxDimB;
    gsk.m_doNotHandlePenetration.m_storage = 1;
    gsk.m_featureChange.m_storage = 0;
    shapeA->convertVertexIdsToVerticesImpl(&m_cache.m_vertices[0], gsk.m_dimA.m_storage, &gsk.m_verticesA[0]);
    shapeB->convertVertexIdsToVerticesImpl(
        &m_cache.m_vertices[gsk.m_dimA.m_storage], gsk.m_dimB.m_storage, &gsk.m_verticesBinB[0]
    );

    hkVector4 separatingNormal;
    hkpGskStatus gjkStatus = gsk.getClosestFeature(shapeA, shapeB, aTb, separatingNormal);
    // Body of hkpGsk::exitAndExportCache.
    if (gsk.m_featureChange.m_storage) {
        gsk.exitAndExportCacheImpl(m_cache);
    }
    if (gjkStatus != HK_GSK_OK) {
        m_separatingNormal.m_quad = _mm_setzero_ps();
        collector.addCdBodyPair(bodyA, bodyB);
    }
    else {
        // The distance is rounded to a float before it is stored and compared.
        float distance = separatingNormal.m_quad[3] - shapeA->m_radius - shapeB->m_radius;
        const hkRotation& rotationA = bodyA.m_motion.pState->m_transform.m_rotation;
        const hkQuadReal n = separatingNormal.m_quad;
        m_separatingNormal.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(n, n, 85), rotationA.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(n, n, 0), rotationA.m_col0.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(n, n, 170), rotationA.m_col2.m_quad)
        );
        m_separatingNormal.m_quad[3] = distance;
        if (distance < 0.0f) {
            collector.addCdBodyPair(bodyA, bodyB);
        }
    }
    HK_TIMER_END_LIST();
}

// Havok SDK: runs GSK from the agent's cache for the pair and reports the closest point it finds, with the separating
// normal, between bodyA and bodyB.
void hkpGskBaseAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("Gsk", nullptr);
    hkTransform aTb;
    aTb.setMulInverseMul(*bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion);
    hkpGsk::GetClosesetPointInput gskInput;
    gskInput.m_shapeA.m_storage = static_cast<const hkpConvexShape*>(bodyA.m_shape);
    gskInput.m_shapeB.m_storage = static_cast<const hkpConvexShape*>(bodyB.m_shape);
    gskInput.m_transformA.m_storage = bodyA.m_motion.pMotion;
    gskInput.m_aTb.m_storage = &aTb;
    gskInput.m_collisionTolerance.m_storage = input.m_tolerance;
    hkVector4 separatingNormal;
    hkVector4 pointOnB;
    if (hkpGsk::getClosestPoint(gskInput, m_cache, separatingNormal, pointOnB) == HK_SUCCESS) {
        hkpCdPoint event;
        event.m_contact.m_position.m_quad = pointOnB.m_quad;
        event.m_contact.m_separatingNormal.m_quad = separatingNormal.m_quad;
        event.m_cdBodyA = &bodyA;
        event.m_cdBodyB = &bodyB;
        collector.addCdPoint(event);
    }
    HK_TIMER_END();
}

// The closest points with the tolerance grown by the cached path length, through this agent's own getClosestPoints; a
// hit goes to the start collector when the tolerance is larger than its distance. While the path still moves into the
// surface the fraction along it is refined, the A body moved there through a copy of its motion state, until a hit
// stops closing in, the configuration's iterations run out or its early-out distance is reached. The dot products are
// computed in single precision.
void hkpGskBaseAgent::linearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& castCollector,
    hkpCdPointCollector* startCollector
) {
    hkpCollisionInput kInput = input;
    kInput.m_tolerance += input.m_cachedPathLength;

    hkpSimpleClosestContactCollector kClosest;
    getClosestPoints(bodyA, bodyB, kInput, kClosest);
    if (!kClosest.m_hasHit.m_bool) {
        return;
    }

    hkpCdPoint kPoint;
    kPoint.m_contact = kClosest.m_hitPoint;
    kPoint.m_cdBodyA = &bodyA;
    kPoint.m_cdBodyB = &bodyB;
    double fDist = kPoint.m_contact.m_separatingNormal.m_quad[3];
    if ((input.m_tolerance > fDist) && (startCollector)) {
        startCollector->addCdPoint(kPoint);
        fDist = kPoint.m_contact.m_separatingNormal.m_quad[3];
    }

    const hkQuadReal& rPath = input.m_path.m_quad;
    const __m128 kProduct = _mm_mul_ps(rPath, kPoint.m_contact.m_separatingNormal.m_quad);
    float fDot = _mm_cvtss_f32(_mm_add_ps(
        _mm_shuffle_ps(kProduct, kProduct, 0xAA), _mm_add_ps(_mm_shuffle_ps(kProduct, kProduct, 0x55), kProduct)
    ));
    double fSum = fDist + fDot;
    if (fSum > 0.0) {
        return;
    }
    const hkpCollisionAgentConfig* pConfig = input.m_config;
    if (!(fDot + input.m_maxExtraPenetration < 0.0)) {
        return;
    }
    if (pConfig->m_iterativeLinearCastEarlyOutDistance < fDist) {
        hkMotionState kMotionState(*bodyA.m_motion.pState);
        hkpCdBody kBodyA;
        kBodyA.m_shape = bodyA.m_shape;
        kBodyA.m_shapeKey = 0xFFFFFFFF;
        kBodyA.m_motion.pState = &kMotionState;
        kBodyA.m_parent = &bodyA;

        hkpSimpleClosestContactCollector kSecond;
        double fFraction = fDist / (fDist - fSum);
        int32_t iIterations = pConfig->m_iterativeLinearCastMaxIterations - 1;
        if (iIterations < 0) {
            castCollector.addCdPoint(kPoint);
            return;
        }
        for (;;) {
            kSecond.m_earlyOutDistance = 3.40282e38f;
            kSecond.m_hasHit.m_bool = false;
            kSecond.m_hitPoint.m_separatingNormal.m_quad[3] = 3.40282e38f;
            const float fFraction32 = static_cast<float>(fFraction);
            const hkQuadReal& rState = bodyA.m_motion.pState->m_transform.m_translation.m_quad;
            kMotionState.m_transform.m_translation.m_quad[0] = fFraction32 * rPath[0] + rState[0];
            kMotionState.m_transform.m_translation.m_quad[1] = fFraction32 * rPath[1] + rState[1];
            kMotionState.m_transform.m_translation.m_quad[2] = fFraction32 * rPath[2] + rState[2];
            kMotionState.m_transform.m_translation.m_quad[3] = fFraction32 * rPath[3] + rState[3];
            getClosestPoints(kBodyA, bodyB, kInput, kSecond);
            if (!kSecond.m_hasHit.m_bool) {
                return;
            }
            kPoint.m_contact.m_position = kSecond.m_hitPoint.m_position;
            kPoint.m_contact.m_separatingNormal = kSecond.m_hitPoint.m_separatingNormal;
            kPoint.m_contact.m_separatingNormal.m_quad[3] = fFraction32;
            const __m128 kNewProduct = _mm_mul_ps(rPath, kSecond.m_hitPoint.m_separatingNormal.m_quad);
            double fNewDot = _mm_cvtss_f32(_mm_add_ps(
                _mm_shuffle_ps(kNewProduct, kNewProduct, 0xAA),
                _mm_add_ps(_mm_shuffle_ps(kNewProduct, kNewProduct, 0x55), kNewProduct)
            ));
            if (fNewDot >= 0.0) {
                return;
            }
            const double fNegDot = -fNewDot;
            const double fNewDist = kSecond.m_hitPoint.m_separatingNormal.m_quad[3];
            if (fNegDot * fFraction + fNewDist > fNegDot) {
                return;
            }
            if (pConfig->m_iterativeLinearCastEarlyOutDistance >= fNewDist) {
                castCollector.addCdPoint(kPoint);
                return;
            }
            fFraction = fFraction + fNewDist / fNegDot;
            if (castCollector.m_earlyOutDistance < fFraction) {
                return;
            }
            if (--iIterations < 0) {
                castCollector.addCdPoint(kPoint);
                return;
            }
        }
    }
    if (fDist <= 0.0) {
        kPoint.m_contact.m_separatingNormal.m_quad[3] = 0.0f;
        castCollector.addCdPoint(kPoint);
        return;
    }
    if ((fDist - fSum) * castCollector.m_earlyOutDistance >= fDist) {
        kPoint.m_contact.m_separatingNormal.m_quad[3] = 0.0f;
        castCollector.addCdPoint(kPoint);
    }
}

void hkpGskBaseAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    ;
}

// The agent keeps no contact points, so cleanup only deletes the agent itself.
void hkpGskBaseAgent::cleanup(hkpConstraintOwner&) {
    delete this;
}

// Forgets the cached separating normal: its time becomes -1 and the four components zero.
void hkpGskBaseAgent::invalidateTim(const hkpCollisionInput&) {
    m_timeOfSeparatingNormal = -1.0f;
    m_separatingNormal.m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
}

// Takes the cached separating normal's time over to newTime, or, when the old time does not
// match, forgets the normal (its time becomes -1 and the four components zero).
void hkpGskBaseAgent::warpTime(float oldTime, float newTime, const hkpCollisionInput& input) {
    if (oldTime == m_timeOfSeparatingNormal) {
        m_timeOfSeparatingNormal = newTime;
    }
    else {
        m_timeOfSeparatingNormal = -1.0f;
        m_separatingNormal.m_quad = _mm_setzero_ps();
    }
}

// GSK from the cached simplex, the cache written back on a feature change. Within the tolerance the closest points move
// onto the shapes' surfaces: A's by its radius along the normal, B's (in B's space) by the distance from A's. The
// distance is stored as a float.
hkBool hkpGskBaseAgent::_getClosestPoint(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpExtendedGskOut& output
) {
    const hkpConvexShape* shapeB = static_cast<const hkpConvexShape*>(bodyB.m_shape);
    const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
    hkTransform aTb;
    aTb.setMulInverseMul(*bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion);
    // The SDK's inline hkpGsk init from the cache.
    hkpGsk gsk;
    gsk.m_dimB.m_storage = m_cache.m_dimB;
    gsk.m_maxDimA.m_storage = m_cache.m_maxDimA;
    gsk.m_maxDimB.m_storage = m_cache.m_maxDimB;
    gsk.m_dimA.m_storage = m_cache.m_dimA;
    gsk.m_doNotHandlePenetration.m_storage = 0;
    gsk.m_featureChange.m_storage = 0;
    shapeA->convertVertexIdsToVerticesImpl(&m_cache.m_vertices[0], gsk.m_dimA.m_storage, &gsk.m_verticesA[0]);
    shapeB->convertVertexIdsToVerticesImpl(
        &m_cache.m_vertices[gsk.m_dimA.m_storage], gsk.m_dimB.m_storage, &gsk.m_verticesBinB[0]
    );
    hkVector4 separatingNormal;
    gsk.getClosestFeature(shapeA, shapeB, aTb, separatingNormal);
    if (gsk.m_featureChange.m_storage) {
        gsk.exitAndExportCacheImpl(m_cache);
    }
    gsk.convertFeatureToClosestDistance(separatingNormal, output);
    const float fGskDistance = output.m_distance.m_storage;
    const hkQuadReal n = output.m_normalInA.m_quad;
    // EXE-INLINED 0xC7F700
    const hkRotation& rWa = bodyA.m_motion.pMotion->getRotation();
    output.m_normalInWorld.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(n, n, 0x55), rWa.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(n, n, 0x00), rWa.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(n, n, 0xAA), rWa.m_col2.m_quad)
    );
    const float fDistance = fGskDistance - shapeA->m_radius - shapeB->m_radius;
    output.m_distance.m_storage = fDistance;
    if (!(input.m_tolerance > fDistance)) {
        return 0;
    }
    const float fRadiusA = shapeA->m_radius;
    const float fNegDistance = -fDistance;
    output.m_pointAinA.m_quad = _mm_sub_ps(
        output.m_pointAinA.m_quad, _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fRadiusA), _mm_set_ss(fRadiusA), 0), n)
    );
    hkVector4 pointBinA;
    pointBinA.m_quad = _mm_sub_ps(
        _mm_add_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(fNegDistance), _mm_set_ss(fNegDistance), 0), output.m_normalInA.m_quad
            ),
            output.m_pointAinA.m_quad
        ),
        aTb.m_translation.m_quad
    );
    output.m_pointBinB._setRotatedInverseDir(aTb.getRotation(), pointBinA);
    return 1;
}

// Havok SDK: builds a GSK cache for the pair (aTb places B in A's space), runs GSK from it and reports the pair when
// GSK fails or the distance less both radii is negative (a NaN distance is not reported).
void hkpGskBaseAgent::staticGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN("Gsk", nullptr);
    const hkpConvexShape* shapeB = static_cast<const hkpConvexShape*>(bodyB.m_shape);
    const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
    hkTransform aTb;
    aTb.setMulInverseMul(*bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion);

    hkpGskCache cache;
    if (shapeB->m_type == HK_SHAPE_TRIANGLE) {
        cache.initTriangle(shapeA, static_cast<const hkpTriangleShape*>(shapeB), aTb);
    }
    else {
        cache.init(shapeA, shapeB, aTb);
    }

    // hkpGsk::init, with hkPadSpu's stores and loads written as plain field accesses.
    hkpGsk gsk;
    gsk.m_dimA.m_storage = cache.m_dimA;
    gsk.m_dimB.m_storage = cache.m_dimB;
    gsk.m_maxDimA.m_storage = cache.m_maxDimA;
    gsk.m_maxDimB.m_storage = cache.m_maxDimB;
    gsk.m_doNotHandlePenetration.m_storage = 1;
    gsk.m_featureChange.m_storage = 0;
    shapeA->convertVertexIdsToVerticesImpl(&cache.m_vertices[0], gsk.m_dimA.m_storage, &gsk.m_verticesA[0]);
    shapeB->convertVertexIdsToVerticesImpl(
        &cache.m_vertices[gsk.m_dimA.m_storage], gsk.m_dimB.m_storage, &gsk.m_verticesBinB[0]
    );

    hkVector4 separatingNormal;
    hkpGskStatus gjkStatus = gsk.getClosestFeature(shapeA, shapeB, aTb, separatingNormal);
    HK_TIMER_END();
    if (gjkStatus == HK_GSK_OK) {
        // The distance is rounded to a float before the compare.
        float dist = separatingNormal.m_quad[3] - shapeA->m_radius - shapeB->m_radius;
        if (!(dist < 0.0f)) {
            return;
        }
    }
    collector.addCdBodyPair(bodyA, bodyB);
}

// Havok SDK: builds a GSK cache for the pair (aTb places B in A's space) and runs GSK from it. When the distance less
// both radii is below the tolerance (a NaN distance is not), the closest point on B, in B's space, is welded against
// a triangle B's neighbours where the input asks for it (a rejected point is not reported) and reported in world
// space with the normal rotated to world space.
void hkpGskBaseAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("Gsk", nullptr);
    hkTransform aTb;
    aTb.setMulInverseMul(*bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion);
    alignas(16) hkpGskCache cache;
    if (bodyB.m_shape->m_type == HK_SHAPE_TRIANGLE) {
        cache.initTriangle(
            static_cast<const hkpConvexShape*>(bodyA.m_shape), static_cast<const hkpTriangleShape*>(bodyB.m_shape), aTb
        );
    }
    else {
        cache.init(
            static_cast<const hkpConvexShape*>(bodyA.m_shape), static_cast<const hkpConvexShape*>(bodyB.m_shape), aTb
        );
    }
    const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
    const hkpConvexShape* shapeB = static_cast<const hkpConvexShape*>(bodyB.m_shape);

    // hkpGsk::init, with hkPadSpu's stores and loads written as plain field accesses.
    hkpGsk gsk;
    gsk.m_dimA.m_storage = cache.m_dimA;
    gsk.m_dimB.m_storage = cache.m_dimB;
    gsk.m_maxDimA.m_storage = cache.m_maxDimA;
    gsk.m_maxDimB.m_storage = cache.m_maxDimB;
    gsk.m_doNotHandlePenetration.m_storage = 0;
    gsk.m_featureChange.m_storage = 0;
    shapeA->convertVertexIdsToVerticesImpl(&cache.m_vertices[0], cache.m_dimA, &gsk.m_verticesA[0]);
    shapeB->convertVertexIdsToVerticesImpl(
        &cache.m_vertices[gsk.m_dimA.m_storage], gsk.m_dimB.m_storage, &gsk.m_verticesBinB[0]
    );

    hkVector4 separatingNormal;
    if (gsk.getClosestFeature(shapeA, shapeB, aTb, separatingNormal) == HK_GSK_OK) {
        hkpGskOut out;
        gsk.convertFeatureToClosestDistance(separatingNormal, out);
        hkVector4 normal;
        normal.setRotatedDir(bodyA.m_motion.pMotion->m_rotation, out.m_normalInA);
        // The distance is rounded to a float before the compare.
        float distance = out.m_distance.m_storage - shapeA->m_radius - shapeB->m_radius;
        out.m_distance.m_storage = distance;
        if (input.m_tolerance > distance) {
            // pointAinA.addMul4(-radiusA, normal), then B's point (pointAinA - distance * normal) in B's space.
            float negRadiusA = -shapeA->m_radius;
            out.m_pointAinA.m_quad =
                _mm_add_ps(_mm_mul_ps(_mm_set1_ps(negRadiusA), out.m_normalInA.m_quad), out.m_pointAinA.m_quad);
            float negDistance = -distance;
            hkVector4 pointOnBinA;
            pointOnBinA.m_quad = _mm_sub_ps(
                _mm_add_ps(_mm_mul_ps(_mm_set1_ps(negDistance), out.m_normalInA.m_quad), out.m_pointAinA.m_quad),
                aTb.m_translation.m_quad
            );
            hkVector4 pointInB;
            pointInB.setRotatedInverseDir(aTb.m_rotation, pointOnBinA);

            int32_t weldResult = hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_UNMODIFIED;
            if (shapeB->m_type == HK_SHAPE_TRIANGLE && input.m_weldClosestPoints.m_storage) {
                if (gsk.m_featureChange.m_storage) {
                    gsk.exitAndExportCacheImpl(cache);
                }
                hkVector4 weldedNormal = normal;
                weldResult = shapeB->weldContactPointImpl(
                    &cache.m_vertices[cache.m_dimA], cache.m_dimB, pointInB, bodyB.m_motion.pMotion, shapeA,
                    bodyA.m_motion.pMotion, weldedNormal
                );
                if (weldResult == hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED) {
                    normal = weldedNormal;
                }
            }
            if (weldResult != hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT) {
                hkpCdPoint event;
                event.m_cdBodyA = &bodyA;
                event.m_cdBodyB = &bodyB;
                // EXE-INLINED 0xC7F640
                const hkTransform& transformB = *bodyB.m_motion.pMotion;
                __m128 v = pointInB.m_quad;
                event.m_contact.m_position.m_quad = _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)), transformB.m_rotation.m_col1.m_quad),
                        _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0)), transformB.m_rotation.m_col0.m_quad)
                    ),
                    _mm_add_ps(
                        _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)), transformB.m_rotation.m_col2.m_quad),
                        transformB.m_translation.m_quad
                    )
                );
                event.m_contact.m_separatingNormal.m_quad = normal.m_quad;
                event.m_contact.m_separatingNormal(3) = out.m_distance.m_storage;
                collector.addCdPoint(event);
            }
        }
    }
    HK_TIMER_END();
}

// Havok SDK: runs GSK from gsk's simplex (aTb places B in A's space) and gives the separating normal rotated into
// world space by A's transform, its w the distance less both radii. earlyOutTolerance is unused.
void hkpGskBaseAgent::calcSeparatingNormal(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    float earlyOutTolerance,
    hkpGsk& gsk,
    hkVector4& separatingNormalOut
) {
    const hkpConvexShape* shapeB = static_cast<const hkpConvexShape*>(bodyB.m_shape);
    const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
    hkTransform aTb;
    aTb.setMulInverseMul(*bodyA.m_motion.pMotion, *bodyB.m_motion.pMotion);
    hkVector4 separatingNormal;
    gsk.getClosestFeature(shapeA, shapeB, aTb, separatingNormal);
    double distance = separatingNormal.m_quad[3];
    const hkRotation& rotationA = bodyA.m_motion.pState->m_transform.m_rotation;
    const hkQuadReal n = separatingNormal.m_quad;
    separatingNormalOut.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(n, n, 85), rotationA.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(n, n, 0), rotationA.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(n, n, 170), rotationA.m_col2.m_quad)
    );
    separatingNormalOut.m_quad[3] = distance - shapeA->m_radius - shapeB->m_radius;
}
