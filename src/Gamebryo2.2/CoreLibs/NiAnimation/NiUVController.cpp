#include "Gamebryo2.2/CoreLibs/NiAnimation/NiUVController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiUVData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiUVController::ms_RTTI("NiUVController", &NiTimeController::ms_RTTI);

NiUVController::NiUVController()
	: m_uiLastUOffsetIdx(0), m_uiLastUTilingIdx(0), m_uiLastVOffsetIdx(0), m_uiLastVTilingIdx(0), m_usTextureSet(0),
	  m_bNeedPreDisplay(false) {}

NiUVController::~NiUVController() {
	m_spUVData = nullptr;
}

NiObject* NiUVController::CreateClone(NiCloningProcess& kCloning) {
	NiUVController* pkClone = static_cast<NiUVController*>(NiMemObject::operator new(sizeof(NiUVController)));
	if (pkClone) {
		::new (pkClone) NiUVController;
	} else {
		pkClone = nullptr;
	}
	CopyMembers(pkClone, kCloning);
	pkClone->m_uiLastUOffsetIdx = m_uiLastUOffsetIdx;
	pkClone->m_uiLastVOffsetIdx = m_uiLastVOffsetIdx;
	pkClone->m_uiLastUTilingIdx = m_uiLastUTilingIdx;
	pkClone->m_uiLastVTilingIdx = m_uiLastVTilingIdx;
	pkClone->m_usTextureSet = m_usTextureSet;
	NiUVData* pkData = m_spUVData.m_pObject;
	if (pkData) {
		pkClone->SetUVData(pkData);
	}
	return pkClone;
}

void NiUVController::LoadBinary(NiStream& kStream) {
	NiTimeController::LoadBinary(kStream);
	NiStreamLoadBinary(kStream, m_usTextureSet);
	kStream.ReadLinkID();
}

void NiUVController::LinkObject(NiStream& kStream) {
	NiTimeController::LinkObject(kStream);
	m_spUVData = static_cast<NiUVData*>(kStream.GetObjectFromLinkID());
}

bool NiUVController::RegisterStreamables(NiStream& kStream) {
	if (!NiTimeController::RegisterStreamables(kStream))
		return false;

	if (m_spUVData)
		m_spUVData->RegisterStreamables(kStream);
	return true;
}

void NiUVController::SaveBinary(NiStream& kStream) {
	NiTimeController::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_usTextureSet);
	kStream.SaveLinkID(m_spUVData);
}

bool NiUVController::IsEqual(NiObject* pkObject) {
	if (!NiTimeController::IsEqual(pkObject))
		return false;

	NiUVController* pkOther = static_cast<NiUVController*>(pkObject);
	if (m_usTextureSet != pkOther->m_usTextureSet)
		return false;

	NiUVData* pkData = m_spUVData;
	NiUVData* pkOtherData = pkOther->m_spUVData;
	if ((pkData == nullptr) != (pkOtherData == nullptr))
		return false;

	// Returns its own true after the data's IsEqual succeeds, not that call's result.
	if (pkData && !pkData->IsEqual(pkOtherData))
		return false;
	return true;
}

void NiUVController::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
	NiTimeController::GetViewerStrings(pStrings);
	pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The texture coordinates are only rewritten in OnPreDisplay, once the target is about to be drawn.
void NiUVController::Update(NiUpdateData& UpdateData) {
	if (!DontDoUpdate(UpdateData.fTime))
		m_bNeedPreDisplay = true;
}

// Resets the values last applied, so that the next OnPreDisplay starts from untransformed coordinates. The UV data
// must be set.
void NiUVController::SetTarget(NiObjectNET* pkTarget) {
	NiUVData* pkData = m_spUVData;
	pkData->m_fLastUOffset = 0.0f;
	pkData->m_fLastVOffset = 0.0f;
	pkData->m_fLastUTiling = 1.0f;
	pkData->m_fLastVTiling = 1.0f;
	NiTimeController::SetTarget(pkTarget);
}

