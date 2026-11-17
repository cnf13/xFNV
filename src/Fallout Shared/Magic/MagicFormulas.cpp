#include "Fallout Shared/Magic/MagicFormulas.h"

#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/SkillLevel.h"
#include "Fallout/Magic/MagicBoltProjectile.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> fMagicUnitsPerFoot("fMagicUnitsPerFoot", 22.0f);
SettingT<GameSettingCollection> fMagicFatigueDrainBase("fMagicFatigueDrainBase", 0.0f);
SettingT<GameSettingCollection> fMagicFatigueDrainMult("fMagicFatigueDrainMult", 0.25f);
SettingT<GameSettingCollection> fMagicAreaBaseCostMult("fMagicAreaBaseCostMult", 0.15f);
SettingT<GameSettingCollection> fMagicDurMagBaseCostMult("fMagicDurMagBaseCostMult", 0.1f);
SettingT<GameSettingCollection> fMagicRangeTargetCostMult("fMagicRangeTargetCostMult", 1.5f);
SettingT<GameSettingCollection> fMagicCasterSkillCostBase("fMagicCasterSkillCostBase", 0.1f);
SettingT<GameSettingCollection> fMagicCasterSkillCostMult("fMagicCasterSkillCostMult", 1.2f);
SettingT<GameSettingCollection> fMagicSpellLevelCostBase("fMagicSpellLevelCostBase", 0.0f);
SettingT<GameSettingCollection> fMagicSpellLevelCostMult("fMagicSpellLevelCostMult", 1.0f);
SettingT<GameSettingCollection> fMagicSpellLevelApprenticeMin("fMagicSpellLevelApprenticeMin", 25.0f);
SettingT<GameSettingCollection> fMagicSpellLevelJourneymanMin("fMagicSpellLevelJourneymanMin", 50.0f);
SettingT<GameSettingCollection> fMagicSpellLevelExpertMin("fMagicSpellLevelExpertMin", 75.0f);
SettingT<GameSettingCollection> fMagicSpellLevelMasterMin("fMagicSpellLevelMasterMin", 100.0f);
SettingT<GameSettingCollection> fMagicEnchantmentChargeBase("fMagicEnchantmentChargeBase", 0.0f);
SettingT<GameSettingCollection> fMagicEnchantmentChargeMult("fMagicEnchantmentChargeMult", 1.0f);
SettingT<GameSettingCollection> fMagicEnchantmentDrainBase("fMagicEnchantmentDrainBase", 0.0f);
SettingT<GameSettingCollection> fMagicEnchantmentDrainMult("fMagicEnchantmentDrainMult", 0.25f);
SettingT<GameSettingCollection> fMagicResistActorSkillBase("fMagicResistActorSkillBase", 0.0f);
SettingT<GameSettingCollection> fMagicResistActorSkillMult("fMagicResistActorSkillMult", -0.0025f);
SettingT<GameSettingCollection> fMagicResistTargetWillpowerBase("fMagicResistTargetWillpowerBase", 0.0f);
SettingT<GameSettingCollection> fMagicResistTargetWillpowerMult("fMagicResistTargetWillpowerMult", 0.005f);
SettingT<GameSettingCollection> fMagicDefaultTouchDistance("fMagicDefaultTouchDistance", 256.0f);
SettingT<GameSettingCollection> fMagicAbsorbDistanceReachMult("fMagicAbsorbDistanceReachMult", 3.0f);
SettingT<GameSettingCollection> fMagicBoltSegmentLength("fMagicBoltSegmentLength", 300.0f);
SettingT<GameSettingCollection> fMagicBoltDuration("fMagicBoltDuration", 1.5f);
SettingT<GameSettingCollection> fMagicProjectileMaxDistance("fMagicProjectileMaxDistance", 10000.0f);
SettingT<GameSettingCollection> fMagicExplosionPowerMin("fMagicExplosionPowerMin", 0.0f);
SettingT<GameSettingCollection> fMagicExplosionPowerMax("fMagicExplosionPowerMax", 100.0f);
SettingT<GameSettingCollection> fMagicExplosionPowerBase("fMagicExplosionPowerBase", 100.0f);
SettingT<GameSettingCollection> fMagicExplosionPowerMult("fMagicExplosionPowerMult", 0.4f);
SettingT<GameSettingCollection> fMagicExplosionAgilityMult("fMagicExplosionAgilityMult", 0.0f);
SettingT<GameSettingCollection> fMagicExplosionIncorporealMult("fMagicExplosionIncorporealMult", 600.0f);
SettingT<GameSettingCollection> fMagicExplosionIncorporealTime("fMagicExplosionIncorporealTime", 1.25f);
SettingT<GameSettingCollection> fMagicExplosionClutterMult("fMagicExplosionClutterMult", 6.5f);
SettingT<GameSettingCollection> fMagicAreaScale("fMagicAreaScale", 1.75f);
SettingT<GameSettingCollection> fMagicAreaScaleMax("fMagicAreaScaleMax", 100.0f);
SettingT<GameSettingCollection> fMagicAreaScaleMin("fMagicAreaScaleMin", 2.5f);
SettingT<GameSettingCollection> fMagicProjectileBaseSpeed("fMagicProjectileBaseSpeed", 1000.0f);
SettingT<GameSettingCollection> fMagicCloudSpeedBase("fMagicCloudSpeedBase", 750.0f);
SettingT<GameSettingCollection> fMagicCloudLifeScale("fMagicCloudLifeScale", 0.5f);
SettingT<GameSettingCollection> fMagicCloudSpeedScale("fMagicCloudSpeedScale", 0.75f);
SettingT<GameSettingCollection> fMagicCloudSizeScale("fMagicCloudSizeScale", 2.0f);
SettingT<GameSettingCollection> fMagicCloudSlowdownRate("fMagicCloudSlowdownRate", 0.2f);
SettingT<GameSettingCollection> fMagicCloudFindTargetTime("fMagicCloudFindTargetTime", 0.25f);
SettingT<GameSettingCollection> fMagicCloudDurationMin("fMagicCloudDurationMin", 5.0f);
SettingT<GameSettingCollection> fMagicCloudAreaMin("fMagicCloudAreaMin", 1.0f);
SettingT<GameSettingCollection> fMagickaReturnBase("fMagickaReturnBase", 0.75f);
SettingT<GameSettingCollection> fMagickaReturnMult("fMagickaReturnMult", 0.02f);
SettingT<GameSettingCollection> fFatigueCastBase("fFatigueCastBase", 15.0f);
SettingT<GameSettingCollection> fFatigueCastMult("fFatigueCastMult", 0.0f);
SettingT<GameSettingCollection> fMagicArmorPenaltyMin("fMagicArmorPenaltyMin", 0.05f);
SettingT<GameSettingCollection> fMagicArmorPenaltyMax("fMagicArmorPenaltyMax", 0.3f);
SettingT<GameSettingCollection> fWortcraftChanceIntDenom("fWortcraftChanceIntDenom", 5.0f);
SettingT<GameSettingCollection> fWortcraftChanceLuckDenom("fWortcraftChanceLuckDenom", 10.0f);
SettingT<GameSettingCollection> fWortcraftStrChanceDenom("fWortcraftStrChanceDenom", 4.0f);
SettingT<GameSettingCollection> fWortcraftStrCostDenom("fWortcraftStrCostDenom", 10.0f);
static SettingT<GameSettingCollection> fWortAlchmult("fWortalchmult", 1.0f);
SettingT<GameSettingCollection> fWortStrMult("fWortStrMult", 0.2f);
SettingT<GameSettingCollection> fWortcraftFatigueMag("fWortcraftFatigueMag", 5.0f);
SettingT<GameSettingCollection> fMagicSunDamageBaseDamage("fMagicSunDamageBaseDamage", 2.0f);
SettingT<GameSettingCollection> fMagicSunDamageWaterScale("fMagicSunDamageWaterScale", 0.2f);
SettingT<GameSettingCollection> fMagicSunDamageSunHiddenScale("fMagicSunDamageSunHiddenScale", 0.2f);
SettingT<GameSettingCollection> fMagicSunDamageMinWeather("fMagicSunDamageMinWeather", 0.0f);
SettingT<GameSettingCollection> fMagicSunDamageScreenGlowMult("fMagicSunDamageScreenGlowMult", 3.0f);
SettingT<GameSettingCollection> fMagicSunDamageScreenGlowRateUp("fMagicSunDamageScreenGlowRateUp", 1.0f);
SettingT<GameSettingCollection> fMagicSunDamageScreenGlowRateDown("fMagicSunDamageScreenGlowRateDown", 1.0f);
SettingT<GameSettingCollection> fMagicSunDamagePainTimer("fMagicSunDamagePainTimer", 1000000000000.0f);
SettingT<GameSettingCollection> fMagicSunDamagePainInitialDelay("fMagicSunDamagePainInitialDelay", 2.0f);
SettingT<GameSettingCollection> fSpellmakingGoldMult("fSpellmakingGoldMult", 1.0f);
SettingT<GameSettingCollection> fEnchantmentGoldMult("fEnchantmentGoldMult", 1.0f);
SettingT<GameSettingCollection> fAlchemyGoldMult("fAlchemyGoldMult", 1.0f);
SettingT<GameSettingCollection> fItemPointsMult("fItemPointsMult", 1.0f);
SettingT<GameSettingCollection> fMagicPlayerMinimumInvisibility("fMagicPlayerMinimumInvisibility", 0.1f);
SettingT<GameSettingCollection> iMagicLightMaxCount("iMagicLightMaxCount", 4);
SettingT<GameSettingCollection> fMagicLightForwardOffset("fMagicLightForwardOffset", 22.0f);
SettingT<GameSettingCollection> fMagicLightHeightOffset("fMagicLightHeightOffset", 10.0f);
SettingT<GameSettingCollection> fMagicLightSideOffset("fMagicLightSideOffset", 0.0f);
SettingT<GameSettingCollection> fMagicLightRadiusBase("fMagicLightRadiusBase", 0.0f);
SettingT<GameSettingCollection> fMagicTelekinesisMoveBase("fMagicTelekinesisMoveBase", 50.0f);
SettingT<GameSettingCollection> fMagicTelekinesisMoveAccelerate("fMagicTelekinesisMoveAccelerate", 400.0f);
SettingT<GameSettingCollection> fMagicTelekinesisMoveMax("fMagicTelekinesisMoveMax", 1000.0f);
SettingT<GameSettingCollection> fMagicTelekinesisDistanceMin("fMagicTelekinesisDistanceMin", 48.0f);
SettingT<GameSettingCollection> fMagicTelekinesisThrow("fMagicTelekinesisThrow", 1000.0f);
SettingT<GameSettingCollection> fMagicTelekinesisSpringDamping("fMagicTelekinesisSpringDamping", 0.5f);
SettingT<GameSettingCollection> fMagicTelekinesisSpringElasticity("fMagicTelekinesisSpringElasticity", 0.075f);
SettingT<GameSettingCollection> fMagicTelekinesisObjectDamping("fMagicTelekinesisObjectDamping", 0.95f);
SettingT<GameSettingCollection> fMagicTelekinesisMaxForce("fMagicTelekinesisMaxForce", 1000.0f);
SettingT<GameSettingCollection> fMagicTelekinesisComplexSpringDamping("fMagicTelekinesisComplexSpringDamping", 0.9f);
SettingT<GameSettingCollection> fMagicTelekinesisComplexSpringElasticity("fMagicTelekinesisComplexSpringElasticity",
	0.1f);
