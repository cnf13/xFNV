#include "BSMain/BSPSysMultiTargetEmitterCtlr.h"

#include "BSMain/BSMasterParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysVolumeEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

#include <cfloat>

NiRTTI BSPSysMultiTargetEmitterCtlr::ms_RTTI("BSPSysMultiTargetEmitterCtlr", &NiPSysEmitterCtlr::ms_RTTI);

BSPSysMultiTargetEmitterCtlr::BSPSysMultiTargetEmitterCtlr() : pMasterParticleSystem(nullptr), usMaxFullEmitters(0) {}

BSPSysMultiTargetEmitterCtlr::BSPSysMultiTargetEmitterCtlr(
    const char* pcModifierName,
    BSMasterParticleSystem* apMasterParticleSystem,
    uint16_t ausMaxFullEmitters
)
  : NiPSysEmitterCtlr(pcModifierName), pMasterParticleSystem(apMasterParticleSystem),
    usMaxFullEmitters(ausMaxFullEmitters) {}

BSPSysMultiTargetEmitterCtlr::~BSPSysMultiTargetEmitterCtlr() {
    pMasterParticleSystem = nullptr;
}

NiObject* BSPSysMultiTargetEmitterCtlr::CreateClone(NiCloningProcess& kCloning) {
    BSPSysMultiTargetEmitterCtlr* v3;
    v3 = new BSPSysMultiTargetEmitterCtlr;
    CopyMembers(v3, kCloning);
    return v3;
}

// At least one emitter object always gets the full count.
void BSPSysMultiTargetEmitterCtlr::LoadBinary(NiStream& kStream) {
    NiPSysEmitterCtlr::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, usMaxFullEmitters);
    if (usMaxFullEmitters < 1) {
        usMaxFullEmitters = 1;
    }
    pMasterParticleSystem = static_cast<BSMasterParticleSystem*>(kStream.ResolveLinkID());
}

void BSPSysMultiTargetEmitterCtlr::LinkObject(NiStream& kStream) {
    NiPSysEmitterCtlr::LinkObject(kStream);
}

bool BSPSysMultiTargetEmitterCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysEmitterCtlr::RegisterStreamables(kStream);
}

void BSPSysMultiTargetEmitterCtlr::SaveBinary(NiStream& kStream) {
    NiPSysEmitterCtlr::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, usMaxFullEmitters);
    kStream.SaveLinkID(pMasterParticleSystem);
}

bool BSPSysMultiTargetEmitterCtlr::IsEqual(NiObject* pkObject) {
    if (!NiPSysEmitterCtlr::IsEqual(pkObject)) {
        return false;
    }

    BSPSysMultiTargetEmitterCtlr* pkOther = static_cast<BSPSysMultiTargetEmitterCtlr*>(pkObject);
    return pMasterParticleSystem == pkOther->pMasterParticleSystem && usMaxFullEmitters == pkOther->usMaxFullEmitters;
}

void BSPSysMultiTargetEmitterCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysEmitterCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Master Particle System", static_cast<const void*>(pMasterParticleSystem)));
    pkStrings->Add(NiGetViewerString("Max Full Emitters", usMaxFullEmitters));
}

// The clone's master is the master's clone. The lookup result is not checked: when the master was not cloned the
// variable still holds the clone itself.
void BSPSysMultiTargetEmitterCtlr::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysEmitterCtlr::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    BSPSysMultiTargetEmitterCtlr* pkClone = static_cast<BSPSysMultiTargetEmitterCtlr*>(pkObject);
    kCloning.m_pkCloneMap->GetAt(pMasterParticleSystem, pkObject);
    pkClone->pMasterParticleSystem = static_cast<BSMasterParticleSystem*>(pkObject);
}

