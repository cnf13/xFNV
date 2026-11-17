#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"

#include <xmmintrin.h>

using AabbTreeQueryUtils32 = hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>;
using AabbTreeQueryTreeCollisionFn = int32_t (*)(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>*,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>*,
    AabbTreeQueryUtils32::TreeTreeCollector*
);

// A depth-first walk from the root with a local stack: a root leaf goes to the collector at once (its result unused);
// otherwise each inner node's children whose boxes lie nearer to point than the closest distance so far are pushed,
// the nearer one on top, and each leaf popped goes to the collector, whose result becomes the closest distance; a
// zero ends the walk.
void queryClosestPoint(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* a1,
    const hkVector4& point,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* a3
) {
    using Tree = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>;
    using Node = Tree::Node;
    const Node* root = a1->m_rootNode;
    if (!root) {
        return;
    }
    float closest = 3.40282e+38f;
    using NodeStack = hkInplaceArray<const Node*, 128, hkContainerHeapAllocator>;
    FNVD_INLINED_LOCAL(NodeStack, stack, std::nullopt, std::nullopt);
    // EXE-INLINED none: hkInplaceArray<const Node*, 128>::hkInplaceArray
    stack.m_data = stack.m_storage;
    stack.m_size = 0;
    stack.m_capacityAndFlags = 128 | hkArrayBase<const Node*>::DONT_DEALLOCATE_FLAG;
    if (root->isLeaf()) {
        a3->addLeaf(root);
    }
    else {
        // EXE-INLINED none: hkArray<const Node*>::pushBack
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (stack.m_size == stack.getCapacity()) {
            hkArrayUtil::_reserveMore(router.heap(), &stack, sizeof(const Node*));
        }
        stack.m_data[stack.m_size] = root;
        ++stack.m_size;
        const Tree::NodeManager& nodeManager = a1->m_nodeManager;
        do {
            const Node* node = stack.m_data[--stack.m_size];
            if (node->isLeaf()) {
                closest = a3->addLeaf(node);
                if (closest == 0.0f) {
                    break;
                }
                continue;
            }
            const Node* left = nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
            const Node* right = nodeManager.getNode(node->m_nodeData.m_childIdx[1]);
            hkAabb leftAabb;
            leftAabb.m_min = left->m_volume.m_min;
            leftAabb.m_max = left->m_volume.m_max;
            hkAabb rightAabb;
            rightAabb.m_min = right->m_volume.m_min;
            rightAabb.m_max = right->m_volume.m_max;
            const float leftDistance = hkAabbUtil::distanceSquared(leftAabb, point);
            const float rightDistance = hkAabbUtil::distanceSquared(rightAabb, point);
            const int32_t nearer = ((leftDistance < closest ? 1 : 0) + (rightDistance < closest ? 2 : 0)) & 3;
            // The children to push, the one to visit first last.
            const Node* pushed[2];
            int32_t numPushed = 0;
            if (nearer == 1) {
                pushed[numPushed++] = left;
            }
            else if (nearer == 2) {
                pushed[numPushed++] = right;
            }
            else if (nearer == 3) {
                if (rightDistance > leftDistance) {
                    pushed[numPushed++] = right;
                    pushed[numPushed++] = left;
                }
                else {
                    pushed[numPushed++] = left;
                    pushed[numPushed++] = right;
                }
            }
            for (int32_t i = 0; i < numPushed; ++i) {
                // EXE-INLINED none: hkArray<const Node*>::pushBack
                hkMemoryRouter& pushRouter = hkMemoryRouter::getInstance();
                if (stack.m_size == stack.getCapacity()) {
                    hkArrayUtil::_reserveMore(pushRouter.heap(), &stack, sizeof(const Node*));
                }
                stack.m_data[stack.m_size] = pushed[i];
                ++stack.m_size;
            }
        }
        while (stack.m_size != 0);
    }
    // EXE-INLINED none: hkInplaceArray<const Node*, 128>::~hkInplaceArray
    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
    stack.m_size = 0;
    if ((stack.m_capacityAndFlags & hkArrayBase<const Node*>::DONT_DEALLOCATE_FLAG) == 0) {
        heap._bufFree<const Node*>(stack.m_data, stack.getCapacity());
    }
}

