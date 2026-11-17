#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class AlchemyItem;
class Animation;
class BGSMusicType;
class BGSNote;
class BGSPerkEntry;
class BGSQuestObjective;
class bhkMouseSpringAction;
class BipedAnim;
class CameraCaster;
struct CasinoData;
class CombatGroup;
class DialoguePackage;
class HitCaster;
class ImageSpaceModifierInstanceDOF;
class ImageSpaceModifierInstanceDRB;
class MagicShaderHitEffect;
class MediaLocationController;
class Script;
struct SCRIPT_PARAMETER;
class ScriptLocals;
class TESAmmo;
class TESCaravanCard;
class TESClass;
class TESObjectBOOK;
class TESObjectCELL;
class TESObjectDOOR;
class TESObjectWEAP;
class TESQuest;
class TESQuestStageItem;
class TESQuestTarget;
class TESRegion;
struct TESRegionSound;
class TESReputation;
class TESTopic;
class TESWaterForm;
class TESWorldSpace;

enum PLAYER_ACTION : int32_t;

// Flags apObject and the nodes under it as the player's bones.
void MarkPlayerBones(NiAVObject* apObject);
bool Cmd_DisableHardcoreTracking(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool au_re_Cmd_IsAlwaysHardcore(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
void SettingsMenu_ToggleHardcore(StartMenu::UserOption* a1);
bool Cmd_GetOwnerLastTarget_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_ShowRecipeMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_GetCasinoWinningsLevel(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_SetCasinoWinningsLevel(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
bool Cmd_UnlockChallenge(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset);
bool Cmd_RemoveRecurringFromChallenge(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset);
// An IterateInventory callback: reads its first argument as an ItemChange and passes its second to
// BSStringT<char>::operator+=.
uint8_t apCallback(ItemChange* apItem, BSStringT<char>* apData);
// apThis is the TESAnimGroup returned by BSAnimGroupSequence::GetAnimGroup.
bool TESAnimGroupIsJumpingLandAnim(TESAnimGroup* apThis, [[maybe_unused]] void* apEdx);
void ShakeData();
void FreeCameraShake();
void DeleteCharacterProgressionData(CharacterProgression* apThis, [[maybe_unused]] void* apEdx);
void set_11E07AB();
void SetLastAntiCheatTime();

namespace PackageNS {
enum CRIME_TYPE : int32_t;
}

// The player's kill camera setting.
enum KILL_CAM_TYPE : int32_t {
	KILL_CAM_NONE = 0,
	KILL_CAM_PLAYER = 1,
	KILL_CAM_VATS = 2,
};

// A perk the player has and its rank.
struct PerkRankData {
	BGSPerk* pPerk;
	uint8_t cCurrentRank;
	uint8_t padding05[3];
};

// Moves a value toward an end value over a total time along a 100-step table of percentages.
class NonLinearInterpolator {
public:
	NonLinearInterpolator();

	bool Interpolate(float afDeltaTime);
	bool GetFinished();
	bool Setup(float afStart, float afEnd, float afTime);
	float GetCurrent();

	float fPercentages[100];
	float fCurrent;
	float fEnd;
	float fRemainingTime;
	float fTotalTime;
};

// An actor the compass tracks for the player.
struct CompassTarget {
	CompassTarget();

	// The list sort's compare of two compass targets, nearest the player first.
	static int32_t CompareForSorting(CompassTarget* apTarget1, CompassTarget* apTarget2);

	Actor* pTarget;
	bool bIsHostile;
	bool bIsUndetected;
	uint8_t padding06[2];
};

// An action the player is performing, kept in the player's action list.
struct PlayerActionObject {
	PlayerActionObject();

	PLAYER_ACTION eAction;
	float ftimer;
	TESObjectREFR* prefObj;
};

// An audio marker reference near the player and its marker data.
struct AudioMarkerInfo {
	AudioMarkerInfo();
	AudioMarkerInfo(TESObjectREFR* apRef);

	TESObjectREFR* pMarkerRef;
	AudioMarker* pMarkerData;
};

// A queued request to move the player, filled by RequestPositionPlayer and consumed by HandlePositionPlayerRequest.
struct PLAYER_TARGET_LOC {
	using ArrivalFunc = void(uintptr_t);

	PLAYER_TARGET_LOC();

	TESWorldSpace* pWorld;
	TESObjectCELL* pInterior;
	NiPoint3 Location;
	NiPoint3 Angle;
	bool bResetWeather;
	uint8_t padding21[3];
	ArrivalFunc* pfnArrivalFunc;
	uintptr_t iArrivalFuncData;
	TESObjectREFR* pFurnitureRef;
	TESObjectREFR* pFastTravelMarker;
};

// One achievement's progress: unlocked, the progress value and whether it is tracked. The PlayerCharacter constructor
// clears it and sets bTracking from the achievement table.
struct AchievementInstance {
	bool bUnlocked;
	uint32_t iCurrentValue;
	bool bTracking;
};

// What the player is doing, for the reactions of the actors around (GetPlayerAction).
enum PLAYER_ACTION : int32_t {
	PLAYER_ACTION_NONE = 0,
	PLAYER_ACTION_SWING_MELEE_WEAPON = 1,
	PLAYER_ACTION_THROW_GRANADE = 2,
	PLAYER_ACTION_FIRE_WEAPON = 3,
	PLAYER_ACTION_LAY_MINE = 4,
	PLAYER_ACTION_Z_KEY_OBJECT = 5,
	PLAYER_ACTION_JUMPING = 6,
	PLAYER_ACTION_KNOCKING_OVER_OBJECTS = 7,
	PLAYER_ACTION_STAND_ON_TABLE_CHAIR = 8,
	PLAYER_ACTION_IRON_SITES = 9,
	PLAYER_ACTION_DESTROY_OBJECT = 10,
	PLAYER_ACTION_LOCKED_OBJECT = 11,
	PLAYER_ACTION_PICKPOCKET = 12,
	PLAYER_ACTION_TOTAL = 13,
};

// The player's actor. Only some fields are named; the paddings between them keep those fields at their engine offsets.
class PlayerCharacter : public Character {
public:
	// The modifier groups GetModifier and AlterModifier act on.
	enum ModifierGroup : int32_t {
		TEMPORARY = 0,
		SCRIPTS = 1,
		DAMAGE = 2,
	};

	// The events ExportProgressData records (ProgressDataEventNames' index).
	enum PROGRESS_EVENT : int32_t {
		LEVEL_UP = 0,
		DEATH = 1,
		EVENT_COUNT = 2,
	};

	// What the player can do with a terminal, as returned by GetTerminalAccess.
	enum TerminalAccessType : int32_t {
		TAT_HAS_NOTE = 0x0,
		TAT_HACKED = 0x1,
		TAT_LOCKED = 0x2,
		TAT_HACK_ELIGIBLE = 0x3,
		TAT_HACK_INELIGIBLE = 0x4,
	};

	// The control bits a script can disable for the player (GetControlsDisabled's aeDPC).
	enum DISABLE_PLAYER_CONTROLS : uint32_t {
		DPC_NONE = 0,
		DPC_MOVEMENT = 1,
		DPC_LOOKING = 2,
		DPC_PIPBOY = 4,
		DPC_FIGHTING = 8,
		DPC_POV_SWITCH = 16,
		DPC_ROLL_OVER = 32,
		DPC_SNEAKING = 64,
		DPC_ALL = 255,
	};

	// What the player holds with the grab key (eGrabType).
	enum GrabbingType : uint32_t {
		GRAB_NONE = 0,
		GRAB_NORMAL = 1,
		GRAB_INVENTORY_DROP = 2,
		GRAB_TELEKINESIS = 3,
	};

	// A map marker reference near the player and its marker data.
	struct MapMarkerInfo {
		MapMarkerData* pMarkerData;
		TESObjectREFR* pMarkerRef;
	};

	using Actor::SetMoveMode;

	PlayerCharacter();
	~PlayerCharacter() override;

	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
	void Revert(uint32_t aiFlags) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	void UpdateRefLight() override;
	NiPoint3 GetLookingAtLocation() override;
	NiAVObject* Load3D(bool abBackgroundLoading) override;
	void Set3D(NiAVObject* apObject, bool abTestProcessLevel) override;
	NiNode* Get3D() override;
	void UpdateAnimation() override;
	Animation* GetAnimation() override;
	// The first-person or the regular biped.
	BipedAnim* GetBiped() override;
	BipedAnim* GetCurrentBiped() override;
	void SetParentCell(TESObjectCELL* apCell) override;

	void SetDialoguewithPlayer(bool abFlag) override;
	void EndDialogue() override {}
	float GetHeading(bool abIgnoreImmobile) override;
	void Update(float afDelta) override;

	void Resurrect(bool abResetInventory, bool abAttach3D, bool abGetupResurect) override;
	void SetAdvanceNumber(int32_t inumber) override;
	int32_t GetAdvanceNumber() override;
	void SetSkillAdvanceDelayed(ActorValue::Index aeskill) override;
	ActorValue::Index GetSkillAdvanceDelayed() override;
	void UseActionPoints(ActionPoints::Action aeAction) override;
	void UseActionPoints(float afAmount) override;
	bool IsOverEncumbered() override;
	bool IsPc() override { return true; }
	bool GetCannibal() override;
	bool GetSandman() override;
	void InitiateSandmanPackage(Actor* apTarget, TESObjectREFR* apfurniture, FurnitureMark* aMarker,
		uint8_t auiIndex) override;
	void InitiateCannibalPackage(Actor* apTarget) override;
	void SetRefraction(bool abEnable, float afRefractionPower) override;
	int32_t GetActorType() override { return 2; }
	void SetActorValue(ActorValue::Index aiWhich, float afValue) override;
	void TempModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) override;
	void TempModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) override;
	void PermanentModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) override;
	void PermanentModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) override;
	void DamageModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) override;
	void DamageModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) override;
	void ModActorBaseValue(ActorValue::Index aiWhich, int32_t aiValue) override;
	void ModActorBaseValue(ActorValue::Index aiWhich, float afValue) override;
	bool DamageEquipment(ItemChange* apEquipment, float afDamage, bool abIgnoreArmorSkill) override;
	TESObjectREFR* DropObject(TESObject* apObject, ExtraDataList* apExtra, int32_t ainumber, const NiPoint3* apPoint,
		const NiPoint3* apRotate) override;
	void PickUpObject(TESObjectREFR* apobjREFR, int32_t aicount, bool abflag) override;
	void CastScroll(TESObjectBOOK* apScroll, MagicTarget* apTarget) override;
	bool AddSpell(SpellItem* apSpellIn) override;
	bool RemoveSpell(SpellItem* apSpellIn) override;
	bool ReloadWeapon(TESObjectWEAP* apWeapon, int32_t aiReloadAnim, bool bUseModValue) override;
	bool ReloadWeaponNV(TESObjectWEAP* apWeapon, int32_t aiReloadAnim, bool bUseModValue, bool bFromInventory) override;
	uint32_t UseAmmo(int32_t aiShotCount) override;
	void CheckTempModifiers() override;
	CombatGroup* GetCombatGroup() override;
	void SetCombatGroup(CombatGroup* apGroup) override;
	void InitiateSpectator() override {}
	void InitiateGetUpPackage() override;
	void SetAlpha(float afNewAlpha) override;
	CombatController* GetCombatController() override { return nullptr; }
	Actor* GetCombatTarget() override { return pPlayersTargetActor; }
	void StopCombat(Actor*) override {}
	bool HasBeenAttacked() override;
	void SetBeenAttacked(bool abFlag) override;
	void UseSkill(ActorValue::Index aiSkill, uint32_t aiAction) override;
	void RewardExperience(int32_t aiAmount) override;
	float GetBaseValueOverride(ActorValue::Index aiWhich, bool& abExists) override;
	void SetBaseValueOverride(ActorValue::Index aiWhich, float afValue) override;
	void AddGunWobble() override;
	void AddPerk(BGSPerk* apPerk, uint8_t acRank, bool bAlt) override;
	void RemovePerk(BGSPerk* apPerk, bool bAlt) override;
	uint8_t GetPerkRank(BGSPerk* apPerk, bool bAlt) override;
	void AddPerkEntry(BGSPerkEntry* apPerkEntry, bool bAlt) override;
	void RemovePerkEntry(BGSPerkEntry* apPerkEntry, bool bAlt) override;
	BSSimpleList<BGSPerkEntry*>* GetPerkEntryList(uint8_t acEntryPoint, bool bAlt) override;
	void StartAnimOn1stPerson(uint16_t asAnimGroup, ACTION_FLAGS aeFlags) override;
	bool IsImmobile() override;
	void HandleHealthDamage(Actor* apAttacker, float afDamage) override;
	void CreateActorMover() override;
	NiPoint3 CalculateAnticipatedLocation(float afDeltaT) override;

	void CastAbility(SpellItem* apSpell, bool abLoadCast) override;
	void TransferDisease(SpellItem* apSpell, MagicTarget* apDesiredTarget, bool abLoadCast) override;
	MagicTarget* GetTouchTarget() override;
	void StartAim() override;
	bool CheckCast(MagicItem* apSpell, float* afEffectStrength, MagicItem::CannotCastReason* apReason,
		bool abUseBaseMagicka) override;
	float GetMagicEffectivenessModifier(bool, float) override;
	MagicItem* GetCurrentSpell() override { return pCurrentSpell; }
	void SetCurrentSpell(MagicItem* apCurrentSpell) override;
	MagicTarget* GetDesiredTarget() override { return pDesiredTarget; }
	void SetDesiredTarget(MagicTarget* apDesiredTarget) override;

	bool AddTarget(MagicCaster* apCaster, MagicItem* apSpell, ActiveEffect* apActiveEffect, bool abAreaTarget) override;
	BSSimpleList<ActiveEffect*>* GetActiveEffectList() override;
	bool IsInvulnerable() override;

	int32_t GetActorValue(ActorValue::Index aiWhich) override;
	float GetActorFloatValue(ActorValue::Index aiWhich) override;
	float GetTemporaryModifier(ActorValue::Index aiWhich) override;
	float GetDamageModifier(ActorValue::Index aiWhich) override;
	float GetPermanentModifier(ActorValue::Index aiWhich) override;
	int32_t GetPermanentActorValue(ActorValue::Index aiWhich) override;
	float GetPermanentActorFloatValue(ActorValue::Index aiWhich) override;

	virtual bool InitiateDialogue(MobileObject*, PackageLocation*, PackageLocation*, bool, bool, bool, TESTopic*) {
		return false;
	}

	// The perk ranks of the player, or of the companions when abCompanion is set.
	virtual BSSimpleList<PerkRankData*>* GetPerkList(bool bAlt);

	// What the player can do with the terminal apTerminalRef.
	TerminalAccessType GetTerminalAccess(TESObjectREFR* apTerminalRef);
	// The last action of the player's action list.
	int32_t GetLastAction();
	// Whether action aeAction is in the player's action list.
	bool IsActionInList(PLAYER_ACTION aeAction);
	// A player test the crosshair prompt of a sleeping actor asks.
	bool QCanWakeSleepingActor();
	// Rebuilds the player's 3D after an equipment change.
	void UpdatePlayer3d();
	// Finds where apObject lands when dropped into aPoint; false when there is no room.
	bool GetDropPoint(TESBoundObject* apObject, NiPoint3& aPoint, bool abAllowBackupPoint, bool abAdvanceExtraRotate);
	TESObjectWEAP* GetQueuedWeaponAttach() { return pQueuedWeaponAttach; }
	// Whether hardcore tracking is off for good (bNoHardcoreTracking).
	bool GetAlwaysHardcore();
	// Stores the hours to sleep or wait and whether the player sleeps.
	void SetSleeping(int32_t aiTime, bool absleep);
	// Adds aiAmount to the amount sold or stolen.
	void ModAmountStolenSold(int32_t aiAmount);
	// Marks the player as resisting arrest (being chased by the guards).
	void SetPlayerResistingArrest(bool abflag);
	// The callback of the hardcore setting: 1 turns hardcore on, 0 turns it off; other values do nothing.
	void SetHardcoreCallback(int32_t aiVal);
	// Marks the player as being greeted.
	void SetPlayerGreetFlag();
	// Whether a hostile actor has detected the player.
	bool GetHostileDetection();
	// Takes apActor off the player's combat group's targets.
	void RemoveActorFromPlayercombatList(Actor* apActor);
	// Queues apTargetLoc to move the player (freeing an earlier request) and handles it at once in a cell test.
	void RequestPositionPlayer(PLAYER_TARGET_LOC* apTargetLoc);
	NiPoint3 GetLastKnownGoodPosition();
	BSSimpleList<TESQuestStageItem*>* GetQuestLog();
	bool GetTelekinesisSelected();
	// Changes the karma by aiAmount.
	void RewardKarma(int32_t aiAmount);
	void SetPCYoung(bool abVal);
	// Ends the greeting once its timer passes 5 seconds.
	void CheckPlayerGreetFlag();
	// Returns pInactiveListofCaravanCards.
	BSSimpleList<TESCaravanCard*>* GetCaravanCards1();
	// Returns pActiveListofCaravanCards.
	BSSimpleList<TESCaravanCard*>* GetCaravanCards2();
	// Appends one tab-separated line of the player's progress (name, version, hours, the aeEvent event name, race,
	// sex, level, perks, attributes and skills, inventory, quests, misc stats, cell, and for DEATH the death) to
	// ProgressData_1.txt in the sTrackProgressPath folder.
	void ExportProgressData(PROGRESS_EVENT aeEvent);
	uint32_t GetCaravanLosses();
	uint32_t GetCaravanLargestWinning();
	void SetCaravanCapWinnings(int32_t iValue);
	void SetCaravanCapLosses(int32_t iValue);
	void SetCaravanWinnings(int32_t iValue);
	void SetCaravanLosses(int32_t iValue);
	void SetCaravanLargestWinning(int32_t iValue);
	// Stores pRegion as the region the player is in.
	void SetOccupiedRegion(TESRegion* pRegion);
	// The player's active quest's objective that is displayed, else null. It returns an item of listObjectives, so the
	// result is a BGSQuestObjective.
	BGSQuestObjective* GetCurrentQuestObjective();
	// The audio marker the player is in or nearest to.
	AudioMarkerInfo* GetClosestAudioMarkerInfo();
	void SetControlsDisabled(bool abDisable, PlayerCharacter::DISABLE_PLAYER_CONTROLS aeDPC);
	// Sets which player controls are disabled.
	void SetControlsDisabled(uint8_t aucDisabled);
	PlayerCharacter* SetKillCamMode(KILL_CAM_TYPE eType);
	void ResetKillCam(KILL_CAM_TYPE eOldType, bool bForceReset);
	uint32_t GetTotalPlayingTime();
	void UpdateTotalPlayingTime();
	void RemoveTeammate(Actor* pActor);
	void AddTeammate(Actor* pActor);
	// Sets the wanted view and shows or hides the first-person 3D. Returns whether the wanted view was already the
	// current one (ForceTemp1stPerson passes it on to Actor::SetIronSights, which reads it).
	bool SetFirstPerson(bool abFirst);
	// Switches to the third-person view for a while.
	bool ForceTemp3rdPerson(bool abForceCameraDistance);
	void SetSelectedSpell(MagicItem* apSelectedSpell);
	void ResetPlayerGreetFlag();
	// Picks the closest conversation to the player.
	void CheckForClosesConversation(MobileObject* apSpeaker, MobileObject* apTarget, DialoguePackage* apDialogPack);
	void AddTopics(BSSimpleList<TESTopic*>* apAddTopicList);
	void StopVanityMode();
	void SpellLearned(SpellItem* apSpellIn);
	void IncNumCrimes(PackageNS::CRIME_TYPE aiCrimeType, int32_t aiAmount);
	// Readies an inventory node for use: drops its non-transform controllers, clears the collision object and shows
	// its subtree.
	void RecursiveReadyInventoryNode(NiNode* apNode);
	// The root of the inventory menu's player model.
	NiNode* GetInventory3D();
	// Sets the count of shots fired since the last ammo regeneration tick.
	void SetShotsSinceLastAmmoRegenTick(uint8_t acShots);
	// Records a note on the player, once, and shows its name.
	void AddNote(BGSNote* apNote, bool abVerbose);
	// Sets the scroll the player has selected.
	void SetSelectedScroll(TESObjectBOOK* apScroll);
	// Swaps the ammo of the current weapon for the next type in its ammo list that the player carries.
	void AmmoSwapHelper(bool abStartReloadAnimation, bool bOnlyIfEmpty);
	// Stores apAmmo as the ammo of hotkey aiHotkey in HotKeyLastAmmo.
	void SetAmmoForHotKey(int32_t aiHotkey, TESAmmo* apAmmo);
	// The ammo stored for hotkey aiHotkey, or null outside 0-8.
	TESAmmo* GetAmmoForHotKey(int32_t aiHotkey);
	// Moves the third-person camera at apCameraPos to its chase distance from apTargetPos, zooming, colliding and
	// fading the player as it closes in; abSnapXY snaps it there at once.
	void UpdateChaseCamera(NiPoint3* apCameraPos, NiPoint3* apTargetPos, bool abSnapXY);
	BSSimpleList<TESTopic*>* GetTopicList();
	BSSimpleList<TESRegionSound*>* GetRegionSounds();
	// The player's level-up progression data.
	CharacterProgression* GetCharacterProgressionInfo();

	bool Is3rdPerson();
	void Force3rdPerson(bool ab3rdPerson);
	bool IsActually3rdPerson();
	// The third-person 3D when the camera is actually in third person, else the first-person
	// 3D.
	NiAVObject* GetCurrent3D();
	bool IsSleeping();
	bool IsSleepingorResting();
	bool IsPCYoung();
	bool IsPCToddler();
	bool IsHardcore();
	bool GetAllowEGMCacheClear();
	bool GetMenuModeButtonClicked();
	bool GetUnderAIControl();
	void SetCanWait(bool abFlag);
	bool IsAcousticSpaceTransition();
	// True while the pip-boy is up.
	bool IsPipboyActive();
	// The actors the compass tracks.
	BSSimpleList<CompassTarget*>* GetListofPerceivedActors();
	// Whether target is among the perceived actors and hostile; false when it is not perceived.
	bool IsTargetPerceivedAndHostile(Actor* target);
	// Frees and removes apActor's entry from the perceived actors.
	void RemovePerceivedActor(Actor* apActor);
	// The reference of the most recent player action, or null.
	TESObjectREFR* GetRecentAction();
	// The compass entry for apActor, or null.
	CompassTarget* IsActorPerceived(Actor* apActor);
	// Whether apActor has an entry in pListofPercievedActors that is not bIsUndetected; false when it has none.
	// CombatManager::ShouldPlayCombatMusic calls it on pPlayer.
	bool IsPerceivedActorDetected(Actor* apActor);
	// How many actors follow the player.
	int32_t GetNumberOfFollowers();
	// Whether apActor is one of the player's followers.
	bool IsPlayerFollower(Actor* apActor);
	// The quest the player has active.
	TESQuest* GetActiveQuest();
	// Makes apActiveQuest the player's active quest.
	void SetActiveQuest(TESQuest* apActiveQuest);
	// Refreshes the player's objective list for apObjective.
	void AddQuestObjective(BGSQuestObjective* apObj);
	// The summed counts of the Rock-It Launcher's ammo items.
	int32_t GetNumRockItAmmo();
	bool IsInSlowMo();
	// Picks the actor the aim assist locks on to.
	void UpdateAutoAimActor();
	// The aim assist's actor, or null when there is none or it no longer qualifies.
	Actor* GetAutoAimActor();
	// Turns apHeading and apLooking toward the aim assist's actor for the projectile or
	// the release node apReleaseObj.
	void ApplyAutoAimActor(Projectile* apProjectile, NiAVObject* apReleaseObj, float& apHeading, float& apLooking,
		NiPoint3* apOrigin, float afStartHeading, float afStartLooking);
	// Moves the player to the queued position request; true when it did.
	bool HandlePositionPlayerRequest();
	// Places the player at aPos, turned to aAngle, in apCell, and brings the scene, sound and camera along.
	void PositionPlayer(NiPoint3 aPos, NiPoint3 aAngle, TESObjectCELL* apCell, bool abResetWeather);
	// Places the player at aPos in apWorld.
	void PositionPlayerExterior(NiPoint3 aPos, NiPoint3 aAngle, TESWorldSpace* apWorld, bool abResetWeather);
	// Fast-travels the player to apRef.
	void FastTravel(TESObjectREFR* apRef);
	// Unloads the loaded cells unless apTarget (or apWorldSpace's) is already loaded.
	void ClearCellsIfTargetNotLoaded(TESObjectCELL* apTarget, TESWorldSpace* apWorldSpace);
	// Updates the player camera. abSkipUpdateLOD.
	void UpdateCamera(bool abForce, bool abSkipUpdateLOD);
	// Turns the player's view from the mouse or controller, or toward the VATS target.
	bool UpdateHeadingAndLooking(float afDelta, bool abHorse, uint16_t* apMoveMode, bool abScanFinished);
	// Moves the player to the cell named apCellID, or to apCellIn when it is given (the coc console command).
	void CenterOnCell(char* apCellID, TESObjectCELL* apCellIn);
	// Bit 2 of bCanFastTravel.
	bool GetFlag66DIs2();
	// Opens the wait menu, or shows why the player can't wait now.
	bool CanPassTime();
	// Whether an alarm is sounding for the player.
	bool IsAlarmForPlayerCharactaer();
	// Whether the player may wait (bCanWait).
	bool GetCanWait();
	// Optionally culls the third-person 3D, then finds its camera node.
	void Setup3rdPerson3D(NiAVObject* ap3rdPerson3D, bool abCull3rdPerson3D);
	// Drops the magic cast sound.
	void ResetMagicCastSound();
	// Moves the fly camera by the frame time.
	void UpdateFlyCamera(float afDelta, bool abFreezeTime);
	// Keeps a cooked grenade held while a menu is up.
	void ForceGrenadeHold();
	// Reads the attack control: queues and starts the player's attacks, power attacks, throws, reloads and VATS
	// shots; false when a reload loop has no ammo left to load.
	bool CheckUserInputAttacks();
	void SetAcousticSpaceTransition(bool val);

	// True when the control bits of aeDPC are disabled.
	bool GetControlsDisabled(PlayerCharacter::DISABLE_PLAYER_CONTROLS aeDPC);

	// True while the player is in combat; also reports whether every combat target is only searching.
	bool IsPlayerCharacterInCombat(bool* abAreAllCombatTargetsSearching);

	GamePlayFormulas::DifficultyLevel GetGameDifficulty();
	KILL_CAM_TYPE GetKillCamMode();
	float GetWorldFOV();
	float Get1stPersonFOV();

	TESRegion* GetOccupiedRegion();
	TESObjectREFR* GetGrabObject();
	NiAVObject* GetWeaponSightingNode();
	void SetWeaponSightingNode(NiAVObject* apSightingNode);
	void SetForceActivateRef(TESObjectREFR* apRef);
	void SetMegatonDestroyed(bool abDestroyed);
	void SetTimeSinceLastAmmoRegenTick(float fTime);
	void SetBlockCounterTimer(float fTime);
	float GetBlockCounterTimer();
	void ClearsHardcoreFlag();
	// Shows sNoCharge once, until ResetInsufficientChargeMessage clears bInsufficientChargeMessageShown.
	void ShowInsufficientChargeMessage();
	void ResetInsufficientChargeMessage();
	void ClearClosestAudioMarkerInfo();
	// Registers the first-person skeleton's idle directories, then the third-person model's locomotion ones.
	void Preload1stPersonAnimsFileList();
	// Queues the player's first-person skeleton, its animations and the third-person model's animations.
	void QueuePlayerAnims(IO_TASK_PRIORITY aePriority, QueuedFile* apParent);
	// Turns hardcore mode on or off; turning it off clears the hardcore needs unless bIgnoreTracking.
	void SetHardcore(GamePlayFormulas::HardcoreSetting aeHardcoreLevel, bool bIgnoreTracking);

	BipedAnim* GetBiped(bool abFirstPerson);
	bool Is1stPersonBiped(BipedAnim* apBiped);
	// Whether either of the player's bytes at +0x64F and +0x64D is set.
	bool GetTempPerson();

	Animation* GetAnimation(bool abFirstPerson);
	// The first-person 3D when abFirstPerson is set, else the third-person one.
	NiAVObject* Get3D(bool abFirstPerson);
	// Rebuilds the shape caster after the player's scale or controller changes.
	void ResetShapeCasterInfo();
	TESRegionList* GetAllOccupiedRegions();
	void SetCanFastTravel(bool abflag);
	// True while bit 2 of the fast travel flags debounces the movement keys.
	bool GetIsDebounceWASD();
	uint32_t GetCaravanWinnings();
	BSSimpleList<CasinoData*>* GetCasinoDataList();
	// Whether the casino anti-cheat timer still runs; once it has run out it deactivates the
	// measures and clears the timer.
	bool CasinoAntiCheatCheck();
	// The anti-cheat seconds left: the timer's start plus iAntiCheatDuration minus the tick
	// count in seconds.
	int32_t GetCasinoCheatTimeRemaining();
	void ResetLastPlayingTimeUpdate();
	void ClearAudioMarkerList();
	void ResetAudioMarkerList();
	// Frees the map marker entries and empties the list.
	void ClearMapMarkerStructList();
	void ResetMapMarkerStructList();
	void UpdateFirstPersonZoom();
	// Switches the player's 3D to or from first person.
	void Show1stPerson(bool abFirstPerson);
	float SetFOV(float afFOV);
	bool AddTopic(TESTopic* apNewTopic, bool abResort, bool abDisplayMessage);
	// The targets of the active quest's objectives, rebuilt when needed; null without an
	// active quest.
	BSSimpleList<TESQuestTarget*>* GetCurrentTargetList();
	bool IsActorFollowingPlayer(Actor* apActor);
	void RemoveConflictingObject(TESObjectREFR* apRef);
	void RemovePlayerAction(PLAYER_ACTION aeAction, TESObjectREFR* apActObject);
	// Records the player action aeAction on apActObject for afTime seconds.
	void AddPlayerAction(PLAYER_ACTION aeAction, float afTime, TESObjectREFR* apActObject);
	// Eases the iron sights depth of field toward the iron sights strength cap while aiming in first person.
	void UpdateImageSpaceIronsights();
	// Eases the VATS depth of field and radial blur toward their caps while VATS targets a reference.
	void UpdateImageSpaceVATS();
	// The first door on the path to the player's map marker, else the marker itself.
	TESObjectREFR* QPlayerMarkerTrackingRef();
	// Creates the player's map marker on first use, places it at arPoint and parents it to
	// the cell of apForm, then rebuilds the path to it.
	void SetPlayerMapMarker(NiPoint3 arPoint, TESForm* apForm);
	// True while an actor greets the player.
	bool GetPlayerGreetFlag();
	// True when apNote is in the player's notes.
	bool HasNote(BGSNote* apNote);
	// Removes apNote from the player's notes and from the map menu's selection.
	void RemoveNote(BGSNote* apNote);
	// Pitches the first-person arms with the camera, easing how far they follow.
	void UpdateArmRotation();
	// Forces a quest target update when apMovingRef is the player, a quest target, or a container holding one.
	void CheckForQuestTargetUpdate(TESObjectREFR* apMovingRef);
	// Finds the doors between the player and apTargetRef.
	void BuildPathToTarget(TESObjectREFR* apTargetRef, TeleportPath* apOutResultPath, bool abIgnoreLocks);
	// Sets whether a hostile actor has detected the player.
	void SetHostileDetection(bool abflag);
	// Marks apActor as hostile or not to the player, for the compass.
	void ChangePerceivedActorHostileStatus(Actor* apActor, bool abHostile);
	// True while the running AI conversation has a talking actor.
	bool IsImportantConversationRunning();
	// Stores pAIConversationRunning.
	void SetImportantConversationRunning(DialoguePackage* appack) { pAIConversationRunning = appack; }
	// Zeroes the Rock-It Launcher's ammo weight; abRecalculateWeight sums it again from the ammo items.
	void ResetRockItAmmoWeight(bool abRecalculateWeight);
	// Frees the compass targets in pListofPercievedActors and
	// deletes the list.
	void FreeCompassTargets();
	// Switches the player to or from the toddler body.
	void SetPCToddler(bool abVal);
	// Empties the queued food and drink items.
	void CastEatDrinkItems();
	// Casts the enchantments queued for the player.
	void CastQueuedEnchantments();
	// Sets the first- or third-person light the player carries.
	void SetLightPointer(NiLight* apLight, bool abFirstPerson);
	// The space index last used through apDoor's random door, 0xFF when none is recorded.
	uint8_t GetLastSpaceForDoor(TESObjectDOOR* apDoor);
	void SetLastSpaceForDoor(TESObjectDOOR* apDoor, uint8_t acSpaceIndex);
	// Sets or clears the move mode flag aiMoveMode on the player's mover; turning on 0x400 blends out an aim
	// animation on the first-person weapon layer.
	void SetMoveMode(uint32_t aiMoveMode, bool abValue);
	// True when the character controller stands on a furniture reference.
	bool IsStandingOnFurniture();
	// The reference whose body the character controller bumped, unless it is the support body.
	TESObjectREFR* GetBumpedRef();
	// Passes the ingested and the environmental radiation deltas to the HUD's radiation sound.
	void ReportRadiationChange();
	// Drops the extra data of every Rock-It Launcher ammo item, then empties the list and resets its weight.
	void ClearRockItAmmo();
	int8_t GetShotsSinceLastAmmoRegenTick();
	bool GetCateyeActive();

