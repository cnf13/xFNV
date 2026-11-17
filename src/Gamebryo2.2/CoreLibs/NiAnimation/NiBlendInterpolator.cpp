#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <bit>
#include <climits>

NiRTTI NiBlendInterpolator::ms_RTTI("NiBlendInterpolator", &NiInterpolator::ms_RTTI);

namespace {

// Files before 10.1.0.110 store priorities as 4-byte ints; their lowest value becomes the lowest char.
char LoadOldPriority(NiStream& arStream) {
	int32_t iPriority;
	NiStreamLoadBinary(arStream, iPriority);
	if (iPriority == INT32_MIN)
		return CHAR_MIN;
	return static_cast<char>(iPriority);
}

} // namespace

void NiBlendInterpolator::InterpArrayItem::LoadBinary(NiStream& kStream) {
	m_spInterpolator = static_cast<NiInterpolator*>(kStream.ResolveLinkID());
	NiStreamLoadBinary(kStream, m_fWeight);
	NiStreamLoadBinary(kStream, m_fNormalizedWeight);
	if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 110))
		NiStreamLoadBinary(kStream, m_cPriority);
	else
		m_cPriority = LoadOldPriority(kStream);
	NiStreamLoadBinary(kStream, m_fEaseSpinner);
}

// KEEP: a short read of the pre-10.1.0.110 priority (malformed data) leaves the exe's slot as `push ecx` set it, to
// this item's address, a value src cannot express; porting.md's unwritten-slot exception covers GetChunkData only.
void NiBlendInterpolator::InterpArrayItem::SaveBinary(NiStream& kStream) {
	kStream.SaveLinkID(m_spInterpolator.m_pObject);
	NiStreamSaveBinary(kStream, m_fWeight);
	NiStreamSaveBinary(kStream, m_fNormalizedWeight);
	NiStreamSaveBinary(kStream, m_cPriority);
	NiStreamSaveBinary(kStream, m_fEaseSpinner);
}

// The update time is not compared.
bool NiBlendInterpolator::InterpArrayItem::IsEqual(InterpArrayItem& kDest) {
	if (m_spInterpolator.m_pObject) {
		if (!kDest.m_spInterpolator.m_pObject || !m_spInterpolator->IsEqual(kDest.m_spInterpolator.m_pObject))
			return false;
	} else if (kDest.m_spInterpolator.m_pObject) {
		return false;
	}
	return m_fWeight == kDest.m_fWeight && m_fNormalizedWeight == kDest.m_fNormalizedWeight &&
		m_cPriority == kDest.m_cPriority && m_fEaseSpinner == kDest.m_fEaseSpinner;
}

NiBlendInterpolator::InterpArrayItem& NiBlendInterpolator::InterpArrayItem::operator=(const InterpArrayItem& arItem) {
	m_spInterpolator = arItem.m_spInterpolator;
	m_fWeight = arItem.m_fWeight;
	m_fNormalizedWeight = arItem.m_fNormalizedWeight;
	m_cPriority = arItem.m_cPriority;
	m_fEaseSpinner = arItem.m_fEaseSpinner;
	m_fUpdateTime = arItem.m_fUpdateTime;
	return *this;
}

NiBlendInterpolator::NiBlendInterpolator()
	: NiInterpolator(), m_uFlags(0), m_ucArraySize(0), m_ucInterpCount(0), m_ucSingleIdx(INVALID_INDEX),
	  m_cHighPriority(CHAR_MIN), m_cNextHighPriority(CHAR_MIN), m_pkInterpArray(nullptr),
	  m_pkSingleInterpolator(nullptr), m_fWeightThreshold(0.0f), m_fSingleTime(INVALID_TIME),
	  m_fHighSumOfWeights(INVALID_TIME), m_fNextHighSumOfWeights(INVALID_TIME), m_fHighEaseSpinner(INVALID_TIME) {}

