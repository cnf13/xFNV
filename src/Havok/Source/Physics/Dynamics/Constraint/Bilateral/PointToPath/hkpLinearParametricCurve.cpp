#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpLinearParametricCurve.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpParametricCurve.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The reflected members of hkpLinearParametricCurve.
using hkpLinearParametricCurveClass_MembersArray = const hkClassMember[5];
static hkpLinearParametricCurveClass_MembersArray hkpLinearParametricCurveClass_Members{
    {"smoothingFactor", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
    {"closedLoop", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
    {"dirNotParallelToTangentAlongWholePath",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10,
     nullptr},
    {"points", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_VECTOR4}, 0, {0}, 0x20, nullptr},
    {"distance", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, {0}, 0x2C, nullptr}
};

hkClass hkpLinearParametricCurveClass(
    "hkpLinearParametricCurve",
    &hkpParametricCurveClass,
    64,
    nullptr,
    0,
    nullptr,
    0,
    hkpLinearParametricCurveClass_Members,
    5,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpLinearParametricCurveTypeInfo = {
    "hkpLinearParametricCurve",
    finishLoadedObjecthkpLinearParametricCurve,
    cleanupLoadedObjecthkpLinearParametricCurve,
    getVtablehkpLinearParametricCurve(),
    &typeid(hkpLinearParametricCurve),
    sizeof(hkpLinearParametricCurve)
};

// The SDK's constructor: a smoothing factor of 0.01, an open loop, two empty arrays and the direction (0, 1, 0, 0).
hkpLinearParametricCurve::hkpLinearParametricCurve() : m_smoothingFactor(0.01f) {
    m_closedLoop.m_bool = 0;
    m_dirNotParallelToTangentAlongWholePath.set(0.0f, 1.0f, 0.0f, 0.0f);
}

// The points and distances go with their members.
hkpLinearParametricCurve::~hkpLinearParametricCurve() {}

void hkpLinearParametricCurve::getPoint(float t, hkVector4& pointOnCurve) const {
    int32_t v5;
    uint32_t v10;
    double v4 = t;
    if (!(t < 0.0f)) {
        v5 = hkMath::hkFloatToInt(t);
        v4 = t;
    }
    else {
        v5 = 0;
    }
    double v6 = v4 - static_cast<double>(v5);
    int32_t v7 = v5 + 1;
    if (v7 >= m_points.m_size) {
        pointOnCurve = m_points.m_data[v5];
    }
    else {
        *reinterpret_cast<float*>(&v10) = v6;
        __m128 v9 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v10)), _mm_set_ss(std::bit_cast<float>(v10)), 0);
        pointOnCurve.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v9), m_points.m_data[v5].m_quad),
            _mm_mul_ps(m_points.m_data[v7].m_quad, v9)
        );
    }
}

