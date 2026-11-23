#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/ConvexHull/hkPlaneEquationUtil.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

hkVector4 hkVector4Zero(0.0f, 0.0f, 0.0f, 0.0f);

// Finds the extreme points along the line where planes planeA and planeB meet: every other plane's intersection point
// with the two (hkPlaneEquationUtil::findTriPlaneIntersectionPoint) that lies on the inner side of the current
// extreme (within 1.1920929e-07) replaces it, maxOut starting 100000 along dir from the first point found and minOut
// at the negation of that. A side that found no point takes the other side's plane indices; returns 1 when neither
// found one, else 0. Each dot product is stored as a float and compared on the x87 stack.
int32_t findPlanarIntersections(
    const hkVector4* dir,
    const hkArray<hkVector4>* planes,
    int32_t planeA,
    int32_t planeB,
    hkPlaneEquationUtil::IntersectionPoint* minOut,
    hkPlaneEquationUtil::IntersectionPoint* maxOut
) {
    minOut->m_planeA = -1;
    minOut->m_planeB = -1;
    minOut->m_planeC = -1;
    maxOut->m_planeA = -1;
    maxOut->m_planeB = -1;
    maxOut->m_planeC = -1;
    bool initialized = false;
    for (int32_t i = 0; i < planes->m_size; ++i) {
        if (i == planeA || i == planeB) {
            continue;
        }
        hkPlaneEquationUtil::IntersectionPoint point;
        if (hkPlaneEquationUtil::findTriPlaneIntersectionPoint(*planes, planeA, planeB, i, point) != HK_SUCCESS) {
            continue;
        }
        const __m128 kPoint = point.m_location.m_quad;
        if (!initialized) {
            const __m128 kFar = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(-100000.0f), dir->m_quad), kPoint);
            maxOut->m_location.m_quad = kFar;
            minOut->m_location.m_quad = _mm_xor_ps(kFar, hkQuadRealNegateMask);
            initialized = true;
        }
        const __m128 kMaxDot = _mm_mul_ps(_mm_sub_ps(maxOut->m_location.m_quad, kPoint), planes->m_data[i].m_quad);
        const float maxDot = _mm_cvtss_f32(_mm_add_ss(
            _mm_shuffle_ps(kMaxDot, kMaxDot, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(kMaxDot, kMaxDot, 0x55), kMaxDot)
        ));
        if (maxDot > -HK_REAL_EPSILON) {
            maxOut->m_planeA = point.m_planeA;
            maxOut->m_location.m_quad = kPoint;
            maxOut->m_planeB = point.m_planeB;
            maxOut->m_planeC = point.m_planeC;
        }
        const __m128 kMinDot = _mm_mul_ps(_mm_sub_ps(minOut->m_location.m_quad, kPoint), planes->m_data[i].m_quad);
        const float minDot = _mm_cvtss_f32(_mm_add_ss(
            _mm_shuffle_ps(kMinDot, kMinDot, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(kMinDot, kMinDot, 0x55), kMinDot)
        ));
        if (minDot > -HK_REAL_EPSILON) {
            minOut->m_location.m_quad = kPoint;
            minOut->m_planeA = point.m_planeA;
            minOut->m_planeB = point.m_planeB;
            minOut->m_planeC = point.m_planeC;
        }
    }
    if (minOut->m_planeA == -1) {
        if (maxOut->m_planeA == -1) {
            return 1;
        }
        minOut->m_planeA = maxOut->m_planeA;
        minOut->m_planeB = maxOut->m_planeB;
        minOut->m_planeC = maxOut->m_planeC;
    }
    if (maxOut->m_planeA == -1) {
        if (minOut->m_planeA == -1) {
            return 1;
        }
        maxOut->m_planeA = minOut->m_planeA;
        maxOut->m_planeB = minOut->m_planeB;
        maxOut->m_planeC = minOut->m_planeC;
    }
    return 0;
}

hkVector4::hkVector4(float afX, float afY, float afZ, float afW) {
    XFNV_M128_F32(m_quad, 0) = afX;
    XFNV_M128_F32(m_quad, 1) = afY;
    XFNV_M128_F32(m_quad, 2) = afZ;
    XFNV_M128_F32(m_quad, 3) = afW;
}

// Computes (v.y * col1 + v.x * col0) + (v.z * col2 + translation) in this order. Each step rounds to float, so each
// goes through a float local; the four components are stored once, after all inputs are read.
void hkVector4::setTransformedPos(const hkTransform& t, const hkVector4& v) {
    const hkRotation& r = t.m_rotation;
    float fX = XFNV_M128_F32(v.m_quad, 0);
    float fY = XFNV_M128_F32(v.m_quad, 1);
    float fZ = XFNV_M128_F32(v.m_quad, 2);
    hkQuadReal kResult = _mm_setzero_ps();
    for (int32_t i = 0; i < 4; ++i) {
        float fYCol1 = fY * (&reinterpret_cast<const float*>(&r.m_col1.m_quad)[0])[i];
        float fXCol0 = fX * (&reinterpret_cast<const float*>(&r.m_col0.m_quad)[0])[i];
        float fXY = fYCol1 + fXCol0;
        float fZCol2 = fZ * (&reinterpret_cast<const float*>(&r.m_col2.m_quad)[0])[i];
        float fZT = fZCol2 + (&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0])[i];
        (&reinterpret_cast<float*>(&kResult)[0])[i] = fXY + fZT;
    }
    m_quad = kResult;
}

