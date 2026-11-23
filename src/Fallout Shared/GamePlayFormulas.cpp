#include "Fallout Shared/GamePlayFormulas.h"

#include "BSCore/BSRandom.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/Main.h"

#include <cmath>

// The difficulty settings stand above the tables that point to them (file-local, so declared before use).
static SettingT<GameSettingCollection> fDiffMultHPToPC_VE("fDiffMultHPToPCVE", 0.5f);
static SettingT<GameSettingCollection> fDiffMultHPToPC_E("fDiffMultHPToPCE", 0.75f);
static SettingT<GameSettingCollection> fDiffMultHPToPC_N("fDiffMultHPToPCN", 1.0f);
static SettingT<GameSettingCollection> fDiffMultHPToPC_H("fDiffMultHPToPCH", 1.5f);
static SettingT<GameSettingCollection> fDiffMultHPToPC_VH("fDiffMultHPToPCVH", 2.0f);
static SettingT<GameSettingCollection> fDiffMultHPByPC_VE("fDiffMultHPByPCVE", 2.0f);
static SettingT<GameSettingCollection> fDiffMultHPByPC_E("fDiffMultHPByPCE", 1.5f);
static SettingT<GameSettingCollection> fDiffMultHPByPC_N("fDiffMultHPByPCN", 1.0f);
static SettingT<GameSettingCollection> fDiffMultHPByPC_H("fDiffMultHPByPCH", 0.75f);
static SettingT<GameSettingCollection> fDiffMultHPByPC_VH("fDiffMultHPByPCVH", 0.5f);
static SettingT<GameSettingCollection> fDiffMultXP_VE("fDiffMultXPVE", 0.5f);
static SettingT<GameSettingCollection> fDiffMultXP_E("fDiffMultXPE", 0.75f);
static SettingT<GameSettingCollection> fDiffMultXP_N("fDiffMultXPN", 1.0f);
static SettingT<GameSettingCollection> fDiffMultXP_H("fDiffMultXPH", 1.5f);
static SettingT<GameSettingCollection> fDiffMultXP_VH("fDiffMultXPVH", 2.0f);
fDiffMultHPToPC_t fDiffMultHPToPC{
    &fDiffMultHPToPC_VE,
    &fDiffMultHPToPC_E,
    &fDiffMultHPToPC_N,
    &fDiffMultHPToPC_H,
    &fDiffMultHPToPC_VH
};
fDiffMultHPByPC_t fDiffMultHPByPC{
    &fDiffMultHPByPC_VE,
    &fDiffMultHPByPC_E,
    &fDiffMultHPByPC_N,
    &fDiffMultHPByPC_H,
    &fDiffMultHPByPC_VH
};
fDiffMultXP_t fDiffMultXP{&fDiffMultXP_VE, &fDiffMultXP_E, &fDiffMultXP_N, &fDiffMultXP_H, &fDiffMultXP_VH};

