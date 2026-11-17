#include "Gamebryo2.2/CoreLibs/NiParticle/NiMeshParticleSystem.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiMeshPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysMeshEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysMeshUpdateModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysUpdateCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMessageBox.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cmath>

NiRTTI NiMeshParticleSystem::ms_RTTI("NiMeshParticleSystem", &NiParticleSystem::ms_RTTI);

NiMeshParticleSystem::NiMeshParticleSystem() : m_fTime(0.0f), m_bUpdateControllers(true) {}

NiMeshParticleSystem::~NiMeshParticleSystem() {}

NiObject* NiMeshParticleSystem::CreateClone(NiCloningProcess& kCloning) {
    NiMeshParticleSystem* pkObject_2 =
        static_cast<NiMeshParticleSystem*>(NiMemObject::operator new(sizeof(NiMeshParticleSystem)));
    if (pkObject_2) {
        ::new (pkObject_2) NiMeshParticleSystem;
    }
    else {
        pkObject_2 = 0;
    }
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_fTime = m_fTime;
    pkObject_2->m_bUpdateControllers = m_bUpdateControllers;
    return pkObject_2;
}

// Bethesda's warning: every file but the two rain meshes is reported through the message box hook, then loaded anyway.
// The file name buffer is also tested for null.
void NiMeshParticleSystem::LoadBinary(NiStream& kStream) {
    const char* pcFileName = kStream.m_acFileName;
    if (!pcFileName ||
        (NiStricmp(pcFileName, "Meshes\\Sky\\RainHeavy.NIF") && NiStricmp(pcFileName, "Meshes\\Sky\\RainLight.NIF"))) {
        const char* pcAuthor = kStream.m_BSStreamHeader.cAuthor;
        if (!kStream.m_BSStreamHeader.cAuthor[0]) {
            pcAuthor = "Please";
        }
        char acMessage[260];
        std::sprintf(acMessage, "File %s contains a mesh particle system! %s remove it.", pcFileName, pcAuthor);
        if (NiMessageBox::ms_pfnMessageBox) {
            NiMessageBox::ms_pfnMessageBox(acMessage, "Unsupported Object", nullptr);
        }
    }
    NiParticleSystem::LoadBinary(kStream);
}

void NiMeshParticleSystem::LinkObject(NiStream& kStream) {
    NiParticleSystem::LinkObject(kStream);
}

bool NiMeshParticleSystem::RegisterStreamables(NiStream& kStream) {
    return NiParticleSystem::RegisterStreamables(kStream);
}

void NiMeshParticleSystem::SaveBinary(NiStream& kStream) {
    NiParticleSystem::SaveBinary(kStream);
}

bool NiMeshParticleSystem::IsEqual(NiObject* pkObject) {
    return NiParticleSystem::IsEqual(pkObject);
}

void NiMeshParticleSystem::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiParticleSystem::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The mesh container node follows the system's selective update flags with transforms forced on, and is never rigid.
void NiMeshParticleSystem::SetSelectiveUpdateFlags(bool& bSelectiveUpdate, int32_t, bool& bRigid) {
    NiParticleSystem::SetSelectiveUpdateFlags(bSelectiveUpdate, true, bRigid);

    NiMeshPSysData* pkData = static_cast<NiMeshPSysData*>(m_spModelData.m_pObject);
    pkData->m_spParticleMeshes.m_pObject->SetSelectiveUpdateFlags(bSelectiveUpdate, true, bRigid);
    bRigid = false;
    NiNode* pkMeshes = pkData->m_spParticleMeshes.m_pObject;
    pkMeshes->m_uFlags = NiFlags(pkMeshes->m_uFlags & ~SELECTIVE_UPDATE_RIGID);
}

