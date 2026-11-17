#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolTimelineInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBoolTimelineInterpolator::ms_RTTI("NiBoolTimelineInterpolator", &NiBoolInterpolator::ms_RTTI);

NiBoolTimelineInterpolator::NiBoolTimelineInterpolator(NiBoolData* apBoolData)
	: NiBoolInterpolator(apBoolData), m_uiPreviousIdx(0), m_bPreviousValue(0) {}

NiBoolTimelineInterpolator::~NiBoolTimelineInterpolator() {}

const NiRTTI* NiBoolTimelineInterpolator::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* NiBoolTimelineInterpolator::CreateClone(NiCloningProcess& kCloning) {
	NiBoolTimelineInterpolator* pkObject =
		static_cast<NiBoolTimelineInterpolator*>(NiMemObject::operator new(sizeof(NiBoolTimelineInterpolator)));
	if (pkObject) {
		pkObject = ::new (pkObject) NiBoolTimelineInterpolator(static_cast<NiBoolData*>(nullptr));
		CopyMembers(pkObject, kCloning);
		return pkObject;
	} else {
		CopyMembers(0, kCloning);
		return 0;
	}
}

void NiBoolTimelineInterpolator::LoadBinary(NiStream& kStream) {
	NiBoolInterpolator::LoadBinary(kStream);
}

void NiBoolTimelineInterpolator::LinkObject(NiStream& kStream) {
	NiBoolInterpolator::LinkObject(kStream);
}

bool NiBoolTimelineInterpolator::RegisterStreamables(NiStream& kStream) {
	return NiBoolInterpolator::RegisterStreamables(kStream);
}

void NiBoolTimelineInterpolator::SaveBinary(NiStream& kStream) {
	NiBoolInterpolator::SaveBinary(kStream);
}

bool NiBoolTimelineInterpolator::IsEqual(NiObject* pkObject) {
	return NiBoolInterpolator::IsEqual(pkObject);
}

void NiBoolTimelineInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiBoolInterpolator::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// When the sampled value equals the previous one, a key of the opposite value passed since the previous update still
// reports that opposite value once, so no toggle is lost between two updates. GenInterp's raw byte is kept: it is
// compared with m_bPreviousValue and stored there unchanged, and only m_bBoolValue gets 0 or 1.
// KEEP: the exe pushes the whole dword of its key-size slot ([esp+0x10], 0xA52DF4) to GenInterp, but writes only
// its low byte (0xA52DE4): the upper three bytes are an uninitialised read src cannot reproduce.
bool NiBoolTimelineInterpolator::Update(float fTime, NiObjectNET*, bool& bValue) {
	if (m_fLastTime == fTime) {
		if (m_bBoolValue == INVALID_BOOL) {
			bValue = false;
			return false;
		}
		bValue = m_bBoolValue != 0;
		return true;
	}

	if (m_spBoolData) {
		uint32_t uiNumKeys = m_spBoolData->m_uiNumKeys;
		NiBoolKey* pkKeys = m_spBoolData->m_pkKeys;
		NiBoolKey::KeyType eType = m_spBoolData->m_eType;
		uint8_t ucKeySize = m_spBoolData->m_ucKeySize;
		if (uiNumKeys > 0) {
			m_uiPreviousIdx = m_uiLastIdx;
			uint8_t ucValue = NiBoolKey::GenInterp(fTime, pkKeys, eType, uiNumKeys, m_uiLastIdx, ucKeySize);
			uint32_t uiLastIdx = m_uiLastIdx;
			uint8_t ucResult = ucValue;
			if (ucValue == m_bPreviousValue) {
				uint8_t ucTarget = ucValue == 0;
				uint32_t uiPreviousIdx = m_uiPreviousIdx;
				if (uiPreviousIdx != uiLastIdx && uiPreviousIdx + 1 > uiLastIdx) {
					// The keys wrapped around: search the end of the array, then its start.
					for (uint32_t ui = uiPreviousIdx + 1; ui < uiNumKeys; ui++) {
						if (pkKeys->GetKeyAt(ui, ucKeySize)->GetBool() == ucTarget) {
							ucResult = ucTarget;
							break;
						}
					}
					if (ucResult != ucTarget) {
						for (uint32_t ui = 0; ui < uiLastIdx; ui++) {
							if (pkKeys->GetKeyAt(ui, ucKeySize)->GetBool() == ucTarget) {
								ucResult = ucTarget;
								break;
							}
						}
					}
				} else {
					for (uint32_t ui = uiPreviousIdx + 1; ui < uiLastIdx && ui < uiNumKeys; ui++) {
						if (pkKeys->GetKeyAt(ui, ucKeySize)->GetBool() == ucTarget) {
							ucResult = ucTarget;
							break;
						}
					}
				}
			}
			m_bBoolValue = ucResult != 0;
			m_bPreviousValue = ucValue;
		}
	}

	if (m_bBoolValue == INVALID_BOOL) {
		bValue = false;
		return false;
	}
	bValue = m_bBoolValue != 0;
	m_fLastTime = fTime;
	return true;
}

NiObject* NiBoolTimelineInterpolator::CreateObject() {
	NiBoolTimelineInterpolator* pkObject =
		static_cast<NiBoolTimelineInterpolator*>(NiMemObject::operator new(sizeof(NiBoolTimelineInterpolator)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiBoolTimelineInterpolator(static_cast<NiBoolData*>(nullptr));
}
