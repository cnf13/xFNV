#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysResetOnLoopCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

#include <cfloat>

NiRTTI NiPSysResetOnLoopCtlr::ms_RTTI("NiPSysResetOnLoopCtlr", &NiTimeController::ms_RTTI);

NiPSysResetOnLoopCtlr::NiPSysResetOnLoopCtlr() : NiTimeController(), m_fLastScaledTime(-3.4028235e+38f) {}

NiPSysResetOnLoopCtlr::~NiPSysResetOnLoopCtlr() {}

NiObject* NiPSysResetOnLoopCtlr::CreateClone(NiCloningProcess& kCloning) {
	NiPSysResetOnLoopCtlr* pkObject =
		static_cast<NiPSysResetOnLoopCtlr*>(NiMemObject::operator new(sizeof(NiPSysResetOnLoopCtlr)));
	if (pkObject) {
		::new (pkObject) NiPSysResetOnLoopCtlr;
		CopyMembers(pkObject, kCloning);
		return pkObject;
	} else {
		CopyMembers(0, kCloning);
		return 0;
	}
}

// The exe's slot is a bare jmp to NiTimeController::LoadBinary. NiPSysUpdateCtlr uses the same function.
void NiPSysResetOnLoopCtlr::LoadBinary(NiStream& kStream) {
	NiTimeController::LoadBinary(kStream);
}

void NiPSysResetOnLoopCtlr::LinkObject(NiStream& kStream) {
	NiTimeController::LinkObject(kStream);
}

bool NiPSysResetOnLoopCtlr::RegisterStreamables(NiStream& kStream) {
	return NiTimeController::RegisterStreamables(kStream);
}

void NiPSysResetOnLoopCtlr::SaveBinary(NiStream& kStream) {
	NiTimeController::SaveBinary(kStream);
}

bool NiPSysResetOnLoopCtlr::IsEqual(NiObject* pkObject) {
	return NiTimeController::IsEqual(pkObject);
}

void NiPSysResetOnLoopCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiTimeController::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// Forces the LOOP cycle type, then resets the particle system whenever the scaled time wraps back. The first
// update (last time still -FLT_MAX) only records the time.
void NiPSysResetOnLoopCtlr::Update(NiUpdateData& UpdateData) {
	m_usFlags &= ~CYCLETYPE_MASK;
	if (!m_pkTarget || DontDoUpdate(UpdateData.fTime))
		return;

	if (m_fLastScaledTime == -FLT_MAX)
		m_fLastScaledTime = m_fScaledTime;

	if (m_fScaledTime < m_fLastScaledTime)
		static_cast<NiParticleSystem*>(m_pkTarget)->m_bResetSystem = true;

	m_fLastScaledTime = m_fScaledTime;
}

// NiPSysUpdateCtlr uses the same function.
bool NiPSysResetOnLoopCtlr::TargetIsRequiredType() const {
	return NiObject::IsKindOf(&NiParticleSystem::ms_RTTI, m_pkTarget);
}

NiObject* NiPSysResetOnLoopCtlr::CreateObject() {
	NiPSysResetOnLoopCtlr* pkObject =
		static_cast<NiPSysResetOnLoopCtlr*>(NiMemObject::operator new(sizeof(NiPSysResetOnLoopCtlr)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiPSysResetOnLoopCtlr;
}
