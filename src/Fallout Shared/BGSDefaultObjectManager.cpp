#include "Fallout Shared/BGSDefaultObjectManager.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/DEFAULT_OBJECT_DATA.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

// Reads the DOBJ record: the DATA chunk holds the default objects' form IDs, resolved later by InitItem. The exe
// keeps the record type in a byte before comparing it.
bool BGSDefaultObjectManager::Load(TESFile* apFile) {
	uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
	if (cFormType != DOBJ_ID) {
		return false;
	}
	LoadForm(apFile);
	SetInitialized(false);
	for (;;) {
		uint32_t uiChunkID = apFile->GetTESChunk();
		if (uiChunkID == NO_CHUNK) {
			break;
		}
		if (uiChunkID == DATA_ID) {
			apFile->GetChunkData(pObjectArray, sizeof(pObjectArray));
#if FNVD_64BIT
			// The file keeps a 4-byte form ID for each object, resolved by InitItem.
			TESFile::WidenFormIDRecords(pObjectArray,
				apFile->GetChunkSize() / 4 < DO_COUNT ? apFile->GetChunkSize() / 4 : DO_COUNT, 4, sizeof(TESForm*),
				{{0, 0}});
#endif
			if (apFile->QEndian()) {
				for (int32_t i = 0; i < DO_COUNT; ++i) {
					ENDIAN(pObjectArray[i], false);
				}
			}
		}
		if (!apFile->NextChunk()) {
			break;
		}
	}
	SetInitialized(false);
	return true;
}

BGSDefaultObjectManager::BGSDefaultObjectManager() : TESForm() {
	cFormType = 100;
	TESForm::SetFormID(49u, 1);
	TESForm::SetFormEditorID("DefaultObjectManager");
	BGSDefaultObjectManager::InitializeData();
}

BGSDefaultObjectManager::~BGSDefaultObjectManager() {}

void BGSDefaultObjectManager::InitializeData() {
	ourMemset(pObjectArray, 0, sizeof(pObjectArray));
}

// Writes the DOBJ record: one DATA chunk with the form ID of every default object (0 for an empty slot).
void BGSDefaultObjectManager::Save() {
	StartForm();
	uint32_t aiFormIDs[DO_COUNT];
	for (int32_t i = 0; i < DO_COUNT; ++i) {
		if (pObjectArray[i]) {
			aiFormIDs[i] = pObjectArray[i]->GetFormID();
		} else {
			aiFormIDs[i] = 0;
		}
	}
	AddChunkArray(DATA_ID, aiFormIDs, DO_COUNT);
	CloseForm();
}

void BGSDefaultObjectManager::InitItem() {
	if (!GetInitialized()) {
		for (int32_t i = 0; i < 34; ++i) {
			uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pObjectArray[i]));
			if (itempint) {
				TESFile* File = GetFile(-1);
				TESForm::AddCompileIndex(itempint, File);
				TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
				pObjectArray[i] = FormByNumericID;
				if (pObjectArray[i]) {
					int32_t FormType = pObjectArray[i]->GetFormType();
					if (FormType != kDefaultObjectData[i].eType) {
						pObjectArray[i] = 0;
					}
				}
				if (!pObjectArray[i]) {
					Warning("Unable to find valid '%s' default object for ID (%08X).", kDefaultObjectData[i].pName,
						itempint);
				}
			}
		}
		SetInitialized(1);
	}
}

BGSDefaultObjectManager* BGSDefaultObjectManager::Instance() {
	BGSDefaultObjectManager* v1;
	if (BGSDefaultObjectManager::pSingleton) {
		return BGSDefaultObjectManager::pSingleton;
	}
	void* v2 = ourMalloc(sizeof(BGSDefaultObjectManager));
	if (v2) {
		v1 = ::new (v2) BGSDefaultObjectManager;
	} else {
		v1 = nullptr;
	}
	BGSDefaultObjectManager::pSingleton = v1;
	return BGSDefaultObjectManager::pSingleton;
}

TESForm* BGSDefaultObjectManager::GetDefaultObject(DEFAULT_OBJECT aeObject) {
	BGSDefaultObjectManager* singleton = BGSDefaultObjectManager::Instance();
	if (singleton->TESForm::GetInitialized()) {
		return BGSDefaultObjectManager::Instance()->pObjectArray[(uint32_t)aeObject];
	} else {
		return nullptr;
	}
}

void BGSDefaultObjectManager::ReleaseInstance() {
	if (BGSDefaultObjectManager::pSingleton) {
		delete BGSDefaultObjectManager::pSingleton;
		BGSDefaultObjectManager::pSingleton = 0;
	}
}
