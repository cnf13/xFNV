#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbCastUtils.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayBundleCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppRayBundleVirtualMachine.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpTriangleShape(void* p);

static hkQuadReal data_11B3B70{65536.0f, 65536.0f, 65536.0f, 65536.0f};
static hkQuadReal hkpTriangleShape_s_quadReal65536{65536.0f, 65536.0f, 65536.0f, 65536.0f};

// The weldingType member's enum pointer is set directly in this initialiser.
using hkpTriangleShapeClass_MembersArray = hkClassMember[7];
static hkpTriangleShapeClass_MembersArray hkpTriangleShapeClass_Members{
    {"weldingInfo", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
    {"weldingType",
     nullptr,
     &hkpWeldingUtilityEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0x16,
     nullptr},
    {"isExtruded", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x17, nullptr},
    {"vertexA", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr},
    {"vertexB", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30, nullptr},
    {"vertexC", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x40, nullptr},
    {"extrusion", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x50, nullptr}
};
static hkQuadReal data_11BD390{65536.0f, 65536.0f, 65536.0f, 65536.0f};

static void cleanupLoadedObjecthkpTriangleShape(void* p) {
    static_cast<hkpTriangleShape*>(p)->~hkpTriangleShape();
}

namespace {
    // The SDK's default struct of hkpTriangleShape: each member's offset into it (-1: no default), then the one
    // default, weldingType.
    struct hkpTriangleShape_DefaultStruct {
        int32_t s_defaultOffsets[7];
        hkEnum<hkpWeldingUtility::WeldingType, uint8_t> m_weldingType;
    };
} // namespace
static const hkpTriangleShape_DefaultStruct hkpTriangleShape_Default{
    {-1, 0x1C, -1, -1, -1, -1, -1},
    {hkpWeldingUtility::WELDING_TYPE_NONE}
};

hkClass hkpTriangleShapeClass(
    "hkpTriangleShape",
    &hkpConvexShapeClass,
    0x60,
    nullptr,
    0,
    nullptr,
    0,
    hkpTriangleShapeClass_Members,
    7,
    &hkpTriangleShape_Default,
    nullptr,
    0,
    0
);
hkTypeInfo hkpTriangleShapeTypeInfo = {
    "hkpTriangleShape",          finishLoadedObjecthkpTriangleShape, cleanupLoadedObjecthkpTriangleShape,
    getVtablehkpTriangleShape(), &typeid(hkpTriangleShape),          sizeof(hkpTriangleShape)
};

// Returns the edge's five welding bits. The shift uses the low five bits of edgeIndex * 5.
int32_t getSingleEdgeBitcode(uint16_t triangleEdgesBitcode, int32_t edgeIndex) {
    const uint32_t shift = (static_cast<uint32_t>(edgeIndex) * 5u) & 31u;
    return static_cast<int32_t>((static_cast<uint32_t>(triangleEdgesBitcode) >> shift) & 0x1Fu);
}

__m128* calcTriangleNormal(const __m128* a1, const __m128* a2, __m128* a3, const hkVector4* r_eax) {
    __m128 v11 = _mm_undefined_ps();
    __m128 v4 = _mm_sub_ps(r_eax->m_quad, *a1);
    __m128 v5 = _mm_sub_ps(*a2, r_eax->m_quad);
    __m128 v7 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v4, v4, 201), _mm_shuffle_ps(v5, v5, 210)),
        _mm_mul_ps(_mm_shuffle_ps(v4, v4, 210), _mm_shuffle_ps(v5, v5, 201))
    );
    __m128 v8 = _mm_mul_ps(v7, v7);
    XFNV_M128_F32(v4, 0) = XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 85), 0) + XFNV_M128_F32(v8, 0);
    __m128 v9 = _mm_shuffle_ps(v8, v8, 170);
    XFNV_M128_F32(v9, 0) = XFNV_M128_F32(v9, 0) + XFNV_M128_F32(v4, 0);
    float v10 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v9, 0))));
    XFNV_M128_F32(v11, 0) = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v12 = _mm_cmpeq_ss(v9, v11);
    XFNV_M128_F32(v11, 0) = 3.0f - ((XFNV_M128_F32(v9, 0) * v10) * v10);
    __m128 v13 = _mm_setzero_ps();
    XFNV_M128_F32(v13, 0) = (0.5f * v10) * XFNV_M128_F32(v11, 0);
    __m128 v14 = _mm_andnot_ps(v12, v13);
    *a3 = _mm_mul_ps(_mm_shuffle_ps(v14, v14, 0), v7);
    return a3;
}

