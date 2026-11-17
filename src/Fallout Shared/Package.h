#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageSchedule.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class Actor;
class BGSIdleCollection;
class BGSLoadFormBuffer;
class GuardPackageData;
class MobileObject;
class PackageTarget;
class Script;
class TESCombatStyle;
class TESObject;
class TESObjectCELL;
class TESObjectREFR;
class TESTopic;
class TESWorldSpace;

// What a package makes its owner do; the actor's schedule selects a package by this type.
enum PACKAGE_TYPE : int32_t {
    PACKAGE_TYPE_NONE = -1,
    PACKAGE_TYPE_EXPLORE = 0x0,
    PACKAGE_TYPE_FOLLOW = 0x1,
    PACKAGE_TYPE_ESCORT = 0x2,
    PACKAGE_TYPE_EAT = 0x3,
    PACKAGE_TYPE_SLEEP = 0x4,
    PACKAGE_TYPE_WANDER = 0x5,
    PACKAGE_TYPE_TRAVEL = 0x6,
    PACKAGE_TYPE_ACCOMPANY = 0x7,
    PACKAGE_TYPE_USE_ITEM_AT = 0x8,
    PACKAGE_TYPE_AMBUSH = 0x9,
    PACKAGE_TYPE_FLEE_NON_COMBAT = 0xA,
    PACKAGE_TYPE_CAST_MAGIC = 0xB,
    PACKAGE_TYPE_SANDBOX = 0xC,
    PACKAGE_TYPE_PATROL = 0xD,
    PACKAGE_TYPE_GUARD = 0xE,
    PACKAGE_TYPE_DIALOGUE = 0xF,
    PACKAGE_TYPE_USE_WEAPON = 0x10,
    PACKAGE_TYPE_FIND = 0x11,
    PACKAGE_TYPE_COMBAT = 0x12,
    PACKAGE_TYPE_COMBAT_LOW = 0x13,
    PACKAGE_TYPE_ACTIVATE = 0x14,
    PACKAGE_TYPE_ALARM = 0x15,
    PACKAGE_TYPE_FLEE = 0x16,
    PACKAGE_TYPE_TRESPASS = 0x17,
    PACKAGE_TYPE_SPECTATOR = 0x18,
    PACKAGE_TYPE_REACT_TO_DEAD = 0x19,
    PACKAGE_TYPE_GET_UP = 0x1A,
    PACKAGE_TYPE_DO_NOTHING = 0x1B,
    PACKAGE_TYPE_IN_GAME_DIALOGUE = 0x1C,
    PACKAGE_TYPE_SURFACE = 0x1D,
    PACKAGE_TYPE_SEARCH_FOR_ATTACKER = 0x1E,
    PACKAGE_TYPE_AVOID_RADIATION = 0x1F,
    PACKAGE_TYPE_REACT_TO_DESTROYED_OBJECT = 0x20,
    PACKAGE_TYPE_REACT_TO_GRENADE_OR_MINE = 0x21,
    PACKAGE_TYPE_STEAL_WARNING = 0x22,
    PACKAGE_TYPE_PICKPOCKET_WARNING = 0x23,
    PACKAGE_TYPE_MOVEMENT_BLOCKED = 0x24,
    PACKAGE_TYPE_SANDMAN = 0x25,
    PACKAGE_TYPE_CANNIBAL = 0x26,
    PACKAGE_TYPE_BACKUP = 0x27,
    PACKAGE_TYPE_COUNT = 0x28,
};

namespace PackageNS {
    // The crimes an actor can witness or report.
    enum CRIME_TYPE : int32_t {
        CRIME_TYPE_NONE = -1,
        CRIME_TYPE_STEAL = 0,
        CRIME_TYPE_PICKPOCKET = 1,
        CRIME_TYPE_TRESPASS = 2,
        CRIME_TYPE_ATTACK = 3,
        CRIME_TYPE_MURDER = 4,
        CRIME_TYPE_COUNT = 5,
    };
} // namespace PackageNS

