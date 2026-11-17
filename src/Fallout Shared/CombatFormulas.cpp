#include "Fallout Shared/CombatFormulas.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMain/SettingCollectionMap.h"
#include "BSMenu/DebugText.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESAmmoEffect.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <cstdint>

// Used by NavMesh's grid clamp: sets aVal to aMax when aVal > aMax, else to aMin when aVal < aMin.
template <class T>
void BSClamp(T& aVal, T aMin, T aMax);

WeaponConditionReloadJamSettingTable fWeaponConditionReloadJam{
    &fWeaponConditionReloadJam1, &fWeaponConditionReloadJam2,  &fWeaponConditionReloadJam3, &fWeaponConditionReloadJam4,
    &fWeaponConditionReloadJam5, &fWeaponConditionReloadJam6,  &fWeaponConditionReloadJam7, &fWeaponConditionReloadJam8,
    &fWeaponConditionReloadJam9, &fWeaponConditionReloadJam10, &fWeaponConditionSpread1,    &fWeaponConditionSpread2,
    &fWeaponConditionSpread3,    &fWeaponConditionSpread4,     &fWeaponConditionSpread5,    &fWeaponConditionSpread6,
    &fWeaponConditionSpread7,    &fWeaponConditionSpread8,     &fWeaponConditionSpread9,    &fWeaponConditionSpread10
};

// The game settings this file defines, with their IDs and default values, in the game's initialization order.
SettingT<GameSettingCollection> fAIMeleeWeaponMult("fAIMeleeWeaponMult", 2.0f);
SettingT<GameSettingCollection> fAIMeleeArmorMult("fAIMeleeArmorMult", 1.0f);
SettingT<GameSettingCollection> fAIMeleeHandMult("fAIMeleeHandMult", 1.3f);
SettingT<GameSettingCollection> fAIRangedWeaponMult("fAIRangedWeaponMult", 5.0f);
SettingT<GameSettingCollection> fAIMagicSpellMult("fAIMagicSpellMult", 3.0f);
SettingT<GameSettingCollection> fAIRangMagicSpellMult("fAIRangMagicSpellMult", 5.0f);
SettingT<GameSettingCollection> fAIFleeHealthMult("fAIFleeHealthMult", 20.0f);
SettingT<GameSettingCollection> fAIFleeConfBase("fAIFleeConfBase", 40.0f);
SettingT<GameSettingCollection> fAIFleeConfMult("fAIFleeConfMult", -0.5f);
SettingT<GameSettingCollection> iAIFleeMaxHitCount("iAIFleeMaxHitCount", 3);
SettingT<GameSettingCollection> fAIFleeSuccessTimeout("fAIFleeSuccessTimeout", 120.0f);
SettingT<GameSettingCollection> fAIDodgeFavorLeftRightMult("fAIDodgeFavorLeftRightMult", 2.0f);
SettingT<GameSettingCollection> fAIDodgeDecisionBase("fAIDodgeDecisionBase", 10.0f);
SettingT<GameSettingCollection> fAIDodgeWalkChance("fAIDodgeWalkChance", 75.0f);
SettingT<GameSettingCollection> fAIDodgeVerticalRangedAttackMult("fAIDodgeVerticalRangedAttackMult", 0.1f);
SettingT<GameSettingCollection> fDetectionEventExpireTime("fDetectionEventExpireTime", 10.0f);
SettingT<GameSettingCollection> fAIFaceTargetAnimationAngle("fAIFaceTargetAnimationAngle", 35.0f);
SettingT<GameSettingCollection> fPlayerTargetCombatDistance("fPlayerTargetCombatDistance", 3000.0f);
SettingT<GameSettingCollection> iCombatTargetPlayerSoftCap("iCombatTargetPlayerSoftCap", 15);
SettingT<GameSettingCollection> fCombatSplashDamageMinRadius("fCombatSplashDamageMinRadius", 50.0f);
SettingT<GameSettingCollection> fCombatSplashDamageMinDamage("fCombatSplashDamageMinDamage", 20.0f);
SettingT<GameSettingCollection> fCombatSplashDamageMaxSpeed("fCombatSplashDamageMaxSpeed", 3000.0f);
SettingT<GameSettingCollection> iAvoidHurtingNonTargetsResponsibility("iAvoidHurtingNonTargetsResponsibility", 50);
SettingT<GameSettingCollection> fDangerousProjectileExplosionDamage("fDangerousProjectileExplosionDamage", 5.0f);
SettingT<GameSettingCollection> fDangerousProjectileExplosionRadius("fDangerousProjectileExplosionRadius", 30.0f);
SettingT<GameSettingCollection> fDangerousObjectExplosionDamage("fDangerousObjectExplosionDamage", 25.0f);
SettingT<GameSettingCollection> fDangerousObjectExplosionRadius("fDangerousObjectExplosionRadius", 100.0f);
SettingT<GameSettingCollection> fCombatAvoidThreatsChance("fCombatAvoidThreatsChance", 0.5f);
SettingT<GameSettingCollection> fExplosiveProjectileBlockedWaitTime("fExplosiveProjectileBlockedWaitTime", 2.5f);
SettingT<GameSettingCollection> fExplosiveProjectileBlockedResetTime("fExplosiveProjectileBlockedResetTime", 5.0f);
SettingT<GameSettingCollection> fCombatFindBetterWeaponTime("fCombatFindBetterWeaponTime", 5.0f);
SettingT<GameSettingCollection> fCombatDisarmedFindBetterWeaponTime("fCombatDisarmedFindBetterWeaponTime", 2.5f);
SettingT<GameSettingCollection>
    fCombatDisarmedFindBetterWeaponInitialTime("fCombatDisarmedFindBetterWeaponInitialTime", 1.0f);
SettingT<GameSettingCollection> fPickupWeaponMinDPSImprovementPercent("fPickupWeaponMinDPSImprovementPercent", 0.5f);
SettingT<GameSettingCollection> fPickupWeaponUnarmedDistanceMin("fPickupWeaponUnarmedDistanceMin", 256.0f);
SettingT<GameSettingCollection> fPickupWeaponUnarmedDistanceMax("fPickupWeaponUnarmedDistanceMax", 2048.0f);
SettingT<GameSettingCollection> fPickupWeaponMeleeDistanceMin("fPickupWeaponMeleeDistanceMin", 256.0f);
SettingT<GameSettingCollection> fPickupWeaponMeleeDistanceMax("fPickupWeaponMeleeDistanceMax", 1024.0f);
SettingT<GameSettingCollection> fPickupWeaponRangedDistanceMin("fPickupWeaponRangedDistanceMin", 512.0f);
SettingT<GameSettingCollection> fPickupWeaponRangedDistanceMax("fPickupWeaponRangedDistanceMax", 2048.0f);
SettingT<GameSettingCollection> gs_fDisarmedPickupWeaponDistanceMult("fDisarmedPickupWeaponDistanceMult", 2.0f);
SettingT<GameSettingCollection>
    fPickupWeaponTargetUnreachableDistanceMult("fPickupWeaponTargetUnreachableDistanceMult", 4.0f);
SettingT<GameSettingCollection>
    fPickupWeaponRangedMeleeDPSRatioThreshold("fPickupWeaponRangedMeleeDPSRatioThreshold", 0.5f);
SettingT<GameSettingCollection> fPickupWeaponDistanceMinMaxDPSMult("fPickupWeaponDistanceMinMaxDPSMult", 5.0f);
SettingT<GameSettingCollection> fPickupWeaponMeleeWeaponDPSMult("fPickupWeaponMeleeWeaponDPSMult", 0.5f);
SettingT<GameSettingCollection> fPickupItemDistanceFudge("fPickupItemDistanceFudge", 32.0f);
SettingT<GameSettingCollection> gs_fGrenadeHighArcSpeedPercentage("fGrenadeHighArcSpeedPercentage", 0.67f);
SettingT<GameSettingCollection> fCoverEvaluationLastSeenExpireTime("fCoverEvaluationLastSeenExpireTime", 5.0f);
SettingT<GameSettingCollection> fCoverFiredProjectileExpireTime("fCoverFiredProjectileExpireTime", 5.0f);
SettingT<GameSettingCollection> fCoverWaitReloadClipPercent("fCoverWaitReloadClipPercent", 0.33f);
SettingT<GameSettingCollection> fCoverFiringReloadClipPercent("fCoverFiringReloadClipPercent", 0.25f);
SettingT<GameSettingCollection> fEmbeddedWeaponSwitchChance("fEmbeddedWeaponSwitchChance", 0.5f);
SettingT<GameSettingCollection> fEmbeddedWeaponSwitchTime("fEmbeddedWeaponSwitchTime", 5.0f);
SettingT<GameSettingCollection> fMovementTargetAvoidRadiusMult("fMovementTargetAvoidRadiusMult", 1.25f);
SettingT<GameSettingCollection> fMovementTargetAvoidRadius("fMovementTargetAvoidRadius", 512.0f);
SettingT<GameSettingCollection> fMovementTargetAvoidCost("fMovementTargetAvoidCost", 25.0f);
SettingT<GameSettingCollection> fMovementNearTargetAvoidRadius("fMovementNearTargetAvoidRadius", 64.0f);
SettingT<GameSettingCollection> fMovementNearTargetAvoidCost("fMovementNearTargetAvoidCost", 25.0f);
SettingT<GameSettingCollection> fCombatIronSightsDistance("fCombatIronSightsDistance", 512.0f);
SettingT<GameSettingCollection> fCombatIronSightsRangeMult("fCombatIronSightsRangeMult", 5.0f);
SettingT<GameSettingCollection> fTargetMovedRepathLength("fTargetMovedRepathLength", 64.0f);
SettingT<GameSettingCollection> fTargetMovedCoveredMoveRepathLength("fTargetMovedCoveredMoveRepathLength", 256.0f);
SettingT<GameSettingCollection> fTargetMovedRepathLengthLow("fTargetMovedRepathLengthLow", 2048.0f);
SettingT<GameSettingCollection> iBallisticProjectilePathPickSegments("iBallisticProjectilePathPickSegments", 4);
SettingT<GameSettingCollection> fGrenadeThrowHitFractionThreshold("fGrenadeThrowHitFractionThreshold", 0.8f);
SettingT<GameSettingCollection> fNPCMaxGunWobbleAngle("fNPCMaxGunWobbleAngle", 5.0f);
SettingT<GameSettingCollection> fMeleeMovementRestrictionsUpdateTime("fMeleeMovementRestrictionsUpdateTime", 1.0f);
SettingT<GameSettingCollection> fAutomaticWeaponBurstFireTime("fAutomaticWeaponBurstFireTime", 1.0f);
SettingT<GameSettingCollection> fAutomaticWeaponBurstCooldownTime("fAutomaticWeaponBurstCooldownTime", 1.0f);
SettingT<GameSettingCollection> fCombatAbsoluteMaxRangeMult("fCombatAbsoluteMaxRangeMult", 1.5f);
SettingT<GameSettingCollection>
    fCombatCurrentWeaponAbsoluteMaxRangeMult("fCombatCurrentWeaponAbsoluteMaxRangeMult", 1.5f);
SettingT<GameSettingCollection> fCombatProjectileMaxRangeOptimalMult("fCombatProjectileMaxRangeOptimalMult", 0.85f);
SettingT<GameSettingCollection> fCombatCoverMaxRangeMult("fCombatCoverMaxRangeMult", 1.5f);
SettingT<GameSettingCollection> fCombatMissileStickDepth("fCombatMissileStickDepth", 10.0f);
SettingT<GameSettingCollection> fCombatMissileImpaleDepth("fCombatMissileImpaleDepth", 12.0f);
SettingT<GameSettingCollection> fCombatPathingLookAheadDelta("fCombatPathingLookAheadDelta", 0.5f);
SettingT<GameSettingCollection> fCoveredAdvanceMinAdvanceDistanceMin("fCoveredAdvanceMinAdvanceDistanceMin", 256.0f);
SettingT<GameSettingCollection> fCoveredAdvanceMinAdvanceDistanceMax("fCoveredAdvanceMinAdvanceDistanceMax", 512.0f);
SettingT<GameSettingCollection>
    fCombatGroupFleeingTargetLostRemoveTime("fCombatGroupFleeingTargetLostRemoveTime", 30.0f);
SettingT<GameSettingCollection>
    fCombatGroupFleeingTargetLostRemoveDistanceTime("fCombatGroupFleeingTargetLostRemoveDistanceTime", 15.0f);
SettingT<GameSettingCollection>
    fCombatGroupFleeingTargetLostRemoveDistance("fCombatGroupFleeingTargetLostRemoveDistance", 3072.0f);
SettingT<GameSettingCollection> fCombatTargetLostRemoveTime("fCombatTargetLostRemoveTime", 30.0f);
SettingT<GameSettingCollection> fCombatTargetLostRemoveDistanceTime("fCombatTargetLostRemoveDistanceTime", 60.0f);
SettingT<GameSettingCollection> fCombatTargetLostRemoveDistance("fCombatTargetLostRemoveDistance", 4096.0f);
SettingT<GameSettingCollection> iCombatTargetLostRemoveSearchCount("iCombatTargetLostRemoveSearchCount", 2);
SettingT<GameSettingCollection> fCombatSearchTargetHiddenRemoveTime("fCombatSearchTargetHiddenRemoveTime", 10.0f);
SettingT<GameSettingCollection> fCombatSearchTargetHiddenMaxTime("fCombatSearchTargetHiddenMaxTime", 20.0f);
SettingT<GameSettingCollection> fCombatLOSBufferTime("fCombatLOSBufferTime", 5.0f);
SettingT<GameSettingCollection> fCombatDetectionBufferTime("fCombatDetectionBufferTime", 10.0f);
SettingT<GameSettingCollection> fCombatDetectionLostTime("fCombatDetectionLostTime", 15.0f);
SettingT<GameSettingCollection> fCombatFollowSneakFollowRadius("fCombatFollowSneakFollowRadius", 512.0f);
SettingT<GameSettingCollection> fCombatFollowRadiusMult("fCombatFollowRadiusMult", 3.0f);
SettingT<GameSettingCollection> fCombatFollowRadiusMin("fCombatFollowRadiusMin", 512.0f);
SettingT<GameSettingCollection> fCombatFollowRadiusBase("fCombatFollowRadiusBase", 512.0f);
SettingT<GameSettingCollection> fCombatTeammateFollowRadiusMult("fCombatTeammateFollowRadiusMult", 3.0f);
SettingT<GameSettingCollection> fCombatTeammateFollowRadiusMin("fCombatTeammateFollowRadiusMin", 512.0f);
SettingT<GameSettingCollection> fCombatTeammateFollowRadiusBase("fCombatTeammateFollowRadiusBase", 512.0f);
SettingT<GameSettingCollection> fCombatGuardRadiusMult("fCombatGuardRadiusMult", 1.5f);
SettingT<GameSettingCollection> fCombatGuardRadiusMin("fCombatGuardRadiusMin", 384.0f);
SettingT<GameSettingCollection> fCombatLocationTargetRadiusMin("fCombatLocationTargetRadiusMin", 10.0f);
SettingT<GameSettingCollection> fCombatGuardFollowBufferDistance("fCombatGuardFollowBufferDistance", 384.0f);
SettingT<GameSettingCollection> fCombatItemRestoreTimer("fCombatItemRestoreTimer", 30.0f);
SettingT<GameSettingCollection> fCombatItemBuffTimer("fCombatItemBuffTimer", 45.0f);
SettingT<GameSettingCollection> fCombatInventoryUpdateTimer("fCombatInventoryUpdateTimer", 5.0f);
SettingT<GameSettingCollection> fCombatFiringArcStationaryTurnMult("fCombatFiringArcStationaryTurnMult", 0.75f);
SettingT<GameSettingCollection> fCombatRadiusMinMult("fCombatRadiusMinMult", 0.9f);
SettingT<GameSettingCollection> fCombatMoveToActorBufferDistance("fCombatMoveToActorBufferDistance", 64.0f);
SettingT<GameSettingCollection> fCombatHideFailedTargetDistance("fCombatHideFailedTargetDistance", 384.0f);
SettingT<GameSettingCollection> fCombatFleeWaitTime("fCombatFleeWaitTime", 60.0f);
SettingT<GameSettingCollection> fCombatFleeMinAllyDistance("fCombatFleeMinAllyDistance", 1536.0f);
SettingT<GameSettingCollection> fCombatFleeMaxAllyDistance("fCombatFleeMaxAllyDistance", 4096.0f);
SettingT<GameSettingCollection> fCombatFleeMaxDoorDistance("fCombatFleeMaxDoorDistance", 1024.0f);
SettingT<GameSettingCollection> fCombatFleeMinCoverDistance("fCombatFleeMinCoverDistance", 1536.0f);
SettingT<GameSettingCollection> fCombatFleeMaxCoverDistance("fCombatFleeMaxCoverDistance", 3072.0f);
SettingT<GameSettingCollection> fCombatFleeNormalDistance("fCombatFleeNormalDistance", 2048.0f);
SettingT<GameSettingCollection>
    fCombatFleeBoostConfidenceTargetRadius("fCombatFleeBoostConfidenceTargetRadius", 512.0f);
