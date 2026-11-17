#include "Havok/Source/Physics/Collide/Query/Collector/AabbTreeCollector/hkpAabbTreeWorldLinearCaster.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgentConfig.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"

#include <xmmintrin.h>

// A leaf's user data is its collidable; the cast collector's early-out distance bounds the rest of the query.
template <class BOUNDING_VOLUME>
float hkpAabbTreeWorldLinearCaster<BOUNDING_VOLUME>::addLeaf(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* leaf
) {
    testCollidable(static_cast<const hkpCollidable*>(leaf->m_leafData.m_userData));
    return m_castCollector->m_earlyOutDistance;
}

template <class BOUNDING_VOLUME>
float hkpAabbTreeWorldLinearCaster<BOUNDING_VOLUME>::addLeaf(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* leaf
) {
    testCollidable(static_cast<const hkpCollidable*>(leaf->m_leafData.m_userData));
    return m_castCollector->m_earlyOutDistance;
}

// Casts collidable A against collB through the dispatcher's linear cast function for their shape types, unless
// collB has no shape, is A itself or the filter keeps the pair apart.
template <class BOUNDING_VOLUME>
void hkpAabbTreeWorldLinearCaster<BOUNDING_VOLUME>::testCollidable(const hkpCollidable* collB) {
    const hkpShape* shapeB = collB->m_shape;
    if (!shapeB || m_collidableA == collB) {
        return;
    }
    if (!m_filter->isCollisionEnabled(*m_collidableA, *collB).m_bool) {
        return;
    }
    hkpCollisionDispatcher* dispatcher = m_shapeInput.m_dispatcher;
    dispatcher->m_agent2Func[dispatcher->m_agent2Types[m_typeA][shapeB->m_type]].m_linearCastFunc(
        *m_collidableA, *collB, m_shapeInput, *m_castCollector, m_startPointCollector
    );
}

// Sets the caster up for collA's cast along input, then sweeps collA's AABB (widened by the start point tolerance)
// from its centre along the path through the tree, which hands each leaf it reaches to addLeaf.
template <class BOUNDING_VOLUME>
void hkpAabbTreeWorldLinearCaster<BOUNDING_VOLUME>::linearCast(
    const hkAabbTree<BOUNDING_VOLUME, uint16_t>* tree,
    const hkpCollidable* collA,
    const hkpLinearCastInput& input,
    const hkpCollidableCollidableFilter* filter,
    const hkpCollisionInput& collInput,
    hkpCollisionAgentConfig* config,
    hkpCdPointCollector& castCollector,
    hkpCdPointCollector* startPointCollector
) {
    m_castCollector = &castCollector;
    m_filter = filter;
    m_startPointCollector = startPointCollector;
    m_collidableA = collA;
    m_input = &input;
    m_typeA = static_cast<hkpShapeType>(collA->m_shape->m_type.m_storage);
    static_cast<hkpCollisionInput&>(m_shapeInput) = collInput;
    m_shapeInput.m_config = config;
    m_shapeInput.m_path.setSub4(input.m_to, collA->m_motion.pState->m_transform.m_translation);
    hkVector4 lengthSquared;
    lengthSquared.setMul4(m_shapeInput.m_path, m_shapeInput.m_path);
    m_shapeInput.m_cachedPathLength =
        _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(lengthSquared(2) + (lengthSquared(1) + lengthSquared(0)))));
    m_shapeInput.m_tolerance = input.m_startPointTolerance;
    m_shapeInput.m_maxExtraPenetration = input.m_maxExtraPenetration;
    hkAabb aabb;
    collA->m_shape->getAabbImpl(collA->m_motion.pState->m_transform, input.m_startPointTolerance, aabb);
    const __m128 kHalf = _mm_set1_ps(0.5f);
    hkAabbTreeData::CastVolumeInput castInput;
    castInput.m_from.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), kHalf), aabb.m_min.m_quad), _mm_mul_ps(aabb.m_max.m_quad, kHalf)
    );
    castInput.m_to.setAdd4(castInput.m_from, m_shapeInput.m_path);
    castInput.m_halfExtents.m_quad = _mm_mul_ps(kHalf, _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad));
    hkAabbTreeQueryUtils<BOUNDING_VOLUME, uint16_t>::castAabb(
        tree, aabb, castInput, static_cast<hkAabbTreeQueryUtils<BOUNDING_VOLUME, uint16_t>::Collector*>(this),
        input.m_maxExtraPenetration, nullptr, 0
    );
}

// The only instantiation, the world's 16-bit tree caster.
template class hkpAabbTreeWorldLinearCaster<hkAabbTreeData::hkAabbTreeAabb16>;
