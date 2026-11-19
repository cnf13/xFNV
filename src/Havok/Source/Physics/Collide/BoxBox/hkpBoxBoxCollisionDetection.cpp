#include "Havok/Source/Physics/Collide/BoxBox/hkpBoxBoxCollisionDetection.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/BoxBox/hkpBoxBoxContactPoint.h"
#include "Havok/Source/Physics/Collide/BoxBox/hkpBoxBoxManifold.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The vertex flips (bits 4-6 of a feature id) calcManifold tries for
// each additional face point, the last one none.
using VertexFlipArray = uint16_t[7];
static VertexFlipArray vertexFlipArray{80, 48, 96, 16, 32, 64};

double sub_DB09F0(int32_t r_eax) {
    return std::bit_cast<float>(((r_eax & 0xFFF8) << 28 | std::bit_cast<uint32_t>(static_cast<float>(1.0))));
}

int32_t markVertexEdges(uint32_t* a1, int32_t a2, int32_t r_eax) {
    int32_t v3 = r_eax >> 4;
    bool v4 = (v3 & 1) == 0;
    bool v5 = (v3 & 2) == 0;
    int32_t result = (v3 & 4) == 0;
    if (a2) {
        int32_t v7 = 1 << 2 * (v5 + 2 * result);
        if ((2 * v7 & *a1) == 0) {
            *a1 += v7;
        }
    }
    if (a2 != 1) {
        result = 1 << 2 * (v4 + 2 * result + 4);
        if ((2 * result & *a1) == 0) {
            *a1 += result;
        }
    }
    if (a2 != 2) {
        result = 1 << 2 * (v4 + 2 * v5 + 8);
        if ((2 * result & *a1) == 0) {
            *a1 += result;
        }
    }
    return result;
}

// The axis (the low nibble: 0, 1 or 2) and the sign bits (bits 4 to 6) of featureCode pick a bit pair of *bits: two
// sign bits of the other axes select one of four, the axis one of three groups. Its low bit is added unless the high
// bit is already set; returns the low bit.
int32_t markFaceEdge(int32_t featureCode, uint32_t* bits) {
    const int32_t signs = featureCode >> 4;
    const int32_t clearX = ~signs & 1;
    const int32_t clearY = ~(signs >> 1) & 1;
    const int32_t clearZ = ~(signs >> 2) & 1;
    const int32_t axis = featureCode & 15;
    int32_t index;
    if (axis == 0) {
        index = clearY + clearZ * 2;
    }
    else if (axis == 1) {
        index = clearX + clearZ * 2 + 4;
    }
    else {
        index = clearX + clearY * 2 + 8;
    }
    const int32_t bit = 1 << (index * 2);
    if (!(*bits & static_cast<uint32_t>(bit * 2))) {
        *bits += bit;
    }
    return bit;
}

const __m128* getVertexSign(int32_t r_eax) {
    return &vertexSignArray[(r_eax & 0x70) >> 4];
}

uint32_t calcVertexFeatureCode(uint32_t a1, const hkVector4* r_eax) {
    const int32_t signs = _mm_movemask_ps(_mm_cmplt_ps(r_eax->m_quad, _mm_setzero_ps()));
    return ((a1 >> 28) & 8) | ((signs & 7) << 4);
}

// The compares take the greater index when it is not below, unordered included.
int32_t getMaxComponentIndex(int32_t* r_edi, const float* r_esi) {
    int32_t index;
    int32_t result;
    if (!(r_esi[1] < r_esi[0])) {
        index = 1;
        result = 0x20;
    }
    else {
        index = 0;
        result = 0x10;
    }
    if (!(r_esi[index] < r_esi[2])) {
        *r_edi = index;
        return result;
    }
    *r_edi = 2;
    return 0x40;
}

// Copies both vectors, sets lanes a3 and a4 to 1.0 and takes the sign bits.
uint8_t calcFeatureCodes(uint8_t* a1, const hkVector4* a2, int32_t a3, int32_t a4, const hkVector4* r_eax) {
    hkVector4 vecA = *a2;
    hkVector4 vecB = *r_eax;
    // The lane stores address the locals' raw floats by index.
    reinterpret_cast<float*>(&vecA)[a3] = 1.0f;
    reinterpret_cast<float*>(&vecB)[a4] = 1.0f;
    const int32_t signsA = _mm_movemask_ps(_mm_cmplt_ps(vecA.m_quad, _mm_setzero_ps()));
    const int32_t signsB = _mm_movemask_ps(_mm_cmplt_ps(vecB.m_quad, _mm_setzero_ps()));
    const uint8_t featureA = static_cast<uint8_t>(((signsA | 0xF8) << 4) | a3);
    const uint8_t featureB = static_cast<uint8_t>(((~signsB & 7) << 4) + a4);
    a1[0] = featureA;
    a1[1] = featureB;
    return featureB;
}

// The SDK's calculateClosestPoint: the closest feature pair findClosestPoint finds (2: a new one) gives the contact.
// A face of A or B: the point is the other box's vertex in world space and the normal the face's axis times minus the
// normal sign; an edge pair: the point on A's edge and fpp's normal, both to world space. The normal's w and the
// contact's distance are fpp's distance; for a feature of B the point moves onto A's surface by minus the distance
// along the normal.
hkBool hkpBoxBoxCollisionDetection::calculateClosestPoint(hkContactPoint& contact) const {
    hkBool result;
    result.m_bool = 0;
    hkpBoxBoxManifold manifold;
    hkpFeatureContactPoint fcp;
    fcp.m_contactPointId = 0;
    initWorkVariables();
    if (checkIntersection(m_tolerance4) != HK_SUCCESS) {
        return result;
    }
    hkpFeaturePointCache fpp;
    if (findClosestPoint(manifold, fcp, fpp) != 2) {
        result.m_bool = 0;
        return result;
    }
    const int32_t featureIndexA = fpp.m_featureIndexA;
    const uint8_t featureIdA = fcp.m_featureIdA;
    result.m_bool = 1;
    __m128 position;
    __m128 normal;
    if (featureIndexA <= 2) {
        // A face of A: B's vertex to world space, A's face axis.
        const __m128 v = fpp.m_vB.m_quad;
        const hkTransform* wTb = m_wTb;
        const float negSign = -fpp.m_normalSign;
        position = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v, v, 85), wTb->m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0), wTb->m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 170), wTb->m_rotation.m_col2.m_quad), wTb->m_translation.m_quad)
        );
        normal = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(negSign), _mm_set_ss(negSign), 0), (&m_wTa->m_rotation.m_col0)[featureIdA].m_quad
        );
    }
    else if (featureIndexA <= 6) {
        // A face of B: A's vertex to world space, B's face axis.
        const __m128 v = fpp.m_vA.m_quad;
        const hkTransform* wTa = m_wTa;
        const float negSign = -fpp.m_normalSign;
        position = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v, v, 85), wTa->m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0), wTa->m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 170), wTa->m_rotation.m_col2.m_quad), wTa->m_translation.m_quad)
        );
        normal = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(negSign), _mm_set_ss(negSign), 0),
            (&m_wTb->m_rotation.m_col0)[featureIdA - 4].m_quad
        );
    }
    else {
        // An edge pair: the point on A's edge to world space, fpp's normal rotated.
        const __m128 v = fpp.m_vA.m_quad;
        const __m128 n = fpp.m_nA.m_quad;
        const hkTransform* wTa = m_wTa;
        position = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v, v, 85), wTa->m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0), wTa->m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 170), wTa->m_rotation.m_col2.m_quad), wTa->m_translation.m_quad)
        );
        normal = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(n, n, 85), wTa->m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(n, n, 0), wTa->m_rotation.m_col0.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(n, n, 170), wTa->m_rotation.m_col2.m_quad)
        );
    }
    const float distance = fpp.m_distance;
    XFNV_M128_F32(normal, 3) = distance;
    if (featureIdA > 2u) {
        const float negDistance = -distance;
        position = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(negDistance), _mm_set_ss(negDistance), 0), normal), position
        );
    }
    contact.m_separatingNormal.m_quad = normal;
    contact.m_position.m_quad = position;
    XFNV_M128_F32(contact.m_separatingNormal.m_quad, 3) = distance;
    return result;
}

