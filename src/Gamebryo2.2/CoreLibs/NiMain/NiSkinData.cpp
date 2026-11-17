#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMessageBox.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"

NiRTTI NiSkinData::ms_RTTI("NiSkinData", &NiObject::ms_RTTI);

NiSkinData::BoneData::BoneData() : m_pkBoneVertData(nullptr), m_usVerts(0) {}

NiSkinData::NiSkinData() : m_spSkinPartition(nullptr), m_pkBoneData(nullptr) {}

NiSkinData::NiSkinData(
    uint32_t uiBones,
    NiSkinData::BoneData* pkBoneData,
    const NiTransform& kRootParentToSkin,
    const NiPoint3* pkVerts
)
  : m_spSkinPartition(nullptr), m_kRootParentToSkin(kRootParentToSkin), m_pkBoneData(pkBoneData), m_uiBones(uiBones) {
    ComputeBoneBounds(pkVerts);
}

// The bone array is the engine's delete[] of an element type without a destructor: a plain MemoryManager free, null or
// not.
NiSkinData::~NiSkinData() {
    FreeBoneVertData();
    MemoryManager_Singleton.Deallocate(m_pkBoneData);
}

void NiSkinData::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);

    m_kRootParentToSkin.LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_uiBones);
    uint32_t uiCount = m_uiBones;
    uint64_t ullSize = uint64_t{sizeof(BoneData)} * uiCount;
    size_t stSize = ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize);
    void* pvMemory = MemoryManager_Singleton.Allocate(stSize);
    BoneData* pkBoneData = nullptr;
    if (pvMemory) {
        rt::eh_vector_constructor_iterator(
            pvMemory, sizeof(BoneData), static_cast<int32_t>(uiCount), &fnvd::constructElement<NiSkinData::BoneData>
        );
        pkBoneData = static_cast<BoneData*>(pvMemory);
    }
    m_pkBoneData = pkBoneData;

    // The skin partition link, read here and resolved in LinkObject.
    if (kStream.GetFileVersion() >= NiStream::GetVersion(4, 0, 0, 1) &&
        kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 101)) {
        kStream.ReadLinkID();
    }

    // Files before 4.1.0.13 always carry the per-vertex weights.
    bool bHasWeights;
    if (kStream.GetFileVersion() < NiStream::GetVersion(4, 1, 0, 13)) {
        bHasWeights = true;
    }
    else {
        NiStreamLoadBinary(kStream, bHasWeights);
    }

    // Bethesda's check for skins exported before user version 11; the rat mesh is a known exception.
    if (kStream.m_uiNifFileUserDefinedVersion < 11 && kStream.m_uiLastError == NiStream::STREAM_OKAY && m_uiBones &&
        NiStricmp(kStream.m_acFileName, "Meshes\\Creatures\\Rat\\mange.NIF")) {
        kStream.m_uiLastError = NiStream::OLDER_VERSION;

        char acText[512];
        if (kStream.m_BSStreamHeader.cAuthor[0]) {
            std::sprintf(
                acText, "%s: %s contains old skin info and needs to be re-exported.", kStream.m_BSStreamHeader.cAuthor,
                kStream.m_acFileName
            );
        }
        else {
            std::sprintf(acText, "%s contains old skin info and needs to be re-exported.", kStream.m_acFileName);
        }

        if (NiMessageBox::ms_pfnMessageBox) {
            NiMessageBox::ms_pfnMessageBox(acText, "Old Version", nullptr);
        }
    }

    for (uint32_t i = 0; i < m_uiBones; i++) {
        BoneData* pkBoneData = &m_pkBoneData[i];
        pkBoneData->m_kSkinToBone.LoadBinary(kStream);
        pkBoneData->m_kBound.LoadBinary(kStream);
        NiStreamLoadBinary(kStream, pkBoneData->m_usVerts);

        if (bHasWeights) {
            pkBoneData->m_pkBoneVertData = NiTNewInterface<BoneVertData>::Allocate(pkBoneData->m_usVerts);
            for (uint32_t j = 0; j < pkBoneData->m_usVerts; j++) {
                NiStreamLoadBinary(kStream, pkBoneData->m_pkBoneVertData[j].m_usVert);
                NiStreamLoadBinary(kStream, pkBoneData->m_pkBoneVertData[j].m_fWeight);
            }
        }
        else {
            pkBoneData->m_pkBoneVertData = nullptr;
        }
    }

    // 4.1.0.9 files end with a byte that is no longer used.
    if (kStream.GetFileVersion() == NiStream::GetVersion(4, 1, 0, 9)) {
        bool bUnused;
        NiStreamLoadBinary(kStream, bUnused);
    }
}

void NiSkinData::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);

    if (kStream.GetFileVersion() >= NiStream::GetVersion(4, 0, 0, 1) &&
        kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 101)) {
        m_spSkinPartition = static_cast<NiSkinPartition*>(kStream.GetObjectFromLinkID());
    }
}

bool NiSkinData::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

