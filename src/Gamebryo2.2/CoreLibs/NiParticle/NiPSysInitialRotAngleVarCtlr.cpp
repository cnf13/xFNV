#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysInitialRotAngleVarCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysRotationModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysInitialRotAngleVarCtlr::ms_RTTI("NiPSysInitialRotAngleVarCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysInitialRotAngleVarCtlr::NiPSysInitialRotAngleVarCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysInitialRotAngleVarCtlr::~NiPSysInitialRotAngleVarCtlr() {}

NiObject* NiPSysInitialRotAngleVarCtlr::CreateClone(NiCloningProcess& kCloning) {
    NiPSysModifierCtlr* v4 = new NiPSysInitialRotAngleVarCtlr;
    NiPSysModifierCtlr::CopyMembers(v4, kCloning);
    return v4;
}

void NiPSysInitialRotAngleVarCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysInitialRotAngleVarCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysInitialRotAngleVarCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysInitialRotAngleVarCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysInitialRotAngleVarCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysInitialRotAngleVarCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysInitialRotAngleVarCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysRotationModifier::ms_RTTI, pkModifier);
}

void NiPSysInitialRotAngleVarCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysRotationModifier*>(m_pkModifier)->m_fInitialRotAngleVar;
}

void NiPSysInitialRotAngleVarCtlr::SetTargetFloatValue(float fValue) {
    static_cast<NiPSysRotationModifier*>(m_pkModifier)->m_fInitialRotAngleVar = fValue;
}

NiObject* NiPSysInitialRotAngleVarCtlr::CreateObject() {
    return new NiPSysInitialRotAngleVarCtlr;
}
