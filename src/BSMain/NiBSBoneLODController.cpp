#include "BSMain/NiBSBoneLODController.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSStream.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStringExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

NiRTTI NiBSBoneLODController::ms_RTTI("NiBSBoneLODController", &NiTimeController::ms_RTTI);

namespace {

    // An empty bone set from the engine heap, built with a zero initial size, so nothing is allocated.
    NiTPrimitiveSet<NiNode*>* NewBoneSet() {
        NiTPrimitiveSet<NiNode*>* pkSet = static_cast<NiTPrimitiveSet<NiNode*>*>(
            MemoryManager::Instance().Allocate(sizeof(NiTPrimitiveSet<NiNode*>))
        );
        if (pkSet) {
            pkSet->m_pBase = nullptr;
            pkSet->m_uiAlloced = 0;
            pkSet->m_uiUsed = 0;
        }
        return pkSet;
    }

} // namespace

NiBSBoneLODController::NiBSBoneLODController() : m_iLOD(-1), m_uiNumLODs(0), m_kBoneArray(0, 1) {}

// The bone array's buffer is freed by the array's own destructor.
NiBSBoneLODController::~NiBSBoneLODController() {
    CleanUp();
}

NiObject* NiBSBoneLODController::CreateClone(NiCloningProcess& arCloneData) {
    NiBSBoneLODController* v3;
    void* v5 = NiMemObject::operator new(sizeof(NiBSBoneLODController));
    if (v5) {
        v3 = ::new (v5) NiBSBoneLODController;
    }
    else {
        v3 = nullptr;
    }
    CopyMembers(v3, arCloneData);
    return v3;
}

// Each bone slot is first filled with this controller as a placeholder; LinkObject replaces them.
void NiBSBoneLODController::LoadBinary(NiStream& kStream) {
    NiTimeController::LoadBinary(kStream);
    // The LOD is loaded through the 4-byte transfer NiStreamLoadBinary<uint32_t>, which moves four untyped bytes, so
    // the field is passed as that instance's argument.
    NiStreamLoadBinary<uint32_t>(kStream, reinterpret_cast<uint32_t&>(m_iLOD));
    NiStreamLoadBinary(kStream, m_uiNumLODs);

    uint32_t uiNumBoneSets = 0;
    NiStreamLoadBinary(kStream, uiNumBoneSets);
    for (uint32_t i = 0; i < uiNumBoneSets; i++) {
        NiTPrimitiveSet<NiNode*>* pkBones = NewBoneSet();
        m_kBoneArray.SetAtGrow(i, pkBones);

        uint32_t uiNumBones = 0;
        NiStreamLoadBinary(kStream, uiNumBones);
        for (uint32_t j = 0; j < uiNumBones; j++) {
            pkBones->Add(reinterpret_cast<NiNode*>(this));
            kStream.ReadLinkID();
        }
    }
}

void NiBSBoneLODController::LinkObject(NiStream& kStream) {
    NiTimeController::LinkObject(kStream);

    for (uint32_t i = 0; i < m_kBoneArray.GetSize(); i++) {
        NiTPrimitiveSet<NiNode*>* pkBones = m_kBoneArray.GetAt(i);
        if (!pkBones) {
            continue;
        }
        for (uint32_t j = 0; j < pkBones->GetSize(); j++) {
            pkBones->ReplaceAt(j, static_cast<NiNode*>(kStream.GetObjectFromLinkID()));
        }
    }
}

// The bones belong to the scene graph, so nothing beyond the base is registered.
bool NiBSBoneLODController::RegisterStreamables(NiStream& kStream) {
    return NiTimeController::RegisterStreamables(kStream);
}

// An empty bone slot is saved as a set of zero bones. The signed values go through NiAnimationKey::SaveBinary, which
// writes the four untyped bytes at the key's start: the value's address is passed as the key.
void NiBSBoneLODController::SaveBinary(NiStream& kStream) {
    NiTimeController::SaveBinary(kStream);
    NiAnimationKey::SaveBinary(kStream, reinterpret_cast<NiAnimationKey*>(&m_iLOD));
    NiStreamSaveBinary(kStream, m_uiNumLODs);

    uint32_t uiNumBoneSets = m_kBoneArray.GetSize();
    NiStreamSaveBinary(kStream, uiNumBoneSets);
    for (uint32_t i = 0; i < m_kBoneArray.GetSize(); i++) {
        NiTPrimitiveSet<NiNode*>* pkBones = m_kBoneArray.GetAt(i);
        if (pkBones) {
            uint32_t uiNumBones = pkBones->GetSize();
            NiStreamSaveBinary(kStream, uiNumBones);
            for (uint32_t j = 0; j < pkBones->GetSize(); j++) {
                kStream.SaveLinkID(pkBones->GetAt(j));
            }
        }
        else {
            int32_t iNumBones = 0;
            NiAnimationKey::SaveBinary(kStream, reinterpret_cast<NiAnimationKey*>(&iNumBones));
        }
    }
}