// Property controllers run first, then every controller but the NiPSysUpdateCtlr (the last one found), which runs
// after the world data so that the meshes see this frame's transform. The time and controller flag are kept for the
// mesh update modifier.
void NiMeshParticleSystem::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t) {
    if (UpdateData.bParallelUpdate) {
        UpdateData.bFoundParticles = true;
        return;
    }

    NiTimeController* pkUpdateCtlr = nullptr;
    if (UpdateData.bUpdateControllers) {
        NiTListItem<NiPointer<NiProperty>>* kPos = m_kPropertyList.GetHeadPos();
        while (kPos) {
            NiProperty* pkProperty = m_kPropertyList.GetNext(kPos);
            if (pkProperty && pkProperty->m_spControllers) {
                pkProperty->Update(UpdateData);
            }
        }

        for (NiTimeController* pkCtlr = m_spControllers; pkCtlr; pkCtlr = pkCtlr->m_spNext.m_pObject) {
            if (pkCtlr->IsKindOf(&NiPSysUpdateCtlr::ms_RTTI)) {
                pkUpdateCtlr = pkCtlr;
            }
            else {
                pkCtlr->Update(UpdateData);
            }
        }

        UpdateWorldData(UpdateData);
        if (pkUpdateCtlr) {
            pkUpdateCtlr->Update(UpdateData);
        }
    }
    else {
        UpdateWorldData(UpdateData);
    }
    UpdateWorldBound();

    m_fTime = UpdateData.fTime;
    m_bUpdateControllers = UpdateData.bUpdateControllers;
}

// UpdateRigidDownwardPass uses the same function. The parallel check is repeated in the base pass.
void NiMeshParticleSystem::UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    if (UpdateData.bParallelUpdate) {
        UpdateData.bFoundParticles = true;
        return;
    }

    NiParticleSystem::UpdateSelectedDownwardPass(UpdateData, uFlags);
    m_fTime = UpdateData.fTime;
}

void NiMeshParticleSystem::UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    if (UpdateData.bParallelUpdate) {
        UpdateData.bFoundParticles = true;
        return;
    }

    NiParticleSystem::UpdateSelectedDownwardPass(UpdateData, uFlags);
    m_fTime = UpdateData.fTime;
}

// The meshes inherit this geometry's property state.
void NiMeshParticleSystem::UpdatePropertiesDownward(NiPropertyState* pkParentState) {
    NiParticleSystem::UpdatePropertiesDownward(pkParentState);

    static_cast<NiMeshPSysData*>(m_spModelData.m_pObject)
        ->m_spParticleMeshes.m_pObject->UpdatePropertiesDownward(&m_PropertyState);
}

// The mesh container's local transform becomes this system's world transform (with the scale made positive), and
// the container is updated from there.
void NiMeshParticleSystem::UpdateWorldData(NiUpdateData& UpdateData) {
    NiParticleSystem::UpdateWorldData(UpdateData);

    NiMeshPSysData* pkData = static_cast<NiMeshPSysData*>(m_spModelData.m_pObject);
    pkData->m_spParticleMeshes.m_pObject->m_kLocal.m_Rotate = m_kWorld.m_Rotate;
    pkData->m_spParticleMeshes.m_pObject->m_kLocal.m_Translate = m_kWorld.m_Translate;
    float fScale = std::fabs(m_kWorld.m_fScale);
    NiNode* pkMeshes = pkData->m_spParticleMeshes.m_pObject;
    pkMeshes->m_kLocal.m_fScale = fScale;
    pkMeshes->UpdateWorldData(UpdateData);
}

// The world bound is the mesh container's (null bound: NullBoundS).
void NiMeshParticleSystem::UpdateWorldBound() {
    NiParticleSystem::UpdateWorldBound();

    NiNode* pkMeshes = static_cast<NiMeshPSysData*>(m_spModelData.m_pObject)->m_spParticleMeshes.m_pObject;
    NiBound* pkBound = pkMeshes->m_kWorldBound ? pkMeshes->m_kWorldBound : &NiAVObject::NullBoundS;
    *m_kWorldBound = *pkBound;
}

