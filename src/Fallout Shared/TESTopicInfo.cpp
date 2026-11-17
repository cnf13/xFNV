#include "Fallout Shared/TESTopicInfo.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueInfo.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AudioMarkerData.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ResponseListWrapper.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESConversationData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESResponse.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TopicInfoArray.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"

extern TESTopicInfo* pCurrentResultScriptInfo;

// The info whose responses kResponseList holds, and the info whose result script is running.
static TESTopicInfo* pCurrentResponsesInfo;
TESTopicInfo* pCurrentResultScriptInfo;

// The two shared result scripts (begin and end) the last asked info's scripts are loaded into.
using ResultScriptArray = Script[TIRS_COUNT];
static ResultScriptArray pResultScriptA;

// GetResultScript's function-local static: the variable an SLSD chunk added last, which SCVR names.
static ScriptVariable* pNewVariable;

// The game settings this file defines, in their initialization order.
SettingT<GameSettingCollection> iSpeechChallengeDifficultyVeryEasy("iSpeechChallengeDifficultyVeryEasy", 0);
SettingT<GameSettingCollection> iSpeechChallengeDifficultyEasy("iSpeechChallengeDifficultyEasy", 25);
SettingT<GameSettingCollection> iSpeechChallengeDifficultyAverage("iSpeechChallengeDifficultyAverage", 50);
SettingT<GameSettingCollection> iSpeechChallengeDifficultyHard("iSpeechChallengeDifficultyHard", 75);
SettingT<GameSettingCollection> iSpeechChallengeDifficultyVeryHard("iSpeechChallengeDifficultyVeryHard", 100);
ResponseListWrapper kResponseList;

TESTopicInfo::TESTopicInfo(DIALOGUE_TYPE aeType) {
	TESTopicInfo::InitializeData();
	m_Data.type = (uint8_t)aeType;
	if (aeType == 1) {
		m_pConversationData = new TESConversationData();
		SetRandom(1);
	}
}

TESTopicInfo::~TESTopicInfo() {
	TESTopicInfo::ClearData();
}

// Stores the fields in this order; the two calls to the shared setter clear pPerkSkillStat.
void TESTopicInfo::InitializeData() {
	ourMemset(&m_Data, 0, sizeof(m_Data));
	iFileOffset = 0;
	bSaidOnce = false;
	iInfoIndex = 0xFFFF;
	SetFormType(INFO_ID);
	m_pConversationData = nullptr;
	pOwnerQuest = nullptr;
	SetPrompt("");
	SetSpeaker(nullptr);
	SetActorValue(nullptr);
	SetPerk(nullptr);
	SetDifficultyLevel(SCL_VERYEASY);
}

void TESTopicInfo::ClearData() {
	if (!GetTemporary()) {
		if (GetInfoIndex() != 0xFFFF) {
			TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(this);
			if (pOwnerTopic) {
				RemoveInfoFromTopic(pOwnerTopic);
			}
		}
		GetInfoConditions()->ClearAllConditionItems();
		ClearResponseList();
		ClearAddTopicList();
		// The result-script temporary: raw storage, so no C++ destructor runs on it.
		alignas(4) uint8_t aucScript[sizeof(Script)];
		Script* pScript = ::new (aucScript) Script;
		pResultScriptA[TIRS_BEGIN].CopyResultScript(pScript);
		pResultScriptA[TIRS_END].CopyResultScript(pScript);
		pScript->~Script();
	}
	pCurrentResponsesInfo = nullptr;
	pCurrentResultScriptInfo = nullptr;
	if (m_pConversationData) {
		delete m_pConversationData;
	}
}

// The DNAM value that GetChunkData fills starts at 0 for a zero-size chunk (malformed data). Form IDs read from the
// file are stored in the pointer fields and lists until InitItem resolves them, as the game does.
bool TESTopicInfo::Load(TESFile* apFile) {
	uint8_t formid = apFile->GetTESForm();
	if (formid != 70) {
		return 0;
	}
	if (!apFile->GetOptimizedFile()) {
		iFileOffset = apFile->GetOffset();
	}
	uint32_t iPrevInfoID = -1;
	uint32_t questID = 0;
	LoadForm(apFile);
	do {
		CHUNK_ID chunkid = apFile->GetTESChunk();
		if (chunkid == NO_CHUNK) {
			break;
		}
		if (chunkid > DNAM_ID) {
			if (chunkid > RNAM_ID) {
				if (chunkid == TCLT_ID) {
					if (!GetConversationData()) {
						LoadConversation(0);
					}
					uint32_t topicID_2 = 0;
					apFile->GetChunkData<uint32_t>(topicID_2);
					TESTopic* pLinkTo = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(topicID_2));
					(&GetConversationData()->m_listLinkTo)->AddTail(pLinkTo);
				} else {
					if (chunkid == TCFU_ID) {
						if (!GetConversationData()) {
							LoadConversation(0);
						}
						uint32_t topicInfoID = 0;
						apFile->GetChunkData<uint32_t>(topicInfoID);
						TESTopicInfo* pFollowUp = reinterpret_cast<TESTopicInfo*>(static_cast<uintptr_t>(topicInfoID));
						GetConversationData()->GetFollowUpInfoList()->AddTail(pFollowUp);
					}
				}
			} else {
				switch (chunkid) {
				case RNAM_ID: {
					char szBuf[256];
					apFile->GetChunkData(szBuf, 256u);
					SetPrompt(szBuf);
					break;
				}
				case KNAM_ID: {
					uint32_t skillID = 0;
					apFile->GetChunkData<uint32_t>(skillID);
					pPerkSkillStat = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(skillID));
					break;
				}
				case PNAM_ID: {
					apFile->GetChunkData<uint32_t>(iPrevInfoID);
					if (iPrevInfoID) {
						TESForm::AddCompileIndex(iPrevInfoID, apFile);
					}
					break;
				}
				}
			}
		} else {
			if (chunkid == DNAM_ID) {
				uint32_t uiDifficulty = 0;
				apFile->GetChunkData<uint32_t>(uiDifficulty);
				if (uiDifficulty < static_cast<uint32_t>(SCL_COUNT)) {
					SetDifficultyLevel(static_cast<SPEECH_CHALLENGE_LEVEL>(uiDifficulty));
				}
				if (eDifficulty == SCL_NONE) {
					eDifficulty = SCL_VERYEASY;
				}
			} else {
				if (chunkid > TCLF_ID) {
					if (chunkid == QSTI_ID) {
						apFile->GetChunkData<uint32_t>(questID);
						TESForm::AddCompileIndex(questID, apFile);
					} else {
						if (chunkid == ANAM_ID) {
							uint32_t iid = 0;
							apFile->GetChunkData<uint32_t>(iid);
							pSpeaker = reinterpret_cast<TESActorBase*>(static_cast<uintptr_t>(iid));
						}
					}
				} else {
					switch (chunkid) {
					case TCLF_ID: {
						if (!GetConversationData()) {
							LoadConversation(0);
						}
						uint32_t topicID_3 = 0;
						apFile->GetChunkData<uint32_t>(topicID_3);
						TESTopic* pLinkFrom = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(topicID_3));
						(&GetConversationData()->m_listLinkFrom)->AddTail(pLinkFrom);
						break;
					}
					case CIDA_ID: {
						(&objConditions)->Load(apFile);
						break;
					}
					case DATA_ID: {
						apFile->GetChunkData(&m_Data, sizeof(TOPIC_INFO_DATA));
						if (m_Data.type >= 8) {
							SetDelete(1);
						}
						if (m_Data.type == 1) {
							LoadConversation(0);
						}
						break;
					}
					case NAME_ID: {
						uint32_t topicID = 0;
						apFile->GetChunkData<uint32_t>(topicID);
						TESTopic* pAddTopic = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(topicID));
						(&m_listAddTopics)->AddTail(pAddTopic);
						break;
					}
					}
				}
			}
		}
	} while (apFile->NextChunk());
	if (m_Data.type == 3) {
		m_Data.type = 0;
	}
	if (!GetDelete() &&
		!TESTopic::pLastTopic->InsertInfo(reinterpret_cast<TESQuest*>(static_cast<uintptr_t>(questID)), this,
			iPrevInfoID)) {
		uint32_t v5 = questID;
		int32_t CastingType = TESTopic::pLastTopic->iFormID;
		const char* v3 = TESTopic::pLastTopic->GetFormEditorID();
		Warning("MASTERFILE: Unable to insert topic info (%08X) into topic '%s' (%08X), quest (%08X).", iFormID, v3,
			CastingType, v5);
	}
	return 1;
}

