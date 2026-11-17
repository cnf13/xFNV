#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiDynamicGeometryGroup.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBDynamicSet.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBSet.h"
#include "SDK/MSVC/string.h"

// Both maps have 37 buckets.
NiDynamicGeometryGroup::NiDynamicGeometryGroup()
  : m_kSets(37), m_kChips(37), m_kMultiStreamSets(0, 1), m_kMultiStreamChips(0, 1), m_uiNextIndex(0) {}

// Deletes the sets and chips and clears the shared group pointer when it is this group; the maps and arrays free
// their own storage.
NiDynamicGeometryGroup::~NiDynamicGeometryGroup() {
    NiTMapItem<uint32_t, NiVBDynamicSet*>* kPos = m_kSets.GetFirstPos();
    while (kPos) {
        uint32_t uiIndex;
        NiVBDynamicSet* pkSet;
        m_kSets.GetNext(kPos, uiIndex, pkSet);
        m_kSets.RemoveAt(uiIndex);
        delete pkSet;
    }

    NiTMapItem<uint32_t, NiVBChip*>* kChipPos = m_kChips.GetFirstPos();
    while (kChipPos) {
        uint32_t uiIndex;
        NiVBChip* pkChip;
        m_kChips.GetNext(kChipPos, uiIndex, pkChip);
        m_kChips.RemoveAt(uiIndex);
        pkChip->m_pkVB = nullptr;
        delete pkChip;
    }

    for (uint32_t i = 0; i < m_kMultiStreamSets.GetAllocatedSize(); ++i) {
        delete m_kMultiStreamSets.RemoveAt(i);
        NiVBChip* pkChip = m_kMultiStreamChips.RemoveAt(i);
        pkChip->m_pkVB = nullptr;
        delete pkChip;
    }
    if (this == ms_pkGroup) {
        ms_pkGroup = nullptr;
    }
}

// Gives a screen texture without buffer data a new dynamic one and dirties all of its data.
void NiDynamicGeometryGroup::AddObject(NiScreenTexture* pkScreenTexture) {
    if (!pkScreenTexture->m_pkBuffData) {
        void* pMemory = MemoryManager_Singleton.Allocate(sizeof(NiGeometryBufferData));
        NiGeometryBufferData* pkBuffData = pMemory ? ::new (pMemory) NiGeometryBufferData : nullptr;
        pkBuffData->m_eType = D3DPT_TRIANGLELIST;
        pkScreenTexture->m_pkBuffData = pkBuffData;
        pkBuffData->m_uiFlags = 0x1400000;
        NiGeometryGroup::AddObject(pkBuffData);
        pkScreenTexture->m_usDirtyFlags = 0xFFFF;
    }
}

// Deletes every dynamic set; the chips stay, with no vertex buffer.
void NiDynamicGeometryGroup::Purge() {
    NiTMapItem<uint32_t, NiVBDynamicSet*>* kPos = m_kSets.GetFirstPos();
    while (kPos) {
        uint32_t uiIndex;
        NiVBDynamicSet* pkSet;
        m_kSets.GetNext(kPos, uiIndex, pkSet);
        m_kSets.RemoveAt(uiIndex);
        if (pkSet) {
            pkSet->~NiVBDynamicSet();
            NiMemObject::operator delete(pkSet, sizeof(NiVBDynamicSet));
        }
    }

    NiTMapItem<uint32_t, NiVBChip*>* kChipPos = m_kChips.GetFirstPos();
    while (kChipPos) {
        uint32_t uiIndex;
        NiVBChip* pkChip;
        m_kChips.GetNext(kChipPos, uiIndex, pkChip);
        pkChip->m_pkVB = nullptr;
    }

    for (uint32_t i = 0; i < m_kMultiStreamSets.GetAllocatedSize(); i++) {
        NiVBDynamicSet* pkSet = m_kMultiStreamSets.RemoveAt(i);
        if (pkSet) {
            pkSet->~NiVBDynamicSet();
            NiMemObject::operator delete(pkSet, sizeof(NiVBDynamicSet));
        }
        m_kMultiStreamChips.GetAt(i)->m_pkVB = nullptr;
    }
}

