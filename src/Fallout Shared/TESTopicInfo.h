#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESForm.h"
#include "Reconstruction.hpp"

class ActorValueInfo;
class BGSPerk;
enum DIALOGUE_TYPE : int32_t;
class ResponseListWrapper;
class Script;
class TESActorBase;
class TESConversationData;
class TESObjectREFR;
class TESQuest;
class TESTopic;

// Whether an info belongs to the dummy dialogue nodes. Values from the Xbox PDB.
enum DUMMY_NODE_STATE : int32_t {
	DUM_NONE = 0,
	DUM_DUMMY_ONLY = 1,
	DUM_EITHER = 2,
	DUM_NON_DUMMY_ONLY = 3,
};

// An info's DATA: its dialogue type, next speaker and flag bytes.
struct TOPIC_INFO_DATA {
	// read sign-extended by Load (movsx, signed compare with 8 at 0x61DDBA)
	char type;
	uint8_t nextSpeaker;
	// read sign-extended by IsRandom (movsx)
	char flags;
	char flagsExt;
};

// Who speaks an info's next line (TESTopicInfo::GetNextSpeaker). Names from the Xbox PDB; the exe reads the byte
// sign-extended into a dword (0x61B9FA movsx).
enum DIALOGUE_SPEAKER : int32_t {
	DS_TARGET = 0,
	DS_SELF = 1,
	DS_EITHER = 2,
	DIALOGUE_SPEAKER_COUNT = 3,
};

// How hard an info's speech challenge is (GetSpeechChallengeDifficultyMult 0x61F2B0 switches on 1 to 5). Values from
// the Xbox PDB.
enum SPEECH_CHALLENGE_LEVEL : int32_t {
	SCL_NONE = 0,
	SCL_VERYEASY = 1,
	SCL_EASY = 2,
	SCL_AVERAGE = 3,
	SCL_HARD = 4,
	SCL_VERYHARD = 5,
	SCL_COUNT = 6,
};

// Which of an info's result scripts runs: the one at the start of the response or the one at its end.
enum TOPIC_INFO_RESULT_SCRIPT : int32_t {
	TIRS_BEGIN = 0,
	TIRS_END = 1,
	TIRS_COUNT = 2,
};

// One response of a dialogue topic (INFO): its conditions, responses and result scripts.
class TESTopicInfo : public TESForm {
public:
	explicit TESTopicInfo(DIALOGUE_TYPE aeType);
	~TESTopicInfo() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	// the exe shares this body with TESTopic::SavesBefore (0x619510)
	bool SavesBefore(FORM* apGroupFORM) override;
	bool SavesBefore(TESForm* apForm) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Revert(uint32_t aiFlags) override;
	void InitItem() override;
	void GetFormDetailedString(BSStringT<char>& aString) override;
	void SetAltered(bool abVal) override;
	void Copy(TESForm* apCopy) override;
	bool BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) override;
	void CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup) override;

	// Clears the said-once flag and its change flag.
	void ResetSaidOnceFlags();
	// Marks the info as said once unless it is a refusal (change flag 0x80000000).
	void SetSaidOnce();
	// Sets the file offset of the info's data (thiscall, ret 4, 0x589F50; the exe folds it
	// with BGSPlaceableWater::SetWaterType).
	void SetTempDataOffset(uint32_t aiOffset);
	TOPIC_INFO_DATA* GetData();
	// The begin or end result script, read on demand into the shared result scripts.
	Script* GetResultScript(TOPIC_INFO_RESULT_SCRIPT aeScript);
	// Sets the info's index in its quest's info array.
	void SetInfoIndex(uint16_t iIdx);
	// Who speaks next, from m_Data.
	DIALOGUE_SPEAKER GetNextSpeaker();
	// Sets the info's quest (thiscall, ret 4, 0x673400; the exe folds it with
	// TESPackage::SetCombatStyle).
	void SetOwnerQuest(TESQuest* apQuest);
	// The info's index in its quest's info array.
	uint16_t GetInfoIndex();
	// Whether the info's random flag (DATA flags bit 2) is set.
	bool IsRandom();
	// Sets or clears the info's random flag (DATA flags bit 2). The exe tests the whole byte of acVal, so it is a
	// char rather than a bool.
	void SetRandom(bool abVal);
	// Frees the shared response list kResponseList.
	void ClearResponseList();

	// The exe's shared "this + 0x18" getter. The info's conditions.
	TESCondition* GetInfoConditions() { return &objConditions; }

	// Removes the info from apTopic's info list (thiscall, ret 4, 0x61D140). IDA name
	// RemoveTopicInfo.
	void RemoveInfoFromTopic(TESTopic* apTopic);
	// pPerkSkillStat when it is an actor value, else null.
	ActorValueInfo* GetActorValue();
	// pPerkSkillStat when it is a perk, else null.
	BGSPerk* GetPerk();
	void SetSpeaker(TESActorBase* apActor);
	// Copies apPrompt into cPrompt. No return value: its callers (0x61CF9D, 0x61DF8F, 0x61E4A1) do not read eax.
	void SetPrompt(const char* apPrompt);
	// Sets eDifficulty. The exe folds the body with the processes' vtable slot 326; this member owns it.
	void SetDifficultyLevel(SPEECH_CHALLENGE_LEVEL aeDifficulty);
	TESConversationData* GetConversationData();
	// Stores pPerkSkillStat.
	void SetActorValue(ActorValueInfo* apAVI);
	// Stores pPerkSkillStat.
	void SetPerk(BGSPerk* apPerk);
	ResponseListWrapper* CloneResponseList(ResponseListWrapper* apList);
	bool IsRunImmediately();
	bool IsSayOnce() const;
	bool IsGoodbye() const;
	bool IsRandomEnd() const;
	bool IsSayOnceADay();
	// The exe calls it out of line, also from SetSaidOnce in the same file.
	bool IsInfoRefusal() const;
	void RunResult(TOPIC_INFO_RESULT_SCRIPT aeScript, TESObjectREFR* apRef);
	void AddTopicList();
	bool IsSpeechChallenge() const;
	// The second flag byte's always-darkened bit.
	bool IsAlwaysDarkened();
	bool IsRunForRumors();
	DUMMY_NODE_STATE GetDummyNodeState();
	uint32_t GetSpeechChallengeDifficultyMult();
	void SetConditions(TESCondition* apCondition);
	// Writes the info's display text into aString, only the first response when
	// abFirstOnly.
	void GetInfoDisplayText(BSStringT<char>& aString, bool abFirstOnly);
	// &m_listAddTopics, through the exe's shared "this + 0x30" getter (thiscall, ret 0,
	// 0x436AA0).
	BSSimpleList<TESTopic*>* GetAddTopicList();
	// pSpeaker, through the exe's shared one-field getter of +0x3C (thiscall, ret 0,
	// 0x639B40).
	TESActorBase* GetSpeaker();
	bool HasPrompt() const;

