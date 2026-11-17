#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAirFieldInheritVelocityCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAirFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysAirFieldInheritVelocityCtlr::ms_RTTI("NiPSysAirFieldInheritVelocityCtlr",
	&NiPSysModifierFloatCtlr::ms_RTTI);

NiPSysAirFieldInheritVelocityCtlr::NiPSysAirFieldInheritVelocityCtlr() : NiPSysModifierFloatCtlr() {}

NiPSysAirFieldInheritVelocityCtlr::~NiPSysAirFieldInheritVelocityCtlr() {}

NiObject* NiPSysAirFieldInheritVelocityCtlr::CreateClone(NiCloningProcess& kCloning) {
	void* v3 = NiMemObject::operator new(sizeof(NiPSysAirFieldInheritVelocityCtlr));
	NiPSysModifierCtlr* v4 = static_cast<NiPSysModifierCtlr*>(v3);
	if (v3) {
		::new (v3) NiPSysAirFieldInheritVelocityCtlr;
		NiPSysModifierCtlr::CopyMembers(v4, kCloning);
		return v4;
	} else {
		NiPSysModifierCtlr::CopyMembers(nullptr, kCloning);
		return nullptr;
	}
}

void NiPSysAirFieldInheritVelocityCtlr::LoadBinary(NiStream& kStream) {
	NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

void NiPSysAirFieldInheritVelocityCtlr::LinkObject(NiStream& kStream) {
	NiPSysModifierFloatCtlr::LinkObject(kStream);
}

bool NiPSysAirFieldInheritVelocityCtlr::RegisterStreamables(NiStream& kStream) {
	return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

void NiPSysAirFieldInheritVelocityCtlr::SaveBinary(NiStream& kStream) {
	NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

bool NiPSysAirFieldInheritVelocityCtlr::IsEqual(NiObject* pkObject) {
	return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

void NiPSysAirFieldInheritVelocityCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPSysModifierFloatCtlr::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiPSysAirFieldInheritVelocityCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
	if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget))
		return false;

	NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
	return NiObject::IsKindOf(&NiPSysAirFieldModifier::ms_RTTI, pkModifier);
}

void NiPSysAirFieldInheritVelocityCtlr::GetTargetFloatValue(float& fValue) {
	fValue = static_cast<NiPSysAirFieldModifier*>(m_pkModifier)->m_fInheritVelocity;
}

// Clamps the value to [0, 1].
void NiPSysAirFieldInheritVelocityCtlr::SetTargetFloatValue(float fValue) {
	NiPSysAirFieldModifier* pkModifier = static_cast<NiPSysAirFieldModifier*>(m_pkModifier);
	if (fValue < 0.0)
		fValue = 0.0f;
	if (fValue > 1.0)
		fValue = 1.0f;
	pkModifier->m_fInheritVelocity = fValue;
}

NiObject* NiPSysAirFieldInheritVelocityCtlr::CreateObject() {
	NiPSysAirFieldInheritVelocityCtlr* v0 = static_cast<
		NiPSysAirFieldInheritVelocityCtlr*>(NiMemObject::operator new(sizeof(NiPSysAirFieldInheritVelocityCtlr)));
	if (!v0) {
		return 0;
	}
	return ::new (v0) NiPSysAirFieldInheritVelocityCtlr;
}
