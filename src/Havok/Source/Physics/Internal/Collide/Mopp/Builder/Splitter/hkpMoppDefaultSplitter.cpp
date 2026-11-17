#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppDefaultSplitter.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppAssembler.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Mediator/hkpMoppMediator.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppCostFunction.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppSplitter.h"

#include <bit>

hkpMoppDefaultSplitter::hkpMoppDefaultSplitter()
  : m_compileParams(nullptr), m_maxList(nullptr), m_freeNodes{nullptr, 0}, m_freeTerminals{nullptr, 0} {}

hkpMoppDefaultSplitter::~hkpMoppDefaultSplitter() {}

// A released node goes back on the free list of its kind.
void hkpMoppDefaultSplitter::releaseNode(hkpMoppTreeNode* nodeToRelease) {
    if (nodeToRelease) {
        // EXE-INLINED 0x5609F0
        if (nodeToRelease->m_isTerminal.m_bool) {
            m_freeTerminals.addElem(static_cast<hkpMoppTreeTerminal*>(nodeToRelease));
        }
        else {
            m_freeNodes.addElem(static_cast<hkpMoppTreeInternalNode*>(nodeToRelease));
        }
    }
}

// The freed nodes and terminals together.
int32_t hkpMoppDefaultSplitter::getFreeNodes() {
    return m_freeNodes.getNumFreeElems() + m_freeTerminals.getNumFreeElems();
}

// Fills the free lists from the scratch area, builds the tree from the root down and has the assembler assemble it.
// The max list lives on this frame for the build only.
hkpMoppTreeNode* hkpMoppDefaultSplitter::buildTree(
    hkpMoppMediator* mediator,
    hkpMoppCostFunction* costFunction,
    hkpMoppAssembler* assembler,
    const hkpMoppSplitter::hkpMoppSplitParams& in,
    hkpMoppSplitter::hkpMoppScratchArea& temp
) {
    m_costFunction = costFunction;
    m_compileParams = &in;
    m_mediator = mediator;
    m_assembler = assembler;
    const int32_t numPrimitives = mediator->getNumPrimitives();
    hkpMoppPrimitiveArray primitives;
    primitives.m_maxPrimitiveSplits = in.m_maxPrimitiveSplits;
    primitives.m_numPrimitives = numPrimitives;
    primitives.m_primitives = temp.m_primitives;
    // The optimal depth is the bit length of the primitive count.
    m_optimalDepth = 0;
    if (numPrimitives) {
        int32_t remaining = numPrimitives;
        int32_t depth = 0;
        do {
            remaining >>= 1;
            ++depth;
        }
        while (remaining);
        m_optimalDepth = depth;
    }
    // Interleaved building works in a scratch area of a fixed 4096 nodes and terminals.
    int32_t numNodes;
    if (m_compileParams->m_interleavedBuildingEnabled.m_bool) {
        numNodes = 4096;
    }
    else {
        numNodes = numPrimitives + primitives.m_maxPrimitiveSplits;
    }
    m_freeNodes.m_elems = nullptr;
    m_freeNodes.m_freeElems = 0;
    hkpMoppTreeInternalNode* node = temp.m_nodes;
    for (int32_t i = numNodes; i > 0; i--) {
        m_freeNodes.addElem(node);
        node++;
    }
    m_freeTerminals.m_elems = nullptr;
    m_freeTerminals.m_freeElems = 0;
    hkpMoppTreeTerminal* terminal = temp.m_terminals;
    for (int32_t i = numNodes; i > 0; i--) {
        m_freeTerminals.addElem(terminal);
        terminal++;
    }
    m_numPlaneDirections = assembler->getNumSplittingPlaneDirections();
    m_planeDirections = assembler->getSplittingPlaneDirections();
    const int32_t maxListSize = m_compileParams->m_maxPrimitiveSplitsPerNode + 2;
    // EXE-INLINED none: hkpMoppDefaultSplitter::hkpMoppMaxList::hkpMoppMaxList
    hkpMoppMaxList maxList;
    maxList.buffer = static_cast<hkpMoppCompilerPrimitive**>(hkMemoryRouter::easyAlloc(
        *hkMemoryRouter::getInstance().m_heap, static_cast<int32_t>(sizeof(hkpMoppCompilerPrimitive*) * maxListSize)
    ));
    maxList.m_sizeOfArray = maxListSize;
    maxList.m_currentNumElements = 0;
    maxList.lastRemovedElement = nullptr;
    m_maxList = &maxList;
    hkpMoppTreeNode* root = split(nullptr, primitives, HK_MOPP_DS_LEFT, 0);
    assembler->assemble(root, this, 2 * numNodes);
    m_maxList = nullptr;
    m_planeDirections = nullptr;
    m_numPlaneDirections = 0;
    // EXE-INLINED none: hkpMoppDefaultSplitter::hkpMoppMaxList::~hkpMoppMaxList
    hkMemoryRouter::easyFree(*hkMemoryRouter::getInstance().m_heap, maxList.buffer);
    return root;
}

