#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstdlib>
#include <cstring>

NiRTTI NiSkinInstance::ms_RTTI("NiSkinInstance", &NiObject::ms_RTTI);

namespace {

    // Orders bone names for the viewer; a null entry sorts after the other.
    int CompareBoneNames(const void* pvName0, const void* pvName1) {
        if (!pvName0) {
            return 1;
        }
        if (!pvName1) {
            return -1;
        }
        return strcmp(*static_cast<const char* const*>(pvName0), *static_cast<const char* const*>(pvName1));
    }

} // namespace

NiSkinInstance::NiSkinInstance()
  : m_spSkinData(nullptr), m_spSkinPartition(nullptr), m_pkRootParent(nullptr), m_ppkBones(nullptr),
    m_uiFrameID(0xFFFFFFFF), m_uiNumMatrices(0), m_uiNumRegisters(0), m_uiAllocatedSize(0), m_pvBoneMatrices(nullptr),
    m_pvSkinToWorldWorldToSkinMatrix(nullptr) {}

NiSkinInstance::NiSkinInstance(NiSkinData* apSkinData, NiAVObject* apRootParent, NiAVObject** appBones)
  : m_spSkinData(apSkinData), m_spSkinPartition(nullptr), m_pkRootParent(apRootParent), m_ppkBones(appBones),
    m_uiFrameID(0xFFFFFFFF), m_uiNumMatrices(0), m_uiNumRegisters(0), m_uiAllocatedSize(0), m_pvBoneMatrices(nullptr),
    m_pvSkinToWorldWorldToSkinMatrix(nullptr) {}

// The renderer drops whatever it built for this instance; the smart pointers release the partition and skin data.
NiSkinInstance::~NiSkinInstance() {
    _NiFree(m_ppkBones);
    if (NiRenderer::ms_pkRenderer) {
        NiRenderer::ms_pkRenderer->PurgeSkinInstance(this);
    }
}

// Only an instance whose root parent or a bone is being cloned gets a clone; the others are shared. The clone keeps the
// original's root and bones where those are not cloned. Goes on with a null clone when the allocation fails.
NiObject* NiSkinInstance::CreateClone(NiCloningProcess& kCloning) {
    if (!NeedClone(kCloning)) {
        return this;
    }

    // The default constructor, as in CreateObject.
    NiSkinInstance* pkClone = new NiSkinInstance;
    CopyMembers(pkClone, kCloning);

    NiObject* pkClonedObject = nullptr;
    if (kCloning.m_pkCloneMap->GetAt(m_pkRootParent, pkClonedObject)) {
        pkClone->m_pkRootParent = static_cast<NiAVObject*>(pkClonedObject);
    }
    else {
        pkClone->m_pkRootParent = m_pkRootParent;
    }

    uint32_t uiBones = m_spSkinData->m_uiBones;
    pkClone->m_ppkBones = static_cast<NiAVObject**>(_NiMalloc(sizeof(NiAVObject*) * uiBones));
    for (uint32_t i = 0; i < uiBones; i++) {
        if (kCloning.m_pkCloneMap->GetAt(m_ppkBones[i], pkClonedObject)) {
            pkClone->m_ppkBones[i] = static_cast<NiAVObject*>(pkClonedObject);
        }
        else {
            pkClone->m_ppkBones[i] = m_ppkBones[i];
        }
    }

    return pkClone;
}

void NiSkinInstance::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);

    // Skin data, skin partition (10.1.0.101 on; older files keep it in the skin data), root parent, bones.
    kStream.ReadLinkID();
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 101)) {
        kStream.ReadLinkID();
    }
    kStream.ReadLinkID();
    kStream.ReadMultipleLinkIDs();
}

void NiSkinInstance::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);

    m_spSkinData = static_cast<NiSkinData*>(kStream.GetObjectFromLinkID());
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 101)) {
        m_spSkinPartition = static_cast<NiSkinPartition*>(kStream.GetObjectFromLinkID());
    }
    m_pkRootParent = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());

    uint32_t uiBones = kStream.GetNumberOfLinkIDs();
    m_ppkBones = static_cast<NiAVObject**>(_NiMalloc(sizeof(NiAVObject*) * uiBones));
    for (uint32_t i = 0; i < uiBones; i++) {
        m_ppkBones[i] = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
    }
}

bool NiSkinInstance::RegisterStreamables(NiStream& kStream) {
    if (!NiObject::RegisterStreamables(kStream)) {
        return false;
    }

    m_spSkinData->RegisterStreamables(kStream);
    if (m_spSkinPartition) {
        m_spSkinPartition->RegisterStreamables(kStream);
    }
    m_pkRootParent->RegisterStreamables(kStream);

    uint32_t uiBones = m_spSkinData->m_uiBones;
    for (uint32_t i = 0; i < uiBones; i++) {
        m_ppkBones[i]->RegisterStreamables(kStream);
    }

    return true;
}

