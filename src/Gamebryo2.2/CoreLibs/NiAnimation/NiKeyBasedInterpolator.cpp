#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

NiRTTI NiKeyBasedInterpolator::ms_RTTI("NiKeyBasedInterpolator", &NiInterpolator::ms_RTTI);

NiKeyBasedInterpolator::NiKeyBasedInterpolator() {}

NiKeyBasedInterpolator::~NiKeyBasedInterpolator() {}

const NiRTTI* NiKeyBasedInterpolator::GetRTTI() const {
	return &ms_RTTI;
}

// NiInterpolator adds nothing to the stream, so this ends in NiObject's body. The other key based interpolators call
// this function directly.
void NiKeyBasedInterpolator::LoadBinary(NiStream& kStream) {
	NiObject::LoadBinary(kStream);
}

void NiKeyBasedInterpolator::LinkObject(NiStream& kStream) {
	NiInterpolator::LinkObject(kStream);
}

bool NiKeyBasedInterpolator::RegisterStreamables(NiStream& kStream) {
	return NiObject::RegisterStreamables(kStream);
}

void NiKeyBasedInterpolator::SaveBinary(NiStream& kStream) {
	NiObject::SaveBinary(kStream);
}

bool NiKeyBasedInterpolator::IsEqual(NiObject* pkOther) {
	return NiObject::IsEqual(pkOther);
}

void NiKeyBasedInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiInterpolator::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The span from the earliest first key to the latest last key over all channels; 0 to 0 when no channel has keys.
void NiKeyBasedInterpolator::GetActiveTimeRange(float& fBeginKeyTime, float& fEndKeyTime) const {
	fBeginKeyTime = FLT_MAX;
	fEndKeyTime = -FLT_MAX;
	for (uint16_t usChannel = 0; usChannel < GetKeyChannelCount(); usChannel++) {
		// The key array is raw memory stepped through by the channel's key stride.
		// EXE-INLINED 0xA3CA80
		uint8_t ucBeginKeySize = GetKeyStride(usChannel);
		uintptr_t uiBeginKeys = reinterpret_cast<uintptr_t>(GetKeyArray(usChannel));
		NiAnimationKey* pkBeginKey = reinterpret_cast<NiAnimationKey*>(uiBeginKeys + 0u * ucBeginKeySize);
		uint32_t uiEndIndex = GetKeyCount(usChannel) - 1;
		// EXE-INLINED 0xA3CA80
		uint8_t ucEndKeySize = GetKeyStride(usChannel);
		uintptr_t uiEndKeys = reinterpret_cast<uintptr_t>(GetKeyArray(usChannel));
		NiAnimationKey* pkEndKey = reinterpret_cast<NiAnimationKey*>(uiEndKeys + uiEndIndex * ucEndKeySize);
		if (pkBeginKey && pkEndKey) {
			if (pkBeginKey->GetTime() < fBeginKeyTime) {
				fBeginKeyTime = pkBeginKey->GetTime();
			}
			if (pkEndKey->GetTime() > fEndKeyTime) {
				fEndKeyTime = pkEndKey->GetTime();
			}
		}
	}

	if (fBeginKeyTime == FLT_MAX && fEndKeyTime == -FLT_MAX) {
		fBeginKeyTime = 0.0f;
		fEndKeyTime = 0.0f;
	}
}

// Null-safe: a channel without keys yields a null array, which GetActiveTimeRange tests for.
NiAnimationKey* NiKeyBasedInterpolator::GetKeyAt(uint32_t uiWhichKey, uint16_t usChannel) const {
	uint8_t ucKeySize = GetKeyStride(usChannel);
	uintptr_t uiKeys = reinterpret_cast<uintptr_t>(GetKeyArray(usChannel));
	return reinterpret_cast<NiAnimationKey*>(uiKeys + uiWhichKey * ucKeySize);
}

void NiKeyBasedInterpolator::CopyMembers(NiKeyBasedInterpolator* pkDest, NiCloningProcess& kCloning) {
	NiInterpolator::CopyMembers(pkDest, kCloning);
}