// The cursors move by one primitive at a time; each case copies whole primitives through temporaries. Case 3 splits the
// current primitive at the middle of the plane positions: the far half goes to the new end of the B range, the near
// half (split along the negated direction) replaces the current one.
void hkpMoppDefaultSplitter::sortLeftAndRight(
    int32_t switchPosition,
    hkpMoppBasicNode* bestNode,
    int32_t depth,
    int32_t& maxSplits,
    int32_t& numSplits,
    hkpMoppCompilerPrimitive*& beginUnsorted,
    hkpMoppCompilerPrimitive*& endUnsorted,
    hkpMoppCompilerPrimitive*& beginRight,
    hkpMoppCompilerPrimitive*& beginMiddle
) {
    int32_t* apSplitsLeft = &maxSplits;
    int32_t* apNumSplits = &numSplits;
    hkpMoppCompilerPrimitive** appCurrent = &beginUnsorted;
    hkpMoppCompilerPrimitive** appEndA = &endUnsorted;
    hkpMoppCompilerPrimitive** appEndB = &beginRight;
    hkpMoppCompilerPrimitive** appEndC = &beginMiddle;
    switch (switchPosition) {
        case 0: {
            --*appEndB;
            --*appEndC;
            --*appEndA;
            hkpMoppCompilerPrimitive* pEndA = *appEndA;
            const hkpMoppCompilerPrimitive current = **appCurrent;
            hkpMoppCompilerPrimitive* pEndB = *appEndB;
            const hkpMoppCompilerPrimitive atEndA = *pEndA;
            *pEndB = **appEndC;
            **appEndC = current;
            if (*appCurrent != *appEndA) {
                **appCurrent = atEndA;
            }
            break;
        }
        case 1: ++*appCurrent; break;
        case 2: {
            --*appEndB;
            --*appEndA;
            hkpMoppCompilerPrimitive* pEndA = *appEndA;
            hkpMoppCompilerPrimitive* pCurrent = *appCurrent;
            hkpMoppCompilerPrimitive* pEndB = *appEndB;
            if (pCurrent != pEndB) {
                const hkpMoppCompilerPrimitive atEndA = *pEndA;
                *pEndB = *pCurrent;
                **appCurrent = atEndA;
            }
            break;
        }
        case 3: {
            --*apSplitsLeft;
            ++*apNumSplits;
            --*appEndB;
            const hkpMoppSplittingPlaneDirection* pPlane = bestNode->m_plane;
            const float planeMiddle = static_cast<float>(
                (static_cast<double>(bestNode->m_planeLeftPosition) + bestNode->m_planeRightPosition) * 0.5
            );
            m_mediator->splitPrimitive(**appCurrent, pPlane->m_direction, planeMiddle, depth, *appEndB);
            alignas(16) hkVector4 negDirection;
            // Negates by XORing each lane's bits with this file's sign mask hkQuadRealNegateMask.
            const hkQuadReal& direction = pPlane->m_direction.m_quad;
            negDirection.m_quad[0] = std::bit_cast<float>(
                std::bit_cast<uint32_t>(direction[0]) ^ std::bit_cast<uint32_t>(hkQuadRealNegateMask[0])
            );
            negDirection.m_quad[1] = std::bit_cast<float>(
                std::bit_cast<uint32_t>(direction[1]) ^ std::bit_cast<uint32_t>(hkQuadRealNegateMask[1])
            );
            negDirection.m_quad[2] = std::bit_cast<float>(
                std::bit_cast<uint32_t>(direction[2]) ^ std::bit_cast<uint32_t>(hkQuadRealNegateMask[2])
            );
            negDirection.m_quad[3] = std::bit_cast<float>(
                std::bit_cast<uint32_t>(direction[3]) ^ std::bit_cast<uint32_t>(hkQuadRealNegateMask[3])
            );
            hkpMoppCompilerPrimitive* pCurrent = *appCurrent;
            m_mediator->splitPrimitive(*pCurrent, negDirection, -planeMiddle, depth, pCurrent);
            ++*appCurrent;
            break;
        }
        default: break;
    }
}