void NiDynamicGeometryGroup::AddObject(
    NiGeometryData* apData,
    NiSkinInstance* apSkinInstance,
    NiSkinPartition::Partition* apPartition
) {
    NiGeometryBufferData* pkBuffData;
    char pkPartition;
    if (apPartition) {
        if (apPartition->m_pkBuffData) {
            return;
        }
        NiGeometryBufferData* v4 =
            static_cast<NiGeometryBufferData*>(MemoryManager_Singleton.Allocate(sizeof(NiGeometryBufferData)));
        if (v4) {
            pkBuffData = ::new (v4) NiGeometryBufferData;
        }
        else {
            pkBuffData = nullptr;
        }
        pkBuffData->m_eType = static_cast<D3DPRIMITIVETYPE>((apPartition->m_usStrips != 0) + D3DPT_TRIANGLELIST);
        apPartition->m_pkBuffData = pkBuffData;
    }
    else {
        if (apData->m_pkBuffData) {
            return;
        }
        NiGeometryBufferData* v6 =
            static_cast<NiGeometryBufferData*>(MemoryManager_Singleton.Allocate(sizeof(NiGeometryBufferData)));
        if (v6) {
            pkBuffData = ::new (v6) NiGeometryBufferData;
        }
        else {
            pkBuffData = nullptr;
        }
        const NiRTTI* v7 = apData->GetRTTI();
        if (v7) {
            while (v7 != &NiTriShapeData::ms_RTTI) {
                v7 = v7->GetBaseRTTI();
                if (!v7) {
                    goto LABEL_14;
                }
            }
            pkBuffData->m_eType = D3DPT_TRIANGLELIST;
        }
        else {
        LABEL_14:;
            if (NiObject::IsKindOf_0(&NiTriStripsData::ms_RTTI, apData)) {
                pkBuffData->m_eType = D3DPT_TRIANGLESTRIP;
            }
        }
        apData->m_pkBuffData = pkBuffData;
    }
    if (!apData->m_pkNormal || (pkPartition = 1, (apData->m_usDataFlags & NiGeometryData::NBT_METHOD_MASK) == 0)) {
        pkPartition = 0;
    }
    uint32_t v8 = apData->m_usDataFlags & NiGeometryData::VERTEX_MASK;
    if (v8 > NiD3DPass::ms_uiMaxTextureBlendStages) {
        v8 = NiD3DPass::ms_uiMaxTextureBlendStages;
    }
    int32_t v9 = 0;
    if (apData->m_pkColor) {
        v9 = 0x400000;
    }
    if (apData->m_pkNormal) {
        v9 |= 0x800000u;
    }
    if (pkPartition) {
        v9 |= 0x200000u;
    }
    pkBuffData->m_uiFlags = v9 | v8 << 24;
    NiGeometryGroup::AddObject(pkBuffData);
    apData->MarkAsChanged(0xFFFF);
}

void NiDynamicGeometryGroup::RemoveObject(NiScreenTexture* apScreenTexture) {
    NiGeometryBufferData* pkBuffData = apScreenTexture->m_pkBuffData;
    if (pkBuffData) {
        NiGeometryGroup::RemoveObject(apScreenTexture->m_pkBuffData);
        NiVBChip** m_ppkVBChip = pkBuffData->m_ppkVBChip;
        if (m_ppkVBChip) {
            std::memset(m_ppkVBChip, 0, sizeof(NiVBChip*) * pkBuffData->m_uiStreamCount);
        }
        delete pkBuffData;
        apScreenTexture->m_pkBuffData = nullptr;
    }
}

