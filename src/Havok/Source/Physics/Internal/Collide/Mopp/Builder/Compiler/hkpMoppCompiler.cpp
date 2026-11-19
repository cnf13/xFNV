#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Compiler/hkpMoppCompiler.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppCompilerChunkInfo.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppAssembler.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppCodeGenerator.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppDefaultAssembler.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Mediator/hkpMoppMediator.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppCostFunction.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppDefaultSplitter.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppSplitter.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <xmmintrin.h>

int32_t writeChunkOffset(uint8_t* a1, int32_t r_eax) {
    a1[1] = static_cast<uint8_t>(r_eax >> 24);
    a1[2] = static_cast<uint8_t>(r_eax >> 16);
    a1[3] = static_cast<uint8_t>(r_eax >> 8);
    a1[4] = r_eax;
    return r_eax;
}

int32_t readChunkId(const uint8_t* apThis) {
    return *(apThis + 4) + ((*(apThis + 3) + ((*(apThis + 2) + (*(apThis + 1) << 8)) << 8)) << 8);
}

hkpMoppCompiler::hkpMoppCompiler(int32_t aiMeshType)
  : m_chunkInfo(nullptr), m_splitParams(aiMeshType), m_splitCostParams(aiMeshType) {
    m_assemblerParams.m_relativeFitToleranceOfInternalNodes = 0.5f;
    m_assemblerParams.m_absoluteFitToleranceOfInternalNodes = 0.2f;
    m_assemblerParams.m_absoluteFitToleranceOfTriangles = 1.0f;
    m_assemblerParams.m_groupLevels = 5;
    m_assemblerParams.m_absoluteFitToleranceOfAxisAlignedTriangles.set(0.2f, 0.2f, 0.05f, 0.0f);
    m_assemblerParams.m_interleavedBuildingEnabled = true;
}

// The fields are copied one by one, in order, rather than with a struct copy, because a struct copy gives other bytes
// when the source overlaps this.
void hkpMoppCompiler::setSplitParams(const hkpMoppSplitter::hkpMoppSplitParams& sp) {
    m_splitParams.m_tolerance = sp.m_tolerance;
    m_splitParams.m_maxPrimitiveSplits = sp.m_maxPrimitiveSplits;
    m_splitParams.m_maxPrimitiveSplitsPerNode = sp.m_maxPrimitiveSplitsPerNode;
    m_splitParams.m_minRangeMaxListCheck = sp.m_minRangeMaxListCheck;
    m_splitParams.m_checkAllEveryN = sp.m_checkAllEveryN;
    m_splitParams.m_interleavedBuildingEnabled = sp.m_interleavedBuildingEnabled;
    m_splitParams.padding15[0] = sp.padding15[0];
    m_splitParams.padding15[1] = sp.padding15[1];
    m_splitParams.padding15[2] = sp.padding15[2];
}

int32_t hkpMoppCompiler::calculateRequiredBufferSize(hkpMoppMediator* a1) {
    int32_t v3 = m_splitParams.m_maxPrimitiveSplits + a1->getNumPrimitives();
    int32_t v4 = 4096;
    if (!m_splitParams.m_interleavedBuildingEnabled.m_bool) {
        v4 = v3;
    }
    return (224 * v4) + (20 * v3);
}

void hkpMoppCompiler::setCostParams(const hkpMoppCostFunction::hkpMoppSplitCostParams& sp) {
    m_splitCostParams = sp;
}

void hkpMoppCompiler::setAssemblerParams(const hkpMoppAssembler::hkpMoppAssemblerParams& ap) {
    m_assemblerParams = ap;
}

