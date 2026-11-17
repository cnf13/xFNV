#include "Fallout Shared/BSAnimNoteReceiver.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

using BSAnimNoteReceiverTypeArray = NiTArray<BSAnimNoteReceiver::BSAnimNoteReceiverType*,
	NiTMallocInterface<BSAnimNoteReceiver::BSAnimNoteReceiverType*>>;

void IBSAnimNoteReceiver::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// The exe's cleanup-only C++ EH frame is left out.
BSAnimNoteReceiver::BSAnimNoteReceiver() : CallTypeA(0, 1u) {}

// The exe's C++ EH (unwind) frame is left out: it only matters when a call in here throws.
// The registered types are separate ourMalloc blocks the array only points to.
BSAnimNoteReceiver::~BSAnimNoteReceiver() {
	uint32_t uiSize = CallTypeA.m_usSize;
	for (uint32_t i = 0; i < uiSize; ++i) {
		ourFree(CallTypeA.RemoveEnd());
	}
}

void BSAnimNoteReceiver::Update(BSAnimNote* apNote) {
	for (uint32_t ui = 0; ui < CallTypeA.m_usSize; ++ui) {
		BSAnimNoteReceiverType* ptype = CallTypeA.operator[](ui);
		if (ptype->eType == apNote->eType) {
			CallTypeA.operator[](ui)->pCallback(pObjRefr, apNote);
		}
	}
}

// The exe does not stop at the first match: the last matching index wins.
int32_t BSAnimNoteReceiver::Registered(BSAnimNote::AnimNoteType aeType,
	void (*apCallbackFunc)(TESObjectREFR*, BSAnimNote*)) {
	int32_t iIndex = -1;
	for (uint32_t i = 0; i < CallTypeA.GetSize(); ++i) {
		if (CallTypeA[i]->eType == aeType && CallTypeA[i]->pCallback == apCallbackFunc) {
			iIndex = i;
		}
	}
	return iIndex;
}

// The exe's cleanup-only C++ EH frame is left out.
bool BSAnimNoteReceiver::Register(BSAnimNote::AnimNoteType aeType,
	void (*apCallbackFunc)(TESObjectREFR*, BSAnimNote*)) {
	bool bret = 0;
	if (Registered(aeType, apCallbackFunc) == -1) {
		BSAnimNoteReceiver::BSAnimNoteReceiverType* pReceiverType;
		void* pMemory = ourMalloc(sizeof(BSAnimNoteReceiver::BSAnimNoteReceiverType));
		if (pMemory) {
			pReceiverType = ::new (pMemory) BSAnimNoteReceiver::BSAnimNoteReceiverType(aeType, apCallbackFunc);
		} else {
			pReceiverType = 0;
		}
		BSAnimNoteReceiver::BSAnimNoteReceiverType* pReceiverType_2 = pReceiverType;
		CallTypeA.Add(pReceiverType_2);
	}
	return bret;
}

BSAnimNoteReceiver::BSAnimNoteReceiverType* BSAnimNoteReceiver::BSAnimNoteReceiverType::Construct(void* apMemory,
	BSAnimNote::AnimNoteType aeType, void (*apCallback)(TESObjectREFR*, BSAnimNote*)) {
	BSAnimNoteReceiverType* pThis = static_cast<BSAnimNoteReceiverType*>(apMemory);
	pThis->eType = aeType;
	pThis->pCallback = apCallback;
	return pThis;
}