// The node's axes ordered by length, the longest first: the longer of the other two follows it. Each axis's cost is 0.8
// times the cube of how much shorter than the longest it is, relative to it. Every rounded value is stored as a float
// and reloaded.
void hkpMoppDefaultSplitter::resortAxis(
    const hkpMoppTreeInternalNode* parentNode,
    int32_t* directionsOut,
    float* extraCostsOut
) {
    if (!parentNode) {
        directionsOut[0] = 0;
        directionsOut[1] = 1;
        directionsOut[2] = 2;
        extraCostsOut[0] = 0.0f;
        extraCostsOut[1] = 0.0f;
        extraCostsOut[2] = 0.0f;
        return;
    }
    const hkpMoppSplittingPlaneDirection* planes = m_planeDirections;
    const hkpMoppSplittingPlaneDirection* plane = parentNode->m_plane;
    float axisLength[3];
    float maxLength = 0.0f;
    int32_t longest = 0;
    // Multiplies by the double 0.66f.
    axisLength[0] = parentNode->m_extents.m_extent[0].m_max - parentNode->m_extents.m_extent[0].m_min;
    if (plane == &planes[0]) {
        axisLength[0] = axisLength[0] * 0.6600000262260437;
    }
    if (axisLength[0] > 0.0f) {
        maxLength = axisLength[0];
        longest = 0;
    }
    axisLength[1] = parentNode->m_extents.m_extent[1].m_max - parentNode->m_extents.m_extent[1].m_min;
    if (plane == &planes[1]) {
        axisLength[1] = axisLength[1] * 0.6600000262260437;
    }
    if (maxLength < axisLength[1]) {
        longest = 1;
        maxLength = axisLength[1];
    }
    axisLength[2] = parentNode->m_extents.m_extent[2].m_max - parentNode->m_extents.m_extent[2].m_min;
    if (plane == &planes[2]) {
        axisLength[2] = 0.6600000262260437 * axisLength[2];
    }
    if (axisLength[2] > maxLength) {
        longest = 2;
        maxLength = axisLength[2];
    }
    const int32_t second = (longest + 1) % 3;
    const int32_t third = (longest + 2) % 3;
    directionsOut[0] = longest;
    extraCostsOut[0] = 0.0f;
    int32_t next;
    int32_t last;
    if (axisLength[third] > axisLength[second]) {
        next = third;
        last = second;
    }
    else {
        next = second;
        last = third;
    }
    // EXE-INLINED none: hkpMoppDefaultSplitter::calcAxisLengthCost (twice)
    directionsOut[1] = next;
    float nextShortfall = maxLength - axisLength[next];
    float nextRatio = nextShortfall / maxLength;
    float nextCubed = nextRatio * (nextRatio * nextRatio) * 16.0;
    extraCostsOut[1] = nextCubed * 0.05000000074505806;
    directionsOut[2] = last;
    float lastShortfall = maxLength - axisLength[last];
    float lastRatio = lastShortfall / maxLength;
    float lastCubed = 16.0 * (lastRatio * (lastRatio * lastRatio));
    extraCostsOut[2] = 0.05000000074505806 * lastCubed;
}

// A terminal from the free list for the primitives, with its id and property ranges and its extents along the first
// three plane directions.
hkpMoppTreeTerminal* hkpMoppDefaultSplitter::createTerminal(
    hkpMoppTreeInternalNode* parentNode,
    const hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& params
) {
    hkpMoppTreeTerminal* terminal = m_freeTerminals.getElem();
    terminal->m_assemblerData.m_isAssembled.m_bool = 0;
    terminal->m_assemblerData.m_scaleIsValid.m_bool = 0;
    terminal->m_assemblerData.m_chunkId = -1;
    terminal->m_assemblerData.m_assembledAddress = -1;
    // EXE-INLINED none: hkEnum<E, S>::operator=
    terminal->m_assemblerData.m_rescaleCommandType.m_storage = hkpMoppAssemblerData::HK_MOPP_NO_RESCALE;
    terminal->m_assemblerData.m_cutCommandType[0].m_storage = hkpMoppAssemblerData::HK_MOPP_NO_CUT;
    terminal->m_assemblerData.m_cutCommandType[1].m_storage = hkpMoppAssemblerData::HK_MOPP_NO_CUT;
    terminal->m_assemblerData.m_cutCommandType[2].m_storage = hkpMoppAssemblerData::HK_MOPP_NO_CUT;
    terminal->m_isTerminal.m_bool = 1;
    terminal->m_parent = parentNode;
    terminal->m_primitive = params.m_primitives;
    calculateMaxMinId(params, terminal);
    for (int32_t i = 0; i < 3; i++) {
        m_mediator->findExtents(
            m_planeDirections[i].m_direction, i, params.m_primitives, params.m_numPrimitives,
            &terminal->m_extents.m_extent[i].m_min, &terminal->m_extents.m_extent[i].m_max
        );
    }
    return terminal;
}

// An inner node from the free list, unassembled and with no branches yet.
hkpMoppTreeInternalNode* hkpMoppDefaultSplitter::createNode() {
    hkpMoppTreeInternalNode* node = m_freeNodes.getElem();
    node->m_assemblerData.m_isAssembled.m_bool = 0;
    node->m_assemblerData.m_scaleIsValid.m_bool = 0;
    // EXE-INLINED none: hkEnum<E, S>::operator=
    node->m_assemblerData.m_rescaleCommandType.m_storage = hkpMoppAssemblerData::HK_MOPP_NO_RESCALE;
    node->m_assemblerData.m_cutCommandType[0].m_storage = hkpMoppAssemblerData::HK_MOPP_NO_CUT;
    node->m_assemblerData.m_cutCommandType[1].m_storage = hkpMoppAssemblerData::HK_MOPP_NO_CUT;
    node->m_assemblerData.m_cutCommandType[2].m_storage = hkpMoppAssemblerData::HK_MOPP_NO_CUT;
    node->m_assemblerData.m_chunkId = -1;
    node->m_assemblerData.m_assembledAddress = -1;
    node->m_isTerminal.m_bool = 0;
    node->m_bestOverallCost = 1e+06f;
    node->m_leftBranch = nullptr;
    node->m_planeRightPosition = 1e+10f;
    node->m_rightBranch = nullptr;
    return node;
}

