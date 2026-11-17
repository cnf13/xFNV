#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterPlanarAngleVarCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysEmitterPlanarAngleVarCtlr::ms_RTTI("NiPSysEmitterPlanarAngleVarCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysEmitterPlanarAngleVarCtlr::NiPSysEmitterPlanarAngleVarCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysEmitterPlanarAngleVarCtlr::~NiPSysEmitterPlanarAngleVarCtlr() {}

NiObject* NiPSysEmitterPlanarAngleVarCtlr::CreateClone(NiCloningProcess& kCloning) {
	NiPSysEmitterPlanarAngleVarCtlr* v3 = static_cast<
		NiPSysEmitterPlanarAngleVarCtlr*>(NiMemObject::operator new(sizeof(NiPSysEmitterPlanarAngleVarCtlr)));
	NiPSysModifierCtlr* v4 = v3;
	if (v3) {
		::new (v3) NiPSysEmitterPlanarAngleVarCtlr;
		NiPSysModifierCtlr::CopyMembers(v4, kCloning);
		return v4;
	} else {
		NiPSysModifierCtlr::CopyMembers(nullptr, kCloning);
		return 0;
	}
}

void NiPSysEmitterPlanarAngleVarCtlr::LoadBinary(NiStream& kStream) {
	NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysEmitterPlanarAngleVarCtlr::LinkObject(NiStream& kStream) {
	NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysEmitterPlanarAngleVarCtlr::RegisterStreamables(NiStream& kStream) {
	return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysEmitterPlanarAngleVarCtlr::SaveBinary(NiStream& kStream) {
	NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysEmitterPlanarAngleVarCtlr::IsEqual(NiObject* pkObject) {
	return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysEmitterPlanarAngleVarCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysEmitterPlanarAngleVarCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
	if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget))
		return false;

	NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
	return NiObject::IsKindOf(&NiPSysEmitter::ms_RTTI, pkModifier);
}

// NiPSysGravityStrengthCtlr uses the same function.
void NiPSysEmitterPlanarAngleVarCtlr::GetTargetFloatValue(float& fValue) {
	fValue = static_cast<NiPSysEmitter*>(m_pkModifier)->m_fPlanarAngleVar;
}

// NiPSysGravityStrengthCtlr uses the same function.
void NiPSysEmitterPlanarAngleVarCtlr::SetTargetFloatValue(float fValue) {
	static_cast<NiPSysEmitter*>(m_pkModifier)->m_fPlanarAngleVar = fValue;
}

NiObject* NiPSysEmitterPlanarAngleVarCtlr::CreateObject() {
	NiPSysEmitterPlanarAngleVarCtlr* v0 = static_cast<
		NiPSysEmitterPlanarAngleVarCtlr*>(NiMemObject::operator new(sizeof(NiPSysEmitterPlanarAngleVarCtlr)));
	if (!v0) {
		return 0;
	}
	return ::new (v0) NiPSysEmitterPlanarAngleVarCtlr;
}