// The SDK's separating axis test over the six face axes and the nine edge cross axes; each axis set's distances,
// scaled by the axes' inverse lengths for the edge sets, are cached in m_sepDist (an edge axis shorter than 1/1000
// gets -1e17).
hkResult hkpBoxBoxCollisionDetection::checkIntersection(const hkVector4& tolerance) const {
    hkVector4 v3;
    hkVector4 v17;
    hkVector4 v24;
    float v32;
    float v51;
    float v52;
    __m128 v53;
    __m128 v54;
    __m128 v55;
    float v56;
    float v57;
    float v58;
    float v59;
    float v60;
    float v61;
    hkVector4 v73;
    hkMatrix3 v75;
    v3.m_quad = m_radiusB.m_quad;
    __m128 v4 = _mm_and_ps(m_aTb.m_rotation.m_col0.m_quad, hkVector4_setAbs4_absMask);
    __m128 v5 = _mm_and_ps(m_aTb.m_rotation.m_col1.m_quad, hkVector4_setAbs4_absMask);
    __m128 v6 = _mm_and_ps(m_aTb.m_rotation.m_col2.m_quad, hkVector4_setAbs4_absMask);
    __m128 m_quad = _mm_sub_ps(
        _mm_and_ps(m_dinA.m_quad, hkVector4_setAbs4_absMask),
        _mm_add_ps(
            _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 85), v5),
                    _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 0), v4)
                ),
                _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 170), v6)
            ),
            m_radiusA.m_quad
        )
    );
    if ((_mm_movemask_ps(_mm_cmplt_ps(tolerance.m_quad, m_quad)) & 7) != 0) {
        return HK_FAILURE;
    }
    m_sepDist[0].m_quad = m_quad;
    __m128 v9 = _mm_unpacklo_ps(v4, v5);
    __m128 v10 = _mm_movelh_ps(v9, v6);
    __m128 v11 = _mm_sub_ps(
        _mm_and_ps(m_dinB.m_quad, hkVector4_setAbs4_absMask),
        _mm_add_ps(
            _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(m_radiusA.m_quad, m_radiusA.m_quad, 85),
                        _mm_shuffle_ps(_mm_movehl_ps(v10, v9), v6, 212)
                    ),
                    _mm_mul_ps(_mm_shuffle_ps(m_radiusA.m_quad, m_radiusA.m_quad, 0), v10)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(m_radiusA.m_quad, m_radiusA.m_quad, 170),
                    _mm_shuffle_ps(_mm_unpackhi_ps(v4, v5), v6, 228)
                )
            ),
            m_radiusB.m_quad
        )
    );
    if ((_mm_movemask_ps(_mm_cmplt_ps(tolerance.m_quad, v11)) & 7) != 0) {
        return HK_FAILURE;
    }
    m_sepDist[1].m_quad = v11;
    v75.setTranspose(m_aTb.m_rotation);
    __m128 v74 = _mm_mul_ps(v75.m_col0.m_quad, v75.m_col0.m_quad);
    __m128 v12 = _mm_shuffle_ps(m_dinA.m_quad, m_dinA.m_quad, 170);
    __m128 v67 = _mm_mul_ps(_mm_shuffle_ps(v12, v12, 0), v75.m_col1.m_quad);
    __m128 v13 = _mm_mul_ps(v75.m_col2.m_quad, v75.m_col2.m_quad);
    __m128 v69 = _mm_shuffle_ps(m_dinA.m_quad, m_dinA.m_quad, 85);
    __m128 v14 = _mm_mul_ps(v75.m_col1.m_quad, v75.m_col1.m_quad);
    __m128 v15 = _mm_add_ps(v13, v14);
    double v16 = XFNV_M128_F32(v4, 2) * XFNV_M128_F32(m_radiusA.m_quad, 1) +
        XFNV_M128_F32(v4, 1) * XFNV_M128_F32(m_radiusA.m_quad, 2) +
        XFNV_M128_F32(v6, 0) * XFNV_M128_F32(m_radiusB.m_quad, 1) +
        XFNV_M128_F32(v5, 0) * XFNV_M128_F32(m_radiusB.m_quad, 2);
    v59 = v16;
    v56 = XFNV_M128_F32(v6, 0) * XFNV_M128_F32(m_radiusB.m_quad, 0) +
        (XFNV_M128_F32(v5, 2) * XFNV_M128_F32(m_radiusA.m_quad, 1) +
         XFNV_M128_F32(v5, 1) * XFNV_M128_F32(m_radiusA.m_quad, 2)) +
        XFNV_M128_F32(v4, 0) * XFNV_M128_F32(m_radiusB.m_quad, 2);
    v32 = XFNV_M128_F32(v5, 0) * XFNV_M128_F32(m_radiusB.m_quad, 0) +
        (XFNV_M128_F32(v6, 2) * XFNV_M128_F32(m_radiusA.m_quad, 1) +
         XFNV_M128_F32(v6, 1) * XFNV_M128_F32(m_radiusA.m_quad, 2)) +
        XFNV_M128_F32(v4, 0) * XFNV_M128_F32(m_radiusB.m_quad, 1);
    float v45 = XFNV_M128_F32(v15, 0) + 1.1920928955078125e-07;
    float v46 = std::sqrt((double)v45);
    XFNV_M128_F32(v53, 0) = 1.0 / v46;
    float v47 = XFNV_M128_F32(v15, 1) + 1.1920928955078125e-07;
    float v48 = std::sqrt((double)v47);
    XFNV_M128_F32(v53, 1) = 1.0 / v48;
    float v49 = XFNV_M128_F32(v15, 2) + 1.1920928955078125e-07;
    float v50 = std::sqrt((double)v49);
    v17.m_quad = v75.m_col2.m_quad;
    XFNV_M128_F32(v53, 2) = 1.0 / v50;
    XFNV_M128_F32(v53, 3) = 0.0f;
    __m128 v18 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_and_ps(
                _mm_sub_ps(v67, _mm_mul_ps(_mm_shuffle_ps(v69, v69, 0), v75.m_col2.m_quad)), hkVector4_setAbs4_absMask
            ),
            _mm_unpacklo_ps(
                _mm_unpacklo_ps(_mm_set_ss(v59), _mm_set_ss(v32)), _mm_unpacklo_ps(_mm_set_ss(v56), _mm_set_ss(zero))
            )
        ),
        v53
    );
    v73.m_quad = m_tolerance4.m_quad;
    if ((_mm_movemask_ps(_mm_cmplt_ps(v73.m_quad, v18)) & 7) != 0) {
        return HK_FAILURE;
    }
    __m128 v68 = _mm_shuffle_ps(_mm_set_ss(999.99994f), _mm_set_ss(999.99994f), 0);
    __m128 v19 = _mm_cmplt_ps(v68, v53);
    double v20 = XFNV_M128_F32(v4, 0) * XFNV_M128_F32(m_radiusA.m_quad, 2) +
        XFNV_M128_F32(v4, 2) * XFNV_M128_F32(m_radiusA.m_quad, 0);
    __m128 v21 = _mm_shuffle_ps(_mm_set_ss(-1e17f), _mm_set_ss(-1e17f), 0);
    double v22 = XFNV_M128_F32(v6, 1) * XFNV_M128_F32(m_radiusB.m_quad, 1);
    m_sepDist[2].m_quad = _mm_or_ps(_mm_and_ps(v19, v21), _mm_andnot_ps(v19, v18));
    __m128 v72 = _mm_mul_ps(_mm_shuffle_ps(m_dinA.m_quad, m_dinA.m_quad, 0), v17.m_quad);
    __m128 v65 = _mm_shuffle_ps(m_dinA.m_quad, m_dinA.m_quad, 170);
    __m128 v23 = _mm_add_ps(v13, v74);
    v57 = v20 + v22 + XFNV_M128_F32(v5, 1) * XFNV_M128_F32(m_radiusB.m_quad, 2);
    v60 = XFNV_M128_F32(v6, 1) * XFNV_M128_F32(m_radiusB.m_quad, 0) +
        (XFNV_M128_F32(v5, 2) * XFNV_M128_F32(m_radiusA.m_quad, 0) +
         XFNV_M128_F32(v5, 0) * XFNV_M128_F32(m_radiusA.m_quad, 2)) +
        XFNV_M128_F32(v4, 1) * XFNV_M128_F32(m_radiusB.m_quad, 2);
    v51 = XFNV_M128_F32(v5, 1) * XFNV_M128_F32(m_radiusB.m_quad, 0) +
        (XFNV_M128_F32(v6, 2) * XFNV_M128_F32(m_radiusA.m_quad, 0) +
         XFNV_M128_F32(v6, 0) * XFNV_M128_F32(m_radiusA.m_quad, 2)) +
        XFNV_M128_F32(v4, 1) * XFNV_M128_F32(m_radiusB.m_quad, 1);
    float v33 = XFNV_M128_F32(v23, 0) + 1.1920928955078125e-07;
    float v34 = std::sqrt((double)v33);
    XFNV_M128_F32(v54, 0) = 1.0 / v34;
    float v35 = XFNV_M128_F32(v23, 1) + 1.1920928955078125e-07;
    float v36 = std::sqrt((double)v35);
    XFNV_M128_F32(v54, 1) = 1.0 / v36;
    float v37 = XFNV_M128_F32(v23, 2) + 1.1920928955078125e-07;
    float v38 = std::sqrt((double)v37);
    v24.m_quad = v75.m_col0.m_quad;
    XFNV_M128_F32(v54, 2) = 1.0 / v38;
    XFNV_M128_F32(v54, 3) = 0.0f;
    __m128 v25 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_and_ps(
                _mm_sub_ps(v72, _mm_mul_ps(_mm_shuffle_ps(v65, v65, 0), v75.m_col0.m_quad)), hkVector4_setAbs4_absMask
            ),
            _mm_unpacklo_ps(
                _mm_unpacklo_ps(_mm_set_ss(v57), _mm_set_ss(v51)), _mm_unpacklo_ps(_mm_set_ss(v60), _mm_set_ss(zero))
            )
        ),
        v54
    );
    if ((_mm_movemask_ps(_mm_cmplt_ps(v73.m_quad, v25)) & 7) != 0) {
        return HK_FAILURE;
    }
    __m128 v26 = _mm_cmplt_ps(v68, v54);
    double v27 = XFNV_M128_F32(v4, 0) * XFNV_M128_F32(m_radiusA.m_quad, 1) +
        XFNV_M128_F32(v4, 1) * XFNV_M128_F32(m_radiusA.m_quad, 0);
    m_sepDist[3].m_quad = _mm_or_ps(_mm_and_ps(v26, v21), _mm_andnot_ps(v26, v25));
    __m128 v28 = _mm_shuffle_ps(m_dinA.m_quad, m_dinA.m_quad, 85);
    __m128 v66 = _mm_mul_ps(_mm_shuffle_ps(v28, v28, 0), v24.m_quad);
    __m128 v29 = _mm_add_ps(v74, v14);
    v58 = v27 + XFNV_M128_F32(v6, 2) * XFNV_M128_F32(m_radiusB.m_quad, 1) +
        XFNV_M128_F32(v5, 2) * XFNV_M128_F32(m_radiusB.m_quad, 2);
    v61 = XFNV_M128_F32(v6, 2) * XFNV_M128_F32(m_radiusB.m_quad, 0) +
        (XFNV_M128_F32(v5, 1) * XFNV_M128_F32(m_radiusA.m_quad, 0) +
         XFNV_M128_F32(v5, 0) * XFNV_M128_F32(m_radiusA.m_quad, 1) +
         XFNV_M128_F32(v4, 2) * XFNV_M128_F32(m_radiusB.m_quad, 2));
    v52 = XFNV_M128_F32(v5, 2) * XFNV_M128_F32(m_radiusB.m_quad, 0) +
        (XFNV_M128_F32(v6, 1) * XFNV_M128_F32(m_radiusA.m_quad, 0) +
         XFNV_M128_F32(v6, 0) * XFNV_M128_F32(m_radiusA.m_quad, 1)) +
        XFNV_M128_F32(v4, 2) * XFNV_M128_F32(m_radiusB.m_quad, 1);
    float v39 = XFNV_M128_F32(v29, 0) + 1.1920928955078125e-07;
    float v40 = std::sqrt((double)v39);
    XFNV_M128_F32(v55, 0) = 1.0 / v40;
    float v41 = XFNV_M128_F32(v29, 1) + 1.1920928955078125e-07;
    float v42 = std::sqrt((double)v41);
    XFNV_M128_F32(v55, 1) = 1.0 / v42;
    float v43 = XFNV_M128_F32(v29, 2) + 1.1920928955078125e-07;
    float v44 = std::sqrt((double)v43);
    XFNV_M128_F32(v55, 2) = 1.0 / v44;
    XFNV_M128_F32(v55, 3) = 0.0f;
    __m128 v30 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_and_ps(
                _mm_sub_ps(v66, _mm_mul_ps(_mm_shuffle_ps(m_dinA.m_quad, m_dinA.m_quad, 0), v75.m_col1.m_quad)),
                hkVector4_setAbs4_absMask
            ),
            _mm_unpacklo_ps(
                _mm_unpacklo_ps(_mm_set_ss(v58), _mm_set_ss(v52)), _mm_unpacklo_ps(_mm_set_ss(v61), _mm_set_ss(zero))
            )
        ),
        v55
    );
    if ((_mm_movemask_ps(_mm_cmplt_ps(v73.m_quad, v30)) & 7) != 0) {
        return HK_FAILURE;
    }
    __m128 v31 = _mm_cmplt_ps(v68, v55);
    m_sepDist[4].m_quad = _mm_or_ps(_mm_andnot_ps(v31, v30), _mm_and_ps(v31, v21));
    return HK_SUCCESS;
}

