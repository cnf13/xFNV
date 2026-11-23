#include "Gamebryo2.2/CoreLibs/NiMain/BSPackedAdditionalGeometryData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSPackedAdditionalGeometryData::ms_RTTI("BSPackedAdditionalGeometryData", &NiAdditionalGeometryData::ms_RTTI);

// NiAGDDataBlock's constructor clears the size, the buffer and the ownership flag.
BSPackedAdditionalGeometryData::NiBSPackedAGDDataBlock::NiBSPackedAGDDataBlock() : NiAGDDataBlock() {
    m_bBSHeap = false;
}

BSPackedAdditionalGeometryData::NiBSPackedAGDDataBlock::~NiBSPackedAGDDataBlock() {}

uint8_t* BSPackedAdditionalGeometryData::NiBSPackedAGDDataBlock::AllocateBuffer(uint32_t uiBufferSize) {
    return NiRenderer::ms_pkRenderer->AllocateVertexBufferMemory(uiBufferSize);
}

// Without a buffer argument the block's own buffer is used, if the block owns it. Bethesda-heap blocks are skipped.
void BSPackedAdditionalGeometryData::NiBSPackedAGDDataBlock::ProtectBuffer(void* pvBuffer, uint32_t auiBufferSize) {
    uint8_t* pucBuffer = static_cast<uint8_t*>(pvBuffer);
    uint32_t uiBufferSize = auiBufferSize;
    if (!pvBuffer && m_bOwnData) {
        pucBuffer = m_pucDataBlock;
        uiBufferSize = m_uiDataBlockSize;
    }
    if (!m_bBSHeap) {
        NiRenderer::ms_pkRenderer->ProtectVertexBufferMemory(pucBuffer, uiBufferSize);
    }
}

// Without a buffer argument the block's own buffer is used, if the block owns it. Bethesda-heap blocks are skipped.
void BSPackedAdditionalGeometryData::NiBSPackedAGDDataBlock::UnprotectBuffer(void* pvBuffer, uint32_t auiBufferSize) {
    uint8_t* pucBuffer = static_cast<uint8_t*>(pvBuffer);
    uint32_t uiBufferSize = auiBufferSize;
    if (!pvBuffer && m_bOwnData) {
        pucBuffer = m_pucDataBlock;
        uiBufferSize = m_uiDataBlockSize;
    }
    if (!m_bBSHeap) {
        NiRenderer::ms_pkRenderer->UnprotectVertexBufferMemory(pucBuffer, uiBufferSize);
    }
}

// Without a buffer argument the block's own buffer is freed, if the block owns it.
void BSPackedAdditionalGeometryData::NiBSPackedAGDDataBlock::FreeBuffer(void* pvBuffer) {
    uint8_t* pucBuffer = static_cast<uint8_t*>(pvBuffer);
    if (!pvBuffer && m_bOwnData) {
        pucBuffer = m_pucDataBlock;
    }
    NiRenderer::ms_pkRenderer->FreeVertexBufferMemory(pucBuffer);
}

BSPackedAdditionalGeometryData::BSPackedAdditionalGeometryData()
  : NiAdditionalGeometryData(), m_uiShaderDeclarationIndex(0), m_uiVertexStride(0) {}

BSPackedAdditionalGeometryData::~BSPackedAdditionalGeometryData() {}

void BSPackedAdditionalGeometryData::LoadBinary(NiStream& kStream) {
    NiAdditionalGeometryData::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_uiShaderDeclarationIndex);
    NiStreamLoadBinary(kStream, m_uiVertexStride);
}

void BSPackedAdditionalGeometryData::LinkObject(NiStream& kStream) {
    NiAdditionalGeometryData::LinkObject(kStream);
}

bool BSPackedAdditionalGeometryData::RegisterStreamables(NiStream& kStream) {
    return NiAdditionalGeometryData::RegisterStreamables(kStream);
}

void BSPackedAdditionalGeometryData::SaveBinary(NiStream& kStream) {
    NiAdditionalGeometryData::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uiShaderDeclarationIndex);
    NiStreamSaveBinary(kStream, m_uiVertexStride);
}

bool BSPackedAdditionalGeometryData::IsEqual(NiObject* pkObject) {
    if (!NiAdditionalGeometryData::IsEqual(pkObject)) {
        return false;
    }

    BSPackedAdditionalGeometryData* pkOther = static_cast<BSPackedAdditionalGeometryData*>(pkObject);
    return m_uiShaderDeclarationIndex == pkOther->m_uiShaderDeclarationIndex &&
        m_uiVertexStride == pkOther->m_uiVertexStride;
}

void BSPackedAdditionalGeometryData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiAdditionalGeometryData::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_uiShaderDeclarationIndex", m_uiShaderDeclarationIndex));
}

// The block is built with Bethesda's global new.
NiAdditionalGeometryData::NiAGDDataBlock* BSPackedAdditionalGeometryData::CreateAGDDataBlock() {
    return new NiBSPackedAGDDataBlock;
}

NiObject* BSPackedAdditionalGeometryData::CreateObject() {
    BSPackedAdditionalGeometryData* pkObject = new BSPackedAdditionalGeometryData;
    return pkObject;
}
