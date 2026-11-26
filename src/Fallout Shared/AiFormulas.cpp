#include "Fallout Shared/AiFormulas.h"

#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/MobileObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <cmath>

SandboxSettingTable fSandboxDurationMultProc{
    &fSandboxDurationMultFurniture,
    &fSandboxDurationMultFurniture,
    &fSandboxDurationMultEating,
    &fSandboxDurationMultWandering,
    &fSandboxDurationMultIdleMarker,
    &fSandboxDurationMultWandering
};
SandboxSettingTable fSandboxEnergyMultProc{
    &fSandboxEnergyMultFurniture,
    &fSandboxEnergyMultFurniture,
    &fSandboxEnergyMultEating,
    &fSandboxEnergyMultWandering,
    &fSandboxEnergyMultIdleMarker,
    &fSandboxEnergyMultWandering
};
SettingPointerArray14 fAVDSkillBaseValues{
    &fAVDSkillBarterBase,
    &fAVDSkillBigGunsBase,
    &fAVDSkillEnergyWeaponsBase,
    &fAVDSkillExplosivesBase,
    &fAVDSkillLockpickBase,
    &fAVDSkillMedicineBase,
    &fAVDSkillMeleeWeaponsBase,
    &fAVDSkillRepairBase,
    &fAVDSkillScienceBase,
    &fAVDSkillSmallGunsBase,
    &fAVDSkillSneakBase,
    &fAVDSkillSpeechBase,
    &fAVDSkillSurvivalBase,
    &fAVDSkillUnarmedBase
};

// The game settings this file defines, with the game's IDs and values, in the game's initialization order.
SettingT<GameSettingCollection> fSneakBaseValue("fSneakBaseValue", -25.0f);
SettingT<GameSettingCollection> fSneakMaxDistance("fSneakMaxDistance", 1500.0f);
SettingT<GameSettingCollection> fSneakBootWeightBase("fSneakBootWeightBase", 14.0f);
SettingT<GameSettingCollection> fSneakBootWeightMult("fSneakBootWeightMult", 1.0f);
SettingT<GameSettingCollection> fSneakSkillMult("fSneakSkillMult", 0.5f);
SettingT<GameSettingCollection> fSneakSoundLosMult("fSneakSoundLosMult", 0.25f);
SettingT<GameSettingCollection> fSneakRunningMult("fSneakRunningMult", 2.0f);
SettingT<GameSettingCollection> fSneakLightMult("fSneakLightMult", 1.4f);
SettingT<GameSettingCollection> fSneakSoundsMult("fSneakSoundsMult", 1.6f);
SettingT<GameSettingCollection> fSneakSizeBase("fSneakSizeBase", 20.0f);
SettingT<GameSettingCollection> fSneakActionMult("fSneakActionMult", 1.0f);
SettingT<GameSettingCollection> fDetectionSneakLightMod("fDetectionSneakLightMod", 0.0f);
SettingT<GameSettingCollection> fSneakLightMoveMult("fSneakLightMoveMult", 0.01f);
SettingT<GameSettingCollection> fSneakLightRunMult("fSneakLightRunMult", 0.2f);
SettingT<GameSettingCollection> fSneakSleepMod("fSneakSleepBonus", -0.2f);
SettingT<GameSettingCollection> fSneakAlertMod("fSneakAlertMod", 0.2f);
SettingT<GameSettingCollection> fSneakPerceptionSkillMin("fSneakPerceptionSkillMin", 10.0f);
SettingT<GameSettingCollection> fSneakPerceptionSkillMax("fSneakPerceptionSkillMax", 90.0f);
SettingT<GameSettingCollection> fSneakExteriorDistanceMult("fSneakExteriorDistanceMult", 2.0f);
SettingT<GameSettingCollection> fSneakDistanceAttenuationExponent("fSneakDistanceAttenuationExponent", 2.0f);
SettingT<GameSettingCollection> fSneakStealthBoyMult("fSneakStealthBoyMult", 0.0f);
SettingT<GameSettingCollection> fDetectionViewCone("fDetectionViewCone", 190.0f);
SettingT<GameSettingCollection> fActorAlertSoundTimer("fActorAlertSoundTimer", 5.0f);
SettingT<GameSettingCollection> iSoundLevelSilent("iSoundLevelSilent", 0);
SettingT<GameSettingCollection> iSoundLevelNormal("iSoundLevelNormal", 50);
SettingT<GameSettingCollection> iSoundLevelLoud("iSoundLevelLoud", 100);
SettingT<GameSettingCollection> fPickUpWeaponDelay("fPickUpWeaponDelay", 0.8f);
SettingT<GameSettingCollection> fCrimeSoundBase("fCrimeSoundBase", 1.0f);
SettingT<GameSettingCollection> fCrimeSoundMult("fCrimeSoundMult", 1.0f);
SettingT<GameSettingCollection> fSneakNoticedMin("fSneakNoticedMin", -20.0f);
SettingT<GameSettingCollection> fSneakSeenMin("fSneakSeenMin", 0.0f);
SettingT<GameSettingCollection> fSneakUnseenMin("fSneakUnseenMin", 0.0f);
SettingT<GameSettingCollection> fSneakLostMin("fSneakLostMin", -20.0f);
SettingT<GameSettingCollection> iSneakSkillUseDistance("iSneakSkillUseDistance", 1000);
SettingT<GameSettingCollection> iSpeakSoundLipDistance("iSpeakSoundLipDistance", 750);
SettingT<GameSettingCollection> iBoneLODDistMult("iBoneLODDistMult", 1000);
SettingT<GameSettingCollection> fActorTeleportFadeSeconds("fActorTeleportFadeSeconds", 0.5f);
SettingT<GameSettingCollection> fPlayerTeleportFadeSeconds("fPlayerTeleportFadeSeconds", 0.001f);
SettingT<GameSettingCollection> fDrinkRepeatRate("fDrinkRepeatRate", 1.0f);
SettingT<GameSettingCollection> fDispActorCharBase("fDispActorCharBase", -30.0f);
SettingT<GameSettingCollection> fDispActorCharMult("fDispActorCharMult", 10.0f);
SettingT<GameSettingCollection> fDispTargetCharBase("fDispTargetCharBase", 0.0f);
SettingT<GameSettingCollection> fDispTargetCharMult("fDispTargetCharMult", 5.0f);
SettingT<GameSettingCollection> fDispTargetKarmaMult("fDispTargetKarmaMult", 0.002f);
SettingT<GameSettingCollection> fDispActorRespBase("fDispActorRespBase", -50.0f);
SettingT<GameSettingCollection> fDispActorRespMult("fDispActorRespMult", 1.0f);
SettingT<GameSettingCollection> fDispTargetRaceMult("fDispTargetRaceMult", 1.0f);
SettingT<GameSettingCollection> fDispTargetFactionMult("fDispTargetFactionMult", 1.0f);
SettingT<GameSettingCollection> fDispTargetFactionRankBase("fDispTargetFactionRankBase", 1.0f);
SettingT<GameSettingCollection> fDispTargetFactionRankMult("fDispTargetFactionRankMult", 0.5f);
SettingT<GameSettingCollection> fDispTargetWeaponBase("fDispTargetWeaponBase", -10.0f);
SettingT<GameSettingCollection> fDispTargetDiseaseBase("fDispTargetDiseaseBase", 0.0f);
SettingT<GameSettingCollection> fDispTargetCharOffset("fDispTargetCharOffset", 5.0f);
SettingT<GameSettingCollection> fFightDispBase("fFightDispBase", 50.0f);
SettingT<GameSettingCollection> fFightDispMult("fFightDispMult", -1.0f);
SettingT<GameSettingCollection> fFightAggrBase("fFightAggrBase", -80.0f);
SettingT<GameSettingCollection> fFightAggrMult("fFightAggrMult", 1.0f);
SettingT<GameSettingCollection> fFightDistanceBase("fFightDistanceBase", 1.0f);
SettingT<GameSettingCollection> fFightDistanceMult("fFightDistanceMult", -0.005f);
SettingT<GameSettingCollection> fFightFriendDispBase("fFightFriendDispBase", -50.0f);
SettingT<GameSettingCollection> fFightFriendDispMult("fFightFriendDispMult", 1.0f);
SettingT<GameSettingCollection> fFightAbleToDetectTimer("fFightAbleToDetectTimer", 3.0f);
SettingT<GameSettingCollection> iAIFriendlyHitMinDisposition("iAIFriendlyHitMinDisposition", 15);
SettingT<GameSettingCollection> fSearchPackageTimer("fSearchPackageTimer", 10.0f);
SettingT<GameSettingCollection> fSearchPackageDistanceToTarget("fSearchPackageDistanceToTarget", 350.0f);
SettingT<GameSettingCollection> fAiAcquireStealBase("fAiAcquireStealBase", 0.0f);
SettingT<GameSettingCollection> fAiAcquireStealMult("fAiAcquireStealMult", -1.0f);
SettingT<GameSettingCollection> fAiAcquirePickBase("fAiAcquirePickBase", 0.0f);
SettingT<GameSettingCollection> fAiAcquirePickMult("fAiAcquirePickMult", -1.0f);
SettingT<GameSettingCollection> fAiAcquireKillBase("fAiAcquireKillBase", 50.0f);
SettingT<GameSettingCollection> fAiAcquireKillMult("fAiAcquireKillMult", -1.0f);
SettingT<GameSettingCollection> fFriendHitTimer("fFriendHitTimer", 10.0f);
SettingT<GameSettingCollection> fFriendMinimumLastHitTime("fFriendMinimumLastHitTime", 0.5f);
SettingT<GameSettingCollection> iFriendHitCombatAllowed("iFriendHitCombatAllowed", 3);
SettingT<GameSettingCollection> iFriendHitNonCombatAllowed("iFriendHitNonCombatAllowed", 0);
SettingT<GameSettingCollection> iAllyHitCombatAllowed("iAllyHitCombatAllowed", 1000);
SettingT<GameSettingCollection> iAllyHitNonCombatAllowed("iAllyHitNonCombatAllowed", 3);
SettingT<GameSettingCollection> fAIAcquireObjectDistance("fAIAcquireObjectDistance", 4000.0f);
SettingT<GameSettingCollection> fAIFindBedChairsDistance("fAIFindBedChairsDistance", 8000.0f);
SettingT<GameSettingCollection> fAISocialRadiusToTriggerConversation("fAISocialRadiusToTriggerConversation", 2000.0f);
SettingT<GameSettingCollection>
    fAISocialRadiusToTriggerConversationInterior("fAISocialRadiusToTriggerConversationInterior", 500.0f);
