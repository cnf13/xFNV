#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoneLODController.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiBoneLODController::ms_RTTI("NiBoneLODController", &NiTimeController::ms_RTTI);

namespace {

    // An empty set from the engine heap (NiTSet has no constructor; the fields are zeroed after a null check).
    template <class T>
    NiTPrimitiveSet<T>* NewPrimitiveSet() {
        NiTPrimitiveSet<T>* pkSet =
            static_cast<NiTPrimitiveSet<T>*>(MemoryManager::Instance().Allocate(sizeof(NiTPrimitiveSet<T>)));
        if (pkSet) {
            pkSet->m_pBase = nullptr;
            pkSet->m_uiAlloced = 0;
            pkSet->m_uiUsed = 0;
        }
        return pkSet;
    }

    // Only the skin instance pointer is initialised; the shape is filled in by the caller.
    NiBoneLODController::SkinInfo* NewSkinInfo() {
        void* pMemory = MemoryManager::Instance().Allocate(sizeof(NiBoneLODController::SkinInfo));
        return pMemory ? ::new (pMemory) NiBoneLODController::SkinInfo : nullptr;
    }

} // namespace

// No LOD; the bone and skin arrays empty, growing by one.
NiBoneLODController::NiBoneLODController()
  : NiTimeController(), m_iLOD(-1), m_uiNumLODs(0), m_kBoneArray(0, 1), m_kSkinArray(0, 1), m_kSkinSet(0) {}

// The arrays' and the skin set's destructors free their storage.
NiBoneLODController::~NiBoneLODController() {
    CleanUp();
}

NiObject* NiBoneLODController::CreateClone(NiCloningProcess& kCloning) {
    NiBoneLODController* v4;
    void* v3 = NiMemObject::operator new(sizeof(NiBoneLODController));
    if (v3) {
        v4 = ::new (v3) NiBoneLODController;
    }
    else {
        v4 = nullptr;
    }
    NiTimeController::CopyMembers(v4, kCloning);
    v4->m_iLOD = m_iLOD;
    v4->m_uiNumLODs = m_uiNumLODs;
    return v4;
}

// Each link slot is first filled with this controller as a placeholder; LinkObject replaces them.
void NiBoneLODController::LoadBinary(NiStream& kStream) {
    NiTimeController::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_iLOD);
    NiStreamLoadBinary(kStream, m_uiNumLODs);

    uint32_t uiNumBoneSets = 0;
    NiStreamLoadBinary(kStream, uiNumBoneSets);
    for (uint32_t i = 0; i < uiNumBoneSets; i++) {
        NiTPrimitiveSet<NiNode*>* pkBones = NewPrimitiveSet<NiNode*>();
        if (i >= m_kBoneArray.m_usMaxSize) {
            m_kBoneArray.SetSize(i + m_kBoneArray.m_usGrowBy);
        }
        // EXE-INLINED 0xBA3AA0
        if (i >= m_kBoneArray.m_usSize) {
            m_kBoneArray.m_usSize = static_cast<uint16_t>(i + 1);
            if (pkBones) {
                ++m_kBoneArray.m_usESize;
            }
        }
        else if (pkBones) {
            if (!m_kBoneArray.m_pBase[i]) {
                ++m_kBoneArray.m_usESize;
            }
        }
        else if (m_kBoneArray.m_pBase[i]) {
            --m_kBoneArray.m_usESize;
        }
        m_kBoneArray.m_pBase[i] = pkBones;

        uint32_t uiNumBones = 0;
        NiStreamLoadBinary(kStream, uiNumBones);
        for (uint32_t j = 0; j < uiNumBones; j++) {
            pkBones->Add(reinterpret_cast<NiNode*>(this));
            kStream.ReadLinkID();
        }
    }

    uint32_t uiNumSkinSets = 0;
    NiStreamLoadBinary(kStream, uiNumSkinSets);
    for (uint32_t i = 0; i < uiNumSkinSets; i++) {
        NiTPrimitiveSet<SkinInfo*>* pkSkins = NewPrimitiveSet<SkinInfo*>();
        m_kSkinArray.SetAtGrow(i, pkSkins);

        uint32_t uiNumSkins = 0;
        NiStreamLoadBinary(kStream, uiNumSkins);
        for (uint32_t j = 0; j < uiNumSkins; j++) {
            pkSkins->Add(NewSkinInfo());
            kStream.ReadLinkID();
            kStream.ReadLinkID();
        }
    }

    uint32_t uiNumSkinGeoms = 0;
    NiStreamLoadBinary(kStream, uiNumSkinGeoms);
    for (uint32_t i = 0; i < uiNumSkinGeoms; i++) {
        m_kSkinSet.Add(reinterpret_cast<NiGeometry*>(this));
        kStream.ReadLinkID();
    }
}

