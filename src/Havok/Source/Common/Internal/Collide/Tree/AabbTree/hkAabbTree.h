#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"

#include <cstdint>
#include <stdint.h>

// A bounding volume tree whose nodes are addressed by IDX indices.
template <class BOUNDING_VOLUME, typename IDX>
class hkAabbTree {
public:
    // One tree node: an inner node holds its children's indices, a leaf holds the user's data (a collidable for the
    // world's trees).
    struct Node {
        // The top bit of m_parentIdx marks a leaf; the rest of the word is the parent's index.
        // The SDK's hkIdxType(~0) >> 1: the all-ones index is cast back to IDX before the shift, so that a 16-bit
        // IDX gives 0x7FFF (an int's ~0 shifted stays all ones).
        static const IDX s_leafMask = static_cast<IDX>(static_cast<IDX>(~static_cast<IDX>(0)) >> 1);

        // Whether this node is a leaf; tests m_parentIdx against s_leafFlag and returns a Havok hkBool32 (nonzero means
        // true).
        int32_t isLeaf() const {
            return m_parentIdx & s_leafFlag;
        }

        BOUNDING_VOLUME m_volume;

        union {
            struct {
                IDX m_childIdx[2];
            } m_nodeData;

            struct {
                // Users store and read pointers here (collidables, hkgpMesh triangles).
                void* m_userData;
            } m_leafData;
        };

        IDX m_parentIdx;
        IDX m_skipNodeIdx;

        static const IDX s_leafFlag = static_cast<IDX>(~s_leafMask);
    };

    // The tree's node storage: freed slots go on m_freeList and are handed out again before the storage grows.
    struct NodeManager {
        // The nodes the storage grows by when no slot is free.
        enum { MEM_INCREASE_SIZE = 128 };

        // The members free the free list and the node storage.
        ~NodeManager() {}

        // Grows the storage to size nodes, then points rootNode at node rootIdx again (unless that is 0x7FFF, no
        // node), since growing moves the nodes.
        void reserve(IDX size, Node*& rootNode, IDX& rootIdx);
        // Copies node into a free slot, growing the storage by 128 first when none is free; returns the slot.
        IDX pushBack(Node*& rootNode, IDX& rootIdx, Node& node);
        // Takes a free slot, growing the storage by 128 first when none is free: its index goes to newNodeIdx and
        // the node is returned (rootNode and rootIdx as for reserve).
        Node* newNode(Node*& rootNode, IDX& rootIdx, IDX& newNodeIdx);
        // Hands slot idx back to the free list.
        void deleteNode(IDX idx);
        Node* getNode(IDX idx);
        // The same lookup from a const tree, as the queries take one (the SDK declares both).
        Node* getNode(IDX idx) const;
        // Empties the node storage and the free list, keeping their memory (the sizes at +0x4 and +0x10 to 0).
        void clear();
        // Hands slot nodeIdx back to the free list.
        void removeAt(IDX nodeIdx);

        hkArray<Node> m_nodes;
        hkArray<IDX> m_freeList;
    };

    // buildTreeInternal's work item, kept on a local stack: the leaves indices[m_startIdx, m_startIdx + m_numLeaves) of
    // its index array, the bounds of their centres, and the inner node they go under.
    struct BuildTask {
        hkAabb m_aabb;
        IDX m_startIdx;
        int32_t m_numLeaves;
        Node* m_node;
    };

    hkAabbTree();
    hkAabbTree(const hkVector4& bitOffsetLow, const hkVector4& bitOffsetHigh, const hkVector4& bitScale);
    virtual ~hkAabbTree();

    // Empties the tree: no root (index 0x7FFF), no leaves, and the node storage cleared.
    void clear();
    // Adds a leaf with volume aabb (quantised for a compressed tree) holding userData; returns the leaf's index.
    // userData is a void*, and callers pass pointers.
    IDX add(const hkAabb& aabb, void* userData);
    // Builds the tree over the numLeaves boxes volumes, each leaf holding the matching userDatas entry; leavesOut gets
    // numLeaves entries (the root's index first for a single leaf).
    void build(hkAabb* volumes, void** userDatas, int32_t numLeaves, hkArray<IDX>& leavesOut);
    // Gives leaf leafIdx the volume aabb, moving it in the tree when it leaves its parent's volume.
    void update(IDX leafIdx, const hkAabb& aabb);
    // Splits the leaves top down under root, leafIdxOut getting each leaf's node index; with leavesAreAllocated, the
    // leaves' nodes exist already and leafIdxOut holds their indices.
    void buildTreeInternal(
        BOUNDING_VOLUME* leafVolumes,
        void** userDatas,
        int32_t numLeaves,
        Node* root,
        hkArray<IDX>& leavesOut,
        bool leavesAreAllocated
    );

