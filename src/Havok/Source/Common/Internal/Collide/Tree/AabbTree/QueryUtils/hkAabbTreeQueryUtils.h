#pragma once

#include "BSCore/NewOverloads.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"

#include <stdint.h>

// The tree queries' shared statics: the ray-count-to-lane-mask table and bundleRays, which packs four rays into an
// hkAabbTreeData::RayBundle. No data members.
class hkAabbTreeQueryUtilsData {
public:
    // The bundle casts' lane masks by the number of active rays (0 to 4): MASK_NONE, X, XY, XYZ, XYZW, indexed by
    // m_numActiveRays.
    using NumRaysToMaskArray = hkVector4Comparison::Mask[5];

    static inline NumRaysToMaskArray s_numRaysToMask{
        hkVector4Comparison::MASK_NONE,
        hkVector4Comparison::MASK_X,
        hkVector4Comparison::MASK_XY,
        hkVector4Comparison::MASK_XYZ,
        hkVector4Comparison::MASK_XYZW
    };
};

// Queries over an hkAabbTree.
template <class BOUNDING_VOLUME, typename IDX>
struct hkAabbTreeQueryUtils {
    using Node = hkAabbTree<BOUNDING_VOLUME, IDX>::Node;

    // Two leaves a tree-tree query found overlapping, as nodes.
    struct NodePair {
        Node* m_a;
        Node* m_b;
    };

    // Two leaves a tree-tree query found overlapping, as indices.
    struct IdxPair {
        IDX m_a;
        IDX m_b;
    };

    // Receives the leaf pairs a tree-tree query finds; addHit returns whether to stop.
    struct TreeTreeCollector {
        virtual ~TreeTreeCollector();

        virtual bool addHit(Node* leafA, Node* leafB) = 0;

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }
    };

    // Receives the leaves a query reaches; addLeaf returns the hit fraction the query may stop at.
    struct Collector {
        virtual ~Collector();

        virtual float addLeaf(const hkAabbTree<BOUNDING_VOLUME, IDX>::Node* leaf) = 0;

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }
    };

    // A collector that looks for the leaf closest to a point.
    struct ClosestPointCollector : public Collector {
        ~ClosestPointCollector() override;

        float addLeaf(const Node* leaf) override = 0;

        hkVector4 m_point;
        float m_distanceSquared;
    };

    // A tree-tree collector that appends every pair to a buffer.
    struct TreeTreeNodePairCollector : public TreeTreeCollector {
        explicit TreeTreeNodePairCollector(hkArray<NodePair>* resultBuffer) : m_resultBuffer(resultBuffer) {}
        ~TreeTreeNodePairCollector() override;

        bool addHit(Node* leafA, Node* leafB) override;

        hkArray<NodePair>* m_resultBuffer;
    };

    // A collector that keeps the user data of every leaf it reaches.
    struct AllHitsCollector : public Collector {
        ~AllHitsCollector() override;

        float addLeaf(const Node* leaf) override;

        hkInplaceArray<const void*, 128> m_hits;
    };

    // A collector that keeps every leaf node it reaches, in a caller's buffer.
    struct AllHitsNodeCollector : public Collector {
        ~AllHitsNodeCollector() override;

        float addLeaf(const Node* leaf) override;

        hkArray<Node*>* m_hits;
    };

    // Casts the ray from from to to through tree, handing each leaf it reaches to collector; the walk stops beyond
    // earlyOutFraction and the fractions addLeaf returns. Only the 16-bit instance below is declared.
    static void castRay(
        const hkAabbTree<BOUNDING_VOLUME, IDX>* tree,
        const hkVector4& rayFrom,
        const hkVector4& rayTo,
        Collector* collector,
        float initialEarlyOutFraction
    );

    // Casts four rays through tree below node nodePpu until escapeNode.
    static void castRayBundle(
        const hkAabbTree<BOUNDING_VOLUME, IDX>* tree,
        const Node* nodePpu,
        const Node* escapeNode,
        const hkAabbTreeData::RayBundle& rayBundle,
        Collector** collectors,
        float* earlyHitFractions
    );
    // The SPU form of castRayBundle (the same walk, with the node in SPU storage).
    static void castRayBundleSpu(
        const hkAabbTree<BOUNDING_VOLUME, IDX>* tree,
        const Node* nodeSpu,
        const Node* escapeNode,
        const hkAabbTreeData::RayBundle& rayBundle,
        Collector** collectors,
        float* earlyHitFractions
    );
    // Hands every leaf overlapping aabb to collector; returns how many.
    static int32_t queryAabb(const hkAabbTree<BOUNDING_VOLUME, IDX>* tree, const hkAabb& aabb, Collector* collector);
    // Writes the user data of every leaf overlapping aabb to *hitArray (up to hitCapacity); returns how many.
    static int32_t
    queryAabb(const hkAabbTree<BOUNDING_VOLUME, IDX>* tree, const hkAabb& aabb, void*** hitArray, int32_t hitCapacity);
    // Appends the user data of every leaf overlapping aabb to hitArray; returns how many.
    static int32_t
    queryAabb(const hkAabbTree<BOUNDING_VOLUME, IDX>* tree, const hkAabb& aabb, hkArray<void*>& hitArray);
    // Sweeps queryVolume along input, handing the leaves it hits to collector.
    static int32_t castAabb(
        const hkAabbTree<BOUNDING_VOLUME, IDX>* tree,
        const hkAabb& queryVolume,
        hkAabbTreeData::CastVolumeInput& input,
        Collector* collector,
        float aabbCheckTolerance,
        void** hitArray,
        int32_t hitCapacity
    );
    // Hands every overlapping leaf pair of the two trees to collector.
    static int32_t queryTreeCollision(
        const hkAabbTree<BOUNDING_VOLUME, IDX>* treeA,
        const hkAabbTree<BOUNDING_VOLUME, IDX>* treeB,
        TreeTreeCollector* collector
    );
    // Appends every overlapping leaf pair of the two trees to pairsOut.
    static int32_t queryTreeCollision(
        const hkAabbTree<BOUNDING_VOLUME, IDX>* treeA,
        const hkAabbTree<BOUNDING_VOLUME, IDX>* treeB,
        hkArray<IdxPair>* pairsOut
    );
    // Hands the leaves around point to collector, nearest first.
    static void
    queryClosestPoint(const hkAabbTree<BOUNDING_VOLUME, IDX>* tree, const hkVector4& point, Collector* collector);
};

