#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterDeclinationVarCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysEmitterDeclinationVarCtlr::ms_RTTI("NiPSysEmitterDeclinationVarCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysEmitterDeclinationVarCtlr::NiPSysEmitterDeclinationVarCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysEmitterDeclinationVarCtlr::~NiPSysEmitterDeclinationVarCtlr() {}

NiObject* NiPSysEmitterDeclinationVarCtlr::CreateClone(NiCloningProcess& kCloning) {
    NiPSysModifierCtlr* v4 = new NiPSysEmitterDeclinationVarCtlr;
    NiPSysModifierCtlr::CopyMembers(v4, kCloning);
    return v4;
}

void NiPSysEmitterDeclinationVarCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysEmitterDeclinationVarCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysEmitterDeclinationVarCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysEmitterDeclinationVarCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysEmitterDeclinationVarCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysEmitterDeclinationVarCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysEmitterDeclinationVarCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysEmitter::ms_RTTI, pkModifier);
}

// NiPSysInitialRotAngleVarCtlr uses the same function.
void NiPSysEmitterDeclinationVarCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysEmitter*>(m_pkModifier)->m_fDeclinationVar;
}

// NiPSysInitialRotAngleVarCtlr uses the same function.
void NiPSysEmitterDeclinationVarCtlr::SetTargetFloatValue(float fValue) {
    static_cast<NiPSysEmitter*>(m_pkModifier)->m_fDeclinationVar = fValue;
}

NiObject* NiPSysEmitterDeclinationVarCtlr::CreateObject() {
    return new NiPSysEmitterDeclinationVarCtlr;
}
