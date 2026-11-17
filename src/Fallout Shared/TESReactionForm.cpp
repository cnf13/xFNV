#include "Fallout Shared/TESReactionForm.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/ActorPackage.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "SDK/MSVC/rtti.h"

void GROUP_REACTION::Endian() {
	ENDIAN(pForm, false);
	ENDIAN(iReaction, false);
	ENDIAN(iFightReaction, false);
}

TESReactionForm::TESReactionForm() {
	TESReactionForm::InitializeDataComponent();
}

TESReactionForm::~TESReactionForm() {
	TESReactionForm::ClearDataComponent();
}

void TESReactionForm::InitializeDataComponent() {
	m_cGroupFormType = (uint8_t)8;
}

void TESReactionForm::ClearDataComponent() {
	FreeReactionList();
}

void TESReactionForm::CopyComponent(BaseFormComponent* apCopyFC) {
	TESReactionForm* pCopy = dynamic_cast<TESReactionForm*>(apCopyFC);
	if (!pCopy) {
		return;
	}
	FreeReactionList();
	BSSimpleList<GROUP_REACTION*>* pReactionList = QReactionList();
	BSSimpleList<GROUP_REACTION*>* pCopyList = pCopy->QReactionList();
	while (pCopyList && pCopyList->m_item) {
		GROUP_REACTION* pCopyReaction = pCopyList->m_item;
		GROUP_REACTION* pReaction = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
		pReaction->pForm = pCopyReaction->pForm;
		pReaction->iReaction = pCopyReaction->iReaction;
		pReaction->iFightReaction = pCopyReaction->iFightReaction;
		pReactionList->AddTail(pReaction);
		if (pReactionList->m_pkNext) {
			pReactionList = pReactionList->m_pkNext;
		}
		pCopyList = pCopyList->m_pkNext;
	}
}

bool TESReactionForm::CompareComponent(BaseFormComponent* apCompareFC) {
	TESReactionForm* pCompare = dynamic_cast<TESReactionForm*>(apCompareFC);
	if (!pCompare) {
		return 1;
	}
	BSSimpleList<GROUP_REACTION*>* pList = &m_reactionList;
	BSSimpleList<GROUP_REACTION*>* pCompList = &pCompare->m_reactionList;
	while (pList) {
		if (!pCompList) {
			return 1;
		}
		GROUP_REACTION* reaction = pList->m_item;
		GROUP_REACTION* compReaction = pCompList->m_item;
		if (reaction && compReaction) {
			if (reaction->pForm != compReaction->pForm) {
				return 1;
			}
			if (reaction->iReaction != compReaction->iReaction) {
				return 1;
			}
			if (reaction->iFightReaction != compReaction->iFightReaction) {
				return 1;
			}
		} else {
			if (reaction || compReaction) {
				return 1;
			}
		}
		pCompList = pCompList->m_pkNext;
		pList = pList->m_pkNext;
	}
	return pCompList && !pCompList->IsEmpty();
}

// The bytes the reactions take in a save: a two-byte count, six bytes of save-game block header when the save writes
// them and eight bytes per reaction, logged against the header of the form being saved when bDebugSaveBuffer is set.
uint16_t TESReactionForm::GetSaveSize(uint32_t aiFlags) {
	uint16_t usSize = 0;
	if ((aiFlags & 4) != 0) {
		uint16_t usStart = usSize;
		if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
			usSize += 4;
			usSize += 2;
		}
		BSSimpleList<GROUP_REACTION*>* pReactions = QReactionList();
		usSize += 2;
		usSize = static_cast<uint16_t>(usSize + 8 * pReactions->ItemsInList());
		if (bDebugSaveBuffer.Bool()) {
			SaveFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlySavingFormHeader();
			if (pHeader) {
				TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
				InitializationFlow("GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
					usSize - usStart, pHeader->iFormID, pForm->GetFormEditorID(), pHeader->iFlags, 1367,
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp");
			} else {
				InitializationFlow("GetSaveSize(): %-5i ending at line %i in file %s", usSize - usStart, 1367,
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp");
			}
		}
	}
	return usSize;
}

void TESReactionForm::SetReaction(TESForm* apForm, int32_t aiReaction) {
	if (apForm) {
		bool foundit = 0;
		for (GROUP_REACTION* fr : (&m_reactionList)->ItemsUntilNull()) {
			if (fr->pForm == apForm) {
				if (aiReaction || fr->iFightReaction) {
					fr->iReaction = aiReaction;
				} else {
					m_reactionList.Remove(fr);
					ourFree(fr);
				}
				foundit = 1;
				break;
			}
		}
		if (!foundit) {
			if (aiReaction) {
				GROUP_REACTION* fr_2 = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
				fr_2->pForm = apForm;
				fr_2->iReaction = aiReaction;
				(&m_reactionList)->AddTail(fr_2);
			}
		}
	}
}

void TESReactionForm::InitItem(TESForm* apOwner) {
	TESFile* pownerFile;
	BSSimpleList<GROUP_REACTION*>* pList = &m_reactionList;
	BSSimpleList<GROUP_REACTION*>* pLastList = 0;
	if (apOwner) {
		pownerFile = apOwner->GetFile(-1);
	} else {
		pownerFile = 0;
	}
	while (pList && pList->m_item) {
		GROUP_REACTION* reaction = pList->m_item;
		uint32_t tempID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(reaction->pForm));
		TESForm::AddCompileIndex(tempID, pownerFile);
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempID);
		reaction->pForm = FormByNumericID;
		if (reaction->pForm) {
			pList = pList->m_pkNext;
		} else {
			const char* v4 = apOwner->GetFormEditorID();
			int32_t CastingType = apOwner->iFormID;
			Warning("MASTERFILE: Form (%08X) not found for reaction of pForm (%08X) '%s'.\r\n", tempID, CastingType,
				v4);
			if (!pList->m_pkNext && pLastList) {
				pLastList->Remove(reaction);
				pList = pLastList->m_pkNext;
			} else {
				pList->RemoveHead();
			}
			ourFree(reaction);
		}
	}
}