// How a package reaches its location or its target, and what it does once it is there.
enum PACKAGE_PROCEDURE_TYPE : int32_t {
    PACKAGE_PROCEDURE_NONE = -1,
    PACKAGE_PROCEDURE_EXPLORE_TRAVEL = 0,
    PACKAGE_PROCEDURE_EXPLORE_WANDER = 1,
    PACKAGE_PROCEDURE_EXPLORE_ACTIVATE = 2,
    PACKAGE_PROCEDURE_EXPLORE_ACQUIRE = 3,
    PACKAGE_PROCEDURE_SLEEP = 4,
    PACKAGE_PROCEDURE_EAT = 5,
    PACKAGE_PROCEDURE_FOLLOW_WITH_ESCORT = 6,
    PACKAGE_PROCEDURE_AMBUSH_FOLLOW = 7,
    PACKAGE_PROCEDURE_ESCORT_ACTOR = 8,
    PACKAGE_PROCEDURE_ESCORT_OBJECT = 9,
    PACKAGE_PROCEDURE_DIALOGUE = 10,
    PACKAGE_PROCEDURE_ALARM = 11,
    PACKAGE_PROCEDURE_COMBAT = 12,
    PACKAGE_PROCEDURE_ACTIVATE = 13,
    PACKAGE_PROCEDURE_GREET = 14,
    PACKAGE_PROCEDURE_OBSERVE_COMBAT = 15,
    PACKAGE_PROCEDURE_OBSERVE_DIALOGUE = 16,
    PACKAGE_PROCEDURE_TALK_TO_DEAD = 17,
    PACKAGE_PROCEDURE_COMBAT_NOT_IN_HIGH = 18,
    PACKAGE_PROCEDURE_FLEE = 19,
    PACKAGE_PROCEDURE_TRESPASS = 20,
    PACKAGE_PROCEDURE_GET_UP = 21,
    PACKAGE_PROCEDURE_EXPLORE_NPC = 22,
    PACKAGE_PROCEDURE_MOUNT_HORSE = 23,
    PACKAGE_PROCEDURE_DISMOUNT_HORSE = 24,
    PACKAGE_PROCEDURE_DO_NOTHING = 25,
    PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC = 26,
    PACKAGE_PROCEDURE_ACCOMPANY = 27,
    PACKAGE_PROCEDURE_USEITEMAT = 28,
    PACKAGE_PROCEDURE_SANDMAN = 29,
    PACKAGE_PROCEDURE_AMBUSH = 30,
    PACKAGE_PROCEDURE_SURFACE = 31,
    PACKAGE_PROCEDURE_FLEE_NON_COMBAT = 32,
    PACKAGE_PROCEDURE_SEARCH = 33,
    PACKAGE_PROCEDURE_CLEAR_MOUNT_POSITION = 34,
    PACKAGE_PROCEDURE_SUMMON_CREATURE_DEFEND = 35,
    PACKAGE_PROCEDURE_AVOID_RADIATION = 36,
    PACKAGE_PROCEDURE_SANDBOX = 37,
    PACKAGE_PROCEDURE_PATROL = 38,
    PACKAGE_PROCEDURE_EXPLOSION_REACTION = 39,
    PACKAGE_PROCEDURE_GRENADE_MINE_REACTION = 40,
    PACKAGE_PROCEDURE_GUARD = 41,
    PACKAGE_PROCEDURE_STEAL_WARN = 42,
    PACKAGE_PROCEDURE_PICKPOCKET_WARN = 43,
    PACKAGE_PROCEDURE_USEWEAPON = 44,
    PACKAGE_PROCEDURE_FOLLOW_WITHOUT_ESCORT = 45,
    PACKAGE_PROCEDURE_MOVEMENT_BLOCKED = 46,
    PACKAGE_PROCEDURE_CANNIBAL = 47,
    PACKAGE_PROCEDURE_BACK_UP = 48,
    PACKAGE_PROCEDURE_COUNT = 49,
};

