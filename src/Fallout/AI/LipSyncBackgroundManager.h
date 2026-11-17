#pragma once

#include "BSMain/QueuedFiles.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class LipSyncBackgroundManager;
class LipSynchAnim;
class MobileObject;

// The background task that loads an actor's lip sync animation; it reports back to its LipSyncBackgroundManager.
class LipTask : public QueuedFileEntry {
public:
	LipTask(LipSyncBackgroundManager* apOwner, MobileObject* apActor, const char* apFilename);
	~LipTask() override;

	void Run() override;
	void Finish() override;
	void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
	bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
	uint8_t GetFileIndex() override;

	MobileObject* pActor;
	LipSynchAnim* pLipSynicAnim;
	LipSyncBackgroundManager* pOwner;
	uint8_t padding3C[4];
};

// Loads actors' lip sync animations in the background, one LipTask per actor.
class LipSyncBackgroundManager {
public:
	LipSyncBackgroundManager();
	~LipSyncBackgroundManager();

	// Drops apActor's task.
	void RemoveTask(MobileObject* apActor);
	// Cancels apActor's lip file load.
	void CancelLipFile(MobileObject* apActor);
	void BuildLipSynicFile(MobileObject* apActor, const char* apFilename);

	// Each actor's lip task, keyed by the actor: RemoveTask calls RemoveAt and CancelLipFile calls GetAt through its
	// vtable.
	LockFreeMap<MobileObject*, NiPointer<LipTask>> LipMap;
};