SettingT<GameSettingCollection> fMagicTelekinesisComplexObjectDamping("fMagicTelekinesisComplexObjectDamping", 0.7f);
SettingT<GameSettingCollection> fMagicTelekinesisComplexMaxForce("fMagicTelekinesisComplexMaxForce", 1000.0f);
SettingT<GameSettingCollection> fMagicTelekinesisLiftPowerMult("fMagicTelekinesisLiftPowerMult", 0.5f);
SettingT<GameSettingCollection> fMagicTelekinesiDistanceMult("fMagicTelekinesiDistanceMult", 1.0f);
SettingT<GameSettingCollection> fMagicNightEyeAmbient("fMagicNightEyeAmbient", 0.75f);
SettingT<GameSettingCollection> iMagicMaxPotionsNovice("iMagicMaxPotionsNovice", 5);
SettingT<GameSettingCollection> iMagicMaxPotionsApprentice("iMagicMaxPotionsApprentice", 5);
SettingT<GameSettingCollection> iMagicMaxPotionsJourneyman("iMagicMaxPotionsJourneyman", 7);
SettingT<GameSettingCollection> iMagicMaxPotionsExpert("iMagicMaxPotionsExpert", 7);
SettingT<GameSettingCollection> iMagicMaxPotionsMaster("iMagicMaxPotionsMaster", 10);
SettingT<GameSettingCollection> fMagicLevelMagnitudeMult("fMagicLevelMagnitudeMult", 0.25f);
SettingT<GameSettingCollection> fMagicDispelMagnitudeMult("fMagicDispelMagnitudeMult", 5.0f);
SettingT<GameSettingCollection> fMagicCEEnchantMagOffset("fMagicCEEnchantMagOffset", 5.0f);
SettingT<GameSettingCollection> fMagicDiseaseTransferBase("fMagicDiseaseTransferBase", 0.0f);
SettingT<GameSettingCollection> fMagicDiseaseTransferMult("fMagicDiseaseTransferMult", 0.25f);
SettingT<GameSettingCollection> fMagicTrackingLimit("fMagicTrackingLimit", 3.0f);
SettingT<GameSettingCollection> fMagicTrackingLimitComplex("fMagicTrackingLimitComplex", 1.0f);
SettingT<GameSettingCollection> fMagicTrackingMultBall("fMagicTrackingMultBall", 1.0f);
SettingT<GameSettingCollection> fMagicTrackingMultBolt("fMagicTrackingMultBolt", 0.5f);
SettingT<GameSettingCollection> fMagicTrackingMultFog("fMagicTrackingMultFog", 1.0f);
SettingT<GameSettingCollection> fShockBoltsLength("fShockBoltsLength", 1024.0f);
SettingT<GameSettingCollection> fShockBoltsRadius("fShockBoltsRadius", 48.0f);
SettingT<GameSettingCollection> fShockBoltsRadiusStrength("fShockBoltsRadiusStrength", 3.0f);
SettingT<GameSettingCollection> fShockBoltSmallWidth("fShockBoltSmallWidth", 3.0f);
SettingT<GameSettingCollection> fShockBoltGrowWidth("fShockBoltGrowWidth", 1.0f);
SettingT<GameSettingCollection> iShockNumBolts("iShockNumBolts", 8);
SettingT<GameSettingCollection> fShockCastVOffset("fShockCastVOffset", -5.0f);
SettingT<GameSettingCollection> fShockCoreColorR("fShockCoreColorR", 1.0f);
SettingT<GameSettingCollection> fShockCoreColorG("fShockCoreColorG", 1.0f);
SettingT<GameSettingCollection> fShockCoreColorB("fShockCoreColorB", 1.0f);
SettingT<GameSettingCollection> fShockGlowColorR("fShockGlowColorR", 0.0f);
SettingT<GameSettingCollection> fShockGlowColorG("fShockGlowColorG", 0.6f);
SettingT<GameSettingCollection> fShockGlowColorB("fShockGlowColorB", 1.0f);
SettingT<GameSettingCollection> iShockSegmentsPerBolt("iShockSegmentsPerBolt", 25);
SettingT<GameSettingCollection> fShockSegmentLength("fShockSegmentLength", 32.0f);
SettingT<GameSettingCollection> fShockSegmentVariance("fShockSegmentVariance", 20.0f);
SettingT<GameSettingCollection> iShockSubSegments("iShockSubSegments", 1);
SettingT<GameSettingCollection> fShockSubSegmentVariance("fShockSubSegmentVariance", 0.0f);
SettingT<GameSettingCollection> iShockBranchNumBolts("iShockBranchNumBolts", 3);
SettingT<GameSettingCollection> fShockBranchBoltsRadius("fShockBranchBoltsRadius", 24.0f);
SettingT<GameSettingCollection> fShockBranchBoltsRadiusStrength("fShockBranchBoltsRadiusStrength", 3.0f);
SettingT<GameSettingCollection> iShockBranchSegmentsPerBolt("iShockBranchSegmentsPerBolt", 50);
SettingT<GameSettingCollection> fShockBranchSegmentLength("fShockBranchSegmentLength", 32.0f);
SettingT<GameSettingCollection> fShockBranchSegmentVariance("fShockBranchSegmentVariance", 20.0f);
SettingT<GameSettingCollection> gs_fSubSegmentVariance("fSubSegmentVariance", 0.0f);
SettingT<GameSettingCollection> fShockBranchLifetime("fShockBranchLifetime", 1.0f);
SettingT<GameSettingCollection> iShockDebug("iShockDebug", 0);
SettingT<GameSettingCollection> fAbsorbBoltsRadius("fAbsorbBoltsRadius", 5.0f);
SettingT<GameSettingCollection> fAbsorbBoltsRadiusStrength("fAbsorbBoltsRadiusStrength", 3.0f);
SettingT<GameSettingCollection> fAbsorbSegmentVariance("fAbsorbSegmentVariance", 7.0f);
SettingT<GameSettingCollection> fAbsorbTortuosityVariance("fAbsorbTortuosityVariance", 2.0f);
SettingT<GameSettingCollection> fAbsorbBoltSmallWidth("fAbsorbBoltSmallWidth", 7.0f);
SettingT<GameSettingCollection> fAbsorbBoltGrowWidth("fAbsorbBoltGrowWidth", 0.0f);
SettingT<GameSettingCollection> iAbsorbNumBolts("iAbsorbNumBolts", 1);
SettingT<GameSettingCollection> fAbsorbCoreColorR("fAbsorbCoreColorR", 1.0f);
SettingT<GameSettingCollection> fAbsorbCoreColorG("fAbsorbCoreColorG", 1.0f);
SettingT<GameSettingCollection> fAbsorbCoreColorB("fAbsorbCoreColorB", 1.0f);
SettingT<GameSettingCollection> fAbsorbGlowColorR("fAbsorbGlowColorR", 0.0f);
SettingT<GameSettingCollection> fAbsorbGlowColorG("fAbsorbGlowColorG", 0.6f);
SettingT<GameSettingCollection> fAbsorbGlowColorB("fAbsorbGlowColorB", 1.0f);
SettingT<GameSettingCollection> fAbsorbMoveSpeed("fAbsorbMoveSpeed", -500.0f);
SettingT<GameSettingCollection> fAbsorbSegmentLength("fAbsorbSegmentLength", 10.0f);
SettingT<GameSettingCollection> fMagicCostScale("fMagicCostScale", 1.25f);
SettingT<GameSettingCollection> fScrollCostMult("fScrollCostMult", 0.5f);
SettingT<GameSettingCollection> iMaxPlayerSummonedCreatures("iMaxPlayerSummonedCreatures", 1);
SettingT<GameSettingCollection> iMagnitudeLevelAffectsAll("iMagnitudeLevelAffectsAll", 25);
SettingT<GameSettingCollection> fReflectedAbsorbChanceReduction("fReflectedAbsorbChanceReduction", 0.5f);
SettingT<GameSettingCollection> fActiveEffectConditionUpdateInterval("fActiveEffectConditionUpdateInterval", 1.0f);
SettingT<GameSettingCollection> fConcussionTimer("fConcussionTimer", 15.0f);
SettingT<GameSettingCollection> fMagicVACPartTargetedMult("fMagicVACPartTargetedMult", 0.1f);
SettingT<GameSettingCollection> fMagicVACNoPartTargetedMult("fMagicVACNoPartTargetedMult", 0.1f);
SettingT<GameSettingCollection> fAddictionUsageMonitorThreshold("fAddictionUsageMonitorThreshold", 25.0f);
SettingT<GameSettingCollection> fMagicMedicineSkillBase("fMagicMedicineSkillBase", 1.0f);
SettingT<GameSettingCollection> fMagicMedicineSkillMult("fMagicMedicineSkillMult", 0.5f);
SettingT<GameSettingCollection> fMagicSurvivalSkillBase("fMagicSurvivalSkillBase", 1.0f);
SettingT<GameSettingCollection> fMagicSurvivalSkillMult("fMagicSurvivalSkillMult", 0.5f);