void NiSkinData::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);

    m_kRootParentToSkin.SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uiBones);

    // The first bone decides for all whether weights are stored.
    bool bHasWeights = m_pkBoneData[0].m_pkBoneVertData != nullptr;
    NiStreamSaveBinary(kStream, bHasWeights);

    for (uint32_t i = 0; i < m_uiBones; i++) {
        BoneData* pkBoneData = &m_pkBoneData[i];
        pkBoneData->m_kSkinToBone.SaveBinary(kStream);
        pkBoneData->m_kBound.SaveBinary(kStream);
        NiStreamSaveBinary(kStream, pkBoneData->m_usVerts);

        if (bHasWeights) {
            for (uint32_t j = 0; j < pkBoneData->m_usVerts; j++) {
                NiStreamSaveBinary(kStream, pkBoneData->m_pkBoneVertData[j].m_usVert);
                NiStreamSaveBinary(kStream, pkBoneData->m_pkBoneVertData[j].m_fWeight);
            }
        }
    }
}

bool NiSkinData::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiSkinData* pkSkinData = static_cast<NiSkinData*>(pkObject);

    if (m_kRootParentToSkin != pkSkinData->m_kRootParentToSkin) {
        return false;
    }

    if (m_uiBones != pkSkinData->m_uiBones) {
        return false;
    }

    for (uint32_t i = 0; i < m_uiBones; i++) {
        BoneData* pkBoneData = &m_pkBoneData[i];
        BoneData* pkOtherData = &pkSkinData->m_pkBoneData[i];

        if (pkBoneData->m_kSkinToBone != pkOtherData->m_kSkinToBone) {
            return false;
        }

        if (pkBoneData->m_kBound != pkOtherData->m_kBound) {
            return false;
        }

        if (pkBoneData->m_usVerts != pkOtherData->m_usVerts) {
            return false;
        }

        for (uint32_t j = 0; j < pkBoneData->m_usVerts; j++) {
            if (pkBoneData->m_pkBoneVertData[j].m_usVert != pkOtherData->m_pkBoneVertData[j].m_usVert ||
                pkBoneData->m_pkBoneVertData[j].m_fWeight != pkOtherData->m_pkBoneVertData[j].m_fWeight) {
                return false;
            }
        }
    }

    return true;
}

void NiSkinData::FreeBoneVertData() {
    if (!m_pkBoneData) {
        return;
    }

    for (uint32_t i = 0; i < m_uiBones; i++) {
        MemoryManager_Singleton.Deallocate(m_pkBoneData[i].m_pkBoneVertData);
        m_pkBoneData[i].m_pkBoneVertData = nullptr;
    }
}

void NiSkinData::ComputeBoneBounds(const NiPoint3* pkVerts) {
    using NiPoint3Set_t = NiTSet<NiPoint3, NiTNewInterface<NiPoint3>>;
    FNVD_INLINED_LOCAL(NiPoint3Set_t, v13, std::nullopt, std::nullopt);
    uint32_t m_uiUsed = 0;
    NiPoint3* m_pBase = nullptr;
    std::memset(&v13, 0, sizeof(v13));
    uint32_t v12 = 0;
    if (m_uiBones) {
        int32_t v5 = 0;
        do {
            NiSkinData::BoneData* m_pkBoneData = this->m_pkBoneData;
            uint32_t v7 = 0;
            if (m_pkBoneData[v5].m_usVerts) {
                uint32_t m_usVerts;
                do {
                    const NiPoint3* v8 = &pkVerts[m_pkBoneData[v5].m_pkBoneVertData[v7].m_usVert];
                    if (m_uiUsed == v13.m_uiAlloced) {
                        uint32_t v9 = 2 * v13.m_uiAlloced;
                        if (!v13.m_uiAlloced) {
                            v9 = 1;
                        }
                        v13.Realloc(v9);
                        m_uiUsed = v13.m_uiUsed;
                    }
                    NiPoint3* v10 = &v13.m_pBase[m_uiUsed];
                    v10->x = v8->x;
                    v10->y = v8->y;
                    v10->z = v8->z;
                    m_pkBoneData = this->m_pkBoneData;
                    m_usVerts = m_pkBoneData[v5].m_usVerts;
                    ++m_uiUsed;
                    ++v7;
                    v13.m_uiUsed = m_uiUsed;
                }
                while (v7 < m_usVerts);
                m_pBase = v13.m_pBase;
            }
            this->m_pkBoneData[v5].m_kBound.ComputeFromData(m_uiUsed, m_pBase);
            this->m_pkBoneData[v5].m_kBound.Update(
                this->m_pkBoneData[v5].m_kBound, this->m_pkBoneData[v5].m_kSkinToBone
            );
            m_uiUsed = 0;
            ++v5;
            v13.m_uiUsed = 0;
            ++v12;
        }
        while (v12 < m_uiBones);
    }
    MemoryManager_Singleton.Deallocate(m_pBase);
}

// The default constructor: no bones, no partition; the bone count and root transform are left as allocated.
NiObject* NiSkinData::CreateObject() {
    void* pkObject = NiMemObject::operator new(sizeof(NiSkinData));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiSkinData;
}
