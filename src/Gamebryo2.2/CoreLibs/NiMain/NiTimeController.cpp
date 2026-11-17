#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

#include "BSCore/TLSData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cfloat>
#include <cmath>
#include <cstring>

NiRTTI NiTimeController::ms_RTTI("NiTimeController", &NiObject::ms_RTTI);

// Active, computing scaled time, clamped cycle; the key range empty (lo +FLT_MAX, hi -FLT_MAX).
NiTimeController::NiTimeController()
	: m_usFlags(ACTIVE_MASK | COMPUTESCALEDTIME_MASK | (CLAMP << CYCLETYPE_POS)), m_fFrequency(1.0f), m_fPhase(0.0f),
	  m_fLoKeyTime(0x1.fffffe0000000p+127f), m_fHiKeyTime(-0x1.fffffe0000000p+127f),
	  m_fStartTime(-0x1.fffffe0000000p+127f), m_fLastTime(-0x1.fffffe0000000p+127f), m_fWeightedLastTime(0.0f),
	  m_fScaledTime(-0x1.fffffe0000000p+127f), m_pkTarget(nullptr), m_spNext(nullptr) {}

// The next controller is released by its pointer's destructor.
NiTimeController::~NiTimeController() {}

// Older files keep the flags in an older bit layout; the original word is left in the stream for subclasses that
// kept their own flags in its upper bits. Every loaded controller computes its scaled time and starts without a
// forced update.
void NiTimeController::LoadBinary(NiStream& kStream) {
	NiObject::LoadBinary(kStream);

	kStream.ReadLinkID();
	NiStreamLoadBinary(kStream, m_usFlags);
	NiStreamLoadBinary(kStream, m_fFrequency);
	NiStreamLoadBinary(kStream, m_fPhase);
	NiStreamLoadBinary(kStream, m_fLoKeyTime);
	NiStreamLoadBinary(kStream, m_fHiKeyTime);
	kStream.ReadLinkID();

	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 0, 1, 1)) {
		m_usFlags = uint16_t(((m_usFlags & 0xFFF0) << 1) | (m_usFlags & 0xF));
	}
	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 0, 1, 3)) {
		kStream.m_usNiTimeControllerFlags = m_usFlags;
		m_usFlags = uint16_t(m_usFlags & 0x1F);
	}
	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 109)) {
		m_usFlags = uint16_t(m_usFlags & ~MANAGERCONTROLLED_MASK);
	}
	m_usFlags = uint16_t((m_usFlags & ~FORCEUDPATE_MASK) | COMPUTESCALEDTIME_MASK);
}

// Older files did not mark the data of a vertex controller's geometry as changing.
void NiTimeController::LinkObject(NiStream& kStream) {
	m_spNext = static_cast<NiTimeController*>(kStream.GetObjectFromLinkID());
	m_pkTarget = static_cast<NiObjectNET*>(kStream.GetObjectFromLinkID());

	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 0, 1, 16) && IsVertexController() && m_pkTarget) {
		static_cast<NiGeometry*>(m_pkTarget)->m_spModelData->SetConsistency(NiGeometryData::MUTABLE);
	}
}

bool NiTimeController::RegisterStreamables(NiStream& kStream) {
	if (!NiObject::RegisterStreamables(kStream)) {
		return false;
	}

	if (m_spNext) {
		m_spNext->RegisterStreamables(kStream);
	}
	return true;
}

// The next link skips the controllers that are not saved.
void NiTimeController::SaveBinary(NiStream& kStream) {
	NiObject::SaveBinary(kStream);

	NiTimeController* pkNext = m_spNext;
	while (pkNext && !pkNext->IsStreamable()) {
		pkNext = pkNext->m_spNext;
	}
	kStream.SaveLinkID(pkNext);

	NiStreamSaveBinary(kStream, m_usFlags);
	NiStreamSaveBinary(kStream, m_fFrequency);
	NiStreamSaveBinary(kStream, m_fPhase);
	NiStreamSaveBinary(kStream, m_fLoKeyTime);
	NiStreamSaveBinary(kStream, m_fHiKeyTime);
	kStream.SaveLinkID(m_pkTarget);
}

// The targets only have to be both set or both unset.
bool NiTimeController::IsEqual(NiObject* pkObject) {
	if (!NiObject::IsEqual(pkObject)) {
		return false;
	}

	NiTimeController* pkOther = static_cast<NiTimeController*>(pkObject);
	if (m_usFlags != pkOther->m_usFlags || m_fFrequency != pkOther->m_fFrequency || m_fPhase != pkOther->m_fPhase ||
		m_fLoKeyTime != pkOther->m_fLoKeyTime || m_fHiKeyTime != pkOther->m_fHiKeyTime) {
		return false;
	}

	if ((m_pkTarget && !pkOther->m_pkTarget) || (!m_pkTarget && pkOther->m_pkTarget)) {
		return false;
	}
	return true;
}