// Within a GRUP: a top group saves as any form; a topic children group saves before this info when its topic
// does; the other group types never.
bool TESTopicInfo::SavesBefore(FORM* apGroupFORM) {
	bool bretval = false;
	if (apGroupFORM && apGroupFORM->form == formEnumString[2].uiFormString) {
		switch (apGroupFORM->iFormID) {
		case 0:
			bretval = TESForm::SavesBefore(apGroupFORM);
			break;
		case 7: {
			TESForm* pForm = TESForm::GetFormByNumericID(apGroupFORM->flags);
			TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
			if (pTopic) {
				bretval = SavesBefore(pTopic);
			}
			break;
		}
		default:
			break;
		}
	}
	return bretval;
}

bool TESTopicInfo::SavesBefore(TESForm* apForm) {
	bool bretval = 0;
	int32_t FormType = apForm->GetFormType();
	if (FormType == 69) {
		TESTopic* pothertopic_2 = dynamic_cast<TESTopic*>(apForm);
		if (pothertopic_2) {
			TESTopic* pourtopic_2 = TESTopic::GetOwnerTopic(this);
			if (pourtopic_2) {
				if (pourtopic_2 != pothertopic_2) {
					return pourtopic_2->SavesBefore(pothertopic_2);
				}
			}
		}
	} else {
		if (FormType == 70) {
			TESTopicInfo* pinfo = dynamic_cast<TESTopicInfo*>(apForm);
			if (pinfo) {
				TESTopic* pourtopic = TESTopic::GetOwnerTopic(this);
				TESTopic* pothertopic = TESTopic::GetOwnerTopic(pinfo);
				if (pothertopic) {
					if (pourtopic) {
						if (pothertopic != pourtopic) {
							return pourtopic->SavesBefore(pothertopic);
						}
					}
				}
			}
		} else {
			return TESForm::SavesBefore(apForm);
		}
	}
	return bretval;
}

// The same for the new save system, reading the change flags from the load buffer.
void TESTopicInfo::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESForm::LoadGame(apLoadGameBuffer);
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_TOPIC_SAIDONCE)) {
		bSaidOnce = true;
	}
}

// Loads the form's changes from an old-format save; a changed said-once flag means the info was said.
void TESTopicInfo::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	TESForm::LoadGame(aiFlags, aiCurrentFlags);
	if (aiFlags & CHANGE_TOPIC_SAIDONCE) {
		bSaidOnce = true;
	}
}

// Reverts the form; an info whose said-once flag had changed is unsaid again.
void TESTopicInfo::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESForm::Revert(apLoadGameBuffer);
	if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_TOPIC_SAIDONCE)) {
		bSaidOnce = false;
	}
}

void TESTopicInfo::Revert(uint32_t aiFlags) {
	if ((aiFlags & CHANGE_TOPIC_SAIDONCE) != 0) {
		bSaidOnce = 0;
	}
}