const hkVector4*
calcEdgeSides(const hkVector4* a1, const hkVector4* a2, const hkVector4* a3, hkVector4* a4, const hkVector4* r_eax) {
    const __m128 pointsX = r_eax[0].m_quad;
    const __m128 pointsY = r_eax[1].m_quad;
    const __m128 pointsZ = r_eax[2].m_quad;
    const __m128 v1 = a1->m_quad;
    const __m128 v2 = a2->m_quad;
    // a1 - p and a2 - p, transposed: one lane per hit point.
    const __m128 d1x = _mm_sub_ps(_mm_shuffle_ps(v1, v1, 0x00), pointsX);
    const __m128 d1y = _mm_sub_ps(_mm_shuffle_ps(v1, v1, 0x55), pointsY);
    const __m128 d1z = _mm_sub_ps(_mm_shuffle_ps(v1, v1, 0xAA), pointsZ);
    const __m128 d2x = _mm_sub_ps(_mm_shuffle_ps(v2, v2, 0x00), pointsX);
    const __m128 d2y = _mm_sub_ps(_mm_shuffle_ps(v2, v2, 0x55), pointsY);
    const __m128 d2z = _mm_sub_ps(_mm_shuffle_ps(v2, v2, 0xAA), pointsZ);
    const __m128 crossX = _mm_sub_ps(_mm_mul_ps(d2z, d1y), _mm_mul_ps(d1z, d2y));
    const __m128 crossY = _mm_sub_ps(_mm_mul_ps(d1z, d2x), _mm_mul_ps(d2z, d1x));
    const __m128 crossZ = _mm_sub_ps(_mm_mul_ps(d2y, d1x), _mm_mul_ps(d1y, d2x));
    const __m128 normal = a3->m_quad;
    const __m128 sumX = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normal, normal, 0x00), crossX), _mm_setzero_ps());
    const __m128 sumY = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normal, normal, 0x55), crossY), sumX);
    a4->m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normal, normal, 0xAA), crossZ), sumY);
    return a3;
}

void hkpTriangleShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    hkReferencedObject::calcContentStatistics(collector, &hkpConvexShapeClass);
}

// The shape's size on the SPU is 0x60, whatever the buffer has left.
int32_t hkpTriangleShape::calcSizeForSpu(const CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const {
    return 0x60;
}

void hkpTriangleShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    hkVector4 v7;
    hkVector4 v8;
    uint32_t v10;
    hkVector4 v11;
    hkVector4 v12;
    hkVector4 v13;
    *reinterpret_cast<float*>(&v10) = m_radius + tolerance;
    __m128 v14 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v10)), _mm_set_ss(std::bit_cast<float>(v10)), 0);
    v11.setTransformedPos(localToWorld, m_vertexA);
    v12.setTransformedPos(localToWorld, m_vertexB);
    v13.setTransformedPos(localToWorld, m_vertexC);
    __m128 m_quad = v13.m_quad;
    __m128 v6 = _mm_min_ps(v11.m_quad, v12.m_quad);
    v7.m_quad = _mm_max_ps(_mm_max_ps(v11.m_quad, v12.m_quad), v13.m_quad);
    out.m_max.m_quad = v7.m_quad;
    v8.m_quad = _mm_min_ps(v6, m_quad);
    out.m_min.m_quad = v8.m_quad;
    out.m_min.m_quad = _mm_min_ps(v8.m_quad, _mm_add_ps(m_extrusion.m_quad, v8.m_quad));
    out.m_max.m_quad = _mm_max_ps(v7.m_quad, _mm_add_ps(m_extrusion.m_quad, v7.m_quad));
    out.m_min.m_quad = _mm_sub_ps(out.m_min.m_quad, v14);
    out.m_max.m_quad = _mm_add_ps(out.m_max.m_quad, v14);
}