void NiBoneLODController::LinkObject(NiStream& kStream) {
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

    for (uint32_t i = 0; i < m_kSkinArray.GetSize(); i++) {
        NiTPrimitiveSet<SkinInfo*>* pkSkins = m_kSkinArray.GetAt(i);
        if (!pkSkins) {
            continue;
        }
        for (uint32_t j = 0; j < pkSkins->GetSize(); j++) {
            SkinInfo* pkInfo = pkSkins->GetAt(j);
            pkInfo->m_pkShape = static_cast<NiGeometry*>(kStream.GetObjectFromLinkID());
            pkInfo->m_spSkinInstance = static_cast<NiSkinInstance*>(kStream.GetObjectFromLinkID());
        }
    }

    for (uint32_t i = 0; i < m_kSkinSet.GetSize(); i++) {
        m_kSkinSet.ReplaceAt(i, static_cast<NiGeometry*>(kStream.GetObjectFromLinkID()));
    }
}

// Only the skin instances are owned; bones and shapes belong to the scene graph.
bool NiBoneLODController::RegisterStreamables(NiStream& kStream) {
    if (!NiTimeController::RegisterStreamables(kStream)) {
        return false;
    }

    for (uint32_t i = 0; i < m_kSkinArray.GetSize(); i++) {
        NiTPrimitiveSet<SkinInfo*>* pkSkins = m_kSkinArray.GetAt(i);
        if (!pkSkins) {
            continue;
        }
        for (uint32_t j = 0; j < pkSkins->GetSize(); j++) {
            NiSkinInstance* pkSkinInstance = pkSkins->GetAt(j)->m_spSkinInstance;
            if (pkSkinInstance) {
                pkSkinInstance->RegisterStreamables(kStream);
            }
        }
    }
    return true;
}

// An empty array slot is saved as a set of zero entries.
void NiBoneLODController::SaveBinary(NiStream& kStream) {
    NiTimeController::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_iLOD);
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
            uint32_t uiNumBones = 0;
            NiStreamSaveBinary(kStream, uiNumBones);
        }
    }

    uint32_t uiNumSkinSets = m_kSkinArray.GetSize();
    NiStreamSaveBinary(kStream, uiNumSkinSets);
    for (uint32_t i = 0; i < m_kSkinArray.GetSize(); i++) {
        NiTPrimitiveSet<SkinInfo*>* pkSkins = m_kSkinArray.GetAt(i);
        if (pkSkins) {
            uint32_t uiNumSkins = pkSkins->GetSize();
            NiStreamSaveBinary(kStream, uiNumSkins);
            for (uint32_t j = 0; j < pkSkins->GetSize(); j++) {
                SkinInfo* pkInfo = pkSkins->GetAt(j);
                kStream.SaveLinkID(pkInfo->m_pkShape);
                kStream.SaveLinkID(pkInfo->m_spSkinInstance);
            }
        }
        else {
            uint32_t uiNumSkins = 0;
            NiStreamSaveBinary(kStream, uiNumSkins);
        }
    }

    uint32_t uiNumSkinGeoms = m_kSkinSet.GetSize();
    NiStreamSaveBinary(kStream, uiNumSkinGeoms);
    for (uint32_t i = 0; i < m_kSkinSet.GetSize(); i++) {
        kStream.SaveLinkID(m_kSkinSet.GetAt(i));
    }
}