// Only the low bytes of abManagerControlled and aucArraySize count.
NiBlendInterpolator::NiBlendInterpolator(int32_t abManagerControlled, float afWeightThreshold, int32_t aucArraySize)
	: NiInterpolator(), m_uFlags(static_cast<uint8_t>(abManagerControlled) != 0),
	  m_ucArraySize(static_cast<uint8_t>(aucArraySize)), m_ucInterpCount(0), m_ucSingleIdx(INVALID_INDEX),
	  m_cHighPriority(CHAR_MIN), m_cNextHighPriority(CHAR_MIN), m_pkInterpArray(nullptr),
	  m_pkSingleInterpolator(nullptr), m_fWeightThreshold(afWeightThreshold), m_fSingleTime(INVALID_TIME),
	  m_fHighSumOfWeights(INVALID_TIME), m_fNextHighSumOfWeights(INVALID_TIME), m_fHighEaseSpinner(INVALID_TIME) {
	if (m_ucArraySize) {
		m_pkInterpArray = NiTNewInterface<InterpArrayItem>::Allocate(m_ucArraySize);
	}
}

// The items' destructors release their interpolators.
NiBlendInterpolator::~NiBlendInterpolator() {
	NiTNewInterface<InterpArrayItem>::Deallocate(m_pkInterpArray);
}

// A manager-controlled interpolator streams no items or blend state: its manager adds the items again.
void NiBlendInterpolator::LoadBinary(NiStream& kStream) {
	NiObject::LoadBinary(kStream);

	if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 112)) {
		NiStreamLoadBinary(kStream, m_uFlags);
		SetComputeNormalizedWeights(false);
	}

	if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 110)) {
		NiStreamLoadBinary(kStream, m_ucArraySize);
	} else {
		uint16_t usArraySize;
		NiStreamLoadBinary(kStream, usArraySize);
		m_ucArraySize = static_cast<uint8_t>(usArraySize);
		uint16_t usArrayGrowBy;
		NiStreamLoadBinary(kStream, usArrayGrowBy);
	}
	m_pkInterpArray = NiTNewInterface<InterpArrayItem>::Allocate(m_ucArraySize);

	if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 112)) {
		NiStreamLoadBinary(kStream, m_fWeightThreshold);
		if (!GetManagerControlled()) {
			NiStreamLoadBinary(kStream, m_ucInterpCount);
			NiStreamLoadBinary(kStream, m_ucSingleIdx);
			NiStreamLoadBinary(kStream, m_cHighPriority);
			NiStreamLoadBinary(kStream, m_cNextHighPriority);
			NiStreamLoadBinary(kStream, m_fSingleTime);
			NiStreamLoadBinary(kStream, m_fHighSumOfWeights);
			NiStreamLoadBinary(kStream, m_fNextHighSumOfWeights);
			NiStreamLoadBinary(kStream, m_fHighEaseSpinner);
			for (uint8_t uc = 0; uc < m_ucArraySize; uc++)
				m_pkInterpArray[uc].LoadBinary(kStream);
			m_pkSingleInterpolator = static_cast<NiInterpolator*>(kStream.ResolveLinkID());
		}
		return;
	}

	for (uint8_t uc = 0; uc < m_ucArraySize; uc++)
		m_pkInterpArray[uc].LoadBinary(kStream);

	uint8_t ucManagerControlled;
	NiStreamLoadBinary(kStream, ucManagerControlled);
	if (ucManagerControlled)
		m_uFlags |= MANAGER_CONTROLLED_MASK;
	else
		m_uFlags &= ~MANAGER_CONTROLLED_MASK;
	NiStreamLoadBinary(kStream, m_fWeightThreshold);
	uint8_t ucOnlyUseHighestWeight;
	NiStreamLoadBinary(kStream, ucOnlyUseHighestWeight);
	if (ucOnlyUseHighestWeight)
		m_uFlags |= ONLY_USE_HIGHEST_WEIGHT_MASK;
	else
		m_uFlags &= ~ONLY_USE_HIGHEST_WEIGHT_MASK;

	if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 110)) {
		NiStreamLoadBinary(kStream, m_ucInterpCount);
		NiStreamLoadBinary(kStream, m_ucSingleIdx);
	} else {
		uint16_t usInterpCount;
		NiStreamLoadBinary(kStream, usInterpCount);
		m_ucInterpCount = static_cast<uint8_t>(usInterpCount);
		uint16_t usSingleIdx;
		NiStreamLoadBinary(kStream, usSingleIdx);
		m_ucSingleIdx = static_cast<uint8_t>(usSingleIdx);
	}

	if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 108)) {
		m_pkSingleInterpolator = static_cast<NiInterpolator*>(kStream.ResolveLinkID());
		NiStreamLoadBinary(kStream, m_fSingleTime);
	}

	if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 110)) {
		NiStreamLoadBinary(kStream, m_cHighPriority);
		NiStreamLoadBinary(kStream, m_cNextHighPriority);
	} else {
		m_cHighPriority = LoadOldPriority(kStream);
		m_cNextHighPriority = LoadOldPriority(kStream);
	}
}

