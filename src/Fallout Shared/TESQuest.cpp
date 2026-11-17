#include "Fallout Shared/TESQuest.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/AudioMarkerData.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/Date.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelList.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuestStage.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<INISettingCollection> bActivateAllQuestScripts("bActivateAllQuestScripts:General", false);

QUEST_DATA* QUEST_DATA::Init() {
	ucFlags = (uint8_t)0;
	ucPriority = (uint8_t)0;
	fQuestDelayTime = (float)0.0;
	return this;
}

void QUEST_DATA::Endian() {
	ENDIAN(fQuestDelayTime, false);
}

TESQuest::TESQuest() : pScriptLocals(nullptr) {
	data.Init();
	SetFormType(QUST_ID);
	TESQuest::InitializeData();
}

TESQuest::~TESQuest() {
	TESQuest::ClearData();
}

void TESQuest::InitializeData() {
	SetEnabled(1);
	cCurrentStage = (uint8_t)0;
}

// The exe's `delete pScriptLocals' tests the copy for null again before the deleting destructor (flag 1).
void TESQuest::ClearData() {
	GetConditions()->ClearAllConditionItems();
	ClearStageList();
	ClearObjectiveList();
	if (pScriptLocals) {
		ScriptLocals* pLocals = pScriptLocals;
		if (pLocals)
			delete pLocals;
		pScriptLocals = nullptr;
	}
}

// The exe reads the EDID alloca buffer uninitialised when the chunk is empty (malformed data); src initialises it
// to "" (one zeroed byte, allocated only for an empty chunk).
// QUST. The stage item's own chunks (SCHR, SCDA, SCRO, SCVR, SLSD) are read by TESQuestStageItem::Load, so those
// cases go back to the chunk read instead of calling NextChunk.
bool TESQuest::Load(TESFile* apFile) {
	uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
	if (cFormType != QUST_ID) {
		return false;
	}
	TESQuestStage* pStage = nullptr;
	TESQuestStageItem* pStageItem = nullptr;
	TESQuestTarget* pTarget = nullptr;
	BGSQuestObjective* pObjective = nullptr;
	uint8_t ucLogIndex = 0;
	LoadForm(apFile);
	SetInitialized(false);
	for (;;) {
		uint32_t uiChunk = apFile->GetTESChunk();
		if (uiChunk == NO_CHUNK) {
			break;
		}
		bool bChunkRead = false;
		switch (uiChunk) {
		case EDID_ID: {
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			uint32_t uiChunkSize = apFile->GetChunkSize();
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
			// Not in the exe: an empty chunk leaves the buffer unwritten (see above).
			pBuffer[0] = '\0';
			apFile->GetChunkData(pBuffer, 0x200);
			SetFormEditorID(pBuffer);
			break;
		}
		case OBND_ID:
			LoadObjectBound(apFile);
			break;
		case FULL_ID:
			TESFullName::LoadFullNameChunk(this, apFile);
			break;
		case DATA_ID:
			// The DATA chunk is a raw 8-byte buffer.
			LoadData(apFile, static_cast<char*>(static_cast<void*>(&data)), 8);
			break;
		case SCRI_ID: {
			uint32_t uiScriptID = 0;
			apFile->GetChunkData(uiScriptID);
			// The chunk holds the script's form ID, which the exe stores unresolved.
			TESScriptableForm::SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptID)));
			TESScriptableForm::InitItem(this);
			break;
		}
		case ICON_ID:
			TESTexture::LoadTextureChunk(this, apFile);
			break;
		case QOBJ_ID: {
			void* pMemory = ourMalloc(sizeof(BGSQuestObjective));
			pObjective = pMemory ? ::new (pMemory) BGSQuestObjective : nullptr;
			pObjective->Load(apFile);
			GetObjectiveList()->AddTail(pObjective);
			pObjective->SetOwner(this);
			break;
		}
		case QSTA_ID: {
			pStage = nullptr;
			pStageItem = nullptr;
			void* pMemory = ourMalloc(sizeof(TESQuestTarget));
			pTarget = pMemory ? ::new (pMemory) TESQuestTarget : nullptr;
			pTarget->Load(apFile);
			if (!pObjective) {
				void* pObjectiveMemory = ourMalloc(sizeof(BGSQuestObjective));
				pObjective = pObjectiveMemory ? ::new (pObjectiveMemory) BGSQuestObjective : nullptr;
				GetObjectiveList()->AddTail(pObjective);
				pObjective->SetOwner(this);
			}
			pObjective->GetTargetList()->AddTail(pTarget);
			break;
		}
		case INDX_ID: {
			void* pMemory = ourMalloc(sizeof(TESQuestStage));
			pStage = pMemory ? ::new (pMemory) TESQuestStage : nullptr;
			pStage->Load(apFile);
			GetStageList()->AddTail(pStage);
			break;
		}
		case QSDT_ID:
			if (pStage) {
				void* pMemory = ourMalloc(sizeof(TESQuestStageItem));
				pStageItem = pMemory ? ::new (pMemory) TESQuestStageItem : nullptr;
				pStageItem->Load(apFile);
				uint8_t ucIndex = ucLogIndex;
				pStageItem->SetLogEntryID(ucIndex);
				ucLogIndex = static_cast<uint8_t>(ucLogIndex + 1);
				pStage->GetStageItemList()->AddTail(pStageItem);
			}
			bChunkRead = true;
			break;
		case CNAM_ID:
			if (pStageItem)
				pStageItem->SetHasLogEntry(true);
			break;
		case SCDA_ID:
		case SCHR_ID:
		case SCRO_ID:
		case SCVR_ID:
		case SLSD_ID:
			if (pStageItem) {
				pStageItem->Load(apFile);
				bChunkRead = true;
			}
			break;
		case CIDA_ID:
			if (pStageItem) {
				pStageItem->GetConditionList()->Load(apFile);
			} else if (pTarget) {
				pTarget->GetConditionList()->Load(apFile);
			} else {
				GetConditions()->Load(apFile);
			}
			break;
		default:
			break;
		}
		if (bChunkRead)
			continue;
		if (!apFile->NextChunk())
			break;
	}
	if (apFile->QEndian())
		data.Endian();
	return true;
}

