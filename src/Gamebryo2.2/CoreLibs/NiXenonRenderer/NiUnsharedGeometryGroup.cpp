#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiUnsharedGeometryGroup.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"

NiUnsharedGeometryGroup::NiUnsharedGeometryGroup() {}

// Clears the shared group pointer when it is this group.
NiUnsharedGeometryGroup::~NiUnsharedGeometryGroup() {
    if (this == ms_pkGroup) {
        ms_pkGroup = nullptr;
    }
}

// Gives the partition (or else the data) its own buffer data, once, and adds it to the group: the primitive type from
// the partition's strips or the data's class, the vertex flags from the data's components. Writes through the result
// even when the allocation fails.
void NiUnsharedGeometryGroup::AddObject(
    NiGeometryData* apData,
    NiSkinInstance* apSkinInstance,
    NiSkinPartition::Partition* apPartition
) {
    NiGeometryBufferData* pkBuffData;
    if (apPartition) {
        if (apPartition->m_pkBuffData) {
            return;
        }
        pkBuffData = ::new NiGeometryBufferData;
        pkBuffData->m_eType = apPartition->m_usStrips ? D3DPT_TRIANGLESTRIP : D3DPT_TRIANGLELIST;
        apPartition->m_pkBuffData = pkBuffData;
    }
    else {
        if (apData->m_pkBuffData) {
            return;
        }
        pkBuffData = ::new NiGeometryBufferData;
        bool bTriShape = false;
        for (const NiRTTI* pkRTTI = apData->GetRTTI(); pkRTTI; pkRTTI = pkRTTI->GetBaseRTTI()) {
            if (pkRTTI == &NiTriShapeData::ms_RTTI) {
                bTriShape = true;
                break;
            }
        }
        if (bTriShape) {
            pkBuffData->m_eType = D3DPT_TRIANGLELIST;
        }
        else if (NiObject::IsKindOf_0(&NiTriStripsData::ms_RTTI, apData)) {
            pkBuffData->m_eType = D3DPT_TRIANGLESTRIP;
        }
        apData->m_pkBuffData = pkBuffData;
    }
    bool bBinormalsTangents = apData->m_pkNormal && (apData->m_usDataFlags & NiGeometryData::NBT_METHOD_MASK) != 0;
    uint32_t uiUVCount = apData->m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK;
    if (uiUVCount > NiD3DPass::ms_uiMaxTextureBlendStages) {
        uiUVCount = NiD3DPass::ms_uiMaxTextureBlendStages;
    }
    uint32_t uiFlags = 0;
    if (apData->m_pkColor) {
        uiFlags = NiDX9Renderer::VERTEX_FLAG_DX_HASCOLORS;
    }
    if (apData->m_pkNormal) {
        uiFlags |= NiDX9Renderer::VERTEX_FLAG_DX_HASNORMALS;
    }
    if (bBinormalsTangents) {
        uiFlags |= NiDX9Renderer::VERTEX_FLAG_DX_HASBINORMALSTANGENTS;
    }
    pkBuffData->m_uiFlags = uiFlags | uiUVCount << NiDX9Renderer::VERTEX_FLAG_DX_TEXTURESHIFT;
    NiGeometryGroup::AddObject(pkBuffData);
}

void NiUnsharedGeometryGroup::AddObject(NiScreenTexture* apScreenTexture) {
    if (!apScreenTexture->m_pkBuffData) {
        NiGeometryBufferData* pkBuffData = ::new NiGeometryBufferData;
        pkBuffData->m_eType = D3DPT_TRIANGLELIST;
        apScreenTexture->m_pkBuffData = pkBuffData;
        pkBuffData->m_uiFlags = 0x1400000;
        NiGeometryGroup::AddObject(pkBuffData);
    }
}

// Takes the data's buffer data out of the group and deletes it.
void NiUnsharedGeometryGroup::RemoveObject(NiGeometryData* apData) {
    NiGeometryBufferData* pkBuffData = static_cast<NiGeometryBufferData*>(apData->m_pkBuffData);
    if (pkBuffData) {
        NiGeometryGroup::RemoveObject(pkBuffData);
        delete pkBuffData;
        apData->m_pkBuffData = nullptr;
    }
}

