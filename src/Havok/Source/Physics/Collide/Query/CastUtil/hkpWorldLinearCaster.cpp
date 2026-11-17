#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldLinearCaster.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <xmmintrin.h>

// The SDK's body: a collidable other than the cast one, with a shape and let through by the filter, is cast against
// with the dispatcher's linear cast function for the two shape types. The exe inlines getOwner.
float hkpWorldLinearCaster::addBroadPhaseHandle(const hkpBroadPhaseHandle* broadPhaseHandle, int32_t castIndex) {
	const hkpTypedBroadPhaseHandle* pHandle = static_cast<const hkpTypedBroadPhaseHandle*>(broadPhaseHandle);
	// The owner sits m_ownerOffset bytes from the handle (getOwner: a byte offset over untyped memory).
	// EXE-INLINED 0x9B2C90
	const hkpCollidable* col = static_cast<const hkpCollidable*>(static_cast<
		const void*>(static_cast<const char*>(static_cast<const void*>(pHandle)) + pHandle->m_ownerOffset));
	const hkpShape* shapeB = col->m_shape;
	if (!shapeB) {
		return m_castCollector->m_earlyOutDistance;
	}
	if (col == m_collidableA) {
		return m_castCollector->m_earlyOutDistance;
	}
	if (m_filter->isCollisionEnabled(*m_collidableA, *col).m_bool) {
		hkpShapeType typeB = static_cast<hkpShapeType>(shapeB->m_type.m_storage);
		hkpCollisionDispatcher::LinearCastFunc linearCastFunc =
			m_shapeInput.m_dispatcher->getLinearCastFunc(m_typeA, typeB);
		linearCastFunc(*m_collidableA, *col, m_shapeInput, *m_castCollector, m_startPointCollector);
	}
	return m_castCollector->m_earlyOutDistance;
}

// The SDK's body: the caster's state and collision input set, the path from the collidable's position to the cast's
// end with its length and the start point tolerance (the SDK's setPathAndTolerance, inlined), then the collidable's
// AABB cast along the path through the broad phase: its centre (the SDK's setInterpolate4 at one half) from there
// to the end of the path, with its half extents. The exe writes the inlined vector helpers out, so src does too.
// The float copies of the tolerance (0xCFAFAA), the path length (0xCFAF9C) and m_maxExtraPenetration (0xCFAFF2)
// are x87 copies in the exe.
void hkpWorldLinearCaster::linearCast(const hkpBroadPhase& broadphase, const hkpCollidable* collA,
	const hkpLinearCastInput& input, const hkpCollidableCollidableFilter* filter, const hkpCollisionInput& collInput,
	hkpCollisionAgentConfig* config, hkpBroadPhaseAabbCache* m_broadPhaseCache, hkpCdPointCollector& castCollector,
	hkpCdPointCollector* startPointCollector) {
	m_castCollector = &castCollector;
	m_filter = filter;
	m_startPointCollector = startPointCollector;
	m_collidableA = collA;
	m_input = &input;
	m_typeA = static_cast<hkpShapeType>(collA->m_shape->m_type.m_storage);
	static_cast<hkpCollisionInput&>(m_shapeInput) = collInput;
	m_shapeInput.m_config = config;

	float startPointTolerance = input.m_startPointTolerance;
	// The hkpCastAabbInput constructor clears the cache pointer.
	hkpBroadPhase::hkpCastAabbInput ci;
	ci.m_aabbCacheInfo = nullptr;
	hkVector4 path;
	path.m_quad = _mm_sub_ps(input.m_to.m_quad, collA->m_motion.pState->m_transform.m_translation.m_quad);
	m_shapeInput.m_path.m_quad = path.m_quad;
	// length3: sqrtss of z + (y + x).
	__m128 len2 = _mm_mul_ps(path.m_quad, path.m_quad);
	m_shapeInput.m_cachedPathLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(len2, len2, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(len2, len2, 0x55), len2))));
	m_shapeInput.m_tolerance = startPointTolerance;

	FNVD_INLINED_LOCAL(hkAabb, aabb, std::nullopt, std::nullopt);
	collA->m_shape->getAabb(collA->m_motion.pState->m_transform, input.m_startPointTolerance, aabb);
	__m128 half = _mm_set1_ps(0.5f);
	ci.m_from.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), half), aabb.m_min.m_quad),
		_mm_mul_ps(aabb.m_max.m_quad, half));
	m_shapeInput.m_maxExtraPenetration = input.m_maxExtraPenetration;
	ci.m_to.m_quad = _mm_add_ps(ci.m_from.m_quad, path.m_quad);
	ci.m_aabbCacheInfo = m_broadPhaseCache;
	ci.m_halfExtents.m_quad = _mm_mul_ps(half, _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad));
	broadphase.castAabb(ci, *this);
}
