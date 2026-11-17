#include "BSMain/BSParticleSystemManager.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSMasterParticleSystem.h"
#include "BSMain/BSPSysMultiTargetEmitterCtlr.h"
#include "BSMain/UpdateMasterPsysTaskletData.h"
#include "BSSystem/BSTaskletData.h"
#include "BSSystem/BSTaskletGroup.h"
#include "BSSystem/BSTaskletManager.h"
#include "BSSystem/Win32/BSWin32TaskletData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysVolumeEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "SDK/MSVC/string.h"

// The warning RemoveMasterParticleSystem passes.
using aDefaultTrying_t = const char[86];

extern aDefaultTrying_t aDefaultTrying;
aDefaultTrying_t aDefaultTrying{
    "DEFAULT: Trying to remove a master particle system (idx: %d) that hasn't been loaded."
};

NiRTTI BSParticleSystemManager::ms_RTTI("BSParticleSystemManager", &NiNode::ms_RTTI);

// A 10-entry index lookup with every entry -1 (no child), and a new update tasklet group.
BSParticleSystemManager::BSParticleSystemManager()
  : NiNode(0), aIndexToChildLookup(nullptr), uiIndexToChildLookupSize(10), pUpdateTaskletGroup(nullptr) {
    aIndexToChildLookup =
        static_cast<uint32_t*>(MemoryManager_Singleton.Allocate(sizeof(uint32_t) * uiIndexToChildLookupSize));
    for (uint32_t i = 0; i < uiIndexToChildLookupSize; ++i) {
        aIndexToChildLookup[i] = UINT32_MAX;
    }
    BSTaskletGroup* pGroup = static_cast<BSTaskletGroup*>(MemoryManager_Singleton.Allocate(sizeof(BSTaskletGroup)));
    if (pGroup) {
        pGroup->pData = nullptr;
    }
    pUpdateTaskletGroup = pGroup;
    BSTaskletManager::QInstance().CreateTaskGroup(*pUpdateTaskletGroup);
}

// Frees the index lookup, then ends and frees the update tasklet group.
BSParticleSystemManager::~BSParticleSystemManager() {
    MemoryManager_Singleton.Deallocate(aIndexToChildLookup);
    aIndexToChildLookup = nullptr;
    uiIndexToChildLookupSize = 0;
    BSTaskletManager::QInstance().DestroyTaskGroup(*pUpdateTaskletGroup);
    if (pUpdateTaskletGroup) {
        MemoryManager_Singleton.Deallocate(pUpdateTaskletGroup);
    }
}

NiObject* BSParticleSystemManager::CreateClone(NiCloningProcess& kCloning) {
    BSParticleSystemManager* v3;
    void* v5 = NiMemObject::operator new(sizeof(BSParticleSystemManager));
    if (v5) {
        v3 = ::new (v5) BSParticleSystemManager;
    }
    else {
        v3 = nullptr;
    }
    CopyMembers(v3, kCloning);
    return v3;
}

void BSParticleSystemManager::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiNode::GetViewerStrings(pkStrings);
}

void BSParticleSystemManager::AttachChild(NiAVObject* apChild, int32_t abFirstAvail) {
    bool bIsMaster;
    if (apChild) {
        bIsMaster = apChild->GetRTTI() == &BSMasterParticleSystem::ms_RTTI;
    }
    else {
        bIsMaster = false;
    }
    if (bIsMaster) {
        uint32_t uiIndex = static_cast<BSMasterParticleSystem*>(apChild)->uiIndex;
        if (uiIndex != static_cast<uint32_t>(-1)) {
            if (uiIndex >= uiIndexToChildLookupSize) {
                uint32_t uiNewSize =
                    uiIndexToChildLookupSize + 10 * ((uiIndex - uiIndexToChildLookupSize + 1) / 10 + 1);
                void* pvDest = MemoryManager_Singleton.Allocate(
                    uiNewSize <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiNewSize : SIZE_MAX
                );
                std::memset(pvDest, -1, sizeof(uint32_t) * uiNewSize);
                rt::memcpy(pvDest, aIndexToChildLookup, sizeof(uint32_t) * uiIndexToChildLookupSize);
                MemoryManager_Singleton.Deallocate(aIndexToChildLookup);
                aIndexToChildLookup = static_cast<uint32_t*>(pvDest);
                uiIndexToChildLookupSize = uiNewSize;
            }
            else {
                if (aIndexToChildLookup[uiIndex] != static_cast<uint32_t>(-1)) {
                    return;
                }
            }
            // Passes abFirstAvail's low byte, zero-extended.
            NiNode::AttachChild(apChild, static_cast<uint8_t>(abFirstAvail));
            aIndexToChildLookup[uiIndex] = GetChildIndex(apChild);
        }
    }
}

