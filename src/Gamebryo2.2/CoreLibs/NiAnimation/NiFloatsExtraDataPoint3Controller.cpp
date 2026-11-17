#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatsExtraDataPoint3Controller.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendPoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiExtraDataController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFloatsExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cfloat>
#include <cstring>

NiRTTI NiFloatsExtraDataPoint3Controller::ms_RTTI("NiFloatsExtraDataPoint3Controller", &NiExtraDataController::ms_RTTI);

NiFloatsExtraDataPoint3Controller::NiFloatsExtraDataPoint3Controller() : m_iFloatsExtraDataIndex(-1) {}

NiFloatsExtraDataPoint3Controller::~NiFloatsExtraDataPoint3Controller() {}

const NiRTTI* NiFloatsExtraDataPoint3Controller::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* NiFloatsExtraDataPoint3Controller::CreateClone(NiCloningProcess& kCloning) {
	void* pkObject = NiMemObject::operator new(sizeof(NiFloatsExtraDataPoint3Controller));
	NiFloatsExtraDataPoint3Controller* pkObject_2 = 0;
	if (pkObject) {
		pkObject_2 = ::new (pkObject) NiFloatsExtraDataPoint3Controller;
	}
	CopyMembers(pkObject_2, kCloning);
	pkObject_2->m_iFloatsExtraDataIndex = m_iFloatsExtraDataIndex;
	return pkObject_2;
}

void NiFloatsExtraDataPoint3Controller::LoadBinary(NiStream& kStream) {
	NiExtraDataController::LoadBinary(kStream);
	NiStreamLoadBinary(kStream, m_iFloatsExtraDataIndex);
}

void NiFloatsExtraDataPoint3Controller::LinkObject(NiStream& kStream) {
	NiExtraDataController::LinkObject(kStream);
}

bool NiFloatsExtraDataPoint3Controller::RegisterStreamables(NiStream& kStream) {
	return NiExtraDataController::RegisterStreamables(kStream);
}

// NiFloatsExtraDataController::SaveBinary is the same function.
void NiFloatsExtraDataPoint3Controller::SaveBinary(NiStream& kStream) {
	NiExtraDataController::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_iFloatsExtraDataIndex);
}

// NiFloatsExtraDataController::IsEqual is the same function.
bool NiFloatsExtraDataPoint3Controller::IsEqual(NiObject* pkObject) {
	if (!NiExtraDataController::IsEqual(pkObject))
		return false;

	return m_iFloatsExtraDataIndex ==
		static_cast<NiFloatsExtraDataPoint3Controller*>(pkObject)->m_iFloatsExtraDataIndex;
}

void NiFloatsExtraDataPoint3Controller::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiExtraDataController::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("m_iFloatsExtraDataIndex", m_iFloatsExtraDataIndex));
}

// A manager-controlled controller always samples its interpolator, at the invalid time (-FLT_MAX). The vector goes
// into three consecutive elements of the floats extra data.
void NiFloatsExtraDataPoint3Controller::Update(NiUpdateData& UpdateData) {
	if (m_usFlags & MANAGERCONTROLLED_MASK)
		m_fScaledTime = -FLT_MAX;
	else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate()))
		return;

	if (!m_spInterpolator)
		return;

	NiPoint3 kValue;
	if (!m_spInterpolator->Update(m_fScaledTime, m_pkTarget, kValue))
		return;

	NiFloatsExtraData* pkData = static_cast<NiFloatsExtraData*>(static_cast<NiExtraData*>(m_spExtraData));
	if (!pkData)
		return;

	pkData->SetValue(m_iFloatsExtraDataIndex, kValue.x);
	pkData->SetValue(m_iFloatsExtraDataIndex + 1, kValue.y);
	pkData->SetValue(m_iFloatsExtraDataIndex + 2, kValue.z);
}

// The ID is the extra data name with the first element's index, e.g. "Name[3]", built on first use.
// NiFloatsExtraDataController::GetCtlrID is the same function.
const char* NiFloatsExtraDataPoint3Controller::GetCtlrID() {
	if (m_kCtlrID.GetHandle() == NiGlobalStringTable::NULL_STRING) {
		m_kCtlrID = static_cast<const char*>(nullptr);
		const char* pcName = m_kExtraDataName;
		if (pcName) {
			size_t stLen = strlen(pcName) + 15;
			char* pcID = static_cast<char*>(_NiMalloc(stLen));
			NiSprintf(pcID, stLen, "%s[%d]", pcName, m_iFloatsExtraDataIndex);
			m_kCtlrID = static_cast<const char*>(pcID);
			_NiFree(pcID);
		}
	}
	return m_kCtlrID;
}

// The pose is the three consecutive floats of the extra data, which is not null-checked.
NiInterpolator* NiFloatsExtraDataPoint3Controller::CreatePoseInterpolator(uint16_t) {
	NiFloatsExtraData* pkData = static_cast<NiFloatsExtraData*>(static_cast<NiExtraData*>(m_spExtraData));
	NiPoint3 kValue;
	kValue.x = pkData->GetValue(m_iFloatsExtraDataIndex);
	kValue.y = pkData->GetValue(m_iFloatsExtraDataIndex + 1);
	kValue.z = pkData->GetValue(m_iFloatsExtraDataIndex + 2);
	void* pvMem = NiMemObject::operator new(sizeof(NiPoint3Interpolator));
	if (!pvMem)
		return nullptr;
	return ::new (pvMem) NiPoint3Interpolator(kValue);
}

// apInterp must be a NiPoint3Interpolator; the extra data is not null-checked.
void NiFloatsExtraDataPoint3Controller::SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t) {
	NiPoint3Interpolator* pkInterp = apInterp
		? static_cast<NiPoint3Interpolator*>(apInterp->DynamicCastMember(&NiPoint3Interpolator::ms_RTTI))
		: nullptr;
	NiFloatsExtraData* pkData = static_cast<NiFloatsExtraData*>(static_cast<NiExtraData*>(m_spExtraData));
	NiPoint3 kValue;
	kValue.x = pkData->GetValue(m_iFloatsExtraDataIndex);
	kValue.y = pkData->GetValue(m_iFloatsExtraDataIndex + 1);
	kValue.z = pkData->GetValue(m_iFloatsExtraDataIndex + 2);
	pkInterp->SetPoseValue(kValue);
}

// A new blend interpolator for the animated point, which the controller then feeds its sources.
NiBlendInterpolator* NiFloatsExtraDataPoint3Controller::CreateBlendInterpolator(uint16_t, int32_t bManagerControlled,
	bool, float fWeightThreshold, int32_t ucArraySize) const {
	void* pvMemory = NiMemObject::operator new(sizeof(NiBlendPoint3Interpolator));
	if (!pvMemory) {
		return nullptr;
	}
	return ::new (pvMemory) NiBlendPoint3Interpolator(bManagerControlled, fWeightThreshold, ucArraySize);
}

bool NiFloatsExtraDataPoint3Controller::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
	return pkInterpolator->IsPoint3ValueSupported();
}

NiObject* NiFloatsExtraDataPoint3Controller::CreateObject() {
	void* pkObject = NiMemObject::operator new(sizeof(NiFloatsExtraDataPoint3Controller));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiFloatsExtraDataPoint3Controller;
}
