#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "xfnv/Compiler.h"

#include <xmmintrin.h>

// Each corner takes the point's component when it is the smaller or larger, else the box's, then both w components are
// cleared. The empty box's corners are HK_REAL_MAX splatted. Each point is read through the array again after the store
// before it (a box over its own points sees the store).
hkAabb* hkAabbUtil::calcAabb(const hkVector4* const* vectorArray, int32_t numVertices, hkAabb& aabbOut) {
    if (numVertices == 0) {
        XFNV_M128_F32(aabbOut.m_min.m_quad, 0) = HK_REAL_MAX;
        XFNV_M128_F32(aabbOut.m_min.m_quad, 1) = HK_REAL_MAX;
        XFNV_M128_F32(aabbOut.m_min.m_quad, 2) = HK_REAL_MAX;
        XFNV_M128_F32(aabbOut.m_min.m_quad, 3) = HK_REAL_MAX;
        XFNV_M128_F32(aabbOut.m_max.m_quad, 0) = -HK_REAL_MAX;
        XFNV_M128_F32(aabbOut.m_max.m_quad, 1) = -HK_REAL_MAX;
        XFNV_M128_F32(aabbOut.m_max.m_quad, 2) = -HK_REAL_MAX;
        XFNV_M128_F32(aabbOut.m_max.m_quad, 3) = -HK_REAL_MAX;
        return &aabbOut;
    }
    aabbOut.m_min.m_quad = vectorArray[0]->m_quad;
    aabbOut.m_max.m_quad = vectorArray[0]->m_quad;
    for (int32_t i = 1; i < numVertices; ++i) {
        const hkQuadReal kPoint = vectorArray[i]->m_quad;
        const hkQuadReal kMin = aabbOut.m_min.m_quad;
        hkQuadReal kNewMin;
        XFNV_M128_F32(kNewMin, 0) = XFNV_M128_F32(kPoint, 0) < XFNV_M128_F32(kMin, 0) ? XFNV_M128_F32(kPoint, 0)
                                                                                      : XFNV_M128_F32(kMin, 0);
        XFNV_M128_F32(kNewMin, 1) = XFNV_M128_F32(kPoint, 1) < XFNV_M128_F32(kMin, 1) ? XFNV_M128_F32(kPoint, 1)
                                                                                      : XFNV_M128_F32(kMin, 1);
        XFNV_M128_F32(kNewMin, 2) = XFNV_M128_F32(kPoint, 2) < XFNV_M128_F32(kMin, 2) ? XFNV_M128_F32(kPoint, 2)
                                                                                      : XFNV_M128_F32(kMin, 2);
        XFNV_M128_F32(kNewMin, 3) = XFNV_M128_F32(kPoint, 3) < XFNV_M128_F32(kMin, 3) ? XFNV_M128_F32(kPoint, 3)
                                                                                      : XFNV_M128_F32(kMin, 3);
        const hkQuadReal kMax = aabbOut.m_max.m_quad;
        aabbOut.m_min.m_quad = kNewMin;
        const hkQuadReal kPointAgain = vectorArray[i]->m_quad;
        hkQuadReal kNewMax;
        XFNV_M128_F32(kNewMax, 0) = XFNV_M128_F32(kPointAgain, 0) > XFNV_M128_F32(kMax, 0)
            ? XFNV_M128_F32(kPointAgain, 0)
            : XFNV_M128_F32(kMax, 0);
        XFNV_M128_F32(kNewMax, 1) = XFNV_M128_F32(kPointAgain, 1) > XFNV_M128_F32(kMax, 1)
            ? XFNV_M128_F32(kPointAgain, 1)
            : XFNV_M128_F32(kMax, 1);
        XFNV_M128_F32(kNewMax, 2) = XFNV_M128_F32(kPointAgain, 2) > XFNV_M128_F32(kMax, 2)
            ? XFNV_M128_F32(kPointAgain, 2)
            : XFNV_M128_F32(kMax, 2);
        XFNV_M128_F32(kNewMax, 3) = XFNV_M128_F32(kPointAgain, 3) > XFNV_M128_F32(kMax, 3)
            ? XFNV_M128_F32(kPointAgain, 3)
            : XFNV_M128_F32(kMax, 3);
        aabbOut.m_max.m_quad = kNewMax;
    }
    XFNV_M128_F32(aabbOut.m_min.m_quad, 3) = 0.0f;
    XFNV_M128_F32(aabbOut.m_max.m_quad, 3) = 0.0f;
    return &aabbOut;
}