// Matching bone slots must both be empty or hold sets of equal size whose bones compare equal; bones are compared
// without null checks.
bool NiBSBoneLODController::IsEqual(NiObject* pkObject) {
    if (!NiTimeController::IsEqual(pkObject)) {
        return false;
    }

    NiBSBoneLODController* pkOther = static_cast<NiBSBoneLODController*>(pkObject);
    if (m_iLOD != pkOther->m_iLOD || m_uiNumLODs != pkOther->m_uiNumLODs ||
        m_kBoneArray.GetSize() != pkOther->m_kBoneArray.GetSize()) {
        return false;
    }

    for (uint32_t i = 0; i < m_kBoneArray.GetSize(); i++) {
        NiTPrimitiveSet<NiNode*>* pkBones = m_kBoneArray.GetAt(i);
        NiTPrimitiveSet<NiNode*>* pkOtherBones = pkOther->m_kBoneArray.GetAt(i);
        if ((pkBones && !pkOtherBones) || (!pkBones && pkOtherBones)) {
            return false;
        }
        if (!pkBones || !pkOtherBones) {
            continue;
        }
        if (pkBones->GetSize() != pkOtherBones->GetSize()) {
            return false;
        }
        for (uint32_t j = 0; j < pkBones->GetSize(); j++) {
            if (!pkBones->GetAt(j)->IsEqual(pkOtherBones->GetAt(j))) {
                return false;
            }
        }
    }
    return true;
}

// No base class strings.
void NiBSBoneLODController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_iLOD", m_iLOD));
    pkStrings->Add(NiGetViewerString("m_uiNumLODs", m_uiNumLODs));
}

// The clone gets its own bone sets holding the clones of the bones; a bone that was not cloned (or an empty bone
// slot) becomes null. The clone then re-applies its LOD.
void NiBSBoneLODController::ProcessClone(NiCloningProcess& arCloneData) {
    NiTimeController::ProcessClone(arCloneData);

    NiObject* pkCloneObject = nullptr;
    arCloneData.m_pkCloneMap->GetAt(this, pkCloneObject);
    NiBSBoneLODController* pkClone = static_cast<NiBSBoneLODController*>(pkCloneObject);

    for (uint32_t i = 0; i < m_kBoneArray.GetSize(); i++) {
        NiTPrimitiveSet<NiNode*>* pkBones = m_kBoneArray.GetAt(i);
        if (!pkBones) {
            pkClone->m_kBoneArray.SetAtGrow(i, nullptr);
            continue;
        }

        NiTPrimitiveSet<NiNode*>* pkCloneBones = NewBoneSet();
        pkClone->m_kBoneArray.SetAtGrow(i, pkCloneBones);
        for (uint32_t j = 0; j < pkBones->GetSize(); j++) {
            NiNode* pkBone = pkBones->GetAt(j);
            if (pkBone) {
                NiObject* pkCloneBone = nullptr;
                if (arCloneData.m_pkCloneMap->GetAt(pkBone, pkCloneBone)) {
                    pkCloneBones->Add(static_cast<NiNode*>(pkCloneBone));
                }
                else {
                    pkCloneBones->Add(nullptr);
                }
            }
            else {
                pkCloneBones->Add(nullptr);
            }
        }
    }

    int32_t iLOD = pkClone->m_iLOD;
    pkClone->m_iLOD = -1;
    pkClone->SetBoneLOD(iLOD);
}

// m_iLOD is reset so SetBoneLOD re-applies the loaded LOD.
void NiBSBoneLODController::PostLinkObject(NiStream& kStream) {
    NiTimeController::PostLinkObject(kStream);

    int32_t iLOD = m_iLOD;
    m_iLOD = -1;
    SetBoneLOD(iLOD);
}

bool NiBSBoneLODController::TargetIsRequiredType() const {
    return m_pkTarget && m_pkTarget->IsNode();
}

int32_t NiBSBoneLODController::GetBoneLOD() const {
    return m_iLOD;
}

