#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysTurbulenceFieldModifier.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
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
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysTurbulenceFieldModifier::ms_RTTI("NiPSysTurbulenceFieldModifier", &NiPSysFieldModifier::ms_RTTI);

NiPSysTurbulenceFieldModifier::NiPSysTurbulenceFieldModifier()
  : NiPSysFieldModifier(), m_fFrequency(0.0f), m_fFrequencyUpdateTime(10000.0f), m_fLastUpdateTime(-3.4028235e+38f) {}

NiPSysTurbulenceFieldModifier::~NiPSysTurbulenceFieldModifier() {}

NiObject* NiPSysTurbulenceFieldModifier::CreateClone(NiCloningProcess& kCloning) {
    NiPSysTurbulenceFieldModifier* pkObject =
        static_cast<NiPSysTurbulenceFieldModifier*>(NiMemObject::operator new(sizeof(NiPSysTurbulenceFieldModifier)));
    if (pkObject) {
        ::new (pkObject) NiPSysTurbulenceFieldModifier;
    }
    else {
        pkObject = nullptr;
    }
    CopyMembers(pkObject, kCloning);
    float fFrequency = m_fFrequency;
    pkObject->m_fFrequency = fFrequency;
    // Takes the reciprocal unless the frequency is below 1e-4: a NaN frequency takes it too.
    if (fFrequency < 9.999999747378752e-05) {
        pkObject->m_fFrequencyUpdateTime = 10000.0f;
    }
    else {
        pkObject->m_fFrequencyUpdateTime = 1.0 / fFrequency;
    }
    return pkObject;
}

// Frequencies below 0.0001 update every 10000 seconds; a NaN frequency takes the reciprocal.
void NiPSysTurbulenceFieldModifier::LoadBinary(NiStream& kStream) {
    NiPSysFieldModifier::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_fFrequency);
    if (m_fFrequency < 0.0001f) {
        m_fFrequencyUpdateTime = 10000.0f;
    }
    else {
        m_fFrequencyUpdateTime = 1.0f / m_fFrequency;
    }
}

void NiPSysTurbulenceFieldModifier::LinkObject(NiStream& kStream) {
    NiPSysFieldModifier::LinkObject(kStream);
}

bool NiPSysTurbulenceFieldModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysFieldModifier::RegisterStreamables(kStream);
}

void NiPSysTurbulenceFieldModifier::SaveBinary(NiStream& kStream) {
    NiPSysFieldModifier::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_fFrequency);
}

bool NiPSysTurbulenceFieldModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysFieldModifier::IsEqual(pkObject)) {
        return false;
    }

    NiPSysTurbulenceFieldModifier* pkOther = static_cast<NiPSysTurbulenceFieldModifier*>(pkObject);
    return m_fFrequency == pkOther->m_fFrequency;
}

void NiPSysTurbulenceFieldModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysFieldModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysTurbulenceFieldModifier::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Frequency", m_fFrequency));
}

