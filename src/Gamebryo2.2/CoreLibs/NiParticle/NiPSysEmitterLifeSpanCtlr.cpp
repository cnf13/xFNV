#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterLifeSpanCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysEmitterLifeSpanCtlr::ms_RTTI("NiPSysEmitterLifeSpanCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysEmitterLifeSpanCtlr::NiPSysEmitterLifeSpanCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysEmitterLifeSpanCtlr::~NiPSysEmitterLifeSpanCtlr() {}

NiObject* NiPSysEmitterLifeSpanCtlr::CreateClone(NiCloningProcess& kCloning) {
    NiPSysEmitterLifeSpanCtlr* v3 =
        static_cast<NiPSysEmitterLifeSpanCtlr*>(NiMemObject::operator new(sizeof(NiPSysEmitterLifeSpanCtlr)));
    NiPSysModifierCtlr* v4 = v3;
    if (v3) {
        ::new (v3) NiPSysEmitterLifeSpanCtlr;
        NiPSysModifierCtlr::CopyMembers(v4, kCloning);
        return v4;
    }

    NiPSysModifierCtlr::CopyMembers(nullptr, kCloning);
    return nullptr;
}

void NiPSysEmitterLifeSpanCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysEmitterLifeSpanCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysEmitterLifeSpanCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysEmitterLifeSpanCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysEmitterLifeSpanCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysEmitterLifeSpanCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysEmitterLifeSpanCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysEmitter::ms_RTTI, pkModifier);
}

void NiPSysEmitterLifeSpanCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysEmitter*>(m_pkModifier)->m_fLifeSpan;
}

void NiPSysEmitterLifeSpanCtlr::SetTargetFloatValue(float fValue) {
    static_cast<NiPSysEmitter*>(m_pkModifier)->m_fLifeSpan = fValue;
}

NiObject* NiPSysEmitterLifeSpanCtlr::CreateObject() {
    NiPSysEmitterLifeSpanCtlr* v0 =
        static_cast<NiPSysEmitterLifeSpanCtlr*>(NiMemObject::operator new(sizeof(NiPSysEmitterLifeSpanCtlr)));
    if (!v0) {
        return nullptr;
    }
    return ::new (v0) NiPSysEmitterLifeSpanCtlr;
}
