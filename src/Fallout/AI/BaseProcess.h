#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout/AI/ActorPackage.h"
#include "Fallout/AI/CachedValues.h"
#include "Fallout/AI/ProcessArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class ActiveEffect;
class Actor;
class ActorPackageData;
class ArrowProjectile;
class BGSBodyPartData;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class bhkCharacterController;
class BipedAnim;
class BSBound;
class BSFaceGenNiNode;
class BSShaderPPLightingProperty;
class CachedValuesOwner;
class CombatGroup;
class Crime;
class DetectionEvent;
class DetectionState;
class DialogueItem;
class ExtraDataList;
struct FurnitureMark;
class HitData;
class ItemChange;
class LipSynchAnim;
class MagicCaster;
class MagicItem;
class MagicTarget;
class MobileObject;
class MuzzleFlash;
class NiControllerManager;
class ObjectstoAcquire;
class PathingAvoidNodeArray;
class Projectile;
class SpellItem;
class TESActorBase;
class TESBoundObject;
class TESEffectShader;
class TESIdleForm;
class TESObjectCELL;
class TESObjectREFR;
class TESObjectSTAT;
class TESObjectWEAP;
class TESPackage;
class TESTopic;
class TESWorldSpace;
class WadingWaterData;

enum ANIM_GROUP_ENUM : int32_t;
enum ANIMATION_ACTION : int32_t;
enum DETECTION_LEVEL : int32_t;
enum PACKAGE_OBJECT_TYPE : int32_t;
enum SIT_SLEEP_STATE : uint32_t;

// The knock state GetKnockState returns.
enum KNOCK_STATE_ENUM : int32_t {
    KNOCK_STATE_NORMAL = 0,
    KNOCK_STATE_EXPLODE = 1,
    KNOCK_STATE_EXPLODE_LEAD_IN = 2,
    KNOCK_STATE_OUT = 3,
    KNOCK_STATE_OUT_LEAD_IN = 4,
    KNOCK_STATE_QUEUED = 5,
    KNOCK_STATE_GET_UP = 6,
};

// Which of an actor's sound handles (HighProcess::SoundHandle's index).
enum ACTOR_SOUND_HANDLE : int32_t {
    ACTOR_SOUND_HANDLE_VOICE = 0,
    ACTOR_SOUND_HANDLE_TORCH = 1,
    ACTOR_SOUND_HANDLE_COUNT = 2,
};

// The model parts Update3dModel rebuilds (MiddleHighProcess::cUpdate3DModel's bits).
enum RESET_3D_FLAGS : int32_t {
    RESET_MODEL = 0x1,
    RESET_SKIN = 0x2,
    RESET_HEAD = 0x4,
    RESET_FACE = 0x8,
    RESET_SCALE = 0x10,
};

// The aim sequence slots (MiddleHighProcess::pAimSequence's index).
enum AIM_ENUM : int32_t {
    AIM = 0,
    AIM_UP = 1,
    AIM_DOWN = 2,
    AIM_COUNT = 3,
};

// process base of every actor: the package currently running, when the
// actor was last processed and the cached values of the actor
class BaseProcess {
public:
    // The values of HighProcess::eFadeState.
    enum FADE_STATE : int32_t {
        FADE_NORMAL = 0x0,
        FADE_IN = 0x1,
        FADE_OUT = 0x2,
        FADE_TELEPORT_IN = 0x3,
        FADE_TELEPORT_OUT = 0x4,
        FADE_OUT_DISABLE = 0x5,
        FADE_OUT_DELETE = 0x6,
        FADE_MOVE_TO_MARKER = 0x7,
        FADE_STATE_COUNT = 0x8,
    };

    enum POSTANIM_ACTION : uint32_t {
        POSTANIM_ACTION_NONE = 0x0,
        POSTANIM_ACTION_FIRE_WEAPON = 0x1,
        POSTANIM_ACTION_RELOAD_WEAPON = 0x2,
        POSTANIM_ACTION_SIT_SLEEP = 0x4,
        POSTANIM_ACTION_GET_UP = 0x8,
        POSTANIM_ACTION_PLAY_IDLE = 0x10,
        POSTANIM_ACTION_DIE = 0x20,
        POSTANIM_ACTION_EQUIP_UNEQUIP_WEAPON = 0x40,
        POSTANIM_ACTION_PICK_IDLE_CALL = 0x80,
        POSTANIM_ACTION_HAND_GRIP_WEAPON = 0x100,
        POSTANIM_ACTION_GETOUT_OF_FURNITURE = 0x200,
        POSTANIM_FORCEIDLE_ANIMATION = 0x400,
        POSTANIM_FREEUP_SPECIAL_IDLE = 0x800,
        POSTANIM_RELOAD_TARGETS = 0x1000,
        POSTANIM_ACTION_IDLE_LOADED = 0x2000,
        POSTANIM_ACTION_HAVOKWEAPON = 0x4000,
        POSTANIM_ACTION_BLOCK = 0x8000,
        POSTANIM_ACTION_SET_IRON_SIGHTS = 0x10000,
        POSTANIM_ACTION_EXPEL_SHELLCASING = 0x20000,
        POSTANIM_ACTION_RELOAD_SINGLE = 0x40000,
        POSTANIM_ACTION_START_LOOPING_RELOAD = 0x80000,
    };

    // The slots of HighProcess's head tracking targets, lowest priority first.
    enum HEAD_TRACK_TYPE : int32_t {
        HEAD_TRACK_TYPE_DEFAULT = 0,
        HEAD_TRACK_TYPE_ACTION = 1,
        HEAD_TRACK_TYPE_SCRIPT = 2,
        HEAD_TRACK_TYPE_COMBAT = 3,
        HEAD_TRACK_TYPE_DIALOG = 4,
        HEAD_TRACK_TYPE_USE_WEAPON = 5,
        HEAD_TRACK_TYPE_COUNT = 6,
    };

    BaseProcess();
    virtual ~BaseProcess();

