#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Util/hkAabbTreeRaycastUtil.h"

#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Gjk/Penetration/hkpWingedEdge.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpFixedBufferRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuWorldRaycastJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Util/hkAabbTreeRaycastUtil.inl"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"

#include <xmmintrin.h>

hkAabbTreeData::RayBundle* bundleRaysWithInvDir(
    hkAabbTreeData::RayBundle* apResult,
    const hkVector4& from0,
    const hkVector4& invDir0,
    const hkVector4& from1,
    const hkVector4& invDir1,
    const hkVector4& from2,
    const hkVector4& invDir2,
    const hkVector4& from3,
    const hkVector4& invDir3,
    hkVector4Comparison::Mask mask
) {
    __m128 v11 = _mm_shuffle_ps(from2.m_quad, from3.m_quad, 68);
    __m128 v12 = _mm_shuffle_ps(from0.m_quad, from1.m_quad, 68);
    __m128 v20 = _mm_shuffle_ps(v12, v11, 221);
    __m128 v13 = _mm_shuffle_ps(v12, v11, 136);
    __m128 v14 = _mm_shuffle_ps(
        _mm_shuffle_ps(from0.m_quad, from1.m_quad, 238), _mm_shuffle_ps(from2.m_quad, from3.m_quad, 238), 136
    );
    __m128 v15 = invDir3.m_quad;
    __m128 v16 = invDir2.m_quad;
    __m128 v17 = _mm_shuffle_ps(invDir0.m_quad, invDir1.m_quad, 68);
    __m128 v18 = _mm_shuffle_ps(
        _mm_shuffle_ps(invDir0.m_quad, invDir1.m_quad, 238), _mm_shuffle_ps(invDir2.m_quad, invDir3.m_quad, 238), 136
    );
    apResult->m_from.m_data[0].m_quad = v13;
    apResult->m_from.m_data[1].m_quad = v20;
    apResult->m_from.m_data[2].m_quad = v14;
    __m128 v19 = _mm_shuffle_ps(v16, v15, 68);
    apResult->m_invDir.m_data[0].m_quad = _mm_shuffle_ps(v17, v19, 136);
    apResult->m_invDir.m_data[1].m_quad = _mm_shuffle_ps(v17, v19, 221);
    apResult->m_invDir.m_data[2].m_quad = v18;
    apResult->m_mask = mask;
    return apResult;
}
void setCollector(hkCpuWorldRayCastCollector* a1, hkpWorldRayCastCommand* a2, const hkpCollisionFilter* a3) {
    a1->m_hitFraction = 1.0f;
    a1->m_hit.m_bool = 0;
    a1->m_result = a2->m_results;
    hkpWorldRayCastOutput* m_results = a2->m_results;
    a1->m_originalInput = &a2->m_rayInput;
    a1->m_nextFreeResult = m_results;
    uint32_t m_filterInfo = a2->m_rayInput.m_filterInfo;
    a1->m_command = a2;
    a1->m_workInput.m_filterInfo = m_filterInfo;
    const hkpCollisionFilter* v5 = a2->m_rayInput.m_enableShapeCollectionFilter.m_bool != 0 ? a3 : 0;
    if (v5) {
        a1->m_workInput.m_rayShapeCollectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(v5);
    }
    else {
        a1->m_workInput.m_rayShapeCollectionFilter = 0;
    }
}

template class hkAabbTreeRayCastUtil<hkAabbTreeData::hkAabbTreeAabb, uint16_t>;
template class hkAabbTreeRayCastUtil<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>;

// Registers the fixed buffer ray hit collector's default addRayHit callback.
void registerDefaultAddRayHitFunction() {
    g_FixedBufferRayHitCollectorAddRayHitCallbackFunc = &hkpFixedBufferRayHitCollector::addRayHitImplementation;
}

void hkpWingedEdge::codeBegin() {}

void hkpWingedEdge::codeEnd() {}