// tree is as for queryAabb. collector is called through Collector::addLeaf, which returns a float; hkgpMesh::locate
// passes a NearestFeaturePolicy.
void queryClosestPoint(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* tree,
    const hkVector4& point,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* collector
);
// Takes the tree, the query box, and one of the three outputs queryAabb's forms pass: a collector, an array, or a void*
// write iterator with its capacity (queryAabb, queryAabbToArray and queryAabbInt16ToBuffer).
int32_t queryAabbInternal(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* tree,
    const hkAabb& aabb,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector* collector,
    hkArray<void*>* hitArray,
    void*** hitIterator,
    int32_t hitCapacity
);

// The destructors are empty: the collectors own nothing to destroy.
template <class BOUNDING_VOLUME, typename IDX>
hkAabbTreeQueryUtils<BOUNDING_VOLUME, IDX>::TreeTreeCollector::~TreeTreeCollector() {}

template <class BOUNDING_VOLUME, typename IDX>
hkAabbTreeQueryUtils<BOUNDING_VOLUME, IDX>::Collector::~Collector() {}

// Nothing of its own to destroy.
template <class BOUNDING_VOLUME, typename IDX>
hkAabbTreeQueryUtils<BOUNDING_VOLUME, IDX>::TreeTreeNodePairCollector::~TreeTreeNodePairCollector() {}

// Nothing of its own to destroy (m_point and m_distanceSquared are plain data).
template <class BOUNDING_VOLUME, typename IDX>
hkAabbTreeQueryUtils<BOUNDING_VOLUME, IDX>::ClosestPointCollector::~ClosestPointCollector() {}

// Appends the pair (leafA, leafB) to *m_resultBuffer and returns false, so the query never stops.
template <class BOUNDING_VOLUME, typename IDX>
bool hkAabbTreeQueryUtils<BOUNDING_VOLUME, IDX>::TreeTreeNodePairCollector::addHit(Node* leafA, Node* leafB) {
    NodePair kPair;
    kPair.m_a = leafA;
    kPair.m_b = leafB;
    m_resultBuffer->pushBack(kPair);
    return false;
}

