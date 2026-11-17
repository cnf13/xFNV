#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"

class ActorValueOwner;
class TESObjectARMO;
class TESObjectWEAP;

// The gameplay formulas: experience, barter, health and movement values derived from game settings.
class GamePlayFormulas {
public:
    // Whether hardcore mode is on.
    enum HardcoreSetting : int32_t {
        HARDCORE_OFF = 0,
        HARDCORE_ON = 1,
    };

    // The game's difficulty; LOW and HIGH name the ends.
    enum DifficultyLevel : int32_t {
        DIFFICULTY_VERY_EASY = 0,
        DIFFICULTY_LOW = 0,
        DIFFICULTY_EASY = 1,
        DIFFICULTY_NORMAL = 2,
        DIFFICULTY_HARD = 3,
        DIFFICULTY_VERY_HARD = 4,
        DIFFICULTY_HIGH = 4,
    };

    // The movement speed multiplier of an actor.
    static float GetActorWalkSpeed(
        ActorValueOwner* apActor,
        TESObjectWEAP* pWeap,
        TESObjectARMO* pArmor,
        bool abSneaking,
        bool abNoWeapon,
        bool abIsNpc,
        bool abOverEncumbered
    );
    // The skill points gained per level for aiIntelligence; an odd intelligence earns one
    // more on even levels.
    static int32_t GetLevelUpSkillPoints(int32_t aiIntelligence, int32_t aiLevel);
    // The difficulty multiplier aiWhich (16 hit points, 24 experience; 1 otherwise) for aeDifficulty, the hit point one
    // to or by the player per abToPC.
    static float
    GetDifficultyMultiplier(GamePlayFormulas::DifficultyLevel aeDifficulty, ActorValue::Index aiWhich, bool abToPC);
    // afXP scaled by the experience multiplier of aeDifficulty.
    static float AdjustExperiencePoints(float afXP, GamePlayFormulas::DifficultyLevel aeDifficulty);
    // The experience points needed to reach the level; 0 for level 1 and below.
    static int32_t GetRequiredExperiencePoints(int32_t aiWhichLevel);
    // The run speed of apActor with the given weapon and armor and movement state.
    static float GetActorRunSpeed(
        ActorValueOwner* apActor,
        TESObjectWEAP* pWeap,
        TESObjectARMO* pArmor,
        bool abNoWeapon,
        bool abIsNpc,
        bool abIsSneaking,
        bool abOverEncumbered
    );
    static float GetActorClothingDesirability(uint32_t aiClothingValue);
    static float PainChangePerSecond(float afDamagePercent);
    // afRadiation scaled by how deep afDistance lies inside afRadius, clamped to [0, 1].
    static float GetRadiationLevel(float afRadiation, float afRadius, float afDistance);
    // The value of an item worth afItemValue at afItemHealthPercent health.
    static float CalcItemHealthValue(float afItemValue, float afItemHealthPercent);
    // The condition (times 10) repairing one item with another reaches at aiRepairSkill; when the result does not beat
    // the better condition, *apiMinSkillNeeded gets the skill needed to improve it.
    static float
    GetRepairedCondition(int32_t aiRepairSkill, float afCondition1, float afCondition2, int32_t* apiMinSkillNeeded);
    // The caps a vendor with aiVendorRepairSkill charges to repair an item of afValue at afCondition.
    static int32_t GetRepairCost(int32_t aiVendorRepairSkill, float afCondition, float afValue);
    static float SwimBreathTime(int32_t aiEndurance);
    // The health afHealingRate (per hour) returns per second; sleeping adds the sleeping base rate and scales the
    // waking rate by the sleeping multiplier.
    static float GetHealingRate(float afHealingRate, bool abIsSleeping);
    // The magnitude and duration of a potion effect of afEffectCost at afPotionStrength, raised by the alchemy
    // equipment aiEquipment of the given qualities.
    static void CalculatePotionDurationMagnitude(
        float& aiMagnitude,
        float& aiDuration,
        float afEffectCost,
        float afPotionStrength,
        int32_t aiEquipment,
        bool bhostile,
        float afRetortQuality,
        float afAlembicQuality,
        float afCalcinatorQuality,
        bool abPoison
    );
    // The duration alone of a potion effect, as CalculatePotionDurationMagnitude.
    static void CalculatePotionDuration(
        float& aiDuration,
        float afEffectCost,
        float afPotionStrength,
        int32_t aiEquipment,
        bool bhostile,
        float afRetortQuality,
        float afAlembicQuality,
        float afCalcinatorQuality,
        bool abPoison
    );
    // The magnitude alone of a potion effect, as CalculatePotionDurationMagnitude.
    static void CalculatePotionMagnitude(
        float& aiMagnitude,
        float afEffectCost,
        float afPotionStrength,
        int32_t aiEquipment,
        bool bhostile,
        float afRetortQuality,
        float afAlembicQuality,
        float afCalcinatorQuality,
        bool abPoison
    );
    // The chance in percent, clamped to [0, 100], to force a lock of aiLockLevelMax with aiPlayerSkill.
    static int32_t GetForceLockChance(int32_t aiPlayerSkill, int32_t aiLockLevelMax);
    static float SwimBreathDamage(int32_t aiMaxHealth);
    static void CalculateToolResistance(
        float& afResistanceTime,
        float& afMagnitude,
        float& afMouseScalar,
        float afInjuryPercent,
        float afPainPercent,
        int32_t aiMedicineSkill
    );
    static float GetPainCausedByWound(float afLimbWoundPercent);
    static void GetSurgeryAutoAttemptPercentages(
        float& afLimbHealPercent,
        float& afHealthDamagePercent,
        int32_t aiMedicineSkill,
        bool abMorphineUsed,
        float afWoundPercent
    );
    static float CalcImpactShaderAmount(float afDistance);
    static int32_t GetSpeechChallengeChance(float afPlayerSpeech, float afTargetDisposition, int32_t aiTopicDifficulty);
    static float GetActorSwimWalkSpeed(
        ActorValueOwner* apActor,
        TESObjectWEAP* pWeap,
        TESObjectARMO* pArmor,
        bool abNoWeapon,
        bool abIsNpc,
        bool abOverEncumbered
    );
    static float GetActorSwimRunSpeed(
        ActorValueOwner* apActor,
        TESObjectWEAP* pWeap,
        TESObjectARMO* pArmor,
        bool abNoWeapon,
        bool abIsNpc,
        bool abOverEncumbered
    );
    static double CalcFatigueReturn(float aiEndurance);
    static int32_t GetLevelUpSkillPoints_0(ActorValueOwner* a1);
    static float GetSpeechFailPenalty(int32_t aiSuccessChance);
    static float* GetBarterMultipliers(int32_t a1, float* a2, float* a3);
};

