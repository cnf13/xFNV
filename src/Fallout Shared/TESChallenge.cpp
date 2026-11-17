#include "Fallout Shared/TESChallenge.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSEntryPointFunctionDataActivateChoice.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TESChallengeManager.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<GameSettingCollection> sChallengeComplete("sChallengeComplete", "Challenge Completed");
SettingT<GameSettingCollection> sChallengeProgress("sChallengeProgress", "Challenge Progress");
SettingT<INISettingCollection> bShowChallengeUpdates("bShowChallengeUpdates:GamePlay", true);
TESChallengeManager::ChallengeListTable TESChallengeManager::pInactiveChallengeList;
TESChallengeManager::ChallengeListTable TESChallengeManager::pChallengeList;

void CHALLENGE_DATA::Endian() {
	ENDIAN(nChallengeType, false);
	ENDIAN(iThreshold, false);
	ENDIAN(nFlags, false);
	ENDIAN(iInterval, false);
	ENDIAN(nSpecialDataOne, false);
	ENDIAN(nSpecialDataTwo, false);
	ENDIAN(nSpecialDataThree, false);
}

TESChallenge::TESChallenge() {
	TESChallenge::InitializeData();
	SetFormType(CHAL_ID);
}

TESChallenge::~TESChallenge() {}

void TESChallenge::InitializeData() {
	ourMemset(&data, 0, sizeof(CHALLENGE_DATA));
	ourMemset(&progress, 0, sizeof(CHALLENGE_PROGRESS));
	pFormOne = nullptr;
	pFormTwo = nullptr;
	SetShowZero(true);
}

bool TESChallenge::Load(TESFile* apFile) {
	LoadForm(apFile);
	SetInitialized(false);
	do {
		uint32_t uiChunk = apFile->GetTESChunk();
		if (uiChunk == NO_CHUNK) {
			break;
		}
		switch (uiChunk) {
		case EDID_ID: {
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size, read up
			// to 0x200.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0x200);
			SetFormEditorID(pBuffer);
			break;
		}
		case DATA_ID:
			LoadData(apFile, reinterpret_cast<char*>(&data), 0x18);
			if (apFile->QEndian()) {
				data.Endian();
			}
			break;
		case DESC_ID:
			TESDescription::LoadDescriptionData(static_cast<TESDescription*>(this), apFile);
			break;
		case OBND_ID:
			LoadObjectBound(apFile);
			break;
		case SCRI_ID: {
			uint32_t uiFormID = 0;
			apFile->GetChunkData(uiFormID);
			TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
			pScriptableForm->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiFormID)));
			pScriptableForm->InitItem(this);
			break;
		}
		case FULL_ID:
			TESFullName::LoadFullNameChunk(this, apFile);
			break;
		case ICON_ID:
			TESTexture::LoadTextureChunk(this, apFile);
			break;
		case MICO_ID:
			BGSMessageIcon::LoadMessageIcon(this, apFile);
			break;
		case SNAM_ID: {
			uint32_t uiFormID = 0;
			apFile->GetChunkData(uiFormID);
			SetFormOne(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID)));
			break;
		}
		case XNAM_ID: {
			uint32_t uiFormID = 0;
			apFile->GetChunkData(uiFormID);
			SetFormTwo(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID)));
			break;
		}
		default:
			break;
		}
	} while (apFile->NextChunk());
	return true;
}

void TESChallenge::Save() {
	uint32_t v2;
	uint32_t v3;
	TESForm::StartForm();
	static_cast<TESFullName*>(this)->Save();
	static_cast<TESTexture*>(this)->Save(ICON_ID);
	static_cast<BGSMessageIcon*>(this)->Save(static_cast<CHUNK_ID>(1329809741));
	static_cast<TESScriptableForm*>(this)->Save();
	static_cast<TESDescription*>(this)->Save();
	if (TESForm::QEndianSwapOnSave()) {
		(&data)->Endian();
	}
	TESForm::__SaveData(&data, 0x18u);
	if (TESForm::QEndianSwapOnSave()) {
		(&data)->Endian();
	}
	if (pFormOne) {
		v3 = pFormOne->TESForm::GetFormID();
		TESForm::AddChunk(SNAM_ID, v3);
	}
	if (pFormTwo) {
		v2 = pFormTwo->TESForm::GetFormID();
		TESForm::AddChunk(XNAM_ID, v2);
	}
	TESForm::CloseForm();
}