// Resolves the info's form IDs once: the add-topics list (dropping, with a warning, the IDs that are not topics), the
// speaker, the conditions, the conversation data, the shared response list and the perk or actor value. The
// reinterpret_casts move form IDs out of pointer slots, which hold untyped IDs until here.
void TESTopicInfo::InitItem() {
	if (GetInitialized()) {
		return;
	}
	TESFile* pFile = GetFile(-1);
	BSSimpleList<TESTopic*>* pPrev = nullptr;
	BSSimpleList<TESTopic*>* pNode = GetAddTopicList();
	while (pNode && pNode->GetItem()) {
		uint32_t uiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pNode->GetItem()));
		TESForm::AddCompileIndex(uiID, pFile);
		TESTopic* pTopic = dynamic_cast<TESTopic*>(TESForm::GetFormByNumericID(uiID));
		if (pTopic) {
			pNode->SetItem(pTopic);
			pPrev = pNode;
			pNode = pNode->GetNext();
		} else {
			if (pNode->GetNext() || !pPrev) {
				pNode->RemoveHead();
			} else {
				TESTopic* pRemoveID = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(uiID));
				pPrev->Remove(pRemoveID);
				pNode = pPrev->GetNext();
			}
			TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(this);
			TESQuest* pOwnerQuest = nullptr;
			if (pOwnerTopic) {
				pOwnerQuest = pOwnerTopic->GetOwnerQuest(this);
			}
			char acInfoText[0x104];
			alignas(4) uint8_t aucString[sizeof(BSStringT<char>)];
			BSStringT<char>* pString = ::new (aucString) BSStringT<char>;
			GetInfoDisplayText(*pString, true);
			if (pOwnerTopic && pOwnerQuest) {
				uint32_t uiTopicID = pOwnerTopic->GetFormID();
				const char* pTopicName = pOwnerTopic->GetFormEditorID();
				uint32_t uiQuestID = pOwnerQuest->GetFormID();
				const char* pQuestName = pOwnerQuest->GetEditorName();
				const char* pText = pString->String();
				std::sprintf(acInfoText, "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)", GetFormID(), pText,
					pQuestName, uiQuestID, pTopicName, uiTopicID);
			} else {
				const char* pText = pString->String();
				std::sprintf(acInfoText, "TopicInfo %08X \"%s\"", GetFormID(), pText);
			}
			Warning("MASTERFILE: Unable to find topic (%08X) for %s", uiID, acInfoText);
			pString->~BSStringT<char>();
		}
	}

	// pSpeaker holds the speaker's form ID until here.
	uint32_t uiSpeakerID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetSpeaker()));
	if (uiSpeakerID) {
		TESForm::AddCompileIndex(uiSpeakerID, GetFile(-1));
		SetSpeaker(dynamic_cast<TESActorBase*>(TESForm::GetFormByNumericID(uiSpeakerID)));
	}
	if (GetInfoConditions()) {
		GetInfoConditions()->InitItem(this);
	}
	if (GetConversationData()) {
		GetConversationData()->InitItem(this);
	}
	for (TESResponse* pResponse = kResponseList.QResponseList(); pResponse; pResponse = pResponse->QNext()) {
		pResponse->InitItem(this);
	}
	if (pPerkSkillStat) {
		uint32_t uiStatID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPerkSkillStat));
		TESForm::AddCompileIndex(uiStatID, GetFile(-1));
		TESForm* pStat = TESForm::GetFormByNumericID(uiStatID);
		TESForm* pPerk = static_cast<TESForm*>(FNVD_DYNAMIC_CAST_VOID(BGSPerk)(pStat, RTTI_TESForm, RTTI_BGSPerk));
		if (pPerk) {
			pPerkSkillStat = pPerk;
		} else {
			pPerkSkillStat =
				static_cast<TESForm*>(FNVD_DYNAMIC_CAST_VOID(ActorValueInfo)(pStat, RTTI_TESForm, RTTI_ActorValueInfo));
		}
	}
	SetInitialized(true);
}

void TESTopicInfo::GetFormDetailedString(BSStringT<char>& aString) {
	char v7[264];
	char v8[264];
	char v9[268];
	v8[0] = 0;
	TESTopic* ptopic = TESTopic::GetOwnerTopic(this);
	if (ptopic) {
		int32_t CastingType = ptopic->iFormID;
		std::sprintf(v8, ", Topic '%s' (%08X)", ptopic->GetFormEditorID(), CastingType);
	}
	v7[0] = 0;
	if (ptopic) {
		if (GetInitialized()) {
			TESQuest* pquest = ptopic->GetOwnerQuest(this);
			if (pquest) {
				int32_t v4 = pquest->iFormID;
				std::sprintf(v7, "Quest '%s' (%08X)", pquest->GetEditorName(), v4);
			}
		}
	}
	v9[0] = 0;
	TESResponse* presponse = GetResponseList(0)->pHead;
	if (presponse) {
		std::sprintf(v9, ", Text: \"%s\"", presponse->GetResponseText());
	}
	int32_t v2 = iFormID;
	aString.SPrintF("%s Form '%s' (%08X): %s%s%s", GetFormTypeString(), TESForm::GetFormEditorID(), v2, v7, v8, v9);
}

// Altering a loaded, non-temporary info also marks its topic altered before the form itself.
void TESTopicInfo::SetAltered(bool abVal) {
	if (!GetTemporary() && abVal && GetInitialized()) {
		TESTopic* pTopic = TESTopic::GetOwnerTopic(this);
		if (pTopic)
			pTopic->SetAltered(true);
	}
	TESForm::SetAltered(abVal);
}

void TESTopicInfo::Copy(TESForm* apCopy) {
	TESTopicInfo* pInfoToCopy = dynamic_cast<TESTopicInfo*>(apCopy);
	if (pInfoToCopy) {
		CopyAllComponents(apCopy);
		m_Data = *pInfoToCopy->GetData();
		ClearAddTopicList();
		BSSimpleList<TESTopic*>* pTopicList = &pInfoToCopy->m_listAddTopics;
		while (pTopicList && pTopicList->m_item) {
			TESTopic* pTopic = pTopicList->m_item;
			pTopicList = pTopicList->m_pkNext;
			(&m_listAddTopics)->AddTail(pTopic);
		}
		ClearResponseList();
		for (TESResponse* presponse = pInfoToCopy->GetResponseList(0)->pHead; presponse; presponse = presponse->pNext) {
			TESResponse* pNewResponse;
			TESResponse* apMemory = static_cast<TESResponse*>(ourMalloc(sizeof(TESResponse)));
			if (apMemory) {
				pNewResponse = ::new (apMemory) TESResponse;
			} else {
				pNewResponse = 0;
			}
			pNewResponse->Copy(presponse);
			ResponseListWrapper* ResponseList = GetResponseList(0);
			ResponseList->AddTail(pNewResponse);
		}
		SetPrompt((&pInfoToCopy->cPrompt)->pString);
		if (pInfoToCopy->pPerkSkillStat) {
			int32_t FormType = pInfoToCopy->pPerkSkillStat->GetFormType();
			if (FormType == 86 || FormType == 89) {
				pPerkSkillStat = pInfoToCopy->pPerkSkillStat;
			} else {
				pPerkSkillStat = pInfoToCopy->pPerkSkillStat;
			}
		} else {
			pPerkSkillStat = 0;
		}
		eDifficulty = pInfoToCopy->eDifficulty;
		pSpeaker = pInfoToCopy->GetSpeaker();
		Script* ResultScript = pInfoToCopy->GetResultScript(TIRS_BEGIN);
		Script* v3 = GetResultScript(TIRS_BEGIN);
		v3->CopyResultScript(ResultScript);
		Script* v8 = pInfoToCopy->GetResultScript(TIRS_END);
		Script* v4 = GetResultScript(TIRS_END);
		v4->CopyResultScript(v8);
		uint16_t InfoIndex = pInfoToCopy->GetInfoIndex();
		SetInfoIndex(InfoIndex);
		SetConditions(&pInfoToCopy->objConditions);
		if (pInfoToCopy->GetConversationData()) {
			LoadConversation(pInfoToCopy->GetConversationData());
		}
		pOwnerQuest = pInfoToCopy->pOwnerQuest;
		bool Initialized = pInfoToCopy->GetInitialized();
		SetInitialized(Initialized);
	}
}