void NiDynamicGeometryGroup::RemoveObject(NiSkinPartition::Partition* apPartition) {
    NiGeometryBufferData* m_pkBuffData = apPartition->m_pkBuffData;
    if (m_pkBuffData) {
        NiGeometryGroup::RemoveObject(apPartition->m_pkBuffData);
        NiVBChip** m_ppkVBChip = m_pkBuffData->m_ppkVBChip;
        if (m_ppkVBChip) {
            std::memset(m_ppkVBChip, 0, sizeof(NiVBChip*) * m_pkBuffData->m_uiStreamCount);
        }
        delete m_pkBuffData;
        apPartition->m_pkBuffData = nullptr;
    }
}

// Takes the data's buffer data out of the group, clears its chip pointers without freeing the chips and deletes it.
void NiDynamicGeometryGroup::RemoveObject(NiGeometryData* apData) {
    NiGeometryBufferData* pkBuffData = static_cast<NiGeometryBufferData*>(apData->m_pkBuffData);
    if (pkBuffData) {
        NiGeometryGroup::RemoveObject(pkBuffData);
        if (pkBuffData->m_ppkVBChip) {
            std::memset(pkBuffData->m_ppkVBChip, 0, pkBuffData->m_uiStreamCount * sizeof(NiVBChip*));
        }
        delete pkBuffData;
        apData->m_pkBuffData = nullptr;
    }
}

// Single-stream data shares one dynamic set and chip per vertex format (per stride, flagged 0x80000000, without
// one); multi-stream data takes the next set and chip of a ring that grows to the stream count.
NiVBChip* NiDynamicGeometryGroup::CreateChip(NiGeometryBufferData* pkBuffData, uint32_t uiStream) {
    uint32_t uiNumStreams = pkBuffData->m_uiStreamCount;
    uint32_t uiFVF = pkBuffData->m_uiFVF;
    NiVBChip* pkChip = nullptr;
    uint32_t uiVertexStride;
    if (uiStream >= uiNumStreams) {
        uiVertexStride = 0;
    }
    else {
        uiVertexStride = pkBuffData->m_puiVertexStride[uiStream];
    }
    uint32_t uiSize = uiVertexStride * pkBuffData->m_uiMaxVertCount;
    uint32_t uiOffset = 0;
    uint32_t uiFlags = 0;
    if (uiNumStreams <= 1) {
        NiVBDynamicSet* pkSet = nullptr;
        uint32_t uiIndex;
        if (uiFVF) {
            uiIndex = uiFVF;
        }
        else {
            uiIndex = uiVertexStride | 0x80000000u;
        }
        if (!m_kSets.GetAt(uiIndex, pkSet) || !pkSet) {
            pkSet = NiVBDynamicSet::Create(2048u, m_pkD3DDevice, uiFVF, 0);
            m_kSets.SetAt(uiIndex, pkSet);
        }
        IDirect3DVertexBuffer9* pkVB =
            pkSet->AllocateBufferSpace(uiSize, uiOffset, uiFlags, false, pkBuffData->m_bSoftwareVP);
        m_kChips.GetAt(uiIndex, pkChip);
        if (pkChip) {
            pkChip->m_pkVB = pkVB;
            pkChip->m_uiOffset = uiOffset;
            pkChip->m_uiSize = uiSize;
        }
        else {
            pkChip = NiVBChip::Create(nullptr, pkVB, uiOffset, uiSize, 0);
            m_kChips.SetAt(uiIndex, pkChip);
        }
        pkChip->m_pkDynamicSet = pkSet;
        pkChip->m_uiLockFlags = uiFlags;
        pkBuffData->m_uiBaseVertexIndex = uiOffset / uiVertexStride;
        return pkChip;
    }
    else {
        if (m_kMultiStreamSets.m_usMaxSize < uiNumStreams) {
            m_kMultiStreamSets.SetSize(uiNumStreams);
            m_kMultiStreamChips.SetSize(uiNumStreams);
        }
        if (m_uiNextIndex >= m_kMultiStreamSets.m_usMaxSize) {
            m_uiNextIndex = 0;
        }
        NiVBDynamicSet* pkSet = m_kMultiStreamSets.m_pBase[m_uiNextIndex];
        if (!pkSet) {
            pkSet = NiVBDynamicSet::Create(2048u, m_pkD3DDevice, 0, 0);
            m_kMultiStreamSets.SetAt(m_uiNextIndex, pkSet);
        }
        IDirect3DVertexBuffer9* pkVB =
            pkSet->AllocateBufferSpace(uiSize, uiOffset, uiFlags, true, pkBuffData->m_bSoftwareVP);
        NiVBChip* pkMultiChip = m_kMultiStreamChips.m_pBase[m_uiNextIndex];
        if (pkMultiChip) {
            pkMultiChip->m_pkVB = pkVB;
            pkMultiChip->m_uiOffset = uiOffset;
            pkMultiChip->m_uiSize = uiSize;
        }
        else {
            pkMultiChip = NiVBChip::Create(nullptr, pkVB, uiOffset, uiSize, 0);
            pkChip = pkMultiChip;
            m_kMultiStreamChips.SetAt(m_uiNextIndex, pkChip);
        }
        pkMultiChip->m_pkDynamicSet = pkSet;
        pkMultiChip->m_uiLockFlags = uiFlags;
        pkBuffData->m_uiBaseVertexIndex = 0;
        ++m_uiNextIndex;
        return pkMultiChip;
    }
}

