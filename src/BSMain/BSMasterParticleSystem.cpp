#include "BSMain/BSMasterParticleSystem.h"

#include "BSMain/BSPSysMultiTargetEmitterCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticlesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI BSMasterParticleSystem::ms_RTTI("BSMasterParticleSystem", &NiNode::ms_RTTI);

BSMasterParticleSystem::BSMasterParticleSystem()
  : NiNode(0), usActiveEmitterObjCount(0), usMaxEmitterObj(20), usMaxParticlesPerFrame(0), FirstEmitterObjIter(nullptr),
    uiIndex(0xFFFFFFFFu), uiEditorIndex(0xFFFFFFFFu), ChildParticles(0, 1), fMasterPSysTime(0.0f) {
    lEmitterObjList.RemoveAll();
}

// The emitter objects go and the child particle array is emptied; the members' destructors free the rest.
BSMasterParticleSystem::~BSMasterParticleSystem() {
    FirstEmitterObjIter = nullptr;
    lEmitterObjList.RemoveAll();
    ChildParticles.RemoveAll();
}

NiObject* BSMasterParticleSystem::CreateClone(NiCloningProcess& kCloning) {
    BSMasterParticleSystem* v3;
    v3 = new BSMasterParticleSystem;
    CopyMembers(v3, kCloning);
    return v3;
}

// Version 16 and later files list the child particle systems after the emitter object cap.
void BSMasterParticleSystem::LoadBinary(NiStream& kStream) {
    NiNode::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, usMaxEmitterObj);
    if (kStream.m_BSStreamHeader.uiVersion >= 0x10) {
        kStream.ReadMultipleLinkIDs();
    }
}

void BSMasterParticleSystem::LinkObject(NiStream& kStream) {
    NiNode::LinkObject(kStream);
    if (kStream.m_BSStreamHeader.uiVersion < 0x10) {
        return;
    }

    uint32_t uiCount = kStream.GetNumberOfLinkIDs();
    if (uiCount) {
        ChildParticles.SetSize(uiCount);
        for (uint32_t i = 0; i < uiCount; ++i) {
            NiParticles* pkParticles = static_cast<NiParticles*>(kStream.GetObjectFromLinkID());
            ChildParticles.SetAt(i, pkParticles);
        }
    }
}

bool BSMasterParticleSystem::RegisterStreamables(NiStream& kStream) {
    if (!NiNode::RegisterStreamables(kStream)) {
        return false;
    }

    for (NiParticles* pkParticles : ChildParticles) {
        if (pkParticles) {
            pkParticles->RegisterStreamables(kStream);
        }
    }
    return true;
}

void BSMasterParticleSystem::SaveBinary(NiStream& kStream) {
    NiNode::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, usMaxEmitterObj);
    uint32_t uiCount = ChildParticles.GetSize();
    NiStreamSaveBinary(kStream, uiCount);
    for (uint32_t i = 0; i < uiCount; ++i) {
        kStream.SaveLinkID(ChildParticles.GetAt(i));
    }
}

// The emitter objects must be the same objects, in the same order; the child particle systems need only be equal.
bool BSMasterParticleSystem::IsEqual(NiObject* pkObject) {
    if (!NiNode::IsEqual(pkObject)) {
        return false;
    }

    BSMasterParticleSystem* pkOther = static_cast<BSMasterParticleSystem*>(pkObject);
    NiTListItem<NiPointer<NiAVObject>>* pkPos = lEmitterObjList.GetHeadPos();
    NiTListItem<NiPointer<NiAVObject>>* pkOtherPos = pkOther->lEmitterObjList.GetHeadPos();
    if (usActiveEmitterObjCount != pkOther->usActiveEmitterObjCount) {
        return false;
    }
    if (usMaxEmitterObj != pkOther->usMaxEmitterObj) {
        return false;
    }
    if (uiIndex != pkOther->uiIndex) {
        return false;
    }
    if (usMaxParticlesPerFrame != pkOther->usMaxParticlesPerFrame) {
        return false;
    }

    while (pkPos && pkOtherPos) {
        NiAVObject* pkObject = lEmitterObjList.GetNext(pkPos);
        NiAVObject* pkOtherObject = pkOther->lEmitterObjList.GetNext(pkOtherPos);
        if (pkObject != pkOtherObject) {
            return false;
        }
    }

    uint32_t uiCount = ChildParticles.GetSize();
    uint32_t uiOtherCount = pkOther->ChildParticles.GetSize();
    if (uiCount != uiOtherCount) {
        return false;
    }
    for (uint32_t i = 0; i < uiCount; ++i) {
        NiParticles* pkParticles = ChildParticles.GetAt(i);
        NiParticles* pkOtherParticles = pkOther->ChildParticles.GetAt(i);
        if ((pkParticles && !pkOtherParticles) || (!pkParticles && pkOtherParticles)) {
            return false;
        }
        if (pkParticles && !pkParticles->IsEqual(pkOtherParticles)) {
            return false;
        }
    }

    return !(pkPos || pkOtherPos);
}