bool TESTopicInfo::BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) {
	bool v6 = 0;
	if (apGroupFORM) {
		if (apGroupFORM->form == formEnumString[2].uiFormString) {
			TESTopic* OwnerTopic = TESTopic::GetOwnerTopic(this);
			if (OwnerTopic) {
				if (abAllowParentGroups) {
					v6 = OwnerTopic->BelongsInGroup(apGroupFORM, abAllowParentGroups, abCurrentOnly);
				}
				if (!v6 && apGroupFORM->iFormID == 7) {
					return OwnerTopic->FormIDMatchesWithoutIndex(apGroupFORM->flags);
				}
			}
		}
	}
	return v6;
}

void TESTopicInfo::CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup) {
	if (apOutGroupFORM) {
		apOutGroupFORM->form = 0;
		if (apParentGroup) {
			if (!apParentGroup->GroupData.iFormID &&
				apParentGroup->GroupData.flags == formEnumString[69].uiFormString) {
				TESTopic* pparenttopic = TESTopic::GetOwnerTopic(this);
				if (pparenttopic) {
					apOutGroupFORM->form = formEnumString[2].uiFormString;
					apOutGroupFORM->iFormID = 7;
					apOutGroupFORM->flags = pparenttopic->iFormID;
					apOutGroupFORM->length = 0;
					apOutGroupFORM->iVersionControl = 0;
				}
			}
		}
	}
}

void TESTopicInfo::ResetSaidOnceFlags() {
	bSaidOnce = false;
	RemoveChange(CHANGE_TOPIC_SAIDONCE);
}

void TESTopicInfo::SetSaidOnce() {
	if (!IsInfoRefusal()) {
		bSaidOnce = true;
		AddChange(CHANGE_TOPIC_SAIDONCE);
	}
}

void TESTopicInfo::SetTempDataOffset(uint32_t aiOffset) {
	iFileOffset = aiOffset;
}

TOPIC_INFO_DATA* TESTopicInfo::GetData() {
	return &m_Data;
}

// The result scripts are read on demand from the info's file at iFileOffset; NEXT moves from the begin script to the
// end script. Both shared scripts are first reset from an empty stack script. An empty SCRO/SCRV chunk stores 0 into
// the new reference.
Script* TESTopicInfo::GetResultScript(TOPIC_INFO_RESULT_SCRIPT aeScript) {
	int32_t iScript = aeScript;
	if (this == pCurrentResultScriptInfo)
		return &pResultScriptA[iScript];
	pCurrentResultScriptInfo = this;
	alignas(Script) uint8_t aucTempScript[sizeof(Script)];
	Script* pTempScript = static_cast<Script*>(static_cast<void*>(aucTempScript));
	::new (pTempScript) Script;
	pResultScriptA[TIRS_BEGIN].CopyResultScript(pTempScript);
	pResultScriptA[TIRS_BEGIN].SetTemporary();
	pResultScriptA[TIRS_BEGIN].GetHeader()->bIsQuestScript = false;
	pResultScriptA[TIRS_END].CopyResultScript(pTempScript);
	pResultScriptA[TIRS_END].SetTemporary();
	pResultScriptA[TIRS_END].GetHeader()->bIsQuestScript = false;
	if (!GetEmpty()) {
		Script* pResult = &pResultScriptA[iScript];
		pTempScript->~Script();
		return pResult;
	}
	pResultScriptA[TIRS_BEGIN].SetInitialized(false);
	pResultScriptA[TIRS_END].SetInitialized(false);
	TESFile* pFile = GetFile(-1);
	if (!pFile || !iFileOffset) {
		Script* pResult = &pResultScriptA[iScript];
		pTempScript->~Script();
		return pResult;
	}
	pFile = pFile->GetThreadSafeFile();
	if (!pFile->SetOffset(iFileOffset)) {
		Script* pResult = &pResultScriptA[iScript];
		pTempScript->~Script();
		return pResult;
	}
	uint8_t formid = static_cast<uint8_t>(pFile->GetTESForm());
	if (formid != INFO_ID) {
		Script* pResult = &pResultScriptA[iScript];
		pTempScript->~Script();
		return pResult;
	}
	int32_t iloadscript = 0;
	do {
		uint32_t chunkid = pFile->GetTESChunk();
		if (chunkid == NO_CHUNK)
			break;
		switch (chunkid) {
		case NEXT_ID:
			++iloadscript;
			break;
		case SLSD_ID:
		case SCVR_ID:
			if (pFile->GetTESChunk() == SLSD_ID) {
				void* pMemory = ourMalloc(sizeof(ScriptVariable));
				pNewVariable = pMemory ? ::new (pMemory) ScriptVariable : nullptr;
				pNewVariable->Load(pFile);
				pResultScriptA[iloadscript].GetVariableList()->AddTail(pNewVariable);
			} else if (pNewVariable) {
				// A 16-byte aligned stack buffer of the chunk's size.
				char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(pFile->GetChunkSize(), 128));
				pFile->GetChunkData(pBuffer, 0x200);
				pNewVariable->cName.Set(pBuffer, 0);
			}
			break;
		case SCHR_ID:
			pFile->GetChunkData(pResultScriptA[iloadscript].GetHeader(), 0);
			if (pFile->QEndian())
				pResultScriptA[iloadscript].GetHeader()->Endian();
			break;
		case SCDA_ID: {
			uint32_t n = pFile->GetChunkSize();
			// A 16-byte aligned stack buffer of the chunk's size.
			char* tmpdata = static_cast<char*>(__builtin_alloca_with_align(n, 128));
			ourMemset(tmpdata, 0, n);
			pFile->GetChunkData(tmpdata, 0);
			pResultScriptA[iloadscript].SetCompileData(n, tmpdata);
			break;
		}
		case SCRO_ID: {
			void* pMemory = ourMalloc(sizeof(Script::RefVariable));
			Script::RefVariable* pNewItem = pMemory ? ::new (pMemory) Script::RefVariable : nullptr;
			uint32_t tempID = 0;
			pFile->GetChunkData(tempID);
			// The form ID stands in for the form until InitItem resolves it.
			pNewItem->form = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(tempID));
			pResultScriptA[iloadscript].GetReferencedObjectList()->AddTail(pNewItem);
			break;
		}
		case SCRV_ID: {
			void* pMemory = ourMalloc(sizeof(Script::RefVariable));
			Script::RefVariable* pNewItem = pMemory ? ::new (pMemory) Script::RefVariable : nullptr;
			uint32_t tempID = 0;
			pFile->GetChunkData(tempID);
			pNewItem->varIdx = tempID;
			pResultScriptA[iloadscript].GetReferencedObjectList()->AddTail(pNewItem);
			break;
		}
		default:
			break;
		}
	} while (pFile->NextChunk());
	pResultScriptA[TIRS_BEGIN].InitItem(this);
	pResultScriptA[TIRS_END].InitItem(this);
	Script* pResult = &pResultScriptA[iScript];
	pTempScript->~Script();
	return pResult;
}