// Only the mesh container is drawn.
void NiMeshParticleSystem::OnVisible(NiCullingProcess& kCuller) {
    static_cast<NiMeshPSysData*>(m_spModelData.m_pObject)->m_spParticleMeshes.m_pObject->OnVisible(kCuller);
}

// Also purges the pooled meshes and the meshes held by the mesh emitters and mesh update modifiers. Emitter meshes
// are counted by occupied slots, update modifier meshes by size.
void NiMeshParticleSystem::PurgeRendererData(NiRenderer* pkRenderer) {
    NiParticleSystem::PurgeRendererData(pkRenderer);

    NiMeshPSysData* pkData = static_cast<NiMeshPSysData*>(m_spModelData.m_pObject);
    if (pkData) {
        if (pkData->m_spParticleMeshes.m_pObject) {
            pkData->m_spParticleMeshes.m_pObject->PurgeRendererData(pkRenderer);
        }
        pkData->PurgeRendererDataForPools(pkRenderer);
    }

    NiTListItem<NiPointer<NiPSysModifier>>* kPos = m_kModifierList.GetHeadPos();
    while (kPos) {
        NiPSysModifier* pkModifier = m_kModifierList.GetNext(kPos);
        if (!pkModifier) {
            continue;
        }
        if (pkModifier->IsKindOf(&NiPSysMeshEmitter::ms_RTTI)) {
            NiPSysMeshEmitter* pkEmitter = static_cast<NiPSysMeshEmitter*>(pkModifier);
            uint32_t uiCount = pkEmitter->m_kGeomEmitterArray.GetEffectiveSize();
            for (uint32_t i = 0; i < uiCount; i++) {
                if (i < pkEmitter->m_kGeomEmitterArray.GetEffectiveSize()) {
                    NiGeometry* pkGeometry = pkEmitter->m_kGeomEmitterArray.m_pBase[i].m_pObject;
                    if (pkGeometry) {
                        pkGeometry->PurgeRendererData(pkRenderer);
                    }
                }
            }
        }
        else if (pkModifier->IsKindOf(&NiPSysMeshUpdateModifier::ms_RTTI)) {
            NiPSysMeshUpdateModifier* pkUpdate = static_cast<NiPSysMeshUpdateModifier*>(pkModifier);
            uint32_t uiCount = pkUpdate->m_kMeshes.GetSize();
            for (uint32_t i = 0; i < uiCount; i++) {
                if (i < pkUpdate->m_kMeshes.GetSize()) {
                    NiAVObject* pkMesh = pkUpdate->m_kMeshes.m_pBase[i].m_pObject;
                    if (pkMesh) {
                        pkMesh->PurgeRendererData(pkRenderer);
                    }
                }
            }
        }
    }
}

void NiMeshParticleSystem::SetModelData(NiGeometryData* pkModelData) {
    NiParticleSystem::SetModelData(pkModelData);
}

NiObject* NiMeshParticleSystem::CreateObject() {
    NiMeshParticleSystem* pkObject =
        static_cast<NiMeshParticleSystem*>(NiMemObject::operator new(sizeof(NiMeshParticleSystem)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiMeshParticleSystem;
}

void NiMeshPSysData::PurgeRendererDataForPools(NiRenderer* pkRenderer) {
    uint32_t uiNumGenerations = m_kPools.GetSize();
    for (uint32_t i = 0; i < uiNumGenerations; i++) {
        NiTObjectArray<NiPointer<NiAVObject>>* pkPool = m_kPools.GetAt(i);
        if (pkPool) {
            uint32_t uiPoolSize = pkPool->GetSize();
            for (uint32_t j = 0; j < uiPoolSize; j++) {
                NiAVObject* pkObject = pkPool->m_pBase[j];
                if (pkObject) {
                    pkObject->PurgeRendererData(pkRenderer);
                }
            }
        }
    }
}
