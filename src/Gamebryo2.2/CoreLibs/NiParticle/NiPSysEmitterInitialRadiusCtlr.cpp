#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterInitialRadiusCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysEmitterInitialRadiusCtlr::ms_RTTI("NiPSysEmitterInitialRadiusCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysEmitterInitialRadiusCtlr::NiPSysEmitterInitialRadiusCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysEmitterInitialRadiusCtlr::~NiPSysEmitterInitialRadiusCtlr() {}

NiObject* NiPSysEmitterInitialRadiusCtlr::CreateClone(NiCloningProcess& kCloning) {
    NiPSysModifierCtlr* v4 = new NiPSysEmitterInitialRadiusCtlr;
    NiPSysModifierCtlr::CopyMembers(v4, kCloning);
    return v4;
}

void NiPSysEmitterInitialRadiusCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysEmitterInitialRadiusCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysEmitterInitialRadiusCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysEmitterInitialRadiusCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysEmitterInitialRadiusCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysEmitterInitialRadiusCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysEmitterInitialRadiusCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysEmitter::ms_RTTI, pkModifier);
}

void NiPSysEmitterInitialRadiusCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysEmitter*>(m_pkModifier)->m_fInitialRadius;
}

void NiPSysEmitterInitialRadiusCtlr::SetTargetFloatValue(float fValue) {
    static_cast<NiPSysEmitter*>(m_pkModifier)->m_fInitialRadius = fValue;
}

NiObject* NiPSysEmitterInitialRadiusCtlr::CreateObject() {
    return new NiPSysEmitterInitialRadiusCtlr;
}