protected:
	void SetAIControlledToPosition(bool abTF);
	void SetAIControlledFromPosition(bool abTF);
	// The value of the modifier group eGroup for aiWhich.
	float GetModifier(ModifierGroup eGroup, ActorValue::Index aiWhich);
	// Adds afValue to the modifier of aiWhich in group eGroup, clamped by aeClampStyle.
	void AlterModifier(ModifierGroup eGroup, ActorValue::Index aiWhich, float afValue,
		ModifierList::ClampStyle aeClampStyle);

public:
	// Returns 0 on its early exits, else SetFirstPerson's result; Actor::SetIronSights reads it.
	bool ForceTemp1stPerson(bool abIronSights1stPerson);
	void EnteringMenuMode();
	void QueueAttachWeapon(TESObjectWEAP* apWeapon);
	void DestroyMouseSpring();
	// Pulls the grabbed object toward the point ahead of the eyes, or drops it when it is out of reach.
	void UpdateMouseSpring(float afTimeDelta);
	void SetSpeaking(bool abValue);
	float GetRockItLauncherWeight() const;
	// Removes one random entry of the Rock-It Launcher ammo list and returns it; null when the list is empty.
	ItemChange* GetRandomRockItAmmoItem();
	uint32_t GetMinorCrimeCount() const;
	int32_t GetSleepHours() const;
	NiPoint3* GetCameraPosition();
	BSSimpleList<Actor*>* GetTeammateList();
	float GetEyeHeight() { return fEyeHeight; }
	NiPoint3* GetCached1stPersonCameraPos();
	// Copies kCamera3rdPersonShoulderOffset to apResult.
	NiPoint3* GetCamera3rdPersonShoulderOffset(NiPoint3* apResult);
	BSSimpleArray<ItemChange*>* GetRockItAmmoArray();
	TESObjectREFR* QPlayerMapMarker() const;
	BSSimpleList<BGSQuestObjective*>* GetQuestObjectiveList();
	TeleportPath* QPlayerMarkerPath();
	Animation* GetInventoryAnimation() { return pInventoryAnimation; }
	Actor* GetAutoAimActorSimple() { return pAutoAimActor; }
	TESObjectREFR* GetForceActivateRef() { return pForceActivateRef; }
	BSSimpleList<PlayerCharacter::MapMarkerInfo*>* GetMapMarkerStructList();
	void SetInCharGen(bool abInCharGen);
	bool IsVanityMode();
	void ForcePhysicsGrabDrop();
	void ClearSitHeadingDelta();
	void ClearAutoAim();
	void ClearGrabObject();
	void SetWorldFOV(float afFOV);
	void Set1stPersonFOV(float afFOV);
	void SetBlockActivate(bool abVal);
	bool GetWant3rdPerson();
	char GetPCControlFlags();
	uint32_t GetTeammateCount();
	bool GetInChargen();
	void SetAIControlledPackage(bool abTF);
	void SetDisableFastTravelResetsOnCellChange(char a2);
	void SetPCCanUsePowerArmor(bool abVal);
	void* GetCameraRigidBody();
	bool GetMegatonDestroyed();
	void SetLockpickControlsDisable(char a2);
	void SetAllowEGMCacheClear(bool abAllow);
	MagicShaderHitEffect* GetMagicShaderHitEffect();
	void SetTelekinesisDistance(float a2);
	float GetGrabDistance();
	bool CanPCUsePowerArmor();
	bool IsPlayerResistingArrest();
	bool IsGreetingTimerClear();
	int32_t GetSandManDetectionValue();
	void SetInventoryWeaponEffect(MagicShaderHitEffect* apEffect);
	bool GetAIControlledFromPosition();
	void AddActorToPlayerCombatList(Actor* apActor);
	int32_t GetScriptAllowedFastTravel();
	bool GetBlockActivate();
	MagicItem* GetSelectedSpell();
	bool CheckInvulnerable(ActorValue::Index aiWhich, float afValue);
	int32_t GetNumberActorsInCombat();
	void SetNeedResetHardcoreTimer(bool aNeedReset);
	void StopIronSightsImageSpace();
	void StopVatsImageSpace();
	bool NeedResetHardcoreTimer();
	void SetActorInSneakRange(bool abflag);
	char IsShowQuestItems();
	int32_t GetAmountSoldStolen();
	void SetLastModifiedFactionReputation(TESReputation* a2);
	uint32_t GetCaravanCapWinnings();
	void ToggleKillCamMode(KILL_CAM_TYPE arg0);
	void* GetBhkMouseSpringAction();
	PlayerCharacter::GrabbingType GetGrabMode();
	void StopHeartbeatSound();
	void SetLastGreetingTime();
	void SetLastKnownGoodPosition();
	void ModPlayerGreetTimer();
	bool IsAMurderer();
	void SetIsAMurderer();
	void IssueStealWarning();
	void IssuePickPocketWarning();
	char IsPlayingCombatAttackSound();
	void UpdateTemp3rdPerson();
	void Setup3rdPersonCameraCaster();
	bool ToggleFlyCam(bool abFreezeTime);
	Actor* GetBumpedActor();
	MediaLocationController* GetMediaLocationController();
	char GetInSleepPackage();
	int32_t GetNumberofNotes();
	void ReturnRockItAmmo();
	uint32_t GetCaravanCapLosses();
	// The potions the player has queued to eat or drink, plus one per distinct active potion effect.
	int32_t GetCurrentPotionCount();
	void ClearTeammates();
	bool GetTemp1stPersonSwitchBack();
	bool AddCaravanCard(TESCaravanCard* apCard);
	void SetActorBaseValue(ActorValue::Index a1, int32_t a2);
	void StartKillcamForActor(TESObjectREFR* a1, float a2, bool a3, int32_t a4);
	bool AddEatDrinkItem(MagicItem* apItem);
	void DoIronSightsZoom(float afDelta);
	void CreateMouseSpring(TESObjectREFR* apObject, PlayerCharacter::GrabbingType aeGrabType, float afDistance);
	void PlayMagicFailureSound(MagicSystem::School aeSchool);
	void HaveFollowersRecheckPackages();
	void InsertPerceivedActor(Actor* apActor, bool abAttack, bool abHasLineSightNotDetection);
	bool IsDetectedByCompassTargetExceptActor(Actor* apActor);
	// Copies the argument's byte into bAiControlledActivate unchanged.
	void SetIsDrinkingPlacedWater(bool abDrinking);
	void ClearDroppedReference(TESObjectREFR* apRef);
	void SetDifficultyLevel(GamePlayFormulas::DifficultyLevel aeDifficultyLevel);
	void SetActorBaseValueF(ActorValue::Index a1, float a3);
	TESObjectCELL* GetWaterCell(float afCheckRadius);
	bool CanFastTravel();
	void Sneak();
	void SpotterEffectNearbyEnemies();
	void ShaderNearbyItems();
	void UpdatePlayerCombat(float afDelta);
	// Checks the player against the border regions: keeps the last good position inside them, else returns there.
	void CheckBorderRegion();
	void HandleFlycamMovement();
	void ReturnToLastKnownGoodPosition(bool abPositionPlayer);
	void PlayerSleep();
	void UpdateTemp1stPerson();
	NiNode* CloneInventory3D(bool abFlag);
	bool AddQuestStageItem(TESQuestStageItem* apNewItem);
	void RemovePlayerMapMarker();
	void FocusOnActor(MobileObject* apActor, float afZoomPercent, bool abOnlyZoom);
	void InitiateSitSleepPackage(TESObjectREFR* apCurrentFurniture);
	bool IsActorFollowingEscortingPlayer(Actor* apActor, bool bAll);
	bool ReloadLoop(TESObjectWEAP* apWeapon, int32_t aiReloadAnim, bool bUseModValue, bool bFromInventory);
	void CheckPoisonWeapon(AlchemyItem* apPoison);
	void AddQueuedEnchantment(TESBoundObject* apObject, ExtraDataList* apExtra);
	void RemoveQueuedEnchantment(TESBoundObject* apObject, ExtraDataList* apExtra);
	void HandlePhysicsGrab(float afTimeDelta);
	bool UpdateDropSpring(float afTimeDelta);
	void CheckGetAwayFromGuards();
	void PlayImpactBasedHitShader(TESObjectREFR* apRef);
	void HavokActivateDroppedReference();
	bool IsConflictingObject(NiAVObject* apObj);
	void UpdateConflictingObjects();
	void UpdatePlayerActions();
	void RemoveAllActionsForObject(TESObjectREFR* apActObject);
	BGSNote* GetNotebyIndex(int32_t aiIndex);
	void UpdateMenuModeButton(float afTimeDelta);
	bool CanEquipOrUseObject(ItemChange* apItem);
	uint32_t AttemptYield();
	void AddRockItAmmo(TESBoundObject* apObj, ExtraDataList* apExtra, int32_t aiNumAdded);
	ItemChange* RemoveRockItAmmo(TESBoundObject* apObj, ExtraDataList* apExtra, int32_t aiNumRemoved,
		bool abReturnRemovedItem);
	void unset_11E07AB();
	void resetAntiCheatTimer();
	void UpdateHardcoreMode();
	// Returns the raw byte, which callers test as a byte.
	uint8_t GetActorInSneakRange();
	// The bullet auto-aim point (BulletAutoAim), returned through apResult.
	NiPoint3* GetBulletAutoAim(NiPoint3* apResult);
	double GetTimeGrenadeHeld();
	PlayerActionObject* GetPlayerAction(PLAYER_ACTION aeAction);
	// Stores the max carry weight in afMaxEncumbrance, then actor value 0x2E (the
	// inventory weight) as a float in afCurrentEncumbrance.
	void GetInventoryEncumbrance(float& afCurrentEncumbrance, float& afMaxEncumbrance);

	// Resets every terminal.
	static void DebugResetAllTerminals();
	// The death message box's callback: loads the most recent save (button 1) or returns to the main menu (button 2).
	static void DeathMenu();
	static bool IsGodMode();
	static bool IsDemigodMode();
	static void SpeakDoneCallback(void* apUnused, bool abSucceeded);
	static void ForceReload();
	static void SetGodMode(bool abGodMode);
	static void SetDemigodMode(bool abDemigodMode);
	static void ForceRestart();
	// The sum of the player's seven permanent SPECIAL values (ActorValueOwner's GetPermanentActorValue); the menus
	// compare it with and subtract it from int32_t point counts.
	static int32_t GetSumOfSPECIALStats();
	static void PoisonWeapon();

	// The player's achievement progress. The constructor clears each entry's bUnlocked and iCurrentValue and sets
	// bTracking from the achievement table.
	AchievementInstance PlayerAchievements[3];
	PLAYER_TARGET_LOC* pQueuedTargetLoc;
	// the weapon AddWorn queued for attaching to the player's 3D (QueueAttachWeapon stores it)
	TESObjectWEAP* pQueuedWeaponAttach;
	float fTimeSinceLastAmmoRegenTick;
	int8_t cShotsSinceLastAmmoRegen;
	uint8_t padding1F9[0x3];
	uint32_t iSandmanDetection;
	int32_t iCombatPersue;
	bool bTravelUseDoor;
	bool bOnElevator;
	bool bForceQuestTargetUpdate;
	uint8_t padding207[1];
	DialoguePackage* pClosestConversation;
	bool btransporting;
	uint8_t padding20D[3];
	BSSimpleList<ActiveEffect*>* pActiveEffectList;
	MagicItem* pCurrentSpell;
	MagicTarget* pDesiredTarget;
	CameraCaster* pCameraCaster;
	HitCaster* pHitCaster;
	DialoguePackage* pAIConversationRunning;
	int32_t iNumberofStealWarnings;
	float fStealWarningTimer;
	int32_t iNumberofPickpocketWarnings;
	float fPickPocketWarningTimer;
	BSSimpleList<MagicItem*>* EatDrinkItems;
	BSSimpleList<TESBoundObject*>* QueuedWornEnchantments;
	bool bShowQuestItemsInInventory;
	uint8_t padding241[0x3];
	float TemporaryActorValueModifiers[77];
	float ScriptActorValueModifiers[77];
	float fHealthModifier;
	float DamageActorValueModifiers[77];
	BSSimpleList<BGSNote*> listNotes;
	ImageSpaceModifierInstanceDOF* pIronsightsDOFInstance;
	ImageSpaceModifierInstanceDOF* pVatsDOFInstance;
	ImageSpaceModifierInstanceDRB* pVatsDRBInstance;
	bool bHostileDetection;
	bool bIsAcousticSpaceTransition;
	uint8_t padding5FA[0x2];
	BSSimpleList<Actor*> pListOfTeammates;
	TESObjectREFR* pLastExtDoorActivated;
	bool bSpeaking;
	uint8_t padding609[0x3];
	BSSimpleList<PlayerActionObject*>* pListofActions;
	BSSimpleList<CasinoData*>* pListofCasinoData;
	BSSimpleList<TESCaravanCard*>* pInactiveListofCaravanCards;
	BSSimpleList<TESCaravanCard*>* pActiveListofCaravanCards;
	uint32_t iCaravanCapWinnings;
	uint32_t iCaravanCapLosses;
	int32_t iCaravanWinnings;
	uint32_t iCaravanLosses;
	uint32_t iCaravanLargestWinning;
	int32_t iCasinoCheatLevel;
	NiPointer<bhkMouseSpringAction> spGrabSpring;
	TESObjectREFR* pGrabbedObject;
	GrabbingType eGrabType;
	float fGrabObjectWeight;
	float fGrabDistance;
	bool bsave3rdPerson;
	bool b3rdPersonSaved;
	bool b3rdPerson;
	bool bActually3rdPerson;
	bool bWant3rdPerson;
	bool bTemp3rdPerson;
	bool bTemp3rdPersonSwitchBack;
	bool bTemp1stPerson;
	bool bTemp1stPersonSwitchBack;
	bool bAlwaysRun;
	bool bAutoMove;
	uint8_t padding653[0x1];
	int32_t iSleepTime;
	bool bIsSleeping;
	uint8_t padding659[0x3];
	float fFOV;
	float fsecondRunning;
	float fsecondSwimming;
	float fsecondSneaking;
	uint8_t bActorinSneakRange;
	// The fast travel flags: bit 0 set while scripts allow fast travel, bit 2 debounces the movement keys.
	uint8_t bCanFastTravel;
	bool bCanWait;
	uint8_t padding66F[0x1];
	float fWorldFOV;
	float f1stPersonFOV;
	float f3rdPersonFOV;
	int32_t iNumberTraining;
	uint8_t ucControlsDisabled;
	bool bBlockActivate;
	uint8_t padding682[0x2];
	float fBlockActivateTimer;
	TESObjectREFR* pForceActivateRef;
	BipedAnim* p1stPersonBipedAnim;
	Animation* p1stPersonAnimation;
	NiPointer<NiNode> sp1stPerson3D;
	float fEyeHeight;
	NiPointer<NiNode> spInventoryPC;
	Animation* pInventoryAnimation;
	MagicShaderHitEffect* pInventoryWeaponEffect;
	BSSimpleList<void*> listTopics;
	BSSimpleList<void*> listQuestLog;
	TESQuest* pActiveQuest;
	// LoadGame adds TESQuest::GetObjective results and GetQuestTargets hands it to TESQuest::UpdateCurrentTargetList.
	BSSimpleList<BGSQuestObjective*> listObjectives;
	BSSimpleList<TESQuestTarget*> listQuestTargets;
	bool bGreetingPlayer;
	uint8_t padding6CD[0x3];
	float fGreetTimer;
	int32_t ihourstosleep;
	char cMurder;
	uint8_t padding6D9[0x3];
	int32_t iAmountStolenSold;
	float fSortActorDistanceTimer;
	float fSitHeadingDelta;
	uint8_t bBeenAttacked;
	uint8_t padding6E9[0x3];
	MagicItem* pSelectedSpell;
	TESObjectBOOK* pSelectedScroll;
	TESObjectREFR* pPlayerMapMarker;
	TeleportPath PlayerMarkerPath;
	// The time the throw button has been held.
	float fProjectileReleaseTimer;
	int32_t iNumAdvance;
	ActorValue::Index eskilladvance;
	TESClass* pDefaultClass;
	TESClass* pClassBasedOn;
	// Counts per crime type despite the name: GetMinorCrimeCount returns one, IncNumCrimes adds to one.
	uint32_t pCrimeCounts[5];
	AlchemyItem* pPendingPoison;
	bool bChargen;
	bool bAllowEGMCacheClear;
	bool bTelekinesisSelected;
	uint8_t padding75F[0x1];
	TESRegion* pOccupiedRegion;
	TESRegionList m_AllOccupiedRegions;
	BSSimpleList<void*> CurrentRegionSoundList;
	BSSoundHandle StatusSoundHandle;
	void* pInitialStateBuffer;
	uint32_t iLastPlayingTimeUpdate;
	uint32_t iTotalPlayingTime;
	uint32_t iCharacterSeed;
	bool bAiControlledToPos;
	bool bAiControlledFromPos;
	bool bAiControlledActivate;
	bool bAiControlledPackage;
	bool bInBorderContainedCell;
	bool bReturnToLastKnownGoodPosition;
	uint8_t padding79E[0x2];
	NiPoint3 LastKnownGoodPosition;
	TESForm* pLastKnownGoodLocation;
	NiTPrimitiveArray<TESRegion*>* pBorderRegions;
	// The music type of the cell the player was last placed in.
	BGSMusicType* pLastKnownMusicType;
	GamePlayFormulas::DifficultyLevel eDifficultyLevel;
	GamePlayFormulas::HardcoreSetting eHardcoreSetting;
	KILL_CAM_TYPE eKillCameraSetting;
	bool bBeingChased;
	bool bIsYoung;
	bool bIsToddler;
	bool bCanUsePowerArmor;
	BSSimpleList<MapMarkerInfo*> MapMarkerList;
	TESWorldSpace* pMapWorld;
	BSSimpleList<AudioMarkerInfo*> AudioMarkerList;
	AudioMarkerInfo* pClosestAudioMarkerInfo;
	float fUFOCameraHeading;
	float fUFOCameraPitch;
	NiPoint3 UFOCameraPos;
	uint32_t iSelectedSpellCastSoundID;
	BSSoundHandle SelectedSpellCastSound;
	BSSoundHandle MagicFailureSounds[6];
	BSSimpleList<TESObjectREFR*> DroppedRefList;
	NiTMap<uint32_t, uint8_t> RandomDoorSpaceMap;
	NiPointer<NiLight> sp1stPersonLight;
	NiPointer<NiLight> sp3rdPersonLight;
	bool bInsufficientChargeMessageShown;
	uint8_t padding86D[0x3];
	float fDropAngleMod;
	float fLastDropAngleMod;
	CharacterProgression CharacterProgressionInfo;
	BSSimpleList<PerkRankData*> Perks;
	BSSimpleList<BGSPerkEntry*> PerkEntryLists[74];
	BSSimpleList<PerkRankData*> CompanionPerks;
	BSSimpleList<BGSPerkEntry*> CompanionPerkEntryLists[74];
	Actor* pAutoAimActor;
	NiPoint3 BulletAutoAim;
	NiPointer<NiAVObject> spTargeted3D;
	bool bTarget3DDistant;
	uint8_t paddingD41[0x3];
	Actor* pPlayersTargetActor;
	BSSimpleList<CompassTarget*>* pListofPercievedActors;
	float fMenuModeButtonTimer;
	float fAmmoSwapButtonTimer;
	bool bMenuModeButtonClicked;
	uint8_t paddingD55[0x3];
	NiPoint3 kCamera3rdPersonShoulderOffset;
	CombatGroup* pCombatGroup;
	uint32_t iTeammateCount;
	float fCombatTimer;
	float fYieldTimer;
	// The wobble nodes: [0] the third-person ones by weapon type, [1] the first-person ones by animation group
	// weapon.
	NiAVObject* pWobbleNodes[2][12];
	NiPoint3 Cached1stPersonCameraPos;
	NiPoint3 CachedWorldCameraPos;
	// The rigid body that keeps the camera out of the geometry.
	NiPointer<bhkRigidBody> spCameraRigidBody;
	bool bPlayerInCombat;
	bool bAllCombatTargetsSearching;
	bool bMegatonDestroyed;
	uint8_t paddingDF3[0x1];
	BSSimpleArray<ItemChange*> RockItLauncherAmmoList;
	float fRockItLauncherAmmoWeight;
	bool bNightVisionOn;
	uint8_t paddingE09[0x3];
	TESReputation* pReputationUpdate;
	uint8_t cSloMoCameraHits;
	uint8_t paddingE11[0x3];
	float fTimeSinceLastAttack;
	float fTimeInSlowMoCam;
	float fKillCamCooldown;
	bool bIgnoresGTM;
	bool bTurboISM;
	uint8_t paddingE22[0x2];
	float fLastHelloTime;
	// GetBlockCounterTimer reads it.
	float fCounterAttackTimer;
	bool bCounterAttackCamera;
	bool bHasCateyeActive;
	bool bHasSpotterActive;
	uint8_t paddingE2F[0x1];
	float fCheckForItems;
	NiAVObject* m_pWeaponSightingNode;
	bool bAlwaysHardcore;
	bool bResetHardcoreTimers;
	uint8_t paddingE3A[2];
	BSSimpleArray<TESAmmo*, 1024> HotKeyLastAmmo;
	// The quest LoadingMenu::Destruct enables and clears again (TESQuest::SetEnabled).
	TESQuest* patch04DebugQuest;

	// Guards pListofPercievedActors: AI thread 2 (RunDetectionForAllActors) adds and frees entries while the main
	// thread (MediaLocationController::Update, after Swap) and AI thread 1 walk the list, so a walker could read a
	// freed CompassTarget. Every function that walks or changes the list at run time takes it; the loops of SaveGame
	// and the two load passes, which run with the AI threads stopped, do not. Recursive.
	static BSSpinLock kPerceivedActorsLock;
	// Whether the death message is pending.
	static inline bool bShowDeathMessage;
	static inline bool bReleaseTheArrow;
	static inline bool bVanityMode;
	static inline float fsaveSitHeadingDelta;
	static inline bool bIsAutoVanityMode;
	static inline bool bGodMode;
	static inline bool bDemigodMode;
	static inline float fAutoVanityModeDistRevert;
	static inline bool bZoom3rdPersonCamera;
	static inline bool bSnapChaseCamera;
	static inline float fAutoVanityDelay;
	static inline float fVanityDelay;
	static NiPoint3 kVanityModePos;
	static inline bool bSleepRunUpdates = true;
	static inline float fWalkSpeedMult = 1.0f;
	static inline float fPCTurnScale = 1.0f;
	static inline float fCameraLooking;
	static inline float fCameraHeading;
	static inline TESObjectREFR* pPrisonContainer;
	static NiPoint3 kAutoVanityModePos;
	static inline TESObjectCELL* pLastCell;
	static inline float fStartingCameraY;
	static inline float fDrinkDelay;
	static inline float fDialogZoomPercent;
};