void TESQuest::Save() {
	StartForm();
	TESScriptableForm::Save();
	TESFullName::Save();
	TESTexture::Save(ICON_ID);
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
	__SaveData(&data, 8u);
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
	// The exe tests this address (&objConditions), which is never null.
	(&objConditions)->Save();
	for (BSSimpleList<TESQuestStage*>* pList = &m_listStages; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
	}
	for (BSSimpleList<BGSQuestObjective*>* pObjectiveList = GetObjectiveList();
		pObjectiveList && !pObjectiveList->IsEmpty(); pObjectiveList = pObjectiveList->m_pkNext) {
		pObjectiveList->m_item->Save();
	}
	CloseForm();
}

uint16_t TESQuest::GetSaveSize(uint32_t aiFlags) {
	uint16_t isize = TESForm::GetSaveSize(aiFlags);
	uint16_t v8 = isize;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		isize += 6;
	}
	if ((aiFlags & 2) != 0) {
		++isize;
	}
	if ((aiFlags & CHANGE_QUEST_STAGES) != 0) {
		BSSimpleList<TESQuestStage*>* pstages = &m_listStages;
		++isize;
		while (pstages) {
			TESQuestStage* pstage = pstages->m_item;
			if (pstage) {
				isize += 2 + 5 * (&pstage->listStageItems)->ItemsInList() + 1;
			}
			pstages = pstages->m_pkNext;
		}
	}
	if ((aiFlags & CHANGE_QUEST_SCRIPT) != 0) {
		isize += pScriptLocals->GetSaveSize();
	}
	if (bDebugSaveBuffer.Bool()) {
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
			InitializationFlow("GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				isize - v8, pheader->iFormID, pform->GetFormEditorID(), pheader->iFlags, 3920,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp");
		} else {
			InitializationFlow("GetSaveSize(): %-5i ending at line %i in file %s", isize - v8, 3920,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp");
		}
	}
	return isize;
}