// The SDK's isValidEdgeEdge: the closest points of A's edge fpp.m_featureIndexA and B's edge fpp.m_featureIndexB
// (axis in the low four bits, vertex in bits 4-6), in A's space. The pair is rejected when the edges are parallel, the
// closest point lies outside either edge, the edges' cross product is shorter than the boundary tolerance or the
// distance along it exceeds the tolerance; else fpp gets A's point, the normal (pointing from B to A) and the
// distance.
// KEEP: the exe's findClosestPoint (0xDB1ED0) stores through ecx at 0xDB2341 after calling this without loading it
// again: it relies on ecx (this) left intact, which the exe body never writes and clang does not guarantee.
hkBool hkpBoxBoxCollisionDetection::isValidEdgeEdge(hkpFeaturePointCache& fpp) const {
    hkBool result;
    const int32_t edgeB = fpp.m_featureIndexB & 15;
    const __m128 signA = vertexSignArray[(fpp.m_featureIndexA & 0x70) >> 4];
    __m128 v4 = _mm_mul_ps(m_radiusA.m_quad, signA);
    double v5 = XFNV_M128_F32(m_radiusB.m_quad, edgeB);
    const __m128 signB = vertexSignArray[(fpp.m_featureIndexB & 0x70) >> 4];
    const int32_t edgeA = fpp.m_featureIndexA & 15;
    hkVector4 v8;
    v8.m_quad = m_aTb.m_rotation.m_col0.m_quad;
    const float v41 = v5 + v5;
    __m128 v9 = _mm_mul_ps(signB, m_radiusB.m_quad);
    __m128 v10 = _mm_set_ss(v41);
    float v42 = XFNV_M128_F32(m_radiusA.m_quad, edgeA) * -2.0;
    __m128 v11 = _mm_mul_ps(_mm_shuffle_ps(v10, v10, 0), (&m_aTb.m_rotation.m_col0)[edgeB].m_quad);
    hkVector4 v12;
    v12.m_quad = m_aTb.m_rotation.m_col1.m_quad;
    __m128 v53 = v11;
    double v13 = v42;
    float v37 = XFNV_M128_F32(v53, edgeA) * v42;
    double v14 = XFNV_M128_F32(m_radiusB.m_quad, edgeB);
    float v44 = v14 * v14 * 4.0;
    __m128 v15 = _mm_sub_ps(
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v9, v9, 85), v12.m_quad), _mm_mul_ps(_mm_shuffle_ps(v9, v9, 0), v8.m_quad)
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v9, v9, 170), m_aTb.m_rotation.m_col2.m_quad), m_aTb.m_translation.m_quad
            )
        ),
        v4
    );
    double v16 = v37;
    __m128 v17 = _mm_mul_ps(v15, v11);
    double v18 = v37;
    float v38 = v42 * v42;
    float v43 = XFNV_M128_F32(_mm_shuffle_ps(v17, v17, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v17, v17, 85), 0) + XFNV_M128_F32(v17, 0));
    double v19 = v38 * v44 - v18 * v18;
    double v20 = v44;
    float v45 = v19;
    float v46 = std::fabs((double)v45);
    float v47 = XFNV_M128_F32(v15, edgeA) * v13;
    float v39 = v47 * v20 - v43 * v16;
    double v21 = v46;
    float v49 = 1.1920928955078125e-07 * v46;
    if (v49 >= static_cast<double>(v39)) {
        result.m_bool = 0;
        return result;
    }
    if (v21 * 0.9999998807907104 <= v39) {
        result.m_bool = 0;
        return result;
    }
    float v40 = v39 / v21;
    float v50 = v16 * v40 - v43;
    if (v20 * 1.1920928955078125e-07 >= v50) {
        result.m_bool = 0;
        return result;
    }
    double v22 = v50;
    float v51 = 0.9999998807907104 * v20;
    if (v51 <= v22) {
        result.m_bool = 0;
        return result;
    }
    v53 = _mm_setzero_ps();
    XFNV_M128_F32(v4, edgeA) = v40 * v13 + XFNV_M128_F32(v4, edgeA);
    XFNV_M128_F32(v53, edgeA) = v13 * 0.5;
    // The edges' cross product, normalised with the SDK's rsqrt and one Newton step (zero for a zero length).
    __m128 v23 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v53, v53, 201), _mm_shuffle_ps(v11, v11, 210)),
        _mm_mul_ps(_mm_shuffle_ps(v53, v53, 210), _mm_shuffle_ps(v11, v11, 201))
    );
    __m128 v24 = _mm_mul_ps(v23, v23);
    __m128 v25 = _mm_shuffle_ps(v24, v24, 170);
    XFNV_M128_F32(v25, 0) =
        XFNV_M128_F32(v25, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v24, v24, 85), 0) + XFNV_M128_F32(v24, 0));
    float v26 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v25, 0))));
    __m128 v28 = _mm_cmpeq_ss(v25, _mm_set_ss(zero));
    __m128 v29 = _mm_setzero_ps();
    XFNV_M128_F32(v29, 0) = (0.5f * v26) * (3.0f - ((XFNV_M128_F32(v25, 0) * v26) * v26));
    __m128 v30 = _mm_andnot_ps(v28, v29);
    __m128 v31 = _mm_shuffle_ps(v30, v30, 0);
    __m128 v32 = _mm_mul_ps(v31, v23);
    if (m_boundaryTolerance > static_cast<double>(
                                  ((XFNV_M128_F32(_mm_shuffle_ps(v24, v24, 170), 0) +
                                    (XFNV_M128_F32(_mm_shuffle_ps(v24, v24, 85), 0) + XFNV_M128_F32(v24, 0))) *
                                   XFNV_M128_F32(v31, 0))
                              )) {
        result.m_bool = 0;
        return result;
    }
    __m128 v33 = _mm_mul_ps(signA, v32);
    if ((XFNV_M128_F32(_mm_shuffle_ps(v33, v33, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v33, v33, 85), 0) + XFNV_M128_F32(v33, 0))) < 0.0f) {
        v32 = _mm_xor_ps(v32, hkQuadRealNegateMask);
    }
    __m128 v34 = _mm_mul_ps(v32, v15);
    float distance =
        (XFNV_M128_F32(_mm_shuffle_ps(v34, v34, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v34, v34, 85), 0) + XFNV_M128_F32(v34, 0)));
    // An unordered (NaN) distance is also taken as within the tolerance.
    if (m_tolerance < distance) {
        result.m_bool = 0;
    }
    else {
        fpp.m_vA.m_quad = v4;
        fpp.m_distance = distance;
        fpp.m_nA.m_quad = _mm_xor_ps(v32, hkQuadRealNegateMask);
        result.m_bool = 1;
    }
    return result;
}

