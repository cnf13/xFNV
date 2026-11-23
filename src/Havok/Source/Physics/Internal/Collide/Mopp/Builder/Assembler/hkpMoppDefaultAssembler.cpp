#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppDefaultAssembler.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppCompilerChunkInfo.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppAssembler.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppCodeGenerator.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Mediator/hkpMoppMediator.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppSplitter.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <limits>
#include <xmmintrin.h>

// Floor computed with integer bit operations.
static float hkFloor(float r);
static int32_t hkFloorToInt(float r);
static void validateJumpCommands();

// Shifts by the exponent use its low five bits.
static float hkFloor(float r) {
    float fArgument = r;
    int32_t iBits = std::bit_cast<int32_t>(fArgument);
    int32_t iExponent = static_cast<int32_t>((static_cast<uint32_t>(iBits) >> 23) & 0xFF) - 127;
    // Zero for +-0, so that -0 floors to +0.
    int32_t iNonZero = ~(((iBits & 0x7FFFFFFF) - 1) >> 31);
    int32_t iValue = iBits & iNonZero;
    // All ones while the exponent leaves fraction bits in the mantissa (at most 23).
    int32_t iHasFractionBits = ~((23 - iExponent) >> 31);
    int32_t iShift = (iHasFractionBits & 23) - (iHasFractionBits & (23 - iExponent));
    int32_t iIntegerMask = (static_cast<int32_t>(0xFF800000u) >> (iShift & 31)) | ~iHasFractionBits;
    // Negative when the value has a fraction: then a negative value moves one unit down before the fraction goes.
    int32_t iHasFraction = ~((iValue & ~iIntegerMask) - 1);
    int32_t iUnit = 0x800000 >> (iExponent & 31);
    int32_t iNegative = iValue >> 31;
    int32_t iBelowOne = iExponent >> 31;
    int32_t iResult = (((iHasFraction & iUnit & iNegative) + iValue) & ~iBelowOne & iIntegerMask) |
        (iBelowOne & iNegative & static_cast<int32_t>(0xBF800000u));
    return std::bit_cast<float>(iResult);
}

// Floors to an integer with integer bit operations. Every shift uses its count's low five bits.
static int32_t hkFloorToInt(float r) {
    float fArgument = r;
    int32_t iBits = std::bit_cast<int32_t>(fArgument);
    int32_t iExponent = static_cast<int32_t>((static_cast<uint32_t>(iBits) >> 23) & 0xFF) - 127;
    // All ones while the exponent leaves fraction bits in the mantissa (at most 23).
    int32_t iHasFractionBits = (iExponent - 24) >> 31;
    int32_t iLeftShift = (iExponent - 23) & ~iHasFractionBits;
    int32_t iRightShift = (23 - iExponent) & iHasFractionBits;
    // Zero for +-0.
    int32_t iNonZero = ~(((iBits & 0x7FFFFFFF) - 1) >> 31);
    int32_t iValue = iBits & iNonZero;
    int32_t iMaskShift = (iHasFractionBits & 23) - iRightShift;
    int32_t iIntegerMask = (static_cast<int32_t>(0xFF800000u) >> (iMaskShift & 31)) | ~iHasFractionBits;
    int32_t iBelowOne = iExponent >> 31;
    int32_t iNegative = iValue >> 31;
    int32_t iUnit = 0x800000 >> (iExponent & 31);
    // Negative when the value has a fraction: then a negative value moves one unit down before the fraction goes.
    int32_t iHasFraction = ~((iValue & ~iIntegerMask) - 1);
    int32_t iMantissa = ((iHasFraction & iUnit & iNegative) + (iValue & ~iBelowOne)) & 0x7FFFFF;
    // The implicit one: kept from one up, and set for a negative value below one (which floors to -1).
    int32_t iImplicit = ((iBelowOne & iNegative) | ~iBelowOne) & 0x800000;
    int32_t iSignificand = (iMantissa | iImplicit) & iIntegerMask;
    // Negated when negative: ((2s - 1) | -1) - s + 1 = -s.
    int32_t iSigned = (((2 * iSignificand - 1) | iNegative) - iSignificand) + 1;
    return (iSigned >> (iRightShift & 31)) << (iLeftShift & 31);
}

static void validateJumpCommands() {
    ;
}

void appendJumpCommandsWithOffset(
    const hkArray<hkpMoppCodeGenerator::JumpCommandInfo, hkContainerHeapAllocator>* apSource,
    int32_t aiOffset,
    hkArray<hkpMoppCodeGenerator::JumpCommandInfo, hkContainerHeapAllocator>* apDest
) {
    for (int32_t i = 0; i < apSource->m_size; i++) {
        hkpMoppCodeGenerator::JumpCommandInfo& info = apDest->expandOne();
        info.m_chunkId = apSource->m_data[i].m_chunkId;
        info.m_position = apSource->m_data[i].m_position + aiOffset;
    }
}

int32_t copyCodeAndEmbedTerminals(
    hkpMoppCompilerChunkInfo* apChunkInfo,
    hkpMoppCodeGenerator* apChunk,
    hkArray<uint32_t, hkContainerHeapAllocator>* apTerminals,
    hkpMoppCodeGenerator* dest
) {
    int32_t padding = -apChunk->m_pos & 3;
    if (!apChunkInfo->m_compressor) {
        apTerminals->m_size = 0;
    }
    if (apTerminals->m_size) {
        int32_t size = apChunkInfo->m_compressor->calcPrimitivesSize(apTerminals->m_data, apTerminals->m_size);
        dest->pad(size);
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        // The stack allocator hands out untyped bytes, which are taken as the primitives' buffer.
        uint8_t* pPrimitives = reinterpret_cast<uint8_t*>(router.m_stack.m_next.m_storage);
        router.m_stack.m_next.m_storage += (size + 127) & ~127;
        apChunkInfo->m_compressor->storePrimitives(apTerminals->m_data, apTerminals->m_size, pPrimitives);
        hkString::memCpy(&dest->m_code[dest->m_size - dest->m_pos], pPrimitives, size);
        hkDeallocateStack(reinterpret_cast<char*>(pPrimitives));
    }
    int32_t startPos = dest->m_pos;
    dest->pad(padding + apChunk->m_pos);
    hkString::memCpy(
        &dest->m_code[dest->m_size - dest->m_pos],
        &apChunk->m_code[apChunk->m_size - apChunk->m_pos],
        apChunk->m_pos
    );
    validateJumpCommands();
    appendJumpCommandsWithOffset(&apChunk->m_jumpCommands, dest->m_pos - apChunk->m_pos, &dest->m_jumpCommands);
    validateJumpCommands();
    return dest->m_pos - startPos;
}

hkpMoppAssemblerNodeInfo::hkpMoppAssemblerNodeInfo(
    const hkpMoppAssemblerNodeInfo& arParent,
    const hkpMoppTreeNode* apNode,
    const hkpMoppCode::CodeInfo& arCodeInfo
)
  : hkpMoppAssemblerNodeInfo(arParent) {
    updateOffsetAndSpread(apNode);
    updateExtents(apNode, arCodeInfo);
    m_level = arParent.m_level + 1;
    m_ignoreBandControlCode = false;
    calcMaxExtentsSignificantBits();
}

void hkpMoppAssemblerNodeInfo::updateOffsetAndSpread(const hkpMoppTreeNode* node) {
    m_terminalSpread = node->m_maxPrimitiveId - node->m_minPrimitiveId;
    int32_t v2 = 0;
    m_numProperties = node->m_numProperties;
    if (node->m_numProperties > 0) {
        int32_t* m_propertiesSpread = this->m_propertiesSpread;
        const uint32_t* m_minPropertyValue = node->m_minPropertyValue;
        do {
            m_propertiesSpread[-1] = m_minPropertyValue[0];
            ++v2;
            m_propertiesSpread[0] = m_minPropertyValue[1] - m_minPropertyValue[0];
            ++m_minPropertyValue;
            ++m_propertiesSpread;
        }
        while (v2 < node->m_numProperties);
    }
}

void hkpMoppAssemblerNodeInfo::updateExtents(const hkpMoppTreeNode* node, const hkpMoppCode::CodeInfo& codeInfo) {
    for (int32_t i = 0; i < 3; ++i) {
        const float fMin = (node->m_extents.m_extent[i].m_min - XFNV_M128_F32(codeInfo.m_offset.m_quad, i)) *
            XFNV_M128_F32(codeInfo.m_offset.m_quad, 3);
        m_extents[i][0] = hkFloorToInt(fMin);
        const float fMax = (node->m_extents.m_extent[i].m_max - XFNV_M128_F32(codeInfo.m_offset.m_quad, i)) *
            XFNV_M128_F32(codeInfo.m_offset.m_quad, 3);
        m_extents[i][1] = hkFloorToInt(fMax) + 1;
    }
}

void hkpMoppAssemblerNodeInfo::calcMaxExtentsSignificantBits() {
    int32_t v2 = m_extents[0][1] - m_extents[0][0];
    int32_t v3 = m_extents[1][1] - m_extents[1][0];
    if (v2 <= v3) {
        v2 = v3;
    }
    if (v2 <= m_extents[2][1] - m_extents[2][0]) {
        v2 = m_extents[2][1] - m_extents[2][0];
    }
    char v4 = 0;
    uint32_t v5 = v2;
    if (v2) {
        do {
            v5 >>= 1;
            ++v4;
        }
        while (v5);
    }
    int32_t v6 = -1;
    int32_t v7 = 3;
    int32_t v8 = v2 + (1 << (v4 - 4));
    do {
        uint32_t v9 = v8;
        int32_t v10 = 0;
        if (v8) {
            do {
                v9 >>= 1;
                ++v10;
            }
            while (v9);
        }
        if (v10 > v6) {
            v6 = v10;
        }
        --v7;
    }
    while (v7);
    if (v6 <= 24) {
        m_maxExtentsSignificantBits = v6;
    }
    else {
        m_maxExtentsSignificantBits = 24;
    }
}

