#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout/AI/AITaskManager.h"
#include "Fallout/AI/LipSyncBackgroundManager.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class Crime;
class MagicShaderHitEffect;
class TESEffectShader;
class TESFaction;
class TESNPC;

void UpdateAnimSceneGraph(void* apData, uint32_t aiIndex);
void UpdateAnimMovement(void* apData, uint32_t aiIndex);
// Updates one temp effect of the parallel update's task array apData.
void UpdateTempEffect(void* apData, uint32_t aiIndex);
void* fillchar(void* apDest, int32_t aiValue, uint32_t aCount);

// an operation run on every mob of one process level (ProcessLists::IterateMobs)
class MobIterOperator {
public:
    MobIterOperator();

    // runs the operation on one mob
    virtual void Operate(MobileObject* apMob) = 0;

    // The process level to walk.
    PROCESS_TYPE QProcessType();

    // 0x4 the process level to walk
    PROCESS_TYPE eType;
};

// one temp effect UpdateTempEffectsParallel hands to the worker threads
struct EffectTaskData {
    // 0x0
    BSTempEffect* pTempEffect;
    // 0x4 the time to advance the effect by
    float fDeltaTime;
    // 0x8 the effect is on GlobalTempEffectList, not MagicEffectList
    bool bGlobal;
    // 0x9 the worker found the effect finished
    bool bRemove;
};

// lists of every processed object split by process level, plus the
// post-processing lists the AI update fills
class ProcessLists {
public:
    ProcessLists();
    ~ProcessLists();

    // Rebuilds the 3D the actors need after the player rested.
    void Update3DAfterResting();
    // Empty in this build.
    void PrintActorsInDialog();
    bool IsCrimeinGlobalCrimeList(
        Actor* apTarget,
        Actor* apVictim,
        Actor* apAwareCrime,
        PackageNS::CRIME_TYPE aeType,
        TESForm* apform,
        int32_t aiCrimeNumber
    );
    // Whether apActor is talking through an activator.
    bool IsActorTalkingThroughActivator(Actor* apAct);
    void AddTempEffect(BSTempEffect* apTempEffect);
    // Stops the magic shader visuals of effect shader apShader on apEffectTarget; apShader is compared with
    // MagicShaderHitEffect::pEffectData.
    void FinishMagicShaderHitEffect(TESObjectREFR* apEffectTarget, TESEffectShader* apShader);
    // The number of bytes the process lists save.
    uint32_t GetSaveSize();
    // Saves the temp effect list.
    void SaveTempEffectsList();
    // Gives every actor its leveled items.
    void InitActorLeveledItems();
    // Inserts apActor into the close-to-player list.
    void InsertActorCloseToPlayer(Actor* apActor);
    // Sorts the close-to-player list by distance.
    void SortActorsCloseToPlayer();
    // Initialises the crimes after a load and resets the high actor list.
    void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
    // Marks every geometry decal among the global temp effects finished.
    void FinishGeometryDecalEffects();
    // The crime at aiCrimeIndex in the list of aeType; null, with a warning, past the end.
    Crime* GetCrimeByIndex(PackageNS::CRIME_TYPE aeType, uint16_t aiCrimeIndex);
    // Rebuilds the high actor list.
    void CalculateAliveActorList();
    // Runs the AI package update of the high list actors.
    void UpdateHighListPackages(float afDelta, bool abSleeping);
    // Runs the detection of every high actor.
    void RunDetectionForAllActors();
    // Moves the actors of process level apLevel.
    void UpdateActorsMovement(float afDelta, bool abSleeping, PROCESS_TYPE apLevel);
    // A step of the AI threads that does nothing.
    void UpdateEmptyStep(float afDelta, bool abSleeping) {}
    BSSimpleList<TESObjectREFR*>*
    IsActorTargetingREFinPackage(TESObjectREFR* apActor, PACKAGE_TYPE apType, bool bOnlyHigh);

