#include "Havok/Source/Physics/Collide/Query/Collector/KdTreeCollector/hkpKdTreeWorldLinearCaster.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbCastUtils.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgentConfig.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"

#include <xmmintrin.h>

// This file's static x of hkGetOneOver65K (its function-local constant, no guard), the quad the copy below returns:
// 1 / 65535 in every lane.
static __m128 x{1.5259022e-05f, 1.5259022e-05f, 1.5259022e-05f, 1.5259022e-05f};

// This file's copy of the file-static hkGetOneOver65K. Nothing calls it.
__m128 hkGetOneOver65K_D98620() {
	return _mm_load_ps(&reinterpret_cast<float*>(&x)[0]);
}

// The k-d tree primitive id is the collidable's address: an integer the tree stores untyped, so the SDK's cast back
// to the collidable is a reinterpret_cast. A collidable with a shape, other than collidable A and passing the filter,
// is cast against through the dispatcher's linear-cast function for the pair of shape types.
hkBool* hkpKdTreeWorldLinearCaster::addMidPhaseHit(hkBool* arResult, hkPrimitiveId primitiveId) {
	const hkpCollidable* collB = reinterpret_cast<const hkpCollidable*>(primitiveId);
	const hkpShape* shapeB = collB->m_shape;
	if (shapeB && m_collidableA != collB && m_filter->isCollisionEnabled(*m_collidableA, *collB).m_bool) {
		hkpCollisionDispatcher::LinearCastFunc linearCast =
			m_shapeInput.m_dispatcher->getLinearCastFunc(static_cast<hkpShapeType>(m_typeA.m_storage),
				static_cast<hkpShapeType>(shapeB->m_type.m_storage));
		linearCast(*m_collidableA, *collB, m_shapeInput, *m_castCollector, m_startPointCollector);
		arResult->m_bool = 1;
		return arResult;
	}
	arResult->m_bool = 0;
	return arResult;
}

hkBool* hkpKdTreeWorldLinearCaster::hasHit(hkBool* arResult) {
	hkpCdPointCollector* m_castCollector = this->m_castCollector;
	bool v3 = m_castCollector->m_earlyOutDistance > 1.0;
	bool v4 = 1.0 == m_castCollector->m_earlyOutDistance;
	arResult->m_bool = !v3 && !v4;
	return arResult;
}

float hkpKdTreeWorldLinearCaster::getHitFraction() {
	return m_castCollector->m_earlyOutDistance;
}

void hkpKdTreeWorldLinearCaster::linearCast(const hkKdTree* tree, const hkpCollidable* collA,
	const hkpLinearCastInput& input, const hkpCollidableCollidableFilter* filter, const hkpCollisionInput& collInput,
	hkpCollisionAgentConfig* config, hkpCdPointCollector& castCollector, hkpCdPointCollector* startPointCollector) {
	hkVector4 v11;
	hkVector4 v13;
	hkVector4 v14;
	hkBool arResult;
	hkAabb out_;
	hkKdTreeAabbCastUtils::hkCastAabbInput castInput_12;
	m_castCollector = &castCollector;
	m_filter = filter;
	m_startPointCollector = startPointCollector;
	m_collidableA = collA;
	m_input = &input;
	m_typeA.m_storage = collA->m_shape->m_type.m_storage;
	static_cast<hkpCollisionInput&>(m_shapeInput) = collInput;
	m_shapeInput.m_config = config;
	hkMotionState* pState = collA->m_motion.pState;
	float m_startPointTolerance = input.m_startPointTolerance;
	v11.m_quad = _mm_sub_ps(input.m_to.m_quad, pState->m_transform.m_translation.m_quad);
	m_shapeInput.m_path.m_quad = v11.m_quad;
	__m128 m_quad = v11.m_quad;
	__m128 v12 = _mm_mul_ps(v11.m_quad, v11.m_quad);
	float m_cachedPathLength =
		_mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(static_cast<float>((_mm_shuffle_ps(v12, v12, 170)[0] +
			static_cast<float>(_mm_shuffle_ps(v12, v12, 85)[0] + v12[0]))))));
	m_shapeInput.m_cachedPathLength = m_cachedPathLength;
	m_shapeInput.m_tolerance = m_startPointTolerance;
	collA->m_shape->getAabbImpl(collA->m_motion.pState->m_transform, input.m_startPointTolerance, out_);
	v13.m_quad = out_.m_max.m_quad;
	v14.m_quad = out_.m_min.m_quad;
	__m128 v15 = _mm_shuffle_ps(_mm_set_ss(f0_5_), _mm_set_ss(f0_5_), 0);
	__m128 v16 = _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v15), out_.m_min.m_quad);
	__m128 v17 = _mm_mul_ps(out_.m_max.m_quad, v15);
	m_shapeInput.m_maxExtraPenetration = input.m_maxExtraPenetration;
	float aabbCheckTolerance = config->m_iterativeLinearCastEarlyOutDistance;
	castInput_12.m_from.m_quad = _mm_add_ps(v16, v17);
	castInput_12.m_to.m_quad = _mm_add_ps(castInput_12.m_from.m_quad, m_quad);
	castInput_12.m_halfExtents.m_quad = _mm_mul_ps(v15, _mm_sub_ps(v13.m_quad, v14.m_quad));
	hkKdTreeAabbCastUtils::castAabb(&arResult, tree, castInput_12, this, aabbCheckTolerance);
}
