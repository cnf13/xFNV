#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterDeclinationCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysEmitterDeclinationCtlr::ms_RTTI("NiPSysEmitterDeclinationCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysEmitterDeclinationCtlr::NiPSysEmitterDeclinationCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysEmitterDeclinationCtlr::~NiPSysEmitterDeclinationCtlr() {}

NiObject* NiPSysEmitterDeclinationCtlr::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiPSysEmitterDeclinationCtlr));
    NiPSysModifierCtlr* v4 = static_cast<NiPSysModifierCtlr*>(v3);
    if (v3) {
        ::new (v3) NiPSysEmitterDeclinationCtlr;
        NiPSysModifierCtlr::CopyMembers(v4, kCloning);
        return v4;
    }

    NiPSysModifierCtlr::CopyMembers(nullptr, kCloning);
    return nullptr;
}

void NiPSysEmitterDeclinationCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysEmitterDeclinationCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysEmitterDeclinationCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysEmitterDeclinationCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysEmitterDeclinationCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysEmitterDeclinationCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysEmitterDeclinationCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysEmitter::ms_RTTI, pkModifier);
}

// NiPSysFieldAttenuationCtlr and NiPSysInitialRotAngleCtlr use the same function.
void NiPSysEmitterDeclinationCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysEmitter*>(m_pkModifier)->m_fDeclination;
}

// NiPSysFieldAttenuationCtlr and NiPSysInitialRotAngleCtlr use the same function.
void NiPSysEmitterDeclinationCtlr::SetTargetFloatValue(float fValue) {
    static_cast<NiPSysEmitter*>(m_pkModifier)->m_fDeclination = fValue;
}

NiObject* NiPSysEmitterDeclinationCtlr::CreateObject() {
    NiPSysEmitterDeclinationCtlr* v0 =
        static_cast<NiPSysEmitterDeclinationCtlr*>(NiMemObject::operator new(sizeof(NiPSysEmitterDeclinationCtlr)));
    if (!v0) {
        return nullptr;
    }
    return ::new (v0) NiPSysEmitterDeclinationCtlr;
}