int32_t TESReactionForm::GetReaction(TESForm* apForm) {
	BSSimpleList<GROUP_REACTION*>* v2;
	BSSimpleList<GROUP_REACTION*>* ecx0;
	if (apForm) {
		for (ecx0 = TESReactionForm::QReactionList(); ecx0 && ecx0->m_item; ecx0 = ecx0->m_pkNext) {
			v2 = ecx0;
			if (v2->m_item->pForm == apForm) {
				return v2->m_item->iReaction;
			}
		}
	}
	return 0;
}

void TESReactionForm::SetFightReaction(TESForm* apForm, int32_t aiReaction) {
	if (apForm) {
		char v7 = 0;
		for (GROUP_REACTION* v6 : QReactionList()->ItemsUntilNull()) {
			if (v6->pForm == apForm) {
				v6->iFightReaction = aiReaction;
				v7 = 1;
				break;
			}
		}
		if (!v7) {
			if (aiReaction) {
				GROUP_REACTION* a2 = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
				a2->pForm = apForm;
				a2->iReaction = 0;
				a2->iFightReaction = aiReaction;
				BSSimpleList<GROUP_REACTION*>* ActorPackageThatIsRunning = QReactionList();
				ActorPackageThatIsRunning->AddTail(a2);
			}
		}
	}
}

// Adds aiReaction to the reaction to apForm: frees the entry when the sum is 0, adds a new one (its iFightReaction left
// as ourMalloc returned it, as the exe does) when apForm has none.
void TESReactionForm::ModReaction(TESForm* apForm, int32_t aiReaction) {
	if (!apForm || !aiReaction) {
		return;
	}
	GROUP_REACTION* pReaction = 0;
	BSSimpleList<GROUP_REACTION*>* pList = QReactionList();
	bool bFound = false;
	while (pList && pList->m_item) {
		pReaction = pList->m_item;
		if (pReaction->pForm == apForm) {
			bFound = true;
			break;
		}
		pList = pList->m_pkNext;
	}
	if (!bFound) {
		pReaction = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
		pReaction->pForm = apForm;
		pReaction->iReaction = aiReaction;
		QReactionList()->AddTail(pReaction);
	} else {
		pReaction->iReaction += aiReaction;
		if (!pReaction->iReaction) {
			QReactionList()->Remove(pReaction);
			ourFree(pReaction);
		}
	}
}

void TESReactionForm::FreeReactionList() {
	while ((&m_reactionList)->m_item) {
		ourFree((&m_reactionList)->m_item);
		(&m_reactionList)->RemoveHead();
	}
}

void TESReactionForm::Save() {
	GROUP_REACTION fileReact;
	for (GROUP_REACTION* fr : (&m_reactionList)->ItemsUntilNull()) {
		if (!fr->pForm->GetDelete()) {
			// The file record holds the other form's ID in place of the pointer.
			fileReact.pForm = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(fr->pForm->iFormID));
			fileReact.iReaction = fr->iReaction;
			fileReact.iFightReaction = fr->iFightReaction;
			if (TESForm::QEndianSwapOnSave()) {
				fileReact.Endian();
			}
			TESForm::__AddChunkData(XNAM_ID, &fileReact, 12u);
			if (TESForm::QEndianSwapOnSave()) {
				fileReact.Endian();
			}
		}
	}
}

