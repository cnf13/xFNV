#include "Havok/Source/Physics/Collide/Agent/MiscAgent/MultirayConvex/hkpMultiRayConvexAgent.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
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
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgentConfig.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpSimpleClosestContactCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/MultiRay/hkpMultiRayShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"

#include <x86intrin.h>
#include <xmmintrin.h>

// Timer names that this file's HK_TIMER macros store.
using TimerName3 = const char[3];

// The agent keeps no state for the query, so its body is written out here as staticGetClosestPoints has it.
// EXE-INLINED 0xD37280

using TimerName15 = const char[15];
static TimerName15 s_szMultiRayTimer{"TtmultiRay-cvx"};

// One contact point info per ray of body A's multi-ray shape, each with no contact point yet.
hkpMultiRayConvexAgent::hkpMultiRayConvexAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
)
  : hkpIterativeLinearCastAgent(mgr) {
    const int32_t numRays = static_cast<const hkpMultiRayShape*>(bodyA.m_shape)->m_rays.m_size;
    m_contactInfo.setSize(numRays);
    for (int32_t i = 0; i < numRays; ++i) {
        m_contactInfo.m_data[i].m_contactPointId = 0xFFFF;
    }
}

// m_contactInfo frees its buffer in its own destructor.
hkpMultiRayConvexAgent::~hkpMultiRayConvexAgent() {}

void hkpMultiRayConvexAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    hkpMultiRayConvexAgent::staticGetPenetrations(bodyA, bodyB, input, collector);
}

void hkpMultiRayConvexAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("multiRay-cvx", nullptr);

    const hkpMultiRayShape* pMultiRay = static_cast<const hkpMultiRayShape*>(bodyA.m_shape);
    const hkpShape* pConvex = bodyB.m_shape;
    hkTransform aTb;
    aTb.setMulInverseMul(*bodyB.m_motion.pMotion, *bodyA.m_motion.pMotion);
    const hkpMultiRayShape::Ray* pRay = pMultiRay->m_rays.m_data;
    int32_t iNumRays = pMultiRay->m_rays.m_size;

    hkpCdPoint event;
    event.m_cdBodyA = &bodyA;
    event.m_cdBodyB = &bodyB;
    hkpShapeRayCastInput rayInput;
    rayInput.m_filterInfo = 0;
    rayInput.m_rayShapeCollectionFilter = nullptr;

    const __m128 col0 = aTb.m_rotation.m_col0.m_quad;
    const __m128 col1 = aTb.m_rotation.m_col1.m_quad;
    const __m128 col2 = aTb.m_rotation.m_col2.m_quad;
    const __m128 col3 = aTb.m_translation.m_quad;
    for (; iNumRays > 0; --iNumRays, ++pRay) {
        FNVD_INLINED_LOCAL(hkpShapeRayCastOutput, output, std::nullopt, std::nullopt);
        output.m_hitFraction = 1.0f;
        // EXE-INLINED none: hkVector4::setTransformedPos
        __m128 v = pRay->m_start.m_quad;
        __m128 from = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), col1), _mm_mul_ps(_mm_shuffle_ps(v, v, 0), col0)),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), col2), col3)
        );
        rayInput.m_from.m_quad = from;
        output.m_extraInfo = -1;
        output.m_shapeKeys[0] = 0xFFFFFFFFu;
        output.m_shapeKeyIndex = 0;
        // EXE-INLINED none: hkVector4::setTransformedPos
        v = pRay->m_end.m_quad;
        __m128 to = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), col1), _mm_mul_ps(_mm_shuffle_ps(v, v, 0), col0)),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), col2), col3)
        );
        rayInput.m_to.m_quad = to;

        hkBool bHit = pConvex->castRayImpl(rayInput, output);
        if (bHit.m_bool) {
            const hkTransform& transformB = *bodyB.m_motion.pMotion;
            const float fFraction = output.m_hitFraction;
            const __m128 b0 = transformB.m_rotation.m_col0.m_quad;
            const __m128 b1 = transformB.m_rotation.m_col1.m_quad;
            const __m128 b2 = transformB.m_rotation.m_col2.m_quad;
            const __m128 b3 = transformB.m_translation.m_quad;
            // EXE-INLINED none: hkVector4::setInterpolate4
            __m128 t = _mm_set1_ps(fFraction);
            __m128 p = _mm_add_ps(
                _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), rayInput.m_from.m_quad), _mm_mul_ps(t, rayInput.m_to.m_quad)
            );
            // EXE-INLINED none: hkVector4::setTransformedPos
            __m128 pos = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), b1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0), b0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), b2), b3)
            );
            event.m_contact.m_position.m_quad = pos;
            // EXE-INLINED none: hkVector4::setRotatedDir
            __m128 n = output.m_normal.m_quad;
            __m128 normal = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(n, n, 0x55), b1), _mm_mul_ps(_mm_shuffle_ps(n, n, 0), b0)),
                _mm_mul_ps(_mm_shuffle_ps(n, n, 0xAA), b2)
            );
            event.m_contact.m_separatingNormal.m_quad = normal;
            event.m_contact.m_separatingNormal.m_quad[3] = static_cast<float>(
                (static_cast<double>(fFraction) - 1.0) * pRay->m_start.m_quad[3] + pMultiRay->m_rayPenetrationDistance
            );
            collector.addCdPoint(event);
        }
    }

    HK_TIMER_END();
}

