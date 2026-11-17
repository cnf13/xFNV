#include "Havok/Source/Animation/Ragdoll/PoseMatching/hkaPoseMatchingUtility.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

void hkRemoveTwist(const hkVector4& direction, hkQuaternion& rotationInOut) {
    hkVector4 v2;
    int32_t v4;
    hkVector4 v22;
    hkQuaternion result;
    hkQuaternion v24;
    hkQuaternion v25;
    hkRotation r;
    v2.m_quad = direction.m_quad;
    v24.m_vec.m_quad = _mm_setzero_ps();
    __m128 v3 = _mm_and_ps(v2.m_quad, hkVector4_setAbs4_absMask);
    result.m_vec.m_quad = v3;
    if (!(v3[1] > static_cast<double>(v3[0]))) {
        if (!(result.m_vec.m_quad[2] > static_cast<double>(v3[0]))) {
            v4 = 0;
            goto LABEL_7;
        }
        goto LABEL_5;
    }
    if (result.m_vec.m_quad[2] > static_cast<double>(v3[1])) {
    LABEL_5:;
        v4 = 2;
        goto LABEL_7;
    }
    v4 = 1;
LABEL_7:;
    __m128 m_quad = rotationInOut.m_vec.m_quad;
    __m128 v6 = _mm_shuffle_ps(rotationInOut.m_vec.m_quad, rotationInOut.m_vec.m_quad, 255);
    v24.m_vec.m_quad[(v4 + 1) % 3] = 1.0f;
    __m128 v7 = _mm_mul_ps(m_quad, v24.m_vec.m_quad);
    __m128 v8 = _mm_add_ss(_mm_shuffle_ps(v7, v7, 170), _mm_add_ss(_mm_shuffle_ps(v7, v7, 85), v7));
    __m128 v9 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(m_quad, m_quad, 201), _mm_shuffle_ps(v24.m_vec.m_quad, v24.m_vec.m_quad, 210)
                ),
                _mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 210), _mm_shuffle_ps(v24.m_vec.m_quad, v24.m_vec.m_quad, 201))
            ),
            v6
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v6, v6), hkQuadRealMinusHalf), v24.m_vec.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v8, v8, 0), m_quad)
        )
    );
    __m128 v10 = _mm_add_ps(v9, v9);
    __m128 v11 = _mm_shuffle_ps(v2.m_quad, v2.m_quad, 201);
    __m128 v12 = _mm_shuffle_ps(v2.m_quad, v2.m_quad, 210);
    __m128 v13 =
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v10, v10, 201), v12), _mm_mul_ps(_mm_shuffle_ps(v10, v10, 210), v11));
    __m128 v14 = _mm_mul_ps(v13, v13);
    __m128 v15 = _mm_add_ss(_mm_shuffle_ps(v14, v14, 170), _mm_add_ss(_mm_shuffle_ps(v14, v14, 85), v14));
    // One Newton step on rsqrt: 0.5 * r * (3 - len2 * r * r), zero where len2 is zero. The rsqrt is kept in v24 until
    // hkQuaternion::set overwrites it.
    __m128 v16 = _mm_rsqrt_ss(v15);
    v24.m_vec.m_quad = v16;
    __m128 v17 = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(v15, v16), v16));
    __m128 v20 = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), v16), v17);
    __m128 v21 = _mm_andnot_ps(_mm_cmpeq_ss(v15, _mm_setzero_ps()), v20);
    v22.m_quad = _mm_mul_ps(_mm_shuffle_ps(v21, v21, 0), v13);
    r.m_col0.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v22.m_quad, v22.m_quad, 210), v11),
        _mm_mul_ps(_mm_shuffle_ps(v22.m_quad, v22.m_quad, 201), v12)
    );
    r.m_col1.m_quad = v2.m_quad;
    r.m_col2.m_quad = v22.m_quad;
    v24.set(r);
    result.m_vec.m_quad = _mm_xor_ps(v24.m_vec.m_quad, rdata_10C4C00);
    v25.setMul(result, rotationInOut);
    rotationInOut.m_vec.m_quad = v25.m_vec.m_quad;
}

// The three bone indices and the rotation axis are stored as given; the candidate list starts empty, its storage
// belonging to somebody else until the utility fills it.
hkaPoseMatchingUtility::hkaPoseMatchingUtility(
    int16_t rootBoneIdx,
    int16_t otherBoneIdx,
    int16_t anotherBoneIdx,
    const hkVector4& rotAxis
) {
    m_rootBoneIdx = rootBoneIdx;
    m_otherBoneIdx = otherBoneIdx;
    m_candidatePoses.m_data = nullptr;
    m_candidatePoses.m_size = 0;
    m_candidatePoses.m_capacityAndFlags = hkArray<CandidatePoseInfo>::DONT_DEALLOCATE_FLAG;
    m_anotherBoneIdx = anotherBoneIdx;
    m_rotationAxis = rotAxis;
}

