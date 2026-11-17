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
#include "BSCore/BSTimer.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/MemoryPool.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSDevices/Controls.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkBlendCollisionObjectAddRotation.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkForceController.h"
#include "BSHavok/bhkMouseSpringAction.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRagdollPenetrationUtil.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkRigidBodyCinfo.h"
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
#include "BSMain/SettingCollection.h"
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
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExteriorCellLoader.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FindObjectIterator.h"
#include "Fallout Shared/FOScriptFunctions.h"
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
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionDataImposter.h"
#include "Fallout Shared/Region System/TESRegionDataList.h"
#include "Fallout Shared/Region System/TESRegionDataMap.h"
#include "Fallout Shared/Region System/TESRegionDataSound.h"
#include "Fallout Shared/Region System/TESRegionDataWeather.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Region System/TESRegionPointList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/SCRIPT_PARAMETER.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
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
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/MenuManager.h"
#include "Fallout/Interface/Menus/BlackJackMenu.h"
#include "Fallout/Interface/Menus/CharGenMenu.h"
#include "Fallout/Interface/Menus/HackingMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
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
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cmath>
#include <stdint.h>
#include <xmmintrin.h>

// The names of the causes of death (CAUSE_OF_DEATH).
using CauseOfDeathNameArray = const char* [7];
static CauseOfDeathNameArray CauseOfDeathNames{"Explosion", "Gun", "BluntWeapon", "HandToHand", "ObjectImpact",
	"Poison", "Radiation"};

// The third-person wobble scale AddGunWobble eases toward its target (initialised to 1.0f).
static float fWobbleChaseDrift = 1.0f;
static int32_t idistance = 300;

// UpdateArmRotation's function-local static: how far the arms follow the camera, eased toward the target.
static float fchasePercent = 1.0f;

static bool bPlaceOnHorse;
static bool abflagpick;

static TESObjectREFR* pPCCurrentHorseMarker;
static float fMouseSpringObjectWeight;

static TESObjectREFR* pPickupref;
static int32_t count;

// The node the first-person skeleton looks through; null while the first-person 3D lacks it.
static NiAVObject* p1stPersonLooking;
static float flastCameraZ;
static uint32_t center_guard;
static TESObjectREFR* pprevVatsTarget;

// CheckUserInputAttacks's function-local statics; ivatsAttackDelay counts the frames left before VATS forces the
// next shot.
static int32_t ilargestAttackForce;
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
SettingT<INIPrefSettingCollection> gs_iKillCamera_GamePlay("iKillCamera:GamePlay", 2);
SettingT<INIPrefSettingCollection> gs_iDifficulty_GamePlay("iDifficulty:GamePlay", 2);
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
SettingT<INIPrefSettingCollection> gs_bInvertYValues_Controls("bInvertYValues:Controls", false);
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
SettingT<INISettingCollection> gs_bBorderRegionsEnabled_General("bBorderRegionsEnabled:General", true);
SettingT<INISettingCollection> fForwardBias("fForwardBias:Controls", 1.7f);
SettingT<INISettingCollection> fXenonMinLookSpeed("fXenonMinLookSpeed:Controls", 0.05f);
SettingT<INIPrefSettingCollection> gs_fXenonHorizLookSpeed_Controls("fXenonHorizLookSpeed:Controls", 1500.0f);
SettingT<INIPrefSettingCollection> gs_fXenonVertLookSpeed_Controls("fXenonVertLookSpeed:Controls", 1200.0f);
SettingT<INISettingCollection> iXenonHorizLookAccel("iXenonHorizLookAccel:Controls", 2);
SettingT<INISettingCollection> iXenonVertLookAccel("iXenonVertLookAccel:Controls", 2);
SettingT<INISettingCollection> bAlwaysRunByDefault("bAlwaysRunByDefault:Controls", false);
SettingT<INISettingCollection> fPlayerCharacterTurnMult("fPlayerCharacterTurnMult:Controls", 0.02f);
NiPoint3 PlayerCharacter::kVanityModePos(NiPoint3_ZERO);
NiPoint3 PlayerCharacter::kAutoVanityModePos(NiPoint3_ZERO);
SettingT<INISettingCollection> bDisableAutoVanityMode("bDisableAutoVanityMode:General", false);
NiPoint3 chaseCameraPos(NiPoint3_ZERO);
NiMatrix3 GunWobbleRot = NiMatrix3_IDENTITY;
BSSimpleList<TESObjectREFR*> ConflictingObjects;
NiPick kConflictingPick(0, 8);

void MarkPlayerBones(NiAVObject* apObject) {
	uint32_t i;
	NiAVObject* v2;
	uint32_t a1;
	NiNode* v4;
	if (apObject) {
		apObject->SetPlayerBone(1);
		if (apObject->IsNode()) {
			v4 = apObject->IsNode();
			a1 = 0;
			for (i = (uint32_t)v4->GetArrayCount(); a1 < i; i = (uint32_t)v4->GetArrayCount()) {
				v2 = v4->GetAt(a1);
				if (v2) {
					MarkPlayerBones(v2);
				}
				++a1;
			}
		}
	}
}

bool Cmd_DisableHardcoreTracking(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset) {
	if (pPlayer) {
		pPlayer->ClearsHardcoreFlag();
	}
	return 1;
}

bool au_re_Cmd_IsAlwaysHardcore(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset) {
	return Script::IsAlwaysHardcoreConditionalFunction(apRefObject, 0, 0, afReturnValue);
}

void SettingsMenu_ToggleHardcore(StartMenu::UserOption* a1) {
	if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
		pPlayer->SetHardcoreCallback(a1->iVal);
	} else {
		a1->iVal = (int32_t)((uint8_t)pPlayer->IsHardcore());
		a1->iDefaultVal = 0;
	}
}

bool Cmd_GetOwnerLastTarget_Execute(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset) {
	uint32_t apID;
	HitData* lastTargetInfo;
	Actor* v10;
	afReturnValue = 0.0;
	if (apContainer) {
		if (apContainer->IsActor()) {
			v10 = static_cast<Actor*>(apContainer);
			lastTargetInfo = v10->GetLastAttackHitData();
			if (lastTargetInfo) {
				if (lastTargetInfo->pTarget) {
					apID = lastTargetInfo->pTarget->TESForm::GetFormID();
					Script::PutNumericIDInDouble(apID, afReturnValue);
				}
			}
		}
	}
	return 1;
}

// Opens the recipe menu for the calling reference's vendor: the actor itself, or the speaker of a talking activator
// (the reference's own base object, or the one its mobile object talks through).
bool Cmd_ShowRecipeMenu(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset) {
	TESForm* pCategory = 0;
	Script::ParseParameters(apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript,
		apScriptLocals, &pCategory);
	if (apRefObject) {
		MobileObject* pMobile = 0;
		TESObjectREFR* pVendor = 0;
		if (apRefObject->IsMobileObject()) {
			pMobile = static_cast<MobileObject*>(apRefObject);
		} else {
			BGSTalkingActivator* pActivator = dynamic_cast<BGSTalkingActivator*>(apRefObject->GetObjectReference());
			pVendor = pActivator ? pActivator->GetTempRef() : 0;
		}
		if (pMobile) {
			if (pMobile->IsActor()) {
				pVendor = pMobile;
			} else if (pMobile->IsTalkingActivator()) {
				// Takes the talking activator's base object as a BGSTalkingActivator without a check, as the game does.
				pVendor = static_cast<BGSTalkingActivator*>(pMobile->GetTalkingActivator()->GetObjectReference())
							  ->GetTempRef();
			}
		}
		if (pVendor) {
			Interface::CreateRecipeMenu(pVendor, pCategory);
		}
	}
	return 1;
}

// The winnings level (0-4, quarters of the casino's maximum winnings) the player has reached at a casino.
// KEEP: an uninitialised read. The exe zeroes the dword above the casino's slot ([ebp-4]) but not the slot
// ([ebp-8]) ParseParameters fills, and tests that slot when ParseParameters returns before writing it (a null
// parameter list, a reference code other than 'r').
bool Cmd_GetCasinoWinningsLevel(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset) {
	// The casino's slot and the zeroed slot after it, which ParseParameters is passed with it (call 1's frame bytes).
	TESCasino* apCasino[2];
	apCasino[1] = 0;
	Script::ParseParameters(apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript,
		apScriptLocals, &apCasino[0]);
	TESCasino* pCasino = apCasino[0];
	if (!pCasino) {
		return 0;
	}
	for (BSSimpleList<CasinoData*>* pNode = pPlayer->GetCasinoDataList(); pNode && pNode->m_item;
		pNode = pNode->m_pkNext) {
		if (pNode->m_item->casinoID == pCasino->GetFormID()) {
			int32_t iMaxWinnings = pCasino->data.iMaxWinnings;
			int32_t iTotalWinnings = pNode->m_item->iTotalWinnings;
			afReturnValue = 0.0;
			for (int32_t i = 0; i < 4; ++i) {
				if (static_cast<double>(iTotalWinnings) >=
					static_cast<double>(iMaxWinnings) * (static_cast<double>(i + 1) * 0.25)) {
					afReturnValue = static_cast<double>(i + 1);
				}
			}
		}
	}
	return 1;
}

// Sets the player's winnings level (0-4) at a casino and its winnings to that many quarters of the casino's maximum.
// KEEP: an uninitialised read. The exe zeroes the level's slot and the dword above it ([ebp-8]) but not the casino's
// slot ([ebp-0xC]), and tests that slot when ParseParameters returns before writing it (a null parameter list, a
// reference code other than 'r').
bool Cmd_SetCasinoWinningsLevel(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset) {
	// The casino's slot and the zeroed slot after it, which ParseParameters is passed with it (call 1's frame bytes).
	TESCasino* apCasino[2];
	int32_t iLevel;
	apCasino[1] = 0;
	iLevel = 0;
	Script::ParseParameters(apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript,
		apScriptLocals, &apCasino[0], &iLevel);
	TESCasino* pCasino = apCasino[0];
	if (!pCasino) {
		return 0;
	}
	for (BSSimpleList<CasinoData*>* pNode = pPlayer->GetCasinoDataList(); pNode && pNode->m_item;
		pNode = pNode->m_pkNext) {
		if (pNode->m_item->casinoID == pCasino->GetFormID() && iLevel >= 0 && iLevel <= 4) {
			int32_t iMaxWinnings = pCasino->data.iMaxWinnings;
			pNode->m_item->iWinningLevel = static_cast<int16_t>(iLevel);
			int32_t iTotalWinnings =
				static_cast<int32_t>(static_cast<double>(iLevel) * 0.25 * static_cast<double>(iMaxWinnings));
			pNode->m_item->iTotalWinnings = iTotalWinnings;
		}
	}
	return 1;
}

bool Cmd_UnlockChallenge(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams, TESObjectREFR* apRefObject,
	TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals, double& afReturnValue,
	uint32_t& auiOffset) {
	TESChallenge* a8;
	a8 = 0;
	Script::ParseParameters(apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript,
		apScriptLocals, &a8);
	if (!a8) {
		return 0;
	}
	TESChallengeManager::UnlockChallenge(a8);
	return 1;
}

bool Cmd_RemoveRecurringFromChallenge(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset) {
	TESChallenge* a8;
	a8 = 0;
	Script::ParseParameters(apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript,
		apScriptLocals, &a8);
	if (!a8) {
		return 0;
	}
	a8->RemoveRecurringFromChallenge(1);
	return 1;
}

// An IterateInventory callback: appends the item's worn mark, count, name and form ID to the string apData.
uint8_t apCallback(ItemChange* apItem, BSStringT<char>* apData) {
	if (apData) {
		if (apItem) {
			const char* pWorn;
			char acBuffer[264];
			if (apItem->GetWorn(0)) {
				pWorn = "[E]";
			} else {
				pWorn = "";
			}
			uint32_t uiFormID = apItem->pContainerObj->iFormID;
			const char* pFullName = apItem->GetFullName();
			BSsprintf(acBuffer, 260, "%s%i %s (%08X),", pWorn, apItem->iNumber, pFullName, uiFormID);
			apData->operator+=(static_cast<const char*>(acBuffer));
		}
	}
	return 0;
}

bool TESAnimGroupIsJumpingLandAnim(TESAnimGroup* apThis, [[maybe_unused]] void* apEdx) {
	int32_t type = (int32_t)TESAnimGroup::GetType(apThis->sType);
	return (bool)((uint8_t)((type == 229) || ((type > 240) && (type <= 244))));
}

void ShakeData() {
	char modelPath[268];
	ShakeData_pModel = 0;
	BSsprintf(modelPath, 260u, "%s\\Characters\\CameraShake.nif", "Meshes");
	if (FileFinder::Locate(modelPath, 0, 0, 0xFFFFFFFFu)) {
		ShakeData_pModel = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
		NiFixedString apString(modelPath);
		NiFixedString* a2 = &apString;
		ShakeData_pModel->SetName(*a2);
	}
}

void FreeCameraShake() {
	if (ShakeData_pModel) {
		const char* HandleString = (&ShakeData_pModel->m_kName)->GetHandleString();
		pModelLoader->ReleaseFile(HandleString);
		ShakeData_pModel = 0;
	}
}

void DeleteCharacterProgressionData(CharacterProgression* apThis, [[maybe_unused]] void* apEdx) {
	apThis->DeleteData();
}

void set_11E07AB() {
	bAntiCheatActive = 1;
}

void SetLastAntiCheatTime() {
	uiAntiCheatTimer = GetTickCount() / 1000;
}

// Only the remaining time is cleared, as the game does.
NonLinearInterpolator::NonLinearInterpolator() : fRemainingTime(0.0f) {}

bool NonLinearInterpolator::Interpolate(float afDeltaTime) {
	fRemainingTime = (float)(fRemainingTime - afDeltaTime);
	return (bool)((uint8_t)(fRemainingTime <= 0.0));
}

bool NonLinearInterpolator::GetFinished() {
	return (bool)((uint8_t)(fRemainingTime <= 0.0));
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
		return 0;
	}
	fCurrent = afStart;
	fEnd = afEnd;
	fTotalTime = afTime;
	fRemainingTime = afTime;
	return 1;
}

float NonLinearInterpolator::GetCurrent() {
	float v5 = (fTotalTime - BSMAX(fRemainingTime, static_cast<float>(0.0))) / fTotalTime * 99.0;
	int32_t v7 = static_cast<int32_t>(std::floor(v5));
	if (v7 < 99) {
		if (!(fRemainingTime <= 0.0)) {
			float v3 = fTotalTime - fRemainingTime;
			return FLerp(fPercentages[v7], fPercentages[v7 + 1], 0.0, fTotalTime, v3);
		} else {
			return fPercentages[99];
		}
	} else {
		return fPercentages[99];
	}
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

// pMarkerData stays unset for a null reference, deliberately, as the game does.
AudioMarkerInfo::AudioMarkerInfo(TESObjectREFR* apRef) : pMarkerRef(apRef) {
	if (apRef) {
		pMarkerData = apRef->GetExtraAudioMarker();
	}
}

PLAYER_TARGET_LOC::PLAYER_TARGET_LOC()
	: pWorld(nullptr), pInterior(nullptr), Location(NiPoint3_ZERO), Angle(NiPoint3_ZERO), bResetWeather(false),
	  pfnArrivalFunc(nullptr), iArrivalFuncData(0), pFurnitureRef(nullptr), pFastTravelMarker(nullptr) {}

// The members are default-constructed (the lists, smart pointers, sound handles, perk entry list arrays and the
// character progression; the occupied region list with its flag 1). The field stores stay in the body, in the game's
// order, because they interleave with calls that read and change the same fields (SetCanFastTravel, the process swap,
// InitHavok, SetFOV, SetHardcore, CreateActorMover).
PlayerCharacter::PlayerCharacter() : Character(), m_AllOccupiedRegions(1) {
	RandomDoorSpaceMap.Init(37u);
	AutoMemContext tempMemContext(MEM_CONTEXT_PLAYER, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 711);
	SetInitialized(0);
	Cached1stPersonCameraPos = NiPoint3_ZERO;
	CachedWorldCameraPos = NiPoint3_ZERO;
	InitGunWobble();
	ShakeData();
	kVanityModePos.y = fVanityModeWheelDefault.Float();
	bAiControlledActivate = 0;
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
	btransporting = 0;
	bCanFastTravel = 0;
	SetCanFastTravel(1);
	bCanWait = 1;
	bChargen = 0;
	bAllowEGMCacheClear = 1;
	eskilladvance = ActorValue::ACTOR_VALUE_COUNT;
	pDefaultClass = nullptr;
	pListofActions = nullptr;
	pAutoAimActor = nullptr;
	BulletAutoAim = NiPoint3_ZERO;
	pPlayersTargetActor = nullptr;
	bAiControlledPackage = 0;
	pClassBasedOn = nullptr;
	bTravelUseDoor = 0;
	sp1stPersonLight = nullptr;
	sp3rdPersonLight = nullptr;
	bOnElevator = 1;
	// The player runs in the high process from the start: Character's process is replaced by a new HighProcess.
	HighProcess* pHighProcess = ::new (ourMalloc(sizeof(HighProcess))) HighProcess();
	pHighProcess->Copy(pCurrentProcess);
	ReferenceProcessLists.RemoveReference(this, GetCurrentProcessType());
	if (pCurrentProcess) {
		delete pCurrentProcess;
	}
	pCurrentProcess = pHighProcess;
	ReferenceProcessLists.RemoveReference(this, GetCurrentProcessType());
	PlayerCharacter::SetCurrentSpell(nullptr);
	PlayerCharacter::SetDesiredTarget(nullptr);
	pActiveEffectList = ::new (ourMalloc(sizeof(BSSimpleList<ActiveEffect*>))) BSSimpleList<ActiveEffect*>();
	MobileObject::InitHavok();
	bhkCharacterController* pcon = GetCharController();
	if (pcon) {
		pcon->SetPriority(1000u);
		pcon->SetCollisionGroup(9u);
		CFilter icolFilter;
		pcon->GetCollisionFilter(&icolFilter);
		pCameraCaster = ::new (ourMalloc(sizeof(CameraCaster))) CameraCaster(fCameraCasterSize.Float(), 9u);
	} else {
		pCameraCaster = nullptr;
	}
	b3rdPersonSaved = 0;
	bsave3rdPerson = 0;
	b3rdPerson = 0;
	bActually3rdPerson = 0;
	bWant3rdPerson = 0;
	bTemp3rdPerson = 0;
	bTemp3rdPersonSwitchBack = 0;
	bTemp1stPerson = 0;
	bTemp1stPersonSwitchBack = 0;
	bAlwaysRun = bAlwaysRunByDefault.Bool();
	bAutoMove = 0;
	SetFOV(BSSceneGraph::fDefaultFOV.Float());
	p1stPersonBipedAnim = nullptr;
	p1stPersonAnimation = nullptr;
	sp1stPerson3D = nullptr;
	fEyeHeight = 118.0f;
	spInventoryPC = nullptr;
	pInventoryAnimation = nullptr;
	chaseCameraPos = NiPoint3_ZERO;
	fChaseDistance = 0.0f;
	bSnapChaseCamera = 0;
	bGreetingPlayer = 0;
	ucControlsDisabled = 0;
	pClosestConversation = nullptr;
	bBlockActivate = 0;
	fBlockActivateTimer = 5.0f;
	ihourstosleep = 0;
	fSitHeadingDelta = 0.0f;
	cMurder = 0;
	bBeenAttacked = 0;
	pSelectedSpell = nullptr;
	pSelectedScroll = nullptr;
	pPlayerMapMarker = nullptr;
	pActiveQuest = nullptr;
	EatDrinkItems = ::new (ourMalloc(sizeof(BSSimpleList<MagicItem*>))) BSSimpleList<MagicItem*>();
	QueuedWornEnchantments = nullptr;
	iNumberTraining = 0;
	SetForceActivateRef(nullptr);
	bIsSleeping = 0;
	pPendingPoison = nullptr;
	bTelekinesisSelected = 0;
	pOccupiedRegion = nullptr;
	spGrabSpring = nullptr;
	pGrabbedObject = nullptr;
	eGrabType = PlayerCharacter::GRAB_NONE;
	fGrabObjectWeight = 0.0f;
	fGrabDistance = 0.0f;
	pInitialStateBuffer = nullptr;
	iTotalPlayingTime = 0;
	ResetLastPlayingTimeUpdate();
	for (int32_t j = 0; j < 5; ++j) {
		pCrimeCounts[j] = 0;
	}
	iCharacterSeed = std::time(0);
	SetAIControlledToPosition(0);
	SetAIControlledFromPosition(0);
	bInBorderContainedCell = 0;
	bReturnToLastKnownGoodPosition = 0;
	LastKnownGoodPosition = NiPoint3_ZERO;
	pLastKnownGoodLocation = nullptr;
	pLastKnownMusicType = nullptr;
	pBorderRegions = nullptr;
	pListofCasinoData = ::new (ourMalloc(sizeof(BSSimpleList<CasinoData*>))) BSSimpleList<CasinoData*>();
	pInactiveListofCaravanCards =
		::new (ourMalloc(sizeof(BSSimpleList<TESCaravanCard*>))) BSSimpleList<TESCaravanCard*>();
	pActiveListofCaravanCards =
		::new (ourMalloc(sizeof(BSSimpleList<TESCaravanCard*>))) BSSimpleList<TESCaravanCard*>();
	iCaravanCapWinnings = 0;
	iCaravanCapLosses = 0;
	iCaravanWinnings = 0;
	iCaravanLosses = 0;
	iCaravanLargestWinning = 0;
	iCasinoCheatLevel = 0;
	SetHardcore(GamePlayFormulas::HARDCORE_OFF, 0);
	bAlwaysHardcore = 1;
	eDifficultyLevel = static_cast<GamePlayFormulas::DifficultyLevel>(gs_iDifficulty_GamePlay.Int());
	eKillCameraSetting = static_cast<KILL_CAM_TYPE>(gs_iKillCamera_GamePlay.Int());
	pInventoryWeaponEffect = nullptr;
	bBeingChased = 0;
	bIsYoung = 0;
	bIsToddler = 0;
	bCanUsePowerArmor = 0;
	iCombatPersue = 0;
	pMapWorld = nullptr;
	fUFOCameraHeading = 0.0f;
	fUFOCameraPitch = 0.0f;
	UFOCameraPos = NiPoint3_ZERO;
	bShowQuestItemsInInventory = 1;
	iSelectedSpellCastSoundID = 0;
	SelectedSpellCastSound = BSSoundHandle();
	for (int32_t k = 0; k < 6; ++k) {
		MagicFailureSounds[k] = BSSoundHandle();
	}
	pLastExtDoorActivated = nullptr;
	DroppedRefList.RemoveAll();
	ResetInsufficientChargeMessage();
	fSortActorDistanceListTimer = 0.0f;
	fDropAngleMod = 0.0f;
	fLastDropAngleMod = 0.0f;
	iFormFlags |= 1024;
	fWorldFOV = BSSceneGraph::fDefaultWorldFOV.Float();
	f3rdPersonFOV = fOverShoulderFOV.Float();
	f1stPersonFOV = BSSceneGraph::fDefault1stPersonFOV.Float();
	pCurrentProcess->SetSustainedFire(0);
	PlayerCharacter::CreateActorMover();
	fProjectileReleaseTimer = 0.0f;
	iNumberofStealWarnings = 0;
	fStealWarningTimer = -1.0f;
	iNumberofPickpocketWarnings = 0;
	fPickPocketWarningTimer = -1.0f;
	pListofPercievedActors = nullptr;
	bMenuModeButtonClicked = 0;
	fMenuModeButtonTimer = 0.0f;
	fAmmoSwapButtonTimer = 0.0f;
	pIronsightsDOFInstance = nullptr;
	pVatsDOFInstance = nullptr;
	pVatsDRBInstance = nullptr;
	kCamera3rdPersonShoulderOffset = NiPoint3_ZERO;
	bSpeaking = 0;
	pQueuedTargetLoc = nullptr;
	pQueuedWeaponAttach = nullptr;
	pCombatGroup = nullptr;
	iTeammateCount = 0;
	bPlayerInCombat = 0;
	bAllCombatTargetsSearching = 0;
	fCombatTimer = 0.0f;
	fYieldTimer = 0.0f;
	spCameraRigidBody = nullptr;
	SetEncounterZone(BGSEncounterZone::GetNoZone());
	SetMegatonDestroyed(0);
	pHeadingInterpolator = ::new (ourMalloc(sizeof(NonLinearInterpolator))) NonLinearInterpolator();
	pLookingInterpolator = ::new (ourMalloc(sizeof(NonLinearInterpolator))) NonLinearInterpolator();
	ourMemset(pWobbleNodes, 0, sizeof(pWobbleNodes));
	ResetRockItAmmoWeight(0);
	pAIConversationRunning = nullptr;
	bHostileDetection = 0;
	iSandmanDetection = 0;
	spTargeted3D = nullptr;
	bTarget3DDistant = 0;
	bNightVisionOn = 0;
	bTurboISM = 0;
	pReputationUpdate = nullptr;
	fTimeInSlowMoCam = 0.0f;
	fKillCamCooldown = 0.0f;
	bIgnoresGTM = 0;
	pContinuousBeamPersistant = nullptr;
	bIsAcousticSpaceTransition = 0;
	fLastHelloTime = -1.0f;
	for (int32_t m = 0; m < 3; ++m) {
		PlayerAchievements[m].bUnlocked = 0;
		PlayerAchievements[m].iCurrentValue = 0;
		// The stored value is ~(cFlags & 2) != 0, which is always true, so bTracking is set to true as the game does.
		PlayerAchievements[m].bTracking = true;
	}
	pClosestAudioMarkerInfo = nullptr;
	fCounterAttackTimer = 0.0f;
	bCounterAttackCamera = 0;
	bHasCateyeActive = 0;
	bHasSpotterActive = 0;
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
	CloneInventory3D(0);
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
		pSelectedSpell->Unload(1);
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
	PlayerCharacter::Set3D(nullptr, 1);
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
	SetLightPointer(nullptr, 1);
	SetLightPointer(nullptr, 0);
	kConflictingPick.SetTarget(nullptr);
	kConflictingPick.PickObjects(NiPoint3_ZERO, NiPoint3_ZERO, 0);
	for (BSSimpleList<PerkRankData*>* pperkRankList = &Perks; pperkRankList; pperkRankList = pperkRankList->m_pkNext) {
		ourFree(pperkRankList->m_item);
	}
	Perks.RemoveAll();
	for (uint32_t j = 0; j < 74; ++j) {
		PerkEntryLists[j].RemoveAll();
	}
	for (BSSimpleList<PerkRankData*>* pperkRankList = &CompanionPerks; pperkRankList;
		pperkRankList = pperkRankList->m_pkNext) {
		ourFree(pperkRankList->m_item);
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
	int16_t v14 = 0;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		v14 = 6;
	}
	int16_t v15 = v14 + 616;
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 49u) {
		v15 += 308;
	}
	uint16_t v16 = v15 + 4;
	if (bDebugSaveBuffer.Bool()) {
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
			InitializationFlow("GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", v16,
				pheader->iFormID, pform->GetFormEditorID(), pheader->iFlags, 0x3248,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp");
		} else {
			InitializationFlow("GetSaveSize(): %-5i ending at line %i in file %s", v16, 0x3248,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp");
		}
	}
	uint16_t v17 = Character::GetSaveSize(aiFlags) + v16;
	uint16_t v11 = v17;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		v17 += 6;
	}
	if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
		v17 += ComputeAnimationSaveSize(this, p1stPersonAnimation);
	}
	uint16_t isize = v17 + 84;
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 40u && pSaveLoadGameOLD->GetCurrentVersion() < 45u) {
		isize += 24;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 57u) {
		isize += 192;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 63u) {
		++isize;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
		isize += 12;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 73u) {
		isize += 5;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
		isize += 10;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 120u) {
		++isize;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
		isize += 4;
	}
	uint16_t isize_2 = isize + 40;
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 40u && pSaveLoadGameOLD->GetCurrentVersion() < 45u) {
		isize_2 += 4;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
		isize_2 += 4;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 66u) {
		isize_2 += 4;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 87u) {
		isize_2 += 4;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 96u) {
		isize_2 += 4;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 99u) {
		isize_2 += 2;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 108u) {
		isize_2 += 4;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 111u) {
		isize_2 += 5 * RandomDoorSpaceMap.m_kAllocator.m_uiCount + 2;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 115u) {
		isize_2 += 2 + 4 * ConflictingObjects.ItemsInList();
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
		isize_2 += 2 + 8 * pListofCasinoData->ItemsInList();
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
		int16_t v20 = isize_2 + 2 + 4 * pInactiveListofCaravanCards->ItemsInList() + 2;
		isize_2 = v20 + 4 * pActiveListofCaravanCards->ItemsInList() + 20;
	}
	int16_t v21 = ActiveEffect::GetActiveEffectListSaveSize(pActiveEffectList, this) + isize_2;
	int16_t v2 = CharacterProgression::GetSaveSize(aiFlags);
	int16_t v22 = v2 + v21 + 6 + 4 * listTopics.ItemsInList() + 2;
	int16_t v23 = 6 * listQuestLog.ItemsInList() + v22;
	int16_t v24 = 5 * listObjectives.ItemsInList() + v23;
	int16_t v25 = EffectSettingCollection::GetKnownEffectsSaveSize() + v24;
	int16_t v26 = static_cast<TESNPC*>(GetObjectReference())->TESNPC::GetFaceGenSaveSize(this) + v25;
	const char* pfullname = GetFullName();
	uint16_t isize_3 = v26 + static_cast<uint8_t>(std::strlen(pfullname) + 1) + 1;
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 44u) {
		TESClass* pclass = static_cast<TESNPC*>(GetActorBase())->TESNPC::GetClass();
		uint32_t v4 = iPlayerCustomClass.Int();
		TESClass* pcustomClass = pDataHandler->GetClass(v4);
		isize_3 += 4;
		if (pclass) {
			if (pclass == pcustomClass) {
				isize_3 += pclass->GetSaveSize();
			}
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
		isize_3 += 4;
	}
	if (bDebugSaveBuffer.Bool()) {
		SaveFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader_2) {
			TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
			InitializationFlow("GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				isize_3 - v11, pheader_2->iFormID, pform_2->GetFormEditorID(), pheader_2->iFlags, 0x3365,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp");
		} else {
			InitializationFlow("GetSaveSize(): %-5i ending at line %i in file %s", isize_3 - v11, 0x3365,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp");
		}
	}
	return isize_3;
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
		pSaveLoadGameOLD->SaveGameDataOLD(&iBlockID, 4u);
		pBlockStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->SaveGameDataOLD(&sBlockSize, 2u);
	}
	SaveGameDataOLD(TemporaryActorValueModifiers, 308);
	SaveGameDataOLD(ScriptActorValueModifiers, 308);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 49u) {
		SaveGameDataOLD(DamageActorValueModifiers, 308);
	}
	SaveGameDataOLD(&fHealthModifier, 4);
	if (bDebugSaveBuffer.Bool()) {
		char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
		SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pHeader) {
			TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
			uint32_t iFlags = pHeader->iFlags;
			const char* pEditorID = pForm->GetFormEditorID();
			InitializationFlow("SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				static_cast<int32_t>(pEnd - pStart), pHeader->iFormID, pEditorID, iFlags, 13176,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp");
		} else {
			InitializationFlow("SaveGame(): %-5i ending at line %i in file %s", static_cast<int32_t>(pEnd - pStart),
				13176, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp");
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
		char* pBlockDataStart = pBlockStart;
		if (pEnd > pBlockDataStart + 0xFFFF) {
			Warning("SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 13176);
		}
		// The save buffer is untyped bytes: the block's size is stored as a short where the placeholder went.
		*reinterpret_cast<uint16_t*>(pBlockStart) = static_cast<uint16_t>(pEnd - pBlockDataStart);
	}
	Character::SaveGame(aiFlags);
	uint16_t sBlockSize_2 = 0;
	char* pBlockStart_2 = nullptr;
	char* pStart_2 = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
	if (bDebugSaveBuffer.Bool()) {
		pStart_2 = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		uint32_t iBlockID_2 = 0x424C4F4B;
		pSaveLoadGameOLD->SaveGameDataOLD(&iBlockID_2, 4u);
		pBlockStart_2 = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->SaveGameDataOLD(&sBlockSize_2, 2u);
	}
	if ((aiFlags & 0x10000000) != 0) {
		Animation::SaveAnimation(this, p1stPersonAnimation);
	}
	const NiPoint3* pMarkerPos;
	if (pPlayerMapMarker) {
		pMarkerPos = &pPlayerMapMarker->GetLocationOnReference();
	} else {
		pMarkerPos = &NiPoint3_ZERO;
	}
	float afMarkerPos[3];
	afMarkerPos[0] = pMarkerPos->x;
	afMarkerPos[1] = pMarkerPos->y;
	afMarkerPos[2] = pMarkerPos->z;
	SaveGameDataOLD(&b3rdPerson, 1);
	SaveGameDataOLD(&bTemp3rdPerson, 1);
	SaveGameDataOLD(&bAlwaysRun, 1);
	SaveGameDataOLD(&bAutoMove, 1);
	SaveGameDataOLD(&iSleepTime, 4);
	SaveGameDataOLD(&fsecondRunning, 4);
	SaveGameDataOLD(&fsecondSwimming, 4);
	SaveGameDataOLD(&fsecondSneaking, 4);
	SaveGameDataOLD(&bActorinSneakRange, 1);
	SaveGameDataOLD(&bGreetingPlayer, 1);
	SaveGameDataOLD(&fGreetTimer, 4);
	SaveGameDataOLD(&ihourstosleep, 4);
	SaveGameDataOLD(&cMurder, 1);
	SaveGameDataOLD(&iAmountStolenSold, 4);
	SaveGameDataOLD(&bBeenAttacked, 1);
	SaveGameDataOLD(&fSitHeadingDelta, 4);
	SaveGameDataOLD(afMarkerPos, 12);
	SaveGameDataOLD(&fEyeHeight, 4);
	SaveGameDataOLD(&iNumberTraining, 4);
	SaveGameDataOLD(&iNumAdvance, 4);
	pSaveLoadGameOLD->SaveGameDataOLD(&eskilladvance, 4u);
	SaveGameDataOLD(&bIsSleeping, 1);
	SaveGameDataOLD(&fFOV, 4);
	SaveGameDataOLD(&bChargen, 1);
	SaveGameDataOLD(&bTelekinesisSelected, 1);
	SaveGameDataOLD(&fProjectileReleaseTimer, 4);
	SaveGameDataOLD(&iTotalPlayingTime, 4);
	SaveGameDataOLD(&ucControlsDisabled, 1);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 40u && pSaveLoadGameOLD->GetCurrentVersion() < 45u) {
		SaveGameDataOLD(&LastKnownGoodPosition, 12);
		SaveGameDataOLD(&LastKnownGoodPosition, 12);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 57u) {
		int32_t pMiscStats[44];
		MiscStatManager::GetAll(pMiscStats);
		SaveGameDataOLD(pMiscStats, 172);
		SaveGameDataOLD(pCrimeCounts, 20);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 63u) {
		SaveGameDataOLD(&bBeingChased, 1);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
		pSaveLoadGameOLD->SaveGameDataOLD(&eGrabType, 4u);
		SaveGameDataOLD(&fGrabObjectWeight, 4);
		SaveGameDataOLD(&fGrabDistance, 4);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 73u) {
		SaveGameDataOLD(&iCombatPersue, 4);
		SaveGameDataOLD(&bShowQuestItemsInInventory, 1);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
		SaveGameDataOLD(&bTemp3rdPersonSwitchBack, 1);
		SaveGameDataOLD(&bCanFastTravel, 1);
		SaveGameDataOLD(&iCharacterSeed, 4);
		SaveNumericID(&iSelectedSpellCastSoundID, 4);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 120u) {
		bool bIronSights = GetIronSights();
		SaveGameDataOLD(&bIronSights, 1);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
		SaveGameDataOLD(&PlayerCharacter::kVanityModePos.y, 4);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
		uint32_t iCasinoChipID = 0;
		BSSimpleList<CasinoData*>* pCasinoList = GetCasinoDataList();
		int32_t iListSize = pCasinoList->ItemsInList();
		SaveGameDataOLD(&iListSize, 4);
		while (pCasinoList && pCasinoList->m_item) {
			iCasinoChipID = pCasinoList->m_item->casinoID;
			SaveGameDataOLD(&iCasinoChipID, 4);
			SaveGameDataOLD(&pCasinoList->m_item->iTotalWinnings, 4);
			SaveGameDataOLD(&pCasinoList->m_item->iWinningLevel, 2);
			pCasinoList = pCasinoList->m_pkNext;
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
		uint32_t iCardID = 0;
		BSSimpleList<TESCaravanCard*>* pCardList = GetCaravanCards1();
		int32_t iListSize_2 = pCardList->ItemsInList();
		SaveGameDataOLD(&iListSize_2, 4);
		while (pCardList && pCardList->m_item) {
			iCardID = pCardList->m_item->iFormID;
			SaveGameDataOLD(&iCardID, 4);
			pCardList = pCardList->m_pkNext;
		}
		pCardList = GetCaravanCards2();
		iListSize_2 = pCardList->ItemsInList();
		SaveGameDataOLD(&iListSize_2, 4);
		while (pCardList && pCardList->m_item) {
			iCardID = pCardList->m_item->iFormID;
			SaveGameDataOLD(&iCardID, 4);
			pCardList = pCardList->m_pkNext;
		}
		SaveGameDataOLD(&iCaravanCapWinnings, 4);
		SaveGameDataOLD(&iCaravanCapLosses, 4);
		SaveGameDataOLD(&iCaravanWinnings, 4);
		SaveGameDataOLD(&iCaravanLosses, 4);
		SaveGameDataOLD(&iCaravanLargestWinning, 4);
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
		SaveGameDataOLD(&sCount, 2);
		*pCount = sCount;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 108u) {
		uint32_t iOccupiedRegionWeatherID = 0;
		if (pOccupiedRegion) {
			if (pOccupiedRegion->pCurrentWeather) {
				iOccupiedRegionWeatherID = pOccupiedRegion->pCurrentWeather->iFormID;
			}
		}
		SaveNumericID(&iOccupiedRegionWeatherID, 4);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 111u) {
		uint16_t sMapSize = static_cast<uint16_t>(RandomDoorSpaceMap.m_kAllocator.m_uiCount);
		SaveGameDataOLD(&sMapSize, 2);
		uint16_t sMapCount = 0;
		NiTMapItem<uint32_t, uint8_t>* pPos = RandomDoorSpaceMap.GetFirstPos();
		while (pPos) {
			uint32_t iFormID = 0;
			uint8_t cIndex = 0;
			RandomDoorSpaceMap.GetNext(pPos, iFormID, cIndex);
			SaveNumericID(&iFormID, 4);
			SaveGameDataOLD(&cIndex, 1);
			++sMapCount;
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 115u) {
		uint16_t sCount_2 = 0;
		uint16_t* pCount_2 = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
		SaveGameDataOLD(&sCount_2, 2);
		for (BSSimpleList<TESObjectREFR*>* pList = &ConflictingObjects; pList && !pList->IsEmpty();
			pList = pList->m_pkNext) {
			TESObjectREFR* pRef = pList->m_item;
			uint32_t iRefID = 0;
			if (pRef) {
				iRefID = pRef->iFormID;
			}
			SaveNumericID(&iRefID, 4);
			++sCount_2;
		}
		*pCount_2 = sCount_2;
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
	SaveGameDataOLD(&sTopicCount, 2);
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
	SaveGameDataOLD(&sLogCount, 2);
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
		SaveGameDataOLD(&cStageIndex, 1);
		SaveGameDataOLD(&cStageItemIndex, 1);
		++sLogCount;
	}
	*pLogCount = sLogCount;
	uint16_t sObjectiveCount = 0;
	uint16_t* pObjectiveCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
	SaveGameDataOLD(&sObjectiveCount, 2);
	for (BSSimpleList<BGSQuestObjective*>* pObjectives = &listObjectives; pObjectives && !pObjectives->IsEmpty();
		pObjectives = pObjectives->m_pkNext) {
		BGSQuestObjective* pObjective = pObjectives->m_item;
		TESQuest* pObjectiveQuest = pObjective->pOwnerQuest;
		uint32_t iObjectiveQuestID = pObjectiveQuest->iFormID;
		uint32_t iObjectiveIndex = pObjective->iIndex;
		SaveNumericID(&iObjectiveQuestID, 4);
		SaveGameDataOLD(&iObjectiveIndex, 4);
		++sObjectiveCount;
	}
	*pObjectiveCount = sObjectiveCount;
	TESNPC* pNPC = static_cast<TESNPC*>(GetObjectReference());
	pNPC->SaveFaceGen(this);
	const char* pFullName = GetFullName();
	uint8_t cLength = std::strlen(pFullName) + 1;
	SaveGameDataOLD(&cLength, 1);
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
		if (pClass) {
			if (pClass == pCustomClass) {
				pClass->SaveGameTES();
			}
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
			InitializationFlow("SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				static_cast<int32_t>(pEnd - pStart_2), pHeader->iFormID, pEditorID, iFlags, 13714,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp");
		} else {
			InitializationFlow("SaveGame(): %-5i ending at line %i in file %s", static_cast<int32_t>(pEnd - pStart_2),
				13714, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp");
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
		char* pBlockDataStart = pBlockStart_2;
		if (pEnd > pBlockDataStart + 0xFFFF) {
			Warning("SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 13714);
		}
		// The save buffer is untyped bytes: the block's size is stored as a short where the placeholder went.
		*reinterpret_cast<uint16_t*>(pBlockStart_2) = static_cast<uint16_t>(pEnd - pBlockDataStart);
	}
}

void PlayerCharacter::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	uint32_t v2;
	uint32_t isaveGameCount_2;
	const NiPoint3* v4;
	int32_t CastingType;
	bool bAlwaysHardcoreDude;
	bool bHardcoreMode;
	uint32_t v16;
	uint8_t cstageIndex;
	uint8_t cstageitemIndex;
	uint32_t v73;
	float CameraShakeMult;
	float CameraShakeCurrentTime;
	float apData[3];
	uint32_t isize = 77;
	for (uint32_t i = 0; i < isize; ++i) {
		apSaveGameBuffer->SaveDataEndian(&TemporaryActorValueModifiers[i], 4u, 0);
	}
	for (uint32_t i_2 = 0; i_2 < isize; ++i_2) {
		apSaveGameBuffer->SaveDataEndian(&ScriptActorValueModifiers[i_2], 4u, 0);
	}
	for (uint32_t i_3 = 0; i_3 < isize; ++i_3) {
		apSaveGameBuffer->SaveDataEndian(&DamageActorValueModifiers[i_3], 4u, 0);
	}
	apSaveGameBuffer->SaveDataEndian(&fHealthModifier, 4u, 0);
	Character::SaveGame(apSaveGameBuffer);
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION)) {
		uint32_t isizePosition = apSaveGameBuffer->StartVariableSizedValue();
		uint32_t iposition1 = apSaveGameBuffer->iBufferPosition;
		p1stPersonAnimation->SaveGameBGS(apSaveGameBuffer);
		uint32_t iposition2 = apSaveGameBuffer->iBufferPosition;
		apSaveGameBuffer->SaveVariableSizedValue(iposition2 - iposition1, isizePosition);
	}
	apSaveGameBuffer->SaveDataEndian(&b3rdPerson, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bTemp3rdPerson, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bAlwaysRun, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bAutoMove, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&iSleepTime, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fsecondRunning, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fsecondSwimming, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fsecondSneaking, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bActorinSneakRange, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bGreetingPlayer, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&fGreetTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&ihourstosleep, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&cMurder, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&iAmountStolenSold, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bBeenAttacked, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bBlockActivate, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bIsYoung, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bIsToddler, 1u, 0);
	if (pPlayerMapMarker) {
		v4 = &pPlayerMapMarker->GetLocationOnReference();
	} else {
		v4 = &NiPoint3_ZERO;
	}
	apData[0] = v4->x;
	apData[1] = v4->y;
	apData[2] = v4->z;
	apSaveGameBuffer->SaveDataEndian(&fSitHeadingDelta, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(apData, 12u, 0);
	apSaveGameBuffer->SaveDataEndian(&fEyeHeight, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&iNumberTraining, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&eskilladvance, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bIsSleeping, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&fFOV, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&f1stPersonFOV, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fWorldFOV, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bChargen, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&fProjectileReleaseTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&iTotalPlayingTime, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&ucControlsDisabled, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bBeingChased, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&eGrabType, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fGrabObjectWeight, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fGrabDistance, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&iCombatPersue, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bShowQuestItemsInInventory, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bTemp3rdPersonSwitchBack, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bCanFastTravel, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&iCharacterSeed, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&PlayerCharacter::kVanityModePos.y, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fCombatTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fYieldTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&iNumberofStealWarnings, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fStealWarningTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&iNumberofPickpocketWarnings, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fPickPocketWarningTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bSpeaking, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bMegatonDestroyed, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bTemp1stPerson, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bTemp1stPersonSwitchBack, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bCanUsePowerArmor, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bHostileDetection, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&iSandmanDetection, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fBlockActivateTimer, 4u, 0);
	for (uint32_t i_4 = 0; i_4 < 5; ++i_4) {
		apSaveGameBuffer->SaveDataEndian(&pCrimeCounts[i_4], 4u, 0);
	}
	CharacterProgressionInfo.SaveGame(apSaveGameBuffer);
	TESForm* pplayerMarkerSpace = 0;
	if (pPlayerMapMarker) {
		pplayerMarkerSpace = (TESForm*)pPlayerMapMarker->GetWorldSpace();
		if (!pplayerMarkerSpace) {
			pplayerMarkerSpace = (TESForm*)pPlayerMapMarker->GetParentCell();
		}
	}
	apSaveGameBuffer->SaveFormID(pActiveQuest, 0);
	apSaveGameBuffer->SaveFormID(pDefaultClass, 0);
	apSaveGameBuffer->SaveFormID(pplayerMarkerSpace, 0);
	apSaveGameBuffer->SaveFormID(pOccupiedRegion, 0);
	if (pOccupiedRegion) {
		apSaveGameBuffer->SaveFormID(pOccupiedRegion->pCurrentWeather, 0);
	} else {
		apSaveGameBuffer->SaveFormID(static_cast<TESForm*>(0), 0);
	}
	apSaveGameBuffer->SaveFormID(pClosestConversation, 0);
	apSaveGameBuffer->SaveFormID(pAIConversationRunning, 0);
	apSaveGameBuffer->SaveFormID(pGrabbedObject, 0);
	apSaveGameBuffer->SaveFormID(pLastExtDoorActivated, 0);
	apSaveGameBuffer->SaveFormID(this->GetAutoAimActorSimple(), 0);
	apSaveGameBuffer->SaveFormID(pPlayersTargetActor, 0);
	uint32_t icount = 0;
	uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
	for (BSSimpleList<TESTopic*>* plist = GetTopicList(); plist; plist = plist->m_pkNext) {
		TESTopic* ptopic = plist->m_item;
		if (ptopic) {
			if (!ptopic->IsRandom()) {
				apSaveGameBuffer->SaveFormID(ptopic, 0);
				++icount;
			}
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(icount, isaveGameBufferPos);
	uint32_t isaveGameCount = 0;
	uint32_t isaveGameBufferPos_2 = apSaveGameBuffer->StartVariableSizedValue();
	for (BSSimpleList<BGSNote*>* plist_2 = &listNotes; plist_2; plist_2 = plist_2->m_pkNext) {
		TESForm* pForm = (TESForm*)plist_2->m_item;
		if (pForm) {
			apSaveGameBuffer->SaveFormID(pForm, 0);
			++isaveGameCount;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos_2);
	// The address &RockItLauncherAmmoList is never null, so it is not tested.
	isaveGameCount_2 = RockItLauncherAmmoList.iSize;
	uint32_t aiVariableValue = isaveGameCount_2;
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_2);
	for (uint32_t i_5 = 0; i_5 < aiVariableValue; ++i_5) {
		ItemChange** pchange = &RockItLauncherAmmoList.operator[](i_5);
		(*pchange)->SaveGame(apSaveGameBuffer);
	}
	uint32_t isaveGameCount_3 = 0;
	uint32_t isaveGameBufferPos_3 = apSaveGameBuffer->StartVariableSizedValue();
	for (BSSimpleList<CompassTarget*>* plist_3 = pListofPercievedActors; plist_3; plist_3 = plist_3->m_pkNext) {
		CompassTarget* pactorsNearPlayerData = plist_3->m_item;
		if (pactorsNearPlayerData) {
			apSaveGameBuffer->SaveFormID(pactorsNearPlayerData->pTarget, 0);
			apSaveGameBuffer->SaveDataEndian(&pactorsNearPlayerData->bIsHostile, 1u, 0);
			apSaveGameBuffer->SaveDataEndian(&pactorsNearPlayerData->bIsUndetected, 1u, 0);
			++isaveGameCount_3;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_3, isaveGameBufferPos_3);
	uint32_t isaveGameCount_4 = 0;
	uint32_t isaveGameBufferPos_4 = apSaveGameBuffer->StartVariableSizedValue();
	for (PerkRankData* pperkRank : Perks) {
		if (pperkRank) {
			apSaveGameBuffer->SaveFormID(pperkRank->pPerk, 0);
			apSaveGameBuffer->SaveDataEndian(&pperkRank->cCurrentRank, 1u, 0);
			++isaveGameCount_4;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_4, isaveGameBufferPos_4);
	uint32_t isaveGameCount_5 = 0;
	uint32_t isaveGameBufferPos_5 = apSaveGameBuffer->StartVariableSizedValue();
	for (PlayerActionObject* pplayerAction : *pListofActions) {
		if (pplayerAction) {
			apSaveGameBuffer->SaveDataEndian(pplayerAction, 4u, 0);
			apSaveGameBuffer->SaveDataEndian(&pplayerAction->ftimer, 4u, 0);
			apSaveGameBuffer->SaveFormID(pplayerAction->prefObj, 0);
			++isaveGameCount_5;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_5, isaveGameBufferPos_5);
	uint32_t isaveGameCount_6 = 0;
	uint32_t isaveGameBufferPos_6 = apSaveGameBuffer->StartVariableSizedValue();
	for (CasinoData* pcasinoData : *pListofCasinoData) {
		if (pcasinoData) {
			apSaveGameBuffer->SaveDataEndian(pcasinoData, 4u, 0);
			apSaveGameBuffer->SaveDataEndian(&pcasinoData->iTotalWinnings, 4u, 0);
			apSaveGameBuffer->SaveDataEndian(&pcasinoData->iWinningLevel, 2u, 0);
			++isaveGameCount_6;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_6, isaveGameBufferPos_6);
	uint32_t isaveGameCount_7 = 0;
	uint32_t isaveGameBufferPos_7 = apSaveGameBuffer->StartVariableSizedValue();
	for (BSSimpleList<TESCaravanCard*>* plist_7 = pInactiveListofCaravanCards; plist_7; plist_7 = plist_7->m_pkNext) {
		TESForm* pcaravanData = (TESForm*)plist_7->m_item;
		if (pcaravanData) {
			apSaveGameBuffer->SaveFormID(pcaravanData, 0);
			++isaveGameCount_7;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_7, isaveGameBufferPos_7);
	uint32_t isaveGameCount_8 = 0;
	uint32_t isaveGameBufferPos_8 = apSaveGameBuffer->StartVariableSizedValue();
	for (BSSimpleList<TESCaravanCard*>* plist_8 = pActiveListofCaravanCards; plist_8; plist_8 = plist_8->m_pkNext) {
		TESForm* pcaravanData_2 = (TESForm*)plist_8->m_item;
		if (pcaravanData_2) {
			apSaveGameBuffer->SaveFormID(pcaravanData_2, 0);
			++isaveGameCount_8;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_8, isaveGameBufferPos_8);
	apSaveGameBuffer->SaveDataEndian(&iCaravanCapWinnings, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&iCaravanCapLosses, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&iCaravanWinnings, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&iCaravanLosses, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&iCaravanLargestWinning, 4u, 0);
	uint32_t isaveGameCount_9 = 0;
	uint32_t isaveGameBufferPos_9 = apSaveGameBuffer->StartVariableSizedValue();
	for (BSSimpleList<TESQuestStageItem*>* plist_9 = GetQuestLog(); plist_9; plist_9 = plist_9->m_pkNext) {
		TESQuestStageItem* pstageItem = plist_9->m_item;
		if (pstageItem) {
			TESQuest* pquest = pstageItem->GetOwner();
			cstageIndex = pquest->GetStageIndexForStageItem(pstageItem);
			cstageitemIndex = pstageItem->ucIndex;
			apSaveGameBuffer->SaveFormID(pquest, 0);
			apSaveGameBuffer->SaveDataEndian(&cstageIndex, 1u, 0);
			apSaveGameBuffer->SaveDataEndian(&cstageitemIndex, 1u, 0);
			++isaveGameCount_9;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_9, isaveGameBufferPos_9);
	uint32_t isaveGameCount_10 = 0;
	uint32_t isaveGameBufferPos_10 = apSaveGameBuffer->StartVariableSizedValue();
	for (BGSQuestObjective* pobjective : listObjectives) {
		if (pobjective) {
			TESQuest* pquest_2 = pobjective->pOwnerQuest;
			v16 = pobjective->iIndex;
			apSaveGameBuffer->SaveFormID(pquest_2, 0);
			apSaveGameBuffer->SaveDataEndian(&v16, 4u, 0);
			++isaveGameCount_10;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_10, isaveGameBufferPos_10);
	ActiveEffect::SaveActiveEffectList(apSaveGameBuffer, pActiveEffectList);
	if (pCombatGroup) {
		v2 = pCombatGroup->iGroupID;
	} else {
		v2 = 0;
	}
	v73 = v2;
	apSaveGameBuffer->SaveDataEndian(&v73, 4u, 0);
	CameraShakeMult = GetCameraShakeMult();
	CameraShakeCurrentTime = GetCameraShakeCurrentTime();
	apSaveGameBuffer->SaveDataEndian(&CameraShakeMult, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&CameraShakeCurrentTime, 4u, 0);
	uint32_t isaveGameCount_11 = 0;
	uint32_t isaveGameBufferPos_11 = apSaveGameBuffer->StartVariableSizedValue();
	for (PerkRankData* pperkRank_2 : CompanionPerks) {
		if (pperkRank_2) {
			apSaveGameBuffer->SaveFormID(pperkRank_2->pPerk, 0);
			apSaveGameBuffer->SaveDataEndian(&pperkRank_2->cCurrentRank, 1u, 0);
			++isaveGameCount_11;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_11, isaveGameBufferPos_11);
	bAlwaysHardcoreDude = GetAlwaysHardcore();
	bHardcoreMode = IsHardcore();
	apSaveGameBuffer->SaveDataEndian(&bHardcoreMode, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bAlwaysHardcoreDude, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bCanWait, 1u, 0);
	int32_t v77 = 8;
	for (int32_t i_6 = 0; i_6 < v77; ++i_6) {
		TESAmmo* pAmmoHK = static_cast<TESAmmo*>(GetAmmoForHotKey(i_6));
		CastingType = 0;
		if (pAmmoHK) {
			CastingType = pAmmoHK->iFormID;
		}
		apSaveGameBuffer->SaveDataEndian(&CastingType, 4u, 0);
	}
	Script::SavePendingReferences(apSaveGameBuffer);
}

void PlayerCharacter::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	uint16_t iblockSize = 0;
	uint8_t* istartBufferPosition = nullptr;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		uint32_t iblockHeader;
		pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
		// 'BLOK'.
		if (iblockHeader != 0x424C4F4B) {
			LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
			if (pheader) {
				TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
						"loading form is %08X %s with version %i and flags %08X",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 13726, pheader->iFormID,
					pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags);
			} else {
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
						"version is %i",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 13726,
					pSaveLoadGameOLD->GetCurrentVersion());
			}
		}
		istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 59u) {
		LoadGameDataOLD(TemporaryActorValueModifiers, 308);
		LoadGameDataOLD(ScriptActorValueModifiers, 308);
		LoadGameDataOLD(DamageActorValueModifiers, 308);
	} else {
		LoadGameDataOLD(TemporaryActorValueModifiers, 304);
		LoadGameDataOLD(ScriptActorValueModifiers, 304);
		if (pSaveLoadGameOLD->GetCurrentVersion() >= 49u) {
			LoadGameDataOLD(DamageActorValueModifiers, 304);
		}
	}
	LoadGameDataOLD(&fHealthModifier, 4);
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
					Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
							"form is %08X %s with version %i and flags %08X",
						static_cast<int32_t>(istartBufferPosition + iblockSize - icurrentBufferPosition),
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 13750,
						pheader->iFormID, pEditorID, cVersion, iFlags);
				}
			} else {
				uint32_t iFlags = pheader->iFlags;
				int32_t cVersion = pheader->cVersion;
				const char* pEditorID = pform->GetFormEditorID();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
						"is %08X %s with version %i and flags %08X",
					static_cast<int32_t>(icurrentBufferPosition - (istartBufferPosition + iblockSize)),
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 13750, pheader->iFormID,
					pEditorID, cVersion, iFlags);
			}
		} else {
			if (icurrentBufferPosition <= istartBufferPosition + iblockSize) {
				if (icurrentBufferPosition < istartBufferPosition + iblockSize) {
					int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
					Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
							"%i",
						static_cast<int32_t>(istartBufferPosition + iblockSize - icurrentBufferPosition),
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 13750, iVersion);
				}
			} else {
				int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
					static_cast<int32_t>(icurrentBufferPosition - (istartBufferPosition + iblockSize)),
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 13750, iVersion);
			}
		}
	}
	Character::LoadGame(aiFlags, aiCurrentFlags);
	uint16_t iblockSize_2 = 0;
	uint8_t* istartBufferPosition_2 = nullptr;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		uint32_t iblockHeader;
		pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
		// 'BLOK'.
		if (iblockHeader != 0x424C4F4B) {
			LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
			if (pheader) {
				TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
						"loading form is %08X %s with version %i and flags %08X",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 13754, pheader->iFormID,
					pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags);
			} else {
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
						"version is %i",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 13754,
					pSaveLoadGameOLD->GetCurrentVersion());
			}
		}
		istartBufferPosition_2 = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize_2, 2u);
	}
	if ((aiFlags & 0x10000000) != 0) {
		Animation::LoadAnimation(this, p1stPersonAnimation);
	}
	NiPoint3 playerMarkerLoc;
	LoadGameDataOLD(&b3rdPerson, 1);
	LoadGameDataOLD(&bTemp3rdPerson, 1);
	LoadGameDataOLD(&bAlwaysRun, 1);
	LoadGameDataOLD(&bAutoMove, 1);
	LoadGameDataOLD(&iSleepTime, 4);
	LoadGameDataOLD(&fsecondRunning, 4);
	LoadGameDataOLD(&fsecondSwimming, 4);
	LoadGameDataOLD(&fsecondSneaking, 4);
	LoadGameDataOLD(&bActorinSneakRange, 1);
	LoadGameDataOLD(&bGreetingPlayer, 1);
	LoadGameDataOLD(&fGreetTimer, 4);
	LoadGameDataOLD(&ihourstosleep, 4);
	LoadGameDataOLD(&cMurder, 1);
	LoadGameDataOLD(&iAmountStolenSold, 4);
	LoadGameDataOLD(&bBeenAttacked, 1);
	LoadGameDataOLD(&fSitHeadingDelta, 4);
	LoadGameDataOLD(&playerMarkerLoc, 12);
	LoadGameDataOLD(&fEyeHeight, 4);
	LoadGameDataOLD(&iNumberTraining, 4);
	LoadGameDataOLD(&iNumAdvance, 4);
	pSaveLoadGameOLD->LoadGameDataOLD(&eskilladvance, 4u);
	LoadGameDataOLD(&bIsSleeping, 1);
	LoadGameDataOLD(&fFOV, 4);
	LoadGameDataOLD(&bChargen, 1);
	LoadGameDataOLD(&bTelekinesisSelected, 1);
	LoadGameDataOLD(&fProjectileReleaseTimer, 4);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 29u) {
		LoadGameDataOLD(&iTotalPlayingTime, 4);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 34u) {
		LoadGameDataOLD(&ucControlsDisabled, 1);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 40u && pSaveLoadGameOLD->GetCurrentVersion() < 45u) {
		pSaveLoadGameOLD->AdvanceBuffer(1u);
		LoadGameDataOLD(&LastKnownGoodPosition, 12);
		LoadGameDataOLD(&LastKnownGoodPosition, 12);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 53u && pSaveLoadGameOLD->GetCurrentVersion() < 113u) {
		pSaveLoadGameOLD->AdvanceBuffer(4u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 57u) {
		// Reads 0xAC bytes of misc stats into a buffer of that size.
		int32_t pMiscStats[43];
		LoadGameDataOLD(pMiscStats, sizeof(pMiscStats));
		MiscStatManager::SetAll(pMiscStats);
		LoadGameDataOLD(pCrimeCounts, 20);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 63u) {
		LoadGameDataOLD(&bBeingChased, 1);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&eGrabType, 4u);
		LoadGameDataOLD(&fGrabObjectWeight, 4);
		LoadGameDataOLD(&fGrabDistance, 4);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 73u) {
		LoadGameDataOLD(&iCombatPersue, 4);
		LoadGameDataOLD(&bShowQuestItemsInInventory, 1);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 74u && pSaveLoadGameOLD->GetCurrentVersion() < 89u) {
		pSaveLoadGameOLD->AdvanceBuffer(8u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 89u && pSaveLoadGameOLD->GetCurrentVersion() < 90u) {
		pSaveLoadGameOLD->AdvanceBuffer(4u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
		LoadGameDataOLD(&bTemp3rdPersonSwitchBack, 1);
		LoadGameDataOLD(&bCanFastTravel, 1);
		LoadGameDataOLD(&iCharacterSeed, 4);
		LoadNumericID(&iSelectedSpellCastSoundID, 4);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 120u) {
		bool bIsAiming;
		LoadGameDataOLD(&bIsAiming, 1);
		SetIronSights(bIsAiming, false, false);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
		LoadGameDataOLD(&PlayerCharacter::kVanityModePos.y, 4);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
		int32_t icount = 0;
		LoadGameDataOLD(&icount, 4);
		for (int32_t i = 0; i < icount; ++i) {
			void* apMemory = ourMalloc(sizeof(CasinoData));
			CasinoData* pcasinoData;
			if (apMemory) {
				pcasinoData = ::new (apMemory) CasinoData;
			} else {
				pcasinoData = nullptr;
			}
			LoadGameDataOLD(pcasinoData, 4);
			LoadGameDataOLD(&pcasinoData->iTotalWinnings, 4);
			LoadGameDataOLD(&pcasinoData->iWinningLevel, 2);
			pListofCasinoData->AddTail(pcasinoData);
		}
		if (uiAntiCheatTimer) {
			set_11E07AB();
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 122u) {
		int32_t icount = 0;
		uint32_t icardID = 0;
		pInactiveListofCaravanCards->RemoveAll();
		pActiveListofCaravanCards->RemoveAll();
		LoadGameDataOLD(&icount, 4);
		for (int32_t i = 0; i < icount; ++i) {
			LoadGameDataOLD(&icardID, 4);
			TESCaravanCard* pcard = static_cast<TESCaravanCard*>(TESForm::GetFormByNumericID(icardID));
			pInactiveListofCaravanCards->AddTail(pcard);
		}
		LoadGameDataOLD(&icount, 4);
		for (int32_t i = 0; i < icount; ++i) {
			LoadGameDataOLD(&icardID, 4);
			TESCaravanCard* pcard = static_cast<TESCaravanCard*>(TESForm::GetFormByNumericID(icardID));
			pActiveListofCaravanCards->AddTail(pcard);
		}
		LoadGameDataOLD(&iCaravanCapWinnings, 4);
		LoadGameDataOLD(&iCaravanCapLosses, 4);
		LoadGameDataOLD(&iCaravanWinnings, 4);
		LoadGameDataOLD(&iCaravanLosses, 4);
		LoadGameDataOLD(&iCaravanLargestWinning, 4);
	}
	// The reference fields below keep the saved form IDs until the load's later passes resolve them.
	uint32_t iconversationID;
	LoadNumericID(&iconversationID, 4);
	pClosestConversation = reinterpret_cast<DialoguePackage*>(static_cast<uintptr_t>(iconversationID));
	uint32_t iprisonMarkerID;
	LoadNumericID(&iprisonMarkerID, 4);
	pPrisonMarker = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(iprisonMarkerID));
	SetSelectedSpell(0);
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
		PlayerCharacter::pPrisonContainer =
			reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(iprisonContainerID));
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
		LoadGameDataOLD(&icount, 2);
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
		if (pOccupiedRegion) {
			if (poccupiedRegionWeather) {
				pOccupiedRegion->pCurrentWeather = poccupiedRegionWeather;
			}
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 111u) {
		RandomDoorSpaceMap.RemoveAll();
		uint16_t icount;
		LoadGameDataOLD(&icount, 2);
		for (uint32_t i = 0; i < icount; ++i) {
			uint32_t iformID;
			uint8_t cvalue;
			LoadNumericID(&iformID, 4);
			LoadGameDataOLD(&cvalue, 1);
			if (iformID) {
				RandomDoorSpaceMap.SetAt(iformID, cvalue);
			}
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 115u) {
		uint16_t icount;
		LoadGameDataOLD(&icount, 2);
		for (uint32_t i = 0; i < icount; ++i) {
			uint32_t irefID;
			LoadNumericID(&irefID, 4);
			TESObjectREFR* pref = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(irefID));
			ConflictingObjects.AddHead(pref);
		}
	}
	ActiveEffect::LoadActiveEffectListTES(pActiveEffectList, this);
	CharacterProgressionInfo.LoadGame(aiFlags, aiCurrentFlags);
	uint32_t iactivequestID;
	LoadNumericID(&iactivequestID, 4);
	if (iactivequestID) {
		TESForm* pquestForm = TESForm::GetFormByNumericID(iactivequestID);
		pActiveQuest = dynamic_cast<TESQuest*>(pquestForm);
		if (pActiveQuest) {
			pActiveQuest->UpdateCurrentTargetList(&listQuestTargets, &listObjectives);
		}
	} else {
		pActiveQuest = nullptr;
	}
	listTopics.RemoveAll();
	uint16_t itopicCount;
	LoadGameDataOLD(&itopicCount, 2);
	for (int32_t i = 0; i < itopicCount; ++i) {
		uint32_t itopicId;
		LoadNumericID(&itopicId, 4);
		TESForm* ptopicForm = TESForm::GetFormByNumericID(itopicId);
		// listTopics is a BSSimpleList<void*>; its items are the topics.
		void* ptopic = dynamic_cast<TESTopic*>(ptopicForm);
		if (ptopic) {
			listTopics.AddHead(ptopic);
		}
	}
	TESTopic::SortTopics(1, GetTopicList());
	listQuestLog.RemoveAll();
	uint16_t ilogCount;
	LoadGameDataOLD(&ilogCount, 2);
	for (int32_t i = 0; i < ilogCount; ++i) {
		uint32_t iquestId;
		uint8_t cstageIndex;
		uint8_t cstageItemIndex;
		LoadNumericID(&iquestId, 4);
		LoadGameDataOLD(&cstageIndex, 1);
		LoadGameDataOLD(&cstageItemIndex, 1);
		TESForm* pquestForm = TESForm::GetFormByNumericID(iquestId);
		TESQuest* pquest = dynamic_cast<TESQuest*>(pquestForm);
		if (pquest) {
			TESQuestStage* pstage = pquest->GetStage(cstageIndex);
			if (pstage) {
				// listQuestLog is a BSSimpleList<void*>; its items are the stage items.
				void* pstageItem = pstage->GetStageItem(cstageItemIndex);
				if (pstageItem) {
					listQuestLog.AddTail(pstageItem);
				}
			}
		}
	}
	listObjectives.RemoveAll();
	uint16_t iobjectiveCount;
	LoadGameDataOLD(&iobjectiveCount, 2);
	for (int32_t i = 0; i < iobjectiveCount; ++i) {
		uint32_t iquestId;
		uint32_t iobjectiveIndex;
		LoadNumericID(&iquestId, 4);
		LoadGameDataOLD(&iobjectiveIndex, 4);
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
	LoadGameDataOLD(&clength, 1);
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
		if (pclass) {
			if (pclass == pcustomClass) {
				pclass->LoadGameTES();
			}
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
			if (icurrentBufferPosition <= istartBufferPosition_2 + iblockSize_2) {
				if (icurrentBufferPosition < istartBufferPosition_2 + iblockSize_2) {
					uint32_t iFlags = pheader->iFlags;
					int32_t cVersion = pheader->cVersion;
					const char* pEditorID = pform->GetFormEditorID();
					Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
							"form is %08X %s with version %i and flags %08X",
						static_cast<int32_t>(istartBufferPosition_2 + iblockSize_2 - icurrentBufferPosition),
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 14280,
						pheader->iFormID, pEditorID, cVersion, iFlags);
				}
			} else {
				uint32_t iFlags = pheader->iFlags;
				int32_t cVersion = pheader->cVersion;
				const char* pEditorID = pform->GetFormEditorID();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
						"is %08X %s with version %i and flags %08X",
					static_cast<int32_t>(icurrentBufferPosition - (istartBufferPosition_2 + iblockSize_2)),
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 14280, pheader->iFormID,
					pEditorID, cVersion, iFlags);
			}
		} else {
			if (icurrentBufferPosition <= istartBufferPosition_2 + iblockSize_2) {
				if (icurrentBufferPosition < istartBufferPosition_2 + iblockSize_2) {
					int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
					Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
							"%i",
						static_cast<int32_t>(istartBufferPosition_2 + iblockSize_2 - icurrentBufferPosition),
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 14280, iVersion);
				}
			} else {
				int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
					static_cast<int32_t>(icurrentBufferPosition - (istartBufferPosition_2 + iblockSize_2)),
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 14280, iVersion);
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
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&TemporaryActorValueModifiers[i]), 4);
	}
	for (uint32_t i = 0; i < uiValueCount; ++i) {
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ScriptActorValueModifiers[i]), 4);
	}
	for (uint32_t i = 0; i < uiValueCount; ++i) {
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&DamageActorValueModifiers[i]), 4);
	}
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fHealthModifier), 4);
	Character::LoadGame(apLoadGameBuffer);

	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION)) {
		TESBoundObject* pObject = GetObjectReference();
		pSaveLoadGame->QueueSubBuffer(QUEUED_SUB_BUFFER_ANIMATION, apLoadGameBuffer, pObject);
	}

	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&b3rdPerson), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTemp3rdPerson), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bAlwaysRun), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bAutoMove), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iSleepTime), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fsecondRunning), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fsecondSwimming), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fsecondSneaking), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bActorinSneakRange), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bGreetingPlayer), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fGreetTimer), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ihourstosleep), 4);
	apLoadGameBuffer->LoadData(&cMurder, 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iAmountStolenSold), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bBeenAttacked), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bBlockActivate), 1);

	// A change of age rebuilds the head and the 3D.
	bool bWasYoung = bIsYoung;
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bIsYoung), 1);
	if (bIsYoung != bWasYoung && Get3D()) {
		static_cast<TESNPC*>(GetActorBase())->ClearHead();
		BaseProcess* pProcess = GetCurrentAIProcess();
		pProcess->Set3DUpdateFlag(RESET_FACE);
	}
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bIsToddler), 1);

	NiPoint3 kPlacedMarkerPos;
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fSitHeadingDelta), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&kPlacedMarkerPos), 0xC);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fEyeHeight), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iNumberTraining), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eskilladvance), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bIsSleeping), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fFOV), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&f1stPersonFOV), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fWorldFOV), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bChargen), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fProjectileReleaseTimer), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iTotalPlayingTime), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucControlsDisabled), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bBeingChased), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eGrabType), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fGrabObjectWeight), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fGrabDistance), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCombatPersue), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bShowQuestItemsInInventory), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTemp3rdPersonSwitchBack), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bCanFastTravel), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCharacterSeed), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&kVanityModePos.y), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fCombatTimer), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fYieldTimer), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iNumberofStealWarnings), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fStealWarningTimer), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iNumberofPickpocketWarnings), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPickPocketWarningTimer), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSpeaking), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bMegatonDestroyed), 1);
	if (apLoadGameBuffer->GetVersion() >= 0xD) {
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTemp1stPerson), 1);
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTemp1stPersonSwitchBack), 1);
	}
	if (apLoadGameBuffer->GetVersion() >= 0x10) {
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bCanUsePowerArmor), 1);
	}
	if (apLoadGameBuffer->GetVersion() >= 0x12) {
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHostileDetection), 1);
	}
	if (apLoadGameBuffer->GetVersion() >= 0x15) {
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iSandmanDetection), 4);
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fBlockActivateTimer), 4);
		for (uint32_t i = 0; i < 5; ++i) {
			apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pCrimeCounts[i]), 4);
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
		void* pMemory = ourMalloc(sizeof(ItemChange));
		*ppAmmo = pMemory ? ::new (pMemory) ItemChange : nullptr;
		(*ppAmmo)->LoadGame(apLoadGameBuffer);
	}
	ResetRockItAmmoWeight(true);

	FreeCompassTargets();
	uint32_t uiTargetCount = apLoadGameBuffer->LoadVariableSizedValue();
	if (uiTargetCount) {
		void* pListMemory = ourMalloc(sizeof(BSSimpleList<CompassTarget*>));
		pListofPercievedActors = pListMemory ? ::new (pListMemory) BSSimpleList<CompassTarget*> : nullptr;
		for (uint32_t i = 0; i < uiTargetCount; ++i) {
			void* pMemory = ourMalloc(sizeof(CompassTarget));
			CompassTarget* pTarget = pMemory ? ::new (pMemory) CompassTarget : nullptr;
			apLoadGameBuffer->LoadFormID(pTarget->pTarget);
			apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pTarget->bIsHostile), 1);
			apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pTarget->bIsUndetected), 1);
			pListofPercievedActors->AddHead(pTarget);
		}
	}

	uint32_t uiPerkCount = apLoadGameBuffer->LoadVariableSizedValue();
	for (uint32_t i = 0; i < uiPerkCount; ++i) {
		PerkRankData* pRank = static_cast<PerkRankData*>(ourMalloc(sizeof(PerkRankData)));
		pRank->pPerk = dynamic_cast<BGSPerk*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pRank->cCurrentRank), 1);
		Perks.AddHead(pRank);
	}
	// EXE-INLINED none: the perk-rank list walk that frees and unlinks ranks without a perk
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
		} else {
			bRemove = true;
		}
		if (bRemove) {
			if (pPrevRankNode) {
				pPrevRankNode->Remove(pRankNode->GetItem());
				pRankNode = pPrevRankNode->GetNext();
			} else {
				pRankNode->RemoveHead();
			}
		} else {
			pPrevRankNode = pRankNode;
			pRankNode = pRankNode->GetNext();
		}
	}

	uint32_t uiActionCount = apLoadGameBuffer->LoadVariableSizedValue();
	if (uiActionCount) {
		if (pListofActions) {
			pListofActions->RemoveAll();
		} else {
			void* pListMemory = ourMalloc(sizeof(BSSimpleList<PlayerActionObject*>));
			pListofActions = pListMemory ? ::new (pListMemory) BSSimpleList<PlayerActionObject*> : nullptr;
		}
		for (uint32_t i = 0; i < uiActionCount; ++i) {
			void* pMemory = ourMalloc(sizeof(PlayerActionObject));
			PlayerActionObject* pAction = pMemory ? ::new (pMemory) PlayerActionObject : nullptr;
			apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pAction->eAction), 4);
			apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pAction->ftimer), 4);
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
			void* pListMemory = ourMalloc(sizeof(BSSimpleList<CasinoData*>));
			pListofCasinoData = pListMemory ? ::new (pListMemory) BSSimpleList<CasinoData*> : nullptr;
		}
		for (uint32_t i = 0; i < uiCasinoCount; ++i) {
			void* pMemory = ourMalloc(sizeof(CasinoData));
			CasinoData* pCasino = pMemory ? ::new (pMemory) CasinoData : nullptr;
			apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pCasino->casinoID), 4);
			apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pCasino->iTotalWinnings), 4);
			apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pCasino->iWinningLevel), 2);
			pListofCasinoData->AddHead(pCasino);
		}
	}
	if (uiAntiCheatTimer) {
		set_11E07AB();
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
			void* pListMemory = ourMalloc(sizeof(BSSimpleList<TESCaravanCard*>));
			pInactiveListofCaravanCards = pListMemory ? ::new (pListMemory) BSSimpleList<TESCaravanCard*> : nullptr;
		}
		for (uint32_t i = 0; i < uiInactiveCardCount; ++i) {
			void* pMemory = ourMalloc(sizeof(TESCaravanCard));
			TESCaravanCard* pCard = pMemory ? ::new (pMemory) TESCaravanCard : nullptr;
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
			void* pListMemory = ourMalloc(sizeof(BSSimpleList<TESCaravanCard*>));
			pActiveListofCaravanCards = pListMemory ? ::new (pListMemory) BSSimpleList<TESCaravanCard*> : nullptr;
		}
		for (uint32_t i = 0; i < uiActiveCardCount; ++i) {
			void* pMemory = ourMalloc(sizeof(TESCaravanCard));
			TESCaravanCard* pCard = pMemory ? ::new (pMemory) TESCaravanCard : nullptr;
			pCard = dynamic_cast<TESCaravanCard*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
			pActiveListofCaravanCards->AddHead(pCard);
		}
	}
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCaravanCapWinnings), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCaravanCapLosses), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCaravanWinnings), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCaravanLosses), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCaravanLargestWinning), 4);

	// The quest log: a quest, then its stage index and the stage item's index.
	uint32_t uiLogCount = apLoadGameBuffer->LoadVariableSizedValue();
	for (uint32_t i = 0; i < uiLogCount; ++i) {
		TESQuest* pQuest = nullptr;
		uint8_t ucStage = 0;
		uint8_t ucItem = 0;
		pQuest = dynamic_cast<TESQuest*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucStage), 1);
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucItem), 1);
		if (pQuest) {
			TESQuestStage* pStage = pQuest->GetStage(ucStage);
			if (pStage) {
				void* pItem = pStage->GetStageItem(ucItem);
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
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiObjective), 4);
		if (pQuest) {
			BGSQuestObjective* pObjective = pQuest->GetObjective(uiObjective);
			if (pObjective) {
				listObjectives.AddTail(pObjective);
			}
		}
	}

	ActiveEffect::LoadActiveEffectList(apLoadGameBuffer, pActiveEffectList);
	uint32_t uiCombatGroupID = 0;
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiCombatGroupID), 4);
	if (uiCombatGroupID) {
		pCombatGroup = pCombatManager->GetCombatGroup(uiCombatGroupID);
	}
	if (apLoadGameBuffer->GetVersion() >= 8) {
		float fShakeMult = 0.0f;
		float fShakeTime = 0.0f;
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fShakeMult), 4);
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fShakeTime), 4);
		SetCameraShakeData(fShakeMult, fShakeTime);
	}

	if (apLoadGameBuffer->GetVersion() >= 0x16) {
		uint32_t uiCompanionPerkCount = apLoadGameBuffer->LoadVariableSizedValue();
		for (uint32_t i = 0; i < uiCompanionPerkCount; ++i) {
			PerkRankData* pRank = static_cast<PerkRankData*>(ourMalloc(sizeof(PerkRankData)));
			pRank->pPerk = dynamic_cast<BGSPerk*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
			apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pRank->cCurrentRank), 1);
			CompanionPerks.AddHead(pRank);
		}
		// EXE-INLINED none: the perk-rank list walk that frees and unlinks ranks without a perk
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
			} else {
				bRemove = true;
			}
			if (bRemove) {
				if (pPrevNode) {
					pPrevNode->Remove(pNode->GetItem());
					pNode = pPrevNode->GetNext();
				} else {
					pNode->RemoveHead();
				}
			} else {
				pPrevNode = pNode;
				pNode = pNode->GetNext();
			}
		}
	}

	if (apLoadGameBuffer->GetVersion() >= 0x17) {
		uint8_t ucTracked = 1;
		uint8_t ucHardcore = 0;
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucHardcore), 1);
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucTracked), 1);
		SetHardcore(static_cast<GamePlayFormulas::HardcoreSetting>(ucHardcore), true);
		if (!ucTracked) {
			ClearsHardcoreFlag();
		}
	}
	if (apLoadGameBuffer->GetVersion() >= 0x19) {
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bCanWait), 1);
	}
	if (apLoadGameBuffer->GetVersion() >= 0x1A) {
		int32_t iHotkeyCount = 8;
		for (int32_t i = 0; i < iHotkeyCount; ++i) {
			uint32_t uiAmmoID = 0;
			TESForm* pAmmo = nullptr;
			apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiAmmoID), 4);
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
				TESObjectWEAP* pWeapon =
					static_cast<TESObjectWEAP*>(pWeaponProcess->GetCurrentWeapon()->GetItemChangeObject());
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
	TESObjectREFR* pconflictID;
	Character::InitLoadGame(aiFlags, aiCurrentFlags);
	uint32_t aiSearchID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pClosestConversation));
	if (aiSearchID) {
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(aiSearchID);
		pClosestConversation = dynamic_cast<DialoguePackage*>(FormByNumericID);
	}
	uint32_t iprisonMarkerID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPrisonMarker));
	if (pPrisonMarker) {
		TESForm* v4 = TESForm::GetFormByNumericID(iprisonMarkerID);
		pPrisonMarker = dynamic_cast<TESObjectREFR*>(v4);
	}
	uint32_t pSelectedSpell = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->pSelectedSpell));
	this->pSelectedSpell = 0;
	if (pSelectedSpell) {
		TESForm* pform = TESForm::GetFormByNumericID(pSelectedSpell);
		MagicItemForm* pmagicItemForm = dynamic_cast<MagicItemForm*>(pform);
		MagicItemObject* pmagicItemObject = dynamic_cast<MagicItemObject*>(pform);
		if (pmagicItemForm) {
			SetSelectedSpell(pmagicItemForm);
		} else {
			if (pmagicItemObject) {
				SetSelectedSpell(pmagicItemObject);
			}
		}
	}
	uint32_t icurrentSpellID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentSpell));
	if (icurrentSpellID) {
		pCurrentSpell = MagicItem::GetMagicItemByNumericID(icurrentSpellID);
	}
	uint32_t idesiredTargetID__iconversationID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDesiredTarget));
	if (idesiredTargetID__iconversationID) {
		pDesiredTarget = MagicTarget::GetMagicTargetByNumericID(idesiredTargetID__iconversationID);
	}
	uint32_t pSelectedScroll = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->pSelectedScroll));
	if (pSelectedScroll) {
		TESForm* v5 = TESForm::GetFormByNumericID(pSelectedScroll);
		TESObjectBOOK* v6 = dynamic_cast<TESObjectBOOK*>(v5);
		SetSelectedScroll(v6);
	}
	uint32_t pDefaultClass = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->pDefaultClass));
	if (pDefaultClass) {
		TESForm* v7 = TESForm::GetFormByNumericID(pDefaultClass);
		this->pDefaultClass = dynamic_cast<TESClass*>(v7);
	}
	uint32_t pPendingPoison = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->pPendingPoison));
	if (pPendingPoison) {
		TESForm* v8 = TESForm::GetFormByNumericID(pPendingPoison);
		this->pPendingPoison = dynamic_cast<AlchemyItem*>(v8);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
		uint32_t igrabbedObjectID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pGrabbedObject));
		if (igrabbedObjectID) {
			TESForm* v9 = TESForm::GetFormByNumericID(igrabbedObjectID);
			pGrabbedObject = dynamic_cast<TESObjectREFR*>(v9);
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 66u) {
		uint32_t iprisonContainedID =
			static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PlayerCharacter::pPrisonContainer));
		if (PlayerCharacter::pPrisonContainer) {
			TESForm* v10 = TESForm::GetFormByNumericID(iprisonContainedID);
			PlayerCharacter::pPrisonContainer = dynamic_cast<TESObjectREFR*>(v10);
		}
	}
	// From version 99 the game walks a list it has set to null, so the loop never runs: only the version test is left.
	pSaveLoadGameOLD->GetCurrentVersion();
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 115u) {
		BSSimpleList<TESObjectREFR*>* plist = &ConflictingObjects;
		BSSimpleList<TESObjectREFR*>* plastList = 0;
		while (plist && !plist->IsEmpty()) {
			pconflictID = plist->m_item;
			uint32_t m_item = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pconflictID));
			TESObjectREFR* pref = 0;
			if (m_item) {
				TESForm* v11 = TESForm::GetFormByNumericID(m_item);
				pref = dynamic_cast<TESObjectREFR*>(v11);
				plist->SetItem(pref);
			}
			if (pref) {
				plastList = plist;
				plist = plist->m_pkNext;
			} else {
				if (plastList) {
					plastList->Remove(pconflictID);
					plist = plastList->m_pkNext;
				} else {
					plist->RemoveHead();
				}
			}
		}
	}
	ActiveEffect::InitLoadGameActiveEffectList(pActiveEffectList, this);
	pCurrentProcess->ClearFaceAnimationData();
	TESNPC* pnpc = static_cast<TESNPC*>(GetObjectReference());
	pnpc->InitLoadFaceGen(this);
	SetAIControlledToPosition(0);
	SetAIControlledFromPosition(0);
	SetControlsDisabled(ucControlsDisabled);
}

void PlayerCharacter::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	Actor* v18;
	Actor* v19;
	TESObjectREFR* v20;
	TESObjectREFR* v21;
	DialoguePackage* v23;
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	Character::InitLoadGame(apLoadGameBuffer);
	uint32_t iclosestConversationID =
		pSaveLoadGame->GetChangedFormID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pClosestConversation)));
	if (iclosestConversationID) {
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(iclosestConversationID);
		v23 = dynamic_cast<DialoguePackage*>(FormByNumericID);
	} else {
		v23 = 0;
	}
	pClosestConversation = v23;
	if (apLoadGameBuffer->GetVersion() >= 18u) {
		DialoguePackage* v22;
		uint32_t iAIConversationRunningID =
			pSaveLoadGame->GetChangedFormID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pAIConversationRunning)));
		if (iAIConversationRunningID) {
			TESForm* v3 = TESForm::GetFormByNumericID(iAIConversationRunningID);
			v22 = dynamic_cast<DialoguePackage*>(v3);
		} else {
			v22 = 0;
		}
		pAIConversationRunning = v22;
	}
	if (pGrabbedObject) {
		TESForm* v4 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pGrabbedObject)));
		v21 = dynamic_cast<TESObjectREFR*>(v4);
	} else {
		v21 = 0;
	}
	pGrabbedObject = v21;
	if (pLastExtDoorActivated) {
		TESForm* v5 =
			TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLastExtDoorActivated)));
		v20 = dynamic_cast<TESObjectREFR*>(v5);
	} else {
		v20 = 0;
	}
	pLastExtDoorActivated = v20;
	if (this->GetAutoAimActorSimple()) {
		TESForm* v6 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this
				->GetAutoAimActorSimple())));
		v19 = dynamic_cast<Actor*>(v6);
	} else {
		v19 = 0;
	}
	pAutoAimActor = v19;
	if (pPlayersTargetActor) {
		TESForm* v7 =
			TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPlayersTargetActor)));
		v18 = dynamic_cast<Actor*>(v7);
	} else {
		v18 = 0;
	}
	pPlayersTargetActor = v18;
	BSSimpleList<CompassTarget*>* plist = pListofPercievedActors;
	BSSimpleList<CompassTarget*>* plastList = 0;
	while (plist && !plist->IsEmpty()) {
		CompassTarget* pactorsNearPlayerData = plist->m_item;
		bool bremove = 0;
		if (pactorsNearPlayerData) {
			Actor* pactor;
			if (pactorsNearPlayerData->pTarget) {
				TESForm* v8 =
					TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pactorsNearPlayerData
							->pTarget)));
				pactor = dynamic_cast<Actor*>(v8);
			} else {
				pactor = 0;
			}
			pactorsNearPlayerData->pTarget = pactor;
			if (!pactorsNearPlayerData->pTarget || !pactorsNearPlayerData->pTarget->GetObjectReference()) {
				ourFree(pactorsNearPlayerData);
				bremove = 1;
			}
		} else {
			bremove = 1;
		}
		if (bremove) {
			if (plastList) {
				plastList->Remove(plist->m_item);
				plist = plastList->m_pkNext;
			} else {
				plist->RemoveHead();
			}
		} else {
			plastList = plist;
			plist = plist->m_pkNext;
		}
	}
	for (PlayerActionObject* pplayerAction : *pListofActions) {
		if (pplayerAction) {
			TESObjectREFR* v16;
			if (pplayerAction->prefObj) {
				TESForm* v9 =
					TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pplayerAction
							->prefObj)));
				v16 = dynamic_cast<TESObjectREFR*>(v9);
			} else {
				v16 = 0;
			}
			pplayerAction->prefObj = v16;
			if (pplayerAction->prefObj) {
				pplayerAction->prefObj->SetTargeted(1);
			}
		}
	}
	SetControlsDisabled(ucControlsDisabled);
	ActiveEffect::InitLoadActiveEffectList(apLoadGameBuffer, pActiveEffectList);
	for (PerkRankData* pperkRank : Perks) {
		if (pperkRank) {
			if (pperkRank->pPerk) {
				pperkRank->pPerk->AddPerkEntryPoints(this, pperkRank->cCurrentRank, 0);
			}
		}
	}
	if (p1stPersonCamera) {
		p1stPersonCamera->SetRotate(NiMatrix3_IDENTITY);
	}
	double v15 = GetActorFloatValue(ActorValue::HEALTH);
	float fhealthRatio = v15 / GetPermanentActorFloatValue(ActorValue::HEALTH);
	if (!(fPlayerHealthHeartbeatSlow.Float() >= static_cast<double>(fhealthRatio)) ||
		!(fPlayerHealthHeartbeatFast.Float() < static_cast<double>(fhealthRatio))) {
		if (fPlayerHealthHeartbeatFast.Float() >= static_cast<double>(fhealthRatio)) {
			BSAudio* v11 = &BSAudio::QInstance();
			BSSoundHandle* SoundHandleByName = v11->GetSoundHandleByName(&arHandle, "UIHealthHeartbeatBLP", 49u);
			StatusSoundHandle.operator=(*SoundHandleByName);
			StatusSoundHandle.Play(1);
		}
	} else {
		BSAudio* v10 = &BSAudio::QInstance();
		BSSoundHandle* v14 = v10->GetSoundHandleByName(&a1, "UIHealthHeartbeatALP", 49u);
		StatusSoundHandle.operator=(*v14);
		StatusSoundHandle.Play(1);
	}
	if (Get3D()) {
		if (!apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_LEVELED_INVENTORY | CHANGE_REFR_INVENTORY)) {
			if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_LEVELED_INVENTORY |
					CHANGE_REFR_INVENTORY)) {
				TESNPC* pnpc = static_cast<TESNPC*>(GetActorBase());
				pnpc->InitDefaultWorn(this, 1, 1, 0, 1);
				if (pCurrentProcess) {
					if (pCurrentProcess->GetAll3DUpdateFlags()) {
						pCurrentProcess->Update3dModel(this);
					}
				}
			}
		}
	}
}

void PlayerCharacter::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
	Actor::FinishInitLoadGame(aiFlags, aiOldFlags);
	if (Get3D(1)) {
		SetFirstPerson(!b3rdPerson);
		if (bTemp3rdPerson) {
			ForceTemp3rdPerson(1);
		}
	}
	ActiveEffect::FinishInitLoadGameActiveEffectList(pActiveEffectList, this);
	if (pGrabbedObject) {
		if (eGrabType != PlayerCharacter::GRAB_TELEKINESIS) {
			CreateMouseSpring(pGrabbedObject, eGrabType, fGrabDistance);
		}
	}
	DetectLifeEffect::CheckPlayerRunningDetectLife();
	NightEyeEffect::ApplyVisualEffect();
	SetLastKnownGoodPosition();
	SetFOV(fFOV);
	CharacterProgressionInfo.FinishInitLoadGame(aiFlags, aiOldFlags);
}

void PlayerCharacter::Revert(uint32_t aiFlags) {
	Character::Revert(aiFlags);
	SetNeedResetHardcoreTimer(1);
	fCheckMyDeadBodyTimer = 0.0f;
	bCanFastTravel = 0;
	SetCanFastTravel(1);
	fCheckMyDeadBodyTimer = 0.0f;
	if (pSaveLoadGameOLD->IsUnk048Zero()) {
		pPrisonMarker = 0;
		PlayerCharacter::pPrisonContainer = 0;
		StopVanityMode();
		SetInCharGen(0);
		ConflictingObjects.RemoveAll();
		ActiveEffect::RevertActiveEffectList(pActiveEffectList, this);
		if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
			if (p1stPersonAnimation) {
				p1stPersonAnimation->RevertTES(this);
			}
		}
		ResetLastPlayingTimeUpdate();
		DestroyMouseSpring();
		DroppedRefList.RemoveAll();
	}
	bAllowEGMCacheClear = 1;
}

void PlayerCharacter::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESTopic* ptopic;
	BSSimpleList<PerkRankData*>* pperkRankList;
	Character::Revert(apLoadGameBuffer);
	if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION)) {
		if (p1stPersonAnimation) {
			p1stPersonAnimation->Revert(apLoadGameBuffer);
		}
	}
	NiAVObject* v3 = Get3D(0);
	SetStoredWorldRecurse(v3, 0);
	if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_SCALE)) {
		SetScale(1.0f);
	}
	pPrisonMarker = 0;
	PlayerCharacter::pPrisonContainer = 0;
	StopVanityMode();
	bSpeaking = 0;
	fZoomDelta = 0.0f;
	fChaseDistance = 0.0f;
	fPlayerDeathReloadTime_0 = -1.0f;
	bDelayVATSMenu = 0;
	pCurrentWater = 0;
	bCanWait = 1;
	b3rdPerson = 0;
	bWant3rdPerson = 0;
	pCombatGroup = 0;
	iTeammateCount = 0;
	fCombatTimer = 0.0f;
	fYieldTimer = 0.0f;
	bAiControlledToPos = 0;
	bAiControlledFromPos = 0;
	bAiControlledActivate = 0;
	bAiControlledPackage = 0;
	bBlockActivate = 0;
	bAllowEGMCacheClear = 1;
	fBlockActivateTimer = 5.0f;
	if (apLoadGameBuffer->GetRevertOnly()) {
		bIsYoung = 0;
	}
	bIsToddler = 0;
	bCanUsePowerArmor = 0;
	bHostileDetection = 0;
	ucControlsDisabled = 0;
	bTemp3rdPerson = 0;
	bTemp3rdPersonSwitchBack = 0;
	bTemp1stPerson = 0;
	bTemp1stPersonSwitchBack = 0;
	bVATSForce1stPerson = 0;
	bCanUsePowerArmor = 0;
	fWorldFOV = BSSceneGraph::fDefaultWorldFOV.Float();
	f1stPersonFOV = BSSceneGraph::fDefault1stPersonFOV.Float();
	iNumberofStealWarnings = 0;
	fStealWarningTimer = 0.0f;
	iNumberofPickpocketWarnings = 0;
	fPickPocketWarningTimer = 0.0f;
	bSpeaking = 0;
	bMegatonDestroyed = 0;
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
	for (int32_t i_2 = 0; i_2 < 77; ++i_2) {
		TemporaryActorValueModifiers[i_2] = 0.0f;
		ScriptActorValueModifiers[i_2] = 0.0f;
		DamageActorValueModifiers[i_2] = 0.0f;
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
	pClosestConversation = 0;
	pAIConversationRunning = 0;
	pQueuedWeaponAttach = 0;
	RemovePlayerMapMarker();
	pLastExtDoorActivated = 0;
	SetInCharGen(0);
	while (this->pListofActions && this->pListofActions->m_item) {
		ourFree(this->pListofActions->m_item);
		this->pListofActions->RemoveHead();
	}
	void* pListofActions = (void*)this->pListofActions;
	if (pListofActions) {
		delete static_cast<BSSimpleList<ActiveEffect*>*>(pListofActions);
	}
	this->pListofActions = 0;
	for (pperkRankList = &Perks; pperkRankList; pperkRankList = pperkRankList->m_pkNext) {
		ourFree(pperkRankList->m_item);
	}
	Perks.RemoveAll();
	for (uint32_t i_3 = 0; i_3 < 74; ++i_3) {
		PerkEntryLists[i_3].RemoveAll();
	}
	for (pperkRankList = &CompanionPerks; pperkRankList; pperkRankList = pperkRankList->m_pkNext) {
		ourFree(pperkRankList->m_item);
	}
	CompanionPerks.RemoveAll();
	for (uint32_t i_4 = 0; i_4 < 74; ++i_4) {
		CompanionPerkEntryLists[i_4].RemoveAll();
	}
	ourFree(pQueuedTargetLoc);
	pQueuedTargetLoc = 0;
	bAlwaysHardcore = 1;
	pReputationUpdate = 0;
	SetWeaponSightingNode(0);
	ActiveEffect::RevertActiveEffectList(apLoadGameBuffer, pActiveEffectList);
	for (BSSimpleList<TESTopic*>* plist = pDataHandler->GetTopicList(); plist; plist = plist->m_pkNext) {
		ptopic = plist->m_item;
		if (ptopic) {
			if (ptopic->IsRandom()) {
				listTopics.AddHead(*reinterpret_cast<void**>(&ptopic));
			}
		}
	}
	BSSimpleList<TESTopic*>* TopicList = GetTopicList();
	TESTopic::SortTopics(1, TopicList);
	ClearAudioMarkerList();
	ClearMapMarkerStructList();
	ClearAutoAim();
	ClearPendingReferenceLists();
}

void PlayerCharacter::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	bool IsSpellTarget;
	Character::FinishLoadGame(apLoadGameBuffer);
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(16)) {
		float distance = fRefScale;
		SetScale(distance);
	}
	bWant3rdPerson = b3rdPerson;
	if (Get3D(1)) {
		if (bTemp3rdPerson) {
			ForceTemp3rdPerson(1);
		}
		Show1stPerson(!b3rdPerson);
	}
	if (fPlayerCameraAlpha < 1.0) {
		fPlayerCameraAlpha = 1.0f;
		UpdateAlpha();
	}
	UpdateArmRotation();
	Animation* Animation_0 = GetAnimation(1);
	Animation_0->UpdateSceneGraphNoController();
	UpdateCamera(1, 0);
	if (appMain->bFlyCamera) {
		UpdateFlyCamera(0.0f, 1);
	}
	if (pGrabbedObject) {
		if (eGrabType != PlayerCharacter::GRAB_TELEKINESIS) {
			CreateMouseSpring(pGrabbedObject, eGrabType, fGrabDistance);
			Interface::SetDragDroppingState(2);
		}
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
		BaseProcess* CurrentAIProcess = pCurrentProcess;
		CurrentAIProcess->SetIronSights(0);
		SetIronSights(1, 0, 0);
	}
	SpellItem* PipBoyLightSpell = MagicSystem::GetPipBoyLightSpell();
	if (PipBoyLightSpell) {
		IsSpellTarget = this->IsSpellTarget(PipBoyLightSpell, 1);
	} else {
		IsSpellTarget = this->IsSpellTarget(0, 1);
	}
	if (IsSpellTarget) {
		FOPipboyManager* Pipboy = Interface::GetPipboy();
		Pipboy->ShowPipboyLightEffect(1, 1, 1);
		FOPipboyManager* v5 = Interface::GetPipboy();
		v5->ShowPipboyLightEffect(0, 1, 1);
	}
	CheckTempModifiers();
}

void PlayerCharacter::UpdateRefLight() {
	LightRefData* plightData = (&m_Extra)->GetLight();
	if (plightData) {
		NiAVObject* v1;
		if (plightData->spLight == sp1stPersonLight && (v1 = Get3D(1), v1->GetAppCulled())) {
			plightData->spLight.operator=(sp3rdPersonLight);
		} else {
			if (plightData->spLight == sp3rdPersonLight) {
				NiAVObject* v2 = Get3D(0);
				if (v2->GetAppCulled()) {
					plightData->spLight.operator=(sp1stPersonLight);
				}
			}
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
	AutoMemContext kContext(MEM_CONTEXT_PLAYER, true,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 9304);
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
		} else if (pInventoryChanges) {
			ItemChange* pPipBoy = pInventoryChanges->GetWornItem(BO_PIP_BOY, false);
			if (pPipBoy) {
				delete pPipBoy;
			} else {
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
	NiNode* p1stPerson3D =
		pModelLoader->LoadFile(sFirstPersonSkeleton.String(), LOD_MULT_ACTORS, true, false, false, false);
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
	NiUpdateData kUpdateData;
	kUpdateData.~NiUpdateData();
	::new (&kUpdateData) NiUpdateData(0.0f, false, false);
	p1stPerson3D->Update(kUpdateData);
	fEyeHeight = p1stPersonCamera->GetWorldTranslate().z;
	NiMatrix3 kRotIn = NiMatrix3_IDENTITY;
	p1stPerson3D->SetRotate(MultipleMatrixByRace(kRotIn));
	void* pMemory = ourMalloc(sizeof(BipedAnim));
	p1stPersonBipedAnim = pMemory ? ::new (pMemory) BipedAnim(this, p1stPerson3D) : nullptr;
	pMemory = ourMalloc(sizeof(Animation));
	p1stPersonAnimation = pMemory ? ::new (pMemory) Animation : nullptr;
	// Actor::Load3D loads the third-person 3D.
	bool bWas3rdPerson = b3rdPerson;
	b3rdPerson = true;
	NiAVObject* p3rdPerson3D = Actor::Load3D(abBackgroundLoading);
	b3rdPerson = bWas3rdPerson;
	GetAnimation(false)->ClearGroup(AGS_IDLE, 0.0f);
	BSSimpleList<const char*>* pKFFileList =
		pModelLoader->BuildKFFileList(sFirstPersonSkeleton.String(), true, false, ANIM_GROUP_WEAPON_COUNT);
	char acIdlePath[260];
	BSstrcpy(acIdlePath, 260, sFirstPersonSkeleton.String());
	char* pFileName = std::strrchr(acIdlePath, '\\');
	if (IsPCToddler()) {
		BSstrcpy(pFileName, 260 - (pFileName - acIdlePath) - 1, "\\Locomotion\\Toddler\\IdleAnims");
		pModelLoader->CopyFilenameList(pIdleManager->GetRootFilenameList(acIdlePath, AGW_NONE), pKFFileList);
	}
	if (GetSex() == 1) {
		BSstrcpy(pFileName, 260 - (pFileName - acIdlePath) - 1, "\\Locomotion\\Female\\IdleAnims");
	} else {
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
			pModelLoader->ReleaseFile(NavMeshInfo::GetCellOrCoords(const_cast<char*>(sFirstPersonSkeleton.String()),
				0));
		}
	} else {
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
	double Seconds = appTimer.fDelta;
	float fsecondsPassed = pVATS.GetPlayerUpdateMult() * Seconds;
	Animation* panim = GetAnimation(0);
	if (panim) {
		panim->Update(this, fsecondsPassed, -1.0f);
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
	uint8_t v1 = (uint8_t)IsActually3rdPerson();
	return GetBiped((bool)((uint8_t)(!v1)));
}

// Moves the player into apCell: water height, multibound, Havok, decal casters and the camera's rigid body, gun wobble,
// climate and image space; then finds the regions the player stands in (sounds, the occupied region, border regions),
// checks the last known good position, shows the occupied region's map name in the HUD and toggles the imposters of the
// regions left and entered. Without a cell, only the region sounds are cleared.
void PlayerCharacter::SetParentCell(TESObjectCELL* apCell) {
	// The region name passed to Interface::SetRegionText, which destroys it.
	FNVD_INLINED_LOCAL(BSStringT<char>, kRegionName, std::nullopt, std::nullopt);
	if (apCell == GetParentCell()) {
		return;
	}
	if (!TESWaterSystem::GetCameraInPlaceableWater()) {
		float fWaterHeight;
		if (apCell && apCell->HasWater()) {
			fWaterHeight = apCell->GetWaterHeight();
		} else {
			fWaterHeight = -3.4028235e+38f;
		}
		TESWaterSystem::SetCameraRelevantWaterHeight(fWaterHeight);
	}
	m_Extra.SetMultiBoundRef(0);
	MobileObject::SetParentCell(apCell);
	if (!apCell) {
		CurrentRegionSoundList.RemoveAll();
		FalloutAudio::UpdateRegionSounds();
		return;
	}
	if (!apCell->GetInterior()) {
		SetOccupiedRegion(0);
	}
	FalloutRadio::NewCellEntered();
	pLastKnownMusicType = apCell->GetMusicType(0);
	bhkWorld* pCellWorld = apCell->GetbhkWorld();
	if (pCellWorld) {
		pCellWorld->pAcousticSpaceListener->bBufferRemovalOperations = 1;
	}
	MoveHavok(0);
	if (GetParentCell() && GetParentCell()->GetbhkWorld()) {
		if (DecalCaster::GetInstance(DecalCaster::GetMaxDecalWidthLarge(), 0)) {
			bhkWorld* pWorld = GetParentCell()->GetbhkWorld();
			DecalCaster::GetInstance(DecalCaster::GetMaxDecalWidthLarge(), 1)->Add(pWorld);
		}
	}
	if (GetParentCell() && GetParentCell()->GetbhkWorld()) {
		if (DecalCaster::GetInstance(DecalCaster::GetMaxDecalWidthSmall(), 0)) {
			bhkWorld* pWorld = GetParentCell()->GetbhkWorld();
			DecalCaster::GetInstance(DecalCaster::GetMaxDecalWidthSmall(), 1)->Add(pWorld);
		}
	}
	if (spCameraRigidBody.operator==(static_cast<bhkRigidBody*>(0))) {
		hkVector4 hkv;
		bhkSphereShape* pSphereMemory = static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
		bhkSphereShape* pSphere = pSphereMemory ? ::new (pSphereMemory) bhkSphereShape : nullptr;
		pSphere->Init(0.001f);
		bhkRigidBodyCinfo kInfo;
		kInfo.SetShape(pSphere);
		kInfo.SetCollisionFilter(0x30021u);
		bhkRigidBody* pBodyMemory = static_cast<bhkRigidBody*>(NiMemObject::operator new(sizeof(bhkRigidBody)));
		bhkRigidBody* pBody = pBodyMemory ? ::new (pBodyMemory) bhkRigidBody(kInfo) : nullptr;
		spCameraRigidBody.operator=(pBody);
		spCameraRigidBody.m_pObject->SetActive(1);
		bhkRigidBody* pCameraBody = spCameraRigidBody.m_pObject;
		pCameraBody->SetMotionType(hkpMotion::MOTION_KEYFRAMED);
		spCameraRigidBody.m_pObject->GetCollidableRw()->m_broadPhaseHandle.m_objectQualityType = 9;
		NiPoint3 kLocation = GetLocationOnReference();
		NI2HK(hkv, kLocation);
		spCameraRigidBody.m_pObject->SetPosition(kLocation);
	}
	bhkRigidBody* pCameraBody = spCameraRigidBody.m_pObject;
	pCameraBody->Add(GetParentCell()->GetbhkWorld());
	bool bIncapacitated = IsUnConscious() || IsDead(0) || IsDying() || IsKnockedOut();
	bool bAddWobble = !bIncapacitated && !IsSleeping();
	if (bAddWobble) {
		AddGunWobble();
	}
	pModelLoader->UpdateQueuedReferencePriorities(0);
	TESClimate* pClimate = apCell->GetClimate();
	Sky::GetInstance()->SetCurrentClimate(pClimate, 0);
	BSShaderManager::SetImageSpaceParameters(&apCell->GetUsableImageSpace()->Data);
	TESWorldSpace* pWorldSpace = apCell->GetWorldSpace();
	bool bInBorderRegion = false;
	bInBorderContainedCell = 0;
	if (pBorderRegions) {
		pBorderRegions->RemoveAll();
		NiTPrimitiveArray<TESRegion*>* pOldBorderRegions = pBorderRegions;
		if (pOldBorderRegions) {
			delete pOldBorderRegions;
		}
		pBorderRegions = 0;
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
	FalloutAudioMedia::pCurrentRegionSoundData = 0;
	BSSimpleList<TESRegion*> OldRegionList;
	BSSimpleList<TESRegion*>* pOccupied = nullptr;
	// The loop tests the address &m_AllOccupiedRegions, which is never null, as the game does.
	pOccupied = &static_cast<BSSimpleList<TESRegion*>&>(m_AllOccupiedRegions);
	for (BSSimpleList<TESRegion*>* pIter = pOccupied; pIter && pIter->m_item; pIter = pIter->m_pkNext) {
		TESRegion* pRegion = pIter->m_item;
		if (pRegion) {
			OldRegionList.AddHead(pRegion);
		}
	}
	m_AllOccupiedRegions.RemoveAll();
	TESRegionDataSound* pSoundOverrideRegion = 0;
	TESRegionList* pCellRegions = apCell->GetRegionList(1);
	for (BSSimpleList<TESRegion*>* pIter = static_cast<BSSimpleList<TESRegion*>*>(pCellRegions); pIter;
		pIter = pIter->m_pkNext) {
		TESRegion* pCurrentRegion = pIter->m_item;
		if (!pCurrentRegion) {
			break;
		}
		// The constructor from a world position on the reference's location.
		TESRegionPoint testPoint(data.kPosition);
		BSSimpleList<TESRegionSound*>* pRegSounds = 0;
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
			TESRegionDataSound* pRSD =
				static_cast<TESRegionDataSound*>(pCurrentRegion->pDataList->Find(REGION_DATA_SOUND_ID));
			if (pRSD) {
				if (pRSD->bOverride) {
					if (!pSoundOverrideRegion ||
						(pSoundOverrideRegion != pRSD && pSoundOverrideRegion->cPriority < pRSD->cPriority)) {
						CurrentRegionSoundList.RemoveAll();
						pSoundOverrideRegion = pRSD;
					}
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
				if (pRPL->IsInsideRegion(&sw) && pRPL->IsInsideRegion(&nw) && pRPL->IsInsideRegion(&se) &&
					pRPL->IsInsideRegion(&ne)) {
					bInBorderContainedCell = 1;
				}
			}
			if (pOccupiedRegion) {
				TESRegionDataWeather* pOccRWD =
					static_cast<TESRegionDataWeather*>(pOccupiedRegion->pDataList->Find(REGION_DATA_WEATHER_ID));
				TESRegionDataWeather* pCurRWD =
					static_cast<TESRegionDataWeather*>(pCurrentRegion->pDataList->Find(REGION_DATA_WEATHER_ID));
				if (pCurRWD) {
					if (!pOccRWD || (pCurRWD->bOverride && !pOccRWD->bOverride) ||
						(pCurRWD->bOverride == pOccRWD->bOverride && pCurRWD->cPriority > pOccRWD->cPriority)) {
						SetOccupiedRegion(pCurrentRegion);
					}
				}
			} else {
				SetOccupiedRegion(pCurrentRegion);
			}
			bFound = true;
		}
		if (pCurrentRegion->TESForm::GetHasSpokenFlag()) {
			if (!pBorderRegions) {
				NiTPrimitiveArray<TESRegion*>* pMemory =
					static_cast<NiTPrimitiveArray<TESRegion*>*>(ourMalloc(sizeof(NiTPrimitiveArray<TESRegion*>)));
				pBorderRegions = pMemory ? ::new (pMemory) NiTPrimitiveArray<TESRegion*>(1, 1) : nullptr;
			}
			pBorderRegions->Add(pCurrentRegion);
		}
	}
	if (gs_bBorderRegionsEnabled_General.Bool() && pWorldSpace && pWorldSpace->GetHasBorderRegion() &&
		!TES::pInstance->CellTestRunning() && !bInBorderRegion) {
		TESObjectCELL* pGoodCell = 0;
		TESWorldSpace* pGoodWorldSpace = 0;
		if (pLastKnownGoodLocation) {
			int32_t iFormType = pLastKnownGoodLocation->GetFormType();
			if (iFormType == CELL_ID) {
				pGoodCell = static_cast<TESObjectCELL*>(pLastKnownGoodLocation);
			} else if (iFormType == WRLD_ID) {
				pGoodWorldSpace = static_cast<TESWorldSpace*>(pLastKnownGoodLocation);
			}
		}
		if (pGoodCell && pGoodCell != apCell) {
			bReturnToLastKnownGoodPosition = 1;
		} else if (pGoodWorldSpace && pGoodWorldSpace != apCell->GetWorldSpace()) {
			bReturnToLastKnownGoodPosition = 1;
		} else {
			NiPoint3 kOffset = GetLocationOnReference();
			kOffset.operator-=(LastKnownGoodPosition);
			if (kOffset.Length() > 2048.0) {
				bReturnToLastKnownGoodPosition = 1;
			}
		}
	}
	TESRegionDataMap* pMapData = nullptr;
	if (pOccupiedRegion) {
		pMapData = static_cast<TESRegionDataMap*>(pOccupiedRegion->pDataList->Find(REGION_DATA_MAP_ID));
	}
	const char* pMapName = pMapData ? pMapData->GetMapName() : nullptr;
	::new (&kRegionName) BSStringT<char>(pMapName);
	Interface::SetRegionText(pOccupiedRegion, kRegionName, true);
	// Interface::SetRegionText takes its argument by value and receives a copy of this raw-storage local, so the local
	// is destroyed here.
	kRegionName.~BSStringT<char>();
	ResetAudioMarkerList();
	ResetMapMarkerStructList();
	// The regions left: their imposters are hidden.
	for (BSSimpleList<TESRegion*>* pIter = &OldRegionList; pIter && pIter->m_item; pIter = pIter->m_pkNext) {
		TESRegion* pRegion = pIter->m_item;
		if (pRegion) {
			TESRegionDataImposter* pImposterData =
				static_cast<TESRegionDataImposter*>(pRegion->pDataList->Find(REGION_DATA_IMPOSTER_ID));
			if (pImposterData) {
				pImposterData->SetImpostersVisible(0, 0);
			}
		}
	}
	// The regions the player is in: their imposters are added to the cell and shown at once.
	pOccupied = nullptr;
	// The loop tests the address &m_AllOccupiedRegions, which is never null, as the game does.
	pOccupied = &static_cast<BSSimpleList<TESRegion*>&>(m_AllOccupiedRegions);
	for (BSSimpleList<TESRegion*>* pIter = pOccupied; pIter && pIter->m_item; pIter = pIter->m_pkNext) {
		TESRegion* pRegion = pIter->m_item;
		if (!pRegion) {
			continue;
		}
		TESRegionDataImposter* pImposterData =
			static_cast<TESRegionDataImposter*>(pRegion->pDataList->Find(REGION_DATA_IMPOSTER_ID));
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
	} else {
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
	FNVD_INLINED_LOCAL(BSSoundHandle, v135, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v157, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v175, std::nullopt, std::nullopt);
	NiPoint3 v187;
	bool bsearch;
	NiPoint3 v230;
	alignas(16) NiMatrix3 currentMatrix;
	FNVD_INLINED_LOCAL(BSSoundHandle, v238, std::nullopt, std::nullopt);
	alignas(16) FNVD_INLINED_LOCAL(BSSoundHandle, v241, std::nullopt, std::nullopt);
	alignas(16) FNVD_INLINED_LOCAL(BSSoundHandle, v257, std::nullopt, std::nullopt);
	float v286;
	float v287;
	uint16_t smoveMode;
	TESObjectWEAP* pweapon;
	FNVD_INLINED_LOCAL(BSSoundHandle, v317, std::nullopt, std::nullopt);
	Controls* pcontrols = appMain->GetControls();
	time_followerStealthing = time_followerStealthing - afDelta;
	if (bVATSForce1stPerson) {
		if (bVATSReturnPlayerTo1stPerson) {
			pPlayer->SetFirstPerson(1);
		} else {
			pPlayer->SetFirstPerson(0);
		}
		bVATSForce1stPerson = 0;
	}
	if (pVATS.eMode) {
		BSAudio* v4 = &BSAudio::QInstance();
		if (v4->GetMasterSecondaryVolume() > 0.9900000095367432 &&
			fVATSCameraEffectVolume.Float() < 0.9900000095367432) {
			float fValue = fVATSCameraEffectVolume.Float();
			BSAudio* v5 = &BSAudio::QInstance();
			v5->SetMasterSecondaryVolume(fValue);
			if (!BSAudio::kVatsLoopHandle.IsValid()) {
				BSAudio* v6 = &BSAudio::QInstance();
				BSSoundHandle* SoundHandleByName = v6->GetSoundHandleByName(&v317, "UIVATSLOOP", 49u);
				BSAudio::kVatsLoopHandle.operator=(*SoundHandleByName);
			}
			BSAudio::kVatsLoopHandle.Play(0);
		}
	} else {
		BSAudio::QInstance();
		if (BSAudio::kVatsLoopHandle.IsValid()) {
			BSAudio::QInstance();
			BSAudio::kVatsLoopHandle.FadeOutAndRelease(1500u);
		}
		BSAudio* v2 = &BSAudio::QInstance();
		if (v2->GetMasterSecondaryVolume() < 0.9900000095367432) {
			BSAudio* v3 = &BSAudio::QInstance();
			v3->ClearSecondaryVolumes();
		}
	}
	if (this->fKillCamCooldown > 0.0) {
		this->fKillCamCooldown = this->fKillCamCooldown - afDelta;
	}
	char v314 = 0;
	if (!(this->fTimeInSlowMoCam > 0.0)) {
		bool v11 = this->Is3rdPerson();
		Animation* Animation_0 = this->GetAnimation(!v11);
		Animation_0->SetGlobalTimeMultiplier(1.0f);
		bool v13 = this->Is3rdPerson();
		Animation* v14 = this->GetAnimation(v13);
		v14->SetGlobalTimeMultiplier(1.0f);
	} else {
		if (pcontrols->GetUserAction(Controls::UACTN_ACTIVATE, 1)) {
			this->fTimeInSlowMoCam = 0.0f;
		}
		this->fTimeInSlowMoCam = this->fTimeInSlowMoCam - afDelta;
		appTimer.SetGlobalTimeMultiplier(0.3f, 0);
		if (this->bIgnoresGTM) {
			bool v7 = this->Is3rdPerson();
			Animation* v8 = this->GetAnimation(!v7);
			v8->SetGlobalTimeMultiplier(3.3333333f);
			bool v9 = this->Is3rdPerson();
			Animation* v10 = this->GetAnimation(v9);
			v10->SetGlobalTimeMultiplier(3.3333333f);
		}
		if (!(this->fTimeInSlowMoCam <= 0.0)) {
			v314 = 1;
		} else {
			this->ResetKillCam(this->eKillCameraSetting, 0);
		}
	}
	if (this->GetBlockCounterTimer() > 0.0) {
		float v313 = this->GetBlockCounterTimer() - afDelta;
		this->SetBlockCounterTimer(v313);
	}
	if (this->GetActorValue(ActorValue::TURBO) <= 0 || v314) {
		if (!v314) {
			TESImageSpaceModifier* apMod_2 =
				static_cast<TESImageSpaceModifier*>(TESForm::GetFormByEditorID("TurboDOF50"));
			ImageSpaceModifierInstanceForm::Stop(apMod_2);
			if (this->bTurboISM) {
				appTimer.RestoreGlobalTimeMultiplierCache();
			}
			this->bTurboISM = 0;
		}
	} else {
		if (!this->bTurboISM) {
			BSTimer::SaveGlobalTimeMultiplier();
			TESImageSpaceModifier* apMod =
				static_cast<TESImageSpaceModifier*>(TESForm::GetFormByEditorID("TurboDOF50"));
			ImageSpaceModifierInstanceForm::Trigger(apMod, 1.0f, 0);
			this->bTurboISM = 1;
		}
		float* v15 = &fTurboTimeMultiplier.Float();
		appTimer.SetGlobalTimeMultiplier(*v15, 0);
		float fPersonalMultiplier = 1.0 / BSTimer::QGlobalTimeMultiplier();
		bool v16 = this->Is3rdPerson();
		Animation* v17 = this->GetAnimation(!v16);
		v17->SetGlobalTimeMultiplier(fPersonalMultiplier);
		bool v18 = this->Is3rdPerson();
		Animation* v19 = this->GetAnimation(v18);
		v19->SetGlobalTimeMultiplier(fPersonalMultiplier);
	}
	if (this->pQueuedWeaponAttach) {
		if (!this->pCurrentProcess->Get3DUpdateFlag(RESET_MODEL)) {
			this->AttachWeapon(this->pQueuedWeaponAttach);
			this->pQueuedWeaponAttach = 0;
		}
	}
	if (this->bBlockActivate && !appMain->IsInMenuMode()) {
		this->fBlockActivateTimer = this->fBlockActivateTimer - afDelta;
		if (this->fBlockActivateTimer < 0.0) {
			this->fBlockActivateTimer = 0.0f;
			this->bBlockActivate = 0;
		}
	}
	iActorsDismemberedThisFrame = 0;
	TESObjectREFR* phitRef = TESObjectREFR::FindReferenceFor3D(this->spTargeted3D.m_pObject);
	HUDMainMenu::HandleHealthbarForCrosshairActor(phitRef, this->bTarget3DDistant);
	this->spTargeted3D.operator=(static_cast<NiAVObject*>(0));
	this->ResetCachedGunSkill();
	alignas(16) AutoMemContext tempMemContext(MEM_CONTEXT_PLAYER, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 3137);
	SceneGraph* pWorldRoot = Main::GetWorldSceneGraph();
	if (this->pCurrentProcess->GetCurrentWeapon()) {
		pweapon = static_cast<TESObjectWEAP*>(this->pCurrentProcess->GetCurrentWeapon()->pContainerObj);
	} else {
		pweapon = 0;
	}
	Animation* panim = this->GetAnimation(0);
	Animation* panim1st = this->GetAnimation(1);
	bool v302 = 0;
	smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
		(static_cast<uint32_t>(static_cast<uint16_t>(0))));
	BSSoundHandle hndl;
	this->fDropAngleMod = 0.0;
	this->fLastDropAngleMod = 0.0;
	if (!(this->fTimeronAction > 0.0)) {
		this->iActionValue = 0;
	} else {
		double Seconds = appTimer.fDelta;
		this->fTimeronAction = this->fTimeronAction - Seconds;
	}
	this->UpdateConflictingObjects();
	this->pCurrentProcess->Update3dModel(this);
	if (this->GetBumpedRef()) {
		TESObjectREFR* BumpedRef = this->GetBumpedRef();
		this->AddPlayerAction(PLAYER_ACTION_KNOCKING_OVER_OBJECTS, 2.0f, BumpedRef);
	}
	if (this->GetBumpedActor()) {
		if (this->fTimeronAction <= 0.0) {
			int32_t fValuea = iSoundLevelLoud.Int();
			this->SetActionSoundValue(fValuea);
		}
	}
	bool IronSights = this->GetIronSights();
	if (IronSights && this->pCurrentProcess->GetWeaponDrawn()) {
		TESObjectWEAP* pcurrentWeapon;
		bool v298 = 0;
		if (this->GetParentCell()->GetInterior()) {
			v298 = 1;
		} else {
			Sky* psky = TES::pInstance->pSky;
			float ftimeOfDay = psky->GetCurrentGameHour();
			float fsunriseTime = psky->GetSunriseEnd();
			float fsunsetTime = psky->GetSunsetBegin();
			if (fsunsetTime <= static_cast<double>(ftimeOfDay) || fsunriseTime >= static_cast<double>(ftimeOfDay)) {
				v298 = 1;
			}
		}
		if (pVATS.eMode || this->fTimeInSlowMoCam > 0.0) {
			v298 = 0;
		}
		ItemChange* pcurrentWeaponItem = this->pCurrentProcess->GetCurrentWeapon();
		if (pcurrentWeaponItem) {
			pcurrentWeapon = static_cast<TESObjectWEAP*>(pcurrentWeaponItem->pContainerObj);
		} else {
			pcurrentWeapon = 0;
		}
		if (pcurrentWeapon &&
			(pcurrentWeapon->HasNightScope() &&
				(!this->bNightVisionOn &&
					(v298 && (!pcurrentWeapon->HasModScope() || pcurrentWeaponItem->HasModEffectActive(14)))))) {
			this->bNightVisionOn = 1;
			TESImageSpaceModifier* apMod_3 = static_cast<TESImageSpaceModifier*>(TESForm::GetFormByNumericID(0x8CD8Cu));
			ImageSpaceModifierInstanceForm::Trigger(apMod_3, 1.0f, 0);
		} else {
			if (!v298) {
				TESImageSpaceModifier* FormByNumericID =
					static_cast<TESImageSpaceModifier*>(TESForm::GetFormByNumericID(0x8CD8Cu));
				ImageSpaceModifierInstanceForm::Stop(FormByNumericID);
				this->RemovePlayerAction(PLAYER_ACTION_IRON_SITES, 0);
				this->bNightVisionOn = 0;
			}
		}
		if (!this->GetPlayerAction(PLAYER_ACTION_IRON_SITES)) {
			this->AddPlayerAction(PLAYER_ACTION_IRON_SITES, 20.0f, 0);
		}
	} else {
		if (this->bNightVisionOn) {
			TESImageSpaceModifier* v288 = static_cast<TESImageSpaceModifier*>(TESForm::GetFormByNumericID(0x8CD8Cu));
			ImageSpaceModifierInstanceForm::Stop(v288);
			this->bNightVisionOn = 0;
		}
		this->RemovePlayerAction(PLAYER_ACTION_IRON_SITES, 0);
	}
	if (!IronSights || (this->bHasSpotterActive || pVATS.eMode == VATS::VATS_PLAYBACK)) {
		if (this->bHasSpotterActive && !IronSights) {
			TESEffectShader* v22 = pDataHandler->SpotterEffectItem();
			ReferenceProcessLists.FinishMagicShaderHitEffectAll(v22);
			this->bHasSpotterActive = 0;
		}
	} else {
		v287 = 0.0;
		BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_IMPROVED_SPOTTING, pPlayer, &v287);
		if (v287 > 0.0) {
			this->bHasSpotterActive = 1;
			this->SpotterEffectNearbyEnemies();
		}
	}
	if (this->GetActorValue(ActorValue::NIGHT_EYE_BONUS) <= 0) {
		if (this->bHasCateyeActive && this->GetActorValue(ActorValue::NIGHT_EYE_BONUS) <= 0) {
			ReferenceProcessLists.FinishMagicShaderHitEffectAll(pDataHandler->pCateyeMobileShader);
			this->bHasCateyeActive = 0;
		}
	} else {
		this->bHasCateyeActive = 1;
	}
	v286 = 0.0;
	BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_ITEM_DETECTION, pPlayer, &v286);
	this->fCheckForItems = this->fCheckForItems - afDelta;
	if (v286 > 0.0 && this->fCheckForItems <= 0.0 && IronSights) {
		float* v24 = &fItemDetectionTimer.Float();
		this->fCheckForItems = *v24;
		this->ShaderNearbyItems();
	}
	this->UpdatePlayerActions();
	if (this->IsHardcore()) {
		this->UpdateHardcoreMode();
	}
	BaseProcess* CurrentAIProcess = this->pCurrentProcess;
	CurrentAIProcess->CalculateLightValue(this, 0);
	if (!this->IsStandingOnFurniture() || this->GetSitSleepState()) {
		this->RemovePlayerAction(PLAYER_ACTION_STAND_ON_TABLE_CHAIR, 0);
	} else {
		if (!this->GetPlayerAction(PLAYER_ACTION_STAND_ON_TABLE_CHAIR)) {
			this->AddPlayerAction(PLAYER_ACTION_STAND_ON_TABLE_CHAIR, 20.0f, 0);
		}
	}
	bhkCharacterController* CharController = this->GetCharController();
	if (CharController && static_cast<uint8_t>(CharController->IsOnElevator())) {
		if (!this->bOnElevator) {
			ReferenceProcessLists.MoveFollowersOnToPlayer();
			this->bOnElevator = 1;
		}
	} else {
		this->bOnElevator = 0;
	}
	BaseProcess* v283 = this->pCurrentProcess;
	if (!(v283->GetDetectionModifierTimer() > 0.0)) {
		BaseProcess* v281 = this->pCurrentProcess;
		v281->SetDetectionModifier(0.0f);
	} else {
		BaseProcess* v282 = this->pCurrentProcess;
		v282->ModDetectionModifierTimer();
	}
	if (this->bBeingChased) {
		if (!(this->GetDetectionTimer() <= 0.0)) {
			this->ReduceDetectionTimer();
		} else {
			this->CheckGetAwayFromGuards();
			BaseProcess* v280 = this->pCurrentProcess;
			float* v25 = &fDetectionTimerSetting.Float();
			v280->SetDetectionTimer(*v25);
		}
	}
	if (!(this->fStealWarningTimer > 0.0)) {
		this->fStealWarningTimer = -1.0f;
	} else {
		this->fStealWarningTimer = this->fStealWarningTimer - afDelta;
	}
	if (!(this->fPickPocketWarningTimer > 0.0)) {
		this->fPickPocketWarningTimer = -1.0f;
	} else {
		this->fPickPocketWarningTimer = this->fPickPocketWarningTimer - afDelta;
	}
	if (!(this->fSortActorDistanceTimer <= 0.0)) {
		this->fSortActorDistanceTimer = this->fSortActorDistanceTimer - afDelta;
	} else {
		ReferenceProcessLists.SortActorsCloseToPlayer();
		float* v26 = &fSortActorDistanceListTimer.Float();
		this->fSortActorDistanceTimer = *v26;
	}
	if (this->bReturnToLastKnownGoodPosition) {
		this->bReturnToLastKnownGoodPosition = 0;
		this->ReturnToLastKnownGoodPosition(1);
	}
	if (this->GetForceActivateRef()) {
		TESObjectREFR* ForceActivateRef = this->GetForceActivateRef();
		ForceActivateRef->Activate(this, 0, 0, 1);
		this->SetForceActivateRef(0);
	} else {
		BaseProcess* v279 = this->pCurrentProcess;
		v279->RandomlyPlaySpecialIdles(this);
		int32_t v278 = this->pCurrentProcess->GetPostAnimationActions();
		this->pCurrentProcess->ClearPostAnimationActions();
		if ((v278 & 4) != 0 && !this->pCurrentProcess->SitOrSleep(this)) {
			this->pCurrentProcess->SetAnimActionSuccess(0);
		}
		if ((v278 & 8) != 0 && !this->pCurrentProcess->ProcessGetUp(this)) {
			this->pCurrentProcess->SetAnimActionSuccess(0);
		}
		uint16_t slastMoveMode = this->GetMoveMode();
		if (this->IsSneaking() && this->IsMoving()) {
			if (Interface::GetDetectionLevel() < 3) {
				if (this->bActorinSneakRange) {
					if (!(this->fsecondSneaking > 1.0)) {
						this->fsecondSneaking = this->fsecondSneaking + afDelta;
					} else {
						this->UseSkill(ActorValue::SNEAK, 0);
						this->fsecondSneaking = 0.0f;
					}
				}
			}
		} else {
			if (this->IsSwimming() && this->IsMoving()) {
				if (!(this->fsecondSwimming > 1.0)) {
					this->fsecondSwimming = this->fsecondSwimming + afDelta;
				} else {
					this->fsecondSwimming = 0.0f;
				}
			} else {
				if ((slastMoveMode & ActorMover::MF_RUNNING) != 0 && this->IsMoving()) {
					if (!(this->fsecondRunning > 1.0)) {
						this->fsecondRunning = this->fsecondRunning + afDelta;
					} else {
						this->fsecondRunning = 0.0f;
					}
				}
			}
		}
		smoveMode = static_cast<uint16_t>(slastMoveMode &
			(ActorMover::MF_SNEAKING | ActorMover::MF_SWIMMING | ActorMover::MF_FALL | ActorMover::MF_SLIDE));
		this->pPenetrationDetection->Update();
		if (this->UpdateHeadingAndLooking(afDelta, 0, &smoveMode, 0)) {
			v302 = 1;
		}
		if (((PlayerCharacter::bVanityMode || PlayerCharacter::bZoom3rdPersonCamera) &&
				this->pCurrentProcess->GetFadeState() == BaseProcess::FADE_NORMAL) ||
			!static_cast<HighProcess*>(this->pCurrentProcess)->HighProcess::FadeUpdate(this)) {
			BaseProcess* fCheckForSpotter = this->pCurrentProcess;
			if (fCheckForSpotter->IsSustainedFire() && !pVATS.IsInVats()) {
				if (this->pCurrentProcess->GetCurrentWeapon()) {
					ANIMATION_ACTION eaction = this->GetAnimAction();
					if (eaction == ANIM_ACTION_ATTACK ||
						(eaction == ANIM_ACTION_ATTACK_WAIT_FOR_EJECT ||
							(eaction == ANIM_ACTION_ATTACK_FOLLOW_THROUGH ||
								(eaction == ANIM_ACTION_ATTACK_THROW || eaction == ANIM_ACTION_ATTACK_THROW_ATTACH)))) {
						TESObjectWEAP* pweapon_3 =
							static_cast<TESObjectWEAP*>(this->pCurrentProcess->GetCurrentWeapon()->pContainerObj);
						if (pweapon_3) {
							bool bhasAmmo = 1;
							if (pweapon_3->GetCurrentAmmo(pPlayer)) {
								ItemChange* pammo = this->pCurrentProcess->GetCurrentAmmo();
								if (pammo) {
									int32_t FormHealth_0 = pammo->iNumber;
									bhasAmmo = FormHealth_0 >= pweapon_3->GetAmmoPerShot();
								} else {
									bhasAmmo = 0;
								}
							}
						}
					}
				}
			}
			if (this->pCurrentProcess->GetKnockState() || this->IsParalyzed()) {
				pVATS.QuitVATSPlayback(0, 0);
				this->bAiControlledActivate = 0;
				pCurrentWater = 0;
				if (!this->b3rdPerson) {
					this->ForceTemp3rdPerson(1);
				}
			}
			if (this->GetUnderAIControl() || this->IsDead(0)) {
				if (this->bActually3rdPerson &&
					(!this->b3rdPerson && this->GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH))) {
					this->Show1stPerson(1);
				}
				this->pActorMover->ForceMoveMode(static_cast<uint16_t>(smoveMode));
				if (!(fAiControlTimer > 15.0) ||
					(this->GetSitSleepState() && this->GetSitSleepState() != SIT_SLEEP_IS_SITTING)) {
					if (this->GetInSleepPackage()) {
						fAiControlTimer = fAiControlTimer + afDelta;
						if (this->pCurrentProcess->IsPackageDone(this) ||
							(this->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
								(this->pCurrentProcess->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
									!this->pCurrentProcess->GetPackageThatIsRunning()))) {
							this->SetAIControlledToPosition(0);
							this->pCurrentProcess->ClearRunOncePackage();
							fAiControlTimer = 0.0f;
						}
					} else {
						if (this->GetAIControlledFromPosition()) {
							fAiControlTimer = fAiControlTimer + afDelta;
							if (this->GetSitSleepState() == SIT_SLEEP_NORMAL) {
								this->SetAIControlledFromPosition(0);
								this->pCurrentProcess->ClearRunOncePackage();
								fAiControlTimer = 0.0f;
							}
						}
					}
					if (this->bWant3rdPerson == this->b3rdPerson) {
						int32_t fValueb = this->b3rdPerson;
						this->Actor::Process(afDelta);
					}
					static_cast<PlayerMover*>(this->pActorMover)->PlayerMover::SetSideVector(NiPoint3_ZERO);
					this->pActorMover->Update(afDelta);
					if (!(fPlayerDeathReloadTime.Float() > 0.0)) {
						if (this->GetLifeState() == ACTOR_LIFE_STATE_DEAD && !PlayerCharacter::bShowDeathMessage) {
							this->DetachCharController();
							Interface::SetGunScopeVisible(0);
							if (pSaveLoadManager->HasMostRecentSaveGame()) {
								const char* afMult = sMiscPlayerDeadMenuOption.String();
								const char* v109 = sMiscPlayerDeadLoadOption.String();
								Interface::CreateMessageMenu(sMiscPlayerDeadMessage.String(),
									static_cast<const char*>(0), static_cast<const char*>(0),
									&PlayerCharacter::DeathMenu, 1, 23, 0.0f, 0.0f, v109, afMult, nullptr);
							} else {
								StartMenu::ChooseMainMenu();
							}
							PlayerCharacter::bShowDeathMessage = 1;
						}
					} else {
						if (this->GetLifeState() == ACTOR_LIFE_STATE_DEAD ||
							this->GetLifeState() == ACTOR_LIFE_STATE_DYING) {
							if (fPlayerDeathReloadTime_0 < 0.0) {
								Interface::SetGunScopeVisible(0);
								fPlayerDeathReloadTime_0 = fPlayerDeathReloadTime.Float();
							}
							fPlayerDeathReloadTime_0 = fPlayerDeathReloadTime_0 - afDelta;
							if (fPlayerDeathReloadTime_0 < 0.0) {
								fPlayerDeathReloadTime_0 = -1.0f;
								if (!pSaveLoadManager->LoadMostRecentSaveGame()) {
									StartMenu::ChooseMainMenu();
								}
							}
						} else {
							fPlayerDeathReloadTime_0 = -1.0f;
						}
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
					Animation* v31 = this->GetAnimation(!this->b3rdPerson);
					this->UpdateAnimationMovement(v31, afDelta);
					this->b3rdPerson = !this->b3rdPerson;
					this->Character::Update(afDelta);
					Animation* v32 = this->GetAnimation(!this->b3rdPerson);
					this->UpdateAnimationMovement(v32, afDelta);
					this->UpdateCamera(0, 0);
					this->GetParentCell()->TestObjectAgainstMultibounds(this, 1, 0);
					NiNode* afMulta = this->Get3D();
					ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
					ShadowSceneNode_->UpdateObjectLighting(afMulta, 1);
					BGSMusicType* MusicType = this->GetParentCell()->GetMusicType(0);
					this->pLastKnownMusicType = MusicType;
					if (this->bAiControlledActivate) {
						if (pCurrentWater) {
							if ((fangryFace_guard & 1) == 0) {
								fangryFace_guard |= 1u;
								PlayerCharacter::fDrinkDelay = fDrinkRepeatRate.Float();
							}
							if ((pcontrols->GetUserAction(Controls::UACTN_ACTIVATE, 1) ||
									pcontrols->GetUserAction(Controls::UACTN_ACTIVATE, 0)) &&
								!this->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
								Animation* v35 = this->GetAnimation();
								v35->SetLoopCount(AGS_SPECIAL_IDLE, 1);
								PlayerCharacter::fDrinkDelay = PlayerCharacter::fDrinkDelay - afDelta;
								while (PlayerCharacter::fDrinkDelay < 0.0) {
									PlayerCharacter::fDrinkDelay =
										PlayerCharacter::fDrinkDelay + fDrinkRepeatRate.Float();
									if (pCurrentWater) {
										pCurrentWater->Activate(0, this, 0, 0, 0);
									}
								}
							} else {
								alignas(16) BSSoundHandle actLoopHandle;
								(&pPlayer->m_Extra)->GetActivateLoopSound(actLoopHandle);
								actLoopHandle.Stop();
								Animation* v36 = this->GetAnimation();
								v36->SetLoopCount(AGS_SPECIAL_IDLE, 0);
								PlayerCharacter::fDrinkDelay = fDrinkRepeatRate.Float();
								pCurrentWater = 0;
							}
						}
						Animation* v37 = this->GetAnimation();
						if (v37->SpecialIdleDonePlaying() &&
							this->pCurrentProcess->GetAnimAction() == ANIM_ACTION_NONE &&
							this->pCurrentProcess->GetKnockState() == KNOCK_STATE_NORMAL) {
							this->bAiControlledActivate = 0;
							pCurrentWater = 0;
						}
					}
					if (pcontrols->GetUserAction(Controls::UACTN_QUICKLOAD, 1)) {
						if (pSaveLoadManager->Quickload()) {
							fPlayerDeathReloadTime_0 = -1.0f;
						}
					}
					if (!this->IsDead(0) && !this->IsDying()) {
						this->UpdateMagic(0, 0);
						if (pcontrols->GetUserAction(Controls::UACTN_ACTIVATE, 1)) {
							if (!this->IsPipboyActive() && !Interface::IsInMenuMode() && !this->IsParalyzed() &&
								!this->GetTempPerson()) {
								Actor* pactor_2 = static_cast<Actor*>(Interface::GetActivateREFR());
								char v269 = 1;
								bool v268 = 0;
								if (pactor_2) {
									if (pactor_2->IsFurniture()) {
										v268 = IronSights;
									}
								}
								if (this->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
									v268 = 1;
								}
								if (pactor_2) {
									if (pactor_2->IsProjectile()) {
										ArrowProjectile* parrow = dynamic_cast<ArrowProjectile*>(pactor_2);
										if (parrow) {
											if (parrow->eState == ArrowProjectile::STATE_FLYING) {
												v268 = 1;
											}
										}
									}
								}
								if (pactor_2) {
									if (pactor_2->IsActor()) {
										BaseProcess* pprocess = pactor_2->pCurrentProcess;
										if (pprocess->GetFadeState() == BaseProcess::FADE_OUT_DISABLE ||
											pprocess->GetFadeState() == BaseProcess::FADE_OUT_DELETE) {
											v268 = 1;
										}
									}
								}
								int32_t v264 = this->GetSitSleepState() - 1;
								switch (v264) {
								case 0:
								case 1:
								case 2:
								case 4:
								case 5:
								case 6:
								case 7:
								case 9: {
									v268 = 1;
									break;
								}
								default: {
									break;
								}
								}
								ANIMATION_ACTION AnimAction = this->GetAnimAction();
								if (AnimAction != ANIM_ACTION_NONE) {
									v268 = 1;
								}
								if (this->bWant3rdPerson != this->b3rdPerson) {
									v268 = 1;
								}
								if (this->GetBlockActivate()) {
									v268 = 1;
								}
								bool v262 = 0;
								if (!v268) {
									Interface::SetInfoForRef(0, 0, 0);
									if (pactor_2) {
										BSSimpleList<BGSEntryPointFunctionDataActivateChoice*> ChoiceList;
										BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ACTIVATE, this, pactor_2,
											&ChoiceList, pactor_2);
										if (ChoiceList.IsEmpty()) {
											int32_t icount_2 = pactor_2->GetCount();
											if (pactor_2->Activate(this, 0, 0, icount_2)) {
												v269 = 0;
											}
										} else {
											if (!MessageMenu::AddMessageToQueue(&ChoiceList, pactor_2)) {
												int32_t icount = pactor_2->GetCount();
												if (pactor_2->Activate(this, 0, 0, icount)) {
													v269 = 0;
												}
											}
										}
									} else {
										v262 = 1;
									}
									if (v269 && this->pCurrentProcess->GetCurrentFurniture()) {
										TESObjectREFR* v38 = this->pCurrentProcess->GetCurrentFurniture();
										v262 = !v38->Activate(this, 0, 0, 1);
									} else {
										if (v269) {
											v262 = 1;
										}
									}
								}
								if (v262 || v268) {
									(&this->m_Extra)->GetCreatureAwakeSound(hndl);
									TESSound* pnoactSound = FalloutAudio::GetNoActivationSound();
									if (!hndl.IsValid()) {
										if (pnoactSound) {
											const char* Filename = pnoactSound->GetFilename();
											BSAudio* v39 = &BSAudio::QInstance();
											BSSoundHandle* SoundHandleByFilename = v39->GetSoundHandleByFilename(&v257,
												Filename,
												BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
													BSGameSound::SOUND_FLAG_ONE_SHOT,
												pnoactSound);
											hndl.operator=(*SoundHandleByFilename);
										}
									}
									if (!hndl.IsPlaying()) {
										hndl.Play(0);
									}
									(&this->m_Extra)->SetCreatureAwakeSound(hndl);
								}
								v302 = 1;
							}
						}
					}
				} else {
					if (this->GetAIControlledFromPosition() && this->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
						this->SetAIControlledFromPosition(0);
						this->pCurrentProcess->ClearRunOncePackage();
					} else {
						if (this->IsDead(0) || this->GetSitSleepState() == SIT_SLEEP_NORMAL) {
							this->pCurrentProcess->ClearRunOncePackage();
							this->SetAIControlledToPosition(0);
							fAiControlTimer = 0.0f;
						}
					}
					this->GetOutofFurnitureQuick();
					fAiControlTimer = 0.0f;
				}
			} else {
				TESObjectCELL* ParentCell;
				TESBoundObject* FormFlags;
				NiPoint3 forwardVec__motionVec;
				if (pPlayer->pCurrentProcess->GetCurrentPackage()) {
					pPlayer->pCurrentProcess->ClearCurrentPackage();
				}
				if (pPlayer->pCurrentProcess->GetRunOncePackage()) {
					pPlayer->pCurrentProcess->ClearRunOncePackage();
				}
				this->ModPlayerGreetTimer();
				this->CheckPlayerGreetFlag();
				bool v255 = 0;
				this->fAmmoSwapButtonTimer = this->fAmmoSwapButtonTimer + afDelta;
				if (pcontrols->GetUserAction(Controls::UACTN_HOTKEY2, 1)) {
					if (this->pCurrentProcess->GetCurrentWeapon()) {
						if (this->IsWeaponDrawn()) {
							int32_t iupState = InterfaceManager::CheckMenuButton(Interface::eButtonDPadUp, 0);
							int32_t ileftState = InterfaceManager::CheckMenuButton(Interface::eButtonDPadLeft, 0);
							int32_t irightState = InterfaceManager::CheckMenuButton(Interface::eButtonDPadRight, 0);
							int32_t v251 = iupState == 2 || !iupState;
							bool v250 = v251;
							int32_t v249 = ileftState == 2 || !ileftState;
							bool v248 = v249;
							int32_t v247 = irightState == 2 || !irightState;
							bool v246 = v247;
							if (!v250 || (!v248 && !v246)) {
								float* v40 = &fPlayerAmmoSwapTimer.Float();
								bool bStartReload = *v40 < static_cast<double>(this->fAmmoSwapButtonTimer);
								this->AmmoSwapHelper(bStartReload, 0);
								if (bStartReload) {
									this->fAmmoSwapButtonTimer = 0.0f;
								}
							}
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
										this->SetWantWeaponDrawn(1);
									}
									g_bWasReadyItemHeldLastFrame = 1;
								}
							} else {
								v255 = 1;
								this->SetWantWeaponDrawn(0);
								g_bWasReadyItemHeldLastFrame = 1;
							}
						}
					}
				} else {
					if (!pcontrols->GetUserAction(Controls::UACTN_READY_RELOAD_WEAPON, 2)) {
						fReloadTimer = 0.0f;
						g_bWasReadyItemHeldLastFrame = 0;
					}
				}
				this->UpdateMenuModeButton(afDelta);
				if (pcontrols->GetUserAction(Controls::UACTN_TOGGLERUN, 1)) {
					this->bAlwaysRun = !this->bAlwaysRun;
				}
				if (pcontrols->GetUserAction(Controls::UACTN_AUTOMOVE, 1)) {
					this->bAutoMove = !this->bAutoMove;
				}
				if (this->bAutoMove && (!Interface::IsPipBoyOpening() && !Interface::IsInMenuMode())) {
					if (pcontrols->GetUserAction(Controls::UACTN_FORWARD, 0) ||
						(pcontrols->GetUserAction(Controls::UACTN_BACK, 0) ||
							(pcontrols->GetUserAction(Controls::UACTN_SLDRIGHT, 0) ||
								pcontrols->GetUserAction(Controls::UACTN_SLDLEFT, 0)))) {
						this->bAutoMove = 0;
					} else {
						pcontrols->ForceUserAction(Controls::UACTN_FORWARD);
					}
				}
				bool v243 = 0;
				if (pcontrols->GetUserAction(Controls::UACTN_CROUCH, 1)) {
					if (!this->IsParalyzed() && !this->IsDead(0) && !this->IsKnockedOut() && !this->IsRestrained() &&
						!this->IsUnConscious() && !this->GetControlsDisabled(PlayerCharacter::DPC_SNEAKING) &&
						this->GetSitSleepState() == SIT_SLEEP_NORMAL) {
						int32_t v242 = this->GetAnimAction() + 1;
						switch (v242) {
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
								smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
									(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode & 0xFBFF)))));
								BSAudio* v41 = &BSAudio::QInstance();
								BSSoundHandle* v240 = v41->GetSoundHandleByName(&v241, "NPCHumanCrouchUp",
									BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
										BSGameSound::SOUND_FLAG_FIRST_PERSON);
								hndl.operator=(*v240);
								const NiPoint3* v42 = &this->GetLocationOnReference();
								hndl.SetPosition(*v42);
								hndl.Play(0);
								this->UseActionPoints(ActionPoints::ACTION_STAND);
							} else {
								smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
									(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 1024)))));
								BSAudio* v43 = &BSAudio::QInstance();
								BSSoundHandle* v237 = v43->GetSoundHandleByName(&v238, "NPCHumanCrouchDown",
									BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
										BSGameSound::SOUND_FLAG_FIRST_PERSON);
								hndl.operator=(*v237);
								const NiPoint3* v44 = &this->GetLocationOnReference();
								hndl.SetPosition(*v44);
								hndl.Play(0);
								this->UseActionPoints(ActionPoints::ACTION_CROUCH);
							}
							if (!this->bTemp1stPerson) {
								this->SetIronSights(0, 0, 0);
								this->SetBlock(0);
							}
							break;
						}
						default: {
							break;
						}
						}
					}
					v243 = 1;
				}
				currentMatrix = (*&pWorldRoot->GetCameraRoot()->m_kLocal).m_Rotate;
				NiPoint3 currentPos = *this->QLocationOnReference();
				NiPoint3 oldPos = currentPos;
				alignas(16) NiPoint3 sideVec = *::new (&v230) NiPoint3(1.0f, 0.0f, 0.0f);
				currentMatrix.GetCol(1u, forwardVec__motionVec);
				if (this->GetNoCollision()) {
					currentMatrix.GetCol(0, sideVec);
				} else {
					forwardVec__motionVec.z = 0.0f;
					forwardVec__motionVec.Unitize();
				}
				bool bdropping = 0;
				if (this->eGrabType == PlayerCharacter::GRAB_INVENTORY_DROP) {
					bdropping = this->UpdateDropSpring(afDelta);
				}
				this->HavokActivateDroppedReference();
				int32_t ControllerValue = 0;
				int32_t v227 = 0;
				int32_t v226 = 0;
				if (InterfaceManager::Has360Controller()) {
					ControllerValue = pcontrols->GetControllerValue(0, 7);
					v226 = pcontrols->GetControllerValue(0, 9);
					v227 = pcontrols->GetControllerValue(0, 8);
					if (std::abs(ControllerValue) < 0x1EA9) {
						ControllerValue = 0;
					}
					if (std::abs(v227) < 0x1EA9) {
						v227 = 0;
					}
					if (std::abs(v226) < 0x21F1) {
						v226 = 0;
					}
					ControllerValue = 100 * ControllerValue / 0x7FFF;
					v227 = 100 * v227 / 0x7FFF;
					v226 /= 0x7FFF;
				}
				if (!InterfaceManager::Has360Controller()) {
					if (ControllerValue) {
						if (ControllerValue <= 0) {
							pcontrols->ForceUserAction(Controls::UACTN_SLDLEFT);
						} else {
							pcontrols->ForceUserAction(Controls::UACTN_SLDRIGHT);
						}
						int32_t v225 = std::abs(ControllerValue);
						double v224 = static_cast<double>(v225);
						PlayerCharacter::fPCTurnScale = fPlayerCharacterTurnMult.Float() * v224;
						if (PlayerCharacter::fPCTurnScale > 1.5) {
							PlayerCharacter::fPCTurnScale = 1.5f;
						}
					}
					int32_t v45 = std::abs(v227);
					if (v45 >= std::abs(ControllerValue)) {
						if (v227) {
							if (v227 >= 0) {
								pcontrols->ForceUserAction(Controls::UACTN_BACK);
							} else {
								pcontrols->ForceUserAction(Controls::UACTN_FORWARD);
							}
							int32_t v222 = std::abs(v227);
							PlayerCharacter::fWalkSpeedMult = static_cast<double>(v222) * 0.019999999552965164;
						}
					} else {
						int32_t v223 = std::abs(ControllerValue);
						PlayerCharacter::fWalkSpeedMult = static_cast<double>(v223) * 0.019999999552965164;
						if (ControllerValue >= 0) {
							smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
								(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 8)))));
						} else {
							smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
								(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 4)))));
						}
					}
				}
				ItemChange* pcurrentWeaponItem_2 = this->pCurrentProcess->GetCurrentWeapon();
				if (pcurrentWeaponItem_2) {
					FormFlags = pcurrentWeaponItem_2->pContainerObj;
				} else {
					FormFlags = 0;
				}
				if (PlayerCharacter::fWalkSpeedMult > 1.0) {
					PlayerCharacter::fWalkSpeedMult = 1.0f;
				}
				if (InterfaceManager::Has360Controller()) {
					double v206;
					float v209;
					NiPoint3 motionVec;
					NiPoint3 kMoveVec;
					float fTurnInput;
					float fForwardInput;
					float v217 = static_cast<double>(ControllerValue) / 100.0;
					fForwardInput = static_cast<double>(v227) / 100.0;
					fTurnInput = static_cast<double>(v226) / 100.0;
					if (this->GetAnimAction() == ANIM_ACTION_DODGE) {
						v217 = 0.0f;
						fForwardInput = 0.0f;
						fTurnInput = 0.0f;
					}
					double v215 = std::fabs(v217);
					float* v46 = &fForwardBias.Float();
					float v214 = *v46 * fForwardInput;
					int32_t v213 = std::fabs(v214) > v215;
					bool v212 = v213;
					if (!v213) {
						if (!(v217 < 0.0)) {
							if (v217 > 0.0) {
								smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
									(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 8)))));
							}
						} else {
							smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
								(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 4)))));
						}
					}
					if (v212) {
						if (!(fForwardInput < 0.0)) {
							if (fForwardInput > 0.0) {
								smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
									(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 1)))));
							}
						} else {
							smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
								(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 2)))));
						}
					}
					kMoveVec.x = v217;
					kMoveVec.y = fForwardInput;
					kMoveVec.z = 0.0f;
					PlayerCharacter::fWalkSpeedMult = 0.0f;
					char v211 = 0;
					motionVec.x = v217;
					motionVec.y = fForwardInput;
					motionVec.z = 0.0f;
					if (!(motionVec.Length() > 1.0)) {
						v209 = motionVec.Length();
					} else {
						v209 = 1.0f;
					}
					if (PlayerCharacter::fWalkSpeedMult < 1.0) {
						if (!(v209 > 0.949999988079071)) {
							if (!(v209 > 0.800000011920929)) {
								PlayerCharacter::fWalkSpeedMult = FLerp(0.1f, 1.8f, 0.0f, 0.8f, v209);
							} else {
								PlayerCharacter::fWalkSpeedMult = FLerp(0.7f, 1.0f, 0.8f, 1.0f, v209);
								v211 = 1;
							}
						} else {
							PlayerCharacter::fWalkSpeedMult = 1.0f;
							v211 = 1;
						}
					}
					if (kMoveVec.Length() > 0.0) {
						if (pcontrols->GetUserAction(Controls::UACTN_RUN, 1) ||
							(pcontrols->GetUserAction(Controls::UACTN_RUN, 0) || v211)) {
							float* v47;
							if (this->pCurrentProcess->GetAnimAction() == ANIM_ACTION_BLOCK ||
								(this->IsOverEncumbered() ||
									(this->GetIronSights() ||
										(v47 = &fGrabMaxWeightRunning.Float(),
											!(*v47 >= static_cast<double>(this->fGrabObjectWeight)))))) {
								PlayerCharacter::fWalkSpeedMult = gs_fMaxAnalogWalkSpeed_Controls.Float();
								smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
									(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 256)))));
							} else {
								smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
									(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 512)))));
							}
						} else {
							smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
								(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 256)))));
						}
					}
					if (PlayerCharacter::fWalkSpeedMult > 1.0 && this->IsOverEncumbered()) {
						PlayerCharacter::fWalkSpeedMult = 1.0f;
					}
					if (PlayerCharacter::fWalkSpeedMult > 1.0 && this->GetIronSights()) {
						PlayerCharacter::fWalkSpeedMult = 1.0f;
					}
					double v207 = std::fabs(fTurnInput);
					if (!(std::fabs(v217) < v207) ||
						(v206 = std::fabs(fTurnInput), !(std::fabs(fForwardInput) < v206))) {
						PlayerCharacter::fPCTurnScale = std::fabs(v217) * 1.5;
					} else {
						PlayerCharacter::fPCTurnScale = std::fabs(fTurnInput) * 1.5;
					}
					float v205 = this->GetCurrentSpeed() * PlayerCharacter::fWalkSpeedMult;
					kMoveVec *= v205;
					if (this->GetNoCollision()) {
						NiMatrix3 rot;
						float fValuec = -this->GetLooking();
						rot.MakeXRotation(fValuec);
						kMoveVec = kMoveVec * rot;
					}
					kMoveVec *= 4.25f;
					currentPos.operator+=(kMoveVec);
					sideVec = kMoveVec;
				} else {
					NiPoint2 kStick;
					PlayerCharacter::fWalkSpeedMult = 1.0f;
					smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
						(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 64)))));
					kStick.x = static_cast<float>(ControllerValue);
					kStick.y = static_cast<float>(v227);
					if (kStick.Length() > 90.0) {
						pcontrols->ForceUserAction(Controls::UACTN_RUN);
						if (this->pCurrentProcess->GetAnimAction() != ANIM_ACTION_BLOCK && !this->IsOverEncumbered() &&
							!this->GetIronSights()) {
							smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
								(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 512)))));
						}
					}
					if (InterfaceManager::Has360Controller() || !this->bAlwaysRun) {
						if ((pcontrols->GetUserAction(Controls::UACTN_RUN, 1) ||
								pcontrols->GetUserAction(Controls::UACTN_RUN, 0)) &&
							this->pCurrentProcess->GetAnimAction() != ANIM_ACTION_BLOCK && !this->IsOverEncumbered() &&
							!this->GetIronSights()) {
							smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
								(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 512)))));
						}
					} else {
						if (!pcontrols->GetUserAction(Controls::UACTN_RUN, 1) &&
							!pcontrols->GetUserAction(Controls::UACTN_RUN, 0) &&
							this->pCurrentProcess->GetAnimAction() != ANIM_ACTION_BLOCK && !this->IsOverEncumbered() &&
							!this->GetIronSights()) {
							pcontrols->ForceUserAction(Controls::UACTN_RUN);
							smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
								(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 512)))));
						}
					}
					float* v48 = &fGrabMaxWeightRunning.Float();
					if (*v48 < static_cast<double>(this->fGrabObjectWeight)) {
						smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
							(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode & 0xFDFF)))));
					}
					float CurrentSpeed = this->GetCurrentSpeed();
					float v200 = CurrentSpeed * PlayerCharacter::fWalkSpeedMult;
					sideVec.operator*=(v200);
					float v199 = CurrentSpeed * PlayerCharacter::fWalkSpeedMult;
					forwardVec__motionVec.operator*=(v199);
					if (pcontrols->GetUserAction(Controls::UACTN_FORWARD, 1) ||
						pcontrols->GetUserAction(Controls::UACTN_FORWARD, 0)) {
						currentPos.operator+=(forwardVec__motionVec);
						if ((smoveMode & 512) == 0) {
							smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
								(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 256)))));
						}
						smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
							(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 1)))));
					}
					if (pcontrols->GetUserAction(Controls::UACTN_BACK, 1) ||
						pcontrols->GetUserAction(Controls::UACTN_BACK, 0)) {
						currentPos.operator-=(forwardVec__motionVec);
						if ((smoveMode & 512) == 0) {
							smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
								(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 256)))));
						}
						smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
							(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 2)))));
					}
					if (pcontrols->GetUserAction(Controls::UACTN_SLDLEFT, 1) ||
						pcontrols->GetUserAction(Controls::UACTN_SLDLEFT, 0)) {
						currentPos.operator-=(sideVec);
						if ((smoveMode & 512) == 0) {
							smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
								(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 256)))));
						}
						smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
							(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 4)))));
					}
					if (pcontrols->GetUserAction(Controls::UACTN_SLDRIGHT, 1) ||
						pcontrols->GetUserAction(Controls::UACTN_SLDRIGHT, 0)) {
						currentPos.operator+=(sideVec);
						if ((smoveMode & 512) == 0) {
							smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
								(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 256)))));
						}
						smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
							(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode | 8)))));
					}
				}
				if (pVATS.eMode == VATS::VATS_PLAYBACK) {
					smoveMode = static_cast<int32_t>((static_cast<uint32_t>(smoveMode) & 0xFFFF0000u) |
						(static_cast<uint32_t>(static_cast<uint16_t>((smoveMode & 0xFFC0)))));
				}
				this->pActorMover->ForceMoveMode(static_cast<uint16_t>(smoveMode));
				if (v243) {
					this->SetWalkRunFlagDirty();
				}
				this->pActorMover->UpdateTurnTimer(afDelta);
				this->UpdateTemp3rdPerson();
				this->UpdateTemp1stPerson();
				this->bsave3rdPerson = this->b3rdPerson;
				this->b3rdPersonSaved = 1;
				this->b3rdPerson = 1;
				if ((!pcontrols->GetUserAction(Controls::UACTN_USE, 0) ||
						(!bUserDebounceBow ||
							(!this->pCurrentProcess->GetCurrentAmmo() || !this->GetWantWeaponDrawn()))) &&
					this->pCurrentProcess->GetAutomaticShotDelay() != 0.0 &&
					(pVATS.eMode != VATS::VATS_PLAYBACK ||
						(!pVATS.GetCurrentAction() || !pVATS.GetCurrentAction()->cFireShots))) {
					this->pCurrentProcess->ClearAutomaticShotsToFire(0);
				}
				char v198 = 0;
				if (this->IsKnockedOut() ||
					(this->IsParalyzed() ||
						(this->GetLifeState() ||
							(this->GetAnimAction() == ANIM_ACTION_STAGGER ||
								this->pCurrentProcess->GetSitSleepState())))) {
					v198 = 1;
				}
				if (v198 || this->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
					static_cast<PlayerMover*>(this->pActorMover)->PlayerMover::SetMoveMode(831u, 0);
				}
				if (!v198 && !this->GetControlsDisabled(PlayerCharacter::DPC_FIGHTING)) {
					if (this->eGrabType == PlayerCharacter::GRAB_INVENTORY_DROP ||
						this->eGrabType == PlayerCharacter::GRAB_TELEKINESIS) {
						inextAttack = 0;
						fpowerTimer = 0.0f;
					} else {
						if (pcontrols->GetUserAction(Controls::UACTN_BLOCK, 1) ||
							pcontrols->GetUserAction(Controls::UACTN_BLOCK, 0)) {
							if (this->pCurrentProcess->GetAnimAction() != ANIM_ACTION_BLOCK && !this->GetIronSights() &&
								this->bWant3rdPerson == this->bActually3rdPerson && !this->IsPipboyActive() &&
								this->pCurrentProcess->CanAttack()) {
								if ((!pweapon || (pweapon->IsMeleeWeapon() && this->IsWeaponDrawn())) &&
									(pweapon || this->IsWeaponDrawn())) {
									this->SetBlock(1);
								} else {
									this->SetIronSights(1, 0, 0);
								}
								if (!PlayerCharacter::bVanityMode || PlayerCharacter::bIsAutoVanityMode) {
									v302 = 1;
								}
							}
						} else {
							if ((pcontrols->GetUserAction(Controls::UACTN_BLOCK, 2) ||
									(!pcontrols->GetUserAction(Controls::UACTN_BLOCK, 0) || this->IsPipboyActive())) &&
								(this->GetIronSights() ||
									this->pCurrentProcess->GetAnimAction() == ANIM_ACTION_BLOCK)) {
								this->SetIronSights(0, 0, 0);
								this->SetBlock(0);
								v302 = 1;
							}
						}
						if (!this->pCurrentProcess->CanAttack() || (smoveMode & 2048) != 0) {
							inextAttack = 0;
							fpowerTimer = 0.0f;
						} else {
							if (this->CheckUserInputAttacks()) {
								v302 = 1;
							}
						}
					}
				}
				if (!v198 && !this->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
					if (pcontrols->GetUserAction(Controls::UACTN_JUMP, 1)) {
						if (!this->IsOverEncumbered()) {
							Animation* v49 = this->GetAnimation();
							uint16_t Group = v49->GetGroup(AGS_WEAPON);
							if (!TESAnimGroup::IsPowerAttackAction(Group) &&
								this->GetAnimAction() != ANIM_ACTION_DODGE) {
								Animation* v197 = this->GetAnimation();
								if (this->GetAnimAction() == ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM) {
									if (v197->GetCurrentSequence(AGS_MOVEMENT)) {
										BSAnimGroupSequence* CurrentSequence = v197->GetCurrentSequence(AGS_MOVEMENT);
										TESAnimGroup* AnimGroup = CurrentSequence->GetAnimGroup();
										if (TESAnimGroupIsJumpingLandAnim(AnimGroup, nullptr)) {
											this->SetAnimAction(ANIM_ACTION_NONE, 0);
										}
									}
								}
								if (this->GetAnimAction() != ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM) {
									int32_t v196 = !this->IsJumping() && !this->IsSwimming();
									bool v194 = pcontrols->GetUserAction(Controls::UACTN_BLOCK, 0) != 0;
									bool bblocking = this->GetBlocked();
									if (v194) {
										if (Interface::GetActivateREFR()) {
											if (!bblocking) {
												bhkCharacterController* v54 = this->GetCharController();
												if (static_cast<uint8_t>(v54->CanJump())) {
													this->Jump();
												}
											}
										} else {
											bhkCharacterController* v53 = this->GetCharController();
											if (v53->GetState() == HK_CHARACTER_ON_GROUND) {
												this->Dodge(smoveMode);
											}
										}
									} else {
										bhkCharacterController* v55 = this->GetCharController();
										if (static_cast<uint8_t>(v55->CanJump())) {
											this->Jump();
										}
									}
									this->ForcePhysicsGrabDrop();
									v302 = 1;
								}
							}
						}
					}
				}
				if (!this->IsTresPassing()) {
					if ((&this->m_Extra)->GetTrespassPackage()) {
						(&this->m_Extra)->RemoveTrespassPackage();
					}
				}
				bsearch = 0;
				if (pcontrols->GetUserAction(Controls::UACTN_RESTMENU, 1)) {
					if (!this->GetPCControlFlags()) {
						if (this->IsTresPassing()) {
							const char* v56 = sNoWaitTrespass.String();
							Interface::ShowMessage(v56, 0,
								"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0);
						} else {
							if (this->IsAlarmForPlayerCharactaer()) {
								const char* v57 = sNoWaitWhileAlarmSounding.String();
								Interface::ShowMessage(v57, 0,
									"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0);
							} else {
								TESObjectCELL* afMultb = this->GetParentCell();
								const NiPoint3* v58 = &this->GetLocationOnReference();
								if (this->IsPointDeepUnderWater(*v58, afMultb, 0.875f)) {
									const char* v59 = sNoWaitUnderWater.String();
									Interface::ShowMessage(v59, 0,
										"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0);
								} else {
									bool Interior = pPlayer->GetInterior();
									if (ReferenceProcessLists.AreHostilActorsNear(Interior)) {
										const char* v61 = sNoWaitHostilActorsNear.String();
										Interface::ShowMessage(v61, 0,
											"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f,
											0);
									} else {
										bhkCharacterController* v63;
										bhkCharacterController* v62 = pPlayer->GetCharController();
										if (v62->GetState() == HK_CHARACTER_JUMPING ||
											(v63 = pPlayer->GetCharController(),
												v63->GetState() == HK_CHARACTER_IN_AIR)) {
											const char* v64 = sNoWaitInAir.String();
											Interface::ShowMessage(v64, 0,
												"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0,
												2.0f, 0);
										} else {
											if (!pPlayer->GetParentCell()->GetCantWaitHere() && pPlayer->GetCanWait()) {
												BaseProcess* v191;
												if (ReferenceProcessLists.IsPlayerInRadiationArea() ||
													(this->pCurrentProcess &&
														(v191 = this->pCurrentProcess,
															v191->GetRadiationWaterDelta() != 0.0))) {
													const char* v66 = sNoWaitInRadiation.String();
													Interface::ShowMessage(v66, 0,
														"Interface\\Icons\\Message "
														"Icons\\glow_message_vaultboy_sad.dds",
														0, 2.0f, 0);
												} else {
													if (pPlayer->HasDamageHealthEffect()) {
														const char* v67 = sNoWaitTakingHealthDamage.String();
														Interface::ShowMessage(v67, 0,
															"Interface\\Icons\\Message "
															"Icons\\glow_message_vaultboy_sad.dds",
															0, 2.0f, 0);
													} else {
														Interface::CreateSleepMenu(0);
													}
												}
											} else {
												const char* v65 = sNoWaitInCell.String();
												Interface::ShowMessage(v65, 0,
													"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0,
													2.0f, 0);
											}
										}
									}
								}
							}
						}
					}
				}
				this->b3rdPerson = 1;
				this->UpdateAnimation();
				this->PickAnimations(PlayerCharacter::fWalkSpeedMult, PlayerCharacter::fPCTurnScale);
				this->pCurrentProcess->UpdateKnockState(this);
				this->b3rdPerson = this->bsave3rdPerson;
				this->b3rdPersonSaved = 0;
				if (this->IsSleeping()) {
					this->PlayerSleep();
				}
				if (TES::QCollisionDisableFlag() || bDisablePlayerCollision.Bool()) {
					if (!this->GetNoCollision()) {
						bhkCharacterController* pcon = this->GetCharController();
						if (pcon) {
							pcon->SetPosition(currentPos);
							const hkVector4* Zero = &hkVector4::getZero();
							pcon->SetLinearVelocity(*Zero);
						}
						this->SetNoCollision(1);
					}
					alignas(16) NiPoint3 delta = currentPos - oldPos;
					delta.operator*=(afDelta);
					NiPoint3* v69 = ::new (&v187) NiPoint3(0.0f, 0.0f, delta.z);
					this->SetPosition(oldPos + *v69);
				} else {
					this->SetNoCollision(0);
				}
				static_cast<PlayerMover*>(this->pActorMover)->PlayerMover::SetSideVector(sideVec);
				this->pActorMover->Update(afDelta);
				this->CheckBorderRegion();
				if ((smoveMode & 15) != 0) {
					v302 = 1;
				}
				if (pcontrols->GetUserAction(Controls::UACTN_QUICKSAVE, 1)) {
					pSaveLoadManager->Quicksave();
				}
				if (pcontrols->GetUserAction(Controls::UACTN_QUICKLOAD, 1)) {
					pSaveLoadManager->Quickload();
				}
				bool v185 = pcontrols->GetUserAction(Controls::UACTN_VATSMENU, 1) > 0;
				if (bDelayVATSMenu ||
					(v185 &&
						(!this->GetControlsDisabled(PlayerCharacter::DPC_FIGHTING) &&
							(this->GetSitSleepState() == SIT_SLEEP_NORMAL &&
								(pVATS.eMode != VATS::VATS_PLAYBACK &&
									(!this->IsPipboyActive() &&
										(!Interface::IsInPipboyMenu() &&
											(!pVATS.eMode && (this->GetMoveMode() & 2048) == 0)))))))) {
					if (VATSMenu::BuildTargetList()) {
						double GlobalTimeMultiplier;
						if (v185) {
							bVATSReturnPlayerTo1stPerson = !pPlayer->Is3rdPerson();
						}
						char v184 = 0;
						ANIMATION_ACTION v183 = this->GetAnimAction();
						if (v183 == ANIM_ACTION_NONE || v183 == ANIM_ACTION_BLOCK) {
							ANIM_GROUP_ENUM Type;
							TESAnimGroup* v180;
							BSAnimGroupSequence* v182 = panim->GetCurrentSequence(AGS_WEAPON);
							BSAnimGroupSequence* v181 = panim->GetCurrentSequence(AGS_WEAPON);
							if (v182) {
								v180 = v182->GetAnimGroup();
							} else {
								v180 = 0;
							}
							if (v180) {
								Type = v180->GetType();
							} else {
								Type = ANIM_GROUP_NONE;
							}
							int32_t v177 = Type;
							if (Type >= ANIM_GROUP_RELOAD_W && v177 < 199) {
								if (v182) {
									panim->SkipNextBlend();
									panim->BlendOut(AGS_WEAPON, 0);
									if (v181) {
										panim1st->SkipNextBlend();
										panim1st->BlendOut(AGS_WEAPON, 0);
									}
								}
							} else {
								v184 = 1;
							}
						}
						if (!v184 ||
							(this->bTemp1stPerson ||
								((GlobalTimeMultiplier = BSTimer::QGlobalTimeMultiplier(),
									 GlobalTimeMultiplier != BSTimer::GetGlobalTimeMultiplierTarget()) ||
									(this->bTemp3rdPerson ||
										(!this->IsWeaponDrawn() ||
											(this->IsVanityMode() ||
												(this->b3rdPerson || this->bWant3rdPerson != this->b3rdPerson))))))) {
							bDelayVATSMenu = 1;
							this->SetWantWeaponDrawn(1);
							if (this->IsVanityMode() || this->b3rdPerson) {
								this->StopVanityMode();
								this->Show1stPerson(1);
								this->bWant3rdPerson = 0;
								this->b3rdPerson = 0;
							}
						} else {
							TESImageSpaceModifier* Hit = TESImageSpaceModifier::GetGetHit();
							ImageSpaceModifierInstanceForm::Stop(Hit);
							appMain->OnIdle_UpdateImageSpace(1);
							Interface::CreateVATSMenu();
							bDelayVATSMenu = 0;
						}
					} else {
						BSAudio* v72 = &BSAudio::QInstance();
						BSSoundHandle* v174 = v72->GetSoundHandleByName(&v175, "UIVATSEnterFail", 289u);
						v174->Play(0);
						bDelayVATSMenu = 0;
					}
					v302 = 1;
				}
				if (!this->IsPipboyActive() && !pCurrentWater && !this->bTemp1stPerson &&
					(!this->bTemp3rdPerson || PlayerCharacter::bVanityMode) && !bDelayVATSMenu) {
					if ((pcontrols->GetUserAction(Controls::UACTN_TOGGLEPOV, 1) ||
							pcontrols->GetUserAction(Controls::UACTN_TOGGLEPOV, 0)) &&
						!this->GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
						if (!Interface::IsInMenuMode() && bFOVTargetReached) {
							v302 = 1;
							PlayerCharacter::bZoom3rdPersonCamera = 1;
							if (!PlayerCharacter::bVanityMode) {
								bWas3rdPersonBeforeVanityMode = this->bActually3rdPerson;
								this->SetFirstPerson(0);
								PlayerCharacter::fAutoVanityDelay = 0.0f;
								PlayerCharacter::bIsAutoVanityMode = 0;
								PlayerCharacter::bVanityMode = 1;
								HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_VANITY);
								PlayerCharacter::kVanityModePos.z = 0.0f;
								PlayerCharacter::fStartingCameraY = PlayerCharacter::kVanityModePos.y;
							}
							PlayerCharacter::fVanityDelay = PlayerCharacter::fVanityDelay + afDelta;
						}
					} else {
						if (pcontrols->GetUserAction(Controls::UACTN_TOGGLEPOV, 2) ||
							(PlayerCharacter::bVanityMode && !PlayerCharacter::bIsAutoVanityMode)) {
							PlayerCharacter::fAutoVanityDelay = 0.0f;
							PlayerCharacter::bIsAutoVanityMode = 0;
							PlayerCharacter::bVanityMode = 0;
							HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_CHANGED);
							if (fVanityModeDelay.Float() > static_cast<double>(PlayerCharacter::fVanityDelay) &&
								(InterfaceManager::Has360Controller() ||
									PlayerCharacter::kVanityModePos.y == PlayerCharacter::fStartingCameraY) &&
								!this->GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
								if (bWas3rdPersonBeforeVanityMode) {
									this->SetFirstPerson(!this->bTemp3rdPersonSwitchBack);
								}
								this->bTemp3rdPerson = 0;
								this->bTemp3rdPersonSwitchBack = 0;
							}
							PlayerCharacter::fVanityDelay = 0.0f;
							v302 = 1;
							PlayerCharacter::bZoom3rdPersonCamera = 0;
							PlayerCharacter::kVanityModePos.x = 0.0f;
							PlayerCharacter::kVanityModePos.z = 0.0f;
						}
					}
				}
				bool v172 = 0;
				if (this->pCurrentProcess->GetCurrentFurniture()) {
					v172 = (smoveMode & 15) != 0;
				}
				if ((pcontrols->GetUserAction(Controls::UACTN_ACTIVATE, 1) || v172) && !bDelayVATSMenu &&
					!this->IsPipboyActive() && !Interface::IsInMenuMode() && !this->IsParalyzed() && !bdropping &&
					!this->GetTempPerson()) {
					Actor* pactor_3 = static_cast<Actor*>(Interface::GetActivateREFR());
					bool v170 = 1;
					bool bDontActivate = 0;
					if (pactor_3) {
						if (pactor_3->IsFurniture()) {
							bDontActivate = IronSights;
						}
					}
					if (this->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
						bDontActivate = 1;
					}
					if (pactor_3) {
						if (pactor_3->IsProjectile()) {
							ArrowProjectile* parrow_2 = dynamic_cast<ArrowProjectile*>(pactor_3);
							if (parrow_2) {
								if (parrow_2->eState == ArrowProjectile::STATE_FLYING) {
									bDontActivate = 1;
								}
							}
						}
					}
					if (pactor_3) {
						if (pactor_3->IsActor()) {
							BaseProcess* pprocess_2 = pactor_3->pCurrentProcess;
							if (!pactor_3->IsDead(0)) {
								const NiPoint3* v74;
								TESObjectCELL* afMultd;
								TESObjectCELL* afMultc = pactor_3->GetParentCell();
								const NiPoint3* v73 = &pactor_3->GetLocationOnReference();
								if (pactor_3->IsPointDeepUnderWater(*v73, afMultc, 0.875f) ||
									((afMultd = this->GetParentCell(),
										 (v74 = &pPlayer->GetLocationOnReference(),
											 pPlayer->IsPointDeepUnderWater(*v74, afMultd, 0.875f))) ||
										((pprocess_2 && pprocess_2->GetFadeState() == BaseProcess::FADE_OUT_DISABLE) ||
											(pprocess_2 &&
												pprocess_2->GetFadeState() == BaseProcess::FADE_OUT_DELETE)))) {
									bDontActivate = 1;
								}
							}
						}
					}
					int32_t v165 = this->GetSitSleepState() - 1;
					switch (v165) {
					case 0:
					case 1:
					case 2:
					case 4:
					case 5:
					case 6:
					case 7:
					case 9: {
						bDontActivate = 1;
						break;
					}
					default: {
						break;
					}
					}
					ANIMATION_ACTION v164 = this->GetAnimAction();
					if (v164 != ANIM_ACTION_NONE) {
						bDontActivate = 1;
					}
					if (this->bWant3rdPerson != this->b3rdPerson) {
						bDontActivate = 1;
					}
					if (this->GetBlockActivate()) {
						bDontActivate = 1;
					}
					bool v163 = 0;
					if (!bDontActivate) {
						if (!pactor_3 || !pactor_3->IsActor()) {
							Interface::SetInfoForRef(0, 0, 0);
						}
						if (!pactor_3 || v172) {
							v163 = 1;
						} else {
							alignas(16) BSSimpleList<BGSEntryPointFunctionDataActivateChoice*> ChoiceList_2;
							BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ACTIVATE, this, pactor_3, &ChoiceList_2,
								pactor_3);
							if (ChoiceList_2.IsEmpty()) {
								int32_t icount_4 = pactor_3->GetCount();
								if (pactor_3->Activate(this, 0, 0, icount_4)) {
									v170 = 0;
								} else {
									if (pactor_3->GetLock()) {
										TESObjectREFR* pref_2 = (TESObjectREFR*)pactor_3;
										LockData* Lock = pactor_3->GetLock();
										if (Lock->GetLockLevel(pref_2) != 5 && !pactor_3->GetInaccessible()) {
											v170 = 0;
										}
									}
								}
							} else {
								if (!MessageMenu::AddMessageToQueue(&ChoiceList_2, pactor_3)) {
									int32_t icount_3 = pactor_3->GetCount();
									if (pactor_3->Activate(this, 0, 0, icount_3)) {
										v170 = 0;
									}
								}
							}
						}
						if (v170 && this->pCurrentProcess->GetCurrentFurniture()) {
							TESObjectREFR* v76 = this->pCurrentProcess->GetCurrentFurniture();
							v163 = !v76->Activate(this, 0, 0, 1);
						} else {
							if (v170) {
								v163 = 1;
							}
						}
					}
					if (v163 || bDontActivate) {
						BSSoundHandle hndl_2;
						(&this->m_Extra)->GetCreatureAwakeSound(hndl_2);
						TESSound* pnoactSound_3 = FalloutAudio::GetNoActivationSound();
						if (!hndl_2.IsValid()) {
							if (pnoactSound_3) {
								const char* v111 = pnoactSound_3->GetFilename();
								BSAudio* v77 = &BSAudio::QInstance();
								BSSoundHandle* v156 = v77->GetSoundHandleByFilename(&v157, v111,
									BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
										BSGameSound::SOUND_FLAG_ONE_SHOT,
									pnoactSound_3);
								hndl_2.operator=(*v156);
							}
						}
						if (!hndl_2.IsPlaying()) {
							hndl_2.Play(0);
						}
						(&this->m_Extra)->SetCreatureAwakeSound(hndl_2);
					}
					v302 = 1;
				}
				if (PlayerCharacter::bIsAutoVanityMode) {
					if (v302 || this->GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
						this->StopVanityMode();
					} else {
						PlayerCharacter::kAutoVanityModePos.z = PlayerCharacter::kAutoVanityModePos.z -
							fVanityModeAutoXSpeed.Float() * 0.01745329238474369 * afDelta;
						fzSin = fVanityModeAutoYSpeed.Float() * 0.01745329238474369 * afDelta + fzSin;
						double v154 = std::sin(fzSin);
						PlayerCharacter::kAutoVanityModePos.x =
							fVanityModeAutoYDegrees.Float() * v154 * 0.01745329238474369;
						if (fzSin > 6.2831854820251465) {
							fzSin = fzSin - 6.2831854820251465;
						}
					}
				} else {
					if (!PlayerCharacter::bVanityMode) {
						if (v302) {
							PlayerCharacter::fAutoVanityDelay = 0.0f;
						} else {
							PlayerCharacter::fAutoVanityDelay = PlayerCharacter::fAutoVanityDelay + afDelta;
						}
						if (bDisableAutoVanityMode.operator!() &&
							fVanityModeAutoDelay.Float() < static_cast<double>(PlayerCharacter::fAutoVanityDelay) &&
							!this->GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
							PlayerCharacter::fAutoVanityModeDistRevert = PlayerCharacter::kVanityModePos.y;
							PlayerCharacter::kAutoVanityModePos.z = 0.0f;
							if (this->b3rdPerson) {
								PlayerCharacter::kAutoVanityModePos.x = this->GetLooking();
							} else {
								PlayerCharacter::kAutoVanityModePos.x = 0.0f;
							}
							this->ForceTemp3rdPerson(1);
							PlayerCharacter::bIsAutoVanityMode = 1;
							PlayerCharacter::bVanityMode = 1;
							HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_VANITY);
							PlayerCharacter::bSnapChaseCamera = 1;
						}
					}
				}
				this->HandlePhysicsGrab(afDelta);
				if (pPlayerSphere) {
					uint32_t v78 = count_11E0BB4++;
					if (v78 < 20) {
						NiPoint3 np;
						np.z = currentPos.z + 128.0;
						np.y = currentPos.y + 64.0;
						np.x = currentPos.x + 64.0;
						pPlayerSphere->SetPosition(np);
						pPlayerSphere->SetLinearVelocity(NiPoint3_ZERO);
					}
				}
				if (!this->IsDead(0) && !this->IsDying()) {
					this->UpdateMagic(0, 0);
				}
				float fValued = panim->GetMoveSpeed();
				panim1st->SetMoveSpeed(fValued);
				float fValuee = panim->m_fAttackSpeed;
				panim1st->SetAttackSpeed(fValuee);
				if (!Interface::InDialog()) {
					this->DoIronSightsZoom(afDelta);
				}
				this->b3rdPerson = !this->b3rdPerson;
				this->Character::Update(afDelta);
				Animation* v79 = this->GetAnimation(!this->b3rdPerson);
				this->UpdateAnimationMovement(v79, afDelta);
				this->b3rdPerson = !this->b3rdPerson;
				this->Character::Update(afDelta);
				Animation* v80 = this->GetAnimation(!this->b3rdPerson);
				this->UpdateAnimationMovement(v80, afDelta);
				this->HandlePostAnimationActions();
				if (!bVATSForce1stPerson) {
					this->UpdateCamera(0, 0);
				}
				this->GetParentCell()->TestObjectAgainstMultibounds(this, 1, 0);
				NiNode* afMulte = this->Get3D();
				ShadowSceneNode* v81 = BSShaderManager::GetShadowSceneNode(0);
				v81->UpdateObjectLighting(afMulte, 1);
				if (this->pCurrentProcess->GetCurrentMuzzleFlash()) {
					MuzzleFlash* v82 = this->pCurrentProcess->GetCurrentMuzzleFlash();
					v82->Update(afDelta, this);
				}
				if (this->IsDead(0) &&
					(pVATS.eMode != VATS::VATS_PLAYBACK ||
						(!pVATS.GetCurrentAction() ||
							pVATS.GetCurrentAction()->eAction != ActionPoints::ACTION_PLAYER_VATS_DEATH))) {
					pVATS.InitPlayerDeath();
				}
				TESObjectCELL::UpdateSeenDataForPlayer();
				if (!PlayerCharacter::pLastCell ||
					(ParentCell = this->GetParentCell(), PlayerCharacter::pLastCell != ParentCell)) {
					PlayerCharacter::pLastCell = this->GetParentCell();
					if (pModelLoader->IsFileQueuedForCell(PlayerCharacter::pLastCell)) {
						TES::pInstance->ShowLoadingMessage("Loading Area (Player)...");
						pModelLoader->UpdateQueuedReferencePriorities(PlayerCharacter::pLastCell);
						pIOManager->LoadQueuedPriority(IO_TASK_PRIORITY_CRITICAL);
					}
				}
				bsearch = 0;
				CharacterProgression* CharacterProgressionInfo = this->GetCharacterProgressionInfo();
				if (CharacterProgressionInfo->GetReadyToLevelUp()) {
					if (!this->IsPlayerCharacterInCombat(&bsearch) && !pVATS.IsInVats() &&
						HUDMainMenu::IsXPMeterFinishedMoving() && InterfaceManager::IsPipboyClosed() &&
						!InterfaceManager::GetContainerMenuQueued() && !this->GetInChargen()) {
						CharacterProgression* v85 = this->GetCharacterProgressionInfo();
						v85->BeginLevelUp();
					}
				}
				if (this->pReputationUpdate) {
					if (!this->IsPlayerCharacterInCombat(&bsearch) && !pVATS.IsInVats() && !Interface::InDialog() &&
						!StartMenu::PauseMenuIsActive()) {
						this->pReputationUpdate->DisplayReputationTitleChange();
						this->pReputationUpdate = 0;
					}
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
				} else {
					if (fAngryFace > 0.0) {
						fAngryFace = 0.0f;
						BSFaceGenAnimationData* pfaceAnim_2 = this->GetFaceAnimationData();
						if (pfaceAnim_2) {
							pfaceAnim_2->ResetCurrentMorphs(0.5f, 1, 0, 0, 0, 0);
						}
					}
				}
				if (v255) {
					float* v86 = &fCombatYieldTime.Float();
					if (*v86 > static_cast<double>(this->fCombatTimer) && this->fYieldTimer <= 0.0) {
						if (this->AttemptYield()) {
							float* v87 = &fCombatYieldRetryTime.Float();
							this->fYieldTimer = *v87;
						}
					}
				}
				if (this->fYieldTimer > 0.0) {
					this->fYieldTimer = this->fYieldTimer - afDelta;
					double v88 = BSMAX(this->fYieldTimer, 0.0f);
					this->fYieldTimer = v88;
				}
				if (this->pCurrentProcess->GetCurrentWeapon()) {
					TESObjectWEAP* pWeap =
						static_cast<TESObjectWEAP*>(this->pCurrentProcess->GetCurrentWeapon()->pContainerObj);
					ItemChange* v89 = this->pCurrentProcess->GetCurrentWeapon();
					bool HasModEffectActive_0 = v89->HasModEffectActive(6);
					if (pWeap->GetAmmoRegenRate(HasModEffectActive_0) > 0.0) {
						double Height = this->fTimeSinceLastAmmoRegenTick;
						float v148 = Height - appTimer.fDelta;
						this->SetTimeSinceLastAmmoRegenTick(v148);
						if (this->pCurrentProcess->GetCurrentAmmo() && this->fTimeSinceLastAmmoRegenTick <= 0.0) {
							ItemChange* v91 = this->pCurrentProcess->GetCurrentWeapon();
							bool v92 = v91->HasModEffectActive(6);
							float fValueg = pWeap->GetAmmoRegenRate(v92);
							this->SetTimeSinceLastAmmoRegenTick(fValueg);
							ItemChange* v93 = this->pCurrentProcess->GetCurrentAmmo();
							int32_t iCount = v93->iNumber;
							ItemChange* v94 = this->pCurrentProcess->GetCurrentWeapon();
							bool v95 = v94->HasModEffectActive(2);
							if (iCount < pWeap->GetFormClipRounds(v95)) {
								this->pCurrentProcess->GetCurrentAmmo()->iNumber = ++iCount;
							}
						} else {
							if (this->fTimeSinceLastAmmoRegenTick <= 0.0) {
								TESContainer tempcont;
								TESAmmo* CurrentAmmo = pweapon->GetCurrentAmmo(pPlayer);
								tempcont.AddObject(CurrentAmmo, 1, 0);
								tempcont.AddContainerToReference(pPlayer, 1);
								TESAmmo* v97 = pweapon->GetCurrentAmmo(pPlayer);
								pPlayer->EquipObject(v97, 1, 0, 1, 0, 0);
							}
						}
					}
				}
				if (this->IsMoving()) {
					TESObjectCELL* pCell = this->GetParentCell();
					if (pCell) {
						BSSimpleList<TESObjectREFR*>* pList = pCell->GetRefList();
						if (time_11E0BA8 < BSGetTickCount()) {
							while (pList && pList->m_item) {
								if (pList->m_item->GetFormType() == REFR_ID) {
									TESObjectSTAT* pstat =
										dynamic_cast<TESObjectSTAT*>(pList->m_item->GetObjectReference());
									if (pstat) {
										if (pstat->cPassThroughSoundIndex != -1) {
											TESObjectREFR* m_item = pList->m_item;
											if (m_item->Get3D()) {
												TESObjectREFR* v141 = pList->m_item;
												NiNode* v98 = v141->Get3D();
												float v140 = v98->GetWorldBoundRadius();
												v140 = v140 * v140;
												TESObjectREFR* v139 = pList->m_item;
												double v138 = v140;
												const NiPoint3* fValueh = &this->GetLocationOnReference();
												const NiPoint3* v100 = &v139->GetLocationOnReference();
												double v102 = (*v100 - *fValueh).SqrLength();
												if (v102 < v138) {
													TESSound* NthBrushPassthroughSound =
														GetNthBrushPassthroughSound(pstat->cPassThroughSoundIndex);
													if (NthBrushPassthroughSound) {
														uint32_t CastingType = NthBrushPassthroughSound->iFormID;
														BSAudio* v103 = &BSAudio::QInstance();
														BSSoundHandle* SoundHandleByNumericID =
															v103->GetSoundHandleByNumericID(&v135, CastingType, 257u);
														SoundHandleByNumericID->Play(0);
														uint32_t v104 = BSGetTickCount();
														time_11E0BA8 = RandomInt(1500, 2000) + v104;
													}
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
					bool v133;
					ItemChange* pammo_2 = this->pCurrentProcess->GetCurrentAmmo();
					if (pammo_2) {
						int32_t v105 = pammo_2->iNumber;
						v133 = v105 >= pweapon->GetAmmoPerShot();
					} else {
						v133 = 0;
					}
					if (!v133 ||
						(pcontrols->GetUserAction(Controls::UACTN_USE, 2) ||
							!pcontrols->GetUserAction(Controls::UACTN_USE, 0))) {
						this->pContinuousBeamPersistant->SetKill(this);
					}
					this->pContinuousBeamPersistant->UpdateProjectile(0.0f);
				}
				if (this->fLastHelloTime > 0.0) {
					double v131 = static_cast<double>(BSGetTickCount()) - this->fLastHelloTime;
					if (fHelloCooldownTime.Float() * 1000.0 < v131) {
						this->fLastHelloTime = -1.0f;
					}
				}
			}
		} else {
			if (this->spGrabSpring.m_pObject) {
				this->ForcePhysicsGrabDrop();
			}
			this->ForceGrenadeHold();
			this->b3rdPerson = !this->b3rdPerson;
			this->Character::Update(afDelta);
			Animation* v28 = this->GetAnimation(!this->b3rdPerson);
			this->UpdateAnimationMovement(v28, afDelta);
			this->b3rdPerson = !this->b3rdPerson;
			this->Character::Update(afDelta);
			Animation* v29 = this->GetAnimation(!this->b3rdPerson);
			this->UpdateAnimationMovement(v29, afDelta);
			this->UpdateCamera(0, 0);
		}
	}
}

void PlayerCharacter::Resurrect(bool abResetInventory, bool abAttach3D, bool abGetupResurect) {
	HighProcess* v9;
	LowProcess* v10;
	fHealthModifier = 0.0f;
	Interface::UpdateActorValue(ActorValue::HEALTH);
	ReferenceProcessLists.kUpdateLock.Lock(0);
	BaseProcess* pCurrentProcess = this->pCurrentProcess;
	if (pCurrentProcess) {
		delete pCurrentProcess;
	}
	LowProcess* apMemory = static_cast<LowProcess*>(ourMalloc(sizeof(LowProcess)));
	if (apMemory) {
		v10 = ::new (apMemory) LowProcess;
	} else {
		v10 = 0;
	}
	this->pCurrentProcess = v10;
	HighProcess* v14 = static_cast<HighProcess*>(ourMalloc(sizeof(HighProcess)));
	if (v14) {
		v9 = ::new (v14) HighProcess;
	} else {
		v9 = 0;
	}
	BaseProcess* v19 = v9;
	v9->Copy(this->pCurrentProcess);
	PROCESS_TYPE CurrentProcessType = GetCurrentProcessType();
	ReferenceProcessLists.RemoveReference(this, CurrentProcessType);
	BaseProcess* v13 = this->pCurrentProcess;
	if (v13) {
		delete v13;
	}
	this->pCurrentProcess = v19;
	PROCESS_TYPE v5 = GetCurrentProcessType();
	ReferenceProcessLists.RemoveReference(this, v5);
	ReferenceProcessLists.kUpdateLock.Unlock();
	InitHavok();
	bhkCharacterController* CharController = GetCharController();
	if (CharController) {
		CameraCaster* v8;
		CFilter a2;
		CharController->GetCollisionFilter(&a2);
		uint32_t aiGroup = a2.GetGroup();
		CameraCaster* v12 = static_cast<CameraCaster*>(ourMalloc(sizeof(CameraCaster)));
		if (v12) {
			uint32_t v7 = aiGroup;
			float* v6 = &fCameraCasterSize.Float();
			v8 = ::new (v12) CameraCaster(*v6, v7);
		} else {
			v8 = 0;
		}
		pCameraCaster = v8;
	} else {
		pCameraCaster = 0;
	}
	Actor::Resurrect(0, 0, 0);
	InitAnimation();
}

void PlayerCharacter::SetAdvanceNumber(int32_t inumber) {
	iNumAdvance = inumber;
}

int32_t PlayerCharacter::GetAdvanceNumber() {
	return iNumAdvance;
}

void PlayerCharacter::SetSkillAdvanceDelayed(ActorValue::Index aeskill) {
	eskilladvance = static_cast<ActorValue::Index>((int32_t)aeskill);
}

ActorValue::Index PlayerCharacter::GetSkillAdvanceDelayed() {
	return static_cast<ActorValue::Index>((int32_t)eskilladvance);
}

void PlayerCharacter::UseActionPoints(ActionPoints::Action aeAction) {
	bool v2 = pVATS.IsInVats();
	float RequiredAPForAction = ActionPoints::GetRequiredAPForAction(aeAction, v2);
	UseActionPoints(RequiredAPForAction);
}

void PlayerCharacter::UseActionPoints(float afAmount) {
	float v2;
	if (((!PlayerCharacter::IsGodMode()) && (!pSaveLoadGame->GetSaveGameLoading())) && (afAmount > 0.0)) {
		v2 = -afAmount;
		DamageModActorValue(ActorValue::ACTION_POINTS, v2, nullptr);
	}
}

bool PlayerCharacter::IsOverEncumbered() {
	if (PlayerCharacter::IsGodMode()) {
		return (bool)0;
	}
	double v2 = (double)static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::INVENTORY_WEIGHT);
	return (bool)((uint8_t)(Actor::GetMaxCarryWeight() < v2));
}

// True while the player, under AI control, runs a cannibal package.
bool PlayerCharacter::GetCannibal() {
	if (!GetUnderAIControl())
		return false;
	BaseProcess* pProcess = MobileObject::GetCurrentAIProcess();
	TESPackage* pPackage = pProcess->GetCurrentPackage();
	if (!pPackage)
		return false;
	if (pPackage->GetPackType() == PACKAGE_TYPE_CANNIBAL)
		return true;
	return false;
}

bool PlayerCharacter::GetSandman() {
	uint8_t result = 0;
	if (!GetUnderAIControl()) {
		return (bool)result;
	}
	BaseProcess* currentProcess = MobileObject::GetCurrentAIProcess();
	TESPackage* v4 = currentProcess->GetCurrentPackage();
	if (!v4) {
		return (bool)result;
	}
	if (((int32_t)v4->GetPackType()) == 37) {
		return (bool)1;
	}
	return (bool)result;
}

void PlayerCharacter::InitiateSandmanPackage(Actor* apTarget, TESObjectREFR* apfurniture, FurnitureMark* aMarker,
	uint8_t auiIndex) {
	PackageTarget* pnew;
	PackageLocation* ploc;
	iSandmanDetection = ReferenceProcessLists.GetHighestDetectionLevelForActor(pPlayer, 0);
	SetAIControlledPackage(1);
	ForceTemp3rdPerson(1);
	pCurrentProcess->SetRunOncePackage(0, this);
	pCurrentProcess->ClearActionHeadTrackTarget(0);
	TESPackage* ppack = TESPackage::CreatePackage(PACKAGE_TYPE_SANDMAN);
	ppack->SetMustReachLocation(1);
	ppack->SetMustComplete(1);
	PackageLocation* v10 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
	if (v10) {
		ploc = ::new (v10) PackageLocation;
	} else {
		ploc = 0;
	}
	ploc->SetLocType(PLT_REFERENCE);
	ploc->SetLocReference(this);
	ppack->SetPackageLocation(ploc);
	if (ploc) {
		delete ploc;
	}
	PackageTarget* v9 = static_cast<PackageTarget*>(ourMalloc(sizeof(PackageTarget)));
	if (v9) {
		pnew = ::new (v9) PackageTarget;
	} else {
		pnew = 0;
	}
	ppack->SetPackageTarget(pnew);
	if (pnew) {
		delete pnew;
	}
	ppack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_SANDMAN;
	ppack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
	ppack->GetPackageTarget()->SetTargReference(apTarget);
	ppack->GetPackageTarget()->iValue = 0;
	BaseProcess* CurrentAIProcess = pCurrentProcess;
	CurrentAIProcess->ComputeLastTimeProcessed();
	SetSandman(1);
	pCurrentProcess->SetFurniture(apfurniture, auiIndex, aMarker);
	PutCreatedPackage(ppack, 0, 1);
	apTarget->InitiateDoNothingPackage();
}

void PlayerCharacter::InitiateCannibalPackage(Actor* apTarget) {
	if (!apTarget->GetHasBeenEaten()) {
		PackageTarget* pnew;
		PackageLocation* ploc;
		SetAIControlledPackage(1);
		ForceTemp3rdPerson(1);
		pCurrentProcess->SetRunOncePackage(0, this);
		pCurrentProcess->ClearActionHeadTrackTarget(0);
		TESPackage* ppack = TESPackage::CreatePackage(PACKAGE_TYPE_CANNIBAL);
		ppack->SetMustReachLocation(1);
		ppack->SetMustComplete(1);
		PackageLocation* v7 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
		if (v7) {
			ploc = ::new (v7) PackageLocation;
		} else {
			ploc = 0;
		}
		ploc->SetLocType(PLT_REFERENCE);
		ploc->SetLocReference(this);
		ppack->SetPackageLocation(ploc);
		if (ploc) {
			delete ploc;
		}
		PackageTarget* v6 = static_cast<PackageTarget*>(ourMalloc(sizeof(PackageTarget)));
		if (v6) {
			pnew = ::new (v6) PackageTarget;
		} else {
			pnew = 0;
		}
		ppack->SetPackageTarget(pnew);
		if (pnew) {
			delete pnew;
		}
		ppack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_CANNIBAL;
		ppack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
		ppack->GetPackageTarget()->SetTargReference(apTarget);
		ppack->GetPackageTarget()->iValue = 0;
		BaseProcess* CurrentAIProcess = pCurrentProcess;
		CurrentAIProcess->ComputeLastTimeProcessed();
		PutCreatedPackage(ppack, 0, 1);
	}
}

// Applies refraction to both 3D models, the extra data and the pip-boy. With the flag set and a power of 0 or less, the
// script's refraction power is used, and a power still 0 or less clears the flag. A NaN power counts as positive in
// both compares.
void PlayerCharacter::SetRefraction(bool abEnable, float afRefractionPower) {
	float fPower = afRefractionPower;
	if (abEnable && fPower <= 0.0) {
		if (this->GetCurrentAIProcess()) {
			BaseProcess* pProcess = this->GetCurrentAIProcess();
			fPower = pProcess->GetScriptRefractionPower();
		}
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
	Actor::CheckSkillLevelChange(aiWhich, 1);
}

void PlayerCharacter::TempModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
	if (!ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
		ActorValueOwner* a1;
		ActorValueOwner* a5;
		float foldValue;
		int32_t aiValuea = DifficultyLevelCheckModHealthI(aiWhich, aiValue, apAttacker);
		if (ActorValue::HasModifiedCallback(aiWhich)) {
			foldValue = GetActorFloatValue(aiWhich);
		} else {
			foldValue = 0.0f;
		}
		AlterModifier(PlayerCharacter::TEMPORARY, aiWhich, static_cast<float>(aiValuea), ModifierList::NO_CLAMP);
		Interface::UpdateActorValue(aiWhich);
		if (aiWhich == ActorValue::HEALTH && aiValuea < 0) {
			HandleHealthDamage(apAttacker, static_cast<float>(aiValuea));
		}
		CheckSkillLevelChange(aiWhich, 0);
		if (apAttacker) {
			a5 = static_cast<ActorValueOwner*>(apAttacker);
		} else {
			a5 = 0;
		}
		if (this) {
			a1 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
		} else {
			a1 = 0;
		}
		ActorValue::CheckCallModifiedCallback(a1, aiWhich, foldValue, static_cast<float>(aiValuea), a5);
	}
}

void PlayerCharacter::TempModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
	if (!ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
		ActorValueOwner* a1;
		ActorValueOwner* a5;
		float foldValue;
		float afValuea = DifficultyLevelCheckModHealth(aiWhich, afValue, apAttacker);
		if (ActorValue::HasModifiedCallback(aiWhich)) {
			foldValue = GetActorFloatValue(aiWhich);
		} else {
			foldValue = 0.0f;
		}
		AlterModifier(PlayerCharacter::TEMPORARY, aiWhich, afValuea, ModifierList::NO_CLAMP);
		Interface::UpdateActorValue(aiWhich);
		if (aiWhich == ActorValue::HEALTH && afValuea < 0.0) {
			HandleHealthDamage(apAttacker, afValuea);
		}
		CheckSkillLevelChange(aiWhich, 0);
		if (apAttacker) {
			a5 = static_cast<ActorValueOwner*>(apAttacker);
		} else {
			a5 = 0;
		}
		if (this) {
			a1 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
		} else {
			a1 = 0;
		}
		ActorValue::CheckCallModifiedCallback(a1, aiWhich, foldValue, afValuea, a5);
	}
}

void PlayerCharacter::PermanentModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
	float afValue = static_cast<float>(aiValue);
	if (CheckInvulnerable(aiWhich, afValue) && !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
		ActorValueOwner* a1;
		ActorValueOwner* a5;
		float foldValue;
		int32_t aiValuea = DifficultyLevelCheckModHealthI(aiWhich, aiValue, apAttacker);
		if (ActorValue::HasModifiedCallback(aiWhich)) {
			foldValue = GetActorFloatValue(aiWhich);
		} else {
			foldValue = 0.0f;
		}
		AlterModifier(PlayerCharacter::SCRIPTS, aiWhich, static_cast<float>(aiValuea), ModifierList::NO_CLAMP);
		Interface::UpdateActorValue(aiWhich);
		if (aiWhich == ActorValue::HEALTH && aiValuea < 0) {
			HandleHealthDamage(apAttacker, static_cast<float>(aiValuea));
		}
		CheckSkillLevelChange(aiWhich, 0);
		if (apAttacker) {
			a5 = static_cast<ActorValueOwner*>(apAttacker);
		} else {
			a5 = 0;
		}
		if (this) {
			a1 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
		} else {
			a1 = 0;
		}
		ActorValue::CheckCallModifiedCallback(a1, aiWhich, foldValue, static_cast<float>(aiValuea), a5);
	}
}

void PlayerCharacter::PermanentModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
	if (CheckInvulnerable(aiWhich, afValue) && !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
		ActorValueOwner* a1;
		ActorValueOwner* a5;
		float foldValue;
		float v8 = DifficultyLevelCheckModHealth(aiWhich, afValue, apAttacker);
		if (ActorValue::HasModifiedCallback(aiWhich)) {
			foldValue = GetActorFloatValue(aiWhich);
		} else {
			foldValue = 0.0f;
		}
		AlterModifier(PlayerCharacter::SCRIPTS, aiWhich, v8, ModifierList::NO_CLAMP);
		Interface::UpdateActorValue(aiWhich);
		if (aiWhich == ActorValue::HEALTH && v8 < 0.0) {
			HandleHealthDamage(apAttacker, v8);
		}
		CheckSkillLevelChange(aiWhich, 0);
		if (apAttacker) {
			a5 = static_cast<ActorValueOwner*>(apAttacker);
		} else {
			a5 = 0;
		}
		if (this) {
			a1 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
		} else {
			a1 = 0;
		}
		ActorValue::CheckCallModifiedCallback(a1, aiWhich, foldValue, v8, a5);
	}
}

void PlayerCharacter::DamageModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	float fValue = static_cast<float>(aiValue);
	if (CheckInvulnerable(aiWhich, fValue) && !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
		ActorValueOwner* v10;
		ActorValueOwner* a5;
		float foldValue;
		double v16;
		if (aiWhich == ActorValue::HEALTH) {
			double v17 = GetActorFloatValue(ActorValue::HEALTH);
			v16 = v17 / GetPermanentActorFloatValue(ActorValue::HEALTH);
		} else {
			v16 = 1.0;
		}
		float foldHealthRatio = v16;
		int32_t aiValuea = DifficultyLevelCheckModHealthI(aiWhich, aiValue, apAttacker);
		if (ActorValue::HasModifiedCallback(aiWhich)) {
			foldValue = GetActorFloatValue(aiWhich);
		} else {
			foldValue = 0.0f;
		}
		ModifierList::ClampStyle eclampStyle =
			static_cast<ModifierList::ClampStyle>(ActorValue::IsFlagSet(aiWhich, ActorValue::DAMAGE_IS_POSITIVE));
		AlterModifier(PlayerCharacter::DAMAGE, aiWhich, static_cast<float>(aiValuea), eclampStyle);
		Interface::UpdateActorValue(aiWhich);
		if (aiWhich == ActorValue::HEALTH) {
			double v14 = GetActorFloatValue(ActorValue::HEALTH);
			float fhealthRatio = v14 / GetPermanentActorFloatValue(ActorValue::HEALTH);
			if (static_cast<double>(aiValuea) < 0.0) {
				HandleHealthDamage(apAttacker, static_cast<float>(aiValuea));
			}
			if (!(fhealthRatio <= 0.0) && !(fPlayerHealthHeartbeatSlow.Float() < static_cast<double>(fhealthRatio))) {
				if (!(fPlayerHealthHeartbeatSlow.Float() >= static_cast<double>(fhealthRatio)) ||
					(!(fPlayerHealthHeartbeatFast.Float() < static_cast<double>(fhealthRatio)) ||
						(!(fPlayerHealthHeartbeatFast.Float() >= static_cast<double>(foldHealthRatio)) &&
							!(fPlayerHealthHeartbeatSlow.Float() < static_cast<double>(foldHealthRatio))))) {
					if (fPlayerHealthHeartbeatFast.Float() < static_cast<double>(foldHealthRatio) &&
						fPlayerHealthHeartbeatFast.Float() >= static_cast<double>(fhealthRatio)) {
						StatusSoundHandle.Stop();
						StatusSoundHandle.Release();
						BSAudio* v5 = &BSAudio::QInstance();
						BSSoundHandle* SoundHandleByName =
							v5->GetSoundHandleByName(&arHandle, "UIHealthHeartbeatBLP", 49u);
						StatusSoundHandle.operator=(*SoundHandleByName);
						StatusSoundHandle.Play(1);
					}
				} else {
					StatusSoundHandle.Stop();
					StatusSoundHandle.Release();
					BSAudio* v4 = &BSAudio::QInstance();
					BSSoundHandle* v13 = v4->GetSoundHandleByName(&a1, "UIHealthHeartbeatALP", 49u);
					StatusSoundHandle.operator=(*v13);
					StatusSoundHandle.Play(1);
				}
			} else {
				StatusSoundHandle.FadeOutAndRelease(1000u);
			}
		}
		CheckSkillLevelChange(aiWhich, 0);
		if (apAttacker) {
			a5 = static_cast<ActorValueOwner*>(apAttacker);
		} else {
			a5 = 0;
		}
		if (this) {
			v10 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
		} else {
			v10 = 0;
		}
		ActorValue::CheckCallModifiedCallback(v10, aiWhich, foldValue, static_cast<float>(aiValuea), a5);
	}
}

void PlayerCharacter::DamageModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	if (CheckInvulnerable(aiWhich, afValue) && !ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
		ActorValueOwner* apActor;
		ActorValueOwner* a5;
		float foldValue;
		double v12;
		if (aiWhich == ActorValue::HEALTH) {
			double v13 = GetActorFloatValue(ActorValue::HEALTH);
			v12 = v13 / GetPermanentActorFloatValue(ActorValue::HEALTH);
		} else {
			v12 = 1.0;
		}
		float foldHealthRatio = v12;
		float afValuea = DifficultyLevelCheckModHealth(aiWhich, afValue, apAttacker);
		if (ActorValue::HasModifiedCallback(aiWhich)) {
			foldValue = GetActorFloatValue(aiWhich);
		} else {
			foldValue = 0.0f;
		}
		ModifierList::ClampStyle eclampStyle =
			static_cast<ModifierList::ClampStyle>(ActorValue::IsFlagSet(aiWhich, ActorValue::DAMAGE_IS_POSITIVE));
		AlterModifier(PlayerCharacter::DAMAGE, aiWhich, afValuea, eclampStyle);
		Interface::UpdateActorValue(aiWhich);
		if (aiWhich == ActorValue::HEALTH) {
			double v10 = GetActorFloatValue(ActorValue::HEALTH);
			float fhealthRatio = v10 / GetPermanentActorFloatValue(ActorValue::HEALTH);
			if (afValuea < 0.0) {
				HandleHealthDamage(apAttacker, afValuea);
			}
			if (!(fhealthRatio <= 0.0) && !(fPlayerHealthHeartbeatSlow.Float() < static_cast<double>(fhealthRatio))) {
				if (!(fPlayerHealthHeartbeatSlow.Float() >= static_cast<double>(fhealthRatio)) ||
					(!(fPlayerHealthHeartbeatFast.Float() < static_cast<double>(fhealthRatio)) ||
						(!(fPlayerHealthHeartbeatFast.Float() >= static_cast<double>(foldHealthRatio)) &&
							!(fPlayerHealthHeartbeatSlow.Float() < static_cast<double>(foldHealthRatio))))) {
					if (fPlayerHealthHeartbeatFast.Float() < static_cast<double>(foldHealthRatio) &&
						fPlayerHealthHeartbeatFast.Float() >= static_cast<double>(fhealthRatio)) {
						StatusSoundHandle.Stop();
						StatusSoundHandle.Release();
						BSAudio* v5 = &BSAudio::QInstance();
						BSSoundHandle* SoundHandleByName =
							v5->GetSoundHandleByName(&arHandle, "UIHealthHeartbeatBLP", 49u);
						StatusSoundHandle.operator=(*SoundHandleByName);
						StatusSoundHandle.Play(1);
					}
				} else {
					StatusSoundHandle.Stop();
					StatusSoundHandle.Release();
					BSAudio* v4 = &BSAudio::QInstance();
					BSSoundHandle* v9 = v4->GetSoundHandleByName(&a1, "UIHealthHeartbeatALP", 49u);
					StatusSoundHandle.operator=(*v9);
					StatusSoundHandle.Play(1);
				}
			} else {
				StatusSoundHandle.FadeOutAndRelease(1000u);
			}
		}
		CheckSkillLevelChange(aiWhich, 0);
		if (apAttacker) {
			a5 = static_cast<ActorValueOwner*>(apAttacker);
		} else {
			a5 = 0;
		}
		if (this) {
			apActor = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
		} else {
			apActor = 0;
		}
		ActorValue::CheckCallModifiedCallback(apActor, aiWhich, foldValue, afValuea, a5);
	}
}

void PlayerCharacter::ModActorBaseValue(ActorValue::Index aiWhich, int32_t aiValue) {
	ActorValueOwner* a1;
	float foldValue;
	if (ActorValue::HasModifiedCallback(aiWhich)) {
		foldValue = GetActorFloatValue(aiWhich);
	} else {
		foldValue = 0.0f;
	}
	bool v10 = 0;
	float v9 = GetBaseValueOverride(aiWhich, v10);
	if (v10) {
		float v6 = static_cast<double>(aiValue) + foldValue;
		SetBaseValueOverride(aiWhich, v6);
	} else {
		TESActorBase* ActorBase = GetActorBase();
		ActorBase->ModActorValue(aiWhich, aiValue);
	}
	Interface::UpdateActorValue(aiWhich);
	CheckSkillLevelChange(aiWhich, 1);
	if (this) {
		a1 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
	} else {
		a1 = 0;
	}
	ActorValue::CheckCallModifiedCallback(a1, aiWhich, foldValue, static_cast<float>(aiValue), 0);
}

void PlayerCharacter::ModActorBaseValue(ActorValue::Index aiWhich, float afValue) {
	ActorValueOwner* a1;
	float foldValue;
	if (ActorValue::HasModifiedCallback(aiWhich)) {
		foldValue = GetActorFloatValue(aiWhich);
	} else {
		foldValue = 0.0f;
	}
	bool v9 = 0;
	float v8 = GetBaseValueOverride(aiWhich, v9);
	if (v9) {
		float v5 = foldValue + afValue;
		SetBaseValueOverride(aiWhich, v5);
	} else {
		TESActorBase* ActorBase = GetActorBase();
		ActorBase->ModActorValue(aiWhich, afValue);
	}
	Interface::UpdateActorValue(aiWhich);
	CheckSkillLevelChange(aiWhich, 1);
	if (this) {
		a1 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*this)));
	} else {
		a1 = 0;
	}
	ActorValue::CheckCallModifiedCallback(a1, aiWhich, foldValue, afValue, 0);
}

bool PlayerCharacter::DamageEquipment(ItemChange* apEquipment, float afDamage, bool abIgnoreArmorSkill) {
	return (bool)((uint8_t)((!PlayerCharacter::IsGodMode()) &&
		Actor::DamageEquipment(apEquipment, afDamage, abIgnoreArmorSkill)));
}

TESObjectREFR* PlayerCharacter::DropObject(TESObject* apObject, ExtraDataList* apExtra, int32_t ainumber,
	const NiPoint3* apPoint, const NiPoint3* apRotate) {
	pVATS.QuitVATSPlayback(0, 0);
	TESObjectCELL* pparentcell = GetParentCell();
	if (Interface::PreloadWorldMap() && pparentcell) {
		TESObjectREFR* plootbag = pparentcell->GetLootBag();
		if (!plootbag) {
			plootbag = pparentcell->CreateLootBagAtRef(this);
		}
		if (plootbag) {
			RemoveItem(static_cast<TESBoundObject*>(apObject), apExtra, ainumber, 0, 0, plootbag, 0, 0, 1, 0);
			return 0;
		}
	} else {
		if (apObject->GetFormType() == AMMO_ID) {
			if (GetCurrentWeapon()) {
				PlayerCharacter* v11 = pPlayer;
				TESObjectWEAP* CurrentWeapon = GetCurrentWeapon();
				if (CurrentWeapon->GetCurrentAmmo(v11) == apObject) {
					pCurrentProcess->GetCurrentAmmo()->iNumber = 0;
					NiAVObject* v8 = Get3D(0);
					NiAVObject* pround = BSUtilities::GetObjectByName(v8, "##NifRound");
					if (pround) {
						pround->SetAppCulled(1);
					}
					NiAVObject* v9 = Get3D(0);
					pround = BSUtilities::GetObjectByName(v9, "##NifRound");
					if (pround) {
						pround->SetAppCulled(1);
					}
				}
			}
		}
	}
	if (pQueuedWeaponAttach) {
		if (apObject == pQueuedWeaponAttach) {
			if (apExtra) {
				if (apExtra->GetWorn(0)) {
					pQueuedWeaponAttach = 0;
				}
			}
		}
	}
	TESBoundObject* pboundobj = apObject->IsBoundObject() ? static_cast<TESBoundObject*>(apObject) : 0;
	TESObjectREFR* pobjREFR = RemoveItem(pboundobj, apExtra, ainumber, 0, 1, 0, apPoint, apRotate, 1, 0);
	if (pobjREFR) {
		NiAVObject* pav = pobjREFR->Get3D();
		if (pav) {
			bhkWorld::Enable(pav, 0, 1, 1);
			NiUpdateData UpdateData(0.0f, 0, 0);
			pav->Update(UpdateData);
			bhkWorld::Enable(pav, 1, 1, 1);
			NiPoint3 location = pobjREFR->GetLocationOnReference();
			const NiBound* WorldBound = &pav->GetWorldBound();
			NiPoint3 shift = location - WorldBound->m_kCenter;
			location.operator+=(shift);
			pav->SetTranslate(location);
			pobjREFR->SetLocationOnReference(location);
			bhkNiCollisionObject::ResetSim(pav, 1);
			bhkWorld::Activate(pav, 1, 1, 0);
		} else {
			DroppedRefList.AddHead(pobjREFR);
			pobjREFR->SetRecentPCDrop(1);
			pobjREFR->SetCenterOnCreation(1);
		}
		PlayPickUpSound(pboundobj, 0, 0);
	}
	return pobjREFR;
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
	} else if (eFormType == AMMO_ID) {
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
		} else {
			pActor->SetCurrentTarget(nullptr);
		}
		pIter = pIter->GetNext();
	}
	if (pTargeters) {
		pTargeters->RemoveAll();
		delete pTargeters;
	}
	TESFile* pFile = apobjREFR->GetFile(-1);
	if (!Interface::IsMenuIDVisible(Interface::eDialogMenu, 0) && !Interface::ContainerIsBarter() &&
		!Interface::ContainerIsRecipe()) {
		TESForm* pOwner = apobjREFR->GetOwner();
		if (!pOwner || apobjREFR->IsAnOwner(this, true) || apobjREFR->GetObjectReference()->GetFormType() == PROJ_ID) {
			apobjREFR->GetExtra()->RemoveOwnership();
		} else {
			pContainerOwner = ReferenceProcessLists.FindOwnerSellsObject(apobjREFR);
			if (apobjREFR->GetMaster() || pFile || apobjREFR->GetAction(OBJECT_ACTION_ACTIVATE)) {
				TESBoundObject* pObject = apobjREFR->GetObjectReference();
				StealAlarm(apobjREFR, pObject, aicount, 0, pOwner);
			} else if (pContainerOwner) {
				TESBoundObject* pObject = apobjREFR->GetObjectReference();
				StealAlarm(pContainerOwner, pObject, aicount, 0, pOwner);
			}
			if (apobjREFR->IsOwnerEvil(nullptr)) {
				apobjREFR->GetExtra()->RemoveOwnership();
			} else {
				apobjREFR->GetExtra()->SetOwnership(pOwner);
			}
		}
	}
	uint8_t eObjectType = apobjREFR->GetObjectReference()->GetFormType();
	if (eObjectType == WEAP_ID) {
		pWeapon = apobjREFR->GetObjectReference();
		// A thrown weapon is its own ammo: picking up the equipped one adds to both stacks.
		if (static_cast<TESObjectWEAP*>(pWeapon)->IsThrownWeapon() && pCurrentProcess->GetCurrentAmmo() &&
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
	} else if (eObjectType == AMMO_ID) {
		abflag = false;
	}
	bool bDeleteRef = false;
	if (apobjREFR->GetMaster() || pFile) {
		apobjREFR->MarkAsPickedUp();
		AddReferenceItem(apobjREFR, aicount, abflag, bAmmoAdded);
	} else if (apobjREFR->GetAction(OBJECT_ACTION_ACTIVATE)) {
		apobjREFR->MarkAsPickedUp();
		AddReferenceItem(apobjREFR, aicount, false, bAmmoAdded);
	} else {
		AddReferenceItem(apobjREFR, aicount, false, bAmmoAdded);
		bDeleteRef = true;
	}
	if (!pCurrentProcess->GetCurrentAmmo() || !pCurrentProcess->GetCurrentAmmo()->GetCount()) {
		if (pCurrentProcess->GetCurrentWeapon() && apobjREFR) {
			TESBoundObject* pObject = apobjREFR->GetObjectReference();
			PlayerCharacter* pHolder = pPlayer;
			TESObjectWEAP* pCurrentWeapon =
				static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject());
			if (pObject == pCurrentWeapon->GetCurrentAmmo(pHolder)) {
				// WEAPON_MOD_AMMO_REGEN_SECONDS
				bool bRegenMod = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(6);
				TESObjectWEAP* pRegenWeapon =
					static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject());
				if (pRegenWeapon->GetAmmoRegenRate(bRegenMod) <= 0.0) {
					// WEAPON_MOD_INCREASE_CLIP_SIZE
					bool bUseModValue = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(2);
					int32_t iReloadAnim = IsWeaponDrawn() ? 2 : 0;
					TESObjectWEAP* pReloadWeapon =
						static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject());
					ReloadWeaponNV(pReloadWeapon, iReloadAnim, bUseModValue, false);
				}
			}
		}
	}
	if (bDeleteRef) {
		delete apobjREFR;
	}
}

void PlayerCharacter::CastScroll(TESObjectBOOK* apScroll, MagicTarget* apTarget) {
	Actor::CastScroll(apScroll, apTarget);
	uint32_t CastingType = apScroll->GetFormID();
	ItemChange* InventoryItem_0 = GetInventoryItem(apScroll, CastingType);
	if (InventoryItem_0) {
		delete InventoryItem_0;
		SetSelectedScroll(apScroll);
	} else {
		SetSelectedScroll(0);
	}
}

bool PlayerCharacter::AddSpell(SpellItem* apSpellIn) {
	int8_t v4 = (int8_t)Actor::AddSpell(apSpellIn);
	if (v4) {
		SpellLearned(apSpellIn);
	}
	return (bool)v4;
}

bool PlayerCharacter::RemoveSpell(SpellItem* apSpellIn) {
	MagicItem* v3;
	int8_t v5 = (int8_t)Actor::RemoveSpell(apSpellIn);
	if (!v5) {
		return (bool)v5;
	}
	if (apSpellIn) {
		v3 = static_cast<MagicItem*>(apSpellIn);
	} else {
		v3 = nullptr;
	}
	if (v3 == GetSelectedSpell()) {
		SetSelectedSpell(nullptr);
	}
	return (bool)v5;
}

// Reloads apWeapon: picks its ammo (the weapon itself for grenades, mines and thrown weapons; a hotkeyed ammo
// override), makes it the current ammo, plays the reload animation for aiReloadAnim 2 (only when the clip is neither
// full nor holding all the ammo) or 1, equips the ammo and charges the reload's action points outside VATS.
bool PlayerCharacter::ReloadWeapon(TESObjectWEAP* apWeapon, int32_t aiReloadAnim, bool bUseModValue) {
	TESForm* pAmmo = apWeapon->GetCurrentAmmo(pPlayer);
	ItemChange* pWeaponItem = pCurrentProcess->GetCurrentWeapon();
	// 10-13: the one-handed grenade, mine, lunchbox mine and thrown weapon types are their own ammo.
	if (!pAmmo &&
		(apWeapon->GetWeaponType() == 10 || apWeapon->GetWeaponType() == 11 || apWeapon->GetWeaponType() == 12 ||
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
		void* pMemory = ourMalloc(sizeof(ItemChange));
		ItemChange* pNewAmmo =
			pMemory ? ::new (pMemory) ItemChange(static_cast<TESBoundObject*>(pAmmo), iCount) : nullptr;
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
		if (!pCurrentProcess || pCurrentProcess->GetProcessLevel() > PROCESS_TYPE_MIDDLE_HIGH ||
			Interface::IsInMenuMode() || pSaveLoadGame->GetSaveGameLoading()) {
			QueueEquipObject(static_cast<TESBoundObject*>(pAmmo), iCount, nullptr, true, false, false);
		}
		pAmmo->SetFormType(0x28);
	} else {
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
			} else {
				pAnimation->QueueReloadAnim(sGroup);
			}
		}
	}
	if (!pVATS.IsInVats()) {
		UseActionPoints(ActionPoints::ACTION_RELOAD);
	}
	return true;
}

bool PlayerCharacter::ReloadWeaponNV(TESObjectWEAP* apWeapon, int32_t aiReloadAnim, bool bUseModValue,
	bool bFromInventory) {
	if (apWeapon && apWeapon->GetIsLoopingReload()) {
		return ReloadLoop(apWeapon, aiReloadAnim, bUseModValue, bFromInventory);
	} else {
		return ReloadWeapon(apWeapon, aiReloadAnim, bUseModValue);
	}
}

uint32_t PlayerCharacter::UseAmmo(int32_t aiShotCount) {
	if ((!pCurrentProcess) || (!pCurrentProcess->GetCurrentAmmo())) {
		return (uint32_t)0;
	}
	if ((!PlayerCharacter::IsGodMode()) || PlayerCharacter::IsDemigodMode()) {
		return Actor::UseAmmo(aiShotCount);
	}
	return (uint32_t)100;
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
	SetAIControlledFromPosition(1);
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
	bBeenAttacked = (uint8_t)abFlag;
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
				pPlayer->ApplyAutoAimActor(nullptr, pprojectile, fdeltaHeading, fdeltaLooking, nullptr, 1.17549435e-38f,
					1.17549435e-38f);
			}
			double dHeadingDelta = static_cast<double>(fcurrentChaseHeading) - fdeltaHeading;
			double dHeadingStep = appTimer.GetSeconds() * dHeadingDelta;
			fcurrentChaseHeading = fcurrentChaseHeading - fAutoAimMeleeChaseMult.Float() * dHeadingStep;
			double dLookingDelta = static_cast<double>(fcurrentChaseLooking) - fdeltaLooking;
			double dLookingStep = appTimer.GetSeconds() * dLookingDelta;
			fcurrentChaseLooking = fcurrentChaseLooking - fAutoAimMeleeChaseMult.Float() * dLookingStep;
			gunWobble.FromEulerAnglesXYZ(fcurrentChaseHeading, 0.0f, fcurrentChaseLooking);
		} else {
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
				} else {
					fdrift = GetGunSkill(GUN_SKILL_ACTOR);
				}
				bool battacking = IsAttacking();
				if (!battacking) {
					fdrift = fdrift * fNonAttackGunWobbleMult.Float();
				}
				float fdelta = fdrift - fWobbleChaseDrift;
				float* pfChaseDriftTime = &fGunWobbleChaseDriftTime.Float();
				float fstep = appTimer.GetSeconds() * *pfChaseDriftTime;
				// The compares are written so that a NaN delta still eases, a NaN step snaps, and a NaN delta steps
				// down, as the game does.
				if (fdelta != 0.0) {
					if (!(fstep < std::fabs(fdelta))) {
						fWobbleChaseDrift = fdrift;
					} else if (fdelta > 0.0) {
						if (!battacking) {
							fWobbleChaseDrift = fWobbleChaseDrift + fstep;
							fdrift = fWobbleChaseDrift;
						} else {
							fWobbleChaseDrift = fdrift;
						}
					} else {
						fWobbleChaseDrift = fWobbleChaseDrift - fstep;
						fdrift = fWobbleChaseDrift;
					}
				}
				if (!pwobbleObj->GetCollisionObject()) {
					void* pMemory = NiMemObject::operator new(sizeof(bhkBlendCollisionObjectAddRotation));
					bhkBlendCollisionObjectAddRotation* pnewBlendCol =
						pMemory ? ::new (pMemory) bhkBlendCollisionObjectAddRotation : nullptr;
					pwobbleObj->SetCollisionObject(pnewBlendCol);
				} else {
					bhkBlendCollisionObject* pblendCol = static_cast<
						bhkBlendCollisionObject*>(NiObject::DynamicCastStatic(&bhkBlendCollisionObject::ms_RTTI,
						pwobbleObj->GetCollisionObject()));
					if (pblendCol) {
						NiObject* pAddRotation =
							NiObject::DynamicCastStatic(&bhkBlendCollisionObjectAddRotation::ms_RTTI, pblendCol);
						if (!pAddRotation) {
							void* pMemory = NiMemObject::operator new(sizeof(bhkBlendCollisionObjectAddRotation));
							bhkBlendCollisionObjectAddRotation* pnewBlendCol =
								pMemory ? ::new (pMemory) bhkBlendCollisionObjectAddRotation : nullptr;
							pnewBlendCol->CopyFrom(pblendCol);
							pwobbleObj->SetCollisionObject(pnewBlendCol);
						}
					}
				}
				bhkBlendCollisionObjectAddRotation* pblendColObj =
					static_cast<bhkBlendCollisionObjectAddRotation*>(NiObject::
							DynamicCastStatic(&bhkBlendCollisionObjectAddRotation::ms_RTTI,
								pwobbleObj->GetCollisionObject()));
				if (pblendColObj) {
					NiMatrix3 addRot = NiMatrix3_IDENTITY;
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
	GunWobbleRot = NiMatrix3_IDENTITY;
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
			} else {
				NiAVObject* pwobbleObj = pWobbleNodes[1][eweaponType];
				if (!pwobbleObj) {
					pwobbleObj = BSUtilities::GetObjectByName(Get3D(true), pwobbleNodeName);
					pWobbleNodes[1][eweaponType] = pwobbleObj;
				}
				if (pwobbleObj) {
					if (!pwobbleObj->GetCollisionObject()) {
						void* pMemory = NiMemObject::operator new(sizeof(bhkBlendCollisionObjectAddRotation));
						bhkBlendCollisionObjectAddRotation* pnewBlendCol =
							pMemory ? ::new (pMemory) bhkBlendCollisionObjectAddRotation : nullptr;
						pwobbleObj->SetCollisionObject(pnewBlendCol);
					}
					bhkBlendCollisionObjectAddRotation* pblendColObj =
						static_cast<bhkBlendCollisionObjectAddRotation*>(NiObject::
								DynamicCastStatic(&bhkBlendCollisionObjectAddRotation::ms_RTTI,
									pwobbleObj->GetCollisionObject()));
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
		PerkRankData* pperkRank_2 = static_cast<PerkRankData*>(ourMalloc(sizeof(PerkRankData)));
		pperkRank_2->pPerk = apPerk;
		pperkRank_2->cCurrentRank = acRank;
		if (bAlt) {
			if (!CompanionPerks.IsInList(pperkRank_2)) {
				CompanionPerks.AddHead(pperkRank_2);
			}
		} else {
			if (!Perks.IsInList(pperkRank_2)) {
				Perks.AddHead(pperkRank_2);
			}
		}
		apPerk->ApplyPerk(this, 0, acRank, bAlt);
		if (!bAlt) {
			StatsMenu::UpdatePerks();
		}
		SetReloadSpeed();
	}
}

void PlayerCharacter::RemovePerk(BGSPerk* apPerk, bool bAlt) {
	PerkRankData* v4;
	BSSimpleList<PerkRankData*>* ecx0;
	PerkRankData* v6;
	if (apPerk) {
		v6 = nullptr;
		ecx0 = &Perks;
		if (bAlt) {
			ecx0 = &CompanionPerks;
		}
		while (ecx0 && (!v6)) {
			v4 = ecx0->m_item;
			if (v4 && (v4->pPerk == apPerk)) {
				v6 = v4;
			} else {
				ecx0 = ecx0->m_pkNext;
			}
		}
		if (v6) {
			apPerk->RemovePerk(this, bAlt);
			if (bAlt) {
				(&CompanionPerks)->Remove(v6);
			} else {
				(&Perks)->Remove(v6);
			}
			StatsMenu::UpdatePerks();
			Actor::SetReloadSpeed();
		}
	}
}

uint8_t PlayerCharacter::GetPerkRank(BGSPerk* apPerk, bool bAlt) {
	BSSimpleList<PerkRankData*>* pIter = &Perks;
	if (bAlt)
		pIter = &CompanionPerks;
	while (pIter) {
		PerkRankData* pData;
		pData = pIter->GetItem();
		if (pData && pData->pPerk == apPerk)
			return pData->cCurrentRank;
		pIter = pIter->GetNext();
	}
	return 0;
}

void PlayerCharacter::AddPerkEntry(BGSPerkEntry* apPerkEntry, bool bAlt) {
	if (apPerkEntry) {
		if (apPerkEntry->GetType() == PERK_ENTRY_TYPE_ENTRY_POINT) {
			uint8_t centryPoint = static_cast<BGSEntryPointPerkEntry*>(apPerkEntry)->GetEntryPoint();
			if (centryPoint < 74u) {
				if (bAlt) {
					CompanionPerkEntryLists[centryPoint].Insert(apPerkEntry, &BGSPerk::PerkEntryPrioritySortFunction);
				} else {
					PerkEntryLists[centryPoint].Insert(apPerkEntry, &BGSPerk::PerkEntryPrioritySortFunction);
				}
				InventoryChanges* pinventory = (&pPlayer->m_Extra)->GetContainerChanges();
				pinventory->ForceWeightRecalculation();
			}
		}
	}
}

// Removes the entry from its entry point's list, or from the companion list when bAlt is set.
void PlayerCharacter::RemovePerkEntry(BGSPerkEntry* apPerkEntry, bool bAlt) {
	if (!apPerkEntry || apPerkEntry->GetType() != PERK_ENTRY_TYPE_ENTRY_POINT)
		return;
	uint8_t cEntryPoint = static_cast<BGSEntryPointPerkEntry*>(apPerkEntry)->GetEntryPoint();
	if (cEntryPoint >= 74)
		return;
	if (bAlt)
		CompanionPerkEntryLists[cEntryPoint].Remove(apPerkEntry);
	else
		PerkEntryLists[cEntryPoint].Remove(apPerkEntry);
}

BSSimpleList<BGSPerkEntry*>* PlayerCharacter::GetPerkEntryList(uint8_t acEntryPoint, bool bAlt) {
	if (((uint32_t)((int32_t)acEntryPoint)) >= 0x4Au) {
		return (BSSimpleList<BGSPerkEntry*>*)0;
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
		asAnimGroup &= 0x7FFF;
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
	} else if (TESAnimGroup::IsIronSightsAction(asAnimGroup) &&
		pAnimation->GroupLoaded(static_cast<uint16_t>(asAnimGroup - 3))) {
		pAnimation->PlayGroup(static_cast<uint16_t>(asAnimGroup - 3), aeFlags, -1, AGS_NONE);
	} else {
		uint16_t sGroup =
			GetAnimGroup(static_cast<ANIM_GROUP_ENUM>(iType), nullptr, GetCurrentWeapon() == nullptr, pAnimation);
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
	IsDead(0);
}

// Replaces the actor mover with a player mover.
void PlayerCharacter::CreateActorMover() {
	if (pActorMover) {
		delete pActorMover;
	}
	void* pMemory = ourMalloc(sizeof(PlayerMover));
	pActorMover = pMemory ? ::new (pMemory) PlayerMover(this) : nullptr;
}

NiPoint3 PlayerCharacter::CalculateAnticipatedLocation(float afDeltaT) {
	NiPoint3 a2;
	NiPoint3 result;
	NiPoint3 kPos = *QLocationOnReference();
	if (afDeltaT > 0.0) {
		if (GetCharController()) {
			NiPoint3 kVelocity;
			bhkCharacterController* pController = GetCharController();
			pController->GetLinearVelocity(kVelocity);
			NiPoint3* v4 = &(result = kVelocity * afDeltaT);
			kPos.operator+=(*v4);
		}
	}
	a2 = kPos;
	return a2;
}

void PlayerCharacter::CastAbility(SpellItem* apSpell, bool abLoadCast) {
	static_cast<MagicCaster*>(this)->MagicCaster::CastAbility(apSpell, abLoadCast);
}

void PlayerCharacter::TransferDisease(SpellItem* apSpell, MagicTarget* apDesiredTarget, bool abLoadCast) {
	static_cast<MagicCaster*>(this)->MagicCaster::TransferDisease(apSpell, apDesiredTarget, abLoadCast);
}

MagicTarget* PlayerCharacter::GetTouchTarget() {
	MagicTarget* ptarget = 0;
	double v2 = GetReach();
	float freach = GetScale() * v2;
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

bool PlayerCharacter::CheckCast(MagicItem* apSpell, float* afEffectStrength, MagicItem::CannotCastReason* apReason,
	bool abUseBaseMagicka) {
	MagicItem::CannotCastReason ereason;
	ereason = MagicItem::NOCAST_OK;
	bool bresult = Actor::CheckCast(apSpell, afEffectStrength, &ereason, abUseBaseMagicka);
	if (apReason) {
		*apReason = ereason;
	}
	if (!bresult && ereason == MagicItem::NOCAST_MULTIPLEBOUNDEFFECTS) {
		return 0;
	}
	return PlayerCharacter::IsGodMode() || bresult;
}

float PlayerCharacter::GetMagicEffectivenessModifier(bool a1, float a2) {
	if (PlayerCharacter::IsGodMode()) {
		return Actor::GetMagicEffectivenessModifier(a1, 0.0f);
	} else {
		return 1.0f;
	}
}

void PlayerCharacter::SetCurrentSpell(MagicItem* apCurrentSpell) {
	pCurrentSpell = apCurrentSpell;
}

void PlayerCharacter::SetDesiredTarget(MagicTarget* apDesiredTarget) {
	pDesiredTarget = apDesiredTarget;
}

bool PlayerCharacter::AddTarget(MagicCaster* apCaster, MagicItem* apSpell, ActiveEffect* apActiveEffect,
	bool abAreaTarget) {
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
	if (ActorValue::IsSkill(static_cast<ActorValue::Index>(aiWhich))) {
		double dValue =
			AiFormulas::AVDeriveSkill(pPlayer ? static_cast<ActorValueOwner*>(pPlayer) : nullptr, aiWhich, 1);
		bool bExists;
		dValue = GetBaseValueOverride(static_cast<ActorValue::Index>(aiWhich), bExists) + dValue;
		fValue = static_cast<float>(GetPermanentModifier(aiWhich) + dValue);
	} else {
		double dValue = GetBaseActorFloatValue(aiWhich);
		fValue = static_cast<float>(GetPermanentModifier(aiWhich) + dValue);
	}
	return ActorValue::ClampActorValue(static_cast<ActorValue::Index>(aiWhich), fValue);
}

BSSimpleList<PerkRankData*>* PlayerCharacter::GetPerkList(bool bAlt) {
	if (bAlt) {
		return &CompanionPerks;
	}
	return &Perks;
}

PlayerCharacter::TerminalAccessType PlayerCharacter::GetTerminalAccess(TESObjectREFR* apTerminalRef) {
	BGSTerminal* v7 = 0;
	if (apTerminalRef) {
		if (apTerminalRef->GetObjectReference()) {
			TESBoundObject* ObjectReference = apTerminalRef->GetObjectReference();
			v7 = dynamic_cast<BGSTerminal*>(ObjectReference);
		}
	}
	if (!v7) {
		return PlayerCharacter::TAT_HACK_INELIGIBLE;
	}
	BGSNote* CriticalQueuedRefCount = v7->pPassword;
	if (HasNote(CriticalQueuedRefCount)) {
		return PlayerCharacter::TAT_HAS_NOTE;
	}
	if (TESObjectREFR::IsHacked(apTerminalRef) || v7->IsUnlocked(apTerminalRef)) {
		return PlayerCharacter::TAT_HACKED;
	}
	if (TESObjectREFR::IsLockedOut(apTerminalRef) || v7->GetRawDifficulty(apTerminalRef) == LL_IMPOSSIBLE) {
		return PlayerCharacter::TAT_LOCKED;
	}
	float a2 = GetClampedActorFloatValue(ActorValue::SCIENCE);
	if (v7->IsHackEligible(apTerminalRef, a2)) {
		return PlayerCharacter::TAT_HACK_ELIGIBLE;
	} else {
		return PlayerCharacter::TAT_HACK_INELIGIBLE;
	}
}

int32_t PlayerCharacter::GetLastAction() {
	int32_t v3 = 0;
	if (pListofActions) {
		if (pListofActions->m_item) {
			return pListofActions->m_item->eAction;
		}
	}
	return v3;
}

bool PlayerCharacter::IsActionInList(PLAYER_ACTION aeAction) {
	bool bexits = 0;
	for (BSSimpleList<PlayerActionObject*>* plist = pListofActions; plist && (plist->m_item && !bexits);
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
	if (!pCurrentProcess->Get3DUpdateFlag(RESET_MODEL) && Interface::GetInventoryMenuVisible()) {
	}
	pCurrentProcess->Update3dModel(this);
}

bool PlayerCharacter::GetDropPoint(TESBoundObject* apObject, NiPoint3& aPoint, bool abAllowBackupPoint,
	bool abAdvanceExtraRotate) {
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
		// abAdvanceExtraRotate is tested a second time here, as the game does.
		if (abAdvanceExtraRotate) {
			float fsin = fobjectSize * 0.5 / fdropDistance;
			double dangle = std::asin(BSMIN(1.0f, fsin));
			this->fDropAngleMod = dangle + this->fDropAngleMod;
			if (this->fDropAngleMod > 6.2831854820251465) {
				this->fDropAngleMod = this->fDropAngleMod - 6.2831854820251465;
			}
		}
	} else {
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
		// bhkPickData is deliberately not destroyed, as the game does.
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
			float fhit = pcollector.getHits().operator[]<hkpWorldRayCastOutput>(0).m_hitFraction * dlength;
			float fpos = fhit - fobjectSize / 2.0;
			testDirection *= fpos;
			aPoint = origin + testDirection;
			alignas(16) NiPoint3 differenceHoriz(testDirection.x, testDirection.y, 0.0f);
			float fdist = differenceHoriz.Length() - fobjectSize / 2.0;
			if (fdist - fcharconRadius > 0.0) {
				bvalidDirection = true;
			}
		} else {
			float fpos = fobjectSize / 2.0 + fdropDistance;
			testDirection *= fpos;
			aPoint = origin + testDirection;
			bvalidDirection = true;
		}
		if (bvalidDirection) {
			bhkSphereShape* pshape;
			bhkSphereShape* pmemory = static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
			if (pmemory) {
				float fradius = fobjectSize / 2.0;
				pshape = ::new (pmemory) bhkSphereShape(fradius, true);
			} else {
				pshape = nullptr;
			}
			NiPointer<bhkSphereShape> spshape = pshape;
			CFilter kplayerFilter2;
			pPlayer->MobileObject::GetCollisionFilter(&kplayerFilter2.iFilter);
			uint32_t igroup = kplayerFilter2.GetGroup();
			alignas(16) CFilter ifilter(L_DROPPINGPICK, igroup, P_OTHER);
			alignas(16) hkSimpleShapePhantomCinfo info;
			info.m_collisionFilterInfo = ifilter.iFilter;
			info.m_shape = spshape->operator*();
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
	} else if (aiVal == 0) {
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
	int32_t* v6;
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	int32_t icurrent = GetActorValue(ActorValue::KARMA);
	GetActorBase();
	float afKarma = GetActorFloatValue(ActorValue::KARMA);
	TESActorBaseData::ALIGNMENT ealign = TESActorBaseData::GetAlignmentForKarma(afKarma);
	if (aiAmount >= 0) {
		const char* v4;
		const char* v9;
		if (aiAmount + icurrent > iKarmaMax.Int()) {
			aiAmount = iKarmaMax.Int() - icurrent;
		}
		if (aiAmount >= iKarmaChangeThreshold.Int()) {
			v9 = sKarmaMajorGainedImage.String();
			v4 = sKarmaMajorGained.String();
		} else {
			v9 = sKarmaMinorGainedImage.String();
			v4 = sKarmaMinorGained.String();
		}
		Interface::ShowMessage(v4, 0, v9, 0, 2.0f, 0);
		BSSoundHandle psoundHand_2;
		BSAudio* v5 = &BSAudio::QInstance();
		BSSoundHandle* SoundHandleByName = v5->GetSoundHandleByName(&arHandle, "UIKarmaUp", 289u);
		psoundHand_2.operator=(*SoundHandleByName);
		psoundHand_2.Play(0);
	} else {
		const char* v2;
		const char* v8;
		if (aiAmount + icurrent < iKarmaMin.Int()) {
			aiAmount = iKarmaMin.Int() - icurrent;
		}
		if (aiAmount >= -iKarmaChangeThreshold.Int()) {
			v8 = sKarmaMinorLostImage.String();
			v2 = sKarmaMinorLost.String();
		} else {
			v8 = sKarmaMajorLostImage.String();
			v2 = sKarmaMajorLost.String();
		}
		Interface::ShowMessage(v2, 0, v8, 0, 2.0f, 0);
		BSSoundHandle psoundHand;
		BSAudio* v3 = &BSAudio::QInstance();
		BSSoundHandle* v12 = v3->GetSoundHandleByName(&a1, "UIKarmaDown", 289u);
		psoundHand.operator=(*v12);
		psoundHand.Play(0);
	}
	if (aiAmount <= 0 || (v6 = &iKarmaMax.Int(), icurrent < *v6)) {
		int32_t* v7;
		if (aiAmount >= 0 || (v7 = &iKarmaMin.Int(), icurrent > *v7)) {
			PermanentModActorValue(ActorValue::KARMA, aiAmount, 0);
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

// A file that is not good is neither closed nor deleted, and the string is then never built, as the game does. Every
// Write is Write(arString, true). Call arguments with side effects are evaluated right to left, in the game's order.
void PlayerCharacter::ExportProgressData(PROGRESS_EVENT aeEvent) {
	char acFileName[0x104];
	BSsprintf(acFileName, 0x104, "%sProgressData_1.txt", sTrackProgressPath.String());
	void* pMemory = ourMalloc(sizeof(BSFile));
	BSFile* pFile = pMemory ? ::new (pMemory) BSFile(acFileName, NiFile::WRITE_ONLY, 0x4000, false) : nullptr;
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
	} else {
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
		pInventory->IterateInventory(&fnvd::callFunction<&apCallback>, pString, 0);
		*pString += "\"\t";
	} else {
		pString->SPrintF("\"\"\t");
	}
	pFile->Write(*pString, true);
	float fInventoryWeight;
	float fMaxCarryWeight;
	GetInventoryEncumbrance(fInventoryWeight, fMaxCarryWeight);
	pString->SPrintF("%.2f\t%.2f\t", static_cast<double>(fInventoryWeight), static_cast<double>(fMaxCarryWeight));
	pFile->Write(*pString, true);
	if (pInventory) {
		pString->SPrintF("%i\t", pInventory->GetInventoryValue(false, false));
	} else {
		pString->SPrintF("%i\t", 0);
	}
	pFile->Write(*pString, true);
	pString->SPrintF("%i\t", GetGoldCount());
	pFile->Write(*pString, true);
	// Slot 271 through the vtable.
	Actor* pActor = this;
	pString->SPrintF("%.2f\t", static_cast<double>(pActor->GetArmorRating()));
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
	} else if (pCell->GetInterior()) {
		uint32_t uiFormID = pCell->GetFormID();
		pString->SPrintF("%s (%08X)\t", pCell->TESFullName::GetFullName(), uiFormID);
	} else {
		TESWorldSpace* pWorldSpace = pCell->GetWorldSpace();
		uint32_t uiFormID = pCell->GetFormID();
		int32_t iY = pCell->GetDataY();
		int32_t iX = pCell->GetDataX();
		if (pWorldSpace) {
			pString->SPrintF("%s (%i, %i) (%08X)\t", pWorldSpace->TESFullName::GetFullName(), iX, iY, uiFormID);
		} else {
			pString->SPrintF("UNKNOWN (%i, %i) (%08X)\t", iX, iY, uiFormID);
		}
	}
	pFile->Write(*pString, true);
	if (aeEvent == DEATH) {
		ExtraDismemberedLimbs* pLimbs =
			static_cast<ExtraDismemberedLimbs*>(GetExtra()->GetExtraData(EXTRA_DISMEMBERED_LIMBS));
		if (pLimbs) {
			if (static_cast<int32_t>(pLimbs->eCauseofDeath) != -1) {
				pString->SPrintF("%s\t", CauseOfDeathNames[pLimbs->eCauseofDeath]);
			} else {
				pString->SPrintF("NONE\t");
			}
			pFile->Write(*pString, true);
			// The death object's line is built but never written, as the game does: the next write overwrites it.
			if (pLimbs->pDeathObject) {
				uint32_t uiFormID = pLimbs->pDeathObject->GetFormID();
				const char* pEditorID = pLimbs->pDeathObject->GetFormEditorID();
				const char* pType = pLimbs->pDeathObject->GetFormTypeString();
				pString->SPrintF("%s %s(%08X)\t", pType, pEditorID, uiFormID);
			} else {
				pString->SPrintF("NONE\t");
			}
		} else {
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
					pString->SPrintF("O:%s(%08X) T:%s(%08X)\t", pOriginalEditorID, uiOriginalID, pTemplateEditorID,
						uiTemplateID);
				} else if (pOriginal) {
					uint32_t uiOriginalID = pOriginal->GetFormID();
					pString->SPrintF("O:%s(%08X) T:UNKNOWN)\t", pOriginal->GetFormEditorID(), uiOriginalID);
				} else if (pTemplate) {
					uint32_t uiTemplateID = pTemplate->GetFormID();
					pString->SPrintF("O:UNKNOWN T:%s(%08X)\t", pTemplate->GetFormEditorID(), uiTemplateID);
				} else {
					pString->SPrintF("UNKNOWN LEV CREATURE\t");
				}
			} else {
				TESActorBase* pBase = pMyKiller->GetActorBase();
				uint32_t uiBaseID = pMyKiller->GetActorBase()->GetFormID();
				pString->SPrintF("%s (%08X)\t", pBase->GetFormEditorID(), uiBaseID);
			}
			pFile->Write(*pString, true);
			pString->SPrintF("%.2f\t", static_cast<double>(pMyKiller->GetHealth()));
			pFile->Write(*pString, true);
			TESObjectWEAP* pWeapon = pMyKiller->GetCurrentWeapon();
			if (pWeapon) {
				uint32_t uiWeaponID = pWeapon->GetFormID();
				pString->SPrintF("%s (%08X)\t", pWeapon->TESFullName::GetFullName(), uiWeaponID);
			} else {
				pString->SPrintF("NO WEAPON\t");
			}
			pFile->Write(*pString, true);
		} else {
			pString->SPrintF("UNKNOWN KILLER\t");
			pFile->Write(*pString, true);
		}
	}
	pString->SPrintF("\r\n");
	pFile->Write(*pString, true);
	pFile->Close();
	delete pFile;
	// SWEEP-INLINE 0x4037D0
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
	return 0;
}

AudioMarkerInfo* PlayerCharacter::GetClosestAudioMarkerInfo() {
	if (pClosestAudioMarkerInfo) {
		return pClosestAudioMarkerInfo;
	}
	if (AudioMarkerList.IsEmpty()) {
		return 0;
	}
	AudioMarkerInfo* pMarker = 0;
	float dist = 3.4028235e+38f;
	bool bInside = 0;
	bool bInsideCurrent = 0;
	BSSimpleList<AudioMarkerInfo*>* pList = &AudioMarkerList;
	while (pList && pList->m_item) {
		AudioMarkerInfo* pAudioMarker = pList->m_item;
		pList = pList->m_pkNext;
		if (pAudioMarker->pMarkerRef) {
			NiPoint3 Loc = pAudioMarker->pMarkerRef->GetLocationOnReference();
			const NiPoint3* v2 = &GetLocationOnReference();
			NiPoint3 Dist = Loc - *v2;
			float ndist = Dist.SqrXYLength();
			float rad = pAudioMarker->pMarkerRef->GetRadius();
			bInsideCurrent = ndist < rad * rad;
			if (!(dist > ndist)) {
				if (bInsideCurrent && !bInside) {
					dist = ndist;
					pMarker = pAudioMarker;
					bInside = bInsideCurrent;
				}
			} else {
				if (!bInside) {
					dist = ndist;
					pMarker = pAudioMarker;
					bInside = bInsideCurrent;
				}
			}
		}
	}
	pClosestAudioMarkerInfo = pMarker;
	return pMarker;
}

void PlayerCharacter::SetControlsDisabled(bool abDisable, PlayerCharacter::DISABLE_PLAYER_CONTROLS aeDPC) {
	uint8_t v3;
	if (abDisable) {
		v3 = (uint8_t)(aeDPC | ucControlsDisabled);
	} else {
		v3 = (uint8_t)((~((uint8_t)aeDPC)) & ucControlsDisabled);
	}
	SetControlsDisabled(v3);
}

void PlayerCharacter::SetControlsDisabled(uint8_t aucDisabled) {
	ucControlsDisabled = aucDisabled;
	if (ucControlsDisabled != 255) {
		ucControlsDisabled &= ~128;
	}
	if ((ucControlsDisabled & 1) != 0) {
		HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_MOVEMENT_DISABLED);
	} else {
		HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_CHANGED);
	}
	if ((ucControlsDisabled & 8) != 0) {
		SetWantWeaponDrawn(0);
	}
	if ((ucControlsDisabled & 16) != 0) {
		bWant3rdPerson = 0;
		if (sp1stPerson3D.m_pObject) {
			UpdateCamera(0, 0);
		} else {
			fChaseDistance = 0.0f;
		}
	}
	if ((ucControlsDisabled & 64) != 0) {
		uint16_t smoveMode = GetMoveMode() & 0xFBFF;
		pActorMover->ForceMoveMode(smoveMode);
	}
}

PlayerCharacter* PlayerCharacter::SetKillCamMode(KILL_CAM_TYPE eType) {
	eKillCameraSetting = static_cast<KILL_CAM_TYPE>(eType);
	return this;
}

void PlayerCharacter::ResetKillCam(KILL_CAM_TYPE eOldType, bool bForceReset) {
	appTimer.RestoreGlobalTimeMultiplierCache();
	bool v3 = Is3rdPerson();
	if (GetAnimation(!v3)) {
		bool v4 = Is3rdPerson();
		Animation* Animation_0 = GetAnimation(!v4);
		Animation_0->SetGlobalTimeMultiplier(1.0f);
	}
	if (eOldType == KILL_CAM_VATS && (bForceReset || !StartMenu::PauseMenuIsActive())) {
		pPlayer->SetControlsDisabled(0,
			static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(PlayerCharacter::DPC_MOVEMENT |
				PlayerCharacter::DPC_LOOKING));
		pVATS.QuitVATSPlayback(0, 1);
		pVATS.ClearData();
		pVATS.SetMode(VATS::VATS_MODE_NONE, 1);
		bVATSForce1stPerson = 1;
	}
	fTimeInSlowMoCam = 0.0f;
}

uint32_t PlayerCharacter::GetTotalPlayingTime() {
	UpdateTotalPlayingTime();
	return (uint32_t)((int32_t)iTotalPlayingTime);
}

void PlayerCharacter::UpdateTotalPlayingTime() {
	iTotalPlayingTime += (BSGetTickCount() - ((int32_t)iLastPlayingTimeUpdate));
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
	if (!bVanityMode && (&sp1stPerson3D)->operator->() && bWant3rdPerson && bWasWanted) {
		chaseCameraPos = p1stPersonCamera->GetWorldTranslate();
		this->Show1stPerson(!bWant3rdPerson);
	} else if (!bVanityMode) {
		if ((&sp1stPerson3D)->operator->()) {
			if ((&sp1stPerson3D)->operator->()->GetAppCulled()) {
				if (!bWant3rdPerson) {
					b3rdPerson = true;
				}
			}
		}
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
			pSelectedSpell->Unload(1);
		}
		pSelectedSpell = apSelectedSpell;
		if (pSelectedSpell) {
			pSelectedSpell->Preload(0);
		}
		ResetMagicCastSound();
	}
}

void PlayerCharacter::ResetPlayerGreetFlag() {
	bGreetingPlayer = (bool)0;
	fGreetTimer = (float)0.0;
}

void PlayerCharacter::CheckForClosesConversation(MobileObject* apSpeaker, MobileObject* apTarget,
	DialoguePackage* apDialogPack) {
	TESObjectREFR* actorStartedConversation;
	TESObjectREFR* cullingProcess;
	PlayerCharacter* v6;
	PlayerCharacter* v7;
	float v8;
	float fDistToPlayer = (float)apSpeaker->GetDistanceFromReference(pPlayer, 0, 0);
	float distanceFromReference = (float)apTarget->GetDistanceFromReference(pPlayer, 0, 0);
	if (fDistToPlayer > ((double)distanceFromReference)) {
		fDistToPlayer = distanceFromReference;
	}
	float v10 = 0x1.fffffe0000000p+127f;
	if (pPlayer->pClosestConversation) {
		if (pPlayer->pClosestConversation->GetActorStartedConversation()) {
			v6 = pPlayer;
			actorStartedConversation = pPlayer->pClosestConversation->GetActorStartedConversation();
			v10 = (float)actorStartedConversation->GetDistanceFromReference(v6, 0, 0);
		}
		if (pPlayer->pClosestConversation->DialoguePackage::GetTargetOfConversation()) {
			v7 = pPlayer;
			cullingProcess = pPlayer->pClosestConversation->DialoguePackage::GetTargetOfConversation();
			v8 = (float)cullingProcess->GetDistanceFromReference(v7, 0, 0);
			if (v10 > ((double)v8)) {
				v10 = v8;
			}
		}
	}
	if ((static_cast<const void*>(apTarget) == static_cast<const void*>(pPlayer)) ||
		(static_cast<const void*>(apSpeaker) == static_cast<const void*>(pPlayer))) {
		pPlayer->pClosestConversation = nullptr;
	} else {
		if (((!pPlayer->pClosestConversation) && (fDistToPlayer <= 1200.0)) ||
			(pPlayer->pClosestConversation && (fDistToPlayer <= ((double)v10)))) {
			pPlayer->pClosestConversation = apDialogPack;
		}
	}
}

void PlayerCharacter::AddTopics(BSSimpleList<TESTopic*>* apAddTopicList) {
	if (apAddTopicList) {
		char v5 = 0;
		for (BSSimpleList<TESTopic*>* pList = apAddTopicList; pList && pList->m_item; pList = pList->m_pkNext) {
			TESTopic** v2 = &pList->m_item;
			if (AddTopic(*v2, 0, 1)) {
				v5 = 1;
			}
		}
		if (v5) {
			BSSimpleList<TESTopic*>* TopicList = GetTopicList();
			TESTopic::SortTopics(1, TopicList);
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
			(apSpellIn->GetSpellType() == MagicSystem::POWER ||
				apSpellIn->GetSpellType() == MagicSystem::LESSER_POWER))) {
		for (BSSimpleList<EffectItem*>* plist = apSpellIn; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
			EffectItem* pitem = plist->m_item;
			if (pitem) {
				pitem->pEffectSetting->SetFlag(EffectSetting::PLAYER_KNOWS, 1);
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
	for (uint32_t i = static_cast<uint32_t>(apNode->GetArrayCount()); auiIndex < i;
		i = static_cast<uint32_t>(apNode->GetArrayCount())) {
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
	return (&spInventoryPC)->m_pObject;
}

void PlayerCharacter::SetShotsSinceLastAmmoRegenTick(uint8_t acShots) {
	cShotsSinceLastAmmoRegen = (int8_t)acShots;
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
	EnchantmentItem* v2;
	if ((!apScroll) || static_cast<TESEnchantableForm*>(apScroll)->TESEnchantableForm::GetFormEnchanting()) {
		if ((!pSelectedScroll) || (pSelectedScroll != apScroll)) {
			if (pSelectedScroll) {
				if (!apScroll) {
					SetSelectedSpell(nullptr);
				}
			}
			pSelectedScroll = apScroll;
			if (pSelectedScroll) {
				v2 = static_cast<TESEnchantableForm*>(pSelectedScroll)->TESEnchantableForm::GetFormEnchanting();
				if (v2) {
					SetSelectedSpell(static_cast<MagicItem*>(v2));
				} else {
					SetSelectedSpell(nullptr);
				}
			} else {
				SetSelectedSpell(nullptr);
			}
		}
	} else {
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
		} else if (bPassedCurrent || !pCurrentAmmo) {
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
		void* pMemory = ourMalloc(sizeof(ItemChange));
		ItemChange* pNewAmmo =
			pMemory ? ::new (pMemory) ItemChange(static_cast<TESBoundObject*>(pAmmo), iCount) : nullptr;
		pCurrentProcess->SetCurrentAmmo(pNewAmmo);
		pCurrentProcess->GetCurrentAmmo()->SetCount(0);
		int32_t iReloadAnim = (abStartReloadAnimation && IsWeaponDrawn()) ? 2 : 0;
		// 2 is WEAPON_MOD_INCREASE_CLIP_SIZE.
		bool bUseModValue = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(2);
		TESObjectWEAP* pReloadWeapon =
			static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetContainerObject());
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
		return 0;
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
		} else {
			float* pMinMult = &fFirstPersonZoomMinMult.Float();
			double dWheel = kVanityModePos.y - fVanityModeWheelMin.Float();
			float* pWheelMax = &fVanityModeWheelMax.Float();
			double dWheelFraction = dWheel / (*pWheelMax - fVanityModeWheelMin.Float());
			fZoomMult = static_cast<float>(fFirstPersonZoomMaxMult.Float() * dWheelFraction + *pMinMult);
		}
	}
	if (abSnapXY || bSnapChaseCamera) {
		kCameraPos = *apCameraPos;
		chaseCameraPos = kCameraPos;
	} else {
		kCameraPos = *apCameraPos;
		if (bIsAutoVanityMode) {
			float fSeconds = appTimer.GetSeconds();
			float fVanityXYMult = fChase3rdPersonVanityXYMult.Float();
			chaseCameraPos += (kCameraPos - chaseCameraPos) * fVanityXYMult * fSeconds;
			kCameraPos = chaseCameraPos;
		} else {
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
			} else {
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
	float* pUnitsPerSecond = &fChase3rdPersonZUnitsPerSecond.Float();
	float fStep = appTimer.GetSeconds() * *pUnitsPerSecond * fZoomMult;
	float fMaxDistance = fChaseCameraMax.Float();
	if (bVanityMode) {
		fMaxDistance = fVanityModeWheelMax.Float();
	}
	if (cZoom) {
		if (cZoom == 1 && fDistance < cZoom * fStep + fChaseDistance) {
			fChaseDistance = fDistance;
			fZoomDelta = 0.0f;
			cZoom = 0;
		} else {
			fChaseDistance = cZoom * fStep + fChaseDistance;
		}
		if (fMaxDistance < fChaseDistance) {
			fChaseDistance = fMaxDistance;
			fZoomDelta = 0.0f;
			cZoom = 0;
		} else if (fChaseDistance < 0.0) {
			fChaseDistance = 0.0f;
			fZoomDelta = 0.0f;
			cZoom = 0;
		}
	} else if (abSnapXY || bSnapChaseCamera) {
		fChaseDistance = fDistance;
		bSnapChaseCamera = false;
	} else {
		if (fChaseDistance >= fDistance) {
			fChaseDistance = fDistance;
		} else if (fDistance < fChaseDistance + fStep) {
			fChaseDistance = fDistance;
		} else {
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
		} else if (fZoom3rdPersonSnapDist.Float() > fChaseDistance) {
			fChaseDistance = fZoom3rdPersonSnapDist.Float();
		}
	} else if (!Interface::IsMenuIDVisible(Interface::eRaceSexMenu, 0) && pVATS.GetMode() != VATS::VATS_PLAYBACK) {
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
	} else {
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
	bWant3rdPerson = (bool)((uint8_t)ab3rdPerson);
	b3rdPerson = (bool)((uint8_t)ab3rdPerson);
}

bool PlayerCharacter::IsActually3rdPerson() {
	return bActually3rdPerson;
}

NiAVObject* PlayerCharacter::GetCurrent3D() {
	if (IsActually3rdPerson()) {
		return TESObjectREFR::Get3D();
	} else {
		return (&sp1stPerson3D)->operator->();
	}
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
	if (p1stPersonAnimation && p1stPersonAnimation->GetCurrentSequence(AGS_LEFT_ARM)) {
		if (p1stPersonAnimation->GetCurrentSequence(AGS_LEFT_ARM)->GetAnimGroup()->GetType() == ANIM_GROUP_PIPBOY ||
			p1stPersonAnimation->GetCurrentSequence(AGS_LEFT_ARM)->GetAnimGroup()->GetType() ==
				ANIM_GROUP_PIPBOY_CHILD) {
			return true;
		}
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
		if (!pTarget)
			break;
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
		if (!pTarget)
			break;
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

// Calls GetItem twice, as the game does.
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
	CompassTarget* pFound = 0;
	kPerceivedActorsLock.Lock(nullptr);
	for (CompassTarget* v4 : pListofPercievedActors->ItemsUntilNull()) {
		if (v4->pTarget == apActor) {
			pFound = v4;
			break;
		}
	}
	kPerceivedActorsLock.Unlock();
	return pFound;
}

// Under kPerceivedActorsLock (see PlayerCharacter.h).
bool PlayerCharacter::IsPerceivedActorDetected(Actor* apActor) {
	bool bDetected = 0;
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
	TESPackage* currentPackage;
	Actor* v4;
	BSSimpleList<Actor*>* ecx0;
	int32_t v6 = 0;
	ExtraDataList* extra = pPlayer->TESObjectREFR::GetExtra();
	ExtraFollower* followerExtra = extra->GetFollowerExtra();
	if (followerExtra) {
		if (followerExtra->pActorlist) {
			ecx0 = followerExtra->pActorlist;
			while (ecx0 && ecx0->m_item) {
				v4 = ecx0->m_item;
				if (v4 &&
					(v4->MobileObject::GetCurrentAIProcess() &&
						(v4->GetCurrentPackage() &&
							(((currentPackage = v4->GetCurrentPackage()),
								 (((int32_t)currentPackage->GetPackType()) == 1)) &&
								(static_cast<const void*>(v4->GetCurrentTarget()) ==
									static_cast<const void*>(pPlayer)))))) {
					++v6;
					ecx0 = ecx0->m_pkNext;
				} else {
					followerExtra->pActorlist->Remove(v4);
					ecx0 = followerExtra->pActorlist;
					v6 = 0;
				}
			}
		}
	}
	return v6;
}

bool PlayerCharacter::IsPlayerFollower(Actor* apActor) {
	ExtraDataList* Extra = pPlayer->GetExtra();
	ExtraFollower* FollowerExtra = Extra->GetFollowerExtra();
	if (FollowerExtra && FollowerExtra->pActorlist) {
		for (Actor*& rpItem : FollowerExtra->pActorlist->ItemsUntilNull()) {
			if (rpItem == apActor) {
				return 1;
			}
		}
	}
	return 0;
}

TESQuest* PlayerCharacter::GetActiveQuest() {
	return pActiveQuest;
}

void PlayerCharacter::SetActiveQuest(TESQuest* apActiveQuest) {
	if (apActiveQuest != pActiveQuest) {
		pActiveQuest = apActiveQuest;
		if (!pActiveQuest) {
			(&listQuestTargets)->RemoveAll();
		}
	}
}

int32_t PlayerCharacter::GetNumRockItAmmo() {
	int32_t v4 = 0;
	for (uint32_t uiIndex = 0; uiIndex < RockItLauncherAmmoList.iSize; ++uiIndex) {
		v4 += RockItLauncherAmmoList.GetAt(uiIndex)->iNumber;
	}
	return v4;
}

bool PlayerCharacter::IsInSlowMo() {
	return (bool)((uint8_t)(fTimeInSlowMoCam > 0.0));
}

// KEEP: when the pick hit no reference the exe still takes NiPoint3::Length (0x964FDB) of the vector at ebp-0x3B0,
// which only the hit branch writes (0x964F7C runs its empty constructor): an uninitialised read.
// The exe's cleanup-only C++ EH frame is left out.
void PlayerCharacter::UpdateAutoAimActor() {
	alignas(16) float v48;
	float fbestValue;
	alignas(16) CFilter fltr;
	alignas(16) NiMatrix3 kCameraRotMatrix;
	NiMatrix3 rotMod_2;
	NiMatrix3 playerRot;
	if (Get3D()) {
		bool v35;
		alignas(16) NiPoint3 v38;
		TESObjectWEAP* FormFlags;
		Actor* phitRef;
		CFilter icolFilter;
		Actor* pbestActor = 0;
		float v101 = fAutoAimMaxDistance.Float();
		NiCamera* Camera = Main::GetWorldSceneGraph()->GetCamera();
		float v99 = fAutoAimScreenPercentage.Float() * 0.004999999888241291;
		float v98 = -fAutoAimScreenPercentage.Float() * 0.004999999888241291;
		NiPoint2 screenTarget(v98, v99);
		MobileObject* pmob = 0;
		this->pPlayersTargetActor = 0;
		for (uint32_t i = 0;; ++i) {
			uint32_t ProcessLevelTail =
				(&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH);
			if (i >= ProcessLevelTail) {
				break;
			}
			pmob = (&ReferenceProcessLists.MobProcessArray)->GetItem(i);
			if (pmob) {
				if (pmob->IsActor() && !pmob->IsDead(0) && pmob->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
					if (pmob->Get3D()) {
						if (pPlayer->IsActorPerceived(static_cast<Actor*>(pmob))) {
							Actor* ptestActor = static_cast<Actor*>(pmob);
							const NiPoint3* z = &this->GetLocationOnReference();
							const NiPoint3* v2 = &ptestActor->GetLocationOnReference();
							NiPoint3 v93 = *v2 - *z;
							float v92 = v93.Length();
							bool bIsHostileToPlayer = pPlayer->IsActorPerceived(static_cast<Actor*>(pmob))->bIsHostile;
							if (v101 > static_cast<double>(v92)) {
								NiPoint3 npcMinY;
								NiPoint3 npcMaxX;
								alignas(16) NiPoint3 npcMinX;
								NiNode* v3 = pmob->Get3D();
								const NiBound* WorldBound = &v3->GetWorldBound();
								NiBound kBound = *WorldBound;
								NiPoint3 npcMaxY;
								Camera->ScreenSpaceBoundBound(kBound, npcMinY, npcMaxY, 1e-05f);
								kBound.SetRadius(ptestActor->GetBoundMax().x);
								Camera->ScreenSpaceBoundBound(kBound, npcMinX, npcMaxX, 1e-05f);
								char bShouldBeTargeted = 1;
								if (npcMinX.x == -1.0 && npcMaxX.x == 1.0) {
									bShouldBeTargeted = 0;
								} else {
									if (!(screenTarget.x > static_cast<double>(npcMinX.x)) ||
										!(screenTarget.x > static_cast<double>(npcMaxX.x))) {
										if (!(screenTarget.y < static_cast<double>(npcMinX.x)) ||
											!(screenTarget.y < static_cast<double>(npcMaxX.x))) {
											if (npcMinY.y == -1.0 && npcMaxY.y == 1.0) {
												bShouldBeTargeted = 0;
											} else {
												if (!(screenTarget.x > static_cast<double>(npcMinY.y)) ||
													!(screenTarget.x > static_cast<double>(npcMaxY.y))) {
													if (screenTarget.y < static_cast<double>(npcMinY.y) &&
														screenTarget.y < static_cast<double>(npcMaxY.y)) {
														bShouldBeTargeted = 0;
													}
												} else {
													bShouldBeTargeted = 0;
												}
											}
										} else {
											bShouldBeTargeted = 0;
										}
									} else {
										bShouldBeTargeted = 0;
									}
								}
								if (bShouldBeTargeted) {
									if (pPlayer->LineOfSight(0, ptestActor, 1, 0, 0)) {
										this->pPlayersTargetActor = ptestActor;
										if (bIsHostileToPlayer) {
											pbestActor = ptestActor;
											v101 = v92;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		Actor* v84 = pbestActor;
		bhkPickData pickData;
		SpecificItemCollector collector(L_LOS, pbestActor);
		if (pbestActor) {
			pickData.SetClosestCollector(&collector);
		}
		NiPoint3 origPoint = this->GetLocationOnReference();
		float* v6 = &fAutoAimMaxDistance.Float();
		NiPoint3 destPoint(0.0f, *v6, 0.0f);
		if (pVATS.eMode == VATS::VATS_PLAYBACK || PlayerCharacter::bVanityMode) {
			alignas(16) NiMatrix3 rotMod;
			double EyeHeight = this->GetEyeHeight();
			origPoint.z = EyeHeight + origPoint.z;
			float za = this->GetHeading(0);
			playerRot.MakeZRotation(za);
			float zb = this->GetLooking();
			rotMod.MakeXRotation(zb);
			playerRot = *::new (&rotMod_2) NiMatrix3(playerRot * rotMod);
			destPoint = playerRot * destPoint + origPoint;
		} else {
			alignas(16) float v72[3];
			NiMatrix3 rotMod_3;
			kCameraRotMatrix = Camera->GetWorldTransform().m_Rotate;
			kCameraRotMatrix.GetRow_005DF560(0, &v72[1], &v72[2], v72);
			rotMod_3.SetRow_005DF510(0, v72);
			kCameraRotMatrix.GetRow_005DF560(1, &v72[1], &v72[2], v72);
			rotMod_3.SetRow_005DF510(1, v72);
			kCameraRotMatrix.GetRow_005DF560(2, &v72[1], &v72[2], v72);
			rotMod_3.SetRow_005DF510(2, v72);
			origPoint = *this->GetCameraPosition();
			destPoint = rotMod_3 * destPoint + origPoint;
		}
		pickData.NiSetFrom(origPoint);
		pickData.NiSetTo(destPoint);
		this->GetCollisionFilter(&icolFilter.iFilter);
		fltr.iFilter = 0;
		fltr.SetLayer(L_LOS);
		uint32_t Group = icolFilter.GetGroup();
		fltr.SetGroup(Group);
		pickData.SetFilter(fltr);
		// The whole byte returned by the pick count override getter is passed back when restoring it.
		uint8_t ucPrevPickOverride = bhkWorld::QPickCountOverride();
		bhkWorld::SetPickCountOverride(1);
		NiAVObject* phit = TES::pInstance->Pick(pickData);
		bhkWorld::SetPickCountOverride(ucPrevPickOverride);
		if (phit) {
			phitRef = static_cast<Actor*>(TESObjectREFR::FindReferenceFor3D(phit));
		} else {
			phitRef = 0;
		}
		this->spTargeted3D.operator=(phit);
		if (phitRef) {
			v84 = phitRef;
		}
		float zc = pickData.m_hitFraction;
		this->BulletAutoAim = origPoint + (destPoint - origPoint) * zc;
		BaseProcess* CurrentAIProcess = this->pCurrentProcess;
		ItemChange* v59 = CurrentAIProcess->GetCurrentWeapon();
		if (v59) {
			FormFlags = static_cast<TESObjectWEAP*>(v59->pContainerObj);
		} else {
			FormFlags = 0;
		}
		if (pVATS.eMode != VATS::VATS_MENU_MODE && (phitRef && phitRef == pbestActor)) {
			NiPoint3 v53;
			NiPoint3 v54;
			NiNode* v56 = pbestActor->Get3D();
			fbestValue = fAutoAimMaxDistance.Float();
			InterfaceManager* v14 = InterfaceManager::Instance();
			int32_t ScriptProcessingDelay = static_cast<int32_t>(v14->sRealCursorPos.z);
			InterfaceManager* v15 = InterfaceManager::Instance();
			int32_t DataClipDist = static_cast<int32_t>(v15->sRealCursorPos.x);
			SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
			WorldSceneGraph->GetCamera()->WindowPointToRay(DataClipDist, ScriptProcessingDelay, v54, v53, 0);
			NiAVObject* v52 = ArrowProjectile::PickSceneGraph(v56, v54, v53, fbestValue, 0);
			if (v52) {
				TESObjectREFR* ReferenceFor3D = TESObjectREFR::FindReferenceFor3D(v52);
				if (ReferenceFor3D) {
					v84 = static_cast<Actor*>(ReferenceFor3D);
					if (ReferenceFor3D->IsActor()) {
						this->pPlayersTargetActor = static_cast<Actor*>(ReferenceFor3D);
					}
				}
			}
		} else {
			if (pVATS.eMode == VATS::VATS_MENU_MODE) {
				if (pVATSTarget) {
					if (FormFlags) {
						if (!FormFlags->IsMeleeWeapon() && !InterfaceManager::Has360Controller()) {
							NiPoint3 v46;
							NiPoint3 v47;
							NiNode* v50 = pVATSTarget->Get3D();
							const NiPoint3* zd = &pPlayer->Get3D()->GetWorldTranslate();
							NiNode* v17 = pVATSTarget->Get3D();
							v48 = (v17->GetWorldTranslate() - *zd).Length() + 128.0;
							InterfaceManager* v19 = InterfaceManager::Instance();
							int32_t v29 = static_cast<int32_t>(v19->sRealCursorPos.z);
							InterfaceManager* v20 = InterfaceManager::Instance();
							int32_t v27 = static_cast<int32_t>(v20->sRealCursorPos.x);
							SceneGraph* v21 = Main::GetWorldSceneGraph();
							v21->GetCamera()->WindowPointToRay(v27, v29, v47, v46, 0);
							NiAVObject* v45 = ArrowProjectile::PickSceneGraph(v50, v47, v46, v48, 0);
							if (v45) {
								Actor* v44 = static_cast<Actor*>(TESObjectREFR::FindReferenceFor3D(v45));
								eVATSTargetLimb = ActorValue::VALUE_NONE;
								if (v44->IsActor()) {
									BGSBodyPart::LIMB_ENUM DamageLocation = BGSBodyPart::LIMB_NONE;
									if (v45->IsNode()) {
										NiNode* v22 = v45->IsNode();
										DamageLocation = v44->GetDamageLocation(v22);
									} else {
										DamageLocation = v44->GetDamageLocation(v45->m_pkParent);
									}
									VATSMenu::SetLimb(DamageLocation);
									TESActorBase* ActorBase = v44->GetActorBase();
									BGSBodyPartData* v40 = ActorBase->GetBodyPartData();
									if (v40) {
										BGSBodyPart* BodyPart = v40->GetBodyPart(DamageLocation);
										if (BodyPart) {
											eVATSTargetLimb = static_cast<ActorValue::Index>(BodyPart->GetActorValue());
										}
									}
								}
							}
						}
					}
				}
			}
		}
		if (phitRef) {
			if (phitRef == pbestActor) {
				if (this->GetCurrentWeapon()) {
					TESObjectWEAP* CurrentWeapon = this->GetCurrentWeapon();
					if (!CurrentWeapon->IsMeleeWeapon()) {
						pbestActor = 0;
					}
				}
			}
		}
		this->pAutoAimActor = pbestActor;
		if (phitRef) {
			const NiPoint3* v24 = &phitRef->GetLocationOnReference();
			v38 = *v24 - origPoint;
		}
		float v36 = v38.Length();
		if (phitRef) {
			InterfaceManager* v25 = InterfaceManager::Instance();
			v35 = v36 > static_cast<double>(v25->iPickDistance);
		} else {
			v35 = 0;
		}
		this->bTarget3DDistant = v35;
		this->UpdateImageSpaceIronsights();
		this->UpdateImageSpaceVATS();
	}
}

Actor* PlayerCharacter::GetAutoAimActor() {
	if (pAutoAimActor && pAutoAimActor->Get3D()) {
		return pAutoAimActor;
	} else {
		return nullptr;
	}
}

void PlayerCharacter::ApplyAutoAimActor(Projectile* apProjectile, NiAVObject* apReleaseObj, float& apHeading,
	float& apLooking, NiPoint3* apOrigin, float afStartHeading, float afStartLooking) {
	TESObjectWEAP* v8;
	ItemChange* v9;
	TESObjectWEAP* v10;
	TESObjectWEAP* v11;
	const NiPoint3* v62;
	NiMatrix3 v67;
	NiPoint3 apResult;
	TESObjectREFR* phitRef;
	NiMatrix3 gunRot;
	NiPoint3 delta = NiPoint3_ZERO;
	VATS_COMMAND* pcurrentVATSAction = pVATS.GetCurrentAction();
	bool bvatsPlayback = 0;
	NiAVObject* pvatsTarget3d = 0;
	if (apOrigin) {
		v62 = apOrigin;
	} else {
		const NiPoint3* pNodePos;
		if (apReleaseObj) {
			pNodePos = &apReleaseObj->GetWorldTranslate();
		} else {
			pNodePos = &NiPoint3_ZERO;
		}
		v62 = pNodePos;
	}
	NiPoint3 startPos = *v62;
	if (!GetIronSights() ||
		(pVATS.eMode ||
			(!GetCurrentWeapon() ||
				(((v8 = GetCurrentWeapon(), !v8->HasModScope()) ||
					 (!pCurrentProcess ||
						 (!pCurrentProcess->GetCurrentWeapon() ||
							 (v9 = pCurrentProcess->GetCurrentWeapon(),
								 !v9->HasModEffectActive(WEAPON_MOD_IRON_SITES))))) &&
					((v10 = GetCurrentWeapon(), !v10->HasScope()) ||
						(v11 = GetCurrentWeapon(), v11->HasModScope())))))) {
		TESObjectWEAP* v12;
		if (pVATS.eMode == VATS::VATS_PLAYBACK) {
			if (pcurrentVATSAction) {
				if (pcurrentVATSAction->pTarget) {
					bvatsPlayback = 1;
					pvatsTarget3d = (NiAVObject*)pcurrentVATSAction->pTarget->Get3D();
				}
			}
		}
		float fadditionalMissHeading = 0.0f;
		float fadditionalMissLooking = 0.0f;
		if (!apReleaseObj || (!GetCurrentWeapon() || (v12 = GetCurrentWeapon(), v12->IsMeleeWeapon()))) {
			if (GetAutoAimActor()) {
				Actor* AutoAimActor = GetAutoAimActor();
				NiAVObject* v13 = Get3D(0);
				const NiPoint3* x = &v13->GetWorldBound().m_kCenter;
				NiNode* v14 = AutoAimActor->Get3D();
				const NiBound* WorldBound = &v14->GetWorldBound();
				delta = WorldBound->m_kCenter - *x;
			} else {
				if (BulletAutoAim.operator!=(NiPoint3_ZERO)) {
					NiAVObject* v16 = Get3D(0);
					const NiBound* v17 = &v16->GetWorldBound();
					delta = BulletAutoAim - v17->m_kCenter;
				}
			}
		LABEL_62:;
			bool v54 =
				apProjectile || (afStartLooking != 1.1754943508222875e-38 && afStartHeading != 1.1754943508222875e-38);
			if (delta.operator!=(NiPoint3_ZERO)) {
				TESObjectWEAP* CurrentWeapon;
				TESObjectWEAP* v31;
				float fheading;
				float flooking;
				float fmaxDegreeDelta;
				NiPoint3 kdirection = delta;
				kdirection.Unitize();
				if (!v54) {
					if (apReleaseObj) {
						gunRot = apReleaseObj->GetWorldTransform().m_Rotate;
					} else {
						gunRot = Get3D(0)->GetWorldTransform().m_Rotate;
					}
					NiMatrix3* v27 = gunRot.Inverse(&v67);
					kdirection = *v27 * kdirection;
					if (!bvatsPlayback) {
						kdirection = GunWobbleRot * kdirection;
					}
				}
				if (kdirection.y == 0.0) {
					fheading = 0.0f;
					flooking = 0.0f;
				} else {
					if (bvatsPlayback && (apProjectile && apProjectile->GetProjectileBase()->GetGravity() > 0.0)) {
						if (apProjectile->GetFormType() == PGRE_ID) {
							double v53 = delta.Length();
							float v52 = CombatUtilities::GetWorldGravity() * v53;
							float flowSpeed = std::sqrt(v52);
							float fhighSpeed = apProjectile->GetSpeed() * 1.25;
							float fspeedMult = 0.0f;
							for (int32_t i = 1; fspeedMult == 0.0 && i < 2; ++i) {
								bool bhighArc = i != 1;
								for (float fspeed = flowSpeed;
									fspeedMult == 0.0 && fhighSpeed >= static_cast<double>(fspeed);
									fspeed = (fhighSpeed - flowSpeed) * 0.1 + fspeed) {
									float fhitfraction = 0.0f;
									bool bhighArc_2 = 0;
									delta.z = delta.z + 10.0;
									float a1 = CombatUtilities::GetWorldGravity();
									float a6 = fspeed;
									NiPoint3 v126 = NiPoint3_ZERO;
									NiPoint3 v124 = delta + startPos;
									NiPoint3 arResult =
										CombatUtilities::GetProjectedPointForTarget(startPos, v124, v126, a6, a1, 0);
									phitRef = 0;
									NiPoint3 v127 = arResult;
									NiPoint3 v125 = startPos;
									float projectileSpeed = fspeed;
									BGSProjectile* ProjectileBase = apProjectile->GetProjectileBase();
									bool bresult = CombatUtilities::CalculateProjectileLOS(pPlayer, ProjectileBase,
										projectileSpeed, v125, v127, 0, &phitRef, 0);
									if (!bresult || phitRef == pcurrentVATSAction->pTarget) {
										double Speed = apProjectile->GetSpeed();
										fspeedMult = fspeed / Speed;
									}
								}
							}
							if (fspeedMult != 0.0) {
								double Time = apProjectile->fSpeedMult;
								float v51 = Time * fspeedMult;
								apProjectile->SetSpeedMult(v51);
							}
						}
						if (v54) {
							fheading = GetZAngleFromVector(kdirection);
							flooking = -CombatUtilities::FindAngleOfProjectile_0(apProjectile, delta, 0.0f, 0);
						} else {
							float v50 = kdirection.x / kdirection.y;
							fheading = std::atanf(v50);
							flooking =
								-CombatUtilities::FindAngleOfProjectile_0(apProjectile, delta, 0.0f, 0) - apLooking;
						}
					} else {
						if (v54) {
							fheading = GetZAngleFromVector(kdirection);
						} else {
							float v49 = kdirection.x / kdirection.y;
							fheading = std::atanf(v49);
						}
						flooking = -std::asinf(kdirection.z);
					}
				}
				float fheadingDelta = 0.0f;
				float flookingDelta = 0.0f;
				if (v54) {
					float fstartLooking;
					float v48;
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
					if (apProjectile) {
						v48 = apProjectile->GetHeading(0);
					} else {
						v48 = afStartHeading;
					}
					float fstartHeading = v48;
					if (apProjectile) {
						fstartLooking = apProjectile->GetLooking();
					} else {
						fstartLooking = afStartLooking;
					}
					float fstartLooking_2 = fstartLooking;
					while (fstartHeading < -3.1415927410125732) {
						fstartHeading = fstartHeading + 6.2831854820251465;
					}
					while (fstartHeading > 3.1415927410125732) {
						fstartHeading = fstartHeading - 6.2831854820251465;
					}
					if (fstartLooking_2 > 1.5533430576324463) {
						fstartLooking_2 = 1.553343f;
					}
					if (fstartLooking_2 < -1.5533430576324463) {
						fstartLooking_2 = -1.553343f;
					}
					fheadingDelta = fheading - fstartHeading;
					flookingDelta = flooking - fstartLooking_2;
				} else {
					fheadingDelta = fheading;
					flookingDelta = flooking;
				}
				while (fheadingDelta < -3.1415927410125732) {
					fheadingDelta = fheadingDelta + 6.2831854820251465;
				}
				while (fheadingDelta > 3.1415927410125732) {
					fheadingDelta = fheadingDelta - 6.2831854820251465;
				}
				if (!GetCurrentWeapon() || (CurrentWeapon = GetCurrentWeapon(), CurrentWeapon->IsMeleeWeapon())) {
					if (fheadingDelta < 0.0) {
						fheadingDelta = 0.0f;
					}
					flookingDelta = 0.0f;
				}
				if (GetCurrentWeapon() && (v31 = GetCurrentWeapon(), !v31->IsMeleeWeapon())) {
					if (bvatsPlayback) {
						fmaxDegreeDelta = fAutoAimMaxDegreesVATS.Float();
					} else {
						if (IsActually3rdPerson()) {
							fmaxDegreeDelta = fAutoAimMaxDegrees3rdPerson.Float();
						} else {
							fmaxDegreeDelta = fAutoAimMaxDegrees.Float();
						}
					}
				} else {
					fmaxDegreeDelta = fAutoAimMaxDegreesMelee.Float();
				}
				fmaxDegreeDelta = fmaxDegreeDelta * 0.01745329238474369;
				if (!(fmaxDegreeDelta < static_cast<double>(fheadingDelta))) {
					if (fheadingDelta < -fmaxDegreeDelta) {
						fheadingDelta = -fmaxDegreeDelta;
					}
				} else {
					fheadingDelta = fmaxDegreeDelta;
				}
				if (!(fmaxDegreeDelta < static_cast<double>(flookingDelta))) {
					if (flookingDelta < -fmaxDegreeDelta) {
						flookingDelta = -fmaxDegreeDelta;
					}
				} else {
					flookingDelta = fmaxDegreeDelta;
				}
				TESObjectWEAP* pweapon = GetCurrentWeapon();
				if (v54) {
					bool bUseSplit = 0;
					if (pCurrentProcess->GetCurrentWeapon()) {
						ItemChange* v32 = pCurrentProcess->GetCurrentWeapon();
						bUseSplit = v32->HasModEffectActive(WEAPON_MOD_SPLIT_BEAM);
					}
					if (bvatsPlayback) {
						if (pweapon) {
							if (pweapon->GetNumProjectiles(bUseSplit, 0, pPlayer) > 1u) {
								float AttackSkillModBase;
								float CurrentGameHour;
								if (apProjectile) {
									CurrentGameHour = apProjectile->fSpreadHeading;
								} else {
									CurrentGameHour = 0.0f;
								}
								float* v35 = &fVatsShotgunSpreadRatio.Float();
								fheadingDelta = *v35 * CurrentGameHour + fheadingDelta;
								if (apProjectile) {
									AttackSkillModBase = apProjectile->fSpreadLooking;
								} else {
									AttackSkillModBase = 0.0f;
								}
								float* v36 = &fVatsShotgunSpreadRatio.Float();
								flookingDelta = *v36 * AttackSkillModBase + flookingDelta;
							}
						}
					} else {
						float v45;
						float v46;
						NiPoint3 wobbleDirection;
						GunWobbleRot.GetCol(1u, wobbleDirection);
						double ZAngleFromVector = GetZAngleFromVector(wobbleDirection);
						fheadingDelta = ZAngleFromVector + fheadingDelta;
						double flooking_2 = std::asinf(wobbleDirection.z);
						flookingDelta = flookingDelta - flooking_2;
						if (apProjectile) {
							v46 = apProjectile->fSpreadHeading;
						} else {
							v46 = 0.0f;
						}
						fheadingDelta = fheadingDelta + v46;
						if (apProjectile) {
							v45 = apProjectile->fSpreadLooking;
						} else {
							v45 = 0.0f;
						}
						flookingDelta = flookingDelta + v45;
					}
				}
				if (bvatsPlayback && !pcurrentVATSAction->bSuccess) {
					float fmissMaxDegreeDelta = fAutoAimMaxDegreesMiss.Float() * 0.01745329238474369;
					if (!(fmissMaxDegreeDelta < static_cast<double>(fadditionalMissHeading))) {
						if (fadditionalMissHeading < -fmissMaxDegreeDelta) {
							fadditionalMissHeading = -fmissMaxDegreeDelta;
						}
					} else {
						fadditionalMissHeading = fmissMaxDegreeDelta;
					}
					if (!(fmissMaxDegreeDelta < static_cast<double>(fadditionalMissLooking))) {
						if (fadditionalMissLooking < -fmissMaxDegreeDelta) {
							fadditionalMissLooking = -fmissMaxDegreeDelta;
						}
					} else {
						fadditionalMissLooking = fmissMaxDegreeDelta;
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
			return;
		}
		if (!bvatsPlayback) {
			if (GetAutoAimActor()) {
				Actor* v55 = GetAutoAimActor();
				NiNode* v25 = v55->Get3D();
				const NiBound* v26 = &v25->GetWorldBound();
				delta = v26->m_kCenter - startPos;
			} else {
				if (BulletAutoAim.operator!=(NiPoint3_ZERO)) {
					delta = BulletAutoAim - startPos;
				}
			}
			goto LABEL_62;
		}
		if (pcurrentVATSAction->pTarget->IsActor() && (!apProjectile || apProjectile->GetFormType() != PGRE_ID)) {
			Actor* pactor = static_cast<Actor*>(pcurrentVATSAction->pTarget);
			TESActorBase* ActorBase = pactor->GetActorBase();
			BGSBodyPartData* pbodyPartData = ActorBase->GetBodyPartData();
			if (pbodyPartData) {
				ActorValue::Index eTargetBodyPart = pcurrentVATSAction->eLimb;
				BGSBodyPart* pbodyPart = pbodyPartData->GetBodyPart_005E5130(eTargetBodyPart);
				if (pbodyPart || eTargetBodyPart == ActorValue::VALUE_NONE) {
					const char* pname;
					if (eTargetBodyPart == ActorValue::VALUE_NONE) {
						pname = "Weapon";
					} else {
						pname = pbodyPart->GetTargetName();
					}
					NiNode* v18 = pactor->Get3D();
					NiAVObject* ptargetBodyPart3d = BSUtilities::GetObjectByName(v18, pname);
					if (ptargetBodyPart3d) {
						pvatsTarget3d = ptargetBodyPart3d;
					}
				}
			}
		}
		if (pvatsTarget3d) {
			NiPoint3 ktargetLocation = pvatsTarget3d->GetWorldTranslate();
			if (pcurrentVATSAction->bSuccess) {
				NiNode* pnode = pvatsTarget3d->IsNode();
				NiPoint3 localPoint = NiPoint3_ZERO;
				if (!apProjectile || apProjectile->GetFormType() != PGRE_ID) {
					localPoint = *TESHavokUtilities::GetLocalPointWithinCollision(&apResult, pvatsTarget3d);
				}
				if (apProjectile) {
					if (apProjectile->pTarget) {
						ProjectileTarget* ptarget = apProjectile->pTarget;
						ptarget->pReference = pcurrentVATSAction->pTarget;
						ptarget->spPart.operator=(pvatsTarget3d);
						ptarget->kOffset = localPoint;
					}
				}
				ktargetLocation = pvatsTarget3d->GetWorldTransform() * localPoint;
			} else {
				const NiPoint3* WorldTranslate;
				const NiBound* v19 = &pvatsTarget3d->GetWorldBound();
				if (v19->m_fRadius == 0.0) {
					WorldTranslate = &pvatsTarget3d->GetWorldTranslate();
				} else {
					WorldTranslate = &pvatsTarget3d->GetWorldBound().m_kCenter;
				}
				ktargetLocation = *WorldTranslate;
			}
			delta = ktargetLocation - startPos;
			if (!pcurrentVATSAction->bSuccess) {
				const NiBound* v20 = &pvatsTarget3d->GetWorldBound();
				float ftargetRadius = v20->m_fRadius;
				if (ftargetRadius == 0.0) {
					ftargetRadius = 16.0f;
				}
				float xa = fAutoAimMissRatioHigh.Float();
				float* v21 = &fAutoAimMissRatioLow.Float();
				double v22 = RandomFloat(*v21, xa);
				ftargetRadius = v22 * ftargetRadius;
				float fdistanceToTarget = delta.Length();
				float v56 = ftargetRadius * ftargetRadius + fdistanceToTarget * fdistanceToTarget;
				float fdistanceToMissPoint = NiSqrt(v56);
				float frandomAngle = RandomFloat(0.0f, 6.2831855f);
				double v23 = std::cos(frandomAngle);
				fadditionalMissHeading = v23 * (ftargetRadius / fdistanceToMissPoint);
				double v24 = std::sin(frandomAngle);
				fadditionalMissLooking = v24 * (ftargetRadius / fdistanceToMissPoint);
			}
			goto LABEL_62;
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
		} else {
			if (pQueuedTargetLoc->pWorld || pQueuedTargetLoc->pInterior) {
				if (pQueuedTargetLoc->pWorld) {
					PositionPlayerExterior(pQueuedTargetLoc->Location, pQueuedTargetLoc->Angle,
						pQueuedTargetLoc->pWorld, pQueuedTargetLoc->bResetWeather);
				} else {
					PositionPlayer(pQueuedTargetLoc->Location, pQueuedTargetLoc->Angle, pQueuedTargetLoc->pInterior,
						pQueuedTargetLoc->bResetWeather);
				}
				bhkCharacterController* pcon = nullptr;
				pcon = static_cast<bhkCharacterController*>(GetCharController());
				pcon->SetStartingHeight(pQueuedTargetLoc->Location.z);
				if (pQueuedTargetLoc->pfnArrivalFunc) {
					pQueuedTargetLoc->pfnArrivalFunc(pQueuedTargetLoc->iArrivalFuncData);
				}
				bretval = true;
			} else {
				Warning("AI: Invalid PositionPlayer request encountered. No movement will take place.");
			}
			if (pQueuedTargetLoc->pFurnitureRef && pQueuedTargetLoc->pFurnitureRef->IsFurniture()) {
				BaseProcess* pProcess = GetCurrentAIProcess();
				FurnitureMark* pmarker = pProcess->GetFurnitureMarker();
				int32_t index = -1;
				TESFurniture* pFurniture =
					static_cast<TESFurniture*>(pQueuedTargetLoc->pFurnitureRef->GetObjectReference());
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
	// The target world is fetched and never used, as the game does.
	[[maybe_unused]] TESWorldSpace* ptargetworld = apCell->GetWorldSpace();
	BGSEncounterZone* pnextzone = apCell->GetEncounterZone();
	BGSEncounterZone* pcurrzone = nullptr;
	if (GetParentCell()) {
		pcurrzone = GetParentCell()->GetEncounterZone();
	} else if (pcurrworld) {
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
	} else {
		if (apCell) {
			pLastKnownMusicType = apCell->GetMusicType(nullptr);
			if (BSTreeManager::GetUseCanopyShadows() && *apCell->GetWorldSpace()->GetCanopyShadowTextureName()) {
				BSTreeManager::GetCanopyShadow();
			} else {
				BSTreeManager::ClearCanopyShadow();
				TES::pInstance->GetGridCellA()->ClearCanopyShadowMaskTexture();
			}
			TES::pInstance->SetWorldSpace(apCell->GetWorldSpace());
		}
		// Leaving an interior: put out the pip-boy light.
		if (binterior) {
			if (IsSpellTarget(MagicSystem::GetPipBoyLightSpell(), true)) {
				Interface::GetPipboy()->ShowPipboyLightEffect(true, false, true);
				Interface::GetPipboy()->ShowPipboyLightEffect(false, false, true);
				Dispel(MagicSystem::GetPipBoyLightSpell(), nullptr, nullptr);
			}
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

	NiMatrix3 looking = NiMatrix3_IDENTITY;
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
	} else {
		rot.MakeXRotation(aAngle.x - aAngle.y);
	}
	if (Get3D()) {
		Get3D()->SetRotate(rot);
	}
	looking = rot * looking;
	Main::GetWorldSceneGraph()->GetCameraRoot()->SetRotate(looking);
	Main::GetWorldSceneGraph()->GetCameraRoot()->SetTranslate(aPos);
	NiUpdateData UpdateData;
	UpdateData.~NiUpdateData();
	::new (&UpdateData) NiUpdateData(0.0f, false, false);
	Main::GetWorldSceneGraph()->GetCameraRoot()->Update(UpdateData);
	TES::pInstance->GetRoot()->UpdateProperties();

	BSAudio::QInstance().PositionListener(aPos);
	// GetHeading/NiCos run before GetHeading/NiSin, as the game does.
	float fcos = NiCos(GetHeading(false));
	float fsin = NiSin(GetHeading(false));
	NiPoint3 front(fsin, fcos, 0.0f);
	static_cast<BSWin32Audio&>(BSAudio::QInstance()).SetOrientation(front, NiPoint3_UNIT_Z);
	CheckForQuestTargetUpdate(this);
	BSShaderAccumulator* paccum = BSShaderManager::GetAccumulator();
	if (paccum) {
		paccum->ClearAllBoundVolumes();
	}

	if (sp1stPerson3D.operator->()) {
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
				NiPoint3 delta = NiPoint3_ZERO;
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
	if (spCameraRigidBody.operator->()) {
		// Converts the position into hkv and then moves the body by the game-unit position, as the game does.
		hkVector4 hkv;
		NiPoint3 loc = GetLocationOnReference();
		NI2HK(hkv, loc);
		spCameraRigidBody.operator->()->SetPosition(loc);
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
	} else {
		HUDMainMenu::SetRegionText(nullptr);
	}
	ReferenceProcessLists.MoveFollowersOnToPlayer();
}

// Places the player in apWorld's exterior cell at aPos: cancels the exterior cell loads, then takes the cell at the
// position's cell coordinates (FloatToInt >> 12), else loads it, else has the data handler create it, and hands it to
// the global player's PositionPlayer.
void PlayerCharacter::PositionPlayerExterior(NiPoint3 aPos, NiPoint3 aAngle, TESWorldSpace* apWorld,
	bool abResetWeather) {
	int32_t a7;
	int32_t aiY;
	TESObjectCELL* pCell = nullptr;
	if (apWorld) {
		if (pExteriorCellLoader) {
			pExteriorCellLoader->CancelAllCellLoads();
		}
		a7 = FloatToInt(aPos.x) >> 12;
		aiY = FloatToInt(aPos.y) >> 12;
		pCell = apWorld->GetCellFromCellCoord(a7, aiY);
		ClearCellsIfTargetNotLoaded(pCell, apWorld);
		if (!pCell) {
			pCell = apWorld->LoadCell(a7, aiY);
		}
		if (!pCell) {
			pCell = pDataHandler->NewCell(nullptr, a7, aiY, apWorld);
		}
	}
	if (pCell) {
		pPlayer->PositionPlayer(aPos, aAngle, pCell, abResetWeather);
	}
}

void PlayerCharacter::FastTravel(TESObjectREFR* apRef) {
	FNVD_INLINED_LOCAL(PathingLocation, apLocation, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingLocation, apMemory, std::nullopt, std::nullopt);
	MemoryLevelManager::MEM_FREE_DATA a1;
	float ftotaldist;
	float fCheckPerk;
	fCheckPerk = 0.0f;
	BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_ALWAYS_FAST_TRAVEL, this, &fCheckPerk);
	if (!IsOverEncumbered() || fCheckPerk != 0.0) {
		NiPoint3 lastcoord;
		static int32_t lastFastTravelSaveTime = appTimer.iLastTime;
		uint32_t BUFSIZE = appTimer.iLastTime - lastFastTravelSaveTime;
		if (gs_bSaveOnTravel_GamePlay.Bool() && (!BUFSIZE || BUFSIZE > 0x4E20)) {
			pSaveLoadManager->Autosave();
			lastFastTravelSaveTime = appTimer.iLastTime;
		}
		pPlayer->btransporting = 1;
		MemoryLevelManager::BeginMemoryFree(&a1, 1, 1, 1);
		TES::pInstance->ClearAllCells(1, 0);
		MemoryLevelManager::FreeReleasedObjects(a1.bLockedFaceGen);
		MemoryLevelManager::EndMemoryFree(a1);
		static_cast<PlayerMover*>(pActorMover)->PlayerMover::ClearMoveMode(2048u);
		bhkCharacterController* CharController = GetCharController();
		CharController->SetWantState(HK_CHARACTER_ON_GROUND);
		GetOutofFurnitureQuick();
		ftotaldist = 0.0f;
		lastcoord.x = (&pPlayer->GetLocationOnReference())->x;
		lastcoord.y = (&pPlayer->GetLocationOnReference())->y;
		lastcoord.z = (&pPlayer->GetLocationOnReference())->z;
		ActorLockData playerLockData(pPlayer);
		PathingLocation* aAngle_8 = ::new (&apMemory) PathingLocation(apRef);
		PathingLocation* aAngle = ::new (&apLocation) PathingLocation(pPlayer);
		ftotaldist = Pathing::ComputeTeleportDoorPathLength(*aAngle, *aAngle_8, 0.0f, &playerLockData,
			Pathing::DOOR_LOCK_DISPOSITION_NORMAL);
		apLocation.~PathingLocation();
		apMemory.~PathingLocation();
		if (ftotaldist == 3.4028234663852886e+38) {
			char Dst[1024];
			int32_t v23 = 1024;
			BSsprintf(Dst, 1024, "FastTravel: Could not compute path length to Cell ");
			if (apRef->GetParentCell()) {
				BSStringT<char> v21;
				TESObjectCELL* ParentCell = apRef->GetParentCell();
				ParentCell->GetFormDetailedString(v21);
				BSstrcat(Dst, 1024, v21.pString);
			}
			ftotaldist = 0.0f;
			Warning(Dst);
		}
		TESGlobal* ptimescale = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(58u));
		double RunSpeed = pPlayer->GetRunSpeed();
		float ftime = ftotaldist / RunSpeed;
		ftime = ftime / 3600.0;
		double v4 = ptimescale->fValue;
		ftime = v4 * ftime;
		this->iSleepTime = static_cast<int32_t>(ftime);
		bIsSleeping = 0;
		TES::pInstance->ShowLoadingMenu(1, apRef, 0);
		if (this->iSleepTime) {
			int32_t iSleepTime = this->iSleepTime;
			while (this->iSleepTime > 0) {
				ftime = 3600.0 / appCalendar.GetTimeScale();
				double SystemTimeClock = ReferenceProcessLists.GetSystemTimeClock();
				float v13 = SystemTimeClock + ftime;
				ReferenceProcessLists.SetSystemTimeClock(v13);
				appCalendar.Update(ftime);
				ReferenceProcessLists.kUpdateLock.Lock(0);
				ReferenceProcessLists.UpdateHighListPackages(3600.0f, 1);
				ReferenceProcessLists.UpdateActorsMovement(3600.0f, 1, PROCESS_TYPE_HIGH);
				ReferenceProcessLists.UpdateMiddleHighList(3600.0f, 1);
				ReferenceProcessLists.UpdateMiddleLowList(3600.0f, 1);
				ReferenceProcessLists.UpdateLowList(3600.0f, 1);
				ReferenceProcessLists.ChangeProcessLevelTempList();
				ReferenceProcessLists.kUpdateLock.Unlock();
				--this->iSleepTime;
				pPlayer->RestoreHealth(2.0f);
			}
		} else {
			double v12 = ftime * 3600.0;
			ftime = v12 / appCalendar.GetTimeScale();
			double v6 = ReferenceProcessLists.GetSystemTimeClock();
			float v11 = v6 + ftime;
			ReferenceProcessLists.SetSystemTimeClock(v11);
			appCalendar.Update(ftime);
		}
		ReferenceProcessLists.MoveActorsToReference(apRef);
		if (TES::GetSelectivePurgeUnusedOnFastTravel()) {
			TES::pInstance->SetAllowUnusedPurge(0);
		}
		if (BSFaceGenManager::GetModelCache()) {
			BSFaceGenModelMap* ModelCache = BSFaceGenManager::GetModelCache();
			ModelCache->UnloadAllEGMAndEGTData(0);
		}
		MemoryPool::FreeUnusedPagesForAllPools();
		Sky* Instance = Sky::GetInstance();
		Instance->SetFastTravel(1);
		Interface::bIsFastTraveling = 1;
		TESWorldSpace* WorldSpace = apRef->GetWorldSpace();
		NiPoint3 v34 = apRef->GetAngleOnReference();
		NiPoint3 v33 = apRef->GetLocationOnReference();
		pPlayer->PositionPlayerExterior(v33, v34, WorldSpace, 1);
		Interface::bIsFastTraveling = 0;
		this->iSleepTime = 0;
		Sky* v9 = Sky::GetInstance();
		v9->SetFastTravel(0);
		if (TES::GetSelectivePurgeUnusedOnFastTravel()) {
			TES::pInstance->SetAllowUnusedPurge(1);
		}
		ReferenceProcessLists.PrintActorsInDialog();
		TES::pInstance->CreateFurnitureList();
		ReferenceProcessLists.PlaceActorsinBedsOrChairs();
		ReferenceProcessLists.ProcessHighActorsForFastTravel();
		pPlayer->btransporting = 0;
		ReferenceProcessLists.Update3DAfterResting();
		CheckTempModifiers();
		ResetMagicCastSound();
		pPlayer->bTravelUseDoor = 0;
	}
}

void PlayerCharacter::ClearCellsIfTargetNotLoaded(TESObjectCELL* apTarget, TESWorldSpace* apWorldSpace) {
	MemoryLevelManager::MEM_FREE_DATA freedata;
	if (!apTarget || !TES::pInstance->IsCellLoaded(apTarget, 0)) {
		NiNode* pnode;
		TESForm* plocation;
		if (apTarget) {
			plocation = (TESForm*)apTarget;
		} else {
			plocation = (TESForm*)apWorldSpace;
		}
		TES::pInstance->ShowLoadingMenu(1, plocation, 0);
		if (pPlayer->GetParentCell()) {
			pPlayer->GetParentCell()->RemoveReference(pPlayer);
		}
		NiAVObject* p3D = Get3D(1);
		if (p3D) {
			pnode = p3D->IsNode();
		} else {
			pnode = 0;
		}
		if (pnode) {
			if (pnode->m_pkParent) {
				NiNode* Parent = pnode->m_pkParent;
				Parent->DetachChild(p3D);
			}
		}
		MemoryLevelManager::BeginMemoryFree(&freedata, 0, 1, 0);
		TES::pInstance->ClearAllCells(0, 1);
		ProcessLists::bIsForciblyUpdated = 1;
		ReferenceProcessLists.UpdateProcessLists();
		ReferenceProcessLists.ChangeProcessLevelTempList();
		ProcessLists::bIsForciblyUpdated = 0;
		MemoryLevelManager::FreeReleasedObjects(freedata.bLockedFaceGen);
		MemoryLevelManager::EndMemoryFree(freedata);
		if (BSFaceGenManager::GetModelCache()) {
			BSFaceGenModelMap* ModelCache = BSFaceGenManager::GetModelCache();
			ModelCache->UnloadAllEGMAndEGTData(0);
		}
		MemoryPool::FreeUnusedPagesForAllPools();
	}
}

void PlayerCharacter::UpdateCamera(bool abForce, bool abSkipUpdateLOD) {
	NiPoint3 v84;
	NiPoint3 v85;
	NiPoint3 v88;
	NiPoint3 v90;
	NiMatrix3 cameraShake_2;
	NiPoint3 v97;
	NiPoint3 v98;
	NiPoint3 v100;
	NiPoint3 v102;
	NiPoint3 shoulderOffset_3;
	NiPoint3 v111;
	NiMatrix3 rotMod_3;
	NiPoint3 dst;
	NiMatrix3 xrot_2;
	NiMatrix3 rotMod_2;
	NiMatrix3 vatsCameraRot_2;
	NiPoint3 a2;
	NiPoint3 cameraPos;
	NiMatrix3 rotMod;
	NiPoint3 cameraRow;
	NiPoint3 currentPos;
	NiMatrix3 cameraRot;
	PlayerCharacter::fCameraHeading = GetHeading(0);
	PlayerCharacter::fCameraLooking = GetLooking();
	SceneGraph* pWorldRoot = Main::GetWorldSceneGraph();
	f1_0_ = 1.0f;
	static NiCamera chaseCamera;
	float fWorldFOV = this->fWorldFOV;
	SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
	WorldSceneGraph->SetCameraFOV(fWorldFOV, 0, 0, 0);
	BSShaderManager::SetFOV(this->fWorldFOV);
	if (pVATS.eMode == VATS::VATS_PLAYBACK && bWant3rdPerson) {
		NiMatrix3 vatsCameraRot;
		currentPos = chaseCameraPos;
		chaseCamera.SetTranslate(currentPos);
		NiUpdateData UpdateData(0.0f, 0, 0);
		chaseCamera.Update(UpdateData);
		NiPoint3* CamshotTargetPos = pVATS.GetCamshotTargetPos(&a2);
		chaseCamera.LookAtWorldPoint(*CamshotTargetPos, NiPoint3_UNIT_Z);
		rotMod = (*&chaseCamera.m_kLocal).m_Rotate;
		rotMod.GetRow(0, cameraRow);
		cameraRot.SetRow(0, cameraRow.z, cameraRow.x, cameraRow.y);
		rotMod.GetRow(1u, cameraRow);
		cameraRot.SetRow(1u, cameraRow.z, cameraRow.x, cameraRow.y);
		rotMod.GetRow(2u, cameraRow);
		cameraRot.SetRow(2u, cameraRow.z, cameraRow.x, cameraRow.y);
		vatsCameraRot.FromEulerAnglesXYZ(kVATSCameraAngle.x, kVATSCameraAngle.y, kVATSCameraAngle.z);
		cameraRot = *::new (&vatsCameraRot_2) NiMatrix3(cameraRot * vatsCameraRot);
	} else {
		if (b3rdPerson || (PlayerCharacter::bVanityMode || bWant3rdPerson != b3rdPerson)) {
			NiPoint3 shoulderOffset_2;
			NiPoint3 targetPos = Get3D(0)->GetWorldTranslate();
			float fYDir = 1000.0f;
			if (PlayerCharacter::bIsAutoVanityMode) {
				if (!(PlayerCharacter::kAutoVanityModePos.z < 0.0)) {
					if (PlayerCharacter::kAutoVanityModePos.z > 6.2831854820251465) {
						PlayerCharacter::kAutoVanityModePos.z =
							PlayerCharacter::kAutoVanityModePos.z - 6.2831854820251465;
					}
				} else {
					PlayerCharacter::kAutoVanityModePos.z = PlayerCharacter::kAutoVanityModePos.z + 6.2831854820251465;
				}
				float v81 = GetHeading(0) + PlayerCharacter::kAutoVanityModePos.z;
				cameraRot.MakeZRotation(v81);
				rotMod.MakeXRotation(PlayerCharacter::kAutoVanityModePos.x);
				targetPos.z = GetScale() * 100.0 + targetPos.z;
			} else {
				float f3rdPersonFOV = this->f3rdPersonFOV;
				SceneGraph* v10 = Main::GetWorldSceneGraph();
				v10->SetCameraFOV(f3rdPersonFOV, 0, 0, 0);
				BSShaderManager::SetFOV(this->f3rdPersonFOV);
				float v80 = PlayerCharacter::fCameraHeading + PlayerCharacter::kVanityModePos.z;
				cameraRot.MakeZRotation(v80);
				float v79 = PlayerCharacter::fCameraLooking + PlayerCharacter::kVanityModePos.x;
				rotMod.MakeXRotation(v79);
				double v78 = this->GetEyeHeight() * f1_0_;
				targetPos.z = GetScale() * v78 + targetPos.z;
				const NiPoint3* WorldTranslate = &p1stPersonCamera->GetWorldTranslate();
				float feyeDelta = WorldTranslate->z - (&p3rdPersonCamera->GetWorldTranslate())->z;
				targetPos.z = targetPos.z + feyeDelta;
			}
			float fshoulderMult = 1.0f;
			if (fOverShoulderStartBlendDist.Float() > 0.0) {
				fshoulderMult = 1.0 - PlayerCharacter::kVanityModePos.y / fOverShoulderStartBlendDist.Float();
				if (fshoulderMult < 0.0) {
					fshoulderMult = 0.0f;
				}
			}
			float v51 = fOverShoulderPosZ.Float();
			float* v12 = &fOverShoulderPosX.Float();
			NiPoint3* v13 = ::new (&dst) NiPoint3(*v12, 0.0, v51);
			NiPoint3 shoulderOffset = *v13 * fshoulderMult;
			float v62 = fOverShoulderPosZ.Float();
			float v77 = fYDir - PlayerCharacter::kVanityModePos.y;
			float* v14 = &fOverShoulderPosX.Float();
			NiPoint3 shoulderLookAt_2(*v14, v77, v62);
			char czoomingMode = bWant3rdPerson - b3rdPerson;
			cameraRot = *::new (&rotMod_3) NiMatrix3(cameraRot * rotMod);
			NiPoint3* v15 = ::new (&v111) NiPoint3(0.0, -PlayerCharacter::kVanityModePos.y, 0.0);
			cameraPos = targetPos + cameraRot * (*v15 + shoulderOffset);
			NiPoint3 cameraTarget = targetPos + cameraRot * shoulderLookAt_2;
			Setup3rdPersonCameraCaster();
			targetPos.operator+=(*::new (&shoulderOffset_2) NiPoint3(cameraRot * shoulderOffset));
			NiPoint3 newTargetPos(data.kPosition.x, data.kPosition.y, targetPos.z);
			UpdateChaseCamera(&cameraPos, &newTargetPos, abForce);
			NiPoint3* v19 = ::new (&shoulderOffset_3) NiPoint3(cameraRot * shoulderOffset);
			targetPos.operator-=(*v19);
			currentPos = cameraPos;
			shoulderLookAt_2 = targetPos + shoulderLookAt_2;
			kCamera3rdPersonShoulderOffset = cameraRot * shoulderOffset;
			if (fshoulderMult > 0.0) {
				NiPoint3 v94;
				NiMatrix3 shoulderRot;
				shoulderRot.MakeZRotation(PlayerCharacter::fCameraHeading);
				if (!(PlayerCharacter::fVanityDelay > 0.0)) {
					if (czoomingMode || (bCameraCasterHitObject && !PlayerCharacter::bVanityMode)) {
						float v64 = fOverShoulderOffsetPointZoomingZ.Float();
						float v54 = fOverShoulderOffsetPointZoomingY.Float();
						float* v24 = &fOverShoulderOffsetPointZoomingX.Float();
						shoulderOffset = *::new (&v98) NiPoint3(*v24, v54, v64);
					} else {
						float v65 = fOverShoulderOffsetPointZ.Float();
						float v55 = fOverShoulderOffsetPointY.Float();
						float* v25 = &fOverShoulderOffsetPointX.Float();
						shoulderOffset = *::new (&v97) NiPoint3(*v25, v55, v65);
					}
				} else {
					NiPoint3 v99;
					float fmult = PlayerCharacter::fVanityDelay / fVanityModeDelay.Float();
					if (fmult > 1.0) {
						fmult = 1.0f;
					}
					float v52 = fOverShoulderOffsetPointZ.Float();
					float v49 = fOverShoulderOffsetPointY.Float();
					float* v20 = &fOverShoulderOffsetPointX.Float();
					NiPoint3* v21 = ::new (&v102) NiPoint3(*v20, v49, v52);
					shoulderOffset = *v21 * fmult;
					float v76 = 1.0 - fmult;
					float v53 = fOverShoulderOffsetPointZoomingZ.Float();
					float v50 = fOverShoulderOffsetPointZoomingY.Float();
					float* v22 = &fOverShoulderOffsetPointZoomingX.Float();
					NiPoint3* v23 = ::new (&v100) NiPoint3(*v22, v50, v53);
					shoulderOffset.operator+=(*::new (&v99) NiPoint3(*v23 * v76));
				}
				rotMod.MakeXRotation(PlayerCharacter::fCameraLooking);
				shoulderOffset = rotMod * shoulderOffset;
				shoulderOffset = shoulderRot * shoulderOffset;
				float v75 = 1.0 - fOverShoulderRotMult.Float();
				targetPos.operator+=(*::new (&v94) NiPoint3(shoulderOffset * v75));
			}
			chaseCamera.SetTranslate(currentPos);
			NiUpdateData UpdateData_3(0.0f, 0, 0);
			chaseCamera.Update(UpdateData_3);
			if (PlayerCharacter::bIsAutoVanityMode) {
				chaseCamera.LookAtWorldPoint(targetPos, NiPoint3_UNIT_Z);
			} else {
				chaseCamera.LookAtWorldPoint(cameraTarget, NiPoint3_UNIT_Z);
			}
			rotMod = (*&chaseCamera.m_kLocal).m_Rotate;
			rotMod.GetRow(0, cameraRow);
			cameraRot.SetRow(0, cameraRow.z, cameraRow.x, cameraRow.y);
			rotMod.GetRow(1u, cameraRow);
			cameraRot.SetRow(1u, cameraRow.z, cameraRow.x, cameraRow.y);
			rotMod.GetRow(2u, cameraRow);
			cameraRot.SetRow(2u, cameraRow.z, cameraRow.x, cameraRow.y);
		} else {
			cameraRot.MakeZRotation(PlayerCharacter::fCameraHeading);
			rotMod.MakeXRotation(PlayerCharacter::fCameraLooking);
			cameraRot = *::new (&rotMod_2) NiMatrix3(cameraRot * rotMod);
			if (!IsInDialoguewithPlayer()) {
				NiMatrix3 xrot;
				xrot.MakeXRotation(-f1stPersonArmRot);
				rotMod = p1stPersonCamera->GetWorldTransform().m_Rotate;
				cameraRot = *::new (&xrot_2) NiMatrix3(rotMod * xrot);
			}
			if (p1stPersonCamera) {
				currentPos = p1stPersonCamera->GetWorldTranslate();
			}
			HighProcess* phighProcess = static_cast<HighProcess*>(pCurrentProcess);
			BipedAnim* Biped_0 = GetBiped(0);
			NiNode* ParentBone = Biped_0->GetParentBone(BIPED_BONE_HEAD);
			Animation* v6 = phighProcess->GetAnimation();
			float fblendValue = v6->GetSmallestHavokBlendValue(ParentBone);
			if (fblendValue < 1.0) {
				NiPoint3 posDelta = p3rdPersonCamera->GetWorldTranslate() - currentPos;
				float* v7 = &fFirstPersonCameraMult.Float();
				float v82 = *v7 * fblendValue;
				posDelta.operator*=(v82);
				currentPos.operator+=(posDelta);
				if (p1stPersonBip) {
					p1stPersonBip->SetTranslate(p1stPersonBip->GetTranslate() +
						(posDelta * (&sp1stPerson3D.m_pObject->m_kLocal)->m_Rotate));
					NiUpdateData UpdateData_2(0.0f, 0, 0);
					p1stPersonBip->Update(UpdateData_2);
				}
			}
			if (pCameraCaster) {
				if (pCameraCaster->GetWorld()) {
					if (pVATS.eMode != VATS::VATS_PLAYBACK) {
						pCameraCaster->Remove();
					}
				}
			}
		}
	}
	if (pVATS.eMode != VATS::VATS_PLAYBACK || (pVATS.pCamera && pVATS.pCamera->IsInitialized())) {
		NiMatrix3 cameraShake;
		Animation* Animation_ = Actor::GetAnimation();
		float Time = Animation_->GetTime();
		float fmult_2 = GetCameraShake(Time, cameraShake);
		if (fmult_2 > 0.0) {
			float fy;
			float fz;
			float fx;
			cameraShake.ToEulerAnglesXYZ(fx, fy, fz);
			float v74 = fz * fmult_2;
			float v73 = fy * fmult_2;
			float v72 = fx * fmult_2;
			cameraShake.FromEulerAnglesXYZ(v72, v73, v74);
			cameraRot = *::new (&cameraShake_2) NiMatrix3(cameraRot * cameraShake);
			HUDMainMenu::SetHUDShake(fmult_2);
		}
		NiNode* CameraRoot = pWorldRoot->GetCameraRoot();
		CameraRoot->SetTranslate(currentPos);
		NiNode* v28 = pWorldRoot->GetCameraRoot();
		v28->SetRotate(cameraRot);
		NiUpdateData UpdateData_4(0.0f, 0, 0);
		NiNode* v29 = pWorldRoot->GetCameraRoot();
		v29->Update(UpdateData_4);
	}
	NiPoint3 v162 = currentPos;
	BSAudio* v30 = &BSAudio::QInstance();
	v30->PositionListener(v162);
	NiPoint3 v163 = pWorldRoot->GetCamera()->GetWorldUpVector();
	NiPoint3 v161 = pWorldRoot->GetCamera()->GetWorldDirection();
	BSWin32Audio* v31 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
	v31->SetOrientation(v161, v163);
	TESObjectCELL* pcell = GetParentCell();
	if (pcell) {
		bool v66 = IsPointDeepUnderWater(currentPos.z, pcell) >= 0.03999999910593033;
		BSAudio* v32 = &BSAudio::QInstance();
		v32->SetListenerUnderwater(v66);
	}
	static NiPoint3 kLastGrassUpdatePos = NiPoint3_ZERO;
	static NiPoint3 kLastGrassUpdateDir = NiPoint3_ZERO;
	static NiPoint3 kLastLODUpdatePos = NiPoint3_ZERO;
	NiPoint3* v33 = ::new (&v90) NiPoint3(currentPos.x, currentPos.y, 0.0f);
	float v67 = (*v33 - kLastGrassUpdatePos).Length();
	float fmovedist = std::fabsf(v67);
	NiPoint3* v35 = ::new (&v88) NiPoint3(currentPos.x, currentPos.y, 0.0f);
	float v68 = (*v35 - kLastLODUpdatePos).Length();
	float fmovedistLOD = std::fabsf(v68);
	NiPoint3 kcurDir_2 = NiPoint3_ZERO;
	NiCamera* pworldcam_2 = pWorldRoot->GetCamera();
	const NiFrustum* ViewFrustum = &pworldcam_2->GetViewFrustum();
	double v71 = std::atanf(ViewFrustum->m_fRight);
	float fov = v71 / pworldcam_2->m_fLODAdjust;
	float theta = 1.0 - fov / 8.0;
	kcurDir_2 = pworldcam_2->GetWorldDirection();
	kcurDir_2.Unitize();
	float fviewdot = kLastGrassUpdateDir.Dot(kcurDir_2);
	if (fmovedist > 64.0 || (theta > static_cast<double>(fviewdot) || abForce)) {
		float v69 = std::acos(theta);
		TESTerrainLODManager::UpdateGrass(currentPos, kcurDir_2, v69);
		kLastGrassUpdatePos = *::new (&v85) NiPoint3(currentPos.x, currentPos.y, 0.0f);
		kLastGrassUpdateDir = kcurDir_2;
	}
	if (!abSkipUpdateLOD) {
		if (GetWorldSpace()) {
			TESWorldSpace* WorldSpace = GetWorldSpace();
			BGSTerrainManager* TerrainManager = WorldSpace->GetTerrainManager();
			if (TerrainManager->bNeedsImmediateUpdate) {
				const NiPoint3* v57 = &GetLocationOnReference();
				TESWorldSpace* v40 = GetWorldSpace();
				BGSTerrainManager* v41 = v40->GetTerrainManager();
				v41->Update(*v57, BGSTerrainManager::UPDATE_ALL);
			} else {
				if (fmovedistLOD > 512.0 || abForce) {
					const NiPoint3* v58 = &GetLocationOnReference();
					TESWorldSpace* v42 = GetWorldSpace();
					BGSTerrainManager* v43 = v42->GetTerrainManager();
					v43->Update(*v58, BGSTerrainManager::UPDATE_ALL);
					kLastLODUpdatePos = *::new (&v84) NiPoint3(currentPos.x, currentPos.y, 0.0f);
				}
			}
			const NiPoint3* v70 = &GetLocationOnReference();
			TESWorldSpace* v44 = GetWorldSpace();
			BGSTerrainManager* v45 = v44->GetTerrainManager();
			v45->UpdateMorphParameters(*v70);
		}
	}
	if (p1stPersonCamera) {
		Cached1stPersonCameraPos = p1stPersonCamera->GetWorldTranslate();
	}
	NiQuaternion quatid = NiQuaternion_IDENTITY;
	CachedWorldCameraPos = pWorldRoot->GetCamera()->GetWorldTranslate();
	spCameraRigidBody.m_pObject->ApplyHardKeyframe(CachedWorldCameraPos, quatid, 0);
	UpdateFirstPersonZoom();
}

// Turns the player's heading and looking from the mouse or the controller, or, in VATS, moves the camera toward the
// target (playback: aims at the current action's target limb). Returns whether the view was moved.
// KEEP: the exe reads an uninitialised local. In the VATS menu's limb test it reads screenPos.y after
// NiCamera::WorldPtToScreenPt (0xA6FC50), which leaves it unwritten when the limb is behind the camera (w at or
// below the zero tolerance: `jp 0xA6FDA0` before the first store).
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
						} else {
							imouseX = 0;
						}
					}
				} else {
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
					float fhorizLookSpeed = gs_fXenonHorizLookSpeed_Controls.Float();
					float fvertLookSpeed = gs_fXenonVertLookSpeed_Controls.Float();
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
			} else {
				imouseX = pcontrols->GetMouseAxis(1);
				imouseY = pcontrols->GetMouseAxis(2);
				imouseZ = pcontrols->GetMouseAxis(3);
			}
			if (gs_bInvertYValues_Controls.Bool()) {
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
						PlayerCharacter::kVanityModePos.y =
							PlayerCharacter::kVanityModePos.y - fVanityModeWheelOutMult.Float() * dzoom;
					} else {
						double dzoom = static_cast<double>(imouseZ) / 120.0 * PlayerCharacter::kVanityModePos.y;
						PlayerCharacter::kVanityModePos.y =
							PlayerCharacter::kVanityModePos.y - fVanityModeWheelInMult.Float() * dzoom;
					}
					if (IsDead(0)) {
						if (fVanityModeWheelDeadMin.Float() > static_cast<double>(PlayerCharacter::kVanityModePos.y)) {
							PlayerCharacter::kVanityModePos.y = fVanityModeWheelDeadMin.Float();
						}
					} else {
						if (fVanityModeWheelMin.Float() > static_cast<double>(PlayerCharacter::kVanityModePos.y)) {
							if (!GetUnderAIControl() && !PlayerCharacter::bVanityMode && !bTemp3rdPerson &&
								!GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
								SetFirstPerson(1);
							}
							PlayerCharacter::kVanityModePos.y = fVanityModeWheelMin.Float();
						}
					}
					float fMaxChaseDistance = fChaseCameraMax.Float();
					if (PlayerCharacter::bVanityMode) {
						fMaxChaseDistance = fVanityModeWheelMax.Float();
					}
					if (fMaxChaseDistance < static_cast<double>(PlayerCharacter::kVanityModePos.y)) {
						PlayerCharacter::kVanityModePos.y = fMaxChaseDistance;
					}
				}
				if (!GetUnderAIControl() && !bTemp1stPerson && !b3rdPerson && imouseZ < 0 &&
					!GetControlsDisabled(PlayerCharacter::DPC_POV_SWITCH)) {
					SetFirstPerson(0);
				}
			}
			float fdefaultFOV = BSSceneGraph::fDefaultWorldFOV.Float();
			float fcurrentFOV = Main::GetWorldSceneGraph()->GetCameraFOV();
			float ffovModMult = 1.0f;
			if (fdefaultFOV > static_cast<double>(fcurrentFOV)) {
				ffovModMult = fcurrentFOV / fdefaultFOV;
			}
			PlayerCharacter::fCameraHeading = GetHeading(0);
			PlayerCharacter::fCameraLooking = GetLooking();
			if ((PlayerCharacter::bVanityMode || PlayerCharacter::bZoom3rdPersonCamera) &&
				pCurrentProcess->GetFadeState() == BaseProcess::FADE_NORMAL) {
				double dangleX = static_cast<double>(imouseX) * 0.01745329238474369;
				PlayerCharacter::kVanityModePos.z =
					fVanityModeXMult.Float() * dangleX * afDelta + PlayerCharacter::kVanityModePos.z;
				if (!InterfaceManager::Has360Controller()) {
					double dangleY = static_cast<double>(imouseY) * 0.01745329238474369;
					PlayerCharacter::kVanityModePos.x =
						fVanityModeYMult.Float() * dangleY * afDelta + PlayerCharacter::kVanityModePos.x;
				}
			} else {
				if (imouseX || abHorse) {
					double dmouseX = static_cast<double>(imouseX);
					float fmouseX = fMouseSensitivity.Float() * dmouseX;
					if (apMoveMode) {
						if (imouseX && fmouseX < 0.0) {
							*apMoveMode |= ActorMover::MF_TURNLEFT;
						} else if (imouseX) {
							*apMoveMode |= ActorMover::MF_TURNRIGHT;
						}
					}
					if (!GetLifeState() && GetAnimAction() != ANIM_ACTION_STAGGER && !GetUnderAIControl()) {
						SIT_SLEEP_STATE esitState = GetSitSleepState();
						if (esitState == SIT_SLEEP_IS_SITTING) {
							fSitHeadingDelta = fSitHeadingDelta + fmouseX;
							if (!(fSitHeadingDelta < -1.5707963705062866)) {
								if (fSitHeadingDelta > 1.5707963705062866) {
									fSitHeadingDelta = 1.5707964f;
								}
							} else {
								fSitHeadingDelta = -1.5707964f;
							}
							if (abHorse) {
								PlayerCharacter::fPCTurnScale = 1.5f;
							}
						} else if (esitState == SIT_SLEEP_NORMAL) {
							double dabsMouseX = std::fabsf(fmouseX);
							PlayerCharacter::fPCTurnScale = dabsMouseX / (fMouseSensitivity.Float() * 24.0);
							if (PlayerCharacter::fPCTurnScale > 1.5) {
								PlayerCharacter::fPCTurnScale = 1.5f;
							}
							float fheadingMod = fmouseX * ffovModMult;
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
				} else {
					fVATSUserCameraZOffset = 0.0f;
				}
			}
			if (ptarget3d) {
				Actor* pactor = static_cast<Actor*>(ptarget);
				targetPos = pactor->Get3D()->GetWorldTranslate();
				if (ptarget->IsActor()) {
					static NiPoint3 center = NiPoint3_ZERO;
					// A new target: the camera centres on the target's head when it is within idistance of the
					// player, else on the mean of its limbs.
					if (bresetInterpolators) {
						BGSBodyPartData* pbodyPartData = pactor->GetActorBase()->GetBodyPartData();
						BGSBodyPart** pbodyPartArray = pbodyPartData->pPartArray;
						int32_t ilimbCount = 0;
						NiPoint3 totalLoc = NiPoint3_ZERO;
						NiAVObject* plimb = nullptr;
						if (pbodyPartArray[BGSBodyPart::LIMB_HEAD_1]) {
							const char* pname = pbodyPartData->GetBodyPart(BGSBodyPart::LIMB_HEAD_1)->GetTargetName();
							plimb = BSUtilities::GetObjectByName(pactor->Get3D(), pname);
						}
						if (plimb &&
							static_cast<double>(idistance) >
								(plimb->GetWorldTranslate() - pPlayer->Get3D()->GetWorldTranslate()).Length()) {
							++ilimbCount;
							NiPoint3 worldTranslate = plimb->GetWorldTranslate();
							totalLoc += worldTranslate;
						} else {
							for (int32_t eindex = 0; eindex < 15; ++eindex) {
								if (pbodyPartArray[eindex]) {
									const char* pname =
										pbodyPartData->GetBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(eindex))
											->GetTargetName();
									NiAVObject* ppartLimb = BSUtilities::GetObjectByName(pactor->Get3D(), pname);
									if (ppartLimb) {
										++ilimbCount;
										NiPoint3 worldTranslate = ppartLimb->GetWorldTranslate();
										totalLoc += worldTranslate;
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
					if (pVATS.eMode == VATS::VATS_MENU_MODE && pinterfaceManager->GetMouseVisible() &&
						pHeadingInterpolator->GetFinished() && pLookingInterpolator->GetFinished()) {
						[[maybe_unused]] int32_t icursorX = pinterfaceManager->GetCursorX();
						int32_t icursorY = pinterfaceManager->GetCursorY();
						if (icursorY >= 0 && InterfaceManager::GetScreenHeight() >= static_cast<double>(icursorY)) {
							float fmoveCameraPercent = fVATSMoveCameraYPercent.Float();
							float fcursorPercent = static_cast<double>(icursorY) / InterfaceManager::GetScreenHeight();
							bUseMouseForCameraOffset = true;
							float fmaxZ = 0.0f;
							float fminZ = 0.0f;
							BGSBodyPartData* pbodyPartData =
								static_cast<Actor*>(ptarget)->GetActorBase()->GetBodyPartData();
							BGSBodyPart** pbodyPartArray = pbodyPartData->pPartArray;
							for (int32_t eindex = 0; eindex < 15; ++eindex) {
								if (pbodyPartArray[eindex]) {
									NiPoint2 screenPos;
									const char* pname =
										pbodyPartData->GetBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(eindex))
											->GetTargetName();
									NiAVObject* plimb = BSUtilities::GetObjectByName(ptarget->Get3D(), pname);
									if (plimb) {
										Main::GetWorldSceneGraph()
											->GetCamera()
											->WorldPtToScreenPt(plimb->GetWorldTranslate(), screenPos.x, screenPos.y,
												1e-05f);
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
							NiPoint3 rot = CombatUtilities::GetAngleToProjectedPoint(Main::GetWorldSceneGraph()
																						 ->GetCamera()
																						 ->GetWorldTranslate(),
								targetPos);
							if (fmoveCameraPercent > fcursorPercent) {
								fVATSUserCameraZOffset = fmaxZ - targetPos.z;
							} else if (fcursorPercent > 1.0 - fmoveCameraPercent) {
								fVATSUserCameraZOffset = fminZ - targetPos.z;
							} else if ((rot.x < GetLooking() &&
										   fcursorPercent < fmoveCameraPercent + fmoveCameraPercent) ||
								(rot.x > GetLooking() &&
									fcursorPercent > 1.0 - (fmoveCameraPercent + fmoveCameraPercent))) {
								fVATSUserCameraZOffset = fcenteredCameraOffset;
							} else {
								bUseMouseForCameraOffset = false;
							}
						}
					}
					// Otherwise, once the camera has settled on a selected limb (or the weapon bone), keep that limb
					// on screen.
					if (!bUseMouseForCameraOffset && pVATS.eMode != VATS::VATS_SCAN_MODE &&
						pVATS.eMode != VATS::VATS_TARGET_SELECT && bFOVTargetReached &&
						pHeadingInterpolator->GetFinished() && pLookingInterpolator->GetFinished()) {
						BGSBodyPartData* pbodyPartData = pactor->GetActorBase()->GetBodyPartData();
						if (pbodyPartData) {
							BGSBodyPart* pbodyPart = pbodyPartData->GetBodyPart_005E5130(etargetLimb);
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
							} else {
								plimb = pweaponBone;
							}
							if (plimb) {
								NiPoint3 limbPos = plimb->GetWorldTranslate();
								NiPoint2 screenPos;
								Main::GetWorldSceneGraph()->GetCamera()->WorldPtToScreenPt(limbPos, screenPos.x,
									screenPos.y, 1e-05f);
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
			} else {
				targetPos = ptarget->GetLocationOnReference();
			}
			if (!(center_guard & 2)) {
				center_guard |= 2;
				flastCameraZ = targetPos.z;
			}
			if (targetPos.z != flastCameraZ && pHeadingInterpolator->GetFinished() &&
				pLookingInterpolator->GetFinished()) {
				flastCameraZ = targetPos.z;
				bresetInterpolators = true;
			}
			float fheadingDelta = 0.0f;
			float flookingDelta = 0.0f;
			NiPoint3 rot =
				CombatUtilities::GetAngleToProjectedPoint(Main::GetWorldSceneGraph()->GetCamera()->GetWorldTranslate(),
					targetPos);
			fheadingDelta = rot.z - GetHeading(0);
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
			rot.z = GetHeading(0) + fheadingDelta;
			rot.x = GetLooking() + flookingDelta;
			if (bresetInterpolators) {
				float fcamTime = 0.0f;
				if (pVATS.eMode == VATS::VATS_TARGET_SELECT) {
					fcamTime = fVATSTargetSelectCamPanTime.Float();
				} else if (pVATS.eMode == VATS::VATS_MENU_MODE) {
					if (blimbSelect) {
						fcamTime = fVATSLimbSelectCamPanTime.Float();
					} else {
						fcamTime = fVATSCamZoomInTime.Float();
					}
				} else {
					fcamTime = 0.75f;
				}
				pHeadingInterpolator->Setup(GetHeading(0), rot.z, fcamTime);
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
			} else {
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
			NiPoint3 targetPos;
			bool btargetSet = false;
			if (etargetLimb != ActorValue::VALUE_NONE && ptarget->IsActor()) {
				Actor* pactor = static_cast<Actor*>(ptarget);
				BGSBodyPartData* pbodyPartData = pactor->GetActorBase()->GetBodyPartData();
				if (pbodyPartData) {
					BGSBodyPart* pbodyPart = pbodyPartData->GetBodyPart_005E5130(etargetLimb);
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
				} else {
					targetPos = ptarget->GetLocationOnReference();
				}
			}
			if (pCurrentProcess->IsCurrentWeaponGrenade()) {
				double ddistance = GetDistanceFromPoint(targetPos);
				targetPos.z = fVATSGrenadeDistAimZMult.Float() * ddistance + targetPos.z;
			}
			NiPoint3 rot;
			CombatUtilities::GetAngleToProjectedPoint(&rot, this, targetPos);
			SetHeading(rot.z);
			SetLooking(rot.x);
		}
		pVATS.Update();
		*apMoveMode = static_cast<uint16_t>(GetMoveMode() &
			(ActorMover::MF_SNEAKING | ActorMover::MF_SWIMMING | ActorMover::MF_FALL | ActorMover::MF_SLIDE));
		bretVal = true;
		break;
	}
	default:
		break;
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
	} else if (!pCell->GetInterior()) {
		pworld = pCell->GetWorldSpace();
		if (pworld) {
			ClearCellsIfTargetNotLoaded(pCell, pworld);
			int32_t iDataY = pCell->GetDataY();
			int32_t iDataX = pCell->GetDataX();
			pworld->LoadCell(iDataX, iDataY);
		}
	}
	if (pCell) {
		NiPoint3 rot = NiPoint3_ZERO;
		pCell->GetCOCPlacementInfo(pos, rot);
		PositionPlayer(pos, rot, pCell, true);
		if (!pCell->GetInterior()) {
			float z;
			TES::pInstance->GetLandHeight(pos, z);
			// A negative or NaN height puts the player at 0.
			if (z >= 0.0) {
				pos.z = z;
			} else {
				pos.z = 0.0f;
			}
			SetPosition(pos);
		}
	}
}

bool PlayerCharacter::GetFlag66DIs2() {
	return (static_cast<int8_t>(bCanFastTravel) & 2) != 0;
}

// Each refusal shows its text for 2 seconds with emotion 0, the sad vault boy icon and no sound, and returns
// ShowMessage's result. When nothing forbids waiting it opens the wait menu.
bool PlayerCharacter::CanPassTime() {
	if (IsTresPassing()) {
		return Interface::ShowMessage(sNoWaitTrespass.String(), 0,
			"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false);
	}
	if (IsAlarmForPlayerCharactaer()) {
		return Interface::ShowMessage(sNoWaitWhileAlarmSounding.String(), 0,
			"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false);
	}
	// GetParentCell is called before the GetLocationOnReference virtual.
	TESObjectCELL* pCell = GetParentCell();
	const NiPoint3* pLocation = &GetLocationOnReference();
	if (IsPointDeepUnderWater(*pLocation, pCell, 0.875f)) {
		return Interface::ShowMessage(sNoWaitUnderWater.String(), 0,
			"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false);
	}
	if (ReferenceProcessLists.AreHostilActorsNear(pPlayer->GetInterior())) {
		return Interface::ShowMessage(sNoWaitHostilActorsNear.String(), 0,
			"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false);
	}
	if (static_cast<bhkCharacterController*>(pPlayer->GetCharController())->GetState() == HK_CHARACTER_JUMPING ||
		static_cast<bhkCharacterController*>(pPlayer->GetCharController())->GetState() == HK_CHARACTER_IN_AIR) {
		return Interface::ShowMessage(sNoWaitInAir.String(), 0,
			"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false);
	}
	if (pPlayer->GetParentCell()->GetCantWaitHere() || !pPlayer->GetCanWait()) {
		return Interface::ShowMessage(sNoWaitInCell.String(), 0,
			"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false);
	}
	// Both radiation tests show the same message.
	if (ReferenceProcessLists.IsPlayerInRadiationArea() ||
		(GetCurrentAIProcess() && GetCurrentAIProcess()->GetRadiationWaterDelta() != 0.0)) {
		return Interface::ShowMessage(sNoWaitInRadiation.String(), 0,
			"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false);
	}
	if (pPlayer->HasDamageHealthEffect()) {
		return Interface::ShowMessage(sNoWaitTakingHealthDamage.String(), 0,
			"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false);
	}
	Interface::CreateSleepMenu(true);
	return false;
}

bool PlayerCharacter::IsAlarmForPlayerCharactaer() {
	void* plist = ReferenceProcessLists.IsActorTargetingREFinPackage(this, PACKAGE_TYPE_ALARM, 0);
	if (plist) {
		delete static_cast<BSSimpleList<ActiveEffect*>*>(plist);
	}
	return plist != 0;
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
	NiPoint3 dst;
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	NiMatrix3 currentMatrix;
	// Move mode, passed by address to UpdateHeadingAndLooking.
	uint16_t smoveMode;
	AutoMemContext tempMemContext(MEM_CONTEXT_PLAYER, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 0x17A3);
	Controls* pcontrols = appMain->GetControls();
	SceneGraph* pWorldRoot = Main::GetWorldSceneGraph();
	if (pCurrentProcess->GetCurrentWeapon()) {
		pweapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
	} else {
		pweapon = 0;
	}
	// A byte of its own that is stored and never read, as the game does.
	[[maybe_unused]] bool buserInput = 0;
	smoveMode = 0;
	fDropAngleMod = 0.0f;
	fLastDropAngleMod = 0.0f;
	bOnElevator = 0;
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
			BaseProcess* CurrentAIProcess = pCurrentProcess;
			CurrentAIProcess->RandomlyPlaySpecialIdles(this);
		}
		uint16_t slastMoveMode = GetMoveMode();
		smoveMode = slastMoveMode & 0xCC00;
		if (pVATS.eMode) {
			if (UpdateHeadingAndLooking(afDelta, 0, &smoveMode, 0)) {
				buserInput = 1;
			}
		}
		int32_t imouseX = 0;
		int32_t v50 = 0;
		int32_t v54 = 0;
		PlayerCharacter::fCameraHeading = GetHeading(0);
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
						((!(fPlayerWeaponReloadTimer.Float() < static_cast<double>(fReloadTimer)) &&
							 (pweapon && pweapon->GetCurrentAmmo(pPlayer))) ||
							!GetWantWeaponDrawn())) {
						if (!bDebounce && !GetWantWeaponDrawn()) {
							if ((GetMoveMode() & 2048) == 0) {
								SetWantWeaponDrawn(1);
							}
							bDebounce = 1;
						}
					} else {
						SetWantWeaponDrawn(0);
						bDebounce = 1;
					}
				}
			}
		} else {
			if (!pcontrols->GetUserAction(Controls::UACTN_READY_RELOAD_WEAPON, 2)) {
				fReloadTimer = 0.0f;
				bDebounce = 0;
			}
		}
		if (pcontrols->GetUserAction(Controls::UACTN_AUTOMOVE, 1)) {
			bAutoMove = !bAutoMove;
		}
		if (bAutoMove) {
			if (pcontrols->GetUserAction(Controls::UACTN_FORWARD, 0) ||
				(pcontrols->GetUserAction(Controls::UACTN_BACK, 0) ||
					(pcontrols->GetUserAction(Controls::UACTN_SLDRIGHT, 0) ||
						pcontrols->GetUserAction(Controls::UACTN_SLDLEFT, 0)))) {
				bAutoMove = 0;
			} else {
				pcontrols->ForceUserAction(Controls::UACTN_FORWARD);
			}
		}
		char v53 = 0;
		if (pcontrols->GetUserAction(Controls::UACTN_CROUCH, 1)) {
			if (!IsParalyzed() && !IsDead(0) && !IsKnockedOut() && !IsRestrained() && !IsUnConscious() &&
				GetSitSleepState() == SIT_SLEEP_NORMAL) {
				ActionPoints::Action eaction = ActionPoints::ACTION_STAND;
				if ((smoveMode & 1024) != 0) {
					smoveMode = smoveMode & 0xFBFF;
					BSAudio* v3 = &BSAudio::QInstance();
					BSSoundHandle* SoundHandleByName = v3->GetSoundHandleByName(&a1, "NPCHumanCrouchUp",
						BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
							BSGameSound::SOUND_FLAG_FIRST_PERSON);
					SoundHandleByName->Play(0);
				} else {
					smoveMode = smoveMode | 1024;
					eaction = ActionPoints::ACTION_CROUCH;
					BSAudio* v4 = &BSAudio::QInstance();
					BSSoundHandle* v23 = v4->GetSoundHandleByName(&arHandle, "NPCHumanCrouchDown",
						BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
							BSGameSound::SOUND_FLAG_FIRST_PERSON);
					v23->Play(0);
				}
				UseActionPoints(eaction);
			}
			v53 = 1;
		}
		currentMatrix = (*&pWorldRoot->GetCameraRoot()->m_kLocal).m_Rotate;
		NiPoint3 v55 = *QLocationOnReference();
		NiPoint3 sideVec = *::new (&dst) NiPoint3(1.0f, 0.0f, 0.0f);
		currentMatrix.GetCol(1u, forwardVec);
		if (GetNoCollision()) {
			currentMatrix.GetCol(0, sideVec);
		} else {
			forwardVec.z = 0.0f;
			forwardVec.Unitize();
		}
		bool bdropping = 0;
		if (eGrabType == PlayerCharacter::GRAB_INVENTORY_DROP) {
			bdropping = UpdateDropSpring(afDelta);
		}
		if (ijoyX) {
			if (ijoyX <= 0) {
				pcontrols->ForceUserAction(Controls::UACTN_SLDLEFT);
			} else {
				pcontrols->ForceUserAction(Controls::UACTN_SLDRIGHT);
			}
			double v22 = static_cast<double>(std::abs(ijoyX));
			PlayerCharacter::fPCTurnScale = fPlayerCharacterTurnMult.Float() * v22;
			if (PlayerCharacter::fPCTurnScale > 1.5) {
				PlayerCharacter::fPCTurnScale = 1.5f;
			}
		}
		int32_t v5 = std::abs(ijoyY);
		if (v5 >= std::abs(ijoyX)) {
			if (ijoyY) {
				if (ijoyY >= 0) {
					pcontrols->ForceUserAction(Controls::UACTN_BACK);
				} else {
					pcontrols->ForceUserAction(Controls::UACTN_FORWARD);
				}
				PlayerCharacter::fWalkSpeedMult = static_cast<double>(std::abs(ijoyY)) * 0.019999999552965164;
			}
		} else {
			PlayerCharacter::fWalkSpeedMult = static_cast<double>(std::abs(ijoyX)) * 0.019999999552965164;
			if (ijoyX >= 0) {
				smoveMode = smoveMode | 8;
			} else {
				smoveMode = smoveMode | 4;
			}
		}
		if (PlayerCharacter::fWalkSpeedMult > 1.0) {
			PlayerCharacter::fWalkSpeedMult = 1.0f;
		}
		float CurrentSpeed = GetCurrentSpeed();
		float v21 = CurrentSpeed * PlayerCharacter::fWalkSpeedMult;
		sideVec.operator*=(v21);
		float v20 = CurrentSpeed * PlayerCharacter::fWalkSpeedMult;
		forwardVec.operator*=(v20);
		pActorMover->ForceMoveMode(smoveMode);
		if (v53) {
			SetWalkRunFlagDirty();
		}
		b3rdPerson = 1;
		bool bdisabled = 0;
		if (IsKnockedOut() ||
			(IsParalyzed() || (GetLifeState() || (GetAnimAction() == ANIM_ACTION_STAGGER || GetSitSleepState())))) {
			bdisabled = 1;
		}
		if (bdisabled || GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
			static_cast<PlayerMover*>(pActorMover)->PlayerMover::SetMoveMode(831u, 0);
		}
		if (!bdisabled && !GetControlsDisabled(PlayerCharacter::DPC_FIGHTING)) {
			if (eGrabType == PlayerCharacter::GRAB_INVENTORY_DROP || eGrabType == PlayerCharacter::GRAB_TELEKINESIS) {
				inextAttack = 0;
				fpowerTimer = 0.0f;
			} else {
				if (pCurrentProcess->CanAttack()) {
					if (CheckUserInputAttacks()) {
						buserInput = 1;
					}
				} else {
					inextAttack = 0;
					fpowerTimer = 0.0f;
				}
				if ((pcontrols->GetUserAction(Controls::UACTN_BLOCK, 1) ||
						pcontrols->GetUserAction(Controls::UACTN_BLOCK, 0)) &&
					(pCurrentProcess->GetAnimAction() != ANIM_ACTION_BLOCK && (!GetIronSights() && IsWeaponDrawn()))) {
					if (!IsPipboyActive() && pCurrentProcess->CanAttack()) {
						if (IsWeaponDrawn() && (pweapon && !pweapon->IsMeleeWeapon())) {
							SetIronSights(1, 0, 0);
						} else {
							SetBlock(1);
						}
						if (!PlayerCharacter::bVanityMode || PlayerCharacter::bIsAutoVanityMode) {
							buserInput = 1;
						}
					}
				} else {
					if ((pcontrols->GetUserAction(Controls::UACTN_BLOCK, 2) ||
							(!pcontrols->GetUserAction(Controls::UACTN_BLOCK, 0) || IsPipboyActive())) &&
						(GetIronSights() || pCurrentProcess->GetAnimAction() == ANIM_ACTION_BLOCK)) {
						SetIronSights(0, 0, 0);
						SetBlock(0);
						buserInput = 1;
					}
				}
			}
			if (pcontrols->GetUserAction(Controls::UACTN_JUMP, 1)) {
				if (!IsOverEncumbered()) {
					Animation* v6 = GetAnimation();
					uint16_t Group = v6->GetGroup(AGS_WEAPON);
					if (!TESAnimGroup::IsPowerAttackAction(Group) && GetAnimAction() != ANIM_ACTION_DODGE) {
						bool v19 = !IsJumping() && !IsSwimming();
						bool bsuccess = 0;
						bool bpressingBlock = pcontrols->GetUserAction(Controls::UACTN_BLOCK, 0) != 0;
						bool bblocking = GetBlocked();
						if (bpressingBlock) {
							if (Interface::GetActivateREFR()) {
								if (!bblocking) {
									bhkCharacterController* CharController = GetCharController();
									if (static_cast<uint8_t>(CharController->CanJump())) {
										bsuccess = Jump() != 0;
									}
								}
							} else {
								bhkCharacterController* v8 = GetCharController();
								if (v8->GetState() == HK_CHARACTER_ON_GROUND) {
									// Dodge takes the move mode's low byte.
									ANIM_GROUP_ENUM eresult = Dodge(static_cast<uint8_t>(smoveMode));
									bsuccess = eresult != ANIM_GROUP_NONE;
								}
							}
						} else {
							bhkCharacterController* v10 = GetCharController();
							if (static_cast<uint8_t>(v10->CanJump())) {
								bsuccess = Jump() != 0;
							}
						}
						ForcePhysicsGrabDrop();
						if (v19 && bsuccess) {
							bhkCharacterController* v11 = GetCharController();
							v11->CanJump();
						}
						buserInput = 1;
					}
				}
			}
		}
		b3rdPerson = 1;
		UpdateAnimation();
		PickAnimations(PlayerCharacter::fWalkSpeedMult, PlayerCharacter::fPCTurnScale);
		b3rdPerson = bActually3rdPerson;
		if (IsSleeping()) {
			PlayerSleep();
		}
		SetNoCollision(0);
		static_cast<PlayerMover*>(pActorMover)->PlayerMover::SetSideVector(sideVec);
		pActorMover->Update(afDelta);
		if (imouseX || (v50 || (smoveMode & 15) != 0)) {
			buserInput = 1;
		}
		if (pcontrols->GetUserAction(Controls::UACTN_ACTIVATE, 1)) {
			if (!Interface::IsInMenuMode() && !GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT) && !IsParalyzed() &&
				!bdropping) {
				Actor* pactor = static_cast<Actor*>(Interface::GetActivateREFR());
				bool bcheckFurniture = 1;
				bool bskipActivate = 0;
				if (pactor) {
					if (pactor->IsFurniture() && GetIronSights()) {
						bskipActivate = 1;
					}
				}
				if (pactor) {
					if (pactor->IsProjectile()) {
						TESObjectCONT* parrow =
							static_cast<TESObjectCONT*>(FNVD_DYNAMIC_CAST_VOID(ArrowProjectile)(pactor,
								RTTI_TESObjectREFR, RTTI_ArrowProjectile));
						if (parrow) {
							if (!parrow->GetOpenSound()) {
								bskipActivate = 1;
							}
						}
					}
				}
				if (pactor) {
					if (pactor->IsActor()) {
						TESObjectREFR* pref = (TESObjectREFR*)pactor;
						BaseProcess* pprocess = pactor->pCurrentProcess;
						if (pprocess->GetFadeState() == BaseProcess::FADE_OUT_DISABLE ||
							pprocess->GetFadeState() == BaseProcess::FADE_OUT_DELETE) {
							bskipActivate = 1;
						}
					}
				}
				if (!bskipActivate) {
					Interface::SetInfoForRef(0, 0, 0);
					if (pactor) {
						if (pactor->Activate(this, 0, 0, 1)) {
							bcheckFurniture = 0;
						}
					}
					if (bcheckFurniture) {
						if (pCurrentProcess->GetCurrentFurniture()) {
							TESObjectREFR* pref_2 = pCurrentProcess->GetCurrentFurniture();
							pref_2->Activate(this, 0, 0, 1);
						}
					}
				}
				buserInput = 1;
			}
		}
		if (pPlayerSphere) {
			uint32_t v13 = iFlyCameraFrameCount++;
			if (v13 < 20) {
				NiPoint3 np;
				np.z = v55.z + 128.0;
				np.y = v55.y + 64.0;
				np.x = v55.x + 64.0;
				pPlayerSphere->SetPosition(np);
				pPlayerSphere->SetLinearVelocity(NiPoint3_ZERO);
			}
		}
		if (!IsDead(0) && !IsDying()) {
			UpdateMagic(0, 0);
		}
		Animation* panim = GetAnimation(0);
		Animation* panim1st = GetAnimation(1);
		float a2 = panim->GetMoveSpeed();
		panim1st->SetMoveSpeed(a2);
		float a2a = panim->m_fAttackSpeed;
		panim1st->SetAttackSpeed(a2a);
		b3rdPerson = !b3rdPerson;
		Character::Update(afDelta);
		Animation* Animation_0 = GetAnimation(!b3rdPerson);
		UpdateAnimationMovement(Animation_0, afDelta);
		b3rdPerson = !b3rdPerson;
		Character::Update(afDelta);
		Animation* v15 = GetAnimation(!b3rdPerson);
		UpdateAnimationMovement(v15, afDelta);
		HandlePostAnimationActions();
		if (pCurrentProcess->GetCurrentMuzzleFlash()) {
			MuzzleFlash* v16 = pCurrentProcess->GetCurrentMuzzleFlash();
			v16->Update(afDelta, this);
		}
	}
	if (pcontrols->GetUserAction(Controls::UACTN_QUICKSAVE, 1)) {
		if (!bQuickSaving) {
			bQuickSaving = 1;
			pSaveLoadManager->Quicksave();
			bQuickSaving = 0;
		}
	}
	if (pcontrols->GetUserAction(Controls::UACTN_QUICKLOAD, 1)) {
		if (!bQuickLoading) {
			bQuickLoading = 1;
			pSaveLoadManager->Quickload();
			bQuickLoading = 0;
		}
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
	TESObjectWEAP* pWeapon = pCurrentProcess->GetCurrentWeapon()
		? static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject())
		: nullptr;
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
		} else if (pCurrentAction && ActionPoints::IsAttackAction(pCurrentAction->eAction) &&
			pCurrentAction->cFireShots && !pCurrentAction->bStranger && !pCurrentAction->bMissFortune) {
			if ((pWeapon && pWeapon->IsAutomaticWeapon()) ||
				((TESAnimGroup::GetType(pAnimation->GetGroup(AGS_WEAPON)) == ANIM_GROUP_AIM ||
					 TESAnimGroup::GetType(pAnimation->GetGroup(AGS_WEAPON)) == ANIM_GROUP_AIM_IS) &&
					pAnimation->GetCurrentSequence(AGS_WEAPON)->GetState() == NiControllerSequence::kAnimating &&
					(!pAnimation->GetCurrentSequence(AGS_WEAPON_UP) ||
						pAnimation->GetCurrentSequence(AGS_WEAPON_UP)->GetState() ==
							NiControllerSequence::kAnimating) &&
					(!pAnimation->GetCurrentSequence(AGS_WEAPON_DOWN) ||
						pAnimation->GetCurrentSequence(AGS_WEAPON_DOWN)->GetState() ==
							NiControllerSequence::kAnimating))) {
				bool bFire = false;
				if (pVATS.GetMeleeAttack() ||
					((!pWeapon || pWeapon->IsMeleeWeapon()) && pVATS.GetCurrentCamera() &&
						pVATS.GetCurrentCamera()->IsInitialized() && pVATS.GetCurrentCameraMinTimer() <= 0.0)) {
					if (pAnimation->SpecialIdleLoaded() && !pAnimation->IsIdleNotLoaded(pVATS.GetMeleeAttack())) {
						bFire = true;
					}
				} else {
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
						case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3:
							--pCurrentAction->cFireShots;
							break;
						default:
							break;
						}
					}
				}
			}
		}
	} else {
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
						} else if (pCurrentProcess->IsCurrentWeaponGrenade() ||
							pCurrentProcess->IsCurrentWeaponThrown()) {
							eAttackAnim = ANIM_GROUP_ATTACK_THROW;
						} else if (pCurrentProcess->IsCurrentWeaponMine()) {
							eAttackAnim = ANIM_GROUP_PLACE_MINE;
						}
						fProjectileReleaseTimer = 0.0f;
					}
				} else if (pAnimation->GetGroupAction(AGS_WEAPON) == 3) {
					if (inextAttack == 1) {
						switch (pAnimation->FindSwitchKey(nullptr)) {
						case '3':
							eAttackAnim = ANIM_GROUP_ATTACK_3;
							break;
						case '4':
							eAttackAnim = ANIM_GROUP_ATTACK_4;
							break;
						case '5':
							eAttackAnim = ANIM_GROUP_ATTACK_5;
							break;
						case '6':
							eAttackAnim = ANIM_GROUP_ATTACK_6;
							break;
						case '7':
							eAttackAnim = ANIM_GROUP_ATTACK_7;
							break;
						case '8':
							eAttackAnim = ANIM_GROUP_ATTACK_8;
							break;
						case 'l':
							eAttackAnim = ANIM_GROUP_ATTACK_LEFT;
							break;
						default:
							if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
								eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
							} else {
								eAttackAnim = ANIM_GROUP_ATTACK_RIGHT;
							}
							break;
						}
					} else if (inextAttack == 2) {
						eAttackAnim = ANIM_GROUP_ATTACK_NORMAL_POWER;
					}
				}
			}
		} else if (pCurrentProcess->IsCurrentWeaponGrenadeOrMine()) {
			if (GetAnimAction() != ANIM_ACTION_ATTACK_FOLLOW_THROUGH || pAnimation->GetGroupAction(AGS_WEAPON) > 2) {
				if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
					eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
				} else if (pCurrentProcess->IsCurrentWeaponGrenade() || pCurrentProcess->IsCurrentWeaponThrown()) {
					eAttackAnim = ANIM_GROUP_ATTACK_THROW;
				} else {
					eAttackAnim = ANIM_GROUP_PLACE_MINE;
				}
				fProjectileReleaseTimer = 0.0f;
			}
		} else if (inextAttack == 1) {
			if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
				eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
			} else {
				eAttackAnim = ANIM_GROUP_ATTACK_RIGHT;
			}
		} else if (inextAttack == 2) {
			eAttackAnim = ANIM_GROUP_ATTACK_NORMAL_POWER;
		}
	} else if (pControls->GetUserAction(Controls::UACTN_USE, Controls::isPressed) && !bReleaseTheArrow) {
		// The attack control went down.
		if (!pCurrentProcess->GetWeaponDrawn()) {
			if (!(GetMoveMode() & 0x800)) {
				SetWantWeaponDrawn(true);
			}
		} else if (GetAnimAction() == ANIM_ACTION_RELOAD_LOOP) {
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
			} else if (usMoveMode & 0x2000) {
				eMove = AGM_SWIM;
			} else if (usMoveMode & 0x400) {
				eMove = AGM_WALK;
			}
			ANIM_GROUP_ENUM eType = ANIM_GROUP_NONE;
			if (usMoveMode & 0x200) {
				if (usMoveMode & 1) {
					eType = ANIM_GROUP_FAST_FORWARD;
				} else if (usMoveMode & 2) {
					eType = ANIM_GROUP_FAST_BACK;
				} else if (usMoveMode & 4) {
					eType = ANIM_GROUP_FAST_LEFT;
				} else if (usMoveMode & 8) {
					eType = ANIM_GROUP_FAST_RIGHT;
				}
			} else if (usMoveMode & 1) {
				eType = ANIM_GROUP_MOVE_FORWARD;
			} else if (usMoveMode & 2) {
				eType = ANIM_GROUP_MOVE_BACK;
			} else if (usMoveMode & 4) {
				eType = ANIM_GROUP_MOVE_LEFT;
			} else if (usMoveMode & 8) {
				eType = ANIM_GROUP_MOVE_RIGHT;
			}
			int32_t iCount = 0;
			int32_t iClipRounds = 0;
			ItemChange* pAmmo = pCurrentProcess->GetCurrentAmmo();
			ItemChange* pWeaponChange = pCurrentProcess->GetCurrentWeapon();
			InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(this);
			if (pWeaponChange) {
				iClipRounds =
					pWeapon->GetFormClipRounds(pWeaponChange->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE));
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
			uint16_t usGroup =
				pAnimation->PickBestAnimation(TESAnimGroup::AnimGroup(eMove, eWeaponType, eType, bPowerArmor), false);
			if (usGroup != ANIM_GROUP_NONE) {
				GetAnimation(false)->PlayGroup(usGroup, ACTION_START, 1, AGS_WEAPON);
				SetAnimAction(ANIM_ACTION_NONE, GetAnimation(false)->GetCurrentSequence(AGS_WEAPON));
				StartAnimOn1stPerson(usGroup, static_cast<ACTION_FLAGS>(1));
			}
		} else if (TESAnimGroup::IsAttackAction(pAnimation->GetGroup(AGS_WEAPON)) &&
			!(pWeapon && pWeapon->IsAutomaticWeapon())) {
			// During a single-shot attack: queue the next one once the current one is past its switch key.
			if (!pCurrentProcess->IsCurrentWeaponGrenadeOrMine() &&
				(!IsSneaking() || !TESAnimGroup::IsPowerAttackAction(pAnimation->GetGroup(AGS_WEAPON)))) {
				if (pWeapon && (pWeapon->GetCurrentAmmo(pPlayer) || pWeapon->IsRockItLauncher())) {
					float fLastScaledTime = pAnimation->GetCurrentSequence(AGS_WEAPON)->GetLastScaledTime();
					float fKeyLength =
						static_cast<float>(pAnimation->GetCurrentSequence(AGS_WEAPON)->GetKeyLengthOverFrequency());
					float fSwitchTime = 0.0f;
					pAnimation->FindSwitchKey(&fSwitchTime);
					// The two floats are subtracted at double precision and the unrounded difference is compared.
					if (!pWeapon->IsAutomaticWeapon() &&
						(fSwitchTime <= fLastScaledTime ||
							static_cast<double>(fKeyLength) - static_cast<double>(fLastScaledTime) < 0.5)) {
						inextAttack = 1;
					}
				} else {
					inextAttack = 1;
				}
			}
		} else {
			// Starts a new attack.
			bSpinAttackReleasedEarly = false;
			pCurrentProcess->ClearAutomaticShotsToFire(true);
			pCurrentProcess->SetShotsToFire(1);
			if (pWeapon) {
				bSustainedFire = true;
			}
			if (pWeapon && pWeapon->IsAutomaticWeapon() &&
				!TESAnimGroup::IsAttackSpinAction(pWeapon->GetAttackAnim())) {
				pCurrentProcess->SetAutomaticFireAtLeastOnce(true);
			}
			if (!pWeapon || !TESAnimGroup::IsAttackSpinAction(pWeapon->GetAttackAnim())) {
				if (pCurrentProcess->IsCurrentWeaponGrenade()) {
					if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
						eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
					} else {
						eAttackAnim = ANIM_GROUP_ATTACK_THROW;
					}
					AddPlayerAction(PLAYER_ACTION_THROW_GRANADE, 3.0f, nullptr);
					if (bInVATS) {
						fProjectileReleaseTimer = 10.0f;
					} else {
						fProjectileReleaseTimer = 0.0f;
					}
				} else if (pCurrentProcess->IsCurrentWeaponMine()) {
					if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
						eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
					} else {
						eAttackAnim = ANIM_GROUP_PLACE_MINE;
					}
					AddPlayerAction(PLAYER_ACTION_LAY_MINE, 3.0f, nullptr);
					if (bInVATS) {
						fProjectileReleaseTimer = 10.0f;
					} else {
						fProjectileReleaseTimer = 0.0f;
					}
				} else if (pCurrentProcess->IsCurrentWeaponThrown()) {
					if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
						eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
					} else {
						eAttackAnim = ANIM_GROUP_ATTACK_THROW;
					}
					AddPlayerAction(PLAYER_ACTION_FIRE_WEAPON, 3.0f, nullptr);
					if (bInVATS) {
						fProjectileReleaseTimer = 10.0f;
					} else {
						fProjectileReleaseTimer = 0.0f;
					}
				} else if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
					eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
				} else {
					eAttackAnim = ANIM_GROUP_ATTACK_RIGHT;
				}
			}
		}
		fpowerTimer = 0.0f;
		bResult = true;
	} else if (pControls->GetUserAction(Controls::UACTN_USE, Controls::isHeld) && pCurrentProcess->GetWeaponDrawn() &&
		GetWantWeaponDrawn() && !bReleaseTheArrow) {
		// The attack control is held: cook throws, keep automatic fire going and time the power attack.
		if ((pCurrentProcess->IsCurrentWeaponGrenade() || pCurrentProcess->IsCurrentWeaponThrown()) &&
			(GetAnimAction() == ANIM_ACTION_ATTACK_THROW || GetAnimAction() == ANIM_ACTION_ATTACK_THROW_ATTACH) &&
			pAnimation->GetGroupAction(AGS_WEAPON) <= 2) {
			fProjectileReleaseTimer = fProjectileReleaseTimer + fDelta;
		}
		if (pCurrentAction && pWeapon && pWeapon->GetCurrentAmmo(pPlayer) &&
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
					} else {
						bSustainedFire = true;
					}
				} else {
					bSustainedFire = true;
				}
				if (ucShots && bFire) {
					pCurrentProcess->SetShotsToFire(ucShots);
					switch (pAnimation->FindSwitchKey(nullptr)) {
					case '3':
						eAttackAnim = ANIM_GROUP_ATTACK_3;
						break;
					case '4':
						eAttackAnim = ANIM_GROUP_ATTACK_4;
						break;
					case '5':
						eAttackAnim = ANIM_GROUP_ATTACK_5;
						break;
					case '6':
						eAttackAnim = ANIM_GROUP_ATTACK_6;
						break;
					case '7':
						eAttackAnim = ANIM_GROUP_ATTACK_7;
						break;
					case '8':
						eAttackAnim = ANIM_GROUP_ATTACK_8;
						break;
					case 'l':
						eAttackAnim = ANIM_GROUP_ATTACK_LEFT;
						break;
					default:
						if (pWeapon && pWeapon->GetAttackAnim() != ANIM_GROUP_NONE) {
							eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetAttackAnim());
						} else {
							eAttackAnim = ANIM_GROUP_ATTACK_RIGHT;
						}
						break;
					}
				}
			}
		} else {
			if (pCurrentProcess->IsCurrentWeaponGrenade()) {
				AddPlayerAction(PLAYER_ACTION_THROW_GRANADE, 3.0f, nullptr);
			} else if (pCurrentProcess->IsCurrentWeaponMine()) {
				AddPlayerAction(PLAYER_ACTION_LAY_MINE, 3.0f, nullptr);
			} else if (pCurrentProcess->IsCurrentWeaponThrown()) {
				AddPlayerAction(PLAYER_ACTION_FIRE_WEAPON, 3.0f, nullptr);
			} else {
				AddPlayerAction(PLAYER_ACTION_SWING_MELEE_WEAPON, 1.5f, nullptr);
			}
			if (!TESAnimGroup::IsPowerAttackAction(pAnimation->GetGroup(AGS_WEAPON)) && inextAttack != 2 &&
				!pCurrentProcess->IsCurrentWeaponGrenadeOrMine()) {
				fpowerTimer = fpowerTimer + fDelta;
			}
		}
		if (!IsOverEncumbered() && fPowerAttackDelay.Float() < fpowerTimer) {
			if (IsSwimming()) {
				inextAttack = 1;
			} else if (TESAnimGroup::IsAttackAction(pAnimation->GetGroup(AGS_WEAPON)) &&
				pAnimation->GetTime(pAnimation->GetCurrentSequence(AGS_WEAPON)) > 0.0) {
				inextAttack = 2;
			} else {
				eAttackAnim = ANIM_GROUP_ATTACK_NORMAL_POWER;
			}
			fpowerTimer = 0.0f;
		}
		bResult = true;
	} else if (pControls->GetUserAction(Controls::UACTN_READY_RELOAD_WEAPON, Controls::isDepressed) &&
		pCurrentProcess->GetWeaponDrawn() && fReloadTimer != 0.0 && fPlayerWeaponReloadTimer.Float() > fReloadTimer &&
		pCurrentProcess->GetCurrentWeapon()) {
		// A short press of the reload control reloads (or opens the Rock-It Launcher's container).
		if (GetAnimAction() == ANIM_ACTION_NONE || pAnimation->GetGroupAction(AGS_WEAPON) == 3) {
			if (pWeapon->IsRockItLauncher()) {
				Interface::QueueMenuCreate(Interface::OPEN_CONTAINER, nullptr, nullptr, nullptr,
					Interface::CMM_ROCK_N_LOAD, false);
			} else {
				bool bRegenMod = pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_AMMO_REGEN_SECONDS);
				TESObjectWEAP* pCurrentWeapon =
					static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject());
				if (pCurrentWeapon->GetAmmoRegenRate(bRegenMod) <= 0.0) {
					ReloadWeaponNV(pWeapon, 2,
						pCurrentProcess->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE), false);
				}
			}
			pCurrentProcess->SetSustainedFire(0);
			bSustainedFire = false;
		}
	} else if (TESAnimGroup::IsAttackAction(pAnimation->GetGroup(AGS_WEAPON)) &&
		(pControls->GetUserAction(Controls::UACTN_USE, Controls::isDepressed) ||
			!pControls->GetUserAction(Controls::UACTN_USE, Controls::isHeld))) {
		// The attack control was let go during an attack.
		pCurrentProcess->SetSustainedFire(0);
		bSustainedFire = false;
		if (TESAnimGroup::IsAttackSpinAction(pAnimation->GetGroup(AGS_WEAPON))) {
			bSpinAttackReleasedEarly = pAnimation->GetGroupAction(AGS_WEAPON) < 1;
		}
	}

	if (eAttackAnim != ANIM_GROUP_NONE) {
		if ((!pWeapon || pWeapon->IsMeleeWeapon()) && GetAnimAction() == ANIM_ACTION_BLOCK) {
			SetBlock(false);
		}
		if (IsSneaking() && (!pWeapon || (pWeapon->IsMeleeWeapon() && !pWeapon->IsAutomaticWeapon())) &&
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
			} else {
				uint16_t usMoveMode = static_cast<uint16_t>(GetMoveMode());
				if (usMoveMode & 1) {
					eAttackAnim = ANIM_GROUP_ATTACK_FORWARD_POWER;
				} else if (usMoveMode & 2) {
					eAttackAnim = ANIM_GROUP_ATTACK_BACK_POWER;
				} else if (usMoveMode & 4) {
					eAttackAnim = ANIM_GROUP_ATTACK_LEFT_POWER;
				} else if (usMoveMode & 8) {
					eAttackAnim = ANIM_GROUP_ATTACK_RIGHT_POWER;
				}
				if (eAttackAnim == ANIM_GROUP_ATTACK_FORWARD_POWER && pWeapon &&
					pWeapon->GetPowerAttackAnimOverride() != ANIM_GROUP_NONE) {
					eAttackAnim = static_cast<ANIM_GROUP_ENUM>(pWeapon->GetPowerAttackAnimOverride());
				}
				if (bUnarmed) {
					float fForward = 0.0f;
					BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_UNARMED_FORWARD_POWER_ATTACK, pPlayer,
						&fForward);
					float fBack = 0.0f;
					BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_UNARMED_BACKWARD_POWER_ATTACK, pPlayer, &fBack);
					float fLeft = 0.0f;
					BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_UNARMED_LEFT_POWER_ATTACK, pPlayer, &fLeft);
					float fRight = 0.0f;
					BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_UNARMED_RIGHT_POWER_ATTACK, pPlayer, &fRight);
					if (eAttackAnim == ANIM_GROUP_ATTACK_FORWARD_POWER && fForward > 0.0) {
						eAttackAnim = ANIM_GROUP_ATTACK_CUSTOM_1_POWER;
					} else if (eAttackAnim == ANIM_GROUP_ATTACK_BACK_POWER && fBack > 0.0) {
						eAttackAnim = ANIM_GROUP_ATTACK_CUSTOM_2_POWER;
					} else if (eAttackAnim == ANIM_GROUP_ATTACK_LEFT_POWER && fLeft > 0.0) {
						eAttackAnim = ANIM_GROUP_ATTACK_CUSTOM_4_POWER;
					} else if (eAttackAnim == ANIM_GROUP_ATTACK_RIGHT_POWER && fRight > 0.0) {
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
			} else if (pCurrentAction->eAction == ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK && !IsSneaking()) {
				eAttackAnim = ANIM_GROUP_ATTACK_6;
			} else if (pCurrentAction->eAction == ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2) {
				eAttackAnim = ANIM_GROUP_ATTACK_7;
			} else if (pCurrentAction->eAction == ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3) {
				eAttackAnim = ANIM_GROUP_STOMP;
			}
		}
		bool bStarted = StartAttack(eAttackAnim);
		if (bStarted && pWeapon && (pWeapon->IsAutomaticWeapon() || pContinuousBeamPersistant) && bSustainedFire) {
			pCurrentProcess->SetSustainedFire(1);
		}
		if (bStarted && bSpeak) {
			// Combat topic 1, the power attack taunt.
			pCombatDialogueManager->StartDialogue(pPlayer, nullptr, DT_COMBAT, 1, false, nullptr);
		}
		inextAttack = 0;
		bUserDebounceBow = true;
		ilargestAttackForce = 0;
	}

	// While the attack control is held, a melee or thrown attack waits at its hold key.
	if ((pControls->GetUserAction(Controls::UACTN_USE, Controls::isHeld) ||
			pControls->GetUserAction(Controls::UACTN_USE, Controls::isPressed)) &&
		pCurrentProcess->GetWeaponDrawn() && !bReleaseTheArrow) {
		if ((!pWeapon || pWeapon->IsMeleeWeapon() || pCurrentProcess->IsCurrentWeaponGrenade()) &&
			(!pWeapon || pWeapon->GetCurrentAmmo(pPlayer) || !pWeapon->IsAutomaticWeapon()) &&
			TESAnimGroup::IsAttackAction(pAnimation->GetGroup(AGS_WEAPON)) &&
			!TESAnimGroup::IsPowerAttackAction(pAnimation->GetGroup(AGS_WEAPON)) && !inextAttack && !bInVATS) {
			BSAnimGroupSequence* pSequence = pAnimation->GetCurrentSequence(AGS_WEAPON);
			float fTime = pAnimation->GetTime(pSequence);
			if (pCurrentProcess->IsCurrentWeaponGrenade()) {
				if (bUserDebounceBow) {
					ForceGrenadeHold();
				}
			} else if ((fTime <= 0.0 || pSequence->GetState() != NiControllerSequence::kAnimating) &&
				!IsOverEncumbered()) {
				pAnimation->SkipUpdate(AGS_WEAPON);
				p1stAnimation->SkipUpdate(AGS_WEAPON);
			}
		}
	} else {
		bUserDebounceBow = false;
		if (GetAnimAction() == ANIM_ACTION_NONE) {
			bReleaseTheArrow = false;
		}
	}
	return bResult;
}

void PlayerCharacter::SetAcousticSpaceTransition(bool val) {
	bIsAcousticSpaceTransition = (bool)((uint8_t)val);
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
// through unchecked, as the game does. An AutoMemContext lives for the whole function.
void PlayerCharacter::Preload1stPersonAnimsFileList() {
	AutoMemContext context(MEM_CONTEXT_PLAYER, true,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 16837);
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
	AutoMemContext context(MEM_CONTEXT_PLAYER, true,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\PlayerCharacter.cpp", 16895);
	TESNPC* pnpc = static_cast<TESNPC*>(GetObjectReference());
	TESModel* pModel = pnpc;
	if (pnpc->GetSex() == TESActorBase::SEX_FEMALE) {
		pModel->SetModel(sNPCModelFemale.String());
	} else {
		pModel->SetModel(sNPCModelMale.String());
	}
	BSSimpleList<const char*>* pfileList =
		pModelLoader->BuildKFFileList(sFirstPersonSkeleton.String(), true, false, ANIM_GROUP_WEAPON_COUNT);
	char plocomotionSexDirectory[260];
	BSstrcpy(plocomotionSexDirectory, 260, sFirstPersonSkeleton.String());
	char* pslash = std::strrchr(plocomotionSexDirectory, '\\');
	if (IsPCToddler()) {
		BSstrcpy(pslash, 260 - (pslash - plocomotionSexDirectory) - 1, "\\Locomotion\\Toddler\\IdleAnims");
		pModelLoader->CopyFilenameList(pIdleManager->GetRootFilenameList(plocomotionSexDirectory, AGW_NONE), pfileList);
	}
	if (GetSex() == TESActorBase::SEX_FEMALE) {
		BSstrcpy(pslash, 260 - (pslash - plocomotionSexDirectory) - 1, "\\Locomotion\\Female\\IdleAnims");
	} else {
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
		return (&sp1stPerson3D)->operator->();
	} else {
		return TESObjectREFR::Get3D();
	}
}

void PlayerCharacter::ResetShapeCasterInfo() {
	int32_t filterGroup;
	void* charController = MobileObject::GetCharController();
	if (charController) {
		filterGroup = (int32_t)static_cast<bhkCharacterController*>(charController)->GetFilterGroup();
		pCameraCaster->SetPlayerGroup(filterGroup);
	}
}

TESRegionList* PlayerCharacter::GetAllOccupiedRegions() {
	return &m_AllOccupiedRegions;
}

void PlayerCharacter::SetCanFastTravel(bool abflag) {
	if (abflag) {
		bCanFastTravel |= 1;
	} else {
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
		return 1;
	}
	resetAntiCheatTimer();
	unset_11E07AB();
	return 0;
}

int32_t PlayerCharacter::GetCasinoCheatTimeRemaining() {
	uint32_t v1 = iAntiCheatDuration.UInt() + uiAntiCheatTimer;
	return v1 - GetTickCount() / 1000;
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
				void* pMemory = ourMalloc(sizeof(AudioMarkerInfo));
				AudioMarkerInfo* pInfo = pMemory ? ::new (pMemory) AudioMarkerInfo(pRef) : nullptr;
				if (pInfo->pMarkerData) {
					AudioMarkerList.AddHead(pInfo);
				} else {
					ourFree(pInfo);
				}
			}
		}
		pList->RemoveAll();
		if (pList) {
			delete pList;
		}
	} else {
		void* pListMemory = ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>));
		BSSimpleList<TESObjectREFR*>* pList = pListMemory ? ::new (pListMemory) BSSimpleList<TESObjectREFR*> : nullptr;
		TESObjectCELL* pCell = GetParentCell();
		if (pCell) {
			pCell->AddAudioMarkersToList(pList);
		}
		for (BSSimpleList<TESObjectREFR*>* pIter = pList; pIter; pIter = pIter->GetNext()) {
			TESObjectREFR* pRef = pIter->GetItem();
			if (pRef) {
				void* pMemory = ourMalloc(sizeof(AudioMarkerInfo));
				AudioMarkerInfo* pInfo = pMemory ? ::new (pMemory) AudioMarkerInfo(pRef) : nullptr;
				if (pInfo->pMarkerData) {
					AudioMarkerList.AddHead(pInfo);
				} else {
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
	BSSimpleList<PlayerCharacter::MapMarkerInfo*>* v1;
	BSSimpleList<PlayerCharacter::MapMarkerInfo*>* ecx0;
	for (ecx0 = &MapMarkerList; ecx0; ecx0 = ecx0->m_pkNext) {
		v1 = ecx0;
		ourFree(v1->m_item);
	}
	(&MapMarkerList)->RemoveAll();
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
				} else {
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
	if (Get3D(1) && (Get3D(0) && (!abFirstPerson || (!IsDead(0) && !bTemp3rdPerson)))) {
		ItemChange* v8;
		NiAVObject* v2 = Get3D(0);
		int32_t AppCulled = v2->GetAppCulled();
		NiAVObject* v4 = Get3D(1);
		if (AppCulled != v4->GetAppCulled()) {
			if (!abFirstPerson) {
				NiAVObject* v5 = Get3D(0);
				if (!v5->GetAppCulled()) {
					return;
				}
			}
			if (abFirstPerson) {
				NiAVObject* v6 = Get3D(1);
				if (!v6->GetAppCulled()) {
					bActually3rdPerson = 0;
					return;
				}
			}
		}
		NiAVObject* v7 = Get3D(0);
		v7->SetAppCulled(abFirstPerson);
		if (abFirstPerson &&
			(bTemp1stPerson &&
				(GetIronSights() &&
					(pCurrentProcess->GetCurrentWeapon() &&
						(v8 = pCurrentProcess->GetCurrentWeapon(),
							static_cast<TESObjectWEAP*>(v8->pContainerObj)->GetScopeModel()->GetModelLength()))))) {
			ItemChange* v10;
			TESObjectWEAP* pweap = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
			fVal = 0.0f;
			if (!pweap->HasModScope() ||
				(v10 = pCurrentProcess->GetCurrentWeapon(), v10->HasModEffectActive(WEAPON_MOD_IRON_SITES, fVal))) {
				Interface::SetGunScopeVisible(1);
			}
		} else {
			NiAVObject* v11 = Get3D(1);
			v11->SetAppCulled(!abFirstPerson);
		}
		bActually3rdPerson = !abFirstPerson;
		if (abFirstPerson) {
			NiMatrix3 rot;
			NiAVObject* p3dObj = Get3D(0);
			const NiPoint3* v12 = &GetLocationOnReference();
			p3dObj->SetTranslate(*v12);
			float a2 = GetHeading(0);
			rot.MakeZRotation(a2);
			p3dObj->SetRotate(MultipleMatrixByRace(rot));
			fPlayerCameraAlpha = 1.0f;
			UpdateAlpha();
			Animation* panim = GetAnimation(0);
			if (panim) {
				panim->UpdateMovement(this);
			} else {
				NiUpdateData UpdateData(0.0f, 0, 0);
				p3dObj->Update(UpdateData);
			}
			BSSoundHandle weaponSoundHandle;
			(&m_Extra)->GetWeaponIdleSound(weaponSoundHandle);
			if (weaponSoundHandle.IsValid()) {
				weaponSoundHandle.SetFollowsListener(1);
			}
			(&m_Extra)->GetWeaponAttackSound(weaponSoundHandle);
			if (weaponSoundHandle.IsValid()) {
				weaponSoundHandle.SetFollowsListener(1);
			}
		} else {
			BSSoundHandle weaponSoundHandle_2;
			(&m_Extra)->GetWeaponIdleSound(weaponSoundHandle_2);
			if (weaponSoundHandle_2.IsValid()) {
				weaponSoundHandle_2.SetFollowsListener(0);
			}
			(&m_Extra)->GetWeaponAttackSound(weaponSoundHandle_2);
			if (weaponSoundHandle_2.IsValid()) {
				weaponSoundHandle_2.SetFollowsListener(0);
			}
		}
		SetFireNode(0);
		if (GetShouldCheckMagicNode()) {
			NiNode* pnodeTo;
			NiNode* pnodeFrom;
			Animation* panimFrom = 0;
			Animation* v36 = 0;
			NiNode* v33 = 0;
			panimFrom = GetAnimation(!abFirstPerson);
			Animation* panimTo = GetAnimation(abFirstPerson);
			NiControllerManager* ControllerManager = panimFrom->GetControllerManager();
			NiAVObjectPalette* ObjectPalette = ControllerManager->GetObjectPalette();
			NiFixedString* v15 = &FixedStrings::QMagicNode();
			NiAVObject* pobj = ObjectPalette->GetAVObject(*v15);
			if (pobj) {
				pnodeFrom = pobj->IsNode();
			} else {
				pnodeFrom = 0;
			}
			NiControllerManager* v16 = panimTo->GetControllerManager();
			NiDefaultAVObjectPalette* v23 = static_cast<NiDefaultAVObjectPalette*>(v16->GetObjectPalette());
			NiFixedString* v17 = &FixedStrings::QMagicNode();
			NiAVObject* pobj_2 = v23->GetAVObject(*v17);
			if (pobj_2) {
				pnodeTo = pobj_2->IsNode();
			} else {
				pnodeTo = 0;
			}
			if (pnodeFrom) {
				if (pnodeTo) {
					for (uint32_t I = 0;; ++I) {
						uint32_t ArrayCount = pnodeFrom->GetArrayCount();
						if (I >= ArrayCount) {
							break;
						}
						NiPointer<NiAVObject> pchild(static_cast<NiAVObject*>(0));
						pnodeFrom->DetachChildAt(I, pchild);
						if (pchild.operator!=(static_cast<NiAVObject*>(0))) {
							NiAVObject* v19 = pchild.m_pObject;
							pnodeTo->AttachChild(v19, 1);
						}
					}
				}
			}
		}
		NiNode* v47 = Get3D();
		NiNode* pnode = v47;
		if (abFirstPerson) {
			NiAVObject* pobj_3 = pnode->GetAt(0);
			pnode = static_cast<NiNode*>(pobj_3);
		}
		InitLightingPropertyPtr(pnode);
		BaseProcess* CurrentAIProcess = pCurrentProcess;
		CurrentAIProcess->SetShouldRefreshMagicShadersEffects(1, 1, 0);
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
	void* pTopic = apNewTopic;
	listTopics.AddHead(pTopic);
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
	bsearch = 0;
	if (!IsPlayerCharacterInCombat(&bsearch)) {
		PlayerActionObject* pobject = 0;
		if (!pListofActions) {
			BSSimpleList<PlayerActionObject*>* v5;
			BSSimpleList<PlayerActionObject*>* v8 =
				static_cast<BSSimpleList<PlayerActionObject*>*>(ourMalloc(sizeof(BSSimpleList<PlayerActionObject*>)));
			if (v8) {
				v5 = ::new (v8) BSSimpleList<PlayerActionObject*>;
			} else {
				v5 = 0;
			}
			pListofActions = v5;
		}
		BSSimpleList<PlayerActionObject*>* plist = pListofActions;
		PlayerActionObject* v12 = 0;
		while (plist && (plist->m_item && !pobject)) {
			PlayerActionObject* pAction = plist->m_item;
			if (pAction) {
				if (pAction->eAction == aeAction) {
					pobject = pAction;
				}
			}
			plist = plist->m_pkNext;
		}
		if (pobject) {
			pListofActions->Remove(pobject);
		} else {
			PlayerActionObject* pobject_2;
			PlayerActionObject* apMemory = static_cast<PlayerActionObject*>(ourMalloc(sizeof(PlayerActionObject)));
			if (apMemory) {
				pobject_2 = ::new (apMemory) PlayerActionObject;
			} else {
				pobject_2 = 0;
			}
			pobject = pobject_2;
		}
		pobject->eAction = aeAction;
		pobject->ftimer = afTime;
		if (apActObject) {
			apActObject->SetTargeted(1);
		}
		pobject->prefObj = apActObject;
		pListofActions->AddHead(pobject);
	}
}

void PlayerCharacter::UpdateImageSpaceIronsights() {
	if (!pIronsightsDOFInstance) {
		float fRange = fIronSightsDOFRange.Float();
		float fDistance = fIronSightsDOFDistance.Float();
		pIronsightsDOFInstance = ImageSpaceModifierInstanceDOF::Trigger(fDistance, fRange,
			ImageSpaceModifierInstanceDOF::DOF_FRONT_MASK, 0.0f, 0.0f, 0.0f);
		if (pIronsightsDOFInstance) {
			pIronsightsDOFInstance->SetPermanent(true);
		}
	}
	float fStrengthCap = fIronSightsDOFStrengthCap.Float();
	float fTargetStrength = 0.0f;
	float fStrength = pIronsightsDOFInstance->QStrength();
	if (GetIronSights() && !b3rdPerson) {
		// The motion blur setting is read and never used, as the game does.
		fIronSightsGunMotionBlur.Float();
		fTargetStrength = fIronSightsDOFStrengthCap.Float();
		BSShaderManager::SetFullScreenMotionBlurIronSights(true);
	} else {
		BSShaderManager::SetFullScreenMotionBlurIronSights(false);
	}
	if (fStrength != fTargetStrength) {
		// Keeps the rate as a double and rounds the step to a float, as the game does.
		double dRate = 1.0 / fIronSightsDOFSwitchSeconds.Float();
		float fStep = static_cast<float>(appTimer.GetSeconds() * dRate);
		if (fStrength < fTargetStrength) {
			fStrength = fStrength + fStep;
		} else {
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
		pVatsDOFInstance = ImageSpaceModifierInstanceDOF::Trigger(fDistance, fRange,
			ImageSpaceModifierInstanceDOF::DOF_FRONT_BACK, 0.0f, 0.0f, 0.0f);
		if (pVatsDOFInstance) {
			pVatsDOFInstance->SetPermanent(true);
		}
	}
	if (!pVatsDRBInstance) {
		NiPoint2 Center(0.5f, 0.5f);
		float fStart = fVATSRadialStart.Float();
		float fRampup = fVATSRadialRampup.Float();
		float fRange = fVATSDOFRange.Float();
		pVatsDRBInstance = ImageSpaceModifierInstanceDRB::Trigger(fDistance, fRange,
			ImageSpaceModifierInstanceDRB::DOF_FRONT_BACK, 0.0f, fRampup, fStart, 0.0f, 0.0f, Center, 0.0f);
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
		// Keeps the rate as a double and rounds the step to a float, as the game does.
		double dRate = 1.0 / fVATSDOFSwitchSeconds.Float();
		float fStep = static_cast<float>(appTimer.GetSeconds() * dRate);
		if (fDOFStrength < fDOFTarget) {
			fDOFStrength = fDOFStrength + fStep;
		} else {
			fDOFStrength = fDOFStrength - fStep;
		}
		BSClamp(fDOFStrength, 0.0f, fDOFStrengthCap);
	}
	if (fRadialStrength != fRadialTarget) {
		double dRate = 1.0 / fVATSDOFSwitchSeconds.Float();
		float fStep = static_cast<float>(appTimer.GetSeconds() * dRate);
		if (fRadialStrength < fRadialTarget) {
			fRadialStrength = fRadialStrength + fStep;
		} else {
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
		void* pMemory = ourMalloc(sizeof(TESObjectREFR));
		pPlayerMapMarker = pMemory ? ::new (pMemory) TESObjectREFR : nullptr;
		pPlayerMapMarker->SetObjectReference(static_cast<TESBoundObject*>(pXMarkerHeading));
	}
	pPlayerMapMarker->SetLocationOnReferenceSimple(arPoint);
	if (apForm->GetFormType() == CELL_ID) {
		pPlayerMapMarker->SetParentCellSimple(static_cast<TESObjectCELL*>(apForm));
	} else if (apForm->GetFormType() == WRLD_ID) {
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
	if (eact != ANIM_ACTION_ATTACK_FOLLOW_THROUGH && egroup >= ANIM_GROUP_WEAPONS_START &&
		egroup <= ANIM_GROUP_WEAPONS_STOP) {
		fpercent = 1.0f;
	} else if (eact != ANIM_ACTION_ATTACK_FOLLOW_THROUGH && IsPipboyActive()) {
		fpercent = 1.0f;
	} else if (GetIronSights()) {
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
		} else if (fchaseDelta > 0.0 && fpercent < fchasePercent) {
			fchasePercent = fpercent;
		}
		fpercent = fchasePercent;
	}
	NiMatrix3 rot;
	NiMatrix3 cameraRot;
	float fheading = GetHeading(false);
	if (p1stPersonLooking) {
		rot.MakeXRotation(GetLooking());
	} else {
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
	if (apTargetRef) {
		if (apOutResultPath) {
			// Destroyed in reverse order: the path, the marker's location, then the player's, as the game does.
			PathingLocation kPlayerPathing(pPlayer);
			PathingLocation kMarkerPathing(apTargetRef);
			TeleportPath kPath;
			ActorLockData apMemory(pPlayer);
			apOutResultPath->Clear();
			if (!Pathing::BuildLowPath(kPlayerPathing, kMarkerPathing, apOutResultPath, &apMemory,
					Pathing::DOOR_LOCK_DISPOSITION_LOCKS_ARE_MINIMAL_USE)) {
				apOutResultPath->Clear();
			}
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
			double fFormWeight = TESWeightForm::GetFormWeight(pObject, bIsHardcore);
			int32_t iCount = RockItLauncherAmmoList.GetAt(i)->GetCount();
			fRockItLauncherAmmoWeight = iCount * fFormWeight + fRockItLauncherAmmoWeight;
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
		this->pListofPercievedActors = 0;
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
				EnchantmentItem* FormEnchanting;
				TESEnchantableForm* penchantable = dynamic_cast<TESEnchantableForm*>(plist->m_item);
				if (penchantable) {
					FormEnchanting = penchantable->pFormEnchanting;
				} else {
					FormEnchanting = 0;
				}
				if (FormEnchanting) {
					CastWornEnchantment(FormEnchanting, plist->m_item, 0);
				}
			}
		}
		QueuedWornEnchantments->RemoveAll();
		if (QueuedWornEnchantments) {
			delete QueuedWornEnchantments;
		}
		QueuedWornEnchantments = 0;
	}
}

void PlayerCharacter::SetLightPointer(NiLight* apLight, bool abFirstPerson) {
	if (abFirstPerson) {
		sp1stPersonLight = apLight;
	} else {
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

// Sets the mode on the global player's mover, not on this, as the game does.
void PlayerCharacter::SetMoveMode(uint32_t aiMoveMode, bool abValue) {
	static_cast<PlayerMover*>(pPlayer->pActorMover)->SetMoveMode(aiMoveMode, abValue);
	if ((aiMoveMode & 0x400) && GetAnimation(false)) {
		if (TESAnimGroup::GetType(GetAnimation()->GetGroup(AGS_WEAPON)) == ANIM_GROUP_AIM) {
			GetAnimation(false)->BlendOut(AGS_WEAPON, false);
		}
	}
}

bool PlayerCharacter::IsStandingOnFurniture() {
	bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
	if (pController) {
		bhkRigidBody* pSupportBody = pController->GetSupportBody();
		bhkCollisionObject* pCollisionObject;
		if (pSupportBody) {
			pCollisionObject = bhkCollisionObject::Getbhk(pSupportBody->operator*());
		} else {
			pCollisionObject = nullptr;
		}
		NiAVObject* pObject;
		if (pCollisionObject) {
			pObject = pCollisionObject->GetSceneGraphObject();
		} else {
			pObject = nullptr;
		}
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
	bhkCollisionObject* pCollisionObject;
	if (pBumpedBody) {
		pCollisionObject = bhkCollisionObject::Getbhk(pBumpedBody->operator*());
	} else {
		pCollisionObject = nullptr;
	}
	NiAVObject* pObject;
	if (pCollisionObject) {
		pObject = pCollisionObject->GetSceneGraphObject();
	} else {
		pObject = nullptr;
	}
	if (pObject) {
		return TESObjectREFR::FindReferenceFor3D(pObject);
	}
	return nullptr;
}

// Fetches the process three times before the first delta, as the game does.
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
	} else {
		SetAvoidanceDisabled(true);
	}
}

void PlayerCharacter::SetAIControlledFromPosition(bool abTF) {
	bAiControlledFromPos = abTF;
	if (GetUnderAIControl()) {
		SetAvoidanceDisabled(false);
	} else {
		SetAvoidanceDisabled(true);
	}
}

float PlayerCharacter::GetModifier(ModifierGroup eGroup, ActorValue::Index aiWhich) {
	float fValue = 0.0f;
	switch (eGroup) {
	case TEMPORARY:
		fValue = TemporaryActorValueModifiers[aiWhich];
		break;
	case SCRIPTS:
		fValue = ScriptActorValueModifiers[aiWhich];
		break;
	case DAMAGE:
		if (aiWhich == ActorValue::HEALTH) {
			fValue = fHealthModifier;
		} else {
			fValue = DamageActorValueModifiers[aiWhich];
		}
		break;
	}
	return fValue;
}

// Adds afValue to the modifier of aiWhich in group eGroup, clamped by aeClampStyle; -1 is no actor value. A damage
// change is first clamped against the actor's value; health keeps its damage in fHealthModifier.
void PlayerCharacter::AlterModifier(ModifierGroup eGroup, ActorValue::Index aiWhich, float afValue,
	ModifierList::ClampStyle aeClampStyle) {
	switch (eGroup) {
	case TEMPORARY:
		if (aiWhich != -1) {
			TemporaryActorValueModifiers[aiWhich] =
				ModifierList::ModifyAndClamp(TemporaryActorValueModifiers[aiWhich], afValue, aeClampStyle);
		}
		break;
	case SCRIPTS:
		if (aiWhich != -1) {
			ScriptActorValueModifiers[aiWhich] =
				ModifierList::ModifyAndClamp(ScriptActorValueModifiers[aiWhich], afValue, aeClampStyle);
		}
		break;
	case DAMAGE: {
		float fDamage = Actor::CheckClampDamageModifier(static_cast<ActorValueOwner*>(this), aiWhich, afValue);
		if (aiWhich == ActorValue::HEALTH) {
			fHealthModifier = ModifierList::ModifyAndClamp(fHealthModifier, fDamage, aeClampStyle);
		} else if (aiWhich != -1) {
			DamageActorValueModifiers[aiWhich] =
				ModifierList::ModifyAndClamp(DamageActorValueModifiers[aiWhich], fDamage, aeClampStyle);
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
	} else if (!b3rdPerson) {
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
	ItemChange* v2;
	TESBoundObject* apObject;
	BaseProcess* currentAIProcess;
	if (ArrowProjectile::GetArrowCount() > 0) {
		currentAIProcess = MobileObject::GetCurrentAIProcess();
		if (currentAIProcess) {
			if (currentAIProcess->GetCurrentAmmo()) {
				v2 = currentAIProcess->GetCurrentAmmo();
				apObject = v2->ItemChange::GetItemChangeObject();
				if (apObject) {
					ProcessLists::DeleteAttachedArrows(apObject, 2147483647, this, 1, 1);
				}
			}
		}
	}
}

void PlayerCharacter::DestroyMouseSpring() {
	if (spGrabSpring.m_pObject) {
		bhkWorld* pw;
		TESObjectCELL* pcell = GetParentCell();
		if (pcell) {
			pw = pcell->GetbhkWorld();
		} else {
			pw = 0;
		}
		bhkMouseSpringAction* v1 = spGrabSpring.m_pObject;
		v1->Remove();
		if (pw) {
			pw->MarkForWrite();
		}
		hkpEntity* Entity = spGrabSpring.m_pObject->GetEntity();
		if (Entity) {
			Entity->activate();
		}
		if (pw) {
			pw->MarkForWrite();
		}
	}
	RemovePlayerAction(PLAYER_ACTION_Z_KEY_OBJECT, 0);
	spGrabSpring.operator=(static_cast<bhkMouseSpringAction*>(0));
	eGrabType = PlayerCharacter::GRAB_NONE;
	fGrabObjectWeight = 0.0f;
	if (pGrabbedObject) {
		if (pGrabbedObject->IsReference()) {
			Script::SetActionFlag(0, &pGrabbedObject->m_Extra, SCRIPT_ACTION_ONRELEASE);
		}
	}
	pGrabbedObject = 0;
}

// Moves the grab spring's target to the point fGrabDistance ahead of the eyes (shortened to the first hit of a ray cast
// along the look direction), or drops the object: when the spring or the object is gone, when the player stands on it,
// when telekinesis sees something else between the eyes and it, or when it is too far from the target and held by
// heavier contacts. afTimeDelta is unused. The collector's destructor runs where its scope ends, after
// DestroyMouseSpring on each early return.
void PlayerCharacter::UpdateMouseSpring(float afTimeDelta) {
	if (!pGrabbedObject || !spGrabSpring.operator->() || !spGrabSpring.operator->()->GetWorldObject()) {
		DestroyMouseSpring();
		return;
	}
	bhkCharacterController* pCharController = static_cast<bhkCharacterController*>(GetCharController());
	if (pCharController) {
		bhkRigidBody* pSupportBody = pCharController->GetSupportBody();
		if (pSupportBody == spGrabSpring.operator->()->GetWorldObject() && pCharController->GetSupportCount() == 1) {
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
	hkpEntity* pEntity = spGrabSpring.operator->()->GetEntity();
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
		for (int32_t i = 0; fHitFraction == 1.0; ++i) {
			if (i >= pCollector->getHits().getSize()) {
				break;
			}
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
	kTransform.~hkTransform();
	::new (&kTransform) hkTransform;
	spGrabSpring.operator->()->GetMousePositionInWorld(kWorld);
	spGrabSpring.operator->()->GetMousePositionInRBLocal(kLocal);
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
				} else {
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
			NiAVObject* pObject;
			if (pCollisionObject) {
				pObject = pCollisionObject->GetSceneGraphObject();
			} else {
				pObject = nullptr;
			}
			while (pObject && pObject != pNode) {
				if (pObject == pRefNode) {
					bComplex = true;
				}
				pObject = pObject->GetParent();
			}
		}
	}

	if (pRefNode && bComplex && fZKeyComplexHelperMinDistance.Float() < fSpringDistance) {
		float* pHelperScale = &fZKeyComplexHelperScale.Float();
		float* pHelperMinDistance = &fZKeyComplexHelperMinDistance.Float();
		float fHelperScale = (fSpringDistance - *pHelperMinDistance) * *pHelperScale / fSpringDistance;
		float fWeightMin = fZKeyComplexHelperWeightMin.Float();
		float fWeightMax = fZKeyComplexHelperWeightMax.Float();
		float fWeight = BSMIN(fWeightMax, BSMAX(fWeightMin, bhkUtilFunctions::GetTotalMass(pRefNode)));
		float fWeightLerp = FLerp(0.0f, 1.0f, fWeightMax, fWeightMin, fWeight);
		// Multiplied by -1.0 as a double, then by the float weight, and stored as a float.
		float fPull = static_cast<float>(fHelperScale * -1.0 * fWeightLerp);
		hkSimdReal kPull;
		kPull.~hkSimdReal();
		::new (&kPull) hkSimdReal(fPull);
		kDiff.mul4(kPull);
		if (bArticulated || pGrabbedObject->IsActor()) {
			NiPoint3 kVelocity;
			HK2NI(kVelocity, kDiff);
			TESHavokUtilities::AddVelocity(pRefNode, kVelocity, false);
		} else {
			hkpEntity* pSpringEntity = spGrabSpring.operator->()->GetEntity();
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
	spGrabSpring.operator->()->SetMousePosition(kTarget);
}

void PlayerCharacter::SetSpeaking(bool abValue) {
	bSpeaking = (bool)((uint8_t)abValue);
}

float PlayerCharacter::GetRockItLauncherWeight() const {
	return fRockItLauncherAmmoWeight;
}

ItemChange* PlayerCharacter::GetRandomRockItAmmoItem() {
	ItemChange* preturnAmmo = 0;
	if (!RockItLauncherAmmoList.IsEmpty()) {
		ExtraDataList* apExtra;
		uint32_t v1 = Random();
		ItemChange* prandomItem = RockItLauncherAmmoList.GetAt(v1 % RockItLauncherAmmoList.iSize);
		if (prandomItem->pExtraObjectList) {
			apExtra = prandomItem->pExtraObjectList->m_item;
		} else {
			apExtra = 0;
		}
		return RemoveRockItAmmo(prandomItem->pContainerObj, apExtra, 1, 1);
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

NiPoint3* PlayerCharacter::GetCamera3rdPersonShoulderOffset(NiPoint3* apResult) {
	*apResult = kCamera3rdPersonShoulderOffset;
	return apResult;
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
	fSitHeadingDelta = (float)0.0;
}

void PlayerCharacter::ClearAutoAim() {
	pAutoAimActor = nullptr;
	BulletAutoAim = NiPoint3_ZERO;
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
	bBlockActivate = (bool)((uint8_t)abVal);
	if (abVal) {
		fBlockActivateTimer = 5.0f;
	} else {
		fBlockActivateTimer = (float)0.0;
	}
}

bool PlayerCharacter::GetWant3rdPerson() {
	return (bool)((uint8_t)bWant3rdPerson);
}

char PlayerCharacter::GetPCControlFlags() {
	return (int8_t)ucControlsDisabled;
}

uint32_t PlayerCharacter::GetTeammateCount() {
	return (uint32_t)((int32_t)iTeammateCount);
}

bool PlayerCharacter::GetInChargen() {
	return (bool)((uint8_t)bChargen);
}

void PlayerCharacter::SetAIControlledPackage(bool abTF) {
	bAiControlledPackage = (bool)((uint8_t)abTF);
}

void PlayerCharacter::SetDisableFastTravelResetsOnCellChange(char a2) {
	uint8_t ucFastTravelFlags;
	uint8_t v3;
	if (a2) {
		bCanFastTravel |= 2u;
		ucFastTravelFlags = bCanFastTravel;
	} else {
		bCanFastTravel &= (~2u);
		v3 = bCanFastTravel;
	}
	uint8_t v4 = bCanFastTravel;
}

void PlayerCharacter::SetPCCanUsePowerArmor(bool abVal) {
	bCanUsePowerArmor = (bool)((uint8_t)abVal);
}

void* PlayerCharacter::GetCameraRigidBody() {
	return (&spCameraRigidBody)->operator->();
}

bool PlayerCharacter::GetMegatonDestroyed() {
	return (bool)((uint8_t)bMegatonDestroyed);
}

void PlayerCharacter::SetLockpickControlsDisable(char a2) {
	uint8_t ucFastTravelFlags;
	uint8_t v3;
	if (a2) {
		bCanFastTravel |= 4;
		ucFastTravelFlags = bCanFastTravel;
	} else {
		bCanFastTravel &= (~4);
		v3 = bCanFastTravel;
	}
	uint8_t v4 = bCanFastTravel;
}

void PlayerCharacter::SetAllowEGMCacheClear(bool abAllow) {
	bAllowEGMCacheClear = (bool)((uint8_t)abAllow);
}

MagicShaderHitEffect* PlayerCharacter::GetMagicShaderHitEffect() {
	return pInventoryWeaponEffect;
}

void PlayerCharacter::SetTelekinesisDistance(float a2) {
	fGrabDistance = a2;
}

float PlayerCharacter::GetGrabDistance() {
	return fGrabDistance;
}

bool PlayerCharacter::CanPCUsePowerArmor() {
	return (bool)((uint8_t)bCanUsePowerArmor);
}

bool PlayerCharacter::IsPlayerResistingArrest() {
	return (bool)((uint8_t)bBeingChased);
}

bool PlayerCharacter::IsGreetingTimerClear() {
	return (bool)((uint8_t)(fLastHelloTime < 0.0));
}

int32_t PlayerCharacter::GetSandManDetectionValue() {
	return (int32_t)iSandmanDetection;
}

void PlayerCharacter::SetInventoryWeaponEffect(MagicShaderHitEffect* apEffect) {
	pInventoryWeaponEffect = apEffect;
}

bool PlayerCharacter::GetAIControlledFromPosition() {
	return (bool)((uint8_t)bAiControlledFromPos);
}

void PlayerCharacter::AddActorToPlayerCombatList(Actor* apActor) {
	if (!pCombatGroup) {
		pCombatGroup = pCombatManager->CreateCombatGroup();
		pCombatGroup->AddMember(this);
	}
	pCombatGroup->AddTarget(apActor);
}

int32_t PlayerCharacter::GetScriptAllowedFastTravel() {
	return (int32_t)((uint8_t)((bCanFastTravel & 1) != 0));
}

bool PlayerCharacter::GetBlockActivate() {
	return (bool)((uint8_t)bBlockActivate);
}

MagicItem* PlayerCharacter::GetSelectedSpell() {
	return pSelectedSpell;
}

bool PlayerCharacter::CheckInvulnerable(ActorValue::Index aiWhich, float afValue) {
	if (!static_cast<MagicTarget*>(this)->IsInvulnerable()) {
		return (bool)1;
	}
	if (!(afValue < 0.0)) {
		if ((afValue > 0.0) && (((int32_t)aiWhich) == 54)) {
			return (bool)0;
		}
	} else {
		if ((((int32_t)aiWhich) == 16) || ((((int32_t)aiWhich) > 24) && (((int32_t)aiWhich) <= 31))) {
			return (bool)0;
		}
	}
	return (bool)1;
}

int32_t PlayerCharacter::GetNumberActorsInCombat() {
	if (pCombatGroup) {
		return (int32_t)pCombatGroup->GetTargetCount();
	} else {
		return 0;
	}
}

void PlayerCharacter::SetNeedResetHardcoreTimer(bool aNeedReset) {
	bResetHardcoreTimers = (bool)((uint8_t)aNeedReset);
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
	return (bool)((uint8_t)bResetHardcoreTimers);
}

void PlayerCharacter::SetActorInSneakRange(bool abflag) {
	bActorinSneakRange = (uint8_t)abflag;
}

char PlayerCharacter::IsShowQuestItems() {
	return (int8_t)((uint8_t)bShowQuestItemsInInventory);
}

int32_t PlayerCharacter::GetAmountSoldStolen() {
	return iAmountStolenSold;
}

void PlayerCharacter::SetLastModifiedFactionReputation(TESReputation* a2) {
	pReputationUpdate = a2;
}

uint32_t PlayerCharacter::GetCaravanCapWinnings() {
	return (uint32_t)((int32_t)iCaravanCapWinnings);
}

void PlayerCharacter::ToggleKillCamMode(KILL_CAM_TYPE arg0) {
	KILL_CAM_TYPE a2 = GetKillCamMode();
	SetKillCamMode(arg0);
	if (a2 != GetKillCamMode() && IsInSlowMo()) {
		ResetKillCam(a2, 1);
	}
}

void* PlayerCharacter::GetBhkMouseSpringAction() {
	return spGrabSpring.m_pObject;
}

PlayerCharacter::GrabbingType PlayerCharacter::GetGrabMode() {
	return static_cast<PlayerCharacter::GrabbingType>((uint32_t)eGrabType);
}

void PlayerCharacter::StopHeartbeatSound() {
	(&StatusSoundHandle)->FadeOutAndRelease(0x3E8u);
}

void PlayerCharacter::SetLastGreetingTime() {
	fLastHelloTime = (float)((float)BSGetTickCount());
}

void PlayerCharacter::SetLastKnownGoodPosition() {
	TESObjectCELL* v1;
	TESObjectCELL* parentCell;
	LastKnownGoodPosition = *TESObjectREFR::QLocationOnReference();
	if (TESObjectREFR::GetParentCell() && ((v1 = TESObjectREFR::GetParentCell()), v1->GetWorldSpace())) {
		parentCell = TESObjectREFR::GetParentCell();
		pLastKnownGoodLocation = parentCell->GetWorldSpace();
	} else {
		pLastKnownGoodLocation = TESObjectREFR::GetParentCell();
	}
}

void PlayerCharacter::ModPlayerGreetTimer() {
	if (bGreetingPlayer) {
		fGreetTimer = (float)((&appTimer)->GetSeconds() + fGreetTimer);
	}
}

bool PlayerCharacter::IsAMurderer() {
	return (((uint8_t)cMurder) != 0);
}

void PlayerCharacter::SetIsAMurderer() {
	cMurder = 1;
	IncNumCrimes(PackageNS::CRIME_TYPE_MURDER, 1);
}

void PlayerCharacter::IssueStealWarning() {
	++iNumberofStealWarnings;
	if (fStealWarningTimer == (-1.0)) {
		fStealWarningTimer = (float)((&::fWarningTimer)->Float());
	}
}

void PlayerCharacter::IssuePickPocketWarning() {
	++iNumberofPickpocketWarnings;
	if (fPickPocketWarningTimer == (-1.0)) {
		fPickPocketWarningTimer = (float)((&::fWarningTimer)->Float());
	}
}

char PlayerCharacter::IsPlayingCombatAttackSound() {
	return (int8_t)((uint8_t)bSpeaking);
}

void PlayerCharacter::UpdateTemp3rdPerson() {
	if (bTemp3rdPerson &&
		((((pCurrentProcess->GetAnimAction() == ((-1))) && (!pCurrentProcess->GetKnockState())) &&
			 (!PlayerCharacter::bVanityMode)) &&
			(!PlayerCharacter::bIsAutoVanityMode))) {
		bTemp3rdPerson = (bool)0;
		if (bTemp3rdPersonSwitchBack) {
			SetFirstPerson(1);
		}
		bTemp3rdPersonSwitchBack = (bool)0;
	}
}

void PlayerCharacter::Setup3rdPersonCameraCaster() {
	bhkWorldM* v1;
	TESObjectCELL* parentCell;
	TESObjectCELL* v3;
	bhkWorld* havokWorld;
	if (pCameraCaster) {
		if (TESObjectREFR::GetParentCell()) {
			v1 = static_cast<bhkWorldM*>(pCameraCaster->GetWorld());
			parentCell = TESObjectREFR::GetParentCell();
			if (static_cast<const void*>(v1) != static_cast<const void*>(parentCell->GetbhkWorld())) {
				v3 = TESObjectREFR::GetParentCell();
				havokWorld = v3->GetbhkWorld();
				pCameraCaster->Add(havokWorld);
			}
		}
	}
}

bool PlayerCharacter::ToggleFlyCam(bool abFreezeTime) {
	uint8_t v6 = (uint8_t)(!appMain->GetFlyCamera());
	appMain->SetFlyCamera(v6);
	if (v6 && abFreezeTime) {
		appMain->SetFreezeTime(1);
	} else {
		appMain->SetFreezeTime(0);
	}
	FOHUDEffectManager* unk0178 = Interface::GetHUDEffects();
	unk0178->bHideHUD = v6;
	if (!v6) {
		return (bool)v6;
	}
	const NiPoint3* v3 = &GetLocationOnReference();
	UFOCameraPos.x = v3->x;
	UFOCameraPos.y = v3->y;
	UFOCameraPos.z = v3->z;
	UFOCameraPos.z = (float)((TESObjectREFR::GetScale() * fEyeHeight) + UFOCameraPos.z);
	fUFOCameraHeading = TESObjectREFR::GetAngleOnReference().z;
	fUFOCameraPitch = TESObjectREFR::GetAngleOnReference().x;
	return (bool)v6;
}

Actor* PlayerCharacter::GetBumpedActor() {
	NiAVObject* niObject;
	int32_t v6 = 0;
	void* charController = MobileObject::GetCharController();
	if (!charController) {
		return static_cast<Actor*>(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(v6)));
	}
	bhkCharacterController* bumpedCharacterController =
		static_cast<bhkCharacterController*>(charController)->GetBumpedCharacterController();
	if (bumpedCharacterController) {
		niObject = bumpedCharacterController->GetNiObject();
	} else {
		niObject = nullptr;
	}
	TESObjectREFR* referenceFor3D = TESObjectREFR::FindReferenceFor3D(niObject);
	if (referenceFor3D && referenceFor3D->IsActor()) {
		return static_cast<Actor*>(referenceFor3D);
	} else {
		return nullptr;
	}
	return static_cast<Actor*>(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(v6)));
}

MediaLocationController* PlayerCharacter::GetMediaLocationController() {
	if (pClosestAudioMarkerInfo && pClosestAudioMarkerInfo->pMarkerData) {
		return static_cast<MediaLocationController*>(static_cast<AudioMarker*>(pClosestAudioMarkerInfo->pMarkerData)
				->GetMediaLocationController());
	}
	AudioMarkerInfo* closestAudioMarkerInfo = GetClosestAudioMarkerInfo();
	if (closestAudioMarkerInfo && closestAudioMarkerInfo->pMarkerData) {
		return static_cast<MediaLocationController*>(static_cast<AudioMarker*>(closestAudioMarkerInfo->pMarkerData)
				->GetMediaLocationController());
	} else {
		return nullptr;
	}
}

char PlayerCharacter::GetInSleepPackage() {
	return (int8_t)((uint8_t)bAiControlledToPos);
}

int32_t PlayerCharacter::GetNumberofNotes() {
	return (&listNotes)->ItemsInList();
}

void PlayerCharacter::ReturnRockItAmmo() {
	ItemChange** v1;
	uint32_t i;
	InventoryChanges* inventoryChanges = InventoryChanges::GetInventoryChanges(pPlayer);
	for (i = 0; inventoryChanges &&
		(i < ((uint32_t)((int32_t)(&RockItLauncherAmmoList)->BSSimpleArray<ItemChange*>::GetSize())));
		++i) {
		v1 = &(&RockItLauncherAmmoList)->GetAt(i);
		inventoryChanges->AddChangeItem(*v1, 1);
	}
	(&RockItLauncherAmmoList)->Clear(1);
	ResetRockItAmmoWeight(0);
}

uint32_t PlayerCharacter::GetCaravanCapLosses() {
	return (uint32_t)((int32_t)iCaravanCapLosses);
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
	MagicItem** pPotions = static_cast<MagicItem**>(__builtin_alloca_with_align(sizeof(MagicItem*) * iMax, 128));
	uint32_t* pEffectCounts = static_cast<uint32_t*>(__builtin_alloca_with_align(4 * iMax, 128));
	if (!pPotions || !pEffectCounts) {
		return iCount;
	}
	for (int32_t i = 0; i < iMax; ++i) {
		pPotions[i] = nullptr;
		pEffectCounts[i] = 0;
	}
	BSSimpleList<ActiveEffect*>* pEffects = GetActiveEffectList();
	for (; pEffects && !pEffects->IsEmpty(); pEffects = pEffects->GetNext()) {
		MagicItem* pPotion;
		if (pEffects->GetItem()) {
			pPotion = pEffects->GetItem()->GetSpell();
		} else {
			pPotion = nullptr;
		}
		if (pPotion) {
			if (pPotion->GetSpellType() == MagicSystem::POTION) {
				bool bQueued = EatDrinkItems && EatDrinkItems->IsInList(pPotion);
				if (!bQueued) {
					bool bFound = false;
					for (int32_t k = 0; (k < iMax) && !bFound; ++k) {
						if (!pPotions[k]) {
							pPotions[k] = pPotion;
							pEffectCounts[k] = 1;
							++iCount;
							bFound = true;
						} else if (pPotions[k] == pPotion) {
							if (pEffectCounts[k] < static_cast<uint32_t>(pPotions[k]->ItemsInList())) {
								++pEffectCounts[k];
								bFound = true;
							}
						}
					}
				}
			}
		}
	}
	return iCount;
}

void PlayerCharacter::ClearTeammates() {
	iTeammateCount = (uint32_t)0;
	(&pListOfTeammates)->RemoveAll();
}

bool PlayerCharacter::GetTemp1stPersonSwitchBack() {
	return (bool)((uint8_t)bTemp1stPersonSwitchBack);
}

bool PlayerCharacter::AddCaravanCard(TESCaravanCard* apCard) {
	if (pInactiveListofCaravanCards->IsInList(apCard) || pActiveListofCaravanCards->IsInList(apCard)) {
		return 1;
	}
	apCard->bIsActive = 0;
	pInactiveListofCaravanCards->AddHead(apCard);
	return 1;
}

void PlayerCharacter::SetActorBaseValue(ActorValue::Index a1, int32_t a2) {
	bool v7 = 0;
	float v6 = GetBaseValueOverride(a1, v7);
	if (v7) {
		SetBaseValueOverride(a1, static_cast<float>(a2));
	} else {
		TESActorBase* ActorBase = GetActorBase();
		ActorBase->SetActorValue(a1, a2);
	}
	Interface::UpdateActorValue(a1);
	CheckSkillLevelChange(a1, 1);
}

void PlayerCharacter::StartKillcamForActor(TESObjectREFR* a1, float a2, bool a3, int32_t a4) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1a, std::nullopt, std::nullopt);
	if (eKillCameraSetting && (!GetParentCell() || GetParentCell()->iFormID != 0x161E98)) {
		BSSoundHandle th_s;
		BSAudio* v5 = &BSAudio::QInstance();
		BSSoundHandle* arHandle = v5->GetSoundHandleByName(&a1a, "UIPopUpQuestNew", 289u);
		th_s.operator=(*arHandle);
		th_s.Play(0);
		if (!a1 || pVATS.eMode) {
			goto LABEL_21;
		}
		if (a3 && fKillCamCooldown <= 0.0) {
			fKillCamCooldown = fKillCameraCooldown.Float();
		} else {
			if (a3) {
			LABEL_21:;
				return;
			}
		}
		fTimeInSlowMoCam = a2;
		if (eKillCameraSetting == 2) {
			if (b3rdPersonSaved) {
				bVATSReturnPlayerTo1stPerson = !bsave3rdPerson;
			} else {
				bVATSReturnPlayerTo1stPerson = !pPlayer->Is3rdPerson();
			}
			pVATS.AppendCamTarget(a1, a4);
			if (GetIronSights()) {
				Interface::SetGunScopeVisible(0);
			}
		} else {
			fTimeInSlowMoCam = fTimeInSlowMoCam * 0.4000000059604645;
		}
		if (!pPlayer->GetActorValue(ActorValue::TURBO)) {
			BSTimer::SaveGlobalTimeMultiplier();
		}
		goto LABEL_21;
	}
}

bool PlayerCharacter::AddEatDrinkItem(MagicItem* apItem) {
	if (apItem->GetSpellType() == MagicSystem::POTION) {
		if (GetCurrentPotionCount() < 100) {
			EatDrinkItems->AddHead(apItem);
			CastAlchemy(apItem, 1);
		}
	} else {
		CastAlchemy(apItem, 1);
	}
	ProcessEffectsFromItem(apItem);
	EvaluateSpellConditions();
	UpdateTarget(0.01f);
	return 1;
}

void PlayerCharacter::DoIronSightsZoom(float afDelta) {
	if (bIronSightsZoomEnable.Bool()) {
		TESObjectWEAP* pweapon;
		float fextraZoom;
		if (pCurrentProcess->GetCurrentWeapon()) {
			pweapon = static_cast<TESObjectWEAP*>(pCurrentProcess->GetCurrentWeapon()->pContainerObj);
		} else {
			pweapon = 0;
		}
		float fdefault = BSSceneGraph::fDefaultWorldFOV.Float();
		float f1stdefault = BSSceneGraph::fDefault1stPersonFOV.Float();
		float fpipboy = BSSceneGraph::fPipboy1stPersonFOV.Float();
		float fzoom = fIronSightsZoomDefault.Float();
		float fchangeTime = fIronSightsFOVTimeChange.Float();
		[[maybe_unused]] float fzoomScale = 1.0f;
		SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
		WorldSceneGraph->GetCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
		if (pweapon) {
			if (pCurrentProcess->GetWeaponDrawn()) {
				float a1 = pweapon->GetIronSightFOV();
				fzoom = std::fabs(a1);
				ItemChange* pchange = pCurrentProcess->GetCurrentWeapon();
				fextraZoom = 0.0f;
				if (pchange->HasModEffectActive(WEAPON_MOD_IRON_SITES, fextraZoom)) {
					fzoom = fzoom - fextraZoom;
				}
			}
		}
		if (IsPipboyActive()) {
			if (GetIronSights()) {
				SetIronSights(0, 0, 0);
			}
			f1stPersonFOV = fpipboy;
			fWorldFOV = fdefault;
		} else {
			if (PlayerCharacter::bVanityMode && bFOVTargetReached) {
				SceneGraph* v3 = Main::GetWorldSceneGraph();
				double CameraFOV = v3->GetCameraFOV();
				if (fdefault != CameraFOV) {
					SetFOV(fdefault);
				}
			} else {
				float ftarget = fdefault;
				float f1target = f1stdefault;
				float fdelta = afDelta * 30.0 / fchangeTime;
				if (GetIronSights() && fzoom > 5.0 &&
					((GetAnimAction() != ANIM_ACTION_RELOAD && GetAnimAction() != ANIM_ACTION_RELOAD_LOOP) ||
						pVATS.eMode == VATS::VATS_PLAYBACK)) {
					ftarget = fzoom;
					f1target = fzoom / fdefault * f1stdefault;
				}
				if ((pVATS.eMode == VATS::VATS_MENU_MODE || pVATS.eMode == VATS::VATS_SCAN_MODE) &&
					(pVATSTarget && bWant3rdPerson == b3rdPerson)) {
					NiAVObject* pobj = (NiAVObject*)pVATSTarget->Get3D();
					const NiBound* rbound = (const NiBound*)&pobj->GetWorldBound();
					NiPoint3 src = p1stPersonCamera->GetWorldTranslate() - rbound->GetCenter();
					float fdist = src.Length();
					double fTargetRadius = rbound->m_fRadius;
					double v5 = fDlgFocus.operator float();
					float ftan = v5 * fTargetRadius / fdist;
					float* v6 = &fVATSTargetFOVMultFar.Float();
					float ffovMultDelta = *v6 - fVATSTargetFOVMultNear.Float();
					float* v7 = &fVATSTargetFOVMultFarDist.Float();
					float fdistMult = fdist / *v7;
					if (fdistMult > 1.0) {
						fdistMult = 1.0f;
					}
					float* v8 = &fVATSTargetFOVMultNear.Float();
					float ffovMult = ffovMultDelta * fdistMult + *v8;
					ftarget = std::atan(ftan) * ffovMult;
					float* v9 = &fVATSTargetFOVMinFOV.Float();
					if (*v9 > static_cast<double>(ftarget)) {
						ftarget = fVATSTargetFOVMinFOV.Float();
					}
					f1target = ftarget;
					float* v10 = &BSSceneGraph::fDefaultFOV.Float();
					if (*v10 <= static_cast<double>(f1target)) {
						f1target = BSSceneGraph::fDefaultFOV.Float();
					}
					static NonLinearInterpolator* pVATSWorldZoomInterpolator = [&] {
						NonLinearInterpolator* v16;
						NonLinearInterpolator* apMemory =
							static_cast<NonLinearInterpolator*>(ourMalloc(sizeof(NonLinearInterpolator)));
						if (apMemory) {
							v16 = ::new (apMemory) NonLinearInterpolator;
						} else {
							v16 = 0;
						}
						return v16;
					}();
					static NonLinearInterpolator* pVATSViewmodelZoomInterpolator = [&] {
						NonLinearInterpolator* v15;
						NonLinearInterpolator* v20 =
							static_cast<NonLinearInterpolator*>(ourMalloc(sizeof(NonLinearInterpolator)));
						if (v20) {
							v15 = ::new (v20) NonLinearInterpolator;
						} else {
							v15 = 0;
						}
						return v15;
					}();
					if (poldVATSTarget != pVATSTarget) {
						float fcamTime = 0.75f;
						if (pVATS.eMode == VATS::VATS_TARGET_SELECT) {
							fcamTime = fVATSTargetSelectCamPanTime.Float();
							pVATSWorldZoomInterpolator->Setup(fWorldFOV, ftarget, fcamTime);
						} else {
							if (pVATS.eMode == VATS::VATS_MENU_MODE) {
								fcamTime = fVATSCamZoomInTime.Float();
							}
							pVATSWorldZoomInterpolator->Setup(fWorldFOV, ftarget, fcamTime);
						}
						pVATSViewmodelZoomInterpolator->Setup(f1stPersonFOV, f1target, fcamTime);
						poldVATSTarget = pVATSTarget;
					}
					float a1b = appTimer.fDelta;
					pVATSWorldZoomInterpolator->Interpolate(a1b);
					float a1c = appTimer.fDelta;
					pVATSViewmodelZoomInterpolator->Interpolate(a1c);
					fWorldFOV = pVATSWorldZoomInterpolator->GetCurrent();
					f1stPersonFOV = pVATSViewmodelZoomInterpolator->GetCurrent();
					if (pVATSViewmodelZoomInterpolator->GetFinished()) {
						VATSMenu::SetHeadingReached(1);
						bFOVTargetReached = 1;
					} else {
						VATSMenu::SetHeadingReached(0);
						bFOVTargetReached = 0;
					}
				} else {
					if (pVATS.eMode == VATS::VATS_PLAYBACK) {
						poldVATSTarget = 0;
						fWorldFOV = ftarget;
						f1stPersonFOV = f1target;
						bFOVTargetReached = 1;
					} else {
						poldVATSTarget = 0;
						if (!(ftarget < static_cast<double>(fWorldFOV))) {
							if (ftarget > static_cast<double>(fWorldFOV)) {
								fWorldFOV = fWorldFOV + fdelta;
								if (ftarget < static_cast<double>(fWorldFOV)) {
									fWorldFOV = ftarget;
								}
							}
						} else {
							fWorldFOV = fWorldFOV - fdelta;
							if (ftarget > static_cast<double>(fWorldFOV)) {
								fWorldFOV = ftarget;
							}
						}
						if (!(f1target < static_cast<double>(f1stPersonFOV))) {
							if (!(f1target > static_cast<double>(f1stPersonFOV))) {
								bFOVTargetReached = 1;
							} else {
								f1stPersonFOV = f1stPersonFOV + fdelta;
								if (!(f1target < static_cast<double>(f1stPersonFOV))) {
									bFOVTargetReached = 0;
								} else {
									f1stPersonFOV = f1target;
									bFOVTargetReached = 1;
								}
							}
						} else {
							f1stPersonFOV = f1stPersonFOV - fdelta;
							if (!(f1target > static_cast<double>(f1stPersonFOV))) {
								bFOVTargetReached = 0;
							} else {
								f1stPersonFOV = f1target;
								bFOVTargetReached = 1;
							}
						}
					}
				}
			}
		}
	}
}

void PlayerCharacter::CreateMouseSpring(TESObjectREFR* apObject, PlayerCharacter::GrabbingType aeGrabType,
	float afDistance) {
	hkVector4 hkv;
	CFilter v33;
	NiPoint3 v49;
	hkVector4 tmphk;
	alignas(16) NiPoint3 mousePoint;
	CFilter v65;
	CFilter fltr;
	hkVector4 hkvCenterOfMass;
	float fmassTotal;
	if (bAllowHavokGrabTheLiving.Bool() || (!apObject->IsActor() || apObject->IsDead(0))) {
		Projectile* pprojectile;
		// A grenade projectile that is a live mine is never grabbed.
		if (!apObject ||
			(!apObject->IsProjectile() ||
				((pprojectile = static_cast<Projectile*>(apObject),
					 pprojectile->GetProjectileType() != Projectile::PROJECTILE_TYPE_GRENADE) ||
					(!pprojectile->IsMine() || pprojectile->IsFlagSet(Projectile::TURNED_OFF))))) {
			if (apObject) {
				if (apObject->GetParentCell()) {
					NiNode* pav = apObject->Get3D();
					bhkRigidBody* pbody = bhkRigidBody::FindHeaviestRigidBody(pav);
					if (pbody) {
						NiPoint3 point;
						NiPoint3 dir;
						int32_t bnormalGrabSuccess = BSXFlags::FindFlag(pav, BSXFlags::COMPLEX) || apObject->IsActor();
						bool v75 = bnormalGrabSuccess;
						this->pGrabbedObject = apObject;
						// The script action bit 0x80000 has no name in OBJECT_ACTION.
						Script::SetActionFlag(0, &this->pGrabbedObject->m_Extra, SCRIPT_ACTION_ONGRAB);
						this->eGrabType = aeGrabType;
						this->fGrabDistance = afDistance;
						hkMouseSpringActionCinfo v74;
						fmassTotal = 0.0f;
						if (v75 && apObject->IsActor()) {
							GetTotalMassData(pav, &hkvCenterOfMass, &fmassTotal);
						} else {
							pbody->GetCenterOfMassInWorld(hkvCenterOfMass);
						}
						this->GetEyeVector(point, dir, 1);
						char v69 = 0;
						bhkWorld* pw = apObject->GetParentCell()->GetbhkWorld();
						if (this->eGrabType == PlayerCharacter::GRAB_NORMAL) {
							if (pw) {
								if (!pSaveLoadGame->GetSaveGameLoading()) {
									if (Interface::IsActivatePickFuzzy() && ViewCaster::GetFuzzyBody()) {
										Interface::GetActivatePickLocation(&mousePoint);
										if (!pSaveLoadGame->GetSaveGameLoading()) {
											double v9 = (mousePoint - point).Length();
											this->fGrabDistance = v9;
										}
										v74.m_mousePositionInWorld.NI2HK(mousePoint);
										bhkRigidBody* pfuzzyBody = ViewCaster::GetFuzzyBody();
										v74.m_body = pfuzzyBody->operator*();
										v69 = 1;
									} else {
										bhkPickData pd;
										fltr.iFilter = 0;
										fltr.SetLayer(L_ITEMPICKER);
										// GetCollisionFilter returns its argument: the group is read from v65.
										this->GetCollisionFilter(&v65.iFilter);
										uint32_t Group = v65.GetGroup();
										fltr.SetGroup(Group);
										pd.SetFilter(fltr);
										pd.NiSetFrom(point);
										pd.NiSetLength(dir * this->fGrabDistance);
										if (pw->PickObject(pd)) {
											NiAVObject* ppickedAV;
											hkpRigidBody* prb = pd.GethkRigidBody();
											bhkCollisionObject* pcol = bhkCollisionObject::Getbhk(prb);
											if (pcol) {
												ppickedAV = pcol->m_pkSceneObject;
											} else {
												ppickedAV = 0;
											}
											TESObjectREFR* ppickedRef = TESObjectREFR::FindReferenceFor3D(ppickedAV);
											if (prb) {
												if (ppickedRef == apObject) {
													if (!v75) {
														bhkRigidBody* pbhkbody =
															static_cast<bhkRigidBody*>(bhkEntity::Getbhk(prb));
														pbhkbody->GetCenterOfMassInWorld(hkvCenterOfMass);
													}
													v69 = 1;
													v74.m_body = prb;
													this->fGrabDistance = this->fGrabDistance * pd.m_hitFraction;
													NiPoint3 springLoc = point + dir * this->fGrabDistance;
													v74.m_mousePositionInWorld.NI2HK(springLoc);
												}
											}
										}
									}
								}
							}
						}
						if (!v69) {
							v74.m_mousePositionInWorld.operator=(hkvCenterOfMass);
							if (v75) {
								hkVector4 addition;
								NiPoint3* v10 =
									::new (&v49) NiPoint3((&pav->GetWorldTransform())->m_Rotate * NiPoint3_UNIT_Z);
								NI2HKNS(addition, *v10);
								v74.m_mousePositionInWorld.ToHK(tmphk);
								tmphk.add4(addition);
								v74.m_mousePositionInWorld.operator=(tmphk);
							}
							if (!pSaveLoadGame->GetSaveGameLoading()) {
								NiPoint3 mousePoint_2;
								hkVector4* v11 = &v74.m_mousePositionInWorld.ToHK(tmphk);
								HK2NI(mousePoint_2, *v11);
								double v13 = (mousePoint_2 - point).Length();
								this->fGrabDistance = v13;
							}
							v74.m_body = pbody->operator*();
						}
						if (this->eGrabType == PlayerCharacter::GRAB_NORMAL) {
							this->AddPlayerAction(PLAYER_ACTION_Z_KEY_OBJECT, 20.0f, apObject);
						}
						hkpRigidBody* phkbody = v74.m_body;
						if (!v74.m_body->m_world ||
							(phkbody->getMotionType() == hkpMotion::MOTION_FIXED ||
								phkbody->getMotionType() == hkpMotion::MOTION_KEYFRAMED)) {
							this->DestroyMouseSpring();
						} else {
							bhkMouseSpringAction* v29;
							hkVector4 hktemp;
							bhkCharacterController* pcon = this->GetCharController();
							if (pcon) {
								float a2 = pcon->GetRadius();
								float v44 = HK2NI(a2) + 5.0;
								if (v44 > static_cast<double>(this->fGrabDistance)) {
									this->fGrabDistance = v44;
								}
							}
							if (this->eGrabType == PlayerCharacter::GRAB_TELEKINESIS) {
								if (v75) {
									v74.m_springDamping = fMagicTelekinesisComplexSpringDamping.Float();
									v74.m_springElasticity = fMagicTelekinesisComplexSpringElasticity.Float();
									v74.m_objectDamping = fMagicTelekinesisComplexObjectDamping.Float();
									v74.m_maxRelativeForce = fMagicTelekinesisComplexMaxForce.Float();
								} else {
									v74.m_springDamping = fMagicTelekinesisSpringDamping.Float();
									v74.m_springElasticity = fMagicTelekinesisSpringElasticity.Float();
									v74.m_objectDamping = fMagicTelekinesisObjectDamping.Float();
									v74.m_maxRelativeForce = fMagicTelekinesisMaxForce.Float();
								}
							} else {
								float fscaleForce = 1.0f;
								float v42 = 1.0f;
								if (this->pGrabbedObject->IsActor()) {
									float fminWeight = fZKeyMaxForceWeightLow.Float();
									float fmaxWeight = fZKeyMaxForceWeightHigh.Float();
									float fscaleHigh = fZKeyMaxForceScaleHigh.Float();
									float fscaleLow = fZKeyMaxForceScaleLow.Float();
									float a2a = bhkUtilFunctions::GetTotalMass(pav);
									float a2b = BSMAX(fminWeight, a2a);
									float ftotalWeight = BSMIN(fmaxWeight, a2b);
									fscaleForce = FLerp(fscaleLow, fscaleHigh, fmaxWeight, fminWeight, ftotalWeight) *
										fscaleForce;
									bool bonlyContactConstraints = 1;
									for (int32_t i = 0;; ++i) {
										if (i >= phkbody->m_constraintsMaster.getSize() || !bonlyContactConstraints) {
											break;
										}
										hkSmallArray<hkConstraintInternal>* v15 = &phkbody->m_constraintsMaster;
										hkpConstraintData* DataCalled = (&v15->operator[](i))->m_constraint->m_data;
										bonlyContactConstraints =
											DataCalled->getType() == hkpConstraintData::CONSTRAINT_TYPE_CONTACT;
									}
									if (bonlyContactConstraints) {
										v42 = v42 * 0.10000000149011612;
										fscaleForce = fscaleForce * 0.20000000298023224;
									}
								} else {
									CFilter* v16 = pbody->GetCollisionFilter(&v33);
									if (v16->GetLayer() == L_TRAP) {
										v42 = v42 * 0.10000000149011612;
										fscaleForce = fscaleForce * 0.5;
									}
								}
								v74.m_springDamping = fZKeySpringDamping.Float();
								float* v17 = &fZKeySpringElasticity.Float();
								v74.m_springElasticity = *v17 * v42;
								v74.m_objectDamping = fZKeyObjectDamping.Float();
								float* v18 = &fZKeyMaxForce.Float();
								v74.m_maxRelativeForce = *v18 * fscaleForce;
							}
							v74.m_mousePositionInWorld.ToHK(hktemp);
							const hkVector4* Translation = &phkbody->getTranslation();
							hktemp.sub4(*Translation);
							const hkTransform* a1 = &phkbody->getTransform();
							hkVector4* v20 = &v74.m_positionInRbLocal.ToHK(hkv);
							v20->setRotatedInverseDir(a1->m_rotation, hktemp);
							v74.m_positionInRbLocal.operator=(hkv);
							this->pGrabbedObject->GetParentCell()->GetbhkWorld()->MarkForWrite();
							bhkMouseSpringAction* v30 = static_cast<
								bhkMouseSpringAction*>(NiMemObject::operator new(sizeof(bhkMouseSpringAction)));
							if (v30) {
								v29 = ::new (v30) bhkMouseSpringAction(v74);
							} else {
								v29 = 0;
							}
							this->spGrabSpring.operator=(v29);
							bhkMouseSpringAction* v28 = this->spGrabSpring.m_pObject;
							bhkWorld* v21 = this->pGrabbedObject->GetParentCell()->GetbhkWorld();
							v28->Add(v21);
							this->pGrabbedObject->GetParentCell()->GetbhkWorld()->MarkForWrite();
							NiNode* v22 = apObject->Get3D();
							double TotalMass = bhkUtilFunctions::GetTotalMass(v22);
							this->fGrabObjectWeight = TotalMass;
						}
					} else {
						if (aeGrabType == PlayerCharacter::GRAB_INVENTORY_DROP) {
							this->pGrabbedObject = apObject;
							// The script action bit 0x80000 has no name in OBJECT_ACTION.
							Script::SetActionFlag(0, &this->pGrabbedObject->m_Extra, SCRIPT_ACTION_ONGRAB);
							this->eGrabType = PlayerCharacter::GRAB_INVENTORY_DROP;
							this->fGrabDistance = afDistance;
						}
					}
				}
			}
		}
	}
}

void PlayerCharacter::PlayMagicFailureSound(MagicSystem::School aeSchool) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a2, std::nullopt, std::nullopt);
	if (MagicFailureSounds[aeSchool].IsValid()) {
		MagicFailureSounds[aeSchool].Stop();
		NiPoint3 v8 = GetLocationOnReference();
		MagicFailureSounds[aeSchool].SetPosition(v8.x, v8.y, v8.z);
		NiNode* v2 = Get3D();
		MagicFailureSounds[aeSchool].SetObjectToFollow(v2);
		MagicFailureSounds[aeSchool].Play(0);
	} else {
		TESSound* psound = MagicSystem::GetMagicFailureSound(aeSchool);
		if (psound) {
			uint32_t CastingType = psound->iFormID;
			BSSoundHandle* arHandle = PlaySound(&a2, CastingType, 0, 2u, 1);
			MagicFailureSounds[aeSchool].operator=(*arHandle);
		}
	}
}

// Has each follower look for a new package; a follower whose package check changes the list restarts the walk from
// the head.
void PlayerCharacter::HaveFollowersRecheckPackages() {
	ExtraDataList* Extra = pPlayer->GetExtra();
	ExtraFollower* FollowerExtra = Extra->GetFollowerExtra();
	if (FollowerExtra && FollowerExtra->pActorlist) {
		BSSimpleList<Actor*>* pActorlist = FollowerExtra->pActorlist;
		while (pActorlist && pActorlist->m_item) {
			BSSimpleList<Actor*>* pnext = pActorlist->m_pkNext;
			Actor* pactor = pActorlist->m_item;
			if (pactor && pactor->GetCurrentAIProcess()) {
				BaseProcess* pprocess = pactor->GetCurrentAIProcess();
				pprocess->CheckforNewPackage(pactor, 1);
				if (pnext != pActorlist->m_pkNext) {
					pActorlist = FollowerExtra->pActorlist;
				} else {
					pActorlist = pnext;
				}
			} else {
				pActorlist = pnext;
			}
		}
	}
}

// Runs under kPerceivedActorsLock, which is an addition of this code and not part of the original game (see
// PlayerCharacter.h).
void PlayerCharacter::InsertPerceivedActor(Actor* apActor, bool abAttack, bool abHasLineSightNotDetection) {
	TESActorBaseData* pBaseData = static_cast<TESActorBaseData*>(apActor->GetActorBase());
	if (!pBaseData->GetIsGhost() && !apActor->IsDead(0)) {
		kPerceivedActorsLock.Lock(nullptr);
		CompassTarget* pdata = IsActorPerceived(apActor);
		if (pdata) {
			pdata->bIsUndetected = abHasLineSightNotDetection;
			pdata->bIsHostile = abAttack;
		} else {
			CompassTarget* pData;
			if (!pListofPercievedActors) {
				BSSimpleList<CompassTarget*>* v5;
				BSSimpleList<CompassTarget*>* v9 =
					static_cast<BSSimpleList<CompassTarget*>*>(ourMalloc(sizeof(BSSimpleList<CompassTarget*>)));
				if (v9) {
					v5 = ::new (v9) BSSimpleList<CompassTarget*>;
				} else {
					v5 = 0;
				}
				pListofPercievedActors = v5;
			}
			CompassTarget* apMemory = static_cast<CompassTarget*>(ourMalloc(sizeof(CompassTarget)));
			if (apMemory) {
				pData = ::new (apMemory) CompassTarget;
			} else {
				pData = 0;
			}
			CompassTarget* pData_2 = pData;
			pData->pTarget = apActor;
			pData_2->bIsHostile = abAttack;
			pData_2->bIsUndetected = abHasLineSightNotDetection;
			pListofPercievedActors->AddHead(pData_2);
		}
		kPerceivedActorsLock.Unlock();
	}
}

// Under kPerceivedActorsLock (see PlayerCharacter.h).
bool PlayerCharacter::IsDetectedByCompassTargetExceptActor(Actor* apActor) {
	bool bDetected = 0;
	kPerceivedActorsLock.Lock(nullptr);
	for (CompassTarget* pCompassTarget : pListofPercievedActors->ItemsUntilNull()) {
		Actor* pTarget = pCompassTarget->pTarget;
		if (pTarget && !pTarget->IsPlayerTeammate() && pTarget != apActor) {
			bool bLineOfSight = pTarget->LineOfSight(0, pPlayer, 1, 0, 0);
			if (pTarget->GetDetectionLevelAgainstActor(0, pPlayer, bLineOfSight, 0, 0, 0, 0) > 0) {
				bDetected = 1;
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

void PlayerCharacter::SetActorBaseValueF(ActorValue::Index a1, float a3) {
	bool v6 = 0;
	float v5 = GetBaseValueOverride(a1, v6);
	if (v6) {
		SetBaseValueOverride(a1, a3);
	} else {
		TESActorBase* ActorBase = GetActorBase();
		ActorBase->SetActorValue(a1, a3);
	}
	Interface::UpdateActorValue(a1);
	CheckSkillLevelChange(a1, 1);
}

TESObjectCELL* PlayerCharacter::GetWaterCell(float afCheckRadius) {
	char v16 = 0;
	if (afCheckRadius > 2048.0) {
		afCheckRadius = 2048.0f;
		v16 = 1;
	}
	TESObjectCELL* cell = GetParentCell();
	if (!cell || (!cell->GetInterior() && (!cell->HasWater() && TES::pInstance->pWorldSpace))) {
		NiPoint3 kPosition = (&data)->kPosition;
		int32_t aiX = FloatToInt(kPosition.x) >> 12;
		int32_t aiY = FloatToInt(kPosition.y) >> 12;
		NiPoint2 v10(static_cast<float>(aiX << 12), static_cast<float>(aiY << 12));
		float v6 = static_cast<double>(aiY << 12) + 4096.0;
		float v5 = static_cast<double>(aiX << 12) + 4096.0;
		NiPoint2 v11(v5, v6);
		int32_t v14 = 0;
		if (!(v10.x >= kPosition.x - afCheckRadius)) {
			v14 = v11.x <= kPosition.x + afCheckRadius;
		} else {
			v14 = -1;
		}
		if (v14) {
			cell = TES::pInstance->GetGridCellCell(v14 + aiX, aiY);
		}
		if (!cell || !cell->HasWater()) {
			int32_t v9;
			if (!(v10.y >= kPosition.y - afCheckRadius)) {
				v9 = v11.y <= kPosition.y + afCheckRadius;
			} else {
				v9 = -1;
			}
			if (v9) {
				cell = TES::pInstance->GetGridCellCell(aiX, v9 + aiY);
			}
			if (!cell || !cell->HasWater()) {
				if (v14) {
					if (v9) {
						cell = TES::pInstance->GetGridCellCell(v14 + aiX, v9 + aiY);
						if (!cell || !cell->HasWater()) {
							if (v16) {
								GridCell* FirstGridCellWithWater = TES::pInstance->FindFirstGridCellWithWater();
								if (FirstGridCellWithWater) {
									cell = FirstGridCellWithWater->pCell;
								}
							}
						}
					}
				}
			}
		}
	}
	if (cell) {
		if (!cell->HasWater()) {
			return 0;
		}
	}
	return cell;
}

bool PlayerCharacter::CanFastTravel() {
	float fCheckPerk;
	char msg[504];
	bool bretval = 0;
	bool bsearch = 0;
	bool Interior = pPlayer->GetInterior();
	if (ReferenceProcessLists.AreHostilActorsNear(Interior)) {
		BSsprintf(msg, 500, sNoFastTravelHostileActorsNear.String());
		Interface::ShowMessage(msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0);
		return 0;
	} else {
		BSSimpleList<TESObjectREFR*>* preflist =
			ReferenceProcessLists.IsActorTargetingREFinPackage(pPlayer, PACKAGE_TYPE_ALARM, 0);
		if (!preflist || preflist->IsEmpty()) {
			BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList();
			if (pCurrentProcess) {
				while (plist && !plist->IsEmpty()) {
					BSSimpleList<ActiveEffect*>* pnext = plist->m_pkNext;
					ActiveEffect* peffect = plist->m_item;
					if (peffect && peffect->IsCausingDamage()) {
						BSsprintf(msg, 500, sFastTravelNoTravelHealthDamage.String());
						Interface::ShowMessage(msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
							0, 2.0f, 0);
						return 0;
					}
					plist = pnext;
				}
			}
			if (static_cast<uint8_t>(pPlayer->GetScriptAllowedFastTravel())) {
				if (pPlayer->IsOverEncumbered() &&
					(fCheckPerk = 0.0f,
						(BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_ALWAYS_FAST_TRAVEL, this, &fCheckPerk),
							fCheckPerk < 1.0))) {
					BSsprintf(msg, 500, sNoFastTravelOverencumbered.String());
					Interface::ShowMessage(msg, 0, 0, 0, 2.0f, 0);
					return 0;
				} else {
					if (pPlayer->GetActorValue(ActorValue::FATIGUE) >= 1 &&
						pPlayer->GetActorValue(ActorValue::HEALTH) >= 1) {
						if (pPlayer->IsJumping()) {
							BSsprintf(msg, 500, sNoFastTravelInAir.String());
							Interface::ShowMessage(msg, 0,
								"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0);
							return 0;
						} else {
							if (GetParentCell()) {
								bretval = !GetParentCell()->CantTravelFrom();
							} else {
								TESWorldSpace* pworld = GetWorldSpace();
								if (pworld) {
									bretval = !pworld->CantTravelFrom();
								}
							}
							if (!bretval) {
								BSsprintf(msg, 500, sNoFastTravelCell.String());
								Interface::ShowMessage(msg, 0,
									"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0);
							}
							return bretval;
						}
					} else {
						BSsprintf(msg, 500, sFastTravelNoTravelHealthDamage.String());
						Interface::ShowMessage(msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
							0, 2.0f, 0);
						return 0;
					}
				}
			} else {
				BSsprintf(msg, 500, sNoFastTravelScriptBlock.String());
				Interface::ShowMessage(msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0,
					2.0f, 0);
				return 0;
			}
		} else {
			BSsprintf(msg, 500, sNoFastTravelAlarm.String());
			Interface::ShowMessage(msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f,
				0);
			preflist->RemoveAllPointerList();
			if (preflist) {
				delete preflist;
			}
			return 0;
		}
	}
}

void PlayerCharacter::Sneak() {
	uint16_t slastMoveMode = GetMoveMode();
	pActorMover->ForceMoveMode(static_cast<uint16_t>(slastMoveMode &
								   (ActorMover::MF_SWIMMING | ActorMover::MF_FALL | ActorMover::MF_SLIDE)) |
		ActorMover::MF_SNEAKING);
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
				MagicHitEffect* peffect;
				MagicShaderHitEffect* apMemory =
					static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
				if (apMemory) {
					peffect = ::new (apMemory) MagicShaderHitEffect(pact->pTarget, pEffectData, -1.0f);
				} else {
					peffect = 0;
				}
				if (peffect->Init()) {
					ReferenceProcessLists.AddTempEffect(peffect);
				} else {
					if (peffect) {
						delete peffect;
					}
				}
			}
			plist = plist->m_pkNext;
		}
	}
	kPerceivedActorsLock.Unlock();
}

void PlayerCharacter::ShaderNearbyItems() {
	FindObjectIterator arIterator(WEAP_ID, this);
	FindObjectIterator apMemory(AMMO_ID, this);
	FindObjectIterator v17(ALCH_ID, this);
	FindObjectIterator apThis(KEYM_ID, this);
	float a3 = fItemDetectionTimer.Float();
	TESObjectREFR* arpRef = 0;
	ItemChange* arpItemChange = 0;
	TESEffectShader* a2 = static_cast<TESEffectShader*>(pDataHandler->GetItemDetectedShader());
	if (a2) {
		while (pLoadedReferenceCollection->GetNextObject(arIterator, arpRef, arpItemChange, 3u)) {
			MagicShaderHitEffect* v4;
			MagicShaderHitEffect* v9 =
				static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
			if (v9) {
				v4 = ::new (v9) MagicShaderHitEffect(arpRef, a2, a3);
			} else {
				v4 = 0;
			}
			if (v4->Init()) {
				ReferenceProcessLists.AddTempEffect(v4);
			} else {
				if (v4) {
					delete v4;
				}
			}
		}
		while (pLoadedReferenceCollection->GetNextObject(apMemory, arpRef, arpItemChange, 3u)) {
			MagicShaderHitEffect* v3;
			MagicShaderHitEffect* v8 =
				static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
			if (v8) {
				v3 = ::new (v8) MagicShaderHitEffect(arpRef, a2, a3);
			} else {
				v3 = 0;
			}
			if (v3->Init()) {
				ReferenceProcessLists.AddTempEffect(v3);
			} else {
				if (v3) {
					delete v3;
				}
			}
		}
		while (pLoadedReferenceCollection->GetNextObject(v17, arpRef, arpItemChange, 3u)) {
			MagicShaderHitEffect* v2;
			MagicShaderHitEffect* v7 =
				static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
			if (v7) {
				v2 = ::new (v7) MagicShaderHitEffect(arpRef, a2, a3);
			} else {
				v2 = 0;
			}
			if (v2->Init()) {
				ReferenceProcessLists.AddTempEffect(v2);
			} else {
				if (v2) {
					delete v2;
				}
			}
		}
		while (pLoadedReferenceCollection->GetNextObject(apThis, arpRef, arpItemChange, 3u)) {
			MagicShaderHitEffect* v1;
			MagicShaderHitEffect* v6 =
				static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
			if (v6) {
				v1 = ::new (v6) MagicShaderHitEffect(arpRef, a2, a3);
			} else {
				v1 = 0;
			}
			if (v1->Init()) {
				ReferenceProcessLists.AddTempEffect(v1);
			} else {
				if (v1) {
					delete v1;
				}
			}
		}
	}
}

void PlayerCharacter::UpdatePlayerCombat(float afDelta) {
	uint32_t apNumLost;
	if (pCombatGroup) {
		if (!pCombatGroup->GetTargetCount()) {
			pCombatGroup->RemoveMember(this);
			pCombatGroup = 0;
		}
	}
	if (pCombatGroup) {
		fCombatTimer = fCombatTimer + afDelta;
	} else {
		fCombatTimer = 0.0;
	}
	apNumLost = 0;
	uint32_t CombatantCount = pCombatManager->GetCombatantCount(pPlayer, &apNumLost);
	if (CombatantCount) {
		bPlayerInCombat = 1;
		bAllCombatTargetsSearching = CombatantCount == apNumLost;
	} else {
		bPlayerInCombat = 0;
		bAllCombatTargetsSearching = 0;
	}
}

void PlayerCharacter::CheckBorderRegion() {
	if (gs_bBorderRegionsEnabled_General.Bool() && !TES::pInstance->CellTestRunning()) {
		bool binBorderRegion = 0;
		if (bInBorderContainedCell) {
			binBorderRegion = 1;
		} else {
			if (pBorderRegions) {
				// The constructor from a world position on the reference's location.
				TESRegionPoint testPoint(*QLocationOnReference());
				uint32_t icount = pBorderRegions->m_usSize;
				for (uint32_t i = 0; i < icount; ++i) {
					TESRegion* pregion = pBorderRegions->operator[](i);
					if (!pregion->GetDelete()) {
						for (BSSimpleList<TESRegionPointList*>* plist = pregion->pPointLists;
							!binBorderRegion && (plist && !plist->IsEmpty()); plist = plist->m_pkNext) {
							if (plist->m_item->IsInsideRegion(&testPoint)) {
								binBorderRegion = 1;
							}
						}
					}
					if (binBorderRegion) {
						break;
					}
				}
			} else {
				TESWorldSpace* pworld = GetWorldSpace();
				if (!pworld || !pworld->GetHasBorderRegion()) {
					binBorderRegion = 1;
				}
			}
		}
		if (binBorderRegion) {
			SetLastKnownGoodPosition();
		} else {
			ReturnToLastKnownGoodPosition(0);
		}
	}
}

void PlayerCharacter::HandleFlycamMovement() {
	static int32_t uiLastFlycamTime;
	NiMatrix3 mat;
	NiMatrix3 ecx0;
	Controls* Controls_ = appMain->GetControls();
	float v27 = 0.0f;
	float v24 = 0.0f;
	int32_t v28 = 0x7FFF;
	int32_t ControllerValue = 0;
	int32_t MouseAxis = 0;
	InterfaceManager::Instance();
	if (InterfaceManager::Has360Controller()) {
		ControllerValue = Controls_->GetControllerValue(0, 9);
		MouseAxis = Controls_->GetControllerValue(0, 10);
		if (NiAbs(static_cast<float>(ControllerValue)) <= 7849.0) {
			ControllerValue = 0;
		}
		if (NiAbs(static_cast<float>(MouseAxis)) <= 7849.0) {
			MouseAxis = 0;
		}
	} else {
		ControllerValue = Controls_->GetMouseAxis(1);
		MouseAxis = Controls_->GetMouseAxis(2);
	}
	if (gs_bInvertYValues_Controls.Bool()) {
		MouseAxis = -MouseAxis;
	}
	InterfaceManager::Instance();
	if (InterfaceManager::Has360Controller()) {
		fUFOCameraHeading =
			fMouseSensitivity.Float() * static_cast<double>(ControllerValue) / 1500.0 + fUFOCameraHeading;
		fUFOCameraPitch = fMouseSensitivity.Float() * static_cast<double>(MouseAxis) / -1500.0 + fUFOCameraPitch;
	} else {
		fUFOCameraHeading = fMouseSensitivity.Float() * static_cast<double>(ControllerValue) + fUFOCameraHeading;
		fUFOCameraPitch = fMouseSensitivity.Float() * static_cast<double>(MouseAxis) + fUFOCameraPitch;
	}
	ecx0.MakeZRotation(fUFOCameraHeading);
	mat.MakeXRotation(fUFOCameraPitch);
	NiMatrix3 result = ecx0 * mat;
	NiPoint3 pt = NiPoint3_ZERO;
	ControllerValue = 0;
	MouseAxis = 0;
	InterfaceManager::Instance();
	if (InterfaceManager::Has360Controller()) {
		ControllerValue = Controls_->GetControllerValue(0, 7);
		MouseAxis = Controls_->GetControllerValue(0, 8);
		if (NiAbs(static_cast<float>(ControllerValue)) <= 7849.0) {
			ControllerValue = 0;
		}
		v27 = static_cast<double>(ControllerValue) / 32767.0;
		if (NiAbs(static_cast<float>(MouseAxis)) <= 7849.0) {
			MouseAxis = 0;
		}
		v24 = static_cast<double>(MouseAxis) / 32767.0;
		pt.x = v27 * 15.0 + pt.x;
		pt.y = v24 * 15.0 + pt.y;
	} else {
		if (Controls_->GetUserAction(Controls::UACTN_FORWARD, 0)) {
			pt.y = pt.y + 10.0;
		}
		if (Controls_->GetUserAction(Controls::UACTN_BACK, 0)) {
			pt.y = pt.y - 10.0;
		}
		if (Controls_->GetUserAction(Controls::UACTN_SLDRIGHT, 0)) {
			pt.x = pt.x + 10.0;
		}
		if (Controls_->GetUserAction(Controls::UACTN_SLDLEFT, 0)) {
			pt.x = pt.x - 10.0;
		}
		if (Controls_->GetUserAction(Controls::UACTN_RUN, 0)) {
			pt = pt * 2.0f;
		}
	}
	uint32_t v30 = BSGetTickCount();
	float afA = 0.0f;
	if (uiLastFlycamTime) {
		afA = static_cast<double>(v30 - uiLastFlycamTime) / 1000.0;
		afA = BSMIN(afA, 1.0f);
	}
	uiLastFlycamTime = v30;
	float X_4 = afA * 10.0;
	pt.operator*=(X_4);
	pt = result * pt;
	pt.operator*=(fUFOCamSpeedMult.Float());
	UFOCameraPos.operator+=(pt);
	BSSceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
	NiNode* CameraRoot = WorldSceneGraph->GetCameraRoot();
	CameraRoot->SetTranslate(UFOCameraPos);
	NiNode* v2 = WorldSceneGraph->GetCameraRoot();
	v2->SetRotate(result);
	NiUpdateData a2(0.0f, 0, 0);
	NiNode* v3 = WorldSceneGraph->GetCameraRoot();
	v3->Update(a2);
	if (GetWorldSpace()) {
		TESWorldSpace* WorldSpace = GetWorldSpace();
		if (WorldSpace->GetTerrainManager()) {
			TESWorldSpace* v5 = GetWorldSpace();
			BGSTerrainManager* TerrainManager = v5->GetTerrainManager();
			TerrainManager->Update(UFOCameraPos, BGSTerrainManager::UPDATE_ALL);
			TESWorldSpace* v7 = GetWorldSpace();
			BGSTerrainManager* v8 = v7->GetTerrainManager();
			v8->UpdateMorphParameters(UFOCameraPos);
		}
	}
}

void PlayerCharacter::ReturnToLastKnownGoodPosition(bool abPositionPlayer) {
	if (pLastKnownGoodLocation) {
		if (abPositionPlayer) {
			const char* v2 = sPlayerLeavingBorderRegion.String();
			Interface::ShowMessage(v2, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0);
			TESObjectCELL* pcell = 0;
			TESWorldSpace* pworld = 0;
			if (pLastKnownGoodLocation) {
				int32_t FormType = pLastKnownGoodLocation->GetFormType();
				if (FormType == 57) {
					pcell = static_cast<TESObjectCELL*>(pLastKnownGoodLocation);
				} else {
					if (FormType == 65) {
						pworld = static_cast<TESWorldSpace*>(pLastKnownGoodLocation);
					}
				}
			}
			if (pworld) {
				NiPoint3 v25 = GetAngleOnReference();
				NiPoint3 v23 = GetLastKnownGoodPosition();
				PositionPlayerExterior(v23, v25, pworld, 0);
			} else {
				if (pcell) {
					NiPoint3 v26 = GetAngleOnReference();
					NiPoint3 v24 = GetLastKnownGoodPosition();
					PositionPlayer(v24, v26, pcell, 0);
				}
			}
		} else {
			Actor* pactor = this;
			const char* v3 = sPlayerLeavingBorderRegion.String();
			Interface::ShowMessage(v3, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0);
			bhkCharacterController* pcon = GetCharController();
			if (pcon) {
				if (!pcon->QSupported()) {
					LastKnownGoodPosition.z = pactor->QLocationOnReference()->z;
				}
			}
			pactor->SetLocationOnReference(LastKnownGoodPosition);
			if (pcon) {
				pcon->SetPosition(LastKnownGoodPosition);
			}
			TESObjectCELL* pcell_2 = 0;
			TESWorldSpace* pworld_2 = 0;
			if (pLastKnownGoodLocation) {
				int32_t v6 = pLastKnownGoodLocation->GetFormType();
				if (v6 == 57) {
					pcell_2 = static_cast<TESObjectCELL*>(pLastKnownGoodLocation);
				} else {
					if (v6 == 65) {
						pworld_2 = static_cast<TESWorldSpace*>(pLastKnownGoodLocation);
					}
				}
			}
			if (pworld_2) {
				int32_t icellx = FloatToInt(LastKnownGoodPosition.x) >> 12;
				int32_t icelly = FloatToInt(LastKnownGoodPosition.y) >> 12;
				pcell_2 = pDataHandler->GetCellFromCellCoord(icellx, icelly, pworld_2, 0);
			}
			if (pcell_2) {
				pactor->SetParentCell(pcell_2);
				SetParentCell(pcell_2);
			}
			NiAVObject* pobj3D = Get3D(0);
			NiAVObject* pobj1st3D = Get3D(1);
			pobj3D->SetTranslate(LastKnownGoodPosition);
			pobj1st3D->SetTranslate(LastKnownGoodPosition);
			bhkNiCollisionObject::ResetSim(pobj3D, 1);
			bhkNiCollisionObject::ResetSim(pobj1st3D, 1);
			NiUpdateData UpdateData1(0.0f, 0, 0);
			pobj3D->Update(UpdateData1);
			NiUpdateData UpdateData2(0.0f, 0, 0);
			pobj1st3D->Update(UpdateData2);
		}
	}
}

void PlayerCharacter::PlayerSleep() {
	BSAudio* v1 = &BSAudio::QInstance();
	v1->EndAllSoundsOfType(4u);
	InterruptCast();
	float ftime = 3600.0 / appCalendar.GetTimeScale();
	float v5 = ReferenceProcessLists.GetSystemTimeClock() + ftime;
	ReferenceProcessLists.SetSystemTimeClock(v5);
	appCalendar.Update(ftime);
	if (PlayerCharacter::bSleepRunUpdates) {
		if (GetInChargen()) {
			UpdateTarget(ftime);
		} else {
			if (ReferenceProcessLists.bRunScheduals) {
				ReferenceProcessLists.kUpdateLock.Lock(0);
				ReferenceProcessLists.UpdateHighListPackages(3600.0f, 1);
				ReferenceProcessLists.UpdateMiddleHighList(3600.0f, 1);
				ReferenceProcessLists.UpdateMiddleLowList(3600.0f, 1);
				ReferenceProcessLists.UpdateLowList(3600.0f, 1);
				ReferenceProcessLists.kUpdateLock.Unlock();
			}
		}
		pPlayer->RestoreHealth(ftime);
		if (IsHardcore() && pPlayer->IsSleeping()) {
			float fSleepRestored_2;
			float fCurrentSLP = GetActorFloatValue(ActorValue::SLEEPDEPREVATION);
			float fSleepRestored = fHCSleepRestorationMod.Float();
			if (!(fCurrentSLP < static_cast<double>(fSleepRestored))) {
				fSleepRestored_2 = fSleepRestored;
			} else {
				fSleepRestored_2 = fCurrentSLP;
			}
			PermanentModActorValue(ActorValue::SLEEPDEPREVATION, static_cast<float>(-fSleepRestored_2),
				static_cast<Actor*>(0));
		}
	}
	float fDelta = appTimer.fDelta;
	TES::pInstance->pSky->Update(fDelta);
	ReferenceProcessLists.UpdateTheCrimeList();
	UpdateMagic(1, 0);
	if (--iSleepTime <= 0) {
		PlayerCharacter::bSleepRunUpdates = 1;
		if (bIsSleeping && !IsHardcore()) {
			RestoreFullHealthAndConditions();
		}
		bIsSleeping = 0;
	}
}

void PlayerCharacter::UpdateTemp1stPerson() {
	if (bTemp1stPerson && (!IsPipboyActive() && pVATS.eMode != VATS::VATS_PLAYBACK)) {
		ItemChange* v1;
		if (bDelayVATSMenu ||
			(!GetIronSights() ||
				(!pCurrentProcess->GetCurrentWeapon() ||
					(v1 = pCurrentProcess->GetCurrentWeapon(),
						!static_cast<TESObjectWEAP*>(v1->pContainerObj)->GetScopeModel()->GetModelLength())))) {
			bTemp1stPerson = 0;
			bSkipAlphaFadeOut = 1;
			if (bTemp1stPersonSwitchBack) {
				SetFirstPerson(0);
			}
			bTemp1stPersonSwitchBack = 0;
		}
	}
}

NiNode* PlayerCharacter::CloneInventory3D(bool abFlag) {
	BSCloningProcess cloneData;
	NiMatrix3 rot;
	if (!abFlag || spInventoryPC.m_pObject) {
		if (!abFlag) {
			if (spInventoryPC.m_pObject) {
				Animation* apThis = this->GetInventoryAnimation();
				if (apThis) {
					delete apThis;
				}
				pInventoryAnimation = 0;
				NiNode* v27 = spInventoryPC.m_pObject;
				NiFixedString* v19 = &FixedStrings::QRaceHeadBiped();
				NiAVObject* pav_3 = v27->GetObjectByName(*v19);
				BSFaceGenNiNode* pfacenode_3 =
					static_cast<BSFaceGenNiNode*>(NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, pav_3));
				if (pfacenode_3) {
					pfacenode_3->UpdateAllChildrenMorphData(1);
				}
				NiNode* v26 = spInventoryPC.m_pObject;
				NiFixedString* v20 = &FixedStrings::QRaceHeadSkinned();
				NiAVObject* pav_4 = v26->GetObjectByName(*v20);
				BSFaceGenNiNode* pfacenode_4 =
					static_cast<BSFaceGenNiNode*>(NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, pav_4));
				if (pfacenode_4) {
					pfacenode_4->UpdateAllChildrenMorphData(1);
				}
				spInventoryPC.operator=(static_cast<NiNode*>(0));
				MagicShaderHitEffect* peffect = GetMagicShaderHitEffect();
				if (peffect) {
					delete peffect;
					SetInventoryWeaponEffect(0);
				}
			}
		}
	} else {
		Animation* v31;
		NiNode* pmagicBone;
		NiNode* parrowBone;
		BSFaceGenAnimationData* v38;
		NiNode* v39;
		NiNode* pclonednode_2;
		char apDest[268];
		char Str1[264];
		bool boldForce = BSShaderPPLightingProperty::GetForceNoProjectedShadowTexture();
		BSShaderPPLightingProperty::SetForceNoProjectedShadowTexture(1);
		ReferenceProcessLists.FinishGeometryDecalEffects();
		ReferenceProcessLists.UpdateTempEffects(0.0f);
		NiNode* v2 = Get3D();
		bhkForceController::RemoveForce(v2);
		NiAVObject* proot = Get3D(0);
		bhkForceController::RemoveForce(proot);
		NiAVObject* proot_2 = Get3D(0);
		NiPointer<NiTimeController> spmanager(proot_2->GetControllers());
		proot_2->RemoveAllControllers();
		cloneData.~BSCloningProcess();
		::new (&cloneData) BSCloningProcess(1.0f);
		NiObject* pobj = proot_2->Clone(cloneData);
		if (pobj) {
			pclonednode_2 = pobj->IsNode();
		} else {
			pclonednode_2 = 0;
		}
		if (pclonednode_2) {
			ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
			ShadowSceneNode_->FindAndCullLight(pclonednode_2, 0);
		}
		NiTimeController* a2 = spmanager.m_pObject;
		proot_2->SetControllers(a2);
		if (pobj) {
			v39 = pobj->IsNode();
		} else {
			v39 = 0;
		}
		spInventoryPC.operator=(v39);
		spInventoryPC.m_pObject->RemoveAllControllers();
		Animation::ClearControllersInterpolators(spInventoryPC.m_pObject);
		spInventoryPC.m_pObject->RemoveProperty(NiAlphaProperty::GetType());
		spInventoryPC.m_pObject->UpdateProperties();
		BSShaderUtil::RecursiveSetPropertyFadeAlpha(spInventoryPC.m_pObject, 1.0f);
		RecursiveReadyInventoryNode(spInventoryPC.m_pObject);
		spInventoryPC.m_pObject->SetTranslate(NiPoint3_ZERO);
		static_cast<TESNPC*>(GetObjectReference())->TESNPC::FixDisplayedHeadParts(this, spInventoryPC.m_pObject);
		rot = NiMatrix3_IDENTITY;
		spInventoryPC.m_pObject->SetRotate(MultipleMatrixByRace(rot));
		BSFaceGenAnimationData* v44 =
			static_cast<BSFaceGenAnimationData*>(NiMemObject::operator new(sizeof(BSFaceGenAnimationData)));
		if (v44) {
			v38 = ::new (v44) BSFaceGenAnimationData;
		} else {
			v38 = 0;
		}
		NiPointer<BSFaceGenAnimationData> spanimdata(v38);
		NiNode* v37 = spInventoryPC.m_pObject;
		NiFixedString* v6 = &FixedStrings::QRaceHeadBiped();
		NiAVObject* pav = v37->GetObjectByName(*v6);
		BSFaceGenNiNode* pfacenode =
			static_cast<BSFaceGenNiNode*>(NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, pav));
		if (pfacenode) {
			BSFaceGenAnimationData* a2a = spanimdata.m_pObject;
			pfacenode->SetAnimationData(a2a);
		}
		NiNode* v36 = spInventoryPC.m_pObject;
		NiFixedString* v7 = &FixedStrings::QRaceHeadSkinned();
		NiAVObject* pav_2 = v36->GetObjectByName(*v7);
		BSFaceGenNiNode* pfacenode_2 =
			static_cast<BSFaceGenNiNode*>(NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, pav_2));
		if (pfacenode_2) {
			BSFaceGenAnimationData* a2b = spanimdata.m_pObject;
			pfacenode_2->SetAnimationData(a2b);
			pfacenode_2->UpdateAllChildrenMorphData(1);
		}
		spanimdata.operator=(static_cast<BSFaceGenAnimationData*>(0));
		NiNode* v35 = spInventoryPC.m_pObject;
		NiFixedString* v8 = &FixedStrings::QArrowBone();
		NiAVObject* parrowObj = v35->GetObjectByName(*v8);
		if (parrowObj) {
			parrowBone = parrowObj->IsNode();
		} else {
			parrowBone = 0;
		}
		if (parrowBone) {
			for (int32_t n = 0;; ++n) {
				uint32_t ArrayCount = parrowBone->GetArrayCount();
				if (n >= ArrayCount) {
					break;
				}
				parrowBone->DetachChildAt(static_cast<uint32_t>(n));
			}
		}
		NiNode* v33 = spInventoryPC.m_pObject;
		NiFixedString* v10 = &FixedStrings::QMagicNode();
		NiAVObject* pmagicObj = v33->GetObjectByName(*v10);
		if (pmagicObj) {
			pmagicBone = pmagicObj->IsNode();
		} else {
			pmagicBone = 0;
		}
		if (pmagicBone) {
			for (int32_t n_2 = 0;; ++n_2) {
				uint32_t v11 = pmagicBone->GetArrayCount();
				if (n_2 >= v11) {
					break;
				}
				pmagicBone->DetachChildAt(static_cast<uint32_t>(n_2));
			}
		}
		BSShaderPPLightingProperty::SetRefractionRecurse(spInventoryPC.m_pObject, 0, 0.0f, 0, 0.0f, 0);
		Animation* v43 = static_cast<Animation*>(ourMalloc(sizeof(Animation)));
		if (v43) {
			v31 = ::new (v43) Animation;
		} else {
			v31 = 0;
		}
		pInventoryAnimation = v31;
		TESBoundObject* pref_2 = GetObjectReference();
		BSsprintf(apDest, 260, "%s", GetModel());
		char* pslash = std::strrchr(apDest, 92);
		if (!pslash) {
			return 0;
		}
		BSstrcpy(pslash, 260 - (pslash - apDest), "\\MTIdle.KF");
		BSsprintf(Str1, 260, "Data\\%s\\%s", "Meshes", apDest);
		const char* Model_ = GetModel();
		BSSimpleList<const char*>* pkfFiles = pModelLoader->BuildFileList(Str1, Model_, 0);
		*pslash = 0;
		// Each weapon kind's idle and torch idle file, when it exists, goes into the list as a 260-byte path the list
		// owns.
		for (int32_t n_3 = 1; n_3 < 12; ++n_3) {
			char* pIdleFile;
			char* pTorchIdleFile;
			BSsprintf(Str1, 260, "Data\\%s\\%s\\%sIdle.KF", "Meshes", apDest, animWeaponInfo[n_3]);
			FileFinder* Singleton = FileFinder::GetSingleton();
			if (Singleton->Exist(Str1, 0, 0, FileFinder::ARCHIVE_TYPE_ANY)) {
				pIdleFile = static_cast<char*>(ourMalloc(260u));
				const char* v15 = GetModel();
				BSstrcpy(pIdleFile, 260, v15);
				pslash = std::strrchr(pIdleFile, 92);
				if (pslash) {
					++pslash;
					BSsprintf(pslash, 260 - (pslash - pIdleFile + 1), "%sIdle.KF", animWeaponInfo[n_3]);
					pkfFiles->AddTail(pIdleFile);
				}
			}
			BSsprintf(Str1, 260, "Data\\%s\\%s\\%sTorchIdle.KF", "Meshes", apDest, animWeaponInfo[n_3]);
			FileFinder* v16 = FileFinder::GetSingleton();
			if (v16->Exist(Str1, 0, 0, FileFinder::ARCHIVE_TYPE_ANY)) {
				pTorchIdleFile = static_cast<char*>(ourMalloc(260u));
				const char* v17 = GetModel();
				BSstrcpy(pTorchIdleFile, 260, v17);
				pslash = std::strrchr(pTorchIdleFile, 92);
				if (pslash) {
					++pslash;
					BSsprintf(pslash, 260 - (pslash - pTorchIdleFile + 1), "%sTorchIdle.KF", animWeaponInfo[n_3]);
					pkfFiles->AddTail(pTorchIdleFile);
				}
			}
		}
		NiNode* v21 = spInventoryPC.m_pObject;
		this->GetInventoryAnimation()->Init(pkfFiles, v21, this, 1);
		LightRefData* lightPtr = (&m_Extra)->GetLight();
		NiNode* v30 = spInventoryPC.m_pObject;
		v30->UpdateWorldBound();
		if (pCurrentProcess) {
			BaseProcess* CurrentAIProcess = pCurrentProcess;
			CurrentAIProcess->SetShouldRefreshMagicShadersEffects(1, 0, 0);
		}
		BSShaderPPLightingProperty::SetForceNoProjectedShadowTexture(boldForce);
		NiNode* v28 = spInventoryPC.m_pObject;
		NiFixedString* v18 = &FixedStrings::QHeadMeshExp();
		NiAVObject* pspecialFace = v28->GetObjectByName(*v18);
		if (pspecialFace) {
			pspecialFace->SetAppCulled(1);
		}
	}
	return spInventoryPC.m_pObject;
}

bool PlayerCharacter::AddQuestStageItem(TESQuestStageItem* apNewItem) {
	if (!apNewItem) {
		return 0;
	}
	BSSimpleList<TESQuestStageItem*>* ecx0a = GetQuestLog();
	while (ecx0a && ecx0a->m_item) {
		TESQuestStageItem* v7 = ecx0a->m_item;
		ecx0a = ecx0a->m_pkNext;
		if (v7 == apNewItem) {
			TESQuest* TalkingActivator = v7->GetOwner();
			if (!TalkingActivator->GetAllowRepeatStages()) {
				return 0;
			}
		}
	}
	listQuestLog.AddHead(apNewItem);
	if (!Script::GetProcessScripts()) {
		return 1;
	}
	if (apNewItem->GetEndQuest()) {
		Interface::PlayMenuSound(9);
		TESQuest* v4 = apNewItem->GetOwner();
		if (v4 == pPlayer->GetActiveQuest()) {
			if (apNewItem->m_pNextQuest) {
				TESQuest* v5 = apNewItem->m_pNextQuest;
				v5->SetEnabled(1u);
			}
		}
	}
	return 1;
}

void PlayerCharacter::RemovePlayerMapMarker() {
	if (pPlayerMapMarker) {
		delete pPlayerMapMarker;
	}
	pPlayerMapMarker = 0;
	PlayerMarkerPath.Clear();
}

// The "Bip01 Speaker" string allocated here is never freed, as the game does.
void PlayerCharacter::FocusOnActor(MobileObject* apActor, float afZoomPercent, bool abOnlyZoom) {
	if (apActor) {
		float fclampedLow;
		float fzoomPercent;
		if (!(afZoomPercent < 0.0)) {
			fclampedLow = afZoomPercent;
		} else {
			fclampedLow = 0.0f;
		}
		if (!(fclampedLow > 1.0)) {
			fzoomPercent = fclampedLow;
		} else {
			fzoomPercent = 1.0f;
		}
		if (PlayerCharacter::bVanityMode) {
			StopVanityMode();
			Show1stPerson(!b3rdPerson);
		}
		if (GetParentCell() && GetParentCell()->GetInterior()) {
			TESObjectCELL* ParentCell = apActor->GetParentCell();
			if (ParentCell != GetParentCell()) {
				return;
			}
		} else {
			TESWorldSpace* WorldSpace = apActor->GetWorldSpace();
			if (WorldSpace != GetWorldSpace()) {
				return;
			}
		}
		NiPoint3 src = p1stPersonCamera->GetWorldTranslate();
		NiPoint3 target;
		bool bcalculated = 0;
		NiAVObject* pav = 0;
		if (apActor->IsTalkingActivator()) {
			TESObjectREFR* pref = apActor->GetTalkingActivator();
			pav = pref->Get3D();
		} else {
			pav = apActor->Get3D();
		}
		bool bvalidTarget = 1;
		if (pav) {
			BSFaceGenNiNode* pfaceGenHead = apActor->GetFaceNodeSkinned(0);
			NiNode* phead = 0;
			if (!pfaceGenHead) {
				NiAVObject* pheadObject = BSUtilities::GetObjectByName(pav, *FixedStrings::GetBip01_Head());
				phead = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pheadObject));
			}
			NiFixedString* pSpeaker;
			NiFixedString* pSpeakerMemory = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
			if (pSpeakerMemory) {
				pSpeaker = ::new (static_cast<void*>(pSpeakerMemory)) NiFixedString("Bip01 Speaker");
			} else {
				pSpeaker = 0;
			}
			if (!phead) {
				NiAVObject* pspeakerObject = BSUtilities::GetObjectByName(pav, *pSpeaker);
				phead = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pspeakerObject));
			}
			if (pfaceGenHead || phead) {
				if (pfaceGenHead) {
					if (pfaceGenHead->GetAppCulled()) {
						pfaceGenHead->SetAppCulled(0);
						NiUpdateData UpdateData(0.0f, 0, 0);
						pfaceGenHead->Update(UpdateData);
						pfaceGenHead->UpdateWorldBound();
						pfaceGenHead->SetAppCulled(1);
					}
				}
				NiBound headBound;
				if (pfaceGenHead) {
					headBound.Copy(&pfaceGenHead->GetWorldBound());
				} else {
					headBound.SetCenter(phead->GetWorldTranslate());
					headBound.SetRadius(32.0f);
				}
				target = headBound.m_kCenter;
				target.z = target.z + fDlgLookAdj.Float();
				src -= target;
				NiCamera* pcam = Main::GetWorldSceneGraph()->GetCamera();
				if (pcam) {
					bcalculated = 1;
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
					float fstep1st =
						fdelta1st / ((1.0 - PlayerCharacter::fDialogZoomPercent) * fDialogZoomInSeconds.Float());
					f1stPersonFOV = BSMIN(fstep1st, 1.0f) * f1stPersonFOVDelta + f1stPersonFOV;
				}
			} else {
				if (plastWarningActor != apActor) {
					Warning("DIALOGUE: PlayerCharacter::FocusOnActor( ): Attempting to talk to an object '%s' without "
							"a 'Bip01 Head'.",
						apActor->GetModel());
				}
				plastWarningActor = apActor;
				bvalidTarget = 0;
			}
			if (!abOnlyZoom && bvalidTarget) {
				float fangleDelta;
				float ffovMult = f1stPersonFOV / BSSceneGraph::fDefaultFOV.Float();
				double flength = src.Length();
				float fsinPitch = src.z / flength;
				float ftargetAngle = std::asin(fsinPitch);
				fangleDelta = ftargetAngle - GetLooking();
				double fabsLook = std::fabs(fangleDelta);
				if (fDlgLookDegStart.Float() * 0.01745329238474369 * ffovMult < fabsLook) {
					badjustLooking = 1;
				}
				if (PlayerCharacter::fDialogZoomPercent < 1.0) {
					badjustLooking = 1;
				}
				if (badjustLooking) {
					if (!(PlayerCharacter::fDialogZoomPercent < 1.0)) {
						double fscaled = fangleDelta * fDlgLookMult.Float();
						float fmod = appTimer.fDelta * fscaled;
						ModLooking(fmod);
					} else {
						double fdelta = appTimer.fDelta;
						float fstep =
							fdelta / ((1.0 - PlayerCharacter::fDialogZoomPercent) * fDialogZoomInSeconds.Float());
						float fmod = BSMIN(fstep, 1.0f) * fangleDelta;
						ModLooking(fmod);
					}
					double fabsLookAfter = std::fabs(fangleDelta);
					if (fDlgLookDegStop.Float() * 0.01745329238474369 * ffovMult > fabsLookAfter) {
						badjustLooking = 0;
					}
				}
				double fzAngle = GetZAngleFromVector(-src);
				for (fangleDelta = fzAngle - GetHeading(0); fangleDelta > 3.1415927410125732;
					fangleDelta = fangleDelta - 6.2831854820251465) {
					;
				}
				while (fangleDelta < -3.1415927410125732) {
					fangleDelta = fangleDelta + 6.2831854820251465;
				}
				double fabsHeading = std::fabs(fangleDelta);
				if (fDlgHeadingDegStart.Float() * 0.01745329238474369 * ffovMult < fabsHeading) {
					badjustHeading = 1;
				}
				if (PlayerCharacter::fDialogZoomPercent < 1.0) {
					badjustHeading = 1;
				}
				if (badjustHeading) {
					if (GetSitSleepState()) {
						double fsitAngle = GetZAngleFromVector(-src);
						for (fangleDelta = fsitAngle - Actor::GetHeading(0) - fSitHeadingDelta;
							fangleDelta > 3.1415927410125732; fangleDelta = fangleDelta - 6.2831854820251465) {
							;
						}
						while (fangleDelta < -3.1415927410125732) {
							fangleDelta = fangleDelta + 6.2831854820251465;
						}
						if (!(PlayerCharacter::fDialogZoomPercent < 1.0)) {
							double fscaled = fangleDelta * fDlgLookMult.Float();
							fSitHeadingDelta = appTimer.fDelta * fscaled + fSitHeadingDelta;
						} else {
							double fdelta = appTimer.fDelta;
							float fstep =
								fdelta / ((1.0 - PlayerCharacter::fDialogZoomPercent) * fDialogZoomInSeconds.Float());
							double fstepMin = BSMIN(fstep, 1.0f);
							fSitHeadingDelta = fstepMin * fangleDelta + fSitHeadingDelta;
						}
					} else {
						if (!(PlayerCharacter::fDialogZoomPercent < 1.0)) {
							double fscaled = fangleDelta * fDlgLookMult.Float();
							float fmod = appTimer.fDelta * fscaled;
							ModHeading(fmod);
						} else {
							double fdelta = appTimer.fDelta;
							float fstep =
								fdelta / ((1.0 - PlayerCharacter::fDialogZoomPercent) * fDialogZoomInSeconds.Float());
							double fstepMin = BSMIN(fstep, 1.0f);
							float fmod = fstepMin * fangleDelta;
							ModHeading(fmod);
						}
					}
					double fabsHeadingAfter = std::fabs(fangleDelta);
					if (fDlgHeadingDegStop.Float() * 0.01745329238474369 * ffovMult > fabsHeadingAfter) {
						badjustHeading = 0;
					}
				}
				PlayerCharacter::fCameraHeading = GetHeading(0);
				PlayerCharacter::fCameraLooking = GetLooking();
				SetFirstPerson(1);
				UpdateFirstPersonZoom();
				UpdateTemp3rdPerson();
				static_cast<PlayerMover*>(pActorMover)->PlayerMover::ClearMoveMode(63u);
				bhkWorld::ClearPickData();
				b3rdPerson = 1;
				UpdateAnimation();
				PickAnimations(1.0f, 1.0f);
				Animation* pAnim3rd = GetAnimation(0);
				UpdateAnimationMovement(pAnim3rd, 0.0f);
				b3rdPerson = 0;
				float fupdateDelta = appTimer.fDelta;
				Character::Update(fupdateDelta);
				float fmoveDelta = appTimer.fDelta;
				Animation* pAnim = GetAnimation(!b3rdPerson);
				UpdateAnimationMovement(pAnim, fmoveDelta);
			}
			if (!bcalculated) {
				if (pav) {
					int32_t fov = static_cast<int32_t>((src.Length() - 45.0) / 100.0 * 25.0) + 15;
					SetFOV(static_cast<float>(fov));
				}
			}
		}
		PlayerCharacter::fDialogZoomPercent = fzoomPercent;
	}
}

void PlayerCharacter::InitiateSitSleepPackage(TESObjectREFR* apCurrentFurniture) {
	if (!pPlayer->IsPathingComplete()) {
		pPlayer->pActorMover->ClearCurrentData();
	}
	SetAIControlledToPosition(1);
	if (pCurrentProcess->GetAnimAction() == ANIM_ACTION_BLOCK) {
		SetBlock(0);
	}
	TESPackage* Package = TESPackage::CreatePackage(PACKAGE_TYPE_TRAVEL);
	Package->SetPackType(PACKAGE_TYPE_TRAVEL);
	Package->SetMustReachLocation(0);
	Package->SetMustComplete(1);
	PackageLocation* v2 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
	if (v2) {
		v2 = ::new (v2) PackageLocation;
	} else {
		v2 = 0;
	}
	v2->SetLocReference(apCurrentFurniture);
	Package->SetPackageLocation(v2);
	if (v2) {
		delete v2;
	}
	Package->SetProcedureType(PACKAGE_PROCEDURE_EXPLORE_TRAVEL);
	PutCreatedPackage(Package, 0, 1);
}

bool PlayerCharacter::IsActorFollowingEscortingPlayer(Actor* apActor, bool bAll) {
	if (apActor->IsDead(0) || (apActor->GetDisabled() || (apActor->IsParalyzed() || apActor->IsRestrained()))) {
		return 0;
	}
	BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
	CurrentAIProcess->CheckforNewPackage(apActor, 1);
	TESPackage* pPack = apActor->GetPackageSetAsPcurrent();
	ActorPackage* pCurrentPackage = &apActor->pCurrentProcess->CurrentPackage;
	if (!bAll) {
		if (pPack) {
			if (pPack->GetPackType() == PACKAGE_TYPE_FOLLOW || pPack->GetPackType() == PACKAGE_TYPE_ACCOMPANY) {
				if (pPack->GetPackageTarget()) {
					TESObjectREFR* pref_2 = apActor->pCurrentProcess->CurrentPackage.pTarget;
					if (!pref_2) {
						TESBoundObject* pobject_2 =
							static_cast<TESBoundObject*>(pPack->GetPackageTarget()->GetTargObject());
						if (pobject_2) {
							if (pobject_2 == pPlayer->GetObjectReference()) {
								pref_2 = pPlayer;
							}
						}
					}
					if (pref_2 == pPlayer) {
						return !pPack->GetWaiting();
					}
				}
			}
		}
		return 0;
	}
	TESObjectREFR* pref = 0;
	if (pCurrentPackage) {
		if (pCurrentPackage->pTarget) {
			pref = pCurrentPackage->pTarget;
			if (!pref) {
				TESBoundObject* pobject = static_cast<TESBoundObject*>(pPack->GetPackageTarget()->GetTargObject());
				if (pobject) {
					if (pobject == pPlayer->GetObjectReference()) {
						pref = pPlayer;
					}
				}
			}
		}
	}
	if (!pPack || pPack->GetLocationReference(apActor) != pPlayer) {
		if (pref != pPlayer) {
			BaseProcess* v4 = apActor->pCurrentProcess;
			if (v4->GetTarget() != pPlayer) {
				return 0;
			}
		}
		if (pPack->GetPackType() == PACKAGE_TYPE_AMBUSH) {
			return 0;
		}
	}
	return pref != pPlayer || !apActor->IsWaiting();
}

bool PlayerCharacter::ReloadLoop(TESObjectWEAP* apWeapon, int32_t aiReloadAnim, bool bUseModValue,
	bool bFromInventory) {
	uint32_t FormHealth_0;
	bool HasModEffectActive_0;
	TESBoundObject* pammo = (TESBoundObject*)apWeapon->GetCurrentAmmo(pPlayer);
	InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(this);
	pinventory->GetObjectCount(pammo);
	ItemChange* pweaponItem = pCurrentProcess->GetCurrentWeapon();
	ItemChange* pAmmoItem = pCurrentProcess->GetCurrentAmmo();
	GetAnimation(0);
	if (apWeapon) {
		if (pammo) {
			if (apWeapon != pammo) {
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
		}
	}
	int32_t icount = pinventory->GetObjectCount(pammo);
	if (!pAmmoItem || pAmmoItem->pContainerObj != pammo) {
		ItemChange* parrows;
		ItemChange* apMemory = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
		if (apMemory) {
			parrows = ::new (apMemory) ItemChange(pammo, icount);
		} else {
			parrows = 0;
		}
		pCurrentProcess->SetCurrentAmmo(parrows);
		pCurrentProcess->GetCurrentAmmo()->iNumber = 0;
		pAmmoItem = parrows;
	}
	if (!pweaponItem || !pAmmoItem) {
		goto LABEL_21;
	}
	FormHealth_0 = pAmmoItem->iNumber;
	HasModEffectActive_0 = pweaponItem->HasModEffectActive(2);
	if (FormHealth_0 == apWeapon->GetFormClipRounds(HasModEffectActive_0)) {
		return 0;
	}
	if (icount == pAmmoItem->iNumber) {
		return 0;
	}
LABEL_21:;
	if (apWeapon) {
		if (pammo != apWeapon) {
			int32_t FormClipRounds = apWeapon->GetFormClipRounds(bUseModValue);
			icount = BSMIN(icount, FormClipRounds);
		}
	}
	if (aiReloadAnim) {
		if (aiReloadAnim == 1) {
			Animation* panim = GetAnimation(0);
			ANIM_GROUP_ENUM eanimGroup = static_cast<ANIM_GROUP_ENUM>(apWeapon->GetReloadAnim(bUseModValue) - 23);
			uint16_t sanimGroup = GetAnimGroup(eanimGroup, 0, 0, panim);
			if (TESAnimGroup::GetType(sanimGroup) == eanimGroup) {
				panim->QueueReloadAnim(sanimGroup);
			}
			return 1;
		} else {
			if (aiReloadAnim == 2) {
				ANIM_GROUP_ENUM eanimGroup_2 = static_cast<ANIM_GROUP_ENUM>(apWeapon->GetReloadAnim(bUseModValue) - 23);
				uint16_t sanimGroup_2 = GetAnimGroup(eanimGroup_2, 0, 0, 0);
				if (TESAnimGroup::GetType(sanimGroup_2) == eanimGroup_2) {
					Animation* panim_2 = GetAnimation(0);
					panim_2->PlayGroup(sanimGroup_2, ACTION_START, -1, AGS_NONE);
					StartAnimOn1stPerson(sanimGroup_2, ACTION_START);
					BSAnimGroupSequence* CurrentSequence = panim_2->GetCurrentSequence(AGS_WEAPON);
					SetAnimAction(ANIM_ACTION_RELOAD_LOOP, CurrentSequence);
				}
				return 1;
			} else {
				return 0;
			}
		}
	} else {
		int32_t iCurrentCount = 1;
		if (pCurrentProcess->GetCurrentAmmo()) {
			ItemChange* v10 = pCurrentProcess->GetCurrentAmmo();
			iCurrentCount = v10->iNumber + 1;
		}
		if (bFromInventory) {
			iCurrentCount = icount;
		}
		QueueEquipObject(pammo, iCurrentCount, 0, 1, 0, 0);
		return 1;
	}
}

void PlayerCharacter::CheckPoisonWeapon(AlchemyItem* apPoison) {
	if (apPoison) {
		TESObjectWEAP* pweapon;
		BaseProcess* CurrentAIProcess = pCurrentProcess;
		ItemChange* pweaponItem = CurrentAIProcess->GetCurrentWeapon();
		if (pweaponItem) {
			pweapon = dynamic_cast<TESObjectWEAP*>(pweaponItem->pContainerObj);
		} else {
			pweapon = 0;
		}
		if (pweapon &&
			(pweapon->GetWeaponSkill() == ActorValue::UNARMED ||
				pweapon->GetWeaponSkill() == ActorValue::MELEE_WEAPONS)) {
			if (pweaponItem->GetPoison()) {
				const char* v3 = sOk.String();
				Interface::CreateMessageMenu(sPoisonAlreadyPoisonedMessage.String(), static_cast<const char*>(0),
					static_cast<const char*>(0), &PlayerCharacter::PoisonWeapon, 1, 23, 0.0f, 0.0f, v3, nullptr);
			} else {
				char apText[256];
				pPendingPoison = apPoison;
				std::sprintf(apText, "%s%s?", sPoisonConfirmMessage.String(), pweapon->GetFullName());
				const char* v4 = sNo.String();
				Interface::CreateMessageMenu(apText, static_cast<const char*>(0), static_cast<const char*>(0),
					&PlayerCharacter::PoisonWeapon, 2, 23, 0.0f, 0.0f, sYes.String(), v4, nullptr);
			}
		} else {
			const char* v2 = sOk.String();
			Interface::CreateMessageMenu(sPoisonNoWeaponMessage.String(), static_cast<const char*>(0),
				static_cast<const char*>(0), &PlayerCharacter::PoisonWeapon, 1, 23, 0.0f, 0.0f, v2, nullptr);
		}
	}
}

void PlayerCharacter::AddQueuedEnchantment(TESBoundObject* apObject, ExtraDataList* apExtra) {
	if (apObject->GetFormType() == ARMO_ID || apObject->GetFormType() == CLOT_ID) {
		EnchantmentItem* FormEnchanting;
		TESEnchantableForm* penchantable = dynamic_cast<TESEnchantableForm*>(apObject);
		if (penchantable) {
			FormEnchanting = penchantable->pFormEnchanting;
		} else {
			FormEnchanting = 0;
		}
		if (FormEnchanting) {
			CastWornEnchantment(FormEnchanting, apObject, 0);
			ProcessEffectsFromItem(FormEnchanting);
		}
	}
}

void PlayerCharacter::RemoveQueuedEnchantment(TESBoundObject* apObject, ExtraDataList* apExtra) {
	if (apObject->GetFormType() == ARMO_ID || apObject->GetFormType() == CLOT_ID) {
		if (QueuedWornEnchantments) {
			EnchantmentItem* FormEnchanting;
			TESEnchantableForm* penchantable = dynamic_cast<TESEnchantableForm*>(apObject);
			if (penchantable) {
				FormEnchanting = penchantable->pFormEnchanting;
			} else {
				FormEnchanting = 0;
			}
			if (FormEnchanting) {
				if (IsSpellTarget(FormEnchanting, 1)) {
					Dispel(apObject, 0);
				}
				QueuedWornEnchantments->Remove(apObject);
				if (QueuedWornEnchantments->IsEmpty()) {
					if (QueuedWornEnchantments) {
						delete QueuedWornEnchantments;
					}
					QueuedWornEnchantments = 0;
				}
			}
		}
	}
}

void PlayerCharacter::HandlePhysicsGrab(float afTimeDelta) {
	Controls* pcontrols = appMain->GetControls();
	int32_t UserAction = pcontrols->GetUserAction(Controls::UACTN_GRAB, 1);
	bool bbuttonPressed = UserAction != 0;
	if (UserAction) {
		Interface::SetDragDroppingState(0);
	}
	int32_t istate = Interface::GetDragDroppingState();
	if (istate == 1) {
		Interface::SetDragDroppingState(2);
	}
	if (bbuttonPressed || (bIsGrabbing || (spGrabSpring.m_pObject || (istate == 1 || istate == 2)))) {
		if (spGrabSpring.m_pObject) {
			if (eGrabType == PlayerCharacter::GRAB_NORMAL) {
				if ((bIsGrabbing && !bbuttonPressed) || istate == 2) {
					UpdateMouseSpring(afTimeDelta);
				} else {
					DestroyMouseSpring();
					bIsGrabbing = 0;
				}
			}
		} else {
			if ((bbuttonPressed || istate == 1) && eGrabType == PlayerCharacter::GRAB_NONE) {
				TESObjectREFR* pObject = Interface::GetActivateREFR();
				if (pObject) {
					TESObjectACTI* ObjectReference = static_cast<TESObjectACTI*>(pObject->GetObjectReference());
					if (ObjectReference != pAshPile1) {
						TESObjectACTI* v4 = static_cast<TESObjectACTI*>(pObject->GetObjectReference());
						if (v4 != pAshPile2) {
							bhkRigidBody* pbody;
							bhkCollisionObject* pcol;
							NiNode* pav = pObject->Get3D();
							if (pav) {
								pcol = FindFirstCollisionObject(pav);
							} else {
								pcol = 0;
							}
							if (pcol) {
								pbody = pcol->GetRigidBody();
							} else {
								pbody = 0;
							}
							if (pbody) {
								if (pbody->GethkWorld()) {
									float fobjectMass = pbody->GetMass();
									if (fobjectMass <= static_cast<double>(fGrabMaxWeightWalking.Float())) {
										CreateMouseSpring(pObject, PlayerCharacter::GRAB_NORMAL, 1000.0f);
										if (spGrabSpring.m_pObject) {
											pObject->AddChange(4);
										}
										bIsGrabbing = 1;
										(&pObject->m_Extra)->NotifyGuardsAboutActivation(pObject, this);
										(&pObject->m_Extra)->RemoveExtra(124);
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

bool PlayerCharacter::UpdateDropSpring(float afTimeDelta) {
	if (eGrabType == PlayerCharacter::GRAB_INVENTORY_DROP) {
		Controls* Controls_ = appMain->GetControls();
		if (!spGrabSpring.m_pObject) {
			double Scale = GetScale();
			float v3 = fPlayerDropDistance.Float() * Scale;
			CreateMouseSpring(pGrabbedObject, PlayerCharacter::GRAB_INVENTORY_DROP, v3);
		}
		if (spGrabSpring.m_pObject) {
			if (!Controls_->GetUserAction(Controls::UACTN_USE, 2) &&
				(Controls_->GetUserAction(Controls::UACTN_USE, 1) ||
					Controls_->GetUserAction(Controls::UACTN_USE, 0))) {
				UpdateMouseSpring(afTimeDelta);
			} else {
				DestroyMouseSpring();
			}
		}
	}
	return eGrabType == PlayerCharacter::GRAB_INVENTORY_DROP;
}

// Sets bBeingChased when a guard is among the actors with a combat package on the player, then frees the list
// IsActorTargetingREFinPackage made.
void PlayerCharacter::CheckGetAwayFromGuards() {
	BSSimpleList<TESObjectREFR*>* preflist =
		ReferenceProcessLists.IsActorTargetingREFinPackage(pPlayer, PACKAGE_TYPE_COMBAT, 0);
	BSSimpleList<TESObjectREFR*>* plist = preflist;
	pPlayer->bBeingChased = 0;
	while (plist) {
		Actor* pactor = 0;
		if (plist->m_item) {
			TESObjectREFR* m_item = plist->m_item;
			if (m_item->IsActor()) {
				pactor = static_cast<Actor*>(plist->m_item);
			}
		}
		if (pactor && pactor->IsActoraGuard()) {
			pPlayer->bBeingChased = 1;
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
	if (apRef) {
		if (apRef->Get3D()) {
			NiPoint3 result;
			const NiPoint3* afDistance = &GetLocationOnReference();
			const NiPoint3* v2 = &apRef->GetLocationOnReference();
			result = *v2 - *afDistance;
			float afDistancea = result.Length();
			float v10 = GamePlayFormulas::CalcImpactShaderAmount(afDistancea);
			if (v10 > 0.0 && !pVATS.GetMode()) {
				NiNode* afDistanceb = apRef->Get3D();
				TESImageSpaceModifier* Hit = TESImageSpaceModifier::GetGetHit();
				ImageSpaceModifierInstanceForm::Trigger(Hit, v10, afDistanceb);
			}
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
				NiNode* v1 = pref->Get3D();
				bhkWorld::SetMotion(v1, hkpMotion::MOTION_DYNAMIC, 1, 1, 1);
				NiNode* v2 = pref->Get3D();
				bhkWorld::Activate(v2, 1, 1, 0);
				DroppedRefList.Remove(pref);
				pref->SetRecentPCDrop(0);
				pcurrent = plast->m_pkNext;
			} else {
				plast = pcurrent;
				pcurrent = pcurrent->m_pkNext;
			}
		}
	}
}

bool PlayerCharacter::IsConflictingObject(NiAVObject* apObj) {
	NiPoint3 v9;
	NiPoint3 v11;
	if (!apObj) {
		return 0;
	}
	if (!pConflictingPick) {
		pConflictingPick = &kConflictingPick;
		kConflictingPick.SetFrontOnly(0);
		pConflictingPick->SetReturnNormal(1);
		pConflictingPick->SetObserveAppCullFlag(1);
	}
	NiPoint3 pos = GetLocationOnReference();
	NiPoint3 dir(0.0f, 24.0f, 0.0f);
	bool bresult = 0;
	pos.z = pPlayer->GetHeight() * 0.25 + pos.z;
	pConflictingPick->SetTarget(apObj);
	if (pConflictingPick->PickObjects(pos + dir, dir, 0)) {
		NiPick::Record* ppickRecord = (&pConflictingPick->m_pickResults)->GetAt(0);
		NiPoint3 normal = *&ppickRecord->m_normal;
		if (dir.Dot(normal) > 0.0 &&
			!NiObject::IsExactKindOf(&NiBillboardNode::ms_RTTI, ppickRecord->GetAVObject()->m_pkParent)) {
			bresult = 1;
		}
	}
	if (!bresult) {
		dir = *::new (&v11) NiPoint3(24.0f, -24.0f, 0.0f);
		if (pConflictingPick->PickObjects(pos + dir, dir, 0)) {
			NiPick::Record* ppickRecord_2 = (&pConflictingPick->m_pickResults)->GetAt(0);
			NiPoint3 normal_2 = *&ppickRecord_2->m_normal;
			if (dir.Dot(normal_2) > 0.0 &&
				!NiObject::IsExactKindOf(&NiBillboardNode::ms_RTTI, ppickRecord_2->GetAVObject()->m_pkParent)) {
				bresult = 1;
			}
		}
		if (!bresult) {
			dir = *::new (&v9) NiPoint3(-24.0f, -24.0f, 0.0f);
			if (pConflictingPick->PickObjects(pos + dir, dir, 0)) {
				NiPick::Record* ppickRecord_3 = (&pConflictingPick->m_pickResults)->GetAt(0);
				NiPoint3 normal_3 = *&ppickRecord_3->m_normal;
				if (dir.Dot(normal_3) > 0.0 &&
					!NiObject::IsExactKindOf(&NiBillboardNode::ms_RTTI, ppickRecord_3->GetAVObject()->m_pkParent)) {
					bresult = 1;
				}
			}
		}
	}
	pConflictingPick->SetTarget(0);
	pConflictingPick->ClearResultsArray(0);
	if (bresult) {
		return 1;
	}
	bhkCharacterController* CharController = GetCharController();
	return CharController->CheckInsideOfObject();
}

void PlayerCharacter::UpdateConflictingObjects() {
	if (!ConflictingObjects.IsEmpty()) {
		BSSimpleList<TESObjectREFR*>* plist = &ConflictingObjects;
		while (plist) {
			bool bremoveFromList = 0;
			TESObjectREFR* pref = plist->m_item;
			NiAVObject* pobj = pref->Get3D();
			if (pobj) {
				if (!IsConflictingObject(pobj)) {
					bremoveFromList = 1;
					pobj->SetAppCulled(0);
					bhkWorld::EnableCollision(pobj, 1, 1, 0);
				}
			} else {
				bremoveFromList = 1;
			}
			if (bremoveFromList) {
				ConflictingObjects.Remove(pref);
				if (ConflictingObjects.IsEmpty()) {
					return;
				}
				plist = &ConflictingObjects;
			} else {
				plist = plist->m_pkNext;
			}
		}
	}
}

void PlayerCharacter::UpdatePlayerActions() {
	PlayerActionObject* pAction;
	BSSimpleList<PlayerActionObject*>* plist = pListofActions;
	ListBoxBase* item = 0;
	while (plist && plist->m_item) {
		pAction = plist->m_item;
		if (!(pAction->ftimer <= 0.0) ||
			(pAction->eAction == PLAYER_ACTION_Z_KEY_OBJECT ||
				pAction->eAction == PLAYER_ACTION_STAND_ON_TABLE_CHAIR)) {
			plist = plist->m_pkNext;
		} else {
			plist->Remove(pAction);
			ourFree(pAction);
			plist = pListofActions;
		}
	}
	for (BSSimpleList<PlayerActionObject*>* plist_2 = pListofActions; plist_2 && plist_2->m_item;
		plist_2 = plist_2->m_pkNext) {
		pAction = plist_2->m_item;
		double Seconds = appTimer.fDelta;
		pAction->ftimer = pAction->ftimer - Seconds;
	}
}

void PlayerCharacter::RemoveAllActionsForObject(TESObjectREFR* apActObject) {
	if (pListofActions) {
		if (apActObject) {
			BSSimpleList<PlayerActionObject*>* plist = pListofActions;
			ListBoxBase* item = 0;
			while (plist && plist->m_item) {
				PlayerActionObject* pAction = plist->m_item;
				if (pAction && apActObject == pAction->prefObj) {
					plist->Remove(pAction);
					ourFree(pAction);
					plist = pListofActions;
				} else {
					plist = plist->m_pkNext;
				}
			}
		}
	}
}

BGSNote* PlayerCharacter::GetNotebyIndex(int32_t aiIndex) {
	BSSimpleList<BGSNote*>* ecx0 = &listNotes;
	int32_t v4 = 0;
	while (ecx0 && ecx0->m_item) {
		if (v4 == aiIndex) {
			return ecx0->m_item;
		}
		ecx0 = ecx0->m_pkNext;
		++v4;
	}
	return 0;
}

void PlayerCharacter::UpdateMenuModeButton(float afTimeDelta) {
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	Controls* pcontrols = appMain->GetControls();
	if (bPipboyLightDebounce ||
		(!pcontrols->GetUserAction(Controls::UACTN_MENUMODE, 2) ||
			!(fPlayerPipBoyLightTimer.Float() > static_cast<double>(fMenuModeButtonTimer)))) {
		if (pcontrols->GetUserAction(Controls::UACTN_MENUMODE, 1) ||
			pcontrols->GetUserAction(Controls::UACTN_MENUMODE, 0)) {
			fMenuModeButtonTimer = fMenuModeButtonTimer + afTimeDelta;
			if (!bPipboyLightDebounce) {
				if (Interface::GetPipboy()->Get3rdPersonPipboyRoot()) {
					if (fPlayerPipBoyLightTimer.Float() < static_cast<double>(fMenuModeButtonTimer)) {
						MagicItem* a2;
						SpellItem* PipBoyLightSpell = MagicSystem::GetPipBoyLightSpell();
						if (PipBoyLightSpell) {
							a2 = &static_cast<MagicItem&>(static_cast<MagicItemForm&>(*PipBoyLightSpell));
						} else {
							a2 = 0;
						}
						if (IsSpellTarget(a2, 1)) {
							MagicItem* apSpell;
							FOPipboyManager* Pipboy = Interface::GetPipboy();
							Pipboy->ShowPipboyLightEffect(1, 0, 1);
							FOPipboyManager* v3 = Interface::GetPipboy();
							v3->ShowPipboyLightEffect(0, 0, 1);
							BSAudio* v4 = &BSAudio::QInstance();
							BSSoundHandle* SoundHandleByName = v4->GetSoundHandleByName(&a1, "UIPipBoyLightOff", 289u);
							SoundHandleByName->Play(0);
							SpellItem* v15 = MagicSystem::GetPipBoyLightSpell();
							if (v15) {
								apSpell = &static_cast<MagicItem&>(static_cast<MagicItemForm&>(*v15));
							} else {
								apSpell = 0;
							}
							Dispel(apSpell, static_cast<MagicCaster*>(0), static_cast<ActiveEffect*>(0));
						} else {
							if (!pPlayer->GetControlsDisabled(PlayerCharacter::DPC_PIPBOY)) {
								FOPipboyManager* v5 = Interface::GetPipboy();
								v5->ShowPipboyLightEffect(1, 1, 1);
								FOPipboyManager* v6 = Interface::GetPipboy();
								v6->ShowPipboyLightEffect(0, 1, 1);
								BSAudio* v7 = &BSAudio::QInstance();
								BSSoundHandle* v13 = v7->GetSoundHandleByName(&arHandle, "UIPipBoyLightOn", 289u);
								v13->Play(0);
								SpellItem* v8 = MagicSystem::GetPipBoyLightSpell();
								CastAbility(v8, 0);
							}
						}
						bPipboyLightDebounce = 1;
					}
				}
			}
		} else {
			bMenuModeButtonClicked = 0;
			fMenuModeButtonTimer = 0.0f;
			bPipboyLightDebounce = 0;
		}
	} else {
		bMenuModeButtonClicked = 1;
	}
	FOPipboyManager* v9 = Interface::GetPipboy();
	if (v9->QPipboyLightOn() && !Interface::GetPipboy()->Get3rdPersonPipboyRoot()) {
		FOPipboyManager* v10 = Interface::GetPipboy();
		v10->ShowPipboyLightEffect(1, 0, 1);
		FOPipboyManager* v11 = Interface::GetPipboy();
		v11->ShowPipboyLightEffect(0, 0, 1);
		SpellItem* v12 = MagicSystem::GetPipBoyLightSpell();
		if (v12) {
			Dispel(v12, static_cast<MagicCaster*>(0), static_cast<ActiveEffect*>(0));
		} else {
			Dispel(static_cast<MagicItem*>(0), static_cast<MagicCaster*>(0), static_cast<ActiveEffect*>(0));
		}
	}
}

bool PlayerCharacter::CanEquipOrUseObject(ItemChange* apItem) {
	ExtraDataList* pextra;
	float fhealth;
	bool bcanEquip = 1;
	TESBoundObject* pobj = apItem->pContainerObj;
	if (apItem->pExtraObjectList && apItem->pExtraObjectList->m_item) {
		pextra = apItem->pExtraObjectList->m_item;
	} else {
		pextra = 0;
	}
	if (pobj->GetQuestObject() &&
		((pobj->GetFormType() != WEAP_ID && pobj->GetFormType() != ARMO_ID) ||
			!static_cast<TESObjectWEAP*>(pobj)->TESObjectWEAP::GetIsPlayable())) {
		bcanEquip = 0;
	}
	if (pextra) {
		if (pextra->GetCanNotWear()) {
			bcanEquip = 0;
		}
	}
	ExtraHealth* pHealth = 0;
	if (pextra) {
		pHealth = static_cast<ExtraHealth*>(pextra->GetExtraData(EXTRA_HEALTH));
	}
	if (pHealth) {
		fhealth = pHealth->fHealth;
	} else {
		fhealth = static_cast<float>(TESHealthForm::GetFormHealth(pobj));
	}
	switch (apItem->pContainerObj->GetFormType()) {
	case ARMO_ID:
	case WEAP_ID: {
		if (fhealth <= 0.0) {
			bcanEquip = 0;
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
			bcanEquip = 0;
		}
		break;
	}
	case AMMO_ID: {
		bcanEquip = 1;
		TESAmmo* pObj = static_cast<TESAmmo*>(apItem->pContainerObj);
		TESObjectWEAP* pCurrWeapon = GetCurrentWeapon();
		if (pCurrWeapon && pObj) {
			TESAmmo* pAmmo = pCurrWeapon->GetAmmoForm();
			BGSListForm* pAmmoList = pCurrWeapon->GetAmmoFormList();
			if (pAmmo != pObj && (!pAmmoList || pAmmoList->GetItemIndex(pObj) == -1)) {
				bcanEquip = 0;
			}
		} else {
			bcanEquip = 0;
		}
		break;
	}
	default: {
		bcanEquip = 0;
		break;
	}
	}
	return bcanEquip;
}

uint32_t PlayerCharacter::AttemptYield() {
	uint32_t iplayerGroupMemberCount;
	uint32_t iyieldingActors = 0;
	float* v1 = &fCombatYieldTime.Float();
	BSSimpleList<Crime*>* pcrimeList = ReferenceProcessLists.RemoveCrimesForYieldCheck(*v1);
	if (!pcrimeList) {
		return 0;
	}
	CombatGroup* pplayerGroup = pCombatGroup;
	CombatGroup* psplitGroup = 0;
	if (pplayerGroup) {
		iplayerGroupMemberCount = pplayerGroup->GetMemberCount();
	} else {
		iplayerGroupMemberCount = 0;
	}
	uint32_t iteamMateCount = 0;
	uint32_t inonTeamMateCount = 0;
	for (uint32_t i = 0; i < iplayerGroupMemberCount; ++i) {
		Actor* pmember = pplayerGroup->GetMember(i);
		if (pmember != pPlayer) {
			if (pmember->bPlayerTeammate) {
				++iteamMateCount;
			} else {
				++inonTeamMateCount;
			}
		}
	}
	if (inonTeamMateCount) {
		psplitGroup = pCombatManager->CreateCombatGroup();
		psplitGroup->CopyTargets(pplayerGroup);
	}
	uint32_t icombatGroupCount = pCombatManager->GetCombatGroupCount();
	for (uint32_t i_2 = 0; i_2 < icombatGroupCount; ++i_2) {
		CombatGroup* phostileGroup = pCombatManager->GetCombatGroupByIndex(i_2);
		if (phostileGroup &&
			(phostileGroup != pplayerGroup && (phostileGroup != psplitGroup && phostileGroup->IsTarget(this)))) {
			bool bacceptYield = 1;
			bool bacceptNonTeammateYield = 1;
			uint32_t imemberCount = phostileGroup->GetMemberCount();
			for (uint32_t j = 0; j < imemberCount; ++j) {
				Actor* pmember_2 = phostileGroup->GetMember(j);
				FIGHT_REACTION efightReaction = FIGHT_REACTION_NEUTRAL;
				if (pmember_2->GetShouldAttackActor(this, 0, efightReaction, 1)) {
					bacceptYield = 0;
					break;
				}
				if (bacceptNonTeammateYield && inonTeamMateCount) {
					for (uint32_t k = 0; k < iplayerGroupMemberCount; ++k) {
						Actor* pplayerMember = pplayerGroup->GetMember(k);
						if (pplayerMember != pPlayer &&
							(!pplayerMember->bPlayerTeammate &&
								(pmember_2->GetShouldAttackActor(pplayerMember, 0, efightReaction, 1) ||
									pplayerMember->GetShouldAttackActor(pmember_2, 0, efightReaction, 1)))) {
							bacceptNonTeammateYield = 0;
							break;
						}
					}
				}
			}
			if (bacceptYield) {
				Actor* pclosestMember = 0;
				float fclosestMemberDistanceSquared = 3.4028235e+38f;
				NiPoint3 myLoc = GetLocationOnReference();
				if (pplayerGroup) {
					for (uint32_t j_2 = 0; j_2 < iplayerGroupMemberCount; ++j_2) {
						Actor* pplayerMember_2 = pplayerGroup->GetMember(j_2);
						if (bacceptNonTeammateYield ||
							(pplayerMember_2->bPlayerTeammate || pplayerMember_2 == pPlayer)) {
							phostileGroup->RemoveTarget(pplayerMember_2);
						} else {
							if (!bacceptNonTeammateYield) {
								if (psplitGroup) {
									psplitGroup->AddMember(pplayerMember_2);
								}
							}
						}
					}
				} else {
					phostileGroup->RemoveTarget(this);
				}
				for (uint32_t j_3 = 0;; ++j_3) {
					uint32_t TargetCount = phostileGroup->GetTargetCount();
					if (j_3 >= TargetCount) {
						break;
					}
					Actor* ptarget = phostileGroup->GetTarget(j_3);
					if (ptarget) {
						if (ptarget->bPlayerTeammate && !ptarget->IsAngryWithPlayer()) {
							phostileGroup->RemoveTarget(ptarget);
							--j_3;
							ptarget->StopCombat(0);
						}
					}
				}
				for (uint32_t j_4 = 0; j_4 < imemberCount; ++j_4) {
					Actor* pmember_3 = phostileGroup->GetMember(j_4);
					++iyieldingActors;
					if (pplayerGroup) {
						pplayerGroup->RemoveTarget(pmember_3);
					}
					if (bacceptNonTeammateYield) {
						if (psplitGroup) {
							psplitGroup->RemoveTarget(pmember_3);
						}
					}
					const NiPoint3* v4 = &pmember_3->GetLocationOnReference();
					NiPoint3 memberVec = myLoc - *v4;
					float fdistanceSquared_2 = memberVec.SqrLength();
					if (fclosestMemberDistanceSquared > static_cast<double>(fdistanceSquared_2)) {
						pclosestMember = pmember_3;
						fclosestMemberDistanceSquared = fdistanceSquared_2;
					}
					BSSimpleList<Crime*>* plist = pcrimeList;
					BSSimpleList<Crime*>* plastList = 0;
					while (plist && !plist->IsEmpty()) {
						Crime* pcrime = plist->m_item;
						bool bremove = 0;
						if (pcrime) {
							if (pcrime->pcrimetarget == pmember_3) {
								delete pcrime;
								bremove = 1;
							}
						} else {
							bremove = 1;
						}
						if (bremove) {
							if (plastList) {
								plastList->Remove(plist->m_item);
								plist = plastList->m_pkNext;
							} else {
								plist->RemoveHead();
							}
						} else {
							plastList = plist;
							plist = plist->m_pkNext;
						}
					}
				}
				if (pclosestMember) {
					pCombatDialogueManager->StartDialogue(pclosestMember, this, DT_COMBAT, 11, 1, 0);
				}
			}
		}
	}
	if (pplayerGroup) {
		if (iyieldingActors) {
			for (uint32_t j_5 = 0; j_5 < pplayerGroup->GetMemberCount(); ++j_5) {
				Actor* pplayerMember_3 = pplayerGroup->GetMember(j_5);
				if (pplayerMember_3) {
					if (pplayerMember_3->GetCombatController()) {
						CombatController* v5 = pplayerMember_3->GetCombatController();
						v5->ForceStopCombat();
					}
				}
			}
			while (pplayerGroup->GetTargetCount()) {
				Actor* Target = pplayerGroup->GetTarget(0);
				pplayerGroup->RemoveTarget(Target);
			}
		}
	}
	for (Actor* pmember_4 : *ReferenceProcessLists.GetAliveActorList()) {
		if (pmember_4) {
			TESPackage* pspec__pflee = pmember_4->GetCurrentPackage();
			if (pspec__pflee) {
				PACKAGE_TYPE epackType = pspec__pflee->GetPackType();
				switch (epackType) {
				case PACKAGE_TYPE_SPECTATOR: {
					static_cast<SpectatorPackage*>(pspec__pflee)
						->SpectatorPackage::RemoveActorAsThreatSourceOrTarget(*pPlayer);
					pmember_4->EvaluatePackage(0, 0);
					break;
				}
				case PACKAGE_TYPE_FLEE: {
					static_cast<FleePackage*>(pspec__pflee)->FleePackage::RemoveAvoidedRef(pPlayer);
					pmember_4->EvaluatePackage(0, 0);
					break;
				}
				case PACKAGE_TYPE_FLEE_NON_COMBAT: {
					pmember_4->EvaluatePackage(0, 0);
					break;
				}
				}
			}
		}
	}
	bPlayerInCombat = 0;
	for (Crime* pcrime_2 : *pcrimeList) {
		if (pcrime_2) {
			Actor* pactor = static_cast<Actor*>(pcrime_2->pcrimetarget);
			if (pactor) {
				pactor->AddBackCrime(pcrime_2);
			}
		}
	}
	if (psplitGroup) {
		uint32_t isplitGroupMembers = psplitGroup->GetMemberCount();
		for (uint32_t i_3 = 0; i_3 < isplitGroupMembers; ++i_3) {
			Actor* psplitMember = psplitGroup->GetMember(i_3);
			psplitMember->SetCombatGroup(psplitGroup);
			pplayerGroup->RemoveMember(psplitMember);
		}
	}
	return iyieldingActors;
}

void PlayerCharacter::AddRockItAmmo(TESBoundObject* apObj, ExtraDataList* apExtra, int32_t aiNumAdded) {
	bool bfoundMatch = 0;
	for (int32_t iitemCt = 0; !bfoundMatch; ++iitemCt) {
		uint32_t FormFlags = RockItLauncherAmmoList.iSize;
		if (iitemCt >= FormFlags) {
			break;
		}
		ItemChange* pitem = RockItLauncherAmmoList.GetAt(iitemCt);
		if (RockItLauncherAmmoList.GetAt(iitemCt)->pContainerObj == apObj) {
			bool v14 = !pitem->pExtraObjectList ||
				(!pitem->pExtraObjectList->m_item || pitem->pExtraObjectList->m_item->IsExtraDefaultforContainer(0));
			bool bisDefaultStack = v14;
			if (apExtra) {
				for (BSSimpleList<ExtraDataList*>* pextraObj = pitem->pExtraObjectList;
					!bfoundMatch && (pextraObj && pextraObj->m_item); pextraObj = pextraObj->m_pkNext) {
					if (!pextraObj->m_item->CompareListForContainer(apExtra, 1, 0) &&
						!apExtra->CompareListForContainer(pextraObj->m_item, 1, 0)) {
						uint32_t FormHealth_0 = RockItLauncherAmmoList.GetAt(iitemCt)->iNumber;
						pitem->iNumber = aiNumAdded + FormHealth_0;
						int16_t Count = pextraObj->m_item->GetCount();
						pextraObj->m_item->SetCount(aiNumAdded + Count);
						bfoundMatch = 1;
					}
				}
				if (!bfoundMatch && apExtra->IsExtraDefaultforContainer(0) && bisDefaultStack) {
					ExtraDataList* paddedExtra;
					ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
					if (apMemory) {
						paddedExtra = ::new (apMemory) ExtraDataList;
					} else {
						paddedExtra = 0;
					}
					ExtraDataList* paddedExtra_2 = paddedExtra;
					paddedExtra->DuplicateExtraListForContainer(apExtra);
					paddedExtra_2->SetCount(aiNumAdded);
					if (!pitem->pExtraObjectList) {
						BSSimpleList<ExtraDataList*>* v12;
						BSSimpleList<ExtraDataList*>* v18 =
							static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
						if (v18) {
							v12 = ::new (v18) BSSimpleList<ExtraDataList*>;
						} else {
							v12 = 0;
						}
						pitem->pExtraObjectList = v12;
					}
					pitem->pExtraObjectList->AddHead(paddedExtra_2);
					int32_t v9 = aiNumAdded + RockItLauncherAmmoList.GetAt(iitemCt)->iNumber;
					pitem->iNumber = v9;
					bfoundMatch = 1;
				}
			} else {
				if (bisDefaultStack) {
					int32_t v8 = aiNumAdded + RockItLauncherAmmoList.GetAt(iitemCt)->iNumber;
					pitem->iNumber = v8;
					bfoundMatch = 1;
				}
			}
		}
	}
	if (!bfoundMatch) {
		ItemChange* pnewItem;
		ItemChange* v17 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
		if (v17) {
			pnewItem = ::new (v17) ItemChange(apObj, aiNumAdded);
		} else {
			pnewItem = 0;
		}
		ItemChange* pnewItem_2 = pnewItem;
		if (apExtra) {
			ExtraDataList* paddedExtra_3;
			ExtraDataList* v16 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
			if (v16) {
				paddedExtra_3 = ::new (v16) ExtraDataList;
			} else {
				paddedExtra_3 = 0;
			}
			ExtraDataList* paddedExtra_4 = paddedExtra_3;
			paddedExtra_3->DuplicateExtraListForContainer(apExtra);
			paddedExtra_3->SetCount(aiNumAdded);
			pnewItem_2->pExtraObjectList->AddHead(paddedExtra_4);
		}
		RockItLauncherAmmoList.Add(pnewItem_2);
	}
	bool IsHardcore = this->IsHardcore();
	fRockItLauncherAmmoWeight =
		TESWeightForm::GetFormWeight(apObj, IsHardcore) * static_cast<double>(aiNumAdded) + fRockItLauncherAmmoWeight;
}

ItemChange* PlayerCharacter::RemoveRockItAmmo(TESBoundObject* apObj, ExtraDataList* apExtra, int32_t aiNumRemoved,
	bool abReturnRemovedItem) {
	bool bfoundMatch = 0;
	ItemChange* preturnedItem = 0;
	int32_t iamountToRemove = aiNumRemoved;
	for (int32_t iitemCt = 0; !bfoundMatch; ++iitemCt) {
		uint32_t FormFlags = RockItLauncherAmmoList.iSize;
		if (iitemCt >= FormFlags) {
			break;
		}
		ItemChange* pitem = RockItLauncherAmmoList.GetAt(iitemCt);
		if (pitem->pContainerObj == apObj) {
			if (apExtra) {
				for (BSSimpleList<ExtraDataList*>* pextraObj = pitem->pExtraObjectList;
					!bfoundMatch && (pextraObj && pextraObj->m_item); pextraObj = pextraObj->m_pkNext) {
					ExtraDataList* m_item = pextraObj->m_item;
					if (m_item == apExtra) {
						int16_t Count = apExtra->GetCount();
						bool bremoveExtra = Count <= aiNumRemoved;
						if (Count <= aiNumRemoved) {
							iamountToRemove = apExtra->GetCount();
						}
						if (abReturnRemovedItem) {
							ItemChange* preturnedItem_3;
							ExtraDataList* preturnedExtra = apExtra;
							ItemChange* v21 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
							if (v21) {
								preturnedItem_3 = ::new (v21) ItemChange(apObj, iamountToRemove);
							} else {
								preturnedItem_3 = 0;
							}
							preturnedItem = preturnedItem_3;
							if (!bremoveExtra) {
								ExtraDataList* preturnedExtra_2;
								ExtraDataList* v20 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
								if (v20) {
									preturnedExtra_2 = ::new (v20) ExtraDataList;
								} else {
									preturnedExtra_2 = 0;
								}
								preturnedExtra = preturnedExtra_2;
								preturnedExtra_2->DuplicateExtraListForContainer(apExtra);
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
								apExtra = 0;
							}
						} else {
							int16_t v10 = apExtra->GetCount();
							apExtra->SetCount(v10 - aiNumRemoved);
						}
						int32_t FormHealth_0 = pitem->iNumber;
						if (iamountToRemove < FormHealth_0) {
							uint32_t v12 = pitem->iNumber;
							pitem->iNumber = v12 - iamountToRemove;
						} else {
							RockItLauncherAmmoList.Remove(iitemCt, 1);
						}
						bfoundMatch = 1;
					}
				}
			} else {
				if (!pitem->pExtraObjectList || !pitem->pExtraObjectList->m_item) {
					if (abReturnRemovedItem) {
						ItemChange* preturnedItem_2;
						ItemChange* apMemory = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
						if (apMemory) {
							preturnedItem_2 = ::new (apMemory) ItemChange(apObj, iamountToRemove);
						} else {
							preturnedItem_2 = 0;
						}
						preturnedItem = preturnedItem_2;
					}
					int32_t v6 = pitem->iNumber;
					if (iamountToRemove < v6) {
						uint32_t v7 = pitem->iNumber;
						pitem->iNumber = v7 - iamountToRemove;
					} else {
						RockItLauncherAmmoList.Remove(iitemCt, 1);
					}
					bfoundMatch = 1;
				}
			}
		}
	}
	bool IsHardcore = this->IsHardcore();
	double FormWeight = TESWeightForm::GetFormWeight(apObj, IsHardcore);
	fRockItLauncherAmmoWeight = fRockItLauncherAmmoWeight - FormWeight * static_cast<double>(iamountToRemove);
	return preturnedItem;
}

void PlayerCharacter::unset_11E07AB() {
	bAntiCheatActive = 0;
}

void PlayerCharacter::resetAntiCheatTimer() {
	uiAntiCheatTimer = 0;
}

void PlayerCharacter::UpdateHardcoreMode() {
	if (!pPlayer->GetControlsDisabled(PlayerCharacter::DPC_PIPBOY)) {
		float TimeScale = appCalendar.GetTimeScale();
		float v8 = fHCDehydrationRate.Float() * TimeScale / 60.0;
		float v3 = fHCSleepDeprivationRate.Float() * TimeScale / 60.0;
		float v4 = fHCStarvationRate.Float() * TimeScale / 60.0;
		fHardcoreTimerSLP = appCalendar.GetMinutesPassed();
		fHardcoreTimerFOD = fHardcoreTimerSLP;
		fHardcoreTimerH20 = fHardcoreTimerSLP;
		if (fPrevHardcoreTickH20 == 0.0 || NeedResetHardcoreTimer()) {
			fPrevHardcoreTickH20 = fHardcoreTimerH20;
			fPrevHardcoreTickFOD = fHardcoreTimerFOD;
			fPrevHardcoreTickSLP = fHardcoreTimerSLP;
			SetNeedResetHardcoreTimer(0);
		}
		float v2 = fHardcoreTimerH20 - fPrevHardcoreTickH20;
		float v7 = fHardcoreTimerFOD - fPrevHardcoreTickFOD;
		float v6 = fHardcoreTimerSLP - fPrevHardcoreTickSLP;
		if (v8 <= static_cast<double>(v2)) {
			DamageModActorValue(ActorValue::DEHYDRATION, static_cast<int32_t>(v2 / v8), static_cast<Actor*>(0));
			fHardcoreTimerH20 = appCalendar.GetMinutesPassed();
			fPrevHardcoreTickH20 = fHardcoreTimerH20;
		}
		if (v3 <= static_cast<double>(v6)) {
			if (!pPlayer->IsSleeping()) {
				DamageModActorValue(ActorValue::SLEEPDEPREVATION, static_cast<int32_t>(v6 / v3),
					static_cast<Actor*>(0));
			}
			fHardcoreTimerSLP = appCalendar.GetMinutesPassed();
			fPrevHardcoreTickSLP = fHardcoreTimerSLP;
		}
		if (v4 <= static_cast<double>(v7)) {
			DamageModActorValue(ActorValue::HUNGER, static_cast<int32_t>(v7 / v4), static_cast<Actor*>(0));
			fHardcoreTimerFOD = appCalendar.GetMinutesPassed();
			fPrevHardcoreTickFOD = fHardcoreTimerFOD;
		}
	}
}

NiPoint3* PlayerCharacter::GetBulletAutoAim(NiPoint3* apResult) {
	*apResult = BulletAutoAim;
	return apResult;
}

double PlayerCharacter::GetTimeGrenadeHeld() {
	return fProjectileReleaseTimer;
}

PlayerActionObject* PlayerCharacter::GetPlayerAction(PLAYER_ACTION aeAction) {
	PlayerActionObject* v4 = 0;
	if (!pListofActions) {
		return 0;
	}
	for (BSSimpleList<PlayerActionObject*>* ecx0a = pListofActions; ecx0a && (ecx0a->m_item && !v4);
		ecx0a = ecx0a->m_pkNext) {
		PlayerActionObject* v5 = ecx0a->m_item;
		if (v5) {
			if (v5->eAction == aeAction) {
				v4 = v5;
			}
		}
	}
	return v4;
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
		pform = 0;
		iid = 0;
		TESForm::pAllForms->GetNext(iter, iid, pform);
		if (pform) {
			if (pform->GetFormType() == REFR_ID) {
				TESObjectREFR* pref_2 = static_cast<TESObjectREFR*>(pform);
				if (pref_2->GetObjectReference()->GetFormType() == TERM_ID) {
					BGSTerminal::ResetState(pref_2);
				}
			}
		}
	}
}

void PlayerCharacter::DeathMenu() {
	bShowDeathMessage = false;
	int8_t cResult = Interface::GetMessageMenuResult();
	if (cResult == 1) {
		pSaveLoadManager->LoadMostRecentSaveGame();
	} else if (cResult == 2) {
		StartMenu::ChooseMainMenu();
	}
}

bool PlayerCharacter::IsGodMode() {
	return (bool)((uint8_t)(((&::bSetDemigodMode)->Bool()) || PlayerCharacter::bGodMode));
}

bool PlayerCharacter::IsDemigodMode() {
	if ((&::bSetDemigodMode)->Bool()) {
		return (bool)1;
	} else {
		return (bool)((int8_t)PlayerCharacter::bDemigodMode);
	}
}

void PlayerCharacter::SpeakDoneCallback(void* apUnused, bool abSucceeded) {
	pPlayer->SetSpeaking(false);
}

void PlayerCharacter::ForceReload() {
	Interface::ForceReload();
}

void PlayerCharacter::SetGodMode(bool abGodMode) {
	PlayerCharacter::bGodMode = (bool)((uint8_t)abGodMode);
}

void PlayerCharacter::SetDemigodMode(bool abDemigodMode) {
	PlayerCharacter::bDemigodMode = (bool)((int8_t)abDemigodMode);
}

void PlayerCharacter::ForceRestart() {
	appMain->ResetGame();
}

int32_t PlayerCharacter::GetSumOfSPECIALStats() {
	int32_t v0 = pPlayer->GetPermanentActorValue(ActorValue::STRENGTH);
	int32_t v1 = pPlayer->GetPermanentActorValue(ActorValue::PERCEPTION) + v0;
	int32_t v2 = pPlayer->GetPermanentActorValue(ActorValue::ENDURANCE) + v1;
	int32_t v3 = pPlayer->GetPermanentActorValue(ActorValue::CHARISMA) + v2;
	int32_t v4 = pPlayer->GetPermanentActorValue(ActorValue::INTELLIGENCE) + v3;
	int32_t v5 = pPlayer->GetPermanentActorValue(ActorValue::AGILITY) + v4;
	return v5 + pPlayer->GetPermanentActorValue(ActorValue::LUCK);
}

void PlayerCharacter::PoisonWeapon() {
	if (pPlayer->pPendingPoison) {
		if (Interface::GetMessageMenuResult() == 2) {
			char* v0;
			BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
			ItemChange* v2 = CurrentAIProcess->GetCurrentWeapon();
			if (v2) {
				v0 = static_cast<char*>(FNVD_DYNAMIC_CAST_VOID(TESObjectWEAP)(v2->pContainerObj, RTTI_TESBoundObject,
					RTTI_TESObjectWEAP));
			} else {
				v0 = 0;
			}
			if (v0) {
				TESChallengeManager::UpdateChallenge(CHALLENGE_TYPE_USE_ITEM, 1, pPlayer->pPendingPoison, 0, 0, 0, 0);
				TESChallengeManager::UpdateChallenge(CHALLENGE_TYPE_USE_ITEM_LIST, 1, pPlayer->pPendingPoison, 0, 0, 0,
					0);
				v2->SetPoison(pPlayer->pPendingPoison);
				pPlayer->RemoveItem(pPlayer->pPendingPoison, 0, 1, 0, 0, 0, 0, 0, 1, 0);
				Interface::UpdateInventory();
			}
		}
		pPlayer->pPendingPoison = 0;
	}
}

uint32_t BSRandom::UnsignedInt(uint32_t auiMin, uint32_t auiMax) {
	return auiMin + UnsignedInt(auiMax - auiMin);
}

void bhkMouseSpringAction::GetMousePositionInWorld(hkVector4& arPos) {
	hkpMouseSpringAction* pAction = **this;
	if (pAction) {
		arPos = pAction->m_mousePositionInWorld;
	}
}

void bhkMouseSpringAction::GetMousePositionInRBLocal(hkVector4& arPos) {
	hkpMouseSpringAction* pAction = **this;
	if (pAction) {
		arPos = pAction->m_positionInRbLocal;
	}
}

void BSShaderManager::SetFullScreenMotionBlurIronSights(bool abSet) {
	BSShaderManager::bFullScreenMotionBlurIronSights = (bool)((int8_t)abSet);
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
		this->iFormFlags |= 0x400000;
		uint32_t iFormFlags = this->iFormFlags;
	} else {
		this->iFormFlags &= ~0x400000u;
		uint32_t v3 = this->iFormFlags;
	}
	uint32_t v4 = this->iFormFlags;
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
	if (pme)
		return pme->bXPMeterFinishedMoving;
	return false;
}

bool Main::IsInMenuMode() {
	return bInMenuMode || bMenuBGReady;
}
