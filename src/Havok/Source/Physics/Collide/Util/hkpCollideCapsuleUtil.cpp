#include "Havok/Source/Physics/Collide/Util/hkpCollideCapsuleUtil.h"

#include "Havok/Source/Common/Base/Algorithm/Collide/LineSegment/hkLineSegmentUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideTriangleUtil.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The squared distance test compares against the product of fMaxDistance with itself, which is exact for a float
// product, hence the double. The normalize3 Newton step uses the constants 0.0, 3.0 and 0.5. The three-lane sums and
// the Newton step are written packed: only lane 0 is read.
hkResult hkCollideCapsuleUtilClostestPointCapsVsCaps(
    const hkVector4* capsA,
    float capsARadius,
    const hkVector4* capsB,
    float capsBRadius,
    float collisionTolerance,
    hkContactPoint& cpoint
) {
    hkVector4 dA;
    _mm_store_ps(
        &reinterpret_cast<float*>(&dA.m_quad)[0],
        _mm_sub_ps(
            _mm_load_ps(&reinterpret_cast<const float*>(&capsA[1].m_quad)[0]),
            _mm_load_ps(&reinterpret_cast<const float*>(&capsA[0].m_quad)[0])
        )
    );
    hkVector4 dB;
    _mm_store_ps(
        &reinterpret_cast<float*>(&dB.m_quad)[0],
        _mm_sub_ps(
            _mm_load_ps(&reinterpret_cast<const float*>(&capsB[1].m_quad)[0]),
            _mm_load_ps(&reinterpret_cast<const float*>(&capsB[0].m_quad)[0])
        )
    );
    hkLineSegmentUtil::ClosestLineSegLineSegResult result;
    const int32_t iParallel = hkLineSegmentUtil::closestLineSegLineSeg(capsA[0], dA, capsB[0], dB, result);

    const float fRadiusSum = capsARadius + capsBRadius;
    const float fMaxDistance = fRadiusSum + collisionTolerance;
    const float fDistanceSquared = result.m_distanceSquared;
    if (fDistanceSquared > static_cast<double>(fMaxDistance) * fMaxDistance) {
        return static_cast<hkResult>(1);
    }
    const float fDistance = std::sqrt(fDistanceSquared);

    float* pNormal = &reinterpret_cast<float*>(&cpoint.m_separatingNormal.m_quad)[0];
    bool bHaveNormal = false;
    if (iParallel) {
        if (0.0f < fDistanceSquared) {
            _mm_store_ps(pNormal, _mm_load_ps(&reinterpret_cast<float*>(&result.m_closestAminusClosestB.m_quad)[0]));
            bHaveNormal = true;
        }
    }
    else {
        // The normal of both directions, turned towards A.
        const __m128 kA = _mm_load_ps(&reinterpret_cast<float*>(&dA.m_quad)[0]);
        const __m128 kB = _mm_load_ps(&reinterpret_cast<float*>(&dB.m_quad)[0]);
        const __m128 kProduct = _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xC9), _mm_shuffle_ps(kA, kA, 0xD2));
        hkVector4 kCross;
        kCross.m_quad = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xD2), _mm_shuffle_ps(kA, kA, 0xC9)), kProduct);
        const __m128 kSquares = _mm_mul_ps(kCross.m_quad, kCross.m_quad);
        const float fLengthSquared = _mm_cvtss_f32(_mm_add_ps(
            _mm_shuffle_ps(kSquares, kSquares, 0xAA), _mm_add_ps(_mm_shuffle_ps(kSquares, kSquares, 0x55), kSquares)
        ));
        if (1.1920929e-07f < fLengthSquared) {
            const __m128 kDot = _mm_mul_ps(
                kCross.m_quad, _mm_load_ps(&reinterpret_cast<float*>(&result.m_closestAminusClosestB.m_quad)[0])
            );
            const float fDot = _mm_cvtss_f32(
                _mm_add_ps(_mm_shuffle_ps(kDot, kDot, 0xAA), _mm_add_ps(_mm_shuffle_ps(kDot, kDot, 0x55), kDot))
            );
            if (0.0f > fDot) {
                kCross.setNeg4(kCross);
            }
            _mm_store_ps(pNormal, kCross.m_quad);
            bHaveNormal = true;
        }
    }
    if (!bHaveNormal) {
        // Any direction perpendicular to dA: its smallest component zeroed, the other two swapped, one negated.
        const float* pA = &reinterpret_cast<float*>(&dA.m_quad)[0];
        float fMin = __builtin_fabsf(pA[0]);
        const float fAbsY = __builtin_fabsf(pA[1]);
        const float fAbsZ = __builtin_fabsf(pA[2]);
        int32_t iMin = 0;
        int32_t iFirst = 1;
        int32_t iSecond = 2;
        if (fMin > fAbsY) {
            fMin = fAbsY;
            iFirst = 0;
            iMin = 1;
        }
        if (fMin > fAbsZ) {
            iSecond = iMin;
        }
        const float fSecond = pA[iSecond];
        _mm_store_ps(pNormal, _mm_setzero_ps());
        pNormal[iFirst] = fSecond;
        pNormal[iSecond] = -pA[iFirst];
    }

    const float fRadiusBMinusDistance = capsBRadius - fDistance;
    cpoint.m_separatingNormal.normalize3();
    _mm_store_ps(
        &reinterpret_cast<float*>(&cpoint.m_position.m_quad)[0],
        _mm_add_ps(
            _mm_mul_ps(_mm_set1_ps(fRadiusBMinusDistance), _mm_load_ps(pNormal)),
            _mm_load_ps(&reinterpret_cast<float*>(&result.m_closestPointA.m_quad)[0])
        )
    );
    cpoint.m_separatingNormal.m_quad[3] = fDistance - fRadiusSum;
    return static_cast<hkResult>(0);
}

