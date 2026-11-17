#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterSpeedCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysEmitterSpeedCtlr::ms_RTTI("NiPSysEmitterSpeedCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysEmitterSpeedCtlr::NiPSysEmitterSpeedCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysEmitterSpeedCtlr::~NiPSysEmitterSpeedCtlr() {}

NiObject* NiPSysEmitterSpeedCtlr::CreateClone(NiCloningProcess& kCloning) {
	NiPSysEmitterSpeedCtlr* v3 =
		static_cast<NiPSysEmitterSpeedCtlr*>(NiMemObject::operator new(sizeof(NiPSysEmitterSpeedCtlr)));
	NiPSysModifierCtlr* v4 = v3;
	if (v3) {
		::new (v3) NiPSysEmitterSpeedCtlr;
		NiPSysModifierCtlr::CopyMembers(v4, kCloning);
		return v4;
	} else {
		NiPSysModifierCtlr::CopyMembers(nullptr, kCloning);
		return 0;
	}
}

void NiPSysEmitterSpeedCtlr::LoadBinary(NiStream& kStream) {
	NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysEmitterSpeedCtlr::LinkObject(NiStream& kStream) {
	NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysEmitterSpeedCtlr::RegisterStreamables(NiStream& kStream) {
	return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysEmitterSpeedCtlr::SaveBinary(NiStream& kStream) {
	NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysEmitterSpeedCtlr::IsEqual(NiObject* pkObject) {
	return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysEmitterSpeedCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysEmitterSpeedCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
	if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget))
		return false;

	NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
	return NiObject::IsKindOf(&NiPSysEmitter::ms_RTTI, pkModifier);
}

// NiPSysInitialRotSpeedCtlr uses the same function.
void NiPSysEmitterSpeedCtlr::GetTargetFloatValue(float& fValue) {
	fValue = static_cast<NiPSysEmitter*>(m_pkModifier)->m_fSpeed;
}

// NiPSysInitialRotSpeedCtlr uses the same function.
void NiPSysEmitterSpeedCtlr::SetTargetFloatValue(float fValue) {
	static_cast<NiPSysEmitter*>(m_pkModifier)->m_fSpeed = fValue;
}

NiObject* NiPSysEmitterSpeedCtlr::CreateObject() {
	NiPSysEmitterSpeedCtlr* v0 =
		static_cast<NiPSysEmitterSpeedCtlr*>(NiMemObject::operator new(sizeof(NiPSysEmitterSpeedCtlr)));
	if (!v0) {
		return 0;
	}
	return ::new (v0) NiPSysEmitterSpeedCtlr;
}
