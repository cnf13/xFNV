#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatExtraDataController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiExtraDataController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFloatExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

NiRTTI NiFloatExtraDataController::ms_RTTI("NiFloatExtraDataController", &NiExtraDataController::ms_RTTI);

NiFloatExtraDataController::NiFloatExtraDataController() {}

NiFloatExtraDataController::~NiFloatExtraDataController() {}

const NiRTTI* NiFloatExtraDataController::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* NiFloatExtraDataController::CreateClone(NiCloningProcess& kCloning) {
	void* v3 = NiMemObject::operator new(sizeof(NiFloatExtraDataController));
	if (v3) {
		NiFloatExtraDataController* pkClone = ::new (v3) NiFloatExtraDataController;
		CopyMembers(pkClone, kCloning);
		return pkClone;
	} else {
		CopyMembers(0, kCloning);
		return 0;
	}
}

// Files before 10.1.0.104 also store the link to an old-style float data, which LinkObject resolves.
// NiColorExtraDataController::LoadBinary and the NiPSysModifierCtlr float and bool controllers' are the same function.
void NiFloatExtraDataController::LoadBinary(NiStream& kStream) {
	NiExtraDataController::LoadBinary(kStream);
	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104))
		kStream.ReadLinkID();
}

// Files before 10.1.0.104 link an old-style float data: it becomes the controller's float interpolator.
void NiFloatExtraDataController::LinkObject(NiStream& kStream) {
	NiExtraDataController::LinkObject(kStream);
	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
		NiFloatData* pkFloatData = static_cast<NiFloatData*>(kStream.GetObjectFromLinkID());
		void* pvMem = NiMemObject::operator new(sizeof(NiFloatInterpolator));
		NiFloatInterpolator* pkInterp = pvMem ? ::new (pvMem) NiFloatInterpolator(pkFloatData) : nullptr;
		// The old interpolator is released before the new one is held.
		m_spInterpolator = pkInterp;
		m_spInterpolator.m_pObject->Collapse();
	}
}

// NiColorExtraDataController, NiFloatsExtraDataController and NiFloatsExtraDataPoint3Controller use the same function.
bool NiFloatExtraDataController::RegisterStreamables(NiStream& kStream) {
	return NiExtraDataController::RegisterStreamables(kStream);
}

// NiColorExtraDataController and the NiPSysModifierCtlr float and bool controllers use the same function.
void NiFloatExtraDataController::SaveBinary(NiStream& kStream) {
	NiExtraDataController::SaveBinary(kStream);
}

// NiColorExtraDataController uses the same function.
bool NiFloatExtraDataController::IsEqual(NiObject* pkObject) {
	return NiExtraDataController::IsEqual(pkObject);
}

void NiFloatExtraDataController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiExtraDataController::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// A manager-controlled controller always samples its interpolator, at the invalid time (-FLT_MAX). The value is
// written straight into the extra data.
void NiFloatExtraDataController::Update(NiUpdateData& UpdateData) {
	if (m_usFlags & MANAGERCONTROLLED_MASK)
		m_fScaledTime = -FLT_MAX;
	else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate()))
		return;

	if (!m_spInterpolator)
		return;

	float fValue;
	if (m_spInterpolator->Update(m_fScaledTime, m_pkTarget, fValue) && m_spExtraData)
		static_cast<NiFloatExtraData*>(static_cast<NiExtraData*>(m_spExtraData))->m_fValue = fValue;
}

NiInterpolator* NiFloatExtraDataController::CreatePoseInterpolator(uint16_t usIndex) {
	NiFloatInterpolator* v3 = static_cast<NiFloatInterpolator*>(NiMemObject::operator new(sizeof(NiFloatInterpolator)));
	if (v3) {
		// The source's NiSmartPointerCast(NiFloatExtraData, m_spExtraData): this controller's extra data is a float.
		NiFloatExtraData* pkData = static_cast<NiFloatExtraData*>(m_spExtraData.m_pObject);
		return ::new (v3) NiFloatInterpolator(pkData->m_fValue);
	} else {
		return 0;
	}
}

// apInterp must be a NiFloatInterpolator; the extra data is not null-checked.
void NiFloatExtraDataController::SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t) {
	NiFloatInterpolator* pkInterp = apInterp
		? static_cast<NiFloatInterpolator*>(apInterp->DynamicCastMember(&NiFloatInterpolator::ms_RTTI))
		: nullptr;
	pkInterp->SetPoseValue(static_cast<NiFloatExtraData*>(static_cast<NiExtraData*>(m_spExtraData))->m_fValue);
}

NiBlendInterpolator* NiFloatExtraDataController::CreateBlendInterpolator(uint16_t, int32_t bManagerControlled, bool,
	float fWeightThreshold, int32_t ucArraySize) const {
	void* pvMem = NiMemObject::operator new(sizeof(NiBlendFloatInterpolator));
	return pvMem ? ::new (pvMem) NiBlendFloatInterpolator(bManagerControlled, fWeightThreshold, ucArraySize) : nullptr;
}

bool NiFloatExtraDataController::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
	return pkInterpolator->IsFloatValueSupported();
}

NiObject* NiFloatExtraDataController::CreateObject() {
	void* v0 = NiMemObject::operator new(sizeof(NiFloatExtraDataController));
	if (!v0) {
		return 0;
	}
	return ::new (v0) NiFloatExtraDataController;
}