SettingT<GameSettingCollection> fCombatFleeTargetAvoidRadius("fCombatFleeTargetAvoidRadius", 2048.0f);
SettingT<GameSettingCollection> fCombatFleeTargetGatherRadius("fCombatFleeTargetGatherRadius", 3072.0f);
SettingT<GameSettingCollection> fCombatFleeTargetAvoidCost("fCombatFleeTargetAvoidCost", 5.0f);
SettingT<GameSettingCollection> fCombatFleeLastDoorRestrictTime("fCombatFleeLastDoorRestrictTime", 30.0f);
SettingT<GameSettingCollection> fCombatFleeUseDoorRestrictTime("fCombatFleeUseDoorRestrictTime", 15.0f);
SettingT<GameSettingCollection> fCombatFleeInitialDoorRestrictChance("fCombatFleeInitialDoorRestrictChance", 0.75f);
SettingT<GameSettingCollection> fCombatFleeDoorTargetCheckDistance("fCombatFleeDoorTargetCheckDistance", 128.0f);
SettingT<GameSettingCollection> fCombatSearchUpdateTime("fCombatSearchUpdateTime", 1.0f);
SettingT<GameSettingCollection> fCombatSearchAreaUpdateTime("fCombatSearchAreaUpdateTime", 5.0f);
SettingT<GameSettingCollection> fCombatSearchSightRadius("fCombatSearchSightRadius", 1536.0f);
SettingT<GameSettingCollection> fCombatSearchExteriorMinRadius("fCombatSearchExteriorMinRadius", 1024.0f);
SettingT<GameSettingCollection> fCombatSearchExteriorMediumRadius("fCombatSearchExteriorMediumRadius", 2048.0f);
SettingT<GameSettingCollection> fCombatSearchExteriorMaxRadius("fCombatSearchExteriorMaxRadius", 4096.0f);
SettingT<GameSettingCollection> fCombatSearchInteriorMinRadius("fCombatSearchInteriorMinRadius", 512.0f);
SettingT<GameSettingCollection> fCombatSearchInteriorMediumRadius("fCombatSearchInteriorMediumRadius", 1024.0f);
SettingT<GameSettingCollection> fCombatSearchInteriorMaxRadius("fCombatSearchInteriorMaxRadius", 2048.0f);
SettingT<GameSettingCollection> fCombatSearchBufferDistance("fCombatSearchBufferDistance", 256.0f);
SettingT<GameSettingCollection> fCombatSearchLocationRemoveDistance("fCombatSearchLocationRemoveDistance", 256.0f);
SettingT<GameSettingCollection> iCombatSearchMaxSearchDoorFailure("iCombatSearchMaxSearchDoorFailure", 5);
SettingT<GameSettingCollection> fCombatSearchLookTime("fCombatSearchLookTime", 1.0f);
SettingT<GameSettingCollection>
    fCombatSearchCheckDestinationMaxDistance("fCombatSearchCheckDestinationMaxDistance", 512.0f);
SettingT<GameSettingCollection>
    fCombatSearchCheckDestinationMidDistance("fCombatSearchCheckDestinationMidDistance", 384.0f);
SettingT<GameSettingCollection>
    fCombatSearchCheckDestinationMinDistance("fCombatSearchCheckDestinationMinDistance", 256.0f);
SettingT<GameSettingCollection> fCombatSearchCheckDestinationTime("fCombatSearchCheckDestinationTime", 1.0f);
SettingT<GameSettingCollection>
    fCombatSearchCheckInvestigateMaxDistance("fCombatSearchCheckInvestigateMaxDistance", 512.0f);
SettingT<GameSettingCollection> fCombatSearchCheckInvestigateTime("fCombatSearchCheckInvestigateTime", 1.0f);
SettingT<GameSettingCollection>
    fCombatSearchCheckInvestigateInitialTime("fCombatSearchCheckInvestigateInitialTime", 2.0f);
SettingT<GameSettingCollection> fCombatSearchCheckDoorDistance("fCombatSearchCheckDoorDistance", 1536.0f);
SettingT<GameSettingCollection> fCombatSearchCheckDoorMovingDistance("fCombatSearchCheckDoorMovingDistance", 1024.0f);
SettingT<GameSettingCollection> fCombatSearchCheckDoorLowDistance("fCombatSearchCheckDoorLowDistance", 4096.0f);
SettingT<GameSettingCollection> fCombatSearchCheckDoorMovingTime("fCombatSearchCheckDoorMovingTime", 5.0f);
SettingT<GameSettingCollection> fCombatDialogueAttackMinElapsedTime("fCombatDialogueAttackMinElapsedTime", 1.0f);
SettingT<GameSettingCollection> fCombatDialogueAttackMaxElapsedTime("fCombatDialogueAttackMaxElapsedTime", 5.0f);
SettingT<GameSettingCollection>
    fCombatDialogueFireExplosiveMinElapsedTime("fCombatDialogueFireExplosiveMinElapsedTime", 1.0f);
SettingT<GameSettingCollection>
    fCombatDialogueFireExplosiveMaxElapsedTime("fCombatDialogueFireExplosiveMaxElapsedTime", 5.0f);
SettingT<GameSettingCollection>
    fCombatDialogueAvoidThreatMinElapsedTime("fCombatDialogueAvoidThreatMinElapsedTime", 3.0f);
SettingT<GameSettingCollection>
    fCombatDialogueAvoidThreatMaxElapsedTime("fCombatDialogueAvoidThreatMaxElapsedTime", 5.0f);
SettingT<GameSettingCollection> fCombatDialogueFleeMinElapsedTime("fCombatDialogueFleeMinElapsedTime", 3.0f);
SettingT<GameSettingCollection> fCombatDialogueFleeMaxElapsedTime("fCombatDialogueFleeMaxElapsedTime", 5.0f);
SettingT<GameSettingCollection> fCombatDetectionDialogueMinElapsedTime("fCombatDetectionDialogueMinElapsedTime", 1.0f);
SettingT<GameSettingCollection> fCombatDetectionDialogueMaxElapsedTime("fCombatDetectionDialogueMaxElapsedTime", 3.0f);
SettingT<GameSettingCollection>
    fCombatDetectionDialogueIdleMinElapsedTime("fCombatDetectionDialogueIdleMinElapsedTime", 3.0f);
SettingT<GameSettingCollection>
    fCombatDetectionDialogueIdleMaxElapsedTime("fCombatDetectionDialogueIdleMaxElapsedTime", 10.0f);
SettingT<GameSettingCollection> gs_fCombatThreatExtrudeVelocityThreshold("fCombatThreatExtrudeVelocityThreshold", 2.0f);
SettingT<GameSettingCollection> fCombatThreatExtrudeTime("fCombatThreatExtrudeTime", 2.0f);
SettingT<GameSettingCollection> fCombatThreatNegativeExtrudeTime("fCombatThreatNegativeExtrudeTime", 0.5f);
SettingT<GameSettingCollection> fCombatThreatTimedExplosionLength("fCombatThreatTimedExplosionLength", 5.0f);
SettingT<GameSettingCollection> fCombatThreatExplosiveObjectThreatTime("fCombatThreatExplosiveObjectThreatTime", 3.0f);
SettingT<GameSettingCollection> fCombatThreatDangerousObjectHealth("fCombatThreatDangerousObjectHealth", 30.0f);
SettingT<GameSettingCollection> fCombatThreatSignificantScore("fCombatThreatSignificantScore", 0.05f);
SettingT<GameSettingCollection> fCombatThreatAvoidCost("fCombatThreatAvoidCost", 1.0f);
SettingT<GameSettingCollection>
    fCombatThreatProximityExplosionAvoidTime("fCombatThreatProximityExplosionAvoidTime", 10.0f);
