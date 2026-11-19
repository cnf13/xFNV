#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/hkpSphereUtil.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

static int32_t
calcClosestPointsLineLine(const __m128* a1, const __m128* a2, const __m128* a3, const __m128* a4, __m128* a5);
static void finishLoadedObjecthkpCapsuleShape(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
static const void* getVtablehkpCapsuleShape();
static void cleanupLoadedObjecthkpCapsuleShape(void* p);

// The timer tag HK_TIMER_END stores into the monitor stream: one literal the timer macros of several files share,
// defined here.
using TimerName3 = const char[3];
static TimerName3 aEt{"Et"};

// The enum's items and the enum.
using hkpCapsuleShapeRayHitTypeEnumItemArray = const hkClassEnum::Item[3];
static hkpCapsuleShapeRayHitTypeEnumItemArray hkpCapsuleShapeRayHitTypeEnumItems{
    {hkpCapsuleShape::HIT_CAP0, "HIT_CAP0"},
    {hkpCapsuleShape::HIT_CAP1, "HIT_CAP1"},
    {hkpCapsuleShape::HIT_BODY, "HIT_BODY"}
};
using hkpCapsuleShapeEnumArray = const hkClassEnum[1];
static hkpCapsuleShapeEnumArray hkpCapsuleShapeEnums{{"RayHitType", hkpCapsuleShapeRayHitTypeEnumItems, 3, nullptr, 0}};

// The class record's member table.
using hkpCapsuleShapeClassMemberArray = const hkClassMember[2];
static hkpCapsuleShapeClassMemberArray hkpCapsuleShapeClass_Members{
    {"vertexA", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x20, nullptr},
    {"vertexB", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x30, nullptr}
};
hkClass hkpCapsuleShapeClass(
    "hkpCapsuleShape",
    &hkpConvexShapeClass,
    0x40,
    nullptr,
    0,
    hkpCapsuleShapeEnums,
    1,
    hkpCapsuleShapeClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpCapsuleShapeTypeInfo{
    "hkpCapsuleShape",          finishLoadedObjecthkpCapsuleShape, cleanupLoadedObjecthkpCapsuleShape,
    getVtablehkpCapsuleShape(), &typeid(hkpCapsuleShape),          sizeof(hkpCapsuleShape)
};

static int32_t
calcClosestPointsLineLine(const __m128* a1, const __m128* a2, const __m128* a3, const __m128* a4, __m128* a5) {
    int32_t result;
    double v22;
    double v23;
    uint32_t v29;
    alignas(16) uint32_t v30;
    __m128 v5 = *a4;
    __m128 v6 = *a2;
    __m128 v7 = _mm_mul_ps(*a4, *a2);
    float v31 = XFNV_M128_F32(_mm_shuffle_ps(v7, v7, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v7, v7, 85), 0) + XFNV_M128_F32(v7, 0));
    __m128 v8 = _mm_sub_ps(*a3, *a1);
    __m128 v9 = _mm_mul_ps(*a2, v8);
    float v10 = XFNV_M128_F32(_mm_shuffle_ps(v9, v9, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v9, v9, 85), 0) + XFNV_M128_F32(v9, 0));
    __m128 v11 = _mm_mul_ps(*a4, v8);
    float v37 = XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 85), 0) + XFNV_M128_F32(v11, 0));
    __m128 v12 = _mm_mul_ps(v6, v6);
    float v28 = XFNV_M128_F32(_mm_shuffle_ps(v12, v12, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v12, v12, 85), 0) + XFNV_M128_F32(v12, 0));
    __m128 v13 = _mm_mul_ps(v5, v5);
    double v14 =
        (XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 85), 0) + XFNV_M128_F32(v13, 0)));
    double v15 = v14 * v28;
    double v16 = v31;
    double v17 = v31 * v31;
    float v32 = v15 - v17;
    double v18 = v32;
    float v33 = std::fabs((double)v32);
    double v20 = v33;
    float v34 = v15;
    float v35 = std::fabs((double)v34);
    float v36 = (v17 + v35) * 9.5367431640625e-07;
    if (!(v36 < v20)) {
        v23 = v37;
        result = 1;
        v22 = 0.0;
    }
    else {
        result = 0;
        v22 = (v14 * v10 - v37 * v16) / v18;
        v23 = v37;
    }
    *reinterpret_cast<float*>(&v29) = v22;
    __m128 v24 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v29)), _mm_set_ss(std::bit_cast<float>(v29)), 0), v6),
        *a1
    );
    *reinterpret_cast<float*>(&v30) = (v16 * *reinterpret_cast<float*>(&v29) - v23) / v14;
    __m128 v25 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v30)), _mm_set_ss(std::bit_cast<float>(v30)), 0), v5),
        *a3
    );
    __m128 v26 = _mm_sub_ps(v24, v25);
    __m128 v27 = _mm_mul_ps(v26, v26);
    XFNV_M128_F32((*a5), 0) = XFNV_M128_F32(_mm_shuffle_ps(v27, v27, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v27, v27, 85), 0) + XFNV_M128_F32(v27, 0));
    a5[1] = v24;
    a5[2] = v25;
    XFNV_M128_F32((*a5), 1) = *reinterpret_cast<float*>(&v29);
    XFNV_M128_F32((*a5), 2) = *reinterpret_cast<float*>(&v30);
    return result;
}