// KEEP (niread:): before 10.1.0.110 the exe reads the 2-byte array size into the stream argument's own slot, so a
// short read (malformed data) leaves the low byte of the stream's address as m_ucArraySize, and it reads the
// manager-controlled flag into a slot nothing wrote; src cannot express either. It also calls the KEEP
// InterpArrayItem::LoadBinary.
void NiBlendInterpolator::LinkObject(NiStream& kStream) {
	NiObject::LinkObject(kStream);
}

bool NiBlendInterpolator::RegisterStreamables(NiStream& kStream) {
	if (!NiObject::RegisterStreamables(kStream))
		return false;

	if (!GetManagerControlled()) {
		for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
			NiInterpolator* pkInterp = m_pkInterpArray[uc].m_spInterpolator.m_pObject;
			if (pkInterp)
				pkInterp->RegisterStreamables(kStream);
		}
	}
	return true;
}

void NiBlendInterpolator::SaveBinary(NiStream& kStream) {
	NiInterpolator::SaveBinary(kStream);

	NiStreamSaveBinary(kStream, m_uFlags);
	NiStreamSaveBinary(kStream, m_ucArraySize);
	NiStreamSaveBinary(kStream, m_fWeightThreshold);
	if (!GetManagerControlled()) {
		NiStreamSaveBinary(kStream, m_ucInterpCount);
		NiStreamSaveBinary(kStream, m_ucSingleIdx);
		NiStreamSaveBinary(kStream, m_cHighPriority);
		NiStreamSaveBinary(kStream, m_cNextHighPriority);
		NiStreamSaveBinary(kStream, m_fSingleTime);
		NiStreamSaveBinary(kStream, m_fHighSumOfWeights);
		NiStreamSaveBinary(kStream, m_fNextHighSumOfWeights);
		NiStreamSaveBinary(kStream, m_fHighEaseSpinner);
		for (uint8_t uc = 0; uc < m_ucArraySize; uc++)
			m_pkInterpArray[uc].SaveBinary(kStream);
		kStream.SaveLinkID(m_pkSingleInterpolator);
	}
}

// The single interpolators are compared by identity once their contents match.
bool NiBlendInterpolator::IsEqual(NiObject* pkObject) {
	if (!NiObject::IsEqual(pkObject))
		return false;

	NiBlendInterpolator* pkOther = static_cast<NiBlendInterpolator*>(pkObject);
	if (m_ucArraySize != pkOther->m_ucArraySize)
		return false;
	for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
		if (!m_pkInterpArray[uc].IsEqual(pkOther->m_pkInterpArray[uc]))
			return false;
	}

	if (m_uFlags != pkOther->m_uFlags || m_fWeightThreshold != pkOther->m_fWeightThreshold ||
		m_ucInterpCount != pkOther->m_ucInterpCount || m_ucSingleIdx != pkOther->m_ucSingleIdx)
		return false;
	if (m_pkSingleInterpolator && !m_pkSingleInterpolator->IsEqual(pkOther->m_pkSingleInterpolator))
		return false;
	return m_pkSingleInterpolator == pkOther->m_pkSingleInterpolator && m_fSingleTime == pkOther->m_fSingleTime &&
		m_cHighPriority == pkOther->m_cHighPriority && m_cNextHighPriority == pkOther->m_cNextHighPriority;
}