void hkVector4::setTransformedPos(const hkQsTransform& t, const hkVector4& v) {
    hkQuaternion v3;
    hkVector4 v9;
    v3.m_vec.m_quad = t.m_rotation.m_vec.m_quad;
    __m128 v4 = _mm_mul_ps(t.m_scale.m_quad, v.m_quad);
    __m128 v5 = _mm_mul_ps(v3.m_vec.m_quad, v4);
    __m128 v6 = _mm_shuffle_ps(v5, v5, 170);
    XFNV_M128_F32(v6, 0) = XFNV_M128_F32(v6, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 85), 0) + XFNV_M128_F32(v5, 0));
    __m128 v7 = _mm_shuffle_ps(v3.m_vec.m_quad, v3.m_vec.m_quad, 255);
    __m128 v8 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(v3.m_vec.m_quad, v3.m_vec.m_quad, 201), _mm_shuffle_ps(v4, v4, 210)),
                _mm_mul_ps(_mm_shuffle_ps(v3.m_vec.m_quad, v3.m_vec.m_quad, 210), _mm_shuffle_ps(v4, v4, 201))
            ),
            v7
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v7, v7), hkQuadRealMinusHalf), v4),
            _mm_mul_ps(_mm_shuffle_ps(v6, v6, 0), v3.m_vec.m_quad)
        )
    );
    v9.m_quad = _mm_add_ps(v8, v8);
    m_quad = v9.m_quad;
    m_quad = _mm_add_ps(t.m_translation.m_quad, v9.m_quad);
}

hkBool hkVector4::isOk3() const {
    hkBool kResult;
    isOkN(3, &kResult, this);
    return kResult;
}

void hkVector4::setAll3(float a) {
    XFNV_M128_F32(m_quad, 0) = a;
    XFNV_M128_F32(m_quad, 1) = a;
    XFNV_M128_F32(m_quad, 2) = a;
}

// Computes (a.y * col1 + a.x * col0) + a.z * col2 in that order, with each step rounded to float as in
// setTransformedPos.
void hkVector4::setMul3(const hkMatrix3& r, const hkVector4& b) {
    float fX = XFNV_M128_F32(b.m_quad, 0);
    float fY = XFNV_M128_F32(b.m_quad, 1);
    float fZ = XFNV_M128_F32(b.m_quad, 2);
    hkQuadReal kResult = _mm_setzero_ps();
    for (int32_t i = 0; i < 4; ++i) {
        float fYCol1 = fY * (&reinterpret_cast<const float*>(&r.m_col1.m_quad)[0])[i];
        float fXCol0 = fX * (&reinterpret_cast<const float*>(&r.m_col0.m_quad)[0])[i];
        float fXY = fYCol1 + fXCol0;
        float fZCol2 = fZ * (&reinterpret_cast<const float*>(&r.m_col2.m_quad)[0])[i];
        (&reinterpret_cast<float*>(&kResult)[0])[i] = fXY + fZCol2;
    }
    m_quad = kResult;
}

// The rotation's turn on a direction vector: (a.y * col1 + a.x * col0) + a.z * col2, in this order, each step rounded
// to float as in setMul3.
void hkVector4::setRotatedDir(const hkRotation& a, const hkVector4& b) {
    float fX = XFNV_M128_F32(b.m_quad, 0);
    float fY = XFNV_M128_F32(b.m_quad, 1);
    float fZ = XFNV_M128_F32(b.m_quad, 2);
    hkQuadReal kResult = _mm_setzero_ps();
    for (int32_t i = 0; i < 4; ++i) {
        float fYCol1 = fY * (&reinterpret_cast<const float*>(&a.m_col1.m_quad)[0])[i];
        float fXCol0 = fX * (&reinterpret_cast<const float*>(&a.m_col0.m_quad)[0])[i];
        float fXY = fYCol1 + fXCol0;
        float fZCol2 = fZ * (&reinterpret_cast<const float*>(&a.m_col2.m_quad)[0])[i];
        (&reinterpret_cast<float*>(&kResult)[0])[i] = fXY + fZCol2;
    }
    m_quad = kResult;
}

