#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
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
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/hkpSphereUtil.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpSphereShape(void* p);

using hkpSphereShapeClass_MembersArray = const hkClassMember[1];
static hkpSphereShapeClass_MembersArray hkpSphereShapeClass_Members{
    {"pad16",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     3,
     {hkClassMember::SERIALIZE_IGNORED},
     0x14,
     nullptr}
};
hkClass hkpSphereShapeClass(
    "hkpSphereShape",
    &hkpConvexShapeClass,
    0x20,
    nullptr,
    0,
    nullptr,
    0,
    hkpSphereShapeClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpSphereShapeTypeInfo = {
    "hkpSphereShape",          finishLoadedObjecthkpSphereShape, cleanupLoadedObjecthkpSphereShape,
    getVtablehkpSphereShape(), &typeid(hkpSphereShape),          sizeof(hkpSphereShape)
};

static void cleanupLoadedObjecthkpSphereShape(void* p) {
    static_cast<hkpSphereShape*>(p)->~hkpSphereShape();
}

hkpSphereShape::hkpSphereShape(float afRadius) : hkpConvexShape(HK_SHAPE_SPHERE, afRadius) {}

hkpSphereShape::~hkpSphereShape() {}

void hkpSphereShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    hkReferencedObject::calcContentStatistics(collector, &hkpConvexShapeClass);
}

int32_t hkpSphereShape::calcSizeForSpu(const CalcSizeForSpuInput& a1, int32_t a2) const {
    return 32;
}

void hkpSphereShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    uint32_t v5;
    *reinterpret_cast<float*>(&v5) = m_radius + tolerance;
    __m128 v4 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v5)), _mm_set_ss(std::bit_cast<float>(v5)), 0);
    out.m_min.m_quad = _mm_sub_ps(localToWorld.m_translation.m_quad, v4);
    out.m_max.m_quad = _mm_add_ps(v4, localToWorld.m_translation.m_quad);
}

hkBool hkpSphereShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    HK_TIMER_BEGIN("rcSphere", nullptr);
    hkBool result = hkpSphereUtil::castRayUtil(m_radius, input, results);
    HK_TIMER_END();
    return result;
}