SettingT<GameSettingCollection> fCombatMusicUpdateTime("fCombatMusicUpdateTime", 1.0f);
SettingT<GameSettingCollection> fCombatMusicStopTime("fCombatMusicStopTime", 5.0f);
SettingT<GameSettingCollection> fCombatMusicNearCombatInnerRadius("fCombatMusicNearCombatInnerRadius", 2500.0f);
SettingT<GameSettingCollection> fCombatMusicNearCombatOuterRadius("fCombatMusicNearCombatOuterRadius", 3500.0f);
SettingT<GameSettingCollection> fCombatMusicPlayerTargetedThreatRatio("fCombatMusicPlayerTargetedThreatRatio", 2.0f);
SettingT<GameSettingCollection> fCombatMusicGroupThreatRatioTimer("fCombatMusicGroupThreatRatioTimer", 1.0f);
SettingT<GameSettingCollection> fCombatMusicGroupThreatRatioMin("fCombatMusicGroupThreatRatioMin", 0.5f);
SettingT<GameSettingCollection> fCombatMusicGroupThreatRatioMax("fCombatMusicGroupThreatRatioMax", 2.0f);
SettingT<GameSettingCollection> fCombatMusicPlayerNearStrengthMult("fCombatMusicPlayerNearStrengthMult", 1.0f);
SettingT<GameSettingCollection> fCombatMusicPlayerCombatStrengthCap("fCombatMusicPlayerCombatStrengthCap", 35000.0f);
SettingT<GameSettingCollection> fCombatClusterUpdateTime("fCombatClusterUpdateTime", 1.0f);
SettingT<GameSettingCollection> fCombatYieldTime("fCombatYieldTime", 10.0f);
SettingT<GameSettingCollection> fCombatYieldRetryTime("fCombatYieldRetryTime", 600.0f);
SettingT<GameSettingCollection> fCombatStrengthUpdateTime("fCombatStrengthUpdateTime", 1.0f);
SettingT<GameSettingCollection> fCombatThreatRatioUpdateTime("fCombatThreatRatioUpdateTime", 5.0f);
SettingT<GameSettingCollection> fCombatGroupCombatStrengthUpdateTime("fCombatGroupCombatStrengthUpdateTime", 2.0f);
SettingT<GameSettingCollection> fCombatConfidenceModifierMin("fCombatConfidenceModifierMin", -0.5f);
SettingT<GameSettingCollection> fCombatConfidenceModifierMax("fCombatConfidenceModifierMax", 0.5f);
SettingT<GameSettingCollection> fCombatLowTargetHitPercent("fCombatLowTargetHitPercent", 0.5f);
SettingT<GameSettingCollection> fCombatLowFleeingTargetHitPercent("fCombatLowFleeingTargetHitPercent", 0.25f);
SettingT<GameSettingCollection> fCombatLowMaxAttackDistance("fCombatLowMaxAttackDistance", 2048.0f);
SettingT<GameSettingCollection> fAIPowerAttackCreatureChance("fAIPowerAttackCreatureChance", 75.0f);
SettingT<GameSettingCollection> fAIPowerAttackNPCChance("fAIPowerAttackNPCChance", 20.0f);
SettingT<GameSettingCollection> fAIPowerAttackRecoilBonus("fAIPowerAttackRecoilBonus", 10.0f);
SettingT<GameSettingCollection> fAIPowerAttackKnockdownBonus("fAIPowerAttackKnockdownBonus", 15.0f);
SettingT<GameSettingCollection> fAIPowerAttackFatigueBase("fAIPowerAttackFatigueBase", 15.0f);
SettingT<GameSettingCollection> fAIPowerAttackFatigueMult("fAIPowerAttackFatigueMult", -15.0f);
SettingT<GameSettingCollection> fDamageWeaponMult("fDamageWeaponMult", 1.0f);
SettingT<GameSettingCollection> fDamageSkillBase("fDamageSkillBase", 0.2f);
SettingT<GameSettingCollection> fDamageSkillMult("fDamageSkillMult", 1.8f);
SettingT<GameSettingCollection> fDamageGunWeapCondBase("fDamageGunWeapCondBase", 0.0f);
SettingT<GameSettingCollection> fDamageGunWeapCondMult("fDamageGunWeapCondMult", 1.0f);
SettingT<GameSettingCollection> fDamageMeleeWeapCondBase("fDamageMeleeWeapCondBase", 0.0f);
SettingT<GameSettingCollection> fDamageMeleeWeapCondMult("fDamageMeleeWeapCondMult", 1.0f);
SettingT<GameSettingCollection> fDamageStrengthBase("fDamageStrengthBase", 0.5f);
SettingT<GameSettingCollection> fDamageStrengthMult("fDamageStrengthMult", 1.0f);
SettingT<GameSettingCollection> fDamageToArmorPercentage("fDamageToArmorPercentage", 0.5f);
SettingT<GameSettingCollection> fDamagePowerAttackBonus("fDamagePowerAttackBonus", 3.0f);
SettingT<GameSettingCollection> fDamagePowerAttackStandBonus("fDamagePowerAttackStandBonus", 4.0f);
SettingT<GameSettingCollection> fDamagePowerAttackSideBonus("fDamagePowerAttackSideBonus", 3.0f);
SettingT<GameSettingCollection> fDamagePowerAttackBackBonus("fDamagePowerAttackBackBonus", 3.0f);
SettingT<GameSettingCollection> fDamagePowerAttackForwardBonus("fDamagePowerAttackForwardBonus", 3.0f);
SettingT<GameSettingCollection> fDamageSneakAttackMult("fDamageSneakAttackMult", 4.0f);
SettingT<GameSettingCollection> fAutoAimMaxDistance("fAutoAimMaxDistance", 1024.0f);
SettingT<GameSettingCollection> fAutoAimScreenPercentage("fAutoAimScreenPercentage", 3.0f);
SettingT<GameSettingCollection> fAutoAimMaxDegrees("fAutoAimMaxDegrees", 10.0f);
SettingT<GameSettingCollection> fAutoAimMaxDegrees3rdPerson("fAutoAimMaxDegrees3rdPerson", 10.0f);
SettingT<GameSettingCollection> fAutoAimMaxDegreesVATS("fAutoAimMaxDegreesVATS", 15.0f);
SettingT<GameSettingCollection> fAutoAimMaxDegreesMelee("fAutoAimMaxDegreesMelee", 90.0f);
SettingT<GameSettingCollection> fAutoAimMaxDegreesMiss("fAutoAimMaxDegreesMiss", 3.0f);
SettingT<GameSettingCollection> fAutoAimMeleeChaseMult("fAutoAimMeleeChaseMult", 1.0f);
SettingT<GameSettingCollection> fAutoAimMissRatioLow("fAutoAimMissRatioLow", 0.86f);
SettingT<GameSettingCollection> fAutoAimMissRatioHigh("fAutoAimMissRatioHigh", 1.15f);
SettingT<GameSettingCollection> fDOFDistanceMult("fDOFDistanceMult", 5.0f);
SettingT<GameSettingCollection> fIronSightsDOFSwitchSeconds("fIronSightsDOFSwitchSeconds", 0.15f);
SettingT<GameSettingCollection> fIronSightsDOFStrengthCap("fIronSightsDOFStrengthCap", 1.7f);
SettingT<GameSettingCollection> fIronSightsDOFDistance("fIronSightsDOFDistance", 310.0f);
SettingT<GameSettingCollection> fIronSightsDOFRange("fIronSightsDOFRange", 600.0f);
SettingT<GameSettingCollection> fVATSDOFSwitchSeconds("fVATSDOFSwitchSeconds", 0.15f);
SettingT<GameSettingCollection> fVATSDOFStrengthCap("fVATSDOFStrengthCap", 1.0f);
SettingT<GameSettingCollection> fVATSDOFRange("fVATSDOFRange", 300.0f);
SettingT<GameSettingCollection> fVATSRadialStrength("fVATSRadialStrength", 0.25f);
SettingT<GameSettingCollection> fVATSRadialRampup("fVATSRadialRampup", 5.0f);
SettingT<GameSettingCollection> fVATSRadialStart("fVATSRadialStart", 0.3f);
SettingT<GameSettingCollection> fIronSightsMotionBlur("fIronSightsMotionBlur", 0.15f);
SettingT<GameSettingCollection> fIronSightsGunMotionBlur("fIronSightsGunMotionBlur", 0.1f);
SettingT<GameSettingCollection> fMotionBlur("fMotionBlur", 0.0f);
SettingT<GameSettingCollection> fObjectMotionBlur("fObjectMotionBlur", 100.0f);
SettingT<GameSettingCollection> fVATSPlayerTimeUpdateMult("fVATSPlayerTimeUpdateMult", 3.0f);
SettingT<GameSettingCollection> fVATSTargetTimeUpdateMult("fVATSTargetTimeUpdateMult", 1.0f);
SettingT<GameSettingCollection> fVATSShotBurstTime("fVATSShotBurstTime", 0.43f);
SettingT<GameSettingCollection> fVATSShotLongBurstTime("fVATSShotLongBurstTime", 0.75f);
SettingT<GameSettingCollection> iVATSCameraHitDist("iVATSCameraHitDist", 100);
SettingT<GameSettingCollection> fVATSCameraMaxTime("fVATSCameraMaxTime", 20.0f);
SettingT<GameSettingCollection> fVATSCameraMinTime("fVATSCameraMinTime", 0.0f);
SettingT<GameSettingCollection> fVATSGrenadeDistAimZMult("fVATSGrenadeDistAimZMult", 0.4f);
SettingT<GameSettingCollection> fVATSImageSpaceTransitionTime("fVATSImageSpaceTransitionTime", 0.15f);
SettingT<GameSettingCollection> fVATSPlaybackDelay("fVATSPlaybackDelay", 3.0f);
SettingT<GameSettingCollection> fVATSCamTransRBStrengthCap("fVATSCamTransRBStrengthCap", 0.5f);
SettingT<GameSettingCollection> fVATSCamTransRBRampup("fVATSCamTransRBRampup", 2.0f);
SettingT<GameSettingCollection> fVATSCamTransRBStart("fVATSCamTransRBStart", 0.0f);
SettingT<GameSettingCollection> fVATSCamTransRBRampDown("fVATSCamTransRBRampDown", 0.0f);
SettingT<GameSettingCollection> fVATSCamTransRBDownStart("fVATSCamTransRBDownStart", 0.0f);
SettingT<GameSettingCollection> fVATSPlayerDamageMult("fVATSPlayerDamageMult", 1.0f);
SettingT<GameSettingCollection> fVATSMeleeMaxDistance("fVATSMeleeMaxDistance", 300.0f);
SettingT<GameSettingCollection> fVATSMeleeReachMult("fVATSMeleeReachMult", 2.0f);
SettingT<GameSettingCollection> fVATSStrangerDistance("fVATSStrangerDistance", 192.0f);
SettingT<GameSettingCollection> fVATSStrangerOdds("fVATSStrangerOdds", 0.1f);
SettingT<GameSettingCollection> fVATSFortuneOdds("fVATSFortuneOdds", 0.1f);
SettingT<GameSettingCollection> iVATSStrangerMaxHP("iVATSStrangerMaxHP", 50);
SettingT<GameSettingCollection> fVatsShotgunSpreadRatio("fVatsShotgunSpreadRatio", 1.0f);
SettingT<GameSettingCollection> fVATSParalyzePalmChance("fVATSParalyzePalmChance", 0.1f);
SettingT<GameSettingCollection> fVATSGrenadeSuccessExplodeTimer("fVATSGrenadeSuccessExplodeTimer", 0.0f);
SettingT<GameSettingCollection> fVATSGrenadeSuccessMaxDistance("fVATSGrenadeSuccessMaxDistance", 75.0f);
SettingT<GameSettingCollection> fVATSCriticalChanceBonus("fVATSCriticalChanceBonus", 15.0f);
SettingT<GameSettingCollection> fVATSH2HWarpDistanceMult("fVATSH2HWarpDistanceMult", 0.5f);
SettingT<GameSettingCollection> fVATSMeleeWarpDistanceMult("fVATSMeleeWarpDistanceMult", 0.5f);
SettingT<GameSettingCollection> fMinDamMultiplier("fMinDamMultiplier", 0.2f);
SettingT<GameSettingCollection> fVATSAutomaticMeleeDamageMult("fVATSAutomaticMeleeDamageMult", 2.0f);
SettingT<GameSettingCollection> fWeaponConditionJam1("fWeaponConditionJam1", 0.4f);
SettingT<GameSettingCollection> fWeaponConditionJam2("fWeaponConditionJam2", 0.2f);
SettingT<GameSettingCollection> fWeaponConditionJam3("fWeaponConditionJam3", 0.1f);
SettingT<GameSettingCollection> fWeaponConditionJam4("fWeaponConditionJam4", 0.05f);
SettingT<GameSettingCollection> fWeaponConditionJam5("fWeaponConditionJam5", 0.0f);
SettingT<GameSettingCollection> fWeaponConditionJam6("fWeaponConditionJam6", 0.0f);
SettingT<GameSettingCollection> fWeaponConditionJam7("fWeaponConditionJam7", 0.0f);
SettingT<GameSettingCollection> fWeaponConditionJam8("fWeaponConditionJam8", 0.0f);
SettingT<GameSettingCollection> fWeaponConditionJam9("fWeaponConditionJam9", 0.0f);
SettingT<GameSettingCollection> fWeaponConditionJam10("fWeaponConditionJam10", 0.0f);
SettingT<GameSettingCollection> fWeaponConditionRateOfFire1("fWeaponConditionRateOfFire1", 0.5f);
SettingT<GameSettingCollection> fWeaponConditionRateOfFire2("fWeaponConditionRateOfFire2", 0.55f);
SettingT<GameSettingCollection> fWeaponConditionRateOfFire3("fWeaponConditionRateOfFire3", 0.6f);
SettingT<GameSettingCollection> fWeaponConditionRateOfFire4("fWeaponConditionRateOfFire4", 0.65f);
SettingT<GameSettingCollection> fWeaponConditionRateOfFire5("fWeaponConditionRateOfFire5", 0.7f);
SettingT<GameSettingCollection> fWeaponConditionRateOfFire6("fWeaponConditionRateOfFire6", 0.75f);
SettingT<GameSettingCollection> fWeaponConditionRateOfFire7("fWeaponConditionRateOfFire7", 0.8f);
SettingT<GameSettingCollection> fWeaponConditionRateOfFire8("fWeaponConditionRateOfFire8", 0.85f);
SettingT<GameSettingCollection> fWeaponConditionRateOfFire9("fWeaponConditionRateOfFire9", 0.9f);
SettingT<GameSettingCollection> fWeaponConditionRateOfFire10("fWeaponConditionRateOfFire10", 1.0f);
SettingT<GameSettingCollection> fWeaponConditionReloadJam1("fWeaponConditionReloadJam1", 1.0f);
SettingT<GameSettingCollection> fWeaponConditionReloadJam2("fWeaponConditionReloadJam2", 0.75f);
SettingT<GameSettingCollection> fWeaponConditionReloadJam3("fWeaponConditionReloadJam3", 0.5f);
SettingT<GameSettingCollection> fWeaponConditionReloadJam4("fWeaponConditionReloadJam4", 0.25f);
SettingT<GameSettingCollection> fWeaponConditionReloadJam5("fWeaponConditionReloadJam5", 0.1f);
SettingT<GameSettingCollection> fWeaponConditionReloadJam6("fWeaponConditionReloadJam6", 0.0f);
SettingT<GameSettingCollection> fWeaponConditionReloadJam7("fWeaponConditionReloadJam7", 0.0f);
SettingT<GameSettingCollection> fWeaponConditionReloadJam8("fWeaponConditionReloadJam8", 0.0f);
SettingT<GameSettingCollection> fWeaponConditionReloadJam9("fWeaponConditionReloadJam9", 0.0f);
SettingT<GameSettingCollection> fWeaponConditionReloadJam10("fWeaponConditionReloadJam10", 0.0f);
SettingT<GameSettingCollection> fWeaponConditionSpread1("fWeaponConditionSpread1", 1.0f);
SettingT<GameSettingCollection> fWeaponConditionSpread2("fWeaponConditionSpread2", 0.8f);
SettingT<GameSettingCollection> fWeaponConditionSpread3("fWeaponConditionSpread3", 0.7f);
SettingT<GameSettingCollection> fWeaponConditionSpread4("fWeaponConditionSpread4", 0.6f);
SettingT<GameSettingCollection> fWeaponConditionSpread5("fWeaponConditionSpread5", 0.5f);
SettingT<GameSettingCollection> fWeaponConditionSpread6("fWeaponConditionSpread6", 0.4f);
SettingT<GameSettingCollection> fWeaponConditionSpread7("fWeaponConditionSpread7", 0.3f);
SettingT<GameSettingCollection> fWeaponConditionSpread8("fWeaponConditionSpread8", 0.2f);
SettingT<GameSettingCollection> fWeaponConditionSpread9("fWeaponConditionSpread9", 0.1f);
SettingT<GameSettingCollection> fWeaponConditionSpread10("fWeaponConditionSpread10", 0.0f);
SettingT<GameSettingCollection> fWeapStrengthReqPenalty("fWeapStrengthReqPenalty", 0.1f);
SettingT<GameSettingCollection> fWeapSkillReqPenalty("fWeapSkillReqPenalty", 0.1f);
SettingT<GameSettingCollection> fHandDamageSkillBase("fHandDamageSkillBase", 0.0f);
SettingT<GameSettingCollection> fHandDamageSkillMult("fHandDamageSkillMult", 1.0f);
SettingT<GameSettingCollection> fHandDamageStrengthBase("fHandDamageStrengthBase", 0.0f);
SettingT<GameSettingCollection> fHandDamageStrengthMult("fHandDamageStrengthMult", 0.75f);
SettingT<GameSettingCollection> fHandHealthMin("fHandHealthMin", 1.0f);
SettingT<GameSettingCollection> fHandHealthMax("fHandHealthMax", 20.0f);
SettingT<GameSettingCollection> fHandFatigueDamageMult("fHandFatigueDamageMult", 5.0f);
SettingT<GameSettingCollection> fHandFatigueDamageBase("fHandFatigueDamageBase", 0.0f);
SettingT<GameSettingCollection> fHandReachMult("fHandReachMult", 0.5f);
SettingT<GameSettingCollection> iArmorDamageCuirassChance("iArmorDamageCuirassChance", 25);
SettingT<GameSettingCollection> iArmorDamageGreavesChance("iArmorDamageGreavesChance", 15);
SettingT<GameSettingCollection> iArmorDamageHelmChance("iArmorDamageHelmChance", 10);
SettingT<GameSettingCollection> iArmorDamageGauntletsChance("iArmorDamageGauntletsChance", 10);
SettingT<GameSettingCollection> iArmorDamageBootsChance("iArmorDamageBootsChance", 10);
SettingT<GameSettingCollection> iArmorDamageShieldChance("iArmorDamageShieldChance", 30);
SettingT<GameSettingCollection> fBlockSkillBase("fBlockSkillBase", 0.0f);
SettingT<GameSettingCollection> fBlockSkillMult("fBlockSkillMult", 1.0f);
SettingT<GameSettingCollection> fBlockMax("fBlockMax", 0.75f);
SettingT<GameSettingCollection> fBlockAmountWeaponMult("fBlockAmountWeaponMult", 0.5f);
SettingT<GameSettingCollection> fBlockAmountHandToHandMult("fBlockAmountHandToHandMult", 0.25f);
SettingT<GameSettingCollection> fBlockScoreNoShieldMult("fBlockScoreNoShieldMult", 0.5f);
SettingT<GameSettingCollection> fCombatMinEngageDistance("fCombatMinEngageDistance", 64.0f);
SettingT<GameSettingCollection> fCombatDistance("fCombatDistance", 128.0f);
SettingT<GameSettingCollection> fCombatHitConeAngle("fCombatHitConeAngle", 35.0f);
SettingT<GameSettingCollection> fCombatDeadActorHitConeMult("fCombatDeadActorHitConeMult", 2.0f);
SettingT<GameSettingCollection> fCombatStepAdvanceDistance("fCombatStepAdvanceDistance", 128.0f);
SettingT<GameSettingCollection> fCombatSpeakAttackChance("fCombatSpeakAttackChance", 0.1f);
SettingT<GameSettingCollection> fCombatSpeakPowerAttackChance("fCombatSpeakPowerAttackChance", 0.5f);
SettingT<GameSettingCollection> fCombatSpeakTauntChance("fCombatSpeakTauntChance", 0.01f);
SettingT<GameSettingCollection> fCombatSpeakHitChance("fCombatSpeakHitChance", 0.35f);
SettingT<GameSettingCollection> fCombatSpeakHitThreshold("fCombatSpeakHitThreshold", 0.1f);
SettingT<GameSettingCollection> fCombatForwardAttackChance("fCombatForwardAttackChance", 0.25f);
SettingT<GameSettingCollection> fCombatAdvanceNormalAttackChance("fCombatAdvanceNormalAttackChance", 0.0f);
SettingT<GameSettingCollection> fCombatSweepHitConeAngle("fCombatSweepHitConeAngle", 100.0f);
SettingT<GameSettingCollection> fCombatOverheadHitConeAngle("fCombatOverheadHitConeAngle", 40.0f);
SettingT<GameSettingCollection> fCombatUppercutHitConeAngle("fCombatUppercutHitConeAngle", 50.0f);
SettingT<GameSettingCollection> fCombatBetweenAdvanceTimer("fCombatBetweenAdvanceTimer", 2.0f);
SettingT<GameSettingCollection> fCombatStaffTimer("fCombatStaffTimer", 2.0f);
SettingT<GameSettingCollection> fKnockdownAgilBase("fKnockdownAgilBase", 0.0f);
SettingT<GameSettingCollection> fKnockdownAgilMult("fKnockdownAgilMult", 1.0f);
SettingT<GameSettingCollection> fKnockdownDamageBase("fKnockdownDamageBase", 0.0f);
SettingT<GameSettingCollection> fKnockdownDamageMult("fKnockdownDamageMult", 0.3f);
SettingT<GameSettingCollection> fKnockdownChance("fKnockdownChance", 0.25f);
SettingT<GameSettingCollection> fKnockbackAgilBase("fKnockbackAgilBase", 1.0f);
SettingT<GameSettingCollection> fKnockbackAgilMult("fKnockbackAgilMult", -0.008f);
SettingT<GameSettingCollection> fKnockbackDamageBase("fKnockbackDamageBase", 50.0f);
SettingT<GameSettingCollection> fKnockbackDamageMult("fKnockbackDamageMult", 10.0f);
SettingT<GameSettingCollection> fKnockbackForceMax("fKnockbackForceMax", 512.0f);
SettingT<GameSettingCollection> fKnockbackTime("fKnockbackTime", 1.0f);
SettingT<GameSettingCollection> fFatigueAttackWeaponBase("fFatigueAttackWeaponBase", 8.0f);
SettingT<GameSettingCollection> fFatigueAttackWeaponMult("fFatigueAttackWeaponMult", 0.1f);
SettingT<GameSettingCollection> fPowerAttackFatiguePenalty("fPowerAttackFatiguePenalty", 5.0f);
SettingT<GameSettingCollection> fFatigueBlockBase("fFatigueBlockBase", 0.0f);
SettingT<GameSettingCollection> fFatigueBlockMult("fFatigueBlockMult", 1.0f);
SettingT<GameSettingCollection> fFatigueBlockSkillBase("fFatigueBlockSkillBase", 5.0f);
SettingT<GameSettingCollection> fFatigueBlockSkillMult("fFatigueBlockSkillMult", -0.04f);
SettingT<GameSettingCollection> fMarksmanFatigueBurnPerSecond("fMarksmanFatigueBurnPerSecond", 15.0f);
SettingT<GameSettingCollection> iMarksmanFatigueBurnPerSecondSkill("iMarksmanFatigueBurnPerSecondSkill", 20);
SettingT<GameSettingCollection> fMarksmanFatigueBurnPerShot("fMarksmanFatigueBurnPerShot", 5.0f);
SettingT<GameSettingCollection> fFleeDistanceExterior("fFleeDistanceExterior", 2000.0f);
SettingT<GameSettingCollection> fFleeDistanceInterior("fFleeDistanceInterior", 300.0f);
SettingT<GameSettingCollection> fFleeDoneDistanceExterior("fFleeDoneDistanceExterior", 2500.0f);
SettingT<GameSettingCollection> fFleeDoneDistanceInterior("fFleeDoneDistanceInterior", 600.0f);
SettingT<GameSettingCollection> fAICombatFleeScoreThreshold("fAICombatFleeScoreThreshold", 10.0f);
SettingT<GameSettingCollection> fGunSpreadCondBase("fGunSpreadCondBase", 1.0f);
SettingT<GameSettingCollection> fGunSpreadCondMult("fGunSpreadCondMult", -1.0f);
SettingT<GameSettingCollection> fGunSpreadSkillBase("fGunSpreadSkillBase", 1.0f);
SettingT<GameSettingCollection> fGunSpreadSkillMult("fGunSpreadSkillMult", -0.01f);
SettingT<GameSettingCollection> fGunSpreadArmBase("fGunSpreadArmBase", 1.0f);
SettingT<GameSettingCollection> fGunSpreadArmMult("fGunSpreadArmMult", -1.0f);
SettingT<GameSettingCollection> fGunSpreadNPCArmBase("fGunSpreadNPCArmBase", 0.5f);
SettingT<GameSettingCollection> fGunSpreadNPCArmMult("fGunSpreadNPCArmMult", 1.0f);
SettingT<GameSettingCollection> fGunSpreadHeadBase("fGunSpreadHeadBase", 1.0f);
SettingT<GameSettingCollection> fGunSpreadHeadMult("fGunSpreadHeadMult", -1.0f);
SettingT<GameSettingCollection> fGunSpreadDriftBase("fGunSpreadDriftBase", 0.0f);
SettingT<GameSettingCollection> fGunSpreadDriftMult("fGunSpreadDriftMult", 0.1f);
SettingT<GameSettingCollection> fGunSpreadIronSightsBase("fGunSpreadIronSightsBase", 1.0f);
SettingT<GameSettingCollection> fGunSpreadIronSightsMult("fGunSpreadIronSightsMult", -0.65f);
SettingT<GameSettingCollection> fGunSpreadCrouchBase("fGunSpreadCrouchBase", 1.0f);
SettingT<GameSettingCollection> fGunSpreadCrouchMult("fGunSpreadCrouchMult", -0.4f);
SettingT<GameSettingCollection> fGunSpreadWalkBase("fGunSpreadWalkBase", 1.0f);
SettingT<GameSettingCollection> fGunSpreadWalkMult("fGunSpreadWalkMult", 0.0f);
SettingT<GameSettingCollection> fGunSpreadRunBase("fGunSpreadRunBase", 1.0f);
SettingT<GameSettingCollection> fGunSpreadRunMult("fGunSpreadRunMult", 1.0f);
SettingT<GameSettingCollection> fGunShellRotateSpeed("fGunShellRotateSpeed", 300.0f);
SettingT<GameSettingCollection> fGunShellEjectSpeed("fGunShellEjectSpeed", 100.0f);
SettingT<GameSettingCollection> fGunShellLifetime("fGunShellLifetime", 5.0f);
SettingT<GameSettingCollection> fGunShellDirectionRandomize("fGunShellDirectionRandomize", 0.1f);
SettingT<GameSettingCollection> fGunShellRotateRandomize("fGunShellRotateRandomize", 0.1f);
SettingT<GameSettingCollection> fGunShellCameraDistance("fGunShellCameraDistance", 512.0f);
SettingT<GameSettingCollection> fGunDecalCameraDistance("fGunDecalCameraDistance", 2048.0f);
SettingT<GameSettingCollection> fGunParticleCameraDistance("fGunParticleCameraDistance", 1024.0f);
SettingT<GameSettingCollection> fGunReferenceSkill("fGunReferenceSkill", 0.0f);
SettingT<GameSettingCollection> fDebrisFadeTime("fDebrisFadeTime", 1.0f);
SettingT<GameSettingCollection> iDebrisMaxCount("iDebrisMaxCount", 25);
SettingT<GameSettingCollection> fNonAttackGunWobbleMult("fNonAttackGunWobbleMult", 0.25f);
SettingT<GameSettingCollection> fGunWobbleMultScope("fGunWobbleMultScope", 0.2f);
SettingT<GameSettingCollection> fGunWobbleChaseDriftTime("fGunWobbleChaseDriftTime", 0.75f);
SettingT<GameSettingCollection> fGrenadeAgeMax("fGrenadeAgeMax", 90.0f);
SettingT<GameSettingCollection> fMineAgeMax("fMineAgeMax", 0.0f);
SettingT<GameSettingCollection> fArrowSpeedMult("fArrowSpeedMult", 1500.0f);
SettingT<GameSettingCollection> fArrowAgeMax("fArrowAgeMax", 90.0f);
SettingT<GameSettingCollection> fArrowBounceRotateSpeed("fArrowBounceRotateSpeed", 7.5f);
SettingT<GameSettingCollection> fArrowBounceLinearSpeed("fArrowBounceLinearSpeed", 0.1f);
SettingT<GameSettingCollection> fArrowBounceBlockPercentage("fArrowBounceBlockPercentage", 0.95f);
SettingT<GameSettingCollection> fArrowGravityBase("fArrowGravityBase", 0.3f);
SettingT<GameSettingCollection> fArrowGravityMult("fArrowGravityMult", 0.002f);
SettingT<GameSettingCollection> fArrowGravityMin("fArrowGravityMin", 0.05f);
SettingT<GameSettingCollection> fArrowBowTimerBase("fArrowBowTimerBase", 0.25f);
SettingT<GameSettingCollection> fArrowBowTimerMult("fArrowBowTimerMult", 0.4f);
SettingT<GameSettingCollection> fArrowWeakSpeed("fArrowWeakSpeed", 0.01f);
SettingT<GameSettingCollection> fArrowWeakGravity("fArrowWeakGravity", 1.75f);
SettingT<GameSettingCollection> fIronSightsFOVTimeChange("fIronSightsFOVTimeChange", 0.25f);
SettingT<GameSettingCollection> fVATSTargetFOVMultNear("fVATSTargetFOVMultNear", 46.0f);
SettingT<GameSettingCollection> fVATSTargetFOVMultFar("fVATSTargetFOVMultFar", 30.0f);
SettingT<GameSettingCollection> fVATSTargetFOVMultFarDist("fVATSTargetFOVMultFarDist", 1750.0f);
SettingT<GameSettingCollection> fVATSTargetFOVMinDist("fVATSTargetFOVMinDist", 150.0f);
SettingT<GameSettingCollection> fVATSTargetFOVMinFOV("fVATSTargetFOVMinFOV", 1.5f);
SettingT<GameSettingCollection> fVATSTargetRotateMult("fVATSTargetRotateMult", 0.4f);
SettingT<GameSettingCollection> fVATSTargetScanRotateMult("fVATSTargetScanRotateMult", 0.7f);
SettingT<GameSettingCollection> fVATSTargetActorZMultNear("fVATSTargetActorZMultNear", 0.8f);
SettingT<GameSettingCollection> fVATSTargetActorZMultFar("fVATSTargetActorZMultFar", 0.2f);
SettingT<GameSettingCollection> fVATSTargetActorZMultFarDist("fVATSTargetActorZMultFarDist", 175.0f);
SettingT<GameSettingCollection> fVATSTargetActorHeightPanMult("fVATSTargetActorHeightPanMult", 0.2f);
SettingT<GameSettingCollection> fVATSMoveCameraXPercent("fVATSMoveCameraXPercent", 0.0f);
SettingT<GameSettingCollection> fVATSMoveCameraYPercent("fVATSMoveCameraYPercent", 0.15f);
SettingT<GameSettingCollection> fVATSMoveCameraMaxSpeed("fVATSMoveCameraMaxSpeed", 20000.0f);
SettingT<GameSettingCollection> fVATSMoveCameraLimbPercent("fVATSMoveCameraLimbPercent", 0.1f);
SettingT<GameSettingCollection> fVATSMoveCameraLimbMult("fVATSMoveCameraLimbMult", 0.35f);
SettingT<GameSettingCollection> fProjectileMaxDistance("fProjectileMaxDistance", 32768.0f);
SettingT<GameSettingCollection> iAimingNumIterations("iAimingNumIterations", 10);
SettingT<GameSettingCollection> iArrowInventoryChance("iArrowInventoryChance", 50);
SettingT<GameSettingCollection> iArrowMaxRefCount("iArrowMaxRefCount", 15);
SettingT<GameSettingCollection> fArrowFakeMass("fArrowFakeMass", 0.25f);
SettingT<GameSettingCollection> fProjectileCollisionImpulseScale("fProjectileCollisionImpulseScale", 1.0f);
SettingT<GameSettingCollection> fProjectileKnockMultClutter("fProjectileKnockMultClutter", 0.15f);
SettingT<GameSettingCollection> fProjectileKnockMultProp("fProjectileKnockMultProp", 1.0f);
SettingT<GameSettingCollection> fProjectileKnockMultBiped("fProjectileKnockMultBiped", 1.75f);
SettingT<GameSettingCollection> fProjectileKnockMultTrap("fProjectileKnockMultTrap", 0.05f);
SettingT<GameSettingCollection> fProjectileKnockMinMass("fProjectileKnockMinMass", 5.0f);
SettingT<GameSettingCollection> fMineExteriorRadiusMult("fMineExteriorRadiusMult", 1.0f);
SettingT<GameSettingCollection> iMineDisarmExperience("iMineDisarmExperience", 0);
SettingT<GameSettingCollection> fArrowOptimalDistance("fArrowOptimalDistance", 1000.0f);
SettingT<GameSettingCollection> fArrowMaxDistance("fArrowMaxDistance", 2000.0f);
SettingT<GameSettingCollection> fGrenadeRestitution("fGrenadeRestitution", 0.0f);
SettingT<GameSettingCollection> fGrenadeFriction("fGrenadeFriction", 0.0f);
SettingT<GameSettingCollection> fProjectileInventoryGrenadeTimer("fProjectileInventoryGrenadeTimer", 2.0f);
SettingT<GameSettingCollection>
    fProjectileInventoryGrenadeFreakoutTime("fProjectileInventoryGrenadeFreakoutTime", 0.5f);
