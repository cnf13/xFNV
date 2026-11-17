#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysVolumeEmitter.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysVolumeEmitter::ms_RTTI("NiPSysVolumeEmitter", &NiPSysEmitter::ms_RTTI);

NiPSysVolumeEmitter::NiPSysVolumeEmitter() : m_pkEmitterObj(nullptr) {}

// The emitter part is built with order 1000 and the same shape arguments; the emitter object follows.
NiPSysVolumeEmitter::NiPSysVolumeEmitter(
    const char* apcName,
    NiAVObject* apEmitterObj,
    float afSpeed,
    float afSpeedVar,
    float afDeclination,
    float afDeclinationVar,
    float afPlanarAngle,
    float afPlanarAngleVar,
    NiColorA akInitialColor,
    float afInitialRadius,
    float afLifeSpan,
    float afLifeSpanVar,
    float afRadiusVar
)
  : NiPSysEmitter(
        apcName,
        1000u,
        afSpeed,
        afSpeedVar,
        afDeclination,
        afDeclinationVar,
        afPlanarAngle,
        afPlanarAngleVar,
        akInitialColor,
        afInitialRadius,
        afLifeSpan,
        afLifeSpanVar,
        afRadiusVar
    ),
    m_pkEmitterObj(apEmitterObj) {}

// The emitter object's link ID is read and dropped; LinkObject reads it again.
void NiPSysVolumeEmitter::LoadBinary(NiStream& kStream) {
    NiPSysEmitter::LoadBinary(kStream);

    kStream.ReadLinkID();
}

void NiPSysVolumeEmitter::LinkObject(NiStream& kStream) {
    NiPSysEmitter::LinkObject(kStream);

    m_pkEmitterObj = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
}

bool NiPSysVolumeEmitter::RegisterStreamables(NiStream& kStream) {
    return NiPSysEmitter::RegisterStreamables(kStream);
}

void NiPSysVolumeEmitter::SaveBinary(NiStream& kStream) {
    NiPSysEmitter::SaveBinary(kStream);

    kStream.SaveLinkID(m_pkEmitterObj);
}

// Only whether both emitters have an emitter object is compared, not the objects.
bool NiPSysVolumeEmitter::IsEqual(NiObject* pkObject) {
    if (!NiPSysEmitter::IsEqual(pkObject)) {
        return false;
    }

    NiPSysVolumeEmitter* pkOther = static_cast<NiPSysVolumeEmitter*>(pkObject);
    return !((m_pkEmitterObj && !pkOther->m_pkEmitterObj) || (!m_pkEmitterObj && pkOther->m_pkEmitterObj));
}

// No line is added when there is no emitter object.
void NiPSysVolumeEmitter::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysEmitter::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysVolumeEmitter::ms_RTTI.GetName()));
    if (m_pkEmitterObj) {
        pkStrings->Add(NiGetViewerString("Emitter Object", (const char*)m_pkEmitterObj->m_kName));
    }
}

void NiPSysVolumeEmitter::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysEmitter::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiPSysVolumeEmitter* pkClone = static_cast<NiPSysVolumeEmitter*>(pkObject);
    if (m_pkEmitterObj) {
        if (kCloning.m_pkCloneMap->GetAt(m_pkEmitterObj, pkObject)) {
            pkClone->m_pkEmitterObj = static_cast<NiAVObject*>(pkObject);
        }
        else {
            pkClone->m_pkEmitterObj = m_pkEmitterObj;
        }
    }
}

// The volume is placed in particle system space: the emitter object's world transform brought into the particle
// system's, or the identity without an emitter object.
void NiPSysVolumeEmitter::ComputeInitialPositionAndVelocity(NiPoint3& kPosition, NiPoint3& kVelocity) {
    NiTransform kEmitterToPSys;
    if (m_pkEmitterObj) {
        NiTransform kEmitterToWorld = m_pkEmitterObj->m_kWorld;
        NiTransform kPSysToWorld = m_pkTarget->m_kWorld;
        NiTransform kWorldToPSys;
        kPSysToWorld.Invert(kWorldToPSys);
        kEmitterToPSys = kWorldToPSys * kEmitterToWorld;
    }
    else {
        kEmitterToPSys.MakeIdentity();
    }
    ComputeVolumeInitialPositionAndVelocity(kEmitterToPSys, kPosition, kVelocity);
}

void NiPSysVolumeEmitter::CopyMembers(NiPSysVolumeEmitter* pkDest, NiCloningProcess& kCloning) {
    NiPSysEmitter::CopyMembers(pkDest, kCloning);
}
