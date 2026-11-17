#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVertWeightsExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

NiRTTI NiObjectNET::ms_RTTI("NiObjectNET", &NiObject::ms_RTTI);
NiCriticalSection NiObjectNET::ExtraDataCrit;

NiObjectNET::NiObjectNET() : m_spControllers(nullptr), m_ppkExtra(nullptr), m_usExtraDataSize(0), m_usMaxSize(0) {}

// The extra data are removed and the controller released; the name's reference is released by its NiFixedString
// member.
NiObjectNET::~NiObjectNET() {
	RemoveAllExtraData();
	if (m_spControllers) {
		NiDecRefCount(m_spControllers);
	}
}

void NiObjectNET::LoadBinary(NiStream& kStream) {
	NiObject::LoadBinary(kStream);
	if (kStream.m_uiNifFileVersion < 0x14010001) {
		kStream.LoadCStringAsFixedString(m_kName);
	} else {
		kStream.LoadFixedString(m_kName);
	}
	if (kStream.m_uiNifFileVersion < 0x0500000B) {
		kStream.ReadLinkID();
	} else {
		kStream.ReadMultipleLinkIDs();
	}
	kStream.ReadLinkID();
}

// Files before 5.0.0.11 hold a single extra-data link, the head of a legacy chain that PostLinkObject unlinks; newer
// ones a counted list. The controller link is assigned as the exe inlines NiPointer's assignment.
void NiObjectNET::LinkObject(NiStream& kStream) {
	NiObject::LinkObject(kStream);
	if (kStream.m_uiNifFileVersion < 0x0500000B) {
		m_pkLegacyExtra = static_cast<NiExtraData*>(kStream.GetObjectFromLinkID());
	} else {
		uint32_t uiSize = kStream.GetNumberOfLinkIDs();
		SetExtraDataSize(uiSize);
		for (uint32_t i = uiSize; i != 0; --i) {
			AddExtraData(static_cast<NiExtraData*>(kStream.GetObjectFromLinkID()));
		}
	}
	NiTimeController* pkController = static_cast<NiTimeController*>(kStream.GetObjectFromLinkID());
	NiTimeController* pkOld = m_spControllers;
	// EXE-INLINED 0x66B0D0
	if (pkOld != pkController) {
		if (pkOld) {
			pkOld->DecRefCount();
		}
		m_spControllers = pkController;
		if (pkController) {
			pkController->IncRefCount();
		}
	}
}

// Registers the object and its name, then each streamable extra datum and the controller chain.
bool NiObjectNET::RegisterStreamables(NiStream& kStream) {
	if (!NiObject::RegisterStreamables(kStream)) {
		return false;
	}
	if (!kStream.RegisterFixedString(m_kName)) {
		return false;
	}
	for (uint16_t usIndex = 0; usIndex < m_usExtraDataSize; ++usIndex) {
		NiExtraData* pkExtra = m_ppkExtra[usIndex];
		if (pkExtra && pkExtra->IsStreamable()) {
			pkExtra->RegisterStreamables(kStream);
		}
	}
	if (m_spControllers) {
		m_spControllers->RegisterStreamables(kStream);
	}
	return true;
}

// The object part (the shared empty SaveBinary, called out of line), the name, the count and link IDs of the
// streamable extra data, then the link ID of the first streamable controller. The count write is NiStreamSaveBinary
// with NiBinaryStream::BinaryWrite inlined (m_pfnWrite is cdecl: the caller pops its five arguments); the controller
// walk reads m_spNext itself.
void NiObjectNET::SaveBinary(NiStream& kStream) {
	NiObject::SaveBinary(kStream);
	kStream.SaveFixedString(m_kName);
	uint32_t uiExtraDataSize = 0;
	for (uint16_t i = 0; i < m_usExtraDataSize; ++i) {
		NiExtraData* pkExtra = m_ppkExtra[i];
		if (pkExtra->IsStreamable()) {
			++uiExtraDataSize;
		}
	}
	uint32_t uiSize = 4;
	// EXE-INLINED none: NiStreamSaveBinary
	kStream.m_pkOstr->BinaryWrite(&uiExtraDataSize, 4, &uiSize, 1);
	for (uint16_t j = 0; j < m_usExtraDataSize; ++j) {
		NiExtraData* pkExtra = m_ppkExtra[j];
		if (pkExtra->IsStreamable()) {
			kStream.SaveLinkID(pkExtra);
		}
	}
	NiTimeController* pkController = m_spControllers;
	while (pkController && !pkController->IsStreamable()) {
		pkController = pkController->m_spNext.m_pObject;
	}
	kStream.SaveLinkID(pkController);
}

