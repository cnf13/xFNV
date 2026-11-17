#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorExtraDataController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendColorInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiExtraDataController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColorExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

NiRTTI NiColorExtraDataController::ms_RTTI("NiColorExtraDataController", &NiExtraDataController::ms_RTTI);

NiColorExtraDataController::NiColorExtraDataController() : NiExtraDataController() {}

NiColorExtraDataController::~NiColorExtraDataController() {}

const NiRTTI* NiColorExtraDataController::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* NiColorExtraDataController::CreateClone(NiCloningProcess& kCloning) {
	void* v3 = NiMemObject::operator new(sizeof(NiColorExtraDataController));
	if (v3) {
		NiExtraDataController* v4 = ::new (v3) NiColorExtraDataController;
		NiExtraDataController::CopyMembers(v4, kCloning);
		return v4;
	} else {
		NiExtraDataController::CopyMembers(nullptr, kCloning);
		return nullptr;
	}
}

void NiColorExtraDataController::LoadBinary(NiStream& kStream) {
	NiExtraDataController::LoadBinary(kStream);
	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104))
		kStream.ReadLinkID();
}

void NiColorExtraDataController::LinkObject(NiStream& kStream) {
	NiExtraDataController::LinkObject(kStream);
	if ((&kStream)->GetFileVersion() < 0xA010068) {
		NiColorInterpolator* v5;
		NiColorData* pkColorData = static_cast<NiColorData*>(kStream.GetObjectFromLinkID());
		NiColorInterpolator* v4 =
			static_cast<NiColorInterpolator*>(NiMemObject::operator new(sizeof(NiColorInterpolator)));
		if (v4) {
			v5 = ::new (v4) NiColorInterpolator(pkColorData);
		} else {
			v5 = 0;
		}
		m_spInterpolator = v5;
		m_spInterpolator.m_pObject->Collapse();
	}
}

bool NiColorExtraDataController::RegisterStreamables(NiStream& kStream) {
	return NiExtraDataController::RegisterStreamables(kStream);
}

void NiColorExtraDataController::SaveBinary(NiStream& kStream) {
	NiExtraDataController::SaveBinary(kStream);
}

bool NiColorExtraDataController::IsEqual(NiObject* pkObject) {
	return NiExtraDataController::IsEqual(pkObject);
}

void NiColorExtraDataController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiExtraDataController::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// A manager-controlled controller always samples its interpolator, at the invalid time (-FLT_MAX).
void NiColorExtraDataController::Update(NiUpdateData& UpdateData) {
	if (m_usFlags & MANAGERCONTROLLED_MASK)
		m_fScaledTime = -FLT_MAX;
	else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate()))
		return;

	if (!m_spInterpolator)
		return;

	NiColorA kValue(0.0f, 0.0f, 0.0f, 0.0f);
	if (m_spInterpolator->Update(m_fScaledTime, m_pkTarget, kValue) && m_spExtraData)
		static_cast<NiColorExtraData*>(static_cast<NiExtraData*>(m_spExtraData))->SetValue(kValue);
}

// The pose interpolator holds the extra data's color (not null-checked).
NiInterpolator* NiColorExtraDataController::CreatePoseInterpolator(uint16_t usIndex) {
	NiColorInterpolator* pkInterp =
		static_cast<NiColorInterpolator*>(NiMemObject::operator new(sizeof(NiColorInterpolator)));
	if (!pkInterp) {
		return nullptr;
	}
	return ::new (pkInterp)
		NiColorInterpolator(static_cast<NiColorExtraData*>(static_cast<NiExtraData*>(m_spExtraData))->m_kColorA);
}

// apInterp must be a NiColorInterpolator; the extra data is not null-checked. The exe calls NiObject::DynamicCast
// out of line (0x653290).
void NiColorExtraDataController::SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t) {
	NiColorInterpolator* pkInterp =
		static_cast<NiColorInterpolator*>(apInterp ? apInterp->DynamicCastMember(&NiColorInterpolator::ms_RTTI)
												   : nullptr);
	pkInterp->SetPoseValue(static_cast<NiColorExtraData*>(static_cast<NiExtraData*>(m_spExtraData))->m_kColorA);
}

// IDA's Func0037. ausIndex and abAccumulateAnimations are unused.
NiBlendInterpolator* NiColorExtraDataController::CreateBlendInterpolator(uint16_t, int32_t bManagerControlled, bool,
	float fWeightThreshold, int32_t ucArraySize) const {
	void* pvMem = NiMemObject::operator new(sizeof(NiBlendColorInterpolator));
	if (!pvMem)
		return nullptr;
	return ::new (pvMem) NiBlendColorInterpolator(bManagerControlled, fWeightThreshold, ucArraySize);
}

bool NiColorExtraDataController::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
	return pkInterpolator->IsColorAValueSupported();
}

NiObject* NiColorExtraDataController::CreateObject() {
	void* v0 = NiMemObject::operator new(sizeof(NiColorExtraDataController));
	if (!v0) {
		return nullptr;
	}
	return ::new (v0) NiColorExtraDataController;
}
