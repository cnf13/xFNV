#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppUtility.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppCompilerChunkInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppCompilerInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Compiler/hkpMoppCompiler.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Mediator/CachedShape/hkpMoppCachedShapeMediator.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Mediator/Shape/hkpMoppShapeMediator.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Mediator/hkpMoppMediator.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"

hkpMoppCode* hkpMoppUtility::buildCode(
    const hkpShapeContainer* shapeContainer,
    const hkpMoppCompilerInput& moppInput,
    hkArray<hkpMoppCodeReindexedTerminal, hkContainerHeapAllocator>* reindexInfo
) {
    hkpMoppCode* v3;
    if (moppInput.m_cachePrimitiveExtents.m_bool) {
        hkpMoppCachedShapeMediator a1(shapeContainer);
        v3 = hkpMoppUtility::buildCodeInternal(a1, shapeContainer, moppInput, reindexInfo);
    }
    else {
        hkpMoppShapeMediator apMemory(shapeContainer);
        v3 = hkpMoppUtility::buildCodeInternal(apMemory, shapeContainer, moppInput, reindexInfo);
    }
    v3->m_buildType.m_storage = moppInput.m_enableChunkSubdivision.m_bool == 0;
    return v3;
}

hkpMoppCode* hkpMoppUtility::buildCodeInternal(
    hkpMoppMediator& mediator,
    const hkpShapeContainer* shapeContainer,
    const hkpMoppCompilerInput& moppInput,
    hkArray<hkpMoppCodeReindexedTerminal>* reindexInfo
) {
    (void)shapeContainer;
    alignas(16) hkpMoppCompiler kCompiler;
    kCompiler.~hkpMoppCompiler();
    ::new (&kCompiler) hkpMoppCompiler(0);

    alignas(16) hkpMoppCostFunction::hkpMoppSplitCostParams kCostParams;
    kCostParams.~hkpMoppSplitCostParams();
    ::new (&kCostParams) hkpMoppCostFunction::hkpMoppSplitCostParams(0);
    kCostParams.m_weightPrimitiveSplit = 1.0f;
    if (!moppInput.m_useShapeKeys || moppInput.m_enableChunkSubdivision) {
        kCostParams.m_weightPrimitiveIdSpread = 0.0f;
    }
    kCompiler.setCostParams(kCostParams);

    bool bInterleaved = moppInput.m_enableInterleavedBuilding;
    if (moppInput.m_enableChunkSubdivision) {
        bInterleaved = false;
    }

    // EXE-INLINED none: hkpMoppAssembler::hkpMoppAssemblerParams::hkpMoppAssemblerParams
    alignas(16) hkpMoppAssembler::hkpMoppAssemblerParams kAssemblerParams;
    kAssemblerParams.m_relativeFitToleranceOfInternalNodes = 0.5f;
    kAssemblerParams.m_absoluteFitToleranceOfInternalNodes = 0.2f;
    kAssemblerParams.m_absoluteFitToleranceOfTriangles = 1.0f;
    kAssemblerParams.m_groupLevels = 5;
    kAssemblerParams.m_interleavedBuildingEnabled.m_bool = 1;
    kAssemblerParams.m_absoluteFitToleranceOfAxisAlignedTriangles.m_quad[0] = 0.2f;
    kAssemblerParams.m_absoluteFitToleranceOfAxisAlignedTriangles.m_quad[1] = 0.2f;
    kAssemblerParams.m_absoluteFitToleranceOfAxisAlignedTriangles.m_quad[2] = 0.05f;
    kAssemblerParams.m_absoluteFitToleranceOfAxisAlignedTriangles.m_quad[3] = 0.0f;

    kAssemblerParams.m_relativeFitToleranceOfInternalNodes = moppInput.getRelativeFitToleranceOfInternalNodes();
    kAssemblerParams.m_absoluteFitToleranceOfInternalNodes = moppInput.getAbsoluteFitToleranceOfInternalNodes();
    kAssemblerParams.m_absoluteFitToleranceOfTriangles = moppInput.getAbsoluteFitToleranceOfTriangles();
    kAssemblerParams.m_absoluteFitToleranceOfAxisAlignedTriangles =
        moppInput.getAbsoluteFitToleranceOfAxisAlignedTriangles();
    kAssemblerParams.m_interleavedBuildingEnabled.m_bool = bInterleaved;
    kCompiler.setAssemblerParams(kAssemblerParams);

    alignas(16) hkpMoppSplitter::hkpMoppSplitParams kSplitParams;
    kSplitParams.~hkpMoppSplitParams();
    ::new (&kSplitParams) hkpMoppSplitter::hkpMoppSplitParams(0);
    if (moppInput.m_enablePrimitiveSplitting) {
        kSplitParams.m_maxPrimitiveSplitsPerNode = 50;
    }
    else {
        kSplitParams.m_maxPrimitiveSplits = 0;
        kSplitParams.m_maxPrimitiveSplitsPerNode = 0;
    }
    kSplitParams.m_interleavedBuildingEnabled.m_bool = bInterleaved;
    kSplitParams.m_minRangeMaxListCheck = 5;
    kCompiler.setSplitParams(kSplitParams);

    int32_t iBufferSize = kCompiler.calculateRequiredBufferSize(&mediator);
    char* pBuffer = static_cast<char*>(hkMemoryRouter::getInstance().temp().blockAlloc(iBufferSize));

    // EXE-INLINED none: hkpMoppCompilerChunkInfo::hkpMoppCompilerChunkInfo
    FNVD_INLINED_LOCAL(hkpMoppCompilerChunkInfo, kChunkInfo, std::nullopt, std::nullopt);
    kChunkInfo.m_maxChunkSize = 0x200;
    kChunkInfo.m_reindexInfo.m_data = nullptr;
    kChunkInfo.m_reindexInfo.m_size = 0;
    kChunkInfo.m_reindexInfo.m_capacityAndFlags = 0x80000000;
    kChunkInfo.m_chunks.m_data = nullptr;
    kChunkInfo.m_chunks.m_size = 0;
    kChunkInfo.m_chunks.m_capacityAndFlags = 0x80000000;
    kChunkInfo.m_compressor = nullptr;
    kChunkInfo.m_safetySize = 0x14;
    if (moppInput.m_enableChunkSubdivision) {
        kCompiler.m_chunkInfo = &kChunkInfo;
    }

    // Warning id 0x7DD65995 is switched off around the compile and restored after.
    hkBool bWasEnabled;
    hkError::s_instance->isEnabled(&bWasEnabled, 0x7DD65995);
    hkBool kDisabled;
    kDisabled.m_bool = 0;
    hkError::s_instance->setEnabled(0x7DD65995, kDisabled);
    hkpMoppCode* pCode = kCompiler.compile(&mediator, pBuffer, iBufferSize);
    hkError::s_instance->setEnabled(0x7DD65995, bWasEnabled);

    if (reindexInfo) {
        reindexInfo->_insertAt(
            hkMemoryRouter::getInstance().heap(), 0, kChunkInfo.m_reindexInfo.m_data, kChunkInfo.m_reindexInfo.m_size
        );
    }
    hkMemoryRouter::getInstance().temp().blockFree(pBuffer, iBufferSize);
    kChunkInfo.~hkpMoppCompilerChunkInfo();
    return pCode;
}