void TESReactionForm::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(4)) {
		uint32_t isaveGameCount = 0;
		uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
		for (GROUP_REACTION* preaction : m_reactionList) {
			if (preaction) {
				apSaveGameBuffer->SaveFormID(preaction->pForm, 0u);
				apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&preaction->iReaction), 4u, 0);
				apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&preaction->iFightReaction), 4u, 0);
				++isaveGameCount;
			}
		}
		apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
	}
}

void TESReactionForm::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(4)) {
		FreeReactionList();
		uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
		for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
			GROUP_REACTION* preaction = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
			apLoadGameBuffer->LoadFormID(preaction->pForm);
			apLoadGameBuffer->LoadData(&preaction->iReaction, 4u);
			apLoadGameBuffer->LoadData(&preaction->iFightReaction, 4u);
			m_reactionList.AddHead(preaction);
		}
	}
}

int32_t TESReactionForm::GetReaction_0048C1B0(TESForm* apForm) {
	if (apForm) {
		for (BSSimpleList<GROUP_REACTION*>* ecx0a = QReactionList(); ecx0a && ecx0a->m_item; ecx0a = ecx0a->m_pkNext) {
			BSSimpleList<GROUP_REACTION*>* v2 = ecx0a;
			if (v2->m_item->pForm == apForm) {
				return v2->m_item->iFightReaction;
			}
		}
	}
	return 0;
}

// Writes the reactions to the old-format save: an optional save-game block header, a two-byte count patched in after
// the reactions, then each reaction's form ID and iReaction.
void TESReactionForm::SaveGameTES(uint32_t aiFlags) {
	if ((aiFlags & 4) == 0) {
		return;
	}
	uint16_t usBlockSize = 0;
	char* pBlockStart = 0;
	char* pStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
	if (bDebugSaveBuffer.Bool()) {
		pStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		uint32_t uiBlockTag = 0x424C4F4B;
		pSaveLoadGameOLD->SaveGameDataOLD(&uiBlockTag, 4u);
		pBlockStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, 2u);
	}
	uint16_t usCount = 0;
	uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
	pSaveLoadGameOLD->SaveGameDataOLD(&usCount, 2u);
	for (BSSimpleList<GROUP_REACTION*>* pList = QReactionList(); pList; pList = pList->m_pkNext) {
		GROUP_REACTION* pReaction = pList->m_item;
		if (pReaction) {
			uint32_t uiFormID = pReaction->pForm->iFormID;
			pSaveLoadGameOLD->SaveNumericID(&uiFormID, 4u);
			pSaveLoadGameOLD->SaveGameDataOLD(&pReaction->iReaction, 4u);
			++usCount;
		}
	}
	*pCount = usCount;
	if (bDebugSaveBuffer.Bool()) {
		char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
		SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pHeader) {
			TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
			InitializationFlow("SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				static_cast<int32_t>(pEnd - pStart), pHeader->iFormID, pForm->GetFormEditorID(), pHeader->iFlags, 1403,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp");
		} else {
			InitializationFlow("SaveGame(): %-5i ending at line %i in file %s", static_cast<int32_t>(pEnd - pStart),
				1403, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp");
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
		if (pEnd > pBlockStart + 0xFFFF) {
			Warning("SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp", 1403);
		}
		// The save buffer is untyped bytes: the block's two-byte size slot sits right after its tag.
		*reinterpret_cast<uint16_t*>(pBlockStart) = static_cast<uint16_t>(pEnd - pBlockStart);
	}
}