// The point b taken into the space of a: the inverse transform applied to it.
void hkVector4::setTransformedInversePos(const hkTransform& a, const hkVector4& b) {
    hkVector4 v3;
    hkVector4 v4;
    v3.m_quad = a.m_rotation.m_col1.m_quad;
    v4.m_quad = a.m_rotation.m_col2.m_quad;
    __m128 v5 = _mm_sub_ps(b.m_quad, a.m_translation.m_quad);
    __m128 v6 = _mm_unpacklo_ps(a.m_rotation.m_col0.m_quad, v3.m_quad);
    __m128 v7 = _mm_movelh_ps(v6, v4.m_quad);
    m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v5, v5, 85), _mm_shuffle_ps(_mm_movehl_ps(v7, v6), v4.m_quad, 212)),
            _mm_mul_ps(_mm_shuffle_ps(v5, v5, 0), v7)
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(_mm_unpackhi_ps(a.m_rotation.m_col0.m_quad, v3.m_quad), v4.m_quad, 228),
            _mm_shuffle_ps(v5, v5, 170)
        )
    );
}

// The point b taken into the space of the quaternion-scale transform a.
void hkVector4::setTransformedInversePos(const hkQsTransform& a, const hkVector4& b) {
    hkQuaternion v3;
    hkVector4 v9;
    hkVector4 v10;
    v3.m_vec.m_quad = a.m_rotation.m_vec.m_quad;
    __m128 v4 = _mm_sub_ps(b.m_quad, a.m_translation.m_quad);
    __m128 v5 = _mm_mul_ps(v3.m_vec.m_quad, v4);
    __m128 v6 = _mm_shuffle_ps(v5, v5, 170);
    XFNV_M128_F32(v6, 0) = XFNV_M128_F32(v6, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 85), 0) + XFNV_M128_F32(v5, 0));
    __m128 v7 = _mm_shuffle_ps(v3.m_vec.m_quad, v3.m_vec.m_quad, 255);
    __m128 v8 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(v3.m_vec.m_quad, v3.m_vec.m_quad, 210), _mm_shuffle_ps(v4, v4, 201)),
                _mm_mul_ps(_mm_shuffle_ps(v3.m_vec.m_quad, v3.m_vec.m_quad, 201), _mm_shuffle_ps(v4, v4, 210))
            ),
            v7
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v7, v7), hkQuadRealMinusHalf), v4),
            _mm_mul_ps(_mm_shuffle_ps(v6, v6, 0), v3.m_vec.m_quad)
        )
    );
    v9.m_quad = _mm_add_ps(v8, v8);
    m_quad = v9.m_quad;
    v10.m_quad = a.m_scale.m_quad;
    __m128 v11 = _mm_rcp_ps(v10.m_quad);
    m_quad = _mm_mul_ps(_mm_sub_ps(_mm_add_ps(v11, v11), _mm_mul_ps(_mm_mul_ps(v11, v10.m_quad), v11)), v9.m_quad);
}

// As isOk3, with the lane count 4.
hkBool hkVector4::isOk4() const {
    hkBool kResult;
    isOkN(4, &kResult, this);
    return kResult;
}

// Whether x, y and z make a vector of length one within epsilon. The squares are summed as y + x, then z + that, 1.0 is
// subtracted and each step is stored as a float, then fabs is taken; a NaN gives false.
hkBool hkVector4::isNormalized3(float eps) const {
    hkBool kResult;
    if (!isOk3().m_bool) {
        kResult.m_bool = 0;
        return kResult;
    }
    hkQuadReal kSq = _mm_mul_ps(m_quad, m_quad);
    hkQuadReal kXY = _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0x55), kSq);
    hkQuadReal kXYZ = _mm_add_ss(_mm_shuffle_ps(kSq, kSq, 0xAA), kXY);
    float fLengthSquared = XFNV_M128_F32(kXYZ, 0);
    float fDiff = fLengthSquared - 1.0;
    float fAbs = std::fabsf(fDiff);
    kResult.m_bool = eps > fAbs;
    return kResult;
}

// As isNormalized3 over the four lanes: the squares are summed as a horizontal add, and the first lane is taken.
hkBool hkVector4::isNormalized4(float eps) const {
    hkBool kResult;
    if (!isOk4().m_bool) {
        kResult.m_bool = 0;
        return kResult;
    }
    hkQuadReal kSq = _mm_mul_ps(m_quad, m_quad);
    hkQuadReal kSum0 = _mm_add_ps(_mm_shuffle_ps(kSq, kSq, 0x4E), kSq);
    hkQuadReal kSum1 = _mm_add_ps(_mm_shuffle_ps(kSum0, kSum0, 0xB1), kSum0);
    float fLengthSquared = XFNV_M128_F32(kSum1, 0);
    float fDiff = fLengthSquared - 1.0;
    float fAbs = std::fabsf(fDiff);
    kResult.m_bool = eps > fAbs;
    return kResult;
}

double hkVector4::getNextW() const {
    return this[1](3);
}

bool* hkVector4::hasValidAxis(bool* result) {
    __m128 v2 = _mm_mul_ps(m_quad, m_quad);
    *result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(
                  (XFNV_M128_F32(_mm_shuffle_ps(v2, v2, 170), 0) +
                   (XFNV_M128_F32(_mm_shuffle_ps(v2, v2, 85), 0) + XFNV_M128_F32(v2, 0)))
              ))) > HK_REAL_EPSILON;
    return result;
}

