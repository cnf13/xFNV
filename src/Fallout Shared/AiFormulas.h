#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"

#include <stdint.h>

class Actor;
class ActorValueOwner;

// A BSSimpleArray::Sort compare (BSSimpleArrayRoot::CompareFunction,
// the qsort form): orders FollowerDist elements by operator>.
int32_t CompareFollowerDist(const void* a1, const void* a2);

// A follower of a follow target and its distances, for spreading several followers around the target
// (GetFollowRadiusFudgeForMultipleFollowers).
struct FollowerDist {
	// Whether this sorts after aFollowerDist: the larger fPackDesiredDist, then the larger
	// fActualDistSqr.
	bool operator>(const FollowerDist& aFollowerDist) const;

	Actor* pActor;
	float fActualDistSqr;
	float fPackDesiredDist;
	float fActorRadius;
};

// The settings of one sandbox activity: when it starts and how long it lasts.
struct PerActivitySettings {
	PerActivitySettings(SettingT<GameSettingCollection>& apStartMin, SettingT<GameSettingCollection>& apStartMax,
		SettingT<GameSettingCollection>& apDurMin, SettingT<GameSettingCollection>& apDurMax);

	SettingT<GameSettingCollection>* StartMin;
	SettingT<GameSettingCollection>* StartMax;
	SettingT<GameSettingCollection>* DurMin;
	SettingT<GameSettingCollection>* DurMax;
};

// Formulas that derive actor values from other actor values
class AiFormulas {
public:
	static float AVDeriveBodyPartCondition(ActorValueOwner* apActor, ActorValue::Index aiWhich);
	static void AVGetDerivedSkillPrimaryAttributeAndBase(ActorValue::Index aiSkill,
		ActorValue::Index& aiPrimaryAttribute, float& afBase);
	static float AVDeriveCriticalChance(ActorValueOwner* apActor, ActorValue::Index aiWhich);
	static float AVDeriveActionPoints(ActorValueOwner* apActor, ActorValue::Index aiWhich);
	static float AVDeriveHealth(ActorValueOwner* apActor, ActorValue::Index aiWhich);
	static float AVDeriveFatigue(ActorValueOwner* apActor, ActorValue::Index aiWhich);
	static float AVDeriveSkill_0(ActorValueOwner* apOwner, ActorValue::Index aiSkill);
	static float CalculateDerivedSkillValue(float afSkillBase, float afPrimaryAttribute, float afLuck);
	static float AVDeriveResistance(ActorValueOwner* apActor, ActorValue::Index aiWhich);