// A walk along the skip links from the root; each node's box, restored to world space, is tested against the ray up to
// the early-out fraction; a hit inner node descends to its first child, a hit leaf goes to collector, whose result
// becomes the early-out fraction.
template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTreeQueryUtils<BOUNDING_VOLUME, IDX>::castRay(
    const hkAabbTree<BOUNDING_VOLUME, IDX>* tree,
    const hkVector4& rayFrom,
    const hkVector4& rayTo,
    Collector* collector,
    float initialEarlyOutFraction
) {
    using Tree = hkAabbTree<BOUNDING_VOLUME, IDX>;
    if (!tree) {
        return;
    }
    const Node* node = tree->m_rootNode;
    if (!node) {
        return;
    }
    hkAabbTreeData::Ray ray;
    ray.setRay(rayFrom, rayTo);
    hkVector4 earlyOut;
    earlyOut.setAll(initialEarlyOutFraction);
    for (;;) {
        hkAabbTreeData::hkAabbTreeAabb volume;
        BOUNDING_VOLUME::restoreAabb(node->m_volume, tree->m_bitScaleInv, tree->m_bitOffsetLow, volume);
        const Node* next = (node->m_skipNodeIdx == Tree::s_invalidIdx)
            ? nullptr
            : tree->m_nodeManager.getNode(node->m_skipNodeIdx);
        if (volume.overlaps(ray, earlyOut)) {
            if (!node->isLeaf()) {
                node = tree->m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
                continue;
            }
            earlyOut.setAll(collector->addLeaf(node));
        }
        node = next;
        if (!node) {
            return;
        }
    }
}

// Walks the four rays of rayBundle from nodePpu along the skip links until escapeNode. Each node's box, restored to
// world space, is slab-tested against the rays the bundle's mask selects, each up to its early-out fraction (a local
// copy: the caller's array is only read); a hit inner node descends to its first child, a hit leaf goes to the
// collector of each ray that hit it, whose result becomes that ray's fraction.
template <class BOUNDING_VOLUME, typename IDX>
void hkAabbTreeQueryUtils<BOUNDING_VOLUME, IDX>::castRayBundle(
    const hkAabbTree<BOUNDING_VOLUME, IDX>* tree,
    const Node* nodePpu,
    const Node* escapeNode,
    const hkAabbTreeData::RayBundle& rayBundle,
    Collector** collectors,
    float* earlyHitFractions
) {
    using Tree = hkAabbTree<BOUNDING_VOLUME, IDX>;
    hkVector4 earlyOut;
    earlyOut.set(earlyHitFractions[0], earlyHitFractions[1], earlyHitFractions[2], earlyHitFractions[3]);
    const Node* node = nodePpu;
    const Node* skipNode;
    do {
        for (;;) {
            hkAabb volume;
            BOUNDING_VOLUME::restoreAabb(node->m_volume, tree->m_bitScaleInv, tree->m_bitOffsetLow, volume);
            skipNode = (node->m_skipNodeIdx == Tree::s_invalidIdx) ? nullptr
                                                                   : tree->m_nodeManager.getNode(node->m_skipNodeIdx);
            // Per axis, the four rays' entry and exit fractions of the slab.
            __m128 tEnter[3];
            __m128 tExit[3];
            for (int32_t i = 0; i < 3; ++i) {
                const __m128 lo = _mm_set1_ps(volume.m_min(i));
                const __m128 hi = _mm_set1_ps(volume.m_max(i));
                const __m128 rayFrom = rayBundle.m_from.m_data[i].m_quad;
                const __m128 invDir = rayBundle.m_invDir.m_data[i].m_quad;
                const __m128 tLo = _mm_mul_ps(_mm_sub_ps(lo, rayFrom), invDir);
                const __m128 tHi = _mm_mul_ps(_mm_sub_ps(hi, rayFrom), invDir);
                tEnter[i] = _mm_min_ps(tLo, tHi);
                tExit[i] = _mm_max_ps(tLo, tHi);
            }
            const __m128 enter = _mm_max_ps(
                _mm_max_ps(_mm_max_ps(tEnter[0], tEnter[1]), tEnter[2]),
                hkVector4::getZero().m_quad
            );
            const __m128 exit = _mm_min_ps(_mm_min_ps(_mm_min_ps(tExit[0], tExit[1]), tExit[2]), earlyOut.m_quad);
            // The lanes hit, of those the bundle's mask selects.
            const int32_t hits = _mm_movemask_ps(_mm_cmplt_ps(enter, exit)) & rayBundle.m_mask;
            if (hits == 0) {
                break;
            }
            if (!node->isLeaf()) {
                node = tree->m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
                continue;
            }
            for (int32_t i = 0; i < 4; ++i) {
                if ((hits >> i) & hkVector4Comparison::MASK_X) {
                    earlyOut(i) = collectors[i]->addLeaf(node);
                }
            }
            break;
        }
        node = skipNode;
    }
    while (node != escapeNode);
}

