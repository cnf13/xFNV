#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiFloatInterpolator::ms_RTTI("NiFloatInterpolator", &NiKeyBasedInterpolator::ms_RTTI);

NiFloatInterpolator::NiFloatInterpolator() : NiFloatInterpolator(static_cast<NiFloatData*>(nullptr)) {}

NiFloatInterpolator::NiFloatInterpolator(NiFloatData* pkFloatData)
	: m_fFloatValue(-3.4028235e+38f), m_spFloatData(pkFloatData), m_uiLastIdx(0) {}

NiFloatInterpolator::NiFloatInterpolator(float fPoseValue)
	: m_fFloatValue(fPoseValue), m_spFloatData(nullptr), m_uiLastIdx(0) {}

NiFloatInterpolator::~NiFloatInterpolator() {}

const NiRTTI* NiFloatInterpolator::GetRTTI() const {
	return &ms_RTTI;
}

// The clone is constructed through NiFloatInterpolator(NiFloatData*) with no data (Gamebryo's default argument).
NiObject* NiFloatInterpolator::CreateClone(NiCloningProcess& kCloning) {
	void* v3 = NiMemObject::operator new(sizeof(NiFloatInterpolator));
	if (v3) {
		NiFloatInterpolator* v4 = ::new (v3) NiFloatInterpolator(static_cast<NiFloatData*>(nullptr));
		CopyMembers(v4, kCloning);
		return v4;
	} else {
		CopyMembers(nullptr, kCloning);
		return nullptr;
	}
}

void NiFloatInterpolator::LoadBinary(NiStream& kStream) {
	NiKeyBasedInterpolator::LoadBinary(kStream);
	NiStreamLoadBinary(kStream, m_fFloatValue);
	m_spFloatData = static_cast<NiFloatData*>(kStream.ResolveLinkID());
}

// NiBoolInterpolator, NiColorInterpolator, NiPoint3Interpolator and NiQuaternionInterpolator use the same function.
void NiFloatInterpolator::LinkObject(NiStream& kStream) {
	NiKeyBasedInterpolator::LinkObject(kStream);
}

// NiBoolInterpolator uses the same function.
bool NiFloatInterpolator::RegisterStreamables(NiStream& kStream) {
	if (!NiKeyBasedInterpolator::RegisterStreamables(kStream)) {
		return false;
	}
	if (m_spFloatData) {
		m_spFloatData->RegisterStreamables(kStream);
	}
	return true;
}

void NiFloatInterpolator::SaveBinary(NiStream& kStream) {
	NiKeyBasedInterpolator::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_fFloatValue);
	kStream.SaveLinkID(m_spFloatData);
}

bool NiFloatInterpolator::IsEqual(NiObject* pkObject) {
	if (!NiKeyBasedInterpolator::IsEqual(pkObject)) {
		return false;
	}

	NiFloatInterpolator* pkOther = static_cast<NiFloatInterpolator*>(pkObject);
	if (m_fFloatValue != pkOther->m_fFloatValue) {
		return false;
	}
	if ((m_spFloatData && !pkOther->m_spFloatData) || (!m_spFloatData && pkOther->m_spFloatData)) {
		return false;
	}
	if (m_spFloatData && !m_spFloatData->IsEqual(pkOther->m_spFloatData)) {
		return false;
	}
	return true;
}

void NiFloatInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiKeyBasedInterpolator::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("m_fFloatValue", m_fFloatValue));
	pkStrings->Add(NiGetViewerString("m_spFloatData", static_cast<const void*>(m_spFloatData)));
}

// At an unchanged time the cached value is returned. Fails while the value is invalid.
bool NiFloatInterpolator::Update(float fTime, NiObjectNET*, float& fValue) {
	if (m_fLastTime == fTime) {
		fValue = m_fFloatValue;
		return m_fFloatValue != INVALID_FLOAT;
	}

	if (m_spFloatData) {
		uint32_t uiNumKeys = m_spFloatData->m_uiNumKeys;
		NiFloatKey* pkKeys = m_spFloatData->m_pkKeys;
		NiFloatKey::KeyType eType = m_spFloatData->m_eType;
		uint8_t ucKeySize = m_spFloatData->m_ucKeySize;
		if (uiNumKeys > 0) {
			m_fFloatValue = NiFloatKey::GenInterp(fTime, pkKeys, eType, uiNumKeys, m_uiLastIdx, ucKeySize);
		}
	}

	if (m_fFloatValue == INVALID_FLOAT) {
		return false;
	}
	fValue = m_fFloatValue;
	m_fLastTime = fTime;
	return true;
}