// Havok SDK: one primitive becomes a terminal; more become an inner node, hooked to its parent (or the root) with
// the parent's extents, whose best splitting plane is searched over the three axes in the order resortAxis gives
// and then the other plane directions, while a direction's cost stays within the best cost found; the extents of
// the axes not searched are found plainly. The primitives are projected on the best plane again where another one
// was the last projected, grouped into the two sides and each side split in turn, right first.
hkpMoppTreeNode* hkpMoppDefaultSplitter::split(
    hkpMoppTreeInternalNode* parentNode,
    const hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& params,
    hkpMoppDefaultSplitter::hkpMoppDsSide side,
    int32_t depth
) {
    if (params.m_numPrimitives == 1) {
        if (m_compileParams->m_interleavedBuildingEnabled.m_bool && m_freeTerminals.getNumFreeElems() == 0) {
            m_assembler->assemble(m_rootNode, this, 2048);
        }
        return createTerminal(parentNode, params);
    }
    if (m_compileParams->m_interleavedBuildingEnabled.m_bool && m_freeNodes.getNumFreeElems() == 0) {
        m_assembler->assemble(m_rootNode, this, 2048);
    }
    hkpMoppTreeInternalNode* node = createNode();
    node->m_numPrimitives = params.m_numPrimitives;
    node->m_parent = parentNode;
    if (parentNode == nullptr) {
        m_rootNode = node;
    }
    else {
        if (side == HK_MOPP_DS_LEFT) {
            parentNode->m_leftBranch = node;
        }
        else {
            parentNode->m_rightBranch = node;
        }
        node->m_extents = parentNode->m_extents;
    }
    const hkpMoppSplittingPlaneDirection* lastPlane = nullptr;
    int32_t axisOrder[3];
    float axisCost[3];
    resortAxis(parentNode, axisOrder, axisCost);
    int32_t i = 0;
    for (; i < 3; i++) {
        const int32_t axis = axisOrder[i];
        const hkpMoppSplittingPlaneDirection* plane = &m_planeDirections[axis];
        const float cost = axisCost[i] + plane->m_cost;
        if (node->m_bestOverallCost < cost) {
            break;
        }
        hkpMoppExtent& extent = node->m_extents.m_extent[axis];
        m_mediator->projectPrimitives(
            plane->m_direction, axisOrder[i], params.m_primitives, params.m_numPrimitives, &extent.m_min, &extent.m_max
        );
        const int32_t numPrimitives = params.m_numPrimitives;
        if (numPrimitives > 1) {
            hkAlgorithm::quickSortRecursive(params.m_primitives, 0, numPrimitives - 1, uint8_t{0});
        }
        lastPlane = plane;
        findSplittingPlanePositions(node, plane, params, extent, depth);
    }
    for (; i < 3; i++) {
        const int32_t axis = axisOrder[i];
        m_mediator->findExtents(
            m_planeDirections[axis].m_direction, axis, params.m_primitives, params.m_numPrimitives,
            &node->m_extents.m_extent[axis].m_min, &node->m_extents.m_extent[axis].m_max
        );
    }
    hkpMoppExtent extent;
    for (int32_t direction = 3; direction < m_numPlaneDirections; direction++) {
        const hkpMoppSplittingPlaneDirection* plane = &m_planeDirections[direction];
        if (node->m_bestOverallCost < plane->m_cost) {
            break;
        }
        m_mediator->projectPrimitives(
            plane->m_direction, direction, params.m_primitives, params.m_numPrimitives, &extent.m_min, &extent.m_max
        );
        const int32_t numPrimitives = params.m_numPrimitives;
        if (numPrimitives > 1) {
            hkAlgorithm::quickSortRecursive(params.m_primitives, 0, numPrimitives - 1, uint8_t{0});
        }
        lastPlane = plane;
        findSplittingPlanePositions(node, plane, params, extent, depth);
    }
    calculateMaxMinId(params, node);
    if (lastPlane != node->m_plane) {
        float absMin;
        float absMax;
        m_mediator->projectPrimitives(
            node->m_plane->m_direction, node->m_plane->m_index, params.m_primitives, params.m_numPrimitives, &absMin,
            &absMax
        );
    }
    hkpMoppPrimitiveArray left;
    hkpMoppPrimitiveArray right;
    groupPrimitives(params, node, depth, left, right);
    node->m_rightBranch = split(node, right, HK_MOPP_DS_RIGHT, depth + 1);
    node->m_leftBranch = split(node, left, HK_MOPP_DS_LEFT, depth + 1);
    return node;
}