int32_t hkCollideCapsuleUtilManifoldCapsVsCaps(
    const hkVector4* capsAPointsIn,
    float capsARadius,
    const hkVector4* capsBPoints,
    float capsBRadius,
    hkContactPoint* points
) {
    __m128 m_quad_3;
    double v26;
    double v27;
    double v30;
    alignas(16) uint32_t v99;
    alignas(16) uint32_t v101;
    alignas(16) uint32_t v102;
    float v104;
    float v105;
    float v109;
    int32_t n2_1;
    alignas(16) uint32_t v117;
    __m128 m_quad_4;
    const hkVector4* capsB_1 = capsBPoints;
    __m128 m_quad = capsAPointsIn[1].m_quad;
    __m128 m_quad_1 = capsAPointsIn->m_quad;
    __m128 m_quad_2 = capsBPoints->m_quad;
    __m128 v9 = _mm_sub_ps(capsBPoints[1].m_quad, capsBPoints->m_quad);
    __m128 v10 = _mm_sub_ps(m_quad, capsAPointsIn->m_quad);
    __m128 v11 = _mm_mul_ps(v9, v10);
    float v124 = _mm_shuffle_ps(v11, v11, 170)[0] + (_mm_shuffle_ps(v11, v11, 85)[0] + v11[0]);
    float v123 = v124;
    __m128 v129 = v10;
    __m128 v133 = v9;
    if (!(v124 < 0.0f)) {
        m_quad_3 = m_quad_1;
        m_quad_4 = m_quad;
    }
    else {
        v10 = _mm_xor_ps(v10, ::hkQuadRealNegateMask);
        v123 = -v124;
        v129 = v10;
        m_quad_3 = m_quad;
        m_quad_4 = m_quad_1;
    }
    __m128 v13 = _mm_sub_ps(m_quad_2, m_quad_3);
    __m128 v14 = _mm_mul_ps(v13, v10);
    __m128 v15 = _mm_mul_ps(v13, v9);
    float v16 = _mm_shuffle_ps(v14, v14, 170)[0] + (_mm_shuffle_ps(v14, v14, 85)[0] + v14[0]);
    float v120 = _mm_shuffle_ps(v15, v15, 170)[0] + (_mm_shuffle_ps(v15, v15, 85)[0] + v15[0]);
    __m128 v17 = _mm_mul_ps(v10, v10);
    float v126 = _mm_shuffle_ps(v17, v17, 170)[0] + (_mm_shuffle_ps(v17, v17, 85)[0] + v17[0]);
    __m128 v18 = _mm_mul_ps(v9, v9);
    float v125 = _mm_shuffle_ps(v18, v18, 170)[0] + (_mm_shuffle_ps(v18, v18, 85)[0] + v18[0]);
    double v19 = v125;
    float v122 = v16;
    double v20 = v125 * v126;
    int32_t n2 = 2;
    double v22 = v123 * v123;
    float v90 = v20 - v22;
    float v91 = std::fabs((double)v90);
    float v103 = v125 * v16 - v123 * v120;
    double v24 = v91;
    double v25 = v103;
    if (!(v103 * v91 >= v91 * v91)) {
        if (!(v25 <= 0.0)) {
            double v28;
            float v92 = v20;
            float v93 = std::fabs((double)v92);
            float v94 = (v22 + v93) * 9.5367431640625e-07;
            if (!(v94 < v24)) {
                n2_1 = 1;
                v28 = 1.0;
            }
            else {
                v28 = v25 / v24;
                n2_1 = 0;
            }
            v104 = v28;
            v26 = v24;
            v27 = 0.0f;
        }
        else {
            n2_1 = 2;
            v27 = 0.0f;
            v26 = v91;
            v104 = 0.0f;
        }
    }
    else {
        n2_1 = 1;
        v26 = v91;
        v104 = 1.0f;
        v27 = 0.0;
    }
    float v108 = v104 * v123 - v120;
    double v29 = v108;
    if (!(v108 >= v19)) {
        if (!(v29 <= v27)) {
            v109 = v29 / v19;
            goto LABEL_22;
        }
        n2_1 = 8;
        v109 = v27;
    }
    else {
        n2_1 = 4;
        v109 = 1.0f;
    }
    v105 = v123 * v109 + v16;
    v30 = v105;
    if (!(v105 <= v27)) {
        if (!(v126 <= v30)) {
            v104 = v30 / v126;
        }
        else {
            n2_1 |= 1u;
            v104 = 1.0f;
        }
    }
    else {
        n2_1 |= 2u;
        v104 = v27;
    }
LABEL_22:;
    float v127 = capsARadius + capsBRadius;
    __m128 v31 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v104), _mm_set_ss(v104), 0), v10), m_quad_3);
    double v32 = points->m_separatingNormal.m_quad[3];
    __m128 m_quad_6 =
        _mm_sub_ps(v31, _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v109), _mm_set_ss(v109), 0), v9), m_quad_2));
    if (v32 < 1.1920928955078125e-07) {
        v32 = 1.1920929e-07f;
    }
    float v110 = v32;
    __m128 v34 = _mm_mul_ps(m_quad_6, m_quad_6);
    float v35 = _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_set_ss((_mm_shuffle_ps(v34, v34, 170)[0] + (_mm_shuffle_ps(v34, v34, 85)[0] + v34[0]))))
    );
    double v36 = v35;
    float v106 = v35;
    points->m_separatingNormal.m_quad = m_quad_6;
    float v95 = 1.0 / v35;
    float v118 = v35 - v127;
    if (v110 > static_cast<double>(v118)) {
        hkVector4 v52;
        double v66;
        double v67;
        double v68;
        double v69;
        double v70;
        __m128 v73;
        __m128 m_quad_10;
        __m128 m_quad_12;
        __m128 m_quad_13;
        __m128 m_quad_9;
        bool v37 = v36 > 1.1920929e-07f;
        bool v38 = 1.1920929e-07f == v36;
        double v39 = v118;
        if (v37 || v38) {
            points->m_separatingNormal.m_quad =
                _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v95), _mm_set_ss(v95), 0), points->m_separatingNormal.m_quad);
        }
        else {
            hkVector4 v43;
            __m128 v48 = _mm_undefined_ps();
            int32_t n2_2 = 0;
            v106 = 0.0f;
            int32_t v41 = 1;
            float v96 = std::fabs((double)v129[0]);
            float v111 = v96;
            float v97 = std::fabs((double)v129[1]);
            float v98 = std::fabs((double)v129[2]);
            if (v111 > static_cast<double>(v97)) {
                v41 = 0;
                v111 = v97;
                n2_2 = 1;
            }
            if (v111 > static_cast<double>(v98)) {
                n2 = n2_2;
            }
            double v42 = v129[n2];
            points->m_separatingNormal.m_quad = _mm_setzero_ps();
            points->m_separatingNormal.m_quad[v41] = v42;
            points->m_separatingNormal.m_quad[n2] = -v129[v41];
            v43.m_quad = points->m_separatingNormal.m_quad;
            __m128 v44 = _mm_mul_ps(v43.m_quad, v43.m_quad);
            float v45 = _mm_shuffle_ps(v44, v44, 85)[0] + v44[0];
            __m128 v46 = _mm_shuffle_ps(v44, v44, 170);
            v46[0] = v46[0] + v45;
            float v47 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v46[0])));
            v48[0] = zero;
            __m128 v49 = _mm_cmpeq_ss(v46, v48);
            v48[0] = 3.0f - ((v46[0] * v47) * v47);
            __m128 v50 = _mm_setzero_ps();
            v50[0] = (0.5f * v47) * v48[0];
            __m128 v51 = _mm_andnot_ps(v49, v50);
            points->m_separatingNormal.m_quad = _mm_mul_ps(_mm_shuffle_ps(v51, v51, 0), v43.m_quad);
        }
        *reinterpret_cast<float*>(&v99) = capsBRadius - v106;
        v52.m_quad = points->m_separatingNormal.m_quad;
        points->m_separatingNormal.m_quad[3] = v39;
        points->m_position.m_quad = _mm_add_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v99)), _mm_set_ss(std::bit_cast<float>(v99)), 0),
                v52.m_quad
            ),
            v31
        );
        if (0.20000000298023224 * v126 * v19 < v26) {
            return n2_1;
        }
        __m128 hkQuadRealNegateMask = ::hkQuadRealNegateMask;
        __m128 m_quad_7 = m_quad_3;
        __m128 m_quad_8 = m_quad_3;
        m_quad_9 = capsBPoints->m_quad;
        int32_t v57 = 0;
        __m128 m_quad_14 = m_quad_9;
        hkContactPoint* v59 = points + 1;
        int32_t n10 = 10;
        double v61 = 1.0 / v20;
        double v62 = v126;
        float v100 = v61;
        float v107 = v19 * v100;
        float v112 = v100 * v126;
        float v121 = -v120;
        double v63 = v107;
        double v64 = 0.0;
        while (true) {
            double v65;
            float v74;
            double v85;
            float v114;
            float v115;
            float v116;
            v65 = v122;
            v66 = v121;
            if ((n10 & n2_1) != 0) {
                goto LABEL_54;
            }
            if (v66 > v64) {
                break;
            }
            v67 = v122;
            v68 = v121;
            if (v122 > v64) {
                if (v67 > v62) {
                    goto LABEL_57;
                }
                v69 = v122;
            LABEL_47:;
                double v71 = v68;
                double v72 = v63 * v69;
                v70 = v71;
                *reinterpret_cast<float*>(&v102) = v72;
                m_quad_8 = _mm_add_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(
                            _mm_set_ss(std::bit_cast<float>(v102)), _mm_set_ss(std::bit_cast<float>(v102)), 0
                        ),
                        v10
                    ),
                    m_quad_8
                );
                m_quad_7 = m_quad_8;
                goto LABEL_49;
            }
            v69 = v122;
            v70 = v121;
        LABEL_49:;
            m_quad_9 = _mm_sub_ps(m_quad_8, m_quad_14);
            v73 = _mm_mul_ps(m_quad_9, m_quad_9);
            v114 = v59->m_separatingNormal.m_quad[3] + v127;
            v74 = _mm_shuffle_ps(v73, v73, 170)[0] + (_mm_shuffle_ps(v73, v73, 85)[0] + v73[0]);
            v115 = v114 * v114;
            if (!(v115 > static_cast<double>(v74))) {
                double v88;
                v88 = v70;
                v63 = v107;
                v68 = v88;
                v67 = v69;
                v64 = 0.0;
            LABEL_57:;
                double v89 = v67;
                v66 = v68;
                v65 = v89;
                goto LABEL_54;
            }
            v116 = std::sqrt((double)v74);
            v59->m_separatingNormal.m_quad = m_quad_9;
            if (!(v74 > 1.1920929e-07f)) {
                m_quad_10 = points->m_separatingNormal.m_quad;
            }
            else {
                __m128 v80;
                __m128 m_quad_11 = v59->m_separatingNormal.m_quad;
                __m128 v76 = _mm_mul_ps(m_quad_11, m_quad_11);
                float v77 = _mm_shuffle_ps(v76, v76, 85)[0] + v76[0];
                __m128 v78 = _mm_shuffle_ps(v76, v76, 170);
                v78[0] = v78[0] + v77;
                float v79 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v78[0])));
                v80[0] = zero;
                __m128 v81 = _mm_cmpeq_ss(v78, v80);
                v80[0] = 3.0f - ((v78[0] * v79) * v79);
                __m128 v82 = _mm_setzero_ps();
                v82[0] = (0.5f * v79) * v80[0];
                __m128 v83 = _mm_andnot_ps(v81, v82);
                m_quad_10 = _mm_mul_ps(_mm_shuffle_ps(v83, v83, 0), m_quad_11);
            }
            v9 = v133;
            capsB_1 = capsBPoints;
            v85 = v116;
            v59->m_separatingNormal.m_quad = m_quad_10;
            *reinterpret_cast<float*>(&v117) = capsBRadius - v116;
            m_quad_12 = v59->m_separatingNormal.m_quad;
            v59->m_separatingNormal.m_quad[3] = v85 - v127;
            v19 = v125;
            m_quad_13 = _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v117)), _mm_set_ss(std::bit_cast<float>(v117)), 0),
                    m_quad_12
                ),
                m_quad_7
            );
            v62 = v126;
            v10 = v129;
            v59->m_position.m_quad = m_quad_13;
            hkQuadRealNegateMask = ::hkQuadRealNegateMask;
            v63 = v107;
            v65 = v122;
            v66 = v121;
            v64 = 0.0;
        LABEL_54:;
            if (v57 == 1) {
                return n2_1;
            }
            m_quad_8 = m_quad_4;
            m_quad_14 = capsB_1[1].m_quad;
            v10 = _mm_xor_ps(v10, hkQuadRealNegateMask);
            v9 = _mm_xor_ps(v9, hkQuadRealNegateMask);
            ++v59;
            m_quad_7 = m_quad_4;
            v121 = v19 - v66 - v123;
            v129 = v10;
            v133 = v9;
            ++v57;
            n10 = 5;
            v122 = v62 - v65 - v123;
        }
        if (v66 > v19) {
            goto LABEL_54;
        }
        v67 = v122;
        v68 = v121;
        if (!(v122 > v64)) {
            v69 = v122;
        LABEL_43:;
            v70 = v121;
            *reinterpret_cast<float*>(&v101) = v68 * v112;
            m_quad_14 = _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v101)), _mm_set_ss(std::bit_cast<float>(v101)), 0),
                    v9
                ),
                m_quad_14
            );
            goto LABEL_49;
        }
        if (v67 > v62) {
            goto LABEL_57;
        }
        v69 = v122;
        if (!(v69 * v69 * v63 < v68 * v68 * v112)) {
            goto LABEL_43;
        }
        goto LABEL_47;
    }
    points->m_separatingNormal.m_quad =
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v95), _mm_set_ss(v95), 0), points->m_separatingNormal.m_quad);
    points->m_separatingNormal.m_quad[3] = v118;
    return n2_1;
}

