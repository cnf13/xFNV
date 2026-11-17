#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/SkillLevel.h"

class ActorValueOwner;

float hypotf_3(float X, float Y);

// The formulas of the magic system (spell costs and levels, resistances, sun damage and the like), all static; it
// has no members.
class MagicFormulas {
public:
	// The damage the sun does to a target.
	static float CalcSunDamage(float afMagnitude, float afTimeOfDay, float afSunrise, float afSunset, bool abSunVisible,
		float afWeatherScale, bool abUnderWater);
	// The cost of an effect from its base cost, area, duration, magnitude and range.
	static float GetMagicEffectCost(float afBaseEffectCost, int32_t aiArea, int32_t aiDuration, int32_t aiMagnitude,
		bool abIsRangeTarget);
	// A spell cost scaled by the caster's school skill. The callers pass the caster's luck too; the function never
	// reads it.
	static float AdjustMagicSpellCost(float afSpellCost, int32_t aiCasterSkill, int32_t aiCasterLuck);
	// The multiplier a target's magic-type and effect resistances (in percent) leave of an effect; 0 when either is at
	// least 100.
	static float GetMagicTargetResistance(float afTargetMagicTypeResistance, float afTargetEffectResistance);
	static bool CheckTargetLevelMagnitude(int32_t aiTargetLevel, float afMagnitude);
	// The explosion power from the spell cost, the area in feet, the distance and the target's agility. The luck and
	// the fatigue percentage the callers pass are not read.
	static float CheckExplosionStrength(int32_t aiCost, int32_t aiArea, float afDistance, int32_t aiAgility,
		int32_t aiLuck, float afFatiguePercentage);
	// The medicine effectiveness multiplier from the actor's medicine skill.
	static float CalcMedicineEffectivenessMult(ActorValueOwner* apActor);
	// The survival effectiveness multiplier from the actor's survival skill.
	static float CalcSurvivalEffectivenessMult(ActorValueOwner* apActor);
	static SkillLevel::Value GetMagicSpellLevel(float afSpellCost);
};

