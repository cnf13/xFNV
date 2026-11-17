#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysGravityStrengthCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysGravityModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"

NiRTTI NiPSysGravityStrengthCtlr::ms_RTTI("NiPSysGravityStrengthCtlr", &NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysGravityStrengthCtlr::NiPSysGravityStrengthCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysGravityStrengthCtlr::~NiPSysGravityStrengthCtlr() {}

NiObject* NiPSysGravityStrengthCtlr::CreateClone(NiCloningProcess& kCloning) {
	void* v3 = NiMemObject::operator new(sizeof(NiPSysGravityStrengthCtlr));
	NiPSysModifierCtlr* v4 = static_cast<NiPSysModifierCtlr*>(v3);
	if (v3) {
		::new (v3) NiPSysGravityStrengthCtlr;
		NiPSysModifierCtlr::CopyMembers(v4, kCloning);
		return v4;
	} else {
		NiPSysModifierCtlr::CopyMembers(nullptr, kCloning);
		return nullptr;
	}
}

void NiPSysGravityStrengthCtlr::LoadBinary(NiStream& kStream) {
	NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysGravityStrengthCtlr::LinkObject(NiStream& kStream) {
	NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysGravityStrengthCtlr::RegisterStreamables(NiStream& kStream) {
	return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysGravityStrengthCtlr::SaveBinary(NiStream& kStream) {
	NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysGravityStrengthCtlr::IsEqual(NiObject* pkObject) {
	return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysGravityStrengthCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The target's modifier with this controller's name must be a NiPSysGravityModifier.
bool NiPSysGravityStrengthCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
	if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget))
		return false;

	NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
	return NiObject::IsKindOf(&NiPSysGravityModifier::ms_RTTI, pkModifier);
}

void NiPSysGravityStrengthCtlr::GetTargetFloatValue(float& fValue) {
	fValue = static_cast<NiPSysGravityModifier*>(m_pkModifier)->m_fStrength;
}

void NiPSysGravityStrengthCtlr::SetTargetFloatValue(float fValue) {
	static_cast<NiPSysGravityModifier*>(m_pkModifier)->m_fStrength = fValue;
}

NiObject* NiPSysGravityStrengthCtlr::CreateObject() {
	NiPSysGravityStrengthCtlr* v0 =
		static_cast<NiPSysGravityStrengthCtlr*>(NiMemObject::operator new(sizeof(NiPSysGravityStrengthCtlr)));
	if (!v0) {
		return 0;
	}
	return ::new (v0) NiPSysGravityStrengthCtlr;
}
