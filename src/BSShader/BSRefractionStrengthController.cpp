#include "BSShader/BSRefractionStrengthController.h"

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

#include <cfloat>

NiRTTI BSRefractionStrengthController::ms_RTTI("BSRefractionStrengthController", &NiFloatInterpController::ms_RTTI);

BSRefractionStrengthController::BSRefractionStrengthController() {}

// Destruction runs only the interpolator controller's destructor.
BSRefractionStrengthController::~BSRefractionStrengthController() {}

const NiRTTI* BSRefractionStrengthController::GetRTTI() const {
    return &ms_RTTI;
}

// The base copies the members and the cloning process records the clone (a failed allocation is passed through as
// null).
NiObject* BSRefractionStrengthController::CreateClone(NiCloningProcess& kCloning) {
    BSRefractionStrengthController* pkClone =
        static_cast<BSRefractionStrengthController*>(NiMemObject::operator new(sizeof(BSRefractionStrengthController)));
    if (pkClone) {
        ::new (pkClone) BSRefractionStrengthController;
    }
    CopyMembers(pkClone, kCloning);
    return pkClone;
}

void BSRefractionStrengthController::LoadBinary(NiStream& kStream) {
    NiFloatInterpController::LoadBinary(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        kStream.ReadLinkID();
    }
}

// Files older than 10.1.0.104 keep the float data on the controller: it is wrapped in a new float interpolator, which
// is collapsed.
void BSRefractionStrengthController::LinkObject(NiStream& kStream) {
    NiFloatInterpController::LinkObject(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        NiFloatData* pkData = static_cast<NiFloatData*>(kStream.GetObjectFromLinkID());
        NiFloatInterpolator* pkInterpolator =
            static_cast<NiFloatInterpolator*>(NiMemObject::operator new(sizeof(NiFloatInterpolator)));
        if (pkInterpolator) {
            pkInterpolator = ::new (pkInterpolator) NiFloatInterpolator(pkData);
        }
        m_spInterpolator = pkInterpolator;
        m_spInterpolator.m_pObject->Collapse();
    }
}

bool BSRefractionStrengthController::RegisterStreamables(NiStream& kStream) {
    return NiFloatInterpController::RegisterStreamables(kStream);
}

void BSRefractionStrengthController::SaveBinary(NiStream& kStream) {
    NiFloatInterpController::SaveBinary(kStream);
}

bool BSRefractionStrengthController::IsEqual(NiObject* pkObject) {
    return NiFloatInterpController::IsEqual(pkObject);
}

void BSRefractionStrengthController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiFloatInterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// A manager-controlled controller always samples its interpolator, at the invalid time (-FLT_MAX). The target is
// not null-checked.
void BSRefractionStrengthController::Update(NiUpdateData& UpdateData) {
    if (m_usFlags & MANAGERCONTROLLED_MASK) {
        m_fScaledTime = -FLT_MAX;
    }
    else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate())) {
        return;
    }

    if (!m_spInterpolator) {
        return;
    }

    float fValue;
    if (m_spInterpolator->Update(m_fScaledTime, m_pkTarget, fValue)) {
        static_cast<BSShaderPPLightingProperty*>(m_pkTarget)->SetRefractionStrength(fValue);
    }
}

bool BSRefractionStrengthController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    return NiObject::IsKindOf(&BSShaderPPLightingProperty::ms_RTTI, pkTarget);
}

void BSRefractionStrengthController::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<BSShaderPPLightingProperty*>(m_pkTarget)->fRefractionPower;
}

// Stream factory for this class: returns a new controller, or null.
NiObject* BSRefractionStrengthController::CreateObject() {
    BSRefractionStrengthController* pkController =
        static_cast<BSRefractionStrengthController*>(NiMemObject::operator new(sizeof(BSRefractionStrengthController)));
    if (pkController) {
        ::new (pkController) BSRefractionStrengthController;
    }
    return pkController;
}
