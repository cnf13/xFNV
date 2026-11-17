#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiSingleInterpController::ms_RTTI("NiSingleInterpController", &NiInterpController::ms_RTTI);

NiSingleInterpController::NiSingleInterpController() : m_spInterpolator(nullptr) {}

NiSingleInterpController::~NiSingleInterpController() {}

// Files before 10.1.0.104 have no interpolator link; the interpolator is resolved at once, not in LinkObject.
void NiSingleInterpController::LoadBinary(NiStream& kStream) {
    NiInterpController::LoadBinary(kStream);

    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 104)) {
        m_spInterpolator = static_cast<NiInterpolator*>(kStream.ResolveLinkID());
    }
}

void NiSingleInterpController::LinkObject(NiStream& kStream) {
    NiInterpController::LinkObject(kStream);
}

bool NiSingleInterpController::RegisterStreamables(NiStream& kStream) {
    if (!NiInterpController::RegisterStreamables(kStream)) {
        return false;
    }

    if (m_spInterpolator) {
        m_spInterpolator->RegisterStreamables(kStream);
    }
    return true;
}

void NiSingleInterpController::SaveBinary(NiStream& kStream) {
    NiInterpController::SaveBinary(kStream);
    kStream.SaveLinkID(m_spInterpolator);
}

// Both interpolators unset, or both set and equal.
bool NiSingleInterpController::IsEqual(NiObject* pkObject) {
    if (!NiInterpController::IsEqual(pkObject)) {
        return false;
    }

    NiSingleInterpController* pkOther = static_cast<NiSingleInterpController*>(pkObject);
    if ((m_spInterpolator && !pkOther->m_spInterpolator) || (!m_spInterpolator && pkOther->m_spInterpolator)) {
        return false;
    }
    if (m_spInterpolator && !m_spInterpolator->IsEqual(pkOther->m_spInterpolator)) {
        return false;
    }
    return true;
}

void NiSingleInterpController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiInterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(NiSingleInterpController::ms_RTTI.GetName()));
    if (m_spInterpolator) {
        m_spInterpolator->GetViewerStrings(pkStrings);
    }
}

void NiSingleInterpController::ProcessClone(NiCloningProcess& kCloning) {
    NiInterpController::ProcessClone(kCloning);
    if (m_spInterpolator) {
        m_spInterpolator->ProcessClone(kCloning);
    }
}

// The one interpolator answers to the target's name.
uint16_t NiSingleInterpController::GetInterpolatorIndexFx(const NiFixedString& arName) const {
    if (m_pkTarget->m_kName == arName) {
        return 0;
    }
    return INVALID_INDEX;
}

NiInterpolator* NiSingleInterpController::GetInterpolator(uint16_t) const {
    return m_spInterpolator;
}

void NiSingleInterpController::SetInterpolator(NiInterpolator* pkInterpolator, uint16_t) {
    m_spInterpolator = pkInterpolator;
}

// Unless manager-controlled, the key range becomes the interpolator's active time range.
void NiSingleInterpController::ResetTimeExtrema() {
    if (GetManagerControlled()) {
        return;
    }

    NiInterpController::ResetTimeExtrema();
    if (m_spInterpolator) {
        m_spInterpolator->GetActiveTimeRange(m_fLoKeyTime, m_fHiKeyTime);
    }
}

void NiSingleInterpController::GuaranteeTimeRange(float fStartTime, float fEndTime) {
    if (m_spInterpolator) {
        m_spInterpolator->GuaranteeTimeRange(fStartTime, fEndTime);
    }
}

bool NiSingleInterpController::TargetIsRequiredType() const {
    return InterpTargetIsCorrectType(m_pkTarget);
}

void NiSingleInterpController::CopyMembers(NiSingleInterpController* pkDest, NiCloningProcess& kCloning) {
    NiInterpController::CopyMembers(pkDest, kCloning);

    if (m_spInterpolator) {
        pkDest->m_spInterpolator = static_cast<NiInterpolator*>(m_spInterpolator->CreateSharedClone(kCloning));
    }
}