// Bone sets from iLOD on get selective update and the rest lose it; empty bone slots are skipped. Unlike
// NiBoneLODController there are no skins to switch.
bool NiBSBoneLODController::SetBoneLOD(int32_t iLOD) {
    if (!m_uiNumLODs || m_iLOD == iLOD) {
        return false;
    }
    if (iLOD != -1 && (iLOD < 0 || static_cast<uint32_t>(iLOD) >= m_uiNumLODs)) {
        return false;
    }

    bool bOn = false;
    for (uint32_t i = 0; i < m_kBoneArray.GetSize(); i++) {
        NiTPrimitiveSet<NiNode*>* pkBones = m_kBoneArray.GetAt(i);
        if (!pkBones) {
            continue;
        }

        if (i == static_cast<uint32_t>(iLOD)) {
            bOn = true;
        }
        for (uint32_t j = 0; j < pkBones->GetSize(); j++) {
            NiNode* pkBone = pkBones->GetAt(j);
            if (!pkBone) {
                continue;
            }
            if (bOn) {
                pkBone->m_uFlags = NiAVObject::NiFlags(pkBone->m_uFlags | NiAVObject::SELECTIVE_UPDATE);
            }
            else {
                pkBone->m_uFlags = NiAVObject::NiFlags(pkBone->m_uFlags & ~NiAVObject::SELECTIVE_UPDATE);
            }
        }
    }

    m_iLOD = iLOD;
    return true;
}

void NiBSBoneLODController::CleanUp() {
    for (uint32_t i = 0; i < m_kBoneArray.m_usSize; ++i) {
        NiTPrimitiveSet<NiNode*>* Block = m_kBoneArray.m_pBase[i];
        if (Block) {
            _NiFree(Block->m_pBase);
            MemoryManager_Singleton.Deallocate(Block);
        }
    }
    for (uint16_t j = 0; j < static_cast<int32_t>(m_kBoneArray.m_usSize); ++j) {
        m_kBoneArray.m_pBase[j] = 0;
    }
    m_kBoneArray.m_usSize = 0;
    m_kBoneArray.m_usESize = 0;
}

// The bone array is not copied: ProcessClone rebuilds it.
void NiBSBoneLODController::CopyMembers(NiBSBoneLODController* pkDest, NiCloningProcess& arCloneData) {
    NiTimeController::CopyMembers(pkDest, arCloneData);
    pkDest->m_iLOD = m_iLOD;
    pkDest->m_uiNumLODs = m_uiNumLODs;
}

bool NiBSBoneLODController::InitializeData(NiNode* pkBoneRoot) {
    CleanUp();
    using kBoneToLOD_t = NiTPointerMap<NiNode*, uint32_t>;
    kBoneToLOD_t kBoneToLOD(37u);
    using kUsedLODs_t = NiTPointerMap<uint32_t, bool>;
    kUsedLODs_t kUsedLODs(37u);
    CollectMarkedBones(pkBoneRoot, kBoneToLOD, kUsedLODs);

    // The highest LOD any bone is marked with sets the LOD count.
    uint32_t uiMaxLOD = 0;
    NiTMapItem<uint32_t, bool>* kUsedIter = kUsedLODs.GetFirstPos();
    while (kUsedIter) {
        uint32_t uiLOD;
        bool bTemp;
        kUsedLODs.GetNext(kUsedIter, uiLOD, bTemp);
        if (uiLOD > uiMaxLOD) {
            uiMaxLOD = uiLOD;
        }
    }
    m_uiNumLODs = uiMaxLOD + 1;
    m_kBoneArray.SetSize(m_uiNumLODs);
    for (uint32_t uiLOD = 0; uiLOD < m_uiNumLODs; ++uiLOD) {
        NiTPrimitiveSet<NiNode*>* pkNoSet = nullptr;
        m_kBoneArray.SetAt(uiLOD, pkNoSet);
    }

    // Each marked bone goes into its LOD's set, created on first use.
    NiTMapItem<NiNode*, uint32_t>* kBoneIter = kBoneToLOD.GetFirstPos();
    while (kBoneIter) {
        NiNode* pkBone;
        uint32_t uiLOD;
        kBoneToLOD.GetNext(kBoneIter, pkBone, uiLOD);
        NiTPrimitiveSet<NiNode*>* pkSet = m_kBoneArray.m_pBase[uiLOD];
        if (!pkSet) {
            NiTPrimitiveSet<NiNode*>* pkNewSet = static_cast<NiTPrimitiveSet<NiNode*>*>(
                MemoryManager_Singleton.Allocate(sizeof(NiTPrimitiveSet<NiNode*>))
            );
            if (pkNewSet) {
                ::new (pkNewSet) NiTPrimitiveSet<NiNode*>(0);
                pkSet = pkNewSet;
            }
            else {
                pkSet = nullptr;
            }
            m_kBoneArray.SetAt(uiLOD, pkSet);
        }
        pkSet->Add(pkBone);
    }
    uint32_t uiCurrentLOD = 0;
    BuildBoneMap(pkBoneRoot, kBoneToLOD, uiCurrentLOD);
    SetBoneLOD(-1);
    return true;
}

