#include "BSMain/BSRangeNode.h"

#include "BSMain/BSNiNode.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <stdint.h>

NiRTTI BSRangeNode::ms_RTTI("BSRangeNode", &BSNiNode::ms_RTTI);

// Enabled, at level 0, over the whole range [0, 255].
BSRangeNode::BSRangeNode() : bEnabled(true), cCurrent(0), cMin(0), cMax(UINT8_MAX) {}

BSRangeNode::~BSRangeNode() {}

const NiRTTI* BSRangeNode::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* BSRangeNode::CreateClone(NiCloningProcess& arCloneData) {
	void* pMemory = NiMemObject::operator new(sizeof(BSRangeNode));
	BSRangeNode* pkClone = pMemory ? ::new (pMemory) BSRangeNode : nullptr;
	CopyMembers(pkClone, arCloneData);
	return pkClone;
}

// The current level goes through SetCurrent so the children's visibility follows it. Each level comes through the
// exe's out-of-line 1-byte transfer, NiStreamLoadBinary<bool>, into one stack byte that is copied on as it is.
void BSRangeNode::LoadBinary(NiStream& arStream) {
	NiNode::LoadBinary(arStream);
	bool bLevel;
	NiStreamLoadBinary(arStream, bLevel);
	cMin = bLevel;
	NiStreamLoadBinary(arStream, bLevel);
	cMax = bLevel;
	NiStreamLoadBinary(arStream, bLevel);
	SetCurrent(bLevel);
}

void BSRangeNode::LinkObject(NiStream& arStream) {
	NiNode::LinkObject(arStream);
}

bool BSRangeNode::RegisterStreamables(NiStream& arStream) {
	return NiNode::RegisterStreamables(arStream);
}

// Each level goes through the exe's out-of-line 1-byte save (0xA29550).
void BSRangeNode::SaveBinary(NiStream& arStream) {
	NiNode::SaveBinary(arStream);
	uint8_t cLevel = cMin;
	NiStreamSaveByte(&arStream, &cLevel);
	cLevel = cMax;
	NiStreamSaveByte(&arStream, &cLevel);
	cLevel = cCurrent;
	NiStreamSaveByte(&arStream, &cLevel);
}

bool BSRangeNode::IsEqual(NiObject* apObject) {
	if (!NiNode::IsEqual(apObject)) {
		return false;
	}
	BSRangeNode* pkOther = static_cast<BSRangeNode*>(apObject);
	return cMin == pkOther->cMin && cMax == pkOther->cMax && cCurrent == pkOther->cCurrent;
}

// No title line: the subclasses add their own after these.
void BSRangeNode::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	NiNode::GetViewerStrings(apStrings);
	apStrings->Add(NiGetViewerString("Min", cMin));
	apStrings->Add(NiGetViewerString("Max", cMax));
	apStrings->Add(NiGetViewerString("Current", cCurrent));
	apStrings->Add(NiGetViewerString("Enabled", bEnabled));
}

// The range is the "min-max" (or single "n") at the end of the name; a missing max means 255. The bounds are
// swapped when given in reverse.
void BSRangeNode::ParseNameForRanges() {
	const char* pcName = m_kName;
	size_t uiLength = std::strlen(pcName);
	size_t i = 1;
	for (; (pcName[uiLength - i] >= '0' && pcName[uiLength - i] <= '9') || pcName[uiLength - i] == '-'; ++i) {
		if (i > uiLength) {
			break;
		}
	}
	const char* pcRange = &pcName[uiLength - i + 1];
	int32_t iMin = 0;
	int32_t iMax = 255;
	const char* pcDash = std::strstr(pcRange, "-");
	if (pcDash) {
		if (pcDash != pcRange) {
			iMin = std::atoi(pcRange);
		}
		if (pcDash[1]) {
			iMax = std::atoi(pcDash + 1);
		}
	} else if (*pcRange) {
		iMin = std::atoi(pcRange);
		iMax = iMin;
	}
	cMin = uint8_t(iMin >= iMax ? iMax : iMin);
	cMax = uint8_t(iMin <= iMax ? iMax : iMin);
}

bool BSRangeNode::SetCurrent(uint8_t acVal) {
	bool bEnabled = acVal >= static_cast<int32_t>(cMin) && acVal <= static_cast<int32_t>(cMax);
	if ((m_uFlags & 1) == 0 != bEnabled) {
		if (bEnabled) {
			m_uFlags &= ~1u;
		} else {
			m_uFlags |= 1u;
		}
		if (BSRangeNode::pVisibilityChangedCB) {
			BSRangeNode::pVisibilityChangedCB(this, bEnabled);
		}
	}
	return bEnabled;
}

void BSRangeNode::CopyMembers(BSRangeNode* pDest, NiCloningProcess& arCloneData) {
	BSNiNode::CopyMembers(pDest, arCloneData);
	pDest->cMin = cMin;
	pDest->cMax = cMax;
	pDest->SetCurrent(cCurrent);
}

// The exe's cleanup-only C++ EH frame is left out.
NiObject* BSRangeNode::CreateObject() {
	void* apMemory = NiMemObject::operator new(sizeof(BSRangeNode));
	if (apMemory) {
		return ::new (apMemory) BSRangeNode;
	} else {
		return 0;
	}
}