// The exe's cleanup-only C++ EH frame is left out.
void TESQuest::SaveGame(uint32_t aiFlags) {
	uint8_t cindex2;
	uint8_t ccount2;
	uint8_t cindex;
	uint8_t bdone;
	uint8_t ccount;
	uint8_t cflags;
	uint32_t iblockHeader;
	TESForm::SaveGame(aiFlags);
	uint16_t iblockSize = 0;
	uint16_t* pblockSize = 0;
	uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
	if (bDebugSaveBuffer.Bool()) {
		itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		iblockHeader = 0x424C4F4B;
		pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
		pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
	}
	if ((aiFlags & 2) != 0) {
		cflags = (&data)->ucFlags;
		SaveGameDataOLD(&cflags, 1);
	}
	if ((aiFlags & CHANGE_QUEST_STAGES) != 0) {
		ccount = 0;
		uint8_t* pcount = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		SaveGameDataOLD(&ccount, 1);
		for (TESQuestStage* pstage : *&m_listStages) {
			if (pstage) {
				cindex = pstage->m_Data.ucIndex;
				bdone = pstage->m_Data.bIsDone;
				SaveGameDataOLD(&cindex, 1);
				SaveGameDataOLD(&bdone, 1);
				ccount2 = 0;
				uint8_t* pcount2 = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
				SaveGameDataOLD(&ccount2, 1);
				for (TESQuestStageItem* pstageitem : *&pstage->listStageItems) {
					if (pstageitem) {
						cindex2 = pstageitem->ucIndex;
						void* pdate = (void*)pstageitem->m_pLogDate;
						SaveGameDataOLD(&cindex2, 1);
						if (pdate) {
							pSaveLoadGameOLD->SaveGameDataOLD(pdate, 4u);
						} else {
							FNVD_CONSTRUCTED_LOCAL(Date, date, &Date::Destruct);
							pSaveLoadGameOLD->SaveGameDataOLD(&date, 4u);
						}
						++ccount2;
					}
				}
				*pcount2 = ccount2;
				++ccount;
			}
		}
		*pcount = ccount;
	}
	if ((aiFlags & CHANGE_QUEST_SCRIPT) != 0) {
		pScriptLocals->SaveGame();
	}
	if (bDebugSaveBuffer.Bool()) {
		uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
			uint32_t iFlags = pheader->iFlags;
			const char* v2 = pform->GetFormEditorID();
			InitializationFlow("SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v2, iFlags, 4003,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp");
		} else {
			InitializationFlow("SaveGame(): %-5i ending at line %i in file %s",
				static_cast<int32_t>(BufferPointer - itempStartPosition), 4003,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp");
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		// The block's size: the buffer's advance past the size word, compared as unsigned addresses.
		char* pBufferNow = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
		char* pBlockStart = static_cast<char*>(static_cast<void*>(pblockSize));
		if (pBufferNow > pBlockStart + 0xFFFF) {
			Warning("SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp", 4003);
		}
		*pblockSize = static_cast<uint16_t>(pBufferNow - pBlockStart);
	}
}

void TESQuest::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	uint32_t FormHealth_0;
	QUEST_OBJECTIVE_STATE estate;
	bool bhasDate;
	uint8_t cindex2;
	uint8_t cindex;
	uint8_t bdone;
	TESForm::SaveGame(apSaveGameBuffer);
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(2)) {
		apSaveGameBuffer->SaveDataEndian(&data, 1u, 0);
	}
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(4)) {
		apSaveGameBuffer->SaveDataEndian(&data.fQuestDelayTime, 4u, 0);
	}
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_STAGES)) {
		uint32_t isaveGameCount = 0;
		uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
		for (TESQuestStage* pstage : m_listStages) {
			if (pstage) {
				cindex = pstage->m_Data.ucIndex;
				bdone = pstage->m_Data.bIsDone;
				apSaveGameBuffer->SaveDataEndian(&cindex, 1u, 0);
				apSaveGameBuffer->SaveDataEndian(&bdone, 1u, 0);
				uint32_t isaveGameCount_2 = 0;
				uint32_t isaveGameBufferPos_2 = apSaveGameBuffer->StartVariableSizedValue();
				for (BSSimpleList<TESQuestStageItem*>* plist_2 = &pstage->listStageItems; plist_2;
					plist_2 = plist_2->m_pkNext) {
					TESQuestStageItem* pstageItem = plist_2->m_item;
					if (pstageItem) {
						cindex2 = pstageItem->ucIndex;
						Date* pdate = pstageItem->m_pLogDate;
						bhasDate = pdate != 0;
						apSaveGameBuffer->SaveDataEndian(&cindex2, 1u, 0);
						apSaveGameBuffer->SaveDataEndian(&bhasDate, 1u, 0);
						if (bhasDate) {
							apSaveGameBuffer->SaveDataEndian(pdate, 4u, 0);
						}
						++isaveGameCount_2;
					}
				}
				apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_2, isaveGameBufferPos_2);
				++isaveGameCount;
			}
		}
		apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
	}
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_SCRIPT)) {
		pScriptLocals->SaveGame(apSaveGameBuffer);
	}
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_OBJECTIVES)) {
		uint32_t icount = 0;
		uint32_t isaveGameBufferPos_3 = apSaveGameBuffer->StartVariableSizedValue();
		for (BGSQuestObjective* pobjective : m_listObjectives) {
			if (pobjective) {
				estate = pobjective->eState;
				if (estate) {
					FormHealth_0 = pobjective->iIndex;
					apSaveGameBuffer->SaveDataEndian(&FormHealth_0, 4u, 0);
					apSaveGameBuffer->SaveDataEndian(&estate, 4u, 0);
					++icount;
				}
			}
		}
		apSaveGameBuffer->SaveVariableSizedValue(icount, isaveGameBufferPos_3);
	}
}