// From the segment holding t, walks back while the projection's fraction is below zero (stopping at the first segment)
// and on while the near point lies past the segment's end and the next segment's start; in a corner between two
// segments the fraction is 0.99 on this one or 0.01 on the next. The dots are computed as z + (y + x); the fraction is
// their quotient, rounded to float; the corner's directions are normalized with a reciprocal square root and one Newton
// step with 0.5f and 3.0f, zero for a zero length. The compares: fraction < 0, bp.ab <= 0 breaks (NaN goes on), bc.bp >
// 0 advances and abN.bp < -bcN.bp keeps 0.99 (NaN takes the other way). A closed loop near either end runs again from
// the other end through the virtual call; the second end test's 0.25 is a double constant.
float hkpLinearParametricCurve::getNearestPoint(float t, const hkVector4& nearPoint, hkVector4& pointOnCurve) const {
    if (t < 0.0f) {
        t = 0.0f;
    }
    int32_t ia = hkMath::hkFloatToInt(t);
    const int32_t numPoints = m_points.m_size;
    int32_t ib = ia + 1;
    if (ib >= numPoints) {
        ib = numPoints - 1;
        ia = ib - 1;
    }
    const __m128 kNear = nearPoint.m_quad;
    const hkVector4* points = m_points.m_data;
    float fraction;
    while (true) {
        hkVector4 ab;
        ab.m_quad = _mm_sub_ps(points[ib].m_quad, points[ia].m_quad);
        hkVector4 ap;
        ap.m_quad = _mm_sub_ps(kNear, points[ia].m_quad);
        const float fApAb = ap.dot3(ab);
        const float fAbAb = ab.lengthSquared3();
        fraction = fApAb / fAbAb;
        if (fraction < 0.0f) {
            if (ia == 0) {
                break;
            }
            ia--;
            ib--;
            continue;
        }
        hkVector4 bp;
        bp.m_quad = _mm_sub_ps(kNear, points[ib].m_quad);
        const float fBpAb = bp.dot3(ab);
        if (fBpAb <= 0.0f || ib + 1 >= numPoints) {
            break;
        }
        hkVector4 bc;
        bc.m_quad = _mm_sub_ps(m_points.m_data[ib + 1].m_quad, m_points.m_data[ib].m_quad);
        const float fBcBp = bc.dot3(bp);
        if (fBcBp > 0.0f) {
            ia++;
            ib++;
            continue;
        }
        hkVector4 abN = ab;
        abN.normalize3();
        hkVector4 bcN = bc;
        bcN.normalize3();
        const float fAbNBp = abN.dot3(bp);
        const float fBcNBp = bcN.dot3(bp);
        if (fAbNBp < -fBcNBp) {
            fraction = 0.99f;
        }
        else {
            ia++;
            ib++;
            fraction = 0.01f;
        }
        break;
    }
    float tOut = static_cast<double>(ia) + fraction;
    pointOnCurve.setInterpolate4(m_points.m_data[ia], m_points.m_data[ib], fraction);
    if (m_closedLoop.m_bool) {
        float end = static_cast<float>(m_points.m_size - 1);
        const hkpParametricCurve* pCurve = this;
        float tNearest;
        if (tOut < 0.25f) {
            float tOther = end - (1.0f - tOut);
            tNearest = pCurve->getNearestPoint(tOther, nearPoint, pointOnCurve);
            return tNearest;
        }
        if (end - 0.25 < tOut) {
            float tOther = 1.0f - (end - tOut);
            tNearest = pCurve->getNearestPoint(tOther, nearPoint, pointOnCurve);
            return tNearest;
        }
    }
    return tOut;
}

