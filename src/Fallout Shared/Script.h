#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/TESForm.h"
#include "Reconstruction.hpp"

class Actor;
class BGSEncounterZone;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class BSBound;
enum COMPILER_NAME : int32_t;
class ExtraDataList;
struct SCRIPT_LOCAL;
struct SCRIPT_PARAMETER;
class ScriptCompileData;
class ScriptCompiler;
class ScriptLocals;
class ScriptVariable;
class TESIdleForm;
class TESObject;
class TESObjectREFR;
class TESQuest;

bool Cmd_GetIsCurrentWeather(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_IsWeaponSkillType(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_SameFactionAsPC(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
// IDA name; not in the PDB. Whether apRefObject and the actor apParam1 share a faction, into
// afReturnValue (cdecl, 0x59D400). apRefObject counts only when its virtual at +0x100 says it is an actor (false for a
// reference, 0x47C850; true for an actor, 0x8D0360); apParam1 is read as an actor without a test
// (Actor::GetActorBase). Cmd_SameFactionAsPC passes the player.
bool Cmd_SameFaction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
// Stores 0 in afReturnValue and returns true: a condition function,
// typed by its table slot.
bool Cmd_SetArgTo0AndReturnTrue(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_SameRaceAsPC(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_SameSexAsPC(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_SameRace(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_SameSex(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_IsWeaponInList(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetDefaultOpen(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetFactionRelation(TESObjectREFR* apRefObject, Actor* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_IsLastIdlePlayed(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
void ClearPendingReferenceLists();

// Header of a script's compiled data blob.
struct SCRIPT_HEADER {
	// Byte-swaps the four counts read from a big-endian file; the flags are left alone.
	void Endian();

	// slots left empty by the compiler for variables created later
	uint32_t variableCount;
	// number of forms the compiled data references
	uint32_t refObjectCount;
	// size of the compiled data
	uint32_t dataSize;
	// the last local variable ID handed out
	uint32_t m_uiLastID;
	// the kind of script: a quest script, a magic effect script, or neither for an object script
	bool bIsQuestScript;
	bool bIsMagicEffectScript;
	// set once the source compiled cleanly
	bool bIsCompiled;
	uint8_t padding13;
};

// A compiled script form: its source text, compiled data, referenced forms and local variables.
class Script : public TESForm {
public:
	// One form the script text references by editor name.
	struct RefVariable {
		RefVariable();
		RefVariable(const RefVariable* apSource);

		// The exe's deleting destructor frees with ourFree (0x5AA3F0).
		static void operator delete(void* apMemory) { ourFree(apMemory); }

		// the editor name as written in the source
		BSStringT<char> name;
		// the form the name resolved to
		TESForm* form;
		// the local variable index the reference resolved to, if any
		uint32_t varIdx;
	};

	// The constructor, run over apMemory.
	Script();
	~Script() override;

	void InitializeData() override;

	// Frees the compiled data, source text, reference list and local variables.
	void ClearData() override;

	bool Load(TESFile* pFile) override;
	void Save() override;
	bool SaveEdit(TESFile* pFile) override;
	void InitItem() override;

	using TESForm::Save;

	// Installs the editor name, compiled data, references and variables of a freshly compiled script.
	void Init(ScriptCompileData* apCompileData);

	// Initializes the reference list from the owning form.
	void InitItem(TESForm* apOwner);
	// Deletes every local variable and empties the list.
	void ClearVariableList();

	// Deletes every referenced object and empties the list; forgets the last reference lookup when it was ours.
	void ClearAllReferenceObjects();

	// Copies apScriptToCopy's header, compiled data, referenced objects, variables, text and initialized flag; the
	// unused-variable count starts over at 0.
	void CopyResultScript(Script* apScriptToCopy);

	// Clears the forms of the references that resolve through a local variable; forgets the last reference lookup
	// when it was ours.
	void ClearLocalVariableRefObjects();

	// The PDB's EndianData, which byte-swaps the compiled data on big-endian builds; the PC
	// build calls the engine's shared empty thiscall function (0x483710) for it.
	void EndianData();

	// The referenced object with 1-based index aiIndex, its form refreshed from the local variable it names in
	// apScriptLocals; the last lookup is cached per thread.
	RefVariable* GetReferencedObject(uint32_t aiIndex, ScriptLocals* apScriptLocals);
	// Replaces the compiled data with auiDataSize bytes copied from apCompileData.
	void SetCompileData(uint32_t auiDataSize, char* apCompileData);

	// Replaces the source text with a copy of newText.
	void SetText(const char* newText);

	// The name of local variable auiID, or null after a warning when it does not exist.
	const char* GetVariableName(uint32_t auiID);

	// Looks a local variable up by name; the returned character is 's' or 'f'.
	char FindVariable(const char* varName, uint32_t* auiID);
	BSSimpleList<SCRIPT_LOCAL*>* CreateLocalList();
	BSSimpleList<ScriptVariable*>* GetVariableList();

	// The compiled-data header (the shared one-field getter 0x500940, thiscall).
	SCRIPT_HEADER* GetHeader();
	// The quest script's GetSecondsPassed buffer (thiscall, ret 0, ST0).
	float GetQuestScriptGetSecondsBuffer();
	// The source text (the shared one-field getter 0x55B980, thiscall).
	char* GetText();
	// The referenced-object list (the shared getter 0x5D43C0, thiscall).
	BSSimpleList<RefVariable*>* GetReferencedObjectList();
	// Sets the header's compiled flag (the shared setter 0x7F7C90, thiscall, ret 4).
	void SetIsCompiled(bool abCompiled);
	// The compiled data (the shared one-field getter 0x671D10, thiscall).
	char* GetCompiledData();
	// True when apCompileData's name, header, compiled data, variables or references differ from this script's.
	bool NeedsUpdate(ScriptCompileData* apCompileData);
	// Adds afTime to the profiler timer.
	void AddProfilerTime(float afTime);
	// The quest this script belongs to.
	void SetOwnerQuest(TESQuest* apQuest);

	// A new set of local variables for the script.
	ScriptLocals* CloneLocals();

	// Runs the script on ownerObject with its locals; a quest script waits out its delay first.
	bool Run(TESObjectREFR* ownerObject, ScriptLocals* ownerVars, TESObjectREFR* ownerContainer,
		bool abIsPartialScript);

	// Compiles the script's text as a partial script with compiler under compilerIndex and runs it once on
	// ownerObject; the system window compiler echoes condition results to the console meanwhile.
	void CompileAndRun(ScriptCompiler* compiler, COMPILER_NAME compilerIndex, TESObjectREFR* ownerObject);
	void RunScriptEffectStart(TESObjectREFR* apTargetRef, ScriptLocals* apEffectLocals);
	void RunScriptEffectFinish(TESObjectREFR* apTargetRef, ScriptLocals* apEffectLocals);
	void RunScriptEffectUpdate(TESObjectREFR* apTargetRef, ScriptLocals* apEffectLocals, float afSecondsElapsed);
	bool GetIsCompiled();
	void SaveResultScript();
	// True when apCompare's header, compiled data, text or referenced forms differ from this script's.
	bool CompareResultScripts(Script* apCompare);

	// Evaluates one compiled operand (a constant, a local variable, a referenced object or a function call) at
	// *auiDataOffset into *adValue. With abEndian set on a script not yet initialized, a function call only has its
	// parameters parsed.
	static bool GetValue(double& adValue, const char* apCompiledData, uint32_t& auiDataOffset,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		bool abEndian);

	static void GetNumericIDFromDouble(uint32_t& aiID, const double& adVal);
	// Whether apRef is waiting to be enabled.
	static bool IsPendingEnabledReference(TESObjectREFR* apRef);
	// Whether apRef is waiting to be disabled with a fade.
	static bool IsPendingFadeDisabledReference(TESObjectREFR* apRef);
	// Sets the base scale of apRef.
	static void SetScaleBase(TESObjectREFR* apRefObject, float afValue);
	// Damages apRef's object by afDamage.
	static void DamageObjectBase(TESObjectREFR* apRef, float afValue);
	// Matches the face geometry of apCharacter to apGeometry.
	static void MatchFaceGeometryBase(TESObjectREFR* apRef, TESObjectREFR* apMatchActor, int32_t aiValue);
	// The GetInZone condition.
	static bool GetInZoneConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// The GetPermanentActorValue condition; apParam1 holds the ActorValue::Index.
	static bool GetPermanentActorValueConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// The GetBaseActorValue evaluation (cdecl, 0x59C5D0); it passes aeActorValue on to
	// Actor::GetBaseActorValue and ActorValue::GetActorValueScriptName as their ActorValue::Index. It returns a bool
	// (`mov al, 1`), as the other evaluations do.
	static bool GetBaseActorValue_Eval(Actor* apActor, ActorValue::Index aeActorValue, int32_t aiArg2,
		double* apResult);
	// The GetAlarmed condition.
	static bool GetAlarmedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// The IsWeaponOut condition.
	static bool IsWeaponOutConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// The GetOpenState condition (cdecl, 0x5A1C30).
	// Inferred type: apRefObject: TESObjectREFR* from 0x5ccb05 (parameter 2 of Script::GetOpenStateFunction)
	static bool GetOpenStateConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// The GetThreatRatio condition.
	static bool GetThreatRatioConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// The GetRadiationLevel condition.
	static bool GetRadiationLevel(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
	// The IsCombatTarget condition.
	static bool IsCombatTargetConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetWeaponAnimTypeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetCurrentAIProcedureConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetActorCrimePlayerEnemyConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetActorsInHighConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_HasLoaded3D.
	static bool HasLoaded3DConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetFatiguePercentage_Eval.
	static bool GetFatiguePercentageConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetHealthPercentage_Eval.
	static bool GetHealthPercentageConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetScale_Eval.
	static bool GetScaleConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: GetLineOfSight.
	static bool GetLOSConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetDisabled_Eval.
	static bool GetDisabledConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetLocked.
	static bool GetLockedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetLockLevel.
	static bool GetLockLevelConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsLockBroken.
	static bool GetIsLockBrokenConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetDisease.
	static bool GetDiseaseConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetVampire.
	static bool GetVampireConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetClothingValue.
	static bool GetClothingValueConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetsSecondsPassed_Eval. The seconds since aiParam1, a script, last ran (cdecl,
	// 0x59C430).
	static bool GetSecondsPassedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsRace. Whether apRefObject's NPC base is of race aiParam1 (cdecl, 0x59F240).
	static bool GetIsRaceConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetInFaction. Whether the actor apRefObject is in faction aiParam1 (cdecl,
	// 0x59F630).
	static bool GetInFactionConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetArmorRatingUpperBody. The armor rating of apRefObject's upper-body armor
	// (cdecl, 0x5A2CE0).
	static bool GetArmorRatingUpperConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetDetected_Eval.
	static bool GetDetectedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetDead_Eval.
	static bool GetDeadConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetItemCount_Execute.
	static bool GetItemCountConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetEquipped_Execute.
	static bool GetEquippedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetGold.
	static bool GetGoldConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetSleeping.
	static bool GetSleepingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetSitting.
	static bool GetSittingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetFurnitureMarkerID.
	static bool GetFurnitureMarkerIDConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetQuestRunning.
	static bool GetQuestRunningConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsPleasant.
	static bool GetIsPleasantConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsCloudy.
	static bool GetIsCloudyConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsRaining.
	static bool GetIsRainingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsSnowing.
	static bool GetIsSnowingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetCurrentWeatherPercent.
	static bool GetWeatherPercentConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetAttacked.
	static bool GetAttackedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsCreature.
	static bool GetIsCreatureConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsPlayableRace.
	static bool GetIsPlayableRaceConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsInList_Eval.
	static bool IsInListConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsChild.
	static bool GetIsChildConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsUsedItem.
	static bool GetIsUsedItemConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsUsedItemLevel.
	static bool GetUsedItemLevelConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsUsedItemActivate.
	static bool GetUsedItemActivateConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetRestrained.
	static bool GetRestrainedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetRandomPercent_Eval.
	static bool GetRandomPercentConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetLevel.
	static bool GetLevelConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetArmorRating.
	static bool GetArmorRatingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: GetDeadCount.
	static bool GetDeadCountConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsAlerted.
	static bool GetAlertConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsFacingUp_Eval.
	static bool IsFacingUpConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsLeftUp_Eval.
	static bool IsLeftUpConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetKnockedState.
	static bool GetKnockedStateConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetCurrentAIPackage_Eval.
	static bool GetCurrentAIPackageConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsWaiting.
	static bool IsWaitingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsIdlePlaying.
	static bool IsIdlePlayingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsGuard.
	static bool IsGuardConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsTalking.
	static bool IsTalkingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetTrespassWarningLevel.
	static bool GetTrespassWarningLevelConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsTrespassing.
	static bool IsTrespassingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetAmountSoldStolen.
	static bool GetAmountStolenSoldConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetGroupMemberCount.
	static bool GetGroupMemberCountConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetWindSpeed.
	static bool GetWindSpeedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsInMyOwnedCell.
	static bool IsInMyOwnedCellConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsContinuingPackagePCNear.
	static bool IsContinuingPackagePCNearConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_CanHaveFlames.
	static bool CanHaveFlamesConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_HasFlames.
	static bool HasFlamesConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetDayOfWeek.
	static bool GetDayOfWeekConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsPCSleeping.
	static bool IsPCSleepingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsPCAMurderer.
	static bool IsPCAMurdererConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetDetectionLevel_Eval.
	static bool GetDetectionLevelConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsSwimming_Eval.
	static bool IsSwimmingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetDestroyed.
	static bool GetDestroyedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_HasMagicEffect_Eval.
	static bool HasMagicEffectConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetVATSMode.
	static bool GetVATSModeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetVATSTargetHeight.
	static bool GetVATSTargetHeightConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsInSandmanAction.
	static bool GetSandmanConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsInCannibalAction.
	static bool GetCannibalConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_HasBeenEaten.
	static bool HasBeenEatenConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsGhost_Eval.
	static bool GetGhostConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetOffersServicesNow.
	static bool GetOffersServiceConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetBarterGold.
	static bool GetBarterGoldConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsActorUsingATorch.
	static bool IsActorUsingATorchConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsTimePassing.
	static bool IsTimePassingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsSneaking.
	static bool IsSneakingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsRunning.
	static bool IsRunningConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetFriendHit.
	static bool GetFriendHitConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsInInterior.
	static bool IsInInteriorConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsActorEvil.
	static bool IsActorEvilConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsActorAVictim.
	static bool IsActorVictimConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIdleDoneOnce.
	static bool GetIdleDoneOnceConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_WhichServiceMenu.
	static bool GetWhichServiceConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsRidingHorse.
	static bool IsActorRidingHorseConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsInDangerousWater.
	static bool IsInDangerousWaterConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsXBox.
	static bool IsXboxConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsPS3.
	static bool IsPS3ConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsPC.
	static bool IsPCConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsEssential.
	static bool IsEssentialConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsActor.
	static bool IsActorConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsPlayerMovingIntoNewSpace.
	static bool IsPlayerMovingIntoNewSpaceConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetTimeDead.
	static bool GetTimeDeadConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetLastPlayerAction_Eval.
	static bool GetLastPlayerActionConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetLastHitCritical.
	static bool GetLastHitCriticalConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetCauseOfDeath_Execute.
	static bool GetCauseofDeathConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetKillingBlowLimb.
	static bool GetKillingBlowLimbConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_HasFriendDisposition.
	static bool HasFriendDispositionConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsMoving_eval.
	static bool GetIsMovingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetAnimAction.
	static bool GetAnimActionConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetGroupTargetCount.
	static bool GetGroupTargetCountConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetPlantedExplosive.
	static bool GetPlantedExplosiveConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsActorTalkingThroughActivator.
	static bool IsActorTalkingThroughActivatorConditionFunction(TESObjectREFR* apRefObject, void* apParam1,
		void* apParam2, double& afReturnValue);
	// IDA: Cmd_GetDialogueEmotion.
	static bool GetDialogueEmotionConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetDialogueEmotionValue.
	static bool GetDialogueEmotionValueConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsWaterObject.
	static bool IsWaterObjectConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_HasPerk_Eval.
	static bool HasPerkConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetPlayerTeammate_Eval.
	static bool GetPlayerTeammateConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetActorFactionPlayerEnemy.
	static bool GetActorFactionPlayerEnemyConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetMinorCrimeCount.
	static bool GetMinorCrimeCountConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetMajorCrimeCount.
	static bool GetMajorCrimeCountConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetDestructionStage.
	static bool GetDestructionStageConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIsUsedItemEquipType.
	static bool GetIsUsedItemEquipTypeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetActorAggroRadiusViolated.
	static bool GetAggroRadiusViolatedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetMapMarkerVisible.
	static bool GetMapMarkerVisibleConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetWeaponHealthPerc.
	static bool GetWeaponHealthPercConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsGreetingPlayer.
	static bool IsGreetingPlayerConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetXPForNextLevel.
	static bool GetXPForNextLevelConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_GetIgnoreCrime.
	static bool GetIgnoreCrimeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsGoreDisabled.
	static bool IsGoreDisabledConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsHardcore_Eval.
	static bool IsHardcoreConditionalFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA: Cmd_IsAlwaysHardcore.
	static bool IsAlwaysHardcoreConditionalFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// IDA name _sub_5A4160.
	static bool GetIsTurningConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Parses the parameters of a script function at auiOffset into the trailing arguments, one pointer per parsed
	// parameter; nothing is stored while apScript is not initialized (cdecl, variadic).
	static bool ParseParameters(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, uint32_t& auiOffset,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, ...);

	// IDA name; not in the PDB. Expands the script variables in apText into *apOut, reading
	// the command's own parameters (cdecl, 10 arguments, 0x5B4960).
	// Inferred type: apCompiledParams: char* from 0x5b49d6 (passed to Script::GetValue 1); auiOffset: uint32_t* from
	// 0x5b49d2 (passed to Script::GetValue 2); apText: char* from 0x5b4a52 (passed to BSStringT<char>::SPrintF 0)
	static bool ParseString(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double* afReturnValue, uint32_t* auiOffset, const char* apText, BSStringT<char>* apOut);
	// The ShowMessage script command: shows a message form as a message box with its
	// buttons, or as a HUD message.
	// Inferred type: apCompiledParams: char* from 0x5b46f1 (passed to Script::ParseString 1); auiOffset: uint32_t* from
	// 0x5b46d9 (passed to Script::ParseString 7)
	static bool ShowMessage(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// The thread's m_bSystemOutput (IDA name; not in the PDB).
	static bool VerboseOutput();

	// The exe's per-thread statics (PDB S_GTHREAD32 records), reached the way the exe reaches them: whether
	// condition functions echo to the console, and the last GetReferencedObject lookup.
	static bool& GetThreadSystemOutput();
	static Script*& GetThreadLastRefSearchScript();
	static ScriptLocals*& GetThreadLastScriptLocals();
	static uint32_t& GetThreadLastRefSearchIndex();
	static RefVariable*& GetThreadLastRefObject();

	// Script functions of TESScriptFunctions.cpp, cdecl in the exe, each taking the compiled parameters.
	static bool PrintHDRParamFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool AddScriptPackageFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// The PDB's Activate script command (IDA Script::Activate): the reference activates the
	// parsed activator (else its action ref), at most 5 levels deep, setting and clearing its action flags around
	// TESObjectREFR::Activate unless the second parsed parameter is set; refuses the player (cdecl, 0x5B59F0).
	static bool ActivateFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Copies the class and level of the parsed reference's NPC base onto the calling actor's NPC base and has the
	// latter recompute its values.
	static bool DuplicateNPCStatsFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Script functions that parse their parameters and hand them to the matching condition function.
	static bool GetDisposition(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetPCFactionMurderFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetPlayerEnemyofFactionFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetPCFactionAttackFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsPlayerActionActiveFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetTalkingActivatorActorFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetHasNoteFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ExistsFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Moves apMoveRef to apTargetRef offset by (afX, afY, afZ).
	static void MoveToFunctionBase(TESObjectREFR* apMoveRef, TESObjectREFR* apTargetRef, float afX, float afY,
		float afZ);
	// IDA name Cmd_MoveToMarker; not named in the PDB or IDA as a Script member, name chosen here.
	static bool MoveToFunction(SCRIPT_PARAMETER* apParameters, char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double* afReturnValue,
		uint32_t* auiOffset);
	// IDA name Cmd_MoveToMarkerWithFade; not named in the PDB or IDA as a Script member, name chosen here.
	static bool MoveToFadeFunction(SCRIPT_PARAMETER* apParameters, char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double* afReturnValue,
		uint32_t* auiOffset);
	static bool WaitFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ShowSPECIALBookMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ShowLoveTesterMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool AutoDisplayObjectives(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetMerchantContainer(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool PlaceAtMeHealthPercentFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetObjectiveDisplayedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// The script functions whose bodies are reconstructed in TESScriptFunctions.cpp. Each takes the compiled
	// parameters, the calling reference and container, the script and its locals, the return value slot and the
	// offset the parameters are parsed from (cdecl).
	static bool OutputArchiveProfile(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetActorAlpha(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetRigidBodyMass(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Sets the rigid body mass of apRefObject's physics body.
	static void SetRigidBodyMassBase(TESObjectREFR* apRefObject, float afValue);
	// Prints the reference's full name and deletes it; reads only its third argument.
	static bool DeleteReference(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleCastShadows(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Console command: prints or sets the reference's FaceGen expressions, phonemes and modifiers, rebuilds or
	// cycles its face coordinates ("coord"), or resets its morphs ("reset").
	static bool ModifyFaceGen(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsCodeRedeemedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool RedeemCodeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Parses one integer and
	// makes the radio dialogue-only when it is 1, else not.
	static bool SetRadioToDialogueOnlyModeFunction(SCRIPT_PARAMETER* apParameters, char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double* afReturnValue, uint32_t* auiOffset);
	// Console command: sets the falloff angles and opacities (-1 keeps one unchanged) of the no-lighting shaders on
	// the reference's 3D; "No Reference Selected" without a reference.
	static bool SetTargetFalloff(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Console command: sets the UFO camera's speed multiplier setting.
	static bool SetUFOCamSpeedMult(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Console command: sets the local and LOD tree mipmap biases, printing the old and the new values.
	static bool SetTreeMipmapBias(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Console command: projects a blood or impact decal from the camera onto the reference.
	static bool AddDecal(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	// Script opcodes that parse their axis or type and hand the reference to their condition function.
	static bool GetPosFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetStartingPosFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetAngleFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetStartingAngleFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsSex(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool GetPCIsSex(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsUsedItemType(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Script opcode: opens and plays the music type it parses.
	static bool PlayMusicFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Script opcode: places count copies of the parsed object at the reference; returns the last one's form ID.
	static bool PlaceAtMeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Places aiCount copies of apObject at apPlaceAtRef, aiDistance away in aiDirection, at
	// afHealthPerc health; returns the last reference placed.
	static TESObjectREFR* PlaceAtMeFunctionBase(TESObjectREFR* apPlaceAtRef, TESObject* apObject, int32_t aiCount,
		int32_t aiDistance, int32_t aiDirection, float afHealthPerc);
	// Console command: turns the audio update thread on or off.
	static bool SetAudioMultiThreading(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Script opcode: shows (a positive value) or hides the scripted loading menu.
	static bool EnableLoadingMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Condition functions: the reference's position, starting position, angle and starting
	// angle on the axis apParam1 ('X', 'Y' or 'Z'), and whether the used item is of type apParam1.
	static bool GetPosConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetAngleConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetStartingPosConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetStartingAngleConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetIsUsedItemTypeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static void CopyReferencedObjectList(BSSimpleList<RefVariable*>* pCopyFromList,
		BSSimpleList<RefVariable*>* pCopyToList, Script* apOwner);
	static void CopyVariableList(BSSimpleList<ScriptVariable*>* pCopyFromList,
		BSSimpleList<ScriptVariable*>* pCopyToList, Script* apOwner);
	static bool IsPendingDisabledReference(TESObjectREFR* apRef);
	// Empties the four pending reference
	// lists under ScriptRefListCrit and reloads them from apLoadGameBuffer.
	// Inferred type: apLoadGameBuffer: BGSLoadGameBuffer* from 0x95c081 (parameter 0 of PlayerCharacter::LoadGame)
	static void LoadPendingReferenceLists(BGSLoadGameBuffer* apLoadGameBuffer);
	// Forgets the per-thread last referenced-object lookup.
	static void ClearOptimizations();

	// True while the game runs its per-frame script processing pass.
	static bool GetProcessScripts();
	// Turns the per-frame script processing pass on or off.
	static void SetProcessScripts(bool abProcessScripts);
	// Console commands: PrintNPCDialog, ToggleMotionBlur on the reference's 3D, TogglePrimitives, ToggleScripts and
	// ToggleTrees. Each returns true.
	static bool PrintNPCDialog(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleMotionBlur(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool TogglePrimitivesFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleScriptsFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleTrees(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Copies the 4 bytes at apID (a form ID) into the low half of *adVal.
	static void PutNumericIDInDouble(const uint32_t& aiID, double& adVal);

	// Sets aiFlag on the action list apExtra carries for apActionForm.
	static bool SetActionFlag(TESForm* apActionForm, ExtraDataList* apExtra, uint32_t aiFlag);
	// Queues apRef to be disabled, faded out with abFade.
	static void AddPendingDisabledReference(TESObjectREFR* apRef, bool abFade);

	// Gives apOwnerObject's script a start action for the list apExtra carries.
	static void InitActionList(TESObjectREFR* ownerObject, ExtraDataList* apExtra);

	// Script opcode: the menu mode state parsed from the parameters.
	static bool MenuModeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Console command: shows or hides the grass and reports the new state.
	static bool ToggleGrassFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Console command: prints the float it parses. Not named in the PDB or IDA; name chosen here.
	static bool ShowFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Script opcodes that hand the reference to their condition function (IsTrespassing: true without one).
	static bool IsTrespassingFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetAmountStolenSoldFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetGroupMemberCountFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetPlayerTeammateCountFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Script opcode: sets the reference's critical stage when it is an actor. IDA name Cmd_SetCriticalStage; not in
	// the PDB.
	static bool SetCriticalStageFunction(SCRIPT_PARAMETER* apParameters, char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double* afReturnValue, uint32_t* auiOffset);

	// Script opcode: true outside of menus and the pause fade.
	static bool GameModeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Script opcode: 1 while the current script effect has started.
	static char ScriptEffectStart(SCRIPT_PARAMETER* apParameters, char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double* afReturnValue,
		uint32_t* auiOffset);

	// Script opcode: sets the world and first-person fields of view (at most 160; 0 means 75, and a negative value
	// v means 1 / -v) on the scene graph, the shader manager, the player and the default FOV settings.
	static bool SetCameraFOV(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Event helper: sets *afReturnValue to 1 when apLocals holds aiFlag for apActionForm.
	static void CheckFlagFunction(uint32_t aiFlag, TESObjectREFR* apRefObject, TESForm* apActionForm,
		ScriptLocals* apScriptLocals, double& afReturnValue);

	// Condition function: whether the player carries the note apParam1.
	static bool GetHasNoteConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Condition function: the hit location (damage limb) of apRefObject's last hit.
	static bool GetHitLocationConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Condition function: whether menu apParam1 is up (the top menu under the old rule), or any menu when it is 0.
	static bool GetMenuModeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Condition function: whether the reference apParam2 (else apRefObject) is in a cell whose editor ID starts with
	// that of cell apParam1; the last answer is cached.
	static bool GetInCellConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Condition function: apRefObject's distance to the reference apParam1.
	static bool GetDistanceConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Condition function: actor value apParam1 of the actor apRefObject (its base's while it is disabled).
	static bool GetActorValueConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Condition function: whether the actor apRefObject (else the reference apParam1) has spoken to the player.
	static bool GetTalkedToPCConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Condition function: 100 when the actor apRefObject should attack the actor apParam1, else 0.
	static bool GetShouldAttackConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Condition function: whether apRefObject's NPC base form has sex apParam1.
	static bool GetIsSexConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Condition function: whether apRefObject's base form (its original leveled base first) is apParam1; the last
	// lookup is cached.
	static bool GetIsIDConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Condition function: whether the actor apRefObject is in combat.
	static bool IsInCombatConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Condition function: the walking speed of the actor apRefObject.
	static bool GetWalkSpeedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);

	// Script opcode: plays the parsed sound form in 2D, as a system sound when the second parameter is positive.
	static bool PlaySoundFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Script opcode: prints the memory of the loaded area's scene data in MB.
	static bool TestCode(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);

	// Script opcodes: quit the game; purge the buffered cells; set Actor::bAllPointsVisible and
	// bhkAvoidBox::bNoAvoidance from the parsed integer; set the particle limit of emitters (below 0x10000).
	static bool QuitGame(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool PurgeCellBuffers(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetAllVisible(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetNoAvoidance(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetEmitterParticleMax(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleWaterSystem(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetSceneIsComplex(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleDecalRendering(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ShowTutorialMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Script opcodes: flip the grab IK (of apRefObject's ragdoll, else the global setting), flip the ragdoll
	// animation setting, and set apRefObject's scale (queued on a multi-threaded machine).
	static bool ToggleGrabIK(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Script opcode: flips the look IK of apRefObject's ragdoll, else the global setting.
	static bool ToggleLookIK(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleRagdollAnim(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetScaleFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Script opcode: sets the global time multiplier and whether the audio ignores it.
	static bool SetGlobalTimeMultiplierFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Script opcode: plays the parsed sound form in 3D at apRefObject, following its 3D.
	static bool PlaySound3DFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Script opcode: the walking speed of the actor apRefObject.
	static bool GetWalkSpeedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Script opcode: queues the hacking menu for the terminal apRefObject (no reference when it is not one).
	static bool ShowHackingMiniGame(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Script opcode: starts the parsed number of screen blood splatters.
	static bool TriggerScreenBlood(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Script opcode: closes the console and queues the player name entry menu.
	static bool GetPlayerName(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	// Clears the values cached for the condition functions between checks.
	static void ClearCachedValues();
	// Enables, disables, fade-disables and deletes the references queued for it.
	static void RunDelayedScriptActionsOnReferences();
	static bool ToggleShadowVolumesFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static void ClearButtonPressed();
	static void HandleMessageMenu();
	static bool ToggleFullScreenMotionBlur(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsPC1stPersonConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetButtonPressedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleCharControllerShapeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SkipAnimFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetTalkedToPC(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ForceRadioStationUpdateFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetNoRumors(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
	static bool RevertWorld(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleMaterialGeometryFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleRefractionDebug(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool TriggerHUDShudder(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool DumpMenuPackingInfo(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Inferred type: afReturnValue: double* from 0x5cb7bc (passed to Script::GetWeaponAnimTypeConditionFunction 3)
	static bool GetWeaponAnimType(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetCurrentAIProcedureFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleWaterRadius(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool Reset3DStateFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool DebugCombatPlan(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ClearActorsFactionsPlayerEnemyFunction(const SCRIPT_PARAMETER* apParameters,
		const char* apCompiledParams, TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript,
		ScriptLocals* apScriptLocals, double& afReturnValue, uint32_t& auiOffset);
	static bool GetActorCrimePlayerEnemyFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetActorsInHighFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ShowRenderPasses(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool TriggerLODApocalypseFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetCurrentTimeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool IsCurrentFurnitureRefConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetIsRefConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetUnconsciousConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// The angle (degrees, -180 to 180) from the mobile reference's heading to the direction of the reference apParam1.
	static bool GetHeadingAngleConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetTalkingActivatorActorConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetPlayerTeammateCountConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool IsPlayerGrabbedRefConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetVATSRightAreaFreeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetVATSLeftAreaFreeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetVATSBackAreaFreeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetVATSFrontAreaFreeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetVATSRightTargetVisibleConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetVATSLeftTargetVisibleConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetVATSBackTargetVisibleConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetVATSFrontTargetVisibleConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool ChallengeLockedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetChallengeCompletedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool ToggleVerboseFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleDemigodModeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleSafeZoneFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetAllReachable(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool HasLoaded3DFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetKillingBlowLimbFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool HasFriendDispositionFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetScaleFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ScriptEffectUpdateFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ScriptEffectElapsedTimeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetDisease(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetVampire(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetClothingValue(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetDead(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool GetGold(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool GetSleeping(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetSitting(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetFurnitureMarkerID(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetAlarmed(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsPleasant(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsRaining(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsSnowing(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetWindSpeedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsCreature(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsChild(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetUsedItemLevelFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetUsedItemActivateFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetRandomPercent(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetLevel(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool GetArmorRating(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsGuard(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool IsTalkingFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetTrespassWarningLevelFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsInMyOwnedCellFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsContinuingPackagePCNearFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool CanHaveFlamesFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool HasFlamesFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetOpenStateFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetDayOfWeekFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsPCSleepingFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsPCAMurdererFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsSwimming(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetPlayerSleepHours(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetDestroyedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsActorEvilFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsActorVictimFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIdleDoneOnceFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetNoRumorsFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetWhichServiceFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsActorRidingHorseFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsInDangerousWaterFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetLastPlayerActionFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetBroadcastState(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetBroadcastState(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsActorTalkingThroughActivatorFunction(const SCRIPT_PARAMETER* apParameters,
		const char* apCompiledParams, TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript,
		ScriptLocals* apScriptLocals, double& afReturnValue, uint32_t& auiOffset);
	static bool RemoveMerchantContainer(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ClearLocationMarkersFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetDialogueEmotionFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetDialogueEmotionValueFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsWaterObjectFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetActorFactionPlayerEnemyFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetPlayerGrabbedRefFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetDestructionStageFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetRadiationLevelFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetMapMarkerVisibleFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetWeaponHealthPercFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ClearNoActivationSound(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsGreetingPlayerFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIgnoreCrimeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ResetPipboyManagerFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ForceTerminalBackFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool RadioShutOffFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetCurrentTimeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetUnconsciousFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool DumpNiUpdates(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static void RemoveDelayedScriptActionReference(TESObjectREFR* apRef);
	static bool RefreshINI(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetOpenState(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ResetInventoryFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool AddFlamesFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool PlayBinkFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool DamageActorValueFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool StopCombatAlarmOnActorFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool RefreshShaders(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetArmorRatingUpperFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool FreezeRendererAccumulation(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleBorders(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static void SetAngleBase(TESObjectREFR* apRefObject, char acAxis, float afValue);
	static bool GetGroupTargetCountFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static void AddPendingEnabledReference(TESObjectREFR* apRef);
	static bool ReloadCurrentClimate(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ForceCloseFile(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetSecondsPassedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool StopCombatFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool RemoveFlamesFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool RevokeCodeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsCurrentPackageConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	// Inferred type: a1: BGSSaveGameBuffer* from 0x95a394 (parameter 0 of PlayerCharacter::SaveGame)
	static void SavePendingReferences(BGSSaveGameBuffer* a1);
	static bool ToggleOcclusion(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetActorValueInfoFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetPosFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static void SetPosBase(TESObjectREFR* apRefObject, char acAxis, float afValue);
	static bool ShowVarsFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool StartConversation(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool CompleteAllQuestStages(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetObjectiveCompletedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetMPSParticleMax(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsCurrentFurnitureObjConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetStageConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetStageDoneConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetFactionRankDifferenceConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetInSameCellConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetIsVoiceTypeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetFactionRankConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetGlobalValueConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetDispositionConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetCrimeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool IsSpellTargetConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetSpellUsageNumberConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool IsPlayerActionActiveConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool IsLimbGoneConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetKillerConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetKillerObjectConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetFactionCombatReactionConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetIsObjectTypeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetObjectiveDisplayedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool IsPlayerTagSkillConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetCasinoWinningsLevelConditionalFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2,
		double& afReturnValue);
	static bool GetDistanceFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetInZoneFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool AddItemFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool RemoveItemFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleDialogSubtitle(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Not registered (KEEP): an uninitialised read when ParseParameters fails (see the body).
	static bool OutputMemStats(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool OutputMemStatsFullWalk(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleMapMarkersFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleGodModeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleEmotionsFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool RemoveImageSpaceModifier(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool TestLocalMap(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleMagicStatsFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SaveWorldFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleMenus(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleLeavesFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleDetectionStats(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetTargetRefraction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetMotionBlur(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetVelFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ShowViewerStrings(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleLODLand(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetLODObjectDistanceFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool HighlightBodyPart(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool HighlightAdditionalReference(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool DeactivateAllHighlights(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool AddOcclusionPlane(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetGlobalRadialBlur(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleAI(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool ToggleFlyCam(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleNavMesh(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool PickRefByIDFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetActorValueFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetPermanentActorValueFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetActorValueFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ModActorValueFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool RestoreActorValueFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ForceActorValueFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsLimbGoneFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetLightingPassesFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetFogDepthFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetImageSpaceGlowParamFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetHUDGlowConstants(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool HairTint(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool RotateFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetAngleFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool PlayGroupFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool LoopGroupFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsAnimPlayingFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool StartCombatFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool AddSpell_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool StreamMusicFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// KEEP body (the exe's PrintLine reads arguments it never pushed), 0x5C25F0.
	static bool CalcLowPathToPointFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ShowQuestVarsFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ShowAnim(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool EnableFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool DisableFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SaveGame_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool CopySaves(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool DumpModelMap(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SexChange(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsCurrentFurnitureRef(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsCurrentFurnitureObj(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetTalkedToPCParam(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetStage(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool GetIsCurrentWeatherFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetLockLevelFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsVoiceTypeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SameFaction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SameRace(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool SameSex(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool SameRaceAsPC(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SameSexAsPC(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetQuestCompleted(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetStage(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool GetStageDone(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetFactionRankDifference(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetInFaction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetPCIsClass(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetPCIsRace(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetGlobalValue(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetShouldAttack(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetInCellParam(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetInCell(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetPCInFaction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsID(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool IsInListFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsUsedItem(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsRef(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool GetFactionRank(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetFactionRank(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ModDisposition(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetDeadCount(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ShowMap(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
		TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
		uint32_t& auiOffset);
	static bool SayFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool PlayVATSCameras(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ShowQuestLog(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool LockFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool UnlockFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetCrimeKnownFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetCrimeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool MoveToQuestTargetFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool WakeUpPCFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetCombatStyleFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SelectPlayerSpellFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetEssentialFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetQuestObjectFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetDetectionLevelFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool StopWaitingFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetPlayerSleepHours(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ModAmountStolenSoldFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetPCExpelledFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetPCExpelledFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetDestroyedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetSecuritronExpression(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SwapTextureOnRef(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetUnconsciousFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetRestrainedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetCanPCFastTravelFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ModPCMiscStat(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetPCMiscStat(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetNoRumorsFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool TriggerHitShaderFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool AddAchievement(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool FlushPersistantActors(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleFogOfWar(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool OutputLocalMapPictures(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool WasteMemory(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SendCrimeAlarmFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ForceGroupStrategy(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool PushActorAwayFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetObjectiveCompletedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetObjectiveDisplayedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool PipboyRadio(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetNPCRadio(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool StartRadioConversation(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool PlaceLocationMarkerFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsObjectTypeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool MarkForDelete(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetPlayerTagSkillFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsPlayerTagSkillFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsPlayerGrabbedRefFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool PlaceLeveledActorAtMeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ForceActiveQuestFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool MatchFaceGeometryFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsUsedItemEquipTypeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool AddItemHealthPercentFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SendAssaultAlarmFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetThreatRatioFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool MatchRaceFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetPCYoungFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetPCToddlerFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetConcussedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetVATSTargetFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetZoneRespawnsFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool CreateDetectionEventFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IgnoreCrimeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsCombatTargetFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// KEEP body (uninitialised read on stairs), 0x5DB870.
	static bool AttachAshPileFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetMinimalUseFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetPCCanUsePowerArmorFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetReputationThreshold(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetLastExtDoorActivated(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetTaskThreadSleepFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// IDA name; the PDB's is Script::IsImageSpaceActiveFunction.
	static bool IsImageSpaceActive(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ResetXP_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// IDA name; the PDB's is Script::PlaceAtReticleFunction.
	static bool PlaceAtReticle(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ShowSleepWaitMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static void SetCrimeVictim(TESForm* apform);
	static bool SaveIniFiles(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ReloadCurrentWeather(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool TestSeenData(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleFullHelp(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleSkyFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleCollisionFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleWireframeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleFootIK(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleDebugTextFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ShowScenegraphFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleEOFImageSpace(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleAiDetection(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleActorAI(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsActorsAIOff(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleAISchedules(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool TogglePathingInfoFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleGrassUpdateFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool SetAtStartFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool PlayExplosionFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ScriptEffectFinishFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsCloudy(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetAttacked(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool RemoveScriptPackageFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetActionRefFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ResetFallDamageTimerFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsPlayersLastRiddenHorseFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ClearAdaptedLight(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleVATSLight(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ResetDialogueFlags(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ToggleDepthBiasFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool DisableAllMinesFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool KillQuestUpdates(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Inferred type: afReturnValue: double* from 0x5cb09d (passed to Script::GetHeadingAngleConditionFunction 3)
	static bool GetHeadingAngleFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Inferred type: afReturnValue: double* from 0x5cb66c (passed to Script::IsWeaponOutConditionFunction 3)
	static bool IsWeaponOutFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetDefaultOpenFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Inferred type: afReturnValue: double* from 0x5d1196 (passed to Script::IsTimePassingConditionFunction 3)
	static bool IsTimePassingFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetCellOwnershipFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Inferred type: afReturnValue: double* from 0x5d1a36 (passed to Script::IsSneakingConditionFunction 3)
	static bool IsSneakingFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Inferred type: afReturnValue: double* from 0x5d2506 (passed to Script::IsInInteriorConditionFunction 3)
	static bool IsInInteriorFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Inferred type: afReturnValue: double* from 0x5d7521 (passed to Script::GetKillerConditionFunction 3)
	static bool GetKillerFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Inferred type: afReturnValue: double* from 0x5d7581 (passed to Script::GetKillerObjectConditionFunction 3)
	static bool GetKillerObjectFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Inferred type: afReturnValue: double* from 0x5d75ee (passed to Script::GetFactionCombatReactionConditionFunction 3)
	static bool GetFactionCombatReactionFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetFactionRelationFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool IsLastIdlePlayedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetIsAlignmentFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Inferred type: afReturnValue: double* from 0x5db581 (passed to Script::GetVATSRightAreaFreeConditionFunction 3)
	static bool GetVATSRightAreaFreeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Inferred type: afReturnValue: double* from 0x5db5e1 (passed to Script::GetVATSLeftAreaFreeConditionFunction 3)
	static bool GetVATSLeftAreaFreeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Name chosen here; the PDB's is Script::GetReputationFunction (GetReputationValue for a script command).
	static bool GetReputationFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	// Name chosen here; the PDB's is Script::GetReputationPctFunction (GetReputationPct for a script command).
	static bool GetReputationPctFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool ChallengeLockedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);
	static bool GetChallengeCompletedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
		TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
		double& afReturnValue, uint32_t& auiOffset);

	SCRIPT_HEADER m_header;
	// the decompiled source text
	char* m_text;
	// the compiled data
	char* m_data;
	float fProfilerTimer;
	float fQuestScriptDelay;
	float fQuestScriptGetSecondsBuffer;
	TESQuest* pOwnerQuest;
	// the forms the compiled data references
	BSSimpleList<RefVariable*> listRefObjects;
	// the script's local variables
	BSSimpleList<ScriptVariable*> listVariables;

	static BSCriticalSection ScriptRefListCrit;
	static BSSimpleList<TESObjectREFR*> ReferencesToDisable;
	static BSSimpleList<TESObjectREFR*> ReferencesToEnable;
	static BSSimpleList<TESObjectREFR*> ReferencesToDelete;
	static BSSimpleList<TESObjectREFR*> ReferencesToFadeDisable;
	// The button the player picked in the last message box; -1 for none.
	static inline char m_cButtonValue = -1;
	// The owner of the message box whose button is being waited for (0 for none).
	static inline int32_t m_iButtonOwner;
};

inline constexpr fnvd::Vtable Script_vftable{};

// Whether the pathing info is shown (what Script::TogglePathingInfoFunction flips).
inline bool g_bIsShowPathingInfo;

// IDA names; not in the PDB.
inline bool bIsMagicStats;
inline bool bIsConversationStats;
inline bool bIsProjectileDebug;

// Set while the game runs its per-frame script processing pass.
inline bool bIsScriptProcessing = true;