// The quest's old-format load: the form's data, the block header, then by the change flags the DATA flags (2), the
// stages' done flags and the log dates of their items (0x80000000) and the script's locals (0x40000000); the block
// size is checked last. The exe's cleanup-only unwind frame (the Date local's empty destructor, the allocated date's
// and locals' frees when a constructor throws) is left out; the Date local is destroyed where the exe does it.
void TESQuest::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	TESForm::LoadGame(aiFlags, aiCurrentFlags);
	uint16_t sBlockSize = 0;
	char* pBlockStart = nullptr;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		uint32_t iBlockID;
		pSaveLoadGameOLD->LoadGameDataOLD(&iBlockID, 4);
		if (iBlockID != 0x424C4F4B) {
			LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
			if (pHeader) {
				TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
				uint32_t iFlags = pHeader->iFlags;
				uint8_t cVersion = pHeader->cVersion;
				const char* pEditorID = pForm->GetFormEditorID();
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
						"Currently loading form is %08X %s with version %i and flags %08X",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp", 4015, pHeader->iFormID,
					pEditorID, cVersion, iFlags);
			} else {
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
						"Current version is %i",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp", 4015,
					pSaveLoadGameOLD->GetCurrentVersion());
			}
		}
		pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
		pSaveLoadGameOLD->LoadGameDataOLD(&sBlockSize, 2);
	}
	// The saved bytes are read into char locals (the reader's buffer type) and used zero-extended, as the exe does.
	if (aiFlags & 2) {
		char cFlags;
		TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cFlags), 1);
		GetData()->ucFlags = static_cast<uint8_t>(cFlags);
	}
	if (aiFlags & 0x80000000) {
		char cStageCount;
		TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cStageCount), 1);
		for (int32_t i = 0; i < static_cast<uint8_t>(cStageCount); ++i) {
			char cStageIndex;
			TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cStageIndex), 1);
			char cDone;
			TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cDone), 1);
			TESQuestStage* pStage = GetStage(static_cast<uint8_t>(cStageIndex));
			if (pStage) {
				pStage->SetDone(static_cast<uint8_t>(cDone));
			} else {
				Warning("SAVELOAD: Could not find stage %i in quest %s during load.",
					static_cast<uint32_t>(static_cast<uint8_t>(cStageIndex)), GetEditorName());
			}
			char cItemCount;
			TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cItemCount), 1);
			for (int32_t j = 0; j < static_cast<uint8_t>(cItemCount); ++j) {
				Date kLogDate;
				kLogDate.~Date();
				::new (&kLogDate) Date;
				char cItemIndex;
				TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cItemIndex), 1);
				pSaveLoadGameOLD->LoadGameDataOLD(&kLogDate, 4);
				if (pStage) {
					TESQuestStageItem* pItem = pStage->GetStageItem(static_cast<uint8_t>(cItemIndex));
					if (pItem) {
						if (kLogDate.GetDate() || kLogDate.GetYear()) {
							Date* pLogDate = pItem->GetLogDate();
							if (pLogDate) {
								*pLogDate = kLogDate;
							} else {
								void* pMemory = ourMalloc(sizeof(Date));
								Date* pNewDate = pMemory ? ::new (pMemory) Date(kLogDate) : nullptr;
								pItem->SetLogDate(pNewDate);
							}
						} else {
							Date* pLogDate = pItem->GetLogDate();
							if (pLogDate) {
								delete pLogDate;
								pItem->SetLogDate(nullptr);
							}
						}
					}
				}
			}
		}
		cCurrentStage = 0;
		FindCurrentStage();
	}
	if (aiFlags & 0x40000000) {
		if (!pScriptLocals) {
			// No locals to load into: the saved ones are read into a scratch set and dropped; the exe loads through
			// the result before testing it for null.
			void* pMemory = ourMalloc(sizeof(ScriptLocals));
			ScriptLocals* pScratch = pMemory ? ::new (pMemory) ScriptLocals : nullptr;
			pScratch->LoadGame();
			if (pScratch) {
				delete pScratch;
			}
			pSaveLoadGameOLD->QueueRemoveChange(0x40000000);
		} else {
			pScriptLocals->LoadGame();
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		char* pBlockEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
		LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
		if (pHeader) {
			TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
			if (pBlockEnd > pBlockStart + sBlockSize) {
				uint32_t iFlags = pHeader->iFlags;
				uint8_t cVersion = pHeader->cVersion;
				const char* pEditorID = pForm->GetFormEditorID();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading "
						"form is %08X %s with version %i and flags %08X",
					static_cast<int32_t>(pBlockEnd - (pBlockStart + sBlockSize)),
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp", 4112, pHeader->iFormID,
					pEditorID, cVersion, iFlags);
			} else if (pBlockEnd < pBlockStart + sBlockSize) {
				uint32_t iFlags = pHeader->iFlags;
				uint8_t cVersion = pHeader->cVersion;
				const char* pEditorID = pForm->GetFormEditorID();
				Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
						"form is %08X %s with version %i and flags %08X",
					static_cast<int32_t>((pBlockStart + sBlockSize) - pBlockEnd),
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp", 4112, pHeader->iFormID,
					pEditorID, cVersion, iFlags);
			}
		} else if (pBlockEnd > pBlockStart + sBlockSize) {
			uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
			Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
				static_cast<int32_t>(pBlockEnd - (pBlockStart + sBlockSize)),
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp", 4112, cVersion);
		} else if (pBlockEnd < pBlockStart + sBlockSize) {
			uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
			Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
				static_cast<int32_t>((pBlockStart + sBlockSize) - pBlockEnd),
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp", 4112, cVersion);
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void TESQuest::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	uint32_t iindex;
	QUEST_OBJECTIVE_STATE estate;
	bool bhasDate;
	uint8_t cindex2;
	uint8_t cindex;
	uint8_t bdone;
	TESForm::LoadGame(apLoadGameBuffer);
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(2)) {
		apLoadGameBuffer->LoadData(&data, 1u);
	}
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(4)) {
		apLoadGameBuffer->LoadData(&data.fQuestDelayTime, 4u);
	}
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_STAGES)) {
		uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
		for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
			cindex = 0;
			bdone = 0;
			apLoadGameBuffer->LoadData(&cindex, 1u);
			apLoadGameBuffer->LoadData(&bdone, 1u);
			TESQuestStage* pstage = GetStage(cindex);
			if (pstage) {
				pstage->SetDone(bdone);
			} else {
				const char* v2 = GetEditorName();
				SaveGameWarning("SAVELOAD: Could not find stage %i in quest %s during load.", cindex, v2);
			}
			uint32_t isaveGameCount_2 = apLoadGameBuffer->LoadVariableSizedValue();
			for (uint32_t iloop_2 = 0; iloop_2 < isaveGameCount_2; ++iloop_2) {
				cindex2 = 0;
				bhasDate = 0;
				FNVD_CONSTRUCTED_LOCAL(Date, date, &Date::Destruct);
				apLoadGameBuffer->LoadData(&cindex2, 1u);
				apLoadGameBuffer->LoadData(&bhasDate, 1u);
				if (bhasDate) {
					apLoadGameBuffer->LoadData(&date, 4u);
				}
				if (bhasDate) {
					if (pstage) {
						TESQuestStageItem* pstageItem = pstage->GetStageItem(cindex2);
						if (pstageItem) {
							Date* pDate;
							void* pMemory = ourMalloc(sizeof(Date));
							if (pMemory) {
								pDate = ::new (pMemory) Date(date);
							} else {
								pDate = nullptr;
							}
							pstageItem->SetLogDate(pDate);
						}
					}
				}
			}
		}
		cCurrentStage = 0;
		FindCurrentStage();
	}
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_SCRIPT)) {
		if (pScriptLocals) {
			pScriptLocals->LoadGame(apLoadGameBuffer);
		} else {
			FNVD_CONSTRUCTED_LOCAL(ScriptLocals, tempLocals, &ScriptLocals::Destruct);
			tempLocals.LoadGame(apLoadGameBuffer);
		}
	}
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_OBJECTIVES)) {
		uint32_t isaveGameCount_3 = apLoadGameBuffer->LoadVariableSizedValue();
		for (uint32_t iloop_3 = 0; iloop_3 < isaveGameCount_3; ++iloop_3) {
			iindex = 0;
			estate = QOS_DORMANT;
			apLoadGameBuffer->LoadData(&iindex, 4u);
			apLoadGameBuffer->LoadData(&estate, 4u);
			BGSQuestObjective* pobjective = GetObjective(iindex);
			if (pobjective) {
				pobjective->eState = estate;
			}
		}
	}
}