// Projects the middle primitives on the node's longest axis other than its plane's, sorts them along it and deals
// them out: each goes to the right range, or (while the left range is the shorter by a factor of 4) swaps over to the
// left one.
void hkpMoppDefaultSplitter::distributeMiddle(
    hkpMoppCompilerPrimitive*& startLeft,
    hkpMoppCompilerPrimitive*& endLeft,
    hkpMoppCompilerPrimitive*& startRight,
    hkpMoppCompilerPrimitive*& endRight,
    hkpMoppCompilerPrimitive*& startMiddle,
    hkpMoppCompilerPrimitive*& endMiddle,
    const hkpMoppBasicNode* bestNode
) {
    hkpMoppCompilerPrimitive* middle = startMiddle;
    // Each count is a signed division of the byte distance, as the game does; a plain pointer difference lets clang
    // assume a whole number of primitives.
    const int32_t numMiddle = static_cast<int32_t>(
        static_cast<intptr_t>(reinterpret_cast<uintptr_t>(endMiddle) - reinterpret_cast<uintptr_t>(startMiddle)) /
        static_cast<intptr_t>(sizeof(hkpMoppCompilerPrimitive))
    );
    if (!numMiddle) {
        return;
    }
    float maxLength = 0.0f;
    const int32_t planeIndex = bestNode->m_plane->m_index;
    int32_t axis = 0;
    if (planeIndex != 0) {
        float length = bestNode->m_extents.m_extent[0].m_max - bestNode->m_extents.m_extent[0].m_min;
        if (length > 0.0f) {
            maxLength = length;
        }
    }
    if (planeIndex != 1) {
        float length = bestNode->m_extents.m_extent[1].m_max - bestNode->m_extents.m_extent[1].m_min;
        if (maxLength < length) {
            maxLength = length;
            axis = 1;
        }
    }
    if (planeIndex != 2) {
        float length = bestNode->m_extents.m_extent[2].m_max - bestNode->m_extents.m_extent[2].m_min;
        if (maxLength < length) {
            axis = 2;
        }
    }
    float absMin;
    float absMax;
    m_mediator->projectPrimitives(m_planeDirections[axis].m_direction, axis, middle, numMiddle, &absMin, &absMax);
    if (numMiddle > 1) {
        hkAlgorithm::quickSortRecursive(startMiddle, 0, numMiddle - 1, uint8_t{0});
    }
    bool toLeft =
        static_cast<int32_t>(
            static_cast<intptr_t>(reinterpret_cast<uintptr_t>(endLeft) - reinterpret_cast<uintptr_t>(startLeft)) /
            static_cast<intptr_t>(sizeof(hkpMoppCompilerPrimitive))
        ) <
        static_cast<int32_t>(
            static_cast<intptr_t>(reinterpret_cast<uintptr_t>(endRight) - reinterpret_cast<uintptr_t>(startRight)) /
            static_cast<intptr_t>(sizeof(hkpMoppCompilerPrimitive))
        );
    while (startMiddle < endMiddle) {
        int32_t numLeft = static_cast<int32_t>(
            static_cast<intptr_t>(reinterpret_cast<uintptr_t>(endLeft) - reinterpret_cast<uintptr_t>(startLeft)) /
            static_cast<intptr_t>(sizeof(hkpMoppCompilerPrimitive))
        );
        int32_t numRight = static_cast<int32_t>(
            static_cast<intptr_t>(reinterpret_cast<uintptr_t>(endRight) - reinterpret_cast<uintptr_t>(startRight)) /
            static_cast<intptr_t>(sizeof(hkpMoppCompilerPrimitive))
        );
        if (toLeft) {
            numRight *= 4;
        }
        else {
            numLeft *= 4;
        }
        if (numLeft >= numRight) {
            ++endRight;
            ++startMiddle;
            toLeft = false;
        }
        else {
            // The last middle primitive goes to the end of the left range, the first right one to the start of the
            // middle, and the first middle one to the end of the middle; every range moves up by one.
            hkpMoppCompilerPrimitive* last = endMiddle - 1;
            const hkpMoppCompilerPrimitive lastMiddle = *last;
            *last = *startMiddle;
            const hkpMoppCompilerPrimitive firstRight = *startRight;
            *endLeft = lastMiddle;
            *startMiddle = firstRight;
            ++endLeft;
            ++startRight;
            ++startMiddle;
            ++endRight;
            toLeft = true;
        }
    }
}

// Havok SDK: sorts the primitives into left, right and middle ranges against the node's plane positions (splitting
// the ones that straddle both while splits are left), spreads the middle ones over the two sides and gives each side
// its primitives and its share of the splits left, in proportion to its primitives. The right primitives move up
// past the room the left side's splits need, unless the right side gets no splits.
void hkpMoppDefaultSplitter::groupPrimitives(
    const hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& in,
    hkpMoppBasicNode* bestNode,
    int32_t depth,
    hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& leftOut,
    hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& rightOut
) {
    hkpMoppCompilerPrimitive* startLeft = in.m_primitives;
    hkpMoppCompilerPrimitive* endLeft = in.m_primitives + in.m_numPrimitives;
    hkpMoppCompilerPrimitive* startRight = in.m_primitives + (in.m_numPrimitives + in.m_maxPrimitiveSplits);
    hkpMoppCompilerPrimitive* startMiddle = startRight;
    hkpMoppCompilerPrimitive* endMiddle = startRight;
    int32_t splitsLeft = in.m_maxPrimitiveSplits;
    int32_t numSplits = 0;
    hkpMoppCompilerPrimitive* current = in.m_primitives;
    while (current < endLeft) {
        int32_t side;
        if (bestNode->m_planeRightPosition > current->m_extent.m_min) {
            side = !(bestNode->m_planeLeftPosition >= current->m_extent.m_max) ? 3 : 1;
        }
        else {
            side = bestNode->m_planeLeftPosition < current->m_extent.m_max ? 2 : 0;
        }
        sortLeftAndRight(side, bestNode, depth, splitsLeft, numSplits, current, endLeft, startRight, startMiddle);
    }
    hkpMoppCompilerPrimitive* endRight = startMiddle;
    distributeMiddle(startLeft, endLeft, startRight, endRight, startMiddle, endMiddle, bestNode);
    // The counts are signed divisions of the byte distances, as the game does; a plain pointer difference lets clang
    // assume a whole number of primitives.
    const int32_t numLeft = static_cast<int32_t>(
        static_cast<intptr_t>(reinterpret_cast<uintptr_t>(endLeft) - reinterpret_cast<uintptr_t>(startLeft)) /
        static_cast<intptr_t>(sizeof(hkpMoppCompilerPrimitive))
    );
    leftOut.m_numPrimitives = numLeft;
    const float leftSplits =
        static_cast<double>(numLeft) * static_cast<double>(splitsLeft) / static_cast<double>(in.m_numPrimitives);
    leftOut.m_maxPrimitiveSplits = hkMath::hkFloatToInt(leftSplits);
    rightOut.m_numPrimitives = static_cast<int32_t>(
        static_cast<intptr_t>(reinterpret_cast<uintptr_t>(endRight) - reinterpret_cast<uintptr_t>(startRight)) /
        static_cast<intptr_t>(sizeof(hkpMoppCompilerPrimitive))
    );
    rightOut.m_maxPrimitiveSplits = splitsLeft - leftOut.m_maxPrimitiveSplits;
    leftOut.m_primitives = startLeft;
    hkpMoppCompilerPrimitive* dest = endLeft + leftOut.m_maxPrimitiveSplits;
    rightOut.m_primitives = dest;
    if (rightOut.m_maxPrimitiveSplits != 0) {
        for (hkpMoppCompilerPrimitive* source = startRight; source < endRight; source++) {
            *dest = *source;
            dest++;
        }
    }
}