// Matching array slots must both be empty or hold sets of equal size whose entries compare equal; entries are
// compared without null checks.
bool NiBoneLODController::IsEqual(NiObject* pkObject) {
    if (!NiTimeController::IsEqual(pkObject)) {
        return false;
    }

    NiBoneLODController* pkOther = static_cast<NiBoneLODController*>(pkObject);
    if (m_iLOD != pkOther->m_iLOD || m_uiNumLODs != pkOther->m_uiNumLODs ||
        m_kBoneArray.GetSize() != pkOther->m_kBoneArray.GetSize() ||
        m_kSkinArray.GetSize() != pkOther->m_kSkinArray.GetSize() ||
        m_kSkinSet.GetSize() != pkOther->m_kSkinSet.GetSize()) {
        return false;
    }

    for (uint32_t i = 0; i < m_kBoneArray.GetSize(); i++) {
        NiTPrimitiveSet<NiNode*>* pkBones = m_kBoneArray.GetAt(i);
        NiTPrimitiveSet<NiNode*>* pkOtherBones = pkOther->m_kBoneArray.GetAt(i);
        if (!pkBones) {
            if (pkOtherBones) {
                return false;
            }
            continue;
        }
        if (!pkOtherBones || pkBones->GetSize() != pkOtherBones->GetSize()) {
            return false;
        }
        for (uint32_t j = 0; j < pkBones->GetSize(); j++) {
            if (!pkBones->GetAt(j)->IsEqual(pkOtherBones->GetAt(j))) {
                return false;
            }
        }
    }

    for (uint32_t i = 0; i < m_kSkinArray.GetSize(); i++) {
        NiTPrimitiveSet<SkinInfo*>* pkSkins = m_kSkinArray.GetAt(i);
        NiTPrimitiveSet<SkinInfo*>* pkOtherSkins = pkOther->m_kSkinArray.GetAt(i);
        if (!pkSkins) {
            if (pkOtherSkins) {
                return false;
            }
            continue;
        }
        if (!pkOtherSkins || pkSkins->GetSize() != pkOtherSkins->GetSize()) {
            return false;
        }
        for (uint32_t j = 0; j < pkSkins->GetSize(); j++) {
            SkinInfo* pkInfo = pkSkins->GetAt(j);
            SkinInfo* pkOtherInfo = pkOtherSkins->GetAt(j);
            if (!pkInfo->m_pkShape->IsEqual(pkOtherInfo->m_pkShape)) {
                return false;
            }
            if (!pkInfo->m_spSkinInstance->IsEqual(pkOtherInfo->m_spSkinInstance)) {
                return false;
            }
        }
    }

    for (uint32_t i = 0; i < m_kSkinSet.GetSize(); i++) {
        if (!m_kSkinSet.GetAt(i)->IsEqual(pkOther->m_kSkinSet.GetAt(i))) {
            return false;
        }
    }
    return true;
}

// No base class strings.
void NiBoneLODController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_iLOD", m_iLOD));
    pkStrings->Add(NiGetViewerString("m_uiNumLODs", m_uiNumLODs));
}

