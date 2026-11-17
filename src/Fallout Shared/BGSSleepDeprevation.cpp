#include "Fallout Shared/BGSSleepDeprevation.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// Reads the DATA chunk (the stage's sleep deprivation and the form ID of its sickness spell) and the editor ID.
// GetTESForm's result is stored as a byte and compared zero-extended, hence the uint8_t.
bool BGSSleepDeprevationStage::Load(TESFile* apFile) {
	uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
	if (ucFormType != SLPD_ID) {
		return false;
	}
	ClearData();
	LoadForm(apFile);
	SetInitialized(false);
	for (;;) {
		CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
		if (eChunk == NO_CHUNK) {
			break;
		}

		if (eChunk == DATA_ID) {
			LoadData(apFile, GetData(), 8);
#if FNVD_64BIT
			// The file's record is 8 bytes, the sickness's 4-byte form ID at 4.
			TESFile::WidenFormIDRecords(GetData(), 1, 8, sizeof(BGSSleepDeprevationStageData),
				{{4, offsetof(BGSSleepDeprevationStageData, pSickness)}});
#endif
		} else if (eChunk == EDID_ID) {
			// A 16-byte aligned stack buffer of the chunk's size.
			uint32_t uiChunkSize = apFile->GetChunkSize();
			char* pEditorID = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
			// An empty chunk leaves the buffer unwritten (see above).
			pEditorID[0] = '\0';
			apFile->GetChunkData(pEditorID, 0x200);
			static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
		} else if (eChunk == OBND_ID) {
			static_cast<TESForm*>(this)->LoadObjectBound(apFile);
		}

		if (eChunk == DATA_ID && apFile->QEndian()) {
			data.Endian();
		}

		if (!apFile->NextChunk()) {
			break;
		}
	}
	return true;
}

void BGSSleepDeprevationStageData::Endian() {
	ENDIAN(iSleep, false);
	ENDIAN(pSickness, false);
}

void BGSSleepDeprevationStageData::Clear() {
	iSleep = 0;
	pSickness = nullptr;
}

BGSSleepDeprevationStage::BGSSleepDeprevationStage() {
	SetFormType(SLPD_ID);
	BGSSleepDeprevationStage::InitializeData();
}

BGSSleepDeprevationStage::~BGSSleepDeprevationStage() {
	BGSSleepDeprevationStage::ClearData();
}

void BGSSleepDeprevationStage::InitializeData() {
	BGSSleepDeprevationStageData* pData = GetData();
	pData->Clear();
}

void BGSSleepDeprevationStage::ClearData() {
	BGSSleepDeprevationStage::RemoveStage(this);
}

void BGSSleepDeprevationStage::Save() {
	StartForm();
	SpellItem* pSickness = GetSickness();
	SpellItem* pSicknessID;
	if (GetSickness()) {
		// The file stores the sickness's form ID in the pointer field's place.
		pSicknessID = reinterpret_cast<SpellItem*>(static_cast<uintptr_t>(GetSickness()->GetFormID()));
	} else {
		pSicknessID = 0;
	}
	data.pSickness = pSicknessID;
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
	__SaveData(&data, 8u);
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
	data.pSickness = pSickness;
	CloseForm();
}

void BGSSleepDeprevationStage::InitItem() {
	if (!GetInitialized()) {
		// Load keeps the sickness's form ID in the SpellItem* field until here, so it is read back as a uint32_t.
		uint32_t uiFormID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pSickness));
		if (uiFormID) {
			TESFile* pFile = GetFile(-1);
			TESForm::AddCompileIndex(uiFormID, pFile);
			TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
			data.pSickness = dynamic_cast<SpellItem*>(pForm);
		}
		SetInitialized(1);
		BGSSleepDeprevationStage::AddStage(this);
	}
}

void BGSSleepDeprevationStage::Copy(TESForm* apCopy) {
	BGSSleepDeprevationStageData* v2;
	BGSSleepDeprevationStageData* formList;
	BGSSleepDeprevationStage* v5 = dynamic_cast<BGSSleepDeprevationStage*>(apCopy);
	if (v5) {
		TESForm::CopyAllComponents(v5);
		formList = v5->GetData();
		v2 = GetData();
		ourMemcpy(v2, formList, sizeof(BGSSleepDeprevationStageData));
	}
}