SettingT<GameSettingCollection> fAISocialchanceForConversation("fAISocialchanceForConversation", 60.0f);
SettingT<GameSettingCollection> fAISocialchanceForConversationInterior("fAISocialchanceForConversationInterior", 25.0f);
SettingT<GameSettingCollection> fAISocialTimerForConversationsMax("fAISocialTimerForConversationsMax", 30.0f);
SettingT<GameSettingCollection> fAISocialTimerForConversationsMin("fAISocialTimerForConversationsMin", 5.0f);
SettingT<GameSettingCollection> fAIFurnitureDestinationRadius("fAIFurnitureDestinationRadius", 5.0f);
SettingT<GameSettingCollection> fAIRadiusToRunDetectionExterior("fAIRadiusToRunDetectionExterior", 2000.0f);
SettingT<GameSettingCollection> fAIRadiusToRunDetectionInterior("fAIRadiusToRunDetectionInterior", 800.0f);
SettingT<GameSettingCollection> fAISpectatorCommentTimer("fAISpectatorCommentTimer", 20.0f);
SettingT<GameSettingCollection> fAISpectatorRememberThreatTimer("fAISpectatorRememberThreatTimer", 20.0f);
SettingT<GameSettingCollection> fDetectionCommentTimer("fDetectionCommentTimer", 20.0f);
SettingT<GameSettingCollection> fDistancetoPlayerforConversations("fDistancetoPlayerforConversations", 3000.0f);
SettingT<GameSettingCollection> iDistancetoAttackedTarget("iDistancetoAttackedTarget", 384);
SettingT<GameSettingCollection> fIdleChatterCommentTimer("fIdleChatterCommentTimer", 5.0f);
SettingT<GameSettingCollection> fIdleChatterCommentTimerMax("fIdleChatterCommentTimerMax", 30.0f);
SettingT<GameSettingCollection> fDistanceProjectileExplosionDetection("fDistanceProjectileExplosionDetection", 400.0f);
SettingT<GameSettingCollection> fSneakCombatMod("fSneakCombatMod", -0.4f);
SettingT<GameSettingCollection> fSneakAmbushTargetMod("fSneakAmbushTargetMod", 2.5f);
SettingT<GameSettingCollection> fSneakAmbushNonTargetMod("fSneakAmbushNonTargetMod", -0.25f);
SettingT<GameSettingCollection> iChanceFleeDialog("iChanceFleeDialog", 25);
SettingT<GameSettingCollection> fPickPocketActorSkillBase("fPickPocketActorSkillBase", 0.0f);
SettingT<GameSettingCollection> fPickPocketActorSkillMult("fPickPocketActorSkillMult", 1.0f);
SettingT<GameSettingCollection> fPickPocketTargetSkillBase("fPickPocketTargetSkillBase", 0.0f);
SettingT<GameSettingCollection> fPickPocketTargetSkillMult("fPickPocketTargetSkillMult", -1.0f);
SettingT<GameSettingCollection> fPickPocketAmountBase("fPickPocketAmountBase", 0.0f);
SettingT<GameSettingCollection> fPickPocketAmountMult("fPickPocketAmountMult", -3.0f);
SettingT<GameSettingCollection> fAIEnergyLevelBase("fAIEnergyLevelBase", 0.0f);
SettingT<GameSettingCollection> fAIEnergyLevelMult("fAIEngergyLevelMult", 1.0f);
SettingT<GameSettingCollection> fAIMaxWanderTime("fAIMaxWanderTime", 100.0f);
SettingT<GameSettingCollection> fAIIdleWaitTime("fAIIdleWaitTime", 1.0f);
SettingT<GameSettingCollection> fPickPocketMinChance("fPickPocketMinChance", 5.0f);
SettingT<GameSettingCollection> fPickPocketMaxChance("fPickPocketMaxChance", 75.0f);
SettingT<GameSettingCollection> fPickNumBase("fPickNumBase", 1.0f);
SettingT<GameSettingCollection> fPickNumMult("fPickNumMult", 0.1f);
SettingT<GameSettingCollection> fPickSpring1("fPickSpring1", 5.0f);
SettingT<GameSettingCollection> fPickSpring2("fPickSpring2", 4.0f);
SettingT<GameSettingCollection> fPickSpring3("fPickSpring3", 3.0f);
SettingT<GameSettingCollection> fPickSpring4("fPickSpring4", 2.0f);
SettingT<GameSettingCollection> fPickSpring5("fPickSpring5", 1.0f);
SettingT<GameSettingCollection> fPickLevelBase("fPickLevelBase", 0.0f);
SettingT<GameSettingCollection> fPickLevelMult("fPickLevelMult", 0.0f);
SettingT<GameSettingCollection> fLockSkillMult("fLockSkillMult", 1.0f);
SettingT<GameSettingCollection> fLockLevelBase("fLockLevelBase", 0.0f);
SettingT<GameSettingCollection> fLockLevelMult("fLockLevelMult", -1.0f);
SettingT<GameSettingCollection> fLockPickQualityBase("fLockPickQualityBase", 1.0f);
SettingT<GameSettingCollection> fLockPickQualityMult("fLockPickQualityMult", 0.02f);
SettingT<GameSettingCollection> fLockPickBreakBase("fLockPickBreakBase", 0.05f);
SettingT<GameSettingCollection> fLockPickBreakMult("fLockPickBreakMult", -0.001f);
SettingT<GameSettingCollection> fLockTrapGoOffBase("fLockTrapGoOffBase", 0.02f);
SettingT<GameSettingCollection> fLockTrapGoOffMult("fLockTrapGoOffMult", -0.002f);
SettingT<GameSettingCollection> iAlertAgressionMin("iAlertAgressionMin", 50);
SettingT<GameSettingCollection> iCrimeAlarmRecDistance("iCrimeAlarmRecDistance", 10000);
SettingT<GameSettingCollection> fCrimeAlarmRespMult("fCrimeAlarmRespMult", 1.7f);
SettingT<GameSettingCollection> iNumberGuardsCrimeResponse("iNumberGuardsCrimeResponse", 4);
SettingT<GameSettingCollection> iNumberActorsInCombatPlayer("iNumberActorsInCombatPlayer", 20);
SettingT<GameSettingCollection> iNumberActorsAllowedToFollowPlayer("iNumberActorsAllowedToFollowPlayer", 6);
SettingT<GameSettingCollection> fAIMoveDistanceToRecalcFollowPath("fAIMoveDistanceToRecalcFollowPath", 300.0f);
SettingT<GameSettingCollection> fAIPursueDistanceLineOfSight("fAIPursueDistanceLineOfSight", 2500.0f);
SettingT<GameSettingCollection> fAIEscortWaitDistanceExterior("fAIEscortWaitDistanceExterior", 2.0f);
SettingT<GameSettingCollection> fAIEscortWaitDistanceInterior("fAIEscortWaitDistanceInterior", 512.0f);
SettingT<GameSettingCollection> fAIEscortHysteresisWidth("fAIEscortHysteresisWidth", 50.0f);
SettingT<GameSettingCollection> fAItalktosameNPCtimer("fAItalktosameNPCtimer", 120.0f);
SettingT<GameSettingCollection> fAItalktoNPCtimer("fAItalktoNPCtimer", 60.0f);
SettingT<GameSettingCollection> fAIMaxHeadTrackDistanceFromPC("fAIMaxHeadTrackDistanceFromPC", 2000.0f);
SettingT<GameSettingCollection> fAIMaxHeadTrackDistance("fAIMaxHeadTrackDistance", 400.0f);
SettingT<GameSettingCollection> fAIBestHeadTrackDistance("fAIBestHeadTrackDistance", 500.0f);
SettingT<GameSettingCollection> fAIInteriorHeadTrackMult("fAIInteriorHeadTrackMult", 0.5f);
SettingT<GameSettingCollection> fAIMinGreetingDistance("fAIMinGreetingDistance", 150.0f);
SettingT<GameSettingCollection> fAIMaxSmileDistance("fAIMaxSmileDistance", 200.0f);
SettingT<GameSettingCollection> fAIGreetingTimer("fAIGreetingTimer", 20.0f);
SettingT<GameSettingCollection> fAIDefaultSpeechMult("fAIDefaultSpeechMult", 0.05f);
SettingT<GameSettingCollection> fAIHoldDefaultHeadTrackTimer("fAIHoldDefaultHeadTrackTimer", 10.0f);
SettingT<GameSettingCollection> fAIExteriorSpectatorDetection("fAIExteriorSpectatorDetection", 1000.0f);
SettingT<GameSettingCollection> fAIExteriorSpectatorDistance("fAIExteriorSpectatorDistance", 450.0f);
SettingT<GameSettingCollection> fAIInteriorSpectatorDetection("fAIInteriorSpectatorDetection", 600.0f);
SettingT<GameSettingCollection> fAIInteriorSpectatorDistance("fAIInteriorSpectatorDistance", 200.0f);
SettingT<GameSettingCollection> iAIDistanceRadiusMinLocation("iAIDistanceRadiusMinLocation", 100);
SettingT<GameSettingCollection> fAITrespassWarningTimer("fAITrespassWarningTimer", 10.0f);
SettingT<GameSettingCollection> fAIAwareofPlayerTimer("fAIAwareofPlayerTimer", 5.0f);
SettingT<GameSettingCollection> fAIConversationExploreTime("fAIConversationExploreTime", 0.25f);
SettingT<GameSettingCollection> fPowerAttackDelay("fPowerAttackDelay", 0.3f);
SettingT<GameSettingCollection> fVanityModeDelay("fVanityModeDelay", 0.25f);
SettingT<GameSettingCollection> fVanityModeWheelInMult("fVanityModeWheelInMult", 0.05f);
SettingT<GameSettingCollection> fVanityModeWheelOutMult("fVanityModeWheelOutMult", 0.1f);
SettingT<GameSettingCollection> fVanityModeWheelMin("fVanityModeWheelMin", 30.0f);
SettingT<GameSettingCollection> fVanityModeWheelMax("fVanityModeWheelMax", 600.0f);
SettingT<GameSettingCollection> fChaseCameraMax("fChaseCameraMax", 120.0f);
SettingT<GameSettingCollection> fVanityModeWheelDefault("fVanityModeWheelDefault", 60.0f);
SettingT<GameSettingCollection> fVanityModeForceDefault("fVanityModeForceDefault", 300.0f);
SettingT<GameSettingCollection> fVanityModeWheelDeadMin("fVanityModeWheelDeadMin", 75.0f);
SettingT<GameSettingCollection> fVanityModeXMult("fVanityModeXMult", 30.0f);
SettingT<GameSettingCollection> fVanityModeYMult("fVanityModeYMult", 10.0f);
SettingT<GameSettingCollection> fVanityModeXBoxXMult("fVanityModeXBoxXMult", 4.0f);
SettingT<GameSettingCollection> fVanityModeAutoDelay("fVanityModeAutoDelay", 30.0f);
SettingT<GameSettingCollection> fVanityModeAutoXSpeed("fVanityModeAutoXSpeed", 7.0f);
SettingT<GameSettingCollection> fVanityModeAutoYSpeed("fVanityModeAutoYSpeed", 2.0f);
SettingT<GameSettingCollection> fVanityModeAutoYDegrees("fVanityModeAutoYDegrees", 22.0f);
SettingT<GameSettingCollection> sFirstPersonSkeleton("sFirstPersonSkeleton", "Characters\\_1stPerson\\Skeleton.NIF");
SettingT<GameSettingCollection> fFirstPersonZoomMinMult("fFirstPersonZoomMinMult", 0.25f);
SettingT<GameSettingCollection> fFirstPersonZoomMaxMult("fFirstPersonZoomMaxMult", 5.0f);
SettingT<GameSettingCollection> fFirstPersonHandFollowMult("fFirstPersonHandFollowMult", 0.85f);
SettingT<GameSettingCollection> fFirstPersonHandChaseSeconds("fFirstPersonHandChaseSeconds", 2.0f);
SettingT<GameSettingCollection> fFirstPersonHandChaseSecondsAttack("fFirstPersonHandChaseSecondsAttack", 0.05f);
SettingT<GameSettingCollection> fFirstPersonCameraMult("fFirstPersonCameraMult", 1.0f);
SettingT<GameSettingCollection> fChase3rdPersonVanityXYMult("fChase3rdPersonVanityXYMult", 0.5f);
SettingT<GameSettingCollection> fChase3rdPersonZUnitsPerSecond("fChase3rdPersonZUnitsPerSecond", 300.0f);
SettingT<GameSettingCollection> iInventoryMenuIdleDelay("iInventoryMenuIdleDelay", 10);
SettingT<GameSettingCollection> fCharacterDefaultTurningSpeed("fCharacterDefaultTurningSpeed", 90.0f);
SettingT<GameSettingCollection> fPCTurnAnimDeltaThreshold("fPCTurnAnimDeltaThreshold", 0.15f);
SettingT<GameSettingCollection> fPCTurnAnimMinTime("fPCTurnAnimMinTime", 0.1f);
SettingT<GameSettingCollection> fActorTurnAnimMinTime("fActorTurnAnimMinTime", 0.3f);
SettingT<GameSettingCollection> iActorTurnDegree("iActorTurnDegree", 100);
SettingT<GameSettingCollection> iActorKeepTurnDegree("iActorKeepTurnDegree", 10);
SettingT<GameSettingCollection> iActorTorsoMaxRotation("iActorTorsoMaxRotation", 45);
SettingT<GameSettingCollection> fOverShoulderStartBlendDist("fOverShoulderStartBlendDist", 0.0f);
SettingT<GameSettingCollection> fOverShoulderPosX("fOverShoulderPosX", 40.0f);
SettingT<GameSettingCollection> fOverShoulderPosZ("fOverShoulderPosZ", -10.0f);
SettingT<GameSettingCollection> fOverShoulderRotMult("fOverShoulderRotMult", 0.6f);
SettingT<GameSettingCollection> fOverShoulderOffsetPointZoomingX("fOverShoulderOffsetPointZoomingX", 0.0f);
SettingT<GameSettingCollection> fOverShoulderOffsetPointZoomingY("fOverShoulderOffsetPointZoomingY", 10000.0f);
SettingT<GameSettingCollection> fOverShoulderOffsetPointZoomingZ("fOverShoulderOffsetPointZoomingZ", -10.0f);
SettingT<GameSettingCollection> fOverShoulderOffsetPointX("fOverShoulderOffsetPointX", 98.4f);
SettingT<GameSettingCollection> fOverShoulderOffsetPointY("fOverShoulderOffsetPointY", 16.0f);
SettingT<GameSettingCollection> fOverShoulderOffsetPointZ("fOverShoulderOffsetPointZ", -24.8f);
SettingT<GameSettingCollection> fOverShoulderFOV("fOverShoulderFOV", 55.0f);
SettingT<GameSettingCollection> fMountedMaxLookingDown("fMountedMaxLookingDown", 40.0f);
SettingT<GameSettingCollection> fSittingMaxLookingDown("fSittingMaxLookingDown", 40.0f);
static SettingT<GameSettingCollection> iLowResponsibility("iLowResponsiblityLevel", 36);
static SettingT<GameSettingCollection> iMediumResponsibility("iMediumResponsiblityLevel", 50);
SettingT<GameSettingCollection> iHighResponsibility("iHighResponsibility", 90);
SettingT<GameSettingCollection> fDistanceExteriorReactCombat("fDistanceExteriorReactCombat", 600.0f);
SettingT<GameSettingCollection> fDistanceInteriorReactCombat("fDistanceInteriorReactCombat", 300.0f);
SettingT<GameSettingCollection> fFleeIsSafeTimer("fFleeIsSafeTimer", 30.0f);
SettingT<GameSettingCollection> fBowHoldTimer("fBowHoldTimer", 0.8f);
SettingT<GameSettingCollection> fCombatRoundAmount("fCombatRoundAmount", 3.0f);
static SettingT<GameSettingCollection> iCombatCurrentTargetBonus("iCurrentTargetBonus", 50);
SettingT<GameSettingCollection> fRemoteCombatMissedAttack("fRemoteCombatMissedAttack", 4.0f);
SettingT<GameSettingCollection> fDyingTimer("fDyingTimer", 2.0f);
SettingT<GameSettingCollection> fReEquipArmorTime("fReEquipArmorTime", 1.0f);
SettingT<GameSettingCollection> fPlayerDeathReloadTime("fPlayerDeathReloadTime", 5.0f);
SettingT<GameSettingCollection> fPlayerWeaponReloadTimer("fPlayerWeaponReloadTimer", 0.5f);
SettingT<GameSettingCollection> fPlayerPipBoyLightTimer("fPlayerPipBoyLightTimer", 0.8f);
SettingT<GameSettingCollection> fPlayerAmmoSwapTimer("fPlayerAmmoSwapTimer", 1.0f);
SettingT<GameSettingCollection> fObjectWeightPickupDetectionMult("fObjectWeightPickupDetectionMult", 1.0f);
SettingT<GameSettingCollection> fSpellCastingDetectionMod("fSpellCastingDetectionMod", 10.0f);
SettingT<GameSettingCollection> fSpellCastingDetectionHitActorMod("fSpellCastingDetectionHitActorMod", 40.0f);
SettingT<GameSettingCollection> fDetectionActionTimer("fDetectionActionTimer", 1.2f);
SettingT<GameSettingCollection> fCheckDeadBodyTimer("fCheckDeadBodyTimer", 20.0f);
SettingT<GameSettingCollection> fAIIdleAnimationDistance("fAIIdleAnimationDistance", 2000.0f);
SettingT<GameSettingCollection> fAIUseWeaponDistance("fAIUseWeaponDistance", 5000.0f);
SettingT<GameSettingCollection> iAINumberActorsComplexScene("iAINumberActorsComplexScene", 20);
SettingT<GameSettingCollection> fShaderShadowUpdateDistance("fShaderShadowUpdateDistance", 3000.0f);
SettingT<GameSettingCollection> fSortActorDistanceListTimer("fSortActorDistanceListTimer", 3.0f);
SettingT<GameSettingCollection> iDispKaramMax("iDispKaramMax", 20);
SettingT<GameSettingCollection> iStealWarnings("iStealWarnings", 2);
SettingT<GameSettingCollection> iPickPocketWarnings("iPickPocketWarnings", 2);
SettingT<GameSettingCollection> iGuardWarnings("iGuardWarnings", 2);
SettingT<GameSettingCollection> fWarningTimer("fWarningTimer", 5.0f);
SettingT<GameSettingCollection> fTakeBackTimerSetting("fTakeBackTimerSetting", 20.0f);
SettingT<GameSettingCollection> iCoolDownTimerSetting("iCoolDownTimerSetting", 3);
SettingT<GameSettingCollection> fPerceptionMult("fPerceptionMult", 400.0f);
SettingT<GameSettingCollection> fGuardPackageAttackRadiusMult("fGuardPackageAttackRadiusMult", 0.25f);
SettingT<GameSettingCollection>
    fAmbushOverRideRadiusforPlayerDetection("fAmbushOverRideRadiusforPlayerDetection", 1000.0f);