// Each item is listed only when one of the viewer's last two controller managers has a sequence that owns it.
void NiBlendInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiInterpolator::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("m_ucArraySize", m_ucArraySize));
	pkStrings->Add(NiGetViewerString("ms_ucArrayGrowBy", ms_ucArrayGrowBy));
	pkStrings->Add(NiGetViewerString("ManagerControlled", GetManagerControlled()));
	pkStrings->Add(NiGetViewerString("m_fWeightThreshold", m_fWeightThreshold));
	pkStrings->Add(NiGetViewerString("m_cHighPriority", static_cast<int32_t>(m_cHighPriority)));
	pkStrings->Add(NiGetViewerString("m_cNextHighPriority", static_cast<int32_t>(m_cNextHighPriority)));
	pkStrings->Add(NiGetViewerString("m_fHighSumOfWeights", m_fHighSumOfWeights));
	pkStrings->Add(NiGetViewerString("m_fNextHighSumOfWeights", m_fNextHighSumOfWeights));
	pkStrings->Add(NiGetViewerString("m_bOnlyUseHighestWeight", GetOnlyUseHighestWeight()));
	pkStrings->Add(NiGetViewerString("m_ucInterpCount", m_ucInterpCount));
	pkStrings->Add(NiGetViewerString("m_ucSingleIdx", m_ucSingleIdx));

	for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
		InterpArrayItem& kItem = m_pkInterpArray[uc];
		if (!kItem.m_spInterpolator.m_pObject || !pLastViewerStringsControllerManager[0])
			continue;

		NiControllerSequence* pkSequence =
			pLastViewerStringsControllerManager[0]->FindControllerSequence(kItem.m_spInterpolator.m_pObject, true);
		if (!pkSequence) {
			if (!pLastViewerStringsControllerManager[1])
				continue;
			pkSequence =
				pLastViewerStringsControllerManager[1]->FindControllerSequence(kItem.m_spInterpolator.m_pObject, true);
			if (!pkSequence)
				continue;
		}

		pkStrings->Add(NiGetViewerString("NiControllerSequence", static_cast<const char*>(pkSequence->m_kName)));
		pkStrings->Add(NiGetViewerString("m_cPriority", static_cast<uint8_t>(kItem.m_cPriority)));
		pkStrings->Add(NiGetViewerString("m_fWeight", kItem.m_fWeight));
		pkStrings->Add(NiGetViewerString("m_fNormalizedWeight", kItem.m_fNormalizedWeight));
		pkStrings->Add(NiGetViewerString("m_fEaseSpinner", kItem.m_fEaseSpinner));
		kItem.m_spInterpolator->GetViewerStrings(pkStrings);
	}
}

// The clone's single interpolator becomes the clone of this one's, or stays shared when it was not cloned.
void NiBlendInterpolator::ProcessClone(NiCloningProcess& kCloning) {
	NiObject::ProcessClone(kCloning);

	NiObject* pkClone = nullptr;
	kCloning.m_pkCloneMap->GetAt(this, pkClone);

	for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
		NiInterpolator* pkInterp = m_pkInterpArray[uc].m_spInterpolator.m_pObject;
		if (pkInterp)
			pkInterp->ProcessClone(kCloning);
	}

	if (m_pkSingleInterpolator) {
		NiBlendInterpolator* pkDest = static_cast<NiBlendInterpolator*>(pkClone);
		NiObject* pkSingleClone;
		if (kCloning.m_pkCloneMap->GetAt(m_pkSingleInterpolator, pkSingleClone))
			pkDest->m_pkSingleInterpolator = static_cast<NiInterpolator*>(pkSingleClone);
		else
			pkDest->m_pkSingleInterpolator = m_pkSingleInterpolator;
	}
}

void NiBlendInterpolator::Collapse() {
	if (m_ucInterpCount == 1) {
		m_pkSingleInterpolator->Collapse();
		return;
	}
	for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
		NiInterpolator* pkInterp = m_pkInterpArray[uc].m_spInterpolator.m_pObject;
		if (pkInterp)
			pkInterp->Collapse();
	}
}

void NiBlendInterpolator::GuaranteeTimeRange(float fStartTime, float fEndTime) {
	if (m_ucInterpCount == 1) {
		m_pkSingleInterpolator->GuaranteeTimeRange(fStartTime, fEndTime);
		return;
	}
	for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
		NiInterpolator* pkInterp = m_pkInterpArray[uc].m_spInterpolator.m_pObject;
		if (pkInterp)
			pkInterp->GuaranteeTimeRange(fStartTime, fEndTime);
	}
}