// Replaces keys that hold a single value by that value: no keys leave the value invalid.
void NiFloatInterpolator::Collapse() {
	if (!m_spFloatData) {
		return;
	}

	uint32_t uiNumKeys = m_spFloatData->m_uiNumKeys;
	NiFloatKey* pkKeys = m_spFloatData->m_pkKeys;
	NiFloatKey::KeyType eType = m_spFloatData->m_eType;
	uint8_t ucKeySize = m_spFloatData->m_ucKeySize;
	if (uiNumKeys == 0) {
		m_spFloatData = nullptr;
		m_fFloatValue = INVALID_FLOAT;
		return;
	}

	float fValue = pkKeys->GetKeyAt(0, ucKeySize)->GetValue();
	bool bCollapse = true;
	if (uiNumKeys != 1) {
		if (eType != NiAnimationKey::LINKEY && eType != NiAnimationKey::STEPKEY) {
			return;
		}
		for (uint32_t ui = 1; ui < uiNumKeys && bCollapse; ui++) {
			if (pkKeys->GetKeyAt(ui, ucKeySize)->GetValue() != fValue) {
				bCollapse = false;
			}
		}
	}

	if (bCollapse) {
		m_spFloatData = nullptr;
		m_fFloatValue = fValue;
	}
}

void NiFloatInterpolator::GuaranteeTimeRange(float fStartTime, float fEndTime) {
	if (m_spFloatData) {
		m_spFloatData->GuaranteeKeysAtStartAndEnd(fStartTime, fEndTime);
	}
}

NiInterpolator* NiFloatInterpolator::GetSequenceInterpolator(float fStartTime, float fEndTime) {
	NiFloatInterpolator* pkSeqInterp =
		static_cast<NiFloatInterpolator*>(NiInterpolator::GetSequenceInterpolator(fStartTime, fEndTime));
	if (m_spFloatData && pkSeqInterp) {
		NiPointer<NiFloatData> spNewData;
		m_spFloatData->GetSequenceData(fStartTime, fEndTime, spNewData);
		pkSeqInterp->SetFloatData(spNewData);
		pkSeqInterp->m_uiLastIdx = 0;
	}
	return pkSeqInterp;
}

// NiBoolInterpolator uses the same function.
uint32_t NiFloatInterpolator::GetKeyCount(uint16_t) const {
	if (!m_spFloatData) {
		return 0;
	}
	return m_spFloatData->m_uiNumKeys;
}

// NiBoolInterpolator uses the same function.
NiAnimationKey::KeyType NiFloatInterpolator::GetKeyType(uint16_t) const {
	if (!m_spFloatData) {
		return NiAnimationKey::NOINTERP;
	}
	return m_spFloatData->m_eType;
}

// NiBoolInterpolator uses the same function.
NiAnimationKey* NiFloatInterpolator::GetKeyArray(uint16_t) const {
	if (!m_spFloatData) {
		return nullptr;
	}
	return m_spFloatData->m_pkKeys;
}

// NiBoolInterpolator uses the same function.
uint8_t NiFloatInterpolator::GetKeyStride(uint16_t) const {
	if (!m_spFloatData) {
		return 0;
	}
	return m_spFloatData->m_ucKeySize;
}

// Posed: no keys, but a valid value.
bool NiFloatInterpolator::GetChannelPosed(uint16_t) const {
	if (m_spFloatData) {
		return false;
	}
	return m_fFloatValue != INVALID_FLOAT;
}

void NiFloatInterpolator::SetPoseValue(float fPoseValue) {
	m_spFloatData = nullptr;
	m_fFloatValue = fPoseValue;
}

void NiFloatInterpolator::SetFloatData(NiFloatData* pkFloatData) {
	m_spFloatData = pkFloatData;
}

NiFloatKey* NiFloatInterpolator::GetKeys(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const {
	if (m_spFloatData.m_pObject) {
		NiFloatData* v4 = m_spFloatData.m_pObject;
		return v4->GetAnim(uiNumKeys, eType, ucSize);
	} else {
		uiNumKeys = 0;
		eType = NiAnimationKey::NOINTERP;
		ucSize = 0;
		return 0;
	}
}

void NiFloatInterpolator::CopyMembers(NiFloatInterpolator* pkDest, NiCloningProcess& kCloning) {
	NiKeyBasedInterpolator::CopyMembers(pkDest, kCloning);
	pkDest->m_fFloatValue = m_fFloatValue;
	pkDest->m_spFloatData = m_spFloatData;
	pkDest->m_uiLastIdx = m_uiLastIdx;
}

// Uses the NiFloatInterpolator(NiFloatData*) constructor with no data.
NiObject* NiFloatInterpolator::CreateObject() {
	void* pkObject = NiMemObject::operator new(sizeof(NiFloatInterpolator));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiFloatInterpolator(static_cast<NiFloatData*>(nullptr));
}