// Up to eight times the feature with the largest cached separating distance (a face of A, a face of B or an edge pair;
// the w lanes of m_sepDist are no features) gives a candidate point: fpp gets its points, normal sign and feature
// indices, fcp its feature ids. Returns 1 when the manifold already holds that feature pair, 2 when the point lies
// within the keep radii (an edge pair: when isValidEdgeEdge accepts it); otherwise the feature's distance becomes
// -3.40282e+38 and the next one is tried, 0 after the eighth.
int32_t hkpBoxBoxCollisionDetection::findClosestPoint(
    hkpBoxBoxManifold& manifold,
    hkpFeatureContactPoint& fcp,
    hkpFeaturePointCache& fpp
) const {
    for (int32_t iterationsLeft = 8;;) {
        uint32_t feature = HK_BOXBOX_FACE_A_X;
        float maxDist = XFNV_M128_F32(m_sepDist[0].m_quad, 0);
        for (uint32_t i = HK_BOXBOX_FACE_A_Y; i < HK_BOXBOX_FEATURE_COUNT; ++i) {
            if ((i & 3) != 3 && maxDist < XFNV_M128_F32(m_sepDist[i >> 2].m_quad, i & 3)) {
                feature = i;
                maxDist = XFNV_M128_F32(m_sepDist[i >> 2].m_quad, i & 3);
            }
        }

        if (feature <= HK_BOXBOX_FACE_A_Z) {
            // A face of A: B's vertex nearest to it, flipped by the side of A's centre (the sign of m_dinB).
            fcp.m_featureIdA = static_cast<uint8_t>(feature);
            fpp.m_featureIndexA = static_cast<int32_t>(feature);
            // Row `feature` of aTb's rotation, w 0.
            const __m128 row = _mm_unpacklo_ps(
                _mm_unpacklo_ps(
                    _mm_set_ss(XFNV_M128_F32(m_aTb.getColumn(0).m_quad, feature)),
                    _mm_set_ss(XFNV_M128_F32(m_aTb.getColumn(2).m_quad, feature))
                ),
                _mm_unpacklo_ps(_mm_set_ss(XFNV_M128_F32(m_aTb.getColumn(1).m_quad, feature)), _mm_set_ss(0.0f))
            );
            if (0.0f > XFNV_M128_F32(m_dinA.m_quad, feature)) {
                fpp.m_normalSign = -1.0f;
                fpp.m_vB = m_radiusB;
            }
            else {
                fpp.m_normalSign = 1.0f;
                fpp.m_vB.setNeg4(m_radiusB);
            }
            const __m128 inside = _mm_cmplt_ps(_mm_set1_ps(0.0f), m_dinB.m_quad);
            const __m128 boundary =
                _mm_or_ps(_mm_and_ps(_mm_set1_ps(1e-05f), inside), _mm_andnot_ps(inside, _mm_set1_ps(-1e-05f)));
            const __m128 offset = _mm_add_ps(_mm_mul_ps(boundary, _mm_set1_ps(fpp.m_normalSign)), row);
            fpp.m_vB.m_quad = _mm_xor_ps(_mm_and_ps(offset, hkQuadRealNegateMask), fpp.m_vB.m_quad);
            fpp.m_vA.setTransformedPos(m_aTb, fpp.m_vB);
            const uint8_t featureIdB = static_cast<uint8_t>(
                ((_mm_movemask_ps(_mm_cmplt_ps(fpp.m_vB.m_quad, _mm_setzero_ps())) & 7) << 4) |
                ((std::bit_cast<uint32_t>(fpp.m_normalSign) >> 28) & 8)
            );
            fcp.m_featureIdB = featureIdB;
            // EXE-INLINED 0xDB09B0: hkpBoxBoxManifold::findInManifold
            for (int32_t i = manifold.m_numPoints - 1; i >= 0; --i) {
                if (manifold.m_contactPoints[i].m_featureIdA == fcp.m_featureIdA &&
                    manifold.m_contactPoints[i].m_featureIdB == featureIdB) {
                    return 1;
                }
            }
            hkVector4 absA;
            absA.setAbs4(fpp.m_vA);
            if ((_mm_movemask_ps(_mm_cmplt_ps(absA.m_quad, m_keepRadiusA.m_quad)) & 7) == 7) {
                fpp.m_distance = XFNV_M128_F32(fpp.m_vA.m_quad, fpp.m_featureIndexA) * fpp.m_normalSign -
                    XFNV_M128_F32(m_radiusA.m_quad, fpp.m_featureIndexA);
                return 2;
            }
        }
        else if (feature <= HK_BOXBOX_FACE_B_Z) {
            // A face of B: A's vertex nearest to it, flipped by the side of B's centre (the sign of m_dinA).
            fcp.m_featureIdA = static_cast<uint8_t>(feature);
            fpp.m_featureIndexA = static_cast<int32_t>(feature);
            const int32_t axis = static_cast<int32_t>(feature) - HK_BOXBOX_FACE_B_X;
            const __m128 column = m_aTb.getColumn(axis).m_quad;
            if (0.0f > XFNV_M128_F32(m_dinB.m_quad, axis)) {
                fpp.m_normalSign = -1.0f;
                fpp.m_vA.setNeg4(m_radiusA);
            }
            else {
                fpp.m_normalSign = 1.0f;
                fpp.m_vA = m_radiusA;
            }
            const float negSign = -fpp.m_normalSign;
            const __m128 inside = _mm_cmplt_ps(_mm_set1_ps(0.0f), m_dinA.m_quad);
            const __m128 boundary =
                _mm_or_ps(_mm_and_ps(_mm_set1_ps(-1e-05f), inside), _mm_andnot_ps(inside, _mm_set1_ps(1e-05f)));
            const __m128 offset = _mm_add_ps(_mm_mul_ps(boundary, _mm_set1_ps(negSign)), column);
            fpp.m_vA.m_quad = _mm_xor_ps(_mm_and_ps(offset, hkQuadRealNegateMask), fpp.m_vA.m_quad);
            fpp.m_vB.setTransformedInversePos(m_aTb, fpp.m_vA);
            const uint8_t featureIdB = static_cast<uint8_t>(
                ((_mm_movemask_ps(_mm_cmplt_ps(fpp.m_vA.m_quad, _mm_setzero_ps())) & 7) << 4) |
                ((std::bit_cast<uint32_t>(fpp.m_normalSign) >> 28) & 8)
            );
            fcp.m_featureIdB = featureIdB;
            // EXE-INLINED 0xDB09B0: hkpBoxBoxManifold::findInManifold
            for (int32_t i = manifold.m_numPoints - 1; i >= 0; --i) {
                if (manifold.m_contactPoints[i].m_featureIdA == fcp.m_featureIdA &&
                    manifold.m_contactPoints[i].m_featureIdB == featureIdB) {
                    return 1;
                }
            }
            hkVector4 absB;
            absB.setAbs4(fpp.m_vB);
            if ((_mm_movemask_ps(_mm_cmplt_ps(absB.m_quad, m_keepRadiusB.m_quad)) & 7) == 7) {
                const int32_t i = fpp.m_featureIndexA - HK_BOXBOX_FACE_B_X;
                fpp.m_distance =
                    XFNV_M128_F32(fpp.m_vB.m_quad, i) * -fpp.m_normalSign - XFNV_M128_F32(m_radiusB.m_quad, i);
                return 2;
            }
        }
        else {
            // An edge pair: A's edge axis edgeA and B's edge along aTb's column edgeB; the normal is their cross
            // product, turned away from A's centre.
            const uint32_t edgeA = (feature - HK_BOXBOX_EDGE_0_0) >> 2;
            const uint32_t edgeB = (feature - HK_BOXBOX_EDGE_0_0) & 3;
            fpp.m_featureIndexA = static_cast<int32_t>(edgeA);
            fpp.m_featureIndexB = static_cast<int32_t>(edgeB);
            const __m128 dirB = m_aTb.getColumn(static_cast<int32_t>(edgeB)).m_quad;
            __m128 dirA = _mm_setzero_ps();
            XFNV_M128_F32(dirA, edgeA) = 1.0f;
            // EXE-INLINED 0x630DF0: hkVector4::setCross(dirA, dirB), each product with dirB's lanes first.
            hkVector4 normal;
            normal.m_quad = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(dirB, dirB, 0xD2), _mm_shuffle_ps(dirA, dirA, 0xC9)),
                _mm_mul_ps(_mm_shuffle_ps(dirB, dirB, 0xC9), _mm_shuffle_ps(dirA, dirA, 0xD2))
            );
            if (0.0f > _mm_cvtss_f32(m_dinA.dot3(normal).getQuad())) {
                normal.setNeg4(normal);
            }
            fpp.m_vA = normal;
            __m128 signsA = fpp.m_vA.m_quad;
            XFNV_M128_F32(signsA, edgeA) = 1.0f;
            fpp.m_vB.setRotatedInverseDir(m_aTb.getRotation(), normal);
            __m128 signsB = fpp.m_vB.m_quad;
            XFNV_M128_F32(signsB, edgeB) = 1.0f;
            // The vertex bits: A's from the normal's signs (bit 7 set), B's from the inverted signs.
            const uint8_t featureIdA =
                static_cast<uint8_t>(((_mm_movemask_ps(_mm_cmplt_ps(signsA, _mm_setzero_ps())) | 0xF8) << 4) | edgeA);
            const uint8_t featureIdB =
                static_cast<uint8_t>(((~_mm_movemask_ps(_mm_cmplt_ps(signsB, _mm_setzero_ps())) & 7) << 4) + edgeB);
            fcp.m_featureIdA = featureIdA;
            fcp.m_featureIdB = featureIdB;
            fpp.m_featureIndexA = featureIdA;
            fpp.m_featureIndexB = fcp.m_featureIdB;
            // EXE-INLINED 0xDB09B0: hkpBoxBoxManifold::findInManifold
            for (int32_t i = manifold.m_numPoints - 1; i >= 0; --i) {
                if (manifold.m_contactPoints[i].m_featureIdA == fcp.m_featureIdA &&
                    manifold.m_contactPoints[i].m_featureIdB == fcp.m_featureIdB) {
                    return 1;
                }
            }
            if (isValidEdgeEdge(fpp).m_bool) {
                return 2;
            }
        }

        XFNV_M128_F32(m_sepDist[feature >> 2].m_quad, feature & 3) = -3.40282e+38f;
        if (--iterationsLeft <= 0) {
            return 0;
        }
    }
}

