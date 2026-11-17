#pragma once

#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESReputation.h"
#include "Reconstruction.hpp"

class TESWorldSpace;

// The BSRandom singleton's UnsignedInt(aiMax): a random number below aiMax (cdecl, ret 0, 0x5A00A0). Not
// reconstructed yet.
uint32_t RandomUnsignedInt(uint32_t aiMax);
bool Cmd_GetPCIsClass(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
// Inferred type: afReturnValue: double* from 0x59f756 (passed to Script::GetIsRaceConditionFunction 3)
bool Cmd_GetPCIsRace(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
// Inferred type: afReturnValue: double* from 0x59f776 (passed to Script::GetIsSexConditionFunction 3)
bool Cmd_GetPCIsSex(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
// Inferred type: afReturnValue: double* from 0x59f796 (passed to Script::GetInFactionConditionFunction 3)
bool Cmd_GetPCInFaction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetInWorldspace(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
// apRefObject is read as a reference: the body calls TESObjectREFR::GetObjectReference
// on it and Cmd_GetPCIsClass passes the player.
bool Cmd_GetIsClass(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetIsCreatureType(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetQuestVariable_Eval(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetQuestCompleted(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetPlayerControlsDisabled(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetPCExpelled(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetPCFactionMurder(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_IsInCriticalStage(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_IsOwner_Eval(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_IsCellOwner(TESObjectREFR* apRefObject, void* apParam1, TESForm* apParam2, double& afReturnValue);
bool Cmd_GetPCMiscStat(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_Exists(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
// Inferred type: apParam1: TESActorBaseData::ALIGNMENT: compared with result of TESActorBaseData::GetAlignmentForKarma
// at 0x5a4e2e
bool Cmd_GetIsAlignment(TESObjectREFR* apRefObject, TESActorBaseData::ALIGNMENT apParam1, void* apParam2,
	double& afReturnValue);
bool Cmd_GetConcussed(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
char get_bConcussion_11DBE50();
bool Cmd_GetForceHitReaction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetReputation(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
// Inferred type: apParam2: ReputationType: passed to TESReputation::GetReputationPct 0 at 0x5a5c5b
bool Cmd_GetReputationPct(TESObjectREFR* apRefObject, void* apParam1, ReputationType apParam2, double& afReturnValue);
bool Cmd_GetReputationThreshold(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetObjectiveCompleted_Eval(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_PlayerInRegion(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

// Set by the menu mode setting: MenuMode n asks whether menu n is the top menu instead of whether it is shown.
inline bool bUseOldMenuModeScriptFunction;
using rdata_10357B8_t = float[4];
inline rdata_10357B8_t rdata_10357B8{45.0f, 21.0f, 0.0f, -1.0f};
using rdata_10357C8_t = float[7];
inline rdata_10357C8_t rdata_10357C8{45.0f, 48.0f, 8.0f, 10.0f, 7.0f, 46.0f, 4.0f};

// Cached condition function arguments, invalid until the next check refills them.
inline void* pDispositionActor;
inline void* pDispositionTarget;
inline float fDisposition;
inline void* pInCellRef;
inline void* pInCell;
inline float fInCell;
inline void* pFaction;
inline void* pInFactionRef;
inline float fInFaction;
inline void* pOPTidobj;
inline void* pOPTidref;
inline void* pOPTvoice;
inline void* pOPTvoiceref;
inline void* pIsPlayableRaceRef;
inline float fIsPlayableRace;

// The names of the seven player control flags, which GetPlayerControlsDisabled prints.
using pPlayerControls_t = const char* [7];
inline pPlayerControls_t pPlayerControls{"Movement", "Looking", "Pipboy", "Fighting", "POV Switch", "Rollover Text",
	"Sneaking"};
