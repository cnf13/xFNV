#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysRadialFieldModifier.h"

#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysRadialFieldModifier::ms_RTTI("NiPSysRadialFieldModifier", &NiPSysFieldModifier::ms_RTTI);

NiPSysRadialFieldModifier::NiPSysRadialFieldModifier() : NiPSysFieldModifier(), m_fRadialType(0.0f) {}

NiPSysRadialFieldModifier::~NiPSysRadialFieldModifier() {}

NiObject* NiPSysRadialFieldModifier::CreateClone(NiCloningProcess& kCloning) {
    NiPSysRadialFieldModifier* pkObject =
        static_cast<NiPSysRadialFieldModifier*>(NiMemObject::operator new(sizeof(NiPSysRadialFieldModifier)));
    if (pkObject) {
        ::new (pkObject) NiPSysRadialFieldModifier;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }
    else {
        CopyMembers(nullptr, kCloning);
        return nullptr;
    }
}

void NiPSysRadialFieldModifier::LoadBinary(NiStream& kStream) {
    NiPSysFieldModifier::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_fRadialType);
}

void NiPSysRadialFieldModifier::LinkObject(NiStream& kStream) {
    NiPSysFieldModifier::LinkObject(kStream);
}

bool NiPSysRadialFieldModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysFieldModifier::RegisterStreamables(kStream);
}

// NiPSysTurbulenceFieldModifier uses the same function for its frequency.
void NiPSysRadialFieldModifier::SaveBinary(NiStream& kStream) {
    NiPSysFieldModifier::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_fRadialType);
}

// The radial type is not compared.
bool NiPSysRadialFieldModifier::IsEqual(NiObject* pkObject) {
    return NiPSysFieldModifier::IsEqual(pkObject);
}

void NiPSysRadialFieldModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysFieldModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysRadialFieldModifier::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("RadialType", m_fRadialType));
}

// Particles are pushed along their offset from the field object; those whose last update is at afTime are skipped.
void NiPSysRadialFieldModifier::Update(float fTime, NiPSysData* pkData) {
    if (m_fMagnitude == 0.0f) {
        return;
    }
    if (!pkData->m_usActive) {
        return;
    }
    if (!m_pkFieldObj) {
        return;
    }
    if (!m_bUseMaxDistance && (m_fAttenuation == 0.0f || m_fRadialType == 0.0f)) {
        UpdateNoAttenuationNoDirNoDist(fTime, pkData);
        return;
    }

    // The field transform is held 8-aligned, as the game does.
    alignas(8) NiTransform kField = m_pkFieldObj->m_kWorld;
    NiTransform kPSys = m_pkTarget->m_kWorld;
    NiTransform kInvPSys;
    kPSys.Invert(kInvPSys);
    NiTransform kFieldToPSys = kInvPSys * kField;

    float fHalfAttenuation = m_fAttenuation * 0.5;

    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
        float fDeltaT = fTime - pkInfo->m_fLastUpdate;
        if (fDeltaT == 0.0f) {
            continue;
        }

        NiPoint3 kPos = pkData->m_pkVertex[i];
        float fDiffX = kPos.x - kFieldToPSys.m_Translate.x;
        float fDiffY = kPos.y - kFieldToPSys.m_Translate.y;
        float fDiffZ = kPos.z - kFieldToPSys.m_Translate.z;
        float fDistSqr = fDiffX * fDiffX + fDiffY * fDiffY + fDiffZ * fDiffZ;
        if (m_bUseMaxDistance && !(m_fMaxDistanceSqr >= fDistSqr)) {
            continue;
        }

        float fScale;
        if (m_fAttenuation == 0.0f || fDistSqr == 0.0f) {
            fScale = fDeltaT * m_fMagnitude;
        }
        else {
            // The CRT's __CIpow, both arguments on the x87 stack.
            float fPow = std::pow(fDistSqr, fHalfAttenuation);
            float fFalloff = fPow * m_fRadialType;
            fScale = fDeltaT * m_fMagnitude / BSMAX(1.0f, fFalloff);
        }
        float fAddX = fDiffX * fScale;
        float fAddY = fDiffY * fScale;
        float fAddZ = fDiffZ * fScale;
        pkInfo->m_kVelocity.x += fAddX;
        pkInfo->m_kVelocity.y += fAddY;
        pkInfo->m_kVelocity.z += fAddZ;
    }
}

// The position pointer only advances past particles that are updated, so after a particle whose last update is
// at afTime the later particles read their predecessors' positions.
void NiPSysRadialFieldModifier::UpdateNoAttenuationNoDirNoDist(float fTime, NiPSysData* pkData) {
    NiTransform kField = m_pkFieldObj->m_kWorld;
    NiTransform kPSys = m_pkTarget->m_kWorld;
    NiTransform kInvPSys;
    kPSys.Invert(kInvPSys);
    NiTransform kFieldToPSys = kInvPSys * kField;

    float fMagnitude = m_fMagnitude;
    const NiPoint3* pkPos = pkData->m_pkVertex;
    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
        float fDeltaT = fTime - pkInfo->m_fLastUpdate;
        if (fDeltaT == 0.0f) {
            continue;
        }

        float fDiffX = pkPos->x - kFieldToPSys.m_Translate.x;
        float fDiffY = pkPos->y - kFieldToPSys.m_Translate.y;
        float fDiffZ = pkPos->z - kFieldToPSys.m_Translate.z;
        pkPos++;
        float fScale = fDeltaT * fMagnitude;
        float fAddX = fDiffX * fScale;
        float fAddY = fDiffY * fScale;
        float fAddZ = fDiffZ * fScale;
        pkInfo->m_kVelocity.x += fAddX;
        pkInfo->m_kVelocity.y += fAddY;
        pkInfo->m_kVelocity.z += fAddZ;
    }
}

NiObject* NiPSysRadialFieldModifier::CreateObject() {
    NiPSysRadialFieldModifier* pkObject =
        static_cast<NiPSysRadialFieldModifier*>(NiMemObject::operator new(sizeof(NiPSysRadialFieldModifier)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiPSysRadialFieldModifier;
}