// A feature pair the manifold lacks becomes a new point when its edge-edge closest point is valid and at least 0.999 of
// closestPointDist away, no face point of the manifold lies within the tolerance of it (in world space; for an edge
// pair the test is skipped), the manifold has room, and, past the first point, its normal does not point against the
// previous contact's. The contact manager then takes it; a rejected point leaves the manifold again.
void hkpBoxBoxCollisionDetection::addAdditionalEdgeHelper(
    hkpBoxBoxManifold& manifold,
    hkpFeatureContactPoint& fcp,
    float closestPointDist
) const {
    if (manifold.findInManifold(fcp).m_bool) {
        return;
    }
    alignas(16) hkpFeaturePointCache fpp;
    fpp.m_normalSign = 0.0f;
    fpp.m_featureIndexA = fcp.m_featureIdA;
    fpp.m_vA.m_quad = _mm_setzero_ps();
    fpp.m_vB.m_quad = _mm_setzero_ps();
    fpp.m_featureIndexB = fcp.m_featureIdB;
    if (!isValidEdgeEdge(fpp).m_bool) {
        return;
    }
    // The point is also skipped when the compare is unordered.
    if (!(closestPointDist * 0.9990000128746033 <= fpp.m_distance)) {
        return;
    }
    if (fcp.m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
        // The new point in world space: B's point for a face of A, A's point for a face of B.
        __m128 position;
        if (fcp.m_featureIdA <= HK_BOXBOX_FACE_A_Z) {
            const __m128 v = fpp.m_vB.m_quad;
            const hkTransform* wTb = m_wTb;
            position = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), wTb->m_rotation.m_col1.m_quad),
                    _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), wTb->m_rotation.m_col0.m_quad)
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), wTb->m_rotation.m_col2.m_quad), wTb->m_translation.m_quad
                )
            );
        }
        else {
            const __m128 v = fpp.m_vA.m_quad;
            const hkTransform* wTa = m_wTa;
            position = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), wTa->m_rotation.m_col1.m_quad),
                    _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), wTa->m_rotation.m_col0.m_quad)
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), wTa->m_rotation.m_col2.m_quad), wTa->m_translation.m_quad
                )
            );
        }
        // A face point of the manifold within the tolerance of it (squared, plus the float epsilon) already covers it.
        for (int32_t i = 0; i < manifold.m_numPoints; i++) {
            if (manifold.m_contactPoints[i].m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
                const __m128 diff = _mm_sub_ps(position, m_result->m_contactPoints[i].m_contact.m_position.m_quad);
                const __m128 sq = _mm_mul_ps(diff, diff);
                const float distSq = _mm_cvtss_f32(
                    _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
                );
                const float limit = m_tolerance * m_tolerance + 1.1920928955078125e-07;
                if (limit >= distSq) {
                    return;
                }
            }
        }
    }
    if (manifold.m_numPoints >= hkpBoxBoxManifold::HK_BOXBOX_MANIFOLD_MAX_POINTS) {
        return;
    }
    const int32_t index = manifold.addPoint(*m_bodyA, *m_bodyB, fcp);
    if (index < 0) {
        return;
    }
    hkpProcessCdPoint* cdPoint = m_result->m_firstFreeContactPoint.m_storage;
    __m128 normal;
    if (fpp.m_featureIndexA <= HK_BOXBOX_FACE_A_Z) {
        // A face of A: B's point to world space, minus A's face axis.
        const float negSign = -fpp.m_normalSign;
        cdPoint->m_contactPointId.m_storage = fcp.m_contactPointId;
        const __m128 v = fpp.m_vB.m_quad;
        const hkTransform* wTb = m_wTb;
        cdPoint->m_contact.m_position.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), wTb->m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), wTb->m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), wTb->m_rotation.m_col2.m_quad), wTb->m_translation.m_quad)
        );
        normal = _mm_mul_ps(_mm_set1_ps(negSign), (&m_wTa->m_rotation.m_col0)[fcp.m_featureIdA].m_quad);
    }
    else if (fpp.m_featureIndexA <= HK_BOXBOX_FACE_B_Z) {
        // A face of B: A's point to world space, minus B's face axis.
        const float negSign = -fpp.m_normalSign;
        cdPoint->m_contactPointId.m_storage = fcp.m_contactPointId;
        const __m128 v = fpp.m_vA.m_quad;
        const hkTransform* wTa = m_wTa;
        cdPoint->m_contact.m_position.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), wTa->m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), wTa->m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), wTa->m_rotation.m_col2.m_quad), wTa->m_translation.m_quad)
        );
        normal =
            _mm_mul_ps(_mm_set1_ps(negSign), (&m_wTb->m_rotation.m_col0)[fcp.m_featureIdA - HK_BOXBOX_FACE_B_X].m_quad);
    }
    else {
        // An edge pair: A's point to world space, fpp's normal rotated by A's transform.
        cdPoint->m_contactPointId.m_storage = fcp.m_contactPointId;
        const __m128 v = fpp.m_vA.m_quad;
        const hkTransform* wTa = m_wTa;
        cdPoint->m_contact.m_position.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), wTa->m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), wTa->m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), wTa->m_rotation.m_col2.m_quad), wTa->m_translation.m_quad)
        );
        const __m128 n = fpp.m_nA.m_quad;
        normal = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(n, n, 0x55), wTa->m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(n, n, 0x00), wTa->m_rotation.m_col0.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(n, n, 0xAA), wTa->m_rotation.m_col2.m_quad)
        );
    }
    cdPoint->m_contact.m_separatingNormal.m_quad = normal;
    XFNV_M128_F32(cdPoint->m_contact.m_separatingNormal.m_quad, 3) = fpp.m_distance;
    if (manifold.m_numPoints > 1) {
        // A normal against the previous contact's (dot product at most zero; unordered keeps the point) is dropped.
        const hkpProcessCdPoint* previous = m_result->m_firstFreeContactPoint.m_storage - 1;
        const __m128 prod =
            _mm_mul_ps(previous->m_contact.m_separatingNormal.m_quad, cdPoint->m_contact.m_separatingNormal.m_quad);
        const float dot = _mm_cvtss_f32(
            _mm_add_ss(_mm_shuffle_ps(prod, prod, 0xAA), _mm_add_ss(_mm_shuffle_ps(prod, prod, 0x55), prod))
        );
        if (dot <= 0.0f) {
            manifold.removePoint(index);
            return;
        }
    }
    const uint16_t contactPointId =
        m_contactMgr->addContactPoint(*m_bodyA, *m_bodyB, *m_env, *m_result, nullptr, cdPoint->m_contact);
    manifold.m_contactPoints[index].m_contactPointId = contactPointId;
    if (contactPointId == HK_INVALID_CONTACT_POINT) {
        manifold.removePoint(index);
        return;
    }
    m_result->m_firstFreeContactPoint.m_storage++;
    fcp.m_contactPointId = manifold.m_contactPoints[index].m_contactPointId;
    cdPoint->m_contactPointId.m_storage = manifold.m_contactPoints[index].m_contactPointId;
    if (fcp.m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
        manifold.m_faceVertexFeatureCount++;
    }
}

// The edge of A along axis edgeA and the edge of B along edgeB, each named by the signs of its vertex's other
// coordinates, are offered to addAdditionalEdgeHelper, then with the sign of A's axis nextEdgeA flipped, then also B's
// axis nextEdgeB, then B's alone.
void hkpBoxBoxCollisionDetection::tryToAddPointOnEdge(
    hkpBoxBoxManifold& manifold,
    int32_t edgeA,
    int32_t edgeB,
    int32_t nextVertA,
    int32_t nextVertB,
    const hkVector4& normalA,
    const hkVector4& normalB,
    float closestPointDist
) const {
    hkpFeatureContactPoint fcp;
    fcp.m_contactPointId = 0;
    hkVector4 signsA = normalA;
    hkVector4 signsB = normalB;
    XFNV_M128_F32(signsA.m_quad, edgeA) = 1.0f;
    XFNV_M128_F32(signsB.m_quad, edgeB) = 1.0f;
    const int32_t negativeA = _mm_movemask_ps(_mm_cmplt_ps(signsA.m_quad, _mm_setzero_ps()));
    const int32_t negativeB = _mm_movemask_ps(_mm_cmplt_ps(signsB.m_quad, _mm_setzero_ps()));
    fcp.m_featureIdA = static_cast<uint8_t>(((negativeA | 0xF8) << 4) | edgeA);
    fcp.m_featureIdB = static_cast<uint8_t>(((~negativeB & 7) << 4) + edgeB);
    addAdditionalEdgeHelper(manifold, fcp, closestPointDist);

    const int32_t axisA = nextVertA ? (nextVertA != 1) + 1 : 0;
    const int32_t flipA = 1 << (axisA + 4);
    const int32_t axisB = nextVertB ? (nextVertB != 1) + 1 : 0;
    fcp.m_featureIdA ^= static_cast<uint8_t>(flipA);
    addAdditionalEdgeHelper(manifold, fcp, closestPointDist);
    fcp.m_featureIdB ^= static_cast<uint8_t>(1 << (axisB + 4));
    addAdditionalEdgeHelper(manifold, fcp, closestPointDist);
    fcp.m_featureIdA ^= static_cast<uint8_t>(flipA);
    addAdditionalEdgeHelper(manifold, fcp, closestPointDist);
}