void NiBSBoneLODController::CollectMarkedBones(
    NiNode* pkBone,
    NiTPointerMap<NiNode*, uint32_t>& kBoneToLOD,
    NiTPointerMap<uint32_t, bool>& kUsedLODs
) {
    char* Context;
    uint32_t i;
    for (i = 0; i < pkBone->m_usExtraDataSize; ++i) {
        NiStringExtraData* pStringExtraData;
        NiExtraData* pExtraData =
            static_cast<NiObjectNET&>(static_cast<NiAVObject&>(*pkBone)).m_ppkExtra[static_cast<uint16_t>(i)];
        if (pExtraData) {
            bool bIsStringExtraData;
            for (const NiRTTI* j = pExtraData->GetRTTI(); j; j = j->GetBaseRTTI()) {
                if (j == &NiStringExtraData::ms_RTTI) {
                    bIsStringExtraData = true;
                    goto LABEL_11;
                }
            }
            bIsStringExtraData = false;
        LABEL_11:;
            pStringExtraData = bIsStringExtraData ? static_cast<NiStringExtraData*>(pExtraData) : nullptr;
        }
        else {
            pStringExtraData = nullptr;
        }
        if (pStringExtraData) {
            const char* pcString = pStringExtraData->m_kString.GetHandle();
            if ((pcString) && (std::strstr(pcString, "BSBoneLOD#Bone#"))) {
                int32_t stDestSize = std::strlen(pcString) + 1;
                char* pcDest = static_cast<char*>(MemoryManager_Singleton.Allocate(stDestSize));
                strcpy_s(pcDest, stDestSize, pcString);
                char* String = strtok_s(pcDest, "#", &Context);
                if ((String) && (!std::strcmp(String, "BSBoneLOD"))) {
                    char* Stringa = strtok_s(nullptr, "#", &Context);
                    if ((Stringa) && (!std::strcmp(Stringa, "Bone"))) {
                        char* pcLOD = strtok_s(nullptr, "#", &Context);
                        if (pcLOD) {
                            uint32_t uiLOD = std::atoi(pcLOD);
                            kBoneToLOD.SetAt(pkBone, uiLOD);
                            kUsedLODs.SetAt(uiLOD, true);
                        }
                    }
                }

                MemoryManager_Singleton.Deallocate(pcDest);
                break;
            }
        }
    }
    for (i = 0; i < pkBone->m_kChildren.m_usSize; ++i) {
        NiAVObject* pkChild = pkBone->m_kChildren.m_pBase[i].m_pObject;
        if ((pkChild) && (pkChild->IsNode())) {
            CollectMarkedBones(static_cast<NiNode*>(pkChild), kBoneToLOD, kUsedLODs);
        }
    }
}

void NiBSBoneLODController::BuildBoneMap(
    NiNode* pkBone,
    NiTPointerMap<NiNode*, uint32_t>& kBoneToLOD,
    uint32_t& uiCurrentLOD
) {
    uint32_t uiLOD;
    uint32_t uiOrigLOD = uiCurrentLOD;
    for (uint32_t ui = 0; ui < pkBone->m_kChildren.m_usSize; ++ui) {
        NiAVObject* pkChild = pkBone->m_kChildren.m_pBase[ui].m_pObject;
        if ((pkChild) && (pkChild->IsNode())) {
            uint32_t uiReturnedLOD = uiOrigLOD;
            BuildBoneMap(static_cast<NiNode*>(pkChild), kBoneToLOD, uiReturnedLOD);
            if (uiReturnedLOD > uiCurrentLOD) {
                uiCurrentLOD = uiReturnedLOD;
            }
        }
    }
    if (kBoneToLOD.GetAt(pkBone, uiLOD)) {
        if (uiLOD > uiCurrentLOD) {
            uiCurrentLOD = uiLOD;
        }
    }
    else {
        kBoneToLOD.SetAt(pkBone, uiCurrentLOD);
    }
}