// The manager's children are master particle systems; the child's uiIndex is read.
void BSParticleSystemManager::DetachChild(NiAVObject* apChild, NiPointer<NiAVObject>& arspAVObject) {
    NiPointer<NiAVObject> spChild;
    NiNode::DetachChild(apChild, spChild);
    if (spChild.m_pObject) {
        aIndexToChildLookup[static_cast<BSMasterParticleSystem*>(spChild.m_pObject)->uiIndex] = -1;
    }
    arspAVObject.operator=(spChild);
}

void BSParticleSystemManager::DetachChild(NiAVObject* apChild) {
    NiPointer<NiAVObject> v3;
    DetachChild(apChild, v3);
}

void BSParticleSystemManager::DetachChildAt(uint32_t auiIndex, NiPointer<NiAVObject>& arspAVObject) {
    (&arspAVObject)->operator=(nullptr);
}

void BSParticleSystemManager::DetachChildAt(uint32_t auiIndex) {
    NiPointer<NiAVObject> v3;
    DetachChildAt(auiIndex, v3);
}

void BSParticleSystemManager::SetAt(uint32_t auiIndex, NiAVObject* apChild, NiPointer<NiAVObject>& arspAVObject) {
    arspAVObject.operator=(static_cast<NiAVObject*>(nullptr));
}

void BSParticleSystemManager::SetAt(uint32_t auiIndex, NiAVObject* apChild) {
    NiPointer<NiAVObject> v4;
    SetAt(auiIndex, apChild, v4);
}

BSMasterParticleSystem* BSParticleSystemManager::GetMasterParticleSystem(uint32_t auiIndex) {
    uint32_t v3;
    BSMasterParticleSystem* v4 = nullptr;
    if (auiIndex < uiIndexToChildLookupSize) {
        v3 = aIndexToChildLookup[auiIndex];
        if (v3 != (-1)) {
            return static_cast<BSMasterParticleSystem*>(m_kChildren.m_pBase[v3].m_pObject);
        }
    }
    return v4;
}

void BSParticleSystemManager::AddMasterParticleSystem(BSMasterParticleSystem* apMasterParticleSystem) {
    // Attaches the system as a child, flagged as an auto-generated node.
    AttachChild(apMasterParticleSystem, true);
}

void BSParticleSystemManager::RemoveMasterParticleSystem(uint32_t auiIndex) {
    NiAVObject* pMasterParticleSystem = GetMasterParticleSystem(auiIndex);
    if (pMasterParticleSystem) {
        RemoveMasterParticleSystem(static_cast<BSMasterParticleSystem*>(pMasterParticleSystem));
    }
    else {
        Warning(aDefaultTrying, auiIndex);
    }
}

void BSParticleSystemManager::RemoveMasterParticleSystem(BSMasterParticleSystem* apMasterParticleSystem) {
    NiPointer<NiAVObject> v3;
    DetachChild(apMasterParticleSystem, v3);
}

// The game's tasklet path (an UpdateMasterPsysTaskletData array, one tasklet per child) sits behind a flag that is
// always false, so it never runs and is left out: every child updates on this thread.
void BSParticleSystemManager::UpdateParallel(NiUpdateData& updateData) {
    uint32_t uiCount = m_kChildren.m_usSize;
    for (uint32_t i = 0; i < uiCount; ++i) {
        NiAVObject* pParticle = m_kChildren.m_pBase[i].m_pObject;
        if (pParticle) {
            pParticle->UpdateDownwardPass(updateData, 0);
        }
    }
    UpdateUpwardPass();
}

