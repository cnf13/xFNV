#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAgeDeathModifier.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSpawnModifier.h"

NiRTTI NiPSysAgeDeathModifier::ms_RTTI("NiPSysAgeDeathModifier", &NiPSysModifier::ms_RTTI);

NiPSysAgeDeathModifier::NiPSysAgeDeathModifier() : m_bSpawnOnDeath(false), m_pkSpawnModifier(nullptr) {}

NiPSysAgeDeathModifier::~NiPSysAgeDeathModifier() {}

NiObject* NiPSysAgeDeathModifier::CreateClone(NiCloningProcess& kCloning) {
    NiPSysAgeDeathModifier* pkObject_2 = new NiPSysAgeDeathModifier;
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_bSpawnOnDeath = m_bSpawnOnDeath;
    return pkObject_2;
}

// A short read leaves ucSpawnOnDeath unwritten and it is stored anyway: an uninitialised read, kept deliberately.
void NiPSysAgeDeathModifier::LoadBinary(NiStream& kStream) {
    NiPSysModifier::LoadBinary(kStream);

    uint8_t ucSpawnOnDeath;
    NiStreamLoadBinary(kStream, ucSpawnOnDeath);
    m_bSpawnOnDeath = ucSpawnOnDeath != 0;
    kStream.ReadLinkID();
}

// The link is taken as is, without a type check.
void NiPSysAgeDeathModifier::LinkObject(NiStream& kStream) {
    NiPSysModifier::LinkObject(kStream);
    m_pkSpawnModifier = static_cast<NiPSysSpawnModifier*>(kStream.GetObjectFromLinkID());
}

bool NiPSysAgeDeathModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifier::RegisterStreamables(kStream);
}

void NiPSysAgeDeathModifier::SaveBinary(NiStream& kStream) {
    NiPSysModifier::SaveBinary(kStream);

    uint8_t ucSpawnOnDeath = m_bSpawnOnDeath;
    NiStreamSaveBinary(kStream, ucSpawnOnDeath);
    kStream.SaveLinkID(m_pkSpawnModifier);
}

// Spawn modifiers compare by value, the other's asked first.
bool NiPSysAgeDeathModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysModifier::IsEqual(pkObject)) {
        return false;
    }

    NiPSysAgeDeathModifier* pkOther = static_cast<NiPSysAgeDeathModifier*>(pkObject);
    if (pkOther->m_bSpawnOnDeath != m_bSpawnOnDeath) {
        return false;
    }
    if ((pkOther->m_pkSpawnModifier && !m_pkSpawnModifier) || (!pkOther->m_pkSpawnModifier && m_pkSpawnModifier)) {
        return false;
    }
    if (pkOther->m_pkSpawnModifier && m_pkSpawnModifier && !pkOther->m_pkSpawnModifier->IsEqual(m_pkSpawnModifier)) {
        return false;
    }
    return true;
}

void NiPSysAgeDeathModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysAgeDeathModifier::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Spawn on Death", m_bSpawnOnDeath));
    pkStrings->Add(NiGetViewerString(
        "Spawn Modifier",
        m_pkSpawnModifier ? static_cast<const char*>(m_pkSpawnModifier->m_kName) : "None"
    ));
}

// The clone points at the clone of the spawn modifier, or at the original when that was not cloned.
void NiPSysAgeDeathModifier::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysModifier::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiPSysAgeDeathModifier* pkClone = static_cast<NiPSysAgeDeathModifier*>(pkObject);
    if (m_pkSpawnModifier) {
        if (kCloning.m_pkCloneMap->GetAt(m_pkSpawnModifier, pkObject)) {
            pkClone->m_pkSpawnModifier = static_cast<NiPSysSpawnModifier*>(pkObject);
        }
        else {
            pkClone->m_pkSpawnModifier = m_pkSpawnModifier;
        }
    }
}

// Ages every active particle, last first, and removes those past their life span. With spawning on, each dying particle
// first spawns at the moment it died.
void NiPSysAgeDeathModifier::Update(float fTime, NiPSysData* pkData) {
    if (m_bSpawnOnDeath && m_pkSpawnModifier) {
        uint16_t i = pkData->m_usActive;
        uint32_t uiIndex = static_cast<uint16_t>(i - 1);
        for (; i > 0; i--, uiIndex += 0xFFFF) {
            uint16_t usIndex = static_cast<uint16_t>(uiIndex);
            NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[usIndex];
            float fAge = fTime - pkInfo->m_fLastUpdate + pkInfo->m_fAge;
            pkInfo->m_fAge = fAge;
            if (pkInfo->m_fLifeSpan < fAge) {
                float fSpawnTime = pkInfo->m_fLifeSpan + fTime - fAge;
                m_pkSpawnModifier->SpawnParticles(fTime, fSpawnTime, static_cast<int32_t>(uiIndex), m_pkTarget);
                pkData->RemoveParticle(static_cast<int32_t>(uiIndex));
            }
        }
    }
    else {
        uint16_t i = pkData->m_usActive;
        uint32_t uiIndex = static_cast<uint16_t>(i - 1);
        for (; i > 0; i--, uiIndex += 0xFFFF) {
            uint16_t usIndex = static_cast<uint16_t>(uiIndex);
            NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[usIndex];
            float fAge = fTime - pkInfo->m_fLastUpdate + pkInfo->m_fAge;
            pkInfo->m_fAge = fAge;
            if (pkInfo->m_fLifeSpan < fAge) {
                pkData->RemoveParticle(static_cast<int32_t>(uiIndex));
            }
        }
    }
}

NiObject* NiPSysAgeDeathModifier::CreateObject() {
    return new NiPSysAgeDeathModifier;
}
