#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Reconstruction.hpp"

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

	// The quest's editor ID. The PC build's own slot; the debug info has no name for it (IDA's name).
	virtual const char* GetEditorName();
	using TESForm::Save;

	// Sets every objective's state from aiState.
	void SetAllObjectivesComplete(int32_t aiState);
	// The objective with index aiIndex, or null.
	BGSQuestObjective* GetObjective(uint32_t aiIndex);
	// &m_listObjectives; the exe's shared address getter (thiscall, ret 0, 0x5D43E0).
	BSSimpleList<BGSQuestObjective*>* GetObjectiveList();
	// Sets the quest's stage to aucIndex; whether it was set.
	bool SetStageDone(uint8_t aucIndex);
	// Starts or stops the quest and marks the change. Called out of line.
	void SetEnabled(uint8_t abEnabled);
	// The quest's conditions (&+0x54); the forwarder stays for src's callers.
	TESCondition* GetConditions();
	// Whether the quest is running.
	bool GetEnabled();
	// The delay between runs of the quest's script, or 0 for the default (thiscall, ret 0,
	// ST0, 0x792760).
	float GetScriptProcessingDelay();
	// Sets the delay between runs of the quest's script (thiscall, ret 4; the exe's shared float setter 0x444020).
	void SetScriptProcessingDelay(float afVal);
	// The quest script's locals (the shared one-field getter 0x59E300, thiscall, ret 0).
	ScriptLocals* GetScriptLocals();
	// Runs the quest's script on its locals, without an owner.
	void RunScript();
	// &m_listStages; the exe's shared address getter (thiscall, ret 0, 0x5D43C0).
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
	// The quest's DATA chunk (thiscall, ret 0; the exe shares the one-field getter 0x5A8080).
	QUEST_DATA* GetData();
	// Sets or clears the starts-enabled flag (0x10) of the DATA flags.
	// The exe tests the whole byte; bool would assume 0/1.
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
	// IDA name; not in the PDB (the editor ID GetEditorName returns).
	BSStringT<char> strEditorID;
};
inline constexpr fnvd::Vtable TESQuest_vftable{};
inline constexpr fnvd::Vtable TESQuest_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESQuest_vftable_TESIcon{};
inline constexpr fnvd::Vtable TESQuest_vftable_TESScriptableForm{};

// The game settings TESQuest.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bActivateAllQuestScripts;
