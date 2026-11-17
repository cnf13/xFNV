#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldMagnitudeCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysFieldMagnitudeCtlr::ms_RTTI("NiPSysFieldMagnitudeCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysFieldMagnitudeCtlr::NiPSysFieldMagnitudeCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysFieldMagnitudeCtlr::~NiPSysFieldMagnitudeCtlr() {}

NiObject* NiPSysFieldMagnitudeCtlr::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiPSysFieldMagnitudeCtlr));
    NiPSysModifierCtlr* v4 = static_cast<NiPSysModifierCtlr*>(v3);
    if (v3) {
        ::new (v3) NiPSysFieldMagnitudeCtlr;
        NiPSysModifierCtlr::CopyMembers(v4, kCloning);
        return v4;
    }
    else {
        NiPSysModifierCtlr::CopyMembers(nullptr, kCloning);
        return nullptr;
    }
}

void NiPSysFieldMagnitudeCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysFieldMagnitudeCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysFieldMagnitudeCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysFieldMagnitudeCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysFieldMagnitudeCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysFieldMagnitudeCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysFieldMagnitudeCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysFieldModifier::ms_RTTI, pkModifier);
}

// NiPSysInitialRotSpeedVarCtlr uses the same function.
void NiPSysFieldMagnitudeCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysFieldModifier*>(m_pkModifier)->m_fMagnitude;
}

// NiPSysInitialRotSpeedVarCtlr uses the same function.
void NiPSysFieldMagnitudeCtlr::SetTargetFloatValue(float fValue) {
    static_cast<NiPSysFieldModifier*>(m_pkModifier)->m_fMagnitude = fValue;
}

NiObject* NiPSysFieldMagnitudeCtlr::CreateObject() {
    NiPSysFieldMagnitudeCtlr* v0 =
        static_cast<NiPSysFieldMagnitudeCtlr*>(NiMemObject::operator new(sizeof(NiPSysFieldMagnitudeCtlr)));
    if (!v0) {
        return nullptr;
    }
    return ::new (v0) NiPSysFieldMagnitudeCtlr;
}