// The SDK's castRayImpl: the ray against the triangle's plane, then the hit point against the three edges, with a
// tolerance scaled by the normal's squared length. A hit nearer than output's hit fraction is stored, with the
// normal turned against the ray.
hkBool hkpTriangleShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    hkBool result;
    HK_TIMER_BEGIN("rcTriangle", nullptr);
    hkVector4 edge0;
    edge0.setSub4(m_vertexB, m_vertexA);
    hkVector4 edge1;
    edge1.setSub4(m_vertexC, m_vertexA);
    hkVector4 normal;
    normal.setCross(edge0, edge1);
    const float planeDist = m_vertexA.dot3(normal);
    const float fromDot = input.m_from.dot3(normal);
    const float fromDist = fromDot - planeDist;
    const float toDot = input.m_to.dot3(normal);
    const float toDist = toDot - planeDist;
    // Both ends on one side misses, unless only the start lies in the plane. The product is compared unrounded; the
    // double product of two floats is exact.
    if (static_cast<double>(toDist) * fromDist >= 0.0 && (fromDist != 0.0f || toDist == 0.0f)) {
        HK_TIMER_END();
        result.m_bool = 0;
        return result;
    }
    const float denominator = fromDist - toDist;
    const float hitFraction = fromDist / denominator;
    if (!(results.m_hitFraction > hitFraction)) {
        HK_TIMER_END();
        result.m_bool = 0;
        return result;
    }
    const float normalLength2 = normal.dot3(normal);
    // EXE-INLINED 0x5DBF70
    const __m128 fraction = _mm_set1_ps(hitFraction);
    hkVector4 hitPoint;
    hitPoint.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, fraction), input.m_from.m_quad), _mm_mul_ps(input.m_to.m_quad, fraction)
    );
    const float tolerance = static_cast<float>(normalLength2 * -9.999999747378752e-05);
    hkVector4 toA;
    toA.setSub4(m_vertexA, hitPoint);
    hkVector4 toB;
    toB.setSub4(m_vertexB, hitPoint);
    hkVector4 toC;
    toC.setSub4(m_vertexC, hitPoint);
    // Each edge's side: the triple product of the normal with the vectors to the edge's ends.
    hkVector4 edgeCross;
    edgeCross.setCross(toA, toB);
    const float side0 = edgeCross.dot3(normal);
    if (side0 >= tolerance) {
        edgeCross.setCross(toB, toC);
        const float side1 = edgeCross.dot3(normal);
        if (side1 >= tolerance) {
            edgeCross.setCross(toC, toA);
            const float side2 = edgeCross.dot3(normal);
            if (side2 >= tolerance) {
                results.m_hitFraction = hitFraction;
                if (!(fromDist > 0.0f)) {
                    normal.m_quad = _mm_xor_ps(normal.m_quad, hkQuadRealNegateMask);
                }
                results.m_normal = normal;
                results.m_normal.normalize3();
                results.m_shapeKeys[results.m_shapeKeyIndex] = HK_INVALID_SHAPE_KEY;
                HK_TIMER_END();
                result.m_bool = 1;
                return result;
            }
        }
    }
    HK_TIMER_END();
    result.m_bool = 0;
    return result;
}