// Each manifold point is computed again from its features into m_result; one that is no longer valid leaves the
// manifold and the contact manager. From two points on, the manifold's normal is checked against the packed copy in B's
// space: a complete manifold whose normal turned by less than acos 0.95 is kept as it is. The closest point
// (findClosestPoint) sets the manifold's normal where needed (points whose normal turned too far leave) and joins the
// manifold unless a face point already covers it. For a face-face contact (both boxes' axes within acos 0.98 of the
// normal), the other vertices of each face's vertex join as further face points, then checkCompleteness decides whether
// the manifold is complete.
void hkpBoxBoxCollisionDetection::calcManifold(hkpBoxBoxManifold& manifold) const {
    initWorkVariables();
    const hkpProcessCdPoint* firstNewPoint = m_result->m_firstFreeContactPoint.m_storage;
    alignas(16) hkpFeaturePointCache fpp;

    // refreshManifold.
    for (int32_t i = 0; i < manifold.m_numPoints;) {
        const int32_t featureIdA = manifold.m_contactPoints[i].m_featureIdA;
        const uint32_t featureIdB = manifold.m_contactPoints[i].m_featureIdB;
        fpp.m_featureIndexA = featureIdA;
        bool valid;
        if (featureIdA <= HK_BOXBOX_FACE_A_Z) {
            // B's vertex (bits 4-6 of its id) in A's space, within A's keep radii; bit 3 is the normal's sign.
            const float normalSign = std::bit_cast<float>(std::bit_cast<uint32_t>(1.0f) | ((featureIdB & ~7u) << 28));
            const __m128 vB = _mm_mul_ps(vertexSignArray[(featureIdB & 0x70) >> 4], m_radiusB.m_quad);
            fpp.m_vB.m_quad = vB;
            fpp.m_vA.setTransformedPos(m_aTb, fpp.m_vB);
            fpp.m_normalSign = normalSign;
            hkVector4 absA;
            absA.setAbs4(fpp.m_vA);
            valid = (_mm_movemask_ps(_mm_cmplt_ps(absA.m_quad, m_keepRadiusA.m_quad)) & 7) == 7;
            if (valid) {
                fpp.m_distance = fpp.m_vA(featureIdA) * normalSign - m_radiusA(featureIdA);
            }
        }
        else if (featureIdA <= HK_BOXBOX_FACE_B_Z) {
            // A's vertex in B's space, within B's keep radii.
            const float normalSign = std::bit_cast<float>(std::bit_cast<uint32_t>(1.0f) | ((featureIdB & ~7u) << 28));
            hkVector4 vA;
            vA.m_quad = _mm_mul_ps(vertexSignArray[(featureIdB & 0x70) >> 4], m_radiusA.m_quad);
            fpp.m_vB.setTransformedInversePos(m_aTb, vA);
            fpp.m_normalSign = normalSign;
            hkVector4 absB;
            absB.setAbs4(fpp.m_vB);
            fpp.m_vA = vA;
            valid = (_mm_movemask_ps(_mm_cmplt_ps(absB.m_quad, m_keepRadiusB.m_quad)) & 7) == 7;
            if (valid) {
                const float negSign = -normalSign;
                fpp.m_distance =
                    fpp.m_vB(featureIdA - HK_BOXBOX_FACE_B_X) * negSign - m_radiusB(featureIdA - HK_BOXBOX_FACE_B_X);
            }
        }
        else {
            fpp.m_featureIndexB = static_cast<int32_t>(featureIdB);
            valid = isValidEdgeEdge(fpp).m_bool;
        }
        if (valid) {
            hkpProcessCdPoint* cdPoint = m_result->m_firstFreeContactPoint.m_storage;
            m_result->m_firstFreeContactPoint.m_storage++;
            contactPointFromFeaturePointCache(*cdPoint, manifold.m_contactPoints[i], fpp);
            i++;
        }
        else {
            if (manifold.m_contactPoints[i].m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
                manifold.m_faceVertexFeatureCount--;
            }
            m_contactMgr->removeContactPoint(
                manifold.m_contactPoints[i].m_contactPointId, *m_result->m_constraintOwner.m_storage
            );
            manifold.removePoint(i);
        }
    }

    // checkManifoldNormalConsistency: whether the manifold's normal is to be set again.
    bool updateManifoldNormal;
    if (manifold.m_numPoints < 2) {
        manifold.m_manifoldNormalInitialized.m_bool = 0;
        updateManifoldNormal = true;
    }
    else if (!manifold.m_manifoldNormalInitialized.m_bool) {
        alignas(16) hkVector4 normalInB;
        normalInB.setRotatedInverseDir(m_aTb.getRotation(), manifold.m_manifoldNormalA);
        manifold.m_manifoldNormalInitialized.m_bool = 1;
        manifold.m_manifoldNormalB = hkVector4Util::packQuaternionIntoInt32(normalInB);
        updateManifoldNormal = true;
    }
    else {
        hkVector4 normalInB;
        hkVector4Util::unpackInt32IntoQuaternion(manifold.m_manifoldNormalB, normalInB);
        hkVector4 normalInA;
        normalInA.setRotatedDir(m_aTb.getRotation(), normalInB);
        const __m128 prod = _mm_mul_ps(normalInA.m_quad, manifold.m_manifoldNormalA.m_quad);
        const float dot = _mm_cvtss_f32(
            _mm_add_ss(_mm_shuffle_ps(prod, prod, 0xAA), _mm_add_ss(_mm_shuffle_ps(prod, prod, 0x55), prod))
        );
        // The unordered (NaN) case is taken as unchanged.
        updateManifoldNormal = dot < 0.949999988079071;
        if (!updateManifoldNormal && manifold.m_isComplete.m_bool) {
            return;
        }
    }

    // Separation test, with the tolerance down to the smallest refreshed distance less 5% of the tolerance.
    hkResult intersection;
    if (manifold.m_numPoints != 0) {
        float minDist = firstNewPoint[0].m_contact.m_separatingNormal(3);
        for (int32_t k = manifold.m_numPoints - 1; k > 0; k--) {
            const float dist = firstNewPoint[k].m_contact.m_separatingNormal(3);
            // The test is written as !(dist > minDist) so that the unordered (NaN) case also updates minDist.
            if (!(dist > minDist)) {
                minDist = dist;
            }
        }
        const float closest = minDist - m_tolerance4(0) * 0.05000000074505806;
        alignas(16) hkVector4 tolerance;
        tolerance.m_quad = _mm_set1_ps(closest);
        intersection = checkIntersection(tolerance);
    }
    else {
        intersection = checkIntersection(m_tolerance4);
    }
    if (intersection != HK_SUCCESS) {
        return;
    }

    hkpFeatureContactPoint fcp;
    fcp.m_contactPointId = 0;
    fpp.m_nA.m_quad = _mm_setzero_ps();
    const int32_t found = findClosestPoint(manifold, fcp, fpp);
    if (updateManifoldNormal && found != 0) {
        // calcManifoldNormal: A's face axis, B's face axis in A's space, or the edge pair's normal.
        if (fpp.m_featureIndexA <= HK_BOXBOX_FACE_A_Z) {
            manifold.m_manifoldNormalA.m_quad = _mm_setzero_ps();
            manifold.m_manifoldNormalA(fcp.m_featureIdA) = -fpp.m_normalSign;
        }
        else if (fpp.m_featureIndexA <= HK_BOXBOX_FACE_B_Z) {
            const float negSign = -fpp.m_normalSign;
            manifold.m_manifoldNormalA.m_quad =
                _mm_mul_ps(_mm_set1_ps(negSign), m_aTb.getColumn(fcp.m_featureIdA - HK_BOXBOX_FACE_B_X).m_quad);
        }
        else if (found == 2) {
            manifold.m_manifoldNormalA = fpp.m_nA;
        }
        if (manifold.m_manifoldNormalInitialized.m_bool) {
            // The manifold's points (the last ones of m_result) whose normal turned by acos 0.95 or more leave; the
            // last point of m_result takes the place of a point that leaves.
            alignas(16) hkVector4 worldNormal;
            worldNormal.setRotatedDir(m_wTa->getRotation(), manifold.m_manifoldNormalA);
            const int32_t first =
                static_cast<int32_t>(m_result->m_firstFreeContactPoint.m_storage - m_result->m_contactPoints) -
                manifold.m_numPoints;
            for (int32_t j = 0; j < manifold.m_numPoints;) {
                hkpProcessCdPoint& point = m_result->m_contactPoints[first + j];
                const __m128 prod = _mm_mul_ps(point.m_contact.m_separatingNormal.m_quad, worldNormal.m_quad);
                const float dot = _mm_cvtss_f32(
                    _mm_add_ss(_mm_shuffle_ps(prod, prod, 0xAA), _mm_add_ss(_mm_shuffle_ps(prod, prod, 0x55), prod))
                );
                // The point is kept in the unordered case (dot is NaN).
                if (dot < 0.949999988079071) {
                    // Copies the members, not the padding after m_contactPointId.
                    const hkpProcessCdPoint* last = m_result->m_firstFreeContactPoint.m_storage - 1;
                    point.m_contact.m_position = last->m_contact.m_position;
                    point.m_contact.m_separatingNormal = last->m_contact.m_separatingNormal;
                    point.m_contactPointId.m_storage = last->m_contactPointId.m_storage;
                    if (manifold.m_contactPoints[j].m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
                        manifold.m_faceVertexFeatureCount--;
                    }
                    m_contactMgr->removeContactPoint(
                        manifold.m_contactPoints[j].m_contactPointId, *m_result->m_constraintOwner.m_storage
                    );
                    manifold.removePoint(j);
                    m_result->m_firstFreeContactPoint.m_storage--;
                }
                else {
                    j++;
                }
            }
            if (manifold.m_numPoints < 2) {
                manifold.m_manifoldNormalInitialized.m_bool = 0;
            }
            else {
                alignas(16) hkVector4 normalInB;
                normalInB.setRotatedInverseDir(m_aTb.getRotation(), manifold.m_manifoldNormalA);
                manifold.m_manifoldNormalB = hkVector4Util::packQuaternionIntoInt32(normalInB);
                manifold.m_manifoldNormalInitialized.m_bool = 1;
            }
        }
    }
    if (found != 2) {
        return;
    }

    // addPoint: a face point of the manifold within the tolerance of the closest point already covers it.
    if (fcp.m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
        hkVector4 position;
        if (fcp.m_featureIdA <= HK_BOXBOX_FACE_A_Z) {
            position.setTransformedPos(*m_wTb, fpp.m_vB);
        }
        else {
            position.setTransformedPos(*m_wTa, fpp.m_vA);
        }
        for (int32_t c = 0; c < manifold.m_numPoints; c++) {
            if (manifold.m_contactPoints[c].m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
                const __m128 diff =
                    _mm_sub_ps(position.m_quad, m_result->m_contactPoints[c].m_contact.m_position.m_quad);
                const __m128 sq = _mm_mul_ps(diff, diff);
                const float distSq = _mm_cvtss_f32(
                    _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
                );
                const float limit = m_tolerance * m_tolerance + 1.1920928955078125e-07;
                if (limit >= distSq) {
                    return;
                }
            }
        }
    }
    if (manifold.m_numPoints >= hkpBoxBoxManifold::HK_BOXBOX_MANIFOLD_MAX_POINTS) {
        return;
    }
    const int32_t closestIndex = manifold.addPoint(*m_bodyA, *m_bodyB, fcp);
    if (closestIndex >= 0) {
        hkpProcessCdPoint* cdPoint = m_result->m_firstFreeContactPoint.m_storage;
        contactPointFromFeaturePointCache(*cdPoint, fcp, fpp);
        if (manifold.m_numPoints > 1) {
            // A normal against the previous contact's (dot product at most zero; unordered keeps the point) ends it.
            const hkpProcessCdPoint* previous = m_result->m_firstFreeContactPoint.m_storage - 1;
            const __m128 prod =
                _mm_mul_ps(previous->m_contact.m_separatingNormal.m_quad, cdPoint->m_contact.m_separatingNormal.m_quad);
            const float dot = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(prod, prod, 0xAA), _mm_add_ss(_mm_shuffle_ps(prod, prod, 0x55), prod))
            );
            if (dot <= 0.0f) {
                manifold.removePoint(closestIndex);
                return;
            }
        }
        const uint16_t contactPointId =
            m_contactMgr->addContactPoint(*m_bodyA, *m_bodyB, *m_env, *m_result, nullptr, cdPoint->m_contact);
        manifold.m_contactPoints[closestIndex].m_contactPointId = contactPointId;
        if (contactPointId == HK_INVALID_CONTACT_POINT) {
            manifold.removePoint(closestIndex);
        }
        else {
            m_result->m_firstFreeContactPoint.m_storage++;
            fcp.m_contactPointId = manifold.m_contactPoints[closestIndex].m_contactPointId;
            cdPoint->m_contactPointId.m_storage = manifold.m_contactPoints[closestIndex].m_contactPointId;
            if (fcp.m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
                manifold.m_faceVertexFeatureCount++;
            }
        }
    }

    // findAdditionalManifoldPoints: only where both boxes have an axis within acos 0.98 of the first manifold
    // point's normal (the last points of m_result are the manifold's).
    hkpFeatureContactPoint fcpFaceA = fcp;
    if (manifold.m_numPoints < 1) {
        return;
    }
    const int32_t numPoints = manifold.m_numPoints;
    const int32_t first =
        static_cast<int32_t>(m_result->m_firstFreeContactPoint.m_storage - m_result->m_contactPoints) - numPoints;
    const hkVector4& worldNormal = m_result->m_contactPoints[first].m_contact.m_separatingNormal;
    // The normal pointing from A in A's space (negated), and in B's space.
    hkVector4 normalA;
    normalA.setRotatedInverseDir(m_wTa->getRotation(), worldNormal);
    normalA.m_quad = _mm_xor_ps(normalA.m_quad, hkQuadRealNegateMask);
    hkVector4 normalB;
    normalB.setRotatedInverseDir(m_wTb->getRotation(), worldNormal);
    hkVector4 absA;
    absA.setAbs4(normalA);
    hkVector4 absB;
    absB.setAbs4(normalB);
    // Each box's axis nearest the normal, and its plane mask (bits 4-6). Unordered comparisons are taken as not less.
    int32_t axisA;
    int32_t planeMaskA;
    if (absA(1) < absA(0)) {
        axisA = 0;
        planeMaskA = 0x10;
    }
    else {
        axisA = 1;
        planeMaskA = 0x20;
    }
    if (absA(axisA) < absA(2)) {
        axisA = 2;
        planeMaskA = 0x40;
    }
    int32_t axisB;
    int32_t planeMaskB;
    if (absB(1) < absB(0)) {
        axisB = 0;
        planeMaskB = 0x10;
    }
    else {
        axisB = 1;
        planeMaskB = 0x20;
    }
    if (absB(axisB) < absB(2)) {
        axisB = 2;
        planeMaskB = 0x40;
    }
    if (absA(axisA) < 0.9800000190734863) {
        return;
    }
    if (absB(axisB) < 0.9800000190734863) {
        return;
    }
    // The face of A along axisA with B's vertex from the normal's signs in B (bit 3: the side of A's centre), and the
    // face of B along axisB with A's vertex; the first keeps the closest point's contact point id.
    fcpFaceA.m_featureIdA = static_cast<uint8_t>(axisA);
    fcpFaceA.m_featureIdB = static_cast<uint8_t>(
        ((_mm_movemask_ps(_mm_cmplt_ps(normalB.m_quad, _mm_setzero_ps())) & 7) << 4) |
        ((std::bit_cast<uint32_t>(m_dinA(axisA)) >> 28) & 8)
    );
    hkpFeatureContactPoint fcpFaceB;
    fcpFaceB.m_featureIdA = static_cast<uint8_t>(axisB + HK_BOXBOX_FACE_B_X);
    fcpFaceB.m_featureIdB = static_cast<uint8_t>(
        ((_mm_movemask_ps(_mm_cmplt_ps(normalA.m_quad, _mm_setzero_ps())) & 7) << 4) |
        ((std::bit_cast<uint32_t>(m_dinB(axisB)) >> 28) & 8)
    );
    fcpFaceB.m_contactPointId = 0;
    // The smallest distance of the manifold's points, at most 1.
    float minDistance = 1.0f;
    for (int32_t k = 0; k < numPoints; k++) {
        const float dist = m_result->m_contactPoints[first + k].m_contact.m_separatingNormal(3);
        if (minDistance > dist) {
            minDistance = dist;
        }
    }

    // tryToAddPointFaceA, for each vertex flip of B's vertex that keeps it on B's face along axisB.
    for (int32_t t = 0; t < 7; t++) {
        const uint16_t flip = vertexFlipArray[t];
        if ((planeMaskB & flip) != 0) {
            continue;
        }
        hkpFeatureContactPoint fcpTry = fcpFaceA;
        fcpTry.m_featureIdB =
            static_cast<uint8_t>((fcpFaceA.m_featureIdB & 0xF) | (static_cast<uint8_t>(flip) ^ fcpFaceA.m_featureIdB));
        // EXE-INLINED 0xDB09B0: hkpBoxBoxManifold::findInManifold
        bool inManifold = false;
        for (int32_t k = manifold.m_numPoints - 1; k >= 0; k--) {
            if (manifold.m_contactPoints[k].m_featureIdA == fcpTry.m_featureIdA &&
                manifold.m_contactPoints[k].m_featureIdB == fcpTry.m_featureIdB) {
                inManifold = true;
                break;
            }
        }
        if (inManifold) {
            continue;
        }
        // isValidFaceAVertexB: B's vertex in A's space, within A's keep radii and no farther than the manifold's
        // smallest distance less the float epsilon.
        const uint32_t featureIdB = fcpTry.m_featureIdB;
        const int32_t featureIdA = fcpTry.m_featureIdA;
        const float normalSign = std::bit_cast<float>(std::bit_cast<uint32_t>(1.0f) | ((featureIdB & ~7u) << 28));
        const __m128 vB = _mm_mul_ps(vertexSignArray[(featureIdB & 0x70) >> 4], m_radiusB.m_quad);
        fpp.m_vB.m_quad = vB;
        fpp.m_vA.setTransformedPos(m_aTb, fpp.m_vB);
        hkVector4 absVA;
        absVA.setAbs4(fpp.m_vA);
        fpp.m_normalSign = normalSign;
        fpp.m_featureIndexA = featureIdA;
        if ((_mm_movemask_ps(_mm_cmplt_ps(absVA.m_quad, m_keepRadiusA.m_quad)) & 7) != 7) {
            continue;
        }
        fpp.m_distance = normalSign * fpp.m_vA(featureIdA) - m_radiusA(featureIdA);
        // The negated compare also skips the unordered (NaN) case.
        if (!(fpp.m_distance >= minDistance - 1.1920928955078125e-07)) {
            continue;
        }
        if (fcpTry.m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
            hkVector4 position;
            if (fcpTry.m_featureIdA <= HK_BOXBOX_FACE_A_Z) {
                position.setTransformedPos(*m_wTb, fpp.m_vB);
            }
            else {
                position.setTransformedPos(*m_wTa, fpp.m_vA);
            }
            bool covered = false;
            for (int32_t c = 0; c < manifold.m_numPoints; c++) {
                if (manifold.m_contactPoints[c].m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
                    const __m128 diff =
                        _mm_sub_ps(position.m_quad, m_result->m_contactPoints[c].m_contact.m_position.m_quad);
                    const __m128 sq = _mm_mul_ps(diff, diff);
                    const float distSq = _mm_cvtss_f32(
                        _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
                    );
                    const float limit = m_tolerance * m_tolerance + 1.1920928955078125e-07;
                    if (limit >= distSq) {
                        covered = true;
                        break;
                    }
                }
            }
            if (covered) {
                continue;
            }
        }
        if (manifold.m_numPoints >= hkpBoxBoxManifold::HK_BOXBOX_MANIFOLD_MAX_POINTS) {
            continue;
        }
        const int32_t index = manifold.addPoint(*m_bodyA, *m_bodyB, fcpTry);
        if (index < 0) {
            continue;
        }
        hkpProcessCdPoint* cdPoint = m_result->m_firstFreeContactPoint.m_storage;
        contactPointFromFeaturePointCache(*cdPoint, fcpTry, fpp);
        if (manifold.m_numPoints > 1) {
            const hkpProcessCdPoint* previous = m_result->m_firstFreeContactPoint.m_storage - 1;
            const __m128 prod =
                _mm_mul_ps(previous->m_contact.m_separatingNormal.m_quad, cdPoint->m_contact.m_separatingNormal.m_quad);
            const float dot = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(prod, prod, 0xAA), _mm_add_ss(_mm_shuffle_ps(prod, prod, 0x55), prod))
            );
            if (dot <= 0.0f) {
                manifold.removePoint(index);
                continue;
            }
        }
        const uint16_t contactPointId =
            m_contactMgr->addContactPoint(*m_bodyA, *m_bodyB, *m_env, *m_result, nullptr, cdPoint->m_contact);
        manifold.m_contactPoints[index].m_contactPointId = contactPointId;
        if (contactPointId == HK_INVALID_CONTACT_POINT) {
            manifold.removePoint(index);
            continue;
        }
        m_result->m_firstFreeContactPoint.m_storage++;
        cdPoint->m_contactPointId.m_storage = manifold.m_contactPoints[index].m_contactPointId;
        if (fcpTry.m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
            manifold.m_faceVertexFeatureCount++;
        }
    }

    // tryToAddPointFaceB, for each vertex flip of A's vertex that keeps it on A's face along axisA.
    for (int32_t t = 0; t < 7; t++) {
        const uint16_t flip = vertexFlipArray[t];
        if ((planeMaskA & flip) != 0) {
            continue;
        }
        hkpFeatureContactPoint fcpTry = fcpFaceB;
        fcpTry.m_featureIdB =
            static_cast<uint8_t>((fcpFaceB.m_featureIdB & 0xF) | (static_cast<uint8_t>(flip) ^ fcpFaceB.m_featureIdB));
        if (manifold.findInManifold(fcpTry).m_bool) {
            continue;
        }
        // isValidFaceBVertexA: A's vertex in B's space, within B's keep radii and the distance limit above.
        const uint32_t featureIdB = fcpTry.m_featureIdB;
        const int32_t featureIdA = fcpTry.m_featureIdA;
        const float normalSign = std::bit_cast<float>(std::bit_cast<uint32_t>(1.0f) | ((featureIdB & ~7u) << 28));
        hkVector4 vA;
        vA.m_quad = _mm_mul_ps(vertexSignArray[(featureIdB & 0x70) >> 4], m_radiusA.m_quad);
        hkVector4 vB;
        vB.setTransformedInversePos(m_aTb, vA);
        hkVector4 absVB;
        absVB.m_quad = vB.m_quad;
        absVB.setAbs4(absVB);
        fpp.m_normalSign = normalSign;
        fpp.m_featureIndexA = featureIdA;
        fpp.m_vA = vA;
        fpp.m_vB = vB;
        if ((_mm_movemask_ps(_mm_cmplt_ps(absVB.m_quad, m_keepRadiusB.m_quad)) & 7) != 7) {
            continue;
        }
        const float negSign = -normalSign;
        fpp.m_distance =
            fpp.m_vB(featureIdA - HK_BOXBOX_FACE_B_X) * negSign - m_radiusB(featureIdA - HK_BOXBOX_FACE_B_X);
        // The negated compare also skips the unordered (NaN) case.
        if (!(fpp.m_distance >= minDistance - 1.1920928955078125e-07)) {
            continue;
        }
        if (fcpTry.m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
            hkVector4 position;
            if (fcpTry.m_featureIdA <= HK_BOXBOX_FACE_A_Z) {
                position.setTransformedPos(*m_wTb, vB);
            }
            else {
                position.setTransformedPos(*m_wTa, vA);
            }
            bool covered = false;
            for (int32_t c = 0; c < manifold.m_numPoints; c++) {
                if (manifold.m_contactPoints[c].m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
                    const __m128 diff =
                        _mm_sub_ps(position.m_quad, m_result->m_contactPoints[c].m_contact.m_position.m_quad);
                    const __m128 sq = _mm_mul_ps(diff, diff);
                    const float distSq = _mm_cvtss_f32(
                        _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
                    );
                    const float limit = m_tolerance * m_tolerance + 1.1920928955078125e-07;
                    if (limit >= distSq) {
                        covered = true;
                        break;
                    }
                }
            }
            if (covered) {
                continue;
            }
        }
        if (manifold.m_numPoints >= hkpBoxBoxManifold::HK_BOXBOX_MANIFOLD_MAX_POINTS) {
            continue;
        }
        const int32_t index = manifold.addPoint(*m_bodyA, *m_bodyB, fcpTry);
        if (index < 0) {
            continue;
        }
        hkpProcessCdPoint* cdPoint = m_result->m_firstFreeContactPoint.m_storage;
        contactPointFromFeaturePointCache(*cdPoint, fcpTry, fpp);
        if (manifold.m_numPoints > 1) {
            const hkpProcessCdPoint* previous = m_result->m_firstFreeContactPoint.m_storage - 1;
            const __m128 prod =
                _mm_mul_ps(previous->m_contact.m_separatingNormal.m_quad, cdPoint->m_contact.m_separatingNormal.m_quad);
            const float dot = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(prod, prod, 0xAA), _mm_add_ss(_mm_shuffle_ps(prod, prod, 0x55), prod))
            );
            if (dot <= 0.0f) {
                manifold.removePoint(index);
                continue;
            }
        }
        const uint16_t contactPointId =
            m_contactMgr->addContactPoint(*m_bodyA, *m_bodyB, *m_env, *m_result, nullptr, cdPoint->m_contact);
        manifold.m_contactPoints[index].m_contactPointId = contactPointId;
        if (contactPointId == HK_INVALID_CONTACT_POINT) {
            manifold.removePoint(index);
            continue;
        }
        m_result->m_firstFreeContactPoint.m_storage++;
        cdPoint->m_contactPointId.m_storage = manifold.m_contactPoints[index].m_contactPointId;
        if (fcpTry.m_featureIdA <= HK_BOXBOX_FACE_B_Z) {
            manifold.m_faceVertexFeatureCount++;
        }
    }

    if (manifold.m_faceVertexFeatureCount >= 4) {
        manifold.m_isComplete.m_bool = 1;
        return;
    }
    checkCompleteness(manifold, planeMaskA, planeMaskB);
}