// The isOk test: false at the first of the aiCount lanes whose exponent bits are all set (an infinity or a NaN), else
// true.
hkBool* isOkN(int32_t aiCount, hkBool* apResult, const hkVector4* apVector) {
    for (int32_t i = 0; i < aiCount; ++i) {
        if ((std::bit_cast<uint32_t>((*apVector)(i)) & 0x7F800000) == 0x7F800000) {
            *apResult = false;
            return apResult;
        }
    }
    *apResult = true;
    return apResult;
}

// The triple product (b x a) . c, returned as a float.
float calcTripleProduct(const hkVector4* a, const hkVector4* c, const hkVector4* b) {
    const __m128 va = a->m_quad;
    const __m128 vb = b->m_quad;
    const __m128 cross = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(va, va, 0xD2), _mm_shuffle_ps(vb, vb, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(va, va, 0xC9), _mm_shuffle_ps(vb, vb, 0xD2))
    );
    const __m128 product = _mm_mul_ps(cross, c->m_quad);
    return _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(product, product, 0xAA), _mm_add_ss(_mm_shuffle_ps(product, product, 0x55), product))
    );
}

// The direction from the segment apSegment[0] -> apSegment[1] towards the point (or away from it, by aiSide): when the
// point projects outside the segment, from the nearer end (apSegment[1] and apSegment[5] moved down to [0] and [4] when
// that is the second end); else the edge direction crossed twice with the plane normal.
void calcSegmentSearchDirection(hkVector4* apSegment, const hkVector4* apPoint, int32_t aiSide, hkVector4* apOut) {
    const __m128 point = apPoint->m_quad;
    const __m128 a = apSegment[0].m_quad;
    const __m128 b = apSegment[1].m_quad;
    const __m128 ab = _mm_sub_ps(b, a);
    const __m128 pb = _mm_sub_ps(b, point);
    const __m128 mb = _mm_mul_ps(pb, ab);
    const __m128 dotB = _mm_add_ss(_mm_shuffle_ps(mb, mb, 0xAA), _mm_add_ss(_mm_shuffle_ps(mb, mb, 0x55), mb));
    const __m128 pa = _mm_sub_ps(a, point);
    const __m128 ma = _mm_mul_ps(pa, ab);
    const __m128 dotA = _mm_add_ss(_mm_shuffle_ps(ma, ma, 0xAA), _mm_add_ss(_mm_shuffle_ps(ma, ma, 0x55), ma));
    const int32_t signB = _mm_movemask_ps(dotB) & 1;
    const int32_t signA = _mm_movemask_ps(dotA) & 1;
    if (signB == signA) {
        if (signB) {
            apSegment[0].m_quad = apSegment[1].m_quad;
            apSegment[4].m_quad = apSegment[5].m_quad;
        }
        const __m128 nearest = apSegment[0].m_quad;
        apOut->m_quad = (aiSide == 1) ? _mm_sub_ps(point, nearest) : _mm_sub_ps(nearest, point);
        return;
    }
    const __m128 sq = _mm_mul_ps(ab, ab);
    const __m128 lengthSq = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq));
    const __m128 invLength = _mm_rsqrt_ss(lengthSq);
    const __m128 dir = _mm_mul_ps(_mm_shuffle_ps(invLength, invLength, 0), ab);
    const __m128 normal = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(pa, pa, 0xD2), _mm_shuffle_ps(pb, pb, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(pa, pa, 0xC9), _mm_shuffle_ps(pb, pb, 0xD2))
    );
    const __m128 c0 = _mm_mul_ps(_mm_shuffle_ps(normal, normal, 0xC9), _mm_shuffle_ps(dir, dir, 0xD2));
    const __m128 c1 = _mm_mul_ps(_mm_shuffle_ps(normal, normal, 0xD2), _mm_shuffle_ps(dir, dir, 0xC9));
    apOut->m_quad = (aiSide == 1) ? _mm_sub_ps(c1, c0) : _mm_sub_ps(c0, c1);
}