bool NiBSBoneLODController::GetHasBones() {
    for (uint32_t i = 0; i < m_kBoneArray.m_usSize; ++i) {
        NiTPrimitiveSet<NiNode*>* v5 = m_kBoneArray.m_pBase[i];
        if (v5) {
            for (uint32_t j = 0;; ++j) {
                NiTPrimitiveSet<NiNode*>* v1 = m_kBoneArray.m_pBase[i];
                if (j >= v5->m_uiUsed) {
                    break;
                }
                NiTPrimitiveSet<NiNode*>* v2 = m_kBoneArray.m_pBase[i];
                if (v5->m_pBase[j]) {
                    return true;
                }
            }
        }
    }
    return false;
}

NiObject* NiBSBoneLODController::CreateObject() {
    void* v2 = NiMemObject::operator new(sizeof(NiBSBoneLODController));
    if (v2) {
        return ::new (v2) NiBSBoneLODController;
    }

    return nullptr;
}

bool NiBSBoneLODController::ProcessScene(NiNode* pkSceneRoot) {
    // Raw storage: the body frees the set's buffer itself on every return, so no C++ destructor may free it again.
    FNVD_INLINED_LOCAL(NiTPrimitiveSet<NiNode*>, kBoneRootNodes, std::nullopt, std::nullopt);
    std::memset(&kBoneRootNodes, 0, sizeof(kBoneRootNodes));
    NiBSBoneLODController::FindBoneRootNodes(pkSceneRoot, kBoneRootNodes);
    if (kBoneRootNodes.m_uiUsed) {
        for (uint32_t stSizeInBytes = 0; stSizeInBytes < kBoneRootNodes.m_uiUsed; ++stSizeInBytes) {
            NiBSBoneLODController* pkCtlr;
            NiNode* pkBoneRoot = kBoneRootNodes.m_pBase[stSizeInBytes];
            NiBSBoneLODController* apMemory =
                static_cast<NiBSBoneLODController*>(NiMemObject::operator new(sizeof(NiBSBoneLODController)));
            if (apMemory) {
                pkCtlr = ::new (apMemory) NiBSBoneLODController;
            }
            else {
                pkCtlr = nullptr;
            }
            pkCtlr->SetTarget(pkBoneRoot);
            if (!pkCtlr->InitializeData(pkBoneRoot)) {
                pkBoneRoot->RemoveController(pkCtlr);
                _NiFree(kBoneRootNodes.m_pBase);
                return false;
            }
            pkCtlr->SetBoneLOD(0);
        }
        _NiFree(kBoneRootNodes.m_pBase);
        return true;
    }

    _NiFree(kBoneRootNodes.m_pBase);
    return false;
}

void NiBSBoneLODController::FindBoneRootNodes(NiNode* pkNode, NiTPrimitiveSet<NiNode*>& kBoneRootNodes) {
    for (uint32_t i = 0; i < pkNode->m_usExtraDataSize; ++i) {
        NiStringExtraData* pkStringExtra;
        NiStringExtraData* pkExtra = static_cast<NiStringExtraData*>(
            static_cast<NiObjectNET&>(static_cast<NiAVObject&>(*pkNode)).m_ppkExtra[static_cast<uint16_t>(i)]
        );
        if (pkExtra) {
            bool v2;
            for (const NiRTTI* j = pkExtra->GetRTTI(); j; j = j->GetBaseRTTI()) {
                if (j == &NiStringExtraData::ms_RTTI) {
                    v2 = true;
                    goto LABEL_11;
                }
            }
            v2 = false;
        LABEL_11:;
            pkStringExtra = v2 ? pkExtra : nullptr;
        }
        else {
            pkStringExtra = nullptr;
        }
        if (pkStringExtra) {
            const char* pcString = pkStringExtra->m_kString.GetHandle();
            if ((pcString) && (std::strstr(pcString, "BSBoneLOD#BoneRoot#"))) {
                kBoneRootNodes.Add(pkNode);
                return;
            }
        }
    }
    for (uint32_t k = 0; k < pkNode->m_kChildren.m_usSize; ++k) {
        NiAVObject* pkChild = pkNode->m_kChildren.m_pBase[k].m_pObject;
        if ((pkChild) && (pkChild->IsNode())) {
            NiBSBoneLODController::FindBoneRootNodes(static_cast<NiNode*>(pkChild), kBoneRootNodes);
        }
    }
}

void NiBSBoneLODController::InitLoaders() {
    NiStream::RegisterLoader("NiBSBoneLODController", &NiBSBoneLODController::CreateObject);
}