// One line per emitter object: its name, or "No Name", and its address.
void BSMasterParticleSystem::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiNode::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Index", uiIndex));
    pkStrings->Add(NiGetViewerString("Max Emitter Objs", usMaxEmitterObj));
    pkStrings->Add(NiGetViewerString("Emitter Objs"));

    NiTListItem<NiPointer<NiAVObject>>* pkPos = lEmitterObjList.GetHeadPos();
    while (pkPos) {
        NiAVObject* pkObject = lEmitterObjList.GetNext(pkPos);
        const char* pcName = pkObject->m_kName.Exists() ? static_cast<const char*>(pkObject->m_kName) : "No Name";
        pkStrings->Add(NiGetViewerString(pcName, static_cast<const void*>(pkObject)));
    }
}

// Older files have no child list; it is gathered from the linked subtree instead.
void BSMasterParticleSystem::PostLinkObject(NiStream& kStream) {
    NiNode::PostLinkObject(kStream);
    if (kStream.m_BSStreamHeader.uiVersion < 0x10) {
        UpdateParticleSystemList();
    }
}

// The three passes run the subtree on the master's own clock, which only advances while an emitter object is
// active or a child still has particles.
void BSMasterParticleSystem::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    ClearUnreferencedEmitters();
    if (usActiveEmitterObjCount > 0 || ChildHasActiveParticles()) {
        fMasterPSysTime += UpdateData.fTime;
        float fTime = UpdateData.fTime;
        UpdateData.fTime = fMasterPSysTime;
        NiNode::UpdateDownwardPass(UpdateData, uFlags);
        UpdateData.fTime = fTime;
    }
}

void BSMasterParticleSystem::UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    ClearUnreferencedEmitters();
    if (usActiveEmitterObjCount > 0 || ChildHasActiveParticles()) {
        fMasterPSysTime += UpdateData.fTime;
        float fTime = UpdateData.fTime;
        UpdateData.fTime = fMasterPSysTime;
        NiNode::UpdateSelectedDownwardPass(UpdateData, uFlags);
        UpdateData.fTime = fTime;
    }
}

void BSMasterParticleSystem::UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    ClearUnreferencedEmitters();
    if (usActiveEmitterObjCount > 0 || ChildHasActiveParticles()) {
        fMasterPSysTime += UpdateData.fTime;
        float fTime = UpdateData.fTime;
        UpdateData.fTime = fMasterPSysTime;
        NiNode::UpdateRigidDownwardPass(UpdateData, uFlags);
        UpdateData.fTime = fTime;
    }
}

void BSMasterParticleSystem::UpdateParticleSystemList() {
    ChildParticles.RemoveAll();
    FindNiParticlesRecurse(this, &ChildParticles);
}

void BSMasterParticleSystem::SetEditorIndex(uint32_t auiEditorIndex) {
    uiEditorIndex = auiEditorIndex;
}

void BSMasterParticleSystem::SetIndex(uint32_t auiIndex) {
    uiIndex = auiIndex;
}

void BSMasterParticleSystem::SetMaxParticlesPerFrame(uint16_t ausMaxParticlesPerFrame) {
    usMaxParticlesPerFrame = ausMaxParticlesPerFrame;
}

void BSMasterParticleSystem::ClearAllParticles() {
    NiParticleSystem* v2;
    uint32_t i;
    uint32_t m_usSize;
    m_usSize = ChildParticles.m_usSize;
    for (i = 0; i < m_usSize; ++i) {
        v2 = static_cast<NiParticleSystem*>(ChildParticles.m_pBase[i]);
        if (v2) {
            v2->m_bResetSystem = true;
        }
    }
}

void BSMasterParticleSystem::ClearAllEmitterObj() {
    usActiveEmitterObjCount = 0;
    FirstEmitterObjIter = nullptr;
    lEmitterObjList.NiTPointerListBase<NiPointer<NiAVObject>>::RemoveAll();
}

void BSMasterParticleSystem::SetMaxEmitterObj(uint16_t ausMaxEmitterObj) {
    usMaxEmitterObj = ausMaxEmitterObj;
}

// One pass around the list from the iterator: an emitter object only the list still holds, or one that is culled,
// is dropped. When more than usMaxEmitterObj remain, the iterator stays on the first one past the cap, so the
// next round of emission starts there.
void BSMasterParticleSystem::ClearUnreferencedEmitters() {
    usActiveEmitterObjCount = 0;
    if (!FirstEmitterObjIter) {
        FirstEmitterObjIter = lEmitterObjList.GetHeadPos();
    }

    NiTListItem<NiPointer<NiAVObject>>* pkFirstPastCap = nullptr;
    uint32_t uiCount = lEmitterObjList.GetSize();
    while (FirstEmitterObjIter && uiCount-- != 0) {
        NiAVObject* pkObject = FirstEmitterObjIter->m_element;
        if (pkObject->m_uiRefCount == 1 || pkObject->GetAppCulled()) {
            // The removed element comes back by value and is released right away.
            lEmitterObjList.RemovePos(FirstEmitterObjIter);
        }
        else {
            if (usActiveEmitterObjCount == usMaxEmitterObj) {
                pkFirstPastCap = FirstEmitterObjIter;
            }
            ++usActiveEmitterObjCount;
            FirstEmitterObjIter = lEmitterObjList.GetNextPos(FirstEmitterObjIter);
        }
        if (!FirstEmitterObjIter) {
            FirstEmitterObjIter = lEmitterObjList.GetHeadPos();
        }
    }

    if (pkFirstPastCap) {
        FirstEmitterObjIter = pkFirstPastCap;
    }
    uint16_t usMax = usMaxEmitterObj;
    uint16_t usActive = usActiveEmitterObjCount;
    usActiveEmitterObjCount = usActive < usMax ? usActive : usMax;
    if (!usActiveEmitterObjCount) {
        FirstEmitterObjIter = nullptr;
    }
}