// Keeps the controller's own clock (the emitter active keys are not consulted) and emits at the birth rate for the
// scaled time since the last update, on the master's emitter objects. Nothing runs while the master has no active
// emitter object.
void BSPSysMultiTargetEmitterCtlr::Update(NiUpdateData& UpdateData) {
    NiFloatInterpolator* pkBirthRate = nullptr;
    float fSystemTime = 0.0f;
    float fHiKeyTime = 0.0f;

    uint16_t usActive = pMasterParticleSystem->usActiveEmitterObjCount;
    if (!usActive) {
        m_fLastTime = UpdateData.fTime;
        return;
    }

    if (UpdateData.fTime < m_fLastTime) {
        m_fLastScaledTime = -FLT_MAX;
    }
    if (!m_pkTarget || !m_pkModifier) {
        return;
    }
    if (!(m_usFlags & ACTIVE_MASK)) {
        return;
    }
    if (UpdateData.fTime == m_fLastTime && !(m_usFlags & FORCEUDPATE_MASK)) {
        return;
    }

    if (m_fStartTime == -FLT_MAX) {
        m_fStartTime = UpdateData.fTime;
    }
    float fDeltaTime;
    if (m_fLastTime == -FLT_MAX) {
        fDeltaTime = 0.0f;
    }
    else {
        fDeltaTime = UpdateData.fTime - m_fLastTime;
    }
    m_fWeightedLastTime = fDeltaTime * m_fFrequency + m_fWeightedLastTime;
    m_fScaledTime = m_fWeightedLastTime + m_fPhase;
    m_fLastTime = UpdateData.fTime;

    fHiKeyTime = m_fHiKeyTime;
    if (fHiKeyTime != 0.0f) {
        float fScaledTime = m_fScaledTime;
        float fWrapped =
            static_cast<float>(std::fmod(static_cast<double>(fScaledTime), static_cast<double>(fHiKeyTime)));
        m_fScaledTime = fWrapped;
        if (m_fScaledTime < 0.0f) {
            m_fScaledTime = m_fScaledTime + fHiKeyTime;
        }
    }
    else {
        m_fScaledTime = 0.0f;
    }

    // The first update after a reset only syncs, unless the scaled time is already past zero.
    if (m_fLastScaledTime == -FLT_MAX) {
        if (m_fScaledTime > 0.0f) {
            m_fLastScaledTime = 0.0f;
        }
        else {
            m_fLastScaledTime = m_fScaledTime;
            return;
        }
    }

    pkBirthRate = GetBirthRateInterpolator();
    fSystemTime = static_cast<NiParticleSystem*>(m_pkTarget)->m_fLastTime;
    bool bWrapped = false;
    float fEmitDelta;
    if (m_fLastScaledTime < m_fScaledTime) {
        fEmitDelta = m_fScaledTime - m_fLastScaledTime;
    }
    else {
        fEmitDelta = m_fHiKeyTime - m_fLastScaledTime + m_fScaledTime;
        bWrapped = true;
    }

    if (pkBirthRate) {
        float fBirthRate;
        if (pkBirthRate->Update(0.0f, m_pkTarget, fBirthRate)) {
            float fCurTime = bWrapped ? m_fHiKeyTime : m_fScaledTime;
            float fTime = fSystemTime + fEmitDelta;
            EmitParticles(fTime, fCurTime, m_fLastScaledTime, 0.0f, m_fHiKeyTime, fBirthRate);
        }
    }
    m_fLastScaledTime = m_fScaledTime;
}