// Equal when the base comparison holds, the names are one handle, the streamable extra data compare equal pairwise and
// the controller chains are as long and compare equal pairwise. The exe inlines both index arrays' constructors and
// destructors (0xA5C760: only the frees of their buffers on each exit, the vtable stores being dead).
bool NiObjectNET::IsEqual(NiObject* pkObject) {
	if (!NiObject::IsEqual(pkObject)) {
		return false;
	}
	NiObjectNET* pkNET = static_cast<NiObjectNET*>(pkObject);
	if (m_kName.GetHandle() != pkNET->m_kName.GetHandle()) {
		return false;
	}

	// The indices of the streamable extra data of this object and of the other one (no slots, a grow-by of one).
	using IndexArrayBase = NiTArray<uint16_t, NiTMallocInterface<uint16_t>>;
	FNVD_INLINED_LOCAL(NiTPrimitiveArray<uint16_t>, kSelfEntries, std::nullopt, std::nullopt);
	::new (&kSelfEntries) NiTPrimitiveArray<uint16_t>(0u, 1u);
	FNVD_INLINED_LOCAL(NiTPrimitiveArray<uint16_t>, kOtherEntries, std::nullopt, std::nullopt);
	::new (&kOtherEntries) NiTPrimitiveArray<uint16_t>(0u, 1u);

	uint16_t i;
	for (i = 0; i < m_usExtraDataSize; ++i) {
		if (m_ppkExtra[i]->IsStreamable()) {
			kSelfEntries.Add(i);
		}
	}
	for (i = 0; i < pkNET->m_usExtraDataSize; ++i) {
		if (pkNET->m_ppkExtra[i]->IsStreamable()) {
			kOtherEntries.Add(i);
		}
	}

	if (kSelfEntries.m_usSize != kOtherEntries.m_usSize) {
		FNVD_DESTRUCT(IndexArrayBase)(&kOtherEntries);
		FNVD_DESTRUCT(IndexArrayBase)(&kSelfEntries);
		return false;
	}
	for (i = 0; i < kSelfEntries.m_usSize; ++i) {
		NiExtraData* pkSelfExtra = m_ppkExtra[kSelfEntries.m_pBase[i]];
		if (!pkSelfExtra->IsEqual(pkNET->m_ppkExtra[kOtherEntries.m_pBase[i]])) {
			FNVD_DESTRUCT(IndexArrayBase)(&kOtherEntries);
			FNVD_DESTRUCT(IndexArrayBase)(&kSelfEntries);
			return false;
		}
	}

	NiTimeController* pkControl0 = m_spControllers;
	NiTimeController* pkControl1 = pkNET->m_spControllers;
	int32_t uiControlCount0 = pkControl0 ? pkControl0->GetLast() : 0;
	int32_t uiControlCount1 = pkControl1 ? pkControl1->GetLast() : 0;
	if (uiControlCount0 != uiControlCount1) {
		FNVD_DESTRUCT(IndexArrayBase)(&kOtherEntries);
		FNVD_DESTRUCT(IndexArrayBase)(&kSelfEntries);
		return false;
	}
	while (pkControl0 && pkControl1) {
		if (!pkControl0->IsEqual(pkControl1)) {
			FNVD_DESTRUCT(IndexArrayBase)(&kOtherEntries);
			FNVD_DESTRUCT(IndexArrayBase)(&kSelfEntries);
			return false;
		}
		pkControl0 = pkControl0->m_spNext.m_pObject;
		pkControl1 = pkControl1->m_spNext.m_pObject;
	}
	FNVD_DESTRUCT(IndexArrayBase)(&kOtherEntries);
	FNVD_DESTRUCT(IndexArrayBase)(&kSelfEntries);
	return true;
}

