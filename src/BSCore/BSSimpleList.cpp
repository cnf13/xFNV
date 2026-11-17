#include "BSCore/BSSimpleList.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Xenon/XGamerProfile.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkLiquidAction.h"
#include "BSHavok/bhkSerializable.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSTMessageQueue.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/ObstacleTaskNavMeshOperation.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLoadScreen.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/Interface/Menus/HackingMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

bool au_re_dereference_70(const BSSoundHandle* apThis, [[maybe_unused]] void* apEdx, int32_t a2) {
	return (bool)((uint8_t)(a2 == apThis->iSoundID));
}

uint32_t* RemoveAll(void* apThis, [[maybe_unused]] void* apEdx) {
	static_cast<BSSimpleList<FileData*>*>(apThis)->RemoveAll();
	// Returns the list, as RemoveAll leaves it.
	return static_cast<uint32_t*>(apThis);
}

// The sound handle list's empty item is BSSoundHandle(0), not the default (invalid) handle.
template <> BSSimpleList<BSSoundHandle>::BSSimpleList() : m_item(0u), m_pkNext(nullptr) {}

// As the generic AddHead, with the handle's tests: an item with sound ID 0 is ignored, and the head's item counts as
// present when its ID is not 0.
template <> void BSSimpleList<BSSoundHandle>::AddHead(const BSSoundHandle& item) {
	if (item.iSoundID == 0) {
		return;
	}
	if (m_item != 0u) {
		void* pMemory = ourMalloc(sizeof(BSSimpleList));
		BSSimpleList* pNode = pMemory ? new (pMemory) BSSimpleList(m_item) : nullptr;
		pNode->m_pkNext = m_pkNext;
		m_pkNext = pNode;
	}
	m_item = item;
}

// As the generic AddTail, with the handle's tests.
template <> void BSSimpleList<BSSoundHandle>::AddTail(const BSSoundHandle& item) {
	if (item.iSoundID == 0) {
		return;
	}
	BSSimpleList* pLast = this;
	while (pLast->m_pkNext) {
		pLast = pLast->m_pkNext;
	}
	if (pLast->m_item != 0u) {
		void* pMemory = ourMalloc(sizeof(BSSimpleList));
		pLast->m_pkNext = pMemory ? new (pMemory) BSSimpleList(item) : nullptr;
	} else {
		pLast->m_item = item;
	}
}

// Each node after the head is unlinked and deleted; the head's item becomes BSSoundHandle(0).
template <> void BSSimpleList<BSSoundHandle>::RemoveAll() {
	while (m_pkNext) {
		BSSimpleList* pNext = m_pkNext->m_pkNext;
		m_pkNext->m_pkNext = nullptr;
		delete m_pkNext;
		m_pkNext = pNext;
	}
	m_item = BSSoundHandle(0u);
}

// The string lists' members (the HackingMenu word lists): they free and copy items as C strings, so they are defined
// here and instantiated only for the string lists that use them.

// Prepends a heap copy of apSrc.
template <class T> void BSSimpleList<T>::AppendStr(char* apSrc) {
	uint32_t uiSize = BSstrlen(apSrc) + 1;
	char* pString = static_cast<char*>(ourMalloc(uiSize));
	BSstrcpy(pString, uiSize, apSrc);
	AddHead(pString);
}

// Frees the head's item and drops the head until the head is empty.
template <class T> BSSimpleList<T>* BSSimpleList<T>::DeleteAll() {
	while (m_item) {
		ourFree(m_item);
		RemoveHead();
	}
	return this;
}

// DeleteAll, then RemoveAll on the emptied list, which is what destroying the list in place does.
template <class T> void BSSimpleList<T>::FreeStrings() {
	DeleteAll();
	RemoveAll();
}

// Sort with the HackingMenu's random compare. The compare takes the items as void*: the char* list's items are passed
// on through a conversion.
template <class T> void BSSimpleList<T>::Randomize() {
	Sort([](T aItem1, T aItem2) -> int32_t { return RandomCompare(aItem1, aItem2); });
}

template void BSSimpleList<char*>::AppendStr(char* apSrc);
template BSSimpleList<char*>* BSSimpleList<char*>::DeleteAll();
template void BSSimpleList<char*>::FreeStrings();
template void BSSimpleList<void*>::Randomize();
template void BSSimpleList<char*>::Randomize();
