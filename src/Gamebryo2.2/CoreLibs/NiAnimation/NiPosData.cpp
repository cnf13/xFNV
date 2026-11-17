#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosData.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiPosData::ms_RTTI("NiPosData", &NiObject::ms_RTTI);

NiPosData::~NiPosData() {
	if (m_pkKeys) {
		NiAnimationKey::ms_deletes[m_eType + 6](m_pkKeys);
	}
}

const NiRTTI* NiPosData::GetRTTI() const {
	return &ms_RTTI;
}

// KEEP, unregistered (niread): the exe reads the key count into its arStream argument slot (lea [esp+0x24] at
// 0xA49264) and the key type into an uninitialised local (0xA49293), and uses both; a short read leaves them
// unwritten: an uninitialised read.
void NiPosData::LoadBinary(NiStream& stream) {
	NiObject::LoadBinary(stream);

	uint32_t uiNumKeys;
	NiStreamLoadBinary(stream, uiNumKeys);
	if (uiNumKeys) {
		NiPosKey::KeyType eType;
		NiStreamLoadBinary(stream, eType);
		m_ucKeySize = NiPosKey::GetKeySize(eType);
		NiPosKey* pkKeys =
			static_cast<NiPosKey*>(NiAnimationKey::GetCreateFunction(NiAnimationKey::POSKEY, eType)(stream, uiNumKeys));
		NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::POSKEY, eType)(pkKeys, uiNumKeys, m_ucKeySize);
		ReplaceAnim(pkKeys, uiNumKeys, eType);
	}
}

void NiPosData::LinkObject(NiStream& stream) {
	NiObject::LinkObject(stream);
}

bool NiPosData::RegisterStreamables(NiStream& stream) {
	return NiObject::RegisterStreamables(stream);
}

void NiPosData::SaveBinary(NiStream& stream) {
	NiObject::SaveBinary(stream);

	NiStreamSaveBinary(stream, m_uiNumKeys);
	if (m_uiNumKeys) {
		NiStreamSaveBinary(stream, m_eType);
		NiAnimationKey::GetSaveFunction(NiAnimationKey::POSKEY, m_eType)(stream, m_pkKeys, m_uiNumKeys);
	}
}

bool NiPosData::IsEqual(NiObject* pkObject) {
	if (!NiObject::IsEqual(pkObject))
		return false;

	NiPosData* pkData = static_cast<NiPosData*>(pkObject);
	if (m_uiNumKeys != pkData->m_uiNumKeys || m_eType != pkData->m_eType || m_ucKeySize != pkData->m_ucKeySize)
		return false;

	NiAnimationKey::EqualFunction pfnEqual = NiAnimationKey::GetEqualFunction(NiAnimationKey::POSKEY, m_eType);
	for (uint32_t i = 0; i < m_uiNumKeys; i++) {
		if (!pfnEqual(*m_pkKeys->GetKeyAt(i, m_ucKeySize), *pkData->m_pkKeys->GetKeyAt(i, m_ucKeySize)))
			return false;
	}

	return true;
}

void NiPosData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
	NiObject::GetViewerStrings(pStrings);
	pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pStrings->Add(NiGetViewerString("m_uiNumKeys", m_uiNumKeys));
}

// Adds keys at the start and end times when the array lacks them, reallocating it and updating its count.
void NiPosData::GuaranteeKeysAtStartAndEnd(float fStartTime, float fEndTime) {
	NiAnimationKey* pkKeys = m_pkKeys;
	NiAnimationKey::GuaranteeKeyAtStartAndEnd(NiAnimationKey::POSKEY, m_eType, pkKeys, m_uiNumKeys, fStartTime,
		fEndTime);
	m_pkKeys = static_cast<NiPosKey*>(pkKeys);
}

// A deep copy of this data reduced to the keys between the start and end times, stored in aspNewData.
void NiPosData::GetSequenceData(float fStartTime, float fEndTime, NiPointer<NiPosData>& aspPosData) {
	NiPointer<NiObject> spCopy;
	CreateDeepCopy(spCopy);
	NiPointer<NiPosData> spNewData = static_cast<NiPosData*>(spCopy.m_pObject);
	if (m_uiNumKeys) {
		NiAnimationKey* pkNewKeys = nullptr;
		uint32_t uiNewNumKeys = 0;
		NiAnimationKey::CopySequence(NiAnimationKey::POSKEY, m_eType, m_pkKeys, m_uiNumKeys, fStartTime, fEndTime,
			pkNewKeys, uiNewNumKeys);
		spNewData->ReplaceAnim(static_cast<NiPosKey*>(pkNewKeys), uiNewNumKeys, m_eType);
	}
	aspPosData = spNewData;
}

// Frees the current keys and takes over pkKeys; with no keys the data is left empty.
void NiPosData::ReplaceAnim(NiPosKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
	if (m_pkKeys)
		NiAnimationKey::GetDeleteFunction(NiAnimationKey::POSKEY, m_eType)(m_pkKeys);

	if (pkKeys && uiNumKeys) {
		m_uiNumKeys = uiNumKeys;
		m_pkKeys = pkKeys;
		m_eType = eType;
		m_ucKeySize = NiPosKey::GetKeySize(eType);
	} else {
		m_uiNumKeys = 0;
		m_pkKeys = nullptr;
		m_eType = NiAnimationKey::NOINTERP;
		m_ucKeySize = 0;
	}
}

void NiPosData::SetAnim(NiPosKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
	if (pkKeys && uiNumKeys) {
		uint8_t v4 = NiAnimationKey::ms_keysizes[eType + 6];
		m_pkKeys = pkKeys;
		m_uiNumKeys = uiNumKeys;
		m_ucKeySize = v4;
		m_eType = eType;
	} else {
		m_uiNumKeys = 0;
		m_pkKeys = 0;
		m_ucKeySize = 0;
		m_eType = NiAnimationKey::NOINTERP;
	}
}

NiObject* NiPosData::CreateObject() {
	NiPosData* pkObject = static_cast<NiPosData*>(NiMemObject::operator new(sizeof(NiPosData)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiPosData;
}