SettingT<GameSettingCollection> iProjectileMineShooterCanTrigger("iProjectileMineShooterCanTrigger", 0);
SettingT<GameSettingCollection> fRockitDamageBonusWeightMult("fRockitDamageBonusWeightMult", 0.0f);
SettingT<GameSettingCollection> fRockitDamageBonusWeightMin("fRockitDamageBonusWeightMin", 0.0f);
SettingT<GameSettingCollection> fMinesDelayMin("fMinesDelayMin", 0.1f);
SettingT<GameSettingCollection> fMinesBlinkMax("fMinesBlinkMax", 2.0f);
SettingT<GameSettingCollection> fMinesBlinkSlow("fMinesBlinkSlow", 0.25f);
SettingT<GameSettingCollection> fMinesBlinkFast("fMinesBlinkFast", 0.1f);
SettingT<GameSettingCollection> fEnemyHealthBarTimer("fEnemyHealthBarTimer", 5.5f);
SettingT<GameSettingCollection> fCombatMaxHoldScore("fCombatMaxHoldScore", 30.0f);
SettingT<GameSettingCollection> fWeaponClutterKnockMult("fWeaponClutterKnockMult", 750.0f);
SettingT<GameSettingCollection> fWeaponClutterKnockMaxWeaponMass("fWeaponClutterKnockMaxWeaponMass", 5.0f);
SettingT<GameSettingCollection> fWeaponClutterKnockMinClutterMass("fWeaponClutterKnockMinClutterMass", 15.0f);
SettingT<GameSettingCollection> fWeaponClutterKnockBipedScale("fWeaponClutterKnockBipedScale", 5.0f);
SettingT<GameSettingCollection> sBloodParticleDefault("sBloodParticleDefault", "Effects\\BloodSpray.nif");
SettingT<GameSettingCollection> sDismemberParticleDefault("sDismemberParticleDefault", "Gore\\BloodSprayLimb01.nif");
SettingT<GameSettingCollection>
    sDismemberRobotParticleDefault("sDismemberRobotParticleDefault", "Gore\\RobotLimbSpray01.nif");
SettingT<GameSettingCollection> sBloodTextureDefault("sBloodTextureDefault", "Effects\\blooddecal.dds");
SettingT<GameSettingCollection>
    sBloodParticleMeleeDefault("sBloodParticleMeleeDefault", "Effects\\ImpactMeleeBloodSpray01.nif");
SettingT<GameSettingCollection>
    sImpactParticleMetalDefault("sImpactParticleMetalDefault", "Effects\\ImpactBallisticMetal01.nif");
SettingT<GameSettingCollection>
    sImpactParticleConcreteDefault("sImpactParticleConcreteDefault", "Effects\\ImpactBallisticConcrete01.nif");
SettingT<GameSettingCollection>
    sImpactParticleWoodDefault("sImpactParticleWoodDefault", "Effects\\ImpactBallisticWood01.nif");
SettingT<GameSettingCollection> fDeathForceForceMin("fDeathForceForceMin", 35.0f);
SettingT<GameSettingCollection> fDeathForceForceMax("fDeathForceForceMax", 85.0f);
SettingT<GameSettingCollection> fDeathForceDamageMin("fDeathForceDamageMin", 0.0f);
SettingT<GameSettingCollection> fDeathForceDamageMax("fDeathForceDamageMax", 50.0f);
SettingT<GameSettingCollection> fDeathForceRangedForceMin("fDeathForceRangedForceMin", 20.0f);
SettingT<GameSettingCollection> fDeathForceRangedForceMax("fDeathForceRangedForceMax", 40.0f);
SettingT<GameSettingCollection> fDeathForceRangedDamageMin("fDeathForceRangedDamageMin", 0.1f);
SettingT<GameSettingCollection> fDeathForceRangedDamageMax("fDeathForceRangedDamageMax", 10.0f);
SettingT<GameSettingCollection> iAINPCRacePowerChance("iAINPCRacePowerChance", 5);
SettingT<GameSettingCollection> sNormalWeaponsResisted("sNormalWeaponsResisted", "Your attack has no effect.");
SettingT<GameSettingCollection> iCombatHighPriorityModifier("iCombatHighPriorityModifier", 100);
SettingT<GameSettingCollection> fAICombatUnreachableTargetPriorityMult("fAICombatUnreachableTargetPriorityMult", 0.9f);
SettingT<GameSettingCollection> fAICombatNoTargetLOSPriorityMult("fAICombatNoTargetLOSPriorityMult", 1.0f);
SettingT<GameSettingCollection> iPerkAttackDisarmChance("iPerkAttackDisarmChance", 5);
SettingT<GameSettingCollection> iPerkBlockDisarmChance("iPerkBlockDisarmChance", 5);
SettingT<GameSettingCollection> iPerkBlockStaggerChance("iPerkBlockStaggerChance", 5);
SettingT<GameSettingCollection> iPerkMarksmanKnockdownChance("iPerkMarksmanKnockdownChance", 5);
SettingT<GameSettingCollection> iPerkMarksmanParalyzeChance("iPerkMarksmanParalyzeChance", 5);
SettingT<GameSettingCollection> iPerkHandToHandBlockRecoilChance("iPerkHandToHandBlockRecoilChance", 25);
SettingT<GameSettingCollection> fPerkLightArmorMasterRatingMult("fPerkLightArmorMasterRatingMult", 1.5f);
SettingT<GameSettingCollection> iPerkLightArmorMasterMinSum("iPerkLightArmorMasterMinSum", 5);
SettingT<GameSettingCollection> fCombatGiantCreatureReachMult("fCombatGiantCreatureReachMult", 2.0f);
SettingT<GameSettingCollection> fCreatureDefaultTurningSpeed("fCreatureDefaultTurningSpeed", 45.0f);
SettingT<GameSettingCollection> iCombatCastDrainMinimumValue("iCombatCastDrainMinimumValue", 1);
SettingT<GameSettingCollection> fCombatLineOfSightTimer("fCombatLineOfSightTimer", 0.25f);
SettingT<GameSettingCollection> fCombatCollectAlliesTimer("fCombatCollectAlliesTimer", 1.0f);
SettingT<GameSettingCollection> fCombatMonitorBuffsTimer("fCombatMonitorBuffsTimer", 3.0f);
SettingT<GameSettingCollection> fCombatInTheWayTimer("fCombatInTheWayTimer", 1.0f);
SettingT<GameSettingCollection> fCombatBuffStandoffTimer("fCombatBuffStandoffTimer", 3.0f);
SettingT<GameSettingCollection> fCombatBuffMaxTimer("fCombatBuffMaxTimer", 6.0f);
SettingT<GameSettingCollection> fCombatRangedStandoffTimer("fCombatRangedStandoffTimer", 6.0f);
SettingT<GameSettingCollection> iAICombatMaxAllySummonCount("iAICombatMaxAllySummonCount", 3);
SettingT<GameSettingCollection> fAICombatNoAreaEffectAllyDistance("fAICombatNoAreaEffectAllyDistance", 350.0f);
SettingT<GameSettingCollection> fAICombatTargetUnreachablePriorityMult("fAICombatTargetUnreachablePriorityMult", 0.25f);
SettingT<GameSettingCollection> fAIUpdateMovementRestrictionsDistance("fAIUpdateMovementRestrictionsDistance", 48.0f);
SettingT<GameSettingCollection> iAICombatRestoreHealthPercentage("iAICombatRestoreHealthPercentage", 30);
SettingT<GameSettingCollection> iAICombatRestoreMagickaPercentage("iAICombatRestoreMagickaPercentage", 30);
SettingT<GameSettingCollection> iAICombatRestoreFatiguePercentage("iAICombatRestoreFatiguePercentage", 30);
SettingT<GameSettingCollection> fAIMagicTimer("fAIMagicTimer", 3.0f);
SettingT<GameSettingCollection> iAICombatMinDetection("iAICombatMinDetection", -50);
SettingT<GameSettingCollection> sActivateNPCCalmed("sActivateNPCCalmed", "is calmed and cannot respond.");
SettingT<GameSettingCollection>
    sActivateCreatureCalmed("sActivateCreatureCalmed", "The creature is calmed and cannot respond.");