// Kicks every particle in a random direction once per frequency period. The time is recorded before the
// magnitude, particle count and field object are checked.
void NiPSysTurbulenceFieldModifier::Update(float fTime, NiPSysData* pkData) {
    if (m_fLastUpdateTime <= fTime && m_fFrequencyUpdateTime + m_fLastUpdateTime > fTime) {
        return;
    }
    m_fLastUpdateTime = fTime;

    if (m_fMagnitude == 0.0f) {
        return;
    }
    if (!pkData->m_usActive) {
        return;
    }
    if (!m_pkFieldObj) {
        return;
    }
    if (m_fAttenuation == 0.0f) {
        if (m_bUseMaxDistance) {
            UpateNoAttenuation(fTime, pkData);
        }
        else {
            UpateNoAttenuationNoDistance(fTime, pkData);
        }
        return;
    }

    NiTransform kField = m_pkFieldObj->m_kWorld;
    NiTransform kPSys = m_pkTarget->m_kWorld;
    NiTransform kInvPSys;
    kPSys.Invert(kInvPSys);
    NiTransform kFieldToPSys = kInvPSys * kField;

    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
        NiPoint3 kPos = pkData->m_pkVertex[i];
        float fDiffX = kPos.x - kFieldToPSys.m_Translate.x;
        float fDiffY = kPos.y - kFieldToPSys.m_Translate.y;
        float fDiffZ = kPos.z - kFieldToPSys.m_Translate.z;
        float fDistSqr = fDiffX * fDiffX + fDiffY * fDiffY + fDiffZ * fDiffZ;
        // The exe inlines fsqrt (__CIsqrt 0xEC6040) on the float reloaded from the stack.
        float fDist = std::sqrt(fDistSqr);
        if (m_bUseMaxDistance && !(m_fMaxDistance >= fDist)) {
            continue;
        }

        float fRandomX = NiSymmetricRandom();
        float fRandomY = NiSymmetricRandom();
        float fRandomZ = NiSymmetricRandom();
        float fScale = m_fMagnitude / (m_fAttenuation * fDist + 1.0f);
        float fAddX = fScale * fRandomX;
        float fAddY = fScale * fRandomY;
        float fAddZ = fScale * fRandomZ;
        pkInfo->m_kVelocity.x += fAddX;
        pkInfo->m_kVelocity.y += fAddY;
        pkInfo->m_kVelocity.z += fAddZ;
    }
}

// Every particle is kicked, whatever its last update time.
void NiPSysTurbulenceFieldModifier::UpateNoAttenuationNoDistance(float, NiPSysData* pkData) {
    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
        float fRandomX = NiSymmetricRandom();
        float fRandomY = NiSymmetricRandom();
        float fRandomZ = NiSymmetricRandom();
        float fMagnitude = m_fMagnitude;
        float fAddX = fMagnitude * fRandomX;
        float fAddY = fMagnitude * fRandomY;
        float fAddZ = fMagnitude * fRandomZ;
        pkInfo->m_kVelocity.x += fAddX;
        pkInfo->m_kVelocity.y += fAddY;
        pkInfo->m_kVelocity.z += fAddZ;
    }
}

// Unlike Update, the maximum distance is checked against the squared distance.
void NiPSysTurbulenceFieldModifier::UpateNoAttenuation(float, NiPSysData* pkData) {
    NiTransform kField = m_pkFieldObj->m_kWorld;
    NiTransform kPSys = m_pkTarget->m_kWorld;
    NiTransform kInvPSys;
    kPSys.Invert(kInvPSys);
    NiTransform kFieldToPSys = kInvPSys * kField;

    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
        NiPoint3 kPos = pkData->m_pkVertex[i];
        float fDiffX = kPos.x - kFieldToPSys.m_Translate.x;
        float fDiffY = kPos.y - kFieldToPSys.m_Translate.y;
        float fDiffZ = kPos.z - kFieldToPSys.m_Translate.z;
        float fDistSqr = fDiffX * fDiffX + fDiffY * fDiffY + fDiffZ * fDiffZ;
        if (!(m_fMaxDistanceSqr >= fDistSqr)) {
            continue;
        }

        float fRandomX = NiSymmetricRandom();
        float fRandomY = NiSymmetricRandom();
        float fRandomZ = NiSymmetricRandom();
        float fMagnitude = m_fMagnitude;
        float fAddX = fMagnitude * fRandomX;
        float fAddY = fMagnitude * fRandomY;
        float fAddZ = fMagnitude * fRandomZ;
        pkInfo->m_kVelocity.x += fAddX;
        pkInfo->m_kVelocity.y += fAddY;
        pkInfo->m_kVelocity.z += fAddZ;
    }
}

NiObject* NiPSysTurbulenceFieldModifier::CreateObject() {
    NiPSysTurbulenceFieldModifier* pkObject =
        static_cast<NiPSysTurbulenceFieldModifier*>(NiMemObject::operator new(sizeof(NiPSysTurbulenceFieldModifier)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiPSysTurbulenceFieldModifier;
}