    // The high-process actor whose base is apNpc, or null.
    Actor* GetActorRefInHigh(TESNPC* apNpc, bool abDetection);
    // puts an object on the list of its process level
    void AddReference(
        MobileObject* apActor,
        PROCESS_TYPE aWhichList,
        bool aiWhere,
        bool abinsert,
        MobileObject* apInsertActor
    );
    // takes an object off the list of its process level
    void RemoveReference(MobileObject* apActor, PROCESS_TYPE aWhichList);
    void SetPlayerInRadiatedWater(bool abIrradiated);
    // runs the scripts of every actor on the high list
    void RunActorScripts();
    void RunActorMagicUpdates();
    void RunActorUpdates();
    void RunActorRagdollAnimationUpdates();
    void RunActorAnimationUpdates();
    // instantly finishes the fade-in of every actor on the high list
    void SkipHighListFadeIn();
    // lets the processes of the player and of the high list actors redo their
    // detection data
    void ReconcileDetectionDataForActor();
    // adds an actor to the alive actor list
    void InsertAliveActor(Actor* apActor);
    // the game clock the AI update started at
    float GetSystemTimeClock();
    // runs arIterOp on every loaded mob of the level it names
    void IterateMobs(MobIterOperator& arIterOp);
    // the number of actors in the high process level
    uint32_t CountHighActors();
    // apActor is among the first aiNumber actors close to the player (always true for none and the player)
    bool IsActorNCloseToPlayer(Actor* apActor, int32_t aiNumber);
    // drops apActor from the temporary change list
    void RemoveActorFromTempChangeList(MobileObject* apActor);
    // adds apActor to the temporary change list when it must change process level
    void AddActorToTempChangeList(MobileObject* apActor);
    // gives the actors on the temporary change list their escort and follow targets
    void UpdateFollowerTempList();
    // moves the player's teammates, and followers farther than 350 units, to the player
    void MoveFollowersOnToPlayer();
    // updates the reference muzzle flashes and frees the ones that went out
    void UpdateReferenceMuzzleFlashes(float afDeltaTime);
    // Takes apActor off the alive-actor lists.
    void RemoveAliveActor(Actor* apActor);
    // Takes pactor off the list of actors close to the player.
    void RemoveActorCloseToPlayer(Actor* pactor);
    // Removes apActor from every detection list.
    void RemoveActorInDetectionLists(Actor* apActor);
    // Clears apRef from the targets the processes hold; abDeleted when apRef is being deleted.
    void NullOutTargetRef(TESObjectREFR* apRef, bool abDeleted);
    // Plays the flare hit effect aeFlareType on apEffectTarget; true when it played.
    bool FlareHitEffect(TESObjectREFR* apEffectTarget, MagicSystem::FlareType aeFlareType);
    // tells the live high actors (not dead) that their position is invalid
    void HandleInvalidPositions();
    // the animation movement pass of the attached high actors; with the setting on, the worker threads run it
    void ParallelActorAnimationMovementUpdates();
    // puts the high actors whose package wants a bed, chair or meal into their furniture at once
    void PlaceActorsinBedsOrChairs();
    // the projectile pass after the cell update; drops the projectiles that are done from the post-process list
    void PostProcessProjectiles();
    // the impact pass of the high list's projectiles and explosions
    void UpdateHighListImpacts(float afDelta);
    // Updates the actors' animation scene graphs serially.
    void UpdateAnimationScenegraph();
    // updates the animation scene graphs of the high and middle-high actors, with the worker threads when the
    // setting allows
    void ParallelUpdateAnimationScenegraph();
    // the AI work that may not run while rendering
    void UpdateNonRenderSafeStuff(float afDelta);
    // lets every actor on the high list drop its detection event
    void RemoveDetectionEventsOnActors();
    // loads the dynamic idle of revivable dead actors and knocks down the high list actors out of fatigue
    void AnimateDeadActors();
    // stamps the game date and sets up the package locations of the low list actors
    void InitPackageLocations();
    // The per-frame AI update of every process level.
    void UpdateProcessLists();
    // moves the mobs queued on the temp change list to their new process level
    // Moves references between process levels.
    void ChangeProcessLevel(BSSimpleList<TESObjectREFR*>* apList, bool abChangeLevel);
    // Processes the supplied reference list.
    void ProcessActors(BSSimpleList<TESObjectREFR*>* apList);
    void ChangeProcessLevelTempList();
    // updates the temporary effects and frees the finished ones, serially or with the worker threads
    void UpdateTempEffects(float afDeltaTime);
    void UpdateTempEffectsParallel(float afDeltaTime);
    ProcessArray* GetProcessArray();
    // Ends the magic hit effects on apRef.
    void FinishMagicHitEffect(TESObjectREFR* apEffectTarget);
    uint32_t GetNumberOfActorsInHigh() const;
    BSSimpleList<Actor*>* GetAliveActorList();
    // Reverts the process lists' save data.
    void Revert(BGSLoadGameBuffer* apLoadGameBuffer);
    // Reverts the temporary effects.
    void RevertTempEffects(BGSLoadGameBuffer* apLoadGameBuffer);
    // Whether hostile actors are near the player.
    bool AreHostilActorsNear(bool abInterior);
    bool IsPlayerInRadiationArea();
    void ResetPlayerActionsCommentTimer();
    void SetSystemTimeClock(float afvalue);
    int32_t GetCrimeNumber();
    // Drops the temp effects of apCell.
    void RemoveTempEffectsForCell(TESObjectCELL* apCell);
    // A new heap list of the actors targeting the object aId, apActorPickup left out.
    BSSimpleList<TESObjectREFR*>* GetActorTargetinganObject(uint32_t aId, Actor* apActorPickup);
    // The vendor who owns and sells pRef, else null.
    Actor* FindOwnerSellsObject(TESObjectREFR* pRef);
    // Removes every crime apActor committed from the global crime lists.
    void RemoveAllCrimesforAnActor(Actor* apActor);
    // Takes apCrime out of the global crime lists.
    void RemoveCrimeGlobalCrimeList(class Crime* apCrime);
    // Updates the radiation the player takes from the radiating references.
    void UpdateRadiationList();
    bool IsActorDetected(Actor* apActor);
    void NullOutActorsPackage(TESPackage* apPackage);
    void StopCombatandAlarmonActor(Actor* apActor, bool abCombatOnly);
    void UpdateTheCrimeList();
    void SendGrenadeMineAlert(TESObjectREFR* apShooter, Projectile* apProjectile);
    void UpdateMiddleHighList(float afDelta, bool abSleeping);
    BSSimpleList<TESObjectREFR*>* SendActorsYellAlarm(Crime* apCrime, BSSimpleList<TESObjectREFR*>* apDoorList);
    void UpdateLowList(float afDelta, bool abSleeping);
    void UpdateMiddleLowList(float afDelta, bool abSleeping);
    void ProcessHighActorsForFastTravel();
    Actor* GetActorRefInHigh(TESFaction* apFaction, bool abDetection, bool abClosetoPlayer);
    BSSimpleList<TESObjectREFR*>* SendCrimetoHighList(Crime* apCrime);
    BSSimpleList<TESObjectREFR*>* SendResistArresttoGuards(Actor* apActor, BSSimpleList<TESObjectREFR*>* apDoorList);
    BSSimpleList<TESObjectREFR*>* GetActorsProcessCrimeAlarm(Crime* apCrime, bool bdelete);
    void TurnOffMovementinHigh();
    BSSimpleList<Crime*>* RemoveCrimesForYieldCheck(float afTime);
    void AddtoGlobalCrimeList(Crime* apCrime);
    bool IsEnemyFactionofOtherCrime(TESFaction* apFaction, Crime* apCrime);
    void BuildFollowerListRecursive(Actor* apActor, BSSimpleList<Actor*>& aReturnList);
    int32_t GetHighestDetectionLevelForActor(Actor* apActor, bool abIncludeFollowers);
    void MoveActorsToReference(TESObjectREFR* apMarker);
    // Drops the particle temp effects following apFollow3D.
    void RemoveParticleTempEffectsFor3D(NiAVObject* apFollow3D);
    MagicShaderHitEffect* FindActiveShaderHitEffect(TESObjectREFR* apEffectTarget);
    MagicShaderHitEffect* FindAndCleanupWeaponShaderHitEffect(TESObjectREFR* apEffectTarget, TESEffectShader* apShader);
    uint32_t EndAllNonPlayerConversations();
    int32_t GetActiveMagicProjectileCount();
    void NullOutActorInCrimeList(TESObjectREFR* apRef);
    int16_t GetCrimesSaveSize();
    void SaveGame_Crimes();
    void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
    void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
    void FlushNonPersistActors(int32_t aeMode);
    void ReferDestructorNullOut(TESObjectREFR* apActor);
    void DisableExcessDeadActors(float afDeltaTime);
    void UnloadAllActorPermanentMagic();
    void RebuildActorsCloseToPlayer();
    void UpdateHighListAnimation(float afDelta, bool abSleeping);
    void RemoveReferenceMuzzleFlash(TESObjectREFR* apParentRef);
    void SpawnReferenceMuzzleFlash(Projectile* apProjectile, TESObjectREFR* apParentRef);
    Crime* GetCrimeinGlobalCrimeList(PackageNS::CRIME_TYPE aeType, int32_t aiCrime);
    Crime* GetCrimeinGlobalCrimeList(Actor* apCriminal, TESObjectREFR* apVictim, PackageNS::CRIME_TYPE aeType);
    uint16_t GetCrimeIndex(PackageNS::CRIME_TYPE aeType, Crime* apCrime);
    void FinishMagicShaderHitEffectAll(TESEffectShader* apShader);
    void FinishMagicModelHitEffect(TESObjectREFR* apEffectTarget, char* apFilename);
    void CommentOnPlayerAction(PLAYER_ACTION eaction, TESObjectREFR* apObject, bool abMakecomment);

