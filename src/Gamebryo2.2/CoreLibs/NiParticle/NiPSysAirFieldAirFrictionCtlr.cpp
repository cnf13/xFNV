#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAirFieldAirFrictionCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAirFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysAirFieldAirFrictionCtlr::ms_RTTI("NiPSysAirFieldAirFrictionCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysAirFieldAirFrictionCtlr::NiPSysAirFieldAirFrictionCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysAirFieldAirFrictionCtlr::~NiPSysAirFieldAirFrictionCtlr() {}

NiObject* NiPSysAirFieldAirFrictionCtlr::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiPSysAirFieldAirFrictionCtlr));
    NiPSysModifierCtlr* v4 = static_cast<NiPSysModifierCtlr*>(v3);
    if (v3) {
        ::new (v3) NiPSysAirFieldAirFrictionCtlr;
        NiPSysModifierCtlr::CopyMembers(v4, kCloning);
        return v4;
    }
    else {
        NiPSysModifierCtlr::CopyMembers(nullptr, kCloning);
        return nullptr;
    }
}

// The streaming and compare slots are jumps to the NiPSysModifierFloatCtlr bodies, shared by every
// NiPSysModifierFloatCtlr leaf (and, except LinkObject, NiPSysModifierActiveCtlr); this class owns them.
void NiPSysAirFieldAirFrictionCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysAirFieldAirFrictionCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysAirFieldAirFrictionCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysAirFieldAirFrictionCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysAirFieldAirFrictionCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysAirFieldAirFrictionCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The target's modifier with this controller's name must be a NiPSysAirFieldModifier.
// NiPSysAirFieldInheritVelocityCtlr and NiPSysAirFieldSpreadCtlr use the same function.
bool NiPSysAirFieldAirFrictionCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysAirFieldModifier::ms_RTTI, pkModifier);
}

void NiPSysAirFieldAirFrictionCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysAirFieldModifier*>(m_pkModifier)->m_fAirFriction;
}

// Clamps the value to [0, 1].
void NiPSysAirFieldAirFrictionCtlr::SetTargetFloatValue(float fValue) {
    NiPSysAirFieldModifier* pkModifier = static_cast<NiPSysAirFieldModifier*>(m_pkModifier);
    if (fValue < 0.0) {
        fValue = 0.0f;
    }
    if (fValue > 1.0) {
        fValue = 1.0f;
    }
    pkModifier->m_fAirFriction = fValue;
}

NiObject* NiPSysAirFieldAirFrictionCtlr::CreateObject() {
    NiPSysAirFieldAirFrictionCtlr* v0 =
        static_cast<NiPSysAirFieldAirFrictionCtlr*>(NiMemObject::operator new(sizeof(NiPSysAirFieldAirFrictionCtlr)));
    if (!v0) {
        return 0;
    }
    return ::new (v0) NiPSysAirFieldAirFrictionCtlr;
}