protected:
	// Frees the add-topics list (thiscall, 0x61D250). IDA name FreeAddTopicsList.
	void ClearAddTopicList();
	// Reads the info's responses from its file into apList, or into the shared kResponseList when apList is null.
	ResponseListWrapper* GetResponseList(ResponseListWrapper* apList);

public:
	// Copies apSource into the info's conversation data (made when
	// missing): the body passes it to TESConversationData::Copy, and Copy passes another info's
	// GetConversationData().
	void LoadConversation(TESConversationData* apSource);
	bool CheckCondition(bool& abDispositionFailure, TESQuest* apQuest, TESObjectREFR* apActionRef,
		TESObjectREFR* apTargetRef);

	// Inserts apTopicInfo into the sorted apTopicInfoList.
	static void AddTopicInfoToSortedList(TESTopicInfo* apTopicInfo, BSSimpleList<TESTopicInfo*>* apTopicInfoList);
	// Whether apTopicInfo1's address is below apTopicInfo2's (compared unsigned): the order of the sorted lists.
	static bool CompareTopicInfos(TESTopicInfo* apTopicInfo1, TESTopicInfo* apTopicInfo2);

	TESCondition objConditions;
	uint16_t iInfoIndex;
	bool bSaidOnce;
	TOPIC_INFO_DATA m_Data;
	BSStringT<char> cPrompt;
	BSSimpleList<TESTopic*> m_listAddTopics;
	TESConversationData* m_pConversationData;
	TESActorBase* pSpeaker;
	// a BGSPerk or an ActorValueInfo (a form ID until InitItem)
	TESForm* pPerkSkillStat;
	SPEECH_CHALLENGE_LEVEL eDifficulty;
	TESQuest* pOwnerQuest;
	uint32_t iFileOffset;
};
inline constexpr fnvd::Vtable TESTopicInfo_vftable{};

// The game settings TESTopicInfo.cpp defines (PDB), at their exe addresses.
extern SettingT<GameSettingCollection> iSpeechChallengeDifficultyVeryHard;
extern SettingT<GameSettingCollection> iSpeechChallengeDifficultyHard;
extern SettingT<GameSettingCollection> iSpeechChallengeDifficultyEasy;
extern SettingT<GameSettingCollection> iSpeechChallengeDifficultyAverage;
extern SettingT<GameSettingCollection> iSpeechChallengeDifficultyVeryEasy;