// The priorities keep the highest and the next lower one among the items.
uint8_t NiBlendInterpolator::AddInterpInfo(NiInterpolator* pkInterpolator, float fWeight, char cPriority,
	float fEaseSpinner) {
	uint8_t ucIndex;
	for (ucIndex = 0; ucIndex < m_ucArraySize; ucIndex++) {
		if (!m_pkInterpArray[ucIndex].m_spInterpolator.m_pObject)
			break;
	}
	if (ucIndex == m_ucArraySize && !ReallocateArrays())
		return INVALID_INDEX;

	InterpArrayItem& kItem = m_pkInterpArray[ucIndex];
	kItem.m_spInterpolator = pkInterpolator;
	kItem.m_fWeight = fWeight;
	kItem.m_cPriority = cPriority;
	kItem.m_fNormalizedWeight = 0.0f;
	kItem.m_fEaseSpinner = fEaseSpinner;
	kItem.m_fUpdateTime = INVALID_TIME;

	if (cPriority > m_cNextHighPriority) {
		if (cPriority > m_cHighPriority) {
			m_cNextHighPriority = m_cHighPriority;
			m_cHighPriority = cPriority;
		} else if (cPriority != m_cHighPriority) {
			m_cNextHighPriority = cPriority;
		}
	}

	if (++m_ucInterpCount == 1) {
		m_ucSingleIdx = ucIndex;
		m_pkSingleInterpolator = pkInterpolator;
	} else {
		m_ucSingleIdx = INVALID_INDEX;
		m_fSingleTime = INVALID_TIME;
		m_pkSingleInterpolator = nullptr;
	}

	SetComputeNormalizedWeights(true);
	ClearWeightSums();
	return ucIndex;
}

// Removing an item of the highest or next priority finds both again among the remaining items.
void NiBlendInterpolator::RemoveInterpInfo(uint8_t ucIndex) {
	InterpArrayItem& kItem = m_pkInterpArray[ucIndex];
	char cPriority = kItem.m_cPriority;
	kItem.m_spInterpolator = nullptr;

	if (cPriority == m_cHighPriority || cPriority == m_cNextHighPriority) {
		m_cNextHighPriority = CHAR_MIN;
		m_cHighPriority = CHAR_MIN;
		for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
			InterpArrayItem& kOther = m_pkInterpArray[uc];
			if (!kOther.m_spInterpolator.m_pObject)
				continue;
			if (kOther.m_cPriority > m_cNextHighPriority) {
				if (kOther.m_cPriority > m_cHighPriority) {
					m_cNextHighPriority = m_cHighPriority;
					m_cHighPriority = kOther.m_cPriority;
				} else if (kOther.m_cPriority < m_cHighPriority) {
					m_cNextHighPriority = kOther.m_cPriority;
				}
			}
		}
	}

	if (--m_ucInterpCount == 1) {
		if (m_ucArraySize == 2) {
			m_ucSingleIdx = ucIndex == 0 ? 1 : 0;
		} else {
			for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
				if (m_pkInterpArray[uc].m_spInterpolator.m_pObject) {
					m_ucSingleIdx = uc;
					break;
				}
			}
		}
		InterpArrayItem& kSingle = m_pkInterpArray[m_ucSingleIdx];
		m_pkSingleInterpolator = kSingle.m_spInterpolator.m_pObject;
		m_fSingleTime = kSingle.m_fUpdateTime;
	} else if (m_ucInterpCount == 0) {
		m_cNextHighPriority = CHAR_MIN;
		m_cHighPriority = CHAR_MIN;
		m_ucSingleIdx = INVALID_INDEX;
		m_pkSingleInterpolator = nullptr;
		m_fSingleTime = INVALID_TIME;
	}

	SetComputeNormalizedWeights(true);
	ClearWeightSums();
}

// Grows the item array by ms_ucArrayGrowBy, to at least 4 items.
bool NiBlendInterpolator::ReallocateArrays() {
	if (!ms_ucArrayGrowBy)
		return false;

	uint8_t ucNewArraySize = static_cast<uint8_t>(m_ucArraySize + ms_ucArrayGrowBy);
	if (ucNewArraySize < 4)
		ucNewArraySize = 4;

	InterpArrayItem* pkNewInterpArray = NiTNewInterface<InterpArrayItem>::Allocate(ucNewArraySize);
	for (uint8_t uc = 0; uc < m_ucArraySize; uc++)
		pkNewInterpArray[uc] = m_pkInterpArray[uc];
	NiTNewInterface<InterpArrayItem>::Deallocate(m_pkInterpArray);

	m_pkInterpArray = pkNewInterpArray;
	m_ucArraySize = ucNewArraySize;
	return true;
}

// The single interpolator stands for its item while it is the only one.
NiInterpolator* NiBlendInterpolator::GetInterpolator(uint8_t ucIndex) const {
	if (m_ucInterpCount == 1 && ucIndex == m_ucSingleIdx)
		return m_pkSingleInterpolator;
	return m_pkInterpArray[ucIndex].m_spInterpolator.m_pObject;
}