// The game settings this file defines, with their IDs and values, in initialization order.
static SettingT<GameSettingCollection> fActorLookForceSetting("fActorLookForce", 500.0f);
SettingT<GameSettingCollection> iHeavyArmorStealthPenalty("iHeavyArmorStealthPenalty", 20);
SettingT<GameSettingCollection> iMediumArmorStealthPenalty("iMediumArmorStealthPenalty", 10);
SettingT<GameSettingCollection> fAgilityReloadBase("fAgilityReloadBase", 5.0f);
SettingT<GameSettingCollection> fAgilityReloadModifier("fAgilityReloadModifier", 0.1f);
SettingT<GameSettingCollection> fItemDetectionTimer("fItemDetectionTimer", 5.0f);
SettingT<GameSettingCollection> sAchieveSMMenaceName("sAchieveSMMenaceName", "Mutant Menace");
SettingT<GameSettingCollection> sAchieveSMMenaceDesc(
    "sAchieveSMMenaceDesc",
    "Your nuetralization of Super Mutants is legendary and provides a bonus to attacks against them."
);
SettingT<GameSettingCollection> sAchieveSMMasterName("sAchieveSMMasterName", "Mutant Master");
SettingT<GameSettingCollection> sAchieveSMMasterDesc("sAchieveSMMasterDesc", "You kill Super Mutants --  A lot.");
SettingT<GameSettingCollection> iMolotovTest("iMolotovTest", 0);
SettingT<GameSettingCollection> iExplosionDebug("iExplosionDebug", 0);
SettingT<GameSettingCollection> fProjectileDefaultTracerRange("fProjectileDefaultTracerRange", 10000.0f);
SettingT<GameSettingCollection> fProjectileReorientTracerMin("fProjectileReorientTracerMin", 200.0f);
SettingT<GameSettingCollection> fPathPointZDistanceAllowance("fPathPointZDistanceAllowance", 10.0f);
SettingT<GameSettingCollection> fPathPointTurningSpeed("fPathPointTurningSpeed", 10.0f);
SettingT<GameSettingCollection> fLockpickBonusHealth("fLockpickBonusHealth", 0.01f);
SettingT<GameSettingCollection> iVatsTargetAngle("iVatsTargetAngle", 15);
SettingT<GameSettingCollection> fHavokTauRatio("fHavokTauRatio", 0.5f);
SettingT<GameSettingCollection> gs_sYesText("sYesText", "&Yes");
SettingT<GameSettingCollection> gs_sNoText("sNoText", "&No");
SettingT<GameSettingCollection> gs_sYesToAllText("sYesToAllText", "Yes to &All");
static SettingT<GameSettingCollection> pAbortText("sAbortText", "&Abort");
static SettingT<GameSettingCollection> pRetryText("sRetryText", "&Retry");
static SettingT<GameSettingCollection> pIgnoreText("sIgnoreText", "&Ignore");
static SettingT<GameSettingCollection> pOKText("sOKText", "&OK");
static SettingT<GameSettingCollection> pContinueText("sContinueText", "Continue Running Executable?");
SettingT<GameSettingCollection> fHCFlashTextThreshold("fHCFlashTextThreshold", 800.0f);
SettingT<GameSettingCollection> fHCRedTextThreshold("fHCRedTextThreshold", 600.0f);
SettingT<GameSettingCollection> fHCFullAlphaTextThreshold("fHCFullAlphaTextThreshold", 400.0f);
SettingT<GameSettingCollection> fHCHalfAlphaTextThreshold("fHCHalfAlphaTextThreshold", 200.0f);
SettingT<GameSettingCollection> fHCDehydrationRate("fHCDehydrationRate", 10.0f);
SettingT<GameSettingCollection> fHCSleepDeprivationRate("fHCSleepDeprivationRate", 0.26f);
SettingT<GameSettingCollection> fHCStarvationRate("fHCStarvationRate", 0.39f);
SettingT<GameSettingCollection> fHCSleepRestorationMod("fHCSleepRestorationMod", 60.0f);
SettingT<GameSettingCollection> fHelloCooldownTime("fHelloCooldownTime", 30.0f);
SettingT<GameSettingCollection> fImprovedDetectModifier("fImprovedDetectModifier", 0.25f);
SettingT<GameSettingCollection> fBSUnitsPerFoot("fBSUnitsPerFoot", 22.0f);
SettingT<GameSettingCollection> iActorLuckSkillBase("iActorLuckSkillBase", -20);
SettingT<GameSettingCollection> fActorLuckSkillMult("fActorLuckSkillMult", 0.4f);
SettingT<GameSettingCollection> fArmorWeightLightMaxMod("fArmorWeightLightMaxMod", 0.6f);
SettingT<GameSettingCollection> iArmorBaseSkill("iArmorBaseSkill", 50);
SettingT<GameSettingCollection> fActorWeaponDesirabilityDamageMult("fActorWeaponDesirabilityDamageMult", 0.5f);
SettingT<GameSettingCollection> fActorWeaponDesirabilitySkillMult("fActorWeaponDesirabilitySkillMult", 10.0f);
SettingT<GameSettingCollection> fActorArmorDesirabilityDamageMult("fActorArmorDesirabilityDamageMult", 0.5f);
SettingT<GameSettingCollection> fActorArmorDesirabilitySkillMult("fActorArmorDesirabilitySkillMult", 10.0f);
SettingT<GameSettingCollection> fValueofItemForNoOwnership("fValueofItemForNoOwnership", 45.0f);
SettingT<GameSettingCollection> fMoveMediumArmorPenalty("fMoveMediumArmorPenalty", 0.1f);
SettingT<GameSettingCollection> fMoveHeavyArmorPenalty("fMoveHeavyArmorPenalty", 0.2f);
SettingT<GameSettingCollection> fMove2HRPenalty("fMove2HRPenalty", 0.1f);
SettingT<GameSettingCollection> fMove2HBigPenalty("fMove2HBigPenalty", 0.15f);
SettingT<GameSettingCollection> fMoveRunMult("fMoveRunMult", 4.0f);
SettingT<GameSettingCollection> fMoveNoWeaponMult("fMoveNoWeaponMult", 1.1f);
SettingT<GameSettingCollection> fMoveSneakMult("fMoveSneakMult", 0.6f);
SettingT<GameSettingCollection> fMoveBaseSpeed("fMoveBaseSpeed", 85.0f);
SettingT<GameSettingCollection> fMoveLegConditionMult("fMoveLegConditionMult", 0.9f);
SettingT<GameSettingCollection> fMoveSwimWalkBase("fMoveSwimWalkBase", 0.5f);
SettingT<GameSettingCollection> fMoveSwimWalkAthleticsMult("fMoveSwimWalkAthleticsMult", 0.02f);
SettingT<GameSettingCollection> fMoveSwimRunBase("fMoveSwimRunBase", 0.5f);
SettingT<GameSettingCollection> fMoveSwimRunAthleticsMult("fMoveSwimRunAthleticsMult", 0.1f);
SettingT<GameSettingCollection> fMoveMinFlySpeed("fMoveMinFlySpeed", 5.0f);
SettingT<GameSettingCollection> fMoveMaxFlySpeed("fMoveMaxFlySpeed", 300.0f);
SettingT<GameSettingCollection> fJumpFallHeightMin("fJumpFallHeightMin", 256.0f);
SettingT<GameSettingCollection> fJumpFallHeightExponent("fJumpFallHeightExponent", 2.0f);
SettingT<GameSettingCollection> fJumpFallHeightMult("fJumpFallHeightMult", 0.025f);
static SettingT<GameSettingCollection> fJumpFallVelocityMin("fJumpFallVelocityMin", 600.0f);
SettingT<GameSettingCollection> fJumpFallSkillBase("fJumpFallSkillBase", 1.25f);
SettingT<GameSettingCollection> fJumpFallSkillMult("fJumpFallSkillMult", -0.01f);
SettingT<GameSettingCollection> fJumpFallRiderMult("fJumpFallRiderMult", 0.0f);
SettingT<GameSettingCollection> fJumpHeightMin("fJumpHeightMin", 64.0f);
SettingT<GameSettingCollection> fJumpSwimmingMult("fJumpSwimmingMult", 2.0f);
SettingT<GameSettingCollection> fJumpDoubleMult("fJumpDoubleMult", 0.5f);
static SettingT<GameSettingCollection> fJumpMoveBase("fJumpMoveBase", 0.0f);
static SettingT<GameSettingCollection> fJumpMoveMult("fJumpMoveMult", 0.3f);
SettingT<GameSettingCollection> iPerkHeavyArmorJumpSum("iPerkHeavyArmorJumpSum", 35);
SettingT<GameSettingCollection> iPerkHeavyArmorSinkSum("iPerkHeavyArmorSinkSum", 35);
SettingT<GameSettingCollection> fPerkHeavyArmorSinkGravityMult("fPerkHeavyArmorSinkGravityMult", 15.0f);
SettingT<GameSettingCollection> fPerkHeavyArmorExpertSpeedMult("fPerkHeavyArmorExpertSpeedMult", 0.5f);
SettingT<GameSettingCollection> fPerkHeavyArmorMasterSpeedMult("fPerkHeavyArmorMasterSpeedMult", 0.0f);
SettingT<GameSettingCollection> fPerkLightArmorExpertSpeedMult("fPerkLightArmorExpertSpeedMult", 0.0f);
SettingT<GameSettingCollection> fPerkLightArmorNoviceDamageMult("fPerkLightArmorNoviceDamageMult", 1.5f);
SettingT<GameSettingCollection> fPerkHeavyArmorNoviceDamageMult("fPerkHeavyArmorNoviceDamageMult", 1.5f);
SettingT<GameSettingCollection> fPerkHeavyArmorJourneymanDamageMult("fPerkHeavyArmorJourneymanDamageMult", 0.5f);
SettingT<GameSettingCollection> fPerkLightArmorJourneymanDamageMult("fPerkLightArmorJourneymanDamageMult", 0.5f);
SettingT<GameSettingCollection> fPerkJumpFatigueExpertMult("fPerkJumpFatigueExpertMult", 0.5f);
SettingT<GameSettingCollection> iHorseTurnDegreesPerSecond("iHorseTurnDegreesPerSecond", 45);
SettingT<GameSettingCollection> iHorseTurnDegreesRampUpPerSecond("iHorseTurnDegreesRampUpPerSecond", 80);
SettingT<GameSettingCollection> fMinDistanceUseHorse("fMinDistanceUseHorse", 10000.0f);
SettingT<GameSettingCollection> fFallLegDamageMult("fFallLegDamageMult", 0.5f);
SettingT<GameSettingCollection> iFallLegDamageChance("iFallLegDamageChance", 50);
SettingT<GameSettingCollection> fFatigueReturnBase("fFatigueReturnBase", 1.0f);
SettingT<GameSettingCollection> fFatigueReturnMult("fFatigueReturnMult", 0.0f);
SettingT<GameSettingCollection> fActorStrengthEncumbranceMult("fActorStrengthEncumbranceMult", 5.0f);
SettingT<GameSettingCollection> fActorSwimBreathBase("fActorSwimBreathBase", 10.0f);
SettingT<GameSettingCollection> fActorSwimBreathMult("fActorSwimBreathMult", 0.5f);
SettingT<GameSettingCollection> fActorSwimBreathDamage("fActorSwimBreathDamage", 0.2f);
SettingT<GameSettingCollection> fBarterBuyBase("fBarterBuyBase", 1.5f);
SettingT<GameSettingCollection> fBarterSellBase("fBarterSellBase", 0.5f);
SettingT<GameSettingCollection> fBarterBuyMult("fBarterBuyMult", -0.5f);
SettingT<GameSettingCollection> fBarterSellMult("fBarterSellMult", 0.5f);
SettingT<GameSettingCollection> fItemConditionValueMult("fItemConditionValueMult", 0.03162f);
SettingT<GameSettingCollection> fItemConditionValueExp("fItemConditionValueExp", 1.5f);
SettingT<GameSettingCollection> fItemConditionValueBase("fItemConditionValueBase", 0.25f);
SettingT<GameSettingCollection> fBarterDispBase("fBarterDispBase", 1.5f);
SettingT<GameSettingCollection> fBarterHaggleDispMult("fBarterHaggleDispMult", 0.5f);
SettingT<GameSettingCollection> fBarterHaggleBase("fBarterHaggleBase", 0.55f);
SettingT<GameSettingCollection> fBarterHaggleMax("fBarterHaggleMax", 40.0f);
SettingT<GameSettingCollection> fBarterHaggleCurve("fBarterHaggleCurve", 1.0f);
SettingT<GameSettingCollection> fBarterDispositionMod("fBarterDispositionMod", 0.01f);
SettingT<GameSettingCollection> iBarterDispositionPenalty("iBarterDispositionPenalty", -1);
SettingT<GameSettingCollection> iPerkExtraBarterGoldMaster("iPerkExtraBarterGoldMaster", 200);
SettingT<GameSettingCollection> fPerkAthleticsNoviceFatigueMult("fPerkAthleticsNoviceFatigueMult", 1.0f);
SettingT<GameSettingCollection> fPerkAthleticsApprenticeFatigueMult("fPerkAthleticsApprenticeFatigueMult", 0.75f);
SettingT<GameSettingCollection> fPerkAthleticsJourneymanFatigueMult("fPerkAthleticsJourneymanFatigueMult", 0.5f);
SettingT<GameSettingCollection> fPerkAthleticsExpertFatigueMult("fPerkAthleticsExpertFatigueMult", 0.25f);
SettingT<GameSettingCollection> fPerkAthleticsMasterFatigueMult("fPerkAthleticsMasterFatigueMult", 0.0f);
SettingT<GameSettingCollection> iLevelUpSkillCount("iLevelUpSkillCount", 10);
SettingT<GameSettingCollection> iLevelUp01Mult("iLevelUp01Mult", 2);
SettingT<GameSettingCollection> iLevelUp02Mult("iLevelUp02Mult", 2);
SettingT<GameSettingCollection> iLevelUp03Mult("iLevelUp03Mult", 2);
SettingT<GameSettingCollection> iLevelUp04Mult("iLevelUp04Mult", 2);
SettingT<GameSettingCollection> iLevelUp05Mult("iLevelUp05Mult", 3);
SettingT<GameSettingCollection> iLevelUp06Mult("iLevelUp06Mult", 3);
SettingT<GameSettingCollection> iLevelUp07Mult("iLevelUp07Mult", 3);
SettingT<GameSettingCollection> iLevelUp08Mult("iLevelUp08Mult", 4);
SettingT<GameSettingCollection> iLevelUp09Mult("iLevelUp09Mult", 4);
SettingT<GameSettingCollection> iLevelUp10Mult("iLevelUp10Mult", 5);
SettingT<GameSettingCollection> fSkillUseSpecMult("fSkillUseSpecMult", 0.75f);
SettingT<GameSettingCollection> fSkillUseMajorMult("fSkillUseMajorMult", 0.75f);
SettingT<GameSettingCollection> fSkillUseMinorMult("fSkillUseMinorMult", 1.25f);
SettingT<GameSettingCollection> iCrimeGoldAttack("iCrimeGoldAttack", 40);
SettingT<GameSettingCollection> iCrimeGoldMurder("iCrimeGoldMurder", 1000);
SettingT<GameSettingCollection> fCrimeGoldSteal("fCrimeGoldSteal", 0.5f);
SettingT<GameSettingCollection> iCrimeGoldPickpocket("iCrimeGoldPickpocket", 25);
SettingT<GameSettingCollection> iCrimeGoldTresspass("iCrimeGoldTresspass", 5);
SettingT<GameSettingCollection> iCrimeGoldStealHorse("iCrimeGoldStealHorse", 25);
SettingT<GameSettingCollection> iCrimeGoldMinValue("iCrimeGoldMinValue", 10);
SettingT<GameSettingCollection> fCrimeDispAttack("fCrimeDispAttack", -10.0f);
SettingT<GameSettingCollection> fCrimeDispMurder("fCrimeDispMurder", -50.0f);
SettingT<GameSettingCollection> fCrimeDispSteal("fCrimeDispSteal", -0.5f);
SettingT<GameSettingCollection> fCrimeDispPickpocket("fCrimeDispPickpocket", -25.0f);
SettingT<GameSettingCollection> fCrimeDispTresspass("fCrimeDispTresspass", -20.0f);
SettingT<GameSettingCollection> fCrimeDispPersonal("fCrimeDispPersonal", 2.0f);
SettingT<GameSettingCollection> iCrimeDaysInPrisonMod("iCrimeDaysInPrisonMod", 100);
SettingT<GameSettingCollection> fMaximumWind("fMaximumWind", 256.0f);
SettingT<GameSettingCollection> fStatsHealthStartMult("fStatsHealthStartMult", 0.5f);
SettingT<GameSettingCollection> fStatsHealthLevelMult("fStatsHealthLevelMult", 0.1f);
SettingT<GameSettingCollection> fPCBaseMagickaMult("fPCBaseMagickaMult", 0.5f);
SettingT<GameSettingCollection> fNPCBaseMagickaMult("fNPCBaseMagickaMult", 0.2f);
SettingT<GameSettingCollection> fPCBaseHealthMult("fPCBaseHealthMult", 2.0f);
SettingT<GameSettingCollection> iTrainingSkills("iTrainingSkills", 5);
SettingT<GameSettingCollection> fTorchLightLevelMorning("fTorchLightLevelMorning", 0.6f);
SettingT<GameSettingCollection> fTorchLightLevelNight("fTorchLightLevelNight", 1.2f);
SettingT<GameSettingCollection> fTorchLightLevelInterior("fTorchLightLevelInterior", 40.0f);
SettingT<GameSettingCollection> fTrainingCostMult("fTrainingCostMult", 1.0f);
SettingT<GameSettingCollection> fPersAdmireAggr("fPersAdmireAggr", -5.0f);
SettingT<GameSettingCollection> fPersAdmireConf("fPersAdmireConf", 1.0f);
SettingT<GameSettingCollection> fPersAdmireEner("fPersAdmireEner", -2.0f);
SettingT<GameSettingCollection> fPersAdmireResp("fPersAdmireResp", 2.0f);
SettingT<GameSettingCollection> fPersAdmireStre("fPersAdmireStre", 1.0f);
SettingT<GameSettingCollection> fPersAdmireIntel("fPersAdmireIntel", -1.0f);
SettingT<GameSettingCollection> fPersAdmireWillp("fPersAdmireWillp", -2.0f);
SettingT<GameSettingCollection> fPersAdmirePers("fPersAdmirePers", -1.0f);
SettingT<GameSettingCollection> fPersJokeAggr("fPersJokeAggr", 5.0f);
SettingT<GameSettingCollection> fPersJokeConf("fPersJokeConf", -1.0f);
SettingT<GameSettingCollection> fPersJokeEner("fPersJokeEner", 2.0f);
SettingT<GameSettingCollection> fPersJokeResp("fPersJokeResp", -2.0f);
SettingT<GameSettingCollection> fPersJokeStre("fPersJokeStre", 1.0f);
SettingT<GameSettingCollection> fPersJokeIntel("fPersJokeIntel", -2.0f);
SettingT<GameSettingCollection> fPersJokeWillp("fPersJokeWillp", 2.0f);
SettingT<GameSettingCollection> fPersJokePers("fPersJokePers", 1.0f);
SettingT<GameSettingCollection> fPersBullyAggr("fPersBullyAggr", -10.0f);
SettingT<GameSettingCollection> fPersBullyConf("fPersBullyConf", 2.0f);
SettingT<GameSettingCollection> fPersBullyEner("fPersBullyEner", 1.0f);
SettingT<GameSettingCollection> fPersBullyResp("fPersBullyResp", -1.0f);
SettingT<GameSettingCollection> fPersBullyStre("fPersBullyStre", 2.0f);
SettingT<GameSettingCollection> fPersBullyIntel("fPersBullyIntel", -1.0f);
SettingT<GameSettingCollection> fPersBullyWillp("fPersBullyWillp", 1.0f);
SettingT<GameSettingCollection> fPersBullyPers("fPersBullyPers", -2.0f);
SettingT<GameSettingCollection> fPersBoastAggr("fPersBoastAggr", 10.0f);
SettingT<GameSettingCollection> fPersBoastConf("fPersBoastConf", -2.0f);
SettingT<GameSettingCollection> fPersBoastEner("fPersBoastEner", -1.0f);
SettingT<GameSettingCollection> fPersBoastResp("fPersBoastResp", 1.0f);
SettingT<GameSettingCollection> fPersBoastStre("fPersBoastStre", 1.0f);
SettingT<GameSettingCollection> fPersBoastIntel("fPersBoastIntel", 2.0f);
SettingT<GameSettingCollection> fPersBoastWillp("fPersBoastWillp", -1.0f);
SettingT<GameSettingCollection> fPersBoastPers("fPersBoastPers", -2.0f);
SettingT<GameSettingCollection> iPersuasionPower1("iPersuasionPower1", 5);
SettingT<GameSettingCollection> iPersuasionPower2("iPersuasionPower2", 20);
SettingT<GameSettingCollection> iPersuasionPower3("iPersuasionPower3", 40);
SettingT<GameSettingCollection> iPersuasionAngleMin("iPersuasionAngleMin", 10);
SettingT<GameSettingCollection> iPersuasionAngleMax("iPersuasionAngleMax", 90);
SettingT<GameSettingCollection> iPersuasionBribeGold("iPersuasionBribeGold", 10);
SettingT<GameSettingCollection> iPersuasionDemandGold("iPersuasionDemandGold", 10);
SettingT<GameSettingCollection> iPersuasionOuter("iPersuasionOuter", 50);
SettingT<GameSettingCollection> iPersuasionMiddle("iPersuasionMiddle", 25);
SettingT<GameSettingCollection> iPersuasionInner("iPersuasionInner", 15);
SettingT<GameSettingCollection> iPersuasionDemandDisposition("iPersuasionDemandDisposition", -2);
SettingT<GameSettingCollection> fDetectionTimerSetting("fDetectionTimerSetting", 0.3f);
SettingT<GameSettingCollection> iDeathDropWeaponChance("iDeathDropWeaponChance", 0);
SettingT<GameSettingCollection> fSpeechCraftBase("fSpeechCraftBase", 100.0f);
SettingT<GameSettingCollection> fSpeechCraftMult("fSpeechCraftMult", 1.0f);
SettingT<GameSettingCollection> fBribeBase("fBribeBase", 10.0f);
SettingT<GameSettingCollection> fBribeMult("fBribeMult", 0.3f);
SettingT<GameSettingCollection> fDemandBase("fDemandBase", 10.0f);
SettingT<GameSettingCollection> fDemandMult("fDemandMult", 0.3f);
SettingT<GameSettingCollection> fPersuasionShape("fPersuasionShape", 10.0f);
static SettingT<GameSettingCollection> fPersuasionMinDisposition("fPersuasionMinDispostion", 5.0f);
SettingT<GameSettingCollection> fPersuasionMaxDisposition("fPersuasionMaxDisposition", 50.0f);
SettingT<GameSettingCollection> fPersuasionBaseValueShape("fPersuasionBaseValueShape", 14.5f);
static SettingT<GameSettingCollection> fPersuasionBaseValueMinDisposition("fPersuasionBaseValueMinDispostion", 0.4f);
SettingT<GameSettingCollection> fPersuasionBaseValueMaxDisposition("fPersuasionBaseValueMaxDisposition", 5.0f);
static SettingT<GameSettingCollection> fPersuasionAccuracyMinDisposition("fPersuasionAccuracyMinDispostion", 0.05f);
SettingT<GameSettingCollection> fPersuasionAccuracyMaxDisposition("fPersuasionAccuracyMaxDisposition", 1.0f);
SettingT<GameSettingCollection> fPersuasionAccuracyMinSelect("fPersuasionAccuracyMinSelect", 0.0f);
SettingT<GameSettingCollection> fPersuasionAccuracyMaxSelect("fPersuasionAccuracyMaxSelect", 100.0f);
SettingT<GameSettingCollection> fPersuasionBaseValueMinSelect("fPersuasionBaseValueMinSelect", 0.0f);
SettingT<GameSettingCollection> fPersuasionBaseValueMaxSelect("fPersuasionBaseValueMaxSelect", 100.0f);
SettingT<GameSettingCollection> fPersuasionMinSelect("fPersuasionMinSelect", 0.0f);
SettingT<GameSettingCollection> fPersuasionMaxSelect("fPersuasionMaxSelect", 100.0f);
SettingT<GameSettingCollection> iPersuasionBribeScale("iPersuasionBribeScale", 5);
SettingT<GameSettingCollection> iPersuasionBribeRefuse("iPersuasionBribeRefuse", 60);
SettingT<GameSettingCollection> iPersuasionBribeCrime("iPersuasionBribeCrime", 90);
SettingT<GameSettingCollection> iPersuasionDemandScale("iPersuasionDemandScale", 5);
SettingT<GameSettingCollection> iPersuasionDemandRefuse("iPersuasionDemandRefuse", 75);
SettingT<GameSettingCollection> fPersuasionMaxInput("fPersuasionMaxInput", 100.0f);
SettingT<GameSettingCollection> fPersuasionMinInput("fPersuasionMinInput", 0.0f);
SettingT<GameSettingCollection> fPersuasionMinPercentCircle("fPersuasionMinPercentCircle", 25.0f);
SettingT<GameSettingCollection> fRepairMin("fRepairMin", 0.5f);
SettingT<GameSettingCollection> fRepairMax("fRepairMax", 2.0f);
SettingT<GameSettingCollection> fRepairScavengeMult("fRepairScavengeMult", 0.05f);
SettingT<GameSettingCollection> fRepairSkillBase("fRepairSkillBase", 4.0f);
SettingT<GameSettingCollection> fRepairSkillMax("fRepairSkillMax", 9.0f);
SettingT<GameSettingCollection> fItemRepairCostMult("fItemRepairCostMult", 1.0f);
SettingT<GameSettingCollection> fRechargeGoldMult("fRechargeGoldMult", 2.0f);
SettingT<GameSettingCollection> fPotionMortPestleMult("fPotionMortPestleMult", 0.25f);
SettingT<GameSettingCollection> fPotionT1MagMult("fPotionT1MagMult", 2.5f);
SettingT<GameSettingCollection> fPotionT1RetMagMult("fPotionT1RetMagMult", 0.005f);
SettingT<GameSettingCollection> fPotionT1RetDurMult("fPotionT1RetDurMult", 0.003f);
SettingT<GameSettingCollection> fPotionT1CalMagMult("fPotionT1CalMagMult", 0.0035f);
SettingT<GameSettingCollection> fPotionT1CalDurMult("fPotionT1CalDurMult", 0.0025f);
SettingT<GameSettingCollection> fPotionT1AleMagMult("fPotionT1AleMagMult", -0.01f);
SettingT<GameSettingCollection> fPotionT1AleDurMult("fPotionT1AleDurMult", -0.01f);
SettingT<GameSettingCollection> fPotionT2RetDurMult("fPotionT2RetDurMult", 0.0035f);
SettingT<GameSettingCollection> fPotionT2CalDurMult("fPotionT2CalDurMult", 0.0025f);
SettingT<GameSettingCollection> fPotionT2AleDurMult("fPotionT2AleDurMult", -0.02f);
SettingT<GameSettingCollection> fPotionT3RetMagMult("fPotionT3RetMagMult", 0.005f);
SettingT<GameSettingCollection> fPotionT3CalMagMult("fPotionT3CalMagMult", 0.003f);
SettingT<GameSettingCollection> fPotionT3AleMagMult("fPotionT3AleMagMult", -0.01f);
SettingT<GameSettingCollection> fPotionGoldValueMult("fPotionGoldValueMult", 1.0f);
SettingT<GameSettingCollection> fDetectionNightEyeBonus("fDetectionNightEyeBonus", 3.0f);
SettingT<GameSettingCollection> fTorchEvaluationTimer("fTorchEvaluationTimer", 5.0f);
SettingT<GameSettingCollection> fEnchantmentPointsMult("fEnchantmentPointsMult", 0.4f);
SettingT<GameSettingCollection> fEnchantmentEffectPointsMult("fEnchantmentEffectPointsMult", 0.4f);
SettingT<GameSettingCollection> iPCStartSpellSkillLevel("iPCStartSpellSkillLevel", 30);
SettingT<GameSettingCollection> fBuoyancyStone("fBuoyancyStone", -0.75f);
SettingT<GameSettingCollection> fBuoyancySodaCan("fBuoyancySodaCan", 0.75f);
SettingT<GameSettingCollection> fBuoyancyBottle("fBuoyancyBottle", 0.75f);
SettingT<GameSettingCollection> fBuoyancyCloth("fBuoyancyCloth", 0.75f);
SettingT<GameSettingCollection> fBuoyancyDirt("fBuoyancyDirt", 0.25f);
SettingT<GameSettingCollection> fBuoyancyGlass("fBuoyancyGlass", -0.2f);
SettingT<GameSettingCollection> fBuoyancyGrass("fBuoyancyGrass", 0.25f);
SettingT<GameSettingCollection> fBuoyancyMetal("fBuoyancyMetal", -1.0f);
SettingT<GameSettingCollection> fBuoyancyOrganic("fBuoyancyOrganic", 0.75f);
SettingT<GameSettingCollection> fBuoyancySkin("fBuoyancySkin", 0.75f);
SettingT<GameSettingCollection> fBuoyancyWater("fBuoyancyWater", 0.0f);
SettingT<GameSettingCollection> fBuoyancyWood("fBuoyancyWood", 1.0f);
SettingT<GameSettingCollection> fBuoyancyMultBody("fBuoyancyMultBody", 1.0f);
SettingT<GameSettingCollection> fBuoyancyMultExtremity("fBuoyancyMultExtremity", -0.3f);
SettingT<GameSettingCollection> fSubmergedMaxSpeed("fSubmergedMaxSpeed", 2.0f);
SettingT<GameSettingCollection> fSubmergedLODDistance("fSubmergedLODDistance", 384.0f);
SettingT<GameSettingCollection> fSubmergedMaxWaterDistance("fSubmergedMaxWaterDistance", 48.0f);
SettingT<GameSettingCollection> fSubmergedLinearDampingH("fSubmergedLinearDampingH", 2.0f);
SettingT<GameSettingCollection> fSubmergedLinearDampingV("fSubmergedLinearDampingV", 4.0f);
SettingT<GameSettingCollection> fSubmergedAngularDamping("fSubmergedAngularDamping", 0.75f);
SettingT<GameSettingCollection> fSplashSoundHeavy("fSplashSoundHeavy", 50.0f);
SettingT<GameSettingCollection> fSplashSoundMedium("fSplashSoundMedium", 25.0f);
SettingT<GameSettingCollection> fSplashSoundLight("fSplashSoundLight", 5.0f);
SettingT<GameSettingCollection> fSplashSoundTimer("fSplashSoundTimer", 0.25f);
SettingT<GameSettingCollection> fSplashSoundVelocityMult("fSplashSoundVelocityMult", 0.05f);
SettingT<GameSettingCollection> fSplashSoundOutMult("fSplashSoundOutMult", 0.5f);
SettingT<GameSettingCollection> sSplashParticles("sSplashParticles", "Effects\\waterSplash.NIF");
SettingT<GameSettingCollection> fSplashScale1("fSplashScale1", 1.0f);
SettingT<GameSettingCollection> fSplashScale2("fSplashScale2", 0.75f);
SettingT<GameSettingCollection> fSplashScale3("fSplashScale3", 0.4f);
SettingT<GameSettingCollection> fGrabMaxWeightWalking("fGrabMaxWeightWalking", 100.0f);
SettingT<GameSettingCollection> fGrabMaxWeightRunning("fGrabMaxWeightRunning", 50.0f);
SettingT<GameSettingCollection> fHostileActorInteriorDistance("fHostileActorInteriorDistance", 2000.0f);
SettingT<GameSettingCollection> fHostileActorExteriorDistance("fHostileActorExteriorDistance", 3000.0f);
SettingT<GameSettingCollection> fDifficultyMinValue("fDifficultyMinValue", -2.0f);
SettingT<GameSettingCollection> fDifficultyMaxValue("fDifficultyMaxValue", 2.0f);
SettingT<GameSettingCollection> fDifficultyDefaultValue("fDifficultyDefaultValue", 0.0f);
SettingT<GameSettingCollection> fDifficultyDamageMultiplier("fDifficultyDamageMultiplier", 10.0f);
SettingT<GameSettingCollection> fClothingJewelryBase("fClothingJewelryBase", 5.0f);
SettingT<GameSettingCollection> fClothingJewelryScale("fClothingJewelryScale", 0.05f);
SettingT<GameSettingCollection> fClothingBase("fClothingBase", 0.0f);
SettingT<GameSettingCollection> fClothingClassScale("fClothingClassScale", 2.0f);
SettingT<GameSettingCollection> fClothingArmorScale("fClothingArmorScale", 0.25f);
SettingT<GameSettingCollection> fClothingArmorBase("fClothingArmorBase", 0.0f);
SettingT<GameSettingCollection> iMapMarkerRevealDistance("iMapMarkerRevealDistance", 1000);
SettingT<GameSettingCollection> iMapMarkerVisibleDistance("iMapMarkerVisibleDistance", 20000);
SettingT<GameSettingCollection> fLowLevelNPCBaseHealthMult("fLowLevelNPCBaseHealthMult", 0.25f);
SettingT<GameSettingCollection> iLowLevelNPCMaxLevel("iLowLevelNPCMaxLevel", 3);
SettingT<GameSettingCollection> iNPCBasePerLevelHealthMult("iNPCBasePerLevelHealthMult", 4);
SettingT<GameSettingCollection> fNPCAttributeHealthMult("fNPCAttributeHealthMult", 0.5f);
SettingT<GameSettingCollection> fPlayerHealthHeartbeatSlow("fPlayerHealthHeartbeatSlow", 0.2f);
SettingT<GameSettingCollection> fPlayerHealthHeartbeatFast("fPlayerHealthHeartbeatFast", 0.1f);
SettingT<GameSettingCollection> fPainDelay("fPainDelay", 1.5f);
SettingT<GameSettingCollection> fRumblePainTime("fRumblePainTime", 0.5f);
SettingT<GameSettingCollection> fRumblePainStrength("fRumblePainStrength", 0.75f);
SettingT<GameSettingCollection> fRumbleStruckTime("fRumbleStruckTime", 0.3f);
SettingT<GameSettingCollection> fRumbleStruckStrength("fRumbleStruckStrength", 0.75f);
SettingT<GameSettingCollection> fRumbleBlockTime("fRumbleBlockTime", 0.3f);
SettingT<GameSettingCollection> fRumbleBlockStrength("fRumbleBlockStrength", 0.5f);
SettingT<GameSettingCollection> fRumbleHitBlockedTime("fRumbleHitBlockedTime", 0.3f);
SettingT<GameSettingCollection> fRumbleHitBlockedStrength("fRumbleHitBlockedStrength", 1.0f);
SettingT<GameSettingCollection> fRumbleHitTime("fRumbleHitTime", 0.3f);
SettingT<GameSettingCollection> fRumbleHitStrength("fRumbleHitStrength", 0.5f);
SettingT<GameSettingCollection> fRumbleShakeRadiusMult("fRumbleShakeRadiusMult", 1.0f);
SettingT<GameSettingCollection> fRumbleShakeTimeMult("fRumbleShakeTimeMult", 1.0f);
SettingT<GameSettingCollection> iClassAcrobat("iClassAcrobat", 145320);
SettingT<GameSettingCollection> iClassAgent("iClassAgent", 145298);
SettingT<GameSettingCollection> iClassAssassin("iClassAssassin", 145299);
SettingT<GameSettingCollection> iClassPilgrim("iClassPilgrim", 2098);
SettingT<GameSettingCollection> iClassKnight("iClassKnight", 2102);
SettingT<GameSettingCollection> iClassHealer("iClassHealer", 2320);
SettingT<GameSettingCollection> iClassMage("iClassMage", 67030);
SettingT<GameSettingCollection> iClassBard("iClassBard", 116772);
SettingT<GameSettingCollection> iPlayerCustomClass("iPlayerCustomClass", 141379);
SettingT<GameSettingCollection> iClassPriest("iClassPriest", 145285);
SettingT<GameSettingCollection> iClassWarrior("iClassWarrior", 145287);
SettingT<GameSettingCollection> iClassBarbarian("iClassBarbarian", 145288);
SettingT<GameSettingCollection> iClassCrusader("iClassCrusader", 145289);
SettingT<GameSettingCollection> iClassScout("iClassScout", 145290);
SettingT<GameSettingCollection> iClassArcher("iClassArcher", 145291);
SettingT<GameSettingCollection> iClassRogue("iClassRogue", 145292);
SettingT<GameSettingCollection> iClassThief("iClassThief", 145295);
SettingT<GameSettingCollection> iClassMonk("iClassMonk", 145321);
SettingT<GameSettingCollection> iClassSorcerer("iClassSorcerer", 145324);
SettingT<GameSettingCollection> iClassBattlemage("iClassBattlemage", 145815);
SettingT<GameSettingCollection> iClassWitchhunter("iClassWitchhunter", 145816);
SettingT<GameSettingCollection> iClassSpellsword("iClassSpellsword", 146439);
SettingT<GameSettingCollection> iClassNightblade("iClassNightblade", 146440);
SettingT<GameSettingCollection> iClassCharactergenClass("iClassCharactergenClass", 143590);
SettingT<GameSettingCollection> fEssentialDeathTime("fEssentialDeathTime", 10.0f);
SettingT<GameSettingCollection> fEssentialHealthPercentReGain("fEssentialHealthPercentReGain", 0.3f);
SettingT<GameSettingCollection> iNumberActorsGoThroughLoadDoorInCombat("iNumberActorsGoThroughLoadDoorInCombat", 2);
SettingT<GameSettingCollection> fMaxArmorRating("fMaxArmorRating", 90.0f);
SettingT<GameSettingCollection> fPlayerMaxResistance("fPlayerMaxResistance", 85.0f);
SettingT<GameSettingCollection> fPlayerDropDistance("fPlayerDropDistance", 100.0f);
SettingT<GameSettingCollection> iActivatePickLength("iActivatePickLength", 150);
SettingT<GameSettingCollection> fObjectHitWeaponReach("fObjectHitWeaponReach", 150.0f);
SettingT<GameSettingCollection> fObjectHitH2HReach("fObjectHitH2HReach", 64.0f);
SettingT<GameSettingCollection> fObjectHitOneHandReach("fObjectHitOneHandReach", 80.0f);
SettingT<GameSettingCollection> fObjectHitTwoHandReach("fObjectHitTwoHandReach", 112.0f);
SettingT<GameSettingCollection> fRemoveExcessDeadTime("fRemoveExcessDeadTime", 10.0f);
SettingT<GameSettingCollection> iRemoveExcessDeadCount("iRemoveExcessDeadCount", 15);
SettingT<GameSettingCollection> iRemoveExcessDeadTotalActorCount("iRemoveExcessDeadTotalActorCount", 20);
SettingT<GameSettingCollection> gs_fRemoveExcessComplexDeadTime("fRemoveExcessComplexDeadTime", 2.5f);
SettingT<GameSettingCollection> iRemoveExcessDeadComplexCount("iRemoveExcessDeadComplexCount", 3);
SettingT<GameSettingCollection> iRemoveExcessDeadComplexTotalActorCount("iRemoveExcessDeadComplexTotalActorCount", 20);
SettingT<GameSettingCollection> fActorAnimZAdjust("fActorAnimZAdjust", 1.8f);
SettingT<GameSettingCollection> fDeathSoundMaxDistance("fDeathSoundMaxDistance", 1000.0f);
SettingT<GameSettingCollection> iMerchantRespawnDay1("iMerchantRespawnDay1", 0);
SettingT<GameSettingCollection> iMerchantRespawnDay2("iMerchantRespawnDay2", 3);
SettingT<GameSettingCollection> fExplosionForceKnockdownMinimum("fExplosionForceKnockdownMinimum", 0.15f);
SettingT<GameSettingCollection> fExplosionForceClutterUpBias("fExplosionForceClutterUpBias", 0.5f);
SettingT<GameSettingCollection> fExplosionForceMultLinear("fExplosionForceMultLinear", 1.0f);
SettingT<GameSettingCollection> fExplosionForceMultAngular("fExplosionForceMultAngular", 1.0f);
SettingT<GameSettingCollection> fExplosionLOSBuffer("fExplosionLOSBuffer", 6.0f);
SettingT<GameSettingCollection> fExplosionImageSpaceSwapPower("fExplosionImageSpaceSwapPower", 0.6f);
SettingT<GameSettingCollection> sExplosionSplashParticles("sExplosionSplashParticles", "Effects\\ExplosionSplash.NIF");
SettingT<GameSettingCollection> fExplosionSplashRadius("fExplosionSplashRadius", 1.0f);
SettingT<GameSettingCollection> fExplosionMaxImpulse("fExplosionMaxImpulse", 8000.0f);
SettingT<GameSettingCollection> fExplosionLOSBufferDistance("fExplosionLOSBufferDistance", 24.0f);
SettingT<GameSettingCollection> fExplosionWaterRadiusRatio("fExplosionWaterRadiusRatio", 0.75f);
SettingT<GameSettingCollection> fExplosionSourceRefMult("fExplosionSourceRefMult", 1.0f);
SettingT<GameSettingCollection> iNumExplosionDecalCDPoint("iNumExplosionDecalCDPoint", 250);
SettingT<GameSettingCollection> fGameplayImpulseScale("fGameplayImpulseScale", 150.0f);
SettingT<GameSettingCollection> fGameplayImpulseMultTrap("fGameplayImpulseMultTrap", 0.15f);
SettingT<GameSettingCollection> fGameplayImpulseMultBiped("fGameplayImpulseMultBiped", 0.2f);
SettingT<GameSettingCollection> fGameplayImpulseMultProp("fGameplayImpulseMultProp", 0.1f);
SettingT<GameSettingCollection> fGameplayImpulseMultClutter("fGameplayImpulseMultClutter", 1.0f);
SettingT<GameSettingCollection> fGameplayImpulseMinMass("fGameplayImpulseMinMass", 5.0f);
SettingT<GameSettingCollection> fGameplayImpulseMultDebrisLarge("fGameplayImpulseMultDebrisLarge", 1.0f);
SettingT<GameSettingCollection> fExplosionShudderPowerMin("fExplosionShudderPowerMin", 0.5f);
SettingT<GameSettingCollection> fExplosionShudderDurationX("fExplosionShudderDurationX", 500.0f);
SettingT<GameSettingCollection> fExplosionShudderDurationY("fExplosionShudderDurationY", 750.0f);
SettingT<GameSettingCollection> fExplosionShudderIntensityX("fExplosionShudderIntensityX", 0.05f);
SettingT<GameSettingCollection> fExplosionShudderIntensityY("fExplosionShudderIntensityY", 0.1f);
SettingT<GameSettingCollection> fExplosionShudderFrequencyX("fExplosionShudderFrequencyX", 0.3f);
SettingT<GameSettingCollection> fExplosionShudderFrequencyY("fExplosionShudderFrequencyY", 1.0f);
SettingT<GameSettingCollection> fRadiationReevaluationRate("fRadiationReevaluationRate", 2.0f);
SettingT<GameSettingCollection> fRadiationAccumulationRate("fRadiationAccumulationRate", 0.01f);
SettingT<GameSettingCollection> fRadiationDecayRate("fRadiationDecayRate", 1.0f);
SettingT<GameSettingCollection> fSwimRadiationDamageMult("fSwimRadiationDamageMult", 0.5f);
SettingT<GameSettingCollection> fWadeRadiationDamageMult("fWadeRadiationDamageMult", 0.1f);
SettingT<GameSettingCollection> iXPRewardKillOpponent("iXPRewardKillOpponent", 20);
SettingT<GameSettingCollection> iXPRewardPickLock("iXPRewardPickLock", 10);
SettingT<GameSettingCollection> iXPRewardHackComputer("iXPRewardHackComputer", 10);
SettingT<GameSettingCollection> iXPRewardDiscoverMapMarker("iXPRewardDiscoverMapMarker", 10);
SettingT<GameSettingCollection> iXPRewardDiscoverSecretArea("iXPRewardDiscoverSecretArea", 10);
SettingT<GameSettingCollection> iDialogueDispositionFriendValue("iDialogueDispositionFriendValue", 50);
SettingT<GameSettingCollection> iDialogueDummySpeakThisIntOrBelow("iDialogueDummySpeakThisIntOrBelow", 4);
SettingT<GameSettingCollection> fZKeySpringDamping("fZKeySpringDamping", 0.5f);
SettingT<GameSettingCollection> fZKeySpringElasticity("fZKeySpringElasticity", 0.2f);
SettingT<GameSettingCollection> fZKeyObjectDamping("fZKeyObjectDamping", 0.75f);
SettingT<GameSettingCollection> fZKeyMaxForce("fZKeyMaxForce", 750.0f);
SettingT<GameSettingCollection> fZKeyMaxForceScaleHigh("fZKeyMaxForceScaleHigh", 1.0f);
SettingT<GameSettingCollection> fZKeyMaxForceScaleLow("fZKeyMaxForceScaleLow", 0.2f);
SettingT<GameSettingCollection> fZKeyMaxForceWeightHigh("fZKeyMaxForceWeightHigh", 750.0f);
SettingT<GameSettingCollection> fZKeyMaxForceWeightLow("fZKeyMaxForceWeightLow", 0.0f);
SettingT<GameSettingCollection> fZKeyMaxContactMassRatio("fZKeyMaxContactMassRatio", 4.0f);
SettingT<GameSettingCollection> fZKeyMaxContactDistance("fZKeyMaxContactDistance", 10.0f);
SettingT<GameSettingCollection> fZKeyComplexHelperMinDistance("fZKeyComplexHelperMinDistance", 6.0f);
SettingT<GameSettingCollection> fZKeyComplexHelperScale("fZKeyComplexHelperScale", 2.0f);
SettingT<GameSettingCollection> fZKeyComplexHelperWeightMax("fZKeyComplexHelperWeightMax", 100.0f);
SettingT<GameSettingCollection> fZKeyComplexHelperWeightMin("fZKeyComplexHelperWeightMin", 0.0f);
SettingT<GameSettingCollection> fCommentOnPlayerActionsTimer("fCommentOnPlayerActionsTimer", 10.0f);
SettingT<GameSettingCollection> fSayOncePerDayInfoTimer("fSayOncePerDayInfoTimer", 12.0f);
SettingT<GameSettingCollection> fCommentOnPlayerKnockingThings("fCommentOnPlayerKnockingThings", 20.0f);
SettingT<GameSettingCollection> sRagdollDataDefault("sRagdollDataDefault", "RagdollConstraint\\Default.rdt");
SettingT<GameSettingCollection> fCameraShakeTime("fCameraShakeTime", 1.25f);
SettingT<GameSettingCollection> fCameraShakeFadeTime("fCameraShakeFadeTime", 0.25f);
SettingT<GameSettingCollection> fCameraShakeMultMin("fCameraShakeMultMin", 0.0f);
SettingT<GameSettingCollection> fCameraShakeDistMin("fCameraShakeDistMin", 300.0f);
SettingT<GameSettingCollection> fCameraShakeDistFadeStart("fCameraShakeDistFadeStart", 2000.0f);
SettingT<GameSettingCollection> fCameraShakeDistFadeDelta("fCameraShakeDistFadeDelta", 2500.0f);
SettingT<GameSettingCollection> fCameraShakeExplosionDistMult("fCameraShakeExplosionDistMult", 0.001f);
SettingT<GameSettingCollection> fLockSkillBase("fLockSkillBase", 10.0f);
SettingT<GameSettingCollection> fVATSSmartCameraCheckStepDistance("fVATSSmartCameraCheckStepDistance", 128.0f);
SettingT<GameSettingCollection> fVATSSmartCameraCheckStepCount("fVATSSmartCameraCheckStepCount", 5.0f);
SettingT<GameSettingCollection> fVATSSmartCameraCheckHeight("fVATSSmartCameraCheckHeight", 96.0f);
SettingT<GameSettingCollection> fPlayerDetectActorValue("fPlayerDetectActorValue", -35.0f);
SettingT<GameSettingCollection> fKillCameraTime("fKillCameraTime", 3.5f);
SettingT<GameSettingCollection> fKillCameraCooldown("fKillCameraCooldown", 15.0f);
SettingT<GameSettingCollection> fTurboTimeMultiplier("fTurboTimeMultiplier", 0.35f);
SettingT<GameSettingCollection> iHackingRetryMilliseconds("iHackingRetryMilliseconds", 10000);
SettingT<GameSettingCollection> fUpperCutThreshold("fUpperCutThreshold", 50.0f);
SettingT<GameSettingCollection> fUpperCutAPCost("fUpperCutAPCost", 20.0f);
SettingT<GameSettingCollection> fUpperCutSkillChance("fUpperCutSkillChance", 0.15f);
SettingT<GameSettingCollection> fUpperCutVatsMultiplier("fUpperCutVatsMultiplier", 1.15f);
SettingT<GameSettingCollection> fCrossThreshold("fCrossThreshold", 75.0f);
SettingT<GameSettingCollection> fCrossSkillChance("fCrossSkillChance", 0.15f);
SettingT<GameSettingCollection> fCrossSkillDamageMultiplier("fCrossSkillDamageMultiplier", 2.5f);
SettingT<GameSettingCollection> fCrossAPCost("fCrossAPCost", 20.0f);
SettingT<GameSettingCollection> fCrossVatsMultiplier("fCrossVatsMultiplier", 1.1f);
SettingT<GameSettingCollection> fGroundAttackVatsMultiplier("fGroundAttackVatsMultiplier", 2.0f);
SettingT<GameSettingCollection> fCounterAttackTimer("fCounterAttackTimer", 1.0f);
SettingT<GameSettingCollection> fMoveOneCrippledLegSpeedMult("fMoveOneCrippledLegSpeedMult", 0.85f);
SettingT<GameSettingCollection> fMoveTwoCrippledLegsSpeedMult("fMoveTwoCrippledLegsSpeedMult", 0.75f);
SettingT<GameSettingCollection> fSkillUseExp("fSkillUseExp", 1.0f);
SettingT<GameSettingCollection> fSkillUseFactor("fSkillUseFactor", 1.0f);
SettingT<GameSettingCollection> fImpactShaderMaxDistance("fImpactShaderMaxDistance", 5000.0f);
SettingT<GameSettingCollection> fImpactShaderMinMagnitude("fImpactShaderMinMagnitude", 0.1f);
SettingT<GameSettingCollection> fImpactShaderMaxMagnitude("fImpactShaderMaxMagnitude", 5.0f);
SettingT<GameSettingCollection> fHealingRateSleepingBase("fHealingRateSleepingBase", 2.0f);
SettingT<GameSettingCollection> fHealingRateSleepingMult("fHealingRateSleepingMult", 2.0f);
SettingT<GameSettingCollection> iMaxCharacterLevel("iMaxCharacterLevel", 20);
SettingT<GameSettingCollection> iXPBase("iXPBase", 200);
SettingT<GameSettingCollection> iXPBumpBase("iXPBumpBase", 100);
SettingT<GameSettingCollection> iLevelUpSkillPointsBase("iLevelUpSkillPointsBase", 7);
SettingT<GameSettingCollection> iLevelUpSkillPointsInterval("iLevelUpSkillPointsInterval", 2);
SettingT<GameSettingCollection> fSpeechAutoSuccessThreshold("fSpeechAutoSuccessThreshold", 0.8f);
SettingT<GameSettingCollection> fSpeechChallengeMultiplier("fSpeechChallengeMultiplier", 1.0f);
SettingT<GameSettingCollection> fSpeechChallengeDispositionMultiplier("fSpeechChallengeDispositionMultiplier", 2.0f);
SettingT<GameSettingCollection> fSpeechChallengeSpeechBase("fSpeechChallengeSpeechBase", 50.0f);
SettingT<GameSettingCollection> fSpeechChallengeDispositionBase("fSpeechChallengeDispositionBase", 50.0f);
SettingT<GameSettingCollection> fSpeechChallengeFailureBase("fSpeechChallengeFailureBase", 10.0f);
SettingT<GameSettingCollection> fSpeechChallengeFailureMultiplier("fSpeechChallengeFailureMultiplier", 50.0f);