// With at least three points, four face points make the manifold complete; otherwise each point's features add to 2-bit
// counters of the planes it touches (v5 for A's, v6 for B's; a counter stops at 2), the axis planeMaskA or planeMaskB
// names (bits 4-6) left out, and the manifold is complete when no counter holds 1.
void hkpBoxBoxCollisionDetection::checkCompleteness(
    hkpBoxBoxManifold& manifold,
    int32_t planeMaskA,
    int32_t planeMaskB
) const {
    if (manifold.m_numPoints >= 3u) {
        bool v26;
        bool v27;
        char v28;
        int32_t v29;
        bool v30;
        int32_t v36;
        int32_t a3a;
        if (manifold.m_faceVertexFeatureCount >= 4u) {
            manifold.m_isComplete.m_bool = 1;
            return;
        }
        if ((planeMaskA >> 4 & 1) != 0) {
            v36 = 0;
        }
        else {
            v36 = 2 - ((planeMaskA >> 4 & 2) != 0);
        }
        if ((planeMaskB >> 4 & 1) != 0) {
            a3a = 0;
        }
        else {
            a3a = 2 - ((planeMaskB >> 4 & 2) != 0);
        }
        int32_t v4 = 0;
        int32_t v5 = 0;
        int32_t v6 = 0;
        int32_t a2a = 0;
        while (true) {
            uint8_t v7;
            int32_t v14;
            bool v15;
            int32_t v23;
            int32_t v24;
            bool v25;
            int32_t v31;
            bool v32;
            bool v33;
            bool v34;
            char v35;
            v7 = manifold.m_contactPoints[v4].m_featureIdA;
            if (v7 <= 2u) {
                int32_t v8 = manifold.m_contactPoints[v4].m_featureIdB >> 4;
                bool v9 = (manifold.m_contactPoints[v4].m_featureIdB & 16) == 0;
                bool v10 = (manifold.m_contactPoints[v4].m_featureIdB & 32) == 0;
                bool v11 = (v8 & 4) == 0;
                if (a3a) {
                    int32_t v12 = 1 << 2 * (v10 + 2 * v11);
                    if ((2 * v12 & v6) == 0) {
                        v6 += v12;
                    }
                    v4 = a2a;
                }
                if (a3a != 1) {
                    int32_t v13 = 1 << 2 * (v9 + 2 * v11 + 4);
                    if ((2 * v13 & v6) == 0) {
                        v6 += v13;
                    }
                }
                if (a3a == 2) {
                    goto LABEL_48;
                }
                v14 = 1 << 2 * (v9 + 2 * v10 + 8);
                v15 = (2 * v14 & v6) == 0;
                goto LABEL_46;
            }
            if (v7 <= 6u) {
                int32_t v16 = manifold.m_contactPoints[v4].m_featureIdB >> 4;
                bool v17 = (manifold.m_contactPoints[v4].m_featureIdB & 16) == 0;
                bool v18 = (manifold.m_contactPoints[v4].m_featureIdB & 32) == 0;
                bool v19 = (v16 & 4) == 0;
                if (v36) {
                    int32_t v20 = 1 << 2 * (v18 + 2 * v19);
                    if ((2 * v20 & v5) == 0) {
                        v5 += v20;
                    }
                    v4 = a2a;
                }
                if (v36 != 1) {
                    int32_t v21 = 1 << 2 * (v17 + 2 * v19 + 4);
                    if ((2 * v21 & v5) == 0) {
                        v5 += v21;
                    }
                }
                if (v36 != 2) {
                    int32_t v22 = 1 << 2 * (v17 + 2 * v18 + 8);
                    if ((2 * v22 & v5) == 0) {
                        v5 += v22;
                    }
                }
                goto LABEL_48;
            }
            v23 = static_cast<int32_t>(v7) >> 4;
            v24 = manifold.m_contactPoints[v4].m_featureIdA & 15;
            v25 = (v23 & 1) == 0;
            v26 = (v23 & 2) == 0;
            v27 = (v23 & 4) == 0;
            if (!v24) {
                break;
            }
            if (v24 != 1) {
                v28 = v25 + 2 * v26 + 8;
                goto LABEL_37;
            }
            v29 = 1 << 2 * (v25 + 2 * v27 + 4);
            v30 = (2 * v29 & v5) == 0;
        LABEL_38:;
            if (v30) {
                v5 += v29;
            }
            v31 = static_cast<int32_t>(manifold.m_contactPoints[v4].m_featureIdB) >> 4;
            v32 = (v31 & 1) == 0;
            v33 = (v31 & 2) == 0;
            v34 = (v31 & 4) == 0;
            if ((manifold.m_contactPoints[v4].m_featureIdB & 15) != 0) {
                if ((manifold.m_contactPoints[v4].m_featureIdB & 15) == 1) {
                    v14 = 1 << 2 * (v32 + 2 * v34 + 4);
                    v15 = (2 * v14 & v6) == 0;
                    goto LABEL_46;
                }
                v35 = v32 + 2 * v33 + 8;
            }
            else {
                v35 = v33 + 2 * v34;
            }
            v14 = 1 << 2 * v35;
            v15 = (2 * v14 & v6) == 0;
        LABEL_46:;
            if (v15) {
                v6 += v14;
            }
        LABEL_48:;
            a2a = ++v4;
            if (v4 >= manifold.m_numPoints) {
                // The low bit of every 2-bit counter clear.
                manifold.m_isComplete.m_bool = (v5 & 0x555555) == 0 && (v6 & 0x555555) == 0;
                return;
            }
        }
        v28 = v26 + 2 * v27;
    LABEL_37:;
        v29 = 1 << 2 * v28;
        v30 = (2 * v29 & v5) == 0;
        goto LABEL_38;
    }
}

