#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParallelUpdateTaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticlesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysUpdateTask.h"

#include <limits>

NiRTTI NiParticleSystem::ms_RTTI("NiParticleSystem", &NiParticles::ms_RTTI);

NiParticleSystem::NiParticleSystem()
  : m_bWorldSpace(true), m_fLastTime(-std::numeric_limits<float>::max()), m_bResetSystem(false),
    m_bDynamicBounds(false) {}

// The destructor body is empty because the modifier list's own destructor empties the list.
NiParticleSystem::~NiParticleSystem() {}

NiObject* NiParticleSystem::CreateClone(NiCloningProcess& kCloning) {
    NiParticleSystem* pkObject = new NiParticleSystem;
    CopyMembers(pkObject, kCloning);
    return pkObject;
}

// The modifier link IDs are queued here and resolved in LinkObject. A short read leaves ucWorldSpace uninitialised.
void NiParticleSystem::LoadBinary(NiStream& kStream) {
    NiParticles::LoadBinary(kStream);

    uint8_t ucWorldSpace;
    NiStreamLoadBinary(kStream, ucWorldSpace);
    m_bWorldSpace = ucWorldSpace != 0;
    kStream.ReadMultipleLinkIDs();
}

// Each linked object is added as a modifier without a type check.
void NiParticleSystem::LinkObject(NiStream& kStream) {
    NiParticles::LinkObject(kStream);

    uint32_t uiNumModifiers = kStream.GetNumberOfLinkIDs();
    for (uint32_t i = 0; i < uiNumModifiers; i++) {
        AddModifier(static_cast<NiPSysModifier*>(kStream.GetObjectFromLinkID()));
    }
}

// The modifiers' results are ignored, and the list elements are not null-checked.
bool NiParticleSystem::RegisterStreamables(NiStream& kStream) {
    if (!NiParticles::RegisterStreamables(kStream)) {
        return false;
    }

    NiTListItem<NiPointer<NiPSysModifier>>* kPos = m_kModifierList.GetHeadPos();
    while (kPos) {
        NiPSysModifier* pkModifier = m_kModifierList.GetNext(kPos);
        pkModifier->RegisterStreamables(kStream);
    }
    return true;
}

void NiParticleSystem::SaveBinary(NiStream& kStream) {
    NiParticles::SaveBinary(kStream);

    uint8_t ucWorldSpace = m_bWorldSpace;
    NiStreamSaveBinary(kStream, ucWorldSpace);
    uint32_t uiCount = m_kModifierList.m_uiCount;
    NiStreamSaveBinary(kStream, uiCount);
    NiTListItem<NiPointer<NiPSysModifier>>* kPos = m_kModifierList.GetHeadPos();
    while (kPos) {
        kStream.SaveLinkID(m_kModifierList.GetNext(kPos));
    }
}

// Each modifier is compared with the other system's modifier of the same name (null when there is none).
bool NiParticleSystem::IsEqual(NiObject* pkObject) {
    if (!NiParticles::IsEqual(pkObject)) {
        return false;
    }

    NiParticleSystem* pkOther = static_cast<NiParticleSystem*>(pkObject);
    if (m_bWorldSpace != pkOther->m_bWorldSpace) {
        return false;
    }
    if (m_bDynamicBounds != pkOther->m_bDynamicBounds) {
        return false;
    }

    NiTListItem<NiPointer<NiPSysModifier>>* kPos = m_kModifierList.GetHeadPos();
    while (kPos) {
        NiPSysModifier* pkModifier = m_kModifierList.GetNext(kPos);
        NiPSysModifier* pkOtherModifier = pkOther->GetModifierByName(pkModifier->m_kName);
        if (!pkModifier->IsEqual(pkOtherModifier)) {
            return false;
        }
    }
    return true;
}

// The model data is read as particle data without a null check.
void NiParticleSystem::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiParticles::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("World Space", m_bWorldSpace));
    pkStrings->Add(NiGetViewerString("NumMods", m_kModifierList.m_uiCount));
    pkStrings->Add(
        NiGetViewerString("NumParticles", static_cast<NiParticlesData*>(m_spModelData.m_pObject)->m_usActive)
    );
    pkStrings->Add(
        NiGetViewerString("MaxParticles", static_cast<NiParticlesData*>(m_spModelData.m_pObject)->m_usVertices)
    );
}

// The clone is looked up and never used; the modifiers and the model data are not null-checked.
void NiParticleSystem::ProcessClone(NiCloningProcess& kCloning) {
    NiParticles::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiTListItem<NiPointer<NiPSysModifier>>* kPos = m_kModifierList.GetHeadPos();
    while (kPos) {
        NiPSysModifier* pkModifier = m_kModifierList.GetNext(kPos);
        pkModifier->ProcessClone(kCloning);
    }
    m_spModelData->ProcessClone(kCloning);
}