// The package flag word, as a save game stores it. The shipped debug info has no name for these bits.
enum PACKAGE_FLAGS : uint32_t {
    kPackageFlag_MustReachLocation = 0x2,
    kPackageFlag_MustComplete = 0x4,
    kPackageFlag_LockDoorsAtStart = 0x8,
    kPackageFlag_LockDoorsAtEnd = 0x10,
    kPackageFlag_LockDoorsAtLocation = 0x20,
    kPackageFlag_UnlockDoorsAtStart = 0x40,
    kPackageFlag_UnlockDoorsAtEnd = 0x80,
    kPackageFlag_UnlockDoorsAtLocation = 0x100,
    kPackageFlag_PCContinue = 0x200,
    kPackageFlag_OncePerDay = 0x400,
    kPackageFlag_IsCreated = 0x800,
    kPackageFlag_SelectiveFalloutBehavior = 0x1000,
    kPackageFlag_ScriptPackage = 0x4000,
    kPackageFlag_NeverToRun = 0x8000,
    kPackageFlag_AlwaysSneak = 0x20000,
    kPackageFlag_NoHeadTrack = 0x100000,
    kPackageFlag_ForceUnequipWeapon = 0x200000,
    kPackageFlag_DrawWeapon = 0x800000,
    kPackageFlag_PretendInCombat = 0x2000000,
    kPackageFlag_ContinueDuringCombat = 0x4000000,
};

// The second flag word of a package. A set bit forbids one sandbox activity, so the setters take the "allowed"
// argument and clear the bit for it. Package types other than sandbox put their own flags in the same word.
enum PACKAGE_SPECIFIC_FLAGS : uint16_t {
    kPackSpecificFlag_NoEating = 0x1,
    kPackSpecificFlag_NoSleeping = 0x2,
    kPackSpecificFlag_NoConversation = 0x4,
    kPackSpecificFlag_NoIdleMarkers = 0x8,
    kPackSpecificFlag_NoFurniture = 0x10,
    kPackSpecificFlag_NoWandering = 0x20,
    kPackSpecificFlag_LocationIsLinkedRef = 0x40,
};

// The Fallout behaviour flag word of a package (PACKAGE_DATA::iFOBehaviorFlags): the ambient behaviours the actor may
// still show while the package runs.
enum PACKAGE_FO_BEHAVIOR_FLAGS : uint16_t {
    kFOBehaviorFlag_EnableRandomConversations = 0x2,
};

// The type and the two flag words of a package, held in one value so that a save game can write them as a block.
struct PACKAGE_DATA {
    // Byte-swaps the flag word and the two flag halfwords.
    void Endian();

    uint32_t iPackFlags;
    int8_t cPackType;
    uint8_t padding05;
    uint16_t iFOBehaviorFlags;
    uint16_t iPackageSpecificFlags;
};

// What GetEscortTargetForPackage hands the reference enumeration: the object (or type) to look for, the actor asking,
// and whether actors already following someone are skipped.
struct TESEvaluateEscortContext {
    TESObject* pObject;
    Actor* pRequester;
    bool bSkipExistingFollowers;
};

// An instruction an actor can be given: where to go, what to do on arrival, and what to do when it is finished.
class TESPackage : public TESForm {
public:
    // A package of form type 73, its data cleared.
    TESPackage();
    ~TESPackage() override;

    // Clears the package: no type, no procedure, no location, no target and no combat style. Called by the
    // constructor and whenever a loaded package has to be reset.
    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void Revert(uint32_t aiFlags) override;
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitItem() override;
    uint32_t GetRefCount() override {
        return uiRefCount;
    }
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    const char* GetObjectTypeName() override;

