#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"

#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"

#include <xmmintrin.h>

hkKdTreeCinfo::hkKdTreeCinfo() : m_numPrimitives(-1) {
    m_emptyNodeAllocation.m_storage = STANDARD_EMPTY_NODES;
    m_isDistributedBuild.m_bool = 1;
    m_aabb.m_min.m_quad = _mm_set1_ps(HK_REAL_MAX);
    m_aabb.m_max.m_quad = _mm_set1_ps(-HK_REAL_MAX);
}

// The depth is -1; the counts and m_aabb are deliberately left uninitialised.
hkKdTree::hkKdTree(int32_t numNodes) : m_nodes(), m_maxDepth(-1), m_projectedEntries() {
    initNodeArray(numNodes);
}

hkKdTree::hkKdTree(const hkKdTreeCinfo& cinfo) : m_nodes(), m_projectedEntries() {
    reset(cinfo);
}

// The members free the projected entries and the nodes.
hkKdTree::~hkKdTree() {}

void hkKdTree::initNodeArray(int32_t numNodes) {
    int32_t v2 = numNodes;
    if (numNodes <= 2) {
        v2 = 2;
    }
    if (m_nodes.m_size < v2) {
        m_nodes.reserve(v2);
        int32_t m_size = m_nodes.m_size;
        int32_t v8 = v2 - m_size;
        hkKdTreeNode* v9 = &m_nodes.m_data[m_size];
        if (v8 > 0) {
            do {
                if (v9) {
                    v9->m_data.m_leaf.m_type_primId = 0x7FFFFFFF;
                    v9->m_data.m_leaf.m_numPrimitives = 0;
                }
                ++v9;
                --v8;
            }
            while (v8);
        }
        m_nodes.m_size = v2;
    }
    hkKdTreeNode* m_data = m_nodes.begin();
    if (m_nodes.begin()) {
        m_data->m_data.m_leaf.m_type_primId = 0x7FFFFFFF;
        m_data->m_data.m_leaf.m_numPrimitives = 0;
    }
    m_nodes.begin()->setEmpty();
    m_nodes.m_data[1].setEmpty();
}

// Builds the 32-bit id from the entry's two id words and stops at the first match.
void hkKdTree::invalidatePrimitive(hkPrimitiveId aId) {
    for (hkKdTreeProjectedEntry& rEntry : m_projectedEntries) {
        hkPrimitiveId id = rEntry.getPrimitiveId();
        if (id == aId) {
            rEntry.m_min[0] = 0xFFFF;
            rEntry.m_min[1] = 0xFFFF;
            rEntry.m_min[2] = 0xFFFF;
            rEntry.m_max[0] = 0;
            rEntry.m_max[1] = 0;
            rEntry.m_max[2] = 0;
            return;
        }
    }
}

// Resets the bounds, the regular nodes (twice the primitives, eight more for a distributed build), the reserved empty
// nodes (the primitives divided by the empty-node allocation, none when it is 0), the node array for both, then
// setNumPrimitives. cinfo.m_numPrimitives is read again at each use.
void hkKdTree::reset(const hkKdTreeCinfo& cinfo) {
    m_maxDepth = -1;
    m_aabb.m_min.m_quad = cinfo.m_aabb.m_min.m_quad;
    m_aabb.m_max.m_quad = cinfo.m_aabb.m_max.m_quad;
    const int32_t numRegularNodes = cinfo.m_isDistributedBuild.m_bool ? 2 * cinfo.m_numPrimitives + 8
                                                                      : 2 * cinfo.m_numPrimitives;
    m_numRegularNodes = numRegularNodes;
    const int32_t emptyNodeAllocation = cinfo.m_emptyNodeAllocation.m_storage;
    const int32_t numReservedEmptyNodes = emptyNodeAllocation ? cinfo.m_numPrimitives / emptyNodeAllocation : 0;
    m_numReservedEmptyNodes = numReservedEmptyNodes;
    initNodeArray(numRegularNodes + numReservedEmptyNodes);
    setNumPrimitives(cinfo.m_numPrimitives);
}

// Casts from apTree's root through castRayRecursive with the tree's minimum and extent (its AABB's max - min).
void castRayKdTree(
    hkKdTree* apTree,
    __m128* orig,
    __m128* dir,
    float tNear,
    float tFar,
    float originalLength,
    hkKdTreeCastCollector* collector
) {
    alignas(16) __m128 treeMin = apTree->m_aabb.m_min.m_quad;
    alignas(16) __m128 treeExtent = _mm_sub_ps(apTree->m_aabb.m_max.m_quad, apTree->m_aabb.m_min.m_quad);
    hkKdTreeNode* pRoot = apTree->m_nodes.m_data;
    castRayRecursive(
        pRoot,
        pRoot,
        apTree->m_projectedEntries.m_data,
        &treeExtent,
        &treeMin,
        orig,
        dir,
        tNear,
        tFar,
        originalLength,
        collector
    );
}