    // Cuts leaf leafIdx out of the tree: its sibling takes its parent's place, and the volumes and skip indices up the
    // tree are fixed. Returns the node it stopped at (the root, or the node whose volume did not change).
    Node* removeLeafInternal(uint32_t leafIdx, IDX& toDeleteNodeIdx);
    // Takes leaf leafIdx out of the tree and hands its slot (and its parent's) back to the node storage.
    void remove(uint32_t leafIdx);
    // Sets node's skip index to skipNodeIdx and, below an inner node, its children's parent index to nodeIdx, down
    // the whole subtree, merging the children's volumes into each inner node.
    void fixupTree(Node* node, IDX nodeIdx, IDX skipNodeIdx);
    // Puts leaf newLeafIdx into the tree below insertPointIdx, using node scratchNodeIdx (already taken from the node
    // storage) as the new inner node.
    void addLeafInternal(IDX insertPointIdx, IDX newLeafIdx, IDX scratchNodeIdx);
    // Walks the subtree under root with its skip nodes; the checks themselves are compiled out, leaving only the walk.
    void checkConsistency(Node* root, Node* skipNode);

    // Frees the tree's block back to the Havok heap, passing the tree's size.
    static void operator delete(void* apMemory, size_t aSize) {
        hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
    }

    // The vtable pointer is at 0; the 16-byte alignment of the vectors puts the first member at 0x10.
    Node* m_rootNode;
    IDX m_rootIdx;
    int32_t m_numLeaves;
    NodeManager m_nodeManager;
    hkVector4 m_bitOffsetLow;
    hkVector4 m_bitOffsetHigh;
    hkVector4 m_bitScale;
    hkVector4 m_bitScaleInv;

    // The index that names no node (0x7FFF for 16-bit indices): no root, no skip node.
    static const IDX s_invalidIdx = static_cast<IDX>(static_cast<IDX>(~static_cast<IDX>(0)) >> 1);
};
// No root, no leaves, empty node storage, zero bit offsets and bit scales of (1, 1, 1, 0).
template <class BOUNDING_VOLUME, typename IDX>
hkAabbTree<BOUNDING_VOLUME, IDX>::hkAabbTree()
  : m_rootNode(nullptr), m_rootIdx(s_invalidIdx), m_numLeaves(0), m_nodeManager() {
    m_bitOffsetLow.m_quad = _mm_setzero_ps();
    m_bitOffsetHigh.m_quad = _mm_setzero_ps();
    m_bitScale.m_quad = _mm_setr_ps(1.0f, 1.0f, 1.0f, 0.0f);
    m_bitScaleInv.m_quad = m_bitScale.m_quad;
}

// An empty tree with the given quantisation: m_bitScaleInv is bitScale's reciprocal with its w taken as 1.
template <class BOUNDING_VOLUME, typename IDX>
hkAabbTree<BOUNDING_VOLUME, IDX>::hkAabbTree(
    const hkVector4& bitOffsetLow,
    const hkVector4& bitOffsetHigh,
    const hkVector4& bitScale
)
  : m_rootNode(nullptr), m_rootIdx(s_invalidIdx), m_numLeaves(0), m_nodeManager() {
    m_bitOffsetLow.m_quad = bitOffsetLow.m_quad;
    m_bitOffsetHigh.m_quad = bitOffsetHigh.m_quad;
    m_bitScale.m_quad = bitScale.m_quad;
    const __m128 scale = _mm_shuffle_ps(bitScale.m_quad, _mm_unpackhi_ps(bitScale.m_quad, _mm_set1_ps(1.0f)), 196);
    const __m128 rcp = _mm_rcp_ps(scale);
    m_bitScaleInv.m_quad = _mm_sub_ps(_mm_add_ps(rcp, rcp), _mm_mul_ps(_mm_mul_ps(rcp, scale), rcp));
}

// Empties the tree first; the node manager frees the storage.
template <class BOUNDING_VOLUME, typename IDX>
hkAabbTree<BOUNDING_VOLUME, IDX>::~hkAabbTree() {
    clear();
}