enum ACHIEVEMENT_DEF : int32_t {
	ACHIEVEMENT_NONE = -1,
	ACHIEVEMENT_CURBSTOMPER = 0,
	ACHIEVEMENT_SUPERMUTANT_MENACE = 1,
	ACHIEVEMENT_SUPERMUTANT_MASTER = 2,
	ACHIEVEMENT_COUNT = 3,
};
// One achievement's definition; the PlayerCharacter constructor reads cFlags as a signed byte.
struct AchievementData {
	ACHIEVEMENT_DEF AchievementID;
	SettingT<GameSettingCollection>* pName;
	SettingT<GameSettingCollection>* pDescription;
	int32_t iThreshold;
	int32_t iProgress;
	uint32_t nPlatformReward;
	const char* sPerkEditorID;
	int32_t nXPReward;
	ACHIEVEMENT_DEF nRequiredAchievement;
	char cFlags;
};

// The player.
inline PlayerCharacter* pPlayer;

// The vendor PickUpObject found selling the picked-up reference, or null.
inline Actor* pContainerOwner;

inline bool bSkipAlphaFadeOut = true;

// The game settings PlayerCharacter.cpp defines.
extern SettingT<INISettingCollection> fXenonMinLookSpeed;
extern SettingT<INISettingCollection> iJoystickLookLeftRight;
extern SettingT<INISettingCollection> bEssentialTakeNoDamage;
extern SettingT<INISettingCollection> bSetDemigodMode;
extern SettingT<INISettingCollection> iXenonVertLookAccel;
extern SettingT<INISettingCollection> fDlgHeadingDegStop;
extern SettingT<INISettingCollection> fDlgLookAdj;
extern SettingT<INISettingCollection> fDlgLookDegStop;
extern SettingT<INISettingCollection> fUFOCamSpeedMult;
extern SettingT<INISettingCollection> fCameraCasterSize;
extern SettingT<INISettingCollection> fDlgLookDegStart;
extern SettingT<INISettingCollection> fDlgLookMult;
extern SettingT<INISettingCollection> fPlayerCharacterTurnMult;
extern SettingT<INISettingCollection> fIronSightsZoomDefault;
extern SettingT<INISettingCollection> bIronSightsZoomEnable;
extern SettingT<INISettingCollection> bHealthBarShowing;
extern SettingT<INISettingCollection> iJoystickMoveLeftRight;
extern SettingT<INISettingCollection> bInstantLevelUp;
extern SettingT<INISettingCollection> fCameraCasterPlayerSize;
extern SettingT<INISettingCollection> sTrackProgressPath;
extern SettingT<INISettingCollection> bDisableAutoVanityMode;
extern SettingT<INISettingCollection> iActorsDismemberedPerFrame;
extern SettingT<INISettingCollection> iJoystickMoveFrontBack;
extern SettingT<INISettingCollection> bTrackProgress;
extern SettingT<INISettingCollection> bShowGunTarget;
extern SettingT<INISettingCollection> fZoom3rdPersonSnapDist;
extern SettingT<INISettingCollection> iXenonHorizLookAccel;
extern SettingT<INISettingCollection> iHoursToSleep;
extern SettingT<INISettingCollection> iJoystickLookUpDown;
extern SettingT<INISettingCollection> bAllowHavokGrabTheLiving;
extern SettingT<INISettingCollection> fDlgHeadingDegStart;
extern SettingT<INISettingCollection> bAlwaysRunByDefault;
extern SettingT<INISettingCollection> bDisablePlayerCollision;
extern SettingT<INISettingCollection> fDlgFocus;
extern SettingT<INISettingCollection> fForwardBias;