void hkpMoppAssemblerCutInfo::calcPossibleCutPlanes() {
    int32_t m_currentBitsResolution = this->m_currentBitsResolution;
    int32_t v3 = m_extents[0][0] >> m_currentBitsResolution << m_currentBitsResolution;
    int32_t v4 = this->m_currentBitsResolution;
    int32_t v5 = ((m_extents[1][1] >> v4) + 1) << v4;
    int32_t v6 = this->m_currentBitsResolution;
    int32_t v7 = ((m_extents[2][1] >> v6) + 1) << v6;
    m_highCutPlanePosition[0] = ((m_extents[0][1] >> m_currentBitsResolution) + 1) << m_currentBitsResolution;
    m_lowCutPlanePosition[0] = v3;
    m_lowCutPlanePosition[1] = m_extents[1][0] >> v4 << v4;
    m_highCutPlanePosition[1] = v5;
    m_lowCutPlanePosition[2] = m_extents[2][0] >> v6 << v6;
    m_highCutPlanePosition[2] = v7;
}

// The 13 splitting plane directions (the axes, the face and the corner diagonals), normalized, each with its cost.
hkpMoppDefaultAssembler::hkpMoppDefaultAssembler(
    const hkpMoppAssembler::hkpMoppAssemblerParams& ap,
    hkpMoppCodeGenerator* code,
    hkpMoppMediator* mediator
)
  : m_mediator(mediator), m_nodeMgr(nullptr), m_code(code) {
    static const float kDirections[13][4] = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, -1.0f, 0.0f},
        {1.0f, 0.0f, -1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, -1.0f, 0.0f},
        {1.0f, -1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, -1.0f, 0.0f},
    };
    static const float kCosts[13] = {
        0.0f,
        0.0f,
        0.0f,
        0.2f,
        0.2f,
        0.2f,
        0.25f,
        0.25f,
        0.25f,
        0.3f,
        0.32f,
        0.32f,
        0.34f,
    };
    m_directions.reserveExactly(13);
    m_directions.setSize(13);
    for (int32_t i = 0; i < 13; ++i) {
        hkpMoppSplittingPlaneDirection& rDirection = m_directions.m_data[i];
        rDirection.m_direction.set(kDirections[i][0], kDirections[i][1], kDirections[i][2], kDirections[i][3]);
        rDirection.m_direction.normalize3();
        rDirection.m_cost = kCosts[i];
        rDirection.m_index = i;
    }
    updateParams(ap);
}

// The directions free themselves.
hkpMoppDefaultAssembler::~hkpMoppDefaultAssembler() {}

int32_t hkpMoppDefaultAssembler::getNumSplittingPlaneDirections() const {
    return HK_MOPP_SD_MAX;
}

void hkpMoppDefaultAssembler::assemble(hkpMoppTreeNode* rootNode, hkpMoppNodeMgr* nodeMgr, int32_t minAssembledElems) {
    m_nodeMgr = nodeMgr;
    m_minAssembledElems = minAssembledElems;
    m_numNodesAssembled = 0;
    getScaleInfo(rootNode, &m_codeInfo);
    if (!rootNode) {
        return;
    }
    // The tree's depth: the bits of the largest primitive id.
    m_minDepthToBuild = 0;
    uint32_t uiMaxPrimitiveId = rootNode->m_maxPrimitiveId;
    if (uiMaxPrimitiveId) {
        int32_t iBits = 0;
        do {
            uiMaxPrimitiveId >>= 1;
            ++iBits;
        }
        while (uiMaxPrimitiveId);
        m_minDepthToBuild = iBits;
    }
    const int32_t iTreeDepth = m_minDepthToBuild;
    m_halfTreeDepth = iTreeDepth >> 1;
    m_minDepthToBuild = m_params.m_groupLevels * (iTreeDepth / m_params.m_groupLevels + 2) - 1;

    // Starts zeroed (a departure): updateOffsetAndSpread writes m_propertiesOffset[0] and m_propertiesSpread[0] only
    // for a node with properties, the node infos of the whole tree are copied from this one, and checkAndAddProperties
    // reads both whatever the count; the exe reads them uninitialised.
    alignas(8) hkpMoppAssemblerNodeInfo rootInfo{};
    rootInfo.updateExtents(rootNode, m_codeInfo);
    rootInfo.m_currentBitsResolution = 16;
    rootInfo.m_accumOffset[0] = 0;
    rootInfo.m_accumOffset[1] = 0;
    rootInfo.m_accumOffset[2] = 0;
    rootInfo.m_terminalOffset = 0;
    rootInfo.m_level = -1;
    rootInfo.m_ignoreBandControlCode.m_bool = 0;
    rootInfo.updateOffsetAndSpread(rootNode);
    rootInfo.calcMaxExtentsSignificantBits();

    alignas(8) hkpMoppAssemblerCutInfo parentInfo(rootInfo, rootNode, m_codeInfo);
    preCalcScale(rootNode, rootInfo, parentInfo);
    parentInfo.updateExtents(rootNode, m_codeInfo);
    parentInfo.m_currentBitsResolution = 16;
    parentInfo.m_accumOffset[0] = 0;
    parentInfo.m_accumOffset[1] = 0;
    parentInfo.m_accumOffset[2] = 0;
    parentInfo.m_terminalOffset = 0;
    parentInfo.m_level = -1;
    parentInfo.m_ignoreBandControlCode.m_bool = 0;
    parentInfo.updateOffsetAndSpread(rootNode);
    parentInfo.calcMaxExtentsSignificantBits();

    alignas(8) hkpMoppAssemblerCutInfo cutInfo(parentInfo, rootNode, m_codeInfo);
    preCalcCutPositions(rootNode, parentInfo, cutInfo);

    alignas(8) hkpMoppAssemblerNodeInfo info(rootInfo, rootNode, m_codeInfo);
    if (!m_chunkInfo) {
        assemblesubNode(rootNode, rootInfo, info);
        return;
    }

    // Chunked: the root is chunk 0. The first pass picks the chunk points and numbers each chunk's terminals, the
    // second assembles the chunks; the root chunk's code then goes into the main code generator.
    rootNode->m_assemblerData.m_chunkId = 0;
    m_chunkInfo->m_chunks.expandOne();
    hkpMoppCodeGenerator* pCode = m_code;
    alignas(8) hkpMoppAssemblerNodeInfo chunkInfo(rootInfo, rootNode, m_codeInfo);
    alignas(8) hkArray<uint32_t, hkContainerHeapAllocator> terminals;
    // A scratch code generator of 1 MB (0x100000 bytes), destroyed and built again between the passes.
    alignas(8) hkpMoppCodeGenerator chunkCode(0x100000);
    m_code = &chunkCode;
    calcChunkPoints(rootNode, rootInfo, chunkInfo, terminals);
    calcTerminalIdsForChunks(rootNode, rootNode->m_assemblerData.m_chunkId, 0);
    terminals.clearAndDeallocate();
    chunkCode.~hkpMoppCodeGenerator();
    ::new (&chunkCode) hkpMoppCodeGenerator(0x100000);
    m_code = &chunkCode;
    assembleSubNodeIntoChunks(rootNode, rootInfo, info, rootNode->m_assemblerData.m_chunkId, terminals);
    if (m_chunkInfo->m_compressor) {
        // The data offset: past the 5-byte command, rounded up to 4 bytes.
        const int32_t iEnd = m_code->m_pos + 5;
        addDataOffset(iEnd + (-iEnd & 3), terminals.m_size);
    }
    hkpMoppCompilerChunkInfo* pChunkInfo = m_chunkInfo;
    m_code = pCode;
    copyCodeAndEmbedTerminals(pChunkInfo, &chunkCode, &terminals, pCode);
    hkpMoppCompilerChunkInfo::Chunk& chunk = m_chunkInfo->m_chunks.m_data[rootNode->m_assemblerData.m_chunkId];
    chunk.m_code = m_code;
    m_code->addReference();
    chunk.m_codeSize = m_code->m_pos;
}

void hkpMoppDefaultAssembler::getScaleInfo(hkpMoppTreeNode* node, hkpMoppCode::CodeInfo* codeInfo) {
    float v3 = 0.0f;
    codeInfo->m_offset.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(node->m_extents.m_extent[0u].m_min))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(node->m_extents.m_extent[2].m_min)))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(node->m_extents.m_extent[1].m_min))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    float v4 = node->m_extents.m_extent[0u].m_max - node->m_extents.m_extent[0u].m_min;
    if (!(v4 < 0.0)) {
        v3 = node->m_extents.m_extent[0u].m_max - node->m_extents.m_extent[0u].m_min;
    }
    float v5 = node->m_extents.m_extent[1].m_max - node->m_extents.m_extent[1].m_min;
    if (!(v5 < static_cast<double>(v3))) {
        v3 = node->m_extents.m_extent[1].m_max - node->m_extents.m_extent[1].m_min;
    }
    float v6 = node->m_extents.m_extent[2].m_max - node->m_extents.m_extent[2].m_min;
    if (!(v6 < static_cast<double>(v3))) {
        v3 = node->m_extents.m_extent[2].m_max - node->m_extents.m_extent[2].m_min;
    }
    XFNV_M128_F32(codeInfo->m_offset.m_quad, 3) = 254.0 / v3 * 65536.0;
}