// The closest points with the tolerance grown by the cached path length, through this agent's own getClosestPoints; a
// hit goes to the start collector when the tolerance is larger than its distance. While the path still moves into the
// surface the fraction along it is refined, the A body moved there through a copy of its motion state, until a hit
// stops closing in, the configuration's iterations run out or its early-out distance is reached. The dot products are
// single precision.
void hkpMultiRayConvexAgent::linearCast(
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
    if (input.m_tolerance > fDist) {
        if (startCollector) {
            startCollector->addCdPoint(kPoint);
            fDist = kPoint.m_contact.m_separatingNormal.m_quad[3];
        }
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

// Casts every ray of the multi-ray shape (body A) against the convex shape (body B). A ray that hits makes a contact
// point at the hit in world space, added to the contact manager when the ray has none yet and handed to the output when
// it has one; a ray that misses removes its contact point. The input's filter fields are set once, before the loop, the
// output's in every pass. The distance is (fraction - 1.0) unrounded, times the ray's w, plus m_rayPenetrationDistance,
// computed in double and then stored as a float.
void hkpMultiRayConvexAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN("multiRay-cvx", nullptr);

    const hkpMultiRayShape* pMultiRay = static_cast<const hkpMultiRayShape*>(bodyA.m_shape);
    const hkpShape* pConvex = bodyB.m_shape;
    hkTransform aTb;
    aTb.setMulInverseMul(*bodyB.m_motion.pMotion, *bodyA.m_motion.pMotion);
    const int32_t iNumRays = pMultiRay->m_rays.m_size;
    hkpShapeRayCastInput rayInput;
    rayInput.m_filterInfo = 0;
    rayInput.m_rayShapeCollectionFilter = nullptr;
    const hkpMultiRayShape::Ray* pRay = pMultiRay->m_rays.m_data;
    for (int32_t i = 0; i < iNumRays; ++i, ++pRay) {
        FNVD_INLINED_LOCAL(hkpShapeRayCastOutput, output, std::nullopt, std::nullopt);
        output.m_hitFraction = 1.0f;
        // EXE-INLINED 0xC7F640
        __m128 v = pRay->m_start.m_quad;
        rayInput.m_from.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), aTb.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0), aTb.m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), aTb.m_rotation.m_col2.m_quad), aTb.m_translation.m_quad)
        );
        // EXE-INLINED 0xC7F640
        v = pRay->m_end.m_quad;
        output.m_extraInfo = -1;
        output.m_shapeKeys[0] = 0xFFFFFFFFu;
        output.m_shapeKeyIndex = 0;
        rayInput.m_to.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), aTb.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0), aTb.m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), aTb.m_rotation.m_col2.m_quad), aTb.m_translation.m_quad)
        );
        hkBool bHit = pConvex->castRayImpl(rayInput, output);
        if (bHit.m_bool) {
            hkpProcessCdPoint* pPoint = result.m_firstFreeContactPoint.m_storage;
            // EXE-INLINED none: hkVector4::setInterpolate4 (the exe reads its own copy of the ones)
            const __m128 t = _mm_set1_ps(output.m_hitFraction);
            hkVector4 hitLocal;
            hitLocal.m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), rayInput.m_from.m_quad), _mm_mul_ps(t, rayInput.m_to.m_quad)
            );
            pPoint->m_contact.m_position.setTransformedPos(*bodyB.m_motion.pMotion, hitLocal);
            pPoint->m_contact.m_separatingNormal.setRotatedDir(bodyB.m_motion.pMotion->m_rotation, output.m_normal);
            pPoint->m_contact.m_separatingNormal.m_quad[3] = static_cast<float>(
                (static_cast<double>(output.m_hitFraction) - 1.0) * pRay->m_start.m_quad[3] +
                pMultiRay->m_rayPenetrationDistance
            );
            if (m_contactInfo.m_data[i].m_contactPointId == 0xFFFF) {
                m_contactInfo.m_data[i].m_contactPointId =
                    m_contactMgr->addContactPointImpl(bodyA, bodyB, input, result, nullptr, pPoint->m_contact);
            }
            if (m_contactInfo.m_data[i].m_contactPointId != 0xFFFF) {
                ++result.m_firstFreeContactPoint.m_storage;
                pPoint->m_contactPointId.m_storage = m_contactInfo.m_data[i].m_contactPointId;
            }
        }
        else {
            const uint16_t uiContactPointId = m_contactInfo.m_data[i].m_contactPointId;
            if (uiContactPointId != 0xFFFF) {
                m_contactMgr->removeContactPointImpl(uiContactPointId, *result.m_constraintOwner.m_storage);
                m_contactInfo.m_data[i].m_contactPointId = 0xFFFF;
            }
        }
    }

    HK_TIMER_END();
}