void NiObjectNET::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiObject::GetViewerStrings(pkStrings);
	char* ViewerString = NiGetViewerString(NiObjectNET::ms_RTTI.GetName());
	uint32_t m_usSize = pkStrings->m_usSize;
	uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
	char* v10 = ViewerString;
	if (m_usSize >= m_usMaxSize) {
		pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(m_usSize, v10);
	char* ViewerString_9 = NiGetViewerString("m_kName", m_kName.GetHandle());
	uint32_t v8 = pkStrings->m_usSize;
	uint32_t v9 = pkStrings->m_usMaxSize;
	v10 = ViewerString_9;
	if (v8 >= v9) {
		pkStrings->SetSize(v8 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v8, v10);
}

void NiObjectNET::ProcessClone(NiCloningProcess& kCloning) {
	NiObject::ProcessClone(kCloning);
	for (uint16_t i = 0; i < m_usExtraDataSize; ++i) {
		NiExtraData* pkExtra = m_ppkExtra[i];
		if (pkExtra) {
			pkExtra->ProcessClone(kCloning);
		}
	}
	NiTimeController* pkController = m_spControllers;
	if (pkController) {
		pkController->ProcessClone(kCloning);
	}
}

// A legacy chain left by LinkObject is unlinked first: each datum's name field holds the next one, so the name is
// cleared before the datum is added. Every NiVertWeightsExtraData is then removed, the scan restarting after each.
void NiObjectNET::PostLinkObject(NiStream& kStream) {
	// The empty NiObject::PostLinkObject, which the exe calls out of line (its folded body at 0xA89AF0).
	NiObject::PostLinkObject(kStream);
	if (kStream.m_uiNifFileVersion < 0x0500000B) {
		NiExtraData* pkExtra = m_pkLegacyExtra;
		if (pkExtra) {
			m_ppkExtra = nullptr;
			do {
				// The exe reads the name's handle as the next datum's address (raw memory, not a string here).
				NiExtraData* pkNext = reinterpret_cast<NiExtraData*>(const_cast<char*>(pkExtra->GetName().GetHandle()));
				pkExtra->ClearName();
				AddExtraData(pkExtra);
				pkExtra = pkNext;
			} while (pkExtra);
		}
	}
	for (int32_t i = 0; static_cast<uint32_t>(i) < m_usExtraDataSize; ++i) {
		NiExtraData* pkExtraData = m_ppkExtra[static_cast<uint16_t>(i)];
		if (pkExtraData && pkExtraData->GetRTTI() == &NiVertWeightsExtraData::ms_RTTI) {
			// EXE-INLINED 0xA5BF50
			ExtraDataCrit.Lock();
			if (static_cast<uint16_t>(i) < m_usExtraDataSize) {
				DeleteExtraData(i);
			}
			ExtraDataCrit.Unlock();
			i = -1;
		}
	}
}

// The exe reads only the low word of the index.
void NiObjectNET::DeleteExtraData(int32_t usExtraDataIndex) {
	ExtraDataCrit.Lock();
	uint16_t usIndex = static_cast<uint16_t>(usExtraDataIndex);
	if (usIndex < m_usExtraDataSize) {
		NiExtraData* pkExtra = m_ppkExtra[usIndex];
		if (pkExtra) {
			pkExtra->DecRefCount();
		}
		for (uint16_t i = usIndex; static_cast<int32_t>(i) < m_usExtraDataSize - 1; ++i) {
			m_ppkExtra[i] = m_ppkExtra[i + 1];
		}
		NiExtraData** ppkExtra = m_ppkExtra;
		--m_usExtraDataSize;
		ppkExtra[m_usExtraDataSize] = nullptr;
	}
	ExtraDataCrit.Unlock();
}

// The exe compares the low word of the index and passes its whole argument dword on to DeleteExtraData.
bool NiObjectNET::RemoveExtraDataAt(int32_t usExtraDataIndex) {
	ExtraDataCrit.Lock();
	if (static_cast<uint16_t>(usExtraDataIndex) >= m_usExtraDataSize) {
		ExtraDataCrit.Unlock();
		return false;
	}
	DeleteExtraData(usExtraDataIndex);
	ExtraDataCrit.Unlock();
	return true;
}

// Only the low word of the size is read.
bool NiObjectNET::SetExtraDataSize(uint16_t usSize) {
	uint16_t usNewSize = static_cast<uint16_t>(usSize);
	if (!usNewSize) {
		return false;
	}
	ExtraDataCrit.Lock();
	bool bEmpty = m_usMaxSize == 0;
	m_usMaxSize = usNewSize;
	if (bEmpty) {
		m_ppkExtra = static_cast<NiExtraData**>(_NiMalloc(sizeof(NiExtraData*) * usNewSize));
		m_usExtraDataSize = 0;
	} else {
		NiExtraData** ppkExtra = static_cast<NiExtraData**>(_NiMalloc(sizeof(NiExtraData*) * usNewSize));
		// The asm calls memcpy at 0xEC44D0, despite the packet's inline-copy annotation.
		rt::memcpy(ppkExtra, m_ppkExtra, sizeof(NiExtraData*) * m_usExtraDataSize);
		_NiFree(m_ppkExtra);
		m_ppkExtra = ppkExtra;
	}
	ExtraDataCrit.Unlock();
	return true;
}

void NiObjectNET::RemoveAllExtraData() {
	ExtraDataCrit.Lock();
	for (int16_t i = static_cast<int16_t>(m_usExtraDataSize - 1); i >= 0; --i) {
		DeleteExtraData(static_cast<uint16_t>(i));
	}
	_NiFree(m_ppkExtra);
	m_ppkExtra = nullptr;
	m_usMaxSize = 0;
	m_usExtraDataSize = 0;
	ExtraDataCrit.Unlock();
}

bool NiObjectNET::InsertExtraData(NiExtraData* pkExtra) {
	if (!pkExtra) {
		return 0;
	}
	NiIncRefCount(pkExtra);
	EnterCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
	uint16_t m_usMaxSize = this->m_usMaxSize;
	if (m_usMaxSize) {
		int32_t i_2;
		if (m_usExtraDataSize == m_usMaxSize) {
			uint16_t v6 = 2 * m_usMaxSize + 1;
			this->m_usMaxSize = v6;
			NiExtraData** ppkNewExtra_2 = static_cast<NiExtraData**>(_NiMalloc(sizeof(NiExtraData*) * v6));
			rt::memcpy(ppkNewExtra_2, this->m_ppkExtra, sizeof(NiExtraData*) * m_usExtraDataSize);
			_NiFree(this->m_ppkExtra);
			this->m_ppkExtra = ppkNewExtra_2;
		}
		this->m_ppkExtra[m_usExtraDataSize++] = pkExtra;
		for (int32_t i = m_usExtraDataSize; static_cast<uint16_t>(i) < this->m_usMaxSize; this->m_ppkExtra[i_2] = 0) {
			i_2 = static_cast<uint16_t>(i++);
		}
		uint16_t i_3 = m_usExtraDataSize - 1;
		if (m_usExtraDataSize == 1) {
		LABEL_13:;
			LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
			return 1;
		} else {
			while (1) {
				int32_t i_4 = i_3;
				const char* m_kHandle = (&this->m_ppkExtra[i_4 - 1]->GetName())->GetHandle();
				int32_t v13 = m_kHandle - (&this->m_ppkExtra[i_4]->GetName())->GetHandle();
				if (!v13) {
					break;
				}
				if (v13 > 0) {
					NiExtraData** m_ppkExtra = this->m_ppkExtra;
					NiExtraData* pkTmpED = m_ppkExtra[i_4 - 1];
					m_ppkExtra[i_3 - 1] = m_ppkExtra[i_3];
					--i_3;
					this->m_ppkExtra[i_4] = pkTmpED;
					if (i_3) {
						continue;
					}
				}
				goto LABEL_13;
			}
			DeleteExtraData(i_3);
			LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
			return 0;
		}
	} else {
		this->m_usMaxSize = 1;
		m_usExtraDataSize = 1;
		NiExtraData** ppkNewExtra = static_cast<NiExtraData**>(_NiMalloc(sizeof(NiExtraData*)));
		this->m_ppkExtra = ppkNewExtra;
		*ppkNewExtra = pkExtra;
		LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
		return 1;
	}
}

const NiFixedString& NiObjectNET::GetName() const {
	return m_kName;
}

NiTimeController* NiObjectNET::GetControllers() const {
	return m_spControllers;
}

void NiObjectNET::SetControllers(NiTimeController* pkControl) {
	if (m_spControllers == pkControl) {
		return;
	}
	if (m_spControllers) {
		NiDecRefCount(m_spControllers);
	}
	m_spControllers = pkControl;
	if (pkControl) {
		NiIncRefCount(pkControl);
	}
}

void NiObjectNET::PrependController(NiTimeController* pkControl) {
	if (!pkControl) {
		return;
	}

	pkControl->m_spNext = m_spControllers;

	if (m_spControllers != pkControl) {
		if (m_spControllers) {
			NiDecRefCount(m_spControllers);
		}
		m_spControllers = pkControl;
		NiIncRefCount(pkControl);
	}
}

NiTimeController* NiObjectNET::GetController(const NiRTTI* pkRTTI) const {
	for (NiTimeController* pController = m_spControllers; pController; pController = pController->m_spNext) {
		const NiRTTI* pRTTI = pController->GetRTTI();
		for (; pRTTI; pRTTI = pRTTI->m_pkBaseRTTI) {
			if (pRTTI == pkRTTI) {
				return pController;
			}
		}
	}
	return nullptr;
}

// The removed controller is held across the unlink and released afterwards. Unlinked from the head, the exe inlines
// the hold and its release; unlinked from further down, the hold is pkControl's own argument slot, released through
// the out-of-line NiPointer destructor.
void NiObjectNET::RemoveController(NiTimeController* pkControl) {
	if (!pkControl || !m_spControllers) {
		return;
	}
	if (m_spControllers == pkControl) {
		pkControl->IncRefCount();
		AssignControllers(&m_spControllers, pkControl->m_spNext.m_pObject);
		pkControl->m_spNext = nullptr;
		pkControl->DecRefCount();
		return;
	}
	NiTimeController* pkPrev = m_spControllers;
	NiTimeController* pkCurr = pkPrev->m_spNext.m_pObject;
	while (pkCurr && pkCurr != pkControl) {
		pkPrev = pkCurr;
		pkCurr = pkCurr->m_spNext.m_pObject;
	}
	if (!pkCurr) {
		return;
	}
	pkControl->IncRefCount();
	pkPrev->SetNext(pkControl->m_spNext.m_pObject);
	pkControl->m_spNext = nullptr;
	// Releases the reference taken above: the exe runs NiPointer's destructor (0x45CEC0) on the parameter's slot.
	if (pkControl) {
		pkControl->DecRefCount();
	}
}

void NiObjectNET::RemoveAllControllers() {
	NiTimeController* pkController = m_spControllers;
	if (pkController) {
		NiDecRefCount(pkController);
		m_spControllers = nullptr;
	}
}

// The name (shared, or with the append character for a unique copy), clones of the cloneable extra data, and a clone
// of the controller chain. The fixed-string and smart-pointer reference counts are inlined in the exe; its string
// counts are the untyped first dword of the interned buffer, decremented and incremented as raw memory.
void NiObjectNET::CopyMembers(NiObjectNET* pkDest, NiCloningProcess& kCloning) {
	NiObject::CopyMembers(pkDest, kCloning);
	if (kCloning.m_eCopyType == COPY_EXACT) {
		// EXE-INLINED none: NiFixedString::operator=
		NiGlobalStringTable::GlobalStringHandle kHandle = m_kName.GetHandle();
		if (pkDest->m_kName.GetHandle() != kHandle) {
			if (kHandle != NiGlobalStringTable::NULL_STRING) {
				// EXE-INLINED 0x438210
				InterlockedIncrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kHandle - 8)));
			}
			NiGlobalStringTable::GlobalStringHandle kOld = pkDest->m_kName.GetHandle();
			if (kOld != NiGlobalStringTable::NULL_STRING) {
				// EXE-INLINED 0x438210
				InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kOld - 8)));
			}
			pkDest->m_kName.SetHandle(kHandle);
		}
	} else if (kCloning.m_eCopyType == COPY_UNIQUE) {
		if (m_kName.GetHandle()) {
			size_t stQuantity = strlen(m_kName.GetHandle()) + 2;
			char* pcNamePlus = static_cast<char*>(_NiMalloc(stQuantity));
			strcpy_s(pcNamePlus, stQuantity, m_kName.GetHandle());
			pcNamePlus[stQuantity - 2] = kCloning.m_cAppendChar;
			pcNamePlus[stQuantity - 1] = 0;
			// The temporary NiFixedString(pcNamePlus), its constructor inlined.
			// EXE-INLINED 0x438170
			NiGlobalStringTable::GlobalStringHandle kNew =
				pcNamePlus ? NiGlobalStringTable::AddString(pcNamePlus) : NiGlobalStringTable::NULL_STRING;
			NiFixedString kName;
			kName.SetHandle(kNew);
			pkDest->SetName(kName);
			// EXE-INLINED 0x4381B0
			if (kNew != NiGlobalStringTable::NULL_STRING) {
				// EXE-INLINED 0x438210
				InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kNew - 8)));
			}
			_NiFree(pcNamePlus);
		}
	}
	for (uint16_t i = 0; i < m_usExtraDataSize; ++i) {
		NiExtraData* pkExtra = m_ppkExtra[i];
		if (pkExtra && pkExtra->IsCloneable()) {
			pkDest->AddExtraData(static_cast<NiExtraData*>(pkExtra->CreateClone(kCloning)));
		}
	}
	if (m_spControllers) {
		NiTimeController* pkClone = static_cast<NiTimeController*>(m_spControllers->CreateClone(kCloning));
		// EXE-INLINED none: NiPointer<NiTimeController>::operator=
		NiTimeController* pkOld = pkDest->m_spControllers;
		if (pkOld != pkClone) {
			if (pkOld) {
				pkOld->DecRefCount();
			}
			pkDest->m_spControllers = pkClone;
			if (pkClone) {
				pkClone->IncRefCount();
			}
		}
	}
}

