#pragma once

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/ModifierList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

#include <stdint.h>

class Actor;
class BGSListForm;
class GuardActorPackageData;
class LowProcess;
class ObjectstoAcquire;
class PatrolActorPackageData;
class TESBoundObject;
class TESFaction;
class TESObjectREFR;
class TESPackage;

// The thread holding a low process's lock and how many times it took it.
struct LP_LOCK_DATA {
    LP_LOCK_DATA() = default;
    // The value NiTMap::DeleteItem clears an item with: Clear clears both fields (its argument unused).
    LP_LOCK_DATA(int32_t) : iThreadID(0), iLockCount(0) {}

    uint32_t* Clear(int32_t aiClearValue);

    uint32_t iThreadID;
    uint32_t iLockCount;
};

// Process of an actor: BaseProcess plus the package flags and the package state below. The chain is BaseProcess,
// LowProcess, MiddleLowProcess, MiddleHighProcess, HighProcess; each level adds what its process functions need.
class LowProcess : public BaseProcess {
public:
    // the flags at 0x30, one bit each, tested by GetBit and set by SetBit
    enum {
        kFlagTargetActivated = 0x01,
        kFlagCurrentActionComplete = 0x02,
        kFlagAlerted = 0x08,
        kFlagFollower = 0x10,
        kFlagPackageDoneOnce = 0x20,
        kFlagPackageIdleDone = 0x40,
        kFlagLockedLocation = 0x80,
    };

    LowProcess();
    ~LowProcess() override;