// The achievement definitions, indexed by ACHIEVEMENT_DEF.
using AchievementDataArray = AchievementData[ACHIEVEMENT_COUNT];
inline AchievementDataArray pBaseAchievementData{
	{ACHIEVEMENT_CURBSTOMPER, nullptr, nullptr, 100, 10, 0, nullptr, 100, ACHIEVEMENT_NONE, 0},
	{ACHIEVEMENT_SUPERMUTANT_MENACE, &sAchieveSMMenaceName, &sAchieveSMMenaceDesc, 6, 2, 0, "FastReloadPerk", 2000,
		ACHIEVEMENT_NONE, 0},
	{ACHIEVEMENT_SUPERMUTANT_MASTER, &sAchieveSMMasterName, &sAchieveSMMasterDesc, 3, 1, 0, nullptr, 1000,
		ACHIEVEMENT_SUPERMUTANT_MENACE, 3}};
inline bool bFOVTargetReached = true;
inline float fPlayerDeathReloadTime_0 = -1.0f;

// The event names ExportProgressData's aeEvent indexes ("LevelUp",
// "Death").
using ProgressDataEventNameArray = const char* [2];
inline ProgressDataEventNameArray ProgressDataEventNames{"LevelUp", "Death"};

using data_11A3B4C_t = int32_t[5];
inline data_11A3B4C_t data_11A3B4C{0, 1, 2, 3, 4};
inline float fVATSUserCameraZOffset;
inline NonLinearInterpolator* pLookingInterpolator;
inline NonLinearInterpolator* pHeadingInterpolator;
inline TESObjectREFR* pPrisonMarker;
inline TESWaterForm* pCurrentWater;
inline float fAiControlTimer;
inline bhkRigidBody* pPlayerSphere;
extern NiPick kConflictingPick;
inline int32_t time_11E0BA8;
inline float fAngryFace;
inline int32_t count_11E0BB4;
inline float fzSin;
inline char bWas3rdPersonBeforeVanityMode;
inline bool g_bWasReadyItemHeldLastFrame;
inline int32_t fangryFace_guard;
inline char bQuickLoading;
inline char bQuickSaving;
inline char bDebounce;
inline bool badjustHeading;
inline bool badjustLooking;
inline MobileObject* plastWarningActor;
inline float fzoomedFOV;
inline float ftan;
inline TESObjectREFR* poldVATSTarget;
inline bool bIsGrabbing;
inline NiPick* pConflictingPick;
inline char bPipboyLightDebounce;
inline float fPrevHardcoreTickSLP;
inline float fPrevHardcoreTickFOD;
inline float fPrevHardcoreTickH20;
inline float fHardcoreTimerH20;
inline float fHardcoreTimerFOD;
inline float fHardcoreTimerSLP;
inline char bAntiCheatActive;