// The lines start with this class' heading: NiObject's lines are not added.
void NiTimeController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	pkStrings->Add(NiGetViewerString(NiTimeController::ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("m_fFrequency", m_fFrequency));
	pkStrings->Add(NiGetViewerString("m_fPhase", m_fPhase));
	pkStrings->Add(NiGetViewerString("m_fLoKeyTime", m_fLoKeyTime));
	pkStrings->Add(NiGetViewerString("m_fHiKeyTime", m_fHiKeyTime));
	pkStrings->Add(NiGetViewerString("m_fStartTime", m_fStartTime));
	pkStrings->Add(NiGetViewerString("m_fLastTime", m_fLastTime));
	pkStrings->Add(NiGetViewerString("m_pkTarget", static_cast<const void*>(m_pkTarget)));
	pkStrings->Add(GetViewerString("anim type", static_cast<AnimType>(m_usFlags & ANIMTYPE_MASK)));
	pkStrings->Add(GetViewerString("cycle type",
		static_cast<CycleType>((m_usFlags & CYCLETYPE_MASK) >> CYCLETYPE_POS)));
	pkStrings->Add(NiGetViewerString("Active", (m_usFlags & ACTIVE_MASK) != 0));
	pkStrings->Add(NiGetViewerString("Play Backwards", (m_usFlags & DIRECTION_MASK) != 0));
}

void NiTimeController::ProcessClone(NiCloningProcess& kCloning) {
	NiObject::ProcessClone(kCloning);
	if (m_spNext) {
		m_spNext->ProcessClone(kCloning);
	}
}

// The last time is reset so that the next Update runs; an APP_INIT controller measures its time from afTime.
void NiTimeController::Start(float fTime) {
	m_usFlags = uint16_t(m_usFlags | ACTIVE_MASK);
	m_fLastTime = -FLT_MAX;
	if (m_usFlags & ANIMTYPE_MASK) {
		m_fStartTime = fTime;
	}
}

void NiTimeController::Stop() {
	m_usFlags = uint16_t(m_usFlags & ~ACTIVE_MASK);
	m_fLastTime = -FLT_MAX;
	if (m_usFlags & ANIMTYPE_MASK) {
		m_fStartTime = -FLT_MAX;
	}
}

// Moves the controller from its old target's list to the front of the new target's list (unless it is already
// there). The old list may hold the last reference, so the controller holds one of its own meanwhile.
void NiTimeController::SetTarget(NiObjectNET* pkTarget) {
	if (m_pkTarget == pkTarget) {
		return;
	}

	NiIncRefCount(this);

	if (m_pkTarget && m_pkTarget->m_spControllers) {
		m_pkTarget->RemoveController(this);
	}

	m_pkTarget = pkTarget;
	if (pkTarget) {
		NiTimeController* pkControl = pkTarget->m_spControllers;
		while (pkControl && pkControl != this) {
			pkControl = pkControl->m_spNext;
		}
		if (!pkControl) {
			pkTarget->PrependController(this);
			if (IsVertexController()) {
				static_cast<NiGeometry*>(m_pkTarget)->m_spModelData->SetConsistency(NiGeometryData::MUTABLE);
			}
		}
	}

	NiDecRefCount(this);
}

