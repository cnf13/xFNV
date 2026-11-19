#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysInitialRotAngleCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysRotationModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysInitialRotAngleCtlr::ms_RTTI("NiPSysInitialRotAngleCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysInitialRotAngleCtlr::NiPSysInitialRotAngleCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysInitialRotAngleCtlr::~NiPSysInitialRotAngleCtlr() {}

NiObject* NiPSysInitialRotAngleCtlr::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiPSysInitialRotAngleCtlr));
    NiPSysModifierCtlr* v4 = static_cast<NiPSysModifierCtlr*>(v3);
    if (v3) {
        ::new (v3) NiPSysInitialRotAngleCtlr;
        NiPSysModifierCtlr::CopyMembers(v4, kCloning);
        return v4;
    }

    NiPSysModifierCtlr::CopyMembers(nullptr, kCloning);
    return nullptr;
}

void NiPSysInitialRotAngleCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysInitialRotAngleCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysInitialRotAngleCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysInitialRotAngleCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysInitialRotAngleCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysInitialRotAngleCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The target's modifier with this controller's name must be a NiPSysRotationModifier.
// NiPSysInitialRotAngleVarCtlr, NiPSysInitialRotSpeedCtlr and NiPSysInitialRotSpeedVarCtlr use the same function.
bool NiPSysInitialRotAngleCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysRotationModifier::ms_RTTI, pkModifier);
}

void NiPSysInitialRotAngleCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysRotationModifier*>(m_pkModifier)->m_fInitialRotAngle;
}

void NiPSysInitialRotAngleCtlr::SetTargetFloatValue(float fValue) {
    static_cast<NiPSysRotationModifier*>(m_pkModifier)->m_fInitialRotAngle = fValue;
}

NiObject* NiPSysInitialRotAngleCtlr::CreateObject() {
    return new NiPSysInitialRotAngleCtlr;
}
