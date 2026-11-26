#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierActiveCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierBoolCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysModifierActiveCtlr::ms_RTTI("NiPSysModifierActiveCtlr", &NiPSysModifierBoolCtlr::ms_RTTI);

NiPSysModifierActiveCtlr::NiPSysModifierActiveCtlr() : NiPSysModifierBoolCtlr() {}

NiPSysModifierActiveCtlr::~NiPSysModifierActiveCtlr() {}

NiObject* NiPSysModifierActiveCtlr::CreateClone(NiCloningProcess& kCloning) {
    NiPSysModifierCtlr* v4 = new NiPSysModifierActiveCtlr;
    NiPSysModifierCtlr::CopyMembers(v4, kCloning);
    return v4;
}

void NiPSysModifierActiveCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierBoolCtlr::LoadBinary(kStream);
}

void NiPSysModifierActiveCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierBoolCtlr::LinkObject(kStream);
}

bool NiPSysModifierActiveCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierBoolCtlr::RegisterStreamables(kStream);
}

void NiPSysModifierActiveCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierBoolCtlr::SaveBinary(kStream);
}

bool NiPSysModifierActiveCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierBoolCtlr::IsEqual(pkObject);
}

void NiPSysModifierActiveCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierBoolCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The target must also hold the named modifier.
bool NiPSysModifierActiveCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    return static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName) != nullptr;
}

void NiPSysModifierActiveCtlr::GetTargetBoolValue(bool& bValue) {
    bValue = m_pkModifier->m_bActive;
}

// SetActive runs only when the value changes.
void NiPSysModifierActiveCtlr::SetTargetBoolValue(bool bValue) {
    if (m_pkModifier->m_bActive != bValue) {
        m_pkModifier->SetActive(bValue);
    }
}

NiObject* NiPSysModifierActiveCtlr::CreateObject() {
    return new NiPSysModifierActiveCtlr;
}
