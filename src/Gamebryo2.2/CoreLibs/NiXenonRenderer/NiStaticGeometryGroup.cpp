#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiStaticGeometryGroup.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBBlock.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBSet.h"
#include "SDK/MSVC/string.h"

// The set map has 37 buckets.
NiStaticGeometryGroup::NiStaticGeometryGroup() : m_kSets(37) {}

// Removes each set's entry from the map before deleting the set.
void NiStaticGeometryGroup::Purge() {
    NiTMapItem<uint32_t, NiVBSet*>* pkPos = m_kSets.GetFirstPos();
    while (pkPos) {
        uint32_t uiIndex;
        NiVBSet* pkSet;
        m_kSets.GetNext(pkPos, uiIndex, pkSet);
        m_kSets.RemoveAt(uiIndex);
        delete pkSet;
    }
}

// Unlike the dynamic and unshared groups, this does not store the new buffer data in the screen texture.
void NiStaticGeometryGroup::AddObject(NiScreenTexture* pkScreenTexture) {
    if (!pkScreenTexture->m_pkBuffData) {
        NiGeometryBufferData* pkBuffData = ::new NiGeometryBufferData;
        pkBuffData->m_eType = D3DPT_TRIANGLELIST;
        pkBuffData->m_uiFlags = 0x1400000;
        NiGeometryGroup::AddObject(pkBuffData);
    }
}

void NiStaticGeometryGroup::AddObject(
    NiGeometryData* pkData,
    NiSkinInstance* pkSkinInstance,
    NiSkinPartition::Partition* pkPartition
) {
    NiGeometryBufferData* v5;
    if (pkPartition) {
        if (pkPartition->m_pkBuffData) {
            return;
        }
        v5 = ::new NiGeometryBufferData;
        v5->m_eType = static_cast<D3DPRIMITIVETYPE>((pkPartition->m_usStrips != 0) + D3DPT_TRIANGLELIST);
        pkPartition->m_pkBuffData = v5;
    }
    else {
        if (pkData->m_pkBuffData) {
            return;
        }
        v5 = ::new NiGeometryBufferData;
        const NiRTTI* v7 = pkData->GetRTTI();
        while (v7 && v7 != &NiTriShapeData::ms_RTTI) {
            v7 = v7->GetBaseRTTI();
        }
        if (v7) {
            v5->m_eType = D3DPT_TRIANGLELIST;
        }
        else {
            if (NiObject::IsKindOf_0(&NiTriStripsData::ms_RTTI, pkData)) {
                v5->m_eType = D3DPT_TRIANGLESTRIP;
            }
        }
        pkData->m_pkBuffData = v5;
    }
    char pkPartitiona = pkData->m_pkNormal && (pkData->m_usDataFlags & NiGeometryData::NBT_METHOD_MASK) != 0;
    uint32_t v8 = pkData->m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK;
    if (v8 > NiD3DPass::ms_uiMaxTextureBlendStages) {
        v8 = NiD3DPass::ms_uiMaxTextureBlendStages;
    }
    int32_t v9 = 0;
    if (pkData->m_pkColor) {
        v9 = 0x400000;
    }
    if (pkData->m_pkNormal) {
        v9 |= 0x800000u;
    }
    if (pkPartitiona) {
        v9 |= 0x200000u;
    }
    v5->m_uiFlags = v9 | v8 << 24;
    NiGeometryGroup::AddObject(v5);
}

// Takes the screen texture's buffer data out of the group and deletes it.
void NiStaticGeometryGroup::RemoveObject(NiScreenTexture* apScreenTexture) {
    NiGeometryBufferData* pkBuffData = apScreenTexture->m_pkBuffData;
    if (pkBuffData) {
        NiGeometryGroup::RemoveObject(pkBuffData);
        delete pkBuffData;
        apScreenTexture->m_pkBuffData = nullptr;
    }
}

// Takes the partition's buffer data out of the group and deletes it.
void NiStaticGeometryGroup::RemoveObject(NiSkinPartition::Partition* apPartition) {
    NiGeometryBufferData* pkBuffData = apPartition->m_pkBuffData;
    if (pkBuffData) {
        NiGeometryGroup::RemoveObject(pkBuffData);
        delete pkBuffData;
        apPartition->m_pkBuffData = nullptr;
    }
}