// Grows the node storage to size nodes (their parent words zeroed, as Node's SDK constructor does) and puts the new
// slots on the free list, the highest first; growing moves the nodes, so rootNode is pointed at rootIdx again.
template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTree<BOUNDING_VOLUME, IDX>::NodeManager::reserve(IDX size, Node*& rootNode, IDX& rootIdx) {
    const int32_t n = static_cast<int32_t>(size);
    const int32_t oldSize = m_nodes.getSize();
    m_nodes.reserve(n);
    for (int32_t i = oldSize; i < n; ++i) {
        m_nodes.m_data[i].m_parentIdx = 0;
    }
    m_nodes.m_size = n;
    m_freeList.reserve(n);
    for (int32_t i = n - 1; i >= oldSize; --i) {
        m_freeList.m_data[m_freeList.m_size++] = static_cast<IDX>(i);
    }
    if (rootIdx != s_invalidIdx) {
        rootNode = &m_nodes.m_data[rootIdx];
    }
}

// Copies node into the last free slot (growing the storage first when none is left) and returns the slot.
template <class BOUNDING_VOLUME, typename IDX>
IDX hkAabbTree<BOUNDING_VOLUME, IDX>::NodeManager::pushBack(Node*& rootNode, IDX& rootIdx, Node& node) {
    if (m_freeList.getSize() == 0) {
        reserve(static_cast<IDX>(m_nodes.getSize() + MEM_INCREASE_SIZE), rootNode, rootIdx);
    }
    const IDX idx = m_freeList.m_data[m_freeList.m_size - 1];
    m_freeList.popBack();
    m_nodes.m_data[idx] = node;
    return idx;
}

// Takes the last free slot (growing the storage first when none is left): its index in newNodeIdx, its node returned.
template <class BOUNDING_VOLUME, typename IDX>
hkAabbTree<BOUNDING_VOLUME, IDX>::Node*
hkAabbTree<BOUNDING_VOLUME, IDX>::NodeManager::newNode(Node*& rootNode, IDX& rootIdx, IDX& newNodeIdx) {
    if (m_freeList.getSize() == 0) {
        reserve(static_cast<IDX>(m_nodes.getSize() + MEM_INCREASE_SIZE), rootNode, rootIdx);
    }
    newNodeIdx = m_freeList.m_data[m_freeList.m_size - 1];
    m_freeList.popBack();
    return &m_nodes.m_data[newNodeIdx];
}

template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTree<BOUNDING_VOLUME, IDX>::NodeManager::deleteNode(IDX idx) {
    removeAt(idx);
}

template <class BOUNDING_VOLUME, typename IDX>
hkAabbTree<BOUNDING_VOLUME, IDX>::Node* hkAabbTree<BOUNDING_VOLUME, IDX>::NodeManager::getNode(IDX idx) {
    return &m_nodes.m_data[idx];
}

template <class BOUNDING_VOLUME, typename IDX>
hkAabbTree<BOUNDING_VOLUME, IDX>::Node* hkAabbTree<BOUNDING_VOLUME, IDX>::NodeManager::getNode(IDX idx) const {
    return &m_nodes.m_data[idx];
}

// Empties both arrays, keeping their storage.
template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTree<BOUNDING_VOLUME, IDX>::NodeManager::clear() {
    m_nodes.m_size = 0;
    m_freeList.m_size = 0;
}

template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTree<BOUNDING_VOLUME, IDX>::NodeManager::removeAt(IDX nodeIdx) {
    m_freeList.pushBack(nodeIdx);
}

template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTree<BOUNDING_VOLUME, IDX>::clear() {
    m_nodeManager.clear();
    m_rootNode = nullptr;
    m_rootIdx = s_invalidIdx;
    m_numLeaves = 0;
}

// The leaves' boxes converted to the tree's space, then a single leaf put at the root or the tree built under it, the
// skip and parent indices fixed up and the tree walked.
template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTree<BOUNDING_VOLUME, IDX>::build(
    hkAabb* volumes,
    void** userDatas,
    int32_t numLeaves,
    hkArray<IDX>& leavesOut
) {
    m_numLeaves = numLeaves;
    if (!numLeaves) {
        return;
    }
    leavesOut.setSize(numLeaves);
    m_nodeManager.reserve(static_cast<IDX>(2 * numLeaves - 1), m_rootNode, m_rootIdx);
    m_rootNode = m_nodeManager.newNode(m_rootNode, m_rootIdx, m_rootIdx);
    m_rootNode->m_parentIdx |= s_invalidIdx;
    hkArray<BOUNDING_VOLUME> leafVolumes;
    leafVolumes.setSize(numLeaves);
    for (int32_t i = 0; i < numLeaves; ++i) {
        BOUNDING_VOLUME::convertAabbToTreeSpace(
            volumes[i], m_bitOffsetLow, m_bitOffsetHigh, m_bitScale, leafVolumes.m_data[i]
        );
    }
    if (numLeaves == 1) {
        m_rootNode->m_leafData.m_userData = userDatas[0];
        m_rootNode->m_parentIdx |= Node::s_leafFlag;
        m_rootNode->m_skipNodeIdx = s_invalidIdx;
        m_rootNode->m_volume = leafVolumes.m_data[0];
        leavesOut.m_data[0] = m_rootIdx;
    }
    else {
        buildTreeInternal(leafVolumes.m_data, userDatas, numLeaves, m_rootNode, leavesOut, false);
    }
    fixupTree(m_rootNode, m_rootIdx, s_invalidIdx);
    checkConsistency(m_rootNode, nullptr);
}