// cp gets fcp's id, fpp's point and normal in world space, and fpp's distance as the normal's w. A face of A: B's
// vertex, minus the normal sign times A's face axis; a face of B: A's vertex and B's face axis; an edge pair: A's point
// and fpp's normal.
inline void hkpBoxBoxCollisionDetection::contactPointFromFeaturePointCache(
    hkpProcessCdPoint& ccpOut,
    const hkpFeatureContactPoint& fcp,
    const hkpFeaturePointCache& fpp
) const {
    __m128 normal;
    if (fpp.m_featureIndexA <= HK_BOXBOX_FACE_A_Z) {
        const float negSign = -fpp.m_normalSign;
        ccpOut.m_contactPointId.m_storage = fcp.m_contactPointId;
        ccpOut.m_contact.m_position.setTransformedPos(*m_wTb, fpp.m_vB);
        normal = _mm_mul_ps(_mm_set1_ps(negSign), m_wTa->getColumn(fcp.m_featureIdA).m_quad);
    }
    else if (fpp.m_featureIndexA <= HK_BOXBOX_FACE_B_Z) {
        const float negSign = -fpp.m_normalSign;
        ccpOut.m_contactPointId.m_storage = fcp.m_contactPointId;
        ccpOut.m_contact.m_position.setTransformedPos(*m_wTa, fpp.m_vA);
        normal = _mm_mul_ps(_mm_set1_ps(negSign), m_wTb->getColumn(fcp.m_featureIdA - HK_BOXBOX_FACE_B_X).m_quad);
    }
    else {
        ccpOut.m_contactPointId.m_storage = fcp.m_contactPointId;
        ccpOut.m_contact.m_position.setTransformedPos(*m_wTa, fpp.m_vA);
        hkVector4 normalWs;
        normalWs.setRotatedDir(m_wTa->getRotation(), fpp.m_nA);
        normal = normalWs.m_quad;
    }
    ccpOut.m_contact.m_separatingNormal.m_quad = normal;
    ccpOut.m_contact.m_separatingNormal(3) = fpp.m_distance;
}

void hkpBoxBoxCollisionDetection::initWorkVariables() const {
    hkVector4 v1;
    hkVector4 v2;
    m_dinA = m_aTb.m_translation;
    v1.m_quad = m_aTb.m_rotation.m_col1.m_quad;
    v2.m_quad = m_aTb.m_rotation.m_col2.m_quad;
    __m128 v3 = _mm_unpacklo_ps(m_aTb.m_rotation.m_col0.m_quad, v1.m_quad);
    __m128 v4 = _mm_movelh_ps(v3, v2.m_quad);
    m_dinB.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(m_dinA.m_quad, m_dinA.m_quad, 85), _mm_shuffle_ps(_mm_movehl_ps(v4, v3), v2.m_quad, 212)
            ),
            _mm_mul_ps(_mm_shuffle_ps(m_dinA.m_quad, m_dinA.m_quad, 0), v4)
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(m_dinA.m_quad, m_dinA.m_quad, 170),
            _mm_shuffle_ps(_mm_unpackhi_ps(m_aTb.m_rotation.m_col0.m_quad, v1.m_quad), v2.m_quad, 228)
        )
    );
}