    virtual bool IsReadyToRun(MobileObject* apActor);
    virtual bool
    IsActorAtLocation(MobileObject* apMobileObject, bool bignoredistance, float afExtraRadius, bool bInfurniture);
    virtual bool IsActorAtSecondLocation(
        MobileObject* apMobileObject,
        MobileObject* apPackageowner,
        bool bignoredistance,
        float afOtherRadius,
        bool bInfurniture
    );
    virtual bool IsActorAtRefTarget(MobileObject* apMobileObject, int32_t aiExtraRadius);
    virtual bool IsTargetAtLocation(MobileObject* apActor, int32_t aiExtraRadius);
    // The four save game members below are new overloads of TESForm's names. The overrides of TESForm's overloads come
    // after them.
    virtual uint16_t GetSaveSize();
    virtual void SaveGame();
    virtual void LoadGame();
    virtual void InitLoadGame();
    virtual bool IsPackageOwner(MobileObject*) {
        return true;
    }

    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
    // Initializes the package's location, target and data after a BGS load.
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

    using TESForm::GetSaveSize;
    using TESForm::InitLoadGame;
    using TESForm::Save;
    using TESForm::SaveGame;

    // The radius of the second location.
    uint32_t GetSecondLocationRadius();
    // Counts one more holder of the package. The package is only ever freed by the game's form list.
    uint32_t IncRefCount();

    // The "created" flag is only maintained for forms that the game itself created at run time.
    void SetIsCreated(bool abVal);
    bool GetIsCreated();

    // Whether the package has been switched off for this actor; it is then skipped when a package is chosen.
    bool IsNeverToRun();

    bool GetMustComplete();
    bool GetMustReachLocation();

    uint32_t GetPackageFlags();
    // Whether the actor reserves a furniture marker of its target.
    bool GetShouldReserveTarget();

    bool GetForceUnequipWeapon();
    bool GetDrawWeaponFlag();
    void SetDrawWeaponFlag(bool abVal);

    bool GetUnlockDoorsAtStart();
    // Tests kPackageFlag_LockDoorsAtStart.
    bool GetLockDoorsAtStart();
    bool GetLockDoorsAtEnd();
    bool GetUnlockDoorsAtEnd();
    bool GetLockDoorsAtLocation();
    bool GetUnlockDoorsAtLocation();

    bool GetOncePerDay();

    // Whether the package goes on while the player is near.
    bool GetPCContinue();
    // Whether the actor keeps only the Fallout behaviours the package's behaviour flags allow.
    bool GetSelectiveFalloutBehavior();
    bool GetAlwaysSneak();
    bool GetNoHeadTrack();
    bool GetFOEnableRandomConversations();
    // Whether a script gave the package to the actor, rather than his AI package list.
    bool IsScriptPackage();

    void SandboxSetLocationIsLinkedRef(bool abVal);
    void SandboxSetAllowEating(bool abVal);
    void SandboxSetAllowSleeping(bool abVal);
    void SandboxSetAllowConversation(bool abVal);
    void SandboxSetAllowIdleMarkers(bool abVal);
    void SandboxSetAllowFurniture(bool abVal);
    void SandboxSetAllowWandering(bool abVal);
    bool SandboxGetLocationIsLinkedRef();

    // A guard package reuses the sandbox sleeping bit to mark its target as a linked reference.
    bool GuardGetTargetIsLinkedRef();

    void SetSelectiveFalloutBehavior(bool abVal);
    // Clears the no-combat-alert flag when abVal is set, sets it otherwise.
    void SetCombatAlert(bool abVal);
    // Clears the no-guard-warn flag when abVal is set, sets it otherwise.
    void SetGuardWarnBehavior(bool abVal);
    // Sets or clears the Fallout behaviour flag that enables hellos to the player.
    void SetFOEnableHellosToPlayer(bool abVal);
    // Sets or clears the Fallout behaviour flag that enables the aggro radius behaviour.
    void SetFOEnableAggroRadiusBehavior(bool abVal);
    // The package data when the package is an escort package, else null.
    const TESEscortPackageData* GetEscortPackageData();
    // The package data when the package is a patrol package, else null.
    const TESPatrolPackageData* GetPatrolPackageData();

    PACKAGE_TYPE GetPackType();
    // The use-weapon data of a use-weapon package, else null.
    const TESUseWeaponPackageData* GetUseWeaponPackageData();
    PACKAGE_PROCEDURE_TYPE GetProcedureType();
    // Works out the procedure the package runs, given the actor's linked reference.
    void CalculateProcedureType(TESObjectREFR* apLinkedRef);