// The whole local CandidatePoseInfo is copied into the array's new slot, and the padding after m_time is never written,
// as the game does.
int32_t hkaPoseMatchingUtility::addCandidatePose(
    const hkQsTransform* poseModelSpace,
    const hkaAnimationBinding* binding,
    float time
) {
    CandidatePoseInfo info;
    info.m_time = time;
    info.m_binding = binding;
    computeOrientation(poseModelSpace, info.m_reference);
    hkRemoveTwist(m_rotationAxis, info.m_reference);
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (m_candidatePoses.m_size ==
        (m_candidatePoses.m_capacityAndFlags & hkArrayBase<hkaPoseMatchingUtility::CandidatePoseInfo>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &m_candidatePoses, sizeof(CandidatePoseInfo));
    }
    CandidatePoseInfo* pSlot = &m_candidatePoses.m_data[m_candidatePoses.m_size];
    if (pSlot) {
        *pSlot = info;
    }
    return m_candidatePoses.m_size++;
}

void hkaPoseMatchingUtility::computeOrientation(const hkQsTransform* poseModelSpace, hkQuaternion& frameOut) {
    __m128 v8 = _mm_undefined_ps();
    hkVector4 v18;
    __m128 v42 = _mm_undefined_ps();
    hkQuaternion v45;
    hkRotation r;
    hkVector4 v47;
    v47.m_quad = poseModelSpace[m_rootBoneIdx].m_translation.m_quad;
    __m128 v3 = _mm_sub_ps(poseModelSpace[m_otherBoneIdx].m_translation.m_quad, v47.m_quad);
    __m128 v4 = _mm_mul_ps(v3, v3);
    float v5 = _mm_shuffle_ps(v4, v4, 85)[0] + v4[0];
    __m128 v6 = _mm_shuffle_ps(v4, v4, 170);
    v6[0] = v6[0] + v5;
    float v7 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v6[0])));
    v8[0] = 0.0f;
    int32_t m_anotherBoneIdx = this->m_anotherBoneIdx;
    __m128 v11 = _mm_setzero_ps();
    v11[0] = 0.5f;
    const hkQsTransform* v13 = &poseModelSpace[m_anotherBoneIdx];
    float v14 = 3.0f - static_cast<float>(static_cast<float>(v6[0] * v7) * v7);
    __m128 v15 = _mm_cmpeq_ss(v6, v8);
    __m128 v16 = v11;
    v16[0] = static_cast<float>(v11[0] * v7) * v14;
    __m128 v17 = _mm_andnot_ps(v15, v16);
    __m128 v43 = v13->m_translation.m_quad;
    v18.m_quad = _mm_mul_ps(_mm_shuffle_ps(v17, v17, 0), v3);
    __m128 v19 = _mm_sub_ps(v43, v47.m_quad);
    __m128 v20 = _mm_mul_ps(v19, v19);
    v42[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    float v21 = _mm_shuffle_ps(v20, v20, 85)[0] + v20[0];
    __m128 v22 = _mm_shuffle_ps(v20, v20, 170);
    v22[0] = v22[0] + v21;
    float v23 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v22[0])));
    __m128 v24 = _mm_cmpeq_ss(v22, v8);
    v8[0] = 3.0f - static_cast<float>(static_cast<float>(v22[0] * v23) * v23);
    __m128 v25 = v11;
    v25[0] = static_cast<float>(v11[0] * v23) * v8[0];
    __m128 v26 = _mm_andnot_ps(v24, v25);
    __m128 v27 = _mm_mul_ps(_mm_shuffle_ps(v26, v26, 0), v19);
    __m128 v44 = _mm_shuffle_ps(v18.m_quad, v18.m_quad, 201);
    __m128 v28 = _mm_shuffle_ps(v18.m_quad, v18.m_quad, 210);
    __m128 v29 =
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v27, v27, 201), v28), _mm_mul_ps(_mm_shuffle_ps(v27, v27, 210), v44));
    __m128 v30 = _mm_mul_ps(v29, v29);
    float v31 = _mm_shuffle_ps(v30, v30, 85)[0] + v30[0];
    __m128 v32 = _mm_shuffle_ps(v30, v30, 170);
    v32[0] = v32[0] + v31;
    v47.m_quad[0] = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v32[0])));
    __m128 v33 = _mm_cmpeq_ss(v32, v42);
    v8[0] = 3.0f - static_cast<float>(static_cast<float>(v32[0] * v47.m_quad[0]) * v47.m_quad[0]);
    __m128 v34 = v11;
    v34[0] = static_cast<float>(v11[0] * v47.m_quad[0]) * v8[0];
    __m128 v35 = _mm_andnot_ps(v33, v34);
    __m128 v36 = _mm_mul_ps(_mm_shuffle_ps(v35, v35, 0), v29);
    __m128 v37 =
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v36, v36, 201), v28), _mm_mul_ps(_mm_shuffle_ps(v36, v36, 210), v44));
    __m128 v38 = _mm_mul_ps(v37, v37);
    v33[0] = _mm_shuffle_ps(v38, v38, 85)[0] + v38[0];
    __m128 m_quad_1 = _mm_shuffle_ps(v38, v38, 170);
    m_quad_1[0] = m_quad_1[0] + v33[0];
    // The rsqrt is kept in v45 until hkQuaternion::set overwrites it.
    v45.m_vec.m_quad = _mm_rsqrt_ss(m_quad_1);
    float v39 = v45.m_vec.m_quad[0];
    v11[0] = static_cast<float>(v11[0] * v39) *
        static_cast<float>(3.0f - static_cast<float>(static_cast<float>(m_quad_1[0] * v39) * v39));
    __m128 v41 = _mm_andnot_ps(_mm_cmpeq_ss(m_quad_1, v42), v11);
    r.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v41, v41, 0), v37);
    r.m_col1.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(r.m_col0.m_quad, r.m_col0.m_quad, 210), v44),
        _mm_mul_ps(_mm_shuffle_ps(r.m_col0.m_quad, r.m_col0.m_quad, 201), v28)
    );
    r.m_col2.m_quad = v18.m_quad;
    v45.set(r);
    frameOut.m_vec.m_quad = v45.m_vec.m_quad;
}