void NiSkinInstance::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);

    kStream.SaveLinkID(m_spSkinData);
    kStream.SaveLinkID(m_spSkinPartition);
    kStream.SaveLinkID(m_pkRootParent);

    uint32_t uiBones = m_spSkinData->m_uiBones;
    NiStreamSaveBinary(kStream, uiBones);
    for (uint32_t i = 0; i < uiBones; i++) {
        kStream.SaveLinkID(m_ppkBones[i]);
    }
}

bool NiSkinInstance::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiSkinInstance* pkSkinInstance = static_cast<NiSkinInstance*>(pkObject);

    if (!m_spSkinData->IsEqual(pkSkinInstance->m_spSkinData)) {
        return false;
    }

    if (m_spSkinPartition && !m_spSkinPartition->IsEqual(pkSkinInstance->m_spSkinPartition)) {
        return false;
    }

    if (!m_spSkinPartition && pkSkinInstance->m_spSkinPartition) {
        return false;
    }

    return true;
}

void NiSkinInstance::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(ms_RTTI.m_pcName));

    if (m_pkRootParent) {
        pkStrings->Add(NiGetViewerString("Root Parent", static_cast<const char*>(m_pkRootParent->m_kName)));
    }

    if (m_spSkinData) {
        pkStrings->Add(
            NiGetViewerString("Hardware partitions", m_spSkinPartition ? m_spSkinPartition->m_uiPartitions : 0u)
        );

        uint32_t uiBones = m_spSkinData->m_uiBones;
        pkStrings->Add(NiGetViewerString("Bone Count", uiBones));

        // The bones by name (with their address), sorted.
        char** ppcNames = static_cast<char**>(_NiMalloc(sizeof(char*) * uiBones));
        for (uint32_t i = 0; i < uiBones; i++) {
            char acName[1024];
            std::sprintf(acName, "%s (%08X)", static_cast<const char*>(m_ppkBones[i]->m_kName), m_ppkBones[i]);
            char* pcCopy = static_cast<char*>(_NiMalloc(std::strlen(acName) + 1));
            ppcNames[i] = pcCopy;
            std::strcpy(pcCopy, acName);
        }

        std::qsort(ppcNames, uiBones, sizeof(char*), &CompareBoneNames);

        for (uint32_t i = 0; i < uiBones; i++) {
            char acPrefix[10];
            NiSprintf(acPrefix, sizeof(acPrefix), "  %3d", i);
            pkStrings->Add(NiGetViewerString(acPrefix, ppcNames[i] ? ppcNames[i] : "<noname>"));
        }

        for (uint32_t i = 0; i < uiBones; i++) {
            _NiFree(ppcNames[i]);
        }
        _NiFree(ppcNames);

        m_spSkinPartition->GetViewerStrings(pkStrings);
    }
}

void NiSkinInstance::PostLinkObject(NiStream& kStream) {
    // Older files keep the partition in the skin data; the instance takes it over.
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 101) && m_spSkinData) {
        m_spSkinPartition = m_spSkinData->m_spSkinPartition.m_pObject;
        m_spSkinData->m_spSkinPartition = nullptr;
    }
}

void NiSkinInstance::Deform(
    const NiGeometryData* apModelData,
    NiPoint3* apDstVertex,
    NiPoint3* apDstNormal,
    NiPoint3* apDstBinormal,
    NiPoint3* apDstTangent,
    uint32_t auiStride
) {
    uint32_t fatigue = apModelData->NiGeometryData::GetVertexCount();
    NiPoint3* unk24 = apModelData->GetNormals();
    NiPoint3* objectReference = apModelData->NiGeometryData::GetVertices();
    Deform(objectReference, unk24, fatigue, apDstVertex, apDstNormal, apDstBinormal, apDstTangent, auiStride);
}

void NiSkinInstance::CopyMembers(NiSkinInstance* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);

    pkDest->m_spSkinData = m_spSkinData;
    pkDest->m_spSkinPartition = m_spSkinPartition;
}

// Whether the root parent or any bone is in the clone map, that is, is being cloned.
bool NiSkinInstance::NeedClone(NiCloningProcess& kCloning) {
    NiObject* pkClonedObject;
    if (kCloning.m_pkCloneMap->GetAt(m_pkRootParent, pkClonedObject)) {
        return true;
    }

    uint32_t uiBones = m_spSkinData->m_uiBones;
    for (uint32_t i = 0; i < uiBones; i++) {
        if (kCloning.m_pkCloneMap->GetAt(m_ppkBones[i], pkClonedObject)) {
            return true;
        }
    }
    return false;
}

// NiSkinInstance::UpdateModelBound is in Xenon/NiSkinInstance_Xenon.cpp. The default constructor: no skin data,
// partition, root or bones; no bone matrices yet (frame ID -1).
NiObject* NiSkinInstance::CreateObject() {
    return new NiSkinInstance;
}