// Each of up to usMaxFullEmitters active emitter objects in turn takes the volume emitter's place and gets the same
// share of the new particles, capped at usMaxParticlesPerEmitterS; too few for one each go one apiece to as many
// emitter objects. The modifier's own emitter object and scale are restored afterwards.
void BSPSysMultiTargetEmitterCtlr::EmitParticles(
    float fTime,
    float fCurTime,
    float fLastTime,
    float fEmitStart,
    float fEmitStop,
    float fBirthRate
) {
    if (!pMasterParticleSystem) {
        return;
    }
    if (!(fBirthRate > 0.0f)) {
        return;
    }
    if (!(fEmitStop > fEmitStart)) {
        return;
    }
    if (!(fEmitStart < fCurTime)) {
        return;
    }
    if (!(fEmitStop > fLastTime)) {
        return;
    }

    // Takes the stop branch also when the compare is unordered.
    float fCurDelta;
    if (!(fEmitStop >= fCurTime)) {
        fCurDelta = fEmitStop - fEmitStart;
    }
    else {
        fCurDelta = fCurTime - fEmitStart;
    }
    float fLastDelta = 0.0f;
    if (fEmitStart <= fLastTime) {
        fLastDelta = fLastTime - fEmitStart;
    }

    uint16_t usActive = pMasterParticleSystem->usActiveEmitterObjCount;
    if (!usActive) {
        return;
    }

    uint16_t usCurCount = static_cast<uint16_t>(static_cast<int32_t>(fBirthRate * fCurDelta));
    uint16_t usLastCount = static_cast<uint16_t>(static_cast<int32_t>(fBirthRate * fLastDelta));
    uint16_t usNumParticles = usCurCount - usLastCount;
    if (!usNumParticles) {
        return;
    }

    uint16_t usEmitters = pMasterParticleSystem->usActiveEmitterObjCount;
    uint16_t usFullCount = usMaxFullEmitters * usNumParticles;
    int32_t iAllCount = usEmitters * usNumParticles;
    uint16_t usTotal = static_cast<uint16_t>(usFullCount < iAllCount ? usFullCount : iAllCount);
    uint16_t usPerEmitter = static_cast<uint16_t>(usTotal / usEmitters);
    if (usPerEmitter > usMaxParticlesPerEmitterS) {
        usPerEmitter = usMaxParticlesPerEmitterS;
    }
    if (!usPerEmitter) {
        usEmitters = usTotal;
        usPerEmitter = 1;
    }

    float fInvBirthRate = 1.0f / fBirthRate;
    m_kParticleAges.RemoveAll();
    for (uint16_t i = 0; i < usPerEmitter; ++i) {
        ++usLastCount;
        float fAge = fCurDelta - usLastCount * fInvBirthRate;
        m_kParticleAges.Add(fAge);
    }

    NiPSysVolumeEmitter* pkEmitter = static_cast<NiPSysVolumeEmitter*>(m_pkModifier);
    NiAVObject* pkOrigEmitterObj = pkEmitter->m_pkEmitterObj;
    float fOrigScale = pkEmitter->m_fScale;
    while (usEmitters > 0) {
        NiAVObject* pkEmitterObj = pMasterParticleSystem->GetNextEmitterObj();
        // Tests APP_CULLED directly on m_uFlags, not through GetAppCulled.
        if (pkEmitterObj && !(pkEmitterObj->m_uFlags & NiAVObject::APP_CULLED)) {
            pkEmitter->SetEmitterObj(pkEmitterObj);
            float fScale = pkEmitterObj->m_kLocal.m_fScale;
            pkEmitter->m_fScale = fScale;
            pkEmitter->EmitParticles(fTime, usPerEmitter, m_kParticleAges.GetBase());
            --usEmitters;
        }
    }
    pkEmitter->SetEmitterObj(pkOrigEmitterObj);
    pkEmitter->m_fScale = fOrigScale;
}

void BSPSysMultiTargetEmitterCtlr::CopyMembers(BSPSysMultiTargetEmitterCtlr* pDest, NiCloningProcess& arCloneData) {
    NiPSysEmitterCtlr::CopyMembers(pDest, arCloneData);
    pDest->usMaxFullEmitters = usMaxFullEmitters;
}

void BSPSysMultiTargetEmitterCtlr::SetMaxParticlesPerEmitter(uint16_t asValue) {
    BSPSysMultiTargetEmitterCtlr::usMaxParticlesPerEmitterS = asValue;
}

NiObject* BSPSysMultiTargetEmitterCtlr::CreateObject() {
    return new BSPSysMultiTargetEmitterCtlr;
}