// Cuts leaf leafIdx out of the tree: its sibling takes its parent's place under the grandparent (or becomes the root),
// the volumes up from there are recomputed until one does not change, and the right spines that skipped to the parent
// skip past it. The parent is read without checking that the leaf has one, as the game does. Returns the node it
// stopped at (the root when it reached the top).
template <class BOUNDING_VOLUME, typename IDX>
hkAabbTree<BOUNDING_VOLUME, IDX>::Node*
hkAabbTree<BOUNDING_VOLUME, IDX>::removeLeafInternal(uint32_t leafIdx, IDX& toDeleteNodeIdx) {
    Node* leaf = m_nodeManager.getNode(static_cast<IDX>(leafIdx));
    const IDX parentIdx = static_cast<IDX>(leaf->m_parentIdx & Node::s_leafMask);
    Node* parent = parentIdx == s_invalidIdx ? nullptr : m_nodeManager.getNode(parentIdx);
    const IDX grandParentIdx = static_cast<IDX>(parent->m_parentIdx & Node::s_leafMask);
    Node* grandParent = grandParentIdx == s_invalidIdx ? nullptr : m_nodeManager.getNode(grandParentIdx);
    const IDX siblingIdx =
        static_cast<IDX>(leafIdx ^ parent->m_nodeData.m_childIdx[0] ^ parent->m_nodeData.m_childIdx[1]);
    Node* sibling = m_nodeManager.getNode(siblingIdx);
    toDeleteNodeIdx = parentIdx;
    Node* stoppedAt;
    if (grandParent) {
        const int32_t isChild1 = m_nodeManager.getNode(grandParent->m_nodeData.m_childIdx[1]) == parent;
        grandParent->m_nodeData.m_childIdx[isChild1] = siblingIdx;
        sibling->m_parentIdx = static_cast<IDX>(grandParentIdx | (sibling->m_parentIdx & Node::s_leafFlag));
        // The right spine below the grandparent's left child skips the parent that is gone.
        if (isChild1) {
            Node* node = m_nodeManager.getNode(grandParent->m_nodeData.m_childIdx[0]);
            node->m_skipNodeIdx = siblingIdx;
            while (!node->isLeaf()) {
                node = m_nodeManager.getNode(node->m_nodeData.m_childIdx[1]);
                node->m_skipNodeIdx = siblingIdx;
            }
        }
        stoppedAt = grandParent;
        for (;;) {
            BOUNDING_VOLUME merged;
            BOUNDING_VOLUME::combine(
                m_nodeManager.getNode(stoppedAt->m_nodeData.m_childIdx[0])->m_volume,
                m_nodeManager.getNode(stoppedAt->m_nodeData.m_childIdx[1])->m_volume, merged
            );
            if (stoppedAt->m_volume.equals(merged)) {
                break;
            }
            stoppedAt->m_volume = merged;
            const IDX upIdx = static_cast<IDX>(stoppedAt->m_parentIdx & Node::s_leafMask);
            if (upIdx == s_invalidIdx) {
                stoppedAt = m_rootNode;
                break;
            }
            stoppedAt = m_nodeManager.getNode(upIdx);
        }
    }
    else {
        m_rootNode = sibling;
        sibling->m_parentIdx |= Node::s_leafMask;
        stoppedAt = m_rootNode;
        m_rootIdx = siblingIdx;
    }
    // With the sibling on the left, the right spine below it skips the parent that is gone.
    if (m_nodeManager.getNode(parent->m_nodeData.m_childIdx[0]) == sibling) {
        const IDX skipIdx = parent->m_skipNodeIdx;
        Node* node = sibling;
        node->m_skipNodeIdx = skipIdx;
        while (!node->isLeaf()) {
            node = m_nodeManager.getNode(node->m_nodeData.m_childIdx[1]);
            node->m_skipNodeIdx = skipIdx;
        }
    }
    return stoppedAt;
}