// The edge normal of the triangle vertices scaled by params[3] goes to normalOut; the matrix's first three columns are
// the cross products of the edges scaled by params[0..2] with that normal, transposed in place, with the normal as
// their w; the last column is
// (-params[0] * params[4], 0, 0, 0).
hkMatrix4*
calcTriangleBarycentricMatrix(const hkVector4* vertices, const float* params, hkMatrix4* out, hkVector4* normalOut) {
    __m128 p0 = vertices[0].m_quad;
    __m128 p1 = vertices[1].m_quad;
    __m128 p2 = vertices[2].m_quad;
    __m128 e0 = _mm_sub_ps(p2, p1);
    __m128 e1 = _mm_sub_ps(p1, p0);
    __m128 e2 = _mm_sub_ps(p0, p2);
    __m128 scale = _mm_load_ss(&params[3]);
    __m128 normal = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xD2), _mm_shuffle_ps(e0, e0, 0xC9)),
            _mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xC9), _mm_shuffle_ps(e0, e0, 0xD2))
        ),
        _mm_shuffle_ps(scale, scale, 0)
    );
    normalOut->m_quad = normal;
    __m128 s0 = _mm_load_ss(&params[0]);
    __m128 v0 = _mm_mul_ps(_mm_shuffle_ps(s0, s0, 0), e0);
    __m128 s1 = _mm_load_ss(&params[1]);
    __m128 v1 = _mm_mul_ps(_mm_shuffle_ps(s1, s1, 0), e2);
    __m128 s2 = _mm_load_ss(&params[2]);
    __m128 v2 = _mm_mul_ps(_mm_shuffle_ps(s2, s2, 0), e1);
    __m128 nYzx = _mm_shuffle_ps(normal, normal, 0xC9);
    __m128 nZxy = _mm_shuffle_ps(normal, normal, 0xD2);
    out->m_col0.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v0, v0, 0xC9), nZxy),
        _mm_mul_ps(_mm_shuffle_ps(v0, v0, 0xD2), nYzx)
    );
    out->m_col1.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v1, v1, 0xC9), nZxy),
        _mm_mul_ps(_mm_shuffle_ps(v1, v1, 0xD2), nYzx)
    );
    out->m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v2, v2, 0xC9), nZxy),
        _mm_mul_ps(_mm_shuffle_ps(v2, v2, 0xD2), nYzx)
    );
    float t = XFNV_M128_F32(out->m_col1.m_quad, 0);
    XFNV_M128_F32(out->m_col1.m_quad, 0) = XFNV_M128_F32(out->m_col0.m_quad, 1);
    XFNV_M128_F32(out->m_col0.m_quad, 1) = t;
    t = XFNV_M128_F32(out->m_col2.m_quad, 0);
    XFNV_M128_F32(out->m_col2.m_quad, 0) = XFNV_M128_F32(out->m_col0.m_quad, 2);
    XFNV_M128_F32(out->m_col0.m_quad, 2) = t;
    t = XFNV_M128_F32(out->m_col2.m_quad, 1);
    XFNV_M128_F32(out->m_col2.m_quad, 1) = XFNV_M128_F32(out->m_col1.m_quad, 2);
    XFNV_M128_F32(out->m_col1.m_quad, 2) = t;
    XFNV_M128_F32(out->m_col0.m_quad, 3) = XFNV_M128_F32(normalOut->m_quad, 0);
    XFNV_M128_F32(out->m_col1.m_quad, 3) = XFNV_M128_F32(normalOut->m_quad, 1);
    XFNV_M128_F32(out->m_col2.m_quad, 3) = XFNV_M128_F32(normalOut->m_quad, 2);
    out->m_col3.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(out->m_col3.m_quad, 0) = params[4] * -params[0];
    return out;
}

// The triangle of the vertices at offsets 4, 5 and 6 seen from the one at offset 7: out[0] gets the point of the
// triangle's plane closest to the origin side, out[1] the edge from the 7th vertex to the 4th, out[2] the unit normal
// (flipped to face the direction when it does not). Returns the mask of the triangle's edges the 7th vertex lies
// inside of.
int32_t calcTriangleFaceNormalAndDistance(const hkVector4* vertices, const uint8_t* simplex, hkVector4* out) {
    const uint8_t* base = reinterpret_cast<const uint8_t*>(vertices);
    const __m128* v4 = reinterpret_cast<const __m128*>(base + simplex[4]);
    const __m128* v6 = reinterpret_cast<const __m128*>(base + simplex[6]);
    const __m128* v5 = reinterpret_cast<const __m128*>(base + simplex[5]);
    const __m128* v7 = reinterpret_cast<const __m128*>(base + simplex[7]);
    out[0].m_quad = *v7;
    __m128 edge74 = _mm_sub_ps(*v4, *v7);
    out[1].m_quad = edge74;
    __m128 a = _mm_sub_ps(*v4, *v6);
    __m128 aYzx = _mm_shuffle_ps(a, a, 0xC9);
    __m128 aZxy = _mm_shuffle_ps(a, a, 0xD2);
    __m128 b = _mm_sub_ps(*v6, *v5);
    __m128 bYzx = _mm_shuffle_ps(b, b, 0xC9);
    __m128 bZxy = _mm_shuffle_ps(b, b, 0xD2);
    __m128 c = _mm_sub_ps(*v5, *v4);
    __m128 cross = _mm_sub_ps(_mm_mul_ps(bYzx, aZxy), _mm_mul_ps(bZxy, aYzx));
    __m128 sq = _mm_mul_ps(cross, cross);
    __m128 lenSq = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq));
    __m128 rsq = _mm_rsqrt_ss(lenSq);
    __m128 isZero = _mm_cmpeq_ss(lenSq, _mm_set_ss(0.0f));
    __m128 nr = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(lenSq, rsq), rsq));
    __m128 inv = _mm_andnot_ps(isZero, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), rsq), nr));
    __m128 normal = _mm_mul_ps(_mm_shuffle_ps(inv, inv, 0), cross);
    __m128 d75 = _mm_sub_ps(*v7, *v5);
    __m128 d76 = _mm_sub_ps(*v7, *v6);
    __m128 p7 = *v7;
    __m128 d74 = _mm_sub_ps(p7, *v4);
    __m128 e6 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(d76, d76, 0xC9), aZxy),
        _mm_mul_ps(_mm_shuffle_ps(d76, d76, 0xD2), aYzx)
    );
    __m128 dirDot = _mm_mul_ps(vertices[16].m_quad, normal);
    __m128 e4 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(d74, d74, 0xC9), _mm_shuffle_ps(c, c, 0xD2)),
            _mm_mul_ps(_mm_shuffle_ps(d74, d74, 0xD2), _mm_shuffle_ps(c, c, 0xC9))
        ),
        normal
    );
    __m128 e6n = _mm_mul_ps(e6, normal);
    __m128 e5 = _mm_mul_ps(
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(d75, d75, 0xC9), bZxy), _mm_mul_ps(_mm_shuffle_ps(d75, d75, 0xD2), bYzx)),
        normal
    );
    __m128 h = _mm_mul_ps(edge74, normal);
    __m128 hi = _mm_shuffle_ps(e5, e6n, 0xEE);
    __m128 lo = _mm_shuffle_ps(e5, e6n, 0x44);
    __m128 dist = _mm_add_ss(_mm_shuffle_ps(h, h, 0xAA), _mm_add_ss(_mm_shuffle_ps(h, h, 0x55), h));
    __m128 lo2 = _mm_shuffle_ps(e4, dirDot, 0x44);
    int32_t mask = _mm_movemask_ps(_mm_cmplt_ps(
        _mm_add_ps(
            _mm_shuffle_ps(hi, _mm_shuffle_ps(e4, dirDot, 0xEE), 0x88),
            _mm_add_ps(_mm_shuffle_ps(lo, lo2, 0xDD), _mm_shuffle_ps(lo, lo2, 0x88))
        ),
        _mm_setzero_ps()
    ));
    out[0].m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(dist, dist, 0), normal), p7);
    out[2].m_quad = normal;
    if (mask & 8) {
        out[2].m_quad = _mm_xor_ps(normal, hkQuadRealNegateMask);
    }
    return ~mask & 7;
}