// The clone gets its own sets. Bones and shapes map to their clones when they were cloned too (otherwise the
// originals are shared); skin instances are always cloned. The clone then re-applies its LOD.
void NiBoneLODController::ProcessClone(NiCloningProcess& kCloning) {
    NiTimeController::ProcessClone(kCloning);

    NiObject* pkCloneObject;
    kCloning.m_pkCloneMap->GetAt(this, pkCloneObject);
    NiBoneLODController* pkClone = static_cast<NiBoneLODController*>(pkCloneObject);

    for (uint32_t i = 0; i < m_kBoneArray.GetSize(); i++) {
        NiTPrimitiveSet<NiNode*>* pkBones = m_kBoneArray.GetAt(i);
        if (!pkBones) {
            pkClone->m_kBoneArray.SetAtGrow(i, nullptr);
            continue;
        }

        NiTPrimitiveSet<NiNode*>* pkCloneBones = NewPrimitiveSet<NiNode*>();
        pkClone->m_kBoneArray.SetAtGrow(i, pkCloneBones);
        for (uint32_t j = 0; j < pkBones->GetSize(); j++) {
            NiNode* pkBone = pkBones->GetAt(j);
            NiNode* pkNewBone = pkBone;
            NiObject* pkCloneBone;
            if (kCloning.m_pkCloneMap->GetAt(pkBone, pkCloneBone)) {
                pkNewBone = static_cast<NiNode*>(pkCloneBone);
            }
            // EXE-INLINED 0xA543B0
            if (pkCloneBones->m_uiUsed == pkCloneBones->m_uiAlloced) {
                pkCloneBones->Realloc(pkCloneBones->m_uiAlloced ? pkCloneBones->m_uiAlloced * 2 : 1);
            }
            pkCloneBones->m_pBase[pkCloneBones->m_uiUsed++] = pkNewBone;
        }
    }

    for (uint32_t i = 0; i < m_kSkinArray.GetSize(); i++) {
        NiTPrimitiveSet<SkinInfo*>* pkSkins = m_kSkinArray.GetAt(i);
        if (!pkSkins) {
            pkClone->m_kSkinArray.SetAtGrow(i, nullptr);
            continue;
        }

        NiTPrimitiveSet<SkinInfo*>* pkCloneSkins = NewPrimitiveSet<SkinInfo*>();
        pkClone->m_kSkinArray.SetAtGrow(i, pkCloneSkins);
        for (uint32_t j = 0; j < pkSkins->GetSize(); j++) {
            SkinInfo* pkInfo = pkSkins->GetAt(j);
            SkinInfo* pkCloneInfo = NewSkinInfo();
            pkCloneSkins->Add(pkCloneInfo);

            NiObject* pkCloneShape;
            if (kCloning.m_pkCloneMap->GetAt(pkInfo->m_pkShape, pkCloneShape)) {
                pkCloneInfo->m_pkShape = static_cast<NiGeometry*>(pkCloneShape);
            }
            else {
                pkCloneInfo->m_pkShape = pkInfo->m_pkShape;
            }
            pkCloneInfo->m_spSkinInstance =
                static_cast<NiSkinInstance*>(pkInfo->m_spSkinInstance->CreateClone(kCloning));
            pkInfo->m_spSkinInstance->ProcessClone(kCloning);
        }
    }

    for (uint32_t i = 0; i < m_kSkinSet.GetSize(); i++) {
        NiGeometry* pkGeom = m_kSkinSet.GetAt(i);
        NiObject* pkCloneGeom;
        if (kCloning.m_pkCloneMap->GetAt(pkGeom, pkCloneGeom)) {
            pkClone->m_kSkinSet.Add(static_cast<NiGeometry*>(pkCloneGeom));
        }
        else {
            pkClone->m_kSkinSet.Add(pkGeom);
        }
    }

    int32_t iLOD = pkClone->m_iLOD;
    pkClone->m_iLOD = -1;
    pkClone->SetBoneLOD(iLOD);
}

// m_iLOD is reset so SetBoneLOD re-applies the loaded LOD.
void NiBoneLODController::PostLinkObject(NiStream& kStream) {
    NiTimeController::PostLinkObject(kStream);

    int32_t iLOD = m_iLOD;
    m_iLOD = -1;
    SetBoneLOD(iLOD);
}

// Follows the global LOD override (-1 = none), clamped to the last LOD only when it is above the LOD count. The
// update time is not used.
void NiBoneLODController::Update([[maybe_unused]] NiUpdateData& UpdateData) {
    int32_t iLOD = ms_iGlobalLOD;
    if (iLOD == -1 || iLOD == m_iLOD) {
        return;
    }

    if (iLOD > static_cast<int32_t>(m_uiNumLODs)) {
        iLOD = m_uiNumLODs - 1;
    }
    if (iLOD != m_iLOD) {
        SetBoneLOD(iLOD);
    }
}