// Hands a non-terminal node's two branches back to the node manager when nodes are built interleaved.
void hkpMoppDefaultAssembler::releaseNode(hkpMoppTreeNode* node) {
    if ((!node->m_isTerminal.m_bool) && m_params.m_interleavedBuildingEnabled.m_bool) {
        hkpMoppTreeInternalNode* internalNode = static_cast<hkpMoppTreeInternalNode*>(node);
        m_nodeMgr->releaseNode(internalNode->m_leftBranch);
        m_nodeMgr->releaseNode(internalNode->m_rightBranch);
        internalNode->m_leftBranch = nullptr;
        internalNode->m_rightBranch = nullptr;
    }
}

void hkpMoppDefaultAssembler::fixScale(hkpMoppTreeNode* currentNode) {
    for (hkpMoppTreeNode* i = currentNode; i; i = i->m_parent) {
        if (i->m_assemblerData.m_scaleIsValid.m_bool) {
            break;
        }
        i->m_assemblerData.m_scaleIsValid.m_bool = 1;
    }
}

void hkpMoppDefaultAssembler::addProperty(int32_t property, int32_t value) {
    hkpMoppCodeGenerator* m_code = this->m_code;
    if (value >= 0) {
        if (value < 256) {
            m_code->addCommand8(property + 96, value);
            return;
        }
        if (value < 0x10000) {
            m_code->addCommand16(property + 100, value);
            return;
        }
    }
    m_code->addCommand32(property + 104, value);
}

int32_t hkpMoppDefaultAssembler::addJumpChunk(int32_t chunkId) {
    m_code->addCommand32(112, chunkId);
    return 5;
}

int32_t hkpMoppDefaultAssembler::addDataOffset(int32_t offset, int32_t numTerminals) {
    hkpMoppCodeGenerator* m_code = this->m_code;
    m_code->m_code[m_code->m_size - m_code->m_pos++ - 1] = numTerminals;
    if (m_code->m_pos >= m_code->m_size) {
        m_code->resize();
    }
    hkpMoppCodeGenerator* v5 = this->m_code;
    v5->m_code[v5->m_size - v5->m_pos++ - 1] = static_cast<uint8_t>(numTerminals >> 8);
    if (v5->m_pos >= v5->m_size) {
        v5->resize();
    }
    this->m_code->addCommand16(13, offset);
    return 5;
}

void hkpMoppDefaultAssembler::addTermIdOffset(uint32_t offset) {
    hkpMoppCodeGenerator* m_code = this->m_code;
    if (offset >= 256) {
        if (offset >= 0x10000) {
            m_code->addCommand32(11, offset);
        }
        else {
            // addCommand16 takes a signed value.
            m_code->addCommand16(10, static_cast<int32_t>(offset));
        }
    }
    else {
        m_code->addCommand8(9, static_cast<int32_t>(offset));
    }
}

// The offset is compared with 0xFFFFFF.
void hkpMoppDefaultAssembler::addJump(uint32_t jumpPos) {
    hkpMoppCodeGenerator* m_code = this->m_code;
    int32_t v3 = m_code->m_pos - jumpPos;
    if (v3 > 0) {
        uint32_t v4 = m_code->m_pos - jumpPos;
        if (v3 >= 255) {
            if (v3 >= 0xFFFF) {
                if (v3 >= 0xFFFFFF) {
                    m_code->addCommand32(8, v4);
                }
                else {
                    m_code->addCommand24(7, v4);
                }
            }
            else {
                m_code->addCommand16(6, v4);
            }
        }
        else {
            m_code->addCommand8(5, v4);
        }
    }
}

void hkpMoppDefaultAssembler::updateParams(const hkpMoppAssembler::hkpMoppAssemblerParams& ap) {
    m_params = ap;
}

int32_t
hkpMoppDefaultAssembler::calcTerminalIdsForChunks(hkpMoppTreeNode* currentNode, int32_t chunkId, int32_t currentId) {
    int32_t id = currentId;
    int32_t nodeChunkId = currentNode->m_assemblerData.m_chunkId;
    currentNode->m_assemblerData.m_isAssembled.m_bool = 0;
    // A node that starts a chunk numbers its terminals from 0.
    if (nodeChunkId < 0) {
        nodeChunkId = chunkId;
    }
    else {
        id = 0;
    }
    if (m_chunkInfo->m_compressor) {
        currentNode->m_minPrimitiveId = id;
    }
    if (currentNode->m_isTerminal.m_bool) {
        if (m_chunkInfo->m_compressor) {
            hkpMoppTreeTerminal* pTerminal = static_cast<hkpMoppTreeTerminal*>(currentNode);
            pTerminal->m_primitive->m_origPrimitiveID = pTerminal->m_primitive->m_primitiveID;
            hkArray<hkpMoppCodeReindexedTerminal, hkContainerHeapAllocator>& reindexInfo = m_chunkInfo->m_reindexInfo;
            hkpMoppCodeReindexedTerminal& reindexed = reindexInfo.expandOne();
            reindexed.m_reindexedShapeKey = id | nodeChunkId << 8;
            reindexed.m_origShapeKey = pTerminal->m_primitive->m_primitiveID;
            pTerminal->m_primitive->m_primitiveID = id++;
        }
    }
    else {
        hkpMoppTreeInternalNode* pNode = static_cast<hkpMoppTreeInternalNode*>(currentNode);
        int32_t rightId = calcTerminalIdsForChunks(pNode->m_rightBranch, nodeChunkId, id);
        id = calcTerminalIdsForChunks(pNode->m_leftBranch, nodeChunkId, rightId);
    }
    if (m_chunkInfo->m_compressor) {
        currentNode->m_minPrimitiveId = 0;
        currentNode->m_maxPrimitiveId = id;
    }
    if (currentNode->m_assemblerData.m_chunkId >= 0) {
        return currentId;
    }
    return id;
}

int32_t hkpMoppDefaultAssembler::assembleJumpChunkCommand(hkpMoppTreeNode* currentNode) {
    int32_t m_pos = this->m_code->m_pos;
    addJumpChunk(currentNode->m_assemblerData.m_chunkId);
    int32_t v3 = this->m_code->m_pos;
    currentNode->m_assemblerData.m_assembledAddress = v3;
    currentNode->m_assemblerData.m_isAssembled.m_bool = 1;
    hkpMoppCodeGenerator::JumpCommandInfo& rJump = this->m_code->m_jumpCommands.expandOne();
    rJump.m_chunkId = currentNode->m_assemblerData.m_chunkId;
    rJump.m_position = v3;
    validateJumpCommands();
    return v3 - m_pos;
}

void hkpMoppDefaultAssembler::checkAndAddProperties(
    hkpMoppTreeNode* currentNode,
    const hkpMoppAssemblerNodeInfo& parentInfo,
    hkpMoppAssemblerNodeInfo& currentInfo
) {
    if (currentNode->m_parent) {
        if ((!currentInfo.m_propertiesSpread[0]) && parentInfo.m_propertiesSpread[0]) {
            addProperty(0, currentInfo.m_propertiesOffset[0]);
        }
    }
    else {
        if (!currentInfo.m_propertiesSpread[0]) {
            int32_t v4 = currentInfo.m_propertiesOffset[0];
            if (v4) {
                addProperty(0, v4);
            }
        }
    }
}

void hkpMoppDefaultAssembler::addSplit(
    int32_t lowPlane,
    int32_t highPlane,
    HK_MOPP_SPLIT_DIRECTIONS splitCode,
    int32_t lpos,
    int32_t rpos
) {
    int32_t rightPos = rpos;
    int32_t v10;
    uint8_t v13;
    hkpMoppCodeGenerator* m_code = this->m_code;
    int32_t v9 = m_code->m_pos - rpos;
    if (splitCode >= 3) {
        goto add_jumps_and_split;
    }
    v10 = m_code->m_pos - lpos;
    if ((v10 > 0 || v9 > 248) && (v10 < 0x10000 && v9 < 0x10000)) {
        m_code->addCode(v9);
        this->m_code->addCode(static_cast<uint8_t>(v9 >> 8));
        this->m_code->addCode(v10);
        this->m_code->addCode(static_cast<uint8_t>(v10 >> 8));
        this->m_code->addCode(lowPlane);
        this->m_code->addCode(highPlane);
        this->m_code->addCode(splitCode + 35);
        return;
    }
    if (highPlane - lowPlane == 1) {
        if (v9 > 251) {
            addJump(rpos);
            rightPos = this->m_code->m_pos;
        }
        addJump(lpos);
        hkpMoppCodeGenerator* v11 = this->m_code;
        v11->m_code[v11->m_size - v11->m_pos++ - 1] = static_cast<uint8_t>(v11->m_pos) - rightPos;
        if (v11->m_pos >= v11->m_size) {
            v11->resize();
        }
        hkpMoppCodeGenerator* v12 = this->m_code;
        v12->m_code[v12->m_size - v12->m_pos++ - 1] = lowPlane;
        if (v12->m_pos >= v12->m_size) {
            v12->resize();
        }
        v13 = splitCode + 32;
    }
    else {
    add_jumps_and_split:;
        if (v9 > 251) {
            addJump(rpos);
            rightPos = this->m_code->m_pos;
        }
        addJump(lpos);
        hkpMoppCodeGenerator* v14 = this->m_code;
        v14->m_code[v14->m_size - v14->m_pos++ - 1] = static_cast<uint8_t>(v14->m_pos) - rightPos;
        if (v14->m_pos >= v14->m_size) {
            v14->resize();
        }
        hkpMoppCodeGenerator* v15 = this->m_code;
        v15->m_code[v15->m_size - v15->m_pos++ - 1] = lowPlane;
        if (v15->m_pos >= v15->m_size) {
            v15->resize();
        }
        hkpMoppCodeGenerator* v16 = this->m_code;
        v16->m_code[v16->m_size - v16->m_pos++ - 1] = highPlane;
        if (v16->m_pos >= v16->m_size) {
            v16->resize();
        }
        v13 = splitCode + 16;
    }
    hkpMoppCodeGenerator* v17 = this->m_code;
    v17->m_code[v17->m_size - v17->m_pos++ - 1] = v13;
    if (v17->m_pos >= v17->m_size) {
        v17->resize();
    }
}