void TESQuest::Revert(uint32_t aiFlags) {
	if ((aiFlags & CHANGE_QUEST_STAGES) != 0) {
		for (TESQuestStage* pstage : *&m_listStages) {
			if (pstage) {
				pstage->SetDone(0);
				for (TESQuestStageItem* m_item : *&pstage->listStageItems) {
					void* LogDate;
					if (m_item) {
						LogDate = m_item->m_pLogDate;
					} else {
						LogDate = 0;
					}
					if (LogDate) {
						m_item->SetLogDate(0);
						delete static_cast<Date*>(LogDate);
					}
				}
			}
		}
		cCurrentStage = 0;
	}
	if ((aiFlags & CHANGE_QUEST_SCRIPT) != 0) {
		if (pScriptLocals) {
			pScriptLocals->Revert_0();
		}
	}
}

void TESQuest::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
	if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_QUEST_STAGES)) {
		for (TESQuestStage* pstage : m_listStages) {
			if (pstage) {
				pstage->SetDone(0);
				for (BSSimpleList<TESQuestStageItem*>* plist_2 = &pstage->listStageItems; plist_2;
					plist_2 = plist_2->m_pkNext) {
					TESQuestStageItem* pstageItem = plist_2->m_item;
					if (pstageItem) {
						if (pstageItem->m_pLogDate) {
							Date* pdate = pstageItem->m_pLogDate;
							if (pdate) {
								delete pdate;
							}
							pstageItem->m_pLogDate = 0;
						}
					}
				}
			}
		}
		cCurrentStage = 0;
	}
	if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_QUEST_SCRIPT)) {
		if (pScriptLocals) {
			pScriptLocals->Revert(apLoadGameBuffer);
		}
	}
	if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_QUEST_OBJECTIVES)) {
		for (BGSQuestObjective* pobjective : m_listObjectives) {
			if (pobjective) {
				pobjective->eState = QOS_DORMANT;
			}
		}
	}
}

void TESQuest::CheckSaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	BGSChangeFlags v4;
	BGSChangeFlags* v2 = &(v4 = apSaveGameBuffer->GetChangeFlags());
	if (v2->CheckFlags(0x40000000u) && (!pScriptLocals)) {
		apSaveGameBuffer->RemoveChange(1073741824);
	}
}

