#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysCollider.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysColliderManager.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSpawnModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysCollider::ms_RTTI("NiPSysCollider", &NiObject::ms_RTTI);

NiPSysCollider::NiPSysCollider(
    float fBounce,
    bool bSpawnOnCollide,
    bool bDieOnCollide,
    NiPSysSpawnModifier* pkSpawnModifier
)
  : m_fBounce(fBounce), m_bSpawnOnCollide(bSpawnOnCollide), m_bDieOnCollide(bDieOnCollide),
    m_pkSpawnModifier(pkSpawnModifier), m_kCollisionPoint(NiPoint3::ZERO), m_fCollisionTime(0.0f), m_pkManager(nullptr),
    m_spNext(nullptr) {}

NiPSysCollider::~NiPSysCollider() {}

// The flags are stored as one byte each; any non-zero byte reads as true. The three link ids (spawn modifier,
// manager, next collider) are only read here; LinkObject resolves them.
// KEEP: each flag byte is read into a stack slot nothing initialised (the first into the kStream argument's slot) and
// then tested, so a short read leaves stack garbage there.
void NiPSysCollider::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_fBounce);
    uint8_t ucSpawnOnCollide;
    NiStreamLoadBinary(kStream, ucSpawnOnCollide);
    m_bSpawnOnCollide = ucSpawnOnCollide != 0;
    uint8_t ucDieOnCollide;
    NiStreamLoadBinary(kStream, ucDieOnCollide);
    m_bDieOnCollide = ucDieOnCollide != 0;
    kStream.ReadLinkID();
    kStream.ReadLinkID();
    kStream.ReadLinkID();
}

// The link ids are cast without a type check.
void NiPSysCollider::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
    m_pkSpawnModifier = static_cast<NiPSysSpawnModifier*>(kStream.GetObjectFromLinkID());
    m_pkManager = static_cast<NiPSysColliderManager*>(kStream.GetObjectFromLinkID());
    m_spNext = static_cast<NiPSysCollider*>(kStream.GetObjectFromLinkID());
}

// Registers the rest of the collider list; its result is ignored.
bool NiPSysCollider::RegisterStreamables(NiStream& kStream) {
    if (!NiObject::RegisterStreamables(kStream)) {
        return false;
    }

    if (m_spNext) {
        m_spNext->RegisterStreamables(kStream);
    }

    return true;
}

// The flags are written from one-byte copies.
void NiPSysCollider::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_fBounce);
    uint8_t ucSpawnOnCollide = m_bSpawnOnCollide;
    NiStreamSaveBinary(kStream, ucSpawnOnCollide);
    uint8_t ucDieOnCollide = m_bDieOnCollide;
    NiStreamSaveBinary(kStream, ucDieOnCollide);
    kStream.SaveLinkID(m_pkSpawnModifier);
    kStream.SaveLinkID(m_pkManager);
    kStream.SaveLinkID(m_spNext);
}

// The spawn modifier and the next collider are compared with IsEqual; the manager only for being set on both.
bool NiPSysCollider::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiPSysCollider* pkOther = static_cast<NiPSysCollider*>(pkObject);
    if (m_fBounce != pkOther->m_fBounce) {
        return false;
    }

    if (m_bSpawnOnCollide != pkOther->m_bSpawnOnCollide) {
        return false;
    }

    if (m_bDieOnCollide != pkOther->m_bDieOnCollide) {
        return false;
    }

    if ((m_pkSpawnModifier && !pkOther->m_pkSpawnModifier) || (!m_pkSpawnModifier && pkOther->m_pkSpawnModifier)) {
        return false;
    }

    if (m_pkSpawnModifier && pkOther->m_pkSpawnModifier && !m_pkSpawnModifier->IsEqual(pkOther->m_pkSpawnModifier)) {
        return false;
    }

    if ((m_pkManager && !pkOther->m_pkManager) || (!m_pkManager && pkOther->m_pkManager)) {
        return false;
    }

    if ((m_spNext && !pkOther->m_spNext) || (!m_spNext && pkOther->m_spNext)) {
        return false;
    }

    if (m_spNext && pkOther->m_spNext && !m_spNext->IsEqual(pkOther->m_spNext)) {
        return false;
    }

    return true;
}

void NiPSysCollider::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Bounce", m_fBounce));
    pkStrings->Add(NiGetViewerString("Spawn on Collide", m_bSpawnOnCollide));
    pkStrings->Add(NiGetViewerString("Die on Collide", m_bDieOnCollide));
    pkStrings->Add(NiGetViewerString(
        "Spawn Modifier",
        m_pkSpawnModifier ? static_cast<const char*>(m_pkSpawnModifier->m_kName) : "None"
    ));
    pkStrings->Add(NiGetViewerString("Manager", static_cast<const void*>(m_pkManager)));

    if (m_spNext) {
        m_spNext->GetViewerStrings(pkStrings);
    }
}

// The clone points at the clones of the spawn modifier and the manager, or at the originals when those were not
// cloned. One out variable serves all three map lookups.
void NiPSysCollider::ProcessClone(NiCloningProcess& kCloning) {
    NiObject::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiPSysCollider* pkClone = static_cast<NiPSysCollider*>(pkObject);

    if (m_pkSpawnModifier) {
        if (kCloning.m_pkCloneMap->GetAt(m_pkSpawnModifier, pkObject)) {
            pkClone->m_pkSpawnModifier = static_cast<NiPSysSpawnModifier*>(pkObject);
        }
        else {
            pkClone->m_pkSpawnModifier = m_pkSpawnModifier;
        }
    }

    if (m_pkManager) {
        if (kCloning.m_pkCloneMap->GetAt(m_pkManager, pkObject)) {
            pkClone->m_pkManager = static_cast<NiPSysColliderManager*>(pkObject);
        }
        else {
            pkClone->m_pkManager = m_pkManager;
        }
    }

    if (m_spNext) {
        m_spNext->ProcessClone(kCloning);
    }
}

// Pure in the vtable, but it has a body: the colliders derived from this one call it last, and it only asks the
// rest of the list.
NiPSysCollider*
NiPSysCollider::Resolve(float fInitialTime, float& fCollisionTime, NiPSysData* pkData, int32_t usCurParticle) {
    if (m_spNext) {
        return m_spNext->Resolve(fInitialTime, fCollisionTime, pkData, usCurParticle);
    }

    return nullptr;
}

// Moves the particle to the collision point, then spawns from it and/or removes it. The manager is not checked for
// null. Both callees get the caller's whole usCurParticle value, not truncated to 16 bits.
void NiPSysCollider::Update(float fTime, NiPSysData* pkData, int32_t usCurParticle) {
    pkData->m_pkVertex[static_cast<uint16_t>(usCurParticle)] = m_kCollisionPoint;

    if (m_bSpawnOnCollide && m_pkSpawnModifier) {
        m_pkSpawnModifier->SpawnParticles(fTime, m_fCollisionTime, usCurParticle, m_pkManager->m_pkTarget);
    }

    if (m_bDieOnCollide) {
        pkData->RemoveParticle(usCurParticle);
    }
}

void NiPSysCollider::CopyMembers(NiPSysCollider* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);
    pkDest->m_fBounce = m_fBounce;
    pkDest->m_bSpawnOnCollide = m_bSpawnOnCollide;
    pkDest->m_bDieOnCollide = m_bDieOnCollide;
    NiPSysCollider* m_pObject = m_spNext.m_pObject;
    if (m_pObject) {
        NiObject* v5 = m_pObject->CreateClone(kCloning);
        pkDest->m_spNext = static_cast<NiPSysCollider*>(v5);
    }
}