SettingT<GameSettingCollection> fCombatVulnerabilityMod("fCombatVulnerabilityMod", 0.1f);
SettingT<GameSettingCollection> fCombatRelativeDamageMod("fCombatRelativeDamageMod", 100.0f);
SettingT<GameSettingCollection> fCombatDamageScale("fCombatDamageScale", 0.5f);
SettingT<GameSettingCollection> fCombatDistanceMin("fCombatDistanceMin", 400.0f);
SettingT<GameSettingCollection> fCheckPositionFallDistance("fCheckPositionFallDistance", 72.0f);
SettingT<GameSettingCollection> fAICombatSlopeDifference("fAICombatSlopeDifference", 48.0f);
SettingT<GameSettingCollection> fPhysicsDamage1Mass("fPhysicsDamage1Mass", 10.0f);
SettingT<GameSettingCollection> fPhysicsDamage2Mass("fPhysicsDamage2Mass", 50.0f);
SettingT<GameSettingCollection> fPhysicsDamage3Mass("fPhysicsDamage3Mass", 100.0f);
SettingT<GameSettingCollection> fPhysicsDamage1Damage("fPhysicsDamage1Damage", 1.0f);
SettingT<GameSettingCollection> fPhysicsDamage2Damage("fPhysicsDamage2Damage", 5.0f);
SettingT<GameSettingCollection> fPhysicsDamage3Damage("fPhysicsDamage3Damage", 10.0f);
SettingT<GameSettingCollection> fPhysicsDamageSpeedBase("fPhysicsDamageSpeedBase", 1.0f);
SettingT<GameSettingCollection> fPhysicsDamageSpeedMult("fPhysicsDamageSpeedMult", 0.0001f);
SettingT<GameSettingCollection> fPhysicsDamageSpeedMin("fPhysicsDamageSpeedMin", 150.0f);
SettingT<GameSettingCollection> fPhysicsDamage1SpeedMin("fPhysicsDamage1SpeedMin", 500.0f);
SettingT<GameSettingCollection> fPhysicsDamage2SpeedMin("fPhysicsDamage2SpeedMin", 350.0f);
SettingT<GameSettingCollection> fPhysicsDamage3SpeedMin("fPhysicsDamage3SpeedMin", 150.0f);
SettingT<GameSettingCollection> fGetHitPainMult("fGetHitPainMult", 1.5f);
SettingT<GameSettingCollection> fBeamWidthDefault("fBeamWidthDefault", 5.0f);
SettingT<GameSettingCollection> iMessIntactLimbDismemberChance("iMessIntactLimbDismemberChance", 75);
SettingT<GameSettingCollection> iMessIntactLimbExplodeBonus("iMessIntactLimbExplodeBonus", 50);
SettingT<GameSettingCollection> iMessCrippledLimbExplodeBonus("iMessCrippledLimbExplodeBonus", 50);
SettingT<GameSettingCollection> iMessTargetedLimbExplodeBonus("iMessTargetedLimbExplodeBonus", 50);
SettingT<GameSettingCollection> iMessTorsoExplodeChance("iMessTorsoExplodeChance", 50);
SettingT<GameSettingCollection> iCombatCrippledTorsoHitStaggerChance("iCombatCrippledTorsoHitStaggerChance", 50);
SettingT<GameSettingCollection> iDismemberBloodDecalCount("iDismemberBloodDecalCount", 10);
SettingT<GameSettingCollection> fExplodeLimbRemovalDelay("fExplodeLimbRemovalDelay", 0.25f);
SettingT<GameSettingCollection> fExplodeLimbRemovalDelayVATS("fExplodeLimbRemovalDelayVATS", 0.25f);
SettingT<GameSettingCollection> fBloodSplatterDuration("fBloodSplatterDuration", 5.0f);
SettingT<GameSettingCollection> fBloodSplatterFadeStart("fBloodSplatterFadeStart", 0.8f);
SettingT<GameSettingCollection> fBloodSplatterMinSize("fBloodSplatterMinSize", 0.025f);
SettingT<GameSettingCollection> fBloodSplatterMaxSize("fBloodSplatterMaxSize", 0.125f);
SettingT<GameSettingCollection> fBloodSplatterMinOpacity("fBloodSplatterMinOpacity", 0.1f);
SettingT<GameSettingCollection> fBloodSplatterMaxOpacity("fBloodSplatterMaxOpacity", 1.0f);
SettingT<GameSettingCollection> fBloodSplatterMinOpacity2("fBloodSplatterMinOpacity2", 0.1f);
SettingT<GameSettingCollection> fBloodSplatterMaxOpacity2("fBloodSplatterMaxOpacity2", 1.0f);
SettingT<GameSettingCollection> fBloodSplatterOpacityChance("fBloodSplatterOpacityChance", 0.8f);
SettingT<GameSettingCollection> fBloodSplatterFlareOffsetScale("fBloodSplatterFlareOffsetScale", 0.43f);
SettingT<GameSettingCollection> fBloodSplatterFlareSize("fBloodSplatterFlareSize", 0.065f);
SettingT<GameSettingCollection> fBloodSplatterFlareMult("fBloodSplatterFlareMult", 1.0f);
SettingT<GameSettingCollection> iBloodSplatterMaxCount("iBloodSplatterMaxCount", 10);
SettingT<GameSettingCollection>
    sBloodSplatterAlpha01OPTFilename("sBloodSplatterAlpha01OPTFilename", "Textures\\Gore\\ScreenBloodAlpha01OPT.dds");
SettingT<GameSettingCollection>
    sBloodSplatterColor01OPTFilename("sBloodSplatterColor01OPTFilename", "Textures\\Gore\\ScreenBloodColor01OPT.dds");
SettingT<GameSettingCollection>
    sBloodSplatterFlare01Filename("sBloodSplatterFlare01Filename", "Textures\\Effects\\FXBloodFlare.dds");
SettingT<GameSettingCollection> fBloodSplatterCountBase("fBloodSplatterCountBase", 2.0f);
SettingT<GameSettingCollection> fBloodSplatterCountDamageBase("fBloodSplatterCountDamageBase", 0.0f);
SettingT<GameSettingCollection> fBloodSplatterCountDamageMult("fBloodSplatterCountDamageMult", 0.1f);
SettingT<GameSettingCollection> fBloodSplatterCountRandomMargin("fBloodSplatterCountRandomMargin", 1.0f);
SettingT<GameSettingCollection> fCombatEnvironmentBloodChance("fCombatEnvironmentBloodChance", 0.75f);
SettingT<GameSettingCollection> fCombatSneakAttackBonusMult("fCombatSneakAttackBonusMult", 3.0f);
SettingT<GameSettingCollection> fCombatDamageBonusSneakingMult("fCombatDamageBonusSneakingMult", 1.0f);
SettingT<GameSettingCollection> fCombatDamageBonusMeleeSneakingMult("fCombatDamageBonusMeleeSneakingMult", 5.0f);
SettingT<GameSettingCollection> fMinimumFatigue("fMinimumFatigue", -25.0f);
SettingT<GameSettingCollection> iWeaponCriticalHitDropChance("iWeaponCriticalHitDropChance", 100);
SettingT<GameSettingCollection> fCombatPlayerLimbDamageMult("fCombatPlayerLimbDamageMult", 0.5f);
SettingT<GameSettingCollection> fCombatDismemberedLimbVelocity("fCombatDismemberedLimbVelocity", 10.0f);
SettingT<GameSettingCollection> fCombatUnarmedCritDamageMult("fCombatUnarmedCritDamageMult", 0.5f);
SettingT<GameSettingCollection> fThrowingStrengthPenalty("fThrowingStrengthPenalty", 0.05f);
SettingT<GameSettingCollection> fAimSpeedPerkMultiplier("fAimSpeedPerkMultiplier", 1.25f);
SettingT<GameSettingCollection> fAIExplosiveWeaponDamageMult("fAIExplosiveWeaponDamageMult", 1.0f);
SettingT<GameSettingCollection> fAIExplosiveWeaponRangeMult("fAIExplosiveWeaponRangeMult", 1.0f);
SettingT<GameSettingCollection> fDamageArmConditionMult("fDamageArmConditionMult", 0.8f);
SettingT<GameSettingCollection> fDamageArmConditionBase("fDamageArmConditionBase", 0.2f);
SettingT<GameSettingCollection> iMinClipSizeToAddReloadDelay("iMinClipSizeToAddReloadDelay", 2);
SettingT<GameSettingCollection> fUnarmedNPCDPSMult("fUnarmedNPCDPSMult", 1.57f);
SettingT<GameSettingCollection> fUnarmedCreatureDPSMult("fUnarmedCreatureDPSMult", 1.0f);
SettingT<GameSettingCollection> fUnarmedDamageMult("fUnarmedDamageMult", 0.5f);
SettingT<GameSettingCollection> fVATSDamageToWeaponMult("fVATSDamageToWeaponMult", 2.0f);
SettingT<GameSettingCollection> fDamageToWeaponValue("fDamageToWeaponValue", 0.1f);
SettingT<GameSettingCollection> fArmorRatingMax("fArmorRatingMax", 1.0f);
SettingT<GameSettingCollection> fArmorRatingBase("fArmorRatingBase", 0.35f);
SettingT<GameSettingCollection> fArmorRatingMult("fArmorRatingMult", 1.0f);
SettingT<GameSettingCollection> fArmorRatingConditionBase("fArmorRatingConditionBase", 0.0f);
SettingT<GameSettingCollection> fArmorRatingConditionMult("fArmorRatingConditionMult", 1.0f);
SettingT<GameSettingCollection> fKnockdownBaseHealthThreshold("fKnockdownBaseHealthThreshold", 75.0f);
SettingT<GameSettingCollection> fKnockdownCurrentHealthThreshold("fKnockdownCurrentHealthThreshold", 25.0f);
SettingT<GameSettingCollection> fPerkSneakAttackMeleeNoviceMult("fPerkSneakAttackMeleeNoviceMult", 4.0f);
SettingT<GameSettingCollection> fPerkSneakAttackMarksmanNoviceMult("fPerkSneakAttackMarksmanNoviceMult", 6.0f);
SettingT<GameSettingCollection> fPerkSneakAttackMeleeApprenticeMult("fPerkSneakAttackMeleeApprenticeMult", 6.0f);
SettingT<GameSettingCollection> fPerkSneakAttackMarksmanApprenticeMult("fPerkSneakAttackMarksmanApprenticeMult", 8.0f);
SettingT<GameSettingCollection> fPerkSneakAttackMeleeJourneymanMult("fPerkSneakAttackMeleeJourneymanMult", 6.0f);
SettingT<GameSettingCollection> fPerkSneakAttackMarksmanJourneymanMult("fPerkSneakAttackMarksmanJourneymanMult", 8.0f);
SettingT<GameSettingCollection> fPerkSneakAttackMeleeExpertMult("fPerkSneakAttackMeleeExpertMult", 6.0f);
SettingT<GameSettingCollection> fPerkSneakAttackMarksmanExpertMult("fPerkSneakAttackMarksmanExpertMult", 8.0f);
SettingT<GameSettingCollection> fPerkSneakAttackMeleeMasterMult("fPerkSneakAttackMeleeMasterMult", 6.0f);
SettingT<GameSettingCollection> fPerkSneakAttackMarksmanMasterMult("fPerkSneakAttackMarksmanMasterMult", 8.0f);
SettingT<GameSettingCollection> fStandingSpreadPenalty("fStandingSpreadPenalty", 0.1f);
SettingT<GameSettingCollection> fWalkingSpreadPenalty("fWalkingSpreadPenalty", 0.1f);
SettingT<GameSettingCollection> fRunningSpreadPenalty("fRunningSpreadPenalty", 0.2f);
SettingT<GameSettingCollection> fUnaimedSpreadPenalty("fUnaimedSpreadPenalty", 0.2f);
SettingT<GameSettingCollection> fCrippledArm1HSpreadPenalty("fCrippledArm1HSpreadPenalty", 0.2f);
SettingT<GameSettingCollection> fCrippledArm2HSpreadPenalty("fCrippledArm2HSpreadPenalty", 0.4f);
SettingT<GameSettingCollection> fCrippledArms1HSpreadPenalty("fCrippledArms1HSpreadPenalty", 0.4f);
SettingT<GameSettingCollection> fCrippledArms2HSpreadPenalty("fCrippledArms2HSpreadPenalty", 0.6f);
SettingT<GameSettingCollection> fMinGunSpreadValue("fMinGunSpreadValue", 0.01f);
SettingT<GameSettingCollection> fWobbleToSkillConversion("fWobbleToSkillConversion", 0.5f);
SettingT<GameSettingCollection> iCombatDismemberPartChance("iCombatDismemberPartChance", 25);
SettingT<GameSettingCollection> iCombatExplodePartChance("iCombatExplodePartChance", 5);
SettingT<GameSettingCollection> fVATSThrownWeaponRangeMult("fVATSThrownWeaponRangeMult", 2.0f);
SettingT<GameSettingCollection> fVATSMaxChance("fVATSMaxChance", 95.0f);
SettingT<GameSettingCollection> fVATSRangeSpreadMax("fVATSRangeSpreadMax", 2.0f);
SettingT<GameSettingCollection> fVATSHitChanceMult("fVATSHitChanceMult", 1.0f);
SettingT<GameSettingCollection> fVATSSpreadMult("fVATSSpreadMult", 1.0f);
SettingT<GameSettingCollection> fVATSScreenPercentFactor("fVATSScreenPercentFactor", 1.0f);
SettingT<GameSettingCollection> fVATSMeleeChanceMult("fVATSMeleeChanceMult", 1.0f);
SettingT<GameSettingCollection> fVATSSkillFactor("fVATSSkillFactor", 1.0f);
SettingT<GameSettingCollection> fVATSMeleeArmConditionMult("fVATSMeleeArmConditionMult", 0.8f);
SettingT<GameSettingCollection> fVATSMeleeArmConditionBase("fVATSMeleeArmConditionBase", 0.2f);
static SettingT<GameSettingCollection> fVATSCrippledArmThrowingPenatly("fVATSCrippledArmThrowingPenalty", 0.5f);
SettingT<GameSettingCollection> fVATSStealthMult("fVATSStealthMult", 0.5f);
SettingT<GameSettingCollection> fVATSGrenadeTargetArea("fVATSGrenadeTargetArea", 25.0f);
SettingT<GameSettingCollection> fVATSDestructibleMult("fVATSDestructibleMult", 1.25f);
SettingT<GameSettingCollection> fVATSGrenadeChanceMult("fVATSGrenadeChanceMult", 1.0f);
SettingT<GameSettingCollection> fVATSGrenadeRangeMult("fVATSGrenadeRangeMult", 2.0f);
SettingT<GameSettingCollection> fVATSGrenadeRangeMin("fVATSGrenadeRangeMin", 128.0f);
SettingT<GameSettingCollection> fVATSGrenadeSkillFactor("fVATSGrenadeSkillFactor", 0.4f);
SettingT<GameSettingCollection> fCombatGrenadeBounceTimeMin("fCombatGrenadeBounceTimeMin", 0.5f);
SettingT<GameSettingCollection> fCombatGrenadeBounceTimeMax("fCombatGrenadeBounceTimeMax", 5.0f);

double GetWeaponConditionJamChance(int32_t a1) {
    float fresult = 0.0f;
    if (a1 >= 0 && a1 < 10) {
        return fWeaponConditionJam[a1]->Float();
    }
    return fresult;
}

void BSClampUInt32(uint32_t* a1, uint32_t a2, uint32_t a3) {
    if (*a1 <= a3) {
        if (*a1 < a2) {
            *a1 = a2;
        }
    }
    else {
        *a1 = a3;
    }
}

float CombatFormulas::CalcDamageToWeapon(Actor* apActor) {
    float fDamageToWeaponValue = ::fDamageToWeaponValue.Float();
    BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
    TESObjectWEAP* FormFlags = static_cast<TESObjectWEAP*>(CurrentAIProcess->GetCurrentWeapon()->pContainerObj);
    if (FormFlags) {
        if (FormFlags->GetCurrentAmmo(apActor)) {
            TESAmmo* CurrentAmmo = FormFlags->GetCurrentAmmo(apActor);
            BSSimpleList<TESAmmoEffect*>* AmmoEffectList = CurrentAmmo->GetAmmoEffectList();
            return TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_CONDITION, AmmoEffectList, fDamageToWeaponValue);
        }
    }
    return fDamageToWeaponValue;
}

