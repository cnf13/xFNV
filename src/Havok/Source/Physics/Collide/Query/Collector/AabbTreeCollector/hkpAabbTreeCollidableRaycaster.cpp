#include "Havok/Source/Physics/Collide/Query/Collector/AabbTreeCollector/hkpAabbTreeCollidableRaycaster.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

using hkAabbTreeCollector16 = hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector;

hkpAabbTreeCollidableRaycaster::hkpAabbTreeCollidableRaycaster()
  : m_input(nullptr), m_filter(nullptr), m_collectionFilter(nullptr), m_output(nullptr), m_collector(nullptr),
    m_earlyOutHitFraction(1.0f) {}

hkpAabbTreeCollidableRaycaster::~hkpAabbTreeCollidableRaycaster() {}

// Casts the ray against the collidable a float-precision leaf holds, then returns the fraction the tree walk may stop
// at, read from m_earlyOutHitFraction after the cast.
float hkpAabbTreeCollidableRaycaster::addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* leaf) {
    testCollidable(static_cast<const hkpCollidable*>(leaf->m_leafData.m_userData));
    return m_earlyOutHitFraction;
}

// The same for a 16-bit leaf.
float hkpAabbTreeCollidableRaycaster::addLeaf(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* leaf
) {
    testCollidable(static_cast<const hkpCollidable*>(leaf->m_leafData.m_userData));
    return m_earlyOutHitFraction;
}

// Casts the ray against one collidable, filtered by m_filter, reporting hits to m_collector.
void hkpAabbTreeCollidableRaycaster::testCollidable(const hkpCollidable* col) {
    hkBool v20;
    hkpShapeRayCastInput input_;
    const hkpShape* m_shape = col->m_shape;
    if (m_shape) {
        const hkpRayCollidableFilter* m_filter = this->m_filter;
        if (!m_filter || (::new (&v20) hkBool(m_filter->isCollisionEnabled(*this->m_input, *col)))->m_bool) {
            __m128* p_m_quad = &col->m_motion.pState->m_transform.m_rotation.m_col0.m_quad;
            const hkpWorldRayCastInput* m_input = this->m_input;
            input_.m_filterInfo = 0;
            input_.m_rayShapeCollectionFilter = nullptr;
            __m128 v6 = p_m_quad[1];
            __m128 v7 = p_m_quad[2];
            __m128 v8 = _mm_sub_ps(m_input->m_from.m_quad, p_m_quad[3]);
            __m128 v9 = _mm_unpacklo_ps(*p_m_quad, v6);
            __m128 v10 = _mm_movelh_ps(v9, v7);
            input_.m_from.m_quad = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v8, v8, 85), _mm_shuffle_ps(_mm_movehl_ps(v10, v9), v7, 212)),
                    _mm_mul_ps(_mm_shuffle_ps(v8, v8, 0), v10)
                ),
                _mm_mul_ps(_mm_shuffle_ps(_mm_unpackhi_ps(*p_m_quad, v6), v7, 228), _mm_shuffle_ps(v8, v8, 170))
            );
            __m128 v11 = p_m_quad[1];
            __m128 v12 = p_m_quad[2];
            __m128 v13 = _mm_sub_ps(m_input->m_to.m_quad, p_m_quad[3]);
            __m128 v14 = _mm_unpacklo_ps(*p_m_quad, v11);
            __m128 v15 = _mm_movelh_ps(v14, v12);
            input_.m_to.m_quad = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v13, v13, 85), _mm_shuffle_ps(_mm_movehl_ps(v15, v14), v12, 212)),
                    _mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), v15)
                ),
                _mm_mul_ps(_mm_shuffle_ps(_mm_unpackhi_ps(*p_m_quad, v11), v12, 228), _mm_shuffle_ps(v13, v13, 170))
            );
            uint32_t m_filterInfo = m_input->m_filterInfo;
            input_.m_rayShapeCollectionFilter = m_collectionFilter;
            hkpRayHitCollector* m_collector = this->m_collector;
            input_.m_filterInfo = m_filterInfo;
            if (m_collector) {
                m_shape->castRayWithCollectorImpl(input_, *col, *m_collector);
                m_earlyOutHitFraction = this->m_collector->m_earlyOutHitFraction;
            }
            else {
                v20 = m_shape->castRayImpl(input_, *this->m_output);
                if (v20.m_bool) {
                    this->m_output->m_rootCollidable = const_cast<hkpCollidable*>(col);
                    hkpWorldRayCastOutput* m_output = this->m_output;
                    m_output->m_normal.m_quad = _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(
                                _mm_shuffle_ps(m_output->m_normal.m_quad, m_output->m_normal.m_quad, 85), p_m_quad[1]
                            ),
                            _mm_mul_ps(
                                _mm_shuffle_ps(m_output->m_normal.m_quad, m_output->m_normal.m_quad, 0), *p_m_quad
                            )
                        ),
                        _mm_mul_ps(
                            _mm_shuffle_ps(m_output->m_normal.m_quad, m_output->m_normal.m_quad, 170), p_m_quad[2]
                        )
                    );
                    m_earlyOutHitFraction = this->m_output->m_hitFraction;
                }
            }
        }
    }
}

void hkpAabbTreeCollidableRaycaster::operator delete(void* apMemory) {
    if (apMemory) {
        hkMemoryRouter::getInstance().heap().blockFree(apMemory, sizeof(hkpAabbTreeCollidableRaycaster));
    }
}
