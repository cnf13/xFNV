#include "Fallout/AI/PlayerCharacter.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/MemoryPool.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSDevices/XPrefSettings.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkBlendCollisionObjectAddRotation.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkForceController.h"
#include "BSHavok/bhkMouseSpringAction.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRagdollPenetrationUtil.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldM.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/IOManager.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSMovie/MoviePlayer.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSCameraShot.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSEntryPointPerkEntry.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSPerkEntry.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRadiation.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BGSTerminal.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExteriorCellLoader.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/FindObjectIterator.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/Havok/BGSAcousticSpaceListener.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/Havok/SpecificItemCollector.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/Havok/ViewCaster.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/ImageSpaceModifierInstanceDOF.h"
#include "Fallout Shared/ImageSpaceModifierInstanceDRB.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/LockLevel.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicItemForm.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MediaLocationController.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionDataImposter.h"
#include "Fallout Shared/Region System/TESRegionDataList.h"
#include "Fallout Shared/Region System/TESRegionDataMap.h"
#include "Fallout Shared/Region System/TESRegionDataSound.h"
#include "Fallout Shared/Region System/TESRegionDataWeather.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Region System/TESRegionPointList.h"
#include "Fallout Shared/SCRIPT_PARAMETER.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESCaravan.h"
#include "Fallout Shared/TESCasinoChips.h"
#include "Fallout Shared/TESChallenge.h"
#include "Fallout Shared/TESChallengeManager.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEffectShader.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESFurniture.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpace.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestStage.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESReputation.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/AlarmPackage.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CameraShake.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatDialogueManager.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/ContinuousBeamProjectile.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/MuzzleFlash.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Combat/ProjectileTarget.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/FalloutCharacterProgression.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/GunWobble.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/ModifierList.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/Movement/PlayerMover.h"
#include "Fallout/AI/MusicMarker.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/AI/SpectatorPackage.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/MenuManager.h"
#include "Fallout/Interface/Menus/BlackJackMenu.h"
#include "Fallout/Interface/Menus/CharGenMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/HackingMenu.h"
#include "Fallout/Interface/Menus/LevelUpMenu.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/MapMenu.h"
#include "Fallout/Interface/Menus/MessageMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/DetectLifeEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicHitEffect.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/NightEyeEffect.h"
#include "Fallout/Misc/BGSMoviePlayer.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/MemoryLevelManager.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <xmmintrin.h>

// The names of the causes of death (CAUSE_OF_DEATH).
using CauseOfDeathNameArray = const char* [7];
static CauseOfDeathNameArray
    CauseOfDeathNames{"Explosion", "Gun", "BluntWeapon", "HandToHand", "ObjectImpact", "Poison", "Radiation"};

// The third-person wobble scale AddGunWobble eases toward its target (initialised to 1.0f).
static float fWobbleChaseDrift = 1.0f;
static int32_t idistance = 300;

// UpdateArmRotation's function-local static: how far the arms follow the camera, eased toward the target.
static float fchasePercent = 1.0f;

// The node the first-person skeleton looks through; null while the first-person 3D lacks it.
static NiAVObject* p1stPersonLooking;
static float flastCameraZ;
static uint32_t center_guard;
static TESObjectREFR* pprevVatsTarget;

// CheckUserInputAttacks's function-local static: ivatsAttackDelay counts the frames left before VATS forces the
// next shot.
static int32_t ivatsAttackDelay;

// AddGunWobble's function-local statics: the melee aim chase angles (zero-initialised, unguarded).
static float fcurrentChaseLooking;
static float fcurrentChaseHeading;
BSSpinLock PlayerCharacter::kPerceivedActorsLock;

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bSetDemigodMode("bSetDemigodMode:GamePlay", false);
SettingT<INISettingCollection> bInstantLevelUp("bInstantLevelUp:GamePlay", false);
SettingT<INISettingCollection> bHealthBarShowing("bHealthBarShowing:GamePlay", false);
SettingT<INISettingCollection> bEssentialTakeNoDamage("bEssentialTakeNoDamage:GamePlay", true);
SettingT<INISettingCollection> bAllowHavokGrabTheLiving("bAllowHavokGrabTheLiving:GamePlay", false);
SettingT<INISettingCollection> iActorsDismemberedPerFrame("iActorsDismemberedPerFrame:GamePlay", 2);
SettingT<INISettingCollection> bTrackProgress("bTrackProgress:GamePlay", false);

// No code reads this setting.
static SettingT<INIPrefSettingCollection> uiRedeemedContentMask("uiRedeemedContent:GamePlay", 0u);
SettingT<INIPrefSettingCollection> iKillCamera("iKillCamera:GamePlay", 2);
SettingT<INIPrefSettingCollection> iDifficulty("iDifficulty:GamePlay", 2);
SettingT<INIPrefSettingCollection> bShowQuestMarkers("bShowQuestMarkers:GamePlay", true);
SettingT<INISettingCollection> sTrackProgressPath("sTrackProgressPath:GamePlay", "\\\\vault2\\Fallout\\LevelData\\");
SettingT<INISettingCollection> fCameraCasterSize("fCameraCasterSize:HAVOK", 10.0f);
SettingT<INISettingCollection> fCameraCasterPlayerSize("fCameraCasterPlayerSize:HAVOK", 1.0f);
SettingT<INISettingCollection> fUFOCamSpeedMult("fUFOCamSpeedMult:Controls", 1.0f);
SettingT<INISettingCollection> bShowGunTarget("bShowGunTarget:General", false);
SettingT<INISettingCollection> iJoystickMoveFrontBack("iJoystickMoveFrontBack:Controls", 2);
SettingT<INISettingCollection> iJoystickMoveLeftRight("iJoystickMoveLeftRight:Controls", 1);
SettingT<INISettingCollection> iJoystickLookUpDown("iJoystickLookUpDown:Controls", 6);
SettingT<INISettingCollection> iJoystickLookLeftRight("iJoystickLookLeftRight:Controls", 3);
SettingT<INIPrefSettingCollection> fMouseSensitivity("fMouseSensitivity:Controls", 0.002f);

// The controller stick and 360 controller mouse multipliers, which no code here reads.
static SettingT<INISettingCollection> gs_fJoystickMoveFBMult_Controls("fJoystickMoveFBMult:Controls", 1.0f);
static SettingT<INISettingCollection> gs_fJoystickMoveLRMult_Controls("fJoystickMoveLRMult:Controls", 1.0f);
static SettingT<INISettingCollection> gs_fJoystickLookUDMult_Controls("fJoystickLookUDMult:Controls", 0.02f);
static SettingT<INISettingCollection> gs_fJoystickLookLRMult_Controls("fJoystickLookLRMult:Controls", 0.02f);
static SettingT<INISettingCollection> gs_fPC360ControllerMouseXMult("fPC360ControllerMouseXMult", 1600.0f);
static SettingT<INISettingCollection> gs_fPC360ControllerMouseYMult("fPC360ControllerMouseYMult", -1600.0f);
SettingT<INIPrefSettingCollection> bInvertYValues("bInvertYValues:Controls", false);
SettingT<INISettingCollection> fDlgFocus("fDlgFocus:Interface", 3.2f);
SettingT<INISettingCollection> fDlgLookDegStart("fDlgLookDegStart:Interface", 13.0f);
SettingT<INISettingCollection> fDlgLookDegStop("fDlgLookDegStop:Interface", 0.2f);
SettingT<INISettingCollection> fDlgHeadingDegStart("fDlgHeadingDegStart:Interface", 13.0f);
SettingT<INISettingCollection> fDlgHeadingDegStop("fDlgHeadingDegStop:Interface", 0.2f);
SettingT<INISettingCollection> fDlgLookAdj("fDlgLookAdj:Interface", -5.0f);
SettingT<INISettingCollection> fDlgLookMult("fDlgLookMult:Interface", 2.0f);
SettingT<INISettingCollection> bDisablePlayerCollision("bDisablePlayerCollision:HAVOK", false);
SettingT<INISettingCollection> bIronSightsZoomEnable("bIronSightsZoomEnable:Combat", true);
SettingT<INISettingCollection> fIronSightsZoomDefault("fIronSightsZoomDefault:Combat", 50.0f);
SettingT<INISettingCollection> fZoom3rdPersonSnapDist("fZoom3rdPersonSnapDist:General", 50.0f);
SettingT<INISettingCollection> iHoursToSleep("iHoursToSleep:General", 3);
SettingT<INISettingCollection> bEnableBorderRegion("bBorderRegionsEnabled:General", true);
SettingT<INISettingCollection> fForwardBias("fForwardBias:Controls", 1.7f);
SettingT<INISettingCollection> fXenonMinLookSpeed("fXenonMinLookSpeed:Controls", 0.05f);
SettingT<INIPrefSettingCollection> fXenonHorizLookSpeed("fXenonHorizLookSpeed:Controls", 1500.0f);
SettingT<INIPrefSettingCollection> fXenonVertLookSpeed("fXenonVertLookSpeed:Controls", 1200.0f);
SettingT<INISettingCollection> iXenonHorizLookAccel("iXenonHorizLookAccel:Controls", 2);
SettingT<INISettingCollection> iXenonVertLookAccel("iXenonVertLookAccel:Controls", 2);
SettingT<INISettingCollection> bAlwaysRunByDefault("bAlwaysRunByDefault:Controls", false);
SettingT<INISettingCollection> fPlayerCharacterTurnMult("fPlayerCharacterTurnMult:Controls", 0.02f);
NiPoint3 PlayerCharacter::kVanityModePos(NiPoint3::ZERO);
NiPoint3 PlayerCharacter::kAutoVanityModePos(NiPoint3::ZERO);
SettingT<INISettingCollection> bDisableAutoVanityMode("bDisableAutoVanityMode:General", false);
NiPoint3 chaseCameraPos(NiPoint3::ZERO);
NiMatrix3 GunWobbleRot = NiMatrix3::IDENTITY;
BSSimpleList<TESObjectREFR*> ConflictingObjects;
NiPick kConflictingPick(0, 8);

void MarkPlayerBones(NiAVObject* apObject) {
    if (apObject) {
        apObject->SetPlayerBone(true);
        if (apObject->IsNode()) {
            NiNode* pNode = apObject->IsNode();
            for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                NiAVObject* pChild = pNode->GetAt(i);
                if (pChild) {
                    MarkPlayerBones(pChild);
                }
            }
        }
    }
}

// An IterateInventory callback: appends the item's worn mark, count, name and form ID to the string apData.
uint8_t apCallback(ItemChange* apItem, BSStringT<char>* apData) {
    if (apData && apItem) {
        char acBuffer[264];
        const char* pWorn = apItem->GetWorn(false) ? "[E]" : "";
        uint32_t uiFormID = apItem->pContainerObj->iFormID;
        const char* pFullName = apItem->GetFullName();
        BSsprintf(acBuffer, 260, "%s%i %s (%08X),", pWorn, apItem->iNumber, pFullName, uiFormID);
        *apData += static_cast<const char*>(acBuffer);
    }

    return 0;
}

bool TESAnimGroupIsJumpingLandAnim(TESAnimGroup* apThis, [[maybe_unused]] void* apEdx) {
    int32_t iType = TESAnimGroup::GetType(apThis->sType);
    return iType == ANIM_GROUP_JUMP_LAND ||
        (iType >= ANIM_GROUP_JUMP_LAND_FORWARD && iType <= ANIM_GROUP_JUMP_LAND_RIGHT);
}

void ShakeData() {
    char acModelPath[268];
    ShakeData_pModel = nullptr;
    BSsprintf(acModelPath, 260u, "%s\\Characters\\CameraShake.nif", "Meshes");
    if (FileFinder::Locate(acModelPath, nullptr, 0, 0xFFFFFFFFu)) {
        ShakeData_pModel = pModelLoader->LoadFile(acModelPath, LOD_MULT_NONE, true, false, false, false);
        NiFixedString kName(acModelPath);
        ShakeData_pModel->SetName(kName);
    }
}

void FreeCameraShake() {
    if (ShakeData_pModel) {
        const char* pName = ShakeData_pModel->m_kName.GetHandleString();
        pModelLoader->ReleaseFile(pName);
        ShakeData_pModel = nullptr;
    }
}

void DeleteCharacterProgressionData(CharacterProgression* apThis, [[maybe_unused]] void* apEdx) {
    apThis->DeleteData();
}

void SetLastAntiCheatTime() {
    uiAntiCheatTimer = GetTickCount() / 1000;
}

// Only the remaining time is cleared.
NonLinearInterpolator::NonLinearInterpolator() : fRemainingTime(0.0f) {}

bool NonLinearInterpolator::Interpolate(float afDeltaTime) {
    fRemainingTime = fRemainingTime - afDeltaTime;
    return fRemainingTime <= 0.0;
}

bool NonLinearInterpolator::GetFinished() {
    return fRemainingTime <= 0.0;
}

bool NonLinearInterpolator::Setup(float afStart, float afEnd, float afTime) {
    float fcurrent = 0.0f;
    float fincrement = 0.02f;
    for (int32_t i = 0; i < 50; ++i) {
        fPercentages[i] = fcurrent / 99.0;
        fPercentages[99 - i] = (99.0 - fcurrent) / 99.0;
        fcurrent = fcurrent + fincrement;
        fincrement = fincrement + 0.04f;
    }
    for (int32_t j = 0; j < 100; ++j) {
        fPercentages[j] = FLerp(afStart, afEnd, 0.0f, 1.0f, fPercentages[j]);
    }
    if (!std_time_original) {
        return false;
    }
    fCurrent = afStart;
    fEnd = afEnd;
    fTotalTime = afTime;
    fRemainingTime = afTime;
    return true;
}

float NonLinearInterpolator::GetCurrent() {
    float fProgress = (fTotalTime - BSMAX(fRemainingTime, 0.0f)) / fTotalTime * 99.0;
    int32_t iIndex = static_cast<int32_t>(std::floor(fProgress));
    if (iIndex < 99) {
        if (fRemainingTime <= 0.0) {
            return fPercentages[99];
        }
        float fElapsed = fTotalTime - fRemainingTime;
        return FLerp(fPercentages[iIndex], fPercentages[iIndex + 1], 0.0, fTotalTime, fElapsed);
    }

    return fPercentages[99];
}

CompassTarget::CompassTarget() : pTarget(nullptr), bIsHostile(false), bIsUndetected(false) {}

// Orders the compass targets by their squared distance from the player: 1 when apTarget2's actor is nearer, -1 when
// it is farther, 0 when they are as far or either target is empty.
int32_t CompassTarget::CompareForSorting(CompassTarget* apTarget1, CompassTarget* apTarget2) {
    if (!apTarget1 || !apTarget1->pTarget || !apTarget2 || !apTarget2->pTarget) {
        return 0;
    }
    NiPoint3 kPlayerPos = pPlayer->GetLocationOnReference();
    NiPoint3 kPos1 = apTarget1->pTarget->GetLocationOnReference();
    float fDist1 = std::fabs((kPos1 - kPlayerPos).SqrLength());
    NiPoint3 kPos2 = apTarget2->pTarget->GetLocationOnReference();
    float fDist2 = std::fabs((kPos2 - kPlayerPos).SqrLength());
    if (fDist2 < fDist1) {
        return 1;
    }
    if (fDist2 > fDist1) {
        return -1;
    }
    return 0;
}

PlayerActionObject::PlayerActionObject() : eAction(PLAYER_ACTION_NONE), ftimer(0.0f), prefObj(nullptr) {}

AudioMarkerInfo::AudioMarkerInfo() : pMarkerRef(nullptr), pMarkerData(nullptr) {}

// pMarkerData stays unset for a null reference, deliberately.
AudioMarkerInfo::AudioMarkerInfo(TESObjectREFR* apRef) : pMarkerRef(apRef) {
    if (apRef) {
        pMarkerData = apRef->GetExtraAudioMarker();
    }
}

PLAYER_TARGET_LOC::PLAYER_TARGET_LOC()
  : pWorld(nullptr), pInterior(nullptr), Location(NiPoint3::ZERO), Angle(NiPoint3::ZERO), bResetWeather(false),
    pfnArrivalFunc(nullptr), iArrivalFuncData(0), pFurnitureRef(nullptr), pFastTravelMarker(nullptr) {}

// The members are default-constructed (the lists, smart pointers, sound handles, perk entry list arrays and the
// character progression; the occupied region list with its flag 1). The field stores stay in the body, in the game's
// order, because they interleave with calls that read and change the same fields (SetCanFastTravel, the process swap,
// InitHavok, SetFOV, SetHardcore, CreateActorMover).
PlayerCharacter::PlayerCharacter() : Character(), m_AllOccupiedRegions(true) {
    RandomDoorSpaceMap.Init(37u);
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PLAYER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
        711
    );
    SetInitialized(false);
    Cached1stPersonCameraPos = NiPoint3::ZERO;
    CachedWorldCameraPos = NiPoint3::ZERO;
    InitGunWobble();
    ShakeData();
    kVanityModePos.y = fVanityModeWheelDefault.Float();
    bAiControlledActivate = false;
    for (int32_t i = 0; i < 77; ++i) {
        TemporaryActorValueModifiers[i] = 0.0f;
        ScriptActorValueModifiers[i] = 0.0f;
        DamageActorValueModifiers[i] = 0.0f;
    }
    fTimeSinceLastAmmoRegenTick = 0.0f;
    cShotsSinceLastAmmoRegen = 0;
    fHealthModifier = 0.0f;
    iSleepTime = 0;
    fsecondRunning = 0.0f;
    fsecondSwimming = 0.0f;
    fsecondSneaking = 0.0f;
    iAmountStolenSold = 0;
    iNumAdvance = 0;
    btransporting = false;
    bCanFastTravel = 0;
    SetCanFastTravel(true);
    bCanWait = true;
    bChargen = false;
    bAllowEGMCacheClear = true;
    eskilladvance = ActorValue::ACTOR_VALUE_COUNT;
    pDefaultClass = nullptr;
    pListofActions = nullptr;
    pAutoAimActor = nullptr;
    BulletAutoAim = NiPoint3::ZERO;
    pPlayersTargetActor = nullptr;
    bAiControlledPackage = false;
    pClassBasedOn = nullptr;
    bTravelUseDoor = false;
    sp1stPersonLight = nullptr;
    sp3rdPersonLight = nullptr;
    bOnElevator = true;
    // The player runs in the high process from the start: Character's process is replaced by a new HighProcess.
    HighProcess* pHighProcess = new HighProcess();
    pHighProcess->Copy(pCurrentProcess);
    ReferenceProcessLists.RemoveReference(this, GetCurrentProcessType());
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = pHighProcess;
    ReferenceProcessLists.RemoveReference(this, GetCurrentProcessType());
    PlayerCharacter::SetCurrentSpell(nullptr);
    PlayerCharacter::SetDesiredTarget(nullptr);
    pActiveEffectList = new BSSimpleList<ActiveEffect*>();
    MobileObject::InitHavok();
    bhkCharacterController* pcon = GetCharController();
    if (pcon) {
        pcon->SetPriority(1000u);
        pcon->SetCollisionGroup(9u);
        CFilter icolFilter;
        pcon->GetCollisionFilter(&icolFilter);
        pCameraCaster = new CameraCaster(fCameraCasterSize.Float(), 9u);
    }
    else {
        pCameraCaster = nullptr;
    }
    b3rdPersonSaved = false;
    bsave3rdPerson = false;
    b3rdPerson = false;
    bActually3rdPerson = false;
    bWant3rdPerson = false;
    bTemp3rdPerson = false;
    bTemp3rdPersonSwitchBack = false;
    bTemp1stPerson = false;
    bTemp1stPersonSwitchBack = false;
    bAlwaysRun = bAlwaysRunByDefault.Bool();
    bAutoMove = false;
    SetFOV(BSSceneGraph::fDefaultFOV.Float());
    p1stPersonBipedAnim = nullptr;
    p1stPersonAnimation = nullptr;
    sp1stPerson3D = nullptr;
    fEyeHeight = 118.0f;
    spInventoryPC = nullptr;
    pInventoryAnimation = nullptr;
    chaseCameraPos = NiPoint3::ZERO;
    fChaseDistance = 0.0f;
    bSnapChaseCamera = false;
    bGreetingPlayer = false;
    ucControlsDisabled = 0;
    pClosestConversation = nullptr;
    bBlockActivate = false;
    fBlockActivateTimer = 5.0f;
    ihourstosleep = 0;
    fSitHeadingDelta = 0.0f;
    cMurder = 0;
    bBeenAttacked = 0;
    pSelectedSpell = nullptr;
    pSelectedScroll = nullptr;
    pPlayerMapMarker = nullptr;
    pActiveQuest = nullptr;
    EatDrinkItems = new BSSimpleList<MagicItem*>();
    QueuedWornEnchantments = nullptr;
    iNumberTraining = 0;
    SetForceActivateRef(nullptr);
    bIsSleeping = false;
    pPendingPoison = nullptr;
    bTelekinesisSelected = false;
    pOccupiedRegion = nullptr;
    spGrabSpring = nullptr;
    pGrabbedObject = nullptr;
    eGrabType = PlayerCharacter::GRAB_NONE;
    fGrabObjectWeight = 0.0f;
    fGrabDistance = 0.0f;
    pInitialStateBuffer = nullptr;
    iTotalPlayingTime = 0;
    ResetLastPlayingTimeUpdate();
    for (uint32_t& uiCrimeCount : pCrimeCounts) {
        uiCrimeCount = 0;
    }
    iCharacterSeed = std::time(nullptr);
    SetAIControlledToPosition(false);
    SetAIControlledFromPosition(false);
    bInBorderContainedCell = false;
    bReturnToLastKnownGoodPosition = false;
    LastKnownGoodPosition = NiPoint3::ZERO;
    pLastKnownGoodLocation = nullptr;
    pLastKnownMusicType = nullptr;
    pBorderRegions = nullptr;
    pListofCasinoData = new BSSimpleList<CasinoData*>();
    pInactiveListofCaravanCards = new BSSimpleList<TESCaravanCard*>();
    pActiveListofCaravanCards = new BSSimpleList<TESCaravanCard*>();
    iCaravanCapWinnings = 0;
    iCaravanCapLosses = 0;
    iCaravanWinnings = 0;
    iCaravanLosses = 0;
    iCaravanLargestWinning = 0;
    iCasinoCheatLevel = 0;
    SetHardcore(GamePlayFormulas::HARDCORE_OFF, false);
    bAlwaysHardcore = true;
    eDifficultyLevel = static_cast<GamePlayFormulas::DifficultyLevel>(iDifficulty.Int());
    eKillCameraSetting = static_cast<KILL_CAM_TYPE>(iKillCamera.Int());
    pInventoryWeaponEffect = nullptr;
    bBeingChased = false;
    bIsYoung = false;
    bIsToddler = false;
    bCanUsePowerArmor = false;
    iCombatPersue = 0;
    pMapWorld = nullptr;
    fUFOCameraHeading = 0.0f;
    fUFOCameraPitch = 0.0f;
    UFOCameraPos = NiPoint3::ZERO;
    bShowQuestItemsInInventory = true;
    iSelectedSpellCastSoundID = 0;
    SelectedSpellCastSound = BSSoundHandle();
    for (BSSoundHandle& rSound : MagicFailureSounds) {
        rSound = BSSoundHandle();
    }
    pLastExtDoorActivated = nullptr;
    DroppedRefList.RemoveAll();
    ResetInsufficientChargeMessage();
    fSortActorDistanceListTimer = 0.0f;
    fDropAngleMod = 0.0f;
    fLastDropAngleMod = 0.0f;
    iFormFlags |= kFormFlags_IsRefPersistent;
    fWorldFOV = BSSceneGraph::fDefaultWorldFOV.Float();
    f3rdPersonFOV = fOverShoulderFOV.Float();
    f1stPersonFOV = BSSceneGraph::fDefault1stPersonFOV.Float();
    pCurrentProcess->SetSustainedFire(false);
    PlayerCharacter::CreateActorMover();
    fProjectileReleaseTimer = 0.0f;
    iNumberofStealWarnings = 0;
    fStealWarningTimer = -1.0f;
    iNumberofPickpocketWarnings = 0;
    fPickPocketWarningTimer = -1.0f;
    pListofPercievedActors = nullptr;
    bMenuModeButtonClicked = false;
    fMenuModeButtonTimer = 0.0f;
    fAmmoSwapButtonTimer = 0.0f;
    pIronsightsDOFInstance = nullptr;
    pVatsDOFInstance = nullptr;
    pVatsDRBInstance = nullptr;
    kCamera3rdPersonShoulderOffset = NiPoint3::ZERO;
    bSpeaking = false;
    pQueuedTargetLoc = nullptr;
    pQueuedWeaponAttach = nullptr;
    pCombatGroup = nullptr;
    iTeammateCount = 0;
    bPlayerInCombat = false;
    bAllCombatTargetsSearching = false;
    fCombatTimer = 0.0f;
    fYieldTimer = 0.0f;
    spCameraRigidBody = nullptr;
    SetEncounterZone(BGSEncounterZone::GetNoZone());
    SetMegatonDestroyed(false);
    pHeadingInterpolator = new NonLinearInterpolator();
    pLookingInterpolator = new NonLinearInterpolator();
    ourMemset(pWobbleNodes, 0, sizeof(pWobbleNodes));
    ResetRockItAmmoWeight(false);
    pAIConversationRunning = nullptr;
    bHostileDetection = false;
    iSandmanDetection = 0;
    spTargeted3D = nullptr;
    bTarget3DDistant = false;
    bNightVisionOn = false;
    bTurboISM = false;
    pReputationUpdate = nullptr;
    fTimeInSlowMoCam = 0.0f;
    fKillCamCooldown = 0.0f;
    bIgnoresGTM = false;
    pContinuousBeamPersistant = nullptr;
    bIsAcousticSpaceTransition = false;
    fLastHelloTime = -1.0f;
    for (AchievementInstance& rAchievement : PlayerAchievements) {
        rAchievement.bUnlocked = false;
        rAchievement.iCurrentValue = 0;
        // The stored value is ~(cFlags & 2) != 0, which is always true, so bTracking is set to true.
        rAchievement.bTracking = true;
    }
    pClosestAudioMarkerInfo = nullptr;
    fCounterAttackTimer = 0.0f;
    bCounterAttackCamera = false;
    bHasCateyeActive = false;
    bHasSpotterActive = false;
    fCheckForItems = 0.0f;
    SetWeaponSightingNode(nullptr);
    for (int32_t n = 0; n < 8; ++n) {
        HotKeyLastAmmo.Add(nullptr);
    }
    patch04DebugQuest = nullptr;
}

// The members are destroyed by the compiler after this body (the character progression, the lists, the smart pointers,
// the sound handles and the perk entry list arrays).
PlayerCharacter::~PlayerCharacter() {
    ClearAudioMarkerList();
    ClearMapMarkerStructList();
    m_AllOccupiedRegions.RemoveAll();
    FreeGunWobble();
    FreeCameraShake();
    CloneInventory3D(false);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = nullptr;
    if (EatDrinkItems) {
        EatDrinkItems->RemoveAll();
        delete EatDrinkItems;
    }
    if (QueuedWornEnchantments) {
        QueuedWornEnchantments->RemoveAll();
        delete QueuedWornEnchantments;
    }
    if (pCameraCaster) {
        delete pCameraCaster;
    }
    spGrabSpring = nullptr;
    if (pSelectedSpell) {
        pSelectedSpell->Unload(true);
    }
    if (pActiveEffectList) {
        while (pActiveEffectList && !pActiveEffectList->IsEmpty()) {
            if (pActiveEffectList->m_item) {
                delete pActiveEffectList->m_item;
            }
            pActiveEffectList->RemoveHead();
        }
        delete pActiveEffectList;
    }
    if (pListofActions) {
        while (pListofActions && pListofActions->m_item) {
            ourFree(pListofActions->m_item);
            pListofActions->RemoveHead();
        }
        delete pListofActions;
    }
    if (pListofCasinoData && !pListofCasinoData->IsEmpty()) {
        while (pListofCasinoData && pListofCasinoData->m_item) {
            ourFree(pListofCasinoData->m_item);
            pListofCasinoData->RemoveHead();
        }
        delete pListofCasinoData;
    }
    if (pInactiveListofCaravanCards && !pInactiveListofCaravanCards->IsEmpty()) {
        while (pInactiveListofCaravanCards && pInactiveListofCaravanCards->m_item) {
            delete pInactiveListofCaravanCards->m_item;
            pInactiveListofCaravanCards->RemoveHead();
        }
        delete pInactiveListofCaravanCards;
    }
    if (pActiveListofCaravanCards && !pActiveListofCaravanCards->IsEmpty()) {
        while (pActiveListofCaravanCards && pActiveListofCaravanCards->m_item) {
            delete pActiveListofCaravanCards->m_item;
            pActiveListofCaravanCards->RemoveHead();
        }
        delete pActiveListofCaravanCards;
    }
    Actor::DestroyActorMover();
    listTopics.RemoveAll();
    listQuestLog.RemoveAll();
    listQuestTargets.RemoveAll();
    listObjectives.RemoveAll();
    FreeCompassTargets();
    ClearRockItAmmo();
    if (pInventoryWeaponEffect) {
        delete pInventoryWeaponEffect;
    }
    PlayerCharacter::Set3D(nullptr, true);
    if (pInitialStateBuffer) {
        pSaveLoadGameOLD->DestroyBuffer(pInitialStateBuffer);
    }
    if (pBorderRegions) {
        pBorderRegions->RemoveAll();
        delete pBorderRegions;
        pBorderRegions = nullptr;
    }
    ResetMagicCastSound();
    for (int32_t i = 0; i < 6; ++i) {
        if (MagicFailureSounds[i].IsValid()) {
            MagicFailureSounds[i].Stop();
            MagicFailureSounds[i] = BSSoundHandle();
        }
    }
    DroppedRefList.RemoveAll();
    SetLightPointer(nullptr, true);
    SetLightPointer(nullptr, false);
    kConflictingPick.SetTarget(nullptr);
    kConflictingPick.PickObjects(NiPoint3::ZERO, NiPoint3::ZERO, false);
    for (BSSimpleList<PerkRankData*>* pPerkRankList = &Perks; pPerkRankList; pPerkRankList = pPerkRankList->m_pkNext) {
        ourFree(pPerkRankList->m_item);
    }
    Perks.RemoveAll();
    for (uint32_t j = 0; j < 74; ++j) {
        PerkEntryLists[j].RemoveAll();
    }
    for (BSSimpleList<PerkRankData*>* pPerkRankList = &CompanionPerks; pPerkRankList;
         pPerkRankList = pPerkRankList->m_pkNext) {
        ourFree(pPerkRankList->m_item);
    }
    CompanionPerks.RemoveAll();
    for (uint32_t k = 0; k < 74; ++k) {
        CompanionPerkEntryLists[k].RemoveAll();
    }
    listNotes.RemoveAll();
    StopIronSightsImageSpace();
    StopVatsImageSpace();
    RemovePlayerMapMarker();
    ourFree(pHeadingInterpolator);
    ourFree(pLookingInterpolator);
}

uint16_t PlayerCharacter::GetSaveSize(uint32_t aiFlags) {
    int16_t sModifierSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        sModifierSize = 6;
    }
    sModifierSize = sModifierSize + 616;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 49u) {
        sModifierSize += 308;
    }
    uint16_t usSize = sModifierSize + 4;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                usSize,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                0x3248,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize,
                0x3248,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp"
            );
        }
    }
    usSize = Character::GetSaveSize(aiFlags) + usSize;
    uint16_t usBlockStart = usSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        usSize += 6;
    }
    if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
        usSize += ComputeAnimationSaveSize(this, p1stPersonAnimation);
    }
    usSize = usSize + 84;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 40u && pSaveLoadGameOLD->GetCurrentVersion() < 45u) {
        usSize += 24;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 57u) {
        usSize += 192;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 63u) {
        ++usSize;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
        usSize += 12;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 73u) {
        usSize += 5;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        usSize += 10;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 120u) {
        ++usSize;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
        usSize += 4;
    }
    usSize = usSize + 40;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 40u && pSaveLoadGameOLD->GetCurrentVersion() < 45u) {
        usSize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
        usSize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 66u) {
        usSize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 87u) {
        usSize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 96u) {
        usSize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 99u) {
        usSize += 2;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 108u) {
        usSize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 111u) {
        usSize += 5 * RandomDoorSpaceMap.m_kAllocator.m_uiCount + 2;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 115u) {
        usSize += 2 + 4 * ConflictingObjects.ItemsInList();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
        usSize += 2 + 8 * pListofCasinoData->ItemsInList();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
        int16_t sCardsSize = usSize + 2 + 4 * pInactiveListofCaravanCards->ItemsInList() + 2;
        usSize = sCardsSize + 4 * pActiveListofCaravanCards->ItemsInList() + 20;
    }
    int16_t sTotalSize = ActiveEffect::GetActiveEffectListSaveSize(pActiveEffectList, this) + usSize;
    int16_t sProgressionSize = CharacterProgression::GetSaveSize(aiFlags);
    sTotalSize = sProgressionSize + sTotalSize + 6 + 4 * listTopics.ItemsInList() + 2;
    sTotalSize = 6 * listQuestLog.ItemsInList() + sTotalSize;
    sTotalSize = 5 * listObjectives.ItemsInList() + sTotalSize;
    sTotalSize = EffectSettingCollection::GetKnownEffectsSaveSize() + sTotalSize;
    sTotalSize = static_cast<TESNPC*>(GetObjectReference())->TESNPC::GetFaceGenSaveSize(this) + sTotalSize;
    const char* pFullName = GetFullName();
    usSize = sTotalSize + static_cast<uint8_t>(std::strlen(pFullName) + 1) + 1;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 44u) {
        TESClass* pClass = static_cast<TESNPC*>(GetActorBase())->TESNPC::GetClass();
        uint32_t iCustomClass = iPlayerCustomClass.Int();
        TESClass* pCustomClass = pDataHandler->GetClass(iCustomClass);
        usSize += 4;
        if (pClass && pClass == pCustomClass) {
            usSize += pClass->GetSaveSize();
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
        usSize += 4;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                usSize - usBlockStart,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                0x3365,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize - usBlockStart,
                0x3365,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp"
            );
        }
    }
    return usSize;
}

// Writes the player's state: its actor value modifiers in one save block, Character's state, then its own fields,
// lists and maps in a second block.
void PlayerCharacter::SaveGame(uint32_t aiFlags) {
    uint16_t sBlockSize = 0;
    char* pBlockStart = nullptr;
    char* pStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t iBlockID = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockID, sizeof(iBlockID));
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&sBlockSize, sizeof(sBlockSize));
    }
    SaveGameDataOLD(TemporaryActorValueModifiers, sizeof(TemporaryActorValueModifiers));
    SaveGameDataOLD(ScriptActorValueModifiers, sizeof(ScriptActorValueModifiers));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 49u) {
        SaveGameDataOLD(DamageActorValueModifiers, sizeof(DamageActorValueModifiers));
    }
    SaveGameDataOLD(&fHealthModifier, sizeof(fHealthModifier));
    if (bDebugSaveBuffer.Bool()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            uint32_t iFlags = pHeader->iFlags;
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart),
                pHeader->iFormID,
                pEditorID,
                iFlags,
                13176,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart),
                13176,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        char* pBlockDataStart = pBlockStart;
        if (pEnd > pBlockDataStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                13176
            );
        }
        // The save buffer is untyped bytes: the block's size is stored as a short where the placeholder went.
        *reinterpret_cast<uint16_t*>(pBlockStart) = static_cast<uint16_t>(pEnd - pBlockDataStart);
    }
    Character::SaveGame(aiFlags);
    uint16_t sPlayerBlockSize = 0;
    char* pPlayerBlockStart = nullptr;
    char* pPlayerStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pPlayerStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t iBlockID = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockID, sizeof(iBlockID));
        pPlayerBlockStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&sPlayerBlockSize, sizeof(sPlayerBlockSize));
    }
    if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
        Animation::SaveAnimation(this, p1stPersonAnimation);
    }
    const NiPoint3* pMarkerPos = pPlayerMapMarker ? &pPlayerMapMarker->GetLocationOnReference() : &NiPoint3::ZERO;
    float afMarkerPos[3];
    afMarkerPos[0] = pMarkerPos->x;
    afMarkerPos[1] = pMarkerPos->y;
    afMarkerPos[2] = pMarkerPos->z;
    SaveGameDataOLD(&b3rdPerson, sizeof(b3rdPerson));
    SaveGameDataOLD(&bTemp3rdPerson, sizeof(bTemp3rdPerson));
    SaveGameDataOLD(&bAlwaysRun, sizeof(bAlwaysRun));
    SaveGameDataOLD(&bAutoMove, sizeof(bAutoMove));
    SaveGameDataOLD(&iSleepTime, sizeof(iSleepTime));
    SaveGameDataOLD(&fsecondRunning, sizeof(fsecondRunning));
    SaveGameDataOLD(&fsecondSwimming, sizeof(fsecondSwimming));
    SaveGameDataOLD(&fsecondSneaking, sizeof(fsecondSneaking));
    SaveGameDataOLD(&bActorinSneakRange, sizeof(bActorinSneakRange));
    SaveGameDataOLD(&bGreetingPlayer, sizeof(bGreetingPlayer));
    SaveGameDataOLD(&fGreetTimer, sizeof(fGreetTimer));
    SaveGameDataOLD(&ihourstosleep, sizeof(ihourstosleep));
    SaveGameDataOLD(&cMurder, sizeof(cMurder));
    SaveGameDataOLD(&iAmountStolenSold, sizeof(iAmountStolenSold));
    SaveGameDataOLD(&bBeenAttacked, sizeof(bBeenAttacked));
    SaveGameDataOLD(&fSitHeadingDelta, sizeof(fSitHeadingDelta));
    SaveGameDataOLD(afMarkerPos, sizeof(afMarkerPos));
    SaveGameDataOLD(&fEyeHeight, sizeof(fEyeHeight));
    SaveGameDataOLD(&iNumberTraining, sizeof(iNumberTraining));
    SaveGameDataOLD(&iNumAdvance, sizeof(iNumAdvance));
    pSaveLoadGameOLD->SaveGameDataOLD(&eskilladvance, sizeof(eskilladvance));
    SaveGameDataOLD(&bIsSleeping, sizeof(bIsSleeping));
    SaveGameDataOLD(&fFOV, sizeof(fFOV));
    SaveGameDataOLD(&bChargen, sizeof(bChargen));
    SaveGameDataOLD(&bTelekinesisSelected, sizeof(bTelekinesisSelected));
    SaveGameDataOLD(&fProjectileReleaseTimer, sizeof(fProjectileReleaseTimer));
    SaveGameDataOLD(&iTotalPlayingTime, sizeof(iTotalPlayingTime));
    SaveGameDataOLD(&ucControlsDisabled, sizeof(ucControlsDisabled));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 40u && pSaveLoadGameOLD->GetCurrentVersion() < 45u) {
        SaveGameDataOLD(&LastKnownGoodPosition, sizeof(LastKnownGoodPosition));
        SaveGameDataOLD(&LastKnownGoodPosition, sizeof(LastKnownGoodPosition));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 57u) {
        int32_t pMiscStats[44];
        MiscStatManager::GetAll(pMiscStats);
        SaveGameDataOLD(pMiscStats, 172);
        SaveGameDataOLD(pCrimeCounts, sizeof(pCrimeCounts));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 63u) {
        SaveGameDataOLD(&bBeingChased, sizeof(bBeingChased));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&eGrabType, sizeof(eGrabType));
        SaveGameDataOLD(&fGrabObjectWeight, sizeof(fGrabObjectWeight));
        SaveGameDataOLD(&fGrabDistance, sizeof(fGrabDistance));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 73u) {
        SaveGameDataOLD(&iCombatPersue, sizeof(iCombatPersue));
        SaveGameDataOLD(&bShowQuestItemsInInventory, sizeof(bShowQuestItemsInInventory));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        SaveGameDataOLD(&bTemp3rdPersonSwitchBack, sizeof(bTemp3rdPersonSwitchBack));
        SaveGameDataOLD(&bCanFastTravel, sizeof(bCanFastTravel));
        SaveGameDataOLD(&iCharacterSeed, sizeof(iCharacterSeed));
        SaveNumericID(&iSelectedSpellCastSoundID, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 120u) {
        bool bIronSights = GetIronSights();
        SaveGameDataOLD(&bIronSights, sizeof(bIronSights));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
        SaveGameDataOLD(&PlayerCharacter::kVanityModePos.y, sizeof(PlayerCharacter::kVanityModePos.y));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
        uint32_t iCasinoChipID = 0;
        BSSimpleList<CasinoData*>* pCasinoList = GetCasinoDataList();
        int32_t iListSize = pCasinoList->ItemsInList();
        SaveGameDataOLD(&iListSize, sizeof(iListSize));
        while (pCasinoList && pCasinoList->m_item) {
            iCasinoChipID = pCasinoList->m_item->casinoID;
            SaveGameDataOLD(&iCasinoChipID, sizeof(iCasinoChipID));
            SaveGameDataOLD(&pCasinoList->m_item->iTotalWinnings, sizeof(pCasinoList->m_item->iTotalWinnings));
            SaveGameDataOLD(&pCasinoList->m_item->iWinningLevel, sizeof(pCasinoList->m_item->iWinningLevel));
            pCasinoList = pCasinoList->m_pkNext;
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
        uint32_t iCardID = 0;
        BSSimpleList<TESCaravanCard*>* pCardList = GetCaravanCards1();
        int32_t iListSize = pCardList->ItemsInList();
        SaveGameDataOLD(&iListSize, sizeof(iListSize));
        while (pCardList && pCardList->m_item) {
            iCardID = pCardList->m_item->iFormID;
            SaveGameDataOLD(&iCardID, sizeof(iCardID));
            pCardList = pCardList->m_pkNext;
        }
        pCardList = GetCaravanCards2();
        iListSize = pCardList->ItemsInList();
        SaveGameDataOLD(&iListSize, sizeof(iListSize));
        while (pCardList && pCardList->m_item) {
            iCardID = pCardList->m_item->iFormID;
            SaveGameDataOLD(&iCardID, sizeof(iCardID));
            pCardList = pCardList->m_pkNext;
        }
        SaveGameDataOLD(&iCaravanCapWinnings, sizeof(iCaravanCapWinnings));
        SaveGameDataOLD(&iCaravanCapLosses, sizeof(iCaravanCapLosses));
        SaveGameDataOLD(&iCaravanWinnings, sizeof(iCaravanWinnings));
        SaveGameDataOLD(&iCaravanLosses, sizeof(iCaravanLosses));
        SaveGameDataOLD(&iCaravanLargestWinning, sizeof(iCaravanLargestWinning));
    }
    uint32_t iConversationID = 0;
    if (pClosestConversation) {
        iConversationID = pClosestConversation->iFormID;
    }
    SaveNumericID(&iConversationID, 4);
    uint32_t iPrisonMarkerID = 0;
    if (pPrisonMarker) {
        iPrisonMarkerID = pPrisonMarker->iFormID;
    }
    SaveNumericID(&iPrisonMarkerID, 4);
    uint32_t iSelectedSpellID = 0;
    if (pSelectedSpell) {
        SpellItem* pSpell = dynamic_cast<SpellItem*>(pSelectedSpell);
        if (pSpell) {
            iSelectedSpellID = pSpell->iFormID;
        }
    }
    SaveNumericID(&iSelectedSpellID, 4);
    uint32_t iCurrentSpellID = 0;
    if (pCurrentSpell) {
        iCurrentSpellID = pCurrentSpell->GetMagicItemFormID();
    }
    SaveNumericID(&iCurrentSpellID, 4);
    uint32_t iDesiredTargetID = 0;
    if (pDesiredTarget) {
        iDesiredTargetID = pDesiredTarget->GetMagicTargetFormID();
    }
    SaveNumericID(&iDesiredTargetID, 4);
    uint32_t iOccupiedRegionID = 0;
    if (pOccupiedRegion) {
        iOccupiedRegionID = pOccupiedRegion->iFormID;
    }
    SaveNumericID(&iOccupiedRegionID, 4);
    uint32_t iSelectedScrollID = 0;
    if (pSelectedScroll) {
        iSelectedScrollID = pSelectedScroll->iFormID;
    }
    SaveNumericID(&iSelectedScrollID, 4);
    uint32_t iDefaultClassID = 0;
    if (pDefaultClass) {
        iDefaultClassID = pDefaultClass->iFormID;
    }
    SaveNumericID(&iDefaultClassID, 4);
    uint32_t iPendingPoisonID = 0;
    if (pPendingPoison) {
        iPendingPoisonID = pPendingPoison->iFormID;
    }
    SaveNumericID(&iPendingPoisonID, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 40u && pSaveLoadGameOLD->GetCurrentVersion() < 45u) {
        uint32_t iLastKnownGoodLocationID = 0;
        if (pLastKnownGoodLocation) {
            iLastKnownGoodLocationID = pLastKnownGoodLocation->iFormID;
        }
        SaveNumericID(&iLastKnownGoodLocationID, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
        uint32_t iGrabbedObjectID = 0;
        if (pGrabbedObject) {
            iGrabbedObjectID = pGrabbedObject->iFormID;
        }
        SaveNumericID(&iGrabbedObjectID, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 66u) {
        uint32_t iPrisonContainerID = 0;
        if (PlayerCharacter::pPrisonContainer) {
            iPrisonContainerID = PlayerCharacter::pPrisonContainer->iFormID;
        }
        SaveNumericID(&iPrisonContainerID, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 87u) {
        uint32_t iLastDoorActivatedID = 0;
        if (pLastExtDoorActivated) {
            iLastDoorActivatedID = pLastExtDoorActivated->iFormID;
        }
        SaveNumericID(&iLastDoorActivatedID, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 96u) {
        uint32_t iSpaceID = 0;
        if (pPlayerMapMarker) {
            TESForm* pSpace = pPlayerMapMarker->GetWorldSpace();
            if (!pSpace) {
                pSpace = pPlayerMapMarker->GetParentCell();
            }
            if (pSpace) {
                iSpaceID = pSpace->iFormID;
            }
        }
        SaveNumericID(&iSpaceID, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 99u) {
        // Writes a count and walks a list that is always null here, so the count stays 0.
        uint16_t sCount = 0;
        uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveGameDataOLD(&sCount, sizeof(sCount));
        *pCount = sCount;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 108u) {
        uint32_t iOccupiedRegionWeatherID = 0;
        if (pOccupiedRegion && pOccupiedRegion->pCurrentWeather) {
            iOccupiedRegionWeatherID = pOccupiedRegion->pCurrentWeather->iFormID;
        }

        SaveNumericID(&iOccupiedRegionWeatherID, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 111u) {
        uint16_t sMapSize = static_cast<uint16_t>(RandomDoorSpaceMap.m_kAllocator.m_uiCount);
        SaveGameDataOLD(&sMapSize, sizeof(sMapSize));
        NiTMapItem<uint32_t, uint8_t>* pPos = RandomDoorSpaceMap.GetFirstPos();
        while (pPos) {
            uint32_t iFormID = 0;
            uint8_t cIndex = 0;
            RandomDoorSpaceMap.GetNext(pPos, iFormID, cIndex);
            SaveNumericID(&iFormID, 4);
            SaveGameDataOLD(&cIndex, sizeof(cIndex));
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 115u) {
        uint16_t sCount = 0;
        uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveGameDataOLD(&sCount, sizeof(sCount));
        for (BSSimpleList<TESObjectREFR*>* pList = &ConflictingObjects; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            TESObjectREFR* pRef = pList->m_item;
            uint32_t iRefID = 0;
            if (pRef) {
                iRefID = pRef->iFormID;
            }
            SaveNumericID(&iRefID, 4);
            ++sCount;
        }
        *pCount = sCount;
    }
    ActiveEffect::SaveActiveEffectList(pActiveEffectList, this);
    CharacterProgressionInfo.SaveGame(aiFlags);
    uint32_t iActiveQuestID = 0;
    if (pActiveQuest) {
        iActiveQuestID = pActiveQuest->iFormID;
    }
    SaveNumericID(&iActiveQuestID, 4);
    uint16_t sTopicCount = 0;
    uint16_t* pTopicCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    SaveGameDataOLD(&sTopicCount, sizeof(sTopicCount));
    for (BSSimpleList<TESTopic*>* pTopics = GetTopicList(); pTopics && !pTopics->IsEmpty();
         pTopics = pTopics->m_pkNext) {
        TESTopic* pTopic = pTopics->m_item;
        uint32_t iTopicID = pTopic->iFormID;
        SaveNumericID(&iTopicID, 4);
        ++sTopicCount;
    }
    *pTopicCount = sTopicCount;
    // listQuestLog is declared as BSSimpleList<void*>, but its items are TESQuestStageItem pointers. Each is saved as
    // its quest, its stage's index and its own index.
    uint16_t sLogCount = 0;
    uint16_t* pLogCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    SaveGameDataOLD(&sLogCount, sizeof(sLogCount));
    for (BSSimpleList<void*>* pStageItems = &listQuestLog; pStageItems && !pStageItems->IsEmpty();
         pStageItems = pStageItems->m_pkNext) {
        TESQuestStageItem* pStageItem = static_cast<TESQuestStageItem*>(pStageItems->m_item);
        TESQuest* pQuest = pStageItem->m_pOwner;
        uint32_t iQuestID = pQuest->iFormID;
        uint8_t cStageIndex = 0;
        uint8_t cStageItemIndex = pStageItem->ucIndex;
        for (BSSimpleList<TESQuestStage*>* pStages = &pQuest->m_listStages; pStages && !pStages->IsEmpty();
             pStages = pStages->m_pkNext) {
            TESQuestStage* pStage = pStages->m_item;
            if (pStageItem == pStage->GetStageItem(cStageItemIndex)) {
                cStageIndex = pStage->m_Data.ucIndex;
                break;
            }
        }
        SaveNumericID(&iQuestID, 4);
        SaveGameDataOLD(&cStageIndex, sizeof(cStageIndex));
        SaveGameDataOLD(&cStageItemIndex, sizeof(cStageItemIndex));
        ++sLogCount;
    }
    *pLogCount = sLogCount;
    uint16_t sObjectiveCount = 0;
    uint16_t* pObjectiveCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    SaveGameDataOLD(&sObjectiveCount, sizeof(sObjectiveCount));
    for (BSSimpleList<BGSQuestObjective*>* pObjectives = &listObjectives; pObjectives && !pObjectives->IsEmpty();
         pObjectives = pObjectives->m_pkNext) {
        BGSQuestObjective* pObjective = pObjectives->m_item;
        TESQuest* pObjectiveQuest = pObjective->pOwnerQuest;
        uint32_t iObjectiveQuestID = pObjectiveQuest->iFormID;
        uint32_t iObjectiveIndex = pObjective->iIndex;
        SaveNumericID(&iObjectiveQuestID, 4);
        SaveGameDataOLD(&iObjectiveIndex, sizeof(iObjectiveIndex));
        ++sObjectiveCount;
    }
    *pObjectiveCount = sObjectiveCount;
    TESNPC* pNPC = static_cast<TESNPC*>(GetObjectReference());
    pNPC->SaveFaceGen(this);
    const char* pFullName = GetFullName();
    uint8_t cLength = std::strlen(pFullName) + 1;
    SaveGameDataOLD(&cLength, sizeof(cLength));
    SaveGameDataOLD(pFullName, cLength);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 44u) {
        TESNPC* pBase = static_cast<TESNPC*>(GetActorBase());
        TESClass* pClass = pBase->GetClass();
        uint32_t iCustomClass = iPlayerCustomClass.Int();
        TESClass* pCustomClass = pDataHandler->GetClass(iCustomClass);
        uint32_t iClassID = 0;
        if (pClass) {
            iClassID = pClass->iFormID;
        }
        SaveNumericID(&iClassID, 4);
        if (pClass && pClass == pCustomClass) {
            pClass->SaveGame();
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
        uint32_t iClassBasedOnID = 0;
        if (pClassBasedOn) {
            iClassBasedOnID = pClassBasedOn->iFormID;
        }
        SaveNumericID(&iClassBasedOnID, 4);
    }
    if (bDebugSaveBuffer.Bool()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            uint32_t iFlags = pHeader->iFlags;
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pPlayerStart),
                pHeader->iFormID,
                pEditorID,
                iFlags,
                13714,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pPlayerStart),
                13714,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        char* pBlockDataStart = pPlayerBlockStart;
        if (pEnd > pBlockDataStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                13714
            );
        }
        // The save buffer is untyped bytes: the block's size is stored as a short where the placeholder went.
        *reinterpret_cast<uint16_t*>(pPlayerBlockStart) = static_cast<uint16_t>(pEnd - pBlockDataStart);
    }
}

void PlayerCharacter::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    int32_t iAmmoID;
    bool bAlwaysHardcoreDude;
    bool bHardcoreMode;
    uint32_t uiObjectiveIndex;
    uint8_t cStageIndex;
    uint8_t cStageItemIndex;
    uint32_t uiCombatGroupID;
    float fCameraShakeMult;
    float fCameraShakeCurrentTime;
    float afMarkerPos[3];
    uint32_t uiSize = 77;
    for (uint32_t i = 0; i < uiSize; ++i) {
        apSaveGameBuffer->SaveDataEndian(&TemporaryActorValueModifiers[i], sizeof(TemporaryActorValueModifiers[i]), 0);
    }
    for (uint32_t i = 0; i < uiSize; ++i) {
        apSaveGameBuffer->SaveDataEndian(&ScriptActorValueModifiers[i], sizeof(ScriptActorValueModifiers[i]), 0);
    }
    for (uint32_t i = 0; i < uiSize; ++i) {
        apSaveGameBuffer->SaveDataEndian(&DamageActorValueModifiers[i], sizeof(DamageActorValueModifiers[i]), 0);
    }
    apSaveGameBuffer->SaveDataEndian(&fHealthModifier, sizeof(fHealthModifier), 0);
    Character::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION)) {
        uint32_t isizePosition = apSaveGameBuffer->StartVariableSizedValue();
        uint32_t iposition1 = apSaveGameBuffer->iBufferPosition;
        p1stPersonAnimation->SaveGame(apSaveGameBuffer);
        uint32_t iposition2 = apSaveGameBuffer->iBufferPosition;
        apSaveGameBuffer->SaveVariableSizedValue(iposition2 - iposition1, isizePosition);
    }
    apSaveGameBuffer->SaveDataEndian(&b3rdPerson, sizeof(b3rdPerson), 0);
    apSaveGameBuffer->SaveDataEndian(&bTemp3rdPerson, sizeof(bTemp3rdPerson), 0);
    apSaveGameBuffer->SaveDataEndian(&bAlwaysRun, sizeof(bAlwaysRun), 0);
    apSaveGameBuffer->SaveDataEndian(&bAutoMove, sizeof(bAutoMove), 0);
    apSaveGameBuffer->SaveDataEndian(&iSleepTime, sizeof(iSleepTime), 0);
    apSaveGameBuffer->SaveDataEndian(&fsecondRunning, sizeof(fsecondRunning), 0);
    apSaveGameBuffer->SaveDataEndian(&fsecondSwimming, sizeof(fsecondSwimming), 0);
    apSaveGameBuffer->SaveDataEndian(&fsecondSneaking, sizeof(fsecondSneaking), 0);
    apSaveGameBuffer->SaveDataEndian(&bActorinSneakRange, sizeof(bActorinSneakRange), 0);
    apSaveGameBuffer->SaveDataEndian(&bGreetingPlayer, sizeof(bGreetingPlayer), 0);
    apSaveGameBuffer->SaveDataEndian(&fGreetTimer, sizeof(fGreetTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&ihourstosleep, sizeof(ihourstosleep), 0);
    apSaveGameBuffer->SaveDataEndian(&cMurder, sizeof(cMurder), 0);
    apSaveGameBuffer->SaveDataEndian(&iAmountStolenSold, sizeof(iAmountStolenSold), 0);
    apSaveGameBuffer->SaveDataEndian(&bBeenAttacked, sizeof(bBeenAttacked), 0);
    apSaveGameBuffer->SaveDataEndian(&bBlockActivate, sizeof(bBlockActivate), 0);
    apSaveGameBuffer->SaveDataEndian(&bIsYoung, sizeof(bIsYoung), 0);
    apSaveGameBuffer->SaveDataEndian(&bIsToddler, sizeof(bIsToddler), 0);
    const NiPoint3* pMarkerPos = pPlayerMapMarker ? &pPlayerMapMarker->GetLocationOnReference() : &NiPoint3::ZERO;
    afMarkerPos[0] = pMarkerPos->x;
    afMarkerPos[1] = pMarkerPos->y;
    afMarkerPos[2] = pMarkerPos->z;
    apSaveGameBuffer->SaveDataEndian(&fSitHeadingDelta, sizeof(fSitHeadingDelta), 0);
    apSaveGameBuffer->SaveDataEndian(afMarkerPos, sizeof(afMarkerPos), 0);
    apSaveGameBuffer->SaveDataEndian(&fEyeHeight, sizeof(fEyeHeight), 0);
    apSaveGameBuffer->SaveDataEndian(&iNumberTraining, sizeof(iNumberTraining), 0);
    apSaveGameBuffer->SaveDataEndian(&eskilladvance, sizeof(eskilladvance), 0);
    apSaveGameBuffer->SaveDataEndian(&bIsSleeping, sizeof(bIsSleeping), 0);
    apSaveGameBuffer->SaveDataEndian(&fFOV, sizeof(fFOV), 0);
    apSaveGameBuffer->SaveDataEndian(&f1stPersonFOV, sizeof(f1stPersonFOV), 0);
    apSaveGameBuffer->SaveDataEndian(&fWorldFOV, sizeof(fWorldFOV), 0);
    apSaveGameBuffer->SaveDataEndian(&bChargen, sizeof(bChargen), 0);
    apSaveGameBuffer->SaveDataEndian(&fProjectileReleaseTimer, sizeof(fProjectileReleaseTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&iTotalPlayingTime, sizeof(iTotalPlayingTime), 0);
    apSaveGameBuffer->SaveDataEndian(&ucControlsDisabled, sizeof(ucControlsDisabled), 0);
    apSaveGameBuffer->SaveDataEndian(&bBeingChased, sizeof(bBeingChased), 0);
    apSaveGameBuffer->SaveDataEndian(&eGrabType, sizeof(eGrabType), 0);
    apSaveGameBuffer->SaveDataEndian(&fGrabObjectWeight, sizeof(fGrabObjectWeight), 0);
    apSaveGameBuffer->SaveDataEndian(&fGrabDistance, sizeof(fGrabDistance), 0);
    apSaveGameBuffer->SaveDataEndian(&iCombatPersue, sizeof(iCombatPersue), 0);
    apSaveGameBuffer->SaveDataEndian(&bShowQuestItemsInInventory, sizeof(bShowQuestItemsInInventory), 0);
    apSaveGameBuffer->SaveDataEndian(&bTemp3rdPersonSwitchBack, sizeof(bTemp3rdPersonSwitchBack), 0);
    apSaveGameBuffer->SaveDataEndian(&bCanFastTravel, sizeof(bCanFastTravel), 0);
    apSaveGameBuffer->SaveDataEndian(&iCharacterSeed, sizeof(iCharacterSeed), 0);
    apSaveGameBuffer->SaveDataEndian(&PlayerCharacter::kVanityModePos.y, sizeof(PlayerCharacter::kVanityModePos.y), 0);
    apSaveGameBuffer->SaveDataEndian(&fCombatTimer, sizeof(fCombatTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&fYieldTimer, sizeof(fYieldTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&iNumberofStealWarnings, sizeof(iNumberofStealWarnings), 0);
    apSaveGameBuffer->SaveDataEndian(&fStealWarningTimer, sizeof(fStealWarningTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&iNumberofPickpocketWarnings, sizeof(iNumberofPickpocketWarnings), 0);
    apSaveGameBuffer->SaveDataEndian(&fPickPocketWarningTimer, sizeof(fPickPocketWarningTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&bSpeaking, sizeof(bSpeaking), 0);
    apSaveGameBuffer->SaveDataEndian(&bMegatonDestroyed, sizeof(bMegatonDestroyed), 0);
    apSaveGameBuffer->SaveDataEndian(&bTemp1stPerson, sizeof(bTemp1stPerson), 0);
    apSaveGameBuffer->SaveDataEndian(&bTemp1stPersonSwitchBack, sizeof(bTemp1stPersonSwitchBack), 0);
    apSaveGameBuffer->SaveDataEndian(&bCanUsePowerArmor, sizeof(bCanUsePowerArmor), 0);
    apSaveGameBuffer->SaveDataEndian(&bHostileDetection, sizeof(bHostileDetection), 0);
    apSaveGameBuffer->SaveDataEndian(&iSandmanDetection, sizeof(iSandmanDetection), 0);
    apSaveGameBuffer->SaveDataEndian(&fBlockActivateTimer, sizeof(fBlockActivateTimer), 0);
    for (uint32_t& uiCrimeCount : pCrimeCounts) {
        apSaveGameBuffer->SaveDataEndian(&uiCrimeCount, sizeof(uiCrimeCount), 0);
    }
    CharacterProgressionInfo.SaveGame(apSaveGameBuffer);
    TESForm* pPlayerMarkerSpace = nullptr;
    if (pPlayerMapMarker) {
        pPlayerMarkerSpace = pPlayerMapMarker->GetWorldSpace();
        if (!pPlayerMarkerSpace) {
            pPlayerMarkerSpace = pPlayerMapMarker->GetParentCell();
        }
    }
    apSaveGameBuffer->SaveFormID(pActiveQuest, 0);
    apSaveGameBuffer->SaveFormID(pDefaultClass, 0);
    apSaveGameBuffer->SaveFormID(pPlayerMarkerSpace, 0);
    apSaveGameBuffer->SaveFormID(pOccupiedRegion, 0);
    if (pOccupiedRegion) {
        apSaveGameBuffer->SaveFormID(pOccupiedRegion->pCurrentWeather, 0);
    }
    else {
        apSaveGameBuffer->SaveFormID(static_cast<TESForm*>(nullptr), 0);
    }
    apSaveGameBuffer->SaveFormID(pClosestConversation, 0);
    apSaveGameBuffer->SaveFormID(pAIConversationRunning, 0);
    apSaveGameBuffer->SaveFormID(pGrabbedObject, 0);
    apSaveGameBuffer->SaveFormID(pLastExtDoorActivated, 0);
    apSaveGameBuffer->SaveFormID(this->GetAutoAimActorSimple(), 0);
    apSaveGameBuffer->SaveFormID(pPlayersTargetActor, 0);
    uint32_t uiTopicCount = 0;
    uint32_t uiTopicCountPos = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<TESTopic*>* pList = GetTopicList(); pList; pList = pList->m_pkNext) {
        TESTopic* pTopic = pList->m_item;
        if (pTopic && !pTopic->IsRandom()) {
            apSaveGameBuffer->SaveFormID(pTopic, 0);
            ++uiTopicCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiTopicCount, uiTopicCountPos);
    uint32_t uiNoteCount = 0;
    uint32_t uiNoteCountPos = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<BGSNote*>* pList = &listNotes; pList; pList = pList->m_pkNext) {
        TESForm* pForm = pList->m_item;
        if (pForm) {
            apSaveGameBuffer->SaveFormID(pForm, 0);
            ++uiNoteCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiNoteCount, uiNoteCountPos);
    // The address &RockItLauncherAmmoList is never null, so it is not tested.
    uint32_t uiAmmoCount = RockItLauncherAmmoList.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(uiAmmoCount);
    for (uint32_t i = 0; i < uiAmmoCount; ++i) {
        RockItLauncherAmmoList[i]->SaveGame(apSaveGameBuffer);
    }
    uint32_t uiPerceivedCount = 0;
    uint32_t uiPerceivedCountPos = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<CompassTarget*>* pList = pListofPercievedActors; pList; pList = pList->m_pkNext) {
        CompassTarget* pActorsNearPlayerData = pList->m_item;
        if (pActorsNearPlayerData) {
            apSaveGameBuffer->SaveFormID(pActorsNearPlayerData->pTarget, 0);
            apSaveGameBuffer
                ->SaveDataEndian(&pActorsNearPlayerData->bIsHostile, sizeof(pActorsNearPlayerData->bIsHostile), 0);
            apSaveGameBuffer->SaveDataEndian(
                &pActorsNearPlayerData->bIsUndetected,
                sizeof(pActorsNearPlayerData->bIsUndetected),
                0
            );
            ++uiPerceivedCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiPerceivedCount, uiPerceivedCountPos);
    uint32_t uiPerkCount = 0;
    uint32_t uiPerkCountPos = apSaveGameBuffer->StartVariableSizedValue();
    for (PerkRankData* pPerkRank : Perks) {
        if (pPerkRank) {
            apSaveGameBuffer->SaveFormID(pPerkRank->pPerk, 0);
            apSaveGameBuffer->SaveDataEndian(&pPerkRank->cCurrentRank, sizeof(pPerkRank->cCurrentRank), 0);
            ++uiPerkCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiPerkCount, uiPerkCountPos);
    uint32_t uiActionCount = 0;
    uint32_t uiActionCountPos = apSaveGameBuffer->StartVariableSizedValue();
    for (PlayerActionObject* pPlayerAction : *pListofActions) {
        if (pPlayerAction) {
            apSaveGameBuffer->SaveDataEndian(pPlayerAction, 4u, 0);
            apSaveGameBuffer->SaveDataEndian(&pPlayerAction->ftimer, sizeof(pPlayerAction->ftimer), 0);
            apSaveGameBuffer->SaveFormID(pPlayerAction->prefObj, 0);
            ++uiActionCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiActionCount, uiActionCountPos);
    uint32_t uiCasinoCount = 0;
    uint32_t uiCasinoCountPos = apSaveGameBuffer->StartVariableSizedValue();
    for (CasinoData* pCasinoData : *pListofCasinoData) {
        if (pCasinoData) {
            apSaveGameBuffer->SaveDataEndian(pCasinoData, 4u, 0);
            apSaveGameBuffer->SaveDataEndian(&pCasinoData->iTotalWinnings, sizeof(pCasinoData->iTotalWinnings), 0);
            apSaveGameBuffer->SaveDataEndian(&pCasinoData->iWinningLevel, sizeof(pCasinoData->iWinningLevel), 0);
            ++uiCasinoCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiCasinoCount, uiCasinoCountPos);
    uint32_t uiInactiveCardCount = 0;
    uint32_t uiInactiveCardCountPos = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<TESCaravanCard*>* pList = pInactiveListofCaravanCards; pList; pList = pList->m_pkNext) {
        TESForm* pCaravanData = pList->m_item;
        if (pCaravanData) {
            apSaveGameBuffer->SaveFormID(pCaravanData, 0);
            ++uiInactiveCardCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiInactiveCardCount, uiInactiveCardCountPos);
    uint32_t uiActiveCardCount = 0;
    uint32_t uiActiveCardCountPos = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<TESCaravanCard*>* pList = pActiveListofCaravanCards; pList; pList = pList->m_pkNext) {
        TESForm* pCaravanData = pList->m_item;
        if (pCaravanData) {
            apSaveGameBuffer->SaveFormID(pCaravanData, 0);
            ++uiActiveCardCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiActiveCardCount, uiActiveCardCountPos);
    apSaveGameBuffer->SaveDataEndian(&iCaravanCapWinnings, sizeof(iCaravanCapWinnings), 0);
    apSaveGameBuffer->SaveDataEndian(&iCaravanCapLosses, sizeof(iCaravanCapLosses), 0);
    apSaveGameBuffer->SaveDataEndian(&iCaravanWinnings, sizeof(iCaravanWinnings), 0);
    apSaveGameBuffer->SaveDataEndian(&iCaravanLosses, sizeof(iCaravanLosses), 0);
    apSaveGameBuffer->SaveDataEndian(&iCaravanLargestWinning, sizeof(iCaravanLargestWinning), 0);
    uint32_t uiStageItemCount = 0;
    uint32_t uiStageItemCountPos = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<TESQuestStageItem*>* pList = GetQuestLog(); pList; pList = pList->m_pkNext) {
        TESQuestStageItem* pStageItem = pList->m_item;
        if (pStageItem) {
            TESQuest* pQuest = pStageItem->GetOwner();
            cStageIndex = pQuest->GetStageIndexForStageItem(pStageItem);
            cStageItemIndex = pStageItem->ucIndex;
            apSaveGameBuffer->SaveFormID(pQuest, 0);
            apSaveGameBuffer->SaveDataEndian(&cStageIndex, sizeof(cStageIndex), 0);
            apSaveGameBuffer->SaveDataEndian(&cStageItemIndex, sizeof(cStageItemIndex), 0);
            ++uiStageItemCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiStageItemCount, uiStageItemCountPos);
    uint32_t uiObjectiveCount = 0;
    uint32_t uiObjectiveCountPos = apSaveGameBuffer->StartVariableSizedValue();
    for (BGSQuestObjective* pObjective : listObjectives) {
        if (pObjective) {
            TESQuest* pQuest = pObjective->pOwnerQuest;
            uiObjectiveIndex = pObjective->iIndex;
            apSaveGameBuffer->SaveFormID(pQuest, 0);
            apSaveGameBuffer->SaveDataEndian(&uiObjectiveIndex, sizeof(uiObjectiveIndex), 0);
            ++uiObjectiveCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiObjectiveCount, uiObjectiveCountPos);
    ActiveEffect::SaveActiveEffectList(apSaveGameBuffer, pActiveEffectList);
    uiCombatGroupID = pCombatGroup ? pCombatGroup->iGroupID : 0;
    apSaveGameBuffer->SaveDataEndian(&uiCombatGroupID, sizeof(uiCombatGroupID), 0);
    fCameraShakeMult = GetCameraShakeMult();
    fCameraShakeCurrentTime = GetCameraShakeCurrentTime();
    apSaveGameBuffer->SaveDataEndian(&fCameraShakeMult, sizeof(fCameraShakeMult), 0);
    apSaveGameBuffer->SaveDataEndian(&fCameraShakeCurrentTime, sizeof(fCameraShakeCurrentTime), 0);
    uint32_t uiCompanionPerkCount = 0;
    uint32_t uiCompanionPerkCountPos = apSaveGameBuffer->StartVariableSizedValue();
    for (PerkRankData* pPerkRank : CompanionPerks) {
        if (pPerkRank) {
            apSaveGameBuffer->SaveFormID(pPerkRank->pPerk, 0);
            apSaveGameBuffer->SaveDataEndian(&pPerkRank->cCurrentRank, sizeof(pPerkRank->cCurrentRank), 0);
            ++uiCompanionPerkCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiCompanionPerkCount, uiCompanionPerkCountPos);
    bAlwaysHardcoreDude = GetAlwaysHardcore();
    bHardcoreMode = IsHardcore();
    apSaveGameBuffer->SaveDataEndian(&bHardcoreMode, sizeof(bHardcoreMode), 0);
    apSaveGameBuffer->SaveDataEndian(&bAlwaysHardcoreDude, sizeof(bAlwaysHardcoreDude), 0);
    apSaveGameBuffer->SaveDataEndian(&bCanWait, sizeof(bCanWait), 0);
    int32_t iHotKeyCount = 8;
    for (int32_t i = 0; i < iHotKeyCount; ++i) {
        TESAmmo* pAmmoHK = static_cast<TESAmmo*>(GetAmmoForHotKey(i));
        iAmmoID = 0;
        if (pAmmoHK) {
            iAmmoID = pAmmoHK->iFormID;
        }
        apSaveGameBuffer->SaveDataEndian(&iAmmoID, sizeof(iAmmoID), 0);
    }
    Script::SavePendingReferences(apSaveGameBuffer);
}

void PlayerCharacter::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    uint16_t iblockSize = 0;
    uint8_t* istartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t iblockHeader;
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        // 'BLOK'.
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                    13726,
                    pheader->iFormID,
                    pform->GetFormEditorID(),
                    pheader->cVersion,
                    pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                    13726,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 59u) {
        LoadGameDataOLD(TemporaryActorValueModifiers, sizeof(TemporaryActorValueModifiers));
        LoadGameDataOLD(ScriptActorValueModifiers, sizeof(ScriptActorValueModifiers));
        LoadGameDataOLD(DamageActorValueModifiers, sizeof(DamageActorValueModifiers));
    }
    else {
        LoadGameDataOLD(TemporaryActorValueModifiers, 304);
        LoadGameDataOLD(ScriptActorValueModifiers, 304);
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 49u) {
            LoadGameDataOLD(DamageActorValueModifiers, 304);
        }
    }
    LoadGameDataOLD(&fHealthModifier, sizeof(fHealthModifier));
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            if (icurrentBufferPosition <= istartBufferPosition + iblockSize) {
                if (icurrentBufferPosition < istartBufferPosition + iblockSize) {
                    uint32_t iFlags = pheader->iFlags;
                    int32_t cVersion = pheader->cVersion;
                    const char* pEditorID = pform->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(istartBufferPosition + iblockSize - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                        13750,
                        pheader->iFormID,
                        pEditorID,
                        cVersion,
                        iFlags
                    );
                }
            }
            else {
                uint32_t iFlags = pheader->iFlags;
                int32_t cVersion = pheader->cVersion;
                const char* pEditorID = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - (istartBufferPosition + iblockSize)),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                    13750,
                    pheader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else {
            if (icurrentBufferPosition <= istartBufferPosition + iblockSize) {
                if (icurrentBufferPosition < istartBufferPosition + iblockSize) {
                    int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        static_cast<int32_t>(istartBufferPosition + iblockSize - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                        13750,
                        iVersion
                    );
                }
            }
            else {
                int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    static_cast<int32_t>(icurrentBufferPosition - (istartBufferPosition + iblockSize)),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                    13750,
                    iVersion
                );
            }
        }
    }
    Character::LoadGame(aiFlags, aiCurrentFlags);
    uint16_t iplayerBlockSize = 0;
    uint8_t* iplayerStartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t iblockHeader;
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        // 'BLOK'.
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                    13754,
                    pheader->iFormID,
                    pform->GetFormEditorID(),
                    pheader->cVersion,
                    pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                    13754,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        iplayerStartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iplayerBlockSize, sizeof(iplayerBlockSize));
    }
    if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
        Animation::LoadAnimation(this, p1stPersonAnimation);
    }
    NiPoint3 playerMarkerLoc;
    LoadGameDataOLD(&b3rdPerson, sizeof(b3rdPerson));
    LoadGameDataOLD(&bTemp3rdPerson, sizeof(bTemp3rdPerson));
    LoadGameDataOLD(&bAlwaysRun, sizeof(bAlwaysRun));
    LoadGameDataOLD(&bAutoMove, sizeof(bAutoMove));
    LoadGameDataOLD(&iSleepTime, sizeof(iSleepTime));
    LoadGameDataOLD(&fsecondRunning, sizeof(fsecondRunning));
    LoadGameDataOLD(&fsecondSwimming, sizeof(fsecondSwimming));
    LoadGameDataOLD(&fsecondSneaking, sizeof(fsecondSneaking));
    LoadGameDataOLD(&bActorinSneakRange, sizeof(bActorinSneakRange));
    LoadGameDataOLD(&bGreetingPlayer, sizeof(bGreetingPlayer));
    LoadGameDataOLD(&fGreetTimer, sizeof(fGreetTimer));
    LoadGameDataOLD(&ihourstosleep, sizeof(ihourstosleep));
    LoadGameDataOLD(&cMurder, sizeof(cMurder));
    LoadGameDataOLD(&iAmountStolenSold, sizeof(iAmountStolenSold));
    LoadGameDataOLD(&bBeenAttacked, sizeof(bBeenAttacked));
    LoadGameDataOLD(&fSitHeadingDelta, sizeof(fSitHeadingDelta));
    LoadGameDataOLD(&playerMarkerLoc, sizeof(playerMarkerLoc));
    LoadGameDataOLD(&fEyeHeight, sizeof(fEyeHeight));
    LoadGameDataOLD(&iNumberTraining, sizeof(iNumberTraining));
    LoadGameDataOLD(&iNumAdvance, sizeof(iNumAdvance));
    pSaveLoadGameOLD->LoadGameDataOLD(&eskilladvance, sizeof(eskilladvance));
    LoadGameDataOLD(&bIsSleeping, sizeof(bIsSleeping));
    LoadGameDataOLD(&fFOV, sizeof(fFOV));
    LoadGameDataOLD(&bChargen, sizeof(bChargen));
    LoadGameDataOLD(&bTelekinesisSelected, sizeof(bTelekinesisSelected));
    LoadGameDataOLD(&fProjectileReleaseTimer, sizeof(fProjectileReleaseTimer));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 29u) {
        LoadGameDataOLD(&iTotalPlayingTime, sizeof(iTotalPlayingTime));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 34u) {
        LoadGameDataOLD(&ucControlsDisabled, sizeof(ucControlsDisabled));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 40u && pSaveLoadGameOLD->GetCurrentVersion() < 45u) {
        pSaveLoadGameOLD->AdvanceBuffer(1u);
        LoadGameDataOLD(&LastKnownGoodPosition, sizeof(LastKnownGoodPosition));
        LoadGameDataOLD(&LastKnownGoodPosition, sizeof(LastKnownGoodPosition));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 53u && pSaveLoadGameOLD->GetCurrentVersion() < 113u) {
        pSaveLoadGameOLD->AdvanceBuffer(4u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 57u) {
        // Reads 0xAC bytes of misc stats into a buffer of that size.
        int32_t pMiscStats[43];
        LoadGameDataOLD(pMiscStats, sizeof(pMiscStats));
        MiscStatManager::SetAll(pMiscStats);
        LoadGameDataOLD(pCrimeCounts, sizeof(pCrimeCounts));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 63u) {
        LoadGameDataOLD(&bBeingChased, sizeof(bBeingChased));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&eGrabType, sizeof(eGrabType));
        LoadGameDataOLD(&fGrabObjectWeight, sizeof(fGrabObjectWeight));
        LoadGameDataOLD(&fGrabDistance, sizeof(fGrabDistance));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 73u) {
        LoadGameDataOLD(&iCombatPersue, sizeof(iCombatPersue));
        LoadGameDataOLD(&bShowQuestItemsInInventory, sizeof(bShowQuestItemsInInventory));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 74u && pSaveLoadGameOLD->GetCurrentVersion() < 89u) {
        pSaveLoadGameOLD->AdvanceBuffer(8u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 89u && pSaveLoadGameOLD->GetCurrentVersion() < 90u) {
        pSaveLoadGameOLD->AdvanceBuffer(4u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        LoadGameDataOLD(&bTemp3rdPersonSwitchBack, sizeof(bTemp3rdPersonSwitchBack));
        LoadGameDataOLD(&bCanFastTravel, sizeof(bCanFastTravel));
        LoadGameDataOLD(&iCharacterSeed, sizeof(iCharacterSeed));
        LoadNumericID(&iSelectedSpellCastSoundID, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 120u) {
        bool bIsAiming;
        LoadGameDataOLD(&bIsAiming, sizeof(bIsAiming));
        SetIronSights(bIsAiming, false, false);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
        LoadGameDataOLD(&PlayerCharacter::kVanityModePos.y, sizeof(PlayerCharacter::kVanityModePos.y));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
        int32_t icount = 0;
        LoadGameDataOLD(&icount, sizeof(icount));
        for (int32_t i = 0; i < icount; ++i) {
            CasinoData* pcasinoData = new CasinoData;
            LoadGameDataOLD(pcasinoData, 4);
            LoadGameDataOLD(&pcasinoData->iTotalWinnings, sizeof(pcasinoData->iTotalWinnings));
            LoadGameDataOLD(&pcasinoData->iWinningLevel, sizeof(pcasinoData->iWinningLevel));
            pListofCasinoData->AddTail(pcasinoData);
        }
        if (uiAntiCheatTimer) {
            bAntiCheatActive = 1;
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
        int32_t icount = 0;
        uint32_t icardID = 0;
        pInactiveListofCaravanCards->RemoveAll();
        pActiveListofCaravanCards->RemoveAll();
        LoadGameDataOLD(&icount, sizeof(icount));
        for (int32_t i = 0; i < icount; ++i) {
            LoadGameDataOLD(&icardID, sizeof(icardID));
            TESCaravanCard* pcard = static_cast<TESCaravanCard*>(TESForm::GetFormByNumericID(icardID));
            pInactiveListofCaravanCards->AddTail(pcard);
        }
        LoadGameDataOLD(&icount, sizeof(icount));
        for (int32_t i = 0; i < icount; ++i) {
            LoadGameDataOLD(&icardID, sizeof(icardID));
            TESCaravanCard* pcard = static_cast<TESCaravanCard*>(TESForm::GetFormByNumericID(icardID));
            pActiveListofCaravanCards->AddTail(pcard);
        }
        LoadGameDataOLD(&iCaravanCapWinnings, sizeof(iCaravanCapWinnings));
        LoadGameDataOLD(&iCaravanCapLosses, sizeof(iCaravanCapLosses));
        LoadGameDataOLD(&iCaravanWinnings, sizeof(iCaravanWinnings));
        LoadGameDataOLD(&iCaravanLosses, sizeof(iCaravanLosses));
        LoadGameDataOLD(&iCaravanLargestWinning, sizeof(iCaravanLargestWinning));
    }
    // The reference fields below keep the saved form IDs until the load's later passes resolve them.
    uint32_t iconversationID;
    LoadNumericID(&iconversationID, 4);
    pClosestConversation = reinterpret_cast<DialoguePackage*>(static_cast<uintptr_t>(iconversationID));
    uint32_t iprisonMarkerID;
    LoadNumericID(&iprisonMarkerID, 4);
    pPrisonMarker = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(iprisonMarkerID));
    SetSelectedSpell(nullptr);
    uint32_t iselectedSpellID;
    LoadNumericID(&iselectedSpellID, 4);
    pSelectedSpell = reinterpret_cast<MagicItem*>(static_cast<uintptr_t>(iselectedSpellID));
    uint32_t icurrentSpellID;
    LoadNumericID(&icurrentSpellID, 4);
    pCurrentSpell = reinterpret_cast<MagicItem*>(static_cast<uintptr_t>(icurrentSpellID));
    uint32_t idesiredTargetID;
    LoadNumericID(&idesiredTargetID, 4);
    pDesiredTarget = reinterpret_cast<MagicTarget*>(static_cast<uintptr_t>(idesiredTargetID));
    uint32_t ioccupiedRegionID;
    LoadNumericID(&ioccupiedRegionID, 4);
    TESForm* pregionForm = TESForm::GetFormByNumericID(ioccupiedRegionID);
    TESRegion* pregion = dynamic_cast<TESRegion*>(pregionForm);
    SetOccupiedRegion(pregion);
    uint32_t iselectedScrollID;
    LoadNumericID(&iselectedScrollID, 4);
    pSelectedScroll = reinterpret_cast<TESObjectBOOK*>(static_cast<uintptr_t>(iselectedScrollID));
    uint32_t idefaultClassID;
    LoadNumericID(&idefaultClassID, 4);
    pDefaultClass = reinterpret_cast<TESClass*>(static_cast<uintptr_t>(idefaultClassID));
    uint32_t ipendingPoisonID;
    LoadNumericID(&ipendingPoisonID, 4);
    pPendingPoison = reinterpret_cast<AlchemyItem*>(static_cast<uintptr_t>(ipendingPoisonID));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 40u && pSaveLoadGameOLD->GetCurrentVersion() < 45u) {
        uint32_t ilastKnownGoodLocationID;
        LoadNumericID(&ilastKnownGoodLocationID, 4);
        pLastKnownGoodLocation = TESForm::GetFormByNumericID(ilastKnownGoodLocationID);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
        uint32_t igrabbedObjectID;
        LoadNumericID(&igrabbedObjectID, 4);
        pGrabbedObject = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(igrabbedObjectID));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 66u) {
        uint32_t iprisonContainerID;
        LoadNumericID(&iprisonContainerID, 4);
        PlayerCharacter::pPrisonContainer = reinterpret_cast<TESObjectREFR*>(
            static_cast<uintptr_t>(iprisonContainerID)
        );
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 87u) {
        uint32_t ilastDoorActivatedID;
        LoadNumericID(&ilastDoorActivatedID, 4);
        TESForm* pdoorForm = TESForm::GetFormByNumericID(ilastDoorActivatedID);
        pLastExtDoorActivated = dynamic_cast<TESObjectREFR*>(pdoorForm);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 96u) {
        uint32_t ispaceID;
        LoadNumericID(&ispaceID, 4);
        TESForm* pspace = TESForm::GetFormByNumericID(ispaceID);
        pPlayerMapMarker = nullptr;
        if (pspace) {
            SetPlayerMapMarker(playerMarkerLoc, pspace);
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 99u) {
        // Obsolete reference IDs: read and dropped.
        uint16_t icount;
        LoadGameDataOLD(&icount, sizeof(icount));
        for (uint32_t i = 0; i < icount; ++i) {
            uint32_t irefID;
            LoadNumericID(&irefID, 4);
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 108u) {
        uint32_t iweatherID;
        LoadNumericID(&iweatherID, 4);
        TESForm* pweatherForm = TESForm::GetFormByNumericID(iweatherID);
        TESWeather* poccupiedRegionWeather = dynamic_cast<TESWeather*>(pweatherForm);
        if (pOccupiedRegion && poccupiedRegionWeather) {
            pOccupiedRegion->pCurrentWeather = poccupiedRegionWeather;
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 111u) {
        RandomDoorSpaceMap.RemoveAll();
        uint16_t icount;
        LoadGameDataOLD(&icount, sizeof(icount));
        for (uint32_t i = 0; i < icount; ++i) {
            uint32_t iformID;
            uint8_t cvalue;
            LoadNumericID(&iformID, 4);
            LoadGameDataOLD(&cvalue, sizeof(cvalue));
            if (iformID) {
                RandomDoorSpaceMap.SetAt(iformID, cvalue);
            }
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 115u) {
        uint16_t icount;
        LoadGameDataOLD(&icount, sizeof(icount));
        for (uint32_t i = 0; i < icount; ++i) {
            uint32_t irefID;
            LoadNumericID(&irefID, 4);
            TESObjectREFR* pref = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(irefID));
            ConflictingObjects.AddHead(pref);
        }
    }
    ActiveEffect::LoadActiveEffectList(pActiveEffectList, this);
    CharacterProgressionInfo.LoadGame(aiFlags, aiCurrentFlags);
    uint32_t iactivequestID;
    LoadNumericID(&iactivequestID, 4);
    if (iactivequestID) {
        TESForm* pquestForm = TESForm::GetFormByNumericID(iactivequestID);
        pActiveQuest = dynamic_cast<TESQuest*>(pquestForm);
        if (pActiveQuest) {
            pActiveQuest->UpdateCurrentTargetList(&listQuestTargets, &listObjectives);
        }
    }
    else {
        pActiveQuest = nullptr;
    }
    listTopics.RemoveAll();
    uint16_t itopicCount;
    LoadGameDataOLD(&itopicCount, sizeof(itopicCount));
    for (int32_t i = 0; i < itopicCount; ++i) {
        uint32_t itopicId;
        LoadNumericID(&itopicId, 4);
        TESForm* ptopicForm = TESForm::GetFormByNumericID(itopicId);
        // listTopics is a BSSimpleList<void*>; its items are the topics.
        TESTopic* ptopic = dynamic_cast<TESTopic*>(ptopicForm);
        if (ptopic) {
            listTopics.AddHead(ptopic);
        }
    }
    TESTopic::SortTopics(true, GetTopicList());
    listQuestLog.RemoveAll();
    uint16_t ilogCount;
    LoadGameDataOLD(&ilogCount, sizeof(ilogCount));
    for (int32_t i = 0; i < ilogCount; ++i) {
        uint32_t iquestId;
        uint8_t cstageIndex;
        uint8_t cstageItemIndex;
        LoadNumericID(&iquestId, 4);
        LoadGameDataOLD(&cstageIndex, sizeof(cstageIndex));
        LoadGameDataOLD(&cstageItemIndex, sizeof(cstageItemIndex));
        TESForm* pquestForm = TESForm::GetFormByNumericID(iquestId);
        TESQuest* pquest = dynamic_cast<TESQuest*>(pquestForm);
        if (pquest) {
            TESQuestStage* pstage = pquest->GetStage(cstageIndex);
            if (pstage) {
                // listQuestLog is a BSSimpleList<void*>; its items are the stage items.
                TESQuestStageItem* pstageItem = pstage->GetStageItem(cstageItemIndex);
                if (pstageItem) {
                    listQuestLog.AddTail(pstageItem);
                }
            }
        }
    }
    listObjectives.RemoveAll();
    uint16_t iobjectiveCount;
    LoadGameDataOLD(&iobjectiveCount, sizeof(iobjectiveCount));
    for (int32_t i = 0; i < iobjectiveCount; ++i) {
        uint32_t iquestId;
        uint32_t iobjectiveIndex;
        LoadNumericID(&iquestId, 4);
        LoadGameDataOLD(&iobjectiveIndex, sizeof(iobjectiveIndex));
        TESForm* pquestForm = TESForm::GetFormByNumericID(iquestId);
        TESQuest* pquest = dynamic_cast<TESQuest*>(pquestForm);
        if (pquest) {
            BGSQuestObjective* pobjective = pquest->GetObjective(iobjectiveIndex);
            if (pobjective) {
                listObjectives.AddTail(pobjective);
            }
        }
    }
    TESNPC* pnpc = static_cast<TESNPC*>(GetObjectReference());
    pnpc->LoadFaceGen(this);
    // The name SaveGame wrote, set as the base form's full name through its TESFullName component.
    uint8_t clength;
    char cname[260];
    LoadGameDataOLD(&clength, sizeof(clength));
    LoadGameDataOLD(cname, clength);
    TESNPC* pbase = static_cast<TESNPC*>(GetObjectReference());
    pbase->TESFullName::SetFullName(cname);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 44u) {
        TESNPC* pactorBase = static_cast<TESNPC*>(GetActorBase());
        uint32_t icustomClassIndex = iPlayerCustomClass.Int();
        TESClass* pcustomClass = pDataHandler->GetClass(icustomClassIndex);
        uint32_t iclassID;
        LoadNumericID(&iclassID, 4);
        TESForm* pclassForm = TESForm::GetFormByNumericID(iclassID);
        TESClass* pclass = dynamic_cast<TESClass*>(pclassForm);
        if (pclass) {
            pactorBase->SetClass(pclass);
        }
        if (pclass && pclass == pcustomClass) {
            pclass->LoadGame();
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
        uint32_t iclassBasedOnID;
        LoadNumericID(&iclassBasedOnID, 4);
        if (iclassBasedOnID) {
            TESForm* pclassForm = TESForm::GetFormByNumericID(iclassBasedOnID);
            pClassBasedOn = dynamic_cast<TESClass*>(pclassForm);
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            if (icurrentBufferPosition <= iplayerStartBufferPosition + iplayerBlockSize) {
                if (icurrentBufferPosition < iplayerStartBufferPosition + iplayerBlockSize) {
                    uint32_t iFlags = pheader->iFlags;
                    int32_t cVersion = pheader->cVersion;
                    const char* pEditorID = pform->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(iplayerStartBufferPosition + iplayerBlockSize - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                        14280,
                        pheader->iFormID,
                        pEditorID,
                        cVersion,
                        iFlags
                    );
                }
            }
            else {
                uint32_t iFlags = pheader->iFlags;
                int32_t cVersion = pheader->cVersion;
                const char* pEditorID = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - (iplayerStartBufferPosition + iplayerBlockSize)),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                    14280,
                    pheader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else {
            if (icurrentBufferPosition <= iplayerStartBufferPosition + iplayerBlockSize) {
                if (icurrentBufferPosition < iplayerStartBufferPosition + iplayerBlockSize) {
                    int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        static_cast<int32_t>(iplayerStartBufferPosition + iplayerBlockSize - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                        14280,
                        iVersion
                    );
                }
            }
            else {
                int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    static_cast<int32_t>(icurrentBufferPosition - (iplayerStartBufferPosition + iplayerBlockSize)),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
                    14280,
                    iVersion
                );
            }
        }
    }
}

// The player's state from a save buffer. The base character data is loaded after the actor value modifiers; every read
// size is the game's. LoadData fills raw bytes and LoadFormID writes a form ID into a pointer slot (resolved later by
// InitLoadGame), so both take their destinations as untyped memory. The perk-rank lists are pruned of ranks whose perk
// no longer exists.
void PlayerCharacter::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    SetNeedResetHardcoreTimer(true);
    StatusSoundHandle.Stop();
    StatusSoundHandle.Release();

    uint32_t uiValueCount = ActorValue::ACTOR_VALUE_COUNT;
    for (uint32_t i = 0; i < uiValueCount; ++i) {
        apLoadGameBuffer->LoadData(
            reinterpret_cast<char*>(&TemporaryActorValueModifiers[i]),
            sizeof(TemporaryActorValueModifiers[i])
        );
    }
    for (uint32_t i = 0; i < uiValueCount; ++i) {
        apLoadGameBuffer->LoadData(
            reinterpret_cast<char*>(&ScriptActorValueModifiers[i]),
            sizeof(ScriptActorValueModifiers[i])
        );
    }
    for (uint32_t i = 0; i < uiValueCount; ++i) {
        apLoadGameBuffer->LoadData(
            reinterpret_cast<char*>(&DamageActorValueModifiers[i]),
            sizeof(DamageActorValueModifiers[i])
        );
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fHealthModifier), sizeof(fHealthModifier));
    Character::LoadGame(apLoadGameBuffer);

    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION)) {
        TESBoundObject* pObject = GetObjectReference();
        pSaveLoadGame->QueueSubBuffer(QUEUED_SUB_BUFFER_ANIMATION, apLoadGameBuffer, pObject);
    }

    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&b3rdPerson), sizeof(b3rdPerson));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTemp3rdPerson), sizeof(bTemp3rdPerson));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bAlwaysRun), sizeof(bAlwaysRun));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bAutoMove), sizeof(bAutoMove));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iSleepTime), sizeof(iSleepTime));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fsecondRunning), sizeof(fsecondRunning));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fsecondSwimming), sizeof(fsecondSwimming));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fsecondSneaking), sizeof(fsecondSneaking));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bActorinSneakRange), sizeof(bActorinSneakRange));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bGreetingPlayer), sizeof(bGreetingPlayer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fGreetTimer), sizeof(fGreetTimer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ihourstosleep), sizeof(ihourstosleep));
    apLoadGameBuffer->LoadData(&cMurder, sizeof(cMurder));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iAmountStolenSold), sizeof(iAmountStolenSold));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bBeenAttacked), sizeof(bBeenAttacked));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bBlockActivate), sizeof(bBlockActivate));

    // A change of age rebuilds the head and the 3D.
    bool bWasYoung = bIsYoung;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bIsYoung), sizeof(bIsYoung));
    if (bIsYoung != bWasYoung && Get3D()) {
        static_cast<TESNPC*>(GetActorBase())->ClearHead();
        BaseProcess* pProcess = GetCurrentAIProcess();
        pProcess->Set3DUpdateFlag(RESET_FACE);
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bIsToddler), sizeof(bIsToddler));

    NiPoint3 kPlacedMarkerPos;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fSitHeadingDelta), sizeof(fSitHeadingDelta));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&kPlacedMarkerPos), sizeof(kPlacedMarkerPos));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fEyeHeight), sizeof(fEyeHeight));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iNumberTraining), sizeof(iNumberTraining));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eskilladvance), sizeof(eskilladvance));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bIsSleeping), sizeof(bIsSleeping));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fFOV), sizeof(fFOV));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&f1stPersonFOV), sizeof(f1stPersonFOV));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fWorldFOV), sizeof(fWorldFOV));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bChargen), sizeof(bChargen));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fProjectileReleaseTimer), sizeof(fProjectileReleaseTimer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iTotalPlayingTime), sizeof(iTotalPlayingTime));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucControlsDisabled), sizeof(ucControlsDisabled));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bBeingChased), sizeof(bBeingChased));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eGrabType), sizeof(eGrabType));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fGrabObjectWeight), sizeof(fGrabObjectWeight));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fGrabDistance), sizeof(fGrabDistance));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCombatPersue), sizeof(iCombatPersue));
    apLoadGameBuffer->LoadData(
        reinterpret_cast<char*>(&bShowQuestItemsInInventory),
        sizeof(bShowQuestItemsInInventory)
    );
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTemp3rdPersonSwitchBack), sizeof(bTemp3rdPersonSwitchBack));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bCanFastTravel), sizeof(bCanFastTravel));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCharacterSeed), sizeof(iCharacterSeed));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&kVanityModePos.y), sizeof(kVanityModePos.y));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fCombatTimer), sizeof(fCombatTimer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fYieldTimer), sizeof(fYieldTimer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iNumberofStealWarnings), sizeof(iNumberofStealWarnings));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fStealWarningTimer), sizeof(fStealWarningTimer));
    apLoadGameBuffer->LoadData(
        reinterpret_cast<char*>(&iNumberofPickpocketWarnings),
        sizeof(iNumberofPickpocketWarnings)
    );
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPickPocketWarningTimer), sizeof(fPickPocketWarningTimer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSpeaking), sizeof(bSpeaking));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bMegatonDestroyed), sizeof(bMegatonDestroyed));
    if (apLoadGameBuffer->GetVersion() >= 0xD) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTemp1stPerson), sizeof(bTemp1stPerson));
        apLoadGameBuffer->LoadData(
            reinterpret_cast<char*>(&bTemp1stPersonSwitchBack),
            sizeof(bTemp1stPersonSwitchBack)
        );
    }
    if (apLoadGameBuffer->GetVersion() >= 0x10) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bCanUsePowerArmor), sizeof(bCanUsePowerArmor));
    }
    if (apLoadGameBuffer->GetVersion() >= 0x12) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHostileDetection), sizeof(bHostileDetection));
    }
    if (apLoadGameBuffer->GetVersion() >= 0x15) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iSandmanDetection), sizeof(iSandmanDetection));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fBlockActivateTimer), sizeof(fBlockActivateTimer));
        for (uint32_t i = 0; i < 5; ++i) {
            apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pCrimeCounts[i]), sizeof(pCrimeCounts[i]));
        }
    }
    CharacterProgressionInfo.LoadGame(apLoadGameBuffer);

    TESForm* pPlacedForm = nullptr;
    pActiveQuest = dynamic_cast<TESQuest*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    pDefaultClass = dynamic_cast<TESClass*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    pPlacedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
    pPlayerMapMarker = nullptr;
    if (pPlacedForm) {
        SetPlayerMapMarker(kPlacedMarkerPos, pPlacedForm);
    }
    pOccupiedRegion = dynamic_cast<TESRegion*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    TESWeather* pWeather = nullptr;
    pWeather = dynamic_cast<TESWeather*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    if (pOccupiedRegion && pWeather) {
        pOccupiedRegion->SetCurrentWeather(pWeather);
    }

    apLoadGameBuffer->LoadFormID(pClosestConversation);
    if (apLoadGameBuffer->GetVersion() >= 0x12) {
        apLoadGameBuffer->LoadFormID(pAIConversationRunning);
    }
    apLoadGameBuffer->LoadFormID(pGrabbedObject);
    apLoadGameBuffer->LoadFormID(pLastExtDoorActivated);
    apLoadGameBuffer->LoadFormID(pAutoAimActor);
    apLoadGameBuffer->LoadFormID(pPlayersTargetActor);

    uint32_t uiTopicCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiTopicCount; ++i) {
        void* pTopic = nullptr;
        pTopic = dynamic_cast<TESTopic*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
        if (pTopic) {
            listTopics.AddHead(pTopic);
        }
    }
    // listTopics is declared as BSSimpleList<void*> (GetTopicList's callers); its items are the topics.
    TESTopic::SortTopics(true, reinterpret_cast<BSSimpleList<TESTopic*>*>(&listTopics));

    uint32_t uiNoteCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiNoteCount; ++i) {
        BGSNote* pNote = nullptr;
        pNote = dynamic_cast<BGSNote*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
        if (pNote) {
            listNotes.AddTail(pNote);
        }
    }

    // The Rock-It Launcher's ammo: one new item change per slot, each loading itself.
    uint32_t uiAmmoCount = apLoadGameBuffer->LoadVariableSizedValue();
    RockItLauncherAmmoList.SetSize(uiAmmoCount, true);
    for (uint32_t i = 0; i < uiAmmoCount; ++i) {
        ItemChange** ppAmmo = &RockItLauncherAmmoList[i];
        *ppAmmo = new ItemChange;
        (*ppAmmo)->LoadGame(apLoadGameBuffer);
    }
    ResetRockItAmmoWeight(true);

    FreeCompassTargets();
    uint32_t uiTargetCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (uiTargetCount) {
        pListofPercievedActors = new BSSimpleList<CompassTarget*>;
        for (uint32_t i = 0; i < uiTargetCount; ++i) {
            CompassTarget* pTarget = new CompassTarget;
            apLoadGameBuffer->LoadFormID(pTarget->pTarget);
            apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pTarget->bIsHostile), sizeof(pTarget->bIsHostile));
            apLoadGameBuffer->LoadData(
                reinterpret_cast<char*>(&pTarget->bIsUndetected),
                sizeof(pTarget->bIsUndetected)
            );
            pListofPercievedActors->AddHead(pTarget);
        }
    }

    uint32_t uiPerkCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiPerkCount; ++i) {
        PerkRankData* pRank = static_cast<PerkRankData*>(ourMalloc(sizeof(PerkRankData)));
        pRank->pPerk = dynamic_cast<BGSPerk*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pRank->cCurrentRank), sizeof(pRank->cCurrentRank));
        Perks.AddHead(pRank);
    }
    BSSimpleList<PerkRankData*>* pRankNode = &Perks;
    BSSimpleList<PerkRankData*>* pPrevRankNode = nullptr;
    while (pRankNode && !pRankNode->IsEmpty()) {
        PerkRankData* pRank = pRankNode->GetItem();
        bool bRemove = false;
        if (pRank) {
            if (!pRank->pPerk) {
                ourFree(pRank);
                bRemove = true;
            }
        }
        else {
            bRemove = true;
        }
        if (bRemove) {
            if (pPrevRankNode) {
                pPrevRankNode->Remove(pRankNode->GetItem());
                pRankNode = pPrevRankNode->GetNext();
            }
            else {
                pRankNode->RemoveHead();
            }
        }
        else {
            pPrevRankNode = pRankNode;
            pRankNode = pRankNode->GetNext();
        }
    }

    uint32_t uiActionCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (uiActionCount) {
        if (pListofActions) {
            pListofActions->RemoveAll();
        }
        else {
            pListofActions = new BSSimpleList<PlayerActionObject*>;
        }
        for (uint32_t i = 0; i < uiActionCount; ++i) {
            PlayerActionObject* pAction = new PlayerActionObject;
            apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pAction->eAction), sizeof(pAction->eAction));
            apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pAction->ftimer), sizeof(pAction->ftimer));
            apLoadGameBuffer->LoadFormID(pAction->prefObj);
            pListofActions->AddHead(pAction);
        }
    }

    if (pListofCasinoData) {
        for (BSSimpleList<CasinoData*>* pNode = pListofCasinoData; pNode; pNode = pNode->GetNext()) {
            ourFree(pNode->GetItem());
        }
        pListofCasinoData->RemoveAll();
    }
    if (pListofCasinoData) {
        pListofCasinoData->RemoveAll();
    }
    uint32_t uiCasinoCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (uiCasinoCount) {
        if (!pListofCasinoData) {
            pListofCasinoData = new BSSimpleList<CasinoData*>;
        }
        for (uint32_t i = 0; i < uiCasinoCount; ++i) {
            CasinoData* pCasino = new CasinoData;
            apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pCasino->casinoID), sizeof(pCasino->casinoID));
            apLoadGameBuffer->LoadData(
                reinterpret_cast<char*>(&pCasino->iTotalWinnings),
                sizeof(pCasino->iTotalWinnings)
            );
            apLoadGameBuffer->LoadData(
                reinterpret_cast<char*>(&pCasino->iWinningLevel),
                sizeof(pCasino->iWinningLevel)
            );
            pListofCasinoData->AddHead(pCasino);
        }
    }
    if (uiAntiCheatTimer) {
        bAntiCheatActive = 1;
    }

    // The caravan cards are deleted and reloaded.
    if (pInactiveListofCaravanCards) {
        for (BSSimpleList<TESCaravanCard*>* pNode = pInactiveListofCaravanCards; pNode; pNode = pNode->GetNext()) {
            TESCaravanCard* pCard = pNode->GetItem();
            delete pCard;
        }
        pInactiveListofCaravanCards->RemoveAll();
    }
    if (pInactiveListofCaravanCards) {
        pInactiveListofCaravanCards->RemoveAll();
    }
    uint32_t uiInactiveCardCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (uiInactiveCardCount) {
        if (!pInactiveListofCaravanCards) {
            pInactiveListofCaravanCards = new BSSimpleList<TESCaravanCard*>;
        }
        for (uint32_t i = 0; i < uiInactiveCardCount; ++i) {
            TESCaravanCard* pCard = new TESCaravanCard;
            pCard = dynamic_cast<TESCaravanCard*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
            pInactiveListofCaravanCards->AddHead(pCard);
        }
    }
    if (pActiveListofCaravanCards) {
        for (BSSimpleList<TESCaravanCard*>* pNode = pActiveListofCaravanCards; pNode; pNode = pNode->GetNext()) {
            TESCaravanCard* pCard = pNode->GetItem();
            delete pCard;
        }
        pActiveListofCaravanCards->RemoveAll();
    }
    if (pActiveListofCaravanCards) {
        pActiveListofCaravanCards->RemoveAll();
    }
    uint32_t uiActiveCardCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (uiActiveCardCount) {
        if (!pActiveListofCaravanCards) {
            pActiveListofCaravanCards = new BSSimpleList<TESCaravanCard*>;
        }
        for (uint32_t i = 0; i < uiActiveCardCount; ++i) {
            TESCaravanCard* pCard = new TESCaravanCard;
            pCard = dynamic_cast<TESCaravanCard*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
            pActiveListofCaravanCards->AddHead(pCard);
        }
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCaravanCapWinnings), sizeof(iCaravanCapWinnings));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCaravanCapLosses), sizeof(iCaravanCapLosses));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCaravanWinnings), sizeof(iCaravanWinnings));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCaravanLosses), sizeof(iCaravanLosses));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCaravanLargestWinning), sizeof(iCaravanLargestWinning));

    // The quest log: a quest, then its stage index and the stage item's index.
    uint32_t uiLogCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiLogCount; ++i) {
        TESQuest* pQuest = nullptr;
        uint8_t ucStage = 0;
        uint8_t ucItem = 0;
        pQuest = dynamic_cast<TESQuest*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucStage), sizeof(ucStage));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucItem), sizeof(ucItem));
        if (pQuest) {
            TESQuestStage* pStage = pQuest->GetStage(ucStage);
            if (pStage) {
                TESQuestStageItem* pItem = pStage->GetStageItem(ucItem);
                if (pItem) {
                    listQuestLog.AddTail(pItem);
                }
            }
        }
    }
    uint32_t uiObjectiveCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiObjectiveCount; ++i) {
        TESQuest* pQuest = nullptr;
        uint32_t uiObjective = 0;
        pQuest = dynamic_cast<TESQuest*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiObjective), sizeof(uiObjective));
        if (pQuest) {
            BGSQuestObjective* pObjective = pQuest->GetObjective(uiObjective);
            if (pObjective) {
                listObjectives.AddTail(pObjective);
            }
        }
    }

    ActiveEffect::LoadActiveEffectList(apLoadGameBuffer, pActiveEffectList);
    uint32_t uiCombatGroupID = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiCombatGroupID), sizeof(uiCombatGroupID));
    if (uiCombatGroupID) {
        pCombatGroup = pCombatManager->GetCombatGroup(uiCombatGroupID);
    }
    if (apLoadGameBuffer->GetVersion() >= 8) {
        float fShakeMult = 0.0f;
        float fShakeTime = 0.0f;
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fShakeMult), sizeof(fShakeMult));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fShakeTime), sizeof(fShakeTime));
        SetCameraShakeData(fShakeMult, fShakeTime);
    }

    if (apLoadGameBuffer->GetVersion() >= 0x16) {
        uint32_t uiCompanionPerkCount = apLoadGameBuffer->LoadVariableSizedValue();
        for (uint32_t i = 0; i < uiCompanionPerkCount; ++i) {
            PerkRankData* pRank = static_cast<PerkRankData*>(ourMalloc(sizeof(PerkRankData)));
            pRank->pPerk = dynamic_cast<BGSPerk*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
            apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pRank->cCurrentRank), sizeof(pRank->cCurrentRank));
            CompanionPerks.AddHead(pRank);
        }
        BSSimpleList<PerkRankData*>* pNode = &CompanionPerks;
        BSSimpleList<PerkRankData*>* pPrevNode = nullptr;
        while (pNode && !pNode->IsEmpty()) {
            PerkRankData* pRank = pNode->GetItem();
            bool bRemove = false;
            if (pRank) {
                if (!pRank->pPerk) {
                    ourFree(pRank);
                    bRemove = true;
                }
            }
            else {
                bRemove = true;
            }
            if (bRemove) {
                if (pPrevNode) {
                    pPrevNode->Remove(pNode->GetItem());
                    pNode = pPrevNode->GetNext();
                }
                else {
                    pNode->RemoveHead();
                }
            }
            else {
                pPrevNode = pNode;
                pNode = pNode->GetNext();
            }
        }
    }

    if (apLoadGameBuffer->GetVersion() >= 0x17) {
        uint8_t ucTracked = 1;
        uint8_t ucHardcore = 0;
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucHardcore), sizeof(ucHardcore));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucTracked), sizeof(ucTracked));
        SetHardcore(static_cast<GamePlayFormulas::HardcoreSetting>(ucHardcore), true);
        if (!ucTracked) {
            ClearsHardcoreFlag();
        }
    }
    if (apLoadGameBuffer->GetVersion() >= 0x19) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bCanWait), sizeof(bCanWait));
    }
    if (apLoadGameBuffer->GetVersion() >= 0x1A) {
        int32_t iHotkeyCount = 8;
        for (int32_t i = 0; i < iHotkeyCount; ++i) {
            uint32_t uiAmmoID = 0;
            TESForm* pAmmo = nullptr;
            apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiAmmoID), sizeof(uiAmmoID));
            if (uiAmmoID) {
                pAmmo = TESForm::GetFormByNumericID(uiAmmoID);
            }
            SetAmmoForHotKey(i, static_cast<TESAmmo*>(pAmmo));
        }
        // The process is fetched again for each call; the reload uses the weapon's mod effect 2.
        if (GetCurrentAIProcess()) {
            BaseProcess* pProcess = GetCurrentAIProcess();
            if (pProcess->GetCurrentWeapon()) {
                BaseProcess* pAmmoProcess = GetCurrentAIProcess();
                pAmmoProcess->SetCurrentAmmo(nullptr);
                BaseProcess* pModProcess = GetCurrentAIProcess();
                BaseProcess* pWeaponProcess = GetCurrentAIProcess();
                bool bUseModValue = pModProcess->GetCurrentWeapon()->HasModEffectActive(2);
                TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(
                    pWeaponProcess->GetCurrentWeapon()->GetItemChangeObject()
                );
                ReloadWeaponNV(pWeapon, 0, bUseModValue, true);
            }
        }
    }
    AmmoSwapHelper(false, true);
    if (apLoadGameBuffer->GetVersion() >= 0x1B) {
        Script::LoadPendingReferenceLists(apLoadGameBuffer);
    }
}

void PlayerCharacter::InitLoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    Character::InitLoadGame(aiFlags, aiCurrentFlags);
    uint32_t iconversationID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pClosestConversation));
    if (iconversationID) {
        TESForm* pForm = TESForm::GetFormByNumericID(iconversationID);
        pClosestConversation = dynamic_cast<DialoguePackage*>(pForm);
    }
    uint32_t iprisonMarkerID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPrisonMarker));
    if (pPrisonMarker) {
        TESForm* pForm = TESForm::GetFormByNumericID(iprisonMarkerID);
        pPrisonMarker = dynamic_cast<TESObjectREFR*>(pForm);
    }
    uint32_t iselectedSpellID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSelectedSpell));
    pSelectedSpell = nullptr;
    if (iselectedSpellID) {
        TESForm* pform = TESForm::GetFormByNumericID(iselectedSpellID);
        MagicItemForm* pmagicItemForm = dynamic_cast<MagicItemForm*>(pform);
        MagicItemObject* pmagicItemObject = dynamic_cast<MagicItemObject*>(pform);
        if (pmagicItemForm) {
            SetSelectedSpell(pmagicItemForm);
        }
        else if (pmagicItemObject) {
            SetSelectedSpell(pmagicItemObject);
        }
    }
    uint32_t icurrentSpellID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentSpell));
    if (icurrentSpellID) {
        pCurrentSpell = MagicItem::GetMagicItemByNumericID(icurrentSpellID);
    }
    uint32_t idesiredTargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDesiredTarget));
    if (idesiredTargetID) {
        pDesiredTarget = MagicTarget::GetMagicTargetByNumericID(idesiredTargetID);
    }
    uint32_t iselectedScrollID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSelectedScroll));
    if (iselectedScrollID) {
        TESForm* pForm = TESForm::GetFormByNumericID(iselectedScrollID);
        TESObjectBOOK* pScroll = dynamic_cast<TESObjectBOOK*>(pForm);
        SetSelectedScroll(pScroll);
    }
    uint32_t idefaultClassID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDefaultClass));
    if (idefaultClassID) {
        TESForm* pForm = TESForm::GetFormByNumericID(idefaultClassID);
        pDefaultClass = dynamic_cast<TESClass*>(pForm);
    }
    uint32_t ipendingPoisonID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPendingPoison));
    if (ipendingPoisonID) {
        TESForm* pForm = TESForm::GetFormByNumericID(ipendingPoisonID);
        pPendingPoison = dynamic_cast<AlchemyItem*>(pForm);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
        uint32_t igrabbedObjectID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pGrabbedObject));
        if (igrabbedObjectID) {
            TESForm* pForm = TESForm::GetFormByNumericID(igrabbedObjectID);
            pGrabbedObject = dynamic_cast<TESObjectREFR*>(pForm);
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 66u) {
        uint32_t iprisonContainedID = static_cast<uint32_t>(
            reinterpret_cast<uintptr_t>(PlayerCharacter::pPrisonContainer)
        );
        if (PlayerCharacter::pPrisonContainer) {
            TESForm* pForm = TESForm::GetFormByNumericID(iprisonContainedID);
            PlayerCharacter::pPrisonContainer = dynamic_cast<TESObjectREFR*>(pForm);
        }
    }
    // From version 99 the game walks a list it has set to null, so the loop never runs: only the version test is left.
    pSaveLoadGameOLD->GetCurrentVersion();
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 115u) {
        BSSimpleList<TESObjectREFR*>* plist = &ConflictingObjects;
        BSSimpleList<TESObjectREFR*>* plastList = nullptr;
        while (plist && !plist->IsEmpty()) {
            TESObjectREFR* pconflictID = plist->m_item;
            uint32_t irefID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pconflictID));
            TESObjectREFR* pref = nullptr;
            if (irefID) {
                TESForm* pForm = TESForm::GetFormByNumericID(irefID);
                pref = dynamic_cast<TESObjectREFR*>(pForm);
                plist->SetItem(pref);
            }
            if (pref) {
                plastList = plist;
                plist = plist->m_pkNext;
            }
            else if (plastList) {
                plastList->Remove(pconflictID);
                plist = plastList->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
        }
    }
    ActiveEffect::InitLoadGameActiveEffectList(pActiveEffectList, this);
    pCurrentProcess->ClearFaceAnimationData();
    TESNPC* pnpc = static_cast<TESNPC*>(GetObjectReference());
    pnpc->InitLoadFaceGen(this);
    SetAIControlledToPosition(false);
    SetAIControlledFromPosition(false);
    SetControlsDisabled(ucControlsDisabled);
}

void PlayerCharacter::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Character::InitLoadGame(apLoadGameBuffer);
    uint32_t iclosestConversationID = pSaveLoadGame->GetChangedFormID(
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pClosestConversation))
    );
    DialoguePackage* pConversation = nullptr;
    if (iclosestConversationID) {
        TESForm* pForm = TESForm::GetFormByNumericID(iclosestConversationID);
        pConversation = dynamic_cast<DialoguePackage*>(pForm);
    }
    pClosestConversation = pConversation;
    if (apLoadGameBuffer->GetVersion() >= 18u) {
        uint32_t iAIConversationRunningID = pSaveLoadGame->GetChangedFormID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pAIConversationRunning))
        );
        DialoguePackage* pRunningConversation = nullptr;
        if (iAIConversationRunningID) {
            TESForm* pForm = TESForm::GetFormByNumericID(iAIConversationRunningID);
            pRunningConversation = dynamic_cast<DialoguePackage*>(pForm);
        }
        pAIConversationRunning = pRunningConversation;
    }
    TESObjectREFR* pGrabbed = nullptr;
    if (pGrabbedObject) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pGrabbedObject))
        );
        pGrabbed = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pGrabbedObject = pGrabbed;
    TESObjectREFR* pLastDoor = nullptr;
    if (pLastExtDoorActivated) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLastExtDoorActivated))
        );
        pLastDoor = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pLastExtDoorActivated = pLastDoor;
    Actor* pAutoAim = nullptr;
    if (this->GetAutoAimActorSimple()) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->GetAutoAimActorSimple()))
        );
        pAutoAim = dynamic_cast<Actor*>(pForm);
    }
    pAutoAimActor = pAutoAim;
    Actor* pTargetActor = nullptr;
    if (pPlayersTargetActor) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPlayersTargetActor))
        );
        pTargetActor = dynamic_cast<Actor*>(pForm);
    }
    pPlayersTargetActor = pTargetActor;
    BSSimpleList<CompassTarget*>* plist = pListofPercievedActors;
    BSSimpleList<CompassTarget*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        CompassTarget* pactorsNearPlayerData = plist->m_item;
        bool bremove = false;
        if (pactorsNearPlayerData) {
            Actor* pactor = nullptr;
            if (pactorsNearPlayerData->pTarget) {
                TESForm* pForm = TESForm::GetFormByNumericID(
                    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pactorsNearPlayerData->pTarget))
                );
                pactor = dynamic_cast<Actor*>(pForm);
            }
            pactorsNearPlayerData->pTarget = pactor;
            if (!pactorsNearPlayerData->pTarget || !pactorsNearPlayerData->pTarget->GetObjectReference()) {
                ourFree(pactorsNearPlayerData);
                bremove = true;
            }
        }
        else {
            bremove = true;
        }
        if (bremove) {
            if (plastList) {
                plastList->Remove(plist->m_item);
                plist = plastList->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
        }
        else {
            plastList = plist;
            plist = plist->m_pkNext;
        }
    }
    for (PlayerActionObject* pplayerAction : *pListofActions) {
        if (pplayerAction) {
            TESObjectREFR* pRef = nullptr;
            if (pplayerAction->prefObj) {
                TESForm* pForm = TESForm::GetFormByNumericID(
                    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pplayerAction->prefObj))
                );
                pRef = dynamic_cast<TESObjectREFR*>(pForm);
            }
            pplayerAction->prefObj = pRef;
            if (pplayerAction->prefObj) {
                pplayerAction->prefObj->SetTargeted(true);
            }
        }
    }
    SetControlsDisabled(ucControlsDisabled);
    ActiveEffect::InitLoadActiveEffectList(apLoadGameBuffer, pActiveEffectList);
    for (PerkRankData* pperkRank : Perks) {
        if (pperkRank && pperkRank->pPerk) {
            pperkRank->pPerk->AddPerkEntryPoints(this, pperkRank->cCurrentRank, false);
        }
    }
    if (p1stPersonCamera) {
        p1stPersonCamera->SetRotate(NiMatrix3::IDENTITY);
    }
    double dHealth = GetActorFloatValue(ActorValue::HEALTH);
    float fhealthRatio = dHealth / GetPermanentActorFloatValue(ActorValue::HEALTH);
    if (fPlayerHealthHeartbeatSlow.Float() >= static_cast<double>(fhealthRatio) &&
        fPlayerHealthHeartbeatFast.Float() < static_cast<double>(fhealthRatio)) {
        StatusSoundHandle = BSAudio::QInstance().GetSoundHandleByName("UIHealthHeartbeatALP", 49u);
        StatusSoundHandle.Play(true);
    }
    else if (fPlayerHealthHeartbeatFast.Float() >= static_cast<double>(fhealthRatio)) {
        StatusSoundHandle = BSAudio::QInstance().GetSoundHandleByName("UIHealthHeartbeatBLP", 49u);
        StatusSoundHandle.Play(true);
    }
    if (Get3D() &&
        !apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_LEVELED_INVENTORY | CHANGE_REFR_INVENTORY) &&
        apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_LEVELED_INVENTORY | CHANGE_REFR_INVENTORY)) {
        TESNPC* pnpc = static_cast<TESNPC*>(GetActorBase());
        pnpc->InitDefaultWorn(this, true, true, false, true);
        if (pCurrentProcess && pCurrentProcess->GetAll3DUpdateFlags()) {
            pCurrentProcess->Update3dModel(this);
        }
    }
}

void PlayerCharacter::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    Actor::FinishInitLoadGame(aiFlags, aiOldFlags);
    if (Get3D(true)) {
        SetFirstPerson(!b3rdPerson);
        if (bTemp3rdPerson) {
            ForceTemp3rdPerson(true);
        }
    }
    ActiveEffect::FinishInitLoadGameActiveEffectList(pActiveEffectList, this);
    if (pGrabbedObject && eGrabType != PlayerCharacter::GRAB_TELEKINESIS) {
        CreateMouseSpring(pGrabbedObject, eGrabType, fGrabDistance);
    }

    DetectLifeEffect::CheckPlayerRunningDetectLife();
    NightEyeEffect::ApplyVisualEffect();
    SetLastKnownGoodPosition();
    SetFOV(fFOV);
    CharacterProgressionInfo.FinishInitLoadGame(aiFlags, aiOldFlags);
}

void PlayerCharacter::Revert(uint32_t aiFlags) {
    Character::Revert(aiFlags);
    SetNeedResetHardcoreTimer(true);
    fCheckMyDeadBodyTimer = 0.0f;
    bCanFastTravel = 0;
    SetCanFastTravel(true);
    fCheckMyDeadBodyTimer = 0.0f;
    if (pSaveLoadGameOLD->IsUnk048Zero()) {
        pPrisonMarker = nullptr;
        PlayerCharacter::pPrisonContainer = nullptr;
        StopVanityMode();
        SetInCharGen(false);
        ConflictingObjects.RemoveAll();
        ActiveEffect::RevertActiveEffectList(pActiveEffectList, this);
        if (((aiFlags & CHANGE_REFR_ANIMATION) != 0) && p1stPersonAnimation) {
            p1stPersonAnimation->Revert(this);
        }

        ResetLastPlayingTimeUpdate();
        DestroyMouseSpring();
        DroppedRefList.RemoveAll();
    }
    bAllowEGMCacheClear = true;
}

void PlayerCharacter::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESTopic* ptopic;
    Character::Revert(apLoadGameBuffer);
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION) && p1stPersonAnimation) {
        p1stPersonAnimation->Revert(apLoadGameBuffer);
    }

    SetStoredWorldRecurse(Get3D(false), nullptr);
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_SCALE)) {
        SetScale(1.0f);
    }
    pPrisonMarker = nullptr;
    PlayerCharacter::pPrisonContainer = nullptr;
    StopVanityMode();
    bSpeaking = false;
    fZoomDelta = 0.0f;
    fChaseDistance = 0.0f;
    fPlayerDeathReloadTime_0 = -1.0f;
    bDelayVATSMenu = false;
    pCurrentWater = nullptr;
    bCanWait = true;
    b3rdPerson = false;
    bWant3rdPerson = false;
    pCombatGroup = nullptr;
    iTeammateCount = 0;
    fCombatTimer = 0.0f;
    fYieldTimer = 0.0f;
    bAiControlledToPos = false;
    bAiControlledFromPos = false;
    bAiControlledActivate = false;
    bAiControlledPackage = false;
    bBlockActivate = false;
    bAllowEGMCacheClear = true;
    fBlockActivateTimer = 5.0f;
    if (apLoadGameBuffer->GetRevertOnly()) {
        bIsYoung = false;
    }
    bIsToddler = false;
    bCanUsePowerArmor = false;
    bHostileDetection = false;
    ucControlsDisabled = 0;
    bTemp3rdPerson = false;
    bTemp3rdPersonSwitchBack = false;
    bTemp1stPerson = false;
    bTemp1stPersonSwitchBack = false;
    bVATSForce1stPerson = false;
    bCanUsePowerArmor = false;
    fWorldFOV = BSSceneGraph::fDefaultWorldFOV.Float();
    f1stPersonFOV = BSSceneGraph::fDefault1stPersonFOV.Float();
    iNumberofStealWarnings = 0;
    fStealWarningTimer = 0.0f;
    iNumberofPickpocketWarnings = 0;
    fPickPocketWarningTimer = 0.0f;
    bSpeaking = false;
    bMegatonDestroyed = false;
    iSleepTime = 0;
    fSitHeadingDelta = 0.0f;
    for (int32_t i = 0; i < 5; ++i) {
        pCrimeCounts[i] = 0;
    }
    pListofCasinoData->RemoveAll();
    pInactiveListofCaravanCards->RemoveAll();
    pActiveListofCaravanCards->RemoveAll();
    iCaravanCapWinnings = 0;
    iCaravanCapLosses = 0;
    iCaravanWinnings = 0;
    iCaravanLosses = 0;
    iCaravanLargestWinning = 0;
    iSandmanDetection = 0;
    fBlockActivateTimer = 5.0f;
    iTotalPlayingTime = 0;
    ResetLastPlayingTimeUpdate();
    fHealthModifier = 0.0f;
    for (int32_t i = 0; i < 77; ++i) {
        TemporaryActorValueModifiers[i] = 0.0f;
        ScriptActorValueModifiers[i] = 0.0f;
        DamageActorValueModifiers[i] = 0.0f;
    }
    ConflictingObjects.RemoveAll();
    ResetLastPlayingTimeUpdate();
    DestroyMouseSpring();
    DroppedRefList.RemoveAll();
    listTopics.RemoveAll();
    listObjectives.RemoveAll();
    listQuestLog.RemoveAll();
    listNotes.RemoveAll();
    listQuestTargets.RemoveAll();
    ClearRockItAmmo();
    FreeCompassTargets();
    CharacterProgressionInfo.Revert(apLoadGameBuffer);
    pClosestConversation = nullptr;
    pAIConversationRunning = nullptr;
    pQueuedWeaponAttach = nullptr;
    RemovePlayerMapMarker();
    pLastExtDoorActivated = nullptr;
    SetInCharGen(false);
    while (this->pListofActions && this->pListofActions->m_item) {
        ourFree(this->pListofActions->m_item);
        this->pListofActions->RemoveHead();
    }
    delete this->pListofActions;
    this->pListofActions = nullptr;
    for (BSSimpleList<PerkRankData*>* pPerkRankList = &Perks; pPerkRankList; pPerkRankList = pPerkRankList->m_pkNext) {
        ourFree(pPerkRankList->m_item);
    }
    Perks.RemoveAll();
    for (uint32_t i = 0; i < 74; ++i) {
        PerkEntryLists[i].RemoveAll();
    }
    for (BSSimpleList<PerkRankData*>* pPerkRankList = &CompanionPerks; pPerkRankList;
         pPerkRankList = pPerkRankList->m_pkNext) {
        ourFree(pPerkRankList->m_item);
    }
    CompanionPerks.RemoveAll();
    for (uint32_t i = 0; i < 74; ++i) {
        CompanionPerkEntryLists[i].RemoveAll();
    }
    ourFree(pQueuedTargetLoc);
    pQueuedTargetLoc = nullptr;
    bAlwaysHardcore = true;
    pReputationUpdate = nullptr;
    SetWeaponSightingNode(nullptr);
    ActiveEffect::RevertActiveEffectList(apLoadGameBuffer, pActiveEffectList);
    for (BSSimpleList<TESTopic*>* plist = pDataHandler->GetTopicList(); plist; plist = plist->m_pkNext) {
        ptopic = plist->m_item;
        if (ptopic && ptopic->IsRandom()) {
            listTopics.AddHead(*reinterpret_cast<void**>(&ptopic));
        }
    }
    TESTopic::SortTopics(true, GetTopicList());
    ClearAudioMarkerList();
    ClearMapMarkerStructList();
    ClearAutoAim();
    ClearPendingReferenceLists();
}

void PlayerCharacter::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    bool bIsSpellTarget;
    Character::FinishLoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_SCALE)) {
        SetScale(fRefScale);
    }
    bWant3rdPerson = b3rdPerson;
    if (Get3D(true)) {
        if (bTemp3rdPerson) {
            ForceTemp3rdPerson(true);
        }
        Show1stPerson(!b3rdPerson);
    }
    if (fPlayerCameraAlpha < 1.0) {
        fPlayerCameraAlpha = 1.0f;
        UpdateAlpha();
    }
    UpdateArmRotation();
    GetAnimation(true)->UpdateSceneGraphNoController();
    UpdateCamera(true, false);
    if (appMain->bFlyCamera) {
        UpdateFlyCamera(0.0f, true);
    }
    if (pGrabbedObject && eGrabType != PlayerCharacter::GRAB_TELEKINESIS) {
        CreateMouseSpring(pGrabbedObject, eGrabType, fGrabDistance);
        Interface::SetDragDroppingState(2);
    }

    SetLastKnownGoodPosition();
    SetFOV(fFOV);
    ActiveEffect::FinishLoadActiveEffectList(apLoadGameBuffer, pActiveEffectList);
    ResetAudioMarkerList();
    ResetMapMarkerStructList();
    if (pActiveQuest) {
        pActiveQuest->UpdateCurrentTargetList(&listQuestTargets, &listObjectives);
    }
    Interface::ResetPipboy();
    UpdatePlayerCombat(0.0f);
    if (GetIronSights()) {
        pCurrentProcess->SetIronSights(false);
        SetIronSights(true, false, false);
    }
    SpellItem* pPipBoyLightSpell = MagicSystem::GetPipBoyLightSpell();
    if (pPipBoyLightSpell) {
        bIsSpellTarget = IsSpellTarget(pPipBoyLightSpell, true);
    }
    else {
        bIsSpellTarget = IsSpellTarget(nullptr, true);
    }
    if (bIsSpellTarget) {
        Interface::GetPipboy()->ShowPipboyLightEffect(true, true, true);
        Interface::GetPipboy()->ShowPipboyLightEffect(false, true, true);
    }
    CheckTempModifiers();
}

void PlayerCharacter::UpdateRefLight() {
    LightRefData* plightData = m_Extra.GetLight();
    if (plightData) {
        if (plightData->spLight == sp1stPersonLight && Get3D(true)->GetAppCulled()) {
            plightData->spLight = sp3rdPersonLight;
        }
        else if (plightData->spLight == sp3rdPersonLight && Get3D(false)->GetAppCulled()) {
            plightData->spLight = sp1stPersonLight;
        }
        Actor::UpdateRefLight();
    }
}

// The first-person camera's position; in third person, or without the camera, the actor's look target.
NiPoint3 PlayerCharacter::GetLookingAtLocation() {
    if (b3rdPerson || !p1stPersonCamera) {
        return Actor::GetLookingAtLocation();
    }
    return p1stPersonCamera->GetWorldTranslate();
}

// Equips a default outfit or a pip-boy unless a save is loading, loads the first-person skeleton with its biped,
// animation and idles, then the third-person 3D through Actor::Load3D (as third person), and adds it to the Havok
// world. AutoMemContext as in Preload1stPersonAnimsFileList.
NiAVObject* PlayerCharacter::Load3D(bool abBackgroundLoading) {
    AutoMemContext kContext(
        MEM_CONTEXT_PLAYER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
        9304
    );
    TESBoundObject* pBaseObject = GetObjectReference();
    if (!pSaveLoadGame->GetSaveGameLoading() && !Interface::bIsLoadingNewGame) {
        bool bNoWornItems = true;
        InventoryChanges* pInventoryChanges = InventoryChanges::GetInventoryChanges(this);
        if (pInventoryChanges) {
            // The worn items returned are copies; the one found is deleted.
            for (int32_t i = 0; i < BIPED_OBJECT_COUNT; ++i) {
                ItemChange* pWornItem = pInventoryChanges->GetWornItem(static_cast<BIPED_OBJECT>(i), false);
                if (pWornItem) {
                    bNoWornItems = false;
                    delete pWornItem;
                    break;
                }
            }
        }
        if (bNoWornItems) {
            static_cast<TESNPC*>(pBaseObject)->InitDefaultWorn(this, true, true, false, false);
        }
        else if (pInventoryChanges) {
            ItemChange* pPipBoy = pInventoryChanges->GetWornItem(BO_PIP_BOY, false);
            if (pPipBoy) {
                delete pPipBoy;
            }
            else {
                ItemChange* pBestArmor = pInventoryChanges->GetBestArmor(GetActorBase(), BO_PIP_BOY, true);
                if (pBestArmor) {
                    TESBoundObject* pArmor = pBestArmor->GetContainerObject();
                    ExtraDataList* pExtra = nullptr;
                    if (pBestArmor->GetExtraDataList()) {
                        pExtra = pBestArmor->GetExtraDataList()->GetItem();
                    }
                    AddWornItem(pArmor, 1, pExtra, false);
                    delete pBestArmor;
                }
            }
        }
    }
    NiNode* p1stPerson3D = pModelLoader
                               ->LoadFile(sFirstPersonSkeleton.String(), LOD_MULT_ACTORS, true, false, false, false);
    p1stPerson3D->SetPickOff(true);
    sp1stPerson3D = p1stPerson3D;
    p1stPersonLooking = sp1stPerson3D->GetObjectByName(FixedStrings::QBip01Looking());
    p1stPersonCamera = sp1stPerson3D->GetObjectByName(FixedStrings::QCamera1st());
    p1stPersonBip = sp1stPerson3D->GetObjectByName(FixedStrings::QBip());
    if (!p1stPersonCamera) {
        InitializationFlow("Missing 'Camera1st' on '%s'.", sFirstPersonSkeleton.String());
    }
    if (!p1stPersonBip) {
        InitializationFlow("Missing 'Bip' on '%s'.", sFirstPersonSkeleton.String());
    }
    NiUpdateData kUpdateData(0.0f, false, false);
    p1stPerson3D->Update(kUpdateData);
    fEyeHeight = p1stPersonCamera->GetWorldTranslate().z;
    NiMatrix3 kRotIn = NiMatrix3::IDENTITY;
    p1stPerson3D->SetRotate(MultipleMatrixByRace(kRotIn));
    p1stPersonBipedAnim = new BipedAnim(this, p1stPerson3D);
    p1stPersonAnimation = new Animation;
    // Actor::Load3D loads the third-person 3D.
    bool bWas3rdPerson = b3rdPerson;
    b3rdPerson = true;
    NiAVObject* p3rdPerson3D = Actor::Load3D(abBackgroundLoading);
    b3rdPerson = bWas3rdPerson;
    GetAnimation(false)->ClearGroup(AGS_IDLE, 0.0f);
    BSSimpleList<const char*>* pKFFileList = pModelLoader->BuildKFFileList(
        sFirstPersonSkeleton.String(),
        true,
        false,
        ANIM_GROUP_WEAPON_COUNT
    );
    char acIdlePath[260];
    BSstrcpy(acIdlePath, 260, sFirstPersonSkeleton.String());
    char* pFileName = std::strrchr(acIdlePath, '\\');
    if (IsPCToddler()) {
        BSstrcpy(pFileName, 260 - (pFileName - acIdlePath) - 1, "\\Locomotion\\Toddler\\IdleAnims");
        pModelLoader->CopyFilenameList(pIdleManager->GetRootFilenameList(acIdlePath, AGW_NONE), pKFFileList);
    }
    if (GetSex() == 1) {
        BSstrcpy(pFileName, 260 - (pFileName - acIdlePath) - 1, "\\Locomotion\\Female\\IdleAnims");
    }
    else {
        BSstrcpy(pFileName, 260 - (pFileName - acIdlePath) - 1, "\\Locomotion\\Male\\IdleAnims");
    }
    pModelLoader->CopyFilenameList(pIdleManager->GetRootFilenameList(acIdlePath, AGW_NONE), pKFFileList);
    p1stPersonAnimation->Init(pKFFileList, p1stPerson3D, this, true);
    BaseProcess* pWeaponProcess = GetCurrentAIProcess();
    BaseProcess* pProcess = GetCurrentAIProcess();
    pProcess->SetCurrentWeapon(pWeaponProcess->GetCurrentWeapon(), nullptr, false);
    Setup3rdPerson3D(p3rdPerson3D, true);
    if (!p3rdPersonCamera) {
        InitializationFlow("Missing 'Camera3rd' on '%s'.", GetModel());
    }
    PreloadPermanentMagic();
    pCurrentProcess->ClearAll3DUpdateFlags();
    InitLightingPropertyPtr(p1stPerson3D);
    InitHavok();
    auto* pController = static_cast<bhkCharacterController*>(GetCharController());
    bhkWorld* pWorld = pController ? pController->GetWorld() : nullptr;
    if (pWorld) {
        // GetCollisionFilter returns the address of the filter it writes, which GetGroup reads.
        CFilter kFilter;
        pController->GetCollisionFilter(&kFilter);
        pWorld->AddObjects(p3rdPerson3D, true, false, kFilter.GetGroup(), false);
    }
    Interface::ResetPipboy();
    if (bIsToddler) {
        bIsToddler = false;
        SetPCToddler(true);
    }
    MarkPlayerBones(GetBiped(false)->GetRoot());
    MarkPlayerBones(GetBiped(true)->GetRoot());
    return p3rdPerson3D;
}

// Sets the third-person 3D; a null node also drops every first-person part and releases the first-person skeleton
// model, which only happens once the game has started.
void PlayerCharacter::Set3D(NiAVObject* apObject, bool abTestProcessLevel) {
    if (!appMain || appMain->QQuit() || appMain->GameReset() || apObject) {
        TESObjectREFR::Set3D(apObject, abTestProcessLevel);
        if (!apObject) {
            if (p1stPersonBipedAnim) {
                delete p1stPersonBipedAnim;
            }
            p1stPersonBipedAnim = nullptr;
            if (p1stPersonAnimation) {
                delete p1stPersonAnimation;
            }
            p1stPersonAnimation = nullptr;
            sp1stPerson3D = nullptr;
            p1stPersonCamera = nullptr;
            p3rdPersonCamera = nullptr;
            p1stPersonBip = nullptr;
            ourMemset(pWobbleNodes, 0, sizeof(pWobbleNodes));
            pModelLoader->ReleaseFile(
                NavMeshInfo::GetCellOrCoords(const_cast<char*>(sFirstPersonSkeleton.String()), 0)
            );
        }
    }
    else {
        Warning("MODEL: PlayerCharacter::Set3D( 0 ) called before the game was over.");
    }
}

NiNode* PlayerCharacter::Get3D() {
    if (!b3rdPerson && sp1stPerson3D) {
        return sp1stPerson3D;
    }
    return TESObjectREFR::Get3D_Body();
}

void PlayerCharacter::UpdateAnimation() {
    ++cSkipThreadQueue;
    double dSeconds = appTimer.fDelta;
    float fsecondsPassed = pVATS.GetPlayerUpdateMult() * dSeconds;
    Animation* pAnim = GetAnimation(false);
    if (pAnim) {
        pAnim->Update(this, fsecondsPassed, -1.0f);
    }
    if (p1stPersonAnimation) {
        p1stPersonAnimation->Update(this, fsecondsPassed, -1.0f);
    }
    if (pCurrentProcess) {
        pCurrentProcess->UpdateDeferredHideDismemberedLimbs(this, fsecondsPassed);
    }
    UpdateRefLight();
    --cSkipThreadQueue;
}

Animation* PlayerCharacter::GetAnimation() {
    if (pInventoryAnimation) {
        return pInventoryAnimation;
    }
    if (!b3rdPerson && p1stPersonAnimation) {
        return p1stPersonAnimation;
    }
    return Actor::GetAnimation();
}

BipedAnim* PlayerCharacter::GetBiped() {
    if (!b3rdPerson && p1stPersonBipedAnim) {
        return p1stPersonBipedAnim;
    }
    return Character::GetBiped();
}

BipedAnim* PlayerCharacter::GetCurrentBiped() {
    return GetBiped(!IsActually3rdPerson());
}

// Moves the player into apCell: water height, multibound, Havok, decal casters and the camera's rigid body, gun wobble,
// climate and image space; then finds the regions the player stands in (sounds, the occupied region, border regions),
// checks the last known good position, shows the occupied region's map name in the HUD and toggles the imposters of the
// regions left and entered. Without a cell, only the region sounds are cleared.
void PlayerCharacter::SetParentCell(TESObjectCELL* apCell) {
    if (apCell == GetParentCell()) {
        return;
    }
    if (!TESWaterSystem::GetCameraInPlaceableWater()) {
        float fWaterHeight;
        if (apCell && apCell->HasWater()) {
            fWaterHeight = apCell->GetWaterHeight();
        }
        else {
            fWaterHeight = -std::numeric_limits<float>::max();
        }
        TESWaterSystem::SetCameraRelevantWaterHeight(fWaterHeight);
    }
    m_Extra.SetMultiBoundRef(nullptr);
    MobileObject::SetParentCell(apCell);
    if (!apCell) {
        CurrentRegionSoundList.RemoveAll();
        FalloutAudio::UpdateRegionSounds();
        return;
    }
    if (!apCell->GetInterior()) {
        SetOccupiedRegion(nullptr);
    }
    FalloutRadio::NewCellEntered();
    pLastKnownMusicType = apCell->GetMusicType(nullptr);
    bhkWorld* pCellWorld = apCell->GetbhkWorld();
    if (pCellWorld) {
        pCellWorld->pAcousticSpaceListener->bBufferRemovalOperations = true;
    }
    MoveHavok(false);
    if (GetParentCell() &&
        GetParentCell()->GetbhkWorld() &&
        DecalCaster::GetInstance(DecalCaster::GetMaxDecalWidthLarge(), false)) {
        bhkWorld* pWorld = GetParentCell()->GetbhkWorld();
        DecalCaster::GetInstance(DecalCaster::GetMaxDecalWidthLarge(), true)->Add(pWorld);
    }

    if (GetParentCell() &&
        GetParentCell()->GetbhkWorld() &&
        DecalCaster::GetInstance(DecalCaster::GetMaxDecalWidthSmall(), false)) {
        bhkWorld* pWorld = GetParentCell()->GetbhkWorld();
        DecalCaster::GetInstance(DecalCaster::GetMaxDecalWidthSmall(), true)->Add(pWorld);
    }

    if (spCameraRigidBody == static_cast<bhkRigidBody*>(nullptr)) {
        hkVector4 hkv;
        bhkSphereShape* pSphere = new bhkSphereShape;
        pSphere->Init(0.001f);
        bhkRigidBodyCinfo kInfo;
        kInfo.SetShape(pSphere);
        kInfo.SetCollisionFilter(0x30021u);
        bhkRigidBody* pBody = new bhkRigidBody(kInfo);
        spCameraRigidBody = pBody;
        spCameraRigidBody->SetActive(true);
        bhkRigidBody* pCameraBody = spCameraRigidBody.m_pObject;
        pCameraBody->SetMotionType(hkpMotion::MOTION_KEYFRAMED);
        spCameraRigidBody->GetCollidableRw()->m_broadPhaseHandle.m_objectQualityType = 9;
        NiPoint3 kLocation = GetLocationOnReference();
        NI2HK(hkv, kLocation);
        spCameraRigidBody->SetPosition(kLocation);
    }
    bhkRigidBody* pCameraBody = spCameraRigidBody.m_pObject;
    pCameraBody->Add(GetParentCell()->GetbhkWorld());
    bool bIncapacitated = IsUnConscious() || IsDead(false) || IsDying() || IsKnockedOut();
    bool bAddWobble = !bIncapacitated && !IsSleeping();
    if (bAddWobble) {
        AddGunWobble();
    }
    pModelLoader->UpdateQueuedReferencePriorities(nullptr);
    TESClimate* pClimate = apCell->GetClimate();
    Sky::GetInstance()->SetCurrentClimate(pClimate, false);
    BSShaderManager::SetImageSpaceParameters(&apCell->GetUsableImageSpace()->Data);
    TESWorldSpace* pWorldSpace = apCell->GetWorldSpace();
    bool bInBorderRegion = false;
    bInBorderContainedCell = false;
    if (pBorderRegions) {
        pBorderRegions->RemoveAll();
        NiTPrimitiveArray<TESRegion*>* pOldBorderRegions = pBorderRegions;
        if (pOldBorderRegions) {
            delete pOldBorderRegions;
        }
        pBorderRegions = nullptr;
    }
    int32_t iX = apCell->GetDataX();
    int32_t iY = apCell->GetDataY();
    float fLowX = static_cast<float>(iX << 12);
    float fHighX = fLowX + 4096.0;
    float fLowY = static_cast<float>(iY << 12);
    float fHighY = fLowY + 4096.0;
    TESRegionPoint sw(fLowX, fLowY);
    TESRegionPoint nw(fLowX, fHighY);
    TESRegionPoint se(fHighX, fLowY);
    TESRegionPoint ne(fHighX, fHighY);
    CurrentRegionSoundList.RemoveAll();
    FalloutAudioMedia::pCurrentRegionSoundData = nullptr;
    BSSimpleList<TESRegion*> OldRegionList;
    BSSimpleList<TESRegion*>* pOccupied = nullptr;
    // The loop tests the address &m_AllOccupiedRegions, which is never null.
    pOccupied = &static_cast<BSSimpleList<TESRegion*>&>(m_AllOccupiedRegions);
    for (BSSimpleList<TESRegion*>* pIter = pOccupied; pIter && pIter->m_item; pIter = pIter->m_pkNext) {
        TESRegion* pRegion = pIter->m_item;
        if (pRegion) {
            OldRegionList.AddHead(pRegion);
        }
    }
    m_AllOccupiedRegions.RemoveAll();
    TESRegionDataSound* pSoundOverrideRegion = nullptr;
    TESRegionList* pCellRegions = apCell->GetRegionList(true);
    for (BSSimpleList<TESRegion*>* pIter = static_cast<BSSimpleList<TESRegion*>*>(pCellRegions); pIter;
         pIter = pIter->m_pkNext) {
        TESRegion* pCurrentRegion = pIter->m_item;
        if (!pCurrentRegion) {
            break;
        }
        // The constructor from a world position on the reference's location.
        TESRegionPoint testPoint(data.kPosition);
        BSSimpleList<TESRegionSound*>* pRegSounds = nullptr;
        if (pCurrentRegion->GetDelete()) {
            continue;
        }
        if (pCurrentRegion->pWorldSpace && pCurrentRegion->pWorldSpace != GetWorldSpace()) {
            continue;
        }
        if (!pCurrentRegion->pPointLists || pCurrentRegion->pPointLists->IsEmpty()) {
            continue;
        }
        bool bFound = false;
        for (BSSimpleList<TESRegionPointList*>* pPointIter = pCurrentRegion->pPointLists; !bFound;
             pPointIter = pPointIter->m_pkNext) {
            if (!pPointIter) {
                break;
            }
            TESRegionPointList* pRPL = pPointIter->m_item;
            if (!pRPL) {
                break;
            }
            if (!pRPL->IsInsideRegion(&testPoint)) {
                continue;
            }
            m_AllOccupiedRegions.Add(pCurrentRegion);
            if (OldRegionList.IsInList(pCurrentRegion)) {
                OldRegionList.Remove(pCurrentRegion);
            }
            TESRegionDataSound* pRSD = static_cast<TESRegionDataSound*>(
                pCurrentRegion->pDataList->Find(REGION_DATA_SOUND_ID)
            );
            if (pRSD) {
                if (pRSD->bOverride &&
                    (!pSoundOverrideRegion ||
                     (pSoundOverrideRegion != pRSD && pSoundOverrideRegion->cPriority < pRSD->cPriority))) {
                    CurrentRegionSoundList.RemoveAll();
                    pSoundOverrideRegion = pRSD;
                }

                if (!pSoundOverrideRegion || pSoundOverrideRegion == pRSD) {
                    for (pRegSounds = &pRSD->sounds.soundList; pRegSounds && pRegSounds->m_item;
                         pRegSounds = pRegSounds->m_pkNext) {
                        // GetRegionSounds is a trivial accessor: &CurrentRegionSoundList, typed as the
                        // region sounds it holds.
                        GetRegionSounds()->AddHead(pRegSounds->m_item);
                    }
                    if (pRSD->GetMediaSetIncidentals()) {
                        FalloutAudioMedia::pCurrentRegionSoundData = pRSD;
                    }
                }
            }
            if (pCurrentRegion->TESForm::GetHasSpokenFlag()) {
                bInBorderRegion = true;
                if (pRPL->IsInsideRegion(&sw) &&
                    pRPL->IsInsideRegion(&nw) &&
                    pRPL->IsInsideRegion(&se) &&
                    pRPL->IsInsideRegion(&ne)) {
                    bInBorderContainedCell = true;
                }
            }
            if (pOccupiedRegion) {
                TESRegionDataWeather* pOccRWD = static_cast<TESRegionDataWeather*>(
                    pOccupiedRegion->pDataList->Find(REGION_DATA_WEATHER_ID)
                );
                TESRegionDataWeather* pCurRWD = static_cast<TESRegionDataWeather*>(
                    pCurrentRegion->pDataList->Find(REGION_DATA_WEATHER_ID)
                );
                if (pCurRWD &&
                    (!pOccRWD ||
                     (pCurRWD->bOverride && !pOccRWD->bOverride) ||
                     (pCurRWD->bOverride == pOccRWD->bOverride && pCurRWD->cPriority > pOccRWD->cPriority))) {
                    SetOccupiedRegion(pCurrentRegion);
                }
            }
            else {
                SetOccupiedRegion(pCurrentRegion);
            }
            bFound = true;
        }
        if (pCurrentRegion->TESForm::GetHasSpokenFlag()) {
            if (!pBorderRegions) {
                pBorderRegions = ::new NiTPrimitiveArray<TESRegion*>(1, 1);
            }
            pBorderRegions->Add(pCurrentRegion);
        }
    }
    if (bEnableBorderRegion.Bool() &&
        pWorldSpace &&
        pWorldSpace->GetHasBorderRegion() &&
        !TES::pInstance->CellTestRunning() &&
        !bInBorderRegion) {
        TESObjectCELL* pGoodCell = nullptr;
        TESWorldSpace* pGoodWorldSpace = nullptr;
        if (pLastKnownGoodLocation) {
            int32_t iFormType = pLastKnownGoodLocation->GetFormType();
            if (iFormType == CELL_ID) {
                pGoodCell = static_cast<TESObjectCELL*>(pLastKnownGoodLocation);
            }
            else if (iFormType == WRLD_ID) {
                pGoodWorldSpace = static_cast<TESWorldSpace*>(pLastKnownGoodLocation);
            }
        }
        if (pGoodCell && pGoodCell != apCell) {
            bReturnToLastKnownGoodPosition = true;
        }
        else if (pGoodWorldSpace && pGoodWorldSpace != apCell->GetWorldSpace()) {
            bReturnToLastKnownGoodPosition = true;
        }
        else {
            NiPoint3 kOffset = GetLocationOnReference();
            kOffset -= LastKnownGoodPosition;
            if (kOffset.Length() > 2048.0) {
                bReturnToLastKnownGoodPosition = true;
            }
        }
    }
    TESRegionDataMap* pMapData = nullptr;
    if (pOccupiedRegion) {
        pMapData = static_cast<TESRegionDataMap*>(pOccupiedRegion->pDataList->Find(REGION_DATA_MAP_ID));
    }
    const char* pMapName = pMapData ? pMapData->GetMapName() : nullptr;
    Interface::SetRegionText(pOccupiedRegion, BSStringT<char>(pMapName), true);
    ResetAudioMarkerList();
    ResetMapMarkerStructList();
    // The regions left: their imposters are hidden.
    for (BSSimpleList<TESRegion*>* pIter = &OldRegionList; pIter && pIter->m_item; pIter = pIter->m_pkNext) {
        TESRegion* pRegion = pIter->m_item;
        if (pRegion) {
            TESRegionDataImposter* pImposterData = static_cast<TESRegionDataImposter*>(
                pRegion->pDataList->Find(REGION_DATA_IMPOSTER_ID)
            );
            if (pImposterData) {
                pImposterData->SetImpostersVisible(false, false);
            }
        }
    }
    // The regions the player is in: their imposters are added to the cell and shown at once.
    pOccupied = nullptr;
    // The loop tests the address &m_AllOccupiedRegions, which is never null.
    pOccupied = &static_cast<BSSimpleList<TESRegion*>&>(m_AllOccupiedRegions);
    for (BSSimpleList<TESRegion*>* pIter = pOccupied; pIter && pIter->m_item; pIter = pIter->m_pkNext) {
        TESRegion* pRegion = pIter->m_item;
        if (!pRegion) {
            continue;
        }
        TESRegionDataImposter* pImposterData = static_cast<TESRegionDataImposter*>(
            pRegion->pDataList->Find(REGION_DATA_IMPOSTER_ID)
        );
        if (!pImposterData) {
            continue;
        }
        for (BSSimpleList<TESObjectREFR*>* pList = &pImposterData->m_Imposters; pList; pList = pList->m_pkNext) {
            if (pList->m_item) {
                GetParentCell()->AddReference(pList->m_item, false);
                TESRegionDataImposter::SetImposterVisible(pList->m_item, true, false);
            }
        }
    }
    OldRegionList.RemoveAll();
}

void PlayerCharacter::SetDialoguewithPlayer(bool abFlag) {
    StopVanityMode();
    if (abFlag) {
        PlayerCharacter::fsaveSitHeadingDelta = fSitHeadingDelta;
    }
    else {
        fSitHeadingDelta = PlayerCharacter::fsaveSitHeadingDelta;
    }
    // Only the flag's byte is used, zero-extended, whatever the caller left in the upper bytes.
    uint8_t ucFlag = static_cast<uint8_t>(abFlag);
    MobileObject::SetDialoguewithPlayer(ucFlag != 0);
}

float PlayerCharacter::GetHeading(bool abIgnoreImmobile) {
    float lfHeading = Actor::GetHeading(false) + fSitHeadingDelta;
    return ClampAngle(lfHeading);
}

void PlayerCharacter::Update(float afDelta) {
    NiPoint3 kVerticalDelta;
    bool bsearch;
    NiPoint3 kDefaultSideVec;
    alignas(16) NiMatrix3 currentMatrix;
    float fItemDetection;
    float fImprovedSpotting;
    uint16_t smoveMode;
    TESObjectWEAP* pweapon;
    Controls* pcontrols = appMain->GetControls();
    time_followerStealthing = time_followerStealthing - afDelta;
    if (bVATSForce1stPerson) {
        if (bVATSReturnPlayerTo1stPerson) {
            pPlayer->SetFirstPerson(true);
        }
        else {
            pPlayer->SetFirstPerson(false);
        }
        bVATSForce1stPerson = false;
    }
    if (pVATS.eMode) {
        if (BSAudio::QInstance().GetMasterSecondaryVolume() > 0.9900000095367432 &&
            fVATSCameraEffectVolume.Float() < 0.9900000095367432) {
            float fValue = fVATSCameraEffectVolume.Float();
            BSAudio::QInstance().SetMasterSecondaryVolume(fValue);
            if (!BSAudio::kVatsLoopHandle.IsValid()) {
                BSAudio::kVatsLoopHandle = BSAudio::QInstance().GetSoundHandleByName("UIVATSLOOP", 49u);
            }
            BSAudio::kVatsLoopHandle.Play(false);
        }
    }
    else {
        BSAudio::QInstance();
        if (BSAudio::kVatsLoopHandle.IsValid()) {
            BSAudio::QInstance();
            BSAudio::kVatsLoopHandle.FadeOutAndRelease(1500u);
        }
        if (BSAudio::QInstance().GetMasterSecondaryVolume() < 0.9900000095367432) {
            BSAudio::QInstance().ClearSecondaryVolumes();
        }
    }
    if (this->fKillCamCooldown > 0.0) {
        this->fKillCamCooldown = this->fKillCamCooldown - afDelta;
    }
    char cInSlowMoCam = 0;
    if (!(this->fTimeInSlowMoCam > 0.0)) {
        this->GetAnimation(!this->Is3rdPerson())->SetGlobalTimeMultiplier(1.0f);
        this->GetAnimation(this->Is3rdPerson())->SetGlobalTimeMultiplier(1.0f);
    }
    else {
        if (pcontrols->GetUserAction(Controls::UACTN_ACTIVATE, 1)) {
            this->fTimeInSlowMoCam = 0.0f;
        }
        this->fTimeInSlowMoCam = this->fTimeInSlowMoCam - afDelta;
        appTimer.SetGlobalTimeMultiplier(0.3f, false);
        if (this->bIgnoresGTM) {
            this->GetAnimation(!this->Is3rdPerson())->SetGlobalTimeMultiplier(3.3333333f);
            this->GetAnimation(this->Is3rdPerson())->SetGlobalTimeMultiplier(3.3333333f);
        }
        if (!(this->fTimeInSlowMoCam <= 0.0)) {
            cInSlowMoCam = 1;
        }
        else {
            this->ResetKillCam(this->eKillCameraSetting, false);
        }
    }
    if (this->GetBlockCounterTimer() > 0.0) {
        float fBlockCounterTimer = this->GetBlockCounterTimer() - afDelta;
        this->SetBlockCounterTimer(fBlockCounterTimer);
    }
    if (this->GetActorValue(ActorValue::TURBO) <= 0 || cInSlowMoCam) {
        if (!cInSlowMoCam) {
            TESImageSpaceModifier* pTurboDOF = static_cast<TESImageSpaceModifier*>(
                TESForm::GetFormByEditorID("TurboDOF50")
            );
            ImageSpaceModifierInstanceForm::Stop(pTurboDOF);
            if (this->bTurboISM) {
                appTimer.RestoreGlobalTimeMultiplierCache();
            }
            this->bTurboISM = false;
        }
    }
    else {
        if (!this->bTurboISM) {
            BSTimer::SaveGlobalTimeMultiplier();
            TESImageSpaceModifier* pTurboDOF = static_cast<TESImageSpaceModifier*>(
                TESForm::GetFormByEditorID("TurboDOF50")
            );
            ImageSpaceModifierInstanceForm::Trigger(pTurboDOF, 1.0f, nullptr);
            this->bTurboISM = true;
        }
        appTimer.SetGlobalTimeMultiplier(fTurboTimeMultiplier.Float(), false);
        float fPersonalMultiplier = 1.0 / BSTimer::QGlobalTimeMultiplier();
        this->GetAnimation(!this->Is3rdPerson())->SetGlobalTimeMultiplier(fPersonalMultiplier);
        this->GetAnimation(this->Is3rdPerson())->SetGlobalTimeMultiplier(fPersonalMultiplier);
    }
    if (this->pQueuedWeaponAttach && !this->pCurrentProcess->Get3DUpdateFlag(RESET_MODEL)) {
        this->AttachWeapon(this->pQueuedWeaponAttach);
        this->pQueuedWeaponAttach = nullptr;
    }

    if (this->bBlockActivate && !appMain->IsInMenuMode()) {
        this->fBlockActivateTimer = this->fBlockActivateTimer - afDelta;
        if (this->fBlockActivateTimer < 0.0) {
            this->fBlockActivateTimer = 0.0f;
            this->bBlockActivate = false;
        }
    }
    iActorsDismemberedThisFrame = 0;
    TESObjectREFR* phitRef = TESObjectREFR::FindReferenceFor3D(this->spTargeted3D.m_pObject);
    HUDMainMenu::SetTargetType(phitRef, this->bTarget3DDistant);
    this->spTargeted3D = static_cast<NiAVObject*>(nullptr);
    this->ResetCachedGunSkill();
    alignas(16) AutoMemContext tempMemContext(
        MEM_CONTEXT_PLAYER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
        3137
    );
    SceneGraph* pWorldRoot = Main::GetWorldSceneGraph();
    if (this->pCurrentProcess->GetCurrentWeapon()) {
        pweapon = static_cast<TESObjectWEAP*>(this->pCurrentProcess->GetCurrentWeapon()->pContainerObj);
    }
    else {
        pweapon = nullptr;
    }
    Animation* panim = this->GetAnimation(false);
    Animation* panim1st = this->GetAnimation(true);
    bool bPlayerInput = false;
    smoveMode = 0;
    BSSoundHandle hndl;
    this->fDropAngleMod = 0.0;
    this->fLastDropAngleMod = 0.0;
    if (!(this->fTimeronAction > 0.0)) {
        this->iActionValue = 0;
    }
    else {
        double dSeconds = appTimer.fDelta;
        this->fTimeronAction = this->fTimeronAction - dSeconds;
    }
    this->UpdateConflictingObjects();
    this->pCurrentProcess->Update3dModel(this);
    if (this->GetBumpedRef()) {
        TESObjectREFR* pBumpedRef = this->GetBumpedRef();
        this->AddPlayerAction(PLAYER_ACTION_KNOCKING_OVER_OBJECTS, 2.0f, pBumpedRef);
    }
    if (this->GetBumpedActor() && this->fTimeronAction <= 0.0) {
        int32_t iSoundLevel = iSoundLevelLoud.Int();
        this->SetActionSoundValue(iSoundLevel);
    }

    bool bIronSights = this->GetIronSights();
    if (bIronSights && this->pCurrentProcess->GetWeaponDrawn()) {
        bool bDark = false;
        if (this->GetParentCell()->GetInterior()) {
            bDark = true;
        }
        else {
            Sky* psky = TES::pInstance->pSky;
            float ftimeOfDay = psky->GetCurrentGameHour();
            float fsunriseTime = psky->GetSunriseEnd();
            float fsunsetTime = psky->GetSunsetBegin();
            if (fsunsetTime <= static_cast<double>(ftimeOfDay) || fsunriseTime >= static_cast<double>(ftimeOfDay)) {
                bDark = true;
            }
        }
        if (pVATS.eMode || this->fTimeInSlowMoCam > 0.0) {
            bDark = false;
        }
        ItemChange* pcurrentWeaponItem = this->pCurrentProcess->GetCurrentWeapon();
        TESObjectWEAP* pcurrentWeapon = nullptr;
        if (pcurrentWeaponItem) {
            pcurrentWeapon = static_cast<TESObjectWEAP*>(pcurrentWeaponItem->pContainerObj);
        }
        if (pcurrentWeapon &&
            pcurrentWeapon->HasNightScope() &&
            !this->bNightVisionOn &&
            bDark &&
            (!pcurrentWeapon->HasModScope() || pcurrentWeaponItem->HasModEffectActive(14))) {
            this->bNightVisionOn = true;
            TESImageSpaceModifier* pNightVision = static_cast<TESImageSpaceModifier*>(
                TESForm::GetFormByNumericID(0x8CD8Cu)
            );
            ImageSpaceModifierInstanceForm::Trigger(pNightVision, 1.0f, nullptr);
        }
        else if (!bDark) {
            TESImageSpaceModifier* pNightVision = static_cast<TESImageSpaceModifier*>(
                TESForm::GetFormByNumericID(0x8CD8Cu)
            );
            ImageSpaceModifierInstanceForm::Stop(pNightVision);
            this->RemovePlayerAction(PLAYER_ACTION_IRON_SITES, nullptr);
            this->bNightVisionOn = false;
        }
        if (!this->GetPlayerAction(PLAYER_ACTION_IRON_SITES)) {
            this->AddPlayerAction(PLAYER_ACTION_IRON_SITES, 20.0f, nullptr);
        }
    }
    else {
        if (this->bNightVisionOn) {
            TESImageSpaceModifier* pNightVision = static_cast<TESImageSpaceModifier*>(
                TESForm::GetFormByNumericID(0x8CD8Cu)
            );
            ImageSpaceModifierInstanceForm::Stop(pNightVision);
            this->bNightVisionOn = false;
        }
        this->RemovePlayerAction(PLAYER_ACTION_IRON_SITES, nullptr);
    }
    if (!bIronSights || this->bHasSpotterActive || pVATS.eMode == VATS::VATS_PLAYBACK) {
        if (this->bHasSpotterActive && !bIronSights) {
            TESEffectShader* pSpotterShader = pDataHandler->SpotterEffectItem();
            ReferenceProcessLists.FinishMagicShaderHitEffectAll(pSpotterShader);
            this->bHasSpotterActive = false;
        }
    }
    else {
        fImprovedSpotting = 0.0;
        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_IMPROVED_SPOTTING, pPlayer, &fImprovedSpotting);
        if (fImprovedSpotting > 0.0) {
            this->bHasSpotterActive = true;
            this->SpotterEffectNearbyEnemies();
        }
    }
    if (this->GetActorValue(ActorValue::NIGHT_EYE_BONUS) <= 0) {
        if (this->bHasCateyeActive && this->GetActorValue(ActorValue::NIGHT_EYE_BONUS) <= 0) {
            ReferenceProcessLists.FinishMagicShaderHitEffectAll(pDataHandler->pCateyeMobileShader);
            this->bHasCateyeActive = false;
        }
    }
    else {
        this->bHasCateyeActive = true;
    }
    fItemDetection = 0.0;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_ITEM_DETECTION, pPlayer, &fItemDetection);
    this->fCheckForItems = this->fCheckForItems - afDelta;
    if (fItemDetection > 0.0 && this->fCheckForItems <= 0.0 && bIronSights) {
        this->fCheckForItems = fItemDetectionTimer.Float();
        this->ShaderNearbyItems();
    }
    this->UpdatePlayerActions();
    if (this->IsHardcore()) {
        this->UpdateHardcoreMode();
    }
    this->pCurrentProcess->CalculateLightValue(this, false);
    if (!this->IsStandingOnFurniture() || this->GetSitSleepState()) {
        this->RemovePlayerAction(PLAYER_ACTION_STAND_ON_TABLE_CHAIR, nullptr);
    }
    else if (!this->GetPlayerAction(PLAYER_ACTION_STAND_ON_TABLE_CHAIR)) {
        this->AddPlayerAction(PLAYER_ACTION_STAND_ON_TABLE_CHAIR, 20.0f, nullptr);
    }
    bhkCharacterController* pCharController = this->GetCharController();
    if (pCharController && static_cast<uint8_t>(pCharController->IsOnElevator())) {
        if (!this->bOnElevator) {
            ReferenceProcessLists.MoveFollowersOnToPlayer();
            this->bOnElevator = true;
        }
    }
    else {
        this->bOnElevator = false;
    }
    if (!(this->pCurrentProcess->GetDetectionModifierTimer() > 0.0)) {
        this->pCurrentProcess->SetDetectionModifier(0.0f);
    }
    else {
        this->pCurrentProcess->ModDetectionModifierTimer();
    }
    if (this->bBeingChased) {
        if (!(this->GetDetectionTimer() <= 0.0)) {
            this->ReduceDetectionTimer();
        }
        else {
            this->CheckGetAwayFromGuards();
            this->pCurrentProcess->SetDetectionTimer(fDetectionTimerSetting.Float());
        }
    }
    if (!(this->fStealWarningTimer > 0.0)) {
        this->fStealWarningTimer = -1.0f;
    }
    else {
        this->fStealWarningTimer = this->fStealWarningTimer - afDelta;
    }
    if (!(this->fPickPocketWarningTimer > 0.0)) {
        this->fPickPocketWarningTimer = -1.0f;
    }
    else {
        this->fPickPocketWarningTimer = this->fPickPocketWarningTimer - afDelta;
    }
    if (!(this->fSortActorDistanceTimer <= 0.0)) {
        this->fSortActorDistanceTimer = this->fSortActorDistanceTimer - afDelta;
    }
    else {
        ReferenceProcessLists.SortActorsCloseToPlayer();
        this->fSortActorDistanceTimer = fSortActorDistanceListTimer.Float();
    }
    if (this->bReturnToLastKnownGoodPosition) {
        this->bReturnToLastKnownGoodPosition = false;
        this->ReturnToLastKnownGoodPosition(true);
    }
    if (this->GetForceActivateRef()) {
        this->GetForceActivateRef()->Activate(this, false, nullptr, 1);
        this->SetForceActivateRef(nullptr);
    }
    else {
        this->pCurrentProcess->RandomlyPlaySpecialIdles(this);
        int32_t iPostAnimationActions = this->pCurrentProcess->GetPostAnimationActions();
        this->pCurrentProcess->ClearPostAnimationActions();
        if ((iPostAnimationActions & 4) != 0 && !this->pCurrentProcess->SitOrSleep(this)) {
            this->pCurrentProcess->SetAnimActionSuccess(0);
        }
        if ((iPostAnimationActions & 8) != 0 && !this->pCurrentProcess->ProcessGetUp(this)) {
            this->pCurrentProcess->SetAnimActionSuccess(0);
        }
        uint16_t slastMoveMode = this->GetMoveMode();
        if (this->IsSneaking() && this->IsMoving()) {
            if (Interface::GetDetectionLevel() < 3 && this->bActorinSneakRange) {
                if (!(this->fsecondSneaking > 1.0)) {
                    this->fsecondSneaking = this->fsecondSneaking + afDelta;
                }
                else {
                    this->UseSkill(ActorValue::SNEAK, 0);
                    this->fsecondSneaking = 0.0f;
                }
            }
        }
        else if (this->IsSwimming() && this->IsMoving()) {
            if (!(this->fsecondSwimming > 1.0)) {
                this->fsecondSwimming = this->fsecondSwimming + afDelta;
            }
            else {
                this->fsecondSwimming = 0.0f;
            }
        }
        else if ((slastMoveMode & ActorMover::MF_RUNNING) != 0 && this->IsMoving()) {
            if (!(this->fsecondRunning > 1.0)) {
                this->fsecondRunning = this->fsecondRunning + afDelta;
            }
            else {
                this->fsecondRunning = 0.0f;
            }
        }
        smoveMode = static_cast<uint16_t>(
            slastMoveMode &
            (ActorMover::MF_SNEAKING | ActorMover::MF_SWIMMING | ActorMover::MF_FALL | ActorMover::MF_SLIDE)
        );
        this->pPenetrationDetection->Update();
        if (this->UpdateHeadingAndLooking(afDelta, false, &smoveMode, false)) {
            bPlayerInput = true;
        }
        if (((PlayerCharacter::bVanityMode || PlayerCharacter::bZoom3rdPersonCamera) &&
             this->pCurrentProcess->GetFadeState() == BaseProcess::FADE_NORMAL) ||
            !static_cast<HighProcess*>(this->pCurrentProcess)->HighProcess::FadeUpdate(this)) {
            if (this->pCurrentProcess->GetKnockState() || this->IsParalyzed()) {
                pVATS.QuitVATSPlayback(false, false);
                this->bAiControlledActivate = false;
                pCurrentWater = nullptr;
                if (!this->b3rdPerson) {
                    this->ForceTemp3rdPerson(true);
                }
            }
            if (this->GetUnderAIControl() || this->IsDead(false)) {
                if (this->bActually3rdPerson &&
                    !this->b3rdPerson &&
                    this->GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
                    this->Show1stPerson(true);
                }
                this->pActorMover->ForceMoveMode(smoveMode);
                if (!(fAiControlTimer > 15.0) ||
                    (this->GetSitSleepState() && this->GetSitSleepState() != SIT_SLEEP_IS_SITTING)) {
                    if (this->GetInSleepPackage()) {
                        fAiControlTimer = fAiControlTimer + afDelta;
                        if (this->pCurrentProcess->IsPackageDone(this) ||
                            this->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                            this->pCurrentProcess->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
                            !this->pCurrentProcess->GetPackageThatIsRunning()) {
                            this->SetAIControlledToPosition(false);
                            this->pCurrentProcess->ClearRunOncePackage();
                            fAiControlTimer = 0.0f;
                        }
                    }
                    else if (this->GetAIControlledFromPosition()) {
                        fAiControlTimer = fAiControlTimer + afDelta;
                        if (this->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                            this->SetAIControlledFromPosition(false);
                            this->pCurrentProcess->ClearRunOncePackage();
                            fAiControlTimer = 0.0f;
                        }
                    }
                    if (this->bWant3rdPerson == this->b3rdPerson) {
                        this->Actor::Process(afDelta);
                    }
                    static_cast<PlayerMover*>(this->pActorMover)->PlayerMover::SetSideVector(NiPoint3::ZERO);
                    this->pActorMover->Update(afDelta);
                    if (!(fPlayerDeathReloadTime.Float() > 0.0)) {
                        if (this->GetLifeState() == ACTOR_LIFE_STATE_DEAD && !PlayerCharacter::bShowDeathMessage) {
                            this->DetachCharController();
                            Interface::SetGunScopeVisible(false);
                            if (pSaveLoadManager->HasMostRecentSaveGame()) {
                                const char* pMenuOption = sMiscPlayerDeadMenuOption.String();
                                const char* pLoadOption = sMiscPlayerDeadLoadOption.String();
                                Interface::CreateMessageMenu(
                                    sMiscPlayerDeadMessage.String(),
                                    static_cast<const char*>(nullptr),
                                    static_cast<const char*>(nullptr),
                                    &PlayerCharacter::DeathMenu,
                                    1,
                                    23,
                                    0.0f,
                                    0.0f,
                                    pLoadOption,
                                    pMenuOption,
                                    nullptr
                                );
                            }
                            else {
                                StartMenu::ChooseMainMenu();
                            }
                            PlayerCharacter::bShowDeathMessage = true;
                        }
                    }
                    else if (
                        this->GetLifeState() == ACTOR_LIFE_STATE_DEAD || this->GetLifeState() == ACTOR_LIFE_STATE_DYING
                    ) {
                        if (fPlayerDeathReloadTime_0 < 0.0) {
                            Interface::SetGunScopeVisible(false);
                            fPlayerDeathReloadTime_0 = fPlayerDeathReloadTime.Float();
                        }
                        fPlayerDeathReloadTime_0 = fPlayerDeathReloadTime_0 - afDelta;
                        if (fPlayerDeathReloadTime_0 < 0.0) {
                            fPlayerDeathReloadTime_0 = -1.0f;
                            if (!pSaveLoadManager->LoadMostRecentSaveGame()) {
                                StartMenu::ChooseMainMenu();
                            }
                        }
                    }
                    else {
                        fPlayerDeathReloadTime_0 = -1.0f;
                    }
                    this->UpdateAnimation();
                    if (this->b3rdPerson) {
                        this->PickAnimations(PlayerCharacter::fWalkSpeedMult, PlayerCharacter::fPCTurnScale);
                    }
                    this->b3rdPerson = !this->b3rdPerson;
                    if (this->b3rdPerson) {
                        this->PickAnimations(PlayerCharacter::fWalkSpeedMult, PlayerCharacter::fPCTurnScale);
                    }
                    this->Character::Update(afDelta);
                    this->UpdateAnimationMovement(this->GetAnimation(!this->b3rdPerson), afDelta);
                    this->b3rdPerson = !this->b3rdPerson;
                    this->Character::Update(afDelta);
                    this->UpdateAnimationMovement(this->GetAnimation(!this->b3rdPerson), afDelta);
                    this->UpdateCamera(false, false);
                    this->GetParentCell()->TestObjectAgainstMultibounds(this, true, false);
                    NiNode* p3D = this->Get3D();
                    BSShaderManager::GetShadowSceneNode(0)->UpdateObjectLighting(p3D, true);
                    this->pLastKnownMusicType = this->GetParentCell()->GetMusicType(nullptr);
                    if (this->bAiControlledActivate) {
                        if (pCurrentWater) {
                            if ((fangryFace_guard & 1) == 0) {
                                fangryFace_guard |= 1u;
                                PlayerCharacter::fDrinkDelay = fDrinkRepeatRate.Float();
                            }
                            if ((pcontrols->GetUserAction(Controls::UACTN_ACTIVATE, 1) ||
                                 pcontrols->GetUserAction(Controls::UACTN_ACTIVATE, 0)) &&
                                !this->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
                                this->GetAnimation()->SetLoopCount(AGS_SPECIAL_IDLE, 1);
                                PlayerCharacter::fDrinkDelay = PlayerCharacter::fDrinkDelay - afDelta;
                                while (PlayerCharacter::fDrinkDelay < 0.0) {
                                    PlayerCharacter::fDrinkDelay = PlayerCharacter::fDrinkDelay +
                                        fDrinkRepeatRate.Float();
                                    if (pCurrentWater) {
                                        pCurrentWater->Activate(nullptr, this, false, nullptr, 0);
                                    }
                                }
                            }
                            else {
                                alignas(16) BSSoundHandle actLoopHandle;
                                pPlayer->m_Extra.GetActivateLoopSound(actLoopHandle);
                                actLoopHandle.Stop();
                                this->GetAnimation()->SetLoopCount(AGS_SPECIAL_IDLE, 0);
                                PlayerCharacter::fDrinkDelay = fDrinkRepeatRate.Float();
                                pCurrentWater = nullptr;
                            }
                        }
                        if (this->GetAnimation()->SpecialIdleDonePlaying() &&
                            this->pCurrentProcess->GetAnimAction() == ANIM_ACTION_NONE &&
                            this->pCurrentProcess->GetKnockState() == KNOCK_STATE_NORMAL) {
                            this->bAiControlledActivate = false;
                            pCurrentWater = nullptr;
                        }
                    }
                    if (pcontrols->GetUserAction(Controls::UACTN_QUICKLOAD, 1) && pSaveLoadManager->Quickload()) {
                        fPlayerDeathReloadTime_0 = -1.0f;
                    }

                    if (!this->IsDead(false) && !this->IsDying()) {
                        this->UpdateMagic(false, false);
                        if (pcontrols->GetUserAction(Controls::UACTN_ACTIVATE, 1) &&
                            !this->IsPipboyActive() &&
                            !Interface::IsInMenuMode() &&
                            !this->IsParalyzed() &&
                            !this->GetTempPerson()) {
                            Actor* pActivateTarget = static_cast<Actor*>(Interface::GetActivateREFR());
                            char cNotActivated = 1;
                            bool bActivationBlocked = false;
                            if (pActivateTarget && pActivateTarget->IsFurniture()) {
                                bActivationBlocked = bIronSights;
                            }

                            if (this->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
                                bActivationBlocked = true;
                            }
                            if (pActivateTarget && pActivateTarget->IsProjectile()) {
                                ArrowProjectile* pArrow = dynamic_cast<ArrowProjectile*>(pActivateTarget);
                                if (pArrow && pArrow->eState == ArrowProjectile::STATE_FLYING) {
                                    bActivationBlocked = true;
                                }
                            }

                            if (pActivateTarget && pActivateTarget->IsActor()) {
                                BaseProcess* pProcess = pActivateTarget->pCurrentProcess;
                                if (pProcess->GetFadeState() == BaseProcess::FADE_OUT_DISABLE ||
                                    pProcess->GetFadeState() == BaseProcess::FADE_OUT_DELETE) {
                                    bActivationBlocked = true;
                                }
                            }

                            switch (this->GetSitSleepState() - 1) {
                                case 0:
                                case 1:
                                case 2:
                                case 4:
                                case 5:
                                case 6:
                                case 7:
                                case 9: {
                                    bActivationBlocked = true;
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                            if (this->GetAnimAction() != ANIM_ACTION_NONE) {
                                bActivationBlocked = true;
                            }
                            if (this->bWant3rdPerson != this->b3rdPerson) {
                                bActivationBlocked = true;
                            }
                            if (this->GetBlockActivate()) {
                                bActivationBlocked = true;
                            }
                            bool bNothingActivated = false;
                            if (!bActivationBlocked) {
                                Interface::SetInfoForRef(nullptr, false, false);
                                if (pActivateTarget) {
                                    BSSimpleList<BGSEntryPointFunctionDataActivateChoice*> ChoiceList;
                                    BGSEntryPoint::HandleEntryPoint(
                                        BGSEntryPoint::ACTIVATE,
                                        this,
                                        pActivateTarget,
                                        &ChoiceList,
                                        pActivateTarget
                                    );
                                    if (ChoiceList.IsEmpty()) {
                                        int32_t iCount = pActivateTarget->GetCount();
                                        if (pActivateTarget->Activate(this, false, nullptr, iCount)) {
                                            cNotActivated = 0;
                                        }
                                    }
                                    else if (!MessageMenu::AddMessageToQueue(&ChoiceList, pActivateTarget)) {
                                        int32_t iCount = pActivateTarget->GetCount();
                                        if (pActivateTarget->Activate(this, false, nullptr, iCount)) {
                                            cNotActivated = 0;
                                        }
                                    }
                                }
                                else {
                                    bNothingActivated = true;
                                }
                                if (cNotActivated && this->pCurrentProcess->GetCurrentFurniture()) {
                                    TESObjectREFR* pFurniture = this->pCurrentProcess->GetCurrentFurniture();
                                    bNothingActivated = !pFurniture->Activate(this, false, nullptr, 1);
                                }
                                else if (cNotActivated) {
                                    bNothingActivated = true;
                                }
                            }
                            if (bNothingActivated || bActivationBlocked) {
                                this->m_Extra.GetCreatureAwakeSound(hndl);
                                TESSound* pnoactSound = FalloutAudio::GetNoActivationSound();
                                if (!hndl.IsValid() && pnoactSound) {
                                    const char* pFilename = pnoactSound->GetFilename();
                                    hndl = BSAudio::QInstance().GetSoundHandleByFilename(
                                        pFilename,
                                        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                            BSGameSound::SOUND_FLAG_ONE_SHOT,
                                        pnoactSound
                                    );
                                }

                                if (!hndl.IsPlaying()) {
                                    hndl.Play(false);
                                }
                                this->m_Extra.SetCreatureAwakeSound(hndl);
                            }
                            bPlayerInput = true;
                        }
                    }
                }
                else {
                    if (this->GetAIControlledFromPosition() && this->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
                        this->SetAIControlledFromPosition(false);
                        this->pCurrentProcess->ClearRunOncePackage();
                    }
                    else if (this->IsDead(false) || this->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                        this->pCurrentProcess->ClearRunOncePackage();
                        this->SetAIControlledToPosition(false);
                        fAiControlTimer = 0.0f;
                    }
                    this->GetOutofFurnitureQuick();
                    fAiControlTimer = 0.0f;
                }
            }
            else {
                TESObjectCELL* pParentCell;
                NiPoint3 kForwardVec;
                if (pPlayer->pCurrentProcess->GetCurrentPackage()) {
                    pPlayer->pCurrentProcess->ClearCurrentPackage();
                }
                if (pPlayer->pCurrentProcess->GetRunOncePackage()) {
                    pPlayer->pCurrentProcess->ClearRunOncePackage();
                }
                this->ModPlayerGreetTimer();
                this->CheckPlayerGreetFlag();
                bool bHolsteredWeapon = false;
                this->fAmmoSwapButtonTimer = this->fAmmoSwapButtonTimer + afDelta;
                if (pcontrols->GetUserAction(Controls::UACTN_HOTKEY2, 1) &&
                    this->pCurrentProcess->GetCurrentWeapon() &&
                    this->IsWeaponDrawn()) {
                    int32_t iupState = InterfaceManager::CheckMenuButton(Interface::eButtonDPadUp, false);
                    int32_t ileftState = InterfaceManager::CheckMenuButton(Interface::eButtonDPadLeft, false);
                    int32_t irightState = InterfaceManager::CheckMenuButton(Interface::eButtonDPadRight, false);
                    bool bUpReleased = iupState == 2 || !iupState;
                    bool bLeftReleased = ileftState == 2 || !ileftState;
                    bool bRightReleased = irightState == 2 || !irightState;
                    if (!bUpReleased || (!bLeftReleased && !bRightReleased)) {
                        bool bStartReload = fPlayerAmmoSwapTimer.Float() <
                            static_cast<double>(this->fAmmoSwapButtonTimer);
                        this->AmmoSwapHelper(bStartReload, false);
                        if (bStartReload) {
                            this->fAmmoSwapButtonTimer = 0.0f;
                        }
                    }
                }

                if (pcontrols->GetUserAction(Controls::UACTN_READY_RELOAD_WEAPON, 1) ||
                    pcontrols->GetUserAction(Controls::UACTN_READY_RELOAD_WEAPON, 0)) {
                    if (!this->GetControlsDisabled(PlayerCharacter::DPC_FIGHTING)) {
                        fReloadTimer = fReloadTimer + afDelta;
                        if (this->CanMove() && this->GetAnimAction() == ANIM_ACTION_NONE) {
                            if (g_bWasReadyItemHeldLastFrame ||
                                ((!(fPlayerWeaponReloadTimer.Float() < static_cast<double>(fReloadTimer)) &&
                                  (pweapon && (pweapon->GetCurrentAmmo(pPlayer) || pweapon->IsRockItLauncher()))) ||
                                 !this->GetWantWeaponDrawn())) {
                                if (!g_bWasReadyItemHeldLastFrame && !this->GetWantWeaponDrawn()) {
                                    if ((this->GetMoveMode() & 2048) == 0) {
                                        this->SetWantWeaponDrawn(true);
                                    }
                                    g_bWasReadyItemHeldLastFrame = true;
                                }
                            }
                            else {
                                bHolsteredWeapon = true;
                                this->SetWantWeaponDrawn(false);
                                g_bWasReadyItemHeldLastFrame = true;
                            }
                        }
                    }
                }
                else if (!pcontrols->GetUserAction(Controls::UACTN_READY_RELOAD_WEAPON, 2)) {
                    fReloadTimer = 0.0f;
                    g_bWasReadyItemHeldLastFrame = false;
                }
                this->UpdateMenuModeButton(afDelta);
                if (pcontrols->GetUserAction(Controls::UACTN_TOGGLERUN, 1)) {
                    this->bAlwaysRun = !this->bAlwaysRun;
                }
                if (pcontrols->GetUserAction(Controls::UACTN_AUTOMOVE, 1)) {
                    this->bAutoMove = !this->bAutoMove;
                }
                if (this->bAutoMove && !Interface::IsPipBoyOpening() && !Interface::IsInMenuMode()) {
                    if (pcontrols->GetUserAction(Controls::UACTN_FORWARD, 0) ||
                        pcontrols->GetUserAction(Controls::UACTN_BACK, 0) ||
                        pcontrols->GetUserAction(Controls::UACTN_SLDRIGHT, 0) ||
                        pcontrols->GetUserAction(Controls::UACTN_SLDLEFT, 0)) {
                        this->bAutoMove = false;
                    }
                    else {
                        pcontrols->ForceUserAction(Controls::UACTN_FORWARD);
                    }
                }
                bool bCrouchToggled = false;
                if (pcontrols->GetUserAction(Controls::UACTN_CROUCH, 1)) {
                    if (!this->IsParalyzed() &&
                        !this->IsDead(false) &&
                        !this->IsKnockedOut() &&
                        !this->IsRestrained() &&
                        !this->IsUnConscious() &&
                        !this->GetControlsDisabled(PlayerCharacter::DPC_SNEAKING) &&
                        this->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                        switch (this->GetAnimAction() + 1) {
                            case 0:
                            case 3:
                            case 4:
                            case 5:
                            case 6:
                            case 7:
                            case 8:
                            case 10:
                            case 18: {
                                if ((smoveMode & 1024) != 0) {
                                    smoveMode &= ~ActorMover::MF_SNEAKING;
                                    hndl = BSAudio::QInstance().GetSoundHandleByName(
                                        "NPCHumanCrouchUp",
                                        BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                            BSGameSound::SOUND_FLAG_FIRST_PERSON
                                    );
                                    hndl.SetPosition(this->GetLocationOnReference());
                                    hndl.Play(false);
                                    this->UseActionPoints(ActionPoints::ACTION_STAND);
                                }
                                else {
                                    smoveMode |= ActorMover::MF_SNEAKING;
                                    hndl = BSAudio::QInstance().GetSoundHandleByName(
                                        "NPCHumanCrouchDown",
                                        BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                            BSGameSound::SOUND_FLAG_FIRST_PERSON
                                    );
                                    hndl.SetPosition(this->GetLocationOnReference());
                                    hndl.Play(false);
                                    this->UseActionPoints(ActionPoints::ACTION_CROUCH);
                                }
                                if (!this->bTemp1stPerson) {
                                    this->SetIronSights(false, false, false);
                                    this->SetBlock(false);
                                }
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                    }
                    bCrouchToggled = true;
                }
                currentMatrix = (*&pWorldRoot->GetCameraRoot()->m_kLocal).m_Rotate;
                NiPoint3 currentPos = *this->QLocationOnReference();
                NiPoint3 oldPos = currentPos;
                alignas(16) NiPoint3 sideVec = *::new (&kDefaultSideVec) NiPoint3(1.0f, 0.0f, 0.0f);
                currentMatrix.GetCol(1u, kForwardVec);
                if (this->GetNoCollision()) {
                    currentMatrix.GetCol(0, sideVec);
                }
                else {
                    kForwardVec.z = 0.0f;
                    kForwardVec.Unitize();
                }
                bool bdropping = false;
                if (this->eGrabType == PlayerCharacter::GRAB_INVENTORY_DROP) {
                    bdropping = this->UpdateDropSpring(afDelta);
                }
                this->HavokActivateDroppedReference();
                int32_t iStrafeValue = 0;
                int32_t iForwardValue = 0;
                int32_t iTurnValue = 0;
                if (InterfaceManager::Has360Controller()) {
                    iStrafeValue = pcontrols->GetControllerValue(0, 7);
                    iTurnValue = pcontrols->GetControllerValue(0, 9);
                    iForwardValue = pcontrols->GetControllerValue(0, 8);
                    if (std::abs(iStrafeValue) < 0x1EA9) {
                        iStrafeValue = 0;
                    }
                    if (std::abs(iForwardValue) < 0x1EA9) {
                        iForwardValue = 0;
                    }
                    if (std::abs(iTurnValue) < 0x21F1) {
                        iTurnValue = 0;
                    }
                    iStrafeValue = 100 * iStrafeValue / 0x7FFF;
                    iForwardValue = 100 * iForwardValue / 0x7FFF;
                    iTurnValue /= 0x7FFF;
                }
                if (!InterfaceManager::Has360Controller()) {
                    if (iStrafeValue) {
                        if (iStrafeValue <= 0) {
                            pcontrols->ForceUserAction(Controls::UACTN_SLDLEFT);
                        }
                        else {
                            pcontrols->ForceUserAction(Controls::UACTN_SLDRIGHT);
                        }
                        double dStrafe = static_cast<double>(std::abs(iStrafeValue));
                        PlayerCharacter::fPCTurnScale = fPlayerCharacterTurnMult.Float() * dStrafe;
                        if (PlayerCharacter::fPCTurnScale > 1.5) {
                            PlayerCharacter::fPCTurnScale = 1.5f;
                        }
                    }
                    if (std::abs(iForwardValue) >= std::abs(iStrafeValue)) {
                        if (iForwardValue) {
                            if (iForwardValue >= 0) {
                                pcontrols->ForceUserAction(Controls::UACTN_BACK);
                            }
                            else {
                                pcontrols->ForceUserAction(Controls::UACTN_FORWARD);
                            }
                            PlayerCharacter::fWalkSpeedMult = static_cast<double>(std::abs(iForwardValue)) *
                                0.019999999552965164;
                        }
                    }
                    else {
                        PlayerCharacter::fWalkSpeedMult = static_cast<double>(std::abs(iStrafeValue)) *
                            0.019999999552965164;
                        if (iStrafeValue >= 0) {
                            smoveMode |= ActorMover::MF_RIGHT;
                        }
                        else {
                            smoveMode |= ActorMover::MF_LEFT;
                        }
                    }
                }
                if (PlayerCharacter::fWalkSpeedMult > 1.0) {
                    PlayerCharacter::fWalkSpeedMult = 1.0f;
                }
                if (InterfaceManager::Has360Controller()) {
                    float fMotionLength;
                    NiPoint3 motionVec;
                    NiPoint3 kMoveVec;
                    float fTurnInput;
                    float fForwardInput;
                    float fStrafeInput = static_cast<double>(iStrafeValue) / 100.0;
                    fForwardInput = static_cast<double>(iForwardValue) / 100.0;
                    fTurnInput = static_cast<double>(iTurnValue) / 100.0;
                    if (this->GetAnimAction() == ANIM_ACTION_DODGE) {
                        fStrafeInput = 0.0f;
                        fForwardInput = 0.0f;
                        fTurnInput = 0.0f;
                    }
                    double dAbsStrafe = std::fabs(fStrafeInput);
                    float fBiasedForward = fForwardBias.Float() * fForwardInput;
                    bool bForwardDominant = std::fabs(fBiasedForward) > dAbsStrafe;
                    if (!bForwardDominant) {
                        if (!(fStrafeInput < 0.0)) {
                            if (fStrafeInput > 0.0) {
                                smoveMode |= ActorMover::MF_RIGHT;
                            }
                        }
                        else {
                            smoveMode |= ActorMover::MF_LEFT;
                        }
                    }
                    if (bForwardDominant) {
                        if (!(fForwardInput < 0.0)) {
                            if (fForwardInput > 0.0) {
                                smoveMode |= ActorMover::MF_FORWARD;
                            }
                        }
                        else {
                            smoveMode |= ActorMover::MF_BACKWARD;
                        }
                    }
                    kMoveVec.x = fStrafeInput;
                    kMoveVec.y = fForwardInput;
                    kMoveVec.z = 0.0f;
                    PlayerCharacter::fWalkSpeedMult = 0.0f;
                    char cFullSpeed = 0;
                    motionVec.x = fStrafeInput;
                    motionVec.y = fForwardInput;
                    motionVec.z = 0.0f;
                    if (!(motionVec.Length() > 1.0)) {
                        fMotionLength = motionVec.Length();
                    }
                    else {
                        fMotionLength = 1.0f;
                    }
                    if (PlayerCharacter::fWalkSpeedMult < 1.0) {
                        if (!(fMotionLength > 0.949999988079071)) {
                            if (!(fMotionLength > 0.800000011920929)) {
                                PlayerCharacter::fWalkSpeedMult = FLerp(0.1f, 1.8f, 0.0f, 0.8f, fMotionLength);
                            }
                            else {
                                PlayerCharacter::fWalkSpeedMult = FLerp(0.7f, 1.0f, 0.8f, 1.0f, fMotionLength);
                                cFullSpeed = 1;
                            }
                        }
                        else {
                            PlayerCharacter::fWalkSpeedMult = 1.0f;
                            cFullSpeed = 1;
                        }
                    }
                    if (kMoveVec.Length() > 0.0) {
                        if (pcontrols->GetUserAction(Controls::UACTN_RUN, 1) ||
                            pcontrols->GetUserAction(Controls::UACTN_RUN, 0) ||
                            cFullSpeed) {
                            if (this->pCurrentProcess->GetAnimAction() == ANIM_ACTION_BLOCK ||
                                this->IsOverEncumbered() ||
                                this->GetIronSights() ||
                                !(fGrabMaxWeightRunning.Float() >= static_cast<double>(this->fGrabObjectWeight))) {
                                PlayerCharacter::fWalkSpeedMult = fMaxAnalogWalkSpeed.Float();
                                smoveMode |= ActorMover::MF_WALKING;
                            }
                            else {
                                smoveMode |= ActorMover::MF_RUNNING;
                            }
                        }
                        else {
                            smoveMode |= ActorMover::MF_WALKING;
                        }
                    }
                    if (PlayerCharacter::fWalkSpeedMult > 1.0 && this->IsOverEncumbered()) {
                        PlayerCharacter::fWalkSpeedMult = 1.0f;
                    }
                    if (PlayerCharacter::fWalkSpeedMult > 1.0 && this->GetIronSights()) {
                        PlayerCharacter::fWalkSpeedMult = 1.0f;
                    }
                    double dAbsTurn = std::fabs(fTurnInput);
                    if (!(std::fabs(fStrafeInput) < dAbsTurn) || !(std::fabs(fForwardInput) < dAbsTurn)) {
                        PlayerCharacter::fPCTurnScale = std::fabs(fStrafeInput) * 1.5;
                    }
                    else {
                        PlayerCharacter::fPCTurnScale = std::fabs(fTurnInput) * 1.5;
                    }
                    float fMoveSpeed = this->GetCurrentSpeed() * PlayerCharacter::fWalkSpeedMult;
                    kMoveVec *= fMoveSpeed;
                    if (this->GetNoCollision()) {
                        NiMatrix3 rot;
                        float fLooking = -this->GetLooking();
                        rot.MakeXRotation(fLooking);
                        kMoveVec = kMoveVec * rot;
                    }
                    kMoveVec *= 4.25f;
                    currentPos += kMoveVec;
                    sideVec = kMoveVec;
                }
                else {
                    NiPoint2 kStick;
                    PlayerCharacter::fWalkSpeedMult = 1.0f;
                    smoveMode |= ActorMover::MF_NONCONTROLLER;
                    kStick.x = static_cast<float>(iStrafeValue);
                    kStick.y = static_cast<float>(iForwardValue);
                    if (kStick.Length() > 90.0) {
                        pcontrols->ForceUserAction(Controls::UACTN_RUN);
                        if (this->pCurrentProcess->GetAnimAction() != ANIM_ACTION_BLOCK &&
                            !this->IsOverEncumbered() &&
                            !this->GetIronSights()) {
                            smoveMode |= ActorMover::MF_RUNNING;
                        }
                    }
                    if (InterfaceManager::Has360Controller() || !this->bAlwaysRun) {
                        if ((pcontrols->GetUserAction(Controls::UACTN_RUN, 1) ||
                             pcontrols->GetUserAction(Controls::UACTN_RUN, 0)) &&
                            this->pCurrentProcess->GetAnimAction() != ANIM_ACTION_BLOCK &&
                            !this->IsOverEncumbered() &&
                            !this->GetIronSights()) {
                            smoveMode |= ActorMover::MF_RUNNING;
                        }
                    }
                    else if (
                        !pcontrols->GetUserAction(Controls::UACTN_RUN, 1) &&
                        !pcontrols->GetUserAction(Controls::UACTN_RUN, 0) &&
                        this->pCurrentProcess->GetAnimAction() != ANIM_ACTION_BLOCK &&
                        !this->IsOverEncumbered() &&
                        !this->GetIronSights()
                    ) {
                        pcontrols->ForceUserAction(Controls::UACTN_RUN);
                        smoveMode |= ActorMover::MF_RUNNING;
                    }
                    if (fGrabMaxWeightRunning.Float() < static_cast<double>(this->fGrabObjectWeight)) {
                        smoveMode &= ~ActorMover::MF_RUNNING;
                    }
                    float fCurrentSpeed = this->GetCurrentSpeed();
                    float fSideSpeed = fCurrentSpeed * PlayerCharacter::fWalkSpeedMult;
                    sideVec *= fSideSpeed;
                    float fForwardSpeed = fCurrentSpeed * PlayerCharacter::fWalkSpeedMult;
                    kForwardVec *= fForwardSpeed;
                    if (pcontrols->GetUserAction(Controls::UACTN_FORWARD, 1) ||
                        pcontrols->GetUserAction(Controls::UACTN_FORWARD, 0)) {
                        currentPos += kForwardVec;
                        if ((smoveMode & 512) == 0) {
                            smoveMode |= ActorMover::MF_WALKING;
                        }
                        smoveMode |= ActorMover::MF_FORWARD;
                    }
                    if (pcontrols->GetUserAction(Controls::UACTN_BACK, 1) ||
                        pcontrols->GetUserAction(Controls::UACTN_BACK, 0)) {
                        currentPos -= kForwardVec;
                        if ((smoveMode & 512) == 0) {
                            smoveMode |= ActorMover::MF_WALKING;
                        }
                        smoveMode |= ActorMover::MF_BACKWARD;
                    }
                    if (pcontrols->GetUserAction(Controls::UACTN_SLDLEFT, 1) ||
                        pcontrols->GetUserAction(Controls::UACTN_SLDLEFT, 0)) {
                        currentPos -= sideVec;
                        if ((smoveMode & 512) == 0) {
                            smoveMode |= ActorMover::MF_WALKING;
                        }
                        smoveMode |= ActorMover::MF_LEFT;
                    }
                    if (pcontrols->GetUserAction(Controls::UACTN_SLDRIGHT, 1) ||
                        pcontrols->GetUserAction(Controls::UACTN_SLDRIGHT, 0)) {
                        currentPos += sideVec;
                        if ((smoveMode & 512) == 0) {
                            smoveMode |= ActorMover::MF_WALKING;
                        }
                        smoveMode |= ActorMover::MF_RIGHT;
                    }
                }
                if (pVATS.eMode == VATS::VATS_PLAYBACK) {
                    smoveMode &= ~(
                        ActorMover::MF_FORWARD | ActorMover::MF_BACKWARD | ActorMover::MF_LEFT | ActorMover::MF_RIGHT |
                        ActorMover::MF_TURNLEFT | ActorMover::MF_TURNRIGHT
                    );
                }
                this->pActorMover->ForceMoveMode(smoveMode);
                if (bCrouchToggled) {
                    this->SetWalkRunFlagDirty();
                }
                this->pActorMover->UpdateTurnTimer(afDelta);
                this->UpdateTemp3rdPerson();
                this->UpdateTemp1stPerson();
                this->bsave3rdPerson = this->b3rdPerson;
                this->b3rdPersonSaved = true;
                this->b3rdPerson = true;
                if ((!pcontrols->GetUserAction(Controls::UACTN_USE, 0) ||
                     !bUserDebounceBow ||
                     !this->pCurrentProcess->GetCurrentAmmo() ||
                     !this->GetWantWeaponDrawn()) &&
                    this->pCurrentProcess->GetAutomaticShotDelay() != 0.0 &&
                    (pVATS.eMode != VATS::VATS_PLAYBACK ||
                     !pVATS.GetCurrentAction() ||
                     !pVATS.GetCurrentAction()->cFireShots)) {
                    this->pCurrentProcess->ClearAutomaticShotsToFire(false);
                }
                char cMovementBlocked = 0;
                if (this->IsKnockedOut() ||
                    this->IsParalyzed() ||
                    this->GetLifeState() ||
                    this->GetAnimAction() == ANIM_ACTION_STAGGER ||
                    this->pCurrentProcess->GetSitSleepState()) {
                    cMovementBlocked = 1;
                }
                if (cMovementBlocked || this->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
                    static_cast<PlayerMover*>(this->pActorMover)->PlayerMover::SetMoveMode(831u, false);
                }
                if (!cMovementBlocked && !this->GetControlsDisabled(PlayerCharacter::DPC_FIGHTING)) {
                    if (this->eGrabType == PlayerCharacter::GRAB_INVENTORY_DROP ||
                        this->eGrabType == PlayerCharacter::GRAB_TELEKINESIS) {
                        inextAttack = 0;
                        fpowerTimer = 0.0f;
                    }
                    else {
                        if (pcontrols->GetUserAction(Controls::UACTN_BLOCK, 1) ||
                            pcontrols->GetUserAction(Controls::UACTN_BLOCK, 0)) {
                            if (this->pCurrentProcess->GetAnimAction() != ANIM_ACTION_BLOCK &&
                                !this->GetIronSights() &&
                                this->bWant3rdPerson == this->bActually3rdPerson &&
                                !this->IsPipboyActive() &&
                                this->pCurrentProcess->CanAttack()) {
                                if ((!pweapon || (pweapon->IsMeleeWeapon() && this->IsWeaponDrawn())) &&
                                    (pweapon || this->IsWeaponDrawn())) {
                                    this->SetBlock(true);
                                }
                                else {
                                    this->SetIronSights(true, false, false);
                                }
                                if (!PlayerCharacter::bVanityMode || PlayerCharacter::bIsAutoVanityMode) {
                                    bPlayerInput = true;
                                }
                            }
                        }
                        else if (
                            (pcontrols->GetUserAction(Controls::UACTN_BLOCK, 2) ||
                             !pcontrols->GetUserAction(Controls::UACTN_BLOCK, 0) ||
                             this->IsPipboyActive()) &&
                            (this->GetIronSights() || this->pCurrentProcess->GetAnimAction() == ANIM_ACTION_BLOCK)
                        ) {
                            this->SetIronSights(false, false, false);
                            this->SetBlock(false);
                            bPlayerInput = true;
                        }
                        if (!this->pCurrentProcess->CanAttack() || (smoveMode & 2048) != 0) {
                            inextAttack = 0;
                            fpowerTimer = 0.0f;
                        }
                        else if (this->CheckUserInputAttacks()) {
                            bPlayerInput = true;
                        }
                    }
                }
                if (!cMovementBlocked &&
                    !this->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT) &&
                    pcontrols->GetUserAction(Controls::UACTN_JUMP, 1) &&
                    !this->IsOverEncumbered()) {
                    uint16_t usGroup = this->GetAnimation()->GetGroup(AGS_WEAPON);
                    if (!TESAnimGroup::IsPowerAttackAction(usGroup) && this->GetAnimAction() != ANIM_ACTION_DODGE) {
                        Animation* pAnimation = this->GetAnimation();
                        if (this->GetAnimAction() == ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM &&
                            pAnimation->GetCurrentSequence(AGS_MOVEMENT)) {
                            BSAnimGroupSequence* pSequence = pAnimation->GetCurrentSequence(AGS_MOVEMENT);
                            if (TESAnimGroupIsJumpingLandAnim(pSequence->GetAnimGroup(), nullptr)) {
                                this->SetAnimAction(ANIM_ACTION_NONE, nullptr);
                            }
                        }

                        if (this->GetAnimAction() != ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM) {
                            bool bBlockHeld = pcontrols->GetUserAction(Controls::UACTN_BLOCK, 0) != 0;
                            bool bblocking = this->GetBlocked();
                            if (bBlockHeld) {
                                if (Interface::GetActivateREFR()) {
                                    if (!bblocking && static_cast<uint8_t>(this->GetCharController()->CanJump())) {
                                        this->Jump();
                                    }
                                }
                                else if (this->GetCharController()->GetState() == HK_CHARACTER_ON_GROUND) {
                                    this->Dodge(smoveMode);
                                }
                            }
                            else if (static_cast<uint8_t>(this->GetCharController()->CanJump())) {
                                this->Jump();
                            }
                            this->ForcePhysicsGrabDrop();
                            bPlayerInput = true;
                        }
                    }
                }

                if (!this->IsTresPassing() && this->m_Extra.GetTrespassPackage()) {
                    this->m_Extra.RemoveTrespassPackage();
                }

                bsearch = false;
                if (pcontrols->GetUserAction(Controls::UACTN_RESTMENU, 1) && !this->GetPCControlFlags()) {
                    if (this->IsTresPassing()) {
                        Interface::ShowMessage(
                            sNoWaitTrespass.String(),
                            0,
                            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                            nullptr,
                            2.0f,
                            false
                        );
                    }
                    else if (this->IsAlarmForPlayerCharactaer()) {
                        Interface::ShowMessage(
                            sNoWaitWhileAlarmSounding.String(),
                            0,
                            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                            nullptr,
                            2.0f,
                            false
                        );
                    }
                    else {
                        TESObjectCELL* pCell = this->GetParentCell();
                        if (this->IsPointDeepUnderWater(this->GetLocationOnReference(), pCell, 0.875f)) {
                            Interface::ShowMessage(
                                sNoWaitUnderWater.String(),
                                0,
                                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                nullptr,
                                2.0f,
                                false
                            );
                        }
                        else if (ReferenceProcessLists.AreHostilActorsNear(pPlayer->GetInterior())) {
                            Interface::ShowMessage(
                                sNoWaitHostilActorsNear.String(),
                                0,
                                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                nullptr,
                                2.0f,
                                false
                            );
                        }
                        else if (
                            pPlayer->GetCharController()->GetState() == HK_CHARACTER_JUMPING ||
                            pPlayer->GetCharController()->GetState() == HK_CHARACTER_IN_AIR
                        ) {
                            Interface::ShowMessage(
                                sNoWaitInAir.String(),
                                0,
                                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                nullptr,
                                2.0f,
                                false
                            );
                        }
                        else if (!pPlayer->GetParentCell()->GetCantWaitHere() && pPlayer->GetCanWait()) {
                            if (ReferenceProcessLists.IsPlayerInRadiationArea() ||
                                (this->pCurrentProcess && this->pCurrentProcess->GetRadiationWaterDelta() != 0.0)) {
                                Interface::ShowMessage(
                                    sNoWaitInRadiation.String(),
                                    0,
                                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                    nullptr,
                                    2.0f,
                                    false
                                );
                            }
                            else if (pPlayer->HasDamageHealthEffect()) {
                                Interface::ShowMessage(
                                    sNoWaitTakingHealthDamage.String(),
                                    0,
                                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                    nullptr,
                                    2.0f,
                                    false
                                );
                            }
                            else {
                                Interface::CreateSleepMenu(false);
                            }
                        }
                        else {
                            Interface::ShowMessage(
                                sNoWaitInCell.String(),
                                0,
                                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                nullptr,
                                2.0f,
                                false
                            );
                        }
                    }
                }

                this->b3rdPerson = true;
                this->UpdateAnimation();
                this->PickAnimations(PlayerCharacter::fWalkSpeedMult, PlayerCharacter::fPCTurnScale);
                this->pCurrentProcess->UpdateKnockState(this);
                this->b3rdPerson = this->bsave3rdPerson;
                this->b3rdPersonSaved = false;
                if (this->IsSleeping()) {
                    this->PlayerSleep();
                }
                if (TES::QCollisionDisableFlag() || bDisablePlayerCollision.Bool()) {
                    if (!this->GetNoCollision()) {
                        bhkCharacterController* pcon = this->GetCharController();
                        if (pcon) {
                            pcon->SetPosition(currentPos);
                            pcon->SetLinearVelocity(hkVector4::getZero());
                        }
                        this->SetNoCollision(true);
                    }
                    alignas(16) NiPoint3 delta = currentPos - oldPos;
                    delta *= afDelta;
                    NiPoint3* pVerticalDelta = ::new (&kVerticalDelta) NiPoint3(0.0f, 0.0f, delta.z);
                    this->SetPosition(oldPos + *pVerticalDelta);
                }
                else {
                    this->SetNoCollision(false);
                }
                static_cast<PlayerMover*>(this->pActorMover)->PlayerMover::SetSideVector(sideVec);
                this->pActorMover->Update(afDelta);
                this->CheckBorderRegion();
                if ((smoveMode & 15) != 0) {
                    bPlayerInput = true;
                }
                if (pcontrols->GetUserAction(Controls::UACTN_QUICKSAVE, 1)) {
                    pSaveLoadManager->Quicksave();
                }
                if (pcontrols->GetUserAction(Controls::UACTN_QUICKLOAD, 1)) {
                    pSaveLoadManager->Quickload();
                }
                bool bVATSPressed = pcontrols->GetUserAction(Controls::UACTN_VATSMENU, 1) > 0;
                if (bDelayVATSMenu ||
                    (bVATSPressed &&
                     !this->GetControlsDisabled(PlayerCharacter::DPC_FIGHTING) &&
                     this->GetSitSleepState() == SIT_SLEEP_NORMAL &&
                     pVATS.eMode != VATS::VATS_PLAYBACK &&
                     !this->IsPipboyActive() &&
                     !Interface::IsInPipboyMenu() &&
                     !pVATS.eMode &&
                     (this->GetMoveMode() & 2048) == 0)) {
                    if (VATSMenu::BuildTargetList()) {
                        double dGlobalTimeMultiplier;
                        if (bVATSPressed) {
                            bVATSReturnPlayerTo1stPerson = !pPlayer->Is3rdPerson();
                        }
                        char cWeaponReady = 0;
                        ANIMATION_ACTION eAnimAction = this->GetAnimAction();
                        if (eAnimAction == ANIM_ACTION_NONE || eAnimAction == ANIM_ACTION_BLOCK) {
                            BSAnimGroupSequence* pWeaponSequence = panim->GetCurrentSequence(AGS_WEAPON);
                            BSAnimGroupSequence* pCheckSequence = panim->GetCurrentSequence(AGS_WEAPON);
                            TESAnimGroup* pAnimGroup = pWeaponSequence ? pWeaponSequence->GetAnimGroup() : nullptr;
                            ANIM_GROUP_ENUM eType = pAnimGroup ? pAnimGroup->GetType() : ANIM_GROUP_NONE;
                            if (eType >= ANIM_GROUP_RELOAD_W && eType < 199) {
                                if (pWeaponSequence) {
                                    panim->SkipNextBlend();
                                    panim->BlendOut(AGS_WEAPON, false);
                                    if (pCheckSequence) {
                                        panim1st->SkipNextBlend();
                                        panim1st->BlendOut(AGS_WEAPON, false);
                                    }
                                }
                            }
                            else {
                                cWeaponReady = 1;
                            }
                        }
                        if (!cWeaponReady ||
                            this->bTemp1stPerson ||
                            (dGlobalTimeMultiplier = BSTimer::QGlobalTimeMultiplier(),
                             dGlobalTimeMultiplier != BSTimer::GetGlobalTimeMultiplierTarget()) ||
                            this->bTemp3rdPerson ||
                            !this->IsWeaponDrawn() ||
                            this->IsVanityMode() ||
                            this->b3rdPerson ||
                            this->bWant3rdPerson != this->b3rdPerson) {
                            bDelayVATSMenu = true;
                            this->SetWantWeaponDrawn(true);
                            if (this->IsVanityMode() || this->b3rdPerson) {
                                this->StopVanityMode();
                                this->Show1stPerson(true);
                                this->bWant3rdPerson = false;
                                this->b3rdPerson = false;
                            }
                        }
                        else {
                            ImageSpaceModifierInstanceForm::Stop(TESImageSpaceModifier::GetGetHit());
                            appMain->OnIdle_UpdateImageSpace(true);
                            Interface::CreateVATSMenu();
                            bDelayVATSMenu = false;
                        }
                    }
                    else {
                        BSAudio::QInstance().GetSoundHandleByName("UIVATSEnterFail", 289u).Play(false);
                        bDelayVATSMenu = false;
                    }
                    bPlayerInput = true;
                }
                if (!this->IsPipboyActive() &&
                    !pCurrentWater &&
                    !this->bTemp1stPerson &&
                    (!this->bTemp3rdPerson || PlayerCharacter::bVanityMode) &&
                    !bDelayVATSMenu) {
                    if ((pcontrols->GetUserAction(Controls::UACTN_TOGGLEPOV, 1) ||
                         pcontrols->GetUserAction(Controls::UACTN_TOGGLEPOV, 0)) &&
                        !this->GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
                        if (!Interface::IsInMenuMode() && bFOVTargetReached) {
                            bPlayerInput = true;
                            PlayerCharacter::bZoom3rdPersonCamera = true;
                            if (!PlayerCharacter::bVanityMode) {
                                bWas3rdPersonBeforeVanityMode = this->bActually3rdPerson;
                                this->SetFirstPerson(false);
                                PlayerCharacter::fAutoVanityDelay = 0.0f;
                                PlayerCharacter::bIsAutoVanityMode = false;
                                PlayerCharacter::bVanityMode = true;
                                HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_VANITY);
                                PlayerCharacter::kVanityModePos.z = 0.0f;
                                PlayerCharacter::fStartingCameraY = PlayerCharacter::kVanityModePos.y;
                            }
                            PlayerCharacter::fVanityDelay = PlayerCharacter::fVanityDelay + afDelta;
                        }
                    }
                    else if (
                        pcontrols->GetUserAction(Controls::UACTN_TOGGLEPOV, 2) ||
                        (PlayerCharacter::bVanityMode && !PlayerCharacter::bIsAutoVanityMode)
                    ) {
                        PlayerCharacter::fAutoVanityDelay = 0.0f;
                        PlayerCharacter::bIsAutoVanityMode = false;
                        PlayerCharacter::bVanityMode = false;
                        HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_CHANGED);
                        if (fVanityModeDelay.Float() > static_cast<double>(PlayerCharacter::fVanityDelay) &&
                            (InterfaceManager::Has360Controller() ||
                             PlayerCharacter::kVanityModePos.y == PlayerCharacter::fStartingCameraY) &&
                            !this->GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
                            if (bWas3rdPersonBeforeVanityMode) {
                                this->SetFirstPerson(!this->bTemp3rdPersonSwitchBack);
                            }
                            this->bTemp3rdPerson = false;
                            this->bTemp3rdPersonSwitchBack = false;
                        }
                        PlayerCharacter::fVanityDelay = 0.0f;
                        bPlayerInput = true;
                        PlayerCharacter::bZoom3rdPersonCamera = false;
                        PlayerCharacter::kVanityModePos.x = 0.0f;
                        PlayerCharacter::kVanityModePos.z = 0.0f;
                    }
                }
                bool bFurnitureMove = false;
                if (this->pCurrentProcess->GetCurrentFurniture()) {
                    bFurnitureMove = (smoveMode & 15) != 0;
                }
                if ((pcontrols->GetUserAction(Controls::UACTN_ACTIVATE, 1) || bFurnitureMove) &&
                    !bDelayVATSMenu &&
                    !this->IsPipboyActive() &&
                    !Interface::IsInMenuMode() &&
                    !this->IsParalyzed() &&
                    !bdropping &&
                    !this->GetTempPerson()) {
                    Actor* pActivateTarget = static_cast<Actor*>(Interface::GetActivateREFR());
                    bool bNotActivated = true;
                    bool bDontActivate = false;
                    if (pActivateTarget && pActivateTarget->IsFurniture()) {
                        bDontActivate = bIronSights;
                    }

                    if (this->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
                        bDontActivate = true;
                    }
                    if (pActivateTarget && pActivateTarget->IsProjectile()) {
                        ArrowProjectile* pArrow = dynamic_cast<ArrowProjectile*>(pActivateTarget);
                        if (pArrow && pArrow->eState == ArrowProjectile::STATE_FLYING) {
                            bDontActivate = true;
                        }
                    }

                    if (pActivateTarget && pActivateTarget->IsActor()) {
                        BaseProcess* pProcess = pActivateTarget->pCurrentProcess;
                        if (!pActivateTarget->IsDead(false)) {
                            TESObjectCELL* pPlayerCell;
                            TESObjectCELL* pTargetCell = pActivateTarget->GetParentCell();
                            if (pActivateTarget->IsPointDeepUnderWater(
                                    pActivateTarget->GetLocationOnReference(),
                                    pTargetCell,
                                    0.875f
                                ) ||
                                (pPlayerCell = this->GetParentCell(),
                                 pPlayer
                                     ->IsPointDeepUnderWater(pPlayer->GetLocationOnReference(), pPlayerCell, 0.875f)) ||
                                (pProcess && pProcess->GetFadeState() == BaseProcess::FADE_OUT_DISABLE) ||
                                (pProcess && pProcess->GetFadeState() == BaseProcess::FADE_OUT_DELETE)) {
                                bDontActivate = true;
                            }
                        }
                    }

                    switch (this->GetSitSleepState() - 1) {
                        case 0:
                        case 1:
                        case 2:
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                        case 9: {
                            bDontActivate = true;
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    if (this->GetAnimAction() != ANIM_ACTION_NONE) {
                        bDontActivate = true;
                    }
                    if (this->bWant3rdPerson != this->b3rdPerson) {
                        bDontActivate = true;
                    }
                    if (this->GetBlockActivate()) {
                        bDontActivate = true;
                    }
                    bool bNothingActivated = false;
                    if (!bDontActivate) {
                        if (!pActivateTarget || !pActivateTarget->IsActor()) {
                            Interface::SetInfoForRef(nullptr, false, false);
                        }
                        if (!pActivateTarget || bFurnitureMove) {
                            bNothingActivated = true;
                        }
                        else {
                            alignas(16) BSSimpleList<BGSEntryPointFunctionDataActivateChoice*> ChoiceList;
                            BGSEntryPoint::HandleEntryPoint(
                                BGSEntryPoint::ACTIVATE,
                                this,
                                pActivateTarget,
                                &ChoiceList,
                                pActivateTarget
                            );
                            if (ChoiceList.IsEmpty()) {
                                int32_t iCount = pActivateTarget->GetCount();
                                if (pActivateTarget->Activate(this, false, nullptr, iCount)) {
                                    bNotActivated = false;
                                }
                                else if (pActivateTarget->GetLock()) {
                                    if (pActivateTarget->GetLock()->GetLockLevel(pActivateTarget) != 5 &&
                                        !pActivateTarget->GetInaccessible()) {
                                        bNotActivated = false;
                                    }
                                }
                            }
                            else if (!MessageMenu::AddMessageToQueue(&ChoiceList, pActivateTarget)) {
                                int32_t iCount = pActivateTarget->GetCount();
                                if (pActivateTarget->Activate(this, false, nullptr, iCount)) {
                                    bNotActivated = false;
                                }
                            }
                        }
                        if (bNotActivated && this->pCurrentProcess->GetCurrentFurniture()) {
                            TESObjectREFR* pFurniture = this->pCurrentProcess->GetCurrentFurniture();
                            bNothingActivated = !pFurniture->Activate(this, false, nullptr, 1);
                        }
                        else if (bNotActivated) {
                            bNothingActivated = true;
                        }
                    }
                    if (bNothingActivated || bDontActivate) {
                        BSSoundHandle kSoundHandle;
                        this->m_Extra.GetCreatureAwakeSound(kSoundHandle);
                        TESSound* pnoactSound = FalloutAudio::GetNoActivationSound();
                        if (!kSoundHandle.IsValid() && pnoactSound) {
                            const char* pFilename = pnoactSound->GetFilename();
                            kSoundHandle = BSAudio::QInstance().GetSoundHandleByFilename(
                                pFilename,
                                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                    BSGameSound::SOUND_FLAG_ONE_SHOT,
                                pnoactSound
                            );
                        }

                        if (!kSoundHandle.IsPlaying()) {
                            kSoundHandle.Play(false);
                        }
                        this->m_Extra.SetCreatureAwakeSound(kSoundHandle);
                    }
                    bPlayerInput = true;
                }
                if (PlayerCharacter::bIsAutoVanityMode) {
                    if (bPlayerInput || this->GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
                        this->StopVanityMode();
                    }
                    else {
                        PlayerCharacter::kAutoVanityModePos.z = PlayerCharacter::kAutoVanityModePos.z -
                            fVanityModeAutoXSpeed.Float() * 0.01745329238474369 * afDelta;
                        fzSin = fVanityModeAutoYSpeed.Float() * 0.01745329238474369 * afDelta + fzSin;
                        double dSin = std::sin(fzSin);
                        PlayerCharacter::kAutoVanityModePos.x = fVanityModeAutoYDegrees.Float() * dSin *
                            0.01745329238474369;
                        if (fzSin > 6.2831854820251465) {
                            fzSin = fzSin - 6.2831854820251465;
                        }
                    }
                }
                else if (!PlayerCharacter::bVanityMode) {
                    if (bPlayerInput) {
                        PlayerCharacter::fAutoVanityDelay = 0.0f;
                    }
                    else {
                        PlayerCharacter::fAutoVanityDelay = PlayerCharacter::fAutoVanityDelay + afDelta;
                    }
                    if (!bDisableAutoVanityMode &&
                        fVanityModeAutoDelay.Float() < static_cast<double>(PlayerCharacter::fAutoVanityDelay) &&
                        !this->GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
                        PlayerCharacter::fAutoVanityModeDistRevert = PlayerCharacter::kVanityModePos.y;
                        PlayerCharacter::kAutoVanityModePos.z = 0.0f;
                        if (this->b3rdPerson) {
                            PlayerCharacter::kAutoVanityModePos.x = this->GetLooking();
                        }
                        else {
                            PlayerCharacter::kAutoVanityModePos.x = 0.0f;
                        }
                        this->ForceTemp3rdPerson(true);
                        PlayerCharacter::bIsAutoVanityMode = true;
                        PlayerCharacter::bVanityMode = true;
                        HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_VANITY);
                        PlayerCharacter::bSnapChaseCamera = true;
                    }
                }
                this->HandlePhysicsGrab(afDelta);
                if (pPlayerSphere) {
                    uint32_t uiSphereCount = iPlayerSphereMoveCount++;
                    if (uiSphereCount < 20) {
                        NiPoint3 np;
                        np.z = currentPos.z + 128.0;
                        np.y = currentPos.y + 64.0;
                        np.x = currentPos.x + 64.0;
                        pPlayerSphere->SetPosition(np);
                        pPlayerSphere->SetLinearVelocity(NiPoint3::ZERO);
                    }
                }
                if (!this->IsDead(false) && !this->IsDying()) {
                    this->UpdateMagic(false, false);
                }
                panim1st->SetMoveSpeed(panim->GetMoveSpeed());
                panim1st->SetAttackSpeed(panim->m_fAttackSpeed);
                if (!Interface::InDialog()) {
                    this->DoIronSightsZoom(afDelta);
                }
                this->b3rdPerson = !this->b3rdPerson;
                this->Character::Update(afDelta);
                this->UpdateAnimationMovement(this->GetAnimation(!this->b3rdPerson), afDelta);
                this->b3rdPerson = !this->b3rdPerson;
                this->Character::Update(afDelta);
                this->UpdateAnimationMovement(this->GetAnimation(!this->b3rdPerson), afDelta);
                this->HandlePostAnimationActions();
                if (!bVATSForce1stPerson) {
                    this->UpdateCamera(false, false);
                }
                this->GetParentCell()->TestObjectAgainstMultibounds(this, true, false);
                NiNode* p3D = this->Get3D();
                BSShaderManager::GetShadowSceneNode(0)->UpdateObjectLighting(p3D, true);
                if (this->pCurrentProcess->GetCurrentMuzzleFlash()) {
                    this->pCurrentProcess->GetCurrentMuzzleFlash()->Update(afDelta, this);
                }
                if (this->IsDead(false) &&
                    (pVATS.eMode != VATS::VATS_PLAYBACK ||
                     !pVATS.GetCurrentAction() ||
                     pVATS.GetCurrentAction()->eAction != ActionPoints::ACTION_PLAYER_VATS_DEATH)) {
                    pVATS.InitPlayerDeath();
                }
                TESObjectCELL::UpdateSeenDataForPlayer();
                if (!PlayerCharacter::pLastCell ||
                    (pParentCell = this->GetParentCell(), PlayerCharacter::pLastCell != pParentCell)) {
                    PlayerCharacter::pLastCell = this->GetParentCell();
                    if (pModelLoader->IsFileQueuedForCell(PlayerCharacter::pLastCell)) {
                        TES::pInstance->ShowLoadingMessage("Loading Area (Player)...");
                        pModelLoader->UpdateQueuedReferencePriorities(PlayerCharacter::pLastCell);
                        pIOManager->LoadQueuedPriority(IO_TASK_PRIORITY_CRITICAL);
                    }
                }
                bsearch = false;
                CharacterProgression* pProgression = this->GetCharacterProgressionInfo();
                if (pProgression->GetReadyToLevelUp() &&
                    !this->IsPlayerCharacterInCombat(&bsearch) &&
                    !pVATS.IsInVats() &&
                    HUDMainMenu::IsXPMeterFinishedMoving() &&
                    Interface::IsPipboyClosed() &&
                    !InterfaceManager::GetContainerMenuQueued() &&
                    !this->GetInChargen()) {
                    this->GetCharacterProgressionInfo()->BeginLevelUp();
                }

                if (this->pReputationUpdate &&
                    !this->IsPlayerCharacterInCombat(&bsearch) &&
                    !pVATS.IsInVats() &&
                    !Interface::InDialog() &&
                    !StartMenu::PauseMenuIsActive()) {
                    this->pReputationUpdate->DisplayReputationTitleChange();
                    this->pReputationUpdate = nullptr;
                }

                if (this->pCurrentProcess) {
                    this->pCurrentProcess->CheckForExpiredDetectionEvent();
                }
                this->UpdatePlayerCombat(afDelta);
                if (pPlayer->IsAttacking()) {
                    if (fAngryFace < 1.0) {
                        fAngryFace = 1.0f;
                        BSFaceGenAnimationData* pfaceAnim = this->GetFaceAnimationData();
                        if (pfaceAnim) {
                            pfaceAnim->SetExpressionTarget(BS_FG_EXPRESSION_COMBAT_ANGRY, 1.0f);
                        }
                    }
                }
                else if (fAngryFace > 0.0) {
                    fAngryFace = 0.0f;
                    BSFaceGenAnimationData* pfaceAnim = this->GetFaceAnimationData();
                    if (pfaceAnim) {
                        pfaceAnim->ResetCurrentMorphs(0.5f, true, false, false, false, false);
                    }
                }
                if (bHolsteredWeapon) {
                    if (fCombatYieldTime.Float() > static_cast<double>(this->fCombatTimer) &&
                        this->fYieldTimer <= 0.0 &&
                        this->AttemptYield()) {
                        this->fYieldTimer = fCombatYieldRetryTime.Float();
                    }
                }
                if (this->fYieldTimer > 0.0) {
                    this->fYieldTimer = this->fYieldTimer - afDelta;
                    double dYieldTimer = BSMAX(this->fYieldTimer, 0.0f);
                    this->fYieldTimer = dYieldTimer;
                }
                if (this->pCurrentProcess->GetCurrentWeapon()) {
                    TESObjectWEAP* pWeap = static_cast<TESObjectWEAP*>(
                        this->pCurrentProcess->GetCurrentWeapon()->pContainerObj
                    );
                    bool bRegenMod = this->pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(6);
                    if (pWeap->GetAmmoRegenRate(bRegenMod) > 0.0) {
                        double dTimeSinceRegenTick = this->fTimeSinceLastAmmoRegenTick;
                        this->SetTimeSinceLastAmmoRegenTick(dTimeSinceRegenTick - appTimer.fDelta);
                        if (this->pCurrentProcess->GetCurrentAmmo() && this->fTimeSinceLastAmmoRegenTick <= 0.0) {
                            bRegenMod = this->pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(6);
                            this->SetTimeSinceLastAmmoRegenTick(pWeap->GetAmmoRegenRate(bRegenMod));
                            int32_t iCount = this->pCurrentProcess->GetCurrentAmmo()->iNumber;
                            bool bClipMod = this->pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(2);
                            if (iCount < pWeap->GetFormClipRounds(bClipMod)) {
                                this->pCurrentProcess->GetCurrentAmmo()->iNumber = ++iCount;
                            }
                        }
                        else if (this->fTimeSinceLastAmmoRegenTick <= 0.0) {
                            TESContainer tempcont;
                            tempcont.AddObject(pweapon->GetCurrentAmmo(pPlayer), 1, nullptr);
                            tempcont.AddContainerToReference(pPlayer, true);
                            pPlayer->EquipObject(pweapon->GetCurrentAmmo(pPlayer), 1, nullptr, true, false, false);
                        }
                    }
                }
                if (this->IsMoving()) {
                    TESObjectCELL* pCell = this->GetParentCell();
                    if (pCell) {
                        BSSimpleList<TESObjectREFR*>* pList = pCell->GetRefList();
                        if (iNextPassthroughSoundTime < BSGetTickCount()) {
                            while (pList && pList->m_item) {
                                if (pList->m_item->GetFormType() == REFR_ID) {
                                    TESObjectSTAT* pstat = dynamic_cast<TESObjectSTAT*>(
                                        pList->m_item->GetObjectReference()
                                    );
                                    if (pstat && pstat->cPassThroughSoundIndex != -1) {
                                        if (pList->m_item->Get3D()) {
                                            float fRadius = pList->m_item->Get3D()->GetWorldBoundRadius();
                                            fRadius = fRadius * fRadius;
                                            TESObjectREFR* pRef = pList->m_item;
                                            double dRadiusSquared = fRadius;
                                            const NiPoint3* pPlayerPos = &this->GetLocationOnReference();
                                            const NiPoint3* pRefPos = &pRef->GetLocationOnReference();
                                            double dDistanceSquared = (*pRefPos - *pPlayerPos).SqrLength();
                                            if (dDistanceSquared < dRadiusSquared) {
                                                TESSound* pSound = TESSound::GetPassThroughSound(
                                                    pstat->cPassThroughSoundIndex
                                                );
                                                if (pSound) {
                                                    uint32_t uiSoundID = pSound->iFormID;
                                                    BSAudio::QInstance()
                                                        .GetSoundHandleByNumericID(uiSoundID, 257u)
                                                        .Play(false);
                                                    uint32_t uiTickCount = BSGetTickCount();
                                                    iNextPassthroughSoundTime = RandomInt(1500, 2000) + uiTickCount;
                                                }
                                            }
                                        }
                                    }
                                }
                                pList = pList->m_pkNext;
                            }
                        }
                    }
                }
                this->UpdateAutoAimActor();
                if (this->pContinuousBeamPersistant) {
                    bool bhasAmmo;
                    ItemChange* pammo = this->pCurrentProcess->GetCurrentAmmo();
                    if (pammo) {
                        int32_t iAmmoCount = pammo->iNumber;
                        bhasAmmo = iAmmoCount >= pweapon->GetAmmoPerShot();
                    }
                    else {
                        bhasAmmo = false;
                    }
                    if (!bhasAmmo ||
                        pcontrols->GetUserAction(Controls::UACTN_USE, 2) ||
                        !pcontrols->GetUserAction(Controls::UACTN_USE, 0)) {
                        this->pContinuousBeamPersistant->SetKill(this);
                    }
                    this->pContinuousBeamPersistant->UpdateProjectile(0.0f);
                }
                if (this->fLastHelloTime > 0.0) {
                    double dElapsed = static_cast<double>(BSGetTickCount()) - this->fLastHelloTime;
                    if (fHelloCooldownTime.Float() * 1000.0 < dElapsed) {
                        this->fLastHelloTime = -1.0f;
                    }
                }
            }
        }
        else {
            if (this->spGrabSpring.m_pObject) {
                this->ForcePhysicsGrabDrop();
            }
            this->ForceGrenadeHold();
            this->b3rdPerson = !this->b3rdPerson;
            this->Character::Update(afDelta);
            this->UpdateAnimationMovement(this->GetAnimation(!this->b3rdPerson), afDelta);
            this->b3rdPerson = !this->b3rdPerson;
            this->Character::Update(afDelta);
            this->UpdateAnimationMovement(this->GetAnimation(!this->b3rdPerson), afDelta);
            this->UpdateCamera(false, false);
        }
    }
}

void PlayerCharacter::Resurrect(bool abResetInventory, bool abAttach3D, bool abGetupResurect) {
    fHealthModifier = 0.0f;
    Interface::UpdateActorValue(ActorValue::HEALTH);
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    BaseProcess* pCurrentProcess = this->pCurrentProcess;
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    LowProcess* pLowProcess = new LowProcess;
    this->pCurrentProcess = pLowProcess;
    HighProcess* pHighProcess = new HighProcess;
    pHighProcess->Copy(this->pCurrentProcess);
    ReferenceProcessLists.RemoveReference(this, GetCurrentProcessType());
    BaseProcess* pOldProcess = this->pCurrentProcess;
    if (pOldProcess) {
        delete pOldProcess;
    }
    this->pCurrentProcess = pHighProcess;
    ReferenceProcessLists.RemoveReference(this, GetCurrentProcessType());
    ReferenceProcessLists.kUpdateLock.Unlock();
    InitHavok();
    bhkCharacterController* pCharController = GetCharController();
    if (pCharController) {
        CFilter kFilter;
        pCharController->GetCollisionFilter(&kFilter);
        uint32_t aiGroup = kFilter.GetGroup();
        CameraCaster* pCaster = new CameraCaster(fCameraCasterSize.Float(), aiGroup);
        pCameraCaster = pCaster;
    }
    else {
        pCameraCaster = nullptr;
    }
    Actor::Resurrect(false, false, false);
    InitAnimation();
}

void PlayerCharacter::SetAdvanceNumber(int32_t inumber) {
    iNumAdvance = inumber;
}

int32_t PlayerCharacter::GetAdvanceNumber() {
    return iNumAdvance;
}

void PlayerCharacter::SetSkillAdvanceDelayed(ActorValue::Index aeskill) {
    eskilladvance = aeskill;
}

ActorValue::Index PlayerCharacter::GetSkillAdvanceDelayed() {
    return eskilladvance;
}

void PlayerCharacter::UseActionPoints(ActionPoints::Action aeAction) {
    float fRequiredAP = ActionPoints::GetRequiredAPForAction(aeAction, pVATS.IsInVats());
    UseActionPoints(fRequiredAP);
}

void PlayerCharacter::UseActionPoints(float afAmount) {
    if (!PlayerCharacter::IsGodMode() && !pSaveLoadGame->GetSaveGameLoading() && afAmount > 0.0) {
        DamageModActorValue(ActorValue::ACTION_POINTS, -afAmount, nullptr);
    }
}

bool PlayerCharacter::IsOverEncumbered() {
    if (PlayerCharacter::IsGodMode()) {
        return false;
    }
    double dInventoryWeight = static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::INVENTORY_WEIGHT);
    return Actor::GetMaxCarryWeight() < dInventoryWeight;
}

// True while the player, under AI control, runs a cannibal package.
bool PlayerCharacter::GetCannibal() {
    if (!GetUnderAIControl()) {
        return false;
    }
    BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
    TESPackage* pPackage = pProcess->GetCurrentPackage();
    if (!pPackage) {
        return false;
    }
    if (pPackage->GetPackType() == PACKAGE_TYPE_CANNIBAL) {
        return true;
    }
    return false;
}

bool PlayerCharacter::GetSandman() {
    if (!GetUnderAIControl()) {
        return false;
    }
    BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
    TESPackage* pPackage = pProcess->GetCurrentPackage();
    if (!pPackage) {
        return false;
    }
    if (pPackage->GetPackType() == PACKAGE_TYPE_SANDMAN) {
        return true;
    }
    return false;
}

void PlayerCharacter::InitiateSandmanPackage(
    Actor* apTarget,
    TESObjectREFR* apfurniture,
    FurnitureMark* aMarker,
    uint8_t auiIndex
) {
    iSandmanDetection = ReferenceProcessLists.GetHighestDetectionLevelForActor(pPlayer, false);
    SetAIControlledPackage(true);
    ForceTemp3rdPerson(true);
    pCurrentProcess->SetRunOncePackage(nullptr, this);
    pCurrentProcess->ClearActionHeadTrackTarget(false);
    TESPackage* ppack = TESPackage::CreatePackage(PACKAGE_TYPE_SANDMAN);
    ppack->SetMustReachLocation(true);
    ppack->SetMustComplete(true);
    PackageLocation* ploc = new PackageLocation;
    ploc->SetLocType(PLT_REFERENCE);
    ploc->SetLocReference(this);
    ppack->SetPackageLocation(ploc);
    if (ploc) {
        delete ploc;
    }
    PackageTarget* pnew = new PackageTarget;
    ppack->SetPackageTarget(pnew);
    if (pnew) {
        delete pnew;
    }
    ppack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_SANDMAN;
    ppack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
    ppack->GetPackageTarget()->SetTargReference(apTarget);
    ppack->GetPackageTarget()->iValue = 0;
    pCurrentProcess->ComputeLastTimeProcessed();
    SetSandman(true);
    pCurrentProcess->SetFurniture(apfurniture, auiIndex, aMarker);
    PutCreatedPackage(ppack, false, true);
    apTarget->InitiateDoNothingPackage();
}

void PlayerCharacter::InitiateCannibalPackage(Actor* apTarget) {
    if (!apTarget->GetHasBeenEaten()) {
        SetAIControlledPackage(true);
        ForceTemp3rdPerson(true);
        pCurrentProcess->SetRunOncePackage(nullptr, this);
        pCurrentProcess->ClearActionHeadTrackTarget(false);
        TESPackage* ppack = TESPackage::CreatePackage(PACKAGE_TYPE_CANNIBAL);
        ppack->SetMustReachLocation(true);
        ppack->SetMustComplete(true);
        PackageLocation* ploc = new PackageLocation;
        ploc->SetLocType(PLT_REFERENCE);
        ploc->SetLocReference(this);
        ppack->SetPackageLocation(ploc);
        if (ploc) {
            delete ploc;
        }
        PackageTarget* pnew = new PackageTarget;
        ppack->SetPackageTarget(pnew);
        if (pnew) {
            delete pnew;
        }
        ppack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_CANNIBAL;
        ppack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
        ppack->GetPackageTarget()->SetTargReference(apTarget);
        ppack->GetPackageTarget()->iValue = 0;
        pCurrentProcess->ComputeLastTimeProcessed();
        PutCreatedPackage(ppack, false, true);
    }
}

// Applies refraction to both 3D models, the extra data and the pip-boy. With the flag set and a power of 0 or less, the
// script's refraction power is used, and a power still 0 or less clears the flag. A NaN power counts as positive in
// both compares.
void PlayerCharacter::SetRefraction(bool abEnable, float afRefractionPower) {
    float fPower = afRefractionPower;
    if (abEnable && fPower <= 0.0 && this->GetCurrentAIProcess()) {
        BaseProcess* pProcess = this->GetCurrentAIProcess();
        fPower = pProcess->GetScriptRefractionPower();
    }

    bool bRefractive = abEnable;
    if (abEnable && fPower <= 0.0) {
        bRefractive = false;
    }
    BSShaderPPLightingProperty::SetRefractionRecurse(this->Get3D(true), bRefractive, fPower, false, 0.0f, true);
    BSShaderPPLightingProperty::SetRefractionRecurse(this->Get3D(false), bRefractive, fPower, false, 0.0f, true);
    ExtraDataList* pExtra = this->GetExtra();
    if (pExtra) {
        pExtra->SetRefractionPropertyExtra(bRefractive, afRefractionPower);
    }
    FOPipboyManager* pPipboy = Interface::GetPipboy();
    if (pPipboy) {
        pPipboy->SetRefractive(abEnable);
    }
}

void PlayerCharacter::SetActorValue(ActorValue::Index aiWhich, float afValue) {
    Actor::SetActorValue(aiWhich, afValue);
    Interface::UpdateActorValue(aiWhich);
    Actor::CheckSkillLevelChange(aiWhich, true);
}

void PlayerCharacter::TempModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
    if (!ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        int32_t iAdjustedValue = DifficultyLevelCheckModHealthI(aiWhich, aiValue, apAttacker);
        float foldValue = ActorValue::HasModifiedCallback(aiWhich) ? GetActorFloatValue(aiWhich) : 0.0f;
        AlterModifier(PlayerCharacter::TEMPORARY, aiWhich, static_cast<float>(iAdjustedValue), ModifierList::NO_CLAMP);
        Interface::UpdateActorValue(aiWhich);
        if (aiWhich == ActorValue::HEALTH && iAdjustedValue < 0) {
            HandleHealthDamage(apAttacker, static_cast<float>(iAdjustedValue));
        }
        CheckSkillLevelChange(aiWhich, false);
        ActorValueOwner* pAttackerOwner = static_cast<ActorValueOwner*>(apAttacker);
        ActorValueOwner* pOwner = nullptr;
        if (this) {
            pOwner = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
        }
        ActorValue::CheckCallModifiedCallback(
            pOwner,
            aiWhich,
            foldValue,
            static_cast<float>(iAdjustedValue),
            pAttackerOwner
        );
    }
}

void PlayerCharacter::TempModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
    if (!ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        float fAdjustedValue = DifficultyLevelCheckModHealth(aiWhich, afValue, apAttacker);
        float foldValue = ActorValue::HasModifiedCallback(aiWhich) ? GetActorFloatValue(aiWhich) : 0.0f;
        AlterModifier(PlayerCharacter::TEMPORARY, aiWhich, fAdjustedValue, ModifierList::NO_CLAMP);
        Interface::UpdateActorValue(aiWhich);
        if (aiWhich == ActorValue::HEALTH && fAdjustedValue < 0.0) {
            HandleHealthDamage(apAttacker, fAdjustedValue);
        }
        CheckSkillLevelChange(aiWhich, false);
        ActorValueOwner* pAttackerOwner = static_cast<ActorValueOwner*>(apAttacker);
        ActorValueOwner* pOwner = nullptr;
        if (this) {
            pOwner = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
        }
        ActorValue::CheckCallModifiedCallback(pOwner, aiWhich, foldValue, fAdjustedValue, pAttackerOwner);
    }
}

void PlayerCharacter::PermanentModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
    float afValue = static_cast<float>(aiValue);
    if (CheckInvulnerable(aiWhich, afValue) && !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        int32_t iAdjustedValue = DifficultyLevelCheckModHealthI(aiWhich, aiValue, apAttacker);
        float foldValue = ActorValue::HasModifiedCallback(aiWhich) ? GetActorFloatValue(aiWhich) : 0.0f;
        AlterModifier(PlayerCharacter::SCRIPTS, aiWhich, static_cast<float>(iAdjustedValue), ModifierList::NO_CLAMP);
        Interface::UpdateActorValue(aiWhich);
        if (aiWhich == ActorValue::HEALTH && iAdjustedValue < 0) {
            HandleHealthDamage(apAttacker, static_cast<float>(iAdjustedValue));
        }
        CheckSkillLevelChange(aiWhich, false);
        ActorValueOwner* pAttackerOwner = static_cast<ActorValueOwner*>(apAttacker);
        ActorValueOwner* pOwner = nullptr;
        if (this) {
            pOwner = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
        }
        ActorValue::CheckCallModifiedCallback(
            pOwner,
            aiWhich,
            foldValue,
            static_cast<float>(iAdjustedValue),
            pAttackerOwner
        );
    }
}

void PlayerCharacter::PermanentModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
    if (CheckInvulnerable(aiWhich, afValue) && !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        float fAdjustedValue = DifficultyLevelCheckModHealth(aiWhich, afValue, apAttacker);
        float foldValue = ActorValue::HasModifiedCallback(aiWhich) ? GetActorFloatValue(aiWhich) : 0.0f;
        AlterModifier(PlayerCharacter::SCRIPTS, aiWhich, fAdjustedValue, ModifierList::NO_CLAMP);
        Interface::UpdateActorValue(aiWhich);
        if (aiWhich == ActorValue::HEALTH && fAdjustedValue < 0.0) {
            HandleHealthDamage(apAttacker, fAdjustedValue);
        }
        CheckSkillLevelChange(aiWhich, false);
        ActorValueOwner* pAttackerOwner = static_cast<ActorValueOwner*>(apAttacker);
        ActorValueOwner* pOwner = nullptr;
        if (this) {
            pOwner = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
        }
        ActorValue::CheckCallModifiedCallback(pOwner, aiWhich, foldValue, fAdjustedValue, pAttackerOwner);
    }
}

void PlayerCharacter::DamageModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
    float fValue = static_cast<float>(aiValue);
    if (CheckInvulnerable(aiWhich, fValue) && !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        double dOldRatio;
        if (aiWhich == ActorValue::HEALTH) {
            double dHealth = GetActorFloatValue(ActorValue::HEALTH);
            dOldRatio = dHealth / GetPermanentActorFloatValue(ActorValue::HEALTH);
        }
        else {
            dOldRatio = 1.0;
        }
        float foldHealthRatio = dOldRatio;
        int32_t iAdjustedValue = DifficultyLevelCheckModHealthI(aiWhich, aiValue, apAttacker);
        float foldValue = ActorValue::HasModifiedCallback(aiWhich) ? GetActorFloatValue(aiWhich) : 0.0f;
        ModifierList::ClampStyle eclampStyle = static_cast<ModifierList::ClampStyle>(
            ActorValue::IsFlagSet(aiWhich, ActorValue::DAMAGE_IS_POSITIVE)
        );
        AlterModifier(PlayerCharacter::DAMAGE, aiWhich, static_cast<float>(iAdjustedValue), eclampStyle);
        Interface::UpdateActorValue(aiWhich);
        if (aiWhich == ActorValue::HEALTH) {
            double dHealth = GetActorFloatValue(ActorValue::HEALTH);
            float fhealthRatio = dHealth / GetPermanentActorFloatValue(ActorValue::HEALTH);
            if (static_cast<double>(iAdjustedValue) < 0.0) {
                HandleHealthDamage(apAttacker, static_cast<float>(iAdjustedValue));
            }
            if (!(fhealthRatio <= 0.0) && !(fPlayerHealthHeartbeatSlow.Float() < static_cast<double>(fhealthRatio))) {
                if (!(fPlayerHealthHeartbeatSlow.Float() >= static_cast<double>(fhealthRatio)) ||
                    !(fPlayerHealthHeartbeatFast.Float() < static_cast<double>(fhealthRatio)) ||
                    (!(fPlayerHealthHeartbeatFast.Float() >= static_cast<double>(foldHealthRatio)) &&
                     !(fPlayerHealthHeartbeatSlow.Float() < static_cast<double>(foldHealthRatio)))) {
                    if (fPlayerHealthHeartbeatFast.Float() < static_cast<double>(foldHealthRatio) &&
                        fPlayerHealthHeartbeatFast.Float() >= static_cast<double>(fhealthRatio)) {
                        StatusSoundHandle.Stop();
                        StatusSoundHandle.Release();
                        StatusSoundHandle = BSAudio::QInstance().GetSoundHandleByName("UIHealthHeartbeatBLP", 49u);
                        StatusSoundHandle.Play(true);
                    }
                }
                else {
                    StatusSoundHandle.Stop();
                    StatusSoundHandle.Release();
                    StatusSoundHandle = BSAudio::QInstance().GetSoundHandleByName("UIHealthHeartbeatALP", 49u);
                    StatusSoundHandle.Play(true);
                }
            }
            else {
                StatusSoundHandle.FadeOutAndRelease(1000u);
            }
        }
        CheckSkillLevelChange(aiWhich, false);
        ActorValueOwner* pAttackerOwner = static_cast<ActorValueOwner*>(apAttacker);
        ActorValueOwner* pOwner = nullptr;
        if (this) {
            pOwner = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
        }
        ActorValue::CheckCallModifiedCallback(
            pOwner,
            aiWhich,
            foldValue,
            static_cast<float>(iAdjustedValue),
            pAttackerOwner
        );
    }
}

void PlayerCharacter::DamageModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
    if (CheckInvulnerable(aiWhich, afValue) && !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        double dOldRatio;
        if (aiWhich == ActorValue::HEALTH) {
            double dHealth = GetActorFloatValue(ActorValue::HEALTH);
            dOldRatio = dHealth / GetPermanentActorFloatValue(ActorValue::HEALTH);
        }
        else {
            dOldRatio = 1.0;
        }
        float foldHealthRatio = dOldRatio;
        float fAdjustedValue = DifficultyLevelCheckModHealth(aiWhich, afValue, apAttacker);
        float foldValue = ActorValue::HasModifiedCallback(aiWhich) ? GetActorFloatValue(aiWhich) : 0.0f;
        ModifierList::ClampStyle eclampStyle = static_cast<ModifierList::ClampStyle>(
            ActorValue::IsFlagSet(aiWhich, ActorValue::DAMAGE_IS_POSITIVE)
        );
        AlterModifier(PlayerCharacter::DAMAGE, aiWhich, fAdjustedValue, eclampStyle);
        Interface::UpdateActorValue(aiWhich);
        if (aiWhich == ActorValue::HEALTH) {
            double dHealth = GetActorFloatValue(ActorValue::HEALTH);
            float fhealthRatio = dHealth / GetPermanentActorFloatValue(ActorValue::HEALTH);
            if (fAdjustedValue < 0.0) {
                HandleHealthDamage(apAttacker, fAdjustedValue);
            }
            if (!(fhealthRatio <= 0.0) && !(fPlayerHealthHeartbeatSlow.Float() < static_cast<double>(fhealthRatio))) {
                if (!(fPlayerHealthHeartbeatSlow.Float() >= static_cast<double>(fhealthRatio)) ||
                    !(fPlayerHealthHeartbeatFast.Float() < static_cast<double>(fhealthRatio)) ||
                    (!(fPlayerHealthHeartbeatFast.Float() >= static_cast<double>(foldHealthRatio)) &&
                     !(fPlayerHealthHeartbeatSlow.Float() < static_cast<double>(foldHealthRatio)))) {
                    if (fPlayerHealthHeartbeatFast.Float() < static_cast<double>(foldHealthRatio) &&
                        fPlayerHealthHeartbeatFast.Float() >= static_cast<double>(fhealthRatio)) {
                        StatusSoundHandle.Stop();
                        StatusSoundHandle.Release();
                        StatusSoundHandle = BSAudio::QInstance().GetSoundHandleByName("UIHealthHeartbeatBLP", 49u);
                        StatusSoundHandle.Play(true);
                    }
                }
                else {
                    StatusSoundHandle.Stop();
                    StatusSoundHandle.Release();
                    StatusSoundHandle = BSAudio::QInstance().GetSoundHandleByName("UIHealthHeartbeatALP", 49u);
                    StatusSoundHandle.Play(true);
                }
            }
            else {
                StatusSoundHandle.FadeOutAndRelease(1000u);
            }
        }
        CheckSkillLevelChange(aiWhich, false);
        ActorValueOwner* pAttackerOwner = static_cast<ActorValueOwner*>(apAttacker);
        ActorValueOwner* pOwner = nullptr;
        if (this) {
            pOwner = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
        }
        ActorValue::CheckCallModifiedCallback(pOwner, aiWhich, foldValue, fAdjustedValue, pAttackerOwner);
    }
}

void PlayerCharacter::ModActorBaseValue(ActorValue::Index aiWhich, int32_t aiValue) {
    float foldValue = ActorValue::HasModifiedCallback(aiWhich) ? GetActorFloatValue(aiWhich) : 0.0f;
    bool bHasOverride = false;
    GetBaseValueOverride(aiWhich, bHasOverride);
    if (bHasOverride) {
        float fNewValue = static_cast<double>(aiValue) + foldValue;
        SetBaseValueOverride(aiWhich, fNewValue);
    }
    else {
        GetActorBase()->ModActorValue(aiWhich, aiValue);
    }
    Interface::UpdateActorValue(aiWhich);
    CheckSkillLevelChange(aiWhich, true);
    ActorValueOwner* pOwner = nullptr;
    if (this) {
        pOwner = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
    }
    ActorValue::CheckCallModifiedCallback(pOwner, aiWhich, foldValue, static_cast<float>(aiValue), nullptr);
}

void PlayerCharacter::ModActorBaseValue(ActorValue::Index aiWhich, float afValue) {
    float foldValue = ActorValue::HasModifiedCallback(aiWhich) ? GetActorFloatValue(aiWhich) : 0.0f;
    bool bHasOverride = false;
    GetBaseValueOverride(aiWhich, bHasOverride);
    if (bHasOverride) {
        float fNewValue = foldValue + afValue;
        SetBaseValueOverride(aiWhich, fNewValue);
    }
    else {
        GetActorBase()->ModActorValue(aiWhich, afValue);
    }
    Interface::UpdateActorValue(aiWhich);
    CheckSkillLevelChange(aiWhich, true);
    ActorValueOwner* pOwner = nullptr;
    if (this) {
        pOwner = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
    }
    ActorValue::CheckCallModifiedCallback(pOwner, aiWhich, foldValue, afValue, nullptr);
}

bool PlayerCharacter::DamageEquipment(ItemChange* apEquipment, float afDamage, bool abIgnoreArmorSkill) {
    return !PlayerCharacter::IsGodMode() && Actor::DamageEquipment(apEquipment, afDamage, abIgnoreArmorSkill);
}

TESObjectREFR* PlayerCharacter::DropObject(
    TESObject* apObject,
    ExtraDataList* apExtra,
    int32_t ainumber,
    const NiPoint3* apPoint,
    const NiPoint3* apRotate
) {
    pVATS.QuitVATSPlayback(false, false);
    TESObjectCELL* pParentCell = GetParentCell();
    if (Interface::PreloadWorldMap() && pParentCell) {
        TESObjectREFR* pLootBag = pParentCell->GetLootBag();
        if (!pLootBag) {
            pLootBag = pParentCell->CreateLootBagAtRef(this);
        }
        if (pLootBag) {
            RemoveItem(
                static_cast<TESBoundObject*>(apObject),
                apExtra,
                ainumber,
                false,
                false,
                pLootBag,
                nullptr,
                nullptr,
                true,
                false
            );
            return nullptr;
        }
    }
    else if (apObject->GetFormType() == AMMO_ID && GetCurrentWeapon()) {
        PlayerCharacter* pPlayerCharacter = pPlayer;
        TESObjectWEAP* pWeapon = GetCurrentWeapon();
        if (pWeapon->GetCurrentAmmo(pPlayerCharacter) == apObject) {
            pCurrentProcess->GetCurrentAmmo()->iNumber = 0;
            NiAVObject* pRound = BSUtilities::GetObjectByName(Get3D(false), "##NifRound");
            if (pRound) {
                pRound->SetAppCulled(true);
            }
            pRound = BSUtilities::GetObjectByName(Get3D(false), "##NifRound");
            if (pRound) {
                pRound->SetAppCulled(true);
            }
        }
    }
    if (pQueuedWeaponAttach && apObject == pQueuedWeaponAttach && apExtra && apExtra->GetWorn(false)) {
        pQueuedWeaponAttach = nullptr;
    }

    TESBoundObject* pBoundObj = apObject->IsBoundObject() ? static_cast<TESBoundObject*>(apObject) : nullptr;
    TESObjectREFR*
        pObjREFR = RemoveItem(pBoundObj, apExtra, ainumber, false, true, nullptr, apPoint, apRotate, true, false);
    if (pObjREFR) {
        NiAVObject* pAV = pObjREFR->Get3D();
        if (pAV) {
            bhkWorld::Enable(pAV, false, true, true);
            NiUpdateData kUpdateData(0.0f, false, false);
            pAV->Update(kUpdateData);
            bhkWorld::Enable(pAV, true, true, true);
            NiPoint3 kLocation = pObjREFR->GetLocationOnReference();
            NiPoint3 kShift = kLocation - pAV->GetWorldBound().m_kCenter;
            kLocation += kShift;
            pAV->SetTranslate(kLocation);
            pObjREFR->SetLocationOnReference(kLocation);
            bhkNiCollisionObject::ResetSim(pAV, true);
            bhkWorld::Activate(pAV, true, true, false);
        }
        else {
            DroppedRefList.AddHead(pObjREFR);
            pObjREFR->SetRecentPCDrop(true);
            pObjREFR->SetCenterOnCreation(true);
        }
        PlayPickUpSound(pBoundObj, false, false);
    }
    return pObjREFR;
}

// The weapon and ammo getters (IsPlayable, IsThrownWeapon) are called on the base object held as a TESBoundObject; the
// casts only retype it. The 2 passed to GetAction is the action the picked-up reference carries.
void PlayerCharacter::PickUpObject(TESObjectREFR* apobjREFR, int32_t aicount, bool abflag) {
    TESBoundObject* pWeapon = nullptr;
    TESBoundObject* pAmmo = nullptr;
    bool bAmmoAdded = false;
    uint8_t eFormType = apobjREFR->GetObjectReference()->GetFormType();
    if (eFormType == WEAP_ID) {
        pWeapon = apobjREFR->GetObjectReference();
        if (!static_cast<TESObjectWEAP*>(pWeapon)->GetIsPlayable()) {
            return;
        }
    }
    else if (eFormType == AMMO_ID) {
        pAmmo = apobjREFR->GetObjectReference();
        if (!static_cast<TESAmmo*>(pAmmo)->GetIsPlayable()) {
            return;
        }
    }
    PlayPickUpSound(apobjREFR->GetObjectReference(), true, false);
    bhkWorld::Activate(apobjREFR->Get3D(), true, true, false);
    BSSimpleList<TESObjectREFR*>* pIter = ReferenceProcessLists.GetActorTargetinganObject(apobjREFR->GetFormID(), this);
    BSSimpleList<TESObjectREFR*>* pTargeters = pIter;
    while (pIter && pIter->GetItem()) {
        Actor* pActor = static_cast<Actor*>(pIter->GetItem());
        if (pActor && pActor->GetCurrentPackageTarget() == apobjREFR) {
            pActor->SetCurrentTarget(this);
        }
        else {
            pActor->SetCurrentTarget(nullptr);
        }
        pIter = pIter->GetNext();
    }
    if (pTargeters) {
        pTargeters->RemoveAll();
        delete pTargeters;
    }
    TESFile* pFile = apobjREFR->GetFile(-1);
    if (!Interface::IsMenuIDVisible(Interface::eDialogMenu, 0) &&
        !Interface::ContainerIsBarter() &&
        !Interface::ContainerIsRecipe()) {
        TESForm* pOwner = apobjREFR->GetOwner();
        if (!pOwner || apobjREFR->IsAnOwner(this, true) || apobjREFR->GetObjectReference()->GetFormType() == PROJ_ID) {
            apobjREFR->GetExtra()->RemoveOwnership();
        }
        else {
            pContainerOwner = ReferenceProcessLists.FindOwnerSellsObject(apobjREFR);
            if (apobjREFR->GetMaster() || pFile || apobjREFR->GetAction(OBJECT_ACTION_ACTIVATE)) {
                TESBoundObject* pObject = apobjREFR->GetObjectReference();
                StealAlarm(apobjREFR, pObject, aicount, 0, pOwner);
            }
            else if (pContainerOwner) {
                TESBoundObject* pObject = apobjREFR->GetObjectReference();
                StealAlarm(pContainerOwner, pObject, aicount, 0, pOwner);
            }
            if (apobjREFR->IsOwnerEvil(nullptr)) {
                apobjREFR->GetExtra()->RemoveOwnership();
            }
            else {
                apobjREFR->GetExtra()->SetOwnership(pOwner);
            }
        }
    }
    uint8_t eObjectType = apobjREFR->GetObjectReference()->GetFormType();
    if (eObjectType == WEAP_ID) {
        pWeapon = apobjREFR->GetObjectReference();
        // A thrown weapon is its own ammo: picking up the equipped one adds to both stacks.
        if (static_cast<TESObjectWEAP*>(pWeapon)->IsThrownWeapon() &&
            pCurrentProcess->GetCurrentAmmo() &&
            pCurrentProcess->GetCurrentAmmo()->GetItemChangeObject() == pWeapon) {
            if (pCurrentProcess->GetCurrentWeapon()) {
                int32_t iCount = pCurrentProcess->GetCurrentWeapon()->GetCount() + aicount;
                pCurrentProcess->GetCurrentWeapon()->SetCount(iCount);
            }
            if (pCurrentProcess->GetCurrentAmmo()) {
                int32_t iCount = pCurrentProcess->GetCurrentAmmo()->GetCount() + aicount;
                pCurrentProcess->GetCurrentAmmo()->SetCount(iCount);
            }
            bAmmoAdded = true;
        }
    }
    else if (eObjectType == AMMO_ID) {
        abflag = false;
    }
    bool bDeleteRef = false;
    if (apobjREFR->GetMaster() || pFile) {
        apobjREFR->MarkAsPickedUp();
        AddReferenceItem(apobjREFR, aicount, abflag, bAmmoAdded);
    }
    else if (apobjREFR->GetAction(OBJECT_ACTION_ACTIVATE)) {
        apobjREFR->MarkAsPickedUp();
        AddReferenceItem(apobjREFR, aicount, false, bAmmoAdded);
    }
    else {
        AddReferenceItem(apobjREFR, aicount, false, bAmmoAdded);
        bDeleteRef = true;
    }
    if ((!pCurrentProcess->GetCurrentAmmo() || !pCurrentProcess->GetCurrentAmmo()->GetCount()) &&
        (pCurrentProcess->GetCurrentWeapon() && apobjREFR)) {
        TESBoundObject* pObject = apobjREFR->GetObjectReference();
        PlayerCharacter* pHolder = pPlayer;
        TESObjectWEAP* pCurrentWeapon = static_cast<TESObjectWEAP*>(
            pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject()
        );
        if (pObject == pCurrentWeapon->GetCurrentAmmo(pHolder)) {
            // WEAPON_MOD_AMMO_REGEN_SECONDS
            bool bRegenMod = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(6);
            TESObjectWEAP* pRegenWeapon = static_cast<TESObjectWEAP*>(
                pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject()
            );
            if (pRegenWeapon->GetAmmoRegenRate(bRegenMod) <= 0.0) {
                // WEAPON_MOD_INCREASE_CLIP_SIZE
                bool bUseModValue = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(2);
                int32_t iReloadAnim = IsWeaponDrawn() ? 2 : 0;
                TESObjectWEAP* pReloadWeapon = static_cast<TESObjectWEAP*>(
                    pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject()
                );
                ReloadWeaponNV(pReloadWeapon, iReloadAnim, bUseModValue, false);
            }
        }
    }

    if (bDeleteRef) {
        delete apobjREFR;
    }
}

void PlayerCharacter::CastScroll(TESObjectBOOK* apScroll, MagicTarget* apTarget) {
    Actor::CastScroll(apScroll, apTarget);
    ItemChange* pItem = GetInventoryItem(apScroll, apScroll->GetFormID());
    if (pItem) {
        delete pItem;
        SetSelectedScroll(apScroll);
    }
    else {
        SetSelectedScroll(nullptr);
    }
}

bool PlayerCharacter::AddSpell(SpellItem* apSpellIn) {
    int8_t cAdded = Actor::AddSpell(apSpellIn);
    if (cAdded) {
        SpellLearned(apSpellIn);
    }
    return cAdded;
}

bool PlayerCharacter::RemoveSpell(SpellItem* apSpellIn) {
    int8_t cRemoved = Actor::RemoveSpell(apSpellIn);
    if (!cRemoved) {
        return cRemoved;
    }
    MagicItem* pMagicItem = static_cast<MagicItem*>(apSpellIn);
    if (pMagicItem == GetSelectedSpell()) {
        SetSelectedSpell(nullptr);
    }
    return cRemoved;
}

// Reloads apWeapon: picks its ammo (the weapon itself for grenades, mines and thrown weapons; a hotkeyed ammo
// override), makes it the current ammo, plays the reload animation for aiReloadAnim 2 (only when the clip is neither
// full nor holding all the ammo) or 1, equips the ammo and charges the reload's action points outside VATS.
bool PlayerCharacter::ReloadWeapon(TESObjectWEAP* apWeapon, int32_t aiReloadAnim, bool bUseModValue) {
    TESForm* pAmmo = apWeapon->GetCurrentAmmo(pPlayer);
    ItemChange* pWeaponItem = pCurrentProcess->GetCurrentWeapon();
    // 10-13: the one-handed grenade, mine, lunchbox mine and thrown weapon types are their own ammo.
    if (!pAmmo &&
        (apWeapon->GetWeaponType() == 10 ||
         apWeapon->GetWeaponType() == 11 ||
         apWeapon->GetWeaponType() == 12 ||
         apWeapon->GetWeaponType() == 13)) {
        pAmmo = apWeapon;
    }
    if (!pAmmo) {
        return false;
    }
    InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(this);
    if (apWeapon && pAmmo && pAmmo != apWeapon) {
        ItemChange* pItem = pChanges->GetInventoryItem(apWeapon, 0);
        if (pItem->GetHotKey() >= 0) {
            TESForm* pHotkeyAmmo = pPlayer->GetAmmoForHotKey(pItem->GetHotKey());
            if (pHotkeyAmmo) {
                pAmmo = pHotkeyAmmo;
            }
        }
        if (pItem) {
            delete pItem;
        }
    }
    int32_t iCount = pChanges->GetObjectCount(static_cast<TESBoundObject*>(pAmmo));
    ItemChange* pCurrentAmmo = pCurrentProcess->GetCurrentAmmo();
    if (!pCurrentAmmo || pCurrentAmmo->GetItemChangeObject() != pAmmo) {
        ItemChange* pNewAmmo = new ItemChange(static_cast<TESBoundObject*>(pAmmo), iCount);
        pCurrentProcess->SetCurrentAmmo(pNewAmmo);
        pCurrentProcess->GetCurrentAmmo()->SetCount(0);
    }
    if (apWeapon && pAmmo != apWeapon) {
        iCount = BSMIN(iCount, apWeapon->GetFormClipRounds(bUseModValue));
    }
    if (aiReloadAnim == 2 && pCurrentProcess->GetCurrentAmmo()) {
        // 2: WEAPON_MOD_INCREASE_CLIP_SIZE.
        int32_t iClipRounds = apWeapon->GetFormClipRounds(pWeaponItem->HasModEffectActive(2));
        if (iClipRounds != pCurrentProcess->GetCurrentAmmo()->GetCount() &&
            iCount != pCurrentProcess->GetCurrentAmmo()->GetCount()) {
            int32_t eReloadGroup = apWeapon->GetReloadAnim(bUseModValue);
            uint16_t sGroup = GetAnimGroup(static_cast<ANIM_GROUP_ENUM>(eReloadGroup), nullptr, false, nullptr);
            if (TESAnimGroup::GetType(sGroup) == eReloadGroup) {
                Animation* pAnimation = GetAnimation(false);
                // 1: start immediately; -1: loop forever, no section.
                pAnimation->PlayGroup(sGroup, ACTION_START, -1, AGS_NONE);
                StartAnimOn1stPerson(sGroup, ACTION_START);
                // 4: AGS_WEAPON.
                SetAnimAction(ANIM_ACTION_RELOAD, pAnimation->GetCurrentSequence(AGS_WEAPON));
            }
        }
    }
    if (!iCount) {
        return false;
    }
    if (pCurrentProcess->GetCurrentAmmo()->GetCount()) {
        if (!pWeaponItem) {
            return false;
        }
        int32_t iClipRounds = apWeapon->GetFormClipRounds(pWeaponItem->HasModEffectActive(2));
        if (iClipRounds == pCurrentProcess->GetCurrentAmmo()->GetCount()) {
            return false;
        }
        if (iCount == pCurrentProcess->GetCurrentAmmo()->GetCount()) {
            return false;
        }
    }
    if (pAmmo == apWeapon) {
        // The weapon is its own ammo: it is equipped as ammo (form type 0x29, TESAmmo) and then restored to a weapon
        // (0x28, TESObjectWEAP).
        pAmmo->SetFormType(0x29);
        if (!pCurrentProcess ||
            pCurrentProcess->GetProcessLevel() > PROCESS_TYPE_MIDDLE_HIGH ||
            Interface::IsInMenuMode() ||
            pSaveLoadGame->GetSaveGameLoading()) {
            QueueEquipObject(static_cast<TESBoundObject*>(pAmmo), iCount, nullptr, true, false, false);
        }
        pAmmo->SetFormType(0x28);
    }
    else {
        QueueEquipObject(static_cast<TESBoundObject*>(pAmmo), iCount, nullptr, true, false, false);
    }
    if (aiReloadAnim == 1) {
        Animation* pAnimation = GetAnimation(false);
        int32_t eReloadGroup = apWeapon->GetReloadAnim(bUseModValue);
        uint16_t sGroup = GetAnimGroup(static_cast<ANIM_GROUP_ENUM>(eReloadGroup), nullptr, false, pAnimation);
        if (TESAnimGroup::GetType(sGroup) == eReloadGroup) {
            if (apWeapon->IsAutomaticWeapon()) {
                // 5, 6: AGS_WEAPON_UP, AGS_WEAPON_DOWN.
                pAnimation->BlendOut(AGS_WEAPON_UP, false);
                pAnimation->BlendOut(AGS_WEAPON_DOWN, false);
                pAnimation->PlayGroup(sGroup, ACTION_START, -1, AGS_NONE);
                StartAnimOn1stPerson(sGroup, ACTION_START);
                SetAnimAction(ANIM_ACTION_RELOAD, pAnimation->GetCurrentSequence(AGS_WEAPON));
            }
            else {
                pAnimation->QueueReloadAnim(sGroup);
            }
        }
    }
    if (!pVATS.IsInVats()) {
        UseActionPoints(ActionPoints::ACTION_RELOAD);
    }
    return true;
}

bool PlayerCharacter::ReloadWeaponNV(
    TESObjectWEAP* apWeapon,
    int32_t aiReloadAnim,
    bool bUseModValue,
    bool bFromInventory
) {
    if (apWeapon && apWeapon->GetIsLoopingReload()) {
        return ReloadLoop(apWeapon, aiReloadAnim, bUseModValue, bFromInventory);
    }

    return ReloadWeapon(apWeapon, aiReloadAnim, bUseModValue);
}

uint32_t PlayerCharacter::UseAmmo(int32_t aiShotCount) {
    if (!pCurrentProcess || !pCurrentProcess->GetCurrentAmmo()) {
        return 0;
    }
    if (!PlayerCharacter::IsGodMode() || PlayerCharacter::IsDemigodMode()) {
        return Actor::UseAmmo(aiShotCount);
    }
    return 100;
}

void PlayerCharacter::CheckTempModifiers() {
    for (ActorValue::Index aeAVIndex = ActorValue::AGGRESSION; aeAVIndex < ActorValue::ACTOR_VALUE_COUNT;
         aeAVIndex = static_cast<ActorValue::Index>(aeAVIndex + 1)) {
        if (TemporaryActorValueModifiers[aeAVIndex] != 0.0 && !HasRecoverableActorValueEffect(aeAVIndex)) {
            TemporaryActorValueModifiers[aeAVIndex] = 0.0f;
        }
    }
}

CombatGroup* PlayerCharacter::GetCombatGroup() {
    return pCombatGroup;
}

void PlayerCharacter::SetCombatGroup(CombatGroup* apGroup) {
    pCombatGroup = apGroup;
}

void PlayerCharacter::InitiateGetUpPackage() {
    if (!pPlayer->Actor::IsPathingComplete()) {
        pPlayer->pActorMover->ClearCurrentData();
    }
    SetAIControlledFromPosition(true);
    Actor::InitiateGetUpPackage();
}

// The first-person 3D fades with the actor.
void PlayerCharacter::SetAlpha(float afNewAlpha) {
    Actor::SetAlpha(afNewAlpha);
    NiAVObject* pNode = Get3D(true);
    if (pNode) {
        BSShaderUtil::RecursiveSetPropertyFadeAlpha(static_cast<NiNode*>(pNode), afNewAlpha);
    }
}

// Returns the flag's whole byte; reading it as a bool keeps clang from normalising it to 0/1.
bool PlayerCharacter::HasBeenAttacked() {
    return reinterpret_cast<const bool&>(bBeenAttacked);
}

void PlayerCharacter::SetBeenAttacked(bool abFlag) {
    bBeenAttacked = abFlag;
}

// Hands the skill use to the progression data, whose UseSkill is an empty stub.
void PlayerCharacter::UseSkill(ActorValue::Index aiSkill, uint32_t aiAction) {
    CharacterProgressionInfo.UseSkill(aiSkill, aiAction);
}

void PlayerCharacter::RewardExperience(int32_t aiAmount) {
    CharacterProgressionInfo.RewardExperience(aiAmount);
}

float PlayerCharacter::GetBaseValueOverride(ActorValue::Index aiWhich, bool& abExists) {
    return Actor::GetBaseValueOverride(aiWhich, abExists);
}

void PlayerCharacter::SetBaseValueOverride(ActorValue::Index aiWhich, float afValue) {
    Actor::SetBaseValueOverride(aiWhich, afValue);
}

// Wobbles the third-person gun node (the spine for melee, easing toward the aim assist's actor) by the weapon's wobble
// scaled by the gun skill, eased by fWobbleChaseDrift, through a blend collision object that adds the rotation; then
// the first-person wobble node the same way, or with ironsights on a scope the view itself.
void PlayerCharacter::AddGunWobble() {
    TESObjectWEAP* pweapon = GetCurrentWeapon();
    char* pwobbleNodeName = nullptr;
    float fdrift = 0.0f;
    NiMatrix3 gunWobble;
    if (pCurrentProcess && pCurrentProcess->GetWeaponDrawn()) {
        Animation* panim = Actor::GetAnimation();
        int32_t eweaponType = WEAPON_HAND_TO_HAND;
        if (!pweapon || pweapon->IsMeleeWeapon()) {
            pwobbleNodeName = const_cast<char*>("Bip01 Spine");
            float fdeltaHeading = 0.0f;
            float fdeltaLooking = 0.0f;
            NiAVObject* pprojectile = BSUtilities::GetObjectByName(pPlayer->Get3D(false), "ProjectileNode");
            if (pPlayer->GetAutoAimActor()) {
                pPlayer->ApplyAutoAimActor(
                    nullptr,
                    pprojectile,
                    fdeltaHeading,
                    fdeltaLooking,
                    nullptr,
                    std::numeric_limits<float>::min(),
                    std::numeric_limits<float>::min()
                );
            }
            double dHeadingDelta = static_cast<double>(fcurrentChaseHeading) - fdeltaHeading;
            double dHeadingStep = appTimer.GetSeconds() * dHeadingDelta;
            fcurrentChaseHeading = fcurrentChaseHeading - fAutoAimMeleeChaseMult.Float() * dHeadingStep;
            double dLookingDelta = static_cast<double>(fcurrentChaseLooking) - fdeltaLooking;
            double dLookingStep = appTimer.GetSeconds() * dLookingDelta;
            fcurrentChaseLooking = fcurrentChaseLooking - fAutoAimMeleeChaseMult.Float() * dLookingStep;
            gunWobble.FromEulerAnglesXYZ(fcurrentChaseHeading, 0.0f, fcurrentChaseLooking);
        }
        else {
            eweaponType = pweapon->GetWeaponType();
            pwobbleNodeName = GetGunWobble(weaponToAnimGroupWeapon[eweaponType], panim->GetTime(), gunWobble);
        }
        if (pwobbleNodeName) {
            NiAVObject* pwobbleObj = pWobbleNodes[0][eweaponType];
            if (!pwobbleObj) {
                pwobbleObj = BSUtilities::GetObjectByName(TESObjectREFR::Get3D(), pwobbleNodeName);
                pWobbleNodes[0][eweaponType] = pwobbleObj;
            }
            if (pwobbleObj) {
                if (!pweapon || pweapon->IsMeleeWeapon()) {
                    fdrift = 1.0f;
                }
                else {
                    fdrift = GetGunSkill(GUN_SKILL_ACTOR);
                }
                bool battacking = IsAttacking();
                if (!battacking) {
                    fdrift = fdrift * fNonAttackGunWobbleMult.Float();
                }
                float fdelta = fdrift - fWobbleChaseDrift;
                float fstep = appTimer.GetSeconds() * fGunWobbleChaseDriftTime.Float();
                // The compares are written so that a NaN delta still eases, a NaN step snaps, and a NaN delta steps
                // down.
                if (fdelta != 0.0) {
                    if (!(fstep < std::fabs(fdelta))) {
                        fWobbleChaseDrift = fdrift;
                    }
                    else if (fdelta > 0.0) {
                        if (!battacking) {
                            fWobbleChaseDrift = fWobbleChaseDrift + fstep;
                            fdrift = fWobbleChaseDrift;
                        }
                        else {
                            fWobbleChaseDrift = fdrift;
                        }
                    }
                    else {
                        fWobbleChaseDrift = fWobbleChaseDrift - fstep;
                        fdrift = fWobbleChaseDrift;
                    }
                }
                if (!pwobbleObj->GetCollisionObject()) {
                    bhkBlendCollisionObjectAddRotation* pnewBlendCol = new bhkBlendCollisionObjectAddRotation;
                    pwobbleObj->SetCollisionObject(pnewBlendCol);
                }
                else {
                    bhkBlendCollisionObject* pblendCol = static_cast<bhkBlendCollisionObject*>(
                        NiObject::DynamicCastStatic(&bhkBlendCollisionObject::ms_RTTI, pwobbleObj->GetCollisionObject())
                    );
                    if (pblendCol) {
                        NiObject* pAddRotation = NiObject::DynamicCastStatic(
                            &bhkBlendCollisionObjectAddRotation::ms_RTTI,
                            pblendCol
                        );
                        if (!pAddRotation) {
                            bhkBlendCollisionObjectAddRotation* pnewBlendCol = new bhkBlendCollisionObjectAddRotation;
                            pnewBlendCol->CopyFrom(pblendCol);
                            pwobbleObj->SetCollisionObject(pnewBlendCol);
                        }
                    }
                }
                bhkBlendCollisionObjectAddRotation* pblendColObj = static_cast<bhkBlendCollisionObjectAddRotation*>(
                    NiObject::DynamicCastStatic(
                        &bhkBlendCollisionObjectAddRotation::ms_RTTI,
                        pwobbleObj->GetCollisionObject()
                    )
                );
                if (pblendColObj) {
                    NiMatrix3 addRot = NiMatrix3::IDENTITY;
                    float fx;
                    float fy;
                    float fz;
                    gunWobble.ToEulerAnglesXYZ(fx, fy, fz);
                    gunWobble.FromEulerAnglesXYZ(fx * fdrift, fy * fdrift, fz * fdrift);
                    addRot = addRot * gunWobble;
                    pblendColObj->SetAdditionalRotation(addRot);
                }
            }
        }
    }
    GunWobbleRot = NiMatrix3::IDENTITY;
    if (pCurrentProcess && pweapon && pCurrentProcess->GetWeaponDrawn()) {
        ANIM_GROUP_WEAPON_ENUM eweaponType = weaponToAnimGroupWeapon[pweapon->GetWeaponType()];
        if (GetIronSights() && pweapon->GetScopeModel()->GetModelLength()) {
            float fVal;
            if (!pweapon->HasModScope() ||
                pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_IRON_SITES, fVal)) {
                eweaponType = AGW_NONE;
            }
        }
        pwobbleNodeName = GetGunWobble(eweaponType, p1stPersonAnimation->GetTime(), gunWobble);
        if (pwobbleNodeName) {
            if (eweaponType == AGW_NONE) {
                // With a scope up the wobble turns the view itself.
                fdrift = GetGunSkill(GUN_SKILL_HUD);
                fdrift = fdrift * fGunWobbleMultScope.Float();
                float fx;
                float fy;
                float fz;
                gunWobble.ToEulerAnglesXYZ(fx, fy, fz);
                ModLooking(fx * fdrift);
                ModHeading(fz * fdrift);
                GunWobbleRot.FromEulerAnglesXYZ(-fz * fdrift, fy * fdrift, fx * fdrift);
            }
            else {
                NiAVObject* pwobbleObj = pWobbleNodes[1][eweaponType];
                if (!pwobbleObj) {
                    pwobbleObj = BSUtilities::GetObjectByName(Get3D(true), pwobbleNodeName);
                    pWobbleNodes[1][eweaponType] = pwobbleObj;
                }
                if (pwobbleObj) {
                    if (!pwobbleObj->GetCollisionObject()) {
                        bhkBlendCollisionObjectAddRotation* pnewBlendCol = new bhkBlendCollisionObjectAddRotation;
                        pwobbleObj->SetCollisionObject(pnewBlendCol);
                    }
                    bhkBlendCollisionObjectAddRotation* pblendColObj = static_cast<bhkBlendCollisionObjectAddRotation*>(
                        NiObject::DynamicCastStatic(
                            &bhkBlendCollisionObjectAddRotation::ms_RTTI,
                            pwobbleObj->GetCollisionObject()
                        )
                    );
                    if (pblendColObj) {
                        float fx;
                        float fy;
                        float fz;
                        gunWobble.ToEulerAnglesXYZ(fx, fy, fz);
                        GunWobbleRot.FromEulerAnglesXYZ(fx * fdrift, fy * fdrift, fz * fdrift);
                        pblendColObj->SetAdditionalRotation(GunWobbleRot);
                        GunWobbleRot.FromEulerAnglesXYZ(-fz * fdrift, fy * fdrift, fx * fdrift);
                    }
                }
            }
        }
    }
}

void PlayerCharacter::AddPerk(BGSPerk* apPerk, uint8_t acRank, bool bAlt) {
    if (apPerk) {
        BSSimpleList<PerkRankData*>* plist = &Perks;
        if (bAlt) {
            plist = &CompanionPerks;
        }
        while (plist) {
            PerkRankData* pperkRank = plist->m_item;
            if (pperkRank && pperkRank->pPerk == apPerk) {
                if (pperkRank->cCurrentRank != acRank) {
                    apPerk->ApplyPerk(this, pperkRank->cCurrentRank, acRank, bAlt);
                    pperkRank->cCurrentRank = acRank;
                    if (!bAlt) {
                        StatsMenu::UpdatePerks();
                    }
                }
                return;
            }
            plist = plist->m_pkNext;
        }
        PerkRankData* pperkRank = static_cast<PerkRankData*>(ourMalloc(sizeof(PerkRankData)));
        pperkRank->pPerk = apPerk;
        pperkRank->cCurrentRank = acRank;
        if (bAlt) {
            if (!CompanionPerks.IsInList(pperkRank)) {
                CompanionPerks.AddHead(pperkRank);
            }
        }
        else if (!Perks.IsInList(pperkRank)) {
            Perks.AddHead(pperkRank);
        }
        apPerk->ApplyPerk(this, 0, acRank, bAlt);
        if (!bAlt) {
            StatsMenu::UpdatePerks();
        }
        SetReloadSpeed();
    }
}

void PlayerCharacter::RemovePerk(BGSPerk* apPerk, bool bAlt) {
    if (apPerk) {
        PerkRankData* pFound = nullptr;
        BSSimpleList<PerkRankData*>* pIter = &Perks;
        if (bAlt) {
            pIter = &CompanionPerks;
        }
        while (pIter && !pFound) {
            PerkRankData* pData = pIter->m_item;
            if (pData && pData->pPerk == apPerk) {
                pFound = pData;
            }
            else {
                pIter = pIter->m_pkNext;
            }
        }
        if (pFound) {
            apPerk->RemovePerk(this, bAlt);
            if (bAlt) {
                CompanionPerks.Remove(pFound);
            }
            else {
                Perks.Remove(pFound);
            }
            StatsMenu::UpdatePerks();
            Actor::SetReloadSpeed();
        }
    }
}

uint8_t PlayerCharacter::GetPerkRank(BGSPerk* apPerk, bool bAlt) {
    BSSimpleList<PerkRankData*>* pIter = &Perks;
    if (bAlt) {
        pIter = &CompanionPerks;
    }
    while (pIter) {
        PerkRankData* pData;
        pData = pIter->GetItem();
        if (pData && pData->pPerk == apPerk) {
            return pData->cCurrentRank;
        }
        pIter = pIter->GetNext();
    }
    return 0;
}

void PlayerCharacter::AddPerkEntry(BGSPerkEntry* apPerkEntry, bool bAlt) {
    if (apPerkEntry && apPerkEntry->GetType() == PERK_ENTRY_TYPE_ENTRY_POINT) {
        uint8_t centryPoint = static_cast<BGSEntryPointPerkEntry*>(apPerkEntry)->GetEntryPoint();
        if (centryPoint < 74u) {
            if (bAlt) {
                CompanionPerkEntryLists[centryPoint].Insert(apPerkEntry, &BGSPerk::PerkEntryPrioritySortFunction);
            }
            else {
                PerkEntryLists[centryPoint].Insert(apPerkEntry, &BGSPerk::PerkEntryPrioritySortFunction);
            }
            InventoryChanges* pinventory = pPlayer->m_Extra.GetContainerChanges();
            pinventory->ForceWeightRecalculation();
        }
    }
}

// Removes the entry from its entry point's list, or from the companion list when bAlt is set.
void PlayerCharacter::RemovePerkEntry(BGSPerkEntry* apPerkEntry, bool bAlt) {
    if (!apPerkEntry || apPerkEntry->GetType() != PERK_ENTRY_TYPE_ENTRY_POINT) {
        return;
    }
    uint8_t cEntryPoint = static_cast<BGSEntryPointPerkEntry*>(apPerkEntry)->GetEntryPoint();
    if (cEntryPoint >= 74) {
        return;
    }
    if (bAlt) {
        CompanionPerkEntryLists[cEntryPoint].Remove(apPerkEntry);
    }
    else {
        PerkEntryLists[cEntryPoint].Remove(apPerkEntry);
    }
}

BSSimpleList<BGSPerkEntry*>* PlayerCharacter::GetPerkEntryList(uint8_t acEntryPoint, bool bAlt) {
    if (acEntryPoint >= 0x4Au) {
        return nullptr;
    }
    if (bAlt) {
        return &CompanionPerkEntryLists[acEntryPoint];
    }
    return &PerkEntryLists[acEntryPoint];
}

// Plays asAnimGroup (without its 0x8000 bit) on the first-person animation: synced to the third-person sequence for a
// multi-sequence group, else the group, its non-iron-sights group (3 below), or the weapon's group of the same type.
// Type 0xFF (no group) plays nothing. Animation::GroupLoaded and Animation::PlayGroup take the group as a uint16_t and
// read only its low word, so asAnimGroup - 3 for group 0 is passed as 0xFFFD.
void PlayerCharacter::StartAnimOn1stPerson(uint16_t asAnimGroup, ACTION_FLAGS aeFlags) {
    if (asAnimGroup & 0x8000) {
        asAnimGroup &= ~0x8000;
    }
    int32_t iType = TESAnimGroup::GetType(asAnimGroup);
    if (iType == 0xFF) {
        return;
    }
    Animation* pAnimation = GetAnimation(true);
    bool bSynced = false;
    if (animGroupInfo[iType].bMultipleSequences) {
        Animation* p3rdPersonAnimation = GetAnimation(false);
        BSAnimGroupSequence* pSequence = p3rdPersonAnimation->GetCurrentSequence(animGroupInfo[iType].eSectionLayer);
        bSynced = pAnimation->SyncSequences(pSequence, asAnimGroup) != nullptr;
    }
    if (bSynced) {
        return;
    }
    if (pAnimation->GroupLoaded(asAnimGroup)) {
        pAnimation->PlayGroup(asAnimGroup, aeFlags, -1, AGS_NONE);
    }
    else if (
        TESAnimGroup::IsIronSightsAction(asAnimGroup) && pAnimation->GroupLoaded(static_cast<uint16_t>(asAnimGroup - 3))
    ) {
        pAnimation->PlayGroup(static_cast<uint16_t>(asAnimGroup - 3), aeFlags, -1, AGS_NONE);
    }
    else {
        uint16_t sGroup = GetAnimGroup(
            static_cast<ANIM_GROUP_ENUM>(iType),
            nullptr,
            GetCurrentWeapon() == nullptr,
            pAnimation
        );
        if (TESAnimGroup::GetType(sGroup) == iType ||
            (TESAnimGroup::IsIronSightsAction(static_cast<ANIM_GROUP_ENUM>(iType)) &&
             TESAnimGroup::GetType(sGroup) == iType - 3)) {
            pAnimation->PlayGroup(sGroup, aeFlags, -1, AGS_NONE);
        }
    }
}

bool PlayerCharacter::IsImmobile() {
    return GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT);
}

void PlayerCharacter::HandleHealthDamage(Actor* apAttacker, float afDamage) {
    Actor::HandleHealthDamage(apAttacker, afDamage);
    IsDead(false);
}

// Replaces the actor mover with a player mover.
void PlayerCharacter::CreateActorMover() {
    if (pActorMover) {
        delete pActorMover;
    }
    pActorMover = new PlayerMover(this);
}

NiPoint3 PlayerCharacter::CalculateAnticipatedLocation(float afDeltaT) {
    NiPoint3 kPos = *QLocationOnReference();
    if (afDeltaT > 0.0 && GetCharController()) {
        NiPoint3 kVelocity;
        bhkCharacterController* pController = GetCharController();
        pController->GetLinearVelocity(kVelocity);
        kPos += kVelocity * afDeltaT;
    }

    return kPos;
}

void PlayerCharacter::CastAbility(SpellItem* apSpell, bool abLoadCast) {
    static_cast<MagicCaster*>(this)->MagicCaster::CastAbility(apSpell, abLoadCast);
}

void PlayerCharacter::TransferDisease(SpellItem* apSpell, MagicTarget* apDesiredTarget, bool abLoadCast) {
    static_cast<MagicCaster*>(this)->MagicCaster::TransferDisease(apSpell, apDesiredTarget, abLoadCast);
}

MagicTarget* PlayerCharacter::GetTouchTarget() {
    MagicTarget* ptarget = nullptr;
    double dReach = GetReach();
    float freach = GetScale() * dReach;
    TESObjectREFR* ptargetRef = CombatUtilities::GetFacingTarget(pPlayer, freach);
    if (!ptargetRef) {
        ptargetRef = Interface::GetActivateREFR();
    }
    if (ptargetRef) {
        return ptargetRef->GetMagicTarget();
    }
    return ptarget;
}

void PlayerCharacter::StartAim() {
    StartCast();
}

bool PlayerCharacter::CheckCast(
    MagicItem* apSpell,
    float* afEffectStrength,
    MagicItem::CannotCastReason* apReason,
    bool abUseBaseMagicka
) {
    MagicItem::CannotCastReason ereason;
    ereason = MagicItem::NOCAST_OK;
    bool bresult = Actor::CheckCast(apSpell, afEffectStrength, &ereason, abUseBaseMagicka);
    if (apReason) {
        *apReason = ereason;
    }
    if (!bresult && ereason == MagicItem::NOCAST_MULTIPLEBOUNDEFFECTS) {
        return false;
    }
    return PlayerCharacter::IsGodMode() || bresult;
}

float PlayerCharacter::GetMagicEffectivenessModifier(bool abAdjust, float afModifier) {
    if (PlayerCharacter::IsGodMode()) {
        return Actor::GetMagicEffectivenessModifier(abAdjust, 0.0f);
    }

    return 1.0f;
}

void PlayerCharacter::SetCurrentSpell(MagicItem* apCurrentSpell) {
    pCurrentSpell = apCurrentSpell;
}

void PlayerCharacter::SetDesiredTarget(MagicTarget* apDesiredTarget) {
    pDesiredTarget = apDesiredTarget;
}

bool PlayerCharacter::AddTarget(
    MagicCaster* apCaster,
    MagicItem* apSpell,
    ActiveEffect* apActiveEffect,
    bool abAreaTarget
) {
    return MagicTarget::AddTarget(apCaster, apSpell, apActiveEffect, abAreaTarget);
}

BSSimpleList<ActiveEffect*>* PlayerCharacter::GetActiveEffectList() {
    return pActiveEffectList;
}

bool PlayerCharacter::IsInvulnerable() {
    return PlayerCharacter::IsGodMode();
}

// The base value plus the three modifier groups, each truncated to an integer before it is added.
int32_t PlayerCharacter::GetActorValue(ActorValue::Index aiWhich) {
    if (!GetActorBase()) {
        return 0;
    }
    int32_t iValue = GetBaseActorValue(aiWhich);
    iValue += static_cast<int32_t>(GetModifier(TEMPORARY, aiWhich));
    iValue += static_cast<int32_t>(GetModifier(SCRIPTS, aiWhich));
    return static_cast<int32_t>(GetModifier(DAMAGE, aiWhich)) + iValue;
}

// The sum is kept as a double and rounded to float once at the end.
float PlayerCharacter::GetActorFloatValue(ActorValue::Index aiWhich) {
    double dValue = GetBaseActorFloatValue(aiWhich);
    dValue = GetModifier(TEMPORARY, aiWhich) + dValue;
    dValue = GetModifier(SCRIPTS, aiWhich) + dValue;
    float fValue = static_cast<float>(GetModifier(DAMAGE, aiWhich) + dValue);
    return fValue;
}

float PlayerCharacter::GetTemporaryModifier(ActorValue::Index aiWhich) {
    return PlayerCharacter::GetModifier(TEMPORARY, aiWhich);
}

float PlayerCharacter::GetDamageModifier(ActorValue::Index aiWhich) {
    return PlayerCharacter::GetModifier(DAMAGE, aiWhich);
}

float PlayerCharacter::GetPermanentModifier(ActorValue::Index aiWhich) {
    return GetModifier(SCRIPTS, aiWhich);
}

int32_t PlayerCharacter::GetPermanentActorValue(ActorValue::Index aiWhich) {
    return static_cast<int32_t>(std::floorf(GetPermanentActorFloatValue(aiWhich)));
}

// A skill is derived from the player's attributes plus its base override; anything else is the base value. The
// permanent modifier is added and the result clamped to the value's range.
float PlayerCharacter::GetPermanentActorFloatValue(ActorValue::Index aiWhich) {
    float fValue;
    if (ActorValue::IsSkill(aiWhich)) {
        double dValue = AiFormulas::AVDeriveSkill(
            pPlayer ? static_cast<ActorValueOwner*>(pPlayer) : nullptr,
            aiWhich,
            true
        );
        bool bExists;
        dValue = GetBaseValueOverride(aiWhich, bExists) + dValue;
        fValue = static_cast<float>(GetPermanentModifier(aiWhich) + dValue);
    }
    else {
        double dValue = GetBaseActorFloatValue(aiWhich);
        fValue = static_cast<float>(GetPermanentModifier(aiWhich) + dValue);
    }
    return ActorValue::ClampActorValue(aiWhich, fValue);
}

BSSimpleList<PerkRankData*>* PlayerCharacter::GetPerkList(bool bAlt) {
    if (bAlt) {
        return &CompanionPerks;
    }
    return &Perks;
}

PlayerCharacter::TerminalAccessType PlayerCharacter::GetTerminalAccess(TESObjectREFR* apTerminalRef) {
    BGSTerminal* pTerminal = nullptr;
    if (apTerminalRef && apTerminalRef->GetObjectReference()) {
        TESBoundObject* pObject = apTerminalRef->GetObjectReference();
        pTerminal = dynamic_cast<BGSTerminal*>(pObject);
    }

    if (!pTerminal) {
        return PlayerCharacter::TAT_HACK_INELIGIBLE;
    }
    if (HasNote(pTerminal->pPassword)) {
        return PlayerCharacter::TAT_HAS_NOTE;
    }
    if (TESObjectREFR::IsHacked(apTerminalRef) || pTerminal->IsUnlocked(apTerminalRef)) {
        return PlayerCharacter::TAT_HACKED;
    }
    if (TESObjectREFR::IsLockedOut(apTerminalRef) || pTerminal->GetRawDifficulty(apTerminalRef) == LL_IMPOSSIBLE) {
        return PlayerCharacter::TAT_LOCKED;
    }
    float fScience = GetClampedActorFloatValue(ActorValue::SCIENCE);
    if (pTerminal->IsHackEligible(apTerminalRef, fScience)) {
        return PlayerCharacter::TAT_HACK_ELIGIBLE;
    }

    return PlayerCharacter::TAT_HACK_INELIGIBLE;
}

int32_t PlayerCharacter::GetLastAction() {
    if (pListofActions && pListofActions->m_item) {
        return pListofActions->m_item->eAction;
    }

    return 0;
}

bool PlayerCharacter::IsActionInList(PLAYER_ACTION aeAction) {
    bool bexits = false;
    for (BSSimpleList<PlayerActionObject*>* plist = pListofActions; plist && plist->m_item && !bexits;
         plist = plist->m_pkNext) {
        PlayerActionObject* pAction = plist->m_item;
        if (pAction) {
            bexits = pAction->eAction == aeAction;
        }
    }
    return bexits;
}

bool PlayerCharacter::QCanWakeSleepingActor() {
    return false;
}

void PlayerCharacter::UpdatePlayer3d() {
    if (!pCurrentProcess->Get3DUpdateFlag(RESET_MODEL) && Interface::GetInventoryMenuVisible()) {}
    pCurrentProcess->Update3dModel(this);
}

bool PlayerCharacter::GetDropPoint(
    TESBoundObject* apObject,
    NiPoint3& aPoint,
    bool abAllowBackupPoint,
    bool abAdvanceExtraRotate
) {
    float fobjectSize = apObject->GetBoundSize() * 2.0;
    bhkCharacterController* pcon = pPlayer->MobileObject::GetCharController();
    float fcharconRadius = HK2NI(pcon->GetRadius());
    double dScale = this->GetScale();
    float fdropDistance = fPlayerDropDistance.Float() * dScale;
    NiPoint3 origin;
    NiPoint3 dir;
    this->GetEyeVector(origin, dir, true);
    NiMatrix3 extraRot;
    if (abAdvanceExtraRotate) {
        if (this->fDropAngleMod != 0.0) {
            float fsin = fobjectSize * 0.5 / fdropDistance;
            double dangle = std::asin(BSMIN(1.0f, fsin));
            this->fDropAngleMod = dangle + this->fDropAngleMod;
        }
        extraRot.MakeZRotation(this->fDropAngleMod);
        float fangle = this->fDropAngleMod;
        this->fLastDropAngleMod = fangle;
        // abAdvanceExtraRotate is tested a second time here.
        if (abAdvanceExtraRotate) {
            float fsin = fobjectSize * 0.5 / fdropDistance;
            double dangle = std::asin(BSMIN(1.0f, fsin));
            this->fDropAngleMod = dangle + this->fDropAngleMod;
            if (this->fDropAngleMod > 6.2831854820251465) {
                this->fDropAngleMod = this->fDropAngleMod - 6.2831854820251465;
            }
        }
    }
    else {
        extraRot.MakeZRotation(this->fLastDropAngleMod);
    }
    bool bfoundPoint = false;
    int32_t inumBackups = abAllowBackupPoint ? 10 : 0;
    for (int32_t i = 0; !bfoundPoint && i <= inumBackups; ++i) {
        NiPoint3 testDirection = extraRot * dir;
        if (i) {
            testDirection.z = 0.0f;
            testDirection.Unitize();
            int32_t iprev = i - 1;
            float frotate = static_cast<double>(iprev) * (6.2831854820251465 / static_cast<double>(inumBackups));
            NiMatrix3 backupTestRot;
            backupTestRot.MakeZRotation(frotate);
            testDirection = backupTestRot * testDirection;
        }
        bool bvalidDirection = false;
        // bhkPickData is deliberately not destroyed.
        alignas(16) bhkPickData pd;
        CFilter filter;
        filter.iFilter = 0;
        filter.SetLayer(L_DROPPINGPICK);
        CFilter kplayerFilter;
        pPlayer->MobileObject::GetCollisionFilter(&kplayerFilter.iFilter);
        filter.SetGroup(kplayerFilter.GetGroup());
        pd.SetFilter(filter);
        pd.NiSetFrom(origin);
        float flength = fdropDistance + fobjectSize;
        pd.NiSetLength(testDirection * flength);
        alignas(16) hkpAllRayHitCollector pcollector;
        pd.SetAllCollector(&pcollector);
        bhkWorld* pworld = pPlayer->GetParentCell()->GetbhkWorld();
        if (pworld->PickObject(pd)) {
            double dlength = fdropDistance + fobjectSize;
            float fhit = pcollector.getHits()[0].m_hitFraction * dlength;
            float fpos = fhit - fobjectSize / 2.0;
            testDirection *= fpos;
            aPoint = origin + testDirection;
            alignas(16) NiPoint3 differenceHoriz(testDirection.x, testDirection.y, 0.0f);
            float fdist = differenceHoriz.Length() - fobjectSize / 2.0;
            if (fdist - fcharconRadius > 0.0) {
                bvalidDirection = true;
            }
        }
        else {
            float fpos = fobjectSize / 2.0 + fdropDistance;
            testDirection *= fpos;
            aPoint = origin + testDirection;
            bvalidDirection = true;
        }
        if (bvalidDirection) {
            bhkSphereShape* pshape;
            pshape = new bhkSphereShape(fobjectSize / 2.0, true);
            NiPointer<bhkSphereShape> spshape = pshape;
            CFilter kplayerFilter2;
            pPlayer->MobileObject::GetCollisionFilter(&kplayerFilter2.iFilter);
            uint32_t igroup = kplayerFilter2.GetGroup();
            alignas(16) CFilter ifilter(L_DROPPINGPICK, igroup, P_OTHER);
            alignas(16) hkSimpleShapePhantomCinfo info;
            info.m_collisionFilterInfo = ifilter.iFilter;
            info.m_shape = **spshape;
            info.m_transform.setIdentity();
            alignas(16) hkVector4 location;
            NI2HK(location, aPoint);
            info.m_transform.Trans = location;
            bhkSimpleShapePhantom pphantom(info);
            pphantom.bhkWorldObject::Add(this->GetParentCell()->GetbhkWorld());
            alignas(16) hkpAllCdBodyPairCollector collector;
            collector.hkpAllCdBodyPairCollector::reset();
            pphantom.GetPenetrations(&collector);
            pphantom.bhkPhantom::Remove();
            spshape = nullptr;
            if (!collector.getHits().m_size) {
                bfoundPoint = true;
            }
        }
    }
    if (bfoundPoint || !abAllowBackupPoint) {
        return bfoundPoint;
    }
    float fhalfSize = fobjectSize / 2.0;
    float fheight = BSMIN(64.0f, fhalfSize);
    aPoint = *this->QLocationOnReference();
    aPoint.z = aPoint.z + fheight;
    return true;
}

bool PlayerCharacter::GetAlwaysHardcore() {
    return bAlwaysHardcore;
}

void PlayerCharacter::SetSleeping(int32_t aiTime, bool absleep) {
    iSleepTime = aiTime;
    bIsSleeping = absleep;
}

void PlayerCharacter::ModAmountStolenSold(int32_t aiAmount) {
    iAmountStolenSold += aiAmount;
}

void PlayerCharacter::SetPlayerResistingArrest(bool abflag) {
    bBeingChased = abflag;
}

void PlayerCharacter::SetHardcoreCallback(int32_t aiVal) {
    if (aiVal == 1) {
        SetHardcore(static_cast<GamePlayFormulas::HardcoreSetting>(aiVal), false);
    }
    else if (aiVal == 0) {
        SetHardcore(static_cast<GamePlayFormulas::HardcoreSetting>(aiVal), false);
    }
}

void PlayerCharacter::SetPlayerGreetFlag() {
    bGreetingPlayer = true;
}

bool PlayerCharacter::GetHostileDetection() {
    return bHostileDetection;
}

void PlayerCharacter::RemoveActorFromPlayercombatList(Actor* apActor) {
    if (pCombatGroup) {
        pCombatGroup->RemoveTarget(apActor);
    }
}

void PlayerCharacter::RequestPositionPlayer(PLAYER_TARGET_LOC* apTargetLoc) {
    if (pQueuedTargetLoc) {
        if (apTargetLoc) {
            Warning("AI: PositionPlayer request bashing earlier PositionPlayer request.");
        }
        ourFree(pQueuedTargetLoc);
    }
    pQueuedTargetLoc = apTargetLoc;
    if (TES::pInstance->CellTestRunning()) {
        HandlePositionPlayerRequest();
    }
}

NiPoint3 PlayerCharacter::GetLastKnownGoodPosition() {
    return LastKnownGoodPosition;
}

BSSimpleList<TESQuestStageItem*>* PlayerCharacter::GetQuestLog() {
    // listQuestLog is a BSSimpleList<void*>; its items are the stage items.
    return reinterpret_cast<BSSimpleList<TESQuestStageItem*>*>(&listQuestLog);
}

bool PlayerCharacter::GetTelekinesisSelected() {
    return bTelekinesisSelected;
}

void PlayerCharacter::RewardKarma(int32_t aiAmount) {
    int32_t icurrent = GetActorValue(ActorValue::KARMA);
    GetActorBase();
    GetActorFloatValue(ActorValue::KARMA);
    if (aiAmount >= 0) {
        const char* pMessage;
        const char* pImage;
        if (aiAmount + icurrent > iKarmaMax.Int()) {
            aiAmount = iKarmaMax.Int() - icurrent;
        }
        if (aiAmount >= iKarmaChangeThreshold.Int()) {
            pImage = sKarmaMajorGainedImage.String();
            pMessage = sKarmaMajorGained.String();
        }
        else {
            pImage = sKarmaMinorGainedImage.String();
            pMessage = sKarmaMinorGained.String();
        }
        Interface::ShowMessage(pMessage, 0, pImage, nullptr, 2.0f, false);
        BSSoundHandle kSoundHandle;
        kSoundHandle = BSAudio::QInstance().GetSoundHandleByName("UIKarmaUp", 289u);
        kSoundHandle.Play(false);
    }
    else {
        const char* pMessage;
        const char* pImage;
        if (aiAmount + icurrent < iKarmaMin.Int()) {
            aiAmount = iKarmaMin.Int() - icurrent;
        }
        if (aiAmount >= -iKarmaChangeThreshold.Int()) {
            pImage = sKarmaMinorLostImage.String();
            pMessage = sKarmaMinorLost.String();
        }
        else {
            pImage = sKarmaMajorLostImage.String();
            pMessage = sKarmaMajorLost.String();
        }
        Interface::ShowMessage(pMessage, 0, pImage, nullptr, 2.0f, false);
        BSSoundHandle kSoundHandle;
        kSoundHandle = BSAudio::QInstance().GetSoundHandleByName("UIKarmaDown", 289u);
        kSoundHandle.Play(false);
    }
    if (aiAmount <= 0 || icurrent < iKarmaMax.Int()) {
        if (aiAmount >= 0 || icurrent > iKarmaMin.Int()) {
            PermanentModActorValue(ActorValue::KARMA, aiAmount, nullptr);
        }
    }
}

// A change re-picks the replacement locomotions and resets the 3D.
void PlayerCharacter::SetPCYoung(bool abVal) {
    if (bIsYoung != abVal) {
        bIsYoung = abVal;
        QueueReplacementLocomotions(false);
        Reset3D();
    }
}

// Clears the greeting state once the player has been greeted for more than 5 seconds.
void PlayerCharacter::CheckPlayerGreetFlag() {
    if (bGreetingPlayer && fGreetTimer > 5.0) {
        ResetPlayerGreetFlag();
    }
}

BSSimpleList<TESCaravanCard*>* PlayerCharacter::GetCaravanCards1() {
    return pInactiveListofCaravanCards;
}

BSSimpleList<TESCaravanCard*>* PlayerCharacter::GetCaravanCards2() {
    return pActiveListofCaravanCards;
}

// A file that is not good is neither closed nor deleted, and the string is then never built. Every
// Write is Write(arString, true). Call arguments with side effects are evaluated right to left, in the game's order.
void PlayerCharacter::ExportProgressData(PROGRESS_EVENT aeEvent) {
    char acFileName[0x104];
    BSsprintf(acFileName, 0x104, "%sProgressData_1.txt", sTrackProgressPath.String());
    BSFile* pFile = new BSFile(acFileName, NiFile::WRITE_ONLY, 0x4000, false);
    if (!pFile || !pFile->FileIsGood()) {
        return;
    }
    pFile->Seek(0, NiFile::ms_iSeekEnd);
    alignas(BSStringT<char>) char kStringMem[sizeof(BSStringT<char>)];
    BSStringT<char>* pString = ::new (kStringMem) BSStringT<char>;
    // The result is not used.
    GetObjectReference();
    if (userNameG[0]) {
        pString->SPrintF("%s\t", userNameG);
    }
    else {
        pString->SPrintF("%s\t", "UNKNOWN");
    }
    pFile->Write(*pString, true);
    pString->SPrintF("%s\t", "1.4.0.525");
    pFile->Write(*pString, true);
    uint32_t uiHoursPassed = appCalendar.GetHoursPassed();
    pString->SPrintF("%i\t", uiHoursPassed);
    pFile->Write(*pString, true);
    pString->SPrintF("%s\t", ProgressDataEventNames[aeEvent]);
    pFile->Write(*pString, true);
    pString->SPrintF("%s\t", TESObjectREFR::GetFullName());
    pFile->Write(*pString, true);
    pString->SPrintF("%s\t", GetRace()->TESFullName::GetFullName());
    pFile->Write(*pString, true);
    pString->SPrintF("%s\t", sexName[GetSex()]);
    pFile->Write(*pString, true);
    pString->SPrintF("%i\t", static_cast<uint32_t>(GetLevel()));
    pFile->Write(*pString, true);
    ActorValueOwner* pAVOwner = this;
    pString->SPrintF("%i\t", pAVOwner->GetActorValue(ActorValue::EXPERIENCE_POINTS));
    pFile->Write(*pString, true);
    pString->Set("\"", 0);
    for (BSSimpleList<PerkRankData*>* pNode = &Perks; pNode;) {
        if (pNode->IsEmpty()) {
            break;
        }
        PerkRankData* pPerkData = pNode->GetItem();
        pNode = pNode->GetNext();
        if (pPerkData->pPerk) {
            uint32_t uiRank = pPerkData->cCurrentRank;
            uint32_t uiFormID = pPerkData->pPerk->GetFormID();
            const char* pName = pPerkData->pPerk->TESFullName::GetFullName();
            char acPerk[0x104];
            BSsprintf(acPerk, 0x104, "%s (%08X)-%i,", pName, uiFormID, uiRank);
            *pString += acPerk;
        }
    }
    *pString += "\"\t";
    pFile->Write(*pString, true);
    for (int32_t i = 0; i < 14; ++i) {
        int32_t iAV = i + 32;
        pString->SPrintF("%i\t", pAVOwner->GetActorValue(static_cast<ActorValue::Index>(iAV)));
        pFile->Write(*pString, true);
    }
    for (int32_t i = 0; i < 7; ++i) {
        int32_t iAV = i + 5;
        pString->SPrintF("%i\t", pAVOwner->GetBaseActorValue(static_cast<ActorValue::Index>(iAV)));
        pFile->Write(*pString, true);
        pString->SPrintF("%i\t", pAVOwner->GetActorValue(static_cast<ActorValue::Index>(iAV)));
        pFile->Write(*pString, true);
    }
    for (int32_t i = 0; i < 20; ++i) {
        int32_t iAV = i + 12;
        pString->SPrintF("%i\t", pAVOwner->GetBaseActorValue(static_cast<ActorValue::Index>(iAV)));
        pFile->Write(*pString, true);
        pString->SPrintF("%i\t", pAVOwner->GetActorValue(static_cast<ActorValue::Index>(iAV)));
        pFile->Write(*pString, true);
    }
    InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(this);
    if (pInventory) {
        pString->Set("\"", 0);
        pInventory->IterateInventory(
            [](ItemChange* apItem, void* apData) {
                return static_cast<bool>(apCallback(apItem, static_cast<BSStringT<char>*>(apData)));
            },
            pString,
            0
        );
        *pString += "\"\t";
    }
    else {
        pString->SPrintF("\"\"\t");
    }
    pFile->Write(*pString, true);
    float fInventoryWeight;
    float fMaxCarryWeight;
    GetInventoryEncumbrance(fInventoryWeight, fMaxCarryWeight);
    pString->SPrintF("%.2f\t%.2f\t", fInventoryWeight, fMaxCarryWeight);
    pFile->Write(*pString, true);
    if (pInventory) {
        pString->SPrintF("%i\t", pInventory->GetInventoryValue(false, false));
    }
    else {
        pString->SPrintF("%i\t", 0);
    }
    pFile->Write(*pString, true);
    pString->SPrintF("%i\t", GetGoldCount());
    pFile->Write(*pString, true);
    // Slot 271 through the vtable.
    Actor* pActor = this;
    pString->SPrintF("%.2f\t", pActor->GetArmorRating());
    pFile->Write(*pString, true);
    pString->Set("\"", 0);
    for (BSSimpleList<void*>* pNode = &listQuestLog; pNode;) {
        if (pNode->IsEmpty()) {
            break;
        }
        // The quest log holds the stage items of the logged stages.
        TESQuestStageItem* pItem = static_cast<TESQuestStageItem*>(pNode->GetItem());
        pNode = pNode->GetNext();
        // Tests only the low byte of the flag.
        if (static_cast<uint8_t>(pItem->GetEndQuest()) && pItem->GetOwner()) {
            TESQuest* pQuest = pItem->GetOwner();
            uint32_t uiFormID = pItem->GetOwner()->GetFormID();
            const char* pEditorName = pQuest->GetEditorName();
            char acQuest[0x104];
            BSsprintf(acQuest, 0x104, "%s(%08X), ", pEditorName, uiFormID);
            *pString += acQuest;
        }
    }
    *pString += "\"\t";
    pFile->Write(*pString, true);
    for (int32_t i = 0; i < 43; ++i) {
        pString->SPrintF("%i\t", MiscStatManager::GetVal(static_cast<MiscStatManager::MiscStatID>(i)));
        pFile->Write(*pString, true);
    }
    TESObjectCELL* pCell = GetParentCell();
    if (!pCell) {
        pString->SPrintF("NONE\t");
    }
    else if (pCell->GetInterior()) {
        uint32_t uiFormID = pCell->GetFormID();
        pString->SPrintF("%s (%08X)\t", pCell->TESFullName::GetFullName(), uiFormID);
    }
    else {
        TESWorldSpace* pWorldSpace = pCell->GetWorldSpace();
        uint32_t uiFormID = pCell->GetFormID();
        int32_t iY = pCell->GetDataY();
        int32_t iX = pCell->GetDataX();
        if (pWorldSpace) {
            pString->SPrintF("%s (%i, %i) (%08X)\t", pWorldSpace->TESFullName::GetFullName(), iX, iY, uiFormID);
        }
        else {
            pString->SPrintF("UNKNOWN (%i, %i) (%08X)\t", iX, iY, uiFormID);
        }
    }
    pFile->Write(*pString, true);
    if (aeEvent == DEATH) {
        ExtraDismemberedLimbs* pLimbs = static_cast<ExtraDismemberedLimbs*>(
            GetExtra()->GetExtraData(EXTRA_DISMEMBERED_LIMBS)
        );
        if (pLimbs) {
            if (static_cast<int32_t>(pLimbs->eCauseofDeath) != -1) {
                pString->SPrintF("%s\t", CauseOfDeathNames[pLimbs->eCauseofDeath]);
            }
            else {
                pString->SPrintF("NONE\t");
            }
            pFile->Write(*pString, true);
            // The death object's line is built but never written: the next write overwrites it.
            if (pLimbs->pDeathObject) {
                uint32_t uiFormID = pLimbs->pDeathObject->GetFormID();
                const char* pEditorID = pLimbs->pDeathObject->GetFormEditorID();
                const char* pType = pLimbs->pDeathObject->GetFormTypeString();
                pString->SPrintF("%s %s(%08X)\t", pType, pEditorID, uiFormID);
            }
            else {
                pString->SPrintF("NONE\t");
            }
        }
        else {
            pString->SPrintF("UNKNOWN CAUSE\t");
            pFile->Write(*pString, true);
            pString->SPrintF("UNKNOWN OBJECT\t");
            pFile->Write(*pString, true);
        }
        if (pMyKiller) {
            uint32_t uiKillerID = pMyKiller->GetFormID();
            pString->SPrintF("%s (%08X)\t", pMyKiller->TESObjectREFR::GetFullName(), uiKillerID);
            pFile->Write(*pString, true);
            if (pMyKiller->IsLeveledCreature()) {
                TESForm* pOriginal = pMyKiller->GetExtra()->GetLevCreaOriginalBase();
                TESForm* pTemplate = pMyKiller->GetExtra()->GetLevCreaTemplate();
                if (pOriginal && pTemplate) {
                    uint32_t uiTemplateID = pTemplate->GetFormID();
                    const char* pTemplateEditorID = pTemplate->GetFormEditorID();
                    uint32_t uiOriginalID = pOriginal->GetFormID();
                    const char* pOriginalEditorID = pOriginal->GetFormEditorID();
                    pString->SPrintF(
                        "O:%s(%08X) T:%s(%08X)\t",
                        pOriginalEditorID,
                        uiOriginalID,
                        pTemplateEditorID,
                        uiTemplateID
                    );
                }
                else if (pOriginal) {
                    uint32_t uiOriginalID = pOriginal->GetFormID();
                    pString->SPrintF("O:%s(%08X) T:UNKNOWN)\t", pOriginal->GetFormEditorID(), uiOriginalID);
                }
                else if (pTemplate) {
                    uint32_t uiTemplateID = pTemplate->GetFormID();
                    pString->SPrintF("O:UNKNOWN T:%s(%08X)\t", pTemplate->GetFormEditorID(), uiTemplateID);
                }
                else {
                    pString->SPrintF("UNKNOWN LEV CREATURE\t");
                }
            }
            else {
                TESActorBase* pBase = pMyKiller->GetActorBase();
                uint32_t uiBaseID = pMyKiller->GetActorBase()->GetFormID();
                pString->SPrintF("%s (%08X)\t", pBase->GetFormEditorID(), uiBaseID);
            }
            pFile->Write(*pString, true);
            pString->SPrintF("%.2f\t", pMyKiller->GetHealth());
            pFile->Write(*pString, true);
            TESObjectWEAP* pWeapon = pMyKiller->GetCurrentWeapon();
            if (pWeapon) {
                uint32_t uiWeaponID = pWeapon->GetFormID();
                pString->SPrintF("%s (%08X)\t", pWeapon->TESFullName::GetFullName(), uiWeaponID);
            }
            else {
                pString->SPrintF("NO WEAPON\t");
            }
            pFile->Write(*pString, true);
        }
        else {
            pString->SPrintF("UNKNOWN KILLER\t");
            pFile->Write(*pString, true);
        }
    }
    pString->SPrintF("\r\n");
    pFile->Write(*pString, true);
    pFile->Close();
    delete pFile;
    pString->~BSStringT<char>();
}

uint32_t PlayerCharacter::GetCaravanLosses() {
    return iCaravanLosses;
}

uint32_t PlayerCharacter::GetCaravanLargestWinning() {
    return iCaravanLargestWinning;
}

void PlayerCharacter::SetCaravanCapWinnings(int32_t iValue) {
    iCaravanCapWinnings = iValue;
}

void PlayerCharacter::SetCaravanCapLosses(int32_t iValue) {
    iCaravanCapLosses = iValue;
}

void PlayerCharacter::SetCaravanWinnings(int32_t iValue) {
    iCaravanWinnings = iValue;
}

void PlayerCharacter::SetCaravanLosses(int32_t iValue) {
    iCaravanLosses = iValue;
}

void PlayerCharacter::SetCaravanLargestWinning(int32_t iValue) {
    iCaravanLargestWinning = iValue;
}

void PlayerCharacter::SetOccupiedRegion(TESRegion* pRegion) {
    pOccupiedRegion = pRegion;
}

BGSQuestObjective* PlayerCharacter::GetCurrentQuestObjective() {
    for (BSSimpleList<BGSQuestObjective*>* ecx0 = &listObjectives; ecx0 && ecx0->m_item; ecx0 = ecx0->m_pkNext) {
        BGSQuestObjective* pObjective = ecx0->m_item;
        if (pObjective->pOwnerQuest == pActiveQuest && pObjective->eState == QOS_DISPLAYED) {
            return pObjective;
        }
    }
    return nullptr;
}

AudioMarkerInfo* PlayerCharacter::GetClosestAudioMarkerInfo() {
    if (pClosestAudioMarkerInfo) {
        return pClosestAudioMarkerInfo;
    }
    if (AudioMarkerList.IsEmpty()) {
        return nullptr;
    }
    AudioMarkerInfo* pMarker = nullptr;
    float dist = std::numeric_limits<float>::max();
    bool bInside = false;
    bool bInsideCurrent = false;
    BSSimpleList<AudioMarkerInfo*>* pList = &AudioMarkerList;
    while (pList && pList->m_item) {
        AudioMarkerInfo* pAudioMarker = pList->m_item;
        pList = pList->m_pkNext;
        if (pAudioMarker->pMarkerRef) {
            NiPoint3 Loc = pAudioMarker->pMarkerRef->GetLocationOnReference();
            NiPoint3 Dist = Loc - GetLocationOnReference();
            float ndist = Dist.SqrXYLength();
            float rad = pAudioMarker->pMarkerRef->GetRadius();
            bInsideCurrent = ndist < rad * rad;
            if (!(dist > ndist)) {
                if (bInsideCurrent && !bInside) {
                    dist = ndist;
                    pMarker = pAudioMarker;
                    bInside = bInsideCurrent;
                }
            }
            else if (!bInside) {
                dist = ndist;
                pMarker = pAudioMarker;
                bInside = bInsideCurrent;
            }
        }
    }
    pClosestAudioMarkerInfo = pMarker;
    return pMarker;
}

void PlayerCharacter::SetControlsDisabled(bool abDisable, PlayerCharacter::DISABLE_PLAYER_CONTROLS aeDPC) {
    uint8_t ucDisabled;
    if (abDisable) {
        ucDisabled = ucControlsDisabled | aeDPC;
    }
    else {
        ucDisabled = ucControlsDisabled & ~aeDPC;
    }
    SetControlsDisabled(ucDisabled);
}

void PlayerCharacter::SetControlsDisabled(uint8_t aucDisabled) {
    ucControlsDisabled = aucDisabled;
    if (ucControlsDisabled != DPC_ALL) {
        ucControlsDisabled &= ~128;
    }
    if ((ucControlsDisabled & DPC_MOVEMENT) != 0) {
        HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_MOVEMENT_DISABLED);
    }
    else {
        HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_CHANGED);
    }
    if ((ucControlsDisabled & DPC_FIGHTING) != 0) {
        SetWantWeaponDrawn(false);
    }
    if ((ucControlsDisabled & DPC_POV_SWITCH) != 0) {
        bWant3rdPerson = false;
        if (sp1stPerson3D.m_pObject) {
            UpdateCamera(false, false);
        }
        else {
            fChaseDistance = 0.0f;
        }
    }
    if ((ucControlsDisabled & DPC_SNEAKING) != 0) {
        uint16_t smoveMode = GetMoveMode() & ~ActorMover::MF_SNEAKING;
        pActorMover->ForceMoveMode(smoveMode);
    }
}

PlayerCharacter* PlayerCharacter::SetKillCamMode(KILL_CAM_TYPE eType) {
    eKillCameraSetting = eType;
    return this;
}

void PlayerCharacter::ResetKillCam(KILL_CAM_TYPE eOldType, bool bForceReset) {
    appTimer.RestoreGlobalTimeMultiplierCache();
    if (GetAnimation(!Is3rdPerson())) {
        GetAnimation(!Is3rdPerson())->SetGlobalTimeMultiplier(1.0f);
    }
    if (eOldType == KILL_CAM_VATS && (bForceReset || !StartMenu::PauseMenuIsActive())) {
        pPlayer->SetControlsDisabled(
            false,
            static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(
                PlayerCharacter::DPC_MOVEMENT | PlayerCharacter::DPC_LOOKING
            )
        );
        pVATS.QuitVATSPlayback(false, true);
        pVATS.ClearData();
        pVATS.SetMode(VATS::VATS_MODE_NONE, true);
        bVATSForce1stPerson = true;
    }
    fTimeInSlowMoCam = 0.0f;
}

uint32_t PlayerCharacter::GetTotalPlayingTime() {
    UpdateTotalPlayingTime();
    return iTotalPlayingTime;
}

void PlayerCharacter::UpdateTotalPlayingTime() {
    iTotalPlayingTime += (BSGetTickCount() - iLastPlayingTimeUpdate);
    ResetLastPlayingTimeUpdate();
}

// Switches the view: sets the wanted third-person flag, moves the chase camera to the old first-person camera and shows
// or hides the first-person 3D. Returns whether the wanted view was already the current one, which ForceTemp1stPerson
// passes on.
bool PlayerCharacter::SetFirstPerson(bool abFirst) {
    // Only the low byte of the argument is read.
    const uint8_t ucFirstPerson = static_cast<uint8_t>(abFirst);
    const bool bWasWanted = static_cast<uint8_t>(bWant3rdPerson) == ucFirstPerson;
    bWant3rdPerson = ucFirstPerson == 0;
    // IsDead is called through a base pointer so the call is virtual.
    if (bWant3rdPerson && static_cast<Actor*>(this)->IsDead(false)) {
        kVanityModePos.y = fVanityModeWheelDeadMin.Float();
    }
    if (!bVanityMode && sp1stPerson3D && bWant3rdPerson && bWasWanted) {
        chaseCameraPos = p1stPersonCamera->GetWorldTranslate();
        this->Show1stPerson(!bWant3rdPerson);
    }
    else if (!bVanityMode && sp1stPerson3D && sp1stPerson3D->GetAppCulled() && !bWant3rdPerson) {
        b3rdPerson = true;
    }

    if (bWasWanted && bVATSForce1stPerson && bVATSReturnPlayerTo1stPerson) {
        b3rdPerson = bWant3rdPerson;
        this->Show1stPerson(true);
    }
    return bWasWanted;
}

// Switches to the third-person view for a while, raising the vanity camera distance when asked to and switching the
// view only when it is in first person. Returns false on the early exits, else SetFirstPerson's result (as
// ForceTemp1stPerson; no caller reads it).
bool PlayerCharacter::ForceTemp3rdPerson(bool abForceCameraDistance) {
    if (this->GetTempPerson()) {
        return false;
    }
    bTemp3rdPerson = true;
    if (abForceCameraDistance && fVanityModeForceDefault.Float() > kVanityModePos.y) {
        kVanityModePos.y = fVanityModeForceDefault.Float();
    }
    if (b3rdPerson) {
        return false;
    }
    bTemp3rdPersonSwitchBack = true;
    return this->SetFirstPerson(false);
}

void PlayerCharacter::SetSelectedSpell(MagicItem* apSelectedSpell) {
    if (pSelectedSpell != apSelectedSpell) {
        if (pSelectedSpell) {
            pSelectedSpell->Unload(true);
        }
        pSelectedSpell = apSelectedSpell;
        if (pSelectedSpell) {
            pSelectedSpell->Preload(false);
        }
        ResetMagicCastSound();
    }
}

void PlayerCharacter::ResetPlayerGreetFlag() {
    bGreetingPlayer = false;
    fGreetTimer = 0.0f;
}

void PlayerCharacter::CheckForClosesConversation(
    MobileObject* apSpeaker,
    MobileObject* apTarget,
    DialoguePackage* apDialogPack
) {
    float fDistToPlayer = apSpeaker->GetDistanceFromReference(pPlayer, false, false);
    float fTargetDistToPlayer = apTarget->GetDistanceFromReference(pPlayer, false, false);
    if (fDistToPlayer > static_cast<double>(fTargetDistToPlayer)) {
        fDistToPlayer = fTargetDistToPlayer;
    }
    float fClosestDist = 0x1.fffffe0000000p+127f;
    if (pPlayer->pClosestConversation) {
        PlayerCharacter* pPlayerRef;
        if (pPlayer->pClosestConversation->GetActorStartedConversation()) {
            pPlayerRef = pPlayer;
            TESObjectREFR* pStarter = pPlayer->pClosestConversation->GetActorStartedConversation();
            fClosestDist = pStarter->GetDistanceFromReference(pPlayerRef, false, false);
        }
        if (pPlayer->pClosestConversation->DialoguePackage::GetTargetOfConversation()) {
            pPlayerRef = pPlayer;
            TESObjectREFR* pConversationTarget = pPlayer->pClosestConversation
                                                     ->DialoguePackage::GetTargetOfConversation();
            float fConversationTargetDist = pConversationTarget->GetDistanceFromReference(pPlayerRef, false, false);
            if (fClosestDist > static_cast<double>(fConversationTargetDist)) {
                fClosestDist = fConversationTargetDist;
            }
        }
    }
    if (static_cast<const void*>(apTarget) == static_cast<const void*>(pPlayer) ||
        static_cast<const void*>(apSpeaker) == static_cast<const void*>(pPlayer)) {
        pPlayer->pClosestConversation = nullptr;
    }
    else if (
        (!pPlayer->pClosestConversation && fDistToPlayer <= 1200.0) ||
        (pPlayer->pClosestConversation && fDistToPlayer <= static_cast<double>(fClosestDist))
    ) {
        pPlayer->pClosestConversation = apDialogPack;
    }
}

void PlayerCharacter::AddTopics(BSSimpleList<TESTopic*>* apAddTopicList) {
    if (apAddTopicList) {
        char cAdded = 0;
        for (BSSimpleList<TESTopic*>* pList = apAddTopicList; pList && pList->m_item; pList = pList->m_pkNext) {
            if (AddTopic(pList->m_item, false, true)) {
                cAdded = 1;
            }
        }
        if (cAdded) {
            TESTopic::SortTopics(true, GetTopicList());
        }
    }
}

// Leaves vanity mode: restores the auto-vanity camera distance, puts the HUD back and resets the vanity camera.
void PlayerCharacter::StopVanityMode() {
    if (PlayerCharacter::bIsAutoVanityMode) {
        PlayerCharacter::kVanityModePos.y = PlayerCharacter::fAutoVanityModeDistRevert;
    }
    PlayerCharacter::bVanityMode = false;
    PlayerCharacter::bIsAutoVanityMode = false;
    HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_CHANGED);
    PlayerCharacter::bSnapChaseCamera = true;
    PlayerCharacter::bZoom3rdPersonCamera = false;
    PlayerCharacter::fVanityDelay = 0.0f;
    PlayerCharacter::fAutoVanityDelay = 0.0f;
    PlayerCharacter::kVanityModePos.x = 0.0f;
    PlayerCharacter::kVanityModePos.z = 0.0f;
    fZoomDelta = 0.0f;
}

void PlayerCharacter::SpellLearned(SpellItem* apSpellIn) {
    if (apSpellIn->GetSpellType() == MagicSystem::SPELL ||
        (apSpellIn->GetSpellType() == MagicSystem::ABILITY ||
         (apSpellIn->GetSpellType() == MagicSystem::POWER || apSpellIn->GetSpellType() == MagicSystem::LESSER_POWER))) {
        for (BSSimpleList<EffectItem*>* plist = apSpellIn; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
            EffectItem* pitem = plist->m_item;
            if (pitem) {
                pitem->pEffectSetting->SetFlag(EffectSetting::PLAYER_KNOWS, true);
            }
        }
    }
}

void PlayerCharacter::IncNumCrimes(PackageNS::CRIME_TYPE aiCrimeType, int32_t aiAmount) {
    pCrimeCounts[aiCrimeType] += aiAmount;
}

// Readies an inventory node for use: drops every controller that is not a transform controller, clears the
// collision object and shows the whole subtree.
void PlayerCharacter::RecursiveReadyInventoryNode(NiNode* apNode) {
    NiTimeController* pController;
    uint32_t auiIndex;
    NiAVObject* pChild;
    if (!apNode) {
        return;
    }
    pController = apNode->GetControllers();
    while (pController) {
        NiTimeController* pNext = pController->GetNext();
        if (!NiObject::IsKindOf_0(&NiTransformController::ms_RTTI, pController)) {
            apNode->RemoveController(pController);
        }
        pController = pNext;
    }
    apNode->SetCollisionObject(nullptr);
    auiIndex = 0;
    for (uint32_t i = apNode->GetArrayCount(); auiIndex < i; i = apNode->GetArrayCount()) {
        pChild = apNode->GetAt(auiIndex);
        if (pChild) {
            pChild->SetAppCulled(false);
            NiNode* pChildNode = pChild->IsNode();
            if (pChildNode) {
                this->RecursiveReadyInventoryNode(pChildNode);
            }
        }
        ++auiIndex;
    }
}

NiNode* PlayerCharacter::GetInventory3D() {
    return spInventoryPC.m_pObject;
}

void PlayerCharacter::SetShotsSinceLastAmmoRegenTick(uint8_t acShots) {
    cShotsSinceLastAmmoRegen = static_cast<int8_t>(acShots);
}

// Records the note on the player's list, once, and shows its name in the message area.
void PlayerCharacter::AddNote(BGSNote* apNote, bool abVerbose) {
    BSStringT<char> kString;
    if (!apNote) {
        Warning("SCRIPTS: Attempt made to add a NULL note to the player character.");
        return;
    }
    if (!listNotes.IsInList(apNote)) {
        listNotes.AddHead(apNote);
        const char* pFullName = apNote->GetFullName();
        kString.SPrintF(sComputersAddedNote.String(), pFullName);
        if (abVerbose) {
            Interface::PrintLine(kString.String());
        }
    }
    // The emotion argument is the HUD's neutral emotion, 0.
    Interface::ShowMessage(kString.String(), 0, nullptr, "UIItemGenericUp", 2.0f, false);
}

void PlayerCharacter::SetSelectedScroll(TESObjectBOOK* apScroll) {
    if (!apScroll || static_cast<TESEnchantableForm*>(apScroll)->TESEnchantableForm::GetFormEnchanting()) {
        if (!pSelectedScroll || pSelectedScroll != apScroll) {
            if (pSelectedScroll && !apScroll) {
                SetSelectedSpell(nullptr);
            }

            pSelectedScroll = apScroll;
            if (pSelectedScroll) {
                EnchantmentItem* pEnchantment = static_cast<TESEnchantableForm*>(pSelectedScroll)
                                                    ->TESEnchantableForm::GetFormEnchanting();
                if (pEnchantment) {
                    SetSelectedSpell(static_cast<MagicItem*>(pEnchantment));
                }
                else {
                    SetSelectedSpell(nullptr);
                }
            }
            else {
                SetSelectedSpell(nullptr);
            }
        }
    }
    else {
        pSelectedScroll = nullptr;
    }
}

// Swaps the ammo of the current weapon for the next type in the weapon's ammo list that the player carries, searching
// on from the current ammo and wrapping around (from the start when the current ammo is not in the list). With
// bOnlyIfEmpty nothing changes while the player still carries the current ammo.
void PlayerCharacter::AmmoSwapHelper(bool abStartReloadAnimation, bool bOnlyIfEmpty) {
    if (!pCurrentProcess || !pCurrentProcess->GetCurrentWeapon()) {
        return;
    }
    ItemChange* pWeaponItem = pCurrentProcess->GetCurrentWeapon();
    TESObjectWEAP* pWeapon = pWeaponItem ? static_cast<TESObjectWEAP*>(pWeaponItem->GetContainerObject()) : nullptr;
    if (!pWeapon || !pWeapon->GetAmmoFormList()) {
        return;
    }
    BSSimpleList<TESForm*>* pAmmoList = pWeapon->GetAmmoFormList()->GetFormList();
    TESForm* pCurrentAmmo = pWeapon->GetCurrentAmmo(pPlayer);
    TESForm* pAmmo = nullptr;
    bool bFound = false;
    bool bPassedCurrent = false;
    ItemChange* pItem = nullptr;
    if (pCurrentAmmo && pAmmoList) {
        TESForm* pSearch = pCurrentAmmo;
        if (!pAmmoList->IsInList(pSearch)) {
            if (pCurrentProcess) {
                pCurrentProcess->SetCurrentAmmo(nullptr);
            }
            pCurrentAmmo = nullptr;
        }
    }
    pItem = GetInventoryItem(static_cast<TESBoundObject*>(pCurrentAmmo), 0);
    if (bOnlyIfEmpty && pCurrentAmmo && pItem && pItem->GetCount() > 0) {
        if (pItem) {
            delete pItem;
        }
        return;
    }
    if (pItem) {
        delete pItem;
    }
    pItem = nullptr;
    while (pAmmoList && pAmmoList->GetItem()) {
        pAmmo = pAmmoList->GetItem();
        pAmmoList = pAmmoList->GetNext();
        if (pCurrentAmmo == pAmmo) {
            if (bPassedCurrent) {
                break;
            }
            bPassedCurrent = true;
        }
        else if (bPassedCurrent || !pCurrentAmmo) {
            pItem = GetInventoryItem(static_cast<TESBoundObject*>(pAmmo), 0);
            if (pItem && pItem->GetCount() > 0) {
                bFound = true;
                break;
            }
        }
        // At the end of the list the search wraps around to its start.
        if (!pAmmoList || !pAmmoList->GetItem()) {
            pAmmoList = pWeapon->GetAmmoFormList()->GetFormList();
        }
    }
    if (bFound) {
        if (pWeaponItem->GetHotKey() >= 0) {
            SetAmmoForHotKey(pWeaponItem->GetHotKey(), static_cast<TESAmmo*>(pAmmo));
        }
        InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(this);
        int32_t iCount = pChanges->GetObjectCount(static_cast<TESBoundObject*>(pAmmo));
        ItemChange* pNewAmmo = new ItemChange(static_cast<TESBoundObject*>(pAmmo), iCount);
        pCurrentProcess->SetCurrentAmmo(pNewAmmo);
        pCurrentProcess->GetCurrentAmmo()->SetCount(0);
        int32_t iReloadAnim = (abStartReloadAnimation && IsWeaponDrawn()) ? 2 : 0;
        // 2 is WEAPON_MOD_INCREASE_CLIP_SIZE.
        bool bUseModValue = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(2);
        TESObjectWEAP* pReloadWeapon = static_cast<TESObjectWEAP*>(
            pCurrentProcess->GetCurrentWeapon()->GetContainerObject()
        );
        ReloadWeaponNV(pReloadWeapon, iReloadAnim, bUseModValue, !abStartReloadAnimation);
    }
    if (pItem) {
        delete pItem;
    }
}

void PlayerCharacter::SetAmmoForHotKey(int32_t aiHotkey, TESAmmo* apAmmo) {
    HotKeyLastAmmo.SetAt(aiHotkey, apAmmo);
}

TESAmmo* PlayerCharacter::GetAmmoForHotKey(int32_t aiHotkey) {
    if (aiHotkey < 0 || aiHotkey > 8) {
        return nullptr;
    }
    return HotKeyLastAmmo[aiHotkey];
}

// The locals passed by address (kDirection, kFrom, the hkpRootCdPoint and the scaled direction of the final position)
// are alignas(16), so that callees receive 16-aligned addresses.
void PlayerCharacter::UpdateChaseCamera(NiPoint3* apCameraPos, NiPoint3* apTargetPos, bool abSnapXY) {
    NiPoint3 kCameraPos;
    // -1 zooming in, 1 zooming out, 0 settled.
    int8_t cZoom = static_cast<int8_t>(bWant3rdPerson - b3rdPerson);
    float fZoomMult = 1.0f;
    if (cZoom) {
        if (abSnapXY || bSnapChaseCamera) {
            cZoom = 0;
        }
        else {
            double dWheel = kVanityModePos.y - fVanityModeWheelMin.Float();
            double dWheelFraction = dWheel / (fVanityModeWheelMax.Float() - fVanityModeWheelMin.Float());
            fZoomMult = static_cast<float>(
                fFirstPersonZoomMaxMult.Float() * dWheelFraction + fFirstPersonZoomMinMult.Float()
            );
        }
    }
    if (abSnapXY || bSnapChaseCamera) {
        kCameraPos = *apCameraPos;
        chaseCameraPos = kCameraPos;
    }
    else {
        kCameraPos = *apCameraPos;
        if (bIsAutoVanityMode) {
            float fSeconds = appTimer.GetSeconds();
            float fVanityXYMult = fChase3rdPersonVanityXYMult.Float();
            chaseCameraPos += (kCameraPos - chaseCameraPos) * fVanityXYMult * fSeconds;
            kCameraPos = chaseCameraPos;
        }
        else {
            chaseCameraPos = kCameraPos;
        }
    }
    alignas(16) NiPoint3 kDirection = kCameraPos - *apTargetPos;
    float fDistance = kDirection.UnitizeGetLength();
    bCameraCasterHitObject = false;
    if (pCameraCaster) {
        alignas(16) NiPoint3 kFrom = *apTargetPos;
        NiPoint3 kTo = kCameraPos;
        alignas(16) hkpRootCdPoint kPoint;
        if (!pCameraCaster->GetWorld()) {
            bhkWorld* pWorld;
            if (GetCharController()) {
                pWorld = static_cast<bhkCharacterController*>(GetCharController())->GetWorld();
            }
            else {
                pWorld = nullptr;
            }
            pCameraCaster->Add(pWorld);
        }
        if (pCameraCaster->Pick(kFrom, kTo, kPoint)) {
            kTo -= kFrom;
            if (fDistance - kTo.Length() > 2.0) {
                bCameraCasterHitObject = true;
            }
            double dHitLength = kTo.Length();
            fDistance = static_cast<float>(dHitLength - fCameraCasterSize.Float() * 0.5);
        }
    }
    float fStep = appTimer.GetSeconds() * fChase3rdPersonZUnitsPerSecond.Float() * fZoomMult;
    float fMaxDistance = fChaseCameraMax.Float();
    if (bVanityMode) {
        fMaxDistance = fVanityModeWheelMax.Float();
    }
    if (cZoom) {
        if (cZoom == 1 && fDistance < cZoom * fStep + fChaseDistance) {
            fChaseDistance = fDistance;
            fZoomDelta = 0.0f;
            cZoom = 0;
        }
        else {
            fChaseDistance = cZoom * fStep + fChaseDistance;
        }
        if (fMaxDistance < fChaseDistance) {
            fChaseDistance = fMaxDistance;
            fZoomDelta = 0.0f;
            cZoom = 0;
        }
        else if (fChaseDistance < 0.0) {
            fChaseDistance = 0.0f;
            fZoomDelta = 0.0f;
            cZoom = 0;
        }
    }
    else if (abSnapXY || bSnapChaseCamera) {
        fChaseDistance = fDistance;
        bSnapChaseCamera = false;
    }
    else {
        if (fChaseDistance >= fDistance) {
            fChaseDistance = fDistance;
        }
        else if (fDistance < fChaseDistance + fStep) {
            fChaseDistance = fDistance;
        }
        else {
            fChaseDistance = fChaseDistance + fStep;
        }
        if (!bCameraCasterHitObject && fVanityModeWheelMin.Float() > fChaseDistance) {
            fChaseDistance = fVanityModeWheelMin.Float();
        }
        if (fMaxDistance < fChaseDistance) {
            fChaseDistance = fMaxDistance;
        }
    }
    NiPoint3 kNewPos = *apTargetPos + kDirection * fChaseDistance;
    bool bKeptOut = false;
    if (cZoom || pVATS.GetMode() == VATS::VATS_PLAYBACK) {
        if (cZoom < 0) {
            if (fZoom3rdPersonSnapDist.Float() > fChaseDistance) {
                fChaseDistance = 0.0f;
                fZoomDelta = 0.0f;
                cZoom = 0;
            }
        }
        else if (fZoom3rdPersonSnapDist.Float() > fChaseDistance) {
            fChaseDistance = fZoom3rdPersonSnapDist.Float();
        }
    }
    else if (!Interface::IsMenuIDVisible(Interface::eRaceSexMenu, 0) && pVATS.GetMode() != VATS::VATS_PLAYBACK) {
        bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
        float fScaleOffset = static_cast<float>(GetHeight() - 124.0);
        if (pCameraCaster->KeepOutOfCharacter(pController, kNewPos, fCameraCasterPlayerSize.Float(), fScaleOffset)) {
            bKeptOut = true;
        }
    }
    if (bKeptOut && !bTemp3rdPerson) {
        if (fPlayerCameraAlpha > 0.0 && !bSkipAlphaFadeOut) {
            fPlayerCameraAlpha = static_cast<float>(fPlayerCameraAlpha - appTimer.GetSeconds() * 4.0);
            if (fPlayerCameraAlpha < 0.0) {
                fPlayerCameraAlpha = 0.0f;
            }
            UpdateAlpha();
        }
    }
    else {
        if (fPlayerCameraAlpha < 1.0) {
            fPlayerCameraAlpha = static_cast<float>(appTimer.GetSeconds() * 4.0 + fPlayerCameraAlpha);
            if (fPlayerCameraAlpha > 1.0 || cZoom || pVATS.GetMode() == VATS::VATS_PLAYBACK) {
                fPlayerCameraAlpha = 1.0f;
            }
            UpdateAlpha();
        }
        bSkipAlphaFadeOut = false;
    }
    alignas(16) NiPoint3 kScaledDirection = kDirection * fChaseDistance;
    NiPoint3 kFinalPos = *apTargetPos + kScaledDirection;
    if (cZoom) {
        NiPoint3 kMoved = *apCameraPos - kFinalPos;
        double dMoved = kMoved.Length();
        fZoomDelta = static_cast<float>(dMoved / appTimer.GetSeconds());
        if (cZoom == -1) {
            NiPoint3 kToFirstPerson = *apCameraPos - p1stPersonCamera->GetWorldTranslate();
            double dMovedAgain = kMoved.Length();
            if (kToFirstPerson.Length() <= dMovedAgain) {
                fZoomDelta = 0.0f;
                fChaseDistance = 0.0f;
            }
        }
    }
    *apCameraPos = kFinalPos;
}

BSSimpleList<TESTopic*>* PlayerCharacter::GetTopicList() {
    // listTopics is a BSSimpleList<void*>; its items are the topics.
    return reinterpret_cast<BSSimpleList<TESTopic*>*>(&listTopics);
}

BSSimpleList<TESRegionSound*>* PlayerCharacter::GetRegionSounds() {
    // CurrentRegionSoundList is declared as BSSimpleList<void*>; its items are the region sounds.
    return reinterpret_cast<BSSimpleList<TESRegionSound*>*>(&CurrentRegionSoundList);
}

CharacterProgression* PlayerCharacter::GetCharacterProgressionInfo() {
    return &CharacterProgressionInfo;
}

bool PlayerCharacter::Is3rdPerson() {
    return b3rdPerson;
}

void PlayerCharacter::Force3rdPerson(bool ab3rdPerson) {
    bWant3rdPerson = ab3rdPerson;
    b3rdPerson = ab3rdPerson;
}

bool PlayerCharacter::IsActually3rdPerson() {
    return bActually3rdPerson;
}

NiAVObject* PlayerCharacter::GetCurrent3D() {
    if (IsActually3rdPerson()) {
        return TESObjectREFR::Get3D();
    }

    return sp1stPerson3D;
}

bool PlayerCharacter::IsSleeping() {
    return bIsSleeping;
}

bool PlayerCharacter::IsSleepingorResting() {
    return iSleepTime > 0;
}

bool PlayerCharacter::IsPCYoung() {
    return bIsYoung;
}

bool PlayerCharacter::IsPCToddler() {
    return bIsToddler;
}

bool PlayerCharacter::IsHardcore() {
    return eHardcoreSetting == 1;
}

bool PlayerCharacter::GetAllowEGMCacheClear() {
    return bAllowEGMCacheClear;
}

bool PlayerCharacter::GetMenuModeButtonClicked() {
    return bMenuModeButtonClicked;
}

bool PlayerCharacter::GetUnderAIControl() {
    return bAiControlledFromPos || bAiControlledToPos || bAiControlledActivate || bAiControlledPackage;
}

bool PlayerCharacter::IsAcousticSpaceTransition() {
    return bIsAcousticSpaceTransition;
}

// The first-person left arm playing a pip-boy group, else the pip-boy menu being open.
bool PlayerCharacter::IsPipboyActive() {
    if (p1stPersonAnimation &&
        p1stPersonAnimation->GetCurrentSequence(AGS_LEFT_ARM) &&
        (p1stPersonAnimation->GetCurrentSequence(AGS_LEFT_ARM)->GetAnimGroup()->GetType() == ANIM_GROUP_PIPBOY ||
         p1stPersonAnimation->GetCurrentSequence(AGS_LEFT_ARM)->GetAnimGroup()->GetType() == ANIM_GROUP_PIPBOY_CHILD)) {
        return true;
    }

    return Interface::IsInPipboyMenu();
}

BSSimpleList<CompassTarget*>* PlayerCharacter::GetListofPerceivedActors() {
    return pListofPercievedActors;
}

// Under kPerceivedActorsLock.
bool PlayerCharacter::IsTargetPerceivedAndHostile(Actor* target) {
    bool bHostile = false;
    kPerceivedActorsLock.Lock(nullptr);
    for (BSSimpleList<CompassTarget*>* pIter = pListofPercievedActors; pIter;) {
        CompassTarget* pTarget;
        pTarget = pIter->GetItem();
        if (!pTarget) {
            break;
        }
        pTarget = pIter->GetItem();
        if (pTarget->pTarget == target) {
            bHostile = pTarget->bIsHostile;
            break;
        }
        pIter = pIter->GetNext();
    }
    kPerceivedActorsLock.Unlock();
    return bHostile;
}

// Under kPerceivedActorsLock.
void PlayerCharacter::RemovePerceivedActor(Actor* apActor) {
    kPerceivedActorsLock.Lock(nullptr);
    for (BSSimpleList<CompassTarget*>* pIter = pListofPercievedActors; pIter;) {
        CompassTarget* pTarget;
        pTarget = pIter->GetItem();
        if (!pTarget) {
            break;
        }
        CompassTarget** ppTarget;
        ppTarget = &pIter->GetItem();
        if ((*ppTarget)->pTarget == apActor) {
            ourFree(*ppTarget);
            pIter->RemoveHead();
            break;
        }
        pIter = pIter->GetNext();
    }
    kPerceivedActorsLock.Unlock();
}

// Calls GetItem twice.
TESObjectREFR* PlayerCharacter::GetRecentAction() {
    TESObjectREFR* pRef = nullptr;
    if (pListofActions) {
        PlayerActionObject* pAction;
        pAction = pListofActions->GetItem();
        if (pAction) {
            pAction = pListofActions->GetItem();
            pRef = pAction->prefObj;
        }
    }
    return pRef;
}

// Under kPerceivedActorsLock (see PlayerCharacter.h).
CompassTarget* PlayerCharacter::IsActorPerceived(Actor* apActor) {
    CompassTarget* pFound = nullptr;
    kPerceivedActorsLock.Lock(nullptr);
    for (CompassTarget* pCompassTarget : pListofPercievedActors->ItemsUntilNull()) {
        if (pCompassTarget->pTarget == apActor) {
            pFound = pCompassTarget;
            break;
        }
    }
    kPerceivedActorsLock.Unlock();
    return pFound;
}

// Under kPerceivedActorsLock (see PlayerCharacter.h).
bool PlayerCharacter::IsPerceivedActorDetected(Actor* apActor) {
    bool bDetected = false;
    kPerceivedActorsLock.Lock(nullptr);
    for (CompassTarget* m_item : pListofPercievedActors->ItemsUntilNull()) {
        if (m_item->pTarget == apActor) {
            bDetected = !m_item->bIsUndetected;
            break;
        }
    }
    kPerceivedActorsLock.Unlock();
    return bDetected;
}

int32_t PlayerCharacter::GetNumberOfFollowers() {
    int32_t iCount = 0;
    ExtraDataList* pExtra = pPlayer->TESObjectREFR::GetExtra();
    ExtraFollower* pFollowerExtra = pExtra->GetFollowerExtra();
    if (pFollowerExtra && pFollowerExtra->pActorlist) {
        BSSimpleList<Actor*>* pIter = pFollowerExtra->pActorlist;
        while (pIter && pIter->m_item) {
            Actor* pFollower = pIter->m_item;
            if (pFollower &&
                pFollower->MobileObject::GetCurrentAIProcess() &&
                pFollower->GetCurrentPackage() &&
                pFollower->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_FOLLOW &&
                static_cast<const void*>(pFollower->GetCurrentTarget()) == static_cast<const void*>(pPlayer)) {
                ++iCount;
                pIter = pIter->m_pkNext;
            }
            else {
                pFollowerExtra->pActorlist->Remove(pFollower);
                pIter = pFollowerExtra->pActorlist;
                iCount = 0;
            }
        }
    }

    return iCount;
}

bool PlayerCharacter::IsPlayerFollower(Actor* apActor) {
    ExtraDataList* pExtra = pPlayer->GetExtra();
    ExtraFollower* pFollowerExtra = pExtra->GetFollowerExtra();
    if (pFollowerExtra && pFollowerExtra->pActorlist) {
        for (Actor*& rpItem : pFollowerExtra->pActorlist->ItemsUntilNull()) {
            if (rpItem == apActor) {
                return true;
            }
        }
    }
    return false;
}

TESQuest* PlayerCharacter::GetActiveQuest() {
    return pActiveQuest;
}

void PlayerCharacter::SetActiveQuest(TESQuest* apActiveQuest) {
    if (apActiveQuest != pActiveQuest) {
        pActiveQuest = apActiveQuest;
        if (!pActiveQuest) {
            listQuestTargets.RemoveAll();
        }
    }
}

int32_t PlayerCharacter::GetNumRockItAmmo() {
    int32_t iCount = 0;
    for (uint32_t uiIndex = 0; uiIndex < RockItLauncherAmmoList.iSize; ++uiIndex) {
        iCount += RockItLauncherAmmoList.GetAt(uiIndex)->iNumber;
    }
    return iCount;
}

bool PlayerCharacter::IsInSlowMo() {
    return fTimeInSlowMoCam > 0.0;
}

// The exe's cleanup-only C++ EH frame is left out.
void PlayerCharacter::UpdateAutoAimActor() {
    alignas(16) float fVATSPickDistance;
    float fbestValue;
    alignas(16) CFilter fltr;
    alignas(16) NiMatrix3 kCameraRotMatrix;
    NiMatrix3 kCombinedRot;
    NiMatrix3 playerRot;
    if (Get3D()) {
        bool bDistant;
        // Starts at 0 (a departure): only the hit branch writes it, and its length is taken on every path; the exe
        // reads it unset when the pick hit no reference (the length is then unused).
        alignas(16) NiPoint3 kHitOffset(0.0f, 0.0f, 0.0f);
        TESObjectWEAP* pWeapon;
        Actor* phitRef;
        CFilter icolFilter;
        Actor* pbestActor = nullptr;
        float fBestDistance = fAutoAimMaxDistance.Float();
        NiCamera* pCamera = Main::GetWorldSceneGraph()->GetCamera();
        float fScreenTargetY = fAutoAimScreenPercentage.Float() * 0.004999999888241291;
        float fScreenTargetX = -fAutoAimScreenPercentage.Float() * 0.004999999888241291;
        NiPoint2 screenTarget(fScreenTargetX, fScreenTargetY);
        MobileObject* pmob = nullptr;
        this->pPlayersTargetActor = nullptr;
        for (uint32_t i = 0; i < ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
            pmob = ReferenceProcessLists.MobProcessArray.GetItem(i);
            if (pmob &&
                pmob->IsActor() &&
                !pmob->IsDead(false) &&
                pmob->GetCurrentProcessType() == PROCESS_TYPE_HIGH &&
                pmob->Get3D() &&
                pPlayer->IsActorPerceived(static_cast<Actor*>(pmob))) {
                Actor* ptestActor = static_cast<Actor*>(pmob);
                const NiPoint3* pPlayerPos = &this->GetLocationOnReference();
                const NiPoint3* pActorPos = &ptestActor->GetLocationOnReference();
                NiPoint3 kDelta = *pActorPos - *pPlayerPos;
                float fDistance = kDelta.Length();
                bool bIsHostileToPlayer = pPlayer->IsActorPerceived(static_cast<Actor*>(pmob))->bIsHostile;
                if (fBestDistance > static_cast<double>(fDistance)) {
                    NiPoint3 npcMinY;
                    NiPoint3 npcMaxX;
                    alignas(16) NiPoint3 npcMinX;
                    NiBound kBound = pmob->Get3D()->GetWorldBound();
                    NiPoint3 npcMaxY;
                    pCamera->ScreenSpaceBoundBound(kBound, npcMinY, npcMaxY, 1e-05f);
                    kBound.SetRadius(ptestActor->GetBoundMax().x);
                    pCamera->ScreenSpaceBoundBound(kBound, npcMinX, npcMaxX, 1e-05f);
                    char bShouldBeTargeted = 1;
                    if (npcMinX.x == -1.0 && npcMaxX.x == 1.0) {
                        bShouldBeTargeted = 0;
                    }
                    else if (
                        !(screenTarget.x > static_cast<double>(npcMinX.x)) ||
                        !(screenTarget.x > static_cast<double>(npcMaxX.x))
                    ) {
                        if (!(screenTarget.y < static_cast<double>(npcMinX.x)) ||
                            !(screenTarget.y < static_cast<double>(npcMaxX.x))) {
                            if (npcMinY.y == -1.0 && npcMaxY.y == 1.0) {
                                bShouldBeTargeted = 0;
                            }
                            else if (
                                !(screenTarget.x > static_cast<double>(npcMinY.y)) ||
                                !(screenTarget.x > static_cast<double>(npcMaxY.y))
                            ) {
                                if (screenTarget.y < static_cast<double>(npcMinY.y) &&
                                    screenTarget.y < static_cast<double>(npcMaxY.y)) {
                                    bShouldBeTargeted = 0;
                                }
                            }
                            else {
                                bShouldBeTargeted = 0;
                            }
                        }
                        else {
                            bShouldBeTargeted = 0;
                        }
                    }
                    else {
                        bShouldBeTargeted = 0;
                    }
                    if (bShouldBeTargeted && pPlayer->LineOfSight(false, ptestActor, true, nullptr, false)) {
                        this->pPlayersTargetActor = ptestActor;
                        if (bIsHostileToPlayer) {
                            pbestActor = ptestActor;
                            fBestDistance = fDistance;
                        }
                    }
                }
            }
        }
        bhkPickData pickData;
        SpecificItemCollector collector(L_LOS, pbestActor);
        if (pbestActor) {
            pickData.SetClosestCollector(&collector);
        }
        NiPoint3 origPoint = this->GetLocationOnReference();
        NiPoint3 destPoint(0.0f, fAutoAimMaxDistance.Float(), 0.0f);
        if (pVATS.eMode == VATS::VATS_PLAYBACK || PlayerCharacter::bVanityMode) {
            alignas(16) NiMatrix3 rotMod;
            double dEyeHeight = this->GetEyeHeight();
            origPoint.z = dEyeHeight + origPoint.z;
            float fHeading = this->GetHeading(false);
            playerRot.MakeZRotation(fHeading);
            float fLooking = this->GetLooking();
            rotMod.MakeXRotation(fLooking);
            playerRot = *::new (&kCombinedRot) NiMatrix3(playerRot * rotMod);
            destPoint = playerRot * destPoint + origPoint;
        }
        else {
            alignas(16) float afRow[3];
            NiMatrix3 rotMod;
            kCameraRotMatrix = pCamera->GetWorldTransform().m_Rotate;
            kCameraRotMatrix.GetRow(0, afRow[1], afRow[2], afRow[0]);
            rotMod.SetRow(0, afRow);
            kCameraRotMatrix.GetRow(1, afRow[1], afRow[2], afRow[0]);
            rotMod.SetRow(1, afRow);
            kCameraRotMatrix.GetRow(2, afRow[1], afRow[2], afRow[0]);
            rotMod.SetRow(2, afRow);
            origPoint = *this->GetCameraPosition();
            destPoint = rotMod * destPoint + origPoint;
        }
        pickData.NiSetFrom(origPoint);
        pickData.NiSetTo(destPoint);
        this->GetCollisionFilter(&icolFilter.iFilter);
        fltr.iFilter = 0;
        fltr.SetLayer(L_LOS);
        fltr.SetGroup(icolFilter.GetGroup());
        pickData.SetFilter(fltr);
        // The whole byte returned by the pick count override getter is passed back when restoring it.
        uint8_t ucPrevPickOverride = bhkWorld::QPickCountOverride();
        bhkWorld::SetPickCountOverride(1);
        NiAVObject* phit = TES::pInstance->Pick(pickData);
        bhkWorld::SetPickCountOverride(ucPrevPickOverride);
        if (phit) {
            phitRef = static_cast<Actor*>(TESObjectREFR::FindReferenceFor3D(phit));
        }
        else {
            phitRef = nullptr;
        }
        this->spTargeted3D = phit;
        this->BulletAutoAim = origPoint + (destPoint - origPoint) * pickData.m_hitFraction;
        ItemChange* pWeaponItem = this->pCurrentProcess->GetCurrentWeapon();
        if (pWeaponItem) {
            pWeapon = static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj);
        }
        else {
            pWeapon = nullptr;
        }
        if (pVATS.eMode != VATS::VATS_MENU_MODE && phitRef && phitRef == pbestActor) {
            NiPoint3 kRayDir;
            NiPoint3 kRayOrigin;
            NiNode* pBest3D = pbestActor->Get3D();
            fbestValue = fAutoAimMaxDistance.Float();
            int32_t iCursorY = static_cast<int32_t>(InterfaceManager::Instance()->sRealCursorPos.z);
            int32_t iCursorX = static_cast<int32_t>(InterfaceManager::Instance()->sRealCursorPos.x);
            Main::GetWorldSceneGraph()->GetCamera()->WindowPointToRay(iCursorX, iCursorY, kRayOrigin, kRayDir, nullptr);
            NiAVObject* pPicked = ArrowProjectile::PickSceneGraph(pBest3D, kRayOrigin, kRayDir, fbestValue, false);
            if (pPicked) {
                TESObjectREFR* pPickedRef = TESObjectREFR::FindReferenceFor3D(pPicked);
                if (pPickedRef && pPickedRef->IsActor()) {
                    this->pPlayersTargetActor = static_cast<Actor*>(pPickedRef);
                }
            }
        }
        else if (
            pVATS.eMode == VATS::VATS_MENU_MODE &&
            pVATSTarget &&
            pWeapon &&
            !pWeapon->IsMeleeWeapon() &&
            !InterfaceManager::Has360Controller()
        ) {
            NiPoint3 kRayDir;
            NiPoint3 kRayOrigin;
            NiNode* pTarget3D = pVATSTarget->Get3D();
            const NiPoint3* pPlayerPos = &pPlayer->Get3D()->GetWorldTranslate();
            fVATSPickDistance = (pVATSTarget->Get3D()->GetWorldTranslate() - *pPlayerPos).Length() + 128.0;
            int32_t iCursorY = static_cast<int32_t>(InterfaceManager::Instance()->sRealCursorPos.z);
            int32_t iCursorX = static_cast<int32_t>(InterfaceManager::Instance()->sRealCursorPos.x);
            Main::GetWorldSceneGraph()->GetCamera()->WindowPointToRay(iCursorX, iCursorY, kRayOrigin, kRayDir, nullptr);
            NiAVObject*
                pPicked = ArrowProjectile::PickSceneGraph(pTarget3D, kRayOrigin, kRayDir, fVATSPickDistance, false);
            if (pPicked) {
                Actor* pHitActor = static_cast<Actor*>(TESObjectREFR::FindReferenceFor3D(pPicked));
                eVATSTargetLimb = ActorValue::VALUE_NONE;
                if (pHitActor->IsActor()) {
                    BGSBodyPart::LIMB_ENUM eLimb = BGSBodyPart::LIMB_NONE;
                    if (pPicked->IsNode()) {
                        eLimb = pHitActor->GetDamageLocation(pPicked->IsNode());
                    }
                    else {
                        eLimb = pHitActor->GetDamageLocation(pPicked->m_pkParent);
                    }
                    VATSMenu::SetLimb(eLimb);
                    BGSBodyPartData* pBodyPartData = pHitActor->GetActorBase()->GetBodyPartData();
                    if (pBodyPartData) {
                        BGSBodyPart* pBodyPart = pBodyPartData->GetBodyPart(eLimb);
                        if (pBodyPart) {
                            eVATSTargetLimb = static_cast<ActorValue::Index>(pBodyPart->GetActorValue());
                        }
                    }
                }
            }
        }
        if (phitRef && phitRef == pbestActor && this->GetCurrentWeapon()) {
            if (!this->GetCurrentWeapon()->IsMeleeWeapon()) {
                pbestActor = nullptr;
            }
        }

        this->pAutoAimActor = pbestActor;
        if (phitRef) {
            kHitOffset = phitRef->GetLocationOnReference() - origPoint;
        }
        float fHitDistance = kHitOffset.Length();
        if (phitRef) {
            bDistant = fHitDistance > static_cast<double>(InterfaceManager::Instance()->iPickDistance);
        }
        else {
            bDistant = false;
        }
        this->bTarget3DDistant = bDistant;
        this->UpdateImageSpaceIronsights();
        this->UpdateImageSpaceVATS();
    }
}

Actor* PlayerCharacter::GetAutoAimActor() {
    if (pAutoAimActor && pAutoAimActor->Get3D()) {
        return pAutoAimActor;
    }

    return nullptr;
}

void PlayerCharacter::ApplyAutoAimActor(
    Projectile* apProjectile,
    NiAVObject* apReleaseObj,
    float& apHeading,
    float& apLooking,
    NiPoint3* apOrigin,
    float afStartHeading,
    float afStartLooking
) {
    const NiPoint3* pStartPos;
    NiMatrix3 kInvGunRot;
    TESObjectREFR* phitRef;
    NiMatrix3 gunRot;
    NiPoint3 delta = NiPoint3::ZERO;
    VATS_COMMAND* pcurrentVATSAction = pVATS.GetCurrentAction();
    bool bvatsPlayback = false;
    NiAVObject* pvatsTarget3d = nullptr;
    if (apOrigin) {
        pStartPos = apOrigin;
    }
    else if (apReleaseObj) {
        pStartPos = &apReleaseObj->GetWorldTranslate();
    }
    else {
        pStartPos = &NiPoint3::ZERO;
    }
    NiPoint3 startPos = *pStartPos;
    if (!GetIronSights() ||
        pVATS.eMode ||
        !GetCurrentWeapon() ||
        ((!GetCurrentWeapon()->HasModScope() ||
          !pCurrentProcess ||
          !pCurrentProcess->GetCurrentWeapon() ||
          !pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_IRON_SITES)) &&
         (!GetCurrentWeapon()->HasScope() || GetCurrentWeapon()->HasModScope()))) {
        if (pVATS.eMode == VATS::VATS_PLAYBACK && pcurrentVATSAction && pcurrentVATSAction->pTarget) {
            bvatsPlayback = true;
            pvatsTarget3d = pcurrentVATSAction->pTarget->Get3D();
        }

        float fadditionalMissHeading = 0.0f;
        float fadditionalMissLooking = 0.0f;
        if (!apReleaseObj || !GetCurrentWeapon() || GetCurrentWeapon()->IsMeleeWeapon()) {
            if (GetAutoAimActor()) {
                Actor* pAutoAimActor = GetAutoAimActor();
                const NiPoint3& kPlayerCenter = Get3D(false)->GetWorldBound().m_kCenter;
                const NiBound& kTargetBound = pAutoAimActor->Get3D()->GetWorldBound();
                delta = kTargetBound.m_kCenter - kPlayerCenter;
            }
            else if (BulletAutoAim != NiPoint3::ZERO) {
                delta = BulletAutoAim - Get3D(false)->GetWorldBound().m_kCenter;
            }
        }
        else if (!bvatsPlayback) {
            if (GetAutoAimActor()) {
                delta = GetAutoAimActor()->Get3D()->GetWorldBound().m_kCenter - startPos;
            }
            else if (BulletAutoAim != NiPoint3::ZERO) {
                delta = BulletAutoAim - startPos;
            }
        }
        else {
            if (pcurrentVATSAction->pTarget->IsActor() && (!apProjectile || apProjectile->GetFormType() != PGRE_ID)) {
                Actor* pactor = static_cast<Actor*>(pcurrentVATSAction->pTarget);
                BGSBodyPartData* pbodyPartData = pactor->GetActorBase()->GetBodyPartData();
                if (pbodyPartData) {
                    ActorValue::Index eTargetBodyPart = pcurrentVATSAction->eLimb;
                    BGSBodyPart* pbodyPart = pbodyPartData->GetBodyPart(eTargetBodyPart);
                    if (pbodyPart || eTargetBodyPart == ActorValue::VALUE_NONE) {
                        const char* pname = eTargetBodyPart == ActorValue::VALUE_NONE ? "Weapon"
                                                                                      : pbodyPart->GetTargetName();
                        NiAVObject* ptargetBodyPart3d = BSUtilities::GetObjectByName(pactor->Get3D(), pname);
                        if (ptargetBodyPart3d) {
                            pvatsTarget3d = ptargetBodyPart3d;
                        }
                    }
                }
            }
            if (!pvatsTarget3d) {
                return;
            }
            NiPoint3 ktargetLocation = pvatsTarget3d->GetWorldTranslate();
            if (pcurrentVATSAction->bSuccess) {
                NiPoint3 localPoint = NiPoint3::ZERO;
                if (!apProjectile || apProjectile->GetFormType() != PGRE_ID) {
                    localPoint = TESHavokUtilities::GetLocalPointWithinCollision(pvatsTarget3d);
                }
                if (apProjectile && apProjectile->pTarget) {
                    ProjectileTarget* ptarget = apProjectile->pTarget;
                    ptarget->pReference = pcurrentVATSAction->pTarget;
                    ptarget->spPart = pvatsTarget3d;
                    ptarget->kOffset = localPoint;
                }

                ktargetLocation = pvatsTarget3d->GetWorldTransform() * localPoint;
            }
            else {
                const NiPoint3* pTargetCenter;
                if (pvatsTarget3d->GetWorldBound().m_fRadius == 0.0) {
                    pTargetCenter = &pvatsTarget3d->GetWorldTranslate();
                }
                else {
                    pTargetCenter = &pvatsTarget3d->GetWorldBound().m_kCenter;
                }
                ktargetLocation = *pTargetCenter;
            }
            delta = ktargetLocation - startPos;
            if (!pcurrentVATSAction->bSuccess) {
                float ftargetRadius = pvatsTarget3d->GetWorldBound().m_fRadius;
                if (ftargetRadius == 0.0) {
                    ftargetRadius = 16.0f;
                }
                float fMissRatioHigh = fAutoAimMissRatioHigh.Float();
                double dMissRatio = RandomFloat(fAutoAimMissRatioLow.Float(), fMissRatioHigh);
                ftargetRadius = dMissRatio * ftargetRadius;
                float fdistanceToTarget = delta.Length();
                float fdistanceToMissPoint = NiSqrt(
                    ftargetRadius * ftargetRadius + fdistanceToTarget * fdistanceToTarget
                );
                float frandomAngle = RandomFloat(0.0f, 6.2831855f);
                double dCos = std::cos(frandomAngle);
                fadditionalMissHeading = dCos * (ftargetRadius / fdistanceToMissPoint);
                double dSin = std::sin(frandomAngle);
                fadditionalMissLooking = dSin * (ftargetRadius / fdistanceToMissPoint);
            }
        }
        bool bHasStartAngles = apProjectile ||
            (afStartLooking != 1.1754943508222875e-38 && afStartHeading != 1.1754943508222875e-38);
        if (delta != NiPoint3::ZERO) {
            float fheading;
            float flooking;
            float fmaxDegreeDelta;
            NiPoint3 kdirection = delta;
            kdirection.Unitize();
            if (!bHasStartAngles) {
                if (apReleaseObj) {
                    gunRot = apReleaseObj->GetWorldTransform().m_Rotate;
                }
                else {
                    gunRot = Get3D(false)->GetWorldTransform().m_Rotate;
                }
                kdirection = *gunRot.Inverse(&kInvGunRot) * kdirection;
                if (!bvatsPlayback) {
                    kdirection = GunWobbleRot * kdirection;
                }
            }
            if (kdirection.y == 0.0) {
                fheading = 0.0f;
                flooking = 0.0f;
            }
            else {
                if (bvatsPlayback && apProjectile && apProjectile->GetProjectileBase()->GetGravity() > 0.0) {
                    if (apProjectile->GetFormType() == PGRE_ID) {
                        double dDistance = delta.Length();
                        float fLowSpeedSqr = CombatUtilities::GetWorldGravity() * dDistance;
                        float flowSpeed = std::sqrt(fLowSpeedSqr);
                        float fhighSpeed = apProjectile->GetSpeed() * 1.25;
                        float fspeedMult = 0.0f;
                        for (int32_t i = 1; fspeedMult == 0.0 && i < 2; ++i) {
                            for (float fspeed = flowSpeed;
                                 fspeedMult == 0.0 && fhighSpeed >= static_cast<double>(fspeed);
                                 fspeed = (fhighSpeed - flowSpeed) * 0.1 + fspeed) {
                                delta.z = delta.z + 10.0;
                                float fGravity = CombatUtilities::GetWorldGravity();
                                NiPoint3 kProjectedPoint = CombatUtilities::GetProjectedPointForTarget(
                                    startPos,
                                    delta + startPos,
                                    NiPoint3::ZERO,
                                    fspeed,
                                    fGravity,
                                    false
                                );
                                phitRef = nullptr;
                                bool bresult = CombatUtilities::CalculateProjectileLOS(
                                    pPlayer,
                                    apProjectile->GetProjectileBase(),
                                    fspeed,
                                    startPos,
                                    kProjectedPoint,
                                    nullptr,
                                    &phitRef,
                                    nullptr
                                );
                                if (!bresult || phitRef == pcurrentVATSAction->pTarget) {
                                    double dSpeed = apProjectile->GetSpeed();
                                    fspeedMult = fspeed / dSpeed;
                                }
                            }
                        }
                        if (fspeedMult != 0.0) {
                            double dSpeedMult = apProjectile->fSpeedMult;
                            float fNewSpeedMult = dSpeedMult * fspeedMult;
                            apProjectile->SetSpeedMult(fNewSpeedMult);
                        }
                    }
                    if (bHasStartAngles) {
                        fheading = GetZAngleFromVector(kdirection);
                        flooking = -CombatUtilities::FindAngleOfProjectile_0(apProjectile, delta, 0.0f, false);
                    }
                    else {
                        fheading = std::atanf(kdirection.x / kdirection.y);
                        flooking = -CombatUtilities::FindAngleOfProjectile_0(apProjectile, delta, 0.0f, false) -
                            apLooking;
                    }
                }
                else {
                    if (bHasStartAngles) {
                        fheading = GetZAngleFromVector(kdirection);
                    }
                    else {
                        fheading = std::atanf(kdirection.x / kdirection.y);
                    }
                    flooking = -std::asinf(kdirection.z);
                }
            }
            float fheadingDelta = 0.0f;
            float flookingDelta = 0.0f;
            if (bHasStartAngles) {
                while (fheading < -3.1415927410125732) {
                    fheading = fheading + 6.2831854820251465;
                }
                while (fheading > 3.1415927410125732) {
                    fheading = fheading - 6.2831854820251465;
                }
                if (flooking > 1.5533430576324463) {
                    flooking = 1.553343f;
                }
                if (flooking < -1.5533430576324463) {
                    flooking = -1.553343f;
                }
                float fstartHeading = apProjectile ? apProjectile->GetHeading(false) : afStartHeading;
                float fstartLooking = apProjectile ? apProjectile->GetLooking() : afStartLooking;
                while (fstartHeading < -3.1415927410125732) {
                    fstartHeading = fstartHeading + 6.2831854820251465;
                }
                while (fstartHeading > 3.1415927410125732) {
                    fstartHeading = fstartHeading - 6.2831854820251465;
                }
                if (fstartLooking > 1.5533430576324463) {
                    fstartLooking = 1.553343f;
                }
                if (fstartLooking < -1.5533430576324463) {
                    fstartLooking = -1.553343f;
                }
                fheadingDelta = fheading - fstartHeading;
                flookingDelta = flooking - fstartLooking;
            }
            else {
                fheadingDelta = fheading;
                flookingDelta = flooking;
            }
            while (fheadingDelta < -3.1415927410125732) {
                fheadingDelta = fheadingDelta + 6.2831854820251465;
            }
            while (fheadingDelta > 3.1415927410125732) {
                fheadingDelta = fheadingDelta - 6.2831854820251465;
            }
            if (!GetCurrentWeapon() || GetCurrentWeapon()->IsMeleeWeapon()) {
                if (fheadingDelta < 0.0) {
                    fheadingDelta = 0.0f;
                }
                flookingDelta = 0.0f;
            }
            if (GetCurrentWeapon() && !GetCurrentWeapon()->IsMeleeWeapon()) {
                if (bvatsPlayback) {
                    fmaxDegreeDelta = fAutoAimMaxDegreesVATS.Float();
                }
                else if (IsActually3rdPerson()) {
                    fmaxDegreeDelta = fAutoAimMaxDegrees3rdPerson.Float();
                }
                else {
                    fmaxDegreeDelta = fAutoAimMaxDegrees.Float();
                }
            }
            else {
                fmaxDegreeDelta = fAutoAimMaxDegreesMelee.Float();
            }
            fmaxDegreeDelta = fmaxDegreeDelta * 0.01745329238474369;
            if (fmaxDegreeDelta < static_cast<double>(fheadingDelta)) {
                fheadingDelta = fmaxDegreeDelta;
            }
            else if (fheadingDelta < -fmaxDegreeDelta) {
                fheadingDelta = -fmaxDegreeDelta;
            }
            if (fmaxDegreeDelta < static_cast<double>(flookingDelta)) {
                flookingDelta = fmaxDegreeDelta;
            }
            else if (flookingDelta < -fmaxDegreeDelta) {
                flookingDelta = -fmaxDegreeDelta;
            }
            TESObjectWEAP* pweapon = GetCurrentWeapon();
            if (bHasStartAngles) {
                bool bUseSplit = false;
                if (pCurrentProcess->GetCurrentWeapon()) {
                    bUseSplit = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_SPLIT_BEAM);
                }
                if (bvatsPlayback) {
                    if (pweapon && pweapon->GetNumProjectiles(bUseSplit, false, pPlayer) > 1u) {
                        float fSpreadHeading = apProjectile ? apProjectile->fSpreadHeading : 0.0f;
                        fheadingDelta = fVatsShotgunSpreadRatio.Float() * fSpreadHeading + fheadingDelta;
                        float fSpreadLooking = apProjectile ? apProjectile->fSpreadLooking : 0.0f;
                        flookingDelta = fVatsShotgunSpreadRatio.Float() * fSpreadLooking + flookingDelta;
                    }
                }
                else {
                    NiPoint3 wobbleDirection;
                    GunWobbleRot.GetCol(1u, wobbleDirection);
                    double dWobbleHeading = GetZAngleFromVector(wobbleDirection);
                    fheadingDelta = dWobbleHeading + fheadingDelta;
                    double dWobbleLooking = std::asinf(wobbleDirection.z);
                    flookingDelta = flookingDelta - dWobbleLooking;
                    float fSpreadHeading = apProjectile ? apProjectile->fSpreadHeading : 0.0f;
                    fheadingDelta = fheadingDelta + fSpreadHeading;
                    float fSpreadLooking = apProjectile ? apProjectile->fSpreadLooking : 0.0f;
                    flookingDelta = flookingDelta + fSpreadLooking;
                }
            }
            if (bvatsPlayback && !pcurrentVATSAction->bSuccess) {
                float fmissMaxDegreeDelta = fAutoAimMaxDegreesMiss.Float() * 0.01745329238474369;
                if (fmissMaxDegreeDelta < static_cast<double>(fadditionalMissHeading)) {
                    fadditionalMissHeading = fmissMaxDegreeDelta;
                }
                else if (fadditionalMissHeading < -fmissMaxDegreeDelta) {
                    fadditionalMissHeading = -fmissMaxDegreeDelta;
                }
                if (fmissMaxDegreeDelta < static_cast<double>(fadditionalMissLooking)) {
                    fadditionalMissLooking = fmissMaxDegreeDelta;
                }
                else if (fadditionalMissLooking < -fmissMaxDegreeDelta) {
                    fadditionalMissLooking = -fmissMaxDegreeDelta;
                }
                fheadingDelta = fheadingDelta + fadditionalMissHeading;
                flookingDelta = flookingDelta + fadditionalMissLooking;
            }
            apHeading = apHeading + fheadingDelta;
            apLooking = apLooking + flookingDelta;
            while (apHeading < -3.1415927410125732) {
                apHeading = apHeading + 6.2831854820251465;
            }
            while (apHeading > 3.1415927410125732) {
                apHeading = apHeading - 6.2831854820251465;
            }
            if (apLooking > 1.5533430576324463) {
                apLooking = 1.553343f;
            }
            if (apLooking < -1.5533430576324463) {
                apLooking = -1.553343f;
            }
        }
    }
}

// Carries out the queued position request: a fast travel, or a move into a worldspace or interior (seating the player
// on the requested furniture), then frees it.
bool PlayerCharacter::HandlePositionPlayerRequest() {
    bool bretval = false;
    if (pQueuedTargetLoc) {
        TES::pInstance->ClearCellLoaderTasks();
        spTargeted3D = nullptr;
        if (pQueuedTargetLoc->pFastTravelMarker) {
            TESObjectREFR* plink = pQueuedTargetLoc->pFastTravelMarker->GetLinkedRef();
            pPlayer->FastTravel(plink ? plink : pQueuedTargetLoc->pFastTravelMarker);
            bretval = true;
        }
        else {
            if (pQueuedTargetLoc->pWorld || pQueuedTargetLoc->pInterior) {
                if (pQueuedTargetLoc->pWorld) {
                    PositionPlayerExterior(
                        pQueuedTargetLoc->Location,
                        pQueuedTargetLoc->Angle,
                        pQueuedTargetLoc->pWorld,
                        pQueuedTargetLoc->bResetWeather
                    );
                }
                else {
                    PositionPlayer(
                        pQueuedTargetLoc->Location,
                        pQueuedTargetLoc->Angle,
                        pQueuedTargetLoc->pInterior,
                        pQueuedTargetLoc->bResetWeather
                    );
                }
                bhkCharacterController* pcon = nullptr;
                pcon = static_cast<bhkCharacterController*>(GetCharController());
                pcon->SetStartingHeight(pQueuedTargetLoc->Location.z);
                if (pQueuedTargetLoc->pfnArrivalFunc) {
                    pQueuedTargetLoc->pfnArrivalFunc(pQueuedTargetLoc->iArrivalFuncData);
                }
                bretval = true;
            }
            else {
                Warning("AI: Invalid PositionPlayer request encountered. No movement will take place.");
            }
            if (pQueuedTargetLoc->pFurnitureRef && pQueuedTargetLoc->pFurnitureRef->IsFurniture()) {
                BaseProcess* pProcess = GetCurrentAIProcess();
                FurnitureMark* pmarker = pProcess->GetFurnitureMarker();
                int32_t index = -1;
                TESFurniture* pFurniture = static_cast<TESFurniture*>(
                    pQueuedTargetLoc->pFurnitureRef->GetObjectReference()
                );
                bool bsleep = false;
                if (pFurniture->CanSleepOn()) {
                    bsleep = true;
                }
                index = pQueuedTargetLoc->pFurnitureRef->GetFirstFreeMarkerIndex(true);
                if (index != -1 && pQueuedTargetLoc->pFurnitureRef->GetMarkerAtIndex(index, *pmarker)) {
                    PutActorInChairBedQuick(pQueuedTargetLoc->pFurnitureRef, pmarker, index, bsleep);
                }
            }
        }
        GarbageCollector::ClearAll(true);
        ourFree(pQueuedTargetLoc);
        pQueuedTargetLoc = nullptr;
    }
    if (bretval && !pSaveLoadGame->GetSaveGameLoading() && !pPlayer->GetFlag66DIs2()) {
        pPlayer->SetCanFastTravel(true);
    }
    return bretval;
}

// Places the player at aPos, turned to aAngle, in apCell: swaps the encounter zone times, loads the interior or leaves
// for the exterior, faces the player and the camera, settles the player onto the ground and refreshes the listener, the
// trees, the terrain and the region name.
void PlayerCharacter::PositionPlayer(NiPoint3 aPos, NiPoint3 aAngle, TESObjectCELL* apCell, bool abResetWeather) {
    if (!TES::pInstance->CellTestRunning()) {
        appMain->RenderMenuBackground();
    }
    ReferenceProcessLists.RemoveDetectionEventsOnActors();
    if (StartMenu::MainMenuIsActive()) {
        Interface::CloseLoadingMenu();
        Menu* pStartMenu = Menu::GetMenuByClass(StartMenu::GetClassStatic())->GetMenu();
        delete pStartMenu;
    }
    bool binterior = TES::pInstance->GetInterior() != nullptr;
    TESWorldSpace* pcurrworld = TES::pInstance->GetWorldSpace();
    BGSEncounterZone* pnextzone = apCell->GetEncounterZone();
    BGSEncounterZone* pcurrzone = nullptr;
    if (GetParentCell()) {
        pcurrzone = GetParentCell()->GetEncounterZone();
    }
    else if (pcurrworld) {
        pcurrzone = pcurrworld->GetEncounterZone();
    }
    if (pcurrzone != pnextzone) {
        uint32_t itime = appCalendar.GetHoursPassed();
        if (pcurrzone) {
            pcurrzone->SetDetachTime(itime);
        }
        if (pnextzone) {
            pnextzone->SetAttachTime(itime);
        }
    }
    bool bmovieSequencePlaying = pMoviePlayer->GetPlayingSequence();
    if (GetParentCell()) {
        GetParentCell()->RemoveReference(this);
    }
    SetPosition(aPos);
    SetRotation(aAngle);
    TESObjectREFR::SetParentCell(apCell);
    if (apCell && apCell->GetInterior()) {
        pLastKnownMusicType = apCell->GetMusicType(nullptr);
        TES::pInstance->LoadInterior(apCell, &aPos);
        BSTreeManager::ClearCanopyShadow();
    }
    else {
        if (apCell) {
            pLastKnownMusicType = apCell->GetMusicType(nullptr);
            if (BSTreeManager::GetUseCanopyShadows() && *apCell->GetWorldSpace()->GetCanopyShadowTextureName()) {
                BSTreeManager::GetCanopyShadow();
            }
            else {
                BSTreeManager::ClearCanopyShadow();
                TES::pInstance->GetGridCellA()->ClearCanopyShadowMaskTexture();
            }
            TES::pInstance->SetWorldSpace(apCell->GetWorldSpace());
        }
        // Leaving an interior: put out the pip-boy light.
        if (binterior && IsSpellTarget(MagicSystem::GetPipBoyLightSpell(), true)) {
            Interface::GetPipboy()->ShowPipboyLightEffect(true, false, true);
            Interface::GetPipboy()->ShowPipboyLightEffect(false, false, true);
            Dispel(MagicSystem::GetPipBoyLightSpell(), nullptr, nullptr);
        }

        TES::pInstance->LeaveInterior(&aPos);
    }
    BSAudio::QInstance().EndAllMovingSounds();
    BSAudio::QInstance().EndAllSoundsOfType(BSGameSound::SOUND_FLAG_REGION);
    if (abResetWeather) {
        Sky::GetInstance()->ResetWeather();
    }
    TESObjectREFR::SetParentCell(nullptr);
    apCell->AddReference(this, false);
    if (!Get3D() && !pSaveLoadGame->GetSaveGameLoading()) {
        pIOManager->LoadAllQueuedPriorities();
        pFaderManager->CreateFader(FADER_TO_BLACK, 0.0f, false);
    }

    NiMatrix3 looking = NiMatrix3::IDENTITY;
    NiMatrix3 rot;
    while (aAngle.z > 3.1415927410125732) {
        aAngle.z = aAngle.z - 6.2831854820251465;
    }
    while (aAngle.z < -3.1415927410125732) {
        aAngle.z = aAngle.z + 6.2831854820251465;
    }
    rot.MakeZRotation(aAngle.z);
    looking = looking * rot;
    // Takes the sum for a heading below zero or unordered (NaN).
    if (!(aAngle.z >= 0.0)) {
        rot.MakeXRotation(aAngle.x + aAngle.y);
    }
    else {
        rot.MakeXRotation(aAngle.x - aAngle.y);
    }
    if (Get3D()) {
        Get3D()->SetRotate(rot);
    }
    looking = rot * looking;
    Main::GetWorldSceneGraph()->GetCameraRoot()->SetRotate(looking);
    Main::GetWorldSceneGraph()->GetCameraRoot()->SetTranslate(aPos);
    NiUpdateData UpdateData(0.0f, false, false);
    Main::GetWorldSceneGraph()->GetCameraRoot()->Update(UpdateData);
    TES::pInstance->GetRoot()->UpdateProperties();

    BSAudio::QInstance().PositionListener(aPos);
    // GetHeading/NiCos run before GetHeading/NiSin.
    float fcos = NiCos(GetHeading(false));
    float fsin = NiSin(GetHeading(false));
    NiPoint3 front(fsin, fcos, 0.0f);
    static_cast<BSWin32Audio&>(BSAudio::QInstance()).SetOrientation(front, NiPoint3::UNIT_Z);
    CheckForQuestTargetUpdate(this);
    BSShaderAccumulator* paccum = BSShaderManager::GetAccumulator();
    if (paccum) {
        paccum->ClearAllBoundVolumes();
    }

    if (sp1stPerson3D) {
        Interface::GetPipboy()->ReInitialize();
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->ProcessQueuedLights();
        if (!pSaveLoadGame->GetSaveGameLoading()) {
            // Lift an exterior arrival above the land, then let the player fall until a step moves it less than 0.1.
            NiPoint3 startpos = GetLocationOnReference();
            if (apCell && !apCell->GetInterior()) {
                float fheight = 0.0f;
                if (apCell->GetLandHeight(startpos, fheight) && startpos.z < fheight + 32.0) {
                    startpos.z = fheight + 32.0;
                    SetPosition(startpos);
                }
            }
            int32_t n;
            for (n = 0; n < 100; ++n) {
                NiPoint3 lastPos = GetLocationOnReference();
                NiPoint3 delta = NiPoint3::ZERO;
                MobileObject::Move(0.16f, delta, 0);
                NiPoint3 currentPos = GetLocationOnReference();
                float fdiff = currentPos.z - lastPos.z;
                if (std::fabs(fdiff) < 0.10000000149011612) {
                    break;
                }
            }
            if (n == 100) {
                SetPosition(startpos);
            }
        }
        // Update both the first- and the third-person animation once.
        b3rdPerson = !b3rdPerson;
        Character::Update(0.0f);
        UpdateAnimationMovement(GetAnimation(!b3rdPerson), 0.0f);
        b3rdPerson = !b3rdPerson;
        Character::Update(0.0f);
        UpdateAnimationMovement(GetAnimation(!b3rdPerson), 0.0f);
        UpdateCamera(false, false);
    }
    if (spCameraRigidBody) {
        // Converts the position into hkv and then moves the body by the game-unit position.
        hkVector4 hkv;
        NiPoint3 loc = GetLocationOnReference();
        NI2HK(hkv, loc);
        spCameraRigidBody->SetPosition(loc);
    }
    BSTreeManager::Update(Main::WorldRootCamera(), false);
    TES::pInstance->UpdateCellAnimations(appTimer.GetSeconds());
    if (!pSaveLoadGame->GetSaveGameLoading()) {
        LoadingMenu::SuspendBackgroundThread();
    }
    if (bmovieSequencePlaying) {
        pMoviePlayer->WaitForSequenceToEnd();
    }
    ResetMagicCastSound();
    TES::pInstance->SetupTemporaryParticleCache();
    if (GetWorldSpace()) {
        // Takes the position before walking to the terrain manager.
        const NiPoint3* pPos = &GetLocationOnReference();
        GetWorldSpace()->GetTerrainManager()->Update(*pPos, BGSTerrainManager::UPDATE_ALL);
        pPos = &GetLocationOnReference();
        GetWorldSpace()->GetTerrainManager()->UpdateMorphParameters(*pPos);
    }
    appMain->ForceUpdateTextures();
    if (Interface::bIsLoadingNewGame) {
        TES::pInstance->ShowLoadingMenu(false, nullptr, false);
        HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_MOVIE);
        FalloutAudioMedia::MediaClose();
        pMoviePlayer->Play(BGSMoviePlayer::sIntroMovie.String(), true, true, false, true, true, false, true, true);
        HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_MOVEMENT_DISABLED);
    }
    if (pPlayer->GetWorldSpace()) {
        HUDMainMenu::SetRegionText(pPlayer->GetWorldSpace()->TESFullName::GetFullName());
    }
    else {
        HUDMainMenu::SetRegionText(nullptr);
    }
    ReferenceProcessLists.MoveFollowersOnToPlayer();
}

// Places the player in apWorld's exterior cell at aPos: cancels the exterior cell loads, then takes the cell at the
// position's cell coordinates (FloatToInt >> 12), else loads it, else has the data handler create it, and hands it to
// the global player's PositionPlayer.
void PlayerCharacter::PositionPlayerExterior(
    NiPoint3 aPos,
    NiPoint3 aAngle,
    TESWorldSpace* apWorld,
    bool abResetWeather
) {
    TESObjectCELL* pCell = nullptr;
    if (apWorld) {
        if (pExteriorCellLoader) {
            pExteriorCellLoader->CancelAllCellLoads();
        }
        int32_t iX = FloatToInt(aPos.x) >> 12;
        int32_t iY = FloatToInt(aPos.y) >> 12;
        pCell = apWorld->GetCellFromCellCoord(iX, iY);
        ClearCellsIfTargetNotLoaded(pCell, apWorld);
        if (!pCell) {
            pCell = apWorld->LoadCell(iX, iY);
        }
        if (!pCell) {
            pCell = pDataHandler->NewCell(nullptr, iX, iY, apWorld);
        }
    }
    if (pCell) {
        pPlayer->PositionPlayer(aPos, aAngle, pCell, abResetWeather);
    }
}

void PlayerCharacter::FastTravel(TESObjectREFR* apRef) {
    MemoryLevelManager::MEM_FREE_DATA freedata;
    float ftotaldist;
    float fCheckPerk;
    fCheckPerk = 0.0f;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_ALWAYS_FAST_TRAVEL, this, &fCheckPerk);
    if (!IsOverEncumbered() || fCheckPerk != 0.0) {
        NiPoint3 lastcoord;
        static int32_t lastFastTravelSaveTime = appTimer.iLastTime;
        uint32_t uiTimeSinceSave = appTimer.iLastTime - lastFastTravelSaveTime;
        if (bSaveOnTravel.Bool() && (!uiTimeSinceSave || uiTimeSinceSave > 0x4E20)) {
            pSaveLoadManager->Autosave();
            lastFastTravelSaveTime = appTimer.iLastTime;
        }
        pPlayer->btransporting = true;
        MemoryLevelManager::BeginMemoryFree(&freedata, true, true, true);
        TES::pInstance->ClearAllCells(true, false);
        MemoryLevelManager::FreeReleasedObjects(freedata.bLockedFaceGen);
        MemoryLevelManager::EndMemoryFree(freedata);
        static_cast<PlayerMover*>(pActorMover)->PlayerMover::ClearMoveMode(2048u);
        GetCharController()->SetWantState(HK_CHARACTER_ON_GROUND);
        GetOutofFurnitureQuick();
        ftotaldist = 0.0f;
        lastcoord.x = pPlayer->GetLocationOnReference().x;
        lastcoord.y = pPlayer->GetLocationOnReference().y;
        lastcoord.z = pPlayer->GetLocationOnReference().z;
        ActorLockData playerLockData(pPlayer);
        {
            PathingLocation kTargetLocation(apRef);
            PathingLocation kPlayerLocation(pPlayer);
            ftotaldist = Pathing::ComputeTeleportDoorPathLength(
                kPlayerLocation,
                kTargetLocation,
                0.0f,
                &playerLockData,
                Pathing::DOOR_LOCK_DISPOSITION_NORMAL
            );
        }
        if (ftotaldist == 3.4028234663852886e+38) {
            char cMessage[1024];
            BSsprintf(cMessage, 1024, "FastTravel: Could not compute path length to Cell ");
            if (apRef->GetParentCell()) {
                BSStringT<char> kCellString;
                apRef->GetParentCell()->GetFormDetailedString(kCellString);
                BSstrcat(cMessage, 1024, kCellString.pString);
            }
            ftotaldist = 0.0f;
            Warning(cMessage);
        }
        TESGlobal* ptimescale = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(58u));
        double dRunSpeed = pPlayer->GetRunSpeed();
        float ftime = ftotaldist / dRunSpeed;
        ftime = ftime / 3600.0;
        double dTimeScale = ptimescale->fValue;
        ftime = dTimeScale * ftime;
        this->iSleepTime = static_cast<int32_t>(ftime);
        bIsSleeping = false;
        TES::pInstance->ShowLoadingMenu(true, apRef, false);
        if (this->iSleepTime) {
            while (this->iSleepTime > 0) {
                ftime = 3600.0 / appCalendar.GetTimeScale();
                double dClock = ReferenceProcessLists.GetSystemTimeClock();
                float fNewClock = dClock + ftime;
                ReferenceProcessLists.SetSystemTimeClock(fNewClock);
                appCalendar.Update(ftime);
                ReferenceProcessLists.kUpdateLock.Lock(nullptr);
                ReferenceProcessLists.UpdateHighListPackages(3600.0f, true);
                ReferenceProcessLists.UpdateActorsMovement(3600.0f, true, PROCESS_TYPE_HIGH);
                ReferenceProcessLists.UpdateMiddleHighList(3600.0f, true);
                ReferenceProcessLists.UpdateMiddleLowList(3600.0f, true);
                ReferenceProcessLists.UpdateLowList(3600.0f, true);
                ReferenceProcessLists.ChangeProcessLevelTempList();
                ReferenceProcessLists.kUpdateLock.Unlock();
                --this->iSleepTime;
                pPlayer->RestoreHealth(2.0f);
            }
        }
        else {
            ftime = ftime * 3600.0 / appCalendar.GetTimeScale();
            double dClock = ReferenceProcessLists.GetSystemTimeClock();
            float fNewClock = dClock + ftime;
            ReferenceProcessLists.SetSystemTimeClock(fNewClock);
            appCalendar.Update(ftime);
        }
        ReferenceProcessLists.MoveActorsToReference(apRef);
        if (TES::GetSelectivePurgeUnusedOnFastTravel()) {
            TES::pInstance->SetAllowUnusedPurge(false);
        }
        if (BSFaceGenManager::GetModelCache()) {
            BSFaceGenManager::GetModelCache()->UnloadAllEGMAndEGTData(false);
        }
        MemoryPool::FreeUnusedPagesForAllPools();
        Sky::GetInstance()->SetFastTravel(true);
        Interface::bIsFastTraveling = true;
        TESWorldSpace* pWorldSpace = apRef->GetWorldSpace();
        NiPoint3 kAngle = apRef->GetAngleOnReference();
        NiPoint3 kLocation = apRef->GetLocationOnReference();
        pPlayer->PositionPlayerExterior(kLocation, kAngle, pWorldSpace, true);
        Interface::bIsFastTraveling = false;
        this->iSleepTime = 0;
        Sky::GetInstance()->SetFastTravel(false);
        if (TES::GetSelectivePurgeUnusedOnFastTravel()) {
            TES::pInstance->SetAllowUnusedPurge(true);
        }
        ReferenceProcessLists.PrintActorsInDialog();
        TES::pInstance->CreateFurnitureList();
        ReferenceProcessLists.PlaceActorsinBedsOrChairs();
        ReferenceProcessLists.ProcessHighActorsForFastTravel();
        pPlayer->btransporting = false;
        ReferenceProcessLists.Update3DAfterResting();
        CheckTempModifiers();
        ResetMagicCastSound();
        pPlayer->bTravelUseDoor = false;
    }
}

void PlayerCharacter::ClearCellsIfTargetNotLoaded(TESObjectCELL* apTarget, TESWorldSpace* apWorldSpace) {
    MemoryLevelManager::MEM_FREE_DATA freedata;
    if (!apTarget || !TES::pInstance->IsCellLoaded(apTarget, false)) {
        TESForm* plocation;
        if (apTarget) {
            plocation = apTarget;
        }
        else {
            plocation = apWorldSpace;
        }
        TES::pInstance->ShowLoadingMenu(true, plocation, false);
        if (pPlayer->GetParentCell()) {
            pPlayer->GetParentCell()->RemoveReference(pPlayer);
        }
        NiAVObject* p3D = Get3D(true);
        NiNode* pnode = p3D ? p3D->IsNode() : nullptr;
        if (pnode && pnode->m_pkParent) {
            pnode->m_pkParent->DetachChild(p3D);
        }

        MemoryLevelManager::BeginMemoryFree(&freedata, false, true, false);
        TES::pInstance->ClearAllCells(false, true);
        ProcessLists::bIsForciblyUpdated = true;
        ReferenceProcessLists.UpdateProcessLists();
        ReferenceProcessLists.ChangeProcessLevelTempList();
        ProcessLists::bIsForciblyUpdated = false;
        MemoryLevelManager::FreeReleasedObjects(freedata.bLockedFaceGen);
        MemoryLevelManager::EndMemoryFree(freedata);
        if (BSFaceGenManager::GetModelCache()) {
            BSFaceGenManager::GetModelCache()->UnloadAllEGMAndEGTData(false);
        }
        MemoryPool::FreeUnusedPagesForAllPools();
    }
}

// In first person with no first-person camera (in dialogue), currentPos is never written and is read uninitialised.
XFNV_UNWRITTEN_READS_BEGIN
void PlayerCharacter::UpdateCamera(bool abForce, bool abSkipUpdateLOD) {
    NiPoint3 cameraPos;
    NiMatrix3 rotMod;
    NiPoint3 cameraRow;
    NiPoint3 currentPos;
    NiMatrix3 cameraRot;
    PlayerCharacter::fCameraHeading = GetHeading(false);
    PlayerCharacter::fCameraLooking = GetLooking();
    SceneGraph* pWorldRoot = Main::GetWorldSceneGraph();
    fCameraEyeHeightMult = 1.0f;
    static NiCamera chaseCamera;
    float fWorldFOV = this->fWorldFOV;
    Main::GetWorldSceneGraph()->SetCameraFOV(fWorldFOV, false, nullptr, false);
    BSShaderManager::SetFOV(this->fWorldFOV);
    if (pVATS.eMode == VATS::VATS_PLAYBACK && bWant3rdPerson) {
        NiMatrix3 vatsCameraRot;
        currentPos = chaseCameraPos;
        chaseCamera.SetTranslate(currentPos);
        NiUpdateData UpdateData(0.0f, false, false);
        chaseCamera.Update(UpdateData);
        chaseCamera.LookAtWorldPoint(pVATS.GetCamshotTargetPos(), NiPoint3::UNIT_Z);
        rotMod = chaseCamera.m_kLocal.m_Rotate;
        rotMod.GetRow(0, cameraRow);
        cameraRot.SetRow(0, cameraRow.z, cameraRow.x, cameraRow.y);
        rotMod.GetRow(1u, cameraRow);
        cameraRot.SetRow(1u, cameraRow.z, cameraRow.x, cameraRow.y);
        rotMod.GetRow(2u, cameraRow);
        cameraRot.SetRow(2u, cameraRow.z, cameraRow.x, cameraRow.y);
        vatsCameraRot.FromEulerAnglesXYZ(kVATSCameraAngle.x, kVATSCameraAngle.y, kVATSCameraAngle.z);
        cameraRot = cameraRot * vatsCameraRot;
    }
    else if (b3rdPerson || PlayerCharacter::bVanityMode || bWant3rdPerson != b3rdPerson) {
        NiPoint3 targetPos = Get3D(false)->GetWorldTranslate();
        float fYDir = 1000.0f;
        if (PlayerCharacter::bIsAutoVanityMode) {
            if (PlayerCharacter::kAutoVanityModePos.z < 0.0) {
                PlayerCharacter::kAutoVanityModePos.z = PlayerCharacter::kAutoVanityModePos.z + 6.2831854820251465;
            }
            else if (PlayerCharacter::kAutoVanityModePos.z > 6.2831854820251465) {
                PlayerCharacter::kAutoVanityModePos.z = PlayerCharacter::kAutoVanityModePos.z - 6.2831854820251465;
            }
            cameraRot.MakeZRotation(GetHeading(false) + PlayerCharacter::kAutoVanityModePos.z);
            rotMod.MakeXRotation(PlayerCharacter::kAutoVanityModePos.x);
            targetPos.z = GetScale() * 100.0 + targetPos.z;
        }
        else {
            float f3rdPersonFOV = this->f3rdPersonFOV;
            Main::GetWorldSceneGraph()->SetCameraFOV(f3rdPersonFOV, false, nullptr, false);
            BSShaderManager::SetFOV(this->f3rdPersonFOV);
            cameraRot.MakeZRotation(PlayerCharacter::fCameraHeading + PlayerCharacter::kVanityModePos.z);
            rotMod.MakeXRotation(PlayerCharacter::fCameraLooking + PlayerCharacter::kVanityModePos.x);
            double dEyeHeight = this->GetEyeHeight() * fCameraEyeHeightMult;
            targetPos.z = GetScale() * dEyeHeight + targetPos.z;
            const NiPoint3& k1stPersonCameraPos = p1stPersonCamera->GetWorldTranslate();
            float feyeDelta = k1stPersonCameraPos.z - p3rdPersonCamera->GetWorldTranslate().z;
            targetPos.z = targetPos.z + feyeDelta;
        }
        float fshoulderMult = 1.0f;
        if (fOverShoulderStartBlendDist.Float() > 0.0) {
            fshoulderMult = 1.0 - PlayerCharacter::kVanityModePos.y / fOverShoulderStartBlendDist.Float();
            if (fshoulderMult < 0.0) {
                fshoulderMult = 0.0f;
            }
        }
        float fShoulderPosZ = fOverShoulderPosZ.Float();
        NiPoint3 shoulderOffset = NiPoint3(fOverShoulderPosX.Float(), 0.0, fShoulderPosZ) * fshoulderMult;
        float fLookAtZ = fOverShoulderPosZ.Float();
        float fLookAtY = fYDir - PlayerCharacter::kVanityModePos.y;
        NiPoint3 shoulderLookAt(fOverShoulderPosX.Float(), fLookAtY, fLookAtZ);
        char czoomingMode = bWant3rdPerson - b3rdPerson;
        cameraRot = cameraRot * rotMod;
        cameraPos = targetPos + cameraRot * (NiPoint3(0.0, -PlayerCharacter::kVanityModePos.y, 0.0) + shoulderOffset);
        NiPoint3 cameraTarget = targetPos + cameraRot * shoulderLookAt;
        Setup3rdPersonCameraCaster();
        targetPos += cameraRot * shoulderOffset;
        NiPoint3 newTargetPos(data.kPosition.x, data.kPosition.y, targetPos.z);
        UpdateChaseCamera(&cameraPos, &newTargetPos, abForce);
        targetPos -= cameraRot * shoulderOffset;
        currentPos = cameraPos;
        shoulderLookAt = targetPos + shoulderLookAt;
        kCamera3rdPersonShoulderOffset = cameraRot * shoulderOffset;
        if (fshoulderMult > 0.0) {
            NiMatrix3 shoulderRot;
            shoulderRot.MakeZRotation(PlayerCharacter::fCameraHeading);
            if (PlayerCharacter::fVanityDelay > 0.0) {
                float fmult = PlayerCharacter::fVanityDelay / fVanityModeDelay.Float();
                if (fmult > 1.0) {
                    fmult = 1.0f;
                }
                float fOffsetZ = fOverShoulderOffsetPointZ.Float();
                float fOffsetY = fOverShoulderOffsetPointY.Float();
                shoulderOffset = NiPoint3(fOverShoulderOffsetPointX.Float(), fOffsetY, fOffsetZ) * fmult;
                float fZoomingMult = 1.0 - fmult;
                float fZoomingZ = fOverShoulderOffsetPointZoomingZ.Float();
                float fZoomingY = fOverShoulderOffsetPointZoomingY.Float();
                shoulderOffset += NiPoint3(fOverShoulderOffsetPointZoomingX.Float(), fZoomingY, fZoomingZ) *
                    fZoomingMult;
            }
            else if (czoomingMode || (bCameraCasterHitObject && !PlayerCharacter::bVanityMode)) {
                float fZoomingZ = fOverShoulderOffsetPointZoomingZ.Float();
                float fZoomingY = fOverShoulderOffsetPointZoomingY.Float();
                shoulderOffset = NiPoint3(fOverShoulderOffsetPointZoomingX.Float(), fZoomingY, fZoomingZ);
            }
            else {
                float fOffsetZ = fOverShoulderOffsetPointZ.Float();
                float fOffsetY = fOverShoulderOffsetPointY.Float();
                shoulderOffset = NiPoint3(fOverShoulderOffsetPointX.Float(), fOffsetY, fOffsetZ);
            }
            rotMod.MakeXRotation(PlayerCharacter::fCameraLooking);
            shoulderOffset = rotMod * shoulderOffset;
            shoulderOffset = shoulderRot * shoulderOffset;
            float fTranslateMult = 1.0 - fOverShoulderRotMult.Float();
            targetPos += shoulderOffset * fTranslateMult;
        }
        chaseCamera.SetTranslate(currentPos);
        NiUpdateData UpdateData(0.0f, false, false);
        chaseCamera.Update(UpdateData);
        if (PlayerCharacter::bIsAutoVanityMode) {
            chaseCamera.LookAtWorldPoint(targetPos, NiPoint3::UNIT_Z);
        }
        else {
            chaseCamera.LookAtWorldPoint(cameraTarget, NiPoint3::UNIT_Z);
        }
        rotMod = chaseCamera.m_kLocal.m_Rotate;
        rotMod.GetRow(0, cameraRow);
        cameraRot.SetRow(0, cameraRow.z, cameraRow.x, cameraRow.y);
        rotMod.GetRow(1u, cameraRow);
        cameraRot.SetRow(1u, cameraRow.z, cameraRow.x, cameraRow.y);
        rotMod.GetRow(2u, cameraRow);
        cameraRot.SetRow(2u, cameraRow.z, cameraRow.x, cameraRow.y);
    }
    else {
        cameraRot.MakeZRotation(PlayerCharacter::fCameraHeading);
        rotMod.MakeXRotation(PlayerCharacter::fCameraLooking);
        cameraRot = cameraRot * rotMod;
        if (!IsInDialoguewithPlayer()) {
            NiMatrix3 xrot;
            xrot.MakeXRotation(-f1stPersonArmRot);
            rotMod = p1stPersonCamera->GetWorldTransform().m_Rotate;
            cameraRot = rotMod * xrot;
        }
        if (p1stPersonCamera) {
            currentPos = p1stPersonCamera->GetWorldTranslate();
        }
        HighProcess* phighProcess = static_cast<HighProcess*>(pCurrentProcess);
        NiNode* pHeadBone = GetBiped(false)->GetParentBone(BIPED_BONE_HEAD);
        float fblendValue = phighProcess->GetAnimation()->GetSmallestHavokBlendValue(pHeadBone);
        if (fblendValue < 1.0) {
            NiPoint3 posDelta = p3rdPersonCamera->GetWorldTranslate() - currentPos;
            posDelta *= fFirstPersonCameraMult.Float() * fblendValue;
            currentPos += posDelta;
            if (p1stPersonBip) {
                p1stPersonBip->SetTranslate(
                    p1stPersonBip->GetTranslate() + posDelta * sp1stPerson3D->m_kLocal.m_Rotate
                );
                NiUpdateData UpdateData(0.0f, false, false);
                p1stPersonBip->Update(UpdateData);
            }
        }
        if (pCameraCaster && pCameraCaster->GetWorld() && pVATS.eMode != VATS::VATS_PLAYBACK) {
            pCameraCaster->Remove();
        }
    }
    if (pVATS.eMode != VATS::VATS_PLAYBACK || (pVATS.pCamera && pVATS.pCamera->IsInitialized())) {
        NiMatrix3 cameraShake;
        float fmult = GetCameraShake(Actor::GetAnimation()->GetTime(), cameraShake);
        if (fmult > 0.0) {
            float fy;
            float fz;
            float fx;
            cameraShake.ToEulerAnglesXYZ(fx, fy, fz);
            cameraShake.FromEulerAnglesXYZ(fx * fmult, fy * fmult, fz * fmult);
            cameraRot = cameraRot * cameraShake;
            HUDMainMenu::SetHUDShake(fmult);
        }
        pWorldRoot->GetCameraRoot()->SetTranslate(currentPos);
        pWorldRoot->GetCameraRoot()->SetRotate(cameraRot);
        NiUpdateData UpdateData(0.0f, false, false);
        pWorldRoot->GetCameraRoot()->Update(UpdateData);
    }
    BSAudio::QInstance().PositionListener(currentPos);
    NiPoint3 kUpVector = pWorldRoot->GetCamera()->GetWorldUpVector();
    NiPoint3 kDirection = pWorldRoot->GetCamera()->GetWorldDirection();
    static_cast<BSWin32Audio*>(&BSAudio::QInstance())->SetOrientation(kDirection, kUpVector);
    TESObjectCELL* pcell = GetParentCell();
    if (pcell) {
        bool bUnderwater = IsPointDeepUnderWater(currentPos.z, pcell) >= 0.03999999910593033;
        BSAudio::QInstance().SetListenerUnderwater(bUnderwater);
    }
    static NiPoint3 kLastGrassUpdatePos = NiPoint3::ZERO;
    static NiPoint3 kLastGrassUpdateDir = NiPoint3::ZERO;
    static NiPoint3 kLastLODUpdatePos = NiPoint3::ZERO;
    float fmovedist = std::fabsf((NiPoint3(currentPos.x, currentPos.y, 0.0f) - kLastGrassUpdatePos).Length());
    float fmovedistLOD = std::fabsf((NiPoint3(currentPos.x, currentPos.y, 0.0f) - kLastLODUpdatePos).Length());
    NiPoint3 kcurDir = NiPoint3::ZERO;
    NiCamera* pworldcam = pWorldRoot->GetCamera();
    double dHalfAngle = std::atanf(pworldcam->GetViewFrustum().m_fRight);
    float fov = dHalfAngle / pworldcam->m_fLODAdjust;
    float theta = 1.0 - fov / 8.0;
    kcurDir = pworldcam->GetWorldDirection();
    kcurDir.Unitize();
    float fviewdot = kLastGrassUpdateDir.Dot(kcurDir);
    if (fmovedist > 64.0 || theta > static_cast<double>(fviewdot) || abForce) {
        TESTerrainLODManager::UpdateGrass(currentPos, kcurDir, std::acos(theta));
        kLastGrassUpdatePos = NiPoint3(currentPos.x, currentPos.y, 0.0f);
        kLastGrassUpdateDir = kcurDir;
    }
    if (!abSkipUpdateLOD && GetWorldSpace()) {
        if (GetWorldSpace()->GetTerrainManager()->bNeedsImmediateUpdate) {
            const NiPoint3& kLocation = GetLocationOnReference();
            GetWorldSpace()->GetTerrainManager()->Update(kLocation, BGSTerrainManager::UPDATE_ALL);
        }
        else if (fmovedistLOD > 512.0 || abForce) {
            const NiPoint3& kLocation = GetLocationOnReference();
            GetWorldSpace()->GetTerrainManager()->Update(kLocation, BGSTerrainManager::UPDATE_ALL);
            kLastLODUpdatePos = NiPoint3(currentPos.x, currentPos.y, 0.0f);
        }
        const NiPoint3& kLocation = GetLocationOnReference();
        GetWorldSpace()->GetTerrainManager()->UpdateMorphParameters(kLocation);
    }

    if (p1stPersonCamera) {
        Cached1stPersonCameraPos = p1stPersonCamera->GetWorldTranslate();
    }
    NiQuaternion quatid = NiQuaternion::IDENTITY;
    CachedWorldCameraPos = pWorldRoot->GetCamera()->GetWorldTranslate();
    spCameraRigidBody->ApplyHardKeyframe(CachedWorldCameraPos, quatid, nullptr);
    UpdateFirstPersonZoom();
}
XFNV_UNWRITTEN_READS_END

// Turns the player's heading and looking from the mouse or the controller, or, in VATS, moves the camera toward the
// target (playback: aims at the current action's target limb). Returns whether the view was moved.
// KEEP: the exe reads an uninitialised local. In the VATS menu's limb test it reads screenPos.y after
// NiCamera::WorldPtToScreenPt, which leaves it unwritten when the limb is behind the camera (w at or below the zero
// tolerance, tested before the first store).
bool PlayerCharacter::UpdateHeadingAndLooking(float afDelta, bool abHorse, uint16_t* apMoveMode, bool abScanFinished) {
    bool bretVal = false;
    TESObjectREFR* ptarget = nullptr;
    ActorValue::Index etargetLimb = ActorValue::VALUE_NONE;
    switch (pVATS.eMode) {
        case VATS::VATS_MODE_NONE: {
            if (!GetUnderAIControl()) {
                Controls* pcontrols = appMain->GetControls();
                pprevVatsTarget = nullptr;
                fVATSUserCameraZOffset = 0.0f;
                int32_t imouseX = 0;
                int32_t imouseY = 0;
                int32_t imouseZ = 0;
                bool b3rdPersonView = Is3rdPerson();
                Animation* panimation = GetAnimation(!b3rdPersonView);
                // Scales the argument in place.
                afDelta = panimation->GetGlobalTimeMultiplier() * afDelta;
                InterfaceManager::Instance();
                if (InterfaceManager::Has360Controller()) {
                    // 0x21F1 is the controller's thumbstick dead zone.
                    if (PlayerCharacter::bZoom3rdPersonCamera) {
                        int32_t itrash = pcontrols->GetControllerValue(0, 10);
                        if (std::abs(itrash) < 0x21F1) {
                            itrash = 0;
                        }
                        imouseZ = static_cast<int32_t>(static_cast<double>(itrash) * 0.10000000149011612 * afDelta);
                        imouseX = pcontrols->GetControllerValue(0, 9);
                        if (std::abs(imouseX) < 0x21F1) {
                            imouseX = 0;
                        }
                        imouseX /= 5;
                        if (imouseX || imouseZ) {
                            int32_t iabsTrash = std::abs(itrash);
                            if (iabsTrash <= std::abs(imouseX)) {
                                imouseZ = 0;
                            }
                            else {
                                imouseX = 0;
                            }
                        }
                    }
                    else {
                        imouseX = pcontrols->GetControllerValue(0, 9);
                        imouseY = pcontrols->GetControllerValue(0, 10);
                        if (std::abs(imouseX) < 0x21F1) {
                            imouseX = 0;
                        }
                        if (std::abs(imouseY) < 0x21F1) {
                            imouseY = 0;
                        }
                    }
                    if (imouseX || imouseY) {
                        bool bNegX = false;
                        if (imouseX < 0) {
                            imouseX = -imouseX;
                            bNegX = true;
                        }
                        bool bNegY = true;
                        if (imouseY < 0) {
                            imouseY = -imouseY;
                            bNegY = false;
                        }
                        NiPoint3 LookVec;
                        LookVec.x = static_cast<double>(imouseX) / 32767.0;
                        LookVec.y = static_cast<double>(imouseY) / 32767.0;
                        LookVec.z = 0.0f;
                        float fMagnitude = LookVec.Length();
                        if (fMagnitude > 1.0) {
                            fMagnitude = 1.0f;
                        }
                        float fRatioX = LookVec.x / fMagnitude;
                        float fRatioY = LookVec.y / fMagnitude;
                        float fNewHorizMagnitude = 1.0f;
                        float fNewVertMagnitude = 1.0f;
                        for (int32_t ipower = 0; ipower < iXenonHorizLookAccel.Int(); ++ipower) {
                            fNewHorizMagnitude = fNewHorizMagnitude * fMagnitude;
                        }
                        for (int32_t ipower = 0; ipower < iXenonVertLookAccel.Int(); ++ipower) {
                            fNewVertMagnitude = fNewVertMagnitude * fMagnitude;
                        }
                        float fminLookSpeed = fXenonMinLookSpeed.Float();
                        float fhorizLookSpeed = fXenonHorizLookSpeed.Float();
                        float fvertLookSpeed = fXenonVertLookSpeed.Float();
                        if (fminLookSpeed > static_cast<double>(fNewHorizMagnitude)) {
                            fNewHorizMagnitude = fminLookSpeed;
                        }
                        if (fminLookSpeed > static_cast<double>(fNewVertMagnitude)) {
                            fNewVertMagnitude = fminLookSpeed;
                        }
                        imouseX = static_cast<int32_t>(fNewHorizMagnitude * fRatioX * fhorizLookSpeed * afDelta);
                        imouseY = static_cast<int32_t>(fNewVertMagnitude * fRatioY * fvertLookSpeed * afDelta);
                        if (bNegX) {
                            imouseX = -imouseX;
                        }
                        if (bNegY) {
                            imouseY = -imouseY;
                        }
                    }
                }
                else {
                    imouseX = pcontrols->GetMouseAxis(1);
                    imouseY = pcontrols->GetMouseAxis(2);
                    imouseZ = pcontrols->GetMouseAxis(3);
                }
                if (bInvertYValues.Bool()) {
                    imouseY = -imouseY;
                }
                if (GetControlsDisabled(PlayerCharacter::DPC_LOOKING)) {
                    imouseX = 0;
                    imouseY = 0;
                    imouseZ = 0;
                }
                if (static_cast<double>(imouseZ) != 0.0 && !IsPipboyActive()) {
                    if (!PlayerCharacter::bVanityMode) {
                        bretVal = true;
                    }
                    if (PlayerCharacter::bVanityMode || b3rdPerson) {
                        if (imouseZ <= 0) {
                            double dzoom = static_cast<double>(imouseZ) / 120.0 * PlayerCharacter::kVanityModePos.y;
                            PlayerCharacter::kVanityModePos.y = PlayerCharacter::kVanityModePos.y -
                                fVanityModeWheelOutMult.Float() * dzoom;
                        }
                        else {
                            double dzoom = static_cast<double>(imouseZ) / 120.0 * PlayerCharacter::kVanityModePos.y;
                            PlayerCharacter::kVanityModePos.y = PlayerCharacter::kVanityModePos.y -
                                fVanityModeWheelInMult.Float() * dzoom;
                        }
                        if (IsDead(false)) {
                            if (fVanityModeWheelDeadMin.Float() >
                                static_cast<double>(PlayerCharacter::kVanityModePos.y)) {
                                PlayerCharacter::kVanityModePos.y = fVanityModeWheelDeadMin.Float();
                            }
                        }
                        else if (fVanityModeWheelMin.Float() > static_cast<double>(PlayerCharacter::kVanityModePos.y)) {
                            if (!GetUnderAIControl() &&
                                !PlayerCharacter::bVanityMode &&
                                !bTemp3rdPerson &&
                                !GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
                                SetFirstPerson(true);
                            }
                            PlayerCharacter::kVanityModePos.y = fVanityModeWheelMin.Float();
                        }
                        float fMaxChaseDistance = fChaseCameraMax.Float();
                        if (PlayerCharacter::bVanityMode) {
                            fMaxChaseDistance = fVanityModeWheelMax.Float();
                        }
                        if (fMaxChaseDistance < static_cast<double>(PlayerCharacter::kVanityModePos.y)) {
                            PlayerCharacter::kVanityModePos.y = fMaxChaseDistance;
                        }
                    }
                    if (!GetUnderAIControl() &&
                        !bTemp1stPerson &&
                        !b3rdPerson &&
                        imouseZ < 0 &&
                        !GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
                        SetFirstPerson(false);
                    }
                }
                float fdefaultFOV = BSSceneGraph::fDefaultWorldFOV.Float();
                float fcurrentFOV = Main::GetWorldSceneGraph()->GetCameraFOV();
                float ffovModMult = 1.0f;
                if (fdefaultFOV > static_cast<double>(fcurrentFOV)) {
                    ffovModMult = fcurrentFOV / fdefaultFOV;
                }
                PlayerCharacter::fCameraHeading = GetHeading(false);
                PlayerCharacter::fCameraLooking = GetLooking();
                if ((PlayerCharacter::bVanityMode || PlayerCharacter::bZoom3rdPersonCamera) &&
                    pCurrentProcess->GetFadeState() == BaseProcess::FADE_NORMAL) {
                    double dangleX = static_cast<double>(imouseX) * 0.01745329238474369;
                    PlayerCharacter::kVanityModePos.z = fVanityModeXMult.Float() * dangleX * afDelta +
                        PlayerCharacter::kVanityModePos.z;
                    if (!InterfaceManager::Has360Controller()) {
                        double dangleY = static_cast<double>(imouseY) * 0.01745329238474369;
                        PlayerCharacter::kVanityModePos.x = fVanityModeYMult.Float() * dangleY * afDelta +
                            PlayerCharacter::kVanityModePos.x;
                    }
                }
                else {
                    if (imouseX || abHorse) {
                        double dmouseX = static_cast<double>(imouseX);
                        float fscaledMouseX = fMouseSensitivity.Float() * dmouseX;
                        if (apMoveMode) {
                            if (imouseX && fscaledMouseX < 0.0) {
                                *apMoveMode |= ActorMover::MF_TURNLEFT;
                            }
                            else if (imouseX) {
                                *apMoveMode |= ActorMover::MF_TURNRIGHT;
                            }
                        }
                        if (!GetLifeState() && GetAnimAction() != ANIM_ACTION_STAGGER && !GetUnderAIControl()) {
                            SIT_SLEEP_STATE esitState = GetSitSleepState();
                            if (esitState == SIT_SLEEP_IS_SITTING) {
                                fSitHeadingDelta = fSitHeadingDelta + fscaledMouseX;
                                if (fSitHeadingDelta < -1.5707963705062866) {
                                    fSitHeadingDelta = -1.5707964f;
                                }
                                else if (fSitHeadingDelta > 1.5707963705062866) {
                                    fSitHeadingDelta = 1.5707964f;
                                }
                                if (abHorse) {
                                    PlayerCharacter::fPCTurnScale = 1.5f;
                                }
                            }
                            else if (esitState == SIT_SLEEP_NORMAL) {
                                double dabsMouseX = std::fabsf(fscaledMouseX);
                                PlayerCharacter::fPCTurnScale = dabsMouseX / (fMouseSensitivity.Float() * 24.0);
                                if (PlayerCharacter::fPCTurnScale > 1.5) {
                                    PlayerCharacter::fPCTurnScale = 1.5f;
                                }
                                float fheadingMod = fscaledMouseX * ffovModMult;
                                ModHeading(fheadingMod);
                            }
                        }
                    }
                    if (imouseY) {
                        double dmouseY = static_cast<double>(imouseY);
                        float flookingMod = fMouseSensitivity.Float() * dmouseY * ffovModMult;
                        ModLooking(flookingMod);
                    }
                }
                if (imouseX || imouseY || imouseZ) {
                    bretVal = true;
                }
            }
            break;
        }
        case VATS::VATS_TARGET_SELECT:
        case VATS::VATS_MENU_MODE:
        case VATS::VATS_SCAN_MODE: {
            ptarget = pVATSTarget;
            etargetLimb = eVATSTargetLimb;
            DoIronSightsZoom(afDelta);
            if (ptarget) {
                NiAVObject* ptarget3d = ptarget->Get3D();
                NiPoint3 targetPos;
                bool bresetInterpolators = false;
                float fcenteredCameraOffset = 0.0f;
                bool blimbSelect = true;
                if (pVATSTarget != pprevVatsTarget) {
                    pprevVatsTarget = pVATSTarget;
                    bresetInterpolators = true;
                    blimbSelect = false;
                    if (ptarget->IsActor()) {
                        fVATSUserCameraZOffset = 0.0f;
                        fcenteredCameraOffset = fVATSUserCameraZOffset;
                    }
                    else {
                        fVATSUserCameraZOffset = 0.0f;
                    }
                }
                if (ptarget3d) {
                    Actor* pactor = static_cast<Actor*>(ptarget);
                    targetPos = pactor->Get3D()->GetWorldTranslate();
                    if (ptarget->IsActor()) {
                        static NiPoint3 center = NiPoint3::ZERO;
                        // A new target: the camera centres on the target's head when it is within idistance of the
                        // player, else on the mean of its limbs.
                        if (bresetInterpolators) {
                            BGSBodyPartData* pbodyPartData = pactor->GetActorBase()->GetBodyPartData();
                            BGSBodyPart** pbodyPartArray = pbodyPartData->pPartArray;
                            int32_t ilimbCount = 0;
                            NiPoint3 totalLoc = NiPoint3::ZERO;
                            NiAVObject* plimb = nullptr;
                            if (pbodyPartArray[BGSBodyPart::LIMB_HEAD_1]) {
                                const char* pname = pbodyPartData->GetBodyPart(BGSBodyPart::LIMB_HEAD_1)
                                                        ->GetTargetName();
                                plimb = BSUtilities::GetObjectByName(pactor->Get3D(), pname);
                            }
                            if (plimb &&
                                static_cast<double>(idistance) >
                                    (plimb->GetWorldTranslate() - pPlayer->Get3D()->GetWorldTranslate()).Length()) {
                                ++ilimbCount;
                                NiPoint3 limbLoc = plimb->GetWorldTranslate();
                                totalLoc += limbLoc;
                            }
                            else {
                                for (int32_t eindex = 0; eindex < 15; ++eindex) {
                                    if (pbodyPartArray[eindex]) {
                                        const char* pname = pbodyPartData
                                                                ->GetBodyPart(
                                                                    static_cast<BGSBodyPart::LIMB_ENUM>(eindex)
                                                                )
                                                                ->GetTargetName();
                                        NiAVObject* ppartLimb = BSUtilities::GetObjectByName(pactor->Get3D(), pname);
                                        if (ppartLimb) {
                                            ++ilimbCount;
                                            NiPoint3 limbLoc = ppartLimb->GetWorldTranslate();
                                            totalLoc += limbLoc;
                                        }
                                    }
                                }
                            }
                            if (ilimbCount) {
                                center = totalLoc / static_cast<float>(ilimbCount);
                            }
                        }
                        targetPos = center;
                        // In the VATS menu with the mouse, a cursor near the top or bottom of the screen moves the
                        // camera to the target's highest or lowest limb.
                        bool bUseMouseForCameraOffset = false;
                        InterfaceManager* pinterfaceManager = InterfaceManager::Instance();
                        if (pVATS.eMode == VATS::VATS_MENU_MODE &&
                            pinterfaceManager->GetMouseVisible() &&
                            pHeadingInterpolator->GetFinished() &&
                            pLookingInterpolator->GetFinished()) {
                            int32_t icursorY = pinterfaceManager->GetCursorY();
                            if (icursorY >= 0 && InterfaceManager::GetScreenHeight() >= static_cast<double>(icursorY)) {
                                float fmoveCameraPercent = fVATSMoveCameraYPercent.Float();
                                float fcursorPercent = static_cast<double>(icursorY) /
                                    InterfaceManager::GetScreenHeight();
                                bUseMouseForCameraOffset = true;
                                float fmaxZ = 0.0f;
                                float fminZ = 0.0f;
                                BGSBodyPartData*
                                    pbodyPartData = static_cast<Actor*>(ptarget)->GetActorBase()->GetBodyPartData();
                                BGSBodyPart** pbodyPartArray = pbodyPartData->pPartArray;
                                for (int32_t eindex = 0; eindex < 15; ++eindex) {
                                    if (pbodyPartArray[eindex]) {
                                        NiPoint2 screenPos;
                                        const char* pname = pbodyPartData
                                                                ->GetBodyPart(
                                                                    static_cast<BGSBodyPart::LIMB_ENUM>(eindex)
                                                                )
                                                                ->GetTargetName();
                                        NiAVObject* plimb = BSUtilities::GetObjectByName(ptarget->Get3D(), pname);
                                        if (plimb) {
                                            Main::GetWorldSceneGraph()->GetCamera()->WorldPtToScreenPt(
                                                plimb->GetWorldTranslate(),
                                                screenPos.x,
                                                screenPos.y,
                                                1e-05f
                                            );
                                            float flimbZ = plimb->GetWorldTranslate().z;
                                            if (fmaxZ == 0.0 || fmaxZ < flimbZ) {
                                                fmaxZ = flimbZ;
                                            }
                                            if (fminZ == 0.0 || fminZ > flimbZ) {
                                                fminZ = flimbZ;
                                            }
                                        }
                                    }
                                }
                                NiPoint3 rot = CombatUtilities::GetAngleToProjectedPoint(
                                    Main::GetWorldSceneGraph()->GetCamera()->GetWorldTranslate(),
                                    targetPos
                                );
                                if (fmoveCameraPercent > fcursorPercent) {
                                    fVATSUserCameraZOffset = fmaxZ - targetPos.z;
                                }
                                else if (fcursorPercent > 1.0 - fmoveCameraPercent) {
                                    fVATSUserCameraZOffset = fminZ - targetPos.z;
                                }
                                else if (
                                    (rot.x < GetLooking() &&
                                     fcursorPercent < fmoveCameraPercent + fmoveCameraPercent) ||
                                    (rot.x > GetLooking() &&
                                     fcursorPercent > 1.0 - (fmoveCameraPercent + fmoveCameraPercent))
                                ) {
                                    fVATSUserCameraZOffset = fcenteredCameraOffset;
                                }
                                else {
                                    bUseMouseForCameraOffset = false;
                                }
                            }
                        }
                        // Otherwise, once the camera has settled on a selected limb (or the weapon bone), keep that
                        // limb on screen.
                        if (!bUseMouseForCameraOffset &&
                            pVATS.eMode != VATS::VATS_SCAN_MODE &&
                            pVATS.eMode != VATS::VATS_TARGET_SELECT &&
                            bFOVTargetReached &&
                            pHeadingInterpolator->GetFinished() &&
                            pLookingInterpolator->GetFinished()) {
                            BGSBodyPartData* pbodyPartData = pactor->GetActorBase()->GetBodyPartData();
                            if (pbodyPartData) {
                                BGSBodyPart* pbodyPart = pbodyPartData->GetBodyPart(etargetLimb);
                                NiAVObject* plimb = nullptr;
                                NiNode* pweaponBone = nullptr;
                                if (pVATSTarget->IsActor()) {
                                    pweaponBone = static_cast<Actor*>(pVATSTarget)
                                                      ->GetCurrentAIProcess()
                                                      ->GetWeaponBone(pVATSTarget->GetBiped());
                                }
                                if (pbodyPart) {
                                    const char* pname = pbodyPart->GetTargetName();
                                    plimb = BSUtilities::GetObjectByName(pactor->Get3D(), pname);
                                }
                                else {
                                    plimb = pweaponBone;
                                }
                                if (plimb) {
                                    NiPoint3 limbPos = plimb->GetWorldTranslate();
                                    NiPoint2 screenPos;
                                    Main::GetWorldSceneGraph()
                                        ->GetCamera()
                                        ->WorldPtToScreenPt(limbPos, screenPos.x, screenPos.y, 1e-05f);
                                    float fscreenPercent = fVATSMoveCameraLimbPercent.Float();
                                    // screenPos.y is unwritten when the limb is behind the camera (the KEEP reason).
                                    if (screenPos.y < fscreenPercent + fscreenPercent ||
                                        screenPos.y > 1.0 - (fscreenPercent + fscreenPercent)) {
                                        fVATSUserCameraZOffset = limbPos.z - targetPos.z;
                                    }
                                }
                            }
                        }
                        targetPos.z = targetPos.z + fVATSUserCameraZOffset;
                    }
                }
                else {
                    targetPos = ptarget->GetLocationOnReference();
                }
                if (!(center_guard & 2)) {
                    center_guard |= 2;
                    flastCameraZ = targetPos.z;
                }
                if (targetPos.z != flastCameraZ &&
                    pHeadingInterpolator->GetFinished() &&
                    pLookingInterpolator->GetFinished()) {
                    flastCameraZ = targetPos.z;
                    bresetInterpolators = true;
                }
                float fheadingDelta = 0.0f;
                float flookingDelta = 0.0f;
                NiPoint3 rot = CombatUtilities::GetAngleToProjectedPoint(
                    Main::GetWorldSceneGraph()->GetCamera()->GetWorldTranslate(),
                    targetPos
                );
                fheadingDelta = rot.z - GetHeading(false);
                flookingDelta = rot.x - GetLooking();
                while (fheadingDelta > 3.1415927410125732) {
                    fheadingDelta = fheadingDelta - 6.2831854820251465;
                }
                while (fheadingDelta < -3.1415927410125732) {
                    fheadingDelta = fheadingDelta + 6.2831854820251465;
                }
                while (flookingDelta > 3.1415927410125732) {
                    flookingDelta = flookingDelta - 6.2831854820251465;
                }
                while (flookingDelta < -3.1415927410125732) {
                    flookingDelta = flookingDelta + 6.2831854820251465;
                }
                rot.z = GetHeading(false) + fheadingDelta;
                rot.x = GetLooking() + flookingDelta;
                if (bresetInterpolators) {
                    float fcamTime = 0.0f;
                    if (pVATS.eMode == VATS::VATS_TARGET_SELECT) {
                        fcamTime = fVATSTargetSelectCamPanTime.Float();
                    }
                    else if (pVATS.eMode == VATS::VATS_MENU_MODE) {
                        if (blimbSelect) {
                            fcamTime = fVATSLimbSelectCamPanTime.Float();
                        }
                        else {
                            fcamTime = fVATSCamZoomInTime.Float();
                        }
                    }
                    else {
                        fcamTime = 0.75f;
                    }
                    pHeadingInterpolator->Setup(GetHeading(false), rot.z, fcamTime);
                    pLookingInterpolator->Setup(GetLooking(), rot.x, fcamTime);
                }
                pHeadingInterpolator->Interpolate(appTimer.GetSeconds());
                pLookingInterpolator->Interpolate(appTimer.GetSeconds());
                float fheading = pHeadingInterpolator->GetCurrent();
                float flooking = pLookingInterpolator->GetCurrent();
                SetHeading(fheading);
                SetLooking(flooking);
                if (pHeadingInterpolator->GetFinished() && pLookingInterpolator->GetFinished() && bFOVTargetReached) {
                    VATSMenu::SetHeadingReached(true);
                }
                else {
                    VATSMenu::SetHeadingReached(false);
                }
            }
            break;
        }
        case VATS::VATS_PLAYBACK: {
            if (pVATS.GetCurrentAction()) {
                ptarget = pVATS.GetCurrentAction()->pTarget;
                etargetLimb = pVATS.GetCurrentAction()->eLimb;
            }
            if (ptarget) {
                NiAVObject* ptarget3d = ptarget->Get3D();
                NiPoint3 targetPos{};
                bool btargetSet = false;
                if (etargetLimb != ActorValue::VALUE_NONE && ptarget->IsActor()) {
                    Actor* pactor = static_cast<Actor*>(ptarget);
                    BGSBodyPartData* pbodyPartData = pactor->GetActorBase()->GetBodyPartData();
                    if (pbodyPartData) {
                        BGSBodyPart* pbodyPart = pbodyPartData->GetBodyPart(etargetLimb);
                        if (pbodyPart) {
                            const char* pname = pbodyPart->GetTargetName();
                            NiAVObject* plimb = BSUtilities::GetObjectByName(pactor->Get3D(), pname);
                            if (plimb) {
                                targetPos = plimb->GetWorldTranslate();
                                btargetSet = true;
                            }
                        }
                    }
                }
                if (!btargetSet) {
                    if (ptarget3d) {
                        targetPos = ptarget3d->GetWorldBound().m_kCenter;
                    }
                    else {
                        targetPos = ptarget->GetLocationOnReference();
                    }
                }
                if (pCurrentProcess->IsCurrentWeaponGrenade()) {
                    double ddistance = GetDistanceFromPoint(targetPos);
                    targetPos.z = fVATSGrenadeDistAimZMult.Float() * ddistance + targetPos.z;
                }
                NiPoint3 rot = CombatUtilities::GetAngleToProjectedPoint(this, targetPos);
                SetHeading(rot.z);
                SetLooking(rot.x);
            }
            pVATS.Update();
            *apMoveMode = static_cast<uint16_t>(
                GetMoveMode() &
                (ActorMover::MF_SNEAKING | ActorMover::MF_SWIMMING | ActorMover::MF_FALL | ActorMover::MF_SLIDE)
            );
            bretVal = true;
            break;
        }
        default: break;
    }
    return bretVal;
}

// Moves the player to the cell named apCellID (or apCellIn), loading an exterior cell first, and drops an exterior
// arrival onto the land.
void PlayerCharacter::CenterOnCell(char* apCellID, TESObjectCELL* apCellIn) {
    InitializationFlow();
    TESObjectCELL* pCell = apCellIn;
    NiPoint3 pos;
    TESWorldSpace* pworld = nullptr;
    if (!pCell) {
        pCell = pDataHandler->GetCellByEditorID(apCellID);
    }
    if (pExteriorCellLoader) {
        pExteriorCellLoader->CancelAllCellLoads();
    }
    if (!pCell) {
        int32_t ix = 0;
        int32_t iy = 0;
        pworld = pDataHandler->GetExtCellDataFromFileByEditorID(apCellID, ix, iy);
        if (pworld) {
            ClearCellsIfTargetNotLoaded(pCell, pworld);
            pCell = pworld->LoadCell(ix, iy);
        }
    }
    else if (!pCell->GetInterior()) {
        pworld = pCell->GetWorldSpace();
        if (pworld) {
            ClearCellsIfTargetNotLoaded(pCell, pworld);
            int32_t iDataY = pCell->GetDataY();
            int32_t iDataX = pCell->GetDataX();
            pworld->LoadCell(iDataX, iDataY);
        }
    }
    if (pCell) {
        NiPoint3 rot = NiPoint3::ZERO;
        pCell->GetCOCPlacementInfo(pos, rot);
        PositionPlayer(pos, rot, pCell, true);
        if (!pCell->GetInterior()) {
            float z;
            TES::pInstance->GetLandHeight(pos, z);
            // A negative or NaN height puts the player at 0.
            if (z >= 0.0) {
                pos.z = z;
            }
            else {
                pos.z = 0.0f;
            }
            SetPosition(pos);
        }
    }
}

bool PlayerCharacter::GetFlag66DIs2() {
    return (bCanFastTravel & 2) != 0;
}

// Each refusal shows its text for 2 seconds with emotion 0, the sad vault boy icon and no sound, and returns
// ShowMessage's result. When nothing forbids waiting it opens the wait menu.
bool PlayerCharacter::CanPassTime() {
    if (IsTresPassing()) {
        return Interface::ShowMessage(
            sNoWaitTrespass.String(),
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
            nullptr,
            2.0f,
            false
        );
    }
    if (IsAlarmForPlayerCharactaer()) {
        return Interface::ShowMessage(
            sNoWaitWhileAlarmSounding.String(),
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
            nullptr,
            2.0f,
            false
        );
    }
    // GetParentCell is called before the GetLocationOnReference virtual.
    TESObjectCELL* pCell = GetParentCell();
    const NiPoint3* pLocation = &GetLocationOnReference();
    if (IsPointDeepUnderWater(*pLocation, pCell, 0.875f)) {
        return Interface::ShowMessage(
            sNoWaitUnderWater.String(),
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
            nullptr,
            2.0f,
            false
        );
    }
    if (ReferenceProcessLists.AreHostilActorsNear(pPlayer->GetInterior())) {
        return Interface::ShowMessage(
            sNoWaitHostilActorsNear.String(),
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
            nullptr,
            2.0f,
            false
        );
    }
    if (static_cast<bhkCharacterController*>(pPlayer->GetCharController())->GetState() == HK_CHARACTER_JUMPING ||
        static_cast<bhkCharacterController*>(pPlayer->GetCharController())->GetState() == HK_CHARACTER_IN_AIR) {
        return Interface::ShowMessage(
            sNoWaitInAir.String(),
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
            nullptr,
            2.0f,
            false
        );
    }
    if (pPlayer->GetParentCell()->GetCantWaitHere() || !pPlayer->GetCanWait()) {
        return Interface::ShowMessage(
            sNoWaitInCell.String(),
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
            nullptr,
            2.0f,
            false
        );
    }
    // Both radiation tests show the same message.
    if (ReferenceProcessLists.IsPlayerInRadiationArea() ||
        (GetCurrentAIProcess() && GetCurrentAIProcess()->GetRadiationWaterDelta() != 0.0)) {
        return Interface::ShowMessage(
            sNoWaitInRadiation.String(),
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
            nullptr,
            2.0f,
            false
        );
    }
    if (pPlayer->HasDamageHealthEffect()) {
        return Interface::ShowMessage(
            sNoWaitTakingHealthDamage.String(),
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
            nullptr,
            2.0f,
            false
        );
    }
    Interface::CreateSleepMenu(true);
    return false;
}

bool PlayerCharacter::IsAlarmForPlayerCharactaer() {
    BSSimpleList<TESObjectREFR*>* plist = ReferenceProcessLists
                                              .IsActorTargetingREFinPackage(this, PACKAGE_TYPE_ALARM, false);
    bool bTargeting = plist != nullptr;
    delete plist;
    return bTargeting;
}

bool PlayerCharacter::GetCanWait() {
    return bCanWait;
}

// Optionally hides the third-person 3D, then finds its camera node by name.
void PlayerCharacter::Setup3rdPerson3D(NiAVObject* ap3rdPerson3D, bool abCull3rdPerson3D) {
    if (abCull3rdPerson3D) {
        ap3rdPerson3D->SetAppCulled(true);
    }
    p3rdPersonCamera = ap3rdPerson3D->GetObjectByName(FixedStrings::QCamera3rd());
}

// Drops the magic cast sound by assigning it a default handle.
void PlayerCharacter::ResetMagicCastSound() {
    if (SelectedSpellCastSound.IsValid()) {
        SelectedSpellCastSound = BSSoundHandle();
        iSelectedSpellCastSoundID = 0;
    }
}

void PlayerCharacter::UpdateFlyCamera(float afDelta, bool abFreezeTime) {
    static int32_t iFlyCameraFrameCount;
    TESObjectWEAP* pweapon;
    NiMatrix3 currentMatrix;
    // Move mode, passed by address to UpdateHeadingAndLooking.
    uint16_t smoveMode;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PLAYER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
        0x17A3
    );
    Controls* pcontrols = appMain->GetControls();
    SceneGraph* pWorldRoot = Main::GetWorldSceneGraph();
    if (pCurrentProcess->GetCurrentWeapon()) {
        pweapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
    }
    else {
        pweapon = nullptr;
    }
    smoveMode = 0;
    fDropAngleMod = 0.0f;
    fLastDropAngleMod = 0.0f;
    bOnElevator = false;
    HandleFlycamMovement();
    int32_t ijoyX = 0;
    int32_t ijoyY = 0;
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        ijoyX = pcontrols->GetControllerValue(0, 7);
        ijoyY = pcontrols->GetControllerValue(0, 8);
    }
    if (!abFreezeTime) {
        NiPoint3 forwardVec;
        if (b3rdPerson) {
            pCurrentProcess->RandomlyPlaySpecialIdles(this);
        }
        uint16_t slastMoveMode = GetMoveMode();
        smoveMode = slastMoveMode & 0xCC00;
        if (pVATS.eMode) {
            UpdateHeadingAndLooking(afDelta, false, &smoveMode, false);
        }

        int32_t imouseX = 0;
        int32_t imouseY = 0;
        PlayerCharacter::fCameraHeading = GetHeading(false);
        PlayerCharacter::fCameraLooking = GetLooking();
        if (pPlayer->pCurrentProcess->GetCurrentPackage()) {
            pPlayer->pCurrentProcess->ClearCurrentPackage();
        }
        if (pPlayer->pCurrentProcess->GetRunOncePackage()) {
            pPlayer->pCurrentProcess->ClearRunOncePackage();
        }
        if (pcontrols->GetUserAction(Controls::UACTN_READY_RELOAD_WEAPON, 1) ||
            pcontrols->GetUserAction(Controls::UACTN_READY_RELOAD_WEAPON, 0)) {
            if (!GetControlsDisabled(PlayerCharacter::DPC_FIGHTING)) {
                fReloadTimer = fReloadTimer + afDelta;
                if (CanMove() && GetAnimAction() == ANIM_ACTION_NONE) {
                    if (bDebounce ||
                        (!(fPlayerWeaponReloadTimer.Float() < static_cast<double>(fReloadTimer)) &&
                         pweapon &&
                         pweapon->GetCurrentAmmo(pPlayer)) ||
                        !GetWantWeaponDrawn()) {
                        if (!bDebounce && !GetWantWeaponDrawn()) {
                            if ((GetMoveMode() & 2048) == 0) {
                                SetWantWeaponDrawn(true);
                            }
                            bDebounce = 1;
                        }
                    }
                    else {
                        SetWantWeaponDrawn(false);
                        bDebounce = 1;
                    }
                }
            }
        }
        else if (!pcontrols->GetUserAction(Controls::UACTN_READY_RELOAD_WEAPON, 2)) {
            fReloadTimer = 0.0f;
            bDebounce = 0;
        }
        if (pcontrols->GetUserAction(Controls::UACTN_AUTOMOVE, 1)) {
            bAutoMove = !bAutoMove;
        }
        if (bAutoMove) {
            if (pcontrols->GetUserAction(Controls::UACTN_FORWARD, 0) ||
                pcontrols->GetUserAction(Controls::UACTN_BACK, 0) ||
                pcontrols->GetUserAction(Controls::UACTN_SLDRIGHT, 0) ||
                pcontrols->GetUserAction(Controls::UACTN_SLDLEFT, 0)) {
                bAutoMove = false;
            }
            else {
                pcontrols->ForceUserAction(Controls::UACTN_FORWARD);
            }
        }
        char cCrouchPressed = 0;
        if (pcontrols->GetUserAction(Controls::UACTN_CROUCH, 1)) {
            if (!IsParalyzed() &&
                !IsDead(false) &&
                !IsKnockedOut() &&
                !IsRestrained() &&
                !IsUnConscious() &&
                GetSitSleepState() == SIT_SLEEP_NORMAL) {
                ActionPoints::Action eaction = ActionPoints::ACTION_STAND;
                if ((smoveMode & ActorMover::MF_SNEAKING) != 0) {
                    smoveMode &= ~ActorMover::MF_SNEAKING;
                    BSAudio::QInstance()
                        .GetSoundHandleByName(
                            "NPCHumanCrouchUp",
                            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                BSGameSound::SOUND_FLAG_FIRST_PERSON
                        )
                        .Play(false);
                }
                else {
                    smoveMode |= ActorMover::MF_SNEAKING;
                    eaction = ActionPoints::ACTION_CROUCH;
                    BSAudio::QInstance()
                        .GetSoundHandleByName(
                            "NPCHumanCrouchDown",
                            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                BSGameSound::SOUND_FLAG_FIRST_PERSON
                        )
                        .Play(false);
                }
                UseActionPoints(eaction);
            }
            cCrouchPressed = 1;
        }
        currentMatrix = pWorldRoot->GetCameraRoot()->m_kLocal.m_Rotate;
        NiPoint3 kLocation = *QLocationOnReference();
        NiPoint3 sideVec(1.0f, 0.0f, 0.0f);
        currentMatrix.GetCol(1u, forwardVec);
        if (GetNoCollision()) {
            currentMatrix.GetCol(0, sideVec);
        }
        else {
            forwardVec.z = 0.0f;
            forwardVec.Unitize();
        }
        bool bdropping = false;
        if (eGrabType == PlayerCharacter::GRAB_INVENTORY_DROP) {
            bdropping = UpdateDropSpring(afDelta);
        }
        if (ijoyX) {
            if (ijoyX <= 0) {
                pcontrols->ForceUserAction(Controls::UACTN_SLDLEFT);
            }
            else {
                pcontrols->ForceUserAction(Controls::UACTN_SLDRIGHT);
            }
            double dJoyX = static_cast<double>(std::abs(ijoyX));
            PlayerCharacter::fPCTurnScale = fPlayerCharacterTurnMult.Float() * dJoyX;
            if (PlayerCharacter::fPCTurnScale > 1.5) {
                PlayerCharacter::fPCTurnScale = 1.5f;
            }
        }
        if (std::abs(ijoyY) >= std::abs(ijoyX)) {
            if (ijoyY) {
                if (ijoyY >= 0) {
                    pcontrols->ForceUserAction(Controls::UACTN_BACK);
                }
                else {
                    pcontrols->ForceUserAction(Controls::UACTN_FORWARD);
                }
                PlayerCharacter::fWalkSpeedMult = static_cast<double>(std::abs(ijoyY)) * 0.019999999552965164;
            }
        }
        else {
            PlayerCharacter::fWalkSpeedMult = static_cast<double>(std::abs(ijoyX)) * 0.019999999552965164;
            if (ijoyX >= 0) {
                smoveMode |= ActorMover::MF_RIGHT;
            }
            else {
                smoveMode |= ActorMover::MF_LEFT;
            }
        }
        if (PlayerCharacter::fWalkSpeedMult > 1.0) {
            PlayerCharacter::fWalkSpeedMult = 1.0f;
        }
        float fCurrentSpeed = GetCurrentSpeed();
        sideVec *= fCurrentSpeed * PlayerCharacter::fWalkSpeedMult;
        forwardVec *= fCurrentSpeed * PlayerCharacter::fWalkSpeedMult;
        pActorMover->ForceMoveMode(smoveMode);
        if (cCrouchPressed) {
            SetWalkRunFlagDirty();
        }
        b3rdPerson = true;
        bool bdisabled = false;
        if (IsKnockedOut() ||
            IsParalyzed() ||
            GetLifeState() ||
            GetAnimAction() == ANIM_ACTION_STAGGER ||
            GetSitSleepState()) {
            bdisabled = true;
        }
        if (bdisabled || GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
            static_cast<PlayerMover*>(pActorMover)->PlayerMover::SetMoveMode(831u, false);
        }
        if (!bdisabled && !GetControlsDisabled(PlayerCharacter::DPC_FIGHTING)) {
            if (eGrabType == PlayerCharacter::GRAB_INVENTORY_DROP || eGrabType == PlayerCharacter::GRAB_TELEKINESIS) {
                inextAttack = 0;
                fpowerTimer = 0.0f;
            }
            else {
                if (pCurrentProcess->CanAttack()) {
                    CheckUserInputAttacks();
                }
                else {
                    inextAttack = 0;
                    fpowerTimer = 0.0f;
                }
                if ((pcontrols->GetUserAction(Controls::UACTN_BLOCK, 1) ||
                     pcontrols->GetUserAction(Controls::UACTN_BLOCK, 0)) &&
                    pCurrentProcess->GetAnimAction() != ANIM_ACTION_BLOCK &&
                    !GetIronSights() &&
                    IsWeaponDrawn()) {
                    if (!IsPipboyActive() && pCurrentProcess->CanAttack()) {
                        if (IsWeaponDrawn() && pweapon && !pweapon->IsMeleeWeapon()) {
                            SetIronSights(true, false, false);
                        }
                        else {
                            SetBlock(true);
                        }
                    }
                }
                else if (
                    (pcontrols->GetUserAction(Controls::UACTN_BLOCK, 2) ||
                     !pcontrols->GetUserAction(Controls::UACTN_BLOCK, 0) ||
                     IsPipboyActive()) &&
                    (GetIronSights() || pCurrentProcess->GetAnimAction() == ANIM_ACTION_BLOCK)
                ) {
                    SetIronSights(false, false, false);
                    SetBlock(false);
                }
            }
            if (pcontrols->GetUserAction(Controls::UACTN_JUMP, 1) && !IsOverEncumbered()) {
                uint16_t usGroup = GetAnimation()->GetGroup(AGS_WEAPON);
                if (!TESAnimGroup::IsPowerAttackAction(usGroup) && GetAnimAction() != ANIM_ACTION_DODGE) {
                    bool bNotJumpingOrSwimming = !IsJumping() && !IsSwimming();
                    bool bsuccess = false;
                    bool bpressingBlock = pcontrols->GetUserAction(Controls::UACTN_BLOCK, 0) != 0;
                    bool bblocking = GetBlocked();
                    if (bpressingBlock) {
                        if (Interface::GetActivateREFR()) {
                            if (!bblocking) {
                                if (static_cast<uint8_t>(GetCharController()->CanJump())) {
                                    bsuccess = Jump() != nullptr;
                                }
                            }
                        }
                        else {
                            if (GetCharController()->GetState() == HK_CHARACTER_ON_GROUND) {
                                // Dodge takes the move mode's low byte.
                                ANIM_GROUP_ENUM eresult = Dodge(static_cast<uint8_t>(smoveMode));
                                bsuccess = eresult != ANIM_GROUP_NONE;
                            }
                        }
                    }
                    else {
                        if (static_cast<uint8_t>(GetCharController()->CanJump())) {
                            bsuccess = Jump() != nullptr;
                        }
                    }
                    ForcePhysicsGrabDrop();
                    if (bNotJumpingOrSwimming && bsuccess) {
                        GetCharController()->CanJump();
                    }
                }
            }
        }
        b3rdPerson = true;
        UpdateAnimation();
        PickAnimations(PlayerCharacter::fWalkSpeedMult, PlayerCharacter::fPCTurnScale);
        b3rdPerson = bActually3rdPerson;
        if (IsSleeping()) {
            PlayerSleep();
        }
        SetNoCollision(false);
        static_cast<PlayerMover*>(pActorMover)->PlayerMover::SetSideVector(sideVec);
        pActorMover->Update(afDelta);
        if (pcontrols->GetUserAction(Controls::UACTN_ACTIVATE, 1) &&
            !Interface::IsInMenuMode() &&
            !GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT) &&
            !IsParalyzed() &&
            !bdropping) {
            Actor* pactor = static_cast<Actor*>(Interface::GetActivateREFR());
            bool bcheckFurniture = true;
            bool bskipActivate = false;
            if (pactor && pactor->IsFurniture() && GetIronSights()) {
                bskipActivate = true;
            }

            if (pactor && pactor->IsProjectile()) {
                TESObjectCONT* parrow = reinterpret_cast<TESObjectCONT*>(dynamic_cast<ArrowProjectile*>(pactor));
                if (parrow && !parrow->GetOpenSound()) {
                    bskipActivate = true;
                }
            }

            if (pactor && pactor->IsActor()) {
                BaseProcess* pprocess = pactor->pCurrentProcess;
                if (pprocess->GetFadeState() == BaseProcess::FADE_OUT_DISABLE ||
                    pprocess->GetFadeState() == BaseProcess::FADE_OUT_DELETE) {
                    bskipActivate = true;
                }
            }

            if (!bskipActivate) {
                Interface::SetInfoForRef(nullptr, false, false);
                if (pactor && pactor->Activate(this, false, nullptr, 1)) {
                    bcheckFurniture = false;
                }

                if (bcheckFurniture && pCurrentProcess->GetCurrentFurniture()) {
                    pCurrentProcess->GetCurrentFurniture()->Activate(this, false, nullptr, 1);
                }
            }
        }

        if (pPlayerSphere) {
            uint32_t uiFrameCount = iFlyCameraFrameCount++;
            if (uiFrameCount < 20) {
                NiPoint3 np;
                np.z = kLocation.z + 128.0;
                np.y = kLocation.y + 64.0;
                np.x = kLocation.x + 64.0;
                pPlayerSphere->SetPosition(np);
                pPlayerSphere->SetLinearVelocity(NiPoint3::ZERO);
            }
        }
        if (!IsDead(false) && !IsDying()) {
            UpdateMagic(false, false);
        }
        Animation* panim = GetAnimation(false);
        Animation* panim1st = GetAnimation(true);
        panim1st->SetMoveSpeed(panim->GetMoveSpeed());
        panim1st->SetAttackSpeed(panim->m_fAttackSpeed);
        b3rdPerson = !b3rdPerson;
        Character::Update(afDelta);
        UpdateAnimationMovement(GetAnimation(!b3rdPerson), afDelta);
        b3rdPerson = !b3rdPerson;
        Character::Update(afDelta);
        UpdateAnimationMovement(GetAnimation(!b3rdPerson), afDelta);
        HandlePostAnimationActions();
        if (pCurrentProcess->GetCurrentMuzzleFlash()) {
            pCurrentProcess->GetCurrentMuzzleFlash()->Update(afDelta, this);
        }
    }
    if (pcontrols->GetUserAction(Controls::UACTN_QUICKSAVE, 1) && !bQuickSaving) {
        bQuickSaving = 1;
        pSaveLoadManager->Quicksave();
        bQuickSaving = 0;
    }

    if (pcontrols->GetUserAction(Controls::UACTN_QUICKLOAD, 1) && !bQuickLoading) {
        bQuickLoading = 1;
        pSaveLoadManager->Quickload();
        bQuickLoading = 0;
    }
}

// While a thrown grenade's attack waits at its stop key, both animations are held at that key.
void PlayerCharacter::ForceGrenadeHold() {
    Animation* p3rdPersonAnimation = GetAnimation(false);
    Animation* p1stAnimation = GetAnimation(true);
    if (!p3rdPersonAnimation || !p1stAnimation) {
        return;
    }
    if (!pCurrentProcess->IsCurrentWeaponGrenade() || !pCurrentProcess->GetWeaponDrawn()) {
        return;
    }
    if (GetAnimAction() != ANIM_ACTION_ATTACK_THROW || p3rdPersonAnimation->GetGroupAction(AGS_WEAPON) != 1) {
        return;
    }
    p3rdPersonAnimation->SkipUpdate(AGS_WEAPON);
    p1stAnimation->SkipUpdate(AGS_WEAPON);
    BSAnimGroupSequence* pSequence = p3rdPersonAnimation->GetCurrentSequence(AGS_WEAPON);
    if (pSequence) {
        double dStopTime = pSequence->GetAnimGroup()->GetTime(AGA_STOP);
        float fTime = static_cast<float>(dStopTime - pSequence->GetOffset());
        p3rdPersonAnimation->SetTime(fTime);
    }
    pSequence = p1stAnimation->GetCurrentSequence(AGS_WEAPON);
    if (pSequence) {
        double dStopTime = pSequence->GetAnimGroup()->GetTime(AGA_STOP);
        float fTime = static_cast<float>(dStopTime - pSequence->GetOffset());
        p1stAnimation->SetTime(fTime);
    }
}

// The movement flags of Actor::GetMoveMode the attacks read: 1 forward, 2 back, 4 left, 8 right, 0x200 run,
// 0x400 walk, 0x800 sneak, 0x2000 swim.
bool PlayerCharacter::CheckUserInputAttacks() {
    ANIM_GROUP_ENUM eAttackAnim = ANIM_GROUP_NONE;
    Animation* pAnimation = GetAnimation(false);
    Animation* p1stAnimation = GetAnimation(true);
    Controls* pControls = appMain->GetControls();
    bool bResult = false;
    TESObjectWEAP* pWeapon = nullptr;
    if (pCurrentProcess->GetCurrentWeapon()) {
        pWeapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject());
    }
    bool bInVATS = pVATS.GetMode() == VATS::VATS_PLAYBACK;
    VATS_COMMAND* pCurrentAction = nullptr;
    // Keeps the timer's seconds as a double and rounds the product to a float.
    double dSeconds = appTimer.GetSeconds();
    float fDelta = static_cast<float>(pVATS.GetPlayerUpdateMult() * dSeconds);

    // In VATS playback the queued command fires through the use control once its aim (or melee idle) is ready.
    if (bInVATS) {
        inextAttack = 0;
        pCurrentAction = pVATS.GetCurrentAction();
        if (ivatsAttackDelay) {
            --ivatsAttackDelay;
        }
        else if (
            (pCurrentAction &&
             ActionPoints::IsAttackAction(pCurrentAction->eAction) &&
             pCurrentAction->cFireShots &&
             !pCurrentAction->bStranger &&
             !pCurrentAction->bMissFortune) &&
            ((pWeapon && pWeapon->IsAutomaticWeapon()) ||
             ((TESAnimGroup::GetType(pAnimation->GetGroup(AGS_WEAPON)) == ANIM_GROUP_AIM ||
               TESAnimGroup::GetType(pAnimation->GetGroup(AGS_WEAPON)) == ANIM_GROUP_AIM_IS) &&
              pAnimation->GetCurrentSequence(AGS_WEAPON)->GetState() == NiControllerSequence::kAnimating &&
              (!pAnimation->GetCurrentSequence(AGS_WEAPON_UP) ||
               pAnimation->GetCurrentSequence(AGS_WEAPON_UP)->GetState() == NiControllerSequence::kAnimating) &&
              (!pAnimation->GetCurrentSequence(AGS_WEAPON_DOWN) ||
               pAnimation->GetCurrentSequence(AGS_WEAPON_DOWN)->GetState() == NiControllerSequence::kAnimating)))
        ) {
            bool bFire = false;
            if (pVATS.GetMeleeAttack() ||
                ((!pWeapon || pWeapon->IsMeleeWeapon()) &&
                 pVATS.GetCurrentCamera() &&
                 pVATS.GetCurrentCamera()->IsInitialized() &&
                 pVATS.GetCurrentCameraMinTimer() <= 0.0)) {
                if (pAnimation->SpecialIdleLoaded() && !pAnimation->IsIdleNotLoaded(pVATS.GetMeleeAttack())) {
                    bFire = true;
                }
            }
            else {
                bFire = true;
            }
            if (bFire) {
                pControls->ForceUserAction(Controls::UACTN_USE);
                if (!pWeapon || !pWeapon->IsAutomaticWeapon()) {
                    ivatsAttackDelay = 3;
                    switch (pCurrentAction->eAction) {
                        case ActionPoints::ACTION_ATTACK_UNARMED:
                        case ActionPoints::ACTION_ATTACK_ONE_HAND_MELEE:
                        case ActionPoints::ACTION_ATTACK_TWO_HAND_MELEE:
                        case ActionPoints::ACTION_PLAYER_SPECIAL_WEAPON_ATTACK:
                        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK:
                        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2:
                        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3: --pCurrentAction->cFireShots; break;
                        default: break;
                    }
                }
            }
        }
    }
    else {
        ivatsAttackDelay = 0;
    }

    if (pCurrentProcess->GetAutomaticFireAtLeastOnce()) {
        inextAttack = 0;
        pControls->ForceUserAction(Controls::UACTN_USE);
    }

    bool bSustainedFire = false;
    if (inextAttack) {
        // A queued attack plays once the current one reaches its switch key.
        if (TESAnimGroup::IsAttackAction(pAnimation->GetGroup(AGS_WEAPON))) {
            if (!TESAnimGroup::IsPowerAttackAction(pAnimation->GetGroup(AGS_WEAPON))) {
                if (pCurrentProcess->IsCurrentWeaponGrenadeOrMine()) {
                    if ((GetAnimAction() != ANIM_ACTION_ATTACK_THROW &&
                         GetAnimAction() != ANIM_ACTION_ATTACK_THROW_ATTACH) ||
                        pAnimation->GetGroupAction(AGS_WEAPON) > 2) {
                        if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
                            eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
                        }
                        else if (
                            pCurrentProcess->IsCurrentWeaponGrenade() || pCurrentProcess->IsCurrentWeaponThrown()
                        ) {
                            eAttackAnim = ANIM_GROUP_ATTACK_THROW;
                        }
                        else if (pCurrentProcess->IsCurrentWeaponMine()) {
                            eAttackAnim = ANIM_GROUP_PLACE_MINE;
                        }
                        fProjectileReleaseTimer = 0.0f;
                    }
                }
                else if (pAnimation->GetGroupAction(AGS_WEAPON) == 3) {
                    if (inextAttack == 1) {
                        switch (pAnimation->FindSwitchKey(nullptr)) {
                            case '3': eAttackAnim = ANIM_GROUP_ATTACK_3; break;
                            case '4': eAttackAnim = ANIM_GROUP_ATTACK_4; break;
                            case '5': eAttackAnim = ANIM_GROUP_ATTACK_5; break;
                            case '6': eAttackAnim = ANIM_GROUP_ATTACK_6; break;
                            case '7': eAttackAnim = ANIM_GROUP_ATTACK_7; break;
                            case '8': eAttackAnim = ANIM_GROUP_ATTACK_8; break;
                            case 'l': eAttackAnim = ANIM_GROUP_ATTACK_LEFT; break;
                            default:
                                if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
                                    eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
                                }
                                else {
                                    eAttackAnim = ANIM_GROUP_ATTACK_RIGHT;
                                }
                                break;
                        }
                    }
                    else if (inextAttack == 2) {
                        eAttackAnim = ANIM_GROUP_ATTACK_NORMAL_POWER;
                    }
                }
            }
        }
        else if (pCurrentProcess->IsCurrentWeaponGrenadeOrMine()) {
            if (GetAnimAction() != ANIM_ACTION_ATTACK_FOLLOW_THROUGH || pAnimation->GetGroupAction(AGS_WEAPON) > 2) {
                if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
                    eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
                }
                else if (pCurrentProcess->IsCurrentWeaponGrenade() || pCurrentProcess->IsCurrentWeaponThrown()) {
                    eAttackAnim = ANIM_GROUP_ATTACK_THROW;
                }
                else {
                    eAttackAnim = ANIM_GROUP_PLACE_MINE;
                }
                fProjectileReleaseTimer = 0.0f;
            }
        }
        else if (inextAttack == 1) {
            if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
                eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
            }
            else {
                eAttackAnim = ANIM_GROUP_ATTACK_RIGHT;
            }
        }
        else if (inextAttack == 2) {
            eAttackAnim = ANIM_GROUP_ATTACK_NORMAL_POWER;
        }
    }
    else if (pControls->GetUserAction(Controls::UACTN_USE, Controls::isPressed) && !bReleaseTheArrow) {
        // The attack control went down.
        if (!pCurrentProcess->GetWeaponDrawn()) {
            if (!(GetMoveMode() & 0x800)) {
                SetWantWeaponDrawn(true);
            }
        }
        else if (GetAnimAction() == ANIM_ACTION_RELOAD_LOOP) {
            // Ends a looping reload: loads one more round when there is one and plays the loop's end.
            ItemChange* pAmmoChange = pCurrentProcess->GetCurrentAmmo();
            if (pAmmoChange && !pAmmoChange->GetCount()) {
                return false;
            }
            uint16_t usMoveMode = static_cast<uint16_t>(GetMoveMode());
            ANIM_GROUP_MOVE_ENUM eMove = AGM_NONE;
            ANIM_GROUP_WEAPON_ENUM eWeaponType = AGW_NONE;
            if (usMoveMode & 0x800) {
                eMove = AGM_SNEAK;
            }
            else if (usMoveMode & 0x2000) {
                eMove = AGM_SWIM;
            }
            else if (usMoveMode & 0x400) {
                eMove = AGM_WALK;
            }
            ANIM_GROUP_ENUM eType = ANIM_GROUP_NONE;
            if (usMoveMode & 0x200) {
                if (usMoveMode & 1) {
                    eType = ANIM_GROUP_FAST_FORWARD;
                }
                else if (usMoveMode & 2) {
                    eType = ANIM_GROUP_FAST_BACK;
                }
                else if (usMoveMode & 4) {
                    eType = ANIM_GROUP_FAST_LEFT;
                }
                else if (usMoveMode & 8) {
                    eType = ANIM_GROUP_FAST_RIGHT;
                }
            }
            else if (usMoveMode & 1) {
                eType = ANIM_GROUP_MOVE_FORWARD;
            }
            else if (usMoveMode & 2) {
                eType = ANIM_GROUP_MOVE_BACK;
            }
            else if (usMoveMode & 4) {
                eType = ANIM_GROUP_MOVE_LEFT;
            }
            else if (usMoveMode & 8) {
                eType = ANIM_GROUP_MOVE_RIGHT;
            }
            int32_t iCount = 0;
            int32_t iClipRounds = 0;
            ItemChange* pAmmo = pCurrentProcess->GetCurrentAmmo();
            ItemChange* pWeaponChange = pCurrentProcess->GetCurrentWeapon();
            InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(this);
            if (pWeaponChange) {
                iClipRounds = pWeapon->GetFormClipRounds(
                    pWeaponChange->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE)
                );
            }
            int32_t iObjectCount = 0;
            if (pChanges) {
                iObjectCount = pChanges->GetObjectCount(pAmmo->GetItemChangeObject());
            }
            if (pAmmo) {
                iCount = pAmmo->GetCount() + 1;
            }
            if (pAmmo && iCount <= iObjectCount && iCount < iClipRounds) {
                QueueEquipObject(pAmmo->GetItemChangeObject(), iCount, nullptr, true, false, false);
            }
            bool bPowerArmor = HasBackPackWorn() || IsPowerBodyArmorWorn();
            uint16_t usGroup = pAnimation->PickBestAnimation(
                TESAnimGroup::AnimGroup(eMove, eWeaponType, eType, bPowerArmor),
                false
            );
            if (usGroup != ANIM_GROUP_NONE) {
                GetAnimation(false)->PlayGroup(usGroup, ACTION_START, 1, AGS_WEAPON);
                SetAnimAction(ANIM_ACTION_NONE, GetAnimation(false)->GetCurrentSequence(AGS_WEAPON));
                StartAnimOn1stPerson(usGroup, ACTION_START);
            }
        }
        else if (
            TESAnimGroup::IsAttackAction(pAnimation->GetGroup(AGS_WEAPON)) && !(pWeapon && pWeapon->IsAutomaticWeapon())
        ) {
            // During a single-shot attack: queue the next one once the current one is past its switch key.
            if (!pCurrentProcess->IsCurrentWeaponGrenadeOrMine() &&
                (!IsSneaking() || !TESAnimGroup::IsPowerAttackAction(pAnimation->GetGroup(AGS_WEAPON)))) {
                if (pWeapon && (pWeapon->GetCurrentAmmo(pPlayer) || pWeapon->IsRockItLauncher())) {
                    float fLastScaledTime = pAnimation->GetCurrentSequence(AGS_WEAPON)->GetLastScaledTime();
                    float fKeyLength = static_cast<float>(
                        pAnimation->GetCurrentSequence(AGS_WEAPON)->GetKeyLengthOverFrequency()
                    );
                    float fSwitchTime = 0.0f;
                    pAnimation->FindSwitchKey(&fSwitchTime);
                    // The two floats are subtracted at double precision and the unrounded difference is compared.
                    if (!pWeapon->IsAutomaticWeapon() &&
                        (fSwitchTime <= fLastScaledTime ||
                         static_cast<double>(fKeyLength) - static_cast<double>(fLastScaledTime) < 0.5)) {
                        inextAttack = 1;
                    }
                }
                else {
                    inextAttack = 1;
                }
            }
        }
        else {
            // Starts a new attack.
            bSpinAttackReleasedEarly = false;
            pCurrentProcess->ClearAutomaticShotsToFire(true);
            pCurrentProcess->SetShotsToFire(1);
            if (pWeapon) {
                bSustainedFire = true;
            }
            if (pWeapon &&
                pWeapon->IsAutomaticWeapon() &&
                !TESAnimGroup::IsAttackSpinAction(pWeapon->GetAttackAnim())) {
                pCurrentProcess->SetAutomaticFireAtLeastOnce(true);
            }
            if (!pWeapon || !TESAnimGroup::IsAttackSpinAction(pWeapon->GetAttackAnim())) {
                if (pCurrentProcess->IsCurrentWeaponGrenade()) {
                    if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
                        eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
                    }
                    else {
                        eAttackAnim = ANIM_GROUP_ATTACK_THROW;
                    }
                    AddPlayerAction(PLAYER_ACTION_THROW_GRANADE, 3.0f, nullptr);
                    if (bInVATS) {
                        fProjectileReleaseTimer = 10.0f;
                    }
                    else {
                        fProjectileReleaseTimer = 0.0f;
                    }
                }
                else if (pCurrentProcess->IsCurrentWeaponMine()) {
                    if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
                        eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
                    }
                    else {
                        eAttackAnim = ANIM_GROUP_PLACE_MINE;
                    }
                    AddPlayerAction(PLAYER_ACTION_LAY_MINE, 3.0f, nullptr);
                    if (bInVATS) {
                        fProjectileReleaseTimer = 10.0f;
                    }
                    else {
                        fProjectileReleaseTimer = 0.0f;
                    }
                }
                else if (pCurrentProcess->IsCurrentWeaponThrown()) {
                    if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
                        eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
                    }
                    else {
                        eAttackAnim = ANIM_GROUP_ATTACK_THROW;
                    }
                    AddPlayerAction(PLAYER_ACTION_FIRE_WEAPON, 3.0f, nullptr);
                    if (bInVATS) {
                        fProjectileReleaseTimer = 10.0f;
                    }
                    else {
                        fProjectileReleaseTimer = 0.0f;
                    }
                }
                else if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
                    eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
                }
                else {
                    eAttackAnim = ANIM_GROUP_ATTACK_RIGHT;
                }
            }
        }
        fpowerTimer = 0.0f;
        bResult = true;
    }
    else if (
        pControls->GetUserAction(Controls::UACTN_USE, Controls::isHeld) &&
        pCurrentProcess->GetWeaponDrawn() &&
        GetWantWeaponDrawn() &&
        !bReleaseTheArrow
    ) {
        // The attack control is held: cook throws, keep automatic fire going and time the power attack.
        if ((pCurrentProcess->IsCurrentWeaponGrenade() || pCurrentProcess->IsCurrentWeaponThrown()) &&
            (GetAnimAction() == ANIM_ACTION_ATTACK_THROW || GetAnimAction() == ANIM_ACTION_ATTACK_THROW_ATTACH) &&
            pAnimation->GetGroupAction(AGS_WEAPON) <= 2) {
            fProjectileReleaseTimer = fProjectileReleaseTimer + fDelta;
        }
        if (pCurrentAction &&
            pWeapon &&
            pWeapon->GetCurrentAmmo(pPlayer) &&
            !pCurrentProcess->GetCurrentAmmo()->GetCount()) {
            pCurrentAction->cFireShots = 0;
        }
        if (pWeapon &&
            ((!pWeapon->GetCurrentAmmo(pPlayer) && pWeapon->IsAutomaticWeapon()) ||
             (pWeapon->GetCurrentAmmo(pPlayer) && pCurrentProcess->GetCurrentAmmo()))) {
            AddPlayerAction(PLAYER_ACTION_FIRE_WEAPON, 2.0f, nullptr);
            if (pWeapon->IsAutomaticWeapon()) {
                bool bFire = true;
                uint8_t ucShots = pCurrentProcess->GetAutomaticShotsToFire(this);
                if (TESAnimGroup::IsAttackSpinAction(pAnimation->GetGroup(AGS_WEAPON))) {
                    int32_t iGroupAction = pAnimation->GetGroupAction(AGS_WEAPON);
                    if (iGroupAction < 1 || iGroupAction >= 2) {
                        bFire = false;
                    }
                    else {
                        bSustainedFire = true;
                    }
                }
                else {
                    bSustainedFire = true;
                }
                if (ucShots && bFire) {
                    pCurrentProcess->SetShotsToFire(ucShots);
                    switch (pAnimation->FindSwitchKey(nullptr)) {
                        case '3': eAttackAnim = ANIM_GROUP_ATTACK_3; break;
                        case '4': eAttackAnim = ANIM_GROUP_ATTACK_4; break;
                        case '5': eAttackAnim = ANIM_GROUP_ATTACK_5; break;
                        case '6': eAttackAnim = ANIM_GROUP_ATTACK_6; break;
                        case '7': eAttackAnim = ANIM_GROUP_ATTACK_7; break;
                        case '8': eAttackAnim = ANIM_GROUP_ATTACK_8; break;
                        case 'l': eAttackAnim = ANIM_GROUP_ATTACK_LEFT; break;
                        default:
                            if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
                                eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
                            }
                            else {
                                eAttackAnim = ANIM_GROUP_ATTACK_RIGHT;
                            }
                            break;
                    }
                }
            }
        }
        else {
            if (pCurrentProcess->IsCurrentWeaponGrenade()) {
                AddPlayerAction(PLAYER_ACTION_THROW_GRANADE, 3.0f, nullptr);
            }
            else if (pCurrentProcess->IsCurrentWeaponMine()) {
                AddPlayerAction(PLAYER_ACTION_LAY_MINE, 3.0f, nullptr);
            }
            else if (pCurrentProcess->IsCurrentWeaponThrown()) {
                AddPlayerAction(PLAYER_ACTION_FIRE_WEAPON, 3.0f, nullptr);
            }
            else {
                AddPlayerAction(PLAYER_ACTION_SWING_MELEE_WEAPON, 1.5f, nullptr);
            }
            if (!TESAnimGroup::IsPowerAttackAction(pAnimation->GetGroup(AGS_WEAPON)) &&
                inextAttack != 2 &&
                !pCurrentProcess->IsCurrentWeaponGrenadeOrMine()) {
                fpowerTimer = fpowerTimer + fDelta;
            }
        }
        if (!IsOverEncumbered() && fPowerAttackDelay.Float() < fpowerTimer) {
            if (IsSwimming()) {
                inextAttack = 1;
            }
            else if (
                TESAnimGroup::IsAttackAction(pAnimation->GetGroup(AGS_WEAPON)) &&
                pAnimation->GetTime(pAnimation->GetCurrentSequence(AGS_WEAPON)) > 0.0
            ) {
                inextAttack = 2;
            }
            else {
                eAttackAnim = ANIM_GROUP_ATTACK_NORMAL_POWER;
            }
            fpowerTimer = 0.0f;
        }
        bResult = true;
    }
    else if (
        pControls->GetUserAction(Controls::UACTN_READY_RELOAD_WEAPON, Controls::isDepressed) &&
        pCurrentProcess->GetWeaponDrawn() &&
        fReloadTimer != 0.0 &&
        fPlayerWeaponReloadTimer.Float() > fReloadTimer &&
        pCurrentProcess->GetCurrentWeapon()
    ) {
        // A short press of the reload control reloads (or opens the Rock-It Launcher's container).
        if (GetAnimAction() == ANIM_ACTION_NONE || pAnimation->GetGroupAction(AGS_WEAPON) == 3) {
            if (pWeapon->IsRockItLauncher()) {
                Interface::QueueMenuCreate(
                    Interface::OPEN_CONTAINER,
                    nullptr,
                    nullptr,
                    nullptr,
                    Interface::CMM_ROCK_N_LOAD,
                    false
                );
            }
            else {
                bool bRegenMod = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_AMMO_REGEN_SECONDS);
                TESObjectWEAP* pCurrentWeapon = static_cast<TESObjectWEAP*>(
                    pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject()
                );
                if (pCurrentWeapon->GetAmmoRegenRate(bRegenMod) <= 0.0) {
                    ReloadWeaponNV(
                        pWeapon,
                        2,
                        pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE),
                        false
                    );
                }
            }
            pCurrentProcess->SetSustainedFire(false);
            bSustainedFire = false;
        }
    }
    else if (
        TESAnimGroup::IsAttackAction(pAnimation->GetGroup(AGS_WEAPON)) &&
        (pControls->GetUserAction(Controls::UACTN_USE, Controls::isDepressed) ||
         !pControls->GetUserAction(Controls::UACTN_USE, Controls::isHeld))
    ) {
        // The attack control was let go during an attack.
        pCurrentProcess->SetSustainedFire(false);
        bSustainedFire = false;
        if (TESAnimGroup::IsAttackSpinAction(pAnimation->GetGroup(AGS_WEAPON))) {
            bSpinAttackReleasedEarly = pAnimation->GetGroupAction(AGS_WEAPON) < 1;
        }
    }

    if (eAttackAnim != ANIM_GROUP_NONE) {
        if ((!pWeapon || pWeapon->IsMeleeWeapon()) && GetAnimAction() == ANIM_ACTION_BLOCK) {
            SetBlock(false);
        }
        if (IsSneaking() &&
            (!pWeapon || (pWeapon->IsMeleeWeapon() && !pWeapon->IsAutomaticWeapon())) &&
            !IsSwimming()) {
            eAttackAnim = ANIM_GROUP_ATTACK_NORMAL_POWER;
        }
        bool bSpeak = false;
        bool bUnarmed = false;
        if (!pWeapon || pWeapon->GetWeaponSkill() == ActorValue::UNARMED) {
            bUnarmed = true;
        }
        if (bUnarmed && GetBlockCounterTimer() > 0.0) {
            float fCounterAttack = 0.0f;
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_COUNTER_ATTACK, pPlayer, &fCounterAttack);
            if (fCounterAttack > 0.0) {
                eAttackAnim = ANIM_GROUP_COUNTER;
            }
        }
        if (eAttackAnim == ANIM_GROUP_ATTACK_NORMAL_POWER) {
            // A power attack takes the direction of the movement, unless sneaking or both legs are crippled.
            ActorValueOwner* pValues = this;
            if (IsSneaking() ||
                (!(pValues->GetActorFloatValue(ActorValue::LEFT_MOBILITY_CONDITION) > 0.0) &&
                 !(pValues->GetActorFloatValue(ActorValue::RIGHT_MOBILITY_CONDITION) > 0.0))) {
                if (bUnarmed) {
                    float fCrouched = 0.0f;
                    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_CROUCHED_POWER_ATTACK, pPlayer, &fCrouched);
                    if (fCrouched > 0.0) {
                        eAttackAnim = ANIM_GROUP_ATTACK_CUSTOM_3_POWER;
                    }
                }
            }
            else {
                uint16_t usMoveMode = static_cast<uint16_t>(GetMoveMode());
                if (usMoveMode & 1) {
                    eAttackAnim = ANIM_GROUP_ATTACK_FORWARD_POWER;
                }
                else if (usMoveMode & 2) {
                    eAttackAnim = ANIM_GROUP_ATTACK_BACK_POWER;
                }
                else if (usMoveMode & 4) {
                    eAttackAnim = ANIM_GROUP_ATTACK_LEFT_POWER;
                }
                else if (usMoveMode & 8) {
                    eAttackAnim = ANIM_GROUP_ATTACK_RIGHT_POWER;
                }
                if (eAttackAnim == ANIM_GROUP_ATTACK_FORWARD_POWER &&
                    pWeapon &&
                    pWeapon->GetPowerAttackAnimOverride() != ANIM_GROUP_NONE) {
                    eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetPowerAttackAnimOverride());
                }
                if (bUnarmed) {
                    float fForward = 0.0f;
                    BGSEntryPoint::HandleEntryPoint(
                        BGSEntryPoint::HAS_UNARMED_FORWARD_POWER_ATTACK,
                        pPlayer,
                        &fForward
                    );
                    float fBack = 0.0f;
                    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_UNARMED_BACKWARD_POWER_ATTACK, pPlayer, &fBack);
                    float fLeft = 0.0f;
                    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_UNARMED_LEFT_POWER_ATTACK, pPlayer, &fLeft);
                    float fRight = 0.0f;
                    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_UNARMED_RIGHT_POWER_ATTACK, pPlayer, &fRight);
                    if (eAttackAnim == ANIM_GROUP_ATTACK_FORWARD_POWER && fForward > 0.0) {
                        eAttackAnim = ANIM_GROUP_ATTACK_CUSTOM_1_POWER;
                    }
                    else if (eAttackAnim == ANIM_GROUP_ATTACK_BACK_POWER && fBack > 0.0) {
                        eAttackAnim = ANIM_GROUP_ATTACK_CUSTOM_2_POWER;
                    }
                    else if (eAttackAnim == ANIM_GROUP_ATTACK_LEFT_POWER && fLeft > 0.0) {
                        eAttackAnim = ANIM_GROUP_ATTACK_CUSTOM_4_POWER;
                    }
                    else if (eAttackAnim == ANIM_GROUP_ATTACK_RIGHT_POWER && fRight > 0.0) {
                        eAttackAnim = ANIM_GROUP_ATTACK_CUSTOM_5_POWER;
                    }
                }
            }
            if (RandomBool(fCombatSpeakPowerAttackChance.Float())) {
                bSpeak = true;
            }
        }
        // The VATS special attacks pick their own groups.
        if (bInVATS && pCurrentAction) {
            if (pCurrentAction->eAction == ActionPoints::ACTION_PLAYER_SPECIAL_WEAPON_ATTACK) {
                eAttackAnim = ANIM_GROUP_ATTACK_FORWARD_POWER;
                // The asm tests the override against 0 here, not 0xFF as above.
                if (pWeapon && pWeapon->GetPowerAttackAnimOverride()) {
                    eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetPowerAttackAnimOverride());
                }
                if (!TESAnimGroup::IsPowerAttackAction(static_cast<uint16_t>(eAttackAnim))) {
                    eAttackAnim = ANIM_GROUP_ATTACK_NORMAL_POWER;
                }
            }
            else if (pCurrentAction->eAction == ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK && !IsSneaking()) {
                eAttackAnim = ANIM_GROUP_ATTACK_6;
            }
            else if (pCurrentAction->eAction == ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2) {
                eAttackAnim = ANIM_GROUP_ATTACK_7;
            }
            else if (pCurrentAction->eAction == ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3) {
                eAttackAnim = ANIM_GROUP_STOMP;
            }
        }
        bool bStarted = StartAttack(eAttackAnim);
        if (bStarted && pWeapon && (pWeapon->IsAutomaticWeapon() || pContinuousBeamPersistant) && bSustainedFire) {
            pCurrentProcess->SetSustainedFire(true);
        }
        if (bStarted && bSpeak) {
            // Combat topic 1, the power attack taunt.
            pCombatDialogueManager->StartDialogue(pPlayer, nullptr, DT_COMBAT, 1, false, nullptr);
        }
        inextAttack = 0;
        bUserDebounceBow = true;
    }

    // While the attack control is held, a melee or thrown attack waits at its hold key.
    if ((pControls->GetUserAction(Controls::UACTN_USE, Controls::isHeld) ||
         pControls->GetUserAction(Controls::UACTN_USE, Controls::isPressed)) &&
        pCurrentProcess->GetWeaponDrawn() &&
        !bReleaseTheArrow) {
        if ((!pWeapon || pWeapon->IsMeleeWeapon() || pCurrentProcess->IsCurrentWeaponGrenade()) &&
            (!pWeapon || pWeapon->GetCurrentAmmo(pPlayer) || !pWeapon->IsAutomaticWeapon()) &&
            TESAnimGroup::IsAttackAction(pAnimation->GetGroup(AGS_WEAPON)) &&
            !TESAnimGroup::IsPowerAttackAction(pAnimation->GetGroup(AGS_WEAPON)) &&
            !inextAttack &&
            !bInVATS) {
            BSAnimGroupSequence* pSequence = pAnimation->GetCurrentSequence(AGS_WEAPON);
            float fTime = pAnimation->GetTime(pSequence);
            if (pCurrentProcess->IsCurrentWeaponGrenade()) {
                if (bUserDebounceBow) {
                    ForceGrenadeHold();
                }
            }
            else if (
                (fTime <= 0.0 || pSequence->GetState() != NiControllerSequence::kAnimating) && !IsOverEncumbered()
            ) {
                pAnimation->SkipUpdate(AGS_WEAPON);
                p1stAnimation->SkipUpdate(AGS_WEAPON);
            }
        }
    }
    else {
        bUserDebounceBow = false;
        if (GetAnimAction() == ANIM_ACTION_NONE) {
            bReleaseTheArrow = false;
        }
    }
    return bResult;
}

void PlayerCharacter::SetAcousticSpaceTransition(bool val) {
    bIsAcousticSpaceTransition = val;
}

bool PlayerCharacter::GetControlsDisabled(PlayerCharacter::DISABLE_PLAYER_CONTROLS aeDPC) {
    return (ucControlsDisabled & aeDPC) != 0;
}

bool PlayerCharacter::IsPlayerCharacterInCombat(bool* abAreAllCombatTargetsSearching) {
    if (abAreAllCombatTargetsSearching) {
        *abAreAllCombatTargetsSearching = bAllCombatTargetsSearching;
    }
    return bPlayerInCombat;
}

GamePlayFormulas::DifficultyLevel PlayerCharacter::GetGameDifficulty() {
    return eDifficultyLevel;
}

KILL_CAM_TYPE PlayerCharacter::GetKillCamMode() {
    return eKillCameraSetting;
}

float PlayerCharacter::GetWorldFOV() {
    return fWorldFOV;
}

float PlayerCharacter::Get1stPersonFOV() {
    return f1stPersonFOV;
}

TESRegion* PlayerCharacter::GetOccupiedRegion() {
    return pOccupiedRegion;
}

TESObjectREFR* PlayerCharacter::GetGrabObject() {
    return pGrabbedObject;
}

NiAVObject* PlayerCharacter::GetWeaponSightingNode() {
    return m_pWeaponSightingNode;
}

void PlayerCharacter::SetWeaponSightingNode(NiAVObject* apSightingNode) {
    m_pWeaponSightingNode = apSightingNode;
}

void PlayerCharacter::SetForceActivateRef(TESObjectREFR* apRef) {
    pForceActivateRef = apRef;
}

void PlayerCharacter::SetMegatonDestroyed(bool abDestroyed) {
    bMegatonDestroyed = abDestroyed;
}

void PlayerCharacter::SetTimeSinceLastAmmoRegenTick(float fTime) {
    fTimeSinceLastAmmoRegenTick = fTime;
}

void PlayerCharacter::SetBlockCounterTimer(float fTime) {
    fCounterAttackTimer = fTime;
}

float PlayerCharacter::GetBlockCounterTimer() {
    return fCounterAttackTimer;
}

void PlayerCharacter::ClearsHardcoreFlag() {
    bAlwaysHardcore = false;
}

// The message shows for 2 seconds with emotion 0, no icon and no sound.
void PlayerCharacter::ShowInsufficientChargeMessage() {
    if (!bInsufficientChargeMessageShown) {
        bInsufficientChargeMessageShown = true;
        Interface::ShowMessage(sNoCharge.String(), 0, nullptr, nullptr, 2.0f, false);
    }
}

void PlayerCharacter::ResetInsufficientChargeMessage() {
    bInsufficientChargeMessageShown = false;
}

void PlayerCharacter::ClearClosestAudioMarkerInfo() {
    pClosestAudioMarkerInfo = nullptr;
}

// Registers the idle directories beside the first-person skeleton (its IdleAnims and the Locomotion ones), then the
// Locomotion ones beside the third-person model. Only the first strrchr is null-checked; later results are copied
// through unchecked. An AutoMemContext lives for the whole function.
void PlayerCharacter::Preload1stPersonAnimsFileList() {
    AutoMemContext context(
        MEM_CONTEXT_PLAYER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
        16837
    );
    char p1stPersonAnimsDirectory[260];
    BSstrcpy(p1stPersonAnimsDirectory, 260, sFirstPersonSkeleton.String());
    char* pslash = std::strrchr(p1stPersonAnimsDirectory, '\\');
    if (pslash) {
        BSstrcpy(pslash + 1, 260 - (pslash - p1stPersonAnimsDirectory) - 1, "IdleAnims");
    }
    pIdleManager->AddRootIdleArray(p1stPersonAnimsDirectory, nullptr, false);
    BSstrcpy(pslash, 260 - (pslash - p1stPersonAnimsDirectory) - 1, "\\Locomotion\\IdleAnims");
    pIdleManager->AddRootIdleArray(p1stPersonAnimsDirectory, nullptr, false);
    BSstrcpy(pslash, 260 - (pslash - p1stPersonAnimsDirectory) - 1, "\\Locomotion\\Hurt\\IdleAnims");
    pIdleManager->AddRootIdleArray(p1stPersonAnimsDirectory, nullptr, false);
    BSstrcpy(pslash, 260 - (pslash - p1stPersonAnimsDirectory) - 1, "\\Locomotion\\Male\\IdleAnims");
    pIdleManager->AddRootIdleArray(p1stPersonAnimsDirectory, nullptr, false);
    BSstrcpy(pslash, 260 - (pslash - p1stPersonAnimsDirectory) - 1, "\\Locomotion\\Female\\IdleAnims");
    pIdleManager->AddRootIdleArray(p1stPersonAnimsDirectory, nullptr, false);
    BSstrcpy(pslash, 260 - (pslash - p1stPersonAnimsDirectory) - 1, "\\Locomotion\\Toddler\\IdleAnims");
    pIdleManager->AddRootIdleArray(p1stPersonAnimsDirectory, nullptr, false);
    BSstrcpy(p1stPersonAnimsDirectory, 260, TESObjectREFR::GetModel());
    pslash = std::strrchr(p1stPersonAnimsDirectory, '\\');
    BSstrcpy(pslash, 260 - (pslash - p1stPersonAnimsDirectory) - 1, "\\Locomotion\\IdleAnims");
    pIdleManager->AddRootIdleArray(p1stPersonAnimsDirectory, nullptr, false);
    BSstrcpy(pslash, 260 - (pslash - p1stPersonAnimsDirectory) - 1, "\\Locomotion\\Hurt\\IdleAnims");
    pIdleManager->AddRootIdleArray(p1stPersonAnimsDirectory, nullptr, false);
    BSstrcpy(pslash, 260 - (pslash - p1stPersonAnimsDirectory) - 1, "\\Locomotion\\Male\\IdleAnims");
    pIdleManager->AddRootIdleArray(p1stPersonAnimsDirectory, nullptr, false);
    BSstrcpy(pslash, 260 - (pslash - p1stPersonAnimsDirectory) - 1, "\\Locomotion\\Female\\IdleAnims");
    pIdleManager->AddRootIdleArray(p1stPersonAnimsDirectory, nullptr, false);
    BSstrcpy(pslash, 260 - (pslash - p1stPersonAnimsDirectory) - 1, "\\Locomotion\\Child\\IdleAnims");
    pIdleManager->AddRootIdleArray(p1stPersonAnimsDirectory, nullptr, false);
}

// Gives the base NPC the default body model of its sex, then queues the first-person skeleton's animations (with the
// toddler idles for a toddler and the idles of the player's sex), the skeleton itself, and the third-person model's
// animations. AutoMemContext as in Preload1stPersonAnimsFileList.
void PlayerCharacter::QueuePlayerAnims(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    AutoMemContext context(
        MEM_CONTEXT_PLAYER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp",
        16895
    );
    TESNPC* pnpc = static_cast<TESNPC*>(GetObjectReference());
    TESModel* pModel = pnpc;
    if (pnpc->GetSex() == TESActorBase::SEX_FEMALE) {
        pModel->SetModel(sNPCModelFemale.String());
    }
    else {
        pModel->SetModel(sNPCModelMale.String());
    }
    BSSimpleList<const char*>*
        pfileList = pModelLoader->BuildKFFileList(sFirstPersonSkeleton.String(), true, false, ANIM_GROUP_WEAPON_COUNT);
    char plocomotionSexDirectory[260];
    BSstrcpy(plocomotionSexDirectory, 260, sFirstPersonSkeleton.String());
    char* pslash = std::strrchr(plocomotionSexDirectory, '\\');
    if (IsPCToddler()) {
        BSstrcpy(pslash, 260 - (pslash - plocomotionSexDirectory) - 1, "\\Locomotion\\Toddler\\IdleAnims");
        pModelLoader->CopyFilenameList(pIdleManager->GetRootFilenameList(plocomotionSexDirectory, AGW_NONE), pfileList);
    }
    if (GetSex() == TESActorBase::SEX_FEMALE) {
        BSstrcpy(pslash, 260 - (pslash - plocomotionSexDirectory) - 1, "\\Locomotion\\Female\\IdleAnims");
    }
    else {
        BSstrcpy(pslash, 260 - (pslash - plocomotionSexDirectory) - 1, "\\Locomotion\\Male\\IdleAnims");
    }
    pModelLoader->CopyFilenameList(pIdleManager->GetRootFilenameList(plocomotionSexDirectory, AGW_NONE), pfileList);
    pModelLoader->QueueKFList(pfileList, aePriority, apParent, nullptr);
    if (pfileList) {
        delete pfileList;
    }
    pModelLoader->QueueModel(sFirstPersonSkeleton.String(), aePriority, apParent, LOD_MULT_NONE, true, false, false);
    pfileList = pModelLoader->BuildKFFileList(TESObjectREFR::GetModel(), true, false, ANIM_GROUP_WEAPON_COUNT);
    pModelLoader->QueueKFList(pfileList, aePriority, apParent, nullptr);
    if (pfileList) {
        delete pfileList;
    }
}

// Recomputes the container weight; turning hardcore off (unless bIgnoreTracking) clears the hardcore flag and,
// outside the initial load, zeroes the dehydration, sleep deprivation and hunger values.
void PlayerCharacter::SetHardcore(GamePlayFormulas::HardcoreSetting aeHardcoreLevel, bool bIgnoreTracking) {
    InventoryChanges* pChanges = GetExtra()->GetContainerChanges();
    if (pChanges) {
        pChanges->ForceWeightRecalculation();
    }
    if (aeHardcoreLevel == GamePlayFormulas::HARDCORE_OFF && !bIgnoreTracking) {
        ClearsHardcoreFlag();
        if (!StartMenu::IsInitialLoading()) {
            // Each negated value is rounded to a float argument before the call.
            ActorValueOwner* pOwner = this;
            float fDehydration = -pOwner->GetActorFloatValue(ActorValue::DEHYDRATION);
            PermanentModActorValue(ActorValue::DEHYDRATION, fDehydration, nullptr);
            float fSleepDeprivation = -pOwner->GetActorFloatValue(ActorValue::SLEEPDEPREVATION);
            PermanentModActorValue(ActorValue::SLEEPDEPREVATION, fSleepDeprivation, nullptr);
            float fHunger = -pOwner->GetActorFloatValue(ActorValue::HUNGER);
            PermanentModActorValue(ActorValue::HUNGER, fHunger, nullptr);
        }
    }
    eHardcoreSetting = aeHardcoreLevel;
}

BipedAnim* PlayerCharacter::GetBiped(bool abFirstPerson) {
    if (abFirstPerson) {
        return p1stPersonBipedAnim;
    }
    return Character::GetBiped();
}

bool PlayerCharacter::Is1stPersonBiped(BipedAnim* apBiped) {
    return p1stPersonBipedAnim && p1stPersonBipedAnim == apBiped;
}

Animation* PlayerCharacter::GetAnimation(bool abFirstPerson) {
    if (abFirstPerson) {
        return p1stPersonAnimation;
    }
    return Actor::GetAnimation();
}

NiAVObject* PlayerCharacter::Get3D(bool abFirstPerson) {
    if (abFirstPerson) {
        return sp1stPerson3D;
    }

    return TESObjectREFR::Get3D();
}

void PlayerCharacter::ResetShapeCasterInfo() {
    int32_t filterGroup;
    bhkCharacterController* pCharController = MobileObject::GetCharController();
    if (pCharController) {
        filterGroup = static_cast<int32_t>(pCharController->GetFilterGroup());
        pCameraCaster->SetPlayerGroup(filterGroup);
    }
}

TESRegionList* PlayerCharacter::GetAllOccupiedRegions() {
    return &m_AllOccupiedRegions;
}

void PlayerCharacter::SetCanFastTravel(bool abflag) {
    if (abflag) {
        bCanFastTravel |= 1;
    }
    else {
        bCanFastTravel &= ~1;
    }
}

bool PlayerCharacter::GetIsDebounceWASD() {
    return (bCanFastTravel & 4) != 0;
}

uint32_t PlayerCharacter::GetCaravanWinnings() {
    return iCaravanWinnings;
}

bool PlayerCharacter::CasinoAntiCheatCheck() {
    if (bAntiCheatActive && GetCasinoCheatTimeRemaining() > 0) {
        return true;
    }
    resetAntiCheatTimer();
    bAntiCheatActive = 0;
    return false;
}

int32_t PlayerCharacter::GetCasinoCheatTimeRemaining() {
    uint32_t uiCheatEndTime = iAntiCheatDuration.UInt() + uiAntiCheatTimer;
    return uiCheatEndTime - GetTickCount() / 1000;
}

void PlayerCharacter::ResetLastPlayingTimeUpdate() {
    iLastPlayingTimeUpdate = BSGetTickCount();
}

// Frees every entry, then empties the list.
void PlayerCharacter::ClearAudioMarkerList() {
    for (BSSimpleList<AudioMarkerInfo*>* pIter = &AudioMarkerList; pIter; pIter = pIter->GetNext()) {
        ourFree(pIter->GetItem());
    }
    AudioMarkerList.RemoveAll();
    pClosestAudioMarkerInfo = nullptr;
}

// Rebuilds the entries from the worldspace's audio markers, or from the parent cell's in an interior. A marker without
// marker data is dropped. The temporary list is emptied with RemoveAll and then deleted, which runs RemoveAll again
// before ourFree.
void PlayerCharacter::ResetAudioMarkerList() {
    TESWorldSpace* pWorldSpace = GetWorldSpace();
    ClearAudioMarkerList();
    if (pWorldSpace) {
        BSSimpleList<TESObjectREFR*>* pList = pWorldSpace->BuildAudioMarkerList(true);
        for (BSSimpleList<TESObjectREFR*>* pIter = pList; pIter; pIter = pIter->GetNext()) {
            TESObjectREFR* pRef = pIter->GetItem();
            if (pRef) {
                AudioMarkerInfo* pInfo = new AudioMarkerInfo(pRef);
                if (pInfo->pMarkerData) {
                    AudioMarkerList.AddHead(pInfo);
                }
                else {
                    ourFree(pInfo);
                }
            }
        }
        pList->RemoveAll();
        if (pList) {
            delete pList;
        }
    }
    else {
        BSSimpleList<TESObjectREFR*>* pList = new BSSimpleList<TESObjectREFR*>;
        TESObjectCELL* pCell = GetParentCell();
        if (pCell) {
            pCell->AddAudioMarkersToList(pList);
        }
        for (BSSimpleList<TESObjectREFR*>* pIter = pList; pIter; pIter = pIter->GetNext()) {
            TESObjectREFR* pRef = pIter->GetItem();
            if (pRef) {
                AudioMarkerInfo* pInfo = new AudioMarkerInfo(pRef);
                if (pInfo->pMarkerData) {
                    AudioMarkerList.AddHead(pInfo);
                }
                else {
                    ourFree(pInfo);
                }
            }
        }
        pList->RemoveAll();
        if (pList) {
            delete pList;
        }
    }
}

void PlayerCharacter::ClearMapMarkerStructList() {
    for (BSSimpleList<PlayerCharacter::MapMarkerInfo*>* pIter = &MapMarkerList; pIter; pIter = pIter->m_pkNext) {
        ourFree(pIter->m_item);
    }
    MapMarkerList.RemoveAll();
    pMapWorld = nullptr;
}

// Rebuilds the map marker entries when the player changed worldspace. A marker without marker data is dropped; the
// temporary list goes as in ResetAudioMarkerList.
void PlayerCharacter::ResetMapMarkerStructList() {
    TESWorldSpace* pWorldSpace = GetWorldSpace();
    if (pMapWorld == pWorldSpace) {
        return;
    }
    ClearMapMarkerStructList();
    if (pWorldSpace) {
        BSSimpleList<TESObjectREFR*>* pList = pWorldSpace->BuildMapMarkerList(true);
        for (BSSimpleList<TESObjectREFR*>* pIter = pList; pIter; pIter = pIter->GetNext()) {
            TESObjectREFR* pRef = pIter->GetItem();
            if (pRef) {
                MapMarkerInfo* pInfo = static_cast<MapMarkerInfo*>(ourMalloc(sizeof(MapMarkerInfo)));
                pInfo->pMarkerData = pRef->GetMapMarkerData();
                pInfo->pMarkerRef = pRef;
                if (pInfo->pMarkerData) {
                    MapMarkerList.AddHead(pInfo);
                }
                else {
                    ourFree(pInfo);
                }
            }
        }
        pList->RemoveAll();
        if (pList) {
            delete pList;
        }
    }
    pMapWorld = pWorldSpace;
}

// Finishes a view switch once the zoom has come within 30 units (a NaN zoom finishes it too).
void PlayerCharacter::UpdateFirstPersonZoom() {
    if (bWant3rdPerson == b3rdPerson) {
        return;
    }
    if (fZoomDelta > 30.0) {
        return;
    }
    b3rdPerson = bWant3rdPerson;
    if (!bVanityMode && sp1stPerson3D && !b3rdPerson) {
        Show1stPerson(!b3rdPerson);
    }
}

void PlayerCharacter::Show1stPerson(bool abFirstPerson) {
    float fVal;
    if (Get3D(true) && Get3D(false) && (!abFirstPerson || (!IsDead(false) && !bTemp3rdPerson))) {
        int32_t i3rdPersonCulled = Get3D(false)->GetAppCulled();
        if (i3rdPersonCulled != Get3D(true)->GetAppCulled()) {
            if (!abFirstPerson) {
                if (!Get3D(false)->GetAppCulled()) {
                    return;
                }
            }
            if (abFirstPerson) {
                if (!Get3D(true)->GetAppCulled()) {
                    bActually3rdPerson = false;
                    return;
                }
            }
        }
        Get3D(false)->SetAppCulled(abFirstPerson);
        if (abFirstPerson &&
            bTemp1stPerson &&
            GetIronSights() &&
            pCurrentProcess->GetCurrentWeapon() &&
            static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj)
                ->GetScopeModel()
                ->GetModelLength()) {
            TESObjectWEAP* pweap = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
            fVal = 0.0f;
            if (!pweap->HasModScope() ||
                pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_IRON_SITES, fVal)) {
                Interface::SetGunScopeVisible(true);
            }
        }
        else {
            Get3D(true)->SetAppCulled(!abFirstPerson);
        }
        bActually3rdPerson = !abFirstPerson;
        if (abFirstPerson) {
            NiMatrix3 rot;
            NiAVObject* p3dObj = Get3D(false);
            p3dObj->SetTranslate(GetLocationOnReference());
            rot.MakeZRotation(GetHeading(false));
            p3dObj->SetRotate(MultipleMatrixByRace(rot));
            fPlayerCameraAlpha = 1.0f;
            UpdateAlpha();
            Animation* panim = GetAnimation(false);
            if (panim) {
                panim->UpdateMovement(this);
            }
            else {
                NiUpdateData UpdateData(0.0f, false, false);
                p3dObj->Update(UpdateData);
            }
            BSSoundHandle weaponSoundHandle;
            m_Extra.GetWeaponIdleSound(weaponSoundHandle);
            if (weaponSoundHandle.IsValid()) {
                weaponSoundHandle.SetFollowsListener(true);
            }
            m_Extra.GetWeaponAttackSound(weaponSoundHandle);
            if (weaponSoundHandle.IsValid()) {
                weaponSoundHandle.SetFollowsListener(true);
            }
        }
        else {
            BSSoundHandle weaponSoundHandle;
            m_Extra.GetWeaponIdleSound(weaponSoundHandle);
            if (weaponSoundHandle.IsValid()) {
                weaponSoundHandle.SetFollowsListener(false);
            }
            m_Extra.GetWeaponAttackSound(weaponSoundHandle);
            if (weaponSoundHandle.IsValid()) {
                weaponSoundHandle.SetFollowsListener(false);
            }
        }
        SetFireNode(nullptr);
        if (GetShouldCheckMagicNode()) {
            Animation* panimFrom = GetAnimation(!abFirstPerson);
            Animation* panimTo = GetAnimation(abFirstPerson);
            NiAVObject* pobj = panimFrom->GetControllerManager()->GetObjectPalette()->GetAVObject(
                FixedStrings::QMagicNode()
            );
            NiNode* pnodeFrom = pobj ? pobj->IsNode() : nullptr;
            NiDefaultAVObjectPalette* pPaletteTo = static_cast<NiDefaultAVObjectPalette*>(
                panimTo->GetControllerManager()->GetObjectPalette()
            );
            pobj = pPaletteTo->GetAVObject(FixedStrings::QMagicNode());
            NiNode* pnodeTo = pobj ? pobj->IsNode() : nullptr;
            if (pnodeFrom && pnodeTo) {
                for (uint32_t i = 0; i < pnodeFrom->GetArrayCount(); ++i) {
                    NiPointer<NiAVObject> pchild(static_cast<NiAVObject*>(nullptr));
                    pnodeFrom->DetachChildAt(i, pchild);
                    if (pchild != static_cast<NiAVObject*>(nullptr)) {
                        pnodeTo->AttachChild(pchild.m_pObject, 1);
                    }
                }
            }
        }
        NiNode* pnode = Get3D();
        if (abFirstPerson) {
            pnode = static_cast<NiNode*>(pnode->GetAt(0));
        }
        InitLightingPropertyPtr(pnode);
        pCurrentProcess->SetShouldRefreshMagicShadersEffects(true, true, false);
    }
}

float PlayerCharacter::SetFOV(float afFOV) {
    fFOV = afFOV;
    float fCameraFOV = fFOV;
    Main::GetWorldSceneGraph()->SetCameraFOV(fCameraFOV, false, nullptr, false);
    BSShaderManager::SetFOV(afFOV);
    return fFOV;
}

// A topic already in the list is not added again; abDisplayMessage is unused.
bool PlayerCharacter::AddTopic(TESTopic* apNewTopic, bool abResort, bool abDisplayMessage) {
    if (!apNewTopic) {
        return false;
    }
    for (BSSimpleList<TESTopic*>* pIter = GetTopicList(); pIter && pIter->GetItem(); pIter = pIter->GetNext()) {
        if (pIter->GetItem() == apNewTopic) {
            return false;
        }
    }
    listTopics.AddHead(apNewTopic);
    if (abResort) {
        TESTopic::SortTopics(true, GetTopicList());
    }
    return true;
}

BSSimpleList<TESQuestTarget*>* PlayerCharacter::GetCurrentTargetList() {
    if (!pActiveQuest) {
        return nullptr;
    }
    BSSimpleList<BGSQuestObjective*>* pObjectives = &listObjectives;
    if (bForceQuestTargetUpdate || !pActiveQuest->ValidateTargetList(&listQuestTargets, pObjectives)) {
        pActiveQuest->UpdateCurrentTargetList(&listQuestTargets, pObjectives);
        bForceQuestTargetUpdate = false;
    }
    return &listQuestTargets;
}

// A live, enabled, free actor on a follow or accompany package targeting the player, and not waiting.
bool PlayerCharacter::IsActorFollowingPlayer(Actor* apActor) {
    bool bResult = false;
    if (apActor->IsDead(false) || apActor->GetDisabled() || apActor->IsParalyzed() || apActor->IsRestrained()) {
        return bResult;
    }
    TESPackage* pPackage = apActor->GetPackageSetAsPcurrent();
    if (!pPackage) {
        return bResult;
    }
    if (pPackage->GetPackType() != PACKAGE_TYPE_FOLLOW && pPackage->GetPackType() != PACKAGE_TYPE_ACCOMPANY) {
        return bResult;
    }
    BaseProcess* pProcess = apActor->GetCurrentAIProcess();
    if (pProcess->GetTarget() == pPlayer && !apActor->IsWaiting()) {
        bResult = true;
    }
    return bResult;
}

void PlayerCharacter::RemoveConflictingObject(TESObjectREFR* apRef) {
    if (apRef) {
        ConflictingObjects.Remove(apRef);
    }
}

// Removes and frees the first action of that kind; a null apActObject matches any object. The node holding the
// action does the removal.
void PlayerCharacter::RemovePlayerAction(PLAYER_ACTION aeAction, TESObjectREFR* apActObject) {
    if (!pListofActions) {
        return;
    }
    BSSimpleList<PlayerActionObject*>* pIter = pListofActions;
    PlayerActionObject* pAction = nullptr;
    while (pIter && pIter->GetItem()) {
        pAction = pIter->GetItem();
        if (pAction && pAction->eAction == aeAction && (!apActObject || apActObject == pAction->prefObj)) {
            pIter->Remove(pAction);
            ourFree(pAction);
            return;
        }
        pIter = pIter->GetNext();
    }
}

void PlayerCharacter::AddPlayerAction(PLAYER_ACTION aeAction, float afTime, TESObjectREFR* apActObject) {
    bool bsearch;
    bsearch = false;
    if (!IsPlayerCharacterInCombat(&bsearch)) {
        PlayerActionObject* pobject = nullptr;
        if (!pListofActions) {
            pListofActions = new BSSimpleList<PlayerActionObject*>;
        }
        BSSimpleList<PlayerActionObject*>* plist = pListofActions;
        while (plist && plist->m_item && !pobject) {
            PlayerActionObject* pAction = plist->m_item;
            if (pAction && pAction->eAction == aeAction) {
                pobject = pAction;
            }

            plist = plist->m_pkNext;
        }
        if (pobject) {
            pListofActions->Remove(pobject);
        }
        else {
            pobject = new PlayerActionObject;
        }
        pobject->eAction = aeAction;
        pobject->ftimer = afTime;
        if (apActObject) {
            apActObject->SetTargeted(true);
        }
        pobject->prefObj = apActObject;
        pListofActions->AddHead(pobject);
    }
}

void PlayerCharacter::UpdateImageSpaceIronsights() {
    if (!pIronsightsDOFInstance) {
        float fRange = fIronSightsDOFRange.Float();
        float fDistance = fIronSightsDOFDistance.Float();
        pIronsightsDOFInstance = ImageSpaceModifierInstanceDOF::Trigger(
            fDistance,
            fRange,
            ImageSpaceModifierInstanceDOF::DOF_FRONT_MASK,
            0.0f,
            0.0f,
            0.0f
        );
        if (pIronsightsDOFInstance) {
            pIronsightsDOFInstance->SetPermanent(true);
        }
    }
    float fStrengthCap = fIronSightsDOFStrengthCap.Float();
    float fTargetStrength = 0.0f;
    float fStrength = pIronsightsDOFInstance->QStrength();
    if (GetIronSights() && !b3rdPerson) {
        // The motion blur setting is read and never used.
        fIronSightsGunMotionBlur.Float();
        fTargetStrength = fIronSightsDOFStrengthCap.Float();
        BSShaderManager::SetFullScreenMotionBlurIronSights(true);
    }
    else {
        BSShaderManager::SetFullScreenMotionBlurIronSights(false);
    }
    if (fStrength != fTargetStrength) {
        // Keeps the rate as a double and rounds the step to a float.
        double dRate = 1.0 / fIronSightsDOFSwitchSeconds.Float();
        float fStep = static_cast<float>(appTimer.GetSeconds() * dRate);
        if (fStrength < fTargetStrength) {
            fStrength = fStrength + fStep;
        }
        else {
            fStrength = fStrength - fStep;
        }
        BSClamp(fStrength, 0.0f, fStrengthCap);
    }
    pIronsightsDOFInstance->SetStrength(fStrength);
    float fMaskConst = fIronSightsGunMotionBlur;
    pIronsightsDOFInstance->SetMaskConst(fMaskConst);
}

void PlayerCharacter::UpdateImageSpaceVATS() {
    float fDistance = 0.0f;
    if (!pVatsDOFInstance) {
        float fRange = fVATSDOFRange.Float();
        pVatsDOFInstance = ImageSpaceModifierInstanceDOF::Trigger(
            fDistance,
            fRange,
            ImageSpaceModifierInstanceDOF::DOF_FRONT_BACK,
            0.0f,
            0.0f,
            0.0f
        );
        if (pVatsDOFInstance) {
            pVatsDOFInstance->SetPermanent(true);
        }
    }
    if (!pVatsDRBInstance) {
        NiPoint2 Center(0.5f, 0.5f);
        float fStart = fVATSRadialStart.Float();
        float fRampup = fVATSRadialRampup.Float();
        float fRange = fVATSDOFRange.Float();
        pVatsDRBInstance = ImageSpaceModifierInstanceDRB::Trigger(
            fDistance,
            fRange,
            ImageSpaceModifierInstanceDRB::DOF_FRONT_BACK,
            0.0f,
            fRampup,
            fStart,
            0.0f,
            0.0f,
            Center,
            0.0f
        );
        if (pVatsDRBInstance) {
            pVatsDRBInstance->SetPermanent(true);
        }
    }
    float fRadialStrengthCap = fVATSRadialStrength.Float();
    float fDOFStrengthCap = fVATSDOFStrengthCap.Float();
    float fDOFTarget = 0.0f;
    float fRadialTarget = 0.0f;
    float fDOFStrength = pVatsDOFInstance->QStrength();
    float fRadialStrength = pVatsDRBInstance->QStrength();
    if ((pVATS.GetMode() == VATS::VATS_MENU_MODE || pVATS.GetMode() == VATS::VATS_SCAN_MODE) && pVATSTarget) {
        fRadialTarget = fRadialStrengthCap;
        fDOFTarget = fDOFStrengthCap;
        const NiPoint3* pPlayerLocation = &GetLocationOnReference();
        fDistance = (pVATSTarget->GetLocationOnReference() - *pPlayerLocation).Length();
    }
    if (fDOFStrength != fDOFTarget) {
        // Keeps the rate as a double and rounds the step to a float.
        double dRate = 1.0 / fVATSDOFSwitchSeconds.Float();
        float fStep = static_cast<float>(appTimer.GetSeconds() * dRate);
        if (fDOFStrength < fDOFTarget) {
            fDOFStrength = fDOFStrength + fStep;
        }
        else {
            fDOFStrength = fDOFStrength - fStep;
        }
        BSClamp(fDOFStrength, 0.0f, fDOFStrengthCap);
    }
    if (fRadialStrength != fRadialTarget) {
        double dRate = 1.0 / fVATSDOFSwitchSeconds.Float();
        float fStep = static_cast<float>(appTimer.GetSeconds() * dRate);
        if (fRadialStrength < fRadialTarget) {
            fRadialStrength = fRadialStrength + fStep;
        }
        else {
            fRadialStrength = fRadialStrength - fStep;
        }
        BSClamp(fRadialStrength, 0.0f, fRadialStrengthCap);
    }
    pVatsDOFInstance->SetStrength(fDOFStrength);
    pVatsDOFInstance->SetDistance(fDistance);
    pVatsDRBInstance->SetStrength(fRadialStrength);
    pVatsDRBInstance->SetDistance(fDistance);
}

TESObjectREFR* PlayerCharacter::QPlayerMarkerTrackingRef() {
    if (PlayerMarkerPath.Doors.GetSize()) {
        return PlayerMarkerPath.Doors.GetAt(0).pDoor;
    }
    return pPlayerMapMarker;
}

// The marker is created on first use; its cell is apForm itself when apForm is a cell and the world space's persistent
// cell when it is a world space.
void PlayerCharacter::SetPlayerMapMarker(NiPoint3 arPoint, TESForm* apForm) {
    if (!pPlayerMapMarker) {
        pPlayerMapMarker = new TESObjectREFR;
        pPlayerMapMarker->SetObjectReference(static_cast<TESBoundObject*>(pXMarkerHeading));
    }
    pPlayerMapMarker->SetLocationOnReferenceSimple(arPoint);
    if (apForm->GetFormType() == CELL_ID) {
        pPlayerMapMarker->SetParentCellSimple(static_cast<TESObjectCELL*>(apForm));
    }
    else if (apForm->GetFormType() == WRLD_ID) {
        TESObjectCELL* pCell = static_cast<TESWorldSpace*>(apForm)->QPersistentCell();
        if (pCell) {
            pPlayerMapMarker->SetParentCellSimple(pCell);
        }
    }
    BuildPathToTarget(pPlayerMapMarker, &PlayerMarkerPath, false);
}

bool PlayerCharacter::GetPlayerGreetFlag() {
    return bGreetingPlayer;
}

bool PlayerCharacter::HasNote(BGSNote* apNote) {
    return apNote && listNotes.IsInList(apNote);
}

void PlayerCharacter::RemoveNote(BGSNote* apNote) {
    MapMenu::NoteRemoved(apNote);
    listNotes.Remove(apNote);
}

// The arms follow the camera's pitch by fFirstPersonHandFollowMult, and fully while equipping or attacking (but not in
// the attack's follow-through), in the pip-boy, or in iron sights; the follow eases toward that target over the chase
// seconds.
void PlayerCharacter::UpdateArmRotation() {
    float fpercent = fFirstPersonHandFollowMult.Float();
    Animation* panim = GetAnimation(false);
    if (!panim) {
        return;
    }
    uint16_t sgroup = panim->GetGroup(AGS_WEAPON);
    int32_t egroup = TESAnimGroup::GetType(sgroup);
    int32_t eact = GetAnimAction();
    if (eact != ANIM_ACTION_ATTACK_FOLLOW_THROUGH &&
        egroup >= ANIM_GROUP_WEAPONS_START &&
        egroup <= ANIM_GROUP_WEAPONS_STOP) {
        fpercent = 1.0f;
    }
    else if (eact != ANIM_ACTION_ATTACK_FOLLOW_THROUGH && IsPipboyActive()) {
        fpercent = 1.0f;
    }
    else if (GetIronSights()) {
        fpercent = 1.0f;
    }
    if (fpercent != fchasePercent) {
        float fchaseTime = fFirstPersonHandChaseSeconds.Float();
        if (fpercent == 1.0) {
            fchaseTime = fFirstPersonHandChaseSecondsAttack.Float();
        }
        float fchaseDelta = appTimer.GetSeconds() / fchaseTime * (fpercent - fchasePercent);
        fchasePercent += fchaseDelta;
        if (fchaseDelta < 0.0 && fpercent > fchasePercent) {
            fchasePercent = fpercent;
        }
        else if (fchaseDelta > 0.0 && fpercent < fchasePercent) {
            fchasePercent = fpercent;
        }
        fpercent = fchasePercent;
    }
    NiMatrix3 rot;
    NiMatrix3 cameraRot;
    float fheading = GetHeading(false);
    if (p1stPersonLooking) {
        rot.MakeXRotation(GetLooking());
    }
    else {
        rot.MakeZRotation(fheading);
        cameraRot.MakeXRotation(GetLooking());
        rot = rot * cameraRot;
    }
    NiPoint3 p1(0.0f, 1.0f, 0.0f);
    NiPoint3 p2;
    p1 = rot * p1;
    p2 = p1;
    p2.z = 0.0f;
    float dot = p1.Dot(p2);
    if (dot < -1.0) {
        dot = -1.0f;
    }
    if (dot > 1.0) {
        dot = 1.0f;
    }
    f1stPersonArmRot = std::acosf(dot);
    f1stPersonArmRot = (1.0f - fpercent) * f1stPersonArmRot;
    if (p2.z > p1.z) {
        f1stPersonArmRot = f1stPersonArmRot * -1.0;
    }
    NiMatrix3 xrot;
    NiMatrix3 rotMod;
    xrot.MakeXRotation(f1stPersonArmRot);
    rotMod = rot;
    rotMod = rotMod * xrot;
    if (p1stPersonLooking) {
        p1stPersonLooking->SetRotate(rotMod);
        rot.MakeZRotation(fheading);
    }
    sp1stPerson3D->SetRotate(rot);
}

// The player moving rebuilds the path to its map marker. The walk stops as soon as an update is forced.
void PlayerCharacter::CheckForQuestTargetUpdate(TESObjectREFR* apMovingRef) {
    if (apMovingRef == pPlayer) {
        if (pPlayerMapMarker) {
            BuildPathToTarget(pPlayerMapMarker, &PlayerMarkerPath, false);
        }
        bForceQuestTargetUpdate = true;
    }
    BSSimpleList<TESQuestTarget*>* pList = GetCurrentTargetList();
    TESQuestTarget* pTarget = nullptr;
    while (pList && pList->GetItem() && !bForceQuestTargetUpdate) {
        pTarget = pList->GetItem();
        pList = pList->GetNext();
        TESObjectREFR* ptargref = pTarget->GetReference(true);
        if (ptargref == apMovingRef) {
            bForceQuestTargetUpdate = true;
        }
        if (ptargref && ptargref->HasContainer()) {
            TESContainer* pCont = ptargref->HasContainer();
            if (pCont) {
                InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(ptargref);
                if (pinventory && pinventory->HasObjectId(ptargref->GetFormID())) {
                    bForceQuestTargetUpdate = true;
                }
            }
        }
    }
}

void PlayerCharacter::BuildPathToTarget(TESObjectREFR* apTargetRef, TeleportPath* apOutResultPath, bool abIgnoreLocks) {
    if (apTargetRef && apOutResultPath) {
        // Destroyed in reverse order: the path, the marker's location, then the player's.
        PathingLocation kPlayerPathing(pPlayer);
        PathingLocation kMarkerPathing(apTargetRef);
        TeleportPath kPath;
        ActorLockData playerLockData(pPlayer);
        apOutResultPath->Clear();
        if (!Pathing::BuildLowPath(
                kPlayerPathing,
                kMarkerPathing,
                apOutResultPath,
                &playerLockData,
                Pathing::DOOR_LOCK_DISPOSITION_LOCKS_ARE_MINIMAL_USE
            )) {
            apOutResultPath->Clear();
        }
    }
}

// Under kPerceivedActorsLock (see PlayerCharacter.h).
void PlayerCharacter::ChangePerceivedActorHostileStatus(Actor* apActor, bool abHostile) {
    kPerceivedActorsLock.Lock(nullptr);
    for (CompassTarget* pact : pListofPercievedActors->ItemsUntilNull()) {
        if (pact->pTarget == apActor) {
            pact->bIsHostile = abHostile;
            break;
        }
    }
    kPerceivedActorsLock.Unlock();
}

bool PlayerCharacter::IsImportantConversationRunning() {
    return pAIConversationRunning && pAIConversationRunning->pTalkingActor;
}

void PlayerCharacter::ResetRockItAmmoWeight(bool abRecalculateWeight) {
    fRockItLauncherAmmoWeight = 0.0f;
    if (abRecalculateWeight) {
        for (uint32_t i = 0; i < RockItLauncherAmmoList.GetSize(); ++i) {
            // These are called in this order: IsHardcore, the item's object, the weight, then the count.
            bool bIsHardcore = IsHardcore();
            TESBoundObject* pObject = RockItLauncherAmmoList.GetAt(i)->GetContainerObject();
            double dFormWeight = TESWeightForm::GetFormWeight(pObject, bIsHardcore);
            int32_t iCount = RockItLauncherAmmoList.GetAt(i)->GetCount();
            fRockItLauncherAmmoWeight = iCount * dFormWeight + fRockItLauncherAmmoWeight;
        }
    }
}

// Under kPerceivedActorsLock (see PlayerCharacter.h).
void PlayerCharacter::FreeCompassTargets() {
    kPerceivedActorsLock.Lock(nullptr);
    if (this->pListofPercievedActors) {
        while (this->pListofPercievedActors && this->pListofPercievedActors->m_item) {
            ourFree(this->pListofPercievedActors->m_item);
            this->pListofPercievedActors->RemoveHead();
        }
        BSSimpleList<CompassTarget*>* pListofPercievedActors = this->pListofPercievedActors;
        if (pListofPercievedActors) {
            delete pListofPercievedActors;
        }
        this->pListofPercievedActors = nullptr;
    }
    kPerceivedActorsLock.Unlock();
}

// Toddler mode swaps the first-person skeleton for the child one, so the animation is replaced with it.
void PlayerCharacter::SetPCToddler(bool abVal) {
    if (bIsToddler != abVal) {
        bIsToddler = abVal;
        // The arguments are built right to left: the null queued file, the skeleton model, the animation.
        char* pModelFileName = NavMeshInfo::GetCellOrCoords(const_cast<char*>(sFirstPersonSkeleton.String()), 0);
        Animation* pAnimation = this->GetAnimation(true);
        this->QueueReplacementLocomotion(false, pAnimation, pModelFileName, nullptr);
    }
}

void PlayerCharacter::CastEatDrinkItems() {
    EatDrinkItems->RemoveAll();
}

void PlayerCharacter::CastQueuedEnchantments() {
    if (QueuedWornEnchantments) {
        for (BSSimpleList<TESBoundObject*>* plist = QueuedWornEnchantments; plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            if (plist->m_item) {
                TESEnchantableForm* penchantable = dynamic_cast<TESEnchantableForm*>(plist->m_item);
                EnchantmentItem* pEnchantment = penchantable ? penchantable->pFormEnchanting : nullptr;
                if (pEnchantment) {
                    CastWornEnchantment(pEnchantment, plist->m_item, false);
                }
            }
        }
        QueuedWornEnchantments->RemoveAll();
        if (QueuedWornEnchantments) {
            delete QueuedWornEnchantments;
        }
        QueuedWornEnchantments = nullptr;
    }
}

void PlayerCharacter::SetLightPointer(NiLight* apLight, bool abFirstPerson) {
    if (abFirstPerson) {
        sp1stPersonLight = apLight;
    }
    else {
        sp3rdPersonLight = apLight;
    }
}

uint8_t PlayerCharacter::GetLastSpaceForDoor(TESObjectDOOR* apDoor) {
    uint8_t cSpaceIndex = 0xFF;
    if (apDoor) {
        RandomDoorSpaceMap.GetAt(apDoor->GetFormID(), cSpaceIndex);
    }
    return cSpaceIndex;
}

void PlayerCharacter::SetLastSpaceForDoor(TESObjectDOOR* apDoor, uint8_t acSpaceIndex) {
    if (apDoor) {
        RandomDoorSpaceMap.SetAt(apDoor->GetFormID(), acSpaceIndex);
    }
}

// Sets the mode on the global player's mover, not on this.
void PlayerCharacter::SetMoveMode(uint32_t aiMoveMode, bool abValue) {
    static_cast<PlayerMover*>(pPlayer->pActorMover)->SetMoveMode(aiMoveMode, abValue);
    if ((aiMoveMode & 0x400) &&
        GetAnimation(false) &&
        TESAnimGroup::GetType(GetAnimation()->GetGroup(AGS_WEAPON)) == ANIM_GROUP_AIM) {
        GetAnimation(false)->BlendOut(AGS_WEAPON, false);
    }
}

bool PlayerCharacter::IsStandingOnFurniture() {
    bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
    if (pController) {
        bhkRigidBody* pSupportBody = pController->GetSupportBody();
        bhkCollisionObject* pCollisionObject = pSupportBody ? bhkCollisionObject::Getbhk(**pSupportBody) : nullptr;
        NiAVObject* pObject = pCollisionObject ? pCollisionObject->GetSceneGraphObject() : nullptr;
        TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pObject);
        if (pRef && pRef->IsFurniture()) {
            return true;
        }
    }
    return false;
}

TESObjectREFR* PlayerCharacter::GetBumpedRef() {
    bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
    if (!pController) {
        return nullptr;
    }
    if (pController->GetSupportBody() == pController->GetBumpedBody()) {
        return nullptr;
    }
    bhkRigidBody* pBumpedBody = pController->GetBumpedBody();
    bhkCollisionObject* pCollisionObject = pBumpedBody ? bhkCollisionObject::Getbhk(**pBumpedBody) : nullptr;
    NiAVObject* pObject = pCollisionObject ? pCollisionObject->GetSceneGraphObject() : nullptr;
    if (pObject) {
        return TESObjectREFR::FindReferenceFor3D(pObject);
    }
    return nullptr;
}

// Fetches the process three times before the first delta.
void PlayerCharacter::ReportRadiationChange() {
    BaseProcess* pProcess = GetCurrentAIProcess();
    BaseProcess* pWaterProcess = GetCurrentAIProcess();
    BaseProcess* pMagicProcess = GetCurrentAIProcess();
    float fStandinRads = pWaterProcess->GetRadiationWaterDelta() + pProcess->GetRadiationDelta();
    HUDMainMenu::SetRadiationLevel(pMagicProcess->GetRadiationMagicDelta(), fStandinRads);
}

void PlayerCharacter::ClearRockItAmmo() {
    for (uint32_t i = 0; i < RockItLauncherAmmoList.GetSize(); ++i) {
        RockItLauncherAmmoList.GetAt(i)->DeleteAllExtra();
    }
    RockItLauncherAmmoList.Clear(true);
    ResetRockItAmmoWeight(false);
}

int8_t PlayerCharacter::GetShotsSinceLastAmmoRegenTick() {
    return cShotsSinceLastAmmoRegen;
}

bool PlayerCharacter::GetCateyeActive() {
    return bHasCateyeActive;
}

// The player avoids other actors only while no AI control is on.
void PlayerCharacter::SetAIControlledToPosition(bool abTF) {
    bAiControlledToPos = abTF;
    if (GetUnderAIControl()) {
        SetAvoidanceDisabled(false);
    }
    else {
        SetAvoidanceDisabled(true);
    }
}

void PlayerCharacter::SetAIControlledFromPosition(bool abTF) {
    bAiControlledFromPos = abTF;
    if (GetUnderAIControl()) {
        SetAvoidanceDisabled(false);
    }
    else {
        SetAvoidanceDisabled(true);
    }
}

float PlayerCharacter::GetModifier(ModifierGroup eGroup, ActorValue::Index aiWhich) {
    float fValue = 0.0f;
    switch (eGroup) {
        case TEMPORARY: fValue = TemporaryActorValueModifiers[aiWhich]; break;
        case SCRIPTS: fValue = ScriptActorValueModifiers[aiWhich]; break;
        case DAMAGE:
            if (aiWhich == ActorValue::HEALTH) {
                fValue = fHealthModifier;
            }
            else {
                fValue = DamageActorValueModifiers[aiWhich];
            }
            break;
    }
    return fValue;
}

// Adds afValue to the modifier of aiWhich in group eGroup, clamped by aeClampStyle; -1 is no actor value. A damage
// change is first clamped against the actor's value; health keeps its damage in fHealthModifier.
void PlayerCharacter::AlterModifier(
    ModifierGroup eGroup,
    ActorValue::Index aiWhich,
    float afValue,
    ModifierList::ClampStyle aeClampStyle
) {
    switch (eGroup) {
        case TEMPORARY:
            if (aiWhich != -1) {
                TemporaryActorValueModifiers[aiWhich] = ModifierList::ModifyAndClamp(
                    TemporaryActorValueModifiers[aiWhich],
                    afValue,
                    aeClampStyle
                );
            }
            break;
        case SCRIPTS:
            if (aiWhich != -1) {
                ScriptActorValueModifiers[aiWhich] = ModifierList::ModifyAndClamp(
                    ScriptActorValueModifiers[aiWhich],
                    afValue,
                    aeClampStyle
                );
            }
            break;
        case DAMAGE: {
            float fDamage = Actor::CheckClampDamageModifier(static_cast<ActorValueOwner*>(this), aiWhich, afValue);
            if (aiWhich == ActorValue::HEALTH) {
                fHealthModifier = ModifierList::ModifyAndClamp(fHealthModifier, fDamage, aeClampStyle);
            }
            else if (aiWhich != -1) {
                DamageActorValueModifiers[aiWhich] = ModifierList::ModifyAndClamp(
                    DamageActorValueModifiers[aiWhich],
                    fDamage,
                    aeClampStyle
                );
            }
            break;
        }
    }
}

// Switches to the first-person view for iron sights or for a forced view, restoring the previous view afterwards.
// Returns false on its early exits and SetFirstPerson's result otherwise; Actor::SetIronSights reads it.
bool PlayerCharacter::ForceTemp1stPerson(bool abIronSights1stPerson) {
    this->StopVanityMode();
    bTemp1stPerson = true;
    if (!abIronSights1stPerson && this->GetIronSights()) {
        this->SetIronSights(false, false, false);
    }
    this->SetBlock(false);
    if (abIronSights1stPerson) {
        if (this->Get3D(false)->GetAppCulled()) {
            return false;
        }
    }
    else if (!b3rdPerson) {
        if (this->Get3D(false)->GetAppCulled()) {
            return false;
        }
        this->Get3D(false)->SetAppCulled(true);
        return false;
    }
    bTemp1stPersonSwitchBack = true;
    return this->SetFirstPerson(true);
}

void PlayerCharacter::EnteringMenuMode() {
    if (ArrowProjectile::GetArrowCount() > 0) {
        BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
        if (pProcess && pProcess->GetCurrentAmmo()) {
            TESBoundObject* pAmmo = pProcess->GetCurrentAmmo()->ItemChange::GetItemChangeObject();
            if (pAmmo) {
                ProcessLists::DeleteAttachedArrows(pAmmo, std::numeric_limits<int32_t>::max(), this, 1, 1);
            }
        }
    }
}

void PlayerCharacter::DestroyMouseSpring() {
    if (spGrabSpring.m_pObject) {
        TESObjectCELL* pcell = GetParentCell();
        bhkWorld* pw = pcell ? pcell->GetbhkWorld() : nullptr;
        spGrabSpring->Remove();
        if (pw) {
            pw->MarkForWrite();
        }
        hkpEntity* pEntity = spGrabSpring->GetEntity();
        if (pEntity) {
            pEntity->activate();
        }
        if (pw) {
            pw->MarkForWrite();
        }
    }
    RemovePlayerAction(PLAYER_ACTION_Z_KEY_OBJECT, nullptr);
    spGrabSpring = static_cast<bhkMouseSpringAction*>(nullptr);
    eGrabType = PlayerCharacter::GRAB_NONE;
    fGrabObjectWeight = 0.0f;
    if (pGrabbedObject && pGrabbedObject->IsReference()) {
        Script::SetActionFlag(nullptr, &pGrabbedObject->m_Extra, SCRIPT_ACTION_ONRELEASE);
    }

    pGrabbedObject = nullptr;
}

// Moves the grab spring's target to the point fGrabDistance ahead of the eyes (shortened to the first hit of a ray cast
// along the look direction), or drops the object: when the spring or the object is gone, when the player stands on it,
// when telekinesis sees something else between the eyes and it, or when it is too far from the target and held by
// heavier contacts. afTimeDelta is unused. The collector's destructor runs where its scope ends, after
// DestroyMouseSpring on each early return.
void PlayerCharacter::UpdateMouseSpring(float afTimeDelta) {
    if (!pGrabbedObject || !spGrabSpring || !spGrabSpring->GetWorldObject()) {
        DestroyMouseSpring();
        return;
    }
    bhkCharacterController* pCharController = static_cast<bhkCharacterController*>(GetCharController());
    if (pCharController) {
        bhkRigidBody* pSupportBody = pCharController->GetSupportBody();
        if (pSupportBody == spGrabSpring->GetWorldObject() && pCharController->GetSupportCount() == 1) {
            DestroyMouseSpring();
            return;
        }
    }

    float fOffset = 0.0f;
    float fDistance = fGrabDistance + fOffset;
    alignas(16) NiPoint3 kOrigin;
    NiPoint3 kDirection;
    GetEyeVector(kOrigin, kDirection, true);

    bhkPickData kPick;
    bhkPickData* pPick = &kPick;
    hkpEntity* pEntity = spGrabSpring->GetEntity();
    bhkEntity* pBody = bhkEntity::Getbhk(pEntity);
    CFilter kFilter;
    pBody->GetCollisionFilter(&kFilter);
    CFilter kPlayerFilter;
    pPlayer->GetCollisionFilter(&kPlayerFilter.iFilter);
    kFilter.SetGroup(kPlayerFilter.GetGroup());
    pPick->SetFilter(kFilter);
    pPick->NiSetFrom(kOrigin);
    pPick->NiSetLength(kDirection * fDistance);
    hkpAllRayHitCollector kCollector;
    hkpAllRayHitCollector* pCollector = &kCollector;
    pPick->SetAllCollector(pCollector);
    bhkWorld* pWorld = pPlayer->GetParentCell()->GetbhkWorld();
    if (pWorld->PickObject(*pPick)) {
        float fHitFraction = 1.0f;
        for (int32_t i = 0; fHitFraction == 1.0 && i < pCollector->getHits().getSize(); ++i) {
            hkpWorldRayCastOutput kHit(pCollector->getHits()[i]);
            hkpWorldRayCastOutput* pHit = &kHit;
            NiAVObject* pHitObject = GetAVObjectForCollidable(pHit->m_rootCollidable);
            if (pHitObject) {
                TESObjectREFR* pHitRef = TESObjectREFR::FindReferenceFor3D(pHitObject);
                if (pHitRef && pHitRef != pGrabbedObject && pHitRef != pPlayer) {
                    fHitFraction = pHit->m_hitFraction;
                }
            }
        }
        float fHitDistance = fHitFraction * fDistance - fOffset;
        fDistance = BSMAX(0.0f, fHitDistance);
        if (eGrabType == GRAB_TELEKINESIS) {
            fGrabDistance = fDistance;
        }
    }

    if (eGrabType == GRAB_TELEKINESIS) {
        bhkPickData kLinePick;
        bhkPickData* pLinePick = &kLinePick;
        CFilter kLineFilter;
        // GetCollisionFilter returns its argument: the filter is read from kLineFilter.
        pBody->GetCollisionFilter(&kLineFilter);
        pLinePick->SetFilter(kLineFilter);
        pLinePick->NiSetFrom(kOrigin);
        NiPoint3 kTo = pGrabbedObject->Get3D()->GetWorldBound().GetCenter();
        pLinePick->NiSetTo(kTo);
        if (pPlayer->GetParentCell()) {
            bhkWorld* pLineWorld = pPlayer->GetParentCell()->GetbhkWorld();
            if (pLineWorld->PickObject(*pLinePick)) {
                TESObjectREFR* pBlocker = TESObjectREFR::FindReferenceFor3D(pLinePick->GetNiAVObject());
                if (pBlocker && pGrabbedObject != pBlocker && !pBlocker->IsActor()) {
                    DestroyMouseSpring();
                    return;
                }
            }
        }
    }

    hkVector4 kDiff;
    hkVector4 kLocal;
    hkVector4 kWorld;
    hkTransform kTransform;
    spGrabSpring->GetMousePositionInWorld(kWorld);
    spGrabSpring->GetMousePositionInRBLocal(kLocal);
    // Slot 61 (+0xF4) through the entity's wrapper.
    static_cast<bhkRigidBody*>(pBody)->GetTransform(kTransform);
    kDiff.setTransformedPos(kTransform, kLocal);
    kDiff.sub4(kWorld);
    float fSpringDistance = HK2NI(static_cast<float>(kDiff.length3()));
    if (eGrabType != GRAB_TELEKINESIS && fSpringDistance > 96.0) {
        DestroyMouseSpring();
        return;
    }

    // The heaviest body held against the grabbed one by a contact constraint.
    int32_t iNumConstraints = pEntity->getNumConstraints();
    float fContactMass = 0.0f;
    for (int32_t j = 0; j < iNumConstraints; ++j) {
        hkpConstraintInstance* pConstraint = pEntity->getConstraint(j);
        if (pConstraint) {
            hkpConstraintData* pData = pConstraint->getDataCalled();
            if (pData->getType() == hkpConstraintData::CONSTRAINT_TYPE_CONTACT) {
                hkpEntity* pOther;
                if (pConstraint->getEntityACalled() == pEntity) {
                    pOther = pConstraint->getEntityBCalled();
                }
                else {
                    pOther = pConstraint->getEntityACalled();
                }
                hkpRigidBody* pOtherBody = static_cast<hkpRigidBody*>(pOther);
                if (pOtherBody && fContactMass < pOtherBody->getMass()) {
                    fContactMass = pOtherBody->getMass();
                }
            }
        }
    }
    float fMassRatio = fContactMass / static_cast<hkpRigidBody*>(pEntity)->getMass();
    if (fZKeyMaxContactDistance.Float() < fSpringDistance && fZKeyMaxContactMassRatio.Float() < fMassRatio) {
        DestroyMouseSpring();
        return;
    }

    NiAVObject* pRefNode = pGrabbedObject->Get3D();
    bool bComplex = BSXFlags::FindFlag(pRefNode, BSXFlags::COMPLEX) != 0;
    bool bArticulated = BSXFlags::FindFlag(pRefNode, BSXFlags::ARTICULATED) != 0;
    if (pGrabbedObject->IsActor()) {
        CFilter kActorFilter;
        // GetCollisionFilter returns its argument, kActorFilter.
        pBody->GetCollisionFilter(&kActorFilter);
        if (kActorFilter.QBipLayer()) {
            // A biped part counts as complex when its body hangs below the actor's first child node.
            bComplex = false;
            NiNode* pNode = pRefNode->IsNode();
            if (pNode && pNode->GetAt(0)) {
                pRefNode = pNode->GetAt(0);
            }
            bhkCollisionObject* pCollisionObject = bhkCollisionObject::Getbhk(static_cast<hkpRigidBody*>(pEntity));
            NiAVObject* pObject = pCollisionObject ? pCollisionObject->GetSceneGraphObject() : nullptr;
            while (pObject && pObject != pNode) {
                if (pObject == pRefNode) {
                    bComplex = true;
                }
                pObject = pObject->GetParent();
            }
        }
    }

    if (pRefNode && bComplex && fZKeyComplexHelperMinDistance.Float() < fSpringDistance) {
        float fHelperScale = (fSpringDistance - fZKeyComplexHelperMinDistance.Float()) *
            fZKeyComplexHelperScale.Float() / fSpringDistance;
        float fWeightMin = fZKeyComplexHelperWeightMin.Float();
        float fWeightMax = fZKeyComplexHelperWeightMax.Float();
        float fWeight = BSMIN(fWeightMax, BSMAX(fWeightMin, bhkUtilFunctions::GetTotalMass(pRefNode)));
        float fWeightLerp = FLerp(0.0f, 1.0f, fWeightMax, fWeightMin, fWeight);
        // Multiplied by -1.0 as a double, then by the float weight, and stored as a float.
        float fPull = static_cast<float>(fHelperScale * -1.0 * fWeightLerp);
        hkSimdReal kPull(fPull);
        kDiff.mul4(kPull);
        if (bArticulated || pGrabbedObject->IsActor()) {
            NiPoint3 kVelocity;
            HK2NI(kVelocity, kDiff);
            TESHavokUtilities::AddVelocity(pRefNode, kVelocity, false);
        }
        else {
            hkpEntity* pSpringEntity = spGrabSpring->GetEntity();
            if (pSpringEntity) {
                bhkRigidBody* pSpringBody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pSpringEntity));
                hkVector4 kVelocity;
                float fZ = kDiff(2);
                float fY = kDiff(1);
                float fX = kDiff(0);
                kVelocity.set(fX, fY, fZ, 0.0f);
                kVelocity.add4(pSpringBody->GetLinearVelocity());
                pSpringBody->SetLinearVelocity(kVelocity);
            }
        }
    }

    NiPoint3 kTarget = kOrigin + kDirection * fDistance;
    if (pCharController) {
        // Keeps the target out of the player's capsule when looking down.
        NiPoint3 kFlat = kDirection * fDistance;
        if (kDirection.z < 0.0) {
            kFlat.z = 0.0f;
            float fFlatLength = kFlat.UnitizeGetLength();
            float fMinLength = static_cast<float>(HK2NI(pCharController->GetRadius()) + 5.0);
            if (fMinLength > fFlatLength) {
                float fPush = fMinLength - fFlatLength;
                kTarget += kFlat * fPush;
            }
        }
    }
    spGrabSpring->SetMousePosition(kTarget);
}

void PlayerCharacter::SetSpeaking(bool abValue) {
    bSpeaking = abValue;
}

float PlayerCharacter::GetRockItLauncherWeight() const {
    return fRockItLauncherAmmoWeight;
}

ItemChange* PlayerCharacter::GetRandomRockItAmmoItem() {
    ItemChange* preturnAmmo = nullptr;
    if (!RockItLauncherAmmoList.IsEmpty()) {
        uint32_t uiRandom = Random();
        ItemChange* prandomItem = RockItLauncherAmmoList.GetAt(uiRandom % RockItLauncherAmmoList.iSize);
        ExtraDataList* pExtra = prandomItem->pExtraObjectList ? prandomItem->pExtraObjectList->m_item : nullptr;
        return RemoveRockItAmmo(prandomItem->pContainerObj, pExtra, 1, true);
    }
    return preturnAmmo;
}

uint32_t PlayerCharacter::GetMinorCrimeCount() const {
    return iMinorCrimes;
}

int32_t PlayerCharacter::GetSleepHours() const {
    return iSleepTime;
}

NiPoint3* PlayerCharacter::GetCameraPosition() {
    return &CachedWorldCameraPos;
}

BSSimpleList<Actor*>* PlayerCharacter::GetTeammateList() {
    return &pListOfTeammates;
}

NiPoint3* PlayerCharacter::GetCached1stPersonCameraPos() {
    return &Cached1stPersonCameraPos;
}

NiPoint3 PlayerCharacter::GetCamera3rdPersonShoulderOffset() {
    return kCamera3rdPersonShoulderOffset;
}

BSSimpleArray<ItemChange*>* PlayerCharacter::GetRockItAmmoArray() {
    return &RockItLauncherAmmoList;
}

TESObjectREFR* PlayerCharacter::QPlayerMapMarker() const {
    return pPlayerMapMarker;
}

BSSimpleList<BGSQuestObjective*>* PlayerCharacter::GetQuestObjectiveList() {
    return &listObjectives;
}

TeleportPath* PlayerCharacter::QPlayerMarkerPath() {
    return &PlayerMarkerPath;
}

BSSimpleList<PlayerCharacter::MapMarkerInfo*>* PlayerCharacter::GetMapMarkerStructList() {
    return &MapMarkerList;
}

void PlayerCharacter::SetInCharGen(bool abInCharGen) {
    bChargen = abInCharGen;
}

bool PlayerCharacter::IsVanityMode() {
    return bVanityMode;
}

void PlayerCharacter::ForcePhysicsGrabDrop() {
    DestroyMouseSpring();
}

void PlayerCharacter::ClearSitHeadingDelta() {
    fSitHeadingDelta = 0.0f;
}

void PlayerCharacter::ClearAutoAim() {
    pAutoAimActor = nullptr;
    BulletAutoAim = NiPoint3::ZERO;
}

void PlayerCharacter::ClearGrabObject() {
    pGrabbedObject = nullptr;
}

void PlayerCharacter::SetWorldFOV(float afFOV) {
    fWorldFOV = afFOV;
}

void PlayerCharacter::Set1stPersonFOV(float afFOV) {
    f1stPersonFOV = afFOV;
}

void PlayerCharacter::SetBlockActivate(bool abVal) {
    bBlockActivate = abVal;
    if (abVal) {
        fBlockActivateTimer = 5.0f;
    }
    else {
        fBlockActivateTimer = 0.0f;
    }
}

bool PlayerCharacter::GetWant3rdPerson() {
    return bWant3rdPerson;
}

char PlayerCharacter::GetPCControlFlags() {
    return static_cast<int8_t>(ucControlsDisabled);
}

uint32_t PlayerCharacter::GetTeammateCount() {
    return iTeammateCount;
}

bool PlayerCharacter::GetInChargen() {
    return bChargen;
}

void PlayerCharacter::SetAIControlledPackage(bool abTF) {
    bAiControlledPackage = abTF;
}

void PlayerCharacter::SetDisableFastTravelResetsOnCellChange(char abDisable) {
    if (abDisable) {
        bCanFastTravel |= 2u;
    }
    else {
        bCanFastTravel &= ~2u;
    }
}

void PlayerCharacter::SetPCCanUsePowerArmor(bool abVal) {
    bCanUsePowerArmor = abVal;
}

void* PlayerCharacter::GetCameraRigidBody() {
    return spCameraRigidBody;
}

bool PlayerCharacter::GetMegatonDestroyed() {
    return bMegatonDestroyed;
}

void PlayerCharacter::SetLockpickControlsDisable(char abDisable) {
    if (abDisable) {
        bCanFastTravel |= 4;
    }
    else {
        bCanFastTravel &= ~4;
    }
}

void PlayerCharacter::SetAllowEGMCacheClear(bool abAllow) {
    bAllowEGMCacheClear = abAllow;
}

MagicShaderHitEffect* PlayerCharacter::GetMagicShaderHitEffect() {
    return pInventoryWeaponEffect;
}

void PlayerCharacter::SetTelekinesisDistance(float afDistance) {
    fGrabDistance = afDistance;
}

float PlayerCharacter::GetGrabDistance() {
    return fGrabDistance;
}

bool PlayerCharacter::CanPCUsePowerArmor() {
    return bCanUsePowerArmor;
}

bool PlayerCharacter::IsPlayerResistingArrest() {
    return bBeingChased;
}

bool PlayerCharacter::IsGreetingTimerClear() {
    return fLastHelloTime < 0.0;
}

int32_t PlayerCharacter::GetSandManDetectionValue() {
    return static_cast<int32_t>(iSandmanDetection);
}

void PlayerCharacter::SetInventoryWeaponEffect(MagicShaderHitEffect* apEffect) {
    pInventoryWeaponEffect = apEffect;
}

bool PlayerCharacter::GetAIControlledFromPosition() {
    return bAiControlledFromPos;
}

void PlayerCharacter::AddActorToPlayerCombatList(Actor* apActor) {
    if (!pCombatGroup) {
        pCombatGroup = pCombatManager->CreateCombatGroup();
        pCombatGroup->AddMember(this);
    }
    pCombatGroup->AddTarget(apActor);
}

int32_t PlayerCharacter::GetScriptAllowedFastTravel() {
    return (bCanFastTravel & 1) != 0;
}

bool PlayerCharacter::GetBlockActivate() {
    return bBlockActivate;
}

MagicItem* PlayerCharacter::GetSelectedSpell() {
    return pSelectedSpell;
}

bool PlayerCharacter::CheckInvulnerable(ActorValue::Index aiWhich, float afValue) {
    if (!static_cast<MagicTarget*>(this)->IsInvulnerable()) {
        return true;
    }
    if (afValue < 0.0) {
        if (aiWhich == ActorValue::HEALTH ||
            (aiWhich > ActorValue::EXPERIENCE_POINTS && aiWhich <= ActorValue::BRAIN_CONDITION)) {
            return false;
        }
    }
    else if (afValue > 0.0 && aiWhich == ActorValue::RADIATION_RADS) {
        return false;
    }
    return true;
}

int32_t PlayerCharacter::GetNumberActorsInCombat() {
    if (pCombatGroup) {
        return static_cast<int32_t>(pCombatGroup->GetTargetCount());
    }

    return 0;
}

void PlayerCharacter::SetNeedResetHardcoreTimer(bool aNeedReset) {
    bResetHardcoreTimers = aNeedReset;
}

void PlayerCharacter::StopIronSightsImageSpace() {
    if (pIronsightsDOFInstance) {
        ImageSpaceModifierInstance::Stop(pIronsightsDOFInstance);
        pIronsightsDOFInstance = nullptr;
    }
}

void PlayerCharacter::StopVatsImageSpace() {
    if (pVatsDOFInstance) {
        ImageSpaceModifierInstance::Stop(pVatsDOFInstance);
        pVatsDOFInstance = nullptr;
    }
}

bool PlayerCharacter::NeedResetHardcoreTimer() {
    return bResetHardcoreTimers;
}

void PlayerCharacter::SetActorInSneakRange(bool abflag) {
    bActorinSneakRange = abflag;
}

char PlayerCharacter::IsShowQuestItems() {
    return bShowQuestItemsInInventory;
}

int32_t PlayerCharacter::GetAmountSoldStolen() {
    return iAmountStolenSold;
}

void PlayerCharacter::SetLastModifiedFactionReputation(TESReputation* apReputation) {
    pReputationUpdate = apReputation;
}

uint32_t PlayerCharacter::GetCaravanCapWinnings() {
    return iCaravanCapWinnings;
}

void PlayerCharacter::ToggleKillCamMode(KILL_CAM_TYPE aeMode) {
    KILL_CAM_TYPE eOldMode = GetKillCamMode();
    SetKillCamMode(aeMode);
    if (eOldMode != GetKillCamMode() && IsInSlowMo()) {
        ResetKillCam(eOldMode, true);
    }
}

void* PlayerCharacter::GetBhkMouseSpringAction() {
    return spGrabSpring.m_pObject;
}

PlayerCharacter::GrabbingType PlayerCharacter::GetGrabMode() {
    return eGrabType;
}

void PlayerCharacter::StopHeartbeatSound() {
    StatusSoundHandle.FadeOutAndRelease(0x3E8u);
}

void PlayerCharacter::SetLastGreetingTime() {
    fLastHelloTime = static_cast<float>(BSGetTickCount());
}

void PlayerCharacter::SetLastKnownGoodPosition() {
    LastKnownGoodPosition = *TESObjectREFR::QLocationOnReference();
    if (TESObjectREFR::GetParentCell() && TESObjectREFR::GetParentCell()->GetWorldSpace()) {
        pLastKnownGoodLocation = TESObjectREFR::GetParentCell()->GetWorldSpace();
    }
    else {
        pLastKnownGoodLocation = TESObjectREFR::GetParentCell();
    }
}

void PlayerCharacter::ModPlayerGreetTimer() {
    if (bGreetingPlayer) {
        fGreetTimer = appTimer.GetSeconds() + fGreetTimer;
    }
}

bool PlayerCharacter::IsAMurderer() {
    return cMurder != 0;
}

void PlayerCharacter::SetIsAMurderer() {
    cMurder = 1;
    IncNumCrimes(PackageNS::CRIME_TYPE_MURDER, 1);
}

void PlayerCharacter::IssueStealWarning() {
    ++iNumberofStealWarnings;
    if (fStealWarningTimer == -1.0) {
        fStealWarningTimer = fWarningTimer.Float();
    }
}

void PlayerCharacter::IssuePickPocketWarning() {
    ++iNumberofPickpocketWarnings;
    if (fPickPocketWarningTimer == -1.0) {
        fPickPocketWarningTimer = fWarningTimer.Float();
    }
}

char PlayerCharacter::IsPlayingCombatAttackSound() {
    return bSpeaking;
}

void PlayerCharacter::UpdateTemp3rdPerson() {
    if (bTemp3rdPerson &&
        pCurrentProcess->GetAnimAction() == -1 &&
        !pCurrentProcess->GetKnockState() &&
        !PlayerCharacter::bVanityMode &&
        !PlayerCharacter::bIsAutoVanityMode) {
        bTemp3rdPerson = false;
        if (bTemp3rdPersonSwitchBack) {
            SetFirstPerson(true);
        }
        bTemp3rdPersonSwitchBack = false;
    }
}

void PlayerCharacter::Setup3rdPersonCameraCaster() {
    if (pCameraCaster && TESObjectREFR::GetParentCell()) {
        bhkWorldM* pCasterWorld = static_cast<bhkWorldM*>(pCameraCaster->GetWorld());
        if (static_cast<const void*>(pCasterWorld) !=
            static_cast<const void*>(TESObjectREFR::GetParentCell()->GetbhkWorld())) {
            pCameraCaster->Add(TESObjectREFR::GetParentCell()->GetbhkWorld());
        }
    }
}

bool PlayerCharacter::ToggleFlyCam(bool abFreezeTime) {
    uint8_t ucFlyCamera = !appMain->GetFlyCamera();
    appMain->SetFlyCamera(ucFlyCamera);
    if (ucFlyCamera && abFreezeTime) {
        appMain->SetFreezeTime(true);
    }
    else {
        appMain->SetFreezeTime(false);
    }
    Interface::GetHUDEffects()->bHideHUD = ucFlyCamera;
    if (!ucFlyCamera) {
        return ucFlyCamera;
    }
    const NiPoint3& kLocation = GetLocationOnReference();
    UFOCameraPos = kLocation;
    UFOCameraPos.z = TESObjectREFR::GetScale() * fEyeHeight + UFOCameraPos.z;
    fUFOCameraHeading = TESObjectREFR::GetAngleOnReference().z;
    fUFOCameraPitch = TESObjectREFR::GetAngleOnReference().x;
    return ucFlyCamera;
}

Actor* PlayerCharacter::GetBumpedActor() {
    bhkCharacterController* pCharController = MobileObject::GetCharController();
    if (!pCharController) {
        return nullptr;
    }
    bhkCharacterController* pBumpedController = pCharController->GetBumpedCharacterController();
    NiAVObject* pObject = pBumpedController ? pBumpedController->GetNiObject() : nullptr;
    TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pObject);
    if (pRef && pRef->IsActor()) {
        return static_cast<Actor*>(pRef);
    }

    return nullptr;
}

MediaLocationController* PlayerCharacter::GetMediaLocationController() {
    if (pClosestAudioMarkerInfo && pClosestAudioMarkerInfo->pMarkerData) {
        return static_cast<MediaLocationController*>(
            static_cast<AudioMarker*>(pClosestAudioMarkerInfo->pMarkerData)->GetMediaLocationController()
        );
    }
    AudioMarkerInfo* pClosestInfo = GetClosestAudioMarkerInfo();
    if (pClosestInfo && pClosestInfo->pMarkerData) {
        return static_cast<MediaLocationController*>(
            static_cast<AudioMarker*>(pClosestInfo->pMarkerData)->GetMediaLocationController()
        );
    }

    return nullptr;
}

char PlayerCharacter::GetInSleepPackage() {
    return bAiControlledToPos;
}

int32_t PlayerCharacter::GetNumberofNotes() {
    return listNotes.ItemsInList();
}

void PlayerCharacter::ReturnRockItAmmo() {
    InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pPlayer);
    for (uint32_t i = 0; pChanges && i < RockItLauncherAmmoList.BSSimpleArray<ItemChange*>::GetSize(); ++i) {
        pChanges->AddChangeItem(RockItLauncherAmmoList.GetAt(i), true);
    }
    RockItLauncherAmmoList.Clear(true);
    ResetRockItAmmoWeight(false);
}

uint32_t PlayerCharacter::GetCaravanCapLosses() {
    return iCaravanCapLosses;
}

// Counts the queued eat/drink potions, then the active potion effects the queue does not hold: each distinct potion
// counts once, and counts once more per extra effect of it the player still has.
int32_t PlayerCharacter::GetCurrentPotionCount() {
    int32_t iCount = 0;
    int32_t iMax = 100;
    BSSimpleList<MagicItem*>* pList = EatDrinkItems;
    for (; pList && !pList->IsEmpty(); pList = pList->GetNext()) {
        if (pList->GetItem()) {
            MagicItem* pQueued = pList->GetItem();
            if (pQueued->GetSpellType() == MagicSystem::POTION) {
                ++iCount;
            }
        }
    }
    // 16-byte aligned stack arrays of iMax entries.
    MagicItem** pPotions = static_cast<MagicItem**>(XFNV_ALLOCA16(sizeof(MagicItem*) * iMax));
    uint32_t* pEffectCounts = static_cast<uint32_t*>(XFNV_ALLOCA16(4 * iMax));
    if (!pPotions || !pEffectCounts) {
        return iCount;
    }
    for (int32_t i = 0; i < iMax; ++i) {
        pPotions[i] = nullptr;
        pEffectCounts[i] = 0;
    }
    BSSimpleList<ActiveEffect*>* pEffects = GetActiveEffectList();
    for (; pEffects && !pEffects->IsEmpty(); pEffects = pEffects->GetNext()) {
        MagicItem* pPotion = pEffects->GetItem() ? pEffects->GetItem()->GetSpell() : nullptr;
        if (pPotion && pPotion->GetSpellType() == MagicSystem::POTION) {
            bool bQueued = EatDrinkItems && EatDrinkItems->IsInList(pPotion);
            if (!bQueued) {
                bool bFound = false;
                for (int32_t k = 0; k < iMax && !bFound; ++k) {
                    if (!pPotions[k]) {
                        pPotions[k] = pPotion;
                        pEffectCounts[k] = 1;
                        ++iCount;
                        bFound = true;
                    }
                    else if (
                        pPotions[k] == pPotion && pEffectCounts[k] < static_cast<uint32_t>(pPotions[k]->ItemsInList())
                    ) {
                        ++pEffectCounts[k];
                        bFound = true;
                    }
                }
            }
        }
    }
    return iCount;
}

void PlayerCharacter::ClearTeammates() {
    iTeammateCount = 0;
    pListOfTeammates.RemoveAll();
}

bool PlayerCharacter::GetTemp1stPersonSwitchBack() {
    return bTemp1stPersonSwitchBack;
}

bool PlayerCharacter::AddCaravanCard(TESCaravanCard* apCard) {
    if (pInactiveListofCaravanCards->IsInList(apCard) || pActiveListofCaravanCards->IsInList(apCard)) {
        return true;
    }
    apCard->bIsActive = false;
    pInactiveListofCaravanCards->AddHead(apCard);
    return true;
}

void PlayerCharacter::SetActorBaseValue(ActorValue::Index aiWhich, int32_t aiValue) {
    bool bOverridden = false;
    GetBaseValueOverride(aiWhich, bOverridden);
    if (bOverridden) {
        SetBaseValueOverride(aiWhich, static_cast<float>(aiValue));
    }
    else {
        GetActorBase()->SetActorValue(aiWhich, aiValue);
    }
    Interface::UpdateActorValue(aiWhich);
    CheckSkillLevelChange(aiWhich, true);
}

void PlayerCharacter::StartKillcamForActor(TESObjectREFR* apTarget, float afTime, bool abUseCooldown, int32_t aiLimb) {
    if (eKillCameraSetting && (!GetParentCell() || GetParentCell()->iFormID != 0x161E98)) {
        BSSoundHandle th_s;
        th_s = BSAudio::QInstance().GetSoundHandleByName("UIPopUpQuestNew", 289u);
        th_s.Play(false);
        if (!apTarget || pVATS.eMode) {
            return;
        }
        if (abUseCooldown && fKillCamCooldown <= 0.0) {
            fKillCamCooldown = fKillCameraCooldown.Float();
        }
        else if (abUseCooldown) {
            return;
        }
        fTimeInSlowMoCam = afTime;
        if (eKillCameraSetting == 2) {
            if (b3rdPersonSaved) {
                bVATSReturnPlayerTo1stPerson = !bsave3rdPerson;
            }
            else {
                bVATSReturnPlayerTo1stPerson = !pPlayer->Is3rdPerson();
            }
            pVATS.AppendCamTarget(apTarget, aiLimb);
            if (GetIronSights()) {
                Interface::SetGunScopeVisible(false);
            }
        }
        else {
            fTimeInSlowMoCam = fTimeInSlowMoCam * 0.4000000059604645;
        }
        if (!pPlayer->GetActorValue(ActorValue::TURBO)) {
            BSTimer::SaveGlobalTimeMultiplier();
        }
    }
}

bool PlayerCharacter::AddEatDrinkItem(MagicItem* apItem) {
    if (apItem->GetSpellType() == MagicSystem::POTION) {
        if (GetCurrentPotionCount() < 100) {
            EatDrinkItems->AddHead(apItem);
            CastAlchemy(apItem, true);
        }
    }
    else {
        CastAlchemy(apItem, true);
    }
    ProcessEffectsFromItem(apItem);
    EvaluateSpellConditions();
    UpdateTarget(0.01f);
    return true;
}

void PlayerCharacter::DoIronSightsZoom(float afDelta) {
    if (bIronSightsZoomEnable.Bool()) {
        TESObjectWEAP* pweapon;
        float fextraZoom;
        if (pCurrentProcess->GetCurrentWeapon()) {
            pweapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
        }
        else {
            pweapon = nullptr;
        }
        float fdefault = BSSceneGraph::fDefaultWorldFOV.Float();
        float f1stdefault = BSSceneGraph::fDefault1stPersonFOV.Float();
        float fpipboy = BSSceneGraph::fPipboy1stPersonFOV.Float();
        float fzoom = fIronSightsZoomDefault.Float();
        float fchangeTime = fIronSightsFOVTimeChange.Float();
        Main::GetWorldSceneGraph()->GetCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
        if (pweapon && pCurrentProcess->GetWeaponDrawn()) {
            fzoom = std::fabs(pweapon->GetIronSightFOV());
            ItemChange* pchange = pCurrentProcess->GetCurrentWeapon();
            fextraZoom = 0.0f;
            if (pchange->HasModEffectActive(WEAPON_MOD_IRON_SITES, fextraZoom)) {
                fzoom = fzoom - fextraZoom;
            }
        }

        if (IsPipboyActive()) {
            if (GetIronSights()) {
                SetIronSights(false, false, false);
            }
            f1stPersonFOV = fpipboy;
            fWorldFOV = fdefault;
        }
        else if (PlayerCharacter::bVanityMode && bFOVTargetReached) {
            double dCameraFOV = Main::GetWorldSceneGraph()->GetCameraFOV();
            if (fdefault != dCameraFOV) {
                SetFOV(fdefault);
            }
        }
        else {
            float ftarget = fdefault;
            float f1target = f1stdefault;
            float fdelta = afDelta * 30.0 / fchangeTime;
            if (GetIronSights() &&
                fzoom > 5.0 &&
                ((GetAnimAction() != ANIM_ACTION_RELOAD && GetAnimAction() != ANIM_ACTION_RELOAD_LOOP) ||
                 pVATS.eMode == VATS::VATS_PLAYBACK)) {
                ftarget = fzoom;
                f1target = fzoom / fdefault * f1stdefault;
            }
            if ((pVATS.eMode == VATS::VATS_MENU_MODE || pVATS.eMode == VATS::VATS_SCAN_MODE) &&
                pVATSTarget &&
                bWant3rdPerson == b3rdPerson) {
                NiAVObject* pobj = pVATSTarget->Get3D();
                const NiBound* rbound = &pobj->GetWorldBound();
                NiPoint3 src = p1stPersonCamera->GetWorldTranslate() - rbound->GetCenter();
                float fdist = src.Length();
                double dTargetRadius = rbound->m_fRadius;
                double dDlgFocus = static_cast<float>(fDlgFocus);
                float ftan = dDlgFocus * dTargetRadius / fdist;
                float ffovMultDelta = fVATSTargetFOVMultFar.Float() - fVATSTargetFOVMultNear.Float();
                float fdistMult = fdist / fVATSTargetFOVMultFarDist.Float();
                if (fdistMult > 1.0) {
                    fdistMult = 1.0f;
                }
                float ffovMult = ffovMultDelta * fdistMult + fVATSTargetFOVMultNear.Float();
                ftarget = std::atan(ftan) * ffovMult;
                if (fVATSTargetFOVMinFOV.Float() > static_cast<double>(ftarget)) {
                    ftarget = fVATSTargetFOVMinFOV.Float();
                }
                f1target = ftarget;
                if (BSSceneGraph::fDefaultFOV.Float() <= static_cast<double>(f1target)) {
                    f1target = BSSceneGraph::fDefaultFOV.Float();
                }
                static NonLinearInterpolator* pVATSWorldZoomInterpolator = new NonLinearInterpolator;
                static NonLinearInterpolator* pVATSViewmodelZoomInterpolator = new NonLinearInterpolator;
                if (poldVATSTarget != pVATSTarget) {
                    float fcamTime = 0.75f;
                    if (pVATS.eMode == VATS::VATS_TARGET_SELECT) {
                        fcamTime = fVATSTargetSelectCamPanTime.Float();
                        pVATSWorldZoomInterpolator->Setup(fWorldFOV, ftarget, fcamTime);
                    }
                    else {
                        if (pVATS.eMode == VATS::VATS_MENU_MODE) {
                            fcamTime = fVATSCamZoomInTime.Float();
                        }
                        pVATSWorldZoomInterpolator->Setup(fWorldFOV, ftarget, fcamTime);
                    }
                    pVATSViewmodelZoomInterpolator->Setup(f1stPersonFOV, f1target, fcamTime);
                    poldVATSTarget = pVATSTarget;
                }
                pVATSWorldZoomInterpolator->Interpolate(appTimer.fDelta);
                pVATSViewmodelZoomInterpolator->Interpolate(appTimer.fDelta);
                fWorldFOV = pVATSWorldZoomInterpolator->GetCurrent();
                f1stPersonFOV = pVATSViewmodelZoomInterpolator->GetCurrent();
                if (pVATSViewmodelZoomInterpolator->GetFinished()) {
                    VATSMenu::SetHeadingReached(true);
                    bFOVTargetReached = true;
                }
                else {
                    VATSMenu::SetHeadingReached(false);
                    bFOVTargetReached = false;
                }
            }
            else if (pVATS.eMode == VATS::VATS_PLAYBACK) {
                poldVATSTarget = nullptr;
                fWorldFOV = ftarget;
                f1stPersonFOV = f1target;
                bFOVTargetReached = true;
            }
            else {
                poldVATSTarget = nullptr;
                if (ftarget < static_cast<double>(fWorldFOV)) {
                    fWorldFOV = fWorldFOV - fdelta;
                    if (ftarget > static_cast<double>(fWorldFOV)) {
                        fWorldFOV = ftarget;
                    }
                }
                else if (ftarget > static_cast<double>(fWorldFOV)) {
                    fWorldFOV = fWorldFOV + fdelta;
                    if (ftarget < static_cast<double>(fWorldFOV)) {
                        fWorldFOV = ftarget;
                    }
                }
                if (f1target < static_cast<double>(f1stPersonFOV)) {
                    f1stPersonFOV = f1stPersonFOV - fdelta;
                    if (f1target > static_cast<double>(f1stPersonFOV)) {
                        f1stPersonFOV = f1target;
                        bFOVTargetReached = true;
                    }
                    else {
                        bFOVTargetReached = false;
                    }
                }
                else if (f1target > static_cast<double>(f1stPersonFOV)) {
                    f1stPersonFOV = f1stPersonFOV + fdelta;
                    if (f1target < static_cast<double>(f1stPersonFOV)) {
                        f1stPersonFOV = f1target;
                        bFOVTargetReached = true;
                    }
                    else {
                        bFOVTargetReached = false;
                    }
                }
                else {
                    bFOVTargetReached = true;
                }
            }
        }
    }
}

void PlayerCharacter::CreateMouseSpring(
    TESObjectREFR* apObject,
    PlayerCharacter::GrabbingType aeGrabType,
    float afDistance
) {
    hkVector4 hkv;
    CFilter kBodyFilter;
    hkVector4 tmphk;
    CFilter kPlayerFilter;
    CFilter fltr;
    hkVector4 hkvCenterOfMass;
    float fmassTotal;
    if (bAllowHavokGrabTheLiving.Bool() || !apObject->IsActor() || apObject->IsDead(false)) {
        // A grenade projectile that is a live mine is never grabbed.
        if ((!apObject ||
             !apObject->IsProjectile() ||
             static_cast<Projectile*>(apObject)->GetProjectileType() != Projectile::PROJECTILE_TYPE_GRENADE ||
             !static_cast<Projectile*>(apObject)->IsMine() ||
             static_cast<Projectile*>(apObject)->IsFlagSet(Projectile::TURNED_OFF)) &&
            apObject &&
            apObject->GetParentCell()) {
            NiNode* pav = apObject->Get3D();
            bhkRigidBody* pbody = bhkRigidBody::FindHeaviestRigidBody(pav);
            if (pbody) {
                NiPoint3 point;
                NiPoint3 dir;
                int32_t bnormalGrabSuccess = BSXFlags::FindFlag(pav, BSXFlags::COMPLEX) || apObject->IsActor();
                bool bComplex = bnormalGrabSuccess;
                this->pGrabbedObject = apObject;
                // The script action bit 0x80000 has no name in OBJECT_ACTION.
                Script::SetActionFlag(nullptr, &this->pGrabbedObject->m_Extra, SCRIPT_ACTION_ONGRAB);
                this->eGrabType = aeGrabType;
                this->fGrabDistance = afDistance;
                hkMouseSpringActionCinfo kCinfo;
                fmassTotal = 0.0f;
                if (bComplex && apObject->IsActor()) {
                    GetTotalMassData(pav, &hkvCenterOfMass, &fmassTotal);
                }
                else {
                    pbody->GetCenterOfMassInWorld(hkvCenterOfMass);
                }
                this->GetEyeVector(point, dir, true);
                char cBodyFound = 0;
                bhkWorld* pw = apObject->GetParentCell()->GetbhkWorld();
                if (this->eGrabType == PlayerCharacter::GRAB_NORMAL && pw && !pSaveLoadGame->GetSaveGameLoading()) {
                    if (Interface::IsActivatePickFuzzy() && ViewCaster::GetFuzzyBody()) {
                        alignas(16) NiPoint3 mousePoint = Interface::GetActivatePickLocation();
                        if (!pSaveLoadGame->GetSaveGameLoading()) {
                            this->fGrabDistance = (mousePoint - point).Length();
                        }
                        kCinfo.m_mousePositionInWorld.NI2HK(mousePoint);
                        kCinfo.m_body = **ViewCaster::GetFuzzyBody();
                        cBodyFound = 1;
                    }
                    else {
                        bhkPickData pd;
                        fltr.iFilter = 0;
                        fltr.SetLayer(L_ITEMPICKER);
                        // GetCollisionFilter returns its argument: the group is read from kPlayerFilter.
                        this->GetCollisionFilter(&kPlayerFilter.iFilter);
                        fltr.SetGroup(kPlayerFilter.GetGroup());
                        pd.SetFilter(fltr);
                        pd.NiSetFrom(point);
                        pd.NiSetLength(dir * this->fGrabDistance);
                        if (pw->PickObject(pd)) {
                            hkpRigidBody* prb = pd.GethkRigidBody();
                            bhkCollisionObject* pcol = bhkCollisionObject::Getbhk(prb);
                            NiAVObject* ppickedAV = pcol ? pcol->m_pkSceneObject : nullptr;
                            TESObjectREFR* ppickedRef = TESObjectREFR::FindReferenceFor3D(ppickedAV);
                            if (prb && ppickedRef == apObject) {
                                if (!bComplex) {
                                    bhkRigidBody* pbhkbody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(prb));
                                    pbhkbody->GetCenterOfMassInWorld(hkvCenterOfMass);
                                }
                                cBodyFound = 1;
                                kCinfo.m_body = prb;
                                this->fGrabDistance = this->fGrabDistance * pd.m_hitFraction;
                                NiPoint3 springLoc = point + dir * this->fGrabDistance;
                                kCinfo.m_mousePositionInWorld.NI2HK(springLoc);
                            }
                        }
                    }
                }

                if (!cBodyFound) {
                    kCinfo.m_mousePositionInWorld = hkvCenterOfMass;
                    if (bComplex) {
                        hkVector4 addition;
                        NI2HKNS(addition, pav->GetWorldTransform().m_Rotate * NiPoint3::UNIT_Z);
                        kCinfo.m_mousePositionInWorld.ToHK(tmphk);
                        tmphk.add4(addition);
                        kCinfo.m_mousePositionInWorld = tmphk;
                    }
                    if (!pSaveLoadGame->GetSaveGameLoading()) {
                        NiPoint3 mousePoint;
                        HK2NI(mousePoint, kCinfo.m_mousePositionInWorld.ToHK(tmphk));
                        this->fGrabDistance = (mousePoint - point).Length();
                    }
                    kCinfo.m_body = **pbody;
                }
                if (this->eGrabType == PlayerCharacter::GRAB_NORMAL) {
                    this->AddPlayerAction(PLAYER_ACTION_Z_KEY_OBJECT, 20.0f, apObject);
                }
                hkpRigidBody* phkbody = kCinfo.m_body;
                if (!kCinfo.m_body->m_world ||
                    phkbody->getMotionType() == hkpMotion::MOTION_FIXED ||
                    phkbody->getMotionType() == hkpMotion::MOTION_KEYFRAMED) {
                    this->DestroyMouseSpring();
                }
                else {
                    hkVector4 hktemp;
                    bhkCharacterController* pcon = this->GetCharController();
                    if (pcon) {
                        float fMinDistance = HK2NI(pcon->GetRadius()) + 5.0;
                        if (fMinDistance > static_cast<double>(this->fGrabDistance)) {
                            this->fGrabDistance = fMinDistance;
                        }
                    }
                    if (this->eGrabType == PlayerCharacter::GRAB_TELEKINESIS) {
                        if (bComplex) {
                            kCinfo.m_springDamping = fMagicTelekinesisComplexSpringDamping.Float();
                            kCinfo.m_springElasticity = fMagicTelekinesisComplexSpringElasticity.Float();
                            kCinfo.m_objectDamping = fMagicTelekinesisComplexObjectDamping.Float();
                            kCinfo.m_maxRelativeForce = fMagicTelekinesisComplexMaxForce.Float();
                        }
                        else {
                            kCinfo.m_springDamping = fMagicTelekinesisSpringDamping.Float();
                            kCinfo.m_springElasticity = fMagicTelekinesisSpringElasticity.Float();
                            kCinfo.m_objectDamping = fMagicTelekinesisObjectDamping.Float();
                            kCinfo.m_maxRelativeForce = fMagicTelekinesisMaxForce.Float();
                        }
                    }
                    else {
                        float fscaleForce = 1.0f;
                        float fscaleElasticity = 1.0f;
                        if (this->pGrabbedObject->IsActor()) {
                            float fminWeight = fZKeyMaxForceWeightLow.Float();
                            float fmaxWeight = fZKeyMaxForceWeightHigh.Float();
                            float fscaleHigh = fZKeyMaxForceScaleHigh.Float();
                            float fscaleLow = fZKeyMaxForceScaleLow.Float();
                            float ftotalWeight = BSMIN(
                                fmaxWeight,
                                BSMAX(fminWeight, bhkUtilFunctions::GetTotalMass(pav))
                            );
                            fscaleForce = FLerp(fscaleLow, fscaleHigh, fmaxWeight, fminWeight, ftotalWeight) *
                                fscaleForce;
                            bool bonlyContactConstraints = true;
                            for (int32_t i = 0; i < phkbody->m_constraintsMaster.getSize() && bonlyContactConstraints;
                                 ++i) {
                                hkpConstraintData* pData = phkbody->m_constraintsMaster[i].m_constraint->m_data;
                                bonlyContactConstraints = pData->getType() ==
                                    hkpConstraintData::CONSTRAINT_TYPE_CONTACT;
                            }
                            if (bonlyContactConstraints) {
                                fscaleElasticity = fscaleElasticity * 0.10000000149011612;
                                fscaleForce = fscaleForce * 0.20000000298023224;
                            }
                        }
                        else {
                            if (pbody->GetCollisionFilter(&kBodyFilter)->GetLayer() == L_TRAP) {
                                fscaleElasticity = fscaleElasticity * 0.10000000149011612;
                                fscaleForce = fscaleForce * 0.5;
                            }
                        }
                        kCinfo.m_springDamping = fZKeySpringDamping.Float();
                        kCinfo.m_springElasticity = fZKeySpringElasticity.Float() * fscaleElasticity;
                        kCinfo.m_objectDamping = fZKeyObjectDamping.Float();
                        kCinfo.m_maxRelativeForce = fZKeyMaxForce.Float() * fscaleForce;
                    }
                    kCinfo.m_mousePositionInWorld.ToHK(hktemp);
                    hktemp.sub4(phkbody->getTranslation());
                    const hkTransform& kBodyTransform = phkbody->getTransform();
                    kCinfo.m_positionInRbLocal.ToHK(hkv).setRotatedInverseDir(kBodyTransform.m_rotation, hktemp);
                    kCinfo.m_positionInRbLocal = hkv;
                    this->pGrabbedObject->GetParentCell()->GetbhkWorld()->MarkForWrite();
                    this->spGrabSpring = new bhkMouseSpringAction(kCinfo);
                    this->spGrabSpring->Add(this->pGrabbedObject->GetParentCell()->GetbhkWorld());
                    this->pGrabbedObject->GetParentCell()->GetbhkWorld()->MarkForWrite();
                    this->fGrabObjectWeight = bhkUtilFunctions::GetTotalMass(apObject->Get3D());
                }
            }
            else if (aeGrabType == PlayerCharacter::GRAB_INVENTORY_DROP) {
                this->pGrabbedObject = apObject;
                // The script action bit 0x80000 has no name in OBJECT_ACTION.
                Script::SetActionFlag(nullptr, &this->pGrabbedObject->m_Extra, SCRIPT_ACTION_ONGRAB);
                this->eGrabType = PlayerCharacter::GRAB_INVENTORY_DROP;
                this->fGrabDistance = afDistance;
            }
        }
    }
}

void PlayerCharacter::PlayMagicFailureSound(MagicSystem::School aeSchool) {
    if (MagicFailureSounds[aeSchool].IsValid()) {
        MagicFailureSounds[aeSchool].Stop();
        NiPoint3 kLocation = GetLocationOnReference();
        MagicFailureSounds[aeSchool].SetPosition(kLocation.x, kLocation.y, kLocation.z);
        MagicFailureSounds[aeSchool].SetObjectToFollow(Get3D());
        MagicFailureSounds[aeSchool].Play(false);
    }
    else {
        TESSound* psound = MagicSystem::GetMagicFailureSound(aeSchool);
        if (psound) {
            MagicFailureSounds[aeSchool] = PlaySound(psound->iFormID, false, 2u, true);
        }
    }
}

// Has each follower look for a new package; a follower whose package check changes the list restarts the walk from
// the head.
void PlayerCharacter::HaveFollowersRecheckPackages() {
    ExtraFollower* pFollowerExtra = pPlayer->GetExtra()->GetFollowerExtra();
    if (pFollowerExtra && pFollowerExtra->pActorlist) {
        BSSimpleList<Actor*>* pActorlist = pFollowerExtra->pActorlist;
        while (pActorlist && pActorlist->m_item) {
            BSSimpleList<Actor*>* pnext = pActorlist->m_pkNext;
            Actor* pactor = pActorlist->m_item;
            if (pactor && pactor->GetCurrentAIProcess()) {
                BaseProcess* pprocess = pactor->GetCurrentAIProcess();
                pprocess->CheckforNewPackage(pactor, true);
                if (pnext != pActorlist->m_pkNext) {
                    pActorlist = pFollowerExtra->pActorlist;
                }
                else {
                    pActorlist = pnext;
                }
            }
            else {
                pActorlist = pnext;
            }
        }
    }
}

// Runs under kPerceivedActorsLock, which is an addition of this code and not part of the original game (see
// PlayerCharacter.h).
void PlayerCharacter::InsertPerceivedActor(Actor* apActor, bool abAttack, bool abHasLineSightNotDetection) {
    TESActorBaseData* pBaseData = static_cast<TESActorBaseData*>(apActor->GetActorBase());
    if (!pBaseData->GetIsGhost() && !apActor->IsDead(false)) {
        kPerceivedActorsLock.Lock(nullptr);
        CompassTarget* pdata = IsActorPerceived(apActor);
        if (pdata) {
            pdata->bIsUndetected = abHasLineSightNotDetection;
            pdata->bIsHostile = abAttack;
        }
        else {
            if (!pListofPercievedActors) {
                pListofPercievedActors = new BSSimpleList<CompassTarget*>;
            }
            CompassTarget* pData = new CompassTarget;
            pData->pTarget = apActor;
            pData->bIsHostile = abAttack;
            pData->bIsUndetected = abHasLineSightNotDetection;
            pListofPercievedActors->AddHead(pData);
        }
        kPerceivedActorsLock.Unlock();
    }
}

// Under kPerceivedActorsLock (see PlayerCharacter.h).
bool PlayerCharacter::IsDetectedByCompassTargetExceptActor(Actor* apActor) {
    bool bDetected = false;
    kPerceivedActorsLock.Lock(nullptr);
    for (CompassTarget* pCompassTarget : pListofPercievedActors->ItemsUntilNull()) {
        Actor* pTarget = pCompassTarget->pTarget;
        if (pTarget && !pTarget->IsPlayerTeammate() && pTarget != apActor) {
            bool bLineOfSight = pTarget->LineOfSight(false, pPlayer, true, nullptr, false);
            if (pTarget->GetDetectionLevelAgainstActor(false, pPlayer, bLineOfSight, false, false, false, nullptr) >
                0) {
                bDetected = true;
                break;
            }
        }
    }
    kPerceivedActorsLock.Unlock();
    return bDetected;
}

void PlayerCharacter::SetIsDrinkingPlacedWater(bool abDrinking) {
    bAiControlledActivate = abDrinking;
}

void PlayerCharacter::SetDifficultyLevel(GamePlayFormulas::DifficultyLevel aeDifficultyLevel) {
    eDifficultyLevel = aeDifficultyLevel;
}

void PlayerCharacter::SetActorBaseValueF(ActorValue::Index aiWhich, float afValue) {
    bool bOverridden = false;
    GetBaseValueOverride(aiWhich, bOverridden);
    if (bOverridden) {
        SetBaseValueOverride(aiWhich, afValue);
    }
    else {
        GetActorBase()->SetActorValue(aiWhich, afValue);
    }
    Interface::UpdateActorValue(aiWhich);
    CheckSkillLevelChange(aiWhich, true);
}

TESObjectCELL* PlayerCharacter::GetWaterCell(float afCheckRadius) {
    char cClamped = 0;
    if (afCheckRadius > 2048.0) {
        afCheckRadius = 2048.0f;
        cClamped = 1;
    }
    TESObjectCELL* cell = GetParentCell();
    if (!cell || (!cell->GetInterior() && !cell->HasWater() && TES::pInstance->pWorldSpace)) {
        NiPoint3 kPosition = data.kPosition;
        int32_t iX = FloatToInt(kPosition.x) >> 12;
        int32_t iY = FloatToInt(kPosition.y) >> 12;
        NiPoint2 kCellMin(static_cast<float>(iX << 12), static_cast<float>(iY << 12));
        float fMaxY = static_cast<double>(iY << 12) + 4096.0;
        float fMaxX = static_cast<double>(iX << 12) + 4096.0;
        NiPoint2 kCellMax(fMaxX, fMaxY);
        int32_t iOffsetX = 0;
        if (kCellMin.x >= kPosition.x - afCheckRadius) {
            iOffsetX = -1;
        }
        else {
            iOffsetX = kCellMax.x <= kPosition.x + afCheckRadius;
        }
        if (iOffsetX) {
            cell = TES::pInstance->GetGridCellCell(iOffsetX + iX, iY);
        }
        if (!cell || !cell->HasWater()) {
            int32_t iOffsetY;
            if (kCellMin.y >= kPosition.y - afCheckRadius) {
                iOffsetY = -1;
            }
            else {
                iOffsetY = kCellMax.y <= kPosition.y + afCheckRadius;
            }
            if (iOffsetY) {
                cell = TES::pInstance->GetGridCellCell(iX, iOffsetY + iY);
            }
            if ((!cell || !cell->HasWater()) && iOffsetX && iOffsetY) {
                cell = TES::pInstance->GetGridCellCell(iOffsetX + iX, iOffsetY + iY);
                if ((!cell || !cell->HasWater()) && cClamped) {
                    GridCell* pWaterGridCell = TES::pInstance->FindFirstGridCellWithWater();
                    if (pWaterGridCell) {
                        cell = pWaterGridCell->pCell;
                    }
                }
            }
        }
    }
    if (cell && !cell->HasWater()) {
        return nullptr;
    }

    return cell;
}

bool PlayerCharacter::CanFastTravel() {
    float fCheckPerk;
    char msg[504];
    bool bretval = false;
    bool bInterior = pPlayer->GetInterior();
    if (ReferenceProcessLists.AreHostilActorsNear(bInterior)) {
        BSsprintf(msg, 500, sNoFastTravelHostileActorsNear.String());
        Interface::ShowMessage(
            msg,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
            nullptr,
            2.0f,
            false
        );
        return false;
    }

    BSSimpleList<TESObjectREFR*>* preflist = ReferenceProcessLists
                                                 .IsActorTargetingREFinPackage(pPlayer, PACKAGE_TYPE_ALARM, false);
    if (!preflist || preflist->IsEmpty()) {
        BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList();
        if (pCurrentProcess) {
            while (plist && !plist->IsEmpty()) {
                BSSimpleList<ActiveEffect*>* pnext = plist->m_pkNext;
                ActiveEffect* peffect = plist->m_item;
                if (peffect && peffect->IsCausingDamage()) {
                    BSsprintf(msg, 500, sFastTravelNoTravelHealthDamage.String());
                    Interface::ShowMessage(
                        msg,
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                        nullptr,
                        2.0f,
                        false
                    );
                    return false;
                }
                plist = pnext;
            }
        }
        if (static_cast<uint8_t>(pPlayer->GetScriptAllowedFastTravel())) {
            if (pPlayer->IsOverEncumbered()) {
                fCheckPerk = 0.0f;
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_ALWAYS_FAST_TRAVEL, this, &fCheckPerk);
                if (fCheckPerk < 1.0) {
                    BSsprintf(msg, 500, sNoFastTravelOverencumbered.String());
                    Interface::ShowMessage(msg, 0, nullptr, nullptr, 2.0f, false);
                    return false;
                }
            }

            if (pPlayer->GetActorValue(ActorValue::FATIGUE) >= 1 && pPlayer->GetActorValue(ActorValue::HEALTH) >= 1) {
                if (pPlayer->IsJumping()) {
                    BSsprintf(msg, 500, sNoFastTravelInAir.String());
                    Interface::ShowMessage(
                        msg,
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                        nullptr,
                        2.0f,
                        false
                    );
                    return false;
                }

                if (GetParentCell()) {
                    bretval = !GetParentCell()->CantTravelFrom();
                }
                else {
                    TESWorldSpace* pworld = GetWorldSpace();
                    if (pworld) {
                        bretval = !pworld->CantTravelFrom();
                    }
                }
                if (!bretval) {
                    BSsprintf(msg, 500, sNoFastTravelCell.String());
                    Interface::ShowMessage(
                        msg,
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                        nullptr,
                        2.0f,
                        false
                    );
                }
                return bretval;
            }

            BSsprintf(msg, 500, sFastTravelNoTravelHealthDamage.String());
            Interface::ShowMessage(
                msg,
                0,
                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                nullptr,
                2.0f,
                false
            );
            return false;
        }

        BSsprintf(msg, 500, sNoFastTravelScriptBlock.String());
        Interface::ShowMessage(
            msg,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
            nullptr,
            2.0f,
            false
        );
        return false;
    }

    BSsprintf(msg, 500, sNoFastTravelAlarm.String());
    Interface::ShowMessage(
        msg,
        0,
        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
        nullptr,
        2.0f,
        false
    );
    preflist->RemoveAllPointerList();
    if (preflist) {
        delete preflist;
    }
    return false;
}

void PlayerCharacter::Sneak() {
    uint16_t slastMoveMode = GetMoveMode();
    pActorMover->ForceMoveMode(
        (slastMoveMode & (ActorMover::MF_SWIMMING | ActorMover::MF_FALL | ActorMover::MF_SLIDE)) |
        ActorMover::MF_SNEAKING
    );
}

// Runs under kPerceivedActorsLock, which is an addition of this code and not part of the original game (see
// PlayerCharacter.h).
void PlayerCharacter::SpotterEffectNearbyEnemies() {
    kPerceivedActorsLock.Lock(nullptr);
    BSSimpleList<CompassTarget*>* plist = pListofPercievedActors;
    TESEffectShader* pEffectData = pDataHandler->SpotterEffectItem();
    if (pEffectData) {
        while (plist && plist->m_item) {
            CompassTarget* pact = plist->m_item;
            if (pact->bIsHostile) {
                MagicHitEffect* peffect = new MagicShaderHitEffect(pact->pTarget, pEffectData, -1.0f);
                if (peffect->Init()) {
                    ReferenceProcessLists.AddTempEffect(peffect);
                }
                else if (peffect) {
                    delete peffect;
                }
            }
            plist = plist->m_pkNext;
        }
    }
    kPerceivedActorsLock.Unlock();
}

void PlayerCharacter::ShaderNearbyItems() {
    FindObjectIterator kWeaponIterator(WEAP_ID, this);
    FindObjectIterator kAmmoIterator(AMMO_ID, this);
    FindObjectIterator kAlchemyIterator(ALCH_ID, this);
    FindObjectIterator kKeyIterator(KEYM_ID, this);
    float fDuration = fItemDetectionTimer.Float();
    TESObjectREFR* pRef = nullptr;
    ItemChange* pItemChange = nullptr;
    TESEffectShader* pShader = static_cast<TESEffectShader*>(pDataHandler->GetItemDetectedShader());
    if (pShader) {
        while (pLoadedReferenceCollection->GetNextObject(kWeaponIterator, pRef, pItemChange, 3u)) {
            MagicShaderHitEffect* pEffect = new MagicShaderHitEffect(pRef, pShader, fDuration);
            if (pEffect->Init()) {
                ReferenceProcessLists.AddTempEffect(pEffect);
            }
            else if (pEffect) {
                delete pEffect;
            }
        }
        while (pLoadedReferenceCollection->GetNextObject(kAmmoIterator, pRef, pItemChange, 3u)) {
            MagicShaderHitEffect* pEffect = new MagicShaderHitEffect(pRef, pShader, fDuration);
            if (pEffect->Init()) {
                ReferenceProcessLists.AddTempEffect(pEffect);
            }
            else if (pEffect) {
                delete pEffect;
            }
        }
        while (pLoadedReferenceCollection->GetNextObject(kAlchemyIterator, pRef, pItemChange, 3u)) {
            MagicShaderHitEffect* pEffect = new MagicShaderHitEffect(pRef, pShader, fDuration);
            if (pEffect->Init()) {
                ReferenceProcessLists.AddTempEffect(pEffect);
            }
            else if (pEffect) {
                delete pEffect;
            }
        }
        while (pLoadedReferenceCollection->GetNextObject(kKeyIterator, pRef, pItemChange, 3u)) {
            MagicShaderHitEffect* pEffect = new MagicShaderHitEffect(pRef, pShader, fDuration);
            if (pEffect->Init()) {
                ReferenceProcessLists.AddTempEffect(pEffect);
            }
            else if (pEffect) {
                delete pEffect;
            }
        }
    }
}

void PlayerCharacter::UpdatePlayerCombat(float afDelta) {
    if (pCombatGroup && !pCombatGroup->GetTargetCount()) {
        pCombatGroup->RemoveMember(this);
        pCombatGroup = nullptr;
    }

    if (pCombatGroup) {
        fCombatTimer = fCombatTimer + afDelta;
    }
    else {
        fCombatTimer = 0.0;
    }
    uint32_t uiNumLost = 0;
    uint32_t uiCombatantCount = pCombatManager->GetCombatantCount(pPlayer, &uiNumLost);
    if (uiCombatantCount) {
        bPlayerInCombat = true;
        bAllCombatTargetsSearching = uiCombatantCount == uiNumLost;
    }
    else {
        bPlayerInCombat = false;
        bAllCombatTargetsSearching = false;
    }
}

void PlayerCharacter::CheckBorderRegion() {
    if (bEnableBorderRegion.Bool() && !TES::pInstance->CellTestRunning()) {
        bool binBorderRegion = false;
        if (bInBorderContainedCell) {
            binBorderRegion = true;
        }
        else if (pBorderRegions) {
            // The constructor from a world position on the reference's location.
            TESRegionPoint testPoint(*QLocationOnReference());
            uint32_t icount = pBorderRegions->m_usSize;
            for (uint32_t i = 0; i < icount; ++i) {
                TESRegion* pregion = (*pBorderRegions)[i];
                if (!pregion->GetDelete()) {
                    for (BSSimpleList<TESRegionPointList*>* plist = pregion->pPointLists;
                         !binBorderRegion && plist && !plist->IsEmpty();
                         plist = plist->m_pkNext) {
                        if (plist->m_item->IsInsideRegion(&testPoint)) {
                            binBorderRegion = true;
                        }
                    }
                }
                if (binBorderRegion) {
                    break;
                }
            }
        }
        else {
            TESWorldSpace* pworld = GetWorldSpace();
            if (!pworld || !pworld->GetHasBorderRegion()) {
                binBorderRegion = true;
            }
        }
        if (binBorderRegion) {
            SetLastKnownGoodPosition();
        }
        else {
            ReturnToLastKnownGoodPosition(false);
        }
    }
}

void PlayerCharacter::HandleFlycamMovement() {
    static int32_t uiLastFlycamTime;
    NiMatrix3 mat;
    NiMatrix3 zrot;
    Controls* pcontrols = appMain->GetControls();
    float fMoveX = 0.0f;
    float fMoveY = 0.0f;
    int32_t iInputX = 0;
    int32_t iInputY = 0;
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        iInputX = pcontrols->GetControllerValue(0, 9);
        iInputY = pcontrols->GetControllerValue(0, 10);
        if (NiAbs(static_cast<float>(iInputX)) <= 7849.0) {
            iInputX = 0;
        }
        if (NiAbs(static_cast<float>(iInputY)) <= 7849.0) {
            iInputY = 0;
        }
    }
    else {
        iInputX = pcontrols->GetMouseAxis(1);
        iInputY = pcontrols->GetMouseAxis(2);
    }
    if (bInvertYValues.Bool()) {
        iInputY = -iInputY;
    }
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        fUFOCameraHeading = fMouseSensitivity.Float() * static_cast<double>(iInputX) / 1500.0 + fUFOCameraHeading;
        fUFOCameraPitch = fMouseSensitivity.Float() * static_cast<double>(iInputY) / -1500.0 + fUFOCameraPitch;
    }
    else {
        fUFOCameraHeading = fMouseSensitivity.Float() * static_cast<double>(iInputX) + fUFOCameraHeading;
        fUFOCameraPitch = fMouseSensitivity.Float() * static_cast<double>(iInputY) + fUFOCameraPitch;
    }
    zrot.MakeZRotation(fUFOCameraHeading);
    mat.MakeXRotation(fUFOCameraPitch);
    NiMatrix3 rot = zrot * mat;
    NiPoint3 pt = NiPoint3::ZERO;
    iInputX = 0;
    iInputY = 0;
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        iInputX = pcontrols->GetControllerValue(0, 7);
        iInputY = pcontrols->GetControllerValue(0, 8);
        if (NiAbs(static_cast<float>(iInputX)) <= 7849.0) {
            iInputX = 0;
        }
        fMoveX = static_cast<double>(iInputX) / 32767.0;
        if (NiAbs(static_cast<float>(iInputY)) <= 7849.0) {
            iInputY = 0;
        }
        fMoveY = static_cast<double>(iInputY) / 32767.0;
        pt.x = fMoveX * 15.0 + pt.x;
        pt.y = fMoveY * 15.0 + pt.y;
    }
    else {
        if (pcontrols->GetUserAction(Controls::UACTN_FORWARD, 0)) {
            pt.y = pt.y + 10.0;
        }
        if (pcontrols->GetUserAction(Controls::UACTN_BACK, 0)) {
            pt.y = pt.y - 10.0;
        }
        if (pcontrols->GetUserAction(Controls::UACTN_SLDRIGHT, 0)) {
            pt.x = pt.x + 10.0;
        }
        if (pcontrols->GetUserAction(Controls::UACTN_SLDLEFT, 0)) {
            pt.x = pt.x - 10.0;
        }
        if (pcontrols->GetUserAction(Controls::UACTN_RUN, 0)) {
            pt = pt * 2.0f;
        }
    }
    uint32_t uiTime = BSGetTickCount();
    float fElapsed = 0.0f;
    if (uiLastFlycamTime) {
        fElapsed = static_cast<double>(uiTime - uiLastFlycamTime) / 1000.0;
        fElapsed = BSMIN(fElapsed, 1.0f);
    }
    uiLastFlycamTime = uiTime;
    float fStep = fElapsed * 10.0;
    pt *= fStep;
    pt = rot * pt;
    pt *= fUFOCamSpeedMult.Float();
    UFOCameraPos += pt;
    BSSceneGraph* pWorldRoot = Main::GetWorldSceneGraph();
    pWorldRoot->GetCameraRoot()->SetTranslate(UFOCameraPos);
    pWorldRoot->GetCameraRoot()->SetRotate(rot);
    NiUpdateData UpdateData(0.0f, false, false);
    pWorldRoot->GetCameraRoot()->Update(UpdateData);
    if (GetWorldSpace()) {
        if (GetWorldSpace()->GetTerrainManager()) {
            GetWorldSpace()->GetTerrainManager()->Update(UFOCameraPos, BGSTerrainManager::UPDATE_ALL);
            GetWorldSpace()->GetTerrainManager()->UpdateMorphParameters(UFOCameraPos);
        }
    }
}

void PlayerCharacter::ReturnToLastKnownGoodPosition(bool abPositionPlayer) {
    if (pLastKnownGoodLocation) {
        if (abPositionPlayer) {
            Interface::ShowMessage(
                sPlayerLeavingBorderRegion.String(),
                0,
                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                nullptr,
                2.0f,
                false
            );
            TESObjectCELL* pcell = nullptr;
            TESWorldSpace* pworld = nullptr;
            if (pLastKnownGoodLocation) {
                int32_t iFormType = pLastKnownGoodLocation->GetFormType();
                if (iFormType == 57) {
                    pcell = static_cast<TESObjectCELL*>(pLastKnownGoodLocation);
                }
                else if (iFormType == 65) {
                    pworld = static_cast<TESWorldSpace*>(pLastKnownGoodLocation);
                }
            }
            if (pworld) {
                NiPoint3 kAngle = GetAngleOnReference();
                NiPoint3 kPosition = GetLastKnownGoodPosition();
                PositionPlayerExterior(kPosition, kAngle, pworld, false);
            }
            else if (pcell) {
                NiPoint3 kAngle = GetAngleOnReference();
                NiPoint3 kPosition = GetLastKnownGoodPosition();
                PositionPlayer(kPosition, kAngle, pcell, false);
            }
        }
        else {
            Actor* pactor = this;
            Interface::ShowMessage(
                sPlayerLeavingBorderRegion.String(),
                0,
                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                nullptr,
                2.0f,
                false
            );
            bhkCharacterController* pcon = GetCharController();
            if (pcon && !pcon->QSupported()) {
                LastKnownGoodPosition.z = pactor->QLocationOnReference()->z;
            }

            pactor->SetLocationOnReference(LastKnownGoodPosition);
            if (pcon) {
                pcon->SetPosition(LastKnownGoodPosition);
            }
            TESObjectCELL* pcell = nullptr;
            TESWorldSpace* pworld = nullptr;
            if (pLastKnownGoodLocation) {
                int32_t iFormType = pLastKnownGoodLocation->GetFormType();
                if (iFormType == 57) {
                    pcell = static_cast<TESObjectCELL*>(pLastKnownGoodLocation);
                }
                else if (iFormType == 65) {
                    pworld = static_cast<TESWorldSpace*>(pLastKnownGoodLocation);
                }
            }
            if (pworld) {
                int32_t icellx = FloatToInt(LastKnownGoodPosition.x) >> 12;
                int32_t icelly = FloatToInt(LastKnownGoodPosition.y) >> 12;
                pcell = pDataHandler->GetCellFromCellCoord(icellx, icelly, pworld, false);
            }
            if (pcell) {
                pactor->SetParentCell(pcell);
                SetParentCell(pcell);
            }
            NiAVObject* pobj3D = Get3D(false);
            NiAVObject* pobj1st3D = Get3D(true);
            pobj3D->SetTranslate(LastKnownGoodPosition);
            pobj1st3D->SetTranslate(LastKnownGoodPosition);
            bhkNiCollisionObject::ResetSim(pobj3D, true);
            bhkNiCollisionObject::ResetSim(pobj1st3D, true);
            NiUpdateData UpdateData1(0.0f, false, false);
            pobj3D->Update(UpdateData1);
            NiUpdateData UpdateData2(0.0f, false, false);
            pobj1st3D->Update(UpdateData2);
        }
    }
}

void PlayerCharacter::PlayerSleep() {
    BSAudio::QInstance().EndAllSoundsOfType(4u);
    InterruptCast();
    float ftime = 3600.0 / appCalendar.GetTimeScale();
    float fNewClock = ReferenceProcessLists.GetSystemTimeClock() + ftime;
    ReferenceProcessLists.SetSystemTimeClock(fNewClock);
    appCalendar.Update(ftime);
    if (PlayerCharacter::bSleepRunUpdates) {
        if (GetInChargen()) {
            UpdateTarget(ftime);
        }
        else if (ReferenceProcessLists.bRunScheduals) {
            ReferenceProcessLists.kUpdateLock.Lock(nullptr);
            ReferenceProcessLists.UpdateHighListPackages(3600.0f, true);
            ReferenceProcessLists.UpdateMiddleHighList(3600.0f, true);
            ReferenceProcessLists.UpdateMiddleLowList(3600.0f, true);
            ReferenceProcessLists.UpdateLowList(3600.0f, true);
            ReferenceProcessLists.kUpdateLock.Unlock();
        }
        pPlayer->RestoreHealth(ftime);
        if (IsHardcore() && pPlayer->IsSleeping()) {
            float fCurrentSLP = GetActorFloatValue(ActorValue::SLEEPDEPREVATION);
            float fSleepRestored = fHCSleepRestorationMod.Float();
            if (fCurrentSLP < static_cast<double>(fSleepRestored)) {
                fSleepRestored = fCurrentSLP;
            }
            PermanentModActorValue(ActorValue::SLEEPDEPREVATION, -fSleepRestored, static_cast<Actor*>(nullptr));
        }
    }
    float fDelta = appTimer.fDelta;
    TES::pInstance->pSky->Update(fDelta);
    ReferenceProcessLists.UpdateTheCrimeList();
    UpdateMagic(true, false);
    if (--iSleepTime <= 0) {
        PlayerCharacter::bSleepRunUpdates = true;
        if (bIsSleeping && !IsHardcore()) {
            RestoreFullHealthAndConditions();
        }
        bIsSleeping = false;
    }
}

void PlayerCharacter::UpdateTemp1stPerson() {
    if (bTemp1stPerson && !IsPipboyActive() && pVATS.eMode != VATS::VATS_PLAYBACK) {
        if (bDelayVATSMenu ||
            !GetIronSights() ||
            !pCurrentProcess->GetCurrentWeapon() ||
            !static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj)
                 ->GetScopeModel()
                 ->GetModelLength()) {
            bTemp1stPerson = false;
            bSkipAlphaFadeOut = true;
            if (bTemp1stPersonSwitchBack) {
                SetFirstPerson(false);
            }
            bTemp1stPersonSwitchBack = false;
        }
    }
}

NiNode* PlayerCharacter::CloneInventory3D(bool abFlag) {
    NiMatrix3 rot;
    if (!abFlag || spInventoryPC.m_pObject) {
        if (!abFlag && spInventoryPC.m_pObject) {
            Animation* pAnimation = this->GetInventoryAnimation();
            if (pAnimation) {
                delete pAnimation;
            }
            pInventoryAnimation = nullptr;
            NiAVObject* pav = spInventoryPC->GetObjectByName(FixedStrings::QRaceHeadBiped());
            BSFaceGenNiNode* pfacenode = static_cast<BSFaceGenNiNode*>(
                NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, pav)
            );
            if (pfacenode) {
                pfacenode->UpdateAllChildrenMorphData(true);
            }
            pav = spInventoryPC->GetObjectByName(FixedStrings::QRaceHeadSkinned());
            pfacenode = static_cast<BSFaceGenNiNode*>(NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, pav));
            if (pfacenode) {
                pfacenode->UpdateAllChildrenMorphData(true);
            }
            spInventoryPC = static_cast<NiNode*>(nullptr);
            MagicShaderHitEffect* peffect = GetMagicShaderHitEffect();
            if (peffect) {
                delete peffect;
                SetInventoryWeaponEffect(nullptr);
            }
        }
    }
    else {
        char cModelPath[268];
        char cFilePath[264];
        bool boldForce = BSShaderPPLightingProperty::GetForceNoProjectedShadowTexture();
        BSShaderPPLightingProperty::SetForceNoProjectedShadowTexture(true);
        ReferenceProcessLists.FinishGeometryDecalEffects();
        ReferenceProcessLists.UpdateTempEffects(0.0f);
        bhkForceController::RemoveForce(Get3D());
        NiAVObject* proot = Get3D(false);
        bhkForceController::RemoveForce(proot);
        proot = Get3D(false);
        NiPointer<NiTimeController> spmanager(proot->GetControllers());
        proot->RemoveAllControllers();
        BSCloningProcess cloneData(1.0f);
        NiObject* pobj = proot->Clone(cloneData);
        NiNode* pclonednode = pobj ? pobj->IsNode() : nullptr;
        if (pclonednode) {
            BSShaderManager::GetShadowSceneNode(1)->FindAndCullLight(pclonednode, 0);
        }
        proot->SetControllers(spmanager.m_pObject);
        spInventoryPC = pobj ? pobj->IsNode() : nullptr;
        spInventoryPC->RemoveAllControllers();
        Animation::ClearControllersInterpolators(spInventoryPC.m_pObject);
        spInventoryPC->RemoveProperty(NiAlphaProperty::GetType());
        spInventoryPC->UpdateProperties();
        BSShaderUtil::RecursiveSetPropertyFadeAlpha(spInventoryPC.m_pObject, 1.0f);
        RecursiveReadyInventoryNode(spInventoryPC.m_pObject);
        spInventoryPC->SetTranslate(NiPoint3::ZERO);
        static_cast<TESNPC*>(GetObjectReference())->TESNPC::FixDisplayedHeadParts(this, spInventoryPC.m_pObject);
        rot = NiMatrix3::IDENTITY;
        spInventoryPC->SetRotate(MultipleMatrixByRace(rot));
        NiPointer<BSFaceGenAnimationData> spanimdata(new BSFaceGenAnimationData);
        NiAVObject* pav = spInventoryPC->GetObjectByName(FixedStrings::QRaceHeadBiped());
        BSFaceGenNiNode* pfacenode = static_cast<BSFaceGenNiNode*>(
            NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, pav)
        );
        if (pfacenode) {
            pfacenode->SetAnimationData(spanimdata.m_pObject);
        }
        pav = spInventoryPC->GetObjectByName(FixedStrings::QRaceHeadSkinned());
        pfacenode = static_cast<BSFaceGenNiNode*>(NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, pav));
        if (pfacenode) {
            pfacenode->SetAnimationData(spanimdata.m_pObject);
            pfacenode->UpdateAllChildrenMorphData(true);
        }
        spanimdata = static_cast<BSFaceGenAnimationData*>(nullptr);
        NiAVObject* parrowObj = spInventoryPC->GetObjectByName(FixedStrings::QArrowBone());
        NiNode* parrowBone = parrowObj ? parrowObj->IsNode() : nullptr;
        if (parrowBone) {
            for (int32_t n = 0; n < parrowBone->GetArrayCount(); ++n) {
                parrowBone->DetachChildAt(static_cast<uint32_t>(n));
            }
        }
        NiAVObject* pmagicObj = spInventoryPC->GetObjectByName(FixedStrings::QMagicNode());
        NiNode* pmagicBone = pmagicObj ? pmagicObj->IsNode() : nullptr;
        if (pmagicBone) {
            for (int32_t n = 0; n < pmagicBone->GetArrayCount(); ++n) {
                pmagicBone->DetachChildAt(static_cast<uint32_t>(n));
            }
        }
        BSShaderPPLightingProperty::SetRefractionRecurse(spInventoryPC.m_pObject, 0, 0.0f, 0, 0.0f, 0);
        pInventoryAnimation = new Animation;
        BSsprintf(cModelPath, 260, "%s", GetModel());
        char* pslash = std::strrchr(cModelPath, 92);
        if (!pslash) {
            return nullptr;
        }
        BSstrcpy(pslash, 260 - (pslash - cModelPath), "\\MTIdle.KF");
        BSsprintf(cFilePath, 260, "Data\\%s\\%s", "Meshes", cModelPath);
        const char* pModel = GetModel();
        BSSimpleList<const char*>* pkfFiles = pModelLoader->BuildFileList(cFilePath, pModel, nullptr);
        *pslash = 0;
        // Each weapon kind's idle and torch idle file, when it exists, goes into the list as a 260-byte path the list
        // owns.
        for (int32_t n = 1; n < 12; ++n) {
            char* pIdleFile;
            char* pTorchIdleFile;
            BSsprintf(cFilePath, 260, "Data\\%s\\%s\\%sIdle.KF", "Meshes", cModelPath, animWeaponInfo[n]);
            if (FileFinder::GetSingleton()->Exist(cFilePath, nullptr, 0, FileFinder::ARCHIVE_TYPE_ANY)) {
                pIdleFile = static_cast<char*>(ourMalloc(260u));
                BSstrcpy(pIdleFile, 260, GetModel());
                pslash = std::strrchr(pIdleFile, 92);
                if (pslash) {
                    ++pslash;
                    BSsprintf(pslash, 260 - (pslash - pIdleFile + 1), "%sIdle.KF", animWeaponInfo[n]);
                    pkfFiles->AddTail(pIdleFile);
                }
            }
            BSsprintf(cFilePath, 260, "Data\\%s\\%s\\%sTorchIdle.KF", "Meshes", cModelPath, animWeaponInfo[n]);
            if (FileFinder::GetSingleton()->Exist(cFilePath, nullptr, 0, FileFinder::ARCHIVE_TYPE_ANY)) {
                pTorchIdleFile = static_cast<char*>(ourMalloc(260u));
                BSstrcpy(pTorchIdleFile, 260, GetModel());
                pslash = std::strrchr(pTorchIdleFile, 92);
                if (pslash) {
                    ++pslash;
                    BSsprintf(pslash, 260 - (pslash - pTorchIdleFile + 1), "%sTorchIdle.KF", animWeaponInfo[n]);
                    pkfFiles->AddTail(pTorchIdleFile);
                }
            }
        }
        this->GetInventoryAnimation()->Init(pkfFiles, spInventoryPC.m_pObject, this, true);
        m_Extra.GetLight();
        spInventoryPC->UpdateWorldBound();
        if (pCurrentProcess) {
            pCurrentProcess->SetShouldRefreshMagicShadersEffects(true, false, false);
        }
        BSShaderPPLightingProperty::SetForceNoProjectedShadowTexture(boldForce);
        NiAVObject* pspecialFace = spInventoryPC->GetObjectByName(FixedStrings::QHeadMeshExp());
        if (pspecialFace) {
            pspecialFace->SetAppCulled(true);
        }
    }
    return spInventoryPC.m_pObject;
}

bool PlayerCharacter::AddQuestStageItem(TESQuestStageItem* apNewItem) {
    if (!apNewItem) {
        return false;
    }
    BSSimpleList<TESQuestStageItem*>* pIter = GetQuestLog();
    while (pIter && pIter->m_item) {
        TESQuestStageItem* pItem = pIter->m_item;
        pIter = pIter->m_pkNext;
        if (pItem == apNewItem) {
            if (!pItem->GetOwner()->GetAllowRepeatStages()) {
                return false;
            }
        }
    }
    listQuestLog.AddHead(apNewItem);
    if (!Script::GetProcessScripts()) {
        return true;
    }
    if (apNewItem->GetEndQuest()) {
        Interface::PlayMenuSound(9);
        TESQuest* pOwner = apNewItem->GetOwner();
        if (pOwner == pPlayer->GetActiveQuest() && apNewItem->m_pNextQuest) {
            apNewItem->m_pNextQuest->SetEnabled(1u);
        }
    }
    return true;
}

void PlayerCharacter::RemovePlayerMapMarker() {
    if (pPlayerMapMarker) {
        delete pPlayerMapMarker;
    }
    pPlayerMapMarker = nullptr;
    PlayerMarkerPath.Clear();
}

// The "Bip01 Speaker" string allocated here is never freed.
void PlayerCharacter::FocusOnActor(MobileObject* apActor, float afZoomPercent, bool abOnlyZoom) {
    if (apActor) {
        float fclampedLow = afZoomPercent < 0.0 ? 0.0f : afZoomPercent;
        float fzoomPercent = fclampedLow > 1.0 ? 1.0f : fclampedLow;
        if (PlayerCharacter::bVanityMode) {
            StopVanityMode();
            Show1stPerson(!b3rdPerson);
        }
        if (GetParentCell() && GetParentCell()->GetInterior()) {
            TESObjectCELL* pCell = apActor->GetParentCell();
            if (pCell != GetParentCell()) {
                return;
            }
        }
        else {
            TESWorldSpace* pWorld = apActor->GetWorldSpace();
            if (pWorld != GetWorldSpace()) {
                return;
            }
        }
        NiPoint3 src = p1stPersonCamera->GetWorldTranslate();
        NiPoint3 target;
        bool bcalculated = false;
        NiAVObject* pav = nullptr;
        if (apActor->IsTalkingActivator()) {
            TESObjectREFR* pref = apActor->GetTalkingActivator();
            pav = pref->Get3D();
        }
        else {
            pav = apActor->Get3D();
        }
        bool bvalidTarget = true;
        if (pav) {
            BSFaceGenNiNode* pfaceGenHead = apActor->GetFaceNodeSkinned(nullptr);
            NiNode* phead = nullptr;
            if (!pfaceGenHead) {
                NiAVObject* pheadObject = BSUtilities::GetObjectByName(pav, *FixedStrings::GetBip01_Head());
                phead = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pheadObject));
            }
            NiFixedString* pSpeaker = new NiFixedString("Bip01 Speaker");
            if (!phead) {
                NiAVObject* pspeakerObject = BSUtilities::GetObjectByName(pav, *pSpeaker);
                phead = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pspeakerObject));
            }
            if (pfaceGenHead || phead) {
                if (pfaceGenHead && pfaceGenHead->GetAppCulled()) {
                    pfaceGenHead->SetAppCulled(false);
                    NiUpdateData UpdateData(0.0f, false, false);
                    pfaceGenHead->Update(UpdateData);
                    pfaceGenHead->UpdateWorldBound();
                    pfaceGenHead->SetAppCulled(true);
                }

                NiBound headBound;
                if (pfaceGenHead) {
                    headBound.Copy(&pfaceGenHead->GetWorldBound());
                }
                else {
                    headBound.SetCenter(phead->GetWorldTranslate());
                    headBound.SetRadius(32.0f);
                }
                target = headBound.m_kCenter;
                target.z = target.z + fDlgLookAdj.Float();
                src -= target;
                NiCamera* pcam = Main::GetWorldSceneGraph()->GetCamera();
                if (pcam) {
                    bcalculated = true;
                    float fworldFOVDelta = 0.0f;
                    float f1stPersonFOVDelta = 0.0f;
                    if (PlayerCharacter::fDialogZoomPercent == 0.0) {
                        double fradius = headBound.m_fRadius;
                        double ffocus = fDlgFocus.Float() * fradius;
                        ftan = ffocus / src.Length();
                        fzoomedFOV = std::atan(ftan) * 100.0;
                        if (BSSceneGraph::fDefaultFOV.Float() < static_cast<double>(fzoomedFOV)) {
                            fzoomedFOV = BSSceneGraph::fDefaultFOV.Float();
                        }
                    }
                    fworldFOVDelta = fzoomedFOV - fWorldFOV;
                    f1stPersonFOVDelta = fzoomedFOV - f1stPersonFOV;
                    double fdelta = appTimer.fDelta;
                    float fstep = fdelta / ((1.0 - PlayerCharacter::fDialogZoomPercent) * fDialogZoomInSeconds.Float());
                    fWorldFOV = BSMIN(fstep, 1.0f) * fworldFOVDelta + fWorldFOV;
                    double fdelta1st = appTimer.fDelta;
                    float fstep1st = fdelta1st /
                        ((1.0 - PlayerCharacter::fDialogZoomPercent) * fDialogZoomInSeconds.Float());
                    f1stPersonFOV = BSMIN(fstep1st, 1.0f) * f1stPersonFOVDelta + f1stPersonFOV;
                }
            }
            else {
                if (plastWarningActor != apActor) {
                    Warning(
                        "DIALOGUE: PlayerCharacter::FocusOnActor( ): Attempting to talk to an object '%s' without "
                        "a 'Bip01 Head'.",
                        apActor->GetModel()
                    );
                }
                plastWarningActor = apActor;
                bvalidTarget = false;
            }
            if (!abOnlyZoom && bvalidTarget) {
                float ffovMult = f1stPersonFOV / BSSceneGraph::fDefaultFOV.Float();
                double flength = src.Length();
                float fsinPitch = src.z / flength;
                float ftargetAngle = std::asin(fsinPitch);
                float fangleDelta = ftargetAngle - GetLooking();
                double fabsLook = std::fabs(fangleDelta);
                if (fDlgLookDegStart.Float() * 0.01745329238474369 * ffovMult < fabsLook) {
                    badjustLooking = true;
                }
                if (PlayerCharacter::fDialogZoomPercent < 1.0) {
                    badjustLooking = true;
                }
                if (badjustLooking) {
                    if (PlayerCharacter::fDialogZoomPercent < 1.0) {
                        double fdelta = appTimer.fDelta;
                        float fstep = fdelta /
                            ((1.0 - PlayerCharacter::fDialogZoomPercent) * fDialogZoomInSeconds.Float());
                        float fmod = BSMIN(fstep, 1.0f) * fangleDelta;
                        ModLooking(fmod);
                    }
                    else {
                        double fscaled = fangleDelta * fDlgLookMult.Float();
                        float fmod = appTimer.fDelta * fscaled;
                        ModLooking(fmod);
                    }
                    double fabsLookAfter = std::fabs(fangleDelta);
                    if (fDlgLookDegStop.Float() * 0.01745329238474369 * ffovMult > fabsLookAfter) {
                        badjustLooking = false;
                    }
                }
                double fzAngle = GetZAngleFromVector(-src);
                for (fangleDelta = fzAngle - GetHeading(false); fangleDelta > 3.1415927410125732;
                     fangleDelta = fangleDelta - 6.2831854820251465) {
                    ;
                }
                while (fangleDelta < -3.1415927410125732) {
                    fangleDelta = fangleDelta + 6.2831854820251465;
                }
                double fabsHeading = std::fabs(fangleDelta);
                if (fDlgHeadingDegStart.Float() * 0.01745329238474369 * ffovMult < fabsHeading) {
                    badjustHeading = true;
                }
                if (PlayerCharacter::fDialogZoomPercent < 1.0) {
                    badjustHeading = true;
                }
                if (badjustHeading) {
                    if (GetSitSleepState()) {
                        double fsitAngle = GetZAngleFromVector(-src);
                        for (fangleDelta = fsitAngle - Actor::GetHeading(false) - fSitHeadingDelta;
                             fangleDelta > 3.1415927410125732;
                             fangleDelta = fangleDelta - 6.2831854820251465) {
                            ;
                        }
                        while (fangleDelta < -3.1415927410125732) {
                            fangleDelta = fangleDelta + 6.2831854820251465;
                        }
                        if (PlayerCharacter::fDialogZoomPercent < 1.0) {
                            double fdelta = appTimer.fDelta;
                            float fstep = fdelta /
                                ((1.0 - PlayerCharacter::fDialogZoomPercent) * fDialogZoomInSeconds.Float());
                            double fstepMin = BSMIN(fstep, 1.0f);
                            fSitHeadingDelta = fstepMin * fangleDelta + fSitHeadingDelta;
                        }
                        else {
                            double fscaled = fangleDelta * fDlgLookMult.Float();
                            fSitHeadingDelta = appTimer.fDelta * fscaled + fSitHeadingDelta;
                        }
                    }
                    else if (PlayerCharacter::fDialogZoomPercent < 1.0) {
                        double fdelta = appTimer.fDelta;
                        float fstep = fdelta /
                            ((1.0 - PlayerCharacter::fDialogZoomPercent) * fDialogZoomInSeconds.Float());
                        double fstepMin = BSMIN(fstep, 1.0f);
                        float fmod = fstepMin * fangleDelta;
                        ModHeading(fmod);
                    }
                    else {
                        double fscaled = fangleDelta * fDlgLookMult.Float();
                        float fmod = appTimer.fDelta * fscaled;
                        ModHeading(fmod);
                    }
                    double fabsHeadingAfter = std::fabs(fangleDelta);
                    if (fDlgHeadingDegStop.Float() * 0.01745329238474369 * ffovMult > fabsHeadingAfter) {
                        badjustHeading = false;
                    }
                }
                PlayerCharacter::fCameraHeading = GetHeading(false);
                PlayerCharacter::fCameraLooking = GetLooking();
                SetFirstPerson(true);
                UpdateFirstPersonZoom();
                UpdateTemp3rdPerson();
                static_cast<PlayerMover*>(pActorMover)->PlayerMover::ClearMoveMode(63u);
                bhkWorld::ClearPickData();
                b3rdPerson = true;
                UpdateAnimation();
                PickAnimations(1.0f, 1.0f);
                Animation* pAnim3rd = GetAnimation(false);
                UpdateAnimationMovement(pAnim3rd, 0.0f);
                b3rdPerson = false;
                float fupdateDelta = appTimer.fDelta;
                Character::Update(fupdateDelta);
                float fmoveDelta = appTimer.fDelta;
                Animation* pAnim = GetAnimation(!b3rdPerson);
                UpdateAnimationMovement(pAnim, fmoveDelta);
            }
            if (!bcalculated && pav) {
                int32_t fov = static_cast<int32_t>((src.Length() - 45.0) / 100.0 * 25.0) + 15;
                SetFOV(static_cast<float>(fov));
            }
        }
        PlayerCharacter::fDialogZoomPercent = fzoomPercent;
    }
}

void PlayerCharacter::InitiateSitSleepPackage(TESObjectREFR* apCurrentFurniture) {
    if (!pPlayer->IsPathingComplete()) {
        pPlayer->pActorMover->ClearCurrentData();
    }
    SetAIControlledToPosition(true);
    if (pCurrentProcess->GetAnimAction() == ANIM_ACTION_BLOCK) {
        SetBlock(false);
    }
    TESPackage* pPackage = TESPackage::CreatePackage(PACKAGE_TYPE_TRAVEL);
    pPackage->SetPackType(PACKAGE_TYPE_TRAVEL);
    pPackage->SetMustReachLocation(false);
    pPackage->SetMustComplete(true);
    PackageLocation* pLocation = new PackageLocation;
    pLocation->SetLocReference(apCurrentFurniture);
    pPackage->SetPackageLocation(pLocation);
    if (pLocation) {
        delete pLocation;
    }
    pPackage->SetProcedureType(PACKAGE_PROCEDURE_EXPLORE_TRAVEL);
    PutCreatedPackage(pPackage, false, true);
}

bool PlayerCharacter::IsActorFollowingEscortingPlayer(Actor* apActor, bool bAll) {
    if (apActor->IsDead(false) || apActor->GetDisabled() || apActor->IsParalyzed() || apActor->IsRestrained()) {
        return false;
    }
    apActor->pCurrentProcess->CheckforNewPackage(apActor, true);
    TESPackage* pPack = apActor->GetPackageSetAsPcurrent();
    ActorPackage* pCurrentPackage = &apActor->pCurrentProcess->CurrentPackage;
    if (!bAll) {
        if (pPack &&
            (pPack->GetPackType() == PACKAGE_TYPE_FOLLOW || pPack->GetPackType() == PACKAGE_TYPE_ACCOMPANY) &&
            pPack->GetPackageTarget()) {
            TESObjectREFR* pref = apActor->pCurrentProcess->CurrentPackage.pTarget;
            if (!pref) {
                TESBoundObject* pobject = static_cast<TESBoundObject*>(pPack->GetPackageTarget()->GetTargObject());
                if (pobject && pobject == pPlayer->GetObjectReference()) {
                    pref = pPlayer;
                }
            }
            if (pref == pPlayer) {
                return !pPack->GetWaiting();
            }
        }

        return false;
    }
    TESObjectREFR* pref = nullptr;
    if (pCurrentPackage && pCurrentPackage->pTarget) {
        pref = pCurrentPackage->pTarget;
        if (!pref) {
            TESBoundObject* pobject = static_cast<TESBoundObject*>(pPack->GetPackageTarget()->GetTargObject());
            if (pobject && pobject == pPlayer->GetObjectReference()) {
                pref = pPlayer;
            }
        }
    }

    if (!pPack || pPack->GetLocationReference(apActor) != pPlayer) {
        if (pref != pPlayer) {
            if (apActor->pCurrentProcess->GetTarget() != pPlayer) {
                return false;
            }
        }
        if (pPack->GetPackType() == PACKAGE_TYPE_AMBUSH) {
            return false;
        }
    }
    return pref != pPlayer || !apActor->IsWaiting();
}

bool PlayerCharacter::ReloadLoop(
    TESObjectWEAP* apWeapon,
    int32_t aiReloadAnim,
    bool bUseModValue,
    bool bFromInventory
) {
    TESBoundObject* pammo = apWeapon->GetCurrentAmmo(pPlayer);
    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(this);
    pinventory->GetObjectCount(pammo);
    ItemChange* pweaponItem = pCurrentProcess->GetCurrentWeapon();
    ItemChange* pAmmoItem = pCurrentProcess->GetCurrentAmmo();
    GetAnimation(false);
    if (apWeapon && pammo && apWeapon != pammo) {
        ItemChange* pWeaponTest = pinventory->GetInventoryItem(apWeapon, 0);
        if (pWeaponTest->GetHotKey() >= 0) {
            int32_t iHK = pWeaponTest->GetHotKey();
            TESBoundObject* pAmmoOverride = static_cast<TESBoundObject*>(pPlayer->GetAmmoForHotKey(iHK));
            if (pAmmoOverride) {
                pammo = pAmmoOverride;
            }
        }
        if (pWeaponTest) {
            delete pWeaponTest;
        }
    }

    int32_t icount = pinventory->GetObjectCount(pammo);
    if (!pAmmoItem || pAmmoItem->pContainerObj != pammo) {
        ItemChange* parrows = new ItemChange(pammo, icount);
        pCurrentProcess->SetCurrentAmmo(parrows);
        pCurrentProcess->GetCurrentAmmo()->iNumber = 0;
        pAmmoItem = parrows;
    }
    if (pweaponItem && pAmmoItem) {
        uint32_t uiAmmoCount = pAmmoItem->iNumber;
        bool bHasClipMod = pweaponItem->HasModEffectActive(2);
        if (uiAmmoCount == apWeapon->GetFormClipRounds(bHasClipMod)) {
            return false;
        }
        if (icount == pAmmoItem->iNumber) {
            return false;
        }
    }
    if (apWeapon && pammo != apWeapon) {
        int32_t iClipRounds = apWeapon->GetFormClipRounds(bUseModValue);
        icount = BSMIN(icount, iClipRounds);
    }

    if (aiReloadAnim) {
        if (aiReloadAnim == 1) {
            Animation* panim = GetAnimation(false);
            ANIM_GROUP_ENUM eanimGroup = static_cast<ANIM_GROUP_ENUM>(apWeapon->GetReloadAnim(bUseModValue) - 23);
            uint16_t sanimGroup = GetAnimGroup(eanimGroup, nullptr, false, panim);
            if (TESAnimGroup::GetType(sanimGroup) == eanimGroup) {
                panim->QueueReloadAnim(sanimGroup);
            }
            return true;
        }

        if (aiReloadAnim == 2) {
            ANIM_GROUP_ENUM eanimGroup = static_cast<ANIM_GROUP_ENUM>(apWeapon->GetReloadAnim(bUseModValue) - 23);
            uint16_t sanimGroup = GetAnimGroup(eanimGroup, nullptr, false, nullptr);
            if (TESAnimGroup::GetType(sanimGroup) == eanimGroup) {
                Animation* panim = GetAnimation(false);
                panim->PlayGroup(sanimGroup, ACTION_START, -1, AGS_NONE);
                StartAnimOn1stPerson(sanimGroup, ACTION_START);
                SetAnimAction(ANIM_ACTION_RELOAD_LOOP, panim->GetCurrentSequence(AGS_WEAPON));
            }
            return true;
        }

        return false;
    }

    int32_t iCurrentCount = 1;
    if (pCurrentProcess->GetCurrentAmmo()) {
        iCurrentCount = pCurrentProcess->GetCurrentAmmo()->iNumber + 1;
    }
    if (bFromInventory) {
        iCurrentCount = icount;
    }
    QueueEquipObject(pammo, iCurrentCount, nullptr, true, false, false);
    return true;
}

void PlayerCharacter::CheckPoisonWeapon(AlchemyItem* apPoison) {
    if (apPoison) {
        ItemChange* pweaponItem = pCurrentProcess->GetCurrentWeapon();
        TESObjectWEAP* pweapon = nullptr;
        if (pweaponItem) {
            pweapon = dynamic_cast<TESObjectWEAP*>(pweaponItem->pContainerObj);
        }
        if (pweapon &&
            (pweapon->GetWeaponSkill() == ActorValue::UNARMED ||
             pweapon->GetWeaponSkill() == ActorValue::MELEE_WEAPONS)) {
            if (pweaponItem->GetPoison()) {
                const char* pcOk = sOk.String();
                Interface::CreateMessageMenu(
                    sPoisonAlreadyPoisonedMessage.String(),
                    static_cast<const char*>(nullptr),
                    static_cast<const char*>(nullptr),
                    &PlayerCharacter::PoisonWeapon,
                    1,
                    23,
                    0.0f,
                    0.0f,
                    pcOk,
                    nullptr
                );
            }
            else {
                char cMessage[256];
                pPendingPoison = apPoison;
                std::sprintf(cMessage, "%s%s?", sPoisonConfirmMessage.String(), pweapon->GetFullName());
                const char* pcNo = sNo.String();
                Interface::CreateMessageMenu(
                    cMessage,
                    static_cast<const char*>(nullptr),
                    static_cast<const char*>(nullptr),
                    &PlayerCharacter::PoisonWeapon,
                    2,
                    23,
                    0.0f,
                    0.0f,
                    sYes.String(),
                    pcNo,
                    nullptr
                );
            }
        }
        else {
            const char* pcOk = sOk.String();
            Interface::CreateMessageMenu(
                sPoisonNoWeaponMessage.String(),
                static_cast<const char*>(nullptr),
                static_cast<const char*>(nullptr),
                &PlayerCharacter::PoisonWeapon,
                1,
                23,
                0.0f,
                0.0f,
                pcOk,
                nullptr
            );
        }
    }
}

void PlayerCharacter::AddQueuedEnchantment(TESBoundObject* apObject, ExtraDataList* apExtra) {
    if (apObject->GetFormType() == ARMO_ID || apObject->GetFormType() == CLOT_ID) {
        TESEnchantableForm* penchantable = dynamic_cast<TESEnchantableForm*>(apObject);
        EnchantmentItem* pEnchantment = penchantable ? penchantable->pFormEnchanting : nullptr;
        if (pEnchantment) {
            CastWornEnchantment(pEnchantment, apObject, false);
            ProcessEffectsFromItem(pEnchantment);
        }
    }
}

void PlayerCharacter::RemoveQueuedEnchantment(TESBoundObject* apObject, ExtraDataList* apExtra) {
    if ((apObject->GetFormType() == ARMO_ID || apObject->GetFormType() == CLOT_ID) && QueuedWornEnchantments) {
        TESEnchantableForm* penchantable = dynamic_cast<TESEnchantableForm*>(apObject);
        EnchantmentItem* pEnchantment = penchantable ? penchantable->pFormEnchanting : nullptr;
        if (pEnchantment) {
            if (IsSpellTarget(pEnchantment, true)) {
                Dispel(apObject, false);
            }
            QueuedWornEnchantments->Remove(apObject);
            if (QueuedWornEnchantments->IsEmpty()) {
                if (QueuedWornEnchantments) {
                    delete QueuedWornEnchantments;
                }
                QueuedWornEnchantments = nullptr;
            }
        }
    }
}

void PlayerCharacter::HandlePhysicsGrab(float afTimeDelta) {
    Controls* pcontrols = appMain->GetControls();
    int32_t iUserAction = pcontrols->GetUserAction(Controls::UACTN_GRAB, 1);
    bool bbuttonPressed = iUserAction != 0;
    if (iUserAction) {
        Interface::SetDragDroppingState(0);
    }
    int32_t istate = Interface::GetDragDroppingState();
    if (istate == 1) {
        Interface::SetDragDroppingState(2);
    }
    if (bbuttonPressed || bIsGrabbing || spGrabSpring.m_pObject || istate == 1 || istate == 2) {
        if (spGrabSpring.m_pObject) {
            if (eGrabType == PlayerCharacter::GRAB_NORMAL) {
                if ((bIsGrabbing && !bbuttonPressed) || istate == 2) {
                    UpdateMouseSpring(afTimeDelta);
                }
                else {
                    DestroyMouseSpring();
                    bIsGrabbing = false;
                }
            }
        }
        else if ((bbuttonPressed || istate == 1) && eGrabType == PlayerCharacter::GRAB_NONE) {
            TESObjectREFR* pObject = Interface::GetActivateREFR();
            if (pObject) {
                if (static_cast<TESObjectACTI*>(pObject->GetObjectReference()) != pAshPile1) {
                    if (static_cast<TESObjectACTI*>(pObject->GetObjectReference()) != pAshPile2) {
                        NiNode* pav = pObject->Get3D();
                        bhkCollisionObject* pcol = pav ? FindFirstCollisionObject(pav) : nullptr;
                        bhkRigidBody* pbody = pcol ? pcol->GetRigidBody() : nullptr;
                        if (pbody && pbody->GethkWorld()) {
                            float fobjectMass = pbody->GetMass();
                            if (fobjectMass <= static_cast<double>(fGrabMaxWeightWalking.Float())) {
                                CreateMouseSpring(pObject, PlayerCharacter::GRAB_NORMAL, 1000.0f);
                                if (spGrabSpring.m_pObject) {
                                    pObject->AddChange(CHANGE_REFR_HAVOK_MOVE);
                                }
                                bIsGrabbing = true;
                                pObject->m_Extra.NotifyGuardsAboutActivation(pObject, this);
                                pObject->m_Extra.RemoveExtra(124);
                            }
                        }
                    }
                }
            }
        }
    }
}

bool PlayerCharacter::UpdateDropSpring(float afTimeDelta) {
    if (eGrabType == PlayerCharacter::GRAB_INVENTORY_DROP) {
        Controls* pcontrols = appMain->GetControls();
        if (!spGrabSpring.m_pObject) {
            double dScale = GetScale();
            float fDropDistance = fPlayerDropDistance.Float() * dScale;
            CreateMouseSpring(pGrabbedObject, PlayerCharacter::GRAB_INVENTORY_DROP, fDropDistance);
        }
        if (spGrabSpring.m_pObject) {
            if (!pcontrols->GetUserAction(Controls::UACTN_USE, 2) &&
                (pcontrols->GetUserAction(Controls::UACTN_USE, 1) ||
                 pcontrols->GetUserAction(Controls::UACTN_USE, 0))) {
                UpdateMouseSpring(afTimeDelta);
            }
            else {
                DestroyMouseSpring();
            }
        }
    }
    return eGrabType == PlayerCharacter::GRAB_INVENTORY_DROP;
}

// Sets bBeingChased when a guard is among the actors with a combat package on the player, then frees the list
// IsActorTargetingREFinPackage made.
void PlayerCharacter::CheckGetAwayFromGuards() {
    BSSimpleList<TESObjectREFR*>* preflist = ReferenceProcessLists
                                                 .IsActorTargetingREFinPackage(pPlayer, PACKAGE_TYPE_COMBAT, false);
    BSSimpleList<TESObjectREFR*>* plist = preflist;
    pPlayer->bBeingChased = false;
    while (plist) {
        Actor* pactor = nullptr;
        if (plist->m_item) {
            if (plist->m_item->IsActor()) {
                pactor = static_cast<Actor*>(plist->m_item);
            }
        }
        if (pactor && pactor->IsActoraGuard()) {
            pPlayer->bBeingChased = true;
            break;
        }
        plist = plist->m_pkNext;
    }
    if (preflist) {
        preflist->RemoveAllPointerList();
        if (preflist) {
            delete preflist;
        }
    }
}

void PlayerCharacter::PlayImpactBasedHitShader(TESObjectREFR* apRef) {
    if (apRef && apRef->Get3D()) {
        NiPoint3 kOffset;
        const NiPoint3* pPlayerPos = &GetLocationOnReference();
        const NiPoint3* pRefPos = &apRef->GetLocationOnReference();
        kOffset = *pRefPos - *pPlayerPos;
        float fDistance = kOffset.Length();
        float fAmount = GamePlayFormulas::CalcImpactShaderAmount(fDistance);
        if (fAmount > 0.0 && !pVATS.GetMode()) {
            NiNode* p3D = apRef->Get3D();
            TESImageSpaceModifier* pHitModifier = TESImageSpaceModifier::GetGetHit();
            ImageSpaceModifierInstanceForm::Trigger(pHitModifier, fAmount, p3D);
        }
    }
}

void PlayerCharacter::HavokActivateDroppedReference() {
    if (!DroppedRefList.IsEmpty()) {
        BSSimpleList<TESObjectREFR*>* plast = &DroppedRefList;
        BSSimpleList<TESObjectREFR*>* pcurrent = &DroppedRefList;
        while (pcurrent && !pcurrent->IsEmpty()) {
            TESObjectREFR* pref = pcurrent->m_item;
            if (pref && pref->Get3D()) {
                bhkWorld::SetMotion(pref->Get3D(), hkpMotion::MOTION_DYNAMIC, true, true, true);
                bhkWorld::Activate(pref->Get3D(), true, true, false);
                DroppedRefList.Remove(pref);
                pref->SetRecentPCDrop(false);
                pcurrent = plast->m_pkNext;
            }
            else {
                plast = pcurrent;
                pcurrent = pcurrent->m_pkNext;
            }
        }
    }
}

bool PlayerCharacter::IsConflictingObject(NiAVObject* apObj) {
    if (!apObj) {
        return false;
    }
    if (!pConflictingPick) {
        pConflictingPick = &kConflictingPick;
        kConflictingPick.SetFrontOnly(false);
        pConflictingPick->SetReturnNormal(true);
        pConflictingPick->SetObserveAppCullFlag(true);
    }
    NiPoint3 pos = GetLocationOnReference();
    NiPoint3 dir(0.0f, 24.0f, 0.0f);
    bool bresult = false;
    pos.z = pPlayer->GetHeight() * 0.25 + pos.z;
    pConflictingPick->SetTarget(apObj);
    if (pConflictingPick->PickObjects(pos + dir, dir, false)) {
        NiPick::Record* ppickRecord = pConflictingPick->m_pickResults.GetAt(0);
        NiPoint3 normal = ppickRecord->m_normal;
        if (dir.Dot(normal) > 0.0 &&
            !NiObject::IsExactKindOf(&NiBillboardNode::ms_RTTI, ppickRecord->GetAVObject()->m_pkParent)) {
            bresult = true;
        }
    }
    if (!bresult) {
        dir = NiPoint3(24.0f, -24.0f, 0.0f);
        if (pConflictingPick->PickObjects(pos + dir, dir, false)) {
            NiPick::Record* ppickRecord = pConflictingPick->m_pickResults.GetAt(0);
            NiPoint3 normal = ppickRecord->m_normal;
            if (dir.Dot(normal) > 0.0 &&
                !NiObject::IsExactKindOf(&NiBillboardNode::ms_RTTI, ppickRecord->GetAVObject()->m_pkParent)) {
                bresult = true;
            }
        }
        if (!bresult) {
            dir = NiPoint3(-24.0f, -24.0f, 0.0f);
            if (pConflictingPick->PickObjects(pos + dir, dir, false)) {
                NiPick::Record* ppickRecord = pConflictingPick->m_pickResults.GetAt(0);
                NiPoint3 normal = ppickRecord->m_normal;
                if (dir.Dot(normal) > 0.0 &&
                    !NiObject::IsExactKindOf(&NiBillboardNode::ms_RTTI, ppickRecord->GetAVObject()->m_pkParent)) {
                    bresult = true;
                }
            }
        }
    }
    pConflictingPick->SetTarget(nullptr);
    pConflictingPick->ClearResultsArray(0);
    if (bresult) {
        return true;
    }
    return GetCharController()->CheckInsideOfObject();
}

void PlayerCharacter::UpdateConflictingObjects() {
    if (!ConflictingObjects.IsEmpty()) {
        BSSimpleList<TESObjectREFR*>* plist = &ConflictingObjects;
        while (plist) {
            bool bremoveFromList = false;
            TESObjectREFR* pref = plist->m_item;
            NiAVObject* pobj = pref->Get3D();
            if (pobj) {
                if (!IsConflictingObject(pobj)) {
                    bremoveFromList = true;
                    pobj->SetAppCulled(false);
                    bhkWorld::EnableCollision(pobj, true, true, false);
                }
            }
            else {
                bremoveFromList = true;
            }
            if (bremoveFromList) {
                ConflictingObjects.Remove(pref);
                if (ConflictingObjects.IsEmpty()) {
                    return;
                }
                plist = &ConflictingObjects;
            }
            else {
                plist = plist->m_pkNext;
            }
        }
    }
}

void PlayerCharacter::UpdatePlayerActions() {
    PlayerActionObject* pAction;
    BSSimpleList<PlayerActionObject*>* plist = pListofActions;
    while (plist && plist->m_item) {
        pAction = plist->m_item;
        if (!(pAction->ftimer <= 0.0) ||
            pAction->eAction == PLAYER_ACTION_Z_KEY_OBJECT ||
            pAction->eAction == PLAYER_ACTION_STAND_ON_TABLE_CHAIR) {
            plist = plist->m_pkNext;
        }
        else {
            plist->Remove(pAction);
            ourFree(pAction);
            plist = pListofActions;
        }
    }
    for (plist = pListofActions; plist && plist->m_item; plist = plist->m_pkNext) {
        pAction = plist->m_item;
        double dDelta = appTimer.fDelta;
        pAction->ftimer = pAction->ftimer - dDelta;
    }
}

void PlayerCharacter::RemoveAllActionsForObject(TESObjectREFR* apActObject) {
    if (pListofActions && apActObject) {
        BSSimpleList<PlayerActionObject*>* plist = pListofActions;
        while (plist && plist->m_item) {
            PlayerActionObject* pAction = plist->m_item;
            if (pAction && apActObject == pAction->prefObj) {
                plist->Remove(pAction);
                ourFree(pAction);
                plist = pListofActions;
            }
            else {
                plist = plist->m_pkNext;
            }
        }
    }
}

BGSNote* PlayerCharacter::GetNotebyIndex(int32_t aiIndex) {
    BSSimpleList<BGSNote*>* pIter = &listNotes;
    int32_t iIndex = 0;
    while (pIter && pIter->m_item) {
        if (iIndex == aiIndex) {
            return pIter->m_item;
        }
        pIter = pIter->m_pkNext;
        ++iIndex;
    }
    return nullptr;
}

void PlayerCharacter::UpdateMenuModeButton(float afTimeDelta) {
    Controls* pcontrols = appMain->GetControls();
    if (bPipboyLightDebounce ||
        !pcontrols->GetUserAction(Controls::UACTN_MENUMODE, 2) ||
        !(fPlayerPipBoyLightTimer.Float() > static_cast<double>(fMenuModeButtonTimer))) {
        if (pcontrols->GetUserAction(Controls::UACTN_MENUMODE, 1) ||
            pcontrols->GetUserAction(Controls::UACTN_MENUMODE, 0)) {
            fMenuModeButtonTimer = fMenuModeButtonTimer + afTimeDelta;
            if (!bPipboyLightDebounce &&
                Interface::GetPipboy()->Get3rdPersonPipboyRoot() &&
                fPlayerPipBoyLightTimer.Float() < static_cast<double>(fMenuModeButtonTimer)) {
                SpellItem* pLightSpell = MagicSystem::GetPipBoyLightSpell();
                MagicItem* pLightItem = nullptr;
                if (pLightSpell) {
                    pLightItem = &static_cast<MagicItem&>(static_cast<MagicItemForm&>(*pLightSpell));
                }
                if (IsSpellTarget(pLightItem, true)) {
                    Interface::GetPipboy()->ShowPipboyLightEffect(true, false, true);
                    Interface::GetPipboy()->ShowPipboyLightEffect(false, false, true);
                    BSAudio::QInstance().GetSoundHandleByName("UIPipBoyLightOff", 289u).Play(false);
                    SpellItem* pDispelSpell = MagicSystem::GetPipBoyLightSpell();
                    MagicItem* pDispelItem = nullptr;
                    if (pDispelSpell) {
                        pDispelItem = &static_cast<MagicItem&>(static_cast<MagicItemForm&>(*pDispelSpell));
                    }
                    Dispel(pDispelItem, static_cast<MagicCaster*>(nullptr), static_cast<ActiveEffect*>(nullptr));
                }
                else if (!pPlayer->GetControlsDisabled(PlayerCharacter::DPC_PIPBOY)) {
                    Interface::GetPipboy()->ShowPipboyLightEffect(true, true, true);
                    Interface::GetPipboy()->ShowPipboyLightEffect(false, true, true);
                    BSAudio::QInstance().GetSoundHandleByName("UIPipBoyLightOn", 289u).Play(false);
                    CastAbility(MagicSystem::GetPipBoyLightSpell(), false);
                }
                bPipboyLightDebounce = 1;
            }
        }
        else {
            bMenuModeButtonClicked = false;
            fMenuModeButtonTimer = 0.0f;
            bPipboyLightDebounce = 0;
        }
    }
    else {
        bMenuModeButtonClicked = true;
    }
    if (Interface::GetPipboy()->QPipboyLightOn() && !Interface::GetPipboy()->Get3rdPersonPipboyRoot()) {
        Interface::GetPipboy()->ShowPipboyLightEffect(true, false, true);
        Interface::GetPipboy()->ShowPipboyLightEffect(false, false, true);
        SpellItem* pLightSpell = MagicSystem::GetPipBoyLightSpell();
        if (pLightSpell) {
            Dispel(pLightSpell, static_cast<MagicCaster*>(nullptr), static_cast<ActiveEffect*>(nullptr));
        }
        else {
            Dispel(
                static_cast<MagicItem*>(nullptr),
                static_cast<MagicCaster*>(nullptr),
                static_cast<ActiveEffect*>(nullptr)
            );
        }
    }
}

bool PlayerCharacter::CanEquipOrUseObject(ItemChange* apItem) {
    float fhealth;
    bool bcanEquip = true;
    TESBoundObject* pobj = apItem->pContainerObj;
    ExtraDataList* pextra = apItem->pExtraObjectList && apItem->pExtraObjectList->m_item
        ? apItem->pExtraObjectList->m_item
        : nullptr;
    if (pobj->GetQuestObject() &&
        ((pobj->GetFormType() != WEAP_ID && pobj->GetFormType() != ARMO_ID) ||
         !static_cast<TESObjectWEAP*>(pobj)->TESObjectWEAP::GetIsPlayable())) {
        bcanEquip = false;
    }
    if (pextra && pextra->GetCanNotWear()) {
        bcanEquip = false;
    }

    ExtraHealth* pHealth = nullptr;
    if (pextra) {
        pHealth = static_cast<ExtraHealth*>(pextra->GetExtraData(EXTRA_HEALTH));
    }
    if (pHealth) {
        fhealth = pHealth->fHealth;
    }
    else {
        fhealth = static_cast<float>(TESHealthForm::GetFormHealth(pobj));
    }
    switch (apItem->pContainerObj->GetFormType()) {
        case ARMO_ID:
        case WEAP_ID: {
            if (fhealth <= 0.0) {
                bcanEquip = false;
            }
            break;
        }
        case BOOK_ID:
        case INGR_ID:
        case LIGH_ID:
        case ALCH_ID: {
            return bcanEquip;
        }
        case CLOT_ID: {
            if (!CanCastWornEnchantment(pobj, pextra)) {
                bcanEquip = false;
            }
            break;
        }
        case AMMO_ID: {
            bcanEquip = true;
            TESAmmo* pObj = static_cast<TESAmmo*>(apItem->pContainerObj);
            TESObjectWEAP* pCurrWeapon = GetCurrentWeapon();
            if (pCurrWeapon && pObj) {
                TESAmmo* pAmmo = pCurrWeapon->GetAmmoForm();
                BGSListForm* pAmmoList = pCurrWeapon->GetAmmoFormList();
                if (pAmmo != pObj && (!pAmmoList || pAmmoList->GetItemIndex(pObj) == -1)) {
                    bcanEquip = false;
                }
            }
            else {
                bcanEquip = false;
            }
            break;
        }
        default: {
            bcanEquip = false;
            break;
        }
    }
    return bcanEquip;
}

uint32_t PlayerCharacter::AttemptYield() {
    uint32_t iyieldingActors = 0;
    BSSimpleList<Crime*>* pcrimeList = ReferenceProcessLists.RemoveCrimesForYieldCheck(fCombatYieldTime.Float());
    if (!pcrimeList) {
        return 0;
    }
    CombatGroup* pplayerGroup = pCombatGroup;
    CombatGroup* psplitGroup = nullptr;
    uint32_t iplayerGroupMemberCount = pplayerGroup ? pplayerGroup->GetMemberCount() : 0;
    uint32_t inonTeamMateCount = 0;
    for (uint32_t i = 0; i < iplayerGroupMemberCount; ++i) {
        Actor* pmember = pplayerGroup->GetMember(i);
        if (pmember != pPlayer) {
            if (!pmember->bPlayerTeammate) {
                ++inonTeamMateCount;
            }
        }
    }
    if (inonTeamMateCount) {
        psplitGroup = pCombatManager->CreateCombatGroup();
        psplitGroup->CopyTargets(pplayerGroup);
    }
    uint32_t icombatGroupCount = pCombatManager->GetCombatGroupCount();
    for (uint32_t i = 0; i < icombatGroupCount; ++i) {
        CombatGroup* phostileGroup = pCombatManager->GetCombatGroupByIndex(i);
        if (phostileGroup &&
            phostileGroup != pplayerGroup &&
            phostileGroup != psplitGroup &&
            phostileGroup->IsTarget(this)) {
            bool bacceptYield = true;
            bool bacceptNonTeammateYield = true;
            uint32_t imemberCount = phostileGroup->GetMemberCount();
            for (uint32_t j = 0; j < imemberCount; ++j) {
                Actor* pmember = phostileGroup->GetMember(j);
                FIGHT_REACTION efightReaction = FIGHT_REACTION_NEUTRAL;
                if (pmember->GetShouldAttackActor(this, false, efightReaction, true)) {
                    bacceptYield = false;
                    break;
                }
                if (bacceptNonTeammateYield && inonTeamMateCount) {
                    for (uint32_t k = 0; k < iplayerGroupMemberCount; ++k) {
                        Actor* pplayerMember = pplayerGroup->GetMember(k);
                        if (pplayerMember != pPlayer &&
                            !pplayerMember->bPlayerTeammate &&
                            (pmember->GetShouldAttackActor(pplayerMember, false, efightReaction, true) ||
                             pplayerMember->GetShouldAttackActor(pmember, false, efightReaction, true))) {
                            bacceptNonTeammateYield = false;
                            break;
                        }
                    }
                }
            }
            if (bacceptYield) {
                Actor* pclosestMember = nullptr;
                float fclosestMemberDistanceSquared = std::numeric_limits<float>::max();
                NiPoint3 myLoc = GetLocationOnReference();
                if (pplayerGroup) {
                    for (uint32_t j = 0; j < iplayerGroupMemberCount; ++j) {
                        Actor* pplayerMember = pplayerGroup->GetMember(j);
                        if (bacceptNonTeammateYield || pplayerMember->bPlayerTeammate || pplayerMember == pPlayer) {
                            phostileGroup->RemoveTarget(pplayerMember);
                        }
                        else if (!bacceptNonTeammateYield && psplitGroup) {
                            psplitGroup->AddMember(pplayerMember);
                        }
                    }
                }
                else {
                    phostileGroup->RemoveTarget(this);
                }
                for (uint32_t j = 0; j < phostileGroup->GetTargetCount(); ++j) {
                    Actor* ptarget = phostileGroup->GetTarget(j);
                    if (ptarget && ptarget->bPlayerTeammate && !ptarget->IsAngryWithPlayer()) {
                        phostileGroup->RemoveTarget(ptarget);
                        --j;
                        ptarget->StopCombat(nullptr);
                    }
                }
                for (uint32_t j = 0; j < imemberCount; ++j) {
                    Actor* pmember = phostileGroup->GetMember(j);
                    ++iyieldingActors;
                    if (pplayerGroup) {
                        pplayerGroup->RemoveTarget(pmember);
                    }
                    if (bacceptNonTeammateYield && psplitGroup) {
                        psplitGroup->RemoveTarget(pmember);
                    }

                    NiPoint3 memberVec = myLoc - pmember->GetLocationOnReference();
                    float fdistanceSquared = memberVec.SqrLength();
                    if (fclosestMemberDistanceSquared > static_cast<double>(fdistanceSquared)) {
                        pclosestMember = pmember;
                        fclosestMemberDistanceSquared = fdistanceSquared;
                    }
                    BSSimpleList<Crime*>* plist = pcrimeList;
                    BSSimpleList<Crime*>* plastList = nullptr;
                    while (plist && !plist->IsEmpty()) {
                        Crime* pcrime = plist->m_item;
                        bool bremove = false;
                        if (pcrime) {
                            if (pcrime->pcrimetarget == pmember) {
                                delete pcrime;
                                bremove = true;
                            }
                        }
                        else {
                            bremove = true;
                        }
                        if (bremove) {
                            if (plastList) {
                                plastList->Remove(plist->m_item);
                                plist = plastList->m_pkNext;
                            }
                            else {
                                plist->RemoveHead();
                            }
                        }
                        else {
                            plastList = plist;
                            plist = plist->m_pkNext;
                        }
                    }
                }
                if (pclosestMember) {
                    pCombatDialogueManager->StartDialogue(pclosestMember, this, DT_COMBAT, 11, true, nullptr);
                }
            }
        }
    }
    if (pplayerGroup && iyieldingActors) {
        for (uint32_t j = 0; j < pplayerGroup->GetMemberCount(); ++j) {
            Actor* pplayerMember = pplayerGroup->GetMember(j);
            if (pplayerMember && pplayerMember->GetCombatController()) {
                pplayerMember->GetCombatController()->ForceStopCombat();
            }
        }
        while (pplayerGroup->GetTargetCount()) {
            pplayerGroup->RemoveTarget(pplayerGroup->GetTarget(0));
        }
    }

    for (Actor* pmember : *ReferenceProcessLists.GetAliveActorList()) {
        if (pmember) {
            TESPackage* pPackage = pmember->GetCurrentPackage();
            if (pPackage) {
                PACKAGE_TYPE epackType = pPackage->GetPackType();
                switch (epackType) {
                    case PACKAGE_TYPE_SPECTATOR: {
                        static_cast<SpectatorPackage*>(pPackage)->SpectatorPackage::RemoveActorAsThreatSourceOrTarget(
                            *pPlayer
                        );
                        pmember->EvaluatePackage(false, false);
                        break;
                    }
                    case PACKAGE_TYPE_FLEE: {
                        static_cast<FleePackage*>(pPackage)->FleePackage::RemoveAvoidedRef(pPlayer);
                        pmember->EvaluatePackage(false, false);
                        break;
                    }
                    case PACKAGE_TYPE_FLEE_NON_COMBAT: {
                        pmember->EvaluatePackage(false, false);
                        break;
                    }
                }
            }
        }
    }
    bPlayerInCombat = false;
    for (Crime* pcrime : *pcrimeList) {
        if (pcrime) {
            Actor* pactor = static_cast<Actor*>(pcrime->pcrimetarget);
            if (pactor) {
                pactor->AddBackCrime(pcrime);
            }
        }
    }
    if (psplitGroup) {
        uint32_t isplitGroupMembers = psplitGroup->GetMemberCount();
        for (uint32_t i = 0; i < isplitGroupMembers; ++i) {
            Actor* psplitMember = psplitGroup->GetMember(i);
            psplitMember->SetCombatGroup(psplitGroup);
            pplayerGroup->RemoveMember(psplitMember);
        }
    }
    return iyieldingActors;
}

void PlayerCharacter::AddRockItAmmo(TESBoundObject* apObj, ExtraDataList* apExtra, int32_t aiNumAdded) {
    bool bfoundMatch = false;
    for (int32_t iitemCt = 0; !bfoundMatch && iitemCt < RockItLauncherAmmoList.iSize; ++iitemCt) {
        ItemChange* pitem = RockItLauncherAmmoList.GetAt(iitemCt);
        if (RockItLauncherAmmoList.GetAt(iitemCt)->pContainerObj == apObj) {
            bool bisDefaultStack = !pitem->pExtraObjectList ||
                !pitem->pExtraObjectList->m_item ||
                pitem->pExtraObjectList->m_item->IsExtraDefaultforContainer(false);
            if (apExtra) {
                for (BSSimpleList<ExtraDataList*>* pextraObj = pitem->pExtraObjectList;
                     !bfoundMatch && pextraObj && pextraObj->m_item;
                     pextraObj = pextraObj->m_pkNext) {
                    if (!pextraObj->m_item->CompareListForContainer(apExtra, true, false) &&
                        !apExtra->CompareListForContainer(pextraObj->m_item, true, false)) {
                        uint32_t uiNumber = RockItLauncherAmmoList.GetAt(iitemCt)->iNumber;
                        pitem->iNumber = aiNumAdded + uiNumber;
                        int16_t sCount = pextraObj->m_item->GetCount();
                        pextraObj->m_item->SetCount(aiNumAdded + sCount);
                        bfoundMatch = true;
                    }
                }
                if (!bfoundMatch && apExtra->IsExtraDefaultforContainer(false) && bisDefaultStack) {
                    ExtraDataList* paddedExtra = new ExtraDataList;
                    paddedExtra->DuplicateExtraListForContainer(apExtra);
                    paddedExtra->SetCount(aiNumAdded);
                    if (!pitem->pExtraObjectList) {
                        pitem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                    }
                    pitem->pExtraObjectList->AddHead(paddedExtra);
                    pitem->iNumber = aiNumAdded + RockItLauncherAmmoList.GetAt(iitemCt)->iNumber;
                    bfoundMatch = true;
                }
            }
            else if (bisDefaultStack) {
                pitem->iNumber = aiNumAdded + RockItLauncherAmmoList.GetAt(iitemCt)->iNumber;
                bfoundMatch = true;
            }
        }
    }
    if (!bfoundMatch) {
        ItemChange* pnewItem = new ItemChange(apObj, aiNumAdded);
        if (apExtra) {
            ExtraDataList* paddedExtra = new ExtraDataList;
            paddedExtra->DuplicateExtraListForContainer(apExtra);
            paddedExtra->SetCount(aiNumAdded);
            pnewItem->pExtraObjectList->AddHead(paddedExtra);
        }
        RockItLauncherAmmoList.Add(pnewItem);
    }
    bool bHardcore = this->IsHardcore();
    fRockItLauncherAmmoWeight = TESWeightForm::GetFormWeight(apObj, bHardcore) * static_cast<double>(aiNumAdded) +
        fRockItLauncherAmmoWeight;
}

ItemChange* PlayerCharacter::RemoveRockItAmmo(
    TESBoundObject* apObj,
    ExtraDataList* apExtra,
    int32_t aiNumRemoved,
    bool abReturnRemovedItem
) {
    bool bfoundMatch = false;
    ItemChange* preturnedItem = nullptr;
    int32_t iamountToRemove = aiNumRemoved;
    for (int32_t iitemCt = 0; !bfoundMatch && iitemCt < RockItLauncherAmmoList.iSize; ++iitemCt) {
        ItemChange* pitem = RockItLauncherAmmoList.GetAt(iitemCt);
        if (pitem->pContainerObj == apObj) {
            if (apExtra) {
                for (BSSimpleList<ExtraDataList*>* pextraObj = pitem->pExtraObjectList;
                     !bfoundMatch && pextraObj && pextraObj->m_item;
                     pextraObj = pextraObj->m_pkNext) {
                    if (pextraObj->m_item == apExtra) {
                        int16_t sCount = apExtra->GetCount();
                        bool bremoveExtra = sCount <= aiNumRemoved;
                        if (sCount <= aiNumRemoved) {
                            iamountToRemove = apExtra->GetCount();
                        }
                        if (abReturnRemovedItem) {
                            ExtraDataList* preturnedExtra = apExtra;
                            preturnedItem = new ItemChange(apObj, iamountToRemove);
                            if (!bremoveExtra) {
                                preturnedExtra = new ExtraDataList;
                                preturnedExtra->DuplicateExtraListForContainer(apExtra);
                                preturnedExtra->SetCount(iamountToRemove);
                            }
                            preturnedItem->pExtraObjectList->AddHead(preturnedExtra);
                        }
                        if (bremoveExtra) {
                            if (pitem->pExtraObjectList) {
                                pitem->pExtraObjectList->Remove(apExtra);
                            }
                            if (!abReturnRemovedItem) {
                                if (apExtra) {
                                    delete apExtra;
                                }
                                apExtra = nullptr;
                            }
                        }
                        else {
                            apExtra->SetCount(apExtra->GetCount() - aiNumRemoved);
                        }
                        int32_t iItemCount = pitem->iNumber;
                        if (iamountToRemove < iItemCount) {
                            uint32_t uiCurrent = pitem->iNumber;
                            pitem->iNumber = uiCurrent - iamountToRemove;
                        }
                        else {
                            RockItLauncherAmmoList.Remove(iitemCt, true);
                        }
                        bfoundMatch = true;
                    }
                }
            }
            else if (!pitem->pExtraObjectList || !pitem->pExtraObjectList->m_item) {
                if (abReturnRemovedItem) {
                    preturnedItem = new ItemChange(apObj, iamountToRemove);
                }
                int32_t iItemCount = pitem->iNumber;
                if (iamountToRemove < iItemCount) {
                    uint32_t uiCurrent = pitem->iNumber;
                    pitem->iNumber = uiCurrent - iamountToRemove;
                }
                else {
                    RockItLauncherAmmoList.Remove(iitemCt, true);
                }
                bfoundMatch = true;
            }
        }
    }
    bool bHardcore = this->IsHardcore();
    double dFormWeight = TESWeightForm::GetFormWeight(apObj, bHardcore);
    fRockItLauncherAmmoWeight = fRockItLauncherAmmoWeight - dFormWeight * static_cast<double>(iamountToRemove);
    return preturnedItem;
}

void PlayerCharacter::resetAntiCheatTimer() {
    uiAntiCheatTimer = 0;
}

void PlayerCharacter::UpdateHardcoreMode() {
    if (!pPlayer->GetControlsDisabled(PlayerCharacter::DPC_PIPBOY)) {
        float fTimeScale = appCalendar.GetTimeScale();
        float fH20Interval = fHCDehydrationRate.Float() * fTimeScale / 60.0;
        float fSLPInterval = fHCSleepDeprivationRate.Float() * fTimeScale / 60.0;
        float fFODInterval = fHCStarvationRate.Float() * fTimeScale / 60.0;
        fHardcoreTimerSLP = appCalendar.GetMinutesPassed();
        fHardcoreTimerFOD = fHardcoreTimerSLP;
        fHardcoreTimerH20 = fHardcoreTimerSLP;
        if (fPrevHardcoreTickH20 == 0.0 || NeedResetHardcoreTimer()) {
            fPrevHardcoreTickH20 = fHardcoreTimerH20;
            fPrevHardcoreTickFOD = fHardcoreTimerFOD;
            fPrevHardcoreTickSLP = fHardcoreTimerSLP;
            SetNeedResetHardcoreTimer(false);
        }
        float fH20Elapsed = fHardcoreTimerH20 - fPrevHardcoreTickH20;
        float fFODElapsed = fHardcoreTimerFOD - fPrevHardcoreTickFOD;
        float fSLPElapsed = fHardcoreTimerSLP - fPrevHardcoreTickSLP;
        if (fH20Interval <= static_cast<double>(fH20Elapsed)) {
            DamageModActorValue(
                ActorValue::DEHYDRATION,
                static_cast<int32_t>(fH20Elapsed / fH20Interval),
                static_cast<Actor*>(nullptr)
            );
            fHardcoreTimerH20 = appCalendar.GetMinutesPassed();
            fPrevHardcoreTickH20 = fHardcoreTimerH20;
        }
        if (fSLPInterval <= static_cast<double>(fSLPElapsed)) {
            if (!pPlayer->IsSleeping()) {
                DamageModActorValue(
                    ActorValue::SLEEPDEPREVATION,
                    static_cast<int32_t>(fSLPElapsed / fSLPInterval),
                    static_cast<Actor*>(nullptr)
                );
            }
            fHardcoreTimerSLP = appCalendar.GetMinutesPassed();
            fPrevHardcoreTickSLP = fHardcoreTimerSLP;
        }
        if (fFODInterval <= static_cast<double>(fFODElapsed)) {
            DamageModActorValue(
                ActorValue::HUNGER,
                static_cast<int32_t>(fFODElapsed / fFODInterval),
                static_cast<Actor*>(nullptr)
            );
            fHardcoreTimerFOD = appCalendar.GetMinutesPassed();
            fPrevHardcoreTickFOD = fHardcoreTimerFOD;
        }
    }
}

NiPoint3 PlayerCharacter::GetBulletAutoAim() {
    return BulletAutoAim;
}

double PlayerCharacter::GetTimeGrenadeHeld() {
    return fProjectileReleaseTimer;
}

PlayerActionObject* PlayerCharacter::GetPlayerAction(PLAYER_ACTION aeAction) {
    PlayerActionObject* pFound = nullptr;
    if (!pListofActions) {
        return nullptr;
    }
    for (BSSimpleList<PlayerActionObject*>* pIter = pListofActions; pIter && pIter->m_item && !pFound;
         pIter = pIter->m_pkNext) {
        PlayerActionObject* pAction = pIter->m_item;
        if (pAction && pAction->eAction == aeAction) {
            pFound = pAction;
        }
    }
    return pFound;
}

void PlayerCharacter::GetInventoryEncumbrance(float& afCurrentEncumbrance, float& afMaxEncumbrance) {
    afMaxEncumbrance = GetMaxCarryWeight();
    afCurrentEncumbrance = static_cast<float>(GetActorValue(ActorValue::INVENTORY_WEIGHT));
}

void PlayerCharacter::DebugResetAllTerminals() {
    uint32_t iid;
    TESForm* pform;
    NiTMapItem<uint32_t, TESForm*>* iter;
    iter = TESForm::pAllForms->GetFirstPos();
    while (iter) {
        pform = nullptr;
        iid = 0;
        TESForm::pAllForms->GetNext(iter, iid, pform);
        if (pform && pform->GetFormType() == REFR_ID) {
            TESObjectREFR* pref = static_cast<TESObjectREFR*>(pform);
            if (pref->GetObjectReference()->GetFormType() == TERM_ID) {
                BGSTerminal::ResetState(pref);
            }
        }
    }
}

void PlayerCharacter::DeathMenu() {
    bShowDeathMessage = false;
    int8_t cResult = Interface::GetMessageMenuResult();
    if (cResult == 1) {
        pSaveLoadManager->LoadMostRecentSaveGame();
    }
    else if (cResult == 2) {
        StartMenu::ChooseMainMenu();
    }
}

bool PlayerCharacter::IsGodMode() {
    return bSetDemigodMode.Bool() || PlayerCharacter::bGodMode;
}

bool PlayerCharacter::IsDemigodMode() {
    if (bSetDemigodMode.Bool()) {
        return true;
    }

    return PlayerCharacter::bDemigodMode;
}

void PlayerCharacter::SpeakDoneCallback(void* apUnused, bool abSucceeded) {
    pPlayer->SetSpeaking(false);
}

void PlayerCharacter::ForceReload() {
    Interface::ForceReload();
}

void PlayerCharacter::SetGodMode(bool abGodMode) {
    PlayerCharacter::bGodMode = abGodMode;
}

void PlayerCharacter::SetDemigodMode(bool abDemigodMode) {
    PlayerCharacter::bDemigodMode = abDemigodMode;
}

void PlayerCharacter::ForceRestart() {
    appMain->ResetGame();
}

int32_t PlayerCharacter::GetSumOfSPECIALStats() {
    int32_t iSum = pPlayer->GetPermanentActorValue(ActorValue::STRENGTH);
    iSum = pPlayer->GetPermanentActorValue(ActorValue::PERCEPTION) + iSum;
    iSum = pPlayer->GetPermanentActorValue(ActorValue::ENDURANCE) + iSum;
    iSum = pPlayer->GetPermanentActorValue(ActorValue::CHARISMA) + iSum;
    iSum = pPlayer->GetPermanentActorValue(ActorValue::INTELLIGENCE) + iSum;
    iSum = pPlayer->GetPermanentActorValue(ActorValue::AGILITY) + iSum;
    return iSum + pPlayer->GetPermanentActorValue(ActorValue::LUCK);
}

void PlayerCharacter::PoisonWeapon() {
    if (pPlayer->pPendingPoison) {
        if (Interface::GetMessageMenuResult() == 2) {
            ItemChange* pweaponItem = pPlayer->pCurrentProcess->GetCurrentWeapon();
            char* pWeapon = nullptr;
            if (pweaponItem) {
                pWeapon = reinterpret_cast<char*>(dynamic_cast<TESObjectWEAP*>(pweaponItem->pContainerObj));
            }
            if (pWeapon) {
                TESChallengeManager::UpdateChallenge(
                    CHALLENGE_TYPE_USE_ITEM,
                    1,
                    pPlayer->pPendingPoison,
                    nullptr,
                    0,
                    0,
                    0
                );
                TESChallengeManager::UpdateChallenge(
                    CHALLENGE_TYPE_USE_ITEM_LIST,
                    1,
                    pPlayer->pPendingPoison,
                    nullptr,
                    0,
                    0,
                    0
                );
                pweaponItem->SetPoison(pPlayer->pPendingPoison);
                pPlayer->RemoveItem(
                    pPlayer->pPendingPoison,
                    nullptr,
                    1,
                    false,
                    false,
                    nullptr,
                    nullptr,
                    nullptr,
                    true,
                    false
                );
                Interface::UpdateInventory();
            }
        }
        pPlayer->pPendingPoison = nullptr;
    }
}

uint8_t BGSEntryPointPerkEntry::GetEntryPoint() {
    return Data.cEntryPoint;
}

void ImageSpaceModifierInstanceDOF::SetDistance(float afDistance) {
    fDistance = afDistance;
}

void ImageSpaceModifierInstanceDOF::SetMaskConst(float afMaskConst) {
    fMaskConst = afMaskConst;
}

void ImageSpaceModifierInstanceDRB::SetDistance(float afDistance) {
    fDistance = afDistance;
}

void TESRegion::SetCurrentWeather(TESWeather* apWeather) {
    pCurrentWeather = apWeather;
}

void TESForm::SetRecentPCDrop(bool abVal) {
    if (abVal) {
        this->iFormFlags |= kFormFlags_RecentPCDrop;
    }
    else {
        this->iFormFlags &= ~kFormFlags_RecentPCDrop;
    }
}

Actor* CombatGroup::GetTarget(uint32_t aiIndex) {
    return TargetArray[aiIndex].pActor;
}

uint32_t CombatManager::GetCombatGroupCount() {
    return CombatGroups.GetSize();
}

float Projectile::GetSpeed() {
    return ComputeSpeed(GetProjectileBase()->GetSpeed(), fPower, fSpeedMult);
}

void Projectile::SetSpeedMult(float afSpeedMult) {
    fSpeedMult = afSpeedMult;
}

NiFixedString& FixedStrings::QArrowBone() {
    return *FixedStrings::pArrowBone;
}

NiFixedString& FixedStrings::QHeadMeshExp() {
    return *FixedStrings::pHeadMeshExp;
}

bool FOPipboyManager::QPipboyLightOn() {
    return b1stPersonLightOn || b3rdPersonLightOn;
}

bool InterfaceManager::GetContainerMenuQueued() {
    return pInstance && pInstance->bContainerOpenQueued;
}

bool HUDMainMenu::IsXPMeterFinishedMoving() {
    if (pme) {
        return pme->bXPMeterFinishedMoving;
    }
    return false;
}

bool Main::IsInMenuMode() {
    return bInMenuMode || bMenuBGReady;
}