SettingT<GameSettingCollection> fSandBoxSearchRadius("fSandBoxSearchRadius", 6000.0f);
SettingT<GameSettingCollection> fSandBoxExtraDialogueRange("fSandBoxExtraDialogueRange", 384.0f);
SettingT<GameSettingCollection> iSandBoxPreventRepeatedActionTime("iSandBoxPreventRepeatedActionTime", 30);
SettingT<GameSettingCollection> iMinSandboxRescanSeconds("iMinSandboxRescanSeconds", 3);
SettingT<GameSettingCollection> iMaxSandboxRescanSeconds("iMaxSandboxRescanSeconds", 6);
SettingT<GameSettingCollection> fSandboxSleepStartMin("fSandboxSleepStartMin", 19.0f);
SettingT<GameSettingCollection> fSandboxSleepStartMax("fSandboxSleepStartMax", 1.0f);
SettingT<GameSettingCollection> fSandboxSleepDurationMin("fSandboxSleepDurationMin", 6.0f);
SettingT<GameSettingCollection> fSandboxSleepDurationMax("fSandboxSleepDurationMax", 10.0f);
SettingT<GameSettingCollection> fSandboxBreakfastMin("fSandboxBreakfastMin", 6.0f);
SettingT<GameSettingCollection> fSandboxBreakfastMax("fSandboxBreakfastMax", 9.0f);
SettingT<GameSettingCollection> fSandboxLunchMin("fSandboxLunchMin", 11.0f);
SettingT<GameSettingCollection> fSandboxLunchMax("fSandboxLunchMax", 14.0f);
SettingT<GameSettingCollection> fSandboxDinnerMin("fSandboxDinnerMin", 17.0f);
SettingT<GameSettingCollection> fSandboxDinnerMax("fSandboxDinnerMax", 20.0f);
SettingT<GameSettingCollection> fSandboxMealDurationMin("fSandboxMealDurationMin", 0.5f);
SettingT<GameSettingCollection> fSandboxMealDurationMax("fSandboxMealDurationMax", 2.0f);
SettingT<GameSettingCollection> fSandboxDurationBase("fSandboxDurationBase", 10.0f);
SettingT<GameSettingCollection> fSandboxDurationMultFurniture("fSandboxDurationMultFurniture", 2.0f);
SettingT<GameSettingCollection> fSandboxDurationMultIdleMarker("fSandboxDurationMultIdleMarker", 1.0f);
SettingT<GameSettingCollection> fSandboxDurationMultEating("fSandboxDurationMultEating", 0.75f);
SettingT<GameSettingCollection> fSandboxDurationMultWandering("fSandboxDurationMultWandering", 0.5f);
SettingT<GameSettingCollection> fSandboxEnergyMult("fSandboxEnergyMult", 0.05f);
SettingT<GameSettingCollection> fSandboxEnergyMultFurniture("fSandboxEnergyMultFurniture", -0.1f);
SettingT<GameSettingCollection> fSandboxEnergyMultIdleMarker("fSandboxEnergyMultIdleMarker", 0.05f);
SettingT<GameSettingCollection> fSandboxEnergyMultEating("fSandboxEnergyMultEating", -0.05f);
SettingT<GameSettingCollection> fSandboxEnergyMultWandering("fSandboxEnergyMultWandering", 0.2f);
SettingT<GameSettingCollection> fSandboxDurationRangeMult("fSandboxDurationRangeMult", 0.25f);
SettingT<GameSettingCollection> fRadiationResistanceAvoidLevel("fRadiationResistanceAvoidLevel", 10.0f);
SettingT<GameSettingCollection> iSkillPointsTagSkillMult("iSkillPointsTagSkillMult", 1);
SettingT<GameSettingCollection> iLevelsPerPerk("iLevelsPerPerk", 2);
SettingT<GameSettingCollection> fKarmaModKillingEvilActor("fKarmaModKillingEvilActor", 1.0f);
SettingT<GameSettingCollection> fKarmaModKillingVeryEvilActor("fKarmaModKillingVeryEvilActor", 2.0f);
SettingT<GameSettingCollection> fKarmaModMurderingNonEvilNPC("fKarmaModMurderingNonEvilNPC", 0.0f);
SettingT<GameSettingCollection> fKarmaModMurderingNonEvilCreature("fKarmaModMurderingNonEvilCreature", 0.0f);
SettingT<GameSettingCollection> fKarmaModMurderingGoodNPC("fKarmaModMurderingGoodNPC", -50.0f);
SettingT<GameSettingCollection> fKarmaModMurderingVeryGoodNPC("fKarmaModMurderingVeryGoodNPC", -100.0f);
SettingT<GameSettingCollection> fKarmaModStealing("fKarmaModStealing", -5.0f);
SettingT<GameSettingCollection> iKarmaMin("iKarmaMin", -1000);
SettingT<GameSettingCollection> iKarmaMax("iKarmaMax", 1000);
SettingT<GameSettingCollection> fFollowWalkMaxSpeedupMultiplier("fFollowWalkMaxSpeedupMultiplier", 1.5f);
SettingT<GameSettingCollection> fFollowWalkMinSlowdownMultiplier("fFollowWalkMinSlowdownMultiplier", 0.75f);
SettingT<GameSettingCollection> fFollowRunMaxSpeedupMultiplier("fFollowRunMaxSpeedupMultiplier", 1.25f);
SettingT<GameSettingCollection> fFollowRunMinSlowdownMultiplier("fFollowRunMinSlowdownMultiplier", 0.9f);
SettingT<GameSettingCollection> fFollowSlowdownZoneWidth("fFollowSlowdownZoneWidth", 150.0f);
SettingT<GameSettingCollection> fFollowStopZoneMinMult("fFollowStopZoneMinMult", 0.5f);
SettingT<GameSettingCollection> fFollowWalkZoneMult("fFollowWalkZoneMult", 1.0f);
SettingT<GameSettingCollection> fFollowMatchSpeedZoneWidth("fFollowMatchSpeedZoneWidth", 128.0f);
SettingT<GameSettingCollection> fFollowExtraCatchUpSpeedMult("fFollowExtraCatchUpSpeedMult", 0.2f);
SettingT<GameSettingCollection> fFollowSpaceBetweenFollowers("fFollowSpaceBetweenFollowers", 100.0f);
SettingT<GameSettingCollection> fRadiationAvoidThreshold("fRadiationAvoidThreshold", 50.0f);
SandboxActivitySettingsArray SandboxActivitySettings = {
    {fSandboxSleepStartMin, fSandboxSleepStartMax, fSandboxSleepDurationMin, fSandboxSleepDurationMax},
    {fSandboxBreakfastMin, fSandboxBreakfastMax, fSandboxMealDurationMin, fSandboxMealDurationMax},
    {fSandboxLunchMin, fSandboxLunchMax, fSandboxMealDurationMin, fSandboxMealDurationMax},
    {fSandboxDinnerMin, fSandboxDinnerMax, fSandboxMealDurationMin, fSandboxMealDurationMax},
};
SettingT<GameSettingCollection> fDetectionLargeActorThreshold("fDetectionLargeActorThreshold", 135.0f);
SettingT<GameSettingCollection> iSneakLevelBonus("iSneakLevelBonus", 5);
SettingT<GameSettingCollection> iSneakStartBonus("iSneakStartBonus", 50);
SettingT<GameSettingCollection> iSneakStartBonusLevelPenatly("iSneakStartBonusLevelPenatly", 10);
SettingT<GameSettingCollection> fDetectionLargeActorSizeMult("fDetectionLargeActorSizeMult", 1.0f);
SettingT<GameSettingCollection> fAVDHealthEnduranceMult("fAVDHealthEnduranceMult", 5.0f);
SettingT<GameSettingCollection> fAVDHealthEnduranceOffset("fAVDHealthEnduranceOffset", -1.0f);
SettingT<GameSettingCollection> fAVDHealthLevelMult("fAVDHealthLevelMult", 20.0f);
SettingT<GameSettingCollection> fAVDNPCHealthEnduranceMult("fAVDNPCHealthEnduranceMult", 5.0f);
SettingT<GameSettingCollection> fAVDNPCHealthEnduranceOffset("fAVDNPCHealthEnduranceOffset", -1.0f);
SettingT<GameSettingCollection> fAVDNPCHealthLevelMult("fAVDNPCHealthLevelMult", 20.0f);
SettingT<GameSettingCollection> fAVDFatigueBase("fAVDFatigueBase", 90.0f);
SettingT<GameSettingCollection> fAVDFatigueEnduranceMult("fAVDFatigueEnduranceMult", 20.0f);
SettingT<GameSettingCollection> fAVDFatigueLevelMult("fAVDFatigueLevelMult", 10.0f);
SettingT<GameSettingCollection> fAVDNPCFatigueBase("fAVDNPCFatigueBase", 90.0f);
SettingT<GameSettingCollection> fAVDNPCFatigueEnduranceMult("fAVDNPCFatigueEnduranceMult", 20.0f);
SettingT<GameSettingCollection> fAVDNPCFatigueLevelMult("fAVDNPCFatigueLevelMult", 10.0f);
SettingT<GameSettingCollection> fAVDActionPointsMult("fAVDActionPointsMult", 2.0f);
SettingT<GameSettingCollection> fAVDActionPointsBase("fAVDActionPointsBase", 65.0f);
SettingT<GameSettingCollection> fAVDCarryWeightMult("fAVDCarryWeightMult", 25.0f);
SettingT<GameSettingCollection> fAVDCarryWeightBase("fAVDCarryWeightsBase", 150.0f);
SettingT<GameSettingCollection> fAVDDefaultBodyPartCondition("fAVDDefaultBodyPartCondition", 100.0f);
SettingT<GameSettingCollection> fAVDHealRateEndurance6Bonus("fAVDHealRateEndurance6Bonus", 1.0f);
SettingT<GameSettingCollection> fAVDHealRateEndurance7Bonus("fAVDHealRateEndurance7Bonus", 1.0f);
SettingT<GameSettingCollection> fAVDHealRateEndurance8Bonus("fAVDHealRateEndurance8Bonus", 1.0f);
SettingT<GameSettingCollection> fAVDHealRateEndurance9Bonus("fAVDHealRateEndurance9Bonus", 2.0f);
SettingT<GameSettingCollection> fAVDHealRateEndurance10Bonus("fAVDHealRateEndurance10Bonus", 2.0f);
SettingT<GameSettingCollection> fAVDCritLuckBase("fAVDCritLuckBase", 0.0f);
SettingT<GameSettingCollection> fAVDCritLuckMult("fAVDCritLuckMult", 1.0f);
SettingT<GameSettingCollection> fAVDMeleeDamageStrengthOffset("fAVDMeleeDamageStrengthOffset", -6.0f);
SettingT<GameSettingCollection> fAVDMeleeDamageStrengthMult("fAVDMeleeDamageStrengthMult", 1.0f);
SettingT<GameSettingCollection> fAVDSkillLuckBonusMult("fAVDSkillLuckBonusMult", 0.5f);
SettingT<GameSettingCollection> fAVDSkillBarterBase("fAVDSkillBarterBase", 3.0f);
SettingT<GameSettingCollection> fAVDSkillBigGunsBase("fAVDSkillBigGunsBase", 0.0f);
SettingT<GameSettingCollection> fAVDSkillEnergyWeaponsBase("fAVDSkillEnergyWeaponsBase", 0.0f);
SettingT<GameSettingCollection> fAVDSkillExplosivesBase("fAVDSkillExplosivesBase", 3.0f);
SettingT<GameSettingCollection> fAVDSkillLockpickBase("fAVDSkillLockpickBase", 3.0f);
SettingT<GameSettingCollection> fAVDSkillMedicineBase("fAVDSkillMedicineBase", 4.0f);
SettingT<GameSettingCollection> fAVDSkillMeleeWeaponsBase("fAVDSkillMeleeWeaponsBase", 12.0f);
SettingT<GameSettingCollection> fAVDSkillRepairBase("fAVDSkillRepairBase", 3.0f);
SettingT<GameSettingCollection> fAVDSkillScienceBase("fAVDSkillScienceBase", 4.0f);
SettingT<GameSettingCollection> fAVDSkillSmallGunsBase("fAVDSkillSmallGunsBase", 7.0f);
SettingT<GameSettingCollection> fAVDSkillSneakBase("fAVDSkillSneakBase", 4.0f);
SettingT<GameSettingCollection> fAVDSkillSpeechBase("fAVDSkillSpeechBase", 4.0f);
SettingT<GameSettingCollection> fAVDSkillSurvivalBase("fAVDSkillSurvivalBase", 0.0f);
SettingT<GameSettingCollection> fAVDSkillUnarmedBase("fAVDSkillUnarmedBase", 15.0f);
SettingT<GameSettingCollection> fAVDSkillPrimaryBonusMult("fAVDSkillPrimaryBonusMult", 2.0f);
SettingT<GameSettingCollection> fAVDTagSkillBonus("fAVDTagSkillBonus", 10.0f);
SettingT<GameSettingCollection> fAVDPoisonResistEnduranceMult("fAVDPoisonResistEnduranceMult", 5.0f);
SettingT<GameSettingCollection> fAVDPoisonResistEnduranceOffset("fAVDPoisonResistEnduranceOffset", -1.0f);
SettingT<GameSettingCollection> fAVDRadResistEnduranceMult("fAVDRadResistEnduranceMult", 2.0f);
SettingT<GameSettingCollection> fAVDRadResistEnduranceOffset("fAVDRadResistEnduranceOffset", -1.0f);
SettingT<GameSettingCollection> fAVDUnarmedDamageMult("fAVDUnarmedDamageMult", 0.15f);
SettingT<GameSettingCollection> fAVDUnarmedDamageBase("fAVDUnarmedDamageBase", 2.0f);