void NiDynamicGeometryGroup::ReleaseChip(NiGeometryBufferData* pkBuffData, uint32_t uiStream) {
    if (uiStream < pkBuffData->m_uiStreamCount) {
        NiVBChip* v3 = pkBuffData->m_ppkVBChip[uiStream];
        if (v3) {
            if (v3->GetVB()) {
                v3->m_pkVB = nullptr;
            }
            if (uiStream < pkBuffData->m_uiStreamCount) {
                pkBuffData->m_ppkVBChip[uiStream] = nullptr;
            }
        }
    }
}

bool NiDynamicGeometryGroup::IsDynamic() {
    return true;
}

NiGeometryGroup* NiDynamicGeometryGroup::Create() {
    if (!ms_pkGroup) {
        void* pMemory = NiMemObject::operator new(sizeof(NiDynamicGeometryGroup));
        ms_pkGroup = pMemory ? ::new (pMemory) NiDynamicGeometryGroup : nullptr;
    }
    return ms_pkGroup;
}

NiVBDynamicSet*
NiVBDynamicSet::Create(uint32_t uiBlockSize, IDirect3DDevice9* pkD3DDevice, uint32_t uiFVF, uint32_t uiIndex) {
    NiVBDynamicSet* pkSet = static_cast<NiVBDynamicSet*>(NiMemObject::operator new(sizeof(NiVBDynamicSet)));
    if (!pkSet) {
        return nullptr;
    }
    uint32_t v5 = uiBlockSize;
    pkSet->m_uiIndex = 0;
    pkSet->m_uiFVF = 0;
    pkSet->m_uiBlockSize = 0;
    pkSet->m_pkD3DDevice = nullptr;
    pkSet->m_pkVB = nullptr;
    if ((uiBlockSize & 15) != 0) {
        v5 = (uiBlockSize & 0xFFFFFFF0u) + 32;
    }
    pkSet->m_uiBlockSize = v5;
    IDirect3DDevice9* m_pkD3DDevice = pkSet->m_pkD3DDevice;
    if (m_pkD3DDevice) {
        pkSet->m_pkD3DDevice->Release();
    }
    if (pkD3DDevice) {
        pkSet->m_pkD3DDevice = pkD3DDevice;
        pkD3DDevice->AddRef();
    }
    else {
        pkSet->m_pkD3DDevice = nullptr;
    }
    pkSet->m_uiFVF = uiFVF;
    pkSet->m_uiIndex = uiIndex;
    return pkSet;
}