    void Update(MobileObject* apRef, float fTimeDelta) override;
    void Update(MobileObject* apRef) override;
    void UpdateMissedPackages(MobileObject* apRef, bool abActionflag) override;
    NiPoint3 GetCurrentPackageLocation(Actor* apActor) override;
    bool CheckforNewPackage(Actor* apActor, bool abFlag) override;
    void ComputeLastTimeProcessed() override;
    bool IsWandering() override;
    int32_t GetProcessedDay() override;
    int32_t GetProcessedMonth() override;
    int32_t GetProcessedYear() override;
    bool SetTargetForPackage(Actor* apActor) override;
    TESObjectREFR* SetLocationForPackage(Actor* apActor, bool abSecondLoc) override;
    TESObjectREFR* GetCurrentDestinationReference(Actor* apActor, bool abInterrupt) override;
    NiPoint3 GetCurrentDestinationCoordinate(Actor* apActor, bool abInterrupt) override;
    TESObjectCELL* GetCurrentDestinationCell(Actor* apActor) override;
    TESWorldSpace* GetCurrentDestinationWorldSpace(Actor* apActor) override;
    float GetCurrentDestinationRadius(Actor* apActor) override;
    bool CheckAndStartCombatControllerForPackage(
        Actor* apActor,
        TESPackage* apPackage,
        bool abCheckContinueInCombat
    ) override;
    bool ShouldUpdateCombatControllerOrEndCombat(Actor* apActor, TESPackage*& apRetPackage) override;
    void ProcessRemoveWorn(Actor* apActor, bool abInterateIndex) override;
    void CreateFollowforEscort(Actor* apActor, TESPackage* apEscort, bool abCheck) override;
    void ModEssentialDownTimer() override;
    float GetEssentialDownTimer() override {
        return fEssentialDownTimer;
    }
    void SetEssentialDownTimer(float afvalue) override;
    void SetCurrentActionComplete(bool abValue) override;
    bool GetCurrentActionComplete() override;
    TESObjectREFR* GetTarget() override {
        return pTarget;
    }
    void SetTarget(TESObjectREFR* apRef) override;
    int32_t GetNumberOfItemsActivated() override {
        return iNumberItemsActivate;
    }
    void SetNumberOfItemsActivated(int32_t aiNumber) override {
        iNumberItemsActivate = aiNumber;
    }
    bool GetLockedLocation() override;
    void SetLockedLocation(bool bflag) override;
    bool GetIsAggressor() override {
        return GetBit(4);
    }
    void SetIsAggressor(bool abFlag) override {
        SetBit(abFlag, 4);
    }
    void SetItemBeingUsed(TESBoundObject* apobject) override {
        pItemBeingUsed = apobject;
    }
    TESBoundObject* GetItemBeingUsed() override {
        return pItemBeingUsed;
    }
    void SetCombatDelayTimer(float afvalue) override {
        fCombatDelayTimer.Reset(afvalue);
    }
    bool CheckCombatDelayTimer() override {
        return fCombatDelayTimer.Check();
    }
    float GetPotentialThreatDamage(Actor* apActor, Actor* apTarget) override;
    bool IsPackageDone(Actor* apActor) override;
    void AddToProcedureIndexRunning(MobileObject* apActor, int32_t ainumb) override;
    void ProcessFollow(Actor* apActor, bool abflag, int32_t emovemode, bool abpursuit) override;
    void ProcessPursue(Actor* apActor, bool abNewpackage, bool abInterateIndex, int32_t emovemode) override;
    void ProcessFlee(Actor* apActor) override;
    void ProcessGreet(
        Actor* apActor,
        TESTopic* apGreetTopic,
        bool abForceSub,
        bool abStop,
        bool abQue,
        bool abSayCallBack
    ) override;
    void UpdateFollowers(Actor* apActor, float afDelta) override;
    bool GetAlert() override;
    void SetAlert(bool abFlag) override;
    bool EnterCombat(
        Actor* apActor,
        Actor* apTarget,
        bool abCheckForAmbush,
        bool absendalarm,
        CombatGroup* apGroup,
        bool abfleeing,
        bool abForceFlee,
        bool abIsAggressor,
        char acTeam,
        bool bforcecombat,
        bool bIgnoreDefensivePackage,
        bool abCastSpell,
        TESPackage* apPackage
    ) override;
    bool IsAFollower() override;
    void SetFollower(bool abFlag) override {
        SetBit(abFlag, 0x10);
    }
    int32_t GetActorValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor) override;
    float GetActorFloatValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor) override;
    void DamageModActorValue(Actor* apActor, ActorValue::Index aiWhich, float afValue) override;
    void DamageModActorValue(Actor* apActor, ActorValue::Index aiWhich, int32_t aiValue) override;
    void ClearObjectFromAcquireList(TESObjectREFR* apRef) override;
    void ClearObjectFromAcquireList(uint32_t aID) override;
    bool IsTargetActivated() override;
    void SetTargetActivated(bool abFlag) override;
    TESObjectREFR* GetGenericLocation() override {
        return pGenericLocation;
    }
    void SetGenericLocation(TESObjectREFR* apLoc) override {
        pGenericLocation = apLoc;
    }
    TESObjectREFR* GetSecondGenericLocation() override {
        return pGenericSecondLocation;
    }
    void SetSecondGenericLocation(TESObjectREFR* apLoc) override;
    void ClearCurrentDataforProcess(Actor* apActor) override;
    uint16_t GetSaveSize(uint32_t aiFlags, MobileObject* pmob) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void SaveGame(uint32_t aiFlags, MobileObject* pmob) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags, MobileObject* pmob) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, MobileObject* pmob) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void Revert(uint32_t aiFlags, MobileObject* pmob) override;
    void StartNewPackage(Actor* apActor, TESPackage* apPack) override;
    void ChangePackage(Actor* apActor, TESPackage* apPack) override;
    void PackageDone(Actor* apActor, TESPackage* apPack) override;
    bool IsPackageDoneOnce() override;
    void SetPackageDoneOnce(bool abflag) override;
    float GetDamageModifier(ActorValue::Index aiWhich) override;
    float GetDeathTime() override {
        return fDeathTime;
    }
    void SetDeathTime(float afDeathTime) override;
    void SavePackageToExtraData(MobileObject* apMob) override;
    void LoadPackageFromExtraData(MobileObject* apMob) override;
    void ModTrackedDamage(Actor* apAggressor, float afAmount) override;
    float GetTrackedDamage() override {
        return fTrackedDamage;
    }
    // run the escort package procedure
    virtual void ProcessEscort(Actor* apActor);
    // run the sleep package procedure
    virtual void ProcessSleep(Actor*) {}
    // run the eat package procedure
    virtual void ProcessEat(Actor*, TESObjectREFR*) {}
    // run the combat procedure
    virtual void ProcessCombat(Actor* apActor);
    // run the alarm procedure
    virtual void ProcessAlarm(Actor* apActor);
    // run the activate procedure
    virtual bool ProcessActivate(Actor* apActor, bool abInterateindex);
    // run the accompany package procedure
    virtual void ProcessAccompany(Actor* apActor);
    // run the acquire procedure
    virtual void ProcessAcquire(Actor* apActor);
    // run the ambush wait procedure
    virtual void ProcessAmbushWait(Actor* apActor);
    // run the use item at procedure
    virtual bool ProcessUseItemAt(Actor* apActor);
    // run the use weapon procedure
    virtual bool ProcessUseWeapon(Actor* apActor);
    virtual void ProcessChooseSpell(Actor*) {}
    // run the flee non-combat procedure
    virtual void ProcessFleeNonCombat(Actor* apActor);
    // run the avoid area procedure
    virtual void ProcessAvoidArea(Actor* apActor);
    // run the sandbox procedure
    virtual void ProcessSandBox(Actor* apActor);
    // run the movement blocked procedure
    virtual void ProcessMovementBlocked(Actor* apActor);
    // run the guard procedure
    virtual void ProcessGuard(Actor* apActor);
    virtual void ProcessGuardStartTravelToLocation(Actor* apActor, GuardActorPackageData* apData);
    // run the patrol procedure
    virtual void ProcessPatrol(Actor* apActor);
    virtual void ProcessSummonCreatureDefend(Actor* apActor);
    virtual void ProcessClearMountPosition(Actor*) {}
    // run the notify procedure
    virtual void ProcessNotify(Actor* apActor);
    // whether the actor has the package's target objects
    virtual bool CheckIfHasObject(Actor* apActor, int32_t ainumb);
    // the target ref for a package
    virtual TESObjectREFR* GetTargetForPackage(Actor* apActor, TESPackage* apPackage);
    // the location ref for a package
    virtual TESObjectREFR* GetLocationForPackage(Actor* apActor, TESPackage* apPackage, bool abSecondLoc);
    virtual void EvaluateOrderAcquireList(Actor* apActor);

    // sets every bit of uMask in the flags when bVal is set, clears them when
    // it is not
    void SetBit(bool bVal, uint8_t uMask);
    // true when every bit of uMask is set in the flags
    bool GetBit(uint8_t uMask);
    // replaces the bits of acMask in the flags with auiValue shifted left by acShift
    void SetField(uint8_t auiValue, char acMask, char acShift);

    void SetPackageIdleDone(bool abFlag);

    bool ShouldUseCombatControllerForPackage(TESPackage* apPackage, bool abCheckContinueInCombat);
    // takes over apProcess's low-level state (BaseProcess::Copy calls it)
    void LowCopy(LowProcess* apProcess);
    bool GetPackageIdleDone() {
        return GetBit(0x40);
    }
    void StartAtPatrolRefBehaviorLow(Actor* apActor, PatrolActorPackageData* apData);
    bool UpdatePackageTargetForCombat(Actor* apActor, TESPackage* apPackage);
    float GetTimeDelta(Actor* apActor);
    void Patrol_StartTravelToRefLow(Actor* apActor, PatrolActorPackageData* apData);
    char Patrol_StartLow(Actor* apActor, PatrolActorPackageData* apData);
    void UpdateTarget();
    // Starts the package apActor's schedule picks for the current time.
    bool SetCurrentPackageFromSchedule(Actor* apActor);

