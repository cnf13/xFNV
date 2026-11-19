#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldAttenuationCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysFieldAttenuationCtlr::ms_RTTI("NiPSysFieldAttenuationCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysFieldAttenuationCtlr::NiPSysFieldAttenuationCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysFieldAttenuationCtlr::~NiPSysFieldAttenuationCtlr() {}

NiObject* NiPSysFieldAttenuationCtlr::CreateClone(NiCloningProcess& kCloning) {
    NiPSysModifierCtlr* v4 = new NiPSysFieldAttenuationCtlr;
    NiPSysModifierCtlr::CopyMembers(v4, kCloning);
    return v4;
}

void NiPSysFieldAttenuationCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysFieldAttenuationCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysFieldAttenuationCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysFieldAttenuationCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysFieldAttenuationCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysFieldAttenuationCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The target's modifier with this controller's name must be a NiPSysFieldModifier.
// NiPSysFieldMagnitudeCtlr and NiPSysFieldMaxDistanceCtlr use the same function.
bool NiPSysFieldAttenuationCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysFieldModifier::ms_RTTI, pkModifier);
}

void NiPSysFieldAttenuationCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysFieldModifier*>(m_pkModifier)->m_fAttenuation;
}

void NiPSysFieldAttenuationCtlr::SetTargetFloatValue(float fValue) {
    static_cast<NiPSysFieldModifier*>(m_pkModifier)->m_fAttenuation = fValue;
}

NiObject* NiPSysFieldAttenuationCtlr::CreateObject() {
    return new NiPSysFieldAttenuationCtlr;
}
