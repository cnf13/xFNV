#include "BSShader/BSRefractionFirePeriodController.h"

#include "BSShader/BSShaderPPLightingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <limits>

NiRTTI BSRefractionFirePeriodController::ms_RTTI("BSRefractionFirePeriodController", &NiFloatInterpController::ms_RTTI);

BSRefractionFirePeriodController::BSRefractionFirePeriodController() {}

BSRefractionFirePeriodController::~BSRefractionFirePeriodController() {}

const NiRTTI* BSRefractionFirePeriodController::GetRTTI() const {
    return &ms_RTTI;
}

// The base copies the members and the cloning process records the clone (a failed allocation is passed through as
// null).
NiObject* BSRefractionFirePeriodController::CreateClone(NiCloningProcess& kCloning) {
    BSRefractionFirePeriodController* pkClone = new BSRefractionFirePeriodController;
    CopyMembers(pkClone, kCloning);
    return pkClone;
}

void BSRefractionFirePeriodController::LoadBinary(NiStream& kStream) {
    NiFloatInterpController::LoadBinary(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        kStream.ReadLinkID();
    }
}

// Files older than 10.1.0.104 keep the float data on the controller: it is wrapped in a new float interpolator, which
// is collapsed.
void BSRefractionFirePeriodController::LinkObject(NiStream& kStream) {
    NiFloatInterpController::LinkObject(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        NiFloatData* pkData = static_cast<NiFloatData*>(kStream.GetObjectFromLinkID());
        NiFloatInterpolator* pkInterpolator = new NiFloatInterpolator(pkData);
        m_spInterpolator = pkInterpolator;
        m_spInterpolator->Collapse();
    }
}

bool BSRefractionFirePeriodController::RegisterStreamables(NiStream& kStream) {
    return NiFloatInterpController::RegisterStreamables(kStream);
}

void BSRefractionFirePeriodController::SaveBinary(NiStream& kStream) {
    NiFloatInterpController::SaveBinary(kStream);
}

bool BSRefractionFirePeriodController::IsEqual(NiObject* pkObject) {
    return NiFloatInterpController::IsEqual(pkObject);
}

void BSRefractionFirePeriodController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiFloatInterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The animated value is rounded half away from zero to the integer fire period. The target is not null-checked.
void BSRefractionFirePeriodController::Update(NiUpdateData& UpdateData) {
    if (m_usFlags & MANAGERCONTROLLED_MASK) {
        m_fScaledTime = -std::numeric_limits<float>::max();
    }
    else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate())) {
        return;
    }

    if (!m_spInterpolator) {
        return;
    }

    float fValue;
    if (m_spInterpolator->Update(m_fScaledTime, m_pkTarget, fValue)) {
        float fRounded = fValue + (fValue > 0.0f ? 0.5f : -0.5f);
        static_cast<BSShaderPPLightingProperty*>(m_pkTarget)->iRefractionFirePeriod = static_cast<int32_t>(fRounded);
    }
}

// BSRefractionStrengthController uses the same function.
bool BSRefractionFirePeriodController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    return NiObject::IsKindOf(&BSShaderPPLightingProperty::ms_RTTI, pkTarget);
}

void BSRefractionFirePeriodController::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<float>(static_cast<BSShaderPPLightingProperty*>(m_pkTarget)->iRefractionFirePeriod);
}

// Stream factory for this class: returns a new controller, or null.
NiObject* BSRefractionFirePeriodController::CreateObject() {
    BSRefractionFirePeriodController* pkController = new BSRefractionFirePeriodController;
    return pkController;
}