// The three builder locals are destroyed at the end in a fixed order (the assembler, then the cost function, then the
// code generator): the locals take no destructor and the calls are written out there. The splitter lives only around
// buildTree.
hkpMoppCode* hkpMoppCompiler::compile(hkpMoppMediator* mediator, char* buffer, int32_t bufferSize) {
    int32_t iNumPrimitives = m_splitParams.m_maxPrimitiveSplits + mediator->getNumPrimitives();
    alignas(16) hkpMoppCodeGenerator codeGenerator(12 * iNumPrimitives);
    alignas(16) hkpMoppCostFunction costFunction(m_splitCostParams);
    alignas(16) hkpMoppDefaultAssembler assembler(m_assemblerParams, &codeGenerator, mediator);
    assembler.m_chunkInfo = m_chunkInfo;
    // The assembler's virtuals are called through a base pointer, the count before the directions.
    hkpMoppAssembler* pAssembler = &assembler;
    int32_t iNumDirections = pAssembler->getNumSplittingPlaneDirections();
    const hkpMoppSplittingPlaneDirection* pDirections = pAssembler->getSplittingPlaneDirections();
    mediator->setSplittingPlaneDirections(pDirections, iNumDirections);

    int32_t iBufferSize = calculateRequiredBufferSize(mediator);
    char* pBuffer = buffer;
    if (!buffer || bufferSize < iBufferSize) {
        pBuffer = static_cast<char*>(hkMemoryRouter::easyAlloc(*hkMemoryRouter::getInstance().m_heap, iBufferSize));
    }
    int32_t iNumNodes = iNumPrimitives;
    if (m_splitParams.m_interleavedBuildingEnabled.m_bool) {
        iNumNodes = 4096;
    }
    // The buffer is untyped scratch memory carved into the primitives, the inner nodes and the leaves.
    hkpMoppSplitter::hkpMoppScratchArea scratchArea;
    scratchArea.m_primitives = reinterpret_cast<hkpMoppCompilerPrimitive*>(pBuffer);
    char* pNodes = pBuffer + iNumPrimitives * static_cast<int32_t>(sizeof(hkpMoppCompilerPrimitive));
    scratchArea.m_nodes = reinterpret_cast<hkpMoppTreeInternalNode*>(pNodes);
    scratchArea.m_terminals = reinterpret_cast<hkpMoppTreeTerminal*>(
        pNodes + iNumNodes * static_cast<int32_t>(sizeof(hkpMoppTreeInternalNode))
    );
    mediator->getPrimitives(scratchArea.m_primitives);

    hkpMoppTreeNode* pRootNode;
    {
        hkpMoppDefaultSplitter splitter;
        pRootNode =
            splitter.hkpMoppDefaultSplitter::buildTree(mediator, &costFunction, &assembler, m_splitParams, scratchArea);
    }
    if (pBuffer != buffer) {
        hkMemoryRouter::easyFree(*hkMemoryRouter::getInstance().m_heap, pBuffer);
    }
    m_debugRootNode = pRootNode;

    hkpMoppCode* pCode;
    if (!m_chunkInfo) {
        pCode = codeGenerator.compileCode();
    }
    else {
        // The chunks' start offsets in the joined code: chunk i starts at chunkOffsets[i], the last entry is the
        // total size.
        hkArray<int32_t> chunkOffsets;
        chunkOffsets.setSize(m_chunkInfo->m_chunks.getSize() + 1);
        chunkOffsets.m_data[0] = 0;
        for (int32_t i = 0; i < m_chunkInfo->m_chunks.getSize(); ++i) {
            int32_t iChunkSize;
            if (m_chunkInfo->m_compressor) {
                iChunkSize = m_chunkInfo->m_maxChunkSize;
            }
            else {
                iChunkSize = (m_chunkInfo->m_chunks.m_data[i].m_code->m_pos + 15) & ~15;
            }
            chunkOffsets.m_data[i + 1] = chunkOffsets.m_data[i] + iChunkSize;
        }
        // Patches each chunk's jump commands: the chunk id after the command byte becomes that chunk's offset.
        for (const hkpMoppCompilerChunkInfo::Chunk& rChunk : m_chunkInfo->m_chunks) {
            hkArray<hkpMoppCodeGenerator::JumpCommandInfo>& rJumps = rChunk.m_code->m_jumpCommands;
            for (const hkpMoppCodeGenerator::JumpCommandInfo& rJump : rJumps) {
                hkpMoppCodeGenerator* pChunkCode = rChunk.m_code;
                uint8_t* pCommand = pChunkCode->m_code - rJump.m_position + pChunkCode->m_size;
                int32_t iChunkId = readChunkId(pCommand);
                writeChunkOffset(pCommand, chunkOffsets.m_data[iChunkId]);
            }
        }
        int32_t iTotalSize = chunkOffsets.m_data[chunkOffsets.getSize() - 1];
        pCode = static_cast<hkpMoppCode*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpMoppCode)));
        pCode->m_memSizeAndFlags = sizeof(hkpMoppCode);
        ::new (pCode) hkpMoppCode;
        pCode->m_data.reserveExactly(iTotalSize);
        pCode->m_data.setSize(iTotalSize, 0xCD);
        // Joins the chunks' code, dropping each chunk's own code object and generator.
        for (int32_t i = 0; i < m_chunkInfo->m_chunks.getSize(); ++i) {
            uint8_t* pDest = pCode->m_data.m_data + chunkOffsets.m_data[i];
            hkpMoppCode* pChunkCode = m_chunkInfo->m_chunks.m_data[i].m_code->compileCode();
            hkString::memCpy(pDest, pChunkCode->m_data.m_data, pChunkCode->m_data.getSize());
            delete pChunkCode;
            m_chunkInfo->m_chunks.m_data[i].m_code->removeReference();
        }
    }
    pAssembler->getScaleInfo(pRootNode, &pCode->m_info);
    return pCode;
}

void hkpMoppCompiler::enableInterleavedBuilding(bool a1) {
    m_splitParams.m_interleavedBuildingEnabled.m_bool = (int8_t)a1;
    m_assemblerParams.m_interleavedBuildingEnabled.m_bool = (int8_t)a1;
}