// The same for the triangle of the vertices at offsets 5, 6 and 7 seen from the one at offset 4: out[0] gets the 4th
// vertex, out[1] the edge from the 5th to the 4th, out[2] the unit normal (flipped to face the direction when it does
// not). Returns the mask of the triangle's edges the 4th vertex lies inside of.
int32_t calcTriangleSimplexNormal(const hkVector4* vertices, hkVector4* out, const uint8_t* simplex) {
    const uint8_t* base = reinterpret_cast<const uint8_t*>(vertices);
    const __m128* v4 = reinterpret_cast<const __m128*>(base + simplex[4]);
    const __m128* v5 = reinterpret_cast<const __m128*>(base + simplex[5]);
    const __m128* v6 = reinterpret_cast<const __m128*>(base + simplex[6]);
    const __m128* v7 = reinterpret_cast<const __m128*>(base + simplex[7]);
    out[0].m_quad = *v4;
    out[1].m_quad = _mm_sub_ps(*v4, *v5);
    __m128 p6 = *v6;
    __m128 p5 = *v5;
    __m128 a = _mm_sub_ps(*v7, p6);
    __m128 aZxy = _mm_shuffle_ps(a, a, 0xD2);
    __m128 aYzx = _mm_shuffle_ps(a, a, 0xC9);
    __m128 b = _mm_sub_ps(p5, *v7);
    __m128 bYzx = _mm_shuffle_ps(b, b, 0xC9);
    __m128 c = _mm_sub_ps(p6, p5);
    __m128 bZxy = _mm_shuffle_ps(b, b, 0xD2);
    __m128 cross = _mm_sub_ps(_mm_mul_ps(aYzx, bZxy), _mm_mul_ps(aZxy, bYzx));
    __m128 sq = _mm_mul_ps(cross, cross);
    __m128 lenSq = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq));
    __m128 isZero = _mm_cmpeq_ss(lenSq, _mm_set_ss(0.0f));
    __m128 rsq = _mm_rsqrt_ss(lenSq);
    __m128 nr = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(lenSq, rsq), rsq));
    __m128 inv = _mm_andnot_ps(isZero, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), rsq), nr));
    __m128 normal = _mm_mul_ps(_mm_shuffle_ps(inv, inv, 0), cross);
    __m128 d46 = _mm_sub_ps(*v4, *v6);
    __m128 d47 = _mm_sub_ps(*v4, *v7);
    __m128 d45 = _mm_sub_ps(*v4, *v5);
    __m128 e7 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(d47, d47, 0xC9), bZxy),
        _mm_mul_ps(_mm_shuffle_ps(d47, d47, 0xD2), bYzx)
    );
    __m128 e5 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(d45, d45, 0xC9), _mm_shuffle_ps(c, c, 0xD2)),
            _mm_mul_ps(_mm_shuffle_ps(d45, d45, 0xD2), _mm_shuffle_ps(c, c, 0xC9))
        ),
        normal
    );
    __m128 e6 = _mm_mul_ps(
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(d46, d46, 0xC9), aZxy), _mm_mul_ps(_mm_shuffle_ps(d46, d46, 0xD2), aYzx)),
        normal
    );
    __m128 e7n = _mm_mul_ps(e7, normal);
    __m128 dirDot = _mm_mul_ps(vertices[16].m_quad, normal);
    __m128 hi = _mm_shuffle_ps(_mm_shuffle_ps(e6, e7n, 0xEE), _mm_shuffle_ps(e5, dirDot, 0xEE), 0x88);
    __m128 lo = _mm_shuffle_ps(e6, e7n, 0x44);
    __m128 lo2 = _mm_shuffle_ps(e5, dirDot, 0x44);
    int32_t mask = _mm_movemask_ps(_mm_cmplt_ps(
        _mm_add_ps(hi, _mm_add_ps(_mm_shuffle_ps(lo, lo2, 0xDD), _mm_shuffle_ps(lo, lo2, 0x88))),
        _mm_setzero_ps()
    ));
    out[2].m_quad = normal;
    if (mask & 8) {
        out[2].m_quad = _mm_xor_ps(normal, hkQuadRealNegateMask);
    }
    return ~mask & 7;
}

