#include "Havok/Source/Physics/Collide/Shape/Misc/MultiRay/hkpMultiRayShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <bit>
#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpMultiRayShapeRayTypeInfo = {"hkpMultiRayShapeRay",
                                          nullptr,
                                          cleanupLoadedObjecthkpMultiRayShapeRay,
                                          nullptr,
                                          &typeid(hkpMultiRayShape::Ray),
                                          sizeof(hkpMultiRayShape::Ray)};

// The reflected members of hkpMultiRayShape::Ray.
using hkpMultiRayShape_RayClass_MembersArray = const hkClassMember[2];
static hkpMultiRayShape_RayClass_MembersArray hkpMultiRayShape_RayClass_Members{
    {"start", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"end", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr}
};

hkpMultiRayShape::MembersArray hkpMultiRayShape::Members{
    {"rays",
     &hkpMultiRayShapeRayClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x10,
     nullptr},
    {"rayPenetrationDistance",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x1C,
     nullptr}
};

hkClass hkpMultiRayShapeRayClass(
    "hkpMultiRayShapeRay",
    nullptr,
    32,
    nullptr,
    0,
    nullptr,
    0,
    hkpMultiRayShape_RayClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpMultiRayShapeClass(
    "hkpMultiRayShape",
    &hkpShapeClass,
    32,
    nullptr,
    0,
    nullptr,
    0,
    hkpMultiRayShape::Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpMultiRayShapeTypeInfo = {
    "hkpMultiRayShape",          finishLoadedObjecthkpMultiRayShape, cleanupLoadedObjecthkpMultiRayShape,
    getVtablehkpMultiRayShape(), &typeid(hkpMultiRayShape),          sizeof(hkpMultiRayShape)
};

void cleanupLoadedObjecthkpMultiRayShape(void* p) {
    static_cast<hkpMultiRayShape*>(p)->~hkpMultiRayShape();
}

const hkClass& hkpMultiRayShape::Ray::staticClass() {
    return hkpMultiRayShapeRayClass;
}

// Each ray keeps its length in m_start.w; its end is pushed along the ray by the penetration distance (a zero-length
// ray's direction is zero, as the game's masked reciprocal square root makes it).
hkpMultiRayShape::hkpMultiRayShape(const hkpMultiRayShape::Ray* Rays, int32_t nRays, float rayPenetrationDistance)
  : hkpShape(HK_SHAPE_MULTI_RAY), m_rayPenetrationDistance(rayPenetrationDistance) {
    Ray* pRay = m_rays.expandBy(nRays);
    for (int32_t i = nRays - 1; i >= 0; --i, ++pRay, ++Rays) {
        pRay->m_start = Rays->m_start;
        pRay->m_end = Rays->m_end;
        __m128 dir = _mm_sub_ps(pRay->m_end.m_quad, pRay->m_start.m_quad);
        __m128 sq = _mm_mul_ps(dir, dir);
        float fLengthSquared = sq[2] + (sq[1] + sq[0]);
        pRay->m_start.m_quad[3] = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(fLengthSquared)));
        float fEstimate = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(fLengthSquared)));
        float fInvLength =
            fLengthSquared == 0.0f ? 0.0f : (0.5f * fEstimate) * (3.0f - (fLengthSquared * fEstimate) * fEstimate);
        pRay->m_end.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(fInvLength), dir), _mm_set1_ps(m_rayPenetrationDistance)),
            pRay->m_end.m_quad
        );
    }
}

hkpMultiRayShape::~hkpMultiRayShape() {}

// The ray array's used and allocated size, then the base's statistics for hkpShapeClass (that record is passed, not a
// multi-ray one, as the game does).
void hkpMultiRayShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t v4 = m_rays.m_capacityAndFlags;
    if (v4 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v5 = hkStatisticsCollector::getAllocatedSize(
            m_rays.m_data, 32 * static_cast<int32_t>(v4 & hkArray<Ray>::CAPACITY_MASK)
        );
        collector->addChunk(hkStatisticsCollector::MEM_ARRAY, "Rays", m_rays.m_data, 32 * m_rays.m_size, v5);
    }
    hkReferencedObject::calcContentStatistics(collector, &hkpShapeClass);
}