// The game settings GamePlayFormulas.cpp defines.
extern SettingT<GameSettingCollection> fExplosionShudderDurationX;
extern SettingT<GameSettingCollection> fRumbleBlockTime;
extern SettingT<GameSettingCollection> iVatsTargetAngle;
extern SettingT<GameSettingCollection> iLevelUpSkillPointsBase;
extern SettingT<GameSettingCollection> fPersuasionAccuracyMaxSelect;
extern SettingT<GameSettingCollection> fZKeySpringElasticity;
extern SettingT<GameSettingCollection> iPersuasionBribeRefuse;
extern SettingT<GameSettingCollection> iPersuasionInner;
extern SettingT<GameSettingCollection> fSpeechChallengeFailureBase;
extern SettingT<GameSettingCollection> sAchieveSMMenaceName;
extern SettingT<GameSettingCollection> fProjectileReorientTracerMin;
extern SettingT<GameSettingCollection> fPotionT3AleMagMult;
extern SettingT<GameSettingCollection> iPersuasionDemandGold;
extern SettingT<GameSettingCollection> fAgilityReloadModifier;
extern SettingT<GameSettingCollection> fExplosionForceMultLinear;
extern SettingT<GameSettingCollection> fTrainingCostMult;
extern SettingT<GameSettingCollection> fPersuasionBaseValueMaxDisposition;
extern SettingT<GameSettingCollection> fMoveSwimRunAthleticsMult;
extern SettingT<GameSettingCollection> fJumpFallHeightMin;
extern SettingT<GameSettingCollection> fRepairMax;
extern SettingT<GameSettingCollection> fBuoyancyBottle;
extern SettingT<GameSettingCollection> fPersBullyAggr;
extern SettingT<GameSettingCollection> fZKeyMaxContactMassRatio;
extern SettingT<GameSettingCollection> fPersuasionBaseValueMinSelect;
extern SettingT<GameSettingCollection> iClassBard;
extern SettingT<GameSettingCollection> fMoveSneakMult;
extern SettingT<GameSettingCollection> fPersBoastConf;
extern SettingT<GameSettingCollection> fCommentOnPlayerKnockingThings;
extern SettingT<GameSettingCollection> iPerkExtraBarterGoldMaster;
extern SettingT<GameSettingCollection> fMoveMediumArmorPenalty;
extern SettingT<GameSettingCollection> fExplosionShudderPowerMin;
extern SettingT<GameSettingCollection> fItemConditionValueBase;
extern SettingT<GameSettingCollection> fZKeyMaxForceWeightHigh;
extern SettingT<GameSettingCollection> iClassNightblade;
extern SettingT<GameSettingCollection> fSkillUseFactor;
extern SettingT<GameSettingCollection> fRumbleBlockStrength;
extern SettingT<GameSettingCollection> iClassAgent;
extern SettingT<GameSettingCollection> fPersuasionMaxInput;
extern SettingT<GameSettingCollection> fPotionT1AleDurMult;
extern SettingT<GameSettingCollection> fSplashSoundVelocityMult;
extern SettingT<GameSettingCollection> fPlayerHealthHeartbeatSlow;
extern SettingT<GameSettingCollection> fMoveSwimWalkBase;
extern SettingT<GameSettingCollection> fBuoyancyMultExtremity;
extern SettingT<GameSettingCollection> fExplosionShudderFrequencyY;
extern SettingT<GameSettingCollection> fPotionT1MagMult;
extern SettingT<GameSettingCollection> fSpeechAutoSuccessThreshold;
extern SettingT<GameSettingCollection> fKillCameraTime;
extern SettingT<GameSettingCollection> fPotionT3CalMagMult;
extern SettingT<GameSettingCollection> fPersJokeWillp;
extern SettingT<GameSettingCollection> fDifficultyDefaultValue;
extern SettingT<GameSettingCollection> fSplashScale2;
extern SettingT<GameSettingCollection> fItemConditionValueMult;
extern SettingT<GameSettingCollection> fEssentialHealthPercentReGain;
extern SettingT<GameSettingCollection> fPersAdmireAggr;
extern SettingT<GameSettingCollection> fPerkLightArmorExpertSpeedMult;
extern SettingT<GameSettingCollection> fDifficultyDamageMultiplier;
extern SettingT<GameSettingCollection> fCommentOnPlayerActionsTimer;
extern SettingT<GameSettingCollection> iPersuasionPower1;
extern SettingT<GameSettingCollection> fRepairSkillMax;
extern SettingT<GameSettingCollection> iTrainingSkills;
extern SettingT<GameSettingCollection> fLowLevelNPCBaseHealthMult;
extern SettingT<GameSettingCollection> fActorStrengthEncumbranceMult;
extern SettingT<GameSettingCollection> iClassPilgrim;
extern SettingT<GameSettingCollection> iHorseTurnDegreesPerSecond;
extern SettingT<GameSettingCollection> fHealingRateSleepingBase;
extern SettingT<GameSettingCollection> iCrimeGoldTresspass;
extern SettingT<GameSettingCollection> fTorchLightLevelMorning;
extern SettingT<GameSettingCollection> fZKeyMaxForceScaleHigh;
extern SettingT<GameSettingCollection> fPotionT1CalMagMult;
extern SettingT<GameSettingCollection> iClassArcher;
extern SettingT<GameSettingCollection> iCrimeGoldMurder;
extern SettingT<GameSettingCollection> iCrimeGoldPickpocket;
extern SettingT<GameSettingCollection> fEnchantmentPointsMult;
extern SettingT<GameSettingCollection> fMoveTwoCrippledLegsSpeedMult;
extern SettingT<GameSettingCollection> fJumpHeightMin;
extern SettingT<GameSettingCollection> fCameraShakeExplosionDistMult;
extern SettingT<GameSettingCollection> fActorWeaponDesirabilitySkillMult;
extern SettingT<GameSettingCollection> fBuoyancySkin;
extern SettingT<GameSettingCollection> fPersJokeStre;
extern SettingT<GameSettingCollection> fPotionT1AleMagMult;
extern SettingT<GameSettingCollection> fPersJokeIntel;
extern SettingT<GameSettingCollection> fPersJokeConf;
extern SettingT<GameSettingCollection> fPersuasionMinInput;
extern SettingT<GameSettingCollection> iClassThief;
extern SettingT<GameSettingCollection> fExplosionForceMultAngular;
extern SettingT<GameSettingCollection> fPersuasionMaxSelect;
extern SettingT<GameSettingCollection> iPersuasionAngleMin;
extern SettingT<GameSettingCollection> fSplashSoundLight;
extern SettingT<GameSettingCollection> fImprovedDetectModifier;
extern SettingT<GameSettingCollection> fMinDistanceUseHorse;
extern SettingT<GameSettingCollection> fCrossSkillDamageMultiplier;
extern SettingT<GameSettingCollection> iLevelUp02Mult;
extern SettingT<GameSettingCollection> iPersuasionBribeCrime;
extern SettingT<GameSettingCollection> fZKeySpringDamping;
extern SettingT<GameSettingCollection> iXPRewardDiscoverMapMarker;
extern SettingT<GameSettingCollection> fBarterSellMult;
extern SettingT<GameSettingCollection> fZKeyComplexHelperWeightMax;
extern SettingT<GameSettingCollection> fPotionT2RetDurMult;
extern SettingT<GameSettingCollection> fHelloCooldownTime;
extern SettingT<GameSettingCollection> iCrimeDaysInPrisonMod;
extern SettingT<GameSettingCollection> fJumpFallSkillBase;
extern SettingT<GameSettingCollection> fBarterBuyBase;
extern SettingT<GameSettingCollection> fPersBullyWillp;
extern SettingT<GameSettingCollection> fBarterHaggleCurve;
extern SettingT<GameSettingCollection> iXPBase;
extern SettingT<GameSettingCollection> fPathPointTurningSpeed;
extern SettingT<GameSettingCollection> fArmorWeightLightMaxMod;
extern SettingT<GameSettingCollection> fBarterBuyMult;
extern SettingT<GameSettingCollection> fActorAnimZAdjust;
extern SettingT<GameSettingCollection> fUpperCutVatsMultiplier;
extern SettingT<GameSettingCollection> iClassPriest;
extern SettingT<GameSettingCollection> fClothingBase;
extern SettingT<GameSettingCollection> fMoveBaseSpeed;
extern SettingT<GameSettingCollection> fPerkHeavyArmorSinkGravityMult;
extern SettingT<GameSettingCollection> fSwimRadiationDamageMult;
extern SettingT<GameSettingCollection> iPersuasionAngleMax;
extern SettingT<GameSettingCollection> fSubmergedLinearDampingV;
extern SettingT<GameSettingCollection> fObjectHitTwoHandReach;
extern SettingT<GameSettingCollection> fExplosionShudderDurationY;
extern SettingT<GameSettingCollection> fPersBullyResp;
extern SettingT<GameSettingCollection> fPlayerHealthHeartbeatFast;
extern SettingT<GameSettingCollection> fHostileActorInteriorDistance;
extern SettingT<GameSettingCollection> fCrimeGoldSteal;
extern SettingT<GameSettingCollection> iPCStartSpellSkillLevel;
extern SettingT<GameSettingCollection> fZKeyComplexHelperWeightMin;
extern SettingT<GameSettingCollection> fPersuasionAccuracyMinSelect;
extern SettingT<GameSettingCollection> fPersBullyIntel;
extern SettingT<GameSettingCollection> fCrimeDispSteal;
extern SettingT<GameSettingCollection> fRumblePainTime;
extern SettingT<GameSettingCollection> fPersAdmireWillp;
extern SettingT<GameSettingCollection> fExplosionImageSpaceSwapPower;
extern SettingT<GameSettingCollection> iMediumArmorStealthPenalty;
extern SettingT<GameSettingCollection> fPersuasionMaxDisposition;
extern SettingT<GameSettingCollection> fLockSkillBase;
extern SettingT<GameSettingCollection> iDeathDropWeaponChance;
extern SettingT<GameSettingCollection> fPotionT1RetDurMult;
extern SettingT<GameSettingCollection> fGameplayImpulseMultBiped;
extern SettingT<GameSettingCollection> fSubmergedAngularDamping;
extern SettingT<GameSettingCollection> iMerchantRespawnDay1;
extern SettingT<GameSettingCollection> fPersBoastIntel;
extern SettingT<GameSettingCollection> fHCHalfAlphaTextThreshold;
extern SettingT<GameSettingCollection> fPersuasionBaseValueMaxSelect;
extern SettingT<GameSettingCollection> fPersAdmirePers;
extern SettingT<GameSettingCollection> fSubmergedLODDistance;
extern SettingT<GameSettingCollection> fExplosionShudderFrequencyX;
extern SettingT<GameSettingCollection> fFatigueReturnBase;
extern SettingT<GameSettingCollection> sAchieveSMMasterDesc;
extern SettingT<GameSettingCollection> fPersBoastStre;
extern SettingT<GameSettingCollection> fImpactShaderMinMagnitude;
extern SettingT<GameSettingCollection> fPlayerDropDistance;
extern SettingT<GameSettingCollection> iLevelUp10Mult;
extern SettingT<GameSettingCollection> iClassRogue;
extern SettingT<GameSettingCollection> fRumbleShakeTimeMult;
extern SettingT<GameSettingCollection> fItemDetectionTimer;
extern SettingT<GameSettingCollection> fZKeyMaxContactDistance;
extern SettingT<GameSettingCollection> fBuoyancyMetal;
extern SettingT<GameSettingCollection> fRumbleHitTime;
extern SettingT<GameSettingCollection> fCrimeDispTresspass;
extern SettingT<GameSettingCollection> fPainDelay;
extern SettingT<GameSettingCollection> iCrimeGoldStealHorse;
extern SettingT<GameSettingCollection> fPotionT1RetMagMult;
extern SettingT<GameSettingCollection> fBuoyancyStone;
extern SettingT<GameSettingCollection> fPerkHeavyArmorJourneymanDamageMult;
extern SettingT<GameSettingCollection> iClassKnight;
extern SettingT<GameSettingCollection> fEssentialDeathTime;
extern SettingT<GameSettingCollection> fGameplayImpulseScale;
extern SettingT<GameSettingCollection> fActorArmorDesirabilityDamageMult;
extern SettingT<GameSettingCollection> fActorWeaponDesirabilityDamageMult;
extern SettingT<GameSettingCollection> iClassWitchhunter;
extern SettingT<GameSettingCollection> fRepairScavengeMult;
extern SettingT<GameSettingCollection> iLowLevelNPCMaxLevel;
extern SettingT<GameSettingCollection> fBarterSellBase;
extern SettingT<GameSettingCollection> iXPRewardPickLock;
extern SettingT<GameSettingCollection> iHorseTurnDegreesRampUpPerSecond;
extern SettingT<GameSettingCollection> fPerkAthleticsJourneymanFatigueMult;
extern SettingT<GameSettingCollection> fPersBoastAggr;
extern SettingT<GameSettingCollection> iPerkHeavyArmorSinkSum;
extern SettingT<GameSettingCollection> fGroundAttackVatsMultiplier;
extern SettingT<GameSettingCollection> fSpeechChallengeSpeechBase;
extern SettingT<GameSettingCollection> sAchieveSMMenaceDesc;
extern SettingT<GameSettingCollection> iXPRewardKillOpponent;
extern SettingT<GameSettingCollection> fNPCBaseMagickaMult;
extern SettingT<GameSettingCollection> fSplashSoundMedium;
extern SettingT<GameSettingCollection> fHCFlashTextThreshold;
extern SettingT<GameSettingCollection> fZKeyComplexHelperScale;
extern SettingT<GameSettingCollection> fActorArmorDesirabilitySkillMult;
extern SettingT<GameSettingCollection> fStatsHealthLevelMult;
extern SettingT<GameSettingCollection> iPersuasionPower2;
extern SettingT<GameSettingCollection> iDialogueDispositionFriendValue;
extern SettingT<GameSettingCollection> fVATSSmartCameraCheckStepCount;
extern SettingT<GameSettingCollection> fPerkLightArmorJourneymanDamageMult;
extern SettingT<GameSettingCollection> fActorSwimBreathBase;
extern SettingT<GameSettingCollection> fPersAdmireConf;
extern SettingT<GameSettingCollection> fRepairMin;
extern SettingT<GameSettingCollection> fBuoyancySodaCan;
extern SettingT<GameSettingCollection> fMoveRunMult;
extern SettingT<GameSettingCollection> fImpactShaderMaxMagnitude;
extern SettingT<GameSettingCollection> fCameraShakeFadeTime;
extern SettingT<GameSettingCollection> fMove2HRPenalty;
extern SettingT<GameSettingCollection> fZKeyObjectDamping;
extern SettingT<GameSettingCollection> iClassMage;
extern SettingT<GameSettingCollection> fPersJokePers;
extern SettingT<GameSettingCollection> iLevelUp05Mult;
extern SettingT<GameSettingCollection> iRemoveExcessDeadComplexTotalActorCount;
extern SettingT<GameSettingCollection> iPersuasionBribeGold;
extern SettingT<GameSettingCollection> fBuoyancyGrass;
extern SettingT<GameSettingCollection> fSplashScale3;
extern SettingT<GameSettingCollection> fPersBullyEner;
extern SettingT<GameSettingCollection> fSkillUseMajorMult;
extern SettingT<GameSettingCollection> fPersBoastEner;
extern SettingT<GameSettingCollection> iPerkHeavyArmorJumpSum;
extern SettingT<GameSettingCollection> iRemoveExcessDeadComplexCount;
extern SettingT<GameSettingCollection> iCrimeGoldAttack;
extern SettingT<GameSettingCollection> fGrabMaxWeightRunning;
extern SettingT<GameSettingCollection> fRumbleHitBlockedTime;
extern SettingT<GameSettingCollection> fSpeechCraftBase;
extern SettingT<GameSettingCollection> iRemoveExcessDeadTotalActorCount;
extern SettingT<GameSettingCollection> fMove2HBigPenalty;
extern SettingT<GameSettingCollection> iNPCBasePerLevelHealthMult;
extern SettingT<GameSettingCollection> fPersJokeAggr;
extern SettingT<GameSettingCollection> fPathPointZDistanceAllowance;
extern SettingT<GameSettingCollection> fCameraShakeMultMin;
extern SettingT<GameSettingCollection> fVATSSmartCameraCheckStepDistance;
extern SettingT<GameSettingCollection> fPerkHeavyArmorExpertSpeedMult;
extern SettingT<GameSettingCollection> iLevelUp08Mult;
extern SettingT<GameSettingCollection> fCrossSkillChance;
extern SettingT<GameSettingCollection> iPersuasionDemandScale;
extern SettingT<GameSettingCollection> fSpeechChallengeDispositionBase;
extern SettingT<GameSettingCollection> fProjectileDefaultTracerRange;
extern SettingT<GameSettingCollection> fNPCAttributeHealthMult;
extern SettingT<GameSettingCollection> fCameraShakeDistFadeStart;
extern SettingT<GameSettingCollection> fKillCameraCooldown;
extern SettingT<GameSettingCollection> iArmorBaseSkill;
extern SettingT<GameSettingCollection> fBarterHaggleBase;
extern SettingT<GameSettingCollection> iExplosionDebug;
extern SettingT<GameSettingCollection> fRemoveExcessDeadTime;
extern SettingT<GameSettingCollection> fDifficultyMinValue;
extern SettingT<GameSettingCollection> iPlayerCustomClass;
extern SettingT<GameSettingCollection> fPerkLightArmorNoviceDamageMult;
extern SettingT<GameSettingCollection> fSplashSoundOutMult;
extern SettingT<GameSettingCollection> fCrimeDispPersonal;
extern SettingT<GameSettingCollection> fRumbleStruckTime;
extern SettingT<GameSettingCollection> fPlayerMaxResistance;
extern SettingT<GameSettingCollection> iXPBumpBase;
extern SettingT<GameSettingCollection> fSayOncePerDayInfoTimer;
extern SettingT<GameSettingCollection> fPersBullyConf;
extern SettingT<GameSettingCollection> fDetectionTimerSetting;
extern SettingT<GameSettingCollection> fDifficultyMaxValue;
extern SettingT<GameSettingCollection> fJumpFallSkillMult;
extern SettingT<GameSettingCollection> fRumbleHitStrength;
extern SettingT<GameSettingCollection> fRumbleShakeRadiusMult;
extern SettingT<GameSettingCollection> fMoveLegConditionMult;
extern SettingT<GameSettingCollection> fPersBullyStre;
extern SettingT<GameSettingCollection> fPersuasionMinPercentCircle;
extern SettingT<GameSettingCollection> iBarterDispositionPenalty;
extern SettingT<GameSettingCollection> iHackingRetryMilliseconds;
extern SettingT<GameSettingCollection> fBribeMult;
extern SettingT<GameSettingCollection> iClassBarbarian;
extern SettingT<GameSettingCollection> fHostileActorExteriorDistance;
extern SettingT<GameSettingCollection> fPotionMortPestleMult;
extern SettingT<GameSettingCollection> iClassMonk;
extern SettingT<GameSettingCollection> iMapMarkerRevealDistance;
extern SettingT<GameSettingCollection> fCounterAttackTimer;
extern SettingT<GameSettingCollection> fBuoyancyGlass;
extern SettingT<GameSettingCollection> fItemConditionValueExp;
extern SettingT<GameSettingCollection> fTorchEvaluationTimer;
extern SettingT<GameSettingCollection> fCrimeDispMurder;
extern SettingT<GameSettingCollection> fHCSleepRestorationMod;
extern SettingT<GameSettingCollection> fMoveNoWeaponMult;
extern SettingT<GameSettingCollection> iMerchantRespawnDay2;
extern SettingT<GameSettingCollection> fHCDehydrationRate;
extern SettingT<GameSettingCollection> fBuoyancyDirt;
extern SettingT<GameSettingCollection> iMaxCharacterLevel;
extern SettingT<GameSettingCollection> fSplashSoundHeavy;
extern SettingT<GameSettingCollection> fHCFullAlphaTextThreshold;
extern SettingT<GameSettingCollection> fSpeechChallengeMultiplier;
extern SettingT<GameSettingCollection> fExplosionMaxImpulse;
extern SettingT<GameSettingCollection> fClothingArmorScale;
extern SettingT<GameSettingCollection> fMoveMinFlySpeed;
extern SettingT<GameSettingCollection> fPerkAthleticsMasterFatigueMult;
extern SettingT<GameSettingCollection> iRemoveExcessDeadCount;
extern SettingT<GameSettingCollection> sExplosionSplashParticles;
extern SettingT<GameSettingCollection> fSubmergedMaxSpeed;
extern SettingT<GameSettingCollection> fTorchLightLevelInterior;
extern SettingT<GameSettingCollection> iPersuasionDemandRefuse;
extern SettingT<GameSettingCollection> fClothingClassScale;
extern SettingT<GameSettingCollection> fJumpFallHeightExponent;
extern SettingT<GameSettingCollection> fGameplayImpulseMultClutter;
extern SettingT<GameSettingCollection> fExplosionShudderIntensityX;
extern SettingT<GameSettingCollection> iClassAcrobat;
extern SettingT<GameSettingCollection> fGameplayImpulseMultProp;
extern SettingT<GameSettingCollection> fPerkAthleticsApprenticeFatigueMult;
extern SettingT<GameSettingCollection> fAgilityReloadBase;
extern SettingT<GameSettingCollection> iClassWarrior;
extern SettingT<GameSettingCollection> fCameraShakeDistMin;
extern SettingT<GameSettingCollection> fJumpFallRiderMult;
extern SettingT<GameSettingCollection> fExplosionSplashRadius;
extern SettingT<GameSettingCollection> fRadiationReevaluationRate;
extern SettingT<GameSettingCollection> fExplosionSourceRefMult;
extern SettingT<GameSettingCollection> iDialogueDummySpeakThisIntOrBelow;
extern SettingT<GameSettingCollection> fPersuasionBaseValueShape;
extern SettingT<GameSettingCollection> fLockpickBonusHealth;
extern SettingT<GameSettingCollection> iXPRewardDiscoverSecretArea;
extern SettingT<GameSettingCollection> fCameraShakeTime;
extern SettingT<GameSettingCollection> fPersAdmireEner;
extern SettingT<GameSettingCollection> iPersuasionOuter;
extern SettingT<GameSettingCollection> fMoveMaxFlySpeed;
extern SettingT<GameSettingCollection> fActorSwimBreathMult;
extern SettingT<GameSettingCollection> fExplosionWaterRadiusRatio;
extern SettingT<GameSettingCollection> fBuoyancyCloth;
extern SettingT<GameSettingCollection> fZKeyMaxForceScaleLow;
extern SettingT<GameSettingCollection> fBuoyancyMultBody;
extern SettingT<GameSettingCollection> fExplosionForceClutterUpBias;
extern SettingT<GameSettingCollection> iClassCharactergenClass;
extern SettingT<GameSettingCollection> iLevelUp09Mult;
extern SettingT<GameSettingCollection> fClothingJewelryScale;
extern SettingT<GameSettingCollection> fMoveHeavyArmorPenalty;
extern SettingT<GameSettingCollection> fBarterDispositionMod;
extern SettingT<GameSettingCollection> fObjectHitH2HReach;
extern SettingT<GameSettingCollection> fPersBoastResp;
extern SettingT<GameSettingCollection> fExplosionShudderIntensityY;
extern SettingT<GameSettingCollection> fSubmergedLinearDampingH;
extern SettingT<GameSettingCollection> fPersuasionShape;
extern SettingT<GameSettingCollection> fPerkJumpFatigueExpertMult;
extern SettingT<GameSettingCollection> fSkillUseSpecMult;
extern SettingT<GameSettingCollection> fRadiationAccumulationRate;
extern SettingT<GameSettingCollection> fEnchantmentEffectPointsMult;
extern SettingT<GameSettingCollection> fGameplayImpulseMultDebrisLarge;
extern SettingT<GameSettingCollection> fPotionT3RetMagMult;
extern SettingT<GameSettingCollection> fRumblePainStrength;
extern SettingT<GameSettingCollection> fBarterHaggleMax;
extern SettingT<GameSettingCollection> iPersuasionMiddle;
extern SettingT<GameSettingCollection> fPotionGoldValueMult;
extern SettingT<GameSettingCollection> fValueofItemForNoOwnership;
extern SettingT<GameSettingCollection> iClassCrusader;
extern SettingT<GameSettingCollection> fDetectionNightEyeBonus;
extern SettingT<GameSettingCollection> fDeathSoundMaxDistance;
extern SettingT<GameSettingCollection> fTurboTimeMultiplier;
extern SettingT<GameSettingCollection> fPersAdmireStre;
extern SettingT<GameSettingCollection> fPersBoastWillp;
extern SettingT<GameSettingCollection> fFatigueReturnMult;
extern SettingT<GameSettingCollection> iMolotovTest;
extern SettingT<GameSettingCollection> fRepairSkillBase;
extern SettingT<GameSettingCollection> fZKeyMaxForce;
extern SettingT<GameSettingCollection> fRumbleHitBlockedStrength;
extern SettingT<GameSettingCollection> fHCStarvationRate;
extern SettingT<GameSettingCollection> fMaxArmorRating;
extern SettingT<GameSettingCollection> fClothingJewelryBase;
extern SettingT<GameSettingCollection> fStatsHealthStartMult;
extern SettingT<GameSettingCollection> fSkillUseExp;
extern SettingT<GameSettingCollection> fObjectHitWeaponReach;
extern SettingT<GameSettingCollection> iClassHealer;
extern SettingT<GameSettingCollection> fGameplayImpulseMultTrap;
extern SettingT<GameSettingCollection> iLevelUpSkillPointsInterval;
extern SettingT<GameSettingCollection> iLevelUpSkillCount;
extern SettingT<GameSettingCollection> fFallLegDamageMult;
extern SettingT<GameSettingCollection> iLevelUp03Mult;
extern SettingT<GameSettingCollection> iNumExplosionDecalCDPoint;
extern SettingT<GameSettingCollection> iLevelUp04Mult;
extern SettingT<GameSettingCollection> fPCBaseHealthMult;
extern SettingT<GameSettingCollection> iFallLegDamageChance;
extern SettingT<GameSettingCollection> iActivatePickLength;
extern SettingT<GameSettingCollection> fBuoyancyOrganic;
extern SettingT<GameSettingCollection> fPerkAthleticsExpertFatigueMult;
extern SettingT<GameSettingCollection> fUpperCutSkillChance;
extern SettingT<GameSettingCollection> fClothingArmorBase;
extern SettingT<GameSettingCollection> fSpeechChallengeDispositionMultiplier;
extern SettingT<GameSettingCollection> fImpactShaderMaxDistance;
extern SettingT<GameSettingCollection> fObjectHitOneHandReach;
extern SettingT<GameSettingCollection> fPerkHeavyArmorNoviceDamageMult;
extern SettingT<GameSettingCollection> fBribeBase;
extern SettingT<GameSettingCollection> fPersuasionAccuracyMaxDisposition;
extern SettingT<GameSettingCollection> fItemRepairCostMult;
extern SettingT<GameSettingCollection> fHealingRateSleepingMult;
extern SettingT<GameSettingCollection> fActorSwimBreathDamage;
extern SettingT<GameSettingCollection> fExplosionForceKnockdownMinimum;
extern SettingT<GameSettingCollection> fPersBoastPers;
extern SettingT<GameSettingCollection> fMoveSwimRunBase;
extern SettingT<GameSettingCollection> iClassSorcerer;
extern SettingT<GameSettingCollection> fMoveOneCrippledLegSpeedMult;
extern SettingT<GameSettingCollection> iCrimeGoldMinValue;
extern SettingT<GameSettingCollection> iClassBattlemage;
extern SettingT<GameSettingCollection> iLevelUp06Mult;
extern SettingT<GameSettingCollection> fMaximumWind;
extern SettingT<GameSettingCollection> fVATSSmartCameraCheckHeight;
extern SettingT<GameSettingCollection> iLevelUp01Mult;
extern SettingT<GameSettingCollection> fBSUnitsPerFoot;
extern SettingT<GameSettingCollection> fBuoyancyWood;
extern SettingT<GameSettingCollection> fGrabMaxWeightWalking;
extern SettingT<GameSettingCollection> fActorLuckSkillMult;
extern SettingT<GameSettingCollection> fSubmergedMaxWaterDistance;
extern SettingT<GameSettingCollection> iClassScout;
extern SettingT<GameSettingCollection> fZKeyMaxForceWeightLow;
extern SettingT<GameSettingCollection> iActorLuckSkillBase;
extern SettingT<GameSettingCollection> fJumpDoubleMult;
extern SettingT<GameSettingCollection> fGameplayImpulseMinMass;
extern SettingT<GameSettingCollection> fPersAdmireResp;
extern SettingT<GameSettingCollection> iNumberActorsGoThroughLoadDoorInCombat;
extern SettingT<GameSettingCollection> sSplashParticles;
extern SettingT<GameSettingCollection> iMapMarkerVisibleDistance;
extern SettingT<GameSettingCollection> fCrossVatsMultiplier;
extern SettingT<GameSettingCollection> fCrossAPCost;
extern SettingT<GameSettingCollection> fMoveSwimWalkAthleticsMult;
extern SettingT<GameSettingCollection> fPersAdmireIntel;
extern SettingT<GameSettingCollection> fZKeyComplexHelperMinDistance;
extern SettingT<GameSettingCollection> fRadiationDecayRate;
extern SettingT<GameSettingCollection> fHavokTauRatio;
extern SettingT<GameSettingCollection> fHCRedTextThreshold;
extern SettingT<GameSettingCollection> fPotionT1CalDurMult;
extern SettingT<GameSettingCollection> fExplosionLOSBufferDistance;
extern SettingT<GameSettingCollection> iClassSpellsword;
extern SettingT<GameSettingCollection> fSplashSoundTimer;
extern SettingT<GameSettingCollection> iXPRewardHackComputer;
extern SettingT<GameSettingCollection> fRumbleStruckStrength;
extern SettingT<GameSettingCollection> fUpperCutThreshold;
extern SettingT<GameSettingCollection> fBarterHaggleDispMult;
extern SettingT<GameSettingCollection> iPersuasionBribeScale;
extern SettingT<GameSettingCollection> fJumpSwimmingMult;
extern SettingT<GameSettingCollection> fPersJokeEner;
extern SettingT<GameSettingCollection> fPerkHeavyArmorMasterSpeedMult;
extern SettingT<GameSettingCollection> fPlayerDetectActorValue;
extern SettingT<GameSettingCollection> fPCBaseMagickaMult;
extern SettingT<GameSettingCollection> fCrimeDispAttack;
extern SettingT<GameSettingCollection> fPersuasionMinSelect;
extern SettingT<GameSettingCollection> fDemandMult;
extern SettingT<GameSettingCollection> iClassAssassin;
extern SettingT<GameSettingCollection> fJumpFallHeightMult;
extern SettingT<GameSettingCollection> fBuoyancyWater;
extern SettingT<GameSettingCollection> fTorchLightLevelNight;
extern SettingT<GameSettingCollection> fCameraShakeDistFadeDelta;
extern SettingT<GameSettingCollection> fHCSleepDeprivationRate;
extern SettingT<GameSettingCollection> fSkillUseMinorMult;
extern SettingT<GameSettingCollection> fSplashScale1;
extern SettingT<GameSettingCollection> fWadeRadiationDamageMult;
extern SettingT<GameSettingCollection> fPersBullyPers;
extern SettingT<GameSettingCollection> fBarterDispBase;
extern SettingT<GameSettingCollection> fCrimeDispPickpocket;
extern SettingT<GameSettingCollection> iPersuasionDemandDisposition;
extern SettingT<GameSettingCollection> fPersJokeResp;
extern SettingT<GameSettingCollection> fSpeechCraftMult;
extern SettingT<GameSettingCollection> fPerkAthleticsNoviceFatigueMult;
extern SettingT<GameSettingCollection> sRagdollDataDefault;
extern SettingT<GameSettingCollection> fSpeechChallengeFailureMultiplier;
extern SettingT<GameSettingCollection> sAchieveSMMasterName;
extern SettingT<GameSettingCollection> fPotionT2AleDurMult;
extern SettingT<GameSettingCollection> fUpperCutAPCost;
extern SettingT<GameSettingCollection> fCrossThreshold;
extern SettingT<GameSettingCollection> fExplosionLOSBuffer;
extern SettingT<GameSettingCollection> fRechargeGoldMult;
extern SettingT<GameSettingCollection> fPotionT2CalDurMult;
extern SettingT<GameSettingCollection> iPersuasionPower3;
extern SettingT<GameSettingCollection> fDemandBase;
extern SettingT<GameSettingCollection> iHeavyArmorStealthPenalty;
extern SettingT<GameSettingCollection> iLevelUp07Mult;

// The five difficulties' fDiffMultHPToPC settings.
using fDiffMultHPToPC_t = SettingT<GameSettingCollection>* [5];
extern fDiffMultHPToPC_t fDiffMultHPToPC;

// The five difficulties' fDiffMultHPByPC settings.
using fDiffMultHPByPC_t = SettingT<GameSettingCollection>* [5];
extern fDiffMultHPByPC_t fDiffMultHPByPC;

// The five difficulties' fDiffMultXP settings.
using fDiffMultXP_t = SettingT<GameSettingCollection>* [5];
extern fDiffMultXP_t fDiffMultXP;