// Runs the finish constructor hkpCapsuleShape(flag) in place.
static void finishLoadedObjecthkpCapsuleShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpCapsuleShape(kFlag);
    }
}

static const void* getVtablehkpCapsuleShape() {
    return hkGetVtableOf<hkpCapsuleShape>();
}

static void cleanupLoadedObjecthkpCapsuleShape(void* p) {
    static_cast<hkpCapsuleShape*>(p)->~hkpCapsuleShape();
}

// calcClosestPointsLineLine fills one result: three floats and a pad, then two vectors. They are copied out in that
// order.
float* calcClosestLineLine(
    const __m128* a1,
    const __m128* a2,
    const __m128* a3,
    const __m128* a4,
    float* a5,
    float* a6,
    float* a7,
    __m128* a8,
    __m128* a9
) {
    __m128 aResult[3];
    calcClosestPointsLineLine(a1, a2, a3, a4, aResult);
    double v9 = XFNV_M128_F32(aResult[0], 1);
    *a8 = aResult[1];
    *a9 = aResult[2];
    *a6 = v9;
    *a7 = XFNV_M128_F32(aResult[0], 2);
    *a5 = XFNV_M128_F32(aResult[0], 0);
    return a5;
}

// Each end point's w holds the radius.
hkpCapsuleShape::hkpCapsuleShape(const hkVector4& arVertexA, const hkVector4& arVertexB, float afRadius)
  : hkpConvexShape(HK_SHAPE_CAPSULE, afRadius), m_vertexA(arVertexA), m_vertexB(arVertexB) {
    XFNV_M128_F32(m_vertexA.m_quad, 3) = afRadius;
    XFNV_M128_F32(m_vertexB.m_quad, 3) = afRadius;
}

hkpCapsuleShape::~hkpCapsuleShape() {}

// The base's statistics, always for the hkpConvexShapeClass record whatever cls is passed.
void hkpCapsuleShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    hkReferencedObject::calcContentStatistics(collector, &hkpConvexShapeClass);
}

int32_t hkpCapsuleShape::calcSizeForSpu(const CalcSizeForSpuInput& a1, int32_t a2) const {
    return 64;
}

// Both end points are transformed, and their min and max are widened by the radius plus the tolerance.
void hkpCapsuleShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    float fRadius = m_radius + tolerance;
    const __m128 kTol4 = _mm_shuffle_ps(_mm_set_ss(fRadius), _mm_set_ss(fRadius), 0);
    hkVector4 kObj[2];
    hkVector4Util::transformPoints(localToWorld, &m_vertexA, 2, kObj);
    const __m128 kMin = _mm_min_ps(kObj[0].m_quad, kObj[1].m_quad);
    out.m_max.m_quad = _mm_max_ps(kObj[0].m_quad, kObj[1].m_quad);
    out.m_min.m_quad = _mm_sub_ps(kMin, kTol4);
    out.m_max.m_quad = _mm_add_ps(out.m_max.m_quad, kTol4);
}