// The vector code is written as SSE lane operations with intrinsics; the scalar steps use x87 precision, as in
// hkCollideCapsuleUtilClostestPointCapsVsCaps above. The plane crossing parameter and the squared lengths and distances
// of the crossing segment's edge test are deliberately computed in double, as the game does. The reciprocal square root
// normalizations are hkVector4::normalize3. The 3.40282e+38 sentinels are the game's own float values, not FLT_MAX.
void hkCollideCapsuleUtilCapsVsTri(
    const hkVector4* capsAPoints,
    float capsARadius,
    const hkVector4* triVertices,
    float triBRadius,
    const hkpCollideTriangleUtil::PointTriangleDistanceCache& cache,
    float collisionTolerance,
    int32_t searchManifold,
    hkContactPoint* points,
    hkpFeatureOutput* featuresOut
) {
    if (featuresOut) {
        for (int32_t i = 0; i < 3; ++i) {
            featuresOut[i].numFeatures = 0;
        }
    }

    // EXE-INLINED none: hkCollideCapsuleUtil_calcTrianglePlaneDirections
    // The triangle's normal (scaled by m_invNormalLen) and its three edge planes, transposed into the rotation of
    // planeEquations with the normal in w and -m_normalLen * m_invEdgeLen[0] as the translation.
    const __m128 kT0 = _mm_load_ps(&reinterpret_cast<const float*>(&triVertices[0].m_quad)[0]);
    const __m128 kT1 = _mm_load_ps(&reinterpret_cast<const float*>(&triVertices[1].m_quad)[0]);
    const __m128 kT2 = _mm_load_ps(&reinterpret_cast<const float*>(&triVertices[2].m_quad)[0]);
    const __m128 kE0 = _mm_sub_ps(kT2, kT1);
    const __m128 kE1 = _mm_sub_ps(kT0, kT2);
    const __m128 kE2 = _mm_sub_ps(kT1, kT0);
    const __m128 kNormal = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kE1, kE1, 0xD2), _mm_shuffle_ps(kE0, kE0, 0xC9)),
            _mm_mul_ps(_mm_shuffle_ps(kE1, kE1, 0xC9), _mm_shuffle_ps(kE0, kE0, 0xD2))
        ),
        _mm_set1_ps(cache.m_invNormalLen)
    );
    const __m128 kS0 = _mm_mul_ps(_mm_set1_ps(cache.m_invEdgeLen[0]), kE0);
    const __m128 kS1 = _mm_mul_ps(_mm_set1_ps(cache.m_invEdgeLen[1]), kE1);
    const __m128 kS2 = _mm_mul_ps(_mm_set1_ps(cache.m_invEdgeLen[2]), kE2);
    const __m128 kNormalC9 = _mm_shuffle_ps(kNormal, kNormal, 0xC9);
    const __m128 kNormalD2 = _mm_shuffle_ps(kNormal, kNormal, 0xD2);
    alignas(16) hkVector4 planeEquations[3];
    _mm_store_ps(
        &reinterpret_cast<float*>(&planeEquations[0].m_quad)[0],
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kS0, kS0, 0xC9), kNormalD2), _mm_mul_ps(_mm_shuffle_ps(kS0, kS0, 0xD2), kNormalC9)
        )
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&planeEquations[1].m_quad)[0],
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kS1, kS1, 0xC9), kNormalD2), _mm_mul_ps(_mm_shuffle_ps(kS1, kS1, 0xD2), kNormalC9)
        )
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&planeEquations[2].m_quad)[0],
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kS2, kS2, 0xC9), kNormalD2), _mm_mul_ps(_mm_shuffle_ps(kS2, kS2, 0xD2), kNormalC9)
        )
    );
    float* pRow0 = &reinterpret_cast<float*>(&planeEquations[0].m_quad)[0];
    float* pRow1 = &reinterpret_cast<float*>(&planeEquations[1].m_quad)[0];
    float* pRow2 = &reinterpret_cast<float*>(&planeEquations[2].m_quad)[0];
    float fSwap = pRow1[0];
    pRow1[0] = pRow0[1];
    pRow0[1] = fSwap;
    fSwap = pRow2[0];
    pRow2[0] = pRow0[2];
    pRow0[2] = fSwap;
    fSwap = pRow2[1];
    pRow2[1] = pRow1[2];
    pRow1[2] = fSwap;
    alignas(16) hkVector4 normal;
    _mm_store_ps(&reinterpret_cast<float*>(&normal.m_quad)[0], kNormal);
    pRow0[3] = normal.m_quad[0];
    pRow1[3] = normal.m_quad[1];
    pRow2[3] = normal.m_quad[2];

    // The capsule's end points relative to the triangle: x, y and z their edge plane distances, w the plane distance
    // (hkVector4::_setMul4xyz1 with planeEquations).
    alignas(16) hkVector4 projected[2];
    alignas(16) hkVector4 translation;
    _mm_store_ps(&reinterpret_cast<float*>(&translation.m_quad)[0], _mm_setzero_ps());
    _mm_store_ps(
        &reinterpret_cast<float*>(&projected[0].m_quad)[0],
        _mm_sub_ps(_mm_load_ps(&reinterpret_cast<const float*>(&capsAPoints[0].m_quad)[0]), kT0)
    );
    translation.m_quad[0] = cache.m_normalLen * -cache.m_invEdgeLen[0];
    _mm_store_ps(
        &reinterpret_cast<float*>(&projected[1].m_quad)[0],
        _mm_sub_ps(_mm_load_ps(&reinterpret_cast<const float*>(&capsAPoints[1].m_quad)[0]), kT0)
    );
    const __m128 kRow0 = _mm_load_ps(pRow0);
    const __m128 kRow1 = _mm_load_ps(pRow1);
    const __m128 kRow2 = _mm_load_ps(pRow2);
    const __m128 kTranslation = _mm_load_ps(&reinterpret_cast<float*>(&translation.m_quad)[0]);
    for (int32_t i = 1; i >= 0; --i) {
        const __m128 kPoint = _mm_load_ps(&reinterpret_cast<float*>(&projected[i].m_quad)[0]);
        _mm_store_ps(
            &reinterpret_cast<float*>(&projected[i].m_quad)[0],
            _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(kPoint, kPoint, 0x55), kRow1),
                    _mm_mul_ps(_mm_shuffle_ps(kPoint, kPoint, 0x00), kRow0)
                ),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kPoint, kPoint, 0xAA), kRow2), kTranslation)
            )
        );
    }

    const float fRadiusSum = capsARadius + triBRadius;
    const float fMaxDistance = fRadiusSum + collisionTolerance;
    points[0].m_separatingNormal.m_quad[3] = 3.40282e+38f;
    points[1].m_separatingNormal.m_quad[3] = 3.40282e+38f;
    points[2].m_separatingNormal.m_quad[3] = 3.40282e+38f;
    const __m128 kProjected0 = _mm_load_ps(&reinterpret_cast<float*>(&projected[0].m_quad)[0]);
    const __m128 kProjected1 = _mm_load_ps(&reinterpret_cast<float*>(&projected[1].m_quad)[0]);
    const __m128 kMaxDistance = _mm_set1_ps(fMaxDistance);
    // Both end points outside the same edge plane or above the triangle: no contact.
    if (_mm_movemask_ps(_mm_and_ps(_mm_cmplt_ps(kMaxDistance, kProjected0), _mm_cmplt_ps(kMaxDistance, kProjected1)))) {
        return;
    }
    const float fNegMaxDistance = -fMaxDistance;
    const float fW0 = projected[0].m_quad[3];
    const float fW1 = projected[1].m_quad[3];
    if (fNegMaxDistance > fW0 && fW1 < fNegMaxDistance) {
        return;
    }

    // Per end point, the planes it lies below (bits 0-2 the edge planes, bit 3 the triangle's plane).
    int32_t outside[2];
    outside[0] = _mm_movemask_ps(_mm_cmplt_ps(kProjected0, _mm_setzero_ps()));
    outside[1] = _mm_movemask_ps(_mm_cmplt_ps(kProjected1, _mm_setzero_ps()));

    if ((outside[0] ^ outside[1]) & 8) {
        // The capsule crosses the triangle's plane: where it does, inside all three edges, it penetrates.
        const float t = static_cast<float>(static_cast<double>(fW0) / (static_cast<double>(fW0) - fW1));
        const __m128 kT = _mm_set1_ps(t);
        const __m128 kCrossing = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&hkQuadReal1111_0)[0]), kT), kProjected0),
            _mm_mul_ps(kProjected1, kT)
        );
        if ((_mm_movemask_ps(
                 _mm_cmplt_ps(kCrossing, _mm_load_ps(&reinterpret_cast<float*>(&hkVector4Zero.m_quad)[0]))
             ) &
             7) == 7) {
            hkContactPoint& cp = points[1];
            cp.m_separatingNormal.m_quad[3] = -3.40282e+38f;
            // The deeper end point along the normal.
            for (int32_t i = 0; i < 2; ++i) {
                const int32_t iOutside = outside[i];
                if ((iOutside & 7) == 7) {
                    const float fAbsDistance = __builtin_fabsf(projected[i].m_quad[3]);
                    const float fDistance = -fAbsDistance - fRadiusSum;
                    if (cp.m_separatingNormal.m_quad[3] < fDistance) {
                        if (iOutside & 8) {
                            _mm_store_ps(&reinterpret_cast<float*>(&cp.m_separatingNormal.m_quad)[0], kNormal);
                            const float fScale = triBRadius - projected[i].m_quad[3];
                            cp.m_separatingNormal.m_quad[3] = fDistance;
                            _mm_store_ps(
                                &reinterpret_cast<float*>(&cp.m_position.m_quad)[0],
                                _mm_add_ps(
                                    _mm_mul_ps(_mm_set1_ps(fScale), kNormal),
                                    _mm_load_ps(&reinterpret_cast<const float*>(&capsAPoints[i].m_quad)[0])
                                )
                            );
                        }
                        else {
                            cp.m_separatingNormal.setNeg4(normal);
                            const float fScale = -triBRadius - projected[i].m_quad[3];
                            cp.m_separatingNormal.m_quad[3] = fDistance;
                            _mm_store_ps(
                                &reinterpret_cast<float*>(&cp.m_position.m_quad)[0],
                                _mm_add_ps(
                                    _mm_mul_ps(_mm_set1_ps(fScale), kNormal),
                                    _mm_load_ps(&reinterpret_cast<const float*>(&capsAPoints[i].m_quad)[0])
                                )
                            );
                        }
                    }
                }
            }

            // The capsule's segment against each edge, nearer than the contact so far.
            alignas(16) hkVector4 dir;
            _mm_store_ps(&reinterpret_cast<float*>(&dir.m_quad)[0], _mm_sub_ps(kProjected1, kProjected0));
            const float* pDir = &reinterpret_cast<float*>(&dir.m_quad)[0];
            const float* pProjected0 = &reinterpret_cast<float*>(&projected[0].m_quad)[0];
            const float fDirW = dir.m_quad[3];
            const double dDirWSquared = static_cast<double>(fDirW) * fDirW;
            for (int32_t i = 0; i < 3; ++i) {
                float fW = fDirW;
                float fD = pDir[i];
                const float fLengthSquared = static_cast<float>(static_cast<double>(fD) * fD + dDirWSquared);
                const float fLengthSquaredInv = static_cast<float>(1.0 / fLengthSquared);
                const float fCross =
                    static_cast<float>(static_cast<double>(fD) * fW0 - static_cast<double>(pProjected0[i]) * fDirW);
                const float fDistanceSquared =
                    static_cast<float>(static_cast<double>(fCross) * fCross * fLengthSquaredInv);
                const float fLimit = cp.m_separatingNormal.m_quad[3] + fRadiusSum;
                if (!(fDistanceSquared < static_cast<double>(fLimit) * fLimit)) {
                    continue;
                }
                const float fDot =
                    static_cast<float>(static_cast<double>(pProjected0[i]) * fD + static_cast<double>(fDirW) * fW0);
                const float project = static_cast<float>(static_cast<double>(fDot) * -fLengthSquaredInv);
                if (!(project > -9.999999747378752e-05 && project < 1.0001f)) {
                    continue;
                }
                const __m128 kEdgePlane = _mm_setr_ps(pRow0[i], pRow1[i], pRow2[i], 0.0f);
                if (0.0 > fDirW) {
                    fW = -fDirW;
                    fD = -fD;
                }
                const float fNegD = -fD;
                _mm_store_ps(
                    &reinterpret_cast<float*>(&cp.m_separatingNormal.m_quad)[0],
                    _mm_add_ps(_mm_mul_ps(_mm_set1_ps(fW), kEdgePlane), _mm_mul_ps(_mm_set1_ps(fNegD), kNormal))
                );
                cp.m_separatingNormal.normalize3();
                const __m128 kSeparatingNormal = cp.m_separatingNormal.m_quad;
                const __m128 kProject = _mm_set1_ps(project);
                const float fDistance = std::sqrt(fDistanceSquared);
                const float fNegDistance = -fDistance;
                const __m128 kOnCapsule = _mm_add_ps(
                    _mm_mul_ps(
                        _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&hkQuadReal1111_0)[0]), kProject),
                        _mm_load_ps(&reinterpret_cast<const float*>(&capsAPoints[0].m_quad)[0])
                    ),
                    _mm_mul_ps(kProject, _mm_load_ps(&reinterpret_cast<const float*>(&capsAPoints[1].m_quad)[0]))
                );
                const float fScale = triBRadius - fNegDistance;
                cp.m_separatingNormal.m_quad[3] = fNegDistance - fRadiusSum;
                _mm_store_ps(
                    &reinterpret_cast<float*>(&cp.m_position.m_quad)[0],
                    _mm_add_ps(kOnCapsule, _mm_mul_ps(_mm_set1_ps(fScale), kSeparatingNormal))
                );
            }
            if (featuresOut) {
                for (int32_t i = 0; i < 3; ++i) {
                    featuresOut[i].featureIds[0] = 0;
                    featuresOut[i].featureIds[1] = 1;
                    featuresOut[i].numFeatures = 3;
                    featuresOut[i].featureIds[2] = 2;
                }
            }
            return;
        }
    }

    const int32_t iOutsideAny = outside[0] | outside[1];
    alignas(16) hkVector4 dA;
    _mm_store_ps(
        &reinterpret_cast<float*>(&dA.m_quad)[0],
        _mm_sub_ps(
            _mm_load_ps(&reinterpret_cast<const float*>(&capsAPoints[1].m_quad)[0]),
            _mm_load_ps(&reinterpret_cast<const float*>(&capsAPoints[0].m_quad)[0])
        )
    );
    hkContactPoint* pPoint = points;
    bool bSinglePoint = false;
    int32_t iPoint = 0;
    if ((iOutsideAny & 7) != 7) {
        if (searchManifold) {
            // The capsule against each edge the end points are not both outside of, keeping the closest result in
            // points[0].
            points[0].m_separatingNormal.m_quad[3] = collisionTolerance;
            points[1].m_separatingNormal.m_quad[3] = collisionTolerance;
            points[2].m_separatingNormal.m_quad[3] = collisionTolerance;
            projected[1].m_quad[3] = 3.40282e+38f;
            int32_t iBit = 1;
            for (int32_t i = 0; i < 3; ++i) {
                if (!(iOutsideAny & iBit)) {
                    const int32_t iStart = hkpCollideTriangleUtil::vertexToEdgeLut[i + 2];
                    const int32_t iEnd = hkpCollideTriangleUtil::vertexToEdgeLut[i];
                    alignas(16) hkVector4 edge[2];
                    _mm_store_ps(
                        &reinterpret_cast<float*>(&edge[0].m_quad)[0],
                        _mm_load_ps(&reinterpret_cast<const float*>(&triVertices[iStart].m_quad)[0])
                    );
                    _mm_store_ps(
                        &reinterpret_cast<float*>(&edge[1].m_quad)[0],
                        _mm_load_ps(&reinterpret_cast<const float*>(&triVertices[iEnd].m_quad)[0])
                    );
                    const int32_t iFlags =
                        hkCollideCapsuleUtilManifoldCapsVsCaps(capsAPoints, capsARadius, edge, triBRadius, points);
                    if (featuresOut) {
                        // Writes every entry, not only this edge's.
                        for (int32_t k = 0; k < 3; ++k) {
                            featuresOut[k].featureIds[0] = static_cast<uint16_t>(iStart);
                            if (iFlags & 0xC) {
                                featuresOut[k].numFeatures = 1;
                            }
                            else {
                                featuresOut[k].numFeatures = 2;
                                featuresOut[k].featureIds[1] = static_cast<uint16_t>(iEnd);
                            }
                        }
                    }
                    if (projected[1].m_quad[3] < points[0].m_separatingNormal.m_quad[3]) {
                        _mm_store_ps(
                            &reinterpret_cast<float*>(&points[0].m_position.m_quad)[0],
                            _mm_load_ps(&reinterpret_cast<float*>(&projected[0].m_quad)[0])
                        );
                        _mm_store_ps(
                            &reinterpret_cast<float*>(&points[0].m_separatingNormal.m_quad)[0],
                            _mm_load_ps(&reinterpret_cast<float*>(&projected[1].m_quad)[0])
                        );
                    }
                    else {
                        _mm_store_ps(
                            &reinterpret_cast<float*>(&projected[0].m_quad)[0],
                            _mm_load_ps(&reinterpret_cast<float*>(&points[0].m_position.m_quad)[0])
                        );
                        _mm_store_ps(
                            &reinterpret_cast<float*>(&projected[1].m_quad)[0],
                            _mm_load_ps(&reinterpret_cast<float*>(&points[0].m_separatingNormal.m_quad)[0])
                        );
                    }
                }
                iBit <<= 1;
            }
            return;
        }
        bSinglePoint = true;
    }

    // Per end point: inside all three edge planes it touches the face; otherwise the capsule's segment is tested
    // against the edges it is outside of. Without searchManifold only one point is made, from both end points. One
    // result is shared by every edge test, and an edge whose end is touched leaves the edge's plane direction in its
    // m_closestAminusClosestB for the next test.
    hkLineSegmentUtil::ClosestLineSegLineSegResult result;
    for (;;) {
        if (!bSinglePoint) {
            if (iPoint >= 2) {
                return;
            }
            const int32_t iOutside = outside[iPoint];
            if ((iOutside & 7) == 7) {
                const float fW = projected[iPoint].m_quad[3];
                const __m128 kFaceNormal = _mm_load_ps(&reinterpret_cast<float*>(&normal.m_quad)[0]);
                hkVector4 kSeparating;
                float fDistance;
                if (iOutside & 8) {
                    const float fScale = -triBRadius - fW;
                    fDistance = -fW;
                    _mm_store_ps(
                        &reinterpret_cast<float*>(&pPoint->m_position.m_quad)[0],
                        _mm_add_ps(
                            _mm_mul_ps(_mm_set1_ps(fScale), kFaceNormal),
                            _mm_load_ps(&reinterpret_cast<const float*>(&capsAPoints[iPoint].m_quad)[0])
                        )
                    );
                    kSeparating.setNeg4(normal);
                }
                else {
                    const float fScale = triBRadius - fW;
                    fDistance = fW;
                    _mm_store_ps(
                        &reinterpret_cast<float*>(&pPoint->m_position.m_quad)[0],
                        _mm_add_ps(
                            _mm_mul_ps(_mm_set1_ps(fScale), kFaceNormal),
                            _mm_load_ps(&reinterpret_cast<const float*>(&capsAPoints[iPoint].m_quad)[0])
                        )
                    );
                    kSeparating.m_quad = kFaceNormal;
                }
                _mm_store_ps(&reinterpret_cast<float*>(&pPoint->m_separatingNormal.m_quad)[0], kSeparating.m_quad);
                pPoint->m_separatingNormal.m_quad[3] = fDistance - fRadiusSum;
                if (featuresOut) {
                    featuresOut[iPoint].numFeatures = 3;
                    featuresOut[iPoint].featureIds[0] = 0;
                    featuresOut[iPoint].featureIds[1] = 1;
                    featuresOut[iPoint].featureIds[2] = 2;
                }
                ++pPoint;
                ++iPoint;
                continue;
            }
        }

        int32_t iEndMask = 1 << iPoint;
        int32_t iBit = 1;
        for (int32_t i = 0; i < 3; ++i, iBit <<= 1) {
            const bool bSkip = bSinglePoint ? (outside[1] & outside[0] & iBit) != 0 : (outside[iPoint] & iBit) != 0;
            if (bSkip) {
                continue;
            }
            const int32_t iEnd = hkpCollideTriangleUtil::vertexToEdgeLut[i];
            const int32_t iStart = hkpCollideTriangleUtil::vertexToEdgeLut[i + 2];
            alignas(16) hkVector4 edge;
            _mm_store_ps(
                &reinterpret_cast<float*>(&edge.m_quad)[0],
                _mm_sub_ps(
                    _mm_load_ps(&reinterpret_cast<const float*>(&triVertices[iEnd].m_quad)[0]),
                    _mm_load_ps(&reinterpret_cast<const float*>(&triVertices[iStart].m_quad)[0])
                )
            );
            const int32_t iFlags =
                hkLineSegmentUtil::closestLineSegLineSeg(capsAPoints[0], dA, triVertices[iStart], edge, result);
            if (featuresOut) {
                featuresOut[iPoint].featureIds[0] = static_cast<uint16_t>(iStart);
                if (iFlags & 0xC) {
                    featuresOut[iPoint].numFeatures = 1;
                }
                else {
                    featuresOut[iPoint].numFeatures = 2;
                    featuresOut[iPoint].featureIds[1] = static_cast<uint16_t>(iEnd);
                }
            }
            const float fLimit = pPoint->m_separatingNormal.m_quad[3] + fRadiusSum;
            const float fLimitSquared = fLimit * fLimit;
            const float fDistanceSquared = result.m_distanceSquared;
            if (!(fLimitSquared > fDistanceSquared)) {
                continue;
            }
            if (iFlags & iEndMask) {
                // The closest point is the other end point's: without searchManifold the point moves to it.
                if (!bSinglePoint) {
                    continue;
                }
                iPoint = 1;
                iEndMask = 2;
            }
            const __m128 kEdge = _mm_load_ps(&reinterpret_cast<float*>(&edge.m_quad)[0]);
            const __m128 kDir = _mm_load_ps(&reinterpret_cast<float*>(&dA.m_quad)[0]);
            __m128 kReference;
            __m128 kCandidate;
            if (iFlags == 0) {
                kReference = _mm_load_ps(&reinterpret_cast<float*>(&result.m_closestAminusClosestB.m_quad)[0]);
                kCandidate = _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(kEdge, kEdge, 0xD2), _mm_shuffle_ps(kDir, kDir, 0xC9)),
                    _mm_mul_ps(_mm_shuffle_ps(kEdge, kEdge, 0xC9), _mm_shuffle_ps(kDir, kDir, 0xD2))
                );
            }
            else if (!(fDistanceSquared <= 1.4210855e-14f)) {
                kReference = _mm_load_ps(&reinterpret_cast<float*>(&result.m_closestAminusClosestB.m_quad)[0]);
                kCandidate = kReference;
            }
            else {
                // Touching an end of the edge: the edge's plane direction, or the segments' normal when they are not
                // parallel.
                kReference = _mm_setr_ps(pRow0[i], pRow1[i], pRow2[i], 0.0f);
                result.m_closestAminusClosestB.m_quad = kReference;
                const __m128 kCross = _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(kEdge, kEdge, 0xD2), _mm_shuffle_ps(kDir, kDir, 0xC9)),
                    _mm_mul_ps(_mm_shuffle_ps(kEdge, kEdge, 0xC9), _mm_shuffle_ps(kDir, kDir, 0xD2))
                );
                const __m128 kCrossSquares = _mm_mul_ps(kCross, kCross);
                const float fCrossLength2 = _mm_cvtss_f32(_mm_add_ps(
                    _mm_shuffle_ps(kCrossSquares, kCrossSquares, 0xAA),
                    _mm_add_ps(_mm_shuffle_ps(kCrossSquares, kCrossSquares, 0x55), kCrossSquares)
                ));
                kCandidate = !(fCrossLength2 <= 1.4210855e-14f) ? kCross : kReference;
            }
            hkVector4 kSeparating;
            kSeparating.m_quad = kCandidate;
            kSeparating.normalize3();
            const __m128 kDotProduct = _mm_mul_ps(kReference, kSeparating.m_quad);
            const float fDot = _mm_cvtss_f32(_mm_add_ps(
                _mm_shuffle_ps(kDotProduct, kDotProduct, 0xAA),
                _mm_add_ps(_mm_shuffle_ps(kDotProduct, kDotProduct, 0x55), kDotProduct)
            ));
            float fDistance = fDot;
            if (0.0f > fDot) {
                fDistance = -fDot;
                kSeparating.setNeg4(kSeparating);
            }
            const float fScale = triBRadius - fDistance;
            _mm_store_ps(
                &reinterpret_cast<float*>(&pPoint->m_position.m_quad)[0],
                _mm_add_ps(
                    _mm_mul_ps(_mm_set1_ps(fScale), kSeparating.m_quad),
                    _mm_load_ps(&reinterpret_cast<float*>(&result.m_closestPointA.m_quad)[0])
                )
            );
            _mm_store_ps(&reinterpret_cast<float*>(&pPoint->m_separatingNormal.m_quad)[0], kSeparating.m_quad);
            pPoint->m_separatingNormal.m_quad[3] = fDistance - fRadiusSum;
        }
        if (bSinglePoint) {
            return;
        }
        ++pPoint;
        ++iPoint;
    }
}

