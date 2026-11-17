#pragma once

#include "Fallout Shared/TESFile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class Script;
class TESIdleForm;
class TESTopic;

// The package event an action belongs to.
enum PACK_EVENT_ACTION_TYPE : int32_t {
	PEAT_BEGIN = 0,
	PEAT_END = 1,
	PEAT_CHANGE = 2,
	PEAT_PATROL = 3,
	NUM_PACK_EVENT_ACTION_TYPES = 4,
};

// What a package does when it begins, ends, or has its conditions change: an idle to play, a script to run or a
// topic to say. eType says which event the action belongs to (set by TESPackage::InitItem).
struct PackageEventAction {
	PackageEventAction();
	~PackageEventAction();

	// Copies apSource into this action.
	void Copy(PackageEventAction* apCopy);
	// Reads the action's idle, topic and script chunks.
	void Load(TESFile* apFile);
	// Resolves the action's form IDs once the forms are loaded.
	void InitItem(TESForm* apOwner);
	void SetType(int32_t aeType);
	// Returns pResultScript.
	Script* QScript();
	// pIdle.
	TESIdleForm* QIdle() { return pIdle; }
	// pTopic.
	TESTopic* QTopic() { return pTopic; }
	void Save(CHUNK_ID aeChunk);
	// Whether apCompare differs from this action: its idle, topic or result script (QIdle, QTopic, QScript on
	// apCompare, then Script::CompareResultScripts).
	bool Compare(PackageEventAction* apCompare);

	TESIdleForm* pIdle;
	Script* pResultScript;
	TESTopic* pTopic;
	PACK_EVENT_ACTION_TYPE eType;
};

void TESPackageOnBeginEndOrChange_Save(PackageEventAction* apThis, [[maybe_unused]] void* apEdx, CHUNK_ID a2);
