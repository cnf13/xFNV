#pragma once

#include "BSCore/BSRandom.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESReputation.h"

class TESWorldSpace;

// Set by the menu mode setting: MenuMode n asks whether menu n is the top menu instead of whether it is shown.
inline bool bUseOldMenuModeScriptFunction;
// GetCurrentAIProcedure's results for a sandbox package: by sandbox state, and by the sandbox procedure (plus one)
// while that procedure runs.
using SandboxStateProcedureTable = float[4];
inline SandboxStateProcedureTable fSandboxStateProcedures{45.0f, 21.0f, 0.0f, -1.0f};
using SandboxProcedureTable = float[7];
inline SandboxProcedureTable fSandboxProcedures{45.0f, 48.0f, 8.0f, 10.0f, 7.0f, 46.0f, 4.0f};

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
inline pPlayerControls_t
    pPlayerControls{"Movement", "Looking", "Pipboy", "Fighting", "POV Switch", "Rollover Text", "Sneaking"};