hkAabb* hkAabbUtil::calcAabb(const hkVector4* vertices, int32_t numVertices, hkAabb& aabbOut) {
    if (numVertices) {
        __m128 m_quad = vertices->m_quad;
        __m128 v5 = vertices->m_quad;
        if (numVertices > 1) {
            const hkVector4* v6 = vertices + 1;
            int32_t v7 = numVertices - 1;
            do {
                __m128 v8 = v6->m_quad;
                ++v6;
                --v7;
                m_quad = _mm_min_ps(m_quad, v8);
                v5 = _mm_max_ps(v5, v8);
            }
            while (v7);
        }
        aabbOut.m_min.m_quad = _mm_and_ps(hkQuadRealXYZMask, m_quad);
        aabbOut.m_max.m_quad = _mm_and_ps(hkQuadRealXYZMask, v5);
        return &aabbOut;
    }

    aabbOut.m_min.m_quad = _mm_shuffle_ps(_mm_set_ss(HK_REAL_MAX), _mm_set_ss(HK_REAL_MAX), 0);
    aabbOut.m_max.m_quad = _mm_shuffle_ps(_mm_set_ss(-HK_REAL_MAX), _mm_set_ss(-HK_REAL_MAX), 0);
    return &aabbOut;
}

// calcAabb over strided float points: min/max over each point's x, y and z, then the xyz mask (setXYZ0). The point's w
// is masked out of both results, so it is written as 0.
hkAabb* hkAabbUtil::calcAabb(const float* vertexArray, int32_t numVertices, int32_t striding, hkAabb& aabbOut) {
    hkVector4 point;
    point.set(vertexArray[0], vertexArray[1], vertexArray[2], 0.0f);
    hkQuadReal vMin = point.m_quad;
    hkQuadReal vMax = point.m_quad;
    const float* pVertex = vertexArray;
    for (int32_t i = 1; i < numVertices; i++) {
        pVertex = hkAddByteOffsetConst(pVertex, striding);
        point.set(pVertex[0], pVertex[1], pVertex[2], 0.0f);
        vMin = _mm_min_ps(vMin, point.m_quad);
        vMax = _mm_max_ps(vMax, point.m_quad);
    }
    aabbOut.m_min.m_quad = _mm_and_ps(hkQuadRealXYZMask, vMin);
    aabbOut.m_max.m_quad = _mm_and_ps(hkQuadRealXYZMask, vMax);
    return &aabbOut;
}

void hkAabbUtil::calcAabb(
    const hkTransform& localToWorld,
    const hkVector4& halfExtents,
    const hkVector4& center,
    float extraRadius,
    hkAabb& aabbOut
) {
    hkVector4 v5;
    hkVector4 v6;
    v5.m_quad = localToWorld.m_rotation.m_col1.m_quad;
    v6.m_quad = localToWorld.m_rotation.m_col2.m_quad;
    __m128 v7 = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(center.m_quad, center.m_quad, 0), localToWorld.m_rotation.m_col0.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(center.m_quad, center.m_quad, 85), v5.m_quad)
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(center.m_quad, center.m_quad, 170), v6.m_quad),
            localToWorld.m_translation.m_quad
        )
    );
    __m128 v8 = _mm_add_ps(
        _mm_add_ps(
            _mm_and_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(halfExtents.m_quad, halfExtents.m_quad, 0),
                    localToWorld.m_rotation.m_col0.m_quad
                ),
                hkVector4::absMask
            ),
            _mm_and_ps(
                _mm_mul_ps(_mm_shuffle_ps(halfExtents.m_quad, halfExtents.m_quad, 85), v5.m_quad),
                hkVector4::absMask
            )
        ),
        _mm_add_ps(
            _mm_and_ps(
                _mm_mul_ps(_mm_shuffle_ps(halfExtents.m_quad, halfExtents.m_quad, 170), v6.m_quad),
                hkVector4::absMask
            ),
            _mm_shuffle_ps(_mm_set_ss(extraRadius), _mm_set_ss(extraRadius), 0)
        )
    );
    aabbOut.m_max.m_quad = _mm_add_ps(v7, v8);
    aabbOut.m_min.m_quad = _mm_add_ps(_mm_xor_ps(v8, hkQuadRealNegateMask), v7);
}