uint16_t TESChallenge::GetSaveSize(uint32_t aiFlags) {
	return TESForm::GetSaveSize(aiFlags);
}

void TESChallenge::SaveGame(uint32_t aiFlags) {
	TESForm::SaveGame(aiFlags);
}

// The form's data, then the progress and its flags.
void TESChallenge::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	TESForm::SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&progress.iProgress), 4, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&progress.nProgressFlags), 4, 0);
}

void TESChallenge::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	TESForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESChallenge::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESForm::LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&progress.iProgress), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&progress.nProgressFlags), 4);
}

void TESChallenge::InitItem() {
	if (!GetInitialized()) {
		TESScriptableForm::InitItem(this);
		uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormOne));
		if (itempInt) {
			TESFile* File = GetFile(-1);
			TESForm::AddCompileIndex(itempInt, File);
			pFormOne = TESForm::GetFormByNumericID(itempInt);
			if (!pFormOne) {
				int32_t CastingType = iFormID;
				const char* v3 = GetFormEditorID();
				Warning("MASTERFILE: Could not find Form (%08X) on challenge '%s' (%08X).", itempInt, v3, CastingType);
			}
		}
		itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormTwo));
		if (itempInt) {
			TESFile* v2 = GetFile(-1);
			TESForm::AddCompileIndex(itempInt, v2);
			pFormTwo = TESForm::GetFormByNumericID(itempInt);
			if (!pFormTwo) {
				int32_t v6 = iFormID;
				const char* v4 = GetFormEditorID();
				Warning("MASTERFILE: Could not find Form (%08X) on challenge '%s' (%08X).", itempInt, v4, v6);
			}
		}
		SetInitialized(1);
	}
}

void TESChallenge::Copy(TESForm* apCopy) {
	TESWaterForm* formOne;
	TESWaterForm* formTwo;
	TESChallenge* v5 = dynamic_cast<TESChallenge*>(apCopy);
	if (v5) {
		ourMemcpy(&data.nChallengeType, &v5->data, sizeof(CHALLENGE_DATA) - offsetof(CHALLENGE_DATA, nChallengeType));
		formOne = static_cast<TESWaterForm*>(v5->GetFormOne());
		SetFormOne(formOne);
		formTwo = static_cast<TESWaterForm*>(v5->GetFormTwo());
		SetFormTwo(formTwo);
		TESForm::CopyAllComponents(apCopy);
	}
}

// True when the forms differ: data, the two forms, then the components.
bool TESChallenge::Compare(TESForm* apCompare) {
	TESChallenge* pChallenge = dynamic_cast<TESChallenge*>(apCompare);
	if (!pChallenge) {
		return true;
	}
	if (std::memcmp(&data, pChallenge->GetData(), sizeof(CHALLENGE_DATA))) {
		return true;
	}
	if (GetFormTwo() != pChallenge->GetFormTwo()) {
		return true;
	}
	if (GetFormOne() != pChallenge->GetFormOne()) {
		return true;
	}
	return CompareAllComponents(pChallenge) ? true : false;
}

CHALLENGE_TYPE TESChallenge::GetChallengeType() {
	return static_cast<CHALLENGE_TYPE>(data.nChallengeType);
}

void TESChallenge::MarkForSave() {
	AddChange(2);
}

void TESChallenge::SetShowZero(bool abVal) {
	uint16_t v2;
	uint16_t uiFlags;
	if (abVal) {
		data.nFlags |= 4u;
		uiFlags = data.nFlags;
	} else {
		data.nFlags &= (~4u);
		v2 = data.nFlags;
	}
	uint16_t v4 = data.nFlags;
}

TESForm* TESChallenge::GetFormTwo() {
	return pFormTwo;
}

TESForm* TESChallenge::GetFormOne() {
	return pFormOne;
}

CHALLENGE_DATA* TESChallenge::GetData() {
	return &data;
}

int32_t TESChallenge::GetUnlocked() {
	return (int32_t)(progress.nProgressFlags & 1);
}

uint32_t TESChallenge::GetStartsLocked() {
	return data.nFlags & CHALLENGE_DATA::CHALLENGE_STARTS_LOCKED;
}