// From a triangle's three vertices, writes the normal (the cross product of the edges v0 - v2 and v2 - v1, scaled by
// params[3]) into normalOut, and into out a matrix whose first three columns are, after the transposition of their xyz,
// the edges' cross products with the normal (each edge scaled by its weight params[0..2]), with the normal's components
// in their w lanes; the last column is (params[4] * -params[0], 0, 0, 0). Returns out.
hkMatrix4*
calcTriangleBarycentricPlanes(hkVector4* normalOut, const float* params, hkMatrix4* out, const hkVector4* vertices) {
    const __m128 kV0 = vertices[0].m_quad;
    const __m128 kV1 = vertices[1].m_quad;
    const __m128 kV2 = vertices[2].m_quad;
    const __m128 kEdge12 = _mm_sub_ps(kV2, kV1);
    const __m128 kEdge01 = _mm_sub_ps(kV1, kV0);
    const __m128 kEdge20 = _mm_sub_ps(kV0, kV2);
    const __m128 kCross = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kEdge20, kEdge20, 0xD2), _mm_shuffle_ps(kEdge12, kEdge12, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(kEdge20, kEdge20, 0xC9), _mm_shuffle_ps(kEdge12, kEdge12, 0xD2))
    );
    const __m128 kNormal = _mm_mul_ps(kCross, _mm_load1_ps(&params[3]));
    normalOut->m_quad = kNormal;

    const __m128 kA = _mm_mul_ps(_mm_load1_ps(&params[0]), kEdge12);
    const __m128 kB = _mm_mul_ps(_mm_load1_ps(&params[1]), kEdge20);
    const __m128 kC = _mm_mul_ps(_mm_load1_ps(&params[2]), kEdge01);
    const __m128 kNormalC9 = _mm_shuffle_ps(kNormal, kNormal, 0xC9);
    const __m128 kNormalD2 = _mm_shuffle_ps(kNormal, kNormal, 0xD2);
    out->m_col0.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kA, kA, 0xC9), kNormalD2), _mm_mul_ps(_mm_shuffle_ps(kA, kA, 0xD2), kNormalC9)
    );
    out->m_col1.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xC9), kNormalD2), _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xD2), kNormalC9)
    );
    out->m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kC, kC, 0xC9), kNormalD2), _mm_mul_ps(_mm_shuffle_ps(kC, kC, 0xD2), kNormalC9)
    );

    float fTmp = out->m_col1.m_quad[0];
    out->m_col1.m_quad[0] = out->m_col0.m_quad[1];
    out->m_col0.m_quad[1] = fTmp;
    fTmp = out->m_col2.m_quad[0];
    out->m_col2.m_quad[0] = out->m_col0.m_quad[2];
    out->m_col0.m_quad[2] = fTmp;
    fTmp = out->m_col2.m_quad[1];
    out->m_col2.m_quad[1] = out->m_col1.m_quad[2];
    out->m_col1.m_quad[2] = fTmp;
    out->m_col0.m_quad[3] = normalOut->m_quad[0];
    out->m_col1.m_quad[3] = normalOut->m_quad[1];
    out->m_col2.m_quad[3] = normalOut->m_quad[2];
    out->m_col3.m_quad = _mm_setzero_ps();
    out->m_col3.m_quad[0] = params[4] * -params[0];
    return out;
}