// Offsets and scales the target's texture coordinates by the change since the values last applied. Takes the texture
// set count as m_usDataFlags & 1.
void NiUVController::OnPreDisplay() {
	if (!m_bNeedPreDisplay)
		return;
	NiGeometryData* pkTargetData = static_cast<NiGeometry*>(m_pkTarget)->m_spModelData.m_pObject;
	// EXE-INLINED none: NiGeometryData::GetTextureSets
	if (m_usTextureSet >= static_cast<uint16_t>(pkTargetData->m_usDataFlags & 1))
		return;

	uint32_t uiNumKeys;
	NiFloatKey::KeyType eType;
	uint8_t ucSize;
	float fUOffset = 0.0f;
	NiFloatKey* pkKeys = GetUOffsetData(uiNumKeys, eType, ucSize);
	if (pkKeys)
		fUOffset = NiFloatKey::GenInterp(m_fScaledTime, pkKeys, eType, uiNumKeys, m_uiLastUOffsetIdx, ucSize);
	float fVOffset = 0.0f;
	pkKeys = GetVOffsetData(uiNumKeys, eType, ucSize);
	if (pkKeys)
		fVOffset = NiFloatKey::GenInterp(m_fScaledTime, pkKeys, eType, uiNumKeys, m_uiLastVOffsetIdx, ucSize);
	float fUTiling = 1.0f;
	pkKeys = GetUTilingData(uiNumKeys, eType, ucSize);
	if (pkKeys)
		fUTiling = NiFloatKey::GenInterp(m_fScaledTime, pkKeys, eType, uiNumKeys, m_uiLastUTilingIdx, ucSize);
	float fVTiling = 1.0f;
	pkKeys = GetVTilingData(uiNumKeys, eType, ucSize);
	if (pkKeys)
		fVTiling = NiFloatKey::GenInterp(m_fScaledTime, pkKeys, eType, uiNumKeys, m_uiLastVTilingIdx, ucSize);

	NiUVData* pkUVData = m_spUVData.m_pObject;
	float fUTilingDiff = fUTiling / pkUVData->m_fLastUTiling;
	float fLastUOffset = pkUVData->m_fLastUOffset;
	float fVTilingDiff = fVTiling / pkUVData->m_fLastVTiling;
	float fLastVOffset = pkUVData->m_fLastVOffset;
	NiPoint2* pkTextureSet = pkTargetData->GetTextureSet(m_usTextureSet);
	uint16_t usNumPoints = pkTargetData->m_usVertices;
	float fUOffsetDiff = fUOffset - fLastUOffset * fUTilingDiff;
	float fDiffX = (1.0f - fUTilingDiff) * 0.5 - fUOffsetDiff;
	float fVOffsetDiff = fVOffset - fLastVOffset * fVTilingDiff;
	float fDiffY = (1.0f - fVTilingDiff) * 0.5 + fVOffsetDiff;
	for (uint16_t i = 0; i < usNumPoints; i++) {
		pkTextureSet[i].x = pkTextureSet[i].x * fUTilingDiff + fDiffX;
		pkTextureSet[i].y = pkTextureSet[i].y * fVTilingDiff + fDiffY;
	}

	pkUVData = m_spUVData.m_pObject;
	// EXE-INLINED none: NiUVData::SetLastUVOffsetTiling
	pkUVData->m_fLastUOffset = fUOffset;
	pkUVData->m_fLastVOffset = fVOffset;
	pkUVData->m_fLastUTiling = fUTiling;
	pkUVData->m_fLastVTiling = fVTiling;
	pkTargetData->MarkAsChanged(NiGeometryData::TEXTURE_MASK);
	m_bNeedPreDisplay = false;
}

bool NiUVController::TargetIsRequiredType() const {
	return NiObject::IsKindOf(&NiGeometry::ms_RTTI, m_pkTarget);
}

NiFloatKey* NiUVController::GetUOffsetData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const {
	NiUVData* m_pObject = m_spUVData.m_pObject;
	if (m_pObject) {
		uiNumKeys = m_pObject->m_uiNumUOffsetKeys;
		eType = m_pObject->m_eUOffsetType;
		ucSize = m_pObject->m_ucUOffsetSize;
		return m_pObject->m_pkUOffsetKeys;
	} else {
		uiNumKeys = 0;
		eType = NiAnimationKey::NOINTERP;
		ucSize = 0;
		return 0;
	}
}

NiFloatKey* NiUVController::GetVOffsetData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const {
	NiUVData* m_pObject = m_spUVData.m_pObject;
	if (m_pObject) {
		uiNumKeys = m_pObject->m_uiNumVOffsetKeys;
		eType = m_pObject->m_eVOffsetType;
		ucSize = m_pObject->m_ucVOffsetSize;
		return m_pObject->m_pkVOffsetKeys;
	} else {
		uiNumKeys = 0;
		eType = NiAnimationKey::NOINTERP;
		ucSize = 0;
		return 0;
	}
}

NiFloatKey* NiUVController::GetUTilingData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const {
	NiUVData* m_pObject = m_spUVData.m_pObject;
	if (m_pObject) {
		uiNumKeys = m_pObject->m_uiNumUTilingKeys;
		eType = m_pObject->m_eUTilingType;
		ucSize = m_pObject->m_ucUTilingSize;
		return m_pObject->m_pkUTilingKeys;
	} else {
		uiNumKeys = 0;
		eType = NiAnimationKey::NOINTERP;
		ucSize = 0;
		return 0;
	}
}