int32_t TESChallenge::GetCompleted() {
	return (int32_t)(progress.nProgressFlags & 2);
}

void TESChallenge::ClearProgress() {
	progress.iProgress = (uint32_t)0;
}

int32_t TESChallenge::GetRecurring() {
	return data.nFlags & 2;
}

int32_t TESChallenge::GetSpecialOne() {
	return (int32_t)data.nSpecialDataOne;
}

int32_t TESChallenge::GetSpecialThree() {
	return (int32_t)data.nSpecialDataThree;
}

void TESChallenge::SetCompleted(bool abVal) {
	uint32_t v2;
	uint32_t uiChallengeFlags;
	if (abVal) {
		progress.nProgressFlags |= 2;
		uiChallengeFlags = progress.nProgressFlags;
	} else {
		progress.nProgressFlags &= (~2u);
		v2 = progress.nProgressFlags;
	}
	uint32_t v4 = progress.nProgressFlags;
	MarkForSave();
}

void TESChallenge::SetCompletedRecurring(bool abVal) {
	uint32_t v2;
	uint32_t uiChallengeFlags;
	if (abVal) {
		progress.nProgressFlags |= 4;
		uiChallengeFlags = progress.nProgressFlags;
	} else {
		progress.nProgressFlags &= (~4u);
		v2 = progress.nProgressFlags;
	}
	uint32_t v4 = progress.nProgressFlags;
	MarkForSave();
}

int32_t TESChallenge::GetRemovedRecurring() {
	return (int32_t)(progress.nProgressFlags & 8);
}

void TESChallenge::IncrementProgress(int32_t iValue) {
	progress.iProgress += iValue;
	MarkForSave();
}

void TESChallenge::SetUnlocked(bool abVal) {
	uint32_t v2;
	uint32_t uiChallengeFlags;
	if (abVal) {
		progress.nProgressFlags |= 1;
		uiChallengeFlags = progress.nProgressFlags;
	} else {
		progress.nProgressFlags &= (~1u);
		v2 = progress.nProgressFlags;
	}
	uint32_t v4 = progress.nProgressFlags;
	MarkForSave();
}

int32_t TESChallenge::GetSpecialTwo() {
	return (int32_t)data.nSpecialDataTwo;
}

bool TESChallenge::GetShowZero() {
	return ((data.nFlags & 4) != 0);
}

int32_t TESChallenge::GetCompletedRecurring() {
	return progress.nProgressFlags & 4;
}

void TESChallenge::RemoveRecurringFromChallenge(char a1) {
	if (a1) {
		progress.nProgressFlags |= 8u;
		uint32_t nProgressFlags = progress.nProgressFlags;
	} else {
		progress.nProgressFlags &= ~8u;
		uint32_t v2 = progress.nProgressFlags;
	}
	uint32_t v4 = progress.nProgressFlags;
	MarkForSave();
}