int32_t CompareFollowerDist(const void* apA, const void* apB) {
    const FollowerDist& kA = *static_cast<const FollowerDist*>(apA);
    const FollowerDist& kB = *static_cast<const FollowerDist*>(apB);
    if (kA > kB) {
        return 1;
    }
    if (kB > kA) {
        return -1;
    }
    return 0;
}

bool FollowerDist::operator>(const FollowerDist& aFollowerDist) const {
    if (aFollowerDist.fPackDesiredDist == fPackDesiredDist) {
        return aFollowerDist.fActualDistSqr < static_cast<double>(fActualDistSqr);
    }

    return aFollowerDist.fPackDesiredDist < static_cast<double>(fPackDesiredDist);
}

PerActivitySettings::PerActivitySettings(
    SettingT<GameSettingCollection>& apStartMin,
    SettingT<GameSettingCollection>& apStartMax,
    SettingT<GameSettingCollection>& apDurMin,
    SettingT<GameSettingCollection>& apDurMax
)
  : StartMin(&apStartMin), StartMax(&apStartMax), DurMin(&apDurMin), DurMax(&apDurMax) {}

float AiFormulas::AVDeriveBodyPartCondition(
    [[maybe_unused]] ActorValueOwner* apActor,
    [[maybe_unused]] ActorValue::Index aiWhich
) {
    return fAVDDefaultBodyPartCondition.Float();
}