	static float CalculateDerivedHealth(ActorValueOwner* apActor, float afEndurance);
	// A random hour between afMin and afMax.
	static float RandomHour(float afMin, float afMax);
	static float CalculateDerivedFatigue(ActorValueOwner* apActor, float afEndurance);
	static float AVDeriveSkill(ActorValueOwner* apActor, ActorValue::Index aiWhich, bool abBaseDerivation);
	// The detection size class of an actor afHeightOrLength tall.
	static int32_t GetActorDetectionSize(float afHeightOrLength);
	// The detection formula: the level at which actor A detects actor B; the sound, visual
	// and skill subtotals are returned.
	static int32_t GetActorDetectedTarget(int32_t aiPercepA, int32_t aiSneakB, int32_t aiLineofSite, int32_t aiVisual,
		float afDistance, int32_t aiLightLevelB, int32_t aiBlindess, bool abInvisibilityB, int32_t aiBootweightB,
		bool abMovingB, bool abSneakingB, bool abRunning, bool absleeping, bool abAlert, int32_t aiActionSound,
		int32_t aiTargetSize, bool bExterior, bool abInCombat, int32_t aiAmbushValue, float& afSoundSubtotal,
		float& afVisualSubtotal, float& afSkillSubtotal, const int32_t iLevelA, const int32_t iLevelB,
		const int32_t iArmorB);
	// The disposition formula of an actor towards a target. Every value but abcreature is passed as a 32-bit integer.
	static int32_t GetActorDispositiontoTarget(int32_t aiActorBaseDisp, int32_t aiCharismaTarget, int32_t aiResponActor,
		int32_t aiKarmaTarget, int32_t aiRaceReactionTarget, int32_t aiFactionReactionTarget,
		int32_t aiFactionRankTarget, int32_t aiWeaponStatusTarget, int32_t aiDiseaseStatusTarget, int32_t aiTargetMod,
		bool abcreature);
	// The detection level a detection event reaches: the detecting actor's sneak skill, its line of sight and view cone
	// to the event, the distance, light and action noise.
	static int32_t GetDetectedEvent(int32_t aiPercep, int32_t aiLineofSite, float afDistance, int32_t aiBlindess,
		int32_t aiLightLevel, bool absleeping, bool abAlert, bool abInCombat, int32_t aiActionSound, int32_t aiVisual,
		bool abExterior);
	// The larger of the radius less fFollowSlowdownZoneWidth and the radius times fFollowStopZoneMinMult.
	static float GetFollowRadiusStop(float afbasePackRadius);
	static float GetFollowRadiusSlow(float afbasePackRadius);
	static float AVDeriveCarryWeight(ActorValueOwner* apActor, ActorValue::Index aiWhich);
	static float AVDeriveFromPrimary(ActorValueOwner* apActor, ActorValue::Index aiWhich);
	static float GetFollowRadiusWalk(float afbasePackRadius);
	static float GetFollowRadiusStartMoving(float afbasePackRadius);
	static float GetFollowRadiusMatchSpeed(float afbasePackRadius);
	static int32_t ComputePickpocketSuccess(int32_t aiSneakSkillModed, int32_t aiTargetSneakModed, float fAmountofItem);
	static void ComputeRandomSandboxActivityStartAndDuration(int32_t aiActivity, float& afStartOut,
		float& afDurationOut);
	static float GetFollowRadiusFudgeForMultipleFollowers(Actor* apActor, Actor* apFollowTarget);
	static float ComputeRandomSandboxSubProcessDuration(int32_t aiEngergyLevel, int32_t aiProcIndex);
	static float AVDeriveHealingRate(ActorValueOwner* apActor, ActorValue::Index aiWhich);
	static float AVDeriveUnarmedDamage(ActorValueOwner* apActor, ActorValue::Index aiWhich);
	static double ComputeTimeTillNextWander(int32_t a1);
};

// The settings of the four sandbox activities (sleep, breakfast, lunch, dinner), filled in at startup.
using SandboxActivitySettingsArray = PerActivitySettings[4];
extern SandboxActivitySettingsArray SandboxActivitySettings;

// The primary attribute of each of the 14 skills, BARTER first.
using SkillAttributeArray14 = ActorValue::Index[14];
inline SkillAttributeArray14 SkillToSpecial{ActorValue::CHARISMA, ActorValue::ENDURANCE, ActorValue::PERCEPTION,
	ActorValue::PERCEPTION, ActorValue::PERCEPTION, ActorValue::INTELLIGENCE, ActorValue::STRENGTH,
	ActorValue::INTELLIGENCE, ActorValue::INTELLIGENCE, ActorValue::AGILITY, ActorValue::AGILITY, ActorValue::CHARISMA,
	ActorValue::ENDURANCE, ActorValue::ENDURANCE};

// The base value setting of each of the 14 skills, BARTER first.
using SettingPointerArray14 = Setting* [14];
extern SettingPointerArray14 fAVDSkillBaseValues;

// The settings the sandbox package's sub-process duration is scaled by, by sub-process index.
using SandboxSettingTable = Setting* [6];
extern SandboxSettingTable fSandboxDurationMultProc;
extern SandboxSettingTable fSandboxEnergyMultProc;