void hkpLinearParametricCurve::getTangent(float t, hkVector4& tangent) const {
    __m128 v15 = _mm_undefined_ps();
    double v22;
    hkVector4* v24;
    uint32_t v62;
    uint32_t v63;
    float m_smoothingFactor = this->m_smoothingFactor;
    int32_t v4 = hkMath::hkFloatToInt(t);
    int32_t m_size = m_points.m_size;
    int32_t v6 = v4 + 1;
    int32_t v57 = v4;
    int32_t v56 = v4 + 1;
    if (v4 + 1 >= m_size) {
        v6 = m_size - 1;
        v4 = m_size - 2;
    LABEL_5:;
        v56 = v6;
        v57 = v4;
        goto LABEL_6;
    }
    if (v4 < 0) {
        v4 = 0;
        v6 = 1;
        goto LABEL_5;
    }
LABEL_6:;
    int32_t v7 = v6;
    int32_t v8 = v4;
    __m128 v9 = _mm_sub_ps(m_points.m_data[v6].m_quad, m_points.m_data[v4].m_quad);
    float v58 = t - static_cast<double>(v57);
    __m128 v10 = _mm_mul_ps(v9, v9);
    double v11 = m_smoothingFactor;
    float v12 = XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 85), 0) + XFNV_M128_F32(v10, 0);
    __m128 v13 = _mm_shuffle_ps(v10, v10, 170);
    XFNV_M128_F32(v13, 0) = XFNV_M128_F32(v13, 0) + v12;
    float v14 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v13, 0))));
    XFNV_M128_F32(v15, 0) = 0.0f;
    __m128 v16 = _mm_setzero_ps();
    XFNV_M128_F32(v16, 0) = 0.5f;
    float v18 = 3.0f - ((XFNV_M128_F32(v13, 0) * v14) * v14);
    XFNV_M128_F32(v16, 0) = (0.5f * v14) * v18;
    __m128 v20 = _mm_andnot_ps(_mm_cmpeq_ss(v13, v15), v16);
    tangent.m_quad = _mm_mul_ps(_mm_shuffle_ps(v20, v20, 0), v9);
    float v61 = 1.0 / m_smoothingFactor;
    double v21 = v61;
    if (!(m_smoothingFactor > static_cast<double>(v58)) || v4 <= 0) {
        v24 = &tangent;
        v22 = 0.5;
    }
    else {
        v7 = --v6;
        v8 = v4 - 1;
        v22 = 0.5;
        __m128 v23 = _mm_sub_ps(m_points.m_data[v6].m_quad, m_points.m_data[v8].m_quad);
        v24 = &tangent;
        *reinterpret_cast<float*>(&v62) = (v11 - v58) * 0.5 * v21;
        __m128 v25 = _mm_mul_ps(v23, v23);
        float v26 = XFNV_M128_F32(_mm_shuffle_ps(v25, v25, 85), 0) + XFNV_M128_F32(v25, 0);
        __m128 v27 = _mm_shuffle_ps(v25, v25, 170);
        XFNV_M128_F32(v27, 0) = XFNV_M128_F32(v27, 0) + v26;
        float v28 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v27, 0))));
        __m128 v29 = _mm_cmpeq_ss(v27, v15);
        float v30 = 3.0f - ((XFNV_M128_F32(v27, 0) * v28) * v28);
        XFNV_M128_F32(v16, 0) = (0.5f * v28) * v30;
        __m128 v32 = _mm_andnot_ps(v29, v16);
        __m128 v33 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v62)), _mm_set_ss(std::bit_cast<float>(v62)), 0);
        __m128 v34 = _mm_add_ps(
            _mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(v32, v32, 0), v23), v33),
            _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v33), tangent.m_quad)
        );
        __m128 v35 = _mm_mul_ps(v34, v34);
        XFNV_M128_F32(v23, 0) = XFNV_M128_F32(_mm_shuffle_ps(v35, v35, 85), 0) + XFNV_M128_F32(v35, 0);
        __m128 v36 = _mm_shuffle_ps(v35, v35, 170);
        XFNV_M128_F32(v36, 0) = XFNV_M128_F32(v36, 0) + XFNV_M128_F32(v23, 0);
        float v37 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v36, 0))));
        float v39 = 3.0f - ((XFNV_M128_F32(v36, 0) * v37) * v37);
        XFNV_M128_F32(v16, 0) = (0.5f * v37) * v39;
        __m128 v41 = _mm_andnot_ps(_mm_cmpeq_ss(v36, v15), v16);
        v56 = v6;
        tangent.m_quad = _mm_mul_ps(_mm_shuffle_ps(v41, v41, 0), v34);
    }
    float v59 = static_cast<double>(v56) - t;
    if (v59 < v11 && v6 < m_points.m_size - 1) {
        __m128 v42 = _mm_sub_ps(m_points.m_data[v7 + 1].m_quad, m_points.m_data[v8 + 1].m_quad);
        __m128 v43 = _mm_mul_ps(v42, v42);
        float v44 = XFNV_M128_F32(_mm_shuffle_ps(v43, v43, 85), 0) + XFNV_M128_F32(v43, 0);
        __m128 v45 = _mm_shuffle_ps(v43, v43, 170);
        *reinterpret_cast<float*>(&v63) = v21 * (v22 * (v11 - v59));
        XFNV_M128_F32(v45, 0) = XFNV_M128_F32(v45, 0) + v44;
        float v46 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v45, 0))));
        float v47 = 3.0f - ((XFNV_M128_F32(v45, 0) * v46) * v46);
        __m128 v48 = _mm_cmpeq_ss(v45, v15);
        XFNV_M128_F32(v16, 0) = (0.5f * v46) * v47;
        __m128 v50 = _mm_andnot_ps(v48, v16);
        __m128 v51 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v63)), _mm_set_ss(std::bit_cast<float>(v63)), 0);
        __m128 v52 = _mm_add_ps(
            _mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(v50, v50, 0), v42), v51),
            _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v51), v24->m_quad)
        );
        __m128 v53 = _mm_mul_ps(v52, v52);
        XFNV_M128_F32(v42, 0) = XFNV_M128_F32(_mm_shuffle_ps(v53, v53, 85), 0) + XFNV_M128_F32(v53, 0);
        __m128 v54 = _mm_shuffle_ps(v53, v53, 170);
        XFNV_M128_F32(v54, 0) = XFNV_M128_F32(v54, 0) + XFNV_M128_F32(v42, 0);
        XFNV_M128_F32(v42, 0) = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v54, 0))));
        XFNV_M128_F32(v16, 0) = (0.5f * XFNV_M128_F32(v42, 0)) *
            (3.0f - ((XFNV_M128_F32(v54, 0) * XFNV_M128_F32(v42, 0)) * XFNV_M128_F32(v42, 0)));
        __m128 v55 = _mm_andnot_ps(_mm_cmpeq_ss(v54, v15), v16);
        v24->m_quad = _mm_mul_ps(_mm_shuffle_ps(v55, v55, 0), v52);
    }
}