void AiFormulas::AVGetDerivedSkillPrimaryAttributeAndBase(
    ActorValue::Index aiSkill,
    ActorValue::Index& aiPrimaryAttribute,
    float& afBase
) {
    aiPrimaryAttribute = SkillToSpecial[aiSkill - ActorValue::BARTER];
    afBase = fAVDSkillBaseValues[aiSkill - ActorValue::BARTER]->Float();
}

float AiFormulas::AVDeriveCriticalChance(ActorValueOwner* apActor, [[maybe_unused]] ActorValue::Index aiWhich) {
    double dLuck = apActor->GetClampedActorFloatValue(ActorValue::LUCK);
    return static_cast<float>(fAVDCritLuckMult.Float() * dLuck + fAVDCritLuckBase.Float());
}

float AiFormulas::AVDeriveActionPoints(ActorValueOwner* apActor, [[maybe_unused]] ActorValue::Index aiWhich) {
    double dAgility = apActor->GetClampedActorFloatValue(ActorValue::AGILITY);
    double dScaled = fAVDActionPointsMult.Float() * dAgility;
    return static_cast<float>(fAVDActionPointsBase.Float() + dScaled);
}

float AiFormulas::AVDeriveHealth(ActorValueOwner* apActor, [[maybe_unused]] ActorValue::Index aiWhich) {
    float fEndurance = apActor->GetPermanentActorFloatValue(ActorValue::ENDURANCE);
    float fClamped = ActorValue::ClampActorValue(ActorValue::ENDURANCE, fEndurance);
    return CalculateDerivedHealth(apActor, fClamped);
}

