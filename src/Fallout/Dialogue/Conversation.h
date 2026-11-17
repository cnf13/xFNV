#pragma once

#include "BSCore/BSSimpleList.h"
#include "Reconstruction.hpp"

class BGSLoadFormBuffer;
class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class DialogueItem;
class TESObjectREFR;
class TESTopic;

class Conversation {
public:
	Conversation();
	Conversation(TESObjectREFR* apSpeaker, TESObjectREFR* apListener, TESTopic* apTopic);
	~Conversation();

	// Deletes every queued dialogue item and clears the current one.
	void ClearList();
	// The item of pCurrentItem's list node (thiscall, ret 0, 0x83C820); the exe shares the
	// body with DialogueItem::GetCurrentResponse.
	DialogueItem* GetCurrentItem();
	// Same function as DialogueItem::SetCurrentResponse (0x83C8A0), registered there: points pCurrentItem at the
	// list node holding apDialogueItem (thiscall, ret 4).
	void SetCurrentItem(DialogueItem* apDialogueItem);
	bool GetNext();
	bool FirstItem();
	int16_t GetDialogueItemIndex(DialogueItem* apDialogueItem);
	DialogueItem* GetDialogueItemByIndex(int16_t aiIndex);
	// IDA name; the PDB's is GetSaveSize(). Unsigned: the exe's callers zero-extend ax (DialoguePackage::GetSaveSize
	// 0x9EF60D).
	uint16_t GetSaveSizeTES();
	void SaveGameTES();
	// IDA name; not in the PDB. Void: its only exe caller (DialoguePackage::LoadGame 0x9EFCAE) does not read eax.
	void LoadGameTES();
	void InitLoadGameTES();
	void SaveGameBGS(BGSSaveFormBuffer* a2);
	// Inferred type: arg0: BGSLoadFormBuffer* from 0x836381 (parameter 0 of FalloutRadio::StationState::LoadGame)
	void LoadGameBGS(BGSLoadFormBuffer* arg0);
	// IDA name; the PDB's is InitLoadGame(BGSLoadGameBuffer*). Both exe callers pass their load buffer
	// (FalloutRadio::StationState::InitLoadGame 0x8363A0, DialoguePackage::InitLoadGame 0x9F0740).
	void InitItem(BGSLoadGameBuffer* apLoadGameBuffer);
	// IDA name; the PDB's is PrecacheSounds(): precaches each item's voice files. Void: no exe caller
	// (FalloutRadio::UpdateStation, StartRadioConversation) reads eax after it.
	void PrecacheSounds();

	static void operator delete(void* apMemory);

	// The queued dialogue items and the list node of the current one.
	BSSimpleList<DialogueItem*> lDialogueItems;
	BSSimpleList<DialogueItem*>* pCurrentItem;
};