float hkpLinearParametricCurve::getStart() const {
    return 0.0;
}

float hkpLinearParametricCurve::getEnd() const {
    return static_cast<float>(m_points.m_size - 1);
}

float hkpLinearParametricCurve::getLengthFromStart(float t) const {
    int32_t v5;
    double v6;
    int32_t v3 = hkMath::hkFloatToInt(t);
    int32_t v4 = m_points.m_size - 1;
    int32_t v9 = v3;
    if (v3 < v4) {
        if (v3 < 0) {
            v3 = 0;
            v9 = 0;
        }
        v5 = v3;
        v6 = m_distance.m_data[v5 + 1] - m_distance.m_data[v5];
    }
    else {
        v9 = m_points.m_size - 1;
        v5 = v4;
        v6 = m_distance.m_data[v5] - m_distance.m_data[v5 - 1];
    }
    float v10 = v6;
    return (t - static_cast<double>(v9)) * v10 + m_distance.m_data[v5];
}

// The tangent's y is compared against 0.9800000190734863: at or above it, or NaN, takes the axis branch. Else the
// binormal is dir x tangent. Near the up axis it is the tangent's two smaller components swapped with one negated; only
// a strictly greater magnitude moves the index. The result is normalized with rsqrtss and one Newton step (3.0f and
// 0.5f), giving zero for a zero length.
void hkpLinearParametricCurve::getBinormal(float t, hkVector4& up) const {
    alignas(16) hkVector4 kTangent;
    getTangent(t, kTangent);
    const float fDot = hkTransformIdentity.m_rotation.m_col1.dot3(kTangent);
    if (fDot < 0.9800000190734863) {
        up.setCross(kTangent, m_dirNotParallelToTangentAlongWholePath);
    }
    else {
        float afAbs[3];
        afAbs[0] = std::fabsf(XFNV_M128_F32(kTangent.m_quad, 0));
        afAbs[1] = std::fabsf(XFNV_M128_F32(kTangent.m_quad, 1));
        afAbs[2] = std::fabsf(XFNV_M128_F32(kTangent.m_quad, 2));
        float fMin = afAbs[0];
        int32_t iMin = 0;
        int32_t iFirst = 1;
        int32_t iSecond = 2;
        if (fMin > afAbs[1]) {
            fMin = afAbs[1];
            iFirst = 0;
            iMin = 1;
        }
        if (fMin > afAbs[2]) {
            iSecond = iMin;
        }
        // Loads the component, zeroes the whole vector, then stores it.
        const float fSecond = (&reinterpret_cast<float*>(&kTangent.m_quad)[0])[iSecond];
        _mm_store_ps(&reinterpret_cast<float*>(&up.m_quad)[0], _mm_setzero_ps());
        (&reinterpret_cast<float*>(&up.m_quad)[0])[iFirst] = fSecond;
        (&reinterpret_cast<float*>(&up.m_quad)[0])[iSecond] = -(&reinterpret_cast<float*>(&kTangent.m_quad)[0])[iFirst];
    }
    up.normalize3();
}

hkBool hkpLinearParametricCurve::isClosedLoop() const {
    return (int8_t)((uint8_t)m_closedLoop.m_bool);
}

// The SDK's pathPoints = m_points: hkArray's inline operator=, the plain-data copyFromArray.
void hkpLinearParametricCurve::getPointsToDraw(hkArray<hkVector4, hkContainerHeapAllocator>& pathPoints) const {
    pathPoints.copyFromArray(hkContainerHeapAllocator().get(&pathPoints), m_points, hkTypeIsPod());
}

void hkpLinearParametricCurve::transformPoints(const hkTransform& transformation) {
    int32_t m_size;
    int32_t v4;
    m_size = m_points.m_size;
    if (m_size > 0) {
        v4 = 0;
        do {
            (&m_points.m_data[v4])->setTransformedPos(transformation, m_points.m_data[v4]);
            ++v4;
            --m_size;
        }
        while (m_size);
    }
}

