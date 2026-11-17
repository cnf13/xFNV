#include "BSMain/BSWindModifier.h"

#include "BSMain/BSParentVelocityModifier.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

NiRTTI BSWindModifier::ms_RTTI("BSWindModifier", &NiPSysModifier::ms_RTTI);
NiPoint3 BSWindModifier::WindDir(NiPoint3_ZERO);

BSWindModifier::BSWindModifier() : fStrength(1.0f) {
    m_uiOrder = 4000;
}

BSWindModifier::~BSWindModifier() {}

NiObject* BSWindModifier::CreateClone(NiCloningProcess& arCloneData) {
    BSWindModifier* v3;
    void* v5 = NiMemObject::operator new(sizeof(BSWindModifier));
    if (v5) {
        v3 = ::new (v5) BSWindModifier;
    }
    else {
        v3 = nullptr;
    }
    CopyMembers(v3, arCloneData);
    return v3;
}

void BSWindModifier::LoadBinary(NiStream& arStream) {
    NiPSysModifier::LoadBinary(arStream);

    NiStreamLoadBinary(arStream, reinterpret_cast<uint32_t&>(fStrength));
}

void BSWindModifier::LinkObject(NiStream& arStream) {
    NiPSysModifier::LinkObject(arStream);
}

bool BSWindModifier::RegisterStreamables(NiStream& arStream) {
    return NiPSysModifier::RegisterStreamables(arStream);
}

// The strength is written as a 4-byte value through NiAnimationKey::SaveBinary.
void BSWindModifier::SaveBinary(NiStream& arStream) {
    NiPSysModifier::SaveBinary(arStream);

    NiAnimationKey::SaveBinary(arStream, reinterpret_cast<NiAnimationKey*>(&fStrength));
}

bool BSWindModifier::IsEqual(NiObject* apObject) {
    bool bEqual = NiPSysModifier::IsEqual(apObject);
    if (bEqual) {
        BSWindModifier* pkOther = static_cast<BSWindModifier*>(apObject);
        bEqual = fStrength == pkOther->fStrength;
    }
    return bEqual;
}

// Every active particle's velocity gains the wind direction scaled by the time since its last update and the strength.
void BSWindModifier::Update(float afTime, NiPSysData* apData) {
    uint16_t usActive = apData->m_usActive;
    if (usActive == 0) {
        return;
    }

    NiParticleInfo* pkInfo = apData->m_pkParticleInfo;
    uint32_t uiCount = apData->m_usActive;
    while (uiCount--) {
        float fScale = (afTime - pkInfo->m_fLastUpdate) * fStrength;
        pkInfo->m_kVelocity += WindDir * fScale;
        ++pkInfo;
    }
}

void BSWindModifier::CopyMembers(BSWindModifier* apDest, NiCloningProcess& arCloneData) {
    NiPSysModifier::CopyMembers(apDest, arCloneData);
    apDest->fStrength = fStrength;
}

// Scales afSpeed by the double 100.0 and stores it back as a float; the direction is the row vector times the rotation.
void BSWindModifier::SetWind(float afSpeed, float afAngle) {
    afSpeed = float(afSpeed * 100.0);
    NiPoint3 kWind;
    kWind.x = 0.0f;
    kWind.y = afSpeed;
    kWind.z = 0.0f;
    NiMatrix3 kRotation;
    kRotation.MakeZRotation(afAngle);
    WindDir = kWind * kRotation;
}

NiObject* BSWindModifier::CreateObject() {
    void* apMemory = NiMemObject::operator new(sizeof(BSWindModifier));
    if (apMemory) {
        return ::new (apMemory) BSWindModifier;
    }
    else {
        return nullptr;
    }
}