// The cache of the last inputs and result is a per-thread TLS block, so two threads never share it. NI_INFINITY is
// compared as the double FLT_MAX, and the results of the two fmods are rounded to float.
float NiTimeController::ComputeScaledTime(float afTime) {
	TLSData* pkTLS = GetThreadTLSData();

	float fTime = afTime;
	if (m_fStartTime == -3.4028234663852886e+38) {
		m_fStartTime = afTime;
	}

	if (m_fLastTime == -3.4028234663852886e+38) {
		m_fWeightedLastTime = 0.0f;
		if ((m_usFlags & ANIMTYPE_MASK) != 0) {
			fTime = 0.0f;
		}
	} else {
		fTime = afTime - m_fLastTime;
	}

	uint32_t uiCycleType = (m_usFlags & CYCLETYPE_MASK) >> CYCLETYPE_POS;

	float fWeightedTime = m_fFrequency * fTime + m_fWeightedLastTime;
	m_fWeightedLastTime = fWeightedTime;
	float fScaledTime = fWeightedTime + m_fPhase;
	m_fLastTime = afTime;

	if (m_fHiKeyTime == pkTLS->fLastHiKeyTime && m_fLoKeyTime == pkTLS->fLastLoKeyTime &&
		fScaledTime == pkTLS->fLastWeightedPhaseTime && uiCycleType == pkTLS->eLastCycle) {
		return pkTLS->fLastScaledTime;
	}

	pkTLS->eLastCycle = uiCycleType;
	pkTLS->fLastHiKeyTime = m_fHiKeyTime;
	pkTLS->fLastLoKeyTime = m_fLoKeyTime;
	pkTLS->fLastWeightedPhaseTime = fScaledTime;

	// An open key range (either end infinite) leaves the scaled time to the clamp below.
	if (m_fHiKeyTime == -3.4028234663852886e+38 || m_fLoKeyTime == 3.4028234663852886e+38) {
	} else {
		switch (uiCycleType) {
		case LOOP: {
			float fSpan = m_fHiKeyTime - m_fLoKeyTime;
			if (fSpan != 0.0f) {
				float fMod = static_cast<float>(std::fmod(fScaledTime - m_fLoKeyTime, fSpan));
				fScaledTime = fMod + m_fLoKeyTime;
				if (fScaledTime < m_fLoKeyTime) {
					fScaledTime += fSpan;
				}
			} else {
				fScaledTime = m_fLoKeyTime;
			}
			break;
		}
		case REVERSE: {
			float fSpan = m_fHiKeyTime - m_fLoKeyTime;
			if (fSpan != 0.0f) {
				float fDoubleSpan = fSpan + fSpan;
				float fTimeFract = static_cast<float>(std::fmod(fScaledTime, fDoubleSpan));
				if (fTimeFract < 0.0f) {
					fTimeFract += fDoubleSpan;
				}
				if (fTimeFract > fSpan) {
					fScaledTime = fDoubleSpan - fTimeFract + m_fLoKeyTime;
				} else {
					fScaledTime = fTimeFract + m_fLoKeyTime;
				}
			} else {
				fScaledTime = m_fLoKeyTime;
			}
			break;
		}
		default:
			break;
		}
	}

	if (fScaledTime > m_fHiKeyTime) {
		fScaledTime = m_fHiKeyTime;
	} else if (fScaledTime < m_fLoKeyTime) {
		fScaledTime = m_fLoKeyTime;
	}

	if ((m_usFlags & DIRECTION_MASK) != 0) {
		fScaledTime = m_fHiKeyTime - (fScaledTime - m_fLoKeyTime);
	}

	pkTLS->fLastScaledTime = fScaledTime;
	return fScaledTime;
}

bool NiTimeController::GetBit(uint16_t uMask) const {
	return (m_usFlags & uMask) != 0;
}

void NiTimeController::SetBit(bool bVal, uint16_t uMask) {
	if (bVal) {
		m_usFlags = uint16_t(m_usFlags | uMask);
	} else {
		m_usFlags = uint16_t(m_usFlags & ~uint32_t(uMask));
	}
}

bool NiTimeController::GetActive() const {
	return GetBit(ACTIVE_MASK);
}

void NiTimeController::SetComputeScaledTime(bool bComputeScaledTime) {
	SetBit(bComputeScaledTime, COMPUTESCALEDTIME_MASK);
}

void NiTimeController::SetPhase(float fPhase) {
	m_fPhase = fPhase;
}

void NiTimeController::CopyMembers(NiTimeController* pkDest, NiCloningProcess& kCloning) {
	NiObject::CopyMembers(pkDest, kCloning);

	pkDest->m_usFlags = m_usFlags;
	pkDest->m_fFrequency = m_fFrequency;
	pkDest->m_fPhase = m_fPhase;
	pkDest->m_fLoKeyTime = m_fLoKeyTime;
	pkDest->m_fHiKeyTime = m_fHiKeyTime;
	pkDest->m_fStartTime = m_fStartTime;
	pkDest->m_fLastTime = m_fLastTime;
	pkDest->m_fWeightedLastTime = m_fWeightedLastTime;

	if (m_pkTarget) {
		NiObject* pkClone;
		if (kCloning.m_pkCloneMap->GetAt(m_pkTarget, pkClone) && pkClone->GetRTTI() == m_pkTarget->GetRTTI()) {
			pkDest->m_pkTarget = static_cast<NiObjectNET*>(pkClone);
		} else {
			pkDest->m_pkTarget = nullptr;
		}
	}

	if (m_spNext) {
		pkDest->m_spNext = static_cast<NiTimeController*>(m_spNext->CreateClone(kCloning));
	}
}

