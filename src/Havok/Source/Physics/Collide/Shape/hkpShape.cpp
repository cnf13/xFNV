#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbCastUtils.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/Mesh/hkpMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayBundleCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

// The reflected members of hkpShape; the type member's enum is null.
using hkpShapeClass_MembersArray = const hkClassMember[2];
static hkpShapeClass_MembersArray hkpShapeClass_Members{
    {"userData", nullptr, nullptr, {hkClassMember::TYPE_ULONG}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
    {"type", nullptr, nullptr, {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_UINT32}, 0, {0x400}, 0xC, nullptr}
};

hkClass hkpShapeClass(
    "hkpShape",
    &hkReferencedObjectClass,
    16,
    nullptr,
    0,
    nullptr,
    0,
    hkpShapeClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);

hkpShape::hkpShape(hkpShapeType a1) : m_userData(nullptr) {
    m_type = a1;
}

hkpShape::~hkpShape() {}

// The SDK's getMaximumProjection: the shape's AABB at the identity transform, with no tolerance, and the projection
// on direction of the AABB corner furthest along it: center plus the half extents signed as direction.
float hkpShape::getMaximumProjection(const hkVector4& direction) const {
    hkAabb aabb;
    hkTransform localToWorld;
    // EXE-INLINED 0x577070
    localToWorld.m_rotation.m_col0.m_quad = _mm_setzero_ps();
    localToWorld.m_rotation.m_col0.m_quad[0] = 1.0f;
    localToWorld.m_rotation.m_col1.m_quad = _mm_setzero_ps();
    localToWorld.m_rotation.m_col1.m_quad[1] = 1.0f;
    localToWorld.m_rotation.m_col2.m_quad = _mm_setzero_ps();
    localToWorld.m_rotation.m_col2.m_quad[2] = 1.0f;
    localToWorld.m_translation.m_quad = _mm_setzero_ps();
    getAabbImpl(localToWorld, 0.0f, aabb);
    // The half extents and the center, the half extents' signs flipped where direction's are set, then dot3 with
    // direction, summed y + x, then z + that.
    const hkQuadReal halfExtents = _mm_mul_ps(_mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad), hkQuadRealHalf);
    const hkQuadReal center = _mm_mul_ps(_mm_add_ps(aabb.m_min.m_quad, aabb.m_max.m_quad), hkQuadRealHalf);
    const hkQuadReal support =
        _mm_add_ps(_mm_xor_ps(halfExtents, _mm_and_ps(direction.m_quad, hkQuadRealNegateMask)), center);
    const hkQuadReal products = _mm_mul_ps(support, direction.m_quad);
    const float fXY = _mm_shuffle_ps(products, products, 0x55)[0] + products[0];
    return _mm_shuffle_ps(products, products, 0xAA)[0] + fXY;
}

// A shape that cannot go to the SPU.
int32_t hkpShape::calcSizeForSpu(const CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const {
    return -1;
}

// The SDK's castRayBundleImpl: the bundle's coordinates are transposed into one point per ray (w zero), each ray the
// mask selects is cast on its own through castRayImpl, and the result holds the lanes whose ray hit.
hkVector4Comparison* hkpShape::castRayBundleImpl(
    hkVector4Comparison* apResult,
    const hkpShapeRayBundleCastInput& input,
    hkpShapeRayBundleCastOutput& output,
    const hkVector4Comparison& mask
) const {
    const hkQuadReal kZero = _mm_setzero_ps();
    hkQuadReal from[4];
    hkQuadReal to[4];
    {
        const hkQuadReal xy01 = _mm_shuffle_ps(input.m_from[0].m_quad, input.m_from[1].m_quad, 0x44);
        const hkQuadReal xy23 = _mm_shuffle_ps(input.m_from[0].m_quad, input.m_from[1].m_quad, 0xEE);
        const hkQuadReal z01 = _mm_shuffle_ps(input.m_from[2].m_quad, kZero, 0x44);
        const hkQuadReal z23 = _mm_shuffle_ps(input.m_from[2].m_quad, kZero, 0xEE);
        from[0] = _mm_shuffle_ps(xy01, z01, 0x88);
        from[1] = _mm_shuffle_ps(xy01, z01, 0xDD);
        from[2] = _mm_shuffle_ps(xy23, z23, 0x88);
        from[3] = _mm_shuffle_ps(xy23, z23, 0xDD);
    }
    {
        const hkQuadReal xy01 = _mm_shuffle_ps(input.m_to[0].m_quad, input.m_to[1].m_quad, 0x44);
        const hkQuadReal xy23 = _mm_shuffle_ps(input.m_to[0].m_quad, input.m_to[1].m_quad, 0xEE);
        const hkQuadReal z01 = _mm_shuffle_ps(input.m_to[2].m_quad, kZero, 0x44);
        const hkQuadReal z23 = _mm_shuffle_ps(input.m_to[2].m_quad, kZero, 0xEE);
        to[0] = _mm_shuffle_ps(xy01, z01, 0x88);
        to[1] = _mm_shuffle_ps(xy01, z01, 0xDD);
        to[2] = _mm_shuffle_ps(xy23, z23, 0x88);
        to[3] = _mm_shuffle_ps(xy23, z23, 0xDD);
    }
    hkpShapeRayCastInput rayInput;
    rayInput.m_filterInfo = input.m_filterInfo;
    rayInput.m_rayShapeCollectionFilter = input.m_rayShapeCollectionFilter;
    uint32_t uiHitMask = 0;
    for (int32_t i = 0; i < 4; i++) {
        if ((_mm_movemask_ps(mask.m_mask) & axisMaskBits_10C92E0[i]) != 0) {
            rayInput.m_from.m_quad = from[i];
            rayInput.m_to.m_quad = to[i];
            if (castRayImpl(rayInput, output.m_outputs[i]).m_bool) {
                uiHitMask |= axisMaskBits_10C92E0[i];
            }
        }
    }
    apResult->m_mask = hkVector4Comparison_s_maskFromBits[uiHitMask];
    return apResult;
}

// The class registry's finish callback for hkpMeshShape: runs the
// mesh shape's finish constructor on apMemory when apMemory is set, with the caller's finishing flag.
void finishLoadedObjecthkpMeshShape(void* p, int32_t finishing) {
    if (p != nullptr) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpMeshShape(kFlag);
    }
}