// The four rays of the bundle against the sphere at once, lane by lane. A ray whose start lies far along its
// direction (dir.dir * r^2 * 100 < (from.dir)^2) first moves its start to the closest approach, for precision. The
// result is the mask of the rays that hit before their output's hit fraction; for each, the hit fraction, the
// normal (the hit point over the radius) and an invalid key at the output's key index are written.
// x87 copies: the four 100.0f lanes and each hit fraction.
hkVector4Comparison hkpSphereShape::castRayBundleImpl(
    const hkpShapeRayBundleCastInput& input,
    hkpShapeRayBundleCastOutput& output,
    const hkVector4Comparison& mask
) const {
    hkVector4Comparison result;
    HK_TIMER_BEGIN("rcSphereBundle", nullptr);
    __m128 kHundred;
    XFNV_M128_F32(kHundred, 0) = 100.0f;
    XFNV_M128_F32(kHundred, 1) = 100.0f;
    XFNV_M128_F32(kHundred, 2) = 100.0f;
    XFNV_M128_F32(kHundred, 3) = 100.0f;
    result.m_mask = mask.m_mask;
    const __m128 kRadiusLow = _mm_set_ss(m_radius);
    const __m128 kRadius = _mm_shuffle_ps(kRadiusLow, kRadiusLow, 0);
    const __m128 kRadius2 = _mm_mul_ps(kRadius, kRadius);
    const __m128 kZero = _mm_setzero_ps();
    const __m128 kDir0 = _mm_sub_ps(input.m_to[0].m_quad, input.m_from[0].m_quad);
    const __m128 kDir1 = _mm_sub_ps(input.m_to[1].m_quad, input.m_from[1].m_quad);
    const __m128 kDir2 = _mm_sub_ps(input.m_to[2].m_quad, input.m_from[2].m_quad);
    const __m128 kFromDotDir = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(input.m_from[1].m_quad, kDir1), _mm_mul_ps(input.m_from[0].m_quad, kDir0)),
        _mm_mul_ps(input.m_from[2].m_quad, kDir2)
    );
    const __m128 kDirLen2 =
        _mm_add_ps(_mm_add_ps(_mm_mul_ps(kDir1, kDir1), _mm_mul_ps(kDir0, kDir0)), _mm_mul_ps(kDir2, kDir2));
    const __m128 kFromDotDir2 = _mm_mul_ps(kFromDotDir, kFromDotDir);
    const __m128 kFar = _mm_cmplt_ps(_mm_mul_ps(_mm_mul_ps(kDirLen2, kRadius2), kHundred), kFromDotDir2);
    // The far rays' -from.dir, and the near rays' from.dir and (from.dir)^2 (zero in the other lanes).
    const __m128 kNegFarFd =
        _mm_xor_ps(_mm_or_ps(_mm_and_ps(kFar, kFromDotDir), _mm_andnot_ps(kFar, kZero)), hkQuadRealNegateMask);
    const __m128 kFarZero = _mm_and_ps(kFar, kZero);
    const __m128 kNearFd2 = _mm_or_ps(_mm_andnot_ps(kFar, kFromDotDir2), kFarZero);
    const __m128 kNearFd = _mm_or_ps(_mm_andnot_ps(kFar, kFromDotDir), kFarZero);
    // 1 / dir.dir, one Newton step on the estimate.
    const __m128 kRcpLen2 = _mm_rcp_ps(kDirLen2);
    const __m128 kInvLen2 =
        _mm_sub_ps(_mm_add_ps(kRcpLen2, kRcpLen2), _mm_mul_ps(_mm_mul_ps(kRcpLen2, kDirLen2), kRcpLen2));
    // The fraction the far rays' start moves to, and the new start's distance from the centre, squared.
    const __m128 kStart = _mm_mul_ps(kNegFarFd, kInvLen2);
    const __m128 kOneMinusStart = _mm_sub_ps(hkQuadReal1111, kStart);
    alignas(16) hkVector4 points[4];
    __m128 kStartLen2 = kZero;
    for (int32_t i = 0; i < 3; ++i) {
        points[i].m_quad = _mm_mul_ps(kOneMinusStart, input.m_from[i].m_quad);
        points[i].m_quad = _mm_add_ps(_mm_mul_ps(input.m_to[i].m_quad, kStart), points[i].m_quad);
        kStartLen2 = _mm_add_ps(_mm_mul_ps(points[i].m_quad, points[i].m_quad), kStartLen2);
    }
    // The discriminant over dir.dir; rays without a root drop out of the result.
    const __m128 kDisc =
        _mm_add_ps(_mm_mul_ps(_mm_sub_ps(kStartLen2, kRadius2), _mm_xor_ps(kDirLen2, hkQuadRealNegateMask)), kNearFd2);
    const __m128 kHasRoot = _mm_cmplt_ps(kZero, kDisc);
    const __m128 kRootMask = _mm_and_ps(result.m_mask, kHasRoot);
    result.m_mask = kRootMask;
    // sqrt(disc) as disc * rsqrt(disc), one Newton step on the estimate (zero where there is no root).
    const __m128 kRsqrt = _mm_rsqrt_ps(kDisc);
    const __m128 kSqrt = _mm_or_ps(
        _mm_and_ps(
            _mm_mul_ps(
                _mm_mul_ps(
                    _mm_sub_ps(hkQuadReal3333, _mm_mul_ps(_mm_mul_ps(kDisc, kRsqrt), kRsqrt)),
                    _mm_mul_ps(kRsqrt, hkQuadRealHalf)
                ),
                kDisc
            ),
            kHasRoot
        ),
        _mm_andnot_ps(kHasRoot, kZero)
    );
    alignas(16) hkVector4 hitT;
    hitT.m_quad = _mm_mul_ps(_mm_sub_ps(_mm_sub_ps(kNegFarFd, kNearFd), kSqrt), kInvLen2);
    const __m128 kFractions = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(output.m_outputs[0].m_hitFraction), _mm_set_ss(output.m_outputs[2].m_hitFraction)),
        _mm_unpacklo_ps(_mm_set_ss(output.m_outputs[1].m_hitFraction), _mm_set_ss(output.m_outputs[3].m_hitFraction))
    );
    const __m128 kHit =
        _mm_and_ps(_mm_and_ps(_mm_cmplt_ps(hitT.m_quad, kFractions), kRootMask), _mm_cmple_ps(kZero, hitT.m_quad));
    // 1 / radius, one Newton step on the estimate.
    const __m128 kRcpRadius = _mm_rcp_ps(kRadius);
    const __m128 kInvRadius =
        _mm_sub_ps(_mm_add_ps(kRcpRadius, kRcpRadius), _mm_mul_ps(_mm_mul_ps(kRcpRadius, kRadius), kRcpRadius));
    result.m_mask = kHit;
    // The hit points over the radius: the normals, coordinate by coordinate.
    const __m128 kOneMinusHit = _mm_sub_ps(hkQuadReal1111, hitT.m_quad);
    for (int32_t i = 0; i < 3; ++i) {
        points[i].m_quad = _mm_mul_ps(input.m_from[i].m_quad, kOneMinusHit);
        points[i].m_quad = _mm_add_ps(_mm_mul_ps(input.m_to[i].m_quad, hitT.m_quad), points[i].m_quad);
        points[i].m_quad = _mm_mul_ps(kInvRadius, points[i].m_quad);
    }
    // Transposed to one normal per ray (HK_TRANSPOSE4 with a zero fourth row).
    const __m128 kLo01 = _mm_shuffle_ps(points[0].m_quad, points[1].m_quad, 0x44);
    const __m128 kHi01 = _mm_shuffle_ps(points[0].m_quad, points[1].m_quad, 0xEE);
    const __m128 kLo2 = _mm_shuffle_ps(points[2].m_quad, kZero, 0x44);
    const __m128 kHi2 = _mm_shuffle_ps(points[2].m_quad, kZero, 0xEE);
    points[1].m_quad = _mm_shuffle_ps(kLo01, kLo2, 0xDD);
    points[0].m_quad = _mm_shuffle_ps(kLo01, kLo2, 0x88);
    points[2].m_quad = _mm_shuffle_ps(kHi01, kHi2, 0x88);
    points[3].m_quad = _mm_shuffle_ps(kHi01, kHi2, 0xDD);
    for (int32_t i = 0; i < 4; ++i) {
        if ((_mm_movemask_ps(kHit) & axisMaskBits_10C92E0[i]) != 0) {
            hkpShapeRayCastOutput& rOut = output.m_outputs[i];
            rOut.m_hitFraction = XFNV_M128_F32(hitT.m_quad, i);
            rOut.m_normal.m_quad = points[i].m_quad;
            rOut.m_shapeKeys[rOut.m_shapeKeyIndex] = HK_INVALID_SHAPE_KEY;
        }
    }
    HK_TIMER_END();
    return result;
}