// The scaled time is only recomputed (and compared) for controllers that ask for it.
bool NiTimeController::DontDoUpdate(float fTime) {
	if (!(m_usFlags & ACTIVE_MASK)) {
		return true;
	}
	if (m_fLastTime == fTime && !(m_usFlags & FORCEUDPATE_MASK)) {
		return true;
	}

	if (m_usFlags & COMPUTESCALEDTIME_MASK) {
		float fRounded = ComputeScaledTime(fTime);
		float fScaledTime = fRounded;
		if (m_fScaledTime == fScaledTime && !(m_usFlags & FORCEUDPATE_MASK)) {
			return true;
		}
		m_fScaledTime = fScaledTime;
	}

	m_usFlags = uint16_t(m_usFlags & ~FORCEUDPATE_MASK);
	return false;
}

void NiTimeController::SetActive(bool bActive) {
	SetBit(bActive, 8);
}

int32_t NiTimeController::GetLast() {
	NiTimeController* pThis = this;
	int32_t result;
	for (result = 0; pThis; ++result) {
		pThis = pThis->m_spNext.m_pObject;
	}
	return result;
}

// An unknown type leaves the string unwritten.
char* NiTimeController::GetViewerString(const char* apcPrefix, AnimType aeType) {
	size_t stLength = strlen(apcPrefix) + 12;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	switch (aeType) {
	case APP_TIME:
		NiSprintf(pcString, stLength, "%s = APP_TIME", apcPrefix);
		break;
	case APP_INIT:
		NiSprintf(pcString, stLength, "%s = APP_INIT", apcPrefix);
		break;
	}
	return pcString;
}

char* NiTimeController::GetViewerString(const char* apcPrefix, CycleType aeType) {
	size_t stLength = strlen(apcPrefix) + 11;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	switch (aeType) {
	case LOOP:
		NiSprintf(pcString, stLength, "%s = LOOP", apcPrefix);
		break;
	case REVERSE:
		NiSprintf(pcString, stLength, "%s = REVERSE", apcPrefix);
		break;
	case CLAMP:
		NiSprintf(pcString, stLength, "%s = CLAMP", apcPrefix);
		break;
	default:
		NiSprintf(pcString, stLength, "%s = UNKNOWN!!!", apcPrefix);
		break;
	}
	return pcString;
}

// Starts the controllers of pkObj, of its properties and of its children, recursively.
void NiTimeController::StartAnimations(NiObjectNET* pkObj, float fTime) {
	for (NiTimeController* pkControl = pkObj->m_spControllers; pkControl; pkControl = pkControl->m_spNext.m_pObject) {
		pkControl->Start(fTime);
	}

	if (pkObj->IsKindOf(&NiAVObject::ms_RTTI)) {
		NiAVObject* pkAVObj = static_cast<NiAVObject*>(pkObj);
		NiTListItem<NiPointer<NiProperty>>* pkPos = pkAVObj->m_kPropertyList.m_pkHead;
		while (pkPos) {
			NiProperty* pkProperty = pkPos->m_element.m_pObject;
			pkPos = pkPos->m_pkNext;
			if (pkProperty && pkProperty->m_spControllers) {
				NiTimeController::StartAnimations(pkProperty, fTime);
			}
		}
	}

	if (pkObj->IsNode()) {
		NiNode* pkNode = static_cast<NiNode*>(pkObj);
		for (uint32_t i = 0; i < pkNode->m_kChildren.m_usSize; ++i) {
			NiAVObject* pkChild = pkNode->m_kChildren.m_pBase[i].m_pObject;
			if (pkChild) {
				NiTimeController::StartAnimations(pkChild, fTime);
			}
		}
	}
}

// Starts the controllers of pkObj, of its properties and of its children from -FLT_MAX, recursively.
void NiTimeController::StartAnimations(NiObjectNET* pkObj) {
	for (NiTimeController* pkControl = pkObj->m_spControllers; pkControl; pkControl = pkControl->m_spNext.m_pObject) {
		pkControl->Start(-3.4028235e+38f);
	}

	if (pkObj->IsKindOf(&NiAVObject::ms_RTTI)) {
		NiAVObject* pkAVObj = static_cast<NiAVObject*>(pkObj);
		NiTListItem<NiPointer<NiProperty>>* pkPos = pkAVObj->m_kPropertyList.m_pkHead;
		while (pkPos) {
			NiProperty* pkProperty = pkPos->m_element.m_pObject;
			pkPos = pkPos->m_pkNext;
			if (pkProperty && pkProperty->m_spControllers) {
				NiTimeController::StartAnimations(pkProperty);
			}
		}
	}

	if (pkObj->IsNode()) {
		NiNode* pkNode = static_cast<NiNode*>(pkObj);
		for (uint32_t i = 0; i < pkNode->m_kChildren.m_usSize; ++i) {
			NiAVObject* pkChild = pkNode->m_kChildren.m_pBase[i].m_pObject;
			if (pkChild) {
				NiTimeController::StartAnimations(pkChild);
			}
		}
	}
}