// The player's alpha multiplier from the camera; Actor_Magic.cpp overlays it too.
inline float fPlayerCameraAlpha = 1.0f;
inline float f1_0_ = 1.0f;

// The user name the game runs under (a char buffer); empty when unknown.
using userNameG_t = char[0x204];
inline userNameG_t userNameG{};

// The third-person camera's current distance from its target.
inline float fChaseDistance;

// The pitch UpdateArmRotation last gave the first-person arms.
inline float f1stPersonArmRot;

// Set when a spin attack is released before it spun up.
inline bool bSpinAttackReleasedEarly;

// Set when an attack started, cleared when the attack control is let go.
inline bool bUserDebounceBow;

// The attack queued for the next attack key: 1 a normal attack, 2 a power attack.
inline int32_t inextAttack;

// How long the attack control has been held.
inline float fpowerTimer;

// The tick count in seconds when the casino anti-cheat measures last started; 0 while they are off.
inline uint32_t uiAntiCheatTimer;

// Set when the camera caster's pick cut the chase distance short.
inline bool bCameraCasterHitObject;

// The first-person skeleton's camera node, found when the first-person 3D is loaded. It is typed NiAVObject*
// (InterfaceManager.cpp overlays it as a NiCamera*).
inline NiAVObject* p1stPersonCamera;