// The SDK's castRayBundleImpl: the four rays against the triangle's plane, then each hit point against the three
// edges, with a tolerance scaled by the normal's squared length. The lanes that hit nearer than their output's hit
// fraction get the fraction and the normal turned against the ray; the result is the mask of those lanes.
hkVector4Comparison hkpTriangleShape::castRayBundleImpl(
    const hkpShapeRayBundleCastInput& input,
    hkpShapeRayBundleCastOutput& output,
    const hkVector4Comparison& mask
) const {
    HK_TIMER_BEGIN("rayBundleTriangle", nullptr);
    hkVector4Comparison result = mask;
    hkVector4 edge0;
    edge0.setSub4(m_vertexB, m_vertexA);
    hkVector4 edge1;
    edge1.setSub4(m_vertexC, m_vertexA);
    hkVector4 normal;
    normal.setCross(edge0, edge1);
    const __m128 planeDot = normal.dot3(m_vertexA).getQuad();
    const __m128 planeDist = _mm_shuffle_ps(planeDot, planeDot, 0);
    // The rays' ends' distances from the plane, one lane per ray (the ends are stored transposed).
    const __m128 zero = _mm_setzero_ps();
    const __m128 normalX = _mm_shuffle_ps(normal.m_quad, normal.m_quad, 0x00);
    const __m128 normalY = _mm_shuffle_ps(normal.m_quad, normal.m_quad, 0x55);
    const __m128 normalZ = _mm_shuffle_ps(normal.m_quad, normal.m_quad, 0xAA);
    hkVector4 fromDist;
    fromDist.m_quad = _mm_sub_ps(
        _mm_add_ps(
            _mm_mul_ps(normalZ, input.m_from[2].m_quad),
            _mm_add_ps(
                _mm_mul_ps(normalY, input.m_from[1].m_quad),
                _mm_add_ps(_mm_mul_ps(normalX, input.m_from[0].m_quad), zero)
            )
        ),
        planeDist
    );
    const __m128 toDist = _mm_sub_ps(
        _mm_add_ps(
            _mm_mul_ps(normalZ, input.m_to[2].m_quad),
            _mm_add_ps(
                _mm_mul_ps(normalY, input.m_to[1].m_quad), _mm_add_ps(_mm_mul_ps(normalX, input.m_to[0].m_quad), zero)
            )
        ),
        planeDist
    );
    const __m128 normalLength2 = normal.dot3(normal).getQuad();
    const __m128 tolerance = _mm_mul_ps(_mm_shuffle_ps(normalLength2, normalLength2, 0), _mm_set1_ps(-0.0001f));
    // The hit fractions: fromDist / (fromDist - toDist), through the reciprocal estimate refined by one Newton step.
    const __m128 denominator = _mm_sub_ps(fromDist.m_quad, toDist);
    const __m128 reciprocal = _mm_rcp_ps(denominator);
    hkVector4 hitFractions;
    hitFractions.m_quad = _mm_mul_ps(
        _mm_sub_ps(_mm_add_ps(reciprocal, reciprocal), _mm_mul_ps(_mm_mul_ps(reciprocal, denominator), reciprocal)),
        fromDist.m_quad
    );
    // A ray misses the plane when both ends lie on one side, unless only its start lies in it.
    const __m128 missesPlane = _mm_and_ps(
        _mm_or_ps(_mm_andnot_ps(_mm_cmpeq_ps(toDist, zero), rdata_10CE1C0), _mm_cmpeq_ps(fromDist.m_quad, zero)),
        _mm_cmple_ps(zero, _mm_mul_ps(toDist, fromDist.m_quad))
    );
    const __m128 outputFractions = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(output.m_outputs[0].m_hitFraction), _mm_set_ss(output.m_outputs[2].m_hitFraction)),
        _mm_unpacklo_ps(_mm_set_ss(output.m_outputs[1].m_hitFraction), _mm_set_ss(output.m_outputs[3].m_hitFraction))
    );
    result.m_mask = _mm_and_ps(
        _mm_and_ps(_mm_andnot_ps(missesPlane, rdata_10CE1C0), result.m_mask),
        _mm_cmplt_ps(hitFractions.m_quad, outputFractions)
    );
    // The hit points, transposed as the rays are.
    const __m128 remainders = _mm_sub_ps(hkQuadReal1111, hitFractions.m_quad);
    hkVector4 hitPoints[3];
    for (int32_t i = 0; i < 3; ++i) {
        hitPoints[i].m_quad = _mm_add_ps(
            _mm_mul_ps(hitFractions.m_quad, input.m_to[i].m_quad), _mm_mul_ps(remainders, input.m_from[i].m_quad)
        );
    }
    hkVector4 edgeSides;
    calcEdgeSides(&m_vertexA, &m_vertexB, &normal, &edgeSides, hitPoints);
    result.m_mask = _mm_and_ps(_mm_cmple_ps(tolerance, edgeSides.m_quad), result.m_mask);
    calcEdgeSides(&m_vertexB, &m_vertexC, &normal, &edgeSides, hitPoints);
    result.m_mask = _mm_and_ps(_mm_cmple_ps(tolerance, edgeSides.m_quad), result.m_mask);
    calcEdgeSides(&m_vertexC, &m_vertexA, &normal, &edgeSides, hitPoints);
    result.m_mask = _mm_and_ps(_mm_cmple_ps(tolerance, edgeSides.m_quad), result.m_mask);
    const int32_t iHitBits = _mm_movemask_ps(result.m_mask);
    hkVector4 hitNormal = normal;
    hkVector4 startSide = hitFractions;
    for (int32_t i = 0; i < 4; ++i) {
        if ((iHitBits & axisMaskBits_10C92E0[i]) != 0) {
            hkpShapeRayCastOutput& rOutput = output.m_outputs[i];
            rOutput.m_hitFraction = XFNV_M128_F32(hitFractions.m_quad, i);
            hitNormal.normalize3();
            // The normal takes the sign of the start's distance, so that it faces the ray.
            startSide.setBroadcast(fromDist, i);
            rOutput.m_normal.m_quad = _mm_xor_ps(_mm_and_ps(startSide.m_quad, hkQuadRealNegateMask), hitNormal.m_quad);
            rOutput.m_shapeKeys[rOutput.m_shapeKeyIndex] = HK_INVALID_SHAPE_KEY;
        }
    }
    HK_TIMER_END();
    return result;
}

