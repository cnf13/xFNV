#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/KFModel.h"
#include "Fallout Shared/TESAIForm.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class Actor;
class ActorPathingMessageQueue;
class ActorValueCache;
struct AvoidAreaStruct;
class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class bhkShapePhantom;
class CombatGroup;
class DetectionEvent;
class DetectionState;
class DialogueItem;
class DispositionParamData;
class GuardActorPackageData;
class LipSynchAnim;
class MobileObject;
class MuzzleFlash;
class NiBillboardNode;
class NiBSBoneLODController;
class PathingLocation;
class PathingRequest;
class SpellItem;
class StartCombatStates;
class TESIdleForm;
class TESObjectWEAP;

enum ANIMATION_ACTION : int32_t;
enum DETECTION_LEVEL : int32_t;

// The fade data of a move to a marker: the target and the point the actor fades out at.
// HighProcess::MoveToMarkerWithFade allocates it with ourMalloc and fills the four members.
struct MoveToFadeStruct {
	TESObjectREFR* pRef;
	float x;
	float y;
	float z;
};

// How an acquire package gets an object.
enum ACQUIRE_TYPE : int32_t {
	ACQUIRE_TYPE_NONE = -1,
	ACQUIRE_TYPE_NOOWNED_GROUND = 0,
	ACQUIRE_TYPE_NOOWNED_CONTAINER = 1,
	ACQUIRE_TYPE_BUY_NPC = 2,
	ACQUIRE_TYPE_STEALGROUND = 3,
	ACQUIRE_TYPE_PICKPOCKET = 4,
	ACQUIRE_TYPE_KILL = 5,
	ACQUIRE_TYPE_COUNT = 6,
};

// An object an acquire package wants: where it is, what it is, how it is taken, how many and what it is worth. Declared
// as a class because BaseProcess.h forward declares it as a class.
class ObjectstoAcquire {
public:
	ObjectstoAcquire();

	void* Delete(uint32_t aiFlags = 1);

	TESObjectREFR* pRef;
	TESBoundObject* pObject;
	bool bSteal;
	bool bKill;
	// Compared signed with iNumberItemsActivate.
	int32_t iamount;
	uint32_t iWantValue;
	ExtraDataList* pExtra;
	ACQUIRE_TYPE eType;
};

// What a process knows of an actor it detects or is detected by.
class DetectionState {
public:
	DetectionState();

	// Copies apState's detection; the last seen position and time only when apState has a time.
	void CopyNewData(DetectionState* apState);
	// Saves the detection: the actor's form ID, then the fields from
	// eDetectionLevel to iDetectedEventLevel.
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	void InitLoadGame();

	Actor* pActor;
	DETECTION_LEVEL eDetectionLevel;
	int32_t iLevel;
	NiPoint3 LastPositionDetected;
	CombatTimeStamp fLastTimeDetected;
	bool b360view;
	bool bcombat;
	bool bLineofSight;
	bool bEvaluated;
	int32_t iDetectedEventLevel;
};

// the process of an actor that is near the player and fully simulated:
// detection, dialogue, head tracking, fading and pathing requests
class HighProcess : public MiddleHighProcess {
public:
	HighProcess();
	~HighProcess() override;