void hkpMultiRayConvexAgent::cleanup(hkpConstraintOwner& constraintOwner) {
    int32_t m_size = m_contactInfo.m_size;
    for (int32_t i = 0; i < m_size; ++i) {
        int32_t m_contactPointId = m_contactInfo.m_data[i].m_contactPointId;
        if (static_cast<uint16_t>(m_contactPointId) != 0xFFFF) {
            m_contactMgr->removeContactPointImpl(m_contactPointId, constraintOwner);
        }
    }
    delete this;
}

// Casts every ray of the multi-ray shape (body A) against the convex shape (body B) and reports the body pair once per
// ray that hits, until the collector asks to stop. The output's fields and the input's filter fields are set once,
// before the loop.
void hkpMultiRayConvexAgent::staticGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN("multiRay-cnvx-getPen", nullptr);

    const hkpMultiRayShape* pMultiRay = static_cast<const hkpMultiRayShape*>(bodyA.m_shape);
    const hkpShape* pConvex = bodyB.m_shape;
    hkTransform aTb;
    aTb.setMulInverseMul(*bodyB.m_motion.pMotion, *bodyA.m_motion.pMotion);
    int32_t iNumRays = pMultiRay->m_rays.m_size;
    const hkpMultiRayShape::Ray* pRay = pMultiRay->m_rays.m_data;

    FNVD_INLINED_LOCAL(hkpShapeRayCastOutput, output, std::nullopt, std::nullopt);
    output.m_hitFraction = 1.0f;
    hkpShapeRayCastInput rayInput;
    rayInput.m_filterInfo = 0;
    rayInput.m_rayShapeCollectionFilter = nullptr;
    output.m_extraInfo = -1;
    output.m_shapeKeyIndex = 0;
    output.m_shapeKeys[0] = 0xFFFFFFFFu;
    while (iNumRays != 0) {
        --iNumRays;
        if (collector.m_earlyOut.m_bool) {
            break;
        }
        // EXE-INLINED 0xC7F640
        __m128 v = pRay->m_start.m_quad;
        rayInput.m_from.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), aTb.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0), aTb.m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), aTb.m_rotation.m_col2.m_quad), aTb.m_translation.m_quad)
        );
        // EXE-INLINED 0xC7F640
        v = pRay->m_end.m_quad;
        rayInput.m_to.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), aTb.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0), aTb.m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), aTb.m_rotation.m_col2.m_quad), aTb.m_translation.m_quad)
        );
        hkBool bHit = pConvex->castRayImpl(rayInput, output);
        if (bHit.m_bool) {
            collector.addCdBodyPair(bodyA, bodyB);
        }
        ++pRay;
    }

    HK_TIMER_END();
}