// A particle system always takes part in selective updates and is never rigid; the transform and controller bits are
// always set.
void NiParticleSystem::SetSelectiveUpdateFlags(bool& bSelectiveUpdate, int32_t, bool& bRigid) {
    bSelectiveUpdate = true;
    bRigid = false;
    SetBit(bSelectiveUpdate, SELECTIVE_UPDATE);
    m_uFlags = NiFlags(m_uFlags | SELECTIVE_UPDATE_TRANSFORMS | SELECTIVE_UPDATE_CONTROLLER);
    SetBit(bRigid, SELECTIVE_UPDATE_RIGID);
}

// During a parallel update the system only reports that particles were found.
void NiParticleSystem::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    if (UpdateData.bParallelUpdate) {
        UpdateData.bFoundParticles = true;
        return;
    }

    UpdateWorldData(UpdateData);
    NiParticles::UpdateDownwardPass(UpdateData, uFlags);
}

// UpdateRigidDownwardPass uses the same function.
void NiParticleSystem::UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    if (UpdateData.bParallelUpdate) {
        UpdateData.bFoundParticles = true;
        return;
    }

    if (m_uFlags & SELECTIVE_UPDATE_TRANSFORMS) {
        UpdateWorldData(UpdateData);
    }
    NiParticles::UpdateSelectedDownwardPass(UpdateData, uFlags);
}

void NiParticleSystem::UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    if (UpdateData.bParallelUpdate) {
        UpdateData.bFoundParticles = true;
        return;
    }

    if (m_uFlags & SELECTIVE_UPDATE_TRANSFORMS) {
        UpdateWorldData(UpdateData);
    }
    NiParticles::UpdateSelectedDownwardPass(UpdateData, uFlags);
}

// The world transform before the world-space override is kept for the bound; a world-space system then drops its
// translation and rotation (the scale stays).
void NiParticleSystem::UpdateWorldData(NiUpdateData& UpdateData) {
    NiParticles::UpdateWorldData(UpdateData);

    m_kUnmodifiedWorld = m_kWorld;
    if (m_bWorldSpace) {
        m_kWorld.m_Translate = NiPoint3::ZERO;
        m_kWorld.m_Rotate = NiMatrix3::IDENTITY;
    }
}

// A world-space system's particles are already in world space: a dynamic bound is only scaled, a static one is moved
// by the unmodified world transform.
void NiParticleSystem::UpdateWorldBound() {
    NiParticles::UpdateWorldBound();

    if (m_bWorldSpace) {
        NiBound& rkModelBound = m_spModelData->m_kBound;
        if (m_bDynamicBounds) {
            *m_kWorldBound = rkModelBound;
            float fScale = m_kWorld.m_fScale;
            m_kWorldBound->m_kCenter = m_kWorldBound->m_kCenter * fScale;
            m_kWorldBound->m_fRadius *= m_kWorld.m_fScale;
        }
        else {
            m_kWorldBound->Update(rkModelBound, m_kUnmodifiedWorld);
        }
    }
}

void NiParticleSystem::SetModelData(NiGeometryData* pkModelData) {
    NiParticles::SetModelData(pkModelData);
}

// Queues the update on the parallel update manager when it runs and this is the update thread; a task the manager
// does not take goes back to the pool, and the system updates itself.
void NiParticleSystem::UpdateSystem(float fTime) {
    if (NiParticleSystem::ms_bParallelUpdateEnabled &&
        NiParallelUpdateTaskManager::ms_pkManager &&
        (NiParallelUpdateTaskManager::ms_pkManager->m_bActive &&
         (m_spModelData->m_kBound.m_fRadius > 0.0f && GetCurrentThreadId() == NiParticleSystem::iThreadID))) {
        NiPSysUpdateTask* pkTask = NiPSysUpdateTask::GetFreeObject();
        if (pkTask) {
            pkTask->Init(this, fTime);
            if (NiParallelUpdateTaskManager::ms_pkManager
                    ->AddTask(pkTask, NiParticleSystem::ms_eParallelUpdatePriority)) {
                return;
            }
            pkTask->Clear();
        }
    }

    Do_UpdateSystem(fTime);
}

// Strip update modifiers run after all the others (the last one found wins). A system whose bound radius becomes
// non-zero flags its nearest fade node ancestor so that the world bound is recomputed.
void NiParticleSystem::Do_UpdateSystem(float fTime) {
    NiPSysData* pkData = static_cast<NiPSysData*>(m_spModelData.m_pObject);
    bool bNoBound = pkData->m_kBound.m_iRadiusAsInt == 0;
    pkData->ResolveAddedParticles();

    if (m_bResetSystem || fTime < m_fLastTime) {
        PerformSystemReset();
        m_bResetSystem = false;
    }
    if (m_fLastTime == -std::numeric_limits<float>::max()) {
        m_fLastTime = fTime;
    }

    NiPSysModifier* pkStripUpdate = nullptr;
    NiTListItem<NiPointer<NiPSysModifier>>* kPos = m_kModifierList.GetHeadPos();
    while (kPos) {
        NiPSysModifier* pkModifier = m_kModifierList.GetNext(kPos);
        if (pkModifier->IsBSPSysStripUpdateModifier()) {
            pkStripUpdate = pkModifier;
        }
        else if (pkModifier->m_bActive) {
            pkModifier->Update(fTime, pkData);
        }
    }
    if (pkStripUpdate && pkStripUpdate->m_bActive) {
        pkStripUpdate->Update(fTime, pkData);
    }

    pkData->MarkAsChanged(7);
    m_fLastTime = fTime;

    if (bNoBound && pkData->m_kBound.m_iRadiusAsInt != 0) {
        for (NiNode* pkParent = m_pkParent; pkParent; pkParent = pkParent->m_pkParent) {
            if (pkParent->IsFadeNode()) {
                pkParent->m_uFlags = NiFlags(pkParent->m_uFlags | WORLD_BOUND_CHANGE);
                return;
            }
        }
    }
}