float GamePlayFormulas::GetActorWalkSpeed(
    ActorValueOwner* apActor,
    TESObjectWEAP* pWeap,
    TESObjectARMO* pArmor,
    bool abSneaking,
    bool abNoWeapon,
    bool abIsNpc,
    bool abOverEncumbered
) {
    float fMovePenalties = 0.0;
    if (pArmor) {
        uint8_t ucArmorWeightClass = pArmor->GetArmorWeight();
        if (ucArmorWeightClass == 1) {
            fMovePenalties = fMovePenalties + fMoveHeavyArmorPenalty.Float();
        }
        else if (ucArmorWeightClass == 2) {
            fMovePenalties = fMovePenalties + fMoveMediumArmorPenalty.Float();
        }
    }
    if (pWeap && !abNoWeapon && pWeap->IsTwoHandedWeapon()) {
        if (pWeap->GetWeaponType() == WEAPON_TWO_HAND_HANDLE || pWeap->GetWeaponType() == WEAPON_TWO_HAND_LAUNCHER) {
            fMovePenalties = fMovePenalties + fMove2HBigPenalty.Float();
        }
        else if (pWeap->GetWeaponType() != WEAPON_TWO_HAND_MELEE) {
            fMovePenalties = fMovePenalties + fMove2HRPenalty.Float();
        }
    }

    double dLeftMobility = 1.0 -
        CombatFormulas::GetBodyPartCondition(apActor, ActorValue::LEFT_MOBILITY_CONDITION, false);
    float fTotalMobility = 1.0 -
        CombatFormulas::GetBodyPartCondition(apActor, ActorValue::RIGHT_MOBILITY_CONDITION, false) + dLeftMobility;
    float fCrippledLimbMult = 1.0;
    if (fTotalMobility == 1.0) {
        fCrippledLimbMult = fMoveOneCrippledLegSpeedMult.Float();
    }
    else if (fTotalMobility == 2.0) {
        fCrippledLimbMult = fMoveTwoCrippledLegsSpeedMult.Float();
    }
    if (!abOverEncumbered && apActor->GetActorValue(ActorValue::IGNORE_CRIPPLED_LIMBS)) {
        fCrippledLimbMult = 1.0;
    }

    float fSpeedMult = apActor->GetActorFloatValue(ActorValue::SPEED_MULT) * 0.01;
    float fSpeed = fSpeedMult * fMoveBaseSpeed.Float() * fCrippledLimbMult;
    if (abIsNpc && abNoWeapon) {
        fSpeed = fSpeed * fMoveNoWeaponMult.Float();
    }
    fSpeed = (1.0 - fMovePenalties) * fSpeed;
    if (abSneaking) {
        fSpeed = fSpeed * fMoveSneakMult.Float();
    }
    if (!(fSpeed < 0.0)) {
        return fSpeed;
    }

    return 0.0;
}