NiExtraData* NiObjectNET::GetExtraData(const NiFixedString& kKey) const {
	if (kKey.GetHandle() == NiGlobalStringTable::NULL_STRING) {
		return nullptr;
	}
	ExtraDataCrit.Lock();
	int16_t iHigh = static_cast<int16_t>(m_usExtraDataSize - 1);
	int16_t iLow = 0;
	while (iLow <= iHigh) {
		int16_t iMiddle = static_cast<int16_t>((iLow + iHigh) >> 1);
		const char* pcKey = kKey.GetHandle();
		const char* pcName = m_ppkExtra[iMiddle]->GetName().GetHandle();
		// The exe subtracts untyped handle addresses, not character positions within one string.
		int32_t iDifference =
			static_cast<int32_t>(reinterpret_cast<uintptr_t>(pcKey) - reinterpret_cast<uintptr_t>(pcName));
		if (!iDifference) {
			ExtraDataCrit.Unlock();
			return m_ppkExtra[iMiddle];
		}
		if (iDifference > 0) {
			iLow = static_cast<int16_t>(iMiddle + 1);
		} else {
			iHigh = static_cast<int16_t>(iMiddle - 1);
		}
	}
	ExtraDataCrit.Unlock();
	return nullptr;
}

bool NiObjectNET::RemoveExtraData(const NiFixedString& kKey) {
	if (!m_usExtraDataSize) {
		return 0;
	}
	EnterCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
	if ((&kKey)->GetHandle() == NiGlobalStringTable::NULL_STRING) {
		LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
		return 0;
	}
	int16_t sTop = m_usExtraDataSize - 1;
	int16_t sBottom = 0;
	if (sTop < 0) {
	LABEL_11:;
		LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
		return 0;
	} else {
		int16_t sMiddle;
		while (1) {
			const char* m_kHandle = (&kKey)->GetHandle();
			sMiddle = (sBottom + sTop) >> 1;
			int32_t v8 = m_kHandle - (&this->m_ppkExtra[sMiddle]->GetName())->GetHandle();
			if (!v8) {
				break;
			}
			if (v8 <= 0) {
				sTop = sMiddle - 1;
			} else {
				sBottom = sMiddle + 1;
			}
			if (sBottom > sTop) {
				goto LABEL_11;
			}
		}
		DeleteExtraData(static_cast<uint16_t>(sMiddle));
		LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
		return 1;
	}
}

