#include "Havok/Source/Physics/Collide/Shape/HeightField/Plane/hkpPlaneShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <bit>
#include <xmmintrin.h>

// Timer names that castRayImpl's HK_TIMER macros store into the monitor stream, as in hkpConvexListAgent.cpp.
using TimerName3 = const char[3];
static TimerName3 s_szEndTimer{"Et"};
using TimerName10 = const char[10];
static TimerName10 s_szRcPlaneTimer{"TtrcPlane"};
// The reflected members of hkpPlaneShape.
using hkpPlaneShapeClass_MembersArray = const hkClassMember[3];
static hkpPlaneShapeClass_MembersArray hkpPlaneShapeClass_Members{
    {"plane", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
    {"aabbCenter", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr},
    {"aabbHalfExtents",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     nullptr}
};

hkClass hkpPlaneShapeClass(
    "hkpPlaneShape",
    &hkpHeightFieldShapeClass,
    0x40,
    nullptr,
    0,
    nullptr,
    0,
    hkpPlaneShapeClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpPlaneShapeTypeInfo = {
    "hkpPlaneShape",          finishLoadedObjecthkpPlaneShape, cleanupLoadedObjecthkpPlaneShape,
    getVtablehkpPlaneShape(), &typeid(hkpPlaneShape),          sizeof(hkpPlaneShape)
};

hkpPlaneShape::hkpPlaneShape(const hkVector4& plane, const hkAabb& aabb)
  : hkpHeightFieldShape(HK_SHAPE_PLANE), m_plane(plane) {
    m_aabbCenter.m_quad = _mm_mul_ps(_mm_add_ps(aabb.m_max.m_quad, aabb.m_min.m_quad), hkQuadRealHalf);
    m_aabbHalfExtents.m_quad = _mm_mul_ps(_mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad), hkQuadRealHalf);
}

hkpPlaneShape::hkpPlaneShape(const hkVector4& direction, const hkVector4& center, const hkVector4& halfExtents)
  : hkpHeightFieldShape(HK_SHAPE_PLANE), m_plane(direction), m_aabbCenter(center), m_aabbHalfExtents(halfExtents) {
    __m128 v5 = _mm_mul_ps(direction.m_quad, center.m_quad);
    m_plane.m_quad[3] =
        -static_cast<float>(_mm_shuffle_ps(v5, v5, 170)[0] + static_cast<float>(_mm_shuffle_ps(v5, v5, 85)[0] + v5[0]));
}

hkpPlaneShape::~hkpPlaneShape() {}

void hkpPlaneShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    hkReferencedObject::calcContentStatistics(collector, &hkpHeightFieldShapeClass);
}

// The cached box under localToWorld, grown by the tolerance.
void hkpPlaneShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    hkAabbUtil::calcAabb(localToWorld, m_aabbHalfExtents, m_aabbCenter, tolerance, out);
}

