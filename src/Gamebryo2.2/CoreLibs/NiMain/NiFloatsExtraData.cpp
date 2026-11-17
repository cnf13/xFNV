#include "Gamebryo2.2/CoreLibs/NiMain/NiFloatsExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiFloatsExtraData::ms_RTTI("NiFloatsExtraData", &NiExtraData::ms_RTTI);

NiFloatsExtraData::NiFloatsExtraData() : m_uiSize(0), m_pfValue(nullptr) {}

NiFloatsExtraData::~NiFloatsExtraData() {
	_NiFree(m_pfValue);
}

const NiRTTI* NiFloatsExtraData::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* NiFloatsExtraData::CreateClone(NiCloningProcess& kCloning) {
	NiExtraData* pkObject = static_cast<NiExtraData*>(NiMemObject::operator new(sizeof(NiFloatsExtraData)));
	NiFloatsExtraData* pkObject_2 = static_cast<NiFloatsExtraData*>(pkObject);
	if (pkObject) {
		::new (pkObject) NiFloatsExtraData;
		CopyMembers(pkObject_2, kCloning);
		return pkObject_2;
	} else {
		CopyMembers(0, kCloning);
		return 0;
	}
}

// Also NiIntegersExtraData::LoadBinary (one function in the exe). The old array is not freed.
void NiFloatsExtraData::LoadBinary(NiStream& kStream) {
	NiExtraData::LoadBinary(kStream);
	NiStreamLoadBinary(kStream, m_uiSize);
	if (m_uiSize > 0) {
		m_pfValue = static_cast<float*>(_NiMalloc(m_uiSize * sizeof(float)));
		NiStreamLoadBinary(kStream, m_pfValue, m_uiSize);
	} else {
		m_pfValue = nullptr;
	}
}

void NiFloatsExtraData::LinkObject(NiStream& kStream) {
	NiExtraData::LinkObject(kStream);
}

bool NiFloatsExtraData::RegisterStreamables(NiStream& kStream) {
	return NiExtraData::RegisterStreamables(kStream);
}

// Also NiIntegersExtraData::SaveBinary (one function in the exe). The array is written even when empty.
void NiFloatsExtraData::SaveBinary(NiStream& kStream) {
	NiExtraData::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_uiSize);
	NiStreamSaveBinary(kStream, m_pfValue, m_uiSize);
}

// Only the arrays are compared: neither the class nor the name is checked. Unordered (NaN) values are unequal.
bool NiFloatsExtraData::IsEqual(NiObject* pObject) {
	if (!pObject)
		return false;

	NiFloatsExtraData* pkOther = static_cast<NiFloatsExtraData*>(pObject);
	if (m_uiSize != pkOther->m_uiSize)
		return false;

	if ((m_pfValue && !pkOther->m_pfValue) || (!m_pfValue && pkOther->m_pfValue))
		return false;

	if (m_pfValue) {
		for (uint32_t i = 0; i < m_uiSize; i++) {
			if (m_pfValue[i] != pkOther->m_pfValue[i])
				return false;
		}
	}

	return true;
}

void NiFloatsExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
	NiExtraData::GetViewerStrings(pStrings);
	pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pStrings->Add(NiGetViewerString("m_uiSize", m_uiSize));
	for (uint32_t i = 0; i < m_uiSize; i++)
		pStrings->Add(NiGetViewerString("m_pfValue[i]", m_pfValue[i]));
}

bool NiFloatsExtraData::SetValue(uint32_t uiIndex, float fValue) {
	if (uiIndex >= m_uiSize)
		return false;

	m_pfValue[uiIndex] = fValue;
	return true;
}

float NiFloatsExtraData::GetValue(uint32_t uiIndex) const {
	if (uiIndex >= m_uiSize)
		return 0.0f;

	return m_pfValue[uiIndex];
}

// apDest's old array is not freed.
void NiFloatsExtraData::CopyMembers(NiFloatsExtraData* pDest, NiCloningProcess& kCloning) {
	NiExtraData::CopyMembers(pDest, kCloning);
	if (!m_pfValue || m_uiSize == 0) {
		pDest->m_pfValue = nullptr;
		pDest->m_uiSize = 0;
		return;
	}

	pDest->m_uiSize = m_uiSize;
	pDest->m_pfValue = static_cast<float*>(_NiMalloc(m_uiSize * sizeof(float)));
	for (uint32_t i = 0; i < m_uiSize; i++)
		pDest->m_pfValue[i] = m_pfValue[i];
}

NiObject* NiFloatsExtraData::CreateObject() {
	NiExtraData* pkObject = static_cast<NiExtraData*>(NiMemObject::operator new(sizeof(NiFloatsExtraData)));
	NiFloatsExtraData* pkObject_2 = static_cast<NiFloatsExtraData*>(pkObject);
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiFloatsExtraData;
}