float AiFormulas::AVDeriveFatigue(ActorValueOwner* apActor, [[maybe_unused]] ActorValue::Index aiWhich) {
    float fEndurance = apActor->GetPermanentActorFloatValue(ActorValue::ENDURANCE);
    float fClamped = ActorValue::ClampActorValue(ActorValue::ENDURANCE, fEndurance);
    return CalculateDerivedFatigue(apActor, fClamped);
}

float AiFormulas::AVDeriveSkill_0(ActorValueOwner* apOwner, ActorValue::Index aiSkill) {
    return AVDeriveSkill(apOwner, aiSkill, false);
}

float AiFormulas::CalculateDerivedSkillValue(float afSkillBase, float afPrimaryAttribute, float afLuck) {
    float fPrimaryBonus = afPrimaryAttribute * fAVDSkillPrimaryBonusMult.Float();
    float fValue = std::floorf(fPrimaryBonus) + afSkillBase;
    float fLuckBonus = afLuck * fAVDSkillLuckBonusMult.Float();
    return std::ceilf(fLuckBonus) + fValue;
}

float AiFormulas::AVDeriveResistance(ActorValueOwner* apActor, ActorValue::Index aiWhich) {
    float fResistance = 0.0f;
    if (aiWhich == ActorValue::POISON_RESISTANCE) {
        double dEndurance = apActor->GetClampedActorFloatValue(ActorValue::ENDURANCE);
        double dOffset = fAVDPoisonResistEnduranceOffset.Float() + dEndurance;
        fResistance = static_cast<float>(fAVDPoisonResistEnduranceMult.Float() * dOffset);
    }
    else if (aiWhich == ActorValue::RADIATION_RESISTANCE) {
        double dEndurance = apActor->GetClampedActorFloatValue(ActorValue::ENDURANCE);
        double dOffset = fAVDRadResistEnduranceOffset.Float() + dEndurance;
        fResistance = static_cast<float>(fAVDRadResistEnduranceMult.Float() * dOffset);
    }
    return fResistance;
}

float AiFormulas::CalculateDerivedHealth(ActorValueOwner* apActor, float afEndurance) {
    Actor* pActor = static_cast<Actor*>(apActor);
    float fResult = 0.0f;
    bool bNPC = pActor->IsNpc();
    bool bPC = pActor->IsPc();
    if (bPC) {
        double dOffset = afEndurance + fAVDHealthEnduranceOffset.Float();
        fResult = fAVDHealthEnduranceMult.Float() * dOffset;
    }
    else if (bNPC) {
        double dOffset = afEndurance + fAVDNPCHealthEnduranceOffset.Float();
        fResult = fAVDNPCHealthEnduranceMult.Float() * dOffset;
    }
    if (pActor->IsAutoCalc() && bNPC && !bPC) {
        // The result is converted to float as unsigned.
        fResult = static_cast<float>(static_cast<uint32_t>(pActor->GetActorBase()->CalculateHealth(0)));
    }
    else if (bPC && pActor->GetActorLevel()) {
        double dLevelsGained = static_cast<double>(pActor->GetActorLevel() - 1);
        fResult = fAVDHealthLevelMult.Float() * dLevelsGained + fResult;
    }
    return fResult;
}

// A random hour from afMin to afMax, wrapping past midnight when afMax is earlier than afMin.
float AiFormulas::RandomHour(float afMin, float afMax) {
    float fMax = afMax < static_cast<double>(afMin) ? afMax + 24.0 : afMax;
    float fHour = RandomFloat(afMin, fMax);
    float fWrapped = fHour - 24.0;
    if (fWrapped >= 0.0) {
        return fWrapped;
    }
    return fHour;
}

float AiFormulas::CalculateDerivedFatigue(ActorValueOwner* apActor, float afEndurance) {
    Actor* pActor = static_cast<Actor*>(apActor);
    float fResult = 0.0f;
    bool bNPC = pActor->IsNpc();
    bool bPC = pActor->IsPc();
    if (bPC) {
        fResult = afEndurance * fAVDFatigueEnduranceMult.Float() + fAVDFatigueBase.Float();
    }
    else if (bNPC) {
        fResult = afEndurance * fAVDNPCFatigueEnduranceMult.Float() + fAVDNPCFatigueBase.Float();
    }
    if (pActor->IsAutoCalc() && bNPC && !bPC) {
        // The result is converted to float as unsigned.
        fResult = static_cast<float>(static_cast<TESNPC*>(pActor->GetActorBase())->GetFormFatigueLeveled(true));
    }
    else if (bPC && pActor->GetActorLevel()) {
        double dLevelsGained = static_cast<double>(pActor->GetActorLevel() - 1);
        fResult = fAVDFatigueLevelMult.Float() * dLevelsGained + fResult;
    }
    return fResult;
}

// The skill's derived value: its base plus the primary attribute's and luck's share (the clamped values, or when
// abBaseDerivation is set the base values plus the permanent modifiers, clamped), plus the tag skill bonus when the
// owner is an actor whose class tags the skill.
float AiFormulas::AVDeriveSkill(ActorValueOwner* apActor, ActorValue::Index aiWhich, bool abBaseDerivation) {
    float fValue = 0.0f;
    if (!ActorValue::IsSkill(aiWhich)) {
        return fValue;
    }
    ActorValue::Index ePrimary = ActorValue::ATTRIBUTE_NONE;
    AVGetDerivedSkillPrimaryAttributeAndBase(aiWhich, ePrimary, fValue);
    if (ePrimary != ActorValue::ATTRIBUTE_NONE) {
        if (!abBaseDerivation) {
            float fLuck = apActor->GetClampedActorFloatValue(ActorValue::LUCK);
            float fPrimary = apActor->GetClampedActorFloatValue(ePrimary);
            fValue = CalculateDerivedSkillValue(fValue, fPrimary, fLuck);
        }
        else {
            float fLuckBase = apActor->GetBaseActorFloatValue(ActorValue::LUCK);
            float fLuckSum = apActor->GetPermanentModifier(ActorValue::LUCK) + fLuckBase;
            float fLuck = ActorValue::ClampActorValue(ActorValue::LUCK, fLuckSum);
            float fPrimaryBase = apActor->GetBaseActorFloatValue(ePrimary);
            float fPrimarySum = apActor->GetPermanentModifier(ePrimary) + fPrimaryBase;
            float fPrimary = ActorValue::ClampActorValue(ePrimary, fPrimarySum);
            fValue = CalculateDerivedSkillValue(fValue, fPrimary, fLuck);
        }
    }
    TESForm* pForm = apActor->GetAsForm();
    if (pForm && pForm->IsReference() && pForm->IsActor()) {
        Actor* pActor = static_cast<Actor*>(pForm);
        if (pActor->GetClass() && pActor->GetClass()->IsTagSkill(aiWhich)) {
            fValue = fValue + fAVDTagSkillBonus.Float();
        }
    }

    return fValue;
}

int32_t AiFormulas::GetActorDetectionSize(float afHeightOrLength) {
    if (fDetectionLargeActorThreshold.Float() < static_cast<double>(afHeightOrLength)) {
        return static_cast<int32_t>(afHeightOrLength / fDetectionLargeActorThreshold.Float());
    }
    return 1;
}