int32_t hkaPoseMatchingUtility::findBestCandidatePoseIndex(const hkQsTransform* poseModelSpace, float& error) {
    hkQuaternion a3;
    computeOrientation(poseModelSpace, a3);
    hkRemoveTwist(m_rotationAxis, a3);
    int32_t v4 = 0;
    error = 3.40282e+38f;
    int32_t v5 = -1;
    if (m_candidatePoses.m_size > 0) {
        __m128 m_quad = a3.m_vec.m_quad;
        do {
            __m128 v8 = _mm_mul_ps(m_candidatePoses.m_data[v4].m_reference.m_vec.m_quad, m_quad);
            __m128 v9 = _mm_add_ps(_mm_shuffle_ps(v8, v8, 78), v8);
            float v11 = std::fabs((double)static_cast<float>(_mm_shuffle_ps(v9, v9, 177)[0] + v9[0]));
            float v12 = 1.0 - v11;
            if (error > static_cast<double>(v12)) {
                error = v12;
                v5 = v4;
            }
            ++v4;
        }
        while (v4 < m_candidatePoses.m_size);
    }
    return v5;
}

// The ragdoll's frame turns the animation pose's orientation onto the ragdoll pose's about the root bone; the
// animation's frame is that frame's rotation about m_rotationAxis alone.
void hkaPoseMatchingUtility::computeReferenceFrame(
    const hkQsTransform* animPoseModelSpace,
    const hkQsTransform* ragdollPoseWorldSpace,
    hkQsTransform& animWorldFromModel,
    hkQsTransform& ragdollWorldFromModel
) {
    alignas(16) hkQuaternion animOrientation;
    alignas(16) hkQuaternion ragdollOrientation;
    computeOrientation(animPoseModelSpace, animOrientation);
    computeOrientation(ragdollPoseWorldSpace, ragdollOrientation);
    const hkQuadReal zero = _mm_setzero_ps();
    const hkQuadReal ones = _mm_set1_ps(1.0f);
    ragdollWorldFromModel.m_translation.m_quad = zero;
    ragdollWorldFromModel.m_rotation.setIdentity();
    ragdollWorldFromModel.m_scale.m_quad = ones;
    const int32_t iRoot = m_rootBoneIdx;
    ragdollWorldFromModel.m_translation.m_quad = ragdollPoseWorldSpace[iRoot].m_translation.m_quad;
    ragdollWorldFromModel.m_rotation.setMulInverse(ragdollOrientation, animOrientation);
    hkVector4 rotatedAnimRoot;
    rotatedAnimRoot.setRotatedDir(ragdollWorldFromModel.m_rotation, animPoseModelSpace[iRoot].m_translation);
    ragdollWorldFromModel.m_translation.sub4(rotatedAnimRoot);
    animWorldFromModel.m_translation.m_quad = zero;
    animWorldFromModel.m_rotation.setIdentity();
    animWorldFromModel.m_scale.m_quad = ones;
    animWorldFromModel.m_translation.m_quad = ragdollWorldFromModel.m_translation.m_quad;
    animWorldFromModel.m_rotation.m_vec.m_quad = ragdollWorldFromModel.m_rotation.m_vec.m_quad;
    animWorldFromModel.m_scale.m_quad = ragdollWorldFromModel.m_scale.m_quad;
    alignas(16) hkQuaternion rest;
    alignas(16) float angle;
    animWorldFromModel.m_rotation.decomposeRestAxis(m_rotationAxis, rest, angle);
    animWorldFromModel.m_rotation.setAxisAngle(m_rotationAxis, angle);
}
