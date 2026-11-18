#pragma once

#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class CombatLocation;
class PathingAvoidNodeArray;
class PathingRequest;

class CombatProcedureMove : public CombatProcedure {
public:
    enum MOVE_STATE : int32_t {
        MOVE_STATE_INITIALIZING = 0x0,
        MOVE_STATE_MOVING = 0x1,
        MOVE_STATE_MOVING_USING_COVER = 0x2,
        MOVE_STATE_HIDING_BEHIND_COVER = 0x3,
    };

    CombatProcedureMove(CombatLocation* apCombatLocation, float afTargetRadius, int32_t aiLocation);
    ~CombatProcedureMove() override;

    void Update() override;
    void Shutdown() override;
    void ClearReferencePointers(TESObjectREFR* apRef) override;
    void DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify) override;
    void StartedPlanning() override;
    void FinishedPlanning() override;
    CombatProcedure::COMBAT_PROCEDURES GetType() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

    // Plans a path for the procedure.
    bool CreatePath(NiPointer<PathingAvoidNodeArray>& arspAvoidNodeArray);
    // Fills aspArray with the avoid nodes around the attacker.
    void CreateAvoidNodeArray(NiPointer<PathingAvoidNodeArray>& arspAvoidNodeArray);
    // The movement multiplier for iLocation: 2 for locations 6 and 7, 0.5 for 8, else 1.
    float GetHidingTime();
    NiPoint3 GetDestinationPos();
    bool CreatePath();

    MOVE_STATE eState;
    BGSWorldLocation Destination;
    BGSWorldLocation CurrentDestination;
    float fTargetRadius;
    TESObjectREFR* pTargetRef;
    uint32_t iLocation;
    CombatTimer HidingTimer;
    NiPoint3 LookAtPosition;
    bool bReservedSecondaryLocation;
    uint8_t padding55[3];
    NiPointer<PathingRequest> spQueuedPathingRequest;
    bool bHasQueuedPathingRequest;
    uint8_t padding5D[3];
    uint32_t iCurrentNode;
    CombatTimer CoveredMoveSearchTimer;
    CombatTimer CheckCoverTimer;
    CombatTimer CheckDestinationTimer;
};

inline bool bUseBackgroundCoveredMovePathing = true;