// Assembles an internal node's split: the split command with its two planes and the jumps to the children, the
// terminal id offset, the properties, the cuts and the rescale; returns the size of the code it added.
int32_t hkpMoppDefaultAssembler::assembleNonTerminalCommand(
    hkpMoppTreeNode* currentNode,
    const hkpMoppAssemblerNodeInfo& parentInfo,
    hkpMoppAssemblerNodeInfo& currentInfo,
    hkpMoppAssemblerRescaleCommand& rescaleCommandInfo
) {
    hkpMoppTreeInternalNode* pNode = static_cast<hkpMoppTreeInternalNode*>(currentNode);
    fixScale(currentNode);
    int32_t iStartPos = m_code->m_pos;
    HK_MOPP_SPLIT_DIRECTIONS eSplitCode = static_cast<HK_MOPP_SPLIT_DIRECTIONS>(
        getSplitCode(&pNode->m_plane->m_direction(0))
    );
    int32_t iHighPlane;
    int32_t iLowPlane;
    findPlanes(pNode, currentInfo, iHighPlane, iLowPlane);
    addSplit(
        iLowPlane,
        iHighPlane,
        eSplitCode,
        pNode->m_leftBranch->m_assemblerData.m_assembledAddress,
        pNode->m_rightBranch->m_assemblerData.m_assembledAddress
    );
    uint32_t offset = currentInfo.m_terminalOffset - parentInfo.m_terminalOffset;
    if (offset) {
        addTermIdOffset(offset);
    }
    checkAndAddProperties(pNode, parentInfo, currentInfo);
    addCut(pNode, currentInfo);
    if (rescaleCommandInfo.m_doRescale.m_bool) {
        addRescale(rescaleCommandInfo);
    }
    pNode->m_assemblerData.m_assembledAddress = m_code->m_pos;
    pNode->m_assemblerData.m_isAssembled.m_bool = 1;
    return m_code->m_pos - iStartPos;
}

void hkpMoppDefaultAssembler::addRescale(const hkpMoppAssemblerRescaleCommand& cmd) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&cmd);
    hkpMoppCodeGenerator* m_code = this->m_code;
    m_code->m_code[m_code->m_size - m_code->m_pos++ - 1] = bytes[16];
    if (m_code->m_pos >= m_code->m_size) {
        m_code->resize();
    }
    hkpMoppCodeGenerator* v5 = this->m_code;
    v5->m_code[v5->m_size - v5->m_pos++ - 1] = bytes[12];
    if (v5->m_pos >= v5->m_size) {
        v5->resize();
    }
    hkpMoppCodeGenerator* v6 = this->m_code;
    v6->m_code[v6->m_size - v6->m_pos++ - 1] = bytes[8];
    if (v6->m_pos >= v6->m_size) {
        v6->resize();
    }
    hkpMoppCodeGenerator* v7 = this->m_code;
    v7->m_code[v7->m_size - v7->m_pos++ - 1] = bytes[4];
    if (v7->m_pos >= v7->m_size) {
        v7->resize();
    }
}

// The significant bits a node's scale needs. A terminal: the bits its triangle tolerance takes at the code's
// scale, at least the info's; along each axis where the node is thinner than the axis-aligned tolerance it gets a
// cut, the wide one where that tolerance fits the resolution. An inner node: rescaled where its parent's scale is
// too coarse, then its children's bits computed with infos of their own, and each child marked for a rescale of its
// own where its bits fall below what this node keeps.
int32_t hkpMoppDefaultAssembler::preCalcScale(
    hkpMoppTreeNode* currentNode,
    const hkpMoppAssemblerNodeInfo& parentInfo,
    hkpMoppAssemblerNodeInfo& currentInfo
) {
    int32_t significantBits = currentInfo.m_maxExtentsSignificantBits;
    if (currentNode->m_isTerminal.m_bool) {
        float tolerance = m_params.m_absoluteFitToleranceOfTriangles;
        // 1 is shifted by the low five bits of significantBits.
        int32_t resolution = static_cast<int32_t>(1u << (significantBits & 31));
        for (int32_t axis = 0; axis < 3; axis++) {
            if (!currentNode->m_assemblerData.m_scaleIsValid.m_bool) {
                currentNode->m_assemblerData.m_cutCommandType[axis] = hkpMoppAssemblerData::HK_MOPP_CUT8;
            }
            float axisTolerance = m_params.m_absoluteFitToleranceOfAxisAlignedTriangles(axis);
            float extent = currentNode->m_extents.m_extent[axis].m_max - currentNode->m_extents.m_extent[axis].m_min;
            if (axisTolerance > extent && tolerance > axisTolerance) {
                float scale = XFNV_M128_F32(m_codeInfo.m_offset.m_quad, 3);
                float scaledTolerance = axisTolerance * scale;
                if (hkMath::hkFloatToInt(scaledTolerance) << 7 < resolution) {
                    if (!currentNode->m_assemblerData.m_scaleIsValid.m_bool) {
                        currentNode->m_assemblerData.m_cutCommandType[axis] = hkpMoppAssemblerData::HK_MOPP_CUT24;
                    }
                }
                else {
                    tolerance = axisTolerance;
                }
            }
        }
        float scaledTolerance = XFNV_M128_F32(m_codeInfo.m_offset.m_quad, 3) * 0.8999999761581421 * tolerance;
        uint32_t steps = static_cast<uint32_t>(hkMath::hkFloatToInt(scaledTolerance));
        int32_t numBits = 0;
        while (steps) {
            steps >>= 1;
            ++numBits;
        }
        int32_t neededBits = numBits + 6;
        if (neededBits > significantBits) {
            return neededBits;
        }
        return significantBits;
    }

    if (!currentNode->m_assemblerData.m_scaleIsValid.m_bool &&
        currentInfo.m_level < m_halfTreeDepth &&
        calcRescaleBits(parentInfo, currentInfo) > 2) {
        currentNode->m_assemblerData.m_rescaleCommandType = hkpMoppAssemblerData::HK_MOPP_RESCALE;
    }
    if (currentNode->m_assemblerData.m_rescaleCommandType.m_storage == hkpMoppAssemblerData::HK_MOPP_RESCALE) {
        hkpMoppAssemblerRescaleCommand rescaleCommand;
        recalcRescale(parentInfo, currentInfo, rescaleCommand);
    }
    hkpMoppTreeInternalNode* node = static_cast<hkpMoppTreeInternalNode*>(currentNode);
    hkpMoppTreeNode* children[2] = {node->m_leftBranch, node->m_rightBranch};
    int32_t childBits[2] = {-1, -1};
    int32_t minChildSignificantBits = std::numeric_limits<int32_t>::max();
    // The right child first.
    for (int32_t i = 1; i >= 0; --i) {
        hkpMoppTreeNode* child = children[i];
        if (child) {
            hkpMoppAssemblerNodeInfo childInfo = currentInfo;
            childInfo.updateOffsetAndSpread(child);
            childInfo.updateExtents(child, m_codeInfo);
            childInfo.m_level = currentInfo.m_level + 1;
            childInfo.m_ignoreBandControlCode.m_bool = 0;
            childInfo.calcMaxExtentsSignificantBits();
            childBits[i] = preCalcScale(child, currentInfo, childInfo);
            if (minChildSignificantBits >= childInfo.m_maxExtentsSignificantBits) {
                minChildSignificantBits = childInfo.m_maxExtentsSignificantBits;
            }
        }
    }
    // The add deliberately wraps where no child set the minimum (0x7FFFFFFF + 2).
    int32_t result = static_cast<int32_t>(static_cast<uint32_t>(minChildSignificantBits) + 2u);
    if (significantBits + 2 < result) {
        result = significantBits + 2;
    }
    // The left child first.
    for (int32_t i = 0; i < 2; ++i) {
        hkpMoppTreeNode* child = children[i];
        if (!child) {
            continue;
        }
        if (!currentNode->m_assemblerData.m_scaleIsValid.m_bool) {
            if (childBits[i] < significantBits) {
                child->m_assemblerData.m_rescaleCommandType = hkpMoppAssemblerData::HK_MOPP_RESCALE;
            }
        }
        else if ((!child->m_assemblerData.m_scaleIsValid.m_bool) && (childBits[i] < result)) {
            child->m_assemblerData.m_rescaleCommandType = hkpMoppAssemblerData::HK_MOPP_RESCALE;
        }

        if (child->m_assemblerData.m_rescaleCommandType.m_storage == hkpMoppAssemblerData::HK_MOPP_NO_RESCALE &&
            result >= childBits[i]) {
            result = childBits[i];
        }
    }
    return result;
}