float hypotf_3(float X, float Y) {
	return std::pow(X, Y);
}

float MagicFormulas::CalcSunDamage(float afMagnitude, float afTimeOfDay, float afSunrise, float afSunset,
	bool abSunVisible, float afWeatherScale, bool abUnderWater) {
	if (!(afSunrise > static_cast<double>(afTimeOfDay)) && !(afSunset < static_cast<double>(afTimeOfDay))) {
		double v8;
		float v10;
		float v11;
		float v15 = 12.0f;
		if (!(12.0f < static_cast<double>(afTimeOfDay))) {
			v8 = v15 - afSunrise;
		} else {
			v8 = v15 - afSunset;
		}
		float X = (v15 - afTimeOfDay) / v8;
		float v17 = 1.0 - hypotf_3(X, 4.0f);
		double v12 = (1.0 - fMagicSunDamageMinWeather.Float()) * afWeatherScale;
		float v13 = fMagicSunDamageMinWeather.Float() + v12;
		if (abSunVisible) {
			v11 = 1.0;
		} else {
			v11 = fMagicSunDamageSunHiddenScale.Float();
		}
		if (abUnderWater) {
			v10 = fMagicSunDamageWaterScale.Float();
		} else {
			v10 = 1.0;
		}
		return fMagicSunDamageBaseDamage.Float() * afMagnitude * v17 * v13 * v10 * v11;
	} else {
		return 0.0;
	}
}