// Weights below the threshold drop out and the rest are scaled back to a sum of 1. Only the highest weighted item keeps
// a weight (1) when ONLY_USE_HIGHEST_WEIGHT is set; with no items that writes item 255, as the game does.
void NiBlendInterpolator::ComputeNormalizedWeights() {
	if (!GetComputeNormalizedWeights())
		return;
	SetComputeNormalizedWeights(false);

	if (m_ucInterpCount == 1) {
		m_pkInterpArray[m_ucSingleIdx].m_fNormalizedWeight = 1.0f;
		return;
	}
	if (m_ucInterpCount == 2) {
		ComputeNormalizedWeightsFor2();
		return;
	}

	if (m_fHighSumOfWeights == -FLT_MAX) {
		m_fHighSumOfWeights = 0.0f;
		m_fNextHighSumOfWeights = 0.0f;
		m_fHighEaseSpinner = 0.0f;
		for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
			InterpArrayItem& kItem = m_pkInterpArray[uc];
			if (!kItem.m_spInterpolator.m_pObject)
				continue;
			float fRealWeight = kItem.m_fWeight * kItem.m_fEaseSpinner;
			if (kItem.m_cPriority == m_cHighPriority) {
				m_fHighSumOfWeights += fRealWeight;
				if (kItem.m_fEaseSpinner > m_fHighEaseSpinner)
					m_fHighEaseSpinner = kItem.m_fEaseSpinner;
			} else if (kItem.m_cPriority == m_cNextHighPriority) {
				m_fNextHighSumOfWeights += fRealWeight;
			}
		}
	}

	// The two products and their sum are computed in double precision and only the total is rounded to float, as the
	// game does.
	float fOneMinusHighEaseSpinner = 1.0f - m_fHighEaseSpinner;
	float fTotalSumOfWeights =
		static_cast<float>(static_cast<double>(m_fNextHighSumOfWeights) * fOneMinusHighEaseSpinner +
			static_cast<double>(m_fHighSumOfWeights) * m_fHighEaseSpinner);
	// A NaN total gives 0.
	float fOneOverTotalSumOfWeights = fTotalSumOfWeights > 0.0f ? 1.0f / fTotalSumOfWeights : 0.0f;

	for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
		InterpArrayItem& kItem = m_pkInterpArray[uc];
		if (!kItem.m_spInterpolator.m_pObject)
			continue;
		// Each product chain stays unrounded on the x87 stack until the store.
		if (kItem.m_cPriority == m_cHighPriority) {
			kItem.m_fNormalizedWeight = static_cast<float>(static_cast<double>(kItem.m_fWeight) * m_fHighEaseSpinner *
				kItem.m_fEaseSpinner * fOneOverTotalSumOfWeights);
		} else if (kItem.m_cPriority == m_cNextHighPriority) {
			kItem.m_fNormalizedWeight = static_cast<float>(static_cast<double>(kItem.m_fWeight) *
				fOneMinusHighEaseSpinner * kItem.m_fEaseSpinner * fOneOverTotalSumOfWeights);
		} else {
			kItem.m_fNormalizedWeight = 0.0f;
		}
	}

	// A NaN threshold skips the reduction.
	if (m_fWeightThreshold > 0.0f) {
		float fTotalNormalizedWeight = 0.0f;
		for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
			InterpArrayItem& kItem = m_pkInterpArray[uc];
			if (kItem.m_spInterpolator.m_pObject && kItem.m_fNormalizedWeight != 0.0f) {
				if (kItem.m_fNormalizedWeight < m_fWeightThreshold)
					kItem.m_fNormalizedWeight = 0.0f;
				fTotalNormalizedWeight += kItem.m_fNormalizedWeight;
			}
		}

		if (fTotalNormalizedWeight != 1.0f) {
			float fOneOverTotal = fTotalNormalizedWeight > 0.0f ? 1.0f / fTotalNormalizedWeight : 0.0f;
			for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
				InterpArrayItem& kItem = m_pkInterpArray[uc];
				if (kItem.m_fNormalizedWeight != 0.0f)
					kItem.m_fNormalizedWeight = kItem.m_fNormalizedWeight * fOneOverTotal;
			}
		}
	}

	if (GetOnlyUseHighestWeight()) {
		float fHighest = -1.0f;
		uint8_t ucHighIndex = INVALID_INDEX;
		for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
			if (m_pkInterpArray[uc].m_fNormalizedWeight > fHighest) {
				ucHighIndex = uc;
				fHighest = m_pkInterpArray[uc].m_fNormalizedWeight;
			}
			m_pkInterpArray[uc].m_fNormalizedWeight = 0.0f;
		}
		m_pkInterpArray[ucHighIndex].m_fNormalizedWeight = 1.0f;
	}
}