int32_t hkpSphereShape::getNumCollisionSpheresImpl() const {
    return 1;
}

const hkSphere* hkpSphereShape::getCollisionSpheresImpl(hkSphere* sphereBuffer) const {
    (*sphereBuffer).m_pos.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(sphereBuffer->m_pos.m_quad, 3) = m_radius;
    return sphereBuffer;
}

void hkpSphereShape::getFirstVertex(hkVector4& v) const {
    v.m_quad = _mm_setzero_ps();
}

void hkpSphereShape::getSupportingVertexImpl(const hkVector4& direction, hkpCdVertex& supportingVertexOut) const {
    supportingVertexOut.m_quad = _mm_setzero_ps();
}

// Every vertex of a sphere is its centre: each output vertex is zeroed.
void hkpSphereShape::convertVertexIdsToVerticesImpl(
    const uint16_t* ids,
    int32_t numIds,
    hkpCdVertex* verticesOut
) const {
    for (int32_t i = 0; i < numIds; ++i) {
        verticesOut[i].m_quad = _mm_setzero_ps();
    }
}

void hkpSphereShape::getCentreImpl(hkVector4& centreOut) const {
    centreOut.m_quad = _mm_setzero_ps();
}

const hkClass& hkpSphereShape::staticClass() {
    return hkpSphereShapeClass;
}

