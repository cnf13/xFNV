#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"

#include <stdint.h>

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class DialogueResponse;
class MenuTopic;
class TESObjectREFR;
class TESQuest;
class TESTopic;
class TESTopicInfo;

class MenuTopic {
public:
	MenuTopic();
	MenuTopic(TESQuest* apQuest, TESTopic* apTopic, TESTopicInfo* apInfo, TESObjectREFR* apSpeaker, bool abInfoRefusal);
	~MenuTopic();

	// Whether the response list is empty.
	bool GetResponseListEmpty();
	// Fills the response list of the menu topic's own quest, topic and info for apSpeaker.
	void FillResponseList(TESObjectREFR* apSpeaker);
	// Fills the response list of apTopic's apInfo for apSpeaker in apQuest.
	void FillResponseList(TESQuest* apQuest, TESTopic* apTopic, TESTopicInfo* apInfo, TESObjectREFR* apSpeaker);
	// Returns the topic's info (pInfo).
	TESTopicInfo* GetInfo();
	// Returns the topic (pTopic).
	TESTopic* GetTopic();
	// Starts the response walk at lResponses; whether that node holds a response.
	bool FirstResponse();
	// Steps the response walk; whether the new node holds a response.
	bool NextResponse();
	// The response at the walk's node, or null past its end.
	DialogueResponse* GetCurrentResponse();
	// The topic text (cTopicText.pString).
	const char* GetTopicText();
	// Reads the topic back from an old-format save (TESSaveLoadGame): its text, flags, quest, topic and info, with
	// the block header and size checks when the save uses blocks. apSpeaker is not read.
	void LoadGame(TESObjectREFR* apSpeaker);
	uint16_t GetSaveSize();
	void SaveGame();
	TESTopicInfo* LoadGame_0083FBE0(BGSLoadGameBuffer* a1);
	void SaveGame_0(BGSSaveGameBuffer* apSaveGameBuffer);

	static void operator delete(void* apMemory);

	BSStringT<char> cTopicText;
	bool bTopicIsChoice;
	bool bInfoRefusal;
	bool bPersuasion;
	uint8_t padding0B[1];
	BSSimpleList<DialogueResponse*> lResponses;
	TESQuest* pQuest;
	TESTopicInfo* pInfo;
	TESTopic* pOrigTopic;
	BSSimpleList<DialogueResponse*>* pCurrentResponse;
	bool bGeneralTopic;
	bool bTopicIsNew;
	uint8_t padding26[2];
	TESTopic* pTopic;
};

class MenuTopicManager {
public:
	MenuTopicManager();
	~MenuTopicManager();

	// Makes the first topic of the current list current, skipping one when abSkipGreeting is set; false when there
	// is none.
	bool FirstTopic(bool abSkipGreeting);
	// The topic list on top of listTopicLists.
	BSSimpleList<MenuTopic*>* GetCurrentTopicList();
	// The topic being shown, or null.
	MenuTopic* GetCurrentTopic();
	// Runs the begin result script of the current topic's info for the speaker, unless the info runs for rumors.
	void RunBeginScript();
	// Pushes apTopicList onto listTopicLists; a null list is ignored.
	void PushTopicList(BSSimpleList<MenuTopic*>* apList);
	void ClearData();
	bool GoToTopic(int32_t aiIndex);
	MenuTopic* GetGreeting();
	bool NextTopic();
	void ClearTopicList(BSSimpleList<MenuTopic*>* apList, bool abClearGreeting);
	bool Initialize(TESObjectREFR* apSpeaker, TESTopic* apForceGreeting);
	bool DoSpeechChallengeCheck(MenuTopic* pSpeechTopic);
	void Update();
	void FillTopicList(BSSimpleList<TESTopic*>* apList, bool abPersuasion, bool abIsChoiceList,
		TESTopic* apPreviousTopic);
	void* PopTopicList();

	// Destroys and frees the manager singleton and clears it.
	static void Destroy();
	static MenuTopicManager* Instance();
	static void operator delete(void* apMemory);
	static int32_t PrioritySort(MenuTopic* apTopic1, MenuTopic* apTopic2);

	// A node of one of listTopicLists' lists.
	BSSimpleList<MenuTopic*>* pCurrentTopic;
	BSSimpleList<BSSimpleList<MenuTopic*>*> listTopicLists;
	// Not always an actor: IsActor is tested before the disposition calls.
	TESObjectREFR* pSpeaker;
	bool bSpeechChallengeLoss;
	uint8_t padding11[3];
	uint32_t iLastDialogXP;

	static inline MenuTopicManager* pInstance;
};