// Casts the box's sweep from input.m_from to input.m_to as a ray through the tree's boxes grown by input.m_halfExtents
// plus aabbCheckTolerance (queryVolume is not read). Each leaf reached goes to collector, whose result becomes the
// early-out fraction (1 at first), else its data is written to hitArray, the walk stopping once the count before it has
// reached hitCapacity. Returns the leaves counted.
template <class BOUNDING_VOLUME, typename IDX>
int32_t hkAabbTreeQueryUtils<BOUNDING_VOLUME, IDX>::castAabb(
    const hkAabbTree<BOUNDING_VOLUME, IDX>* tree,
    const hkAabb& queryVolume,
    hkAabbTreeData::CastVolumeInput& input,
    Collector* collector,
    float aabbCheckTolerance,
    void** hitArray,
    int32_t hitCapacity
) {
    using Tree = hkAabbTree<BOUNDING_VOLUME, IDX>;
    if (!tree) {
        return 0;
    }
    const Node* node = tree->m_rootNode;
    if (!node) {
        return 0;
    }
    hkAabbTreeData::Ray ray;
    ray.setRay(input.m_from, input.m_to);
    hkVector4 halfExtents;
    halfExtents.setAll(aabbCheckTolerance);
    halfExtents.add4(input.m_halfExtents);
    hkVector4 earlyOut;
    earlyOut.setAll(1.0f);
    int32_t numHits = 0;
    for (;;) {
        hkAabbTreeData::hkAabbTreeAabb volume;
        BOUNDING_VOLUME::restoreAabb(node->m_volume, tree->m_bitScaleInv, tree->m_bitOffsetLow, volume);
        volume.m_min.sub4(halfExtents);
        volume.m_max.add4(halfExtents);
        const Node* next = (node->m_skipNodeIdx == Tree::s_invalidIdx)
            ? nullptr
            : tree->m_nodeManager.getNode(node->m_skipNodeIdx);
        if (volume.overlaps(ray, earlyOut)) {
            if (!node->isLeaf()) {
                node = tree->m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
                continue;
            }
            if (collector) {
                earlyOut.setAll(collector->addLeaf(node));
            }
            else if (hitArray) {
                *hitArray++ = node->m_leafData.m_userData;
                if (numHits >= hitCapacity) {
                    return numHits;
                }
            }
            ++numHits;
        }
        node = next;
        if (!node) {
            return numHits;
        }
    }
}

// Tree-tree query: the node pairs still to test are kept on a stack of 64, doubled when it comes within four of its
// size. Two identical nodes are expanded as three pairs (a-a, a-b, b-b), treeA's nodes looked up through treeA and
// treeB's through treeB; two overlapping leaves go to collector, the walk stopping when it returns true. The boxes are
// compared as hkAabbs (xyz lanes of a.min <= b.max and b.min <= a.max). Returns 0.
template <class BOUNDING_VOLUME, typename IDX>
int32_t hkAabbTreeQueryUtils<BOUNDING_VOLUME, IDX>::queryTreeCollision(
    const hkAabbTree<BOUNDING_VOLUME, IDX>* treeA,
    const hkAabbTree<BOUNDING_VOLUME, IDX>* treeB,
    TreeTreeCollector* collector
) {
    Node* rootA = treeA->m_rootNode;
    Node* rootB = treeB->m_rootNode;
    if (!rootA || !rootB) {
        return 0;
    }
    hkInplaceArray<NodePair, 64> stack;
    stack.setSize(64);
    stack.m_data[0].m_a = rootA;
    stack.m_data[0].m_b = rootB;
    int32_t sp = 1;
    while (sp > 0) {
        --sp;
        Node* a = stack.m_data[sp].m_a;
        Node* b = stack.m_data[sp].m_b;
        if (sp > stack.m_size - 4) {
            stack.setSize(stack.m_size * 2);
        }
        if (a == b) {
            if (a->isLeaf()) {
                continue;
            }
            Node* child0 = treeA->m_nodeManager.getNode(a->m_nodeData.m_childIdx[0]);
            Node* child1 = treeA->m_nodeManager.getNode(a->m_nodeData.m_childIdx[1]);
            stack.m_data[sp++] = NodePair{child0, child0};
            stack.m_data[sp++] = NodePair{child0, child1};
            stack.m_data[sp++] = NodePair{child1, child1};
            continue;
        }
        if (!static_cast<const hkAabb&>(a->m_volume).overlaps(b->m_volume)) {
            continue;
        }
        if (a->isLeaf()) {
            if (b->isLeaf()) {
                if (collector->addHit(a, b)) {
                    break;
                }
                continue;
            }
            stack.m_data[sp++] = NodePair{a, treeB->m_nodeManager.getNode(b->m_nodeData.m_childIdx[0])};
            stack.m_data[sp++] = NodePair{a, treeB->m_nodeManager.getNode(b->m_nodeData.m_childIdx[1])};
            continue;
        }
        if (b->isLeaf()) {
            stack.m_data[sp++] = NodePair{treeA->m_nodeManager.getNode(a->m_nodeData.m_childIdx[0]), b};
            stack.m_data[sp++] = NodePair{treeA->m_nodeManager.getNode(a->m_nodeData.m_childIdx[1]), b};
            continue;
        }
        Node* childA0 = treeA->m_nodeManager.getNode(a->m_nodeData.m_childIdx[0]);
        Node* childA1 = treeA->m_nodeManager.getNode(a->m_nodeData.m_childIdx[1]);
        Node* childB0 = treeB->m_nodeManager.getNode(b->m_nodeData.m_childIdx[0]);
        Node* childB1 = treeB->m_nodeManager.getNode(b->m_nodeData.m_childIdx[1]);
        stack.m_data[sp++] = NodePair{childA0, childB0};
        stack.m_data[sp++] = NodePair{childA0, childB1};
        stack.m_data[sp++] = NodePair{childA1, childB0};
        stack.m_data[sp++] = NodePair{childA1, childB1};
    }
    return 0;
}