bool TESChallengeManager::UpdateChallenge(CHALLENGE_TYPE cType, int32_t iCount, TESForm* pFormOne, TESForm* pFormTwo,
	uint16_t nSpecialDataOne, uint16_t nSpecialDataTwo, uint16_t nSpecialDataThree) {
	// The exe tests the type signed (0x5F5964 cmp 0xE; jge, then cmp 0; jge): the enum's constant, not the class's
	// unsigned CHALLENGE_TYPE_COUNT.
	if (cType >= ::CHALLENGE_TYPE_COUNT || cType < 0) {
		return 0;
	}
	BSSimpleList<TESChallenge*>* pChallenges = &TESChallengeManager::pInactiveChallengeList[cType];
	bool bCompletedChallenges = 0;
	TESChallenge* pItem = 0;
	while (pChallenges) {
		pItem = pChallenges->m_item;
		bool bFound = 1;
		if (!pItem) {
			goto LABEL_11;
		}
		if (!pItem->GetCompleted() || pItem->GetRecurring()) {
			if (!pItem->GetStartsLocked() || pItem->GetUnlocked()) {
				if (pItem->GetFormOne()) {
					if (pFormOne) {
						switch (cType) {
						case CHALLENGE_TYPE_KILL_LIST:
						case CHALLENGE_TYPE_USE_ITEM_LIST:
						case CHALLENGE_TYPE_AQUIRE_LIST: {
							BGSListForm* pformlist = static_cast<BGSListForm*>(pItem->GetFormOne());
							if (pformlist) {
								if (!(&pformlist->ListOfForms)->IsInList(pFormOne)) {
									bFound = 0;
								}
							}
							break;
						}
						case CHALLENGE_TYPE_KILL_SPECIFIC:
						case CHALLENGE_TYPE_HIT:
						case CHALLENGE_TYPE_USE_ITEM:
						case CHALLENGE_TYPE_AQUIRE:
						case CHALLENGE_TYPE_SKILL:
						case CHALLENGE_TYPE_DAMAGE:
						case CHALLENGE_TYPE_CRAFT: {
							BGSListForm* pformlist_2 = static_cast<BGSListForm*>(pItem->GetFormOne());
							if (pFormOne != pformlist_2) {
								bFound = 0;
							}
							break;
						}
						case CHALLENGE_TYPE_DISCOVER: {
							BGSListForm* pformlist_3 = static_cast<BGSListForm*>(pItem->GetFormOne());
							if (pFormOne != pformlist_3) {
								bFound = 0;
							}
							break;
						}
						default: {
							break;
						}
						}
					} else {
						bFound = 0;
					}
				}
				if (!bFound) {
					goto LABEL_60;
				}
				if (pItem->GetFormTwo()) {
					if (pFormTwo) {
						if (cType <= CHALLENGE_TYPE_DISCOVER || cType == CHALLENGE_TYPE_DAMAGE) {
							BGSListForm* pformlist_4 = static_cast<BGSListForm*>(pItem->GetFormTwo());
							if (pformlist_4) {
								if (!(&pformlist_4->ListOfForms)->IsInList(pFormTwo)) {
									bFound = 0;
								}
							}
						}
					} else {
						bFound = 0;
					}
				}
				if (!bFound) {
					goto LABEL_60;
				}
				if (pItem->GetSpecialOne()) {
					if (!nSpecialDataOne) {
						bFound = 0;
					}
					int32_t iSpecialOne = nSpecialDataOne;
					if (iSpecialOne != pItem->GetSpecialOne()) {
						bFound = 0;
					}
				} else {
					if (pItem->GetChallengeType() == CHALLENGE_TYPE_MISC_STAT) {
						int32_t iSpecialOne = nSpecialDataOne;
						if (iSpecialOne != pItem->GetSpecialOne()) {
							bFound = 0;
						}
					}
				}
				if (pItem->GetSpecialTwo()) {
					if (!nSpecialDataTwo) {
						bFound = 0;
					}
					int32_t iSpecialTwo = nSpecialDataTwo;
					if (iSpecialTwo != pItem->GetSpecialTwo()) {
						bFound = 0;
					}
				} else {
					if (pItem->GetChallengeType() == CHALLENGE_TYPE_HIT) {
						if (nSpecialDataTwo) {
							if (nSpecialDataTwo != 1) {
								bFound = 0;
							}
						}
					}
				}
				if (pItem->GetSpecialThree()) {
					if (!nSpecialDataThree) {
						bFound = 0;
					}
					int32_t iSpecialThree = nSpecialDataThree;
					if (iSpecialThree != pItem->GetSpecialThree()) {
						bFound = 0;
					}
				}
				if (!bFound) {
					goto LABEL_60;
				}
				pItem->IncrementProgress(iCount);
				int32_t iProgress = pItem->progress.iProgress;
				if (iProgress < pItem->data.iThreshold) {
					uint32_t uiInterval;
					uint32_t uiProgress = pItem->progress.iProgress;
					if (pItem->data.iInterval) {
						uiInterval = pItem->data.iInterval;
					} else {
						uiInterval = 100;
					}
					uint32_t uiStep = uiInterval;
					for (int32_t i = uiProgress; i >= 0; i -= uiStep) {
						if (!i) {
							if (pItem->GetFullName()) {
								if (pItem->GetDescription(0, DESC_ID)) {
									if (bShowChallengeUpdates.Bool()) {
										char apDest[512];
										const char* pDescription = pItem->GetDescription(0, DESC_ID);
										int32_t iThreshold = pItem->data.iThreshold;
										uint32_t uiCurrent = pItem->progress.iProgress;
										BSsprintf(apDest, 512, "%s   %d\\%d\n%s", pItem->GetFullName(), uiCurrent,
											iThreshold, pDescription);
										const char* TextureName = TESTexture::GetTextureName(pItem, 0);
										Interface::ShowMessage(apDest, 0, TextureName, 0, 2.0f, 0);
									}
								}
							}
						}
					}
				} else {
					if (pItem->pFormScript) {
						pItem->pFormScript->RunScriptEffectStart(pPlayer, 0);
					}
					if (pItem->GetChallengeType() != CHALLENGE_TYPE_MISC_STAT || pItem->GetSpecialOne() != 27) {
						MiscStatManager::Increment(MiscStatManager::MS_CHALLENGES_COMPLETED);
					}
					if (pItem->GetFullName()) {
						if (pItem->GetDescription(0, DESC_ID)) {
							if (bShowChallengeUpdates.Bool()) {
								char msg[512];
								const char* v22 = pItem->GetDescription(0, DESC_ID);
								int32_t time = pItem->data.iThreshold;
								int32_t NumberOfItemsActivated = pItem->data.iThreshold;
								BSsprintf(msg, 512, "%s   %d\\%d\n%s", pItem->GetFullName(), NumberOfItemsActivated,
									time, v22);
								const char* v16 = TESTexture::GetTextureName(pItem, 0);
								Interface::ShowMessage(msg, 0, v16, 0, 2.0f, 0);
								Interface::PlayMenuSound(21);
							}
						}
					}
					if (!pItem->GetRecurring() || pItem->GetRemovedRecurring()) {
						pItem->SetCompleted(1);
						bCompletedChallenges = 1;
					} else {
						uint32_t v17 = pItem->progress.iProgress;
						int32_t iSpillOver = v17 - pItem->data.iThreshold;
						pItem->ClearProgress();
						pItem->SetCompletedRecurring(1);
						pItem->IncrementProgress(iSpillOver);
					}
				}
			}
		LABEL_11:;
			pChallenges = pChallenges->m_pkNext;
		} else {
		LABEL_60:;
			pChallenges = pChallenges->m_pkNext;
		}
	}
	if (bCompletedChallenges) {
		TESChallengeManager::LoadChallenges();
	}
	return 1;
}

