#include "Havok/Source/Physics/Collide/Query/Collector/KdTreeCollector/hkpKdTreeCollidableRaycaster.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbCastUtils.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeBundleCastCollector.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeUtils.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayBundleCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <xmmintrin.h>

hkKdTreeCollidableRayBundleCaster::hkKdTreeCollidableRayBundleCaster()
  : m_input(nullptr), m_filter(nullptr), m_collectionFilter(nullptr), m_output(nullptr) {}

hkKdTreeCollidableRayBundleCaster::~hkKdTreeCollidableRayBundleCaster() {}

// KEEP: on a hit, the copy into m_output[i] takes the bundle output's m_pad words (+0x18, +0x1C of each output),
// which neither this function (it writes m_hitFraction, m_extraInfo, m_shapeKeys[0] and m_shapeKeyIndex) nor a
// shape's castRayBundleImpl writes: uninitialised stack bytes reach the caller's output (0xD98258, 0xD9825E).
// The SDK's addMidPhaseHit for a bundle of four rays: the primitive is a collidable. The rays m_filter rejects (or
// mask leaves out) get a negative filter lane; the four rays are taken into the collidable's space, transposed and
// cast against its shape at once, and each ray's hit closer than its output's is copied there with the collidable
// and the normal turned back to world space.
void hkKdTreeCollidableRayBundleCaster::addMidPhaseHit(hkPrimitiveId primId, const hkVector4Comparison& mask) {
    // The kd tree keeps each collidable's address as an untyped hkPrimitiveId (hkUlong); the SDK casts it back here.
    hkpCollidable* collidable = reinterpret_cast<hkpCollidable*>(primId);
    const hkpShape* shape = collidable->m_shape;
    if (!shape) {
        return;
    }
    const hkTransform& transform = *collidable->m_motion.pMotion;
    hkQuadReal filterLanes;
    if (m_filter) {
        alignas(16) float afLanes[4];
        for (int32_t i = 0; i < 4; ++i) {
            if ((_mm_movemask_ps(mask.m_mask) & axisMaskBits_10C92E0[i]) &&
                m_filter->isCollisionEnabled(m_input[i], *collidable).m_bool) {
                afLanes[i] = 1.0f;
            }
            else {
                afLanes[i] = -1.0f;
            }
        }
        filterLanes = _mm_load_ps(afLanes);
    }
    else {
        filterLanes = _mm_set1_ps(1.0f);
    }
    hkVector4Comparison activeMask;
    activeMask.m_mask = _mm_cmple_ps(_mm_setzero_ps(), filterLanes);
    const hkpWorldRayCastInput* input = m_input;
    hkVector4 from[4];
    hkVector4 to[4];
    for (int32_t i = 0; i < 4; ++i) {
        hkVector4 rel;
        rel.setSub4(input[i].m_from, transform.m_translation);
        // EXE-INLINED 0xC7F690
        from[i]._setRotatedInverseDir(transform.m_rotation, rel);
        rel.setSub4(input[i].m_to, transform.m_translation);
        // EXE-INLINED 0xC7F690
        to[i]._setRotatedInverseDir(transform.m_rotation, rel);
    }
    hkpShapeRayBundleCastInput bundleInput;
    hkQuadReal from0 = from[0].m_quad;
    hkQuadReal from1 = from[1].m_quad;
    hkQuadReal from2 = from[2].m_quad;
    hkQuadReal from3 = from[3].m_quad;
    _MM_TRANSPOSE4_PS(from0, from1, from2, from3);
    bundleInput.m_from[0].m_quad = from0;
    bundleInput.m_from[1].m_quad = from1;
    bundleInput.m_from[2].m_quad = from2;
    hkQuadReal to0 = to[0].m_quad;
    hkQuadReal to1 = to[1].m_quad;
    hkQuadReal to2 = to[2].m_quad;
    hkQuadReal to3 = to[3].m_quad;
    _MM_TRANSPOSE4_PS(to0, to1, to2, to3);
    bundleInput.m_to[0].m_quad = to0;
    bundleInput.m_to[1].m_quad = to1;
    bundleInput.m_to[2].m_quad = to2;
    bundleInput.m_filterInfo = input->m_filterInfo;
    bundleInput.m_rayShapeCollectionFilter = m_collectionFilter;
    FNVD_INLINED_LOCAL(hkpShapeRayBundleCastOutput, bundleOutput, std::nullopt, std::nullopt);
    // EXE-INLINED 0xD98390
    for (int32_t i = 0; i < 4; ++i) {
        bundleOutput.m_outputs[i].m_hitFraction = 1.0f;
        bundleOutput.m_outputs[i].m_shapeKeyIndex = 0;
        bundleOutput.m_outputs[i].m_extraInfo = -1;
        bundleOutput.m_outputs[i].m_shapeKeys[0] = -1;
    }
    // The lanes the shape hit: unused.
    hkVector4Comparison hitLanes;
    shape->castRayBundleImpl(&hitLanes, bundleInput, bundleOutput, activeMask);
    for (int32_t i = 0; i < 4; ++i) {
        const hkpShapeRayCastOutput& rayOutput = bundleOutput.m_outputs[i];
        if (rayOutput.m_hitFraction < 1.0f && m_output[i].m_hitFraction > rayOutput.m_hitFraction) {
            hkpWorldRayCastOutput& result = m_output[i];
            // EXE-INLINED 0x9C1AA0
            result.m_normal.m_quad = rayOutput.m_normal.m_quad;
            result.m_hitFraction = rayOutput.m_hitFraction;
            result.m_extraInfo = rayOutput.m_extraInfo;
            result.m_pad[0] = rayOutput.m_pad[0];
            result.m_pad[1] = rayOutput.m_pad[1];
            for (int32_t k = 0; k < 8; ++k) {
                result.m_shapeKeys[k] = rayOutput.m_shapeKeys[k];
            }
            result.m_shapeKeyIndex = rayOutput.m_shapeKeyIndex;
            m_output[i].m_rootCollidable = collidable;
            const hkQuadReal normal = rayOutput.m_normal.m_quad;
            // EXE-INLINED 0xC7F700
            m_output[i].m_normal.m_quad = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(normal, normal, 0x55), transform.m_rotation.m_col1.m_quad),
                    _mm_mul_ps(_mm_shuffle_ps(normal, normal, 0x00), transform.m_rotation.m_col0.m_quad)
                ),
                _mm_mul_ps(_mm_shuffle_ps(normal, normal, 0xAA), transform.m_rotation.m_col2.m_quad)
            );
        }
    }
}

