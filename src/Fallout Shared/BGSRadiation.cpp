#include "Fallout Shared/BGSRadiation.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

#include <stdint.h>

// Reads the DATA chunk (the stage's rads and the form ID of its sickness spell) and the editor ID. GetTESForm's result
// is stored as a byte and compared zero-extended, hence the uint8_t. When the chunk is empty (malformed data) the EDID
// buffer is initialised to "" (one zeroed byte, allocated only for an empty chunk).
bool BGSRadiationStage::Load(TESFile* apFile) {
	uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
	if (ucFormType != RADS_ID) {
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
			TESFile::WidenFormIDRecords(GetData(), 1, 8, sizeof(BGSRadiationStageData),
				{{4, offsetof(BGSRadiationStageData, pSickness)}});
#endif
		} else if (eChunk == EDID_ID) {
			// A 16-byte aligned buffer of the chunk's size.
			uint32_t uiChunkSize = apFile->GetChunkSize();
			char* pEditorID = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
			// Cleared first because an empty chunk leaves the buffer unwritten; the game does not clear it.
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

void BGSRadiationStageData::Endian() {
	ENDIAN(iRads, false);
	ENDIAN(pSickness, false);
}

void BGSRadiationStageData::Clear() {
	iRads = 0;
	pSickness = nullptr;
}

BGSRadiationStage::BGSRadiationStage() {
	SetFormType(RADS_ID);
	BGSRadiationStage::InitializeData();
}

BGSRadiationStage::~BGSRadiationStage() {
	BGSRadiationStage::ClearData();
}

void BGSRadiationStage::InitializeData() {
	BGSRadiationStageData* pData = GetData();
	pData->Clear();
}

void BGSRadiationStage::ClearData() {
	RemoveStage(this);
}

void BGSRadiationStage::Save() {
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

void BGSRadiationStage::InitItem() {
	if (!GetInitialized()) {
		// Load keeps the sickness's form ID in the SpellItem* field until here.
		uint32_t uiFormID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pSickness));
		if (uiFormID) {
			TESFile* pFile = GetFile(-1);
			TESForm::AddCompileIndex(uiFormID, pFile);
			TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
			data.pSickness = dynamic_cast<SpellItem*>(pForm);
		}
		SetInitialized(1);
		AddStage(this);
	}
}

void BGSRadiationStage::Copy(TESForm* apCopy) {
	BGSRadiationStageData* v2;
	BGSRadiationStageData* formList;
	BGSRadiationStage* v5 = dynamic_cast<BGSRadiationStage*>(apCopy);
	if (v5) {
		TESForm::CopyAllComponents(v5);
		formList = v5->GetData();
		v2 = GetData();
		ourMemcpy(v2, formList, sizeof(BGSRadiationStageData));
	}
}

bool BGSRadiationStage::Compare(TESForm* apCompare) {
	BGSRadiationStage* v6 = dynamic_cast<BGSRadiationStage*>(apCompare);
	if (!v6) {
		return (bool)1;
	}
	if (TESForm::CompareAllComponents(v6)) {
		return (bool)1;
	}
	char* formList = static_cast<char*>(static_cast<void*>(v6->GetData()));
	char* v3 = static_cast<char*>(static_cast<void*>(GetData()));
	return (std::memcmp(v3, formList, sizeof(BGSRadiationStageData)) != 0);
}

BGSRadiationStageData* BGSRadiationStage::GetData() {
	return &data;
}

SpellItem* BGSRadiationStage::GetSickness() {
	return data.pSickness;
}

// The new count is kept in a byte local for the allocation and the stored count.
void BGSRadiationStage::AddStage(BGSRadiationStage* apNewStage) {
	uint8_t v3 = BGSRadiationStage::iNumStages + 1;
	BGSRadiationStage** Stages = BGSRadiationStage::AllocateStages(v3);
	int32_t v4 = 0;
	bool v5 = 0;
	for (uint32_t i = 0; i < BGSRadiationStage::iNumStages; ++i) {
		if (apNewStage->data.iRads > BGSRadiationStage::pStagesArray[i]->data.iRads && !v5) {
			Stages[v4++] = apNewStage;
			v5 = 1;
		}
		Stages[v4++] = BGSRadiationStage::pStagesArray[i];
	}
	if (!v5) {
		Stages[v4++] = apNewStage;
	}
	ourFree(BGSRadiationStage::pStagesArray);
	BGSRadiationStage::iNumStages = v3;
	BGSRadiationStage::pStagesArray = Stages;
}

void BGSRadiationStage::RemoveStage(BGSRadiationStage* apStage) {
	if (BGSRadiationStage::iNumStages) {
		bool v4 = 0;
		for (uint32_t i = 0; !v4 && i < BGSRadiationStage::iNumStages; ++i) {
			v4 = BGSRadiationStage::pStagesArray[i] == apStage;
		}
		if (v4) {
			int32_t aiStagesCount = BGSRadiationStage::iNumStages - 1;
			BGSRadiationStage** Stages = BGSRadiationStage::AllocateStages(static_cast<uint32_t>(aiStagesCount));
			if (Stages) {
				int32_t v2 = 0;
				for (uint32_t j = 0; j < BGSRadiationStage::iNumStages; ++j) {
					if (BGSRadiationStage::pStagesArray[j] != apStage) {
						Stages[v2++] = BGSRadiationStage::pStagesArray[j];
					}
				}
			}
			ourFree(BGSRadiationStage::pStagesArray);
			BGSRadiationStage::iNumStages = aiStagesCount;
			BGSRadiationStage::pStagesArray = Stages;
		}
	}
}

BGSRadiationStage* BGSRadiationStage::GetStage(uint32_t aiIndex) {
	if (aiIndex >= BGSRadiationStage::iNumStages) {
		return nullptr;
	} else {
		return BGSRadiationStage::pStagesArray[aiIndex];
	}
}

BGSRadiationStage** BGSRadiationStage::AllocateStages(uint32_t aiStagesCount) {
	if (!aiStagesCount) {
		return 0;
	}
	// new BGSRadiationStage*[n]: requests SIZE_MAX when the size overflows.
	BGSRadiationStage** v3 =
		static_cast<BGSRadiationStage**>(ourMalloc(aiStagesCount <= SIZE_MAX / sizeof(BGSRadiationStage*)
				? sizeof(BGSRadiationStage*) * aiStagesCount
				: SIZE_MAX));
	for (uint32_t i = 0; i < aiStagesCount; ++i) {
		v3[i] = 0;
	}
	return v3;
}

BGSRadiationStage* BGSRadiationStage::FindStageForRadLevel(float afRads) {
	BGSRadiationStage* pstage = 0;
	for (uint32_t i = 0; i < BGSRadiationStage::iNumStages && !pstage; ++i) {
		BGSRadiationStage* pcurrentStage = GetStage(i);
		if (pcurrentStage) {
			if (afRads >= static_cast<double>(pcurrentStage->data.iRads)) {
				pstage = pcurrentStage;
			}
		}
	}
	return pstage;
}