// Takes the data's buffer data out of the group and deletes it.
void NiStaticGeometryGroup::RemoveObject(NiGeometryData* apData) {
    NiGeometryBufferData* pkBuffData = static_cast<NiGeometryBufferData*>(apData->m_pkBuffData);
    if (pkBuffData) {
        NiGeometryGroup::RemoveObject(pkBuffData);
        delete pkBuffData;
        apData->m_pkBuffData = nullptr;
    }
}

NiVBChip* NiStaticGeometryGroup::CreateChip(NiGeometryBufferData* pkBuffData, uint32_t uiStream) {
    NiVBSet* pkSet_2;
    uint32_t pkBuffDataa;
    uint32_t uiIndex = pkBuffData->m_uiFVF;
    NiVBSet* pkSet = nullptr;
    if (uiStream >= pkBuffData->m_uiStreamCount) {
        pkBuffDataa = 0;
    }
    else {
        pkBuffDataa = pkBuffData->m_puiVertexStride[uiStream];
    }
    if (!uiIndex) {
        uiIndex = pkBuffDataa | INDEX_MASK_STRIDE;
    }
    uint32_t uiFlags = 0;
    if (pkBuffData->m_bSoftwareVP) {
        uiIndex |= INDEX_MASK_SOFTWAREVP;
        uiFlags = NiVBSet::NIVBSET_FLAG_SOFTWAREVP;
    }
    NiTPointerMap<uint32_t, NiVBSet*>* p_m_kSets = &m_kSets;
    if (!m_kSets.GetAt(uiIndex, pkSet) || !pkSet) {
        pkSet_2 = NiVBSet::Create(NiVBSet::NIVBSET_DEF_BLOCKSIZE, m_pkD3DDevice, uiIndex, uiFlags, 0);
        p_m_kSets->SetAt(uiIndex, pkSet_2);
    }
    else {
        pkSet_2 = pkSet;
    }
    NiVBChip* pkChip = pkSet_2->AllocateChip(
        pkBuffDataa * pkBuffData->m_uiMaxVertCount,
        pkBuffData->m_uiStreamCount > 1
    );
    pkBuffData->m_uiBaseVertexIndex = pkChip->GetOffset() / pkBuffDataa;
    return pkChip;
}

// Frees the chip in its block's set when it has a vertex buffer; the stream index is re-tested before clearing the
// slot.
void NiStaticGeometryGroup::ReleaseChip(NiGeometryBufferData* pkBuffData, uint32_t uiStream) {
    if (uiStream < pkBuffData->m_uiStreamCount) {
        NiVBChip* pkChip = pkBuffData->m_ppkVBChip[uiStream];
        if (pkChip) {
            if (pkChip->m_pkVB) {
                NiVBBlock* pkBlock = pkChip->m_pkBlock;
                pkBlock->m_pkSet->FreeChip(pkBlock->m_uiIndex, pkChip->m_uiIndex);
            }
            if (uiStream < pkBuffData->m_uiStreamCount) {
                pkBuffData->m_ppkVBChip[uiStream] = nullptr;
            }
        }
    }
}

// Static geometry is not dynamic.
bool NiStaticGeometryGroup::IsDynamic() {
    return false;
}

// Purges the sets; the set map empties and frees itself.
NiStaticGeometryGroup::~NiStaticGeometryGroup() {
    NiStaticGeometryGroup::Purge();
}

NiGeometryGroup* NiStaticGeometryGroup::Create() {
    return new NiStaticGeometryGroup;
}

// Tests the allocation result for null.
NiVBSet* NiVBSet::Create(
    uint32_t uiBlockSize,
    IDirect3DDevice9* pkD3DDevice,
    uint32_t uiFVF,
    uint32_t uiFlags,
    uint32_t uiIndex
) {
    NiVBSet* pkSet = new NiVBSet;
    if (!pkSet) {
        return nullptr;
    }
    pkSet->m_uiBlockSize = uiBlockSize;
    if (pkD3DDevice) {
        pkSet->m_pkD3DDevice = pkD3DDevice;
        pkD3DDevice->AddRef();
    }
    else {
        if (pkSet->m_pkD3DDevice) {
            pkSet->m_pkD3DDevice->Release();
        }
        pkSet->m_pkD3DDevice = nullptr;
    }
    pkSet->m_uiFVF = uiFVF;
    pkSet->m_uiFlags = uiFlags;
    pkSet->m_uiIndex = uiIndex;
    SetPoolAndUsage(pkSet);
    return pkSet;
}