int32_t GamePlayFormulas::GetLevelUpSkillPoints(int32_t aiIntelligence, int32_t aiLevel) {
    float fBasePoints = static_cast<float>(aiIntelligence * 0.5 + 10.0);
    int32_t iPoints = static_cast<int32_t>(std::floor(fBasePoints));
    if (!(aiLevel % 2) && aiIntelligence % 2 == 1) {
        ++iPoints;
    }
    return iPoints;
}

float GamePlayFormulas::GetDifficultyMultiplier(
    GamePlayFormulas::DifficultyLevel aeDifficulty,
    ActorValue::Index aiWhich,
    bool abToPC
) {
    float fresult = 1.0f;
    if (aiWhich == ActorValue::HEALTH) {
        if (abToPC) {
            return fDiffMultHPToPC[aeDifficulty]->Float();
        }

        return fDiffMultHPByPC[aeDifficulty]->Float();
    }

    if (aiWhich == ActorValue::EXPERIENCE_POINTS) {
        return fDiffMultXP[aeDifficulty]->Float();
    }

    return fresult;
}

// The experience multiplier is GetDifficultyMultiplier's aiWhich 24; the product is rounded to a float local.
float GamePlayFormulas::AdjustExperiencePoints(float afXP, GamePlayFormulas::DifficultyLevel aeDifficulty) {
    float fXP = afXP;
    float fMult = GetDifficultyMultiplier(aeDifficulty, ActorValue::EXPERIENCE_POINTS, true);
    fXP = fXP * fMult;
    return fXP;
}