float CombatFormulas::GetArmCondition(ActorValueOwner* apActor, WEAPON_TYPE aeWeaponType) {
    float v5;
    switch (aeWeaponType) {
        case WEAPON_HAND_TO_HAND:
        case WEAPON_TWO_HAND_MELEE:
        case WEAPON_TWO_HAND_RIFLE:
        case WEAPON_TWO_HAND_AUTOMATIC:
        case WEAPON_TWO_HAND_RIFLE_ENERGY:
        case WEAPON_TWO_HAND_HANDLE:
        case WEAPON_TWO_HAND_LAUNCHER: {
            double BodyPartCondition =
                CombatFormulas::GetBodyPartCondition(apActor, ActorValue::LEFT_ATTACK_CONDITION, false);
            v5 = (CombatFormulas::GetBodyPartCondition(apActor, ActorValue::RIGHT_ATTACK_CONDITION, false) +
                  BodyPartCondition) /
                2.0;
            break;
        }
        default: {
            v5 = CombatFormulas::GetBodyPartCondition(apActor, ActorValue::RIGHT_ATTACK_CONDITION, false);
            break;
        }
    }
    return v5;
}

// Intermediate results are kept at the game's 53-bit precision and rounded only where a float is stored; the double
// arithmetic below reproduces each unrounded intermediate, and every (float) is a rounding to float.
float CombatFormulas::GetVATSChanceToHit(
    ActorValueOwner* apActor,
    TESObjectREFR* apTarget,
    TESObjectWEAP* apWeapon,
    float afWeaponCondition,
    float afReach,
    float afGunSkill,
    BGSBodyPart* apTargetedLimb,
    TESObjectWEAP* apTargetedWeapon,
    float afScreenPercentage,
    float afTargetRange,
    float afTargetSize,
    IDebugText* apDebugText
) {
    (void)afWeaponCondition;
    (void)afReach;
    float fFinalChance = 0.0f;
    bool bDestructible;
    if (apTarget) {
        bDestructible = apTarget->GetDestructible() && !apTarget->IsActor();
    }
    else {
        bDestructible = false;
    }
    // IsProjectile is called without a null test on apTarget, as the game does.
    bool bIsProjectile = apTarget->IsProjectile();
    ActorValueOwner* pTargetOwner;
    if (apTarget && apTarget->IsActor()) {
        pTargetOwner = static_cast<Actor*>(apTarget);
    }
    else {
        pTargetOwner = nullptr;
    }

    float fBaseChance = 0.0f;
    if (apTargetedLimb) {
        fBaseChance = (float)(int32_t)apTargetedLimb->GetToHitChance();
    }
    else if (apTargetedWeapon) {
        fBaseChance = (float)(int32_t)apTargetedWeapon->GetVATSToHitChance(false);
    }
    else {
        fBaseChance = 1.0f;
    }
    if (bIsProjectile) {
        fBaseChance = 1.0f;
    }

    if (apDebugText && (bDestructible || bIsProjectile)) {
        const char* pGrenade = bIsProjectile ? "Grenade Target" : "";
        const char* pDestructible = bDestructible ? "Destructible Target " : "";
        apDebugText->PrintLine(DTL_RIGHT, DTJ_RIGHT, "%s%s", pDestructible, pGrenade);
    }

    float fScreenFactor = BSMIN(fVATSScreenPercentFactor.Float(), 1.0f);
    float fScreenMult = (float)((double)fScreenFactor * afScreenPercentage + (1.0 - fScreenFactor));

    if (apWeapon && !apWeapon->IsMeleeWeapon()) {
        if (apWeapon->GetProjectile_(apActor)) {
            BGSProjectile* pProjectile = apWeapon->GetProjectile_(apActor);
            // Out of range unless the proximity equals 0.0 (NaN included).
            if (pProjectile->GetExplosionProximity() != 0.0) {
                if (apDebugText) {
                    apDebugText->PrintLine(DTL_RIGHT, DTJ_RIGHT, "Target out of explosion range");
                }
                return 0.0f;
            }
        }
        if (apWeapon->IsThrownWeapon()) {
            float fMaxRange = apWeapon->GetMaxRange();
            float fThrownMult = fVATSThrownWeaponRangeMult.Float();
            if (afTargetRange > (double)fMaxRange * fThrownMult) {
                if (apDebugText) {
                    apDebugText->PrintLine(DTL_RIGHT, DTJ_RIGHT, "Target out of thrown weapon range");
                }
                return 0.0f;
            }
            float fModMaxRange = (float)((double)fMaxRange * fVATSGrenadeRangeMult.Float());
            float fRangeGap = (float)((double)fModMaxRange - fVATSGrenadeRangeMin.Float());
            float fRangeOver = (float)((double)afTargetRange - fVATSGrenadeRangeMin.Float());
            float fRangeOdds = (float)(1.0 - (double)BSMAX(fRangeOver, 0.0f) / fRangeGap);
            int32_t iWeaponType;
            if (apWeapon) {
                iWeaponType = apWeapon->GetWeaponType();
            }
            else {
                iWeaponType = 0;
            }
            float fArmCondition = GetArmCondition(apActor, static_cast<WEAPON_TYPE>(iWeaponType));
            // True when fArmCondition is greater than 0.0 or NaN.
            if (!(fArmCondition <= 0.0)) {
                fArmCondition = 1.0f;
            }
            else {
                fArmCondition = (float)(1.0 - fCrippledArm1HSpreadPenalty.Float());
            }
            ActorValue::Index iSkill = apWeapon->GetWeaponSkill();
            float fSkillValue = apActor->GetClampedActorFloatValue(iSkill);
            double dSkillPart = (double)fVATSGrenadeSkillFactor.Float() * fSkillValue / 100.0;
            float fSkillCalc = (float)((1.0 - fVATSSkillFactor.Float() + dSkillPart) * fArmCondition);
            float fChanceMult = fVATSGrenadeChanceMult.Float();
            fFinalChance = (float)(((double)fRangeOdds + fSkillCalc) * fChanceMult * 100.0);
            if (apDebugText) {
                apDebugText->PrintLine(
                    DTL_RIGHT, DTJ_RIGHT, "mod max range: %.2f, range gap: %.2f, range odds: %.2f",
                    (double)fModMaxRange, (double)fRangeGap, (double)fRangeOdds
                );
                apDebugText->PrintLine(
                    DTL_RIGHT, DTJ_RIGHT, "arm condition: %.2f, skill calc %.2f, final chance %.2f",
                    (double)fArmCondition, (double)fSkillCalc, (double)fFinalChance
                );
            }
        }
        else {
            if (apWeapon->IsGunWeapon() && apWeapon->GetIsRangeFixed() && afTargetRange > apWeapon->GetMaxRange()) {
                return 0.0f;
            }
            float fSpreadFactor = 1.0f;
            if (apWeapon->IsGunWeapon()) {
                fSpreadFactor = (float)((double)apWeapon->GetMinSpreadDegrees(false) + afGunSkill);
            }
            float fSpreadAngle = (float)((double)fSpreadFactor * fVATSSpreadMult.Float() * 0.01745329238474369);
            float fRadius = (float)((double)std::tan(fSpreadAngle) * afTargetRange);
            float fArea = (float)((double)fRadius * fRadius * 3.1415927410125732);
            float fTargetArea = (float)((double)afTargetSize / 3.0 * afTargetSize);
            if (bIsProjectile) {
                fTargetArea = fVATSGrenadeTargetArea.Float();
            }
            else {
                if (afTargetSize > (double)fRadius + fRadius) {
                    fTargetArea = (float)(((double)fRadius + fRadius) / afTargetSize * fTargetArea);
                }
                if (bDestructible) {
                    fTargetArea = (float)((double)fTargetArea * fVATSDestructibleMult.Float());
                }
            }
            float fRangeSpread = 0.0f;
            if (fArea > 0.0) {
                float fRangeSpreadMax = fVATSRangeSpreadMax.Float();
                float fRatio = (float)((double)fTargetArea / fArea);
                fRangeSpread = BSMIN(fRatio, fRangeSpreadMax);
            }
            float fHitChanceMult = fVATSHitChanceMult.Float();
            fFinalChance = (float)((double)fHitChanceMult * fScreenMult * fRangeSpread * fBaseChance);
            if (apDebugText) {
                apDebugText->PrintLine(
                    DTL_RIGHT, DTJ_RIGHT, "bound size: %.2f, base chance: %.2f", (double)afTargetSize,
                    (double)fBaseChance
                );
                apDebugText->PrintLine(
                    DTL_RIGHT, DTJ_RIGHT, "spread factor: %.2f, radius %.2f, area %.2f", (double)fSpreadFactor,
                    (double)fRadius, (double)fArea
                );
                apDebugText->PrintLine(
                    DTL_RIGHT, DTJ_RIGHT, "target area: %.2f, range spread: %.2f, final chance: %.2f",
                    (double)fTargetArea, (double)fRangeSpread, (double)fFinalChance
                );
            }
        }
        if (bDestructible || bIsProjectile) {
            fFinalChance = (float)((double)fFinalChance * 100.0);
            if (fFinalChance > 0.0 && fFinalChance < 1.0) {
                fFinalChance = 1.0f;
            }
            if (apDebugText) {
                apDebugText->PrintLine(
                    DTL_RIGHT, DTJ_RIGHT, "destructible/grenade target chance: %.2f", (double)fFinalChance
                );
            }
        }
    }
    else {
        // Out of range when the setting is below the range or the comparison is unordered (NaN).
        if (!(fVATSMeleeMaxDistance.Float() >= afTargetRange)) {
            if (apDebugText) {
                apDebugText->PrintLine(DTL_RIGHT, DTJ_RIGHT, "Melee weapon or unarmed -- out of range");
            }
        }
        else {
            if (apDebugText) {
                apDebugText->PrintLine(DTL_RIGHT, DTJ_RIGHT, "Melee weapon or unarmed assume 100%");
            }
            fFinalChance = 100.0f;
        }
    }

    if (pTargetOwner) {
        if (pTargetOwner->GetActorValue(ActorValue::CHAMELEON) > 0 ||
            apActor->GetActorValue(ActorValue::INVISIBILITY) > 0) {
            if (apDebugText) {
                float fStealthMult = fVATSStealthMult.Float();
                apDebugText->PrintLine(
                    DTL_RIGHT, DTJ_RIGHT, "Stealth bonus: %f becomes %f", (double)fFinalChance,
                    (double)fFinalChance * fStealthMult
                );
            }
            float fDetectionBoost = 0.0f;
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_IMPROVED_DETECTION, pPlayer, &fDetectionBoost);
            if (fDetectionBoost <= 0.0) {
                fFinalChance = (float)((double)fFinalChance * fVATSStealthMult.Float());
            }
        }
    }
    float fMaxChance = fVATSMaxChance.Float();
    float fChance = BSMIN(fFinalChance, fMaxChance);
    return (float)((double)fChance / 100.0);
}

float CombatFormulas::GetGunSpread(
    Actor* apActor,
    TESObjectWEAP* apWeapon,
    float afDrift,
    bool abIronSights,
    bool abCrouching,
    bool abWalking,
    bool abRunning
) {
    ActorValue::Index iweaponSkill;
    float apValue;
    apValue = 1.0f;
    float fretVal = 0.0f;
    if (apActor == pPlayer) {
        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::AIM_MOVEMENT_MODIFIER, pPlayer, apWeapon, &apValue);
        if (apValue < 1.0) {
            apValue = 1.0f;
        }
    }
    if (!abCrouching) {
        float* v7 = &fStandingSpreadPenalty.Float();
        fretVal = fretVal + *v7;
    }
    if (abWalking) {
        float* v8 = &fWalkingSpreadPenalty.Float();
        fretVal = *v8 / apValue + fretVal;
    }
    else {
        if (abRunning) {
            float* v9 = &fRunningSpreadPenalty.Float();
            fretVal = *v9 / apValue + fretVal;
        }
    }
    if (!abIronSights) {
        float* v10 = &fUnaimedSpreadPenalty.Float();
        fretVal = fretVal + *v10;
    }
    float factorLeftArmCondition =
        CombatFormulas::GetBodyPartCondition(apActor, ActorValue::LEFT_ATTACK_CONDITION, false);
    float factorRightArmCondition =
        CombatFormulas::GetBodyPartCondition(apActor, ActorValue::RIGHT_ATTACK_CONDITION, false);
    // The head condition is fetched and never read, as the game does.
    float factorHeadCondition = CombatFormulas::GetBodyPartCondition(apActor, ActorValue::PERCEPTION_CONDITION, false);
    (void)factorHeadCondition;
    if (!apActor->GetActorValue(ActorValue::IGNORE_CRIPPLED_LIMBS)) {
        if (factorLeftArmCondition == 0.0 && factorRightArmCondition == 0.0) {
            float* v14;
            if (apWeapon->IsTwoHandedWeapon()) {
                v14 = &fCrippledArms2HSpreadPenalty.Float();
            }
            else {
                v14 = &fCrippledArms1HSpreadPenalty.Float();
            }
            fretVal = fretVal + *v14;
        }
        else if (factorLeftArmCondition == 0.0 || factorRightArmCondition == 0.0) {
            if (apWeapon->IsTwoHandedWeapon()) {
                float* v15 = &fCrippledArm2HSpreadPenalty.Float();
                fretVal = fretVal + *v15;
            }
            else if (factorRightArmCondition == 0.0) {
                float* v16 = &fCrippledArm1HSpreadPenalty.Float();
                fretVal = fretVal + *v16;
            }
        }
    }
    // GetClampedActorFloatValue's result is truncated directly to an integer.
    uint32_t iSTR = static_cast<int64_t>(apActor->GetClampedActorFloatValue(ActorValue::STRENGTH));
    float fSTRBonus = 0.0f;
    Actor* pactor = apActor;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::WEAPON_HANDLING_STRENGTH_BONUS, pactor, apWeapon, &fSTRBonus);
    uint32_t iSTRMOD = static_cast<int64_t>(fSTRBonus);
    uint32_t iSTRreq = apWeapon->GetStrRequired();
    float fMod = CombatFormulas::GetWeaponStrengthRequirementModifier(iSTRreq, iSTR, iSTRMOD);
    fretVal = fretVal + fMod;
    if (apWeapon) {
        iweaponSkill = apWeapon->GetWeaponSkill();
    }
    else {
        iweaponSkill = ActorValue::UNARMED;
    }
    float factorSkill = static_cast<float>(apActor->GetClampedActorValue(iweaponSkill));
    uint32_t iSkill = static_cast<int64_t>(factorSkill);
    uint32_t iSKILLreq = apWeapon->data.iSkillRequirement;
    float fModSkill = CombatFormulas::GetWeaponSkillRequirementModifier(iSKILLreq, iSkill, 0);
    fretVal = fretVal + fModSkill;
    float fskillMult = 1.0 - factorSkill * fWobbleToSkillConversion.Float() * 0.009999999776482582;
    float fretVal_2 = fretVal * fskillMult;
    float* v17 = &fMinGunSpreadValue.Float();
    if (*v17 > static_cast<double>(fretVal_2)) {
        return fMinGunSpreadValue.Float();
    }
    return fretVal_2;
}

float CombatFormulas::GetArrowGravity(float afArrowPower, float afMarksmanSkill, int32_t aiLuck) {
    float* v3 = &fArrowGravityBase.Float();
    float v6 = *v3 - afMarksmanSkill * fArrowGravityMult.Float();
    float v7 = fArrowWeakGravity.Float() * (1.0 - afArrowPower) + afArrowPower * v6;
    if (v7 < 0.0) {
        return 0.0;
    }
    return v7;
}

float CombatFormulas::CalcArmorRating(uint16_t asArmorRating, float afcondition) {
    float v4 = ApplyArmorConditionScaling(afcondition);
    return (float)(((double)asArmorRating) * v4);
}

