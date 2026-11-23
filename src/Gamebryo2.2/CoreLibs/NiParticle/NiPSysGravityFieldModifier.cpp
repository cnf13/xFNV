#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysGravityFieldModifier.h"

#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

#include <cmath>

NiRTTI NiPSysGravityFieldModifier::ms_RTTI("NiPSysGravityFieldModifier", &NiPSysFieldModifier::ms_RTTI);

NiPSysGravityFieldModifier::NiPSysGravityFieldModifier()
  : m_kDirection(-NiPoint3::UNIT_Y), m_kUnitDirection(-NiPoint3::UNIT_Y) {
    m_kUnitDirection.Unitize();
}

NiPSysGravityFieldModifier::~NiPSysGravityFieldModifier() {}

NiObject* NiPSysGravityFieldModifier::CreateClone(NiCloningProcess& kCloning) {
    NiPSysGravityFieldModifier* pkObject;
    pkObject = new NiPSysGravityFieldModifier;
    CopyMembers(pkObject, kCloning);
    pkObject->m_kDirection = m_kDirection;
    pkObject->m_kUnitDirection = m_kDirection;
    pkObject->m_kUnitDirection.Unitize();
    return pkObject;
}

void NiPSysGravityFieldModifier::LoadBinary(NiStream& kStream) {
    NiPSysFieldModifier::LoadBinary(kStream);

    m_kDirection.LoadBinary(kStream);
    m_kUnitDirection = m_kDirection;
    m_kUnitDirection.Unitize();
}

void NiPSysGravityFieldModifier::LinkObject(NiStream& kStream) {
    NiPSysFieldModifier::LinkObject(kStream);
}

bool NiPSysGravityFieldModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysFieldModifier::RegisterStreamables(kStream);
}

// The unit direction is computed on load and not streamed.
// NiPSysVortexFieldModifier uses the same function.
void NiPSysGravityFieldModifier::SaveBinary(NiStream& kStream) {
    NiPSysFieldModifier::SaveBinary(kStream);

    m_kDirection.SaveBinary(kStream);
}

// The unit direction is not compared. NiPSysVortexFieldModifier uses the same function.
bool NiPSysGravityFieldModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysFieldModifier::IsEqual(pkObject)) {
        return false;
    }

    NiPSysGravityFieldModifier* pkOther = static_cast<NiPSysGravityFieldModifier*>(pkObject);
    return !(pkOther->m_kDirection != m_kDirection);
}

void NiPSysGravityFieldModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysFieldModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysGravityFieldModifier::ms_RTTI.GetName()));
    pkStrings->Add(m_kDirection.GetViewerString("Direction"));
}

// Particles whose last update is at afTime are skipped.
void NiPSysGravityFieldModifier::Update(float fTime, NiPSysData* pkData) {
    if (m_fMagnitude == 0.0f) {
        return;
    }
    if (!pkData->m_usActive) {
        return;
    }
    if (!m_pkFieldObj) {
        return;
    }
    if (!m_bUseMaxDistance && m_fAttenuation == 0.0f) {
        UpdateNoAttenuationNoDist(fTime, pkData);
        return;
    }

    alignas(8) NiTransform kField = m_pkFieldObj->m_kWorld;
    NiTransform kPSys = m_pkTarget->m_kWorld;
    NiTransform kInvPSys;
    kPSys.Invert(kInvPSys);
    NiTransform kFieldToPSys = kInvPSys * kField;

    float fHalfAttenuation = m_fAttenuation * 0.5;
    NiPoint3 kDir = kFieldToPSys.m_Rotate * m_kUnitDirection;

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
            // The scaled step is kept as a double.
            double dScaledStep = fDeltaT * m_fMagnitude;
            float fPow = std::pow(fDistSqr, fHalfAttenuation);
            fScale = static_cast<float>(dScaledStep / BSMAX(1.0f, fPow));
        }
        float fAddX = kDir.x * fScale;
        float fAddY = kDir.y * fScale;
        float fAddZ = kDir.z * fScale;
        pkInfo->m_kVelocity.x += fAddX;
        pkInfo->m_kVelocity.y += fAddY;
        pkInfo->m_kVelocity.z += fAddZ;
    }
}

// The direction is taken through the field's rotation and unitized once; afTime only measures each step.
void NiPSysGravityFieldModifier::UpdateNoAttenuationNoDist(float fTime, NiPSysData* pkData) {
    NiTransform kField = m_pkFieldObj->m_kWorld;
    NiTransform kPSys = m_pkTarget->m_kWorld;
    NiTransform kInvPSys;
    kPSys.Invert(kInvPSys);
    NiTransform kFieldToPSys = kInvPSys * kField;

    NiPoint3 kDir = kFieldToPSys.m_Rotate * m_kDirection;
    kDir.Unitize();
    float fMagnitude = m_fMagnitude;
    kDir.x = kDir.x * fMagnitude;
    kDir.y = kDir.y * fMagnitude;
    kDir.z = kDir.z * fMagnitude;

    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
        float fDeltaT = fTime - pkInfo->m_fLastUpdate;
        if (fDeltaT == 0.0f) {
            continue;
        }

        float fAddX = kDir.x * fDeltaT;
        float fAddY = kDir.y * fDeltaT;
        float fAddZ = kDir.z * fDeltaT;
        pkInfo->m_kVelocity.x += fAddX;
        pkInfo->m_kVelocity.y += fAddY;
        pkInfo->m_kVelocity.z += fAddZ;
    }
}

NiObject* NiPSysGravityFieldModifier::CreateObject() {
    return new NiPSysGravityFieldModifier;
}