    // Changes the package's type: frees the old type's data, clears the location, second location and target the new
    // type does not use, and creates the ones and the type data it needs. A find package becomes an explore package.
    void SetPackType(PACKAGE_TYPE aeType);

    // Sets the procedure the engine runs when this package becomes the actor's current one.
    void SetProcedureType(PACKAGE_PROCEDURE_TYPE aetype);

    // Whether the package interrupts what the actor is doing, rather than being picked when he is idle.
    bool IsInterruptPackage();

    // Whether the actor's current AI process reports its current action as finished.
    bool HasActorCompletedPackage(Actor* apActor);

    PackageSchedule* GetPackageSchedule();
    PackageLocation* GetLocation();
    PackageTarget* GetPackageTarget();
    // The number of targets the package starts with.
    int32_t GetInitialTargetCount();
    BGSIdleCollection* GetPackageIdles();
    TESCombatStyle* GetCombatStyle();
    // Sets the combat style.
    void SetCombatStyle(TESCombatStyle* apStyle);

    // The reference the package's target resolves to for this actor: the target reference itself, or the actor's
    // linked reference when the target is a linked reference. Null when there is no target at all.
    TESObjectREFR* ComputeInitialTarget(MobileObject* apMob);

    // The script to run and the topic to say when the package begins.
    Script* GetOnBeginScript();
    TESTopic* GetOnBeginTopicInfo();
    // The idle to play when the package begins (OnBegin.pIdle).
    TESIdleForm* GetOnBeginIdle();
    // The scripts to run when the package's conditions change and when it ends.
    Script* GetOnChangeScript();
    Script* GetOnEndScript();

    // Copies a schedule into the package; a null schedule resets it to the default one.
    void SetPackageSchedule(PackageSchedule* apPackSched);

    // Copy the argument into the package's own location, second location (kept in the type data) or target, creating
    // it when needed. A null argument frees it.
    void SetPackageLocation(PackageLocation* apPackLoc);
    void SetPackageSecondLocation(PackageLocation* apPackLoc);
    void SetPackageTarget(PackageTarget* apPackTarg);

    // The second location, which only some package types keep in their type data. Null for the others.
    PackageLocation* GetPackageSecondLocation();
    // The search location for the package type.
    PackageLocation* GetPackageSearchLocation();

    // The use weapon data, created when the package has no type data.
    TESUseWeaponPackageData* GetOrCreateUseWeaponPackageData();

    // Where the package's location is for this actor: its world space, cell, coordinates, reference and radius. A
    // package with no location, or one at the actor's current location, uses where the actor started the package.
    TESWorldSpace* GetLocationWorld(MobileObject* apActor);
    TESObjectCELL* GetLocationCell(MobileObject* apActor);
    NiPoint3 GetLocationCoord(MobileObject* apActor);
    TESObjectREFR* GetLocationReference(MobileObject* apActor);
    uint32_t GetLocationRadius(MobileObject* apActor);
    // The radius the actor has to be within of the package's reference target (the second location's with abSecondLoc).
    double GetPackageRadiusActorToRefTarget(MobileObject* apMobileObject, bool abSecondLoc);
    // Marks the package never to run again for the target pref.
    void SetNeverRun(TESObjectREFR* pref, bool abVal);
    // The cell of the package's second location for this actor.
    TESObjectCELL* GetSecondLocationCell(MobileObject* apActor);
    // The cell of the package's search location: the location's cell when the search location is the location,
    // else the second location's.
    TESObjectCELL* GetSearchLocationCell(MobileObject* apActor);
    // The radius within which the actor acquires the target: the location radius for a sandbox package, else the
    // target's acquire radius when it is positive. Returns whether a radius was found.
    bool GetAcquireRadius(MobileObject* apActor, float& afResult);