float CombatFormulas::CalcWeaponDamageForDisplay(
    TESObjectWEAP* apWeapon,
    float afCondition,
    ItemChange* apItem,
    TESAmmo* apLoadedAmmo
) {
    ActorValueOwner* avOwner;
    float fresult;
    fresult = 0.0f;
    if (pPlayer) {
        avOwner = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
    }
    else {
        avOwner = nullptr;
    }
    fresult = CombatUtilities::CalculateItemDPS(
        avOwner, apWeapon, afCondition, true, apItem, false, false, -1, 0.0f, 0.0f, false, false, apLoadedAmmo
    );
    if (apWeapon && (apWeapon->IsAutomaticWeapon() && apWeapon->IsMeleeWeapon())) {
        ActorValueOwner* apActor;
        apWeapon->SetAutomaticWeapon(false);
        if (pPlayer) {
            apActor = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
        }
        else {
            apActor = nullptr;
        }
        fresult = CombatFormulas::CalcWeaponDamage(
            apActor, apWeapon, afCondition, 1.0f, false, false, apItem, apLoadedAmmo != nullptr
        );
        apWeapon->SetAutomaticWeapon(true);
    }
    else {
        if (pPlayer) {
            fresult = CombatFormulas::CalcWeaponDamage(
                &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer))), apWeapon,
                afCondition, 1.0f, false, false, apItem, apLoadedAmmo != nullptr
            );
        }
        else {
            fresult = CombatFormulas::CalcWeaponDamage(
                nullptr, apWeapon, afCondition, 1.0f, false, false, apItem, apLoadedAmmo != nullptr
            );
        }
    }
    TESAmmo* v11 = nullptr;
    if (apWeapon) {
        TESAmmo* CurrentAmmo;
        if (apLoadedAmmo) {
            CurrentAmmo = apLoadedAmmo;
        }
        else {
            CurrentAmmo = apWeapon->GetCurrentAmmo(pPlayer);
        }
        v11 = CurrentAmmo;
    }
    if (!v11) {
        v11 = apWeapon->GetCurrentAmmo(nullptr);
    }
    if (v11) {
        if (apWeapon) {
            // A thrown weapon is its own ammo: the forms are compared as TESForm pointers.
            if (static_cast<TESForm*>(v11) != static_cast<TESForm*>(apWeapon)) {
                BSSimpleList<TESAmmoEffect*>* AmmoEffectList = v11->GetAmmoEffectList();
                fresult = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DAM, AmmoEffectList, fresult);
            }
        }
    }
    if (apItem) {
        if (apItem->HasModEffectActive(12)) {
            fresult = fresult * 1.3;
        }
    }
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::CALCULATE_WEAPON_DAMAGE, pPlayer, apWeapon, pPlayer, &fresult);
    return fresult;
}

float CombatFormulas::CalcCombatWeaponDPS(
    TESForm* apActorForm,
    TESObjectWEAP* apWeapon,
    float afCondition,
    TESCombatStyle* apCombatStyle,
    ItemChange* apItem,
    TESAmmo* apAmmo
) {
    if (apWeapon && apWeapon->GetPreventUseInNormalCombat()) {
        return 0.0f;
    }
    else {
        COMBAT_WEAPON_TYPE ecombatWeaponType;
        float fdamagePerSecond;
        if (apActorForm->GetFormType() == CREA_ID || apActorForm->GetFormType() == NPC__ID) {
            TESActorBase* pActorBase = static_cast<TESActorBase*>(apActorForm);
            fdamagePerSecond = CombatUtilities::CalculateItemDPS(
                pActorBase, apWeapon, afCondition, false, apItem, false, false, -1, 0.0f, 0.0f, false, false, apAmmo
            );
            if (!apCombatStyle) {
                apCombatStyle = pActorBase->GetCombatStyle();
            }
        }
        else {
            Actor* pActor = static_cast<Actor*>(apActorForm);
            fdamagePerSecond = CombatUtilities::CalculateItemDPS(
                pActor, apWeapon, afCondition, false, apItem, false, false, -1, 0.0f, 0.0f, false, false, apAmmo
            );
            if (!apCombatStyle) {
                apCombatStyle = pActor->Actor::GetCombatStyle();
            }
        }
        if (apWeapon) {
            ecombatWeaponType = apWeapon->GetCombatWeaponType();
        }
        else {
            ecombatWeaponType = COMBAT_WEAPON_TYPE_COUNT;
        }
        if (ecombatWeaponType == COMBAT_WEAPON_GRENADE) {
            fdamagePerSecond = fdamagePerSecond / 10.0;
        }
        else {
            if (ecombatWeaponType == COMBAT_WEAPON_MINE) {
                fdamagePerSecond = 0.0f;
            }
        }
        if (apCombatStyle) {
            int8_t cWeaponRestrictions = apCombatStyle->SimpleData.cWeaponRestrictions;
            if (cWeaponRestrictions == 1 && (apWeapon && apWeapon->IsRangedWeapon())) {
                return fdamagePerSecond / 10000.0;
            }
            else {
                if (cWeaponRestrictions == 2 && (!apWeapon || !apWeapon->IsRangedWeapon())) {
                    return fdamagePerSecond / 10000.0;
                }
            }
        }
        return fdamagePerSecond;
    }
}

