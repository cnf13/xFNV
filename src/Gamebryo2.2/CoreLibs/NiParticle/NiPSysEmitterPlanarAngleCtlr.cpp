#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterPlanarAngleCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysEmitterPlanarAngleCtlr::ms_RTTI("NiPSysEmitterPlanarAngleCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysEmitterPlanarAngleCtlr::NiPSysEmitterPlanarAngleCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysEmitterPlanarAngleCtlr::~NiPSysEmitterPlanarAngleCtlr() {}

NiObject* NiPSysEmitterPlanarAngleCtlr::CreateClone(NiCloningProcess& kCloning) {
    NiPSysModifierCtlr* v4 = new NiPSysEmitterPlanarAngleCtlr;
    NiPSysModifierCtlr::CopyMembers(v4, kCloning);
    return v4;
}

void NiPSysEmitterPlanarAngleCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysEmitterPlanarAngleCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysEmitterPlanarAngleCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysEmitterPlanarAngleCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysEmitterPlanarAngleCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysEmitterPlanarAngleCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysEmitterPlanarAngleCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysEmitter::ms_RTTI, pkModifier);
}

// NiPSysFieldMaxDistanceCtlr uses the same function.
void NiPSysEmitterPlanarAngleCtlr::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<NiPSysEmitter*>(m_pkModifier)->m_fPlanarAngle;
}

void NiPSysEmitterPlanarAngleCtlr::SetTargetFloatValue(float fValue) {
    static_cast<NiPSysEmitter*>(m_pkModifier)->m_fPlanarAngle = fValue;
}

NiObject* NiPSysEmitterPlanarAngleCtlr::CreateObject() {
    return new NiPSysEmitterPlanarAngleCtlr;
}