void hkAabbUtil::calcAabb(
    const hkTransform& BvToWorld,
    const hkVector4& halfExtents,
    float extraRadius,
    hkAabb& aabbOut
) {
    hkVector4 v4;
    v4.m_quad = BvToWorld.m_translation.m_quad;
    __m128 v5 = _mm_add_ps(
        _mm_add_ps(
            _mm_and_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(halfExtents.m_quad, halfExtents.m_quad, 85),
                    BvToWorld.m_rotation.m_col1.m_quad
                ),
                hkVector4::absMask
            ),
            _mm_and_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(halfExtents.m_quad, halfExtents.m_quad, 0),
                    BvToWorld.m_rotation.m_col0.m_quad
                ),
                hkVector4::absMask
            )
        ),
        _mm_add_ps(
            _mm_and_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(halfExtents.m_quad, halfExtents.m_quad, 170),
                    BvToWorld.m_rotation.m_col2.m_quad
                ),
                hkVector4::absMask
            ),
            _mm_shuffle_ps(_mm_set_ss(extraRadius), _mm_set_ss(extraRadius), 0)
        )
    );
    aabbOut.m_max.m_quad = _mm_add_ps(v4.m_quad, v5);
    aabbOut.m_min.m_quad = _mm_add_ps(_mm_xor_ps(v5, hkQuadRealNegateMask), v4.m_quad);
}