void TESQuest::InitItem() {
	if (!GetInitialized()) {
		TESScriptableForm::InitItem(this);
		if (pFormScript) {
			pScriptLocals = pFormScript->CloneLocals();
			pFormScript->pOwnerQuest = this;
			if (pScriptLocals) {
				ForceChange(CHANGE_QUEST_SCRIPT);
			}
			if (bActivateAllQuestScripts.Bool()) {
				SetEnabled(1);
			}
		}
		// The exe tests this address (&objConditions), which is never null.
		(&objConditions)->InitItem(this);
		for (BSSimpleList<TESQuestStage*>* pList = &m_listStages; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
			TESQuestStage** v1 = &pList->m_item;
			(*v1)->InitItem(this);
		}
		for (BSSimpleList<BGSQuestObjective*>* pObjectiveList = GetObjectiveList();
			pObjectiveList && !pObjectiveList->IsEmpty(); pObjectiveList = pObjectiveList->m_pkNext) {
			BGSQuestObjective** v2 = &pObjectiveList->m_item;
			(*v2)->InitItem(this);
		}
		bool Enabled = GetEnabled();
		SetStartsEnabled(Enabled);
		SetInitialized(1);
	}
}

// The editor ID is copied to an _alloca buffer, compared with the current one and stored when it changed.
bool TESQuest::SetFormEditorID(const char* apEditorID) {
	uint32_t uiSize = BSstrlen(apEditorID) + 1;
	// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the string's size.
	char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
	BSstrcpy(pBuffer, uiSize, apEditorID);
	if (!strEditorID.StrCmp(pBuffer, true))
		return false;
	strEditorID.Set(pBuffer, 0);
	return true;
}

const char* TESQuest::GetEditorName() {
	if (strEditorID.pString) {
		return strEditorID.pString;
	} else {
		return "";
	}
}

void TESQuest::SetAllObjectivesComplete(int32_t aiState) {
	for (BSSimpleList<BGSQuestObjective*>* ecx0 = GetObjectiveList(); ecx0 && ecx0->m_item; ecx0 = ecx0->m_pkNext) {
		BGSQuestObjective** v2 = &ecx0->m_item;
		QUEST_OBJECTIVE_STATE v4 = static_cast<QUEST_OBJECTIVE_STATE>((*v2)->GetDisplayed() + 2);
		BGSQuestObjective** v3 = &ecx0->m_item;
		(*v3)->SetState(v4);
	}
}

// The exe calls BSSimpleList's IsEmpty (0x8256D0), GetItem (0x6815C0) and GetNext (0x726070) out of line.
BGSQuestObjective* TESQuest::GetObjective(uint32_t aiIndex) {
	for (BSSimpleList<BGSQuestObjective*>* pNode = GetObjectiveList(); pNode && !pNode->IsEmpty();
		pNode = pNode->GetNext()) {
		BGSQuestObjective* pObjective = pNode->GetItem();
		if (pObjective && pObjective->GetIndex() == aiIndex)
			return pObjective;
	}
	return nullptr;
}

bool TESQuest::SetStageDone(uint8_t aucIndex) {
	BSSimpleList<TESQuestStage*>* pList;
	for (pList = &m_listStages;; pList = pList->m_pkNext) {
		if (!pList || pList->IsEmpty()) {
			return 0;
		}
		if (pList->m_item->m_Data.ucIndex == aucIndex) {
			break;
		}
	}
	if (!GetCompleted() || GetAllowRepeatStages()) {
		SetEnabled(1);
	}
	if (aucIndex > static_cast<int32_t>(cCurrentStage)) {
		cCurrentStage = aucIndex;
	}
	pList->m_item->SetDone(this, 0);
	AddChange(CHANGE_QUEST_STAGES);
	return 1;
}

// The change flag 2 (the quest's flags) has no name in the PDB or IDA.
void TESQuest::SetEnabled(uint8_t abEnabled) {
	if (abEnabled)
		GetData()->ucFlags |= 1;
	else
		GetData()->ucFlags &= ~1;
	AddChange(2);
}

TESCondition* TESQuest::GetConditions() {
	return &objConditions;
}

bool TESQuest::GetEnabled() {
	return ((GetData()->ucFlags & 1) != 0);
}

// The exe fetches the script through the shared one-field getter twice.
void TESQuest::RunScript() {
	if (TESScriptableForm::GetFormScript())
		TESScriptableForm::GetFormScript()->Run(nullptr, pScriptLocals, nullptr, false);
}

// The exe calls BSSimpleList's GetItem (0x6815C0) and GetNext (0x726070) out of line.
TESQuestStage* TESQuest::GetStage(uint8_t acIndex) {
	for (BSSimpleList<TESQuestStage*>* pNode = GetStageList(); pNode; pNode = pNode->GetNext()) {
		TESQuestStage* pStage = pNode->GetItem();
		if (pStage && pStage->GetIndex() == acIndex)
			return pStage;
	}
	return nullptr;
}