// Each factor is rounded to a float local; the settings are read through Float, and the float BSMAX and the float
// overload of pow are used.
float MagicFormulas::GetMagicEffectCost(float afBaseEffectCost, int32_t aiArea, int32_t aiDuration, int32_t aiMagnitude,
	bool abIsRangeTarget) {
	float fArea = static_cast<float>(aiArea * static_cast<double>(fMagicAreaBaseCostMult.Float()));
	float fAreaMult = BSMAX(fArea, 1.0f);
	float fDuration;
	if (aiDuration) {
		fDuration = static_cast<float>(aiDuration);
	} else {
		fDuration = 1.0f;
	}
	float* pDurMagMult = &fMagicDurMagBaseCostMult.Float();
	float fDurMag = static_cast<float>(static_cast<double>(afBaseEffectCost) * *pDurMagMult * fDuration);
	float fMagnitude;
	if (aiMagnitude) {
		float* pCostScale = &fMagicCostScale.Float();
		fMagnitude = std::pow(static_cast<float>(aiMagnitude), *pCostScale);
	} else {
		fMagnitude = 1.0f;
	}
	float fMagnitudeMult = fMagnitude;
	float fCost = static_cast<float>(static_cast<double>(fAreaMult) * fDurMag * fMagnitudeMult);
	float fRangeMult;
	if (abIsRangeTarget) {
		fRangeMult = fMagicRangeTargetCostMult.Float();
	} else {
		fRangeMult = 1.0f;
	}
	float fResult = static_cast<float>(static_cast<double>(fRangeMult) * fCost);
	return fResult;
}