hkBool hkpSphereUtil::castRayUtil(float radius, const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) {
    __m128 v6;
    float v7;
    double v8;
    double v9;
    double v10;
    double v11;
    __m128 v14;
    __m128 v15;
    double v16;
    float v22;
    float v23;
    float v24;
    float v25;
    float v27;
    float v28;
    float v30;
    float v31;
    float v32;
    float v33;
    float v34;
    float v35;
    float v21 = radius * radius;
    __m128 v4 = _mm_sub_ps(input.m_to.m_quad, input.m_from.m_quad);
    __m128 v5 = _mm_mul_ps(input.m_from.m_quad, v4);
    float v29 = XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 85), 0) + XFNV_M128_F32(v5, 0));
    float v26 = v29;
    if (v29 >= 0.0f) {
        goto LABEL_9;
    }
    v6 = _mm_mul_ps(v4, v4);
    v7 = XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 85), 0) + XFNV_M128_F32(v6, 0));
    v8 = v21;
    v22 = v7 * v21 * 100.0;
    if (!(v22 < v29 * v29)) {
        v9 = v7;
        v14 = input.m_from.m_quad;
        v10 = 0.0;
        v11 = v8;
        v23 = 0.0f;
    }
    else {
        if (v7 < v8) {
            return false;
        }
        v9 = v7;
        v23 = -v29;
        v27 = v23 / v7;
        v10 = 0.0;
        v11 = v8;
        __m128 v12 = _mm_set_ss(v27);
        v26 = 0.0f;
        __m128 v13 = _mm_shuffle_ps(v12, v12, 0);
        v14 = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v13), input.m_from.m_quad), _mm_mul_ps(input.m_to.m_quad, v13)
        );
    }
    v15 = _mm_mul_ps(v14, v14);
    v30 = (XFNV_M128_F32(_mm_shuffle_ps(v15, v15, 170), 0) +
           (XFNV_M128_F32(_mm_shuffle_ps(v15, v15, 85), 0) + XFNV_M128_F32(v15, 0))) -
        v11;
    v31 = v26 * v26 - v9 * v30;
    if (v31 <= v10 ||
        ((v28 = -v26,
          (v32 = std::sqrt((double)v31),
           (v33 = v28 - v32,
            (v24 = v33 + v23, (v16 = v24, (v34 = v7 * results.m_hitFraction, v34 <= static_cast<double>(v24))))))) ||
         v16 < 0.0)) {
    LABEL_9:;
        return false;
    }
    int32_t v18 = results.m_shapeKeyIndex;
    v25 = v16 / v7;
    results.m_hitFraction = v25;
    __m128 v19 = _mm_shuffle_ps(_mm_set_ss(v25), _mm_set_ss(v25), 0);
    __m128 v20 = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v19), input.m_from.m_quad), _mm_mul_ps(input.m_to.m_quad, v19)
    );
    results.m_normal.m_quad = v20;
    v35 = 1.0 / radius;
    results.m_normal.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v35), _mm_set_ss(v35), 0), v20);
    results.m_shapeKeys[v18] = HK_INVALID_SHAPE_KEY;
    return true;
}