// Writes the node's cuts: per axis an 8-bit double cut (the extents relative to the accumulated offset at the
// current resolution, as bytes) or a 24-bit one (the extents as 24-bit parameters), as its cut command type says.
void hkpMoppDefaultAssembler::addCut(const hkpMoppTreeNode* currentNode, const hkpMoppAssemblerNodeInfo& currentInfo) {
    const hkpMoppAssemblerNodeInfo* v3 = &currentInfo;
    int32_t n3 = 0;
    int32_t n3_1 = 0;
    const int32_t* v6 = &currentInfo.m_extents[0][1];
    const int32_t* m_accumOffset = currentInfo.m_accumOffset;
    while (true) {
        if (currentNode->m_assemblerData.m_cutCommandType[n3].m_storage == hkpMoppAssemblerData::HK_MOPP_CUT8) {
            int32_t m_currentBitsResolution = v3->m_currentBitsResolution;
            hkpMoppCodeGenerator* m_code = this->m_code;
            int32_t v9 = (*(v6 - 1) - *m_accumOffset) >> m_currentBitsResolution;
            m_code->m_code[m_code->m_size - m_code->m_pos++ - 1] = ((*v6 - *m_accumOffset) >> m_currentBitsResolution) +
                1;
            if (m_code->m_pos >= m_code->m_size) {
                m_code->resize();
            }
            hkpMoppCodeGenerator* m_code_1 = this->m_code;
            m_code_1->m_code[m_code_1->m_size - m_code_1->m_pos++ - 1] = v9;
            if (m_code_1->m_pos >= m_code_1->m_size) {
                m_code_1->resize();
            }
            hkpMoppCodeGenerator* m_code_2 = this->m_code;
            m_code_2->m_code[m_code_2->m_size - m_code_2->m_pos++ - 1] = n3_1 + HK_MOPP_DOUBLE_CUT_X;
            if (m_code_2->m_pos >= m_code_2->m_size) {
                m_code_2->resize();
            }
            n3 = n3_1;
        }
        if (currentNode->m_assemblerData.m_cutCommandType[n3].m_storage == hkpMoppAssemblerData::HK_MOPP_CUT24) {
            int32_t param = *(v6 - 1);
            this->m_code->addParam24(*v6 + 1);
            this->m_code->addParam24(param);
            hkpMoppCodeGenerator* m_code_3 = this->m_code;
            m_code_3->m_code[m_code_3->m_size - m_code_3->m_pos++ - 1] = n3 + HK_MOPP_DOUBLE_CUT24_X;
            if (m_code_3->m_pos >= m_code_3->m_size) {
                m_code_3->resize();
            }
        }
        ++m_accumOffset;
        ++n3;
        v6 += 2;
        n3_1 = n3;
        if (n3 >= 3) {
            break;
        }
        v3 = &currentInfo;
    }
}

hkBool
hkpMoppDefaultAssembler::shouldAssemble(hkpMoppTreeNode* currentNode, const hkpMoppAssemblerNodeInfo& currentInfo) {
    if (currentNode->m_numPrimitives < 22 || currentInfo.m_level > m_minDepthToBuild) {
        return 1;
    }
    return 0;
}

// Writes the terminal command for the node's primitive id relative to the terminal offset (the short form below 32,
// else the 8-, 16-, 24- or 32-bit form), then its non-zero properties; returns the bytes written.
int32_t hkpMoppDefaultAssembler::addTerminals(
    hkpMoppTreeTerminal* terminal,
    const hkpMoppAssemblerNodeInfo& parentInfo,
    const hkpMoppAssemblerNodeInfo& currentInfo
) {
    hkpMoppCodeGenerator* code = m_code;
    int32_t startPos = code->m_pos;
    uint32_t offset = terminal->m_primitive->m_primitiveID - currentInfo.m_terminalOffset;
    if (offset < 32) {
        code->addCode(static_cast<int32_t>(offset + HK_MOPP_TERM4_0));
    }
    else if (offset < 0x100) {
        code->addCommand8(HK_MOPP_TERM8, static_cast<int32_t>(offset));
    }
    else if (offset < 0x10000) {
        code->addCommand16(HK_MOPP_TERM16, static_cast<int32_t>(offset));
    }
    else if (offset < 0x1000000) {
        code->addCommand24(HK_MOPP_TERM24, static_cast<int32_t>(offset));
    }
    else {
        code->addCommand32(HK_MOPP_TERM32, offset);
    }
    for (int32_t property = 0; property < terminal->m_numProperties; ++property) {
        int32_t value = static_cast<int32_t>(terminal->m_minPropertyValue[property]);
        if (value && parentInfo.m_propertiesSpread[property]) {
            addProperty(property, value);
        }
    }
    return m_code->m_pos - startPos;
}

// The cut planes of a terminal node along each axis: its extents in the parent's integer space widened by the
// larger of the resolution's step and the triangle tolerance (the axis-aligned tolerance where the node is thinner
// than it along that axis).
void hkpMoppDefaultAssembler::calcTermCut(hkpMoppTreeNode* currentNode, hkpMoppAssemblerCutInfo& currentInfo) {
    for (int32_t axis = 0; axis < 3; axis++) {
        float extent = currentNode->m_extents.m_extent[axis].m_max - currentNode->m_extents.m_extent[axis].m_min;
        float tolerance = static_cast<double>(1 << currentInfo.m_currentBitsResolution) /
            XFNV_M128_F32(m_codeInfo.m_offset.m_quad, 3);
        float triangleTolerance = m_params.m_absoluteFitToleranceOfTriangles;
        if (!(triangleTolerance < tolerance)) {
            tolerance = triangleTolerance;
        }
        float axisTolerance = m_params.m_absoluteFitToleranceOfAxisAlignedTriangles(axis);
        if (axisTolerance > extent && !(tolerance < axisTolerance)) {
            tolerance = axisTolerance;
        }
        float scale = XFNV_M128_F32(m_codeInfo.m_offset.m_quad, 3);
        float scaledTolerance = scale * tolerance;
        int32_t delta = hkMath::hkFloatToInt(scaledTolerance);
        currentNode->m_assemblerData.m_minCutPlanePosition[axis] = currentInfo.m_extents[axis][0] - delta;
        currentNode->m_assemblerData.m_maxCutPlanePosition[axis] = currentInfo.m_extents[axis][1] + delta + 1;
    }
}

void hkpMoppDefaultAssembler::initTopDown(
    hkpMoppTreeNode* currentNode,
    const hkpMoppAssemblerNodeInfo& parentInfo,
    hkpMoppAssemblerNodeInfo& currentInfo,
    hkpMoppAssemblerRescaleCommand& rescaleCommandInfo
) {
    if (currentNode->m_assemblerData.m_rescaleCommandType.m_storage == 1) {
        recalcRescale(parentInfo, currentInfo, rescaleCommandInfo);
    }
    calcReoffset(currentNode, parentInfo, currentInfo);
}

int32_t hkpMoppDefaultAssembler::assembleCutAndTerminalCommand(
    hkpMoppTreeNode* currentNode,
    const hkpMoppAssemblerNodeInfo& parentInfo,
    hkpMoppAssemblerNodeInfo& currentInfo
) {
    fixScale(currentNode);
    hkpMoppCodeGenerator* m_code = this->m_code;
    ++m_numNodesAssembled;
    int32_t m_pos = m_code->m_pos;
    addTerminals(static_cast<hkpMoppTreeTerminal*>(currentNode), parentInfo, currentInfo);
    addCut(currentNode, currentInfo);
    currentNode->m_assemblerData.m_assembledAddress = this->m_code->m_pos;
    int32_t v7 = this->m_code->m_pos;
    currentNode->m_assemblerData.m_isAssembled.m_bool = 1;
    return v7 - m_pos;
}

void hkpMoppDefaultAssembler::preCalcCutPositions(
    hkpMoppTreeNode* currentNode,
    const hkpMoppAssemblerCutInfo& parentInfo,
    hkpMoppAssemblerCutInfo& currentInfo
) {
    hkpMoppAssemblerData& data = currentNode->m_assemblerData;
    if (data.m_isAssembled.m_bool) {
        return;
    }
    if (currentNode->m_isTerminal.m_bool) {
        currentInfo.calcPossibleCutPlanes();
        calcTermCut(currentNode, currentInfo);
        return;
    }
    if (data.m_rescaleCommandType.m_storage == hkpMoppAssemblerData::HK_MOPP_RESCALE) {
        hkpMoppAssemblerRescaleCommand cmd;
        recalcRescale(parentInfo, currentInfo, cmd);
    }
    currentInfo.calcPossibleCutPlanes();
    hkpMoppTreeInternalNode* pNode = static_cast<hkpMoppTreeInternalNode*>(currentNode);
    int32_t splitCode = getSplitCode(&pNode->m_plane->m_direction(0));
    int32_t highPlane;
    int32_t lowPlane;
    findPlanes(pNode, currentInfo, highPlane, lowPlane);
    hkpMoppTreeNode* pRight = pNode->m_rightBranch;
    hkpMoppTreeNode* children[2] = {pRight, pNode->m_leftBranch};
    for (int32_t axis = 0; axis < 3; axis++) {
        hkMath::hkFloatToInt(m_params.m_absoluteFitToleranceOfInternalNodes * m_codeInfo.m_offset(3));
        data.m_minCutPlanePosition[axis] = 0;
        data.m_maxCutPlanePosition[axis] = INT32_MAX;
    }
    for (int32_t i = 0; i < 2; i++) {
        hkpMoppTreeNode* pChild = children[i];
        if (!pChild) {
            continue;
        }
        hkpMoppAssemblerData& childData = pChild->m_assemblerData;
        hkpMoppAssemblerCutInfo childInfo(currentInfo, pChild, m_codeInfo);
        preCalcCutPositions(pChild, currentInfo, childInfo);
        // The child keeps no cut on the splitting plane's side it shares with the other child.
        if (pChild == pRight) {
            if (splitCode <= HK_MOPP_SD_Z) {
                childData.m_minCutPlanePosition[splitCode] = 0;
            }
        }
        else if (splitCode <= HK_MOPP_SD_Z) {
            childData.m_maxCutPlanePosition[splitCode] = INT32_MAX;
        }
        for (int32_t axis = 0; axis < 3; axis++) {
            // A child's cut that this node's cut planes or extents already cover moves up to this node.
            int8_t childCut = childData.m_cutCommandType[axis].m_storage;
            if (childCut == hkpMoppAssemblerData::HK_MOPP_CUT8) {
                if (childData.m_minCutPlanePosition[axis] <= currentInfo.m_lowCutPlanePosition[axis] &&
                    childData.m_maxCutPlanePosition[axis] >= currentInfo.m_highCutPlanePosition[axis]) {
                    int8_t cut = hkpMoppAssemblerData::HK_MOPP_CUT8;
                    if (data.m_cutCommandType[axis].m_storage >= hkpMoppAssemblerData::HK_MOPP_CUT8) {
                        cut = data.m_cutCommandType[axis].m_storage;
                    }
                    data.m_cutCommandType[axis] = static_cast<hkpMoppAssemblerData::hkpCutCommandType>(cut);
                    childData.m_cutCommandType[axis].m_storage = hkpMoppAssemblerData::HK_MOPP_NO_CUT;
                }
            }
            else if (
                (childCut == hkpMoppAssemblerData::HK_MOPP_CUT24) &&
                (childData.m_minCutPlanePosition[axis] <= currentInfo.m_extents[axis][0] &&
                 childData.m_maxCutPlanePosition[axis] >= currentInfo.m_extents[axis][1])
            ) {
                data.m_cutCommandType[axis] = hkpMoppAssemblerData::HK_MOPP_CUT24;
                childData.m_cutCommandType[axis].m_storage = hkpMoppAssemblerData::HK_MOPP_NO_CUT;
            }

            // Without a cut of its own, the child's cut positions narrow this node's.
            if (childData.m_cutCommandType[axis].m_storage == hkpMoppAssemblerData::HK_MOPP_NO_CUT) {
                int32_t minPos = childData.m_minCutPlanePosition[axis];
                if (data.m_minCutPlanePosition[axis] > minPos) {
                    minPos = data.m_minCutPlanePosition[axis];
                }
                int32_t parentMax = data.m_maxCutPlanePosition[axis];
                data.m_minCutPlanePosition[axis] = minPos;
                int32_t maxPos = childData.m_maxCutPlanePosition[axis];
                if (parentMax < maxPos) {
                    maxPos = parentMax;
                }
                data.m_maxCutPlanePosition[axis] = maxPos;
            }
        }
    }
}