void TESTopicInfo::SetInfoIndex(uint16_t iIdx) {
	iInfoIndex = iIdx;
}

void TESTopicInfo::SetRandom(bool abVal) {
	if (abVal) {
		m_Data.flags |= 2;
	} else {
		m_Data.flags &= ~2;
	}
}

void TESTopicInfo::ClearResponseList() {
	kResponseList.ClearResponseList();
}

// Removes the info from apTopic's info list.
void TESTopicInfo::RemoveInfoFromTopic(TESTopic* apTopic) {
	if (apTopic) {
		if (GetInfoIndex() != 0xFFFF) {
			QUEST_INFO* pQuestInfo = apTopic->GetQuestInfoElement(this);
			if (pQuestInfo) {
				TopicInfoArray* pInfoArray = static_cast<TopicInfoArray*>(&pQuestInfo->infoArray);
				if (pInfoArray->GetAt(GetInfoIndex()) == this) {
					pInfoArray->RemoveAt(GetInfoIndex());
				} else {
					int32_t isize = pInfoArray->m_uiSize;
					for (int32_t i = 0; i < isize; ++i) {
						if (pInfoArray->GetAt(i) == this) {
							pInfoArray->RemoveAt(i);
							break;
						}
					}
				}
				pInfoArray->Compact();
				TESTopic::SetTopicIndices(pInfoArray);
			}
		}
	}
}

void TESTopicInfo::SetSpeaker(TESActorBase* apActor) {
	pSpeaker = apActor;
}

void TESTopicInfo::SetPrompt(const char* apPrompt) {
	cPrompt.Set(apPrompt, 0);
}

void TESTopicInfo::SetDifficultyLevel(SPEECH_CHALLENGE_LEVEL aeDifficulty) {
	eDifficulty = aeDifficulty;
}

void TESTopicInfo::SetActorValue(ActorValueInfo* apAVI) {
	pPerkSkillStat = apAVI;
}

void TESTopicInfo::SetPerk(BGSPerk* apPerk) {
	pPerkSkillStat = apPerk;
}

ResponseListWrapper* TESTopicInfo::CloneResponseList(ResponseListWrapper* apList) {
	return GetResponseList(apList);
}

bool TESTopicInfo::IsRunImmediately() {
	return ((((uint8_t)m_Data.flags) & 8) != 0);
}

bool TESTopicInfo::IsSayOnce() const {
	return ((((uint8_t)m_Data.flags) & 4) != 0);
}

bool TESTopicInfo::IsGoodbye() const {
	return ((((uint8_t)m_Data.flags) & 1) != 0);
}

bool TESTopicInfo::IsRandomEnd() const {
	return ((((uint8_t)m_Data.flags) & 32) != 0);
}

bool TESTopicInfo::IsSayOnceADay() {
	return ((((uint8_t)m_Data.flagsExt) & 1) != 0);
}

bool TESTopicInfo::IsInfoRefusal() const {
	return ((((uint8_t)m_Data.flags) & 16) != 0);
}

void TESTopicInfo::RunResult(TOPIC_INFO_RESULT_SCRIPT aeScript, TESObjectREFR* apRef) {
	ExtraDataList* extraDataList;
	ScriptLocals* scriptLocals;
	Script* pScript;
	if (apRef) {
		pScript = GetResultScript(aeScript);
		if (Script::GetProcessScripts()) {
			if (pScript) {
				if (pScript->GetHeader()->dataSize) {
					extraDataList = apRef->TESObjectREFR::GetExtra();
					scriptLocals = extraDataList->GetScriptLocals();
					pScript->Run(apRef, scriptLocals, nullptr, 1);
				}
			}
		}
		SetSaidOnce();
		if (IsSayOnceADay() && apRef->IsMobileObject()) {
			static_cast<MobileObject*>(apRef)->AddSayOnceTopicInfo(this);
		}
	}
}

void TESTopicInfo::AddTopicList() {
	BSSimpleList<TESTopic*>* MBRoomAccumList = GetAddTopicList();
	pPlayer->AddTopics(MBRoomAccumList);
}

bool TESTopicInfo::IsSpeechChallenge() const {
	return ((((uint8_t)m_Data.flags) & 128) != 0);
}

bool TESTopicInfo::IsAlwaysDarkened() {
	return ((((uint8_t)m_Data.flagsExt) & 2) != 0);
}

bool TESTopicInfo::IsRunForRumors() {
	return ((((uint8_t)m_Data.flags) & 64) != 0);
}

DUMMY_NODE_STATE TESTopicInfo::GetDummyNodeState() {
	if ((((uint8_t)m_Data.flagsExt) & 16) != 0) {
		return DUM_DUMMY_ONLY;
	} else {
		return static_cast<DUMMY_NODE_STATE>(((uint8_t)((((uint8_t)m_Data.flagsExt) & 32) != 0)) + 2);
	}
}

uint32_t TESTopicInfo::GetSpeechChallengeDifficultyMult() {
	uint32_t result;
	switch ((uint32_t)eDifficulty) {
	case 1u: {
		result = (&::iSpeechChallengeDifficultyVeryEasy)->UInt();
		break;
	}
	case 2u: {
		result = (&::iSpeechChallengeDifficultyEasy)->UInt();
		break;
	}
	case 3u: {
		result = (&::iSpeechChallengeDifficultyAverage)->UInt();
		break;
	}
	case 4u: {
		result = (&::iSpeechChallengeDifficultyHard)->UInt();
		break;
	}
	case 5u: {
		result = (&::iSpeechChallengeDifficultyVeryHard)->UInt();
		break;
	}
	default: {
		result = (uint32_t)((-1));
		break;
	}
	}
	return result;
}

void TESTopicInfo::SetConditions(TESCondition* apCondition) {
	if (apCondition) {
		(&objConditions)->Copy(apCondition, this);
	} else {
		TESCondition kTemp;
		(&objConditions)->Copy(&kTemp, this);
	}
}