// The same walk over node indices: each pair's nodes are looked up through their trees, and every pair of overlapping
// leaves is appended to pairsOut. Returns the pairs appended.
template <class BOUNDING_VOLUME, typename IDX>
int32_t hkAabbTreeQueryUtils<BOUNDING_VOLUME, IDX>::queryTreeCollision(
    const hkAabbTree<BOUNDING_VOLUME, IDX>* treeA,
    const hkAabbTree<BOUNDING_VOLUME, IDX>* treeB,
    hkArray<IdxPair>* pairsOut
) {
    if (!treeA->m_rootNode || !treeB->m_rootNode) {
        return 0;
    }
    hkInplaceArray<IdxPair, 64> stack;
    stack.setSize(64);
    stack.m_data[0].m_a = treeA->m_rootIdx;
    stack.m_data[0].m_b = treeB->m_rootIdx;
    int32_t sp = 1;
    int32_t numPairs = 0;
    do {
        --sp;
        const IDX idxA = stack.m_data[sp].m_a;
        const IDX idxB = stack.m_data[sp].m_b;
        if (sp > stack.m_size - 4) {
            stack.setSize(stack.m_size * 2);
        }
        Node* a = treeA->m_nodeManager.getNode(idxA);
        Node* b = treeB->m_nodeManager.getNode(idxB);
        if (a == b) {
            if (!a->isLeaf()) {
                const IDX child0 = a->m_nodeData.m_childIdx[0];
                const IDX child1 = a->m_nodeData.m_childIdx[1];
                stack.m_data[sp++] = IdxPair{child0, child0};
                stack.m_data[sp++] = IdxPair{child0, child1};
                stack.m_data[sp++] = IdxPair{child1, child1};
            }
            continue;
        }
        if (!static_cast<const hkAabb&>(a->m_volume).overlaps(b->m_volume)) {
            continue;
        }
        if (a->isLeaf()) {
            if (b->isLeaf()) {
                pairsOut->pushBack(IdxPair{idxA, idxB});
                ++numPairs;
                continue;
            }
            stack.m_data[sp++] = IdxPair{idxA, b->m_nodeData.m_childIdx[0]};
            stack.m_data[sp++] = IdxPair{idxA, b->m_nodeData.m_childIdx[1]};
            continue;
        }
        if (b->isLeaf()) {
            stack.m_data[sp++] = IdxPair{a->m_nodeData.m_childIdx[0], idxB};
            stack.m_data[sp++] = IdxPair{a->m_nodeData.m_childIdx[1], idxB};
            continue;
        }
        const IDX childA0 = a->m_nodeData.m_childIdx[0];
        const IDX childA1 = a->m_nodeData.m_childIdx[1];
        const IDX childB0 = b->m_nodeData.m_childIdx[0];
        const IDX childB1 = b->m_nodeData.m_childIdx[1];
        stack.m_data[sp++] = IdxPair{childA0, childB0};
        stack.m_data[sp++] = IdxPair{childA0, childB1};
        stack.m_data[sp++] = IdxPair{childA1, childB0};
        stack.m_data[sp++] = IdxPair{childA1, childB1};
    }
    while (sp);
    return numPairs;
}