// The rays are transformed to world space into an inline array of 16 (both ends of each ray, last point first), then
// the AABB is the min and max over them. The transform runs once even with no rays, as the game does.
void hkpMultiRayShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    // EXE-INLINED none: hkInplaceArrayAligned16<hkpMultiRayShape::Ray, 16>::hkInplaceArrayAligned16
    using worldRays_t = hkInplaceArrayAligned16<Ray, 16>;
    XFNV_INLINED_LOCAL(worldRays_t, worldRays, std::nullopt, std::nullopt);
    // The inline storage is raw bytes until the rays are written into it.
    worldRays.m_data = reinterpret_cast<Ray*>(worldRays.m_storage);
    worldRays.m_size = 0;
    worldRays.m_capacityAndFlags = hkArrayBase<Ray>::DONT_DEALLOCATE_FLAG | 16;
    // EXE-INLINED none: hkArray<hkpMultiRayShape::Ray>::setSize
    int32_t numRays = m_rays.m_size;
    worldRays._reserve(*hkMemoryRouter::getInstance().m_heap, numRays);
    hkVector4 col0;
    hkVector4 col1;
    hkVector4 col2;
    hkVector4 translation;
    col0.m_quad = localToWorld.m_rotation.m_col0.m_quad;
    col1.m_quad = localToWorld.m_rotation.m_col1.m_quad;
    col2.m_quad = localToWorld.m_rotation.m_col2.m_quad;
    translation.m_quad = localToWorld.m_translation.m_quad;
    // m_size * 2 - 1 wraps, so it is computed unsigned so that no signed overflow is assumed away.
    int32_t i = static_cast<int32_t>(static_cast<uint32_t>(m_rays.m_size) * 2u - 1u);
    worldRays.m_size = numRays;
    // EXE-INLINED none: hkVector4Util::transformPoints
    hkVector4* pDst = &worldRays.m_data[0].m_start + i;
    const hkVector4* pSrc = &m_rays.m_data[0].m_start + i;
    do {
        const __m128 p = pSrc->m_quad;
        pDst->m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(p, p, 0), col0.m_quad), _mm_mul_ps(_mm_shuffle_ps(p, p, 85), col1.m_quad)
            ),
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 170), col2.m_quad), translation.m_quad)
        );
        --i;
        --pSrc;
        --pDst;
    }
    while (i >= 0);
    int32_t count = worldRays.m_size;
    const Ray* pRay = worldRays.m_data;
    __m128 minimum = _mm_shuffle_ps(_mm_set_ss(rdata_104DA48), _mm_set_ss(rdata_104DA48), 0);
    __m128 maximum = _mm_shuffle_ps(_mm_set_ss(rdata_104E148), _mm_set_ss(rdata_104E148), 0);
    if (count > 0) {
        do {
            const __m128 end = pRay->m_end.m_quad;
            const __m128 start = pRay->m_start.m_quad;
            ++pRay;
            --count;
            minimum = _mm_min_ps(_mm_min_ps(minimum, end), start);
            maximum = _mm_max_ps(_mm_max_ps(maximum, end), start);
        }
        while (count);
    }
    out.m_min.m_quad = minimum;
    out.m_max.m_quad = maximum;
    // EXE-INLINED none: hkInplaceArrayAligned16<hkpMultiRayShape::Ray, 16>::~hkInplaceArrayAligned16
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    worldRays.m_size = 0;
    if (worldRays.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            worldRays.m_data,
            static_cast<int32_t>((worldRays.m_capacityAndFlags & hkArrayBase<Ray>::CAPACITY_MASK) * sizeof(Ray))
        );
    }
}

// A multi-ray shape is never hit by a ray.
hkBool hkpMultiRayShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    hkBool result;
    result.m_bool = false;
    return result;
}

void hkpMultiRayShape::castRayWithCollectorImpl(
    const hkpShapeRayCastInput& input,
    const hkpCdBody& cdBody,
    hkpRayHitCollector& collector
) const {
    ;
}

const hkClass& hkpMultiRayShape::staticClass() {
    return hkpMultiRayShapeClass;
}
