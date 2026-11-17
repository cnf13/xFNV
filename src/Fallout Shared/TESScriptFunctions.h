#pragma once

#include "BSHavok/bhkWorldNS.h"
#include "BSMain/Setting.h"
#include "Fallout/AI/ProcessLists.h"

#include <stdint.h>

class ImageSpaceModifierInstanceRB;
class Script;
struct SCRIPT_LINE;
struct SCRIPT_PARAMETER;
class ScriptCompileData;
class ScriptLocals;

// The bhkWorld::RecFunction Script::SetRigidBodyMassBase runs over a reference's collision objects: sets
// the mass of apCollision's rigid body, and of its stored dynamic motion, to the float in arData.
void SetRBMass(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
// BSShaderManager's shader error callback, which Script::RefreshShaders installs: prints the error text to the
// console.
void ConsoleShaderError(const char* apErrorString, const NiAVObject* apObject);
bool Cmd_GetCurrentWeatherPercent_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetDisabled_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetIsLockBroken(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RemoveFromAllFactions(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetXPForNextLevel(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsActorDetected_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowTraitSelectMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowTraitMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetIsGhost_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetMinorCrimeCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_DrawSkeleton(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_IsGoreDisabled(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_SetArgTo0AndReturnTrue_0(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetTimeDead_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleCombatStats(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddCardToPlayer(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetPlayerTeammate_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowWhoDetectsPlayer(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_DispelAllSpells(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetLocked(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_SameFactionAsPC(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetIsPlayableRace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsActor_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetMajorCrimeCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetActorAggroRadiusViolated(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetForceHitReactionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddFaceAnimNote(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_HasBeenEaten(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsEssential_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsPlayerMovingIntoNewSpace_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_DropMe_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_DeprecatedDOF(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_CloseAllMenus(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RemoveMe_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_DuplicateAllItems_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RemoveAllTypedItems(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RemoveAllItems(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsOwner_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetOwnership(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_Drop_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsActionRef(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_PlayerSpellBook(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_PrintAiList(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetWeather(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ForceWeather(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ReleaseWeatherOverride(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ApplyImageSpaceModifier(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetCellImageSpace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetImageSpace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ModScale(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetLineOfSight(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RemoveSpell(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_CastImmediateOnSelf(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_Dispel(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_DisableAllActors(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetIsFormType(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetDetected_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetQuestRunning_0(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_StartQuest(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_StopQuest(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_CompleteQuest(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ModFactionRank_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetFactionReaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetFactionReaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetAlly(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetEnemy(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ModFactionReaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowPivot(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetIsCurrentPackage_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowSlotMachineMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowBlackJackMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowRouletteMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowCaravanMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsPlayerInRegion(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowComputersInterface(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetSandmanFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetCannibalFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_StartMisterSandMan(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_StartCannibal(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetGhost_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_EquipItem_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_UnequipItem_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetClass_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ResetInterior(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_WaterShallowColor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_WaterDeepColor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_WaterReflectionColor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_DamageObject(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ResetAllTerminals(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ExitGame_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowWarning_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_EnterTrigger_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetConsoleOutputFile_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_TestRaceSexTerimnal(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetPlayerTeammate_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_OpenTeammateContainer(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ClearFactionPlayerEnemyFlag(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ResetQuest(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetQuestDelay(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ClearDestruction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_FireWeapon(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetWeaponHealthPerc(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ModWeaponHealthPerc(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetRumble(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetNoActivationSound(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_Abs(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_EnableWeaponMod(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddReputation(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddReputationExact(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RemoveReputation(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RemoveReputationExact(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetReputation(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_DetonatePlacedExplosives(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsLoadingMenuEnabled(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsActorRefEssential(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetActorRefEssential(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsNoHumanGoreExe(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IncrementScriptedChallenge(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_PersistentRefCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AlwaysShowActorSubtitles(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ModWaterShader(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_EssentialDeathReload_Parse(
    const uint16_t asParamCount,
    const SCRIPT_PARAMETER* apParameters,
    SCRIPT_LINE* apScriptLine,
    ScriptCompileData* apCompileData
);
bool Cmd_ShowMessage_Parse(
    const uint16_t asParamCount,
    const SCRIPT_PARAMETER* apParameters,
    SCRIPT_LINE* apScriptLine,
    ScriptCompileData* apCompileData
);
bool Cmd_ShowWarning_Parse(
    const uint16_t asParamCount,
    const SCRIPT_PARAMETER* apParameters,
    SCRIPT_LINE* apScriptLine,
    ScriptCompileData* apCompileData
);
// The compile function of the message commands: the three compile
// functions that call it pass their own four parameters on, then two flags as byte
// constants, which it reads as bytes.
bool Cmd_NonDefault_Parse(
    const uint16_t asParamCount,
    const SCRIPT_PARAMETER* apParameters,
    SCRIPT_LINE* apScriptLine,
    ScriptCompileData* apCompileData,
    bool a5,
    bool a6
);
bool Cmd_EssentialDeathReload_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_PrintDeprecatedReturnTrue(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
uint32_t GetTextureDumpSize(NiTexture* apTexture);
bool Cmd_OutputTextureUseMap(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_TestAllCells(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RenderTestCell(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RenderTestHere(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleConversations(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleCollisionGeometry(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleProjectileDebug(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
void ToggleProjectileDebug();
bool Cmd_SetTargetRefractionFire(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleLiteBrite(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetDebugText(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowFullScenegraph(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetTerminalMenuPos(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_MovePipboyKnob(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleTestLight(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleCombatAI(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool ToggleCombatAI();
bool Cmd_ShowHeadTrackTarget(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleHighProcess(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleLowProcess(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleMiddleLowProcess(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleMiddleHighProcess(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SpeakSound(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_Help(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_DisableNavMesh_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_EnableNavmesh_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_CenterOnCell_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_CenterOnExterior(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_CenterOnWorld(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetSize(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_KillActor_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_KillAllActors(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ResurrectActor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetFatiguePercentageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetHealthPercentage_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetBaseActorValue_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetCauseOfDeath_Eval(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_FriendDispositionBoost(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetGameSetting(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetGameSetting(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetIniSetting_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetINISetting(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_FadeSFX(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetLocationSpecificLoadScreensOnly(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool SetIsLocationSpecificLoadScreensOnly(bool a1);
bool Cmd_ShowQuests(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_Show1stPerson(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_LoadGame_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_Autosave(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ForceSave(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleCellNode(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_DumpTexturePalette(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetItemCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetShowQuestItems(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ResetHealth(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetEquipped(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetInWorldspaceFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetCellPublicFlag(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetInSameCellFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetIsClassFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetInChargen(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetInCharGen(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetIsRaceFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetIsCreatureTypeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SayTo_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ResetAI(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_EvaluatePackage_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddTopic(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetAlert(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetIsAlerted(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetPlantedExplosiveFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_CompleteAllObjectives(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_Look(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_StopLook_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetSkyParam(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
char Event_OnAdd(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnSell(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* a7,
    uint32_t* opcodeOffsetPtr
);
char Event_OnEquip(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnUnEquip(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnDrop(
    SCRIPT_PARAMETER* paramInfo,
    void* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Cmd_OnActivate_Execute(
    SCRIPT_PARAMETER* paramInfo,
    void* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    void* eventList,
    double* a7,
    uint32_t* opcodeOffsetPtr
);
// The script event signature: reads thisObj and afReturnValue.
char Cmd_OnNPCActivate_Execute(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnFire(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
);
char Event_OnOpen(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
);
char Event_OnClose(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
);
char Event_OnGrab(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
);
char Event_OnRelease(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
);
char Event_OnDestructionStageChange(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnHit(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* a7,
    uint32_t* opcodeOffsetPtr
);
char Event_OnHitWith(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnDeath(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnStartCombat(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* a7,
    uint32_t* opcodeOffsetPtr
);
char Event_OnMurder(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnCombatEnd(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
);
char Event_SayToDone(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnPackageStart(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnPackageDone(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnPackageChange(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnLoad(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
);
char Event_OnReset(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
);
char Event_OnMagicEffectHit(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
char Event_OnTrigger(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* a7,
    uint32_t* opcodeOffsetPtr
);
char Event_OnTriggerEnter(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* a7,
    uint32_t* opcodeOffsetPtr
);
char Event_OnTriggerLeave(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
);
bool Cmd_EnablePlayerControls_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_DisablePlayerControls_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetPlayerControlsDisabledFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_PickIdle(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_PlayIdle(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool IsWeaponInListFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool IsTorchOutFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsFacingUp_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsLeftUp_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetKnockedState(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool IsWeaponSkillTypeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetCurrentAIPackage_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_IsWaiting(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_IsIdlePlaying(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowQuestTargets_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
// Returns _mbsnbcpy's result (the destination) and only reads Source.
char* mbsnbcpy_(char* Destination, const char* Source, uint32_t Count);
bool Cmd_ShowFullQuestLog(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetPCFactionMurder(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetPCEnemyofFaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetPCFactionAttack(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetSelf_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetCombatTarget_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetPackageTarget_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetContainer(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetParentRef(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetLinkedRef(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetForceRun(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetForceRun(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetForceSneak(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetForceSneak(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AdvLevel_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_HasMagicEffect_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool IsSpellTargetFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetSpellUsageNumberFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetVATSMode(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetVATSTargetHeightFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetDefaultOpen(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowRaceMenu_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowBarberMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowPlasticSurgeonMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowNameMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowBarterMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetRestrainedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ForceFlee(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ForceTakeCover(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetOffersServicesNow(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ModBarterGold(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetBarterGold(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetBarterGold(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetLevel_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_IsXBox(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_IsPS3(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_IsPC(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool IsActorUsingATorchFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ModPCAttribute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_DamageWeapon(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetWeaponHealth(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetCellOwnership(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_SetArgTo0AndReturnTrue_1(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetCellFullName(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetActorFullName(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetIsActorRunning(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool au_re_Cmd_GetFriendHit(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsInCombat_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetPackDuration_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_PlayMagicShaderVisuals_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_PlayMagicEffectVisuals(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_StopMagicShaderVisuals(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_StopMagicEffectVisuals(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_PreloadMagicEffect(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetIgnoreFriendlyHits(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetIgnoreFriendlyHits_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetItemValue(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetActorRefraction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RunCellTest(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_StartAllQuests(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetsNewGammaRamp_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetMaxAniso(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetTalkingActivatorActor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddPerk_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RemovePerk(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_HasPerk_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SendSherlockDebugText(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RewardXP(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RewardKarma(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowRepairMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddNote(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RemoveNote(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddToFaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RemoveFromFaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetDisposition(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetHitLocation_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetLastHitCritical_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsPC1stPerson_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetCombatActionCost(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleCombatDebug(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetActorsAI(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ClearOwnership(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowInventory(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowChargenMenuParams(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowChargenMenuModValues(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetSPECIALPoints(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddSPECIALPoints(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetTagSkills(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddTagSkills(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowLockpickMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowLockpickMenuDebug(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ResetPerformanceTimers(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ToggleDebugDecal(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetImageSpaceModifiersEnable(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsMoving(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsTurning(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetAnimAction_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_UseWeapon(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_EvaluateSpellConditions(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ClearScreenBlood(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_TriggerScreenSplatter(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetScreenSplatterFade(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetScreenSplatterTexture(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ClearScreenSplatter(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AgeRace_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetVATSBackAreaFreeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetVATSFrontAreaFreeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
void Set(char a1);
bool GetVATSRightTargetVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetVATSLeftTargetVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetVATSBackTargetVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool GetVATSFrontTargetVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetCriticalStage(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool IsInCriticalStageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_Sin(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_Cos(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_Tan(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_Sqrt(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_Log(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ShowQuestStages(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetTaskThreadUpdateSleep(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddFormToFormList_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddNPCToLeveledList(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AddItemToLeveledList(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_RemapWaterType(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsHardcore_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetHardcore_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_ObjectUnderReticle_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetDLCCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsDLCInstalled(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetContainerInventoryCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SystemSave(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
// The bytes from base to pntr.
int32_t hkGetByteOffset(const void* base, const void* pntr);

// The mob operator of Script::ResetDialogueFlags: for each mob, removes the mob's EXTRA_SAY_ONCE_A_DAY_TOPIC_INFO
// extra.
class MobDialogueFlagResetter : public MobIterOperator {
public:
    MobDialogueFlagResetter();

    void Operate(MobileObject* apMob) override;
};

using LifeStateString_t = char* [6];
inline LifeStateString_t LifeStateString{const_cast<char*>("Alive"),     const_cast<char*>("Dying"),
                                         const_cast<char*>("Dead"),      const_cast<char*>("Unconscious"),
                                         const_cast<char*>("Reanimate"), const_cast<char*>("Restrained")};
using SleepStateString_t = char* [11];
inline SleepStateString_t SleepStateString{
    const_cast<char*>(aNormal_0),
    const_cast<char*>("Load sit idle"),
    const_cast<char*>("Want to sit"),
    const_cast<char*>("Waiting for sit anim"),
    const_cast<char*>("Sitting"),
    const_cast<char*>("Want to stand"),
    const_cast<char*>("Loading sleep idle"),
    const_cast<char*>("Want to sleep"),
    const_cast<char*>("Waiting for sleep anim"),
    const_cast<char*>("Sleeping"),
    const_cast<char*>("Want to wake")
};
using KnockStateString_t = char* [7];
inline KnockStateString_t KnockStateString{const_cast<char*>(aNormal_0),           const_cast<char*>("Explode"),
                                           const_cast<char*>("Explode Lead In"),   const_cast<char*>("Knocked Out"),
                                           const_cast<char*>("Knock Out Lead In"), const_cast<char*>("Queued"),
                                           const_cast<char*>("Getting Up")};
inline bool bMenusOn = true;
inline char bBordersOn;
using animGroupSectionNames_t = char* [8];
inline animGroupSectionNames_t animGroupSectionNames{const_cast<char*>("Idle"),       const_cast<char*>("Movement"),
                                                     const_cast<char*>("LeftArm"),    const_cast<char*>("LeftHand"),
                                                     const_cast<char*>("Weapon"),     const_cast<char*>("WeaponUp"),
                                                     const_cast<char*>("WeaponDown"), const_cast<char*>("SpecialIdle")};
using pProcessTypeString_t = char* [4];
inline pProcessTypeString_t pProcessTypeString{
    const_cast<char*>("High"), const_cast<char*>("Middle High"), const_cast<char*>("Middle Low"),
    const_cast<char*>("Low")
};
using pAnimationActionStrings_t = char* [18];
inline pAnimationActionStrings_t pAnimationActionStrings{
    const_cast<char*>("Equip Weapon"),
    const_cast<char*>("Unequip Weapon"),
    const_cast<char*>("Attack"),
    const_cast<char*>("Attack Eject"),
    const_cast<char*>("Attack Follow Through"),
    const_cast<char*>("Attack Throw"),
    const_cast<char*>("Attack Throw Attach"),
    const_cast<char*>("Block"),
    const_cast<char*>("Recoil"),
    const_cast<char*>("Reload"),
    const_cast<char*>("Stagger"),
    const_cast<char*>("Dodge"),
    const_cast<char*>("Lower Body Anim"),
    const_cast<char*>("Special Idle"),
    const_cast<char*>("Script Animation"),
    const_cast<char*>("Reload Loop Start"),
    const_cast<char*>("Reload Loop End"),
    const_cast<char*>("Reload Loop")
};
inline bool s_ToggleSky;
inline bool g_bIsToggleWireframe;
inline ImageSpaceModifierInstanceRB* g_radialBlurIMOD;
using pGSMonth_t = SettingT<GameSettingCollection>* [12];
extern pGSMonth_t pGSMonth;
inline char g_bIsUseModdedWaterShaders;
inline float g_fWaterFresnelTerm;
inline float g_fWaterDisplaceDampener;
inline bool bTestLightOn;
inline char bDumpNextFrame;
inline char g_enableWireframeDecals;