// Casts every ray of the multi-ray shape (body A) against the convex shape (body B) and reports each hit as a contact
// point in world space whose distance is how far the ray's end lies past the hit, less the shape's penetration
// distance. The input's filter fields are set once, before the loop. The distance is computed as (fraction - 1.0)
// unrounded, times the ray's w, plus m_rayPenetrationDistance, in double precision, then stored as a float.
void hkpMultiRayConvexAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    // EXE-INLINED none: HK_TIMER_BEGIN
    hkMonitorStream* pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
    if (pStream->m_end < pStream->m_capacityMinus16) {
        // The stream's buffer is raw memory.
        auto* pBegin = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
        pBegin->m_commandAndMonitor = s_szMultiRayTimer;
        pBegin->m_time0 = static_cast<uint32_t>(__rdtsc());
        pStream->m_end = reinterpret_cast<char*>(pBegin + 1);
    }

    const hkpMultiRayShape* pMultiRay = static_cast<const hkpMultiRayShape*>(bodyA.m_shape);
    const hkpShape* pConvex = bodyB.m_shape;
    hkTransform aTb;
    aTb.setMulInverseMul(*bodyB.m_motion.pMotion, *bodyA.m_motion.pMotion);
    const hkpMultiRayShape::Ray* pRay = pMultiRay->m_rays.m_data;
    int32_t iNumRays = pMultiRay->m_rays.m_size;

    hkpCdPoint event;
    event.m_cdBodyA = &bodyA;
    event.m_cdBodyB = &bodyB;
    hkpShapeRayCastInput rayInput;
    rayInput.m_filterInfo = 0;
    rayInput.m_rayShapeCollectionFilter = nullptr;

    const __m128 col0 = _mm_load_ps(&reinterpret_cast<float*>(&aTb.m_rotation.m_col0.m_quad)[0]);
    const __m128 col1 = _mm_load_ps(&reinterpret_cast<float*>(&aTb.m_rotation.m_col1.m_quad)[0]);
    const __m128 col2 = _mm_load_ps(&reinterpret_cast<float*>(&aTb.m_rotation.m_col2.m_quad)[0]);
    const __m128 col3 = _mm_load_ps(&reinterpret_cast<float*>(&aTb.m_translation.m_quad)[0]);
    for (; iNumRays > 0; --iNumRays, ++pRay) {
        FNVD_INLINED_LOCAL(hkpShapeRayCastOutput, output, std::nullopt, std::nullopt);
        output.m_hitFraction = 1.0f;
        // EXE-INLINED none: hkVector4::setTransformedPos
        __m128 v = _mm_load_ps(&reinterpret_cast<const float*>(&pRay->m_start.m_quad)[0]);
        __m128 from = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), col1), _mm_mul_ps(_mm_shuffle_ps(v, v, 0), col0)),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), col2), col3)
        );
        _mm_store_ps(&reinterpret_cast<float*>(&rayInput.m_from.m_quad)[0], from);
        output.m_extraInfo = -1;
        output.m_shapeKeys[0] = 0xFFFFFFFFu;
        output.m_shapeKeyIndex = 0;
        // EXE-INLINED none: hkVector4::setTransformedPos
        v = _mm_load_ps(&reinterpret_cast<const float*>(&pRay->m_end.m_quad)[0]);
        __m128 to = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), col1), _mm_mul_ps(_mm_shuffle_ps(v, v, 0), col0)),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), col2), col3)
        );
        _mm_store_ps(&reinterpret_cast<float*>(&rayInput.m_to.m_quad)[0], to);

        hkBool bHit = pConvex->castRayImpl(rayInput, output);
        if (bHit.m_bool) {
            const hkTransform& transformB = *bodyB.m_motion.pMotion;
            const float fFraction = output.m_hitFraction;
            const __m128 b0 = _mm_load_ps(&reinterpret_cast<const float*>(&transformB.m_rotation.m_col0.m_quad)[0]);
            const __m128 b1 = _mm_load_ps(&reinterpret_cast<const float*>(&transformB.m_rotation.m_col1.m_quad)[0]);
            const __m128 b2 = _mm_load_ps(&reinterpret_cast<const float*>(&transformB.m_rotation.m_col2.m_quad)[0]);
            const __m128 b3 = _mm_load_ps(&reinterpret_cast<const float*>(&transformB.m_translation.m_quad)[0]);
            // EXE-INLINED none: hkVector4::setInterpolate4
            __m128 t = _mm_set1_ps(fFraction);
            __m128 p = _mm_add_ps(
                _mm_mul_ps(
                    _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&hkQuadReal1111_0)[0]), t),
                    _mm_load_ps(&reinterpret_cast<float*>(&rayInput.m_from.m_quad)[0])
                ),
                _mm_mul_ps(t, _mm_load_ps(&reinterpret_cast<float*>(&rayInput.m_to.m_quad)[0]))
            );
            // EXE-INLINED none: hkVector4::setTransformedPos
            __m128 pos = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), b1), _mm_mul_ps(_mm_shuffle_ps(p, p, 0), b0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), b2), b3)
            );
            _mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_position.m_quad)[0], pos);
            // EXE-INLINED none: hkVector4::setRotatedDir
            __m128 n = _mm_load_ps(&reinterpret_cast<float*>(&output.m_normal.m_quad)[0]);
            __m128 normal = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(n, n, 0x55), b1), _mm_mul_ps(_mm_shuffle_ps(n, n, 0), b0)),
                _mm_mul_ps(_mm_shuffle_ps(n, n, 0xAA), b2)
            );
            _mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_separatingNormal.m_quad)[0], normal);
            event.m_contact.m_separatingNormal.m_quad[3] = static_cast<float>(
                (static_cast<double>(fFraction) - 1.0) * pRay->m_start.m_quad[3] + pMultiRay->m_rayPenetrationDistance
            );
            collector.addCdPoint(event);
        }
    }

    // EXE-INLINED none: HK_TIMER_END
    pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
    if (pStream->m_end < pStream->m_capacityMinus16) {
        auto* pEnd = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
        pEnd->m_commandAndMonitor = "Et";
        pEnd->m_time0 = static_cast<uint32_t>(__rdtsc());
        pStream->m_end = reinterpret_cast<char*>(pEnd + 1);
    }
}

