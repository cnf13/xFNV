#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysInitialRotSpeedCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysRotationModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysInitialRotSpeedCtlr::ms_RTTI("NiPSysInitialRotSpeedCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysInitialRotSpeedCtlr::NiPSysInitialRotSpeedCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysInitialRotSpeedCtlr::~NiPSysInitialRotSpeedCtlr() {}

NiObject* NiPSysInitialRotSpeedCtlr::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiPSysInitialRotSpeedCtlr));
    NiPSysModifierCtlr* v4 = static_cast<NiPSysModifierCtlr*>(v3);
    if (v3) {
        ::new (v3) NiPSysInitialRotSpeedCtlr;
        NiPSysModifierCtlr::CopyMembers(v4, kCloning);
        return v4;
    }
    else {
        NiPSysModifierCtlr::CopyMembers(nullptr, kCloning);
        return nullptr;
    }
}

void NiPSysInitialRotSpeedCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysInitialRotSpeedCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysInitialRotSpeedCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysInitialRotSpeedCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysInitialRotSpeedCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysInitialRotSpeedCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysInitialRotSpeedCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysRotationModifier::ms_RTTI, pkModifier);
}

void NiPSysInitialRotSpeedCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysRotationModifier*>(m_pkModifier)->m_fInitialRotSpeed;
}

void NiPSysInitialRotSpeedCtlr::SetTargetFloatValue(float fValue) {
    static_cast<NiPSysRotationModifier*>(m_pkModifier)->m_fInitialRotSpeed = fValue;
}

NiObject* NiPSysInitialRotSpeedCtlr::CreateObject() {
    NiPSysInitialRotSpeedCtlr* v0 =
        static_cast<NiPSysInitialRotSpeedCtlr*>(NiMemObject::operator new(sizeof(NiPSysInitialRotSpeedCtlr)));
    if (!v0) {
        return nullptr;
    }
    return ::new (v0) NiPSysInitialRotSpeedCtlr;
}