// Takes leaf leafIdx out of the tree (the last leaf empties it) and hands its slot, and its parent's, back.
template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTree<BOUNDING_VOLUME, IDX>::remove(uint32_t leafIdx) {
    if (m_rootNode->isLeaf()) {
        m_rootIdx = s_invalidIdx;
        m_rootNode = nullptr;
    }
    else {
        IDX toDeleteNodeIdx;
        removeLeafInternal(leafIdx, toDeleteNodeIdx);
        m_nodeManager.deleteNode(toDeleteNodeIdx);
    }
    m_nodeManager.deleteNode(static_cast<IDX>(leafIdx));
    --m_numLeaves;
}

// Sets node's skip index and, below an inner node, points both children's parent indices at nodeIdx, recurses into
// them (the left child skipping to its sibling, the right one to node's own skip node) and merges their volumes into
// node's.
template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTree<BOUNDING_VOLUME, IDX>::fixupTree(Node* node, IDX nodeIdx, IDX skipNodeIdx) {
    node->m_skipNodeIdx = skipNodeIdx;
    if (node->isLeaf()) {
        return;
    }
    Node* left = m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
    Node* right = m_nodeManager.getNode(node->m_nodeData.m_childIdx[1]);
    left->m_parentIdx = static_cast<IDX>((left->m_parentIdx & Node::s_leafFlag) | nodeIdx);
    right->m_parentIdx = static_cast<IDX>((right->m_parentIdx & Node::s_leafFlag) | nodeIdx);
    fixupTree(left, node->m_nodeData.m_childIdx[0], node->m_nodeData.m_childIdx[1]);
    fixupTree(right, node->m_nodeData.m_childIdx[1], skipNodeIdx);
    BOUNDING_VOLUME::combine(left->m_volume, right->m_volume, node->m_volume);
}

// Descends from insertPointIdx to a leaf, at each inner node into the child the new leaf's metric selects, makes the
// scratch node the parent of that leaf and the new one, links it in below the leaf's parent (or makes it the root),
// points the skip indices on the left sibling's right spine at it, and widens the volumes above it until one already
// holds the new leaf's volume.
template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTree<BOUNDING_VOLUME, IDX>::addLeafInternal(IDX insertPointIdx, IDX newLeafIdx, IDX scratchNodeIdx) {
    Node* newLeaf = m_nodeManager.getNode(newLeafIdx);
    Node* scratchNode = m_nodeManager.getNode(scratchNodeIdx);
    const BOUNDING_VOLUME volume = newLeaf->m_volume;
    const typename BOUNDING_VOLUME::Metric metric = volume.getMetric();
    Node* node = m_nodeManager.getNode(insertPointIdx);
    IDX nodeIdx = insertPointIdx;
    int32_t index = 0;
    while (!node->isLeaf()) {
        index = metric.select(
            m_nodeManager.getNode(node->m_nodeData.m_childIdx[0])->m_volume,
            m_nodeManager.getNode(node->m_nodeData.m_childIdx[1])->m_volume
        );
        nodeIdx = node->m_nodeData.m_childIdx[index];
        node = m_nodeManager.getNode(nodeIdx);
    }
    const IDX parentIdx = static_cast<IDX>(node->m_parentIdx & Node::s_leafMask);
    Node* parent = parentIdx == s_invalidIdx ? nullptr : m_nodeManager.getNode(parentIdx);
    BOUNDING_VOLUME::combine(newLeaf->m_volume, node->m_volume, scratchNode->m_volume);
    scratchNode->m_nodeData.m_childIdx[0] = nodeIdx;
    scratchNode->m_nodeData.m_childIdx[1] = newLeafIdx;
    node->m_parentIdx = static_cast<IDX>((node->m_parentIdx & Node::s_leafFlag) | scratchNodeIdx);
    newLeaf->m_parentIdx = static_cast<IDX>((newLeaf->m_parentIdx & Node::s_leafFlag) | scratchNodeIdx);
    const IDX skipNodeIdx = node->m_skipNodeIdx;
    node->m_skipNodeIdx = newLeafIdx;
    newLeaf->m_skipNodeIdx = skipNodeIdx;
    scratchNode->m_skipNodeIdx = skipNodeIdx;
    if (!parent) {
        m_rootNode = scratchNode;
        scratchNode->m_parentIdx |= s_invalidIdx;
        m_rootIdx = scratchNodeIdx;
        return;
    }
    parent->m_nodeData.m_childIdx[index] = scratchNodeIdx;
    scratchNode->m_parentIdx = static_cast<IDX>((scratchNode->m_parentIdx & Node::s_leafFlag) | parentIdx);
    if (index == 1) {
        Node* edgeNode = m_nodeManager.getNode(parent->m_nodeData.m_childIdx[0]);
        edgeNode->m_skipNodeIdx = scratchNodeIdx;
        while (!edgeNode->isLeaf()) {
            edgeNode = m_nodeManager.getNode(edgeNode->m_nodeData.m_childIdx[1]);
            edgeNode->m_skipNodeIdx = scratchNodeIdx;
        }
    }
    do {
        if (parent->m_volume.contains(volume).m_bool) {
            return;
        }
        BOUNDING_VOLUME::combine(parent->m_volume, volume, parent->m_volume);
        const IDX upIdx = static_cast<IDX>(parent->m_parentIdx & Node::s_leafMask);
        if (upIdx == s_invalidIdx) {
            return;
        }
        parent = m_nodeManager.getNode(upIdx);
    }
    while (parent);
}

