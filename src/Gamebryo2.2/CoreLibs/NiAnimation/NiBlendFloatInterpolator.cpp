#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendFloatInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBlendFloatInterpolator::ms_RTTI("NiBlendFloatInterpolator", &NiBlendInterpolator::ms_RTTI);

NiBlendFloatInterpolator::NiBlendFloatInterpolator() : NiBlendInterpolator(), m_fFloatValue(-3.4028235e+38f) {}

NiBlendFloatInterpolator::NiBlendFloatInterpolator(int32_t abManagerControlled, float afWeightThreshold,
	int32_t aucArraySize)
	: NiBlendInterpolator(abManagerControlled, afWeightThreshold, aucArraySize),
	  m_fFloatValue(-0x1.fffffe0000000p+127f) {}

NiBlendFloatInterpolator::~NiBlendFloatInterpolator() {}

NiObject* NiBlendFloatInterpolator::CreateClone(NiCloningProcess& kCloning) {
	NiBlendFloatInterpolator* pkObject =
		static_cast<NiBlendFloatInterpolator*>(NiMemObject::operator new(sizeof(NiBlendFloatInterpolator)));
	NiBlendFloatInterpolator* pkObject_2 = pkObject;
	if (pkObject) {
		::new (pkObject) NiBlendFloatInterpolator;
	} else {
		pkObject_2 = 0;
	}
	CopyMembers(pkObject_2, kCloning);
	pkObject_2->m_fFloatValue = m_fFloatValue;
	return pkObject_2;
}

void NiBlendFloatInterpolator::LoadBinary(NiStream& kStream) {
	NiBlendInterpolator::LoadBinary(kStream);
	NiStreamLoadBinary(kStream, m_fFloatValue);
}

void NiBlendFloatInterpolator::LinkObject(NiStream& kStream) {
	NiBlendInterpolator::LinkObject(kStream);
}

bool NiBlendFloatInterpolator::RegisterStreamables(NiStream& kStream) {
	return NiBlendInterpolator::RegisterStreamables(kStream);
}

void NiBlendFloatInterpolator::SaveBinary(NiStream& kStream) {
	NiBlendInterpolator::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_fFloatValue);
}

bool NiBlendFloatInterpolator::IsEqual(NiObject* pkObject) {
	if (!NiBlendInterpolator::IsEqual(pkObject))
		return false;
	NiBlendFloatInterpolator* pkOther = static_cast<NiBlendFloatInterpolator*>(pkObject);
	return pkOther->m_fFloatValue == m_fFloatValue;
}

void NiBlendFloatInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiBlendInterpolator::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("m_fFloatValue", m_fFloatValue));
}

bool NiBlendFloatInterpolator::Update(float fTime, NiObjectNET* pkInterpTarget, float& fValue) {
	bool bReturnValue = false;
	if (m_ucInterpCount == 1) {
		bReturnValue = StoreSingleValue(fTime, pkInterpTarget, fValue);
	} else if (m_ucInterpCount > 0) {
		ComputeNormalizedWeights();
		bReturnValue = BlendValues(fTime, pkInterpTarget, fValue);
	}
	m_fLastTime = fTime;
	return bReturnValue;
}

bool NiBlendFloatInterpolator::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const {
	return pkInterpolator->IsFloatValueSupported();
}

bool NiBlendFloatInterpolator::StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, float& fValue) {
	if (!GetSingleUpdateTime(fTime))
		return false;
	if (m_pkSingleInterpolator->Update(fTime, pkInterpTarget, m_fFloatValue)) {
		fValue = m_fFloatValue;
		return true;
	}
	m_fFloatValue = INVALID_FLOAT;
	fValue = INVALID_FLOAT;
	return false;
}

bool NiBlendFloatInterpolator::BlendValues(float fTime, NiObjectNET* pkInterpTarget, float& fValue) {
	m_fFloatValue = 0.0f;
	float fTotalSumOfWeights = 1.0f;
	bool bValidValue = false;
	for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
		InterpArrayItem& kItem = m_pkInterpArray[uc];
		if (!kItem.m_spInterpolator || !(kItem.m_fNormalizedWeight > 0.0f))
			continue;
		float fUpdateTime = fTime;
		if (!GetUpdateTimeForItem(fUpdateTime, kItem)) {
			fTotalSumOfWeights -= kItem.m_fNormalizedWeight;
			continue;
		}
		float fValue;
		if (kItem.m_spInterpolator->Update(fUpdateTime, pkInterpTarget, fValue)) {
			bValidValue = true;
			m_fFloatValue = kItem.m_fNormalizedWeight * fValue + m_fFloatValue;
		} else {
			fTotalSumOfWeights -= kItem.m_fNormalizedWeight;
		}
	}

	// divided even when nothing was valid; the value is overwritten then
	m_fFloatValue = m_fFloatValue / fTotalSumOfWeights;
	if (bValidValue) {
		fValue = m_fFloatValue;
		return true;
	}
	fValue = INVALID_FLOAT;
	m_fFloatValue = INVALID_FLOAT;
	return false;
}

NiObject* NiBlendFloatInterpolator::CreateObject() {
	NiBlendFloatInterpolator* pkObject =
		static_cast<NiBlendFloatInterpolator*>(NiMemObject::operator new(sizeof(NiBlendFloatInterpolator)));
	if (!pkObject) {
		return 0;
	}
	::new (pkObject) NiBlendFloatInterpolator;
	return pkObject;
}