int32_t hkpMoppDefaultAssembler::assemblesubNode(
    hkpMoppTreeNode* currentNode,
    const hkpMoppAssemblerNodeInfo& parentInfo,
    hkpMoppAssemblerNodeInfo& currentInfo
) {
    if (currentNode->m_assemblerData.m_isAssembled.m_bool) {
        return 3;
    }
    if (currentInfo.m_level <= m_minDepthToBuild &&
        currentInfo.m_level % m_params.m_groupLevels == 0 &&
        !currentInfo.m_ignoreBandControlCode.m_bool) {
        // A band's top node: assemble it again, one band shallower each time a pass adds no code, until it is
        // assembled or enough nodes are.
        currentInfo.m_ignoreBandControlCode.m_bool = 1;
        int32_t minDepthToBuild = m_minDepthToBuild;
        int32_t result = -1;
        while (!currentNode->m_assemblerData.m_isAssembled.m_bool) {
            int32_t pos = m_code->m_pos;
            alignas(8) hkpMoppAssemblerNodeInfo info = currentInfo;
            result = assemblesubNode(currentNode, parentInfo, info);
            if (m_numNodesAssembled > m_minAssembledElems) {
                break;
            }
            if (pos == m_code->m_pos) {
                if (m_minDepthToBuild < 0) {
                    break;
                }
                m_minDepthToBuild -= m_params.m_groupLevels;
            }
        }
        m_minDepthToBuild = minDepthToBuild;
        currentInfo.m_ignoreBandControlCode.m_bool = 0;
        return result;
    }
    if (currentNode->m_isTerminal.m_bool) {
        fixScale(currentNode);
        if (shouldAssemble(currentNode, currentInfo).m_bool) {
            return assembleCutAndTerminalCommand(currentNode, parentInfo, currentInfo);
        }
        return -1;
    }
    hkpMoppAssemblerRescaleCommand rescaleCommandInfo;
    rescaleCommandInfo.m_doRescale.m_bool = 0;
    rescaleCommandInfo.m_scaleCommand = -1;
    initTopDown(currentNode, parentInfo, currentInfo, rescaleCommandInfo);
    hkpMoppTreeInternalNode* pNode = static_cast<hkpMoppTreeInternalNode*>(currentNode);
    int32_t rightSize = -1;
    hkpMoppTreeNode* pChild = pNode->m_rightBranch;
    if (pChild) {
        alignas(8) hkpMoppAssemblerNodeInfo childInfo(currentInfo, pChild, m_codeInfo);
        rightSize = assemblesubNode(pChild, currentInfo, childInfo);
        if (rightSize >= 0) {
            releaseNode(pChild);
        }
    }
    pChild = pNode->m_leftBranch;
    if (!pChild) {
        return -1;
    }
    alignas(8) hkpMoppAssemblerNodeInfo childInfo(currentInfo, pChild, m_codeInfo);
    int32_t leftSize = assemblesubNode(pChild, currentInfo, childInfo);
    if (leftSize < 0) {
        return -1;
    }
    releaseNode(pChild);
    if (rightSize < 0) {
        return -1;
    }
    if (!shouldAssemble(currentNode, currentInfo).m_bool) {
        return -1;
    }
    ++m_numNodesAssembled;
    return assembleNonTerminalCommand(currentNode, parentInfo, currentInfo, rescaleCommandInfo) + leftSize + rightSize;
}

// The terminals of the subtree go into apTerminals; returns the bytes its code and primitives take. An inner node
// sizes both branches, then its own command; while the subtree does not fit in a chunk (m_maxChunkSize less
// m_safetySize), the larger branch still in place becomes a chunk of its own, replaced by a 5-byte jump.
int32_t hkpMoppDefaultAssembler::calcChunkPoints(
    hkpMoppTreeNode* currentNode,
    const hkpMoppAssemblerNodeInfo& parentInfo,
    hkpMoppAssemblerNodeInfo& currentInfo,
    hkArray<uint32_t, hkContainerHeapAllocator>& terminalsOut
) {
    if (m_chunkInfo->m_compressor) {
        currentInfo.m_terminalOffset = parentInfo.m_terminalOffset;
    }
    if (currentNode->m_isTerminal.m_bool) {
        hkpMoppTreeTerminal* pTerminal = static_cast<hkpMoppTreeTerminal*>(currentNode);
        if (m_chunkInfo->m_compressor) {
            currentInfo.m_terminalOffset = pTerminal->m_primitive->m_primitiveID;
        }
        const int32_t iStart = m_code->m_pos;
        assembleCutAndTerminalCommand(currentNode, parentInfo, currentInfo);
        const int32_t iSize = m_code->m_pos - iStart;
        terminalsOut.pushBack(pTerminal->m_primitive->m_primitiveID);
        uint32_t uiPrimitiveId = pTerminal->m_primitive->m_primitiveID;
        if (m_chunkInfo->m_compressor) {
            return iSize + m_chunkInfo->m_compressor->calcPrimitivesSize(&uiPrimitiveId, 1);
        }
        return iSize;
    }

    hkpMoppTreeInternalNode* pNode = static_cast<hkpMoppTreeInternalNode*>(currentNode);
    hkpMoppAssemblerRescaleCommand rescaleCommandInfo;
    rescaleCommandInfo.m_doRescale.m_bool = 0;
    rescaleCommandInfo.m_scaleCommand = -1;
    initTopDown(currentNode, parentInfo, currentInfo, rescaleCommandInfo);
    currentInfo.m_terminalOffset = parentInfo.m_terminalOffset;

    hkArray<uint32_t, hkContainerHeapAllocator> rightTerminals;
    hkpMoppTreeNode* pRight = pNode->m_rightBranch;
    hkpMoppAssemblerNodeInfo childInfo = currentInfo;
    childInfo.updateOffsetAndSpread(pRight);
    childInfo.updateExtents(pRight, m_codeInfo);
    childInfo.m_level = currentInfo.m_level + 1;
    childInfo.m_ignoreBandControlCode.m_bool = 0;
    childInfo.calcMaxExtentsSignificantBits();
    int32_t iStart = m_code->m_pos;
    calcChunkPoints(pRight, currentInfo, childInfo, rightTerminals);
    const int32_t iRightSize = m_code->m_pos - iStart;
    int32_t iRightTotal = iRightSize;
    if (m_chunkInfo->m_compressor && rightTerminals.m_size) {
        iRightTotal = m_chunkInfo->m_compressor->calcPrimitivesSize(rightTerminals.m_data, rightTerminals.m_size) +
            iRightSize;
    }

    hkArray<uint32_t, hkContainerHeapAllocator> leftTerminals;
    hkpMoppTreeNode* pLeft = pNode->m_leftBranch;
    childInfo = hkpMoppAssemblerNodeInfo(currentInfo, pLeft, m_codeInfo);
    iStart = m_code->m_pos;
    calcChunkPoints(pLeft, currentInfo, childInfo, leftTerminals);
    const int32_t iLeftSize = m_code->m_pos - iStart;
    int32_t iLeftTotal = iLeftSize;
    if (m_chunkInfo->m_compressor && leftTerminals.m_size) {
        iLeftTotal = m_chunkInfo->m_compressor->calcPrimitivesSize(leftTerminals.m_data, leftTerminals.m_size) +
            iLeftSize;
    }

    iStart = m_code->m_pos;
    assembleNonTerminalCommand(currentNode, parentInfo, currentInfo, rescaleCommandInfo);
    const int32_t iNodeSize = m_code->m_pos - iStart;
    hkArray<uint32_t, hkContainerHeapAllocator> allTerminals;
    allTerminals._insertAt(hkMemoryRouter::getInstance().heap(), 0, rightTerminals.m_data, rightTerminals.m_size);
    allTerminals._insertAt(
        hkMemoryRouter::getInstance().heap(),
        allTerminals.m_size,
        leftTerminals.m_data,
        leftTerminals.m_size
    );
    int32_t iTotal = iLeftSize + iNodeSize + iRightSize;
    if (m_chunkInfo->m_compressor && allTerminals.m_size) {
        iTotal += m_chunkInfo->m_compressor->calcPrimitivesSize(allTerminals.m_data, allTerminals.m_size);
    }

    bool bLeftChunked = false;
    bool bRightChunked = false;
    while (iTotal >= m_chunkInfo->m_maxChunkSize - m_chunkInfo->m_safetySize) {
        hkArray<hkpMoppCompilerChunkInfo::Chunk, hkContainerHeapAllocator>& chunks = m_chunkInfo->m_chunks;
        const int32_t iChunkId = chunks.m_size;
        hkpMoppCompilerChunkInfo::Chunk& chunk = chunks.expandOne();
        hkpMoppTreeNode* pChunkRoot;
        int32_t iChunkTotal;
        int32_t iChunkSize;
        if (iLeftTotal > iRightTotal) {
            iChunkTotal = iLeftTotal;
            pChunkRoot = pNode->m_leftBranch;
            iChunkSize = iLeftSize;
            iLeftTotal = 5;
            bLeftChunked = true;
            iTotal = bRightChunked ? iNodeSize + 10 : iNodeSize + iRightTotal + 5;
        }
        else {
            pChunkRoot = pNode->m_rightBranch;
            iChunkSize = iRightSize;
            iChunkTotal = iRightTotal;
            iRightTotal = 5;
            bRightChunked = true;
            iTotal = bLeftChunked ? iNodeSize + 10 : iNodeSize + iLeftTotal + 5;
        }
        chunk.m_codeSize = m_chunkInfo->m_safetySize + iChunkTotal + 5;
        pChunkRoot->m_assemblerData.m_chunkId = iChunkId;
        m_code->m_pos += 5 - iChunkSize;
        if (bRightChunked) {
            pNode->m_rightBranch->m_assemblerData.m_assembledAddress += 5 - iChunkSize;
        }
        pNode->m_leftBranch->m_assemblerData.m_assembledAddress += 5 - iChunkSize;
        currentNode->m_assemblerData.m_assembledAddress += 5 - iChunkSize;
    }

    // The terminals of the branches not made chunks go on to the parent.
    terminalsOut.m_size = 0;
    if (!bRightChunked) {
        terminalsOut._insertAt(hkMemoryRouter::getInstance().heap(), 0, rightTerminals.m_data, rightTerminals.m_size);
    }
    if (!bLeftChunked) {
        terminalsOut._insertAt(
            hkMemoryRouter::getInstance().heap(),
            terminalsOut.m_size,
            leftTerminals.m_data,
            leftTerminals.m_size
        );
    }
    allTerminals.clearAndDeallocate();
    leftTerminals.clearAndDeallocate();
    rightTerminals.clearAndDeallocate();
    return iTotal;
}

