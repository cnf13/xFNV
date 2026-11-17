#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESScriptableForm.h"

class BGSQuestObjective;
class ScriptLocals;
class TESQuestStage;
class TESQuestStageItem;
class TESQuestTarget;

// A quest's DATA chunk: its flags, priority and the delay between runs of its script.
struct QUEST_DATA {
	QUEST_DATA* Init();
	// Byte-swaps the delay read from a big-endian file.
	void Endian();

	uint8_t ucFlags;
	uint8_t ucPriority;
	uint8_t padding02[2];
	float fQuestDelayTime;
};

// A quest form (QUST).
class TESQuest : public TESForm, public TESScriptableForm, public TESIcon, public TESFullName {
public:
	TESQuest();
	~TESQuest() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Revert(uint32_t aiFlags) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void CheckSaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void InitItem() override;
	bool SetFormEditorID(const char* apEditorID) override;

	// The quest's editor ID.
	virtual const char* GetEditorName();
	using TESForm::Save;

	// Sets every objective's state from aiState.
	void SetAllObjectivesComplete(int32_t aiState);
	// The objective with index aiIndex, or null.
	BGSQuestObjective* GetObjective(uint32_t aiIndex);
	// Returns &m_listObjectives.
	BSSimpleList<BGSQuestObjective*>* GetObjectiveList();
	// Sets the quest's stage to aucIndex; whether it was set.
	bool SetStageDone(uint8_t aucIndex);
	// Starts or stops the quest and marks the change.
	void SetEnabled(uint8_t abEnabled);
	TESCondition* GetConditions();
	// Whether the quest is running.
	bool GetEnabled();
	// The delay between runs of the quest's script, or 0 for the default.
	float GetScriptProcessingDelay();
	// Sets the delay between runs of the quest's script.
	void SetScriptProcessingDelay(float afVal);
	// The quest script's locals.
	ScriptLocals* GetScriptLocals();
	// Runs the quest's script on its locals, without an owner.
	void RunScript();
	// Returns &m_listStages.
	BSSimpleList<TESQuestStage*>* GetStageList();
	// The stage with index acIndex, or null.
	TESQuestStage* GetStage(uint8_t acIndex);
	// The index of the stage that holds apStageItem, or 0 when no stage does.
	uint8_t GetStageIndexForStageItem(TESQuestStageItem* apStageItem);
	// Checks the quest's targets in apListToValidate against the player's active objectives;
	// false when the list has to be rebuilt.
	bool ValidateTargetList(BSSimpleList<TESQuestTarget*>* apListToValidate,
		BSSimpleList<BGSQuestObjective*>* apPlayerObjectiveList);
	// Fills apListToFill with the targets of the active objectives.
	void UpdateCurrentTargetList(BSSimpleList<TESQuestTarget*>* apListToFill,
		BSSimpleList<BGSQuestObjective*>* apPlayerObjectiveList);
	// The quest's priority.
	uint32_t GetPriority();
	// The quest's DATA chunk.
	QUEST_DATA* GetData();
	// Sets or clears the starts-enabled flag (0x10) of the DATA flags.
	void SetStartsEnabled(bool abVal);
	// Raises cCurrentStage to the highest done stage's index.
	void FindCurrentStage();

protected:
	// Deletes every stage.
	void ClearStageList();
	// Deletes every objective.
	void ClearObjectiveList();

public:
	// Whether the quest has failed.
	bool GetFailed();
	// Whether the quest is completed.
	bool GetCompleted();
	uint32_t GetCurrentStage();
	int32_t GetHasShownQuestUpdate();
	void SetDisplayedQuestStarted(char a2);
	bool GetAllowRepeatStages();
	bool GetAllowRepeats();
	int32_t GetFlag0x10();
	void SetCompleted(bool abCompleted);
	void SetFailed(bool abFailed);
	uint8_t IsStageDone(uint8_t aucIndex);
	void ResetQuest();

	static void GetQuestLog(BSSimpleList<TESQuestStageItem*>* apListToFill, TESQuest* apQuest, bool abShowCompleted,
		bool abNewestFirst, bool abShowAll);

	QUEST_DATA data;
	BSSimpleList<TESQuestStage*> m_listStages;
	BSSimpleList<BGSQuestObjective*> m_listObjectives;
	TESCondition objConditions;
	ScriptLocals* pScriptLocals;
	uint8_t cCurrentStage;
	uint8_t padding61[3];
	BSStringT<char> strEditorID;
};

// The game settings TESQuest.cpp defines.
extern SettingT<INISettingCollection> bActivateAllQuestScripts;