int32_t AiFormulas::GetActorDetectedTarget(
    int32_t aiPercepA,
    int32_t aiSneakB,
    int32_t aiLineofSite,
    int32_t aiVisual,
    float afDistance,
    int32_t aiLightLevelB,
    int32_t aiBlindess,
    bool abInvisibilityB,
    int32_t aiBootweightB,
    bool abMovingB,
    bool abSneakingB,
    bool abRunning,
    bool absleeping,
    bool abAlert,
    int32_t aiActionSound,
    int32_t aiTargetSize,
    bool bExterior,
    bool abInCombat,
    int32_t aiAmbushValue,
    float& afSoundSubtotal,
    float& afVisualSubtotal,
    float& afSkillSubtotal,
    const int32_t iLevelA,
    const int32_t iLevelB,
    const int32_t iArmorB
) {
    float fCalValue = fSneakBaseValue.Float();
    int32_t iSneakStart = iSneakStartBonus.Int();
    int32_t iLevelModifiedSneak = aiSneakB + iSneakLevelBonus.Int() * (iLevelB - iLevelA);
    iLevelModifiedSneak = iLevelModifiedSneak + BSMAX(iSneakStart - iSneakStartBonusLevelPenatly.Int() * iLevelB, 0) -
        iArmorB;
    float fAmbushMod = 0.0f;
    if (aiAmbushValue == 1) {
        fAmbushMod = fSneakAmbushTargetMod.Float();
    }
    else if (static_cast<double>(aiAmbushValue) == 2.0) {
        fAmbushMod = fSneakAmbushNonTargetMod.Float();
    }
    float fPerception = (fSneakPerceptionSkillMax.Float() - fSneakPerceptionSkillMin.Float()) *
            (static_cast<double>(aiPercepA) / 10.0) +
        fSneakPerceptionSkillMin.Float();
    double dAlertMult = fSneakAlertMod.Float() * static_cast<double>(abAlert) + 1.0;
    double dSleepMult = fSneakSleepMod.Float() * static_cast<double>(absleeping) + dAlertMult;
    float fPerceptionScore = (fSneakCombatMod.Float() * static_cast<double>(abInCombat) + dSleepMult + fAmbushMod) *
        fPerception;
    if (abInvisibilityB) {
        aiVisual = 0;
    }
    int32_t iMoving = abMovingB;
    float fMaxDistance = bExterior ? fSneakExteriorDistanceMult.Float() * fSneakMaxDistance.Float()
                                   : fSneakMaxDistance.Float();
    float fDistanceRatio = (fMaxDistance - afDistance) / fMaxDistance;
    float fDistanceAttenuation = BSMAX(fDistanceRatio, 0.0f);
    float fAttenuation = std::pow(
        static_cast<double>(fDistanceAttenuation),
        static_cast<double>(fSneakDistanceAttenuationExponent.Float())
    );
    float fSoundAttenuation = fAttenuation;
    if (aiLineofSite <= 0 && aiAmbushValue != 1) {
        fSoundAttenuation = fAttenuation * fSneakSoundLosMult.Float();
    }
    float fBootSound = (static_cast<double>(aiBootweightB) * fSneakBootWeightMult.Float() +
                        fSneakBootWeightBase.Float()) *
        static_cast<double>(iMoving);
    float fRunMult = 1.0f;
    if (abRunning) {
        fRunMult = fSneakRunningMult.Float();
    }
    float fMoveSound = fBootSound * fRunMult;
    int32_t iActionSound = aiActionSound * static_cast<int32_t>(fSneakActionMult.Float());
    float fSoundScore = fSoundAttenuation * fSneakSoundsMult.Float() * (static_cast<double>(iActionSound) + fMoveSound);
    float fSounds = BSMAX(fSoundScore, 0.0f);
    afSoundSubtotal = fSounds;
    double dMoveLightMult = fSneakLightMoveMult.Float() * static_cast<double>(iMoving) + 1.0;
    float fMoveRunLightMult = fSneakLightRunMult.Float() * static_cast<double>(abRunning) + dMoveLightMult;
    double dLightLevel = static_cast<double>(aiLightLevelB) + fDetectionSneakLightMod.Float();
    float fLight = fSneakLightMult.Float() * dLightLevel;
    float fSize = aiTargetSize > 1
        ? fDetectionLargeActorSizeMult.Float() * (static_cast<double>(aiTargetSize) - 1.0) + 1.0
        : 1.0;
    float fBlind = static_cast<float>((100 - aiBlindess) / 100);
    float fVisualScore = static_cast<double>(aiVisual) * fAttenuation * fMoveRunLightMult * fLight * fBlind * fSize;
    float fVisual = BSMAX(fVisualScore, 0.0f);
    if (abInvisibilityB) {
        fVisual = fVisual * fSneakStealthBoyMult.Float();
    }
    afVisualSubtotal = fVisual;
    float fSneakSkill = static_cast<float>(abSneakingB * iLevelModifiedSneak);
    float fSkill = fPerceptionScore * fAttenuation;
    float fSkillScore = (fSkill - fSneakSkill) * fSneakSkillMult.Float();
    afSkillSubtotal = fSkillScore;
    fCalValue = fCalValue + fSounds + fVisual + fSkillScore;
    if (fCalValue < 1.0 && fCalValue > 0.0) {
        return 1;
    }
    return static_cast<int32_t>(fCalValue);
}

int32_t AiFormulas::GetActorDispositiontoTarget(
    int32_t aiActorBaseDisp,
    int32_t aiCharismaTarget,
    int32_t aiResponActor,
    int32_t aiKarmaTarget,
    int32_t aiRaceReactionTarget,
    int32_t aiFactionReactionTarget,
    int32_t aiFactionRankTarget,
    int32_t aiWeaponStatusTarget,
    int32_t aiDiseaseStatusTarget,
    int32_t aiTargetMod,
    bool abcreature
) {
    int32_t iCharVal = static_cast<int32_t>(
        (static_cast<double>(aiCharismaTarget) - fDispTargetCharOffset.Float()) * fDispTargetCharMult.Float() +
        fDispTargetCharBase.Float()
    );
    int32_t iResponsibility = 0;
    if (!abcreature) {
        double fKarma = static_cast<double>(aiKarmaTarget) * fDispTargetKarmaMult.Float();
        iResponsibility = static_cast<int32_t>(
            (static_cast<double>(aiResponActor) * fDispActorRespMult.Float() + fDispActorRespBase.Float()) * fKarma
        );
    }
    int32_t iRace = aiRaceReactionTarget * static_cast<int32_t>(fDispTargetRaceMult.Float());
    double dFactionModifier = fDispTargetFactionMult.Float() * static_cast<double>(aiFactionReactionTarget);
    int32_t iFaction = static_cast<int32_t>(
        (fDispTargetFactionMult.Float() * static_cast<double>(aiFactionRankTarget) +
         fDispTargetFactionRankBase.Float()) *
        dFactionModifier
    );
    int32_t iWeapon = 0;
    if (!abcreature) {
        iWeapon = aiWeaponStatusTarget * static_cast<int32_t>(fDispTargetWeaponBase.Float());
    }
    return aiTargetMod +
        (aiDiseaseStatusTarget * static_cast<int32_t>(fDispTargetDiseaseBase.Float()) +
         (iWeapon + (iFaction + (iRace + (iResponsibility + (iCharVal + aiActorBaseDisp))))));
}

int32_t AiFormulas::GetDetectedEvent(
    [[maybe_unused]] int32_t aiPercep,
    int32_t aiLineofSite,
    float afDistance,
    int32_t aiBlindess,
    int32_t aiLightLevel,
    [[maybe_unused]] bool absleeping,
    [[maybe_unused]] bool abAlert,
    [[maybe_unused]] bool abInCombat,
    int32_t aiActionSound,
    int32_t aiVisual,
    bool abExterior
) {
    float fCalValue = fSneakBaseValue.Float();
    float fMaxDistance = abExterior ? fSneakExteriorDistanceMult.Float() * fSneakMaxDistance.Float()
                                    : fSneakMaxDistance.Float();
    float fDistanceRatio = (fMaxDistance - afDistance) / fMaxDistance;
    float fDistanceAttenuation = BSMAX(fDistanceRatio, 0.0f);
    float fAttenuation = std::pow(
        static_cast<double>(fDistanceAttenuation),
        static_cast<double>(fSneakDistanceAttenuationExponent.Float())
    );
    float fSoundAttenuation = fAttenuation;
    if (aiLineofSite <= 0) {
        fSoundAttenuation = fAttenuation * fSneakSoundLosMult.Float();
    }
    int32_t iActionSound = aiActionSound * static_cast<int32_t>(fSneakActionMult.Float());
    float fSoundScore = fSoundAttenuation * fSneakSoundsMult.Float() * static_cast<double>(iActionSound);
    float fSounds = BSMAX(fSoundScore, 0.0f);
    double dLightLevel = static_cast<double>(aiLightLevel) + fDetectionSneakLightMod.Float();
    float fLight = fSneakLightMult.Float() * dLightLevel;
    float fBlind = static_cast<float>((100 - aiBlindess) / 100);
    float fVisualScore = static_cast<double>(aiVisual) * fAttenuation * fLight * fBlind;
    float fVisual = BSMAX(fVisualScore, 0.0f);
    fCalValue = fCalValue + fSounds + fVisual;
    if (fCalValue < 1.0 && fCalValue > 0.0) {
        return 1;
    }
    return static_cast<int32_t>(fCalValue);
}