bool BSMasterParticleSystem::ChildHasActiveParticles() {
    uint32_t uiCount = ChildParticles.GetSize();
    for (uint32_t i = 0; i < uiCount; ++i) {
        NiParticles* pkParticles = ChildParticles.GetAt(i);
        if (pkParticles && pkParticles->m_spModelData->GetActiveVertexCount() > 0) {
            return true;
        }
    }
    return false;
}

// Particle geometry is not searched below.
void BSMasterParticleSystem::FindNiParticlesRecurse(
    NiAVObject* apObject,
    NiTPrimitiveArray<NiParticles*>* apNiParticles
) {
    if (!apObject) {
        return;
    }

    if (apObject->IsParticlesGeom()) {
        apNiParticles->Add(static_cast<NiParticles*>(apObject));
        return;
    }

    NiNode* pkNode = apObject->IsNode();
    if (pkNode) {
        uint32_t uiCount = pkNode->m_kChildren.GetSize();
        for (uint32_t i = 0; i < uiCount; ++i) {
            NiAVObject* pkChild = pkNode->m_kChildren.m_pBase[i];
            if (pkChild) {
                FindNiParticlesRecurse(pkChild, apNiParticles);
            }
        }
    }
}

void BSMasterParticleSystem::CopyMembers(BSMasterParticleSystem* apDest, NiCloningProcess& arCloneData) {
    NiNode::CopyMembers(apDest, arCloneData);
    apDest->usMaxEmitterObj = usMaxEmitterObj;
}

void BSMasterParticleSystem::AddEmitterObj(NiAVObject* apEmitterObj) {
    NiTListItem<NiPointer<NiAVObject>>* v9;
    if (apEmitterObj) {
        NiIncRefCount(apEmitterObj);
    }
    NiTListItem<NiPointer<NiAVObject>>* kIter = lEmitterObjList.m_pkHead;
    v9 = nullptr;
    while (kIter) {
        NiTListItem<NiPointer<NiAVObject>>* kLastPos = kIter;
        NiAVObject** pItem = &kIter->m_element.m_pObject;
        kIter = kIter->m_pkNext;
        if (apEmitterObj == *pItem) {
            v9 = kLastPos;
            break;
        }
    }
    if (apEmitterObj) {
        NiDecRefCount(apEmitterObj);
    }
    if (!v9) {
        NiPointer<NiAVObject> spEmitterObj = apEmitterObj;
        lEmitterObjList.AddTail(spEmitterObj);
    }
}

// The children are NiParticleSystem objects (their model data is a NiParticlesData): this adds up the active particle
// count and the vertex count of the model data, and the modifier list's count.
void BSMasterParticleSystem::GetDebugInfo(BSMasterParticleSystem::DebugInfo& apInfo) {
    uint32_t uiactiveParticleCount = 0;
    uint32_t uimaxParticleCount = 0;
    uint32_t uimodifierCount = 0;
    uint32_t uicount = ChildParticles.m_usSize;
    for (uint32_t uindex = 0; uindex < uicount; ++uindex) {
        NiParticleSystem* pparticleSystem = static_cast<NiParticleSystem*>(ChildParticles.m_pBase[uindex]);
        if (pparticleSystem) {
            uiactiveParticleCount += static_cast<NiParticlesData*>(pparticleSystem->m_spModelData.m_pObject)
                                         ->m_usActive;
            uimaxParticleCount += pparticleSystem->m_spModelData->m_usVertices;
            uimodifierCount += pparticleSystem->m_kModifierList.m_uiCount;
        }
    }
    apInfo.uiActiveParticleCount = uiactiveParticleCount;
    apInfo.uiMaxParticleCount = uimaxParticleCount;
    apInfo.uiModifierCount = uimodifierCount;
    apInfo.uiActiveEmitterObjCount = usActiveEmitterObjCount;
    apInfo.uiMaxEmitterObjCount = lEmitterObjList.m_uiCount;
    apInfo.uiParticleSystemCount = uicount;
}

NiObject* BSMasterParticleSystem::CreateObject() {
    return new BSMasterParticleSystem;
}

bool BSMasterParticleSystem::IsMasterParticleSystem(NiParticles* apSystem) {
    for (NiTimeController* i = apSystem->m_spControllers; i; i = i->m_spNext.m_pObject) {
        if (i->GetRTTI() == &BSPSysMultiTargetEmitterCtlr::ms_RTTI) {
            return true;
        }
    }
    return false;
}
