#include "Fallout Shared/TESAnimation.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESFile.h"
#include "SDK/MSVC/rtti.h"

TESAnimation::TESAnimation() {}

TESAnimation::~TESAnimation() {
	TESAnimation::ClearDataComponent();
}

// Frees every KF file name, emptying the list from the head.
void TESAnimation::ClearDataComponent() {
	while (!kfFilenames.IsEmpty()) {
		char* pFilename = kfFilenames.GetItem();
		kfFilenames.RemoveHead();
		ourFree(pFilename);
	}
}

void TESAnimation::CopyComponent(BaseFormComponent* apCopyFC) {
	TESAnimation* pcopy = dynamic_cast<TESAnimation*>(apCopyFC);
	if (pcopy) {
		ClearDataComponent();
		for (const char* pkfFilename : pcopy->kfFilenames) {
			if (pkfFilename) {
				AddKFFile(pkfFilename);
			}
		}
	}
}

bool TESAnimation::CompareComponent(BaseFormComponent* apCompareFC) {
	BSSimpleList<char*>* pkfFileListCompare;
	TESAnimation* pcompare = dynamic_cast<TESAnimation*>(apCompareFC);
	if (!pcompare) {
		return 1;
	}
	BSSimpleList<char*>* pkfFileList = &kfFilenames;
	for (pkfFileListCompare = &pcompare->kfFilenames; pkfFileList && pkfFileListCompare;
		pkfFileListCompare = pkfFileListCompare->m_pkNext) {
		const char* pkfFilename = pkfFileList->m_item;
		const char* pkfFilenameCompare = pkfFileListCompare->m_item;
		if ((!pkfFilename && pkfFilenameCompare) || (pkfFilename && !pkfFilenameCompare)) {
			return 1;
		}
		if (pkfFilename && (pkfFilenameCompare && BSstricmp(pkfFilename, pkfFilenameCompare))) {
			return 1;
		}
		pkfFileList = pkfFileList->m_pkNext;
	}
	return pkfFileList || pkfFileListCompare;
}

void TESAnimation::AddKFFile(const char* apFilename) {
	if (!IsKFFileInList(apFilename)) {
		int32_t iFilenameLen = BSstrlen(apFilename) + 1;
		char* pkfFilename = static_cast<char*>(ourMalloc(iFilenameLen));
		BSstrcpy(pkfFilename, iFilenameLen, apFilename);
		kfFilenames.AddTail(pkfFilename);
	}
}

void TESAnimation::LoadAnimationChunk(TESAnimation* apAnimation, TESFile* apFile) {
	if (apFile) {
		if (apAnimation) {
			if (apFile->GetTESChunk() == KFFZ_ID) {
				uint32_t ChunkSize = apFile->GetChunkSize();
				uint8_t* apData = static_cast<uint8_t*>(ourMalloc(ChunkSize));
				char* apString = reinterpret_cast<char*>(apData);
				apFile->GetChunkData(apData, 0);
				while (*apString) {
					AddKFFile(apString);
					apString += BSstrlen(apString) + 1;
				}
				ourFree(apData);
			}
		}
	}
}

bool TESAnimation::HasKFFiles() {
	return (bool)((uint8_t)(!(&kfFilenames)->IsEmpty()));
}

BSSimpleList<char*>* TESAnimation::GetKFFileList() {
	return &kfFilenames;
}

bool TESAnimation::IsKFFileInList(const char* apFilename) {
	char* apString1;
	BSSimpleList<char*>* ecx0;
	bool vCondition1;
	vCondition1 = (&kfFilenames)->IsEmpty();
	if (vCondition1) {
		return (bool)0;
	}
	for (ecx0 = &kfFilenames; ecx0; ecx0 = ecx0->m_pkNext) {
		apString1 = ecx0->m_item;
		if (!BSstricmp(apString1, apFilename)) {
			return (bool)1;
		}
	}
	return (bool)0;
}