// The ray against the capsule. No hit when the ray starts inside it (closestPointLineSeg). Otherwise the ray is cast
// against the infinite cylinder around the segment: calcClosestLineLine gives the squared distance of the ray's and the
// segment's lines and the ray fraction of their closest approach, from which the hit steps back by the chord half
// length over the ray's direction across the axis (-1.0f for a ray parallel to it). A hit nearer than the output's
// fraction lying within the segment is a body hit (HIT_BODY); any other is cast against the sphere of the end cap on
// its side (castRayUtil, HIT_CAP0 or HIT_CAP1), unless the hit lies behind the start and the start is outside the
// cylinder. The dot products sum y + x, then z; the normalisations are a reciprocal square root with one Newton step,
// zero for a zero length. The timer macros write into the stream's raw buffer (hence the casts).
hkBool hkpCapsuleShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    HK_TIMER_BEGIN("rcCapsule", nullptr);

    // hkVector4::dot3 on two quads, the sum in lane 0.
    auto dot3 = [](__m128 a, __m128 b) {
        hkVector4 kA;
        kA.m_quad = a;
        hkVector4 kB;
        kB.m_quad = b;
        return kA.dot3(kB).getQuad();
    };
    // EXE-INLINED none: the inverse length of hkVector4::normalize3, in all four lanes
    auto invLength = [](__m128 aLengthSq) {
        const __m128 kEstimate = _mm_rsqrt_ss(aLengthSq);
        const __m128 kIsZero = _mm_cmpeq_ss(aLengthSq, _mm_set_ss(0.0f));
        const __m128 kInverse = _mm_mul_ss(
            _mm_mul_ss(_mm_set_ss(0.5f), kEstimate),
            _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(aLengthSq, kEstimate), kEstimate))
        );
        const __m128 kMasked = _mm_andnot_ps(kIsZero, kInverse);
        return _mm_shuffle_ps(kMasked, kMasked, 0);
    };

    hkBool result;
    result.m_bool = [&] -> char {
        alignas(16) hkVector4 closest;
        closestPointLineSeg(input.m_from, m_vertexA, m_vertexB, closest);
        const __m128 kStartOffset = _mm_sub_ps(input.m_from.m_quad, closest.m_quad);
        const float fStartDistSq = _mm_cvtss_f32(dot3(kStartOffset, kStartOffset));
        const float fRadiusSq = m_radius * m_radius;
        if (fRadiusSq > fStartDistSq) {
            return 0;
        }

        alignas(16) hkVector4 rayDir;
        rayDir.m_quad = _mm_sub_ps(input.m_to.m_quad, input.m_from.m_quad);
        alignas(16) hkVector4 segDir;
        segDir.m_quad = _mm_sub_ps(m_vertexB.m_quad, m_vertexA.m_quad);
        // Stores the initial value before the call overwrites it.
        float fLineDistSq;
        fLineDistSq = 3.40282e+38f;
        float fRayFraction;
        alignas(16) float fSegFraction;
        alignas(16) hkVector4 closestOnRay;
        // The callee only reads the ray's start and the segment's start.
        calcClosestLineLine(
            const_cast<__m128*>(&input.m_from.m_quad), &rayDir.m_quad, const_cast<__m128*>(&m_vertexA.m_quad),
            &segDir.m_quad, &fLineDistSq, &fRayFraction, &fSegFraction, &closestOnRay.m_quad, &closest.m_quad
        );
        const float fCylinderRadiusSq = m_radius * m_radius;
        if (fCylinderRadiusSq < fLineDistSq) {
            return 0;
        }

        // The segment's axis and length, both zero for a segment no longer than epsilon.
        const __m128 kSegLengthSq = dot3(segDir.m_quad, segDir.m_quad);
        __m128 kAxis;
        float fSegLength;
        if (1.1920929e-07f < _mm_cvtss_f32(kSegLengthSq)) {
            const __m128 kInvLength = invLength(kSegLengthSq);
            kAxis = _mm_mul_ps(kInvLength, segDir.m_quad);
            fSegLength = _mm_cvtss_f32(_mm_mul_ss(kSegLengthSq, kInvLength));
        }
        else {
            kAxis = _mm_setzero_ps();
            fSegLength = 0.0f;
        }

        // The ray's direction across the axis.
        const __m128 kRayDir = rayDir.m_quad;
        const __m128 kAlong = _mm_sub_ss(_mm_setzero_ps(), dot3(kRayDir, kAxis));
        const __m128 kAcross = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kAlong, kAlong, 0), kAxis), kRayDir);
        const float fAcrossSq = _mm_cvtss_f32(dot3(kAcross, kAcross));
        float fHitFraction;
        if (fAcrossSq == 0.0f) {
            fHitFraction = -1.0f;
        }
        else {
            // The radius squared stays unrounded on the x87 stack; the quotient and the root are rounded to float.
            const float fChordSq = (m_radius * m_radius - fLineDistSq) / fAcrossSq;
            const float fChord = std::sqrt(fChordSq);
            fHitFraction = fRayFraction - fChord;
        }
        if (results.m_hitFraction <= fHitFraction) {
            return 0;
        }

        // The hit point's distance along the axis from vertex A.
        const __m128 kOne = _mm_set1_ps(1.0f);
        const __m128 kFrom = input.m_from.m_quad;
        const __m128 kTo = input.m_to.m_quad;
        const float fAxisA = _mm_cvtss_f32(dot3(m_vertexA.m_quad, kAxis));
        const __m128 kHitFraction = _mm_set1_ps(fHitFraction);
        const __m128 kHit =
            _mm_add_ps(_mm_mul_ps(_mm_sub_ps(kOne, kHitFraction), kFrom), _mm_mul_ps(kHitFraction, kTo));
        const float fHitAlong = _mm_cvtss_f32(dot3(kHit, kAxis)) - fAxisA;
        if (fHitFraction >= 0.0f && fHitAlong > 0.0f && fHitAlong < fSegLength) {
            const float fSegHit = fHitAlong / fSegLength;
            const __m128 kSegHit = _mm_set1_ps(fSegHit);
            results.m_hitFraction = fHitFraction;
            const __m128 kOnAxis = _mm_add_ps(
                _mm_mul_ps(_mm_sub_ps(kOne, kSegHit), m_vertexA.m_quad), _mm_mul_ps(m_vertexB.m_quad, kSegHit)
            );
            const __m128 kNormal = _mm_sub_ps(kHit, kOnAxis);
            results.m_normal.m_quad = _mm_mul_ps(invLength(dot3(kNormal, kNormal)), kNormal);
            results.m_extraInfo = HIT_BODY;
            results.m_shapeKeys[results.m_shapeKeyIndex] = 0xFFFFFFFF;
            return 1;
        }

        // A hit behind the start reaches a cap only from a start within the cylinder: the start's distance from the
        // axis point it projects to.
        const float fFromAlong = _mm_cvtss_f32(dot3(kAxis, kFrom)) - fAxisA;
        const float fSegFrom = fFromAlong / fSegLength;
        const __m128 kSegFrom = _mm_set1_ps(fSegFrom);
        const __m128 kFromOnAxis = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(kOne, kSegFrom), m_vertexA.m_quad), _mm_mul_ps(m_vertexB.m_quad, kSegFrom)
        );
        if (fHitFraction < 0.0f) {
            const __m128 kFromOffset = _mm_sub_ps(kFrom, kFromOnAxis);
            const float fFromDistSq = _mm_cvtss_f32(dot3(kFromOffset, kFromOffset));
            const float fCapRadiusSq = m_radius * m_radius;
            if (fCapRadiusSq < fFromDistSq) {
                return 0;
            }
        }

        // The cap on the hit's side: vertex A's at or before it along the axis (NaN picks B).
        const int32_t iCap = fHitAlong <= 0.0f ? HIT_CAP0 : HIT_CAP1;
        const __m128 kCenter = (iCap != HIT_CAP0 ? m_vertexB : m_vertexA).m_quad;
        // EXE-INLINED 0x632CA0
        alignas(16) hkpShapeRayCastInput capInput;
        capInput.m_filterInfo = 0;
        capInput.m_rayShapeCollectionFilter = nullptr;
        capInput.m_from.m_quad = _mm_sub_ps(kFrom, kCenter);
        capInput.m_to.m_quad = _mm_sub_ps(kTo, kCenter);
        if (!hkpSphereUtil::castRayUtil(m_radius, capInput, results)) {
            return 0;
        }
        results.m_extraInfo = iCap;
        return 1;
    }();

    HK_TIMER_END();
    return result;
}