hkpCollisionAgent* hkpMultiRayConvexAgent::createMultiRayConvexAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpMultiRayConvexAgent* v5 =
        static_cast<hkpMultiRayConvexAgent*>(Instance->m_heap->blockAlloc(sizeof(hkpMultiRayConvexAgent)));
    v5->m_memSizeAndFlags = sizeof(hkpMultiRayConvexAgent);
    return ::new (v5) hkpMultiRayConvexAgent(bodyA, bodyB, input, mgr);
}

hkpCollisionAgent* hkpMultiRayConvexAgent::createConvexMultiRayAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpSymmetricAgentLinearCast<hkpMultiRayConvexAgent>* v5 =
        static_cast<hkpSymmetricAgentLinearCast<hkpMultiRayConvexAgent>*>(
            Instance->m_heap->blockAlloc(sizeof(hkpSymmetricAgentLinearCast<hkpMultiRayConvexAgent>))
        );
    v5->m_memSizeAndFlags = sizeof(hkpSymmetricAgentLinearCast<hkpMultiRayConvexAgent>);
    return ::new (v5) hkpSymmetricAgentLinearCast<hkpMultiRayConvexAgent>(bodyA, bodyB, input, mgr);
}

// Convex against multi-ray (flipped, through the symmetric wrappers) and multi-ray against convex; both are predictive,
// the linear cast of the direct pair is the iterative agent's. The local AgentFuncs' padding bytes are deliberately
// left unwritten and are copied by registerCollisionAgent, an uninitialised read the game also does.
void hkpMultiRayConvexAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_createFunc = &hkpMultiRayConvexAgent::createConvexMultiRayAgent;
    f.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpMultiRayConvexAgent>::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpMultiRayConvexAgent>::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>::staticLinearCast;
    f.m_isFlipped.m_bool = 1;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_CONVEX, HK_SHAPE_MULTI_RAY);
    f.m_createFunc = &hkpMultiRayConvexAgent::createMultiRayConvexAgent;
    f.m_getPenetrationsFunc = &hkpMultiRayConvexAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpMultiRayConvexAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpIterativeLinearCastAgent::staticLinearCast;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_MULTI_RAY, HK_SHAPE_CONVEX);
}