// The primitives' id range and each property's value range.
void hkpMoppDefaultSplitter::calculateMaxMinId(
    const hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& params,
    hkpMoppTreeNode* node
) {
    const hkpMoppCompilerPrimitive* primitive = params.m_primitives;
    uint32_t minId = primitive->m_primitiveID;
    uint32_t maxId = minId;
    node->m_numProperties = 0;
    node->m_minPropertyValue[0] = 0xFFFFFFFF;
    node->m_maxPropertyValue[0] = 0;
    for (int32_t remaining = params.m_numPrimitives - 1; remaining >= 0; remaining--) {
        if (primitive->m_primitiveID > maxId) {
            maxId = primitive->m_primitiveID;
        }
        if (primitive->m_primitiveID < minId) {
            minId = primitive->m_primitiveID;
        }
        uint32_t properties[1];
        const int32_t numProperties = m_mediator->getPrimitiveProperties(*primitive, properties);
        if (numProperties > node->m_numProperties) {
            node->m_numProperties = numProperties;
        }
        // Walks the max values and reaches each min value one element before its max.
        uint32_t* maxValue = node->m_maxPropertyValue;
        for (int32_t p = 0; p < numProperties; p++) {
            const uint32_t value = properties[p];
            if (value < maxValue[-1]) {
                maxValue[-1] = value;
            }
            if (value > *maxValue) {
                *maxValue = value;
            }
            maxValue++;
        }
        primitive++;
    }
    node->m_minPrimitiveId = minId;
    node->m_numPrimitives = params.m_numPrimitives;
    node->m_maxPrimitiveId = maxId;
}