    // How close the actor has to come to the location (or the second location): the location's radius, else a radius
    // from what the location's reference is, else the iAIDistanceRadiusMinLocation setting. The radius is an int
    // returned as a double so that every int value is exact; callers store it as a float.
    double GetPackageRadiusActorToLocation(MobileObject* apMobileObject, bool abSecondLoc);

    bool IsTargetAnActor(MobileObject* apActor);

    // The reference an escort, follow or accompany package's target resolves to for this actor; a follow or accompany
    // target becomes the actor's leader, an escort target his follower.
    TESObjectREFR* GetEscortTargetForPackage(Actor* apActor, bool abSkipExistingFollowers);

    // Whether today (or yesterday) is one of the schedule's days of the week.
    bool IsitDayofTheWeek(bool bTestPreviousDay);

    TESCondition* GetConditions();
    bool GetAllowsSwim();
    void SetAllowsSwim(bool abVal);
    bool GetAllowFalls();
    void SetAllowFalls(bool abVal);
    void SetForceUnequipWeapon(bool abVal);
    void SetNoHeadTrack(bool abVal);
    bool GetContinueDuringCombat();
    void SetContinueDuringCombat(bool abVal);
    bool GetCombatAlert();
    bool GetGuardWarnBehavior();
    bool GetNoIdlesFlag();
    bool GetFOEnableHellosToPlayer();
    void SetFOEnableRandomConversations(bool abVal);
    bool GetFOEnableObserveCombatBehavior();
    void SetFOEnableObserveCombatBehavior(bool abVal);
    bool GetFOEnableGreetCorpseBehavior();
    void SetFOEnableGreetCorpseBehavior(bool abVal);
    bool GetFOEnableReactionToPlayerActions();
    void SetFOEnableReactionToPlayerActions(bool abVal);
    bool GetFOEnableFriendlyFireComments();
    void SetFOEnableFriendlyFireComments(bool abVal);
    bool GetFOEnableAggroRadiusBehavior();
    bool GetFOEnableIdleChatterBehavior();
    void SetFOEnableIdleChatterBehavior(bool abVal);
    bool GetFOEnableAvoidRadiationBehavior();
    void SetFOEnableAvoidRadiationBehavior(bool abVal);
    void SetScriptPackage(bool abVal);
    // Sets the package's waiting flag.
    void SetWaiting(bool abVal);
    bool GetDefensive();
    void SetDefensive(bool abVal);
    bool GetAlwaysRun();
    void SetAlwaysRun(bool abVal);
    void SetAlwaysSneak(bool abVal);
    void SetMustReachLocation(bool abVal);
    void SetMustComplete(bool abVal);
    bool GetServicesEnabled();
    void SetPretendInCombat(bool abVal);
    bool GetWaiting();
    uint32_t DecRefCount();
    bool SearchGetAllowBuying();
    bool SearchGetAllowKilling();
    bool SearchGetAllowStealing();
    bool AmbushGetHideWhileAmbushing();
    bool SandboxGetAllowConversation();
    bool GetPretendInCombat();
    bool SandboxGetAllowEating();
    bool SandboxGetAllowSleeping();
    bool SandboxGetAllowFurniture();
    bool SandboxGetAllowIdleMarkers();
    bool SandboxGetAllowWandering();
    TESIdleForm* GetOnChangeIdle();
    TESIdleForm* GetOnEndIdle();
    bool GetDialogueSayToFlag();
    TESTopic* GetDialogueTopic();
    bool GetDialogueDoNotControlTarget();
    bool GetDialogueNoHeadtrack();
    float GetDialogueFOV();
    uint32_t GetSearchLocationRadius(MobileObject* apActor);
    void CopyAllFlags(TESPackage* apCopy);
    void SetDialogueTopic(TESTopic* apTopic);
    void SetDialogueDoNotControlTarget(bool abControl);
    void SetPackageConditions(TESCondition* apPackCond);
    TESTopic* GetOnChangeTopicInfo();
    TESWorldSpace* GetSecondLocationWorld(MobileObject* apActor);
    NiPoint3 GetSecondLocationCoord(MobileObject* apActor);
    bool IsTargetAtSecondLocation(MobileObject* apActor, float aiRadius);
    int32_t GetEscortFollowDistance();
    void SetDialogueNoHeadTrack(bool abHead);
    void SetDialogueFOV(float afFov);
    void SetPackageIdles(BGSIdleCollection* apIdles);
    TESObjectREFR* GetSecondLocationReference(MobileObject* apActor);
    const NiPoint3* GetSearchLocationCoord(NiPoint3* apResult, MobileObject* apActor);
    const TESFollowPackageData* GetFollowPackageData();
    int32_t ComparePackageSchedule(PackageSchedule* apSchedule);
    TESTopic* GetOnEndTopic();
    const GuardPackageData* GetGuardPackageData();