float CombatFormulas::CalcWeaponDamage(
    ActorValueOwner* apActor,
    TESObjectWEAP* apWeapon,
    float afCondition,
    float afDamageMultiplier,
    bool abIgnoreFatigue,
    bool abIgnoreExplosion,
    ItemChange* apItem,
    bool abUseCurrentAmmo
) {
    double v11;
    float fskill;
    float fAttackDamage;
    ActorValue::Index iskill;
    float fdamageBonus;
    if (apWeapon) {
        iskill = apWeapon->GetWeaponSkill();
    }
    else {
        iskill = ActorValue::UNARMED;
    }
    if (apWeapon) {
        fAttackDamage = static_cast<float>(apWeapon->GetAttackDamage());
    }
    else {
        fAttackDamage = 1.0f;
    }
    float fWeaponDamage = fDamageWeaponMult.Float() * fAttackDamage;
    fdamageBonus = 0.0f;
    if (apItem) {
        apItem->HasModEffectActive(WEAPON_MOD_INCREASE_WEAPON_DAMAGE, fdamageBonus);
    }
    if (apWeapon) {
        switch (apWeapon->GetWeaponType()) {
            case WEAPON_NONE:
            case WEAPON_HAND_TO_HAND: {
                if (apActor) {
                    double v9 = apActor->GetActorFloatValue(ActorValue::UNARMED_DAMAGE);
                    fdamageBonus = v9 + fdamageBonus;
                }
                break;
            }
            case WEAPON_ONE_HAND_MELEE:
            case WEAPON_TWO_HAND_MELEE:
            case WEAPON_ONE_HAND_THROWN: {
                if (apActor) {
                    double v8 = apActor->GetActorFloatValue(ActorValue::MELEE_DAMAGE);
                    fdamageBonus = v8 + fdamageBonus;
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    else {
        if (apActor) {
            double v10 = apActor->GetActorFloatValue(ActorValue::UNARMED_DAMAGE);
            fdamageBonus = v10 + fdamageBonus;
        }
    }
    v11 = 1.0;
    float fmeleeConditionSkillMult = 1.0f;
    if (apActor) {
        if (!apWeapon || apWeapon->IsMeleeWeapon()) {
            WEAPON_TYPE aeWeaponType;
            if (apWeapon) {
                aeWeaponType = apWeapon->GetWeaponType();
            }
            else {
                aeWeaponType = WEAPON_HAND_TO_HAND;
            }
            double ArmCondition = CombatFormulas::GetArmCondition(apActor, aeWeaponType);
            double v23 = fDamageArmConditionMult.Float() * ArmCondition;
            v11 = fDamageArmConditionBase.Float() + v23;
            fmeleeConditionSkillMult = v11;
        }
    }
    if (apActor) {
        fskill = apActor->GetClampedActorFloatValue(iskill);
    }
    else {
        fskill = 100.0f;
    }
    float fdamageSkill = fmeleeConditionSkillMult;
    if (apWeapon) {
        if (apWeapon->GetWeaponType()) {
            float* v12 = &fDamageSkillBase.Float();
            float* v13 = &fDamageSkillMult.Float();
            fdamageSkill = fskill * 0.009999999776482582 * *v13 * fmeleeConditionSkillMult + *v12;
        }
    }
    float fdamageMult = fdamageSkill * afDamageMultiplier;
    float v31 = 0.0f;
    if (!abIgnoreExplosion) {
        if (apWeapon) {
            if (apWeapon->GetProjectile_(abUseCurrentAmmo ? apActor : nullptr)) {
                if (apWeapon->GetProjectile_(abUseCurrentAmmo ? apActor : nullptr)->GetExplosion()) {
                    bool HasModEffectActive_0;
                    Actor* apWeaponHolder;
                    if (abUseCurrentAmmo) {
                        apWeaponHolder = dynamic_cast<Actor*>(apActor);
                    }
                    else {
                        apWeaponHolder = nullptr;
                    }
                    if (apItem) {
                        HasModEffectActive_0 = apItem->HasModEffectActive(12);
                    }
                    else {
                        HasModEffectActive_0 = false;
                    }
                    BGSProjectile* Projectile_ = apWeapon->GetProjectile_(abUseCurrentAmmo ? apActor : nullptr);
                    double v19 = CombatFormulas::CalcExplosionDamage(Projectile_->GetExplosion());
                    v31 =
                        static_cast<double>(apWeapon->GetNumProjectiles(HasModEffectActive_0, false, apWeaponHolder)) *
                        v19;
                }
            }
        }
    }
    float ftimeMult = 1.0f;
    float v33 = 1.0f;
    float fconditionMod = CombatFormulas::ApplyWeaponConditionScaling(afCondition);
    float fNerveBonus = 1.0f;
    Actor* pActorReal = dynamic_cast<Actor*>(apActor);
    if (pActorReal) {
        if (pActorReal->bPlayerTeammate) {
            float afCharisma = pPlayer->GetClampedActorFloatValue(ActorValue::CHARISMA);
            fNerveBonus = afCharisma * 0.05000000074505806 + 1.0;
        }
    }
    if (apWeapon) {
        if (apWeapon->IsAutomaticWeapon() && apWeapon->IsMeleeWeapon() &&
            (!pVATS.IsInVats() || pActorReal != pPlayer)) {
            ftimeMult = appTimer.fDelta;
        }
    }
    return ((fWeaponDamage + v31) * fdamageMult + fdamageBonus) * fconditionMod * ftimeMult * fNerveBonus;
}

// Scales the stored health damage by the double 0.5.
void CombatFormulas::CalcHandToHandDamage(
    ActorValueOwner* apActor,
    bool abOpponentKnockedOut,
    float& afHealthDamage,
    float& afFatigueDamage
) {
    afHealthDamage = CalcWeaponDamage(apActor, nullptr, 1.0f, 1.0f, false, false, nullptr, false);
    if (!abOpponentKnockedOut) {
        afFatigueDamage = float(afHealthDamage * 0.5);
    }
}

float CombatFormulas::GetPowerAttackDamageMultiplier(int32_t aiWeaponSkill, ANIM_GROUP_ENUM aePowerAttackGroup) {
    return fDamagePowerAttackBonus.Float();
}

float CombatFormulas::ChooseMeleeWeapon(float afWeaponDamage, int32_t aiArmorRating, bool abHandToHand) {
    float v5;
    if (abHandToHand) {
        v5 = (&fAIMeleeHandMult)->Float();
    }
    else {
        v5 = (&::fAIMeleeWeaponMult)->Float();
    }
    return (afWeaponDamage * v5);
}

float CombatFormulas::CalcWeaponReach(float afReachMultiplier) {
    return (afReachMultiplier * (&fCombatDistance)->Float());
}

float CombatFormulas::CalcExplosionPowerDistance(float afExplosionRadius, float afPower) {
    float fInvDamagerRatio = (float)(1.0 - afPower);
    if (fInvDamagerRatio < 0.0) {
        return 0.0;
    }
    return (NiSqrt(fInvDamagerRatio) * afExplosionRadius);
}

float CombatFormulas::CalcCreatureDamage(float afFatiguePercent, int32_t aiBaseDamage) {
    return (float)aiBaseDamage;
}

// Each setting is read into a float local. The x87 code subtracts and divides at double precision and rounds the
// ratio to float once; the clamp is BSMAX(ratio, 0) then BSMIN(1, max), and the result is rounded to float before
// the return.
float CombatFormulas::GetDeathForce(float afDamage, bool abRanged) {
    float fDamageMin = abRanged ? fDeathForceRangedDamageMin.Float() : fDeathForceDamageMin.Float();
    float fDamageMax = abRanged ? fDeathForceRangedDamageMax.Float() : fDeathForceDamageMax.Float();
    float fForceMin = abRanged ? fDeathForceRangedForceMin.Float() : fDeathForceForceMin.Float();
    float fForceMax = abRanged ? fDeathForceRangedForceMax.Float() : fDeathForceForceMax.Float();
    float fRatio = std::fabs(afDamage);
    fRatio =
        static_cast<float>((static_cast<double>(fRatio) - fDamageMin) / (static_cast<double>(fDamageMax) - fDamageMin));
    fRatio = BSMIN(1.0f, BSMAX(fRatio, 0.0f));
    return static_cast<float>((static_cast<double>(fForceMax) - fForceMin) * fRatio + fForceMin);
}

float CombatFormulas::CalcExplosionPower(float afExplosionRadius, float afDistance) {
    float v3 = (float)0.0;
    if (!(afDistance <= ((double)afExplosionRadius))) {
        return v3;
    }
    float v4 = afDistance / afExplosionRadius;
    float v5 = v4 * v4;
    return (float)(1.0 - v5);
}

float CombatFormulas::CalcExplosionDamageDistance(
    float afExplosionRadius,
    float afBaseExplosionDamage,
    float afActualDamage
) {
    float fDamageRatio = afActualDamage / afBaseExplosionDamage;
    if (!(fDamageRatio > 1.0)) {
        return CombatFormulas::CalcExplosionPowerDistance(afExplosionRadius, fDamageRatio);
    }
    else {
        return 0.0;
    }
}

float CombatFormulas::CalcDamageToArmor(float afHealthDamage, float afArmorRatingPercent) {
    return ((&fDamageToArmorPercentage)->Float() * (afHealthDamage * afArmorRatingPercent));
}

// Each sum is computed in double precision and only the named values are stored as floats, as the game does.
int32_t CombatFormulas::GetScreenSplatterCount(float fDamage) {
    float fCountBase = fBloodSplatterCountBase.Float();
    float* pDamageBase = &::fBloodSplatterCountDamageBase.Float();
    float fDamageValue = (float)((double)fDamage * ::fBloodSplatterCountDamageMult.Float() + *pDamageBase);
    float fMarginPos = ::fBloodSplatterCountRandomMargin.Float();
    float fRandom = RandomFloat(-::fBloodSplatterCountRandomMargin.Float(), fMarginPos);
    return BSMAX(0, (int32_t)((double)fCountBase + fDamageValue + fRandom));
}

// The condition is kept in a float: 1 when the actor value is outside the limb conditions, else the value capped at
// 100, and when abRaw is false, 1 when the value compares greater than 0.0 (NaN gives 0) and 0 otherwise.
float CombatFormulas::GetBodyPartCondition(ActorValueOwner* apActor, ActorValue::Index aiWhich, bool abRaw) {
    float fCondition = 1.0f;
    if (aiWhich < ActorValue::PERCEPTION_CONDITION || aiWhich > ActorValue::BRAIN_CONDITION) {
        return fCondition;
    }
    float fValue = apActor->GetActorFloatValue(aiWhich);
    fCondition = BSMIN(100.0f, fValue);
    if (abRaw) {
        return fCondition;
    }
    if (fCondition > 0.0) {
        return 1.0f;
    }
    return 0.0f;
}

float CombatFormulas::GetKnockbackSpeed(ActorValueOwner* apActor, int32_t aiDamage) {
    float v7 = (float)(apActor->GetClampedActorFloatValue(ActorValue::AGILITY) * 10.0);
    float* v2 = &(&fKnockbackAgilBase)->Float();
    float v9 = ((&::fKnockbackAgilMult)->Float() * v7) + (*v2);
    float* v3 = &(&::fKnockbackDamageBase)->Float();
    float v8 = (float)((((double)aiDamage) * (&::fKnockbackDamageMult)->Float()) + (*v3));
    return (v9 * v8);
}

float CombatFormulas::CalcBlock(int32_t aiSkill) {
    float v4 = (float)(double)aiSkill;
    float* v1 = &(&fBlockSkillBase)->Float();
    return (((&::fBlockSkillMult)->Float() * v4) + (*v1));
}

// Takes the bound when the two are equal or unordered.
float CombatFormulas::GetRandomBetween(float afMin, float afMax) {
    float fMin = BSMIN(afMin, afMax);
    float fMax = BSMAX(afMin, afMax);
    if (!(fMin != fMax)) {
        return fMin;
    }
    return RandomFloat(fMin, fMax);
}

float CombatFormulas::ChooseAttack(
    TESCombatStyle* apCombatStyle,
    int32_t aiSkill,
    int32_t aiLuck,
    bool abTargetAttacking,
    bool abHandToHand,
    bool abTargetRecoiling,
    bool abTargetUnconscious,
    bool abTargetBlocking
) {
    float maxFarNearRatio;
    float weatherPercent;
    float unk88;
    float rockSpeed;
    float altTimer;
    if (abTargetRecoiling) {
        altTimer = apCombatStyle->GetAttackDuringRecoilStaggerBonus();
    }
    else {
        altTimer = (float)0.0;
    }
    if (abTargetUnconscious) {
        rockSpeed = apCombatStyle->GetAttackDuringUnconsciousBonus();
    }
    else {
        rockSpeed = (float)0.0;
    }
    if (abHandToHand) {
        unk88 = apCombatStyle->GetAttackHandBonus();
    }
    else {
        unk88 = (float)0.0;
    }
    double attackSkillModBase = (double)apCombatStyle->GetAttackSkillModBase();
    float v20 = (float)(((double)aiSkill) / 100.0);
    float v16 = (float)((apCombatStyle->GetAttackSkillMult() * v20) + attackSkillModBase);
    float v17 =
        (float)((((((double)((int8_t)apCombatStyle->GetAttackChance())) + altTimer) + rockSpeed) + unk88) + v16);
    if (abTargetAttacking) {
        weatherPercent = apCombatStyle->GetAttackDuringAttackMult();
    }
    else {
        weatherPercent = apCombatStyle->GetAttackNoAttackMult();
    }
    float v18 = v17 * weatherPercent;
    if (abTargetBlocking) {
        maxFarNearRatio = apCombatStyle->GetAttackDuringBlockMult();
    }
    else {
        maxFarNearRatio = (float)1.0;
    }
    return (v18 * maxFarNearRatio);
}

float CombatFormulas::ChooseBlock(
    TESCombatStyle* apCombatStyle,
    int32_t aiSkill,
    int32_t aiLuck,
    bool abHasShield,
    bool abTargetAttacking
) {
    float v8;
    float explosionTimer;
    float v15 = (float)(((double)aiSkill) / 100.0);
    double isDebugTextVisible = (double)apCombatStyle->GetBlockChance();
    double v11 = apCombatStyle->GetBlockSkillBase() + isDebugTextVisible;
    float v13 = (float)((apCombatStyle->GetBlockSkillMult() * v15) + v11);
    if (abTargetAttacking) {
        explosionTimer = apCombatStyle->GetBlockDuringAttackMult();
    }
    else {
        explosionTimer = apCombatStyle->GetBlockNoAttackMult();
    }
    float v14 = v13 * explosionTimer;
    if (abHasShield) {
        return v14;
    }
    if (abTargetAttacking) {
        v8 = (float)1.0;
    }
    else {
        v8 = 0x1p-2f;
    }
    return ((v14 * (&::fBlockScoreNoShieldMult)->Float()) * v8);
}

float CombatFormulas::ChooseDodge(
    TESCombatStyle* apCombatStyle,
    int32_t aiWalkSpeed,
    float afFatiguePercent,
    bool abTargetAttacking
) {
    float dodgeDuringAttackMult;
    double v7 = apCombatStyle->GetEncumSpeedModMult() * ((double)aiWalkSpeed);
    float v8 = (float)(apCombatStyle->GetDodgeSpeedBase() + v7);
    double v6 = apCombatStyle->GetDodgeFatigueMult() * (1.0 - afFatiguePercent);
    float v11 = (float)(apCombatStyle->GetDodgeFatigueBase() + v6);
    float v9 = (float)((((double)((int8_t)apCombatStyle->GetDodgeChance())) + v8) + v11);
    if (abTargetAttacking) {
        dodgeDuringAttackMult = apCombatStyle->GetDodgeDuringAttackMult();
    }
    else {
        dodgeDuringAttackMult = apCombatStyle->GetDodgeNoAttackMult();
    }
    return (v9 * dodgeDuringAttackMult);
}

float CombatFormulas::GetWeaponConditionReloadJamMult(int32_t aiCondition) {
    float v3 = (float)0.0;
    if (((uint32_t)aiCondition) <= 9) {
        return fWeaponConditionReloadJam[aiCondition]->Float();
    }
    return v3;
}

float CombatFormulas::ApplyArmorConditionScaling(float fArmorHealthPercent) {
    if (!(fArmorHealthPercent > 0.5)) {
        return (float)(1.0 - ((0.5 - fArmorHealthPercent) * 1.0));
    }
    else {
        return (float)1.0;
    }
}

float CombatFormulas::CalcTargetedLimbDamage(ActorValueOwner* apTarget, BGSBodyPart* apBodyPart, float afHealthDamage) {
    float v5;
    float v6;
    float v7 = (float)0.0;
    if (apTarget) {
        if (apBodyPart) {
            v6 = (float)(((double)apBodyPart->GetHealthPercent()) * 0.009999999776482582);
            v5 = apTarget->GetBaseActorFloatValue(ActorValue::HEALTH);
            if (!(v5 <= 0.0)) {
                if (v6 > 0.0) {
                    return (float)((afHealthDamage / (v6 * v5)) * 100.0);
                }
            }
            else {
                return afHealthDamage;
            }
        }
    }
    return v7;
}

float CombatFormulas::GetCriticalChance(ActorValueOwner* apActor, TESObjectWEAP* apWeapon) {
    float v4;
    if (apWeapon) {}
    float TurnSpeed = 1.0;
    if (apWeapon) {
        if (apWeapon->IsAutomaticWeapon()) {
            TurnSpeed = apWeapon->GetShotsPerSecond();
        }
    }
    if (TurnSpeed == 0.0) {
        TurnSpeed = 1.0;
    }
    if (apActor) {
        v4 = apActor->GetActorFloatValue(ActorValue::CRITICAL_CHANCE);
    }
    else {
        v4 = 0.0;
    }
    float v6 = v4 / TurnSpeed;
    if (apWeapon) {
        if (apWeapon->GetCriticalChanceMult() >= 0.0) {
            return apWeapon->GetCriticalChanceMult() * v6;
        }
    }
    return v6;
}

bool CombatFormulas::ChoosePowerAttack(
    TESCombatStyle* apCombatStyle,
    float afFatiguePercent,
    bool abTargetRecoiling,
    bool abTargetUnconscious,
    bool abEncumbered
) {
    float fUnconsciousBonus;
    float fRecoilBonus;
    if (abEncumbered) {
        return false;
    }
    float v9 = static_cast<float>(apCombatStyle->QPowerAttackChance());
    if (v9 == 0.0) {
        return false;
    }
    if (v9 >= 100.0) {
        return true;
    }
    if (abTargetRecoiling) {
        fRecoilBonus = apCombatStyle->Data.fPowerAttackRecoilStaggerBonus;
    }
    else {
        fRecoilBonus = 0.0f;
    }
    if (abTargetUnconscious) {
        fUnconsciousBonus = apCombatStyle->Data.fPowerAttackUnconsciousBonus;
    }
    else {
        fUnconsciousBonus = 0.0f;
    }
    double fFatigueBase = apCombatStyle->AdvancedData.fPowerAttackFatigueBase;
    float v12 = apCombatStyle->AdvancedData.fPowerAttackFatigueMult * (1.0 - afFatiguePercent) + fFatigueBase;
    float v10 = fRecoilBonus + fUnconsciousBonus + v12 + v9;
    float v11 = static_cast<float>(Random() % 100);
    return v10 >= static_cast<double>(v11);
}

uint16_t CombatFormulas::SelectDodgeDirection(
    TESCombatStyle* apCombatStyle,
    bool abDodgerAttacking,
    bool abTargetAttacking,
    float fTargetDistance,
    float fWeaponReach,
    bool abFavorLeftRight,
    bool abTargetUsingRanged,
    uint16_t asMovementRestrictions
) {
    float v10;
    float Range;
    float DefaultWaterHeight;
    uint16_t v18 = 0;
    if (abTargetAttacking) {
        DefaultWaterHeight = apCombatStyle->AdvancedData.fDodgeBackDuringAttackMult;
    }
    else {
        DefaultWaterHeight = apCombatStyle->AdvancedData.fDodgeBackNoAttackMult;
    }
    double v13 = fAIDodgeDecisionBase.Float() * DefaultWaterHeight;
    float v20 = static_cast<double>(Random() % 5 + 1) * v13;
    if (abDodgerAttacking) {
        Range = apCombatStyle->AdvancedData.fDodgeForwardWhileAttackingMult;
    }
    else {
        Range = apCombatStyle->AdvancedData.fDodgeForwardNotAttackingMult;
    }
    double v11 = fAIDodgeDecisionBase.Float() * Range;
    float v17 = static_cast<double>(Random() % 5 + 1) * v11;
    if ((asMovementRestrictions & 2) != 0 || abTargetUsingRanged) {
        v20 = 0.0f;
    }
    if ((asMovementRestrictions & 1) != 0) {
        v17 = 0.0f;
    }
    if (abFavorLeftRight) {
        v10 = fAIDodgeFavorLeftRightMult.Float();
    }
    else {
        v10 = 1.0f;
    }
    double v9 = fAIDodgeDecisionBase.Float() * v10;
    float v19 = static_cast<double>(Random() % 5 + 1) * v9;
    if ((v17 <= static_cast<double>(v20) && v19 < static_cast<double>(v20)) || fWeaponReach * 0.5 > fTargetDistance) {
        return 2;
    }
    if (v20 < static_cast<double>(v17) && (v19 < static_cast<double>(v17) && fWeaponReach * 1.5 < fTargetDistance)) {
        return 1;
    }
    float v15 = static_cast<float>(Random() % 100);
    if (v15 <= static_cast<double>(apCombatStyle->GetLRChance())) {
        float v16 = static_cast<float>(Random() % 100);
        if (!(v16 <= 49.0)) {
            if ((asMovementRestrictions & 8) == 0) {
                return 8;
            }
        }
        else {
            if ((asMovementRestrictions & 4) == 0) {
                return 4;
            }
        }
    }
    return v18;
}

float CombatFormulas::GetWeaponRateOfFire(ActorValueOwner* apActor, TESObjectWEAP* apWeapon, ItemChange* apItem) {
    Actor* pactor;
    float frateOfFire;
    pactor = static_cast<Actor*>(apActor);
    if (apWeapon->IsMeleeWeapon()) {
        frateOfFire = apWeapon->GetSpeed();
    }
    else {
        Actor* v3 = static_cast<Actor*>(
            FNVD_DYNAMIC_CAST_VOID(TESObjectREFR)(apActor, RTTI_ActorValueOwner, RTTI_TESObjectREFR)
        );
        frateOfFire = Actor::GetAttackSpeedMult(v3, apWeapon, 1, apItem);
    }
    bool bUseSplit = false;
    if (apItem) {
        bUseSplit = apItem->HasModEffectActive(12);
    }
    return static_cast<double>(apWeapon->GetNumProjectiles(bUseSplit, false, pactor)) * frateOfFire;
}

float CombatFormulas::ApplyWeaponConditionScaling(float fWeapHealthPercent) {
    if (!(fWeapHealthPercent > 0.75)) {
        return 1.0 - (0.75 - fWeapHealthPercent) * 0.6700000166893005;
    }
    else {
        return 1.0;
    }
}

void CombatFormulas::CheckDismemberOrExplode(
    ActorValueOwner* apAttacker,
    TESObjectWEAP* apWeapon,
    int32_t aiExplodeChance,
    int32_t aiDismemberBonus,
    int32_t aiExplodeBonus,
    bool abExplodeChanceAbsolute,
    bool& abDismember,
    bool& abExplode
) {
    int32_t iexplodeChance;
    abDismember = false;
    abExplode = false;
    int32_t idieRoll = Random() % 100;
    if (abExplodeChanceAbsolute) {
        iexplodeChance = aiExplodeBonus + aiExplodeChance;
    }
    else {
        iexplodeChance = static_cast<int32_t>(
            static_cast<double>(iCombatExplodePartChance.Int() + aiExplodeBonus) *
            (static_cast<double>(aiExplodeChance) * 0.009999999776482582)
        );
    }
    int32_t idismemberChance = iCombatDismemberPartChance.Int() + aiDismemberBonus;
    if (idieRoll >= iexplodeChance) {
        if (idieRoll < idismemberChance) {
            abDismember = true;
        }
    }
    else {
        abExplode = true;
    }
    if (apWeapon) {
        if (abExplode || abDismember) {
            WEAPONHITBEHAVIOR eHitBehavior = apWeapon->data.eHitBehavior;
            if (eHitBehavior == WHB_DISMEMBER_ONLY) {
                abExplode = false;
                abDismember = true;
            }
            else if (eHitBehavior == WHB_EXPLODE_ONLY) {
                abExplode = true;
                abDismember = false;
            }
            else if (eHitBehavior == WHB_NO_DISMEMBER_OR_EXPLODE) {
                abExplode = false;
                abDismember = false;
            }
        }
    }
}

float CombatFormulas::GetWeaponStrengthRequirementModifier(uint32_t aiSTRReq, uint32_t aiSTR, uint32_t aiSTRMod) {
    uint32_t a1;
    a1 = 0;
    if (aiSTR > aiSTRReq) {
        a1 = 0;
    }
    else {
        a1 = aiSTRReq - aiSTR - aiSTRMod;
    }
    BSClamp<int32_t>(*reinterpret_cast<int32_t*>(&a1), 0, 10);
    double v6 = static_cast<double>(static_cast<int32_t>(a1));
    return fWeapStrengthReqPenalty.Float() * v6;
}

float CombatFormulas::GetWeaponSkillRequirementModifier(uint32_t aiSkillReq, uint32_t aiSkill, uint32_t aiSTRMod) {
    uint32_t iDiffSkill = 0;
    float fDiffSkill = 0.0f;
    if (aiSkill <= aiSkillReq) {
        fDiffSkill = static_cast<float>(aiSkillReq - aiSkill - aiSTRMod);
        float fCeil = FloatCeil(fDiffSkill / 10.0);
        iDiffSkill = static_cast<uint32_t>(static_cast<int64_t>(fCeil));
        BSClampUInt32(&iDiffSkill, 0, 10u);
    }
    return fWeapStrengthReqPenalty.Float() * static_cast<double>(iDiffSkill);
}

double CombatFormulas::CalcExplosionDamage(BGSExplosion* apActorBase) {
    float fDamage;
    if (apActorBase) {
        fDamage = apActorBase->GetDamage();
    }
    else {
        fDamage = 0.0;
    }
    float* fDamageGunWeapCondBase = &::fDamageGunWeapCondBase.Float();
    return ((fDamageGunWeapCondMult.Float() + *fDamageGunWeapCondBase) * fDamage);
}

float BGSProjectile::GetExplosionProximity() {
    return Data.fExplosionProximity;
}

float TESCombatStyle::GetAttackNoAttackMult() {
    return AdvancedData.fAttackNoAttackMult;
}

float TESCombatStyle::GetAttackHandBonus() {
    return Data.fAttackHandBonus;
}

float TESCombatStyle::GetAttackDuringRecoilStaggerBonus() {
    return Data.fAttackDuringRecoilStaggerBonus;
}

float TESCombatStyle::GetAttackDuringUnconsciousBonus() {
    return Data.fAttackDuringUnconsciousBonus;
}

float TESCombatStyle::GetAttackDuringAttackMult() {
    return AdvancedData.fAttackDuringAttackMult;
}

float TESCombatStyle::GetAttackDuringBlockMult() {
    return AdvancedData.fAttackDuringBlockMult;
}

char TESCombatStyle::GetAttackChance() {
    return Data.cAttackChance;
}

float TESCombatStyle::GetAttackSkillMult() {
    return AdvancedData.fAttackSkillMult;
}

float TESCombatStyle::GetBlockNoAttackMult() {
    return AdvancedData.fBlockNoAttackMult;
}

char TESCombatStyle::GetBlockChance() {
    return Data.cBlockChance;
}

float TESCombatStyle::GetBlockSkillBase() {
    return AdvancedData.fBlockSkillBase;
}

float TESCombatStyle::GetBlockSkillMult() {
    return AdvancedData.fBlockSkillMult;
}

float TESCombatStyle::GetBlockDuringAttackMult() {
    return AdvancedData.fBlockDuringAttackMult;
}

float TESCombatStyle::GetDodgeFatigueBase() {
    return AdvancedData.fDodgeFatigueBase;
}

float TESCombatStyle::GetDodgeFatigueMult() {
    return AdvancedData.fDodgeFatigueMult;
}

float TESCombatStyle::GetDodgeSpeedBase() {
    return AdvancedData.fDodgeSpeedBase;
}

float TESCombatStyle::GetDodgeNoAttackMult() {
    return AdvancedData.fDodgeNoAttackMult;
}

void TESObjectWEAP::SetAutomaticWeapon(bool abVal) {
    if (abVal) {
        data.cFlags |= 2u;
        uint8_t cFlags = data.cFlags;
    }
    else {
        data.cFlags &= ~2u;
        uint8_t v3 = data.cFlags;
    }
    uint8_t v4 = data.cFlags;
}