// Adds the leaf: its box is converted to the tree's space (a copy for the 32-bit tree, quantised for the 16-bit one),
// copied into the node storage, then made the root or added under it with a scratch node of its own.
template <class BOUNDING_VOLUME, typename IDX>
IDX hkAabbTree<BOUNDING_VOLUME, IDX>::add(const hkAabb& aabb, void* userData) {
    Node node = Node();
    BOUNDING_VOLUME::convertAabbToTreeSpace(aabb, m_bitOffsetLow, m_bitOffsetHigh, m_bitScale, node.m_volume);
    node.m_leafData.m_userData = userData;
    node.m_parentIdx = Node::s_leafFlag;
    const IDX leafIdx = m_nodeManager.pushBack(m_rootNode, m_rootIdx, node);
    if (m_rootIdx == s_invalidIdx) {
        Node* leaf = m_nodeManager.getNode(leafIdx);
        leaf->m_parentIdx |= s_invalidIdx;
        leaf->m_skipNodeIdx = s_invalidIdx;
        ++m_numLeaves;
        m_rootNode = leaf;
        m_rootIdx = leafIdx;
    }
    else {
        IDX scratchNodeIdx;
        m_nodeManager.newNode(m_rootNode, m_rootIdx, scratchNodeIdx);
        addLeafInternal(m_rootIdx, leafIdx, scratchNodeIdx);
        ++m_numLeaves;
    }
    return leafIdx;
}

// The root leaf only takes the new volume; any other leaf is cut out, given the volume and added again from the root,
// its freed parent node serving as the scratch node.
template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTree<BOUNDING_VOLUME, IDX>::update(IDX leafIdx, const hkAabb& aabb) {
    if (m_rootIdx == leafIdx) {
        Node* root = m_nodeManager.getNode(leafIdx);
        BOUNDING_VOLUME::convertAabbToTreeSpace(aabb, m_bitOffsetLow, m_bitOffsetHigh, m_bitScale, root->m_volume);
        return;
    }
    IDX toDeleteNodeIdx;
    removeLeafInternal(leafIdx, toDeleteNodeIdx);
    Node* leaf = m_nodeManager.getNode(leafIdx);
    BOUNDING_VOLUME::convertAabbToTreeSpace(aabb, m_bitOffsetLow, m_bitOffsetHigh, m_bitScale, leaf->m_volume);
    addLeafInternal(m_rootIdx, leafIdx, toDeleteNodeIdx);
}