void hkKdTreeCollidableRayBundleCaster::getHitFractions(hkVector4& hitFractions) const {
    hkpWorldRayCastOutput* m_output = this->m_output;
    hitFractions.m_quad[0] = m_output->m_hitFraction;
    hitFractions.m_quad[1] = m_output[1].m_hitFraction;
    hitFractions.m_quad[2] = m_output[2].m_hitFraction;
    hitFractions.m_quad[3] = m_output[3].m_hitFraction;
}

void hkKdTreeCollidableRayBundleCaster::operator delete(void* apMemory) {
    if (apMemory) {
        hkMemoryRouter::getInstance().heap().blockFree(apMemory, sizeof(hkKdTreeCollidableRayBundleCaster));
    }
}

hkpKdTreeCollidableRaycaster::~hkpKdTreeCollidableRaycaster() {}

// The SDK's addMidPhaseHit: the primitive is a collidable. Unless it has no shape or m_filter rejects it, the ray is
// taken into the collidable's space and cast against its shape: into m_collector when there is one (no hit is
// reported back), else into m_output, whose hit then gets the collidable and its normal turned back to world space.
hkBool* hkpKdTreeCollidableRaycaster::addMidPhaseHit(hkBool* arResult, hkPrimitiveId primitiveId) {
    // The kd tree keeps each collidable's address as an untyped hkPrimitiveId (hkUlong); the SDK casts it back here.
    hkpCollidable* collidable = reinterpret_cast<hkpCollidable*>(primitiveId);
    const hkpShape* shape = collidable->m_shape;
    if (!shape || (m_filter && !m_filter->isCollisionEnabled(*m_input, *collidable).m_bool)) {
        arResult->m_bool = false;
        return arResult;
    }
    const hkTransform& transform = *collidable->m_motion.pMotion;
    const hkpWorldRayCastInput* input = m_input;
    FNVD_INLINED_LOCAL(hkpShapeRayCastInput, shapeInput, std::nullopt, std::nullopt);
    // EXE-INLINED 0x632CA0
    shapeInput.m_filterInfo = 0;
    shapeInput.m_rayShapeCollectionFilter = nullptr;
    hkVector4 from;
    from.setSub4(input->m_from, transform.m_translation);
    // EXE-INLINED 0xC7F690
    shapeInput.m_from._setRotatedInverseDir(transform.m_rotation, from);
    hkVector4 to;
    to.setSub4(input->m_to, transform.m_translation);
    // EXE-INLINED 0xC7F690
    shapeInput.m_to._setRotatedInverseDir(transform.m_rotation, to);
    shapeInput.m_filterInfo = input->m_filterInfo;
    shapeInput.m_rayShapeCollectionFilter = m_collectionFilter;
    if (m_collector) {
        shape->castRayWithCollectorImpl(shapeInput, *collidable, *m_collector);
        arResult->m_bool = false;
        return arResult;
    }
    if (!shape->castRay(shapeInput, *m_output).m_bool) {
        arResult->m_bool = false;
        return arResult;
    }
    m_output->m_rootCollidable = collidable;
    hkpWorldRayCastOutput* output = m_output;
    const hkQuadReal normal = output->m_normal.m_quad;
    // EXE-INLINED 0xC7F700
    output->m_normal.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(normal, normal, 0x00), transform.m_rotation.m_col0.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(normal, normal, 0x55), transform.m_rotation.m_col1.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(normal, normal, 0xAA), transform.m_rotation.m_col2.m_quad)
    );
    arResult->m_bool = true;
    return arResult;
}

hkBool* hkpKdTreeCollidableRaycaster::hasHit(hkBool* arResult) {
    arResult->m_bool = m_output->m_rootCollidable != nullptr;
    return arResult;
}

float hkpKdTreeCollidableRaycaster::getHitFraction() {
    return m_output->m_hitFraction;
}

// The SDK's castRayIterative: this caster collects one ray's hits into output while the kd tree utility walks the
// tree from input's start to its end; the result is whether output has a hit (hasHit, called through the vtable).
hkBool* hkpKdTreeCollidableRaycaster::castRayIterative(
    hkBool* apResult,
    const hkKdTree* tree,
    const hkpWorldRayCastInput& input,
    hkpWorldRayCastOutput& output
) {
    HK_TIMER_BEGIN("KdTreeCastRayIter", nullptr);
    m_input = &input;
    m_output = &output;
    hkBool kCastResult;
    hkKdTreeUtils::castRayIterative(&kCastResult, tree, input.m_from, input.m_to, this);
    hkKdTreeCastCollector* pCollector = this;
    pCollector->hasHit(apResult);
    HK_TIMER_END();
    return apResult;
}

void hkpKdTreeCollidableRaycaster::operator delete(void* apMemory) {
    if (apMemory) {
        hkMemoryRouter::getInstance().heap().blockFree(apMemory, sizeof(hkpKdTreeCollidableRaycaster));
    }
}