uint8_t NiBlendInterpolator::GetHighestWeightedIndex() {
	if (m_ucInterpCount == 1)
		return m_ucSingleIdx;

	float fHighest = 0.0f;
	uint8_t ucHighIndex = INVALID_INDEX;
	for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
		if (m_pkInterpArray[uc].m_fNormalizedWeight > fHighest) {
			ucHighIndex = uc;
			fHighest = m_pkInterpArray[uc].m_fNormalizedWeight;
		}
	}
	return ucHighIndex;
}

float NiBlendInterpolator::GetTime(uint8_t ucIndex) const {
	if ((m_ucInterpCount == 1) && (ucIndex == m_ucSingleIdx)) {
		return m_fSingleTime;
	} else {
		return m_pkInterpArray[ucIndex].m_fUpdateTime;
	}
}

// The interpolator array is allocated with a count cookie. The items come from CreateClone of each interpolator unless
// a manager controls this one (it adds them again); ProcessClone sets the single interpolator.
void NiBlendInterpolator::CopyMembers(NiBlendInterpolator* pkDest, NiCloningProcess& kCloning) {
	NiObject::CopyMembers(pkDest, kCloning);

	pkDest->m_uFlags = m_uFlags;
	pkDest->m_ucArraySize = m_ucArraySize;
	uint32_t uiCount = m_ucArraySize;
	uint64_t ullSize = uint64_t{sizeof(InterpArrayItem)} * uiCount;
	size_t stSize = ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize);
	stSize = stSize + sizeof(size_t) < stSize ? SIZE_MAX : stSize + sizeof(size_t);
	void* pvMemory = MemoryManager_Singleton.Allocate(stSize);
	InterpArrayItem* pkInterpArray = nullptr;
	if (pvMemory) {
		*static_cast<size_t*>(pvMemory) = uiCount;
		pkInterpArray = reinterpret_cast<InterpArrayItem*>(static_cast<size_t*>(pvMemory) + 1);
		rt::eh_vector_constructor_iterator(pkInterpArray, sizeof(InterpArrayItem), uiCount,
			&fnvd::constructElement<NiBlendInterpolator::InterpArrayItem>);
	}
	pkDest->m_pkInterpArray = pkInterpArray;
	pkDest->m_fWeightThreshold = m_fWeightThreshold;
	if (GetManagerControlled())
		return;

	for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
		InterpArrayItem& kItem = m_pkInterpArray[uc];
		InterpArrayItem& kDestItem = pkDest->m_pkInterpArray[uc];
		if (kItem.m_spInterpolator.m_pObject)
			kDestItem.m_spInterpolator = static_cast<NiInterpolator*>(kItem.m_spInterpolator->CreateClone(kCloning));
		kDestItem.m_fWeight = kItem.m_fWeight;
		kDestItem.m_fNormalizedWeight = kItem.m_fNormalizedWeight;
		kDestItem.m_cPriority = kItem.m_cPriority;
		kDestItem.m_fEaseSpinner = kItem.m_fEaseSpinner;
		kDestItem.m_fUpdateTime = kItem.m_fUpdateTime;
	}
	pkDest->m_ucInterpCount = m_ucInterpCount;
	pkDest->m_ucSingleIdx = m_ucSingleIdx;
	pkDest->m_fSingleTime = m_fSingleTime;
	pkDest->m_cHighPriority = m_cHighPriority;
	pkDest->m_cNextHighPriority = m_cNextHighPriority;
}