    // Updates the followers of the player's packages.
    static void UpdatePlayerFollowers();
    static int32_t DistanceListCompareFn(TESObjectREFR* apCompare1, TESObjectREFR* apCompare2);
    static void DeleteAttachedArrows(TESBoundObject* apBase, int32_t aiMax, Actor* apRef, uint8_t abNow, uint8_t abInv);

    // 0x0 seconds without processing anything
    float fSecondsPassedNoProcess;
    // 0x4 every processed object, split by process level
    ProcessArray MobProcessArray;
    // 0x44 the five crime lists
    BSSimpleList<Crime*>* GlobalCrimeListArray[5];
    // 0x58 particle temp effects
    BSSimpleList<NiPointer<BSTempEffect>> GlobalTempEffectList;
    // 0x60 magic temp effects
    BSSimpleList<NiPointer<BSTempEffect>> MagicEffectList;
    // 0x68 muzzle flashes waiting for post-processing
    BSSimpleList<void*> ReferenceMuzzleFlashList;
    // 0x70 projectiles waiting for post-processing
    BSSimpleList<void*> ProjectilePostProcessList;
    // 0x78 actors moving to another process level after the update
    BSSimpleList<void*> TempShouldMoveList;
    // 0x80 actors that are alive
    BSSimpleList<Actor*> AliveActorList;
    // 0x88 actors close to the player, most important first
    Actor* pActorsCloseToPlayerA[50];
    // 0x150 number of valid entries in pActorsCloseToPlayerA
    int32_t iActorsCloseToPlayer;
    // 0x154 time until the player comments on nearby actions again
    float fPlayerActionCommentTimer;
    // 0x158 time until the player comments on knocked objects again
    float fPlayerKnockObjectCommentTimer;
    // 0x15C the player stands in water that irradiates
    bool bPlayerInRadiatedWater;
    // 0x15D the player stands in a radiation area
    bool bPlayerInRadiationArea;
    // 0x160 the AI task queue (Main::OnIdle_UpdateProcessLists calls AITaskManager::UpdateTasks with this+0x160)
    AITaskManager TaskManagerAI;
    // 0x102E4 packages of the running dialog
    BSSimpleList<void*> DialogPackageList;
    uint8_t NearbyActorBookkeeping[0x14];
    // 0x10300 lock guarding the nearby-actor pointers
    BSSpinLock kNearbyActorLock;
    // 0x10320 lock held around the update pass and TES::RunScripts
    BSSpinLock kUpdateLock;
    // 0x10340 spin lock
    BSSpinLock kLock_10340;
    // 0x10360 lip-sync work queue (LockFreeMap)
    LipSyncBackgroundManager kLipSyncManager;
    // 0x103A0 schedules update
    bool bRunScheduals;
    // 0x103A1 detection runs
    bool bRunDetection;
    // 0x103A2 detection stats draw on screen
    bool bShowDetectionStats;
    // 0x103A4 actor whose detection stats are drawn
    Actor* pStatdetect;
    // 0x103A8 actors on the high list get a high process
    bool bProcessHigh;
    // 0x103A9 actors on the low list get a low process
    bool bProcessLow;
    // 0x103AA actors on the middle-high list get a middle-high process
    bool bProcessMHigh;
    // 0x103AB actors on the middle-low list get a middle-low process
    bool bProcessMLow;
    // 0x103AC schedules run
    bool bProcessSche;
    // 0x103AD subtitles show over actors
    bool bShowSubtitle;
    // 0x103AE the low list is being updated
    bool bUpdatingLowList;
    // 0x103B0 how many high processes ran
    uint32_t iNumberHighActors;
    // 0x103B4 time until crimes update again
    float fCrimeUpdateTimer;
    // 0x103B8 the crime the player is witnessing
    uint32_t iCrimeNumber;
    // 0x103BC time until excess dead actors are disabled
    float fRemoveExcessDeadTimer;
    // 0x103C0 semaphore that pauses the movement update
    BSSemaphore MovementSyncSema;

    // the game clock the AI update started at
    static inline float fSystemTimeClock;
    static inline LARGE_INTEGER ulTimerFrequency{};
    static inline bool bIsForciblyUpdated;
};

inline int32_t iArrayNumbStopped;

// The engine's one ProcessLists.
extern ProcessLists ReferenceProcessLists;

// The game date the low processes stamp.
inline float fGameHour;
inline uint32_t iGameYear;

// The game settings ProcessLists.cpp defines.
extern SettingT<INISettingCollection> bRunMiddleLowLevelProcess;
extern SettingT<INISettingCollection> iLowProcessingMilliseconds;
extern SettingT<INISettingCollection> bTaskletActorSceneGraphUpdates;
extern SettingT<INISettingCollection> bTaskletActorAnimMovementUpdates;
extern SettingT<INISettingCollection> bTaskletTempEffectsUpdate;
inline bool bRadiationExists;

extern SettingT<GameSettingCollection> gs_fRemoveExcessComplexDeadTime;