void TESTopicInfo::GetInfoDisplayText(BSStringT<char>& aString, bool abFirstOnly) {
	aString.Set(static_cast<const char*>(0), 0u);
	for (TESResponse* plist = GetResponseList(0)->pHead; plist; plist = plist->pNext) {
		aString.operator+=(plist->GetResponseText());
		if (plist->pNext) {
			aString.operator+=(" | ");
		}
		if (abFirstOnly) {
			break;
		}
	}
}

BSSimpleList<TESTopic*>* TESTopicInfo::GetAddTopicList() {
	return &m_listAddTopics;
}

bool TESTopicInfo::HasPrompt() const {
	return ((&cPrompt)->GetLength() != 0);
}

void TESTopicInfo::ClearAddTopicList() {
	for (BSSimpleList<TESTopic*>* pTopicList = &m_listAddTopics; pTopicList && !pTopicList->IsEmpty();
		pTopicList->RemoveHead()) {
		;
	}
}

// The responses are read on demand from the info's file at iFileOffset. Without apList they go to the shared
// kResponseList, which is kept while the same info asks again; with apList the shared list is only copied.
ResponseListWrapper* TESTopicInfo::GetResponseList(ResponseListWrapper* apList) {
	if (this == pCurrentResponsesInfo) {
		if (apList) {
			apList->CopyResponseList(kResponseList.QResponseList());
			return apList;
		}
		return &kResponseList;
	}
	if (!apList) {
		pCurrentResponsesInfo = this;
		ClearResponseList();
	}
	TESFile* pFile = GetFile(-1);
	if (!pFile || !iFileOffset)
		return apList ? apList : &kResponseList;
	pFile = pFile->GetThreadSafeFile();
	if (!pFile->SetOffset(iFileOffset))
		return apList ? apList : &kResponseList;
	uint8_t cFormType = static_cast<uint8_t>(pFile->GetTESForm());
	if (cFormType != INFO_ID)
		return apList ? apList : &kResponseList;
	TESResponse* pResponse = nullptr;
	do {
		uint32_t uiChunk = pFile->GetTESChunk();
		if (uiChunk == NO_CHUNK)
			break;
		switch (uiChunk) {
		case TRDT_ID: {
			void* pMemory = ourMalloc(sizeof(TESResponse));
			pResponse = pMemory ? ::new (pMemory) TESResponse : nullptr;
			pResponse->Load(pFile);
			pResponse->InitItem(this);
			if (apList)
				apList->AddTail(pResponse);
			else
				kResponseList.AddTail(pResponse);
			break;
		}
		case NAM1_ID:
			if (pResponse) {
				uint32_t uiSize = pFile->GetChunkSize();
				// A 16-byte aligned stack buffer of the chunk's size.
				char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
				pFile->GetChunkData(pBuffer, uiSize);
				pResponse->SetResponseText(pBuffer);
			}
			break;
		case SNAM_ID:
			if (pResponse) {
				uint32_t uiIdleID = 0;
				pFile->GetChunkData(uiIdleID);
				TESForm::AddCompileIndex(uiIdleID, pFile);
				TESIdleForm* pIdle = nullptr;
				TESForm* pForm = TESForm::GetFormByNumericID(uiIdleID);
				if (pForm && pForm->GetFormType() == IDLE_ID)
					pIdle = static_cast<TESIdleForm*>(pForm);
				if (uiIdleID && !pIdle)
					Warning("DEFAULT: Unable to find idle (%08X) for info (%08X).", uiIdleID, GetFormID());
				pResponse->SetSpeakerIdle(pIdle);
			}
			break;
		case LNAM_ID:
			if (pResponse) {
				uint32_t uiIdleID = 0;
				pFile->GetChunkData(uiIdleID);
				TESForm::AddCompileIndex(uiIdleID, pFile);
				TESIdleForm* pIdle = nullptr;
				TESForm* pForm = TESForm::GetFormByNumericID(uiIdleID);
				if (pForm && pForm->GetFormType() == IDLE_ID)
					pIdle = static_cast<TESIdleForm*>(pForm);
				if (uiIdleID && !pIdle)
					Warning("DEFAULT: Unable to find idle (%08X) for info (%08X).", uiIdleID, GetFormID());
				pResponse->SetListenerIdle(pIdle);
			}
			break;
		default:
			break;
		}
	} while (pFile->NextChunk());
	return apList ? apList : &kResponseList;
}

void TESTopicInfo::LoadConversation(TESConversationData* apSource) {
	if (apSource) {
		if (!m_pConversationData) {
			TESConversationData* v3;
			TESConversationData* apMemory = static_cast<TESConversationData*>(ourMalloc(sizeof(TESConversationData)));
			if (apMemory) {
				v3 = ::new (apMemory) TESConversationData;
			} else {
				v3 = 0;
			}
			m_pConversationData = v3;
		}
		m_pConversationData->Copy(this, apSource);
	} else {
		TESConversationData* v2;
		if (m_pConversationData) {
			delete m_pConversationData;
			m_pConversationData = 0;
		}
		TESConversationData* v5 = static_cast<TESConversationData*>(ourMalloc(sizeof(TESConversationData)));
		if (v5) {
			v2 = ::new (v5) TESConversationData;
		} else {
			v2 = 0;
		}
		m_pConversationData = v2;
	}
}

bool TESTopicInfo::CheckCondition(bool& abDispositionFailure, TESQuest* apQuest, TESObjectREFR* apActionRef,
	TESObjectREFR* apTargetRef) {
	int32_t v6;
	if (GetDelete()) {
		return 0;
	}
	if (IsSayOnce() && bSaidOnce) {
		return 0;
	}
	DUMMY_NODE_STATE DummyFlag = GetDummyNodeState();
	Actor* pTargetActor = 0;
	if (apTargetRef) {
		if (apTargetRef->IsActor()) {
			pTargetActor = static_cast<Actor*>(apTargetRef);
		}
	}
	if (pTargetActor &&
		(v6 = pTargetActor->GetActorValue(ActorValue::INTELLIGENCE), v6 <= iDialogueDummySpeakThisIntOrBelow.Int())) {
		if (DummyFlag == DUM_NON_DUMMY_ONLY) {
			return 0;
		}
	} else {
		if (DummyFlag == DUM_DUMMY_ONLY) {
			return 0;
		}
	}
	bool bretval = 0;
	bool bquestconditions = 1;
	if (apQuest) {
		bquestconditions = (&apQuest->objConditions)->IsTrueFull(apActionRef, apTargetRef, &abDispositionFailure, 0);
	}
	if (bquestconditions) {
		return (&objConditions)->IsTrueFull(apActionRef, apTargetRef, &abDispositionFailure, 0);
	}
	return bretval;
}

