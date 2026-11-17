#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"

#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgentConfig.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpSimpleClosestContactCollector.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <xmmintrin.h>

// The same cast for an agent instance: the closest-points query goes through the agent's own getClosestPoints instead
// of the dispatcher's table.
void hkpIterativeLinearCastAgent::linearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& castCollector,
    hkpCdPointCollector* startCollector
) {
    hkpCollisionInput kInput = input;
    kInput.m_tolerance += input.m_cachedPathLength;

    alignas(16) uint8_t aCollector[sizeof(hkpSimpleClosestContactCollector)];
    hkpSimpleClosestContactCollector* pClosest = ::new (aCollector) hkpSimpleClosestContactCollector;
    getClosestPoints(bodyA, bodyB, kInput, *pClosest);
    if (!pClosest->m_hasHit.m_bool) {
        return;
    }

    hkpCdPoint kPoint;
    kPoint.m_contact = pClosest->m_hitPoint;
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
    const __m128 kProduct = _mm_mul_ps(
        _mm_load_ps(&reinterpret_cast<const float*>(&rPath)[0]),
        _mm_load_ps(&reinterpret_cast<float*>(&kPoint.m_contact.m_separatingNormal.m_quad)[0])
    );
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
        alignas(16) uint8_t aMotionState[sizeof(hkMotionState)];
        hkMotionState* pMotionState = ::new (aMotionState) hkMotionState(*bodyA.m_motion.pState);
        hkpCdBody kBodyA;
        kBodyA.m_shape = bodyA.m_shape;
        kBodyA.m_shapeKey = 0xFFFFFFFF;
        kBodyA.m_motion.pState = pMotionState;
        kBodyA.m_parent = &bodyA;

        alignas(16) uint8_t aSecond[sizeof(hkpSimpleClosestContactCollector)];
        hkpSimpleClosestContactCollector* pSecond = ::new (aSecond) hkpSimpleClosestContactCollector;
        double fFraction = fDist / (fDist - fSum);
        int32_t iIterations = pConfig->m_iterativeLinearCastMaxIterations - 1;
        if (iIterations < 0) {
            castCollector.addCdPoint(kPoint);
            return;
        }
        for (;;) {
            pSecond->m_earlyOutDistance = 3.40282e38f;
            pSecond->m_hasHit.m_bool = false;
            pSecond->m_hitPoint.m_separatingNormal.m_quad[3] = 3.40282e38f;
            const float fFraction32 = static_cast<float>(fFraction);
            const hkQuadReal& rState = bodyA.m_motion.pState->m_transform.m_translation.m_quad;
            pMotionState->m_transform.m_translation.m_quad[0] = fFraction32 * rPath[0] + rState[0];
            pMotionState->m_transform.m_translation.m_quad[1] = fFraction32 * rPath[1] + rState[1];
            pMotionState->m_transform.m_translation.m_quad[2] = fFraction32 * rPath[2] + rState[2];
            pMotionState->m_transform.m_translation.m_quad[3] = fFraction32 * rPath[3] + rState[3];
            getClosestPoints(kBodyA, bodyB, kInput, *pSecond);
            if (!pSecond->m_hasHit.m_bool) {
                return;
            }
            kPoint.m_contact.m_position = pSecond->m_hitPoint.m_position;
            kPoint.m_contact.m_separatingNormal = pSecond->m_hitPoint.m_separatingNormal;
            kPoint.m_contact.m_separatingNormal.m_quad[3] = fFraction32;
            const __m128 kNewProduct = _mm_mul_ps(
                _mm_load_ps(&reinterpret_cast<const float*>(&rPath)[0]),
                _mm_load_ps(&reinterpret_cast<float*>(&pSecond->m_hitPoint.m_separatingNormal.m_quad)[0])
            );
            double fNewDot = _mm_cvtss_f32(_mm_add_ps(
                _mm_shuffle_ps(kNewProduct, kNewProduct, 0xAA),
                _mm_add_ps(_mm_shuffle_ps(kNewProduct, kNewProduct, 0x55), kNewProduct)
            ));
            if (fNewDot >= 0.0) {
                return;
            }
            const double fNegDot = -fNewDot;
            const double fNewDist = pSecond->m_hitPoint.m_separatingNormal.m_quad[3];
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

// The closest points of the pair with the tolerance grown by the cached path length; a hit is reported to the start
// collector when the tolerance is larger than its distance. When the path still moves into the surface (the dot of the
// path with the normal plus the distance at or below zero, and the dot plus the maximum extra penetration below it) the
// fraction along the path is refined: the A body is moved there through a copy of its motion state and the
// closest-points query repeats, a hit ending it when its dot with the path is no longer negative or its distance no
// longer closes in, until the configuration's iteration count runs out or its early-out distance is reached - then the
// last point is reported with its distance cleared. The dot products are computed in single precision. The
// closest-points function is the one from the dispatcher's table for this variant.
void hkpIterativeLinearCastAgent::staticLinearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& castCollector,
    hkpCdPointCollector* startCollector
) {
    hkpCollisionInput kInput = input;
    kInput.m_tolerance += input.m_cachedPathLength;

    alignas(16) uint8_t aCollector[sizeof(hkpSimpleClosestContactCollector)];
    hkpSimpleClosestContactCollector* pClosest = ::new (aCollector) hkpSimpleClosestContactCollector;
    hkpCollisionDispatcher* pDispatcher = input.m_dispatcher;
    hkpCollisionDispatcher::GetClosestPointsFunc pGetClosestPoints =
        pDispatcher->m_agent2Func[pDispatcher->m_agent2Types[bodyA.m_shape->m_type][bodyB.m_shape->m_type]]
            .m_getClosestPointFunc;
    pGetClosestPoints(bodyA, bodyB, kInput, *pClosest);
    if (!pClosest->m_hasHit.m_bool) {
        return;
    }

    hkpCdPoint kPoint;
    kPoint.m_contact = pClosest->m_hitPoint;
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
    const __m128 kProduct = _mm_mul_ps(
        _mm_load_ps(&reinterpret_cast<const float*>(&rPath)[0]),
        _mm_load_ps(&reinterpret_cast<float*>(&kPoint.m_contact.m_separatingNormal.m_quad)[0])
    );
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
        alignas(16) uint8_t aMotionState[sizeof(hkMotionState)];
        hkMotionState* pMotionState = ::new (aMotionState) hkMotionState(*bodyA.m_motion.pState);
        hkpCdBody kBodyA;
        kBodyA.m_shape = bodyA.m_shape;
        kBodyA.m_shapeKey = 0xFFFFFFFF;
        kBodyA.m_motion.pState = pMotionState;
        kBodyA.m_parent = &bodyA;

        alignas(16) uint8_t aSecond[sizeof(hkpSimpleClosestContactCollector)];
        hkpSimpleClosestContactCollector* pSecond = ::new (aSecond) hkpSimpleClosestContactCollector;
        double fFraction = fDist / (fDist - fSum);
        int32_t iIterations = pConfig->m_iterativeLinearCastMaxIterations - 1;
        if (iIterations < 0) {
            castCollector.addCdPoint(kPoint);
            return;
        }
        for (;;) {
            pSecond->m_earlyOutDistance = 3.40282e38f;
            pSecond->m_hasHit.m_bool = false;
            pSecond->m_hitPoint.m_separatingNormal.m_quad[3] = 3.40282e38f;
            const float fFraction32 = static_cast<float>(fFraction);
            const hkQuadReal& rState = bodyA.m_motion.pState->m_transform.m_translation.m_quad;
            pMotionState->m_transform.m_translation.m_quad[0] = fFraction32 * rPath[0] + rState[0];
            pMotionState->m_transform.m_translation.m_quad[1] = fFraction32 * rPath[1] + rState[1];
            pMotionState->m_transform.m_translation.m_quad[2] = fFraction32 * rPath[2] + rState[2];
            pMotionState->m_transform.m_translation.m_quad[3] = fFraction32 * rPath[3] + rState[3];
            pGetClosestPoints(kBodyA, bodyB, kInput, *pSecond);
            if (!pSecond->m_hasHit.m_bool) {
                return;
            }
            kPoint.m_contact.m_position = pSecond->m_hitPoint.m_position;
            kPoint.m_contact.m_separatingNormal = pSecond->m_hitPoint.m_separatingNormal;
            kPoint.m_contact.m_separatingNormal.m_quad[3] = fFraction32;
            const __m128 kNewProduct = _mm_mul_ps(
                _mm_load_ps(&reinterpret_cast<const float*>(&rPath)[0]),
                _mm_load_ps(&reinterpret_cast<float*>(&pSecond->m_hitPoint.m_separatingNormal.m_quad)[0])
            );
            double fNewDot = _mm_cvtss_f32(_mm_add_ps(
                _mm_shuffle_ps(kNewProduct, kNewProduct, 0xAA),
                _mm_add_ps(_mm_shuffle_ps(kNewProduct, kNewProduct, 0x55), kNewProduct)
            ));
            if (fNewDot >= 0.0) {
                return;
            }
            const double fNegDot = -fNewDot;
            const double fNewDist = pSecond->m_hitPoint.m_separatingNormal.m_quad[3];
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