void hkpMoppDefaultAssembler::assembleSubNodeIntoChunks(
    hkpMoppTreeNode* currentNode,
    const hkpMoppAssemblerNodeInfo& parentInfo,
    hkpMoppAssemblerNodeInfo& currentInfo,
    int32_t chunkId,
    hkArray<uint32_t, hkContainerHeapAllocator>& terminalsOut
) {
    const int32_t iChunkId = currentNode->m_assemblerData.m_chunkId;
    if (iChunkId > 0 && !currentInfo.m_ignoreBandControlCode.m_bool) {
        // The node starts a chunk: its subtree is assembled into a scratch generator of the chunk's size, copied
        // into a new generator the chunk keeps, and this code jumps to the chunk.
        hkpMoppCodeGenerator* pCode = m_code;
        hkpMoppCompilerChunkInfo::Chunk& chunk = m_chunkInfo->m_chunks.m_data[iChunkId];
        hkpMoppCodeGenerator chunkCode(chunk.m_codeSize);
        m_code = &chunkCode;
        hkArray<uint32_t, hkContainerHeapAllocator> chunkTerminals;
        currentInfo.m_ignoreBandControlCode.m_bool = 1;
        assembleSubNodeIntoChunks(currentNode, parentInfo, currentInfo, iChunkId, chunkTerminals);
        if (m_chunkInfo->m_compressor) {
            // The data offset: past the 5-byte command, rounded up to 4 bytes.
            const int32_t iEnd = m_code->m_pos + 5;
            addDataOffset(iEnd + (-iEnd & 3), chunkTerminals.m_size);
        }
        hkpMoppCodeGenerator* pChunkCode = new hkpMoppCodeGenerator(chunk.m_codeSize);
        chunk.m_code = pChunkCode;
        copyCodeAndEmbedTerminals(m_chunkInfo, m_code, &chunkTerminals, pChunkCode);
        m_code->m_jumpCommands.clear();
        chunkTerminals.clearAndDeallocate();
        m_code = pCode;
        assembleJumpChunkCommand(currentNode);
        return;
    }
    if (currentNode->m_isTerminal.m_bool) {
        assembleCutAndTerminalCommand(currentNode, parentInfo, currentInfo);
        terminalsOut.pushBack(static_cast<hkpMoppTreeTerminal*>(currentNode)->m_primitive->m_origPrimitiveID);
        return;
    }
    hkpMoppAssemblerRescaleCommand rescaleCommandInfo;
    rescaleCommandInfo.m_doRescale.m_bool = 0;
    rescaleCommandInfo.m_scaleCommand = -1;
    initTopDown(currentNode, parentInfo, currentInfo, rescaleCommandInfo);
    hkpMoppTreeInternalNode* pNode = static_cast<hkpMoppTreeInternalNode*>(currentNode);
    hkpMoppTreeNode* pChild = pNode->m_rightBranch;
    hkpMoppAssemblerNodeInfo childInfo(currentInfo, pChild, m_codeInfo);
    assembleSubNodeIntoChunks(pChild, currentInfo, childInfo, chunkId, terminalsOut);
    pChild = pNode->m_leftBranch;
    childInfo = hkpMoppAssemblerNodeInfo(currentInfo, pChild, m_codeInfo);
    assembleSubNodeIntoChunks(pChild, currentInfo, childInfo, chunkId, terminalsOut);
    assembleNonTerminalCommand(currentNode, parentInfo, currentInfo, rescaleCommandInfo);
}

void hkpMoppDefaultAssembler::calcReoffset(
    hkpMoppTreeNode* currentNode,
    const hkpMoppAssemblerNodeInfo& parentInfo,
    hkpMoppAssemblerNodeInfo& currentInfo
) {
    currentInfo.m_terminalOffset = parentInfo.m_terminalOffset;
    uint32_t m_minPrimitiveId = currentNode->m_minPrimitiveId;
    uint32_t m_terminalSpread = currentInfo.m_terminalSpread;
    uint32_t v6 = m_minPrimitiveId + (m_terminalSpread - parentInfo.m_terminalOffset);
    if (v6 >= 32 &&
        (m_terminalSpread > 2 || v6 >= 256) &&
        ((m_terminalSpread < 32 && parentInfo.m_terminalSpread >= 32u) ||
         ((m_terminalSpread < 256 && parentInfo.m_terminalSpread >= 256u) ||
          (m_terminalSpread < 0x10000 && parentInfo.m_terminalSpread >= 0x10000u)))) {
        currentInfo.m_terminalOffset = m_minPrimitiveId;
    }
}

int32_t hkpMoppDefaultAssembler::calcRescaleBits(
    const hkpMoppAssemblerNodeInfo& parentInfo,
    hkpMoppAssemblerNodeInfo& currentInfo
) {
    int32_t v3 = currentInfo.m_maxExtentsSignificantBits - 8;
    if (v3 <= 0) {
        v3 = 0;
    }
    int32_t result = parentInfo.m_currentBitsResolution - v3;
    if (result > parentInfo.m_currentBitsResolution) {
        return parentInfo.m_currentBitsResolution;
    }
    return result;
}