// The exe compares the index signed: the global enum's count, not the class's unsigned copy.
void TESChallengeManager::UnloadChallenges() {
	for (int32_t i = 0; i < ::CHALLENGE_TYPE_COUNT; ++i) {
		pInactiveChallengeList[i].RemoveAll();
		pChallengeList[i].RemoveAll();
	}
}

void TESChallengeManager::LoadChallenges() {
	TESChallengeManager::UnloadChallenges();
	BSSimpleList<TESChallenge*>* pChallenges = pDataHandler->GetChallengeList();
	while (pChallenges) {
		TESChallenge* pItem = pChallenges->m_item;
		if (!pItem || pItem->GetCompleted() || (pItem->GetStartsLocked() && !pItem->GetUnlocked())) {
			if (pItem) {
				CHALLENGE_TYPE eType = pItem->GetChallengeType();
				TESChallengeManager::pChallengeList[eType].AddHead(pItem);
			}
		} else {
			CHALLENGE_TYPE eType = pItem->GetChallengeType();
			TESChallengeManager::pInactiveChallengeList[eType].AddHead(pItem);
		}
		pChallenges = pChallenges->m_pkNext;
	}
}

bool TESChallengeManager::UnlockChallenge(TESChallenge* pChallenge) {
	if (pChallenge) {
		CHALLENGE_TYPE cType = pChallenge->GetChallengeType();
		pChallenge->SetUnlocked(1);
		if (!TESChallengeManager::pInactiveChallengeList[cType].IsInList(pChallenge)) {
			TESChallengeManager::pInactiveChallengeList[cType].AddHead(pChallenge);
		}
		if (TESChallengeManager::pChallengeList[cType].IsInList(pChallenge)) {
			TESChallengeManager::pChallengeList[cType].Remove(pChallenge);
		}
	}
	return 1;
}