uint8_t TESQuest::GetStageIndexForStageItem(TESQuestStageItem* apStageItem) {
	for (BSSimpleList<TESQuestStage*>* pstages = &m_listStages; pstages && !pstages->IsEmpty();
		pstages = pstages->m_pkNext) {
		TESQuestStage* pstage = pstages->m_item;
		if (apStageItem == pstage->GetStageItem(apStageItem->ucIndex)) {
			return pstage->m_Data.ucIndex;
		}
	}
	return 0;
}

bool TESQuest::ValidateTargetList(BSSimpleList<TESQuestTarget*>* apListToValidate,
	BSSimpleList<BGSQuestObjective*>* apPlayerObjectiveList) {
	BSSimpleList<TESQuestTarget*>* pSuppliedList = apListToValidate;
	BSSimpleList<BGSQuestObjective*>* pObjectiveList = GetObjectiveList();
	if (apListToValidate && pObjectiveList) {
		while (pObjectiveList) {
			if (pObjectiveList->m_item &&
				(pObjectiveList->m_item->eState == QOS_DISPLAYED &&
					apPlayerObjectiveList->IsInList(pObjectiveList->m_item))) {
				for (TESQuestTarget* ptarget : *&pObjectiveList->m_item->ListTargets) {
					if (ptarget) {
						TESObjectREFR* Reference = ptarget->GetReference(0);
						if ((&ptarget->objConditions)->IsTrue(Reference, 0)) {
							if (!pSuppliedList || (!pSuppliedList->m_item || pSuppliedList->m_item != ptarget)) {
								return 0;
							}
							pSuppliedList = pSuppliedList->m_pkNext;
						}
					}
				}
			}
			pObjectiveList = pObjectiveList->m_pkNext;
		}
	}
	return !pSuppliedList || pSuppliedList->IsEmpty();
}

void TESQuest::UpdateCurrentTargetList(BSSimpleList<TESQuestTarget*>* apListToFill,
	BSSimpleList<BGSQuestObjective*>* apPlayerObjectiveList) {
	if (apListToFill) {
		apListToFill->RemoveAll();
		if (!GetCompleted()) {
			for (BSSimpleList<BGSQuestObjective*>* pObjectiveList = GetObjectiveList();
				pObjectiveList && pObjectiveList->m_item; pObjectiveList = pObjectiveList->m_pkNext) {
				if (apPlayerObjectiveList->IsInList(pObjectiveList->m_item)) {
					BGSQuestObjective** v3 = &pObjectiveList->m_item;
					(*v3)->UpdateCurrentTargetList(apListToFill);
				}
			}
		}
	}
}

uint32_t TESQuest::GetPriority() {
	return (uint32_t)GetData()->ucPriority;
}

void TESQuest::SetStartsEnabled(bool abVal) {
	if (abVal) {
		GetData()->ucFlags |= 0x10;
	} else {
		GetData()->ucFlags &= ~0x10;
	}
}

void TESQuest::FindCurrentStage() {
	BSSimpleList<TESQuestStage*>* pList = &m_listStages;
	uint8_t maxIndex = 0;
	while (pList && !pList->IsEmpty()) {
		TESQuestStage* pStage = pList->m_item;
		if (pStage->m_Data.bIsDone && pStage->m_Data.ucIndex > static_cast<int32_t>(maxIndex)) {
			maxIndex = pStage->m_Data.ucIndex;
		}
		pList = pList->m_pkNext;
	}
	if (maxIndex > static_cast<int32_t>(cCurrentStage)) {
		cCurrentStage = maxIndex;
	}
}

void TESQuest::ClearStageList() {
	for (BSSimpleList<TESQuestStage*>* pList = &m_listStages; pList && !pList->IsEmpty(); pList->RemoveHead()) {
		TESQuestStage* pstage = pList->m_item;
		if (pstage) {
			delete pstage;
		}
	}
}

void TESQuest::ClearObjectiveList() {
	for (BSSimpleList<BGSQuestObjective*>* pList = GetObjectiveList(); pList && !pList->IsEmpty();
		pList->RemoveHead()) {
		BGSQuestObjective* m_item = pList->m_item;
		if (m_item) {
			delete m_item;
		}
	}
}

bool TESQuest::GetFailed() {
	return ((GetData()->ucFlags & 64) != 0);
}

bool TESQuest::GetCompleted() {
	return ((GetData()->ucFlags & 2) != 0);
}

uint32_t TESQuest::GetCurrentStage() {
	return (uint32_t)cCurrentStage;
}

int32_t TESQuest::GetHasShownQuestUpdate() {
	return (int32_t)((uint8_t)((GetData()->ucFlags & 32) != 0));
}

void TESQuest::SetDisplayedQuestStarted(char a2) {
	QUEST_DATA* flags;
	QUEST_DATA* v3;
	uint8_t v4;
	uint8_t v5;
	if (a2) {
		flags = GetData();
		flags->ucFlags |= 0x20u;
		v4 = flags->ucFlags;
	} else {
		v3 = GetData();
		v3->ucFlags &= (~0x20u);
		v5 = v3->ucFlags;
	}
}