// Takes the partition's buffer data out of the group and deletes it.
void NiUnsharedGeometryGroup::RemoveObject(NiSkinPartition::Partition* apPartition) {
    NiGeometryBufferData* pkBuffData = apPartition->m_pkBuffData;
    if (pkBuffData) {
        NiGeometryGroup::RemoveObject(pkBuffData);
        delete pkBuffData;
        apPartition->m_pkBuffData = nullptr;
    }
}

// Takes the screen texture's buffer data out of the group and deletes it.
void NiUnsharedGeometryGroup::RemoveObject(NiScreenTexture* apScreenTexture) {
    NiGeometryBufferData* pkBuffData = apScreenTexture->m_pkBuffData;
    if (pkBuffData) {
        NiGeometryGroup::RemoveObject(pkBuffData);
        delete pkBuffData;
        apScreenTexture->m_pkBuffData = nullptr;
    }
}

// One vertex buffer per chip, write-only (and software-processed for software vertex processing), managed unless the
// renderer uses Direct3D9Ex. pkVB is not cleared before CreateVertexBuffer, so a call that fails without writing pkVB
// leaves an uninitialised value in the chip.
NiVBChip* NiUnsharedGeometryGroup::CreateChip(NiGeometryBufferData* pkBuffData, uint32_t uiStream) {
    uint32_t uiStride;
    if (uiStream >= pkBuffData->m_uiStreamCount) {
        uiStride = 0;
    }
    else {
        uiStride = pkBuffData->m_puiVertexStride[uiStream];
    }
    uint32_t uiBufferSize = pkBuffData->m_uiMaxVertCount * uiStride;
    uint32_t uiUsage = D3DUSAGE_WRITEONLY;
    if (pkBuffData->m_bSoftwareVP) {
        uiUsage = D3DUSAGE_WRITEONLY | D3DUSAGE_SOFTWAREPROCESSING;
    }
    IDirect3DDevice9* pkD3DDevice = m_pkD3DDevice;
    IDirect3DVertexBuffer9* pkVB;
    // Tests the whole byte against zero, as SetPoolAndUsage does.
    pkD3DDevice->CreateVertexBuffer(
        uiBufferSize,
        uiUsage,
        pkBuffData->m_uiFVF,
        *reinterpret_cast<const uint8_t*>(&NiDX9Renderer::bUseDirect3D9Ex) == 0,
        &pkVB,
        nullptr
    );
    NiVBChip* pkChip = new NiVBChip;
    if (!pkChip) {
        return nullptr;
    }
    pkChip->m_pkVB = pkVB;
    pkChip->m_uiSize = uiBufferSize;
    pkChip->m_uiIndex = 0;
    pkChip->m_pkBlock = nullptr;
    pkChip->m_uiOffset = 0;
    pkChip->m_uiLockFlags = 0;
    return pkChip;
}

// Releases the chip's vertex buffer and frees the chip; the stream index is re-tested before clearing the slot.
void NiUnsharedGeometryGroup::ReleaseChip(NiGeometryBufferData* pkBuffData, uint32_t uiStream) {
    if (uiStream < pkBuffData->m_uiStreamCount) {
        NiVBChip* pkChip = pkBuffData->m_ppkVBChip[uiStream];
        if (pkChip) {
            if (pkChip->m_pkVB) {
                pkChip->m_pkVB->Release();
            }
            if (uiStream < pkBuffData->m_uiStreamCount) {
                pkBuffData->m_ppkVBChip[uiStream] = nullptr;
            }
            pkChip->m_pkVB = nullptr;
            delete pkChip;
        }
    }
}

// Unshared geometry is not dynamic.
bool NiUnsharedGeometryGroup::IsDynamic() {
    return false;
}

NiGeometryGroup* NiUnsharedGeometryGroup::Create() {
    if (!ms_pkGroup) {
        ms_pkGroup = new NiUnsharedGeometryGroup;
    }
    return ms_pkGroup;
}