void BSParticleSystemManager::AddEmitter(uint32_t auiMasterParticleSystemIndex, NiAVObject* apEmitter) {
    BSMasterParticleSystem* MasterParticleSystem = GetMasterParticleSystem(auiMasterParticleSystemIndex);
    if (MasterParticleSystem) {
        MasterParticleSystem->AddEmitterObj(apEmitter);
    }
    else {
        Warning(
            "DEFAULT: Trying to add an emitter (%s) to a master particle system (idx: %d) that hasn't been loaded.",
            apEmitter->m_kName.GetHandle(), auiMasterParticleSystemIndex
        );
    }
}

void BSParticleSystemManager::ClearAllParticles() {
    BSMasterParticleSystem* m_pObject;
    uint32_t i;
    uint32_t m_usSize;
    m_usSize = (uint32_t)m_kChildren.m_usSize;
    for (i = 0; i < m_usSize; ++i) {
        m_pObject = static_cast<BSMasterParticleSystem*>(m_kChildren.m_pBase[i].m_pObject);
        if (m_pObject) {
            m_pObject->ClearAllParticles();
        }
    }
}

uint32_t BSParticleSystemManager::GetChildIndex(NiAVObject* apChild) {
    uint32_t i;
    for (i = 0; i < ((uint32_t)((int32_t)m_kChildren.m_usESize)); ++i) {
        if (m_kChildren.m_pBase[i].m_pObject == apChild) {
            return i;
        }
    }
    return (uint32_t)(-1);
}

void BSParticleSystemManager::CopyMembers(BSParticleSystemManager* pDest, NiCloningProcess& arCloneData) {
    NiNode::CopyMembers(pDest, arCloneData);
}

bool BSParticleSystemManager::CheckEmitterRecurse(NiAVObject* apObject, BSMasterParticleSystem* apMasterSystem) {
    if (!apObject) {
        return false;
    }
    if (apObject->IsParticlesGeom()) {
        if (apObject->GetRTTI() == &NiParticleSystem::ms_RTTI) {
            if (!static_cast<NiParticleSystem*>(apObject)->m_bWorldSpace) {
                Warning(
                    "DEFAULT: \"%s\" is a Master Particle System not emitting in world space.",
                    apObject->m_kName.GetHandle()
                );
                return false;
            }
            for (BSPSysMultiTargetEmitterCtlr* i =
                     static_cast<BSPSysMultiTargetEmitterCtlr*>(apObject->m_spControllers);
                 i; i = static_cast<BSPSysMultiTargetEmitterCtlr*>(i->m_spNext.m_pObject)) {
                if (i->GetRTTI() == &BSPSysMultiTargetEmitterCtlr::ms_RTTI) {
                    NiPSysModifier* m_pkModifier = i->m_pkModifier;
                    if (!apMasterSystem || i->pMasterParticleSystem == apMasterSystem) {
                        char v4;
                        if (m_pkModifier) {
                            char v5;
                            for (const NiRTTI* j = m_pkModifier->GetRTTI(); j; j = j->GetBaseRTTI()) {
                                if (j == &NiPSysVolumeEmitter::ms_RTTI) {
                                    v5 = 1;
                                    goto LABEL_21;
                                }
                            }
                            v5 = 0;
                        LABEL_21:;
                            v4 = v5;
                        }
                        else {
                            v4 = 0;
                        }
                        return v4 != 0;
                    }

                    Warning(
                        "DEFAULT: \"%s\" is a Master Particle System not pointing to the proper root.",
                        apObject->m_kName.GetHandle()
                    );
                    return false;
                }
            }
        }
        Warning(
            "DEFAULT: \"%s\" is a Master Particle System without a multi-target emitter.", apObject->m_kName.GetHandle()
        );
        return false;
    }

    NiNode* pnode = apObject->IsNode();
    if (pnode) {
        uint32_t m_usSize = pnode->m_kChildren.m_usSize;
        for (uint32_t k = 0; k < m_usSize; ++k) {
            NiAVObject* pchild = pnode->m_kChildren.m_pBase[k].m_pObject;
            if (pchild && !BSParticleSystemManager::CheckEmitterRecurse(pchild, apMasterSystem)) {
                return false;
            }
        }
    }
    return true;
}