int32_t hkpCapsuleShape::getNumCollisionSpheresImpl() const {
    return 2;
}

const hkSphere* hkpCapsuleShape::getCollisionSpheresImpl(hkSphere* sphereBuffer) const {
    *sphereBuffer = static_cast<hkSphere>(m_vertexA);
    sphereBuffer[1] = static_cast<hkSphere>(m_vertexB);
    return sphereBuffer;
}

void hkpCapsuleShape::getFirstVertex(hkVector4& v) const {
    v = m_vertexB;
}

void hkpCapsuleShape::getSupportingVertexImpl(const hkVector4& direction, hkpCdVertex& supportingVertexOut) const {
    hkVector4 v3;
    hkVector4 v4;
    v3.m_quad = m_vertexB.m_quad;
    v4.m_quad = m_vertexA.m_quad;
    __m128 v5 = _mm_mul_ps(_mm_sub_ps(v3.m_quad, v4.m_quad), direction.m_quad);
    if (!((XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 170), 0) +
           (XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 85), 0) + XFNV_M128_F32(v5, 0))) < 0.0f)) {
        (*static_cast<hkVector4*>(&supportingVertexOut)).m_quad = v3.m_quad;
        XFNV_M128_F32(supportingVertexOut.m_quad, 3) = std::bit_cast<float>(static_cast<int32_t>(0x3F000010));
    }
    else {
        (*static_cast<hkVector4*>(&supportingVertexOut)).m_quad = v4.m_quad;
        XFNV_M128_F32(supportingVertexOut.m_quad, 3) = std::bit_cast<float>(static_cast<int32_t>(0x3F000000));
    }
}

