#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAirFieldSpreadCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAirFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysAirFieldSpreadCtlr::ms_RTTI("NiPSysAirFieldSpreadCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysAirFieldSpreadCtlr::NiPSysAirFieldSpreadCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysAirFieldSpreadCtlr::~NiPSysAirFieldSpreadCtlr() {}

NiObject* NiPSysAirFieldSpreadCtlr::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiPSysAirFieldSpreadCtlr));
    NiPSysModifierCtlr* v4 = static_cast<NiPSysModifierCtlr*>(v3);
    if (v3) {
        ::new (v3) NiPSysAirFieldSpreadCtlr;
        NiPSysModifierCtlr::CopyMembers(v4, kCloning);
        return v4;
    }
    else {
        NiPSysModifierCtlr::CopyMembers(nullptr, kCloning);
        return nullptr;
    }
}

void NiPSysAirFieldSpreadCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysAirFieldSpreadCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysAirFieldSpreadCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysAirFieldSpreadCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysAirFieldSpreadCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysAirFieldSpreadCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysAirFieldSpreadCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysAirFieldModifier::ms_RTTI, pkModifier);
}

void NiPSysAirFieldSpreadCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysAirFieldModifier*>(m_pkModifier)->m_fSpread;
}

// Clamps the value to [0, 1].
void NiPSysAirFieldSpreadCtlr::SetTargetFloatValue(float fValue) {
    NiPSysAirFieldModifier* pkModifier = static_cast<NiPSysAirFieldModifier*>(m_pkModifier);
    if (fValue < 0.0) {
        fValue = 0.0f;
    }
    if (fValue > 1.0) {
        fValue = 1.0f;
    }
    pkModifier->m_fSpread = fValue;
}

NiObject* NiPSysAirFieldSpreadCtlr::CreateObject() {
    NiPSysAirFieldSpreadCtlr* v0 =
        static_cast<NiPSysAirFieldSpreadCtlr*>(NiMemObject::operator new(sizeof(NiPSysAirFieldSpreadCtlr)));
    if (!v0) {
        return nullptr;
    }
    return ::new (v0) NiPSysAirFieldSpreadCtlr;
}
