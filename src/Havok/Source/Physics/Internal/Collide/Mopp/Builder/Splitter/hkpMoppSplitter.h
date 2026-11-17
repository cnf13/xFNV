#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkpMoppAssembler;
class hkpMoppCostFunction;
class hkpMoppMediator;
class hkpMoppTreeInternalNode;

// The kind of mesh a MOPP is built for, which the split and cost parameters' constructors take. Values from Havok's
// release hkpinternal.pdb (hkpMoppSplitter, hkpMoppCostFunction and hkpMoppCompiler use it).
enum hkpMoppMeshType : int32_t {
    HK_MOPP_MT_LANDSCAPE = 0x0,
    HK_MOPP_MT_INDOOR = 0x1,
};

// A primitive's extent along one splitting direction.
struct hkpMoppExtent {
    float m_min;
    float m_max;
};

// What the MOPP compiler sees of one primitive: its ids and its extent along the current direction.
struct hkpMoppCompilerPrimitive {
    uint32_t m_primitiveID;
    uint32_t m_primitiveID2;
    hkpMoppExtent m_extent;
    uint32_t m_origPrimitiveID;
};

// One of the plane directions the splitter cuts along, with its cost and index.
struct hkpMoppSplittingPlaneDirection {
    hkVector4 m_direction;
    float m_cost;
    int32_t m_index;
};

// The assembler's per-node state.
struct hkpMoppAssemblerData {
    enum hkpCutCommandType : int32_t {
        HK_MOPP_NO_CUT = 0x0,
        HK_MOPP_CUT8 = 0x1,
        HK_MOPP_CUT24 = 0x2,
    };

    enum hkpRescaleCommandType : int32_t {
        HK_MOPP_NO_RESCALE = 0x0,
        HK_MOPP_RESCALE = 0x1,
    };

    hkBool m_isAssembled;
    hkBool m_scaleIsValid;
    hkEnum<hkpRescaleCommandType, int8_t> m_rescaleCommandType;
    hkEnum<hkpCutCommandType, int8_t> m_cutCommandType[3];
    int32_t m_chunkId;
    int32_t m_minCutPlanePosition[3];
    int32_t m_maxCutPlanePosition[3];
    int32_t m_assembledAddress;
};

// A node of the tree the MOPP builder builds.
class hkpMoppTreeNode {
public:
    // The node's extents along the three axes.
    struct hkpMopp3DOPExtents {
        hkpMoppExtent m_extent[3];
    };

    hkpMoppTreeInternalNode* m_parent;
    hkBool m_isTerminal;
    int32_t m_numPrimitives;
    hkpMopp3DOPExtents m_extents;
    uint32_t m_minPrimitiveId;
    uint32_t m_maxPrimitiveId;
    int32_t m_numProperties;
    uint32_t m_minPropertyValue[1];
    uint32_t m_maxPropertyValue[1];
    hkpMoppAssemblerData m_assemblerData;
};

// A node with its splitting plane: the direction and the right and left plane positions along it.
class hkpMoppBasicNode : public hkpMoppTreeNode {
public:
    // The cost terms of a candidate split, each weighted by the cost function's parameters. Members and layout from
    // Havok's release hkpinternal.pdb (sizeof 0x24).
    struct hkpMoppCostInfo {
        float m_splitCost;
        float m_planeRightPositionCost;
        float m_primitiveIdSpread;
        float m_planeLeftPositionCost;
        float m_numUnBalancedCost;
        float m_planeDistanceCost;
        float m_absoluteMin;
        float m_absoluteMax;
        float m_directionCost;
    };

    const hkpMoppSplittingPlaneDirection* m_plane;
    float m_planeRightPosition;
    float m_planeLeftPosition;
    float m_bestOverallCost;
};

// A leaf of the tree: the primitive it holds.
class hkpMoppTreeTerminal : public hkpMoppTreeNode {
public:
    hkpMoppCompilerPrimitive* m_primitive;
};

// An inner node of the tree: its splitting plane and its two branches. m_numPrimitives is the node's own, separate from
// hkpMoppTreeNode's.
class hkpMoppTreeInternalNode : public hkpMoppBasicNode {
public:
    hkpMoppTreeNode* m_leftBranch;
    hkpMoppTreeNode* m_rightBranch;
    int32_t m_numPrimitives;
};

// Hands out tree nodes and takes them back.
class hkpMoppNodeMgr : public hkReferencedObject {
public:
    ~hkpMoppNodeMgr() override;

    virtual void releaseNode(hkpMoppTreeNode* nodeToRelease) = 0;
    virtual int32_t getFreeNodes() = 0;
};

// The MOPP builder's splitter, which cuts the primitives into a tree.
class hkpMoppSplitter : public hkpMoppNodeMgr {
public:
    struct hkpMoppScratchArea;

    // How the splitter may cut primitives: the tolerance, the limits on primitive splits and when to check every
    // split.
    struct hkpMoppSplitParams {
        hkpMoppSplitParams() = default;
        hkpMoppSplitParams(int32_t a1);

        float m_tolerance;
        int32_t m_maxPrimitiveSplits;
        int32_t m_maxPrimitiveSplitsPerNode;
        int32_t m_minRangeMaxListCheck;
        int32_t m_checkAllEveryN;
        hkBool m_interleavedBuildingEnabled;
        uint8_t padding15[3];
    };

    ~hkpMoppSplitter() override;

    virtual hkpMoppTreeNode* buildTree(
        hkpMoppMediator* mediator,
        hkpMoppCostFunction* costFunction,
        hkpMoppAssembler* assembler,
        const hkpMoppSplitter::hkpMoppSplitParams& params,
        hkpMoppSplitter::hkpMoppScratchArea& scratchArea
    ) = 0;
};

// The buffers a build's splitter works in: a copy of the primitives, the tree's inner nodes and its leaves.
struct hkpMoppSplitter::hkpMoppScratchArea {
    hkpMoppCompilerPrimitive* m_primitives;
    hkpMoppTreeInternalNode* m_nodes;
    hkpMoppTreeTerminal* m_terminals;
};