int32_t GamePlayFormulas::GetRequiredExperiencePoints(int32_t aiWhichLevel) {
    if (aiWhichLevel <= 1) {
        return 0;
    }
    uint32_t uiXP = iXPBase.UInt();
    uint32_t uiBump = uiXP;
    uint32_t uiBumpBase = iXPBumpBase.UInt();
    for (int32_t i = 1; i < aiWhichLevel - 1; ++i) {
        uiBump += uiBumpBase;
        uiXP += uiBump;
    }
    return static_cast<int32_t>(uiXP);
}

float GamePlayFormulas::GetActorRunSpeed(
    ActorValueOwner* apActor,
    TESObjectWEAP* pWeap,
    TESObjectARMO* pArmor,
    bool abNoWeapon,
    bool abIsNpc,
    bool abIsSneaking,
    bool abOverEncumbered
) {
    float fWalkSpeed = GamePlayFormulas::GetActorWalkSpeed(
                           apActor,
                           pWeap,
                           pArmor,
                           abIsSneaking,
                           abNoWeapon,
                           abIsNpc,
                           abOverEncumbered
                       ) +
        0.0f;
    return fWalkSpeed * fMoveRunMult.Float();
}

float GamePlayFormulas::GetActorClothingDesirability(uint32_t aiClothingValue) {
    return static_cast<float>(aiClothingValue);
}