// The game settings AiFormulas.cpp defines.
extern SettingT<GameSettingCollection> fVanityModeWheelDefault;
extern SettingT<GameSettingCollection> fSandBoxExtraDialogueRange;
extern SettingT<GameSettingCollection> fPickUpWeaponDelay;
extern SettingT<GameSettingCollection> fVanityModeAutoDelay;
extern SettingT<GameSettingCollection> fAVDTagSkillBonus;
extern SettingT<GameSettingCollection> fSandboxSleepDurationMax;
extern SettingT<GameSettingCollection> fDyingTimer;
extern SettingT<GameSettingCollection> iBoneLODDistMult;
extern SettingT<GameSettingCollection> fShaderShadowUpdateDistance;
extern SettingT<GameSettingCollection> fFightAggrMult;
extern SettingT<GameSettingCollection> fAVDSkillSpeechBase;
extern SettingT<GameSettingCollection> fDispActorCharBase;
extern SettingT<GameSettingCollection> fAiAcquireStealMult;
extern SettingT<GameSettingCollection> fFirstPersonZoomMinMult;
extern SettingT<GameSettingCollection> fDispTargetKarmaMult;
extern SettingT<GameSettingCollection> fAISpectatorCommentTimer;
extern SettingT<GameSettingCollection> iNumberActorsInCombatPlayer;
extern SettingT<GameSettingCollection> fIdleChatterCommentTimer;
extern SettingT<GameSettingCollection> fDispTargetCharOffset;
extern SettingT<GameSettingCollection> fSandboxEnergyMultIdleMarker;
extern SettingT<GameSettingCollection> fAISocialchanceForConversationInterior;
extern SettingT<GameSettingCollection> fAISocialTimerForConversationsMax;
extern SettingT<GameSettingCollection> fAVDSkillLockpickBase;
extern SettingT<GameSettingCollection> iActorTorsoMaxRotation;
extern SettingT<GameSettingCollection> fBowHoldTimer;
extern SettingT<GameSettingCollection> fDistanceInteriorReactCombat;
extern SettingT<GameSettingCollection> iLevelsPerPerk;
extern SettingT<GameSettingCollection> fSneakActionMult;
extern SettingT<GameSettingCollection> fSneakLightMult;
extern SettingT<GameSettingCollection> fPlayerPipBoyLightTimer;
extern SettingT<GameSettingCollection> iMinSandboxRescanSeconds;
extern SettingT<GameSettingCollection> fFightFriendDispBase;
extern SettingT<GameSettingCollection> fAIDefaultSpeechMult;
extern SettingT<GameSettingCollection> fPickSpring1;
extern SettingT<GameSettingCollection> fSandboxDinnerMin;
extern SettingT<GameSettingCollection> fAVDSkillSmallGunsBase;
extern SettingT<GameSettingCollection> fSpellCastingDetectionHitActorMod;
extern SettingT<GameSettingCollection> fCharacterDefaultTurningSpeed;
extern SettingT<GameSettingCollection> fAiAcquireKillMult;
extern SettingT<GameSettingCollection> fAIBestHeadTrackDistance;
extern SettingT<GameSettingCollection> fAVDHealthEnduranceOffset;
extern SettingT<GameSettingCollection> fAVDMeleeDamageStrengthMult;
extern SettingT<GameSettingCollection> iSneakLevelBonus;
extern SettingT<GameSettingCollection> fSneakRunningMult;
extern SettingT<GameSettingCollection> fPlayerAmmoSwapTimer;
extern SettingT<GameSettingCollection> fKarmaModMurderingNonEvilCreature;
extern SettingT<GameSettingCollection> fOverShoulderOffsetPointZoomingZ;
extern SettingT<GameSettingCollection> fLockPickQualityMult;
extern SettingT<GameSettingCollection> fIdleChatterCommentTimerMax;
extern SettingT<GameSettingCollection> fVanityModeAutoYSpeed;
extern SettingT<GameSettingCollection> fFightDispBase;
extern SettingT<GameSettingCollection> fLockPickBreakBase;
extern SettingT<GameSettingCollection> fPickLevelMult;
extern SettingT<GameSettingCollection> fAIEscortHysteresisWidth;
extern SettingT<GameSettingCollection> fSandboxDurationRangeMult;
extern SettingT<GameSettingCollection> fSneakAmbushNonTargetMod;
extern SettingT<GameSettingCollection> iDispKaramMax;
extern SettingT<GameSettingCollection> fGuardPackageAttackRadiusMult;
extern SettingT<GameSettingCollection> iFriendHitCombatAllowed;
extern SettingT<GameSettingCollection> fAIAwareofPlayerTimer;
extern SettingT<GameSettingCollection> fLockSkillMult;
extern SettingT<GameSettingCollection> fDispActorRespBase;
extern SettingT<GameSettingCollection> fLockPickBreakMult;
extern SettingT<GameSettingCollection> fDetectionLargeActorThreshold;
extern SettingT<GameSettingCollection> fAIFindBedChairsDistance;
extern SettingT<GameSettingCollection> fSneakBootWeightBase;
extern SettingT<GameSettingCollection> fAISpectatorRememberThreatTimer;
extern SettingT<GameSettingCollection> fAVDHealthEnduranceMult;
extern SettingT<GameSettingCollection> fAVDNPCHealthEnduranceOffset;
extern SettingT<GameSettingCollection> fChase3rdPersonZUnitsPerSecond;
extern SettingT<GameSettingCollection> fActorTurnAnimMinTime;
extern SettingT<GameSettingCollection> fSandboxDinnerMax;
extern SettingT<GameSettingCollection> fVanityModeWheelOutMult;
extern SettingT<GameSettingCollection> fDispTargetFactionRankMult;
extern SettingT<GameSettingCollection> fAITrespassWarningTimer;
extern SettingT<GameSettingCollection> fDispTargetWeaponBase;
extern SettingT<GameSettingCollection> fSneakLightRunMult;
extern SettingT<GameSettingCollection> fSneakSoundLosMult;
extern SettingT<GameSettingCollection> fAIMaxHeadTrackDistanceFromPC;
extern SettingT<GameSettingCollection> fSneakDistanceAttenuationExponent;
extern SettingT<GameSettingCollection> fSneakStealthBoyMult;
extern SettingT<GameSettingCollection> fOverShoulderOffsetPointZ;
extern SettingT<GameSettingCollection> fAISocialTimerForConversationsMin;
extern SettingT<GameSettingCollection> fPickSpring3;
extern SettingT<GameSettingCollection> fFleeIsSafeTimer;
extern SettingT<GameSettingCollection> fFollowExtraCatchUpSpeedMult;
extern SettingT<GameSettingCollection> fAVDHealRateEndurance7Bonus;
extern SettingT<GameSettingCollection> fSandboxSleepDurationMin;
extern SettingT<GameSettingCollection> iSpeakSoundLipDistance;
extern SettingT<GameSettingCollection> fDispActorCharMult;
extern SettingT<GameSettingCollection> fSandboxDurationMultEating;
extern SettingT<GameSettingCollection> iCoolDownTimerSetting;
extern SettingT<GameSettingCollection> fAVDUnarmedDamageBase;
extern SettingT<GameSettingCollection> iSoundLevelSilent;
extern SettingT<GameSettingCollection> fFightDispMult;
extern SettingT<GameSettingCollection> fDetectionCommentTimer;
extern SettingT<GameSettingCollection> fChase3rdPersonVanityXYMult;
extern SettingT<GameSettingCollection> fFightDistanceBase;
extern SettingT<GameSettingCollection> fSandBoxSearchRadius;
extern SettingT<GameSettingCollection> fMountedMaxLookingDown;
extern SettingT<GameSettingCollection> fPickSpring4;
extern SettingT<GameSettingCollection> fAIMinGreetingDistance;
extern SettingT<GameSettingCollection> fFollowStopZoneMinMult;
extern SettingT<GameSettingCollection> fPCTurnAnimDeltaThreshold;
extern SettingT<GameSettingCollection> fDispTargetDiseaseBase;
extern SettingT<GameSettingCollection> fVanityModeYMult;
extern SettingT<GameSettingCollection> fAIGreetingTimer;
extern SettingT<GameSettingCollection> fSneakNoticedMin;
extern SettingT<GameSettingCollection> iAllyHitNonCombatAllowed;
extern SettingT<GameSettingCollection> fAVDSkillEnergyWeaponsBase;
extern SettingT<GameSettingCollection> iActorTurnDegree;
extern SettingT<GameSettingCollection> fPickPocketTargetSkillBase;
extern SettingT<GameSettingCollection> fVanityModeForceDefault;
extern SettingT<GameSettingCollection> fLockTrapGoOffBase;
extern SettingT<GameSettingCollection> iAllyHitCombatAllowed;
extern SettingT<GameSettingCollection> iSneakStartBonusLevelPenatly;
extern SettingT<GameSettingCollection> fVanityModeAutoXSpeed;
extern SettingT<GameSettingCollection> fAVDHealRateEndurance6Bonus;
extern SettingT<GameSettingCollection> fAVDSkillMeleeWeaponsBase;
extern SettingT<GameSettingCollection> fDetectionActionTimer;
extern SettingT<GameSettingCollection> fKarmaModKillingVeryEvilActor;
extern SettingT<GameSettingCollection> fDistancetoPlayerforConversations;
extern SettingT<GameSettingCollection> fAiAcquirePickMult;
extern SettingT<GameSettingCollection> iInventoryMenuIdleDelay;
extern SettingT<GameSettingCollection> iSoundLevelNormal;
extern SettingT<GameSettingCollection> fAVDNPCHealthEnduranceMult;
extern SettingT<GameSettingCollection> fAIEnergyLevelBase;
extern SettingT<GameSettingCollection> fAmbushOverRideRadiusforPlayerDetection;
extern SettingT<GameSettingCollection> fChaseCameraMax;
extern SettingT<GameSettingCollection> fAIRadiusToRunDetectionExterior;
extern SettingT<GameSettingCollection> fFriendHitTimer;
extern SettingT<GameSettingCollection> fSandboxSleepStartMax;
extern SettingT<GameSettingCollection> fSandboxDurationMultIdleMarker;
extern SettingT<GameSettingCollection> fAVDActionPointsBase;
extern SettingT<GameSettingCollection> iFriendHitNonCombatAllowed;
extern SettingT<GameSettingCollection> fAVDSkillRepairBase;
extern SettingT<GameSettingCollection> fDrinkRepeatRate;
extern SettingT<GameSettingCollection> fAIPursueDistanceLineOfSight;
extern SettingT<GameSettingCollection> fAVDSkillUnarmedBase;
extern SettingT<GameSettingCollection> fFollowRunMinSlowdownMultiplier;
extern SettingT<GameSettingCollection> fWarningTimer;
extern SettingT<GameSettingCollection> fSandboxEnergyMultWandering;
extern SettingT<GameSettingCollection> fVanityModeWheelDeadMin;
extern SettingT<GameSettingCollection> fFirstPersonHandChaseSeconds;
extern SettingT<GameSettingCollection> fPlayerDeathReloadTime;
extern SettingT<GameSettingCollection> fAVDFatigueBase;
extern SettingT<GameSettingCollection> iKarmaMax;
extern SettingT<GameSettingCollection> fSneakSizeBase;
extern SettingT<GameSettingCollection> fFollowSpaceBetweenFollowers;
extern SettingT<GameSettingCollection> fDetectionViewCone;
extern SettingT<GameSettingCollection> fAIIdleAnimationDistance;
extern SettingT<GameSettingCollection> iSkillPointsTagSkillMult;
extern SettingT<GameSettingCollection> fCombatRoundAmount;
extern SettingT<GameSettingCollection> fSearchPackageTimer;
extern SettingT<GameSettingCollection> fAVDHealRateEndurance9Bonus;
extern SettingT<GameSettingCollection> iAIFriendlyHitMinDisposition;
extern SettingT<GameSettingCollection> iStealWarnings;
extern SettingT<GameSettingCollection> fSandboxSleepStartMin;
extern SettingT<GameSettingCollection> iHighResponsibility;
extern SettingT<GameSettingCollection> iDistancetoAttackedTarget;
extern SettingT<GameSettingCollection> fLockTrapGoOffMult;
extern SettingT<GameSettingCollection> fAiAcquireStealBase;
extern SettingT<GameSettingCollection> fFollowRunMaxSpeedupMultiplier;
extern SettingT<GameSettingCollection> fAIMaxSmileDistance;
extern SettingT<GameSettingCollection> fAISocialRadiusToTriggerConversationInterior;
extern SettingT<GameSettingCollection> fOverShoulderOffsetPointZoomingX;
extern SettingT<GameSettingCollection> fFriendMinimumLastHitTime;
extern SettingT<GameSettingCollection> fDistanceProjectileExplosionDetection;
extern SettingT<GameSettingCollection> fKarmaModMurderingGoodNPC;
extern SettingT<GameSettingCollection> fAVDFatigueEnduranceMult;
extern SettingT<GameSettingCollection> fAVDActionPointsMult;
extern SettingT<GameSettingCollection> fPlayerTeleportFadeSeconds;
extern SettingT<GameSettingCollection> fSneakMaxDistance;
extern SettingT<GameSettingCollection> fSneakBaseValue;
extern SettingT<GameSettingCollection> fLockLevelBase;
extern SettingT<GameSettingCollection> iNumberGuardsCrimeResponse;
extern SettingT<GameSettingCollection> fAIInteriorSpectatorDistance;
extern SettingT<GameSettingCollection> fAVDSkillSneakBase;
extern SettingT<GameSettingCollection> fAIEscortWaitDistanceExterior;
extern SettingT<GameSettingCollection> iChanceFleeDialog;
extern SettingT<GameSettingCollection> fOverShoulderStartBlendDist;
extern SettingT<GameSettingCollection> iSneakSkillUseDistance;
extern SettingT<GameSettingCollection> fFollowMatchSpeedZoneWidth;
extern SettingT<GameSettingCollection> fAiAcquirePickBase;
extern SettingT<GameSettingCollection> fRadiationAvoidThreshold;
extern SettingT<GameSettingCollection> fPickSpring5;
extern SettingT<GameSettingCollection> fDispTargetFactionRankBase;
extern SettingT<GameSettingCollection> fAVDMeleeDamageStrengthOffset;
extern SettingT<GameSettingCollection> fPickPocketAmountMult;
extern SettingT<GameSettingCollection> fPickSpring2;
extern SettingT<GameSettingCollection> iSneakStartBonus;
extern SettingT<GameSettingCollection> fSittingMaxLookingDown;
extern SettingT<GameSettingCollection> fActorAlertSoundTimer;
extern SettingT<GameSettingCollection> fPickPocketActorSkillMult;
extern SettingT<GameSettingCollection> fSortActorDistanceListTimer;
extern SettingT<GameSettingCollection> fFightAggrBase;
extern SettingT<GameSettingCollection> fAVDNPCHealthLevelMult;
extern SettingT<GameSettingCollection> fCrimeSoundMult;
extern SettingT<GameSettingCollection> fAIInteriorHeadTrackMult;
extern SettingT<GameSettingCollection> fAISocialRadiusToTriggerConversation;
extern SettingT<GameSettingCollection> fAIMaxHeadTrackDistance;
extern SettingT<GameSettingCollection> fSandboxDurationMultWandering;
extern SettingT<GameSettingCollection> fAIMoveDistanceToRecalcFollowPath;
extern SettingT<GameSettingCollection> fCrimeSoundBase;
extern SettingT<GameSettingCollection> fAIConversationExploreTime;
extern SettingT<GameSettingCollection> fAVDNPCFatigueBase;
extern SettingT<GameSettingCollection> fVanityModeAutoYDegrees;
extern SettingT<GameSettingCollection> fAIExteriorSpectatorDistance;
extern SettingT<GameSettingCollection> fSandboxBreakfastMax;
extern SettingT<GameSettingCollection> fSandboxLunchMin;
extern SettingT<GameSettingCollection> fFollowWalkMaxSpeedupMultiplier;
extern SettingT<GameSettingCollection> fSandboxMealDurationMin;
extern SettingT<GameSettingCollection> fDistanceExteriorReactCombat;
extern SettingT<GameSettingCollection> fAVDSkillMedicineBase;
extern SettingT<GameSettingCollection> fPickLevelBase;
extern SettingT<GameSettingCollection> iPickPocketWarnings;
extern SettingT<GameSettingCollection> fSneakSkillMult;
extern SettingT<GameSettingCollection> fAVDHealRateEndurance8Bonus;
extern SettingT<GameSettingCollection> iActorKeepTurnDegree;
extern SettingT<GameSettingCollection> fFirstPersonCameraMult;
extern SettingT<GameSettingCollection> fOverShoulderOffsetPointX;
extern SettingT<GameSettingCollection> fPickPocketMaxChance;
extern SettingT<GameSettingCollection> fSneakLostMin;
extern SettingT<GameSettingCollection> fOverShoulderFOV;
extern SettingT<GameSettingCollection> fReEquipArmorTime;
extern SettingT<GameSettingCollection> fAVDDefaultBodyPartCondition;
extern SettingT<GameSettingCollection> fSearchPackageDistanceToTarget;
extern SettingT<GameSettingCollection> fPickPocketAmountBase;
extern SettingT<GameSettingCollection> fVanityModeWheelMin;
extern SettingT<GameSettingCollection> fPerceptionMult;
extern SettingT<GameSettingCollection> fAVDUnarmedDamageMult;
extern SettingT<GameSettingCollection> fActorTeleportFadeSeconds;
extern SettingT<GameSettingCollection> fPickNumMult;
extern SettingT<GameSettingCollection> iNumberActorsAllowedToFollowPlayer;
extern SettingT<GameSettingCollection> fAVDFatigueLevelMult;
extern SettingT<GameSettingCollection> fAVDSkillSurvivalBase;
extern SettingT<GameSettingCollection> iGuardWarnings;
extern SettingT<GameSettingCollection> fCheckDeadBodyTimer;
extern SettingT<GameSettingCollection> fAIMaxWanderTime;
extern SettingT<GameSettingCollection> fSandboxMealDurationMax;
extern SettingT<GameSettingCollection> fAVDSkillBarterBase;
extern SettingT<GameSettingCollection> fAVDSkillPrimaryBonusMult;
extern SettingT<GameSettingCollection> iSandBoxPreventRepeatedActionTime;
extern SettingT<GameSettingCollection> fAVDCritLuckMult;
extern SettingT<GameSettingCollection> fAVDNPCFatigueLevelMult;
extern SettingT<GameSettingCollection> fPickNumBase;
extern SettingT<GameSettingCollection> fLockLevelMult;
extern SettingT<GameSettingCollection> fRemoteCombatMissedAttack;
extern SettingT<GameSettingCollection> fDetectionSneakLightMod;
extern SettingT<GameSettingCollection> fSandboxEnergyMultFurniture;
extern SettingT<GameSettingCollection> fFightDistanceMult;
extern SettingT<GameSettingCollection> fAIEscortWaitDistanceInterior;
extern SettingT<GameSettingCollection> fAIHoldDefaultHeadTrackTimer;
extern SettingT<GameSettingCollection> fVanityModeXBoxXMult;
extern SettingT<GameSettingCollection> fSneakCombatMod;
extern SettingT<GameSettingCollection> fAItalktoNPCtimer;
extern SettingT<GameSettingCollection> iCrimeAlarmRecDistance;
extern SettingT<GameSettingCollection> fSneakAmbushTargetMod;
extern SettingT<GameSettingCollection> fAISocialchanceForConversation;
extern SettingT<GameSettingCollection> fSandboxEnergyMult;
extern SettingT<GameSettingCollection> fCrimeAlarmRespMult;
extern SettingT<GameSettingCollection> fDispActorRespMult;
extern SettingT<GameSettingCollection> fLockPickQualityBase;
extern SettingT<GameSettingCollection> fOverShoulderPosZ;
extern SettingT<GameSettingCollection> fSandboxEnergyMultEating;
extern SettingT<GameSettingCollection> fOverShoulderPosX;
extern SettingT<GameSettingCollection> fAiAcquireKillBase;
extern SettingT<GameSettingCollection> fSandboxDurationBase;
extern SettingT<GameSettingCollection> fSneakBootWeightMult;
extern SettingT<GameSettingCollection> fTakeBackTimerSetting;
extern SettingT<GameSettingCollection> fVanityModeWheelInMult;
extern SettingT<GameSettingCollection> fKarmaModMurderingVeryGoodNPC;
extern SettingT<GameSettingCollection> fFirstPersonHandFollowMult;
extern SettingT<GameSettingCollection> fSneakExteriorDistanceMult;
extern SettingT<GameSettingCollection> fRadiationResistanceAvoidLevel;
extern SettingT<GameSettingCollection> fAItalktosameNPCtimer;
extern SettingT<GameSettingCollection> fAVDRadResistEnduranceMult;
extern SettingT<GameSettingCollection> fAVDSkillBigGunsBase;
extern SettingT<GameSettingCollection> fAVDCarryWeightMult;
extern SettingT<GameSettingCollection> fAIFurnitureDestinationRadius;
extern SettingT<GameSettingCollection> fSneakLightMoveMult;
extern SettingT<GameSettingCollection> fFightAbleToDetectTimer;
extern SettingT<GameSettingCollection> fAIIdleWaitTime;
extern SettingT<GameSettingCollection> fAIUseWeaponDistance;
extern SettingT<GameSettingCollection> fSpellCastingDetectionMod;
extern SettingT<GameSettingCollection> fAVDHealRateEndurance10Bonus;
extern SettingT<GameSettingCollection> fFirstPersonZoomMaxMult;
extern SettingT<GameSettingCollection> iKarmaMin;
extern SettingT<GameSettingCollection> sFirstPersonSkeleton;
extern SettingT<GameSettingCollection> fAVDSkillLuckBonusMult;
extern SettingT<GameSettingCollection> fVanityModeXMult;
extern SettingT<GameSettingCollection> fSneakPerceptionSkillMax;
extern SettingT<GameSettingCollection> fPowerAttackDelay;
extern SettingT<GameSettingCollection> fAIInteriorSpectatorDetection;
extern SettingT<GameSettingCollection> fDispTargetFactionMult;
extern SettingT<GameSettingCollection> fFightFriendDispMult;
extern SettingT<GameSettingCollection> iSoundLevelLoud;
extern SettingT<GameSettingCollection> fDispTargetCharBase;
extern SettingT<GameSettingCollection> fOverShoulderRotMult;
extern SettingT<GameSettingCollection> fVanityModeWheelMax;
extern SettingT<GameSettingCollection> fKarmaModStealing;
extern SettingT<GameSettingCollection> iAINumberActorsComplexScene;
extern SettingT<GameSettingCollection> fFirstPersonHandChaseSecondsAttack;
extern SettingT<GameSettingCollection> fSandboxLunchMax;
extern SettingT<GameSettingCollection> fAVDRadResistEnduranceOffset;
extern SettingT<GameSettingCollection> fSneakUnseenMin;
extern SettingT<GameSettingCollection> fAVDSkillExplosivesBase;
extern SettingT<GameSettingCollection> fFollowSlowdownZoneWidth;
extern SettingT<GameSettingCollection> fAIRadiusToRunDetectionInterior;
extern SettingT<GameSettingCollection> fVanityModeDelay;
extern SettingT<GameSettingCollection> iAIDistanceRadiusMinLocation;
extern SettingT<GameSettingCollection> fAVDHealthLevelMult;
extern SettingT<GameSettingCollection> fSandboxBreakfastMin;
extern SettingT<GameSettingCollection> fAIAcquireObjectDistance;
extern SettingT<GameSettingCollection> fPickPocketTargetSkillMult;
extern SettingT<GameSettingCollection> fKarmaModKillingEvilActor;
extern SettingT<GameSettingCollection> fAVDCritLuckBase;
extern SettingT<GameSettingCollection> fDetectionLargeActorSizeMult;
extern SettingT<GameSettingCollection> fPickPocketActorSkillBase;
extern SettingT<GameSettingCollection> fSneakPerceptionSkillMin;
extern SettingT<GameSettingCollection> fSneakSeenMin;
extern SettingT<GameSettingCollection> fAVDSkillScienceBase;
extern SettingT<GameSettingCollection> fPCTurnAnimMinTime;
extern SettingT<GameSettingCollection> fDispTargetCharMult;
extern SettingT<GameSettingCollection> fAVDPoisonResistEnduranceMult;
extern SettingT<GameSettingCollection> fOverShoulderOffsetPointZoomingY;
extern SettingT<GameSettingCollection> iMaxSandboxRescanSeconds;
extern SettingT<GameSettingCollection> fKarmaModMurderingNonEvilNPC;
extern SettingT<GameSettingCollection> fAVDPoisonResistEnduranceOffset;
extern SettingT<GameSettingCollection> fObjectWeightPickupDetectionMult;
extern SettingT<GameSettingCollection> fSandboxDurationMultFurniture;
extern SettingT<GameSettingCollection> fFollowWalkMinSlowdownMultiplier;
extern SettingT<GameSettingCollection> fDispTargetRaceMult;
extern SettingT<GameSettingCollection> fOverShoulderOffsetPointY;
extern SettingT<GameSettingCollection> fSneakAlertMod;
extern SettingT<GameSettingCollection> fPlayerWeaponReloadTimer;
extern SettingT<GameSettingCollection> iAlertAgressionMin;
extern SettingT<GameSettingCollection> fAIExteriorSpectatorDetection;
extern SettingT<GameSettingCollection> fPickPocketMinChance;
extern SettingT<GameSettingCollection> fFollowWalkZoneMult;
extern SettingT<GameSettingCollection> fAVDNPCFatigueEnduranceMult;
extern SettingT<GameSettingCollection> fSneakSoundsMult;

extern SettingT<GameSettingCollection> gs_fSneakSleepBonus;

extern SettingT<GameSettingCollection> gs_fAIEngergyLevelMult;