protected:
    bool ShouldStopCombatControllerToFinishPackage(TESPackage* apPackage, int32_t aiIndex);

public:
    // The body passes apActor to ProcessActivate(Actor*) and
    // EnterCombat(Actor*), and HighProcess::ProcessAcquire passes its Actor* apActor.
    void ProcessKill(Actor* apActor);
    void ClearData();
    void BuildMulitTargetObjectList(Actor* apActor, TESObjectREFR* pref, PACKAGE_OBJECT_TYPE eType);
    double Patrol_TravelingToNextRefLow(Actor* apActor, PatrolActorPackageData* apData, float afTimeRemaining);
    double Patrol_AtRefLow(Actor* apActor, PatrolActorPackageData* apData, float afTimeRemaining);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // door filters: lock (unlock) apRef when the actor aiFilterData owns it; both always return false
    static bool LockDoorsatLocation(TESObjectREFR* apRef, void* aiFilterData);
    static bool UnLockDoorsatLocation(TESObjectREFR* apRef, void* aiFilterData);
    // An EnumReferencesCloseToPoint filter whose data is the searching Actor (a pointer).
    static bool EvaluateReferenceforAcquire(TESObjectREFR* apRef, void* aiFilterData);

    // 0x30 the package flags; see the kFlag* enumerators
    uint8_t m_uFlags;
    // 0x34 the item the actor is using
    TESBoundObject* pItemBeingUsed;
    // 0x38 the delay before the actor enters combat
    CombatTimer fCombatDelayTimer;
    // 0x40 the reference the actor's package targets
    TESObjectREFR* pTarget;
    TESObjectREFR* pGenericLocation;
    // The package's second location; declared as a reference because every store is a reference.
    TESObjectREFR* pGenericSecondLocation;
    BGSListForm* pTargetFormList;
    int16_t sFormListNumb;
    TESObjectREFR* pPatrolLocation;
    // How many of the package's items the actor has activated; compared signed.
    int32_t iNumberItemsActivate;
    // 0x5C the objects the actor's package acquires
    BSSimpleList<ObjectstoAcquire*> ObjectList;
    // 0x64 the object the actor is acquiring
    ObjectstoAcquire* pAcquireObject;
    // 0x68 the acquire object saved while another runs
    ObjectstoAcquire* pSavedAcquireObject;
    // 0x6C the process's generic locations (a reference is removed from it when it is cleared)
    BSSimpleList<TESObjectREFR*> GenericLocationsList;
    // 0x74 the objects an acquire package is enumerating
    BSSimpleList<ObjectstoAcquire*> TempObjectList;
    // 0x7C the locations an acquire package is enumerating
    BSSimpleList<TESObjectREFR*> TempLocList;
    // 0x84 the object an acquire package is enumerating references for (cleared around the enumeration)
    TESBoundObject* pObjecttoAcquire;
    // 0x88 the object an acquire package looks for at its location
    TESBoundObject* pObjectForLocation;
    // 0x8C the type of object the package acquires (POTYPE_NONE outside an enumeration)
    PACKAGE_OBJECT_TYPE eFormType;
    TESFaction* pFactiontoAquire;
    // 0x94 the actor value modifiers this process adds to its actor
    ModifierList pDamageModifiers;
    // 0xA4 the time left before an essential actor gets up
    float fEssentialDownTimer;
    // 0xA8 when the actor died
    float fDeathTime;
    // 0xAC the damage tracked against the actor
    float fTrackedDamage;
    bool bSkippedTimeStampForPathing;

    static inline int32_t eCurrentGameMonth = -1;
    static inline int8_t cCurrentGameDay;
};

// An EnumReferencesCloseToPoint filter: aiFilterData is the searching Actor (a pointer, see
// TESObjectCELL::EnumReferencesCloseToPoint).
bool EvaluateGenericLocation(TESObjectREFR* apRef, void* aiFilterData);
