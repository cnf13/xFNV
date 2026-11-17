#include "Fallout/Misc/SaveLoad/BGSSaveLoadChangesMap.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSUnloadedFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

BGSSaveLoadChangesMap::BGSSaveLoadChangesMap() : NiTPointerMap<uint32_t, BGSFormChanges*>(0x13AFu) {}

BGSSaveLoadChangesMap::~BGSSaveLoadChangesMap() {
	RemoveAllChanges();
}

bool BGSSaveLoadChangesMap::RemoveChanges(uint32_t aiFormID) {
	BGSFormChanges* Changes = GetChanges(aiFormID);
	if (!Changes) {
		return 0;
	}
	delete Changes;
	RemoveAt(aiFormID);
	return 1;
}

BGSFormChanges* BGSSaveLoadChangesMap::GetChanges(uint32_t aiFormID) {
	BGSFormChanges* pChanges = nullptr;
	if (GetAt(aiFormID, pChanges)) {
		return pChanges;
	}
	return nullptr;
}

// The flags are copied inline when the form has changes; otherwise the exe constructs empty flags out of line.
BGSChangeFlags* BGSSaveLoadChangesMap::GetChangeFlags(BGSChangeFlags* apResult, uint32_t aiFormID) {
	BGSFormChanges* pChanges = GetChanges(aiFormID);
	if (pChanges) {
		*apResult = pChanges->iChangeFlags;
	} else {
		::new (apResult) BGSChangeFlags(0);
	}
	return apResult;
}

BGSUnloadedFormBuffer* BGSSaveLoadChangesMap::GetChangeBuffer(BGSUnloadedFormBuffer* apResult, uint32_t aiFormID) {
	BGSFormChanges* Changes = GetChanges(aiFormID);
	if (Changes) {
		apResult->pBuffer = Changes->UnloadedFormBuffer.pBuffer;
	} else {
		apResult->pBuffer = 0;
	}
	return apResult;
}

// The exe reads the flags through BGSChangeFlags::operator int out of line.
bool BGSSaveLoadChangesMap::CheckChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags) {
	BGSFormChanges* pChanges = GetChanges(aiFormID);
	if (!pChanges) {
		return false;
	}
	return pChanges->iChangeFlags.CheckFlags(aiChangeFlags.operator int32_t());
}

// The exe's cleanup-only C++ EH frame is left out.
void BGSSaveLoadChangesMap::AddChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags) {
	BGSFormChanges* pformChanges = GetChanges(aiFormID);
	if (pformChanges) {
		if (pformChanges->UnloadedFormBuffer.pBuffer) {
			return;
		}
	} else {
		BGSFormChanges* pformChanges_2;
		BGSFormChanges* apMemory = static_cast<BGSFormChanges*>(ourMalloc(sizeof(BGSFormChanges)));
		if (apMemory) {
			pformChanges_2 = ::new (apMemory) BGSFormChanges;
		} else {
			pformChanges_2 = 0;
		}
		pformChanges = pformChanges_2;
		SetAt(aiFormID, pformChanges_2);
	}
	pformChanges->iChangeFlags.AddFlags(aiChangeFlags.iFlags);
}

bool BGSSaveLoadChangesMap::RemoveChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags) {
	BGSFormChanges* pformChanges = GetChanges(aiFormID);
	if (pformChanges && !pformChanges->UnloadedFormBuffer.pBuffer) {
		pformChanges->iChangeFlags.RemoveFlags(aiChangeFlags.iFlags);
		if (pformChanges->iChangeFlags.iFlags) {
			return 0;
		}
		if (pDataHandler->IsFormIDCreated(aiFormID)) {
			return 0;
		}
		delete pformChanges;
		RemoveAt(aiFormID);
	}
	return 1;
}

// The exe's cleanup-only C++ EH frame is left out.
void BGSSaveLoadChangesMap::SetChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags) {
	BGSFormChanges* pformChanges = GetChanges(aiFormID);
	if (!pformChanges) {
		BGSFormChanges* pformChanges_2;
		BGSFormChanges* apMemory = static_cast<BGSFormChanges*>(ourMalloc(sizeof(BGSFormChanges)));
		if (apMemory) {
			pformChanges_2 = ::new (apMemory) BGSFormChanges;
		} else {
			pformChanges_2 = 0;
		}
		pformChanges = pformChanges_2;
		SetAt(aiFormID, pformChanges_2);
	}
	pformChanges->iChangeFlags.iFlags = aiChangeFlags.iFlags;
}

// The exe's cleanup-only C++ EH frame is left out.
void BGSSaveLoadChangesMap::SetChanges(uint32_t aiFormID, BGSChangeFlags aiChangeFlags,
	BGSUnloadedFormBuffer akBuffer) {
	BGSFormChanges* a3 = GetChanges(aiFormID);
	if (!a3) {
		BGSFormChanges* v4;
		BGSFormChanges* apMemory = static_cast<BGSFormChanges*>(ourMalloc(sizeof(BGSFormChanges)));
		if (apMemory) {
			v4 = ::new (apMemory) BGSFormChanges;
		} else {
			v4 = 0;
		}
		a3 = v4;
		SetAt(aiFormID, v4);
	}
	a3->iChangeFlags.iFlags = aiChangeFlags.iFlags;
	a3->UnloadedFormBuffer = akBuffer;
}

void BGSSaveLoadChangesMap::RemoveAllChanges() {
	NiTMapItem<uint32_t, BGSFormChanges*>* it;
	it = GetFirstPos();
	while (it) {
		BGSFormChanges* pchanges = 0;
		uint32_t iformID = 0;
		GetNext(it, iformID, pchanges);
		if (pchanges) {
			delete pchanges;
		}
	}
	RemoveAll();
}

BGSFormChanges::BGSFormChanges() : iChangeFlags(), UnloadedFormBuffer() {}

BGSFormChanges::~BGSFormChanges() {
	ourFree(UnloadedFormBuffer.pBuffer);
}
