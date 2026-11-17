#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysColliderManager.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysCollider.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysColliderManager::ms_RTTI("NiPSysColliderManager", &NiPSysModifier::ms_RTTI);

NiPSysColliderManager::NiPSysColliderManager() : NiPSysModifier(), m_spColliders() {}

NiPSysColliderManager::~NiPSysColliderManager() {}

NiObject* NiPSysColliderManager::CreateClone(NiCloningProcess& kCloning) {
    NiPSysColliderManager* pkObject =
        static_cast<NiPSysColliderManager*>(NiMemObject::operator new(sizeof(NiPSysColliderManager)));
    if (pkObject) {
        ::new (pkObject) NiPSysColliderManager;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

// The collider list's link id is only read here; LinkObject resolves it. NiPSysColorModifier shares this body.
void NiPSysColliderManager::LoadBinary(NiStream& kStream) {
    NiPSysModifier::LoadBinary(kStream);
    kStream.ReadLinkID();
}

// The link id is cast without a type check, as the game does.
void NiPSysColliderManager::LinkObject(NiStream& kStream) {
    NiPSysModifier::LinkObject(kStream);
    m_spColliders = static_cast<NiPSysCollider*>(kStream.GetObjectFromLinkID());
}

// Registers the collider list; its result is ignored. NiPSysColorModifier shares this body.
bool NiPSysColliderManager::RegisterStreamables(NiStream& kStream) {
    if (!NiPSysModifier::RegisterStreamables(kStream)) {
        return false;
    }

    if (m_spColliders) {
        m_spColliders->RegisterStreamables(kStream);
    }

    return true;
}

// NiPSysColorModifier shares this body.
void NiPSysColliderManager::SaveBinary(NiStream& kStream) {
    NiPSysModifier::SaveBinary(kStream);
    kStream.SaveLinkID(m_spColliders);
}

// The collider lists are compared with IsEqual.
bool NiPSysColliderManager::IsEqual(NiObject* pkObject) {
    if (!NiPSysModifier::IsEqual(pkObject)) {
        return false;
    }

    NiPSysColliderManager* pkOther = static_cast<NiPSysColliderManager*>(pkObject);
    if ((m_spColliders && !pkOther->m_spColliders) || (!m_spColliders && pkOther->m_spColliders)) {
        return false;
    }

    if (m_spColliders && pkOther->m_spColliders && !m_spColliders->IsEqual(pkOther->m_spColliders)) {
        return false;
    }

    return true;
}

// The collider list is not checked for null.
void NiPSysColliderManager::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifier::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    m_spColliders->GetViewerStrings(pkStrings);
}

// The colliders fix up their own clones.
void NiPSysColliderManager::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysModifier::ProcessClone(kCloning);

    if (m_spColliders) {
        m_spColliders->ProcessClone(kCloning);
    }
}

// Prepares the collider list once, then resolves each active particle from the system's last update time to
// afTime. The first collider hit updates the particle, and its collision time becomes the particle's last update;
// without a hit the last update is the system's last update time. The active count is re-read every iteration,
// since a collider may remove the particle.
void NiPSysColliderManager::Update(float fTime, NiPSysData* pkData) {
    if (m_spColliders && pkData->m_usActive > 0) {
        m_spColliders->SetResolveConstants();
    }

    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        float fInitialTime = m_pkTarget->m_fLastTime;
        float fCollisionTime = fTime;

        NiPSysCollider* pkCollider = nullptr;
        if (m_spColliders) {
            pkCollider = m_spColliders->Resolve(fInitialTime, fCollisionTime, pkData, i);
        }

        if (pkCollider) {
            pkCollider->Update(fTime, pkData, i);
        }
        else {
            fCollisionTime = fInitialTime;
        }

        pkData->m_pkParticleInfo[i].m_fLastUpdate = fCollisionTime;
    }
}

void NiPSysColliderManager::CopyMembers(NiPSysColliderManager* pkDest, NiCloningProcess& kCloning) {
    NiPSysModifier::CopyMembers(pkDest, kCloning);
    NiPSysCollider* m_pObject = m_spColliders.m_pObject;
    if (m_pObject) {
        NiObject* v5 = m_pObject->CreateClone(kCloning);
        pkDest->m_spColliders = static_cast<NiPSysCollider*>(v5);
    }
}

NiObject* NiPSysColliderManager::CreateObject() {
    NiPSysColliderManager* pkObject =
        static_cast<NiPSysColliderManager*>(NiMemObject::operator new(sizeof(NiPSysColliderManager)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiPSysColliderManager;
}