	void Update(MobileObject* apRef, float fTimeDelta) override;
	void Update(MobileObject* apRef) override;
	NiPoint3 GetLastPositionDetected(Actor* apActor, int32_t abWhichList) override;
	float GetLastTimeDetected(Actor* apActor, int32_t abWhichList) override;
	bool CheckforNewPackage(Actor* apActor, bool abFlag) override;
	bool SetupSpecialIdle(MobileObject* apMobile, TESIdleForm* apIdle, AnimIdle::PLAY_TYPE_ENUM aeType, bool abForce3rd,
		bool abChooseNewOne, bool abTestConditions) override;
	void SetLastSeenLocation() override;
	bool EvaluateDetection(Actor* apActor) override;
	void EvaluateCombatDetectionLists(Actor* apActor) override;
	bool ShouldRunPlayerDetection(Actor* apActor) override;
	bool ProcessActivateOneHour(Actor* apActor, float afTimeDelta) override;
	bool IsDoingSayTo() override;
	void ProcessSandman(Actor* apActor) override;
	void ProcessCannibal(Actor* apActor) override;
	void SetDoingSayTo(bool abFlag) override;
	MobileObject* GetDialogTarget() override { return pDialogTarget; }
	void ClearDialogTarget() override;
	float GetSocialTalkTimer() override;
	void SetSocialTalkTimer(float afTime) override;
	void RemoveSpokenToActor(Actor* apActor) override;
	bool GetDetectAlert() override;
	void SetDetectAlert(bool abflag) override;
	void RandomlyPlaySpecialIdles(Actor* apActor) override;
	void PostAnimFreeUpSpecialIdle(Actor* apActor) override;
	void ResetSearchTimer() override;
	void ResetSearchChatterTimer() override;
	DetectionState* InsertIntoDetectionList(Actor* apActor, DETECTION_LEVEL aedetect, bool abLineSight, int32_t aiLevel,
		int32_t abWhichList, bool ab360view, bool abcombat) override;
	BSSimpleList<DetectionState*>* GetWhoDetectsMe(Actor* apActor) override;
	DetectionEvent* GetActorsDetectionEvent(Actor* apActor) override;
	void SetActorsDetectionEvent(Actor* apActor, NiPoint3 aLocation, int32_t aiSoundLevel, int32_t aiDetectionEventType,
		TESObjectREFR* apRef) override;
	void RemoveDetectionEvent() override;
	void CheckForExpiredDetectionEvent() override;
	bool GetLipQuequed() override;
	void SetLipQuequed(bool abFlag) override;
	void CreateFollownoEscort(Actor* apActor, bool abflag) override;
	float GetAwarePlayerTimer() override;
	void ModAwarePlayerTimer(float aftime) override;
	void AddTalkedtoActor(Actor* apActor) override;
	bool ComputeCanInitiateDialogueWithNPC(Actor* apActor, Actor* apTalkToActor) override;
	bool ComputeCanRespondToDialogueFromNPC(Actor* apActor, Actor* apInitiatingActor) override;
	bool IsActorInsideAmbushArea(Actor* apActor, Actor* apTarget) override;
	bool CheckIfThereSomeoneTalkWith(Actor* apActor) override;
	bool ComputeAllowSandboxConversation(Actor* apActor, Actor* apTalkToActor) override;
	void ClearGreetingInfoData() override;
	void SetupNewPackage() override;
	DialogueItem* GetSubtitleItem() override;
	void SetSubtitleItem(DialogueItem* apItem) override;
	PathingAvoidNodeArray* CreateAvoidArray(Actor* apActor) override;
	bool IsAvoidAreaInAvoidPathingList(TESObjectREFR* apRef) override;
	void SetAvoidWaitTimer(float aftime) override;
	void RemoveAvoidPathingNode(TESObjectREFR* apRef) override;
	float GetHeadTrackChangeTimer() override;
	void SetHeadTrackChangeTimer(float afTimer) override;
	void ClearLastHeadTrackTarget() override;
	void EndMoveMessage(Actor* apActor) override;
	void ProcessFollow(Actor* apActor, bool abflag, int32_t aemovemode, bool abpursuit) override;
	void ProcessFlee(Actor* apActor) override;
	void ProcessGreet(Actor* apActor, TESTopic* apGreetTopic, bool abForceSub, bool abStop, bool abQue,
		bool abSayCallBack) override;
	void ProcessObserveCombat(Actor* apActor) override;
	void ProcessTravel(Actor* apActor, bool abNewpackage, bool abInterateindex, int32_t aemovemode,
		bool abSecondLoc) override;
	void RemoveDetectionActor(Actor* apActor, int32_t abWhichList) override;
	void SetDetectionTimer(float aftime) override;
	bool GetLineSightonActor(Actor* apActor, Actor* apTarget, int32_t abWhichList, bool ab360) override;
	bool Get360LineSightonActor(Actor* apActor, Actor* apTarget, int32_t abWhichList) override;
	int32_t GetDetectionActor(Actor* apActor, int32_t abWhichList) override;
	bool GetTalkingtoPC() override;
	void SetTalkingtoPC(bool abflag) override;
	void UpdateFollowers(Actor* apActor, float afDelta) override;
	int32_t GetEmotionsDispostion(Actor* apActor, TESObjectREFR* pRef) override;
	DIALOGUE_EMOTION GetLastSpeakingEmotion() override;
	void SetLastSpeakingEmotion(DIALOGUE_EMOTION aeEmotion) override;
	void RunDetection(Actor* apActor, float fnumber) override;
	float GetDetectionTimer() override;
	void ReduceDetectionTimer() override;
	void SetBreathTimer(float afTimer) override;
	float GetBreathTimer() override;
	bool GetGreetingFlag() override;
	void SetGreetingFlag(bool abFlag) override;
	float GetSoundDelay() override;
	void SetSoundDelay(float afDelay) override;
	bool GetFinishingCombatPackage() override;
	void SetFinishingCombatPackage(bool abFlag) override;
	ItemChange* CreateWeaponAcquireList(Actor* apActor, PACKAGE_OBJECT_TYPE aeType) override;
	void ClearAcquireList() override;
	float GetIdleTimer() override;
	void SetIdleTimer(float aftime) override;
	bool EnterCombat(Actor* apActor, Actor* apTarget, bool abCheckForAmbush, bool absendalarm, CombatGroup* apGroup,
		bool abfleeing, bool abForceFlee, bool abIsAggressor, char acTeam, bool bforcecombat,
		bool bIgnoreDefensivePackage, bool abCastSpell, TESPackage* apPackage) override;
	void GuardCallforHelp(Actor* apActor, Actor* apCriminal) override;
	void CallforHelp(Actor* apActor, Actor* pCriminal, Crime* pcrime) override;
	void SetNumberGuardsArresting(int32_t ivalue) override;
	int32_t CalculateMoveMode(Actor* apActor, float afdistance, float afshortdist, float aflongdist, bool balarmcombat,
		bool abpuretravel) override;
	void SetActorsAnimation(Actor* apActor, int32_t emovemode, bool abCheckAlwaysRun) override;
	void FinishDying(Actor* apActor) override;
	bool ProcessStandOff(Actor*, float, int32_t) override { return false; }
	bool GetWaitingforLipFile() override;
	void SetWaitingforLipFile(bool abflag) override;
	void SetLipFileFailed(bool abflag) override;
	bool GetLipFileFailed() override;
	LipSynchAnim* GetLipAnim() override;
	void SetLipAnim(LipSynchAnim* apAnim) override;
	void SetPlantedExplosive(bool abFlag) override;
	bool GetPlantedExplosive() override;
	void FreeUpSpecialIdle(Actor* apActor) override;
	int32_t GetActorValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor) override;
	float GetActorFloatValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor) override;
	void TempModActorValue(Actor* apActor, ActorValue::Index aiWhich, float afValue) override;
	void TempModActorValue(Actor* apActor, ActorValue::Index aiWhich, int32_t aiValue) override;
	void DamageModActorValue(Actor* apActor, ActorValue::Index aiWhich, float afValue) override;
	void DamageModActorValue(Actor* apActor, ActorValue::Index aiWhich, int32_t aiValue) override;
	void SetCachedActorValueOutOfDate(ActorValue::Index aiWhich) override;
	ANIMATION_ACTION GetAnimAction() override;
	BSAnimGroupSequence* GetAnimActionAnimSeq() override;
	void SetAnimAction(ANIMATION_ACTION aeAction, BSAnimGroupSequence* apAnimSeq) override;
	bool GetAutomaticFireAtLeastOnce() override;
	void SetAutomaticFireAtLeastOnce(bool abVal) override;
	bool CanAttack() override;
	bool CanForceGreet(Actor* apActor) override;
	void AttackCallback(int32_t aiNumAttacks) override;
	void SaveWeaponLastPos(NiPoint3 aPos) override;
	const NiPoint3* GetWeaponLastPos() override;
	void Update3dModel(Actor* apActor) override;
	TESObjectREFR* GetLastGreeted() override;
	void SetLastGreeted(TESObjectREFR* apRef) override;
	// KEEP: The exe assigns into the hidden return buffer; src cannot reach it without constructing a local handle.
	BSSoundHandle GetSoundHandle(ACTOR_SOUND_HANDLE aeType) override;
	void SetSoundHandle(ACTOR_SOUND_HANDLE aeType, BSSoundHandle aSound) override;
	void StopSoundHandle(ACTOR_SOUND_HANDLE aeType) override;
	void StartTorchSound(Actor* apActor) override;
	bool GetMovementStoped() override;
	void ClearMovementStoped() override;
	void SetDetectionModifier(float afvalue) override;
	void SetDetectionModifierTimer() override;
	float GetDetectionModifier() override;
	float GetGreetingTimer() override;
	void SetGreetingTimer(float ftime) override;
	bool IsTalking(MobileObject* apActor) override;
	bool ContinuingPackageforPC() override;
	void SetContinuingPackage(bool abflag) override;
	DetectionState* GetDetectionState(Actor* apActor, int32_t abWhichList) override;
	float GetLightLevelTimer() override;
	void SetLightLevelTimer(float afTimer) override;
	void SetPackageEndTimerValue(float fTime) override;
	float GetPackageEndTimerValue() override;
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
	int32_t GetNumberGuardsArresting() override;
	void ModNumberGuardsArresting(int32_t ivalue) override;
	BSSimpleList<SpellItem*>* GetLeveledSpells(Actor* apActor, bool abIncludeDiseases) override;
	void SetHasHealingSpells(int32_t aiHasHealingSpell) override;
	int32_t GetHasHealingSpells() override;
	void SetHasHealingPotions(int32_t aiHasHealingPotion) override;
	int32_t GetHasHealingPotions() override;
	void SetNeedTalkPlayer(bool abFlag) override;
	bool GetNeedTalkPlayer() override;
	BSSimpleList<Actor*>* GetAggroRadiusActorList() override;
	void NotifyGuardAboutActivation(Actor* apActor, TESObjectREFR* apActionRef, TESObjectREFR* apActivated) override;
	FADE_STATE GetFadeState() override;
	void AddPostAnimationAction(BaseProcess::POSTANIM_ACTION aeAction) override;
	int32_t GetPostAnimationActions() override { return ePostAnimActon; }
	void ClearPostAnimationActions() override;
	void RemovePostAnimationAction(BaseProcess::POSTANIM_ACTION aeAction) override;
	void SetDefaultHeadTrackTarget(TESObjectREFR* apRef) override;
	void SetActionHeadTrackTarget(TESObjectREFR* apRef) override;
	void SetScriptHeadTrackTarget(TESObjectREFR* apRef) override;
	void SetCombatHeadTrackTarget(TESObjectREFR* apRef) override;
	void SetDialogHeadTrackTarget(TESObjectREFR* apRef) override;
	void SetUseWeaponHeadTrackTarget(TESObjectREFR* apRef) override;
	void SetHeadTrackTarget(HEAD_TRACK_TYPE aType, TESObjectREFR* apRef) override;
	void ClearDefaultHeadTrackTarget() override;
	void ClearActionHeadTrackTarget(bool abHoldDefault) override;
	void ClearScriptHeadTrackTarget(bool abHoldDefault) override;
	void ClearCombatHeadTrackTarget() override;
	void ClearDialogHeadTrackTarget(bool abHoldDefault) override;
	void ClearUseWeaponHeadTrackTarget(bool abHoldDefault) override;
	void ModDetectionModifierTimer() override;
	float GetDetectionModifierTimer() override;
	void ClearAllHeadTrackTargets() override;
	void RemoveHeadTrackTarget(TESObjectREFR* apRef) override;
	bool CanSetDefaultHeadTrackTarget() override;
	bool CanSetActionHeadTrackTarget() override;
	bool CanSetCombatHeadTrackTarget() override;
	TESObjectREFR* GetCurrentHeadTrackTarget(HEAD_TRACK_TYPE aType) override;
	TESObjectREFR* GetCurrentHeadTrackTarget() override;
	HEAD_TRACK_TYPE GetCurrentHeadTrackType() override;
	const char* GetCurrentHeadTrackTypeString() override;
	void SetForceRotate(bool abForceRotate) override;
	bool GetForceRotate() override;
	TESObjectREFR* GetLastHeadTrackTarget() override;
	void CreateMuzzleFlash(Projectile* apProjectile, TESObjectREFR* apParentRef) override;
	MuzzleFlash* GetCurrentMuzzleFlash() override;
	void ClearMuzzleFlash() override;
	void SetCurrentlyReanimating(bool abCurrentlyReanimating) override;
	bool GetCurrentlyReanimating() override;
	void UpdateRadiation(Actor* apActor, float afDelta) override;
	void FinishSetupSpecialIdle(Actor* apActor) override;
	TESIdleForm* GetCurrentProcessIdle() override;
	void SetCurrentProcessIdle(TESIdleForm* apIdle) override;
	void SetCurrentProcessAnimIdleLoaded(AnimIdle* apAnimIdle, KFModel* apAnimIdleKF) override;
	AnimIdle* GetCurrentProcessAnimIdle() override;
	KFModel* GetCurrentProcessAnimIdleKF() override;
	float GetActorLightLevel() override;
	void SetActorLightLevel(float aflight) override;
	float GetRadiationDelta() override;
	void SetPathLookAtTarget(TESObjectREFR* apRef) override;
	void ClearPathLookTarget(TESObjectREFR* apRef) override;
	void ProcessEscort(Actor* apActor) override;
	void ProcessSleep(Actor* apActor) override;
	void ProcessEat(Actor* apActor, TESObjectREFR* apLocRef) override;
	void ProcessAlarm(Actor* apActor) override;
	bool ProcessActivate(Actor* apActor, bool abInterateindex) override;
	void ProcessAccompany(Actor* apActor) override;
	void ProcessAcquire(Actor* apActor) override;
	void ProcessAmbushWait(Actor* apActor) override;
	bool ProcessUseItemAt(Actor* apActor) override;
	bool ProcessUseWeapon(Actor* apActor) override;
	void ProcessFleeNonCombat(Actor* apActor) override;
	void ProcessAvoidArea(Actor* apActor) override;
	void ProcessGuard(Actor* apActor) override;
	void ProcessSummonCreatureDefend(Actor* apActor) override;
	bool CheckIfHasObject(Actor* apActor, int32_t ainumb) override;
	void EvaluateOrderAcquireList(Actor* apActor) override;
	void SetAnimation(Animation* apAnim) override;
	void SetAnimationActiveFlag(bool abflag) override;
	void ProcessBuyObject(Actor* apActor) override;
	void ProcessSteal(Actor* apActor) override;
	void ProcessPickPocket(Actor* apActor) override;
	void ProcessDialogue(MobileObject* apActor) override;
	bool ProcessDialogueActivate(Actor* apActor) override;
	void ProcessWander(Actor* apActor, const NiPoint3* apCenter, float afRadius) override;
	bool ProcessSandBoxDialogue(Actor* apActor, TESObjectREFR* aptalkTarget) override;
	void ProcessUseIdleMarker(Actor* apActor, TESObjectREFR* apRef) override;
	// the list of actors to avoid
	virtual BSSimpleList<Actor*>* GetAvoidActorList();

	void FadeOut(Actor* apActor, TESObjectREFR* apRef, bool abTeleport);
	bool ProcessFollowOneHour(Actor* apActor, float afTimeDelta, bool abInterateIndex);
	// Fades apActor out and moves it to apTarget offset by
	// (afX, afY, afZ).
	void FadeOutAndMove(Actor* apActor, TESObjectREFR* apTarget, float afX, float afY, float afZ);
	// Walks apActor back to the player and takes back what the player stole from it; the player gets attacked when
	// the take-back timer runs out.
	void ProcessTakeBackItem(Actor* apActor);
	// Plays a special idle of apActor's toward apRef's object apObject.
	bool FindSpecialIdletoPlay(Actor* apActor, TESBoundObject* apObject, TESObjectREFR* apRef);
	bool CheckTargetCombatDetectionState(Actor* apActor, Actor* apTarget);
	// Whether apActor's combat detection runs against apTarget.
	bool ShouldRunCombatDetection(Actor* apActor, Actor* apTarget);
	bool ShouldRunCombatDetectionEventCheck(Actor* apActor, Actor* apTarget);
	// The hourly (low-detail) acquire and patrol updates: they run LowProcess's package step; always true.
	bool ProcessAcquireOneHour(Actor* apActor, float afTimeDelta);
	bool ProcessPatrolOneHour(Actor* apActor, float afTimeDelta);
	bhkShapePhantom* QDoorDetectionPhantom();
	// true while the door detection phantom exists
	bool CheckPhantom();
	// removes the door detection phantom from the world and drops it
	void RemovePhantom();
	void SetSkippedLastUpdate(bool abVal);
	bool GetSkippedLastUpdate();
	void UpdateHeadTrackTargets();
	// Refreshes the active head-tracking target.
	void OnNewHeadTrackTarget();
	void InitBoneLOD(Actor* apActor);
	// Creates the door detection phantom for apActor.
	void SetupPhantom(Actor* apActor);
	float GetActorHeight();
	void SetActorHeight(float afHeight);
	float GetFadeAlpha();
	void SetMaxAlpha(float afAlpha);
	// finishes the fade-in or teleport fade-in of the actor at once
	void SkipFadeIn(Actor* apActor);
	void FadeIn(Actor* apActor, bool abTeleport);
	// Moves the detection thread's queued states into the detected and detecting lists, adding a state for each
	// actor not in them yet, and flags apActor's combat controller when any state was queued.
	void ReconcileDetectionData(Actor* apActor);

	TESObjectREFR* GetPathLookAtTarget() { return pPathLookAtTarget; }

	NiBSBoneLODController* GetBoneLOD() { return pBoneLOD; }
	float GetVoiceTimer() { return fVoiceTimer; }
	float QMaxAlpha() const;
	float GetEvalTimer() const;
	float GetCurrentRadiation() const;
	float GetRadReEvaluationTimer() const;
	void SetVoiceTimer(float afTime);
	void SetCurrentRadiation(float a2);
	void ProcessAccompanyOneHour(Actor* apActor, float fTimeDelta);
	void FadeAndDelete(Actor* apActor);
	void FadeAndDisable(Actor* apActor);
	bool DoesFadeProhibitAction();
	void HighCopy(HighProcess* apProcess);
	void ClearShootingAction(Actor* a1);
	void ProcessSearch(Actor* apActor);
	void SayGoodBye(Actor* apActor);
	void ProcessGuardStartTravelAndReset(Actor* a2, GuardActorPackageData* a3);
	bool ProcessEatOneHour(Actor* apActor, float afTimeDelta);
	bool FadeUpdate(Actor* apActor);
	void ProcessAlertSearch(Actor* pActor);
	bool ProcessSleepOneHour(Actor* apActor, float afTimeDelta);
	void ProcessGuardIdle(Actor* a1, GuardActorPackageData* a2, bool a3);
	void SayGuardWarning(Actor* a1, GuardActorPackageData* a2);
	BSSimpleList<ObjectstoAcquire*>* SortObjectsListByType();
	BSSimpleList<ObjectstoAcquire*>* SortObjectsListByValue();
	void ProcessWarning(Actor* apActor);
	void ProcessCreateFollow(Actor* apActor);
	void ProcessSurface(Actor* apActor);
	bool ShouldWaitForEscortTarget(Actor* apActor, Actor* apEscortedActor, PathingLocation& goalLoc,
		bool* abSlowDownOut);
	void DetectDoorsForPathing(Actor* apActor);
	void EvaluateBoneLod(MobileObject* apRef);
	void ProcessBackUp(Actor* apActor);
	void ProcessSurfaceOneHour(Actor* apActor);
	bool ProcessWanderOneHour(Actor* apActor, float afTimeDelta);
	bool IsGroupAlreadyInList(CombatGroup* pgroup);
	bool AimAtTarget(Actor* apActor, TESObjectREFR* apTarget, bool abHighArc);
	void CleanUpAfterProcessUseWeapon(Actor* apActor);
	bool ObserveCombat(Actor* apActor, Actor* apTarget);
	void ProcessGuardWaitToAttack(Actor* apActor, GuardActorPackageData* apData);
	void FireAtObject(Actor* apActor, TESObjectWEAP* apWeapon);
	void FreeAvoidNodes();
	// The escort package's hourly processing, called from HighProcess's update. Not
	// MiddleHighProcess::ProcessEscortOneHour(Actor*): HighProcess has its own, with the time delta.
	bool ProcessEscortOneHour(Actor* apActor, float afTimeDelta);
	// Adds an avoid area at aPoint around apRef with its radiation level and expiry, and starts the avoid package. Not
	// an override of BaseProcess's virtual: this member has the extra parameter afRadiationLevel.
	void AddAvoidPathingArea(Actor* apActor, NiPoint3 aPoint, float afRadius, float afTimeToExpire,
		float afRadiationLevel, TESObjectREFR* apRef, TESObjectSTAT* apMarker);
	void DetectActor(TESObjectREFR* pref, Actor* apActor, bool& bspectatorstart, DispositionParamData* pdispdata,
		ACTOR_AGGRESSION aeAggressionvalue, float afConfidence, float afCombatStrength);
	void SaveGame_00928880(BGSSaveFormBuffer* a2);

	// Stub: always reports that no point was found.
	static bool ComputeRandomPointOnPath(Actor* apActor, NiPoint3 aStartPoint, float afRadius, NiPoint3& aOutCoord);
	static void ClearWanderStarted();
	static NiPoint3* ComputeRandomPoint(NiPoint3* apResult, NiPoint3 aStartPoint, float afMaxRadius, float afMinRadius,
		bool bRandZ);
	static bool FindDoorsToSendYellAlarm(TESObjectREFR* apRef, void* aiFilterData);

	// 0x25C actors the process detected
	BSSimpleList<DetectionState*>* pDetectedActorList;
	// 0x260 actors that detect the actor
	BSSimpleList<DetectionState*>* pActorsWhoDetectMeList;
	// 0x264 actors the actor spoke to recently
	BSSimpleList<Actor*>* pLastSpokeToList;
	// 0x268 detections queued by the detection thread
	BSSimpleList<DetectionState*>* pThreadDetectList;
	// 0x26C detecting actors queued by the detection thread
	BSSimpleList<DetectionState*>* pTempActorsWhoDetectMeList;
	// 0x270 detection has to run
	uint8_t bEvaluateDetection;
	uint8_t padding271[3];
	// 0x274 actors that are aggressive to the actor
	BSSimpleList<StartCombatStates*> AggroList;
	// 0x27C groups the actor helps
	BSSimpleList<StartCombatStates*> GroupsToHelpList;
	// 0x284 targets waiting to be added
	BSSimpleList<void*> TargetToAddList;
	// Actors watching the actor.
	BSSimpleList<StartCombatStates*> SpectatorList;
	// 0x294 time until the detection list is rebuilt
	float fDetectListTimer;
	// 0x298 time until the actor talks to itself again
	float fIdleChatterTimer;
	// 0x29C the actor says goodbye to the player
	bool bSayGoodByePlayer;
	// 0x29D the greeting is a say-to
	bool bProcessGreetSayTo;
	uint8_t padding29E[2];
	// 0x2A0 time until the list of spoken-to actors is cleared
	float fClearTalkToListTimer;
	// The reference detected last; InitLoadGame resolves it with a dynamic cast to TESObjectREFR.
	TESObjectREFR* plastDetected;
	// 0x2A8 delay before the actor talks
	float fTalkTimer;
	// 0x2AC the target of the look at the path points to
	TESObjectREFR* pPathLookAtTarget;
	// 0x2B0 the highest alpha the actor fades to
	float fMaxAlpha;
	// 0x2B4 time until the package is evaluated
	float fPackageEvalTimer;
	// 0x2B8 time until the actor uses an item
	float fUseItemTimer;
	// 0x2BC time the actor holds an attack
	float fHoldAttackTimer;
	// 0x2C0 shots fired in the current burst
	int16_t sShotsFired;
	// 0x2C2 shots to fire in the current burst
	int16_t sShotsToFire;
	// 0x2C4 bursts fired
	int16_t sBurstsFired;
	// 0x2C6 the dead actor is checked for talking
	bool bCheckDeadTalk;
	// 0x2C7 the last update was skipped
	bool bSkippedUpdate;
	// 0x2C8 time until conversations between actors are checked
	float fCheckToTalkTimer;
	NiNode* pNode;
	// 0x2D0 delay timer
	float fDelayTimer;
	// 0x2D4 distance the actor moved
	float fDistanceMoved;
	// 0x2D8 time the actor turns for
	float fTurnTime;
	// 0x2DC direction of the last turn
	bool cLastTurnDir;
	uint8_t padding2DD[3];
	// 0x2E0 time until the acquire is evaluated
	float fEvaluateAcquireTimer;
	// 0x2E4 the bone level of detail controller
	NiBSBoneLODController* pBoneLOD;
	// 0x2E8 the bone level of detail used last
	int32_t iLastBoneLOD;
	// 0x2EC the animation action
	int16_t sAnimAction;
	uint8_t padding2EE[2];
	// 0x2F0 the animation sequence of the action
	BSAnimGroupSequence* pAnimSeq;
	// 0x2F4 an automatic weapon fires at least once
	bool bAutomaticFireAtLeastOne;
	uint8_t padding2F5[3];
	// 0x2F8 time until detection runs
	float fDetectionTimer;
	// 0x2FC the level of the last detection
	int16_t iLastDetection;
	uint8_t padding2FE[2];
	// 0x300 where the weapon was last
	NiPoint3 WeaponLastPos;
	// 0x30C the reference the actor greets
	TESObjectREFR* pGreetActor;
	// 0x310 delay of the sound
	float fSoundDelay;
	// 0x314 the two sound handles of the actor
	BSSoundHandle SoundHandle[2];
	// 0x32C the greeting flag
	bool bGreetingFlag;
	uint8_t padding32D[3];
	// 0x330 time until the actor greets
	float fGreetingTimer;
	// 0x334 time until the actor plays an idle
	float fIdleTimer;
	// 0x338 time until the greeting is detected
	float fDetectGreetTimer;
	// 0x33C time until the actor breathes
	float fBreathTimer;
	// 0x340 the head tracks a target
	bool bHeadTrack;
	uint8_t padding341[3];
	// 0x344 time until the voice plays
	float fVoiceTimer;
	// 0x348 a lip file is queued
	bool bLipQuequed;
	// 0x349 the weapon was drawn on alert
	bool bWeaponAlertDrawn;
	uint8_t padding34A[2];
	// 0x34C time the actor is aware of the player
	float fAwarePlayerTimer;
	// 0x350 the idle the process was told to play
	TESIdleForm* pIdleToPlay;
	// 0x354 animations the idle plays
	NiPointer<AnimIdle> spAnimIdleToPlay[2];
	// The animation files of the idle.
	NiPointer<KFModel> spAnimIdleKF[2];
	// 0x364 the actor talks with the player
	bool bDialoguewithPlayer;
	uint8_t padding365[3];
	// 0x368 the greeting topic
	DialogueItem* pGreetTopic;
	// 0x36C the emotion the actor spoke with last
	DIALOGUE_EMOTION eLastSpeakingEmotion;
	// 0x370 the actor of the dialogue
	MobileObject* pDialogTarget;
	// 0x374 the package continues for the player
	bool bContinuingPackageforPC;
	// 0x375 the animation is active
	bool bActivateAnim;
	uint8_t padding376[2];
	// 0x378 when the package a script started ends
	float fScriptPackageEndTime;
	// 0x37C alpha of the health bar
	float fHealthBarAlphaValue;
	// 0x380 node of the health bar
	NiPointer<NiBillboardNode> spHealthBarNode;
	// 0x384 health of the actor as a fraction
	float fActorHealthPercentage;
	// 0x388 emittance of the health bar
	float fHealthBarEmittanceValue;
	// 0x38C actors inside the aggro radius
	BSSimpleList<Actor*> AggroRadiusList;
	// 0x394 actors the actor flees from
	BSSimpleList<Actor*> AvoidActorList;
	// 0x39C number of guards pursuing the actor
	int32_t iNumberGuardsPersuing;
	// 0x3A0 the actor stops
	bool bStop;
	uint8_t padding3A1[3];
	// 0x3A4 time until the armor is equipped again
	float fReEquipArmorTimer;
	// 0x3A8 the armor was taken off to swim
	bool bUnequippedArmorToSwim;
	uint8_t padding3A9[3];
	// 0x3AC the actor has a healing spell
	int32_t iHasHealingSpell;
	// 0x3B0 the actor has a healing potion
	int32_t iHasHealingPotion;
	// 0x3B4 the leveled spells of the actor
	BSSimpleList<SpellItem*>* pLeveledSpellList;
	// 0x3B8 direction of the last turn
	int8_t cLastTurn;
	// 0x3B9 the actor is being reanimated
	bool bCurrentlyReanimating;
	uint8_t padding3BA[2];
	// 0x3BC detection modifier
	float fDetectionModifer;
	// 0x3C0 time the detection modifier applies
	float fDetectionModifierTimer;
	// 0x3C4 light level at the actor
	float fLightLevel;
	// 0x3C8 time until the light level is updated
	float fLightLevelTimer;
	// 0x3CC the lip sync animation
	LipSynchAnim* pLipSynicAnim;
	// 0x3D0 a lip file is loading
	bool bWaitingForLipFile;
	// 0x3D1 loading the lip file failed
	bool bLipFileFailed;
	uint8_t padding3D2[2];
	// 0x3D4 the muzzle flash of the weapon
	MuzzleFlash* pCurrentMuzzleFlash;
	// 0x3D8 detection counter
	uint32_t iDetectionCounter;
	// 0x3DC the detection event the actor made
	DetectionEvent* pActorsGeneratedDetectionEvent;
	// 0x3E0 the combat package is finishing
	bool bFinishingCombatPackage;
	uint8_t padding3E1[3];
	// 0x3E4 the topic of a queued say-to
	DialogueItem* m_pSayToDialogueTopic;
	// 0x3E8 the fade state
	FADE_STATE eFadeState;
	// 0x3EC alpha of the fade
	float fFadeAlpha;
	// 0x3F0 reference of the teleport fade
	TESObjectREFR* pTeleportFadeRef;
	// 0x3F4 the move-to-marker fade data
	MoveToFadeStruct* pMoveToFadeStruct;
	// 0x3F8 the targets the head tracks; the highest slot with a flag set
	// wins
	TESObjectREFR* HeadTrackingTargets[6];
	// 0x410 which head tracking targets are set
	bool HeadTrackingTargetFlags[6];
	uint8_t padding416[2];
	// 0x418 time the head holds its target
	float fHeadTrackTargetTimer;
	// 0x41C the last target
	TESObjectREFR* pLastTarget;
	// 0x420 the actor is forced to rotate
	bool bForceRotate;
	uint8_t padding421[3];
	// 0x424 flags of an idle queued after the animation
	int32_t ePostAnimActon;
	// 0x428 cached actor values
	ActorValueCache* pActorValueCache;
	// 0x42C height of the actor
	float fCachedActorHeight;
	// 0x430 the default idle action
	AnimIdle::PLAY_TYPE_ENUM eSpecialIdleType;
	// 0x434 the phantom that detects doors around the actor
	NiPointer<bhkShapePhantom> spShapePhantom;
	// 0x438 time until doors are detected for pathing
	float fDetectDoorForPathingTimer;
	// 0x43C time until radiation is evaluated
	float fRadiationTimer;
	// 0x440 the highest radiation the actor stood in
	float fHighestRadiation;
	// 0x444 the actor has a live grenade or planted explosive
	bool bPlantedExplosive;
	// 0x445 the actor has to talk to the player after getting up
	bool bNeedTalkPlayer;
	uint8_t padding446[2];
	// 0x448 time until the actor takes something back
	float fTakeBackTimer;
	// 0x44C areas the actor avoids
	BSSimpleList<AvoidAreaStruct*>* pListOfAvoidAreas;
	// 0x450 time the actor waits to avoid
	float fAvoidWaitTimer;
	// 0x454 the topic of the subtitle
	DialogueItem* pSubtitleVoice;
	// 0x458 the actor hides
	bool bHiding;
	// 0x459 the actor does a say-to
	bool bIsDoingSayTo;
	uint8_t padding45A[2];
	// The request that moves the actor to its goal.
	NiPointer<PathingRequest> spMeToGoalRequest;
	// 0x460 the request that moves the escorted actor to its goal
	NiPointer<PathingRequest> spEscortedToGoalRequest;
	// Message queue of the first request.
	NiPointer<ActorPathingMessageQueue> spMeToGoalMessageQueue;
	// 0x468 message queue of the second request
	NiPointer<ActorPathingMessageQueue> spEscortedToGoalMessageQueue;

	static inline uint8_t bWanderStarted;
	static inline bool bWantsToSit;
};

// Set while a model replacement should also update the biped's skins (the biped animation code's global).
inline bool bBipedAnimUpdateSkins;

// The game settings HighProcess.cpp defines.
extern SettingT<INISettingCollection> fHealthBarEmittanceFadeTime;
extern SettingT<INISettingCollection> fHealthBarHeight;
extern SettingT<INISettingCollection> fHealthBarWidth;
extern SettingT<INISettingCollection> fHealthBarSpeed;
extern SettingT<INISettingCollection> fHealthBarFadeOutSpeed;
extern SettingT<INISettingCollection> fINIDetectDoorsForPathingTime;
extern SettingT<INISettingCollection> fHealthBarEmittanceTime;
inline int32_t nFurnitureListDirtied;
inline char HighProcess_bEating;
inline int32_t iStaticDetectionCounter;
extern BSPerformanceTimer HighUpdateTimer;

// The references collected while detection runs.
extern BSSimpleList<TESObjectREFR*> TempRefList;

extern BSSimpleList<TESObjectREFR*> DoorRefListCrime;