float GamePlayFormulas::PainChangePerSecond(float afDamagePercent) {
    return static_cast<float>(afDamagePercent * 60.0);
}

// The share is computed at double precision and stored as a float before the clamps; the product is stored as a
// float too.
float GamePlayFormulas::GetRadiationLevel(float afRadiation, float afRadius, float afDistance) {
    float fShare = static_cast<float>((static_cast<double>(afRadius) - afDistance) / afRadius);
    float fClamped = BSMIN(1.0f, BSMAX(0.0f, fShare));
    float fResult = static_cast<float>(static_cast<double>(afRadiation) * fClamped);
    return fResult;
}

// A negative health percent counts as 10 (full). A positive value is scaled by the condition curve: the base share plus
// (1 - base) times mult times health^exp, each intermediate held as a double; a result of magnitude 1 or more is
// rounded to the nearest whole value.
float GamePlayFormulas::CalcItemHealthValue(float afItemValue, float afItemHealthPercent) {
    float fValue = afItemValue;
    float fHealth = static_cast<float>(afItemHealthPercent / 10.0);
    if (fHealth < 0.0) {
        fHealth = 10.0f;
    }
    if (fValue > 0.0) {
        double dBaseless = (1.0 - fItemConditionValueBase.Float()) * fValue;
        double dScaled = fItemConditionValueMult.Float() * dBaseless;
        double dCurve = std::pow(fHealth, fItemConditionValueExp.Float()) * dScaled;
        fValue = static_cast<float>(static_cast<double>(fValue) * fItemConditionValueBase.Float() + dCurve);
    }
    float fResult;
    if (std::fabsf(fValue) >= 1.0) {
        fResult = BSRoundToNearest(fValue, 1.0f);
    }
    else {
        fResult = fValue;
    }
    return fResult;
}