// A vertex id is the vertex's byte offset from m_vertexA (0 or 16); it goes into the w of the vertex out.
void hkpCapsuleShape::convertVertexIdsToVerticesImpl(
    const uint16_t* ids,
    int32_t numIds,
    hkpCdVertex* verticesOut
) const {
    for (int32_t i = numIds - 1; i >= 0; --i) {
        int32_t v7 = *ids;
        // The id is a byte offset from m_vertexA.
        const hkVector4* pVertex = hkAddByteOffsetConst(&m_vertexA, v7);
        verticesOut->m_quad = pVertex->m_quad;
        XFNV_M128_F32(verticesOut->m_quad, 3) = std::bit_cast<float>(static_cast<int32_t>((v7 | 0x3F000000)));
        ++verticesOut;
        ++ids;
    }
}

void hkpCapsuleShape::getCentreImpl(hkVector4& centreOut) const {
    hkVector4 v2;
    v2.m_quad = _mm_add_ps(m_vertexB.m_quad, m_vertexA.m_quad);
    centreOut.m_quad = v2.m_quad;
    centreOut.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(f0_5_), _mm_set_ss(f0_5_), 0), v2.m_quad);
}

void hkpCapsuleShape::closestPointLineSeg(const hkVector4& A, const hkVector4& B, const hkVector4& B2, hkVector4& a4) {
    hkVector4 v4;
    hkVector4 v5;
    uint32_t v11;
    v4.m_quad = B.m_quad;
    v5.m_quad = B2.m_quad;
    __m128 v6 = _mm_sub_ps(B2.m_quad, B.m_quad);
    __m128 v7 = _mm_mul_ps(_mm_sub_ps(B.m_quad, A.m_quad), v6);
    float v10 =
        -(XFNV_M128_F32(_mm_shuffle_ps(v7, v7, 170), 0) +
          (XFNV_M128_F32(_mm_shuffle_ps(v7, v7, 85), 0) + XFNV_M128_F32(v7, 0)));
    __m128 v8 = _mm_mul_ps(v6, v6);
    float v12 = XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 85), 0) + XFNV_M128_F32(v8, 0));
    double v9 = v10;
    if (!(v10 <= 0.0f)) {
        if (!(v12 <= v9)) {
            *reinterpret_cast<float*>(&v11) = v9 / v12;
            v5.m_quad = _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v11)), _mm_set_ss(std::bit_cast<float>(v11)), 0), v6
                ),
                v4.m_quad
            );
        }
        a4.m_quad = v5.m_quad;
    }
    else {
        a4.m_quad = v4.m_quad;
    }
}

const hkClass& hkpCapsuleShape::staticClass() {
    return hkpCapsuleShapeClass;
}