// For each primitive as the right plane, tries the max list's elements as the left plane (all of them, or a window
// around the last best one when checkAllEveryN is set) and keeps the cheapest split in the node. Every rounded value is
// stored and reloaded as a float.
void hkpMoppDefaultSplitter::findSplittingPlanePositions(
    hkpMoppBasicNode* node,
    const hkpMoppSplittingPlaneDirection* plane,
    const hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& primitives,
    const hkpMoppExtent& extents,
    int32_t treeDepth
) {
    hkpMoppCostFunction::hkpPlanesParams params;
    params.m_currentDepth = treeDepth;
    params.m_optimalDepth = m_optimalDepth;
    float absoluteMin = extents.m_min;
    params.m_absoluteMin = absoluteMin;
    float absoluteMax = extents.m_max;
    params.m_absoluteMax = absoluteMax;
    const int32_t numPrimitives = primitives.m_numPrimitives;
    params.m_numPrimitives = numPrimitives;
    params.m_plane = plane;
    params.m_minPrimitiveId = primitives.m_primitives->m_primitiveID;
    params.m_maxPrimitiveId = params.m_minPrimitiveId;
    int32_t maxSplits = primitives.m_maxPrimitiveSplits;
    int32_t lastBest = 0;
    params.m_numPrimitivesInv = 1.0 / static_cast<double>(numPrimitives);
    float extentLength = extents.m_max - extents.m_min;
    if (!(extentLength < 0.001f)) {
        params.m_extentsInv = 1.0 / extentLength;
    }
    else {
        params.m_extentsInv = 1.0f;
    }
    if (maxSplits > m_compileParams->m_maxPrimitiveSplitsPerNode) {
        maxSplits = m_compileParams->m_maxPrimitiveSplitsPerNode;
    }
    for (int32_t i = 0; i < primitives.m_numPrimitives; i++) {
        const hkpMoppCompilerPrimitive& right = primitives.m_primitives[i];
        params.m_currentNumPrimitivesRight = i;
        float planeRight = right.m_extent.m_min;
        params.m_planeRightPosition = planeRight;
        const uint32_t id = right.m_primitiveID;
        if (params.m_minPrimitiveId >= id) {
            params.m_minPrimitiveId = id;
        }
        if (params.m_maxPrimitiveId <= id) {
            params.m_maxPrimitiveId = id;
        }
        m_maxList->removeElementsLessThan(params.m_planeRightPosition);
        // EXE-INLINED none: hkpMoppDefaultSplitter::hkpMoppMaxList::removeOverflowElements
        hkpMoppMaxList* maxList = m_maxList;
        for (int32_t k = maxList->m_currentNumElements - 1; k > maxSplits; k--) {
            maxList->lastRemovedElement = maxList->buffer[k];
            maxList->buffer[k] = nullptr;
            maxList->m_currentNumElements--;
        }
        const int32_t minRangeMaxListCheck = m_compileParams->m_minRangeMaxListCheck;
        hkpMoppCostFunction* costFunction = m_costFunction;
        const int32_t checkAllEveryN = m_compileParams->m_checkAllEveryN;
        float rightCost = costFunction->costUnbalancedPlaneRight(params);
        float directionCost = params.m_plane->m_cost;
        // The compare of the id spread is signed; the factor is the double value of -0.03f.
        float idSpreadCost;
        if (static_cast<int32_t>(params.m_maxPrimitiveId - params.m_minPrimitiveId) < 16) {
            idSpreadCost = costFunction->m_userScaling.m_weightPrimitiveIdSpread * -0.029999999329447746;
        }
        else {
            idSpreadCost = 0.0;
        }
        const int32_t numElements = m_maxList->m_currentNumElements;
        const int32_t halfRange = minRangeMaxListCheck >> 1;
        int32_t j = 0;
        int32_t lowest = 0;
        float baseCost = directionCost + rightCost + idSpreadCost;
        float bestCost = 10000000.0f;
        // Whether there is a left plane to try, and the max list's element that gives it.
        bool evaluate = false;
        const hkpMoppCompilerPrimitive* left = nullptr;
        if (numElements == 0) {
            left = m_maxList->lastRemovedElement;
            evaluate = left != nullptr;
        }
        else if (!(node->m_bestOverallCost < baseCost)) {
            int32_t highest;
            if (checkAllEveryN > 0) {
                highest = halfRange + lastBest;
                if (highest > numElements) {
                    highest = numElements;
                }
                lowest = lastBest - halfRange;
                if (lowest < 0) {
                    lowest = 0;
                }
            }
            else {
                highest = numElements;
            }
            j = highest - 1;
            if (j < lowest) {
                lastBest--;
            }
            else {
                evaluate = true;
            }
        }
        while (evaluate) {
            if (numElements != 0) {
                // EXE-INLINED none: hkpMoppDefaultSplitter::hkpMoppMaxList::elementAt
                hkpMoppMaxList* list = m_maxList;
                left = (j > list->m_currentNumElements) ? nullptr : list->buffer[j];
            }
            float planeLeft = left->m_extent.m_max;
            params.m_planeLeftPosition = planeLeft;
            params.m_numSplitPrimitives = j;
            params.m_currentNumPrimitivesLeft = primitives.m_numPrimitives - i + j;
            float leftCost = costFunction->costUnbalancedPlaneLeft(params);
            float unbalancedCost = costFunction->costNumUnBalanced(params);
            float splitRatio = static_cast<double>(j) * params.m_numPrimitivesInv;
            float splitCost = splitRatio * 5.0 * costFunction->m_userScaling.m_weightPrimitiveSplit;
            float leftPosition = params.m_planeLeftPosition;
            float rightPosition = params.m_planeRightPosition;
            float distance = leftPosition - rightPosition;
            float scaledDistance = distance * params.m_extentsInv;
            float distanceCost = scaledDistance * costFunction->m_userScaling.m_weightPlaneDistance;
            float sum = unbalancedCost + leftCost + splitCost + distanceCost;
            float total = sum + baseCost;
            if (bestCost > total) {
                bestCost = total;
                lastBest = j;
                if (node->m_bestOverallCost > total) {
                    node->m_bestOverallCost = total;
                    node->m_planeRightPosition = rightPosition;
                    node->m_plane = params.m_plane;
                    node->m_planeLeftPosition = leftPosition;
                    const int32_t low = j - halfRange;
                    if (low < lowest) {
                        lowest = low;
                        if (lowest < 0) {
                            lowest = 0;
                        }
                    }
                }
            }
            j--;
            if (j < lowest) {
                lastBest--;
                break;
            }
        }
        m_maxList->addElement(&primitives.m_primitives[i]);
    }
    // EXE-INLINED none: hkpMoppDefaultSplitter::hkpMoppMaxList::removeAllElements
    hkpMoppMaxList* maxList = m_maxList;
    for (int32_t k = maxList->m_currentNumElements - 1; k > 0; k--) {
        maxList->buffer[k] = nullptr;
    }
    maxList->m_currentNumElements = 0;
    maxList->lastRemovedElement = nullptr;
}