// From a triangle's three vertices: the normal (the cross product of the edges v0 - v2 and v2 - v1, scaled by
// params[3]) into normalOut, and a matrix whose first three columns are, after the transposition of their xyz, the
// edges' cross products with the normal (each edge scaled by its weight params[0..2]), with the normal's components in
// their w lanes; the last column is (params[4] * -params[0], 0, 0, 0). Returns out.
hkMatrix4*
calcTriangleBarycentricPlanes(const hkVector4* vertices, const float* params, hkMatrix4* out, hkVector4* normalOut) {
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
        _mm_mul_ps(_mm_shuffle_ps(kA, kA, 0xC9), kNormalD2),
        _mm_mul_ps(_mm_shuffle_ps(kA, kA, 0xD2), kNormalC9)
    );
    out->m_col1.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xC9), kNormalD2),
        _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xD2), kNormalC9)
    );
    out->m_col2.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kC, kC, 0xC9), kNormalD2),
        _mm_mul_ps(_mm_shuffle_ps(kC, kC, 0xD2), kNormalC9)
    );

    float fTmp = XFNV_M128_F32(out->m_col1.m_quad, 0);
    XFNV_M128_F32(out->m_col1.m_quad, 0) = XFNV_M128_F32(out->m_col0.m_quad, 1);
    XFNV_M128_F32(out->m_col0.m_quad, 1) = fTmp;
    fTmp = XFNV_M128_F32(out->m_col2.m_quad, 0);
    XFNV_M128_F32(out->m_col2.m_quad, 0) = XFNV_M128_F32(out->m_col0.m_quad, 2);
    XFNV_M128_F32(out->m_col0.m_quad, 2) = fTmp;
    fTmp = XFNV_M128_F32(out->m_col2.m_quad, 1);
    XFNV_M128_F32(out->m_col2.m_quad, 1) = XFNV_M128_F32(out->m_col1.m_quad, 2);
    XFNV_M128_F32(out->m_col1.m_quad, 2) = fTmp;
    XFNV_M128_F32(out->m_col0.m_quad, 3) = XFNV_M128_F32(normalOut->m_quad, 0);
    XFNV_M128_F32(out->m_col1.m_quad, 3) = XFNV_M128_F32(normalOut->m_quad, 1);
    XFNV_M128_F32(out->m_col2.m_quad, 3) = XFNV_M128_F32(normalOut->m_quad, 2);
    out->m_col3.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(out->m_col3.m_quad, 0) = params[4] * -params[0];
    return out;
}

// The dot product is summed as (z + (y + x)); the product with its absolute value is stored as a float.
float calcSignedSquaredDistance(const hkVector4* origin, const hkVector4* direction, const hkVector4* point) {
    const __m128 v = _mm_mul_ps(_mm_sub_ps(point->m_quad, origin->m_quad), direction->m_quad);
    const float d = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(v, v, 0xAA), _mm_add_ss(_mm_shuffle_ps(v, v, 0x55), v)));
    const float absD = std::fabs(d);
    const float result = d * absD;
    return result;
}

// The dot product is summed in the order z + (y + x).
hkVector4* calcPlaneDistance(
    const hkVector4* point,
    const hkVector4* origin,
    const hkVector4* normal,
    hkVector4* normalOut,
    hkVector4* distanceOut
) {
    const __m128 diff = _mm_sub_ps(point->m_quad, origin->m_quad);
    const __m128 v3 = normal->m_quad;
    const __m128 v = _mm_mul_ps(v3, diff);
    normalOut->m_quad = v3;
    distanceOut->m_quad = _mm_add_ss(_mm_shuffle_ps(v, v, 0xAA), _mm_add_ss(_mm_shuffle_ps(v, v, 0x55), v));
    return distanceOut;
}

