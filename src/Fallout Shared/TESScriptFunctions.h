#pragma once

#include "BSHavok/bhkWorldNS.h"
#include "BSMain/Setting.h"
#include "Fallout/AI/ProcessLists.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class ImageSpaceModifierInstanceRB;
struct SCRIPT_LINE;
struct SCRIPT_PARAMETER;
class Script;
class ScriptCompileData;
class ScriptLocals;

// The bhkWorld::RecFunction Script::SetRigidBodyMassBase runs over a reference's collision objects: sets
// the mass of apCollision's rigid body, and of its stored dynamic motion, to the float in arData.
void SetRBMass(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
// BSShaderManager's shader error callback, which Script::RefreshShaders installs: prints the error text to the
// console.
void ConsoleShaderError(const char* apErrorString, const NiAVObject* apObject);
// Inferred type: afReturnValue: double* from 0x5c7416 (passed to Script::GetWeatherPercentConditionFunction 3)
bool Cmd_GetCurrentWeatherPercent_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c48c6 (passed to Script::GetDisabledConditionFunction 3)
bool Cmd_GetDisabled_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c4926 (passed to Script::GetIsLockBrokenConditionFunction 3)
bool au_re_Cmd_GetIsLockBroken(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_RemoveFromAllFactions(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c83d6 (passed to Script::GetXPForNextLevelConditionFunction 3)
bool au_re_Cmd_GetXPForNextLevel(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_IsActorDetected_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowTraitSelectMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowTraitMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cff96 (passed to Script::GetGhostConditionFunction 3)
bool Cmd_GetIsGhost_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d94c6 (passed to Script::GetMinorCrimeCountConditionFunction 3)
bool au_re_Cmd_GetMinorCrimeCount(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_DrawSkeleton(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5dc5a6 (passed to Script::IsGoreDisabledConditionFunction 3)
bool au_re_Cmd_IsGoreDisabled(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool au_re_Cmd_SetArgTo0AndReturnTrue_0(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d4b46 (passed to Script::GetTimeDeadConditionFunction 3)
bool Cmd_GetTimeDead_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ToggleCombatStats(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_AddCardToPlayer(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d93f6 (passed to Script::GetPlayerTeammateConditionFunction 3)
bool Cmd_GetPlayerTeammate_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowWhoDetectsPlayer(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_DispelAllSpells(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c48e6 (passed to Script::GetLockedConditionFunction 3)
bool au_re_Cmd_GetLocked(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool au_re_Cmd_SameFactionAsPC(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c79b6 (passed to Script::GetIsPlayableRaceConditionFunction 3)
bool au_re_Cmd_GetIsPlayableRace(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d4aa6 (passed to Script::IsActorConditionFunction 3)
bool Cmd_IsActor_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d94e6 (passed to Script::GetMajorCrimeCountConditionFunction 3)
bool au_re_Cmd_GetMajorCrimeCount(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5da286 (passed to Script::GetAggroRadiusViolatedConditionFunction 3)
bool au_re_Cmd_GetActorAggroRadiusViolated(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool GetForceHitReactionFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_AddFaceAnimNote(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cfdd6 (passed to Script::HasBeenEatenConditionFunction 3)
bool au_re_Cmd_HasBeenEaten(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d4a56 (passed to Script::IsEssentialConditionFunction 3)
bool Cmd_IsEssential_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d4af6 (passed to Script::IsPlayerMovingIntoNewSpaceConditionFunction 3)
bool Cmd_IsPlayerMovingIntoNewSpace_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_DropMe_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_DeprecatedDOF(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_CloseAllMenus(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_RemoveMe_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_DuplicateAllItems_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_RemoveAllTypedItems(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_RemoveAllItems(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_IsOwner_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetOwnership(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_Drop_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_IsActionRef(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_PlayerSpellBook(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_PrintAiList(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetWeather(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ForceWeather(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ReleaseWeatherOverride(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ApplyImageSpaceModifier(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetCellImageSpace(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetImageSpace(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ModScale(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c1d21 (passed to Script::GetLOSConditionFunction 3)
bool Cmd_GetLineOfSight(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_RemoveSpell(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_CastImmediateOnSelf(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_Dispel(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_DisableAllActors(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_GetIsFormType(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c6a31 (passed to Script::GetDetectedConditionFunction 3)
bool Cmd_GetDetected_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c6ff1 (passed to Script::GetQuestRunningConditionFunction 3)
bool Cmd_GetQuestRunning_0(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_StartQuest(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_StopQuest(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_CompleteQuest(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ModFactionRank_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_GetFactionReaction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetFactionReaction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetAlly(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetEnemy(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ModFactionReaction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowPivot(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cd48d (passed to Script::GetIsCurrentPackageConditionFunction 3)
bool Cmd_GetIsCurrentPackage_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowSlotMachineMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowBlackJackMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowRouletteMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowCaravanMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_IsPlayerInRegion(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowComputersInterface(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cfd96 (passed to Script::GetSandmanConditionFunction 3)
bool GetSandmanFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cfdb6 (passed to Script::GetCannibalConditionFunction 3)
bool GetCannibalFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_StartMisterSandMan(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_StartCannibal(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetGhost_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_EquipItem_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_UnequipItem_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetClass_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ResetInterior(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_WaterShallowColor(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_WaterDeepColor(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_WaterReflectionColor(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_DamageObject(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ResetAllTerminals(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ExitGame_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: apCompiledParams: char* from 0x5d8c89 (passed to Script::ParseString 1); auiOffset: uint32_t* from
// 0x5d8c71 (passed to Script::ParseString 7)
bool Cmd_ShowWarning_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_EnterTrigger_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetConsoleOutputFile_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_TestRaceSexTerimnal(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetPlayerTeammate_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_OpenTeammateContainer(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ClearFactionPlayerEnemyFlag(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ResetQuest(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetQuestDelay(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ClearDestruction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_FireWeapon(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// IDA name; the PDB's is Script::SetWeaponHealthPercFunction.
bool Cmd_SetWeaponHealthPerc(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// IDA name; the PDB's is Script::ModWeaponHealthPercFunction.
bool Cmd_ModWeaponHealthPerc(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetRumble(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetNoActivationSound(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_Abs(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// IDA name; the PDB's is Script::EnableWeaponModFunction.
bool Cmd_EnableWeaponMod(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_AddReputation(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// IDA name; the PDB's is Script::AddReputationExactFunction.
bool Cmd_AddReputationExact(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_RemoveReputation(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// IDA name; the PDB's is Script::RemoveReputationExactFunction.
bool Cmd_RemoveReputationExact(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetReputation(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_DetonatePlacedExplosives(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_IsLoadingMenuEnabled(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_IsActorRefEssential(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetActorRefEssential(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_IsNoHumanGoreExe(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_IncrementScriptedChallenge(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_PersistentRefCount(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_AlwaysShowActorSubtitles(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// IDA name; not in the PDB. IDA has it as a usercall on edi and esi; the exe reads neither
// and takes the eight stack arguments of a SCRIPT_FUNCTION::pExecuteFunction.
bool Cmd_ModWaterShader(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_EssentialDeathReload_Parse(const uint16_t asParamCount, const SCRIPT_PARAMETER* apParameters,
	SCRIPT_LINE* apScriptLine, ScriptCompileData* apCompileData);
bool Cmd_ShowMessage_Parse(const uint16_t asParamCount, const SCRIPT_PARAMETER* apParameters, SCRIPT_LINE* apScriptLine,
	ScriptCompileData* apCompileData);
bool Cmd_ShowWarning_Parse(const uint16_t asParamCount, const SCRIPT_PARAMETER* apParameters, SCRIPT_LINE* apScriptLine,
	ScriptCompileData* apCompileData);
// The compile function of the message commands: the three compile
// functions that call it pass their own four parameters on, then two flags as byte
// constants, which it reads as bytes.
bool Cmd_NonDefault_Parse(const uint16_t asParamCount, const SCRIPT_PARAMETER* apParameters, SCRIPT_LINE* apScriptLine,
	ScriptCompileData* apCompileData, bool a5, bool a6);
bool Cmd_EssentialDeathReload_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_PrintDeprecatedReturnTrue(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
uint32_t GetTextureDumpSize(NiTexture* apTexture);
bool Cmd_OutputTextureUseMap(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_TestAllCells(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_RenderTestCell(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_RenderTestHere(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ToggleConversations(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ToggleCollisionGeometry(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ToggleProjectileDebug(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
void ToggleProjectileDebug();
bool Cmd_SetTargetRefractionFire(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ToggleLiteBrite(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetDebugText(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ShowFullScenegraph(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetTerminalMenuPos(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_MovePipboyKnob(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ToggleTestLight(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ToggleCombatAI(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool ToggleCombatAI();
bool Cmd_ShowHeadTrackTarget(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ToggleHighProcess(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ToggleLowProcess(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ToggleMiddleLowProcess(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ToggleMiddleHighProcess(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SpeakSound(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_Help(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_DisableNavMesh_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_EnableNavmesh_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_CenterOnCell_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_CenterOnExterior(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_CenterOnWorld(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetSize(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_KillActor_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_KillAllActors(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// IDA name; the PDB's Script::ResurrectActorFunction.
bool Cmd_ResurrectActor(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5be6a6 (passed to Script::GetFatiguePercentageConditionFunction 3)
bool GetFatiguePercentageFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5be6c6 (passed to Script::GetHealthPercentageConditionFunction 3)
bool Cmd_GetHealthPercentage_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5be721 (passed to Script::GetBaseActorValue_Eval 3)
bool Cmd_GetBaseActorValue_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5be746 (passed to Script::GetCauseofDeathConditionFunction 3)
bool Cmd_GetCauseOfDeath_Eval(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_FriendDispositionBoost(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_GetGameSetting(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetGameSetting(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_GetIniSetting_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetINISetting(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_FadeSFX(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetLocationSpecificLoadScreensOnly(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool SetIsLocationSpecificLoadScreensOnly(bool a1);
bool Cmd_ShowQuests(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_Show1stPerson(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_LoadGame_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_Autosave(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ForceSave(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ToggleCellNode(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_DumpTexturePalette(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c6ab1 (passed to Script::GetItemCountConditionFunction 3)
bool Cmd_GetItemCount(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetShowQuestItems(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ResetHealth(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c6dd1 (passed to Script::GetEquippedConditionFunction 3)
bool Cmd_GetEquipped(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool GetInWorldspaceFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetCellPublicFlag(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c7711 (passed to Script::GetInSameCellConditionFunction 3)
bool GetInSameCellFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool GetIsClassFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetInChargen(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_GetInCharGen(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c78d1 (passed to Script::GetIsRaceConditionFunction 3)
bool GetIsRaceFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool GetIsCreatureTypeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SayTo_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ResetAI(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_EvaluatePackage_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_AddTopic(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetAlert(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c9706 (passed to Script::GetAlertConditionFunction 3)
bool au_re_Cmd_GetIsAlerted(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5c9726 (passed to Script::GetPlantedExplosiveConditionFunction 3)
bool GetPlantedExplosiveFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_CompleteAllObjectives(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_Look(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_StopLook_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetSkyParam(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
char Event_OnAdd(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj,
	Script* scriptObj, ScriptLocals* eventList, double* afReturnValue, uint32_t* opcodeOffsetPtr);
// Inferred type: a7: double* from 0x5ca2a1 (passed to Script::CheckFlagFunction 4)
char Event_OnSell(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj,
	Script* scriptObj, ScriptLocals* eventList, double* a7, uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5ca311 (passed to Script::CheckFlagFunction 4)
char Event_OnEquip(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj,
	Script* scriptObj, ScriptLocals* eventList, double* afReturnValue, uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5ca371 (passed to Script::CheckFlagFunction 4)
char Event_OnUnEquip(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* afReturnValue,
	uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5ca3d1 (passed to Script::CheckFlagFunction 4)
char Event_OnDrop(SCRIPT_PARAMETER* paramInfo, void* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj,
	Script* scriptObj, ScriptLocals* eventList, double* afReturnValue, uint32_t* opcodeOffsetPtr);
char Cmd_OnActivate_Execute(SCRIPT_PARAMETER* paramInfo, void* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, void* eventList, double* a7, uint32_t* opcodeOffsetPtr);
// IDA name; not in the PDB. The script event signature: the exe reads thisObj (+0x10) and afReturnValue (+0x20).
char Cmd_OnNPCActivate_Execute(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* afReturnValue,
	uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5ca51d (passed to Script::CheckFlagFunction 4)
char Event_OnFire(int32_t a1, int32_t a2, TESObjectREFR* apRefObject, int32_t a4, int32_t a5,
	ScriptLocals* apScriptLocals, double* afReturnValue);
// Inferred type: afReturnValue: double* from 0x5ca54d (passed to Script::CheckFlagFunction 4)
char Event_OnOpen(int32_t a1, int32_t a2, TESObjectREFR* apRefObject, int32_t a4, int32_t a5,
	ScriptLocals* apScriptLocals, double* afReturnValue);
// Inferred type: afReturnValue: double* from 0x5ca57d (passed to Script::CheckFlagFunction 4)
char Event_OnClose(int32_t a1, int32_t a2, TESObjectREFR* apRefObject, int32_t a4, int32_t a5,
	ScriptLocals* apScriptLocals, double* afReturnValue);
// Inferred type: afReturnValue: double* from 0x5ca5ad (passed to Script::CheckFlagFunction 4)
char Event_OnGrab(int32_t a1, int32_t a2, TESObjectREFR* apRefObject, int32_t a4, int32_t a5,
	ScriptLocals* apScriptLocals, double* afReturnValue);
// Inferred type: afReturnValue: double* from 0x5ca5dd (passed to Script::CheckFlagFunction 4)
char Event_OnRelease(int32_t a1, int32_t a2, TESObjectREFR* apRefObject, int32_t a4, int32_t a5,
	ScriptLocals* apScriptLocals, double* afReturnValue);
// Inferred type: afReturnValue: double* from 0x5ca648 (passed to Script::CheckFlagFunction 4)
char Event_OnDestructionStageChange(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* afReturnValue,
	uint32_t* opcodeOffsetPtr);
// Inferred type: a7: double* from 0x5ca6b1 (passed to Script::CheckFlagFunction 4)
char Event_OnHit(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj,
	Script* scriptObj, ScriptLocals* eventList, double* a7, uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5ca7a3 (passed to Script::CheckFlagFunction 4)
char Event_OnHitWith(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* afReturnValue,
	uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5ca841 (passed to Script::CheckFlagFunction 4)
char Event_OnDeath(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj,
	Script* scriptObj, ScriptLocals* eventList, double* afReturnValue, uint32_t* opcodeOffsetPtr);
// Inferred type: a7: double* from 0x5ca8a1 (passed to Script::CheckFlagFunction 4)
char Event_OnStartCombat(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* a7, uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5ca911 (passed to Script::CheckFlagFunction 4)
char Event_OnMurder(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj, TESObjectREFR* containingObj,
	Script* scriptObj, ScriptLocals* eventList, double* afReturnValue, uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5ca936 (passed to Script::CheckFlagFunction 4)
char Event_OnCombatEnd(int32_t a1, int32_t a2, TESObjectREFR* apRefObject, int32_t a4, int32_t a5,
	ScriptLocals* apScriptLocals, double* afReturnValue);
// IDA name; not in the PDB. The exe reads its eight arguments as every script event function's (0x5CA950).
char Event_SayToDone(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* afReturnValue,
	uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5ca9f1 (passed to Script::CheckFlagFunction 4)
char Event_OnPackageStart(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* afReturnValue,
	uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5caa61 (passed to Script::CheckFlagFunction 4)
char Event_OnPackageDone(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* afReturnValue,
	uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5caad1 (passed to Script::CheckFlagFunction 4)
char Event_OnPackageChange(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* afReturnValue,
	uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5cab06 (passed to Script::CheckFlagFunction 4)
char Event_OnLoad(int32_t a1, int32_t a2, TESObjectREFR* apRefObject, int32_t a4, int32_t a5,
	ScriptLocals* apScriptLocals, double* afReturnValue);
// Inferred type: afReturnValue: double* from 0x5cab36 (passed to Script::CheckFlagFunction 4)
char Event_OnReset(int32_t a1, int32_t a2, TESObjectREFR* apRefObject, int32_t a4, int32_t a5,
	ScriptLocals* apScriptLocals, double* afReturnValue);
// Inferred type: afReturnValue: double* from 0x5caba1 (passed to Script::CheckFlagFunction 4)
char Event_OnMagicEffectHit(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* afReturnValue,
	uint32_t* opcodeOffsetPtr);
// Inferred type: a7: double* from 0x5cac11 (passed to Script::CheckFlagFunction 4)
char Event_OnTrigger(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* a7, uint32_t* opcodeOffsetPtr);
// Inferred type: a7: double* from 0x5cac81 (passed to Script::CheckFlagFunction 4)
char Event_OnTriggerEnter(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* a7, uint32_t* opcodeOffsetPtr);
// Inferred type: afReturnValue: double* from 0x5cacf1 (passed to Script::CheckFlagFunction 4)
char Event_OnTriggerLeave(SCRIPT_PARAMETER* paramInfo, char* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* eventList, double* afReturnValue,
	uint32_t* opcodeOffsetPtr);
bool Cmd_EnablePlayerControls_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_DisablePlayerControls_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool GetPlayerControlsDisabledFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_PickIdle(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_PlayIdle(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool IsWeaponInListFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool IsTorchOutFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cb72c (passed to Script::IsFacingUpConditionFunction 3)
bool Cmd_IsFacingUp_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cb75c (passed to Script::IsLeftUpConditionFunction 3)
bool Cmd_IsLeftUp_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cb78c (passed to Script::GetKnockedStateConditionFunction 3)
bool au_re_Cmd_GetKnockedState(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool IsWeaponSkillTypeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cb85c (passed to Script::GetCurrentAIPackageConditionFunction 3)
bool Cmd_GetCurrentAIPackage_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cb88c (passed to Script::IsWaitingConditionFunction 3)
bool au_re_Cmd_IsWaiting(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cb8bc (passed to Script::IsIdlePlayingConditionFunction 3)
bool au_re_Cmd_IsIdlePlaying(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowQuestTargets_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// IDA name; not in the PDB. The exe returns _mbsnbcpy's result (the destination) and only reads Source.
char* mbsnbcpy_(char* Destination, const char* Source, uint32_t Count);
// IDA name; the PDB's Script::ShowFullQuestLogFunction.
bool Cmd_ShowFullQuestLog(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetPCFactionMurder(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetPCEnemyofFaction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetPCFactionAttack(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5ce434 (passed to Script::PutNumericIDInDouble 1)
bool Cmd_GetSelf_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5ce4de (passed to Script::PutNumericIDInDouble 1)
bool Cmd_GetCombatTarget_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5ce576 (passed to Script::PutNumericIDInDouble 1)
bool Cmd_GetPackageTarget_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5ce5ec (passed to Script::PutNumericIDInDouble 1)
bool Cmd_GetContainer(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5ce669 (passed to Script::PutNumericIDInDouble 1)
bool Cmd_GetParentRef(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5ce6e9 (passed to Script::PutNumericIDInDouble 1)
bool Cmd_GetLinkedRef(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_GetForceRun(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetForceRun(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// IDA name; the PDB's Script::GetForceSneak may be this command (not checked).
bool Cmd_GetForceSneak(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetForceSneak(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_AdvLevel_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cea71 (passed to Script::HasMagicEffectConditionFunction 3)
bool Cmd_HasMagicEffect_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5cead1 (passed to Script::IsSpellTargetConditionFunction 3)
bool IsSpellTargetFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5ceb31 (passed to Script::GetSpellUsageNumberConditionFunction 3)
bool GetSpellUsageNumberFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5ceb56 (passed to Script::GetVATSModeConditionFunction 3)
bool au_re_Cmd_GetVATSMode(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5ceb76 (passed to Script::GetVATSTargetHeightConditionFunction 3)
bool GetVATSTargetHeightFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// IDA name; the PDB's Script::SetDefaultOpenFunction.
bool Cmd_SetDefaultOpen(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ShowRaceMenu_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowBarberMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ShowPlasticSurgeonMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: apCompiledParams: char* from 0x5cee97 (passed to Script::GetPlayerName 1); afReturnValue: double* from
// 0x5cee83 (passed to Script::GetPlayerName 6); auiOffset: uint32_t* from 0x5cee7f (passed to Script::GetPlayerName 7)
bool Cmd_ShowNameMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ShowBarterMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d09c6 (passed to Script::GetRestrainedConditionFunction 3)
bool GetRestrainedFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ForceFlee(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ForceTakeCover(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d0c76 (passed to Script::GetOffersServiceConditionFunction 3)
bool au_re_Cmd_GetOffersServicesNow(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ModBarterGold(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetBarterGold(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d0e16 (passed to Script::GetBarterGoldConditionFunction 3)
bool au_re_Cmd_GetBarterGold(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetLevel_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d1116 (passed to Script::IsXboxConditionFunction 3)
bool au_re_Cmd_IsXBox(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d1136 (passed to Script::IsPS3ConditionFunction 3)
bool au_re_IsPS3(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d1156 (passed to Script::IsPCConditionFunction 3)
bool au_re_Cmd_IsPC(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d1176 (passed to Script::IsActorUsingATorchConditionFunction 3)
bool IsActorUsingATorchFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ModPCAttribute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_DamageWeapon(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_GetWeaponHealth(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetCellOwnership(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool au_re_Cmd_SetArgTo0AndReturnTrue_1(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetCellFullName(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetActorFullName(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d1a56 (passed to Script::IsRunningConditionFunction 3)
bool au_re_Cmd_GetIsActorRunning(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d1a76 (passed to Script::GetFriendHitConditionFunction 3)
bool au_re_Cmd_GetFriendHit(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d1a96 (passed to Script::IsInCombatConditionFunction 3)
bool Cmd_IsInCombat_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetPackDuration_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_PlayMagicShaderVisuals_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_PlayMagicEffectVisuals(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_StopMagicShaderVisuals(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_StopMagicEffectVisuals(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_PreloadMagicEffect(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetIgnoreFriendlyHits(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_GetIgnoreFriendlyHits_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetItemValue(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetActorRefraction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_RunCellTest(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_StartAllQuests(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetsNewGammaRamp_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetMaxAniso(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetTalkingActivatorActor(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_AddPerk_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_RemovePerk(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d4ff8 (passed to Script::HasPerkConditionFunction 3)
bool Cmd_HasPerk_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SendSherlockDebugText(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_RewardXP(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_RewardKarma(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ShowRepairMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_AddNote(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_RemoveNote(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_AddToFaction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_RemoveFromFaction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetDisposition(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d5636 (passed to Script::GetHitLocationConditionFunction 3)
bool Cmd_GetHitLocation_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d5686 (passed to Script::GetLastHitCriticalConditionFunction 3)
bool Cmd_GetLastHitCritical_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d56d6 (passed to Script::IsPC1stPersonConditionFunction 3)
bool Cmd_IsPC1stPerson_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetCombatActionCost(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ToggleCombatDebug(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetActorsAI(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ClearOwnership(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ShowInventory(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ShowChargenMenuParams(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowChargenMenuModValues(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetSPECIALPoints(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_AddSPECIALPoints(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetTagSkills(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_AddTagSkills(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_ShowLockpickMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowLockpickMenuDebug(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ResetPerformanceTimers(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ToggleDebugDecal(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetImageSpaceModifiersEnable(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d7946 (passed to Script::GetIsMovingConditionFunction 3)
bool Cmd_IsMoving(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d7996 (passed to Script::GetIsTurningConditionFunction 3)
bool Cmd_IsTurning(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5d79e6 (passed to Script::GetAnimActionConditionFunction 3)
bool Cmd_GetAnimAction_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_UseWeapon(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_EvaluateSpellConditions(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ClearScreenBlood(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_TriggerScreenSplatter(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetScreenSplatterFade(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetScreenSplatterTexture(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ClearScreenSplatter(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_AgeRace_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5db641 (passed to Script::GetVATSBackAreaFreeConditionFunction 3)
bool GetVATSBackAreaFreeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5db6a1 (passed to Script::GetVATSFrontAreaFreeConditionFunction 3)
bool GetVATSFrontAreaFreeFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
void Set(char a1);
// Inferred type: afReturnValue: double* from 0x5db731 (passed to Script::GetVATSRightTargetVisibleConditionFunction 3)
bool GetVATSRightTargetVisibleFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5db791 (passed to Script::GetVATSLeftTargetVisibleConditionFunction 3)
bool GetVATSLeftTargetVisibleFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5db7f1 (passed to Script::GetVATSBackTargetVisibleConditionFunction 3)
bool GetVATSBackTargetVisibleFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5db851 (passed to Script::GetVATSFrontTargetVisibleConditionFunction 3)
bool GetVATSFrontTargetVisibleFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetCriticalStage(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool IsInCriticalStageFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_Sin(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_Cos(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_Tan(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_Sqrt(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_Log(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// IDA name; the PDB's Script::ShowQuestStagesFunction.
bool Cmd_ShowQuestStages(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_SetTaskThreadUpdateSleep(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_AddFormToFormList_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_AddNPCToLeveledList(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_AddItemToLeveledList(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_RemapWaterType(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Inferred type: afReturnValue: double* from 0x5de906 (passed to Script::IsHardcoreConditionalFunction 3)
bool Cmd_IsHardcore_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// IDA name; the PDB's Script::SetHardcoreFunction (Fallout Shared\TESScriptFunctions.cpp).
bool Cmd_SetHardcore_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// IDA name; the PDB's Script::ObjectUnderReticleFunction (Fallout Shared\TESScriptFunctions.cpp).
bool Cmd_ObjectUnderReticle_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// IDA name; the PDB's Script::GetDLCCountFunction (Fallout Shared\TESScriptFunctions.cpp).
bool Cmd_GetDLCCount(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_IsDLCInstalled(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_GetContainerInventoryCount(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SystemSave(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
// Not named in the PDB or IDA; the Havok SDK's hkGetByteOffset (hkBaseTypes.h): the bytes from base to pntr.
int32_t hkGetByteOffset(const void* base, const void* pntr);

// The mob operator of Script::ResetDialogueFlags (0x5D8930): for each mob, calls its AI process's virtual at +0x310
// with 0 when the mob's virtual at +0x100 is true, then removes the mob's EXTRA_SAY_ONCE_A_DAY_TOPIC_INFO extra. The
// PDB has it as a class local to that function; src has no function-local classes, so it is defined in that
// function's file.
class MobDialogueFlagResetter : public MobIterOperator {
public:
	MobDialogueFlagResetter();

	void Operate(MobileObject* apMob) override;
};

using LifeStateString_t = char* [6];
inline LifeStateString_t LifeStateString{const_cast<char*>("Alive"), const_cast<char*>("Dying"),
	const_cast<char*>("Dead"), const_cast<char*>("Unconscious"), const_cast<char*>("Reanimate"),
	const_cast<char*>("Restrained")};
using SleepStateString_t = char* [11];
inline SleepStateString_t SleepStateString{const_cast<char*>(aNormal_0), const_cast<char*>("Load sit idle"),
	const_cast<char*>("Want to sit"), const_cast<char*>("Waiting for sit anim"), const_cast<char*>("Sitting"),
	const_cast<char*>("Want to stand"), const_cast<char*>("Loading sleep idle"), const_cast<char*>("Want to sleep"),
	const_cast<char*>("Waiting for sleep anim"), const_cast<char*>("Sleeping"), const_cast<char*>("Want to wake")};
using KnockStateString_t = char* [7];
inline KnockStateString_t KnockStateString{const_cast<char*>(aNormal_0), const_cast<char*>("Explode"),
	const_cast<char*>("Explode Lead In"), const_cast<char*>("Knocked Out"), const_cast<char*>("Knock Out Lead In"),
	const_cast<char*>("Queued"), const_cast<char*>("Getting Up")};
inline bool bMenusOn = true;
inline char bBordersOn;
using animGroupSectionNames_t = char* [8];
inline animGroupSectionNames_t animGroupSectionNames{const_cast<char*>("Idle"), const_cast<char*>("Movement"),
	const_cast<char*>("LeftArm"), const_cast<char*>("LeftHand"), const_cast<char*>("Weapon"),
	const_cast<char*>("WeaponUp"), const_cast<char*>("WeaponDown"), const_cast<char*>("SpecialIdle")};
using pProcessTypeString_t = char* [4];
inline pProcessTypeString_t pProcessTypeString{const_cast<char*>("High"), const_cast<char*>("Middle High"),
	const_cast<char*>("Middle Low"), const_cast<char*>("Low")};
using pAnimationActionStrings_t = char* [18];
inline pAnimationActionStrings_t pAnimationActionStrings{const_cast<char*>("Equip Weapon"),
	const_cast<char*>("Unequip Weapon"), const_cast<char*>("Attack"), const_cast<char*>("Attack Eject"),
	const_cast<char*>("Attack Follow Through"), const_cast<char*>("Attack Throw"),
	const_cast<char*>("Attack Throw Attach"), const_cast<char*>("Block"), const_cast<char*>("Recoil"),
	const_cast<char*>("Reload"), const_cast<char*>("Stagger"), const_cast<char*>("Dodge"),
	const_cast<char*>("Lower Body Anim"), const_cast<char*>("Special Idle"), const_cast<char*>("Script Animation"),
	const_cast<char*>("Reload Loop Start"), const_cast<char*>("Reload Loop End"), const_cast<char*>("Reload Loop")};
inline bool s_ToggleSky;
inline bool g_bIsToggleWireframe;
inline ImageSpaceModifierInstanceRB* g_radialBlurIMOD;
inline constexpr fnvd::Vtable MobDialogueFlagResetter_vftable{};
using pGSMonth_t = SettingT<GameSettingCollection>* [12];
extern pGSMonth_t pGSMonth;
inline char g_bIsUseModdedWaterShaders;
inline float g_fWaterFresnelTerm;
inline float g_fWaterDisplaceDampener;
inline bool bTestLightOn;
inline char bDumpNextFrame;
inline char g_enableWireframeDecals;