void hkpMoppDefaultSplitter::hkpMoppMaxList::addElement(hkpMoppCompilerPrimitive* compPrimitive) {
    int32_t v2 = m_currentNumElements - 1;
    double m_max = compPrimitive->m_extent.m_max;
    if (m_currentNumElements < 4) {
    LABEL_11:;
        if (v2 >= 0) {
            while (1) {
                hkpMoppCompilerPrimitive** v11 = &this->buffer[v2];
                if (!*v11 || !((*v11)->m_extent.m_max < m_max)) {
                    break;
                }
                --v2;
                v11[1] = *v11;
                if (v2 < 0) {
                    this->buffer[v2 + 1] = compPrimitive;
                    ++m_currentNumElements;
                    return;
                }
            }
        }
    }
    else {
        while (1) {
            hkpMoppCompilerPrimitive* v6;
            hkpMoppCompilerPrimitive* v8;
            hkpMoppCompilerPrimitive* v10;
            hkpMoppCompilerPrimitive** v4 = &this->buffer[v2];
            if (!*v4 || !((*v4)->m_extent.m_max < m_max)) {
                break;
            }
            v4[1] = *v4;
            hkpMoppCompilerPrimitive** v5 = &this->buffer[v2];
            if (!*(v5 - 1) || (v6 = *(v5 - 1), !(v6->m_extent.m_max < m_max))) {
                this->buffer[v2] = compPrimitive;
                ++m_currentNumElements;
                return;
            }
            *v5 = v6;
            hkpMoppCompilerPrimitive** buffer = this->buffer;
            if (!buffer[v2 - 2] || (v8 = buffer[v2 - 2], !(v8->m_extent.m_max < m_max))) {
                this->buffer[v2 - 1] = compPrimitive;
                ++m_currentNumElements;
                return;
            }
            buffer[v2 - 1] = v8;
            hkpMoppCompilerPrimitive** v9 = this->buffer;
            if (!v9[v2 - 3] || (v10 = v9[v2 - 3], !(v10->m_extent.m_max < m_max))) {
                v2 -= 3;
                break;
            }
            v9[v2 - 2] = v10;
            v2 -= 4;
            if (v2 < 3) {
                goto LABEL_11;
            }
        }
    }
    this->buffer[v2 + 1] = compPrimitive;
    ++m_currentNumElements;
}

void hkpMoppDefaultSplitter::hkpMoppMaxList::removeElementsLessThan(float position) {
    double v2 = position;
    int32_t v3 = m_currentNumElements - 1;
    if (m_currentNumElements < 4) {
    LABEL_11:;
        while (v3 >= 0) {
            hkpMoppCompilerPrimitive** v11 = &this->buffer[v3];
            if (!*v11) {
                break;
            }
            if (!((*v11)->m_extent.m_max < v2)) {
                break;
            }
            lastRemovedElement = *v11;
            *v11 = 0;
            --m_currentNumElements;
            --v3;
        }
    }
    else {
        while (1) {
            hkpMoppCompilerPrimitive** v4 = &this->buffer[v3];
            if (!*v4) {
                break;
            }
            if (!((*v4)->m_extent.m_max < v2)) {
                break;
            }
            lastRemovedElement = *v4;
            *v4 = 0;
            hkpMoppCompilerPrimitive** buffer = this->buffer;
            --m_currentNumElements;
            hkpMoppCompilerPrimitive** v6 = &buffer[v3 - 1];
            if (!*v6) {
                break;
            }
            if (!((*v6)->m_extent.m_max < v2)) {
                break;
            }
            lastRemovedElement = *v6;
            *v6 = 0;
            hkpMoppCompilerPrimitive** v7 = this->buffer;
            --m_currentNumElements;
            hkpMoppCompilerPrimitive** v8 = &v7[v3 - 2];
            if (!*v8) {
                break;
            }
            if (!((*v8)->m_extent.m_max < v2)) {
                break;
            }
            lastRemovedElement = *v8;
            *v8 = 0;
            hkpMoppCompilerPrimitive** v9 = this->buffer;
            --m_currentNumElements;
            hkpMoppCompilerPrimitive** v10 = &v9[v3 - 3];
            if (!*v10 || !((*v10)->m_extent.m_max < v2)) {
                break;
            }
            lastRemovedElement = *v10;
            *v10 = 0;
            --m_currentNumElements;
            v3 -= 4;
            if (v3 < 3) {
                goto LABEL_11;
            }
        }
    }
}

// The pivot is the middle primitive, copied whole; only its extent's minimum is compared. The swap goes through a
// temporary, in the game's order.
void hkAlgorithm::quickSortRecursive(hkpMoppCompilerPrimitive* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess) {
    int32_t d = aiLow;
    for (;;) {
        int32_t i = d;
        int32_t j = aiHigh;
        alignas(16) const hkpMoppCompilerPrimitive str = apArr[(d + aiHigh) >> 1];
        do {
            // EXE-INLINED none: hkAlgorithm::less<hkpMoppCompilerPrimitive>::operator()
            while (apArr[i].m_extent.m_min < str.m_extent.m_min) {
                i++;
            }
            while (str.m_extent.m_min < apArr[j].m_extent.m_min) {
                j--;
            }
            if (j < i) {
                break;
            }
            if (j != i) {
                alignas(16) const hkpMoppCompilerPrimitive zal = apArr[j];
                apArr[j] = apArr[i];
                apArr[i] = zal;
            }
            j--;
            i++;
        }
        while (i <= j);
        if (d < j) {
            hkAlgorithm::quickSortRecursive(apArr, d, j, aiLess);
        }
        if (i >= aiHigh) {
            return;
        }
        d = i;
    }
}