// The model data is read once, before the list is walked.
void NiParticleSystem::InitializeNewParticle(uint16_t usNewParticle) {
    NiPSysData* pkData = static_cast<NiPSysData*>(m_spModelData.m_pObject);
    NiTListItem<NiPointer<NiPSysModifier>>* kPos = m_kModifierList.GetHeadPos();
    while (kPos) {
        NiPSysModifier* pkModifier = m_kModifierList.GetNext(kPos);
        pkModifier->Initialize(pkData, usNewParticle);
    }
}

NiPSysModifier* NiParticleSystem::GetModifierByName(const NiFixedString& kName) {
    NiTListItem<NiPointer<NiPSysModifier>>* kPos = m_kModifierList.GetHeadPos();
    while (kPos) {
        NiPSysModifier* pkModifier = m_kModifierList.GetNext(kPos);
        if (pkModifier && pkModifier->m_kName == kName) {
            return pkModifier;
        }
    }
    return nullptr;
}

// An empty list is detected by its head, not its count.
void NiParticleSystem::AddModifier(NiPSysModifier* pkModifier) {
    NiTListItem<NiPointer<NiPSysModifier>>* kPos = m_kModifierList.GetHeadPos();
    if (!kPos) {
        m_kModifierList.AddHead(pkModifier);
    }
    else {
        while (kPos && kPos->m_element->m_uiOrder <= pkModifier->m_uiOrder) {
            kPos = kPos->m_pkNext;
        }
        if (kPos) {
            m_kModifierList.InsertBefore(kPos, pkModifier);
        }
        else {
            m_kModifierList.AddTail(pkModifier);
        }
    }
    pkModifier->SetSystemPointer(this);
}

uint32_t NiParticleSystem::GetModifierCount() {
    return m_kModifierList.m_uiCount;
}

uint16_t NiParticleSystem::GetMaxNumParticles() {
    return m_spModelData->NiGeometryData::GetVertexCount();
}

// NiParticles' copy is the engine's NiTriShape::CopyMembers. Each modifier's clone is held in a NiPointer
// across the AddTail; the model data's clone is assigned through the NiPointer.
void NiParticleSystem::CopyMembers(NiParticleSystem* pkDest, NiCloningProcess& kCloning) {
    NiParticles::CopyMembers(pkDest, kCloning);
    pkDest->m_bWorldSpace = m_bWorldSpace;
    NiTListItem<NiPointer<NiPSysModifier>>* pkPos = m_kModifierList.m_pkHead;
    while (pkPos) {
        NiPSysModifier* pkModifier = pkPos->m_element.m_pObject;
        pkPos = pkPos->m_pkNext;
        NiPointer<NiPSysModifier> spClone = static_cast<NiPSysModifier*>(pkModifier->CreateClone(kCloning));
        pkDest->m_kModifierList.AddTail(spClone);
    }
    pkDest->m_fLastTime = m_fLastTime;
    pkDest->m_bDynamicBounds = m_bDynamicBounds;
    pkDest->m_spModelData = static_cast<NiGeometryData*>(m_spModelData->CreateClone(kCloning));
}

// The modifier controllers are found by walking the controller chain with a type check.
void NiParticleSystem::PerformSystemReset() {
    NiPSysData* pkData = static_cast<NiPSysData*>(m_spModelData.m_pObject);
    pkData->SetActiveVertexCount(0);
    pkData->m_usNumAddedParticles = 0;
    pkData->m_usAddedParticlesBase = 0;
    m_fLastTime = -std::numeric_limits<float>::max();

    NiTListItem<NiPointer<NiPSysModifier>>* kPos = m_kModifierList.GetHeadPos();
    while (kPos) {
        NiPSysModifier* pkModifier = m_kModifierList.GetNext(kPos);
        pkModifier->HandleReset();
    }

    for (NiTimeController* pkCtlr = m_spControllers; pkCtlr; pkCtlr = pkCtlr->m_spNext.m_pObject) {
        NiPSysModifierCtlr* pkModifierCtlr = static_cast<NiPSysModifierCtlr*>(
            pkCtlr->DynamicCast(&NiPSysModifierCtlr::ms_RTTI)
        );
        if (pkModifierCtlr) {
            pkModifierCtlr->HandleReset();
        }
    }
}

NiObject* NiParticleSystem::CreateObject() {
    return new NiParticleSystem;
}