bool BGSSleepDeprevationStage::Compare(TESForm* apCompare) {
	BGSSleepDeprevationStage* v6 = dynamic_cast<BGSSleepDeprevationStage*>(apCompare);
	if (!v6) {
		return (bool)1;
	}
	if (TESForm::CompareAllComponents(v6)) {
		return (bool)1;
	}
	char* formList = static_cast<char*>(static_cast<void*>(v6->GetData()));
	char* v3 = static_cast<char*>(static_cast<void*>(GetData()));
	return (std::memcmp(v3, formList, sizeof(BGSSleepDeprevationStageData)) != 0);
}

BGSSleepDeprevationStageData* BGSSleepDeprevationStage::GetData() {
	return &data;
}

SpellItem* BGSSleepDeprevationStage::GetSickness() {
	return data.pSickness;
}

void BGSSleepDeprevationStage::RemoveStage(BGSSleepDeprevationStage* apStage) {
	if (BGSSleepDeprevationStage::iNumStages) {
		bool v4 = 0;
		for (uint32_t i = 0; !v4 && i < BGSSleepDeprevationStage::iNumStages; ++i) {
			v4 = BGSSleepDeprevationStage::pStagesArray[i] == apStage;
		}
		if (v4) {
			uint32_t inewStagesCount = BGSSleepDeprevationStage::iNumStages - 1;
			BGSSleepDeprevationStage** Stages =
				BGSSleepDeprevationStage::AllocateStages(BGSSleepDeprevationStage::iNumStages - 1);
			if (Stages) {
				int32_t v2 = 0;
				for (uint32_t j = 0; j < BGSSleepDeprevationStage::iNumStages; ++j) {
					if (BGSSleepDeprevationStage::pStagesArray[j] != apStage) {
						Stages[v2++] = BGSSleepDeprevationStage::pStagesArray[j];
					}
				}
			}
			ourFree(BGSSleepDeprevationStage::pStagesArray);
			BGSSleepDeprevationStage::iNumStages = inewStagesCount;
			BGSSleepDeprevationStage::pStagesArray = Stages;
		}
	}
}

void BGSSleepDeprevationStage::AddStage(BGSSleepDeprevationStage* apNewStage) {
	uint8_t v3 = BGSSleepDeprevationStage::iNumStages + 1;
	BGSSleepDeprevationStage** Stages =
		BGSSleepDeprevationStage::AllocateStages(static_cast<uint8_t>(BGSSleepDeprevationStage::iNumStages + 1));
	int32_t v4 = 0;
	char v5 = 0;
	for (uint32_t i = 0; i < BGSSleepDeprevationStage::iNumStages; ++i) {
		BGSSleepDeprevationStageData* Data = &apNewStage->data;
		if (Data->iSleep > (&BGSSleepDeprevationStage::pStagesArray[i]->data)->iSleep && !v5) {
			Stages[v4++] = apNewStage;
			v5 = 1;
		}
		Stages[v4++] = BGSSleepDeprevationStage::pStagesArray[i];
	}
	if (!v5) {
		Stages[v4] = apNewStage;
	}
	ourFree(BGSSleepDeprevationStage::pStagesArray);
	BGSSleepDeprevationStage::iNumStages = v3;
	BGSSleepDeprevationStage::pStagesArray = Stages;
}

BGSSleepDeprevationStage** BGSSleepDeprevationStage::AllocateStages(uint32_t aiStagesCount) {
	if (!aiStagesCount) {
		return 0;
	}
	// new BGSSleepDeprevationStage*[n]: requests SIZE_MAX when the size overflows.
	BGSSleepDeprevationStage** v3 =
		static_cast<BGSSleepDeprevationStage**>(ourMalloc(aiStagesCount <= SIZE_MAX / sizeof(BGSSleepDeprevationStage*)
				? sizeof(BGSSleepDeprevationStage*) * aiStagesCount
				: SIZE_MAX));
	for (uint32_t i = 0; i < aiStagesCount; ++i) {
		v3[i] = 0;
	}
	return v3;
}

BGSSleepDeprevationStage* BGSSleepDeprevationStage::GetStage(uint32_t aiIndex) {
	if (aiIndex >= BGSSleepDeprevationStage::iNumStages) {
		return 0;
	} else {
		return pStagesArray[aiIndex];
	}
}

BGSSleepDeprevationStage* BGSSleepDeprevationStage::FindStageForRadLevel(float afRads) {
	BGSSleepDeprevationStage* pstage = 0;
	for (uint32_t i = 0; i < BGSSleepDeprevationStage::iNumStages && !pstage; ++i) {
		BGSSleepDeprevationStage* pcurrentStage = BGSSleepDeprevationStage::GetStage(i);
		if (pcurrentStage) {
			if (afRads >= static_cast<double>(pcurrentStage->data.iSleep)) {
				pstage = pcurrentStage;
			}
		}
	}
	return pstage;
}
