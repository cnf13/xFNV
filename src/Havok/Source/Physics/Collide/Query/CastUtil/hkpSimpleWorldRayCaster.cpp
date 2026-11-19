#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpSimpleWorldRayCaster.h"

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
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <xmmintrin.h>

// When the filter lets the ray through, the ray is taken into the collidable's space and cast against its shape; on a
// hit, the root collidable and the normal are turned back into world space.
float hkpSimpleWorldRayCaster::addBroadPhaseHandle(const hkpBroadPhaseHandle* broadPhaseHandle, int32_t castIndex) {
    const hkpTypedBroadPhaseHandle* pHandle = static_cast<const hkpTypedBroadPhaseHandle*>(broadPhaseHandle);
    hkpCollidable* col = static_cast<hkpCollidable*>(const_cast<hkpTypedBroadPhaseHandle*>(pHandle)->getOwner());
    const hkpShape* shape = col->m_shape;
    if (shape) {
        if (!m_filter->isCollisionEnabled(m_input[castIndex], *col).m_bool) {
            return m_result->m_hitFraction;
        }
        const hkTransform& trans = col->m_motion.pState->m_transform;
        const hkpWorldRayCastInput& input = m_input[castIndex];
        m_shapeInput.m_from.setTransformedInversePos(trans, input.m_from);
        m_shapeInput.m_to.setTransformedInversePos(trans, input.m_to);
        m_shapeInput.m_filterInfo = input.m_filterInfo;
        if (!shape->castRay(m_shapeInput, *m_result).m_bool) {
            return m_result->m_hitFraction;
        }
        m_result->m_rootCollidable = col;
        hkpWorldRayCastOutput* pResult = m_result;
        pResult->m_normal.setRotatedDir(trans.m_rotation, pResult->m_normal);
    }
    return m_result->m_hitFraction;
}

// The SDK's body: the caster's input, filters and output set, then one ray cast through the broad phase with the
// caller's AABB cache, between the timer's begin and end.
void hkpSimpleWorldRayCaster::castRay(
    const hkpBroadPhase& broadphase,
    const hkpWorldRayCastInput& input,
    const hkpCollisionFilter* filter,
    const char* cache,
    hkpWorldRayCastOutput& output
) {
    HK_TIMER_BEGIN("RayCstCchSim", nullptr);
    m_input = &input;
    m_result = &output;
    m_filter = static_cast<const hkpRayCollidableFilter*>(filter);
    if (input.m_enableShapeCollectionFilter.m_bool) {
        m_shapeInput.m_rayShapeCollectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(filter);
    }
    else {
        m_shapeInput.m_rayShapeCollectionFilter = nullptr;
    }
    hkpBroadPhase::hkpCastRayInput rayInput;
    rayInput.m_from.m_quad = input.m_from.m_quad;
    rayInput.m_numCasts = 1;
    rayInput.m_toBase = &input.m_to;
    rayInput.m_toStriding = sizeof(hkVector4);
    rayInput.m_aabbCacheInfo = cache;
    broadphase.castRay(rayInput, this, 0);
    HK_TIMER_END();
}

// The SDK's castRayGroup: one AABB around every ray, the broad phase's AABB cache for it in stack memory, then each
// ray cast through that cache into its output.
void hkpSimpleWorldRayCaster::castRayGroup(
    const hkpBroadPhase& broadphase,
    const hkpWorldRayCastInput* inputArray,
    int32_t numRays,
    const hkpCollisionFilter* filter,
    hkpWorldRayCastOutput* outputs
) {
    HK_TIMER_BEGIN("RayCstGrpSim", nullptr);
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
    hkpWorldRayCastOutput* pOutput = outputs;
    for (int32_t i = numRays - 1; i >= 0; --i) {
        castRay(broadphase, *pInput, filter, pCache, *pOutput);
        ++pOutput;
        ++pInput;
    }
    hkDeallocateStack(pCache);
    HK_TIMER_END();
}

// castRay without an AABB cache: the caster's input, filters and output set, then one ray cast through the broad
// phase, between the timer's begin and end.
void hkpSimpleWorldRayCaster::castRay_0(
    const hkpBroadPhase& broadphase,
    const hkpWorldRayCastInput& input,
    const hkpCollisionFilter* filter,
    hkpWorldRayCastOutput& output
) {
    HK_TIMER_BEGIN("RayCastSimpl", nullptr);
    m_input = &input;
    m_result = &output;
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
    rayInput.m_numCasts = 1;
    rayInput.m_toStriding = sizeof(hkVector4);
    rayInput.m_aabbCacheInfo = nullptr;
    broadphase.castRay(rayInput, this, 0);
    HK_TIMER_END();
}

// The SDK's castRaysFromSinglePoint: numRays rays from inputArray[0]'s start to each input's end, cast through the
// broad phase in one call with the caller's AABB cache; the results go to outputs.
void hkpSimpleWorldRayCaster::castRaysFromSinglePoint(
    const hkpBroadPhase& broadphase,
    const hkpWorldRayCastInput* inputArray,
    int32_t numRays,
    const hkpCollisionFilter* filter,
    const char* cache,
    hkpWorldRayCastOutput* outputs
) {
    HK_TIMER_BEGIN("RayCstFSPSim", nullptr);
    m_input = inputArray;
    m_result = outputs;
    m_filter = static_cast<const hkpRayCollidableFilter*>(filter);
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