bool BSParticleSystemManager::IsValidMasterParticleSystem(NiAVObject* apObject) {
    uint8_t v2;
    int8_t v3 = 0;
    if (apObject) {
        v2 = (uint8_t)(static_cast<const void*>(apObject->GetRTTI()) ==
                       static_cast<const void*>(&BSMasterParticleSystem::ms_RTTI));
    }
    else {
        v2 = 0;
    }
    if (v2) {
        return (bool)((uint8_t)BSParticleSystemManager::CheckEmitterRecurse(
            apObject, static_cast<BSMasterParticleSystem*>(apObject)
        ));
    }
    return (bool)v3;
}

bool BSParticleSystemManager::ReplaceEmitterRecurse(NiAVObject* apObject, BSMasterParticleSystem* apMasterSystem) {
    if (!apObject) {
        return false;
    }
    if (apObject->IsParticlesGeom()) {
        if (apObject->GetRTTI() == &NiParticleSystem::ms_RTTI) {
            static_cast<NiParticleSystem*>(apObject)->m_bWorldSpace = true;
            for (BSPSysMultiTargetEmitterCtlr* i =
                     static_cast<BSPSysMultiTargetEmitterCtlr*>(apObject->m_spControllers);
                 i; i = static_cast<BSPSysMultiTargetEmitterCtlr*>(i->m_spNext.m_pObject)) {
                if (i->GetRTTI() == &BSPSysMultiTargetEmitterCtlr::ms_RTTI) {
                    i->pMasterParticleSystem = apMasterSystem;
                    return true;
                }
            }
        }
        Warning(
            "DEFAULT: \"%s\" is a Master Particle System without a multi-target emitter.", apObject->m_kName.GetHandle()
        );
        return false;
    }

    NiNode* pnode = apObject->IsNode();
    if (pnode) {
        uint32_t ucount = pnode->m_kChildren.m_usSize;
        for (uint32_t uindex = 0; uindex < ucount; ++uindex) {
            NiAVObject* pchild = pnode->m_kChildren.m_pBase[uindex].m_pObject;
            if (pchild && !BSParticleSystemManager::ReplaceEmitterRecurse(pchild, apMasterSystem)) {
                return false;
            }
        }
    }
    return true;
}

BSMasterParticleSystem* BSParticleSystemManager::MakeMasterParticleSystem(NiNode* apBaseNode) {
    BSMasterParticleSystem* apMasterSystem = nullptr;
    if (BSParticleSystemManager::CheckEmitterRecurse(apBaseNode, nullptr)) {
        BSMasterParticleSystem* v2;
        BSMasterParticleSystem* apMemory =
            static_cast<BSMasterParticleSystem*>(NiMemObject::operator new(sizeof(BSMasterParticleSystem)));
        if (apMemory) {
            v2 = ::new (apMemory) BSMasterParticleSystem;
        }
        else {
            v2 = nullptr;
        }
        apMasterSystem = v2;
        BSParticleSystemManager::ReplaceEmitterRecurse(apBaseNode, v2);
        apMasterSystem->AttachChild(apBaseNode, 1);
        v2->UpdateParticleSystemList();
    }
    return apMasterSystem;
}

// The base constructors set the tasklet fields; pObj and pUpdateData are deliberately left unset, as the game does.
UpdateMasterPsysTaskletData::UpdateMasterPsysTaskletData() {}

UpdateMasterPsysTaskletData::~UpdateMasterPsysTaskletData() {}

void UpdateMasterPsysTaskletData::Process() {
    pObj->UpdateDownwardPass(*pUpdateData, 0);
}

void UpdateMasterPsysTaskletData::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
