#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

class hkKdTree;
class hkKdTreeBundleCastCollector;
class hkKdTreeCastCollector;
struct hkKdTreeNode;

// hkKdTreeMath.inl's file-static hkGetOneOver65K: returns 1 / 65535 in every lane.
hkQuadReal hkGetOneOver65K_D1B280();
// Calls isSameSignDiff on apA and apB under aiMask and returns its hkBool through apResult.
hkBool isSameSignDiffWrapper(const hkVector4* apA, const hkVector4* apB, int32_t aiMask);
// Whether, under the lane mask x3, each of the three vectors x2[i] - x1[i] has all its masked lanes of one sign (all
// below, all equal to or all above 0), into the hkBool x0; returns x0.
hkBool isSameSignDiff(const hkVector4* x1, const hkVector4* x2, int32_t x3);

// Ray queries over an hkKdTree.
class hkKdTreeUtils {
public:
    // The node stack's depth where the tree has none (m_maxDepth 0 or less).
    enum { DEFAULT_STACK_SIZE = 128 };

    // A bundle of four rays: its three vectors hold the four rays' x, y and z, one ray per lane (the SDK's
    // hkKdTreeUtils::RayBundle, sizeof 48 in hkinternal.pdb).
    struct RayBundle {
        hkVector4 m_vec[3];
    };

    // An entry of the iterative ray cast's node stack: the node and the ray's parameter range through it. The SDK's
    // HK_ALIGN16 on node pads it to 16 bytes (hkinternal.pdb: sizeof 16, members at 0, 4, 8, 0xC).
    struct NodeState {
        alignas(16) const hkKdTreeNode* node;
        float tNear;
        float tFar;
        uint32_t m_padding;
    };

    // An entry of the iterative ray bundle cast's node stack: the node, each ray's parameter range through it and which
    // rays are still active.
    struct BundleState {
        hkPadSpu<const hkKdTreeNode*> node;
        hkVector4 tNear;
        hkVector4 tFar;
        hkVector4Comparison activeMask;
    };

    // Casts the ray from from to to through tree, reporting each leaf it reaches to collector.
    static hkBool castRayIterative(
        const hkKdTree* tree,
        const hkVector4& from,
        const hkVector4& to,
        hkKdTreeCastCollector* collector
    );

    // Casts the four rays of a bundle, each from its lane of fromBundle to its lane of toBundle, front to back through
    // tree, the rays rayMask selects, reporting each projected entry whose box an active ray reaches before its hit
    // fraction to collector. The hkBool result is written to arResult and is always true.
    static hkBool castRayBundleIterative(
        const hkKdTree* tree,
        const RayBundle& fromBundle,
        const RayBundle& toBundle,
        hkKdTreeBundleCastCollector* collector,
        hkVector4Comparison::Mask rayMask
    );
    static hkBool castRayMultiTree(
        int32_t numTrees,
        hkKdTree* const* trees,
        const hkVector4& start,
        const hkVector4& end,
        hkKdTreeCastCollector* collector
    );
};