    // A new package of the class aPackageType needs, with its type set. No package for
    // PACKAGE_TYPE_NONE, PACKAGE_TYPE_COUNT, ambush or find.
    static TESPackage* CreatePackage(PACKAGE_TYPE aPackageType);
    static PACKAGE_OBJECT_TYPE GetObjectTypeFromForm(TESForm* apForm);
    static bool FormMatchesPackageObjectType(TESForm* apForm, PACKAGE_OBJECT_TYPE aeType);
    // aiEnumData is the TESEvaluateEscortContext GetEscortTargetForPackage passes.
    static bool EvaluateEscortReference(TESObjectREFR* apRef, void* aiEnumData);

    PACKAGE_PROCEDURE_TYPE ePROCEDURE_TYPE;
    PACKAGE_DATA PackData;
    TESPackageData* pPackData;
    PackageLocation* pPackLoc;
    PackageTarget* pPackTarg;
    BGSIdleCollection* pIdleCollection;
    PackageSchedule packSched;
    TESCondition packConditions;
    TESCombatStyle* pCombatStyle;
    PackageEventAction OnBegin;
    PackageEventAction OnEnd;
    PackageEventAction OnChange;
    uint32_t uiRefCount;
};

// The name of each package type.
using PackageTypeNameArray = const char* [PACKAGE_TYPE_COUNT];
inline PackageTypeNameArray packageTypeToString{
    "Find",
    "Follow",
    "Escort",
    "Eat",
    "Sleep",
    "Wander",
    "Travel",
    "Accompany",
    "Use Item At",
    "Ambush",
    "Flee not Combat",
    "Cast Magic",
    "SandBox",
    "Patrol",
    "Guard",
    "Dialogue",
    "Use Weapon",
    "Find",
    "Combat",
    "CombatLow",
    "Activate",
    "Alarm",
    "Flee",
    "Trespass",
    "Spectator",
    "ReactToDead",
    "GetUp from Chair/Bed",
    "Do Nothing",
    "In Game Dialogue",
    "Surface",
    "Search for Attacker",
    "Avoid Radiation",
    "React to Destroyed Object",
    "React to Grenade or Mine",
    " Steal Warning ",
    " PickPocket Warning ",
    "Movement Blocked",
    "SandMan",
    "Cannibal",
    "Backup"
};

// The name of each kind of object a package's target can be; no PC code reads it.
using PackageObjectTypeNameArray = const char* [29];
inline PackageObjectTypeNameArray PackageObjectTypeNames{
    " NONE",
    "Activators",
    "Armor",
    "Books",
    "Clothing",
    "Containers",
    "Doors",
    "Ingredients",
    "Lights",
    "Miscellaneous",
    "Flora",
    "Furniture",
    "Weapons: Any",
    "Ammo",
    "NPCs",
    "Creatures",
    "Keys",
    "Alchemy",
    "Food",
    " All: Combat Wearable",
    " All: Wearable",
    "Weapons: NONE",
    "Weapons: Melee",
    "Weapons: Ranged",
    "Actor Effects: Any",
    "Actor Effects: Range Target",
    "Actor Effects: Range Touch",
    "Actor Effects: Range Self",
    "Actors: Any"
};

// The game settings Package.cpp defines.
extern SettingT<GameSettingCollection> fTargetSearchRadius;