// Walks the 16-bit tree from its root, descending into each inner node whose box overlaps aabb in x, y and z, and
// moving on to the node's skip node otherwise. Each overlapping leaf goes to the one output given, in this order of
// preference: collector (the walk stops when addLeaf returns 0), else the write iterator (the leaf's data is written,
// then the walk stops once the count before it has reached hitCapacity), else hitArray, grown on the heap. With no
// output nothing is walked. Returns the leaves counted.
int32_t queryAabbInternal(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* tree,
    const hkAabb& aabb,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector* collector,
    hkArray<void*>* hitArray,
    void*** hitIterator,
    int32_t hitCapacity
) {
    using Tree = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>;
    const Tree::Node* node = tree->m_rootNode;
    if (!node) {
        return 0;
    }
    int32_t numHits = 0;
    if (!collector && !hitIterator && !hitArray) {
        return numHits;
    }
    for (;;) {
        // restoreAabb: each bound back to 31 bits as a float (exact: 16 significant bits), w 0.
        const __m128 quantMax = _mm_setr_ps(
            static_cast<float>(node->m_volume.m_max[0] << 15), static_cast<float>(node->m_volume.m_max[1] << 15),
            static_cast<float>(node->m_volume.m_max[2] << 15), 0.0f
        );
        const __m128 quantMin = _mm_setr_ps(
            static_cast<float>(node->m_volume.m_min[0] << 15), static_cast<float>(node->m_volume.m_min[1] << 15),
            static_cast<float>(node->m_volume.m_min[2] << 15), 0.0f
        );
        const __m128 nodeMin =
            _mm_sub_ps(_mm_mul_ps(quantMin, tree->m_bitScaleInv.m_quad), tree->m_bitOffsetLow.m_quad);
        const __m128 nodeMax =
            _mm_sub_ps(_mm_mul_ps(tree->m_bitScaleInv.m_quad, quantMax), tree->m_bitOffsetLow.m_quad);
        const uint16_t skipIdx = node->m_skipNodeIdx;
        const Tree::Node* next = (skipIdx == Tree::s_invalidIdx) ? nullptr : tree->m_nodeManager.getNode(skipIdx);
        if ((_mm_movemask_ps(
                 _mm_and_ps(_mm_cmple_ps(nodeMin, aabb.m_max.m_quad), _mm_cmple_ps(aabb.m_min.m_quad, nodeMax))
             ) &
             7) == 7) {
            if (!node->isLeaf()) {
                node = tree->m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
                continue;
            }
            if (collector) {
                if (collector->addLeaf(node) == 0.0f) {
                    return numHits;
                }
            }
            else if (hitIterator) {
                *(*hitIterator)++ = node->m_leafData.m_userData;
                if (numHits >= hitCapacity) {
                    return numHits;
                }
            }
            else {
                // hitArray is the SDK's hkArray<void*>.
                void* userData = node->m_leafData.m_userData;
                hkMemoryRouter& router = hkMemoryRouter::getInstance();
                if (hitArray->m_size == (hitArray->m_capacityAndFlags & hkArrayBase<void*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(router.heap(), hitArray, sizeof(void*));
                }
                hitArray->m_data[hitArray->m_size] = userData;
                ++hitArray->m_size;
            }
            ++numHits;
        }
        node = next;
        if (!node) {
            return numHits;
        }
    }
}

// 0 when b lies in a on x, y and z; else b's offset to the nearest point of the box, squared, its lanes summed as dot3
// does (y + x, then z + that).
float hkAabbUtil::distanceSquared(const hkAabb& a, const hkVector4& b) {
    const __m128 kMax = a.m_max.m_quad;
    if ((_mm_movemask_ps(_mm_and_ps(_mm_cmple_ps(a.m_min.m_quad, b.m_quad), _mm_cmple_ps(b.m_quad, kMax))) & 7) == 7) {
        return 0.0f;
    }
    const __m128 kOffset = _mm_sub_ps(_mm_max_ps(_mm_min_ps(b.m_quad, kMax), a.m_min.m_quad), b.m_quad);
    const __m128 kSquared = _mm_mul_ps(kOffset, kOffset);
    return _mm_cvtss_f32(_mm_add_ss(
        _mm_shuffle_ps(kSquared, kSquared, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSquared, kSquared, 0x55), kSquared)
    ));
}
