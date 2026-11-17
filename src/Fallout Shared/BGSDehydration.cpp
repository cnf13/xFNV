#include "Fallout Shared/BGSDehydration.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSReconstructFormsInAllFilesMap.h"

// Reads the stage's data (the dehydration amount and the sickness spell) and its editor ID.
bool BGSDehydrationStage::Load(TESFile* apFile) {
	uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
	if (ucFormType != DEHY_ID) {
		return false;
	}
	ClearData();
	LoadForm(apFile);
	SetInitialized(false);
	do {
		uint32_t uiChunk = apFile->GetTESChunk();
		if (uiChunk == NO_CHUNK) {
			break;
		}
		if (uiChunk == DATA_ID) {
			// the data is this form's own component, at this+0x18
			LoadData(apFile, static_cast<char*>(static_cast<void*>(GetData())), 8);
#if FNVD_64BIT
			// The file's record is 8 bytes, the sickness's 4-byte form ID at 4.
			TESFile::WidenFormIDRecords(GetData(), 1, 8, sizeof(BGSDehydrationStageData),
				{{4, offsetof(BGSDehydrationStageData, pSickness)}});
#endif
		} else if (uiChunk == EDID_ID) {
			// A 16-byte aligned buffer of the chunk's size, read up to 0x200.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0x200);
			SetFormEditorID(pBuffer);
		} else if (uiChunk == OBND_ID) {
			LoadObjectBound(apFile);
		}
		if (uiChunk == DATA_ID && apFile->QEndian()) {
			data.Endian();
		}
	} while (apFile->NextChunk());
	return true;
}

void BGSDehydrationStageData::Endian() {
	ENDIAN(iH20, false);
	ENDIAN(pSickness, false);
}

void BGSDehydrationStageData::Clear() {
	iH20 = 0;
	pSickness = nullptr;
}

BGSDehydrationStage::BGSDehydrationStage() : TESForm(), data() {
	SetFormType(0x76u);
	BGSDehydrationStage::InitializeData();
}

BGSDehydrationStage::~BGSDehydrationStage() {
	BGSDehydrationStage::ClearData();
}

void BGSDehydrationStage::InitializeData() {
	BGSDehydrationStageData* pData = GetData();
	pData->Clear();
}

void BGSDehydrationStage::ClearData() {
	BGSDehydrationStage::RemoveStage(this);
}

void BGSDehydrationStage::Save() {
	StartForm();
	SpellItem* apItem = GetSickness();
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
	data.pSickness = apItem;
	CloseForm();
}

void BGSDehydrationStage::InitItem() {
	if (!GetInitialized()) {
		// The loaded pointer field holds the sickness's form ID.
		uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetSickness()));
		if (itempInt) {
			TESFile* File = GetFile(-1);
			TESForm::AddCompileIndex(itempInt, File);
			TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempInt);
			data.pSickness = dynamic_cast<SpellItem*>(FormByNumericID);
		}
		SetInitialized(1);
		BGSDehydrationStage::AddStage(this);
	}
}

void BGSDehydrationStage::Copy(TESForm* apCopy) {
	BGSDehydrationStageData* v2;
	BGSDehydrationStageData* formList;
	BGSDehydrationStage* v5 = dynamic_cast<BGSDehydrationStage*>(apCopy);
	if (v5) {
		TESForm::CopyAllComponents(v5);
		formList = v5->GetData();
		v2 = GetData();
		ourMemcpy(v2, formList, sizeof(BGSDehydrationStageData));
	}
}

// True when the stages differ; a form that is not a stage counts as different.
bool BGSDehydrationStage::Compare(TESForm* apCompare) {
	BGSDehydrationStage* pStage = dynamic_cast<BGSDehydrationStage*>(apCompare);
	if (!pStage) {
		return true;
	}
	if (CompareAllComponents(pStage)) {
		return true;
	}
	// Fetches the other stage's data first.
	BGSDehydrationStageData* pOtherData = pStage->GetData();
	return std::memcmp(GetData(), pOtherData, sizeof(BGSDehydrationStageData)) != 0;
}

SpellItem* BGSDehydrationStage::GetSickness() {
	return data.pSickness;
}