// Names an unnamed pkExtra "<RTTI name up to 'ExtraData'>ED<index>" before inserting it.
bool NiObjectNET::AddExtraData(NiExtraData* pkExtra) {
	if (!pkExtra) {
		return false;
	}
	if (!pkExtra->GetName().GetHandle()) {
		const char* pcRTTI = pkExtra->GetRTTI()->m_pcName;
		if (pcRTTI && strlen(pcRTTI) > 0) {
			char pcSuffix[6];
			NiSprintf(pcSuffix, 6, "ED%03d", m_usExtraDataSize);
			const char* pcSubstr = std::strstr(pcRTTI, "ExtraData");
			uint32_t uiStrLength = 0;
			if (pcSubstr > pcRTTI) {
				uiStrLength = static_cast<uint32_t>(pcSubstr - pcRTTI);
			}
			uint32_t uiLen = static_cast<uint32_t>(strlen(pcSuffix)) + uiStrLength + 1;
			char* pcGeneratedKey = static_cast<char*>(_NiMalloc(uiLen));
			NiStrncpy(pcGeneratedKey, uiLen, pcRTTI, uiStrLength);
			strcat_s(pcGeneratedKey, uiLen, pcSuffix);
			NiFixedString kName(pcGeneratedKey);
			pkExtra->SetName(kName);
			// The temporary's destructor, inlined: drops the reference unless it is the null handle. The count is
			// the untyped first dword of the interned buffer, which the exe decrements as raw memory.
			// EXE-INLINED 0x4381B0
			NiGlobalStringTable::GlobalStringHandle kHandle = kName.GetHandle();
			if (kHandle != NiGlobalStringTable::NULL_STRING) {
				InterlockedDecrementUInt32(reinterpret_cast<
					volatile uint32_t*>(const_cast<char*>(NiGlobalStringTable::GetRealBufferStart(kHandle))));
			}
			_NiFree(pcGeneratedKey);
		}
	}
	return InsertExtraData(pkExtra);
}