// The motion state's swept transform at times out to its step's end. Each sample uses t = (time - t0) * invDeltaTime,
// kept as a float, the rotations interpolated by t and normalized with sqrtInverse (one Newton step, zero for a zero
// length), then the centre of mass interpolated and the local centre of mass rotated and subtracted. Each sample's
// translation w is the factor 1/cos of its half-angle step (the series 1 - a^2/2 inverted for one sample).
void hkAabbUtil::initOffsetAabbInput(const hkMotionState* motionState, OffsetAabbInput& input) {
    const hkSweptTransform& swept = motionState->m_sweptTransform;
    input.m_motionState.m_storage = motionState;
    input.m_endTransformInv.setInverse(motionState->m_transform);
    const __m128 zero = _mm_setzero_ps();
    hkQuaternion q;

    // The transform at the step's start (time t0).
    float t = (XFNV_M128_F32(swept.m_centerOfMass0.m_quad, 3) - XFNV_M128_F32(swept.m_centerOfMass0.m_quad, 3)) *
        XFNV_M128_F32(swept.m_centerOfMass1.m_quad, 3);
    __m128 tv = _mm_shuffle_ps(_mm_set_ss(t), _mm_set_ss(t), 0);
    __m128 rot = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tv), swept.m_rotation0.m_vec.m_quad),
        _mm_mul_ps(swept.m_rotation1.m_vec.m_quad, tv)
    );
    __m128 len2 = _mm_mul_ps(rot, rot);
    len2 = _mm_add_ps(_mm_shuffle_ps(len2, len2, 0x4E), len2);
    len2 = _mm_add_ps(_mm_shuffle_ps(len2, len2, 0xB1), len2);
    float r = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(len2, 0))));
    __m128 scale = _mm_setzero_ps();
    XFNV_M128_F32(scale, 0) = (0.5f * r) * (3.0f - ((XFNV_M128_F32(len2, 0) * r) * r));
    scale = _mm_andnot_ps(_mm_cmpeq_ss(len2, zero), scale);
    q.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0), rot);
    input.m_startTransform.m_rotation.set(q);
    hkTransform& start = input.m_startTransform;
    start.m_translation.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tv), swept.m_centerOfMass0.m_quad),
        _mm_mul_ps(swept.m_centerOfMass1.m_quad, tv)
    );
    start.m_translation.m_quad = _mm_sub_ps(
        start.m_translation.m_quad,
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad, 0x55),
                    start.m_rotation.m_col1.m_quad
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad, 0x00),
                    start.m_rotation.m_col0.m_quad
                )
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad, 0xAA),
                start.m_rotation.m_col2.m_quad
            )
        )
    );

    if (0.0f == XFNV_M128_F32(swept.m_centerOfMass1.m_quad, 3)) {
        input.m_numTransforms.m_storage = 0;
        return;
    }
    float deltaTime = 1.0 / XFNV_M128_F32(swept.m_centerOfMass1.m_quad, 3);
    float deltaAngle = XFNV_M128_F32(motionState->m_deltaAngle.m_quad, 3);
    if (deltaAngle <= 0.39269909262657166) {
        // One sample: the start transform.
        input.m_transforms[0] = input.m_startTransform;
        float cosApprox = 1.0 - deltaAngle * deltaAngle * 0.5;
        XFNV_M128_F32(input.m_transforms[0].m_translation.m_quad, 3) = 1.0 / cosApprox;
        input.m_numTransforms.m_storage = 1;
        return;
    }
    if (deltaAngle <= 0.7853981852531433) {
        // One sample: the transform at the step's middle.
        float time = deltaTime * 0.5 + XFNV_M128_F32(swept.m_centerOfMass0.m_quad, 3);
        t = (time - XFNV_M128_F32(swept.m_centerOfMass0.m_quad, 3)) * XFNV_M128_F32(swept.m_centerOfMass1.m_quad, 3);
        tv = _mm_shuffle_ps(_mm_set_ss(t), _mm_set_ss(t), 0);
        rot = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tv), swept.m_rotation0.m_vec.m_quad),
            _mm_mul_ps(swept.m_rotation1.m_vec.m_quad, tv)
        );
        len2 = _mm_mul_ps(rot, rot);
        len2 = _mm_add_ps(_mm_shuffle_ps(len2, len2, 0x4E), len2);
        len2 = _mm_add_ps(_mm_shuffle_ps(len2, len2, 0xB1), len2);
        r = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(len2, 0))));
        scale = _mm_setzero_ps();
        XFNV_M128_F32(scale, 0) = (0.5f * r) * (3.0f - ((XFNV_M128_F32(len2, 0) * r) * r));
        scale = _mm_andnot_ps(_mm_cmpeq_ss(len2, zero), scale);
        q.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0), rot);
        hkTransform& sample = input.m_transforms[0];
        sample.m_rotation.set(q);
        sample.m_translation.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tv), swept.m_centerOfMass0.m_quad),
            _mm_mul_ps(swept.m_centerOfMass1.m_quad, tv)
        );
        sample.m_translation.m_quad = _mm_sub_ps(
            sample.m_translation.m_quad,
            _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad, 0x55),
                        sample.m_rotation.m_col1.m_quad
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad, 0x00),
                        sample.m_rotation.m_col0.m_quad
                    )
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad, 0xAA),
                    sample.m_rotation.m_col2.m_quad
                )
            )
        );
        float cosApprox = 1.0 - deltaAngle * deltaAngle * 0.25 * 0.5;
        XFNV_M128_F32(sample.m_translation.m_quad, 3) = 1.0 / cosApprox;
        input.m_numTransforms.m_storage = 1;
        return;
    }
    // One sample per pi/8 of rotation: at the odd steps i/n of the step, i = 1, 3, ... below n.
    input.m_numTransforms.m_storage = 0;
    float numSteps = (deltaAngle + 0.39269909262657166) / 0.39269909262657166;
    float invNumSteps = 1.0 / numSteps;
    float i = 1.0f;
    double numStepsD = numSteps;
    if (!(numStepsD > 1.0)) {
        return;
    }
    do {
        hkTransform& sample = input.m_transforms[input.m_numTransforms.m_storage++];
        float time = i * invNumSteps * deltaTime + XFNV_M128_F32(swept.m_centerOfMass0.m_quad, 3);
        t = (time - XFNV_M128_F32(swept.m_centerOfMass0.m_quad, 3)) * XFNV_M128_F32(swept.m_centerOfMass1.m_quad, 3);
        tv = _mm_shuffle_ps(_mm_set_ss(t), _mm_set_ss(t), 0);
        rot = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tv), swept.m_rotation0.m_vec.m_quad),
            _mm_mul_ps(tv, swept.m_rotation1.m_vec.m_quad)
        );
        len2 = _mm_mul_ps(rot, rot);
        len2 = _mm_add_ps(_mm_shuffle_ps(len2, len2, 0x4E), len2);
        len2 = _mm_add_ps(_mm_shuffle_ps(len2, len2, 0xB1), len2);
        r = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(len2, 0))));
        scale = _mm_setzero_ps();
        XFNV_M128_F32(scale, 0) = (0.5f * r) * (3.0f - ((XFNV_M128_F32(len2, 0) * r) * r));
        scale = _mm_andnot_ps(_mm_cmpeq_ss(len2, zero), scale);
        q.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0), rot);
        sample.m_rotation.set(q);
        sample.m_translation.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tv), swept.m_centerOfMass0.m_quad),
            _mm_mul_ps(tv, swept.m_centerOfMass1.m_quad)
        );
        sample.m_translation.m_quad = _mm_sub_ps(
            sample.m_translation.m_quad,
            _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad, 0x55),
                        sample.m_rotation.m_col1.m_quad
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad, 0x00),
                        sample.m_rotation.m_col0.m_quad
                    )
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad, 0xAA),
                    sample.m_rotation.m_col2.m_quad
                )
            )
        );
        XFNV_M128_F32(sample.m_translation.m_quad, 3) = 1.0824f;
        i = i + 2.0;
    }
    while (i < numStepsD);
}