// Both conditions are scaled down by 10 in place. The result is the better condition plus fRepairMin, the skill's share
// of fRepairMax - fRepairMin and fRepairScavengeMult times the worse condition. The skill-scaled repair skill base
// computed next is never used, but its three Float calls are made. Every Setting is read through
// Float and the rounded values are truncated.
float GamePlayFormulas::GetRepairedCondition(
    int32_t aiRepairSkill,
    float afCondition1,
    float afCondition2,
    int32_t* apiMinSkillNeeded
) {
    float fRepaired = 0.0f;
    afCondition1 = static_cast<float>(afCondition1 / 10.0);
    afCondition2 = static_cast<float>(afCondition2 / 10.0);
    float fSkillPart = static_cast<float>(
        (static_cast<double>(fRepairMax.Float()) - fRepairMin.Float()) * aiRepairSkill / 100.0
    );
    float fScavengePart = static_cast<float>(
        static_cast<double>(BSMIN(afCondition1, afCondition2)) * fRepairScavengeMult.Float()
    );
    float fBest = BSMAX(afCondition1, afCondition2);
    fRepaired = static_cast<float>(static_cast<double>(fBest) + fRepairMin.Float() + fSkillPart + fScavengePart);
    if (apiMinSkillNeeded) {
        float fRepaired100 = static_cast<float>(fRepaired * 100.0);
        int32_t iRepaired = static_cast<int32_t>(BSRoundToNearest(fRepaired100, 1.0f));
        float fBest100 = static_cast<float>(fBest * 100.0);
        if (iRepaired <= static_cast<int32_t>(BSRoundToNearest(fBest100, 1.0f))) {
            double dTarget = fBest + 0.10000000149011612;
            double dNeeded = (dTarget - fRepairSkillBase.Float()) * 100.0;
            *apiMinSkillNeeded = static_cast<int32_t>(
                dNeeded / (static_cast<double>(fRepairSkillMax.Float()) - fRepairSkillBase.Float())
            );
        }
    }
    float fResult = static_cast<float>(fRepaired * 10.0);
    return fResult;
}

// The vendor's effective skill is fRepairSkillBase plus its share of fRepairSkillMax - fRepairSkillBase; afCondition is
// scaled down by 10 in place. A cost of 0.5 or less (or NaN) is free; otherwise it is rounded and at least 1 (the int
// BSMAX).
int32_t GamePlayFormulas::GetRepairCost(int32_t aiVendorRepairSkill, float afCondition, float afValue) {
    int32_t iCost = 0;
    float fCost = 0.0f;
    float fSkill = static_cast<float>(
        (static_cast<double>(fRepairSkillMax.Float()) - fRepairSkillBase.Float()) * aiVendorRepairSkill / 100.0 +
        fRepairSkillBase.Float()
    );
    afCondition = static_cast<float>(afCondition / 10.0);
    fCost = static_cast<float>(
        static_cast<double>(fItemRepairCostMult.Float()) * afValue *
        ((static_cast<double>(fSkill) - afCondition) / 10.0)
    );
    if (fCost > 0.5) {
        iCost = BSMAX(static_cast<int32_t>(BSRoundToNearest(fCost, 1.0f)), 1);
    }
    return iCost;
}

float GamePlayFormulas::SwimBreathTime(int32_t aiEndurance) {
    return static_cast<float>(
        (fActorSwimBreathMult.Float() * static_cast<double>(aiEndurance)) + fActorSwimBreathBase.Float()
    );
}

// The rates are per hour: 3600.0 is a double constant; the waking rate is rounded to a float local.
float GamePlayFormulas::GetHealingRate(float afHealingRate, bool abIsSleeping) {
    float fRate = static_cast<float>(afHealingRate / 3600.0);
    if (!abIsSleeping) {
        return fRate;
    }
    double dSleepingBase = fHealingRateSleepingBase.Float() / 3600.0;
    return static_cast<float>(fRate * fHealingRateSleepingMult.Float() + dSleepingBase);
}

// The cases fall through deliberately: the retort's bonus also tries the alembic's, and the calcinator and alembic
// combination adds the calcinator's bonus a second time.
void GamePlayFormulas::CalculatePotionDurationMagnitude(
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
) {
    float fExponent = static_cast<float>(1.0 / (fMagicCostScale.Float() + 1.0));
    float fBase = ((afPotionStrength * fPotionT1MagMult.Float()) / afEffectCost);
    aiMagnitude = std::pow(fBase, fExponent);
    // aiMagnitude is read back after the store, so the pow result is rounded to a float.
    double dDuration = aiMagnitude / fPotionT1MagMult.Float();
    aiDuration = static_cast<float>(dDuration / fMagicDurMagBaseCostMult.Float());
    switch (aiEquipment) {
        case 1:
            if (!bhostile && !abPoison) {
                aiMagnitude = (afRetortQuality * fPotionT1RetMagMult.Float() * aiMagnitude + aiMagnitude);
                aiDuration = (afRetortQuality * fPotionT1RetDurMult.Float() * aiDuration + aiDuration);
            }
            [[fallthrough]];
        case 2:
            if (bhostile && !abPoison) {
                aiMagnitude = (afAlembicQuality * fPotionT1AleMagMult.Float() * aiMagnitude + aiMagnitude);
                aiDuration = (afAlembicQuality * fPotionT1AleDurMult.Float() * aiDuration + aiDuration);
            }
            break;
        case 4:
            if (bhostile || abPoison) {
                aiMagnitude = (afCalcinatorQuality * fPotionT1CalMagMult.Float() * aiMagnitude + aiMagnitude);
                aiDuration = (afCalcinatorQuality * fPotionT1CalMagMult.Float() * aiDuration + aiDuration);
            }
            else {
                double dRetMag = aiMagnitude * afRetortQuality;
                double dRetMagBonus = fPotionT1RetMagMult.Float() * dRetMag;
                double dCalMag = aiDuration * afCalcinatorQuality;
                aiMagnitude = static_cast<float>(fPotionT1CalMagMult.Float() * dCalMag + dRetMagBonus + aiMagnitude);
                double dRetDur = aiDuration * afRetortQuality;
                double dRetDurBonus = fPotionT1RetDurMult.Float() * dRetDur;
                double dCalDur = aiDuration * afCalcinatorQuality;
                aiDuration = static_cast<float>(fPotionT1CalMagMult.Float() * dCalDur + dRetDurBonus + aiDuration);
            }
            break;
        case 5:
            if (!bhostile || abPoison) {
                aiMagnitude = (afCalcinatorQuality * fPotionT1CalMagMult.Float() * aiMagnitude + aiMagnitude);
                aiDuration = (afCalcinatorQuality * fPotionT1CalMagMult.Float() * aiDuration + aiDuration);
            }
            else {
                double dAleMag = afAlembicQuality * fPotionT1AleMagMult.Float() * aiMagnitude;
                aiMagnitude = static_cast<float>(
                    afCalcinatorQuality * fPotionT1CalMagMult.Float() * aiMagnitude + dAleMag + aiMagnitude
                );
                double dAleDur = afAlembicQuality * fPotionT1AleDurMult.Float() * aiDuration;
                aiDuration = static_cast<float>(
                    afCalcinatorQuality * fPotionT1CalMagMult.Float() * aiDuration + dAleDur + aiDuration
                );
            }
            [[fallthrough]];
        case 3:
            aiMagnitude = (afCalcinatorQuality * fPotionT1CalMagMult.Float() * aiMagnitude + aiMagnitude);
            aiDuration = (afCalcinatorQuality * fPotionT1CalMagMult.Float() * aiDuration + aiDuration);
            break;
        default: return;
    }
}