void TESTopicInfo::AddTopicInfoToSortedList(TESTopicInfo* apTopicInfo, BSSimpleList<TESTopicInfo*>* apTopicInfoList) {
	if (apTopicInfo && apTopicInfoList) {
		if (apTopicInfoList->IsEmpty()) {
			apTopicInfoList->AddHead(apTopicInfo);
		} else {
			for (BSSimpleList<TESTopicInfo*>* plist = apTopicInfoList; plist; plist = plist->m_pkNext) {
				if (plist->IsEmpty()) {
					break;
				}
				TESTopicInfo* pInfo = plist->m_item;
				if (pInfo == apTopicInfo) {
					break;
				}
				if (CompareTopicInfos(pInfo, apTopicInfo)) {
					plist->AddHead(apTopicInfo);
					return;
				}
				if (!plist->m_pkNext) {
					plist->AddTail(apTopicInfo);
					return;
				}
			}
		}
	}
}

bool TESTopicInfo::CompareTopicInfos(TESTopicInfo* apTopicInfo1, TESTopicInfo* apTopicInfo2) {
	return apTopicInfo1 < apTopicInfo2;
}

ResponseListWrapper::ResponseListWrapper() : pHead(nullptr) {}

ResponseListWrapper::~ResponseListWrapper() {
	ClearResponseList();
}

void ResponseListWrapper::AddTail(TESResponse* apRes) {
	if (!pHead)
		pHead = apRes;
	else
		pHead->AddTail(apRes);
}

int32_t ResponseListWrapper::GetResponseID() {
	if (pHead) {
		return pHead->GetListCount();
	} else {
		return 0;
	}
}

void ResponseListWrapper::ClearResponseList() {
	TESResponse* pNext;
	for (TESResponse* pResponse = pHead; pResponse; pResponse = pNext) {
		pNext = pResponse->pNext;
		if (pResponse) {
			delete pResponse;
		}
	}
	pHead = 0;
}

void ResponseListWrapper::CopyResponseList(TESResponse* apHead) {
	ClearResponseList();
	TESResponse* pprev = 0;
	while (apHead) {
		TESResponse* pnewResponse;
		TESResponse* apMemory = static_cast<TESResponse*>(ourMalloc(sizeof(TESResponse)));
		if (apMemory) {
			pnewResponse = ::new (apMemory) TESResponse;
		} else {
			pnewResponse = 0;
		}
		pnewResponse->Copy(apHead);
		if (pprev) {
			pprev->pNext = pnewResponse;
		} else {
			AddTail(pnewResponse);
		}
		pprev = pnewResponse;
		apHead = apHead->pNext;
	}
}

TESResponse* ResponseListWrapper::QResponseList() {
	return pHead;
}

TESConversationData::TESConversationData() {}

TESConversationData::~TESConversationData() {
	ClearLists();
}