int32_t hkpTriangleShape::getNumCollisionSpheresImpl() const {
    return 3 * (m_isExtruded + 1);
}

const hkSphere* hkpTriangleShape::getCollisionSpheresImpl(hkSphere* sphereBuffer) const {
    (*sphereBuffer).m_pos = m_vertexA;
    XFNV_M128_F32(sphereBuffer->m_pos.m_quad, 3) = m_radius;
    sphereBuffer[1].m_pos = m_vertexB;
    XFNV_M128_F32(sphereBuffer[1].m_pos.m_quad, 3) = m_radius;
    sphereBuffer[2].m_pos = m_vertexC;
    XFNV_M128_F32(sphereBuffer[2].m_pos.m_quad, 3) = m_radius;
    if (m_isExtruded) {
        sphereBuffer[3].m_pos.m_quad = _mm_add_ps(m_extrusion.m_quad, m_vertexA.m_quad);
        XFNV_M128_F32(sphereBuffer[3].m_pos.m_quad, 3) = m_radius;
        sphereBuffer[4].m_pos.m_quad = _mm_add_ps(m_extrusion.m_quad, m_vertexB.m_quad);
        XFNV_M128_F32(sphereBuffer[4].m_pos.m_quad, 3) = m_radius;
        sphereBuffer[5].m_pos.m_quad = _mm_add_ps(m_vertexC.m_quad, m_extrusion.m_quad);
        XFNV_M128_F32(sphereBuffer[5].m_pos.m_quad, 3) = m_radius;
    }
    return sphereBuffer;
}