// Clones the curve: a new curve allocated from the heap's blockAlloc with the size stored in m_memSizeAndFlags, each
// point appended through addPoint (the count read once), the loop flag, the direction's x, y and z with w 0, the
// distances and the smoothing factor.
hkpParametricCurve* hkpLinearParametricCurve::clone() {
    hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
    void* pMemory = kRouter.m_heap->blockAlloc(sizeof(hkpLinearParametricCurve));
    static_cast<hkpLinearParametricCurve*>(pMemory)->m_memSizeAndFlags =
        static_cast<uint16_t>(sizeof(hkpLinearParametricCurve));
    hkpLinearParametricCurve* pNewCurve = ::new (pMemory) hkpLinearParametricCurve;
    const int32_t numPoints = m_points.m_size;
    for (int32_t i = 0; i < numPoints; i++) {
        pNewCurve->addPoint(m_points.m_data[i]);
    }
    pNewCurve->m_closedLoop.m_bool = m_closedLoop.m_bool;
    pNewCurve->m_dirNotParallelToTangentAlongWholePath.set(
        XFNV_M128_F32(m_dirNotParallelToTangentAlongWholePath.m_quad, 0),
        XFNV_M128_F32(m_dirNotParallelToTangentAlongWholePath.m_quad, 1),
        XFNV_M128_F32(m_dirNotParallelToTangentAlongWholePath.m_quad, 2), 0.0f
    );
    pNewCurve->m_distance.copyFromArray(
        hkContainerHeapAllocator().get(&pNewCurve->m_distance), m_distance, hkTypeIsPod()
    );
    pNewCurve->m_smoothingFactor = m_smoothingFactor;
    return pNewCurve;
}

// Each push takes the router before testing for a full array. The segment length rounds each lane step to float before
// the square root, and is added to the last distance before the second router call.
void hkpLinearParametricCurve::addPoint(const hkVector4& p) {
    hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
    if (m_points.m_size == (m_points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(kRouter.heap(), reinterpret_cast<hkArrayBase<void>*>(&m_points), sizeof(hkVector4));
    }
    m_points.m_data[m_points.m_size] = p;
    m_points.m_size++;
    float fDistance;
    if (m_points.m_size == 1) {
        hkMemoryRouter& kDistanceRouter = hkMemoryRouter::getInstance();
        if (m_distance.m_size == (m_distance.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(
                kDistanceRouter.heap(), reinterpret_cast<hkArrayBase<void>*>(&m_distance), sizeof(float)
            );
        }
        fDistance = 0.0f;
    }
    else {
        const hkVector4& kPrevious = m_points.m_data[m_points.m_size - 2];
        float fX = XFNV_M128_F32(p.m_quad, 0) - XFNV_M128_F32(kPrevious.m_quad, 0);
        float fY = XFNV_M128_F32(p.m_quad, 1) - XFNV_M128_F32(kPrevious.m_quad, 1);
        float fZ = XFNV_M128_F32(p.m_quad, 2) - XFNV_M128_F32(kPrevious.m_quad, 2);
        float fXX = fX * fX;
        float fYY = fY * fY;
        float fZZ = fZ * fZ;
        float fSum = fYY + fXX;
        fSum = fZZ + fSum;
        float fLength = xfnv::SqrtF(fSum);
        fDistance = fLength + m_distance.m_data[m_distance.m_size - 1];
        hkMemoryRouter& kDistanceRouter = hkMemoryRouter::getInstance();
        if (m_distance.m_size == (m_distance.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(
                kDistanceRouter.heap(), reinterpret_cast<hkArrayBase<void>*>(&m_distance), sizeof(float)
            );
        }
    }
    m_distance.m_data[m_distance.m_size] = fDistance;
    m_distance.m_size++;
}

void hkpLinearParametricCurve::setClosedLoop(hkBool closeLoop) {
    m_closedLoop = closeLoop;
}

float hkpLinearParametricCurve::getSmoothingFactor() {
    return m_smoothingFactor;
}

void hkpLinearParametricCurve::setSmoothingFactor(float smooth) {
    m_smoothingFactor = smooth;
}

const hkClass& hkpLinearParametricCurve::staticClass() {
    return hkpLinearParametricCurveClass;
}
