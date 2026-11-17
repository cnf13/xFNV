#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNoteListener.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/BSAnimNoteReceiver.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNote.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

// An empty receiver array growing by one.
BSAnimNoteListener::BSAnimNoteListener() : ReceiverA(0, 1) {}

// The receivers are freed; the array's own destructor frees its buffer.
BSAnimNoteListener::~BSAnimNoteListener() {
	for (uint32_t i = 0; i < ReceiverA.m_usSize; ++i) {
		MemoryManager_Singleton.Deallocate(ReceiverA.m_pBase[i]);
	}
}

// The new entry goes at the array's end through the shared SetSize and SetAt. Always returns false.
bool BSAnimNoteListener::Register(BSAnimNote::AnimNoteType aeType, IBSAnimNoteReceiver* apReceiver) {
	if (Registered(aeType, apReceiver) == -1) {
		BSAnimReceiverType* pEntry =
			static_cast<BSAnimReceiverType*>(MemoryManager_Singleton.Allocate(sizeof(BSAnimReceiverType)));
		if (pEntry) {
			pEntry->eNoteType = aeType;
			pEntry->pReceiver = apReceiver;
		} else {
			pEntry = nullptr;
		}
		ReceiverA.Add(pEntry);
	}
	return false;
}

void BSAnimNoteListener::Update(BSAnimNote* apNote) {
	uint32_t i;
	BSAnimNoteListener::BSAnimReceiverType* v4;
	for (i = 0; i < ((uint32_t)((int32_t)ReceiverA.m_usSize)); ++i) {
		v4 = ReceiverA.m_pBase[i];
		if (((int32_t)v4->eNoteType) == ((int32_t)apNote->eType)) {
			v4->pReceiver->Update(apNote);
		}
	}
}

int32_t BSAnimNoteListener::Registered(BSAnimNote::AnimNoteType aeType, IBSAnimNoteReceiver* apReceiver) {
	int32_t result = -1;
	uint32_t v4 = 0;
	if (ReceiverA.m_usSize) {
		BSAnimNoteListener::BSAnimReceiverType** m_pBase = ReceiverA.m_pBase;
		do {
			if ((*m_pBase)->eNoteType == aeType && (*m_pBase)->pReceiver == apReceiver) {
				result = v4;
			}
			++v4;
			++m_pBase;
		} while (v4 < ReceiverA.m_usSize);
	}
	return result;
}
