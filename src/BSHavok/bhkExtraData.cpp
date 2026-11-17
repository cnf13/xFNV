#include "BSHavok/bhkExtraData.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI bhkExtraData::ms_RTTI("bhkExtraData", &NiExtraData::ms_RTTI);

// The exe's constructor sequence: the base's constructor, the class vtable, the controller array built in place,
// then the name "HKE" with the temporary string's reference dropped at once. The EH frame is left out.
// An empty controller array growing by one, and the name "HKE".
bhkExtraData::bhkExtraData() : pControllerA(0, 1) {
	SetName(NiFixedString("HKE"));
}

// The exe's destructor sequence: the class vtable, the controller array destroyed inside the Havok heap context,
// that context restored, the array's vtable reset to the base array's with its buffer freed, then the base
// destructor. The EH frame is left out.
// pControllerA's own destructor releases the controllers and frees its buffer.
bhkExtraData::~bhkExtraData() {}

const NiRTTI* bhkExtraData::GetRTTI() const {
	return &bhkExtraData::ms_RTTI;
}

// The clone is allocated inside the Havok heap context (inlined switch), built by the constructor and filled in by
// CopyMembers, which the exe calls even when the allocation failed.
NiObject* bhkExtraData::CreateClone(NiCloningProcess& arCloneData) {
	int32_t& iMemContext = GetThreadMemContext();
	MEM_CONTEXT eOldMemContext = static_cast<MEM_CONTEXT>(iMemContext);
	iMemContext = MEM_CONTEXT_HAVOK;

	bhkExtraData* pkClone = nullptr;
	void* pvMemory = NiMemObject::operator new(sizeof(bhkExtraData));
	if (pvMemory) {
		pkClone = ::new (pvMemory) bhkExtraData;
	}
	CopyMembers(pkClone, arCloneData);

	iMemContext = eOldMemContext;
	return pkClone;
}

// The base body is called directly, then the controller link IDs are read.
void bhkExtraData::LoadBinary(NiStream& arStream) {
	NiExtraData::LoadBinary(arStream);
	arStream.ReadMultipleLinkIDs();
}

// The linked controllers are taken from the stream's queued link IDs, one per count the file stored. The exe
// brackets the whole body with a switch of the thread's heap context to Havok (inlined).
void bhkExtraData::LinkObject(NiStream& arStream) {
	int32_t& iMemContext = GetThreadMemContext();
	MEM_CONTEXT eOldMemContext = static_cast<MEM_CONTEXT>(iMemContext);
	iMemContext = MEM_CONTEXT_HAVOK;

	NiExtraData::LinkObject(arStream);

	// The count is read once and counted down, as the exe does.
	for (uint32_t uiLinks = arStream.GetNumberOfLinkIDs(); uiLinks != 0; --uiLinks) {
		NiObject* pkObject = arStream.GetObjectFromLinkID();
		if (pkObject) {
			PrependController(static_cast<NiTimeController*>(pkObject));
		}
	}

	iMemContext = eOldMemContext;
}

// The base result is kept in a local and returned after the loop: the exe spills it to the stack before the
// controllers' registration (which clobbers al) and reloads it for the return.
bool bhkExtraData::RegisterStreamables(NiStream& arStream) {
	bool bRegistered = NiExtraData::RegisterStreamables(arStream);
	// The exe reads the effective size once and compares every index against that copy.
	uint32_t uiCount = pControllerA.m_uiESize;
	for (uint32_t i = 0; i < uiCount; ++i) {
		NiTimeController* pkController = pControllerA.m_pBase[i].m_pObject;
		if (pkController) {
			// Virtual, slot 21: NiTimeController::RegisterStreamables.
			pkController->RegisterStreamables(arStream);
		}
	}
	return bRegistered;
}

// The count goes out as a 4-byte write on the output stream (the inlined NiStreamSaveBinary<uint32_t>), then one
// link ID per non-null controller.
void bhkExtraData::SaveBinary(NiStream& arStream) {
	NiExtraData::SaveBinary(arStream);

	// The exe reads the effective size before the write and loops over that copy.
	uint32_t uiCount = pControllerA.m_uiESize;
	NiStreamSaveBinary(arStream, uiCount);
	for (uint32_t i = 0; i < uiCount; ++i) {
		NiTimeController* pkController = pControllerA.m_pBase[i].m_pObject;
		if (pkController) {
			// Virtual, slot 12: NiStream::SaveLinkID.
			arStream.SaveLinkID(pkController);
		}
	}
}