// The leaf split buildTreeInternal makes, shared by the 16-bit and 32-bit trees: a Hoare partition of the leaves
// indices[start, start + num) by their centres about the middle of aabb's longest axis, those at or below it first.
// leftAabb and rightAabb get the bounds of each side's centres, numLeftOut and numRightOut their counts; returns
// numRightOut. A degenerate axis splits at the middle index with aabb for both bounds.
template <class BOUNDING_VOLUME, typename IDX>
int32_t hkAabbTree_splitLeaves(
    const hkAabb& aabb,
    const BOUNDING_VOLUME* volumes,
    hkArray<IDX>& indices,
    IDX start,
    int32_t num,
    hkAabb& leftAabb,
    hkAabb& rightAabb,
    int32_t& numLeftOut,
    int32_t& numRightOut
) {
    leftAabb.setEmpty();
    rightAabb.setEmpty();
    hkVector4 sum;
    sum.setAdd4(aabb.m_min, aabb.m_max);
    hkVector4 extent;
    extent.setSub4(aabb.m_max, aabb.m_min);
    const int32_t axis = extent.getMajorAxis();
    const float mid = sum(axis) * 0.5f;
    bool canSplit = false;
    if (aabb.m_max(axis) - mid > 2.3841858e-07f) {
        canSplit = mid - aabb.m_min(axis) > 2.3841858e-07f;
    }
    int32_t i;
    if (canSplit) {
        i = start;
        int32_t j = start + num - 1;
        do {
            hkVector4 centerI;
            volumes[indices.m_data[i]].center(centerI);
            while (centerI(axis) <= mid) {
                leftAabb.includePoint(centerI);
                ++i;
                volumes[indices.m_data[i]].center(centerI);
            }
            hkVector4 centerJ;
            volumes[indices.m_data[j]].center(centerJ);
            while (centerJ(axis) > mid) {
                rightAabb.includePoint(centerJ);
                --j;
                volumes[indices.m_data[j]].center(centerJ);
            }
            if (i >= j) {
                break;
            }
            const IDX swapped = indices.m_data[i];
            indices.m_data[i] = indices.m_data[j];
            indices.m_data[j] = swapped;
            volumes[indices.m_data[i]].center(centerI);
            volumes[indices.m_data[j]].center(centerJ);
            ++i;
            --j;
            leftAabb.includePoint(centerI);
            rightAabb.includePoint(centerJ);
        }
        while (i <= j);
    }
    else {
        leftAabb = aabb;
        rightAabb = aabb;
        i = start + (num >> 1);
    }
    numLeftOut = i - start;
    numRightOut = num - numLeftOut;
    return numRightOut;
}

// The SDK's private buildTreeInternal: the leaves' indices in a local array, the bounds of the leaves' centres, then a
// stack of tasks split top down (hkAabbTree_splitLeaves). A part of more than one leaf gets an inner node and a task
// of its own; a single leaf, and both leaves of a task of two, get their nodes at once, taken from the node storage
// or, with leavesAreAllocated, looked up by the index leafIdxOut already holds.
template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTree<BOUNDING_VOLUME, IDX>::buildTreeInternal(
    BOUNDING_VOLUME* leafVolumes,
    void** userDatas,
    int32_t numLeaves,
    Node* root,
    hkArray<IDX>& leavesOut,
    bool leavesAreAllocated
) {
    hkArray<IDX> indices;
    indices.setSize(numLeaves);
    for (int32_t i = 0; i < numLeaves; ++i) {
        indices.m_data[i] = static_cast<IDX>(i);
    }
    hkInplaceArray<BuildTask, 64> stack;
    hkAabb centres;
    centres.setEmpty();
    for (int32_t i = 0; i < numLeaves; ++i) {
        hkVector4 centre;
        leafVolumes[i].center(centre);
        centres.includePoint(centre);
    }
    BuildTask first;
    first.m_aabb = centres;
    first.m_startIdx = 0;
    first.m_numLeaves = numLeaves;
    first.m_node = root;
    stack.pushBack(first);
    do {
        BuildTask task = stack.m_data[stack.m_size - 1];
        stack.popBack();
        if (task.m_numLeaves > 2) {
            int32_t numChildLeaves[2];
            hkAabb childCentres[2];
            hkAabbTree_splitLeaves(
                task.m_aabb, leafVolumes, indices, task.m_startIdx, task.m_numLeaves, childCentres[0], childCentres[1],
                numChildLeaves[0], numChildLeaves[1]
            );
            BuildTask children[2];
            int32_t offset = 0;
            for (int32_t c = 0; c < 2; ++c) {
                IDX nodeIdx;
                if (numChildLeaves[c] > 1) {
                    children[c].m_node = m_nodeManager.newNode(m_rootNode, m_rootIdx, nodeIdx);
                    children[c].m_numLeaves = numChildLeaves[c];
                    children[c].m_aabb = childCentres[c];
                    children[c].m_startIdx = static_cast<IDX>(c * numChildLeaves[0] + task.m_startIdx);
                }
                else {
                    const IDX leaf = indices.m_data[task.m_startIdx + offset];
                    Node* node;
                    if (leavesAreAllocated) {
                        nodeIdx = leavesOut.m_data[leaf];
                        node = m_nodeManager.getNode(nodeIdx);
                    }
                    else {
                        node = m_nodeManager.newNode(m_rootNode, m_rootIdx, nodeIdx);
                    }
                    node->m_volume = leafVolumes[leaf];
                    node->m_leafData.m_userData = userDatas[leaf];
                    node->m_parentIdx |= Node::s_leafFlag;
                    leavesOut.m_data[leaf] = nodeIdx;
                }
                task.m_node->m_nodeData.m_childIdx[c] = nodeIdx;
                offset += numChildLeaves[0];
            }
            // The right part goes on the stack first, so that the left one is split next.
            for (int32_t c = 1; c >= 0; --c) {
                if (numChildLeaves[c] > 1) {
                    stack.pushBack(children[c]);
                }
            }
        }
        else {
            for (int32_t k = 0; k < 2; ++k) {
                const IDX leaf = indices.m_data[task.m_startIdx + k];
                IDX nodeIdx;
                Node* node;
                if (leavesAreAllocated) {
                    nodeIdx = leavesOut.m_data[leaf];
                    node = m_nodeManager.getNode(nodeIdx);
                }
                else {
                    node = m_nodeManager.newNode(m_rootNode, m_rootIdx, nodeIdx);
                }
                leavesOut.m_data[leaf] = nodeIdx;
                node->m_volume = leafVolumes[leaf];
                node->m_leafData.m_userData = userDatas[leaf];
                node->m_parentIdx |= Node::s_leafFlag;
                task.m_node->m_nodeData.m_childIdx[k] = nodeIdx;
            }
        }
    }
    while (stack.getSize() != 0);
}