bool NiBoneLODController::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiNode::ms_RTTI, m_pkTarget);
}

// Bone sets from iLOD on get selective update and the rest lose it; every skinned shape is culled, then iLOD's shapes
// are shown again with their skin instances. iLOD -1 only culls the skins. The skin array is indexed without a size
// check.
bool NiBoneLODController::SetBoneLOD(int32_t iLOD) {
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
            if (bOn) {
                pkBone->m_uFlags = NiAVObject::NiFlags(pkBone->m_uFlags | NiAVObject::SELECTIVE_UPDATE);
            }
            else {
                pkBone->m_uFlags = NiAVObject::NiFlags(pkBone->m_uFlags & ~NiAVObject::SELECTIVE_UPDATE);
            }
        }
    }

    for (uint32_t i = 0; i < m_kSkinSet.GetSize(); i++) {
        NiGeometry* pkGeom = m_kSkinSet.GetAt(i);
        pkGeom->m_uFlags =
            NiAVObject::NiFlags((pkGeom->m_uFlags & ~NiAVObject::SELECTIVE_UPDATE) | NiAVObject::APP_CULLED);
    }

    if (iLOD != -1) {
        NiTPrimitiveSet<SkinInfo*>* pkSkins = m_kSkinArray.m_pBase[iLOD];
        if (pkSkins) {
            for (uint32_t i = 0; i < pkSkins->GetSize(); i++) {
                NiGeometry* pkShape = pkSkins->GetAt(i)->m_pkShape;
                pkShape->m_uFlags =
                    NiAVObject::NiFlags((pkShape->m_uFlags & ~NiAVObject::APP_CULLED) | NiAVObject::SELECTIVE_UPDATE);
                pkShape->m_spSkinInstance = pkSkins->GetAt(i)->m_spSkinInstance.m_pObject;
            }
        }
    }

    m_iLOD = iLOD;
    return true;
}

// Frees every bone set and skin set (each skin info releasing its skin instance), then empties both arrays and the skin
// set.
void NiBoneLODController::CleanUp() {
    for (uint32_t i = 0; i < m_kBoneArray.m_usSize; ++i) {
        NiTPrimitiveSet<NiNode*>* pkBones = m_kBoneArray.m_pBase[i];
        if (pkBones) {
            _NiFree(pkBones->m_pBase);
            MemoryManager_Singleton.Deallocate(pkBones);
        }
    }
    for (uint16_t j = 0; j < m_kBoneArray.m_usSize; ++j) {
        m_kBoneArray.m_pBase[j] = nullptr;
    }
    m_kBoneArray.m_usSize = 0;
    m_kBoneArray.m_usESize = 0;

    for (uint32_t i = 0; i < m_kSkinArray.m_usSize; ++i) {
        NiTPrimitiveSet<SkinInfo*>* pkSkins = m_kSkinArray.m_pBase[i];
        if (pkSkins) {
            for (uint32_t j = 0; j < pkSkins->m_uiUsed; ++j) {
                SkinInfo* pkInfo = pkSkins->m_pBase[j];
                if (pkInfo) {
                    NiSkinInstance* pkSkinInstance = pkInfo->m_spSkinInstance.m_pObject;
                    if (pkSkinInstance) {
                        pkSkinInstance->DecRefCount();
                    }
                    MemoryManager_Singleton.Deallocate(pkInfo);
                }
            }
            _NiFree(pkSkins->m_pBase);
            MemoryManager_Singleton.Deallocate(pkSkins);
        }
    }
    for (uint16_t j = 0; j < m_kSkinArray.m_usSize; ++j) {
        m_kSkinArray.m_pBase[j] = nullptr;
    }
    m_kSkinArray.m_usSize = 0;
    m_kSkinArray.m_usESize = 0;
    m_kSkinSet.m_uiUsed = 0;
}

NiObject* NiBoneLODController::CreateObject() {
    void* v0 = NiMemObject::operator new(sizeof(NiBoneLODController));
    if (v0) {
        return ::new (v0) NiBoneLODController;
    }

    return nullptr;
}