    // copies the state of another process into this one
    virtual void Copy(BaseProcess* apProcess);
    // drops apRef from the package targets and the package data
    virtual void HandleDeletedReference(TESObjectREFR* apRef);
    virtual void Update(MobileObject*) {}
    virtual void Update(MobileObject*, float) {}
    virtual void UpdateMissedPackages(MobileObject*, bool) {}
    // the location coordinate of the running package, zero while none runs
    virtual NiPoint3 GetCurrentPackageLocation(Actor* apActor);
    // where apActor was last detected, zero when it is in neither detection list.
    virtual NiPoint3 GetLastPositionDetected(Actor*, int32_t) {
        return NiPoint3::ZERO;
    }
    // when apActor was last detected, -FLT_MAX when it is in neither detection list
    virtual float GetLastTimeDetected(Actor* apActor, int32_t abWhichList);
    virtual bool CheckforNewPackage(Actor*, bool) {
        return false;
    }
    // stores the current game hour and the packed game date as the time last processed
    virtual void ComputeLastTimeProcessed() {}
    virtual bool IsWandering() {
        return false;
    }
    // day of the month the actor was last processed
    virtual int32_t GetProcessedDay() {
        return 0;
    }
    // month of the year the actor was last processed
    virtual int32_t GetProcessedMonth() {
        return 0;
    }
    // game year the actor was last processed
    virtual int32_t GetProcessedYear() {
        return 0;
    }
    // the hour of the day the package was evaluated at
    virtual void SetPackageEvaluateHour(int32_t) {}
    // the hour of the day the package was evaluated at
    virtual int32_t GetPackageEvaluateHour() {
        return 0;
    }
    // picks the special idle to play and queues it
    virtual bool SetupSpecialIdle(
        MobileObject* apMobile,
        TESIdleForm* apIdle,
        AnimIdle::PLAY_TYPE_ENUM aeType,
        bool abForce3d,
        bool abChooseNewOne,
        bool abTestConditions
    );
    // copies the position of the target as the last place it was seen
    virtual void SetLastSeenLocation() {}
    // the spell a package makes the actor cast
    virtual MagicItem* GetCurrentPackageSpell() {
        return nullptr;
    }
    virtual void SetCurrentPackageSpell(MagicItem*) {}
    virtual void PerformOneRoundofCombat(Actor*, int32_t) {}
    // drops the cached face animation data
    virtual void ClearFaceAnimationData() {}
    virtual void UpdateHealthBar(Actor*) {}
    // the idle of the package was played once
    virtual bool GetIdleDoneOnce() {
        return false;
    }
    virtual void SetIdleDoneOnce(bool) {}
    virtual bool EvaluateDetection(Actor*) {
        return false;
    }
    virtual void EvaluateCombatDetectionLists(Actor*) {}
    virtual bool ShouldRunPlayerDetection(Actor*) {
        return false;
    }
    virtual bool ProcessActivateOneHour(Actor*, float) {
        return false;
    }
    // the actor does a say-to
    virtual bool IsDoingSayTo() {
        return false;
    }
    virtual void ProcessSandman(Actor*) {}
    virtual void ProcessCannibal(Actor*) {}
    virtual void ClearFurniture(Actor*) {}
    virtual void SetDoingSayTo(bool) {}
    // resolves the target of the running package into pTarget, true when there is one
    virtual bool SetTargetForPackage(Actor*) {
        return false;
    }
    // resolves the (second) location reference of the running package and stores it
    virtual TESObjectREFR* SetLocationForPackage(Actor*, bool) {
        return nullptr;
    }
    virtual TESObjectREFR* GetCurrentDestinationReference(Actor*, bool) {
        return nullptr;
    }
    virtual NiPoint3 GetCurrentDestinationCoordinate(Actor*, bool) {
        return NiPoint3::ZERO;
    }
    virtual TESObjectCELL* GetCurrentDestinationCell(Actor*) {
        return nullptr;
    }
    virtual TESWorldSpace* GetCurrentDestinationWorldSpace(Actor*) {
        return nullptr;
    }
    virtual float GetCurrentDestinationRadius(Actor*) {
        return 0.0f;
    }
    // the actor of the dialogue
    virtual MobileObject* GetDialogTarget() {
        return nullptr;
    }
    virtual void ClearDialogTarget() {}
    virtual bool CheckAndStartCombatControllerForPackage(Actor*, TESPackage*, bool) {
        return false;
    }
    virtual bool ShouldUpdateCombatControllerOrEndCombat(Actor*, TESPackage*&) {
        return false;
    }
    // time until conversations between actors are checked
    virtual float GetSocialTalkTimer() {
        return 0.0f;
    }
    virtual void SetSocialTalkTimer(float) {}
    // removes apActor from the actors spoken to recently
    virtual void RemoveSpokenToActor(Actor*) {}
    virtual void ProcessRemoveWorn(Actor*, bool) {}
    virtual void CreateFollowforEscort(Actor*, TESPackage*, bool) {}
    // the weapon was drawn on alert
    virtual bool GetDetectAlert() {
        return false;
    }
    virtual void SetDetectAlert(bool) {}
    virtual void RandomlyPlaySpecialIdles(Actor*) {}
    // frees the special idle of apActor's animation and resets its face animation
    virtual void PostAnimFreeUpSpecialIdle(Actor*) {}
    virtual void ResetSearchTimer() {}
    // counts the essential down timer down by the frame time
    virtual void ModEssentialDownTimer() {}
    virtual float GetEssentialDownTimer() {
        return 0.0f;
    }
    virtual void SetEssentialDownTimer(float) {}
    virtual void ResetSearchChatterTimer() {}
    // adds a new detection state for apActor to one of the two queued detection lists
    virtual DetectionState* InsertIntoDetectionList(
        Actor* apactor,
        DETECTION_LEVEL aedetect,
        bool abLineSight,
        int32_t aiLevel,
        int32_t abWhichList,
        bool ab360view,
        bool abcombat
    );
    // a new list with copies of the detection states of the actors that detect this one
    virtual BSSimpleList<DetectionState*>* GetWhoDetectsMe(Actor*) {
        return nullptr;
    }
    // the detection event the actor made
    virtual DetectionEvent* GetActorsDetectionEvent(Actor*) {
        return nullptr;
    }
    // replaces the detection event the actor made
    virtual void SetActorsDetectionEvent(
        Actor* apActor,
        NiPoint3 aLocation,
        int32_t aiSoundLevel,
        int32_t aiDetectionEventType,
        TESObjectREFR* apRef
    );
    // frees the detection event the actor made
    virtual void RemoveDetectionEvent() {}
    // frees the detection event the actor made once it is older than the expiry setting
    virtual void CheckForExpiredDetectionEvent() {}
    virtual float GetWeaponScore() {
        return 0.0f;
    }
    virtual void SetWeaponScore(float) {}
    virtual bool GetPickpocketed() {
        return false;
    }
    virtual void SetPickpocketed(bool) {}
    virtual void SetCurrentActionComplete(bool) {}
    virtual bool GetCurrentActionComplete() {
        return true;
    }
    virtual void SetCurrentMovementComplete(bool) {}
    virtual bool GetCurrentMovementComplete() {
        return true;
    }
    // the reference the package targets
    virtual TESObjectREFR* GetTarget() {
        return nullptr;
    }
    // sets the reference the package targets and marks it targeted
    virtual void SetTarget(TESObjectREFR*) {}
    virtual TESObjectREFR* GetDoor() {
        return nullptr;
    }
    virtual void SetDoor(TESObjectREFR*) {}
    virtual void SetAcquireObject(ObjectstoAcquire*) {}
    virtual ObjectstoAcquire* GetSavedAcquireObject() {
        return nullptr;
    }
    virtual void SetSavedAcquireObject(ObjectstoAcquire*) {}
    // the face animation data of apActor
    virtual BSFaceGenAnimationData* GetFaceAnimationData(Actor*) {
        return nullptr;
    }
    // the weapon the actor holds
    virtual ItemChange* GetCurrentWeapon() {
        return nullptr;
    }
    // the ammo the actor uses
    virtual ItemChange* GetCurrentAmmo() {
        return nullptr;
    }
    // a lip file is queued
    virtual bool GetLipQuequed() {
        return false;
    }
    virtual void SetLipQuequed(bool) {}
    virtual int32_t GetNumberOfItemsActivated() {
        return 0;
    }
    virtual void SetNumberOfItemsActivated(int32_t) {}
    // equips apWeapon as the current weapon and stores its bone pointers
    virtual bool SetCurrentWeapon(ItemChange*, NiNode*, bool) {
        return false;
    }
    virtual bool SetCurrentTorch(ItemChange*) {
        return false;
    }
    // takes ownership of apAmmo; the previous ammo is destroyed
    virtual bool SetCurrentAmmo(ItemChange*) {
        return false;
    }
    virtual bool SetCurrentShield(ItemChange*) {
        return false;
    }
    // equips or unequips the queued items whose models have loaded
    virtual void ResolveEquipmentQueue(Actor*) {}
    // true when the item with this extra data is queued to be equipped (abEquipped) or unequipped
    virtual bool IsItemQueued(TESBoundObject*, ExtraDataList*, bool) {
        return false;
    }
    // queues an item to be equipped or unequipped once its model loads
    virtual void AddQueuedItem(
        Actor* apActor,
        bool abEquip,
        TESBoundObject* apObject,
        int32_t aiNumber,
        ExtraDataList* apExtra,
        bool abNow,
        bool abForceEquip,
        bool abFromMenus,
        bool abForceUnequip,
        bool abEquipCalling,
        bool abPlayPickUpSounds
    );
    // removes the queued item with this extra data
    virtual void RemoveQueuedItem(TESBoundObject*, ExtraDataList*) {}
    // removes every queued item and frees the queue
    virtual void RemoveAllQueuedItems() {}
    // node the projectiles come out of
    virtual NiAVObject* GetFireNode() {
        return nullptr;
    }
    virtual void SetFireNode(NiAVObject*) {}
    // finds the weapon bone in apManager or ap3D and removes its scabbard
    virtual void StoreBonePointers(NiControllerManager*, NiNode*) {}
    // node the weapon is attached to: the parent bone of apBiped's weapon, or pWeaponBone without a biped
    virtual NiNode* GetWeaponBone(BipedAnim*) {
        return nullptr;
    }
    virtual NiNode* GetTorchBone(BipedAnim*) {
        return nullptr;
    }
    virtual NiNode* GetShieldBone(BipedAnim*) {
        return nullptr;
    }
    virtual NiNode* GetQuiverBone(BipedAnim*) {
        return nullptr;
    }
    virtual NiNode* GetRightWeaponBone(BipedAnim*) {
        return nullptr;
    }
    virtual NiNode* GetLeftWeaponBone(BipedAnim*) {
        return nullptr;
    }
    // the weapon is a one hand grenade
    virtual bool IsCurrentWeaponGrenade() {
        return false;
    }
    // the weapon is a one hand mine
    virtual bool IsCurrentWeaponMine() {
        return false;
    }
    // true when the weapon is a one hand grenade, mine or thrown weapon
    virtual bool IsCurrentWeaponGrenadeOrMine() {
        return false;
    }
    // true when the weapon is a one hand thrown weapon
    virtual bool IsCurrentWeaponThrown() {
        return false;
    }
    virtual Animation* GetAnimation() {
        return nullptr;
    }
    virtual bool GetLockedLocation() {
        return false;
    }
    virtual void SetLockedLocation(bool) {}
    virtual bool GetIsAggressor() {
        return false;
    }
    virtual void SetIsAggressor(bool) {}
    // attaches the weapon to the drawn or the sheathed node
    virtual void ForceWeaponDrawnSheathed(bool, BipedAnim*, Animation*, Actor*) {}
    virtual void SetItemBeingUsed(TESBoundObject*) {}
    virtual TESBoundObject* GetItemBeingUsed() {
        return nullptr;
    }
    virtual void UpdateReparentWeapon(Actor*) {}
    virtual void CreateFollownoEscort(Actor*, bool) {}
    virtual float GetAwarePlayerTimer() {
        return 0.0f;
    }
    virtual void ModAwarePlayerTimer(float) {}
    virtual void SetCombatDelayTimer(float) {}
    virtual bool CheckCombatDelayTimer() {
        return true;
    }
    // adds apActor to the actors spoken to recently
    virtual void AddTalkedtoActor(Actor*) {}
    virtual bool ComputeCanInitiateDialogueWithNPC(Actor*, Actor*) {
        return false;
    }
    virtual bool ComputeCanRespondToDialogueFromNPC(Actor*, Actor*) {
        return false;
    }
    virtual bool IsActorInsideAmbushArea(Actor*, Actor*) {
        return false;
    }
    virtual bool CheckIfThereSomeoneTalkWith(Actor*) {
        return false;
    }
    virtual bool ComputeAllowSandboxConversation(Actor*, Actor*) {
        return false;
    }
    virtual float GetPotentialThreatDamage(Actor* apActor, Actor* apTarget);
    // the package that runs once, over the current package
    virtual TESPackage* GetRunOncePackage() {
        return nullptr;
    }
    virtual void SetRunOncePackage(TESPackage*, MobileObject*) {}
    virtual void ClearRunOncePackage() {}
    virtual void SetRunOnceProcedureIndex(int32_t) {}
    virtual int32_t GetRunOnceProcedureIndex() {
        return 0;
    }
    virtual void AddToRunOnceProcedureIndex(int32_t) {}
    virtual void ClearGreetingInfoData() {}
    virtual bool IsPackageDone(Actor*) {
        return false;
    }
    // the package the actor is running, null while none is
    virtual TESPackage* GetCurrentPackage();
    // starts a package for the actor this process belongs to
    virtual void SetCurrentPackage(TESPackage* apNewPackage, MobileObject* apMob);
    // stops the running package
    virtual void ClearCurrentPackage();
    virtual void SetCurrentProcedureIndex(int32_t aiNewIndex);
    virtual int32_t GetCurrentProcedureIndex();
    virtual void AddToCurrentProcedureIndex(int32_t aiOffset);
    virtual void SetupNewPackage() {}
    virtual DialogueItem* GetSubtitleItem() {
        return nullptr;
    }
    virtual void SetSubtitleItem(DialogueItem*) {}
    virtual void AddAvoidPathingArea(
        Actor* apActor,
        NiPoint3 apoint,
        float afRadius,
        float afTimeToExpire,
        TESObjectREFR* apRef,
        TESObjectSTAT* apMarker
    );
    virtual PathingAvoidNodeArray* CreateAvoidArray(Actor*) {
        return nullptr;
    }
    virtual bool IsAvoidAreaInAvoidPathingList(TESObjectREFR*) {
        return false;
    }
    virtual void SetAvoidWaitTimer(float) {}
    virtual void RemoveAvoidPathingNode(TESObjectREFR*) {}
    virtual float GetHeadTrackChangeTimer() {
        return 1.0f;
    }
    virtual void SetHeadTrackChangeTimer(float) {}
    virtual void ClearLastHeadTrackTarget() {}
    virtual ActorPackageData* GetRunOncePackageInstanceData() {
        return nullptr;
    }
    // per-actor data of the running package, null while none is
    virtual ActorPackageData* GetInstanceDataThatIsRunning();
    virtual ActorPackage* GetActorPackageThatIsRunning();
    // the run once package when there is one, otherwise the current package
    virtual TESPackage* GetPackageThatIsRunning();
    virtual int32_t GetProcedureIndexRunning();
    virtual void SetProcedureIndexRunning(int32_t aiNewIndex);
    // adds ainumb to the procedure index of the running package
    virtual void AddToProcedureIndexRunning(MobileObject* apActor, int32_t ainumb);
    // the character controller of the actor
    virtual bhkCharacterController* GetCharController() {
        return nullptr;
    }
    // replaces the character controller
    virtual void SetCharController(NiPointer<bhkCharacterController> apCon);
    // stops the move message of the actor
    virtual void EndMoveMessage(Actor*) {}
    // runs the follow procedure
    virtual void ProcessFollow(Actor*, bool, int32_t, bool) {}
    // runs the pursue procedure
    virtual void ProcessPursue(Actor*, bool, bool, int32_t) {}
    // runs the flee procedure
    virtual void ProcessFlee(Actor*) {}
    // greets with the topic
    virtual void ProcessGreet(Actor*, TESTopic*, bool, bool, bool, bool) {}
    // runs the observe combat procedure
    virtual void ProcessObserveCombat(Actor*) {}
    // gets the actor out of its furniture at once
    virtual void ProcessGetUpOneHour(Actor*) {}
    // sits or sleeps on the furniture of the package
    virtual bool SitOrSleep(Actor*) {
        return true;
    }
    // true when the furniture marker needs the character controller off
    virtual bool ShouldDisableCharController(TESObjectREFR*, uint32_t) {
        return false;
    }
    // gets the actor up from its furniture
    virtual bool ProcessGetUp(Actor*) {
        return true;
    }
    // runs the travel procedure
    virtual void ProcessTravel(Actor*, bool, bool, int32_t, bool) {}
    virtual WadingWaterData* GetWadingWaterData() {
        return nullptr;
    }
    // removes the actor from the detection list aiWhich
    virtual void RemoveDetectionActor(Actor*, int32_t) {}
    virtual void SetDetectionTimer(float) {}
    // line of sight of the detection data of apTarget
    virtual bool GetLineSightonActor(Actor*, Actor*, int32_t, bool) {
        return false;
    }
    // 360 degree line of sight of the detection data of apTarget
    virtual bool Get360LineSightonActor(Actor*, Actor*, int32_t) {
        return false;
    }
    // the detection value of the actor, 0x7fffffff when not in the list
    virtual int32_t GetDetectionActor(Actor*, int32_t) {
        return 0;
    }
    // the actor talks with the player
    virtual bool GetTalkingtoPC() {
        return false;
    }
    virtual void SetTalkingtoPC(bool) {}
    // updates the followers of the actor
    virtual void UpdateFollowers(Actor*, float) {}
    // disposition of the actor towards apRef, or towards the target, 50 by default
    virtual int32_t GetEmotionsDispostion(Actor*, TESObjectREFR*) {
        return 0;
    }
    // the emotion the actor spoke with last
    virtual DIALOGUE_EMOTION GetLastSpeakingEmotion() {
        return static_cast<DIALOGUE_EMOTION>(0);
    }
    virtual void SetLastSpeakingEmotion(DIALOGUE_EMOTION) {}
    // runs detection for the actor
    virtual void RunDetection(Actor*, float) {}
    // time until detection runs
    virtual float GetDetectionTimer() {
        return 0.0f;
    }
    // counts the detection timer down by the frame time
    virtual void ReduceDetectionTimer() {}
    virtual void SetBreathTimer(float) {}
    // time until the actor breathes
    virtual float GetBreathTimer() {
        return 20.0f;
    }
    virtual void SetSurfacing(bool) {}
    virtual bool GetSurfacing() {
        return false;
    }
    // the greeting flag
    virtual bool GetGreetingFlag() {
        return false;
    }
    virtual void SetGreetingFlag(bool) {}
    // delay of the sound
    virtual float GetSoundDelay() {
        return 0.0f;
    }
    virtual void SetSoundDelay(float) {}
    // the actor is alerted
    virtual bool GetAlert() {
        return false;
    }
    virtual void SetAlert(bool) {}
    // the combat package is finishing
    virtual bool GetFinishingCombatPackage() {
        return false;
    }
    virtual void SetFinishingCombatPackage(bool) {}
    // gathers the objects of type aeType around the actor and returns the first as a new item change
    virtual ItemChange* CreateWeaponAcquireList(Actor*, PACKAGE_OBJECT_TYPE) {
        return nullptr;
    }
    // empties the list of objects to acquire
    virtual void ClearAcquireList() {}
    // time until the actor plays an idle
    virtual float GetIdleTimer() {
        return 0.0f;
    }
    virtual void SetIdleTimer(float) {}
    // starts combat of the actor with apTarget
    virtual bool EnterCombat(
        Actor* apActor,
        Actor* apTarget,
        bool abCheckForAmbush,
        bool absendalarm,
        CombatGroup* apGroup,
        bool abfleeing,
        bool abForceFlee,
        bool abAggressor,
        char acTeam,
        bool bforcecombat,
        bool bIgnoreDefensivePackage,
        bool abCastSpell,
        TESPackage* apPackage
    );
    // a guard calls the other guards on apCriminal
    virtual void GuardCallforHelp(Actor*, Actor*) {}
    // sends the crime alarm to the actors near the actor
    virtual void CallforHelp(Actor*, Actor*, Crime*) {}
    virtual void SetNumberGuardsArresting(int32_t) {}
    // the move mode flags for walking or running over the distance
    virtual int32_t CalculateMoveMode(
        Actor* apActor,
        float distance,
        float afshortdist,
        float aflongdist,
        bool balarmcombat,
        bool abpuretravel
    );
    // sets the move mode of the actor
    virtual void SetActorsAnimation(Actor*, int32_t, bool) {}
    // finishes the death of the actor
    virtual void FinishDying(Actor*) {}
    virtual bool ProcessStandOff(Actor*, float, int32_t) {
        return false;
    }
    // true while a run once package runs
    virtual bool IsRunningRunOnce() {
        return false;
    }
    // the actor follows someone
    virtual bool IsAFollower() {
        return false;
    }
    virtual void SetFollower(bool) {}
    // condition of the equipped weapon
    virtual int32_t GetWeaponCondition(Actor*) {
        return 0;
    }
    // a lip file is loading
    virtual bool GetWaitingforLipFile() {
        return false;
    }
    virtual void SetWaitingforLipFile(bool) {}
    virtual void SetLipFileFailed(bool) {}
    // loading the lip file failed
    virtual bool GetLipFileFailed() {
        return false;
    }
    // the lip sync animation
    virtual LipSynchAnim* GetLipAnim() {
        return nullptr;
    }
    virtual void SetLipAnim(LipSynchAnim*) {}
    virtual void SetPlantedExplosive(bool) {}
    // the actor has a live grenade or planted explosive
    virtual bool GetPlantedExplosive() {
        return false;
    }
    // the idle played last
    virtual TESIdleForm* GetLastIdlePlayed() {
        return nullptr;
    }
    virtual void SetLastIdlePlayed(TESIdleForm*) {}
    virtual void FreeUpSpecialIdle(Actor*) {}
    virtual int32_t GetActorValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor);
    virtual float GetActorFloatValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor);
    virtual void TempModActorValue(Actor*, ActorValue::Index, int32_t) {}
    virtual void TempModActorValue(Actor*, ActorValue::Index, float) {}
    virtual void DamageModActorValue(Actor*, ActorValue::Index, int32_t) {}
    virtual void DamageModActorValue(Actor*, ActorValue::Index, float) {}
    // marks the cached actor value and its linked values modified
    virtual void SetCachedActorValueOutOfDate(ActorValue::Index) {}
    virtual void InitializeActiveEffectList() {}
    // the active magic effects
    virtual BSSimpleList<ActiveEffect*>* GetActiveEffectList() {
        return nullptr;
    }
    virtual void DestroyActiveEffectList() {}
    // forwards to MagicCaster::CastAbility
    virtual void CastAbility(MagicCaster*, SpellItem*, bool) {}
    // forwards to MagicCaster::TransferDisease
    virtual void TransferDisease(MagicCaster*, SpellItem*, MagicTarget*, bool) {}
    // the spell the actor casts
    virtual MagicItem* GetCurrentSpell() {
        return nullptr;
    }
    virtual void SetCurrentSpell(MagicItem*) {}
    // the target of the spell the actor casts
    virtual MagicTarget* GetDesiredTarget() {
        return nullptr;
    }
    virtual void SetDesiredTarget(MagicTarget*) {}
    // the magic node has to be checked
    virtual bool GetShouldCheckMagicNode() {
        return false;
    }
    virtual void SetShouldCheckMagicNode(bool) {}
    virtual void Move(MobileObject*, NiPoint3) {}
    // the ANIMATION_ACTION being played, -1 for none
    virtual ANIMATION_ACTION GetAnimAction() {
        return static_cast<ANIMATION_ACTION>(-1);
    }
    // the animation sequence of the action
    virtual BSAnimGroupSequence* GetAnimActionAnimSeq() {
        return nullptr;
    }
    virtual void SetAnimAction(ANIMATION_ACTION, BSAnimGroupSequence*) {}
    // an automatic weapon fires at least once
    virtual bool GetAutomaticFireAtLeastOnce() {
        return false;
    }
    virtual void SetAutomaticFireAtLeastOnce(bool) {}
    virtual bool CanAttack() {
        return true;
    }
    virtual bool CanForceGreet(Actor*) {
        return false;
    }
    virtual void SetIronSights(bool) {}
    // the actor aims down the sights
    virtual bool GetIronSights() {
        return false;
    }
    // the movement animation group is loaded
    virtual bool HasMovementAnim(ANIM_GROUP_ENUM) {
        return true;
    }
    // the knocked state, sign extended
    virtual KNOCK_STATE_ENUM GetKnockState() {
        return KNOCK_STATE_NORMAL;
    }
    virtual void SetKnockState(int8_t) {}
    virtual void KnockDown(Actor*) {}
    virtual void KnockExplosion(Actor*, NiPoint3, float) {}
    virtual void UpdateKnockState(Actor*) {}
    virtual void ClearKnockState(Actor*) {}
    virtual void SetAimSequence(AIM_ENUM, BSAnimGroupSequence*) {}
    virtual void UpdateAimSequences(Actor*) {}
    // angle the aim follows the look target at
    virtual float GetAimLooking() {
        return 0.0f;
    }
    virtual void AddGunDrift(float) {}
    virtual void ClearAutomaticShotsToFire(bool) {}
    // Counts down the automatic shot delay; returns the number of automatic shots that fire now.
    virtual uint8_t GetAutomaticShotsToFire(Actor*) {
        return 0;
    }
    virtual void SetShotsToFire(uint8_t) {}
    virtual uint8_t GetShotsToFire() {
        return 1;
    }
    virtual float GetAutomaticShotDelay() {
        return 0.0f;
    }
    // adds aiNumAttacks to the shots fired in the current burst
    virtual void AttackCallback(int32_t) {}
    virtual bool GetWantWeaponDrawn() {
        return false;
    }
    virtual void SetWantWeaponDrawn(bool) {}
    virtual bool GetWeaponDrawn() {
        return false;
    }
    virtual void SetWeaponDrawn(Actor*, bool) {}
    virtual void SaveWeaponLastPos(NiPoint3) {}
    virtual const NiPoint3* GetWeaponLastPos();
    virtual void Update3dModel(Actor*) {}
    virtual void Set3DUpdateFlag(RESET_3D_FLAGS) {}
    virtual void Clear3DUpdateFlag(RESET_3D_FLAGS) {}
    virtual void ClearAll3DUpdateFlags() {}
    virtual bool Get3DUpdateFlag(RESET_3D_FLAGS) {
        return false;
    }
    virtual uint8_t GetAll3DUpdateFlags() {
        return 0;
    }
    // drops the acquire objects whose reference has the form id aID
    virtual void ClearObjectFromAcquireList(uint32_t) {}
    // drops the acquire objects whose reference is apRef
    virtual void ClearObjectFromAcquireList(TESObjectREFR*) {}
    virtual TESObjectREFR* GetLastGreeted() {
        return nullptr;
    }
    virtual void SetLastGreeted(TESObjectREFR*) {}
    // a copy of the sound handle aeType
    virtual BSSoundHandle GetSoundHandle(ACTOR_SOUND_HANDLE aeType);
    virtual void SetSoundHandle(ACTOR_SOUND_HANDLE aeType, BSSoundHandle aSound);
    virtual void StopSoundHandle(ACTOR_SOUND_HANDLE) {}
    virtual void StartTorchSound(Actor*) {}
    virtual bool GetMovementStoped() {
        return false;
    }
    virtual void ClearMovementStoped() {}
    virtual void SetDetectionModifier(float) {}
    // restarts the detection modifier timer from a game setting
    virtual void SetDetectionModifierTimer() {}
    virtual float GetDetectionModifier() {
        return 0.0f;
    }
    virtual float GetGreetingTimer() {
        return 0.0f;
    }
    virtual void SetGreetingTimer(float) {}
    virtual bool IsTalking(MobileObject*) {
        return false;
    }
    virtual SIT_SLEEP_STATE GetSitSleepState() {
        return static_cast<SIT_SLEEP_STATE>(0);
    }
    virtual void SetSitSleepState(Actor*, SIT_SLEEP_STATE, TESObjectREFR*, uint8_t) {}
    // the type of the furniture marker the actor uses, 0 without one
    virtual uint32_t GetFurnitureMarkerID() {
        return 0;
    }
    virtual TESObjectREFR* GetCurrentFurniture() {
        return nullptr;
    }
    // removes apFurnitureRef from the known chairs and beds, and drops it as the current furniture
    virtual void RemoveFurnitureRef(TESObjectREFR*) {}
    virtual uint32_t GetCurrentFurnitureIndex() {
        return 0;
    }
    virtual FurnitureMark* GetFurnitureMarker() {
        return nullptr;
    }
    virtual bool LoadDynamicIdle(Actor*) {
        return false;
    }
    virtual void LoadSitSleepIdle(Actor*, Animation*) {}
    virtual bool ContinuingPackageforPC() {
        return false;
    }
    virtual void SetContinuingPackage(bool) {}
    virtual bool IsTargetActivated() {
        return false;
    }
    virtual void SetTargetActivated(bool) {}
    virtual bool HasBeenAttacked() {
        return false;
    }
    virtual void SetBeenAttacked(bool) {}
    virtual BSShaderPPLightingProperty* GetLightingShaderProperty() {
        return nullptr;
    }
    virtual void SetLightingShaderProperty(BSShaderPPLightingProperty*) {}
    virtual void CalculateLightValue(Actor*, bool) {}
    // the detection entry for apActor in the detecting (abWhichList != 0) or detected list
    virtual DetectionState* GetDetectionState(Actor*, int32_t) {
        return nullptr;
    }
    virtual DetectionState* InsertIntoDetectionArray(Actor*, DETECTION_LEVEL) {
        return nullptr;
    }
    virtual float GetLightLevelTimer() {
        return 0.0f;
    }
    // sets the time until the light level is updated
    virtual void SetLightLevelTimer(float) {}
    // the generic location of the package
    virtual TESObjectREFR* GetGenericLocation() {
        return nullptr;
    }
    // sets the generic location of the package
    virtual void SetGenericLocation(TESObjectREFR*) {}
    // the second generic location of the package
    virtual TESObjectREFR* GetSecondGenericLocation() {
        return nullptr;
    }
    // sets the second generic location of the package
    virtual void SetSecondGenericLocation(TESObjectREFR*) {}
    // clears the package data of the process
    virtual void ClearCurrentDataforProcess(Actor*) {}
    virtual void SetCommandingActor(Actor*) {}
    // the actor that commands this one
    virtual Actor* GetCommandingActor() {
        return nullptr;
    }
    virtual void SetLastBoundWeapon(TESObjectWEAP*) {}
    // the weapon that was bound last
    virtual TESObjectWEAP* GetLastBoundWeapon() {
        return nullptr;
    }
    // sets when the package a script started ends
    virtual void SetPackageEndTimerValue(float) {}
    // when the package a script started ends
    virtual float GetPackageEndTimerValue() {
        return 0.0f;
    }
    // sets the furniture the actor uses
    virtual void SetFurniture(TESObjectREFR*, uint8_t, FurnitureMark*) {}
    // removes and reattaches the weapon and other equipped items
    virtual void RemoveAndAttachItems(MobileObject*) {}
    // size of the save data of the process
    virtual uint16_t GetSaveSize(uint32_t aiFlags, MobileObject* pmob);
    virtual void SaveGame(uint32_t aiFlags, MobileObject* pmob);
    virtual void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    virtual void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags, MobileObject* pmob);
    virtual void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    virtual void InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, MobileObject* pmob);
    virtual void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    virtual void FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, MobileObject* pmob);
    virtual void Revert(uint32_t aiFlags, MobileObject* pmob);
    virtual void Revert(BGSLoadFormBuffer* apLoadGameBuffer);
    virtual void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // the shader of the enchantment of the weapon
    virtual TESEffectShader* GetWeaponEnchantmentVisuals() {
        return nullptr;
    }
    // sets the shader of the enchantment of the weapon
    virtual void SetWeaponEnchantmentVisuals(TESEffectShader*) {}
    // the magic shader effects have to be refreshed
    virtual bool GetShouldRefreshMagicShadersEffects() {
        return false;
    }
    // flags the magic shader effects for a refresh
    virtual void SetShouldRefreshMagicShadersEffects(bool, bool, bool) {}
    // refreshes the magic shader effects on the actor
    virtual void RefreshMagicShaderEffects(Actor*) {}
    virtual void SetBeginIdlesPlayed(bool) {}
    // the begin idles of the package were played
    virtual bool GetBeginIdlesPlayed() {
        return false;
    }
    virtual void SetEndIdlesPlayed(bool) {}
    // the end idles of the package were played
    virtual bool GetEndIdlesPlayed() {
        return false;
    }
    // runs the begin script and topic of the package
    virtual void StartNewPackage(Actor*, TESPackage*) {}
    // runs the change script and topic of the package
    virtual void ChangePackage(Actor*, TESPackage*) {}
    // runs the end script and topic of the package
    virtual void PackageDone(Actor*, TESPackage*) {}
    // the package was done once
    virtual bool IsPackageDoneOnce() {
        return false;
    }
    virtual void SetPackageDoneOnce(bool) {}
    // alpha multiplier of the actor
    virtual float GetAlphaMult() {
        return 1.0f;
    }
    virtual void SetAlphaMult(float) {}
    // refraction power a script set
    virtual float GetScriptRefractionPower() {
        return 0.0f;
    }
    virtual void SetScriptRefractionPower(float) {}
    virtual void UpdateWadingWater(Actor*) {}
    // number of guards pursuing the actor
    virtual int32_t GetNumberGuardsArresting() {
        return 0;
    }
    // adds to the number of guards pursuing the actor
    virtual void ModNumberGuardsArresting(int32_t) {}
    // the flare flags gathered from the active effects contain the flare type
    virtual bool ShouldCheckFlare(BSSimpleList<ActiveEffect*>*, MagicSystem::FlareType) {
        return false;
    }
    virtual void SetRefreshFlareFlags() {}
    // the leveled spells of the actor
    virtual BSSimpleList<SpellItem*>* GetLeveledSpells(Actor*, bool) {
        return nullptr;
    }
    virtual void SetHasHealingSpells(int32_t) {}
    // the actor has a healing spell
    virtual int32_t GetHasHealingSpells() {
        return 0;
    }
    virtual void SetHasHealingPotions(int32_t) {}
    // the actor has a healing potion
    virtual int32_t GetHasHealingPotions() {
        return 0;
    }
    // temporary modifier of the actor value
    virtual float GetTemporaryModifier(ActorValue::Index) {
        return 0.0f;
    }
    // the permanent modifier of an actor value
    virtual float GetPermanentModifier(ActorValue::Index) {
        return 0.0f;
    }
    // the damage modifier of an actor value
    virtual float GetDamageModifier(ActorValue::Index) {
        return 0.0f;
    }
    // the bound of the actor
    virtual BSBound* GetBSBound() {
        return nullptr;
    }
    // sets the bound of the actor
    virtual void SetBSBound(BSBound*) {}
    // makes the next update run regardless of the timers
    virtual void SetForceNextUpdate(bool) {}
    // the next update runs regardless of the timers
    virtual bool GetForceNextUpdate() {
        return false;
    }
    // the actor has to talk to the player after getting up
    virtual void SetNeedTalkPlayer(bool) {}
    // the actor has to talk to the player after getting up
    virtual bool GetNeedTalkPlayer() {
        return false;
    }
    // actors inside the aggro radius
    virtual BSSimpleList<Actor*>* GetAggroRadiusActorList() {
        return nullptr;
    }
    // lets a guard attack the actor that activated the reference it guards
    virtual void NotifyGuardAboutActivation(Actor*, TESObjectREFR*, TESObjectREFR*) {}
    // the fade state
    virtual FADE_STATE GetFadeState() {
        return FADE_NORMAL;
    }
    // queues an action after the animation
    virtual void AddPostAnimationAction(BaseProcess::POSTANIM_ACTION) {}
    // the actions queued after the animation
    virtual int32_t GetPostAnimationActions() {
        return 0;
    }
    // clears the actions queued after the animation
    virtual void ClearPostAnimationActions() {}
    // removes an action queued after the animation
    virtual void RemovePostAnimationAction(BaseProcess::POSTANIM_ACTION) {}
    // sets the default head tracking target
    virtual void SetDefaultHeadTrackTarget(TESObjectREFR*) {}
    // sets the action head tracking target
    virtual void SetActionHeadTrackTarget(TESObjectREFR*) {}
    // sets the script head tracking target
    virtual void SetScriptHeadTrackTarget(TESObjectREFR*) {}
    // sets the combat head tracking target
    virtual void SetCombatHeadTrackTarget(TESObjectREFR*) {}
    // sets the dialog head tracking target
    virtual void SetDialogHeadTrackTarget(TESObjectREFR*) {}
    // sets the use weapon head tracking target
    virtual void SetUseWeaponHeadTrackTarget(TESObjectREFR*) {}
    // sets the head tracking target of a type
    virtual void SetHeadTrackTarget(HEAD_TRACK_TYPE, TESObjectREFR*) {}
    // clears the default head tracking target
    virtual void ClearDefaultHeadTrackTarget() {}
    // clears the action head tracking target; abHoldDefault makes it the default target
    virtual void ClearActionHeadTrackTarget(bool) {}
    // clears the script head tracking target; abHoldDefault makes it the default target
    virtual void ClearScriptHeadTrackTarget(bool) {}
    // clears the combat head tracking target
    virtual void ClearCombatHeadTrackTarget() {}
    // clears the dialog head tracking target; abHoldDefault makes it the default target
    virtual void ClearDialogHeadTrackTarget(bool) {}
    // clears the use weapon head tracking target; abHoldDefault makes it the default target
    virtual void ClearUseWeaponHeadTrackTarget(bool) {}
    // counts the detection modifier timer down by the frame time
    virtual void ModDetectionModifierTimer() {}
    // time the detection modifier applies
    virtual float GetDetectionModifierTimer() {
        return 0.0f;
    }
    // clears every head tracking target
    virtual void ClearAllHeadTrackTargets() {}
    // clears every head tracking slot that holds the reference
    virtual void RemoveHeadTrackTarget(TESObjectREFR*) {}
    // no other head tracking target is set and the head track change timer ran out
    virtual bool CanSetDefaultHeadTrackTarget() {
        return false;
    }
    // no head tracking target above the action slot is set
    virtual bool CanSetActionHeadTrackTarget() {
        return false;
    }
    // the dialog head tracking target is not set
    virtual bool CanSetCombatHeadTrackTarget() {
        return false;
    }
    // the head tracking target that has the highest slot with its flag set
    virtual TESObjectREFR* GetCurrentHeadTrackTarget() {
        return nullptr;
    }
    // the head tracking target of a type
    virtual TESObjectREFR* GetCurrentHeadTrackTarget(HEAD_TRACK_TYPE) {
        return nullptr;
    }
    // the highest head tracking slot with its flag set, 0 when none
    virtual HEAD_TRACK_TYPE GetCurrentHeadTrackType() {
        return HEAD_TRACK_TYPE_DEFAULT;
    }
    // name of the current head tracking type
    virtual const char* GetCurrentHeadTrackTypeString() {
        return "";
    }
    // forces the actor to rotate
    virtual void SetForceRotate(bool) {}
    // the actor is forced to rotate
    virtual bool GetForceRotate() {
        return false;
    }
    // the last head tracking target
    virtual TESObjectREFR* GetLastHeadTrackTarget() {
        return nullptr;
    }
    virtual bool IsInLastDefaultTargetArray(TESObjectREFR*) {
        return false;
    }
    // marks the actor as a summoned creature
    virtual void SetIsSummonedCreature(bool) {}
    // the actor is a summoned creature
    virtual bool GetIsSummonedCreature() {
        return false;
    }
    // when the actor died
    virtual float GetDeathTime() {
        return 0.0f;
    }
    // sets when the actor died
    virtual void SetDeathTime(float) {}
    // adds an arrow stuck in the actor
    virtual void AddAttachedArrow(ArrowProjectile*) {}
    // removes an arrow stuck in the actor
    virtual void RemoveAttachedArrow(ArrowProjectile*) {}
    // moves the arrows stuck in the actor into the cell of the reference
    virtual void MoveAttachedArrowsToParentCell(TESObjectREFR*) {}
    // arrows stuck in the actor
    virtual BSSimpleList<ArrowProjectile*>* GetAttachedArrowList() {
        return nullptr;
    }
    // creates the muzzle flash of the projectile when none exists
    virtual void CreateMuzzleFlash(Projectile*, TESObjectREFR*) {}
    // the muzzle flash of the weapon
    virtual MuzzleFlash* GetCurrentMuzzleFlash() {
        return nullptr;
    }
    virtual bool GetEnableMuzzleFlash() {
        return false;
    }
    // turns the muzzle flash of the weapon on
    virtual void EnableMuzzleFlash() {}
    // destroys the muzzle flash of the weapon
    virtual void ClearMuzzleFlash() {}
    // looks up the limb root nodes of the body part data under apNode
    virtual void StoreDamageNodes(NiNode*, BGSBodyPartData*) {}
    // stores the limb root node and clears the head/torso node for those limbs
    virtual void StoreDismemberedLimbNode(BGSBodyPart::LIMB_ENUM, NiNode*) {}
    // the limb root node at aeIndex
    virtual NiNode* GetDamageNode(BGSBodyPart::LIMB_ENUM) {
        return nullptr;
    }
    // the head node
    virtual NiAVObject* GetHeadNode() {
        return nullptr;
    }
    // the torso node
    virtual NiAVObject* GetTorsoNode() {
        return nullptr;
    }
    // the actor fires an automatic weapon
    virtual bool IsSustainedFire() {
        return false;
    }
    virtual void SetSustainedFire(bool) {}
    // rescans the inventory of apActor for heavy armor, power armor and backpacks
    virtual void UpdateArmorWorn(Actor*) {}
    // the actor wears heavy armor
    virtual bool IsHeavyBodyArmorWorn() {
        return false;
    }
    // the actor wears a power armor torso
    virtual bool IsPowerBodyArmorWorn() {
        return false;
    }
    // the actor wears a power armor helmet
    virtual bool IsPowerArmorHelmetWorn() {
        return false;
    }
    // the actor wears a backpack
    virtual bool HasBackPackWorn() {
        return false;
    }
    virtual void SetCurrentlyReanimating(bool) {}
    // the actor is being reanimated
    virtual bool GetCurrentlyReanimating() {
        return false;
    }
    virtual void UpdateRadiation(Actor*, float) {}
    // reserves clones of the projectile of the weapon once its model is loaded
    virtual void TryReserveProjectileClones(Actor*) {}
    virtual void SetAnimActionSuccess(int32_t) {}
    virtual void FinishSetupSpecialIdle(Actor*) {}
    virtual void SavePackageToExtraData(MobileObject*) {}
    virtual void LoadPackageFromExtraData(MobileObject*) {}
    // the idle the process was told to play
    virtual TESIdleForm* GetCurrentProcessIdle() {
        return nullptr;
    }
    virtual void SetCurrentProcessIdle(TESIdleForm*) {}
    virtual void SetCurrentProcessAnimIdleLoaded(AnimIdle*, KFModel*) {}
    // animations the idle plays
    virtual AnimIdle* GetCurrentProcessAnimIdle() {
        return nullptr;
    }
    // the animation file of the idle
    virtual KFModel* GetCurrentProcessAnimIdleKF() {
        return nullptr;
    }
    // adds afAmount to the tracked damage when apAggressor is the player or a teammate
    virtual void ModTrackedDamage(Actor*, float) {}
    // damage the player and teammates dealt
    virtual float GetTrackedDamage() {
        return 0.0f;
    }
    // light level at the actor
    virtual float GetActorLightLevel() {
        return 0.0f;
    }
    virtual void SetActorLightLevel(float) {}
    // queues a limb to be hidden later
    virtual void DeferHideDismemberedLimb(BGSBodyPart::LIMB_ENUM, NiNode*, NiAVObject*, bool) {}
    virtual void UpdateDeferredHideDismemberedLimbs(Actor*, float) {}
    // destroys the previous lip sync animation and stores the new one
    virtual void SetCreatureLipSynchAnim(LipSynchAnim*) {}
    // the lip sync animation of a creature
    virtual LipSynchAnim* GetCreatureLipSynchAnim() {
        return nullptr;
    }
    virtual void SetCreatureLipSynchStartTime(uint32_t) {}
    // when the lip sync of a creature started
    virtual uint32_t GetCreatureLipSynchStartTime() {
        return 0;
    }
    virtual void AddRadiationMagicDelta(float) {}
    virtual void RemoveRadiationMagicDelta(float) {}
    // radiation from magic effects
    virtual float GetRadiationMagicDelta() {
        return 0.0f;
    }
    virtual void SetRadiationWaterDelta(float) {}
    // radiation from the water
    virtual float GetRadiationWaterDelta() {
        return 0.0f;
    }
    virtual void SetRadiationDelta(float) {}
    // radiation of the area
    virtual float GetRadiationDelta() {
        return 0.0f;
    }
    // the last hit the actor took
    virtual HitData* GetLastHitData() {
        return nullptr;
    }
    // copies apData into the last hit, allocating it first
    virtual void SetLastHitData(HitData*) {}
    // resets the last hit, allocating it first
    virtual void ClearLastHitData() {}
    // the last hit the actor dealt
    virtual HitData* GetLastAttackHitData() {
        return nullptr;
    }
    // copies apData into the last dealt hit, allocating it first
    virtual void SetLastAttackHitData(HitData*) {}
    // resets the last dealt hit, allocating it first
    virtual void ClearLastAttackHitData() {}
    // condition stage of the weapon
    virtual int32_t GetWeaponConditionStage() {
        return 0;
    }
    virtual void SetWeaponConditionStage(int32_t) {}
    // the facegen node of the actor
    virtual BSFaceGenNiNode* GetFaceNode() {
        return nullptr;
    }
    virtual void SetFaceNode(BSFaceGenNiNode*) {}
    // the skinned facegen node
    virtual BSFaceGenNiNode* GetFaceSkinnedNode() {
        return nullptr;
    }
    // set the skinned facegen node
    virtual void SetFaceSkinnedNode(BSFaceGenNiNode*) {}
    // the head anim face shape
    virtual NiAVObject* GetHeadAnims() {
        return nullptr;
    }
    // set the head anim face shape
    virtual void SetHeadAnims(NiAVObject*) {}
    virtual void CheckCachedValueFlags(Actor*) {}
    // set the path look-at target
    virtual void SetPathLookAtTarget(TESObjectREFR*) {}
    // clear the path look-at target if it is this ref
    virtual void ClearPathLookTarget(TESObjectREFR*) {}

    PROCESS_TYPE GetProcessLevel();
    void SetCachedWalkSpeed(float afWalkSpeed);
    void SetCachedRunSpeed(float afRunSpeed);
    void SetCachedPerceptionCondition(float afPerceptionCondition);
    void SetCachedMedicineEffectivenessMult(float afMedicineEffectivenessMult);
    void SetCachedSurvivalEffectivenessMult(float afSurvivalEffectivenessMult);
    void SetCachedHealingRate(float afHealingRate);
    void SetLastTimeProcessed(float afTime);
    // The hour the actor was last processed at.
    float GetLastTimeProcessed();
    // The packed date the actor was last processed on.
    uint32_t GetLastDateProcessed();
    void SetLastDateProcessed(uint32_t aiDate);
    bool HasCachedValues();
    // true when every bit of aeWhich is set in the cached flags
    bool GetCachedFlag(uint32_t aeWhich);
    void SetCachedValueDirty(CachedValues::Flags aeWhich);
    void SetCachedValueFlag(CachedValues::Flags aeWhich, bool abSet);
    void SetCachedRadius(float afRadius);
    void SetCachedParalysis(float afParalysis);
    // radius of the actor, recomputed through the owner when stale
    float GetRadius(CachedValuesOwner* apOwner);
    // Width, length, forward length, perception condition and eye level of the actor, recomputed through the owner when
    // stale.
    float GetWidth(CachedValuesOwner* apOwner);
    float GetLength(CachedValuesOwner* apOwner);
    float GetCachedForwardLength(CachedValuesOwner* apOwner);
    float GetPerceptionCondition(CachedValuesOwner* apOwner);
    float GetEyeLevel(CachedValuesOwner* apOwner);
    // paralysis of the actor, recomputed through the owner when stale
    float GetParalysis(CachedValuesOwner* apOwner);
    // The cached aggression (an ACTOR_AGGRESSION), recomputed through the owner when stale.
    int32_t GetCachedActorAggression(CachedValuesOwner* apOwner);
    void SetCachedActorAggression(int32_t aeActorAggression);
    // The cached endurance, recomputed through the owner when stale.
    float GetCachedEndurance(CachedValuesOwner* apOwner);
    // Caches the endurance value.
    void SetCachedEndurance(float afEndurance);
    void SetCachedWidth(float afWidth);
    void SetCachedLength(float afLength);
    void SetCachedForwardLength(float afLength);
    void SetCachedEyeLevel(float afEyeLevel);
    void SetCachedWeaponDPS(float afWeaponDPS);
    void SetCachedActorAssistance(int32_t aeActorAssistance);
    // Caches whether the actor's condition keeps it from running.
    void SetCachedConditionPreventsRun(bool abCanRun);
    double GetWalkSpeed(CachedValuesOwner* apOwner);
    double GetCachedRunSpeed(CachedValuesOwner* apOwner);
    // Returns the cached value, or recomputes it through a virtual on apOwner when it is dirty;
    // Actor::GetDamagePerSecond passes its CachedValuesOwner base.
    double GetDamagePerSecond(CachedValuesOwner* apOwner);
    // As GetDamagePerSecond, through apOwner.
    double GetHealRate(CachedValuesOwner* apOwner);
    // As GetDamagePerSecond, through apOwner.
    int32_t GetAssistance(CachedValuesOwner* apOwner);
    double GetMedicineSkillMult(CachedValuesOwner* apOwner);
    double GetSurvivalSkillMult(CachedValuesOwner* apOwner);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    // 0x4 the package the actor is running
    ActorPackage CurrentPackage;
    // 0x1C the game clock the process last updated at
    float fSystemTimeClock;
    // 0x20 the hour of the day the actor was last processed at
    float fHourLastProcessed;
    // 0x24 the game day the actor was last processed at
    uint32_t iDateLastProcessed;
    // 0x28 how often this process updates its actor
    PROCESS_TYPE eLevel;
    // 0x2C the actor's cached values
    CachedValues* pCachedValues;
};