float MagicFormulas::AdjustMagicSpellCost(float afSpellCost, int32_t aiCasterSkill, int32_t aiCasterLuck) {
	float v8 = static_cast<double>(aiCasterSkill) / 100.0;
	float* v3 = &fMagicCasterSkillCostBase.Float();
	float v7 = fMagicCasterSkillCostMult.Float() * (1.0 - v8) + *v3;
	return afSpellCost * v7;
}

// Both compares are against the double 100.0 and a NaN resistance passes them; each fraction is rounded to a float
// local, and the result is rounded to float.
float MagicFormulas::GetMagicTargetResistance(float afTargetMagicTypeResistance, float afTargetEffectResistance) {
	if (afTargetMagicTypeResistance >= 100.0 || afTargetEffectResistance >= 100.0)
		return 0.0f;
	float fMagicTypeResistance = static_cast<float>(afTargetMagicTypeResistance / 100.0);
	float fEffectResistance = static_cast<float>(afTargetEffectResistance / 100.0);
	float fResult = static_cast<float>(1.0 - ((1.0 - fMagicTypeResistance) * fEffectResistance + fMagicTypeResistance));
	return fResult;
}

bool MagicFormulas::CheckTargetLevelMagnitude(int32_t aiTargetLevel, float afMagnitude) {
	float v3 = afMagnitude * ((&::fMagicLevelMagnitudeMult)->Float());
	return (bool)((uint8_t)((v3 >= ((double)((int32_t)((&::iMagnitudeLevelAffectsAll)->UInt())))) ||
		(v3 >= ((double)aiTargetLevel))));
}

