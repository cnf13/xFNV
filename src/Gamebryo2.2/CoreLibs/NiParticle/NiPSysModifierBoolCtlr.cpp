#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierBoolCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolTimelineInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"

#include <cfloat>

NiRTTI NiPSysModifierBoolCtlr::ms_RTTI("NiPSysModifierBoolCtlr", &NiPSysModifierCtlr::ms_RTTI);

NiPSysModifierBoolCtlr::NiPSysModifierBoolCtlr() {}

NiPSysModifierBoolCtlr::~NiPSysModifierBoolCtlr() {}

void NiPSysModifierBoolCtlr::LoadBinary(NiStream& kStream) {
	NiPSysModifierCtlr::LoadBinary(kStream);
	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104))
		kStream.ReadLinkID();
}

// Files older than 10.1.0.104 carry a link to old-style bool data; it becomes a timeline interpolator.
void NiPSysModifierBoolCtlr::LinkObject(NiStream& kStream) {
	NiPSysModifierCtlr::LinkObject(kStream);
	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
		NiBoolData* pkData = static_cast<NiBoolData*>(kStream.GetObjectFromLinkID());
		void* pvMem = NiMemObject::operator new(sizeof(NiBoolTimelineInterpolator));
		NiBoolTimelineInterpolator* pkInterp = pvMem ? ::new (pvMem) NiBoolTimelineInterpolator(pkData) : nullptr;
		m_spInterpolator = pkInterp;
	}
}

bool NiPSysModifierBoolCtlr::RegisterStreamables(NiStream& kStream) {
	return NiPSysModifierCtlr::RegisterStreamables(kStream);
}

void NiPSysModifierBoolCtlr::SaveBinary(NiStream& kStream) {
	NiPSysModifierCtlr::SaveBinary(kStream);
}

// NiPSysModifierFloatCtlr uses the same function.
bool NiPSysModifierBoolCtlr::IsEqual(NiObject* pkObject) {
	return NiPSysModifierCtlr::IsEqual(pkObject);
}

void NiPSysModifierBoolCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPSysModifierCtlr::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

void NiPSysModifierBoolCtlr::Update(NiUpdateData& UpdateData) {
	if (m_usFlags & MANAGERCONTROLLED_MASK)
		m_fScaledTime = -FLT_MAX;
	else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate()))
		return;

	if (!m_spInterpolator)
		return;

	bool bValue;
	if (m_spInterpolator->Update(m_fScaledTime, m_pkTarget, bValue))
		SetTargetBoolValue(bValue);
}

NiInterpolator* NiPSysModifierBoolCtlr::CreatePoseInterpolator(uint16_t) {
	bool bValue;
	GetTargetBoolValue(bValue);
	void* pvMem = NiMemObject::operator new(sizeof(NiBoolInterpolator));
	return pvMem ? ::new (pvMem) NiBoolInterpolator(bValue) : nullptr;
}

// Sets the pose of apInterp, which must be a NiBoolInterpolator, to the target's current value. The exe tests apInterp
// for null and calls the member DynamicCast out of line.
void NiPSysModifierBoolCtlr::SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t) {
	NiBoolInterpolator* pkInterp = apInterp
		? static_cast<NiBoolInterpolator*>(apInterp->DynamicCastMember(&NiBoolInterpolator::ms_RTTI))
		: nullptr;
	bool bValue;
	GetTargetBoolValue(bValue);
	pkInterp->SetPoseValue(bValue);
}

// NiBoolInterpController uses the same function.
NiBlendInterpolator* NiPSysModifierBoolCtlr::CreateBlendInterpolator(uint16_t, int32_t bManagerControlled, bool,
	float fWeightThreshold, int32_t ucArraySize) const {
	void* pvMem = NiMemObject::operator new(sizeof(NiBlendBoolInterpolator));
	return pvMem ? ::new (pvMem) NiBlendBoolInterpolator(bManagerControlled, fWeightThreshold, ucArraySize) : nullptr;
}

bool NiPSysModifierBoolCtlr::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
	return pkInterpolator->IsBoolValueSupported();
}
