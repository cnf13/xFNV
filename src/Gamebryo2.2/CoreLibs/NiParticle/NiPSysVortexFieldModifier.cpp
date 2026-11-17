#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysVortexFieldModifier.h"

#include "BSMain/BSParentVelocityModifier.h"
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

NiRTTI NiPSysVortexFieldModifier::ms_RTTI("NiPSysVortexFieldModifier", &NiPSysFieldModifier::ms_RTTI);

NiPSysVortexFieldModifier::NiPSysVortexFieldModifier() : NiPSysFieldModifier(), m_kDirection(NiPoint3_ZERO) {}

NiPSysVortexFieldModifier::~NiPSysVortexFieldModifier() {}

NiObject* NiPSysVortexFieldModifier::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiPSysVortexFieldModifier));
    NiPSysVortexFieldModifier* v4 = static_cast<NiPSysVortexFieldModifier*>(v3);
    if (v3) {
        ::new (v3) NiPSysVortexFieldModifier;
    }
    else {
        v4 = nullptr;
    }
    NiPSysFieldModifier::CopyMembers(v4, kCloning);
    v4->m_kDirection.x = m_kDirection.x;
    v4->m_kDirection.y = m_kDirection.y;
    v4->m_kDirection.z = m_kDirection.z;
    return v4;
}

void NiPSysVortexFieldModifier::LoadBinary(NiStream& kStream) {
    NiPSysFieldModifier::LoadBinary(kStream);

    m_kDirection.LoadBinary(kStream);
}

void NiPSysVortexFieldModifier::LinkObject(NiStream& kStream) {
    NiPSysFieldModifier::LinkObject(kStream);
}

bool NiPSysVortexFieldModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysFieldModifier::RegisterStreamables(kStream);
}

void NiPSysVortexFieldModifier::SaveBinary(NiStream& kStream) {
    NiPSysFieldModifier::SaveBinary(kStream);

    m_kDirection.SaveBinary(kStream);
}

bool NiPSysVortexFieldModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysFieldModifier::IsEqual(pkObject)) {
        return false;
    }

    NiPSysVortexFieldModifier* pkOther = static_cast<NiPSysVortexFieldModifier*>(pkObject);
    return !(pkOther->m_kDirection != m_kDirection);
}

void NiPSysVortexFieldModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysFieldModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysVortexFieldModifier::ms_RTTI.GetName()));
    pkStrings->Add(m_kDirection.GetViewerString("Direction"));
}

// Spins particles around the direction through the field object. Particles whose last update is at afTime or
// that sit on the field object are skipped. The attenuation applies to the plain distance, not its square.
void NiPSysVortexFieldModifier::Update(float fTime, NiPSysData* pkData) {
    if (m_fMagnitude == 0.0f) {
        return;
    }
    if (!pkData->m_usActive) {
        return;
    }
    if (m_kDirection == NiPoint3_ZERO) {
        return;
    }
    if (!m_pkFieldObj) {
        return;
    }
    if (m_fAttenuation == 0.0f) {
        if (m_bUseMaxDistance) {
            UpdateNoAttenuation(fTime, pkData);
        }
        else {
            UpdateNoAttenuationNoDist(fTime, pkData);
        }
        return;
    }

    alignas(8) NiTransform kField = m_pkFieldObj->m_kWorld;
    NiTransform kPSys = m_pkTarget->m_kWorld;
    NiTransform kInvPSys;
    kPSys.Invert(kInvPSys);
    NiTransform kFieldToPSys = kInvPSys * kField;

    alignas(8) NiPoint3 kOrigin = kFieldToPSys.m_Translate;
    alignas(8) NiPoint3 kAxis = kFieldToPSys.m_Rotate * m_kDirection;
    kAxis.Unitize();

    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
        float fDeltaT = fTime - pkInfo->m_fLastUpdate;
        if (fDeltaT == 0.0f) {
            continue;
        }

        NiPoint3 kPos = pkData->m_pkVertex[i];
        NiPoint3 kDiff;
        kDiff.x = kPos.x - kFieldToPSys.m_Translate.x;
        kDiff.y = kPos.y - kFieldToPSys.m_Translate.y;
        kDiff.z = kPos.z - kFieldToPSys.m_Translate.z;
        float fDist = kDiff.Length();
        if (fDist == 0.0f) {
            continue;
        }
        if (m_bUseMaxDistance && !(m_fMaxDistance >= fDist)) {
            continue;
        }

        NiPoint3 kForce = VortexDirection(kOrigin, kAxis, kPos);
        float fScale;
        if (m_fAttenuation == 0.0f) {
            fScale = fDeltaT * m_fMagnitude;
        }
        else {
            // The product is kept at double precision.
            double dScaled = fDeltaT * m_fMagnitude;
            float fPow = std::pow(fDist, m_fAttenuation);
            fScale = static_cast<float>(dScaled / fPow);
        }
        kForce *= fScale;
        pkInfo->m_kVelocity += kForce;
    }
}