// The ray hits when it starts on or above the plane and ends below it, at a point inside the cached box (the
// interpolated point's distance to m_aabbCenter within m_aabbHalfExtents on x, y, z). The plane distance is computed as
// (z + w) + (y + x). A negative or NaN start distance misses, as does an end distance not below 0 (or NaN). The
// fraction is a float local. The timer macros write into the stream's untyped buffer (hence the reinterpret_casts), as
// in hkpConvexListAgent.
hkBool hkpPlaneShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    {
        // EXE-INLINED none: HK_TIMER_BEGIN
        hkMonitorStream* pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
        if (pStream->m_end < pStream->m_capacityMinus16) {
            auto* pBegin = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
            pBegin->m_commandAndMonitor = s_szRcPlaneTimer;
            pBegin->m_time0 = static_cast<uint32_t>(__rdtsc());
            pStream->m_end = reinterpret_cast<char*>(pBegin + 1);
        }
    }
    // EXE-INLINED none: hkVector4::dot4xyz1, on the product p the caller forms: the exe multiplies from * plane but
    // plane * to (mulps keeps its first operand's NaN).
    auto dot4xyz1 = [](__m128 p, __m128 plane) {
        const __m128 zw = _mm_add_ss(_mm_shuffle_ps(p, p, 0xAA), _mm_shuffle_ps(plane, plane, 0xFF));
        const __m128 xy = _mm_add_ss(_mm_shuffle_ps(p, p, 0x55), p);
        return _mm_cvtss_f32(_mm_add_ps(zw, xy));
    };
    const __m128 kFrom = input.m_from.m_quad;
    const __m128 kTo = input.m_to.m_quad;
    const float fFromDist = dot4xyz1(_mm_mul_ps(kFrom, m_plane.m_quad), m_plane.m_quad);
    const float fToDist = dot4xyz1(_mm_mul_ps(m_plane.m_quad, kTo), m_plane.m_quad);

    hkBool result;
    result.m_bool = false;
    if (fFromDist >= 0.0f && fToDist < 0.0f) {
        const float fFraction = fFromDist / (fFromDist - fToDist);
        // EXE-INLINED none: hkVector4::setInterpolate4, sub4, setAbs4, compareLessThanEqual4
        const __m128 kT = _mm_set1_ps(fFraction);
        const __m128 kOneMinusT = _mm_sub_ps(hkQuadReal1111_0, kT);
        const __m128 kHit = _mm_add_ps(_mm_mul_ps(kOneMinusT, kFrom), _mm_mul_ps(kTo, kT));
        const __m128 kOffset = _mm_and_ps(_mm_sub_ps(kHit, m_aabbCenter.m_quad), hkVector4_setAbs4_absMask);
        if ((_mm_movemask_ps(_mm_cmple_ps(kOffset, m_aabbHalfExtents.m_quad)) & 7) == 7) {
            results.m_hitFraction = fFraction;
            results.m_normal.m_quad = m_plane.m_quad;
            // EXE-INLINED none: hkpShapeRayCastOutput::setKey
            results.m_shapeKeys[results.m_shapeKeyIndex] = 0xFFFFFFFF;
            result.m_bool = true;
        }
    }
    {
        // EXE-INLINED none: HK_TIMER_END
        hkMonitorStream* pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
        if (pStream->m_end < pStream->m_capacityMinus16) {
            auto* pEnd = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
            pEnd->m_commandAndMonitor = s_szEndTimer;
            pEnd->m_time0 = static_cast<uint32_t>(__rdtsc());
            pStream->m_end = reinterpret_cast<char*>(pEnd + 1);
        }
    }
    return result;
}

// KEEP (partcopy:): the output handed to collector.addRayHit has m_pad (bytes 0x18-0x1F) and m_shapeKeys[1..7]
// written by nothing here nor by castRayImpl, and the collectors copy the whole collector output, m_pad included
// (hkpAllRayHitCollector::addRayHit, the inlined 0x9C1B00): stack garbage reaches the collector's hits.
// The SDK's body: the output's constructor inlined with the collector's early-out fraction as the hit fraction (copied
// with fld/fstp, 0xD54F9D), castRayImpl through the vtable, the normal turned into world space by the exe's
// out-of-line setMul3 (0xC7F750) on the body's rotation, then the hit reported.
void hkpPlaneShape::castRayWithCollectorImpl(
    const hkpShapeRayCastInput& inputLocal,
    const hkpCdBody& cdBody,
    hkpRayHitCollector& collector
) const {
    hkpShapeRayCastOutput output;
    // EXE-INLINED 0x4A3D20
    output.m_hitFraction = collector.m_earlyOutHitFraction;
    output.m_extraInfo = -1;
    output.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
    output.m_shapeKeyIndex = 0;
    if (castRayImpl(inputLocal, output).m_bool) {
        output.m_normal.setMul3(cdBody.m_motion.pMotion->getRotation(), output.m_normal);
        collector.addRayHit(cdBody, output);
    }
}