void hkpTriangleShape::getFirstVertex(hkVector4& v) const {
    v = m_vertexA;
}

// The SDK's getSupportingVertexImpl: the vertex furthest along aDirection, moved by the extrusion when that points
// along aDirection; its vertex id (3 to 5 for the extruded copies) goes in w.
void hkpTriangleShape::getSupportingVertexImpl(const hkVector4& aDirection, hkpCdVertex& supportingVertexOut) const {
    // The three vertices transposed (HK_TRANSPOSE3), so that one product gives their three dot products.
    const __m128 vertexB = m_vertexB.m_quad;
    const __m128 vertexC = m_vertexC.m_quad;
    const __m128 lo = _mm_unpacklo_ps(m_vertexA.m_quad, vertexB);
    const __m128 rowX = _mm_movelh_ps(lo, vertexC);
    const __m128 rowY = _mm_shuffle_ps(_mm_movehl_ps(rowX, lo), vertexC, 0xD4);
    const __m128 rowZ = _mm_shuffle_ps(_mm_unpackhi_ps(m_vertexA.m_quad, vertexB), vertexC, 0xE4);
    const __m128 direction = aDirection.m_quad;
    hkVector4 dots;
    dots.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(direction, direction, 0x55), rowY),
            _mm_mul_ps(_mm_shuffle_ps(direction, direction, 0x00), rowX)
        ),
        _mm_mul_ps(_mm_shuffle_ps(direction, direction, 0xAA), rowZ)
    );
    int32_t vertexId;
    double maxDot = XFNV_M128_F32(dots.m_quad, 2);
    if (!(XFNV_M128_F32(dots.m_quad, 1) < maxDot)) {
        maxDot = XFNV_M128_F32(dots.m_quad, 1);
        vertexId = 1;
    }
    else {
        vertexId = 2;
    }
    const float fMaxDot = maxDot;
    if (fMaxDot < static_cast<double>(XFNV_M128_F32(dots.m_quad, 0))) {
        vertexId = 0;
    }
    supportingVertexOut.m_quad = getVertex(vertexId).m_quad;
    const __m128 extrusion = m_extrusion.m_quad;
    const float extrusionDot = aDirection.dot3(m_extrusion);
    if (extrusionDot > 0.0f) {
        vertexId += 3;
        supportingVertexOut.m_quad = _mm_add_ps(extrusion, supportingVertexOut.m_quad);
    }
    supportingVertexOut.setInt24W(vertexId);
}

// The SDK's convertVertexIdsToVerticesImpl: each id's vertex (ids 3 to 5 are the extruded copies of 0 to 2), with the
// id kept in w.
void hkpTriangleShape::convertVertexIdsToVerticesImpl(
    const uint16_t* ids,
    int32_t numIds,
    hkpCdVertex* verticesOut
) const {
    for (int32_t i = numIds - 1; i >= 0; --i) {
        const __m128 vertex = getVertex(mod3Table[*ids]).m_quad;
        verticesOut->m_quad = vertex;
        const __m128 extrusionFactor = _mm_set_ss(addExtrusionTable[*ids]);
        verticesOut->m_quad =
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(extrusionFactor, extrusionFactor, 0), m_extrusion.m_quad), vertex);
        verticesOut->setInt24W(*ids);
        ++verticesOut;
        ++ids;
    }
}

