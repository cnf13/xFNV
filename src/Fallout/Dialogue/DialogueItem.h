#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Reconstruction.hpp"

class BGSLoadFormBuffer;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class DialogueResponse;
class TESObjectREFR;
class TESQuest;
class TESTopic;

// One info a speaker says: its responses, with the one being said, and the info, topic, quest and speaker.
class DialogueItem {
public:
	DialogueItem();
	DialogueItem(TESQuest* apQuest, TESTopic* apTopic, TESTopicInfo* apTopicInfo, TESObjectREFR* apSpeaker);
	~DialogueItem();

	// Makes the first response current; false when there is none.
	bool FirstResponse();
	// The response being said, or null.
	DialogueResponse* GetCurrentResponse();
	void RunResult(TOPIC_INFO_RESULT_SCRIPT aeScript);
	// The info; the exe calls the shared one-field getter 0x84E3A0 (thiscall, ret 0).
	TESTopicInfo* GetInfo();
	// The topic; the exe calls the shared one-field getter 0x44EDB0 (thiscall, ret 0).
	TESTopic* GetTopic();
	void InitLoadGame();
	// IDA name; the PDB's is InitLoadGame(BGSLoadGameBuffer*). Conversation::InitItem (0x83C420) passes its load
	// buffer on.
	void InitItem(BGSLoadGameBuffer* apLoadGameBuffer);
	bool NextResponse();
	void SetCurrentResponse(DialogueResponse* apDialogueResponse);
	int16_t GetDialogueResponseIndex(DialogueResponse* apDialogueResponse);
	DialogueResponse* GetDialogueResponseByIndex(int16_t aiIndex);
	void SaveGameTES();
	void LoadGame();
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	// IDA name; the PDB's is LoadGame(BGSLoadGameBuffer*), void: no exe caller (Conversation::LoadGameBGS,
	// HighProcess::LoadGame) reads eax after it.
	// Inferred type: arg0: BGSLoadFormBuffer* from 0x8fd7f2 (parameter 0 of HighProcess::LoadGame)
	void LoadGameBGS(BGSLoadFormBuffer* apLoadGameBuffer);
	void PrecacheSounds();
	// The save size in bytes.
	uint16_t GetSaveSize();

	static void operator delete(void* apMemory);

	// 0x0 the responses of the info
	BSSimpleList<DialogueResponse*> lResponses;
	// 0x8 the node of the response being said
	BSSimpleList<DialogueResponse*>* pCurrentResponse;
	// 0xC the info being said
	TESTopicInfo* pInfo;
	// 0x10 its topic
	TESTopic* pTopic;
	// 0x14 its quest
	TESQuest* pQuest;
	// 0x18 who says it (the constructor's TESObjectREFR; InitLoadGame casts the loaded form to TESObjectREFR)
	TESObjectREFR* pSpeaker;
};