void TESReactionForm::LoadGameTES(uint32_t a2, uint32_t a3) {
	if ((a2 & 4) != 0) {
		uint16_t usBlockSize = 0;
		uint8_t* pBlockStart = 0;
		if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
			int32_t iBlockTag;
			pSaveLoadGameOLD->LoadGameDataOLD(&iBlockTag, 4u);
			if (iBlockTag != 0x424C4F4B) {
				LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
				if (pHeader) {
					TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
					Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
							"Currently loading form is %08X %s with version %i and flags %08X",
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp", 1415,
						pHeader->iFormID, pForm->GetFormEditorID(), pHeader->cVersion, pHeader->iFlags);
				} else {
					Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
							"Current version is %i",
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp", 1415,
						pSaveLoadGameOLD->GetCurrentVersion());
				}
			}
			pBlockStart = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
			pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, 2u);
		}
		FreeReactionList();
		uint16_t usCount;
		pSaveLoadGameOLD->LoadGameDataOLD(&usCount, 2u);
		for (uint32_t i = 0; i < usCount; ++i) {
			uint32_t uiFormID;
			int32_t iReaction;
			pSaveLoadGameOLD->LoadNumericID(&uiFormID, 4u);
			pSaveLoadGameOLD->LoadGameDataOLD(&iReaction, 4u);
			// EXE-CAST: uint32_t (a form ID) as TESForm* (InitLoadGame resolves it to the form)
			SetReaction(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID)), iReaction);
		}
		if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
			uint8_t* pEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
			LoadFormHeader* pHeader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
			if (pHeader_2) {
				TESForm* pForm_2 = TESForm::GetFormByNumericID(pHeader_2->iFormID);
				if (pEnd <= &pBlockStart[usBlockSize]) {
					if (pEnd < &pBlockStart[usBlockSize]) {
						int32_t iFlags = pHeader_2->iFlags;
						int32_t cVersion = pHeader_2->cVersion;
						const char* pEditorID = pForm_2->GetFormEditorID();
						Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  "
								"Currently loading form is %08X %s with version %i and flags %08X",
							static_cast<int32_t>(&pBlockStart[usBlockSize] - pEnd),
							"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp", 1433,
							pHeader_2->iFormID, pEditorID, cVersion, iFlags);
					}
				} else {
					int32_t iFlags_2 = pHeader_2->iFlags;
					int32_t cVersion_2 = pHeader_2->cVersion;
					const char* pEditorID_2 = pForm_2->GetFormEditorID();
					Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  "
							"Currently loading form is %08X %s with version %i and flags %08X",
						static_cast<int32_t>(pEnd - &pBlockStart[usBlockSize]),
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp", 1433,
						pHeader_2->iFormID, pEditorID_2, cVersion_2, iFlags_2);
				}
			} else {
				if (pEnd <= &pBlockStart[usBlockSize]) {
					if (pEnd < &pBlockStart[usBlockSize]) {
						int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
						Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  "
								"Current version is %i",
							static_cast<int32_t>(&pBlockStart[usBlockSize] - pEnd),
							"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp", 1433,
							iCurrentVersion);
					}
				} else {
					int32_t iCurrentVersion_2 = pSaveLoadGameOLD->GetCurrentVersion();
					Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  "
							"Current version is %i",
						static_cast<int32_t>(pEnd - &pBlockStart[usBlockSize]),
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp", 1433,
						iCurrentVersion_2);
				}
			}
		}
	}
}

// Resolves each reaction's form ID (LoadGameTES keeps it in pForm) and drops the reactions whose form is gone.
void TESReactionForm::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(4)) {
		BSSimpleList<GROUP_REACTION*>* pList = &m_reactionList;
		BSSimpleList<GROUP_REACTION*>* pLastList = 0;
		while (pList && !pList->IsEmpty()) {
			GROUP_REACTION* pReaction = pList->m_item;
			bool bRemove = false;
			if (pReaction) {
				TESForm* pForm;
				if (pReaction->pForm) {
					// EXE-CAST: TESForm* as uint32_t (a form ID until here)
					pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pReaction
							->pForm)));
				} else {
					pForm = 0;
				}
				pReaction->pForm = pForm;
				if (!pReaction->pForm) {
					ourFree(pReaction);
					bRemove = true;
				}
			} else {
				bRemove = true;
			}
			if (bRemove) {
				if (pLastList) {
					pLastList->Remove(pList->m_item);
					pList = pLastList->m_pkNext;
				} else {
					pList->RemoveHead();
				}
			} else {
				pLastList = pList;
				pList = pList->m_pkNext;
			}
		}
	}
}

// With bit 2 of aiFlags, each reaction's other form (its form ID until now) is looked up; a reaction whose form is
// gone is removed from the head of the node it sits in and freed, and the same node is tested again.
void TESReactionForm::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
	if ((aiFlags & 4) != 0) {
		BSSimpleList<GROUP_REACTION*>* pIter = &m_reactionList;
		while (pIter) {
			GROUP_REACTION* preaction = pIter->m_item;
			if (preaction) {
				// The field holds the other form's ID until here.
				uint32_t iformID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(preaction->pForm));
				preaction->pForm = TESForm::GetFormByNumericID(iformID);
				if (!preaction->pForm) {
					pIter->RemoveHead();
					ourFree(preaction);
				} else {
					pIter = pIter->m_pkNext;
				}
			} else {
				pIter = pIter->m_pkNext;
			}
		}
	}
}