// The three lists hold form IDs until here; each is resolved against apOwner's file, and an ID that is not a form of
// the list's type is dropped with a warning naming the owning info. The reinterpret_casts move a form ID in and out of
// an item slot, which holds an untyped value until here.
void TESConversationData::InitItem(TESForm* apOwner) {
	BSSimpleList<TESTopic*>* pPrevTopic = nullptr;
	BSSimpleList<TESTopic*>* pTopicNode = GetLinkToList();
	TESFile* pFile = apOwner ? apOwner->GetFile(-1) : nullptr;
	while (pTopicNode && pTopicNode->GetItem()) {
		uint32_t uiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTopicNode->GetItem()));
		TESForm::AddCompileIndex(uiID, pFile);
		TESTopic* pTopic = nullptr;
		TESForm* pForm = TESForm::GetFormByNumericID(uiID);
		if (pForm && pForm->GetFormType() == DIAL_ID) {
			pTopic = static_cast<TESTopic*>(pForm);
		}
		if (pTopic) {
			pTopicNode->SetItem(pTopic);
			pPrevTopic = pTopicNode;
			pTopicNode = pTopicNode->GetNext();
		} else {
			if (pTopicNode->GetNext() || !pPrevTopic) {
				pTopicNode->RemoveHead();
			} else {
				TESTopic* pRemoveID = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(uiID));
				pPrevTopic->Remove(pRemoveID);
				pTopicNode = pPrevTopic->GetNext();
			}
			TESTopicInfo* pInfo = nullptr;
			if (apOwner && apOwner->GetFormType() == INFO_ID) {
				pInfo = static_cast<TESTopicInfo*>(apOwner);
			}
			TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(pInfo);
			TESQuest* pOwnerQuest = nullptr;
			if (pOwnerTopic) {
				pOwnerQuest = pOwnerTopic->GetOwnerQuest(pInfo);
			}
			char acInfoText[0x400];
			alignas(4) uint8_t aucString[sizeof(BSStringT<char>)];
			BSStringT<char>* pString = ::new (aucString) BSStringT<char>;
			pInfo->GetInfoDisplayText(*pString, true);
			if (pOwnerTopic && pOwnerQuest) {
				uint32_t uiTopicID = pOwnerTopic->GetFormID();
				const char* pTopicName = pOwnerTopic->GetFormEditorID();
				uint32_t uiQuestID = pOwnerQuest->GetFormID();
				const char* pQuestName = pOwnerQuest->GetEditorName();
				const char* pText = pString->String();
				std::sprintf(acInfoText, "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)", pInfo->GetFormID(),
					pText, pQuestName, uiQuestID, pTopicName, uiTopicID);
			} else {
				const char* pText = pString->String();
				std::sprintf(acInfoText, "TopicInfo %08X \"%s\"", pInfo->GetFormID(), pText);
			}
			Warning("MASTERFILE: Unable to find topic (%08X) for conversation data 'Link To' on %s", uiID, acInfoText);
			pString->~BSStringT<char>();
		}
	}

	pPrevTopic = nullptr;
	pTopicNode = GetLinkFromList();
	while (pTopicNode && pTopicNode->GetItem()) {
		uint32_t uiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTopicNode->GetItem()));
		TESForm::AddCompileIndex(uiID, pFile);
		TESTopic* pTopic = nullptr;
		TESForm* pForm = TESForm::GetFormByNumericID(uiID);
		if (pForm && pForm->GetFormType() == DIAL_ID) {
			pTopic = static_cast<TESTopic*>(pForm);
		}
		if (pTopic) {
			pTopicNode->SetItem(pTopic);
			pPrevTopic = pTopicNode;
			pTopicNode = pTopicNode->GetNext();
		} else {
			if (pTopicNode->GetNext() || !pPrevTopic) {
				pTopicNode->RemoveHead();
			} else {
				TESTopic* pRemoveID = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(uiID));
				pPrevTopic->Remove(pRemoveID);
				pTopicNode = pPrevTopic->GetNext();
			}
			TESTopicInfo* pInfo = nullptr;
			if (apOwner && apOwner->GetFormType() == INFO_ID) {
				pInfo = static_cast<TESTopicInfo*>(apOwner);
			}
			TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(pInfo);
			TESQuest* pOwnerQuest = nullptr;
			if (pOwnerTopic) {
				pOwnerQuest = pOwnerTopic->GetOwnerQuest(pInfo);
			}
			char acInfoText[0x400];
			alignas(4) uint8_t aucString[sizeof(BSStringT<char>)];
			BSStringT<char>* pString = ::new (aucString) BSStringT<char>;
			pInfo->GetInfoDisplayText(*pString, true);
			if (pOwnerTopic && pOwnerQuest) {
				uint32_t uiTopicID = pOwnerTopic->GetFormID();
				const char* pTopicName = pOwnerTopic->GetFormEditorID();
				uint32_t uiQuestID = pOwnerQuest->GetFormID();
				const char* pQuestName = pOwnerQuest->GetEditorName();
				const char* pText = pString->String();
				std::sprintf(acInfoText, "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)", pInfo->GetFormID(),
					pText, pQuestName, uiQuestID, pTopicName, uiTopicID);
			} else {
				const char* pText = pString->String();
				std::sprintf(acInfoText, "TopicInfo %08X \"%s\"", pInfo->GetFormID(), pText);
			}
			Warning("MASTERFILE: Unable to find topic (%08X) for conversation data 'Link From' on %s", uiID,
				acInfoText);
			pString->~BSStringT<char>();
		}
	}

	BSSimpleList<TESTopicInfo*>* pInfoNode = GetFollowUpInfoList();
	BSSimpleList<TESTopicInfo*>* pPrevInfo = nullptr;
	while (pInfoNode && pInfoNode->GetItem()) {
		uint32_t uiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pInfoNode->GetItem()));
		TESForm::AddCompileIndex(uiID, pFile);
		TESTopicInfo* pFollowUp = nullptr;
		TESForm* pForm = TESForm::GetFormByNumericID(uiID);
		if (pForm && pForm->GetFormType() == INFO_ID) {
			pFollowUp = static_cast<TESTopicInfo*>(pForm);
		}
		if (pFollowUp) {
			pInfoNode->SetItem(pFollowUp);
			pPrevInfo = pInfoNode;
			pInfoNode = pInfoNode->GetNext();
		} else {
			if (pInfoNode->GetNext() || !pPrevInfo) {
				pInfoNode->RemoveHead();
			} else {
				TESTopicInfo* pRemoveID = reinterpret_cast<TESTopicInfo*>(static_cast<uintptr_t>(uiID));
				pPrevInfo->Remove(pRemoveID);
				pInfoNode = pPrevInfo->GetNext();
			}
			TESTopicInfo* pInfo = nullptr;
			if (apOwner && apOwner->GetFormType() == INFO_ID) {
				pInfo = static_cast<TESTopicInfo*>(apOwner);
			}
			TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(pInfo);
			TESQuest* pOwnerQuest = nullptr;
			if (pOwnerTopic) {
				pOwnerQuest = pOwnerTopic->GetOwnerQuest(pInfo);
			}
			char acInfoText[0x400];
			alignas(4) uint8_t aucString[sizeof(BSStringT<char>)];
			BSStringT<char>* pString = ::new (aucString) BSStringT<char>;
			pInfo->GetInfoDisplayText(*pString, true);
			if (pOwnerTopic && pOwnerQuest) {
				uint32_t uiTopicID = pOwnerTopic->GetFormID();
				const char* pTopicName = pOwnerTopic->GetFormEditorID();
				uint32_t uiQuestID = pOwnerQuest->GetFormID();
				const char* pQuestName = pOwnerQuest->GetEditorName();
				const char* pText = pString->String();
				std::sprintf(acInfoText, "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)", pInfo->GetFormID(),
					pText, pQuestName, uiQuestID, pTopicName, uiTopicID);
			} else {
				const char* pText = pString->String();
				std::sprintf(acInfoText, "TopicInfo %08X \"%s\"", pInfo->GetFormID(), pText);
			}
			Warning("MASTERFILE: Unable to find topic (%08X) for conversation data 'Follow Up' on %s", uiID,
				acInfoText);
			pString->~BSStringT<char>();
		}
	}
}

BSSimpleList<TESTopicInfo*>* TESConversationData::GetFollowUpInfoList() {
	return &m_listFollowUpInfos;
}

void TESConversationData::ClearLists() {
	(&m_listLinkTo)->RemoveAll();
	(&m_listLinkFrom)->RemoveAll();
	(&m_listFollowUpInfos)->RemoveAll();
}

void TESConversationData::Copy(TESTopicInfo* apOwner, TESConversationData* apDataToCopy) {
	if (apDataToCopy) {
		ClearLists();
		BSSimpleList<TESTopic*>* pTopicList = &apDataToCopy->m_listLinkTo;
		TESTopic* pTopic = nullptr;
		while (pTopicList && pTopicList->m_item) {
			pTopic = pTopicList->m_item;
			pTopicList = pTopicList->m_pkNext;
			m_listLinkTo.AddTail(pTopic);
		}
		BSSimpleList<TESTopic*>* pTopicList_2 = &apDataToCopy->m_listLinkFrom;
		pTopic = nullptr;
		while (pTopicList_2 && pTopicList_2->m_item) {
			pTopic = pTopicList_2->m_item;
			pTopicList_2 = pTopicList_2->m_pkNext;
			m_listLinkFrom.AddTail(pTopic);
		}
		BSSimpleList<TESTopicInfo*>* pTopicInfoList = apDataToCopy->GetFollowUpInfoList();
		while (pTopicInfoList) {
			TESTopicInfo* pTopicInfo = pTopicInfoList->m_item;
			if (!pTopicInfo) {
				break;
			}
			pTopicInfoList = pTopicInfoList->m_pkNext;
			GetFollowUpInfoList()->AddTail(pTopicInfo);
		}
	}
}

void TESResponse::SetResponseText(const char* apResponseText) {
	cResponseText.Set(apResponseText, 0);
}