// The first-person skeleton's Bip node, found when the first-person 3D is loaded.
inline NiAVObject* p1stPersonBip;

// How far the first-person zoom has moved since the view switch began.
inline float fZoomDelta;

// How long the reload control has been held.
inline float fReloadTimer;

// Where the third-person chase camera sat when SetFirstPerson last switched the view.
extern NiPoint3 chaseCameraPos;

extern SettingT<INIPrefSettingCollection> gs_fXenonVertLookSpeed_Controls;

extern SettingT<INISettingCollection> gs_bBorderRegionsEnabled_General;

extern SettingT<INIPrefSettingCollection> gs_fXenonHorizLookSpeed_Controls;

extern SettingT<INIPrefSettingCollection> gs_iDifficulty_GamePlay;

// The first-person gun wobble rotation AddGunWobble last built.
extern NiMatrix3 GunWobbleRot;

extern SettingT<INIPrefSettingCollection> gs_bInvertYValues_Controls;

// The references the player's placement must keep clear of.
extern BSSimpleList<TESObjectREFR*> ConflictingObjects;

extern SettingT<INISettingCollection> gs_fMaxAnalogWalkSpeed_Controls;

// The VATS flags that force the first-person view and put the player back in it afterwards.
inline bool bVATSForce1stPerson;
inline bool bVATSReturnPlayerTo1stPerson;

using std_time_original_t = int64_t (*)(int64_t*);
inline const std_time_original_t std_time_original = &std::time;
