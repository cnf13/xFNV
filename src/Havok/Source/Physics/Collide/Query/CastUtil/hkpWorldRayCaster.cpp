#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCaster.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <xmmintrin.h>

// The caster adds nothing to destroy beyond hkpBroadPhaseCastCollector.
hkpWorldRayCaster::~hkpWorldRayCaster() {}

// The handle's collidable is filtered against the ray it was reported for, the ray is taken into the collidable's space
// and cast against its shape into that ray's collector. The ray's start is taken from the first input (m_input->m_from)
// and its end from the cast's own input, as the game does.
float hkpWorldRayCaster::addBroadPhaseHandle(const hkpBroadPhaseHandle* broadPhaseHandle, int32_t castIndex) {
    const hkpCollidable* col = static_cast<const hkpCollidable*>(
        const_cast<hkpTypedBroadPhaseHandle*>(static_cast<const hkpTypedBroadPhaseHandle*>(broadPhaseHandle))
            ->getOwner()
    );
    const hkpShape* shape = col->m_shape;
    // The collector stride is a byte distance added to an untyped address (hkAddByteOffset).
    // EXE-INLINED none: hkAddByteOffset<hkpRayHitCollector>
    hkpRayHitCollector* collector = reinterpret_cast<hkpRayHitCollector*>(
        reinterpret_cast<char*>(m_collectorBase) + castIndex * m_collectorStriding
    );
    if (shape) {
        const hkpWorldRayCastInput& input = m_input[castIndex];
        if (m_filter->isCollisionEnabled(input, *col).m_bool) {
            const hkTransform& trans = col->m_motion.pState->m_transform;
            m_shapeInput.m_from.setTransformedInversePos(trans, m_input->m_from);
            m_shapeInput.m_to.setTransformedInversePos(trans, input.m_to);
            m_shapeInput.m_filterInfo = input.m_filterInfo;
            shape->castRayWithCollectorImpl(m_shapeInput, *col, *collector);
        }
    }
    return collector->m_earlyOutHitFraction;
}

// Casts one ray: this caster becomes the broad phase's cast collector for input, with filter's ray collidable and
// (when the input asks for it) shape collection filters, and the broad phase casts through cache.
void hkpWorldRayCaster::castRay(
    const hkpBroadPhase& broadphase,
    const hkpWorldRayCastInput& input,
    const hkpCollisionFilter* filter,
    const hkpBroadPhaseAabbCache* cache,
    hkpRayHitCollector& collector
) {
    HK_TIMER_BEGIN("RayCstCached", nullptr);
    m_input = &input;
    m_collectorBase = &collector;
    m_collectorStriding = 0;
    m_filter = static_cast<const hkpRayCollidableFilter*>(filter);
    if (input.m_enableShapeCollectionFilter.m_bool) {
        m_shapeInput.m_rayShapeCollectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(filter);
    }
    else {
        m_shapeInput.m_rayShapeCollectionFilter = nullptr;
    }
    hkpBroadPhase::hkpCastRayInput rayInput;
    rayInput.m_from.m_quad = input.m_from.m_quad;
    rayInput.m_toBase = &input.m_to;
    rayInput.m_aabbCacheInfo = cache;
    rayInput.m_numCasts = 1;
    rayInput.m_toStriding = sizeof(hkVector4);
    broadphase.castRay(rayInput, this, 0);
    HK_TIMER_END();
}

// The SDK's castRayGroup: one AABB around every ray, the broad phase's AABB cache for it in stack memory, then each
// ray cast through that cache into its collector, the collectors collectorStriding bytes apart.
void hkpWorldRayCaster::castRayGroup(
    const hkpBroadPhase& broadphase,
    const hkpWorldRayCastInput* inputArray,
    int32_t numRays,
    const hkpCollisionFilter* filter,
    hkpRayHitCollector* collectorBase,
    int32_t collectorStriding
) {
    HK_TIMER_BEGIN("RayCastGroup", nullptr);
    hkAabb aabb;
    __m128 vMin = _mm_min_ps(inputArray->m_from.m_quad, inputArray->m_to.m_quad);
    __m128 vMax = _mm_max_ps(inputArray->m_from.m_quad, inputArray->m_to.m_quad);
    const hkpWorldRayCastInput* pRay = inputArray + 1;
    for (int32_t i = numRays - 2; i >= 0; --i) {
        vMin = _mm_min_ps(_mm_min_ps(vMin, pRay->m_to.m_quad), pRay->m_from.m_quad);
        vMax = _mm_max_ps(_mm_max_ps(vMax, pRay->m_to.m_quad), pRay->m_from.m_quad);
        ++pRay;
    }
    aabb.m_min.m_quad = vMin;
    aabb.m_max.m_quad = vMax;
    int32_t iCacheSize = broadphase.getAabbCacheSize();
    char* pCache = hkAllocateStack<char>(iCacheSize, nullptr);
    broadphase.calcAabbCache(aabb, pCache);
    const hkpWorldRayCastInput* pInput = inputArray;
    hkpRayHitCollector* pCollector = collectorBase;
    for (int32_t i = numRays - 1; i >= 0; --i) {
        castRay(broadphase, *pInput, filter, pCache, *pCollector);
        // The collectors are collectorStriding bytes apart: a byte offset over untyped memory.
        // EXE-INLINED none: hkAddByteOffset<hkpRayHitCollector>
        pCollector = reinterpret_cast<hkpRayHitCollector*>(reinterpret_cast<char*>(pCollector) + collectorStriding);
        ++pInput;
    }
    hkDeallocateStack(pCache);
    HK_TIMER_END();
}

// The SDK's castRaysFromSinglePoint: numRays rays from inputArray[0]'s start to each input's end, cast through the
// broad phase in one call with the caller's AABB cache; ray i's hits go to the collector collectorStriding * i bytes
// past collectorBase.
void hkpWorldRayCaster::castRaysFromSinglePoint(
    const hkpBroadPhase& broadphase,
    const hkpWorldRayCastInput* inputArray,
    int32_t numRays,
    const hkpCollisionFilter* filter,
    const char* cache,
    hkpRayHitCollector* collectorBase,
    int32_t collectorStriding
) {
    HK_TIMER_BEGIN("RayCastFSP", nullptr);
    m_input = inputArray;
    m_filter = static_cast<const hkpRayCollidableFilter*>(filter);
    m_collectorBase = collectorBase;
    m_collectorStriding = collectorStriding;
    if (inputArray->m_enableShapeCollectionFilter.m_bool) {
        m_shapeInput.m_rayShapeCollectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(filter);
    }
    else {
        m_shapeInput.m_rayShapeCollectionFilter = nullptr;
    }
    hkpBroadPhase::hkpCastRayInput rayInput;
    rayInput.m_from.m_quad = inputArray->m_from.m_quad;
    rayInput.m_toBase = &inputArray->m_to;
    rayInput.m_aabbCacheInfo = cache;
    rayInput.m_numCasts = numRays;
    rayInput.m_toStriding = sizeof(hkpWorldRayCastInput);
    broadphase.castRay(rayInput, this, 0);
    HK_TIMER_END();
}