void GamePlayFormulas::CalculatePotionDuration(
    float& aiDuration,
    float afEffectCost,
    float afPotionStrength,
    int32_t aiEquipment,
    bool bhostile,
    float afRetortQuality,
    float afAlembicQuality,
    float afCalcinatorQuality,
    bool abPoison
) {
    aiDuration = ((afPotionStrength / afEffectCost) / fMagicDurMagBaseCostMult.Float());
    switch (aiEquipment) {
        case 1:
            if (!bhostile && !abPoison) {
                double dRet = aiDuration * afRetortQuality;
                aiDuration = static_cast<float>(fPotionT2RetDurMult.Float() * dRet + aiDuration);
            }
            [[fallthrough]];
        case 2:
            if (bhostile && !abPoison) {
                double dAle = aiDuration * afAlembicQuality;
                aiDuration = static_cast<float>(fPotionT2AleDurMult.Float() * dAle + aiDuration);
            }
            break;
        case 3: {
            double dCal = aiDuration * afCalcinatorQuality;
            aiDuration = static_cast<float>(fPotionT2CalDurMult.Float() * dCal + aiDuration);
            break;
        }
        case 4:
            if (bhostile || abPoison) {
                double dCal = aiDuration * afCalcinatorQuality;
                aiDuration = static_cast<float>(fPotionT2CalDurMult.Float() * dCal + aiDuration);
            }
            else {
                double dRet = aiDuration * afRetortQuality;
                double dRetBonus = fPotionT2RetDurMult.Float() * dRet;
                double dCal = aiDuration * afCalcinatorQuality;
                aiDuration = static_cast<float>(fPotionT2CalDurMult.Float() * dCal + dRetBonus + aiDuration);
            }
            break;
        case 5:
            if (!bhostile || abPoison) {
                double dCal = aiDuration * afCalcinatorQuality;
                aiDuration = static_cast<float>(fPotionT2CalDurMult.Float() * dCal + aiDuration);
            }
            else {
                double dAle = aiDuration * afAlembicQuality;
                double dAleBonus = fPotionT2AleDurMult.Float() * dAle;
                double dCal = aiDuration * afCalcinatorQuality;
                aiDuration = static_cast<float>(fPotionT2CalDurMult.Float() * dCal + dAleBonus + aiDuration);
            }
            break;
        default: return;
    }
}

void GamePlayFormulas::CalculatePotionMagnitude(
    float& aiMagnitude,
    float afEffectCost,
    float afPotionStrength,
    int32_t aiEquipment,
    bool bhostile,
    float afRetortQuality,
    float afAlembicQuality,
    float afCalcinatorQuality,
    bool abPoison
) {
    float fY = 1.0 / fMagicCostScale.Float();
    float fX = afPotionStrength / afEffectCost / fMagicDurMagBaseCostMult.Float();
    aiMagnitude = std::pow(fX, fY);
    switch (aiEquipment) {
        case 1: {
            if (!bhostile && !abPoison) {
                double dRetortScaled = aiMagnitude * afRetortQuality;
                aiMagnitude = fPotionT3RetMagMult.Float() * dRetortScaled + aiMagnitude;
            }
            [[fallthrough]];
        }
        case 2: {
            if (bhostile && !abPoison) {
                double dAlembicScaled = aiMagnitude * afAlembicQuality;
                aiMagnitude = fPotionT3AleMagMult.Float() * dAlembicScaled + aiMagnitude;
            }
            break;
        }
        case 3: {
            double dCalcinatorScaled = aiMagnitude * afCalcinatorQuality;
            aiMagnitude = fPotionT3CalMagMult.Float() * dCalcinatorScaled + aiMagnitude;
            break;
        }
        case 4: {
            if (bhostile || abPoison) {
                double dCalcinatorScaled = aiMagnitude * afCalcinatorQuality;
                aiMagnitude = fPotionT3CalMagMult.Float() * dCalcinatorScaled + aiMagnitude;
            }
            else {
                double dRetortScaled = aiMagnitude * afRetortQuality;
                double dCalcinatorScaled = fPotionT3RetMagMult.Float() * dRetortScaled * afCalcinatorQuality;
                aiMagnitude = fPotionT3CalMagMult.Float() * dCalcinatorScaled + aiMagnitude;
            }
            break;
        }
        case 5: {
            if (!bhostile || abPoison) {
                double dCalcinatorScaled = aiMagnitude * afCalcinatorQuality;
                aiMagnitude = fPotionT3CalMagMult.Float() * dCalcinatorScaled + aiMagnitude;
            }
            else {
                double dAlembicScaled = aiMagnitude * afAlembicQuality;
                double dCalcinatorScaled = fPotionT3AleMagMult.Float() * dAlembicScaled * afCalcinatorQuality;
                aiMagnitude = fPotionT3CalMagMult.Float() * dCalcinatorScaled + aiMagnitude;
            }
            break;
        }
        default: {
            break;
        }
    }
}

int32_t GamePlayFormulas::GetForceLockChance(int32_t aiPlayerSkill, int32_t aiLockLevelMax) {
    int32_t iChance = static_cast<int32_t>(fLockSkillBase.Float()) + (aiPlayerSkill - aiLockLevelMax);
    return BSMAX(BSMIN(iChance, 100), 0);
}

float GamePlayFormulas::SwimBreathDamage(int32_t aiMaxHealth) {
    return static_cast<float>(fActorSwimBreathDamage.Float() * static_cast<double>(aiMaxHealth));
}

void GamePlayFormulas::CalculateToolResistance(
    float& afResistanceTime,
    float& afMagnitude,
    float& afMouseScalar,
    float afInjuryPercent,
    float afPainPercent,
    int32_t aiMedicineSkill
) {
    afResistanceTime = static_cast<float>(
        aiMedicineSkill / 100.0 * 0.699999988079071 + (1.0 - afInjuryPercent) * 0.20000000298023224 +
        (1.0 - afPainPercent) * 0.10000000149011612
    );
    afMagnitude = static_cast<float>(
        (100.0 - aiMedicineSkill) / 100.0 * 0.699999988079071 + afInjuryPercent * 0.20000000298023224 +
        afPainPercent * 0.10000000149011612
    );
    afMouseScalar = static_cast<float>(
        aiMedicineSkill / 100.0 * 0.699999988079071 + (1.0 - afInjuryPercent) * 0.20000000298023224 +
        (1.0 - afPainPercent) * 0.10000000149011612
    );
}

float GamePlayFormulas::GetPainCausedByWound(float afLimbWoundPercent) {
    return static_cast<float>(afLimbWoundPercent * 0.5);
}

void GamePlayFormulas::GetSurgeryAutoAttemptPercentages(
    float& afLimbHealPercent,
    float& afHealthDamagePercent,
    int32_t aiMedicineSkill,
    bool abMorphineUsed,
    float afWoundPercent
) {
    float fMaxLimbHeal = static_cast<float>(aiMedicineSkill * 100.0 / 100.0 * (abMorphineUsed * 0.5 + 1.0));
    BSRandom* pRandom = BSRandom::GetSingleton();
    afLimbHealPercent = pRandom->Float(0.0, fMaxLimbHeal);
    float fMaxHealthDamage = static_cast<float>(
        (aiMedicineSkill - 100.0) / 100.0 * ((afWoundPercent - 100.0) / 100.0) * 100.0 * (1.0 - abMorphineUsed * 0.5)
    );
    pRandom = BSRandom::GetSingleton();
    double dHealthDamage = pRandom->Float(0.0, fMaxHealthDamage);
    afHealthDamagePercent = static_cast<float>(dHealthDamage);
}

float GamePlayFormulas::CalcImpactShaderAmount(float afDistance) {
    if (fImpactShaderMaxDistance.Float() < static_cast<double>(afDistance)) {
        return 0.0;
    }
    float fScale = static_cast<float>(1.0 - afDistance / fImpactShaderMaxDistance.Float());
    return (fImpactShaderMaxMagnitude.Float() - fImpactShaderMinMagnitude.Float()) * fScale +
        fImpactShaderMinMagnitude.Float();
}

int32_t
GamePlayFormulas::GetSpeechChallengeChance(float afPlayerSpeech, float afTargetDisposition, int32_t aiTopicDifficulty) {
    float fSpeechMult = static_cast<float>(
        (afPlayerSpeech - fSpeechChallengeSpeechBase.Float()) * fSpeechChallengeMultiplier.Float() / 100.0 + 1.0
    );
    float fDispositionMult = static_cast<float>(
        (afTargetDisposition - fSpeechChallengeDispositionBase.Float()) *
            fSpeechChallengeDispositionMultiplier.Float() / 100.0 +
        1.0
    );
    float fDifficultyMult = static_cast<float>((100 - aiTopicDifficulty) / 100.0);
    float fChance = fSpeechMult * fDispositionMult * fDifficultyMult;
    if (!(fSpeechAutoSuccessThreshold.Float() < static_cast<double>(fChance))) {
        return static_cast<int32_t>(fChance * 100.0 + 0.5);
    }

    return 100;
}

float GamePlayFormulas::GetActorSwimWalkSpeed(
    ActorValueOwner* apActor,
    TESObjectWEAP* pWeap,
    TESObjectARMO* pArmor,
    bool abNoWeapon,
    bool abIsNpc,
    bool abOverEncumbered
) {
    return GamePlayFormulas::GetActorWalkSpeed(apActor, pWeap, pArmor, false, abNoWeapon, abIsNpc, abOverEncumbered);
}

float GamePlayFormulas::GetActorSwimRunSpeed(
    ActorValueOwner* apActor,
    TESObjectWEAP* pWeap,
    TESObjectARMO* pArmor,
    bool abNoWeapon,
    bool abIsNpc,
    bool abOverEncumbered
) {
    return GamePlayFormulas::GetActorRunSpeed(apActor, pWeap, pArmor, abNoWeapon, abIsNpc, false, abOverEncumbered);
}

double GamePlayFormulas::CalcFatigueReturn(float aiEndurance) {
    return static_cast<float>(aiEndurance * 10.0 * fFatigueReturnMult.Float() + fFatigueReturnBase.Float());
}

// The level is read first; Intelligence, capped at 10 by the int BSMIN, is the first argument.
int32_t GamePlayFormulas::GetLevelUpSkillPoints_0(ActorValueOwner* apOwner) {
    uint16_t usLevel = apOwner->GetActorLevel();
    int32_t iIntelligence = BSMIN(apOwner->GetPermanentActorValue(ActorValue::INTELLIGENCE), 10);
    return GamePlayFormulas::GetLevelUpSkillPoints(iIntelligence, usLevel);
}

float GamePlayFormulas::GetSpeechFailPenalty(int32_t aiSuccessChance) {
    double dBase = -fSpeechChallengeFailureBase.Float();
    return dBase - (1.0 - static_cast<double>(aiSuccessChance) / 100.0) * fSpeechChallengeFailureMultiplier.Float();
}

float* GamePlayFormulas::GetBarterMultipliers(int32_t aiBarterSkill, float* apBuyMult, float* apSellMult) {
    *apBuyMult = static_cast<double>(aiBarterSkill) * fBarterBuyMult.Float() / 100.0 + fBarterBuyBase.Float();
    float* pSellMultSetting = &fBarterSellMult.Float();
    *apSellMult = static_cast<double>(aiBarterSkill) * *pSellMultSetting / 100.0 + fBarterSellBase.Float();
    return pSellMultSetting;
}