// Consistency walk with its checks compiled out: down the left children from root, keeping each left child's skip node
// (its right sibling) on a local stack and taking one back at each leaf.
template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTree<BOUNDING_VOLUME, IDX>::checkConsistency(Node* root, Node* skipNode) {
    if (!root) {
        return;
    }
    hkInplaceArray<Node*, 128> stack;
    Node* node = root;
    for (;;) {
        if (node->isLeaf()) {
            if (stack.getSize() == 0) {
                break;
            }
            node = stack.m_data[stack.m_size - 1];
            stack.popBack();
            continue;
        }
        node = m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
        stack.pushBack(node->m_skipNodeIdx == s_invalidIdx ? nullptr : m_nodeManager.getNode(node->m_skipNodeIdx));
    }
}
// The tree used for hkgpMesh's triangle tree.
using hkAabbTreeAabbTree32 = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>;

// The world's broad-phase tree: 16-bit quantised volumes with 16-bit node indices.
using hkAabbTreeAabb16Tree = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>;

using hkAabbTreeNode = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node;

// hkAabbTree<hkAabbTreeAabb,unsigned short>::Node::operator=, the implicit copy assignment (the volume, both union
// words, the parent and skip indices); returns apThis. It is a free function, since a user-declared operator= on the
// template's Node would change every node copy.
hkAabbTreeNode* hkAabbTreeNode_assign(hkAabbTreeNode* apThis, const hkAabbTreeNode& arOther);
// splitInto2 for the 32-bit leaves of hkAabbTree<hkAabbTreeAabb, unsigned int>::buildTreeInternal, its one caller: the
// same split, with the volumes indexed in 32-byte steps and 32-bit indices.
int32_t splitInto2(
    const hkAabb& aabb,
    const hkAabbTreeData::hkAabbTreeAabb* volumes,
    hkArray<uint32_t>& indices,
    uint32_t start,
    int32_t num,
    hkAabb& leftAabb,
    hkAabb& rightAabb,
    int32_t& numLeftOut,
    int32_t& numRightOut
);
// Splits the 16-bit leaves indices[start, start + num) of hkAabbTree<hkAabbTreeAabb16, unsigned
// short>::buildTreeInternal, its one caller, in place about the middle of aabb's longest axis, by their centres: those
// at or below it first. leftAabb and rightAabb get the bounds of each side's centres, numLeftOut and numRightOut their
// counts; returns numRightOut. A degenerate axis splits at the middle index with aabb for both bounds.
int32_t splitInto2(
    const hkAabb& aabb,
    const hkAabbTreeData::hkAabbTreeAabb16* volumes,
    hkArray<uint16_t>& indices,
    uint16_t start,
    int32_t num,
    hkAabb& leftAabb,
    hkAabb& rightAabb,
    int32_t& numLeftOut,
    int32_t& numRightOut
);

using hkAabbTreeNode16 = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node;
using hkAabbTreeNodeManager16 = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::NodeManager;
using hkAabbTreeBuildTask16 = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::BuildTask;