bool TESQuest::GetAllowRepeatStages() {
	return ((GetData()->ucFlags & 8) != 0);
}

bool TESQuest::GetAllowRepeats() {
	return ((GetData()->ucFlags & 4) != 0);
}

int32_t TESQuest::GetFlag0x10() {
	return (int32_t)((uint8_t)((GetData()->ucFlags & 16) != 0));
}

void TESQuest::SetCompleted(bool abCompleted) {
	if (!abCompleted || (data.ucFlags & 2) != 0) {
		if (!abCompleted && (data.ucFlags & 2) != 0) {
			data.ucFlags &= ~2;
			AddChange(2);
			MiscStatManager::Decrement(MiscStatManager::MS_QUESTS_COMPLETED);
		}
	} else {
		data.ucFlags |= 2;
		AddChange(2);
		MiscStatManager::Increment(MiscStatManager::MS_QUESTS_COMPLETED);
		HUDMainMenu::ShowQuestName(this);
	}
}

void TESQuest::SetFailed(bool abFailed) {
	if (!abFailed || !GetCompleted()) {
		if (!abFailed || (data.ucFlags & 64) != 0) {
			if (!abFailed && (data.ucFlags & 64) != 0) {
				data.ucFlags &= ~64;
				AddChange(2);
			}
		} else {
			data.ucFlags |= 64;
			data.ucFlags |= 2;
			AddChange(2);
			HUDMainMenu::ShowQuestName(this);
		}
	}
}

uint8_t TESQuest::IsStageDone(uint8_t aucIndex) {
	for (BSSimpleList<TESQuestStage*>* pList = &m_listStages; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
		if (pList->m_item->m_Data.ucIndex == aucIndex) {
			return pList->m_item->m_Data.bIsDone;
		}
	}
	return 0;
}

void TESQuest::ResetQuest() {
	for (TESQuestStage* pstage : *&m_listStages) {
		if (pstage) {
			pstage->SetDone(0);
			for (TESQuestStageItem* m_item : *&pstage->listStageItems) {
				Date* pLogDate;
				if (m_item) {
					pLogDate = m_item->m_pLogDate;
				} else {
					pLogDate = nullptr;
				}
				if (pLogDate) {
					m_item->SetLogDate(0);
					delete pLogDate;
				}
			}
		}
	}
	cCurrentStage = 0;
	for (BGSQuestObjective*& rpItem : GetObjectiveList()->ItemsUntilNull()) {
		rpItem->SetState(QOS_DORMANT);
	}
	if (pScriptLocals) {
		ScriptLocals* apThis = pScriptLocals;
		if (apThis) {
			delete apThis;
		}
		pScriptLocals = pFormScript->CloneLocals();
	}
	TESTopic::ClearAllSaidOnceInfosForQuest(this);
	// The exe zero-extends the getter's al and passes that byte on unnormalised.
	uint8_t ucFlag0x10 = static_cast<uint8_t>(GetFlag0x10());
	SetEnabled(ucFlag0x10);
	SetCompleted(0);
	SetFailed(0);
	SetDisplayedQuestStarted(0);
}

// The exe's cleanup-only C++ EH frame is left out.
void TESQuest::GetQuestLog(BSSimpleList<TESQuestStageItem*>* apListToFill, TESQuest* apQuest, bool abShowCompleted,
	bool abNewestFirst, bool abShowAll) {
	FNVD_INLINED_LOCAL(BSSimpleList<TESQuest*>, foundList, std::nullopt, std::nullopt);
	if (apListToFill) {
		apListToFill->RemoveAll();
		::new (&foundList) BSSimpleList<TESQuest*>;
		BSSimpleList<TESQuestStageItem*>* pList = pPlayer->GetQuestLog();
		while (pList && pList->m_item) {
			TESQuestStageItem* a2 = pList->m_item;
			TESQuest* a2_4 = a2->GetOwner();
			pList = pList->m_pkNext;
			if (a2_4) {
				bool v6 = !apQuest || a2_4 == apQuest;
				bool v7 = v6;
				if (v6 && !apQuest) {
					bool v5 = (abShowCompleted == a2_4->GetCompleted() || abShowAll) && !foundList.IsInList(a2_4);
					v7 = v5;
					if (v5) {
						foundList.AddHead(a2_4);
					}
				}
				if (v7) {
					if (a2->m_pLogDate) {
						if (a2->GetLogEntry(a2_4)) {
							if (abNewestFirst) {
								apListToFill->AddTail(a2);
							} else {
								apListToFill->AddHead(a2);
							}
						}
					}
				}
			}
		}
		foundList.RemoveAll();
		foundList.~BSSimpleList<TESQuest*>();
	}
}

void BGSQuestObjective::SetOwner(TESQuest* apQuest) {
	pOwnerQuest = apQuest;
}

void TESQuestStageItem::SetLogDate(Date* apDate) {
	m_pLogDate = apDate;
}