BGSDehydrationStageData* BGSDehydrationStage::GetData() {
	return &data;
}

void BGSDehydrationStage::RemoveStage(BGSDehydrationStage* apStage) {
	if (BGSDehydrationStage::iNumStages) {
		bool binArray = 0;
		for (uint32_t i = 0; !binArray && i < BGSDehydrationStage::iNumStages; ++i) {
			binArray = BGSDehydrationStage::pStagesArray[i] == apStage;
		}
		if (binArray) {
			uint32_t inewStagesCount = BGSDehydrationStage::iNumStages - 1;
			BGSDehydrationStage** pnewStages = BGSDehydrationStage::AllocateStages(BGSDehydrationStage::iNumStages - 1);
			if (pnewStages) {
				uint32_t icurrentStage = 0;
				for (uint32_t i_2 = 0; i_2 < BGSDehydrationStage::iNumStages; ++i_2) {
					if (BGSDehydrationStage::pStagesArray[i_2] != apStage) {
						pnewStages[icurrentStage++] = BGSDehydrationStage::pStagesArray[i_2];
					}
				}
			}
			ourFree(BGSDehydrationStage::pStagesArray);
			BGSDehydrationStage::iNumStages = inewStagesCount;
			BGSDehydrationStage::pStagesArray = pnewStages;
		}
	}
}

BGSDehydrationStage* BGSDehydrationStage::FindStageForRadLevel(float afRads) {
	BGSDehydrationStage* pstage = 0;
	for (uint32_t i = 0; i < BGSDehydrationStage::iNumStages && !pstage; ++i) {
		BGSDehydrationStage* pcurrentStage = BGSDehydrationStage::GetStage(i);
		if (pcurrentStage) {
			if (afRads >= static_cast<double>(pcurrentStage->data.iH20)) {
				pstage = pcurrentStage;
			}
		}
	}
	return pstage;
}

int32_t BGSDehydrationStage::AddStage(BGSDehydrationStage* apNewStage) {
	uint8_t v4 = BGSDehydrationStage::iNumStages + 1;
	BGSDehydrationStage** Stages =
		BGSDehydrationStage::AllocateStages(static_cast<uint8_t>(BGSDehydrationStage::iNumStages + 1));
	int32_t v5 = 0;
	char v6 = 0;
	for (uint32_t i = 0; i < BGSDehydrationStage::iNumStages; ++i) {
		BGSDehydrationStageData* Data = &apNewStage->data;
		if (Data->iH20 > (&BGSDehydrationStage::pStagesArray[i]->data)->iH20 && !v6) {
			Stages[v5++] = apNewStage;
			v6 = 1;
		}
		Stages[v5++] = BGSDehydrationStage::pStagesArray[i];
	}
	if (!v6) {
		Stages[v5] = apNewStage;
	}
	ourFree(BGSDehydrationStage::pStagesArray);
	BGSDehydrationStage::iNumStages = v4;
	BGSDehydrationStage::pStagesArray = Stages;
	return v4;
}

BGSDehydrationStage* BGSDehydrationStage::GetStage(uint32_t auiIndex) {
	if (auiIndex >= BGSDehydrationStage::iNumStages) {
		return 0;
	} else {
		return BGSDehydrationStage::pStagesArray[auiIndex];
	}
}

// A zero-filled array of aiCount stage pointers, or null for none.
BGSDehydrationStage** BGSDehydrationStage::AllocateStages(uint32_t aiStagesCount) {
	if (!aiStagesCount) {
		return nullptr;
	}
	uint64_t uiSize = static_cast<uint64_t>(aiStagesCount) * sizeof(BGSDehydrationStage*);
	BGSDehydrationStage** ppStages =
		static_cast<BGSDehydrationStage**>(ourMalloc(uiSize > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(uiSize)));
	for (uint32_t i = 0; i < aiStagesCount; ++i) {
		ppStages[i] = nullptr;
	}
	return ppStages;
}

// The engine's memcpy wrapper: it forwards to the CRT memcpy and returns its result.
void* ourMemcpy(void* apDest, const void* apSrc, size_t aCount) {
	return rt::memcpy(apDest, apSrc, aCount);
}