// The squared lengths of the cross products are summed as z + (y + x). Each radius times its length is rounded to a
// float before the two are summed.
float* calcAngularExtents(
    const hkVector4* normal,
    const hkMotionState* motionA,
    const hkMotionState* motionB,
    const hkVector4* vector,
    float* dotOut,
    float* extentOut,
    const hkVector4* angularVelocities
) {
    const __m128 n = normal->m_quad;
    const __m128 nYzx = _mm_shuffle_ps(n, n, 0xC9);
    const __m128 nZxy = _mm_shuffle_ps(n, n, 0xD2);
    const __m128 angA = angularVelocities[0].m_quad;
    __m128 crossA = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(angA, angA, 0xC9), nZxy),
        _mm_mul_ps(_mm_shuffle_ps(angA, angA, 0xD2), nYzx)
    );
    crossA = _mm_mul_ps(crossA, crossA);
    const __m128 angB = angularVelocities[1].m_quad;
    __m128 crossB = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(angB, angB, 0xC9), nZxy),
        _mm_mul_ps(_mm_shuffle_ps(angB, angB, 0xD2), nYzx)
    );
    crossB = _mm_mul_ps(crossB, crossB);
    const __m128 lenSqA = _mm_add_ss(
        _mm_shuffle_ps(crossA, crossA, 0xAA),
        _mm_add_ss(_mm_shuffle_ps(crossA, crossA, 0x55), crossA)
    );
    const __m128 lenSqB = _mm_add_ss(
        _mm_shuffle_ps(crossB, crossB, 0xAA),
        _mm_add_ss(_mm_shuffle_ps(crossB, crossB, 0x55), crossB)
    );
    const float lenA = _mm_cvtss_f32(_mm_sqrt_ss(lenSqA));
    const float lenB = _mm_cvtss_f32(_mm_sqrt_ss(lenSqB));
    const float extentA = motionA->m_objectRadius * lenA;
    const float extentB = motionB->m_objectRadius * lenB;
    *extentOut = extentA + extentB;
    const __m128 d = _mm_mul_ps(normal->m_quad, vector->m_quad);
    *dotOut = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(d, d, 0xAA), _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d)));
    return extentOut;
}

// Each mass centre is interpolated with its time fraction, stored as a float; the dot product is summed as (z + (y +
// x)).
float calcPointRelativeVelocity(
    const hkVector4* angularVelocities,
    const hkMotionState* motionA,
    const hkVector4* normal,
    const hkVector4* linearVelocity,
    float time,
    const hkMotionState* motionB,
    const hkVector4* point
) {
    const float tA = (time - XFNV_M128_F32(motionA->m_sweptTransform.m_centerOfMass0.m_quad, 3)) *
        XFNV_M128_F32(motionA->m_sweptTransform.m_centerOfMass1.m_quad, 3);
    const __m128 bcA = _mm_shuffle_ps(_mm_set_ss(tA), _mm_set_ss(tA), 0);
    const __m128 centerA = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, bcA), motionA->m_sweptTransform.m_centerOfMass0.m_quad),
        _mm_mul_ps(motionA->m_sweptTransform.m_centerOfMass1.m_quad, bcA)
    );
    const float tB = (time - XFNV_M128_F32(motionB->m_sweptTransform.m_centerOfMass0.m_quad, 3)) *
        XFNV_M128_F32(motionB->m_sweptTransform.m_centerOfMass1.m_quad, 3);
    const __m128 rA = _mm_sub_ps(point->m_quad, centerA);
    const __m128 bcB = _mm_shuffle_ps(_mm_set_ss(tB), _mm_set_ss(tB), 0);
    const __m128 centerB = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, bcB), motionB->m_sweptTransform.m_centerOfMass0.m_quad),
        _mm_mul_ps(motionB->m_sweptTransform.m_centerOfMass1.m_quad, bcB)
    );
    const __m128 rB = _mm_sub_ps(point->m_quad, centerB);
    const __m128 angA = angularVelocities[0].m_quad;
    const __m128 crossA = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(rA, rA, 0xD2), _mm_shuffle_ps(angA, angA, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(rA, rA, 0xC9), _mm_shuffle_ps(angA, angA, 0xD2))
    );
    const __m128 velA = _mm_add_ps(crossA, _mm_xor_ps(linearVelocity->m_quad, hkQuadRealNegateMask));
    const __m128 angB = angularVelocities[1].m_quad;
    const __m128 crossB = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(rB, rB, 0xD2), _mm_shuffle_ps(angB, angB, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(rB, rB, 0xC9), _mm_shuffle_ps(angB, angB, 0xD2))
    );
    const __m128 v = _mm_mul_ps(_mm_sub_ps(velA, crossB), normal->m_quad);
    const float result = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(v, v, 0xAA), _mm_add_ss(_mm_shuffle_ps(v, v, 0x55), v))
    );
    return result;
}