// With two items the ease spinner of the higher priority one splits the weight between them. A threshold only ever
// gives the second item the full weight: the second item's flag is set for both items, deliberately, as the game does.
void NiBlendInterpolator::ComputeNormalizedWeightsFor2() {
	InterpArrayItem* pkItem1 = nullptr;
	InterpArrayItem* pkItem2 = nullptr;
	for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
		if (m_pkInterpArray[uc].m_spInterpolator.m_pObject) {
			if (pkItem1) {
				pkItem2 = &m_pkInterpArray[uc];
				break;
			}
			pkItem1 = &m_pkInterpArray[uc];
		}
	}

	float fRealWeight1 = pkItem1->m_fWeight * pkItem1->m_fEaseSpinner;
	float fRealWeight2 = pkItem2->m_fWeight * pkItem2->m_fEaseSpinner;
	if (fRealWeight1 == 0.0f && fRealWeight2 == 0.0f) {
		pkItem1->m_fNormalizedWeight = 0.0f;
		pkItem2->m_fNormalizedWeight = 0.0f;
		return;
	}

	if (pkItem1->m_cPriority > pkItem2->m_cPriority) {
		if (pkItem1->m_fEaseSpinner == 1.0f) {
			pkItem1->m_fNormalizedWeight = 1.0f;
			pkItem2->m_fNormalizedWeight = 0.0f;
			return;
		}
		// The products and their sum stay unrounded on the x87 stack (53-bit); only the reciprocal and the stored
		// weights are rounded to float.
		float fOneMinusEaseSpinner = 1.0f - pkItem1->m_fEaseSpinner;
		double dWeight1 = static_cast<double>(fRealWeight1) * pkItem1->m_fEaseSpinner;
		double dWeight2 = static_cast<double>(fRealWeight2) * fOneMinusEaseSpinner;
		float fOneOverSumOfWeights = static_cast<float>(1.0 / (dWeight2 + dWeight1));
		pkItem1->m_fNormalizedWeight = static_cast<float>(dWeight1 * fOneOverSumOfWeights);
		pkItem2->m_fNormalizedWeight = static_cast<float>(dWeight2 * fOneOverSumOfWeights);
	} else if (pkItem1->m_cPriority < pkItem2->m_cPriority) {
		if (pkItem2->m_fEaseSpinner == 1.0f) {
			pkItem1->m_fNormalizedWeight = 0.0f;
			pkItem2->m_fNormalizedWeight = 1.0f;
			return;
		}
		float fOneMinusEaseSpinner = 1.0f - pkItem2->m_fEaseSpinner;
		double dWeight1 = static_cast<double>(fRealWeight1) * fOneMinusEaseSpinner;
		float fOneOverSumOfWeights =
			static_cast<float>(1.0 / (static_cast<double>(pkItem2->m_fEaseSpinner) * fRealWeight2 + dWeight1));
		pkItem1->m_fNormalizedWeight = static_cast<float>(dWeight1 * fOneOverSumOfWeights);
		pkItem2->m_fNormalizedWeight = static_cast<float>(static_cast<double>(fOneOverSumOfWeights) *
			(static_cast<double>(fRealWeight2) * pkItem2->m_fEaseSpinner));
	} else {
		float fOneOverSumOfWeights = static_cast<float>(1.0 / (static_cast<double>(fRealWeight2) + fRealWeight1));
		pkItem1->m_fNormalizedWeight = fRealWeight1 * fOneOverSumOfWeights;
		pkItem2->m_fNormalizedWeight = fRealWeight2 * fOneOverSumOfWeights;
	}

	if (GetOnlyUseHighestWeight()) {
		if (pkItem1->m_fNormalizedWeight >= pkItem2->m_fNormalizedWeight) {
			pkItem1->m_fNormalizedWeight = 1.0f;
			pkItem2->m_fNormalizedWeight = 0.0f;
		} else {
			pkItem1->m_fNormalizedWeight = 0.0f;
			pkItem2->m_fNormalizedWeight = 1.0f;
		}
		return;
	}

	if (m_fWeightThreshold > 0.0f) {
		bool bReduced = false;
		if (pkItem1->m_fNormalizedWeight < m_fWeightThreshold) {
			pkItem1->m_fNormalizedWeight = 0.0f;
			bReduced = true;
		}
		// A NaN second weight is left as it is.
		if (m_fWeightThreshold > pkItem2->m_fNormalizedWeight) {
			pkItem2->m_fNormalizedWeight = 0.0f;
			bReduced = true;
		}
		if (bReduced)
			pkItem2->m_fNormalizedWeight = 1.0f;
	}
}

int32_t NiBlendInterpolator::GetHighestPriority() {
	return (int32_t)m_cHighPriority;
}