void hkpPlaneShape::collideSpheres(
    const hkpHeightFieldShape::CollideSpheresInput& input,
    hkVector4* outputArray
) const {
    double v8;
    hkSphere* m_spheres = input.m_spheres;
    for (int32_t i = input.m_numSpheres - 1; i >= 0; outputArray[-1].m_quad[3] = v8) {
        hkVector4 v6;
        *outputArray = m_plane;
        v6.m_quad = m_plane.m_quad;
        __m128 v7 = _mm_mul_ps(m_spheres->m_pos.m_quad, v6.m_quad);
        v8 = static_cast<float>(
                 static_cast<float>(_mm_shuffle_ps(v7, v7, 170)[0] + _mm_shuffle_ps(v6.m_quad, v6.m_quad, 255)[0]) +
                 static_cast<float>(_mm_shuffle_ps(v7, v7, 85)[0] + v7[0])
             ) -
            m_spheres->m_pos.m_quad[3];
        ++outputArray;
        ++m_spheres;
        --i;
    }
}

void hkpPlaneShape::castSphere(
    const hkpHeightFieldShape::hkpSphereCastInput& inputLocal,
    const hkpCdBody& cdBody,
    hkpRayHitCollector& collector
) const {
    hkVector4 v4;
    hkVector4 v7;
    uint32_t v17;
    v4.m_quad = m_plane.m_quad;
    __m128 v5 = _mm_mul_ps(v4.m_quad, inputLocal.m_from.m_quad);
    double v6 = static_cast<float>(
        static_cast<float>(_mm_shuffle_ps(v5, v5, 170)[0] + _mm_shuffle_ps(v4.m_quad, v4.m_quad, 255)[0]) +
        static_cast<float>(_mm_shuffle_ps(v5, v5, 85)[0] + v5[0])
    );
    v7.m_quad = inputLocal.m_to.m_quad;
    double m_radius = inputLocal.m_radius;
    __m128 v9 = _mm_mul_ps(v4.m_quad, v7.m_quad);
    float v16 = v6 - m_radius;
    float v18 = static_cast<float>(
                    static_cast<float>(_mm_shuffle_ps(v9, v9, 170)[0] + _mm_shuffle_ps(v4.m_quad, v4.m_quad, 255)[0]) +
                    static_cast<float>(_mm_shuffle_ps(v9, v9, 85)[0] + v9[0])
                ) -
        m_radius;
    if (!(v18 >= 0.0f)) {
        double v10 = v16;
        double v11 = v16 - v18;
        if (!(inputLocal.m_maxExtraPenetration > v11)) {
            double v12;
            if (!(v10 <= 0.0)) {
                v12 = v10 / v11;
            }
            else {
                v12 = 0.0;
            }
            *reinterpret_cast<float*>(&v17) = v12;
            __m128 v13 =
                _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v17)), _mm_set_ss(std::bit_cast<float>(v17)), 0);
            if ((_mm_movemask_ps(_mm_cmple_ps(
                     _mm_and_ps(
                         _mm_sub_ps(
                             _mm_add_ps(
                                 _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v13), inputLocal.m_from.m_quad),
                                 _mm_mul_ps(v7.m_quad, v13)
                             ),
                             m_aabbCenter.m_quad
                         ),
                         hkVector4_setAbs4_absMask
                     ),
                     m_aabbHalfExtents.m_quad
                 )) &
                 7) == 7) {
                hkVector4 v14;
                hkpShapeRayCastCollectorOutput v19;
                v14.m_quad = m_plane.m_quad;
                v19.m_hitFraction = v12;
                v19.m_extraInfo = -1;
                int32_t v20 = -1;
                int32_t v21 = 0;
                v19.m_normal.m_quad = v14.m_quad;
                collector.addRayHit(cdBody, v19);
            }
        }
    }
}

const hkClass& hkpPlaneShape::staticClass() {
    return hkpPlaneShapeClass;
}
