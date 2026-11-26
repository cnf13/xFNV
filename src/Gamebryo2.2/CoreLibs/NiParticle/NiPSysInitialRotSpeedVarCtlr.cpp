#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysInitialRotSpeedVarCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysRotationModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysInitialRotSpeedVarCtlr::ms_RTTI("NiPSysInitialRotSpeedVarCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysInitialRotSpeedVarCtlr::NiPSysInitialRotSpeedVarCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysInitialRotSpeedVarCtlr::~NiPSysInitialRotSpeedVarCtlr() {}

NiObject* NiPSysInitialRotSpeedVarCtlr::CreateClone(NiCloningProcess& kCloning) {
    NiPSysModifierCtlr* v4 = new NiPSysInitialRotSpeedVarCtlr;
    NiPSysModifierCtlr::CopyMembers(v4, kCloning);
    return v4;
}

void NiPSysInitialRotSpeedVarCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysInitialRotSpeedVarCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysInitialRotSpeedVarCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysInitialRotSpeedVarCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysInitialRotSpeedVarCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysInitialRotSpeedVarCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysInitialRotSpeedVarCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysRotationModifier::ms_RTTI, pkModifier);
}

void NiPSysInitialRotSpeedVarCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysRotationModifier*>(m_pkModifier)->m_fInitialRotSpeedVar;
}

void NiPSysInitialRotSpeedVarCtlr::SetTargetFloatValue(float fValue) {
    static_cast<NiPSysRotationModifier*>(m_pkModifier)->m_fInitialRotSpeedVar = fValue;
}

NiObject* NiPSysInitialRotSpeedVarCtlr::CreateObject() {
    return new NiPSysInitialRotSpeedVarCtlr;
}