// The planes of node's split in its integer space: the lower one into lowPlane and the one past the upper into
// highPlane, each clamped to 0..255. A split along one axis takes the node's two plane positions on that axis
// (floored); a diagonal one their distances from the plane through the node's accumulated offset, in grid steps,
// divided by the direction's length, the negative components' 255 steps each added. The double locals hold the plane
// positions, the scaled distances and the length factors; the float locals hold the values stored as floats.
void hkpMoppDefaultAssembler::findPlanes(
    const hkpMoppTreeInternalNode* node,
    const hkpMoppAssemblerNodeInfo& currentInfo,
    int32_t& highPlane,
    int32_t& lowPlane
) {
    const hkpMoppSplittingPlaneDirection* pPlane = node->m_plane;
    int32_t iNumAxes = 0;
    int32_t iNumNegative = 0;
    int32_t iAxis = 0;
    for (int32_t i = 0; i < 3; ++i) {
        float fComponent = pPlane->m_direction(i);
        if (fComponent != 0.0f) {
            ++iNumAxes;
            iAxis = i;
            if (fComponent < 0.0f) {
                ++iNumNegative;
            }
        }
    }
    float fRight = node->m_planeRightPosition;
    float fLeft = node->m_planeLeftPosition;
    if (iNumAxes == 1) {
        float fScale = m_codeInfo.m_offset(3);
        float fRightScaled = (fRight - m_codeInfo.m_offset(iAxis)) * fScale;
        int32_t iRight = hkMath::hkFloatToInt(hkFloor(fRightScaled));
        float fLeftScaled = (fLeft - m_codeInfo.m_offset(iAxis)) * fScale;
        int32_t iLeft = hkMath::hkFloatToInt(hkFloor(fLeftScaled));
        int32_t iOffset = currentInfo.m_accumOffset[iAxis];
        int32_t iShift = currentInfo.m_currentBitsResolution;
        int32_t iLow = (iRight - iOffset) >> iShift;
        int32_t iHigh = ((iLeft - iOffset) >> iShift) + 1;
        if (iLow <= 0) {
            iLow = 0;
        }
        else if (iLow >= 255) {
            iLow = 255;
        }
        if (iHigh <= 0) {
            iHigh = 0;
        }
        else if (iHigh >= 255) {
            iHigh = 255;
        }
        lowPlane = iLow;
        highPlane = iHigh;
        // Reads the stored lower plane back and clamps it again.
        if (lowPlane < 0) {
            lowPlane = 0;
        }
        return;
    }
    float fScale = m_codeInfo.m_offset(3);
    float fInvScale = 1.0f / fScale;
    // The accumulated offset as floats (w is zero), taken back to world space.
    __m128 vAccum = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_cvtsi32_ss(_mm_setzero_ps(), currentInfo.m_accumOffset[0]),
            _mm_cvtsi32_ss(_mm_setzero_ps(), currentInfo.m_accumOffset[2])
        ),
        _mm_unpacklo_ps(_mm_cvtsi32_ss(_mm_setzero_ps(), currentInfo.m_accumOffset[1]), _mm_set_ss(0.0f))
    );
    hkVector4 point;
    point.m_quad = _mm_add_ps(_mm_mul_ps(vAccum, _mm_set1_ps(fInvScale)), m_codeInfo.m_offset.m_quad);
    float fDistance = point.dot3(pPlane->m_direction);
    float fResolution = static_cast<float>(1 << currentInfo.m_currentBitsResolution);
    double dRight = (fRight - fDistance) * fScale / fResolution;
    double dLeft = (fLeft - fDistance) * fScale / fResolution;
    // The length of a two- or three-axis diagonal direction and its inverse square, as double constants holding widened
    // float values.
    double dLength = 1.0;
    double dInvLengthSquared = 1.0;
    if (iNumAxes == 2) {
        dLength = 1.4142135381698608;
        dInvLengthSquared = 0.5;
    }
    else if (iNumAxes == 3) {
        dLength = 1.7320507764816284;
        dInvLengthSquared = 0.3333333432674408;
    }
    dRight = dRight * dLength;
    dLeft = dLeft * dLength;
    if (iNumNegative != 0) {
        double dSteps = iNumNegative * 255;
        dRight = dRight + dSteps;
        dLeft = dLeft + dSteps;
    }
    int32_t iLow = hkFloorToInt(static_cast<float>(dRight * dInvLengthSquared));
    int32_t iHigh = hkFloorToInt(static_cast<float>(dInvLengthSquared * dLeft)) + 1;
    if (iLow <= 0) {
        iLow = 0;
    }
    else if (iLow >= 255) {
        iLow = 255;
    }
    if (iHigh <= 0) {
        lowPlane = iLow;
        highPlane = 0;
    }
    else {
        if (iHigh >= 255) {
            iHigh = 255;
        }
        lowPlane = iLow;
        highPlane = iHigh;
    }
}

// Rescales the node's integer space by up to 4 bits finer than the parent's: the node takes the parent's offsets
// snapped to the parent's grid, and the rescale is retried one bit coarser while an axis's extent would need 255
// steps or more. With no bits to gain the node keeps the parent's resolution and offsets.
void hkpMoppDefaultAssembler::recalcRescale(
    const hkpMoppAssemblerNodeInfo& parent,
    hkpMoppAssemblerNodeInfo& info,
    hkpMoppAssemblerRescaleCommand& cmd
) {
    int32_t rescaleBits = calcRescaleBits(parent, info);
    cmd.m_doRescale.m_bool = 0;
    if (rescaleBits <= 0) {
        info.m_currentBitsResolution = parent.m_currentBitsResolution;
        info.m_accumOffset[0] = parent.m_accumOffset[0];
        info.m_accumOffset[1] = parent.m_accumOffset[1];
        info.m_accumOffset[2] = parent.m_accumOffset[2];
        return;
    }
    if (rescaleBits >= 4) {
        rescaleBits = 4;
    }
retry:
    info.m_currentBitsResolution = parent.m_currentBitsResolution - rescaleBits;
    for (int32_t axis = 0; axis < 3; axis++) {
        int32_t parentBits = parent.m_currentBitsResolution;
        int32_t parentOffset = parent.m_accumOffset[axis];
        int32_t moppOffset = (info.m_extents[axis][0] - parentOffset) >> parentBits;
        info.m_accumOffset[axis] = parentOffset + (moppOffset << parentBits);
        cmd.m_moppOffset[axis] = moppOffset;
        if ((info.m_extents[axis][1] - info.m_accumOffset[axis]) >> info.m_currentBitsResolution >= 255) {
            --rescaleBits;
            goto retry;
        }
    }
    cmd.m_scaleCommand = rescaleBits;
    cmd.m_doRescale.m_bool = 1;
}

int32_t hkpMoppDefaultAssembler::getSplitCode(const float* direction) {
    int32_t v1;
    if (!(*direction > 0.10000000149011612)) {
        if (!(direction[1] > 0.10000000149011612)) {
            return 2;
        }

        v1 = 1;
        if (direction[2] > 0.10000000149011612) {
            return 3;
        }
        if (direction[2] < -0.10000000149011612) {
            return 4;
        }
    }
    else {
        v1 = 0;
        if (!(direction[2] > 0.10000000149011612)) {
            double v3 = direction[1];
            bool v4 = v3 < 0.10000000149011612;
            bool v5 = v3 == 0.10000000149011612;
            if (!(direction[2] < -0.10000000149011612)) {
                if (!v4 && !v5) {
                    return 7;
                }
                if (direction[1] < -0.10000000149011612) {
                    return 8;
                }
            }
            else {
                v1 = 6;
                if (!v4 && !v5) {
                    return 10;
                }
                if (direction[1] < -0.10000000149011612) {
                    return 12;
                }
            }
        }
        else {
            v1 = 5;
            if (direction[1] > 0.10000000149011612) {
                return 9;
            }
            if (direction[1] < -0.10000000149011612) {
                return 11;
            }
        }
    }
    return v1;
}

void hkpMoppCodeGenerator::addCommand8(int32_t command, int32_t param) {
    m_code[m_size - m_pos++ - 1] = param;
    if (m_pos >= m_size) {
        resize();
    }
    m_code[m_size - m_pos++ - 1] = command;
    if (m_pos >= m_size) {
        resize();
    }
}

void hkpMoppCodeGenerator::addCommand16(int32_t command, int32_t param) {
    m_code[m_size - m_pos++ - 1] = param;
    if (m_pos >= m_size) {
        resize();
    }
    m_code[m_size - m_pos++ - 1] = static_cast<uint8_t>(param >> 8);
    if (m_pos >= m_size) {
        resize();
    }
    m_code[m_size - m_pos++ - 1] = command;
    if (m_pos >= m_size) {
        resize();
    }
}

void hkpMoppCodeGenerator::addCommand24(int32_t command, int32_t param) {
    m_code[m_size - m_pos++ - 1] = param;
    if (m_pos >= m_size) {
        resize();
    }
    m_code[m_size - m_pos++ - 1] = static_cast<uint8_t>(param >> 8);
    if (m_pos >= m_size) {
        resize();
    }
    m_code[m_size - m_pos++ - 1] = static_cast<uint8_t>(param >> 16);
    if (m_pos >= m_size) {
        resize();
    }
    m_code[m_size - m_pos++ - 1] = command;
    if (m_pos >= m_size) {
        resize();
    }
}

void hkpMoppCodeGenerator::addParam24(int32_t param) {
    m_code[m_size - m_pos++ - 1] = param;
    if (m_pos >= m_size) {
        resize();
    }
    m_code[m_size - m_pos++ - 1] = static_cast<uint8_t>(param >> 8);
    if (m_pos >= m_size) {
        resize();
    }
    m_code[m_size - m_pos++ - 1] = static_cast<uint8_t>(param >> 16);
    if (m_pos >= m_size) {
        resize();
    }
}

void hkpMoppCodeGenerator::addCommand32(int32_t command, uint32_t param) {
    m_code[m_size - m_pos++ - 1] = param;
    if (m_pos >= m_size) {
        resize();
    }
    m_code[m_size - m_pos++ - 1] = static_cast<uint8_t>(param >> 8);
    if (m_pos >= m_size) {
        resize();
    }
    m_code[m_size - m_pos++ - 1] = static_cast<uint8_t>(param >> 16);
    if (m_pos >= m_size) {
        resize();
    }
    m_code[m_size - m_pos++ - 1] = static_cast<uint8_t>(param >> 24);
    if (m_pos >= m_size) {
        resize();
    }
    m_code[m_size - m_pos++ - 1] = command;
    if (m_pos >= m_size) {
        resize();
    }
}

void hkpMoppCodeGenerator::pad(int32_t size) {
    if (size > 0) {
        int32_t v3 = size;
        do {
            m_code[m_size - m_pos++ - 1] = -51;
            if (m_pos >= m_size) {
                resize();
            }
            --v3;
        }
        while (v3);
    }
}

void hkpMoppCodeGenerator::addCode(int32_t code) {
    m_code[m_size - m_pos++ - 1] = code;
    if (m_pos >= m_size) {
        resize();
    }
}
