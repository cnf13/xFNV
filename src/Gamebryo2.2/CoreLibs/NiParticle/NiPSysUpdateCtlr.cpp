#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysUpdateCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

#include <limits>

NiRTTI NiPSysUpdateCtlr::ms_RTTI("NiPSysUpdateCtlr", &NiTimeController::ms_RTTI);

// Zeroes the base's key times after the base constructor.
NiPSysUpdateCtlr::NiPSysUpdateCtlr() : NiTimeController() {
    m_fHiKeyTime = 0.0f;
    m_fLoKeyTime = 0.0f;
}

NiPSysUpdateCtlr::~NiPSysUpdateCtlr() {}

NiObject* NiPSysUpdateCtlr::CreateClone(NiCloningProcess& kCloning) {
    NiPSysUpdateCtlr* v3 = new NiPSysUpdateCtlr;
    CopyMembers(v3, kCloning);
    return v3;
}

void NiPSysUpdateCtlr::LoadBinary(NiStream& kStream) {
    NiTimeController::LoadBinary(kStream);
}

void NiPSysUpdateCtlr::LinkObject(NiStream& kStream) {
    NiTimeController::LinkObject(kStream);
}

bool NiPSysUpdateCtlr::RegisterStreamables(NiStream& kStream) {
    return NiTimeController::RegisterStreamables(kStream);
}

void NiPSysUpdateCtlr::SaveBinary(NiStream& kStream) {
    NiTimeController::SaveBinary(kStream);
}

bool NiPSysUpdateCtlr::IsEqual(NiObject* pkObject) {
    return NiTimeController::IsEqual(pkObject);
}

void NiPSysUpdateCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTimeController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// Runs the particle system at the scaled time, with an unbounded key range and the CLAMP cycle type while the time
// is scaled. When the target's first emitter controller loops and the gap since the last update is longer than its
// emitter's life span, the last time jumps to now, so the system does not replay the whole gap.
void NiPSysUpdateCtlr::Update(NiUpdateData& UpdateData) {
    m_fLoKeyTime = -std::numeric_limits<float>::max();
    m_fHiKeyTime = std::numeric_limits<float>::max();
    m_usFlags = (m_usFlags & ~CYCLETYPE_MASK) | CLAMP << CYCLETYPE_POS;

    NiPSysEmitterCtlr* pkEmitterCtlr = nullptr;
    NiPSysEmitter* pkEmitter = nullptr;
    for (NiTimeController* pkCtlr = m_pkTarget->m_spControllers; pkCtlr; pkCtlr = pkCtlr->m_spNext) {
        if (pkCtlr->IsKindOf(&NiPSysEmitterCtlr::ms_RTTI)) {
            pkEmitterCtlr = static_cast<NiPSysEmitterCtlr*>(pkCtlr);
            pkEmitter = static_cast<NiPSysEmitter*>(pkEmitterCtlr->m_pkModifier);
            break;
        }
    }

    if (m_fLastTime != -std::numeric_limits<float>::max() &&
        pkEmitterCtlr &&
        !(pkEmitterCtlr->m_usFlags & CYCLETYPE_MASK) &&
        UpdateData.fTime - m_fLastTime > pkEmitter->m_fLifeSpan) {
        m_fLastTime = UpdateData.fTime;
    }

    bool bDontUpdate = DontDoUpdate(UpdateData.fTime);
    m_fHiKeyTime = 0.0f;
    m_fLoKeyTime = 0.0f;
    if (m_pkTarget && !bDontUpdate) {
        static_cast<NiParticleSystem*>(m_pkTarget)->UpdateSystem(m_fScaledTime);
    }
}

// Moves this controller to the end of the target's controller list, so the particle system updates after the
// controllers that animate it.
void NiPSysUpdateCtlr::SetTarget(NiObjectNET* pkTarget) {
    NiTimeController::SetTarget(pkTarget);
    if (!m_pkTarget) {
        return;
    }

    NiTimeController* pkLast = m_pkTarget->m_spControllers;
    while (pkLast->m_spNext) {
        pkLast = pkLast->m_spNext;
    }

    if (pkLast == this) {
        return;
    }

    pkLast->SetNext(this);
    m_pkTarget->SetControllers(m_spNext);
    m_spNext = nullptr;
}

bool NiPSysUpdateCtlr::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiParticleSystem::ms_RTTI, m_pkTarget);
}

NiObject* NiPSysUpdateCtlr::CreateObject() {
    return new NiPSysUpdateCtlr;
}

void NiTimeController::SetNext(NiTimeController* pkNext) {
    NiTimeController* pkOld = m_spNext.m_pObject;
    NiTimeController* pkNew = pkNext;
    if (pkOld != pkNew) {
        if (pkOld) {
            NiDecRefCount(pkOld);
        }
        m_spNext.m_pObject = pkNew;
        if (pkNew) {
            NiIncRefCount(pkNew);
        }
    }
}