bool TESChallengeManager::IncrementScriptedChallenge(TESChallenge* apChallenge, int32_t aiCount) {
	bool bLoadChallenges = false;
	TESChallenge* pChallenge = nullptr;
	for (BSSimpleList<TESChallenge*>* pNode = &pInactiveChallengeList[CHALLENGE_TYPE_SCRIPTED]; pNode;
		pNode = pNode->m_pkNext) {
		pChallenge = pNode->m_item;
		if (!pChallenge) {
			continue;
		}
		if (pChallenge->GetCompleted() && !pChallenge->GetRecurring()) {
			continue;
		}
		if (pChallenge->GetStartsLocked() && !pChallenge->GetUnlocked()) {
			continue;
		}
		if (apChallenge->iFormID != pChallenge->iFormID) {
			continue;
		}
		pChallenge->IncrementProgress(aiCount);
		if (static_cast<int32_t>(pChallenge->progress.iProgress) < pChallenge->data.iThreshold) {
			// Not done yet: a progress message when the progress is a multiple of the interval (100 when it is 0),
			// found by counting down from the progress.
			int32_t iInterval = pChallenge->data.iInterval ? pChallenge->data.iInterval : 100;
			for (int32_t i = static_cast<int32_t>(pChallenge->progress.iProgress); i >= 0; i -= iInterval) {
				if (i != 0 || !static_cast<TESFullName*>(pChallenge)->GetFullName() ||
					!static_cast<TESDescription*>(pChallenge)->GetDescription(nullptr, CHUNK_ID::DESC_ID) ||
					!bShowChallengeUpdates.Bool()) {
					continue;
				}
				char acMessage[512];
				const char* pDescription =
					static_cast<TESDescription*>(pChallenge)->GetDescription(nullptr, CHUNK_ID::DESC_ID);
				int32_t iThreshold = pChallenge->data.iThreshold;
				uint32_t iProgress = pChallenge->progress.iProgress;
				BSsprintf(acMessage, sizeof(acMessage), "%s   %d\\%d\n%s",
					static_cast<TESFullName*>(pChallenge)->GetFullName(), iProgress, iThreshold, pDescription);
				Interface::ShowMessage(acMessage, 0, TESTexture::GetTextureName(pChallenge, nullptr), nullptr, 2.0f,
					false);
			}
			continue;
		}
		if (pChallenge->pFormScript) {
			pChallenge->pFormScript->RunScriptEffectStart(pPlayer, nullptr);
		}
		// A misc-stat challenge on the completed-challenges stat does not count itself.
		if (pChallenge->GetChallengeType() != CHALLENGE_TYPE_MISC_STAT ||
			pChallenge->GetSpecialOne() != MiscStatManager::MS_CHALLENGES_COMPLETED) {
			MiscStatManager::Increment(MiscStatManager::MS_CHALLENGES_COMPLETED);
		}
		if (static_cast<TESFullName*>(pChallenge)->GetFullName() &&
			static_cast<TESDescription*>(pChallenge)->GetDescription(nullptr, CHUNK_ID::DESC_ID) &&
			bShowChallengeUpdates.Bool()) {
			char acMessage[512];
			const char* pDescription =
				static_cast<TESDescription*>(pChallenge)->GetDescription(nullptr, CHUNK_ID::DESC_ID);
			// The exe passes the threshold for both numbers.
			int32_t iThreshold = pChallenge->data.iThreshold;
			BSsprintf(acMessage, sizeof(acMessage), "%s   %d\\%d\n%s",
				static_cast<TESFullName*>(pChallenge)->GetFullName(), iThreshold, iThreshold, pDescription);
			Interface::ShowMessage(acMessage, 0, TESTexture::GetTextureName(pChallenge, nullptr), nullptr, 2.0f, false);
			Interface::PlayMenuSound(21);
		}
		if (!pChallenge->GetRecurring() || pChallenge->GetRemovedRecurring()) {
			pChallenge->SetCompleted(true);
			bLoadChallenges = true;
		} else {
			// A recurring challenge starts again with what went past the threshold.
			int32_t iOverflow = static_cast<int32_t>(pChallenge->progress.iProgress) - pChallenge->data.iThreshold;
			pChallenge->ClearProgress();
			pChallenge->SetCompletedRecurring(true);
			pChallenge->IncrementProgress(iOverflow);
		}
	}
	if (bLoadChallenges) {
		TESChallengeManager::LoadChallenges();
	}
	return true;
}
