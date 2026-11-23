#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/AI/Procedure.h"

#include <stdint.h>

class Actor;
class BGSLoadFormBuffer;
class MobileObject;
class SandBoxActorPackageData;
class TESObjectREFR;

// The procedure a sandboxing actor performs at its current location: IsTargetNeededForProcedure compares it with EAT
// and WANDER, and MiddleHighProcess's state machine switches on it (0 sit, 1 sleep, 2 eat, 3 wander, 4 use idle
// marker). ProceduresForActivities (indexed by the activity SLEEP 0, BREAKFAST 1, LUNCH 2, DINNER 3) holds 1, 2, 2, 2,
// and the unset value is -1 (ShouldPickNewProcedure compares eProcedure with -1).
enum SANDBOX_PROCEDURE : int32_t {
    SANDBOX_PROCEDURE_NONE = -1,
    SANDBOX_PROCEDURE_SIT = 0x0,
    SANDBOX_PROCEDURE_SLEEP = 0x1,
    SANDBOX_PROCEDURE_EAT = 0x2,
    SANDBOX_PROCEDURE_WANDER = 0x3,
    SANDBOX_PROCEDURE_USE_IDLE_MARKER = 0x4,
    SANDBOX_PROCEDURE_DIALOGUE = 0x5,
    NUM_SANDBOX_PROCEDURES = 0x6,
};

// The activity a sandboxing actor's day is in; SetUpNextActivity tries the four.
enum SANDBOX_ACTIVITY : int32_t {
    SANDBOX_ACTIVITY_NONE = -1,
    SANDBOX_ACTIVITY_SLEEP = 0,
    SANDBOX_ACTIVITY_BREAKFAST = 1,
    SANDBOX_ACTIVITY_LUNCH = 2,
    SANDBOX_ACTIVITY_DINNER = 3,
    NUM_SANDBOX_ACTIVITIES = 4,
};

// One place a sandboxing actor could go: the target reference, the procedure to do there (a SANDBOX_PROCEDURE) and
// its scores before and after the extra weighting.
class SandBoxChoiceListItem {
public:
    SandBoxChoiceListItem();
    SandBoxChoiceListItem(TESForm* apTarget, int32_t aeProcessIndex, uint32_t auiBaseScore);

    // Saves the procedure and the scores, then the target's form ID.
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    // Reads what SaveGame writes; the target's form ID goes into pTarget, resolved later.
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    TESObjectREFR* GetTarget() const;
    // Resolves the target's form ID left by LoadGame and marks the target as targeted.
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

    TESObjectREFR* pTarget;
    SANDBOX_PROCEDURE eProcedure;
    int32_t iBaseScore;
    int32_t iFullScore;
};

// The filter data ScanWorld hands EvaluateReferenceforUse through EnumReferencesCloseToPoint: the scanning actor and
// its sandbox data.
class SandBoxRefEvalFilter {
public:
    SandBoxRefEvalFilter(Actor* apActor, SandBoxActorPackageData* apPackData);

    Actor* pActor;
    SandBoxActorPackageData* pPackData;
};

class SandBoxActorPackageData : public ActorPackageData {
public:
    // The step of the sandbox state machine.
    enum SANDBOX_STATE : int32_t {
        SANDBOX_STATE_INIT = 0,
        SANDBOX_STATE_GETTING_UP = 1,
        SANDBOX_STATE_MOVING = 2,
        SANDBOX_STATE_DOINGSTUFF = 3,
        NUM_SANDBOX_STATES = 4,
    };

    // The procedure of each activity, indexed by eActivity (NUM_SANDBOX_ACTIVITIES entries) and compared with
    // eProcedure by the activity check.
    using ProceduresForActivitiesTable = SANDBOX_PROCEDURE[NUM_SANDBOX_ACTIVITIES];
    // The PROCEDURE of each SANDBOX_PROCEDURE, which GetProcedure reads by
    // eProcedure; 24 bytes of constant data.
    using kProcedures_t = const PROCEDURE[6];

    SandBoxActorPackageData();
    ~SandBoxActorPackageData() override;

    void HandleDeletedReference(TESObjectREFR* pDeletedObj) override;
    PACKAGE_TYPE GetPackageType() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

    // The hours between two game times plus or minus their minute difference.
    float GetTimeDifference(float afTimeA, float afTimeB);
    // Whether the activity's time has run out; abFinishedOut, when given, gets whether the
    // activity ended on the current day.
    bool IsActivityFinished(bool* abJustEndedTodayOut);
    // Whether the current game time is inside the activity's time window.
    bool IsActivityActive();
    // Whether the sitting or idle-marker procedure's animation has finished.
    bool IsDoneWithIdleAnims(Actor* apActor, bool bfreeIfDone);
    void ReleaseTargetObject();
    bool MoveToNextLocation();
    bool CanPerformActivity();
    void HandleSubprocessFinished();
    // Clears the base score of every choice whose target is the current target, then picks a new procedure.
    void ForgetTargetAndPickProcedure(Actor* apActor, TESPackage* apOriginalPackage);
    // Scores the choices and starts the best one.
    void PickProcedure(Actor* apActor, TESPackage* apOriginalPackage);
    void InitLocation(Actor* apActor, PackageLocation* apPackloc);
    void SetUpNextActivity();
    bool ShouldPickNewProcedure(Actor* apActor);
    PROCEDURE GetProcedure();
    void ScanWorld(Actor* apActor);
    bool UpdateIsActorAtLocation(MobileObject* apMobileObject, float afRadius);

    static void operator delete(void* apMemory);
    static bool EvaluateReferenceforUse(TESObjectREFR* apRef, void* aiFilterData);
    static bool IsTargetNeededForProcedure(SANDBOX_PROCEDURE aeProcedure);

    SANDBOX_STATE eState;
    SANDBOX_PROCEDURE eProcedure;
    float fProcedureDuration;
    TESObjectREFR* pLocation;
    float fLastTimeChanged;
    float fTotalTime;
    int32_t iNextTimeToChangeLocations;
    BSSimpleArray<SandBoxChoiceListItem, 1024> PotentialTargets;
    uint32_t iNextTimeToScanWorld;
    bool bShouldPickNewProcedure;
    bool bFood;
    bool bChair;
    bool bBed;
    uint32_t iIdleMarkers;
    TESObjectREFR* pCurrentTargetObject;
    uint8_t cCurrentTargetMarker;
    TESObjectREFR* pPrevTargetObject;
    uint32_t iPrevTargetChangedTimeStamp;
    uint32_t iActivityStartDay;
    // These two floats are in the PC order, which is the reverse of the Xbox layout.
    float fActivityDuration;
    float fActivityStartHour;
    SANDBOX_ACTIVITY eActivity;
    uint8_t cProcedureWeight[6];

    static inline ProceduresForActivitiesTable ProceduresForActivities{
        SANDBOX_PROCEDURE_SLEEP,
        SANDBOX_PROCEDURE_EAT,
        SANDBOX_PROCEDURE_EAT,
        SANDBOX_PROCEDURE_EAT
    };
    static inline kProcedures_t kProcedures{
        PROCEDURE_SLEEP,
        PROCEDURE_SLEEP,
        PROCEDURE_EAT,
        PROCEDURE_WANDER,
        PROCEDURE_USE_IDLE_MARKER,
        PROCEDURE_DIALOGUE
    };
};
