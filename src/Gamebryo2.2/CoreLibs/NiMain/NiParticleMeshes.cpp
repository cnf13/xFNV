#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleMeshes.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleMeshesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "SDK/MSVC/string.h"

#include <cmath>

NiRTTI NiParticleMeshes::ms_RTTI("NiParticleMeshes", &NiParticles::ms_RTTI);

// The controllers update; the time is left unset for the stream load, as the game does.
NiParticleMeshes::NiParticleMeshes() : m_bUpdateControllers(true) {}

NiParticleMeshes::~NiParticleMeshes() {}

NiObject* NiParticleMeshes::CreateClone(NiCloningProcess& kCloning) {
    NiParticleMeshes* pkObject = static_cast<NiParticleMeshes*>(NiMemObject::operator new(sizeof(NiParticleMeshes)));
    if (pkObject) {
        ::new (pkObject) NiParticleMeshes;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }
    else {
        CopyMembers(nullptr, kCloning);
        return nullptr;
    }
}

void NiParticleMeshes::LoadBinary(NiStream& kStream) {
    NiParticles::LoadBinary(kStream);
}

void NiParticleMeshes::LinkObject(NiStream& kStream) {
    NiParticles::LinkObject(kStream);
}

bool NiParticleMeshes::RegisterStreamables(NiStream& kStream) {
    return NiParticles::RegisterStreamables(kStream);
}

void NiParticleMeshes::SaveBinary(NiStream& kStream) {
    NiParticles::SaveBinary(kStream);
}

bool NiParticleMeshes::IsEqual(NiObject* pkObject) {
    return NiParticles::IsEqual(pkObject);
}

// Adds NiParticles's class name, not NiParticleMeshes's, deliberately, as the game does.
void NiParticleMeshes::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiParticles::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(NiParticles::ms_RTTI.m_pcName));
}

// Both the geometry and the mesh node always get selective transform updates; the node is never rigid.
void NiParticleMeshes::SetSelectiveUpdateFlags(bool& bSelectiveUpdate, int32_t, bool& bRigid) {
    NiGeometry::SetSelectiveUpdateFlags(bSelectiveUpdate, true, bRigid);
    NiNode* pkMeshes = static_cast<NiParticleMeshesData*>(m_spModelData.m_pObject)->m_spParticleMeshes.m_pObject;
    pkMeshes->SetSelectiveUpdateFlags(bSelectiveUpdate, true, bRigid);
    bRigid = false;
    uint32_t& ruFlags = pkMeshes->m_uFlags;
    ruFlags = static_cast<NiAVObject::NiFlags>(ruFlags & ~NiAVObject::SELECTIVE_UPDATE_RIGID);
}

void NiParticleMeshes::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    m_fTime = (&UpdateData)->fTime;
    m_bUpdateControllers = (bool)((uint8_t)(&UpdateData)->bUpdateControllers);
    NiAVObject::UpdateDownwardPass(UpdateData, uFlags);
}

void NiParticleMeshes::UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    m_fTime = (&UpdateData)->fTime;
    NiAVObject::UpdateSelectedDownwardPass(UpdateData, uFlags);
}

void NiParticleMeshes::UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    m_fTime = UpdateData.fTime;
    NiAVObject::UpdateRigidDownwardPass(UpdateData, uFlags);
}

// The mesh node inherits this geometry's property state.
void NiParticleMeshes::UpdatePropertiesDownward(NiPropertyState* pkParentState) {
    NiGeometry::UpdatePropertiesDownward(pkParentState);
    static_cast<NiParticleMeshesData*>(m_spModelData.m_pObject)
        ->m_spParticleMeshes.m_pObject->UpdatePropertiesDownward(&m_PropertyState);
}

void NiParticleMeshes::UpdateWorldData(NiUpdateData& UpdateData) {
    NiAVObject::UpdateWorldData(UpdateData);
    NiParticleMeshesData* pkData = static_cast<NiParticleMeshesData*>(m_spModelData.m_pObject);
    NiNode* m_pObject = pkData->m_spParticleMeshes.m_pObject;
    pkData->UpdateParticles();
    rt::memcpy(&m_pObject->m_kLocal, &m_kWorld, 44u);
    m_pObject->m_kLocal.m_Translate.z = m_kWorld.m_Translate.z;
    float v6 = std::fabs((double)m_kWorld.m_fScale);
    m_pObject->m_kLocal.m_fScale = v6;
    m_pObject->UpdateDownwardPass(UpdateData, 0);
    if (pkData->m_bNeedsPropertyEffectUpdate) {
        UpdateProperties();
        pkData->m_bNeedsPropertyEffectUpdate = false;
    }
}

void NiParticleMeshes::OnVisible(NiCullingProcess& kCuller) {
    static_cast<NiParticleMeshesData*>(m_spModelData.m_pObject)->m_spParticleMeshes.m_pObject->OnVisible(kCuller);
}

void NiParticleMeshes::PurgeRendererData(NiRenderer* pkRenderer) {
    NiGeometry::PurgeRendererData(pkRenderer);
    NiParticleMeshesData* pkData = static_cast<NiParticleMeshesData*>(m_spModelData.m_pObject);
    if (pkData && pkData->m_spParticleMeshes.m_pObject) {
        pkData->m_spParticleMeshes.m_pObject->PurgeRendererData(pkRenderer);
    }
}

void NiParticleMeshes::CopyMembers(NiParticleMeshes* pkDest, NiCloningProcess& kCloning) {
    NiGeometry::CopyMembers(pkDest, kCloning);
}

NiObject* NiParticleMeshes::CreateObject() {
    NiParticleMeshes* pkObject = static_cast<NiParticleMeshes*>(NiMemObject::operator new(sizeof(NiParticleMeshes)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiParticleMeshes;
}