NiPoint3 NiPSysVortexFieldModifier::VortexDirection(NiPoint3& kVortexPos, NiPoint3& kDirection, NiPoint3& kParticle) {
    NiPoint3 kPerp;
    kPerp.x = kParticle.x - kVortexPos.x;
    kPerp.y = kParticle.y - kVortexPos.y;
    kPerp.z = kParticle.z - kVortexPos.z;
    float fDot = kDirection.x * kPerp.x + kPerp.y * kDirection.y + kPerp.z * kDirection.z;
    float fProjX = kDirection.x * fDot;
    float fProjY = fDot * kDirection.y;
    float fProjZ = fDot * kDirection.z;
    kPerp.x = kPerp.x - fProjX;
    kPerp.y = kPerp.y - fProjY;
    kPerp.z = kPerp.z - fProjZ;
    if (kPerp.x == NiPoint3_ZERO.x && kPerp.y == NiPoint3_ZERO.y && kPerp.z == NiPoint3_ZERO.z) {
        return kPerp;
    }
    return kDirection.UnitCross(kPerp);
}

// Unlike Update, the squared maximum distance is compared with the plain distance.
void NiPSysVortexFieldModifier::UpdateNoAttenuation(float fTime, NiPSysData* pkData) {
    NiTransform kField = m_pkFieldObj->m_kWorld;
    NiTransform kPSys = m_pkTarget->m_kWorld;
    NiTransform kInvPSys;
    kPSys.Invert(kInvPSys);
    NiTransform kFieldToPSys = kInvPSys * kField;

    NiPoint3 kOrigin = kFieldToPSys.m_Translate;
    const NiMatrix3& kRotate = kFieldToPSys.m_Rotate;
    NiPoint3 kAxis;
    kAxis.x = kRotate.m_pEntry[0][0] * m_kDirection.x + kRotate.m_pEntry[0][1] * m_kDirection.y +
        kRotate.m_pEntry[0][2] * m_kDirection.z;
    kAxis.y = kRotate.m_pEntry[1][0] * m_kDirection.x + kRotate.m_pEntry[1][1] * m_kDirection.y +
        kRotate.m_pEntry[1][2] * m_kDirection.z;
    kAxis.z = kRotate.m_pEntry[2][0] * m_kDirection.x + kRotate.m_pEntry[2][1] * m_kDirection.y +
        kRotate.m_pEntry[2][2] * m_kDirection.z;
    kAxis.Unitize();
    float fMaxDistanceSqr = m_fMaxDistance * m_fMaxDistance;

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
        float fDist = std::sqrt(fDistSqr);
        if (fDist == 0.0f) {
            continue;
        }
        if (!(fMaxDistanceSqr >= fDist)) {
            continue;
        }

        NiPoint3 kForce = VortexDirection(kOrigin, kAxis, kPos);
        float fScale = m_fMagnitude * fDeltaT;
        float fAddX = kForce.x * fScale;
        float fAddY = kForce.y * fScale;
        float fAddZ = fScale * kForce.z;
        pkInfo->m_kVelocity.x += fAddX;
        pkInfo->m_kVelocity.y += fAddY;
        pkInfo->m_kVelocity.z += fAddZ;
    }
}

void NiPSysVortexFieldModifier::UpdateNoAttenuationNoDist(float fTime, NiPSysData* pkData) {
    NiTransform kField = m_pkFieldObj->m_kWorld;
    NiTransform kPSys = m_pkTarget->m_kWorld;
    NiTransform kInvPSys;
    kPSys.Invert(kInvPSys);
    NiTransform kFieldToPSys = kInvPSys * kField;

    NiPoint3 kOrigin = kFieldToPSys.m_Translate;
    const NiMatrix3& kRotate = kFieldToPSys.m_Rotate;
    NiPoint3 kAxis;
    // EXE-INLINED 0x4B4500
    kAxis.x = kRotate.m_pEntry[0][0] * m_kDirection.x + kRotate.m_pEntry[0][1] * m_kDirection.y +
        kRotate.m_pEntry[0][2] * m_kDirection.z;
    kAxis.y = kRotate.m_pEntry[1][0] * m_kDirection.x + kRotate.m_pEntry[1][1] * m_kDirection.y +
        kRotate.m_pEntry[1][2] * m_kDirection.z;
    kAxis.z = kRotate.m_pEntry[2][0] * m_kDirection.x + kRotate.m_pEntry[2][1] * m_kDirection.y +
        kRotate.m_pEntry[2][2] * m_kDirection.z;
    kAxis.Unitize();

    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
        float fDeltaT = fTime - pkInfo->m_fLastUpdate;
        if (fDeltaT == 0.0f) {
            continue;
        }

        NiPoint3 kPos = pkData->m_pkVertex[i];
        NiPoint3 kForce = VortexDirection(kOrigin, kAxis, kPos);
        float fScale = m_fMagnitude * fDeltaT;
        float fAddX = kForce.x * fScale;
        float fAddY = kForce.y * fScale;
        float fAddZ = fScale * kForce.z;
        pkInfo->m_kVelocity.x += fAddX;
        pkInfo->m_kVelocity.y += fAddY;
        pkInfo->m_kVelocity.z += fAddZ;
    }
}

NiObject* NiPSysVortexFieldModifier::CreateObject() {
    NiPSysVortexFieldModifier* pkObject =
        static_cast<NiPSysVortexFieldModifier*>(NiMemObject::operator new(sizeof(NiPSysVortexFieldModifier)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiPSysVortexFieldModifier;
}

NiPSysFieldModifier::~NiPSysFieldModifier() {}