float AiFormulas::GetFollowRadiusStop(float afbasePackRadius) {
    float fSlowdown = afbasePackRadius - fFollowSlowdownZoneWidth.Float();
    float fStop = afbasePackRadius * fFollowStopZoneMinMult.Float();
    return BSMAX(fStop, fSlowdown);
}

float AiFormulas::GetFollowRadiusSlow(float afbasePackRadius) {
    return afbasePackRadius;
}

float AiFormulas::AVDeriveCarryWeight(ActorValueOwner* apActor, [[maybe_unused]] ActorValue::Index aiWhich) {
    double dStrength = apActor->GetClampedActorFloatValue(ActorValue::STRENGTH);
    double dScaled = fAVDCarryWeightMult.Float() * dStrength;
    return static_cast<float>(fAVDCarryWeightBase.Float() + dScaled);
}

float AiFormulas::AVDeriveFromPrimary(ActorValueOwner* apActor, ActorValue::Index aiWhich) {
    float fValue = 0.0f;
    if (aiWhich == ActorValue::MELEE_DAMAGE) {
        double dStrength = apActor->GetClampedActorFloatValue(ActorValue::STRENGTH);
        double dOffset = fAVDMeleeDamageStrengthOffset.Float() + dStrength;
        float fDamage = std::floorf(static_cast<float>(fAVDMeleeDamageStrengthMult.Float() * dOffset));
        if (fDamage > 0.0) {
            return fValue + fDamage;
        }
    }
    return fValue;
}

float AiFormulas::GetFollowRadiusWalk(float afbasePackRadius) {
    return afbasePackRadius * fFollowWalkZoneMult.Float();
}

float AiFormulas::GetFollowRadiusStartMoving(float afbasePackRadius) {
    return static_cast<float>(fFollowMatchSpeedZoneWidth.Float() * 0.5 + afbasePackRadius);
}

float AiFormulas::GetFollowRadiusMatchSpeed(float afbasePackRadius) {
    return afbasePackRadius + fFollowMatchSpeedZoneWidth.Float();
}

int32_t
AiFormulas::ComputePickpocketSuccess(int32_t aiSneakSkillModed, int32_t aiTargetSneakModed, float fAmountofItem) {
    float fActorSkill = static_cast<float>(
        static_cast<double>(aiSneakSkillModed) * fPickPocketActorSkillMult.Float() + fPickPocketActorSkillBase.Float()
    );
    float fTargetSkill = static_cast<float>(
        static_cast<double>(aiTargetSneakModed) * fPickPocketTargetSkillMult.Float() +
        fPickPocketTargetSkillBase.Float()
    );
    float fAmount = fPickPocketAmountMult.Float() * fAmountofItem + fPickPocketAmountBase.Float();
    int32_t iChance = static_cast<int32_t>(fActorSkill + fTargetSkill + fAmount);
    if (fPickPocketMinChance.Float() > static_cast<double>(iChance)) {
        return static_cast<int32_t>(fPickPocketMinChance.Float());
    }
    if (fPickPocketMaxChance.Float() < static_cast<double>(iChance)) {
        return static_cast<int32_t>(fPickPocketMaxChance.Float());
    }
    return iChance;
}

void AiFormulas::ComputeRandomSandboxActivityStartAndDuration(
    int32_t aiActivity,
    float& afStartOut,
    float& afDurationOut
) {
    PerActivitySettings& kSettings = SandboxActivitySettings[aiActivity];
    float fStartMax = kSettings.StartMax->Float();
    afStartOut = RandomHour(kSettings.StartMin->Float(), fStartMax);
    float fDurationMax = kSettings.DurMax->Float();
    afDurationOut = RandomFloat(kSettings.DurMin->Float(), fDurationMax);
}

float AiFormulas::GetFollowRadiusFudgeForMultipleFollowers(Actor* apActor, Actor* apFollowTarget) {
    using followerDists_t = BSSimpleArray<FollowerDist, 1024>;
    float fFudge = 0.0f;
    ExtraFollower* pFollowerExtra = apFollowTarget ? apFollowTarget->m_Extra.GetFollowerExtra() : nullptr;
    if (pFollowerExtra) {
        uint32_t uiNumActualFollowers = 0;
        if (!pFollowerExtra->pActorlist->IsEmpty()) {
            for (Actor* pFollower : *pFollowerExtra->pActorlist) {
                TESPackage* pPack = pFollower->GetCurrentPackage();
                if (pPack && pPack->GetPackType() == PACKAGE_TYPE_FOLLOW) {
                    ++uiNumActualFollowers;
                }
            }
        }
        if (uiNumActualFollowers) {
            followerDists_t followerDists(uiNumActualFollowers);
            uint32_t i = 0;
            for (Actor* pFollower : *pFollowerExtra->pActorlist) {
                TESPackage* pPack = pFollower->GetCurrentPackage();
                if (pPack && pPack->GetPackType() == PACKAGE_TYPE_FOLLOW) {
                    FollowerDist& followerInfo = followerDists.GetAt(static_cast<int32_t>(i));
                    followerInfo.pActor = pFollower;
                    const NiPoint3& kFollowerPos = pFollower->GetLocationOnReference();
                    const NiPoint3& kTargetPos = apFollowTarget->GetLocationOnReference();
                    followerInfo.fActualDistSqr = (kTargetPos - kFollowerPos).SqrLength();
                    followerInfo.fPackDesiredDist = static_cast<double>(iAIDistanceRadiusMinLocation.Int());
                    followerInfo.fActorRadius = pFollower->GetRadius();
                    followerInfo.fPackDesiredDist = pPack->GetPackageRadiusActorToRefTarget(apActor, false);
                    ++i;
                }
            }
            followerDists.Sort(&CompareFollowerDist);
            float fMinFollowDist = 0.0f;
            for (i = 0; i < followerDists.iSize; ++i) {
                FollowerDist& followerInfo = followerDists.GetAt(static_cast<int32_t>(i));
                if (i) {
                    FollowerDist& kPrevious = followerDists.GetAt(static_cast<int32_t>(i - 1));
                    double dRadii = kPrevious.fActorRadius + followerInfo.fActorRadius;
                    fMinFollowDist = fFollowSpaceBetweenFollowers.Float() + dRadii + fMinFollowDist;
                }
                fMinFollowDist = BSMAX(fMinFollowDist, followerInfo.fPackDesiredDist);
                if (followerDists.GetAt(static_cast<int32_t>(i)).pActor == apActor) {
                    fFudge = fMinFollowDist - followerDists.GetAt(static_cast<int32_t>(i)).fPackDesiredDist;
                    break;
                }
            }
        }
    }
    return fFudge;
}

float AiFormulas::ComputeRandomSandboxSubProcessDuration(int32_t aiEnergyLevel, int32_t aiProcIndex) {
    float fEnergyLevel = static_cast<float>(aiEnergyLevel);
    float fDurationVal = fSandboxDurationBase.Float() * fSandboxDurationMultProc[aiProcIndex]->Float();
    float fEnergyVal = fSandboxEnergyMult.Float() * fSandboxEnergyMultProc[aiProcIndex]->Float();
    float fFinalValue = (fEnergyLevel * fEnergyVal + 1.0) * fDurationVal;
    float fMin = (1.0 - fSandboxDurationRangeMult.Float()) * fFinalValue;
    float fMax = (fSandboxDurationRangeMult.Float() + 1.0) * fFinalValue;
    return RandomFloat(fMin, fMax);
}

float AiFormulas::AVDeriveHealingRate(ActorValueOwner* apActor, [[maybe_unused]] ActorValue::Index aiWhich) {
    float fRate = 0.0f;
    switch (apActor->GetClampedActorValue(ActorValue::ENDURANCE)) {
        case 6: {
            fRate = fAVDHealRateEndurance6Bonus.Float();
            break;
        }
        case 7: {
            fRate = fAVDHealRateEndurance7Bonus.Float();
            break;
        }
        case 8: {
            fRate = fAVDHealRateEndurance8Bonus.Float();
            break;
        }
        case 9: {
            fRate = fAVDHealRateEndurance9Bonus.Float();
            break;
        }
        case 10: {
            fRate = fAVDHealRateEndurance10Bonus.Float();
            break;
        }
        default: {
            break;
        }
    }
    return fRate;
}

float AiFormulas::AVDeriveUnarmedDamage(ActorValueOwner* apActor, ActorValue::Index aiWhich) {
    if (aiWhich == ActorValue::UNARMED_DAMAGE) {
        double dScaled = apActor->GetActorFloatValue(ActorValue::UNARMED) * fAVDUnarmedDamageMult.Float();
        return fAVDUnarmedDamageBase.Float() + dScaled;
    }
    return 0.0f;
}

double AiFormulas::ComputeTimeTillNextWander(int32_t aiEnergyLevel) {
    float fEnergy = static_cast<double>(aiEnergyLevel) * fAIEnergyLevelMult.Float() + fAIEnergyLevelBase.Float();
    return static_cast<float>((fAIMaxWanderTime.Float() - fEnergy) / 2.0);
}
