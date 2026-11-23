#include "Fallout/AI/Actor.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkBlendCollisionObjectAddRotation.h"
#include "BSHavok/bhkBlendController.h"
#include "BSHavok/bhkCachingShapePhantom.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkFilteredCdBodyCollector.h"
#include "BSHavok/bhkForceController.h"
#include "BSHavok/bhkLimitedHingeConstraint.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkOrientHingedBodyAction.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkPoseArray.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRagdollPenetrationUtil.h"
#include "BSHavok/bhkRagdollTemplate.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkTTNode.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldM.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSHavok/hkOrientHingedBodyAction.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSDecalPlacementVectorExtraData.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSTestObjects.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/IOManager.h"
#include "BSMain/NiBSBoneLODController.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/Tile.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueInfo.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/AnimSequenceBase.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSAmmoForm.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSCameraShot.h"
#include "Fallout Shared/BGSDebris.h"
#include "Fallout Shared/BGSDecalEmitter.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSImpactData.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BGSTouchSpellForm.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BSAnimNoteReceiver.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/CreatureSounds.h"
#include "Fallout Shared/ExperiencePoints.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSWaterCollisionManager.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/HitCaster.h"
#include "Fallout Shared/Havok/SpecificItemCollector.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/Havok/TESTrapListener.h"
#include "Fallout Shared/Havok/TrapEntry.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/LipSynchAnim.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicItemForm.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageDataUseWeapon.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/Pathfinding/NavMeshEdgeLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingSearchRayCast.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESAIForm.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESChallengeManager.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESFurniture.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESLandTexture.h"
#include "Fallout Shared/TESLevItem.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestStage.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESReactionForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESSpellList.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/ActorPackage.h"
#include "Fallout/AI/AlarmPackage.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BackUpPackage.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CachedValues.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/CombatDialogueManager.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/ContinuousBeamProjectile.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/Combat/GrenadeProjectile.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Fallout/AI/Combat/MuzzleFlash.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/Creature.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/FollowerBarks.h"
#include "Fallout/AI/GunWobble.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LipSyncBackgroundManager.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/ModifierList.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/Movement/DetailedActorPathHandler.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/Procedure.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/AI/SandBoxPackage.h"
#include "Fallout/AI/SearchPackage.h"
#include "Fallout/AI/TrespassPackage.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Dialogue/Conversation.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/SurgeryMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/DetectLifeEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicLight.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ReanimateEffect.h"
#include "Fallout/Magic/SummonCreatureEffect.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNote.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNoteListener.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSGrabIKNote.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatExtraDataController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpCachingShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace bhkConvert {} // namespace bhkConvert

static float fFirstPersonSwimFloatHeight = 1.3f;

// The Hit settings of the ragdoll hit blend: the nodes the hit starts at and pushes, and the bounds of the hit and
// block forces.
static SettingT<BlendSettingCollection> fHitForceMin("fHFMin:HIT", 0.0f);
static SettingT<BlendSettingCollection> fHitForceMax("fHFMax:HIT", 0.0f);
static SettingT<BlendSettingCollection> fBlockForceMin("fBFMin:HIT", 0.0f);
static SettingT<BlendSettingCollection> fBlockForceMax("fBFMax:HIT", 0.0f);
static SettingT<BlendSettingCollection> sHitSpot("sHitSpot:HIT", "Bip01 Spine2");
static SettingT<BlendSettingCollection> sHitStart("sHitStart:HIT", "Bip01 Spine1");

static BSPerformanceTimer AIProcessingProcess("AIProcessing", "Process", true, true);

// Started and stopped around the update.
static BSPerformanceTimer AIProcessing_Update("AIProcessing", "Update", true, true);

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bVATSSmartCameraCheckDebug("bVATSSmartCameraCheckDebug:VATS", false);
SettingT<INISettingCollection> bDismemberOneLimb("bDismemberOneLimb:Combat", false);
SettingT<INISettingCollection> bTrackAllDeaths("bTrackAllDeaths:General", false);
SettingT<INISettingCollection> iShowHitVector("iShowHitVector:Combat", 0);
SettingT<INISettingCollection> fHitVectorDelay("fHitVectorDelay:Combat", 0.4f);
SettingT<INISettingCollection> fMinBloodDamage("fMinBloodDamage:Combat", 1.0f);
SettingT<INISettingCollection> iMaxHiPerfCombatCount("iMaxHiPerfCombatCount:Combat", 4);
SettingT<INISettingCollection> fJumpAnimDelay("fJumpAnimDelay:HAVOK", 0.75f);
SettingT<INISettingCollection> iDetectionPicks("iDetectionPicks:GamePlay", 21);
SettingT<INISettingCollection> iDetectionHighNumPicks("iDetectionHighNumPicks", 40);
SettingT<INISettingCollection> bPlayStaggers("bPlayStaggers:Combat", true);
SettingT<INISettingCollection> bPlayHitLocationIdles("bPlayHitLocationIdles:Combat", true);
SettingT<INISettingCollection> bDisableNPCAttacks("bDisableNPCAttacks:Combat", false);
SettingT<INISettingCollection> bDisableAllGore("bDisableAllGore:General", false);
SettingT<INISettingCollection> bUseMovementBlockedPackage("bUseMovementBlockedPackage:General", false);
SettingT<INISettingCollection> bDisableHeadTracking("bDisableHeadTracking:HeadTracking", false);
SettingT<INISettingCollection> iUpdateActorsPerFrame("iUpdateActorsPerFrame:HeadTracking", 1);
SettingT<INISettingCollection> fUpdateDelaySecondsMin("fUpdateDelaySecondsMin:HeadTracking", 1.0f);
SettingT<INISettingCollection> fUpdateDelaySecondsMax("fUpdateDelaySecondsMax:HeadTracking", 1.5f);
SettingT<INISettingCollection> fUpdateDelayNewTargetSecondsMin("fUpdateDelayNewTargetSecondsMin:HeadTracking", 6.0f);
SettingT<INISettingCollection> fUpdateDelayNewTargetSecondsMax("fUpdateDelayNewTargetSecondsMax:HeadTracking", 10.0f);
SettingT<INISettingCollection> fHardLandingDamageThreshold("fHardLandingDamageThreshold:Audio", 500.0f);
BSPerformanceTimer CombatAI_Total("Combat AI", "Total", true, true);
BSPerformanceTimer AIProcessing_UpdateMovement("AIProcessing", "UpdateMovement", true, true);
BSPerformanceTimer AIProcessing_UpdateAnimation("AIProcessing", "UpdateAnimation", true, true);
BSPerformanceTimer AIProcessing_UpdateAnimationMovement("AIProcessing", "UpdateAnimationMovement", true, true);
SettingT<INISettingCollection> Actor::fDialogHitSoundCooldownMin("fDialogHitSoundCooldownMin:Audio", 2.0f);
SettingT<INISettingCollection> Actor::fDialogHitSoundCooldownMax("fDialogHitSoundCooldownMax:Audio", 4.0f);
BSCriticalSection Actor::kSpeakSoundCriticalSection;
Actor::CachedPositionArray Actor::CachedPositions;

CachedValuesOwner::CachedValuesOwner() {}

void CachedValuesOwner::CalculateAll() {
    CalculateCachedRadius();
    CalculateCachedWidth();
    CalculateCachedLength();
    CalculateCachedForwardLength();
    CalculateCachedWeaponDPS();
    CalculateCachedMedicineEffectivenessMult();
    CalculateCachedParalysis();
    CalculateCachedHealingRate();
    CalculateCachedEndurance();
    CalculateCachedPerceptionCondition();
    CalculateCachedEyeLevel();
    CalculateCachedActorAggression();
    CalculateCachedActorAssistance();
    CalculateCachedWalkSpeed();
    CalculateCachedRunSpeed();
    CalculateCachedConditionPreventsRun();
}

Actor::Actor(bool abAddToProcessList) {
    ConstructorInitialize(abAddToProcessList);
}

Actor::~Actor() {
    if (pRagdollController) {
        delete pRagdollController;
    }
    if (pPenetrationDetection) {
        delete pPenetrationDetection;
    }
    BSSoundHandle handle;
    m_Extra.GetSound(handle);
    if (handle.IsValid()) {
        handle.Release();
    }
    FalloutRadio::DisableNPCRadio(this);
    if (pCurrentCombatTargetArray) {
        delete pCurrentCombatTargetArray;
    }
    if (pCurrentCombatMemberArray) {
        delete pCurrentCombatMemberArray;
    }
    if (!GetTemporary()) {
        pPlayer->RemoveActorFromPlayercombatList(this);
        pPlayer->RemovePerceivedActor(this);
        ReferenceProcessLists.RemoveActorCloseToPlayer(this);
        ClearDispositionModifiers();
        ReferenceProcessLists.kLipSyncManager.CancelLipFile(this);
        ReferenceProcessLists.RemoveReferenceMuzzleFlash(this);
        if (!pDataHandler->GetClearingData()) {
            Dispel();
            ActorCause::ClearActor(this);
            CleanUpOtherMobileObjects(true);
            EndInterruptPackage(false);
            ReferenceProcessLists.ReferDestructorNullOut(this);
            ReferenceProcessLists.NullOutTargetRef(this, true);
            ReferenceProcessLists.NullOutActorInCrimeList(this);
            if (pCurrentProcess) {
                if (pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH || !pCurrentProcess->eLevel) {
                    BSShaderAccumulator* paccum = BSShaderManager::GetAccumulator();
                    if (paccum) {
                        paccum->RemoveBoundVolume(iFormID);
                    }
                    UnloadPermanentMagic();
                }
                CheckSummonedActor();
            }
        }
        ReferenceProcessLists.RemoveActorFromTempChangeList(this);
        if (bInCombat) {
            Actor::StopCombat(nullptr);
        }
        DeleteCastPowers();
        pPermanentModifiers.DeleteAllModifiers();
        pBaseValueOverrides.DeleteAllModifiers();
        Actor::DestroyActorMover();
        TESObjectREFR::Set3D(nullptr, false);
    }
}

uint16_t Actor::GetSaveSize(uint32_t aiFlags) {
    uint16_t isize = MobileObject::GetSaveSize(aiFlags);
    uint16_t ibaseSize = isize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        isize += 6;
    }
    isize = isize + 7;
    if ((aiFlags & CHANGE_ACTOR_LIFESTATE) != 0) {
        ++isize;
    }
    if ((aiFlags & CHANGE_ACTOR_DISPOSITION_MODIFIERS) != 0) {
        isize += 2;
        for (BSSimpleList<DispositionModifier*>* pmodlist = &DispModifierList; pmodlist && !pmodlist->IsEmpty();
             pmodlist = pmodlist->m_pkNext) {
            if (pmodlist->m_item->pactormodified) {
                isize += 8;
            }
        }
    }
    isize = isize + 2 + 8 * CastPowers.ItemsInList();
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 50u) {
        isize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 60u) {
        isize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 68u && (aiFlags & CHANGE_ACTOR_PERMANENT_MODIFIERS) != 0) {
        isize += pPermanentModifiers.GetSaveSize();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
        isize += 5;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 97u) {
        isize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 101u) {
        isize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        isize += 10;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 115u) {
        ++isize;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 123u) {
        ++isize;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                isize - ibaseSize,
                pheader->iFormID,
                pform->GetFormEditorID(),
                pheader->iFlags,
                0x4C64,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                isize - ibaseSize,
                0x4C64,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp"
            );
        }
    }
    return isize;
}

// Saves the actor's state in the old format: a block header and size when the save uses blocks, the timers and flags,
// the life state, the disposition modifiers and the cast powers (each a form ID and a value, behind a count patched in
// afterwards), then the fields later save versions added. Two blocks guarded by `aiFlags & 0` can never run and are
// left out.
void Actor::SaveGame(uint32_t aiFlags) {
    MobileObject::SaveGame(aiFlags);
    uint16_t usBlockSize = 0;
    void* pBlockSize = nullptr;
    uint8_t* pStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // 'BLOK', the block header.
        uint32_t iBlockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockHeader, sizeof(iBlockHeader));
        pBlockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    SaveGameDataOLD(&fLastUpdate, sizeof(fLastUpdate));
    SaveGameDataOLD(&bForceRun, sizeof(bForceRun));
    SaveGameDataOLD(&bForceSneak, sizeof(bForceSneak));
    SaveGameDataOLD(&bProcessMe, sizeof(bProcessMe));
    if ((aiFlags & CHANGE_ACTOR_LIFESTATE) != 0) {
        uint8_t clifestate = GetLifeState();
        SaveGameDataOLD(&clifestate, sizeof(clifestate));
    }
    if ((aiFlags & CHANGE_ACTOR_DISPOSITION_MODIFIERS) != 0) {
        uint16_t usCount = 0;
        uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&usCount, sizeof(usCount));
        for (BSSimpleList<DispositionModifier*>* pmodlist = &DispModifierList; pmodlist && !pmodlist->IsEmpty();
             pmodlist = pmodlist->m_pkNext) {
            DispositionModifier* pmod = pmodlist->m_item;
            if (pmod->pactormodified) {
                uint32_t iactorID = pmod->pactormodified->iFormID;
                SaveNumericID(&iactorID, 4);
                SaveGameDataOLD(&pmod->modifieramount, sizeof(pmod->modifieramount));
                ++usCount;
            }
        }
        *pCount = usCount;
    }
    uint16_t usPowerCount = 0;
    uint16_t* pPowerCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&usPowerCount, sizeof(usPowerCount));
    for (BSSimpleList<void*>* plist = &CastPowers; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        // CastPowers is a void* list whose items are CastPowerItem pointers.
        CastPowerItem* pcastPower = static_cast<CastPowerItem*>(plist->m_item);
        uint32_t ipowerID = 0;
        if (pcastPower->pPower) {
            ipowerID = pcastPower->pPower->iFormID;
        }
        SaveNumericID(&ipowerID, 4);
        SaveGameDataOLD(&pcastPower->fTimeRemaining, sizeof(pcastPower->fTimeRemaining));
        ++usPowerCount;
    }
    *pPowerCount = usPowerCount;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 50u) {
        uint32_t imyKillerID = 0;
        if (pMyKiller) {
            imyKillerID = pMyKiller->iFormID;
        }
        SaveNumericID(&imyKillerID, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 60u) {
        uint32_t itemplateActorBaseID = 0;
        if (pTemplateActorBase) {
            itemplateActorBaseID = pTemplateActorBase->iFormID;
        }
        SaveNumericID(&itemplateActorBaseID, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 68u && (aiFlags & CHANGE_ACTOR_PERMANENT_MODIFIERS) != 0) {
        pPermanentModifiers.SaveGame();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bMurderAlarm, sizeof(bMurderAlarm));
        uint32_t iaddMovementBlockedID = 0;
        pSaveLoadGameOLD->SaveNumericID(&iaddMovementBlockedID, 4u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 97u) {
        uint32_t idialogueItemTargetID = 0;
        if (pDialogueItemTarget) {
            idialogueItemTargetID = pDialogueItemTarget->iFormID;
        }
        pSaveLoadGameOLD->SaveNumericID(&idialogueItemTargetID, 4u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 101u) {
        SaveGameDataOLD(&fCheckMyDeadBodyTimer, sizeof(fCheckMyDeadBodyTimer));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        SaveGameDataOLD(&bForceUpdateQuestTarget, sizeof(bForceUpdateQuestTarget));
        SaveGameDataOLD(&bInWater, sizeof(bInWater));
        SaveGameDataOLD(&fHeadTrackTimer, sizeof(fHeadTrackTimer));
        SaveGameDataOLD(&fUpdateTargetTimer, sizeof(fUpdateTargetTimer));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 115u) {
        SaveGameDataOLD(&bSetOnDeath, sizeof(bSetOnDeath));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 123u) {
        SaveGameDataOLD(&bDeadFlag, sizeof(bDeadFlag));
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pEndPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* pEditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                pEndPosition - pStartPosition,
                pheader->iFormID,
                pEditorID,
                iFlags,
                19721,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                pEndPosition - pStartPosition,
                19721,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pCurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pBlockStart = static_cast<uint8_t*>(pBlockSize);
        if (pCurrent > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp",
                19721
            );
        }
        *static_cast<uint16_t*>(pBlockSize) = static_cast<uint16_t>(pCurrent - pBlockStart);
    }
}

void Actor::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t clifestate;
    float flastUpdateDelta;
    MobileObject::SaveGame(apSaveGameBuffer);
    flastUpdateDelta = ReferenceProcessLists.GetSystemTimeClock() - fLastUpdate;
    apSaveGameBuffer->SaveDataEndian(&flastUpdateDelta, sizeof(flastUpdateDelta), 0);
    apSaveGameBuffer->SaveDataEndian(&bForceRun, sizeof(bForceRun), 0);
    apSaveGameBuffer->SaveDataEndian(&bForceSneak, sizeof(bForceSneak), 0);
    apSaveGameBuffer->SaveDataEndian(&bProcessMe, sizeof(bProcessMe), 0);
    apSaveGameBuffer->SaveDataEndian(&bMurderAlarm, sizeof(bMurderAlarm), 0);
    apSaveGameBuffer->SaveDataEndian(&fCheckMyDeadBodyTimer, sizeof(fCheckMyDeadBodyTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&bTalkingToPlayer, sizeof(bTalkingToPlayer), 0);
    apSaveGameBuffer->SaveDataEndian(&eQueuedattack, sizeof(eQueuedattack), 0);
    apSaveGameBuffer->SaveDataEndian(&bDeadFlag, sizeof(bDeadFlag), 0);
    apSaveGameBuffer->SaveDataEndian(&bForceUpdateQuestTarget, sizeof(bForceUpdateQuestTarget), 0);
    apSaveGameBuffer->SaveDataEndian(&bEVPBuffered, sizeof(bEVPBuffered), 0);
    apSaveGameBuffer->SaveDataEndian(&bResetAI, sizeof(bResetAI), 0);
    apSaveGameBuffer->SaveDataEndian(&bInWater, sizeof(bInWater), 0);
    apSaveGameBuffer->SaveDataEndian(&bSwimming, sizeof(bSwimming), 0);
    apSaveGameBuffer->SaveDataEndian(&iActionValue, sizeof(iActionValue), 0);
    apSaveGameBuffer->SaveDataEndian(&fTimeronAction, sizeof(fTimeronAction), 0);
    apSaveGameBuffer->SaveDataEndian(&fHeadTrackTimer, sizeof(fHeadTrackTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&bSetOnDeath, sizeof(bSetOnDeath), 0);
    apSaveGameBuffer->SaveDataEndian(&bContainerReset, sizeof(bContainerReset), 0);
    apSaveGameBuffer->SaveDataEndian(&bPlayerTeammate, sizeof(bPlayerTeammate), 0);
    apSaveGameBuffer->SaveDataEndian(&iEmotion, sizeof(iEmotion), 0);
    apSaveGameBuffer->SaveDataEndian(&iEmotionValue, sizeof(iEmotionValue), 0);
    apSaveGameBuffer->SaveDataEndian(&bBlockPostAnim, sizeof(bBlockPostAnim), 0);
    apSaveGameBuffer->SaveDataEndian(&bReloadTargetQueued, sizeof(bReloadTargetQueued), 0);
    apSaveGameBuffer->SaveDataEndian(&eCriticalStage, sizeof(eCriticalStage), 0);
    apSaveGameBuffer->SaveDataEndian(&bAttackOnNextTheft, sizeof(bAttackOnNextTheft), 0);
    apSaveGameBuffer->SaveDataEndian(&iThiefCrimeStamp, sizeof(iThiefCrimeStamp), 0);
    apSaveGameBuffer->SaveDataEndian(&bIgnoreCrime, sizeof(bIgnoreCrime), 0);
    apSaveGameBuffer->SaveDataEndian(&iMinorCrimes, sizeof(iMinorCrimes), 0);
    apSaveGameBuffer->SaveDataEndian(&iMajorCrimes, sizeof(iMajorCrimes), 0);
    apSaveGameBuffer->SaveDataEndian(&iLastSeenTime, sizeof(iLastSeenTime), 0);
    apSaveGameBuffer->SaveFormID(pMyKiller, 0);
    apSaveGameBuffer->SaveFormID(pTemplateActorBase, 0);
    apSaveGameBuffer->SaveFormID(pDialogueItemTarget, 0);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_LIFESTATE)) {
        clifestate = GetLifeState();
        apSaveGameBuffer->SaveDataEndian(&clifestate, sizeof(clifestate), 0);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_DISPOSITION_MODIFIERS)) {
        uint32_t isaveGameCount = 0;
        uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
        for (DispositionModifier* pmod : DispModifierList) {
            if (pmod) {
                apSaveGameBuffer->SaveFormID(pmod->pactormodified, 0);
                apSaveGameBuffer->SaveDataEndian(pmod, 4u, 0);
                ++isaveGameCount;
            }
        }
        apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_PERMANENT_MODIFIERS)) {
        pPermanentModifiers.SaveGame(apSaveGameBuffer);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_OVERRIDE_MODIFIERS)) {
        pBaseValueOverrides.SaveGame(apSaveGameBuffer);
    }
    pActorMover->SaveGame(apSaveGameBuffer);
}

// Loads the actor's state in the old format. A save without the inventory change re-equips the base's default outfit,
// and one that drops the life state brings a dead actor back (or restores its ragdoll). The life state and dead flag
// MobileObject::LoadGame changes are put back before this class's block, which is checked against its size afterwards.
// Form IDs read for the disposition modifiers, cast powers, killer and dialogue target stay in their pointer fields
// until InitLoadGame resolves them. Two blocks guarded by `aiFlags & 0` never run and are left out.
void Actor::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    uint32_t idialogueItemTargetID;
    uint32_t iaddMovementBlockedID;
    uint32_t itemplateActorBaseID;
    uint32_t imyKillerID;
    uint8_t clifestate;
    uint32_t iblockHeader;
    if ((aiCurrentFlags & CHANGE_REFR_INVENTORY) != 0 && (aiFlags & CHANGE_REFR_INVENTORY) == 0) {
        TESBoundObject* pbase = GetObjectReference();
        TESNPC* pnpc = nullptr;
        TESCreature* pcreature = nullptr;
        int32_t iFormType = pbase->GetFormType();
        if (iFormType == NPC__ID) {
            pnpc = static_cast<TESNPC*>(pbase);
        }
        else if (iFormType == CREA_ID) {
            pcreature = static_cast<TESCreature*>(pbase);
        }
        bool bweararmor = true;
        bool bwearweapon = true;
        TESPackage* pPack = GetCurrentPackage();
        if (pPack) {
            bwearweapon = !pPack->GetForceUnequipWeapon();
        }
        if (pnpc) {
            pnpc->InitDefaultWorn(this, bweararmor, bwearweapon, false, true);
        }
        else if (pcreature) {
            pcreature->InitDefaultWorn(this, bweararmor, bwearweapon, true);
        }
    }
    if ((aiFlags & CHANGE_ACTOR_LIFESTATE) == 0 && (aiCurrentFlags & CHANGE_ACTOR_LIFESTATE) != 0) {
        if (GetActorBase()->GetStartsDead()) {
            RestoreRagDollData(nullptr);
        }
        else {
            SetLifeState(ACTOR_LIFE_STATE_ALIVE);
            bDeadFlag = false;
            ClearDeathState();
        }
    }
    ACTOR_LIFE_STATE eprevLifeState = GetLifeState();
    bool brestoreDeadFlag = bDeadFlag;
    pDataHandler->SetSaveLoad(true);
    MobileObject::LoadGame(aiFlags, aiCurrentFlags);
    pDataHandler->SetSaveLoad(false);
    uint16_t usBlockSize = 0;
    uint8_t* istartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        // 'BLOK', the block header.
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp",
                    19784,
                    pheader->iFormID,
                    pform->GetFormEditorID(),
                    pheader->cVersion,
                    pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp",
                    19784,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    eLifeState = eprevLifeState;
    bDeadFlag = brestoreDeadFlag;
    LoadGameDataOLD(&fLastUpdate, sizeof(fLastUpdate));
    LoadGameDataOLD(&bForceRun, sizeof(bForceRun));
    LoadGameDataOLD(&bForceSneak, sizeof(bForceSneak));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 37u) {
        LoadGameDataOLD(&bProcessMe, sizeof(bProcessMe));
    }
    if ((aiFlags & CHANGE_ACTOR_LIFESTATE) != 0) {
        LoadGameDataOLD(&clifestate, sizeof(clifestate));
        ACTOR_LIFE_STATE ecurrentLifeState = GetLifeState();
        if (ecurrentLifeState != clifestate) {
            eLifeState = static_cast<ACTOR_LIFE_STATE>(clifestate);
            if (clifestate != ACTOR_LIFE_STATE_DEAD &&
                clifestate != ACTOR_LIFE_STATE_DYING &&
                clifestate != ACTOR_LIFE_STATE_ESSENTIAL_DOWN &&
                (eprevLifeState == ACTOR_LIFE_STATE_DEAD ||
                 eprevLifeState == ACTOR_LIFE_STATE_DYING ||
                 eprevLifeState == ACTOR_LIFE_STATE_ESSENTIAL_DOWN)) {
                ClearDeathState();
                bDeadFlag = false;
            }
        }
    }
    if ((aiFlags & CHANGE_ACTOR_DISPOSITION_MODIFIERS) != 0) {
        uint16_t usCount;
        pSaveLoadGameOLD->LoadGameDataOLD(&usCount, sizeof(usCount));
        for (uint32_t i = 0; i < usCount; ++i) {
            DispositionModifier* pmod = static_cast<DispositionModifier*>(ourMalloc(sizeof(DispositionModifier)));
            uint32_t iactorID;
            LoadNumericID(&iactorID, 4);
            pmod->pactormodified = reinterpret_cast<Actor*>(static_cast<uintptr_t>(iactorID));
            LoadGameDataOLD(&pmod->modifieramount, sizeof(pmod->modifieramount));
            DispModifierList.AddHead(pmod);
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 20u) {
        uint16_t usPowerCount;
        pSaveLoadGameOLD->LoadGameDataOLD(&usPowerCount, sizeof(usPowerCount));
        for (uint32_t i = 0; i < usPowerCount; ++i) {
            // CastPowers is a void* list, so the item is kept as the void* the list takes.
            void* pitem = ourMalloc(sizeof(CastPowerItem));
            CastPowerItem* pcastPower = static_cast<CastPowerItem*>(pitem);
            uint32_t ipowerID;
            LoadNumericID(&ipowerID, 4);
            pcastPower->pPower = reinterpret_cast<SpellItem*>(static_cast<uintptr_t>(ipowerID));
            LoadGameDataOLD(&pcastPower->fTimeRemaining, sizeof(pcastPower->fTimeRemaining));
            CastPowers.AddHead(pitem);
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 50u) {
        LoadNumericID(&imyKillerID, 4);
        pMyKiller = reinterpret_cast<Actor*>(static_cast<uintptr_t>(imyKillerID));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 60u) {
        LoadNumericID(&itemplateActorBaseID, 4);
        if (itemplateActorBaseID) {
            TESForm* ptemplateForm = TESForm::GetFormByNumericID(itemplateActorBaseID);
            pTemplateActorBase = dynamic_cast<TESActorBase*>(ptemplateForm);
        }
        else {
            pTemplateActorBase = nullptr;
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 68u && (aiFlags & CHANGE_ACTOR_PERMANENT_MODIFIERS) != 0) {
        pPermanentModifiers.LoadGameTES();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bMurderAlarm, sizeof(bMurderAlarm));
        pSaveLoadGameOLD->LoadNumericID(&iaddMovementBlockedID, 4u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 97u) {
        pSaveLoadGameOLD->LoadNumericID(&idialogueItemTargetID, 4u);
        pDialogueItemTarget = reinterpret_cast<MobileObject*>(static_cast<uintptr_t>(idialogueItemTargetID));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 101u) {
        LoadGameDataOLD(&fCheckMyDeadBodyTimer, sizeof(fCheckMyDeadBodyTimer));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        LoadGameDataOLD(&bForceUpdateQuestTarget, sizeof(bForceUpdateQuestTarget));
        LoadGameDataOLD(&bInWater, sizeof(bInWater));
        LoadGameDataOLD(&fHeadTrackTimer, sizeof(fHeadTrackTimer));
        LoadGameDataOLD(&fUpdateTargetTimer, sizeof(fUpdateTargetTimer));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 115u) {
        LoadGameDataOLD(&bSetOnDeath, sizeof(bSetOnDeath));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 123u) {
        LoadGameDataOLD(&bDeadFlag, sizeof(bDeadFlag));
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[usBlockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[usBlockSize]) {
                    uint32_t iFlags = pheader->iFlags;
                    int32_t iVersion = pheader->cVersion;
                    const char* pEditorID = pform->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  "
                        "Currently loading form is %08X %s with version %i and flags %08X",
                        &istartBufferPosition[usBlockSize] - icurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp",
                        19967,
                        pheader->iFormID,
                        pEditorID,
                        iVersion,
                        iFlags
                    );
                }
            }
            else {
                uint32_t iFlags = pheader->iFlags;
                int32_t iVersion = pheader->cVersion;
                const char* pEditorID = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    icurrentBufferPosition - &istartBufferPosition[usBlockSize],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp",
                    19967,
                    pheader->iFormID,
                    pEditorID,
                    iVersion,
                    iFlags
                );
            }
        }
        else if (icurrentBufferPosition <= &istartBufferPosition[usBlockSize]) {
            if (icurrentBufferPosition < &istartBufferPosition[usBlockSize]) {
                int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    &istartBufferPosition[usBlockSize] - icurrentBufferPosition,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp",
                    19967,
                    iVersion
                );
            }
        }
        else {
            int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                icurrentBufferPosition - &istartBufferPosition[usBlockSize],
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp",
                19967,
                iVersion
            );
        }
    }
}

void Actor::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint8_t clifestate;
    ACTOR_LIFE_STATE eprevLifeState = GetLifeState();
    MobileObject::LoadGame(apLoadGameBuffer);
    eLifeState = eprevLifeState;
    SetWalkRunFlagDirty();
    apLoadGameBuffer->LoadData(&fLastUpdate, sizeof(fLastUpdate));
    fLastUpdate = ReferenceProcessLists.GetSystemTimeClock() - fLastUpdate;
    apLoadGameBuffer->LoadData(&bForceRun, sizeof(bForceRun));
    apLoadGameBuffer->LoadData(&bForceSneak, sizeof(bForceSneak));
    apLoadGameBuffer->LoadData(&bProcessMe, sizeof(bProcessMe));
    apLoadGameBuffer->LoadData(&bMurderAlarm, sizeof(bMurderAlarm));
    apLoadGameBuffer->LoadData(&fCheckMyDeadBodyTimer, sizeof(fCheckMyDeadBodyTimer));
    apLoadGameBuffer->LoadData(&bTalkingToPlayer, sizeof(bTalkingToPlayer));
    apLoadGameBuffer->LoadData(&eQueuedattack, sizeof(eQueuedattack));
    apLoadGameBuffer->LoadData(&bDeadFlag, sizeof(bDeadFlag));
    apLoadGameBuffer->LoadData(&bForceUpdateQuestTarget, sizeof(bForceUpdateQuestTarget));
    apLoadGameBuffer->LoadData(&bEVPBuffered, sizeof(bEVPBuffered));
    apLoadGameBuffer->LoadData(&bResetAI, sizeof(bResetAI));
    apLoadGameBuffer->LoadData(&bInWater, sizeof(bInWater));
    apLoadGameBuffer->LoadData(&bSwimming, sizeof(bSwimming));
    apLoadGameBuffer->LoadData(&iActionValue, sizeof(iActionValue));
    apLoadGameBuffer->LoadData(&fTimeronAction, sizeof(fTimeronAction));
    apLoadGameBuffer->LoadData(&fHeadTrackTimer, sizeof(fHeadTrackTimer));
    apLoadGameBuffer->LoadData(&bSetOnDeath, sizeof(bSetOnDeath));
    apLoadGameBuffer->LoadData(&bContainerReset, sizeof(bContainerReset));
    apLoadGameBuffer->LoadData(&bPlayerTeammate, sizeof(bPlayerTeammate));
    apLoadGameBuffer->LoadData(&iEmotion, sizeof(iEmotion));
    apLoadGameBuffer->LoadData(&iEmotionValue, sizeof(iEmotionValue));
    apLoadGameBuffer->LoadData(&bBlockPostAnim, sizeof(bBlockPostAnim));
    apLoadGameBuffer->LoadData(&bReloadTargetQueued, sizeof(bReloadTargetQueued));
    if (apLoadGameBuffer->GetVersion() >= 8u) {
        apLoadGameBuffer->LoadData(&eCriticalStage, sizeof(eCriticalStage));
    }
    if (apLoadGameBuffer->GetVersion() >= 9u) {
        apLoadGameBuffer->LoadData(&bAttackOnNextTheft, sizeof(bAttackOnNextTheft));
        apLoadGameBuffer->LoadData(&iThiefCrimeStamp, sizeof(iThiefCrimeStamp));
        apLoadGameBuffer->LoadData(&bIgnoreCrime, sizeof(bIgnoreCrime));
        apLoadGameBuffer->LoadData(&iMinorCrimes, sizeof(iMinorCrimes));
        apLoadGameBuffer->LoadData(&iMajorCrimes, sizeof(iMajorCrimes));
        SetActorRefEssential(false);
        SetAlwaysShowSubtitles(false);
    }
    if (apLoadGameBuffer->GetVersion() >= 13u) {
        apLoadGameBuffer->LoadData(&iLastSeenTime, sizeof(iLastSeenTime));
    }
    apLoadGameBuffer->LoadFormID(pMyKiller);
    uint32_t itemplateActorBaseID = apLoadGameBuffer->LoadFormID();
    TESForm* ptemplateForm = TESForm::GetFormByNumericID(itemplateActorBaseID);
    pTemplateActorBase = dynamic_cast<TESActorBase*>(ptemplateForm);
    apLoadGameBuffer->LoadFormID(pDialogueItemTarget);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_LIFESTATE)) {
        clifestate = 0;
        apLoadGameBuffer->LoadData(&clifestate, sizeof(clifestate));
        eLifeState = static_cast<ACTOR_LIFE_STATE>(clifestate);
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_DISPOSITION_MODIFIERS)) {
        uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
        for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
            DispositionModifier* pmod = static_cast<DispositionModifier*>(ourMalloc(sizeof(DispositionModifier)));
            apLoadGameBuffer->LoadFormID(pmod->pactormodified);
            apLoadGameBuffer->LoadData(pmod, 4u);
            DispModifierList.AddHead(pmod);
        }
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_PERMANENT_MODIFIERS)) {
        pPermanentModifiers.LoadGame(apLoadGameBuffer);
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_OVERRIDE_MODIFIERS)) {
        pBaseValueOverrides.LoadGame(apLoadGameBuffer);
    }
    pActorMover->LoadGame(apLoadGameBuffer);
    if (TESObjectREFR::Get3D() && pCurrentProcess) {
        BipedAnim* pbipedAnim = GetBiped();
        InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(this);
        if (pbipedAnim && pinventory) {
            uint32_t icount = 19;
            for (BIPED_OBJECT i = BO_HEAD; i < icount; i = static_cast<BIPED_OBJECT>(i + 1)) {
                TESForm* ppartForm = pbipedAnim->GetParent(i);
                ItemChange* pitemChange = pinventory->GetWornItem(i, false);
                TESForm* pwornItem = pitemChange ? pitemChange->pContainerObj : nullptr;
                if (ppartForm) {
                    if (ppartForm->GetFormType() == RACE_ID) {
                        ppartForm = nullptr;
                    }
                    else if (ppartForm->IsArmorAddon()) {
                        ppartForm = nullptr;
                    }
                }
                if (ppartForm != pwornItem) {
                    pCurrentProcess->Set3DUpdateFlag(RESET_MODEL);
                    break;
                }
                if (pitemChange) {
                    delete pitemChange;
                }
            }
        }
        else if (pinventory && IsCreature() && bPlayerTeammate) {
            const char* pfullName;
            pfullName = GetFullName();
            if (std::strstr(pfullName, "Lily")) {
                ItemChange* pwornWeapon;
                pwornWeapon = pinventory->GetWornItem(BO_WEAPON, false);
                TESBoundObject* pwornWeaponObj;
                pwornWeaponObj = pwornWeapon ? pwornWeapon->pContainerObj : nullptr;
                if (pCurrentProcess->GetCurrentWeapon()) {
                    if (pCurrentProcess->GetCurrentWeapon()->pContainerObj != pwornWeaponObj) {
                        pCurrentProcess->Set3DUpdateFlag(RESET_MODEL);
                    }
                }
            }
        }
    }
    if (!IsDead(false) && eCriticalStage) {
        eCriticalStage = ACS_NONE;
    }
}

void Actor::InitLoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    MobileObject::InitLoadGame(aiFlags, aiCurrentFlags);
    if ((aiFlags & CHANGE_ACTOR_DISPOSITION_MODIFIERS) != 0) {
        // LoadGame left each modifier's actor as a form ID; a modifier whose actor no longer resolves is dropped.
        BSSimpleList<DispositionModifier*>* pmodlist = &DispModifierList;
        BSSimpleList<DispositionModifier*>* plastList = nullptr;
        while (pmodlist && !pmodlist->IsEmpty()) {
            DispositionModifier* pmod = pmodlist->m_item;
            uint32_t iactorID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pmod->pactormodified));
            TESForm* pactorForm = TESForm::GetFormByNumericID(iactorID);
            pmod->pactormodified = dynamic_cast<Actor*>(pactorForm);
            if (pmod->pactormodified) {
                plastList = pmodlist;
                pmodlist = pmodlist->m_pkNext;
            }
            else {
                if (plastList) {
                    plastList->Remove(pmod);
                    pmodlist = plastList->m_pkNext;
                }
                else {
                    pmodlist->RemoveHead();
                }
                ourFree(pmod);
            }
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 20u) {
        // LoadGame left each cast power's spell as a form ID; a power whose spell no longer resolves is dropped.
        // CastPowers is a list of void* holding CastPowerItem pointers, so each item is cast here.
        BSSimpleList<void*>* plist = &CastPowers;
        BSSimpleList<void*>* plastList = nullptr;
        while (plist && !plist->IsEmpty()) {
            void* pitem = plist->m_item;
            CastPowerItem* pcastPower = static_cast<CastPowerItem*>(pitem);
            uint32_t ispellID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pcastPower->pPower));
            if (ispellID) {
                TESForm* pspellForm = TESForm::GetFormByNumericID(ispellID);
                pcastPower->pPower = dynamic_cast<SpellItem*>(pspellForm);
            }
            if (pcastPower->pPower) {
                plastList = plist;
                plist = plist->m_pkNext;
            }
            else {
                if (plastList) {
                    plastList->Remove(pitem);
                    plist = plastList->m_pkNext;
                }
                else {
                    plist->RemoveHead();
                }
                ourFree(pcastPower);
            }
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 50u) {
        uint32_t imyKillerID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pMyKiller));
        if (imyKillerID) {
            TESForm* pkillerForm = TESForm::GetFormByNumericID(imyKillerID);
            pMyKiller = dynamic_cast<Actor*>(pkillerForm);
        }
        else {
            pMyKiller = nullptr;
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 97u) {
        uint32_t idialogueItemTargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDialogueItemTarget));
        if (idialogueItemTargetID) {
            TESForm* ptargetForm = TESForm::GetFormByNumericID(idialogueItemTargetID);
            pDialogueItemTarget = dynamic_cast<MobileObject*>(ptargetForm);
        }
        else {
            pDialogueItemTarget = nullptr;
        }
    }
    if (pCurrentProcess && pCurrentProcess->eLevel) {
        if (GetParentCell()) {
            if (GetParentCell()->GetAttached()) {
                MovetoHigh();
            }
        }
        else if (GetRefPersists()) {
            TESWorldSpace* pworld = GetWorldSpace();
            if (pworld) {
                // Copies the whole position before converting x and y.
                NiPoint3 kLocation = GetLocationOnReference();
                int32_t icellx = FloatToInt(kLocation.x) >> 12;
                int32_t icelly = FloatToInt(kLocation.y) >> 12;
                TESObjectCELL* pparentcell = pDataHandler->GetCellFromCellCoord(icellx, icelly, pworld, false);
                if (pparentcell && pparentcell->GetAttached()) {
                    MovetoHigh();
                    bool biniting = pSaveLoadGameOLD->GetLoading();
                    pSaveLoadGameOLD->SetLoading(false);
                    pparentcell->AddReference(this, false);
                    pSaveLoadGameOLD->SetLoading(biniting);
                }
            }
        }
    }

    if (!pCurrentProcess && !GetDelete() && !GetDisabled()) {
        pCurrentProcess = new LowProcess;
        PROCESS_TYPE enewLevel = GetDesiredProcessLevel();
        if (enewLevel) {
            if (enewLevel == PROCESS_TYPE_MIDDLE_HIGH) {
                MovetoMiddleHigh();
            }
            else if (enewLevel == PROCESS_TYPE_MIDDLE_LOW) {
                MovetoMiddleLow();
            }
        }
        else {
            MovetoHigh();
        }
    }
}

void Actor::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    MobileObject::InitLoadGame(apLoadGameBuffer);
    Actor* pkiller = nullptr;
    if (pMyKiller) {
        TESForm* pkillerForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pMyKiller))
        );
        pkiller = dynamic_cast<Actor*>(pkillerForm);
    }
    pMyKiller = pkiller;
    MobileObject* pdialogueTarget = nullptr;
    if (pDialogueItemTarget) {
        TESForm* ptargetForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDialogueItemTarget))
        );
        pdialogueTarget = dynamic_cast<MobileObject*>(ptargetForm);
    }
    pDialogueItemTarget = pdialogueTarget;
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_DISPOSITION_MODIFIERS)) {
        for (DispositionModifier* pmod : DispModifierList) {
            if (pmod) {
                Actor* pactor = nullptr;
                if (pmod->pactormodified) {
                    TESForm* pactorForm = TESForm::GetFormByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pmod->pactormodified))
                    );
                    pactor = dynamic_cast<Actor*>(pactorForm);
                }
                pmod->pactormodified = pactor;
            }
        }
    }
    TESPackage* pcurrentPackage = pCurrentProcess ? pCurrentProcess->GetCurrentPackage() : nullptr;
    if (pcurrentPackage && pcurrentPackage->GetPackType() == PACKAGE_TYPE_COMBAT) {
        bInCombat = true;
    }
    else {
        ClearInCombat(false);
    }
    pActorMover->InitLoadGame(apLoadGameBuffer);
    if (bPlayerTeammate) {
        pPlayer->AddTeammate(this);
    }
    if (apLoadGameBuffer->GetReconstructedForm() &&
        !apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_INVENTORY | CHANGE_REFR_LEVELED_INVENTORY) &&
        Get3D()) {
        if (IsCreature()) {
            TESCreature* pcreature = static_cast<TESCreature*>(GetActorBase());
            pcreature->InitDefaultWorn(this, true, true, true);
        }
        else {
            TESNPC* pnpc = static_cast<TESNPC*>(GetActorBase());
            pnpc->InitDefaultWorn(this, true, true, false, true);
        }
    }

    if (Get3D()) {
        if (this != pPlayer && GetLifeState() != ACTOR_LIFE_STATE_DEAD && GetLifeState() != ACTOR_LIFE_STATE_DYING) {
            Animation* panim = Actor::GetAnimation();
            if (panim && !panim->GroupLoaded(3)) {
                ResetLoadedAnimations();
            }
        }
        QueueReplacementLocomotions(false);
    }
    if (pCurrentProcess && pCurrentProcess->GetAll3DUpdateFlags()) {
        pCurrentProcess->Update3dModel(this);
    }

    if (pRagdollController) {
        pRagdollController->DisableRagdollAnim(false);
    }
    if (pPenetrationDetection) {
        pPenetrationDetection->ResetSystems();
    }
}

void Actor::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    if (pCurrentProcess) {
        pCurrentProcess->Update3dModel(this);
    }
    MobileObject::FinishInitLoadGame(aiFlags, aiOldFlags);
    if (GetCurrentSpell() && TESObjectREFR::Get3D()) {
        EffectSetting* pavEffectSetting = GetCurrentSpell()->GetAVEffect(MagicSystem::SELF);
        if (pavEffectSetting && pavEffectSetting->GetModelLength()) {
            Animation* panimation;
            NiAVObject* pdata3D = MagicProjectile::Clone3D_0(pavEffectSetting);
            if (this == pPlayer) {
                if (pPlayer->Is3rdPerson()) {
                    panimation = pPlayer->GetAnimation(false);
                }
                else {
                    panimation = pPlayer->GetAnimation(true);
                }
            }
            else {
                panimation = Actor::GetAnimation();
            }
            if (panimation && panimation->GetControllerManager()) {
                NiAVObject* pobj = panimation->GetControllerManager()->GetObjectPalette()->GetAVObject(
                    FixedStrings::QMagicNode()
                );
                NiNode* pnode = pobj ? pobj->IsNode() : nullptr;
                if (pdata3D && pnode) {
                    pnode->AttachChild(pdata3D, 1);
                    if (pavEffectSetting->data.pLight && !Main::GetSceneIsComplex()) {
                        MagicLight* plight;
                        plight = new MagicLight(pavEffectSetting->data.pLight->iFormID, pobj);
                        pLight = plight;
                    }
                }
            }
        }
    }

    UpdateAlpha();
    if (pCurrentProcess) {
        float frefract = pCurrentProcess->GetScriptRefractionPower();
        if (frefract > 0.0) {
            SetRefraction(true, frefract);
        }
    }
    if (GetSitSleepState() == SIT_SLEEP_IS_SLEEPING || (IsUnConscious() || IsDead(false))) {
        BSFaceGenAnimationData* panimData = GetFaceAnimationData();
        if (panimData) {
            panimData->SetDead(true, true);
        }
    }
    CheckTempModifiers();
    if (this != pPlayer && GetLifeState() != ACTOR_LIFE_STATE_DEAD && GetLifeState() != ACTOR_LIFE_STATE_DYING) {
        Animation* panim = Actor::GetAnimation();
        if (panim && !panim->GroupLoaded(3)) {
            InitAnimation();
        }
    }
    if (pCurrentProcess) {
        pCurrentProcess->CheckCachedValueFlags(this);
    }
}

void Actor::Revert(uint32_t aiFlags) {
    if (pSaveLoadGameOLD->IsUnk048Zero()) {
        Animation* panim;
        if (this == pPlayer) {
            Animation* panimation = pPlayer->GetAnimation(true);
            if (panimation && panimation->GetControllerManager()) {
                NiAVObject* pobj = panimation->GetControllerManager()->GetObjectPalette()->GetAVObject(
                    FixedStrings::QMagicNode()
                );
                NiNode* pnode = pobj ? pobj->IsNode() : nullptr;
                if (pnode) {
                    pnode->RemoveAllChildren();
                }
            }

            panim = pPlayer->GetAnimation(false);
        }
        else {
            panim = Actor::GetAnimation();
        }
        if (panim && panim->GetControllerManager()) {
            NiAVObject* pobj = panim->GetControllerManager()->GetObjectPalette()->GetAVObject(
                FixedStrings::QMagicNode()
            );
            NiNode* pnode = pobj ? pobj->IsNode() : nullptr;
            if (pnode) {
                pnode->RemoveAllChildren();
            }
        }

        if (pLight) {
            delete pLight;
        }
        pLight = nullptr;
    }
    MobileObject::Revert(aiFlags);
    if ((aiFlags & CHANGE_ACTOR_DISPOSITION_MODIFIERS) != 0) {
        // Frees the head's modifier and drops the head until the list is empty.
        BSSimpleList<DispositionModifier*>* pmodlist = &DispModifierList;
        while (pmodlist && !pmodlist->IsEmpty()) {
            ourFree(pmodlist->m_item);
            pmodlist->RemoveHead();
        }
    }
    if ((aiFlags & CHANGE_ACTOR_PERMANENT_MODIFIERS) != 0) {
        pPermanentModifiers.Revert();
    }
    if (pSaveLoadGameOLD->IsUnk048Zero()) {
        ResetFaceData();
        BSFaceGenAnimationData* panimData = GetFaceAnimationData();
        if (panimData) {
            panimData->SetDead(false, true);
        }
        DeleteCastPowers();
        bMurderAlarm = false;
        iVisFlags = 0;
        bWasInFrustum = true;
        bSetOnDeath = false;
        bInWater = false;
        bDeadFlag = false;
    }
}

void Actor::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    RemoveFromAllWater(true);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_LEVELED_INVENTORY | CHANGE_REFR_INVENTORY) ||
        apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_LEVELED_INVENTORY | CHANGE_REFR_INVENTORY)) {
        RemoveWeapon();
    }
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS) &&
        !apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS)) {
        ClearDismemberedLimbs();
    }
    if (pRagdollController && pRagdollController->QRagdollFeedbackActive()) {
        pRagdollController->SetRagdollFeedbackActive(0);
    }

    MobileObject::Revert(apLoadGameBuffer);
    ResetCachedGunSkill();
    ResetCachedWeaponCondition();
    ClearInCombat(false);
    SetWalkRunFlagDirty();
    fLastUpdate = 0.0f;
    bForceRun = false;
    bForceSneak = false;
    bProcessMe = true;
    bMurderAlarm = false;
    fCheckMyDeadBodyTimer = 0.0f;
    bTalkingToPlayer = false;
    eQueuedattack = ANIM_GROUP_NONE;
    bDeadFlag = false;
    bForceUpdateQuestTarget = false;
    bEVPBuffered = false;
    bResetAI = 0;
    bInWater = false;
    bSwimming = false;
    iActionValue = 0;
    fTimeronAction = 0.0f;
    fHeadTrackTimer = 0.0f;
    bSetOnDeath = false;
    bContainerReset = false;
    bPlayerTeammate = false;
    bFootIKInRange = false;
    bBlockPostAnim = false;
    bReloadTargetQueued = false;
    iEmotion = 0;
    iEmotionValue = 0;
    bSearchingInCombat = 0;
    pMyKiller = nullptr;
    pTemplateActorBase = nullptr;
    pDialogueItemTarget = nullptr;
    bLightingUpdatedNonMoving = false;
    bSearchingInCombat = 0;
    pCurrentCombatTarget = nullptr;
    if (pCurrentCombatTargetArray) {
        delete pCurrentCombatTargetArray;
    }
    pCurrentCombatTargetArray = nullptr;
    if (pCurrentCombatMemberArray) {
        delete pCurrentCombatMemberArray;
    }
    pCurrentCombatMemberArray = nullptr;
    bAttackOnNextTheft = false;
    bIgnoreCrime = false;
    iThiefCrimeStamp = 0;
    iMinorCrimes = 0;
    iMajorCrimes = 0;
    ResetRadiationResistanceMult();
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_ACTOR_DISPOSITION_MODIFIERS)) {
        ClearDispositionModifiers();
    }
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_ACTOR_PERMANENT_MODIFIERS)) {
        pPermanentModifiers.RevertBGS(apLoadGameBuffer);
    }
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_ACTOR_OVERRIDE_MODIFIERS)) {
        pBaseValueOverrides.RevertBGS(apLoadGameBuffer);
    }
    pActorMover->Revert(apLoadGameBuffer);
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_ACTOR_LIFESTATE)) {
        if (eLifeState) {
            ClearDeathState();
        }
        if (static_cast<double>(GetActorBase()->GetHealth()) > 0.0) {
            eLifeState = ACTOR_LIFE_STATE_ALIVE;
        }
        else {
            eLifeState = ACTOR_LIFE_STATE_DEAD;
        }
    }
    eCriticalStage = ACS_NONE;
    if (Get3D()) {
        NiNode* p3D = Get3D();
        if (TESHavokUtilities::Should3DRagDoll(this, p3D)) {
            RestoreRagDollData(nullptr);
        }
    }
}

void Actor::CheckSaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESObjectREFR::CheckSaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION) && !GetAnimation()) {
        apSaveGameBuffer->RemoveChange(CHANGE_REFR_ANIMATION);
    }
}

void Actor::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    MobileObject::FinishLoadGame(apLoadGameBuffer);
    pActorMover->FinishLoadGame(apLoadGameBuffer);
    if (IsDead(true)) {
        DetachCharController();
    }
    UpdateActor3DPosition();
    NiAVObject* pobj3D = Get3D();
    if (pobj3D) {
        if (!apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_HAVOK_MOVE) && (IsDead(false) || IsDying())) {
            if (pRagdollController) {
                pRagdollController->DisableRagdollAnim(true);
            }
            if (pPenetrationDetection) {
                CFilter kFilter;
                GetCollisionFilter(&kFilter.iFilter);
                pPenetrationDetection->Initialize(pobj3D, kFilter.GetGroup());
            }
            if (!m_Extra.GetRagDollData()) {
                bhkBlendController::DoKnockDown(pobj3D, NiPoint3::ZERO, 1, 0.0f, 0);
            }
        }
        NiUpdateData UpdateData(0.0f, false, false);
        pobj3D->Update(UpdateData);
    }
    UpdateAlpha();
    if (GetSitSleepState() == SIT_SLEEP_IS_SLEEPING || (IsUnConscious() || IsDead(false))) {
        BSFaceGenAnimationData* panimData = GetFaceAnimationData();
        if (panimData) {
            panimData->SetDead(true, true);
        }
    }
    if (eCriticalStage) {
        ApplyCriticalStage();
    }
    UpdateCombatData();
    UpdateDismemberedLimbVel();
}

bool Actor::GetEditorLocation(
    NiPoint3& arOriginalLocation,
    NiPoint3& arOriginalAngle,
    TESForm*& arpLocationForm,
    TESForm* apLocationForm
) {
    if (!pEditorLocForm) {
        return false;
    }
    arOriginalLocation = EditorLocCoord;
    arOriginalAngle.x = 0.0f;
    arOriginalAngle.y = 0.0f;
    arOriginalAngle.z = fEditorLocZRot;
    arpLocationForm = pEditorLocForm;
    return true;
}

NiPoint3 Actor::GetStartingAngle() {
    return NiPoint3(0.0f, 0.0f, fEditorLocZRot);
}

NiPoint3 Actor::GetStartingLocation() {
    return EditorLocCoord;
}

void Actor::UpdateRefLight() {
    TESObjectREFR::GetExtra()->GetLight();
    MobileObject::GetCurrentAIProcess();
}

// For each biped slot the new model fills, every worn biped model sharing it is unequipped and its parts removed,
// unless a can-not-wear item sits in the slot (that blocks the rest, and the add unless the save game is loading). A
// weapon or projectile replaces the current weapon (same can-not-wear rule) and reloads, using the mod clip size when
// an active mod increases it; ammo replaces the current ammo. Then the item is added worn, an NPC's biped gets it, its
// enchantment is preloaded and the refraction refreshed. The UnEquipObject results it stores are never read.
bool Actor::AddWornItem(TESBoundObject* apObject, int32_t aiNumber, ExtraDataList* apExtra, bool abForceEquip) {
    BipedAnim* pBipedAnim = this == pPlayer ? pPlayer->GetBiped(false) : GetBiped();
    TESBipedModelForm* pmodel = TESBipedModelForm::GetFormAsBipedModel(apObject);
    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(this);
    bool bleft = false;
    // Set once a slot holds an item that may not be unequipped.
    bool baddflag = false;
    if (pmodel && pBipedAnim) {
        for (int32_t i = BO_HEAD; i < BIPED_OBJECT_COUNT; ++i) {
            if (!pmodel->FillsBipedSlot(static_cast<BIPED_OBJECT>(i), false, nullptr)) {
                continue;
            }
            for (int32_t n = BO_HEAD; n < BIPED_OBJECT_COUNT; ++n) {
                TESForm* psearchForm = pBipedAnim->GetParent(static_cast<BIPED_OBJECT>(n));
                TESBipedModelForm* psearchModel = dynamic_cast<TESBipedModelForm*>(psearchForm);
                if (!psearchModel ||
                    !psearchModel->FillsBipedSlot(static_cast<BIPED_OBJECT>(i), false, nullptr) ||
                    psearchForm->GetFormType() == RACE_ID) {
                    continue;
                }
                ItemChange* pchange = GetExtra()->GetContainerChanges()->GetWornItem(
                    static_cast<BIPED_OBJECT>(i),
                    false
                );
                ExtraDataList* pextra = nullptr;
                if (pchange && pchange->GetExtraDataList() && pchange->GetExtraDataList()->GetItem()) {
                    if (pchange->GetExtraDataList()->GetItem()->GetCanNotWear()) {
                        baddflag = true;
                    }
                    pextra = pchange->GetExtraDataList()->GetItem();
                }
                if (!baddflag) {
                    UnEquipObject(static_cast<TESBoundObject*>(psearchForm), 1, pextra, false, false, true);
                    for (int32_t ioldslot = BO_HEAD; ioldslot < BIPED_OBJECT_COUNT; ++ioldslot) {
                        if (psearchModel->FillsBipedSlot(static_cast<BIPED_OBJECT>(ioldslot), false, nullptr)) {
                            pBipedAnim->RemovePart(static_cast<BIPED_OBJECT>(ioldslot), true, 0);
                        }
                    }
                }
                if (pchange) {
                    delete pchange;
                }
            }
        }
    }
    if (baddflag && !pSaveLoadGameOLD->GetLoading()) {
        return false;
    }
    if (pCurrentProcess) {
        switch (apObject->GetFormType()) {
            case WEAP_ID:
            case PROJ_ID: {
                ItemChange* pchangeItem = pCurrentProcess->GetCurrentWeapon();
                if (pchangeItem) {
                    bool bCanNotWear = false;
                    if (pchangeItem->GetContainerObject() != apObject &&
                        pchangeItem->GetExtraDataList() &&
                        pchangeItem->GetExtraDataList()->GetItem() &&
                        pchangeItem->GetExtraDataList()->GetItem()->GetCanNotWear()) {
                        bCanNotWear = true;
                    }
                    UnEquipObject(
                        pchangeItem->GetContainerObject(),
                        1,
                        pchangeItem->GetExtraDataList()->GetItem(),
                        false,
                        false,
                        false
                    );
                    if (bCanNotWear && !pSaveLoadGameOLD->GetLoading()) {
                        return false;
                    }
                    if (!GetActorType()) {
                        ReloadTargets(false);
                    }
                }
                else if (this == pPlayer) {
                    pCurrentProcess->SetWeaponDrawn(this, false);
                }
                bool bUseModValue = false;
                TESObjectWEAP* pweap = static_cast<TESObjectWEAP*>(apObject);
                if (apExtra) {
                    if (pweap->GetModEffect(TESObjectWEAP::MOD_ONE) == WEAPON_MOD_INCREASE_CLIP_SIZE &&
                        apExtra->GetWeaponModSlotActive(TESObjectWEAP::MOD_ONE)) {
                        bUseModValue = true;
                    }
                    else if (
                        pweap->GetModEffect(TESObjectWEAP::MOD_TWO) == WEAPON_MOD_INCREASE_CLIP_SIZE &&
                        apExtra->GetWeaponModSlotActive(TESObjectWEAP::MOD_TWO)
                    ) {
                        bUseModValue = true;
                    }
                    else if (
                        pweap->GetModEffect(TESObjectWEAP::MOD_THREE) == WEAPON_MOD_INCREASE_CLIP_SIZE &&
                        apExtra->GetWeaponModSlotActive(TESObjectWEAP::MOD_THREE)
                    ) {
                        bUseModValue = true;
                    }
                }
                ReloadWeaponNV(pweap, 0, bUseModValue, true);
                break;
            }
            case AMMO_ID: {
                ItemChange* pchangeItem = pCurrentProcess->GetCurrentAmmo();
                if (pchangeItem && pchangeItem->GetContainerObject() != apObject) {
                    UnEquipObject(
                        pchangeItem->GetContainerObject(),
                        1,
                        pchangeItem->GetExtraDataList()->GetItem(),
                        false,
                        false,
                        true
                    );
                }
                break;
            }
            default: break;
        }
    }
    pinventory = InventoryChanges::GetInventoryChanges(this);
    pinventory->AddWorn(apObject, aiNumber, this, apExtra, bleft, abForceEquip);
    if (IsNpc() && this != pPlayer && apObject->GetFormType() != WEAP_ID) {
        BipedAnim* pbiped = GetBiped();
        if (pbiped) {
            static_cast<TESNPC*>(GetObjectReference())->InitWornObject(this, pbiped, apObject);
        }
    }
    // Reads the form type again and drops it.
    apObject->GetFormType();
    EnchantmentItem* penchant = TESEnchantableForm::GetFormEnchanting(apObject);
    if (penchant) {
        penchant->Preload(false);
    }
    if (BSShaderManager::IsRefractionEnabled() && IsRefractive()) {
        SetRefraction(true, GetExtra()->GetRefractionPropertyExtra()->fRefractionPower);
    }
    return true;
}

void Actor::DoTrap(TrapEntry* apTrap, TargetEntry* apTarget) {
    uint32_t itrapHasHitID;
    alignas(16) ImpactMixer::COLLISION_SOUND collisionData;
    NiPoint3 kheightOffset;
    CFilter ifilt;
    hkVector4 trapLoc;
    hkVector4 trapVel;
    alignas(16) uint32_t itrapDeathPushBackID;
    uint32_t itrapContinuousID;
    uint32_t itrapMinVelID;
    alignas(16) uint32_t itrapPushBackID;
    uint32_t ilevelDamageID;
    uint32_t itrapDamageID;
    float fdamage = 0.0f;
    float fpushBack = 0.0f;
    float fdeathPushBack = 0.0f;
    float fminVelocity = 0.0f;
    bool btrapContinuous = false;
    float fvelScale = 1.0f;
    TESObjectREFR* ptrapRef = apTrap->pRef;
    ExtraDataList* ptrapExtra = &ptrapRef->m_Extra;
    Script* pscript = ptrapExtra->GetScript();
    if (this != pPlayer || (!ptrapRef || ptrapRef != pPlayer->GetGrabObject())) {
        if (pscript) {
            ScriptLocals* plocals = ptrapExtra->GetScriptLocals();
            if (plocals) {
                if (pscript->FindVariable("fTrapDamage", &itrapDamageID)) {
                    fdamage = plocals->GetVariable(itrapDamageID, nullptr);
                }
                if (pscript->FindVariable("fLevelledDamage", &ilevelDamageID)) {
                    double dlevel = static_cast<double>(this->GetLevel());
                    float flevelledDamage = plocals->GetVariable(ilevelDamageID, nullptr);
                    fdamage = flevelledDamage * dlevel + fdamage;
                }
                if (pscript->FindVariable("fTrapPushBack", &itrapPushBackID)) {
                    fpushBack = plocals->GetVariable(itrapPushBackID, nullptr);
                }
                if (pscript->FindVariable("fTrapMinVelocity", &itrapMinVelID)) {
                    fminVelocity = plocals->GetVariable(itrapMinVelID, nullptr);
                    if (fminVelocity < 0.0) {
                        fminVelocity = 0.0f;
                    }
                    else {
                        fminVelocity = NI2HK(fminVelocity);
                    }
                }
                if (pscript->FindVariable("bTrapContinuous", &itrapContinuousID)) {
                    btrapContinuous = plocals->GetVariable(itrapContinuousID, nullptr) != 0.0;
                }
                if (pscript->FindVariable("fTrapDeathPushBack", &itrapDeathPushBackID)) {
                    fdeathPushBack = plocals->GetVariable(itrapDeathPushBackID, nullptr);
                }
                else {
                    fdeathPushBack = fpushBack / 10.0;
                }
            }
        }
        if (btrapContinuous || !static_cast<uint8_t>(apTarget->GetHit())) {
            bhkCharacterController* pcon = this->GetCharController();
            hkpRigidBody* ptrapRB = hkpGetRigidBody(apTrap->pCollidable);
            if (ptrapRB) {
                const hkVector4* ptrapPos;
                if (static_cast<uint8_t>(apTarget->GetHit())) {
                    trapVel = ptrapRB->getLinearVelocity();
                    ptrapPos = &ptrapRB->getTranslation();
                }
                else {
                    trapVel = apTarget->VelocityAtHit;
                    ptrapPos = &apTarget->PositionAtHit;
                }
                trapLoc = *ptrapPos;
                if (fminVelocity == 0.0 || fminVelocity <= static_cast<double>(trapVel.length3())) {
                    if (pcon) {
                        hkVector4 targetVel;
                        targetVel = pcon->GethkLinearVelocity();
                        targetVel.sub4(trapVel);
                        float fmax = fminVelocity == 0.0 ? 10.0f : fminVelocity;
                        double drelativeSpeed = targetVel.length3();
                        fvelScale = drelativeSpeed / fmax;
                        if (fvelScale > 2.0) {
                            fvelScale = 2.0f;
                        }
                    }
                }
                else {
                    fdamage = 0.0f;
                }
            }
            else {
                if (static_cast<uint8_t>(apTarget->GetHit())) {
                    // The trap's collidable has no rigid body: its position is its motion's transform.
                    trapLoc = apTrap->pCollidable->m_motion.pMotion->getTranslation();
                }
                else {
                    trapLoc = apTarget->PositionAtHit;
                }
                NI2HK(trapVel, this->GetLocationOnReference());
                trapVel.sub4(trapLoc);
                float fhalfHeight = this->GetHeight() / 2.0;
                trapVel(2) = NI2HK(fhalfHeight) + trapVel(2);
            }
            bool bwasAlive = !this->IsDead(false);
            ifilt.iFilter = apTrap->pCollidable->getCollisionFilterInfo();
            if (ifilt.GetLayer() == L_CLOUDTRAP) {
                float fresistance = this->GetActorFloatValue(ActorValue::POISON_RESISTANCE) / 100.0;
                fresistance = 1.0 - BSMIN(fresistance, 1.0f);
                fdamage = fdamage * fresistance;
            }
            else {
                float fresistance = 1.0 - BSMIN(0.0f, 1.0f);
                fdamage = fdamage * fresistance;
            }
            if (fdamage > 0.0) {
                alignas(16) NiPoint3 sourcePos;
                if (btrapContinuous) {
                    double ddelta = appTimer.fDelta;
                    fdamage = ddelta * fvelScale * fdamage;
                }
                this->DoDamage(fdamage, 0.0f, nullptr);
                float fhalfHeight = this->GetHeight() * 0.5;
                kheightOffset = NiPoint3::UNIT_Z * fhalfHeight;
                NiPoint3 targetPos = this->Get3D()->GetWorldTranslate() + kheightOffset;
                HK2NI(sourcePos, trapLoc);
                if (ifilt.GetLayer() != L_CLOUDTRAP) {
                    HitData data;
                    data.pSourceRef = nullptr;
                    data.kHitLocation = targetPos;
                    data.kHitDirection = targetPos - sourcePos;
                    data.pAggressor = nullptr;
                    data.eDamageLimb = BGSBodyPart::LIMB_NONE;
                    this->CreateBlood(fdamage + fdamage, &data);
                    if (!btrapContinuous && ptrapRB && apTarget->eMaterialAtHit != BHK_MATERIAL_MAX) {
                        NiPoint3 location;
                        location = HK2NI(location, trapLoc);
                        bhkRigidBody* pbody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(ptrapRB));
                        ::new (&collisionData) NiBound;
                        collisionData.fFriction = 0.5f;
                        collisionData.pBody1 = nullptr;
                        collisionData.pBody2 = pbody;
                        collisionData.cMaterialType1 = BHK_MATERIAL_MAX;
                        collisionData.cMaterialType2 = static_cast<int8_t>(apTarget->eMaterialAtHit);
                        collisionData.Location = location;
                        collisionData.fVelocity = trapVel.length3() * hkSimdReal(3.0f);
                        // The sound ID is the sum of the actor's and the body's (32-bit) addresses.
                        collisionData.iID = static_cast<int32_t>(
                            reinterpret_cast<uintptr_t>(this) + reinterpret_cast<uintptr_t>(pbody)
                        );
                        ImpactMixer::PlayCollisionSound(collisionData);
                    }
                }
                this->TriggerPain(true, true);
                bool bjustKilled = bwasAlive && this->IsDead(false);
                if (fpushBack > 0.0) {
                    if (fdeathPushBack > 200.0) {
                        fdeathPushBack = 200.0f;
                    }
                    if (!bjustKilled || this == pPlayer) {
                        if (pcon) {
                            if (fpushBack > 1000.0) {
                                fpushBack = 1000.0f;
                            }
                            if (btrapContinuous) {
                                double ddelta = appTimer.fDelta;
                                fpushBack = ddelta * fvelScale * fpushBack;
                            }
                            if (apTrap->pCollidable) {
                                NiPoint3 dir;
                                HK2NI(dir, trapVel);
                                dir.Unitize();
                                dir *= fpushBack;
                                pcon->SetVelocityModifier(dir, 0.5f);
                            }
                        }
                    }
                    else {
                        bhkWorld::SetMotion(this->Get3D(), hkpMotion::MOTION_DYNAMIC, true, false, true);
                        bhkForceController::RemoveForce(this->Get3D());
                        trapVel.normalize3();
                        hkSimdReal koffset(1024.0f);
                        trapLoc.subMul4(koffset, trapVel);
                        TESHavokUtilities::AddExplosionImpulse(this->Get3D(), trapLoc, fdeathPushBack, nullptr);
                    }
                }
                if (pscript) {
                    ScriptLocals* plocals = ptrapExtra->GetScriptLocals();
                    if (plocals && pscript->FindVariable("bTrapHasHit", &itrapHasHitID)) {
                        plocals->SetVariable(itrapHasHitID, 1.0);
                    }
                }
            }
            apTarget->SetHit(true);
        }
    }
}

// Adds aicount of apObj (the instance apExtra) to the actor's inventory: the player reads a note instead; a
// reference instance becomes the actor's and the actors targeting it drop it; an NPC outside barter equips its best
// new weapon, reloads a weapon that had no ammo, and its combat controller rescans the inventory.
void Actor::AddObjecttoContainer(TESBoundObject* apObj, ExtraDataList* apExtra, int32_t aicount) {
    if (apObj->GetFormType() == NOTE_ID && this == pPlayer) {
        pPlayer->AddNote(static_cast<BGSNote*>(apObj), true);
        delete apExtra;
        return;
    }
    if (apExtra && this == pPlayer) {
        TESForm* pOwner = apExtra->GetOwner();
        if (pOwner == pPlayer->GetObjectReference()) {
            apExtra->RemoveOwnership();
        }
    }
    if (apExtra) {
        TESObjectREFR* pRef = apExtra->GetReferencePointer();
        if (pRef) {
            uint32_t uiID = pRef->GetFormID();
            pRef->GetExtra()->SetReferencePointer(this);
            pRef->AddChange(CHANGE_OBJECT_EXTRA_ITEM_DATA);
            BSSimpleList<Actor*>* pIter = reinterpret_cast<BSSimpleList<Actor*>*>(
                ReferenceProcessLists.GetActorTargetinganObject(uiID, this)
            );
            BSSimpleList<Actor*>* pList = pIter;
            while (pIter && pIter->GetItem()) {
                Actor* pActor = pIter->GetItem();
                if (pActor && pActor->GetCurrentPackageTarget()) {
                    pActor->SetCurrentTarget(this);
                }
                else {
                    pActor->SetCurrentTarget(nullptr);
                }
                pIter = pIter->GetNext();
            }
            if (pList) {
                pList->RemoveAll();
                delete pList;
            }
        }
    }
    AddBoundObjectItem(apObj, apExtra, aicount);
    if (IsNpc() &&
        pPlayer != this &&
        !Interface::ContainerIsBarter() &&
        (!Interface::GetContainerMode() || Interface::GetContainerMode() == 3)) {
        ItemChange* pBestWeapon = nullptr;
        if (apObj->GetFormType() == WEAP_ID) {
            pBestWeapon = GetBestWeapon(COMBAT_WEAPON_TYPE_COUNT);
        }
        if (pBestWeapon && !IsDead(false) && !IsInCombat()) {
            if (Interface::IsInMenuMode() && IsPlayerTeammate()) {
                EquipObject(pBestWeapon->GetContainerObject(), 1, nullptr, true, false, true);
            }
            else {
                QueueEquipObject(pBestWeapon->GetContainerObject(), 1, nullptr, true, false, true);
            }
            delete pBestWeapon;
        }
        else if (pBestWeapon) {
            delete pBestWeapon;
        }
    }
    if (pCurrentProcess && !pCurrentProcess->GetCurrentAmmo() && pCurrentProcess->GetCurrentWeapon()) {
        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetContainerObject());
        if (apObj == pWeapon->GetCurrentAmmo(this) && (this == pPlayer || !IsInDialoguewithPlayer())) {
            bool bRegenMod = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_AMMO_REGEN_SECONDS);
            pWeapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetContainerObject());
            if (pWeapon->GetAmmoRegenRate(bRegenMod) <= 0.0) {
                // The arguments are evaluated right to left.
                bool bUseModValue = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(
                    WEAPON_MOD_INCREASE_CLIP_SIZE
                );
                bool bDrawn = IsWeaponDrawn();
                pWeapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetContainerObject());
                ReloadWeaponNV(pWeapon, bDrawn, bUseModValue, false);
            }
        }
    }
    CombatController* pCombatController = GetCombatController();
    if (pCombatController) {
        pCombatController->SetInventoryDirty(true);
    }
}

NiPoint3 Actor::GetLookingAtLocation() {
    BipedAnim* pbiped = GetBiped();
    NiAVObject* phead = nullptr;
    NiPoint3 kLocation = NiPoint3::ZERO;
    if (pbiped) {
        phead = pbiped->GetParentBone(BIPED_BONE_HEAD);
    }
    if (!phead) {
        NiObjectNET* p3D = Get3D();
        if (p3D) {
            NiControllerManager* pcontrollerManager = static_cast<NiControllerManager*>(
                p3D->GetController(&NiControllerManager::ms_RTTI)
            );
            if (pcontrollerManager) {
                NiAVObjectPalette* ppalette = pcontrollerManager->GetObjectPalette();
                if (ppalette) {
                    phead = ppalette->GetAVObject(*FixedStrings::GetBip01_Head());
                }
            }
        }
    }
    if (phead) {
        kLocation += phead->GetWorldTranslate();
        if (pRagdollController) {
            NiPoint3 keyeOffset;
            if (pRagdollController->GetEyeOffset(keyeOffset)) {
                kLocation.z = keyeOffset.z;
            }
        }
    }
    else {
        kLocation.z = GetHeight() * 0.8999999761581421 + kLocation.z;
        kLocation += GetLocationOnReference();
    }
    return kLocation;
}

MagicCaster* Actor::GetMagicCaster() {
    return static_cast<MagicCaster*>(this);
}

MagicTarget* Actor::GetMagicTarget() {
    return static_cast<MagicTarget*>(this);
}

TESActorBase* Actor::GetTemplateActorBase() {
    return pTemplateActorBase;
}

// Loads the 3D as a mobile object, stores the process's damage nodes of the base's body part data, turns off self
// shadowing when the setting is off and queues the hurt locomotion of a crippled actor.
NiAVObject* Actor::Load3D(bool abBackgroundLoading) {
    NiAVObject* pNode = MobileObject::Load3D(abBackgroundLoading);
    if (!pNode) {
        return pNode;
    }
    if (pCurrentProcess) {
        pCurrentProcess->StoreDamageNodes(static_cast<NiNode*>(pNode), GetActorBase()->GetBodyPartData());
    }
    if (!Renderer::bActorSelfShadowing.Bool()) {
        pNode->SetShadowReceiver(false);
    }
    if (IsCrippled()) {
        QueueReplacementLocomotion(true, nullptr);
    }
    return pNode;
}

Animation* Actor::GetAnimation() {
    if (pCurrentProcess) {
        return pCurrentProcess->GetAnimation();
    }
    return nullptr;
}

// A swimming actor's position is a copy, in a ring of five, with z lowered to just under the water surface.
const NiPoint3& Actor::GetLocationOnReference() {
    if (!IsSwimming() || GetRelevantWaterHeight() == -3.4028234663852886e+38) {
        return *QLocationOnReference();
    }
    if (++LastPositionID >= 5) {
        LastPositionID = 0;
    }
    CachedPositions[LastPositionID] = *QLocationOnReference();
    TESObjectCELL* pCell = GetParentCell();
    if (pCell && pCell->HasWater()) {
        float fWaterHeight = GetRelevantWaterHeight() - 5.0;
        float fHeight = GetHeight() * 0.75;
        if (fWaterHeight > CachedPositions[LastPositionID].z + fHeight) {
            CachedPositions[LastPositionID].z = CachedPositions[LastPositionID].z + fHeight;
        }
        else {
            CachedPositions[LastPositionID].z = fWaterHeight;
        }
    }
    return CachedPositions[LastPositionID];
}

void Actor::SetActionComplete(bool abValue) {
    if (pCurrentProcess) {
        pCurrentProcess->SetCurrentActionComplete(abValue);
        pCurrentProcess->ClearMovementStoped();
    }
}

void Actor::SetMovementComplete(bool abValue) {
    if (pCurrentProcess) {
        pCurrentProcess->SetCurrentMovementComplete(abValue);
    }
}

// Refills the inventory as TESObjectREFR does, then re-equips the base's default items when the actor has 3D. Unless
// the old save loader is loading, the actor's 3D is flagged for an update first (or detached without a middle-high
// or high process); the path manager forgets the old inventory.
void Actor::ResetInventory(bool abLeveledOnly) {
    if (!pSaveLoadGameOLD->GetLoading()) {
        BaseProcess* pProcess = nullptr;
        if (GetCurrentAIProcess() && GetCurrentAIProcess()->GetProcessLevel() <= PROCESS_TYPE_MIDDLE_HIGH) {
            pProcess = GetCurrentAIProcess();
        }
        if (pProcess) {
            pProcess->Set3DUpdateFlag(RESET_MODEL);
        }
        else {
            Set3D(nullptr, true);
        }
    }
    InventoryChanges* pChanges = GetExtra()->GetContainerChanges();
    PathManager::QInstance()->ClearInventoryPointersFromCurrentRequests(pChanges);
    TESObjectREFR::ResetInventory(abLeveledOnly);
    if (!Get3D()) {
        return;
    }
    TESBoundObject* pBaseForm = GetObjectReference();
    TESNPC* pNPC = nullptr;
    TESCreature* pCreature = nullptr;
    uint8_t eType = pBaseForm->GetFormType();
    if (eType == NPC__ID) {
        pNPC = static_cast<TESNPC*>(pBaseForm);
    }
    else if (eType == CREA_ID) {
        pCreature = static_cast<TESCreature*>(pBaseForm);
    }
    bool bArmor = true;
    bool bHasWeapons = true;
    TESPackage* pPackage = GetCurrentPackage();
    if (pPackage && pPackage->GetForceUnequipWeapon()) {
        bHasWeapons = false;
    }
    if (pNPC) {
        pNPC->InitDefaultWorn(this, bArmor, bHasWeapons, false, true);
    }
    else if (pCreature) {
        pCreature->InitDefaultWorn(this, bArmor, bHasWeapons, true);
    }
}

NiAVObject* Actor::GetFireNode() {
    if (!pCurrentProcess) {
        return nullptr;
    }
    NiAVObject* pcachedFireNode = pCurrentProcess->GetFireNode();
    if (pcachedFireNode) {
        return pcachedFireNode;
    }
    NiNode* currentNode;
    if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
        currentNode = static_cast<NiNode*>(pPlayer->GetCurrent3D());
    }
    else {
        currentNode = Get3D();
    }
    if (!currentNode) {
        return nullptr;
    }
    TESObjectWEAP* pCurrentWeapon = GetCurrentWeapon();
    NiNode* fireNode;
    if (pCurrentWeapon) {
        fireNode = static_cast<NiNode*>(pCurrentWeapon->GetFireNode(currentNode));
    }
    else {
        fireNode = static_cast<NiNode*>(BSUtilities::GetObjectByName(currentNode, "ProjectileNode"));
    }
    pCurrentProcess->SetFireNode(fireNode);
    return fireNode;
}

void Actor::SetFireNode(NiAVObject* apFireNode) {
    if (pCurrentProcess) {
        pCurrentProcess->SetFireNode(static_cast<NiNode*>(apFireNode));
    }
}

bool Actor::IsDead(bool abNotEssential) {
    if (abNotEssential) {
        return GetLifeState() == ACTOR_LIFE_STATE_DEAD || GetLifeState() == ACTOR_LIFE_STATE_DYING;
    }
    return GetLifeState() == ACTOR_LIFE_STATE_DEAD ||
        GetLifeState() == ACTOR_LIFE_STATE_DYING ||
        GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN;
}

bool Actor::IsKnockedOut() {
    return pCurrentProcess && pCurrentProcess->GetKnockState() != 0;
}

bool Actor::IsParalyzed() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        return pCurrentProcess->GetParalysis(this) > 0.0;
    }
    return CalculateCachedParalysis() > 0.0;
}

// Moves the actor to the high process. Already high (or without a process): only re-equips the worn weapon on the 3D
// and draws it when alert or the package says so. Otherwise: lists the actor as alive and high, moves an actor that
// has no placement yet (or comes from the low processes) to a safe point, replaces the process with a HighProcess
// copy of it, re-equips, and lets a dead actor without saved ragdoll state fall into its death pose. The "Lily"
// teammate creature gets its default outfit back.
bool Actor::MovetoHigh() {
    if (!pCurrentProcess || pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
        if (!pCurrentProcess || !Get3D()) {
            return true;
        }
        InventoryChanges* pinventory = GetExtra()->GetContainerChanges();
        if (pinventory) {
            if (IsPlayerTeammate() && IsCreature() && std::strstr(GetFullName(), "Lily")) {
                pCurrentProcess->SetCurrentWeapon(pinventory->GetWornItem(BO_WEAPON, false), Get3D(), true);
            }
            else {
                pCurrentProcess->SetCurrentWeapon(pinventory->GetWornItem(BO_WEAPON, false), nullptr, true);
            }
        }
        if (pCurrentProcess->GetAlert() ||
            (pCurrentProcess->GetCurrentPackage() && pCurrentProcess->GetCurrentPackage()->GetDrawWeaponFlag())) {
            pCurrentProcess->SetWantWeaponDrawn(true);
        }
        return true;
    }

    bool bloading = pSaveLoadGame->GetSaveGameLoading() || IsStillLoading();
    bDoNotRunSayToCallBack = false;
    if (GetCurrentProcessType() != PROCESS_TYPE_MIDDLE_HIGH) {
        GetCurrentAIProcess()->ClearFurniture(this);
    }
    bool bInCombat = IsInCombat();
    if (!bInCombat && pCurrentProcess && !bloading) {
        pCurrentProcess->CheckforNewPackage(this, true);
    }
    // The result of IsFleeing is never used.
    IsFleeing(false);
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    if (!IsDead(false)) {
        ReferenceProcessLists.InsertAliveActor(this);
    }
    PROCESS_TYPE eCurrentList = PROCESS_TYPE_LOW;
    TESPackage* ppack = nullptr;
    if (pCurrentProcess) {
        eCurrentList = pCurrentProcess->GetProcessLevel();
        ppack = pCurrentProcess->GetCurrentPackage();
        ReferenceProcessLists.RemoveReference(this, eCurrentList);
    }
    bool bfindpoint = false;
    bool bforceMove = false;
    // An x angle of FLT_MAX marks a reference that was never placed.
    if (GetAngleOnReference().x == 3.4028234663852886e+38) {
        bfindpoint = true;
        SetAngleOnReferenceX(0.0f);
        if (GetBaseActorFloatValue(ActorValue::HEALTH) > 0.0) {
            bforceMove = true;
        }
    }
    else if (GetData()->kPosition == NiPoint3::ZERO) {
        bfindpoint = true;
    }
    if (!bfindpoint &&
        !pSaveLoadGame->GetSaveGameLoading() &&
        eCurrentList >= PROCESS_TYPE_MIDDLE_LOW &&
        eCurrentList <= PROCESS_TYPE_LOW) {
        bfindpoint = true;
    }
    bool bneedsupdate = true;
    if (bfindpoint && MoveToSafePoint(bforceMove)) {
        bneedsupdate = false;
    }
    if (bneedsupdate) {
        UpdateCurrentDebugGeometry();
    }

    HighProcess* pNewProcess = new HighProcess;
    pNewProcess->Copy(pCurrentProcess);
    BaseProcess* pOldProcess = pCurrentProcess;
    if (pOldProcess) {
        delete pOldProcess;
    }
    pCurrentProcess = pNewProcess;
    if (Get3D()) {
        pCurrentProcess->StoreDamageNodes(Get3D(), GetActorBase()->GetBodyPartData());
    }
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, false, false, nullptr);
    ReferenceProcessLists.kUpdateLock.Unlock();
    pNewProcess->InitBoneLOD(this);
    pNewProcess->ClearFaceAnimationData();

    InventoryChanges* pinventory = GetExtra()->GetContainerChanges();
    if (pinventory) {
        if (IsPlayerTeammate() && std::strstr(GetFullName(), "Lily") && IsCreature()) {
            static_cast<TESCreature*>(GetObjectReference())->InitDefaultWorn(this, true, true, true);
        }
        ItemChange* pchange = pinventory->GetWornItem(BO_WEAPON, false);
        if (pinventory && eCurrentList != PROCESS_TYPE_MIDDLE_HIGH) {
            pCurrentProcess->SetCurrentWeapon(pchange, nullptr, false);
        }
    }
    if (pCurrentProcess->GetAlert() || (ppack && ppack->GetDrawWeaponFlag())) {
        pCurrentProcess->SetWantWeaponDrawn(true);
    }
    pCurrentProcess->SetLastSeenLocation();
    UnlockLockDoorsProcedure();
    if (bInCombat) {
        HandleCombatProcessLevelChange(eCurrentList, PROCESS_TYPE_HIGH);
    }

    // Calls TESObjectREFR::Get3D directly.
    NiAVObject* p3dObj = TESObjectREFR::Get3D();
    bool bshouldHaveRagDoll = true;
    Creature* pcreature = nullptr;
    if (GetObjectReference()->GetFormType() == CREA_ID) {
        pcreature = static_cast<Creature*>(this);
    }
    if (pcreature && p3dObj && !pcreature->HasRagDoll()) {
        bshouldHaveRagDoll = false;
    }
    if (IsDead(true) && bshouldHaveRagDoll && !GetExtra()->GetRagDollData() && !GetExtra()->GetSavedHavokData()) {
        bDeadFlag = true;
    }
    pNewProcess->SetupPhantom(this);
    MoveHavok(false);
    if (bDeadFlag && p3dObj) {
        DetachCharController();
        Animation* panim = Actor::GetAnimation();
        if (pRagdollController) {
            pRagdollController->DisableRagdollAnim(true);
        }
        if (pPenetrationDetection) {
            CFilter kFilter;
            GetCollisionFilter(&kFilter.iFilter);
            pPenetrationDetection->Initialize(p3dObj, kFilter.GetGroup());
        }
        if (panim && panim->GroupLoaded(ANIM_GROUP_DEATH)) {
            panim->ForceSection(AGS_WHOLE_BODY, ANIM_GROUP_DEATH, AGA_NONE, 0.0f, -1);
            BSAnimGroupSequence* pseq = panim->GetCurrentSequence(AGS_MOVEMENT);
            if (pseq) {
                if (!HasRagDoll()) {
                    pseq->SetOffset(pseq->GetEndKeyTime());
                    panim->Update(this, 0.0f, pseq->GetEndKeyTime());
                    panim->UpdateMovement(this);
                }
                else {
                    panim->Update(this, 0.0f, pseq->GetAnimGroup()->GetTime(AGA_STOP));
                    panim->UpdateMovement(this);
                    NiMatrix3 rot;
                    rot.MakeZRotation(GetAngleOnReference().z);
                    NiPoint3 dr(0.0f, 1.0f, 0.0f);
                    dr = rot * dr;
                    bhkBlendController::DoKnockDown(TESObjectREFR::Get3D(), dr, true, 0.0f, true);
                    AddChange(CHANGE_REFR_HAVOK_MOVE);
                }
            }
            DropAddOnObjects();
            SetLifeState(ACTOR_LIFE_STATE_DYING);
        }
        else if (bDeadFlag) {
            SetLifeState(ACTOR_LIFE_STATE_DYING);
            NiMatrix3 rot;
            rot.MakeZRotation(GetHeading(false));
            NiPoint3 dr(0.0f, 1.0f, 0.0f);
            dr = rot * dr;
            bhkBlendController::DoKnockDown(p3dObj, dr, true, 0.0f, true);
            AddChange(CHANGE_REFR_HAVOK_MOVE);
        }
        pCurrentProcess->ComputeLastTimeProcessed();
        bDeadFlag = false;
    }
    if (p3dObj) {
        UpdateAlpha();
    }

    BaseProcess* pproc = GetCurrentAIProcess();
    ppack = GetCurrentPackage();
    if (ppack && ppack->GetPackType() == PACKAGE_TYPE_TRAVEL) {
        pproc->SetProcedureIndexRunning(0);
    }
    if (!IsDead(false) && (eCurrentList == PROCESS_TYPE_LOW || eCurrentList == PROCESS_TYPE_MIDDLE_LOW)) {
        CastPermanentMagic(false);
    }
    // A follower of an actor that is not high is taken out of the high list while unmarked.
    if (pproc && pproc->IsAFollower() && pproc->GetTarget() && pproc->GetTarget()->IsActor()) {
        Actor* pact = static_cast<Actor*>(pproc->GetTarget());
        if (pact && pact->GetCurrentProcessType() != PROCESS_TYPE_HIGH) {
            pproc->SetFollower(false);
            ReferenceProcessLists.RemoveReference(this, PROCESS_TYPE_HIGH);
            pproc->SetFollower(true);
        }
    }
    if (Get3D()) {
        // Reads the 3D before it fetches the scene node.
        NiNode* p3D = Get3D();
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->AddShadowLight(p3D);
    }
    pActorMover->OnMoveToHigh();
    if (!GetCurrentAIProcess()->GetRunOncePackage() && !GetCurrentAIProcess()->IsPackageDone(this) && !bloading) {
        TESPackage* ppack = GetCurrentAIProcess()->GetCurrentPackage();
        if (ppack) {
            GetCurrentAIProcess()->CheckAndStartCombatControllerForPackage(this, ppack, IsInCombat());
        }
    }
    pCurrentProcess->CheckCachedValueFlags(this);
    return true;
}

// Moves the actor to the low process: leaves dialogue and combat that cannot go on there, drops the actor from the
// high-process lists and its permanent magic, finishes a dying actor, and replaces the process with a LowProcess copy
// of it. Already in the low process: nothing to do. A local flag that is always false is tested before listing the
// actor, so it always inserts at index 1.
bool Actor::MovetoLow() {
    if (pCurrentProcess && pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_LOW) {
        return true;
    }
    if (pCurrentProcess && pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
        SetLastSeenTime();
        static_cast<HighProcess*>(pCurrentProcess)->RemovePhantom();
        BSShaderAccumulator* pAccumulator = BSShaderManager::GetAccumulator();
        if (pAccumulator) {
            pAccumulator->RemoveBoundVolume(GetFormID());
        }
    }
    cCurrentSitSleepState = SIT_SLEEP_NORMAL;
    if (GetCurrentAIProcess()) {
        TESPackage* pRunning = GetCurrentAIProcess()->GetPackageThatIsRunning();
        if (pRunning &&
            pRunning->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE &&
            GetCurrentAIProcess()->GetTalkingtoPC()) {
            EndInterruptPackage(false);
        }
    }
    if (IsInCombat(false) && !GetRunsInLow()) {
        StopCombat(nullptr);
    }
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    ReferenceProcessLists.RemoveAliveActor(this);
    ReferenceProcessLists.RemoveActorCloseToPlayer(this);
    if (GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
        SetLifeState(ACTOR_LIFE_STATE_ALIVE);
        pCurrentProcess->SetEssentialDownTimer(0.0f);
        RestoreFullHealthAndConditions();
        pCurrentProcess->ClearKnockState(this);
    }
    if (pCurrentProcess &&
        pCurrentProcess->GetCurrentPackage() &&
        pCurrentProcess->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE &&
        pCurrentProcess->GetTarget() != pPlayer) {
        EndInterruptPackage(false);
    }
    if (IsAlarmed() && !IsActoraGuard() && (!pPlayer->GetParentCell() || !pPlayer->GetParentCell()->GetInterior())) {
        EndInterruptPackage(false);
    }

    PROCESS_TYPE eOldLevel = pCurrentProcess->GetProcessLevel();
    DispelPermanentMagic();
    if (eOldLevel == PROCESS_TYPE_MIDDLE_HIGH || eOldLevel == PROCESS_TYPE_HIGH) {
        UnloadPermanentMagic();
    }
    MagicTarget::Dispel();
    if (!pDataHandler->GetClearingData() && !pSaveLoadGameOLD->GetLoading()) {
        ReferenceProcessLists.RemoveActorInDetectionLists(this);
    }
    if (IsDying()) {
        DoDeathStuff();
        pCurrentProcess->FinishDying(this);
        RunScript();
    }
    else if (IsDead(false) && pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
        SetHavokDeath(true);
    }
    ReferenceProcessLists.RemoveReference(this, eOldLevel);
    bool bContinuingPCNear = IsContinuingPackagePCNear();
    bool bInCombat = IsInCombat();
    if (pCurrentProcess->GetCurrentPackage() &&
        pCurrentProcess->GetCurrentPackage()->IsInterruptPackage() &&
        !bInCombat &&
        !IsAlarmed()) {
        EndInterruptPackage(false);
    }
    LowProcess* pLowProcess = new LowProcess;
    if (pCurrentProcess->GetCurrentFurniture()) {
        pCurrentProcess->SetFurniture(nullptr, 0, nullptr);
    }
    pLowProcess->Copy(pCurrentProcess);
    BaseProcess* pOldProcess = pCurrentProcess;
    if (pOldProcess) {
        delete pOldProcess;
    }
    pCurrentProcess = pLowProcess;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_LOW, true, false, nullptr);
    if (!bInCombat && bContinuingPCNear) {
        pCurrentProcess->CheckforNewPackage(this, false);
    }
    if (pDataHandler && !pDataHandler->GetClearingData()) {
        if (bContinuingPCNear) {
            pCurrentProcess->CheckforNewPackage(this, false);
        }
        if (bInCombat) {
            HandleCombatProcessLevelChange(eOldLevel, PROCESS_TYPE_LOW);
        }
        else if (
            (IsQuestObject() && bForceUpdateQuestTarget) ||
            ((IsFollowing() || IsEscorterBehind()) && GetCurrentAIProcess()->GetTarget() == pPlayer)
        ) {
            float fLastHour = static_cast<float>(appCalendar.GetHour() - 1.0);
            pCurrentProcess->SetLastTimeProcessed(fLastHour);
            ReferenceProcessLists.RemoveReference(this, PROCESS_TYPE_LOW);
            ReferenceProcessLists.AddReference(this, PROCESS_TYPE_LOW, false, false, nullptr);
        }
        bForceUpdateQuestTarget = true;
    }
    ReferenceProcessLists.kUpdateLock.Unlock();
    if (eOldLevel == PROCESS_TYPE_HIGH) {
        pActorMover->OnMoveToLow();
    }
    pCurrentProcess->CheckCachedValueFlags(this);
    RemoveChange(CHANGE_ACTOR_TEMP_MODIFIERS);
    return true;
}

bool Actor::MovetoMiddleLow() {
    ReferenceProcessLists.RemoveActorCloseToPlayer(this);
    cCurrentSitSleepState = SIT_SLEEP_NORMAL;
    if (pCurrentProcess && pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_LOW) {
        return true;
    }
    if (pCurrentProcess && !pCurrentProcess->eLevel) {
        SetLastSeenTime();
        static_cast<HighProcess*>(pCurrentProcess)->HighProcess::RemovePhantom();
        BSShaderAccumulator* paccum = BSShaderManager::GetAccumulator();
        if (paccum) {
            paccum->RemoveBoundVolume(iFormID);
        }
    }

    if (pCurrentProcess) {
        TESPackage* pPack = pCurrentProcess->GetPackageThatIsRunning();
        if (pPack && pPack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
            if (pCurrentProcess->GetTalkingtoPC()) {
                EndInterruptPackage(false);
            }
        }
    }
    PROCESS_TYPE eCurrentList = pCurrentProcess->eLevel;
    DispelPermanentMagic();
    if (eCurrentList == PROCESS_TYPE_MIDDLE_HIGH || eCurrentList == PROCESS_TYPE_HIGH) {
        UnloadPermanentMagic();
    }
    Dispel();
    if (IsDying()) {
        DoDeathStuff();
        pCurrentProcess->FinishDying(this);
        RunScript();
    }
    else if (IsDead(false) && !pCurrentProcess->eLevel) {
        SetHavokDeath(true);
    }
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    ReferenceProcessLists.RemoveReference(this, eCurrentList);
    ReferenceProcessLists.RemoveAliveActor(this);
    if (!pDataHandler->GetClearingData()) {
        if (!pSaveLoadGameOLD->GetLoading()) {
            ReferenceProcessLists.RemoveActorInDetectionLists(this);
        }
        if (pCurrentProcess && pCurrentProcess->GetCurrentPackage()) {
            if (pCurrentProcess->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_WANDER) {
                pCurrentProcess->SetProcedureIndexRunning(0);
            }
        }
    }
    if (GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
        SetLifeState(ACTOR_LIFE_STATE_ALIVE);
        pCurrentProcess->SetEssentialDownTimer(0.0f);
        RestoreFullHealthAndConditions();
        pCurrentProcess->ClearKnockState(this);
    }
    bool bContinuingPCNear = IsContinuingPackagePCNear();
    MiddleLowProcess* pNewProcess = new MiddleLowProcess;
    bool bInCombat = IsInCombat();
    if (pCurrentProcess && pCurrentProcess->GetCurrentPackage()) {
        if (pCurrentProcess->GetCurrentPackage()->IsInterruptPackage() && !bInCombat && !IsAlarmed()) {
            EndInterruptPackage(false);
        }
    }

    pNewProcess->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = pNewProcess;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_LOW, true, false, nullptr);
    if (!bInCombat && bContinuingPCNear) {
        pCurrentProcess->CheckforNewPackage(this, false);
    }
    if (pDataHandler && !pDataHandler->GetClearingData()) {
        if (bContinuingPCNear) {
            pCurrentProcess->CheckforNewPackage(this, false);
        }
        if (bInCombat) {
            HandleCombatProcessLevelChange(eCurrentList, PROCESS_TYPE_MIDDLE_LOW);
        }
        else if (
            (IsQuestObject() && bForceUpdateQuestTarget) ||
            ((IsFollowing() || IsEscorterBehind()) && pCurrentProcess->GetTarget() == pPlayer)
        ) {
            float fLastTime = appCalendar.GetHour() - 0.5;
            pCurrentProcess->SetLastTimeProcessed(fLastTime);
            ReferenceProcessLists.RemoveReference(this, PROCESS_TYPE_MIDDLE_LOW);
            ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_LOW, false, false, nullptr);
        }
        bForceUpdateQuestTarget = true;
    }

    ReferenceProcessLists.kUpdateLock.Unlock();
    if (eCurrentList == PROCESS_TYPE_HIGH) {
        pActorMover->OnMoveToLow();
    }
    pCurrentProcess->CheckCachedValueFlags(this);
    return true;
}

// Moves the actor to a new middle-high process (none when it already has one): ends the in-game dialogue, the created
// run-once package and the alarm cases that should not carry over, finishes dying or revives an essential actor, gets
// it up from furniture it lost, then swaps the process under the update lock (copying the old one and deleting it),
// restores the weapon, restarts the package and the combat or follow bookkeeping, relocks the doors around a trespassed
// actor, recasts the permanent magic when it came from a low level and tells the mover when it came from high.
bool Actor::MovetoMiddleHigh() {
    ReferenceProcessLists.RemoveActorCloseToPlayer(this);
    if (pCurrentProcess && pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH) {
        return true;
    }
    if (pCurrentProcess && pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
        SetLastSeenTime();
        BSShaderAccumulator* pAccumulator = BSShaderManager::GetAccumulator();
        if (pAccumulator) {
            pAccumulator->RemoveBoundVolume(GetFormID());
        }
    }
    if (GetCurrentAIProcess()) {
        TESPackage* pRunning = GetCurrentAIProcess()->GetPackageThatIsRunning();
        if (pRunning &&
            pRunning->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE &&
            GetCurrentAIProcess()->GetTalkingtoPC()) {
            EndInterruptPackage(false);
        }
    }
    TESPackage* pRunOnce = pCurrentProcess->GetRunOncePackage();
    if (pRunOnce && pRunOnce->GetIsCreated()) {
        TESPackage* pPackage = pCurrentProcess->GetCurrentPackage();
        if (pPackage) {
            if (!IsAlarmed() &&
                pPackage->GetPCContinue() &&
                pPackage->GetServicesEnabled() &&
                GetParentCell() &&
                GetParentCell()->IsActorCellOwner(this)) {
                EndInterruptPackage(false);
            }
        }
        else if (IsAlarmed() && !IsActoraGuard() && !pPlayer->GetParentCell()->GetInterior()) {
            EndInterruptPackage(false);
        }
    }
    PROCESS_TYPE eOldLevel = pCurrentProcess->GetProcessLevel();
    if (IsDying()) {
        DoDeathStuff();
        pCurrentProcess->FinishDying(this);
        RunScript();
    }
    else if (IsDead(true) && pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
        SetHavokDeath(true);
    }
    else if (GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
        SetLifeState(ACTOR_LIFE_STATE_ALIVE);
        pCurrentProcess->SetEssentialDownTimer(0.0f);
        pCurrentProcess->ClearKnockState(this);
    }
    if (GetSitSleepState() &&
        ((GetSitSleepState() != SIT_SLEEP_IS_SITTING &&
          GetSitSleepState() != SIT_SLEEP_IS_SLEEPING &&
          pCurrentProcess->GetCurrentFurniture()) ||
         (GetCurrentPackage() && GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_GET_UP))) {
        pCurrentProcess->ProcessGetUpOneHour(this);
        pCurrentProcess->ClearRunOncePackage();
    }
    if (GetSitSleepState() &&
        GetSitSleepState() != SIT_SLEEP_IS_SITTING &&
        GetSitSleepState() != SIT_SLEEP_IS_SLEEPING &&
        !pCurrentProcess->GetCurrentFurniture()) {
        pCurrentProcess->SetSitSleepState(this, SIT_SLEEP_NORMAL, nullptr, 127);
        cCurrentSitSleepState = SIT_SLEEP_NORMAL;
    }
    if (!pDataHandler->GetClearingData() && !pSaveLoadGameOLD->GetLoading()) {
        ReferenceProcessLists.RemoveActorInDetectionLists(this);
    }
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    ReferenceProcessLists.RemoveReference(this, eOldLevel);
    ReferenceProcessLists.RemoveAliveActor(this);
    if (GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
        SetLifeState(ACTOR_LIFE_STATE_ALIVE);
        pCurrentProcess->SetEssentialDownTimer(0.0f);
        RestoreFullHealthAndConditions();
    }
    bool bContinuingPackage = IsContinuingPackagePCNear();
    BaseProcess* pNewProcess = new MiddleHighProcess;
    pNewProcess->Copy(pCurrentProcess);
    BaseProcess* pOldProcess = pCurrentProcess;
    delete pOldProcess;
    pCurrentProcess = pNewProcess;
    if (Get3D()) {
        BGSBodyPartData* pBodyPartData = GetActorBase()->GetBodyPartData();
        pCurrentProcess->StoreDamageNodes(Get3D(), pBodyPartData);
    }
    if (pCurrentProcess->GetAlert()) {
        SetWantWeaponDrawn(true);
    }
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_HIGH, true, false, nullptr);
    ReferenceProcessLists.kUpdateLock.Unlock();
    InventoryChanges* pChanges = GetExtra()->GetContainerChanges();
    if (pChanges) {
        ItemChange* pWeapon = pChanges->GetWornItem(BO_WEAPON, false);
        if (pChanges && eOldLevel != PROCESS_TYPE_HIGH) {
            pCurrentProcess->SetCurrentWeapon(pWeapon, nullptr, false);
        }
        else if (pWeapon) {
            delete pWeapon;
        }
    }
    if (pDataHandler && !pDataHandler->GetClearingData()) {
        if (bContinuingPackage) {
            pCurrentProcess->CheckforNewPackage(this, false);
        }
        if (IsInCombat()) {
            HandleCombatProcessLevelChange(eOldLevel, PROCESS_TYPE_MIDDLE_HIGH);
        }
        else if (
            (IsQuestObject() && bForceUpdateQuestTarget) ||
            ((IsFollowing() || IsEscorterBehind()) && GetCurrentAIProcess()->GetTarget() == pPlayer)
        ) {
            // Subtracts the double 0.5 and rounds the result to a float.
            float fLastTime = appCalendar.GetHour() - 0.5;
            pCurrentProcess->SetLastTimeProcessed(fLastTime);
            ReferenceProcessLists.RemoveReference(this, PROCESS_TYPE_MIDDLE_HIGH);
            ReferenceProcessLists.AddReference(this, GetCurrentProcessType(), false, false, nullptr);
        }
        bForceUpdateQuestTarget = true;
        if (IsProcessingTresPassAlarm()) {
            Actor* pTarget = nullptr;
            TESObjectREFR* pTargetRef = pCurrentProcess->GetTarget();
            if (pTargetRef && pTargetRef->IsActor()) {
                pTarget = static_cast<Actor*>(pTargetRef);
            }
            if (pTarget && !pTarget->IsTresPassing()) {
                EndInterruptPackage(false);
                const NiPoint3* pPoint2 = &GetLocationOnReference();
                const NiPoint3* pPoint1 = &GetLocationOnReference();
                TESObjectCELL* pCell = GetParentCell();
                // Passes the actor itself as the enumeration data.
                pDataHandler->EnumReferencesCloseToPoint(
                    pCell,
                    *pPoint1,
                    5000.0f,
                    *pPoint2,
                    5000.0f,
                    &LowProcess::LockDoorsatLocation,
                    this
                );
            }
        }
    }
    if (!IsDead(false) && (eOldLevel == PROCESS_TYPE_LOW || eOldLevel == PROCESS_TYPE_MIDDLE_LOW)) {
        CastPermanentMagic(false);
    }
    if (eOldLevel == PROCESS_TYPE_HIGH) {
        pActorMover->OnMoveToLow();
    }
    pCurrentProcess->CheckCachedValueFlags(this);
    return true;
}

bhkCharacterController* Actor::Move(float afDeltaTime, NiPoint3& arDeltaMove, uint32_t aiMoveDir) {
    if (IsInDialoguewithPlayer() && (aiMoveDir & 63) == 0) {
        return nullptr;
    }
    if (!pLoadedData) {
        return nullptr;
    }
    bhkCharacterController* pcc = MobileObject::Move(afDeltaTime, arDeltaMove, aiMoveDir);
    if (pcc && pcc->QHitDamage()) {
        float ffallDistance = pcc->GetFallDistance();
        if (fJumpFallHeightMin.Float() < static_cast<double>(ffallDistance)) {
            float fdamageDistance = ffallDistance - fJumpFallHeightMin.Float();
            float ffallingDamage = std::pow(fdamageDistance, fJumpFallHeightExponent.Float()) *
                fJumpFallHeightMult.Float();
            TESActorBase* pbase = GetActorBase();
            if (pbase && static_cast<TESCreature*>(pbase)->TESCreature::DoesFly()) {
                ffallingDamage = 0.0f;
            }

            if (ffallingDamage > 0.0) {
                if ((IsPc() || (!IsNpc() || (!GetWorldSpace() || GetWorldSpace()->IsAllowNPCFallDamage()))) &&
                    (IsPc() || !IsPlayerTeammate())) {
                    DoDamage(ffallingDamage, 0.0f, nullptr);
                }

                if (IsPc()) {
                    BSSoundHandle fallHandle;
                    if (ffallingDamage > static_cast<double>(fHardLandingDamageThreshold.Float())) {
                        fallHandle = BSAudio::QInstance().GetSoundHandleByName("FSTLandHardHeavy", 257u);
                    }
                    else {
                        fallHandle = BSAudio::QInstance().GetSoundHandleByName("FSTLandHardLight", 257u);
                    }
                    fallHandle.Play(false);
                    pcc->ClearHitDamage();
                    float flegdamage = ffallingDamage * fFallLegDamageMult.Float();
                    uint32_t uiRoll = Random();
                    if (uiRoll % 100 < iFallLegDamageChance.Int()) {
                        DamageModActorValue(
                            ActorValue::LEFT_MOBILITY_CONDITION,
                            -flegdamage,
                            static_cast<Actor*>(nullptr)
                        );
                    }
                    uiRoll = Random();
                    if (uiRoll % 100 < iFallLegDamageChance.Int()) {
                        DamageModActorValue(
                            ActorValue::RIGHT_MOBILITY_CONDITION,
                            -flegdamage,
                            static_cast<Actor*>(nullptr)
                        );
                    }
                }
                TriggerPain(true, true);
            }
        }
    }

    return pcc;
}

// The character controller of the jump, and the jump sound when the actor actually left the ground.
bhkCharacterController* Actor::Jump() {
    bhkCharacterController* pController = MobileObject::Jump();
    if (pController) {
        ImpactMixer::PlayJumpSound(this, 0);
    }
    return pController;
}

// The AI tick: the action timer, the merchant container's respawn day, the mover and magic updates, the package update
// (or the death handling), the single-hardware-thread magic update and the move to the parent cell's world space. The
// body is bracketed with the AI processing perf timer's two marks.
void Actor::Process(float afDelta) {
    if (GetDisabled() || !bProcessMe) {
        return;
    }
    AIProcessingProcess.Start();
    static_cast<MobileObject*>(this)->UpdateSoundCallBack();
    bShouldRotateToTrack = ShouldRotateToTrack(nullptr);
    float fSeconds = appTimer.GetSeconds();
    if (pVATS.GetMode() == VATS::VATS_PLAYBACK && this == pVATSTarget) {
        fSeconds = pVATS.GetTargetUpdateMult() * fSeconds;
    }
    if (fTimeronAction <= 0.0) {
        iActionValue = 0;
    }
    else {
        fTimeronAction = fTimeronAction - fSeconds;
    }
    if (GetCurrentAIProcess() && !IsDead(false)) {
        BaseProcess* pLastProcess = GetCurrentAIProcess();
        if (pLastProcess->GetProcessedDay() == appCalendar.GetDay()) {
            bContainerReset = false;
        }
        else {
            if (!bContainerReset) {
                int32_t iDayOfTheWeek = appCalendar.GetDayOfTheWeek();
                if (iDayOfTheWeek == static_cast<int32_t>(iMerchantRespawnDay1.UInt()) ||
                    iDayOfTheWeek == static_cast<int32_t>(iMerchantRespawnDay2.UInt())) {
                    ExtraDataList* pExtra = GetExtra();
                    TESObjectREFR* pMerchantContainer = pExtra->GetMerchantContainer();
                    if (pMerchantContainer) {
                        bool bRefRespawns = pMerchantContainer->GetRespawn();
                        pMerchantContainer->ResetInventory(!bRefRespawns);
                    }
                }
            }
            bContainerReset = true;
        }
    }

    BaseProcess* pProcess = GetCurrentAIProcess();
    if (pProcess->GetProcessLevel() || pPlayer->IsSleepingorResting()) {
        float fDelta = afDelta;
        if (pPlayer->IsSleepingorResting()) {
            fDelta = 3600.0f;
        }
        else if (IsInCombat()) {
            fDelta = 0.0f;
        }
        UpdateActorMover(fDelta);
        UpdateMagic(false, false);
    }
    if (!IsDead(false) &&
        !bDeadFlag &&
        ReferenceProcessLists.bRunScheduals &&
        GetLifeState() != ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
        if (bUseMovementBlockedPackage.Bool() &&
            !pCurrentProcess->IsRunningRunOnce() &&
            pActorMover &&
            pActorMover->IsMovementBlocked()) {
            InitiateMovementBlockedPackage();
        }

        char cLastDay = static_cast<char>(pCurrentProcess->GetProcessedDay());
        if (appCalendar.GetDay() != cLastDay) {
            ExtraDataList* pExtra = GetExtra();
            pExtra->RemoveInfoGeneral();
        }
        if (GetEVPBuffered()) {
            uint8_t ucResetAI = GetResetAI();
            EvaluatePackage(true, ucResetAI);
        }
        if (!IsKnockedOut() &&
            (ReferenceProcessLists.bProcessSche ||
             (pCurrentProcess->GetCurrentPackage() && pCurrentProcess->GetCurrentPackage()->IsInterruptPackage())) &&
            (!pCurrentProcess->GetCurrentPackage() ||
             pCurrentProcess->GetCurrentPackage()->GetPackType() != PACKAGE_TYPE_DO_NOTHING)) {
            if (afDelta == 0.0 || this == pPlayer) {
                pCurrentProcess->Update(this);
            }
            else {
                pCurrentProcess->Update(this, afDelta);
            }
        }

        char cDay = appCalendar.GetDay();
        ExtraDataList* pExtra = GetExtra();
        pExtra->CheckRemoveRunOnceDayPackage(cDay);
    }
    else if (IsDead(false)) {
        if (GetLifeState() == ACTOR_LIFE_STATE_DYING) {
            if (GetLifeState() == ACTOR_LIFE_STATE_DYING) {
                if (bEssentialTakeNoDamage.Bool() && GetEssential()) {
                    Resurrect(false, false, true);
                }
                else {
                    pCurrentProcess->Update(this);
                }
            }
        }
        else {
            pCurrentProcess->ComputeLastTimeProcessed();
            fCheckMyDeadBodyTimer = fCheckMyDeadBodyTimer - fSeconds;
            fDeadBodyAlarm = fDeadBodyAlarm - fSeconds;
            if (fDeadBodyAlarm <= 0.0) {
                if (pMyKiller == pPlayer) {
                    BaseProcess* pPlayerProcess = pPlayer->GetCurrentAIProcess();
                    if (!pPlayerProcess->GetActorsDetectionEvent(pPlayer)) {
                        BaseProcess* pPlayerProcess2 = pPlayer->GetCurrentAIProcess();
                        NiPoint3 location = GetLocationOnReference();
                        pPlayerProcess2->SetActorsDetectionEvent(pPlayer, location, 1, 4, nullptr);
                    }
                }
                fDeadBodyAlarm = 3.0f;
            }
        }
        if (!GetEssential() && GetRespawn()) {
            TESObjectCELL* pParentCell = GetParentCell();
            if (!TES::pInstance->IsCellLoaded(pParentCell, false) && GetCurrentAIProcess()) {
                BaseProcess* pDeadProcess = GetCurrentAIProcess();
                if (pDeadProcess->GetDeathTime() != 0.0) {
                    BaseProcess* pDeadProcess2 = GetCurrentAIProcess();
                    double dDeathTime = static_cast<double>(pDeadProcess2->GetDeathTime());
                    double dClearTime = static_cast<double>(TESObjectCELL::GetHoursToRespawnCell()) + dDeathTime;
                    if (static_cast<double>(appCalendar.GetHoursPassed()) > dClearTime) {
                        ReferenceProcessLists.AddActorToTempChangeList(this);
                    }
                }
            }
        }
    }
    if (iNumHWThreads.UInt() == 1) {
        BaseProcess* pHighProcess = GetCurrentAIProcess();
        if (pHighProcess->GetProcessLevel() == PROCESS_TYPE_HIGH && !pSaveLoadGameOLD->GetLoading()) {
            UpdateMagic(false, false);
        }
    }
    if (this != pPlayer && GetObjectReference()) {
        BaseProcess* pMovedProcess = GetCurrentAIProcess();
        if (pMovedProcess->GetProcessLevel()) {
            TESObjectCELL* pParentCell = GetParentCell();
            NiPoint3 location = GetLocationOnReference();
            if (pParentCell && !pParentCell->GetInterior() && !pParentCell->IsPointInCellExterior(location)) {
                TESWorldSpace* pWorldSpace = pParentCell->GetWorldSpace();
                TESObjectREFR::MoveRefToNewSpace(this, nullptr, pWorldSpace);
            }
        }
    }

    AIProcessingProcess.Stop();
}

// Moves the actor to its desired process level. False when the actor has no process or was deleted or disabled;
// a non-persistent actor dropped to a middle-low or low process is unloaded and deleted.
bool Actor::ChangeProcessLevel() {
    if (!pCurrentProcess) {
        return false;
    }
    PROCESS_TYPE eLevel = pCurrentProcess->GetProcessLevel();
    PROCESS_TYPE eDesiredLevel = GetDesiredProcessLevel();
    if (eLevel == eDesiredLevel) {
        return true;
    }
    if (eLevel == PROCESS_TYPE_HIGH && pCurrentProcess->GetFadeState() != HighProcess::FADE_NORMAL) {
        if (pCurrentProcess->GetFadeState() == HighProcess::FADE_OUT_DELETE) {
            delete this;
            return false;
        }
        if (pCurrentProcess->GetFadeState() == HighProcess::FADE_OUT_DISABLE) {
            Disable();
            return false;
        }
    }
    if (eDesiredLevel != PROCESS_TYPE_HIGH) {
        TESPackage* pPackage = pCurrentProcess->GetPackageThatIsRunning();
        if (pPackage &&
            pPackage->GetProcedureType() == PACKAGE_PROCEDURE_ESCORT_ACTOR &&
            pCurrentProcess->GetTarget() == pPlayer) {
            bool bPassesPlayer = pActorMover->ComputePathPassesThroughPlayersArea();
            if (!bPassesPlayer) {
                StopMoving();
            }
        }
    }
    bool bUnload = false;
    switch (eDesiredLevel) {
        case PROCESS_TYPE_HIGH: MovetoHigh(); break;
        case PROCESS_TYPE_MIDDLE_HIGH: MovetoMiddleHigh(); break;
        case PROCESS_TYPE_MIDDLE_LOW:
            MovetoMiddleLow();
            if (!GetRefPersists()) {
                bUnload = true;
            }
            break;
        case PROCESS_TYPE_LOW:
            MovetoLow();
            if (!GetRefPersists()) {
                bUnload = true;
            }
            break;
        default: break;
    }
    if (!bUnload) {
        return true;
    }
    if (!CheckSummonedActor() && GetParentCell()) {
        DispelPermanentMagic();
        static_cast<MagicTarget*>(this)->Dispel();
        CastPermanentMagic(false);
        pSaveLoadGameOLD->UnloadForm(this);
        pSaveLoadGame->UnloadForm(this, false);
    }
    delete this;
    return false;
}

// Runs the actor's dialogue each frame: a new response (bSpeakingDone) plays its voice sound or speaks its line, with
// the subtitle and animation callbacks when abChangeAnimationEndVoice; the actor turns towards the player while
// talking, keeps its bones at full LOD, and its face follows the persuasion emotion.
void Actor::UpdateInDialogue(float afDelta, DialogueResponse* pResponse, bool abChangeAnimationEndVoice) {
    Animation* pAnimation = GetAnimation();
    DialoguePackage* pDialoguePackage = nullptr;
    TESPackage* pPackage = GetCurrentAIProcess()->GetCurrentPackage();
    if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        pDialoguePackage = static_cast<DialoguePackage*>(pPackage);
    }
    if (!pAnimation) {
        return;
    }

    bool bStartedSpeaking = false;
    if (pResponse && GetCurrentAIProcess() && bSpeakingDone) {
        BSSoundHandle kSpeechHandle;
        bStartedSpeaking = true;
        pTalkingToPlayer = this;
        bTalkingToPlayer = true;
        bool bSpeakLine = true;
        if (pResponse->GetVoiceSound()) {
            uint32_t uiSoundFormID = pResponse->GetVoiceSound()->GetFormID();
            // The callbacks' context is the actor's form ID, passed as the untyped context pointer.
            BSSoundHandle kVoiceSound = PlaySound(
                uiSoundFormID,
                false,
                BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT,
                true
            );
            BSAudioManager* pManager = &BSAudioManager::QInstance();
            void* pContext = reinterpret_cast<void*>(static_cast<uintptr_t>(GetFormID()));
            uint32_t uiSoundID = kVoiceSound.GetID();
            pManager->SetCompletionCallback(uiSoundID, &MobileObject::SoundFileCallBack, pContext);
            bSoundFileDone = false;
            bSpeakLine = false;
            bSpeakingDone = false;
        }
        SetShouldUseEmotionAnimation(pResponse->GetUseEmotion());
        if (bSpeakLine && bSoundFileDone) {
            // The arguments are evaluated right to left.
            TESIdleForm* pListenerIdle = pResponse->GetListenerIdle();
            TESIdleForm* pSpeakerIdle = pResponse->GetSpeakerIdle();
            int32_t iLength = pResponse->GetText()->GetLength();
            uint32_t uiEmotionValue = pResponse->GetEmotionValue();
            uint32_t eEmotion = pResponse->GetEmotion();
            const char* pVoiceFile = pResponse->GetVoice()->String();
            float fSoundDelay = SpeakSoundFunction(
                pVoiceFile,
                kSpeechHandle,
                static_cast<DIALOGUE_EMOTION>(eEmotion),
                uiEmotionValue,
                iLength,
                pSpeakerIdle,
                pListenerIdle,
                pPlayer,
                true,
                true,
                false,
                true,
                true
            );
            BSAudioManager* pManager = &BSAudioManager::QInstance();
            Actor* pTarget = nullptr;
            if (abChangeAnimationEndVoice) {
                pTarget = this;
            }
            if (pTarget) {
                void* pContext = reinterpret_cast<void*>(static_cast<uintptr_t>(pTarget->GetFormID()));
                uint32_t uiSoundID = kSpeechHandle.GetID();
                pManager->SetPlayCallback(uiSoundID, &MobileObject::DisplaySubTitleCallback, pContext);
                pContext = reinterpret_cast<void*>(static_cast<uintptr_t>(pTarget->GetFormID()));
                uiSoundID = kSpeechHandle.GetID();
                pManager->SetCompletionCallback(uiSoundID, &Actor::ChangeDialogueAnimation, pContext);
            }
            bVoiceFileDone = false;
            GetCurrentAIProcess()->SetSoundDelay(fSoundDelay);
        }
    }

    if (pRagdollController) {
        pRagdollController->ForceHeadTrack();
    }
    if (pActorMover && GetSitSleepState() == SIT_SLEEP_NORMAL && !IsInCombat()) {
        if (!pActorMover->IsRotating()) {
            const NiPoint3* pLocation = &GetLocationOnReference();
            NiPoint3 kToPlayer = pPlayer->GetLocationOnReference() - *pLocation;
            float fHeading = GetZAngleFromVector(kToPlayer);
            float fTurn = fHeading - GetAngleOnReference().z;
            BSWrap(fTurn, -3.1415927f, 3.1415927f);
            // More than one degree off: turn towards the player.
            if (std::fabs(fTurn) > 0.01745329238474369) {
                pActorMover->RequestRotateActor(pPlayer->GetLocationOnReference(), false);
            }
        }
        pActorMover->SetInDialog(true);
        UpdateMovement(afDelta, false);
        pActorMover->SetInDialog(false);
    }
    UpdateSoundCallBack();

    if (GetCurrentAIProcess() && GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_HIGH) {
        NiBSBoneLODController* pBoneLOD = static_cast<HighProcess*>(GetCurrentAIProcess())->GetBoneLOD();
        if (pBoneLOD && pBoneLOD->GetBoneLOD()) {
            pBoneLOD->SetBoneLOD(0);
        }
    }

    if (GetCurrentAIProcess()) {
        BSSoundHandle kSoundHandle = GetCurrentAIProcess()->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
        BSFaceGenAnimationData* pFaceData = GetFaceAnimationData();
        if (!kSoundHandle.IsPlaying()) {
            if (pDialoguePackage && pDialoguePackage->pTalkingActor) {
                pDialoguePackage->pTalkingActor = nullptr;
            }
            if (pFaceData) {
                if (ePersuasionEmotion == DIALOGUE_EMOTION_COUNT) {
                    if (BSFaceGenManager::GetUseEmotions()) {
                        if ((!GetCurrentAIProcess() || !GetCurrentAIProcess()->GetLastSpeakingEmotion()) &&
                            Interface::InDialog()) {
                            pFaceData->SetExpressionTargetFromMood(
                                static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::MOOD),
                                false
                            );
                        }
                    }
                    else {
                        pFaceData->ResetCurrentMorphs(0.0f, true, false, false, false, false);
                    }
                }
                else if (ePersuasionEmotion == DE_ANGER) {
                    pFaceData->SetExpressionTargetFromMood(6, false);
                }
                else if (ePersuasionEmotion == DE_HAPPY) {
                    pFaceData->SetExpressionTargetFromMood(5, false);
                }
            }
        }
        else if (pFaceData && ePersuasionEmotion == DIALOGUE_EMOTION_COUNT) {
            BSFaceGenAnimationData* pTalkingFaceData = GetFaceAnimationData();
            if (pTalkingFaceData) {
                if (BSFaceGenManager::GetUseEmotions() && pTalkingFaceData) {
                    if ((!GetCurrentAIProcess() || !GetCurrentAIProcess()->GetLastSpeakingEmotion()) &&
                        Interface::InDialog()) {
                        pTalkingFaceData->SetExpressionTargetFromMood(
                            static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::MOOD),
                            false
                        );
                    }
                }
                else if (pTalkingFaceData) {
                    pTalkingFaceData->ResetCurrentMorphs(0.0f, true, false, false, false, false);
                }
            }
        }
    }

    if (bStartedSpeaking && pDialoguePackage) {
        pDialoguePackage->pTalkingActor = this;
    }
    if (pDialoguePackage && pAnimation->SpecialIdleDonePlaying() && !GetCurrentAIProcess()->GetCurrentProcessIdle()) {
        GetCurrentAIProcess()->SetupSpecialIdle(this, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, false, true);
    }
}

void Actor::UpdateAnimation(float afDelta) {
    AIProcessing_UpdateAnimation.Start();
    if (Get3D() && Get3D()->m_pkParent) {
        TESObjectREFR::UpdateAnimation();
        pCurrentProcess->UpdateKnockState(this);
        Animation* panim = GetAnimation();
        if (IsCreature() && panim && panim->GroupLoaded(225) && pCurrentProcess) {
            if (pCurrentProcess->GetCreatureLipSynchAnim()) {
                int32_t iframe = 0;
                uint32_t uiStartTime = pCurrentProcess->GetCreatureLipSynchStartTime();
                if (static_cast<uint32_t>(static_cast<int64_t>(panim->GetTime() * 1000.0)) > uiStartTime) {
                    uiStartTime = pCurrentProcess->GetCreatureLipSynchStartTime();
                    iframe = (static_cast<uint32_t>(static_cast<int64_t>(panim->GetTime() * 1000.0)) - uiStartTime) /
                        33;
                }
                float fpuppetValue = pCurrentProcess->GetCreatureLipSynchAnim()->GetPuppetValue(iframe);
                panim->SetLipTimeMember(fpuppetValue);
                if (pCurrentProcess->GetCreatureLipSynchAnim()) {
                    if (iframe > pCurrentProcess->GetCreatureLipSynchAnim()->iNumFrames) {
                        pCurrentProcess->SetCreatureLipSynchAnim(nullptr);
                        pCurrentProcess->SetCreatureLipSynchStartTime(0);
                    }
                }
            }
        }

        if (bDeadFlag && pCurrentProcess && TESObjectREFR::Get3D()) {
            bool bshouldHaveRagDoll = true;
            Creature* pcreature = nullptr;
            if (GetObjectReference()->GetFormType() == CREA_ID) {
                pcreature = static_cast<Creature*>(this);
            }
            if (pcreature && !pcreature->HasRagDoll()) {
                bshouldHaveRagDoll = false;
            }

            if (!bshouldHaveRagDoll && panim && panim->GroupLoaded(224)) {
                SetLifeState(ACTOR_LIFE_STATE_DYING);
                DetachCharController();
                panim->ForceSection(AGS_WHOLE_BODY, 224, AGA_NONE, 0.0f, -1);
                BSAnimGroupSequence* pseq = panim->GetCurrentSequence(AGS_MOVEMENT);
                if (pseq) {
                    pseq->SetOffset(pseq->m_fEndKeyTime);
                    panim->Update(this, 0.0f, pseq->m_fEndKeyTime);
                    panim->UpdateMovement(this);
                }
                DropAddOnObjects();
                bDeadFlag = false;
            }
        }

        if (pCurrentProcess) {
            if (pVATS.eMode == VATS::VATS_PLAYBACK && this == pVATSTarget) {
                double ddelta = appTimer.fDelta;
                float fscaledDelta = pVATS.GetTargetUpdateMult() * ddelta;
                pCurrentProcess->UpdateDeferredHideDismemberedLimbs(this, fscaledDelta);
            }
            else {
                pCurrentProcess->UpdateDeferredHideDismemberedLimbs(this, appTimer.fDelta);
            }
        }
        if (!IsDead(false)) {
            if (pCurrentProcess->GetWeaponDrawn() && panim && !panim->GetCurrentSequence(AGS_WEAPON)) {
                StartWeaponGroup(ANIM_GROUP_AIM, panim);
            }
        }
        UpdateAnimationMovement(panim, afDelta);
        if (IsCreature()) {
            float frotateScale;
            if (IsInCombat()) {
                frotateScale = gs_fCreatureCombatTurnSpeedScale_Pathfinding.Float();
            }
            else {
                frotateScale = gs_fCreatureTurnSpeedScale_Pathfinding.Float();
            }
            PickAnimations(1.0f, frotateScale);
        }
        else {
            float frotateScale;
            if (IsInCombat()) {
                frotateScale = gs_fAICombatTurnSpeedScale_Pathfinding.Float();
            }
            else {
                frotateScale = gs_fAITurnSpeedScale_Pathfinding.Float();
            }
            PickAnimations(1.0f, frotateScale);
        }
    }

    AIProcessing_UpdateAnimation.Stop();
}

bool Actor::IsInCombat(bool abSearching) {
    if (this == pPlayer) {
        return false;
    }
    if (!pCurrentProcess) {
        return false;
    }
    if (bInCombat && (abSearching && bSearchingInCombat)) {
        return false;
    }
    return bInCombat;
}

bool Actor::InitiateDialogue(
    MobileObject* apTarget,
    PackageLocation* pPackLoc,
    PackageLocation* pPackSecondLoc,
    bool bHeadTrack,
    bool bMove,
    bool bflag,
    TESTopic* pTopic,
    bool abTargetContinueMoving,
    bool abTargetMoveTowardStarter
) {
    if (!pCurrentProcess) {
        return false;
    }
    TESPackage* pTempPack = pCurrentProcess->GetCurrentPackage();
    Conversation* pConversation = nullptr;
    Actor* pactor = nullptr;
    if (apTarget && apTarget->IsActor()) {
        pactor = static_cast<Actor*>(apTarget);
    }

    if (!apTarget || (pactor && pactor->IsTryingToEnterFurniture())) {
        return false;
    }
    if (!apTarget->pCurrentProcess) {
        return false;
    }
    if (IsDead(false) && (GetDismembered(BGSBodyPart::LIMB_HEAD_1) || GetDismembered(BGSBodyPart::LIMB_HEAD_2))) {
        return false;
    }
    if (apTarget != pPlayer) {
        pConversation = new Conversation(this, apTarget, pTopic);
    }
    StopCurrentDialogue();
    pCurrentProcess->ClearRunOncePackage();
    if (apTarget == pPlayer || pConversation->FirstItem()) {
        PackageTarget* pnew;
        DialoguePackage* ppack;
        if (bHeadTrack) {
            pCurrentProcess->ClearActionHeadTrackTarget(true);
            pCurrentProcess->SetDialogHeadTrackTarget(apTarget);
            apTarget->pCurrentProcess->ClearActionHeadTrackTarget(true);
            apTarget->pCurrentProcess->SetDialogHeadTrackTarget(this);
        }
        ppack = new DialoguePackage(pConversation, this, static_cast<Actor*>(apTarget), nullptr, true, true);
        ppack->ptopic = pTopic;
        if (pTempPack) {
            ppack->CopyAllFlags(pTempPack);
        }
        ppack->SetPackType(PACKAGE_TYPE_IN_GAME_DIALOGUE);
        ppack->SetMustReachLocation(true);
        ppack->SetMustComplete(true);
        ppack->SetDialogueTopic(pTopic);
        ppack->SetContinueMoving(bMove);
        ppack->SetTargetContinueMoving(abTargetContinueMoving);
        ppack->SetDialogueDoNotControlTarget(abTargetContinueMoving);
        ppack->SetNoHeadTrack(!bHeadTrack);
        ppack->SetTargetMoveTowardStarter(abTargetMoveTowardStarter);
        PackageLocation kpackLoc;
        if (pPackLoc) {
            kpackLoc.Copy(pPackLoc, true);
        }
        kpackLoc.SetLocReference(this);
        ppack->SetPackageLocation(&kpackLoc);
        PackageLocation ksecondLoc;
        if (pPackSecondLoc) {
            ksecondLoc.Copy(pPackSecondLoc, true);
        }
        double dlocationRadius = static_cast<double>(ppack->GetLocationRadius(this));
        int64_t iradius = static_cast<int64_t>(fSandBoxExtraDialogueRange.Float() + dlocationRadius);
        ksecondLoc.SetRadius(iradius);
        ppack->SetPackageSecondLocation(&ksecondLoc);
        pnew = new PackageTarget;
        ppack->SetPackageTarget(pnew);
        if (pnew) {
            delete pnew;
        }
        ppack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_DIALOGUE;
        ppack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
        ppack->GetPackageTarget()->SetTargReference(apTarget);
        if (pCurrentProcess->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
            ppack->GetPackageTarget()->iValue = 200u;
        }
        else {
            ppack->GetPackageTarget()->iValue = 90u;
        }
        pCurrentProcess->ComputeLastTimeProcessed();
        PutCreatedPackage(ppack, true, true);
        SetDialogueItemTarget(apTarget);
        if (bMove) {
            pCurrentProcess->AddToProcedureIndexRunning(this, 2);
        }
        else if (bflag) {
            pCurrentProcess->AddToProcedureIndexRunning(this, 1);
        }
        pCurrentProcess->BaseProcess::ComputeLastTimeProcessed();
        pCurrentProcess->BaseProcess::ComputeLastTimeProcessed();
        if (apTarget != pPlayer && (apTarget != this && (!abTargetContinueMoving || abTargetMoveTowardStarter))) {
            apTarget->PutCreatedPackage(ppack, true, true);
            DialoguePackage* pcurrentPackage = static_cast<DialoguePackage*>(apTarget->GetCurrentPackage());
            if (pcurrentPackage != ppack) {
                EndDialogue();
                return false;
            }
            apTarget->SetDialogueItemTarget(this);
            if (bMove) {
                apTarget->pCurrentProcess->AddToProcedureIndexRunning(apTarget, 2);
            }
            else {
                apTarget->pCurrentProcess->AddToProcedureIndexRunning(apTarget, 1);
            }
        }
        return true;
    }

    if (pConversation) {
        delete pConversation;
    }
    return false;
}

bool Actor::InitiateDialogue(TESPackage* pDialoguePackage) {
    PackageTarget* ppacktarg;
    DialoguePackage* ppack;
    pCurrentProcess->GetCurrentPackage();
    TESObjectREFR* ptarget = pCurrentProcess->GetTarget();
    MobileObject* pmobileTarget = nullptr;
    MobileObject* ptalkingactivator = nullptr;
    if (!ptarget) {
        pCurrentProcess->SetTargetForPackage(this);
    }
    ptarget = pCurrentProcess->GetTarget();
    if (!ptarget) {
        return false;
    }
    if (ptarget->IsMobileObject()) {
        pmobileTarget = static_cast<MobileObject*>(ptarget);
    }
    else if (ptarget->GetObjectReference()->GetFormType() == TACT_ID) {
        ptarget->m_Extra.SetTalkingActorExtra(
            static_cast<BGSTalkingActivator*>(ptarget->GetObjectReference()),
            ptarget
        );
        pmobileTarget = ptarget->m_Extra.GetTalkingActorExtra()->pTalkObject;
        ptalkingactivator = pmobileTarget;
    }
    if (!pmobileTarget || !pmobileTarget->pCurrentProcess) {
        return false;
    }
    if (pmobileTarget != this) {
        pmobileTarget->EndDialogue();
    }
    TESTopic* ptopic = pDialoguePackage->GetDialogueTopic();
    pCurrentProcess->ClearRunOncePackage();
    if (!pDialoguePackage->GetNoHeadTrack() && !pDialoguePackage->GetDialogueNoHeadtrack()) {
        pCurrentProcess->ClearActionHeadTrackTarget(true);
        pCurrentProcess->SetDialogHeadTrackTarget(pmobileTarget);
    }
    ppack = new DialoguePackage(
        nullptr,
        this,
        pmobileTarget,
        static_cast<Actor*>(ptalkingactivator),
        false,
        pDialoguePackage->pIdleCollection == nullptr
    );
    if (pDialoguePackage) {
        ppack->CopyAllFlags(pDialoguePackage);
    }
    ppack->SetPackType(PACKAGE_TYPE_IN_GAME_DIALOGUE);
    ppack->SetMustReachLocation(pDialoguePackage->GetMustReachLocation());
    ppack->SetMustComplete(pDialoguePackage->GetMustComplete());
    ppack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_DIALOGUE;
    ppack->SetPackageLocation(pDialoguePackage->pPackLoc);
    ppack->ptopic = ptopic;
    if (pDialoguePackage->GetPackageSecondLocation()) {
        ppack->SetPackageSecondLocation(pDialoguePackage->GetPackageSecondLocation());
    }
    ppacktarg = new PackageTarget;
    ppacktarg->SetTargReference(pmobileTarget);
    ppacktarg->SetTargType(PTT_REFERENCE);
    ppacktarg->iValue = pDialoguePackage->GetPackageTarget()->iValue;
    ppack->SetPackageTarget(ppacktarg);
    if (ppacktarg) {
        delete ppacktarg;
    }
    ppack->SetDialogueDoNotControlTarget(pDialoguePackage->GetDialogueDoNotControlTarget());
    ppack->SetDialogueFOV(pDialoguePackage->GetDialogueFOV());
    ppack->SetDialogueTopic(pDialoguePackage->GetDialogueTopic());
    ppack->SetDialogueNoHeadTrack(pDialoguePackage->GetDialogueNoHeadtrack());
    pCurrentProcess->ComputeLastTimeProcessed();
    pPlayer->pAIConversationRunning = ppack;
    pCurrentProcess->SavePackageToExtraData(this);
    PutCreatedPackage(ppack, false, true);
    if (pCurrentProcess->GetProcedureIndexRunning() != 1) {
        pCurrentProcess->SetCurrentProcedureIndex(1);
    }
    pCurrentProcess->SetTarget(pmobileTarget);
    pCurrentProcess->BaseProcess::ComputeLastTimeProcessed();
    pCurrentProcess->BaseProcess::ComputeLastTimeProcessed();
    SetDialogueItemTarget(pmobileTarget);
    if (!pDialoguePackage->GetDialogueDoNotControlTarget() && !pDialoguePackage->GetPackageSecondLocation()) {
        if (pmobileTarget == pPlayer || pmobileTarget == this) {
            if (!pmobileTarget->IsDead(false) && (pmobileTarget != pPlayer && pmobileTarget != this)) {
                pCurrentProcess->AddToProcedureIndexRunning(this, 3);
                return true;
            }
        }
        else {
            pmobileTarget->pCurrentProcess->SavePackageToExtraData(pmobileTarget);
            pmobileTarget->PutCreatedPackage(GetCurrentPackage(), false, true);
            pmobileTarget->pCurrentProcess->SetTarget(pmobileTarget);
            pmobileTarget->pCurrentProcess->AddToProcedureIndexRunning(pmobileTarget, 1);
            pmobileTarget->SetDialogueItemTarget(this);
        }
    }
    ppack->pTargetLocation = pCurrentProcess->SetLocationForPackage(this, true);
    return true;
}

// The callbacks' context is the actor's form ID.
float Actor::SpeakSoundFunction(
    const char* apSoundFile,
    BSSoundHandle& aSoundHand,
    DIALOGUE_EMOTION aeEmotiontype,
    uint32_t aiEmotionValue,
    int32_t aiStringLength,
    TESIdleForm* apResponseSpeakerIdle,
    TESIdleForm* apResponseListenerIdle,
    TESObjectREFR* apTarget,
    bool abSetEmotion,
    bool ab2D,
    bool abQueue,
    bool abLip,
    bool abUseIdleManager
) {
    double dDistanceFromPlayer;
    char acSoundFile[516];
    float fdelay = 0.0f;
    BSStringT<char> tempstring(apSoundFile);
    BSFaceGenAnimationData* panimdata = nullptr;
    this->SetSpeakingEmotion(static_cast<int32_t>(aeEmotiontype));
    this->SetSpeakingEmotionValue(static_cast<int32_t>(aiEmotionValue));
    if (!apSoundFile ||
        (this->GetDismembered(BGSBodyPart::LIMB_HEAD_1) || this->GetDismembered(BGSBodyPart::LIMB_HEAD_2))) {
        if (this->IsinDialogue()) {
            MobileObject::DialogueConversationCallBack(reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)), true);
        }
        else {
            MobileObject::SayToCallBack(reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)), true);
        }
        return 0.0f;
    }
    BSstrcpy(acSoundFile, 512, apSoundFile);
    if (!this || !this->Get3D()) {
        return 0.0f;
    }
    Actor::kSpeakSoundCriticalSection.Enter(nullptr);
    LipSynchAnim* panim = this->pCurrentProcess->GetLipAnim();
    this->StopCurrentDialogue();
    bool bisPlayer = this->IsPc();
    float fSoundDuration = 0.0f;
    bool bplay2D = bisPlayer || ab2D;
    if ((this->IsInDialoguewithPlayer() ||
         (dDistanceFromPlayer = this->GetDistanceFromReference(pPlayer, false, false),
          static_cast<double>(iSpeakSoundLipDistance.Int()) > dDistanceFromPlayer)) &&
        (!bisPlayer || pPlayer->Is3rdPerson())) {
        panimdata = this->GetFaceAnimationData();
    }

    if (!panim && this != pPlayer) {
        if (!abLip || this->pCurrentProcess->GetLipFileFailed()) {
            BSFaceGenAnimationData* pfaceData = this->GetFaceAnimationData();
            if (pfaceData) {
                pfaceData->AddAnimationDataFromNote("BigAah 0.9 0.2 0.1 0.2");
            }
            if (this->pCurrentProcess->GetLipFileFailed()) {
                MobileObject::DisplaySubTitleCallback(reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)), 0);
                this->pCurrentProcess->SetLipFileFailed(false);
            }
        }
        else if (LipSynchAnim::ChangeFilenameToLipSynch(tempstring)) {
            if (abQueue && bBackgroundLoadLipFiles.Bool()) {
                this->pCurrentProcess->SetWaitingforLipFile(true);
                this->pCurrentProcess->SetLipQuequed(true);
                ReferenceProcessLists.kLipSyncManager.BuildLipSynicFile(this, tempstring.pString);
                Actor::kSpeakSoundCriticalSection.Leave();
                return 0.0f;
            }
            panim = LipSynchAnim::LoadFromFile(tempstring);
            if (!panim) {
                MobileObject::DisplaySubTitleCallback(reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)), 0);
                DialogMenu::fUseTimerNoVoice = fDialogSpeechDelaySeconds.Float();
            }
            this->pCurrentProcess->SetWaitingforLipFile(false);
        }
    }
    Animation* panimation = this->GetAnimation();
    if (panimation) {
        if (apResponseSpeakerIdle || abUseIdleManager) {
            bool bchooseNewOne = !this->IsInCombat();
            TESIdleManager::SetUsedItem(this->pCurrentProcess->GetItemBeingUsed());
            this->pCurrentProcess->SetupSpecialIdle(
                this,
                apResponseSpeakerIdle,
                apResponseSpeakerIdle ? AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE : AnimIdle::PLAY_TYPE_AUTO,
                false,
                bchooseNewOne,
                true
            );
            TESIdleManager::SetUsedItem(nullptr);
        }
        if (apTarget && apTarget != this && (apTarget->IsMobileObject() || apTarget->IsActor())) {
            Actor* ptargact = static_cast<Actor*>(apTarget);
            bool bnoIdles = true;
            if (apTarget->IsActor() && ptargact->pCurrentProcess) {
                TESPackage* ptargpack = ptargact->GetCurrentPackage();
                if (ptargpack) {
                    bnoIdles = ptargpack->pIdleCollection == nullptr;
                }
                if (apResponseListenerIdle || bnoIdles) {
                    BaseProcess* pListenerProcess = ptargact->pCurrentProcess;
                    pListenerProcess->SetupSpecialIdle(
                        ptargact,
                        apResponseListenerIdle,
                        apResponseListenerIdle ? AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE : AnimIdle::PLAY_TYPE_AUTO,
                        false,
                        true,
                        true
                    );
                }
            }
        }
    }
    if (panim) {
        int64_t iframes = panim->iNumFrames;
        fSoundDuration = static_cast<double>(iframes) / 30.0;
        panim->ApplyToNode(panimdata, 0.2f);
    }
    if (abSetEmotion && this->pCurrentProcess) {
        this->pCurrentProcess->SetLastSpeakingEmotion(aeEmotiontype);
    }

    uint32_t isoundFlags = 260;
    if (!bisPlayer && this->ShouldModulateVoice()) {
        isoundFlags |= BSGameSound::SOUND_FLAG_MODULATED;
    }
    if (bplay2D) {
        isoundFlags |= BSGameSound::SOUND_FLAG_IS_2D;
    }
    else {
        isoundFlags |= BSGameSound::SOUND_FLAG_IS_3D;
    }
    aSoundHand = BSAudio::QInstance().GetSoundHandleByFilename(acSoundFile, isoundFlags, nullptr);
    aSoundHand.SetPriority(64u);
    if (bplay2D) {
        double dattenuation = fDBVoiceAttenuationIn2D.Float() * 100.0;
        aSoundHand.SetStaticAttenuation(static_cast<int32_t>(dattenuation));
    }
    else {
        NiPoint3 kLocation = this->GetLocationOnReference();
        // Gets the maximum before the minimum.
        float fMaxDist = BSAudio::GetMaxDialogDist();
        float fMinDist = BSAudio::GetMinDialogDist();
        aSoundHand.SetMinMax(fMinDist, fMaxDist);
        float fsoundZ = kLocation.z + 200.0;
        aSoundHand.SetPosition(kLocation.x, kLocation.y, fsoundZ);
        aSoundHand.SetObjectToFollow(this->Get3D());
    }
    BSAudioManager::QInstance().SetPlayCallback(
        aSoundHand.iSoundID,
        &MobileObject::DisplaySubTitleCallback,
        reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID))
    );
    if (panim) {
        double dstartingDelay = panim->GetStartingDelay();
        float fplaydelay = (BSFaceGenAnimationData::GetSpeechDelay() + dstartingDelay) * 1000.0;
        aSoundHand.PlayAfter(static_cast<int32_t>(fplaydelay), 0);
        dstartingDelay = panim->GetStartingDelay();
        fdelay = BSFaceGenAnimationData::GetSpeechDelay() + dstartingDelay;
        if (fSoundDuration > 0.0) {
            fdelay = fdelay + fSoundDuration;
        }
        else if (aSoundHand.GetDuration()) {
            int32_t iduration = aSoundHand.GetDuration();
            fdelay = static_cast<double>(iduration) / 1000.0 + fdelay;
        }
        else {
            double dstringLength = static_cast<double>(aiStringLength);
            fdelay = fAIDefaultSpeechMult.Float() * dstringLength + fdelay;
        }
        if (this->IsCreature()) {
            // Converts through a 64-bit integer and passes the low 32 bits.
            int64_t istartTime = static_cast<int64_t>(panimation->time * 1000.0 + fplaydelay);
            this->pCurrentProcess->SetCreatureLipSynchStartTime(static_cast<uint32_t>(istartTime));
            this->pCurrentProcess->SetCreatureLipSynchAnim(panim);
        }
    }
    else {
        aSoundHand.PlayAfter(1u, 0);
        double dstringLength = static_cast<double>(aiStringLength);
        fdelay = fAIDefaultSpeechMult.Float() * dstringLength;
    }
    if (this->bTalkingToPlayer) {
        this->bSpeakingDone = false;
    }
    BaseProcess* pprocess = this->pCurrentProcess;
    BSSoundHandle kVoiceHandle;
    kVoiceHandle = aSoundHand;
    pprocess->SetSoundHandle(ACTOR_SOUND_HANDLE_VOICE, kVoiceHandle);
    if (panim) {
        LipSynchAnim* pcreatureAnim = this->pCurrentProcess->GetCreatureLipSynchAnim();
        if (panim != pcreatureAnim && panim) {
            delete panim;
        }

        this->pCurrentProcess->SetLipAnim(nullptr);
        this->pCurrentProcess->SetWaitingforLipFile(false);
    }
    Actor::kSpeakSoundCriticalSection.Leave();
    return fdelay;
}

// Ends the actor's dialogue: drops the talking extra and the subtitle, then for an in-game dialogue package
// (the run-once one first, else the current one) releases both speakers and the talking activator, stops the
// voice and lip files, restores head tracking and packages and frees a sitting or sleeping speaker's idle. Without
// one it only resets the dialogue head track target. Last, a weapon without ammo regen and no ammo is reloaded.
void Actor::EndDialogue() {
    if (!pCurrentProcess || pSaveLoadGameOLD->GetLoading()) {
        return;
    }
    bool bIsRunOnce = true;
    bool bResetTarget = true;
    GetExtra()->RemoveTalkingActorExtra();
    pCurrentProcess->SetSubtitleItem(nullptr);
    TESPackage* pPackage = pCurrentProcess->GetRunOncePackage();
    if (!pPackage || pPackage->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        bIsRunOnce = false;
        pPackage = pCurrentProcess->GetCurrentPackage();
    }
    if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        if (pPackage == pPlayer->pClosestConversation) {
            pPlayer->pClosestConversation = nullptr;
        }
        DialoguePackage* pDialoguePackage = static_cast<DialoguePackage*>(pPackage);
        Actor* pStarter = static_cast<Actor*>(pDialoguePackage->GetActorStartedConversation());
        Actor* pTarget = static_cast<Actor*>(pDialoguePackage->GetTargetOfConversation());
        MobileObject* pActivator = pDialoguePackage->GetTalkingActivator();
        if (pActivator) {
            pActivator->GetTalkingActivator()->GetExtra()->RemoveTalkingActorExtra();
            pActivator->GetTalkingActivator()->GetExtra()->RemoveTalkingActorExtra();
            ReferenceProcessLists.RemoveReference(pActivator, PROCESS_TYPE_HIGH);
            ReferenceProcessLists.AddActorToTempChangeList(static_cast<Actor*>(pActivator));
        }
        if (pStarter != pTarget && bIsRunOnce) {
            Actor* pOther = this == pStarter ? pTarget : pStarter;
            // 0xEF: the form ID of the topic that keeps the other speaker's run-once package.
            if (pDialoguePackage->ptopic && pDialoguePackage->ptopic->GetFormID() == 0xEF) {
                if (pOther->GetCurrentAIProcess()) {
                    BaseProcess* pOtherProcess = pOther->GetCurrentAIProcess();
                    if (pOtherProcess->GetRunOncePackage() == pPackage) {
                        BaseProcess* pProcess = GetCurrentAIProcess();
                        pProcess->ClearRunOncePackage();
                        return;
                    }
                }
            }
            else if (pTarget) {
                if (!pTarget->GetCurrentAIProcess()) {
                    bResetTarget = false;
                }
                else {
                    BaseProcess* pTargetProcess = pTarget->GetCurrentAIProcess();
                    if (pTargetProcess->GetRunOncePackage() != pPackage) {
                        bResetTarget = false;
                    }
                }
            }
        }
        if (pDialoguePackage->handle.IsValid()) {
            pDialoguePackage->DeleteSoundHandle();
        }
        if (pStarter && pStarter->IsActor()) {
            ReferenceProcessLists.kLipSyncManager.CancelLipFile(pStarter);
            pStarter->bVoiceFileDone = true;
        }
        if (pTarget && pTarget->IsActor()) {
            ReferenceProcessLists.kLipSyncManager.CancelLipFile(pTarget);
            pTarget->bVoiceFileDone = true;
        }
        // The result is deliberately never used.
        pPackage->GetDialogueDoNotControlTarget();
        if (pStarter && pStarter->GetCurrentAIProcess()) {
            BaseProcess* pProcess = pStarter->GetCurrentAIProcess();
            pProcess->ClearDialogHeadTrackTarget(true);
            pProcess = pStarter->GetCurrentAIProcess();
            pProcess->SetDefaultHeadTrackTarget(pTarget);
            pStarter->fUpdateTargetTimer = 1.0f;
            pProcess = pStarter->GetCurrentAIProcess();
            pProcess->SetLipFileFailed(false);
            pProcess = pStarter->GetCurrentAIProcess();
            pProcess->SetLastSpeakingEmotion(DE_NEUTRAL);
            if (bIsRunOnce) {
                pProcess = pStarter->GetCurrentAIProcess();
                pProcess->ClearRunOncePackage();
            }
            else {
                pProcess = pStarter->GetCurrentAIProcess();
                pProcess->LoadPackageFromExtraData(pStarter);
            }
            if (!pStarter->IsInCombat(true)) {
                pProcess = GetCurrentAIProcess();
                pProcess->SetCurrentProcessIdle(nullptr);
                if (pStarter->GetAnimation() &&
                    (pStarter->GetSitSleepState() == 0 ||
                     pStarter->GetSitSleepState() == 9 ||
                     pStarter->GetSitSleepState() == 4)) {
                    pProcess = pStarter->GetCurrentAIProcess();
                    pProcess->AddPostAnimationAction(BaseProcess::POSTANIM_FREEUP_SPECIAL_IDLE);
                    pProcess = pStarter->GetCurrentAIProcess();
                    pProcess->SetCurrentProcessIdle(nullptr);
                }
            }
        }
        if (bResetTarget && pTarget && pStarter != pTarget && pTarget != pPlayer && pTarget->GetCurrentAIProcess()) {
            BaseProcess* pProcess;
            if (!pPackage->GetDialogueDoNotControlTarget()) {
                pProcess = pTarget->GetCurrentAIProcess();
                pProcess->ClearDialogHeadTrackTarget(true);
            }
            if (!pPackage->GetDialogueDoNotControlTarget()) {
                pProcess = pTarget->GetCurrentAIProcess();
                pProcess->SetDefaultHeadTrackTarget(pStarter);
                pTarget->fUpdateTargetTimer = 1.0f;
            }
            pProcess = pTarget->GetCurrentAIProcess();
            pProcess->SetLipFileFailed(false);
            pProcess = pTarget->GetCurrentAIProcess();
            pProcess->SetLastSpeakingEmotion(DE_NEUTRAL);
            if (bIsRunOnce) {
                if (pTarget && pTarget->GetCurrentPackage() == pPackage) {
                    pProcess = pTarget->GetCurrentAIProcess();
                    pProcess->ClearRunOncePackage();
                }
            }
            else {
                pProcess = pTarget->GetCurrentAIProcess();
                TESPackage* pRunOnce = pProcess->GetRunOncePackage();
                if (pTarget->IsinDialogue() && !pPackage->GetDialogueDoNotControlTarget()) {
                    if (pRunOnce && pRunOnce->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                        pProcess = pTarget->GetCurrentAIProcess();
                        pProcess->ClearRunOncePackage();
                    }
                    else {
                        pProcess = pTarget->GetCurrentAIProcess();
                        pProcess->LoadPackageFromExtraData(pTarget);
                    }
                }
            }
            if (!pTarget->IsInCombat(true) &&
                pTarget->GetAnimation() &&
                (pTarget->GetSitSleepState() == 0 ||
                 pTarget->GetSitSleepState() == 9 ||
                 pTarget->GetSitSleepState() == 4)) {
                pProcess = pTarget->GetCurrentAIProcess();
                pProcess->AddPostAnimationAction(BaseProcess::POSTANIM_FREEUP_SPECIAL_IDLE);
                pProcess = pTarget->GetCurrentAIProcess();
                pProcess->SetCurrentProcessIdle(nullptr);
            }
        }
    }
    else if (GetCurrentAIProcess()) {
        BaseProcess* pProcess = GetCurrentAIProcess();
        TESObjectREFR* pHeadTrackTarget = pProcess->GetCurrentHeadTrackTarget(BaseProcess::HEAD_TRACK_TYPE_DIALOG);
        pProcess = GetCurrentAIProcess();
        pProcess->ClearDialogHeadTrackTarget(true);
        pProcess = GetCurrentAIProcess();
        pProcess->SetDefaultHeadTrackTarget(pHeadTrackTarget);
        fUpdateTargetTimer = 1.0f;
        pProcess = GetCurrentAIProcess();
        pProcess->SetCurrentProcessIdle(nullptr);
    }
    if (pCurrentProcess && !pCurrentProcess->GetCurrentAmmo() && pCurrentProcess->GetCurrentWeapon()) {
        // 6: the ammo regen mod.
        bool bModValue = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(6);
        auto* pWeapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject());
        if (pWeapon->GetAmmoRegenRate(bModValue) <= 0.0) {
            bool bHasMod2 = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(2);
            bool bDrawn = IsWeaponDrawn();
            auto* pReload = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject());
            ReloadWeaponNV(pReload, bDrawn, bHasMod2, false);
        }
    }
}

void Actor::SetRunOnceExtraPackage(TESPackage* apPack, char acDay) {
    TESObjectREFR::GetExtra()->SetRunOnceExtraPackage(apPack, acDay);
    AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
}

bool Actor::HasEditorLocation() {
    return pEditorLocForm != nullptr;
}

TESWorldSpace* Actor::GetEditorLocationWorld() {
    TESWorldSpace* lpWorld = nullptr;
    if (pEditorLocForm && pEditorLocForm->GetFormType() == 0x41)
    // form type 0x41 is a world space
    {
        lpWorld = static_cast<TESWorldSpace*>(pEditorLocForm);
    }

    return lpWorld;
}

TESObjectCELL* Actor::GetEditorLocationInteriorCell() {
    TESObjectCELL* pCell = nullptr;
    if (pEditorLocForm && pEditorLocForm->GetFormType() == CELL_ID) {
        pCell = static_cast<TESObjectCELL*>(pEditorLocForm);
    }

    if (!pCell) {
        return pCell;
    }
    if (!pCell->GetInterior()) {
        pCell = nullptr;
    }
    return pCell;
}

NiPoint3 Actor::GetEditorLocationCoord() {
    return EditorLocCoord;
}

void Actor::EvaluateDetection(float fnumber) {
    pCurrentProcess->RunDetection(this, fnumber);
}

float Actor::GetDetectionTimer() {
    return pCurrentProcess->GetDetectionTimer();
}

void Actor::ReduceDetectionTimer() {
    pCurrentProcess->ReduceDetectionTimer();
}

// An immobile creature's heading adds the reference's y angle.
float Actor::GetHeading(bool abIgnoreImmobile) {
    float fHeading = GetAngleOnReference().z;
    if (!abIgnoreImmobile && IsActor() && GetActorBase()->IsImmobile()) {
        fHeading = fHeading + GetAngleOnReference().y;
        fHeading = ClampAngle(fHeading);
    }
    return fHeading;
}

// Only a living or reanimated actor turns; otherwise the heading is left alone.
void Actor::SetHeading(float afAngle) {
    if (GetLifeState() == ACTOR_LIFE_STATE_ALIVE || GetLifeState() == ACTOR_LIFE_STATE_REANIMATE) {
        MobileObject::SetHeading(afAngle);
    }
}

void Actor::SetHeading(TESObjectREFR* apTargetReference) {
    MobileObject::SetHeading(apTargetReference);
}

TESObjectREFR* Actor::GetCurrentTarget() {
    if (!pCurrentProcess) {
        return nullptr;
    }
    if (IsInCombat() && (!IsRunningRunOnce() || IsFleeing(false))) {
        Actor* pTarget = GetCombatTarget();
        if (pTarget) {
            return pTarget;
        }
    }
    return pCurrentProcess->GetTarget();
}

// Whether the actor stands within afRadius of aPoint. Height only counts when the actor is on the ground (or
// abAlwaysTestHeight): a point within 30 units below the feet to 30 above the head is level with it. The height terms
// are summed in double.
bool Actor::IsAtPoint(const NiPoint3& aPoint, float afRadius, bool abExpandRadius, bool abAlwaysTestHeight) {
    NiPoint3 kLocation = GetLocationOnReference();
    auto* pController = static_cast<bhkCharacterController*>(GetCharController());
    NiPoint3 kDelta = kLocation - aPoint;
    if (!abAlwaysTestHeight && pController && pController->GetState() != HK_CHARACTER_ON_GROUND) {
        kDelta.z = 0.0f;
    }
    else {
        float fHeight = GetHeight();
        if (fHeight == 0.0) {
            fHeight = 128.0f;
        }
        float fBottom = static_cast<float>(kLocation.z - 30.0);
        float fTop = static_cast<float>(static_cast<double>(kLocation.z) + fHeight + 30.0);
        if (fBottom < aPoint.z && fTop > aPoint.z) {
            kDelta.z = 0.0f;
        }
    }
    if (abExpandRadius) {
        float fActorRadius;
        if (pController) {
            fActorRadius = HK2NI(pController->GetRadius());
            if (pController->QQuadruped()) {
                fActorRadius = fActorRadius + fActorRadius;
            }
            if (fActorRadius < 32.0) {
                fActorRadius = 32.0f;
            }
        }
        else {
            fActorRadius = 32.0f;
        }
        afRadius = afRadius + fActorRadius;
    }
    return NiPoint3::IsMagnitudeLongerThanRadius(&kDelta, afRadius) <= 0;
}

bool Actor::IsDying() {
    return GetLifeState() == ACTOR_LIFE_STATE_DYING;
}

void Actor::PutCreatedPackage(TESPackage* apPack, bool abTempPack, bool abIsaCreatedPackage) {
    if (pCurrentProcess) {
        if (apPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
            apPack->CalculateProcedureType(GetLinkedRef());
        }
        if (apPack->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE &&
            (apPack->GetPackType() != PACKAGE_TYPE_DO_NOTHING &&
             (apPack->GetPackType() != PACKAGE_TYPE_GET_UP &&
              (GetSitSleepState() &&
               ((apPack->GetPackType() != PACKAGE_TYPE_ALARM &&
                 apPack->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) ||
                GetSitSleepState() != SIT_SLEEP_IS_SITTING))))) {
            if (GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                (GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
                 (pCurrentProcess->GetFurnitureMarker() && pCurrentProcess->GetFurnitureMarker()->cNumber > 20u))) {
                InitiateGetUpPackage();
                if (abTempPack) {
                    return;
                }
            }
            else {
                GetOutofFurnitureQuick();
            }
        }
        if (apPack && abIsaCreatedPackage) {
            apPack->SetIsCreated(true);
        }

        if (abTempPack) {
            pCurrentProcess->SetRunOncePackage(apPack, this);
            pCurrentProcess->SetProcedureIndexRunning(0);
            bool bstopMoving = true;
            if (apPack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                bstopMoving = !static_cast<DialoguePackage*>(apPack)->DialoguePackage::GetContinueMoving();
            }
            if (bstopMoving) {
                pCurrentProcess->EndMoveMessage(this);
            }
            AddChange(0);
        }
        else {
            if (pCurrentProcess->GetAwarePlayerTimer() > 0.0) {
                float fawareTimer = pCurrentProcess->GetAwarePlayerTimer() * -1.0;
                pCurrentProcess->ModAwarePlayerTimer(fawareTimer);
                pCurrentProcess->ClearRunOncePackage();
                pCurrentProcess->EndMoveMessage(this);
                pCurrentProcess->SetActionHeadTrackTarget(pPlayer);
            }
            if (!apPack || apPack->GetPackType() != PACKAGE_TYPE_COMBAT) {
                ClearInCombat(true);
            }
            pCurrentProcess->SetCurrentPackage(apPack, this);
            if (!IsInCombat() &&
                Actor::GetAnimation() &&
                ((GetSitSleepState() == SIT_SLEEP_NORMAL ||
                  (GetSitSleepState() == SIT_SLEEP_IS_SLEEPING || GetSitSleepState() == SIT_SLEEP_IS_SITTING)) &&
                 apPack->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE)) {
                pCurrentProcess->AddPostAnimationAction(BaseProcess::POSTANIM_FREEUP_SPECIAL_IDLE);
            }
        }
        pCurrentProcess->SetTarget(nullptr);
        pCurrentProcess->SetupNewPackage();
        if (!IsInCombat() && !GetTurretBehavior()) {
            ClearLookAtTarget();
        }
        AlertAccompanyToPackageChange();
    }
}

// The actor's frame update while its 3D is in an attached cell: the character controller's LOD and sticky collisions,
// the face's dead state, the essential-down timer, the weapon's looping attack sound, follower sneaking, the 3D
// position, the light, then (not for the first-person player) breathing, creature awake sounds, swimming, splashes and
// water damage, the dying actor's death pose and the dead actor's greeting timer; the mover and combat data last. A
// delta of 900 seconds or more only forces the next process update.
void Actor::Update(float afDelta) {
    AIProcessing_Update.Start();
    ResetCachedGunSkill();
    BaseProcess* pProcess = GetCurrentAIProcess();
    if (pProcess) {
        cCurrentSitSleepState = pProcess->GetSitSleepState();
    }
    NiNode* pRoot = Get3D();
    BSFaceGenAnimationData* pFaceAnimData = Actor::GetFaceAnimationData();
    Animation* pAnimData = GetAnimation();
    bool bInCombat = Actor::IsInCombat();
    TESObjectWEAP* pWeapon = GetCurrentWeapon();
    BSSoundHandle kAttackSound;
    bhkCharacterController* pCharController = nullptr;
    bool bIsFirstPersonPlayer = false;
    TESObjectCELL* pCell = GetParentCell();
    if (pRoot && pCell && pCell->GetAttached()) {
        if (afDelta >= 900.0) {
            if (pProcess) {
                pProcess->SetForceNextUpdate(true);
            }
        }
        else {
            SetTresPassing();
            if (this != pPlayer &&
                static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::TURBO) > 0 &&
                GetAnimation()) {
                float fTimeMult = 1.0f / fTurboTimeMultiplier.Float();
                GetAnimation()->SetGlobalTimeMultiplier(fTimeMult);
            }
            else if (this != pPlayer && GetAnimation()) {
                GetAnimation()->SetGlobalTimeMultiplier(1.0f);
            }
            bIsFirstPersonPlayer = this == pPlayer && !pPlayer->Is3rdPerson();
            pCharController = static_cast<bhkCharacterController*>(GetCharController());
            if (pCharController) {
                if (this != pPlayer) {
                    NiPoint3 playerEyePos;
                    if (pPlayer->Is3rdPerson() && p3rdPersonCamera) {
                        playerEyePos = p3rdPersonCamera->GetWorldTranslate();
                    }
                    else if (p1stPersonCamera) {
                        playerEyePos = p1stPersonCamera->GetWorldTranslate();
                    }
                    else {
                        NiPoint3 kEyeOffset = pPlayer->GetEyeLevel() * NiPoint3::UNIT_Z;
                        playerEyePos = *pPlayer->QLocationOnReference() + kEyeOffset;
                    }
                    pCharController->UpdateLODDistance(playerEyePos, pRoot);
                }
                // Sticky while the weapon or the left arm plays a power attack below the counter attack.
                bool bStickyCollision = false;
                if (pAnimData) {
                    uint16_t usWeaponGroup = pAnimData->GetGroup(AGS_WEAPON);
                    if (TESAnimGroup::IsPowerAttackAction(usWeaponGroup) &&
                        TESAnimGroup::GetType(usWeaponGroup) < ANIM_GROUP_COUNTER) {
                        bStickyCollision = true;
                    }
                    else {
                        uint16_t usLeftArmGroup = pAnimData->GetGroup(AGS_LEFT_ARM);
                        if (TESAnimGroup::IsPowerAttackAction(usLeftArmGroup) &&
                            TESAnimGroup::GetType(usLeftArmGroup) < ANIM_GROUP_COUNTER) {
                            bStickyCollision = true;
                        }
                    }
                }
                bhkCharacterListener* pListener = static_cast<bhkCharacterController*>(GetCharController());
                pListener->SetStickyCollisions(bStickyCollision);
            }
            if (pCurrentProcess) {
                pCurrentProcess->TryReserveProjectileClones(this);
                if (cCurrentSitSleepState == SIT_SLEEP_IS_SLEEPING || IsUnConscious()) {
                    if (pFaceAnimData && !pFaceAnimData->GetDead()) {
                        pFaceAnimData->SetDead(true, false);
                    }
                }
                else if (pFaceAnimData && !IsDead(false) && pFaceAnimData->GetDead()) {
                    pFaceAnimData->SetDead(false, false);
                }
                if (GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN &&
                    (pCurrentProcess->GetKnockState() == KNOCK_STATE_OUT ||
                     pCurrentProcess->GetKnockState() == KNOCK_STATE_EXPLODE)) {
                    if (IsPlayerTeammate()) {
                        // A follower's timer only runs down while the player is out of combat.
                        if (!pPlayer || !pPlayer->IsPlayerCharacterInCombat(nullptr)) {
                            pCurrentProcess->ModEssentialDownTimer();
                        }
                        if (pCurrentProcess->GetEssentialDownTimer() <= 0.0) {
                            SetLifeState(ACTOR_LIFE_STATE_ALIVE);
                            RestoreFullHealthAndConditions();
                            FollowerBarks::TriggerFollowerBark(this, FollowerBarks::BARK_CAME_TO);
                        }
                    }
                    else {
                        pCurrentProcess->ModEssentialDownTimer();
                        if (pCurrentProcess->GetEssentialDownTimer() <= 0.0) {
                            SetLifeState(ACTOR_LIFE_STATE_ALIVE);
                            RestoreFullHealthAndConditions();
                        }
                    }
                }
            }
            // The looping attack sound of a weapon that has one (an unarmed weapon only with the spin attack, 0x50):
            // stopped, and an automatic weapon's attack blended out, once the attack ends; started while it plays.
            if (!IsDead(false) &&
                pAnimData &&
                pWeapon &&
                pWeapon->Get3DLoopingShotSound() &&
                pWeapon->Get3DLoopingShotSound()->GetFilename() &&
                (pWeapon->GetWeaponSkill() != ActorValue::UNARMED || pWeapon->GetAttackAnim() == 0x50)) {
                if (!IsAttacking() || IsKnockedOut() || (IsAttacking() && pAnimData->GetGroupAction(AGS_WEAPON) > 1)) {
                    GetExtra()->GetWeaponAttackSound(kAttackSound);
                    if (kAttackSound.IsPlaying()) {
                        kAttackSound.Stop();
                        if (pWeapon->IsAutomaticWeapon() &&
                            pAnimData->GetCurrentSequence(AGS_WEAPON) &&
                            pAnimData->GetCurrentSequence(AGS_WEAPON)->GetAnimGroup()->IsAttackAction() &&
                            pAnimData->GetCurrentSequence(AGS_WEAPON)->GetState() == NiControllerSequence::kAnimating) {
                            if (this == pPlayer) {
                                pPlayer->PlayerCharacter::GetAnimation(true)->BlendOut(AGS_WEAPON, false);
                                pPlayer->PlayerCharacter::GetAnimation(false)->BlendOut(AGS_WEAPON, false);
                            }
                            else {
                                pAnimData->BlendOut(AGS_WEAPON, false);
                            }
                        }
                    }
                    GetExtra()->SetWeaponAttackSound(kAttackSound);
                }
                else if (
                    IsAttacking() &&
                    pAnimData->GetCurrentSequence(AGS_WEAPON) &&
                    TESAnimGroup::IsAttackAction(pAnimData->GetGroup(AGS_WEAPON)) &&
                    pAnimData->GetCurrentSequence(AGS_WEAPON)->GetState() == NiControllerSequence::kAnimating &&
                    pAnimData->GetGroupAction(AGS_WEAPON) < 1
                ) {
                    GetExtra()->GetWeaponAttackSound(kAttackSound);
                    if (!kAttackSound.IsValid()) {
                        NiAVObject* pFollow = GetFireNode();
                        pFollow = pFollow ? pFollow : Get3D();
                        uint32_t uiLoopFlag = TESSound::ConvertLoopFlag(
                            pWeapon->Get3DLoopingShotSound()->GetData().iFlags
                        );
                        uint32_t uiFlags = this != pPlayer
                            ? BSGameSound::SOUND_FLAG_IS_3D
                            : BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_IS_2D_RADIUS;
                        if (this == pPlayer && !pPlayer->IsActually3rdPerson()) {
                            uiFlags |= BSGameSound::SOUND_FLAG_FIRST_PERSON;
                        }
                        uiFlags |= BSGameSound::SOUND_FLAG_DONT_CACHE;
                        TESSound* pSettings = pWeapon->Get3DLoopingShotSound();
                        uint32_t uiTypeFlags = uiLoopFlag | uiFlags | BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN;
                        const char* pFilename = pWeapon->Get3DLoopingShotSound()->GetFilename();
                        kAttackSound = BSAudio::QInstance().GetSoundHandleByFilename(pFilename, uiTypeFlags, pSettings);
                        kAttackSound.SetAssumeSuccess(true);
                        kAttackSound.SetObjectToFollow(pFollow);
                        kAttackSound.SetPosition(GetLocationOnReference());
                    }
                    else if (!kAttackSound.IsPlaying()) {
                        kAttackSound.Play(false);
                    }
                    GetExtra()->SetWeaponAttackSound(kAttackSound);
                }
            }
            if (this == pPlayer) {
                bInCombat = false;
            }
            else {
                TESPackage* pPackage = nullptr;
                if (pCurrentProcess) {
                    pPackage = GetCurrentAIProcess()->GetPackageThatIsRunning();
                }
                if (!bInCombat) {
                    if ((!pPackage ||
                         (pPackage->GetPackType() != PACKAGE_TYPE_USE_ITEM_AT &&
                          pPackage->GetPackType() != PACKAGE_TYPE_USE_WEAPON)) &&
                        GetBlocked()) {
                        SetBlock(false);
                    }
                    // 0x400 is the mover's sneaking flag; there is no enum for the movement flags.
                    if (GetForceSneak() ||
                        (pPackage && pPackage->GetAlwaysSneak()) ||
                        (IsPlayerTeammate() && pPlayer->IsSneaking() && pPlayer->IsActorFollowingPlayer(this))) {
                        if (!IsSneaking()) {
                            SetWalkRunFlagDirty();
                        }
                        if (pActorMover &&
                            !(pActorMover->GetPreferredMoveMode() & 0x400) &&
                            time_followerStealthing <= 0.0) {
                            FollowerBarks::TriggerFollowerBark(this, FollowerBarks::BARK_STEALTHING);
                            time_followerStealthing = 300.0f;
                        }
                        SetMoveMode(static_cast<int32_t>(pActorMover->GetPreferredMoveMode() | 0x400));
                    }
                    else if (
                        IsSneaking() ||
                        (IsPlayerTeammate() && pActorMover && (pActorMover->GetPreferredMoveMode() & 0x400))
                    ) {
                        SetWalkRunFlagDirty();
                        SetMoveMode(static_cast<int32_t>(GetMoveMode() & ~0x400u));
                    }
                }
            }
            if (bIsFirstPersonPlayer) {
                pRoot->SetTranslate(*QLocationOnReference());
                pPlayer->UpdateArmRotation();
            }
            else {
                UpdateActor3DPosition();
                pCurrentProcess->UpdateAimSequences(this);
            }
            LightRefData* pLight = GetLight();
            if (pLight && pLight->spLight) {
                NiLight* pNiLight = pLight->spLight;
                BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->UpdateLight_0(pNiLight);
            }
            NiBlendTransformInterpolator::SetAlwaysBlend(false);
            if (!bIsFirstPersonPlayer) {
                if (pCharController && !Interface::IsInMenuMode() && !IsDead(false)) {
                    // The depth is a fraction of the actor's height: wading from 0.01, swimming from 0.7 and under
                    // water from 0.875 (the compares use the float constants widened to double).
                    float fDepth = IsPointDeepUnderWater(QLocationOnReference()->z, pCell);
                    bool bInWaterNow = fDepth >= 0.009999999776482582;
                    bool bDeep = fDepth >= 0.699999988079071;
                    bool bSubmerged = fDepth >= 0.875;
                    float fBreathTime = GamePlayFormulas::SwimBreathTime(static_cast<int32_t>(GetEndurance()));
                    bool bEnteredWater = !bInWater && bInWaterNow;
                    bInWater = bInWaterNow;
                    bool bCanBreathe = true;
                    if (fDepth <= 0.0) {
                        bCanBreathe = BreathesAboveWater();
                    }
                    else if (
                        bSubmerged && !static_cast<MagicTarget*>(this)->IsInvulnerable() && !BreathesUnderwater()
                    ) {
                        bCanBreathe = false;
                    }
                    if (bCanBreathe) {
                        pCurrentProcess->SetBreathTimer(fBreathTime);
                    }
                    else {
                        float fBreathLeft = pCurrentProcess->GetBreathTimer();
                        fBreathLeft = fBreathLeft - appTimer.GetSeconds();
                        if (fBreathLeft < 0.0) {
                            // Out of breath: an NPC drowns, with its drowning sound and the pain blur.
                            if (IsPc()) {
                                int32_t iHealth = static_cast<ActorValueOwner*>(this)->GetBaseActorValue(
                                    ActorValue::HEALTH
                                );
                                double dBreathDamage = GamePlayFormulas::SwimBreathDamage(iHealth);
                                float fDamage = appTimer.GetSeconds() * dBreathDamage;
                                DoDamage(fDamage, 0.0f, nullptr);
                                BSSoundHandle kDrownSound;
                                GetExtra()->GetSound(kDrownSound);
                                if (!kDrownSound.IsValid()) {
                                    kDrownSound = PlaySoundByEditorName(
                                        "NPCHumanDrowning",
                                        false,
                                        BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT,
                                        false
                                    );
                                    GetExtra()->SetSound(kDrownSound);
                                }
                                TriggerPain(false, true);
                            }
                            fBreathLeft = 0.0f;
                        }
                        else if (this != pPlayer) {
                            // An NPC surfaces when its breath would run out on the way up (1.5 times the swim time
                            // to the surface, plus a second).
                            if (fBreathTime < fBreathLeft) {
                                fBreathLeft = fBreathTime;
                            }
                            float fSwimSpeed = GetSwimWalkSpeed();
                            double dWaterHeight = GetRelevantWaterHeight();
                            float fToSurface = dWaterHeight - GetLocationOnReference().z;
                            float fSurfaceTime = fToSurface / fSwimSpeed;
                            fSurfaceTime = fSurfaceTime * 1.5;
                            if (fBreathLeft < fSurfaceTime + 1.0 && !SwimsOnly() && !IsSurfacing()) {
                                InitiateSurfacePackage();
                            }
                        }
                        pCurrentProcess->SetBreathTimer(fBreathLeft);
                    }
                    bool bDoesFly = false;
                    if (IsCreature()) {
                        bDoesFly = DoesFly();
                        TESCreature* pCreature = static_cast<TESCreature*>(GetObjectReference());
                        TESSound* pAwakeSound = pCreature ? pCreature->PickCreatureSound(CREA_SOUND_AWAKE) : nullptr;
                        if (pAwakeSound && Get3D()) {
                            bool bAwake = !IsUnConscious() && !IsDead(false);
                            BSSoundHandle kAwakeSound;
                            GetExtra()->GetCreatureAwakeSound(kAwakeSound);
                            if (bAwake && !kAwakeSound.IsPlaying()) {
                                if (!kAwakeSound.IsValid()) {
                                    uint32_t uiFlags = TESSound::ConvertLoopFlag(pAwakeSound->GetData().iFlags) |
                                        BSGameSound::SOUND_FLAG_IS_3D;
                                    const char* pFilename = pAwakeSound->GetFilename();
                                    kAwakeSound = BSAudio::QInstance()
                                                      .GetSoundHandleByFilename(pFilename, uiFlags, pAwakeSound);
                                    GetExtra()->SetCreatureAwakeSound(kAwakeSound);
                                    kAwakeSound.SetObjectToFollow(Get3D());
                                }
                                if (!kAwakeSound.IsPlaying()) {
                                    kAwakeSound.SetPosition(GetLocationOnReference());
                                    kAwakeSound.Play(false);
                                }
                            }
                            else if (kAwakeSound.IsPlaying() && (!bAwake || IsDead(false))) {
                                kAwakeSound.Stop();
                                kAwakeSound.Release();
                            }
                        }
                    }
                    if (bDoesFly) {
                        pCharController->SetSwimFloatHeight(0.1f);
                    }
                    else if (this == pPlayer && !pPlayer->IsActually3rdPerson()) {
                        pCharController->SetSwimFloatHeight(fFirstPersonSwimFloatHeight);
                    }
                    else {
                        pCharController->SetSwimFloatHeight(1.6f);
                    }
                    // 0x800 is the mover's swimming flag; -3.4028234663852886e+38 (-FLT_MAX) marks an unknown
                    // water height.
                    if (bDoesFly) {
                        if (pCharController->GetState() == HK_CHARACTER_USER_STATE_0 &&
                            pLoadedData->fRelevantWaterHeight == -3.4028234663852886e+38) {
                            pLoadedData->fRelevantWaterHeight = TES::pInstance->GetWaterHeight(
                                GetLocationOnReference(),
                                nullptr
                            );
                        }
                        if (bDeep) {
                            SetMoveMode(static_cast<int32_t>(pActorMover->GetPreferredMoveMode() | 0x800));
                            if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
                                pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_WALK_SPEED);
                                pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_RUN_SPEED);
                            }
                        }
                        else {
                            ClearMoveMode(0x800);
                            if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
                                pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_WALK_SPEED);
                                pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_RUN_SPEED);
                            }
                        }
                        if (pCharController->GetState() == HK_CHARACTER_USER_STATE_0 && !bInWaterNow) {
                            pCharController->SetWantState(HK_CHARACTER_ON_GROUND);
                            pLoadedData->fRelevantWaterHeight = -std::numeric_limits<float>::max();
                        }
                        else if (
                            pCharController->GetState() != HK_CHARACTER_USER_STATE_0 &&
                            bInWaterNow &&
                            pCharController->OutVelocity(2) < 1.0
                        ) {
                            if (GetRelevantWaterHeight() == -3.4028234663852886e+38) {
                                pLoadedData->fRelevantWaterHeight = TES::pInstance->GetWaterHeight(
                                    GetLocationOnReference(),
                                    nullptr
                                );
                            }
                            pCharController->SetWantState(HK_CHARACTER_USER_STATE_0);
                        }
                        // A flyer floating in shallow water is pushed toward 5 units below the water surface.
                        if (pCharController->GetState() == HK_CHARACTER_USER_STATE_0 &&
                            pCharController->fVelocityTime < 0.009999999776482582 &&
                            bInWaterNow &&
                            !bDeep) {
                            float fRotCenterZ = (*pCharController->GetRotCenter())(2);
                            // The subtraction and addition are done at double precision before the one float store.
                            // GethkPosition returns the position const; its z is read through the non-const operator().
                            float fFloatHeight = static_cast<double>(
                                                     const_cast<hkVector4&>(pCharController->GethkPosition())(2)
                                                 ) -
                                fRotCenterZ + fRotCenterZ * 0.10000000149011612 * pCharController->fScale;
                            float fWaterOffset = pCharController->fWaterHeight - 5.0;
                            float fDiff = fFloatHeight - NI2HK(fWaterOffset);
                            if (fDiff < 0.10000000149011612) {
                                NiPoint3 kUp(0.0f, 0.0f, 5.0f);
                                pCharController->SetVelocityModifier(kUp, 1.0f);
                            }
                            else if (fDiff > 0.10000000149011612) {
                                NiPoint3 kDown(0.0f, 0.0f, -5.0f);
                                pCharController->SetVelocityModifier(kDown, 1.0f);
                            }
                        }
                    }
                    else if (bDeep) {
                        SetMoveMode(static_cast<int32_t>(pActorMover->GetPreferredMoveMode() | 0x800));
                        pCharController->SetWantState(HK_CHARACTER_USER_STATE_0);
                        SetWantWeaponDrawn(false);
                        if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
                            pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_WALK_SPEED);
                            pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_RUN_SPEED);
                        }
                    }
                    else if (
                        IsSwimming() ||
                        (GetMoveMode() & 0x800) ||
                        pCharController->GetState() == HK_CHARACTER_USER_STATE_0
                    ) {
                        if (pCharController->QSupported()) {
                            ClearMoveMode(0x800);
                            pCharController->SetWantState(HK_CHARACTER_ON_GROUND);
                            if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
                                pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_WALK_SPEED);
                                pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_RUN_SPEED);
                            }
                        }
                    }
                    else if (
                        bEnteredWater && pCharController->QFallTime() > 0.5 && pCharController->OutVelocity(2) < -1.0
                    ) {
                        // Falling into the water: the splash sound, its volume the cube of the fall speed over 50,
                        // and in safe water the splash particles.
                        float fVelocityZ = pCharController->OutVelocity(2);
                        float fVolume = 1.0f;
                        if (fVelocityZ > -50.0) {
                            fVolume = fVelocityZ * -1.0 / 50.0;
                        }
                        fVolume = std::powf(fVolume, 3.0f);
                        BSSoundHandle kSplashSound;
                        if (Get3D()) {
                            kSplashSound = BSAudio::QInstance().GetSoundHandleByName(
                                "CWaterHumanoid",
                                BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT
                            );
                            if (!kSplashSound.IsPlaying()) {
                                NiPoint3 kSplashPos = GetLocationOnReference();
                                kSplashSound.SetPosition(kSplashPos.x, kSplashPos.y, kSplashPos.z);
                                kSplashSound.SetVolume(fVolume);
                                kSplashSound.Play(false);
                                if (pCell && pCell->GetWaterType() && !pCell->GetWaterType()->IsDangerous()) {
                                    NiPoint3 kParticlePos = kSplashPos;
                                    kParticlePos.z = GetRelevantWaterHeight();
                                    NiMatrix3 kRotation;
                                    kRotation.MakeZRotation(RandomFloatTwoPi());
                                    // Reads the scale before the particle file name.
                                    float fScale = fSplashScale1.Float();
                                    const char* pParticles = sSplashParticles.String();
                                    BSTempEffectParticle::Spawn(
                                        pCell,
                                        1.0f,
                                        pParticles,
                                        kRotation,
                                        kParticlePos,
                                        fScale,
                                        1,
                                        nullptr
                                    );
                                    float fSplashSize = Get3D()->GetWorldBoundRadius();
                                    BGSWaterCollisionManager::PlaySplashEffects(fSplashSize, kSplashPos);
                                }
                            }
                        }
                    }
                    if (this == pPlayer && pProcess) {
                        pProcess->SetRadiationWaterDelta(0.0f);
                    }
                    if (bInWaterNow && !bDoesFly) {
                        if (pCell) {
                            TESWaterForm* pWater = pCell->GetWaterType();
                            if (pWater && pWater->IsDangerous()) {
                                double dWaterDamage = static_cast<TESAttackDamageForm*>(pWater)->GetAttackDamage();
                                float fDamage = appTimer.GetSeconds() * dWaterDamage;
                                if (fDamage > 0.0) {
                                    DoDamage(fDamage, 0.0f, nullptr);
                                    TriggerPain(true, true);
                                }
                            }
                            if (pWater && pWater->GetRadiationDamage() > 0 && (IsPc() || IsCreature())) {
                                float fRadiation = pWater->GetRadiationDamage();
                                float fMult;
                                if (bDeep || (GetMoveMode() & 0x800)) {
                                    fMult = fSwimRadiationDamageMult.Float();
                                }
                                else {
                                    fMult = fWadeRadiationDamageMult.Float();
                                }
                                fRadiation = fRadiation * fMult;
                                float fEffectiveness = GetRadiationResistanceMult();
                                fRadiation = fRadiation * fEffectiveness;
                                if (this == pPlayer && pProcess) {
                                    pProcess->SetRadiationWaterDelta(fRadiation);
                                    pPlayer->ReportRadiationChange();
                                }
                                float fRads = appTimer.GetSeconds() * fRadiation;
                                if (this == pPlayer) {
                                    ReferenceProcessLists.SetPlayerInRadiatedWater(fRads > 0.0);
                                }
                                DamageModActorValue(ActorValue::RADIATION_RADS, fRads, nullptr);
                            }
                        }
                    }
                    else {
                        ReferenceProcessLists.SetPlayerInRadiatedWater(false);
                    }
                }
                if (IsDying() && pAnimData) {
                    // A dying actor not yet in its death animation is frozen in place once it was processed less
                    // than 0.0025 game hours (times the time scale) ago or its face did not change last update.
                    bool bRecentlyProcessed = true;
                    bool bIsDeathAnim = TESAnimGroup::GetType(pAnimData->GetGroup(AGS_MOVEMENT)) == ANIM_GROUP_DEATH;
                    if (bRecentlyProcessed && !bIsDeathAnim) {
                        float fLastHour = GetCurrentAIProcess()->GetLastTimeProcessed();
                        float fCurrentHour = appCalendar.GetHour();
                        float fHoursPassed = 0.0f;
                        if (fLastHour > fCurrentHour) {
                            fHoursPassed = fLastHour + 23.0 - fCurrentHour;
                        }
                        else {
                            fHoursPassed = fCurrentHour - fLastHour;
                        }
                        bRecentlyProcessed = fHoursPassed < appCalendar.GetTimeScale() * 0.0024999999441206455;
                    }
                    NiNode* pAccumRoot = pAnimData ? pAnimData->GetAccumRoot() : nullptr;
                    NiNode* pAccumRootNode = pAccumRoot ? pAccumRoot->IsNode() : nullptr;
                    if (pAccumRootNode && !bIsDeathAnim && bhkUtilFunctions::GetHeirGain(pAccumRootNode) <= 0.0) {
                        if (this == pPlayer) {
                            Animation* pPlayerAnim = pPlayer->PlayerCharacter::GetAnimation(false);
                            pPlayerAnim->SpecialIdleFree(true, false);
                            pPlayerAnim->ClearGroup(AGS_WHOLE_BODY, 0.0f);
                            pPlayerAnim->ClearAccumRootTranslate();
                            pPlayerAnim = pPlayer->PlayerCharacter::GetAnimation(true);
                            pPlayerAnim->SpecialIdleFree(true, false);
                            pPlayerAnim->ClearGroup(AGS_WHOLE_BODY, 0.0f);
                            pPlayerAnim->ClearAccumRootTranslate();
                        }
                        else {
                            pAnimData->SpecialIdleFree(true, false);
                            pAnimData->ClearGroup(AGS_WHOLE_BODY, 0.0f);
                            pAnimData->ClearAccumRootTranslate();
                        }
                        pCurrentProcess->EndMoveMessage(this);
                    }
                    // The face goes dead once the actor's sound handle 0 (its voice) is no longer valid.
                    bool bSetFaceDead = false;
                    if (pFaceAnimData && !pFaceAnimData->GetDead()) {
                        if (!pCurrentProcess) {
                            bSetFaceDead = true;
                        }
                        else {
                            BSSoundHandle kVoice = pCurrentProcess->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
                            bSetFaceDead = !kVoice.IsValid();
                        }
                    }
                    if (bSetFaceDead) {
                        pFaceAnimData->SetDead(true, false);
                    }
                    if (!bIsDeathAnim &&
                        (!bRecentlyProcessed || !pFaceAnimData || !pFaceAnimData->GetChangedLastUpdate())) {
                        // 0x20 is a post-animation action that has no enum value.
                        bool bClearInterpolators = true;
                        if (pAnimData->GroupLoaded(ANIM_GROUP_DEATH) &&
                            (pAnimData->GetCurrentSequence(animGroupInfo[ANIM_GROUP_DEATH].eSectionLayer) ||
                             (GetCurrentAIProcess()->GetPostAnimationActions() & 0x20))) {
                            bClearInterpolators = false;
                        }
                        SetHavokDeath(true);
                        if (bClearInterpolators) {
                            ClearInterpolators();
                        }
                    }
                }
                else if (IsDead(false)) {
                    if (pCurrentProcess->GetGreetingTimer() == 3.4028234663852886e+38) {
                        pCurrentProcess->SetGreetingTimer(0.0f);
                    }
                    double dGreetingTimer = pCurrentProcess->GetGreetingTimer();
                    float fGreetingTimer = dGreetingTimer - appTimer.GetSeconds();
                    pCurrentProcess->SetGreetingTimer(fGreetingTimer);
                }
            }
            UpdateActorMover(afDelta);
            UpdateCombatData();
        }
    }
    AIProcessing_Update.Stop();
}

void Actor::CleanUpPointersOnDisable() {
    pMyKiller = nullptr;
    ClearDispositionModifiers();
    pCurrentCombatTarget = nullptr;
    if (pCurrentCombatTargetArray) {
        delete pCurrentCombatTargetArray;
    }
    pCurrentCombatTargetArray = nullptr;
    if (pCurrentCombatMemberArray) {
        delete pCurrentCombatMemberArray;
    }
    pCurrentCombatMemberArray = nullptr;
}

MobileObject* Actor::SetUpTalkingActivatorActor(MobileObject* apTarget, MobileObject*& aptalkingactivator) {
    BGSTalkingActivator* pactitalk = static_cast<BGSTalkingActivator*>(apTarget->GetObjectReference());
    apTarget->m_Extra.SetTalkingActorExtra(pactitalk, apTarget);
    apTarget->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    ExtraTalkingActor* pextratalk = apTarget->m_Extra.GetTalkingActorExtra();
    if (pextratalk) {
        apTarget = pextratalk->pTalkObject;
        aptalkingactivator = apTarget;
        if (pactitalk->GetTempRef()) {
            apTarget = pactitalk->GetTempRef();
        }
    }
    pCurrentProcess->SetTarget(apTarget);
    return apTarget;
}

// An actor can always cast: no failure reason and full strength.
bool Actor::CheckCast(
    MagicItem* apSpell,
    float* afEffectStrength,
    MagicItem::CannotCastReason* apReason,
    bool abUseBaseMagicka
) {
    if (apReason) {
        *apReason = MagicItem::NOCAST_OK;
    }
    if (afEffectStrength) {
        *afEffectStrength = 1.0f;
    }
    return true;
}

bool Actor::IsInvulnerable() {
    if (static_cast<TESActorBaseData&>(*GetActorBase()).GetInvulnerable()) {
        return true;
    }
    if (IsCreature()) {
        return GetGhost();
    }
    if (GetGhost()) {
        return true;
    }
    return IsChild(false);
}

int32_t Actor::GetBaseActorValue(ActorValue::Index aiWhich) {
    return static_cast<int32_t>(FloatFloor(GetBaseActorFloatValue(aiWhich)));
}

float Actor::GetBaseActorFloatValue(ActorValue::Index aiWhich) {
    const ActorValue::Index eWhich = aiWhich;
    float fDerivedValue = 0.0f;
    ActorValueOwner& rBaseValues = *GetActorBase();
    float fBaseValue = static_cast<float>(rBaseValues.GetActorValue(aiWhich));
    bool bExists = false;
    float fOverride = GetBaseValueOverride(eWhich, bExists);
    if (bExists && !ActorValue::IsSkill(eWhich)) {
        return fOverride;
    }
    if (IsCreature() && ActorValue::IsFlagSet(eWhich, ActorValue::CREATURE_NOT_DERIVED)) {
        return fBaseValue;
    }
    if (!ActorValue::IsFlagSet(eWhich, ActorValue::DERIVED_ALWAYS) &&
        !IsPc() &&
        !ActorValue::IsFlagSet(eWhich, ActorValue::DERIVED_IF_AUTOCALC) &&
        (!IsPlayerTeammate() || aiWhich != ActorValue::HEALTH)) {
        return fBaseValue;
    }
    if (bExists && ActorValue::IsSkill(eWhich)) {
        fBaseValue = fOverride;
    }
    if (!ActorValue::GetDerivedActorValue(this, eWhich, fDerivedValue)) {
        return fBaseValue;
    }
    if (ActorValue::IsFlagSet(eWhich, ActorValue::DERIVED_PLUS_BASE) || bExists) {
        fDerivedValue = fDerivedValue + fBaseValue;
    }
    float fResult = fDerivedValue;
    return fResult;
}

int32_t Actor::GetActorValue(ActorValue::Index aiWhich) {
    if (!pCurrentProcess) {
        return GetBaseActorValue(aiWhich);
    }
    BaseProcess* pProcess = GetCurrentAIProcess();
    return pProcess->GetActorValue(GetActorBase(), aiWhich, this);
}

// Without a current process, the integer base value is rounded to float, which differs only above 2^24.
float Actor::GetActorFloatValue(ActorValue::Index aiWhich) {
    if (!pCurrentProcess) {
        return static_cast<float>(GetBaseActorValue(aiWhich));
    }
    BaseProcess* pProcess = GetCurrentAIProcess();
    return pProcess->GetActorFloatValue(GetActorBase(), aiWhich, this);
}

float Actor::GetTemporaryModifier(ActorValue::Index aiWhich) {
    // The return is rounded to float.
    float fModifier;
    if (pCurrentProcess) {
        fModifier = pCurrentProcess->GetTemporaryModifier(aiWhich);
    }
    else {
        fModifier = 0.0f;
    }
    return fModifier;
}

float Actor::GetDamageModifier(ActorValue::Index aiWhich) {
    // The return is rounded to float.
    float fModifier;
    if (pCurrentProcess) {
        fModifier = pCurrentProcess->GetDamageModifier(aiWhich);
    }
    else {
        fModifier = 0.0f;
    }
    return fModifier;
}

float Actor::GetPermanentModifier(ActorValue::Index aiWhich) {
    return pPermanentModifiers.GetModifier(static_cast<uint8_t>(aiWhich));
}

uint16_t Actor::GetActorLevel() {
    return GetLevel();
}

// Half the larger horizontal extent of the bound, scaled, at least 32; stored in the process's cache when it has one.
float Actor::CalculateCachedRadius() {
    float fRadius = 0.0f;
    NiPoint3 kMax = GetBoundMax();
    NiPoint3 kMin = GetBoundMin();
    float fWidth = kMax.x - kMin.x;
    float fLength = kMax.y - kMin.y;
    float fSize = BSMAX(fWidth, fLength);
    fSize = GetScale() * fSize;
    fSize = BSMAX(fSize, 32.0f);
    fRadius = fSize * 0.5;
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedRadius(fRadius);
    }
    return fRadius;
}

float Actor::CalculateCachedWidth() {
    float fWidth = 0.0f;
    NiPoint3 kMax = GetBoundMax();
    NiPoint3 kMin = GetBoundMin();
    fWidth = kMax.x - kMin.x;
    double dScale = GetScale();
    float fresult = dScale * fWidth;
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedWidth(fresult);
    }

    return fresult;
}

float Actor::CalculateCachedLength() {
    float fLength = 0.0f;
    NiPoint3 kMax = GetBoundMax();
    NiPoint3 kMin = GetBoundMin();
    fLength = kMax.y - kMin.y;
    double dScale = GetScale();
    float fresult = dScale * fLength;
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedLength(fresult);
    }

    return fresult;
}

// The bound's forward (y) extent, scaled.
float Actor::CalculateCachedForwardLength() {
    float fLength = 0.0f;
    NiPoint3 kMax = GetBoundMax();
    fLength = kMax.y;
    fLength = GetScale() * fLength;
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedForwardLength(fLength);
    }
    return fLength;
}

// The DPS of the held weapon (middle-high and high processes) or of the worn one, with its condition.
float Actor::CalculateCachedWeaponDPS() {
    float fDPS = 0.0f;
    TESBoundObject* pWeapon = nullptr;
    float fCondition = 1.0f;
    ItemChange* pItem = nullptr;
    if (GetCurrentAIProcess() && GetCurrentAIProcess()->GetProcessLevel() <= PROCESS_TYPE_MIDDLE_HIGH) {
        pItem = GetCurrentAIProcess()->GetCurrentWeapon();
        if (pItem) {
            pWeapon = pItem->GetContainerObject();
            fCondition = pItem->GetItemHealth(true) / 100.0;
        }
    }
    else {
        InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(this);
        pItem = pChanges->GetWornItem(BO_WEAPON, false);
        if (pItem) {
            pWeapon = pItem->GetContainerObject();
            fCondition = pItem->GetItemHealth(true) / 100.0;
        }
        if (pItem) {
            delete pItem;
        }
        pItem = nullptr;
    }
    fDPS = CombatUtilities::CalculateItemDPS(
        static_cast<ActorValueOwner*>(this),
        static_cast<TESObjectWEAP*>(pWeapon),
        fCondition,
        false,
        pItem,
        false,
        false,
        -1,
        0.0f,
        0.0f,
        false,
        false,
        nullptr
    );
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedWeaponDPS(fDPS);
    }
    return fDPS;
}

float Actor::CalculateCachedMedicineEffectivenessMult() {
    float fMult = MagicFormulas::CalcMedicineEffectivenessMult(static_cast<ActorValueOwner*>(this));
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedMedicineEffectivenessMult(fMult);
    }
    return fMult;
}

float Actor::CalculateCachedSurvivalEffectivenessMult() {
    float fMult = MagicFormulas::CalcSurvivalEffectivenessMult(static_cast<ActorValueOwner*>(this));
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedSurvivalEffectivenessMult(fMult);
    }
    return fMult;
}

float Actor::CalculateCachedParalysis() {
    float fParalysis = static_cast<ActorValueOwner*>(this)->GetActorFloatValue(ActorValue::PARALYSIS);
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedParalysis(fParalysis);
    }
    return fParalysis;
}

float Actor::CalculateCachedHealingRate() {
    float fRate = static_cast<ActorValueOwner*>(this)->GetActorFloatValue(ActorValue::HEALING_RATE);
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedHealingRate(fRate);
    }
    return fRate;
}

// The endurance cached on the process: the actor's healing rate, stored when the process caches values. This is
// the CachedValuesOwner entry (this = subobject at +0xA8).
float Actor::CalculateCachedEndurance() {
    float fEndurance = static_cast<ActorValueOwner*>(this)->GetActorFloatValue(ActorValue::HEALING_RATE);
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedEndurance(fEndurance);
    }
    return fEndurance;
}

float Actor::CalculateCachedPerceptionCondition() {
    float fresult = static_cast<ActorValueOwner*>(this)->GetActorFloatValue(ActorValue::PERCEPTION_CONDITION);
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedPerceptionCondition(fresult);
    }

    return fresult;
}

float Actor::CalculateCachedEyeLevel() {
    float fresult = 0.0f;
    if (GetActorType()) {
        double dEyeHeight = pPlayer->GetEyeHeight();
        fresult = GetScale() * dEyeHeight;
    }
    else {
        NiPoint3 kMax = GetBoundMax();
        fresult = (kMax.z - GetBoundMin().z) * 0.75;
    }
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedEyeLevel(fresult);
    }

    return fresult;
}

int32_t Actor::CalculateCachedActorAggression() {
    int32_t eActorAggression = static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::AGGRESSION);
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedActorAggression(eActorAggression);
    }
    return eActorAggression;
}

int32_t Actor::CalculateCachedActorAssistance() {
    int32_t eAssistance = static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::ASSISTANCE);
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedActorAssistance(eAssistance);
    }
    return eAssistance;
}

float Actor::CalculateCachedWalkSpeed() {
    float fSpeed = CalculateWalkSpeed();
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedWalkSpeed(fSpeed);
    }
    return fSpeed;
}

float Actor::CalculateCachedRunSpeed() {
    float fSpeed = CalculateRunSpeed();
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedRunSpeed(fSpeed);
    }
    return fSpeed;
}

// Both legs crippled, or crippled limbs ignored, keeps the actor from running; cached on the process when it has one.
bool Actor::CalculateCachedConditionPreventsRun() {
    bool bPreventsRun = false;
    if ((CombatFormulas::GetBodyPartCondition(
             static_cast<ActorValueOwner*>(this),
             ActorValue::LEFT_MOBILITY_CONDITION,
             false
         ) == 0.0 &&
         CombatFormulas::GetBodyPartCondition(
             static_cast<ActorValueOwner*>(this),
             ActorValue::RIGHT_MOBILITY_CONDITION,
             false
         ) == 0.0) ||
        static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::IGNORE_CRIPPLED_LIMBS)) {
        bPreventsRun = true;
    }
    if (pCurrentProcess) {
        pCurrentProcess->SetCachedConditionPreventsRun(bPreventsRun);
    }
    return bPreventsRun;
}

// Brings the actor back to life: clears its death state, crimes and dismemberment; a get-up resurrection with 3D
// restores health and havok in place, otherwise the actor's modifiers, process and (with abResetInventory) inventory
// are reset and it is requeued at its process level (its 3D loaded with abAttach3D).
void Actor::Resurrect(bool abResetInventory, bool abAttach3D, bool abGetupResurect) {
    SetDelete(false);
    GetExtra()->RemoveSavedHavokData();
    GetExtra()->RemoveDismembermentExtra();
    RemoveChange(CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS);
    if (GetCriticalStage()) {
        SetCriticalStage(ACS_NONE);
    }
    bSetOnDeath = false;
    SetWalkRunFlagDirty();
    ReferenceProcessLists.RemoveAllCrimesforAnActor(this);
    GetExtra()->RemovePlayerCrimeListExtra(this);
    if (abGetupResurect && Get3D()) {
        SetLifeState(ACTOR_LIFE_STATE_ALIVE);
        ActorValueOwner* pOwner = this;
        float fCurrent = pOwner->GetActorFloatValue(ActorValue::HEALTH);
        float fBase = static_cast<float>(pOwner->GetBaseActorValue(ActorValue::HEALTH));
        float fDelta = fBase - fCurrent;
        DamageModActorValue(ActorValue::HEALTH, fDelta, nullptr);
        pCurrentProcess->SetCharController(NiPointer<bhkCharacterController>(nullptr));
        InitHavok();
        bhkWorld::SetMotion(Get3D(), hkpMotion::MOTION_DYNAMIC, true, false, true);
        if (pCurrentProcess) {
            pCurrentProcess->SetKnockState(3);
        }
        return;
    }
    RemoveChange(CHANGE_REFR_HAVOK_MOVE);
    pPermanentModifiers.DeleteAllModifiers();
    pBaseValueOverrides.DeleteAllModifiers();
    bDeadFlag = false;
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    if (this != pPlayer) {
        if (pCurrentProcess) {
            ReferenceProcessLists.RemoveReference(this, pCurrentProcess->GetProcessLevel());
        }
        delete pCurrentProcess;
        pCurrentProcess = new LowProcess;
    }
    if (abResetInventory) {
        ResetInventory(false);
    }
    SetLifeState(ACTOR_LIFE_STATE_ALIVE);
    if (this != pPlayer) {
        TESObjectCELL* pCell = GetParentCell();
        if (pCell && TES::pInstance->IsCellLoaded(pCell, false) && abAttach3D) {
            IO_TASK_PRIORITY ePriority = TES::pInstance->GetCellPriority(pCell, nullptr);
            pModelLoader->QueueReference(this, ePriority, false);
            if (pCell->GetAttached()) {
                MovetoHigh();
            }
            else {
                MovetoMiddleHigh();
            }
        }
        else {
            switch (GetDesiredProcessLevel()) {
                case PROCESS_TYPE_HIGH: MovetoHigh(); break;
                case PROCESS_TYPE_MIDDLE_HIGH: MovetoMiddleHigh(); break;
                case PROCESS_TYPE_MIDDLE_LOW: MovetoMiddleLow(); break;
                case PROCESS_TYPE_LOW:
                    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_LOW, false, false, nullptr);
                    break;
                default: break;
            }
        }
    }
    ReferenceProcessLists.kUpdateLock.Unlock();
}

bool Actor::DoDamage(float afHealthDamage, float afFatigueDamage, Actor* apSource) {
    if (static_cast<MagicTarget*>(this)->IsInvulnerable()) {
        return false;
    }
    float ftotalDamage = -DifficultyLevelAdjustHealthModifier(-afHealthDamage, apSource);
    if (ftotalDamage > 0.0) {
        if (apSource && apSource->GetCombatController()) {
            apSource->GetCombatController()->ForceFlee(this, ftotalDamage);
        }

        if (GetCombatController()) {
            QCombatController()->DamagedByAttacker(apSource, ftotalDamage);
        }
        if (!IsPc() && pCurrentProcess && !GetEssential()) {
            pCurrentProcess->ModTrackedDamage(apSource, ftotalDamage);
        }

        DamageModActorValue(ActorValue::HEALTH, -ftotalDamage, apSource);
        TESObjectREFR::DamageObject(afHealthDamage, false);
    }
    if (afFatigueDamage > 0.0) {
        double dFatigue = GetFatigue();
        if (fMinimumFatigue.Float() <= dFatigue) {
            DamageModActorValue(ActorValue::FATIGUE, -afFatigueDamage, apSource);
        }
    }
    return IsDead(false);
}

// The disposition of this actor towards apTargetActor, 0 to 100. apData supplies this actor's race, bases, owner,
// base disposition and responsibility when the caller already has them.
int32_t Actor::GetDisposition(Actor* apTargetActor, DispositionParamData* apData) {
    int32_t iDisposition = 0;
    if (!apTargetActor) {
        return iDisposition;
    }
    Actor* pCommandingActor = nullptr;
    TESRace* pRace = nullptr;
    TESRace* pOwnRace = GetRace();
    TESForm* pNpc = nullptr;
    TESActorBase* pActorBase = nullptr;
    int32_t iBaseDisposition = 0;
    int32_t iResponsibility = 0;
    TESForm* pOwner = nullptr;
    TESActorBase* pTargetBase = apTargetActor->GetActorBase();
    if (pTargetBase && pTargetBase->GetFactionList()->IsEmpty()) {
        pTargetBase = apTargetActor->GetActorBase();
    }
    if (apData) {
        pRace = apData->pRace;
        pOwner = apData->pOwner;
        iBaseDisposition = apData->iBaseDisposition;
        iResponsibility = apData->iResponsibility;
        pNpc = apData->pNpc;
        pActorBase = apData->pActorBase;
    }
    else {
        pRace = GetRace();
        pOwner = GetOwner();
        pActorBase = GetActorBase();
        iBaseDisposition = pActorBase->GetBaseDisposition();
        iResponsibility = static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::RESPONSIBILITY);
        if (IsNpc()) {
            pNpc = GetObjectReference();
        }
    }
    BaseProcess* pProcess = GetCurrentAIProcess();
    if (pProcess) {
        pCommandingActor = pProcess->GetCommandingActor();
    }
    if (apTargetActor->GetObjectReference() == pNpc) {
        return 100;
    }
    if ((!IsNpc() && (pOwner || pCommandingActor)) || IsReanimated()) {
        // A creature with an NPC owner or commander, or a reanimated actor, takes the disposition of that NPC.
        if (!pNpc) {
            if (pOwner && pOwner->GetFormType() == NPC__ID) {
                pNpc = pOwner;
            }
            if (!pNpc && pCommandingActor && pCommandingActor->GetObjectReference()->GetFormType() == NPC__ID) {
                pNpc = pCommandingActor->GetObjectReference();
            }
        }
        if (pNpc && pNpc == apTargetActor->GetObjectReference()) {
            return 100;
        }
        if (pNpc) {
            TESForm* pDispositionNpc = pNpc;
            if (pTargetBase && pDispositionNpc) {
                int32_t iRaceReaction = 0;
                if (apTargetActor->IsNpc()) {
                    iRaceReaction = pRace->GetReaction(pOwnRace);
                }
                int32_t iFactionRank = -1;
                TESFaction* pThisFaction = nullptr;
                TESFaction* pTargetFaction = nullptr;
                int32_t
                    iFactionReaction = GetFactionReaction(apTargetActor, iFactionRank, pThisFaction, pTargetFaction);
                int32_t iWeaponStatus = 0;
                if (apTargetActor->IsWeaponDrawn()) {
                    if (!apTargetActor->IsInCombat()) {
                        iWeaponStatus = 1;
                    }
                    else {
                        CombatController* pCombatController = GetCombatController();
                        if (pCombatController && pCombatController->IsActoraCombatTarget(this)) {
                            iWeaponStatus = 1;
                        }
                    }
                }
                int32_t iDiseaseStatus = 0;
                Actor* pNpcActor = nullptr;
                int32_t iTargetMod = 0;
                if (pNpc == pPlayer->GetObjectReference()) {
                    pNpcActor = pPlayer;
                }
                else {
                    pNpcActor = ReferenceProcessLists.GetActorRefInHigh(static_cast<TESNPC*>(pNpc), false);
                }
                if (pNpcActor) {
                    iTargetMod = static_cast<int32_t>(pNpcActor->GetDispositionModifierTowardActor(apTargetActor));
                }
                bool bCreature = IsCreature();
                int32_t iKarma = static_cast<ActorValueOwner*>(apTargetActor)->GetActorValue(ActorValue::KARMA);
                int32_t iCharisma = static_cast<ActorValueOwner*>(apTargetActor)
                                        ->GetClampedActorValue(ActorValue::CHARISMA);
                iDisposition = AiFormulas::GetActorDispositiontoTarget(
                    iBaseDisposition,
                    iCharisma,
                    iResponsibility,
                    iKarma,
                    iRaceReaction,
                    iFactionReaction,
                    iFactionRank,
                    iWeaponStatus,
                    iDiseaseStatus,
                    iTargetMod,
                    bCreature
                );
            }
        }
    }
    else {
        if (!pNpc && pOwner && pOwner->GetFormType() == NPC__ID) {
            pNpc = pOwner;
        }
        if (pNpc && pNpc == apTargetActor->GetObjectReference()) {
            return 100;
        }
        if (apTargetActor->GetActorBase() && pActorBase) {
            int32_t iRaceReaction = 0;
            if (pRace && pOwnRace) {
                iRaceReaction = pRace->GetReaction(pOwnRace);
            }
            TESFaction* pThisFaction = nullptr;
            int32_t iFactionRank = -1;
            TESFaction* pTargetFaction = nullptr;
            int32_t iFactionReaction = GetFactionReaction(apTargetActor, iFactionRank, pThisFaction, pTargetFaction);
            int32_t iWeaponStatus = apTargetActor->IsWeaponDrawn();
            int32_t iDiseaseStatus = 0;
            int32_t iTargetMod = static_cast<int32_t>(GetDispositionModifierTowardActor(apTargetActor));
            bool bCreature = IsCreature();
            int32_t iKarma = static_cast<ActorValueOwner*>(apTargetActor)->GetActorValue(ActorValue::KARMA);
            int32_t iCharisma = static_cast<ActorValueOwner*>(apTargetActor)
                                    ->GetClampedActorValue(ActorValue::CHARISMA);
            iDisposition = AiFormulas::GetActorDispositiontoTarget(
                iBaseDisposition,
                iCharisma,
                iResponsibility,
                iKarma,
                iRaceReaction,
                iFactionReaction,
                iFactionRank,
                iWeaponStatus,
                iDiseaseStatus,
                iTargetMod,
                bCreature
            );
        }
    }
    float fDisposition = static_cast<float>(iDisposition);
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::GET_DISPOSITION, this, apTargetActor, &fDisposition);
    iDisposition = static_cast<int32_t>(fDisposition);
    if (iDisposition > 100) {
        iDisposition = 100;
    }
    else if (iDisposition < 0) {
        iDisposition = 0;
    }
    return iDisposition;
}

void Actor::UpdateMovement(float afDelta, bool abSleeping) {
    float ftargetAngle;
    CFilter filter;
    NiPoint3 kworldDelta;
    alignas(16) NiPoint3 delta;
    AIProcessing_UpdateMovement.Start();
    float fdelta = afDelta;
    if (fdelta == 0.0 && this->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
        if (pVATS.eMode == VATS::VATS_PLAYBACK && this == pVATSTarget) {
            double ddelta = appTimer.fDelta;
            fdelta = pVATS.GetTargetUpdateMult() * ddelta;
        }
        else {
            fdelta = appTimer.fDelta;
        }
    }
    this->SetSwimming();
    if (this != pPlayer) {
        delta = NiPoint3::ZERO;
        uint32_t imoveMode = 0;
        if (!this->IsImmobile() && !this->GetKnockedDownMovementDelta(delta)) {
            imoveMode = this->pActorMover->GetMoveMode();
            if (!this->pActorMover->GetMoveDelta(delta)) {
                Animation* panim = this->Actor::GetAnimation();
                if (panim) {
                    panim->GetDelta(&delta, this, false, true);
                }
            }
        }
        TESObjectCELL* pparent = this->GetParentCell();
        if (pparent && pparent->GetAttached() && !pPlayer->IsSleepingorResting()) {
            this->Move(fdelta, delta, imoveMode);
        }
        else {
            alignas(16) NiMatrix3 rot;
            rot.MakeZRotation(this->GetAngleOnReference().z);
            kworldDelta = rot * delta;
            this->SetLocationOnReference(this->GetLocationOnReference() + kworldDelta);
        }
    }
    if (this->IsInCombat()) {
        this->CheckInCombat();
    }
    bhkCharacterController* pcharController = this->pCurrentProcess ? this->pCurrentProcess->GetCharController()
                                                                    : nullptr;
    if (this != pPlayer) {
        NiAVObject* pbound3d = this->Get3D();
        if (this != pPlayer && pbound3d) {
            alignas(16) NiPoint3 playerEye;
            if (pPlayer->Is3rdPerson() && p3rdPersonCamera) {
                playerEye = p3rdPersonCamera->GetWorldTranslate();
            }
            else if (p1stPersonCamera) {
                playerEye = p1stPersonCamera->GetWorldTranslate();
            }
            else {
                NiPoint3 kEyeOffset = pPlayer->GetEyeLevel() * NiPoint3::UNIT_Z;
                playerEye = *pPlayer->QLocationOnReference() + kEyeOffset;
            }
            NiBound kBound = pbound3d->GetWorldBound();
            double distance = (playerEye - kBound.m_kCenter).Length();
            float fedgeDistance = distance - kBound.m_fRadius * 1.25;
            bool bfar = false;
            BSFadeNode* pfade = pbound3d->IsFadeNode();
            if (pfade) {
                float ffadeDistance = fedgeDistance / BSFadeNode::QFadeOutMultiplier(LOD_MULT_ACTORS);
                bfar = static_cast<double>(ffadeDistance) > pfade->fFarDistSqr;
            }
            BSShaderAccumulator* paccum = BSShaderManager::GetAccumulator();
            if (paccum && paccum->GetBoundTestGeometry()) {
                int32_t iruntest;
                if (bfar) {
                    iruntest = 0;
                }
                else {
                    iruntest = fedgeDistance > 0.0;
                }
                paccum->AddAndUpdateBoundVolume(this->iFormID, kBound, iruntest);
            }

            if (pcharController) {
                pcharController->SetFarAway(bfar);
            }
        }
    }
    if (pcharController) {
        pcharController->SetQuickSimulate(Main::GetSceneIsComplex());
    }
    if (this->bDeadFlag && this->pCurrentProcess) {
        NiAVObject* p3dObj = this->TESObjectREFR::Get3D();
        if (p3dObj) {
            bool bNoRagdoll = true;
            Actor* pcreature = nullptr;
            if (this->GetObjectReference()->GetFormType() == CREA_ID) {
                pcreature = this;
            }
            if (pcreature && !pcreature->HasRagDoll()) {
                bNoRagdoll = false;
            }

            if (this->pRagdollController && this->pRagdollController->GetRagdollAnimEnable()) {
                this->pRagdollController->DisableRagdollAnim(true);
            }

            if (this->pPenetrationDetection) {
                alignas(16) CFilter kFilter;
                // GetGroup is called on the filter word of kFilter that GetCollisionFilter fills in.
                this->GetCollisionFilter(&kFilter.iFilter);
                this->pPenetrationDetection->Initialize(p3dObj, kFilter.GetGroup());
            }
            this->SetLifeState(ACTOR_LIFE_STATE_DYING);
            this->DetachCharController();
            if (bNoRagdoll) {
                NiMatrix3 rot;
                rot.MakeZRotation(this->GetHeading(false));
                alignas(16) NiPoint3 dr(0.0f, 1.0f, 0.0f);
                dr = rot * dr;
                bhkBlendController::DoKnockDown(p3dObj, dr, 1, 0.0f, 0);
                this->AddChange(CHANGE_REFR_HAVOK_MOVE);
                this->pCurrentProcess->ComputeLastTimeProcessed();
            }
            else {
                Animation* panim = this->GetAnimation();
                if (panim && panim->GroupLoaded(224)) {
                    panim->ForceSection(AGS_WHOLE_BODY, ANIM_GROUP_DEATH, AGA_NONE, 0.0f, -1);
                    BSAnimGroupSequence* pseq = panim->GetCurrentSequence(AGS_MOVEMENT);
                    if (pseq) {
                        pseq->SetOffset(pseq->m_fEndKeyTime);
                        panim->Update(this, 0.0f, pseq->m_fEndKeyTime);
                        panim->UpdateMovement(this);
                    }
                    this->DropAddOnObjects();
                }
            }
            this->bDeadFlag = false;
        }
    }

    if (this->GetTurretBehavior() && !this->IsDead(false) && !this->IsUnConscious()) {
        NiAVObject* proot = this->Get3D();
        NiAVObject* ppivot = BSUtilities::GetObjectByName(proot, FixedStrings::QBip01Yaw());
        bool bmotionChanged = false;
        if (ppivot) {
            bhkBlendCollisionObject* pBlendCollisionObject = static_cast<bhkBlendCollisionObject*>(
                NiObject::DynamicCastStatic(&bhkBlendCollisionObject::ms_RTTI, ppivot->GetCollisionObject())
            );
            if (pBlendCollisionObject) {
                bhkRigidBody* prigid = pBlendCollisionObject->GetRigidBody();
                BSSimpleList<NiPointer<bhkSerializable>>* pactCon = &prigid->kActionConstrants;
                alignas(16) hkpMotion::MotionType eMotion = static_cast<hkpMotion::MotionType>(prigid->GetMotionType());
                if (!bhkRigidBody::IsMotionTypeDynamic(eMotion)) {
                    bhkWorld::SetMotion(ppivot, hkpMotion::MOTION_DYNAMIC, true, true, true);
                    bhkWorld* pw = bhkWorld::Getbhk(prigid->GethkWorld());
                    prigid->AddActConToWorld(pw);
                    bmotionChanged = true;
                }
                prigid->GetCollisionFilter(&filter);
                if (filter.GetLayer() == L_DEADBIP) {
                    bhkUtilFunctions::SetLayer(proot, L_BIPED);
                }
                prigid->SetActive(true);
                while (pactCon && pactCon->m_item) {
                    bhkOrientHingedBodyAction* porientAction = static_cast<bhkOrientHingedBodyAction*>(
                        NiObject::DynamicCastStatic(&bhkOrientHingedBodyAction::ms_RTTI, pactCon->m_item)
                    );
                    if (porientAction) {
                        float fheading = this->GetHeading(false);
                        float fcosHeading = NiCos(fheading);
                        float fRadians = NiSin(fheading);
                        hkVector4 dirHK(fRadians, fcosHeading, 0.0f, 0.0f);
                        porientAction->SetEnabled(true);
                        porientAction->SetTargetDir(dirHK);
                        break;
                    }
                    pactCon = pactCon->m_pkNext;
                }
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    TaskQueueInterface::GetInstance()->QueueActorHavokSyncTask(this, ppivot);
                }
                else {
                    bhkNiCollisionObject::Synchronize(ppivot, bhkNiCollisionObject::SYNC_FROMHAVOK);
                }
            }
        }
        ppivot = BSUtilities::GetObjectByName(proot, FixedStrings::QBip01Pitch());
        if (ppivot) {
            bhkBlendCollisionObject* pBlendCollisionObject = static_cast<bhkBlendCollisionObject*>(
                NiObject::DynamicCastStatic(&bhkBlendCollisionObject::ms_RTTI, ppivot->GetCollisionObject())
            );
            if (pBlendCollisionObject) {
                bhkRigidBody* prigid = pBlendCollisionObject->GetRigidBody();
                BSSimpleList<NiPointer<bhkSerializable>>* pactCon = &prigid->kActionConstrants;
                alignas(16) hkpMotion::MotionType eMotion = static_cast<hkpMotion::MotionType>(prigid->GetMotionType());
                if (!bhkRigidBody::IsMotionTypeDynamic(eMotion)) {
                    bhkWorld::SetMotion(ppivot, hkpMotion::MOTION_DYNAMIC, true, true, true);
                    bhkWorld* pw = bhkWorld::Getbhk(prigid->GethkWorld());
                    prigid->AddActConToWorld(pw);
                    bmotionChanged = true;
                }
                prigid->GetCollisionFilter(&filter);
                if (filter.GetLayer() == L_DEADBIP) {
                    bhkUtilFunctions::SetLayer(proot, L_BIPED);
                }
                prigid->SetActive(true);
                while (pactCon && pactCon->m_item) {
                    bhkSerializable* pconstraint = pactCon->m_item;
                    bhkLimitedHingeConstraint* phinge = pconstraint->IsBhkLimitedHingeConstraint();
                    if (phinge) {
                        hkpLimitedHingeConstraintData* phingeData = phinge->GetMotor();
                        ftargetAngle = this->data.kRotation.x;
                        float fmaxAngle = phingeData->m_atoms.m_angLimit.m_maxAngle;
                        float fminAngle = phingeData->m_atoms.m_angLimit.m_minAngle;
                        BSClamp(ftargetAngle, fminAngle, fmaxAngle);
                        phinge->SetMotorActive(true);
                        phinge->SetMotorTargetAngle(ftargetAngle);
                        break;
                    }
                    pactCon = pactCon->m_pkNext;
                }
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    TaskQueueInterface::GetInstance()->QueueActorHavokSyncTask(this, ppivot);
                }
                else {
                    bhkNiCollisionObject::Synchronize(ppivot, bhkNiCollisionObject::SYNC_FROMHAVOK);
                }
            }
        }
        if (bmotionChanged) {
            this->AddChange(CHANGE_REFR_HAVOK_MOVE);
        }
    }
    AIProcessing_UpdateMovement.Stop();
}

void Actor::UpdateNonRenderSafeMovement(float afDelta, bool abSleeping) {
    NiPoint3 kLocation;
    if (!TESForm::IsStillLoading()) {
        if (pVATS.eMode == VATS::VATS_PLAYBACK &&
            static_cast<const void*>(this) == static_cast<const void*>(pVATSTarget)) {
            afDelta = pVATS.GetTargetUpdateMult() * afDelta;
        }
        if (pCurrentProcess && !Interface::IsMenuIDVisible(Interface::eContainerMenu, 0)) {
            pCurrentProcess->Update3dModel(this);
            if (pCurrentProcess->GetCurrentMuzzleFlash()) {
                pCurrentProcess->GetCurrentMuzzleFlash()->Update(afDelta, this);
            }
        }

        if (TESObjectREFR::GetObjectReference()) {
            TESObjectCELL* pParentCell = TESObjectREFR::GetParentCell();
            kLocation = GetLocationOnReference();
            if (pParentCell && !pParentCell->GetInterior() && !pParentCell->IsPointInCellExterior(kLocation)) {
                TESObjectREFR::MoveRefToNewSpace(this, nullptr, pParentCell->GetWorldSpace());
            }
        }
        if (pCurrentProcess && pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
            static_cast<HighProcess*>(pCurrentProcess)->FadeUpdate(this);
        }
    }
}

void Actor::UpdateAnimationMovementNoWorldUpdate() {
    Animation* pAnimation = static_cast<TESObjectREFR*>(this)->GetAnimation();
    if (pAnimation) {
        pAnimation->UpdateMovementNoWorldUpdate(this);
    }
}

float Actor::GetReach() {
    return CombatFormulas::CalcWeaponReach(fHandReachMult.Float());
}

void Actor::SetActorValue(ActorValue::Index aiWhich, int32_t aiValue) {
    SetActorValue(aiWhich, static_cast<float>(aiValue));
}

// Sets the base value override, clamping an enumerated value to its range, marks the process's cached value out of date
// and runs the value's modified callback with the value before (0 when the value has no callback). The comparison with
// the enum count and the subtraction of 1.0 are done in double.
void Actor::SetActorValue(ActorValue::Index aiWhich, float afValue) {
    float fOldValue;
    if (ActorValue::HasModifiedCallback(aiWhich)) {
        fOldValue = static_cast<ActorValueOwner*>(this)->GetActorFloatValue(aiWhich);
    }
    else {
        fOldValue = 0.0f;
    }
    if (ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        if (afValue < 0.0) {
            afValue = 0.0f;
        }
        else {
            ActorValueInfo* pInfo = ActorValue::GetActorValueInfo(aiWhich);
            if (pInfo && static_cast<double>(afValue) >= static_cast<double>(static_cast<int32_t>(pInfo->iEnumCount))) {
                afValue = static_cast<float>(static_cast<double>(static_cast<int32_t>(pInfo->iEnumCount)) - 1.0);
            }
        }
    }
    SetBaseValueOverride(aiWhich, afValue);
    if (!IsPc() && GetCurrentAIProcess()) {
        GetCurrentAIProcess()->SetCachedActorValueOutOfDate(aiWhich);
    }
    ActorValue::CheckCallModifiedCallback(this, aiWhich, fOldValue, afValue, nullptr);
}

void Actor::TempModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
    if ((aiWhich != ActorValue::FATIGUE || (aiValue >= 0 || HasRagDoll())) &&
        !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        ActorValueOwner* pOwner;
        float foldValue;
        int32_t iadjustedValue = DifficultyLevelCheckModHealthI(aiWhich, aiValue, apAttacker);
        if (ActorValue::HasModifiedCallback(aiWhich)) {
            foldValue = GetActorFloatValue(aiWhich);
        }
        else {
            foldValue = 0.0f;
        }
        if (pCurrentProcess) {
            pCurrentProcess->TempModActorValue(this, aiWhich, iadjustedValue);
        }
        if (aiWhich == ActorValue::HEALTH && iadjustedValue < 0) {
            HandleHealthDamage(apAttacker, static_cast<float>(iadjustedValue));
        }
        AddChange(CHANGE_ACTOR_TEMP_MODIFIERS);
        CheckSkillLevelChange(aiWhich, false);
        ActorValueOwner* pAttackerOwner = static_cast<ActorValueOwner*>(apAttacker);
        if (this) {
            pOwner = static_cast<ActorValueOwner*>(this);
        }
        else {
            pOwner = nullptr;
        }
        ActorValue::CheckCallModifiedCallback(
            pOwner,
            aiWhich,
            foldValue,
            static_cast<float>(iadjustedValue),
            pAttackerOwner
        );
    }
}

void Actor::TempModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
    if ((aiWhich != ActorValue::FATIGUE || (!(afValue < 0.0) || HasRagDoll())) &&
        !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        ActorValueOwner* pOwner;
        float foldValue;
        float fadjustedValue = DifficultyLevelCheckModHealth(aiWhich, afValue, apAttacker);
        if (ActorValue::HasModifiedCallback(aiWhich)) {
            foldValue = GetActorFloatValue(aiWhich);
        }
        else {
            foldValue = 0.0f;
        }
        if (pCurrentProcess) {
            pCurrentProcess->TempModActorValue(this, aiWhich, fadjustedValue);
        }
        if (aiWhich == ActorValue::HEALTH && fadjustedValue < 0.0) {
            HandleHealthDamage(apAttacker, fadjustedValue);
        }
        AddChange(CHANGE_ACTOR_TEMP_MODIFIERS);
        CheckSkillLevelChange(aiWhich, false);
        ActorValueOwner* pAttackerOwner = static_cast<ActorValueOwner*>(apAttacker);
        if (this) {
            pOwner = static_cast<ActorValueOwner*>(this);
        }
        else {
            pOwner = nullptr;
        }
        ActorValue::CheckCallModifiedCallback(pOwner, aiWhich, foldValue, fadjustedValue, pAttackerOwner);
    }
}

void Actor::PermanentModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
    if ((aiWhich != ActorValue::FATIGUE || (aiValue >= 0 || HasRagDoll())) &&
        !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        ActorValueOwner* pOwner;
        float foldValue;
        int32_t iadjustedValue = DifficultyLevelCheckModHealthI(aiWhich, aiValue, apAttacker);
        if (ActorValue::HasModifiedCallback(aiWhich)) {
            foldValue = GetActorFloatValue(aiWhich);
        }
        else {
            foldValue = 0.0f;
        }
        pPermanentModifiers.AdjustModifier(aiWhich, static_cast<float>(iadjustedValue), ModifierList::NO_CLAMP);
        if (aiWhich == ActorValue::HEALTH && iadjustedValue < 0) {
            HandleHealthDamage(apAttacker, static_cast<float>(iadjustedValue));
        }
        AddChange(CHANGE_ACTOR_PERMANENT_MODIFIERS);
        CheckSkillLevelChange(aiWhich, false);
        if (pCurrentProcess) {
            pCurrentProcess->SetCachedActorValueOutOfDate(aiWhich);
        }
        ActorValueOwner* pAttackerOwner = static_cast<ActorValueOwner*>(apAttacker);
        if (this) {
            pOwner = static_cast<ActorValueOwner*>(this);
        }
        else {
            pOwner = nullptr;
        }
        ActorValue::CheckCallModifiedCallback(
            pOwner,
            aiWhich,
            foldValue,
            static_cast<float>(iadjustedValue),
            pAttackerOwner
        );
    }
}

void Actor::PermanentModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
    if ((aiWhich != ActorValue::FATIGUE || (!(afValue < 0.0) || HasRagDoll())) &&
        !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        ActorValueOwner* pOwner;
        float foldValue;
        float fadjustedValue = DifficultyLevelCheckModHealth(aiWhich, afValue, apAttacker);
        if (ActorValue::HasModifiedCallback(aiWhich)) {
            foldValue = GetActorFloatValue(aiWhich);
        }
        else {
            foldValue = 0.0f;
        }
        pPermanentModifiers.AdjustModifier(aiWhich, fadjustedValue, ModifierList::NO_CLAMP);
        if (aiWhich == ActorValue::HEALTH && fadjustedValue < 0.0) {
            HandleHealthDamage(apAttacker, fadjustedValue);
        }
        AddChange(CHANGE_ACTOR_PERMANENT_MODIFIERS);
        CheckSkillLevelChange(aiWhich, false);
        if (pCurrentProcess) {
            pCurrentProcess->SetCachedActorValueOutOfDate(aiWhich);
        }
        ActorValueOwner* pAttackerOwner = static_cast<ActorValueOwner*>(apAttacker);
        if (this) {
            pOwner = static_cast<ActorValueOwner*>(this);
        }
        else {
            pOwner = nullptr;
        }
        ActorValue::CheckCallModifiedCallback(pOwner, aiWhich, foldValue, fadjustedValue, pAttackerOwner);
    }
}

void Actor::DamageModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
    if ((aiWhich != ActorValue::FATIGUE || (aiValue >= 0 || HasRagDoll())) &&
        !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        ActorValueOwner* pOwner;
        float foldValue;
        int32_t iadjustedValue = DifficultyLevelCheckModHealthI(aiWhich, aiValue, apAttacker);
        if (ActorValue::HasModifiedCallback(aiWhich)) {
            foldValue = GetActorFloatValue(aiWhich);
        }
        else {
            foldValue = 0.0f;
        }
        if (pCurrentProcess) {
            pCurrentProcess->DamageModActorValue(this, aiWhich, iadjustedValue);
        }
        if (aiWhich == ActorValue::HEALTH && iadjustedValue < 0) {
            HandleHealthDamage(apAttacker, static_cast<float>(iadjustedValue));
        }
        AddChange(CHANGE_ACTOR_DAMAGE_MODIFIERS);
        CheckSkillLevelChange(aiWhich, false);
        ActorValueOwner* pAttackerOwner = static_cast<ActorValueOwner*>(apAttacker);
        if (this) {
            pOwner = static_cast<ActorValueOwner*>(this);
        }
        else {
            pOwner = nullptr;
        }
        ActorValue::CheckCallModifiedCallback(
            pOwner,
            aiWhich,
            foldValue,
            static_cast<float>(iadjustedValue),
            pAttackerOwner
        );
    }
}

void Actor::DamageModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
    if ((aiWhich != ActorValue::FATIGUE || (!(afValue < 0.0) || HasRagDoll())) &&
        !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        ActorValueOwner* pOwner;
        float foldValue;
        float fadjustedValue = DifficultyLevelCheckModHealth(aiWhich, afValue, apAttacker);
        if (ActorValue::HasModifiedCallback(aiWhich)) {
            foldValue = GetActorFloatValue(aiWhich);
        }
        else {
            foldValue = 0.0f;
        }
        if (aiWhich == ActorValue::FATIGUE && fadjustedValue < 0.0) {
            if (fMinFatigue.Float() >= static_cast<double>(foldValue)) {
                return;
            }
            float fnewValue = foldValue + fadjustedValue;
            if (fMinFatigue.Float() > static_cast<double>(fnewValue)) {
                fadjustedValue = fMinFatigue.Float() - foldValue;
            }
        }
        if (pCurrentProcess) {
            pCurrentProcess->DamageModActorValue(this, aiWhich, fadjustedValue);
        }
        if (aiWhich == ActorValue::HEALTH && fadjustedValue < 0.0) {
            HandleHealthDamage(apAttacker, fadjustedValue);
        }
        AddChange(CHANGE_ACTOR_DAMAGE_MODIFIERS);
        CheckSkillLevelChange(aiWhich, false);
        ActorValueOwner* pAttackerOwner = static_cast<ActorValueOwner*>(apAttacker);
        if (this) {
            pOwner = static_cast<ActorValueOwner*>(this);
        }
        else {
            pOwner = nullptr;
        }
        ActorValue::CheckCallModifiedCallback(pOwner, aiWhich, foldValue, fadjustedValue, pAttackerOwner);
    }
}

void Actor::ModActorBaseValue(ActorValue::Index aiWhich, int32_t aiValue) {
    ModActorBaseValue(aiWhich, static_cast<float>(static_cast<double>(aiValue)));
}

void Actor::ModActorBaseValue(ActorValue::Index aiWhich, float afValue) {
    if (!ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        ActorValueOwner* pOwner;
        float foldValue;
        if (ActorValue::HasModifiedCallback(aiWhich)) {
            foldValue = GetActorFloatValue(aiWhich);
        }
        else {
            foldValue = 0.0f;
        }
        float fnewBase = GetBaseActorFloatValue(aiWhich) + afValue;
        SetActorValue(aiWhich, fnewBase);
        CheckSkillLevelChange(aiWhich, false);
        if (this) {
            pOwner = static_cast<ActorValueOwner*>(this);
        }
        else {
            pOwner = nullptr;
        }
        ActorValue::CheckCallModifiedCallback(pOwner, aiWhich, foldValue, afValue, nullptr);
    }
}

ItemChange* Actor::GetBestWeapon(COMBAT_WEAPON_TYPE aeType) {
    float fvalue;
    InventoryChanges* pchanges = GetExtra()->GetContainerChanges();
    fvalue = 0.0;
    if (pchanges) {
        return pchanges->GetBestWeapon(GetActorBase(), fvalue, aeType, false);
    }
    return nullptr;
}

ItemChange* Actor::GetBestAmmo() {
    InventoryChanges* pchanges = TESObjectREFR::GetExtra()->GetContainerChanges();
    if (!pchanges) {
        return nullptr;
    }
    return pchanges->GetBestAmmo(GetActorBase(), false);
}

// Takes afDamage (after the Modify Damage To Items perks) off the condition of the equipped apEquipment, the actor's
// weapon or armour; a thrown explosive in an NPC's hand loses all of it. Nothing while the actor runs a use-item or
// use-weapon package, for the player's thrown explosives and for unplayable weapons. Crossing 25% condition warns the
// player; a weapon at zero breaks: the player's (or a weapon that cannot drop) is unequipped, or its VATS playback
// quit; an NPC's thrown explosive goes off in its hand, any other weapon is dropped from a drawn hand. True when the
// item broke, and for any thrown weapon, whose condition is never lowered. abIgnoreArmorSkill is ignored.
bool Actor::DamageEquipment(ItemChange* apEquipment, float afDamage, bool abIgnoreArmorSkill) {
    if (!apEquipment || afDamage <= 0.0) {
        return false;
    }
    TESPackage* ppack = GetCurrentAIProcess() ? GetCurrentAIProcess()->GetPackageThatIsRunning() : nullptr;
    if (ppack &&
        (ppack->GetPackType() == PACKAGE_TYPE_USE_ITEM_AT || ppack->GetPackType() == PACKAGE_TYPE_USE_WEAPON)) {
        return false;
    }
    TESObjectWEAP* pweapon = nullptr;
    if (apEquipment->pContainerObj && apEquipment->pContainerObj->GetFormType() == WEAP_ID) {
        pweapon = static_cast<TESObjectWEAP*>(apEquipment->pContainerObj);
    }
    if (pweapon &&
        IsPc() &&
        pweapon->IsThrownWeapon() &&
        pweapon->GetProjectile(this) &&
        pweapon->GetProjectile(this)->GetHasExplosion()) {
        return false;
    }
    if (pweapon && !pweapon->GetIsPlayable()) {
        return false;
    }
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_DAMAGE_TO_ITEMS, this, &afDamage);
    float fuseDamage = afDamage;
    if (pweapon &&
        !IsPc() &&
        pweapon->IsThrownWeapon() &&
        pweapon->GetProjectile(this) &&
        pweapon->GetProjectile(this)->GetHasExplosion()) {
        fuseDamage = apEquipment->GetItemHealth(false);
    }
    TESObjectARMO* parmor = nullptr;
    if (apEquipment->pContainerObj && apEquipment->pContainerObj->GetFormType() == ARMO_ID) {
        parmor = static_cast<TESObjectARMO*>(apEquipment->pContainerObj);
    }
    float fhealth = apEquipment->GetItemHealth(false) - fuseDamage;
    if (fhealth < 1.0) {
        fhealth = 0.0f;
    }
    if (CombatController::GetShowVerboseMessages() && parmor) {
        // The arguments are evaluated right to left: the item's health, then the armour's name, then
        // the actor's.
        double fitemHealth = apEquipment->GetItemHealth(false);
        const char* parmorName = static_cast<TESFullName*>(parmor)->GetFullName();
        const char* pactorName = TESObjectREFR::GetFullName();
        Interface::PrintLine(
            "%.20s's %s takes %.2f points of damage (%.2f/%.2f)!",
            pactorName,
            parmorName,
            static_cast<double>(fuseDamage),
            static_cast<double>(fhealth),
            fitemHealth
        );
    }
    // IsPc is called here and its result dropped.
    IsPc();
    float fprevHealthPercent = apEquipment->GetItemHealth(true);
    if (!pweapon || !pweapon->IsThrownWeapon()) {
        ExtraDataList* pextra = apEquipment->pExtraObjectList->m_item;
        apEquipment->SetItemHealth(fhealth, GetExtra()->GetContainerChanges(), pextra, true);
        if (fprevHealthPercent >= 25.0 && apEquipment->GetItemHealth(true) < 25.0) {
            if (pweapon && IsPc()) {
                Interface::ShowMessage(sWeaponLowCond.String(), 2, nullptr, "WPNBreak", 4.0f, false);
            }
            else if (parmor && IsPc()) {
                Interface::ShowMessage(sArmorLowCond.String(), 2, nullptr, "WPNBreak", 4.0f, false);
            }
        }
        if (parmor) {
            ResetArmorRating();
        }
        if (pweapon) {
            ResetCachedWeaponCondition();
        }
        UpdateWeaponConditionEffects(false);
        if (fhealth > 0.0) {
            return false;
        }
    }
    if (pweapon) {
        if (IsPc() ||
            pweapon->GetCantDrop() ||
            apEquipment->pContainerObj->GetQuestObject() ||
            pweapon->IsEmbeddedWeapon()) {
            if (this == pPlayer) {
                NiAVObject* pplayer3D = pPlayer->Get3D(true);
                BSAudio::QInstance().StopMovingSounds(pplayer3D, 0.0f, 0);
                Interface::ShowMessage(sWeaponBreak.String(), 2, nullptr, "WPNBreak", 4.0f, false);
            }
            NiAVObject* p3D = TESObjectREFR::Get3D();
            BSAudio::QInstance().StopMovingSounds(p3D, 0.0f, 0);
            if (IsPc() && pweapon && pVATS.GetMode() == VATS::VATS_PLAYBACK) {
                pVATS.QuitVATSPlayback_0(apEquipment->pContainerObj, apEquipment->pExtraObjectList->m_item);
            }
            else {
                UnEquipObject(apEquipment->pContainerObj, 1, apEquipment->pExtraObjectList->m_item, false, true, true);
            }
        }
        else {
            NiNode* pnode = parmor ? pCurrentProcess->GetShieldBone(GetBiped())
                                   : pCurrentProcess->GetWeaponBone(GetBiped());
            NiPoint3 point = pnode ? pnode->GetWorldTranslate() : GetLocationOnReference();
            NiMatrix3 rotMatrix = pnode ? pnode->GetWorldTransform().m_Rotate : NiMatrix3::IDENTITY;
            if (pweapon->IsThrownWeapon() &&
                pweapon->GetProjectile(this) &&
                pweapon->GetProjectile(this)->GetHasExplosion()) {
                RemoveItem(
                    apEquipment->pContainerObj,
                    apEquipment->pExtraObjectList->m_item,
                    1,
                    false,
                    false,
                    nullptr,
                    nullptr,
                    nullptr,
                    true,
                    false
                );
                Explosion::SpawnExplosion(
                    pweapon->GetProjectile(this)->GetExplosion(),
                    nullptr,
                    nullptr,
                    GetParentCell(),
                    point,
                    rotMatrix
                );
            }
            else if (IsWeaponDrawn()) {
                NiPoint3 kRotate;
                rotMatrix.ToEulerAnglesZXY(kRotate.z, kRotate.x, kRotate.y);
                TESObjectREFR* pdropref = DropObject(
                    apEquipment->pContainerObj,
                    apEquipment->pExtraObjectList->m_item,
                    1,
                    &point,
                    &kRotate
                );
                TESObjectREFR::SetDroppedItemData(this, pdropref);
                CombatController* pcombatController = GetCombatController();
                if (pcombatController) {
                    pcombatController->Disarmed(nullptr);
                }
                if (IsPlayerTeammate()) {
                    FollowerBarks::TriggerFollowerBark(this, FollowerBarks::BARK_WEAPON_BROKE);
                }
            }
            if (IsWeaponDrawn()) {
                pCurrentProcess->SetWantWeaponDrawn(false);
                Animation* panim = GetAnimation();
                if (panim) {
                    panim->BlendOut(AGS_WEAPON, false);
                    panim->BlendOut(AGS_LEFT_ARM, false);
                }
            }
        }
    }
    return true;
}

TESObjectREFR* Actor::DropObject(
    TESObject* apobject,
    ExtraDataList* apExtra,
    int32_t ainumber,
    const NiPoint3* apPoint,
    const NiPoint3* apRotate
) {
    bool bworn = apExtra ? apExtra->GetWorn(false) : false;
    if (this == pPlayer && (GetAnimAction() != ANIM_ACTION_NONE && bworn)) {
        const char* pcMessage = static_cast<const char*>(sAnimationCanNotUnequip);
        Interface::ShowMessage(
            pcMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
            nullptr,
            2.0f,
            false
        );
        return nullptr;
    }

    if (pCurrentProcess) {
        pCurrentProcess->RemoveQueuedItem(static_cast<TESBoundObject*>(apobject), apExtra);
    }
    Animation* panim = GetAnimation();
    if (bworn && apobject->GetFormType() == WEAP_ID && panim && panim->GetCurrentSequence(AGS_WEAPON)) {
        BSAnimGroupSequence* pseq = panim->GetCurrentSequence(AGS_WEAPON);
        float fanimTime = panim->GetTime();
        float fscaledTime = pseq->GetScaledTime(fanimTime);
        if (fscaledTime != -std::numeric_limits<float>::max()) {
            double dcurrentTime = panim->GetTime();
            double dendTime = pseq->m_fEndKeyTime + dcurrentTime;
            float fsoundEnd = pseq->GetOffset() + dendTime;
            pseq->GetAnimGroup()->PlaySounds(this, fscaledTime, fsoundEnd, pseq);
        }
    }

    TESObjectCELL* pparentcell = GetParentCell();
    if (Interface::PreloadWorldMap() && pparentcell) {
        TESObjectREFR* plootbag = pparentcell->GetLootBag();
        if (!plootbag) {
            plootbag = pparentcell->CreateLootBagAtRef(this);
        }
        if (plootbag) {
            RemoveItem(
                static_cast<TESBoundObject*>(apobject),
                apExtra,
                ainumber,
                false,
                false,
                plootbag,
                nullptr,
                nullptr,
                true,
                false
            );
            return nullptr;
        }
    }

    if (!apExtra || (!apExtra->GetOwner() || (apExtra->GetOwner() && IsDead(false)))) {
        if (apExtra && IsDead(false)) {
            apExtra->RemoveOwnership();
        }

        if (GetParentCell() && GetParentCell()->GetOwner()) {
            double dvalue = static_cast<double>(TESValueForm::GetFormValue(apobject));
            if (fValueofItemForNoOwnership.Float() >= dvalue || IsDead(false)) {
                apExtra->SetOwnership(pPlayer->GetObjectReference());
            }
        }
    }
    TESObjectREFR* pdroppedref = RemoveItem(
        static_cast<TESBoundObject*>(apobject),
        apExtra,
        ainumber,
        false,
        true,
        nullptr,
        apPoint,
        apRotate,
        true,
        false
    );
    if (pdroppedref && apobject->GetFormType() == WEAP_ID && this != pPlayer) {
        SetWantWeaponDrawn(false);
        if (static_cast<TESObjectWEAP*>(apobject)->TESObjectWEAP::GetCurrentAmmo(this)) {
            int32_t iclipRounds = static_cast<TESObjectWEAP*>(apobject)->TESObjectWEAP::GetFormClipRounds(false);
            int32_t iammoCount = RandomInt(0, iclipRounds);
            if (iammoCount > 0) {
                TESAmmo* pammo = static_cast<TESObjectWEAP*>(apobject)->TESObjectWEAP::GetCurrentAmmo(this);
                pdroppedref->m_Extra.SetAmmoExtra(pammo, iammoCount);
                pdroppedref->AddChange(CHANGE_OBJECT_EXTRA_AMMO);
            }
        }
    }

    ItemChange* pitem = nullptr;
    if (this != pPlayer && IsNpc() && !IsDead(false)) {
        int32_t iformType = apobject->GetFormType();
        if (iformType == ARMO_ID) {
            ResetArmorRating();
        }
        else if (iformType == WEAP_ID && !IsInCombat()) {
            pitem = GetBestWeapon(COMBAT_WEAPON_TYPE_COUNT);
        }
    }
    if (pitem) {
        ExtraDataList* pextra = nullptr;
        if (pitem->pExtraObjectList) {
            pextra = pitem->pExtraObjectList->m_item;
        }
        QueueEquipObject(pitem->pContainerObj, 1, pextra, true, false, true);
        delete pitem;
    }
    if (pdroppedref) {
        bhkNiCollisionObject::ResetSim(pdroppedref->Get3D(), true);
        if (!IsDead(false)) {
            PlayPickUpSound(pdroppedref->GetObjectReference(), false, false);
        }
    }
    return pdroppedref;
}

// Picks apobjREFR up: clears the actors targeting it, settles its ownership (or raises a theft alarm), adds it to the
// inventory (a grenade or mine in hand is sheathed and reloaded around it), reloads a weapon whose ammo this was,
// equips a better weapon out of combat, and removes the reference from the world.
void Actor::PickUpObject(TESObjectREFR* apobjREFR, int32_t aicount, bool abflag) {
    apobjREFR->RemoveChange(0);
    TESBoundObject* pObject = apobjREFR ? apobjREFR->GetObjectReference() : nullptr;
    uint32_t eFormType = pObject ? pObject->GetFormType() : 0;
    BSSimpleList<Actor*>* pTargeting = reinterpret_cast<BSSimpleList<Actor*>*>(
        ReferenceProcessLists.GetActorTargetinganObject(apobjREFR->GetFormID(), this)
    );
    BSSimpleList<Actor*>* pList = pTargeting;
    PlayPickUpSound(pObject, true, false);
    while (pTargeting && pTargeting->GetItem()) {
        Actor* pActor = pTargeting->GetItem();
        if (pActor && pActor->GetCurrentPackageTarget() == apobjREFR) {
            pActor->SetCurrentTarget(this);
        }
        else {
            pActor->SetCurrentTarget(nullptr);
        }
        pTargeting = pTargeting->GetNext();
    }
    if (pList) {
        pList->RemoveAll();
        if (pList) {
            delete pList;
        }
    }
    if (IsNpc()) {
        if (!apobjREFR->GetOwner() || apobjREFR->IsAnOwner(this, true) || GetCurrentAIProcess()->GetProcessLevel()) {
            apobjREFR->GetExtra()->RemoveOwnership();
        }
        else {
            StealAlarm(apobjREFR, pObject, aicount, 0, apobjREFR->GetOwner());
            if (IsInCombat() && this != pPlayer) {
                apobjREFR->GetExtra()->RemoveOwnership();
            }
            else {
                TESForm* pOwnerBase = apobjREFR->GetExtra()->GetLevCreaOriginalBase();
                if (!pOwnerBase) {
                    pOwnerBase = apobjREFR->GetObjectReference();
                }
                apobjREFR->GetExtra()->SetOwnership(pOwnerBase);
            }
        }
    }
    bool bResheath = false;
    TESObjectWEAP* pWeapon = nullptr;
    uint32_t eType = apobjREFR->GetObjectReference()->GetFormType();
    // 0x28: a weapon (TESObjectWEAP); 0x29: ammo (TESAmmo).
    if (eType == 0x28) {
        pWeapon = static_cast<TESObjectWEAP*>(apobjREFR->GetObjectReference());
        // 10, 11: the one-hand grenade and mine weapon types.
        if ((pWeapon->GetWeaponType() == 10 || pWeapon->GetWeaponType() == 11) &&
            (pCurrentProcess->GetCurrentAmmo() &&
             pWeapon == pCurrentProcess->GetCurrentAmmo()->GetItemChangeObject() &&
             IsWeaponDrawn())) {
            Animation* pAnimation = GetAnimation();
            BipedAnim* pBiped = GetBiped();
            pCurrentProcess->ForceWeaponDrawnSheathed(false, pBiped, pAnimation, this);
            bResheath = true;
        }
    }
    else if (eType == 0x29) {
        abflag = false;
        if (apobjREFR->GetExtra()->GetCount() < 1) {
            // A reference, because the cast to the subobject is done without a null test.
            TESAmmo& rAmmo = *static_cast<TESAmmo*>(apobjREFR->GetObjectReference());
            aicount = static_cast<BGSClipRoundsForm&>(rAmmo).GetFormClipRounds();
        }
    }
    bool bDelete = false;
    if (apobjREFR->GetMaster() || apobjREFR->GetFile(-1) || apobjREFR->GetRefPersists()) {
        AddReferenceItem(apobjREFR, aicount, abflag, false);
    }
    else if (apobjREFR->GetAction(OBJECT_ACTION_ACTIVATE) || apobjREFR->IsProjectile()) {
        AddReferenceItem(apobjREFR, aicount, false, false);
    }
    else {
        AddReferenceItem(apobjREFR, aicount, false, false);
        bDelete = true;
    }
    if (bResheath) {
        ReloadWeaponNV(pWeapon, 0, false, false);
        Animation* pAnimation = GetAnimation();
        BipedAnim* pBiped = GetBiped();
        pCurrentProcess->ForceWeaponDrawnSheathed(true, pBiped, pAnimation, this);
    }
    if ((!pCurrentProcess->GetCurrentAmmo() || !pCurrentProcess->GetCurrentAmmo()->GetCount()) &&
        pCurrentProcess->GetCurrentWeapon()) {
        TESObjectWEAP* pCurrentWeapon = static_cast<TESObjectWEAP*>(
            pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject()
        );
        if (pObject == pCurrentWeapon->GetCurrentAmmo(this)) {
            bool bRegenMod = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(6);
            TESObjectWEAP* pRegenWeapon = static_cast<TESObjectWEAP*>(
                pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject()
            );
            if (pRegenWeapon->GetAmmoRegenRate(bRegenMod) <= 0.0) {
                bool bModValue = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(2);
                bool bDrawn = IsWeaponDrawn();
                TESObjectWEAP* pReloadWeapon = static_cast<TESObjectWEAP*>(
                    pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject()
                );
                ReloadWeaponNV(pReloadWeapon, bDrawn, bModValue, false);
            }
        }
    }

    if (!IsInCombat()) {
        ItemChange* pBest = nullptr;
        if (eFormType == 0x28) {
            pBest = GetBestWeapon(COMBAT_WEAPON_TYPE_COUNT);
        }
        if (pBest) {
            TESObjectWEAP* pBestWeapon = static_cast<TESObjectWEAP*>(pBest->GetItemChangeObject());
            if (CanUseWeapon(pBestWeapon, true)) {
                QueueEquipObject(pBest->GetItemChangeObject(), 1, nullptr, true, false, true);
            }
            if (pBest) {
                delete pBest;
            }
        }
    }
    apobjREFR->SetDelete(true);
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        TaskQueueInterface::GetInstance()->QueuePickedUpObjectRemoval(apobjREFR, bDelete);
    }
    else {
        Actor::RemovePickedUpObjectFromWorld(apobjREFR, bDelete);
    }
}

bool Actor::AddSpell(SpellItem* apSpellIn) {
    bool bresult = GetActorBase()->AddSpell(apSpellIn);
    if (bresult) {
        GetActorBase()->AddChange(CHANGE_ACTOR_BASE_SPELLLIST);
    }
    if (MagicSystem::IsSpellTypePermanent(apSpellIn->GetSpellType())) {
        MagicItem* pmagicItem;
        if (apSpellIn) {
            pmagicItem = &static_cast<MagicItem&>(static_cast<MagicItemForm&>(*apSpellIn));
        }
        else {
            pmagicItem = nullptr;
        }
        if (!IsSpellTarget(pmagicItem, true)) {
            MagicTarget* ptarget;
            bresult = true;
            CastAbility(apSpellIn, false);
            if (this) {
                ptarget = static_cast<MagicTarget*>(this);
            }
            else {
                ptarget = nullptr;
            }
            TransferDisease(apSpellIn, ptarget, false);
        }
    }
    return bresult;
}

// Loads the clip from the inventory's ammo (or, for thrown weapons, the weapon itself) and plays the reload: the full
// reload for aiReloadAnim 2, the reload group for 1. The temporary inventory entry is deleted on every path.
bool Actor::ReloadWeapon(TESObjectWEAP* apWeapon, int32_t aiReloadAnim, bool bUseModValue) {
    if (!apWeapon) {
        return true;
    }
    TESForm* pAmmo = apWeapon->GetCurrentAmmo(this);
    if (!pAmmo &&
        (apWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE ||
         apWeapon->GetWeaponType() == WEAPON_ONE_HAND_MINE ||
         apWeapon->GetWeaponType() == WEAPON_ONE_HAND_THROWN)) {
        pAmmo = apWeapon;
    }
    if (!pAmmo) {
        return false;
    }
    Animation* pAnimation = GetAnimation();
    InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(this);
    ItemChange* pInventoryItem = pChanges->GetInventoryItem(static_cast<TESBoundObject*>(pAmmo), 0);
    int32_t iCount = pInventoryItem ? pInventoryItem->GetCount() : 0;
    ItemChange* pAmmoItem = pCurrentProcess->GetCurrentAmmo();
    ItemChange* pWeaponItem = pCurrentProcess->GetCurrentWeapon();
    if (!pAmmoItem && pInventoryItem) {
        pAmmoItem = new ItemChange(pInventoryItem->GetContainerObject(), iCount);
        pCurrentProcess->SetCurrentAmmo(pAmmoItem);
    }
    if (aiReloadAnim == 2 && pAmmoItem) {
        bool bClipMod = pWeaponItem ? pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE) : false;
        if (apWeapon->GetFormClipRounds(bClipMod) != pAmmoItem->GetCount() && iCount != pAmmoItem->GetCount()) {
            int32_t eReloadGroup = apWeapon->GetReloadAnim(bUseModValue);
            uint16_t usGroup = GetAnimGroup(static_cast<ANIM_GROUP_ENUM>(eReloadGroup), nullptr, false, nullptr);
            if (TESAnimGroup::GetType(usGroup) == eReloadGroup) {
                // Flags 1: start at once.
                pAnimation->PlayGroup(usGroup, ACTION_START, -1, AGS_NONE);
                SetAnimAction(ANIM_ACTION_RELOAD, pAnimation->GetCurrentSequence(AGS_WEAPON));
            }
        }
    }
    if (aiReloadAnim > 0) {
        CombatController* pController = GetCombatController();
        if (pController) {
            pController->ReloadingWeapon(apWeapon, aiReloadAnim == 2);
        }
    }
    if (iCount && pInventoryItem && pAmmoItem) {
        if ((!ShouldWeaponUseAmmo(apWeapon) && !apWeapon->IsThrownWeapon()) ||
            apWeapon->GetFormClipRounds(bUseModValue) < iCount) {
            iCount = apWeapon->GetFormClipRounds(bUseModValue);
        }
        pAmmoItem->SetCount(iCount);
        if (aiReloadAnim == 1) {
            int32_t eReloadGroup = apWeapon->GetReloadAnim(bUseModValue);
            // A looping reload starts with the group 23 before its loop group.
            if (apWeapon->GetIsLoopingReload()) {
                eReloadGroup -= 23;
            }
            uint16_t usGroup = GetAnimGroup(static_cast<ANIM_GROUP_ENUM>(eReloadGroup), nullptr, false, nullptr);
            if (TESAnimGroup::GetType(usGroup) == eReloadGroup) {
                if (apWeapon->IsAutomaticWeapon()) {
                    pAnimation->BlendOut(AGS_WEAPON_UP, false);
                    pAnimation->BlendOut(AGS_WEAPON_DOWN, false);
                    pAnimation->PlayGroup(usGroup, ACTION_START, -1, AGS_NONE);
                    SetAnimAction(ANIM_ACTION_RELOAD, pAnimation->GetCurrentSequence(AGS_WEAPON));
                }
                else {
                    pAnimation->QueueReloadAnim(usGroup);
                }
            }
        }
        delete pInventoryItem;
        return true;
    }
    if (pInventoryItem) {
        delete pInventoryItem;
    }
    return false;
}

// Both branches make the same call; only the loop test is made first.
bool Actor::ReloadWeaponNV(TESObjectWEAP* apWeapon, int32_t aiReloadAnim, bool bUseModValue, bool bFromInventory) {
    if (apWeapon->GetIsLoopingReload()) {
        return ReloadWeapon(apWeapon, aiReloadAnim, bUseModValue);
    }
    return ReloadWeapon(apWeapon, aiReloadAnim, bUseModValue);
}

// Uses the ammo for aiShotCount shots (-1: the process's shots to fire) and returns the ammo left. When it runs
// out, the weapon reloads if it can; otherwise an ammo-as-weapon item is put away, and the ammo is unequipped.
uint32_t Actor::UseAmmo(int32_t aiShotCount) {
    if (!pCurrentProcess || !pCurrentProcess->GetCurrentWeapon() || !pCurrentProcess->GetCurrentAmmo()) {
        return 0;
    }
    uint32_t uiCount = pCurrentProcess->GetCurrentAmmo()->GetCount();
    TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetContainerObject());
    TESBoundObject* pAmmo = pCurrentProcess->GetCurrentAmmo()->GetContainerObject();
    int32_t iShots = aiShotCount == -1 ? pCurrentProcess->GetShotsToFire() : aiShotCount;
    if (pWeapon) {
        iShots = pWeapon->GetAmmoPerShot() * iShots;
    }
    // The compare is unsigned.
    if (static_cast<uint32_t>(iShots) > uiCount) {
        iShots = uiCount;
    }
    uiCount -= iShots;
    pCurrentProcess->GetCurrentAmmo()->SetCount(uiCount);
    if (ShouldWeaponUseAmmo(pWeapon)) {
        RemoveItem(
            pCurrentProcess->GetCurrentAmmo()->GetContainerObject(),
            nullptr,
            iShots,
            false,
            false,
            nullptr,
            nullptr,
            nullptr,
            true,
            false
        );
    }
    if (!uiCount) {
        MuzzleFlash* pMuzzleFlash = pCurrentProcess->GetCurrentMuzzleFlash();
        if (pMuzzleFlash) {
            pMuzzleFlash->SetDuration(0.0f);
        }
        bool bReloaded = false;
        if (pCurrentProcess->GetCurrentWeapon()) {
            bool bRegenMod = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_AMMO_REGEN_SECONDS);
            TESObjectWEAP* pCurrent = static_cast<TESObjectWEAP*>(
                pCurrentProcess->GetCurrentWeapon()->GetContainerObject()
            );
            if (pCurrent->GetAmmoRegenRate(bRegenMod) <= 0.0) {
                // The arguments are evaluated right to left.
                bool bUseModValue = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(
                    WEAPON_MOD_INCREASE_CLIP_SIZE
                );
                bool bDrawn = IsWeaponDrawn();
                pCurrent = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetContainerObject());
                bReloaded = ReloadWeaponNV(pCurrent, bDrawn, bUseModValue, false);
            }
        }
        if (bReloaded) {
            uiCount = pCurrentProcess->GetCurrentAmmo()->GetCount();
        }
        else if (pCurrentProcess->GetCurrentWeapon() && pCurrentProcess->GetCurrentAmmo()) {
            TESBoundObject* pWeaponObject = pCurrentProcess->GetCurrentWeapon()->GetContainerObject();
            if (pWeaponObject == pCurrentProcess->GetCurrentAmmo()->GetContainerObject()) {
                if (this == pPlayer) {
                    pPlayer->GetBiped(true)->RemoveBipedWeapon();
                    pPlayer->GetBiped(false)->RemoveBipedWeapon();
                }
                else if (GetBiped()) {
                    GetBiped()->RemoveBipedWeapon();
                }
                pCurrentProcess->SetCurrentWeapon(nullptr, nullptr, false);
                SetWantWeaponDrawn(false);
            }
        }
    }
    if (!uiCount) {
        QueueUnEquipObject(pAmmo, 1, nullptr, false, false, true);
        CombatController* pCombatController = GetCombatController();
        if (pCombatController) {
            pCombatController->Disarmed(nullptr);
        }
        if (IsPlayerTeammate()) {
            FollowerBarks::TriggerFollowerBark(this, FollowerBarks::BARK_OUT_OF_AMMO);
        }
    }
    return uiCount;
}

// The combat controller's group; none without a controller.
CombatGroup* Actor::GetCombatGroup() {
    CombatController* pController = GetCombatController();
    if (pController) {
        return pController->GetCombatGroup();
    }
    return nullptr;
}

void Actor::SetCombatGroup(CombatGroup* apGroup) {
    CombatController* pController = GetCombatController();
    if (pController) {
        pController->pCombatGroup = apGroup;
    }
}

void Actor::UpdateNonRenderSafeDialogueUpdate(float afDelta) {
    BaseProcess* pproc = pCurrentProcess;
    TESPackage* pbasepack = pproc->GetCurrentPackage();
    if (pbasepack) {}
    Animation* panim = GetAnimation();
    if (pproc && !pproc->eLevel) {
        NiBSBoneLODController* pboneLOD = static_cast<HighProcess*>(pproc)->HighProcess::GetBoneLOD();
        if (pboneLOD && pboneLOD->m_iLOD) {
            pboneLOD->SetBoneLOD(0);
        }
    }

    UpdateAnimation(afDelta);
    UpdateAnimationMovementNoWorldUpdate();
    if (pRagdollController) {
        UpdateRagdollAnimation(false);
    }
    if (panim) {
        panim->UpdateQueuedScenegraph();
    }
    Update(afDelta);
    TESObjectCELL* pcell = GetParentCell();
    if (pcell) {
        pcell->UpdateLights();
    }
    UpdateRefLight();
}

BSSoundHandle Actor::VoiceSoundFunctionEx(
    TESTopic* apTopic,
    Actor* apTarget,
    bool abIs2D,
    bool abNeverSubtitle,
    int32_t,
    bool abSayTo
) {
    // The virtuals go through the base pointer, so they are dispatched through the vtable.
    TESObjectREFR* pRef = this;
    BSSoundHandle handle;
    DialogueItem* pItem = apTopic->CreateDialogueItem(this, apTarget, nullptr, nullptr, false);
    MobileObject::StopCurrentDialogue();
    SetSayToTopic(apTopic);
    if (pItem) {
        TESTopicInfo* pInfo = pItem->pInfo;
        SetSayToTopicInfo(pInfo);
        if (pInfo) {
            pInfo->AddTopicList();
            pInfo->RunResult(TIRS_BEGIN, this);
        }
        pItem->FirstResponse();
        DialogueResponse* pResponse = pItem->GetCurrentResponse();
        if (pResponse) {
            bool bPlayVoiceFile = true;
            if (pResponse->pVoiceSound && bSoundFileDone) {
                BSSoundHandle soundHandle = MobileObject::PlaySound(
                    pResponse->pVoiceSound->iFormID,
                    false,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                        BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
                    true
                );
                // The context is the speaker's form ID.
                BSAudioManager::QInstance().SetCompletionCallback(
                    soundHandle.iSoundID,
                    &MobileObject::SoundFileCallBack,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID))
                );
                bSoundFileDone = false;
                bPlayVoiceFile = false;
            }
            if (bPlayVoiceFile && bSoundFileDone) {
                if (pResponse->cVoice.pString) {
                    char cVoiceFile[512];
                    BSstrcpy(cVoiceFile, 512, pResponse->cVoice.pString);
                    if (abIs2D) {
                        handle = BSAudio::QInstance().GetSoundHandleByFilename(
                            cVoiceFile,
                            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_VOICE |
                                BSGameSound::SOUND_FLAG_ONE_SHOT,
                            nullptr
                        );
                    }
                    else {
                        handle = BSAudio::QInstance().GetSoundHandleByFilename(
                            cVoiceFile,
                            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_VOICE |
                                BSGameSound::SOUND_FLAG_ONE_SHOT,
                            nullptr
                        );
                    }
                    if (abIs2D) {
                        handle.SetStaticAttenuation(static_cast<int32_t>(fDBVoiceAttenuationIn2D.Float() * 100.0));
                    }
                    else {
                        NiPoint3 kPosition = pRef->GetLocationOnReference();
                        handle.SetPosition(kPosition.x, kPosition.y, kPosition.z);
                        handle.SetObjectToFollow(pRef->Get3D());
                    }
                    handle.Play(false);
                    if (abSayTo) {
                        handle.SetCompletionCallback(
                            &MobileObject::SayToCallBack,
                            reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID))
                        );
                    }
                    bVoiceFileDone = false;
                    // The by-value handle argument is deliberately built with operator=.
                    BSSoundHandle kArgument;
                    kArgument = handle;
                    pCurrentProcess->SetSoundHandle(ACTOR_SOUND_HANDLE_VOICE, kArgument);
                    handle.GetDuration();
                }
                else if (pInfo && abSayTo) {
                    MobileObject::SayToCallBack(reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)), true);
                }
            }
            if ((gs_bGeneralSubtitles_Gameplay.Bool() || static_cast<uint8_t>(GetShowSubtitles())) &&
                !abNeverSubtitle) {
                // Reads the position before it builds the by-value handle argument with operator=.
                NiPoint3 kSpeakerLoc = pRef->GetLocationOnReference();
                BSSoundHandle kArgument;
                kArgument = handle;
                Interface::ShowSubtitle(pResponse->cText.pString, kArgument, kSpeakerLoc, apTarget, true);
            }
        }
        delete pItem;
    }
    // A copy into the caller's result: returning handle itself would let C++ build it there.
    return BSSoundHandle(handle);
}

void Actor::InitiateFlee(
    TESObjectREFR* apFleeRef,
    bool abrunonce,
    bool abknows,
    bool abCombatMode,
    TESObjectCELL* apcell,
    TESObjectREFR* apRef,
    float affleeFromDist,
    float affleeToDist
) {
    if (!IsRestrained() && !IsUnConscious() && !IsDead(false) && !bDeadFlag) {
        PackageLocation* ploc;
        if (GetSitSleepState()) {
            abrunonce = false;
        }
        TESPackage* pbasepack = GetCurrentPackage();
        FleePackage* ppack = nullptr;
        if (pbasepack && pbasepack->GetPackType() == PACKAGE_TYPE_FLEE) {
            ppack = static_cast<FleePackage*>(pbasepack);
        }

        if (!ppack) {
            ppack = new FleePackage(apFleeRef, nullptr, nullptr);
        }
        ppack->AddAvoidedRef(apFleeRef);
        PackageTarget* ppacktarg = ppack->GetPackageTarget();
        if (!ppacktarg) {
            ppacktarg = new PackageTarget;
            ppack->SetPackageTarget(ppacktarg);
        }
        ppacktarg->SetTargReference(apFleeRef);
        if (affleeFromDist >= 0.0) {
            ppacktarg->iValue = static_cast<int32_t>(affleeFromDist);
        }
        pCurrentProcess->ComputeLastTimeProcessed();
        ploc = new PackageLocation;
        if (affleeToDist >= 0.0) {
            ploc->SetRadius(static_cast<int64_t>(affleeToDist));
        }
        if (apRef) {
            ploc->SetLocReference(apRef);
            ppack->SetEvaluatePoint(false);
            ppack->SetPackageLocation(ploc);
            ppack->pRefFleeTo = apRef;
        }
        else if (apcell) {
            ploc->SetLocCell(apcell);
            ppack->SetEvaluatePoint(false);
            ppack->SetPackageLocation(ploc);
        }
        else {
            if (GetParentCell()) {
                GetParentCell()->GetInterior();
            }
            if (!ppack->GetRefFleeTo()) {
                if (pCurrentProcess->GetEssentialDownTimer() <= 0.0) {
                    pCurrentProcess->SetEssentialDownTimer(20.0f);
                }
                ppack->FindTeleportDoor(this, apFleeRef);
            }
        }
        if (ploc) {
            delete ploc;
        }
        if (!abrunonce) {
            pCurrentProcess->SavePackageToExtraData(this);
        }
        ppack->SetKnowsTarget(abknows);
        ppack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_FLEE;
        ppack->SetFleeSucceeded(false);
        float ftimerDelta = ppack->fFleeTimer * -1.0;
        ppack->ModFleeTimer(ftimerDelta);
        ppack->bCombatMode = abCombatMode;
        if (abCombatMode) {
            ppack->SetContinueDuringCombat(false);
            ppack->SetPretendInCombat(false);
        }
        PutCreatedPackage(ppack, abrunonce, true);
    }
}

void Actor::InitiateSearchForAttacker(TESObjectREFR* apAttacker, NiPoint3 apLocation, bool abAttacked) {
    PackageTarget* ptarget;
    PackageLocation* ploc;
    SearchPackage* ppack;
    ppack = new SearchPackage(apLocation, apAttacker, abAttacked);
    ppack->SetPackType(PACKAGE_TYPE_SEARCH_FOR_ATTACKER);
    ppack->SetMustReachLocation(false);
    ppack->SetMustComplete(false);
    ploc = new PackageLocation;
    ploc->SetLocReference(this);
    ppack->SetPackageLocation(ploc);
    if (ploc) {
        delete ploc;
    }
    ppack->SetAlwaysRun(true);
    ptarget = new PackageTarget;
    ppack->SetPackageTarget(ptarget);
    ppack->SetProcedureType(PACKAGE_PROCEDURE_SEARCH);
    ppack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
    ppack->GetPackageTarget()->SetTargReference(apAttacker);
    double dsearchDistance = fSearchPackageDistanceToTarget.Float();
    ppack->GetPackageTarget()->iValue = static_cast<int32_t>(dsearchDistance);
    pCurrentProcess->ResetSearchChatterTimer();
    pCurrentProcess->ComputeLastTimeProcessed();
    pCurrentProcess->ResetSearchTimer();
    PutCreatedPackage(ppack, true, true);
    if (apLocation != NiPoint3::ZERO) {
        ppack->SetCenterLocaiton(apLocation);
    }
    if (abAttacked) {
        pCurrentProcess->AddToProcedureIndexRunning(this, 1);
    }
}

void Actor::InitiateGetUpPackage() {
    TESPackage* prunning = pCurrentProcess->GetPackageThatIsRunning();
    if (!IsRestrained() && !IsUnConscious() && (!prunning || prunning->GetPackType() != PACKAGE_TYPE_GET_UP)) {
        if (pCurrentProcess->GetCurrentFurnitureIndex() == 127) {
            pCurrentProcess->SetSitSleepState(this, SIT_SLEEP_NORMAL, nullptr, 127);
        }
        else {
            PackageLocation* ploc;
            TESPackage* ppack = TESPackage::CreatePackage(PACKAGE_TYPE_GET_UP);
            ppack->SetPackType(PACKAGE_TYPE_GET_UP);
            ppack->SetMustReachLocation(false);
            ppack->SetMustComplete(true);
            ploc = new PackageLocation;
            ploc->SetLocReference(this);
            ppack->SetPackageLocation(ploc);
            if (ploc) {
                delete ploc;
            }
            ppack->SetProcedureType(PACKAGE_PROCEDURE_GET_UP);
            pCurrentProcess->ComputeLastTimeProcessed();
            PutCreatedPackage(ppack, true, true);
        }
    }
}

void Actor::StartCombat(
    Actor* apTarget,
    CombatGroup* apGroup,
    bool abForceCombat,
    bool abIsAggressor,
    bool abfleeing,
    int32_t iPriority,
    bool bCastSpell,
    TESPackage* apPackage
) {
    if (!IsRestrained() && (!IsUnConscious() && (apTarget != this && (!IsDead(false) && !bDeadFlag)))) {
        float fdistance;
        float ffudge;
        if (!abForceCombat) {
            SIT_SLEEP_STATE esitSleepState = GetSitSleepState();
            if (esitSleepState && esitSleepState != SIT_SLEEP_IS_SITTING && esitSleepState != SIT_SLEEP_IS_SLEEPING) {
                if (!pCurrentProcess->GetFurnitureMarker()) {
                    return;
                }
                if (pCurrentProcess->GetFurnitureMarker()->cNumber <= 20u) {
                    return;
                }
                InitiateGetUpPackage();
            }
        }
        if (GetActorBaseSpeedMult() != 0.0 ||
            (fdistance = GetDistanceFromReference(apTarget, false, false),
             (ffudge = GetReach() * 0.25, !(fdistance > GetReach() + ffudge)))) {
            bool bhighPriority = iPriority > 0;
            int32_t icombatLevel = 0;
            if (iPriority) {
                icombatLevel = iPriority;
            }
            else {
                Actor* pthreatenedActor = this;
                if (pCurrentProcess) {
                    if (pCurrentProcess->GetCommandingActor()) {
                        pthreatenedActor = pCurrentProcess->GetCommandingActor();
                    }
                }
                if (apTarget && apTarget->pCurrentProcess) {
                    icombatLevel = static_cast<int32_t>(
                        apTarget->pCurrentProcess->GetPotentialThreatDamage(apTarget, pthreatenedActor)
                    );
                }
            }
            if (GetCombatController()) {
                if (GetCombatController()) {
                    if (!apTarget || QCombatController()->GetTarget() == apTarget) {
                        if (apGroup) {
                            pCombatManager->AddGroupMember(this, apGroup);
                        }
                    }
                    else {
                        QCombatController()->AddTarget(apTarget, icombatLevel, bhighPriority, 0.0f, 0.0f);
                    }
                }
            }
            else {
                pCurrentProcess->SetDetectionTimer(0.0f);
                if (apTarget == pPlayer) {
                    pPlayer->ChangePerceivedActorHostileStatus(this, true);
                }
                if (Interface::GetActivateREFR() == this) {
                    HUDMainMenu::HandleHealthbarForCrosshairActor_(nullptr);
                }
                if (pCurrentProcess->GetCurrentPackage()) {
                    if (pCurrentProcess->GetCurrentPackage()->GetForceUnequipWeapon()) {
                        ItemChange* pchange = GetBestWeapon(COMBAT_WEAPON_TYPE_COUNT);
                        if (pchange) {
                            if (pchange->pContainerObj && pchange->pContainerObj->GetFormType() == WEAP_ID) {}

                            QueueEquipObject(pchange->pContainerObj, 1, nullptr, true, false, true);
                            delete pchange;
                        }
                    }
                }
                EndInterruptPackage(false);
                pCurrentProcess->ClearRunOncePackage();
                if (GetSitSleepState() == SIT_SLEEP_IS_SITTING || GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    InitiateGetUpPackage();
                }
                CombatController* ppack = nullptr;
                if (apGroup) {
                    ppack = pCombatManager->AddGroupMember(this, apGroup);
                }
                else if (apTarget) {
                    ppack = pCombatManager
                                ->AddCombatant(this, apTarget, static_cast<uint32_t>(icombatLevel), bhighPriority);
                }
                else if (apPackage) {
                    ppack = pCombatManager->CreateCombatController(this, apPackage);
                }
                if (abfleeing && ppack) {
                    ppack->ForceFlee(apTarget, 0);
                }

                if (abForceCombat && ppack) {
                    ppack->ForceCombat();
                }

                pCurrentProcess->ComputeLastTimeProcessed();
                if (ppack) {
                    pCurrentProcess->SavePackageToExtraData(this);
                    PutCreatedPackage(ppack, false, true);
                    bInCombat = true;
                }
                else {
                    bInCombat = false;
                }
            }
        }
    }
}

// The process's current package when it is a combat package.
CombatController* Actor::GetCombatController() {
    if (!pCurrentProcess || !IsInCombat()) {
        return nullptr;
    }
    TESPackage* pPackage = pCurrentProcess->GetCurrentPackage();
    if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_COMBAT) {
        return static_cast<CombatController*>(pPackage);
    }
    return nullptr;
}

Actor* Actor::GetCombatTarget() {
    CombatController* pController = GetCombatController();
    if (pController) {
        return pController->GetTarget();
    }

    return nullptr;
}

void Actor::UpdateCombat() {
    if (!(pFaderManager->GetFaderAlpha(FADER_TO_BLACK) > 0.0) && !IsStillLoading() && IsInCombat()) {
        CombatController* pcombatController = QCombatController();
        if (pcombatController) {
            if (pcombatController->ShouldStopCombat()) {
                StopCombat(nullptr);
                return;
            }
            Actor* ptarget = pcombatController->GetTarget();
            if (ptarget && !pcombatController->IsActoraCombatTarget(ptarget)) {
                StopCombat(ptarget);
                pcombatController = GetCombatController();
                if (!pcombatController) {
                    return;
                }
                ptarget = pcombatController->GetTarget();
            }
            if (!IsRestrained() &&
                (ptarget != pPlayer || !pPlayer->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT))) {
                if (ptarget &&
                    ((ptarget->IsDead(false) && !ptarget->IsDying()) ||
                     (ptarget->GetDisabled() || (ptarget->GetDelete() || ptarget->IsUnConscious())))) {
                    StopCombat(ptarget);
                }
                else {
                    if (ptarget == pPlayer && pPlayer->GetDetectionLevel(this) > 0) {
                        pPlayer->AddActorToPlayerCombatList(this);
                    }
                    if (static_cast<uint8_t>(pcombatController->IsNotDetectingTarget())) {
                        pCurrentProcess->ClearCombatHeadTrackTarget();
                    }
                    else {
                        pCurrentProcess->SetCombatHeadTrackTarget(ptarget);
                    }
                    CombatAI_Total.Start();
                    if (GetCurrentProcessType()) {
                        pcombatController->UpdateLow();
                    }
                    else {
                        pcombatController->Update();
                    }
                    CombatAI_Total.Stop();
                }
            }
        }
    }
}

void Actor::StopCombat(Actor* apAttacker) {
    CombatController* pController;
    const char* pcName;
    ExtraDataList* pextra;
    BaseProcess* pPlayerProcess;
    BaseProcess* pProcess;
    uint8_t bstopped;
    if (IsActoraGuard() && (pPlayer->IsPlayerResistingArrest() && IsInCombat())) {
        pProcess = MobileObject::GetCurrentAIProcess();
        if (static_cast<const void*>(pProcess->GetTarget()) == static_cast<const void*>(pPlayer)) {
            pPlayerProcess = pPlayer->MobileObject::GetCurrentAIProcess();
            pPlayerProcess->ModNumberGuardsArresting(-1);
        }
    }
    if (!apAttacker || static_cast<const void*>(apAttacker) == static_cast<const void*>(pPlayer)) {
        pPlayer->RemoveActorFromPlayercombatList(this);
    }
    if (static_cast<const void*>(apAttacker) == static_cast<const void*>(pPlayer)) {
        pPlayer->ChangePerceivedActorHostileStatus(this, false);
    }
    SetBlock(false);
    SetBeenAttacked(false);
    if (GetCombatController()) {
        pController = QCombatController();
        bstopped = pController->StopCombat(apAttacker);
        if (!IsDead(false) && bstopped) {
            SetWalkRunFlagDirty();
            if (CombatController::GetShowVerboseMessages()) {
                pcName = TESObjectREFR::GetFullName();
                Interface::PrintLine("%.20s stops combat.", pcName);
            }
            EndInterruptPackage(true);
            ClearInCombat(true);
            pextra = TESObjectREFR::GetExtra();
            Script::SetActionFlag(this, pextra, SCRIPT_ACTION_ONCOMBATEND);
            eQueuedattack = ANIM_GROUP_NONE;
            pCurrentProcess->RemovePostAnimationAction(BaseProcess::POSTANIM_ACTION_FIRE_WEAPON);
        }
    }
    else {
        ClearInCombat(true);
    }
    if (MobileObject::GetCurrentAIProcess()) {
        pProcess = MobileObject::GetCurrentAIProcess();
        pProcess->SetIsAggressor(false);
        pProcess = MobileObject::GetCurrentAIProcess();
        pProcess->ClearCombatHeadTrackTarget();
    }
}

// The combat controller runs the round when the actor has one; otherwise the process plays the round for the level
// the actor is processed at (1 and 2, else 4, PROCESS_TYPE_COUNT).
void Actor::PerformOneRoundofCombat() {
    if (GetCombatController()) {
        InitializationFlow(this);
        return;
    }
    PROCESS_TYPE eLevel = PROCESS_TYPE_COUNT;
    if (pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH) {
        eLevel = PROCESS_TYPE_MIDDLE_HIGH;
    }
    else if (pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_MIDDLE_LOW) {
        eLevel = PROCESS_TYPE_MIDDLE_LOW;
    }
    pCurrentProcess->PerformOneRoundofCombat(this, eLevel);
}

float Actor::CalculateWalkSpeed() {
    ActorValueOwner* avOwner;
    TESObjectARMO* pArmor;
    TESObjectWEAP* pWeap;
    bool bsneak = IsSneaking();
    bool bnoWeapon = true;
    ItemChange* pweapon = nullptr;
    if (pCurrentProcess) {
        bnoWeapon = !pCurrentProcess->GetWeaponDrawn();
        pweapon = pCurrentProcess->GetCurrentWeapon();
    }
    ItemChange* parmor = GetArmorBeingWorn(BO_UPPER_BODY);
    if (pweapon) {
        pWeap = static_cast<TESObjectWEAP*>(pweapon->pContainerObj);
    }
    else {
        pWeap = nullptr;
    }
    if (parmor) {
        pArmor = static_cast<TESObjectARMO*>(parmor->pContainerObj);
    }
    else {
        pArmor = nullptr;
    }
    if (parmor) {
        delete parmor;
    }
    if (this) {
        avOwner = static_cast<ActorValueOwner*>(this);
    }
    else {
        avOwner = nullptr;
    }
    bool bencumbered = IsOverEncumbered();
    bool bnpc = IsNpc();
    float fspeed = GamePlayFormulas::GetActorWalkSpeed(avOwner, pWeap, pArmor, bsneak, bnoWeapon, bnpc, bencumbered);
    CombatController* cmbtMgr = GetCombatController();
    if (cmbtMgr) {
        return cmbtMgr->fWalkSpeedMult * fspeed;
    }
    return fspeed;
}

float Actor::CalculateRunSpeed() {
    ActorValueOwner* pOwner;
    TESObjectARMO* pArmor;
    TESObjectWEAP* pWeap;
    float fspeed;
    bool bnoWeapon = true;
    ItemChange* pweapon = nullptr;
    if (pCurrentProcess) {
        bnoWeapon = !pCurrentProcess->GetWeaponDrawn();
        pweapon = pCurrentProcess->GetCurrentWeapon();
    }
    ItemChange* parmor = GetArmorBeingWorn(BO_UPPER_BODY);
    if (pweapon) {
        pWeap = static_cast<TESObjectWEAP*>(pweapon->pContainerObj);
    }
    else {
        pWeap = nullptr;
    }
    if (parmor) {
        pArmor = static_cast<TESObjectARMO*>(parmor->pContainerObj);
    }
    else {
        pArmor = nullptr;
    }
    if (parmor) {
        delete parmor;
    }
    if (this) {
        pOwner = static_cast<ActorValueOwner*>(this);
    }
    else {
        pOwner = nullptr;
    }
    bool bencumbered = IsOverEncumbered();
    bool bsneak = IsSneaking();
    bool bnpc = IsNpc();
    fspeed = GamePlayFormulas::GetActorRunSpeed(pOwner, pWeap, pArmor, bnoWeapon, bnpc, bsneak, bencumbered);
    CombatController* pcombatController = GetCombatController();
    if (pcombatController) {
        fspeed = pcombatController->GetRunSpeedMult() * fspeed;
    }
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::RUN_SPEED_MODIFIER, this, &fspeed);
    return fspeed;
}

void Actor::ModifyDispositionTowardActor(Actor* apActor, float afamount) {
    if (apActor == pPlayer) {
        DispositionModifier* pmod;
        BSSimpleList<DispositionModifier*>* plist = &DispModifierList;
        AddChange(CHANGE_ACTOR_DISPOSITION_MODIFIERS);
        while (true) {
            if (!plist || !plist->m_item) {
                pmod = static_cast<DispositionModifier*>(ourMalloc(sizeof(DispositionModifier)));
                pmod->modifieramount = static_cast<int32_t>(afamount);
                pmod->pactormodified = apActor;
                apActor->SetTargeted(true);
                DispModifierList.AddHead(pmod);
                return;
            }
            pmod = plist->m_item;
            if (pmod->pactormodified == apActor) {
                break;
            }
            plist = plist->m_pkNext;
        }
        int32_t idisposition = GetDisposition(apActor, nullptr);
        if (static_cast<double>(idisposition) + afamount < 0.0) {
            afamount = afamount - (static_cast<double>(idisposition) + afamount);
        }
        else if (static_cast<double>(idisposition) + afamount > 100.0) {
            int32_t iremainder = idisposition + static_cast<int32_t>(afamount) - 100;
            if (afamount > static_cast<double>(iremainder)) {
                afamount = afamount - static_cast<double>(iremainder);
            }
            else {
                afamount = 0.0f;
            }
        }
        pmod->modifieramount += static_cast<int32_t>(afamount);
    }
}

float Actor::GetDispositionModifierTowardActor(Actor* apActor) {
    for (DispositionModifier* pmod : DispModifierList.ItemsUntilNull()) {
        if (pmod->pactormodified == apActor) {
            return static_cast<float>(pmod->modifieramount);
        }
    }
    return 0.0f;
}

void Actor::RemoveActorInDispositionModifier(Actor* apActor) {
    for (DispositionModifier* pmod : DispModifierList.ItemsUntilNull()) {
        if (pmod->pactormodified == apActor) {
            DispModifierList.Remove(pmod);
            return;
        }
    }
}

void Actor::SetStartingPosition() {
    if (this != pPlayer) {
        SetEditorLocation();
    }
}

bool Actor::HasBeenAttacked() {
    return MobileObject::GetCurrentAIProcess()->HasBeenAttacked();
}

void Actor::SetBeenAttacked(bool abFlag) {
    if (MobileObject::GetCurrentAIProcess()) {
        MobileObject::GetCurrentAIProcess()->SetBeenAttacked(abFlag);
    }
}

// Pushes the ragdoll for a hit from apAttacker: apAttacker's 3D turns arStartPos and arStopPos into world space, the
// hit force (afDamage, capped at 100) pushes the closest body along the hit and the block force (afBlocked) the left
// forearm; the Combat setting iShowHitVector shows the hit vector. When apAttacker has no 3D, kStart and kStop are left
// unwritten and read in the subtraction (an uninitialised read).
void Actor::HitMe(
    Actor* apAttacker,
    float afDamage,
    float afBlocked,
    const NiPoint3& arStartPos,
    const NiPoint3& arStopPos
) {
    NiNode* p3D = TESObjectREFR::Get3D();
    if (GetGhost()) {
        return;
    }
    NiAVObject* pHitNode = BSUtilities::GetObjectByName(p3D, static_cast<const char*>(sHitStart));
    NiPoint3 kStart;
    NiPoint3 kStop;
    NiNode* pAttacker3D = apAttacker->Get3D();
    if (pAttacker3D) {
        const NiTransform* pWorld;
        pWorld = &pAttacker3D->GetWorldTransform();
        kStart = *pWorld * arStartPos;
        pWorld = &pAttacker3D->GetWorldTransform();
        kStop = *pWorld * arStopPos;
    }
    NiPoint3 kDirection;
    kDirection = kStop - kStart;
    kDirection.Unitize();
    NiAVObject* pSpot = nullptr;
    if (!pHitNode) {
        return;
    }

    if (afDamage > 100.0) {
        afDamage = 100.0f;
    }
    // Keeps each first setting as a double and rounds the range and the force to float.
    double dMax = static_cast<float>(fHitForceMax);
    float fRange = static_cast<float>(dMax - static_cast<float>(fHitForceMin));
    double dScaled = static_cast<double>(fRange) * static_cast<double>(afDamage) * 0.009999999776482582;
    float fHitForce = static_cast<float>(static_cast<float>(fHitForceMin) + dScaled);
    dMax = static_cast<float>(fBlockForceMax);
    fRange = static_cast<float>(dMax - static_cast<float>(fBlockForceMin));
    dScaled = static_cast<double>(fRange) * static_cast<double>(afBlocked);
    float fBlockForce = static_cast<float>(static_cast<float>(fBlockForceMin) + dScaled);
    if (afBlocked > 75.0) {
        fHitForce = 0.0f;
    }
    if (fHitForce == 0.0 && fBlockForce == 0.0) {
        return;
    }

    bool bInThirdPerson = true;
    if (this == pPlayer) {
        bInThirdPerson = static_cast<PlayerCharacter*>(this)->Is3rdPerson();
    }
    if (!IsKnockedOut() && bInThirdPerson) {
        bool bQuadruped = false;
        bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
        if (pController) {
            bQuadruped = pController->QQuadruped();
        }
        bhkBlendController::DoHit(pHitNode, 0.0f, bQuadruped);
    }

    if (fHitForce != 0.0) {
        NiPoint3 kForce = kDirection * fHitForce;
        if (static_cast<int32_t>(static_cast<uint32_t>(iShowHitVector)) > 0) {
            NiColorA kColor(0.0f, 0.0f, 0.0f, 0.0f);
            // The attacker's hit vectors are blue for the player, else red.
            if (apAttacker->IsPc()) {
                kColor = NiColorA(0.0f, 0.0f, 1.0f, 1.0f);
            }
            else {
                kColor = NiColorA(1.0f, 0.0f, 0.0f, 1.0f);
            }
            NiPoint3 kArrow = kForce * 0.1f;
            NiNode* pArrow = static_cast<NiNode*>(BSTestObjects::CreateDirArrow(kArrow, &kColor));
            pArrow->SetTranslate(kStop);
            TES::pInstance->AddTempDebugObject(pArrow, static_cast<float>(fHitVectorDelay));
        }
        if (bInThirdPerson) {
            hkVector4 kHkForce;
            NI2HKNS(kHkForce, kForce);
            hkSphere kSphere;
            hkVector4 kHkStop;
            NI2HK(kHkStop, kStop);
            kSphere.setPosition(kHkStop);
            kSphere.setRadius(30.0f);
            bhkRigidBody* pBody = bhkUtilFunctions::GetClosestBody(p3D, kSphere, false);
            if (pBody) {
                bhkCollisionObject* pCollision = bhkCollisionObject::Getbhk(**pBody);
                if (pCollision) {
                    pSpot = pCollision->GetSceneGraphObject();
                }
            }
            if (!pSpot) {
                pSpot = BSUtilities::GetObjectByName(p3D, static_cast<const char*>(sHitSpot));
            }
            if (pSpot) {
                const char* pSpotName = pSpot->GetName().GetHandleString();
                InitializationFlow("Hit At %s\r\n", pSpotName);
            }
            bhkForceController::TestForce(pHitNode, kHkForce, pSpot);
        }
    }

    if (fBlockForce != 0.0 && bInThirdPerson) {
        kDirection.x = static_cast<float>(kDirection.x + 1.0);
        kDirection.y = static_cast<float>(kDirection.y + 1.0);
        kDirection.Unitize();
        // The block push is scaled by the hit force, not the block force, deliberately.
        NiPoint3 kBlockForce = kDirection * fHitForce;
        hkVector4 kHkBlock;
        NI2HKNS(kHkBlock, kBlockForce);
        pSpot = GetActorBone(p3D, *FixedStrings::QBip01LForearm());
        bhkForceController::TestForce(pHitNode, kHkBlock, pSpot);
    }
}

float Actor::GetBaseValueOverride(ActorValue::Index aiWhich, bool& abExists) {
    float fValue = pBaseValueOverrides.GetModifier(static_cast<uint8_t>(aiWhich), abExists);
    return fValue;
}

// Stores the override and marks the actor's base values changed for the save.
void Actor::SetBaseValueOverride(ActorValue::Index aiWhich, float afValue) {
    pBaseValueOverrides.SetModifier(static_cast<uint8_t>(aiWhich), afValue);
    AddChange(CHANGE_ACTOR_OVERRIDE_MODIFIERS);
}

void Actor::AddGunWobble() {
    NiMatrix3 addRot;
    NiMatrix3 gunWobble;
    if (this == pPlayer &&
        pCurrentProcess &&
        pCurrentProcess->GetCurrentWeapon() &&
        pCurrentProcess->GetWeaponDrawn()) {
        TESObjectWEAP* pweapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
        Animation* panim = Actor::GetAnimation();
        float timePassed = panim->GetTime();
        WEAPON_TYPE eWeaponType = pweapon->GetWeaponType();
        char* pwobbleNodeName = GetGunWobble(weaponToAnimGroupWeapon[eWeaponType], timePassed, gunWobble);
        if (pwobbleNodeName) {
            NiNode* p3D = TESObjectREFR::Get3D();
            NiAVObject* pwobbleObj = BSUtilities::GetObjectByName(
                static_cast<NiAVObject*>(p3D),
                static_cast<const char*>(pwobbleNodeName)
            );
            if (pwobbleObj) {
                float fx;
                float fdrift = GetGunSkill(GUN_SKILL_ACTOR);
                if (!IsAttacking()) {
                    fdrift = fdrift * fNonAttackGunWobbleMult.Float();
                }
                if (pwobbleObj->GetCollisionObject()) {
                    bhkBlendCollisionObject* pblendCol = static_cast<bhkBlendCollisionObject*>(
                        NiObject::DynamicCastStatic(&bhkBlendCollisionObject::ms_RTTI, pwobbleObj->GetCollisionObject())
                    );
                    if (pblendCol &&
                        !NiObject::DynamicCastStatic(&bhkBlendCollisionObjectAddRotation::ms_RTTI, pblendCol)) {
                        bhkBlendCollisionObjectAddRotation* pnewBlendCol;
                        pnewBlendCol = new bhkBlendCollisionObjectAddRotation;
                        pnewBlendCol->CopyFrom(pblendCol);
                        pwobbleObj->SetCollisionObject(pnewBlendCol);
                    }
                }
                else {
                    bhkBlendCollisionObjectAddRotation* pnewBlendCol;
                    pnewBlendCol = new bhkBlendCollisionObjectAddRotation;
                    pwobbleObj->SetCollisionObject(pnewBlendCol);
                }
                bhkBlendCollisionObjectAddRotation* pblendColObj = static_cast<bhkBlendCollisionObjectAddRotation*>(
                    NiObject::DynamicCastStatic(
                        &bhkBlendCollisionObjectAddRotation::ms_RTTI,
                        pwobbleObj->GetCollisionObject()
                    )
                );
                if (pblendColObj) {
                    float fy;
                    float fz;
                    addRot = NiMatrix3::IDENTITY;
                    gunWobble.ToEulerAnglesXYZ(fx, fy, fz);
                    float fZAngle = fz * fdrift;
                    float fYAngle = fy * fdrift;
                    float fXAngle = fx * fdrift;
                    gunWobble.FromEulerAnglesXYZ(fXAngle, fYAngle, fZAngle);
                    addRot = addRot * gunWobble;
                    pblendColObj->SetAdditionalRotation(addRot);
                }
            }
        }
    }
}

bool Actor::IsImmobile() {
    return GetActorBase()->IsImmobile();
}

// The health and the base health go through the ActorValueOwner subobject; the kill is queued on the 3D task thread
// (with a 4-byte damage block the task frees) when that thread is running, and done here otherwise. An attacking
// player that is not a combat target of a teammate's faction ally is dropped before the kill.
void Actor::HandleHealthDamage(Actor* apAttacker, float afDamage) {
    if (!IsDead(false)) {
        float fHealth = static_cast<ActorValueOwner*>(this)->GetActorFloatValue(ActorValue::HEALTH);
        float fBaseHealth = static_cast<ActorValueOwner*>(this)->GetBaseActorFloatValue(ActorValue::HEALTH);
        float fHealthRatio = 1.0f;
        if (fBaseHealth != 0.0f) {
            fHealthRatio = fHealth / fBaseHealth;
        }
        if (IsPlayerTeammate()) {
            if (fHealthRatio < 0.25f) {
                FollowerBarks::TriggerFollowerBark(this, FollowerBarks::BARK_MAJOR_HEALTH_WARNING);
            }
            else if (fHealthRatio < 0.5f) {
                FollowerBarks::TriggerFollowerBark(this, FollowerBarks::BARK_MINOR_HEALTH_WARNING);
            }
        }
        CombatController* pCombatController = GetCombatController();
        bool bFromCrime = false;
        if ((apAttacker == pPlayer && pCombatController) &&
            (!pCombatController->IsActoraCombatTarget(pPlayer) &&
             GetFactionFightReaction(pPlayer, bFromCrime) == FIGHT_REACTION_ALLY)) {
            apAttacker = nullptr;
        }

        if (fHealth < 1.0f) {
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                float* pfDamage = static_cast<float*>(ourMalloc(sizeof(float)));
                *pfDamage = afDamage;
                pMyKiller = apAttacker;
                TaskQueueInterface::GetInstance()->QueueActorKill(this, pfDamage, nullptr);
            }
            else {
                Kill(apAttacker, afDamage);
            }
        }
    }
}

void Actor::AttackedBy(Actor* apAttacker, ActiveEffect* apHostileEffect) {
    bool b360;
    if (!IsDead(false) && !GetGhost() && apAttacker != this) {
        TESPackage* peditorPack = GetPackageSetAsPcurrent();
        if (GetLifeState() != ACTOR_LIFE_STATE_REANIMATE || pCurrentProcess->GetCommandingActor() != apAttacker) {
            bool bminorcrime = false;
            bool bsendalarm = false;
            bool bsentAlarm = false;
            CombatController* pcombat = GetCombatController();
            BaseProcess* pprocess = pCurrentProcess;
            PROCESS_TYPE eprocessLevel = GetCurrentProcessType();
            TESObjectWEAP* pweapon = apAttacker ? apAttacker->GetCurrentWeapon() : nullptr;
            if (pweapon && pweapon->GetIsMinorCrime()) {
                bminorcrime = true;
            }

            if (!pcombat || !pcombat->IsActoraCombatTarget(apAttacker)) {
                bool bLineofSight;
                b360 = true;
                int32_t idetectvalue = GetDetectionLevelAgainstActor(
                    false,
                    apAttacker,
                    bLineofSight,
                    true,
                    true,
                    true,
                    &b360
                );
                DETECTION_LEVEL elevel = DETECTION_LOST;
                if (fSneakSeenMin.Float() < static_cast<double>(idetectvalue)) {
                    elevel = DETECTION_SEEN;
                }
                if (eprocessLevel == PROCESS_TYPE_HIGH) {
                    DetectionState* pstate = pprocess->GetDetectionState(apAttacker, 0);
                    if (!pstate && apAttacker->IsActor()) {
                        pstate = pprocess->InsertIntoDetectionList(
                            apAttacker,
                            elevel,
                            bLineofSight,
                            idetectvalue,
                            0,
                            b360,
                            true
                        );
                    }
                    pstate->iLevel = idetectvalue;
                    pstate->eDetectionLevel = elevel;
                    pstate->bLineofSight = bLineofSight;
                    pstate->b360view = b360;
                    pstate->bcombat = true;
                }
                if (apAttacker == pPlayer && (!pcombat || !pcombat->IsActoraCombatTarget(pPlayer))) {
                    bool bcrimecaused = false;
                    FIGHT_REACTION ereaction = GetFactionFightReaction(pPlayer, bcrimecaused);
                    if (ereaction == FIGHT_REACTION_FRIEND || ereaction == FIGHT_REACTION_ALLY) {
                        if (IsIgnoreFriendlyHits()) {
                            return;
                        }
                        int32_t iacceptableHitCount = 0;
                        if (ereaction == FIGHT_REACTION_FRIEND) {
                            iacceptableHitCount = pcombat ? iFriendHitCombatAllowed.Int()
                                                          : iFriendHitNonCombatAllowed.Int();
                        }
                        else {
                            iacceptableHitCount = pcombat ? iAllyHitCombatAllowed.Int()
                                                          : iAllyHitNonCombatAllowed.Int();
                        }
                        if (iacceptableHitCount > 0) {
                            if (iacceptableHitCount >= 1000) {
                                m_Extra.RemoveFriendHitsExtra();
                            }
                            int32_t inumHits = m_Extra.AddFriendHit();
                            AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
                            if (inumHits <= iacceptableHitCount) {
                                if (!GetIsTalking() &&
                                    !IsinDialogue() &&
                                    !ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_FRIENDLY_FIRE_COMMENTS)) {
                                    pCombatDialogueManager
                                        ->StartDialogue(this, apAttacker, DT_COMBAT, 2, true, nullptr);
                                }
                                return;
                            }
                        }
                    }
                    if (GetOwner()) {
                        Actor* pact = nullptr;
                        TESForm* pfaction__pnpc = GetOwner();
                        if (pfaction__pnpc->GetFormType() == FACT_ID) {
                            pact = ReferenceProcessLists
                                       .GetActorRefInHigh(static_cast<TESFaction*>(pfaction__pnpc), false, true);
                        }
                        else {
                            pact = ReferenceProcessLists.GetActorRefInHigh(static_cast<TESNPC*>(pfaction__pnpc), false);
                        }
                        if (pact) {
                            pact->AttackAlarm(apAttacker, bminorcrime, true);
                        }
                    }
                    AttackAlarm(apAttacker, bminorcrime, true);
                    bsentAlarm = true;
                }
                if (apAttacker != pPlayer &&
                    (IsActoraGuard() &&
                     (!pcombat && (apAttacker->IsNpc() && (apAttacker->IsNpc() && apAttacker->GetRace()))))) {
                    if (!bsentAlarm) {
                        AttackAlarm(apAttacker, bminorcrime, true);
                    }
                    return;
                }
                if (CanAttackActor(apAttacker) &&
                    (apAttacker == pPlayer ||
                     (apAttacker->GetCombatTarget() == this ||
                      (apAttacker->GetCommandingActor() == pPlayer && apAttacker->GetCombatTarget() == pPlayer))) &&
                    this != pPlayer) {
                    bool baggressor = false;
                    bool bcrimecaused = false;
                    FIGHT_REACTION eFactionFight = GetFactionFightReaction(apAttacker, bcrimecaused);
                    if (!IsAngryWithPlayer() &&
                        !bcrimecaused &&
                        GetShouldAttackActor(apAttacker, false, eFactionFight, false)) {
                        baggressor = true;
                    }
                    if (!pcombat && !apAttacker->IsActoraGuard() && apAttacker == pPlayer) {
                        if (GetCombatController() || (IsFleeing(false) || bsentAlarm)) {
                            ModifyDispositionTowardActor(apAttacker, fCrimeDispAttack.Float());
                            bsendalarm = true;
                        }
                        else if (IsCreature()) {
                            if (m_Extra.GetOwner()) {
                                TESForm* pOwner = m_Extra.GetOwner();
                                pPlayer->StealAlarm(this, nullptr, 1, 0, pOwner);
                            }
                        }
                        else {
                            AttackAlarm(apAttacker, bminorcrime, true);
                        }
                    }
                    pprocess->EnterCombat(
                        this,
                        apAttacker,
                        true,
                        true,
                        nullptr,
                        false,
                        false,
                        baggressor,
                        0,
                        false,
                        baggressor,
                        false,
                        nullptr
                    );
                }
                if (this != pPlayer || apAttacker->IsInCombat()) {
                    SetBeenAttacked(true);
                }
                TESActorBase* pactorbase = GetActorBase();
                if (pactorbase && pactorbase->listFactions.IsEmpty()) {
                    pactorbase = GetActorBase();
                }

                if (apAttacker == pPlayer) {
                    pactorbase->SetPCFactionFlags(PC_FACTIONFLAG_ATTACK);
                }
            }
            if (!bsentAlarm && bsendalarm && !IsInCombat()) {
                if (IsCreature()) {
                    if (m_Extra.GetOwner()) {
                        TESForm* pOwner = m_Extra.GetOwner();
                        pPlayer->StealAlarm(this, nullptr, 1, 0, pOwner);
                    }
                }
                else {
                    AttackAlarm(apAttacker, bminorcrime, true);
                }
            }
            pcombat = GetCombatController();
            if (pcombat) {
                pcombat->TakeHit(apAttacker, apHostileEffect);
            }
        }
    }
}

void Actor::HandleBlockedAttack(
    float afFullDamage,
    float afPercentageBlocked,
    Actor* apBlockingActor,
    Projectile* apProjectile
) {
    if (CombatController::GetShowVerboseMessages()) {
        const char* pcAttackerName = TESObjectREFR::GetFullName();
        const char* pcBlockerName = apBlockingActor->TESObjectREFR::GetFullName();
        Interface::PrintLine(
            "%.20s blocks %.0f%% of %.20s's blow!",
            pcBlockerName,
            afPercentageBlocked * 100.0,
            pcAttackerName
        );
    }
}

void Actor::PrecacheData() {
    uint32_t j;
    uint32_t i;
    if (pCurrentProcess) {
        static_cast<CachedValuesOwner*>(this)->CalculateAll();
    }
    GetFireNode();
    if (MobileObject::GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
        for (i = 0; i < 77; ++i) {
            static_cast<ActorValueOwner*>(this)->GetActorFloatValue(static_cast<ActorValue::Index>(i));
        }
        for (j = 3; j < 16; ++j) {
            MobileObject::GetCurrentAIProcess()->HasMovementAnim(static_cast<ANIM_GROUP_ENUM>(j));
        }
    }
}

// The head tracking: the ragdoll look IK of a no-head-track package, then the look-at target update, the rotation
// towards it and the look IK target handed to the ragdoll controller, or the ease-out when it has none.
void Actor::ProcessTracking(float afDelta) {
    if (this == pPlayer || !pCurrentProcess) {
        return;
    }
    TESPackage* pRunningPackage = pCurrentProcess->GetPackageThatIsRunning();
    if (pRunningPackage && pRunningPackage->GetNoHeadTrack()) {
        if (pRagdollController && pRagdollController->GetLookIKActive() && !pRagdollController->GetEaseOutLookAtIK()) {
            pRagdollController->SetEaseOutLookAtIK(true);
        }
        return;
    }
    bhkCharacterController* pCharController = static_cast<bhkCharacterController*>(GetCharController());
    if (pCharController) {
        if (pCharController->GetLODDistance() < 0.0) {
            return;
        }
        float fLODDistance = pCharController->GetLODDistance();
        if (fAIMaxHeadTrackDistanceFromPC.Float() < fLODDistance) {
            return;
        }
    }
    fUpdateTargetTimer = fUpdateTargetTimer - afDelta;
    pCurrentProcess->SetHeadTrackChangeTimer(pCurrentProcess->GetHeadTrackChangeTimer() - afDelta);
    TESObjectREFR* pHeadTrackTarget = pCurrentProcess->GetCurrentHeadTrackTarget();
    bool bEscort = false;
    if (pCurrentProcess->GetPackageThatIsRunning() &&
        pCurrentProcess->GetPackageThatIsRunning()->GetPackType() == PACKAGE_TYPE_ESCORT) {
        bEscort = true;
    }

    if (pHeadTrackTarget == pPlayer) {
        DetectionState* pDetectionState = pCurrentProcess->GetDetectionState(pPlayer, 0);
        if (pDetectionState &&
            pDetectionState->iLevel <= 0 &&
            !pCurrentProcess->GetTalkingtoPC() &&
            !bEscort &&
            !pCurrentProcess->GetGreetingFlag()) {
            pCurrentProcess->ClearAllHeadTrackTargets();
        }
        pHeadTrackTarget = nullptr;
    }
    if (iHeadTrackingActorUpdateCount > static_cast<int32_t>(iUpdateActorsPerFrame.UInt())) {
        return;
    }
    BaseProcess* pHighProcess = nullptr;
    if (pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
        pHighProcess = pCurrentProcess;
    }
    if (!pHighProcess) {
        return;
    }
    // Calls TESObjectREFR::Get3D directly, not through the vtable.
    if (!TESObjectREFR::Get3D()) {
        return;
    }
    if (!bWasInFrustum) {
        bWasInFrustum = true;
    }
    if (!IsVisible(ACTOR_VISIBILITY_ALL)) {
        bWasInFrustum = false;
    }
    if (GetFaceAnimationData()) {
        float fPlayerDistance = pPlayer->GetDistanceFromReference(this, false, false);
        if (fAIMaxHeadTrackDistanceFromPC.Float() < fPlayerDistance) {
            return;
        }
    }
    pHeadTrackTarget = pCurrentProcess->GetCurrentHeadTrackTarget();
    if (pHeadTrackTarget && pHeadTrackTarget->IsMobileObject() && !pHeadTrackTarget->Get3D()) {
        pCurrentProcess->ClearAllHeadTrackTargets();
        pHeadTrackTarget = nullptr;
    }
    fHeadTrackTimer = fHeadTrackTimer - afDelta;
    if (fUpdateTargetTimer <= 0.0) {
        ++iHeadTrackingActorUpdateCount;
        float fDelayMax = fUpdateDelaySecondsMax.Float();
        float fDelayMin = fUpdateDelaySecondsMin.Float();
        fUpdateTargetTimer = RandomFloat(fDelayMin, fDelayMax);
        if (pCurrentProcess->CanSetDefaultHeadTrackTarget()) {
            if (fHeadTrackTimer < 0.0) {
                fHeadTrackTimer = 0.0f;
            }
            TESObjectREFR* pBestTarget = GetBestDefaultHeadTrackTarget();
            if (pBestTarget != pHeadTrackTarget) {
                pHeadTrackTarget = pBestTarget;
                pCurrentProcess->SetDefaultHeadTrackTarget(pBestTarget);
                pCurrentProcess->SetHeadTrackChangeTimer(fAIHoldDefaultHeadTrackTimer.Float());
                float fNewMax = fUpdateDelayNewTargetSecondsMax.Float();
                float fNewMin = fUpdateDelayNewTargetSecondsMin.Float();
                fHeadTrackTimer = RandomFloat(fNewMin, fNewMax);
            }
        }
    }
    NiPoint3 kTargetPos = pHeadTrackTarget ? pHeadTrackTarget->GetLocationOnReference() : NiPoint3::ZERO;
    if (pHeadTrackTarget) {
        if (bShouldRotateToTrack && IsPathingComplete()) {
            NiPoint3 kDirection = kTargetPos - GetLocationOnReference();
            float fZAngle = GetZAngleFromVector(kDirection);
            float fZero = 0.0f;
            float fAngleDelta = GetAngleDelta(GetHeading(false), fZAngle, fZero);
            float fTurnThreshold = static_cast<float>(
                iActorTurnDegree.UInt() * 0.01745329238474369 * 0.800000011920929
            );
            if (IsTurning()) {
                fTurnThreshold = static_cast<float>(
                    iActorKeepTurnDegree.UInt() * 0.01745329238474369 * 0.800000011920929
                );
            }
            if (fTurnThreshold < std::fabs(fAngleDelta)) {
                RequestRotate(kTargetPos, false);
            }
        }
        pPlayer->GetDistanceFromReference(pHeadTrackTarget, false, true);
    }
    bool bNoHeadTrack = false;
    if (GetLifeState() && (GetLifeState() != ACTOR_LIFE_STATE_RESTRAINED)) {
        bNoHeadTrack = true;
    }

    if (bDisableHeadTracking.Bool()) {
        bNoHeadTrack = true;
    }
    if (pCurrentProcess && !bEscort) {
        // Calls Actor::GetAnimation directly, not through the vtable.
        if (TESAnimGroup::IsAttackAction(Actor::GetAnimation()->GetGroup(AGS_WEAPON))) {
            bNoHeadTrack = true;
        }
        else {
            NiAVObject* pBone = nullptr;
            if (GetBiped()) {
                pBone = GetBiped()->GetParentBone(BIPED_BONE_HEAD);
            }
            else if (IsCreature()) {
                pBone = BSUtilities::GetObjectByName(Get3D(), *FixedStrings::GetBip01_Head());
            }
            if (pBone) {
                NiTimeController* pController = pBone->GetController(&NiFloatExtraDataController::ms_RTTI);
                if (pController) {
                    NiInterpolator* pInterpolator = static_cast<NiSingleInterpController*>(pController)
                                                        ->GetInterpolator(0);
                    if (NiObject::IsExactKindOf(&NiBlendFloatInterpolator::ms_RTTI, pInterpolator) &&
                        static_cast<NiBlendFloatInterpolator*>(pInterpolator)->m_fFloatValue >= 90.0) {
                        bNoHeadTrack = true;
                    }
                }
            }
        }
    }
    TESBoundObject* pObjectRef = pHeadTrackTarget ? pHeadTrackTarget->GetObjectReference() : nullptr;
    if (pRagdollController &&
        !bNoHeadTrack &&
        pHeadTrackTarget &&
        pHeadTrackTarget != this &&
        (!pObjectRef || pObjectRef->GetFormType() != 0x1C)) {
        float fTrackingDistance = (GetLocationOnReference() - kTargetPos).Length();
        if (fTrackingDistance <= bhkRagdollController::QMaxTrackingDistance() &&
            fTrackingDistance > bhkRagdollController::QMinTrackingDistance()) {
            pRagdollController->SetEaseOutLookAtIK(false);
            pRagdollController->SetLookIKActive(true);
            NiPoint3 kLookAt = pHeadTrackTarget->GetLookingAtLocation();
            pRagdollController->SetLookAtTarget(&kLookAt, true);
        }
    }
    else if (pRagdollController && pRagdollController->GetLookIKActive() && !pRagdollController->GetEaseOutLookAtIK()) {
        pRagdollController->SetEaseOutLookAtIK(true);
    }
}

// Sets the face expression of a high-process actor near enough to the player: fear when fleeing, anger in combat,
// otherwise its mood when it is not speaking.
void Actor::ProcessEmotions(float afDelta) {
    if (GetCurrentProcessType() != PROCESS_TYPE_HIGH) {
        return;
    }
    bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
    if (pController) {
        if (pController->GetLODDistance() < 0.0) {
            return;
        }
        double fLODDistance = pController->GetLODDistance();
        if (fAIMaxHeadTrackDistanceFromPC.Float() < fLODDistance) {
            return;
        }
    }
    BSFaceGenAnimationData* pFaceData = Actor::GetFaceAnimationData();
    if (pFaceData) {
        double fDistance = pPlayer->GetDistanceFromReference(this, false, false);
        if (fAIMaxHeadTrackDistanceFromPC.Float() < fDistance) {
            return;
        }
    }
    if (!pFaceData) {
        return;
    }
    if (!BSFaceGenManager::GetUseEmotions()) {
        pFaceData->ResetCurrentMorphs(0.0f, true, false, false, false, false);
    }
    else if (IsFleeing(false)) {
        pFaceData->SetExpressionTarget(BS_FG_EXPRESSION_FEAR, 1.0f);
    }
    else if (IsInCombat()) {
        pFaceData->SetExpressionTarget(BS_FG_EXPRESSION_COMBAT_ANGRY, 1.0f);
    }
    else if (pCurrentProcess) {
        BSSoundHandle kSound = pCurrentProcess->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
        bool bSilent = !kSound.IsValid() && !pCurrentProcess->GetLastSpeakingEmotion();
        if (bSilent) {
            pFaceData->SetExpressionTargetFromMood(
                static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::MOOD),
                false
            );
        }
    }
    else {
        pFaceData->SetExpressionTargetFromMood(
            static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::MOOD),
            false
        );
    }
}

void Actor::CreateActorMover() {
    pActorMover = new ActorMover(this);
}

void Actor::DestroyActorMover() {
    // This address (&m_Extra) is never null.
    ExtraDataList* pextra = &m_Extra;
    InventoryChanges* pchanges = pextra->GetContainerChanges();
    PathManager::QInstance()->ClearInventoryPointersFromCurrentRequests(pchanges);
    if (pActorMover) {
        delete pActorMover;
        pActorMover = nullptr;
    }
}

// Asks the mover unless afDeltaT <= 0; a NaN afDeltaT asks the mover too.
NiPoint3 Actor::CalculateAnticipatedLocation(float afDeltaT) {
    NiPoint3 kLocation;
    if (afDeltaT <= 0.0) {
        kLocation = *QLocationOnReference();
    }
    else {
        kLocation = pActorMover->GetAnticipatedLocation(afDeltaT);
    }
    return kLocation;
}

float Actor::GetWidth() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        return pCurrentProcess->GetWidth(static_cast<CachedValuesOwner*>(this));
    }
    return CalculateCachedWidth();
}

float Actor::GetLength() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        return pCurrentProcess->GetLength(static_cast<CachedValuesOwner*>(this));
    }
    return CalculateCachedLength();
}

float Actor::GetForwardLength() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        return pCurrentProcess->GetCachedForwardLength(static_cast<CachedValuesOwner*>(this));
    }
    return CalculateCachedForwardLength();
}

float Actor::GetPerceptionCondition() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        return pCurrentProcess->GetPerceptionCondition(static_cast<CachedValuesOwner*>(this));
    }
    return CalculateCachedPerceptionCondition();
}

float Actor::GetEyeLevel() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        return pCurrentProcess->GetEyeLevel(static_cast<CachedValuesOwner*>(this));
    }
    return CalculateCachedEyeLevel();
}

void Actor::GetEyeVector(NiPoint3& aOrigin, NiPoint3& aDirection, bool abIncludeCameraOffset) {
    NiMatrix3 rot;
    if (this == pPlayer && p1stPersonCamera) {
        aOrigin = p1stPersonCamera->GetWorldTranslate();
    }
    else {
        aOrigin = *QLocationOnReference();
        aOrigin.z = GetEyeLevel() + aOrigin.z;
    }
    if (abIncludeCameraOffset && this == pPlayer && pPlayer->IsActually3rdPerson()) {
        aOrigin += pPlayer->GetCamera3rdPersonShoulderOffset();
    }
    float fHeading = GetHeading(false);
    rot.MakeZRotation(fHeading);
    if (this == pPlayer) {
        NiMatrix3 rot2;
        float fLooking = GetLooking();
        rot2.MakeXRotation(fLooking);
        rot = rot * rot2;
    }
    rot.GetCol(1u, aDirection);
    aDirection.Unitize();
}

void Actor::QueueReplacementLocomotions(bool abForceReplaceAnimation) {
    bool bhurtAnimsLoaded = false;
    bool bfemaleAnimsLoaded = false;
    bool btoddlerAnimsLoaded = false;
    bool bchildAnimsLoaded = false;
    uint32_t ianimationCount = 2 - (pPlayer != this);
    for (uint32_t i = 0; i < ianimationCount; ++i) {
        Animation* panim = nullptr;
        if (this == pPlayer) {
            if (i == 1) {
                panim = pPlayer->GetAnimation(true);
            }
            else {
                panim = pPlayer->GetAnimation(false);
            }
        }
        else {
            panim = Actor::GetAnimation();
        }
        if (!panim) {
            return;
        }
        if (!panim->GetMapAnimSequenceBase()) {
            return;
        }
        AnimSequenceBase* panimSequence = nullptr;
        NiTPointerMap<uint16_t, AnimSequenceBase*>* pmap = panim->GetMapAnimSequenceBase();
        if (!pmap->GetAt(3u, panimSequence)) {
            return;
        }
        if (!panimSequence) {
            return;
        }
        BSAnimGroupSequence* psequence = panimSequence->GetSequenceToPlay(-1);
        if (!psequence || !psequence->m_kName.GetHandleString()) {
            return;
        }
        const char* pname = psequence->m_kName.GetHandleString();
        if (std::strstr(pname, "Hurt\\")) {
            bhurtAnimsLoaded = true;
        }
        if (std::strstr(pname, "Toddler\\")) {
            btoddlerAnimsLoaded = true;
        }
        if (std::strstr(pname, "Child\\")) {
            bchildAnimsLoaded = true;
        }
        if (std::strstr(pname, "Female\\")) {
            bfemaleAnimsLoaded = true;
        }
    }
    bool bhurtNow = IsCrippled();
    bool btoddlerNow = false;
    bool bchildNow = false;
    bool bfemaleNow = GetSex() == TESActorBase::SEX_FEMALE;
    if (this == pPlayer) {
        btoddlerNow = pPlayer->IsPCToddler();
    }
    bchildNow = IsChild(true);
    if (bchildNow) {
        bfemaleAnimsLoaded = bfemaleNow;
    }
    if (abForceReplaceAnimation ||
        (bchildNow != bchildAnimsLoaded ||
         (btoddlerNow != btoddlerAnimsLoaded || (bhurtNow != bhurtAnimsLoaded || bfemaleAnimsLoaded != bfemaleNow)))) {
        if (this == pPlayer) {
            Animation* panim = pPlayer->GetAnimation(false);
            QueueReplacementLocomotion(bhurtNow, panim, static_cast<char*>(nullptr), static_cast<QueuedFile*>(nullptr));
            const char* pcSkeleton = sFirstPersonSkeleton.String();
            panim = pPlayer->GetAnimation(true);
            QueueReplacementLocomotion(bhurtNow, panim, pcSkeleton, static_cast<QueuedFile*>(nullptr));
        }
        else {
            Animation* panim = Actor::GetAnimation();
            QueueReplacementLocomotion(bhurtNow, panim, static_cast<char*>(nullptr), static_cast<QueuedFile*>(nullptr));
        }
    }
}

// Without 3D, while invisible or while the player sleeps there is no delta. A knocked-out or dead actor takes the
// knocked-down delta rotated into its heading; an actor stuck to someone or without animation has none; otherwise
// the animation's delta, rotated and keeping Z for a no-collision actor.
bool Actor::GetAnimationMoveDelta(NiPoint3& aMoveDelta) {
    if (!Get3D() || !IsVisible(7) || pPlayer->IsSleepingorResting()) {
        aMoveDelta = NiPoint3::ZERO;
        return false;
    }
    if (IsKnockedOut() || IsDead(false)) {
        bool bAlive = !IsDead(false);
        bhkUtilFunctions::GetKnockedDownMovementDelta(TESObjectREFR::Get3D(), &aMoveDelta, bAlive);
        NiPoint3 kAngle = GetAngleOnReference();
        NiMatrix3 kRotation;
        kRotation.MakeZRotation(kAngle.z);
        aMoveDelta = aMoveDelta * kRotation;
        return true;
    }
    BaseProcess* pProcess = GetCurrentAIProcess();
    bhkCharacterController* pController = pProcess->GetCharController();
    bhkCharacterListener* pListener = pController ? static_cast<bhkCharacterListener*>(pController) : nullptr;
    if (pListener && pListener->StuckToSomeone()) {
        aMoveDelta = NiPoint3::ZERO;
        return true;
    }
    Animation* pAnimation = GetAnimation();
    if (!pAnimation) {
        aMoveDelta = NiPoint3::ZERO;
        return true;
    }
    if (GetNoCollision()) {
        pAnimation->GetDelta(&aMoveDelta, this, true, false);
    }
    else {
        pAnimation->GetDelta(&aMoveDelta, this, false, true);
    }
    return true;
}

const char* Actor::GetPickUpSoundName(TESBoundObject* apobjREFR, bool abPickUp, bool abUse) {
    BGSEquipType::EQUIP_TYPE eEquipType;
    if (!apobjREFR) {
        return nullptr;
    }
    BGSPickupPutdownSounds* ppickupPutdownSound = dynamic_cast<BGSPickupPutdownSounds*>(apobjREFR);
    if (!abUse && (abPickUp && (ppickupPutdownSound && ppickupPutdownSound->pPickupSound))) {
        return ppickupPutdownSound->pPickupSound->GetFormEditorID();
    }
    if (!abUse && (!abPickUp && (ppickupPutdownSound && ppickupPutdownSound->pPutdownSound))) {
        return ppickupPutdownSound->pPutdownSound->GetFormEditorID();
    }
    int32_t iformType = apobjREFR->GetFormType();
    if (iformType == WEAP_ID) {
        eEquipType = BGSEquipType::GetEquipType(apobjREFR);
        if (!eEquipType) {
            if (abPickUp) {
                return "UIItemGunsBigUp";
            }

            return "UIItemGunsBigDown";
        }
        if (eEquipType == BGSEquipType::ET_SMALL_GUN) {
            if (abPickUp) {
                return "UIItemGunsSmallUp";
            }

            return "UIItemGunsSmallDown";
        }

        if (eEquipType == BGSEquipType::ET_MELEE_WEAPON) {
            if (abPickUp) {
                return "UIItemMeleeUp";
            }

            return "UIItemMeleeDown";
        }
    }
    else if (iformType == ALCH_ID && abUse) {
        AlchemyItem* palchemy = static_cast<AlchemyItem*>(apobjREFR);
        if (!palchemy->data.pConsumptionSound) {
            return nullptr;
        }
        TESSound* pconsumptionSound = palchemy->data.pConsumptionSound;
        return pconsumptionSound->GetFormEditorID();
    }
    if (abPickUp) {
        return "UIItemGenericUp";
    }

    return "UIItemGenericDown";
}

bool Actor::CanAttackActor(Actor* apActor) {
    if (static_cast<const void*>(apActor) != static_cast<const void*>(pPlayer) &&
        (static_cast<const void*>(this) == static_cast<const void*>(pPlayer) || Actor::IsPlayerTeammate())) {
        return true;
    }
    FIGHT_REACTION eReaction = FIGHT_REACTION_NEUTRAL;
    return GetShouldAttackActor(apActor, true, eReaction, false);
}

float Actor::CalculateCombatStrength(float afDamagePerSecond) {
    float fdamagePerSecond = afDamagePerSecond;
    if (afDamagePerSecond < 0.0) {
        fdamagePerSecond = GetDamagePerSecond();
    }
    float fhealth = static_cast<float>(GetActorValue(ActorValue::HEALTH));
    float farmorRating = GetArmorThreshold() / 30.0;
    farmorRating = BSMIN(farmorRating, 0.99f);
    float fmodifiedHealth = fhealth / (1.0 - farmorRating);
    float fresult = fdamagePerSecond * fmodifiedHealth;
    return fresult;
}

float Actor::GetCurrentSpeed() {
    if (!pCurrentProcess || pCurrentProcess->eLevel) {
        if (IsInCombat() && !bSearchingInCombat) {
            return GetRunSpeed();
        }
        return GetWalkSpeed();
    }
    if ((GetMoveMode() & 512) == 0) {
        if ((GetMoveMode() & 2048) != 0) {
            return GetSwimWalkSpeed();
        }
        if ((GetMoveMode() & ActorMover::MF_FLYING) != 0) {
            return GetFlySpeed();
        }
        return GetWalkSpeed();
    }
    if ((GetMoveMode() & 2048) == 0) {
        if ((GetMoveMode() & ActorMover::MF_FLYING) != 0) {
            return GetFlySpeed();
        }
        return GetRunSpeed();
    }
    return GetSwimRunSpeed();
}

bool Actor::QueueAttack(ANIM_GROUP_ENUM aeAttack) {
    TESObjectWEAP* pweapon;
    if (this != pPlayer && bDisableNPCAttacks.Bool()) {
        return false;
    }
    if (pCurrentProcess->GetCurrentWeapon()) {
        pweapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
    }
    else {
        pweapon = nullptr;
    }
    ItemChange* pammo = pCurrentProcess->GetCurrentAmmo();
    if (pweapon && pweapon->GetCurrentAmmo(this)) {
        if (!pammo) {
            return false;
        }
        if (!pammo->iNumber) {
            return false;
        }
    }
    if (GetAnimAction() == ANIM_ACTION_RELOAD || GetAnimAction() == ANIM_ACTION_RELOAD_LOOP) {
        return false;
    }
    if (!IsWeaponDrawn()) {
        return false;
    }
    if (!pCurrentProcess->CanAttack()) {
        return false;
    }
    Animation* panim = GetAnimation();
    if (!panim) {
        return false;
    }
    if (!panim->GetCurrentSequence(AGS_WEAPON) ||
        (panim->GetCurrentSequence(AGS_WEAPON)->GetAnimGroup()->GetMove() == AGM_WALK) == IsSneaking()) {
        float afPercentTrue;
        int32_t iweaponCondition;
        char ballowJam = 1;
        TESPackage* ppack = pCurrentProcess->GetPackageThatIsRunning();
        if (ppack) {
            ballowJam = ppack->GetPackType() != PACKAGE_TYPE_USE_ITEM_AT &&
                ppack->GetPackType() != PACKAGE_TYPE_USE_WEAPON;
        }
        if (ballowJam &&
            (pweapon &&
             (pweapon->GetCurrentAmmo(this) &&
              ((iweaponCondition = pCurrentProcess->GetWeaponCondition(this), !pCurrentProcess->IsSustainedFire()) &&
               (GetWeaponConditionJamChance(iweaponCondition) > 0.0 &&
                (afPercentTrue = GetWeaponConditionJamChance(iweaponCondition), RandomBool(afPercentTrue))))))) {
            ANIM_GROUP_ENUM ejamAnimGroup = static_cast<ANIM_GROUP_ENUM>(pweapon->GetReloadAnim(false) + 23);
            uint16_t sanimGroup = GetAnimGroup(ejamAnimGroup, nullptr, false, nullptr);
            if (TESAnimGroup::GetType(sanimGroup) == ejamAnimGroup) {
                eQueuedattack = ejamAnimGroup;
            }
            return false;
        }

        uint16_t sanimGroup = GetAnimGroup(aeAttack, nullptr, false, nullptr);
        if (this == pPlayer) {
            StartAttack(aeAttack);
            return true;
        }

        if (TESAnimGroup::IsAttackAction(sanimGroup)) {
            eQueuedattack = aeAttack;
            return true;
        }

        const char* pcActorName;
        const char* pcWeaponName;
        const char* pwantAnim;
        const char* pplayAnim;
        ANIM_GROUP_ENUM eplayAnim = TESAnimGroup::GetType(sanimGroup);
        if (eplayAnim == ANIM_GROUP_NONE) {
            pplayAnim = "ANIM_GROUP_NONE";
        }
        else {
            pplayAnim = animGroupInfo[eplayAnim].pName;
        }
        if (aeAttack == ANIM_GROUP_NONE) {
            pwantAnim = "ANIM_GROUP_NONE";
        }
        else {
            pwantAnim = animGroupInfo[aeAttack].pName;
        }
        if (pweapon) {
            if (pweapon->GetFormEditorIDLength()) {
                pcWeaponName = pweapon->GetFormEditorID();
            }
            else {
                pcWeaponName = pweapon->GetFullName();
            }
        }
        else {
            pcWeaponName = "hand-to-hand";
        }
        if (GetFormEditorIDLength()) {
            pcActorName = GetFormEditorID();
        }
        else {
            pcActorName = GetFullName();
        }
        Warning(
            "COMBAT: '%s' is queuing an attack with weapon '%s' with attack '%s' that has non-attack "
            "group '%s'.",
            pcActorName,
            pcWeaponName,
            pwantAnim,
            pplayAnim
        );
        return false;
    }

    panim->BlendOut(AGS_WEAPON, false);
    return false;
}

void Actor::ClearMoveModeAll() {
    StopMoving();
}

// Tests ActorValue::IsSkill and deliberately does nothing with the answer.
void Actor::CheckSkillLevelChange(ActorValue::Index aiWhich, bool abBaseValueModified) {
    ActorValue::IsSkill(aiWhich);
}

// Applies the ragdoll templates of apNodeRDT's "RDT" extra data to the nodes of the actor's 3D that carry their
// names, at the actor's scale.
bool Actor::ApplyConstrTemplate(NiNode* apNodeRDT) {
    NiNode* p3D = Get3D();
    NiFixedString kRDTName("RDT");
    NiExtraData* pExtraData = apNodeRDT->GetExtraData(kRDTName);
    float fScale = GetScale();
    if (!p3D) {
        return false;
    }
    if (!pExtraData) {
        return false;
    }
    bhkRagdollTemplate* pRagdollTemplate = static_cast<bhkRagdollTemplate*>(pExtraData);
    for (uint32_t i = 0; i < pRagdollTemplate->QNumTemplates(); ++i) {
        bhkRagdollTemplateData* pTemplateData = pRagdollTemplate->GetRagdollTemplateData(i);
        NiAVObject* pObject = p3D->GetObjectByName(pTemplateData->GetName());
        if (pObject) {
            pTemplateData->ReplaceRagdollData(pObject, fScale);
        }
    }
    return true;
}

bool Actor::IsProcessingTresPassAlarm() {
    TESPackage* prunning = MobileObject::GetCurrentAIProcess()->GetPackageThatIsRunning();
    if (!dynamic_cast<TrespassPackage*>(prunning)) {
        if (!IsRunningRunOnce()) {
            return false;
        }
        if (MobileObject::GetCurrentAIProcess()->GetPackageThatIsRunning()->GetPackType() != PACKAGE_TYPE_FOLLOW) {
            return false;
        }
    }
    return true;
}

void Actor::QueueUnEquipObject(
    TESBoundObject* apObject,
    int32_t aiNumber,
    ExtraDataList* apExtra,
    bool abForceUnequip,
    bool abEquipCalling,
    bool abPlayPickUpSounds
) {
    if (!pCurrentProcess ||
        (pCurrentProcess->GetProcessLevel() > PROCESS_TYPE_MIDDLE_HIGH ||
         (this == pPlayer && Interface::IsInMenuMode()))) {
        UnEquipObject(apObject, aiNumber, apExtra, abForceUnequip, abEquipCalling, abPlayPickUpSounds);
    }
    else {
        pCurrentProcess->AddQueuedItem(
            this,
            false,
            apObject,
            aiNumber,
            apExtra,
            false,
            false,
            false,
            abForceUnequip,
            abEquipCalling,
            abPlayPickUpSounds
        );
    }
}

// Applies the hit apData to the actor: script hit events, challenges, the damage and its critical, meltdown, VATS and
// knockdown effects, limb damage and dismemberment, hit idles, staggers, blocking, knockback, the ragdoll impulse,
// blood, rumble and the combat reactions. With the 3D task thread running the hit is queued instead.
void Actor::HitMe(HitData* apData, char cMeleeEffect) {
    if (!pCurrentProcess) {
        return;
    }
    bool bKnockedDown = false;
    bool bUnarmed = false;
    ClearLastHitData();
    if (!apData) {
        return;
    }
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        TaskQueueInterface::GetInstance()->QueueActorHitMe(this, apData);
        return;
    }
    if (IsPc() && static_cast<int32_t>(apData->eDamageLimb) == BGSBodyPart::LIMB_WEAPON) {
        return;
    }
    if (IsPc()) {
        // Looks the attacker's and the weapon's names up ("<unknown>" without one) and never uses them.
        if (apData->pAggressor) {
            apData->pAggressor->GetFullName();
        }
        if (apData->pWeapon) {
            apData->pWeapon->TESFullName::GetFullName();
        }
    }
    SIT_SLEEP_STATE eSitSleepState = GetSitSleepState();
    if (GetSitSleepState()) {
        BaseProcess* pMarkerProcess = GetCurrentAIProcess();
        if (pMarkerProcess->GetFurnitureMarkerID() > 20) {
            BaseProcess* pPackageProcess = GetCurrentAIProcess();
            TESPackage* pRunOncePackage = pPackageProcess->GetRunOncePackage();
            if (!pRunOncePackage || pRunOncePackage->GetPackType() != PACKAGE_TYPE_GET_UP) {
                if (GetCurrentAIProcess() && GetCurrentAIProcess()->GetCurrentFurniture()) {
                    BaseProcess* pIndexProcess = GetCurrentAIProcess();
                    BaseProcess* pFurnitureProcess = GetCurrentAIProcess();
                    int32_t iMarkerIndex = pIndexProcess->GetCurrentFurnitureIndex();
                    pFurnitureProcess->GetCurrentFurniture()->SetMarkerUsed(iMarkerIndex, false);
                }
                SetChaseBip(false);
                GetOutofFurnitureQuick();
                TESObjectREFR* pKnockSource = this;
                if (apData->pAggressor) {
                    pKnockSource = apData->pAggressor;
                }
                const NiPoint3* pKnockLocation = &pKnockSource->GetLocationOnReference();
                pCurrentProcess->KnockExplosion(this, *pKnockLocation, 1.0f);
                bKnockedDown = true;
            }
        }
    }
    if (apData->pAggressor) {
        apData->pAggressor->ClearLastAttackHitData();
        apData->pAggressor->SetLastAttackHitData(apData);
    }
    SetLastHitData(apData);
    uint16_t usWeaponType = 1;
    if (apData->pWeapon) {
        usWeaponType = static_cast<uint16_t>(static_cast<uint16_t>(apData->pWeapon->GetWeaponType()) + 1);
        if (apData->pWeapon->GetWeaponSkill() == ActorValue::UNARMED) {
            bUnarmed = true;
        }
    }
    // A hit without a weapon is unarmed unless it came from an explosion.
    else if (!(apData->uiFlags & HitData::HD_EXPLOSION)) {
        bUnarmed = true;
    }
    bool bCritical = (apData->uiFlags & HitData::HD_CRITICAL) != 0;
    if (apData->pAggressor == pPlayer && !IsDead(false) && bCritical && !IsPlayerTeammate()) {
        TESForm* pChallengeBase = GetObjectReference();
        if (IsLeveledCreature()) {
            pChallengeBase = GetExtra()->GetLevCreaTemplate();
        }
        TESChallengeManager::UpdateChallenge(
            CHALLENGE_TYPE_HIT,
            1,
            pChallengeBase,
            apData->pWeapon,
            static_cast<uint16_t>(eLimbForChallenge[static_cast<int32_t>(apData->eDamageLimb) + 1]),
            1,
            usWeaponType
        );
    }
    ExtraDataList* pHitExtra = GetExtra();
    Script::SetActionFlag(apData->pAggressor, pHitExtra, SCRIPT_ACTION_ONHIT);
    if (apData->pWeapon) {
        ExtraDataList* pWeaponExtra = GetExtra();
        Script::SetActionFlag(apData->pWeapon, pWeaponExtra, SCRIPT_ACTION_ONHITWITH);
        if (apData->pAggressor &&
            apData->pAggressor->GetCurrentAIProcess() &&
            static_cast<TESScriptableForm*>(apData->pWeapon)->GetFormScript()) {
            BaseProcess* pAggressorProcess = apData->pAggressor->GetCurrentAIProcess();
            ItemChange* pWeaponItem = pAggressorProcess->GetCurrentWeapon();
            if (pWeaponItem && pWeaponItem->GetExtraDataList() && pWeaponItem->GetExtraDataList()->GetItem()) {
                ExtraDataList* pItemExtra = pWeaponItem->GetExtraDataList()->GetItem();
                Script::SetActionFlag(apData->pTarget, pItemExtra, SCRIPT_ACTION_ONHIT);
            }
        }
        if (apData->pWeapon->GetCurrentAmmo(apData->pAggressor)) {
            TESAmmo* pAmmo = static_cast<TESAmmo*>(apData->pWeapon->GetCurrentAmmo(apData->pAggressor));
            if (static_cast<TESScriptableForm*>(pAmmo)->GetFormScript()) {
                TESAmmo* pScriptAmmo = static_cast<TESAmmo*>(apData->pWeapon->GetCurrentAmmo(apData->pAggressor));
                static_cast<TESScriptableForm*>(pScriptAmmo)->GetFormScript()->RunScriptEffectStart(this, nullptr);
            }
        }
    }
    else {
        ExtraDataList* pFistsExtra = GetExtra();
        Script::SetActionFlag(pFists, pFistsExtra, SCRIPT_ACTION_ONHITWITH);
    }
    bool bCrippled = false;
    bForceHitReaction = false;
    bool bMeleeKnockdown = false;
    bool bStaggeredDown = false;
    bool bSpeak = false;
    float fHealthDamage = apData->fHealthDamage;
    float fFatigueDamage = apData->fFatigueDamage;
    float fTotalDamage = apData->fTotalDamage;
    float fPercentBlocked = apData->fPercentBlocked;
    if (apData->pAggressor &&
        apData->pAggressor->IsPc() &&
        apData->IsFlagSet(HitData::HD_CRITICAL) &&
        !IsDead(false) &&
        !IsPc()) {
        const char* pCriticalText;
        if (apData->IsFlagSet(HitData::HD_SNEAK_ATTACK)) {
            pCriticalText = sSneakAttackCriticalStrike.String();
        }
        else {
            pCriticalText = sCriticalStrike.String();
        }
        char acMessage[500];
        BSsprintf(acMessage, 500, "%s %s", pCriticalText, GetFullName());
        // The string is constructed from "" only; a BSStringT local would call the default constructor's Init first, so
        // the string deliberately lives in raw storage.
        alignas(BSStringT<char>) uint8_t aSoundStorage[sizeof(BSStringT<char>)];
        BSStringT<char>* pSound = ::new (aSoundStorage) BSStringT<char>("");
        // Emotion 1: happy.
        Interface::ShowMessage(
            acMessage,
            1,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
            pSound->String(),
            2.0f,
            false
        );
        pSound->~BSStringT<char>();
    }
    bool bWasAlive = !IsDead(false);
    ActorValueOwner* pValues = this;
    if (fHealthDamage > 0.0 || fFatigueDamage > 0.0 || apData->fTargetedLimbDamage != 0.0) {
        if (CombatController::GetShowVerboseMessages() && apData->pAggressor) {
            if (fHealthDamage > 0.0) {
                const char* pValueName = ActorValue::GetActorValueName(ActorValue::HEALTH);
                double fValue = pValues->GetActorFloatValue(ActorValue::HEALTH);
                const char* pTargetName = GetFullName();
                const char* pAggressorName = apData->pAggressor->GetFullName();
                Interface::PrintLine(
                    "%.20s hits %.20s for %.1f/%.1f %s damage!",
                    pAggressorName,
                    pTargetName,
                    static_cast<double>(fHealthDamage),
                    fValue,
                    pValueName
                );
            }
            if (fFatigueDamage > 0.0) {
                const char* pValueName = ActorValue::GetActorValueName(ActorValue::FATIGUE);
                double fValue = pValues->GetActorFloatValue(ActorValue::FATIGUE);
                const char* pTargetName = GetFullName();
                const char* pAggressorName = apData->pAggressor->GetFullName();
                Interface::PrintLine(
                    "%.20s hits %.20s for %.1f/%.1f %s damage!",
                    pAggressorName,
                    pTargetName,
                    static_cast<double>(fFatigueDamage),
                    fValue,
                    pValueName
                );
            }
        }
        if (apData->pAggressor == pPlayer) {
            Interface::SetEnemyActor(this);
        }
        if (apData->pAggressor == pPlayer) {
            if (bWasAlive && !IsPlayerTeammate()) {
                int32_t iSkill = ActorValue::UNARMED;
                if (!apData->pWeapon && (apData->uiFlags & HitData::HD_EXPLOSION)) {
                    iSkill = ActorValue::EXPLOSIVES;
                }
                TESForm* pChallengeBase = GetObjectReference();
                if (IsLeveledCreature()) {
                    pChallengeBase = GetExtra()->GetLevCreaTemplate();
                }
                if (this != pPlayer) {
                    int32_t iChallengeSkill;
                    if (apData->pWeapon) {
                        iChallengeSkill = apData->pWeapon->GetWeaponSkill();
                    }
                    else {
                        iChallengeSkill = iSkill;
                    }
                    int32_t iChallengeDamage = BSMAX(static_cast<int32_t>(fHealthDamage), 1);
                    TESChallengeManager::UpdateChallenge(
                        CHALLENGE_TYPE_DAMAGE,
                        iChallengeDamage,
                        pChallengeBase,
                        apData->pWeapon,
                        static_cast<uint16_t>(eLimbForChallenge[static_cast<int32_t>(apData->eDamageLimb) + 1]),
                        static_cast<uint16_t>(iChallengeSkill),
                        usWeaponType
                    );
                }
                TESChallengeManager::UpdateChallenge(
                    CHALLENGE_TYPE_HIT,
                    1,
                    pChallengeBase,
                    apData->pWeapon,
                    static_cast<uint16_t>(eLimbForChallenge[static_cast<int32_t>(apData->eDamageLimb) + 1]),
                    0,
                    usWeaponType
                );
            }
            if (bUnarmed) {
                BaseProcess* pPlayerProcess = pPlayer->GetCurrentAIProcess();
                BSAnimGroupSequence* pSequence = pPlayerProcess->GetAnimActionAnimSeq();
                if (pSequence) {
                    int32_t eType = pSequence->GetAnimGroup()->GetType();
                    if ((eType >= ANIM_GROUP_ATTACK_CUSTOM_1_POWER && eType <= ANIM_GROUP_ATTACK_CUSTOM_5_POWER) ||
                        eType == ANIM_GROUP_COUNTER) {
                        TESForm* pPowerAttackEffect = nullptr;
                        TESForm* pDefaultObject = nullptr;
                        switch (eType) {
                            case ANIM_GROUP_ATTACK_CUSTOM_1_POWER:
                                pDefaultObject = BGSDefaultObjectManager::GetDefaultObject(DO_UNARMED_EFFECT_FORWARD);
                                break;
                            case ANIM_GROUP_ATTACK_CUSTOM_2_POWER:
                                pDefaultObject = BGSDefaultObjectManager::GetDefaultObject(DO_UNARMED_EFFECT_BACKWARD);
                                break;
                            case ANIM_GROUP_ATTACK_CUSTOM_3_POWER:
                                pDefaultObject = BGSDefaultObjectManager::GetDefaultObject(DO_UNARMED_EFFECT_CROUCH);
                                break;
                            case ANIM_GROUP_ATTACK_CUSTOM_4_POWER:
                                pDefaultObject = BGSDefaultObjectManager::GetDefaultObject(DO_UNARMED_EFFECT_LEFT);
                                break;
                            case ANIM_GROUP_ATTACK_CUSTOM_5_POWER:
                                pDefaultObject = BGSDefaultObjectManager::GetDefaultObject(DO_UNARMED_EFFECT_RIGHT);
                                break;
                            case ANIM_GROUP_COUNTER:
                                pDefaultObject = BGSDefaultObjectManager::GetDefaultObject(DO_UNARMED_EFFECT_COUNTER);
                                break;
                            default: break;
                        }
                        if (pDefaultObject) {
                            pPowerAttackEffect = pDefaultObject;
                        }
                        if (pPowerAttackEffect) {
                            MagicCaster::InterruptCast();
                            MagicItem* pCastItem = static_cast<SpellItem*>(pPowerAttackEffect);
                            SetCurrentSpell(pCastItem);
                            MagicCaster::FindTargets(1.0f, nullptr, false, 1.0f, false);
                            MagicItem* pEffectItem = static_cast<SpellItem*>(pPowerAttackEffect);
                            MagicTarget::ProcessEffectsFromItem(pEffectItem);
                        }
                    }
                }
                else {
                    Warning(
                        ">>>> Trying to reference a missing animation! This usually resulted in a crash, but I "
                        "thought a Warning message would be more appropriate. Bad things may happen now, "
                        "though..."
                    );
                }
            }
        }
        bool bKilled = DoDamage(fHealthDamage, fFatigueDamage, apData->pAggressor);
        if (bWasAlive) {
            if (apData->IsFlagSet(HitData::HD_CRITICAL) &&
                apData->pCriticalEffect &&
                ((!GetEssential() && bKilled) || !apData->IsFlagSet(HitData::HD_CRITICAL_ON_DEATH))) {
                bool bBannedEffect = false;
                if (eSitSleepState) {
                    // 0x1768D7: the form list of critical effects banned on sitting actors.
                    BGSListForm* pBannedList = static_cast<BGSListForm*>(TESForm::GetFormByNumericID(0x1768D7));
                    if (apData->pCriticalEffect && pBannedList) {
                        TESForm* pCriticalEffect = apData->pCriticalEffect;
                        if (pBannedList->GetFormList()->IsInList(pCriticalEffect)) {
                            bBannedEffect = true;
                        }
                    }
                }
                if (!bDisableAllGore.Bool() && !bBannedEffect) {
                    MagicCaster::InterruptCast();
                    MagicItem* pCastItem = apData->pCriticalEffect;
                    SetCurrentSpell(pCastItem);
                    MagicCaster::FindTargets(1.0f, nullptr, false, 1.0f, false);
                    MagicItem* pEffectItem = apData->pCriticalEffect;
                    MagicTarget::ProcessEffectsFromItem(pEffectItem);
                }
            }
            if (apData->pAggressor) {
                TESBoundObject* pPlayerWeapon = nullptr;
                if (pPlayer && pPlayer->GetCurrentAIProcess() && pPlayer->GetCurrentAIProcess()->GetCurrentWeapon()) {
                    pPlayerWeapon = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon()->GetItemChangeObject();
                }
                if (pPlayerWeapon &&
                    static_cast<TESObjectWEAP*>(pPlayerWeapon)->GetWeaponSkill() == ActorValue::ENERGY_WEAPONS &&
                    bKilled) {
                    float fMeltdown = 0.0f;
                    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_MELTDOWN, apData->pAggressor, &fMeltdown);
                    if (fMeltdown > 0.0 && GetCurrentAIProcess()) {
                        BGSExplosion* pMeltdown = static_cast<BGSExplosion*>(
                            BGSDefaultObjectManager::GetDefaultObject(DO_MELTDOWN_EXPLOSION)
                        );
                        if (pMeltdown) {
                            NiMatrix3 kRotate = Get3D()->GetWorldTransform().m_Rotate;
                            NiPoint3 kOrigin = Get3D()->GetWorldTranslate();
                            TESObjectCELL* pCell = GetParentCell();
                            Explosion* pExplosion = Explosion::SpawnExplosion(
                                pMeltdown,
                                apData->pAggressor,
                                nullptr,
                                pCell,
                                kOrigin,
                                kRotate
                            );
                            if (pExplosion) {
                                NiPoint3 kUp(0.0f, 0.0f, 1.0f);
                                pExplosion->SetClosestPointNormal(kUp);
                                BaseProcess* pAggressorProcess = apData->pAggressor->GetCurrentAIProcess();
                                if (pAggressorProcess->GetCurrentWeapon()) {
                                    BaseProcess* pWeaponProcess = apData->pAggressor->GetCurrentAIProcess();
                                    pExplosion->SetWeaponSource(
                                        static_cast<TESObjectWEAP*>(
                                            pWeaponProcess->GetCurrentWeapon()->GetItemChangeObject()
                                        )
                                    );
                                }
                            }
                        }
                    }
                }
            }
        }
        if (apData->pVATSCommand &&
            (apData->pVATSCommand->eAction == ActionPoints::ACTION_PLAYER_SPECIAL_WEAPON_ATTACK ||
             apData->pVATSCommand->eAction == ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK) &&
            apData->pWeapon &&
            apData->pWeapon->GetVATSSpecialAttackEffect()) {
            MagicCaster::InterruptCast();
            MagicItem* pCastItem = apData->pWeapon->GetVATSSpecialAttackEffect();
            SetCurrentSpell(pCastItem);
            MagicCaster::FindTargets(1.0f, nullptr, false, 1.0f, false);
            MagicItem* pEffectItem = apData->pWeapon->GetVATSSpecialAttackEffect();
            MagicTarget::ProcessEffectsFromItem(pEffectItem);
        }
        if (apData->pAggressor) {
            float fKnockdownChance = 0.0f;
            BGSEntryPoint::HandleEntryPoint(
                BGSEntryPoint::KNOCKDOWN_CHANCE,
                apData->pAggressor,
                apData->pWeapon,
                &fKnockdownChance
            );
            if (fKnockdownChance > 0.0) {
                float fRoll = RandomFloat(0.0f, 1.0f);
                if (fKnockdownChance >= fRoll) {
                    float fKnockbackSpeed = CombatFormulas::GetKnockbackSpeed(pValues, 5);
                    BaseProcess* pProcess = GetCurrentAIProcess();
                    if (pProcess && pProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
                        bKnockedDown = true;
                        if (apData->pSourceRef) {
                            const NiPoint3* pFrom = &apData->pSourceRef->GetLocationOnReference();
                            pProcess->KnockExplosion(this, *pFrom, fKnockbackSpeed);
                        }
                        else {
                            const NiPoint3* pFrom = &apData->pAggressor->GetLocationOnReference();
                            pProcess->KnockExplosion(this, *pFrom, fKnockbackSpeed);
                        }
                    }
                }
            }
        }
        int32_t iLimbValue = -1;
        bool bStagger = false;
        bool bTorsoHit = false;
        if (apData->pSourceRef && apData->pSourceRef->IsExplosion()) {
            BGSBodyPartData* pBodyPartData = GetActorBase()->GetBodyPartData();
            if (pBodyPartData) {
                Explosion* pExplosion = static_cast<Explosion*>(apData->pSourceRef);
                BSSimpleList<Explosion::ExplodedLimb*>* pIter = pExplosion->GetAffectedLimbList(this);
                while (pIter && !pIter->IsEmpty()) {
                    Explosion::ExplodedLimb* pLimb = pIter->GetItem();
                    pIter = pIter->GetNext();
                    BGSBodyPart* pBodyPart = pBodyPartData->GetBodyPart(pLimb->eLimb);
                    if (pBodyPart) {
                        iLimbValue = pBodyPart->GetActorValue();
                        if (iLimbValue >= 0 && iLimbValue < ActorValue::ACTOR_VALUE_COUNT) {
                            double fPartScale = pBodyPart->GetHealthPercent() * 0.009999999776482582;
                            int32_t iBaseHealth = pValues->GetBaseActorValue(ActorValue::HEALTH);
                            float fLimbDamage = static_cast<float>(fHealthDamage / (iBaseHealth * fPartScale) * 100.0);
                            TESObjectWEAP* pHitWeapon = apData->pWeapon ? apData->pWeapon : pFists;
                            BGSEntryPoint::HandleEntryPoint(
                                BGSEntryPoint::ADJUST_LIMB_DAMAGE,
                                this,
                                apData->pAggressor,
                                pHitWeapon,
                                &fLimbDamage
                            );
                            ActorValue::Index eLimbValue = static_cast<ActorValue::Index>(iLimbValue);
                            float fValueChange = -fLimbDamage;
                            Actor* pDamager = apData->pAggressor;
                            DamageModActorValue(eLimbValue, fValueChange, pDamager);
                        }
                        iLimbValue = -1;
                    }
                }
            }
        }
        else {
            // The result of IsPc() is deliberately discarded.
            IsPc();
            int32_t eLimb = static_cast<int32_t>(apData->eDamageLimb);
            if (eLimb == BGSBodyPart::LIMB_WEAPON) {
                float fWeaponDamage = apData->fTargetedLimbDamage;
                DamageEquipment(pCurrentProcess->GetCurrentWeapon(), fWeaponDamage, false);
                if (!IsPc() && !GetEssential() && apData->IsFlagSet(HitData::HD_CRITICAL)) {
                    uint32_t uiRoll = Random() % 100;
                    if (uiRoll < iWeaponCriticalHitDropChance.UInt()) {
                        DropAddOnObjects();
                    }
                }
            }
            else if (eLimb != BGSBodyPart::LIMB_NONE) {
                if (eLimb == BGSBodyPart::LIMB_TORSO) {
                    bTorsoHit = true;
                }
                BGSBodyPartData* pBodyPartData = GetActorBase()->GetBodyPartData();
                BGSBodyPart* pBodyPart = nullptr;
                if (pBodyPartData) {
                    pBodyPart = pBodyPartData->GetBodyPart(apData->eDamageLimb);
                }
                if (pBodyPart) {
                    iLimbValue = pBodyPart->GetActorValue();
                    if (iLimbValue >= 0 && iLimbValue < ActorValue::ACTOR_VALUE_COUNT) {
                        float fValueBefore = pValues->GetActorFloatValue(static_cast<ActorValue::Index>(iLimbValue));
                        float fLimbDamage = apData->fTargetedLimbDamage;
                        if (cMeleeEffect == 2) {
                            fLimbDamage = fLimbDamage * fCrossSkillDamageMultiplier.Float();
                        }
                        TESObjectWEAP* pHitWeapon = apData->pWeapon ? apData->pWeapon : pFists;
                        BGSEntryPoint::HandleEntryPoint(
                            BGSEntryPoint::ADJUST_LIMB_DAMAGE,
                            this,
                            apData->pAggressor,
                            pHitWeapon,
                            &fLimbDamage
                        );
                        ActorValue::Index eLimbValue = static_cast<ActorValue::Index>(iLimbValue);
                        float fValueChange = -fLimbDamage;
                        Actor* pDamager = apData->pAggressor;
                        DamageModActorValue(eLimbValue, fValueChange, pDamager);
                        bCrippled = fValueBefore > 0.0 &&
                            pValues->GetActorFloatValue(static_cast<ActorValue::Index>(iLimbValue)) <= 0.0;
                        if (bTorsoHit && !bCrippled) {
                            if (pValues->GetActorFloatValue(static_cast<ActorValue::Index>(iLimbValue)) <= 0.0) {
                                uint32_t uiRoll = Random() % 100;
                                if (uiRoll <= iCombatCrippledTorsoHitStaggerChance.UInt()) {
                                    bStagger = true;
                                }
                            }
                        }
                        else if (bCrippled) {
                            bStagger = true;
                        }
                        if (pValues->GetActorValue(ActorValue::IGNORE_CRIPPLED_LIMBS)) {
                            bStagger = false;
                        }
                        if (cMeleeEffect == 1) {
                            bStagger = true;
                            bForceHitReaction = true;
                        }
                        ItemChange* pWeaponItem = pCurrentProcess->GetCurrentWeapon();
                        TESObjectWEAP* pCurrentWeapon = nullptr;
                        if (pWeaponItem) {
                            pCurrentWeapon = static_cast<TESObjectWEAP*>(pWeaponItem->GetItemChangeObject());
                        }
                        if (bStagger &&
                            pCurrentWeapon &&
                            !IsPc() &&
                            !GetEssential() &&
                            (apData->eDamageLimb == BGSBodyPart::LIMB_RIGHT_ARM_1 ||
                             apData->eDamageLimb == BGSBodyPart::LIMB_RIGHT_ARM_2 ||
                             (pCurrentWeapon->IsTwoHandedWeapon() &&
                              (apData->eDamageLimb == BGSBodyPart::LIMB_LEFT_ARM_1 ||
                               apData->eDamageLimb == BGSBodyPart::LIMB_LEFT_ARM_2)))) {
                            DropAddOnObjects();
                        }
                    }
                }
            }
        }
        if (Get3D() && GetParentCell() && GetParentCell()->GetAttached()) {
            TESForm* pKiller = nullptr;
            if (apData->pSourceRef) {
                if (apData->pSourceRef->IsActor()) {
                    pKiller = apData->pSourceRef->GetExtra()->GetLevCreaOriginalBase();
                }
                if (!pKiller) {
                    pKiller = apData->pSourceRef->GetObjectReference();
                }
            }
            bool bExplosionHit = false;
            if (apData->pSourceRef && apData->pSourceRef->IsExplosion()) {
                bExplosionHit = true;
            }
            if (bKilled && GetLifeState() != ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
                CAUSE_OF_DEATH eCauseOfDeath = CAUSE_OF_DEATH_HAND_TO_HAND;
                if (apData->pSourceRef) {
                    switch (apData->pSourceRef->GetFormType()) {
                        case WEAP_ID: eCauseOfDeath = CAUSE_OF_DEATH_BLUNT_WEAPON; break;
                        case ALCH_ID: eCauseOfDeath = CAUSE_OF_DEATH_POISON; break;
                        case PGRE_ID:
                        case EXPL_ID: eCauseOfDeath = CAUSE_OF_DEATH_EXPLOSION; break;
                        case DEBR_ID: eCauseOfDeath = CAUSE_OF_DEATH_OBJECT_IMPACT; break;
                        case PROJ_ID:
                        case PMIS_ID:
                        case PBEA_ID:
                        case PFLA_ID:
                        case PCBE_ID: eCauseOfDeath = CAUSE_OF_DEATH_GUN; break;
                        default: break;
                    }
                }
                if (bExplosionHit) {
                    eCauseOfDeath = CAUSE_OF_DEATH_EXPLOSION;
                }
                if (iLimbValue == -1 ||
                    pValues->GetActorFloatValue(static_cast<ActorValue::Index>(iLimbValue)) <= 0.0) {
                    DismemberLimbs(apData, pKiller, eCauseOfDeath);
                    if (apData->pAggressor == pPlayer && bWasAlive && !IsPlayerTeammate()) {
                        MiscStatManager::Increment(MiscStatManager::MS_DISMEMBERMENTS);
                    }
                }
            }
            if (bWasAlive && bKilled) {
                if (GetActorBaseSpeedMult() > 0.0 && apData->pAggressor) {
                    apData->pAggressor->DoWeaponKillForce(apData);
                }
            }
            else if (
                !IsKnockedOut() &&
                bPlayHitLocationIdles.Bool() &&
                static_cast<int32_t>(apData->eDamageLimb) != BGSBodyPart::LIMB_NONE &&
                bStagger &&
                pValues->GetActorValue(ActorValue::IGNORE_CRIPPLED_LIMBS) <= 0
            ) {
                TESIdleForm* pIdle = pIdleManager->GetIdleToPlay(this, apData->pSourceRef);
                Animation* pAnimation = GetAnimation();
                if (pIdle && pAnimation && !pAnimation->IsIdleNotLoaded(pIdle)) {
                    if (this == pPlayer) {
                        pAnimation
                            ->SpecialIdleAuto(pIdle, this, pIdle->GetIdleAnimGroupSection(), AnimIdle::PLAY_TYPE_AUTO);
                        pVATS.QuitVATSPlayback(false, false);
                    }
                    else {
                        BaseProcess* pProcess = GetCurrentAIProcess();
                        pProcess->SetCurrentProcessIdle(pIdle);
                        pProcess->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_PLAY_IDLE);
                    }
                }
            }
            bForceHitReaction = false;
            if (!bKilled) {
                bool bFatigueOut = false;
                if (fFatigueDamage > 0.0 && pValues->GetActorValue(ActorValue::FATIGUE) <= 0) {
                    bFatigueOut = true;
                }
                if (cMeleeEffect == 1) {
                    bMeleeKnockdown = true;
                }
                int32_t iHealth = pValues->GetActorValue(ActorValue::HEALTH);
                float fHealthRatio = static_cast<float>(fHealthDamage / static_cast<double>(iHealth));
                // The result of CheckAttackDisarm is deliberately ignored.
                if (apData->pAggressor) {
                    apData->pAggressor->CheckAttackDisarm(this, apData->eAttackSkill);
                }
                if ((!GetIsTalking() && !IsinDialogue() && !GetCurrentAIProcess()->GetGreetingFlag()) &&
                    (bFatigueOut ||
                     fCombatSpeakHitThreshold.Float() < fHealthRatio ||
                     RandomBool(fCombatSpeakHitChance.Float()))) {
                    bSpeak = true;
                }

                if (bFatigueOut && !IsKnockedOut() && apData->pAggressor) {
                    if (CombatUtilities::IsTargetWithinStrikeAngle(this, apData->pAggressor, nullptr)) {
                        bStaggeredDown = true;
                    }
                    if (CombatController::GetShowVerboseMessages()) {
                        const char* pAggressorName = apData->pAggressor->GetFullName();
                        const char* pTargetName = GetFullName();
                        Interface::PrintLine(
                            "%.20s is staggered down by %.20s's %.1f total point blow!",
                            pTargetName,
                            pAggressorName,
                            static_cast<double>(fTotalDamage)
                        );
                    }
                }
            }
        }
    }
    if (Get3D() && GetParentCell() && GetParentCell()->GetAttached()) {
        if (fPercentBlocked > 0.0) {
            bool bBlockAttack;
            if (apData->pSourceRef) {
                bBlockAttack = false;
            }
            else {
                bBlockAttack = CheckBlockStagger();
            }
            BlockHit(bBlockAttack);
            if (!bBlockAttack) {
                bool bDisarm = CheckBlockDisarm(apData->pAggressor);
                if (!apData->pSourceRef || bDisarm) {
                    apData->pAggressor->Stagger();
                }
            }
            if (this == pPlayer) {
                if (!pVATS.GetMode()) {
                    float fPain = fGetHitPainMult.Float();
                    ImageSpaceModifierInstanceForm::Trigger(TESImageSpaceModifier::GetGetHit(), fPain, nullptr);
                }
                if (!Interface::IsInMenuMode()) {
                    Controls* pControls = appMain->GetControls();
                    int32_t iTime = static_cast<int32_t>(fRumbleBlockTime.Float() * 1000.0);
                    float fRight = fRumbleBlockStrength.Float();
                    float fLeft = fRumbleBlockStrength.Float();
                    pControls->Rumble(fLeft, fRight, iTime, 0, CONSTANT, false, false);
                }
            }
            else if (apData->pAggressor == pPlayer && !apData->pSourceRef && !Interface::IsInMenuMode()) {
                Controls* pControls = appMain->GetControls();
                int32_t iTime = static_cast<int32_t>(fRumbleHitBlockedTime.Float() * 1000.0);
                float fRight = fRumbleHitBlockedStrength.Float();
                float fLeft = fRumbleHitBlockedStrength.Float();
                pControls->Rumble(fLeft, fRight, iTime, 0, CONSTANT, false, false);
            }
        }
        else {
            if (!bStaggeredDown && apData->pAggressor) {
                if (apData->pAggressor->GetCurrentAIProcess()) {
                    BaseProcess* pAggressorProcess = apData->pAggressor->GetCurrentAIProcess();
                    NiPoint3 kStopPos = apData->pAggressor->GetWeaponPosition(false);
                    const NiPoint3* pStartPos = pAggressorProcess->GetWeaponLastPos();
                    HitMe(apData->pAggressor, fHealthDamage, fPercentBlocked, *pStartPos, kStopPos);
                }
                if (GetActorBaseSpeedMult() > 0.0 &&
                    !GetActorBase()->GetNotPushable() &&
                    (!apData->pSourceRef || !apData->pSourceRef->IsProjectile())) {
                    float fKnockbackSpeed = CombatFormulas::GetKnockbackSpeed(
                        pValues,
                        static_cast<int32_t>(fTotalDamage)
                    );
                    const NiPoint3* pFrom = &apData->pAggressor->GetLocationOnReference();
                    NiPoint3 kDirection = GetLocationOnReference() - *pFrom;
                    NiPoint3 kVelocity;
                    if (fKnockbackForceMax.Float() < fKnockbackSpeed) {
                        fKnockbackSpeed = fKnockbackForceMax.Float();
                    }
                    kDirection.Unitize();
                    kVelocity = kDirection * fKnockbackSpeed;
                    bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
                    if (pController) {
                        pController->SetVelocityModifier(kVelocity, fKnockbackTime.Float());
                    }
                }
            }
            PlayHitSound(apData);
            bool bBoneLODClose = false;
            BaseProcess* pProcess = GetCurrentAIProcess();
            if (pProcess && pProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
                NiBSBoneLODController* pBoneLOD = static_cast<HighProcess*>(pProcess)->GetBoneLOD();
                bBoneLODClose = pBoneLOD && pBoneLOD->GetBoneLOD() <= 1;
            }
            if (!IsDead(false) &&
                bBoneLODClose &&
                !IsUnConscious() &&
                pRagdollController &&
                pRagdollController->GetRagdollAnimEnable() &&
                pRagdollController->QRagdollFeedbackEnable() &&
                GetCurrentAIProcess()->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                pRagdollController->SetRagdollFeedbackActive(true);
                if (pRagdollController->QRagdollFeedbackActive()) {
                    hkpRigidBody* pBody = nullptr;
                    alignas(16) bhkPickData kPick;
                    alignas(16) hkVector4 kFrom;
                    NI2HK(kFrom, apData->kHitLocation);
                    alignas(16) hkVector4 kDirection;
                    NI2HKNS(kDirection, apData->kHitDirection);
                    alignas(16) hkVector4 kTo;
                    kTo.setAdd4(kDirection, kFrom);
                    bhkWorld* pWorld = pRagdollController->GetWorld();
                    if (bMeleeKnockdown) {
                        pBody = pRagdollController->GetRigidBodyHead();
                    }
                    else {
                        CFilter kFilter(0);
                        kFilter.SetGroup(0);
                        kFilter.SetLayer(L_BIPED);
                        kPick.SetFilter(kFilter);
                        kFrom.setSub4(kFrom, kDirection);
                        kPick.SetFrom(kFrom);
                        kPick.SetTo(kTo);
                        if (pWorld && pWorld->PickObject(kPick)) {
                            pBody = kPick.GethkRigidBody();
                        }
                    }
                    if (pBody) {
                        bhkCollisionObject* pCollisionObject = bhkCollisionObject::Getbhk(pBody);
                        NiAVObject* pHitObject = pCollisionObject ? pCollisionObject->GetSceneGraphObject() : nullptr;
                        TESObjectREFR* pHitRef = pHitObject ? TESObjectREFR::FindReferenceFor3D(pHitObject) : nullptr;
                        if (!pHitRef || !pHitRef->IsProjectile()) {
                            kDirection.normalize3();
                            uint32_t uiImpulse = pRagdollController->QFeedbackImpulseWeak();
                            if (bMeleeKnockdown || (apData->pWeapon && apData->pWeapon->IsMeleeWeapon())) {
                                uiImpulse = pRagdollController->QFeedbackImpulseStrong();
                            }
                            alignas(16) hkSimdReal kImpulse(static_cast<float>(static_cast<int64_t>(uiImpulse)));
                            kDirection.mul4(kImpulse);
                            (**pWorld)->markForWrite();
                            pBody->applyLinearImpulse(kDirection);
                            (**pWorld)->unmarkForWrite();
                        }
                    }
                }
            }
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                TaskQueueInterface::GetInstance()->QueueActorCreateBlood(this, apData);
            }
            else {
                CreateBlood(fHealthDamage, apData);
                if (apData->pAggressor) {
                    apData->pAggressor->CreateBloodOnWeapon(fHealthDamage, apData, this);
                }
            }
            if (this == pPlayer) {
                if (apData->pAggressor && !pVATS.GetMode()) {
                    NiNode* pAggressor3D = apData->pAggressor->Get3D();
                    float fPain = fGetHitPainMult.Float();
                    ImageSpaceModifierInstanceForm::Trigger(TESImageSpaceModifier::GetGetHit(), fPain, pAggressor3D);
                }
                if (!Interface::IsInMenuMode() && !pVATS.IsInVats()) {
                    Controls* pControls = appMain->GetControls();
                    int32_t iTime = static_cast<int32_t>(fRumbleStruckTime.Float() * 1000.0);
                    float fRight = fRumbleStruckStrength.Float();
                    float fLeft = fRumbleStruckStrength.Float();
                    pControls->Rumble(fLeft, fRight, iTime, 0, CONSTANT, false, false);
                }
            }
            else if (apData->pAggressor == pPlayer && !apData->pSourceRef && !Interface::IsInMenuMode()) {
                Controls* pControls = appMain->GetControls();
                if (!pVATS.IsInVats()) {
                    int32_t iTime = static_cast<int32_t>(fRumbleHitTime.Float() * 1000.0);
                    float fRight = fRumbleHitStrength.Float();
                    float fLeft = fRumbleHitStrength.Float();
                    pControls->Rumble(fLeft, fRight, iTime, 0, CONSTANT, false, false);
                }
                else if (pVATS.GetFirstAttack()) {
                    double fTimeScale = BSTimer::QGlobalTimeMultiplier() * 1000.0;
                    int32_t iTime = static_cast<int32_t>(fRumbleHitTime.Float() * fTimeScale);
                    float fRight = fRumbleHitStrength.Float();
                    float fLeft = fRumbleHitStrength.Float();
                    pControls->Rumble(fLeft, fRight, iTime, 0, CONSTANT, false, false);
                    pVATS.SetFirstAttack(false);
                }
                else {
                    double fTimeScale = BSTimer::QGlobalTimeMultiplier() * 1000.0;
                    int32_t iTime = static_cast<int32_t>(fRumbleHitTime.Float() * fTimeScale);
                    float fRight = fRumbleHitStrength.Float();
                    float fLeft = fRumbleHitStrength.Float();
                    pControls->Rumble(fLeft, fRight, iTime, 0, CONSTANT, false, false);
                }
            }
        }
        if (GetCurrentAIProcess()) {
            BaseProcess* pProcess = GetCurrentAIProcess();
            BSSimpleList<ActiveEffect*>* pEffects = static_cast<MagicTarget*>(this)->GetActiveEffectList();
            if (pProcess->ShouldCheckFlare(pEffects, MagicSystem::FLARE_WEAPON)) {
                ReferenceProcessLists.FlareHitEffect(this, MagicSystem::FLARE_WEAPON);
            }
        }
    }
    if (!IsDead(false)) {
        if (IsPlayerTeammate()) {
            if (bKnockedDown) {
                FollowerBarks::TriggerFollowerBark(this, FollowerBarks::BARK_KNOCKED_BACK);
            }
            else if (bCrippled) {
                FollowerBarks::TriggerFollowerBark(this, FollowerBarks::BARK_CRIPPLED);
                FollowerBarks::TriggerFollowerBark(this, FollowerBarks::BARK_CRIPPLED_REMINDER);
            }
        }
        TESForm* pChallengeBase = GetObjectReference();
        if (IsLeveledCreature()) {
            pChallengeBase = GetExtra()->GetLevCreaTemplate();
        }
        if (apData->pAggressor == pPlayer && !IsPlayerTeammate()) {
            if (bCrippled) {
                TESChallengeManager::UpdateChallenge(
                    CHALLENGE_TYPE_HIT,
                    1,
                    pChallengeBase,
                    apData->pWeapon,
                    static_cast<uint16_t>(eLimbForChallenge[static_cast<int32_t>(apData->eDamageLimb) + 1]),
                    3,
                    usWeaponType
                );
            }
            if (bKnockedDown) {
                TESChallengeManager::UpdateChallenge(
                    CHALLENGE_TYPE_HIT,
                    1,
                    pChallengeBase,
                    apData->pWeapon,
                    static_cast<uint16_t>(eLimbForChallenge[static_cast<int32_t>(apData->eDamageLimb) + 1]),
                    2,
                    usWeaponType
                );
            }
        }
    }
    else if (bWasAlive && IsPlayerTeammate() && !GetEssential()) {
        FollowerBarks::TriggerFollowerBark(this, FollowerBarks::BARK_DIED);
    }
    if (!IsDead(false) && apData->fArmorDamage > 0.0 && IsPc()) {
        bool bHeadHit = apData->eDamageLimb == BGSBodyPart::LIMB_HEAD_1 ||
            apData->eDamageLimb == BGSBodyPart::LIMB_HEAD_2;
        ItemChange* pArmor = GetRandomWornArmor(bHeadHit);
        if (pArmor) {
            DamageEquipment(pArmor, apData->fArmorDamage, false);
            delete pArmor;
        }
    }
    Actor* pAttacker = apData->pAggressor;
    if (!pAttacker && apData->pSourceRef) {
        ActorCause* pCause = apData->pSourceRef->GetActorCause();
        if (pCause && pCause->IsValid()) {
            pAttacker = pCause->GetActor();
        }
    }
    CombatController* pCombatController = GetCombatController();
    if (pAttacker) {
        AttackedBy(pAttacker, nullptr);
    }
    if (GetIsTalking() || IsinDialogue() || uiDialogHitSoundCooldownNext > GetTickCount()) {
        bSpeak = false;
    }
    if (uiDialogHitSoundCooldownNext < GetTickCount()) {
        uint32_t uiNow = GetTickCount();
        int32_t iMaxCooldown = static_cast<int32_t>(fDialogHitSoundCooldownMax.Float() * 1000.0);
        int32_t iMinCooldown = static_cast<int32_t>(fDialogHitSoundCooldownMin.Float() * 1000.0);
        uiDialogHitSoundCooldownNext = uiNow + RandomInt(iMinCooldown, iMaxCooldown);
    }
    if (!pCombatController) {
        pCombatController = GetCombatController();
        if (pCombatController) {
            pCombatController->DamagedByAttacker(pAttacker, fHealthDamage);
        }
    }
    // Topic 2: the combat hit topic.
    if (bSpeak) {
        pCombatDialogueManager->StartDialogue(this, apData->pAggressor, DT_COMBAT, 2, true, pCombatController);
    }
    ClearLastHitData();
}

void Actor::Kill(Actor* apAttacker, float afDamage) {
    uint32_t iplayerCombatantLostCount;
    if (!IsDead(false) && (!this->IsChild(false) || this == pPlayer)) {
        BSSoundHandle hweapLoop;
        this->m_Extra.GetWeaponAttackSound(hweapLoop);
        if (hweapLoop.IsPlaying()) {
            hweapLoop.FadeOutAndRelease(500u);
        }
        else {
            hweapLoop.Release();
        }
        this->m_Extra.SetWeaponAttackSound(hweapLoop);
        alignas(16) BSSoundHandle hwpnIdle;
        this->m_Extra.GetWeaponIdleSound(hwpnIdle);
        if (hwpnIdle.IsPlaying()) {
            hwpnIdle.FadeOutAndRelease(500u);
        }
        else {
            hwpnIdle.Release();
        }
        this->m_Extra.SetWeaponIdleSound(hwpnIdle);
        BSSoundHandle awakeHandle;
        this->m_Extra.GetCreatureAwakeSound(awakeHandle);
        awakeHandle.Stop();
        awakeHandle.Release();
        this->m_Extra.SetCreatureAwakeSound(awakeHandle);
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            float* fdamage = static_cast<float*>(ourMalloc(sizeof(float)));
            *fdamage = afDamage;
            this->pMyKiller = apAttacker;
            TaskQueueInterface::GetInstance()->QueueActorKill(this, fdamage, nullptr);
            return;
        }
        if (apAttacker && apAttacker->IsPc() && !this->IsPc()) {
            TESObjectWEAP* pWeap;
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ADD_LEVELED_LIST_ON_DEATH, pPlayer, this, this);
            BaseProcess* pProcess = apAttacker->pCurrentProcess;
            ItemChange* pItem = pProcess->GetCurrentWeapon();
            if (pItem) {
                pWeap = static_cast<TESObjectWEAP*>(pItem->pContainerObj);
            }
            else {
                pWeap = nullptr;
            }
            TESActorBase* pBaseForm = static_cast<TESActorBase*>(this->GetObjectReference());
            if (this->IsLeveledCreature()) {
                pBaseForm = this->m_Extra.GetLevCreaTemplate();
            }
            char cType = 0;
            if (this->IsNpc()) {
                cType = 0;
            }
            else {
                TESCreature* pcreature = dynamic_cast<TESCreature*>(this->GetActorBase());
                cType = pcreature->GetCreatureType() + 1;
            }
            if (!this->bPlayerTeammate) {
                int32_t iweaponSkill;
                int32_t iweaponType;
                if (pWeap) {
                    iweaponType = static_cast<uint16_t>(pWeap->GetWeaponType()) + 1;
                }
                else {
                    iweaponType = 1;
                }
                if (pWeap) {
                    iweaponSkill = static_cast<uint16_t>(pWeap->GetWeaponSkill());
                }
                else {
                    iweaponSkill = 45;
                }
                TESChallengeManager::UpdateChallenge(
                    CHALLENGE_TYPE_KILL_SPECIFIC,
                    1,
                    pBaseForm,
                    pWeap,
                    0,
                    iweaponSkill,
                    iweaponType
                );
                if (pWeap) {
                    iweaponType = static_cast<uint16_t>(pWeap->GetWeaponType()) + 1;
                }
                else {
                    iweaponType = 1;
                }
                if (pWeap) {
                    iweaponSkill = static_cast<uint16_t>(pWeap->GetWeaponSkill());
                }
                else {
                    iweaponSkill = 45;
                }
                TESChallengeManager::UpdateChallenge(
                    CHALLENGE_TYPE_KILL_LIST,
                    1,
                    pBaseForm,
                    pWeap,
                    0,
                    iweaponSkill,
                    iweaponType
                );
                if (pWeap) {
                    iweaponType = static_cast<uint16_t>(pWeap->GetWeaponType()) + 1;
                }
                else {
                    iweaponType = 1;
                }
                TESChallengeManager::UpdateChallenge(
                    CHALLENGE_TYPE_KILL_CATEGORY,
                    1,
                    nullptr,
                    pWeap,
                    0,
                    static_cast<uint16_t>(cType),
                    static_cast<uint16_t>(iweaponType)
                );
            }
            if (pVATS.eMode == VATS::VATS_PLAYBACK) {
                pVATS.IncrementKillCount();
            }
        }

        this->bMurderAlarm = false;
        bool bIsAggressor = false;
        if (this->pCurrentProcess) {
            bIsAggressor = this->pCurrentProcess->GetIsAggressor();
        }
        this->StopCurrentDialogue();
        this->StopCombat(nullptr);
        if (this->pCurrentProcess) {
            this->pCurrentProcess->ClearMuzzleFlash();
            this->pCurrentProcess->RemoveAllQueuedItems();
        }
        if (this->IsParalyzed()) {
            NiAVObject* p3dObj = this->Get3D();
            bhkRigidBody::SetFixed(p3dObj, 0, 1);
        }
        if (this->pRagdollController) {
            this->pRagdollController->DisableRagdollAnim(true);
        }
        if (bEssentialTakeNoDamage.Bool() && this->GetEssential()) {
            if (this->IsUnConscious() || this->IsRestrained()) {
                int32_t ibaseHealth = this->GetBaseActorValue(ActorValue::HEALTH);
                float endurance = static_cast<float>(ibaseHealth);
                float fadd = fEssentialHealthPercentReGain.Float() * endurance;
                float currentvalue = this->GetActorFloatValue(ActorValue::HEALTH);
                fadd = fadd - currentvalue;
                this->DamageModActorValue(ActorValue::HEALTH, fadd, nullptr);
                return;
            }
            TESPackage* ppack = this->GetCurrentEditorPackage();
            this->m_Extra.RemovePlayerCrimeListExtra(this);
            this->pCurrentProcess->SetEssentialDownTimer(fEssentialDeathTime.Float());
            this->pCurrentProcess->SetBreathTimer(20.0f);
            if (ppack && ppack->GetPackType() != PACKAGE_TYPE_DIALOGUE) {
                this->EndInterruptPackage(false);
            }

            this->ClearInCombat(true);
            if (this->pCurrentProcess) {
                if (this->pCurrentProcess->GetCurrentFurniture()) {
                    BaseProcess* pprocess = this->pCurrentProcess;
                    uint32_t uiFurnitureIndex = pprocess->GetCurrentFurnitureIndex();
                    pprocess->GetCurrentFurniture()->SetMarkerUsed(uiFurnitureIndex, false);
                }
            }
            this->InterruptCast();
            this->SetLifeState(ACTOR_LIFE_STATE_ESSENTIAL_DOWN);
            this->SetHavokWeapon();
            this->RestoreFullHealthAndConditions();
            if (this->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
                alignas(16) char acMessage[204];
                const char* pcDownText = sEssentialCharacterDown.String();
                std::sprintf(acMessage, "%s %s", this->GetFullName(), pcDownText);
                Interface::ShowMessage(
                    acMessage,
                    0,
                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
                    nullptr,
                    2.0f,
                    false
                );
            }
        }
        else {
            ReferenceProcessLists.RemoveAliveActor(this);
            if (this->pCurrentProcess) {
                this->pCurrentProcess->SetGreetingFlag(false);
            }
            if (apAttacker && apAttacker->pCurrentProcess) {
                if (apAttacker->pCurrentProcess->GetCommandingActor()) {
                    apAttacker = apAttacker->pCurrentProcess->GetCommandingActor();
                }
            }

            if (apAttacker && apAttacker->IsNpc()) {
                if (!this->GetCombatController() || this->GetCombatController()->IsActoraCombatTarget(apAttacker)) {
                    this->bMurderAlarm = !bIsAggressor;
                    if (apAttacker == pPlayer) {
                        bool bcrimecaused = false;
                        FIGHT_REACTION ereaction = this->GetFactionFightReaction(pPlayer, bcrimecaused);
                        if ((ereaction == FIGHT_REACTION_FRIEND || ereaction == FIGHT_REACTION_ALLY) &&
                            this->IsIgnoreFriendlyHits()) {
                            this->bMurderAlarm = false;
                        }
                    }
                }
            }

            if (apAttacker == pPlayer && !this->IsDying()) {
                if (this->IsNpc()) {
                    MiscStatManager::Increment(MiscStatManager::MS_HUMANS_KILLED);
                    if (!this->IsEvil() && this->bMurderAlarm) {
                        pPlayer->SetIsAMurderer();
                    }
                }
                else {
                    MiscStatManager::Increment(MiscStatManager::MS_CREATURES_KILLED);
                }
                MiscStatManager::Increment(MiscStatManager::MS_TOTAL_THINGS_KILLED);
                this->GetActorBase();
                ActorValueOwner* pbaseOwner = static_cast<ActorValueOwner*>(this->GetActorBase());
                float fkarma = pbaseOwner->GetActorFloatValue(ActorValue::KARMA);
                TESActorBaseData::ALIGNMENT ealign = TESActorBaseData::GetAlignmentForKarma(fkarma);
                if (!bIsAggressor && this->HasFactionThatCaresAboutCrime()) {
                    float fvalue = fKarmaModMurderingNonEvilNPC.Float();
                    if (this->IsCreature()) {
                        fvalue = fKarmaModMurderingNonEvilCreature.Float();
                    }
                    switch (ealign) {
                        case TESActorBaseData::ALIGNMENT_GOOD: {
                            fvalue = fKarmaModMurderingGoodNPC.Float();
                            break;
                        }
                        case TESActorBaseData::ALIGNMENT_EVIL: {
                            fvalue = fKarmaModKillingEvilActor.Float();
                            break;
                        }
                        case TESActorBaseData::ALIGNMENT_VERY_GOOD: {
                            fvalue = fKarmaModMurderingVeryGoodNPC.Float();
                            break;
                        }
                        case TESActorBaseData::ALIGNMENT_VERY_EVIL: {
                            fvalue = fKarmaModKillingVeryEvilActor.Float();
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    if (fvalue != 0.0) {
                        pPlayer->RewardKarma(static_cast<int32_t>(fvalue));
                    }
                }
            }
            if (apAttacker == pPlayer &&
                pPlayer->IsSneaking() &&
                !pPlayer->IsDetectedByCompassTargetExceptActor(nullptr)) {
                uint32_t uisoundLevel;
                TESObjectWEAP* pWeap = nullptr;
                if (pPlayer->pCurrentProcess) {
                    if (pPlayer->pCurrentProcess->GetCurrentWeapon()) {
                        pWeap = static_cast<TESObjectWEAP*>(
                            pPlayer->pCurrentProcess->GetCurrentWeapon()->pContainerObj
                        );
                    }
                }
                if (!pWeap ||
                    (pWeap->IsMeleeWeapon() ||
                     ((uisoundLevel = pWeap->GetModifiedSoundLevel(false, false),
                       uisoundLevel == iSoundLevelSilent.Int()) ||
                      pPlayer->pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(11)))) {
                    this->bMurderAlarm = false;
                }
            }
            bool brewardTeammateKill = true;
            if (this->bPlayerTeammate) {
                brewardTeammateKill = apAttacker && apAttacker->IsPc();
            }
            if (!this->IsPc() && !this->IsDying() && brewardTeammateKill) {
                float fplayerDamage;
                if (this->pCurrentProcess) {
                    fplayerDamage = this->pCurrentProcess->GetTrackedDamage();
                }
                else {
                    fplayerDamage = 0.0f;
                }
                if (fplayerDamage > 0.0) {
                    int32_t ioriginalHealth = this->GetBaseActorValue(ActorValue::HEALTH);
                    float foriginalHealth = static_cast<float>(ioriginalHealth);
                    if (ExperiencePoints::CheckDamageThreshold(fplayerDamage, foriginalHealth)) {
                        float fexperience;
                        int32_t ilevel = this->GetLevel();
                        bool bcreature = this->IsCreature();
                        fexperience = std::bit_cast<float>(static_cast<uint32_t>(ExperiencePoints::GetExperiencePoints(
                            static_cast<ExperiencePoints::Activity>(!bcreature),
                            ilevel
                        )));
                        fexperience = static_cast<float>(std::bit_cast<int32_t>(fexperience));
                        GamePlayFormulas::DifficultyLevel eDifficulty = pPlayer->GetGameDifficulty();
                        fexperience = GamePlayFormulas::AdjustExperiencePoints(fexperience, eDifficulty);
                        double dexperience = std::floorf(fexperience);
                        pPlayer->RewardExperience(static_cast<int32_t>(dexperience));
                    }
                }
            }
            if (this->pCurrentProcess) {
                if (this->pCurrentProcess->GetCurrentFurniture()) {
                    BaseProcess* pprocess = this->pCurrentProcess;
                    uint32_t uiFurnitureIndex = pprocess->GetCurrentFurnitureIndex();
                    pprocess->GetCurrentFurniture()->SetMarkerUsed(uiFurnitureIndex, false);
                }
            }
            if (this != pPlayer) {
                this->AddDeathItems();
                this->pMyKiller = apAttacker;
            }
            uint16_t sanimGroup = this->GetAnimGroup(ANIM_GROUP_DEATH, nullptr, false, nullptr);
            if (TESAnimGroup::GetType(sanimGroup) == ANIM_GROUP_DEATH) {
                this->pCurrentProcess->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_DIE);
            }
            if (this->pCurrentProcess) {
                this->pCurrentProcess->ClearAllHeadTrackTargets();
                if (!this->pCurrentProcess->eLevel) {
                    bool bskipSound = !this->IsPc() && Main::GetSceneIsComplex();
                    if (bskipSound) {
                        float fdist = this->GetDistanceFromReference(pPlayer, false, false);
                        if (fDeathSoundMaxDistance.Float() >= static_cast<double>(fdist)) {
                            bskipSound = false;
                        }
                    }
                    if (!bskipSound) {
                        TESSound* psound = nullptr;
                        if (this->IsCreature()) {
                            TESCreature* pcreatureRef = nullptr;
                            if (this->IsCreature()) {
                                pcreatureRef = static_cast<TESCreature*>(this->GetObjectReference());
                            }
                            psound = pcreatureRef ? pcreatureRef->PickCreatureSound(CREA_SOUND_DEATH) : nullptr;
                        }
                        if (psound) {
                            uint32_t uiSoundID = psound->iFormID;
                            BSSoundHandle soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 258u);
                            NiPoint3 loc = this->GetLocationOnReference();
                            soundHand.SetPosition(loc.x, loc.y, loc.z);
                            soundHand.SetObjectToFollow(this->Get3D());
                            soundHand.Play(false);
                        }
                        else {
                            pCombatDialogueManager->StartDialogue(this, apAttacker, DT_COMBAT, 6, true, nullptr);
                        }
                    }
                }
            }
            this->SetLifeState(ACTOR_LIFE_STATE_DYING);
            this->SetHavokWeapon();
            if (this == pPlayer) {
                FalloutRadio::PipboyRadioEnable(false);
                FalloutAudioMedia::MediaFadeOut(1000, MEDIA_STANDBY);
                BSAudio::QInstance().GetSoundHandleByName("MUSDeath", 2305u).Play(false);
                pVATS.QuitVATSPlayback(false, false);
                if (this->pCurrentProcess) {
                    this->pCurrentProcess->SetAutomaticFireAtLeastOnce(false);
                }
                if (Interface::IsTopMenuID(Interface::eSurgeryMenu)) {
                    SurgeryMenu::Close();
                }
            }
            BSFaceGenAnimationData* panimdata = this->GetFaceAnimationData();
            if (panimdata) {
                panimdata->SetDead(true, false);
            }
            if (this == pPlayer &&
                (pVATS.eMode != VATS::VATS_PLAYBACK ||
                 (!pVATS.GetCurrentAction() ||
                  pVATS.GetCurrentAction()->eAction != ActionPoints::ACTION_PLAYER_VATS_DEATH))) {
                pVATS.InitPlayerDeath();
            }
            BaseProcess* pprocess = this->pCurrentProcess;
            if (pprocess && !pprocess->eLevel) {
                bool bdetected = static_cast<MiddleHighProcess*>(pprocess)->MiddleHighProcess::GetDetectLifeDetected();
                if (bdetected) {
                    bool bstillDetected = DetectLifeEffect::CheckActorDetected(this, bdetected);
                    static_cast<MiddleHighProcess*>(pprocess)->MiddleHighProcess::SetDetectLifeDetected(bstillDetected);
                }
            }

            if (this->pCurrentProcess) {
                this->pCurrentProcess->ComputeLastTimeProcessed();
                BaseProcess* pdeathProcess = this->pCurrentProcess;
                int64_t ihoursPassed = appCalendar.GetHoursPassed();
                pdeathProcess->SetDeathTime(static_cast<float>(ihoursPassed));
            }
            TESActorBase* pdeadbase = this->m_Extra.GetLevCreaOriginalBase();
            if (!pdeadbase) {
                pdeadbase = this->GetActorBase();
            }
            TES::pInstance->AddDeadCount(pdeadbase, 1);
            if (this->IsTresPassing()) {
                this->m_Extra.RemoveTrespassPackage();
            }
            NiAVObject* p3dObj = this->TESObjectREFR::Get3D();
            alignas(16) hkVector4 charVelocity(0.0f, 0.0f, 0.0f, 0.0f);
            if (this->GetCharController()) {
                NiPoint3 vel;
                this->GetCharController()->GetLinearVelocity(vel);
                NI2HK(charVelocity, vel);
            }
            if (this == pPlayer) {
                this->SetChaseBip(true);
            }
            else {
                this->DetachCharController();
            }
            if (this->pCurrentProcess && (!this->pCurrentProcess->eLevel && p3dObj)) {
                this->pCurrentProcess->SetGreetingTimer(fDyingTimer.Float());
                if (this->pCurrentProcess->GetKnockState() &&
                    this->pCurrentProcess->GetKnockState() != KNOCK_STATE_GET_UP) {
                    if (this->IsParalyzed()) {
                        hkVector4 velocity;
                        NiMatrix3 rot;
                        bhkRigidBody::SetFixed(p3dObj, 0, 1);
                        float fheading = this->GetHeading(false);
                        rot.MakeZRotation(fheading);
                        alignas(16) NiPoint3 direction(0.0f, 5.0f, 0.0f);
                        direction = rot * direction;
                        NI2HK(velocity, direction);
                        TESHavokUtilities::AddVelocity(p3dObj, velocity, false);
                    }
                }
                else if (!this->GetTurretBehavior()) {
                    NiPoint3 direction = NiPoint3::ZERO;
                    if (charVelocity.length3() == 0.0) {
                        NiMatrix3 rot;
                        float fheading = this->GetHeading(false);
                        rot.MakeZRotation(fheading);
                        direction = rot * NiPoint3::UNIT_Y;
                    }
                    else {
                        HK2NI(direction, charVelocity);
                        direction.Unitize();
                    }
                    if (apAttacker == pPlayer && this != pPlayer && this->GetCharController()) {
                        NiPoint3 pPlayerPos = NiPoint3::ZERO;
                        NiPoint3 pMyPos = NiPoint3::ZERO;
                        pPlayer->GetCharController()->GetPosition(pPlayerPos);
                        this->GetCharController()->GetPosition(pMyPos);
                        direction = pMyPos - pPlayerPos;
                        direction.Unitize();
                    }

                    bhkWorld::SetMotion(p3dObj, hkpMotion::MOTION_DYNAMIC, true, true, true);
                    alignas(16) NiUpdateData update(0.0f, false, false);
                    p3dObj->Update(update);
                    direction *= fDeathForceForceMin.Float();
                    TESHavokUtilities::AddVelocity(p3dObj, direction, false);
                    this->AddChange(CHANGE_REFR_HAVOK_MOVE);
                }
                uint32_t uicombatantCount = pCombatManager->GetCombatantCount(nullptr, nullptr);
                if ((uicombatantCount <= iMaxHiPerfCombatCount.Int() || !this->HasRagDoll()) && this->pCurrentProcess) {
                    if (!this->pCurrentProcess->GetCommandingActor()) {
                        uint32_t uiRoll = Random();
                        if (uiRoll % 100 < iDeathDropWeaponChance.Int() || !this->HasRagDoll()) {
                            this->DropAddOnObjects();
                        }
                    }
                }
            }
            else {
                this->DoDeathStuff();
                this->RunScript();
                this->bDeadFlag = true;
            }
            if (CombatController::GetShowVerboseMessages()) {
                Interface::PrintLine("%.20s is dead!", this->GetFullName());
            }
        }
        if (this->pPenetrationDetection) {
            this->pPenetrationDetection->Clear();
            CFilter kFilter;
            this->GetCollisionFilter(&kFilter.iFilter);
            uint32_t uiGroup = kFilter.GetGroup();
            NiNode* p3D = this->Get3D();
            this->pPenetrationDetection->Initialize(p3D, uiGroup);
        }
        if (bTrackAllDeaths.Bool()) {
            if (apAttacker) {
                int32_t iattackerID = apAttacker->iFormID;
                const char* pcAttackerName = apAttacker->GetActorBase()->GetFullName();
                int32_t ivictimID = this->iFormID;
                InitializationFlow(
                    "'%s' (%08X) was killed by '%s' (%08X).",
                    this->GetActorBase()->GetFullName(),
                    ivictimID,
                    pcAttackerName,
                    iattackerID
                );
            }
            else {
                int32_t ivictimID = this->iFormID;
                InitializationFlow(
                    "'%s' (%08X) has died with no attacker.",
                    this->GetActorBase()->GetFullName(),
                    ivictimID
                );
            }
        }
        if (bTrackProgress && this == pPlayer) {
            this->pMyKiller = apAttacker;
            pPlayer->ExportProgressData(PlayerCharacter::DEATH);
            this->pMyKiller = nullptr;
        }
        iplayerCombatantLostCount = 0;
        if (apAttacker == pPlayer || (apAttacker && apAttacker->bPlayerTeammate)) {
            uint32_t iplayerCombatantCount = pCombatManager->GetCombatantCount(pPlayer, &iplayerCombatantLostCount);
            if (iplayerCombatantCount && pPlayer->IsDetectedByCompassTargetExceptActor(this)) {
                this->AddFactionMinorCrime(1, true);
            }

            if (!pVATS.eMode && !iplayerCombatantCount) {
                pPlayer->StartKillcamForActor(this, fKillCameraTime.Float(), true, -1);
            }
        }
        if (pPlayer->GetCateyeActive()) {
            TESEffectShader* pEffectData = pDataHandler->pCateyeMobileShader;
            if (pEffectData) {
                ReferenceProcessLists.FinishMagicShaderHitEffect(this, pEffectData);
            }
        }
        if (this->pContinuousBeamPersistant) {
            this->pContinuousBeamPersistant->SetKill(this);
        }
    }
}

ACTOR_LIFE_STATE Actor::GetLifeState() {
    return eLifeState;
}

bool Actor::IsUnConscious() {
    return GetLifeState() == ACTOR_LIFE_STATE_UNCONSCIOUS;
}

bool Actor::IsRestrained() {
    return GetLifeState() == ACTOR_LIFE_STATE_RESTRAINED;
}

bool Actor::IsWaiting() {
    TESPackage* pPackage = GetCurrentPackage();
    if (pPackage && pPackage->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE) {
        uint32_t uiProcedureType = pPackage->ePROCEDURE_TYPE;
        int32_t iIndex = pCurrentProcess->GetProcedureIndexRunning();
        if (pPackage->GetWaiting() || Procedure::Action[uiProcedureType][iIndex] == PROCEDURE_WAIT) {
            return true;
        }
    }

    return false;
}

// A pick along the hit line from half its length before the impact; when it misses, the rigid body closest to a
// 200-unit sphere at the impact gives the scene object.
NiNode* Actor::GetClosestBone(NiPoint3 aStartingLocation, NiPoint3 aImpactLocation) {
    NiNode* p3D = TESObjectREFR::Get3D();
    NiAVObject* pHit = nullptr;
    float fPickLength = 512.0f;
    NiPoint3 kDirection = aImpactLocation - aStartingLocation;
    NiPoint3::UnitizeVector(kDirection);
    NiPoint3 kPickFrom = aImpactLocation - kDirection * fPickLength * 0.5f;
    pHit = ArrowProjectile::PickSceneGraph(p3D, kPickFrom, kDirection, fPickLength, false);
    if (!pHit) {
        hkSphere kSphere;
        hkVector4 kCenter;
        NI2HK(kCenter, aImpactLocation);
        kSphere.setPosition(kCenter);
        kSphere.setRadius(200.0f);
        bhkRigidBody* pBody = bhkUtilFunctions::GetClosestBody(p3D, kSphere, false);
        if (pBody) {
            bhkCollisionObject* pCollision = bhkCollisionObject::Getbhk(**pBody);
            if (pCollision) {
                pHit = pCollision->GetSceneGraphObject();
            }
        }
    }
    if (pHit) {
        return pHit->IsNode();
    }
    return nullptr;
}

// For an actor that is knocked out (and not getting up, knock state 6), dead or reanimating: sets the character
// controller to follow the ragdoll, has Havok write the knocked-down movement into arMovementDelta and rotates it
// into the actor's Z rotation.
bool Actor::GetKnockedDownMovementDelta(NiPoint3& aMovementDelta) {
    bool bResult = false;
    if (!GetCurrentAIProcess()) {
        return bResult;
    }
    if ((!IsKnockedOut() || GetCurrentAIProcess()->GetKnockState() == 6) &&
        !IsDead(false) &&
        !GetCurrentAIProcess()->GetCurrentlyReanimating()) {
        return bResult;
    }

    bool bAlive = !IsDead(false);
    bhkUtilFunctions::GetKnockedDownMovementDelta(TESObjectREFR::Get3D(), &aMovementDelta, bAlive);
    bhkCharacterController* pCharController = GetCharController();
    if (pCharController) {
        pCharController->SetFollowRagdoll(true);
    }
    NiPoint3 kAngle = GetAngleOnReference();
    NiMatrix3 kRotation;
    kRotation.MakeZRotation(kAngle.z);
    aMovementDelta = aMovementDelta * kRotation;
    return true;
}

BGSBodyPart::LIMB_ENUM Actor::GetDamageLocation(NiNode* apNode) {
    if (apNode && pCurrentProcess) {
        NiNode* pRootNode = Get3D();
        NiNode* pNode = apNode;
        BipedAnim* pBiped = GetBiped();
        NiNode* pWeaponBone = pCurrentProcess->GetWeaponBone(pBiped);
        while (pNode && pNode != pRootNode) {
            if (pNode->m_pkParent == pWeaponBone) {
                return BGSBodyPart::LIMB_WEAPON;
            }
            for (BGSBodyPart::LIMB_ENUM i = BGSBodyPart::LIMB_TORSO; i < BGSBodyPart::LIMB_COUNT;
                 i = static_cast<BGSBodyPart::LIMB_ENUM>(i + 1)) {
                if (pCurrentProcess->GetDamageNode(i) == pNode) {
                    return i;
                }
            }
            pNode = pNode->m_pkParent;
        }
    }
    return BGSBodyPart::LIMB_NONE;
}

void Actor::UpdateAnimationMovement(Animation* apAnim, float afDelta) {
    NiPoint3 movedelta;
    AIProcessing_UpdateAnimationMovement.Start();
    if (this == pPlayer) {
        ++cSkipThreadQueue;
    }
    if (apAnim) {
        bool bfirstPerson = this == pPlayer && !pPlayer->Is3rdPerson();
        if (this == pPlayer || iNumHWThreads.Int() == 1) {
            apAnim->UpdateMovementNoWorldUpdate(this);
        }
        bool bincapacitated = IsUnConscious() || (IsDead(false) || (IsDying() || IsKnockedOut()));
        bool bConsciousState = !bincapacitated && (!IsSleeping() && !bfirstPerson);
        float fdeltaTime = appTimer.fDelta;
        if (pVATS.eMode == VATS::VATS_PLAYBACK && this == pVATSTarget) {
            fdeltaTime = pVATS.GetTargetUpdateMult() * fdeltaTime;
        }
        if (bConsciousState) {
            ProcessTracking(fdeltaTime);
            ProcessEmotions(fdeltaTime);
            AddGunWobble();
        }
        if (this == pPlayer || iNumHWThreads.Int() == 1) {
            // The getter's value is kept as a byte and passed zero-extended to the setter.
            uint8_t bprevPickOverride = bhkWorld::QPickCountOverride();
            if (this == pPlayer) {
                bhkWorld::SetPickCountOverride(1);
            }
            if (!bfirstPerson && UpdateRagdollAnimState(bConsciousState, fdeltaTime)) {
                UpdateRagdollAnimation(true);
            }
            if (this == pPlayer) {
                bhkWorld::SetPickCountOverride(bprevPickOverride);
                TESHavokUtilities::ConsolidateSimIslands(this);
            }
        }
        else {
            UpdateRagdollAnimState(bConsciousState, fdeltaTime);
        }
        apAnim->UpdateSceneGraphNoController();
        switch (GetSitSleepState()) {
            case SIT_SLEEP_WAITING_FOR_SIT_ANIM:
            case SIT_SLEEP_WANT_TO_STAND:
            case SIT_SLEEP_WAITING_FOR_SLEEP_ANIM:
            case SIT_SLEEP_WANT_TO_WAKE: {
                if (apAnim != pPlayer->GetAnimation(true)) {
                    movedelta = NiPoint3::ZERO;
                    float frefRaceScale = GetScale();
                    if (frefRaceScale != 1.0) {
                        bool bnoCollision = GetNoCollision();
                        apAnim->GetDelta(&movedelta, this, bnoCollision, false);
                        if (movedelta.z != 0.0) {
                            float fzOffset = movedelta.z - movedelta.z * frefRaceScale;
                            float fnewZ = GetLocationOnReference().z - fzOffset;
                            SetLocationOnReferenceZ(fnewZ);
                        }
                    }
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    if (this == pPlayer) {
        --cSkipThreadQueue;
    }
    AIProcessing_UpdateAnimationMovement.Stop();
}

// Never-seen actors reset; others once more than iHoursToRespawnCell has passed since they were last seen, unless
// their encounter zone never resets. The compares are unsigned.
bool Actor::ShouldReset(uint32_t aiCurrentTime) {
    bool bReset = false;
    if (!iLastSeenTime) {
        bReset = true;
    }
    else if (aiCurrentTime > iLastSeenTime && aiCurrentTime - iLastSeenTime > TESObjectCELL::GetHoursToRespawnCell()) {
        BGSEncounterZone* pZone = GetEncounterZone();
        if (!pZone || !pZone->QNeverReset()) {
            bReset = true;
        }
    }
    return bReset;
}

bool Actor::IsReanimated() {
    return GetLifeState() == ACTOR_LIFE_STATE_REANIMATE;
}

bool Actor::IsInCombat() {
    return bInCombat;
}

bool Actor::IsPlayerTeammate() {
    return bPlayerTeammate;
}

bool Actor::IsSwimming() {
    return bSwimming;
}

bool Actor::CanSpeak() {
    if (IsParalyzed() || IsDead(false)) {
        return false;
    }
    if (IsKnockedOut()) {
        return false;
    }
    return !IsUnConscious();
}

void Actor::EndMovement() {
    if (pCurrentProcess) {
        pCurrentProcess->EndMoveMessage(this);
    }
}

bool Actor::IsSleeping() {
    return cCurrentSitSleepState == SIT_SLEEP_IS_SLEEPING;
}

bool Actor::IsAlarmed() {
    TESPackage* lpPackage = nullptr;
    if (pCurrentProcess) {
        lpPackage = pCurrentProcess->GetCurrentPackage();
    }
    return lpPackage && lpPackage->GetPackType() == PACKAGE_TYPE_ALARM;
}

bool Actor::IsMoving() {
    return (GetMoveMode() & 0xF) != 0;
}

bool Actor::IsTurning() {
    return (GetMoveMode() & 0x30) != 0;
}

bool Actor::IsEating() {
    bool bEating = false;
    if (pCurrentProcess) {
        TESPackage* pPackage = pCurrentProcess->GetCurrentPackage();
        if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_EAT) {
            PACKAGE_PROCEDURE_TYPE eIndex = pPackage->GetProcedureType();
            if (Procedure::Action[eIndex][pCurrentProcess->GetProcedureIndexRunning()] == PROCEDURE_EAT) {
                bEating = true;
            }
        }
    }
    return bEating;
}

int32_t
Actor::GetFactionReaction(Actor* apActor, int32_t& airank, TESFaction*& apthisfaction, TESFaction*& aptargfaction) {
    uint32_t uihisCount;
    TESFaction* pmyFactionArray[129];
    TESFaction* phisFactionArray[128];
    int32_t ihighestReaction = -100000;
    int32_t ilowestReaction = 100000;
    int32_t ireturnReaction = 0;
    BSSimpleList<FACTION_RANK*>* pmyFactionList = &GetActorBase()->listFactions;
    BSSimpleList<FACTION_RANK*>* phisFactionList = &apActor->GetActorBase()->listFactions;
    ExtraFactionChanges* pmyFactionChanges = m_Extra.GetFactionChangesExtra();
    ExtraFactionChanges* phisFactionChanges = apActor->m_Extra.GetFactionChangesExtra();
    BSSimpleList<FACTION_RANK*>* pmyChangeList = pmyFactionChanges ? pmyFactionChanges->pFactionChanges : nullptr;
    uint32_t uimyCount = IntegrateFactionLists(pmyFactionArray, 128u, pmyFactionList, pmyChangeList);
    if (phisFactionChanges) {
        uihisCount = IntegrateFactionLists(
            phisFactionArray,
            128u,
            phisFactionList,
            phisFactionChanges->pFactionChanges
        );
    }
    else {
        uihisCount = IntegrateFactionLists(phisFactionArray, 128u, phisFactionList, nullptr);
    }
    for (uint32_t i = 0; i < uimyCount; ++i) {
        if (pmyFactionArray[i]) {
            for (uint32_t j = 0; j < uihisCount; ++j) {
                if (phisFactionArray[j]) {
                    if (pmyFactionArray[i] == phisFactionArray[j]) {
                        int32_t ireaction = pmyFactionArray[i]->GetReaction(pmyFactionArray[i]);
                        if (ireaction > ihighestReaction) {
                            ihighestReaction = ireaction;
                            airank = 1;
                            apthisfaction = pmyFactionArray[i];
                            aptargfaction = pmyFactionArray[i];
                            ireturnReaction = ireaction;
                        }
                    }
                    else if (ihighestReaction == -100000) {
                        int32_t ireaction = pmyFactionArray[i]->GetReaction(phisFactionArray[j]);
                        if (ireaction < ilowestReaction) {
                            ilowestReaction = ireaction;
                            airank = 1;
                            apthisfaction = pmyFactionArray[i];
                            aptargfaction = phisFactionArray[j];
                            ireturnReaction = ireaction;
                        }
                    }
                }
            }
        }
    }
    return ireturnReaction;
}

void Actor::HandleInvalidPosition() {}

uint8_t Actor::Has360LOS(Actor* apTarget) {
    if (!pCurrentProcess) {
        return 0;
    }
    DetectionState* pstate = pCurrentProcess->GetDetectionState(apTarget, 0);
    return pstate && pstate->b360view;
}

bool Actor::SetPathfindingGoal(PathingRequest* apRequest) {
    if (apRequest && !apRequest->GetAvoidNodeArray()) {
        PathingAvoidNodeArray* pavoidArray = GetCurrentAIProcess()->CreateAvoidArray(this);
        apRequest->SetAvoidNodeArray(pavoidArray);
    }

    return pActorMover->SetGoal(apRequest);
}

bool Actor::SetPathfindingGoal(
    TESObjectREFR* apPathingRequest,
    float afTargetRadius,
    PathingAvoidNodeArray* apAvoidNodes
) {
    if (!apAvoidNodes) {
        apAvoidNodes = pCurrentProcess->CreateAvoidArray(this);
    }
    return pActorMover->SetGoal(apPathingRequest, afTargetRadius, apAvoidNodes);
}

float Actor::GetHealthPercentage() {
    int32_t ibaseHealth = GetBaseActorValue(ActorValue::HEALTH);
    if (ibaseHealth) {
        return GetActorFloatValue(ActorValue::HEALTH) / static_cast<double>(ibaseHealth);
    }

    return 1.0f;
}

const char* Actor::GetCurrentPackageName() {
    if (!pCurrentProcess || !pCurrentProcess->GetCurrentPackage()) {
        return nullptr;
    }
    return packageTypeToString[pCurrentProcess->GetCurrentPackage()->GetPackType()];
}

bool Actor::GetIsTalking() {
    if (bTalkingToPlayer) {
        return true;
    }
    return pCurrentProcess && pCurrentProcess->IsTalking(this);
}

int32_t Actor::GetBarterGold() {
    return GetActorBase()->GetBarterGold();
}

BGSBodyPart::LIMB_ENUM Actor::GetKillingBlowLimb() {
    BGSBodyPart::LIMB_ENUM eLimb = BGSBodyPart::LIMB_NONE;
    ExtraDismemberedLimbs* pDismembered = m_Extra.GetDismembermentExtra();
    if (pDismembered) {
        eLimb = pDismembered->eLastHitLimb;
    }
    return eLimb;
}

int32_t Actor::GetMinorCrimeCount() {
    TESFaction* pfactionArray[128];
    int32_t icount = 0;
    BSSimpleList<FACTION_RANK*>* pfactionList = &GetActorBase()->listFactions;
    ExtraFactionChanges* pfactionChanges = m_Extra.GetFactionChangesExtra();
    BSSimpleList<FACTION_RANK*>* pchangeList = pfactionChanges ? pfactionChanges->pFactionChanges : nullptr;
    uint32_t uifactionCount = IntegrateFactionLists(pfactionArray, 128u, pfactionList, pchangeList);
    for (uint32_t i = 0; i < uifactionCount; ++i) {
        if (pfactionArray[i]) {
            uint32_t uiminorCrime = pfactionArray[i]->iMinorCrime;
            icount += uiminorCrime;
        }
    }
    return icount;
}

int32_t Actor::GetMajorCrimeCount() {
    TESFaction* pfactionArray[128];
    int32_t icount = 0;
    BSSimpleList<FACTION_RANK*>* pfactionList = &GetActorBase()->listFactions;
    ExtraFactionChanges* pfactionChanges = m_Extra.GetFactionChangesExtra();
    BSSimpleList<FACTION_RANK*>* pchangeList = pfactionChanges ? pfactionChanges->pFactionChanges : nullptr;
    uint32_t uifactionCount = IntegrateFactionLists(pfactionArray, 128u, pfactionList, pchangeList);
    for (uint32_t i = 0; i < uifactionCount; ++i) {
        if (pfactionArray[i]) {
            icount += pfactionArray[i]->iMajorCrime;
        }
    }
    return icount;
}

int32_t Actor::GetMajorCrimesNumber() {
    return iMajorCrimes & 0x3FFFFFFF;
}

void Actor::SetAlert(bool abflag) {
    if (pCurrentProcess) {
        pCurrentProcess->SetAlert(abflag);
    }
}

bool Actor::IsSkinnedToLimb(NiAVObject* apSkin, NiNode* apLimb) {
    if (!apSkin) {
        return false;
    }
    NiGeometry* pgeom = nullptr;
    if (apSkin->IsGeometry()) {
        pgeom = static_cast<NiGeometry*>(apSkin);
    }
    else if (apSkin->IsNode() && !apSkin->IsFadeNode()) {
        NiNode* pnode = apSkin->IsNode();
        uint32_t n = 0;
        for (uint32_t i = pnode->GetArrayCount(); n < i; i = pnode->GetArrayCount()) {
            NiAVObject* pchild = pnode->GetAt(n);
            if (IsSkinnedToLimb(pchild, apLimb)) {
                return true;
            }
            ++n;
        }
    }
    if (!pgeom || !pgeom->IsGeometry()) {
        return false;
    }
    NiSkinInstance* pskin = pgeom->GetSkinInstance();
    if (pskin) {
        uint32_t uiBones = pskin->GetSkinData()->m_uiBones;
        NiAVObject** pbones = pskin->m_ppkBones;
        for (uint32_t j = 0; j < uiBones; ++j) {
            for (uint32_t k = 0; k < uiLimbCount; ++k) {
                if (pLimbBones[k] == pbones[j]) {
                    return true;
                }
            }
        }
    }
    return false;
}

void Actor::SetNonLinesAlpha(NiNode* apNode, float afAlpha) {
    NiNode* pChild;
    BSShaderProperty* property;
    NiGeometry* pGeo = apNode->IsGeometry();
    if (pGeo && !pGeo->IsLinesGeom()) {
        int32_t iType = NiShadeProperty::GetType();
        property = static_cast<BSShaderProperty*>(pGeo->NiAVObject::GetProperty(iType));
        if (property) {
            property->SetAlpha(afAlpha);
            property->SetFadeAlpha(afAlpha);
        }
    }

    NiNode* pNode = apNode->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            pChild = static_cast<NiNode*>(pNode->GetAt(i));
            if (pChild) {
                SetNonLinesAlpha(pChild, afAlpha);
            }
        }
    }
}

void Actor::SetMoveMode(uint32_t auiMoveMode) {
    pActorMover->SetMoveModePreference(auiMoveMode);
}

void Actor::ClearMoveMode(uint32_t auiMoveMode) {
    pActorMover->ClearMoveModePreference(auiMoveMode);
}

bool Actor::IsWeaponDrawn() {
    if (pCurrentProcess) {
        return pCurrentProcess->GetWeaponDrawn();
    }
    return false;
}

bool Actor::IsRefractive() {
    ExtraDataList* lpExtraList = GetExtra();
    return lpExtraList && lpExtraList->GetRefractionPropertyExtra();
}

// Moves the actor onto the navmesh and out of the player's way: to the closest navmesh point, pushed in from a nearby
// open edge by the actor's radius, then (when it would overlap the player) to a close point around the player. An
// actor that cannot move is only moved with abForceMove, and an actor its travel package holds at an XMarkerHeading
// or an exact editor location is left where it is. The move is queued while 3D tasks are queued.
bool Actor::MoveToSafePoint(bool abForceMove) {
    bool bmoved = false;
    if ((!CanMove() || SwimsOnly() || IsImmobile()) && !abForceMove) {
        return bmoved;
    }
    bool bfindpoint = true;
    TESPackage* ppack = nullptr;
    if (pCurrentProcess) {
        ppack = pCurrentProcess->GetCurrentPackage();
    }
    if (ppack && ppack->IsActorAtLocation(this, false, -1.0f, false)) {
        if (ppack->GetPackType() == PACKAGE_TYPE_TRAVEL &&
            ppack->GetLocationReference(this) &&
            ppack->GetLocationReference(this)->GetObjectReference() == pXMarkerHeading) {
            bfindpoint = false;
        }
        else if (
            ppack->GetLocation() &&
            ppack->GetLocation()->GetLocType() == PLT_EDITORLOC &&
            !ppack->GetLocationRadius(this)
        ) {
            bfindpoint = false;
        }
    }
    if (!bfindpoint) {
        return bmoved;
    }
    NiPoint3 dest = GetLocationOnReference();
    if (Pathing::FindClosestPointOnNavmesh(GetWorldSpace(), GetParentCell(), GetLocationOnReference(), &dest)) {
        alignas(PathingLocation) uint8_t aActorLocStorage[sizeof(PathingLocation)];
        PathingLocation* pActorLoc = ::new (aActorLocStorage) PathingLocation(dest, this);
        alignas(NavMeshEdgeLocation) uint8_t aEdgeLocStorage[sizeof(NavMeshEdgeLocation)];
        NavMeshEdgeLocation* pEdgeLoc = ::new (aEdgeLocStorage) NavMeshEdgeLocation;
        if (Pathing::FindClosestOpenEdge(*pActorLoc, GetRadius(), *pEdgeLoc)) {
            NiPoint3 delta = dest - pEdgeLoc->Location;
            delta.z = 0.0f;
            float fdist = delta.UnitizeGetLength();
            if (fdist < GetRadius()) {
                dest = pEdgeLoc->Location + delta * GetRadius();
            }
        }
        pActorLoc->~PathingLocation();
    }
    NiPoint3 playerDelta = dest - pPlayer->GetLocationOnReference();
    float fsumRadii = pPlayer->GetRadius() + GetRadius();
    // The 1.5 constants are doubles.
    if (fsumRadii * fsumRadii * 1.5 * 1.5 > playerDelta.SqrLength()) {
        alignas(PathingRequestClosePoint) uint8_t aRequestStorage[sizeof(PathingRequestClosePoint)];
        PathingRequestClosePoint* pClosePointRequest = ::new (aRequestStorage) PathingRequestClosePoint;
        pClosePointRequest->Initialize(pPlayer);
        pClosePointRequest->SetMinDistance(static_cast<float>(fsumRadii * 1.5));
        pClosePointRequest->SetMaxDistance(static_cast<float>(fsumRadii * 3.0));
        Pathing::FindClosePoint(pClosePointRequest, &dest);
        pClosePointRequest->~PathingRequestClosePoint();
    }
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        TaskQueueInterface::GetInstance()->QueueActorSetPosition(this, dest);
        TaskQueueInterface::GetInstance()->QueueActorMoveCharController(this);
    }
    else {
        SetPosition(dest);
    }
    bmoved = true;
    return bmoved;
}

bool Actor::GetGhost() {
    if (pCurrentProcess) {
        return pCurrentProcess->GetCachedFlag(CachedValues::CVF_ACTOR_IS_GHOST);
    }
    return GetExtra()->GetGhost();
}

bool Actor::GetIronSights() {
    if (pCurrentProcess) {
        return pCurrentProcess->GetIronSights();
    }
    return false;
}

void Actor::SetIronSights(bool abIronSights, bool abPostAnim, bool abCheckAnimBlendOut) {
    float fVal;
    if (pCurrentProcess && (!IsCreature() && (abPostAnim || GetIronSights() != abIronSights))) {
        BSAnimGroupSequence* pseq;
        if (this == pPlayer) {
            if (abIronSights) {
                if (gs_bTrueIronSights_GamePlay.Bool()) {
                    NiAVObject* p3D = pPlayer->Get3D(true);
                    NiAVObject* pSightingNode = BSUtilities::GetObjectByName(p3D, "##SightingNode");
                    pPlayer->SetWeaponSightingNode(pSightingNode);
                }
            }
            else {
                pPlayer->SetWeaponSightingNode(nullptr);
            }
        }
        Animation* panim = Actor::GetAnimation();
        if (panim) {
            pseq = panim->GetCurrentSequence(AGS_WEAPON);
        }
        else {
            pseq = nullptr;
        }
        ANIM_GROUP_ACTION_TYPE eaction = ANIM_GROUP_ACTION_TYPE_COUNT;
        if (pseq && pseq->GetState() != NiControllerSequence::kEaseOut) {
            eaction = animGroupInfo[pseq->GetAnimGroup()->GetType()].eAction;
        }

        if (!abCheckAnimBlendOut || eaction != AGAT_LOOP) {
            pCurrentProcess->SetIronSights(abIronSights);
            if (this == pPlayer && pVATS.eMode != VATS::VATS_PLAYBACK) {
                HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_CHANGED);
                fVal = 1.0f;
                if (pCurrentProcess->GetWeaponDrawn() && pCurrentProcess->GetCurrentWeapon()) {
                    ItemChange* pweaponItem = pCurrentProcess->GetCurrentWeapon();
                    TESModel* pScopeModel = static_cast<TESObjectWEAP*>(pweaponItem->pContainerObj)->GetScopeModel();
                    if (pScopeModel->GetModelLength()) {
                        int32_t iWant3rdPerson = pPlayer->GetWant3rdPerson();
                        if (iWant3rdPerson == pPlayer->IsActually3rdPerson()) {
                            ItemChange* pWeap = pCurrentProcess->GetCurrentWeapon();
                            if (!static_cast<TESObjectWEAP*>(pWeap->pContainerObj)->TESObjectWEAP::HasModScope() ||
                                pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_IRON_SITES, fVal)) {
                                if (abIronSights) {
                                    NiNode* p1stPerson3D = static_cast<NiNode*>(pPlayer->Get3D(true));
                                    pWeap = pCurrentProcess->GetCurrentWeapon();
                                    WEAPON_TYPE eWeaponType = static_cast<TESObjectWEAP*>(pWeap->pContainerObj)
                                                                  ->TESObjectWEAP::GetWeaponType();
                                    ClearGunWobble(weaponToAnimGroupWeapon[eWeaponType], p1stPerson3D);
                                    if (!pPlayer->ForceTemp1stPerson(true)) {
                                        pPlayer->Get3D(true)->SetAppCulled(true);
                                    }
                                }
                                else {
                                    NiNode* p1stPerson3D = static_cast<NiNode*>(pPlayer->Get3D(true));
                                    ClearGunWobble(AGW_NONE, p1stPerson3D);
                                    if (!pPlayer->GetTemp1stPersonSwitchBack()) {
                                        pPlayer->Get3D(true)->SetAppCulled(false);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (eaction == AGAT_LOOP) {
                if (pseq) {
                    ANIM_GROUP_ENUM ebaseAnimGroup = pseq->GetAnimGroup()->GetType();
                    if (TESAnimGroup::IsIronSightsAction(ebaseAnimGroup)) {
                        ebaseAnimGroup = static_cast<ANIM_GROUP_ENUM>(ebaseAnimGroup - 3);
                    }
                    if (GetIronSights()) {
                        if (!pCurrentProcess->GetCurrentWeapon() ||
                            static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj)
                                ->TESObjectWEAP::GetUse3PISAnims()) {
                            ebaseAnimGroup = static_cast<ANIM_GROUP_ENUM>(ebaseAnimGroup + 3);
                        }
                    }
                    uint16_t sanim = GetAnimGroup(ebaseAnimGroup, nullptr, false, panim);
                    if (TESAnimGroup::GetType(sanim) == ebaseAnimGroup) {
                        if (sanim != pseq->GetAnimGroup()->sType) {
                            panim->BlendOut(AGS_WEAPON, true);
                        }
                    }
                }
                if (this == pPlayer) {
                    panim = pPlayer->GetAnimation(true);
                    if (panim) {
                        pseq = panim->GetCurrentSequence(AGS_WEAPON);
                    }
                    else {
                        pseq = nullptr;
                    }
                    if (pseq) {
                        ANIM_GROUP_ENUM ebaseAnimGroup = pseq->GetAnimGroup()->GetType();
                        if (TESAnimGroup::IsIronSightsAction(ebaseAnimGroup)) {
                            ebaseAnimGroup = static_cast<ANIM_GROUP_ENUM>(ebaseAnimGroup - 3);
                        }
                        ItemChange* pWeap = pCurrentProcess->GetCurrentWeapon();
                        if (GetIronSights()) {
                            if (!pWeap ||
                                (static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj)
                                     ->TESObjectWEAP::GetUse1PISAnims() &&
                                 (!static_cast<TESObjectWEAP*>(pWeap->pContainerObj)->TESObjectWEAP::HasModScope() ||
                                  !pWeap->HasModEffectActive(WEAPON_MOD_IRON_SITES)))) {
                                ebaseAnimGroup = static_cast<ANIM_GROUP_ENUM>(ebaseAnimGroup + 3);
                            }
                        }
                        uint16_t sanim = GetAnimGroup(ebaseAnimGroup, nullptr, false, panim);
                        if (TESAnimGroup::GetType(sanim) == ebaseAnimGroup) {
                            if (sanim != pseq->GetAnimGroup()->sType) {
                                panim->BlendOut(AGS_WEAPON, true);
                            }
                        }
                    }
                }
            }
        }
    }
}

float Actor::GetGunSkill(GUN_SKILL_ENUM aeType) {
    ItemChange* pweaponItem;
    uint8_t ironSights;
    double dprocessAimLooking;
    uint8_t isSneaking;
    int16_t moveFlags;
    uint8_t bIsRunning;
    TESObjectWEAP* pWeapon;
    float fSpread;
    uint8_t bIsWalking;
    float fAimLooking;
    if (pCurrentProcess && pCurrentProcess->GetCurrentWeapon()) {
        pweaponItem = pCurrentProcess->GetCurrentWeapon();
        pWeapon = static_cast<TESObjectWEAP*>(pweaponItem->ItemChange::GetItemChangeObject());
        fSpread = 0.0f;
        if (aeType == GUN_SKILL_GUN) {
            if (fGunSkillGun >= 0.0) {
                return fGunSkillGun;
            }

            fSpread = 0.0f;
            fGunSkillGun = 0.0f;
            return fSpread;
        }

        if (aeType == GUN_SKILL_ACTOR && fGunSkillActor >= 0.0) {
            return fGunSkillActor;
        }

        if (aeType || !(fGunSkillHUD >= 0.0)) {
            if (aeType == GUN_SKILL_VATS && fGunSkillVATS >= 0.0) {
                return fGunSkillVATS;
            }

            fAimLooking = pCurrentProcess->GetAimLooking();
            bIsWalking = 0;
            bIsRunning = 0;
            if (aeType != GUN_SKILL_VATS) {
                moveFlags = static_cast<int16_t>(GetMoveMode());
                bIsWalking = (moveFlags & ActorMover::MF_WALKING) != 0;
                bIsRunning = (moveFlags &
                              (ActorMover::MF_FORWARD | ActorMover::MF_BACKWARD | ActorMover::MF_LEFT |
                               ActorMover::MF_RIGHT)) != 0 &&
                    IsRunning();
            }
            if (pVATS.eMode == VATS::VATS_MENU_MODE || pVATS.eMode == VATS::VATS_SCAN_MODE) {
                fSpread = CombatFormulas::GetGunSpread(this, pWeapon, fAimLooking, true, false, bIsWalking, bIsRunning);
            }
            else {
                isSneaking = IsSneaking();
                ironSights = GetIronSights();
                fSpread = CombatFormulas::GetGunSpread(
                    this,
                    pWeapon,
                    fAimLooking,
                    ironSights,
                    isSneaking,
                    bIsWalking,
                    bIsRunning
                );
            }
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::CALCULATE_GUN_SPREAD, this, pWeapon, &fSpread);
            if (aeType == GUN_SKILL_ACTOR) {
                fGunSkillActor = fSpread;
            }
            else if (aeType) {
                if (aeType == GUN_SKILL_VATS) {
                    fSpread = fSpread * fNPCMaxGunWobbleAngle.Float();
                    dprocessAimLooking = MobileObject::GetCurrentAIProcess()->GetAimLooking();
                    fSpread = static_cast<float>(dprocessAimLooking + fSpread);
                    fGunSkillVATS = fSpread;
                }
            }
            else {
                fGunSkillHUD = fSpread;
            }
            return fSpread;
        }

        return fGunSkillHUD;
    }

    return 0.0;
}

float Actor::GetMedicineEffectivenessMultiplier() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        CachedValuesOwner* pOwner;
        if (this) {
            pOwner = static_cast<CachedValuesOwner*>(this);
        }
        else {
            pOwner = nullptr;
        }
        return pCurrentProcess->GetMedicineSkillMult(pOwner);
    }

    return CalculateCachedMedicineEffectivenessMult();
}

float Actor::GetSurvivalEffectivenessMultiplier() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        CachedValuesOwner* pOwner;
        if (this) {
            pOwner = static_cast<CachedValuesOwner*>(this);
        }
        else {
            pOwner = nullptr;
        }
        return pCurrentProcess->GetSurvivalSkillMult(pOwner);
    }

    return CalculateCachedSurvivalEffectivenessMult();
}

bool Actor::IsAttacking() {
    int32_t animAction = GetAnimAction();
    return animAction >= ANIM_ACTION_ATTACK && animAction <= ANIM_ACTION_ATTACK_THROW_ATTACH;
}

// The base form's membership, overridden by the reference's faction changes: an expelled member is out, a
// faction added by the changes is in.
bool Actor::IsInFaction(TESFaction* apFaction) {
    ExtraFactionChanges* pChanges = GetExtra()->GetFactionChangesExtra();
    bool bInFaction = GetActorBase()->TESActorBaseData::IsInFaction(apFaction);
    if (!pChanges) {
        return bInFaction;
    }
    bInFaction = (bInFaction && !pChanges->GetIsExpelled(apFaction)) || pChanges->GetIsInFaction(apFaction);
    return bInFaction;
}

// Expels the actor from apFaction through its faction changes, adding the extra data first when missing.
void Actor::RemoveFaction(TESFaction* apFaction) {
    AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    ExtraFactionChanges* pChanges = GetExtra()->GetFactionChangesExtra();
    if (!pChanges) {
        GetExtra()->AddFactionChangesExtra();
        pChanges = GetExtra()->GetFactionChangesExtra();
    }
    if (pChanges) {
        pChanges->ExpelFromFaction(apFaction);
    }
}

bool Actor::GetTurretBehavior() {
    return bTurretBehavior;
}

void Actor::SetTurretBehavior(bool bVal) {
    bTurretBehavior = bVal;
}

// An immobile creature with a yaw bone and no ragdoll data becomes a turret. Otherwise the controller is built on the
// skeleton model (under a name unique to the actor with abForceUniqueSkeleton) and, with abSystemInspect, the look-at,
// foot and grab IK are set up from the body parts; then the collision group, the death poses, the feedback data and the
// phantom. Warning is an empty stub. An old controller is deleted without clearing the field, so a unique name formats
// the stale pointer.
void Actor::CreateRagdollInstance(
    NiAVObject* apObject,
    bool abDeleteCurrInst,
    bool abSystemInspect,
    bool abForceUniqueSkeleton
) {
    TESActorBase* pBase = GetActorBase();
    BGSBodyPartData* ppartdata = pBase->GetBodyPartData();
    BGSRagdoll* pragdollData = ppartdata ? ppartdata->QRagdollData() : nullptr;
    if (pRagdollController && abDeleteCurrInst) {
        delete pRagdollController;
    }
    NiAVObject* pyawBone = BSUtilities::GetObjectByName(apObject, FixedStrings::QBip01Yaw());
    if (pyawBone && GetActorBase()->IsImmobile() && !pragdollData) {
        SetTurretBehavior(true);
        return;
    }
    const char* pmodelFileName = GetObjectReference()->GetModelFileName(this);
    ENUM_LOD_MULT elod = TES::GetLODMult(GetObjectReference());
    NiNode* p3DPose = pModelLoader->LoadFile(pmodelFileName, elod, true, false, false, true);
    char pscaledFileName[260];
    GetObjectReference()->GetScaledModelFileName(this, pscaledFileName);
    if (abForceUniqueSkeleton) {
        BSsprintf(pscaledFileName, 0x104, "%s_%08X_%08X", pscaledFileName, this, pRagdollController);
    }
    pRagdollController = new bhkRagdollController(apObject, p3DPose, pscaledFileName);
    if (!pRagdollController->QRagdollAnimInit()) {
        const char* pname = GetFormEditorIDLength() ? GetFormEditorID() : GetFullName();
        Warning("AI: Could not initialize Ragdoll Controller for %s.", pname);
        return;
    }
    NiMatrix3 currRot;
    currRot = NiMatrix3::IDENTITY;
    MultipleMatrixByRace(currRot);
    currRot.Inverse(pRagdollController->QScaleInv());
    if (abSystemInspect) {
        BSSimpleList<BGSBodyPart*> IKParts;
        ppartdata->BuildIKPartList(&IKParts);
        alignas(BSSimpleArray<const char*>) uint8_t footNameAStorage[sizeof(BSSimpleArray<const char*>)];
        BSSimpleArray<const char*>* pfootNameA = ::new (footNameAStorage) BSSimpleArray<const char*>;
        bool benableLookIK = false;
        while (!IKParts.IsEmpty()) {
            BGSBodyPart* ppart = IKParts.GetItem();
            IKParts.RemoveHead();
            if (ppart->IsIKBiped()) {
                const char* pnodeName = ppart->GetIKStartNodeName();
                pfootNameA->Add(pnodeName);
            }
            if (!benableLookIK && ppart->DoesHeadtracking()) {
                if (!IsCreature()) {
                    pRagdollController->SetUseHeadPose(true);
                }
                benableLookIK = true;
                pRagdollController->InitLookIK(ppart->GetIKStartNodeName());
                float fmaxAngle = ppart->GetTrackingMaxAngle() * 0.01745329238474369;
                pRagdollController->SetLookAtMaxAngle(bhkRagdollController::HEAD_BONE, fmaxAngle);
                pRagdollController->SetLookIKEnable(true);
                if (!pRagdollController->QLookIKEnable()) {
                    const char* pname = GetFormEditorIDLength() ? GetFormEditorID() : GetFullName();
                    Warning("AI: Could not initialize LookIK system for %s.", pname);
                }
            }
        }
        if (std::strstr(ppartdata->GetFormEditorID(), "Prime")) {
            pRagdollController->SetAlignFeetToTerrain(true);
            pRagdollController->SetUsePhantomFootIKCasts(false);
        }
        pRagdollController->InitFootIK(*pfootNameA);
        pRagdollController->SetFootIKEnable(true);
        pRagdollController->SetGrabIKEnable(true);
        if (!pRagdollController->QFootIKEnable() && pfootNameA->GetSize()) {
            const char* pname = GetFormEditorIDLength() ? GetFormEditorID() : GetFullName();
            Warning("AI: Could not initialize FootIK system for %s.", pname);
        }
        if (pRagdollController->QGrabIKEnable()) {
            RegisterGrabIKNote();
        }
        pfootNameA->~BSSimpleArray<const char*>();
        IKParts.RemoveAll();
    }
    CFilter icolFilter;
    GetCollisionFilter(&icolFilter.iFilter);
    pRagdollController->SetCollisionGroup(icolFilter.GetGroup());
    char poseFileName[260];
    BSstrcpy(poseFileName, 0x104, static_cast<TESModel*>(ppartdata)->GetModel());
    char* pfolder = std::strrchr(poseFileName, '\\');
    if (pfolder) {
        *++pfolder = '\0';
        BSstrcat(poseFileName, 0x104, "Death.psa");
        NiFixedString poseFileNameFS(poseFileName);
        {
            NiPointer<bhkPoseArray> spposeArr(nullptr);
            const char* pcPoseFileName;
            pcPoseFileName = poseFileNameFS;
            pRagdollController->InitPoseMatching(pcPoseFileName, -1, -1, -1);
        }
    }
    if (pragdollData && this != pPlayer) {
        pRagdollController->SetFeedbackData(pragdollData->QFeedbackData());
    }
    pRagdollController->SetPoseMatchEnable(true);
    bhkCharacterController* pcon = static_cast<bhkCharacterController*>(GetCharController());
    if (pcon) {
        pRagdollController->SetPhantom(pcon->GetShapePhantom());
    }
}

void Actor::RestoreHealth(float afSecondsElapsed) {
    uint8_t bIsSleeping = IsSleeping();
    float fHealRate = static_cast<float>(GetHealRate());
    float fHealthReturn = GamePlayFormulas::GetHealingRate(fHealRate, bIsSleeping) * afSecondsElapsed;
    float fPerkBonus = 0.0f;
    if (IsPc()) {
        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_RECOVERED_HEALTH, pPlayer, &fPerkBonus);
        fPerkBonus = fPerkBonus * afSecondsElapsed;
    }
    float famount = fHealthReturn + fPerkBonus;
    RestoreActorValue(ActorValue::HEALTH, famount);
}

void Actor::RestoreFatigue(float afSecondsElapsed) {
    float fendurance = GetEndurance();
    float famount = GamePlayFormulas::CalcFatigueReturn(fendurance) * afSecondsElapsed;
    float fold = GetFatigue();
    RestoreActorValue(ActorValue::FATIGUE, famount);
    if (fold < 0.0) {
        float fnew = fold + famount;
        if (fnew >= 0.0) {
            float fdamage = GetDamageModifier(ActorValue::FATIGUE);
            if (fdamage < 0.0) {
                float frestore = 1.0 - fdamage;
                RestoreActorValue(ActorValue::FATIGUE, frestore);
            }
        }
    }
}

void Actor::RestoreActionPoints(float afSecondsElapsed) {
    float fapValue;
    if (IsPc() && !pVATS.eMode) {
        fapValue = get_fActionPointsRestoreRate() * afSecondsElapsed;
        TESObjectWEAP* pWeap = nullptr;
        if (pCurrentProcess && pCurrentProcess->GetCurrentWeapon()) {
            pWeap = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
        }

        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ACTION_POINT_REGENERATION_ENTRY, this, pWeap, &fapValue);
        float fbaseAP = GetBaseActorFloatValue(ActorValue::ACTION_POINTS);
        float famount = fapValue * fbaseAP;
        RestoreActorValue(ActorValue::ACTION_POINTS, famount);
    }
}

// The process level check is signed: high and middle-high only.
ANIM_GROUP_WEAPON_ENUM Actor::GetCurrentWeaponAnimType() {
    if (IsDead(false)) {
        return AGW_NONE;
    }
    if (this == pPlayer) {
        return ANIM_GROUP_WEAPON_COUNT;
    }
    if (GetCurrentAIProcess()) {
        PROCESS_TYPE eLevel = GetCurrentAIProcess()->GetProcessLevel();
        if (eLevel >= PROCESS_TYPE_HIGH && eLevel <= PROCESS_TYPE_MIDDLE_HIGH) {
            TESObjectWEAP* pWeapon = GetCurrentWeapon();
            if (pWeapon) {
                return weaponToAnimGroupWeapon[pWeapon->GetWeaponType()];
            }
            return AGW_HAND_TO_HAND;
        }
    }
    return AGW_NONE;
}

bool Actor::GetRefEssential() {
    return (iMajorCrimes & ACTOR_FLAG_REF_ESSENTIAL) != 0;
}

ANIMATION_ACTION Actor::GetAnimAction() {
    if (pCurrentProcess) {
        return static_cast<ANIMATION_ACTION>(pCurrentProcess->GetAnimAction());
    }
    return ANIM_ACTION_NONE;
}

BipedAnim* Actor::GetVATSBiped() {
    return nullptr;
}

BSFaceGenAnimationData* Actor::GetFaceAnimationData() {
    if (pCurrentProcess) {
        return pCurrentProcess->GetFaceAnimationData(this);
    }
    return nullptr;
}

bool Actor::ConditionalFree(bool abArg) {
    if (Get3D()) {
        TESObjectCELL* pcell = GetParentCell();
        if (TES::pInstance->IsCellLoaded(pcell, abArg)) {
            if (pCurrentProcess->eLevel) {
                MovetoHigh();
            }
            else {
                if (this) {
                    delete this;
                }
                return false;
            }
        }
    }
    return true;
}

void Actor::PlayPickUpSound(TESBoundObject* apobjREFR, bool abPickUp, bool abUse) {
    if (apobjREFR) {
        BSSoundHandle soundHand;
        const char* soundname = GetPickUpSoundName(apobjREFR, abPickUp, abUse);
        if (this == pPlayer) {
            soundHand = BSAudio::QInstance().GetSoundHandleByName(soundname, 289u);
            if (!soundHand.IsPlaying()) {
                soundHand.Play(false);
            }
        }
        else {
            soundHand = PlaySoundByEditorName(soundname, false, 258u, true);
        }
    }
}

void Actor::StealAlarm(TESObjectREFR* apRef, TESBoundObject* apObj, int32_t ainumb, int32_t aiTotal, TESForm* apowner) {
    bool bsendwarning = false;
    Actor* pactor = nullptr;
    if (this == pPlayer) {
        int32_t inumWarnings = pPlayer->iNumberofStealWarnings;
        if (inumWarnings < iStealWarnings.Int()) {
            bsendwarning = this == pPlayer;
        }
        if (this == pPlayer) {
            pPlayer->IncNumCrimes(PackageNS::CRIME_TYPE_STEAL, ainumb);
        }
        else if (GetActorValue(ActorValue::SNEAK) >= 100) {
            return;
        }
        Actor* ptargactor = nullptr;
        bool bkarma = false;
        if (apRef->IsActor()) {
            ptargactor = static_cast<Actor*>(apRef);
        }
        TESForm* powner = apowner;
        if (!apowner && ptargactor) {
            powner = ptargactor->GetOwner();
        }

        bool bhardcore = pPlayer->IsHardcore();
        float fweight = TESWeightForm::GetFormWeight(apObj, bhardcore);
        if (fweight > 0.0) {
            fweight = fweight * fObjectWeightPickupDetectionMult.Float();
            pCurrentProcess->SetDetectionModifier(fweight);
        }
        if (ptargactor && ptargactor->IsNpc()) {
            pactor = ptargactor;
        }
        else if (powner->GetFormType() == FACT_ID) {
            TESFaction* pfaction = static_cast<TESFaction*>(powner);
            if (!static_cast<TESFaction*>(powner)->TESFaction::GetIsEvil()) {
                bkarma = true;
            }
            pactor = ReferenceProcessLists.GetActorRefInHigh(pfaction, true, false);
        }
        else {
            TESNPC* pnpc = static_cast<TESNPC*>(powner);
            if (powner) {
                float fkarma = pnpc->GetActorFloatValue(ActorValue::KARMA);
                TESActorBaseData::ALIGNMENT ealign = TESActorBaseData::GetAlignmentForKarma(fkarma);
                if (ealign != TESActorBaseData::ALIGNMENT_EVIL && ealign != TESActorBaseData::ALIGNMENT_VERY_EVIL) {
                    bkarma = true;
                }
                pactor = ReferenceProcessLists.GetActorRefInHigh(pnpc, true);
            }
        }
        if (bkarma) {
            double dkarmaMod = fKarmaModStealing.Float();
            pPlayer->RewardKarma(static_cast<int32_t>(dkarmaMod));
        }
        if (pactor && !pactor->GetIgnoreCrime()) {
            bool blinesight;
            int32_t idetection = pactor->GetDetectionLevelAgainstActor(
                false,
                this,
                blinesight,
                false,
                false,
                false,
                nullptr
            );
            if (idetection > 0) {
                Crime* pcrime;
                if (apObj) {
                    pcrime = new Crime(0, apRef, this, apObj, ainumb, apowner);
                }
                else {
                    pcrime = new Crime(0, apRef, this, nullptr, aiTotal, apowner);
                }
                float fhour = appCalendar.GetHour();
                pactor->m_Extra.AddToPlayerCrimeList(pcrime, fhour);
                pactor->IncrementMinorCrimes();
                pactor->AddFactionMinorCrime(1, true);
                ReferenceProcessLists.AddtoGlobalCrimeList(pcrime);
                pcrime->AddtoActorKnowList(pactor);
                if (bAttackOnNextTheft) {
                    bool battack = true;
                    if (iThiefCrimeStamp > 0) {
                        int32_t iday = iThiefCrimeStamp & 511;
                        if (iday != appCalendar.GetDay()) {
                            battack = false;
                            bAttackOnNextTheft = false;
                        }
                    }
                    if (battack) {
                        pactor->StartCombat(pPlayer, nullptr, true, false, false, 0, true, nullptr);
                        return;
                    }
                }
                else if (!pPlayer->GetPlayerAction(PLAYER_ACTION_Z_KEY_OBJECT)) {
                    bAttackOnNextTheft = true;
                    int32_t iMonthBits = static_cast<int32_t>(appCalendar.GetMonth()) << 9;
                    int32_t idayBits = appCalendar.GetDay() | iMonthBits;
                    iThiefCrimeStamp = appCalendar.GetYear() << 13 | idayBits;
                }
                if (bsendwarning) {
                    pactor->InitiateStealWarning(this, apowner);
                }
                else {
                    Script::SetCrimeVictim(powner);
                    SetDialogueItemTarget(pcrime->pcriminal);
                    TESTopic* pTopic = TESTopic::GetTopic(DT_COMBAT, 9);
                    pactor->pCurrentProcess->ProcessGreet(pactor, pTopic, false, false, true, false);
                    Script::SetCrimeVictim(nullptr);
                }
            }
        }
    }
}

void Actor::PostAnimBlock() {
    Animation* panim = GetAnimation();
    BSAnimGroupSequence* pseq = panim->GetCurrentSequence(AGS_LEFT_ARM);
    if (!pseq || pseq->GetState() == NiControllerSequence::kAnimating) {
        bool bstartTheBlock = bBlockPostAnim;
        ANIMATION_ACTION eAnimAction = GetAnimAction();
        if (eAnimAction == ANIM_ACTION_NONE) {
            bstartTheBlock = true;
        }
        else if (eAnimAction == ANIM_ACTION_ATTACK_FOLLOW_THROUGH) {
            if (TESAnimGroup::IsAttackAction(panim->GetGroup(AGS_WEAPON))) {
                if (TESAnimGroup::IsPowerAttackAction(panim->GetGroup(AGS_WEAPON))) {
                    bstartTheBlock = false;
                }
                else if (panim->GetGroupAction(AGS_WEAPON) < AGA_SPECIAL_IDLE_STOP) {
                    bstartTheBlock = false;
                }
            }
            else {
                if (TESAnimGroup::IsPowerAttackAction(panim->GetGroup(AGS_LEFT_ARM))) {
                    bstartTheBlock = false;
                }
            }
        }
        if (pseq && pseq->GetAnimGroup()->GetType() == ANIM_GROUP_BLOCK_IDLE) {
            if (!bBlockPostAnim) {
                float fduration = fAnimationDefaultBlend.Float();
                uint8_t cblendFrames = pseq->GetAnimGroup()->GetBlendInFrames();
                if (cblendFrames) {
                    fduration = static_cast<double>(cblendFrames) / 30.0;
                }
                panim->ClearGroup(AGS_LEFT_ARM, fduration);
                if (this == pPlayer) {
                    pPlayer->GetAnimation(true)->ClearGroup(AGS_LEFT_ARM, fduration);
                }
                SetAnimAction(ANIM_ACTION_NONE, nullptr);
                if (GetCombatController()) {
                    char cblocking = bBlockPostAnim;
                    QCombatController()->SetIsBlocking(cblocking);
                }
            }
        }
        else {
            if (bBlockPostAnim && bstartTheBlock) {
                if (!pCurrentProcess->GetCurrentWeapon() ||
                    static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj)
                        ->TESObjectWEAP::IsMeleeWeapon()) {
                    ANIMATION_ACTION eAction = GetAnimAction();
                    if (eAction != ANIM_ACTION_NONE && eAction != ANIM_ACTION_ATTACK_FOLLOW_THROUGH) {
                        bstartTheBlock = false;
                    }
                }
            }
            if (bBlockPostAnim && bstartTheBlock) {
                uint16_t sanimGroup = GetAnimGroup(ANIM_GROUP_BLOCK_IDLE, nullptr, false, nullptr);
                if (panim->GetGroup(AGS_LEFT_ARM) == sanimGroup) {
                    SetAnimAction(ANIM_ACTION_BLOCK, panim->GetCurrentSequence(AGS_LEFT_ARM));
                    if (GetCombatController()) {
                        char cblocking = bBlockPostAnim;
                        QCombatController()->SetIsBlocking(cblocking);
                    }
                }
                else {
                    panim->PlayGroup(sanimGroup, ACTION_START, -1, AGS_NONE);
                    SetAnimAction(ANIM_ACTION_BLOCK, panim->GetCurrentSequence(AGS_LEFT_ARM));
                    StartAnimOn1stPerson(sanimGroup, ACTION_START);
                    if (GetCombatController()) {
                        char cblocking = bBlockPostAnim;
                        QCombatController()->SetIsBlocking(cblocking);
                    }
                }
            }
        }
    }
    else if (GetCombatController()) {
        char cblocking = GetBlocked();
        QCombatController()->SetIsBlocking(cblocking);
    }
}

// The player runs the block post-animation action immediately; any other actor queues it on its process. Without an
// animation the combat controller takes the actor's blocking state instead.
bool Actor::SetBlock(bool abBlock) {
    if (GetAnimation()) {
        bBlockPostAnim = abBlock;
        if (this == pPlayer) {
            PostAnimBlock();
        }
        else {
            GetCurrentAIProcess()->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_BLOCK);
        }
        return true;
    }
    if (GetCombatController()) {
        bool bBlocking = GetBlocked();
        QCombatController()->SetIsBlocking(bBlocking);
    }
    return false;
}

// The last hit the actor dealt, from its process; the process is fetched twice.
HitData* Actor::GetLastAttackHitData() {
    if (GetCurrentAIProcess()) {
        return GetCurrentAIProcess()->GetLastAttackHitData();
    }
    return nullptr;
}

HitData* Actor::GetLastHitData() {
    BaseProcess* lpProcess = GetCurrentAIProcess();
    if (lpProcess) {
        return lpProcess->GetLastHitData();
    }
    return nullptr;
}

uint32_t Actor::GetMoveMode() {
    if (pActorMover) {
        return pActorMover->GetMoveMode();
    }
    return 0;
}

float Actor::GetRadius() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        return pCurrentProcess->GetRadius(static_cast<CachedValuesOwner*>(this));
    }
    return CalculateCachedRadius();
}

// The swim bit of the mover's move mode (0x800) becomes bSwimming.
void Actor::SetSwimming() {
    bSwimming = (GetMoveMode() & 0x800) != 0;
}

void Actor::TriggerPain(bool abVoice, bool abBlur) {
    if (!IsDead(false) && !IsKnockedOut() && !IsReanimated() && GetCurrentAIProcess()) {
        if (!GetCurrentAIProcess()->GetProcessLevel()) {
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                TaskQueueInterface::GetInstance()->QueueActorTriggerPain(this, abVoice, abBlur);
            }
            else {
                double dvoiceTimer;
                HighProcess* pHighProcess = static_cast<HighProcess*>(GetCurrentAIProcess());
                if (!(static_cast<double>(fLastUpdate) < pHighProcess->GetVoiceTimer()) ||
                    (dvoiceTimer = pHighProcess->GetVoiceTimer(), !(fLastUpdate > dvoiceTimer - fPainDelay.Float()))) {
                    float fnextVoiceTime = fLastUpdate + fPainDelay.Float();
                    pHighProcess->SetVoiceTimer(fnextVoiceTime);
                    if (this == pPlayer) {
                        if (abBlur && !pVATS.GetMode()) {
                            float fstrength = fGetHitPainMult.Float();
                            TESImageSpaceModifier* pGetHit = TESImageSpaceModifier::GetGetHit();
                            ImageSpaceModifierInstanceForm::Trigger(pGetHit, fstrength, nullptr);
                        }
                        if (!Interface::IsInMenuMode()) {
                            Controls* pControls = appMain->GetControls();
                            int32_t iduration = static_cast<int32_t>(fRumblePainTime.Float() * 1000.0);
                            float frightMotorStrength = fRumblePainStrength.Float();
                            pControls->Rumble(
                                fRumblePainStrength.Float(),
                                frightMotorStrength,
                                iduration,
                                0,
                                CONSTANT,
                                false,
                                false
                            );
                        }
                    }
                    if (abVoice) {
                        pCombatDialogueManager->StartDialogue(
                            this,
                            static_cast<TESObjectREFR*>(nullptr),
                            DT_COMBAT,
                            2,
                            false,
                            static_cast<CombatController*>(nullptr)
                        );
                    }
                }
            }
        }
    }
}

TESPackage* Actor::GetPackageSetAsPcurrent() {
    TESPackage* lpPackage = nullptr;
    if (pCurrentProcess) {
        lpPackage = pCurrentProcess->GetCurrentPackage();
    }
    if (lpPackage && lpPackage->IsInterruptPackage()) {
        lpPackage = GetExtra()->GetPackageExtra();
    }
    return lpPackage;
}

TESObjectREFR* Actor::GetCurrentPackageTarget() {
    if (pCurrentProcess) {
        return pCurrentProcess->CurrentPackage.pTarget;
    }
    return nullptr;
}

void Actor::SetCurrentTarget(TESObjectREFR* apRef) {
    if (pCurrentProcess) {
        pCurrentProcess->SetTarget(apRef);
    }
}

PackageLocation* Actor::GetCurrentPackageLocation() {
    if (pCurrentProcess) {
        TESPackage* lpPackage = pCurrentProcess->GetPackageThatIsRunning();
        if (lpPackage) {
            return lpPackage->GetLocation();
        }
    }
    return nullptr;
}

void Actor::SetCachedSitSleepState(SIT_SLEEP_STATE aeState) {
    cCurrentSitSleepState = aeState;
}

void Actor::SetVisibilityMask(bool abVal, int32_t aiVisMask) {
    if (abVal) {
        iVisFlags |= aiVisMask;
    }
    else {
        iVisFlags &= ~aiVisMask;
    }
}

void Actor::SetWalkRunFlagDirty() {
    if (pCurrentProcess) {
        pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_RUN_SPEED);
        pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_WALK_SPEED);
    }
}

void Actor::ResetDamagePerSecond() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_WEAPON_DPS);
    }
}

void Actor::ResetCachedWeaponCondition() {
    fCurrentWeaponCondition = -1.0f;
}

void Actor::ResetRadiationResistanceMult() {
    fRadiationResistanceMult = -1.0f;
}

void Actor::ResetCachedGunSkill() {
    fGunSkillGun = -1.0f;
    fGunSkillHUD = -1.0f;
    fGunSkillActor = -1.0f;
    fGunSkillVATS = -1.0f;
}

void Actor::StopMoving() {
    pActorMover->StopMoving();
}

void Actor::ClearLookAtTarget() {
    pActorMover->ClearLookAtTarget();
}

TESCombatStyle* Actor::GetCombatStyle() {
    TESCombatStyle* pStyle = GetExtra()->GetCombatStyle();
    if (pStyle) {
        return pStyle;
    }
    if (pCurrentProcess && pCurrentProcess->GetCurrentPackage()) {
        pStyle = pCurrentProcess->GetCurrentPackage()->GetCombatStyle();
    }
    if (pStyle) {
        return pStyle;
    }
    return GetActorBase()->GetCombatStyle();
}

void Actor::ClearProcessItems() {
    if (GetCurrentAIProcess()) {
        GetCurrentAIProcess()->SetCurrentWeapon(nullptr, nullptr, false);
        GetCurrentAIProcess()->SetCurrentAmmo(nullptr);
    }
}

// A leg is crippled and the actor does not ignore crippled limbs.
bool Actor::IsCrippled() {
    ActorValueOwner* pOwner = this;
    return (pOwner->GetActorFloatValue(ActorValue::LEFT_MOBILITY_CONDITION) == 0.0 ||
            pOwner->GetActorFloatValue(ActorValue::RIGHT_MOBILITY_CONDITION) == 0.0) &&
        pOwner->GetActorFloatValue(ActorValue::IGNORE_CRIPPLED_LIMBS) == 0.0;
}

void Actor::AddFollower(Actor* apFollower) {
    GetExtra()->AddFollower(apFollower);
    AddChange(CHANGE_ACTOR_EXTRA_PACKAGE_DATA);
}

// The agility-scaled reload speed, adjusted by the perks for the current weapon, set on the animations.
void Actor::SetReloadSpeed() {
    float fSpeed = 1.0f;
    float fAgility = static_cast<ActorValueOwner*>(this)->GetActorFloatValue(ActorValue::AGILITY);
    fAgility = fAgility - fAgilityReloadBase.Float();
    fSpeed = fAgility * fAgilityReloadModifier.Float() + fSpeed;
    TESForm* pWeapon = nullptr;
    if (GetCurrentAIProcess() && GetCurrentAIProcess()->GetCurrentWeapon()) {
        pWeapon = GetCurrentAIProcess()->GetCurrentWeapon()->GetContainerObject();
    }
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::RELOAD_SPEED_ENTRY, this, pWeapon, &fSpeed);
    if (this == pPlayer) {
        if (pPlayer->GetAnimation(true)) {
            pPlayer->GetAnimation(true)->SetReloadModifier(fSpeed);
        }
        if (pPlayer->GetAnimation(false)) {
            pPlayer->GetAnimation(false)->SetReloadModifier(fSpeed);
        }
    }
    else if (GetAnimation()) {
        GetAnimation()->SetReloadModifier(fSpeed);
    }
}

// As SetReloadSpeed, for the equip speed (the same agility settings).
void Actor::SetEquipSpeed() {
    float fSpeed = 1.0f;
    float fAgility = static_cast<ActorValueOwner*>(this)->GetActorFloatValue(ActorValue::AGILITY);
    fAgility = fAgility - fAgilityReloadBase.Float();
    fSpeed = fAgility * fAgilityReloadModifier.Float() + fSpeed;
    TESForm* pWeapon = nullptr;
    if (GetCurrentAIProcess() && GetCurrentAIProcess()->GetCurrentWeapon()) {
        pWeapon = GetCurrentAIProcess()->GetCurrentWeapon()->GetContainerObject();
    }
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::EQUIP_SPEED_ENTRY, this, pWeapon, &fSpeed);
    if (this == pPlayer) {
        if (pPlayer->GetAnimation(true)) {
            pPlayer->GetAnimation(true)->SetEquipSpeed(fSpeed);
        }
        if (pPlayer->GetAnimation(false)) {
            pPlayer->GetAnimation(false)->SetEquipSpeed(fSpeed);
        }
    }
    else if (GetAnimation()) {
        GetAnimation()->SetEquipSpeed(fSpeed);
    }
}

// Returns 1 when the actor is within fAIMaxHeadTrackDistanceFromPC of the player.
int32_t Actor::CheckHighDetail() {
    float fDistance = GetDistanceFromPoint(pPlayer->GetLocationOnReference());
    return fAIMaxHeadTrackDistanceFromPC.Float() >= fDistance;
}

void Actor::UpdateCurrentDebugGeometry() {}

void Actor::DropAddOnObjects() {
    NiPoint3 kangles;
    NiPoint3 point;
    ItemChange* pweaponItem = pCurrentProcess->GetCurrentWeapon();
    TESObjectWEAP* pboundWeapon = pCurrentProcess->GetLastBoundWeapon();
    if (pweaponItem &&
        !static_cast<TESObjectWEAP*>(pweaponItem->pContainerObj)->TESObjectWEAP::GetCantDrop() &&
        !static_cast<TESObjectWEAP*>(pweaponItem->pContainerObj)->TESObjectWEAP::IsEmbeddedWeapon() &&
        IsWeaponDrawn()) {
        if (!pweaponItem->pExtraObjectList->m_item->GetCanNotWear() && pweaponItem->pContainerObj != pboundWeapon) {
            EnchantmentItem* penchant = TESEnchantableForm::GetFormEnchanting(pweaponItem->pContainerObj);
            bool brefreshEnchant = penchant != nullptr;
            BipedAnim* pbiped = GetBiped();
            NiNode* pnode = pCurrentProcess->GetWeaponBone(pbiped);
            if (pnode) {
                point = pnode->GetWorldTranslate();
                pnode->GetWorldTransform().m_Rotate.ToEulerAnglesXYZ(kangles.x, kangles.y, kangles.z);
                TESObjectREFR* pdropref = DropObject(
                    pweaponItem->pContainerObj,
                    pweaponItem->pExtraObjectList->m_item,
                    1,
                    &point,
                    &kangles
                );
                TESObjectREFR::SetDroppedItemData(this, pdropref);
                if (brefreshEnchant) {
                    pCurrentProcess->SetShouldRefreshMagicShadersEffects(true, false, false);
                }
            }
            if (GetCombatController()) {
                QCombatController()->Disarmed(nullptr);
            }
        }
    }
}

uint16_t
Actor::GetAnimGroup(ANIM_GROUP_ENUM aeGroup, ItemChange* apWeaponItem, bool abCheckHandToHand, Animation* apAnim) {
    ItemChange* picWeapon;
    Animation* panim = apAnim;
    if (!apAnim) {
        panim = GetAnimation();
    }
    if (!panim) {
        return 255;
    }
    uint16_t smoveMode = GetMoveMode();
    ANIM_GROUP_MOVE_ENUM eanimMove = AGM_NONE;
    ANIM_GROUP_WEAPON_ENUM eanimWeapon = AGW_NONE;
    if (apWeaponItem == reinterpret_cast<ItemChange*>(static_cast<uintptr_t>(-1))) {
        picWeapon = nullptr;
    }
    else if (apWeaponItem) {
        picWeapon = apWeaponItem;
    }
    else {
        picWeapon = pCurrentProcess->GetCurrentWeapon();
    }
    if ((smoveMode & ActorMover::MF_SWIMMING) != 0) {
        eanimMove = AGM_SNEAK;
    }
    else if ((smoveMode & ActorMover::MF_FLYING) != 0) {
        eanimMove = AGM_SWIM;
    }
    else if ((smoveMode & ActorMover::MF_SNEAKING) != 0) {
        eanimMove = AGM_WALK;
    }
    if ((!GetActorType() && (GetWantWeaponDrawn() || IsInCombat())) ||
        (pCurrentProcess->GetWeaponDrawn() ||
         ((this == pPlayer && pPlayer->GetInventoryAnimation()) ||
          ((picWeapon &&
            (apWeaponItem ||
             (pCurrentProcess->GetWeaponDrawn() || (GetWantWeaponDrawn() && aeGroup != ANIM_GROUP_BLOCK_IDLE)))) ||
           abCheckHandToHand)))) {
        if (!picWeapon || abCheckHandToHand) {
            eanimWeapon = AGW_HAND_TO_HAND;
        }
        else {
            eanimWeapon = weaponToAnimGroupWeapon[static_cast<TESObjectWEAP*>(picWeapon->pContainerObj)
                                                      ->TESObjectWEAP::GetWeaponType()];
        }
    }
    bool bpowerArmor = HasBackPackWorn() || IsPowerBodyArmorWorn();
    uint16_t sanimGroup = TESAnimGroup::AnimGroup(eanimMove, eanimWeapon, aeGroup, bpowerArmor);
    return panim->PickBestAnimation(sanimGroup, false);
}

void Actor::SetAnimAction(ANIMATION_ACTION aeAction, BSAnimGroupSequence* apAnimSeq) {
    ItemChange* pweaponItem;
    uint16_t sgroup;
    TESObjectWEAP* pweapon;
    uint8_t bclearBlock;
    Animation* pAnimation = Actor::GetAnimation();
    if (GetAnimAction() != 4 &&
        pAnimation &&
        pAnimation->GetCurrentSequence(AGS_LEFT_ARM) &&
        aeAction != ANIM_ACTION_BLOCK &&
        (!apAnimSeq || animGroupInfo[apAnimSeq->GetAnimGroup()->GetType()].eSectionLayer != AGS_LEFT_ARM)) {
        bclearBlock = 1;
        if (pCurrentProcess->GetCurrentWeapon()) {
            pweaponItem = pCurrentProcess->GetCurrentWeapon();
            pweapon = static_cast<TESObjectWEAP*>(pweaponItem->ItemChange::GetItemChangeObject());
        }
        else {
            pweapon = nullptr;
        }
        if ((aeAction == ANIM_ACTION_ATTACK ||
             (aeAction > ANIM_ACTION_ATTACK_WAIT_FOR_EJECT && aeAction <= ANIM_ACTION_ATTACK_THROW_ATTACH)) &&
            pweapon &&
            !pweapon->IsMeleeWeapon()) {
            bclearBlock = 0;
        }

        if (bclearBlock) {
            sgroup = pAnimation->GetGroup(AGS_LEFT_ARM);
            if (TESAnimGroup::GetType(sgroup) == 170) {
                pAnimation->ClearGroup(AGS_LEFT_ARM, 0.0);
                if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
                    pPlayer->GetAnimation(true)->ClearGroup(AGS_LEFT_ARM, 0.0);
                }
            }
        }
    }

    if (pCurrentProcess) {
        pCurrentProcess->SetAnimAction(aeAction, apAnimSeq);
    }
}

// The player always uses ammo; other actors use it with weapons flagged for NPC ammo use, or as teammates with
// playable weapons.
bool Actor::ShouldWeaponUseAmmo(TESObjectWEAP* apWeapon) {
    if (this == pPlayer) {
        return true;
    }
    if (!apWeapon) {
        return false;
    }
    if (apWeapon->GetNPCsUseAmmo()) {
        return true;
    }
    if (!apWeapon->GetIsPlayable()) {
        return false;
    }
    return IsPlayerTeammate();
}

// With 3D, a dying, dead, essential-down or early-reanimating (state below 30) actor keeps its havok data when the life
// state is saved; so does any actor being knocked about.
bool Actor::CheckShouldSaveHavokData(uint32_t aiFlags) {
    if (!TESObjectREFR::Get3D()) {
        return false;
    }
    if (aiFlags & CHANGE_ACTOR_LIFESTATE) {
        if (GetLifeState() == ACTOR_LIFE_STATE_DYING ||
            GetLifeState() == ACTOR_LIFE_STATE_DEAD ||
            GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
            return true;
        }
        if (GetLifeState() == ACTOR_LIFE_STATE_REANIMATE) {
            for (BSSimpleList<ActiveEffect*>* pIter = static_cast<MagicTarget*>(this)->GetActiveEffectList();
                 pIter && !pIter->IsEmpty();
                 pIter = pIter->GetNext()) {
                ReanimateEffect* pReanimate = dynamic_cast<ReanimateEffect*>(pIter->GetItem());
                if (pReanimate && pReanimate->GetState() < 30) {
                    return true;
                }
            }
        }
    }
    if (GetCurrentAIProcess() &&
        (GetCurrentAIProcess()->GetKnockState() == KNOCK_STATE_EXPLODE ||
         GetCurrentAIProcess()->GetKnockState() == KNOCK_STATE_OUT ||
         GetCurrentAIProcess()->GetKnockState() == KNOCK_STATE_EXPLODE_LEAD_IN ||
         GetCurrentAIProcess()->GetKnockState() == KNOCK_STATE_OUT_LEAD_IN ||
         GetCurrentAIProcess()->GetKnockState() == KNOCK_STATE_QUEUED)) {
        return true;
    }
    return false;
}

uint32_t Actor::GetPackageSaveFlags(TESPackage* apPackage) {
    return 0;
}

// Armour, weapons, ammo and projectiles go through the process's equip queue when it is middle-high or high, the
// player is not in menu mode and no save is loading; otherwise, and for books, ingredients and aid, they are equipped
// now.
void Actor::QueueEquipObject(
    TESBoundObject* apObject,
    int32_t aiNumber,
    ExtraDataList* apExtra,
    bool abNow,
    bool abForceEquip,
    bool abPlayPickUpSounds
) {
    switch (apObject->GetFormType()) {
        case ARMO_ID:
        case WEAP_ID:
        case AMMO_ID:
        case PROJ_ID:
            if (pCurrentProcess &&
                pCurrentProcess->GetProcessLevel() <= PROCESS_TYPE_MIDDLE_HIGH &&
                !(this == pPlayer && Interface::IsInMenuMode()) &&
                !pSaveLoadGame->GetSaveGameLoading()) {
                pCurrentProcess->AddQueuedItem(
                    this,
                    true,
                    apObject,
                    aiNumber,
                    apExtra,
                    abNow,
                    abForceEquip,
                    false,
                    false,
                    false,
                    abPlayPickUpSounds
                );
            }
            else {
                EquipObject(apObject, aiNumber, apExtra, abNow, abForceEquip, abPlayPickUpSounds);
            }
            break;
        case BOOK_ID:
        case INGR_ID:
        case ALCH_ID: EquipObject(apObject, aiNumber, apExtra, abNow, abForceEquip, abPlayPickUpSounds); break;
        default: break;
    }
}

// Advances the actor mover (a zero delta at high process takes the frame time, scaled for the VATS target and by the
// animation's time scale under turbo), then keeps a robot's or giant's movement sound playing while it moves or turns
// and stops it otherwise. The player skips both.
void Actor::UpdateActorMover(float afDelta) {
    alignas(AutoMemContext) uint8_t aucMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (aucMemContext) AutoMemContext(
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp",
        23814
    );
    if (this != pPlayer) {
        if (GetCurrentAIProcess()) {
            float fDelta = afDelta;
            if (fDelta == 0.0 && GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
                if (pVATS.GetMode() == VATS::VATS_PLAYBACK && this == pVATSTarget) {
                    double dSeconds = appTimer.GetSeconds();
                    fDelta = static_cast<float>(pVATS.GetTargetUpdateMult() * dSeconds);
                }
                else {
                    fDelta = appTimer.GetSeconds();
                }
            }
            ActorValueOwner* pOwner = this;
            if (pOwner->GetActorValue(ActorValue::TURBO) > 0 && GetAnimation()) {
                fDelta = GetAnimation()->GetGlobalTimeMultiplier() * fDelta;
            }
            pActorMover->Update(fDelta);
        }
        if (IsCreature()) {
            TESCreature* pCreature = static_cast<TESCreature*>(GetObjectReference());
            char cCreatureType = pCreature ? pCreature->GetCreatureType() : -1;
            // Creature types 6 and 7: robots and giants.
            if (cCreatureType == 6 || cCreatureType == 7) {
                TESSound* pSound = pCreature ? pCreature->PickCreatureSound(CREA_SOUND_MOVEMENT) : nullptr;
                if (pSound && Get3D()) {
                    bool bIsMoving = (IsMoving() || IsTurning()) && !IsDead(false);
                    BSSoundHandle kSound;
                    GetExtra()->GetCreatureMovementSound(kSound);
                    if (bIsMoving && !kSound.IsPlaying()) {
                        if (!kSound.IsValid()) {
                            TESSound::SOUND_DATA kData = pSound->GetData();
                            uint32_t uiFlags = TESSound::ConvertLoopFlag(kData.iFlags) | BSGameSound::SOUND_FLAG_IS_3D;
                            const char* pFilename = pSound->GetFilename();
                            kSound = BSAudio::QInstance().GetSoundHandleByFilename(pFilename, uiFlags, pSound);
                            kSound.SetAssumeSuccess(true);
                            GetExtra()->SetCreatureMovementSound(kSound);
                            kSound.SetObjectToFollow(Get3D());
                        }
                        if (!kSound.IsPlaying()) {
                            kSound.SetPosition(GetLocationOnReference());
                            kSound.Play(false);
                            GetExtra()->SetCreatureMovementSound(kSound);
                        }
                    }
                    else if (!bIsMoving && kSound.IsPlaying()) {
                        kSound.Stop();
                        GetExtra()->SetCreatureMovementSound(kSound);
                    }
                    if (IsDead(false) && kSound.IsPlaying()) {
                        kSound.Stop();
                        kSound.Release();
                        GetExtra()->SetCreatureMovementSound(kSound);
                    }
                }
            }
        }
    }
    pMemContext->~AutoMemContext();
}

void Actor::EquipObject(
    TESBoundObject* apObject,
    int32_t aiNumber,
    ExtraDataList* apExtra,
    bool abNow,
    bool abForceEquip,
    bool abPlayPickUpSounds
) {
    float fhealth = 0.0f;
    ExtraHealth* pHealth = nullptr;
    bool bdefault = false;
    bool bScript = true;
    SetWalkRunFlagDirty();
    if (apObject->GetFormType() == AMMO_ID && apObject->GetQuestObject()) {
        Script::SetActionFlag(this, apExtra, SCRIPT_ACTION_ONEQUIP);
        Script::SetActionFlag(apObject, &m_Extra, SCRIPT_ACTION_ONEQUIP);
    }
    else if (
        this == pPlayer &&
        (!pPlayer->CanPCUsePowerArmor() &&
         (apObject &&
          (apObject->GetFormType() == ARMO_ID &&
           static_cast<TESBipedModelForm*>(static_cast<TESObjectARMO*>(apObject))->TESBipedModelForm::IsPowerArmor())))
    ) {
        const char* pcMessage = sCantEquipPowerArmor.String();
        Interface::ShowMessage(
            pcMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
            nullptr,
            2.0f,
            false
        );
    }
    else if (apExtra && (apExtra->GetCanNotWear() && !abForceEquip)) {
        if (this == pPlayer) {
            const char* pcMessage = sCantEquipGeneric.String();
            Interface::ShowMessage(
                pcMessage,
                0,
                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                nullptr,
                2.0f,
                false
            );
        }
    }
    else {
        InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(this);
        int32_t inum = pinventory->GetObjectCount(apObject);
        if (apObject->GetFormType() == AMMO_ID ||
            (apObject->GetFormType() == WEAP_ID || (!apExtra || !apExtra->GetWorn(false)))) {
            TESBipedModelForm* pmodel = TESBipedModelForm::GetFormAsBipedModel(apObject);
            if (inum > 0) {
                // The extra's health and the form's health are kept in one float, 0 until set.
                if (aiNumber <= 0 && apObject->GetFormType() == AMMO_ID) {
                    aiNumber = inum;
                }
                if (apExtra) {
                    pHealth = static_cast<ExtraHealth*>(apExtra->GetExtraData(37));
                }
                if (pHealth) {
                    fhealth = pHealth->fHealth;
                }
                else {
                    fhealth = static_cast<float>(TESHealthForm::GetFormHealth(apObject));
                }
                bool bEat = false;
                switch (apObject->GetFormType()) {
                    case BOOK_ID: {
                        TESObjectBOOK* pbook = static_cast<TESObjectBOOK*>(apObject);
                        if (!IsPc() || (pbook->GetTeaches() == ActorValue::VALUE_NONE || !pbook->Read(this))) {
                            break;
                        }
                        RemoveItem(pbook, apExtra, 1, false, false, nullptr, nullptr, nullptr, true, false);
                        PlayPickUpSound(apObject, true, true);
                        return;
                    }
                    case INGR_ID: {
                        Script::SetActionFlag(this, apExtra, SCRIPT_ACTION_ONEQUIP);
                        if (apObject->GetQuestObject()) {
                            if (this == pPlayer) {
                                const char* pcMessage = sNoEatQuestItem.String();
                                Interface::ShowMessage(
                                    pcMessage,
                                    0,
                                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                    nullptr,
                                    2.0f,
                                    false
                                );
                            }
                        }
                        else {
                            Eat(apObject, apExtra, this != pPlayer);
                            bEat = true;
                            PlayPickUpSound(apObject, true, true);
                        }
                        return;
                    }
                    case ARMO_ID: {
                        if (fhealth <= 0.0) {
                            if (IsPc()) {
                                const char* pcMessage = sCantEquipBrokenItem.String();
                                Interface::ShowMessage(
                                    pcMessage,
                                    0,
                                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                    nullptr,
                                    2.0f,
                                    false
                                );
                            }
                            bScript = false;
                            break;
                        }
                        ResetArmorRating();
                        [[fallthrough]];
                    }
                    case LIGH_ID: {
                        if (apObject->GetFormType() == LIGH_ID) {
                            TESSound* psound = pDataHandler->GetSound("ITMTorchHeldEquip");
                            if (psound) {
                                BSSoundHandle handle;
                                if (this == pPlayer && Interface::IsInMenuMode()) {
                                    uint32_t uiSoundID = psound->iFormID;
                                    PlaySound(uiSoundID, false, 288u, true);
                                }
                                else {
                                    uint32_t uiSoundID = psound->iFormID;
                                    PlaySound(uiSoundID, false, 258u, true);
                                }
                            }
                        }
                        [[fallthrough]];
                    }
                    case CLOT_ID: {
                        if (CanCastWornEnchantment(apObject, apExtra)) {
                            if (aiNumber > 1) {
                                aiNumber = 1;
                            }
                            if (Actor::AddWornItem(apObject, aiNumber, apExtra, abForceEquip)) {
                                if (this == pPlayer && !abNow) {
                                    if (this == pPlayer) {
                                        pPlayer->AddQueuedEnchantment(apObject, apExtra);
                                    }
                                }
                                else {
                                    CheckEquipWornEnchantment(apObject, apExtra);
                                }
                                pmodel = TESBipedModelForm::GetFormAsBipedModel(apObject);
                                if (pmodel && pmodel->FillsBipedSlot(BO_PIP_BOY, false, nullptr)) {
                                    FOPipboyManager* pPip = Interface::GetPipboy();
                                    if (pPip) {
                                        pPip->ResetPipBoyManager();
                                    }
                                }
                            }
                            break;
                        }
                        if (this != pPlayer) {
                            break;
                        }
                        const char* pcMessage = sCanNotEquipWornEnchantment.String();
                        Interface::ShowMessage(pcMessage, 0, nullptr, nullptr, 2.0f, false);
                        return;
                    }
                    case WEAP_ID: {
                        if (aiNumber > 1 && !static_cast<TESObjectWEAP*>(apObject)->TESObjectWEAP::IsThrownWeapon()) {
                            aiNumber = 1;
                        }
                        if (fhealth > 0.0) {
                            Actor::AddWornItem(apObject, aiNumber, apExtra, abForceEquip);
                        }
                        else if (this == pPlayer) {
                            const char* pcMessage = sCantEquipBrokenItem.String();
                            Interface::ShowMessage(
                                pcMessage,
                                0,
                                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                nullptr,
                                2.0f,
                                false
                            );
                            bScript = false;
                        }
                        if (this == pPlayer) {
                            pPlayer->ResetInsufficientChargeMessage();
                            pPlayer->AmmoSwapHelper(false, true);
                        }
                        ResetArmorRating();
                        ResetCachedWeaponCondition();
                        ResetDamagePerSecond();
                        SetEquipSpeed();
                        SetReloadSpeed();
                        break;
                    }
                    case AMMO_ID: {
                        ItemChange* parrows;
                        TESObjectWEAP* pweaponForm;
                        ItemChange* pcurrWeapon = pCurrentProcess->GetCurrentWeapon();
                        if (pcurrWeapon) {
                            pweaponForm = static_cast<TESObjectWEAP*>(pcurrWeapon->pContainerObj);
                        }
                        else {
                            pweaponForm = nullptr;
                        }
                        if (pweaponForm &&
                            pweaponForm->GetWeaponType() != WEAPON_ONE_HAND_GRENADE &&
                            pweaponForm->GetWeaponType() != WEAPON_ONE_HAND_MINE &&
                            pweaponForm->GetWeaponType() != WEAPON_ONE_HAND_LUNCHBOX_MINE &&
                            pweaponForm->GetWeaponType() != WEAPON_ONE_HAND_THROWN) {
                            bool bclipMod = pcurrWeapon->HasModEffectActive(2);
                            int32_t iclipRounds = pweaponForm->GetFormClipRounds(bclipMod);
                            aiNumber = BSMIN(iclipRounds, aiNumber);
                        }

                        if (ArrowProjectile::GetArrowCount() > 0) {
                            ProcessLists::DeleteAttachedArrows(apObject, aiNumber, this, 1, 1);
                        }
                        parrows = new ItemChange(apObject, aiNumber);
                        pCurrentProcess->SetCurrentAmmo(parrows);
                        break;
                    }
                    case ALCH_ID: {
                        AlchemyItem* ppotion;
                        ppotion = static_cast<AlchemyItem*>(apObject);
                        bEat = true;
                        if (!apObject) {
                            break;
                        }
                        if (IsPc()) {
                            if (ppotion->GetAddictionItem()) {
                                MiscStatManager::Increment(MiscStatManager::MS_CHEMS_TAKEN);
                            }
                            else if (ppotion == BGSDefaultObjectManager::GetDefaultObject(DO_STIMPAK)) {
                                MiscStatManager::Increment(MiscStatManager::MS_STIMPAKS_TAKEN);
                            }
                            else if (ppotion == BGSDefaultObjectManager::GetDefaultObject(DO_RADAWAY)) {
                                MiscStatManager::Increment(MiscStatManager::MS_RADAWAY_TAKEN);
                            }
                            else if (ppotion == BGSDefaultObjectManager::GetDefaultObject(DO_RADX)) {
                                MiscStatManager::Increment(MiscStatManager::MS_RADX_TAKEN);
                            }
                            else if (ppotion == BGSDefaultObjectManager::GetDefaultObject(DO_DOCTOR_BAG)) {
                                MiscStatManager::Increment(MiscStatManager::MS_DOCTORBAG_USED);
                            }
                            else if (ppotion->IsWater()) {
                                MiscStatManager::Increment(MiscStatManager::MS_WATER_CONSUMED);
                            }
                        }
                        if (ppotion->CanBePoison()) {
                            if (this == pPlayer) {
                                pPlayer->CheckPoisonWeapon(ppotion);
                                return;
                            }
                            break;
                        }
                        TESChallengeManager::UpdateChallenge(CHALLENGE_TYPE_USE_ITEM, 1, ppotion, nullptr, 0, 0, 0);
                        TESChallengeManager::UpdateChallenge(
                            CHALLENGE_TYPE_USE_ITEM_LIST,
                            1,
                            ppotion,
                            nullptr,
                            0,
                            0,
                            0
                        );
                        Script::SetActionFlag(this, apExtra, SCRIPT_ACTION_ONEQUIP);
                        if (DrinkPotion(ppotion, apExtra, this != pPlayer)) {
                            PlayPickUpSound(apObject, true, bEat);
                        }
                        return;
                    }
                    case PROJ_ID: {
                        Actor::AddWornItem(apObject, aiNumber, apExtra, abForceEquip);
                        pPlayer->ResetInsufficientChargeMessage();
                        ResetArmorRating();
                        break;
                    }
                    default: {
                        bdefault = true;
                        break;
                    }
                }
                if (bdefault && this == pPlayer) {
                    const char* pcMessage = sCantEquipGeneric.String();
                    Interface::ShowMessage(
                        pcMessage,
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                        nullptr,
                        2.0f,
                        false
                    );
                }
                if (this == pPlayer && abPlayPickUpSounds) {
                    PlayPickUpSound(apObject, true, bEat);
                }
                if (!pSaveLoadGameOLD->GetLoading() && bScript) {
                    Script::SetActionFlag(this, apExtra, SCRIPT_ACTION_ONEQUIP);
                    Script::SetActionFlag(apObject, &m_Extra, SCRIPT_ACTION_ONEQUIP);
                }
            }
        }
    }
}

bool Actor::UnEquipObject(
    TESBoundObject* apObject,
    int32_t aiNumber,
    ExtraDataList* apExtra,
    bool abForceUnequip,
    bool abEquipCalling,
    bool abPlayPickUpSounds
) {
    bool bflag = false;
    SetWalkRunFlagDirty();
    if (!apExtra || (!apExtra->GetCanNotWear() || IsDead(false))) {
        bool bleft = false;
        if (apExtra) {
            bleft = apExtra->GetWorn(true);
        }
        if (!apObject) {
            return false;
        }
        int32_t iformType = apObject->GetFormType();
        if (iformType == 24) {
            ResetArmorRating();
        }
        else if (iformType != 26) {
            if (iformType == 40) {
                bflag = RemoveWornItem(apObject, aiNumber, apExtra, bleft, abForceUnequip);
                BSSoundHandle hwpnIdle;
                m_Extra.GetWeaponIdleSound(hwpnIdle);
                if (hwpnIdle.IsPlaying()) {
                    hwpnIdle.FadeOutAndRelease(500u);
                }
                else {
                    hwpnIdle.Release();
                }
                m_Extra.SetWeaponIdleSound(hwpnIdle);
                m_Extra.GetWeaponAttackSound(hwpnIdle);
                if (hwpnIdle.IsPlaying()) {
                    hwpnIdle.FadeOutAndRelease(500u);
                }
                else {
                    hwpnIdle.Release();
                }
                m_Extra.SetWeaponAttackSound(hwpnIdle);
                if (pCurrentProcess) {
                    pCurrentProcess->SetWeaponDrawn(this, false);
                    if (pCurrentProcess->GetCurrentAmmo()) {
                        pCurrentProcess->SetCurrentAmmo(nullptr);
                    }
                    if (pCurrentProcess->GetAnimAction() == ANIM_ACTION_BLOCK) {
                        SetBlock(false);
                    }
                }
                ResetCachedWeaponCondition();
                SetEquipSpeed();
                SetReloadSpeed();
                if (this == pPlayer && abPlayPickUpSounds) {
                    PlayPickUpSound(apObject, false, false);
                }
            }
            return bflag;
        }
        Dispel(apObject, true);
        if (this == pPlayer) {
            if (Interface::IsInMenuMode() && !pPlayer->IsSleepingorResting()) {
                pPlayer->UpdateTarget(0.0f);
            }
            pPlayer->RemoveQueuedEnchantment(apObject, apExtra);
            if (abPlayPickUpSounds) {
                PlayPickUpSound(apObject, false, false);
            }
        }
        return RemoveWornItem(apObject, aiNumber, apExtra, bleft, abForceUnequip);
    }
    if (this == pPlayer) {
        const char* pcMessage = static_cast<const char*>(sCantUnequipGeneric);
        Interface::ShowMessage(pcMessage, 0, nullptr, nullptr, 2.0f, false);
    }
    return bflag;
}

bool Actor::RemoveWornItem(
    TESBoundObject* apObject,
    int32_t aiNumber,
    ExtraDataList* apExtra,
    bool ableft,
    bool abforce
) {
    bool bUnequip;
    if (!apObject) {
        return true;
    }
    bool bExtraBad = false;
    bUnequip = true;
    InventoryChanges* pinventory = m_Extra.GetContainerChanges();
    if (pinventory) {
        if (!pSaveLoadGameOLD->GetLoading()) {
            Script::SetActionFlag(this, apExtra, SCRIPT_ACTION_ONUNEQUIP);
            Script::SetActionFlag(apObject, &m_Extra, SCRIPT_ACTION_ONUNEQUIP);
        }
        bExtraBad = pinventory
                        ->RemoveWorn(&bUnequip, apObject, aiNumber, this, apExtra, ableft, abforce, nullptr, nullptr);
    }
    EnchantmentItem* penchant = TESEnchantableForm::GetFormEnchanting(apObject);
    if (penchant) {
        penchant->Unload(true);
    }
    return bExtraBad;
}

// The player's 3D comes from TESObjectREFR::Get3D called directly, anyone else's through the virtual.
void Actor::RegisterGrabIKNote() {
    BSAnimNoteReceiver* pReceiver = CreateAnimNoteReceiver();
    pReceiver->Register(BSAnimNote::BS_ANT_GRABIK, &Actor::HandleGrabIKNote);
    if (this != pPlayer) {
        NiNode* p3D = Get3D();
        NiControllerManager* pManager;
        pManager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, p3D->GetControllers())
        );
        if (pManager) {
            pManager->QNoteListener()->Register(BSAnimNote::BS_ANT_GRABIK, pReceiver);
        }
    }
    else {
        NiNode* p3D = TESObjectREFR::Get3D();
        NiControllerManager* pManager;
        pManager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, p3D->GetControllers())
        );
        if (pManager) {
            pManager->QNoteListener()->Register(BSAnimNote::BS_ANT_GRABIK, pReceiver);
        }
    }
}

// Without faction changes on either side the base faction lists are compared directly; otherwise the side(s) with
// changes are merged into a faction array first. Ranks below 0 are skipped (the rank byte is read signed). An ally
// reaction ends the search; a faction the player is an enemy of makes the reaction an enemy one, flagging
// abFromCrime except when both sides have changes.
FIGHT_REACTION Actor::GetFactionFightReaction(Actor* apActor, bool& abFromCrime) {
    FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
    BSSimpleList<FACTION_RANK*>* pmyFactionList = GetActorBase()->GetFactionList();
    BSSimpleList<FACTION_RANK*>* phisFactionList = apActor->GetActorBase()->GetFactionList();
    ExtraFactionChanges* pmyFactionChanges = GetExtra()->GetFactionChangesExtra();
    ExtraFactionChanges* phisFactionChanges = apActor->GetExtra()->GetFactionChangesExtra();
    if (!pmyFactionChanges && !phisFactionChanges) {
        for (; pmyFactionList; pmyFactionList = pmyFactionList->GetNext()) {
            FACTION_RANK* pmyFactionRank = pmyFactionList->GetItem();
            if (!pmyFactionRank || static_cast<int8_t>(pmyFactionRank->cRank) <= -1) {
                continue;
            }
            BSSimpleList<FACTION_RANK*>* phisFactionListTEMP = phisFactionList;
            if (apActor == pPlayer && pmyFactionRank->pFaction->GetPCAnEnemy()) {
                ereaction = FIGHT_REACTION_ENEMY;
                abFromCrime = true;
                continue;
            }
            for (; phisFactionListTEMP; phisFactionListTEMP = phisFactionListTEMP->GetNext()) {
                FACTION_RANK* phisFactionRank = phisFactionListTEMP->GetItem();
                if (phisFactionRank && static_cast<int8_t>(phisFactionRank->cRank) > -1) {
                    ereaction = UpdateFightReaction(ereaction, pmyFactionRank->pFaction, phisFactionRank->pFaction);
                    if (ereaction == FIGHT_REACTION_ALLY) {
                        return ereaction;
                    }
                }
            }
        }
    }
    else if (pmyFactionChanges && !phisFactionChanges) {
        TESFaction* pmyFactionArray[128];
        uint32_t
            imyCount = IntegrateFactionLists(pmyFactionArray, 128, pmyFactionList, pmyFactionChanges->pFactionChanges);
        for (uint32_t i = 0; i < imyCount; ++i) {
            TESFaction* pmyFaction = pmyFactionArray[i];
            if (!pmyFaction) {
                continue;
            }
            if (apActor == pPlayer && pmyFaction->GetPCAnEnemy()) {
                ereaction = FIGHT_REACTION_ENEMY;
                abFromCrime = true;
                continue;
            }
            for (BSSimpleList<FACTION_RANK*>* phisFactionListTEMP = phisFactionList; phisFactionListTEMP;
                 phisFactionListTEMP = phisFactionListTEMP->GetNext()) {
                FACTION_RANK* phisFactionRank = phisFactionListTEMP->GetItem();
                if (phisFactionRank && static_cast<int8_t>(phisFactionRank->cRank) > -1) {
                    ereaction = UpdateFightReaction(ereaction, pmyFaction, phisFactionRank->pFaction);
                    if (ereaction == FIGHT_REACTION_ALLY) {
                        return ereaction;
                    }
                }
            }
        }
    }
    else if (!pmyFactionChanges && phisFactionChanges) {
        TESFaction* phisFactionArray[128];
        uint32_t ihisCount = IntegrateFactionLists(
            phisFactionArray,
            128,
            phisFactionList,
            phisFactionChanges->pFactionChanges
        );
        for (; pmyFactionList; pmyFactionList = pmyFactionList->GetNext()) {
            FACTION_RANK* pmyFactionRank = pmyFactionList->GetItem();
            if (!pmyFactionRank || static_cast<int8_t>(pmyFactionRank->cRank) <= -1) {
                continue;
            }
            if (apActor == pPlayer && pmyFactionRank->pFaction->GetPCAnEnemy()) {
                ereaction = FIGHT_REACTION_ENEMY;
                abFromCrime = true;
                continue;
            }
            for (uint32_t i = 0; i < ihisCount; ++i) {
                TESFaction* phisFaction = phisFactionArray[i];
                if (!phisFaction) {
                    continue;
                }
                ereaction = UpdateFightReaction(ereaction, pmyFactionRank->pFaction, phisFaction);
                if (ereaction == FIGHT_REACTION_ALLY) {
                    return ereaction;
                }
            }
        }
    }
    else {
        TESFaction* pmyFactionArray[128];
        uint32_t
            imyCount = IntegrateFactionLists(pmyFactionArray, 128, pmyFactionList, pmyFactionChanges->pFactionChanges);
        TESFaction* phisFactionArray[128];
        uint32_t ihisCount = IntegrateFactionLists(
            phisFactionArray,
            128,
            phisFactionList,
            phisFactionChanges->pFactionChanges
        );
        for (uint32_t i = 0; i < imyCount; ++i) {
            TESFaction* pmyFaction = pmyFactionArray[i];
            if (!pmyFaction) {
                continue;
            }
            if (apActor == pPlayer && pmyFaction->GetPCAnEnemy()) {
                ereaction = FIGHT_REACTION_ENEMY;
                continue;
            }
            for (uint32_t j = 0; j < ihisCount; ++j) {
                TESFaction* phisFaction = phisFactionArray[j];
                if (!phisFaction) {
                    continue;
                }
                ereaction = UpdateFightReaction(ereaction, pmyFaction, phisFaction);
                if (ereaction == FIGHT_REACTION_ALLY) {
                    return ereaction;
                }
            }
        }
    }
    return ereaction;
}

// Merges apList1's factions with the faction changes apList2 into apFactionArray: a change adds an unlisted faction
// with a rank and clears a listed one whose rank is -1. Warns and returns aiArraySize when the array fills. cRank is
// sign-extended, so the rank compares as int8_t.
uint32_t Actor::IntegrateFactionLists(
    TESFaction** apFactionArray,
    uint32_t aiArraySize,
    BSSimpleList<FACTION_RANK*>* apList1,
    BSSimpleList<FACTION_RANK*>* apList2
) {
    uint32_t uiCount = 0;
    while (apList1) {
        FACTION_RANK* pRank = apList1->GetItem();
        if (pRank && static_cast<int8_t>(pRank->cRank) > -1) {
            apFactionArray[uiCount++] = pRank->pFaction;
            if (uiCount >= aiArraySize) {
                uint32_t uiFormID = GetFormID();
                const char* pName = TESObjectREFR::GetFullName();
                Warning(
                    WARN_AI,
                    "AI: Maximum number of factions %i reached in Actor::IntegrateFactionLists for actor '%s' (%08X)",
                    aiArraySize,
                    pName,
                    uiFormID
                );
                return aiArraySize;
            }
        }
        apList1 = apList1->GetNext();
    }
    while (apList2) {
        FACTION_RANK* pRank = apList2->GetItem();
        if (pRank) {
            int32_t iIndex = -1;
            for (uint32_t i = 0; i < uiCount; ++i) {
                if (apFactionArray[i] == pRank->pFaction) {
                    iIndex = i;
                }
            }
            if (iIndex == -1) {
                if (static_cast<int8_t>(pRank->cRank) > -1) {
                    apFactionArray[uiCount++] = pRank->pFaction;
                    if (uiCount >= aiArraySize) {
                        uint32_t uiFormID = GetFormID();
                        const char* pName = TESObjectREFR::GetFullName();
                        Warning(
                            WARN_AI,
                            "AI: Maximum number of factions %i reached in Actor::IntegrateFactionLists for actor '%s' "
                            "(%08X)",
                            aiArraySize,
                            pName,
                            uiFormID
                        );
                        return aiArraySize;
                    }
                }
            }
            else if (static_cast<int8_t>(pRank->cRank) == -1) {
                apFactionArray[iIndex] = nullptr;
            }
        }
        apList2 = apList2->GetNext();
    }
    return uiCount;
}

bool Actor::IsFleeing(bool abFleePack) {
    TESPackage* pPackage = GetCurrentPackage();
    if (!pPackage) {
        return false;
    }
    if (pPackage->GetPackType() == PACKAGE_TYPE_FLEE) {
        if (pPlayer != this) {
            return true;
        }
        BaseProcess* pProcess = pPlayer->GetCurrentAIProcess();
        pProcess->ClearCurrentPackage();
        return false;
    }
    if (abFleePack) {
        return false;
    }
    if (pPackage->GetPackType() == PACKAGE_TYPE_FLEE ||
        pPackage->GetPackType() == PACKAGE_TYPE_FLEE_NON_COMBAT ||
        (pPackage->GetPackType() == PACKAGE_TYPE_COMBAT_LOW && pCurrentProcess->GetProcedureIndexRunning() == 1)) {
        return true;
    }
    CombatController* pController = GetCombatController();
    if (pController) {
        return pController->IsFleeing();
    }
    return false;
}

bool Actor::DoesWalk() {
    return GetActorBase()->DoesWalk();
}

bool Actor::DoesFly() {
    return static_cast<TESCreature*>(GetActorBase())->DoesFly();
}

bool Actor::SwimsOnly() {
    return !DoesWalk() && DoesSwim() && !DoesFly();
}

bool Actor::BreathesAboveWater() {
    return static_cast<TESObjectREFR*>(this)->IsNpc() || !SwimsOnly();
}

TESActorBase::SEX Actor::GetSex() {
    return static_cast<TESActorBase::SEX>(GetActorBase()->GetSex());
}

bool Actor::IsAutoCalc() {
    return GetActorBase()->IsAutoCalc();
}

bool Actor::GetEssential() {
    TESActorBase* pBase = GetActorBase();
    if (GetExtra()->GetLevCreaOriginalBase()) {
        pBase = static_cast<TESActorBase*>(GetExtra()->GetLevCreaOriginalBase());
    }
    if (!pBase) {
        return false;
    }
    if (pPlayer && IsPlayerTeammate() && !pPlayer->IsHardcore()) {
        return true;
    }
    return pBase->TESActorBaseData::GetEssential() || GetRefEssential();
}

bool Actor::GetRespawn() {
    return GetActorBase()->GetRespawn();
}

// Reads the position through TESObjectREFR's non-virtual getter, not through Actor's override.
void Actor::SetEditorLocation() {
    EditorLocCoord = *QLocationOnReference();
    fEditorLocZRot = GetHeading(false);
    if (GetParentCell() && GetParentCell()->GetInterior()) {
        pEditorLocForm = GetParentCell();
    }
    else {
        pEditorLocForm = GetWorldSpace();
    }
}

void Actor::SetEditorLocation(TESWorldSpace* apWorld, TESObjectCELL* apInterior, const NiPoint3& aCoord, float afZRot) {
    EditorLocCoord = aCoord;
    fEditorLocZRot = afZRot;
    if (apInterior && apInterior->GetInterior()) {
        pEditorLocForm = apInterior;
    }
    else {
        pEditorLocForm = apWorld;
    }
}

int32_t Actor::GetActorBaseEnergyLevel() {
    return GetActorBase()->TESAIForm::GetEnergy();
}

// No difficulty modifier is applied here: the value is returned unchanged.
float Actor::DifficultyLevelCheckModHealth(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
    return afValue;
}

int32_t Actor::DifficultyLevelCheckModHealthI(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
    float fValue = DifficultyLevelCheckModHealth(aiWhich, static_cast<float>(aiValue), apAttacker);
    return static_cast<int32_t>(FloatFloor(fValue));
}

Actor* Actor::FindMeleeTarget() {
    ItemChange* pWeaponItem = GetCurrentAIProcess() ? GetCurrentAIProcess()->GetCurrentWeapon() : nullptr;
    TESObjectWEAP* pWeapon = pWeaponItem ? static_cast<TESObjectWEAP*>(pWeaponItem->GetContainerObject()) : nullptr;
    float fReach;
    if (pWeapon) {
        fReach = CombatFormulas::CalcWeaponReach(pWeapon->GetReach());
    }
    else {
        fReach = GetReach();
    }
    float fRadius = GetScale() * fReach;
    if (IsPc() && pVATS.GetMode() == VATS::VATS_PLAYBACK) {
        fRadius = fRadius * fVATSMeleeReachMult.Float();
    }
    return CombatUtilities::GetFacingTarget(this, fRadius);
}

// The flag is set only when the multiplier equals 0 (not for NaN).
void Actor::CheckInCombat() {
    bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
    if (pController && !pController->bhkCharacterListener::GetFlag(bhkCharacterController::NOTPUSHABLE_PERMANENT)) {
        bool bNotPushable = GetActorBaseSpeedMult() == 0.0;
        pController->bhkCharacterListener::SetFlag(bhkCharacterController::NOTPUSHABLE, bNotPushable);
    }
}

bool Actor::IsPlayingLowerBodySpecialIdle() {
    bool bResult = false;
    Animation* pAnimation = GetAnimation();
    if (!pAnimation) {
        return bResult;
    }
    ANIM_GROUP_SECTION eSection = AGS_NONE;
    if (IsAnimIdleQuequed() && pAnimation->spAnimIdleQueued) {
        eSection = pAnimation->spAnimIdleQueued->GetSection();
    }
    else if (pAnimation->SpecialIdlePlaying() && pAnimation->spAnimIdle) {
        eSection = pAnimation->spAnimIdle->GetSection();
    }

    if (eSection == AGS_MOVEMENT || eSection == AGS_SPECIAL_IDLE || eSection == AGS_WHOLE_BODY) {
        bResult = true;
    }
    return bResult;
}

void Actor::GetMissedPacks(BSSimpleList<TESPackage*>* apPacks, float afStartdate) {
    float fHour = appCalendar.GetHour();
    TESAIForm::GetMissedPackages(this, apPacks, afStartdate, fHour);
}

bool Actor::DoesSwim() {
    TESActorBase* actorBase = GetActorBase();
    return actorBase->DoesSwim();
}

bool Actor::IsRunningRunOnce() {
    return pCurrentProcess->IsRunningRunOnce();
}

void Actor::ReloadTargets(bool abReload) {
    if (this == pPlayer) {
        if (pPlayer->PlayerCharacter::GetAnimation(true)) {
            pPlayer->PlayerCharacter::GetAnimation(true)->ReloadTargets(abReload);
        }
        if (pPlayer->PlayerCharacter::GetAnimation(false)) {
            pPlayer->PlayerCharacter::GetAnimation(false)->ReloadTargets(abReload);
        }
    }
    else if (Actor::GetAnimation() && pCurrentProcess) {
        Actor::GetAnimation()->ReloadTargets(abReload);
    }
}

void Actor::UpdateWeaponConditionEffects(bool abForce) {
    if (pCurrentProcess && GetCurrentWeapon()) {
        int32_t iconditionStage = pCurrentProcess->GetWeaponCondition(this);
        if (abForce || pCurrentProcess->GetWeaponConditionStage() != iconditionStage) {
            NiNode* p3d__pmuzzleflash;
            pCurrentProcess->SetWeaponConditionStage(iconditionStage);
            MuzzleFlash* pMuzzleFlash = pCurrentProcess->GetCurrentMuzzleFlash();
            if (pMuzzleFlash) {
                BGSDestructibleObjectForm::UpdateDamageStageNodes(
                    pMuzzleFlash->GetAVObject(),
                    static_cast<uint32_t>(iconditionStage)
                );
            }
            if (this == pPlayer) {
                p3d__pmuzzleflash = static_cast<NiNode*>(pPlayer->Get3D(false));
            }
            else {
                p3d__pmuzzleflash = Get3D();
            }
            NiAVObject* pweapon3D = BSUtilities::GetObjectByName(p3d__pmuzzleflash, FixedStrings::QWeapon());
            if (pweapon3D) {
                BGSDestructibleObjectForm::UpdateDamageStageNodes(pweapon3D, static_cast<uint32_t>(iconditionStage));
            }
            // Passes the weapon node found above, not the backpack, to UpdateDamageStageNodes.
            if (BSUtilities::GetObjectByName(p3d__pmuzzleflash, "Backpack")) {
                BGSDestructibleObjectForm::UpdateDamageStageNodes(pweapon3D, static_cast<uint32_t>(iconditionStage));
            }
            if (this == pPlayer) {
                NiNode* p3d = static_cast<NiNode*>(pPlayer->Get3D(true));
                pweapon3D = BSUtilities::GetObjectByName(p3d, FixedStrings::QWeapon());
                if (pweapon3D) {
                    BGSDestructibleObjectForm::UpdateDamageStageNodes(
                        pweapon3D,
                        static_cast<uint32_t>(iconditionStage)
                    );
                }
                // As above: the weapon node goes to UpdateDamageStageNodes when the backpack is found.
                if (BSUtilities::GetObjectByName(p3d, "Backpack")) {
                    BGSDestructibleObjectForm::UpdateDamageStageNodes(
                        pweapon3D,
                        static_cast<uint32_t>(iconditionStage)
                    );
                }
            }
        }
    }
}

// The lighting comes from the "SkinAttachment" object; failing that, from the first non-"BIP" child node with children
// that is visual and holds per-pixel lighting; failing that, from the whole node. The index compare is unsigned.
void Actor::InitLightingPropertyPtr(NiAVObject* apObject) {
    if (!pCurrentProcess || !apObject) {
        return;
    }
    NiNode* pNode = apObject->IsNode();
    BSShaderPPLightingProperty* pLighting = nullptr;
    if (!pNode) {
        return;
    }
    NiAVObject* pObject = BSUtilities::GetObjectByName(apObject, "SkinAttachment");
    if (!pObject) {
        for (uint32_t i = 0; i < pNode->GetArrayCount() && !pObject; ++i) {
            NiAVObject* pChild = pNode->GetAt(i);
            if (!pChild) {
                continue;
            }
            const char* pName = pChild->GetName().GetHandleString();
            if (!pName || !_strnicmp(pName, "BIP", 3)) {
                continue;
            }
            NiNode* pChildNode = pChild->IsNode();
            if (pChildNode && pChildNode->GetArrayCount() && pChildNode->IsVisualObject()) {
                pLighting = FindPerPixelLighting(pChildNode);
                if (pLighting) {
                    pObject = pChildNode;
                }
            }
        }
        if (!pObject) {
            pObject = pNode;
        }
    }
    // The named object is passed to FindPerPixelLighting as a node without a type check.
    if (!pLighting) {
        pLighting = FindPerPixelLighting(static_cast<NiNode*>(pObject));
    }
    pCurrentProcess->SetLightingShaderProperty(pLighting);
}

uint16_t Actor::GetLevel() {
    return GetActorBase()->TESActorBaseData::GetLevel();
}

void Actor::InitiateBackUpPackage() {
    TESObjectCELL* parentCell;
    PlayerCharacter* pplayer;
    void* pbackUpPackage;
    TESPackage* package;
    TESPackage* prunning = MobileObject::GetCurrentAIProcess()->GetPackageThatIsRunning();
    if (!IsRestrained() && !IsUnConscious() && (!prunning || prunning->GetPackType() != PACKAGE_TYPE_GET_UP)) {
        package = TESPackage::CreatePackage(PACKAGE_TYPE_BACKUP);
        package->SetPackType(PACKAGE_TYPE_BACKUP);
        package->SetMustReachLocation(false);
        package->SetMustComplete(true);
        pbackUpPackage = dynamic_cast<BackUpPackage*>(package);
        if (pbackUpPackage) {
            pplayer = pPlayer;
            parentCell = TESObjectREFR::GetParentCell();
            static_cast<BackUpPackage*>(pbackUpPackage)->InitializeBackUpPackage(this, parentCell, pplayer);
        }
        package->SetProcedureType(PACKAGE_PROCEDURE_BACK_UP);
        MobileObject::GetCurrentAIProcess()->ComputeLastTimeProcessed();
        PutCreatedPackage(package, true, true);
    }
}

float Actor::GetFatigue() {
    return static_cast<ActorValueOwner*>(this)->GetActorFloatValue(ActorValue::FATIGUE);
}

float Actor::GetFatiguePercentage() {
    int32_t iFatigue = static_cast<ActorValueOwner*>(this)->GetBaseActorValue(ActorValue::FATIGUE);
    if (iFatigue) {
        return static_cast<double>(static_cast<ActorValueOwner*>(this)->GetActorFloatValue(ActorValue::FATIGUE)) /
            iFatigue;
    }
    return 1.0;
}

uint8_t Actor::QUpdateLighting() {
    return bUpdateLighting;
}

void Actor::SetUpdateLighting(bool abUpdate) {
    bUpdateLighting = abUpdate;
}

// An NPC may use a playable weapon (any weapon with abInInventory); a creature only the weapons of its list.
bool Actor::CanUseWeapon(TESObjectWEAP* apWeapon, bool abInInventory) {
    if (apWeapon->GetIsPlayerOnly()) {
        return false;
    }
    if (IsNpc()) {
        if (!abInInventory && !apWeapon->GetIsPlayable()) {
            return false;
        }
    }
    else {
        BGSListForm* pWeaponList = static_cast<TESCreature*>(GetActorBase())->GetWeaponList();
        if (!pWeaponList) {
            return false;
        }
        if (!pWeaponList->GetFormList()->IsInList(apWeapon)) {
            return false;
        }
    }
    return true;
}

float Actor::GetMaxCarryWeight() {
    float fresult;
    fresult = GetActorFloatValue(ActorValue::CARRY_WEIGHT);
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::GET_MAX_CARRY_WEIGHT, this, &fresult);
    return fresult;
}

bool Actor::HasNoRumors() {
    if (!IsNpc()) {
        return true;
    }
    ExtraHasNoRumors* pextra = static_cast<ExtraHasNoRumors*>(m_Extra.GetExtraData(78));
    if (pextra) {
        return pextra->bNoRumors;
    }

    return GetActorBase()->GetNoRumors();
}

void Actor::RemoveEnemyFlagFromAllFactions() {
    for (Crime*& rpItem : m_Extra.GetPlayerCrimeList()->ItemsUntilNull()) {
        for (BSSimpleList<TESFaction*>* plist = rpItem->pListofEnemyFactions; plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            TESFaction* pfac = plist->m_item;
            pfac->SetPCAnEnemy(false);
        }
    }
}

// The cached endurance from a process with cached values, else computed now.
float Actor::GetEndurance() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        return pCurrentProcess->GetCachedEndurance(this);
    }
    return static_cast<CachedValuesOwner*>(this)->CalculateCachedEndurance();
}

bool Actor::CanRun() {
    if (GetConditionPreventsRun()) {
        return false;
    }
    if (GetAnimAction() == ANIM_ACTION_BLOCK) {
        return false;
    }
    if (this == pPlayer && pCurrentProcess && pCurrentProcess->GetIronSights()) {
        return false;
    }
    return true;
}

bool Actor::GetConditionPreventsRun() {
    return false;
}

// A fighting actor with a combat controller swims only when it fights in water or flees and the controller allows it.
bool Actor::IsAllowedToSwim() {
    if (bInWater) {
        return true;
    }
    if (IsFightingOrAlarmed()) {
        CombatController* pCombatController = GetCombatController();
        if (pCombatController) {
            if (!FightsInWater() && !pCombatController->IsFleeing()) {
                return false;
            }
            if (!pCombatController->QSwimAllowed()) {
                return false;
            }
        }
        return DoesSwim();
    }
    bool bAllowed = false;
    if (IsNpc()) {
        TESPackage* pPackage = pCurrentProcess ? pCurrentProcess->GetCurrentPackage() : nullptr;
        if (pPackage && pPackage->GetAllowsSwim()) {
            bAllowed = true;
        }
    }
    else {
        bAllowed = DoesSwim();
    }
    return bAllowed;
}

void Actor::RestoreActorValue(ActorValue::Index aiWhich, float afAmount) {
    if ((afAmount > 0.0 && GetDamageModifier(aiWhich) < 0.0) ||
        (ActorValue::IsFlagSet(aiWhich, ActorValue::DAMAGE_IS_POSITIVE) && afAmount < 0.0)) {
        DamageModActorValue(aiWhich, afAmount, static_cast<Actor*>(nullptr));
    }
}

double Actor::GetHealRate() {
    CachedValuesOwner* pOwner;
    if (!pCurrentProcess || !pCurrentProcess->HasCachedValues()) {
        return CalculateCachedHealingRate();
    }
    if (this) {
        pOwner = static_cast<CachedValuesOwner*>(this);
    }
    else {
        pOwner = nullptr;
    }
    return pCurrentProcess->GetHealRate(pOwner);
}

bool Actor::ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS aeFalloutType) {
    TESPackage* pPackage = pCurrentProcess->GetCurrentPackage();
    bool bResult = false;
    if (pPackage) {
        if (!pPackage->GetSelectiveFalloutBehavior()) {
            return false;
        }
        switch (aeFalloutType) {
            case FALLOUT_BEHAVIORS_HELLOS_TO_PLAYER: {
                bResult = !pPackage->GetFOEnableHellosToPlayer();
                break;
            }
            case FALLOUT_BEHAVIORS_RANDOM_CONVERSATIONS: {
                bResult = !pPackage->GetFOEnableRandomConversations();
                break;
            }
            case FALLOUT_BEHAVIORS_OBSERVE_COMBAT_BEHAVIOR: {
                bResult = !pPackage->GetFOEnableObserveCombatBehavior();
                break;
            }
            case FALLOUT_BEHAVIORS_REACTION_TO_PLAYER_ACTIONS: {
                bResult = !pPackage->GetFOEnableReactionToPlayerActions();
                break;
            }
            case FALLOUT_BEHAVIORS_FRIENDLY_FIRE_COMMENTS: {
                bResult = !pPackage->GetFOEnableFriendlyFireComments();
                break;
            }
            case FALLOUT_BEHAVIORS_AGGRO_RADIUS_BEHAVIOR: {
                bResult = !pPackage->GetFOEnableAggroRadiusBehavior();
                break;
            }
            case FALLOUT_BEHAVIORS_IDLE_CHATTER: {
                bResult = !pPackage->GetFOEnableIdleChatterBehavior();
                break;
            }
            case FALLOUT_BEHAVIORS_AVOID_RADIATION: {
                bResult = !pPackage->GetFOEnableAvoidRadiationBehavior();
                break;
            }
            default: {
                return bResult;
            }
        }
    }
    return bResult;
}

float Actor::GetActorBaseSpeedMult() {
    return static_cast<float>(GetBaseActorValue(ActorValue::SPEED_MULT));
}

int32_t Actor::GetAssistance() {
    CachedValuesOwner* pOwner;
    if (!pCurrentProcess || !pCurrentProcess->HasCachedValues()) {
        return CalculateCachedActorAssistance();
    }
    if (this) {
        pOwner = static_cast<CachedValuesOwner*>(this);
    }
    else {
        pOwner = nullptr;
    }
    return pCurrentProcess->GetAssistance(pOwner);
}

// The player's crime list makes the actor angry unless its current package is a steal or pickpocket warning.
bool Actor::IsAngryWithPlayer() {
    BSSimpleList<Crime*>* pCrimeList = GetExtra()->GetPlayerCrimeList();
    if (!pCrimeList || pCrimeList->IsEmpty()) {
        return false;
    }
    if (!GetCurrentAIProcess()) {
        return true;
    }
    TESPackage* pPackage = GetCurrentAIProcess()->GetCurrentPackage();
    return !(
        pPackage &&
        (pPackage->GetPackType() == PACKAGE_TYPE_STEAL_WARNING ||
         pPackage->GetPackType() == PACKAGE_TYPE_PICKPOCKET_WARNING)
    );
}

bool Actor::GetShouldHelp(Actor* apActor) {
    if (!apActor) {
        return false;
    }
    if (apActor == this) {
        return true;
    }
    if (IsPlayerTeammate()) {
        if (apActor == pPlayer) {
            return !IsAngryWithPlayer();
        }

        return (!apActor->IsInCombat() ||
                (!apActor->IsInCombatWithActor(pPlayer) && !apActor->IsInCombatWithActor(this))) &&
            pPlayer->GetShouldHelp(apActor);
    }

    if (apActor->IsPlayerTeammate()) {
        return GetShouldHelp(pPlayer);
    }

    if (apActor->GetActorAggression() == 3) {
        return false;
    }

    bool bIsEnemy = false;
    int32_t iAssistance = GetAssistance();
    FIGHT_REACTION eReaction = GetFactionFightReaction(apActor, bIsEnemy);
    if (iAssistance == 1) {
        return eReaction == FIGHT_REACTION_ALLY;
    }
    if (iAssistance == 2 && (eReaction == FIGHT_REACTION_FRIEND || eReaction == FIGHT_REACTION_ALLY)) {
        return true;
    }
    return false;
}

bool Actor::IsCombatGroupMember(Actor* apActor) {
    return pCurrentCombatMemberArray &&
        pCurrentCombatMemberArray->Find(apActor, 0, &BSSimpleArray<Actor*>::DefaultEqualFunctionByAddress) != -1;
}

float Actor::GetDamagePerSecond() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        CachedValuesOwner* pOwner;
        if (this) {
            pOwner = static_cast<CachedValuesOwner*>(this);
        }
        else {
            pOwner = nullptr;
        }
        return pCurrentProcess->GetDamagePerSecond(pOwner);
    }

    return CalculateCachedWeaponDPS();
}

void Actor::CombatHit(Actor* apTarget, bool abPowerAttack, Projectile* apProjectile, char cMeleeEffect) {
    if (apTarget && (!apTarget->GetGhost() && !apTarget->GetDisabledOrDeleted())) {
        VATS_COMMAND* pvatsAction;
        WEAPON_TYPE ecurrentWeaponType;
        TESObjectWEAP* pcurrentWeapon;
        ItemChange* pcurrentWeaponItem;
        bool bFriendlyFire = false;
        if (this != pPlayer && GetCombatController()) {
            bFriendlyFire = !GetCombatController()->IsActoraCombatTarget(apTarget);
        }

        if (pCurrentProcess) {
            pcurrentWeaponItem = pCurrentProcess->GetCurrentWeapon();
        }
        else {
            pcurrentWeaponItem = nullptr;
        }
        if (pcurrentWeaponItem) {
            pcurrentWeapon = static_cast<TESObjectWEAP*>(pcurrentWeaponItem->pContainerObj);
        }
        else {
            pcurrentWeapon = nullptr;
        }
        if (pcurrentWeapon) {
            ecurrentWeaponType = pcurrentWeapon->GetWeaponType();
        }
        else {
            ecurrentWeaponType = WEAPON_HAND_TO_HAND;
        }
        int32_t icurrentWeaponType = ecurrentWeaponType;
        bool binsufficientCharge = false;
        bool benchanted = CheckCastWhenStrikesEnchantment(
            pcurrentWeaponItem,
            apTarget,
            apProjectile,
            binsufficientCharge
        );
        if (binsufficientCharge && IsPc()) {
            pPlayer->ShowInsufficientChargeMessage();
        }
        HitData* pdata = new HitData;
        NiPointer<HitData> spdata(pdata);
        if (IsPc() && pVATS.eMode == VATS::VATS_PLAYBACK) {
            pvatsAction = pVATS.GetCurrentAction();
        }
        else {
            pvatsAction = nullptr;
        }
        if (pvatsAction) {
            spdata->Copy(pvatsAction->pHitData);
            HitData tmpData;
            HitData::InitializeHitData(&tmpData, this, apTarget, pcurrentWeaponItem, abPowerAttack, apProjectile);
            spdata->kHitDirection = tmpData.kHitDirection;
            spdata->kHitLocation = tmpData.kHitLocation;
            if (!pvatsAction->bSuccess) {
                spdata->fHealthDamage = tmpData.fHealthDamage;
                spdata->fTotalDamage = tmpData.fTotalDamage;
                spdata->fFatigueDamage = tmpData.fFatigueDamage;
                spdata->fArmorDamage = tmpData.fArmorDamage;
                spdata->fTargetedLimbDamage = tmpData.fTargetedLimbDamage;
            }
            if (apProjectile && apProjectile->IsFlagSet(Projectile::FAKE_SHOT)) {
                spdata->pCriticalEffect = nullptr;
                spdata->fHealthDamage = 0.0f;
                spdata->fTotalDamage = 0.0f;
                spdata->fFatigueDamage = 0.0f;
                spdata->fDamageToWeapon = 0.0f;
                spdata->fArmorDamage = 0.0f;
                spdata->fTargetedLimbDamage = 0.0f;
                spdata->SetFlag(
                    static_cast<HitData::Flag>(
                        HitData::HD_CRITICAL | HitData::HD_CRITICAL_ON_DEATH | HitData::HD_FATAL |
                        HitData::HD_DISMEMBER_LIMB | HitData::HD_EXPLODE_LIMB | HitData::HD_CRIPPLE_LIMB |
                        HitData::HD_DISARM | HitData::HD_DISABLE_WEAPON
                    ),
                    0
                );
            }

            if (!spdata->pSourceRef && apProjectile) {
                spdata->pSourceRef = apProjectile;
            }

            if (apProjectile && spdata->fHealthDamage != 0.0) {
                bool bUseSplit = false;
                if (pcurrentWeaponItem) {
                    bUseSplit = pcurrentWeaponItem->HasModEffectActive(12);
                }
                int32_t inumProjectiles = apProjectile->pWeaponSource->GetNumProjectiles(bUseSplit, false, this);
                if (inumProjectiles > 1) {
                    spdata->fHealthDamage = spdata->fHealthDamage / static_cast<double>(inumProjectiles);
                    spdata->fTotalDamage = spdata->fTotalDamage / static_cast<double>(inumProjectiles);
                    spdata->fTargetedLimbDamage = spdata->fTargetedLimbDamage / static_cast<double>(inumProjectiles);
                }
                if (apProjectile->GetProjectileBase()->GetExplosion()) {
                    float fexplosionDamage = spdata->fHealthDamage - apProjectile->fDamage;
                    float fpercent = 1.0 - fexplosionDamage / spdata->fHealthDamage;
                    spdata->fHealthDamage = spdata->fHealthDamage * fpercent;
                    spdata->fTotalDamage = spdata->fTotalDamage * fpercent;
                    spdata->fTargetedLimbDamage = spdata->fTargetedLimbDamage * fpercent;
                }
            }
        }
        else {
            HitData::InitializeHitData(
                spdata.m_pObject,
                this,
                apTarget,
                pcurrentWeaponItem,
                abPowerAttack,
                apProjectile
            );
        }
        int32_t iadvance = GetAdvanceNumber();
        SetAdvanceNumber(iadvance + 1);
        SetSkillAdvanceDelayed(spdata->eAttackSkill);
        if (spdata->IsFlagSet(HitData::HD_BLOCKED)) {
            HandleBlockedAttack(spdata->fHealthDamage, spdata->fPercentBlocked, apTarget, apProjectile);
            if (spdata->IsFlagSet(HitData::HD_BLOCK_WITH_WEAPON)) {
                if (apTarget->pCurrentProcess->GetCurrentWeapon()) {
                    NiPoint3 kDefenderPos = apTarget->GetLocationOnReference();
                    NiPoint3 kAttackerPos = GetLocationOnReference();
                    TESObjectWEAP* pdefenderWeapon = static_cast<TESObjectWEAP*>(
                        apTarget->pCurrentProcess->GetCurrentWeapon()->pContainerObj
                    );
                    ImpactMixer::PlayWeaponBlock(
                        icurrentWeaponType,
                        pdefenderWeapon,
                        kAttackerPos,
                        kDefenderPos,
                        benchanted
                    );
                }
                else {
                    NiPoint3 kDefenderPos = apTarget->GetLocationOnReference();
                    NiPoint3 kAttackerPos = GetLocationOnReference();
                    ImpactMixer::PlayWeaponBlock(icurrentWeaponType, pFists, kAttackerPos, kDefenderPos, benchanted);
                }
            }
            else {
                bool bnotEnchanted = !benchanted;
                NiPoint3 kDefenderPos = apTarget->GetLocationOnReference();
                NiPoint3 kAttackerPos = GetLocationOnReference();
                ImpactMixer::PlayWeaponBlock(icurrentWeaponType, nullptr, kAttackerPos, kDefenderPos, bnotEnchanted);
            }
        }
        else {
            int32_t ihitWeaponType;
            if (pcurrentWeapon) {
                ihitWeaponType = pcurrentWeapon->GetWeaponType();
            }
            else {
                ihitWeaponType = -1;
            }
            ImpactMixer::PlayHitSound(
                this,
                spdata->fHealthDamage,
                0.0f,
                apTarget,
                ihitWeaponType,
                -1,
                -1,
                false,
                benchanted
            );
        }
        if (pcurrentWeapon && pcurrentWeaponItem) {
            MagicItem* pmagicItem;
            EnchantmentItem* penchantment = pcurrentWeapon->pFormEnchanting;
            if (penchantment) {
                pmagicItem = &static_cast<MagicItem&>(static_cast<MagicItemForm&>(*penchantment));
            }
            else {
                pmagicItem = nullptr;
            }
            if (!pmagicItem) {
                AlchemyItem* ppoison = pcurrentWeaponItem->GetPoison();
                if (ppoison) {
                    pmagicItem = &static_cast<MagicItem&>(static_cast<MagicItemObject&>(*ppoison));
                }
                else {
                    pmagicItem = nullptr;
                }
            }
            if (pmagicItem && (benchanted && !apProjectile)) {
                double dcharge = pcurrentWeaponItem->GetItemCharge();
                if (pmagicItem->GetCost(this) > dcharge) {
                    pCurrentProcess->SetShouldRefreshMagicShadersEffects(true, false, false);
                }
            }
        }

        if (pcurrentWeaponItem && pcurrentWeapon && (!apProjectile && IsPc() && !pcurrentWeapon->IsEmbeddedWeapon())) {
            if (DamageEquipment(pcurrentWeaponItem, spdata->fDamageToWeapon, false)) {
                pcurrentWeaponItem = nullptr;
            }
        }

        TESPackage* ppack = GetCurrentPackage();
        if (ppack) {
            const TESUseWeaponPackageData* ppackData = ppack->GetUseWeaponPackageData();
            if (ppackData && ppackData->GetDoNoDamage()) {
                spdata->fHealthDamage = 0.0f;
            }
        }
        if (!bFriendlyFire) {
            apTarget->HitMe(spdata.m_pObject, cMeleeEffect);
        }
    }
}

// Swings the actor's melee attack. With the 3D task thread running it is queued there. Otherwise a VATS playback attack
// uses a shot of an automatic weapon or casts the paralyzing palm perk; iShowHitVector above 1 draws the hit vector;
// the attack sets the action sound level; and an in-combat attack hits the use-weapon package's target or the melee
// target (the player hits an object when there is none), or else plays the weapon's attack fail sound and casts the
// base's touch spell when its animation is playing. The combat controller is told of the attack.
void Actor::MeleeAttack(bool abPowerAttack, bool abInCombatAttack) {
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        TaskQueueInterface::GetInstance()->QueueActorMeleeAttack(this, abPowerAttack, abInCombatAttack);
        return;
    }
    if (pVATS.GetMode() == VATS::VATS_PLAYBACK && this == pVATSAttacker) {
        TESObjectWEAP* pWeapon = pCurrentProcess->GetCurrentWeapon()
            ? static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject())
            : nullptr;
        VATS_COMMAND* pAction = pVATS.GetCurrentAction();
        if (pAction) {
            if (pWeapon && pWeapon->IsAutomaticWeapon()) {
                --pAction->cFireShots;
            }
            else if (pAction->bParalyzingPalm && !pAction->pTarget->IsParalyzed() && pAction->pTarget->IsActor()) {
                TESForm* pPerk = BGSDefaultObjectManager::GetDefaultObject(DO_PERK_PARALYSIS);
                if (pPerk) {
                    SetCurrentSpell(dynamic_cast<MagicItem*>(pPerk));
                    // The VATS target is an actor here (IsActor above).
                    Actor* pTarget = static_cast<Actor*>(pAction->pTarget);
                    SetDesiredTarget(pTarget ? static_cast<MagicTarget*>(pTarget) : nullptr);
                    if (ReleaseCast(nullptr)) {
                        MiscStatManager::Increment(MiscStatManager::MS_PARALYZING_PUNCHES);
                    }
                }
            }
        }
    }
    // The setting is compared signed.
    if (static_cast<int32_t>(static_cast<uint32_t>(iShowHitVector)) > 1) {
        NiColorA kColor(0.0f, 0.0f, 0.0f, 0.0f);
        if (IsPc()) {
            kColor = NiColorA(0.0f, 0.0f, 1.0f, 1.0f);
        }
        else {
            kColor = NiColorA(1.0f, 0.0f, 0.0f, 1.0f);
        }
        NiPoint3 kStart;
        NiPoint3 kEnd;
        NiNode* p3D = Get3D();
        if (p3D) {
            const NiPoint3* pLastPos = pCurrentProcess->GetWeaponLastPos();
            kStart = p3D->GetWorldTransform() * *pLastPos;
            NiPoint3 kWeaponPos = GetWeaponPosition(false);
            kEnd = p3D->GetWorldTransform() * kWeaponPos;
        }
        NiPoint3 kDirection;
        kDirection = kEnd - kStart;
        kDirection.Unitize();
        NiPoint3 kVector = kDirection * 800.0f;
        NiPoint3 kArrow = kVector * 0.1f;
        NiAVObject* pArrow = BSTestObjects::CreateDirArrow(kArrow, &kColor);
        pArrow->SetTranslate(kEnd);
        float fDelay = fHitVectorDelay;
        TES::pInstance->AddTempDebugObject(pArrow, fDelay);
    }
    GetCurrentAIProcess()->AttackCallback(1);
    if (IsCreature()) {
        SetActionSoundValue(static_cast<TESCreature*>(GetObjectReference())->GetSoundLevelCopy());
    }
    else {
        SetActionSoundValue(iSoundLevelSilent.UInt());
    }
    if (!abInCombatAttack) {
        return;
    }
    bool bFound = false;
    Actor* pTarget = nullptr;
    BaseProcess* pProcess = GetCurrentAIProcess();
    if (pProcess) {
        TESPackage* pPackage = pProcess->GetPackageThatIsRunning();
        if (pPackage && pPackage->GetProcedureType() == PACKAGE_PROCEDURE_USEWEAPON) {
            ActorPackageData* pData = pProcess->GetInstanceDataThatIsRunning();
            if (pData) {
                // Fetches the process again and drops the result.
                GetCurrentAIProcess();
                TESObjectREFR* pRef = static_cast<UseWeaponActorPackageData*>(pData)->GetAttackTarget();
                if (pRef && pRef->IsActor()) {
                    pTarget = static_cast<Actor*>(pRef);
                    bFound = true;
                }
            }
        }
    }
    if (!bFound) {
        pTarget = FindMeleeTarget();
        bFound = pTarget != nullptr;
    }
    if (!pTarget && IsPc() && ObjectHit(abPowerAttack)) {
        return;
    }
    if (pTarget && pTarget->GetGhost()) {
        pTarget = nullptr;
    }
    ItemChange* pWeaponItem = GetCurrentAIProcess() ? GetCurrentAIProcess()->GetCurrentWeapon() : nullptr;
    TESObjectWEAP* pWeapon = pWeaponItem ? static_cast<TESObjectWEAP*>(pWeaponItem->GetItemChangeObject()) : nullptr;
    if (pTarget) {
        uint8_t ucHitFlags = 0;
        // Weapon type 0: hand-to-hand melee.
        if (!pWeapon || pWeapon->GetWeaponType() == 0) {
            BSAnimGroupSequence* pSequence = pCurrentProcess->GetAnimActionAnimSeq();
            int32_t iType = pSequence->GetAnimGroup()->GetType();
            if (iType == ANIM_GROUP_ATTACK_6 ||
                iType == ANIM_GROUP_ATTACK_CUSTOM_4_POWER ||
                iType == ANIM_GROUP_ATTACK_CUSTOM_5_POWER) {
                ucHitFlags = 1;
            }
            else if (iType == ANIM_GROUP_ATTACK_7) {
                ucHitFlags = 2;
            }
        }
        CombatHit(pTarget, abPowerAttack, nullptr, ucHitFlags);
    }
    else {
        TESSound* pSound = pWeapon ? pWeapon->GetAttackFailSound() : nullptr;
        if (pSound) {
            // Takes the file name before the audio singleton.
            const char* pFilename = pSound->GetFilename();
            BSSoundHandle kHandle = BSAudio::QInstance().GetSoundHandleByFilename(
                pFilename,
                BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT,
                pSound
            );
            NiPoint3 kLocation = GetLocationOnReference();
            kHandle.SetPosition(kLocation.x, kLocation.y, kLocation.z);
            kHandle.SetObjectToFollow(Get3D());
            kHandle.Play(false);
        }
        TESForm* pTouchSpell = GetActorBase()->GetFormTouchSpell();
        MagicItem* pSpell = pTouchSpell ? static_cast<MagicItemForm*>(pTouchSpell) : nullptr;
        if (pSpell && pSpell->HasSelfEffect()) {
            bool bCast = false;
            int32_t iAnimation = GetActorBase()->GetFormTouchSpellAnimation();
            if (iAnimation == 0xFF) {
                bCast = true;
            }
            else if (
                GetCurrentAIProcess() &&
                GetCurrentAIProcess()->GetAnimActionAnimSeq() &&
                GetCurrentAIProcess()->GetAnimActionAnimSeq()->GetAnimGroup() &&
                GetCurrentAIProcess()->GetAnimActionAnimSeq()->GetAnimGroup()->GetType() == iAnimation
            ) {
                bCast = true;
            }
            if (bCast) {
                SetCurrentSpell(pSpell);
                SetDesiredTarget(this);
                ReleaseCast(nullptr);
            }
        }
    }
    CombatController* pCombatController = GetCombatController();
    if (pCombatController) {
        pCombatController->PerformedMeleeAttack(pTarget);
    }
}

// The weapon's reach is read from OBJ_WEAP::fReach.
NiPoint3 Actor::GetWeaponPosition(bool abWorldSpace) {
    NiTransform trans;
    NiAVObject* pav = Get3D();
    if (pav) {
        NiNode* pobj;
        ItemChange* pweapon = pCurrentProcess->GetCurrentWeapon();
        float freach = 0.0f;
        if (pweapon) {
            float fweaponReach = static_cast<TESObjectWEAP*>(pweapon->pContainerObj)->data.fReach;
            freach = CombatFormulas::CalcWeaponReach(fweaponReach);
        }
        else {
            freach = 0.0f;
        }
        if (pCurrentProcess) {
            BipedAnim* pbiped = GetBiped();
            pobj = pCurrentProcess->GetWeaponBone(pbiped);
        }
        else {
            pobj = static_cast<NiNode*>(BSUtilities::GetObjectByName(pav, bipedBoneName[1]));
        }
        if (pobj) {
            trans = pobj->GetWorldTransform();
            if (!abWorldSpace) {
                NiTransform parentTrans;
                pav->GetWorldTransform().Invert(parentTrans);
                trans = parentTrans * trans;
            }
            NiPoint3 kOffset(0.0f, freach, 0.0f);
            NiPoint3 kPos;
            kPos = trans * kOffset;
            return kPos;
        }

        return NiPoint3::ZERO;
    }

    return NiPoint3::ZERO;
}

// Casts the small hit caster along the eye vector and hits the first body it finds. A rigid body gets a knock impulse
// scaled by the weapon's weight (or the hand items', plus 3), the hit sound, the OnHit events, the impact data's decal
// (on every penetrated body of the hit material), blood effect and sounds, and a projectile or weapon reference takes
// the damage. Otherwise a non-player's hit on a living actor's character controller plays the hit sound. The player
// then fires OnTrigger on the activate reference when it is one of its cell's trap triggers. The hkVector4 impulse is
// alignas(16).
bool Actor::ObjectHit(bool abPowerAttack) {
    bool bResult = false;
    TESObjectCELL* pCell = GetParentCell();
    bhkWorld* pWorld = pCell ? pCell->GetbhkWorld() : nullptr;
    HitCaster* pHitCaster = HitCaster::GetInstance(HitCaster::SIZE_SMALL, true);
    ItemChange* pWeaponItem = GetCurrentAIProcess() ? GetCurrentAIProcess()->GetCurrentWeapon() : nullptr;
    TESObjectWEAP* pWeapon = pWeaponItem ? static_cast<TESObjectWEAP*>(pWeaponItem->GetContainerObject()) : pFists;
    // The pick length is converted as a signed integer.
    float fReach = static_cast<float>(static_cast<int32_t>(iActivatePickLength.UInt()));
    if (pWeapon) {
        switch (pWeapon->GetWeaponType()) {
            case WEAPON_HAND_TO_HAND: fReach = fObjectHitH2HReach.Float(); break;
            case WEAPON_ONE_HAND_MELEE: fReach = fObjectHitOneHandReach.Float(); break;
            case WEAPON_TWO_HAND_MELEE: fReach = fObjectHitTwoHandReach.Float(); break;
            default: fReach = fObjectHitWeaponReach.Float(); break;
        }
    }
    if (!pWorld || !pHitCaster) {
        return bResult;
    }
    NiPoint3 kEyeOrigin;
    alignas(16) NiPoint3 kEyeDirection;
    GetEyeVector(kEyeOrigin, kEyeDirection, false);
    pHitCaster->Add(pWorld);
    CFilter kFilter;
    GetCollisionFilter(&kFilter.iFilter);
    uint32_t uiGroup = kFilter.GetGroup();
    if (pHitCaster->GetShapeGroup() != uiGroup) {
        pHitCaster->SetShapeGroup(uiGroup);
    }
    if (!pHitCaster->CastShape(kEyeOrigin, kEyeDirection, fReach)) {
        return bResult;
    }
    hkpAllCdPointCollector* pCollector = pHitCaster->GetPointCollector();
    NiAVObject* pHitObject = GetAVObjectForCollidable(
        const_cast<hkpCollidable*>(pCollector->getHits()[0].m_rootCollidableB)
    );
    TESObjectREFR* pRef = pHitObject ? TESObjectREFR::FindReferenceFor3D(pHitObject) : nullptr;
    hkpRigidBody* pBody = hkpGetRigidBody(pCollector->getHits()[0].m_rootCollidableB);
    Actor* pActor = (pRef && pRef->IsActor()) ? static_cast<Actor*>(pRef) : nullptr;
    int32_t iWeaponType = pWeapon ? pWeapon->GetWeaponType() : -1;
    float fDamage = 0.0f;
    if (pWeaponItem) {
        uint16_t usGroup = GetAnimation()->GetGroup(AGS_WEAPON);
        ANIM_GROUP_ENUM eAttackType = TESAnimGroup::GetType(usGroup);
        float fMult = 1.0f;
        if (!IsCreature() && !IsSneaking() && abPowerAttack) {
            ActorValue::Index iSkill = pWeapon->GetWeaponSkill();
            int32_t iSkillValue = static_cast<ActorValueOwner*>(this)->GetClampedActorValue(iSkill);
            fMult = CombatFormulas::GetPowerAttackDamageMultiplier(iSkillValue, eAttackType);
        }
        fDamage = pWeaponItem->GetModifiedAttackDamage(static_cast<ActorValueOwner*>(this), fMult, false);
    }
    else if (IsCreature()) {
        fDamage = static_cast<float>(GetAttackDamage());
    }
    else {
        float fFatigueDamage;
        CombatFormulas::CalcHandToHandDamage(static_cast<ActorValueOwner*>(this), false, fDamage, fFatigueDamage);
    }
    if (IsPc() && pVATS.IsInVats()) {
        // The critical damage is zero-extended from 16 bits before the conversion to float.
        float fCritical = 0.0f;
        if (pWeapon) {
            fCritical = static_cast<float>(static_cast<int32_t>(pWeapon->GetCriticalDamage()));
        }
        else if (pFists) {
            fCritical = static_cast<float>(static_cast<int32_t>(pFists->GetCriticalDamage()));
        }
        fDamage = fDamage + fCritical;
    }
    if (pBody) {
        bhkEntity* pBhkBody = bhkEntity::Getbhk(pBody);
        bool bBlood = true;
        NiPoint3 kStart;
        NiPoint3 kEnd;
        NiNode* p3D = Get3D();
        if (p3D) {
            const NiPoint3* pLastPos = pCurrentProcess->GetWeaponLastPos();
            kStart = p3D->GetWorldTransform() * *pLastPos;
            NiPoint3 kWeaponPos = GetWeaponPosition(false);
            kEnd = p3D->GetWorldTransform() * kWeaponPos;
        }
        NiPoint3 kHitDirection = kEnd - kStart;
        kHitDirection.Unitize();
        NiPoint3 kHitPos;
        HK2NI(kHitPos, pCollector->getHits()[0].m_contact.getPosition());
        float fWeight;
        if (pWeapon) {
            fWeight = pWeapon->GetFormWeight(false);
        }
        else {
            ItemChange* pLeftItem = GetArmorBeingWorn(BO_LEFT_HAND);
            ItemChange* pRightItem = GetArmorBeingWorn(BO_RIGHT_HAND);
            if (pLeftItem || pRightItem) {
                fWeight = 0.0f;
                // The armour is cast to its TESWeightForm without a null test.
                if (pLeftItem) {
                    TESObjectARMO* pLeftArmor = static_cast<TESObjectARMO*>(pLeftItem->GetContainerObject());
                    fWeight = static_cast<TESWeightForm&>(*pLeftArmor).GetWeight() + fWeight;
                }
                if (pRightItem) {
                    TESObjectARMO* pRightArmor = static_cast<TESObjectARMO*>(pRightItem->GetContainerObject());
                    fWeight = static_cast<TESWeightForm&>(*pRightArmor).GetWeight() + fWeight;
                }
            }
            else {
                fWeight = 0.0f;
                bBlood = false;
            }
            if (pLeftItem) {
                delete pLeftItem;
            }
            if (pRightItem) {
                delete pRightItem;
            }
        }
        fWeight = static_cast<float>(fWeight + 3.0);
        uint32_t uiShapeKey = pCollector->getHits()[0].m_shapeKeyB;
        HavokMaterialType eMaterial = BHK_MATERIAL_STONE;
        CFilter kBodyFilter;
        pBhkBody->GetCollisionFilter(&kBodyFilter);
        if (kBodyFilter.GetLayer() == L_GROUND) {
            eMaterial = TES::pInstance->GetLandMaterialType(kHitPos);
        }
        else {
            bhkShape* pShape = pBhkBody->GetShape();
            if (pShape) {
                eMaterial = static_cast<HavokMaterialType>(pShape->GetMaterial());
                if (uiShapeKey != 0xFFFFFFFF) {
                    bhkShapeCollection* pCollection = pShape->FindShapeCollection();
                    if (pCollection) {
                        eMaterial = static_cast<HavokMaterialType>(pCollection->QSubMaterial(uiShapeKey));
                    }
                }
            }
        }
        fWeight = BSMIN(fWeight, fWeaponClutterKnockMaxWeaponMass.Float());
        NiPoint3 kImpulse = kHitDirection * fWeaponClutterKnockMult.Float() * fWeight;
        float fMass = pBody->getMass();
        // Scaled only when the minimum is greater (not when equal or unordered).
        if (fWeaponClutterKnockMinClutterMass.Float() > fMass) {
            float fMassScale = fMass / fWeaponClutterKnockMinClutterMass.Float();
            kImpulse *= fMassScale;
        }
        CFilter kBipFilter;
        pBhkBody->GetCollisionFilter(&kBipFilter);
        if (kBipFilter.QBipLayer()) {
            kImpulse *= fWeaponClutterKnockBipedScale.Float();
        }
        alignas(16) hkVector4 kHkImpulse;
        NI2HK(kHkImpulse, kImpulse);
        pWorld->MarkForWrite();
        const hkVector4& rHitPoint = pCollector->getHits()[0].m_contact.getPosition();
        pBody->applyPointImpulse(kHkImpulse, rHitPoint);
        pWorld->UnMarkForWrite();
        FakeWeaponHitSound(kHitPos, pRef, static_cast<bhkRigidBody*>(pBhkBody), eMaterial);
        if (pRef) {
            if (pActor) {
                pActor->ClearLastHitData();
            }
            // The OnHit (0x80) and OnHitWith (0x100) script events.
            Script::SetActionFlag(this, pRef->GetExtra(), SCRIPT_ACTION_ONHIT);
            if (pWeapon) {
                Script::SetActionFlag(pWeapon, pRef->GetExtra(), SCRIPT_ACTION_ONHITWITH);
            }
            else {
                Script::SetActionFlag(pFists, pRef->GetExtra(), SCRIPT_ACTION_ONHITWITH);
            }
        }
        BGSImpactData* pImpact;
        if (pWeapon) {
            pImpact = pWeapon->GetImpactDataForMaterial(eMaterial);
        }
        else {
            pImpact = pFists->GetImpactDataForMaterial(eMaterial);
        }
        BGSTextureSet* pTexSet = pImpact ? pImpact->GetDecalTextureSet() : nullptr;
        if (pTexSet) {
            NiPoint3 kNormal;
            HK2NI(kNormal, pCollector->getHits()[0].m_contact.getNormal());
            NiPoint3 kDecalDirection = kNormal;
            kDecalDirection.Unitize();
            NiPoint3 kDecalOrigin = kHitPos;
            float fMaxWidth = pImpact->GetDecalMaxWidth();
            float fMinWidth = static_cast<float>(pImpact->GetMinWidth());
            float fDecalWidth = RandomFloat(fMinWidth, fMaxWidth);
            DecalCaster* pDecalCaster = DecalCaster::GetInstance(fDecalWidth, true);
            pDecalCaster->SetCustomPickCollisions(DecalCaster::CUSTOM_PICK_ACTORS);
            pDecalCaster->SetCollision(L_BIPED, false);
            pDecalCaster->SetCollision(L_DEADBIP, false);
            TESObjectCELL* pDecalWorldCell = GetParentCell();
            bhkWorld* pDecalWorld = pDecalWorldCell ? pDecalWorldCell->GetbhkWorld() : nullptr;
            if (pDecalWorld) {
                pDecalCaster->Add(static_cast<bhkWorldM*>(pDecalWorld));
                alignas(16) uint8_t aBodiesStorage[sizeof(bhkFilteredCdBodyCollector)];
                bhkFilteredCdBodyCollector* pBodies = ::new (aBodiesStorage) bhkFilteredCdBodyCollector;
                pDecalCaster->GetPenetrations(kHitPos, *pBodies);
                int32_t iNumBodies = pBodies->getHits().getSize();
                float fRotation = RandomFloatTwoPi();
                char cSubTex = static_cast<char>(static_cast<int32_t>(std::floor(RandomFloat(0.0f, 4.0f))));
                // A signed compare, tested before the first pass.
                for (int32_t i = 0; i < iNumBodies; ++i) {
                    const hkpCollidable* pCollidable = pBodies->getHits()[i].m_rootCollidableB;
                    if (!pCollidable) {
                        continue;
                    }
                    HavokMaterialType eBodyMaterial = BHK_MATERIAL_MAX;
                    uint32_t uiBodyKey = pBodies->getHits()[i].m_shapeKeyB;
                    if (uiBodyKey != 0xFFFFFFFF) {
                        bhkShape* pBodyShape = bhkShape::Getbhk(pCollidable->getShape());
                        if (!pBodyShape) {
                            continue;
                        }
                        bhkShapeCollection* pBodyCollection = pBodyShape->FindShapeCollection();
                        if (pBodyCollection) {
                            eBodyMaterial = static_cast<HavokMaterialType>(pBodyCollection->QSubMaterial(uiBodyKey));
                        }
                    }
                    else {
                        bhkShape* pBodyShape = bhkShape::Getbhk(pCollidable->getShape());
                        if (!pBodyShape) {
                            continue;
                        }
                        eBodyMaterial = static_cast<HavokMaterialType>(pBodyShape->GetMaterial());
                    }
                    if (eBodyMaterial != eMaterial) {
                        continue;
                    }
                    bool bLand = CFilter(pCollidable->getCollisionFilterInfo()).GetGroup() == 1;
                    NiAVObject* pDecalObject = nullptr;
                    if (bLand) {
                        pDecalObject = TES::pInstance->GetLandGeometry(kHitPos);
                        if (pDecalObject && pDecalObject->GetParent()) {
                            pDecalObject = pDecalObject->GetParent();
                        }
                    }
                    else {
                        pDecalObject = bhkUtilFunctions::GetNiAVObject(const_cast<hkpCollidable*>(pCollidable));
                    }
                    TESObjectREFR* pDecalRef = TESObjectREFR::FindReferenceFor3D(pDecalObject);
                    if (!bLand &&
                        (!pDecalRef ||
                         !BGSDecalManager::IsValidDecalImpact(pDecalRef, BGSDecalManager::DECAL_TYPE_SIMPLE))) {
                        continue;
                    }
                    TESObjectCELL* pDecalCell = TES::pInstance->GetCellForPoint(kHitPos);
                    DECAL_CREATION_DATA kDecalData;
                    kDecalData.pOrigin = kDecalOrigin;
                    kDecalData.pDirection = kDecalDirection;
                    kDecalData.fDecalWidth = fDecalWidth;
                    kDecalData.fDecalHeight = fDecalWidth;
                    kDecalData.fDecalDepth = pImpact->GetDecalDepth();
                    kDecalData.pAVObj = pDecalObject;
                    kDecalData.pTexSet = pTexSet;
                    kDecalData.fRotation = fRotation;
                    kDecalData.cSubTex = cSubTex;
                    kDecalData.bAlphaBlending = pImpact->GetIsAlphaBlend();
                    kDecalData.bAlphaTesting = pImpact->GetIsAlphaTest();
                    kDecalData.bUseParallaxOcclusion = pImpact->GetIsParallax();
                    kDecalData.fParallaxOcclusionScale = pImpact->GetDecalParallaxScale();
                    kDecalData.ucParallaxOcclusionMaxPasses = pImpact->GetParallaxPasses();
                    kDecalData.fSpecular = pImpact->GetDecalShininess();
                    kDecalData.fDecalEpsilon = pImpact->GetDecalAngleThreshold();
                    // Each colour byte over the double 255.0, rounded to float; the high byte goes to z, the low
                    // byte to x.
                    int32_t iHigh = (pImpact->GetDecalColor() >> 16) & 0xFF;
                    float fHigh = static_cast<float>(iHigh / 255.0);
                    int32_t iMid = ((pImpact->GetDecalColor() & 0xFFFF) >> 8) & 0xFF;
                    float fMid = static_cast<float>(iMid / 255.0);
                    int32_t iLow = pImpact->GetDecalColor() & 0xFF;
                    float fLow = static_cast<float>(iLow / 255.0);
                    NiPoint3 kColor;
                    NiPoint3* pColor = ::new (&kColor) NiPoint3(fLow, fMid, fHigh);
                    kDecalData.fVertexColor.r = pColor->x;
                    kDecalData.fVertexColor.g = pColor->y;
                    kDecalData.fVertexColor.b = pColor->z;
                    pDecalCell->AddDecal(kDecalData, BGSDecalManager::DECAL_TYPE_SIMPLE, false);
                }
                pBodies->~bhkFilteredCdBodyCollector();
            }
        }
        if (bBlood && pRef && pRef->IsActor()) {
            TESCreature* pCreature = dynamic_cast<TESCreature*>(GetActorBase());
            if ((pCreature && pCreature->IsHumanoidCreature()) || IsNpc()) {
                bBlood = !bDisableAllGore.Bool();
            }
        }
        if (bBlood && pImpact) {
            // GetModel is called virtually through the TESModel base.
            TESModel* pModel = pImpact;
            if (pModel->GetModel() && *pModel->GetModel()) {
                // The spray direction: the hit direction less its component along the eye direction.
                NiPoint3 kSpray = kHitDirection;
                float fAlong = kSpray.Dot(kEyeDirection);
                kSpray -= fAlong * kEyeDirection;
                kSpray.Unitize();
                const char* pModelName = pModel->GetModel();
                float fLifetime = pImpact->GetEffectDuration();
                TESObjectCELL* pParentCell = GetParentCell();
                BSTempEffectParticle::Spawn(pParentCell, fLifetime, pModelName, kSpray, kHitPos, 1.0f, 7, nullptr);
            }
        }
        BSSoundHandle kSound;
        if (pImpact && pImpact->GetSound1()) {
            TESSound* pSettings = pImpact->GetSound1();
            const char* pFilename = pImpact->GetSound1()->GetFilename();
            kSound = BSAudio::QInstance().GetSoundHandleByFilename(
                pFilename,
                BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
                pSettings
            );
            kSound.SetPosition(kHitPos.x, kHitPos.y, kHitPos.z);
            kSound.Play(false);
        }
        if (pImpact && pImpact->GetSound2()) {
            TESSound* pSettings = pImpact->GetSound2();
            const char* pFilename = pImpact->GetSound2()->GetFilename();
            kSound = BSAudio::QInstance().GetSoundHandleByFilename(
                pFilename,
                BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
                pSettings
            );
            kSound.SetPosition(kHitPos.x, kHitPos.y, kHitPos.z);
            kSound.Play(false);
        }
        if (pRef && (pRef->IsProjectile() || pRef->GetObjectReference()->GetFormType() == WEAP_ID)) {
            // The compare with the keyframed motion type is signed.
            if (static_cast<int32_t>(pBody->getMotionType()) < hkpMotion::MOTION_KEYFRAMED) {
                pRef->SetActorCause(GetActorCause());
            }
            pRef->DamageObject(fDamage, false);
            if (pRef->GetDestroyed() && this == pPlayer) {
                pPlayer->AddPlayerAction(PLAYER_ACTION_DESTROY_OBJECT, 8.0f, nullptr);
            }
        }
        bResult = true;
    }
    else if (this != pPlayer) {
        hkpPhantom* pPhantom = hkpGetPhantom(pCollector->getHits()[0].m_rootCollidableB);
        bhkEntity* pBhkPhantom = pPhantom ? bhkEntity::Getbhk(pPhantom) : nullptr;
        if (pActor && !pRef->IsDead(false) && pBhkPhantom) {
            CFilter kPhantomFilter;
            pBhkPhantom->GetCollisionFilter(&kPhantomFilter);
            if (kPhantomFilter.GetLayer() == L_CHARCONTROLLER) {
                if (pActor->IsCreature()) {
                    ImpactMixer::PlayHitSound(this, fDamage, 0.0f, pActor, iWeaponType, -1, -1, false, false);
                }
                else {
                    ItemChange* pArmor = pActor->GetRandomWornArmor(false);
                    if (pArmor) {
                        TESObjectARMO* pArmorForm = static_cast<TESObjectARMO*>(pArmor->GetContainerObject());
                        int32_t iArmorWeight = pArmorForm->GetArmorWeight();
                        ImpactMixer::PlayHitSound(
                            this,
                            fDamage,
                            fDamage,
                            pActor,
                            iWeaponType,
                            iArmorWeight,
                            -1,
                            true,
                            false
                        );
                        delete pArmor;
                    }
                    else {
                        ImpactMixer::PlayHitSound(this, fDamage, 0.0f, pActor, iWeaponType, -1, -1, true, false);
                    }
                }
                bResult = true;
            }
        }
    }
    if (this != pPlayer) {
        return bResult;
    }
    TESObjectCELL* pRefCell = pRef ? pRef->GetParentCell() : nullptr;
    bhkWorld* pRefWorld = pRefCell ? pRefCell->GetbhkWorld() : nullptr;
    TESTrapListener* pTrapListener = nullptr;
    if (pRefWorld) {
        pTrapListener = static_cast<TESTrapListener*>(pRefWorld->QTrapListener());
    }
    if (!pTrapListener || pRef != Interface::GetActivateREFR() || !pTrapListener->IsCurrentTrigger(pRef)) {
        return bResult;
    }
    ExtraDataList* pExtra = pRef->GetExtra();
    if (!pExtra) {
        return bResult;
    }
    // The OnTrigger script event.
    Script::SetActionFlag(pRef, pExtra, SCRIPT_ACTION_ONTRIGGER);
    return true;
}

void Actor::FakeWeaponHitSound(
    NiPoint3 aLocation,
    TESObjectREFR* apTarget,
    bhkRigidBody* apBody,
    HavokMaterialType eTargetMat
) {
    TESObjectWEAP* pcurrentWeapon;
    ItemChange* pcurrentWeaponItem;
    ImpactMixer::COLLISION_SOUND fakeCollisionData;
    if (!apBody && apTarget && apTarget->Get3D()) {
        bhkCollisionObject* pcol = FindFirstCollisionObject(apTarget->Get3D());
        if (pcol) {
            apBody = pcol->GetRigidBody();
            eTargetMat = apBody->GetShape()->GetMaterial();
        }
    }

    ::new (&fakeCollisionData) NiBound;
    fakeCollisionData.fFriction = 0.5f;
    fakeCollisionData.pBody1 = nullptr;
    fakeCollisionData.pBody2 = apBody;
    fakeCollisionData.cMaterialType1 = 36;
    fakeCollisionData.cMaterialType2 = eTargetMat;
    fakeCollisionData.Location = aLocation;
    fakeCollisionData.fVelocity = 100.0f;
    // The collision's ID is the sum of the two addresses' low 32 bits, only ever compared.
    fakeCollisionData.iID = static_cast<int32_t>(
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this)) +
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apBody))
    );
    if (pCurrentProcess) {
        pcurrentWeaponItem = pCurrentProcess->GetCurrentWeapon();
    }
    else {
        pcurrentWeaponItem = nullptr;
    }
    if (pcurrentWeaponItem) {
        pcurrentWeapon = static_cast<TESObjectWEAP*>(pcurrentWeaponItem->pContainerObj);
    }
    else {
        pcurrentWeapon = nullptr;
    }
    if (apTarget && apTarget->IsActor()) {
        WEAPON_TYPE eWeaponType;
        if (pcurrentWeapon) {
            eWeaponType = pcurrentWeapon->GetWeaponType();
        }
        else {
            eWeaponType = WEAPON_NONE;
        }
        ImpactMixer::PlayHitSound(
            this,
            0.0f,
            0.0f,
            static_cast<Actor*>(apTarget),
            static_cast<int32_t>(eWeaponType),
            -1,
            -1,
            false,
            false
        );
    }
    else {
        if (eTargetMat != BHK_MATERIAL_ORGANIC) {
            if (pcurrentWeapon) {
                WEAPON_TYPE eWeaponType = pcurrentWeapon->GetWeaponType();
                ImpactMixer::PlayHitSound(
                    this,
                    0.0f,
                    0.0f,
                    this,
                    static_cast<int32_t>(eWeaponType),
                    -1,
                    -1,
                    true,
                    false
                );
            }
            else {
                ImpactMixer::PlayHitSound(this, 0.0f, 0.0f, this, -1, -1, -1, true, false);
            }
        }
        ImpactMixer::PlayCollisionSound(fakeCollisionData);
    }
}

void Actor::PickAnimations(float afMoveScale, float afTurnScale) {
    float fSpeed;
    if (!IsDead(false) && !IsKnockedOut()) {
        if (this == pPlayer && !pPlayer->Is3rdPerson()) {
            Warning("AI: Don't call Actor::PickAnimations on the 1st person pc.");
            return;
        }
        Animation* panim = GetAnimation();
        bhkCharacterController* pcharController = GetCharController();
        if (panim && pCurrentProcess && pcharController) {
            TESObjectWEAP* pweapon;
            if (!pCurrentProcess->eLevel) {
                pCurrentProcess->BaseProcess::ComputeLastTimeProcessed();
            }
            BSAnimGroupSequence* pweaponSeq = panim->GetCurrentSequence(AGS_WEAPON);
            if (pweaponSeq &&
                (pweaponSeq->GetAnimGroup()->GetMove() == AGM_WALK) != IsSneaking() &&
                pweaponSeq->GetState() == NiControllerSequence::kAnimating &&
                pweaponSeq->GetAnimGroup()->IsAimAnim()) {
                int32_t igroup = panim->GetGroup(AGS_WEAPON);
                if (igroup != GetAnimGroup(pweaponSeq->GetAnimGroup()->GetType(), nullptr, false, nullptr)) {
                    panim->BlendOut(AGS_WEAPON, false);
                }
            }
            else {
                if ((TESAnimGroup::GetType(panim->GetGroup(AGS_WEAPON)) == ANIM_GROUP_ATTACK_LOOP ||
                     TESAnimGroup::GetType(panim->GetGroup(AGS_WEAPON)) == ANIM_GROUP_ATTACK_LOOP_IS) &&
                    (pCurrentProcess->GetAutomaticShotDelay() == 0.0 &&
                     pweaponSeq->GetState() == NiControllerSequence::kAnimating)) {
                    panim->BlendOut(AGS_WEAPON, false);
                }
            }
            if (this == pPlayer) {
                Animation* p1stPersonAnim = pPlayer->GetAnimation(true);
                pweaponSeq = p1stPersonAnim->GetCurrentSequence(AGS_WEAPON);
                if (pweaponSeq &&
                    (pweaponSeq->GetAnimGroup()->GetMove() == AGM_WALK) != IsSneaking() &&
                    pweaponSeq->GetState() == NiControllerSequence::kAnimating &&
                    pweaponSeq->GetAnimGroup()->IsAimAnim()) {
                    int32_t igroup = p1stPersonAnim->GetGroup(AGS_WEAPON);
                    if (igroup != GetAnimGroup(pweaponSeq->GetAnimGroup()->GetType(), nullptr, false, p1stPersonAnim)) {
                        p1stPersonAnim->BlendOut(AGS_WEAPON, false);
                    }
                }
                else {
                    if ((TESAnimGroup::GetType(p1stPersonAnim->GetGroup(AGS_WEAPON)) == ANIM_GROUP_ATTACK_LOOP ||
                         TESAnimGroup::GetType(p1stPersonAnim->GetGroup(AGS_WEAPON)) == ANIM_GROUP_ATTACK_LOOP_IS) &&
                        (pCurrentProcess->GetAutomaticShotDelay() == 0.0 &&
                         pweaponSeq->GetState() == NiControllerSequence::kAnimating)) {
                        p1stPersonAnim->BlendOut(AGS_WEAPON, false);
                    }
                }
            }
            float fanimSpeed = 1.0f;
            float funitsPerSec = 0.0f;
            bool bwantweapondrawn = GetWantWeaponDrawn();
            ANIM_GROUP_ENUM eanimGroup = ANIM_GROUP_IDLE;
            ANIM_GROUP_MOVE_ENUM eanimMove = AGM_NONE;
            ANIM_GROUP_WEAPON_ENUM eanimWeapon = AGW_NONE;
            ANIMATION_ACTION eanimAction = ANIM_ACTION_NONE;
            BSAnimGroupSequence* pseqLowerBody = panim->GetCurrentSequence(AGS_MOVEMENT);
            BipedAnim* pbiped = GetBiped();
            ItemChange* pcurrentweapon = pCurrentProcess->GetCurrentWeapon();
            if (pcurrentweapon) {
                pweapon = static_cast<TESObjectWEAP*>(pcurrentweapon->pContainerObj);
            }
            else {
                pweapon = nullptr;
            }
            uint16_t smoveMode = GetMoveMode();
            ANIMATION_ACTION eact = pCurrentProcess->GetAnimAction();
            uint32_t isitSleep = pCurrentProcess->GetSitSleepState();
            FurnitureMark* pfurnitureMarker = pCurrentProcess->GetFurnitureMarker();
            if (eact != ANIM_ACTION_NONE) {
                if (pCurrentProcess->GetAnimActionAnimSeq() &&
                    pCurrentProcess->GetAnimActionAnimSeq()->GetState() != NiControllerSequence::kInactive) {
                    switch (eact) {
                        case ANIM_ACTION_EQUIP_WEAPON:
                        case ANIM_ACTION_UNEQUIP_WEAPON: {
                            if (pCurrentProcess->GetWeaponDrawn() != (eact == ANIM_ACTION_EQUIP_WEAPON) &&
                                panim->GetGroupAction(AGS_WEAPON) >= AGA_STOP) {
                                Animation* ptmpAnim = panim;
                                BipedAnim* ptmpBiped = pbiped;
                                int32_t ibipedCount = 1;
                                if (this == pPlayer) {
                                    ibipedCount = 2;
                                }
                                pCurrentProcess->SetWeaponDrawn(this, eact == ANIM_ACTION_EQUIP_WEAPON);
                                while (ibipedCount) {
                                    if (this == pPlayer && ibipedCount == 1) {
                                        ptmpAnim = pPlayer->GetAnimation(true);
                                        ptmpBiped = pPlayer->GetBiped(true);
                                    }
                                    if (!pCurrentProcess->GetWeaponDrawn() &&
                                        pweapon &&
                                        ptmpAnim &&
                                        ptmpAnim->GetSceneRoot()) {
                                        NiNode* psceneRoot = ptmpAnim->GetSceneRoot();
                                        WEAPON_TYPE eWeaponType = pweapon->GetWeaponType();
                                        ClearGunWobble(weaponToAnimGroupWeapon[eWeaponType], psceneRoot);
                                    }

                                    pCurrentProcess->ForceWeaponDrawnSheathed(
                                        eact == ANIM_ACTION_EQUIP_WEAPON,
                                        ptmpBiped,
                                        ptmpAnim,
                                        this
                                    );
                                    --ibipedCount;
                                }
                                SetIronSights(false, false, false);
                                if (this == pPlayer) {
                                    if (eact) {
                                        pRagdollController->SetGrabIKTarget(bhkRagdollController::LEFT_ARM, nullptr);
                                        pRagdollController->SetGrabIKTarget(bhkRagdollController::RIGHT_ARM, nullptr);
                                        pRagdollController->SetGrabIKEaseOut(bhkRagdollController::RIGHT_ARM, true);
                                        pRagdollController->SetGrabIKEaseOut(bhkRagdollController::RIGHT_ARM, true);
                                    }
                                    else {
                                        NiAVObject* pleftTarget = nullptr;
                                        NiAVObject* prightTarget = nullptr;
                                        NiAVObject* pweap3D = pCurrentProcess->GetWeaponBone(pbiped)->GetAtChecked(0);
                                        if (pweap3D) {
                                            BSXFlags* pflags = static_cast<BSXFlags*>(
                                                pweap3D->GetExtraData(BSXFlags::Tag())
                                            );
                                            if (pflags && pflags->QIKTarget()) {
                                                pleftTarget = pweap3D->GetObjectByName(FixedStrings::QGrabLeft());
                                                prightTarget = pweap3D->GetObjectByName(FixedStrings::QGrabRight());
                                            }
                                        }
                                        pRagdollController->SetGrabIKTarget(
                                            bhkRagdollController::LEFT_ARM,
                                            pleftTarget
                                        );
                                        pRagdollController->SetGrabIKTarget(
                                            bhkRagdollController::RIGHT_ARM,
                                            prightTarget
                                        );
                                    }
                                }
                                pCurrentProcess->SetShouldRefreshMagicShadersEffects(true, false, false);
                                if (pCurrentProcess->GetWeaponDrawn() &&
                                    pCurrentProcess->GetWeaponEnchantmentVisuals()) {
                                    TESEffectShader* pshader = pCurrentProcess->GetWeaponEnchantmentVisuals();
                                    MagicShaderHitEffect* peffect = ReferenceProcessLists
                                                                        .FindAndCleanupWeaponShaderHitEffect(
                                                                            this,
                                                                            pshader
                                                                        );
                                    if (peffect) {
                                        peffect->ResetAlphaTimer();
                                    }
                                    if (pweapon) {
                                        MagicItem* penchant;
                                        EnchantmentItem* penchantment = TESEnchantableForm::GetFormEnchanting(pweapon);
                                        if (penchantment) {
                                            penchant = &static_cast<MagicItem&>(
                                                static_cast<MagicItemForm&>(*penchantment)
                                            );
                                        }
                                        else {
                                            penchant = nullptr;
                                        }
                                        if (!penchant) {
                                            AlchemyItem* ppoison = pcurrentweapon->GetPoison();
                                            if (ppoison) {
                                                penchant = &static_cast<MagicItem&>(
                                                    static_cast<MagicItemObject&>(*ppoison)
                                                );
                                            }
                                            else {
                                                penchant = nullptr;
                                            }
                                        }
                                        if (penchant) {
                                            PlaySoundByEditorName(
                                                "WPNBlade1HandEquipEnchanted",
                                                false,
                                                BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                                    BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
                                                true
                                            );
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        case ANIM_ACTION_ATTACK: {
                            ANIM_GROUP_SECTION eSectionLayer = animGroupInfo[pCurrentProcess->GetAnimActionAnimSeq()
                                                                                 ->GetAnimGroup()
                                                                                 ->GetType()]
                                                                   .eSectionLayer;
                            if (eSectionLayer == AGS_LEFT_ARM) {
                                if (panim->GetGroupAction(AGS_LEFT_ARM) == AGA_STOP) {
                                    ANIM_GROUP_ACTION_TYPE
                                    eAction = animGroupInfo[TESAnimGroup::GetType(panim->GetGroup(AGS_LEFT_ARM))]
                                                  .eAction;
                                    if (eAction >= AGAT_ATTACK && eAction <= AGAT_ATTACK_POWER) {
                                        if (GetCurrentSpell()) {
                                            ReleaseCast(nullptr);
                                        }
                                        SetAnimAction(
                                            ANIM_ACTION_ATTACK_FOLLOW_THROUGH,
                                            pCurrentProcess->GetAnimActionAnimSeq()
                                        );
                                    }
                                }
                            }
                            else if (eSectionLayer == AGS_WEAPON) {
                                if (animGroupInfo[pCurrentProcess->GetAnimActionAnimSeq()->GetAnimGroup()->GetType()]
                                            .eAction != AGAT_LOOP &&
                                    panim->GetGroupAction(AGS_WEAPON) == AGA_STOP) {
                                    ANIM_GROUP_ACTION_TYPE
                                    eAction = animGroupInfo[TESAnimGroup::GetType(panim->GetGroup(AGS_WEAPON))].eAction;
                                    if (eAction == AGAT_ATTACK) {
                                        if (GetCurrentSpell()) {
                                            ReleaseCast(nullptr);
                                        }
                                        else if (!pweapon || !pweapon->IsAutomaticWeapon()) {
                                            if (pweapon && pweapon->IsRangedWeapon()) {
                                                pCurrentProcess->AddPostAnimationAction(
                                                    BaseProcess::POSTANIM_ACTION_FIRE_WEAPON
                                                );
                                            }
                                            else {
                                                TESPackage* ppack;
                                                if (this == pPlayer ||
                                                    (IsInCombat() ||
                                                     ((ppack = pCurrentProcess->GetPackageThatIsRunning()) != nullptr &&
                                                      (ppack->GetPackType() == PACKAGE_TYPE_USE_ITEM_AT ||
                                                       ppack->GetPackType() == PACKAGE_TYPE_USE_WEAPON)))) {
                                                    MeleeAttack(false, true);
                                                }
                                                else {
                                                    MeleeAttack(false, false);
                                                }
                                            }
                                        }
                                        if (IsAttacking()) {
                                            if (!pweapon ||
                                                (pweapon->IsAutomaticWeapon() || pweapon->IsMeleeWeapon())) {
                                                SetAnimAction(
                                                    ANIM_ACTION_ATTACK_FOLLOW_THROUGH,
                                                    pCurrentProcess->GetAnimActionAnimSeq()
                                                );
                                            }
                                            else {
                                                SetAnimAction(
                                                    ANIM_ACTION_ATTACK_WAIT_FOR_EJECT,
                                                    pCurrentProcess->GetAnimActionAnimSeq()
                                                );
                                            }
                                        }
                                    }
                                    else if (eAction == AGAT_ATTACK_POWER) {
                                        if (this == pPlayer || IsInCombat()) {
                                            MeleeAttack(true, true);
                                        }
                                        else {
                                            TESPackage* ppack = pCurrentProcess->GetPackageThatIsRunning();
                                            if (ppack &&
                                                ((ppack->GetPackType() == PACKAGE_TYPE_USE_ITEM_AT ||
                                                  ppack->GetPackType() == PACKAGE_TYPE_USE_WEAPON) &&
                                                 !pCurrentProcess->IsCurrentWeaponGrenadeOrMine())) {
                                                MeleeAttack(false, true);
                                            }
                                            else {
                                                MeleeAttack(true, false);
                                            }
                                        }
                                        if (IsAttacking()) {
                                            SetAnimAction(
                                                ANIM_ACTION_ATTACK_FOLLOW_THROUGH,
                                                pCurrentProcess->GetAnimActionAnimSeq()
                                            );
                                        }
                                    }
                                    else if (
                                        eAction == AGAT_ATTACK_SPIN && pCurrentProcess->GetAutomaticShotDelay() == 0.0
                                    ) {
                                        SetAnimAction(
                                            ANIM_ACTION_ATTACK_FOLLOW_THROUGH,
                                            pCurrentProcess->GetAnimActionAnimSeq()
                                        );
                                    }
                                }
                            }
                            break;
                        }
                        case ANIM_ACTION_ATTACK_WAIT_FOR_EJECT: {
                            if (animGroupInfo[pCurrentProcess->GetAnimActionAnimSeq()->GetAnimGroup()->GetType()]
                                        .eSectionLayer == AGS_WEAPON &&
                                pweapon) {
                                if (animGroupInfo[pCurrentProcess->GetAnimActionAnimSeq()->GetAnimGroup()->GetType()]
                                            .eAction != AGAT_LOOP &&
                                    panim->GetGroupAction(AGS_WEAPON) == AGA_SPECIAL_IDLE_LOOP_STOP) {
                                    pCurrentProcess->AddPostAnimationAction(
                                        BaseProcess::POSTANIM_ACTION_EXPEL_SHELLCASING
                                    );
                                    SetAnimAction(
                                        ANIM_ACTION_ATTACK_FOLLOW_THROUGH,
                                        pCurrentProcess->GetAnimActionAnimSeq()
                                    );
                                }
                            }

                            break;
                        }
                        case ANIM_ACTION_ATTACK_THROW: {
                            ANIM_GROUP_ACTION erelease = AGA_SPECIAL_IDLE_LOOP_STOP;
                            BSAnimGroupSequence* panimGroupSeq = pCurrentProcess->GetAnimActionAnimSeq();
                            TESAnimGroup* panimGroup = panimGroupSeq->GetAnimGroup();
                            ANIM_GROUP_ENUM ecurrentGroup = panimGroup->GetType();
                            ANIM_GROUP_ACTION_TYPE eAction = animGroupInfo[ecurrentGroup].eAction;
                            if (eAction != AGAT_ATTACK_THROW) {
                                if (eAction != AGAT_ATTACK_MINE) {
                                    break;
                                }
                                erelease = AGA_STOP;
                            }
                            if (panim->GetGroupAction(AGS_WEAPON) == erelease) {
                                if (pweapon && !pweapon->IsMeleeWeapon()) {
                                    pCurrentProcess->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_FIRE_WEAPON);
                                }

                                if (IsAttacking()) {
                                    SetAnimAction(
                                        ANIM_ACTION_ATTACK_THROW_ATTACH,
                                        pCurrentProcess->GetAnimActionAnimSeq()
                                    );
                                }
                            }
                            break;
                        }
                        case ANIM_ACTION_ATTACK_THROW_ATTACH: {
                            ANIM_GROUP_ACTION eattach = AGA_SPECIAL_IDLE_STOP;
                            ANIM_GROUP_ACTION_TYPE eAction = animGroupInfo[pCurrentProcess->GetAnimActionAnimSeq()
                                                                               ->GetAnimGroup()
                                                                               ->GetType()]
                                                                 .eAction;
                            if (eAction != AGAT_ATTACK_THROW) {
                                if (eAction != AGAT_ATTACK_MINE) {
                                    break;
                                }
                                eattach = AGA_SPECIAL_IDLE_LOOP_STOP;
                            }
                            if (panim->GetGroupAction(AGS_WEAPON) == eattach) {
                                if (pweapon && !pweapon->IsMeleeWeapon()) {
                                    Animation* ptmpAnim = panim;
                                    BipedAnim* ptmpBiped = pbiped;
                                    int32_t ibipedCount = 1;
                                    if (this == pPlayer) {
                                        ibipedCount = 2;
                                    }
                                    while (ibipedCount) {
                                        if (this == pPlayer && ibipedCount == 1) {
                                            ptmpAnim = pPlayer->GetAnimation(true);
                                            ptmpBiped = pPlayer->GetBiped(true);
                                        }
                                        pCurrentProcess->ForceWeaponDrawnSheathed(true, ptmpBiped, ptmpAnim, this);
                                        --ibipedCount;
                                    }
                                }

                                if (IsAttacking()) {
                                    SetAnimAction(
                                        ANIM_ACTION_ATTACK_FOLLOW_THROUGH,
                                        pCurrentProcess->GetAnimActionAnimSeq()
                                    );
                                }
                            }
                            break;
                        }
                        case ANIM_ACTION_DODGE: {
                            funitsPerSec = GetWalkSpeed();
                            break;
                        }
                        case ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM: {
                            if (!isitSleep) {
                                switch (pcharController->GetState()) {
                                    case HK_CHARACTER_JUMPING: SetAnimAction(ANIM_ACTION_NONE, nullptr); break;
                                    case HK_CHARACTER_IN_AIR:
                                        if (pseqLowerBody) {
                                            if (pseqLowerBody->GetAnimGroup()->IsJumpingLoopAnim()) {
                                                SetAnimAction(ANIM_ACTION_NONE, nullptr);
                                            }
                                        }
                                        break;
                                    default: break;
                                }
                            }
                            break;
                        }
                        case ANIM_ACTION_RELOAD_LOOP: {
                            BSAnimGroupSequence* panimGroupSeq = pCurrentProcess->GetAnimActionAnimSeq();
                            TESAnimGroup* panimGroup = panimGroupSeq->GetAnimGroup();
                            ANIM_GROUP_ENUM ecurrentGroup = panimGroup->GetType();
                            if (bwantweapondrawn &&
                                (!pCurrentProcess->GetWeaponDrawn() && (pweapon && pcurrentweapon))) {
                                bool bclipMod = pcurrentweapon->HasModEffectActive(2);
                                ReloadWeaponNV(pweapon, 0, bclipMod, true);
                                eanimAction = ANIM_ACTION_NONE;
                                SetAnimAction(ANIM_ACTION_NONE, nullptr);
                            }
                            else if (ecurrentGroup > ANIM_GROUP_RELOAD_Z_START) {
                                if (panim->GetGroupAction(AGS_WEAPON) == AGA_STOP) {
                                    pCurrentProcess->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_RELOAD_SINGLE);
                                }
                            }
                            else {
                                double dtime = panim->GetTime(panimGroupSeq);
                                if (panimGroupSeq->GetAnimGroup()->GetTime(AGA_STOP) <= dtime) {
                                    bool bclipMod = pcurrentweapon->HasModEffectActive(2);
                                    ANIM_GROUP_ENUM ereloadGroup = pweapon->GetReloadAnim(bclipMod);
                                    uint16_t sanimGroup = GetAnimGroup(ereloadGroup, nullptr, false, nullptr);
                                    if (TESAnimGroup::GetType(sanimGroup) == ereloadGroup) {
                                        GetAnimation()->PlayGroup(sanimGroup, ACTION_START, -1, AGS_NONE);
                                        if (this == pPlayer) {
                                            pPlayer->StartAnimOn1stPerson(sanimGroup, ACTION_START);
                                        }
                                    }
                                    eanimAction = ANIM_ACTION_RELOAD_LOOP;
                                    SetAnimAction(ANIM_ACTION_RELOAD_LOOP, panim->GetCurrentSequence(AGS_WEAPON));
                                }
                            }
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
                else {
                    if (panim->GetCurrentSequence(AGS_LEFT_ARM)) {
                        if (TESAnimGroup::GetType(panim->GetGroup(AGS_LEFT_ARM)) == ANIM_GROUP_BLOCK_IDLE) {
                            eanimAction = ANIM_ACTION_BLOCK;
                            SetAnimAction(ANIM_ACTION_BLOCK, panim->GetCurrentSequence(AGS_LEFT_ARM));
                        }
                    }
                    if (pweapon && (!pweapon->GetNoJamAfterReload() && eact == ANIM_ACTION_RELOAD)) {
                        if (TESAnimGroup::IsReloadAction(
                                pCurrentProcess->GetAnimActionAnimSeq()->GetAnimGroup()->GetType()
                            )) {
                            int32_t iweaponCondition = pCurrentProcess->GetWeaponCondition(this);
                            if (CombatFormulas::GetWeaponConditionReloadJamMult(iweaponCondition) > 0.0) {
                                float fjamChance = CombatFormulas::GetWeaponConditionReloadJamMult(iweaponCondition);
                                if (RandomBool(fjamChance)) {
                                    ANIM_GROUP_ENUM ejamAnimGroup = static_cast<ANIM_GROUP_ENUM>(
                                        pweapon->GetReloadAnim(false) + 23
                                    );
                                    uint16_t sanimGroup = GetAnimGroup(ejamAnimGroup, nullptr, false, nullptr);
                                    if (TESAnimGroup::GetType(sanimGroup) == ejamAnimGroup) {
                                        eanimAction = ANIM_ACTION_RELOAD;
                                        StartWeaponGroup(ejamAnimGroup, panim);
                                        SetAnimAction(ANIM_ACTION_RELOAD, panim->GetCurrentSequence(AGS_WEAPON));
                                        StartAnimOn1stPerson(sanimGroup, ACTION_START);
                                    }
                                }
                            }
                        }
                    }

                    if (eanimAction == ANIM_ACTION_NONE) {
                        SetAnimAction(ANIM_ACTION_NONE, nullptr);
                    }
                }
            }
            if (eanimAction == ANIM_ACTION_NONE) {
                if (!isitSleep) {
                    switch (eact) {
                        case ANIM_ACTION_NONE:
                        case ANIM_ACTION_ATTACK:
                        case ANIM_ACTION_ATTACK_FOLLOW_THROUGH:
                        case ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM: {
                            hkpCharacterStateType eState = pcharController->GetState();
                            if (eState) {
                                if (eState == HK_CHARACTER_JUMPING) {
                                    eanimGroup = ANIM_GROUP_JUMP_LOOP;
                                }
                                else if (eState == HK_CHARACTER_IN_AIR) {
                                    double dfallTime = pcharController->QFallTime();
                                    if (fJumpAnimDelay.Float() < dfallTime ||
                                        (pseqLowerBody && pseqLowerBody->GetAnimGroup()->IsJumpingLoopAnim())) {
                                        eanimGroup = ANIM_GROUP_JUMP_LOOP;
                                    }
                                }
                            }
                            else if (pseqLowerBody) {
                                if (pseqLowerBody->GetAnimGroup()->IsJumpingLoopAnim() ||
                                    pseqLowerBody->GetAnimGroup()->GetType() == ANIM_GROUP_JUMP_START) {
                                    eact = ANIM_ACTION_NONE;
                                    eanimAction = ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM;
                                    if ((smoveMode & 1) != 0) {
                                        eanimGroup = ANIM_GROUP_JUMP_LAND_FORWARD;
                                    }
                                    else if ((smoveMode & 2) != 0) {
                                        eanimGroup = ANIM_GROUP_JUMP_LAND_BACKWARD;
                                    }
                                    else if ((smoveMode & 4) != 0) {
                                        eanimGroup = ANIM_GROUP_JUMP_LAND_LEFT;
                                    }
                                    else if ((smoveMode & 8) != 0) {
                                        eanimGroup = ANIM_GROUP_JUMP_LAND_RIGHT;
                                    }
                                    else {
                                        eanimGroup = ANIM_GROUP_JUMP_LAND;
                                    }
                                    panim->SkipNextBlend();
                                    Actor::PlayLandJumpSound(
                                        static_cast<PlayerCharacter*>(this),
                                        pcharController->eSupportMaterial
                                    );
                                }
                            }
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    if (bwantweapondrawn &&
                        !pCurrentProcess->GetWeaponDrawn() &&
                        eact == ANIM_ACTION_NONE &&
                        CanMove()) {
                        if (!panim->SpecialIdleDonePlaying()) {
                            panim->SpecialIdleFree(true, true);
                        }
                        eanimGroup = ANIM_GROUP_EQUIP;
                        eanimAction = ANIM_ACTION_EQUIP_WEAPON;
                    }
                }
                if (!bwantweapondrawn &&
                    pCurrentProcess->GetWeaponDrawn() &&
                    eact == ANIM_ACTION_NONE &&
                    !IsParalyzed() &&
                    !IsDead(false) &&
                    !IsKnockedOut() &&
                    !IsRestrained() &&
                    !IsUnConscious()) {
                    eanimGroup = ANIM_GROUP_UNEQUIP;
                    eanimAction = ANIM_ACTION_UNEQUIP_WEAPON;
                }
            }
            if ((!GetActorType() && (bwantweapondrawn || IsInCombat())) ||
                (pCurrentProcess->GetWeaponDrawn() ||
                 eanimAction == ANIM_ACTION_EQUIP_WEAPON ||
                 eanimAction == ANIM_ACTION_UNEQUIP_WEAPON)) {
                if (pcurrentweapon) {
                    eanimWeapon = weaponToAnimGroupWeapon[static_cast<TESObjectWEAP*>(pcurrentweapon->pContainerObj)
                                                              ->TESObjectWEAP::GetWeaponType()];
                }
                else {
                    eanimWeapon = AGW_HAND_TO_HAND;
                }
            }
            if ((smoveMode & 2048) != 0) {
                eanimMove = AGM_SNEAK;
            }
            else if ((smoveMode & ActorMover::MF_FLYING) != 0) {
                eanimMove = AGM_SWIM;
            }
            else if ((smoveMode & 1024) != 0) {
                eanimMove = AGM_WALK;
            }
            if (eanimGroup) {
                if (eanimGroup == ANIM_GROUP_JUMP_LOOP && (smoveMode & 15) != 0) {
                    if ((smoveMode & 1) != 0) {
                        eanimGroup = ANIM_GROUP_JUMP_LOOP_FORWARD;
                    }
                    else if ((smoveMode & 2) != 0) {
                        eanimGroup = ANIM_GROUP_JUMP_LOOP_BACKWARD;
                    }
                    else if ((smoveMode & 4) != 0) {
                        eanimGroup = ANIM_GROUP_JUMP_LOOP_LEFT;
                    }
                    else if ((smoveMode & 8) != 0) {
                        eanimGroup = ANIM_GROUP_JUMP_LOOP_RIGHT;
                    }
                }
            }
            else {
                switch (isitSleep) {
                    case 3u:
                    case 8u: {
                        if ((panim->SpecialIdlePlaying() || panim->SpecialIdleDonePlaying()) &&
                            (!pfurnitureMarker || pfurnitureMarker->cNumber <= 20u)) {
                            eanimGroup = ANIM_GROUP_DYNAMIC_IDLE;
                        }
                        break;
                    }
                    case 4u:
                    case 5u:
                    case 9u:
                    case 10u: {
                        if ((this != pPlayer || !pPlayer->GetAnimation(true)->spAnimIdleQueued.m_pObject) &&
                            (!pfurnitureMarker || pfurnitureMarker->cNumber <= 20u)) {
                            eanimGroup = ANIM_GROUP_DYNAMIC_IDLE;
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
                fanimSpeed = afMoveScale;
                if ((smoveMode & 15) != 0) {
                    if ((smoveMode & 512) != 0) {
                        if ((smoveMode & 1) != 0) {
                            eanimGroup = ANIM_GROUP_FAST_FORWARD;
                        }
                        else if ((smoveMode & 2) != 0) {
                            eanimGroup = ANIM_GROUP_FAST_BACK;
                        }
                        else if ((smoveMode & 4) != 0) {
                            eanimGroup = ANIM_GROUP_FAST_LEFT;
                        }
                        else if ((smoveMode & 8) != 0) {
                            eanimGroup = ANIM_GROUP_FAST_RIGHT;
                        }
                        if (GetActorType()) {
                            funitsPerSec = GetRunSpeed();
                        }
                        else {
                            funitsPerSec = GetWalkSpeed();
                        }
                    }
                    else if (
                        (smoveMode &
                         (ActorMover::MF_WALKING | ActorMover::MF_RUNNING | ActorMover::MF_SNEAKING |
                          ActorMover::MF_SWIMMING | ActorMover::MF_JUMP | ActorMover::MF_FLYING | ActorMover::MF_FALL |
                          ActorMover::MF_SLIDE)) != 0
                    ) {
                        if ((smoveMode & 1) != 0) {
                            eanimGroup = ANIM_GROUP_MOVE_FORWARD;
                        }
                        else if ((smoveMode & 2) != 0) {
                            eanimGroup = ANIM_GROUP_MOVE_BACK;
                        }
                        else if ((smoveMode & 4) != 0) {
                            eanimGroup = ANIM_GROUP_MOVE_LEFT;
                        }
                        else if ((smoveMode & 8) != 0) {
                            eanimGroup = ANIM_GROUP_MOVE_RIGHT;
                        }
                        funitsPerSec = GetWalkSpeed();
                    }
                }
                else if ((smoveMode & 16) != 0) {
                    eanimGroup = ANIM_GROUP_TURN_LEFT;
                }
                else if ((smoveMode & 32) != 0) {
                    eanimGroup = ANIM_GROUP_TURN_RIGHT;
                }
            }
            if (eanimAction == ANIM_ACTION_NONE || eact == ANIM_ACTION_NONE) {
                NiControllerSequence* pcurrentSeq;
                if (funitsPerSec < 1.0 &&
                    eanimGroup >= ANIM_GROUP_MOVE_FORWARD &&
                    eanimGroup <= ANIM_GROUP_TURN_RIGHT &&
                    eanimGroup != ANIM_GROUP_TURN_LEFT &&
                    eanimGroup != ANIM_GROUP_TURN_RIGHT) {
                    if (this == pPlayer) {
                        pPlayer->IsOverEncumbered();
                    }
                    eanimGroup = ANIM_GROUP_IDLE;
                }
                bool bpowerArmor = HasBackPackWorn() || IsPowerBodyArmorWorn();
                uint16_t sbaseGroup = TESAnimGroup::AnimGroup(eanimMove, eanimWeapon, eanimGroup, bpowerArmor);
                uint16_t sanimGroup = panim->PickBestAnimation(sbaseGroup, false);
                ANIM_GROUP_ENUM efoundGroup = TESAnimGroup::GetType(sanimGroup);
                if (eanimAction != ANIM_ACTION_NONE && eanimGroup != efoundGroup) {
                    if (eanimAction == ANIM_ACTION_UNEQUIP_WEAPON) {
                        pCurrentProcess->SetWeaponDrawn(this, false);
                    }
                    eanimAction = ANIM_ACTION_NONE;
                }
                eanimGroup = efoundGroup;
                if (eact != ANIM_ACTION_NONE &&
                    (pCurrentProcess->GetAnimActionAnimSeq() &&
                     (pcurrentSeq = panim->GetCurrentSequence(animGroupInfo[eanimGroup].eSectionLayer),
                      pcurrentSeq == pCurrentProcess->GetAnimActionAnimSeq()))) {
                    if (eact == ANIM_ACTION_FORCE_SCRIPT_ANIM) {
                        uint16_t smoveGroup = panim->GetGroup(AGS_MOVEMENT);
                        if (GetActorType()) {
                            switch (TESAnimGroup::GetType(smoveGroup)) {
                                case ANIM_GROUP_MOVE_BACK:
                                case ANIM_GROUP_MOVE_LEFT:
                                case ANIM_GROUP_MOVE_RIGHT:
                                case ANIM_GROUP_DODGE_FORWARD:
                                case ANIM_GROUP_DODGE_BACK:
                                case ANIM_GROUP_DODGE_LEFT:
                                case ANIM_GROUP_DODGE_RIGHT: {
                                    smoveGroup = (smoveGroup & ~TESAnimGroup::TYPE_MASK) | ANIM_GROUP_MOVE_FORWARD;
                                    break;
                                }
                                case ANIM_GROUP_FAST_BACK:
                                case ANIM_GROUP_FAST_LEFT:
                                case ANIM_GROUP_FAST_RIGHT: {
                                    smoveGroup = (smoveGroup & ~TESAnimGroup::TYPE_MASK) | ANIM_GROUP_FAST_FORWARD;
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                        else {
                            smoveGroup = (smoveGroup & ~TESAnimGroup::TYPE_MASK) | ANIM_GROUP_MOVE_FORWARD;
                        }
                        switch (TESAnimGroup::GetType(smoveGroup)) {
                            case ANIM_GROUP_MOVE_FORWARD:
                            case ANIM_GROUP_MOVE_BACK:
                            case ANIM_GROUP_MOVE_LEFT:
                            case ANIM_GROUP_MOVE_RIGHT:
                            case ANIM_GROUP_DODGE_FORWARD:
                            case ANIM_GROUP_DODGE_BACK:
                            case ANIM_GROUP_DODGE_LEFT:
                            case ANIM_GROUP_DODGE_RIGHT: {
                                funitsPerSec = GetWalkSpeed();
                                break;
                            }
                            case ANIM_GROUP_FAST_FORWARD:
                            case ANIM_GROUP_FAST_BACK:
                            case ANIM_GROUP_FAST_LEFT:
                            case ANIM_GROUP_FAST_RIGHT: {
                                funitsPerSec = GetRunSpeed();
                                break;
                            }
                            case ANIM_GROUP_TURN_LEFT:
                            case ANIM_GROUP_TURN_RIGHT: {
                                panim->SetMoveSpeed(afTurnScale);
                                return;
                            }
                            default: {
                                break;
                            }
                        }
                        float fspeed = static_cast<float>(panim->GetGroupSpeed(smoveGroup));
                        if (fspeed != 0.0) {
                            fanimSpeed = funitsPerSec / fspeed * fanimSpeed;
                        }
                        panim->SetMoveSpeed(fanimSpeed);
                    }
                }
                else {
                    if (sanimGroup != 255) {
                        if (eanimGroup == ANIM_GROUP_TURN_LEFT || eanimGroup == ANIM_GROUP_TURN_RIGHT) {
                            panim->SetMoveSpeed(afTurnScale);
                        }
                        else if (eanimGroup < ANIM_GROUP_MOVE_FORWARD || eanimGroup > ANIM_GROUP_TURN_RIGHT) {
                            if (eanimGroup >= ANIM_GROUP_EQUIP && eanimGroup <= ANIM_GROUP_COUNTER) {
                                if (pweapon) {
                                    fSpeed = pweapon->GetSpeed();
                                    BGSEntryPoint::HandleEntryPoint(
                                        BGSEntryPoint::ATTACK_SPEED_MODIFIER,
                                        this,
                                        pweapon,
                                        &fSpeed
                                    );
                                    if (!TESAnimGroup::IsAttackAction(eanimGroup) || pweapon->IsMeleeWeapon()) {
                                        panim->SetAttackSpeed(fSpeed);
                                    }
                                    else {
                                        ItemChange* pweaponItem = pCurrentProcess->GetCurrentWeapon();
                                        bool bspeedMod = pweaponItem->HasModEffectActive(8);
                                        double dattackMult = pweapon->GetAttackSpeed(bspeedMod);
                                        float fattackSpeed = dattackMult * fSpeed;
                                        panim->SetAttackSpeed(fattackSpeed);
                                    }
                                }
                                else {
                                    panim->SetAttackSpeed(1.0f);
                                }
                            }
                        }
                        else {
                            uint16_t sspeedGroup = sanimGroup;
                            if (GetActorType()) {
                                switch (TESAnimGroup::GetType(sanimGroup)) {
                                    case ANIM_GROUP_MOVE_BACK:
                                    case ANIM_GROUP_MOVE_LEFT:
                                    case ANIM_GROUP_MOVE_RIGHT:
                                    case ANIM_GROUP_DODGE_FORWARD:
                                    case ANIM_GROUP_DODGE_BACK:
                                    case ANIM_GROUP_DODGE_LEFT:
                                    case ANIM_GROUP_DODGE_RIGHT: {
                                        sspeedGroup = (sanimGroup & ~TESAnimGroup::TYPE_MASK) | ANIM_GROUP_MOVE_FORWARD;
                                        break;
                                    }
                                    case ANIM_GROUP_FAST_BACK:
                                    case ANIM_GROUP_FAST_LEFT:
                                    case ANIM_GROUP_FAST_RIGHT: {
                                        sspeedGroup = (sanimGroup & ~TESAnimGroup::TYPE_MASK) | ANIM_GROUP_FAST_FORWARD;
                                        break;
                                    }
                                    default: {
                                        break;
                                    }
                                }
                            }
                            else {
                                sspeedGroup = (sanimGroup & ~TESAnimGroup::TYPE_MASK) | ANIM_GROUP_MOVE_FORWARD;
                            }
                            float fgroupSpeed = static_cast<float>(panim->GetGroupSpeed(sspeedGroup));
                            if (fgroupSpeed != 0.0) {
                                fanimSpeed = funitsPerSec / fgroupSpeed * fanimSpeed;
                            }
                            panim->SetMoveSpeed(fanimSpeed);
                        }
                    }
                    ANIM_GROUP_SECTION eags = animGroupInfo[eanimGroup].eSectionLayer;
                    uint16_t scurrentGroup = panim->GetGroup(eags);
                    if ((scurrentGroup != sanimGroup ||
                         (!panim->GetCurrentSequence(eags) || !panim->GetCurrentSequence(eags)->GetState())) &&
                        panim->GroupLoaded(sanimGroup)) {
                        if (animGroupInfo[eanimGroup].eSectionLayer == AGS_MOVEMENT &&
                            panim->GetCurrentSequence(AGS_MOVEMENT)) {
                            if (eanimMove != TESAnimGroup::GetMove(panim->GetGroup(AGS_MOVEMENT))) {
                                bpowerArmor = HasBackPackWorn() || IsPowerBodyArmorWorn();
                                sbaseGroup = TESAnimGroup::AnimGroup(
                                    eanimMove,
                                    eanimWeapon,
                                    ANIM_GROUP_IDLE,
                                    bpowerArmor
                                );
                                uint16_t sidleGroup = panim->PickBestAnimation(sbaseGroup, false);
                                int32_t iidleGroup = sidleGroup;
                                if (iidleGroup != panim->GetGroup(AGS_IDLE)) {
                                    panim->PlayGroup(sidleGroup, ACTION_START, -1, AGS_NONE);
                                    StartAnimOn1stPerson(sidleGroup, ACTION_START);
                                }
                            }
                        }

                        panim->PlayGroup(sanimGroup, ACTION_START, -1, AGS_NONE);
                        if (eanimAction != ANIM_ACTION_NONE && !TESAnimGroup::IsIdleAnim(sanimGroup)) {
                            SetAnimAction(
                                eanimAction,
                                panim->GetCurrentSequence(animGroupInfo[eanimGroup].eSectionLayer)
                            );
                        }
                        StartAnimOn1stPerson(sanimGroup, ACTION_START);
                        if (eanimGroup == ANIM_GROUP_JUMP_START) {
                            ANIM_GROUP_ENUM equeuedLoop = ANIM_GROUP_JUMP_LOOP;
                            if ((smoveMode & 15) != 0) {
                                if ((smoveMode & 1) != 0) {
                                    equeuedLoop = ANIM_GROUP_JUMP_LOOP_FORWARD;
                                }
                                else if ((smoveMode & 2) != 0) {
                                    equeuedLoop = ANIM_GROUP_JUMP_LOOP_BACKWARD;
                                }
                                else if ((smoveMode & 4) != 0) {
                                    equeuedLoop = ANIM_GROUP_JUMP_LOOP_LEFT;
                                }
                                else if ((smoveMode & 8) != 0) {
                                    equeuedLoop = ANIM_GROUP_JUMP_LOOP_RIGHT;
                                }
                            }
                            bpowerArmor = HasBackPackWorn() || IsPowerBodyArmorWorn();
                            sbaseGroup = TESAnimGroup::AnimGroup(eanimMove, eanimWeapon, equeuedLoop, bpowerArmor);
                            uint16_t sloopGroup = panim->PickBestAnimation(sbaseGroup, false);
                            panim->PlayGroup(sloopGroup, ACTION_NORMAL, -1, AGS_NONE);
                            StartAnimOn1stPerson(sloopGroup, ACTION_NORMAL);
                        }
                        else if (
                            eanimGroup >= ANIM_GROUP_JUMP_LAND_FORWARD &&
                            eanimGroup <= ANIM_GROUP_JUMP_LAND_RIGHT &&
                            (smoveMode & 15) != 0
                        ) {
                            ANIM_GROUP_ENUM equeuedLoop = ANIM_GROUP_NONE;
                            if ((smoveMode & 512) != 0) {
                                if ((smoveMode & 1) != 0) {
                                    equeuedLoop = ANIM_GROUP_FAST_FORWARD;
                                }
                                else if ((smoveMode & 2) != 0) {
                                    equeuedLoop = ANIM_GROUP_FAST_BACK;
                                }
                                else if ((smoveMode & 4) != 0) {
                                    equeuedLoop = ANIM_GROUP_FAST_LEFT;
                                }
                                else if ((smoveMode & 8) != 0) {
                                    equeuedLoop = ANIM_GROUP_FAST_RIGHT;
                                }
                            }
                            else if ((smoveMode & 1) != 0) {
                                equeuedLoop = ANIM_GROUP_MOVE_FORWARD;
                            }
                            else if ((smoveMode & 2) != 0) {
                                equeuedLoop = ANIM_GROUP_MOVE_BACK;
                            }
                            else if ((smoveMode & 4) != 0) {
                                equeuedLoop = ANIM_GROUP_MOVE_LEFT;
                            }
                            else if ((smoveMode & 8) != 0) {
                                equeuedLoop = ANIM_GROUP_MOVE_RIGHT;
                            }
                            if (equeuedLoop != ANIM_GROUP_NONE) {
                                bpowerArmor = HasBackPackWorn() || IsPowerBodyArmorWorn();
                                sbaseGroup = TESAnimGroup::AnimGroup(eanimMove, eanimWeapon, equeuedLoop, bpowerArmor);
                                uint16_t sloopGroup = panim->PickBestAnimation(sbaseGroup, false);
                                panim->PlayGroup(sloopGroup, ACTION_NORMAL, -1, AGS_NONE);
                                StartAnimOn1stPerson(sloopGroup, ACTION_NORMAL);
                            }
                        }
                    }

                    if (panim->GetCurrentSequence(AGS_MOVEMENT) &&
                        animGroupInfo[eanimGroup].eSectionLayer != AGS_MOVEMENT) {
                        bool bstopAnim = true;
                        if (eact != ANIM_ACTION_NONE) {
                            BSAnimGroupSequence* pactionSeq = pCurrentProcess->GetAnimActionAnimSeq();
                            if (pactionSeq == panim->GetCurrentSequence(AGS_MOVEMENT)) {
                                bstopAnim = false;
                            }
                        }
                        BSAnimGroupSequence* pidleSeq = panim->SpecialIdleGetSequence();
                        if (pidleSeq == panim->GetCurrentSequence(AGS_MOVEMENT)) {
                            bstopAnim = false;
                        }
                        if (panim->GetCurrentSequence(AGS_MOVEMENT)->GetState() != NiControllerSequence::kAnimating) {
                            bstopAnim = false;
                        }
                        if (bstopAnim) {
                            panim->BlendOut(AGS_MOVEMENT, false);
                            if (this == pPlayer) {
                                pPlayer->GetAnimation(true)->BlendOut(AGS_MOVEMENT, false);
                            }
                        }
                    }

                    if (panim->GetCurrentSequence(AGS_WEAPON)) {
                        if (TESAnimGroup::IsAttackAction(panim->GetGroup(AGS_WEAPON))) {
                            NiPoint3 kweaponPos = GetWeaponPosition(false);
                            pCurrentProcess->SaveWeaponLastPos(kweaponPos);
                        }
                    }
                }
            }
        }
    }
}

bool Actor::SetPathfindingGoal(
    const NiPoint3& apPosition,
    TESObjectCELL* apCell,
    TESWorldSpace* apWorldSpace,
    float afRadius,
    PathingAvoidNodeArray* apAvoidNodes
) {
    if (!apAvoidNodes) {
        apAvoidNodes = MobileObject::GetCurrentAIProcess()->CreateAvoidArray(this);
    }
    return pActorMover->SetGoal(apPosition, apCell, apWorldSpace, afRadius, apAvoidNodes);
}

void Actor::PickpocketAlarm(TESObjectREFR* apRef, TESBoundObject* apObj, int32_t ainumb) {
    if (this == pPlayer) {
        iPickPocketWarnings.Int();
        Actor* ptargactor = dynamic_cast<Actor*>(apRef);
        if (ptargactor->GetIgnoreCrime() || ptargactor->IsTresPassing()) {
            ptargactor->pCurrentProcess->SetPickpocketed(true);
            return;
        }
        if (this == pPlayer || (GetClampedActorValue(ActorValue::SNEAK) != 100 || !IsSneaking())) {
            Crime* pcrime;
            pcrime = new Crime(1u, apRef, this, apObj, ainumb, nullptr);
            if (!pcrime || apRef != pcrime->pcrimetarget) {
                pcrime->pcrimetarget->IsAnOwner(static_cast<Actor*>(apRef), true);
            }
            // The victim marks the thief seen: its existing detection of the criminal, else a new one.
            Actor* pvictim = static_cast<Actor*>(apRef);
            DetectionState* pdetect = pvictim->pCurrentProcess->GetDetectionState(pcrime->pcriminal, 0);
            DETECTION_LEVEL elevel = DETECTION_SEEN;
            if (pdetect) {
                pdetect->eDetectionLevel = elevel;
            }
            else {
                pvictim->pCurrentProcess->InsertIntoDetectionArray(pvictim, elevel);
            }
            ptargactor->pCurrentProcess->SetPickpocketed(true);
            float fTimeStamp = appCalendar.GetHour();
            apRef->m_Extra.AddToPlayerCrimeList(pcrime, fTimeStamp);
            pcrime->AddtoActorKnowList(this);
            ReferenceProcessLists.AddtoGlobalCrimeList(pcrime);
            static_cast<Actor*>(apRef)->Actor::IncrementMinorCrimes();
            static_cast<Actor*>(apRef)->Actor::AddFactionMinorCrime(1, true);
            if (bAttackOnNextTheft) {
                bool battack = true;
                if (iThiefCrimeStamp > 0) {
                    int32_t iday = iThiefCrimeStamp & 511;
                    if (iday != appCalendar.GetDay()) {
                        battack = false;
                        bAttackOnNextTheft = false;
                    }
                }
                if (battack) {
                    StartCombat(pPlayer, nullptr, true, false, false, 0, true, nullptr);
                    return;
                }
            }
            else {
                bAttackOnNextTheft = true;
                int32_t iMonthBits = static_cast<int32_t>(appCalendar.GetMonth()) << 9;
                int32_t idayBits = appCalendar.GetDay() | iMonthBits;
                iThiefCrimeStamp = appCalendar.GetYear() << 13 | idayBits;
            }
            static_cast<Actor*>(apRef)->Actor::InitiatePickPocketWarning(this);
        }
    }
}

TESClass* Actor::GetClass() {
    TESObjectREFR* pReference = this;
    if (pReference->IsNpc()) {
        TESBoundObject* pBaseForm = pReference->GetObjectReference();
        if (pBaseForm != nullptr) {
            return static_cast<TESNPC*>(pBaseForm)->GetClass();
        }
    }
    return nullptr;
}

// Whether apActor is in the actor's current combat target array (only while the actor is in combat).
bool Actor::IsInCombatWithActor(Actor* apActor) {
    return IsInCombat() &&
        pCurrentCombatTargetArray &&
        pCurrentCombatTargetArray->Find(apActor, 0, &BSSimpleArray<Actor*>::DefaultEqualFunctionByAddress) != -1;
}

bool Actor::GetShouldAttackActor(Actor* apActor, bool bAttacked, FIGHT_REACTION& aeReaction, bool abIgnoreCombat) {
    FIGHT_REACTION eFactionFight;
    ACTOR_AGGRESSION eAggresion;
    bool bAttack;
    // The neutral reaction is stored before anything else; its only reader is the recursive call
    // below, which writes it before reading it.
    FIGHT_REACTION ereaction;
    ereaction = FIGHT_REACTION_NEUTRAL;
    bool bIsCompanion = false;
    bool bIsPlayerTeammate = IsPlayerTeammate();
    if (bIsPlayerTeammate) {
        if (apActor == pPlayer) {
            return IsAngryWithPlayer();
        }
        if (apActor->IsChild(false)) {
            return false;
        }
        if (GetLocalVariableOnActor("FollowerSwitchAggressive") == 0.0) {
            return apActor->IsInCombat() &&
                (apActor->IsInCombatWithActor(pPlayer) || apActor->IsInCombatWithActor(this));
        }
        bIsCompanion = true;
    }
    else if (apActor->IsPlayerTeammate()) {
        return GetShouldAttackActor(pPlayer, bAttacked, ereaction, false);
    }
    if (bIsCompanion) {
        eAggresion = ACTOR_AGGRESSION_AGGRESSIVE;
    }
    else {
        eAggresion = GetActorAggression();
    }
    if (eAggresion == ACTOR_AGGRESSION_FRENZIED) {
        return true;
    }
    if ((apActor->IsInCombatWithActor(this) || (IsPlayerTeammate() && apActor->IsInCombatWithActor(pPlayer))) &&
        !abIgnoreCombat) {
        bAttacked = true;
    }
    bool bcrimecaused = false;
    if (bIsCompanion && apActor) {
        eFactionFight = apActor->GetFactionFightReaction(pPlayer, bcrimecaused);
    }
    else {
        eFactionFight = GetFactionFightReaction(apActor, bcrimecaused);
    }
    aeReaction = eFactionFight;
    bAttack = false;
    if (apActor == pPlayer && IsAngryWithPlayer()) {
        bAttack = true;
    }
    else if (bAttacked) {
        if (apActor->GetActorAggression() == 3 ||
            (eFactionFight != FIGHT_REACTION_FRIEND && eFactionFight != FIGHT_REACTION_ALLY)) {
            bAttack = true;
        }
    }
    else {
        switch (eAggresion) {
            case ACTOR_AGGRESSION_AGGRESSIVE: {
                if (eFactionFight == FIGHT_REACTION_ENEMY) {
                    if (bIsPlayerTeammate) {
                        if (pPlayer->IsTargetPerceivedAndHostile(apActor)) {
                            bAttack = true;
                        }
                    }
                    else {
                        bAttack = true;
                    }
                }
                break;
            }
            case ACTOR_AGGRESSION_VERY_AGGRESSIVE: {
                if (eFactionFight <= FIGHT_REACTION_ENEMY) {
                    bAttack = true;
                }
                break;
            }
            case ACTOR_AGGRESSION_FRENZIED: {
                bAttack = true;
                break;
            }
        }
    }
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::GET_SHOULD_ATTACK, this, apActor, &bAttack);
    return bAttack;
}

// Out of range of the player, or a sneaking out-of-combat player's out-of-combat teammate, gives -100. With
// abRunFormula the formula runs on what the actor perceives of apActor and, for an attacked actor or the player, both
// detection lists are updated; otherwise the stored level is returned. The light level and the night-eye bonus are
// truncated to int before multiplying, and the ambush value is passed as an int. The 360-degree flag is a local set to
// true, so its else branch is dead code.
int32_t Actor::GetDetectionLevelAgainstActor(
    bool abRunFormula,
    Actor* apActor,
    bool& abLineofSight,
    bool binCombat,
    bool abattacked,
    bool abCombat,
    bool* ap360LineOfSight
) {
    if (!GetCurrentAIProcess()) {
        return 0;
    }
    int32_t idetection = 0x7FFFFFFF;
    abLineofSight = false;
    float fHasImprovedDetect = 0.0f;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_IMPROVED_DETECTION, this, &fHasImprovedDetect);
    if (GetDistanceFromReference(pPlayer, false, false) >= 8192.0 ||
        apActor->GetDistanceFromReference(pPlayer, false, false) >= 8192.0) {
        return -100;
    }
    if (apActor->IsPlayerTeammate() && !apActor->IsInCombat() && pPlayer->IsSneaking() && !pPlayer->IsInCombat()) {
        return -100;
    }
    BaseProcess* pProcess = GetCurrentAIProcess();
    DetectionState* pstate = pProcess->GetDetectionState(apActor, 0);
    if (pstate) {
        idetection = pstate->iLevel;
        abLineofSight = pstate->bLineofSight;
    }
    if (!apActor->Get3D()) {
        idetection = -100;
    }
    else if (abRunFormula) {
        pProcess = GetCurrentAIProcess();
        TESPackage* ppack = pProcess->GetCurrentPackage();
        float fAmbushValue = 0.0f;
        if (ppack && ppack->GetPackType() == PACKAGE_TYPE_AMBUSH) {
            pProcess = GetCurrentAIProcess();
            if (pProcess->IsActorInsideAmbushArea(this, apActor)) {
                fAmbushValue = 1.0f;
            }
            else {
                fAmbushValue = 2.0f;
            }
        }
        float fdistance = GetDistanceFromReference(apActor, false, false);
        ++Main::iNumberActorsDetection;
        pProcess = apActor->GetCurrentAIProcess();
        int32_t ilightlevel = static_cast<int32_t>(pProcess->GetActorLightLevel());
        if (GetActorFloatValue(ActorValue::NIGHT_EYE_BONUS) > 0.0) {
            ilightlevel *= static_cast<int32_t>(fDetectionNightEyeBonus.Float());
            if (ilightlevel > 100) {
                ilightlevel = 100;
            }
        }
        ACTOR_SEGMENT_INVIEW einView = ACTOR_SEGMENT_INVIEW_NONE;
        bool b360 = true;
        bool bInSight = LineOfSight(true, apActor, true, &einView, b360);
        if (GetCombatController() && static_cast<CombatController*>(QCombatController())->GetTarget() == apActor) {
            static_cast<CombatController*>(QCombatController())->SetTargetSegmentInView(einView);
        }
        if (!b360) {
            abLineofSight = bInSight;
        }
        else {
            *ap360LineOfSight = bInSight;
            if (bInSight) {
                float fconeFOV = fDetectionViewCone.Float() * 0.01745329238474369;
                abLineofSight = IsPointInViewCone(*apActor->QLocationOnReference(), fconeFOV);
            }
            else {
                abLineofSight = false;
            }
        }
        ItemChange* parmor = apActor->GetArmorBeingWorn(BO_UPPER_BODY);
        int32_t ibootweight = apActor->GetBootWeight(parmor);
        int32_t iArmorPenalty = 0;
        TESObjectARMO* pArmor = parmor ? static_cast<TESObjectARMO*>(parmor->GetContainerObject()) : nullptr;
        if (parmor) {
            delete parmor;
        }
        if (pArmor) {
            if (pArmor->IsHeavy()) {
                iArmorPenalty += iHeavyArmorStealthPenalty.UInt();
            }
            else if (pArmor->IsMedium()) {
                iArmorPenalty += iMediumArmorStealthPenalty.UInt();
            }
        }
        bool bAlert = IsInCombat();
        int32_t iAction = apActor->GetActionSoundValue();
        int32_t iVisual = 0;
        if (abLineofSight) {
            iVisual = 1;
        }
        bool bmovingB = apActor->IsMoving();
        bool bRunning = false;
        if (apActor->GetMoveMode() & 0x200) {
            bRunning = true;
        }
        float fresult = 0.0f;
        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::IGNORE_RUNNING_DURING_DETECTION, apActor, &fresult);
        if (fresult != 0.0) {
            bRunning = false;
        }
        bool bsneakingB = apActor->IsSneaking();
        TESObjectCELL* pcell = GetParentCell();
        IsPointDeepUnderWater(GetLocationOnReference(), pcell, 0.875f);
        bool bSleeping = IsSleeping();
        int32_t fpercepA = static_cast<int32_t>(GetClampedActorFloatValue(ActorValue::PERCEPTION));
        int32_t fsneakB = static_cast<int32_t>(apActor->GetClampedActorFloatValue(ActorValue::SNEAK));
        if (apActor->IsPlayerTeammate()) {
            int32_t fPlayerSneakB = static_cast<int32_t>(pPlayer->GetClampedActorFloatValue(ActorValue::SNEAK));
            if (fPlayerSneakB > fsneakB) {
                fsneakB = fPlayerSneakB;
            }
        }
        bool binvisibility = false;
        if (apActor->GetActorFloatValue(ActorValue::INVISIBILITY) > 0.0 ||
            apActor->GetActorValue(ActorValue::CHAMELEON) > 0) {
            binvisibility = true;
        }
        if (apActor->IsPlayerTeammate() &&
            (pPlayer->GetActorFloatValue(ActorValue::INVISIBILITY) > 0.0 ||
             pPlayer->GetActorValue(ActorValue::CHAMELEON) > 0)) {
            binvisibility = true;
        }
        int32_t iBlindess = 0;
        int32_t iSize = AiFormulas::GetActorDetectionSize(GetHeight());
        float fsoundsubtotal = 0.0f;
        float fvisualsubtotal = 0.0f;
        float fskillsubtotal = 0.0f;
        bool bExterior = true;
        if (GetParentCell() && GetParentCell()->GetInterior()) {
            bExterior = false;
        }
        bool bisnotacombattarget = false;
        if (!GetSearchingInCombat() && GetCurrentCombatTarget() && apActor != GetCurrentCombatTarget()) {
            bisnotacombattarget = true;
        }
        fresult = 0.0f;
        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::IGNORE_RUNNING_DURING_DETECTION, apActor, &fresult);
        if (fresult != 0.0 && bsneakingB) {
            bmovingB = false;
        }
        int32_t iLevelA = GetActorLevel();
        int32_t iLevelB = apActor->GetActorLevel();
        if (fHasImprovedDetect > 0.0) {
            bInSight = true;
            fdistance *= fImprovedDetectModifier.Float();
            binvisibility = false;
            bsneakingB = false;
        }
        idetection = AiFormulas::GetActorDetectedTarget(
            fpercepA,
            fsneakB,
            bInSight,
            iVisual,
            fdistance,
            ilightlevel,
            iBlindess,
            binvisibility,
            ibootweight,
            bmovingB,
            bsneakingB,
            bRunning,
            bSleeping,
            bAlert,
            iAction,
            iSize,
            bExterior,
            bisnotacombattarget,
            static_cast<int32_t>(fAmbushValue),
            fsoundsubtotal,
            fvisualsubtotal,
            fskillsubtotal,
            iLevelA,
            iLevelB,
            iArmorPenalty
        );
        if (!abRunFormula || abattacked || this == pPlayer) {
            DETECTION_LEVEL elevel = DETECTION_LOST;
            if (idetection > 0) {
                elevel = DETECTION_SEEN;
            }
            if (apActor->IsActor()) {
                bool b360View = ap360LineOfSight ? *ap360LineOfSight : abLineofSight;
                pProcess = GetCurrentAIProcess();
                pProcess->InsertIntoDetectionList(apActor, elevel, abLineofSight, idetection, 0, b360View, binCombat);
                if (idetection > 0) {
                    b360View = ap360LineOfSight ? *ap360LineOfSight : abLineofSight;
                    pProcess = apActor->GetCurrentAIProcess();
                    pProcess->InsertIntoDetectionList(this, elevel, abLineofSight, idetection, 1, b360View, binCombat);
                }
            }
        }
    }
    else if (idetection == 0x7FFFFFFF) {
        idetection = -100;
    }
    else if (pstate && ap360LineOfSight) {
        *ap360LineOfSight = pstate->b360view;
    }
    return idetection;
}

bool Actor::LineOfSight(
    bool abRunFormula,
    TESObjectREFR* pTarget,
    bool abUseEyeLevel,
    ACTOR_SEGMENT_INVIEW* aeInView,
    bool abfull360
) {
    PlayerCharacter* pSelf = static_cast<PlayerCharacter*>(this);
    bool bInSight = false;
    Actor* pActorTarget = nullptr;
    if (!pTarget || !pTarget->Get3D()) {
        return bInSight;
    }
    if (pTarget->IsDead(false) && (!pSelf->IsPc() && !abRunFormula)) {
        return bInSight;
    }
    if (pTarget->IsActor()) {
        pActorTarget = static_cast<Actor*>(pTarget);
    }
    if (pSelf->GetDistanceFromReference(pTarget, false, false) <= 2.0) {
        return true;
    }
    if (!abRunFormula) {
        if (pSelf->pCurrentProcess) {
            return pSelf->pCurrentProcess->GetLineSightonActor(pSelf, pActorTarget, 0, abfull360);
        }
        return bInSight;
    }
    if (aeInView) {
        *aeInView = ACTOR_SEGMENT_INVIEW_NONE;
    }
    if (!abfull360) {
        float fviewCone = fDetectionViewCone.Float() * 0.01745329238474369;
        if (!pSelf->IsPointInViewCone(*pTarget->QLocationOnReference(), fviewCone)) {
            return bInSight;
        }
    }
    if (!Actor::bAllPointsVisible) {
        const NiPoint3* pkOrigin;
        TESObjectCELL* pcell = pSelf->GetParentCell();
        if (!pcell) {
            return false;
        }
        NiAVObject* phead = nullptr;
        TESActorBase* pbodyPartData = pSelf->GetActorBase();
        if (pbodyPartData->GetBodyPartData() && pSelf != pPlayer) {
            phead = pSelf->pCurrentProcess->GetHeadNode();
        }

        if (phead) {
            pkOrigin = &phead->GetWorldTranslate();
        }
        else {
            pkOrigin = pSelf->QLocationOnReference();
        }
        NiPoint3 origPoint = *pkOrigin;
        float factorHeight = 0.0f;
        if (pSelf->IsSneaking()) {
            factorHeight = pSelf->GetSneakHeight(abUseEyeLevel);
        }
        else {
            double deyeHeight;
            if (abUseEyeLevel) {
                deyeHeight = pSelf->GetEyeLevel();
            }
            else {
                deyeHeight = pSelf->GetHeight() * 0.75;
            }
            factorHeight = deyeHeight;
        }
        if (!phead) {
            origPoint.z = origPoint.z + factorHeight;
        }
        if (TES::pInstance->IsCellLoaded(pcell, true)) {
            hkVector4 temp;
            NiAVObject* pdestNode[3];
            ACTOR_SEGMENT_INVIEW esegments[3];
            float fzs[3];
            alignas(16) CFilter icolFilter;
            bhkPickData pickRes;
            MobileObject* pmob = nullptr;
            if (pTarget->IsMobileObject()) {
                pmob = static_cast<MobileObject*>(pTarget);
            }
            SpecificItemCollector collector(L_LOS, pmob);
            pickRes.SetClosestCollector(&collector);
            NiPoint3 destPoint = *pTarget->QLocationOnReference();
            pickRes.NiSetFrom(origPoint);
            pSelf->GetCollisionFilter(&icolFilter.iFilter);
            CFilter fltr(0u);
            fltr.SetLayer(L_LOS);
            uint32_t uiGroup = icolFilter.GetGroup();
            fltr.SetGroup(uiGroup);
            pickRes.SetFilter(fltr);
            double dScale = pTarget->GetScale();
            alignas(16) NiPoint3 kBoundMax(pTarget->GetBoundMax());
            float ftargetHeight = (kBoundMax.z - pTarget->GetBoundMin().z) * dScale;
            fzs[0] = 0.75f;
            fzs[1] = 0.5f;
            fzs[2] = 0.25f;
            esegments[0] = ACTOR_SEGMENT_INVIEW_TOP;
            esegments[1] = ACTOR_SEGMENT_INVIEW_MIDDLE;
            esegments[2] = ACTOR_SEGMENT_INVIEW_BOTTOM;
            std::memset(pdestNode, 0, sizeof(pdestNode));
            if (pActorTarget) {
                TESActorBase* pTargetBaseForm = pActorTarget->GetActorBase();
                if (pTargetBaseForm->GetBodyPartData()) {
                    BaseProcess* pTargetProc = pActorTarget->pCurrentProcess;
                    pdestNode[0] = pTargetProc->GetHeadNode();
                    pdestNode[1] = pActorTarget->pCurrentProcess->GetTorsoNode();
                }
            }
            float z = destPoint.z;
            int32_t ipickcount = 1;
            if ((pActorTarget && pActorTarget == pPlayer) ||
                (pActorTarget == pSelf->pCurrentCombatTarget || pSelf == pPlayer)) {
                ipickcount = 3;
            }
            char* pcache = nullptr;
            bhkWorld* pw = pcell->GetbhkWorld();
            NI2HKNS(temp, origPoint);
            hkAabb aabb(temp, temp);
            for (int32_t i = 0; i < ipickcount; ++i) {
                if (pdestNode[i]) {
                    NI2HKNS(temp, pdestNode[i]->GetWorldTranslate());
                }
                else {
                    NI2HKNS(temp, destPoint);
                    double dz = ftargetHeight * fzs[i] + z;
                    temp(2) = dz;
                }
                aabb.m_min.setMin4(aabb.m_min, temp);
                aabb.m_max.setMax4(aabb.m_max, temp);
            }
            hkSimdReal kMinScale(bhkConvert::fBS2HkScaleSC);
            aabb.m_min.mul4(kMinScale);
            hkSimdReal kMaxScale(bhkConvert::fBS2HkScaleSC);
            aabb.m_max.mul4(kMaxScale);
            pw->MarkForRead();
            hkpBroadPhase* pBroadPhase = pw->GethkWorld()->m_broadPhase;
            int32_t aabbCacheSize = pBroadPhase->getAabbCacheSize();
            if (aabbCacheSize > 0) {
                pcache = hkAllocateStack<char>(aabbCacheSize, nullptr);
                if (pcache) {
                    pw->GethkWorld()->m_broadPhase->calcAabbCache(aabb, pcache);
                    pickRes.pCache = pcache;
                }
            }
            pw->MarkForRead();
            for (int32_t i = 0; i < ipickcount; ++i) {
                TESObjectREFR* phitref;
                if (pdestNode[i]) {
                    pickRes.NiSetTo(pdestNode[i]->GetWorldTranslate());
                }
                else {
                    destPoint.z = ftargetHeight * fzs[i] + z;
                    pickRes.NiSetTo(destPoint);
                }
                ++Main::iNumberLOSPicks;
                if (pSelf->IsImmobile()) {
                    float fpickLength = (destPoint - origPoint).Length();
                    float fminRatio = fImmobileLOSMinHitDistance / fpickLength;
                    collector.SetMinHitFraction(fminRatio);
                }
                else {
                    collector.SetMinHitFraction(0.0f);
                }
                NiAVObject* phit = TES::pInstance->Pick(pickRes);
                if (phit) {
                    phitref = TESObjectREFR::FindReferenceFor3D(phit);
                }
                else {
                    phitref = nullptr;
                }
                if (!pickRes.bPickFailed && (!phit || phitref == pTarget)) {
                    bInSight = true;
                    if (aeInView) {
                        *aeInView = esegments[i];
                    }
                    break;
                }
            }
            if (pcache) {
                hkDeallocateStack<char>(pcache);
            }
        }
        return bInSight;
    }
    if (aeInView) {
        *aeInView = ACTOR_SEGMENT_INVIEW_TOP;
    }
    return true;
}

CombatController* Actor::QCombatController() {
    return static_cast<CombatController*>(pCurrentProcess->GetCurrentPackage());
}

bool Actor::IsPointInViewCone(const NiPoint3& arPoint, float afConeFOV) {
    float fOutDir;
    NiPoint3 kVector;
    NiPoint3* pkPos = TESObjectREFR::QLocationOnReference();
    kVector = arPoint - *pkPos;
    float fTargetAngle = GetZAngleFromVector(kVector);
    float fStartAngle = GetHeading(false);
    float fAngleDelta = GetAngleDelta(fStartAngle, fTargetAngle, fOutDir);
    return afConeFOV * 0.5 > std::fabs(fAngleDelta);
}

bool Actor::CalculateLOS(NiPoint3* arPos, bool abEyeHeight, bool abSkipConeTest) {
    const NiPoint3* pkOrigin;
    PlayerCharacter* pSelf = static_cast<PlayerCharacter*>(this);
    if (!abSkipConeTest) {
        float fConeFOV = fDetectionViewCone.Float() * 0.01745329238474369;
        if (!pSelf->IsPointInViewCone(*arPos, fConeFOV)) {
            return false;
        }
    }
    TESObjectCELL* pcell = pSelf->GetParentCell();
    if (!pcell) {
        return false;
    }
    NiAVObject* phead = nullptr;
    TESActorBase* pbase = pSelf->GetActorBase();
    if (pbase->GetBodyPartData() && pSelf != pPlayer) {
        phead = pSelf->pCurrentProcess->GetHeadNode();
    }

    if (phead) {
        pkOrigin = &phead->GetWorldTranslate();
    }
    else {
        pkOrigin = pSelf->QLocationOnReference();
    }
    NiPoint3 kOrigin = *pkOrigin;
    float fheightOffset = 0.0f;
    if (pSelf->IsSneaking()) {
        fheightOffset = pSelf->GetSneakHeight(abEyeHeight);
    }
    else {
        double deyeHeight;
        if (abEyeHeight) {
            deyeHeight = pSelf->GetEyeLevel();
        }
        else {
            deyeHeight = pSelf->GetHeight() * 0.75;
        }
        fheightOffset = deyeHeight;
    }
    if (!phead) {
        kOrigin.z = kOrigin.z + fheightOffset;
    }
    if (TES::pInstance->IsCellLoaded(pcell, true)) {
        NiPoint3 kDelta;
        NiPoint3 kHitPoint;
        CFilter kFilter;
        CFilter kActorFilter;
        bhkPickData kPick;
        SpecificItemCollector kCollector(L_LOS, pSelf);
        kPick.SetClosestCollector(&kCollector);
        kPick.NiSetFrom(kOrigin);
        kPick.NiSetTo(*arPos);
        pSelf->GetCollisionFilter(&kActorFilter.iFilter);
        kFilter.iFilter = 0;
        kFilter.SetLayer(L_LOS);
        uint32_t uiGroup = kActorFilter.GetGroup();
        kFilter.SetGroup(uiGroup);
        kPick.SetFilter(kFilter);
        if (!TES::pInstance->Pick(kPick)) {
            return true;
        }
        kPick.NiCalcHitPoint(kHitPoint);
        kDelta = kHitPoint - *arPos;
        if (kDelta.SqrLength() < 25.0) {
            return true;
        }
    }
    return false;
}

ItemChange* Actor::GetArmorBeingWorn(BIPED_OBJECT aePart) {
    ItemChange* pItem = nullptr;
    InventoryChanges* pChanges = GetExtra()->GetContainerChanges();
    if (pChanges) {
        pItem = pChanges->GetWornItem(aePart, true);
    }
    return pItem;
}

int32_t Actor::GetBootWeight(ItemChange* pItem) {
    int32_t iZero = 0;
    if (Main::GetSceneIsComplex()) {
        return 5;
    }
    if (IsCreature()) {
        TESCreature* pBase = static_cast<TESCreature*>(GetObjectReference());
        if (pBase) {
            return static_cast<int32_t>(pBase->GetFootWeight());
        }
    }
    else if (pItem) {
        bool bhardcore = pPlayer->IsHardcore();
        return static_cast<int32_t>(TESWeightForm::GetFormWeight(pItem->pContainerObj, bhardcore));
    }
    return iZero;
}

int32_t Actor::GetActionSoundValue() {
    return iActionValue;
}

// The depth of the point's height under the water, as a fraction of the actor's height, is at least afHeight.
bool Actor::IsPointDeepUnderWater(const NiPoint3& aPoint, TESObjectCELL* apCell, float afHeight) {
    return afHeight <= IsPointDeepUnderWater(aPoint.z, apCell);
}

// Divides the difference at double precision and rounds the depth to float once.
float Actor::IsPointDeepUnderWater(float afPoint, TESObjectCELL* apCell) {
    float fWaterHeight = 0.0f;
    float fDepth = 0.0f;
    if (apCell && apCell != GetParentCell()) {
        fWaterHeight = apCell->GetWaterHeight();
    }
    else {
        fWaterHeight = GetRelevantWaterHeight();
    }
    if (afPoint < fWaterHeight) {
        float fActorHeight = GetHeight();
        fDepth = static_cast<float>((static_cast<double>(fWaterHeight) - afPoint) / fActorHeight);
        if (fDepth > 1.0) {
            fDepth = 1.0f;
        }
    }
    return fDepth;
}

// The player queues its third- and first-person locomotion animations, the second for the first-person skeleton's
// cell; any other actor queues the animation of its own process.
void Actor::QueueReplacementLocomotion(bool abHurt, QueuedFile* apParent) {
    if (this == pPlayer) {
        QueueReplacementLocomotion(abHurt, pPlayer->GetAnimation(false), nullptr, apParent);
        QueueReplacementLocomotion(
            abHurt,
            pPlayer->GetAnimation(true),
            NavMeshInfo::GetCellOrCoords(const_cast<char*>(sFirstPersonSkeleton.String()), 0),
            apParent
        );
    }
    else {
        QueueReplacementLocomotion(abHurt, Actor::GetAnimation(), nullptr, apParent);
    }
}

// The process level is tested as two signed compares (0 <= level <= 1).
void Actor::QueueReplacementLocomotion(
    bool abHurt,
    Animation* apAnimation,
    const char* apCellOrCoords,
    QueuedFile* apParent
) {
    Animation* panim = apAnimation;
    if (!apAnimation) {
        panim = Actor::GetAnimation();
    }
    if (panim || apParent) {
        char cPath[260];
        if (apCellOrCoords) {
            BSstrcpy(cPath, 260, apCellOrCoords);
        }
        else {
            const char* pcModel = GetModel();
            BSstrcpy(cPath, 260, pcModel);
        }
        char* pslash = std::strrchr(cPath, 92);
        if (pslash) {
            BSSimpleList<const char*>* pfileList;
            BSstrcpy(pslash, 260 - (pslash - cPath), "\\Locomotion\\");
            if (abHurt) {
                BSstrcat(pslash, 260 - (pslash - cPath), "Hurt\\");
            }
            else if (this == pPlayer && pPlayer->IsPCToddler() && apCellOrCoords) {
                BSstrcat(pslash, 260 - (pslash - cPath), "Toddler\\");
            }
            BSstrcat(pslash, 260 - (pslash - cPath), "IdleAnims");
            ANIM_GROUP_WEAPON_ENUM eanimWeapon = AGW_NONE;
            if (this == pPlayer) {
                pfileList = pModelLoader->BuildKFFileList(cPath, false, true, ANIM_GROUP_WEAPON_COUNT);
            }
            else {
                PROCESS_TYPE eLevel = pCurrentProcess ? pCurrentProcess->GetProcessLevel() : PROCESS_TYPE_NONE;
                if (pCurrentProcess && eLevel >= PROCESS_TYPE_HIGH && eLevel <= PROCESS_TYPE_MIDDLE_HIGH) {
                    MiddleHighProcess* pprocess = static_cast<MiddleHighProcess*>(pCurrentProcess);
                    if (pprocess->GetCurrentWeapon()) {
                        ItemChange* pweaponItem = pprocess->GetCurrentWeapon();
                        eanimWeapon = weaponToAnimGroupWeapon[static_cast<TESObjectWEAP*>(pweaponItem->pContainerObj)
                                                                  ->TESObjectWEAP::GetWeaponType()];
                    }
                    else {
                        eanimWeapon = static_cast<ANIM_GROUP_WEAPON_ENUM>(GetWantWeaponDrawn());
                    }
                }
                pfileList = pModelLoader->BuildKFFileList(cPath, false, true, eanimWeapon);
            }
            if (pfileList) {
                if (apParent) {
                    IO_TASK_PRIORITY epriority = apParent->GetIOTaskPriority();
                    pModelLoader->QueueKFList(pfileList, epriority, apParent, nullptr);
                }
                else {
                    pModelLoader->CancelReplacementKFList(panim);
                    pModelLoader->QueueReplacementKFList(pfileList, panim, IO_TASK_PRIORITY_VERY_LOW, nullptr, nullptr);
                }
                delete pfileList;
                if (!abHurt && GetActorType()) {
                    if (IsChild(true)) {
                        BSstrcpy(pslash, 260 - (pslash - cPath), "\\Locomotion\\Child\\");
                    }
                    else if (GetSex() == TESActorBase::SEX_FEMALE) {
                        BSstrcpy(pslash, 260 - (pslash - cPath), "\\Locomotion\\Female\\");
                    }
                    else {
                        BSstrcpy(pslash, 260 - (pslash - cPath), "\\Locomotion\\Male\\");
                    }
                    BSstrcat(pslash, 260 - (pslash - cPath), "IdleAnims");
                    pfileList = pModelLoader->BuildKFFileList(cPath, false, false, AGW_NONE);
                    if (pfileList) {
                        if (apParent) {
                            IO_TASK_PRIORITY epriority = apParent->GetIOTaskPriority();
                            pModelLoader->QueueKFList(pfileList, epriority, apParent, nullptr);
                        }
                        else {
                            pModelLoader
                                ->QueueReplacementKFList(pfileList, panim, IO_TASK_PRIORITY_VERY_LOW, nullptr, nullptr);
                        }
                        delete pfileList;
                    }
                }
            }
        }
    }
}

Actor* Actor::GetCurrentCombatTarget() {
    return pCurrentCombatTarget;
}

bool Actor::CanMove() {
    if (IsParalyzed() || IsDead(false)) {
        return false;
    }
    int32_t eAction = GetAnimAction();
    if (eAction >= ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM && eAction <= ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE) {
        return false;
    }
    if (IsKnockedOut()) {
        return false;
    }
    if (IsRestrained() || IsUnConscious()) {
        return false;
    }
    if (GetSitSleepState() != SIT_SLEEP_NORMAL) {
        return false;
    }
    if (this == pPlayer && pPlayer->IsPipboyActive()) {
        return false;
    }
    return true;
}

// The bound's height is kept as a double before scaling it, and the high process's cached height as a float.
float Actor::GetHeight() {
    if (pCurrentProcess && pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
        HighProcess* pProcess = static_cast<HighProcess*>(pCurrentProcess);
        float fHeight = pProcess->GetActorHeight();
        if (fHeight != 0.0) {
            return fHeight;
        }
        NiPoint3 kMax = GetBoundMax();
        NiPoint3 kMin = GetBoundMin();
        double dHeight = static_cast<double>(kMax.z) - kMin.z;
        fHeight = GetScale() * dHeight;
        pProcess->SetActorHeight(fHeight);
        return fHeight;
    }
    NiPoint3 kMax = GetBoundMax();
    NiPoint3 kMin = GetBoundMin();
    double dHeight = static_cast<double>(kMax.z) - kMin.z;
    float fHeight = GetScale() * dHeight;
    return fHeight;
}

// Places the actor's 3D at its position and heading (pitched and rolled by the character controller, then scaled
// by the race). For an actor other than the player while the player is not resting, the multibound rooms are
// retested and the actor relit: every call while it moves or while its dead or dying body is awake, else once.
void Actor::UpdateActor3DPosition() {
    NiNode* pRootNode = Get3D();
    if (!pRootNode) {
        return;
    }
    bhkCharacterController* pCharController = static_cast<bhkCharacterController*>(GetCharController());
    // Reads the position through the non-virtual getter, as SetEditorLocation does.
    NiPoint3 kPosition = *QLocationOnReference();
    float fHeading = 0.0f;
    if (this == pPlayer) {
        fHeading = Actor::GetHeading(true);
    }
    else {
        fHeading = GetHeading(true);
    }
    if (!bSkipHavokThisFrame) {
        pRootNode->SetTranslate(kPosition);
    }
    NiMatrix3 kRotation;
    kRotation.MakeZRotation(fHeading);
    if (pCharController) {
        NiMatrix3 kTilt;
        if (pCharController->QPitchAngle() != 0.0) {
            kTilt.MakeXRotation(pCharController->QPitchAngle());
            kRotation = kRotation * kTilt;
        }
        if (pCharController->QRollAngle() != 0.0) {
            kTilt.MakeYRotation(pCharController->QRollAngle());
            kRotation = kRotation * kTilt;
        }
    }
    pRootNode->SetRotate(MultipleMatrixByRace(kRotation));
    if (this == pPlayer || pPlayer->IsSleepingorResting() || !Get3D()) {
        return;
    }
    bool bDismembered = GetExtra()->GetDismembermentExtra() != nullptr;
    GetParentCell()->TestObjectAgainstMultibounds(this, true, bDismembered);
    if (IsMoving()) {
        NiNode* p3D = Get3D();
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->UpdateObjectLighting(p3D, true);
        bLightingUpdatedNonMoving = false;
        return;
    }
    bool bBodyActive = false;
    if (IsDead(false) || IsDying()) {
        bhkNiCollisionObject* pCollision = bhkUtilFunctions::FindNextCollisionObject(Get3D());
        bhkRigidBody* pBody = pCollision ? static_cast<bhkRigidBody*>(pCollision->GetWorldObject()) : nullptr;
        if (pBody && pBody->IsActive()) {
            NiNode* p3D = Get3D();
            BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->UpdateObjectLighting(p3D, true);
            bLightingUpdatedNonMoving = false;
            bBodyActive = true;
        }
    }
    if (!bLightingUpdatedNonMoving && !bBodyActive) {
        NiNode* p3D = Get3D();
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->UpdateObjectLighting(p3D, false);
        bLightingUpdatedNonMoving = true;
    }
}

// The cached aggression from a process with cached values, else computed now.
ACTOR_AGGRESSION Actor::GetActorAggression() {
    // The cached and computed aggression are int, so they are cast to ACTOR_AGGRESSION.
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        return static_cast<ACTOR_AGGRESSION>(pCurrentProcess->GetCachedActorAggression(this));
    }
    return static_cast<ACTOR_AGGRESSION>(static_cast<CachedValuesOwner*>(this)->CalculateCachedActorAggression());
}

// Builds a new animation for a non-player actor with 3D from its KF files for the current weapon (queued to the task
// thread when 3D tasks are queued), then adds the special animations of its model's folder unless it is dead.
void Actor::ResetLoadedAnimations() {
    if (this == pPlayer) {
        return;
    }
    if (!Get3D()) {
        return;
    }
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        TaskQueueInterface::GetInstance()->QueueActorResetLoadedAnims(this);
        return;
    }
    SetAnimation(nullptr);
    if (pCurrentProcess) {
        // Qualified: inside Actor, AIM alone names MagicCaster's state.
        pCurrentProcess->SetAimSequence(AIM_ENUM::AIM, nullptr);
        pCurrentProcess->SetAimSequence(AIM_UP, nullptr);
        pCurrentProcess->SetAimSequence(AIM_DOWN, nullptr);
    }
    ANIM_GROUP_WEAPON_ENUM eWeaponGroup = AGW_NONE;
    if (GetCurrentAIProcess()) {
        PROCESS_TYPE eLevel = GetCurrentAIProcess()->GetProcessLevel();
        if (eLevel >= PROCESS_TYPE_HIGH && eLevel <= PROCESS_TYPE_MIDDLE_HIGH) {
            BaseProcess* pProcess = GetCurrentAIProcess();
            if (pProcess->GetCurrentWeapon()) {
                TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(
                    pProcess->GetCurrentWeapon()->GetContainerObject()
                );
                eWeaponGroup = weaponToAnimGroupWeapon[pWeapon->GetWeaponType()];
            }
            else if (GetWantWeaponDrawn()) {
                eWeaponGroup = AGW_HAND_TO_HAND;
            }
        }
    }
    Animation* pNewAnimation = new Animation;
    Animation* pAnimation = SetAnimation(pNewAnimation);
    BSSimpleList<const char*>* pAnimList = BuildKFFileList(eWeaponGroup);
    TESActorBase* pBaseForm = GetActorBase();
    NiNode* p3D = Get3D();
    bool bUpdate3D = !IsDead(false);
    NiNode* pRoot = p3D->IsNode();
    if (!pAnimation->Init(pAnimList, pRoot, this, bUpdate3D)) {
        uint32_t iFormID = pBaseForm->GetFormID();
        const char* pEditorID = pBaseForm->GetFormEditorID();
        Warning(
            "ANIMATION: Bad InitAnimation for ActorBase '%s' (%08X). Missing 'Idle' animation.",
            pEditorID,
            iFormID
        );
    }
    if (eLifeState != ACTOR_LIFE_STATE_DEAD && static_cast<TESAnimation*>(pBaseForm)->HasKFFiles()) {
        char acPath[260];
        BSstrcpy(acPath, sizeof(acPath), GetModel());
        char* pSlash = std::strrchr(acPath, '\\');
        if (pSlash) {
            *pSlash = '\0';
            pAnimation->AddSpecialAnimations(
                reinterpret_cast<BSSimpleList<const char*>*>(static_cast<TESAnimation*>(pBaseForm)->GetKFFileList()),
                acPath
            );
        }
    }
}

// Moves the actor to where its package wants it when it is not there: the current package, after the process has
// caught up on missed packages, or the initial package with abUseInitialPackage. The first package outside the main
// loop's idle update becomes the initial one.
void Actor::InitPackageLocations(bool abUseInitialPackage) {
    if (GetDisabled()) {
        return;
    }
    if (!abUseInitialPackage) {
        pCurrentProcess->ComputeLastTimeProcessed();
        if (GetRefPersists()) {
            pCurrentProcess->UpdateMissedPackages(this, false);
        }
        pCurrentProcess->CheckforNewPackage(this, false);
    }
    TESPackage* pPackage = pCurrentProcess->GetCurrentPackage();
    if (!appMain->GetOnIdle() && !pInitialPackage) {
        pInitialPackage = pPackage;
    }
    if (abUseInitialPackage) {
        pPackage = pInitialPackage;
    }
    if (pPackage &&
        pPackage->GetPackType() > PACKAGE_TYPE_ESCORT &&
        !pPackage->IsActorAtLocation(this, false, -1.0f, false)) {
        TESObjectCELL* pCell = pPackage->GetLocationCell(this);
        TESWorldSpace* pWorld = pPackage->GetLocationWorld(this);
        // FLT_MAX.
        SetAngleOnReferenceX(std::numeric_limits<float>::max());
        if ((!pPackage->GetLocation() || pPackage->GetLocation()->GetLocType() != PLT_CELL) &&
            pPackage->GetPackType() != PACKAGE_TYPE_PATROL) {
            SetPosition(pPackage->GetLocationCoord(this));
            MoveRefToNewSpace(this, pCell, pWorld);
        }
    }
    UnlockLockDoorsProcedure();
}

void Actor::ClearInCombat(bool abRestoreHealth) {
    bool bwasInCombat = bInCombat;
    bInCombat = false;
    if (!IsPc() && !IsCreature() && abRestoreHealth && bwasInCombat && GetEssential()) {
        RestoreFullHealthAndConditions();
    }
}

TESObjectWEAP* Actor::GetCurrentWeapon() {
    if (!pCurrentProcess || !pCurrentProcess->GetCurrentWeapon()) {
        return nullptr;
    }
    return static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetContainerObject());
}

ItemChange* Actor::GetBestFoodItem() {
    InventoryChanges* pChanges = GetExtra()->GetContainerChanges();
    return pChanges ? pChanges->GetBestFood() : nullptr;
}

bool Actor::GetBlocked() {
    return GetAnimAction() == 7;
}

bool Actor::HasBackPackWorn() {
    if (pCurrentProcess) {
        return pCurrentProcess->HasBackPackWorn();
    }
    return false;
}

bool Actor::IsPowerBodyArmorWorn() {
    if (pCurrentProcess) {
        return pCurrentProcess->IsPowerBodyArmorWorn();
    }
    return false;
}

bool Actor::IsAnimIdleQuequed() {
    uint32_t uiActions = pCurrentProcess->GetPostAnimationActions();
    return (uiActions & 4) != 0 ||
        (uiActions & 8) != 0 ||
        (uiActions & 0x10) != 0 ||
        (uiActions & 0x40) != 0 ||
        (uiActions & 0x100) != 0 ||
        (uiActions & 0x80) != 0 ||
        (uiActions & 0x400) != 0 ||
        (uiActions & 0x800) != 0 ||
        (uiActions & 0x1000) != 0 ||
        (uiActions & 0x2000) != 0;
}

WeaponObject* Actor::SetHavokWeapon() {
    if (!GetCurrentWeapon()) {
        return nullptr;
    }
    bool abConstraintActive = !pRagdollController || !pRagdollController->GetRagdollAnimEnable();
    return SetHavokWeapon(abConstraintActive);
}

WeaponObject* Actor::SetHavokWeapon(bool abConstraintActive) {
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        TaskQueueInterface::GetInstance()->QueueActorSetHavokWeapon(this, abConstraintActive);
        return nullptr;
    }

    NiAVObject* p3d;
    WeaponObject* pwo = nullptr;
    if (this == pPlayer) {
        p3d = pPlayer->Get3D(false);
    }
    else {
        p3d = TESObjectREFR::Get3D();
    }
    if (p3d && GetCurrentWeapon()) {
        NiAVObject* pav = BSUtilities::GetObjectByName(p3d, FixedStrings::QWeapon());
        if (pav) {
            WeaponObject* pweaponObject;
            CFilter actorFilter;
            bool bforceKeyframed = (!abConstraintActive && (!IsDead(false) && !IsKnockedOut())) ||
                (pRagdollController && pRagdollController->QRagdollFeedbackActive());
            GetCollisionFilter(&actorFilter.iFilter);
            NiCollisionObject* pcol = FindFirstCollisionObject(pav);
            if (pcol) {
                pweaponObject = static_cast<WeaponObject*>(NiObject::DynamicCastStatic(&WeaponObject::ms_RTTI, pcol));
            }
            else {
                pweaponObject = nullptr;
            }
            if (pweaponObject) {
                pweaponObject->UpdateConstraints(abConstraintActive, actorFilter, p3d, bforceKeyframed);
            }
            else {
                pweaponObject = WeaponObject::CreateWeaponObject(actorFilter, p3d, pav, bforceKeyframed);
                if (pweaponObject && !abConstraintActive) {
                    pweaponObject->UpdateConstraints(false, actorFilter, p3d, bforceKeyframed);
                }
            }
            bool bincapacitated = IsUnConscious() || (IsDead(false) || (IsDying() || IsKnockedOut()));
            if (pweaponObject && bincapacitated) {
                pweaponObject->UpdateChildRB3D();
            }
        }
    }

    return pwo;
}

bool Actor::RequestRotate(float afTargetAngle, bool abForceTurn) {
    if (!CanMove()) {
        return true;
    }
    BaseProcess* pProcess = GetCurrentAIProcess();
    if (pProcess &&
        pProcess->GetProcessLevel() == PROCESS_TYPE_HIGH &&
        static_cast<HighProcess*>(pProcess)->GetPathLookAtTarget()) {
        return true;
    }
    return pActorMover->RequestRotateActor(afTargetAngle, abForceTurn);
}

bool Actor::RequestRotate(NiPoint3 akPosition, bool abForceTurn) {
    if (!CanMove()) {
        return true;
    }
    BaseProcess* pProcess = GetCurrentAIProcess();
    if (pProcess &&
        pProcess->GetProcessLevel() == PROCESS_TYPE_HIGH &&
        static_cast<HighProcess*>(pProcess)->GetPathLookAtTarget()) {
        return true;
    }
    return pActorMover->RequestRotateActor(akPosition, abForceTurn);
}

void Actor::SetGhost(bool abflag) {
    m_Extra.SetGhost(abflag);
    if (pCurrentProcess) {
        pCurrentProcess->SetCachedValueFlag(CachedValues::CVF_ACTOR_IS_GHOST, abflag);
    }
    AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
}

void Actor::SetWantWeaponDrawn(bool abTF) {
    if ((!abTF || (GetMoveMode() & 2048) == 0) && (this != pPlayer || !GetIronSights())) {
        BaseProcess* pproc = pCurrentProcess;
        if (pproc && abTF != GetWantWeaponDrawn()) {
            UseActionPoints(ActionPoints::ACTION_TOGGLE_WEAPON_DRAWN);
            pproc->SetWantWeaponDrawn(abTF);
            if (this != pPlayer && !pproc->GetCurrentWeapon() && abTF) {
                Animation* panim = Actor::GetAnimation();
                if (panim) {
                    uint16_t
                        sgroup = TESAnimGroup::AnimGroup(AGM_NONE, AGW_HAND_TO_HAND, ANIM_GROUP_WEAPONS_START, false);
                    if (!panim->GroupLoaded(sgroup)) {
                        const char* pcModel = GetModel();
                        BSSimpleList<const char*>*
                            pfileList = pModelLoader->BuildKFFileList(pcModel, true, false, AGW_HAND_TO_HAND);
                        panim->AddAnimationsFromList(pfileList);
                    }
                }
            }
        }
    }
}

bool Actor::StartAttack(ANIM_GROUP_ENUM aeAttack) {
    TESObjectWEAP* pweapon;
    float fSpeed;
    bool bnoammo;
    if (pCurrentProcess->GetCurrentWeapon()) {
        pweapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
    }
    else {
        pweapon = nullptr;
    }
    ItemChange* pammo = pCurrentProcess->GetCurrentAmmo();
    int32_t iAmmoPerShot = 0;
    if (pweapon) {
        iAmmoPerShot = pweapon->GetAmmoPerShot();
    }
    if (pweapon) {
        bnoammo = false;
        if (pweapon->IsRockItLauncher()) {
            bnoammo = true;
            if (this == pPlayer && pPlayer->GetNumRockItAmmo()) {
                bnoammo = false;
            }
            else {
                Interface::QueueMenuCreate(
                    Interface::OPEN_CONTAINER,
                    nullptr,
                    nullptr,
                    nullptr,
                    Interface::CMM_ROCK_N_LOAD,
                    false
                );
            }
        }
        else if (pweapon->GetCurrentAmmo(this)) {
            if (!pammo || pammo->iNumber < iAmmoPerShot) {
                bnoammo = true;
            }
        }
        else if (pweapon->GetAmmoFormList()) {
            BGSListForm* pammoList = pweapon->GetAmmoFormList();
            TESAmmo* testammo = dynamic_cast<TESAmmo*>(pammoList->ListOfForms.m_item);
            if (testammo) {
                pammo = GetInventoryItem(testammo, 0);
                if (!pammo || pammo->iNumber < iAmmoPerShot) {
                    bnoammo = true;
                }
            }
        }
        if (bnoammo && pammo && pammo->iNumber) {
            bnoammo = false;
        }

        if (bnoammo) {
            Controls* pcontrols = appMain->GetControls();
            if (pweapon->GetAttackFailSound()) {
                bool bisPlayer = this == pPlayer;
                if (this == pPlayer && !pcontrols->GetUserAction(Controls::UACTN_USE, 1)) {
                    return false;
                }
                if (bisPlayer) {
                    BSSoundHandle hfailSound;
                    m_Extra.GetSound(hfailSound);
                    if (!hfailSound.IsValid()) {
                        if (pPlayer->IsActually3rdPerson()) {
                            TESSound* psound = pweapon->GetAttackFailSound();
                            TESSound* pfileSound = pweapon->GetAttackFailSound();
                            const char* pcFilename = pfileSound->GetFilename();
                            hfailSound = BSAudio::QInstance().GetSoundHandleByFilename(pcFilename, 258u, psound);
                            hfailSound.SetPosition(GetLocationOnReference());
                        }
                        else {
                            TESSound* psound = pweapon->GetAttackFailSound();
                            TESSound* pfileSound = pweapon->GetAttackFailSound();
                            const char* pcFilename = pfileSound->GetFilename();
                            hfailSound = BSAudio::QInstance().GetSoundHandleByFilename(
                                pcFilename,
                                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                    BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE,
                                psound
                            );
                        }
                        hfailSound.Play(false);
                        m_Extra.SetSound(hfailSound);
                    }
                }
            }
            pCurrentProcess->SetAutomaticFireAtLeastOnce(false);
            return false;
        }
    }
    if (GetAnimAction() == ANIM_ACTION_RELOAD || GetAnimAction() == ANIM_ACTION_RELOAD_LOOP) {
        return false;
    }
    if (!IsWeaponDrawn()) {
        return false;
    }
    if (!pCurrentProcess->CanAttack()) {
        return false;
    }
    Animation* panim = GetAnimation();
    if (!panim) {
        return false;
    }
    bool canJam = true;
    TESPackage* ppack = pCurrentProcess->GetPackageThatIsRunning();
    if (ppack) {
        canJam = ppack->GetPackType() != PACKAGE_TYPE_USE_ITEM_AT && ppack->GetPackType() != PACKAGE_TYPE_USE_WEAPON;
    }
    if (canJam && pweapon && pweapon->GetCurrentAmmo(this)) {
        int32_t iweaponCondition = pCurrentProcess->GetWeaponCondition(this);
        bool bforceJam = false;
        if (aeAttack >= ANIM_GROUP_JAM_A) {
            bforceJam = aeAttack <= ANIM_GROUP_JAM_Q;
        }
        if (bforceJam ||
            (!pCurrentProcess->IsSustainedFire() &&
             (GetWeaponConditionJamChance(iweaponCondition) > 0.0 &&
              RandomBool(static_cast<float>(GetWeaponConditionJamChance(iweaponCondition)))))) {
            ANIM_GROUP_ENUM ejamAnimGroup;
            if (bforceJam) {
                ejamAnimGroup = aeAttack;
            }
            else {
                ejamAnimGroup = static_cast<ANIM_GROUP_ENUM>(pweapon->GetReloadAnim(false) + 23);
            }
            uint16_t sanimGroup = GetAnimGroup(ejamAnimGroup, nullptr, false, nullptr);
            if (TESAnimGroup::GetType(sanimGroup) == ejamAnimGroup) {
                StartWeaponGroup(ejamAnimGroup, panim);
                SetAnimAction(ANIM_ACTION_RELOAD, panim->GetCurrentSequence(AGS_WEAPON));
                StartAnimOn1stPerson(sanimGroup, ACTION_START);
            }
            return false;
        }
    }

    bool bvatsPlayback = pVATS.eMode == VATS::VATS_PLAYBACK;
    if ((!IsSneaking() || bvatsPlayback) &&
        (IsNpc() || this == pPlayer) &&
        (!pweapon || pweapon->GetWeaponType() == WEAPON_HAND_TO_HAND)) {
        bool bh2hAttackAnim = false;
        if (pweapon) {
            uint32_t uiAttackAnim = pweapon->GetAttackAnim();
            if (uiAttackAnim >= 74) {
                bh2hAttackAnim = uiAttackAnim <= 91;
            }
        }
        float fH2HSkill = static_cast<float>(GetActorValue(ActorValue::UNARMED));
        bool bAttack = false;
        float fUpperCutChance = 0.0f;
        float fCrossChance = 0.0f;
        if (!bh2hAttackAnim && fUpperCutThreshold.Float() < static_cast<double>(fH2HSkill)) {
            bAttack = true;
            fUpperCutChance = fH2HSkill * fUpperCutSkillChance.Float();
        }
        if (!bh2hAttackAnim && fCrossThreshold.Float() < static_cast<double>(fH2HSkill)) {
            bAttack = true;
            fCrossChance = fH2HSkill * fCrossSkillChance.Float();
        }
        if (bAttack && !bvatsPlayback) {
            float fSpecial = RandomFloat(0.0f, 100.0f);
            if (fUpperCutChance >= static_cast<double>(fSpecial)) {
                aeAttack = ANIM_GROUP_ATTACK_6;
            }
            else if (fSpecial < fUpperCutChance + fCrossChance) {
                aeAttack = ANIM_GROUP_ATTACK_7;
            }
        }
    }
    bool bvatsMeleeAttack = false;
    if (bvatsPlayback && pVATS.pMeleeAttack) {
        bvatsMeleeAttack = panim->SpecialIdleLoaded();
    }

    uint16_t sanimGroup = GetAnimGroup(aeAttack, nullptr, false, nullptr);
    if (TESAnimGroup::IsAttackAction(sanimGroup) || bvatsMeleeAttack) {
        bool bretVal = true;
        if (pweapon) {
            fSpeed = pweapon->GetSpeed();
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ATTACK_SPEED_MODIFIER, this, pweapon, &fSpeed);
            ItemChange* pweaponItem = pCurrentProcess->GetCurrentWeapon();
            bool bspeedMod = pweaponItem->HasModEffectActive(8);
            double dattackMult = pweapon->GetAttackSpeed(bspeedMod);
            float fattackSpeed = dattackMult * fSpeed;
            panim->SetAttackSpeed(fattackSpeed);
        }
        else {
            panim->SetAttackSpeed(1.0f);
        }
        BSAnimGroupSequence* pseq = panim->GetCurrentSequence(AGS_WEAPON);
        if (!pseq) {
            return false;
        }
        if (pseq->GetAnimGroup()->IsAimAnim()) {
            if (pseq->GetState() != NiControllerSequence::kAnimating) {
                return false;
            }
            if (panim->GetCurrentSequence(AGS_WEAPON_UP)) {
                if (panim->GetCurrentSequence(AGS_WEAPON_UP)->GetState() != NiControllerSequence::kAnimating) {
                    return false;
                }
            }
            if (panim->GetCurrentSequence(AGS_WEAPON_DOWN)) {
                if (panim->GetCurrentSequence(AGS_WEAPON_DOWN)->GetState() != NiControllerSequence::kAnimating) {
                    return false;
                }
            }
            if (this != pPlayer && IsNpc()) {
                if (pseq->GetAnimGroup()->GetMove() == AGM_WALK) {
                    if (!IsSneaking()) {
                        return false;
                    }
                }
                else if (IsSneaking()) {
                    return false;
                }
            }
        }
        else {
            if (pseq->GetAnimGroup()->IsAttackAction()) {
                ANIM_GROUP_ACTION_TYPE eAction = animGroupInfo[pseq->GetAnimGroup()->GetType()].eAction;
                if (eAction != AGAT_LOOP) {
                    if (eAction == AGAT_ATTACK) {
                        if ((!TESAnimGroup::IsPowerAttackAction(sanimGroup) || pseq->m_fLastScaledTime != 0.0) &&
                            panim->GetGroupAction(AGS_WEAPON) != AGA_SPECIAL_IDLE_STOP) {
                            return false;
                        }
                    }
                    else if (eAction != AGAT_ATTACK_SPIN) {
                        return false;
                    }
                }
            }
        }
        int32_t igroup = sanimGroup;
        if (igroup != panim->GetGroup(AGS_WEAPON) ||
            (aeAttack != ANIM_GROUP_ATTACK_LOOP &&
             (aeAttack != ANIM_GROUP_ATTACK_LOOP_IS &&
              (aeAttack != ANIM_GROUP_ATTACK_SPIN &&
               (aeAttack != ANIM_GROUP_ATTACK_SPIN_IS &&
                (aeAttack != ANIM_GROUP_ATTACK_SPIN_2 && aeAttack != ANIM_GROUP_ATTACK_SPIN_2_IS)))))) {
            if (TESAnimGroup::IsPowerAttackAction(sanimGroup)) {
                bretVal = panim->PlayGroup(sanimGroup, ACTION_START, -1, AGS_NONE) != nullptr;
            }
            else {
                StartWeaponGroup(aeAttack, panim);
            }
            pseq = panim->GetCurrentSequence(AGS_WEAPON);
            if (pseq) {
                if (pCurrentProcess->IsCurrentWeaponGrenadeOrMine()) {
                    SetAnimAction(ANIM_ACTION_ATTACK_THROW, pseq);
                }
                else {
                    SetAnimAction(ANIM_ACTION_ATTACK, pseq);
                }
                if (aeAttack == ANIM_GROUP_STOMP) {
                    TESForm* pform = TESForm::GetFormByEditorID("MeleeStomps");
                    TESIdleForm* pidle = dynamic_cast<TESIdleForm*>(pform);
                    pCurrentProcess->SetCurrentProcessIdle(pidle);
                    pCurrentProcess->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_PICK_IDLE_CALL);
                }
                else {
                    StartAnimOn1stPerson(pseq->GetAnimGroup()->sType, ACTION_START);
                }
            }
        }
        if (pweapon && pweapon->IsAutomaticWeapon()) {
            uint16_t eweaponGroup = TESAnimGroup::GetType(panim->GetGroup(AGS_WEAPON));
            if (eweaponGroup >= 24u && eweaponGroup <= 168u) {
                if (panim->GetCurrentSequence(AGS_WEAPON)->GetState() == NiControllerSequence::kAnimating) {
                    pCurrentProcess->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_FIRE_WEAPON);
                    pCurrentProcess->SetAutomaticFireAtLeastOnce(false);
                }
            }
        }

        return bretVal;
    }

    const char* pcActorName;
    const char* pcWeaponName;
    const char* pwantAnim;
    const char* pplayAnim;
    ANIM_GROUP_ENUM eplayAnim = TESAnimGroup::GetType(sanimGroup);
    if (eplayAnim == ANIM_GROUP_NONE) {
        pplayAnim = "ANIM_GROUP_NONE";
    }
    else {
        pplayAnim = animGroupInfo[eplayAnim].pName;
    }
    if (aeAttack == ANIM_GROUP_NONE) {
        pwantAnim = "ANIM_GROUP_NONE";
    }
    else {
        pwantAnim = animGroupInfo[aeAttack].pName;
    }
    if (pweapon) {
        if (pweapon->GetFormEditorIDLength()) {
            pcWeaponName = pweapon->GetFormEditorID();
        }
        else {
            pcWeaponName = pweapon->GetFullName();
        }
    }
    else {
        pcWeaponName = "hand-to-hand";
    }
    if (GetFormEditorIDLength()) {
        pcActorName = GetFormEditorID();
    }
    else {
        pcActorName = GetFullName();
    }
    Warning(
        "COMBAT: '%s' is starting an attack with weapon '%s' with attack '%s' that has non-attack "
        "group '%s'.",
        pcActorName,
        pcWeaponName,
        pwantAnim,
        pplayAnim
    );
    return false;
}

uint8_t Actor::DidPathingFail() {
    return pActorMover->DidPathingFail();
}

uint8_t Actor::IsWaitingOnPath() {
    return pActorMover->IsWaitingOnPath();
}

uint8_t Actor::IsStoppingMovement() {
    return pActorMover->IsStoppingMovement();
}

PathingRequest* Actor::GetCurrentPathingRequest() {
    return pActorMover->GetCurrentRequest();
}

uint8_t Actor::GetResetAI() {
    return bResetAI;
}

// Moves the actor to aeLifeState: an essential actor that would die goes down instead (if it can be knocked down, else
// stays alive); dying stops movement; death marks the face dead, stops the creature's awake sound and the sounds moving
// with the body, and a teleport fade-in finishes as a normal one.
void Actor::SetLifeState(ACTOR_LIFE_STATE aeLifeState) {
    if (bEssentialTakeNoDamage.Bool() &&
        GetEssential() &&
        (aeLifeState == ACTOR_LIFE_STATE_DEAD || aeLifeState == ACTOR_LIFE_STATE_DYING)) {
        aeLifeState = ACTOR_LIFE_STATE_ESSENTIAL_DOWN;
    }
    if (aeLifeState == ACTOR_LIFE_STATE_ESSENTIAL_DOWN && !CanKnockDown()) {
        aeLifeState = ACTOR_LIFE_STATE_ALIVE;
    }
    if (eLifeState == ACTOR_LIFE_STATE_ALIVE &&
        (aeLifeState == ACTOR_LIFE_STATE_DYING ||
         aeLifeState == ACTOR_LIFE_STATE_DEAD ||
         aeLifeState == ACTOR_LIFE_STATE_ESSENTIAL_DOWN)) {
        StopMoving();
    }
    if (aeLifeState == ACTOR_LIFE_STATE_DEAD) {
        BSFaceGenAnimationData* pFaceAnimData = GetFaceAnimationData();
        if (pFaceAnimData) {
            pFaceAnimData->SetDead(true, true);
        }
        InventoryChanges* pChanges = GetExtra()->GetContainerChanges();
        if (pChanges) {
            pChanges->CleanUpBoundItems(this);
        }
        BSSoundHandle kAwakeSound;
        GetExtra()->GetCreatureAwakeSound(kAwakeSound);
        kAwakeSound.Stop();
        kAwakeSound.Release();
        GetExtra()->SetCreatureAwakeSound(kAwakeSound);
        NiNode* p3D = TESObjectREFR::Get3D();
        static_cast<BSWin32Audio&>(BSAudio::QInstance()).StopMovingSounds(p3D, 0.0f, 0);
    }
    if (aeLifeState != ACTOR_LIFE_STATE_ALIVE &&
        pCurrentProcess &&
        pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
        BaseProcess* pProcess = pCurrentProcess;
        if (pProcess->GetFadeState() == HighProcess::FADE_TELEPORT_IN ||
            pProcess->GetFadeState() == HighProcess::FADE_TELEPORT_OUT) {
            static_cast<HighProcess*>(pProcess)->FadeIn(this, false);
        }
    }
    if (eLifeState != aeLifeState) {
        AddChange(CHANGE_ACTOR_LIFESTATE);
    }
    ACTOR_LIFE_STATE eOldState = eLifeState;
    eLifeState = aeLifeState;
    if (eOldState != eLifeState &&
        ((eOldState == ACTOR_LIFE_STATE_DEAD && eLifeState != ACTOR_LIFE_STATE_DYING) ||
         eLifeState == ACTOR_LIFE_STATE_DEAD)) {
        ResetLoadedAnimations();
    }
}

// The fields an actor starts with, shared by its constructors: a low process (listed with abAddToProcessList, or put on
// the temporary change list while 3D tasks are queued), alive and out of combat, a mover and a ragdoll penetration
// helper.
Actor* Actor::ConstructorInitialize(bool abAddToProcessList) {
    SetStillLoading(true);
    pCurrentProcess = new LowProcess;
    if (abAddToProcessList) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            ReferenceProcessLists.AddActorToTempChangeList(this);
        }
        else {
            ReferenceProcessLists.AddReference(this, PROCESS_TYPE_LOW, false, false, nullptr);
        }
    }
    fLastUpdate = -1.0f;
    iLastSeenTime = 0;
    eLifeState = ACTOR_LIFE_STATE_ALIVE;
    eCriticalStage = ACS_NONE;
    bDeadFlag = false;
    ePersuasionEmotion = DIALOGUE_EMOTION_COUNT;
    bProcessMe = true;
    bMurderAlarm = false;
    fEmotionValue = 0.0f;
    bForceSneak = false;
    bForceRun = false;
    pMyKiller = nullptr;
    pDialogueItemTarget = nullptr;
    pTemplateActorBase = nullptr;
    bForceUpdateQuestTarget = true;
    bInWater = false;
    fHeadTrackTimer = 0.0f;
    bWasInFrustum = true;
    float fMax = fUpdateDelaySecondsMax.Float();
    float fMin = fUpdateDelaySecondsMin.Float();
    fUpdateTargetTimer = RandomFloat(fMin, fMax);
    iVisFlags = 0;
    pEditorLocForm = nullptr;
    fEditorLocZRot = 0.0f;
    EditorLocCoord = NiPoint3::ZERO;
    fCheckMyDeadBodyTimer = 0.0f;
    fDeadBodyAlarm = 3.0f;
    bSetOnDeath = false;
    bContainerReset = true;
    eQueuedattack = ANIM_GROUP_NONE;
    CreateActorMover();
    ClearInCombat(false);
    bSwimming = false;
    pRagdollController = nullptr;
    pPenetrationDetection = new bhkRagdollPenetrationUtil;
    bFootIKInRange = false;
    ResetCachedGunSkill();
    ResetCachedWeaponCondition();
    ResetArmorRating();
    ResetRadiationResistanceMult();
    iActionValue = 0;
    fTimeronAction = 0.0f;
    bReloadTargetQueued = false;
    pBaseValueOverrides.SetZeroValuesAllowed(true);
    bEVPBuffered = false;
    bResetAI = 0;
    bIgnoreChangeAnimationCall = false;
    bBlockPostAnim = false;
    iEmotion = 0;
    iEmotionValue = 0;
    bPlayerTeammate = false;
    bSearchingInCombat = false;
    pCurrentCombatTarget = nullptr;
    pCurrentCombatTargetArray = nullptr;
    pCurrentCombatMemberArray = nullptr;
    cCurrentSitSleepState = SIT_SLEEP_NORMAL;
    bLightingUpdatedNonMoving = false;
    pLastHitData = nullptr;
    bShouldRotateToTrack = true;
    pInitialPackage = nullptr;
    bIgnoreCrime = false;
    bTurretBehavior = false;
    pContinuousBeamPersistant = nullptr;
    bForceHitReaction = false;
    iMajorCrimes = 0;
    iMinorCrimes = 0;
    return this;
}

void Actor::ApplyCriticalStage() {
    NiNode* p3D = Get3D();
    if (!p3D) {
        return;
    }
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        TaskQueueInterface::GetInstance()->QueueActorApplyCriticalStage(this);
        return;
    }
    if (!IsDead(false) && eCriticalStage != ACS_NONE) {
        uint32_t iFormID = GetFormID();
        const char* pEditorID = GetFormEditorID();
        Warning("Applying critical stage %i to actor '%s' (%08X) who is not dead", eCriticalStage, pEditorID, iFormID);
    }
    ACTOR_CRITICAL_STAGE eStage = eCriticalStage;
    if (eStage == ACS_GOO_START) {
        bhkUtilFunctions::ReplaceConstraints(p3D, hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING);
        bhkUtilFunctions::DampBodiesVelocity(p3D, 0.0f);
    }
    else if (eStage == ACS_GOO_END || eStage == ACS_DISINTEGRATE_END) {
        RemoveFromAllWater(false);
        p3D->SetAppCulled(true);
        DetachHavok();
        if (!IsPc() && pMyKiller && pMyKiller->IsPc()) {
            MiscStatManager::Increment(MiscStatManager::MS_DISINTEGRATIONS);
        }
    }
}

// For each dismembered limb whose damage node is not already under its own limb root, an NPC whose starting worn
// objects differ from the ones the limb was cut with (compared once, against the first such limb) gets the limb's
// objects back on its biped, then the limb is dismembered again and, when it was removed, detached. A changed biped
// reloads the targets and flags the 3D for an update.
void Actor::CreateDismemberedLimbs() {
    ExtraDismemberedLimbs* pextraLimbs = GetExtra()->GetDismembermentExtra();
    if (!pextraLimbs) {
        return;
    }
    NiAVObject* p3D = Get3D();
    BGSBodyPartData* pbodyPartData = GetActorBase()->GetBodyPartData();
    if (!p3D || !GetCurrentAIProcess() || !pbodyPartData) {
        return;
    }
    bool bchanged3D = false;
    if (pextraLimbs->sLimbs) {
        TESNPC* pnpc = nullptr;
        Character* pcharacter = nullptr;
        if (IsNpc()) {
            pnpc = static_cast<TESNPC*>(GetActorBase());
            pcharacter = static_cast<Character*>(this);
        }
        bool bfoundFirstObjectArray = false;
        uint32_t ilimbCount = pextraLimbs->GetDismemberedLimbCount();
        for (uint32_t i = 0; i < ilimbCount; ++i) {
            DismemberedLimb* pdismemberedLimb = pextraLimbs->GetDismemberedLimb(i);
            BGSBodyPart::LIMB_ENUM elimb = static_cast<BGSBodyPart::LIMB_ENUM>(pdismemberedLimb->cLimb);
            BGSBodyPart* pbodyPart = pbodyPartData->GetBodyPart(elimb);
            if (!pbodyPart) {
                continue;
            }
            NiNode* plimbRoot = GetCurrentAIProcess()->GetDamageNode(elimb);
            if (plimbRoot) {
                NiAVObject* plimbRootParent = plimbRoot->GetParent();
                NiFixedString limbName(pLimbEnumNames[elimb]);
                bool bunderRoot = plimbRootParent->GetName() == limbName;
                if (bunderRoot) {
                    continue;
                }
            }
            if (pnpc && !pdismemberedLimb->bObjectArrayIdentical) {
                bool bidentical = false;
                if (!bfoundFirstObjectArray) {
                    alignas(BSSimpleArray<TESBoundObject*>)
                        uint8_t startingObjectArrayStorage[sizeof(BSSimpleArray<TESBoundObject*>)];
                    BSSimpleArray<TESBoundObject*>* pstartingObjectArray = ::new (startingObjectArrayStorage)
                        BSSimpleArray<TESBoundObject*>;
                    pnpc->BuildObjectArray(this, pcharacter->GetBiped(), pstartingObjectArray);
                    bidentical = pstartingObjectArray->CompareArrays(pdismemberedLimb->ObjectArray);
                    bfoundFirstObjectArray = true;
                    pstartingObjectArray->~BSSimpleArray<TESBoundObject*>();
                }
                if (!bidentical) {
                    pnpc->LoadObjectArray(this, pcharacter->GetBiped(), &pdismemberedLimb->ObjectArray);
                    bchanged3D = true;
                }
            }
            FillBoneArray(plimbRoot, false);
            UnhideDismemberedLimb(elimb, plimbRoot);
            DismemberLimb(
                static_cast<ActorValue::Index>(pbodyPart->GetActorValue()),
                elimb,
                pdismemberedLimb->bLimbExploded,
                false,
                false,
                true
            );
            if (pdismemberedLimb->bLimbRemoved) {
                RemoveDismemberedLimb(elimb);
            }
        }
    }
    if (bchanged3D) {
        ReloadTargets(false);
        GetCurrentAIProcess()->Set3DUpdateFlag(RESET_MODEL);
    }
}

// Pushes each dismembered limb's node, found among the root's children by its limb name, away from the root's bound
// center at fCombatDismemberedLimbVelocity.
void Actor::UpdateDismemberedLimbVel() {
    ExtraDismemberedLimbs* pdismemberedLimbs = GetExtra()->GetDismembermentExtra();
    if (!pdismemberedLimbs || !Get3D()) {
        return;
    }
    NiNode* proot = Get3D()->IsNode();
    uint32_t uilimbCount = pdismemberedLimbs->GetDismemberedLimbCount();
    if (!proot) {
        return;
    }
    if (uilimbCount) {
        bhkWorld::Activate(proot, true, true, false);
    }
    for (uint32_t ui = 0; ui < uilimbCount; ++ui) {
        NiAVObject* pdism3d = nullptr;
        DismemberedLimb* plimb = pdismemberedLimbs->GetDismemberedLimb(ui);
        NiFixedString dismemberRootName(pLimbEnumNames[plimb->cLimb]);
        for (uint32_t uj = 0; uj < proot->GetArrayCount() && !pdism3d; ++uj) {
            NiAVObject* pchild = proot->GetAt(uj);
            if (pchild && pchild->GetName() == dismemberRootName) {
                pdism3d = pchild;
            }
        }
        if (pdism3d) {
            NiBound bound = proot->GetWorldBound();
            NiPoint3 center = bound.GetCenter();
            NiPoint3 diff = pdism3d->GetWorldTranslate() - center;
            diff.Unitize();
            NiPoint3 velocity = diff * fCombatDismemberedLimbVelocity.Float();
            TESHavokUtilities::AddVelocity(pdism3d, velocity, false);
        }
    }
}

// Cuts limb aeLimb off: an exploding limb whose cut node is already among the root's children is only hidden again;
// otherwise a new root named after the limb gets a copy of the limb (or the exploded remains), placed at the gore
// object's transform offset by the body part's gore translate and rotate, pushed away from the actor's bound center as
// a dynamic Havok body, with the part's explosion, debris, blood particles and decals.
void Actor::DismemberLimb(
    ActorValue::Index aeDismemberLocation,
    BGSBodyPart::LIMB_ENUM aeLimb,
    bool abExplode,
    bool abBodyBloodSpray,
    bool abSpawnEffects,
    bool abExplodeImmediate
) {
    TESCreature* pcreature = dynamic_cast<TESCreature*>(GetActorBase());
    if (((pcreature && pcreature->IsHumanoidCreature()) || IsNpc()) && bDisableAllGore.Bool()) {
        return;
    }
    NiNode* pactorRoot = TESObjectREFR::Get3D();
    bhkWorld* phw = GetParentCell()->GetbhkWorld();
    NiNode* plimbRoot = pCurrentProcess->GetDamageNode(aeLimb);
    NiFixedString pdismemberrootname(pLimbEnumNames[aeLimb]);
    if (abExplode) {
        uint32_t ichildcount = pactorRoot->GetArrayCount();
        for (int32_t i = 0; static_cast<uint32_t>(i) < ichildcount; ++i) {
            NiAVObject* pchild = pactorRoot->GetAt(static_cast<uint32_t>(i));
            if (pchild && NiFixedString::HandlesEqual(pchild->GetName(), pdismemberrootname)) {
                Actor::HideDismemberedLimb(aeLimb, plimbRoot, pchild, abExplode);
                return;
            }
        }
    }
    BGSBodyPartData* pbodyPartData = GetActorBase()->GetBodyPartData();
    BGSBodyPart* pbodyPart = pbodyPartData ? pbodyPartData->GetBodyPart(aeLimb) : nullptr;
    Actor::FillBoneArray(plimbRoot, false);
    NiNode* pclonedLimbRoot = new NiNode(0);
    pclonedLimbRoot->SetName(pdismemberrootname);
    pactorRoot->AttachChild(pclonedLimbRoot, 1);
    NiUpdateData UpdateData(0.0f, false, false);
    pclonedLimbRoot->UpdateWorldData(UpdateData);
    NiAVObject* pgoreObject = pactorRoot->GetObjectByName(pbodyPart->GetGoreObjectName());
    if (!pgoreObject) {
        pgoreObject = plimbRoot;
    }
    NiTransform goreTransform;
    NiTransform offset;
    offset.m_Translate = pbodyPart->GetGoreTranslate();
    // The gore rotation in degrees, read once per axis (z, y, x), times a double constant and stored as floats.
    float frotZ = pbodyPart->GetGoreRotate().z * 0.01745329238474369;
    float frotY = pbodyPart->GetGoreRotate().y * 0.01745329238474369;
    float frotX = pbodyPart->GetGoreRotate().x * 0.01745329238474369;
    NiPoint3 rotation(frotX, frotY, frotZ);
    offset.m_Rotate.FromEulerAnglesXYZ(rotation.x, rotation.y, rotation.z);
    offset.m_fScale = 1.0f;
    NiTransform goreProduct;
    goreTransform = *::new (&goreProduct) NiTransform(pgoreObject->GetWorldTransform() * offset);
    NiAVObject* plimbReplacement = nullptr;
    if (abExplode) {
        plimbReplacement = Actor::ExplodeLimb(aeLimb, pgoreObject, &goreTransform, abSpawnEffects);
    }
    else {
        BSCloningProcess cloneProcess(1.0f);
        cloneProcess.SetScale(1.0f);
        NiObject* pcloneLimbObj = plimbRoot->Clone(cloneProcess);
        plimbReplacement = static_cast<NiAVObject*>(pcloneLimbObj);
        NiAVObject* pweaponObject = BSUtilities::GetObjectByName(plimbReplacement, FixedStrings::QWeapon());
        bool bskinned = false;
        if (pweaponObject && pweaponObject != plimbReplacement && pweaponObject->m_pkParent) {
            // The weapon node stays on the limb only when the actor's weapon has a biped model list one of whose
            // forms the biped wears skinned.
            TESObjectWEAP* pweap = GetBiped() ? static_cast<TESObjectWEAP*>(GetBiped()->object[BO_WEAPON].pParent)
                                              : nullptr;
            BGSListForm* plist = pweap ? static_cast<BGSBipedModelList&>(*pweap).pListForm : nullptr;
            for (BSSimpleList<TESForm*>* pformlist = plist ? &plist->ListOfForms : nullptr;
                 pformlist && pformlist->m_item && !bskinned;
                 pformlist = pformlist->m_pkNext) {
                BIPOBJECT* pbipObj = GetBiped()->FindBipObject(pformlist->m_item);
                bskinned = pbipObj ? pbipObj->bSkinned : false;
            }
            if (!bskinned) {
                pweaponObject->m_pkParent->DetachChild(pweaponObject);
                pweaponObject = nullptr;
            }
        }
        NiTransform rotMult;
        NiTransform rot;
        pactorRoot->GetWorldTransform().InvertNonUniform(rotMult);
        NiTransform rotProduct;
        rot = *::new (&rotProduct) NiTransform(rotMult * plimbRoot->GetWorldTransform());
        plimbReplacement->SetTranslate(rot.m_Translate);
        plimbReplacement->SetRotate(rot.m_Rotate);
        plimbReplacement->SetScale(rot.m_fScale);
        if (abSpawnEffects) {
            if (pbodyPart->Data.pDismemberExplosion) {
                Explosion::SpawnExplosion(
                    pbodyPart->Data.pDismemberExplosion,
                    nullptr,
                    nullptr,
                    GetParentCell(),
                    goreTransform.m_Translate,
                    goreTransform.m_Rotate
                );
            }
            if (pbodyPart->Data.pDismemberGenericDebris && pbodyPart->Data.cDismemberGenericDebrisCount) {
                double dscale = GetScale();
                float fdebrisScale = pbodyPart->Data.fDismemberGenericDebrisScale * dscale;
                pbodyPart->Data.pDismemberGenericDebris->SpawnDebris(
                    GetParentCell(),
                    goreTransform.m_Translate,
                    pbodyPart->Data.cDismemberGenericDebrisCount,
                    fdebrisScale
                );
            }
        }
    }
    if (!plimbReplacement) {
        plimbReplacement = new NiNode(0);
    }
    if (!abExplode) {
        pCurrentProcess->StoreDismemberedLimbNode(aeLimb, plimbReplacement->IsNode());
    }
    if (plimbRoot->GetCollisionObject() && plimbRoot->GetChildCount()) {
        for (int32_t i = 0; static_cast<uint32_t>(i) < plimbRoot->GetArrayCount(); ++i) {
            NiAVObject* pchild = plimbRoot->GetAt(static_cast<uint32_t>(i));
            if (pchild) {
                bhkWorld::RemoveObjects(pchild, true, true);
                bhkWorld::SetDisabled(pchild, true, true, true);
            }
        }
    }
    if (plimbReplacement) {
        bhkCollisionObject* pco = FindFirstCollisionObject(plimbReplacement);
        if (pco) {
            bhkRigidBody* prb = pco->GetRigidBody();
            if (prb) {
                prb->RemoveActionConstraints();
            }
        }
        pclonedLimbRoot->AttachChild(plimbReplacement, 1);
        bhkNiCollisionObject::ResetSim(plimbReplacement, true);
        NiUpdateData UpdateData_2(0.0f, false, false);
        plimbReplacement->Update(UpdateData_2);
        bhkWorld::SetMotion(plimbReplacement, hkpMotion::MOTION_DYNAMIC, true, true, true);
        phw->AddObjects(plimbReplacement, true, false, 0, true);
        NiBound bound = Get3D()->GetWorldBound();
        NiPoint3 center = bound.m_kCenter;
        NiPoint3 diff = plimbReplacement->GetWorldTranslate() - center;
        diff.Unitize();
        NiPoint3 velocity = diff * fCombatDismemberedLimbVelocity.Float();
        TESHavokUtilities::AddVelocity(plimbReplacement, velocity, false);
    }
    if (abSpawnEffects) {
        BGSImpactDataSet* pimpactDataSet = abExplode ? pbodyPart->Data.pExplosionImpactDataSet
                                                     : pbodyPart->GetDismemberImpactDataSet();
        // The base form is cast to TESActorBaseData without a null test.
        BGSImpactData* pimpactData = pimpactDataSet
            ? pimpactDataSet->GetImpactData(static_cast<TESActorBaseData&>(*GetActorBase()).GetBloodImpactMaterial())
            : nullptr;
        TESModel* pbloodModel = pimpactData ? static_cast<TESModel*>(pimpactData) : nullptr;
        if (pbloodModel && pbloodModel->GetModelLength() && eCriticalStage == ACS_NONE) {
            if (abBodyBloodSpray) {
                float fscale = GetScale();
                const char* pmodel = pbloodModel->GetModel();
                BSTempEffectParticle* pparticle = BSTempEffectParticle::Spawn(
                    GetParentCell(),
                    1.0f,
                    pmodel,
                    goreTransform.m_Rotate,
                    goreTransform.m_Translate,
                    fscale,
                    7,
                    pgoreObject
                );
                if (pimpactData->pDecalTextureSet) {
                    pparticle->SetDecalImpactData(pimpactData);
                    BGSDecalEmitter* pnewemitter = new BGSDecalEmitter(
                        pparticle,
                        pimpactData,
                        pbodyPart->GetSeveredDecalCount()
                    );
                    BGSDecalManager::AddEmitter(pnewemitter);
                }
            }
            if (!abExplode) {
                float fscale = GetScale();
                NiMatrix3 inverse;
                NiMatrix3* pinverse = goreTransform.m_Rotate.Inverse(&inverse);
                const char* pmodel = pbloodModel->GetModel();
                BSTempEffectParticle* pparticle = BSTempEffectParticle::Spawn(
                    GetParentCell(),
                    1.0f,
                    pmodel,
                    *pinverse,
                    goreTransform.m_Translate,
                    fscale,
                    7,
                    plimbReplacement
                );
                if (pimpactData->pDecalTextureSet) {
                    pparticle->SetDecalImpactData(pimpactData);
                    BGSDecalEmitter* pnewemitter = new BGSDecalEmitter(
                        pparticle,
                        pimpactData,
                        pbodyPart->GetSeveredDecalCount()
                    );
                    BGSDecalManager::AddEmitter(pnewemitter);
                }
            }
        }
    }
    plimbRoot->SetAppCulled(true);
    if (abExplode && pCurrentProcess && !abExplodeImmediate) {
        pCurrentProcess->DeferHideDismemberedLimb(aeLimb, plimbRoot, plimbReplacement, abExplode);
    }
    else {
        Actor::HideDismemberedLimb(aeLimb, plimbRoot, plimbReplacement, abExplode);
    }
    if (!abExplode && pVATS.eMode == VATS::VATS_PLAYBACK && plimbReplacement) {
        VATS_COMMAND* pcurrentCommand = pVATS.GetCurrentAction();
        if (pcurrentCommand && pcurrentCommand->eLimb == aeDismemberLocation) {
            BGSCameraShot* pcurrentCamera = pVATS.pCamera;
            if (pcurrentCamera) {
                if (pcurrentCamera->Data.eLocation == BGSCameraShot::CO_TARGET) {
                    pcurrentCamera->SetCameraLocation(plimbReplacement);
                }
                if (pcurrentCamera->Data.eTarget == BGSCameraShot::CO_TARGET) {
                    pcurrentCamera->SetCameraTarget(plimbReplacement);
                }
            }
        }
    }
    pactorRoot->UpdateProperties();
    if (pPenetrationDetection) {
        CFilter kfilter;
        MobileObject::GetCollisionFilter(&kfilter.iFilter);
        int32_t igroup = kfilter.GetGroup();
        pPenetrationDetection->Initialize(Get3D(), igroup);
    }
}

void Actor::UnhideDismemberedLimb(BGSBodyPart::LIMB_ENUM aeLimbIndex, NiNode* apDismemberedLimbRoot) {
    NiNode* p3D = TESObjectREFR::Get3D();
    TESActorBase* pActorBase = GetActorBase();
    BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
    BGSBodyPart* pBodyPart = pBodyPartData ? pBodyPartData->GetBodyPart(aeLimbIndex) : nullptr;
    if (pBodyPart && apDismemberedLimbRoot) {
        for (uint32_t i = 0; i < p3D->GetArrayCount(); ++i) {
            NiGeometry* pChild = static_cast<NiGeometry*>(p3D->GetAt(i));
            if (pChild && Animation::FindSkinnedNode(pChild) && IsSkinnedToLimb(pChild, apDismemberedLimbRoot)) {
                if (aeLimbIndex == BGSBodyPart::LIMB_TORSO) {
                    pChild->SetAppCulled(false);
                }
                BGSBodyPart::UnhideLimb(aeLimbIndex, pChild);
                pBodyPartData->ShowChildLimbs(apDismemberedLimbRoot, p3D, aeLimbIndex, pChild);
            }
        }
    }
}

bool Actor::RemoveDismemberedLimb(BGSBodyPart::LIMB_ENUM aeLimb) {
    SetLimbRemoved(aeLimb, true);
    NiNode* pDamageNode = pCurrentProcess->GetDamageNode(aeLimb);
    if (!pDamageNode) {
        return false;
    }
    NiNode* pParent = pDamageNode->m_pkParent;
    if (!pParent) {
        return false;
    }
    pParent->SetAppCulled(true);
    bhkWorld::RemoveObjects(pParent, true, true);
    ReferenceProcessLists.RemoveParticleTempEffectsFor3D(pDamageNode);
    return true;
}

// Fills the file's bone list from apLimb and its children; abRecursing keeps the list collected so far. Beyond the
// 256 bones of the list a warning names the actor.
void Actor::FillBoneArray(NiNode* apLimb, bool abRecursing) {
    if (!abRecursing) {
        ourMemset(pLimbBones, 0, sizeof(pLimbBones));
        uiLimbCount = 0;
    }
    if (!apLimb) {
        return;
    }
    if (!apLimb->IsNode()) {
        return;
    }
    if (uiLimbCount < 0x100) {
        pLimbBones[uiLimbCount] = apLimb;
        uiLimbCount = uiLimbCount + 1;
    }
    else {
        Warning(
            "MODELS: Increase the MAX_LIMB_BONES (%s, %d, %d) define in Actor.cpp to fix dismemberment skin "
            "problems.",
            GetFullName(),
            uiLimbCount,
            256
        );
    }
    for (uint32_t i = 0; i < apLimb->GetArrayCount(); ++i) {
        FillBoneArray(static_cast<NiNode*>(apLimb->GetAt(i)), true);
    }
}

void Actor::RestoreFullHealthAndConditions() {
    float fHealthDamage = GetDamageModifier(ActorValue::HEALTH);
    if (fHealthDamage < 0.0) {
        float fAmount = 1.0 - fHealthDamage;
        RestoreActorValue(ActorValue::HEALTH, fAmount);
    }
    float fFatigueDamage = GetDamageModifier(ActorValue::FATIGUE);
    if (fFatigueDamage < 0.0) {
        float fAmount = 1.0 - fFatigueDamage;
        RestoreActorValue(ActorValue::FATIGUE, fAmount);
    }
    RestoreActorValue(ActorValue::BRAIN_CONDITION, 1000.0f);
    RestoreActorValue(ActorValue::LEFT_MOBILITY_CONDITION, 1000.0f);
    RestoreActorValue(ActorValue::RIGHT_MOBILITY_CONDITION, 1000.0f);
    RestoreActorValue(ActorValue::LEFT_ATTACK_CONDITION, 1000.0f);
    RestoreActorValue(ActorValue::RIGHT_ATTACK_CONDITION, 1000.0f);
    RestoreActorValue(ActorValue::ENDURANCE_CONDITION, 1000.0f);
    RestoreActorValue(ActorValue::PERCEPTION_CONDITION, 1000.0f);
}

bool Actor::CheckSummonedActor() {
    bool bSummoned = false;
    if (!pCurrentProcess) {
        return false;
    }
    if (!pCurrentProcess->GetCommandingActor()) {
        return false;
    }
    Actor* pCommander = pCurrentProcess->GetCommandingActor();
    for (BSSimpleList<ActiveEffect*>* pList = pCommander->GetActiveEffectList(); pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        ActiveEffect* pEffect = pList->m_item;
        if (pEffect) {
            EffectSetting* pEffectSetting = pEffect->GetEffectSetting();
            if (pEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
                SummonCreatureEffect* pSummonEffect = dynamic_cast<SummonCreatureEffect*>(pEffect);
                Actor* pSummonedActor = pSummonEffect ? pSummonEffect->pSummonedCreature : nullptr;
                if (pSummonedActor == this) {
                    pSummonEffect->ClearSummonedCreature();
                    pEffect->Dispel(true);
                    bSummoned = true;
                }
            }
        }
    }
    return bSummoned;
}

void Actor::DispelPermanentMagic() {
    for (SpellItem* pSpell : *&GetActorBase()->spellList) {
        if (pSpell) {
            MagicSystem::SpellType eSpellType = pSpell->GetSpellType();
            if (MagicSystem::IsSpellTypePermanent(eSpellType) && !pSpell->IsPCStartSpell()) {
                Dispel(pSpell, nullptr, nullptr);
            }
        }
    }
    BSSimpleList<SpellItem*>* pSpells = GetRace() ? &GetRace()->spellList : nullptr;
    for (SpellItem* pSpell : *pSpells) {
        if (pSpell) {
            MagicSystem::SpellType eSpellType = pSpell->GetSpellType();
            if (MagicSystem::IsSpellTypePermanent(eSpellType)) {
                Dispel(pSpell, nullptr, nullptr);
            }
        }
    }
    InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(this);
    for (uint32_t i = 0; i < 5; ++i) {
        ItemChange* pItem = pInventory->GetWornItem(WornBodyParts[i], false);
        TESEnchantableForm* pEnchantable = nullptr;
        if (pItem) {
            pEnchantable = dynamic_cast<TESEnchantableForm*>(pItem->pContainerObj);
        }
        EnchantmentItem* pEnchantment = pEnchantable ? pEnchantable->pFormEnchanting : nullptr;
        if (pEnchantment) {
            Dispel(pItem->pContainerObj, false);
        }
        if (pItem) {
            delete pItem;
        }
    }
    TESForm* pEveryActorAbility = BGSDefaultObjectManager::GetDefaultObject(DO_EVERY_ACTOR_ABILITY);
    if (pEveryActorAbility) {
        SpellItem* pSpell = dynamic_cast<SpellItem*>(pEveryActorAbility);
        if (pSpell) {
            MagicSystem::SpellType eSpellType = pSpell->GetSpellType();
            if (MagicSystem::IsSpellTypePermanent(eSpellType)) {
                Dispel(pSpell, nullptr, nullptr);
            }
        }
    }
}

void Actor::SetLastSeenTime() {
    iLastSeenTime = appCalendar.GetHoursPassed();
}

void Actor::EndInterruptPackage(bool bNotRunOnceDialogue) {
    if (pCurrentProcess) {
        if (!bNotRunOnceDialogue) {
            EndDialogue();
        }
        TESPackage* pPackage = pCurrentProcess->GetCurrentPackage();
        if (pPackage == pPlayer->pClosestConversation) {
            pPlayer->pClosestConversation = nullptr;
        }
        bool bStopMoving = false;
        TESPackage* pRunOnce = pCurrentProcess->GetRunOncePackage();
        if (pRunOnce &&
            (!bNotRunOnceDialogue || pRunOnce->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) &&
            pRunOnce->GetPackType() != PACKAGE_TYPE_GET_UP) {
            pCurrentProcess->ClearRunOncePackage();
            bStopMoving = true;
        }

        if (pPackage && pPackage->IsInterruptPackage()) {
            if (pPackage->GetPackType() == PACKAGE_TYPE_TRESPASS) {
                static_cast<TrespassPackage*>(pPackage)->ModNumberActors(-1);
            }
            ClearInCombat(true);
            RemoveChange(0);
            pCurrentProcess->ClearCurrentPackage();
            pCurrentProcess->LoadPackageFromExtraData(this);
            if (!pCurrentProcess->GetCurrentPackage() && !pDataHandler->GetClearingData()) {
                pCurrentProcess->CheckforNewPackage(this, false);
            }
            bStopMoving = true;
        }

        if (bStopMoving) {
            EndMovement();
        }
    }
}

void Actor::MoveActorAndFollowers(
    TESObjectREFR* apMarker,
    bool abUseFollowerSpacing,
    bool abMoveFollowersEvenIfVisible
) {
    TESWorldSpace* pWorldSpace = apMarker->GetWorldSpace();
    TESObjectCELL* pParentCell = apMarker->TESObjectREFR::GetParentCell();
    float fAngle = apMarker->GetAngleOnReference().z;
    MoveActorAndFollowers(
        apMarker->GetLocationOnReference(),
        fAngle,
        pParentCell,
        pWorldSpace,
        abUseFollowerSpacing,
        abMoveFollowersEvenIfVisible
    );
}

void Actor::MoveActorAndFollowers(
    const NiPoint3& aLocation,
    float afAngle,
    TESObjectCELL* apCell,
    TESWorldSpace* apWorldspace,
    bool abUseFollowerSpacing,
    bool abMoveFollowersEvenIfVisible
) {
    float fDummy;
    NiPoint3 farEnoughPt;
    NiPoint3 location = aLocation;
    float y = -std::cos(afAngle);
    float x = -std::sin(afAngle);
    NiPoint3 backward(x, y, 0.0f);
    float fAccumulatedSpace = 0.0f;
    BSSimpleList<Actor*> actorAndFollowers;
    actorAndFollowers.AddHead(this);
    BSSimpleList<Actor*>* scan = &actorAndFollowers;
    // Each actor's followers join the list after it, once each, if they have a process.
    while (scan) {
        ExtraFollower* pFollow = scan->m_item->m_Extra.GetFollowerExtra();
        if (pFollow) {
            for (BSSimpleList<Actor*>* pNode = pFollow->pActorlist; pNode; pNode = pNode->m_pkNext) {
                Actor* pFollower = pNode->m_item;
                if (pFollower && pFollower->pCurrentProcess && !actorAndFollowers.IsInList(pFollower)) {
                    scan->AddTail(pFollower);
                }
            }
        }
        scan = scan->m_pkNext;
    }
    bool bMoveFolowers = abMoveFollowersEvenIfVisible || pPlayer->IsSleepingorResting();
    float fFollowerSpacing = abUseFollowerSpacing ? fFollowSpaceBetweenFollowers.Float() : 0.0f;
    for (scan = &actorAndFollowers; scan; scan = scan->m_pkNext) {
        Actor* pActor = scan->m_item;
        if (!pActor->IsDead(false) &&
            !pActor->GetDisabled() &&
            !pActor->IsWaiting() &&
            !pPlayer->bTravelUseDoor &&
            (bMoveFolowers || pActor == this || pActor == pPlayer || !pActor->Get3D() || !pActor->IsVisible(7))) {
            BaseProcess* pProc = pActor->pCurrentProcess;
            if (pProc && (!pActor->bPlayerTeammate || pActor->GetLocalVariableOnActor("Waiting") != 1.0)) {
                if (pProc->GetSitSleepState()) {
                    uint32_t uiMarkerID = pProc->GetFurnitureMarkerID();
                    pProc->GetCurrentFurniture()->SetMarkerUsed(uiMarkerID, false);
                    pActor->Actor::GetAnimation()->SkipNextBlend();
                    pProc->SetSitSleepState(pActor, SIT_SLEEP_NORMAL, pProc->GetCurrentFurniture(), 127);
                    uint32_t uiMarkerType = pProc->GetFurnitureMarker()->GetType();
                    TESObjectREFR* pFurniture = pProc->GetCurrentFurniture();
                    float fHeadingOffset = -static_cast<TESFurniture*>(pFurniture->GetObjectReference())
                                                ->TESFurniture::GetMarkerHeadingOffset(uiMarkerType);
                    pActor->ModHeading(fHeadingOffset);
                    float fHeading = pActor->GetHeading(false) + 3.1415927410125732;
                    pActor->SetHeading(fHeading);
                    pProc->GetFurnitureMarker()->SetHeading(0.0f);
                    pProc->GetFurnitureMarker()->SetType(0);
                    pProc->GetFurnitureMarker()->SetPosition(NiPoint3::ZERO);
                    pProc->SetFurniture(nullptr, 127, pProc->GetFurnitureMarker());
                    pActor->SetChaseBip(false);
                }
                float fMyRadius = pActor->GetRadius();
                if (pActor != this) {
                    fAccumulatedSpace = fFollowerSpacing + fMyRadius + fAccumulatedSpace;
                    if (apCell && TES::pInstance->IsCellLoaded(apCell, false)) {
                        NiPoint3 adjustedLoc = location + backward * fAccumulatedSpace;
                        PathingSolution followPosSolution;
                        PathingRequest followPosRequest;
                        followPosRequest.Initialize(this);
                        followPosRequest.SetCantOpenDoors(true);
                        followPosRequest.SetDestination(PathingLocation(adjustedLoc, this));
                        followPosRequest.SetAllowIncompletePath(true);
                        if (Pathing::BuildPath(&followPosRequest, &followPosSolution) &&
                            followPosSolution.FindPointOnPath(0.0f, fAccumulatedSpace, true, &fDummy, &farEnoughPt)) {
                            location = farEnoughPt;
                        }
                    }
                }
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    TaskQueueInterface::GetInstance()->QueueActorSetPosition(pActor, location);
                    TaskQueueInterface::GetInstance()->QueueActorMoveCharController(pActor);
                }
                else {
                    pActor->SetPosition(location);
                }
                pActor->SetLocationOnReference(location);
                fAccumulatedSpace = fAccumulatedSpace + fMyRadius;
                if (apCell && TES::pInstance->IsCellLoaded(apCell, false)) {
                    pActor->SetAngleOnReferenceZ(afAngle);
                    pActor->SetAngleOnReferenceX(0.0f);
                }
                else {
                    pActor->SetAngleOnReferenceX(std::numeric_limits<float>::max());
                }
                TESObjectREFR::MoveRefToNewSpace(pActor, apCell, apWorldspace);
                if (pActor != this) {
                    pProc = pActor->pCurrentProcess;
                    pActor->ForceStopMoving();
                    pActor->StopCombat(nullptr);
                    pActor->EndInterruptPackage(false);
                    if (pProc) {
                        pProc->ComputeLastTimeProcessed();
                    }
                }
            }
        }
    }
}

double Actor::GetLocalVariableOnActor(const char* asVariableName) {
    if (this) {
        TESForm* pObject = GetObjectReference();
        Script* pScript = TESScriptableForm::GetFormScript(pObject);
        ScriptLocals* pScriptVariables = GetScriptVariables();
        if (pScript && pScriptVariables) {
            uint32_t uiID;
            pScript->FindVariable(asVariableName, &uiID);
            if (uiID) {
                return pScriptVariables->GetVariable(uiID, nullptr);
            }
        }
    }

    return -1.0;
}

void Actor::ClearForcedMoveMode() {
    pActorMover->ClearForcedMoveMode();
}

// A killer gets its murder alarm (or the owner's attack alarm), the player its detection event and murder crime,
// then the actor is set dead, leaves combat and its packages, and stops being a teammate.
void Actor::DoDeathStuff() {
    Animation* pAnim = GetAnimation();
    if ((!pAnim || !pAnim->GetCurrentSequence(AGS_IDLE) || GetDisabled()) &&
        (!bEssentialTakeNoDamage.Bool() || !GetEssential())) {
        if (bSetOnDeath) {
            SetLifeState(ACTOR_LIFE_STATE_DEAD);
        }
        else {
            bSetOnDeath = true;
            if (bMurderAlarm && pMyKiller && pMyKiller->IsNpc()) {
                Actor* pActor = this;
                TESForm* pOwner = GetOwner();
                if (IsCreature() || pOwner) {
                    if (pOwner) {
                        if (pOwner->GetFormType() == NPC__ID) {
                            pActor = ReferenceProcessLists.GetActorRefInHigh(static_cast<TESNPC*>(pOwner), false);
                        }
                        else if (pOwner->GetFormType() == FACT_ID) {
                            pActor = ReferenceProcessLists
                                         .GetActorRefInHigh(static_cast<TESFaction*>(pOwner), false, false);
                        }
                    }
                    Script::SetActionFlag(pMyKiller, &m_Extra, SCRIPT_ACTION_ONMURDER);
                    if (pActor && pMyKiller) {
                        pActor->AttackAlarm(pMyKiller, false, true);
                    }

                    bMurderAlarm = false;
                }
                else {
                    MurderAlarm(pMyKiller);
                }
            }

            if (pMyKiller == pPlayer) {
                // The trailing arguments are sound level 0, event type 4 and no reference.
                BaseProcess* pKillerProc = pMyKiller->pCurrentProcess;
                NiPoint3 kLocation = GetLocationOnReference();
                pKillerProc->SetActorsDetectionEvent(pMyKiller, kLocation, 0, 4, nullptr);
            }
            if (pMyKiller == pPlayer && bMurderAlarm) {
                TESActorBase* pActorBase = GetActorBase();
                if (pActorBase) {
                    pActorBase->SetPCFactionFlags(PC_FACTIONFLAG_MURDER);
                }
            }
            Script::SetActionFlag(pMyKiller, &m_Extra, SCRIPT_ACTION_ONDEATH);
            if (pCurrentProcess) {
                pCurrentProcess->SetGreetingTimer(std::numeric_limits<float>::max());
                pCurrentProcess->SetSitSleepState(this, SIT_SLEEP_NORMAL, nullptr, 127);
            }
            SetLifeState(ACTOR_LIFE_STATE_DEAD);
            if (IsInCombat()) {
                StopCombat(nullptr);
            }
            if (pCurrentProcess) {
                EndInterruptPackage(false);
                pCurrentProcess->ClearRunOncePackage();
                pCurrentProcess->ClearCurrentPackage();
            }
            SetPlayerTeammate(false);
        }
    }
}

void Actor::HandlePostAnimationActions() {
    int32_t eFlags = 0;
    if (pCurrentProcess) {
        Animation* pAnim = GetAnimation();
        eFlags = pCurrentProcess->GetPostAnimationActions();
        pCurrentProcess->ClearPostAnimationActions();
        if (pAnim) {
            if ((eFlags & BaseProcess::POSTANIM_ACTION_DIE) != 0) {
                uint16_t sAnimGroup = GetAnimGroup(ANIM_GROUP_DEATH, nullptr, false, nullptr);
                pAnim->PlayGroup(sAnimGroup, ACTION_START, -1, AGS_NONE);
                pCurrentProcess->EndMoveMessage(this);
            }
            if (!IsDead(false)) {
                if ((eFlags & BaseProcess::POSTANIM_RELOAD_TARGETS) != 0) {
                    FlushReloadTargetQueque();
                }
                ItemChange* pWeapItem = pCurrentProcess->GetCurrentWeapon();
                TESObjectWEAP* pCurrentWeapon = nullptr;
                if (pWeapItem) {
                    pCurrentWeapon = static_cast<TESObjectWEAP*>(pWeapItem->pContainerObj);
                }
                if (eQueuedattack != ANIM_GROUP_NONE) {
                    StartAttack(eQueuedattack);
                    if (!pCurrentProcess->GetAutomaticFireAtLeastOnce() ||
                        pCurrentProcess->GetAutomaticShotDelay() == 0.0) {
                        pCurrentProcess->SetAutomaticFireAtLeastOnce(false);
                        eQueuedattack = ANIM_GROUP_NONE;
                    }
                }
                if ((eFlags & BaseProcess::POSTANIM_FREEUP_SPECIAL_IDLE) != 0) {
                    if (pCurrentProcess && pAnim && !pAnim->SpecialIdleWorking()) {
                        pCurrentProcess->PostAnimFreeUpSpecialIdle(this);
                    }
                    else {
                        pCurrentProcess->AddPostAnimationAction(BaseProcess::POSTANIM_FREEUP_SPECIAL_IDLE);
                    }
                }
                if ((eFlags & BaseProcess::POSTANIM_FORCEIDLE_ANIMATION) != 0) {
                    ForceIdleAnimation();
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_BLOCK) != 0) {
                    PostAnimBlock();
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_RELOAD_SINGLE) != 0) {
                    Animation* pAnim3rd = GetAnimation();
                    if (this == pPlayer) {
                        pAnim3rd = pPlayer->GetAnimation(false);
                    }
                    ItemChange* pWeaponItem = pCurrentProcess->GetCurrentWeapon();
                    ItemChange* pAmmo = pCurrentProcess->GetCurrentAmmo();
                    int32_t iAmmoCount = 0;
                    int32_t iMaxAmmo = 0;
                    uint16_t sMoveMode = GetMoveMode();
                    ANIM_GROUP_MOVE_ENUM eAnimMove = AGM_NONE;
                    ANIM_GROUP_WEAPON_ENUM eAnimWeapon = AGW_NONE;
                    if ((sMoveMode & ActorMover::MF_SWIMMING) != 0) {
                        eAnimMove = AGM_SNEAK;
                    }
                    else if ((sMoveMode & ActorMover::MF_FLYING) != 0) {
                        eAnimMove = AGM_SWIM;
                    }
                    else {
                        eAnimMove = static_cast<ANIM_GROUP_MOVE_ENUM>((sMoveMode & ActorMover::MF_SNEAKING) != 0);
                    }
                    ANIM_GROUP_ENUM eQueuedLoop = ANIM_GROUP_NONE;
                    if ((sMoveMode & ActorMover::MF_RUNNING) != 0) {
                        if ((sMoveMode & ActorMover::MF_FORWARD) != 0) {
                            eQueuedLoop = ANIM_GROUP_FAST_FORWARD;
                        }
                        else if ((sMoveMode & ActorMover::MF_BACKWARD) != 0) {
                            eQueuedLoop = ANIM_GROUP_FAST_BACK;
                        }
                        else if ((sMoveMode & ActorMover::MF_LEFT) != 0) {
                            eQueuedLoop = ANIM_GROUP_FAST_LEFT;
                        }
                        else if ((sMoveMode & ActorMover::MF_RIGHT) != 0) {
                            eQueuedLoop = ANIM_GROUP_FAST_RIGHT;
                        }
                    }
                    else if ((sMoveMode & ActorMover::MF_FORWARD) != 0) {
                        eQueuedLoop = ANIM_GROUP_MOVE_FORWARD;
                    }
                    else if ((sMoveMode & ActorMover::MF_BACKWARD) != 0) {
                        eQueuedLoop = ANIM_GROUP_MOVE_BACK;
                    }
                    else if ((sMoveMode & ActorMover::MF_LEFT) != 0) {
                        eQueuedLoop = ANIM_GROUP_MOVE_LEFT;
                    }
                    else if ((sMoveMode & ActorMover::MF_RIGHT) != 0) {
                        eQueuedLoop = ANIM_GROUP_MOVE_RIGHT;
                    }
                    if (pWeaponItem && pCurrentWeapon) {
                        bool bClipSizeMod = pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                        iMaxAmmo = pCurrentWeapon->GetFormClipRounds(bClipSizeMod);
                    }

                    InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(this);
                    int32_t iTotalCount = 0;
                    if (pInventory && pAmmo) {
                        iTotalCount = pInventory->GetObjectCount(pAmmo->pContainerObj);
                    }

                    if (pAmmo) {
                        iAmmoCount = pAmmo->iNumber + 1;
                        if (iAmmoCount <= iTotalCount) {
                            QueueEquipObject(pAmmo->pContainerObj, iAmmoCount, nullptr, true, false, false);
                        }
                    }
                    if (iAmmoCount < iMaxAmmo && iTotalCount != iAmmoCount && iTotalCount) {
                        TESObjectWEAP* pWeap = static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj);
                        ANIM_GROUP_ENUM eReloadAnim = pWeap->GetReloadAnim(false);
                        uint16_t sAnimGroup = GetAnimGroup(eReloadAnim, pWeaponItem, false, pAnim3rd);
                        SetAnimAction(ANIM_ACTION_RELOAD_LOOP, pAnim3rd->GetCurrentSequence(AGS_WEAPON));
                        pAnim3rd->PlayGroup(sAnimGroup, ACTION_START, 1, AGS_WEAPON);
                        if (this == pPlayer) {
                            pPlayer->StartAnimOn1stPerson(sAnimGroup, ACTION_START);
                        }
                    }
                    else {
                        bool bBackPackOrPowerArmor = HasBackPackWorn() || IsPowerBodyArmorWorn();
                        uint16_t sLoopGroup = TESAnimGroup::AnimGroup(
                            eAnimMove,
                            eAnimWeapon,
                            eQueuedLoop,
                            bBackPackOrPowerArmor
                        );
                        uint16_t sAnimGroup = pAnim3rd->PickBestAnimation(sLoopGroup, false);
                        if (sAnimGroup != 255 && pAnim3rd) {
                            pAnim3rd->PlayGroup(sAnimGroup, ACTION_START, 1, AGS_WEAPON);
                            if (this != pPlayer) {
                                ANIM_GROUP_ENUM eReloadAnim = pCurrentWeapon->GetReloadAnim(false);
                                uint16_t sReloadGroup = GetAnimGroup(eReloadAnim, nullptr, false, nullptr);
                                if (TESAnimGroup::GetType(sReloadGroup) == eReloadAnim && pAnim) {
                                    pAnim->PlayGroup(sReloadGroup, ACTION_START, -1, AGS_NONE);
                                    StartAnimOn1stPerson(sReloadGroup, ACTION_START);
                                    SetAnimAction(ANIM_ACTION_RELOAD, pAnim->GetCurrentSequence(AGS_WEAPON));
                                }
                            }
                            if (this == pPlayer) {
                                pPlayer->StartAnimOn1stPerson(sAnimGroup, ACTION_NORMAL);
                                pPlayer->SetAnimAction(ANIM_ACTION_NONE, nullptr);
                            }
                        }
                    }
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_FIRE_WEAPON) != 0) {
                    TESObjectWEAP* pWeapon = nullptr;
                    if (pCurrentProcess->GetCurrentWeapon()) {
                        pWeapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
                    }
                    if (pWeapon) {
                        if (pWeapon->IsRangedWeapon()) {
                            pWeapon->Fire(this);
                            ItemChange* pWeaponItem = pCurrentProcess->GetCurrentWeapon();
                            bool bUseModValue = false;
                            bool bSilencer = false;
                            if (pWeaponItem) {
                                bUseModValue = pWeaponItem->HasModEffectActive(WEAPON_MOD_SILENCE);
                                bSilencer = pWeaponItem->HasModEffectActive(16);
                            }
                            uint32_t uiSoundLevel = pWeapon->GetModifiedSoundLevel(bUseModValue, bSilencer);
                            SetActionSoundValue(uiSoundLevel);
                        }
                        else {
                            TESPackage* pPack;
                            if (this == pPlayer ||
                                IsInCombat() ||
                                ((pPack = pCurrentProcess->GetPackageThatIsRunning()) != nullptr &&
                                 (pPack->GetPackType() == PACKAGE_TYPE_USE_ITEM_AT ||
                                  pPack->GetPackType() == PACKAGE_TYPE_USE_WEAPON))) {
                                MeleeAttack(false, true);
                            }
                            else {
                                MeleeAttack(false, false);
                            }
                        }
                        if (pWeapon->pFormScript &&
                            pCurrentProcess->GetCurrentWeapon() &&
                            pCurrentProcess->GetCurrentWeapon()->pExtraObjectList) {
                            ExtraDataList* pExtraList = pCurrentProcess->GetCurrentWeapon()->pExtraObjectList->m_item;
                            Script::SetActionFlag(
                                pCurrentProcess->GetCurrentWeapon()->pContainerObj,
                                pExtraList,
                                SCRIPT_ACTION_ONFIRE
                            );
                        }
                    }
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_RELOAD_WEAPON) != 0 && pCurrentWeapon && pWeapItem) {
                    if ((pAnim->GetCurrentSequence(AGS_IDLE) &&
                         pAnim->GetCurrentSequence(AGS_IDLE)->GetState() != NiControllerSequence::kAnimating) ||
                        (pAnim->GetCurrentSequence(AGS_MOVEMENT) &&
                         pAnim->GetCurrentSequence(AGS_MOVEMENT)->GetState() != NiControllerSequence::kAnimating) ||
                        (pAnim->GetCurrentSequence(AGS_WEAPON) &&
                         pAnim->GetCurrentSequence(AGS_WEAPON)->GetState() != NiControllerSequence::kAnimating)) {
                        pCurrentProcess->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_RELOAD_WEAPON);
                    }
                    else {
                        bool bClipSizeMod = pWeapItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                        ReloadWeaponNV(pCurrentWeapon, 2, bClipSizeMod, false);
                    }
                }

                if ((eFlags & BaseProcess::POSTANIM_ACTION_SIT_SLEEP) != 0) {
                    if (pCurrentProcess->GetCurrentFurnitureIndex() <= 200) {
                        if (!pCurrentProcess->SitOrSleep(this)) {
                            pCurrentProcess->SetAnimActionSuccess(0);
                        }
                    }
                    else {
                        GetOutofFurnitureQuick();
                    }
                    eFlags &= ~BaseProcess::POSTANIM_ACTION_PLAY_IDLE;
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_GET_UP) != 0) {
                    if (!pCurrentProcess->ProcessGetUp(this)) {
                        pCurrentProcess->SetAnimActionSuccess(0);
                    }
                    eFlags &= ~BaseProcess::POSTANIM_ACTION_PLAY_IDLE;
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_GETOUT_OF_FURNITURE) != 0) {
                    GetOutofFurnitureQuick();
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_PICK_IDLE_CALL) != 0) {
                    TESIdleForm* pIdle = pCurrentProcess->GetCurrentProcessIdle();
                    if (pIdle) {
                        ANIM_GROUP_SECTION eIdleSection = pIdle->GetIdleAnimGroupSection();
                        pAnim->SpecialIdleAuto(pIdle, this, eIdleSection, AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE);
                    }
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_PLAY_IDLE) != 0) {
                    pCurrentProcess->FinishSetupSpecialIdle(this);
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_IDLE_LOADED) != 0) {
                    while (pCurrentProcess->GetCurrentProcessAnimIdle()) {
                        NiPointer<AnimIdle> spAnimIdle(pCurrentProcess->GetCurrentProcessAnimIdle());
                        KFModel* pAnimKF = pCurrentProcess->GetCurrentProcessAnimIdleKF();
                        if (spAnimIdle.m_pObject && pAnimKF) {
                            spAnimIdle->Loaded(pAnimKF);
                        }

                        pCurrentProcess->SetCurrentProcessAnimIdleLoaded(nullptr, nullptr);
                    }
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_EQUIP_UNEQUIP_WEAPON) != 0) {
                    ItemChange* pIdleWeapItem;
                    bool bDrawn = pCurrentProcess->GetWeaponDrawn();
                    bool bCheckHandToHand = bDrawn && pWeapItem == nullptr;
                    if (bDrawn) {
                        pIdleWeapItem = pWeapItem;
                    }
                    else {
                        // GetAnimGroup tests its weapon item against the sentinel -1: an integer, not an object.
                        pIdleWeapItem = reinterpret_cast<ItemChange*>(static_cast<uintptr_t>(-1));
                    }
                    uint16_t sAnimGroup = GetAnimGroup(ANIM_GROUP_IDLE, pIdleWeapItem, bCheckHandToHand, pAnim);
                    pAnim->PlayGroup(sAnimGroup, ACTION_START, -1, AGS_NONE);
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_HAND_GRIP_WEAPON) != 0 && GetCurrentWeapon()) {
                    TESObjectWEAP* pWeapon = GetCurrentWeapon();
                    // GetHandGripAnim's byte is zero-extended and passed whole as the anim group.
                    ANIM_GROUP_ENUM eHandGrip = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetHandGripAnim());
                    ItemChange* pWeaponItem = pCurrentProcess->GetCurrentWeapon();
                    uint16_t sAnimGroup = GetAnimGroup(eHandGrip, pWeaponItem, false, pAnim);
                    pAnim->PlayGroup(sAnimGroup, ACTION_START, -1, AGS_NONE);
                }

                if ((eFlags & BaseProcess::POSTANIM_ACTION_HAVOKWEAPON) != 0) {
                    SetHavokWeapon();
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_EXPEL_SHELLCASING) != 0) {
                    TESObjectWEAP* pWeapon = nullptr;
                    if (pCurrentProcess->GetCurrentWeapon()) {
                        pWeapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
                    }
                    if (pWeapon) {
                        pWeapon->ExpelShellCasing(this);
                    }
                }
                if ((eFlags & BaseProcess::POSTANIM_ACTION_SET_IRON_SIGHTS) != 0) {
                    bool bIronSights = pCurrentProcess->GetIronSights();
                    SetIronSights(bIronSights, true, false);
                }
            }
        }
    }
}

bool Actor::IsContinuingPackagePCNear() {
    return pCurrentProcess && pCurrentProcess->ContinuingPackageforPC();
}

void Actor::HandleCombatProcessLevelChange(PROCESS_TYPE aeOldLevel, PROCESS_TYPE aeNewLevel) {
    CombatController* pCombatController = GetCombatController();
    if (pCombatController) {
        pCombatController->ChangeProcessLevel(aeOldLevel, aeNewLevel);
    }
}

bool Actor::IsFollowing() {
    if (!pCurrentProcess) {
        return false;
    }
    TESPackage* pPackage = pCurrentProcess->GetPackageThatIsRunning();
    if (!pPackage) {
        return false;
    }
    if (pPackage->GetPackType() == PACKAGE_TYPE_FOLLOW) {
        return !pPackage->GetIsCreated();
    }
    return false;
}

// Whether the actor runs a follow or accompany package: its own current package (only while it targets apLeader),
// its process's current or run-once package, or its extra package.
bool Actor::IsFollowing(Actor* apLeader) {
    if (!pCurrentProcess) {
        return false;
    }
    TESPackage* pPackage = MobileObject::GetCurrentPackage();
    if (pPackage &&
        (pPackage->GetPackType() == PACKAGE_TYPE_FOLLOW || pPackage->GetPackType() == PACKAGE_TYPE_ACCOMPANY) &&
        pCurrentProcess->GetTarget() == apLeader) {
        return true;
    }
    pPackage = pCurrentProcess->GetCurrentPackage();
    if (pPackage &&
        (pPackage->GetPackType() == PACKAGE_TYPE_FOLLOW || pPackage->GetPackType() == PACKAGE_TYPE_ACCOMPANY)) {
        return true;
    }
    pPackage = pCurrentProcess->GetRunOncePackage();
    if (pPackage &&
        (pPackage->GetPackType() == PACKAGE_TYPE_FOLLOW || pPackage->GetPackType() == PACKAGE_TYPE_ACCOMPANY)) {
        return true;
    }
    pPackage = GetExtra()->GetPackageExtra();
    return pPackage &&
        (pPackage->GetPackType() == PACKAGE_TYPE_FOLLOW || pPackage->GetPackType() == PACKAGE_TYPE_ACCOMPANY);
}

bool Actor::IsEscorterBehind() {
    if (pCurrentProcess) {
        TESPackage* pPackage = pCurrentProcess->GetPackageThatIsRunning();
        PlayerCharacter* pTarget = static_cast<PlayerCharacter*>(pCurrentProcess->GetTarget());
        if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_ESCORT && pTarget == pPlayer) {
            if (pTarget) {
                NiPoint3 kPos = pPackage->GetLocationCoord(this);
                NiPoint3 kActorOffset = GetLocationOnReference() - kPos;
                kPos = pPackage->GetLocationCoord(this);
                NiPoint3 kTargetOffset = pTarget->GetLocationOnReference() - kPos;
                if (pPlayer->TESObjectREFR::GetParentCell() &&
                    pPlayer->TESObjectREFR::GetParentCell()->GetInterior() &&
                    pPackage->GetLocationCell(this) &&
                    pPackage->GetLocationCell(this)->GetInterior()) {
                    TESObjectCELL* pLocationCell = pPackage->GetLocationCell(this);
                    if (pLocationCell != pPlayer->TESObjectREFR::GetParentCell()) {
                        return false;
                    }
                }

                double dActorDist = kActorOffset.Length();
                if (kTargetOffset.Length() < dActorDist) {
                    return true;
                }
            }
        }
    }
    return false;
}

float Actor::GetWalkSpeed() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        CachedValuesOwner* pOwner = nullptr;
        if (this) {
            pOwner = static_cast<CachedValuesOwner*>(this);
        }
        return pCurrentProcess->GetWalkSpeed(pOwner);
    }

    return CalculateWalkSpeed();
}

float Actor::GetRunSpeed() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        CachedValuesOwner* pOwner = nullptr;
        if (this) {
            pOwner = static_cast<CachedValuesOwner*>(this);
        }
        return pCurrentProcess->GetCachedRunSpeed(pOwner);
    }

    return CalculateRunSpeed();
}

float Actor::GetSwimWalkSpeed() {
    bool bNoWeapon = true;
    ItemChange* pWeaponItem = nullptr;
    if (pCurrentProcess) {
        bNoWeapon = !pCurrentProcess->GetWeaponDrawn();
        pWeaponItem = pCurrentProcess->GetCurrentWeapon();
    }
    ItemChange* pArmorItem = GetArmorBeingWorn(BO_UPPER_BODY);
    TESObjectWEAP* pWeap = nullptr;
    if (pWeaponItem) {
        pWeap = static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj);
    }
    TESObjectARMO* pArmor = nullptr;
    if (pArmorItem) {
        pArmor = static_cast<TESObjectARMO*>(pArmorItem->pContainerObj);
    }
    if (pArmorItem) {
        delete pArmorItem;
    }
    ActorValueOwner* pAVOwner = nullptr;
    if (this) {
        pAVOwner = static_cast<ActorValueOwner*>(this);
    }
    bool bOverEncumbered = IsOverEncumbered();
    bool bNPC = IsNpc();
    return GamePlayFormulas::GetActorSwimWalkSpeed(pAVOwner, pWeap, pArmor, bNoWeapon, bNPC, bOverEncumbered);
}

float Actor::GetSwimRunSpeed() {
    bool bNoWeapon = true;
    ItemChange* pWeapon = nullptr;
    if (pCurrentProcess) {
        bNoWeapon = !pCurrentProcess->GetWeaponDrawn();
        pWeapon = pCurrentProcess->GetCurrentWeapon();
    }
    ItemChange* pArmorItem = GetArmorBeingWorn(BO_UPPER_BODY);
    TESObjectWEAP* pWeap = nullptr;
    if (pWeapon) {
        pWeap = static_cast<TESObjectWEAP*>(pWeapon->pContainerObj);
    }
    TESObjectARMO* pArmor = nullptr;
    if (pArmorItem) {
        pArmor = static_cast<TESObjectARMO*>(pArmorItem->pContainerObj);
    }
    if (pArmorItem) {
        delete pArmorItem;
    }
    ActorValueOwner* pAVOwner = nullptr;
    if (this) {
        pAVOwner = static_cast<ActorValueOwner*>(this);
    }
    bool bOverEncumbered = IsOverEncumbered();
    bool bNPC = IsNpc();
    return GamePlayFormulas::GetActorSwimRunSpeed(pAVOwner, pWeap, pArmor, bNoWeapon, bNPC, bOverEncumbered);
}

void Actor::InitiateSurfacePackage() {
    ForceStopMoving();
    TESPackage* pPackage = TESPackage::CreatePackage(PACKAGE_TYPE_SURFACE);
    pPackage->SetPackType(PACKAGE_TYPE_SURFACE);
    pPackage->SetMustReachLocation(true);
    pPackage->SetMustComplete(true);
    PackageLocation* pLocation = new PackageLocation;
    pLocation->SetLocReference(this);
    pPackage->SetPackageLocation(pLocation);
    if (pLocation) {
        delete pLocation;
    }
    pPackage->CalculateProcedureType(nullptr);
    PutCreatedPackage(pPackage, true, true);
}

void Actor::UpdateCombatData() {
    if (IsInCombat()) {
        CombatController* pCombatController = GetCombatController();
        if (pCombatController) {
            pCurrentCombatTarget = pCombatController->GetTarget();
            bSearchingInCombat = pCombatController->IsSearching();
            if (pCombatController->pCombatGroup) {
                if (!pCurrentCombatTargetArray) {
                    pCurrentCombatTargetArray = new BSSimpleArray<Actor*, 1024>;
                }
                pCombatController->pCombatGroup->FillTargetArray(pCurrentCombatTargetArray);
                if (!pCurrentCombatMemberArray) {
                    pCurrentCombatMemberArray = new BSSimpleArray<Actor*, 1024>;
                }
                pCombatController->pCombatGroup->FillMemberArray(pCurrentCombatMemberArray);
            }
        }
    }
    else {
        bSearchingInCombat = 0;
        pCurrentCombatTarget = nullptr;
        if (pCurrentCombatTargetArray) {
            delete pCurrentCombatTargetArray;
        }
        pCurrentCombatTargetArray = nullptr;
        if (pCurrentCombatMemberArray) {
            delete pCurrentCombatMemberArray;
        }
        pCurrentCombatMemberArray = nullptr;
    }
}

float Actor::GetRadiationResistanceMult() {
    if (fRadiationResistanceMult < 0.0) {
        float fMax = IsPc() ? fPlayerMaxResistance.Float() : 100.0f;
        float fResistance = GetActorFloatValue(ActorValue::RADIATION_RESISTANCE);
        float fResult = BSMIN(fMax, fResistance);
        if (fResult < 0.0) {
            fResult = 0.0f;
        }
        fRadiationResistanceMult = 1.0 - fResult * 0.009999999776482582;
    }
    return fRadiationResistanceMult;
}

// 0 when over-encumbered, else the fly speed scaled by the weight of the flight inventory.
float Actor::GetFlySpeed() {
    if (IsOverEncumbered()) {
        return 0.0f;
    }
    float fFlyInventoryWeight = RecalculateFlyInventoryWeight();
    return ActorValueOwner::CalculateFlySpeed(static_cast<ActorValueOwner*>(this), fFlyInventoryWeight);
}

float Actor::RecalculateFlyInventoryWeight() {
    float fCachedWeight = GetEquippedWeight();
    if (fCachedWeight == -1.0) {
        InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(this);
        float fEquippedWeight = pInventory->GetEquippedWeight(this);
        SetEquippedWeight(fEquippedWeight);
    }
    return GetEquippedWeight();
}

// The free distance from the actor's eye point (the smart camera check height above its location) towards its heading
// turned by afAngle: the hit fraction of the first hit on another reference's rigid body along a 10000 unit pick, else
// FLT_MAX. apTarget is unused.
float Actor::GetVATSAreaFree(TESObjectREFR* apTarget, float afAngle) {
    NiPoint3 kFrom = GetLocationOnReference();
    kFrom.z = kFrom.z + fVATSSmartCameraCheckHeight.Float();
    alignas(16) NiPoint3 kDir;
    NiMatrix3 kRotation;
    float fHeading = GetHeading(false) + afAngle;
    kRotation.MakeZRotation(fHeading);
    kRotation.GetCol(1, kDir);
    kDir.Unitize();
    float fCheckDistance = 10000.0f;
    NiPoint3 kTo = kFrom + fCheckDistance * kDir;
    bool bHit = false;
    float fDistance = std::numeric_limits<float>::max();
    bhkPickData kPickData;
    bhkPickData* pPickData = &kPickData;
    pPickData->NiSetFrom(kFrom);
    pPickData->NiSetTo(kTo);
    CFilter kFilter(0);
    kFilter.SetLayer(L_CAMERA);
    CFilter kActorFilter;
    GetCollisionFilter(&kActorFilter.iFilter);
    kFilter.SetGroup(kActorFilter.GetGroup());
    pPickData->SetFilter(kFilter);
    hkpAllRayHitCollector kCollector;
    hkpAllRayHitCollector* pCollector = &kCollector;
    pPickData->SetAllCollector(pCollector);
    bhkWorld* pWorld = pPlayer->GetParentCell()->GetbhkWorld();
    pWorld->PickObject(*pPickData);
    for (int32_t i = 0; !bHit && i < pPickData->GetAllCollector()->getHits().getSize(); ++i) {
        hkpWorldRayCastOutput kHit(pPickData->GetAllCollector()->getHits()[i]);
        hkpWorldRayCastOutput* pHit = &kHit;
        NiAVObject* pObject = GetAVObjectForCollidable(pHit->m_rootCollidable);
        TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pObject);
        hkpRigidBody* pBody = hkpGetRigidBody(pHit->m_rootCollidable);
        if (pObject && pBody && pRef != this) {
            bHit = true;
            fDistance = pHit->m_hitFraction * fCheckDistance;
        }
    }
    if (bVATSSmartCameraCheckDebug.Bool()) {
        NiColorA kColor(1.0f, 1.0f, 0.0f, 1.0f);
        NiLines* pLine = MakeLine(kFrom, kColor, kFrom + fDistance * kDir, kColor, true);
        TES::pInstance->AddTempDebugObject(pLine, 10.0f);
    }
    return fDistance;
}

// How far along the actor's heading turned by afAngle the VATS camera can step from the eye point and still see
// apTarget's world bound centre: per step (the step distance times 1, 2, ... up to the step count) a pick towards the
// centre; a hit on another reference's rigid body blocks it unless past 0.85 of the ray, or for an actor 256 units
// away. 0 without apTarget's 3D.
float Actor::GetVATSTargetVisible(TESObjectREFR* apTarget, float afAngle) {
    if (!apTarget || !apTarget->Get3D()) {
        return 0.0f;
    }
    NiPoint3 kEye = GetLocationOnReference();
    kEye.z = kEye.z + fVATSSmartCameraCheckHeight.Float();
    alignas(16) NiPoint3 kDir;
    NiMatrix3 kRotation;
    float fHeading = GetHeading(false) + afAngle;
    kRotation.MakeZRotation(fHeading);
    kRotation.GetCol(1, kDir);
    kDir.Unitize();
    NiPoint3 kTarget = apTarget->Get3D()->GetWorldBound().GetCenter();
    bool bBlocked = false;
    float fDistance = 0.0f;
    for (int32_t i = 0; !bBlocked; ++i) {
        double dStep = i;
        if (!(fVATSSmartCameraCheckStepCount.Float() > dStep)) {
            break;
        }
        double dNextStep = i + 1;
        float fStepDistance = fVATSSmartCameraCheckStepDistance.Float() * dNextStep;
        NiPoint3 kFrom = kEye + fStepDistance * kDir;
        bhkPickData kPickData;
        bhkPickData* pPickData = &kPickData;
        pPickData->NiSetFrom(kFrom);
        pPickData->NiSetTo(kTarget);
        CFilter kFilter(0);
        kFilter.SetLayer(L_CAMERA);
        CFilter kActorFilter;
        GetCollisionFilter(&kActorFilter.iFilter);
        kFilter.SetGroup(kActorFilter.GetGroup());
        pPickData->SetFilter(kFilter);
        hkpAllRayHitCollector kCollector;
        hkpAllRayHitCollector* pCollector = &kCollector;
        pPickData->SetAllCollector(pCollector);
        if (bVATSSmartCameraCheckDebug.Bool()) {
            NiColorA kColor(1.0f, 1.0f, 0.0f, 1.0f);
            NiLines* pLine = MakeLine(kTarget, kColor, kFrom, kColor, true);
            TES::pInstance->AddTempDebugObject(pLine, 10.0f);
        }
        bhkWorld* pWorld = pPlayer->GetParentCell()->GetbhkWorld();
        pWorld->PickObject(*pPickData);
        for (int32_t j = 0; !bBlocked && j < pPickData->GetAllCollector()->getHits().getSize(); ++j) {
            hkpWorldRayCastOutput kHit(pPickData->GetAllCollector()->getHits()[j]);
            hkpWorldRayCastOutput* pHit = &kHit;
            NiAVObject* pObject = GetAVObjectForCollidable(pHit->m_rootCollidable);
            TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pObject);
            hkpRigidBody* pBody = hkpGetRigidBody(pHit->m_rootCollidable);
            if (pObject && pBody && pRef != apTarget) {
                bBlocked = true;
                if (pHit->m_hitFraction > 0.8500000238418579) {
                    bBlocked = false;
                }
                if (pRef && pRef->IsActor()) {
                    float fHitDistance = (kTarget - kFrom).Length() * pHit->m_hitFraction;
                    if (fHitDistance > 256.0) {
                        bBlocked = false;
                    }
                }
            }
        }
        if (!bBlocked) {
            double dReached = i + 1;
            fDistance = fVATSSmartCameraCheckStepDistance.Float() * dReached;
        }
    }
    return fDistance;
}

bool Actor::GetWantWeaponDrawn() {
    if ((GetMoveMode() & 0x800) == 0) {
        return pCurrentProcess->GetWantWeaponDrawn();
    }
    if (this == pPlayer && pVATS.GetMode() == VATS::VATS_PLAYBACK) {
        pVATS.QuitVATSPlayback(false, false);
    }
    return false;
}

// False while dead, immobile, unable to be knocked down, or reloading.
bool Actor::CanKnockDown() {
    if (IsDead(true)) {
        return false;
    }
    if (IsImmobile()) {
        return false;
    }
    if (!GetActorBase()->GetKnockedDowns()) {
        return false;
    }
    if (pCurrentProcess &&
        (pCurrentProcess->GetAnimAction() == ANIM_ACTION_RELOAD ||
         pCurrentProcess->GetAnimAction() == ANIM_ACTION_RELOAD_LOOP)) {
        return false;
    }
    if (GetAnimation() && GetAnimation()->ReloadAnimQueued()) {
        return false;
    }
    return true;
}

// Puts the actor straight into the furniture's marker aiIndex: heading and position from the marker, then the sit
// or sleep idle. The player is forced into third person for it and goes back to first person if it was there.
// Returns whether the dynamic idle loaded.
bool Actor::PutActorInChairBedQuick(
    TESObjectREFR* apFurniture,
    FurnitureMark* apmarker,
    int32_t aiIndex,
    bool abSleeping
) {
    bool bWas3rdPerson = pPlayer->Is3rdPerson();
    if (this == pPlayer) {
        pPlayer->Force3rdPerson(true);
    }
    if (!GetAnimation() || !GetCharController()) {
        return false;
    }
    TESFurniture* pFurniture = static_cast<TESFurniture*>(apFurniture->GetObjectReference());
    bool bResult = false;
    if (!pFurniture->CanSitOn() && !pFurniture->CanSleepOn()) {
        return false;
    }
    SetHeading(apmarker->GetHeading());
    float fScale = GetScale();
    uint8_t ucNumber = apmarker->cNumber;
    NiPoint3 kDelta = pFurniture->GetMarkerTargetOffset(ucNumber, fScale);
    NiMatrix3 kRotation;
    kRotation.MakeZRotation(apmarker->GetHeading());
    kDelta = kRotation * kDelta;
    pCurrentProcess->SetProcedureIndexRunning(2);
    apFurniture->SetMarkerUsed(aiIndex, true);
    SetPosition(apmarker->QPosition() + kDelta);
    const NiPoint3& rkMarkerPos = apmarker->QPosition();
    static_cast<bhkCharacterController*>(GetCharController())->SetPosition(rkMarkerPos);
    GetAnimation()->ClearGroup(AGS_WHOLE_BODY, 0.0f);
    if (this == pPlayer) {
        pPlayer->GetAnimation(true)->ClearGroup(AGS_WHOLE_BODY, 0.0f);
    }
    ucNumber = apmarker->cNumber;
    ModHeading(static_cast<TESFurniture*>(apFurniture->GetObjectReference())->GetMarkerHeadingOffset(ucNumber));
    if (abSleeping) {
        pCurrentProcess->SetSitSleepState(this, SIT_SLEEP_LOAD_SLEEP_IDLE, apFurniture, aiIndex);
        pCurrentProcess->SetProcedureIndexRunning(1);
    }
    else {
        pCurrentProcess->SetSitSleepState(this, SIT_SLEEP_LOAD_SIT_IDLE, apFurniture, aiIndex);
    }
    if (pCurrentProcess->LoadDynamicIdle(this)) {
        bResult = true;
        if (abSleeping) {
            BSFaceGenAnimationData* pFaceData = GetFaceAnimationData();
            if (pFaceData) {
                pFaceData->SetDead(true, false);
            }
            pCurrentProcess->SetSitSleepState(this, SIT_SLEEP_IS_SLEEPING, apFurniture, aiIndex);
        }
        else {
            pCurrentProcess->SetSitSleepState(this, SIT_SLEEP_IS_SITTING, apFurniture, aiIndex);
        }
    }
    else {
        const char* pFurnitureName = apFurniture->GetFullName();
        const char* pName = TESObjectREFR::GetFullName();
        Warning("%s went to sit at %s and had no animation\n", pName, pFurnitureName);
        pCurrentProcess->SetSitSleepState(this, SIT_SLEEP_NORMAL, nullptr, 0x7F);
    }
    if (this == pPlayer && !bWas3rdPerson) {
        pPlayer->Force3rdPerson(false);
    }
    return bResult;
}

// Locks or unlocks the doors within 5000 units that the current package asks for: at its start, and once the actor is
// at the package location (locking marks the location locked).
void Actor::UnlockLockDoorsProcedure() {
    if (!pCurrentProcess) {
        return;
    }
    if (IsDead(false) || pSaveLoadGame->GetSaveGameLoading()) {
        return;
    }
    TESPackage* pPackage = pCurrentProcess->GetCurrentPackage();
    if (!pPackage || !GetParentCell() || pPackage->IsInterruptPackage()) {
        return;
    }
    if (pPackage->GetLockDoorsAtStart()) {
        const NiPoint3* pPoint2 = &GetLocationOnReference();
        const NiPoint3* pPoint1 = &GetLocationOnReference();
        TESObjectCELL* pCell = GetParentCell();
        pDataHandler->EnumReferencesCloseToPoint(
            pCell,
            *pPoint1,
            5000.0f,
            *pPoint2,
            5000.0f,
            &LowProcess::LockDoorsatLocation,
            this
        );
    }
    else if (pPackage->GetUnlockDoorsAtStart()) {
        const NiPoint3* pPoint2 = &GetLocationOnReference();
        const NiPoint3* pPoint1 = &GetLocationOnReference();
        TESObjectCELL* pCell = GetParentCell();
        pDataHandler->EnumReferencesCloseToPoint(
            pCell,
            *pPoint1,
            5000.0f,
            *pPoint2,
            5000.0f,
            &LowProcess::UnLockDoorsatLocation,
            this
        );
    }
    if (!pPackage->IsActorAtLocation(this, false, -1.0f, false)) {
        return;
    }
    BaseProcess* pProcess = GetCurrentAIProcess();
    if (pProcess->GetLockedLocation()) {
        return;
    }
    if (pPackage->GetLockDoorsAtLocation()) {
        const NiPoint3* pPoint2 = &GetLocationOnReference();
        const NiPoint3* pPoint1 = &GetLocationOnReference();
        TESObjectCELL* pCell = GetParentCell();
        pDataHandler->EnumReferencesCloseToPoint(
            pCell,
            *pPoint1,
            5000.0f,
            *pPoint2,
            5000.0f,
            &LowProcess::LockDoorsatLocation,
            this
        );
        GetCurrentAIProcess()->SetLockedLocation(true);
    }
    else if (pPackage->GetUnlockDoorsAtLocation()) {
        const NiPoint3* pPoint2 = &GetLocationOnReference();
        const NiPoint3* pPoint1 = &GetLocationOnReference();
        TESObjectCELL* pCell = GetParentCell();
        pDataHandler->EnumReferencesCloseToPoint(
            pCell,
            *pPoint1,
            5000.0f,
            *pPoint2,
            5000.0f,
            &LowProcess::UnLockDoorsatLocation,
            this
        );
    }
}

void Actor::UpdateRagdollAnimation(bool abDriveRagdoll) {
    float fHeading;
    float fPitch;
    pRagdollController->DoRagdollAnim(true);
    BSFaceGenAnimationData* pFaceData = GetFaceAnimationData();
    if (pFaceData) {
        if (pRagdollController->GetLookIKActive()) {
            pRagdollController->GetEyeAngles(fHeading, fPitch);
            pFaceData->SetDesiredEyeHeadingPitch(fHeading, fPitch);
        }
        else {
            pFaceData->SetDesiredEyeHeadingPitch(0.0, 0.0);
        }
    }
    if (!pRagdollController->QFootIKActive()) {
        pRagdollController->ClearVertError();
    }
}

void Actor::GetOutofFurnitureQuick() {
    if (pCurrentProcess->GetSitSleepState()) {
        uint8_t bWas3rdPerson = pPlayer->Is3rdPerson();
        if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
            pPlayer->Force3rdPerson(true);
        }
        Animation* pAnimation = GetAnimation();
        if (pAnimation) {
            if (pSaveLoadGame->GetSaveGameLoading() || pPlayer->IsSleepingorResting()) {
                pAnimation->SkipNextBlend();
                pAnimation->ClearGroup(AGS_WHOLE_BODY, 0.0);
            }
            else {
                pAnimation->BlendOut(AGS_WHOLE_BODY, false);
            }
            ItemChange* pWeaponItem = pCurrentProcess->GetCurrentWeapon();
            uint16_t sAnimGroup = GetAnimGroup(ANIM_GROUP_IDLE, pWeaponItem, true, pAnimation);
            if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
                pPlayer->GetAnimation(true)->ClearGroup(AGS_WHOLE_BODY, 0.0);
            }
            else if (!TESAnimGroup::GetType(sAnimGroup)) {
                pAnimation->PlayGroup(sAnimGroup, ACTION_START, -1, AGS_NONE);
            }
            pCurrentProcess->SetCurrentProcessIdle(nullptr);
        }
        pCurrentProcess->ClearFurniture(this);
        MobileObject::SetChaseBip(false);
        if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer) && !bWas3rdPerson) {
            pPlayer->Force3rdPerson(false);
        }
    }
}

void Actor::EvaluatePackage(bool abImmediate, bool abResetAI) {
    if (!IsDead(true)) {
        if (!abImmediate) {
            bEVPBuffered = true;
            bResetAI = abResetAI;
            if (pCurrentProcess) {
                float fLastTime = appCalendar.GetHour() - 1.0;
                pCurrentProcess->SetLastTimeProcessed(fLastTime);
            }
            return;
        }
        if (IsInCombat()) {
            StopCombat(nullptr);
        }
        if (abResetAI) {
            if (pCurrentProcess) {
                TESPackage* pNewPack;
                TESPackage* pPack;
                TESPackage* pCurrentPack;
                pCurrentProcess->GetRunOncePackage();
                if (pCurrentProcess->GetCurrentPackage()) {
                    pCurrentPack = pCurrentProcess->GetCurrentPackage();
                    if (pCurrentPack->GetUnlockDoorsAtEnd()) {
                        const NiPoint3* pPoint2 = &GetLocationOnReference();
                        const NiPoint3* pPoint1 = &GetLocationOnReference();
                        TESObjectCELL* pCell = GetParentCell();
                        pDataHandler->EnumReferencesCloseToPoint(
                            pCell,
                            *pPoint1,
                            5000.0f,
                            *pPoint2,
                            5000.0f,
                            &LowProcess::UnLockDoorsatLocation,
                            this
                        );
                    }
                    if (pCurrentPack->GetLockDoorsAtEnd()) {
                        const NiPoint3* pPoint2 = &GetLocationOnReference();
                        const NiPoint3* pPoint1 = &GetLocationOnReference();
                        TESObjectCELL* pCell = GetParentCell();
                        pDataHandler->EnumReferencesCloseToPoint(
                            pCell,
                            *pPoint1,
                            5000.0f,
                            *pPoint2,
                            5000.0f,
                            &LowProcess::LockDoorsatLocation,
                            this
                        );
                    }
                }
                if (pCurrentProcess->IsAFollower()) {
                    Actor* pAct = nullptr;
                    if (pCurrentProcess->GetTarget()) {
                        Actor* pTarget = static_cast<Actor*>(pCurrentProcess->GetTarget());
                        if (pTarget->IsActor()) {
                            pAct = static_cast<Actor*>(pCurrentProcess->GetTarget());
                        }
                    }
                    if (pAct) {
                        pAct->m_Extra.RemoveFollower(this);
                    }
                }
                pCurrentPack = GetCurrentEditorPackage();
                EndInterruptPackage(false);
                pPack = pCurrentProcess->GetCurrentPackage();
                if (pCurrentProcess->GetCurrentPackage()) {
                    pCurrentProcess->ClearCurrentPackage();
                }
                GetOutofFurnitureQuick();
                pCurrentProcess->ClearCurrentDataforProcess(this);
                pCurrentProcess->SetTargetActivated(false);
                pCurrentProcess->CheckforNewPackage(this, true);
                pNewPack = pCurrentProcess->GetCurrentPackage();
                if (pCurrentPack && pNewPack != pCurrentPack) {
                    Script::SetActionFlag(pCurrentPack, &m_Extra, SCRIPT_ACTION_ONPACKAGECHANGE);
                    pCurrentProcess->ChangePackage(this, pCurrentPack);
                    pCurrentProcess->SetPackageDoneOnce(false);
                }

                if (pPack && (pPack->GetPackType() == PACKAGE_TYPE_SLEEP || pPack->GetForceUnequipWeapon())) {
                    GetObjectReference()->GetFormType();
                }
                EndMovement();
                switch (GetSitSleepState()) {
                    case SIT_SLEEP_NORMAL: {
                        break;
                    }
                    default: {
                        SetChaseBip(false);
                        pCurrentProcess->SetSitSleepState(this, SIT_SLEEP_NORMAL, nullptr, 127);
                        SetAnimAction(ANIM_ACTION_NONE, nullptr);
                        Animation* pAnim = Actor::GetAnimation();
                        if (pAnim) {
                            pAnim->ClearGroup(AGS_WHOLE_BODY, 0.0f);
                            pAnim->SpecialIdleFree(true, false);
                        }
                        break;
                    }
                    case SIT_SLEEP_IS_SITTING: {
                        if (!pCurrentProcess->GetCurrentPackage()) {
                            break;
                        }
                        [[fallthrough]];
                    }
                    case SIT_SLEEP_IS_SLEEPING: {
                        InitiateGetUpPackage();
                        break;
                    }
                }
            }
            if (pCurrentProcess && pCurrentProcess->eLevel) {
                float fLastTime = appCalendar.GetHour() - 1.0;
                pCurrentProcess->SetLastTimeProcessed(fLastTime);
            }

            TESPackage* pCurrentPack = pCurrentProcess ? pCurrentProcess->GetCurrentPackage() : nullptr;
            if (pCurrentPack && pCurrentPack->GetPackageTarget()) {
                int32_t iTargCount = pCurrentPack->GetInitialTargetCount();
                pCurrentProcess->SetNumberOfItemsActivated(iTargCount);
            }

            bEVPBuffered = false;
            bResetAI = 0;
            return;
        }
        if (GetSitSleepState() == SIT_SLEEP_NORMAL ||
            GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
            GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
            StopMoving();
            if (pCurrentProcess) {
                pCurrentProcess->ClearCurrentDataforProcess(this);
                pCurrentProcess->SetCurrentActionComplete(true);
                pCurrentProcess->CheckforNewPackage(this, true);
                if (pCurrentProcess->eLevel) {
                    float fLastTime = appCalendar.GetHour() - 1.0;
                    pCurrentProcess->SetLastTimeProcessed(fLastTime);
                }
            }
            bEVPBuffered = false;
            bResetAI = 0;
        }
    }
}

bool Actor::ShouldRotateToTrack(TESObjectREFR* pRef) {
    bool bResult;
    if (this == pPlayer) {
        return false;
    }
    if (!Get3D()) {
        return false;
    }
    TESActorBase* pBase = GetActorBase();
    if (pBase && pBase->GetNoRotatingToHeadtrack()) {
        return false;
    }
    if (!pRef) {
        pRef = pCurrentProcess->GetCurrentHeadTrackTarget();
    }
    if (GetSitSleepState()) {
        return false;
    }
    if (!pCurrentProcess->GetCurrentHeadTrackTarget()) {
        return false;
    }
    if (pCurrentProcess->GetCurrentHeadTrackTarget() == this) {
        return false;
    }
    if (!IsPathingComplete()) {
        return false;
    }
    if (IsInCombat()) {
        return false;
    }
    if (Actor::GetAnimation()) {
        Animation* pAnim = Actor::GetAnimation();
        uint16_t sGroup = pAnim->GetGroup(AGS_WEAPON);
        if (TESAnimGroup::IsAttackAction(sGroup)) {
            return false;
        }
    }
    NiAVObject* pHead = nullptr;
    if (GetBiped()) {
        pHead = GetBiped()->GetParentBone(BIPED_BONE_HEAD);
    }
    else if (IsCreature()) {
        NiFixedString* pHeadName = FixedStrings::GetBip01_Head();
        NiNode* p3D = Get3D();
        pHead = BSUtilities::GetObjectByName(p3D, *pHeadName);
    }
    if (pHead) {
        NiFloatExtraDataController* pFloatController = static_cast<NiFloatExtraDataController*>(
            pHead->GetController(&NiFloatExtraDataController::ms_RTTI)
        );
        if (pFloatController) {
            NiInterpolator* pInterp = pFloatController->GetInterpolator(0);
            if (NiObject::IsExactKindOf(&NiBlendFloatInterpolator::ms_RTTI, pInterp) &&
                static_cast<NiBlendFloatInterpolator*>(pInterp)->m_fFloatValue >= 90.0) {
                return false;
            }
        }
    }
    bool bLineSight = true;
    if (pPlayer->IsSneaking() && pRef == pPlayer) {
        bool bPlayerInCombat = pPlayer->IsPlayerCharacterInCombat(nullptr);
        if (GetDetectionLevelAgainstActor(false, pPlayer, bLineSight, bPlayerInCombat, false, false, nullptr) <= 0) {
            return false;
        }
    }
    TESPackage* pPack = pCurrentProcess->GetCurrentPackage();
    if (!pPack) {
        return true;
    }
    switch (pPack->GetPackType()) {
        case PACKAGE_TYPE_EXPLORE:
        case PACKAGE_TYPE_ESCORT: {
            return !pCurrentProcess || (GetMoveMode() & 15) == 0;
        }
        case PACKAGE_TYPE_WANDER: {
            if (pCurrentProcess && (GetMoveMode() & 15) != 0) {
                return false;
            }
            Animation* pAnim = GetAnimation();
            return !pAnim || pAnim->SpecialIdleDonePlaying();
        }
        case PACKAGE_TYPE_TRAVEL:
        case PACKAGE_TYPE_PATROL:
        case PACKAGE_TYPE_GUARD: {
            TESObjectREFR* pPackRef = nullptr;
            if (pCurrentProcess->GetCurrentHeadTrackTarget(BaseProcess::HEAD_TRACK_TYPE_USE_WEAPON)) {
                return true;
            }
            if (pPack->pPackLoc) {
                pPackRef = pPack->pPackLoc->GetLocReference();
            }
            if (!pPackRef && pPack->GetPackageSecondLocation()) {
                pPackRef = pPack->GetPackageSecondLocation()->GetLocReference();
            }

            if (!pPackRef) {
                return true;
            }
            TESBoundObject* pObj = pPackRef->GetObjectReference();
            if (pObj == pXMarkerHeading) {
                return false;
            }
            if (pRef->GetObjectReference()->GetFormType() == IDLM_ID) {
                Animation* pAnim = Actor::GetAnimation();
                if (pAnim && !pAnim->SpecialIdleDonePlaying()) {
                    return false;
                }
            }
            if (pPack->pPackLoc && pPack->pPackLoc->GetLocType() == PLT_EDITORLOC) {
                return false;
            }
            if (pRef->IsFurniture()) {
                return false;
            }
            return !pActorMover->IsWaitingForAvoidance();
        }
        case PACKAGE_TYPE_USE_ITEM_AT:
        case PACKAGE_TYPE_DIALOGUE:
        case PACKAGE_TYPE_USE_WEAPON:
        case PACKAGE_TYPE_ACTIVATE:
        case PACKAGE_TYPE_IN_GAME_DIALOGUE: {
            return false;
        }
        case PACKAGE_TYPE_SANDBOX: {
            TESBoundObject* pObj;
            if (pCurrentProcess && (GetMoveMode() & 15) != 0) {
                return false;
            }
            SandBoxActorPackageData* pSandboxData = static_cast<SandBoxActorPackageData*>(
                pCurrentProcess->BaseProcess::GetInstanceDataThatIsRunning()
            );
            if (!pSandboxData || !pSandboxData->IsDoneWithIdleAnims(this, true)) {
                return false;
            }
            TESObjectREFR* pref = pSandboxData->pCurrentTargetObject;
            if (pref) {
                if (pref->IsFurniture()) {
                    return false;
                }
                pObj = pref->GetObjectReference();
                if (pObj) {
                    if (pObj == pXMarkerHeading) {
                        return false;
                    }
                    if (pObj->GetFormType() == IDLM_ID) {
                        return false;
                    }
                }
            }
            Animation* pAnim = GetAnimation();
            if (!pAnim || pAnim->SpecialIdleDonePlaying() || pSandboxData->eProcedure != SANDBOX_PROCEDURE_WANDER) {
                return true;
            }
            bResult = false;
            break;
        }
        default: {
            return true;
        }
    }
    return bResult;
}

TESObjectREFR* Actor::GetHeadingTarget() {
    if (GetCurrentAIProcess()) {
        TESObjectREFR* pTarget = GetCurrentAIProcess()->GetCurrentHeadTrackTarget();
        if (pTarget) {
            return pTarget;
        }
        if (GetCurrentPackage() && this != pPlayer) {
            TESObjectREFR* pPackageTarget = GetCurrentPackageTarget();
            if (pPackageTarget) {
                pPackageTarget->GetDistanceFromReference(this, false, false);
                fAIMaxHeadTrackDistance.Float();
            }
        }
    }
    return nullptr;
}

TESObjectREFR* Actor::GetBestDefaultHeadTrackTarget() {
    TESPackage* pPack = GetCurrentPackage();
    if (pPack &&
        (pPack->GetPackType() == PACKAGE_TYPE_TRAVEL || pPack->GetPackType() == PACKAGE_TYPE_AMBUSH) &&
        pPack->GetPackageTarget()) {
        TESObjectREFR* pPackTarg = pCurrentProcess->GetActorPackageThatIsRunning()->pTarget;
        if (pPackTarg) {
            float fMaxDistance = pPackTarg == pPlayer ? 500.0f : 100.0f;
            if (static_cast<double>(fMaxDistance) > GetDistanceFromReference(pPackTarg, false, true)) {
                return pPackTarg;
            }
        }
    }

    BSSimpleList<MobileObject*> potentialTargets;
    for (uint32_t i = 0; i < ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = ReferenceProcessLists.MobProcessArray.GetItem(i);
        if (CanHeadTrack(pMob, true)) {
            potentialTargets.AddHead(pMob);
        }
    }
    if (CanHeadTrack(pPlayer, true)) {
        MobileObject* pMob = pPlayer;
        potentialTargets.AddHead(pMob);
    }
    float fCurrentHighestTrackingScore = 0.0f;
    MobileObject* pCurrentBestTarget = nullptr;
    for (BSSimpleList<MobileObject*>* pPotentialTargets = &potentialTargets;
         pPotentialTargets && !pPotentialTargets->IsEmpty();
         pPotentialTargets = pPotentialTargets->m_pkNext) {
        MobileObject* pCurrentTarget = pPotentialTargets->m_item;
        float fCurrentScore = ComputeTargetDesirability(pCurrentTarget);
        if (fCurrentHighestTrackingScore < static_cast<double>(fCurrentScore)) {
            fCurrentHighestTrackingScore = fCurrentScore;
            pCurrentBestTarget = pCurrentTarget;
        }
    }
    potentialTargets.RemoveAll();
    return pCurrentBestTarget;
}

bool Actor::IsInWater() {
    return bInWater;
}

bool Actor::IsIgnoreFriendlyHits() {
    return (iFormFlags & TESForm::kFormFlags_IgnoreFriendlyHits) != 0;
}

void Actor::SetForceSneak(bool abflag) {
    bForceSneak = abflag;
}

bool Actor::FightsInWater() {
    TESActorBase* pActorBase = GetActorBase();
    return pActorBase->FightsInWater();
}

bool Actor::ShouldModulateVoice() {
    ExtraDataList* pExtraDataList = TESObjectREFR::GetExtra();
    InventoryChanges* pContainerChanges = pExtraDataList->GetContainerChanges();
    if (pContainerChanges) {
        return pContainerChanges->WearingVoiceModulateItem();
    }
    return false;
}

TESPackage* Actor::GetPackage() {
    return TESAIForm::GetCurrentPackage(this);
}

TESPackage* Actor::GetCurrentEditorPackage() {
    if (!pCurrentProcess) {
        return nullptr;
    }
    TESPackage* pPackage = pCurrentProcess->GetCurrentPackage();
    if (pPackage && !pPackage->IsInterruptPackage()) {
        return pPackage;
    }
    ExtraDataList* pExtraDataList = TESObjectREFR::GetExtra();
    return pExtraDataList->GetPackageExtra();
}

bool Actor::IsRunning() {
    return (GetMoveMode() & 512) != 0;
}

void Actor::ResetConditionPreventsRun() {
    if (pCurrentProcess && pCurrentProcess->HasCachedValues()) {
        pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_CONDITION_PREVENTS_RUN);
    }
}

bool Actor::IsFightingOrAlarmed() {
    TESPackage* pPackage = pCurrentProcess ? pCurrentProcess->GetCurrentPackage() : nullptr;
    if (!pPackage) {
        return false;
    }
    if (pPackage->GetPackType() == PACKAGE_TYPE_COMBAT || pPackage->GetPackType() == PACKAGE_TYPE_ALARM) {
        return true;
    }
    return false;
}

float Actor::GetSneakHeight(bool abEyeLevel) {
    if (abEyeLevel) {
        return static_cast<float>(GetEyeLevel() * 0.5699999928474426);
    }

    return static_cast<float>(GetHeight() * 0.5699999928474426);
}

int32_t Actor::GetGoldAmount() {
    ExtraDataList* pExtraDataList = TESObjectREFR::GetExtra();
    InventoryChanges* pContainerChanges = pExtraDataList->GetContainerChanges();
    if (pContainerChanges) {
        return pContainerChanges->GetGoldAmount();
    }
    return 0;
}

void Actor::PayGoldToActor(Actor* apRef, int32_t aiamount) {
    ExtraDataList* pExtra = TESObjectREFR::GetExtra();
    InventoryChanges* pContainerChanges = pExtra->GetContainerChanges();
    pContainerChanges->RemoveGold(this, aiamount, apRef);
}

ItemChange* Actor::GetRandomWornArmor(bool abHeadHit) {
    if (!abHeadHit) {
        return GetArmorBeingWorn(BO_UPPER_BODY);
    }
    ItemChange* pArmor = GetArmorBeingWorn(BO_HEAD);
    if (!pArmor) {
        return GetArmorBeingWorn(BO_HAIR);
    }
    return pArmor;
}

void Actor::SetCriticalStage(ACTOR_CRITICAL_STAGE aeStage) {
    eCriticalStage = aeStage;
    ApplyCriticalStage();
}

void Actor::ClearLastHitData() {
    if (MobileObject::GetCurrentAIProcess()) {
        BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
        pProcess->ClearLastHitData();
    }
}

bool Actor::GetAlert() {
    return pCurrentProcess && pCurrentProcess->GetAlert();
}

bool Actor::IsinSpectator() {
    TESPackage* pPackage = MobileObject::GetCurrentPackage();
    return pPackage && pPackage->GetPackType() == PACKAGE_TYPE_SPECTATOR;
}

bool Actor::IsSurfacing() {
    if (pCurrentProcess) {
        TESPackage* pPackage = pCurrentProcess->GetPackageThatIsRunning();
        if (pPackage) {
            return pPackage->GetPackType() == PACKAGE_TYPE_SURFACE;
        }
    }
    return false;
}

bool Actor::IsReloading() {
    int32_t iAnimAction = GetAnimAction();
    return iAnimAction == ANIM_ACTION_RELOAD ||
        (iAnimAction >= ANIM_ACTION_RELOAD_LOOP_START && iAnimAction <= ANIM_ACTION_RELOAD_LOOP);
}

void Actor::QueueReload() {
    if (pCurrentProcess) {
        pCurrentProcess->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_RELOAD_WEAPON);
    }
}

bool Actor::GetOffersServicesNow() {
    BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
    TESPackage* pPackage = pProcess->GetCurrentPackage();
    if (!pPackage) {
        return pPackage && pPackage->GetServicesEnabled();
    }
    if (!pPackage->IsInterruptPackage()) {
        return pPackage && pPackage->GetServicesEnabled();
    }
    ExtraDataList* pExtraData = TESObjectREFR::GetExtra();
    pPackage = pExtraData->GetPackageExtra();
    return pPackage && pPackage->GetServicesEnabled();
}

void Actor::SetUnConscious(bool abFlag) {
    if (abFlag) {
        SetLifeState(ACTOR_LIFE_STATE_UNCONSCIOUS);
    }
    else if (GetLifeState() == ACTOR_LIFE_STATE_UNCONSCIOUS) {
        SetLifeState(ACTOR_LIFE_STATE_ALIVE);
    }
}

void Actor::SetRestrained(bool abFlag) {
    if (abFlag) {
        SetLifeState(ACTOR_LIFE_STATE_RESTRAINED);
        ClearMoveModeAll();
    }
    else if (GetLifeState() == ACTOR_LIFE_STATE_RESTRAINED) {
        SetLifeState(ACTOR_LIFE_STATE_ALIVE);
    }
}

void Actor::SetForceUpdateQuest(bool abvalue) {
    bForceUpdateQuestTarget = abvalue;
}

bool Actor::IsVisible(int32_t aiVisMask) {
    return (aiVisMask & ~(aiVisMask & static_cast<int32_t>(iVisFlags))) == 0;
}

void Actor::FlushReloadTargetQueque() {
    uint8_t bQueued = bReloadTargetQueued;
    GetAnimation()->ReloadTargets(bQueued);
    bReloadTargetQueued = false;
}

Actor* Actor::GetCommandingActor() {
    if (pCurrentProcess) {
        return pCurrentProcess->GetCommandingActor();
    }

    return nullptr;
}

bool Actor::IsPathValid() const {
    return pActorMover->IsPathValid();
}

void Actor::ForceStopMoving() {
    pActorMover->ForceStopMoving();
}

void Actor::ResumeMoving() {
    pActorMover->ResumeMoving();
}

bool Actor::IsPathingComplete() const {
    return pActorMover->IsPathingComplete();
}

bool Actor::IsPathing() const {
    return pActorMover->IsPathing();
}

bool Actor::IsRotating() const {
    return pActorMover->IsRotating();
}

bool Actor::IsWaitingForAvoidance() const {
    return pActorMover->IsWaitingForAvoidance();
}

void Actor::SetPathingDoorReference(TESObjectREFR* apDoor) {
    pActorMover->SetPathingDoorReference(apDoor);
}

bool Actor::IsHeavyBodyArmorWorn() {
    return pCurrentProcess && pCurrentProcess->IsHeavyBodyArmorWorn();
}

bool Actor::IsPowerArmorHelmetWorn() {
    return pCurrentProcess && pCurrentProcess->IsPowerArmorHelmetWorn();
}

bool Actor::HasFollower(Actor* apFollower) {
    ExtraDataList* pExtra = TESObjectREFR::GetExtra();
    return pExtra->HasFollower(apFollower);
}

// Melee blood on the attacker's weapon: picks the target's blood impact data, places the decal origin (for the player
// from the weapon model's decal placement vectors) and adds the decal on the weapon bone and on the clone node.
void Actor::CreateBloodOnWeapon(float afDamage, HitData* apHitData, Actor* apTarget) {
    TESCreature* pCreature = dynamic_cast<TESCreature*>(GetActorBase());
    if (((pCreature && pCreature->IsHumanoidCreature()) || IsNpc()) && bDisableAllGore.Bool()) {
        return;
    }

    bool bBloodyMess = apHitData->pAggressor &&
        static_cast<ActorValueOwner*>(apHitData->pAggressor)->GetActorValue(ActorValue::BLOODY_MESS) > 0;
    int32_t iIndex = -1;
    BaseProcess* pBaseProcess = GetCurrentAIProcess();
    bool bHead = apHitData && (apHitData->eDamageLimb == 1 || apHitData->eDamageLimb == 2);
    bool bIsInPA = bHead ? apTarget->IsPowerArmorHelmetWorn() : apTarget->IsPowerBodyArmorWorn();
    int32_t eMaterial;
    if (bIsInPA) {
        eMaterial = 4;
    }
    else {
        // A reference, because the cast to the subobject is done without a null test.
        TESActorBaseData& rBaseData = *apTarget->GetActorBase();
        eMaterial = rBaseData.GetBloodImpactMaterial();
    }
    BGSBodyPartData* pBodyPartData = apTarget ? apTarget->GetActorBase()->GetBodyPartData() : nullptr;
    BGSBodyPart* pBodyPart = pBodyPartData ? pBodyPartData->GetBodyPart(apHitData->eDamageLimb) : nullptr;
    BGSImpactDataSet* pImpactDataSet = pBodyPart ? pBodyPart->GetDismemberImpactDataSet() : nullptr;
    BGSImpactData* pImpactData = nullptr;
    if (pImpactDataSet) {
        pImpactData = pImpactDataSet->GetImpactData(static_cast<BGSImpactDataSet::IMPACT_MATERIAL_TYPE>(eMaterial));
    }
    BGSTextureSet* pTexSet = pImpactData ? pImpactData->GetDecalTextureSet() : nullptr;
    if (!apHitData || !apTarget || !pTexSet || bIsInPA || pBaseProcess->IsCurrentWeaponGrenadeOrMine()) {
        return;
    }
    NiNode* pWeaponBone = pBaseProcess ? pBaseProcess->GetWeaponBone(GetBiped()) : nullptr;
    TESObjectWEAP* pWeapon = nullptr;
    if (pCurrentProcess->GetCurrentWeapon()) {
        pWeapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject());
    }
    if (!pWeaponBone || !pWeapon || pWeapon->IsRangedWeapon()) {
        return;
    }
    if (!(afDamage > BSMAX(1.0f, fMinBloodDamage.Float()))) {
        return;
    }
    NiNode* pClone = nullptr;
    NiPoint3 kOrigin;
    NiPoint3 kVectorDirection;
    NiPoint3 kUnused;
    if (this == pPlayer) {
        NiNode* pFirstPersonBone = pPlayer->GetBiped(true)->GetParentBone(BIPED_BONE_WEAPON);
        NiPoint3 kWeaponPos = GetWeaponPosition(true);
        kOrigin = (pWeaponBone->GetWorldTranslate() - kWeaponPos) / 2.0f;
        pClone = pPlayer->Is3rdPerson() ? pFirstPersonBone : pWeaponBone;
        pWeaponBone = pPlayer->Is3rdPerson() ? pWeaponBone : pFirstPersonBone;
        NiBound kBound = pWeaponBone->GetWorldBound();
        const NiPoint3& rBoneTranslate = pWeaponBone->GetWorldTranslate();
        // Computes the bound's centre minus the bone's translation and discards it.
        static_cast<void>(kBound.GetCenter() - rBoneTranslate);
        int32_t iGroup = 0;
        Animation* pPlayerAnimation = pPlayer->PlayerCharacter::GetAnimation();
        if (pPlayerAnimation) {
            BSAnimGroupSequence* pSequence = pPlayerAnimation->GetCurrentSequence(AGS_WEAPON);
            if (pSequence) {
                TESAnimGroup* pAnimGroup = pSequence->GetAnimGroup();
                if (pAnimGroup) {
                    iGroup = pAnimGroup->GetDecal();
                }
            }
        }
        NiExtraData* pExtra = nullptr;
        if (pWeaponBone->GetAtChecked(0)) {
            NiFixedString kName("DVPG");
            pExtra = pWeaponBone->GetAt(0)->GetExtraData(kName);
        }
        BSDecalPlacementVectorExtraData* pDecalData = static_cast<BSDecalPlacementVectorExtraData*>(pExtra);
        if (!pDecalData) {
            pDecalData = static_cast<BSDecalPlacementVectorExtraData*>(
                pWeaponBone->GetExtraData(*BSDecalPlacementVectorExtraData::GetTag())
            );
        }
        if (!pDecalData || iGroup >= pDecalData->usGroupCount) {
            return;
        }
        int32_t iVector = RandomInt(0, pDecalData->pDecalPlacementGroups[iGroup]->usVectorCount - 1);
        kOrigin = pDecalData->pDecalPlacementGroups[iGroup]->VectorOrigin[iVector];
        kVectorDirection = pDecalData->pDecalPlacementGroups[iGroup]->VectorDirection[iVector];
    }
    else {
        Animation* pAnimation = Actor::GetAnimation();
        if (pAnimation && pAnimation->GetCurrentSequence(AGS_WEAPON)) {
            // Reads the group's decal index here and does not use it.
            pAnimation->GetCurrentSequence(AGS_WEAPON)->GetAnimGroup()->GetDecal();
            const NiPoint3& rBoneTranslate = pWeaponBone->GetWorldTranslate();
            NiPoint3 kWeaponPos = GetWeaponPosition(false);
            NiPoint3 kHalf = (kWeaponPos - rBoneTranslate) / 2.0f;
            kOrigin = pWeaponBone->GetWorldTranslate() + kHalf;
        }
    }
    float fRandom = RandomFloat(0.0f, 4.0f);
    char cSubTex = static_cast<char>(static_cast<int32_t>(std::floor(fRandom)));
    float fRotation = RandomFloatTwoPi();
    float fMaxWidth = pImpactData->GetDecalMaxWidth();
    float fMinWidth = static_cast<float>(pImpactData->GetMinWidth());
    float fWidth = RandomFloat(fMinWidth, fMaxWidth);
    NiPoint3 kDirection;
    if (this == pPlayer) {
        kDirection = kVectorDirection;
    }
    else {
        kDirection = -apHitData->kHitDirection;
    }
    DECAL_CREATION_DATA kDecal;
    kDecal.pOrigin = kOrigin;
    kDecal.pDirection = kDirection;
    kDecal.pAVObj = pWeaponBone;
    kDecal.iIndex = iIndex;
    kDecal.pClone = pClone;
    kDecal.fRotation = fRotation;
    kDecal.cSubTex = cSubTex;
    kDecal.pTexSet = pTexSet;
    kDecal.fDecalWidth = fWidth;
    kDecal.fDecalHeight = fWidth;
    kDecal.fDecalDepth = 48.0f;
    kDecal.fSpecular = pImpactData->GetDecalShininess();
    kDecal.bUseOcclusionQuery = false;
    kDecal.fPlacementRadius = pImpactData->GetPlacementRadius();
    kDecal.fDecalEpsilon = pImpactData->GetDecalAngleThreshold();
    kDecal.bModelSpace = true;
    kDecal.bAlphaTesting = pImpactData->GetIsAlphaTest();
    kDecal.bAlphaBlending = pImpactData->GetIsAlphaBlend();
    // The three channels are computed in this order and passed to NiPoint3's constructor as z, y, x.
    float fZ = static_cast<float>(static_cast<int32_t>((pImpactData->GetDecalColor() >> 16) & 0xFF) / 255.0);
    float fY = static_cast<float>(static_cast<int32_t>((pImpactData->GetDecalColor() >> 8) & 0xFF) / 255.0);
    float fX = static_cast<float>(static_cast<int32_t>(pImpactData->GetDecalColor() & 0xFF) / 255.0);
    NiPoint3 kColor;
    NiPoint3* pColor = ::new (&kColor) NiPoint3(fX, fY, fZ);
    kDecal.fVertexColor.r = pColor->x;
    kDecal.fVertexColor.g = pColor->y;
    kDecal.fVertexColor.b = pColor->z;
    kDecal.bForceFade = true;
    if (bBloodyMess) {
        kDecal.fDecalWidth = static_cast<float>(kDecal.fDecalWidth * 1.5);
        kDecal.fDecalHeight = static_cast<float>(kDecal.fDecalHeight * 1.5);
    }
    GetParentCell()->AddDecal(kDecal, BGSDecalManager::DECAL_TYPE_SIMPLE, false);
    kDecal.pAVObj = pClone;
    GetParentCell()->AddDecal(kDecal, BGSDecalManager::DECAL_TYPE_SIMPLE, false);
}

void Actor::SetLastAttackHitData(HitData* apData) {
    if (MobileObject::GetCurrentAIProcess()) {
        BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
        pProcess->SetLastAttackHitData(apData);
    }
}

void Actor::PlayHitSound(HitData* apHitData) {
    bool bPowerArmor = IsPowerBodyArmorWorn();
    NiPoint3 damageLocation = apHitData->kHitLocation;
    NiPoint3 distanceDelta = NiPoint3::ZERO;
    float distance = 0.0f;
    if (apHitData->pTarget == pPlayer) {
        damageLocation = pPlayer->GetLocationOnReference();
    }
    else {
        distanceDelta = damageLocation - Main::WorldRootCamera()->GetWorldTranslate();
        distance = distanceDelta.Length();
    }
    TESActorBaseData* pBaseData = static_cast<TESActorBaseData*>(GetActorBase());
    BGSImpactDataSet::IMPACT_MATERIAL_TYPE eImpactMaterial = pBaseData->GetBloodImpactMaterial();
    TESObjectWEAP* pWeap = nullptr;
    bool bCreatureAttack = false;
    TESSound* pImpactSound1 = nullptr;
    TESSound* pImpactSound2 = nullptr;
    TESSound* pCreatureAttackSound = nullptr;
    BGSImpactDataSet* pImpactSet = nullptr;
    BGSImpactData* pImpactData = nullptr;
    BSSoundHandle soundHandle;
    if (apHitData->pWeapon) {
        pWeap = apHitData->pWeapon;
    }
    else if (apHitData->pAggressor && apHitData->pAggressor->IsCreature()) {
        bCreatureAttack = true;
    }
    else {
        if (!apHitData->pAggressor || apHitData->pAggressor->IsCreature()) {
            return;
        }
        pWeap = pFists;
    }
    if (bCreatureAttack) {
        TESCreature* pCreature = static_cast<TESCreature*>(apHitData->pAggressor->GetObjectReference());
        pCreatureAttackSound = pCreature ? pCreature->PickCreatureSound(CREA_SOUND_WEAPON) : nullptr;
        pImpactSet = pCreature ? pCreature->GetImpactDataSet() : nullptr;
        pImpactData = pImpactSet ? pImpactSet->GetImpactData(eImpactMaterial) : nullptr;
    }
    else if (pWeap) {
        pImpactSet = pWeap->pImpactDataSet;
        pImpactData = pImpactSet ? pImpactSet->GetImpactData(eImpactMaterial) : nullptr;
    }
    if (pImpactData) {
        pImpactSound1 = pImpactData->pSound1;
        pImpactSound2 = pImpactData->pSound2;
    }
    if (pImpactSound1 && distance < static_cast<double>(100 * pImpactSound1->GetData().iMax)) {
        const char* pFilename = pImpactSound1->GetFilename();
        soundHandle = BSAudio::QInstance().GetSoundHandleByFilename(
            pFilename,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
            pImpactSound1
        );
        soundHandle.SetPosition(damageLocation.x, damageLocation.y, damageLocation.z);
        soundHandle.Play(false);
    }

    if (pImpactSound2 && distance < static_cast<double>(100 * pImpactSound2->GetData().iMax)) {
        const char* pFilename = pImpactSound2->GetFilename();
        soundHandle = BSAudio::QInstance().GetSoundHandleByFilename(
            pFilename,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
            pImpactSound2
        );
        soundHandle.SetPosition(damageLocation.x, damageLocation.y, damageLocation.z);
        soundHandle.Play(false);
    }

    if (pCreatureAttackSound && distance < static_cast<double>(100 * pCreatureAttackSound->GetData().iMax)) {
        const char* pFilename = pCreatureAttackSound->GetFilename();
        soundHandle = BSAudio::QInstance().GetSoundHandleByFilename(
            pFilename,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
            pCreatureAttackSound
        );
        soundHandle.SetPosition(damageLocation.x, damageLocation.y, damageLocation.z);
        soundHandle.Play(false);
    }

    if (bPowerArmor) {
        pImpactData = pImpactSet ? pImpactSet->GetImpactData(BGSImpactDataSet::IMP_MAT_METAL) : nullptr;
        if (pImpactData) {
            pImpactSound1 = pImpactData->pSound1;
            if (pImpactSound1 && distance < static_cast<double>(100 * pImpactSound1->GetData().iMax)) {
                const char* pFilename = pImpactSound1->GetFilename();
                soundHandle = BSAudio::QInstance().GetSoundHandleByFilename(
                    pFilename,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
                    pImpactSound1
                );
                soundHandle.SetPosition(damageLocation.x, damageLocation.y, damageLocation.z);
                soundHandle.Play(false);
            }

            pImpactSound2 = pImpactData->pSound2;
            if (pImpactSound2 && distance < static_cast<double>(100 * pImpactSound2->GetData().iMax)) {
                const char* pFilename = pImpactSound2->GetFilename();
                soundHandle = BSAudio::QInstance().GetSoundHandleByFilename(
                    pFilename,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
                    pImpactSound2
                );
                soundHandle.SetPosition(damageLocation.x, damageLocation.y, damageLocation.z);
                soundHandle.Play(false);
            }
        }
    }
}

void Actor::CreateBlood(float afDamage, HitData* apHitData) {
    NiPoint3 kColor;
    NiPoint3 contactPoint;
    alignas(16) NiPoint3 kRay;
    PlayerCharacter* pActor = static_cast<PlayerCharacter*>(this);
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TES_ACTOR,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor.cpp",
        0x1F69
    );
    int32_t iHeadHit = apHitData &&
        (apHitData->eDamageLimb == BGSBodyPart::LIMB_HEAD_1 || apHitData->eDamageLimb == BGSBodyPart::LIMB_HEAD_2);
    bool bPowerArmor = iHeadHit ? pActor->IsPowerArmorHelmetWorn() : pActor->IsPowerBodyArmorWorn();
    alignas(16) NiPoint3 damageLocation = apHitData->kHitLocation;
    Actor* pAttacker = apHitData->pAggressor;
    BGSBodyPart::LIMB_ENUM eLimb = apHitData->eDamageLimb;
    if (pActor->HasBlood() && afDamage > 0.0) {
        if (pActor == pPlayer) {
            int32_t iSplatterCount = CombatFormulas::GetScreenSplatterCount(apHitData->fHealthDamage);
            if (iSplatterCount > 0) {
                ScreenSplatter::Activate(static_cast<uint32_t>(iSplatterCount), 0.0f, SCREENSPLATTER_BLOOD);
            }
        }
        bool bBloodyMess = apHitData->pAggressor && apHitData->pAggressor->GetActorValue(ActorValue::BLOODY_MESS) > 0;
        NiPoint3 distance = damageLocation - Main::WorldRootCamera()->GetWorldTranslate();
        int32_t iShowBlood = Main::GetSceneIsComplex();
        if (!iShowBlood) {
            uint32_t uiCombatantCount = pCombatManager->GetCombatantCount(nullptr, nullptr);
            iShowBlood = uiCombatantCount > iMaxHiPerfCombatCount.Int() &&
                !pActor->IsPc() &&
                pAttacker &&
                !pAttacker->IsPc();
            if (!iShowBlood) {
                double dDistance = distance.Length();
                iShowBlood = fGunParticleCameraDistance.Float() >= dDistance ||
                    !PointInFrustum(damageLocation, Main::WorldRootCamera(), 32.0f);
            }
        }
        if (iShowBlood) {
            alignas(16) NiPoint3 randomvector;
            int32_t iIndex = -1;
            float fRotation = RandomFloatTwoPi();
            float fSubTexRoll = RandomFloat(0.0f, 4.0f);
            uint8_t cSubTex = static_cast<int32_t>(std::floor(fSubTexRoll));
            BGSImpactDataSet::IMPACT_MATERIAL_TYPE eImpactMaterial;
            if (bPowerArmor) {
                eImpactMaterial = BGSImpactDataSet::IMP_MAT_METAL;
            }
            else {
                TESActorBaseData* pBaseData = static_cast<TESActorBaseData*>(pActor->GetActorBase());
                eImpactMaterial = pBaseData->GetBloodImpactMaterial();
            }
            TESObjectWEAP* pWeap = nullptr;
            if (apHitData->pAggressor) {
                pWeap = apHitData->pAggressor->GetCurrentWeapon();
            }
            BGSImpactDataSet* pImpactSet = pWeap ? pWeap->pImpactDataSet : nullptr;
            BGSImpactData* pImpactData = pImpactSet ? pImpactSet->GetImpactData(eImpactMaterial) : nullptr;
            BGSTextureSet* pTexSet = pImpactData ? pImpactData->pDecalTextureSet : nullptr;
            NiNode* p3D = pActor->Get3D();
            randomvector.x = RandomFloat(-0.8f, 0.8f);
            randomvector.y = RandomFloat(-0.8f, 0.8f);
            randomvector.z = RandomFloat(-0.8f, 0.8f);
            NiPoint3 bloodvector = randomvector + 2.0f * apHitData->kHitDirection;
            bloodvector.Unitize();
            NiPoint3 bloodorigin = damageLocation;
            if (!pImpactData && !pTexSet) {
                pImpactData = BGSImpactDataSet::GetDefaultImpactDataSet()->GetImpactData(eImpactMaterial);
                pTexSet = pImpactData ? pImpactData->pDecalTextureSet : nullptr;
            }
            char cDecalAdded = 0;
            if (p3D &&
                pTexSet &&
                ((eLimb != BGSBodyPart::LIMB_HEAD_1 && eLimb != BGSBodyPart::LIMB_HEAD_2) || !pActor->IsNpc())) {
                cDecalAdded = 1;
                float fMaxWidth = pImpactData->GetDecalMaxWidth();
                float fMinWidth = pImpactData->GetMinWidth();
                float fDecalSize = RandomFloat(fMinWidth, fMaxWidth);
                DecalCaster* pCaster = DecalCaster::GetInstance(fDecalSize, true);
                pCaster->SetCustomPickCollisions(DecalCaster::CUSTOM_PICK_BIPED);
                TESObjectCELL* pCell = pActor->GetParentCell();
                bhkWorld* pWorld = pCell ? pCell->GetbhkWorld() : nullptr;
                if (pWorld) {
                    pCaster->Add(pWorld);
                    hkpAllCdBodyPairCollector collector;
                    pCaster->GetPenetrations(bloodorigin, collector);
                    int32_t iHits = collector.m_hits.m_size;
                    if (iHits) {
                        alignas(16) DECAL_CREATION_DATA data;
                        data.pOrigin = bloodorigin;
                        data.pDirection = bloodvector;
                        data.fDecalWidth = fDecalSize;
                        data.fDecalHeight = fDecalSize;
                        data.pAVObj = p3D;
                        data.iIndex = iIndex;
                        data.fRotation = fRotation;
                        data.cSubTex = cSubTex;
                        data.pTexSet = pTexSet;
                        for (int32_t i = 0; i < iHits; ++i) {
                            const hkpCollidable* pCol = collector.m_hits[i].m_rootCollidableB;
                            if (pCol) {
                                NiAVObject* pAVObj = bhkUtilFunctions::GetNiAVObject(pCol);
                                TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pAVObj);
                                if (pRef && pRef == pActor) {
                                    eLimb = pActor->GetDamageLocation(static_cast<NiNode*>(pAVObj));
                                    data.iBodyParts |= 1 << (eLimb + 1);
                                }
                            }
                        }
                        data.iBodyParts &= ~1u;
                        if (pAttacker && pAttacker->IsPc()) {
                            pActor->GetParentCell()->AddDecal(data, BGSDecalManager::DECAL_TYPE_PROJECTED, true);
                        }
                        else {
                            pActor->GetParentCell()->AddDecal(data, BGSDecalManager::DECAL_TYPE_PROJECTED, false);
                        }
                    }
                }
            }

            char cParticleAdded = 0;
            if (pImpactData && pImpactData->GetModel() && *pImpactData->GetModel()) {
                cParticleAdded = 1;
                NiAVObject* pFollowObject = nullptr;
                if (apHitData && apHitData->pSourceRef && apHitData->pSourceRef->IsProjectile()) {
                    Projectile* pProjectile = static_cast<Projectile*>(apHitData->pSourceRef);
                    Projectile::ImpactData* pImpact = pProjectile->GetImpactData(pActor);
                    if (pImpact && pImpact->pTargetWorldObject) {
                        pFollowObject = GetAVObjectForCollidable(&pImpact->pTargetWorldObject->m_collidable);
                    }
                }

                NiPoint3 kOrigin = bloodorigin;
                NiPoint3 kDirection = -bloodvector;
                const char* pModel = pImpactData->GetModel();
                BSTempEffectParticle::Spawn(
                    pActor->GetParentCell(),
                    1.0f,
                    pModel,
                    kDirection,
                    kOrigin,
                    1.0f,
                    7u,
                    pFollowObject
                );
            }

            if (cParticleAdded || cDecalAdded) {
                TESActorBase* pActorBase = pActor->GetActorBase();
                BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
                BGSBodyPart* pBodyPart = pBodyPartData ? pBodyPartData->GetBodyPart(apHitData->eDamageLimb) : nullptr;
                pImpactSet = pBodyPart ? pBodyPart->GetDismemberImpactDataSet() : nullptr;
                pImpactData = pImpactSet ? pImpactSet->GetImpactData(eImpactMaterial) : nullptr;
                pTexSet = pImpactData ? pImpactData->pDecalTextureSet : nullptr;
                if (!bPowerArmor && pImpactData) {
                    double dBloodChanceRoll = RandomFloat0To1();
                    if (fCombatEnvironmentBloodChance.Float() > dBloodChanceRoll) {
                        DecalCaster* pCaster = DecalCaster::GetInstance(32.0f, true);
                        pCaster->SetCustomPickCollisions(DecalCaster::CUSTOM_PICK_ACTORS);
                        TESObjectCELL* pCell = pActor->GetParentCell();
                        bhkWorld* pWorld = pCell ? pCell->GetbhkWorld() : nullptr;
                        if (pWorld) {
                            alignas(16) NiPoint3 dir;
                            dir.x = bloodvector.x;
                            dir.y = bloodvector.y;
                            dir.z = bloodvector.z - 0.5;
                            dir.Unitize();
                            NiPoint3 to = bloodorigin + dir * 512.0f;
                            bhkPickData pd;
                            CFilter filter;
                            filter.iFilter = 0;
                            filter.SetLayer(L_CUSTOMPICK1);
                            pd.SetFilter(filter);
                            pd.NiSetFrom(bloodorigin);
                            pd.NiSetTo(to);
                            hkpAllRayHitCollector pcollector;
                            pd.SetAllCollector(&pcollector);
                            pWorld->PickObject(pd);
                            int32_t iHitCount = pd.pAllCollector->getHits().m_size;
                            char cPlaced = 0;
                            for (int32_t i = 0; !cPlaced && i < iHitCount; ++i) {
                                const hkpCollidable* pCol = pd.pAllCollector->getHits()[i].m_rootCollidable;
                                if (pCol) {
                                    NiPoint3 contactNormal;
                                    ::new (&kRay) NiPoint3(to - bloodorigin);
                                    ::new (&contactPoint)
                                        NiPoint3(bloodorigin + pd.pAllCollector->getHits()[i].m_hitFraction * kRay);
                                    HK2NINS(contactNormal, pd.pAllCollector->getHits()[i].m_normal);
                                    uint32_t uiFilterInfo = pCol->getCollisionFilterInfo();
                                    CFilter filterInfo;
                                    filterInfo.iFilter = uiFilterInfo;
                                    uint32_t uiGroup = filterInfo.GetGroup();
                                    bool bLand = uiGroup == 1;
                                    NiAVObject* pAVObj = nullptr;
                                    if (uiGroup == 1) {
                                        pAVObj = TES::pInstance->GetLandGeometry(contactPoint);
                                        if (pAVObj && pAVObj->m_pkParent) {
                                            pAVObj = pAVObj->m_pkParent;
                                        }
                                    }
                                    else {
                                        pAVObj = bhkUtilFunctions::GetNiAVObject(pCol);
                                    }
                                    TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pAVObj);
                                    if (bLand ||
                                        (pRef &&
                                         BGSDecalManager::IsValidDecalImpact(
                                             pRef,
                                             BGSDecalManager::DECAL_TYPE_SIMPLE
                                         ))) {
                                        pCell = TES::pInstance->GetCellForPoint(contactPoint);
                                        float fMaxWidth = pImpactData->GetDecalMaxWidth();
                                        float fMinWidth = pImpactData->GetMinWidth();
                                        float fDecalSize = RandomFloat(fMinWidth, fMaxWidth);
                                        alignas(16) DECAL_CREATION_DATA data;
                                        data.pOrigin = contactPoint;
                                        data.pDirection = contactNormal;
                                        data.fDecalWidth = fDecalSize;
                                        data.fDecalHeight = fDecalSize;
                                        data.fDecalDepth = 48.0f;
                                        data.fRotation = fRotation;
                                        data.cSubTex = cSubTex;
                                        data.pAVObj = pAVObj;
                                        data.pTexSet = pTexSet;
                                        data.bUseParallaxOcclusion = pImpactData->GetIsParallax();
                                        data.fParallaxOcclusionScale = pImpactData->GetDecalParallaxScale();
                                        data.ucParallaxOcclusionMaxPasses = pImpactData->GetParallaxPasses();
                                        data.bAlphaTesting = pImpactData->GetIsAlphaTest();
                                        data.bAlphaBlending = pImpactData->GetIsAlphaBlend();
                                        data.fSpecular = pImpactData->GetDecalShininess();
                                        data.fDecalEpsilon = pImpactData->Data.fAngleThreshold;
                                        data.fPlacementRadius = pImpactData->Data.fPlacementRadius;
                                        int32_t iBlue = static_cast<uint8_t>(pImpactData->GetDecalColor() >> 16);
                                        float fBlue = static_cast<double>(iBlue) / 255.0;
                                        int32_t iGreen = static_cast<uint8_t>(
                                            static_cast<uint16_t>(pImpactData->GetDecalColor()) >> 8
                                        );
                                        float fGreen = static_cast<double>(iGreen) / 255.0;
                                        int32_t iRed = static_cast<uint8_t>(pImpactData->GetDecalColor());
                                        float fRed = static_cast<double>(iRed) / 255.0;
                                        NiPoint3* pColor = ::new (&kColor) NiPoint3(fRed, fGreen, fBlue);
                                        data.fVertexColor.r = pColor->x;
                                        data.fVertexColor.g = pColor->y;
                                        data.fVertexColor.b = pColor->z;
                                        if (bBloodyMess) {
                                            data.fDecalWidth = data.fDecalWidth * 1.5;
                                            data.fDecalHeight = data.fDecalHeight * 1.5;
                                        }
                                        pCell->AddDecal(data, BGSDecalManager::DECAL_TYPE_SIMPLE, false);
                                        cPlaced = 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void Actor::Stagger() {
    if (GetAnimAction() != ANIM_ACTION_STAGGER) {
        Animation* pAnimation = Actor::GetAnimation();
        if (pAnimation && pCurrentProcess && GetSitSleepState() == SIT_SLEEP_NORMAL) {
            uint16_t sAnimGroup = GetAnimGroup(ANIM_GROUP_RECOIL, nullptr, false, nullptr);
            if (TESAnimGroup::GetType(sAnimGroup) == ANIM_GROUP_RECOIL) {
                pAnimation->PlayGroup(sAnimGroup, ACTION_START, -1, AGS_NONE);
                SetAnimAction(ANIM_ACTION_RECOIL, pAnimation->GetCurrentSequence(AGS_WEAPON));
                StartAnimOn1stPerson(sAnimGroup, ACTION_START);
            }
            else {
                pAnimation->ClearGroup(AGS_WEAPON, 0.0);
                if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
                    pPlayer->GetAnimation(true)->ClearGroup(AGS_WEAPON, 0.0);
                }
            }
        }
    }
}

// Pushes the dead target's 3D away from the shot: a projectile's hit along its direction from 2048 units back,
// otherwise from the weapon's last position towards the weapon, with the death force (or the weapon's kill impulse
// times 2.5, falling off past the kill impulse distance for projectiles).
void Actor::DoWeaponKillForce(HitData* apData) {
    Actor* pTarget = apData->pTarget;
    float fDamage = apData->fHealthDamage;
    bool bRanged = apData->pSourceRef && apData->pSourceRef->IsProjectile();
    NiAVObject* pAV = Get3D();
    NiPoint3 startPos = NiPoint3::ZERO;
    NiPoint3 stopPos = NiPoint3::ZERO;
    if (pAV) {
        // Reads the weapon's last position before the world transform.
        const NiPoint3* pLastPos = GetCurrentAIProcess()->GetWeaponLastPos();
        startPos = pAV->GetWorldTransform() * *pLastPos;
    }
    if (!pTarget) {
        return;
    }
    if (pTarget->IsCreature() && !pTarget->HasRagDoll()) {
        return;
    }
    NiAVObject* p3D = pTarget->TESObjectREFR::Get3D();
    if (!p3D) {
        return;
    }
    NiPoint3 forceLocation = NiPoint3::ZERO;
    if (bRanged) {
        alignas(16) NiPoint3 kNegDirection = -apData->kHitDirection;
        forceLocation = kNegDirection;
        forceLocation.Unitize();
        forceLocation *= 2048.0f;
        forceLocation += apData->kHitLocation;
    }
    else {
        if (pAV) {
            // Gets the weapon position before the world transform.
            NiPoint3 kWeaponPos = GetWeaponPosition(false);
            stopPos = pAV->GetWorldTransform() * kWeaponPos;
        }
        forceLocation = startPos - stopPos;
        forceLocation.Unitize();
        alignas(16) NiPoint3 kNegDirection = -apData->kHitDirection;
        forceLocation += kNegDirection;
        forceLocation *= 1024.0f;
        forceLocation += startPos;
    }
    float fMagnitude = CombatFormulas::GetDeathForce(fDamage, bRanged);
    if (apData->pWeapon) {
        fMagnitude = apData->pWeapon->GetKillImpulse() * 2.5;
        if (bRanged && apData->pWeapon->GetKillImpulseDistance() > 0.0) {
            alignas(16) NiPoint3 targetPos = p3D->GetWorldTranslate();
            NiPoint3 targetDist = targetPos - startPos;
            float fKIDSquared = apData->pWeapon->GetKillImpulseDistance();
            fKIDSquared = fKIDSquared * fKIDSquared;
            float targetDistSquared = targetDist.x * targetDist.x + targetDist.y * targetDist.y;
            if (fKIDSquared < targetDistSquared) {
                // Compares the squared distance with twice the unsquared kill impulse distance, deliberately.
                if (targetDistSquared > apData->pWeapon->GetKillImpulseDistance() * 2.0) {
                    fMagnitude = fMagnitude * 0.10000000149011612;
                }
                else {
                    float fMagMod = (fKIDSquared * 2.0 - targetDistSquared) / fKIDSquared * 0.8999999761581421 +
                        0.10000000149011612;
                    fMagMod = BSMAX(fMagMod, 0.1f);
                    fMagnitude = fMagnitude * fMagMod;
                }
            }
        }
    }
    bhkForceController::RemoveForce(p3D);
    bhkWorld::SetMotion(p3D, hkpMotion::MOTION_DYNAMIC, true, false, true);
    NiUpdateData UpdateData(0.0f, false, false);
    p3D->Update(UpdateData);
    hkVector4 loc;
    NI2HK(loc, forceLocation);
    TESHavokUtilities::AddExplosionImpulse(p3D, loc, fMagnitude, apData);
}

void Actor::DismemberLimbs(HitData* apData, TESForm* apDeathObject, CAUSE_OF_DEATH aeCause) {
    TESCreature* pCreature = dynamic_cast<TESCreature*>(GetActorBase());
    if (((!pCreature || !pCreature->IsHumanoidCreature()) && !IsNpc()) || !bDisableAllGore.Bool()) {
        TESActorBase* pActorBase = GetActorBase();
        BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
        if (pBodyPartData && apData) {
            if (iActorsDismemberedThisFrame < iActorsDismemberedPerFrame.Int()) {
                ActorValue::Index eDismemberConditions[7];
                int32_t iDismemberBonus[7];
                int32_t iExplodeBonus[7];
                BGSBodyPart::LIMB_ENUM eDismemberLimbs[7];
                ++iActorsDismemberedThisFrame;
                for (int32_t i = 0; i < 7; ++i) {
                    eDismemberLimbs[i] = BGSBodyPart::LIMB_NONE;
                    eDismemberConditions[i] = ActorValue::VALUE_NONE;
                    iDismemberBonus[i] = 0;
                    iExplodeBonus[i] = 0;
                }
                int32_t iLimbCount = 0;
                bool bTorsoExplodes = false;
                if (apData->pAggressor && apData->pAggressor->GetActorValue(ActorValue::BLOODY_MESS) > 0) {
                    ActorValue::Index eTargetedLimbCondition = ActorValue::VALUE_NONE;
                    BGSBodyPart* pBodyPart = pBodyPartData->GetBodyPart(apData->eDamageLimb);
                    GetActorValue(ActorValue::ENDURANCE_CONDITION);
                    BGSBodyPart::LIMB_ENUM eTorsoPart = BGSBodyPart::LIMB_NONE;
                    if (pBodyPart) {
                        bool bExplode = (apData->IsFlagSet(HitData::HD_EXPLODE_LIMB) &&
                                         pBodyPart->IsExplodablePart()) ||
                            (pBodyPart->IsExplodablePart() &&
                             pBodyPart->IsExplodeChanceAbsolute() &&
                             pBodyPart->GetExplosionChance() >= 100u);
                        if (pBodyPart->GetActorValue() == 26) {
                            uint32_t uiRoll;
                            if (bExplode ||
                                (pBodyPart->IsExplodablePart() &&
                                 (uiRoll = Random(), uiRoll % 100 < iMessTorsoExplodeChance.Int()))) {
                                eTorsoPart = pBodyPart->GetPartType();
                                bTorsoExplodes = true;
                            }
                        }
                        else {
                            eTargetedLimbCondition = static_cast<ActorValue::Index>(pBodyPart->GetActorValue());
                            eDismemberLimbs[0] = apData->eDamageLimb;
                            eDismemberConditions[0] = eTargetedLimbCondition;
                            iDismemberBonus[0] = 100;
                            iExplodeBonus[0] = bExplode ? 100 : iMessTargetedLimbExplodeBonus.Int();
                            iLimbCount = 1;
                        }
                    }
                    for (int32_t i = 0; i < 7 && iLimbCount < 7; ++i) {
                        ActorValue::Index eCondition = ActorValue::ToActorValue(ActorValue::CONDITION, i);
                        if (eCondition != eTargetedLimbCondition &&
                            eCondition != ActorValue::VALUE_NONE &&
                            eCondition != ActorValue::ENDURANCE_CONDITION) {
                            pBodyPart = pBodyPartData->GetBodyPart(eCondition);
                            if (pBodyPart) {
                                bool bExplode = apData->IsFlagSet(HitData::HD_EXPLODE_LIMB) ||
                                    (pBodyPart->IsExplodablePart() &&
                                     pBodyPart->IsExplodeChanceAbsolute() &&
                                     pBodyPart->GetExplosionChance() >= 100u);
                                if (GetActorValue(eCondition) > 0) {
                                    uint32_t uiRoll;
                                    if (bExplode ||
                                        bTorsoExplodes ||
                                        (uiRoll = Random(), uiRoll % 100 <= iMessIntactLimbDismemberChance.Int())) {
                                        eDismemberLimbs[iLimbCount] = pBodyPart->GetPartType();
                                        eDismemberConditions[iLimbCount] = eCondition;
                                        iDismemberBonus[iLimbCount] = 100;
                                        iExplodeBonus[iLimbCount++] = bExplode ? 100
                                                                               : iMessIntactLimbExplodeBonus.Int();
                                    }
                                }
                                else {
                                    eDismemberLimbs[iLimbCount] = pBodyPart->GetPartType();
                                    eDismemberConditions[iLimbCount] = eCondition;
                                    iDismemberBonus[iLimbCount] = 100;
                                    iExplodeBonus[iLimbCount++] = bExplode ? 100 : iMessCrippledLimbExplodeBonus.Int();
                                }
                            }
                        }
                    }
                    if (bTorsoExplodes && eTorsoPart != BGSBodyPart::LIMB_NONE) {
                        eDismemberLimbs[iLimbCount] = eTorsoPart;
                        eDismemberConditions[iLimbCount] = ActorValue::ENDURANCE_CONDITION;
                        iDismemberBonus[iLimbCount] = 100;
                        iExplodeBonus[iLimbCount++] = 100;
                    }
                }
                else if (apData->pSourceRef && apData->pSourceRef->IsExplosion()) {
                    BSSimpleList<Explosion::ExplodedLimb*>* pLimbList = static_cast<Explosion*>(apData->pSourceRef)
                                                                            ->Explosion::GetAffectedLimbList(this);
                    while (pLimbList && !pLimbList->IsEmpty() && iLimbCount < 7) {
                        Explosion::ExplodedLimb* pLimbEntry = pLimbList->m_item;
                        pLimbList = pLimbList->m_pkNext;
                        BGSBodyPart* pBodyPart = pBodyPartData->GetBodyPart(pLimbEntry->eLimb);
                        if (pBodyPart && pLimbEntry->eLimb) {
                            eDismemberLimbs[iLimbCount] = pLimbEntry->eLimb;
                            eDismemberConditions[iLimbCount++] = static_cast<ActorValue::Index>(
                                pBodyPart->GetActorValue()
                            );
                        }
                    }
                }
                else {
                    BGSBodyPart* pBodyPart = pBodyPartData->GetBodyPart(apData->eDamageLimb);
                    bool bExplode = pBodyPart &&
                        pBodyPart->IsExplodablePart() &&
                        pBodyPart->IsExplodeChanceAbsolute() &&
                        pBodyPart->GetExplosionChance() >= 100u;
                    if (pBodyPart && (bExplode || apData->eDamageLimb)) {
                        ActorValue::Index eCondition = static_cast<ActorValue::Index>(pBodyPart->GetActorValue());
                        if (eCondition != ActorValue::VALUE_NONE && GetActorValue(eCondition) <= 0) {
                            eDismemberLimbs[0] = apData->eDamageLimb;
                            eDismemberConditions[0] = eCondition;
                            iLimbCount = 1;
                        }
                    }
                }
                if (iLimbCount <= 0) {
                    Dismember(
                        apData,
                        ActorValue::VALUE_NONE,
                        BGSBodyPart::LIMB_NONE,
                        apDeathObject,
                        aeCause,
                        0,
                        0,
                        false
                    );
                }
                for (int32_t i = 0; i < iLimbCount; ++i) {
                    if (eDismemberLimbs[i] != BGSBodyPart::LIMB_NONE &&
                        eDismemberConditions[i] != ActorValue::VALUE_NONE) {
                        if (bTorsoExplodes &&
                            eDismemberConditions[i] == ActorValue::ENDURANCE_CONDITION &&
                            !apData->IsFlagSet(HitData::HD_EXPLODE_LIMB)) {
                            apData->SetFlag(HitData::HD_EXPLODE_LIMB, 1);
                            Dismember(
                                apData,
                                eDismemberConditions[i],
                                eDismemberLimbs[i],
                                apDeathObject,
                                aeCause,
                                iDismemberBonus[i],
                                iExplodeBonus[i],
                                bTorsoExplodes
                            );
                            apData->SetFlag(HitData::HD_EXPLODE_LIMB, 0);
                        }
                        else {
                            Dismember(
                                apData,
                                eDismemberConditions[i],
                                eDismemberLimbs[i],
                                apDeathObject,
                                aeCause,
                                iDismemberBonus[i],
                                iExplodeBonus[i],
                                bTorsoExplodes
                            );
                        }
                    }
                }
            }
        }
    }
}

bool Actor::CheckAttackDisarm(Actor* apTarget, uint32_t aeWeaponSkill) {
    return false;
}

bool Actor::CheckBlockDisarm(Actor* apTarget) {
    return false;
}

bool Actor::CheckBlockStagger() {
    return false;
}

bool Actor::IsFollowAccompanyPackageTarget(TESForm* apTarget) {
    if (!pCurrentProcess) {
        return false;
    }
    TESPackage* pPackage = MobileObject::GetCurrentPackage();
    if (!pPackage) {
        return false;
    }
    if ((pPackage->GetPackType() == PACKAGE_TYPE_FOLLOW || pPackage->GetPackType() == PACKAGE_TYPE_ACCOMPANY) &&
        static_cast<const void*>(pCurrentProcess->GetTarget()) == static_cast<const void*>(apTarget)) {
        return true;
    }
    return false;
}

void Actor::FadeIn() {
    if (MobileObject::GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
        BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
        static_cast<HighProcess*>(pProcess)->FadeIn(this, false);
    }
}

void Actor::FadeOut() {
    if (MobileObject::GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
        BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
        static_cast<HighProcess*>(pProcess)->FadeOut(this, nullptr, false);
    }
}

float Actor::GetFadePercent() {
    if (MobileObject::GetCurrentProcessType()) {
        return 1.0;
    }
    BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
    return static_cast<HighProcess*>(pProcess)->GetFadeAlpha();
}

void Actor::IncrementMinorCrimes() {
    ++iMinorCrimes;
}

void Actor::SetActorRefEssential(bool abEssential) {
    if (abEssential) {
        iMajorCrimes |= ACTOR_FLAG_REF_ESSENTIAL;
    }
    else {
        iMajorCrimes &= ~ACTOR_FLAG_REF_ESSENTIAL;
    }
}

void Actor::SetAlwaysShowSubtitles(bool abShow) {
    if (abShow) {
        iMajorCrimes |= ACTOR_FLAG_ALWAYS_SHOW_SUBTITLES;
    }
    else {
        iMajorCrimes &= ~ACTOR_FLAG_ALWAYS_SHOW_SUBTITLES;
    }
}

int32_t Actor::GetShowSubtitles() {
    return (iMajorCrimes & ACTOR_FLAG_ALWAYS_SHOW_SUBTITLES) != 0;
}

bool Actor::CanSneak() {
    return IsNpc();
}

void Actor::SetEditorLocationForm(TESForm* apSpace) {
    pEditorLocForm = apSpace;
}

bool Actor::IsNotOutOfBreath() {
    double dEndurance = GetEndurance();
    float fBreathTime = GamePlayFormulas::SwimBreathTime(static_cast<int32_t>(dEndurance));
    if (pCurrentProcess) {
        return fBreathTime <= pCurrentProcess->GetBreathTimer();
    }
    return true;
}

void Actor::SetWalkSpeed(float afSpeed) {
    if (pCurrentProcess) {
        pCurrentProcess->SetCachedWalkSpeed(afSpeed);
    }
}

void Actor::SetRunSpeed(float afSpeed) {
    if (pCurrentProcess) {
        pCurrentProcess->SetCachedRunSpeed(afSpeed);
    }
}

bool Actor::HasObjects(
    TESBoundObject* apObj,
    PACKAGE_OBJECT_TYPE aeFormID,
    int32_t ainumber,
    uint32_t aId,
    PACKAGE_OBJECT_TYPE& aeMatchId
) {
    ExtraDataList* pExtra = TESObjectREFR::GetExtra();
    InventoryChanges* pContainerChanges = pExtra->GetContainerChanges();
    return pContainerChanges && pContainerChanges->ContainerHasObjects(apObj, aeFormID, ainumber, aeMatchId, aId, this);
}

bool Actor::ShouldTalkTo(TESObjectREFR* apRef) {
    if (MobileObject::GetCurrentAIProcess()) {
        if (!IsParalyzed() && !IsSleeping() && !IsUnConscious()) {
            BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
            if (pProcess->GetGreetingFlag()) {
                return false;
            }
        }
        if (TESObjectREFR::GetParentCell()) {
            TESObjectCELL* pParentCell = TESObjectREFR::GetParentCell();
            if (pParentCell->GetInterior()) {
                const NiPoint3& kLocation = GetLocationOnReference();
                float fHeightDiff = kLocation.z - apRef->GetLocationOnReference().z;
                if (std::fabs(fHeightDiff) > 90.0) {
                    return false;
                }
            }
        }
        if (MobileObject::GetCurrentAIProcess()->GetCurrentPackage() &&
            ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_RANDOM_CONVERSATIONS)) {
            return false;
        }
        if ((GetSitSleepState() == SIT_SLEEP_IS_SITTING || GetSitSleepState() == SIT_SLEEP_NORMAL) &&
            (!MobileObject::GetCurrentPackage() || !MobileObject::GetCurrentPackage()->IsInterruptPackage())) {
            if (MobileObject::GetCurrentAIProcess()->GetAwarePlayerTimer() <= 0.0) {
                return true;
            }
        }
    }
    return false;
}

void Actor::HandleDeletedReferenceForActorMover(TESObjectREFR* apRef) {
    if (pActorMover) {
        pActorMover->RemoveReference(apRef);
    }
}

bool Actor::GetCurrentPathfindingGoal(PathingLocation* apRetLocation) {
    return pActorMover->GetCurrentGoal(apRetLocation);
}

void Actor::BuildRequest(
    PathingRequest* apRequest,
    const NiPoint3& aGoalLocation,
    TESObjectCELL* apCell,
    TESWorldSpace* apWorldSpace,
    float afTargetRadius,
    PathingAvoidNodeArray* apAvoidNodes
) {
    ActorMover::BuildRequest(this, apRequest, aGoalLocation, apCell, apWorldSpace, afTargetRadius, apAvoidNodes);
    if (IsInCombat()) {
        apRequest->SetAllowIncompletePath(false);
    }
}

uint32_t Actor::GetCurrentNodeIndex() {
    return pActorMover->GetCurrentNodeIndex();
}

char Actor::GetPathingLocationAt(int32_t aiNodeIndex, NiPoint3* apLocation) {
    return pActorMover->GetLocationAt(aiNodeIndex, apLocation);
}

PathingLocation* Actor::GetFailedPathDestination() {
    return pActorMover->GetFailedPathDestination();
}

void Actor::SetLookAtTarget(NiPoint3 aPoint) {
    pActorMover->SetLookAtTarget(aPoint);
}

bool Actor::SetPathfindingHide(const PathingRequest* apRequest) {
    return pActorMover->SetGoal(apRequest);
}

void Actor::SetActionSoundValue(int32_t aiValue) {
    iActionValue = static_cast<uint32_t>(aiValue);
    fTimeronAction = fActorAlertSoundTimer.Float();
}

void Actor::ActivatedReference(
    TESObjectREFR* apRef,
    TESBoundObject* apObject,
    int32_t aiCount,
    bool abActivatedScriptOnly
) {
    CombatController* pCombatController = GetCombatController();
    if (pCombatController) {
        pCombatController->ActivatedReference(apRef, apObject, aiCount, abActivatedScriptOnly);
    }
}

void Actor::SetPlayerTeammate(bool abValue) {
    if (static_cast<const void*>(this) != static_cast<const void*>(pPlayer)) {
        if (abValue && !bPlayerTeammate) {
            pPlayer->AddTeammate(this);
        }
        else if (!abValue && bPlayerTeammate) {
            pPlayer->RemoveTeammate(this);
        }
        bPlayerTeammate = abValue;
    }
}

void Actor::IncrementMajorCrimes() {
    iMajorCrimes = (iMajorCrimes & (ACTOR_FLAG_REF_ESSENTIAL | ACTOR_FLAG_ALWAYS_SHOW_SUBTITLES)) |
        (GetMajorCrimesNumber() + 1);
}

int32_t Actor::ModifyMoveModeForFollow(int32_t emovemode, float afbaseRadius, float afDistToTarget) {
    PlayerCharacter* pLeader = static_cast<PlayerCharacter*>(GetCurrentPackageTarget());
    if (pLeader && pLeader->IsActor()) {
        float fDistance = TESObjectREFR::GetDistanceFromReference(pLeader, false, false);
        float fRadiusWalk = AiFormulas::GetFollowRadiusWalk(afbaseRadius);
        float fMatchSpeed = AiFormulas::GetFollowRadiusMatchSpeed(afbaseRadius);
        int16_t sMoveMode = static_cast<int16_t>(pLeader->Actor::GetMoveMode());
        if (!(fRadiusWalk > static_cast<double>(fDistance)) || (sMoveMode & 15) != 0) {
            if (CanRun() &&
                ((sMoveMode & 1280) == 1024 ||
                 fMatchSpeed < static_cast<double>(fDistance) ||
                 (pLeader == pPlayer && (emovemode & 2048) != 0) ||
                 (sMoveMode & 512) != 0)) {
                return static_cast<int32_t>((emovemode & ~0x300u) | 512);
            }
        }
        else {
            return static_cast<int32_t>((emovemode & ~0x300u) | 256);
        }
    }

    return emovemode;
}

void Actor::SetCombatStyle(TESCombatStyle* apCombatStyle) {
    ExtraDataList* pExtra = TESObjectREFR::GetExtra();
    pExtra->SetCombatStyle(apCombatStyle);
    CombatController* pCombatController = GetCombatController();
    if (pCombatController) {
        pCombatController->UpdateCurrentCombatStyle();
    }
}

bool Actor::CanChildUseObject(TESObjectREFR* apObjectRef) {
    TESBoundObject* pBaseForm = apObjectRef->TESObjectREFR::GetObjectReference();
    int32_t eType = static_cast<int32_t>(pBaseForm->TESForm::GetFormType());
    if (eType != ACTI_ID && eType != FURN_ID && eType != IDLM_ID) {
        return true;
    }
    pBaseForm = apObjectRef->TESObjectREFR::GetObjectReference();
    return pBaseForm->TESForm::GetRefractedByAutoWater();
}

void Actor::SetIgnoreFriendlyHits(bool abSet) {
    if (abSet) {
        iFormFlags |= TESForm::kFormFlags_IgnoreFriendlyHits;
    }
    else {
        iFormFlags &= ~TESForm::kFormFlags_IgnoreFriendlyHits;
    }
}

NiPoint3* Actor::GetActorMoverOverrideMovement() {
    return &pActorMover->CachedMoveDelta;
}

float Actor::DifficultyLevelAdjustHealthModifier(float afValue, Actor* apAttacker) {
    uint8_t bPC = IsPc();
    GamePlayFormulas::DifficultyLevel eDifficulty = pPlayer->GetGameDifficulty();
    float fDifficultyMult = GamePlayFormulas::GetDifficultyMultiplier(eDifficulty, ActorValue::HEALTH, bPC);
    return afValue * fDifficultyMult;
}

void Actor::InitiateMovementBlockedPackage() {
    EndMovement();
    TESPackage* pPackage = TESPackage::CreatePackage(PACKAGE_TYPE_MOVEMENT_BLOCKED);
    pPackage->CalculateProcedureType(nullptr);
    if (MobileObject::GetCurrentProcessType() > PROCESS_TYPE_MIDDLE_HIGH) {
        pPackage->SetMustComplete(true);
        PutCreatedPackage(pPackage, false, false);
    }
    else {
        PutCreatedPackage(pPackage, true, true);
    }
}

void Actor::RemoveFleeTarget(TESObjectREFR* apRef) {
    FleePackage* pFleePackage = nullptr;
    TESPackage* pPackage = MobileObject::GetCurrentPackage();
    if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_FLEE) {
        pFleePackage = static_cast<FleePackage*>(MobileObject::GetCurrentPackage());
    }

    if (pFleePackage && !pFleePackage->IsFleeListEmpty()) {
        pFleePackage->RemoveAvoidedRef(apRef);
        if (pFleePackage->IsFleeListEmpty()) {
            if (IsInCombat()) {
                StopCombat(nullptr);
            }
            else {
                EndInterruptPackage(false);
            }
        }
    }
}

void Actor::SetHasBeenEaten(bool abFlag) {
    if (IsDead(false)) {
        ExtraDismemberedLimbs* pExtraData = static_cast<ExtraDismemberedLimbs*>(
            TESObjectREFR::GetExtra()->GetExtraData(EXTRA_DISMEMBERED_LIMBS)
        );
        if (!pExtraData) {
            TESObjectREFR::GetExtra()->AddDismembermentExtra();
            pExtraData = static_cast<ExtraDismemberedLimbs*>(
                TESObjectREFR::GetExtra()->GetExtraData(EXTRA_DISMEMBERED_LIMBS)
            );
            AddChange(CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS);
        }
        if (pExtraData) {
            pExtraData->SetEaten(abFlag);
        }
    }
}

int32_t Actor::GetDetectionLevel(Actor* apActor) {
    if (!pCurrentProcess) {
        return -100;
    }
    DetectionState* pState = pCurrentProcess->GetDetectionState(apActor, 0);
    if (pState) {
        return pState->iLevel;
    }

    return -100;
}

float Actor::GetCurrentAmmoClipPercent() {
    ItemChange* pAmmo = pCurrentProcess->GetCurrentAmmo();
    if (!pAmmo) {
        return 1.0;
    }
    ItemChange* pWeaponItem = pCurrentProcess->GetCurrentWeapon();
    if (!pWeaponItem) {
        return 1.0;
    }
    TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pWeaponItem->ItemChange::GetItemChangeObject());
    uint8_t bClipSizeMod = pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
    float fClipRounds = static_cast<float>(static_cast<double>(pWeapon->GetFormClipRounds(bClipSizeMod)));
    float fCount = static_cast<float>(static_cast<double>(pAmmo->ItemChange::GetCount()));
    if (fClipRounds == 0.0) {
        return 1.0;
    }
    return fCount / fClipRounds;
}

void Actor::StopAttack() {
    if (pCurrentProcess) {
        int32_t iAnimAction = pCurrentProcess->GetAnimAction();
        if (iAnimAction == ANIM_ACTION_ATTACK || iAnimAction == ANIM_ACTION_ATTACK_FOLLOW_THROUGH) {
            if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
                Animation* pAnim1st = pPlayer->GetAnimation(true);
                Animation* pAnim3rd = pPlayer->GetAnimation(false);
                pAnim1st->ClearGroup(AGS_WEAPON, 0.0);
                pAnim3rd->ClearGroup(AGS_WEAPON, 0.0);
                pAnim1st->ClearGroup(AGS_LEFT_ARM, 0.0);
                pAnim3rd->ClearGroup(AGS_LEFT_ARM, 0.0);
            }
            else {
                Animation* pAnimation = pCurrentProcess->GetAnimation();
                pAnimation->ClearGroup(AGS_WEAPON, 0.0);
                pAnimation->ClearGroup(AGS_LEFT_ARM, 0.0);
            }
            pCurrentProcess->SetAnimAction(ANIM_ACTION_NONE, nullptr);
        }
    }
}

bool Actor::SetPathfindingGoalAndAngle(
    TESObjectREFR* apReference,
    float afTargetRadius,
    PathingAvoidNodeArray* apAvoidNodes
) {
    if (!apAvoidNodes) {
        apAvoidNodes = MobileObject::GetCurrentAIProcess()->CreateAvoidArray(this);
    }
    return pActorMover->SetGoalAndAngle(apReference, afTargetRadius, apAvoidNodes);
}

void Actor::BlockHit(bool abBlockAttack) {
    if (GetAnimAction() != ANIM_ACTION_STAGGER) {
        Animation* pAnimation = Actor::GetAnimation();
        if (pAnimation && pCurrentProcess && GetSitSleepState() == SIT_SLEEP_NORMAL) {
            uint16_t sAnimGroup = GetAnimGroup(ANIM_GROUP_BLOCK_HIT, nullptr, false, nullptr);
            if (TESAnimGroup::GetType(sAnimGroup) != ANIM_GROUP_BLOCK_HIT) {
                TESAnimGroup::GetType(sAnimGroup);
            }
            if (TESAnimGroup::GetType(sAnimGroup) == ANIM_GROUP_BLOCK_HIT) {
                pAnimation->PlayGroup(sAnimGroup, ACTION_START, -1, AGS_NONE);
                BSAnimGroupSequence* pSequence = pAnimation->GetCurrentSequence(
                    animGroupInfo[ANIM_GROUP_BLOCK_HIT].eSectionLayer
                );
                SetAnimAction(ANIM_ACTION_BLOCK, pSequence);
                StartAnimOn1stPerson(sAnimGroup, ACTION_START);
            }
        }
    }
}

bool Actor::BreathesUnderwater() {
    if (!IsNpc() && SwimsOnly()) {
        return true;
    }
    if (static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::WATER_BREATHING)) {
        return true;
    }
    return false;
}

void Actor::SetLastHitData(HitData* apData) {
    if (MobileObject::GetCurrentAIProcess()) {
        BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
        pProcess->SetLastHitData(apData);
    }
}

int32_t Actor::GetCurrentAmmoClipCount() {
    ItemChange* pAmmo = pCurrentProcess->GetCurrentAmmo();
    if (pAmmo) {
        return pAmmo->ItemChange::GetCount();
    }

    return -1;
}

bool Actor::SetLocalVariableOnActor(const char* asVariableName, double afNewValue) {
    uint32_t uiID;
    if (!this) {
        return false;
    }
    TESBoundObject* pObject = TESObjectREFR::GetObjectReference();
    Script* pScript = TESScriptableForm::GetFormScript(pObject);
    ScriptLocals* pScriptVariables = GetScriptVariables();
    if (!pScript) {
        return false;
    }
    if (!pScriptVariables) {
        return false;
    }
    pScript->FindVariable(asVariableName, &uiID);
    if (!uiID) {
        return false;
    }
    pScriptVariables->SetVariable(uiID, afNewValue);
    return true;
}

void Actor::ClearDispositionModifiers() {
    for (BSSimpleList<DispositionModifier*>* pNode = &DispModifierList; pNode && pNode->m_item;
         pNode = pNode->m_pkNext) {
        ourFree(pNode->m_item);
    }
    DispModifierList.RemoveAll();
}

void Actor::ClearLastAttackHitData() {
    if (MobileObject::GetCurrentAIProcess()) {
        BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
        pProcess->ClearLastAttackHitData();
    }
}

void Actor::ResetFaceData() {
    BSFaceGenAnimationData* pFaceData = GetFaceAnimationData();
    if (pFaceData) {
        pFaceData->ResetCurrentMorphs(0.75f, false, true, true, true, false);
    }
}

DETECTION_LEVEL Actor::GetDetectionLevelValue(Actor* apActor) {
    if (pCurrentProcess) {
        DetectionState* pState = pCurrentProcess->GetDetectionState(apActor, 0);
        if (pState) {
            return pState->eDetectionLevel;
        }
    }
    return DETECTION_LOST;
}

void Actor::AlertAccompanyToPackageChange() {
    ExtraDataList* pExtra = TESObjectREFR::GetExtra();
    ExtraFollower* pFollowerExtra = pExtra->GetFollowerExtra();
    if (pFollowerExtra) {
        for (BSSimpleList<Actor*>* pNode = pFollowerExtra->pActorlist; pNode && pNode->m_item;
             pNode = pNode->m_pkNext) {
            Actor* pFollower = pNode->m_item;
            if (pFollower->MobileObject::GetCurrentAIProcess()) {
                TESPackage* pPackage = pFollower->MobileObject::GetCurrentAIProcess()->GetPackageThatIsRunning();
                if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_ACCOMPANY) {
                    pFollower->MobileObject::GetCurrentAIProcess()->SetProcedureIndexRunning(0);
                }
            }
        }
    }
}

void Actor::ForceMoveMode(uint32_t auiMoveMode) {
    pActorMover->ForceMoveMode(auiMoveMode);
}

void Actor::InitiateDoNothingPackage() {
    BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
    if (pProcess->GetProcessLevel() < PROCESS_TYPE_MIDDLE_LOW) {
        TESPackage* pPackage = TESPackage::CreatePackage(PACKAGE_TYPE_DO_NOTHING);
        pPackage->SetMustComplete(true);
        pPackage->CalculateProcedureType(nullptr);
        PutCreatedPackage(pPackage, true, true);
    }
}

void Actor::StartGreetingPlayer(TESTopic* apTopic) {
    if (apTopic && apTopic->TESTopic::GetQuestInfoList()) {
        BSSimpleList<QUEST_INFO*>* pQuestInfos = apTopic->TESTopic::GetQuestInfoList();
        bool bHasQuestInfos = !pQuestInfos->IsEmpty();
        if (bHasQuestInfos && pCurrentProcess) {
            pCurrentProcess->SetActionHeadTrackTarget(pPlayer);
            if (pPlayer->IsGreetingTimerClear()) {
                MobileObject::SetDialogueItemTarget(pPlayer);
                pCurrentProcess->SetGreetingFlag(true);
                bool bLoadLipFiles = bBackgroundLoadLipFiles.Bool();
                pCurrentProcess->ProcessGreet(this, apTopic, false, false, bLoadLipFiles, false);
                pPlayer->SetLastGreetingTime();
            }
            if (!pCurrentProcess->GetSitSleepState() && pCurrentProcess->GetGreetingFlag()) {
                pCurrentProcess->CreateFollownoEscort(this, true);
            }
        }
    }
}

bool Actor::IsTryingToEnterFurniture() {
    if (GetSitSleepState() &&
        GetSitSleepState() != SIT_SLEEP_IS_SITTING &&
        GetSitSleepState() != SIT_SLEEP_IS_SLEEPING) {
        if (MobileObject::GetCurrentAIProcess()->GetFurnitureMarker()) {
            return MobileObject::GetCurrentAIProcess()->GetFurnitureMarker()->cNumber <= 20;
        }
    }

    return false;
}

bool Actor::GetHasBeenEaten() {
    if (!IsDead(false)) {
        return false;
    }
    ExtraDismemberedLimbs* pExtra = static_cast<ExtraDismemberedLimbs*>(m_Extra.GetExtraData(EXTRA_DISMEMBERED_LIMBS));
    if (pExtra) {
        return pExtra->bEaten;
    }

    return false;
}

ANIM_GROUP_ENUM Actor::GetIronSightsAttack(Animation* apAnim, ANIM_GROUP_ENUM aeAttack, TESObjectWEAP* apWeapon) {
    if (apWeapon && apWeapon->GetIronSightFOV() <= 0.0) {
        return aeAttack;
    }
    if (apAnim == pPlayer->GetAnimation(true) && (!apWeapon || apWeapon->GetUse1PISAnims())) {
        return static_cast<ANIM_GROUP_ENUM>(aeAttack + 3);
    }
    if (apAnim == pPlayer->GetAnimation(true) || (apWeapon && !apWeapon->GetUse3PISAnims())) {
        return aeAttack;
    }
    return static_cast<ANIM_GROUP_ENUM>(aeAttack + 3);
}

void Actor::AddFaction(TESFaction* apFaction, int32_t aiRank) {
    AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    ExtraDataList* pExtra = TESObjectREFR::GetExtra();
    ExtraFactionChanges* pFactionChanges = pExtra->GetFactionChangesExtra();
    if (!pFactionChanges) {
        TESObjectREFR::GetExtra()->AddFactionChangesExtra();
        pFactionChanges = TESObjectREFR::GetExtra()->GetFactionChangesExtra();
    }
    if (pFactionChanges) {
        pFactionChanges->AddToFaction(apFaction, aiRank);
    }
}

char Actor::IsEvil() {
    TESForm* pOwner = GetOwner();
    if (pOwner) {
        if (pOwner->GetFormType() == FACT_ID) {
            return static_cast<TESFaction*>(pOwner)->TESFaction::GetIsEvil();
        }

        if (pOwner->GetFormType() == NPC__ID) {
            return static_cast<TESNPC*>(pOwner)->TESActorBaseData::IsInEvilFactionsOnly();
        }
    }
    else {
        TESActorBase* pActorBase = GetTemplateActorBase();
        if (!pActorBase) {
            pActorBase = static_cast<TESActorBase*>(GetObjectReference());
        }
        TESActorBase* pNPC = nullptr;
        if (pActorBase && pActorBase->GetFormType() == NPC__ID) {
            pNPC = pActorBase;
        }

        if (pNPC) {
            return pNPC->IsInEvilFactionsOnly();
        }
    }
    return 0;
}

void Actor::AddDeathItems() {
    TESLevItem* pLevItem = GetActorBase()->pDeathItem;
    if (pLevItem) {
        TESContainer tempcont;
        uint16_t sLevel = GetLevel();
        pLevItem->CalculateCurrentFormList(sLevel, 1, &tempcont, 0);
        for (BSSimpleList<ContainerObject*>* pList = &tempcont.objectList; pList && pList->m_item;
             pList = pList->m_pkNext) {
            ContainerObject* pCont = pList->m_item;
            TESBoundObject* pObj = nullptr;
            if (pList->m_item->pObj) {
                TESBoundObject* pItemToAdd = pList->m_item->pObj;
                if (pItemToAdd->IsBoundObject()) {
                    pObj = pList->m_item->pObj;
                }
            }
            if (pObj) {
                Script* pScript = TESScriptableForm::GetFormScript(pCont->pObj);
                if (pScript) {
                    for (uint32_t n = 0; n < pCont->i_count; ++n) {
                        ExtraDataList* pExtra = new ExtraDataList;
                        pExtra->SetCount(1);
                        if (pExtra && !pExtra->GetScript()) {
                            pExtra->SetScript(pScript);
                            ScriptLocals* pLocals = pExtra->GetScript()->CloneLocals();
                            pExtra->SetScriptLocals(pLocals);
                        }

                        if (pCont->pItemExtra) {
                            TESHealthForm* pHealthForm = TESHealthForm::GetFormAsHealthForm(pCont->pObj);
                            pCont->pItemExtra->AddToExtraDataList(pExtra, pHealthForm);
                        }
                        AddObjecttoContainer(pObj, pExtra, 1);
                    }
                }
                else if (pCont->pItemExtra) {
                    ExtraDataList* pExtra = new ExtraDataList;
                    pExtra->SetCount(pCont->i_count);
                    TESHealthForm* pHealthForm = TESHealthForm::GetFormAsHealthForm(pCont->pObj);
                    pCont->pItemExtra->AddToExtraDataList(pExtra, pHealthForm);
                    AddObjecttoContainer(pObj, pExtra, pCont->i_count);
                }
                else {
                    AddObjecttoContainer(pObj, nullptr, pCont->i_count);
                }
            }
        }
    }
    InventoryChanges* pInventory = m_Extra.GetContainerChanges();
    if (pInventory) {
        pInventory->InitScripts();
    }
}

bool Actor::HasBlood() {
    TESCreature* pCreature = dynamic_cast<TESCreature*>(GetActorBase());
    if (((pCreature && pCreature->IsHumanoidCreature()) || IsNpc()) && (bDisableAllGore.Bool() || IsChild(false))) {
        return false;
    }
    if (eCriticalStage) {
        return false;
    }
    TESActorBaseData* pBaseData = static_cast<TESActorBaseData*>(GetActorBase());
    if (pBaseData->GetNoBloodDecal()) {
        pBaseData = static_cast<TESActorBaseData*>(GetActorBase());
        if (pBaseData->GetNoBloodSpray()) {
            return false;
        }
    }
    return true;
}

const char* Actor::GetRaceFullName() {
    TESNPC* pNpc = nullptr;
    if (IsNpc()) {
        pNpc = static_cast<TESNPC*>(GetObjectReference());
    }
    if (pNpc) {
        // Tests the race for null before it adjusts it to its TESFullName base.
        TESFullName* pName = pNpc->GetRace();
        if (pName && pName->GetFullNameLength()) {
            return pName->GetFullName();
        }
    }
    return nullptr;
}

NiAVObject* Actor::ExplodeLimb(
    BGSBodyPart::LIMB_ENUM aeLimbIndex,
    NiAVObject* apOldLimb,
    NiTransform* aTrans,
    bool abSpawnEffects
) {
    NiTransform kDest;
    TESObjectREFR* pRef = this;
    NiAVObject* pResult = nullptr;
    TESActorBase* pActorBase = GetActorBase();
    BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
    if (pBodyPartData && pRef->GetParentCell()) {
        BGSBodyPart* pBodyPart = pBodyPartData->GetBodyPart(aeLimbIndex);
        if (pBodyPart && pBodyPart->IsExplodablePart()) {
            if (abSpawnEffects) {
                if (pBodyPart->Data.pExplosion) {
                    NiMatrix3 kRotate = apOldLimb->GetWorldTransform().m_Rotate;
                    NiPoint3 kTranslate = aTrans->m_Translate;
                    TESObjectCELL* pCell = pRef->GetParentCell();
                    Explosion::SpawnExplosion(pBodyPart->Data.pExplosion, nullptr, nullptr, pCell, kTranslate, kRotate);
                }
                if (pBodyPart->Data.pExplosionGenericDebris && pBodyPart->GetDebrisCount()) {
                    // Keeps the reference's scale as a double and rounds the product to float.
                    double dRefScale = pRef->GetScale();
                    float fDebrisScale = pBodyPart->Data.fExplosionGenericDebrisScale * dRefScale;
                    uint8_t ucCount = pBodyPart->GetDebrisCount();
                    TESObjectCELL* pCell = pRef->GetParentCell();
                    pBodyPart->Data.pExplosionGenericDebris
                        ->SpawnDebris(pCell, aTrans->m_Translate, ucCount, fDebrisScale);
                }
            }
            if (pBodyPart->ExplosionSpecialDebris.GetModelLength() && apOldLimb) {
                TESModel* pModel = &pBodyPart->ExplosionSpecialDebris;
                NiNode* p3D = pModelLoader->LoadFile(pModel->GetModel(), LOD_MULT_NONE, true, false, false, false);
                if (p3D) {
                    BSCloningProcess cloneProcess(1.0f);
                    pResult = static_cast<NiAVObject*>(p3D->Clone(cloneProcess));
                    NiTransform rotMult;
                    NiTransform rot;
                    pRef->Get3D()->GetWorldTransform().InvertNonUniform(rotMult);
                    rot = *::new (&kDest) NiTransform(rotMult * apOldLimb->GetWorldTransform());
                    pResult->SetTranslate(rot.m_Translate);
                    pResult->SetRotate(rot.m_Rotate);
                    pResult->SetScale(pBodyPart->Data.fExplosionSpecialDebrisScale);
                }
                const char* pFilename = pBodyPart->ExplosionSpecialDebris.GetModel();
                pModelLoader->ReleaseFile(pFilename);
            }
        }
    }

    return pResult;
}

void Actor::HideDismemberedLimb(
    BGSBodyPart::LIMB_ENUM aeLimbIndex,
    NiNode* apLimbRoot,
    NiAVObject* apLimbReplacement,
    bool abExplode
) {
    NiNode* pActorRoot = TESObjectREFR::Get3D();
    NiNode* pClonedLimbRoot = apLimbReplacement ? apLimbReplacement->m_pkParent : nullptr;
    TESActorBase* pActorBase = GetActorBase();
    BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
    BGSBodyPart* pBodyPart = pBodyPartData ? pBodyPartData->GetBodyPart(aeLimbIndex) : nullptr;
    if (pBodyPart && apLimbRoot) {
        for (int32_t n = 0; n < pActorRoot->GetArrayCount(); ++n) {
            NiAVObject* pObj = pActorRoot->GetAt(static_cast<uint32_t>(n));
            if (pObj &&
                Animation::FindSkinnedNode(pObj) &&
                pObj != pClonedLimbRoot &&
                IsSkinnedToLimb(pObj, apLimbRoot)) {
                if (abExplode) {
                    BGSBodyPart::HideLimb(aeLimbIndex, pObj);
                    if (aeLimbIndex) {
                        pBodyPartData->HideChildLimbs(apLimbRoot, pActorRoot, aeLimbIndex, pObj);
                    }
                    else {
                        pObj->SetAppCulled(true);
                    }
                }
                else {
                    BSCloningProcess cloneProcess(1.0f);
                    NiAVObject* pCloneSkin = static_cast<NiAVObject*>(pObj->Clone(cloneProcess));
                    BGSBodyPart::HideLimb(aeLimbIndex, pObj);
                    pBodyPartData->HideChildLimbs(apLimbRoot, pActorRoot, aeLimbIndex, pObj);
                    BGSBodyPart::ShowLimb(aeLimbIndex, pCloneSkin);
                    pBodyPartData->ShowChildLimbs(apLimbRoot, pActorRoot, aeLimbIndex, pCloneSkin);
                    pClonedLimbRoot->AttachChild(pCloneSkin, 1);
                    BipedAnim::AttachToSkeleton(apLimbReplacement->IsNode(), pCloneSkin, nullptr, false);
                }
            }
        }
    }
}

void Actor::ClearDismemberedLimbs() {
    m_Extra.RemoveDismembermentExtra();
    NiAVObject* p3D = TESObjectREFR::Get3D();
    TESActorBase* pActorBase = GetActorBase();
    BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
    if (p3D && pCurrentProcess && pBodyPartData) {
        NiNode* pRoot = p3D->IsNode();
        if (pRoot) {
            uint16_t sLimbsFound = 0;
            uint32_t uiChildCount = pRoot->GetArrayCount();
            for (uint32_t i = 0; i < uiChildCount; ++i) {
                NiAVObject* pChild = pRoot->GetAt(i);
                if (pChild) {
                    const char* pName = pChild->m_kName.GetHandleString();
                    if (pName && !BSstrnicmp(pName, "DismemberedLimb", 15u)) {
                        int32_t iLimb = std::atoi(pName + 15);
                        bhkWorld::RemoveObjects(pChild, true, true);
                        pRoot->DetachChildAt(i);
                        sLimbsFound |= 1 << iLimb;
                    }
                }
            }
            if (sLimbsFound) {
                ReloadTargets(false);
                pActorBase = GetActorBase();
                BaseProcess* pProcess = pCurrentProcess;
                pBodyPartData = pActorBase->GetBodyPartData();
                pProcess->StoreDamageNodes(static_cast<NiNode*>(p3D), pBodyPartData);
                bhkWorld* pWorld = GetParentCell()->GetbhkWorld();
                // The limb is compared unsigned.
                for (BGSBodyPart::LIMB_ENUM eLimb = BGSBodyPart::LIMB_TORSO;
                     static_cast<uint32_t>(eLimb) < static_cast<uint32_t>(BGSBodyPart::LIMB_COUNT);
                     eLimb = static_cast<BGSBodyPart::LIMB_ENUM>(eLimb + 1)) {
                    if ((1 << eLimb & sLimbsFound) != 0) {
                        NiNode* pLimbRoot = pCurrentProcess->GetDamageNode(eLimb);
                        if (pLimbRoot) {
                            FillBoneArray(pLimbRoot, false);
                            pLimbRoot->SetAppCulled(false);
                            UnhideDismemberedLimb(eLimb, pLimbRoot);
                            if (pLimbRoot->GetCollisionObject() && pLimbRoot->GetChildCount()) {
                                for (int32_t i = 0; i < pLimbRoot->GetArrayCount(); ++i) {
                                    NiAVObject* pChild = pLimbRoot->GetAt(i);
                                    if (pChild) {
                                        bhkWorld::SetDisabled(pChild, false, true, true);
                                        if (pWorld) {
                                            pWorld->AddObjects(pChild, true, false, 0, true);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void Actor::AddFactionMinorCrime(int32_t aiNum, bool bModRep) {
    TESFaction* pFactionArray[128];
    BSSimpleList<FACTION_RANK*>* pMyFactionList = &GetActorBase()->listFactions;
    ExtraFactionChanges* pMyFactionChanges = m_Extra.GetFactionChangesExtra();
    BSSimpleList<FACTION_RANK*>* pChangesList = pMyFactionChanges ? pMyFactionChanges->pFactionChanges : nullptr;
    uint32_t uiMyCount = IntegrateFactionLists(pFactionArray, 128u, pMyFactionList, pChangesList);
    for (uint32_t i = 0; i < uiMyCount; ++i) {
        if (pFactionArray[i] && pFactionArray[i]->IsTrackCrime()) {
            pFactionArray[i]->AddMinorCrime(aiNum, static_cast<uint8_t>(bModRep));
        }
    }
}

int32_t Actor::GetFactionRank(TESFaction* apFaction, bool abPC) {
    TESFaction* pMyFactionArray[128];
    int32_t iRank = -1;
    BSSimpleList<FACTION_RANK*>* pMyFactionList = &GetActorBase()->listFactions;
    ExtraFactionChanges* pMyFactionChanges = m_Extra.GetFactionChangesExtra();
    BSSimpleList<FACTION_RANK*>* pChangesList = pMyFactionChanges ? pMyFactionChanges->pFactionChanges : nullptr;
    uint32_t uiMyCount = IntegrateFactionLists(pMyFactionArray, 128u, pMyFactionList, pChangesList);
    for (int32_t i = 0; i < uiMyCount; ++i) {
        if (pMyFactionArray[i] == apFaction) {
            iRank = 1;
        }
    }
    return iRank;
}

bool Actor::HasFactionThatCaresAboutCrime() {
    TESFaction* pFactionArray[128];
    BSSimpleList<FACTION_RANK*>* pFactionList = &GetActorBase()->listFactions;
    ExtraFactionChanges* pFactionChanges = m_Extra.GetFactionChangesExtra();
    BSSimpleList<FACTION_RANK*>* pChangesList = pFactionChanges ? pFactionChanges->pFactionChanges : nullptr;
    uint32_t uiCount = IntegrateFactionLists(pFactionArray, 128u, pFactionList, pChangesList);
    for (uint32_t i = 0; i < uiCount; ++i) {
        TESFaction* pFaction = pFactionArray[i];
        if (pFaction && pFaction->IsTrackCrime()) {
            return true;
        }
    }
    return false;
}

void Actor::SetFactionRank(TESFaction* apFaction, uint8_t acRank) {
    if (apFaction) {
        TESFaction* pFactionArray[129];
        AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        BSSimpleList<FACTION_RANK*>* pMyFactionList = &GetActorBase()->listFactions;
        ExtraFactionChanges* pMyExtraList = m_Extra.GetFactionChangesExtra();
        BSSimpleList<FACTION_RANK*>* pChangesList = pMyExtraList ? pMyExtraList->pFactionChanges : nullptr;
        uint32_t uiMyCount = IntegrateFactionLists(pFactionArray, 128u, pMyFactionList, pChangesList);
        if (!pMyExtraList) {
            m_Extra.AddFactionChangesExtra();
            pMyExtraList = m_Extra.GetFactionChangesExtra();
        }
        for (int32_t i = 0;; ++i) {
            if (i >= uiMyCount) {
                pMyExtraList->AddToFaction(apFaction, static_cast<int8_t>(acRank));
                return;
            }
            if (pFactionArray[i] == apFaction) {
                break;
            }
        }
        if (static_cast<int8_t>(acRank) == -1) {
            pMyExtraList->ExpelFromFaction(apFaction);
        }
        else {
            pMyExtraList->SetFactionRank(apFaction, static_cast<int8_t>(acRank));
        }
    }
}

void Actor::ModFactionRank(TESFaction* apFaction, uint8_t acModify, bool abPC) {
    if (apFaction) {
        int32_t iCurrRank = GetFactionRank(apFaction, abPC);
        if (iCurrRank > -1) {
            int32_t iNewRank = iCurrRank + static_cast<int8_t>(acModify);
            if (iNewRank < 0) {
                iNewRank = 0;
            }
            SetFactionRank(apFaction, static_cast<uint8_t>(iNewRank));
        }
    }
}

void Actor::ProcessDamageImpacts() {
    if (GetCharController()) {
        bhkCharacterController* pCharController = GetCharController();
        for (BSSimpleList<DamageImpactData*>* pHits = pCharController->GetHurtfulHits(); pHits && !pHits->IsEmpty();
             pHits = pHits->m_pkNext) {
            DamageImpactData* pHit = pHits->m_item;
            bhkCollisionObject* pCol = nullptr;
            if (pHit->spBody.m_pObject) {
                pCol = bhkCollisionObject::Getbhk(**pHit->spBody.m_pObject);
            }
            NiAVObject* pAV = pCol ? pCol->m_pkSceneObject : nullptr;
            TESObjectREFR* pRef = pAV ? TESObjectREFR::FindReferenceFor3D(pAV) : nullptr;
            Projectile* pProjectile = static_cast<Projectile*>(pRef);
            if (pRef) {
                Projectile* pGrabObject = static_cast<Projectile*>(pPlayer->GetGrabObject());
                if (pProjectile != pGrabObject) {
                    if (pProjectile->IsProjectile()) {
                        if (pProjectile->GetProjectileType() == Projectile::PROJECTILE_TYPE_GRENADE) {
                            GrenadeProjectile* pGrenade = static_cast<GrenadeProjectile*>(pProjectile);
                            NiPoint3 kNormal = pHit->kNormal;
                            NiPoint3 kLocation = pHit->kLocation;
                            pGrenade->ReportCollision(GetCharController(), kLocation, kNormal);
                        }
                    }
                    else {
                        float fSpeed = pHit->kVelocity.Length();
                        float fMass = pHit->spBody->GetMass();
                        float fDamage = GetObjectImpactDamage(fMass, fSpeed);
                        if (fDamage > 0.0) {
                            HitData* pData = new HitData;
                            NiPointer<HitData> spData(pData);
                            HitData::InitializeImpactData(spData.m_pObject, pProjectile, this, fDamage, pHit);
                            HitMe(spData.m_pObject, 0);
                        }
                    }
                }
            }
        }
    }
}

bool Actor::UpdateRagdollAnimState(bool abConsciousState, float afDelta) {
    NiPoint3 kEyePos;
    if (!pRagdollController || !pRagdollController->QRagdollAnimInit()) {
        return false;
    }
    bool bShouldUpdate = false;
    bool bFirstPersonPlayer = this == pPlayer && !pPlayer->Is3rdPerson();
    if (abConsciousState) {
        NiAVObject* pBound3D = Get3D();
        bShouldUpdate = this != pPlayer || pPlayer->IsActually3rdPerson();
        bFootIKInRange = false;
        if (pBound3D) {
            NiPoint3 cameraPos;
            if (pPlayer->Is3rdPerson() && p3rdPersonCamera) {
                cameraPos = p3rdPersonCamera->GetWorldTranslate();
            }
            else {
                const NiPoint3* pCameraPos;
                if (p1stPersonCamera) {
                    pCameraPos = &p1stPersonCamera->GetWorldTranslate();
                }
                else {
                    float fEyeLevel = pPlayer->GetEyeLevel();
                    pCameraPos = ::new (&kEyePos)
                        NiPoint3(*pPlayer->QLocationOnReference() + fEyeLevel * NiPoint3::UNIT_Z);
                }
                cameraPos = *pCameraPos;
            }
            NiBound bound = pBound3D->GetWorldBound();
            double dCameraDist = (cameraPos - bound.GetCenter()).Length();
            float fPlayerCameraDist = dCameraDist - bound.m_fRadius * 1.25;
            bFootIKInRange = static_cast<double>(fPlayerCameraDist) < bhkRagdollController::QCameraDistance();
        }
        if (bShouldUpdate && bFootIKInRange) {
            if (!IsDead(false) &&
                (!pRagdollController->GetRagdollAnimEnable() || pRagdollController->GetEaseOutLookAtIK())) {
                bool bEnabled = pRagdollController->GetRagdollAnimEnable();
                pRagdollController->EnableRagdollAnim();
                if (!bEnabled) {
                    SetHavokWeapon();
                }
            }
        }
        else if (
            !bFirstPersonPlayer &&
            pRagdollController->GetRagdollAnimEnable() &&
            !pRagdollController->QRagdollAnimEaseOut()
        ) {
            pRagdollController->SetRagdollAnimEaseOut(true);
        }
        bShouldUpdate = bShouldUpdate && (bFootIKInRange || pRagdollController->QRagdollAnimEaseOut());
        if (bShouldUpdate) {
            uint32_t uiCurrMoveMode = GetMoveMode();
            uint32_t uiFootIKMoveModes = static_cast<ActorMover::MovementFlags>(
                ActorMover::MF_WALKING | ActorMover::MF_SNEAKING | ActorMover::MF_SLIDE
            );
            bool bJumping = false;
            Animation* pAnim = GetAnimation();
            if (pAnim && pAnim->GetCurrentSequence(AGS_MOVEMENT)) {
                if (pAnim->GetCurrentSequence(AGS_MOVEMENT)->GetAnimGroup()->IsJumpingLoopAnim()) {
                    bJumping = true;
                }
                else {
                    switch (pAnim->GetCurrentSequence(AGS_MOVEMENT)->GetAnimGroup()->GetType()) {
                        case ANIM_GROUP_JUMP_START:
                        case ANIM_GROUP_JUMP_LAND:
                        case ANIM_GROUP_JUMP_LAND_FORWARD:
                        case ANIM_GROUP_JUMP_LAND_BACKWARD:
                        case ANIM_GROUP_JUMP_LAND_LEFT:
                        case ANIM_GROUP_JUMP_LAND_RIGHT: {
                            bJumping = true;
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
            }

            bool bEnableFootIK = GetSitSleepState() == SIT_SLEEP_NORMAL &&
                !bJumping &&
                !pRagdollController->Leg3DSelectiveUpdate() &&
                !GetCannibal();
            bool bIsMoving = (uiCurrMoveMode &
                              (ActorMover::MF_FORWARD | ActorMover::MF_BACKWARD | ActorMover::MF_LEFT |
                               ActorMover::MF_RIGHT)) != 0;
            bool bIsWalking = (uiFootIKMoveModes & uiCurrMoveMode) != 0 ||
                (this == pPlayer && !pPlayer->IsActually3rdPerson());
            bool bInMovement = bIsWalking || !bIsMoving;
            pRagdollController->SetFootIKEnable(bEnableFootIK);
            if (bEnableFootIK && bFootIKInRange && bInMovement) {
                pRagdollController->SetFootIKActive(true);
            }
            else {
                pRagdollController->SetFootIKActive(false);
            }
            pRagdollController->UseFootDownCast(!bIsMoving);
            if (pVATS.eMode == VATS::VATS_PLAYBACK) {
                pRagdollController->DampenDriveGains();
            }
            else {
                pRagdollController->ResetDriveGains();
            }
            BSFaceGenAnimationData* pAnimData = GetFaceAnimationData();
            if (pAnimData) {
                NiQuaternion headRot;
                pAnimData->GetModifierValuesAsRotation(headRot);
                pRagdollController->SetHeadOffset(headRot);
            }
            else {
                pRagdollController->SetHeadOffset(NiQuaternion::IDENTITY);
            }
            SIT_SLEEP_STATE eSitSleep = pCurrentProcess->GetSitSleepState();
            if (eSitSleep == SIT_SLEEP_WANT_TO_WAKE || eSitSleep == SIT_SLEEP_WANT_TO_STAND) {
                pRagdollController->SetEaseOutLookAtIK(true);
            }
            HighProcess* pHighProc = static_cast<HighProcess*>(pCurrentProcess);
            if (pHighProc && !pHighProc->eLevel) {
                NiBSBoneLODController* pBoneLOD = pHighProc->GetBoneLOD();
                if (pBoneLOD && pBoneLOD->m_iLOD > 1) {
                    if (pRagdollController->QRagdollFeedbackActive()) {
                        pRagdollController->SetRagdollFeedbackActive(0);
                    }
                    if (pRagdollController->GetLookIKActive()) {
                        pRagdollController->SetLookIKActive(false);
                    }
                }
            }
        }
    }
    return bShouldUpdate;
}

bool Actor::HasShadow() {
    TESActorBaseData* pBaseData = static_cast<TESActorBaseData*>(GetActorBase());
    return !pBaseData->GetNoShadow() && !GetGhost();
}

bool Actor::IsJumping() {
    Animation* pAnim = GetAnimation();
    if (pAnim && pAnim->GetCurrentSequence(AGS_MOVEMENT)) {
        if (pAnim->GetCurrentSequence(AGS_MOVEMENT)->GetAnimGroup()->IsJumpingLoopAnim()) {
            return true;
        }
        switch (pAnim->GetCurrentSequence(AGS_MOVEMENT)->GetAnimGroup()->GetType()) {
            case ANIM_GROUP_JUMP_START:
            case ANIM_GROUP_JUMP_LAND:
            case ANIM_GROUP_JUMP_LAND_FORWARD:
            case ANIM_GROUP_JUMP_LAND_BACKWARD:
            case ANIM_GROUP_JUMP_LAND_LEFT:
            case ANIM_GROUP_JUMP_LAND_RIGHT: return true;
            default: break;
        }
    }
    bhkCharacterController* pCon = GetCharController();
    return pCon && pCon->GetState() == HK_CHARACTER_IN_AIR;
}

void Actor::AdjustSpeedForFollowing(float& fspeed, float afBaseRadius, float afDistToTarget) {
    GetCurrentPackage();
    TESObjectREFR* pTarg = GetCurrentPackageTarget();
    if (pTarg && pTarg->IsActor() && pTarg != this) {
        bool bSameCell;
        TESObjectCELL* pMyCell = GetParentCell();
        TESObjectCELL* pTargCell = pTarg->GetParentCell();
        GetWorldSpace();
        TESWorldSpace* pTargWorld = pTarg->GetWorldSpace();
        if ((pTargCell && pTargCell->GetInterior()) || (pMyCell && pMyCell->GetInterior())) {
            bSameCell = pTargCell == pMyCell;
        }
        else {
            bSameCell = pTargWorld == GetWorldSpace();
        }
        if (bSameCell) {
            float fMaxMult;
            float fStopZoneOuterRadius = AiFormulas::GetFollowRadiusStop(afBaseRadius);
            float fSlowdownZoneOuterRadius = AiFormulas::GetFollowRadiusSlow(afBaseRadius);
            float fMatchSpeedZoneOuterRadius = AiFormulas::GetFollowRadiusMatchSpeed(afBaseRadius);
            float fTargSpeed = 0.0f;
            if (pTarg->IsActor()) {
                fTargSpeed = static_cast<Actor*>(pTarg)->Actor::GetCurrentSpeed();
            }
            double dTargetScale = pTarg->GetScale();
            fTargSpeed = dTargetScale / GetScale() * fTargSpeed;
            int16_t sMoveMode = GetMoveMode();
            bool bRunning = (sMoveMode &
                             (ActorMover::MF_FORWARD | ActorMover::MF_BACKWARD | ActorMover::MF_LEFT |
                              ActorMover::MF_RIGHT | ActorMover::MF_RUNNING)) ==
                static_cast<ActorMover::MovementFlags>(
                                ActorMover::MF_FORWARD | ActorMover::MF_BACKWARD | ActorMover::MF_LEFT |
                                ActorMover::MF_RIGHT | ActorMover::MF_RUNNING
                );
            if ((sMoveMode &
                 (ActorMover::MF_FORWARD | ActorMover::MF_BACKWARD | ActorMover::MF_LEFT | ActorMover::MF_RIGHT |
                  ActorMover::MF_RUNNING)) ==
                static_cast<ActorMover::MovementFlags>(
                    ActorMover::MF_FORWARD | ActorMover::MF_BACKWARD | ActorMover::MF_LEFT | ActorMover::MF_RIGHT |
                    ActorMover::MF_RUNNING
                )) {
                fMaxMult = fFollowRunMaxSpeedupMultiplier.Float();
            }
            else {
                fMaxMult = fFollowWalkMaxSpeedupMultiplier.Float();
            }
            float fMaxSpeed = fspeed * fMaxMult;
            float fMinMult = bRunning ? fFollowRunMinSlowdownMultiplier.Float()
                                      : fFollowWalkMinSlowdownMultiplier.Float();
            float fMinSpeed = fspeed * fMinMult;
            if (!(fSlowdownZoneOuterRadius > static_cast<double>(afDistToTarget))) {
                if (fMatchSpeedZoneOuterRadius <= static_cast<double>(afDistToTarget) && bRunning) {
                    fspeed = fTargSpeed * 1.25;
                }
                else {
                    float fDistFromSlowDownZone = afDistToTarget - fSlowdownZoneOuterRadius;
                    float fMatchSpeedZoneWidth = fMatchSpeedZoneOuterRadius - fSlowdownZoneOuterRadius;
                    fspeed = (fDistFromSlowDownZone / fMatchSpeedZoneWidth * fFollowExtraCatchUpSpeedMult.Float() +
                              1.0) *
                        fTargSpeed;
                }
            }
            else {
                float fPastStopZone = afDistToTarget - fStopZoneOuterRadius;
                float fDistFromStopZone = BSMAX(0.0f, fPastStopZone);
                float fSlowdownZoneWidth = fSlowdownZoneOuterRadius - fStopZoneOuterRadius;
                fspeed = BSMIN(fspeed, fTargSpeed) * (fDistFromStopZone / fSlowdownZoneWidth);
            }
            BSClamp(fspeed, fMinSpeed, fMaxSpeed);
        }
    }
}

bool Actor::PutItemInReference(
    TESBoundObject* apObject,
    ExtraDataList* apExtra,
    TESObjectREFR* apRef,
    int32_t aiNumber,
    uint32_t aId
) {
    InventoryChanges* pContainerChanges = m_Extra.GetContainerChanges();
    int32_t iCount = aiNumber;
    if (!aiNumber) {
        iCount = 1;
    }
    if (pContainerChanges) {
        while (iCount) {
            ItemChange* pItem = pContainerChanges->GetInventoryItem(apObject, aId);
            if (pItem) {
                ExtraDataList* pExtraList = nullptr;
                if (pItem->pExtraObjectList) {
                    pExtraList = pItem->pExtraObjectList->m_item;
                }
                if (pExtraList) {
                    TESObjectREFR* pReference = pExtraList->GetReferencePointer();
                    if (pReference) {
                        pReference->m_Extra.SetReferencePointer(apRef);
                        pReference->AddChange(CHANGE_OBJECT_EXTRA_ITEM_DATA);
                    }
                }
                RemoveItem(apObject, pExtraList, iCount--, false, false, apRef, nullptr, nullptr, true, false);
            }
            else {
                iCount = 0;
            }
            if (pItem) {
                delete pItem;
            }
        }
        if (pContainerChanges->IsEmpty()) {
            m_Extra.RemoveContainerChanges();
        }
    }
    if (this == pPlayer) {
        Interface::UpdateInventory();
    }
    else {
        ItemChange* pBestWeapon = GetBestWeapon(COMBAT_WEAPON_TYPE_COUNT);
        if (pBestWeapon && !pBestWeapon->GetWorn(false)) {
            ExtraDataList* pExtraList = nullptr;
            if (pBestWeapon->pExtraObjectList) {
                pExtraList = pBestWeapon->pExtraObjectList->m_item;
            }
            QueueEquipObject(pBestWeapon->pContainerObj, 1, pExtraList, true, false, true);
            delete pBestWeapon;
        }
    }
    if (this != pPlayer) {
        TESIdleManager::SetUsedItem(apRef->GetObjectReference());
        TESIdleManager::SetUsedItemActivate(true);
        double dEyeLevel = GetEyeLevel();
        float fEyeZ = GetLocationOnReference().z + dEyeLevel;
        float fRefZ = apRef->GetLocationOnReference().z;
        int32_t iUsedItemLevel = apRef->IsNpc() || fEyeZ < static_cast<double>(fRefZ);
        TESIdleManager::SetUsedItemLevel(iUsedItemLevel);
        pCurrentProcess->SetupSpecialIdle(this, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, false, true);
        TESIdleManager::SetUsedItem(nullptr);
        TESIdleManager::SetUsedItemActivate(false);
        TESIdleManager::SetUsedItemLevel(-1);
    }
    return false;
}

ANIM_GROUP_ENUM Actor::Dodge(uint8_t acMoveDir) {
    Animation* pAnim = Actor::GetAnimation();
    if (!pAnim || !pCurrentProcess) {
        return ANIM_GROUP_NONE;
    }
    ANIM_GROUP_ENUM eAnimGroup = ANIM_GROUP_NONE;
    if ((acMoveDir & 1) != 0) {
        eAnimGroup = ANIM_GROUP_DODGE_FORWARD;
    }
    else if ((acMoveDir & 2) != 0) {
        eAnimGroup = ANIM_GROUP_DODGE_BACK;
    }
    else if ((acMoveDir & 4) != 0) {
        eAnimGroup = ANIM_GROUP_DODGE_LEFT;
    }
    else if ((acMoveDir & 8) != 0) {
        eAnimGroup = ANIM_GROUP_DODGE_RIGHT;
    }
    uint16_t sAnimGroup = GetAnimGroup(eAnimGroup, nullptr, false, nullptr);
    if (sAnimGroup < 11u || sAnimGroup > 14u) {
        return ANIM_GROUP_NONE;
    }
    pAnim->PlayGroup(sAnimGroup, ACTION_START, -1, AGS_NONE);
    SetAnimAction(ANIM_ACTION_DODGE, pAnim->GetCurrentSequence(AGS_MOVEMENT));
    StartAnimOn1stPerson(sAnimGroup, ACTION_START);
    uint16_t sSpeedGroup = (sAnimGroup & ~TESAnimGroup::TYPE_MASK) | ANIM_GROUP_MOVE_FORWARD;
    float fAnimSpeed = 1.0f;
    float fUnitsPerSec = GetWalkSpeed();
    if (pAnim->GetGroupSpeed(sSpeedGroup)) {
        fAnimSpeed = fUnitsPerSec / static_cast<double>(pAnim->GetGroupSpeed(sSpeedGroup)) * fAnimSpeed;
    }
    pAnim->SetMoveSpeed(fAnimSpeed);
    return eAnimGroup;
}

double Actor::GetAnimActionTime(ANIM_GROUP_ENUM aeGroup) {
    float fTime = 0.0;
    Animation* pAnim = GetAnimation();
    if (pAnim) {
        uint16_t sAnimGroup = GetAnimGroup(aeGroup, nullptr, false, pAnim);
        if (sAnimGroup) {
            TESAnimGroup* pAnimGroup = pAnim->GetTESAnimGroup(sAnimGroup);
            if (pAnimGroup) {
                return pAnimGroup->GetTime(AGA_STOP);
            }
        }
    }
    return fTime;
}

void Actor::InitiateAggroRadiusGuardPackage(Actor* apActor) {
    TESPackage* pPack = TESPackage::CreatePackage(PACKAGE_TYPE_GUARD);
    pPack->SetPackType(PACKAGE_TYPE_GUARD);
    pPack->SetMustReachLocation(false);
    pPack->SetMustComplete(false);
    PackageLocation* pLoc = new PackageLocation;
    pLoc->SetLocReference(this);
    pPack->SetPackageLocation(pLoc);
    if (pLoc) {
        delete pLoc;
    }
    PackageTarget* pNew = new PackageTarget;
    pPack->SetPackageTarget(pNew);
    pPack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_GUARD;
    pPack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
    pPack->GetPackageTarget()->SetTargReference(this);
    TESPackage* pCurrent = pCurrentProcess->GetCurrentPackage();
    if (pCurrent) {
        pPack->CopyAllFlags(pCurrent);
    }
    uint32_t uiAggroRadius = GetActorBase()->AIData.uiAggroRadius;
    pPack->GetPackageTarget()->iValue = uiAggroRadius;
    if (pNew) {
        delete pNew;
    }
    pCurrentProcess->ComputeLastTimeProcessed();
    PutCreatedPackage(pPack, true, true);
}

void Actor::InitiateAvoidPackage(TESObjectREFR* apRadiation, float afTimer) {
    EndMovement();
    TESPackage* pPack = TESPackage::CreatePackage(PACKAGE_TYPE_AVOID_RADIATION);
    pPack->SetMustReachLocation(true);
    pPack->SetMustComplete(true);
    PackageLocation* pLoc = new PackageLocation;
    pLoc->SetLocReference(this);
    pPack->SetPackageLocation(pLoc);
    if (pLoc) {
        delete pLoc;
    }
    PackageTarget* pNew = new PackageTarget;
    pPack->SetPackageTarget(pNew);
    pPack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
    pPack->GetPackageTarget()->SetTargReference(apRadiation);
    apRadiation->SetTargeted(true);
    if (pNew) {
        delete pNew;
    }
    pPack->CalculateProcedureType(nullptr);
    TESPackage* pCurrent = pCurrentProcess->GetCurrentPackage();
    if (pCurrent) {
        pPack->CopyAllFlags(pCurrent);
    }
    pCurrentProcess->SetAvoidWaitTimer(afTimer);
    PutCreatedPackage(pPack, true, true);
}

bool Actor::CanTalk() {
    TESActorBaseData* pBaseData = static_cast<TESActorBaseData*>(GetActorBase());
    return pBaseData->GetAllowPCDialogue();
}

ItemChange* Actor::GetItemBeingEscorted() {
    int32_t iNum;
    TESObjectREFR* pTarget = nullptr;
    TESBoundObject* pObj = nullptr;
    if (pCurrentProcess && pCurrentProcess->GetCurrentPackage()) {
        PackageTarget* pPackTarg = pCurrentProcess->GetCurrentPackage()->GetPackageTarget();
        if (pPackTarg) {
            switch (pPackTarg->GetTargType()) {
                case PTT_REFERENCE:
                case PTT_LINKEDREF: {
                    pTarget = pCurrentProcess->GetActorPackageThatIsRunning()->pTarget;
                    break;
                }
                case PTT_OBJECT: {
                    if (pPackTarg->GetTargObject()) {
                        TESBoundObject* pTargObject = static_cast<TESBoundObject*>(pPackTarg->GetTargObject());
                        if (pTargObject->IsBoundObject()) {
                            pObj = static_cast<TESBoundObject*>(pPackTarg->GetTargObject());
                        }
                    }
                    break;
                }
                case PTT_TYPE: {
                    if (m_Extra.GetContainerChanges()) {
                        PACKAGE_OBJECT_TYPE eTargObjectType = pPackTarg->GetTargObjectType();
                        InventoryChanges* pContainerChanges = m_Extra.GetContainerChanges();
                        pObj = pContainerChanges->GetObjectbyPackObjType(eTargObjectType, iNum);
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }

    if (pTarget) {
        uint32_t uiFormID = pTarget->iFormID;
        return GetInventoryItem(pTarget->GetObjectReference(), uiFormID);
    }

    if (pObj) {
        return GetInventoryItem(pObj, 0);
    }

    return nullptr;
}

float Actor::ComputeTargetDesirability(TESObjectREFR* apRef) {
    void* pHighProcess = nullptr;
    Actor* pTargetActor = nullptr;
    if (GetCurrentAIProcess()) {
        if (!GetCurrentAIProcess()->GetProcessLevel()) {
            pHighProcess = GetCurrentAIProcess();
        }
    }
    if (pHighProcess) {
        if (apRef->IsActor()) {
            pTargetActor = static_cast<Actor*>(apRef);
        }
        float fDistanceToActor = apRef->GetDistanceFromReference(this, false, false);
        bool bLineOfSight = true;
        if (pTargetActor) {
            bLineOfSight = LineOfSight(false, pTargetActor, true, nullptr, false);
        }
        float fDesirability = (1000.0 - fDistanceToActor) / (1000.0 - fAIBestHeadTrackDistance.Float());
        if (!bLineOfSight) {
            fDesirability = fDesirability * 0.5;
        }
        if (apRef == GetCurrentAIProcess()->GetCurrentHeadTrackTarget()) {
            fDesirability = (fHeadTrackTimer + 1.0) * fDesirability;
        }
        if (apRef->IsDead(false)) {
            return fDesirability * 0.5;
        }
        return fDesirability;
    }

    return 0.0;
}

bool Actor::CanHeadTrack(TESObjectREFR* pRef, bool abCheckLineOfSight) {
    bool b360LineOfSight;
    bool bAreAllCombatTargetsSearching;
    if (this == pRef || !pRef->IsActor() || pRef->GetDisabled() || !pRef->Get3D()) {
        return false;
    }
    NiPoint3 kDelta = *pRef->QLocationOnReference() - *QLocationOnReference();
    float fDistance = pRef->GetDistanceFromReference(this, false, true);
    pCurrentProcess->GetCurrentHeadTrackTarget();
    float fMaxDistance = fAIMaxHeadTrackDistance.Float();
    if (fMaxDistance < static_cast<double>(fDistance)) {
        return false;
    }
    if (fDistance < 100.0 && pRef != pPlayer && pRef != pCurrentProcess->GetLastHeadTrackTarget()) {
        return false;
    }
    float fZAngle = GetZAngleFromVector(kDelta);
    float fHeading = GetAngleOnReference().z;
    float fAngleDiff = fZAngle - fHeading;
    if (fAngleDiff > 3.1415927410125732) {
        fAngleDiff = fAngleDiff - 6.2831854820251465;
    }
    if (fAngleDiff < -3.1415927410125732) {
        fAngleDiff = fAngleDiff + 6.2831854820251465;
    }
    float fMaxAngle = pRef == pCurrentProcess->GetCurrentHeadTrackTarget() ? 3.1415927f : 2.0943952f;
    double dAbsAngle = NiAbs(fAngleDiff);
    if (fMaxAngle < dAbsAngle) {
        return false;
    }
    Actor* pToDetect = static_cast<Actor*>(pRef);
    if (this != pPlayer && pToDetect != pPlayer && GetSitSleepState() && pToDetect->GetSitSleepState()) {
        return false;
    }
    if (abCheckLineOfSight) {
        bool bLineOfSight = true;
        b360LineOfSight = false;
        bAreAllCombatTargetsSearching = false;
        bool bInCombat;
        if (pToDetect == pPlayer) {
            bInCombat = pPlayer->IsPlayerCharacterInCombat(&bAreAllCombatTargetsSearching);
        }
        else {
            bInCombat = pToDetect->IsInCombat();
        }
        int32_t iDetectionLevel = GetDetectionLevelAgainstActor(
            false,
            pToDetect,
            bLineOfSight,
            bInCombat,
            false,
            bInCombat,
            &b360LineOfSight
        );
        if (iDetectionLevel <= 0 ||
            (pRef == pPlayer && pRef == pCurrentProcess->GetCurrentHeadTrackTarget() && !b360LineOfSight) ||
            (IsMoving() && !bLineOfSight)) {
            return false;
        }
    }
    return true;
}

void Actor::ForceIdleAnimation() {
    if (GetSitSleepState() == SIT_SLEEP_NORMAL) {
        bool bClearAnimAction = false;
        BSAnimGroupSequence* pAnimActionSeq = pCurrentProcess->GetAnimActionAnimSeq();
        Animation* pAnim = Actor::GetAnimation();
        if (pAnim) {
            if (pAnimActionSeq) {
                bClearAnimAction = pAnimActionSeq == pAnim->GetCurrentSequence(AGS_IDLE);
            }
            int32_t iIdleGroup = pAnim->GetGroup(AGS_IDLE);
            if (iIdleGroup != GetAnimGroup(ANIM_GROUP_IDLE, nullptr, false, nullptr)) {
                pAnim->ClearGroup(AGS_IDLE, 0.0f);
            }
            if (pAnimActionSeq && pAnimActionSeq == pAnim->GetCurrentSequence(AGS_MOVEMENT)) {
                bClearAnimAction = true;
            }

            pAnim->ClearGroup(AGS_MOVEMENT, 0.0f);
            ANIM_GROUP_ENUM eLeftArm = TESAnimGroup::GetType(pAnim->GetGroup(AGS_LEFT_ARM));
            if (eLeftArm < ANIM_GROUP_HAND_GRIP_1 || eLeftArm > ANIM_GROUP_HAND_GRIP_6) {
                if (pAnimActionSeq && pAnimActionSeq == pAnim->GetCurrentSequence(AGS_LEFT_ARM)) {
                    bClearAnimAction = true;
                }

                pAnim->ClearGroup(AGS_LEFT_ARM, 0.0f);
            }
            if (pAnimActionSeq && pAnimActionSeq == pAnim->GetCurrentSequence(AGS_TALKING)) {
                bClearAnimAction = true;
            }

            pAnim->ClearGroup(AGS_TALKING, 0.0f);
            if (pAnimActionSeq && pAnimActionSeq == pAnim->GetCurrentSequence(AGS_SPECIAL_IDLE)) {
                bClearAnimAction = true;
            }

            pAnim->ClearGroup(AGS_SPECIAL_IDLE, 0.0f);
            if (!pAnim->GetCurrentSequence(AGS_IDLE)) {
                pAnim->PlayGroup(GetAnimGroup(ANIM_GROUP_IDLE, nullptr, false, nullptr), ACTION_NORMAL, -1, AGS_NONE);
            }
        }
        if (bClearAnimAction) {
            SetAnimAction(ANIM_ACTION_NONE, nullptr);
        }
        if (this == pPlayer) {
            pAnim = pPlayer->GetAnimation(true);
            if (pAnim) {
                int32_t iIdleGroup = pAnim->GetGroup(AGS_IDLE);
                if (iIdleGroup != GetAnimGroup(ANIM_GROUP_IDLE, nullptr, false, nullptr)) {
                    pAnim->ClearGroup(AGS_IDLE, 0.0f);
                }
                pAnim->ClearGroup(AGS_MOVEMENT, 0.0f);
                ANIM_GROUP_ENUM eLeftArm = TESAnimGroup::GetType(pAnim->GetGroup(AGS_LEFT_ARM));
                if (eLeftArm < ANIM_GROUP_HAND_GRIP_1 || eLeftArm > ANIM_GROUP_HAND_GRIP_6) {
                    pAnim->ClearGroup(AGS_LEFT_ARM, 0.0f);
                }
                pAnim->ClearGroup(AGS_TALKING, 0.0f);
                pAnim->ClearGroup(AGS_SPECIAL_IDLE, 0.0f);
                if (!pAnim->GetCurrentSequence(AGS_IDLE)) {
                    pAnim->PlayGroup(
                        GetAnimGroup(ANIM_GROUP_IDLE, nullptr, false, nullptr),
                        ACTION_NORMAL,
                        -1,
                        AGS_NONE
                    );
                }
            }
        }
    }
}

bool Actor::IsInCombatantFaction() {
    bool bFlag = false;
    BSSimpleList<FACTION_RANK*>* pList = &GetActorBase()->listFactions;
    ExtraFactionChanges* pExtra = m_Extra.GetFactionChangesExtra();
    BSSimpleList<FACTION_RANK*>* pExtraFactionList = pExtra ? pExtra->pFactionChanges : nullptr;
    while (pList && !bFlag) {
        FACTION_RANK* pRank = pList->m_item;
        if (pRank && (!pExtra || !pExtra->GetIsExpelled(pRank->pFaction)) && pRank->pFaction->GetSpecialCombat()) {
            bFlag = true;
        }

        pList = pList->m_pkNext;
    }
    while (pExtraFactionList && pExtraFactionList->m_item && !bFlag) {
        FACTION_RANK* pRank = pExtraFactionList->m_item;
        if (pRank && (!pExtra || !pExtra->GetIsExpelled(pRank->pFaction)) && pRank->pFaction->GetSpecialCombat()) {
            bFlag = true;
        }

        pExtraFactionList = pExtraFactionList->m_pkNext;
    }
    return bFlag;
}

void Actor::ClearDeathState() {
    NiNode* pRootNode = TESObjectREFR::Get3D();
    if (pRootNode) {
        bhkBlendController::DoGetUp(pRootNode, 1.0);
        BSFaceGenAnimationData* pFaceAnimData = GetFaceAnimationData();
        if (pFaceAnimData) {
            pFaceAnimData->SetDead(false, false);
        }
        ReloadTargets(true);
        bhkCharacterController* pCharController = GetCharController();
        if (pCharController && GetParentCell()) {
            pCharController->SetWorld(GetParentCell()->GetbhkWorld());
        }

        if (pRagdollController) {
            pRagdollController->EnableRagdollAnim();
        }
        Animation* pAnim = GetAnimation();
        if (pAnim) {
            pAnim->UpdateSceneGraphNoController();
            if (this == pPlayer) {
                bool b3rdPerson = pPlayer->Is3rdPerson();
                pPlayer->GetAnimation(b3rdPerson)->UpdateSceneGraphNoController();
            }
        }
        // Passes the virtual Get3D's result straight on and culls the root node it read first.
        ACTOR_CRITICAL_STAGE eCriticalStage = this->eCriticalStage;
        switch (eCriticalStage) {
            case ACS_GOO_START: {
                bhkUtilFunctions::RemoveSpringConstraints(Get3D());
                break;
            }
            case ACS_GOO_END: {
                InitHavok();
                bhkUtilFunctions::RemoveSpringConstraints(Get3D());
                pRootNode->SetAppCulled(false);
                break;
            }
            case ACS_DISINTEGRATE_END: {
                InitHavok();
                pRootNode->SetAppCulled(false);
                break;
            }
        }
    }
}

bool Actor::CanSkipActorUpdate() {
    if (IsQuestObject()) {
        return false;
    }
    TESPackage* pPackage = GetCurrentAIProcess()->GetCurrentPackage();
    if (pPackage) {
        if (pPackage->GetPackType() != PACKAGE_TYPE_WANDER || !GetCurrentAIProcess()->GetProcedureIndexRunning()) {
            if (pPackage->GetPackType() != PACKAGE_TYPE_TRAVEL) {
                return false;
            }
            if (!GetCurrentAIProcess()->GetProcedureIndexRunning()) {
                return false;
            }
        }
    }
    if (GetCurrentAIProcess()->GetForceNextUpdate()) {
        GetCurrentAIProcess()->SetForceNextUpdate(false);
        return false;
    }
    else {
        BSFadeNode* pFadeNode = static_cast<BSFadeNode*>(Get3D());
        if (pFadeNode && pFadeNode->IsFadeNode()) {
            if (!pFadeNode->ShouldUpdate(Main::WorldRootCamera(), 0.8f)) {
                return true;
            }
        }
    }
    return false;
}

void Actor::StartWeaponGroup(ANIM_GROUP_ENUM aeBaseAnimGroup, Animation* apAnim) {
    int32_t iCount = 3;
    uint16_t sBaseWeaponAnimGroup = 255;
    bool bAttackOrAim = TESAnimGroup::IsAttackAction(aeBaseAnimGroup) || TESAnimGroup::IsAimAction(aeBaseAnimGroup);
    if (!bAttackOrAim) {
        iCount = 1;
    }
    for (int32_t n = 0; n < iCount; ++n) {
        uint16_t sAimAnimGroup = 255;
        TESObjectWEAP* pWeapon = nullptr;
        if (pCurrentProcess->GetCurrentWeapon()) {
            pWeapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
        }
        if (aeBaseAnimGroup != ANIM_GROUP_NONE) {
            ANIM_GROUP_ENUM eBaseAnimGroup;
            if (bAttackOrAim && GetIronSights()) {
                eBaseAnimGroup = static_cast<ANIM_GROUP_ENUM>(
                    n + GetIronSightsAttack(apAnim, aeBaseAnimGroup, pWeapon)
                );
            }
            else {
                eBaseAnimGroup = static_cast<ANIM_GROUP_ENUM>(n + aeBaseAnimGroup);
            }
            if (n) {
                if (sBaseWeaponAnimGroup != 255) {
                    sAimAnimGroup = apAnim->PickBestAnimation(++sBaseWeaponAnimGroup, false);
                }
            }
            else {
                sAimAnimGroup = GetAnimGroup(eBaseAnimGroup, nullptr, false, apAnim);
                sBaseWeaponAnimGroup = sAimAnimGroup;
                if (bAttackOrAim &&
                    TESAnimGroup::IsIronSightsAction(eBaseAnimGroup) &&
                    TESAnimGroup::GetType(sAimAnimGroup) == eBaseAnimGroup - 3) {
                    eBaseAnimGroup = static_cast<ANIM_GROUP_ENUM>(eBaseAnimGroup - 3);
                    aeBaseAnimGroup = static_cast<ANIM_GROUP_ENUM>(aeBaseAnimGroup - 3);
                }
            }
            if (TESAnimGroup::GetType(sAimAnimGroup) != eBaseAnimGroup) {
                sAimAnimGroup = 255;
            }
        }
        if (sAimAnimGroup == 255) {
            apAnim->ClearGroup(static_cast<ANIM_GROUP_SECTION>(n + 4), 0.0f);
            if (apAnim != pPlayer->GetAnimation(true)) {
                pCurrentProcess->SetAimSequence(static_cast<AIM_ENUM>(n), nullptr);
            }
        }
        else {
            BSAnimGroupSequence* pAimSeq = apAnim->PlayGroup(sAimAnimGroup, ACTION_START, -1, AGS_NONE);
            if (apAnim != pPlayer->GetAnimation(true)) {
                pCurrentProcess->SetAimSequence(static_cast<AIM_ENUM>(n), pAimSeq);
            }
        }
    }
    pCurrentProcess->UpdateAimSequences(this);
}

bool Actor::SetPathfindingGoalAndAngle_0(
    const NiPoint3& aGoalLocation,
    TESObjectCELL* apCell,
    TESWorldSpace* apWorldSpace,
    float afAngleToFace,
    float afTargetRadius,
    PathingAvoidNodeArray* apAvoidNodes
) {
    if (!apAvoidNodes) {
        apAvoidNodes = pCurrentProcess->CreateAvoidArray(this);
    }
    return pActorMover
        ->SetGoalAndAngle(aGoalLocation, apCell, apWorldSpace, afAngleToFace, afTargetRadius, apAvoidNodes);
}

bool Actor::GetCurrentPathfindingGoal(NiPoint3* apRetLocation) {
    return pActorMover->GetCurrentGoal(apRetLocation);
}

void Actor::BuildRequest_0(
    PathingRequest* apRequest,
    const NiPoint3& aGoalLocation,
    TESForm* apSpace,
    float afTargetRadius,
    PathingAvoidNodeArray* apAvoidNodes
) {
    TESObjectCELL* pCell = nullptr;
    TESWorldSpace* pWorld = nullptr;
    if (apSpace) {
        if (apSpace->GetFormType() == CELL_ID) {
            pCell = static_cast<TESObjectCELL*>(apSpace);
        }
        else if (apSpace->GetFormType() == WRLD_ID) {
            pWorld = static_cast<TESWorldSpace*>(apSpace);
        }
    }
    else {
        pCell = GetParentCell();
        pWorld = GetWorldSpace();
    }
    BuildRequest(apRequest, aGoalLocation, pCell, pWorld, afTargetRadius, apAvoidNodes);
}

void Actor::Dismember(
    HitData* apData,
    ActorValue::Index aeDismemberLocation,
    BGSBodyPart::LIMB_ENUM aeLimb,
    TESForm* apDeathObject,
    CAUSE_OF_DEATH aeDeath,
    int32_t aiDismemberBonus,
    int32_t aiExplodeBonus,
    bool abTorsoExplodes
) {
    TESCreature* pCreature = dynamic_cast<TESCreature*>(GetActorBase());
    if (((!pCreature || !pCreature->IsHumanoidCreature()) && !IsNpc()) || !bDisableAllGore.Bool()) {
        if (aeDismemberLocation >= ActorValue::PERCEPTION_CONDITION &&
            aeDismemberLocation <= ActorValue::BRAIN_CONDITION) {
            if (!GetDismembered(aeLimb) && !GetEssential() && !IsChild(false)) {
                int32_t iExplodeChance = 0;
                bool bAlwaysExplode = false;
                bool bExplodeAbsolute = false;
                BGSBodyPartData* pBodyPartData = GetActorBase()->GetBodyPartData();
                if (pBodyPartData) {
                    BGSBodyPart* pBodyPart = pBodyPartData->GetBodyPart(aeLimb);
                    if (pBodyPart) {
                        bExplodeAbsolute = pBodyPart->IsExplodeChanceAbsolute();
                        if (pBodyPart->IsExplodablePart()) {
                            iExplodeChance = pBodyPart->GetExplosionChance();
                        }
                        if (bExplodeAbsolute && iExplodeChance >= 100) {
                            bAlwaysExplode = true;
                        }
                    }
                }
                bool bOtherLimbHit = apData && apData->eDamageLimb != aeLimb;
                bool bDismember = false;
                bool bExplode = bAlwaysExplode;
                if (apData) {
                    if (apData->IsFlagSet(HitData::HD_EXPLODE_LIMB)) {
                        bExplode = true;
                    }
                    else {
                        bDismember = apData->IsFlagSet(HitData::HD_DISMEMBER_LIMB);
                    }
                }
                if (!bDismember && !bExplode) {
                    TESObjectWEAP* pWeapon;
                    if (bOtherLimbHit) {
                        pWeapon = nullptr;
                    }
                    else {
                        TESForm* pWeaponForm;
                        if (apDeathObject && apDeathObject->GetFormType() == WEAP_ID) {
                            pWeaponForm = apDeathObject;
                        }
                        else if (apData && apData->pWeapon) {
                            pWeaponForm = apData->pWeapon;
                        }
                        else {
                            pWeaponForm = apData && apData->eAttackSkill == ActorValue::UNARMED ? pFists : nullptr;
                        }
                        pWeapon = static_cast<TESObjectWEAP*>(pWeaponForm);
                    }
                    if (this) {
                        CombatFormulas::CheckDismemberOrExplode(
                            this,
                            pWeapon,
                            iExplodeChance,
                            aiDismemberBonus,
                            aiExplodeBonus,
                            bExplodeAbsolute,
                            bDismember,
                            bExplode
                        );
                    }
                    else {
                        CombatFormulas::CheckDismemberOrExplode(
                            nullptr,
                            pWeapon,
                            iExplodeChance,
                            aiDismemberBonus,
                            aiExplodeBonus,
                            bExplodeAbsolute,
                            bDismember,
                            bExplode
                        );
                    }
                }
                if (bDismember || bExplode) {
                    for (BGSBodyPart::LIMB_ENUM eLimbIndex = BGSBodyPart::LIMB_TORSO;
                         eLimbIndex < BGSBodyPart::LIMB_COUNT;
                         eLimbIndex = static_cast<BGSBodyPart::LIMB_ENUM>(eLimbIndex + 1)) {
                        if (bDismemberOneLimb.Bool() && eLimbIndex != BGSBodyPart::LIMB_NONE) {
                            eLimbIndex = aeLimb;
                        }
                        pBodyPartData = GetActorBase()->GetBodyPartData();
                        BGSBodyPart* pBodyPart = pBodyPartData ? pBodyPartData->GetBodyPart(eLimbIndex) : nullptr;
                        if (pBodyPart && pBodyPart->GetActorValue() == aeDismemberLocation) {
                            NiNode* pLimbRoot = pCurrentProcess->GetDamageNode(eLimbIndex);
                            if (pLimbRoot && !pLimbRoot->GetAppCulled()) {
                                if (bExplode && !pBodyPart->IsExplodablePart()) {
                                    bExplode = false;
                                }
                                if (pBodyPart->IsSeverablePart() || pBodyPart->IsExplodablePart()) {
                                    SetDismembered(eLimbIndex, apDeathObject, aeDeath, eLimbIndex, bExplode);
                                    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                                        bool bQueueExplode = bExplode;
                                        TaskQueueInterface::GetInstance()->QueueDismemberLimb(
                                            this,
                                            aeDismemberLocation,
                                            eLimbIndex,
                                            bQueueExplode,
                                            !abTorsoExplodes
                                        );
                                    }
                                    else {
                                        DismemberLimb(
                                            aeDismemberLocation,
                                            eLimbIndex,
                                            bExplode,
                                            !abTorsoExplodes,
                                            true,
                                            false
                                        );
                                    }
                                    if (bDismemberOneLimb.Bool() && eLimbIndex != BGSBodyPart::LIMB_NONE) {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else {
            BGSBodyPart::LIMB_ENUM eLimbIndex = apData ? apData->eDamageLimb : aeLimb;
            SetDismembered(BGSBodyPart::LIMB_NONE, apDeathObject, aeDeath, eLimbIndex, false);
        }
    }
}

void Actor::ScriptDismember(CAUSE_OF_DEATH aeCause, BGSBodyPart::LIMB_ENUM aeLimb, Actor* apKiller) {
    if (Get3D()) {
        BGSBodyPartData* pBodyPartData = GetActorBase()->GetBodyPartData();
        BGSBodyPart* pBodyPart = nullptr;
        ActorValue::Index eLimbCondition = ActorValue::VALUE_NONE;
        if (pBodyPartData) {
            pBodyPart = pBodyPartData->GetBodyPart(aeLimb);
        }
        if (pBodyPart) {
            eLimbCondition = static_cast<ActorValue::Index>(pBodyPart->GetActorValue());
            float fValue = static_cast<ActorValueOwner*>(this)->GetActorFloatValue(eLimbCondition);
            float fDamage = fValue + fValue;
            DamageModActorValue(eLimbCondition, fDamage, apKiller);
        }
        Dismember(nullptr, eLimbCondition, aeLimb, nullptr, aeCause, 0, 1000, aeLimb == BGSBodyPart::LIMB_TORSO);
    }
}

void Actor::HandleMajorCrimeFactionReputations(int32_t aiNum, char acModRep) {
    TESFaction* pFactionArray[129];
    BSSimpleList<FACTION_RANK*>* pFactionList = &GetActorBase()->listFactions;
    ExtraFactionChanges* pFactionChanges = m_Extra.GetFactionChangesExtra();
    BSSimpleList<FACTION_RANK*>* pChangesList = pFactionChanges ? pFactionChanges->pFactionChanges : nullptr;
    uint32_t uiCount = IntegrateFactionLists(pFactionArray, 128u, pFactionList, pChangesList);
    for (uint32_t i = 0; i < uiCount; ++i) {
        if (pFactionArray[i] && pFactionArray[i]->IsTrackCrime()) {
            pFactionArray[i]->HandleMajorCrimeReputation(aiNum, acModRep);
        }
    }
}

bool Actor::SetFactionsThatCareAboutCrime(Actor* apActor, Crime* apCrime) {
    TESFaction* pFactionArray[129];
    bool bFound = false;
    BSSimpleList<FACTION_RANK*>* pFactionList = &GetActorBase()->listFactions;
    ExtraFactionChanges* pFactionChanges = m_Extra.GetFactionChangesExtra();
    BSSimpleList<FACTION_RANK*>* pChangesList = pFactionChanges ? pFactionChanges->pFactionChanges : nullptr;
    uint32_t uiCount = IntegrateFactionLists(pFactionArray, 128u, pFactionList, pChangesList);
    for (uint32_t i = 0; i < uiCount; ++i) {
        TESFaction* pFaction = pFactionArray[i];
        if (pFaction && pFaction->IsTrackCrime() && apActor->IsInFaction(pFaction)) {
            pFaction->SetPCAnEnemy(true);
            apCrime->AddEnemyFactions(pFaction);
            bFound = true;
        }
    }
    return bFound;
}

void Actor::InitiateUseWeaponPackage(
    TESBoundObject* apWeap,
    TESObjectREFR* apLocRef,
    TESObjectREFR* apTargRef,
    int32_t aiAttackTimes,
    TESObjectREFR* apTargetLocRef,
    bool abCrouch,
    bool abHoldFire,
    bool abAlwaysHit,
    bool abDoNoDamage
) {
    if (pCurrentProcess) {
        TESPackage* pPack = TESPackage::CreatePackage(PACKAGE_TYPE_USE_WEAPON);
        pPack->SetPackType(PACKAGE_TYPE_USE_WEAPON);
        pPack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_USEWEAPON;
        pPack->SetMustReachLocation(true);
        PackageLocation* pLoc = new PackageLocation;
        pLoc->SetLocReference(apLocRef);
        pPack->SetPackageLocation(pLoc);
        if (pLoc) {
            delete pLoc;
        }
        PackageTarget* pNew = new PackageTarget;
        pNew->SetTargType(PTT_OBJECT);
        pNew->SetTargObject(apWeap);
        pPack->SetPackageTarget(pNew);
        if (pNew) {
            delete pNew;
        }
        PackageTarget attackTarget;
        attackTarget.SetTargType(PTT_REFERENCE);
        attackTarget.SetTargReference(apTargRef);
        TESPackage* pEditorPack = GetCurrentEditorPackage();
        if (pEditorPack) {
            pPack->CopyAllFlags(pEditorPack);
        }
        if (apTargetLocRef) {
            PackageLocation* pSecondLoc = new PackageLocation;
            pSecondLoc->SetLocReference(apTargetLocRef);
            pPack->SetPackageSecondLocation(pSecondLoc);
            if (pSecondLoc) {
                delete pSecondLoc;
            }
        }
        TESUseWeaponPackageData* pData = pPack->GetOrCreateUseWeaponPackageData();
        pData->bCrouch = abCrouch;
        pData->bHoldFire = abHoldFire;
        pData->bAlwaysHit = abAlwaysHit;
        pData->bDoNoDamage = abDoNoDamage;
        pData->bVolleyFire = true;
        pData->bRepeatFire = false;
        pData->sBurstCount = 1;
        pData->SetAttackTarget(&attackTarget);
        pData->sVolleyShotsMin = aiAttackTimes;
        pData->sVolleyShotsMax = aiAttackTimes;
        pCurrentProcess->ComputeLastTimeProcessed();
        PutCreatedPackage(pPack, true, true);
    }
}

void Actor::AddBackCrime(Crime* apCrime) {
    float fTimeStamp = apCrime->GetTimeStamp().fTimeStamp;
    m_Extra.AddToPlayerCrimeList(apCrime, fTimeStamp);
    for (BSSimpleList<TESFaction*>* pFactList = apCrime->pListofEnemyFactions; pFactList && !pFactList->IsEmpty();
         pFactList = pFactList->m_pkNext) {
        TESFaction* pFac = pFactList->m_item;
        pFac->SetPCAnEnemy(true);
    }
    ReferenceProcessLists.AddtoGlobalCrimeList(apCrime);
}

bool Actor::ShouldActorAvoidRadiation() {
    if (IsInCombat()) {
        return false;
    }
    double dRadResist = GetActorFloatValue(ActorValue::RADIATION_RESISTANCE);
    if (fRadiationResistanceAvoidLevel.Float() < dRadResist) {
        return false;
    }
    TESPackage* pPack = GetCurrentPackage();
    if (pPack && pPack->GetSelectiveFalloutBehavior()) {
        return pPack->GetFOEnableAvoidRadiationBehavior();
    }

    return true;
}

void Actor::InitiateAlarm(Crime* apCrime, int32_t aiProcedureIndex, bool abSendAlarm, Actor* apActor) {
    if (!pCurrentProcess ||
        !pCurrentProcess->GetCurrentPackage() ||
        pCurrentProcess->GetCurrentPackage()->GetPackType() != PACKAGE_TYPE_ALARM) {
        if (IsActoraGuard()) {
            if (apCrime && !apActor) {
                apActor = apCrime->pcriminal;
            }

            BSSimpleList<TESObjectREFR*>* pListChase = ReferenceProcessLists.IsActorTargetingREFinPackage(
                apActor,
                PACKAGE_TYPE_ALARM,
                false
            );
            if (pListChase) {
                apActor->pCurrentProcess->SetNumberGuardsArresting(pListChase->ItemsInList());
            }
            else {
                apActor->pCurrentProcess->SetNumberGuardsArresting(0);
            }
            if (apActor) {
                int32_t iGuardsArresting = apActor->pCurrentProcess->GetNumberGuardsArresting();
                if (iGuardsArresting >= iNumberGuardsCrimeResponse.Int()) {
                    return;
                }
                if (apActor == pPlayer && pPlayer->IsPlayerResistingArrest()) {
                    pCurrentProcess->EnterCombat(
                        this,
                        apActor,
                        true,
                        false,
                        nullptr,
                        false,
                        false,
                        false,
                        0,
                        false,
                        false,
                        true,
                        nullptr
                    );
                    return;
                }
                apActor->pCurrentProcess->ModNumberGuardsArresting(1);
            }
        }
        if (GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
            InitiateGetUpPackage();
        }
        AlarmPackage* pAlarm = nullptr;
        TESPackage* pPack = GetPackageSetAsPcurrent();
        if (pPack && pPack->IsInterruptPackage()) {
            pPack = m_Extra.GetPackageExtra();
        }

        if ((IsInCombat() ||
             IsDead(false) ||
             IsFleeing(false) ||
             (pPack && pPack->GetDefensive() && apCrime && apCrime->pcrimetarget != pCurrentProcess->GetTarget())) &&
            abSendAlarm) {
            if (!IsDead(false) && !apCrime->breported) {
                pCurrentProcess->CallforHelp(this, apCrime->pcriminal, apCrime);
            }
        }
        else {
            pCurrentProcess->ClearRunOncePackage();
            if (GetCurrentPackage()) {
                if (GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_ALARM) {
                    pAlarm = static_cast<AlarmPackage*>(GetCurrentPackage());
                }
                else if (GetCurrentPackage()->IsInterruptPackage()) {
                    EndInterruptPackage(false);
                }
            }
            if (apCrime) {
                if (GetClass()->IsGuard()) {
                    apCrime->bguardrespond = true;
                    if (apCrime->pcriminal->IsNpc()) {
                        if (static_cast<PlayerCharacter*>(apCrime->pcriminal) != pPlayer &&
                            !apCrime->pcriminal->IsInCombat() &&
                            apCrime->pcriminal->GetRefPersists()) {
                            Actor* pCrim = apCrime->pcriminal;
                            if (!pCrim->IsFleeing(false)) {
                                pCrim->InitiateFlee(this, false, true, true, nullptr, nullptr, -1.0f, -1.0f);
                            }
                            FleePackage* pFleePack = dynamic_cast<FleePackage*>(pCrim->GetCurrentPackage());
                            if (pFleePack) {
                                pFleePack->AddAvoidedRef(this);
                            }
                        }
                    }
                }
            }
            if (pAlarm) {
                if (apCrime) {
                    Actor* pTarget = static_cast<Actor*>(pCurrentProcess->GetTarget());
                    if (pTarget == apCrime->pcriminal && !pAlarm->IsCrimeInList(apCrime)) {
                        pAlarm->AddCrime(apCrime);
                    }
                }
            }
            else {
                TESPackage* pTempPack = pCurrentProcess->GetCurrentPackage();
                TESPackage* pAlarmPack = new AlarmPackage(apCrime);
                pAlarmPack->SetPackType(PACKAGE_TYPE_ALARM);
                if (pTempPack) {
                    bool bDrawWeapon = pTempPack->GetDrawWeaponFlag();
                    pAlarmPack->SetDrawWeaponFlag(bDrawWeapon);
                    bool bForceUnequipWeapon = pTempPack->GetForceUnequipWeapon();
                    pAlarmPack->SetForceUnequipWeapon(bForceUnequipWeapon);
                }
                pAlarmPack->SetMustReachLocation(true);
                pAlarmPack->SetMustComplete(true);
                PackageLocation* pLoc = new PackageLocation;
                if (apCrime) {
                    pLoc->SetLocReference(apCrime->pcriminal);
                }
                else {
                    pLoc->SetLocReference(apActor);
                }
                pAlarmPack->SetPackageLocation(pLoc);
                if (pLoc) {
                    delete pLoc;
                }
                PackageTarget* pNew = new PackageTarget;
                pAlarmPack->SetPackageTarget(pNew);
                if (pNew) {
                    delete pNew;
                }
                pAlarmPack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_ALARM;
                pAlarmPack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
                if (apCrime) {
                    pAlarmPack->GetPackageTarget()->SetTargReference(apCrime->pcriminal);
                }
                else {
                    pAlarmPack->GetPackageTarget()->SetTargReference(apActor);
                }
                pCurrentProcess->ComputeLastTimeProcessed();
                pCurrentProcess->SavePackageToExtraData(this);
                PutCreatedPackage(pAlarmPack, false, true);
                if (apCrime) {
                    apCrime->AddtoActorKnowList(this);
                }
                pCurrentProcess->SetProcedureIndexRunning(aiProcedureIndex);
            }
        }
    }
}

void Actor::InitiatePickPocketWarning(Actor* apActor) {
    if (!IsInCombat()) {
        pCurrentProcess->ClearRunOncePackage();
        pCurrentProcess->ClearActionHeadTrackTarget(true);
        TESPackage* pTempPack = pCurrentProcess->GetCurrentPackage();
        if (GetCurrentPackage()) {
            if (GetCurrentPackage()->IsInterruptPackage()) {
                EndDialogue();
            }
        }
        TESPackage* pPack = TESPackage::CreatePackage(PACKAGE_TYPE_PICKPOCKET_WARNING);
        pPack->SetPackType(PACKAGE_TYPE_PICKPOCKET_WARNING);
        pPack->SetMustReachLocation(true);
        pPack->SetMustComplete(true);
        PackageLocation* pLoc = new PackageLocation;
        pLoc->SetLocReference(apActor);
        pPack->SetPackageLocation(pLoc);
        if (pLoc) {
            delete pLoc;
        }
        PackageTarget* pNew = new PackageTarget;
        pPack->SetPackageTarget(pNew);
        if (pNew) {
            delete pNew;
        }
        pPack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_PICKPOCKET_WARN;
        pPack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
        pPack->GetPackageTarget()->SetTargReference(apActor);
        pPack->GetPackageTarget()->iValue = 0;
        if (pTempPack) {
            bool bForceUnequipWeapon = pTempPack->GetForceUnequipWeapon();
            pPack->SetForceUnequipWeapon(bForceUnequipWeapon);
        }
        pCurrentProcess->ComputeLastTimeProcessed();
        pCurrentProcess->SavePackageToExtraData(this);
        PutCreatedPackage(pPack, false, true);
        pPlayer->IssuePickPocketWarning();
    }
}

void Actor::InitiateStealWarning(Actor* apActor, TESForm* apOwner) {
    if (!IsInCombat()) {
        pCurrentProcess->ClearRunOncePackage();
        pCurrentProcess->ClearActionHeadTrackTarget(true);
        pCurrentProcess->GetCurrentPackage();
        if (GetCurrentPackage()) {
            if (GetCurrentPackage()->IsInterruptPackage()) {
                EndDialogue();
            }
        }
        TESPackage* pPack = TESPackage::CreatePackage(PACKAGE_TYPE_STEAL_WARNING);
        pPack->SetPackType(PACKAGE_TYPE_STEAL_WARNING);
        pPack->SetMustReachLocation(true);
        pPack->SetMustComplete(true);
        PackageLocation* pLoc = new PackageLocation;
        pLoc->SetLocReference(apActor);
        pPack->SetPackageLocation(pLoc);
        if (pLoc) {
            delete pLoc;
        }
        PackageTarget* pNew = new PackageTarget;
        pPack->SetPackageTarget(pNew);
        if (pNew) {
            delete pNew;
        }
        pPack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_STEAL_WARN;
        pPack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
        pPack->GetPackageTarget()->SetTargReference(apActor);
        pPack->GetPackageTarget()->iValue = 0;
        pCurrentProcess->ComputeLastTimeProcessed();
        pCurrentProcess->SavePackageToExtraData(this);
        PutCreatedPackage(pPack, false, true);
        pPlayer->IssueStealWarning();
    }
}

void Actor::AttackAlarm(TESObjectREFR* apRef, bool abMinor, bool bModdispthis) {
    if (apRef == pPlayer && !GetIgnoreCrime()) {
        Actor* pTargActor = dynamic_cast<Actor*>(apRef);
        if ((!IsInCombatantFaction() || !pTargActor || !pTargActor->IsInCombatantFaction()) &&
            !pTargActor->IsActoraGuard()) {
            Crime* pCrime = new Crime(3u, this, static_cast<Actor*>(apRef), nullptr, 0, nullptr);
            pPlayer->IncNumCrimes(PackageNS::CRIME_TYPE_ATTACK, 1);
            BSSimpleList<TESObjectREFR*>* pRefList = ReferenceProcessLists.SendCrimetoHighList(pCrime);
            BSSimpleList<TESObjectREFR*>* pTemp = pRefList;
            float fTimeStamp = appCalendar.GetHour();
            m_Extra.AddToPlayerCrimeList(pCrime, fTimeStamp);
            pCrime->AddtoActorKnowList(this);
            ReferenceProcessLists.AddtoGlobalCrimeList(pCrime);
            if (pRefList) {
                while (pRefList && pRefList->m_item) {
                    Actor* pActor = static_cast<Actor*>(pRefList->m_item);
                    if (!pActor->IsDead(false) && !pActor->GetIgnoreCrime()) {
                        if ((abMinor && pActor == this) ||
                            (!abMinor && pActor->SetFactionsThatCareAboutCrime(this, pCrime))) {
                            Script::SetCrimeVictim(GetObjectReference());
                            pActor->SetDialogueItemTarget(pCrime->pcriminal);
                            TESTopic* pTopic = TESTopic::GetTopic(DT_COMBAT, 13);
                            pActor->pCurrentProcess->ProcessGreet(pActor, pTopic, false, false, true, false);
                            pCrime->AddtoActorKnowList(pActor);
                            Script::SetCrimeVictim(nullptr);
                        }
                        else {
                            Script::SetCrimeVictim(GetObjectReference());
                            pActor->SetDialogueItemTarget(pCrime->pcriminal);
                            TESTopic* pTopic = TESTopic::GetTopic(DT_COMBAT, 15);
                            FIGHT_REACTION eReaction = FIGHT_REACTION_NEUTRAL;
                            bool bIgnoreAmbush = this == pCrime->pcrimetarget;
                            if (pCrime->pcriminal &&
                                pActor->GetShouldAttackActor(pCrime->pcriminal, false, eReaction, false)) {
                                pActor->pCurrentProcess->EnterCombat(
                                    pActor,
                                    pCrime->pcriminal,
                                    bIgnoreAmbush,
                                    false,
                                    nullptr,
                                    false,
                                    false,
                                    false,
                                    0,
                                    false,
                                    false,
                                    true,
                                    nullptr
                                );
                            }
                            else {
                                pActor->pCurrentProcess->ProcessGreet(pActor, pTopic, false, false, true, false);
                            }
                            Script::SetCrimeVictim(nullptr);
                        }
                    }
                    pRefList = pRefList->m_pkNext;
                }
                if (pTemp) {
                    pTemp->RemoveAll();
                    if (pTemp) {
                        delete pTemp;
                    }
                }
                if (pCrime->GetRefCount()) {
                    if (abMinor) {
                        AddFactionMinorCrime(1, false);
                    }
                    else {
                        pPlayer->IncrementMajorCrimes();
                        HandleMajorCrimeFactionReputations(1, 0);
                    }
                }
                else {
                    char cMessage[308];
                    const char* pTargetName = apRef->GetFullName();
                    std::sprintf(cMessage, "%s  attacking %s no one cared", GetFullName(), pTargetName);
                    Interface::PrintLine(static_cast<const char*>(cMessage));
                }
            }
            if (pRefList) {
                pRefList->RemoveAll();
            }
            if (pRefList) {
                delete pRefList;
            }
        }
    }
}

void Actor::MurderAlarm(TESObjectREFR* apRef) {
    if (apRef) {
        Actor* pTargActor = dynamic_cast<Actor*>(apRef);
        uint8_t bIsKillerATeamMate = 0;
        if (pTargActor) {
            bIsKillerATeamMate = pTargActor->bPlayerTeammate;
        }
        if ((apRef == pPlayer || bIsKillerATeamMate) &&
            (!IsInCombatantFaction() || !pTargActor || !pTargActor->IsInCombatantFaction()) &&
            !pTargActor->IsActoraGuard() &&
            (!pTargActor ||
             pTargActor == pPlayer ||
             pTargActor->GetActorValue(ActorValue::SNEAK) < 100 ||
             !pTargActor->IsSneaking())) {
            Crime* pCrime = new Crime(4u, this, static_cast<Actor*>(apRef), nullptr, 0, nullptr);
            pPlayer->IncNumCrimes(PackageNS::CRIME_TYPE_MURDER, 1);
            float fTimeStamp = appCalendar.GetHour();
            m_Extra.AddToPlayerCrimeList(pCrime, fTimeStamp);
            ReferenceProcessLists.AddtoGlobalCrimeList(pCrime);
            pCrime->AddtoActorKnowList(this);
            SetFactionsThatCareAboutCrime(this, pCrime);
            BSSimpleList<TESObjectREFR*>* pRefList = ReferenceProcessLists.SendCrimetoHighList(pCrime);
            BSSimpleList<TESObjectREFR*>* pTemp = pRefList;
            if (pRefList) {
                while (pRefList && pRefList->m_item) {
                    Actor* pActor = static_cast<Actor*>(pRefList->m_item);
                    if (!pActor->IsDead(false) && !pActor->GetIgnoreCrime()) {
                        pCrime->AddtoActorKnowList(this);
                        if (pActor->SetFactionsThatCareAboutCrime(this, pCrime)) {
                            Script::SetCrimeVictim(GetObjectReference());
                            pActor->SetDialogueItemTarget(pCrime->pcriminal);
                            TESTopic* pTopic = TESTopic::GetTopic(DT_COMBAT, 14);
                            pActor->pCurrentProcess->ProcessGreet(pActor, pTopic, false, false, true, false);
                            pCrime->AddtoActorKnowList(pActor);
                            if (pActor->GetShouldHelp(this) && !pActor->IsInCombat()) {
                                pActor->StartCombat(pCrime->pcriminal, nullptr, false, false, false, 0, true, nullptr);
                            }
                        }
                        else {
                            Script::SetCrimeVictim(GetObjectReference());
                            pActor->SetDialogueItemTarget(pCrime->pcriminal);
                            TESTopic* pTopic = TESTopic::GetTopic(DT_COMBAT, 16);
                            FIGHT_REACTION eReaction = FIGHT_REACTION_NEUTRAL;
                            if ((pCrime->pcriminal &&
                                 pActor->GetShouldAttackActor(pCrime->pcriminal, false, eReaction, false)) ||
                                (pActor->GetShouldHelp(this) && !pActor->IsInCombat())) {
                                pActor->StartCombat(pCrime->pcriminal, nullptr, false, false, false, 0, true, nullptr);
                            }
                            else {
                                pActor->pCurrentProcess->ProcessGreet(pActor, pTopic, false, false, true, false);
                            }
                        }
                        Script::SetCrimeVictim(nullptr);
                    }
                    pRefList = pRefList->m_pkNext;
                }
                if (pTemp) {
                    pTemp->RemoveAll();
                    if (pTemp) {
                        delete pTemp;
                    }
                }
                if (pCrime->GetRefCount() > 0) {
                    pPlayer->IncrementMajorCrimes();
                    HandleMajorCrimeFactionReputations(1, 1);
                }
            }
            if (pRefList) {
                pRefList->RemoveAll();
                if (pRefList) {
                    delete pRefList;
                }
            }
        }
    }
}

int32_t Actor::TrespassAlarm(TESObjectREFR* apRef, TESForm* apOwnership, int32_t aiCrime) {
    ReferenceProcessLists.kNearbyActorLock.Lock(nullptr);
    bool bCrimeSeen = false;
    Actor** pCloseActors = ReferenceProcessLists.pActorsCloseToPlayerA;
    int32_t iNumCloseActors = ReferenceProcessLists.iActorsCloseToPlayer;
    for (int32_t iCurActor = 0; iCurActor < iNumCloseActors; ++iCurActor) {
        Actor* pAct = pCloseActors[iCurActor];
        pAct->GetDetectionLevel(pPlayer);
        IsActorOfFaction(apRef, apOwnership, pAct);
        if (!pAct->GetIgnoreCrime() &&
            pAct->GetDetectionLevel(pPlayer) > 0 &&
            IsActorOfFaction(apRef, apOwnership, pAct)) {
            pAct->pCurrentProcess->EnterCombat(
                pAct,
                pPlayer,
                false,
                false,
                nullptr,
                false,
                false,
                false,
                0,
                false,
                false,
                true,
                nullptr
            );
            if (!bCrimeSeen) {
                pAct->AddFactionMinorCrime(1, true);
                bCrimeSeen = true;
            }
        }
    }
    if (bCrimeSeen) {
        pPlayer->IncrementMinorCrimes();
    }
    ReferenceProcessLists.kNearbyActorLock.Unlock();
    return -1;
}

void Actor::DispellSpellsForRace() {
    if (GetRace()) {
        for (SpellItem* pSpell : GetRace()->spellList.ItemsUntilNull()) {
            RemoveSpell(pSpell);
        }
    }
}

// Resets the armor rating of the actor behind an actor value owner.
void Actor::ResetActorDTDR(
    ActorValueOwner* apOwner,
    ActorValue::Index /*aiWhich*/,
    float /*afOriginalValue*/,
    float /*afDelta*/,
    ActorValueOwner* /*apActorSource*/
) {
    if (apOwner) {
        static_cast<Actor*>(apOwner)->ResetArmorRating();
    }
}

// The ActorValueOwner callback: the actor's armor threshold, through the virtual GetArmorThreshold.
float Actor::GetTotalArmorDT(ActorValueOwner* apOwner, ActorValue::Index /*aiWhich*/) {
    Actor* pActor = apOwner ? static_cast<Actor*>(apOwner) : nullptr;
    return pActor->GetArmorThreshold();
}

float Actor::GetBaseDamageResistance(ActorValueOwner* apActor, ActorValue::Index aiWhich) {
    Actor* pActor = apActor ? static_cast<Actor*>(apActor) : nullptr;
    return pActor->GetArmorRating();
}

void Actor::AssistanceModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    if (!apActor) {
        return;
    }
    Actor* pActor = apActor ? static_cast<Actor*>(apActor) : nullptr;
    if (pActor->pCurrentProcess && pActor->pCurrentProcess->HasCachedValues()) {
        pActor->pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_ACTOR_ASSISTANCE);
    }
}

bool Actor::IsConsciousAndNotAttackingPlayer(Actor* apActor) {
    if (!apActor) {
        return false;
    }
    if (!apActor->CanSpeak()) {
        return false;
    }
    // The reaction comes back in an uninitialised local that is never read.
    FIGHT_REACTION eReaction;
    return !apActor->GetShouldAttackActor(pPlayer, false, eReaction, false);
}

float Actor::GetAttackSpeedMult(Actor* apActor, TESObjectWEAP* apWeapon, int8_t aiHand, ItemChange* apItem) {
    float fWeapSPS = 0.0f;
    float fWeapReloadTime = 0.0f;
    bool bClipSize = false;
    bool bSpeedMod = false;
    Animation* pAnim = pPlayer->GetAnimation(false);
    if (apItem) {
        bClipSize = apItem->HasModEffectActive(2);
        bSpeedMod = apItem->HasModEffectActive(8);
    }
    if (pAnim && aiHand) {
        ANIM_GROUP_ENUM eAnim = apWeapon->GetReloadAnim(bClipSize);
        if (eAnim == ANIM_GROUP_NONE) {
            eAnim = ANIM_GROUP_RELOAD_START;
        }
        uint16_t sAnimGroup = TESAnimGroup::AnimGroup(
            AGM_NONE,
            weaponToAnimGroupWeapon[apWeapon->GetWeaponType()],
            eAnim,
            false
        );
        uint16_t sBestGroup = pAnim->PickBestAnimation(sAnimGroup, false);
        if (TESAnimGroup::GetType(sBestGroup) == eAnim) {
            TESAnimGroup* pAnimGroup = pAnim->GetTESAnimGroup(sBestGroup);
            if (pAnimGroup) {
                double dTime = pAnimGroup->GetTime(AGA_STOP);
                fWeapReloadTime = apWeapon->GetSpeed() * dTime;
            }
        }
    }

    if (!apWeapon->IsAutomaticWeapon() && pAnim) {
        ANIM_GROUP_ENUM eAttackAnim = apWeapon->GetAttackAnim();
        if (eAttackAnim == ANIM_GROUP_NONE) {
            eAttackAnim = ANIM_GROUP_ATTACK_RIGHT;
        }
        uint16_t sAnimGroup = TESAnimGroup::AnimGroup(
            AGM_NONE,
            weaponToAnimGroupWeapon[apWeapon->GetWeaponType()],
            eAttackAnim,
            false
        );
        uint16_t sBestGroup = pAnim->PickBestAnimation(sAnimGroup, false);
        if (TESAnimGroup::GetType(sBestGroup) == eAttackAnim) {
            TESAnimGroup* pAnimGroup = pAnim->GetTESAnimGroup(sBestGroup);
            fWeapSPS = 1.0 / pAnimGroup->GetTime(AGA_SPECIAL_IDLE_STOP);
        }
    }
    else {
        double dShotsPerSec = apWeapon->data.fShotsPerSec;
        fWeapSPS = apWeapon->GetAttackSpeed(bSpeedMod) * dShotsPerSec;
    }
    if (aiHand && fWeapReloadTime > 0.0 && fWeapSPS > 0.0) {
        int32_t iNumRounds;
        if (apWeapon->IsThrownWeapon()) {
            iNumRounds = 1;
        }
        else {
            iNumRounds = apWeapon->GetFormClipRounds(bClipSize);
        }
        float fReloadRounds = fWeapReloadTime / static_cast<double>(iNumRounds);
        float fShotTime = 1.0 / fWeapSPS + fReloadRounds;
        fWeapSPS = 1.0 / fShotTime;
    }

    double dSpeedSPS = apWeapon->GetSpeed() * fWeapSPS;
    return apWeapon->GetAttackSpeed(bSpeedMod) * dSpeedSPS;
}

float Actor::GetInventoryWeight(ActorValueOwner* apActor, ActorValue::Index aiWhich) {
    bool bIsHardcore = pPlayer->IsHardcore();
    // apActor is the ActorValueOwner subobject of an Actor; a null one is passed on as null.
    return static_cast<Actor*>(apActor)->GetWeightInContainer(bIsHardcore);
}

bool Actor::GetDistanceForAnimGroup(Actor* apActor, ANIM_GROUP_ENUM aeGroup, NiPoint3* apDistance) {
    if (!apActor) {
        return false;
    }
    *apDistance = NiPoint3::ZERO;
    uint16_t sAnimGroup = apActor->GetAnimGroup(aeGroup, nullptr, false, nullptr);
    if (TESAnimGroup::IsPowerAttackAction(sAnimGroup)) {
        AnimSequenceBase* pAnimSeq;
        Animation* pAnim = apActor->GetAnimation();
        if (pAnim->GetMapAnimSequenceBase()->GetAt(sAnimGroup, pAnimSeq)) {
            TESAnimGroup* pAnimGroup = pAnimSeq->GetSequenceToPlay(-1)->GetAnimGroup();
            double dActionTime = pAnimGroup->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP);
            float fDuration = dActionTime - pAnimGroup->GetTime(AGA_START);
            NiPoint3 pos = pAnimGroup->GetDeltaPerSecond() * fDuration;
            float fScalar = apActor->GetScale();
            *apDistance = pos * fScalar;
            return true;
        }
    }
    return false;
}

// Each condition is tested against the doubles 0.0 and 2.0; NaN never queues.
void Actor::QueueReplacementLocomotion(
    Actor* apActor,
    float afPreviousBodyPartCondition,
    float afNewBodyPartCondition,
    float afOtherBodyPartCondition
) {
    float fChange = afNewBodyPartCondition - afPreviousBodyPartCondition;
    float fOther = afPreviousBodyPartCondition + afOtherBodyPartCondition;
    if (fChange < 0.0) {
        if (fOther == 2.0 && afNewBodyPartCondition == 0.0) {
            apActor->QueueReplacementLocomotion(true, nullptr);
        }
    }
    else if (fChange > 0.0 && fOther < 2.0 && afNewBodyPartCondition != 0.0 && afOtherBodyPartCondition > 0.0) {
        apActor->QueueReplacementLocomotion(false, nullptr);
    }
}

// Takes the picked-up reference out of the world's collision, then either deletes it (deferred through the garbage
// collector, out of its parent cell) or only marks it as picked up.
void Actor::RemovePickedUpObjectFromWorld(TESObjectREFR* apPickedUpRef, bool abDelete) {
    bhkWorld::Activate(apPickedUpRef->Get3D(), true, true, false);
    if (abDelete) {
        TESObjectCELL* pParentCell = apPickedUpRef->GetParentCell();
        apPickedUpRef->DetachHavok();
        GarbageCollector::Add(apPickedUpRef);
        if (pParentCell) {
            pParentCell->RemoveReference(apPickedUpRef);
        }
    }
    else {
        apPickedUpRef->MarkAsPickedUp();
    }
}

float Actor::CheckClampDamageModifier(ActorValueOwner* apOwner, ActorValue::Index aeAVIndex, float afValue) {
    if (afValue < 0.0 && ActorValue::IsFlagSet(aeAVIndex, ActorValue::DAMAGE_MIN_ZERO)) {
        double dBaseValue = apOwner->GetBaseActorFloatValue(aeAVIndex);
        float fValue = apOwner->GetDamageModifier(aeAVIndex) + dBaseValue;
        if (fValue < -afValue) {
            return -fValue;
        }
    }
    return afValue;
}

float Actor::GetAnimGroupDuration(Actor* apActor, ANIM_GROUP_ENUM aeGroup) {
    AnimSequenceBase* pAnimSequence;
    uint16_t sAnimGroup = apActor->GetAnimGroup(aeGroup, nullptr, false, nullptr);
    if (TESAnimGroup::IsPowerAttackAction(sAnimGroup) &&
        apActor->GetAnimation()->GetMapAnimSequenceBase()->GetAt(sAnimGroup, pAnimSequence)) {
        TESAnimGroup* pAnimGroup = pAnimSequence->GetSequenceToPlay(-1)->GetAnimGroup();
        double dTime = pAnimGroup->GetTime(AGA_STOP);
        return static_cast<float>(dTime - pAnimGroup->GetTime(AGA_START));
    }

    return -0x1p+0f;
}

bool Actor::IsUsingNonMeleeWeapon(Actor* apActor) {
    TESObjectWEAP* pCurrentWeapon = apActor->GetCurrentWeapon();
    return pCurrentWeapon && pCurrentWeapon->IsRangedWeapon();
}

// The callback a dialogue line's sound ends with: the actor talking to the player stops speaking, drops its creature
// lip sync and returns to its idle; a line that failed to play leaves the menu on the no-voice timer.
void Actor::ChangeDialogueAnimation(void* apContext, bool abSucceeded) {
    if (Actor::pTalkingToPlayer) {
        Actor::pTalkingToPlayer->bTalkingToPlayer = false;
        Actor::pTalkingToPlayer->bSpeakingDone = true;
        if (apContext) {
            Actor::pTalkingToPlayer->bIgnoreChangeAnimationCall = true;
        }
        if (Actor::pTalkingToPlayer->GetCurrentAIProcess() &&
            Actor::pTalkingToPlayer->GetCurrentAIProcess()->GetCreatureLipSynchAnim() &&
            Actor::pTalkingToPlayer->GetAnimation()) {
            Actor::pTalkingToPlayer->GetAnimation()->SetLipTimeMember(0.0f);
            Actor::pTalkingToPlayer->GetCurrentAIProcess()->SetCreatureLipSynchAnim(nullptr);
            Actor::pTalkingToPlayer->GetCurrentAIProcess()->SetCreatureLipSynchStartTime(0);
        }
        if (!Actor::pTalkingToPlayer->bIgnoreChangeAnimationCall && Actor::pTalkingToPlayer->GetCurrentAIProcess()) {
            Actor::pTalkingToPlayer->GetCurrentAIProcess()
                ->SetupSpecialIdle(Actor::pTalkingToPlayer, nullptr, AnimIdle::PLAY_TYPE_AUTO, false, true, true);
        }
        else {
            Actor::pTalkingToPlayer->bIgnoreChangeAnimationCall = false;
        }
        if (!abSucceeded) {
            DialogMenu::fUseTimerNoVoice = fDialogSpeechDelaySeconds.Float();
        }
    }
    Actor::pTalkingToPlayer = nullptr;
}

void Actor::ConditionModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    if (apActor) {
        Actor* pActor = static_cast<Actor*>(apActor);
        Actor* pActorSource = static_cast<Actor*>(apActorSource);
        if (pActor) {
            bool bCrippled;
            float fNewValue = pActor->GetActorFloatValue(aiWhich);
            if (pActor->IsPc() &&
                aiWhich >= ActorValue::PERCEPTION_CONDITION &&
                aiWhich <= ActorValue::RIGHT_MOBILITY_CONDITION &&
                pActor == pPlayer) {
                SurgeryMenu::ResetLimb(aiWhich);
            }
            if (pActor->IsPc() && afDelta < 0.0 && aiWhich != ActorValue::IGNORE_CRIPPLED_LIMBS) {
                if (fNewValue <= 0.0 && afOriginalValue > 0.0) {
                    // Calls through HUDMainMenu::pme without a null check: only the address of
                    // HUDMainMenu::pme is tested, and it is never null.
                    HUDMainMenu::pme->ShowCrippledGuy();
                    pActor->TriggerPain(true, false);
                    MiscStatManager::Increment(MiscStatManager::MS_HAVE_LIMBS_CRIPPLED);
                }
            }
            else if (afDelta < 0.0) {
                bCrippled = fNewValue <= 0.0 && afOriginalValue > 0.0;
                switch (aiWhich) {
                    case ActorValue::BRAIN_CONDITION:
                        if (bCrippled) {
                            pActor->SetActorValue(ActorValue::AGGRESSION, 100.0f);
                        }
                        [[fallthrough]];
                    case ActorValue::PERCEPTION_CONDITION:
                    case ActorValue::ENDURANCE_CONDITION:
                    case ActorValue::LEFT_ATTACK_CONDITION:
                    case ActorValue::RIGHT_ATTACK_CONDITION:
                    case ActorValue::LEFT_MOBILITY_CONDITION:
                    case ActorValue::RIGHT_MOBILITY_CONDITION:
                        if (bCrippled &&
                            pActor != pPlayer &&
                            pActorSource == pPlayer &&
                            !pActor->IsDead(false) &&
                            HUDMainMenu::pme) {
                            char cMessage[500];
                            char cPartMessage[200];
                            ourStrcpy(cPartMessage, "");
                            BGSBodyPartData* pPartData = pActor->GetActorBase()->GetBodyPartData();
                            char* pPartName = nullptr;
                            if (pPartData) {
                                BGSBodyPart* pPart = pPartData->GetBodyPart(aiWhich);
                                if (pPart) {
                                    // GetPartName returns const char*; the message builder takes char*.
                                    pPartName = const_cast<char*>(pPart->GetPartName());
                                }
                            }
                            MenuGameSettings::GetDamagedByPlayerBodyPartMessage(aiWhich, pPartName, cPartMessage, 200);
                            BSsprintf(cMessage, 500, "%s %s", pActor->GetFullName(), cPartMessage);
                            HUDMainMenu::pme->ShowMessage(
                                cMessage,
                                HUDMainMenu::HME_HAPPY,
                                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
                                "UIPopUpMessageGeneral",
                                2.0f,
                                false
                            );
                        }
                        break;
                    default: break;
                }
            }
            if (aiWhich == ActorValue::IGNORE_CRIPPLED_LIMBS) {
                double dLeftBodyPartCondition;
                double dRightBodyPartCondition;
                if (pActor) {
                    dLeftBodyPartCondition = CombatFormulas::GetBodyPartCondition(
                        pActor,
                        ActorValue::LEFT_MOBILITY_CONDITION,
                        false
                    );
                }
                else {
                    dLeftBodyPartCondition = CombatFormulas::GetBodyPartCondition(
                        nullptr,
                        ActorValue::LEFT_MOBILITY_CONDITION,
                        false
                    );
                }
                float fLeftCondition = dLeftBodyPartCondition;
                if (pActor) {
                    dRightBodyPartCondition = CombatFormulas::GetBodyPartCondition(
                        pActor,
                        ActorValue::RIGHT_MOBILITY_CONDITION,
                        false
                    );
                }
                else {
                    dRightBodyPartCondition = CombatFormulas::GetBodyPartCondition(
                        nullptr,
                        ActorValue::RIGHT_MOBILITY_CONDITION,
                        false
                    );
                }
                float fRightCondition = dRightBodyPartCondition;
                if (!(fNewValue <= 0.0) || !(afOriginalValue > 0.0)) {
                    if (afOriginalValue == 0.0 && afDelta != 0.0) {
                        if (fLeftCondition <= 0.0) {
                            Actor::QueueReplacementLocomotion(pActor, 0.0f, 1.0f, 1.0f);
                        }
                        if (fRightCondition <= 0.0) {
                            Actor::QueueReplacementLocomotion(pActor, 0.0f, 1.0f, 1.0f);
                        }
                    }
                }
                else {
                    if (fLeftCondition <= 0.0) {
                        Actor::QueueReplacementLocomotion(pActor, 1.0f, 0.0f, 1.0f);
                    }
                    if (fRightCondition <= 0.0) {
                        Actor::QueueReplacementLocomotion(pActor, 1.0f, 0.0f, 1.0f);
                    }
                }
            }
            if (aiWhich == ActorValue::PERCEPTION_CONDITION &&
                afDelta != 0.0 &&
                pActor->pCurrentProcess &&
                pActor->pCurrentProcess->HasCachedValues()) {
                pActor->pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_PERCEPTION_CONDITION);
            }

            if (aiWhich == ActorValue::LEFT_MOBILITY_CONDITION ||
                aiWhich == ActorValue::RIGHT_MOBILITY_CONDITION ||
                aiWhich == ActorValue::IGNORE_CRIPPLED_LIMBS) {
                pActor->ResetConditionPreventsRun();
            }
        }
    }
}

// apActor is the ActorValueOwner subobject of an Actor; the casts to and from Actor test for null.
void Actor::MobilityConditionModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    if (apActor) {
        Actor::ConditionModifiedCallback(apActor, aiWhich, afOriginalValue, afDelta, apActorSource);
        if ((aiWhich == ActorValue::LEFT_MOBILITY_CONDITION || aiWhich == ActorValue::RIGHT_MOBILITY_CONDITION) &&
            !apActor->GetActorValue(ActorValue::IGNORE_CRIPPLED_LIMBS)) {
            Actor* pActor = static_cast<Actor*>(apActor);
            float fPrevious;
            if (!(afOriginalValue <= 0.0)) {
                fPrevious = 1.0f;
            }
            else {
                fPrevious = 0.0f;
            }
            ActorValue::Index eOther = aiWhich == ActorValue::LEFT_MOBILITY_CONDITION
                ? ActorValue::RIGHT_MOBILITY_CONDITION
                : ActorValue::LEFT_MOBILITY_CONDITION;
            float afOtherBodyPartCondition = CombatFormulas::GetBodyPartCondition(pActor, eOther, false);
            pActor->SetWalkRunFlagDirty();
            float fNew = CombatFormulas::GetBodyPartCondition(pActor, aiWhich, false);
            Actor::QueueReplacementLocomotion(pActor, fPrevious, fNew, afOtherBodyPartCondition);
        }
    }
}

void Actor::PlayLandJumpSound(PlayerCharacter* apCharacter, HavokMaterialType aeMaterial) {
    BSSoundHandle kSound;
    TESSound* pExtraSound = nullptr;
    NiPoint3 kLocation = apCharacter->GetLocationOnReference();
    TESObjectCELL* pCell = TES::pInstance->pInteriorCell;
    if (!apCharacter->IsActor()) {
        return;
    }
    if (!pCell) {
        pCell = pDataHandler->GetCellFromWorldCoord(kLocation.x, kLocation.y, TES::pInstance->pWorldSpace, false);
    }
    if (apCharacter->IsActor() &&
        pCell &&
        ((pCell->GetInterior() && pCell->HasWater()) || !pCell->GetInterior()) &&
        !apCharacter->IsSwimming() &&
        static_cast<double>(kLocation.z) < apCharacter->GetRelevantWaterHeight()) {
        aeMaterial = BHK_MATERIAL_WATER;
    }

    if (aeMaterial == BHK_MATERIAL_MAX && !pCell->GetInterior() && pCell->GetLand()) {
        COORD_DATA coordData;
        pCell->GetLand()->GetCoordData(&coordData, kLocation, true);
        uint16_t sVertIdx = coordData.iVertidx;
        uint8_t cBlock = coordData.iBlock;
        TESLandTexture* pMainTexture = pCell->GetLand()->GetMainTexture(cBlock, sVertIdx);
        if (pMainTexture) {
            float fHeight = 0.0f;
            aeMaterial = pMainTexture->GetHavokMaterialType();
            if (pCell->GetLandHeight(kLocation, fHeight)) {
                kLocation.z = fHeight;
            }
        }
    }

    int32_t iFootstepIndex = ImpactMixer::GetFootstepIndexForType(aeMaterial);
    TESSound* pSound = ImpactMixer::pFootSteps[iFootstepIndex][8];
    if (apCharacter == pPlayer && pPlayer->IsPCToddler()) {
        pSound = pDataHandler->GetSound("FSTBabyLand");
    }
    bool b1stPerson = false;
    if (apCharacter == pPlayer) {
        b1stPerson = !pPlayer->IsActually3rdPerson();
    }
    if (b1stPerson) {
        uint32_t uiFlags = static_cast<BSGameSound::SoundFlags>(
            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_FOOTSTEPS | BSGameSound::SOUND_FLAG_ONE_SHOT |
            BSGameSound::SOUND_FLAG_IMPACT
        );
        if (pSound) {
            uint32_t uiSoundID = pSound->iFormID;
            kSound = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, uiFlags);
            float fVolume = 0.949999988079071 - RandomFloat(0.0f, 0.1f);
            kSound.SetVolume(fVolume);
            kSound.Play(false);
        }
        if (pExtraSound) {
            uint32_t uiSoundID = pExtraSound->iFormID;
            kSound = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, uiFlags);
            float fVolume = 0.949999988079071 - RandomFloat(0.0f, 0.1f);
            kSound.SetVolume(fVolume);
            kSound.Play(false);
        }
    }
    else {
        if (!FalloutAudio::SoundDistanceTest(pSound, kLocation, 1.0f) &&
            !FalloutAudio::SoundDistanceTest(pExtraSound, kLocation, 1.0f)) {
            return;
        }
        uint32_t uiFlags = BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_FOOTSTEPS |
            BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT;
        if (pSound) {
            uint32_t uiSoundID = pSound->iFormID;
            kSound = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, uiFlags);
            kSound.SetPosition(kLocation.x, kLocation.y, kLocation.z);
            kSound.SetObjectToFollow(apCharacter->Get3D());
            float fVolume = 0.949999988079071 - RandomFloat(0.0f, 0.1f);
            kSound.SetVolume(fVolume);
            kSound.Play(false);
        }
        if (pExtraSound) {
            uint32_t uiSoundID = pExtraSound->iFormID;
            kSound = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, uiFlags);
            kSound.SetPosition(kLocation.x, kLocation.y, kLocation.z);
            kSound.SetObjectToFollow(apCharacter->Get3D());
            float fVolume = 0.949999988079071 - RandomFloat(0.0f, 0.1f);
            kSound.SetVolume(fVolume);
            kSound.Play(false);
        }
    }
}

// apActor is the ActorValueOwner subobject of an Actor (+0xA4).
void Actor::HealthFatigueModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    if (apActor && (aiWhich == ActorValue::HEALTH || aiWhich == ActorValue::FATIGUE)) {
        Actor* pActor = static_cast<Actor*>(apActor);
        if (pActor->pCurrentProcess) {
            float fNewValue = afOriginalValue + afDelta;
            float fPermanentValue = apActor->GetPermanentActorFloatValue(aiWhich);
            bool bDamaged = afDelta < 0.0 && fPermanentValue > static_cast<double>(fNewValue);
            bool bRestored = afDelta > 0.0 && fPermanentValue <= static_cast<double>(fNewValue);
            if (bDamaged) {
                if (aiWhich == ActorValue::HEALTH) {
                    pActor->pCurrentProcess->SetCachedValueFlag(CachedValues::CVF_HEALTH_DAMAGED, true);
                }
                else if (aiWhich == ActorValue::FATIGUE) {
                    pActor->pCurrentProcess->SetCachedValueFlag(CachedValues::CVF_FATIGUE_DAMAGED, true);
                }
            }
            else if (bRestored) {
                if (aiWhich == ActorValue::HEALTH) {
                    pActor->pCurrentProcess->SetCachedValueFlag(CachedValues::CVF_HEALTH_DAMAGED, false);
                }
                else if (aiWhich == ActorValue::FATIGUE) {
                    pActor->pCurrentProcess->SetCachedValueFlag(CachedValues::CVF_FATIGUE_DAMAGED, false);
                }
            }
        }
    }
}

// apActor is the ActorValueOwner subobject of an Actor (+0xA4).
void Actor::MedicineSkillModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    if (apActor) {
        Actor* pActor = static_cast<Actor*>(apActor);
        if (pActor->pCurrentProcess && pActor->pCurrentProcess->HasCachedValues()) {
            pActor->pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_MEDICINE_EFFECTIVENESS_MULT);
        }
    }
}

void Actor::SurvivalSkillModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    if (apActor) {
        Actor* pActor = static_cast<Actor*>(apActor);
        if (pActor->pCurrentProcess && pActor->pCurrentProcess->HasCachedValues()) {
            pActor->pCurrentProcess->SetCachedValueDirty(CachedValues::kCached_SurvivalSkillMult);
        }
    }
}

void Actor::ParalysisModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    if (apActor) {
        Actor* pActor = static_cast<Actor*>(apActor);
        if (pActor->pCurrentProcess && pActor->pCurrentProcess->HasCachedValues()) {
            pActor->pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_PARALYSIS);
        }
    }
}

void Actor::HealingRateModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    if (apActor) {
        Actor* pActor = static_cast<Actor*>(apActor);
        if (pActor->pCurrentProcess && pActor->pCurrentProcess->HasCachedValues()) {
            pActor->pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_HEALING_RATE);
        }
    }
}

void Actor::EnduranceModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    if (apActor) {
        Actor* pActor = static_cast<Actor*>(apActor);
        if (pActor->pCurrentProcess && pActor->pCurrentProcess->HasCachedValues()) {
            pActor->pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_ENDURANCE);
        }

        if (afDelta < 0.0) {
            float fHealthDamage = apActor->GetDamageModifier(ActorValue::HEALTH);
            if (fHealthDamage < 0.0) {
                float afEndurance = afOriginalValue + afDelta;
                float fNewHealth = AiFormulas::CalculateDerivedHealth(apActor, afEndurance);
                double dBaseHealth = static_cast<double>(pActor->GetActorBase()->GetHealth());
                double dPermanentHealth = apActor->GetPermanentModifier(ActorValue::HEALTH) + dBaseHealth;
                float fHealth = apActor->GetTemporaryModifier(ActorValue::HEALTH) + dPermanentHealth + fHealthDamage +
                    fNewHealth;
                if (fHealth < 0.0) {
                    float fRestore = 1.0 - fHealth;
                    pActor->RestoreActorValue(ActorValue::HEALTH, fRestore);
                }
            }
        }
    }
}

void Actor::AggressionModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    if (apActor) {
        Actor* pActor = static_cast<Actor*>(apActor);
        if (pActor->GetCurrentAIProcess()) {
            if (pActor->GetCombatController()) {
                pActor->QCombatController()->ForceStopCombat();
            }
            pActor->GetCurrentAIProcess()->EvaluateDetection(pActor);
            pActor->GetCurrentAIProcess()->EvaluateCombatDetectionLists(pActor);
        }
        if (pActor->pCurrentProcess && pActor->pCurrentProcess->HasCachedValues()) {
            pActor->pCurrentProcess->SetCachedValueDirty(CachedValues::CVF_ACTOR_AGGRESSION);
        }
    }
}

void Actor::RadiationResistanceModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    if (apActor) {
        static_cast<Actor*>(apActor)->ResetRadiationResistanceMult();
    }
}

void Actor::InventoryWeightModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    if (apActor && static_cast<Actor*>(apActor)->IsPc()) {
        bool bOverEncumbered = pPlayer->IsOverEncumbered();
        bool bWasOverEncumbered = !PlayerCharacter::IsGodMode() &&
            static_cast<double>(afOriginalValue) > pPlayer->GetMaxCarryWeight();
        if (bOverEncumbered && !bWasOverEncumbered) {
            Interface::ShowMessage(
                static_cast<const char*>(sOverEncumbered),
                0,
                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
                nullptr,
                2.0f,
                false
            );
        }
    }
}

// The note callback RegisterGrabIKNote hands the receiver: the note's arm starts or eases out of grab IK on the actor's
// ragdoll controller and takes the note's gain.
void Actor::HandleGrabIKNote(TESObjectREFR* apObjRefr, BSAnimNote* apNote) {
    Actor* pActor = static_cast<Actor*>(apObjRefr);
    BSGrabIKNote* pNote = static_cast<BSGrabIKNote*>(apNote);
    if (pActor->pRagdollController) {
        bhkRagdollController::GRAB_IK_ARM eArm = static_cast<bhkRagdollController::GRAB_IK_ARM>(pNote->eArm);
        if (pNote->eState == BSGrabIKNote::BS_GIK_START) {
            pActor->pRagdollController->SetGrabIKEnabled(eArm, true);
        }
        if (pNote->eState == BSGrabIKNote::BS_GIK_STOP) {
            pActor->pRagdollController->SetGrabIKEaseOut(eArm, true);
        }
        pActor->pRagdollController->SetGrabIKIntrpGain(eArm, pNote->fGain);
    }
}

FIGHT_REACTION
UpdateFightReaction(FIGHT_REACTION aeCurrentFightReaction, TESFaction* apFaction1, TESFaction* apFaction2) {
    FIGHT_REACTION eReaction = static_cast<FIGHT_REACTION>(apFaction1->GetFactionCombatRelation(apFaction2));
    if (aeCurrentFightReaction == FIGHT_REACTION_NEUTRAL) {
        return eReaction;
    }
    if (aeCurrentFightReaction == FIGHT_REACTION_ENEMY) {
        if (eReaction) {
            return eReaction;
        }
    }
    else if (aeCurrentFightReaction == FIGHT_REACTION_FRIEND && eReaction == FIGHT_REACTION_ALLY) {
        return FIGHT_REACTION_ALLY;
    }
    return aeCurrentFightReaction;
}

bool Cmd_GetIgnoreFriendlyHits(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if (static_cast<Actor*>(apRefObject)->IsIgnoreFriendlyHits()) {
        afReturnValue = 1.0;
    }
    return true;
}

bool IsActorOfFaction(TESObjectREFR* apRef, TESForm* apOwnership, Actor* pactor) {
    if (apOwnership) {
        if (apOwnership->GetFormType() == NPC__ID) {
            BSSimpleList<FACTION_RANK*>* pList = static_cast<TESNPC*>(apOwnership)->GetFactionList();
            if (pList && IsActorOfFactionList(pList, pactor, "NPC__ID")) {
                return true;
            }
        }
        else if (apOwnership->GetFormType() == FACT_ID) {
            if (pactor->IsInFaction(static_cast<TESFaction*>(apOwnership))) {
                return true;
            }
        }
    }
    if (apRef) {
        Actor* pRefActor = dynamic_cast<Actor*>(apRef);
        if (pRefActor) {
            uint32_t uiMyCount;
            TESFaction* pFactionArray[128];
            BSSimpleList<FACTION_RANK*>* pRefFactionList = &pRefActor->GetActorBase()->listFactions;
            ExtraFactionChanges* pRefExtraList = pRefActor->m_Extra.GetFactionChangesExtra();
            if (pRefExtraList) {
                uiMyCount = pRefActor->IntegrateFactionLists(
                    pFactionArray,
                    128u,
                    pRefFactionList,
                    pRefExtraList->pFactionChanges
                );
            }
            else {
                uiMyCount = pRefActor->IntegrateFactionLists(pFactionArray, 128u, pRefFactionList, nullptr);
            }
            for (uint32_t i = 0; i < uiMyCount; ++i) {
                if (pactor->IsInFaction(pFactionArray[i])) {
                    return true;
                }
            }
        }
        else if (apRef->GetFormType() == NPC__ID) {
            TESNPC* pNPC = dynamic_cast<TESNPC*>(apRef);
            if (pNPC) {
                BSSimpleList<FACTION_RANK*>* pList = &pNPC->listFactions;
                if (pList && IsActorOfFactionList(pList, pactor, "apRef NPC__ID")) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Returns false when the walk ends (a departure): the exe has no return after the loop and hands back a leftover value,
// the caller's for a null pList and the low byte of the node for a node with no item (so usually true for an empty
// list), 0 only at the list's end.
bool IsActorOfFactionList(BSSimpleList<FACTION_RANK*>* pList, Actor* pActor, const char* debugReason) {
    while (pList && pList->m_item) {
        FACTION_RANK* pRank = pList->m_item;
        if (pRank && pActor->IsInFaction(pRank->pFaction)) {
            return true;
        }
        pList = pList->m_pkNext;
    }
    return false;
}

// The constraint instance's data, else the data of the creation info the constraint was loaded with, else null.
hkpConstraintData* bhkConstraint::GetData() {
    hkpConstraintInstance* pConstraint = **this;
    if (pConstraint) {
        return pConstraint->getDataRw();
    }
    if (!pInfo) {
        return nullptr;
    }
    return static_cast<hkConstraintCinfo*>(pInfo)->pData;
}

bhkFilteredCdBodyCollector::bhkFilteredCdBodyCollector() {}

void bhkLimitedHingeConstraint::SetMotorActive(bool abActive) {
    hkpLimitedHingeConstraintData* pData = GetMotor();
    if (pData) {
        hkBool kActive(abActive);
        pData->setMotorActive(**this, kActive);
    }
}

void bhkOrientHingedBodyAction::SetEnabled(bool abEnable) {
    (**this)->SetEnabled(abEnable);
}

hkOrientHingedBodyAction* bhkOrientHingedBodyAction::operator*() {
    if (this) {
        return static_cast<hkOrientHingedBodyAction*>(GethkObject());
    }

    return nullptr;
}

void bhkRagdollController::SetUsePhantomFootIKCasts(bool abUsePhantom) {
    bUsePhantomFootIKCasts = abUsePhantom;
}

void bhkRagdollController::SetPoseMatchEnable(bool abEnable) {
    bEnablePoseMatch = bInitPoseMatch && abEnable;
}

void bhkRagdollController::SetPhantom(bhkShapePhantom* apPhantom) {
    pControllerPhantom = static_cast<hkpCachingShapePhantomRay*>(**apPhantom);
}

bool bhkRagdollController::QRagdollFeedbackEnable() {
    return bEnableRagdollFeedback;
}

uint32_t bhkRagdollController::QFeedbackImpulseWeak() {
    return uiFeedbackImpulseWeak;
}

uint32_t bhkRagdollController::QFeedbackImpulseStrong() {
    return uiFeedbackImpulseStrong;
}

void bhkRagdollController::SetRagdollAnimEaseOut(bool abEaseOut) {
    bEaseOutFootIK = abEaseOut;
    GrabIKState[1].bEaseOutGrabIK = abEaseOut;
    GrabIKState[0].bEaseOutGrabIK = abEaseOut;
    bEaseOutLookIK = abEaseOut;
    bEaseOutRagdollAnim = abEaseOut;
}

void bhkRagdollController::SetHeadOffset(const NiQuaternion& arRot) {
    NI2HKMIGRATION_QUATERNION(CurrentHeadRotOffset, arRot);
}

void bhkRagdollController::SetGrabIKEaseOut(bhkRagdollController::GRAB_IK_ARM arArm, bool abEnable) {
    GrabIKState[arArm].bEaseOutGrabIK = abEnable;
}

void* bhkRagdollController::operator new(size_t blocksize) {
    return MemoryManager::Instance().Allocate(blocksize);
}

bhkRagdollTemplateData* bhkRagdollTemplate::GetRagdollTemplateData(uint32_t aiIndex) {
    return RagdollTemplateA[aiIndex];
}

void Animation::SetGlobalTimeMultiplier(float fValue) {
    this->m_fGlobalTimeMultiplier = fValue;
}

bool Animation::ReloadAnimQueued() {
    return this->sQueuedReloadGroup != 0xFF;
}

void Animation::QueueReloadAnim(uint16_t asGroup) {
    sQueuedReloadGroup = asGroup;
}

void Animation::SetReloadModifier(float fValue) {
    m_fReloadModifier = fValue;
}

float BGSBodyPart::GetTrackingMaxAngle() {
    return Data.fTrackingMaxAngle;
}

// The name of the node the part's IK chain starts at.
const char* BGSBodyPart::GetIKStartNodeName() {
    return IKStartNodeName.String();
}

float BGSImpactData::GetEffectDuration() {
    return Data.fEffectDuration;
}

TESForm* BipedAnim::GetParent(BIPED_OBJECT aeBipedObject) {
    return this->object[aeBipedObject].pParent;
}

void ExtraDismemberedLimbs::SetEaten(bool abEaten) {
    bEaten = abEaten;
}

void TargetEntry::SetHit(bool abTF) {
    SetFlags(TGE_HIT, abTF);
}

hkpAllCdPointCollector* HitCaster::GetPointCollector() {
    return this;
}

// The shipped debug info places TESPackage::GetPCContinue in this file.
bool TESPackage::GetPCContinue() {
    return (PackData.iPackFlags & kPackageFlag_PCContinue) != 0;
}

void Script::SetCrimeVictim(TESForm* apform) {
    pCrimeVictim = apform;
}

uint8_t TESAnimGroup::GetDecal() {
    return cDecal;
}

float TESObjectWEAP::GetKillImpulse() {
    return data.fKillImpulse;
}

float TESObjectWEAP::GetKillImpulseDistance() {
    return data.fKillImpulseDistance;
}

void BaseProcess::SetCachedWalkSpeed(float afWalkSpeed) {
    if (pCachedValues) {
        pCachedValues->fCachedWalkSpeed = afWalkSpeed;
        pCachedValues->SetUpToDate(CachedValues::CVF_WALK_SPEED);
    }
}

void BaseProcess::SetCachedRunSpeed(float afRunSpeed) {
    if (pCachedValues) {
        pCachedValues->fCachedRunSpeed = afRunSpeed;
        pCachedValues->SetUpToDate(CachedValues::CVF_RUN_SPEED);
    }
}

void BaseProcess::SetCachedPerceptionCondition(float afPerceptionCondition) {
    if (pCachedValues) {
        pCachedValues->fCachedPerceptionCondition = afPerceptionCondition;
        pCachedValues->SetUpToDate(CachedValues::CVF_PERCEPTION_CONDITION);
    }
}

void BaseProcess::SetCachedMedicineEffectivenessMult(float afMedicineEffectivenessMult) {
    if (pCachedValues) {
        pCachedValues->fCachedMedicineEffectivenessMult = afMedicineEffectivenessMult;
        pCachedValues->SetUpToDate(CachedValues::CVF_MEDICINE_EFFECTIVENESS_MULT);
    }
}

void BaseProcess::SetCachedSurvivalEffectivenessMult(float afSurvivalEffectivenessMult) {
    if (pCachedValues) {
        pCachedValues->fCachedSurvivalEffectivenessMult = afSurvivalEffectivenessMult;
        pCachedValues->SetUpToDate(CachedValues::kCached_SurvivalSkillMult);
    }
}

void BaseProcess::SetCachedHealingRate(float afHealingRate) {
    if (pCachedValues) {
        pCachedValues->fCachedHealingRate = afHealingRate;
        pCachedValues->SetUpToDate(CachedValues::CVF_HEALING_RATE);
    }
}

void BaseProcess::SetCachedLength(float afLength) {
    if (pCachedValues) {
        pCachedValues->fCachedLength = afLength;
        pCachedValues->SetUpToDate(CachedValues::CVF_LENGTH);
    }
}

float Projectile::GetDamage() {
    return fDamage;
}

TESIdleForm* VATS::GetMeleeAttack() {
    return pMeleeAttack;
}

MobileObject* DialoguePackage::GetTalkingActivator() {
    return pTalkingActivator;
}

NiFixedString& FixedStrings::QBip01Pitch() {
    return *FixedStrings::pBip01Pitch;
}

NiFixedString& FixedStrings::QGrabLeft() {
    return *FixedStrings::pGrabLeft;
}

NiFixedString& FixedStrings::QGrabRight() {
    return *FixedStrings::pGrabRight;
}

float FleePackage::GetFleeTimer() {
    return fFleeTimer;
}

TESObjectREFR* FleePackage::GetRefFleeTo() {
    return pRefFleeTo;
}

void FleePackage::SetFleeSucceeded(bool abflag) {
    bFleeSucceeded = abflag;
}

void MiddleHighProcess::SetDetectLifeDetected(bool abDetected) {
    bDetectLifeDetected = abDetected;
}

bool MiddleHighProcess::GetDetectLifeDetected() {
    return bDetectLifeDetected;
}

// Calls TESObjectREFR::Get3D directly and passes GetObjectByName two arguments. The comparison is false when
// afMaxHeight is below the height or either is NaN.
bool MobileObject::IsNonAccumNearController(float afMaxHeight) {
    bool bResult = true;
    bhkCharacterController* pCharController = GetCharController();
    if (!pCharController) {
        return bResult;
    }
    NiNode* p3D = TESObjectREFR::Get3D();
    if (!p3D) {
        return bResult;
    }
    NiAVObject* pNonAccum = BSUtilities::GetObjectByName(p3D, "Bip01 NonAccum");
    if (!pNonAccum) {
        return bResult;
    }
    NiPoint3 kNonAccum = pNonAccum->GetWorldTranslate();
    NiPoint3 kPosition;
    pCharController->GetPosition(kPosition);
    float fHeight = kNonAccum.z - kPosition.z;
    bResult = afMaxHeight >= fHeight;
    return bResult;
}

// Reports whether a look-at target is active.
bool ActorMover::HasLookAtTarget() const {
    return bFaceTargetPoint;
}

void PlayerCharacter::RemoveTeammate(Actor* pActor) {
    --iTeammateCount;
    pListOfTeammates.Remove(pActor);
}

void PlayerCharacter::AddTeammate(Actor* pActor) {
    ++iTeammateCount;
    pListOfTeammates.AddHead(pActor);
}

BSAnimNoteListener* NiControllerManager::QNoteListener() {
    return pListener;
}

// m_pos's constructor stores nothing.
hkSphere::hkSphere() {}

void hkSphere::setPosition(const hkVector4& newPos) {
    m_pos.setXYZ(newPos);
}

// Sets w through hkVector4::operator().
void hkSphere::setRadius(float newRadius) {
    m_pos(3) = newRadius;
}

const hkClass& hkSphere::staticClass() {
    return hkSphereClass;
}