float MagicFormulas::CheckExplosionStrength(int32_t aiCost, int32_t aiArea, float afDistance, int32_t aiAgility,
	[[maybe_unused]] int32_t aiLuck, [[maybe_unused]] float afFatiguePercentage) {
	float fFalloff;
	if (aiArea <= 0) {
		fFalloff = 1.0f;
	} else {
		float fDistanceRatio = afDistance / static_cast<double>(aiArea);
		float fRatioSquared = fDistanceRatio * fDistanceRatio;
		fFalloff = 1.0 - fRatioSquared;
	}
	double dScaledCost = fMagicExplosionPowerMult.Float() * fFalloff * static_cast<double>(aiCost);
	float fStrength = fMagicExplosionPowerBase.Float() + dScaledCost;
	float fAgility = static_cast<float>(aiAgility);
	return fStrength - fAgility * fMagicExplosionAgilityMult.Float();
}

float MagicFormulas::CalcMedicineEffectivenessMult(ActorValueOwner* apActor) {
	float fMedicinePercent = (float)(apActor->GetClampedActorFloatValue(ActorValue::MEDICINE) / 100.0);
	float* fBase = &(&::fMagicMedicineSkillBase)->Float();
	return (float)((fMedicinePercent * ((&::fMagicMedicineSkillMult)->Float())) + (*fBase));
}