// The SDK's weldContactPointImpl: the contact's edge from its feature points, then the separating normal against the
// edge's welding angles. A two-sided triangle snaps the normal into the edge's sector; a one-sided one replaces it by
// the edge's snap vector when it points past the edge's limit, then keeps the contact only when the colliding shape's
// centre lies behind it (extruded: clips the distance instead). separatingNormalInOut gets the new normal with the
// distance in w; the result is one of hkpConvexShape's WeldResult values.
int32_t hkpTriangleShape::weldContactPointImpl(
    uint16_t* featurePoints,
    uint8_t& numFeaturePoints,
    hkVector4& contactPointWs,
    const hkTransform* thisObjTransform,
    const hkpConvexShape* collidingShape,
    const hkTransform* collidingTransform,
    hkVector4& separatingNormalInOut
) const {
    const int32_t weldingType = m_weldingType.m_storage;
    if (weldingType == hkpWeldingUtility::WELDING_TYPE_NONE) {
        return hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_UNMODIFIED;
    }
    int32_t edgeId = mod3Table[featurePoints[0]];
    if (numFeaturePoints > 1) {
        int32_t otherEdgeId = mod3Table[featurePoints[1]];
        if (numFeaturePoints == 3) {
            // The three vertices 0, 1 and 2: a face contact, nothing to weld.
            if (featurePoints[0] + featurePoints[1] + featurePoints[2] == 3) {
                return hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_UNMODIFIED;
            }
            if (otherEdgeId == edgeId) {
                otherEdgeId = mod3Table[featurePoints[2]];
            }
            numFeaturePoints = 2;
            featurePoints[0] = static_cast<uint16_t>(edgeId);
            featurePoints[1] = static_cast<uint16_t>(otherEdgeId);
        }
        // The edge starts at the vertex the other one follows.
        if (mod3Table[edgeId + 1] != otherEdgeId) {
            edgeId = otherEdgeId;
        }
    }
    hkVector4 separatingNormal = separatingNormalInOut;
    hkVector4 normalLocal;
    normalLocal._setRotatedInverseDir(thisObjTransform->m_rotation, separatingNormal);
    hkVector4 triangleNormal;
    calcTriangleNormal(&m_vertexA.m_quad, &m_vertexC.m_quad, &triangleNormal.m_quad, &m_vertexB);
    const float cosAngle = triangleNormal.dot3(normalLocal);
    const float absCosAngle = std::fabs(cosAngle);
    float distance = absCosAngle * XFNV_M128_F32(separatingNormal.m_quad, 3);
    hkVector4 edge;
    edge.setSub4(getVertex(mod3Table[edgeId + 1]), getVertex(edgeId));
    edge.normalize3();
    // EXE-INLINED 0xD1FEF0
    const int32_t edgeBitcode = (m_weldingInfo >> (5 * edgeId)) & 0x1F;
    // The SDK reads the edge's table entry as an array of reals, indexed by the sector or the welding type.
    const hkpWeldingUtility::SinCosTableEntry& rSinCos = hkpWeldingUtility::m_sinCosTable[edgeBitcode];
    const float* sinCosTableEntry = &rSinCos.m_cosAccept0;
    if (weldingType == hkpWeldingUtility::WELDING_TYPE_TWO_SIDED) {
        // EXE-INLINED none: hkpWeldingUtility::getSector
        hkpWeldingUtility::SectorType sector;
        if (cosAngle >= rSinCos.m_cosAccept0) {
            sector = hkpWeldingUtility::ACCEPT_0;
        }
        else if (cosAngle >= rSinCos.m_cosSnap0) {
            sector = hkpWeldingUtility::SNAP_0;
        }
        else if (cosAngle >= rSinCos.m_cosSnap1) {
            sector = hkpWeldingUtility::REJECT;
        }
        else if (cosAngle >= rSinCos.m_cosAccept1) {
            sector = hkpWeldingUtility::SNAP_1;
        }
        else {
            sector = hkpWeldingUtility::ACCEPT_1;
        }
        if (sector == hkpWeldingUtility::REJECT) {
            return hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT;
        }
        if (sector != hkpWeldingUtility::SNAP_0 && sector != hkpWeldingUtility::SNAP_1) {
            return hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_UNMODIFIED;
        }
        if (numFeaturePoints == 1) {
            return hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT;
        }
        // EXE-INLINED none: hkpWeldingUtility::snapCollisionNormal
        hkVector4 orthNormalEdge;
        orthNormalEdge.setCross(edge, triangleNormal);
        hkVector4 snapVector;
        snapVector.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_set1_ps(sinCosTableEntry[sector]), triangleNormal.m_quad),
            _mm_mul_ps(_mm_set1_ps(sinCosTableEntry[sector + 1]), orthNormalEdge.m_quad)
        );
        snapVector.normalize3();
        const __m128 snapDot = snapVector.dot3(normalLocal).getQuad();
        const __m128 edgeDot = edge.dot3(normalLocal).getQuad();
        normalLocal.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(snapDot, snapDot, 0), snapVector.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(edgeDot, edgeDot, 0), edge.m_quad)
        );
        normalLocal.normalize3();
        separatingNormal.setRotatedDir(thisObjTransform->m_rotation, normalLocal);
    }
    else {
        // EXE-INLINED none: hkpWeldingUtility::shouldSnapOneSided
        const bool bSnap = (weldingType == hkpWeldingUtility::WELDING_TYPE_ANTICLOCKWISE)
            ? cosAngle < sinCosTableEntry[weldingType]
            : cosAngle > sinCosTableEntry[weldingType];
        if (!bSnap) {
            return hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_UNMODIFIED;
        }
        // EXE-INLINED none: hkpWeldingUtility::calcSnapVectorOneSided
        hkVector4 orthNormalEdge;
        orthNormalEdge.setCross(edge, triangleNormal);
        separatingNormal.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_set1_ps(sinCosTableEntry[weldingType]), triangleNormal.m_quad),
            _mm_mul_ps(_mm_set1_ps(sinCosTableEntry[weldingType + 1]), orthNormalEdge.m_quad)
        );
        separatingNormal.normalize3();
        separatingNormal.setRotatedDir(thisObjTransform->m_rotation, separatingNormal);
        if (m_isExtruded) {
            // The contact's distance below the triangle, along the normal the welding type faces.
            triangleNormal.m_quad =
                _mm_mul_ps(_mm_set1_ps(flipNormalBasedOnWeldingTypeTable[weldingType]), triangleNormal.m_quad);
            hkVector4 contactPointLocal;
            contactPointLocal.setTransformedInversePos(*thisObjTransform, contactPointWs);
            hkVector4 toContact;
            toContact.setSub4(contactPointLocal, m_vertexA);
            const float depth = toContact.dot3(triangleNormal);
            distance = hkMath::min2(depth, distance);
        }
        else {
            // The contact is kept only when the colliding shape's centre lies behind it along the new normal.
            hkVector4 centre;
            collidingShape->getCentre(centre);
            centre.setTransformedPos(*collidingTransform, centre);
            hkVector4 fromCentre;
            fromCentre.setSub4(contactPointWs, centre);
            const __m128 centreDot = fromCentre.dot3(separatingNormal).getQuad();
            if ((_mm_movemask_ps(centreDot) & 1) == 0) {
                return hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT;
            }
        }
    }
    separatingNormalInOut = separatingNormal;
    XFNV_M128_F32(separatingNormalInOut.m_quad, 3) = distance;
    return hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED;
}

void hkpTriangleShape::getCentreImpl(hkVector4& centreOut) const {
    hkVector4 v2;
    hkVector4 v3;
    v2.m_quad = _mm_add_ps(m_vertexB.m_quad, m_vertexA.m_quad);
    centreOut.m_quad = v2.m_quad;
    v3.m_quad = _mm_add_ps(m_vertexC.m_quad, v2.m_quad);
    centreOut.m_quad = v3.m_quad;
    centreOut.m_quad = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(0x3EAAAAABu)), _mm_set_ss(std::bit_cast<float>(0x3EAAAAABu)), 0),
        v3.m_quad
    );
}

const hkClass& hkpTriangleShape::staticClass() {
    return hkpTriangleShapeClass;
}