// The game settings MagicFormulas.cpp defines.
extern SettingT<GameSettingCollection> fAbsorbCoreColorG;
extern SettingT<GameSettingCollection> fReflectedAbsorbChanceReduction;
extern SettingT<GameSettingCollection> fMagicSpellLevelCostBase;
extern SettingT<GameSettingCollection> fMagicBoltDuration;
extern SettingT<GameSettingCollection> fMagicAreaScaleMin;
extern SettingT<GameSettingCollection> fMagicEnchantmentChargeBase;
extern SettingT<GameSettingCollection> fMagicExplosionIncorporealTime;
extern SettingT<GameSettingCollection> fMagicCloudAreaMin;
extern SettingT<GameSettingCollection> fMagicArmorPenaltyMin;
extern SettingT<GameSettingCollection> fMagicUnitsPerFoot;
extern SettingT<GameSettingCollection> fMagicLightHeightOffset;
extern SettingT<GameSettingCollection> fMagicTrackingMultFog;
extern SettingT<GameSettingCollection> fMagicTelekinesisComplexMaxForce;
extern SettingT<GameSettingCollection> fShockBoltSmallWidth;
extern SettingT<GameSettingCollection> fEnchantmentGoldMult;
extern SettingT<GameSettingCollection> fShockBoltsRadius;
extern SettingT<GameSettingCollection> fMagickaReturnBase;
extern SettingT<GameSettingCollection> fWortcraftStrChanceDenom;
extern SettingT<GameSettingCollection> fMagicSunDamageWaterScale;
extern SettingT<GameSettingCollection> fMagicTrackingLimitComplex;
extern SettingT<GameSettingCollection> fMagicNightEyeAmbient;
extern SettingT<GameSettingCollection> fMagicMedicineSkillMult;
extern SettingT<GameSettingCollection> fAbsorbBoltGrowWidth;
extern SettingT<GameSettingCollection> fShockBranchBoltsRadius;
extern SettingT<GameSettingCollection> fMagicSunDamageScreenGlowRateDown;
extern SettingT<GameSettingCollection> fAbsorbGlowColorR;
extern SettingT<GameSettingCollection> fShockBranchSegmentLength;
extern SettingT<GameSettingCollection> fMagicExplosionIncorporealMult;
extern SettingT<GameSettingCollection> fShockSegmentVariance;
extern SettingT<GameSettingCollection> fMagicSunDamageScreenGlowRateUp;
extern SettingT<GameSettingCollection> fMagicCasterSkillCostBase;
extern SettingT<GameSettingCollection> iShockSubSegments;
extern SettingT<GameSettingCollection> fMagicProjectileBaseSpeed;
extern SettingT<GameSettingCollection> fMagicLevelMagnitudeMult;
extern SettingT<GameSettingCollection> fAbsorbBoltsRadius;
extern SettingT<GameSettingCollection> iMaxPlayerSummonedCreatures;
extern SettingT<GameSettingCollection> fMagicTelekinesisComplexObjectDamping;
extern SettingT<GameSettingCollection> fMagicFatigueDrainMult;
extern SettingT<GameSettingCollection> fMagickaReturnMult;
extern SettingT<GameSettingCollection> fShockSegmentLength;
extern SettingT<GameSettingCollection> fMagicTrackingMultBolt;
extern SettingT<GameSettingCollection> fMagicSunDamageBaseDamage;
extern SettingT<GameSettingCollection> fWortcraftFatigueMag;
extern SettingT<GameSettingCollection> fMagicTelekinesisMaxForce;
extern SettingT<GameSettingCollection> fScrollCostMult;
extern SettingT<GameSettingCollection> fMagicCasterSkillCostMult;
extern SettingT<GameSettingCollection> fShockBranchBoltsRadiusStrength;
extern SettingT<GameSettingCollection> fMagicVACNoPartTargetedMult;
extern SettingT<GameSettingCollection> fMagicBoltSegmentLength;
extern SettingT<GameSettingCollection> fShockSubSegmentVariance;
extern SettingT<GameSettingCollection> fMagicMedicineSkillBase;
extern SettingT<GameSettingCollection> fActiveEffectConditionUpdateInterval;
extern SettingT<GameSettingCollection> fMagicTelekinesisThrow;
extern SettingT<GameSettingCollection> fMagicCloudFindTargetTime;
extern SettingT<GameSettingCollection> iShockSegmentsPerBolt;
extern SettingT<GameSettingCollection> fMagicExplosionPowerMin;
extern SettingT<GameSettingCollection> fAbsorbGlowColorB;
extern SettingT<GameSettingCollection> fMagicResistTargetWillpowerBase;
extern SettingT<GameSettingCollection> fAbsorbGlowColorG;
extern SettingT<GameSettingCollection> fMagicTelekinesisComplexSpringElasticity;
extern SettingT<GameSettingCollection> iMagicMaxPotionsJourneyman;
extern SettingT<GameSettingCollection> fMagicSurvivalSkillBase;
extern SettingT<GameSettingCollection> fMagicVACPartTargetedMult;
extern SettingT<GameSettingCollection> iShockNumBolts;
extern SettingT<GameSettingCollection> fAddictionUsageMonitorThreshold;
extern SettingT<GameSettingCollection> fMagicExplosionPowerMax;
extern SettingT<GameSettingCollection> fMagicCloudLifeScale;
extern SettingT<GameSettingCollection> fMagicDiseaseTransferMult;
extern SettingT<GameSettingCollection> fMagicTelekinesisComplexSpringDamping;
extern SettingT<GameSettingCollection> fAbsorbCoreColorB;
extern SettingT<GameSettingCollection> fShockBranchSegmentVariance;
extern SettingT<GameSettingCollection> fWortcraftChanceIntDenom;
extern SettingT<GameSettingCollection> fMagicSunDamagePainTimer;
extern SettingT<GameSettingCollection> fSpellmakingGoldMult;
extern SettingT<GameSettingCollection> fShockCoreColorB;
extern SettingT<GameSettingCollection> fMagicTelekinesiDistanceMult;
extern SettingT<GameSettingCollection> fShockCoreColorG;
extern SettingT<GameSettingCollection> iMagicMaxPotionsMaster;
extern SettingT<GameSettingCollection> fMagicArmorPenaltyMax;
extern SettingT<GameSettingCollection> fMagicTelekinesisLiftPowerMult;
extern SettingT<GameSettingCollection> fMagicSunDamageMinWeather;
extern SettingT<GameSettingCollection> fMagicSunDamagePainInitialDelay;
extern SettingT<GameSettingCollection> fMagicRangeTargetCostMult;
extern SettingT<GameSettingCollection> fMagicAreaBaseCostMult;
extern SettingT<GameSettingCollection> fMagicEnchantmentChargeMult;
extern SettingT<GameSettingCollection> fMagicLightRadiusBase;
extern SettingT<GameSettingCollection> fMagicCostScale;
extern SettingT<GameSettingCollection> fMagicDurMagBaseCostMult;
extern SettingT<GameSettingCollection> fMagicEnchantmentDrainBase;
extern SettingT<GameSettingCollection> fMagicResistTargetWillpowerMult;
extern SettingT<GameSettingCollection> fMagicSpellLevelApprenticeMin;
extern SettingT<GameSettingCollection> fMagicCloudSpeedBase;
extern SettingT<GameSettingCollection> fShockGlowColorG;
extern SettingT<GameSettingCollection> fWortStrMult;
extern SettingT<GameSettingCollection> fMagicCloudSpeedScale;
extern SettingT<GameSettingCollection> fMagicTrackingLimit;
extern SettingT<GameSettingCollection> fMagicTelekinesisSpringDamping;
extern SettingT<GameSettingCollection> iMagnitudeLevelAffectsAll;
extern SettingT<GameSettingCollection> fMagicResistActorSkillMult;
extern SettingT<GameSettingCollection> fMagicLightSideOffset;
extern SettingT<GameSettingCollection> fMagicTelekinesisMoveAccelerate;
extern SettingT<GameSettingCollection> iShockBranchNumBolts;
extern SettingT<GameSettingCollection> fMagicSurvivalSkillMult;
extern SettingT<GameSettingCollection> fItemPointsMult;
extern SettingT<GameSettingCollection> fMagicSunDamageScreenGlowMult;
extern SettingT<GameSettingCollection> fMagicLightForwardOffset;
extern SettingT<GameSettingCollection> fMagicDefaultTouchDistance;
extern SettingT<GameSettingCollection> fShockBoltsLength;
extern SettingT<GameSettingCollection> fMagicAbsorbDistanceReachMult;
extern SettingT<GameSettingCollection> fAbsorbBoltSmallWidth;
extern SettingT<GameSettingCollection> fAbsorbTortuosityVariance;
extern SettingT<GameSettingCollection> fMagicAreaScaleMax;
extern SettingT<GameSettingCollection> fMagicCloudSlowdownRate;
extern SettingT<GameSettingCollection> fShockCastVOffset;
extern SettingT<GameSettingCollection> fAbsorbSegmentLength;
extern SettingT<GameSettingCollection> iShockDebug;
extern SettingT<GameSettingCollection> fFatigueCastMult;
extern SettingT<GameSettingCollection> fShockBranchLifetime;
extern SettingT<GameSettingCollection> fMagicDispelMagnitudeMult;
extern SettingT<GameSettingCollection> fShockBoltGrowWidth;
extern SettingT<GameSettingCollection> fMagicPlayerMinimumInvisibility;
extern SettingT<GameSettingCollection> fMagicTelekinesisObjectDamping;
extern SettingT<GameSettingCollection> iShockBranchSegmentsPerBolt;
extern SettingT<GameSettingCollection> fFatigueCastBase;
extern SettingT<GameSettingCollection> fMagicSunDamageSunHiddenScale;
extern SettingT<GameSettingCollection> fMagicCloudDurationMin;
extern SettingT<GameSettingCollection> fMagicSpellLevelMasterMin;
extern SettingT<GameSettingCollection> fMagicExplosionClutterMult;
extern SettingT<GameSettingCollection> fMagicSpellLevelCostMult;
extern SettingT<GameSettingCollection> fConcussionTimer;
extern SettingT<GameSettingCollection> fMagicExplosionPowerBase;
extern SettingT<GameSettingCollection> fMagicAreaScale;
extern SettingT<GameSettingCollection> fWortcraftChanceLuckDenom;
extern SettingT<GameSettingCollection> fMagicCloudSizeScale;
extern SettingT<GameSettingCollection> fShockCoreColorR;
extern SettingT<GameSettingCollection> fShockBoltsRadiusStrength;
extern SettingT<GameSettingCollection> iMagicMaxPotionsApprentice;
extern SettingT<GameSettingCollection> fMagicEnchantmentDrainMult;
extern SettingT<GameSettingCollection> fAbsorbMoveSpeed;
extern SettingT<GameSettingCollection> fMagicFatigueDrainBase;
extern SettingT<GameSettingCollection> fMagicTrackingMultBall;
extern SettingT<GameSettingCollection> fMagicResistActorSkillBase;
extern SettingT<GameSettingCollection> fMagicCEEnchantMagOffset;
extern SettingT<GameSettingCollection> fShockGlowColorR;
extern SettingT<GameSettingCollection> iMagicMaxPotionsExpert;
extern SettingT<GameSettingCollection> iAbsorbNumBolts;
extern SettingT<GameSettingCollection> iMagicMaxPotionsNovice;
extern SettingT<GameSettingCollection> fMagicTelekinesisSpringElasticity;
extern SettingT<GameSettingCollection> fAbsorbBoltsRadiusStrength;
extern SettingT<GameSettingCollection> fAbsorbCoreColorR;
extern SettingT<GameSettingCollection> fAlchemyGoldMult;
extern SettingT<GameSettingCollection> fMagicTelekinesisMoveMax;
extern SettingT<GameSettingCollection> fMagicDiseaseTransferBase;
extern SettingT<GameSettingCollection> fMagicSpellLevelJourneymanMin;
extern SettingT<GameSettingCollection> fMagicExplosionAgilityMult;
extern SettingT<GameSettingCollection> fShockGlowColorB;
extern SettingT<GameSettingCollection> fMagicExplosionPowerMult;
extern SettingT<GameSettingCollection> fMagicTelekinesisMoveBase;
extern SettingT<GameSettingCollection> fMagicProjectileMaxDistance;
extern SettingT<GameSettingCollection> fMagicSpellLevelExpertMin;
extern SettingT<GameSettingCollection> iMagicLightMaxCount;
extern SettingT<GameSettingCollection> fMagicTelekinesisDistanceMin;
extern SettingT<GameSettingCollection> fWortcraftStrCostDenom;
extern SettingT<GameSettingCollection> fAbsorbSegmentVariance;
