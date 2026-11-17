#pragma once

#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"

class CombatProcedureEngageTarget : public CombatProcedure {
public:
    enum ENGAGE_STATE : int32_t {
        ENGAGE_STATE_INITIALIZING = 0x0,
        ENGAGE_STATE_HOLDING = 0x1,
        ENGAGE_STATE_CROUCHING = 0x2,
        ENGAGE_STATE_MOVING_OPTIMAL_CLOSE = 0x3,
        ENGAGE_STATE_MOVING_OPTIMAL_FAR = 0x4,
        ENGAGE_STATE_APPROACHING_TARGET = 0x5,
        ENGAGE_STATE_DODGING_FORWARD_BACKWARD = 0x6,
        ENGAGE_STATE_DODGING_LEFT_RIGHT = 0x7,
        ENGAGE_STATE_MOVING_TO_TARGET = 0x8,
    };

    CombatProcedureEngageTarget();
    ~CombatProcedureEngageTarget() override;

    void Update() override;
    void Shutdown() override;
    void DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify) override;
    CombatProcedure::COMBAT_PROCEDURES GetType() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

    bool CanAttackerSneak();
    bool CreateOptimalLocationPath(bool abOutsideAbsoluteRange);

    ENGAGE_STATE eState;
    float fMinOptimalRange;
    float fMaxOptimalRange;
    float fAbsoluteMaxRange;
    CombatTimer HoldingTimer;
    CombatTimer LeftRightDodgeLOSTimer;
    bool bLeftRightDodgeHadLOS;
    // These two flags are signed bytes rather than bool.
    int8_t bLeftRightDodgeCheckLOS;
    int8_t bLeftRightDodgeLostLOS;
    int8_t cCheckedOptimalPath;
    NiPoint3 LeftRightDodgeStartLocation;
    BGSWorldLocation Destination;
    BGSWorldLocation TargetOptimalWorldLoc;
    CombatTimeStamp fOptimalLocationPathingFailedTimeStamp;
};

inline bool bUseBackgroundOptimalLocationPathing = true;