bool NiObjectNET::AddExtraData(const NiFixedString& kKey, NiExtraData* pkExtra) {
	if ((&kKey)->GetHandle() == NiGlobalStringTable::NULL_STRING) {
		return 0;
	}
	if (pkExtra) {
		if ((&pkExtra->GetName())->GetHandle() == NiGlobalStringTable::NULL_STRING) {
			pkExtra->SetName(kKey);
			return InsertExtraData(pkExtra);
		}
		if ((&kKey)->GetHandle() == (&pkExtra->GetName())->GetHandle()) {
			return InsertExtraData(pkExtra);
		}
	}
	return 0;
}

void NiObjectNET::SetName(const NiFixedString& strName) {
	NiGlobalStringTable::GlobalStringHandle kHandle = strName.GetHandle();
	if (m_kName.GetHandle() != kHandle) {
		// EXE-INLINED 0x43BA10
		NiGlobalStringTable::AcquireHandle(kHandle);
		NiGlobalStringTable::ReleaseHandle(m_kName.GetHandle());
		m_kName.SetHandle(kHandle);
	}
}

void NiObjectNET::SetDefaultCopyType(CopyType eCopyType) {
	ms_eCopyType = eCopyType;
}

NiTimeController** NiObjectNET::AssignControllers(NiTimeController** appSlot, NiTimeController* apController) {
	NiTimeController* pkOld = *appSlot;
	if (pkOld != apController) {
		if (pkOld)
			NiDecRefCount(pkOld);
		*appSlot = apController;
		if (apController)
			NiIncRefCount(apController);
	}
	return appSlot;
}
