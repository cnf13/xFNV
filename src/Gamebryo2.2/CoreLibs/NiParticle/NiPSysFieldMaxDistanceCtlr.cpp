#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldMaxDistanceCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysFieldMaxDistanceCtlr::ms_RTTI("NiPSysFieldMaxDistanceCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysFieldMaxDistanceCtlr::NiPSysFieldMaxDistanceCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysFieldMaxDistanceCtlr::~NiPSysFieldMaxDistanceCtlr() {}

NiObject* NiPSysFieldMaxDistanceCtlr::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiPSysFieldMaxDistanceCtlr));
    NiPSysModifierCtlr* v4 = static_cast<NiPSysModifierCtlr*>(v3);
    if (v3) {
        ::new (v3) NiPSysFieldMaxDistanceCtlr;
        NiPSysModifierCtlr::CopyMembers(v4, kCloning);
        return v4;
    }
    else {
        NiPSysModifierCtlr::CopyMembers(nullptr, kCloning);
        return nullptr;
    }
}

void NiPSysFieldMaxDistanceCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysFieldMaxDistanceCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysFieldMaxDistanceCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysFieldMaxDistanceCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysFieldMaxDistanceCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysFieldMaxDistanceCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysFieldMaxDistanceCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysFieldModifier::ms_RTTI, pkModifier);
}

void NiPSysFieldMaxDistanceCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysFieldModifier*>(m_pkModifier)->m_fMaxDistance;
}

// Also keeps the squared distance.
void NiPSysFieldMaxDistanceCtlr::SetTargetFloatValue(float fValue) {
    NiPSysFieldModifier* pkModifier = static_cast<NiPSysFieldModifier*>(m_pkModifier);
    pkModifier->m_fMaxDistance = fValue;
    pkModifier->m_fMaxDistanceSqr = fValue * fValue;
}

NiObject* NiPSysFieldMaxDistanceCtlr::CreateObject() {
    NiPSysFieldMaxDistanceCtlr* v0 =
        static_cast<NiPSysFieldMaxDistanceCtlr*>(NiMemObject::operator new(sizeof(NiPSysFieldMaxDistanceCtlr)));
    if (!v0) {
        return 0;
    }
    return ::new (v0) NiPSysFieldMaxDistanceCtlr;
}