float MagicFormulas::CalcSurvivalEffectivenessMult(ActorValueOwner* apActor) {
	float v5 = (float)(apActor->GetClampedActorFloatValue(ActorValue::SURVIVAL) / 100.0);
	float* v1 = &(&::fMagicSurvivalSkillBase)->Float();
	return (float)((v5 * ((&::fMagicSurvivalSkillMult)->Float())) + (*v1));
}

SkillLevel::Value MagicFormulas::GetMagicSpellLevel(float afSpellCost) {
	float* v1 = &fMagicSpellLevelCostBase.Float();
	float v7 = fMagicSpellLevelCostMult.Float() * afSpellCost + *v1;
	if (fMagicSpellLevelApprenticeMin.Float() > static_cast<double>(v7)) {
		return SkillLevel::NOVICE;
	}
	if (fMagicSpellLevelJourneymanMin.Float() > static_cast<double>(v7)) {
		return SkillLevel::APPRENTICE;
	}
	if (fMagicSpellLevelExpertMin.Float() > static_cast<double>(v7)) {
		return SkillLevel::JOURNEYMAN;
	}
	if (!(fMagicSpellLevelMasterMin.Float() > static_cast<double>(v7))) {
		return SkillLevel::MASTER;
	} else {
		return SkillLevel::EXPERT;
	}
}