bool bhkExtraData::IsEqual(NiObject* apObject) {
	return (bool)((uint8_t)NiExtraData::IsEqual(apObject));
}

// The exe's cleanup-only C++ EH frame is left out.
void bhkExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	TLSData* v11 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v11->eMemContext;
	v11->eMemContext = MEM_CONTEXT_HAVOK;
	NiExtraData::GetViewerStrings(apStrings);
	char* ViewerString = NiGetViewerString(bhkExtraData::ms_RTTI.GetName());
	uint32_t m_usSize = apStrings->m_usSize;
	uint32_t m_usMaxSize = apStrings->m_usMaxSize;
	char* v12 = ViewerString;
	if (m_usSize >= m_usMaxSize) {
		apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(m_usSize, v12);
	uint32_t m_uiESize = pControllerA.m_uiESize;
	uint32_t I = 0;
	if (m_uiESize) {
		do {
			NiTimeController* pcon = pControllerA.m_pBase[I].m_pObject;
			if (pcon) {
				pcon->GetViewerStrings(apStrings);
			}
			++I;
		} while (I < m_uiESize);
		v11->eMemContext = eMemContext;
	} else {
		v11->eMemContext = eMemContext;
	}
}

void bhkExtraData::ProcessClone(NiCloningProcess& arCloneData) {
	NiObject::ProcessClone(arCloneData);
}

// The exe's cleanup-only C++ EH frame is left out. The exe keeps the counted reference in apController's own argument
// slot and passes that slot to SetAt; the reference is released after it.
void bhkExtraData::PrependController(NiTimeController* apController) {
	if (apController) {
		NiPointer<NiTimeController> spController = apController;
		uint32_t uiSize = pControllerA.m_uiSize;
		if (uiSize >= pControllerA.m_uiMaxSize) {
			pControllerA.SetSize(uiSize + pControllerA.m_uiGrowBy);
		}
		pControllerA.SetAt(uiSize, spController);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void bhkExtraData::CopyMembers(bhkExtraData* pDest, NiCloningProcess& arCloneData) {
	TLSData* v8 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v8->eMemContext;
	v8->eMemContext = MEM_CONTEXT_HAVOK;
	uint32_t I = 0;
	NiExtraData::CopyMembers(pDest, arCloneData);
	uint32_t uiconts = pControllerA.m_uiESize;
	pDest->pControllerA.SetSize(uiconts);
	if (uiconts) {
		do {
			NiTimeController* pcon = pControllerA.m_pBase[I].m_pObject;
			if (pcon) {
				pcon = static_cast<NiTimeController*>(pcon->CreateClone(arCloneData));
			}
			pDest->PrependController(pcon);
			++I;
		} while (I < uiconts);
		v8->eMemContext = eMemContext;
	} else {
		v8->eMemContext = eMemContext;
	}
}

// The class' stream loader entry: a bhkExtraData from the engine heap, named with the empty string. The exe calls
// SetName even when the allocation failed.
NiObject* bhkExtraData::CreateObject() {
	void* pvMemory = NiMemObject::operator new(sizeof(bhkExtraData));
	bhkExtraData* pkObject = nullptr;
	if (pvMemory) {
		pkObject = ::new (pvMemory) bhkExtraData;
	}
	pkObject->SetName(EmptyFS);
	return pkObject;
}

// The controller array's RemoveAll as the exe keeps it for this instantiation: each controller is released and its
// slot cleared, then the occupied count and the size are zeroed. The exe's cleanup-only EH frame is left out.
void bhkExtraData::DestroyControllerArray(NiTLargePrimitiveArray<NiPointer<NiTimeController>>* apArray) {
	// EXE-INLINED none: NiTLargeArray::RemoveAll
	for (uint32_t i = 0; i < apArray->m_uiSize; ++i) {
		apArray->m_pBase[i] = nullptr;
	}
	apArray->m_uiESize = 0;
	apArray->m_uiSize = 0;
}