NiFloatKey* NiUVController::GetVTilingData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const {
	NiUVData* m_pObject = m_spUVData.m_pObject;
	if (m_pObject) {
		uiNumKeys = m_pObject->m_uiNumVTilingKeys;
		eType = m_pObject->m_eVTilingType;
		ucSize = m_pObject->m_ucVTilingSize;
		return m_pObject->m_pkVTilingKeys;
	} else {
		uiNumKeys = 0;
		eType = NiAnimationKey::NOINTERP;
		ucSize = 0;
		return 0;
	}
}

// The key time range over the four channels' first and last keys. The first channel's keys are compared with the
// extrema's constants, not with the members just set, as the game does.
void NiUVController::ResetKeyTimeExtrema() {
	NiUVData* pkData = m_spUVData.m_pObject;
	m_fLoKeyTime = 3.4028235e+38f;
	m_fHiKeyTime = -3.4028235e+38f;
	if (pkData) {
		NiFloatKey* pkKeys = pkData->m_pkUOffsetKeys;
		uint32_t uiNumKeys = pkData->m_uiNumUOffsetKeys;
		uint8_t ucSize = pkData->m_ucUOffsetSize;
		if (pkKeys) {
			if (pkKeys->GetKeyAt(0, ucSize)->GetTime() < 3.4028234663852886e+38)
				m_fLoKeyTime = pkKeys->GetKeyAt(0, ucSize)->GetTime();
			if (pkKeys->GetKeyAt(uiNumKeys - 1, ucSize)->GetTime() > -3.4028234663852886e+38)
				m_fHiKeyTime = pkKeys->GetKeyAt(uiNumKeys - 1, ucSize)->GetTime();
		}

		pkKeys = pkData->m_pkVOffsetKeys;
		uiNumKeys = pkData->m_uiNumVOffsetKeys;
		ucSize = pkData->m_ucVOffsetSize;
		if (pkKeys) {
			if (pkKeys->GetKeyAt(0, ucSize)->GetTime() < m_fLoKeyTime)
				m_fLoKeyTime = pkKeys->GetKeyAt(0, ucSize)->GetTime();
			if (pkKeys->GetKeyAt(uiNumKeys - 1, ucSize)->GetTime() > m_fHiKeyTime)
				m_fHiKeyTime = pkKeys->GetKeyAt(uiNumKeys - 1, ucSize)->GetTime();
		}

		pkKeys = pkData->m_pkUTilingKeys;
		uiNumKeys = pkData->m_uiNumUTilingKeys;
		ucSize = pkData->m_ucUTilingSize;
		if (pkKeys) {
			if (pkKeys->GetKeyAt(0, ucSize)->GetTime() < m_fLoKeyTime)
				m_fLoKeyTime = pkKeys->GetKeyAt(0, ucSize)->GetTime();
			if (pkKeys->GetKeyAt(uiNumKeys - 1, ucSize)->GetTime() > m_fHiKeyTime)
				m_fHiKeyTime = pkKeys->GetKeyAt(uiNumKeys - 1, ucSize)->GetTime();
		}

		pkKeys = pkData->m_pkVTilingKeys;
		uiNumKeys = pkData->m_uiNumVTilingKeys;
		ucSize = pkData->m_ucVTilingSize;
		if (pkKeys) {
			if (pkKeys->GetKeyAt(0, ucSize)->GetTime() < m_fLoKeyTime)
				m_fLoKeyTime = pkKeys->GetKeyAt(0, ucSize)->GetTime();
			if (pkKeys->GetKeyAt(uiNumKeys - 1, ucSize)->GetTime() > m_fHiKeyTime)
				m_fHiKeyTime = pkKeys->GetKeyAt(uiNumKeys - 1, ucSize)->GetTime();
		}
	}
}

void NiUVController::SetUVData(NiUVData* a1) {
	m_spUVData = a1;
	m_uiLastUOffsetIdx = 0;
	m_uiLastVOffsetIdx = 0;
	m_uiLastUTilingIdx = 0;
	m_uiLastVTilingIdx = 0;
	ResetKeyTimeExtrema();
}

NiObject* NiUVController::CreateObject() {
	NiUVController* v1 = static_cast<NiUVController*>(NiMemObject::operator new(sizeof(NiUVController)));
	if (!v1) {
		return nullptr;
	}
	return ::new (v1) NiUVController;
}
