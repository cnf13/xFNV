#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLightDimmerController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

NiRTTI NiLightDimmerController::ms_RTTI("NiLightDimmerController", &NiFloatInterpController::ms_RTTI);

NiLightDimmerController::NiLightDimmerController() {}

NiLightDimmerController::~NiLightDimmerController() {}

const NiRTTI* NiLightDimmerController::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiLightDimmerController::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiLightDimmerController));
    if (v3) {
        NiLightDimmerController* v4 = ::new (v3) NiLightDimmerController;
        NiFloatInterpController::CopyMembers(v4, kCloning);
        return v4;
    }

    NiFloatInterpController::CopyMembers(nullptr, kCloning);
    return nullptr;
}

// BSMaterialEmittanceMultController uses the same function.
void NiLightDimmerController::LoadBinary(NiStream& kStream) {
    NiFloatInterpController::LoadBinary(kStream);
}

void NiLightDimmerController::LinkObject(NiStream& kStream) {
    NiFloatInterpController::LinkObject(kStream);
}

bool NiLightDimmerController::RegisterStreamables(NiStream& kStream) {
    return NiFloatInterpController::RegisterStreamables(kStream);
}

void NiLightDimmerController::SaveBinary(NiStream& kStream) {
    NiFloatInterpController::SaveBinary(kStream);
}

bool NiLightDimmerController::IsEqual(NiObject* pkObject) {
    return NiFloatInterpController::IsEqual(pkObject);
}

void NiLightDimmerController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiFloatInterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// A manager-controlled controller always samples its interpolator, at the invalid time (-FLT_MAX). Changing the
// dimmer bumps the light's revision.
void NiLightDimmerController::Update(NiUpdateData& UpdateData) {
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
    if (m_spInterpolator->Update(m_fScaledTime, m_pkTarget, fValue) && m_pkTarget) {
        NiLight* pkLight = static_cast<NiLight*>(m_pkTarget->DynamicCastMember(&NiLight::ms_RTTI));
        ++pkLight->m_uiRevID;
        pkLight->m_fDimmer = fValue;
    }
}

// NiLightColorController uses the same function.
bool NiLightDimmerController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    return NiObject::IsKindOf(&NiLight::ms_RTTI, pkTarget);
}

// Without a target afValue is left as it is.
void NiLightDimmerController::GetTargetFloatValue(float& fValue) {
    if (m_pkTarget) {
        fValue = static_cast<NiLight*>(m_pkTarget)->m_fDimmer;
    }
}

NiObject* NiLightDimmerController::CreateObject() {
    return new NiLightDimmerController;
}
