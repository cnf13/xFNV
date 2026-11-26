#include "Fallout Shared/TESScriptFunctions.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSDevices/XPrefSettings.h"
#include "BSDevices/Xenon/XGamerProfile.h"
#include "BSHavok/bhkAvoidBox.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldNS.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPSysMultiTargetEmitterCtlr.h"
#include "BSMain/BSPolygon.h"
#include "BSMain/BSPortalGraph.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/BSTestObjects.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/IOManager.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSMenu/DebugText.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSMenu/TileRect.h"
#include "BSMovie/MoviePlayer.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/DistantLODShaderProperty.h"
#include "BSShader/HDRParams.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectHDR.h"
#include "BSShader/ImageSpaceEffectMap.h"
#include "BSShader/ImageSpaceEffectNoise.h"
#include "BSShader/ImageSpaceEffectWaterDisplacement.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ParticleShader.h"
#include "BSShader/ParticleShaderProperty.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SkyShader.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "BSShader/WaterShader.h"
#include "BSShader/WaterShaderProperty.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSContentPackage.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "BSSystemUtilities/BSDLCSystemUtility.h"
#include "BSSystemUtilities/BSSteamAwardsSystemUtility.h"
#include "BSSystemUtilities/BSSystemUtility.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSAutoWater.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSCameraPath.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSImpactData.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSMenuIcon.h"
#include "Fallout Shared/BGSMessage.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSPrimitive.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRadiation.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSSceneInfo.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BGSTerminal.h"
#include "Fallout Shared/BGSTextureUseMap.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Date.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainNode.h"
#include "Fallout Shared/ExperiencePoints.h"
#include "Fallout Shared/ExteriorCellLoader.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/FaceGenNpcData.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/ImageSpaceModifierInstanceRB.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Location.h"
#include "Fallout Shared/LockLevel.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshBounds.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/SCRIPT_PARAMETER.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/Sky/Atmosphere.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESCaravan.h"
#include "Fallout Shared/TESCasinoChips.h"
#include "Fallout Shared/TESChallenge.h"
#include "Fallout Shared/TESChallengeManager.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESClimate.h"
#include "Fallout Shared/TESCombatStyle.h"
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
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpace.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESLevCharacter.h"
#include "Fallout Shared/TESLevCreature.h"
#include "Fallout Shared/TESLevItem.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
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
#include "Fallout Shared/TESScriptVariable.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTerrainLODManager.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeather.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Fallout Shared/TopicInfoArray.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/CombatAction.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGoal.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatGroupStrategy.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatPlan.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/CombatWorldState.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/Creature.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Interface/FOHUDEffectManager.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/FOVATSEffectManager.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/MenuConsole.h"
#include "Fallout/Interface/Menus/BarterMenu.h"
#include "Fallout/Interface/Menus/BlackJackMenu.h"
#include "Fallout/Interface/Menus/CaravanMenu.h"
#include "Fallout/Interface/Menus/CharGenMenu.h"
#include "Fallout/Interface/Menus/ComputersMenu.h"
#include "Fallout/Interface/Menus/CreditsMenu.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/HackingMenu.h"
#include "Fallout/Interface/Menus/LevelUpMenu.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/LoveTesterMenu.h"
#include "Fallout/Interface/Menus/MapMenu.h"
#include "Fallout/Interface/Menus/RouletteMenu.h"
#include "Fallout/Interface/Menus/SPECIALBookMenu.h"
#include "Fallout/Interface/Menus/SlotMachineMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/SurgeryMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"
#include "Fallout/Interface/Menus/TraitSelectMenu.h"
#include "Fallout/Interface/Menus/TutorialMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicHitEffect.h"
#include "Fallout/Magic/MagicModelHitEffect.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/MemoryLevelManager.h"
#include "Fallout/Misc/MemoryManagerFile.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFileEntry.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Fallout/Misc/SeenData.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SystemDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDevImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalBuffer.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/mbstring.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/Ini.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <xmmintrin.h>

using TESModelArray = NiTArray<TESModel*, NiTMallocInterface<TESModel*>>;

void SetRBMass(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkRigidBody* pRigidBody = static_cast<bhkRigidBody*>(
        NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, apCollision->GetWorldObject())
    );
    if (pRigidBody) {
        pRigidBody->SetMass(arData.uDataA[0u].f);
        hkpMaxSizeMotion* pMotion = static_cast<hkpMaxSizeMotion*>(pRigidBody->GetStoredDynamicMotion());
        if (pMotion) {
            pMotion->setMass(arData.uDataA[0u].f);
        }
    }
}

// Prints the console's shader error text, through NavMeshInfo::GetCellOrCoords (it returns its first argument).
// BSShaderManager's error callback: its caller passes both arguments, but only the text is read.
void ConsoleShaderError(const char* apErrorString, const NiAVObject* apObject) {
    // The identity's owner, NavMeshInfo::GetCellOrCoords, takes a char*; it only returns the text.
    Interface::PrintLine<>(NavMeshInfo::GetCellOrCoords(const_cast<char*>(apErrorString), 0));
}

bool Script::GetWeatherPercentFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetWeatherPercentConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetDisabledFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    // Passes the script locals pointer on as the condition's first parameter.
    return Script::GetDisabledConditionFunction(apRefObject, apScriptLocals, nullptr, afReturnValue);
}

bool Script::GetIsLockBrokenFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetIsLockBrokenConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::RemoveFromAllFactionsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject && apRefObject->IsActor()) {
        apRefObject->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        ExtraFactionChanges* pFactionChanges = apRefObject->TESObjectREFR::GetExtra()->GetFactionChangesExtra();
        if (!pFactionChanges) {
            apRefObject->TESObjectREFR::GetExtra()->AddFactionChangesExtra();
            pFactionChanges = apRefObject->TESObjectREFR::GetExtra()->GetFactionChangesExtra();
        }
        TESActorBase* pActorBase = static_cast<Actor*>(apRefObject)->GetActorBase();
        pFactionChanges->ExpelFromAllFactions(pActorBase);
    }

    return true;
}

bool Script::GetXPForNextLevelFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetXPForNextLevelConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::IsActorDetected(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        afReturnValue = ReferenceProcessLists.IsActorDetected(pActor);
        if (Script::bSystemOutput) {
            if (0.0 == afReturnValue) {
                Interface::PrintLine(" %s is not detected", pActor->GetFullName());
            }
            else {
                Interface::PrintLine(" %s is detected", pActor->GetFullName());
            }
        }
    }
    return true;
}

bool Script::ShowTraitSelectMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer) {
        TraitSelectMenu::Create();
    }
    return true;
}

bool Script::ShowTraitMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer) {
        TraitMenu::Create();
    }
    return true;
}

bool Script::GetGhostFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetGhostConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetMinorCrimeCountFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetMinorCrimeCountConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::DrawSkeleton(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iLines0 = 0;
    int32_t iLines1 = 0;
    int32_t iLines2 = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iLines0,
            &iLines1,
            &iLines2
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        uint8_t bLines0 = iLines0 != 0;
        uint8_t bLines1 = iLines1 != 0;
        uint8_t bLines2 = iLines2 != 0;
        if (pActor && pActor->pRagdollController) {
            NiNode* pNode = pActor->TESObjectREFR::Get3D();
            if (bLines0 || bLines1 || bLines2) {
                pActor->SetNonLinesAlpha(pNode, 0x1p-1f);
            }
            else {
                pActor->SetNonLinesAlpha(pNode, 1.0);
            }
            pActor->pRagdollController->ShowDebugLines(bLines0, bLines1, bLines2);
        }
    }
    return true;
}

bool Script::IsGoreDisabledFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsGoreDisabledConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetTimeDeadFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::GetTimeDeadConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::bSystemOutput) {
        Interface::PrintLine("time dead >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::ToggleCombatStatsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Interface::PrintLine("Command removed. Use TDT and switch to COMBAT INFO page.");
    return true;
}

bool Script::AddCardToPlayer(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals
        )) {
        Warning("Invalid Parameter used in script AddCardToPlayer");
    }
    if (apRefObject && apRefObject->TESObjectREFR::GetObjectReference()) {
        TESBoundObject* pObject = apRefObject->TESObjectREFR::GetObjectReference();
        TESCaravanCard* pCard = dynamic_cast<TESCaravanCard*>(pObject);
        if (pCard) {
            pPlayer->AddCaravanCard(pCard);
        }
        else {
            Warning("%s is not a valid CaravanCard object", apRefObject->GetFormEditorID());
        }
    }

    return true;
}

bool Script::GetPlayerTeammateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetPlayerTeammateConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::ShowWhoDetectsPlayer(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ReferenceProcessLists.ProcessLists::UpdateEmptyStep(1, -1);
    Interface::PrintLine("Detection list printed");
    return true;
}

bool Script::DispelAllSpellsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor) {
            static_cast<MagicTarget*>(pActor)->RemoveEffects(false);
        }
    }
    return true;
}

bool Script::GetLockedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    // Passes the script locals pointer on as the condition's first parameter.
    return Script::GetLockedConditionFunction(apRefObject, apScriptLocals, nullptr, afReturnValue);
}

bool Script::SameFactionAsPC(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::SameFactionConditionFunction(apRefObject, pPlayer, nullptr, afReturnValue);
}

bool Script::GetIsPlayableRace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetIsPlayableRaceConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::IsActorFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::IsActorConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::bSystemOutput) {
        Interface::PrintLine("IsActor >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetMajorCrimeCountFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetMajorCrimeCountConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetAggroRadiusViolatedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetAggroRadiusViolatedConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetForceHitReactionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals
        )) {
        return false;
    }
    if (apRefObject->IsActor()) {
        afReturnValue = static_cast<Actor*>(apRefObject)->GetForceHitReaction();
    }
    return true;
}

bool Script::AddFaceAnimNoteFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int8_t cNote[516];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cNote
        )) {
        return false;
    }
    if (apRefObject) {
        BSFaceGenAnimationData* pAnimData = apRefObject->GetFaceAnimationData(nullptr);
        if (pAnimData) {
            pAnimData->AddAnimationDataFromNote(static_cast<const char*>(static_cast<const void*>(cNote)));
        }
    }
    return true;
}

bool Script::HasBeenEatenFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::HasBeenEatenConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::IsEssentialFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::IsEssentialConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::bSystemOutput) {
        Interface::PrintLine("IsEssential >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsPlayerMovingIntoNewSpaceFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::IsPlayerMovingIntoNewSpaceConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::bSystemOutput) {
        Interface::PrintLine("Player is moving to new area >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::DropMeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!apRefObject || !apContainer) {
        return true;
    }
    apContainer->RemoveItem(
        apRefObject->TESObjectREFR::GetObjectReference(),
        nullptr,
        1,
        false,
        true,
        nullptr,
        nullptr,
        nullptr,
        true,
        false
    );
    pPlayer->CheckForQuestTargetUpdate(apRefObject);
    if (Interface::IsInMenuMode()) {
        Interface::UpdateInventory();
    }
    return false;
}

bool Script::CloseAllMenusFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Interface::EmergencyCloseAllMenusAndBreakStuff();
    return true;
}

bool Script::RemoveMeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pContainer = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pContainer
        )) {
        return false;
    }
    if (!apRefObject || !apContainer) {
        return true;
    }
    TESObjectREFR* pActor = dynamic_cast<Actor*>(apContainer);
    ExtraDataList* pWornExtra = nullptr;
    if (pActor && pActor->WearingObject(apRefObject->GetObjectReference())) {
        InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(apContainer);
        if (pChanges) {
            pWornExtra = pChanges->WearingObject(apRefObject->GetObjectReference(), false);
        }
    }

    apContainer->RemoveItem(
        apRefObject->GetObjectReference(),
        pWornExtra,
        1,
        false,
        false,
        pContainer,
        nullptr,
        nullptr,
        true,
        false
    );
    pPlayer->CheckForQuestTargetUpdate(apRefObject);
    if (apContainer == pPlayer) {
        Interface::UpdateInventory();
    }
    return false;
}

bool Script::DuplicateAllItemsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pContainer = nullptr;
    int32_t iRetainOwnership = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pContainer,
            &iRetainOwnership
        )) {
        return false;
    }
    if (apRefObject) {
        // &apRefObject->m_Extra is never null.
        InventoryChanges* pChanges = apRefObject->m_Extra.GetContainerChanges();
        if (pChanges) {
            pChanges->DuplicateAllItems(apRefObject, pContainer);
        }
    }
    return true;
}

bool Script::RemoveAllTypedItemsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pContainer = nullptr;
    int32_t iRetainOwnership = 0;
    int32_t iSilent = 0;
    int32_t iFormType = -1;
    BGSListForm* pDoNotRemoveList = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pContainer,
            &iRetainOwnership,
            &iSilent,
            &iFormType,
            &pDoNotRemoveList
        )) {
        return false;
    }
    bool bRetainOwnership = iRetainOwnership != 0;
    if (apRefObject) {
        // &apRefObject->m_Extra is never null.
        InventoryChanges* pChanges = apRefObject->m_Extra.GetContainerChanges();
        if (pChanges) {
            pChanges->RemoveAllTypedItems(
                apRefObject,
                pContainer,
                false,
                bRetainOwnership,
                false,
                iSilent != 0,
                iFormType,
                pDoNotRemoveList
            );
            pPlayer->CheckForQuestTargetUpdate(apRefObject);
        }
    }
    return true;
}

bool Script::RemoveAllItemsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pContainer = nullptr;
    int32_t iRetainOwnership = 0;
    int32_t iSilent = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pContainer,
            &iRetainOwnership,
            &iSilent
        )) {
        return false;
    }
    bool bRetainOwnership = iRetainOwnership != 0;
    if (apRefObject) {
        // &apRefObject->m_Extra is never null.
        InventoryChanges* pChanges = apRefObject->m_Extra.GetContainerChanges();
        if (pChanges) {
            pChanges
                ->RemoveAllItems(apRefObject, pContainer, false, bRetainOwnership, false, iSilent != 0, -1, nullptr);
            pPlayer->CheckForQuestTargetUpdate(apRefObject);
        }
    }
    return true;
}

bool Script::GetOwnershipFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    TESBoundObject* pOwner = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pOwner
        )) {
        return false;
    }
    if (!pOwner) {
        pOwner = pPlayer->GetObjectReference();
    }
    Script::GetOwnershipConditionFunction(apRefObject, pOwner, nullptr, afReturnValue);
    return true;
}

bool Script::SetOwnershipFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESBoundObject* pOwner = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pOwner
        )) {
        return false;
    }
    if (!pOwner) {
        pOwner = pPlayer->GetObjectReference();
    }
    if (apRefObject) {
        apRefObject->m_Extra.SetOwnership(pOwner);
        apRefObject->AddChange(CHANGE_REFR_EXTRA_OWNERSHIP);
    }
    return true;
}

bool Script::DropItemFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESBoundObject* pObject = nullptr;
    int32_t iCount = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pObject,
            &iCount
        )) {
        return false;
    }
    if (apRefObject) {
        apRefObject->RemoveItem(pObject, nullptr, iCount, false, true, nullptr, nullptr, nullptr, true, false);
    }
    return true;
}

bool Script::IsActionRefFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    TESObjectREFR* pRef = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pRef
        )) {
        return false;
    }
    if (apRefObject && pRef) {
        if (pRef == apRefObject->GetActionRef()) {
            afReturnValue = 1.0;
        }
    }

    if (Script::bSystemOutput) {
        Interface::PrintLine("IsActionRef >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::PlayerSpellBook(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BSSimpleList<void*>* pNode = &pDataHandler->listSpellItems;
    int32_t iCount = 0;
    while (pNode && pNode->ItemsInList()) {
        if (pNode->m_item &&
            (static_cast<MagicItem*>(static_cast<SpellItem*>(pNode->m_item))->GetSpellType() == MagicSystem::SPELL ||
             static_cast<MagicItem*>(static_cast<SpellItem*>(pNode->m_item))->GetSpellType() == MagicSystem::POWER ||
             static_cast<MagicItem*>(static_cast<SpellItem*>(pNode->m_item))->GetSpellType() ==
                 MagicSystem::LESSER_POWER) &&
            pPlayer->AddSpell(static_cast<SpellItem*>(pNode->m_item))) {
            ++iCount;
        }

        pNode = pNode->m_pkNext;
    }
    Interface::PrintLine("%d spells added to Player Character", iCount);
    return true;
}

bool Script::PrintAILists(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Interface::PrintLine("Ai Lists Printed");
    return true;
}

bool Script::SetActiveWeather(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESWeather* pWeather = nullptr;
    int32_t iOverride = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pWeather,
            &iOverride
        )) {
        return false;
    }
    Sky* pSky = Sky::GetInstance();
    if (pSky->pCurrentWeather) {
        if (iOverride) {
            pSky->pOverrideWeather = pWeather;
        }
        else {
            pSky->pDefaultWeather = pWeather;
            pPlayer->SetOccupiedRegion(nullptr);
        }
        if (!Script::bSystemOutput) {
            pSky->SetAcceleration(true);
        }
    }
    else {
        pSky->ForceWeather(pWeather, iOverride != 0);
    }
    return true;
}

bool Script::ForceWeather(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESWeather* pWeather = nullptr;
    int32_t iOverride = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pWeather,
            &iOverride
        )) {
        return false;
    }
    bool bOverride = iOverride != 0;
    TESWeather* pForcedWeather = pWeather;
    Sky::GetInstance()->ForceWeather(pForcedWeather, bOverride);
    return true;
}

bool Script::ReleaseWeatherOverride(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Sky::GetInstance()->ReleaseWeatherOverride();
    return true;
}

bool Script::ApplyImageSpaceModifier(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESImageSpaceModifier* pModifier = nullptr;
    float fStrength = 1.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pModifier,
            &fStrength
        )) {
        return false;
    }
    ImageSpaceModifierInstanceForm::Trigger(pModifier, fStrength, nullptr);
    return true;
}

bool Script::SetCellImageSpace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESImageSpace* pImageSpace = nullptr;
    TESObjectCELL* pCell = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pCell,
            &pImageSpace
        )) {
        return false;
    }
    pCell->SetImageSpace(pImageSpace);
    return true;
}

bool Script::SetImageSpace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESImageSpace* pImageSpace = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pImageSpace
        )) {
        return false;
    }
    if (pImageSpace) {
        ImageSpaceParameterData* pData = &pImageSpace->Data;
        BSShaderManager::GetImageSpaceManager()->SetActiveImageSpaceParams(pData);
    }
    else {
        BSShaderManager::GetImageSpaceManager()->SetActiveImageSpaceParams(nullptr);
    }
    BSShaderManager::GetImageSpaceManager()->ResetToBaseParameterData();
    return true;
}

bool Script::ModScaleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fScale = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fScale
        )) {
        return false;
    }
    if (apRefObject) {
        double dCurrentScale = apRefObject->GetScale();
        fScale = dCurrentScale + fScale;
        if (fScale > 2.0) {
            fScale = 2.0f;
        }
        else if (fScale < 0.5) {
            fScale = 0.5f;
        }
        Script::SetScaleBase(apRefObject, fScale);
    }
    return true;
}

bool Script::GetLOSFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetLOSConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::RemoveSpellFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    SpellItem* pSpell = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pSpell
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pTarget = dynamic_cast<Actor*>(apRefObject);
        if (pTarget) {
            if (pTarget->RemoveSpell(pSpell)) {
                afReturnValue = 1.0;
                const char* pTargetName = pTarget->GetFullName();
                Interface::PrintLine("Actor Effect '%s' removed from %s", pSpell->GetFullName(), pTargetName);
            }
            else {
                const char* pTargetName = pTarget->GetFullName();
                Interface::PrintLine("Actor Effect '%s' not found in %s", pSpell->GetFullName(), pTargetName);
            }
        }
    }
    return true;
}

bool Script::CastImmediateOnSelfFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    MagicItem* pSpell = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pSpell
        )) {
        return false;
    }
    if (apRefObject) {
        if (!pSpell || pSpell->GetSpellType() == MagicSystem::ABILITY) {
            return true;
        }
        MagicCaster* pCaster = apRefObject->GetMagicCaster();
        MagicTarget* pTarget = apRefObject->GetMagicTarget();
        pCaster->InterruptCast();
        TESObjectCELL* pCell = apRefObject->GetParentCell();
        if (pCell && TES::pInstance->IsCellLoaded(pCell, true) && TES::pInstance->IsCellLoaded(pCell, false)) {
            pCaster->CastSpellImmediate(pSpell, false, pTarget, 1.0, false);
        }
    }
    return true;
}

bool Script::DispelFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    MagicItem* pSpell = nullptr;
    int32_t iUnused = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pSpell,
            &iUnused
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor) {
            pActor->Dispel(pSpell, nullptr, nullptr);
            if (pSpell->HasScript(apScript)) {
                return false;
            }
        }
    }
    return true;
}

bool Script::DisableAllActors(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pActor = dynamic_cast<Actor*>(apRefObject);
    for (uint32_t uiIndex = 0; uiIndex < ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_LOW);
         ++uiIndex) {
        MobileObject* pMobile = ReferenceProcessLists.MobProcessArray.GetItem(uiIndex);
        if (pMobile && pMobile->IsActor() && pMobile != pActor && !pMobile->GetDisabled()) {
            Script::AddPendingDisabledReference(pMobile, true);
            apScriptLocals->m_cFlags |= ScriptLocals::FLAG_DISABLED;
        }
    }
    return true;
}

bool Script::GetIsFormTypeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cFormType[516];
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cFormType
        )) {
        return false;
    }
    int32_t iFormType = apRefObject->GetObjectReference()->GetFormType();
    if (!std::strcmp(formEnumString[iFormType].pFormString, cFormType)) {
        afReturnValue = 1.0;
    }
    if (Script::bSystemOutput) {
        Interface::PrintLine("Get Is Form Type >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetDetected(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetDetectedConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::GetQuestRunning(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pQuest = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pQuest
           ) &&
        Script::GetQuestRunningConditionFunction(apRefObject, pQuest, nullptr, afReturnValue);
}

bool Script::StartQuestFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* pQuest = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest
        )) {
        return false;
    }
    if (pQuest) {
        pQuest->SetEnabled(1);
    }
    return true;
}

bool Script::StopQuestFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* pQuest = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest
        )) {
        return false;
    }
    if (pQuest) {
        pQuest->SetEnabled(0);
    }
    return true;
}

bool Script::CompleteQuestFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* pQuest = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest
        )) {
        return false;
    }
    if (pQuest) {
        pQuest->SetCompleted(true);
    }
    return true;
}

bool Script::ModFactionRank(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction = nullptr;
    int32_t iModValue = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction,
            &iModValue
        )) {
        return false;
    }
    afReturnValue = 0.0;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (!pActor) {
        return true;
    }
    if (pActor && pFaction) {
        pActor->ModFactionRank(pFaction, iModValue, pActor == pPlayer);
    }

    return true;
}

bool Script::GetFactionReaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    TESFaction* pSourceFaction = nullptr;
    TESFaction* pFaction = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pSourceFaction,
            &pFaction
        )) {
        return false;
    }
    if (pSourceFaction && pFaction) {
        afReturnValue = static_cast<double>(pSourceFaction->GetFactionReaction(pFaction));
    }

    if (Script::bSystemOutput) {
        double dReaction = afReturnValue;
        const char* pFactionName = pFaction->GetFullName();
        Interface::PrintLine("%.20s reaction to %.20s is %.1f", pSourceFaction->GetFullName(), pFactionName, dReaction);
    }
    return true;
}

bool Script::SetFactionReaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pSourceFaction = nullptr;
    TESFaction* pFaction = nullptr;
    int32_t iReaction = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pSourceFaction,
            &pFaction,
            &iReaction
        )) {
        return false;
    }
    afReturnValue = 0.0;
    if (pFaction && pSourceFaction) {
        pSourceFaction->SetFactionReaction_0(pFaction, iReaction);
    }

    return true;
}

bool Script::SetAllyFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction = nullptr;
    TESForm* pOtherFaction = nullptr;
    int32_t iFactionAllies = 0;
    int32_t iOtherFactionAllies = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction,
            &pOtherFaction,
            &iFactionAllies,
            &iOtherFactionAllies
        )) {
        return false;
    }
    afReturnValue = 0.0;
    if (pOtherFaction && pFaction) {
        if (iFactionAllies) {
            pFaction->SetFactionReaction(static_cast<TESFaction*>(pOtherFaction), 3);
        }
        else {
            pFaction->SetFactionReaction(static_cast<TESFaction*>(pOtherFaction), 2);
        }
        if (iOtherFactionAllies) {
            static_cast<TESFaction*>(pOtherFaction)->TESFaction::SetFactionReaction(pFaction, 3);
        }
        else {
            static_cast<TESFaction*>(pOtherFaction)->TESFaction::SetFactionReaction(pFaction, 2);
        }
    }

    return true;
}

bool Script::SetEnemyFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction = nullptr;
    TESForm* pOtherFaction = nullptr;
    int32_t iFactionEnemies = 0;
    int32_t iOtherFactionEnemies = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction,
            &pOtherFaction,
            &iFactionEnemies,
            &iOtherFactionEnemies
        )) {
        return false;
    }
    afReturnValue = 0.0;
    if (pOtherFaction && pFaction) {
        if (iFactionEnemies) {
            pFaction->SetFactionReaction(static_cast<TESFaction*>(pOtherFaction), 0);
        }
        else {
            pFaction->SetFactionReaction(static_cast<TESFaction*>(pOtherFaction), 1);
        }
        if (iOtherFactionEnemies) {
            static_cast<TESFaction*>(pOtherFaction)->TESFaction::SetFactionReaction(pFaction, 0);
        }
        else {
            static_cast<TESFaction*>(pOtherFaction)->TESFaction::SetFactionReaction(pFaction, 1);
        }
    }

    return true;
}

bool Script::ModFactionReaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction = nullptr;
    TESFaction* pOtherFaction = nullptr;
    int32_t iReaction = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction,
            &pOtherFaction,
            &iReaction
        )) {
        return false;
    }
    afReturnValue = 0.0;
    if (pOtherFaction && pFaction) {
        pFaction->ModFactionReaction(pOtherFaction, iReaction);
    }

    return true;
}

bool Script::ShowPivotFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        float fSize = 10.0f;
        if (apRefObject->Get3D()) {
            float fHalfRadius = apRefObject->Get3D()->GetWorldBound().m_fRadius * 0.5;
            if (fSize > static_cast<double>(fHalfRadius)) {
                fSize = fHalfRadius;
            }
        }
        NiPoint3* pkVertex = new NiPoint3[6];
        NiColorA* pkColor = new NiColorA[6];
        bool* pbConnect = static_cast<bool*>(ourMalloc(6 * sizeof(bool)));
        pbConnect[0] = true;
        pbConnect[1] = false;
        pbConnect[2] = true;
        pbConnect[3] = false;
        pbConnect[4] = true;
        pbConnect[5] = false;
        pkColor[0] = NiColorA(1.0f, 1.0f, 0.0f, 1.0f);
        pkColor[1] = NiColorA(1.0f, 1.0f, 0.0f, 1.0f);
        pkColor[2] = NiColorA(1.0f, 1.0f, 0.0f, 1.0f);
        pkColor[3] = NiColorA(1.0f, 1.0f, 0.0f, 1.0f);
        pkColor[4] = NiColorA(1.0f, 1.0f, 0.0f, 1.0f);
        pkColor[5] = NiColorA(1.0f, 1.0f, 0.0f, 1.0f);
        pkVertex[0] = NiPoint3(-fSize, 0.0f, 0.0f);
        pkVertex[1] = NiPoint3(fSize, 0.0f, 0.0f);
        pkVertex[2] = NiPoint3(0.0f, -fSize, 0.0f);
        pkVertex[3] = NiPoint3(0.0f, fSize, 0.0f);
        pkVertex[4] = NiPoint3(0.0f, 0.0f, -fSize);
        pkVertex[5] = NiPoint3(0.0f, 0.0f, fSize);
        NiLines* pLines = new NiLines(
            6,
            pkVertex,
            pkColor,
            nullptr,
            1,
            NiGeometryData::NBT_METHOD_NONE,
            reinterpret_cast<uint8_t*>(pbConnect)
        );
        pLines->SetTranslate(apRefObject->GetLocationOnReference());
        TES::pInstance->AddTempDebugObject(pLines, 20.0f);
    }
    return true;
}

bool Script::GetIsCurrentPackageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pPackage = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pPackage
        )) {
        return false;
    }
    if (apRefObject && pPackage) {
        return Script::GetIsCurrentPackageConditionFunction(apRefObject, pPackage, nullptr, afReturnValue);
    }
    return true;
}

bool Script::ShowSlotMachineMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESCasino* pCasino;
    int32_t iMaxBet;
    int32_t iMinBet;
    int32_t iMaxWinnings;
    if (pPlayer) {
        iMinBet = 0;
        iMaxBet = 0;
        iMaxWinnings = 0;
        if (Script::ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &pCasino,
                &iMinBet,
                &iMaxBet,
                &iMaxWinnings
            )) {
            if (pCasino) {
                SlotMachineMenu::Create(pCasino, iMinBet, iMaxBet, iMaxWinnings, false);
            }
            else {
                Warning("Invalid EditorFormID used in script ShowSlotMachineMenu -- is not a valid EditorFormID");
            }
        }
        else {
            Warning("Invalid Parameters used in script ShowSlotMachineMenu ");
        }
    }
    return true;
}

bool Script::ShowBlackJackMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESCasino* pCasino;
    int32_t iMaxBet;
    int32_t iMinBet;
    int32_t iMaxWinnings;
    if (pPlayer) {
        iMinBet = 0;
        iMaxBet = 0;
        iMaxWinnings = 0;
        if (Script::ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &pCasino,
                &iMinBet,
                &iMaxBet,
                &iMaxWinnings
            )) {
            if (pCasino) {
                BlackJackMenu::Create(pCasino, iMinBet, iMaxBet, 0, false);
            }
            else {
                Warning("Invalid EditorFormID used in script ShowBlackJackMenu --  is not a valid EditorFormID");
            }
        }
        else {
            Warning("Invalid Parameters used in script ShowBlackJackMenu ");
        }
    }
    return true;
}

bool Script::ShowRouletteMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESCasino* pCasino;
    int32_t iMaxBet;
    int32_t iMinBet;
    int32_t iMaxWinnings;
    if (pPlayer) {
        iMinBet = 0;
        iMaxBet = 0;
        iMaxWinnings = 0;
        if (Script::ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &pCasino,
                &iMinBet,
                &iMaxBet,
                &iMaxWinnings
            )) {
            if (pCasino) {
                RouletteMenu::Create(pCasino, iMinBet, iMaxBet, iMaxWinnings, false);
            }
            else {
                Warning("Invalid EditorFormID used in script ShowBlackJackMenu -- is not a valid EditorFormID");
            }
        }
        else {
            Warning("Invalid Parameters used in script ShowRouletteMenu");
        }
    }
    return true;
}

bool Script::ShowCaravanMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESCaravanDeck* pDeck;
    int32_t iDiffLevel;
    float fBetPercentage;
    if (pPlayer) {
        pDeck = nullptr;
        MobileObject* pMobile = nullptr;
        Actor* pOpponent = nullptr;
        iDiffLevel = 0;
        fBetPercentage = 0.0f;
        if (!Script::ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &pDeck,
                &iDiffLevel,
                &fBetPercentage
            )) {
            Warning("Invalid Parameters used in script ShowCaravanMenu ");
            return true;
        }
        if (apRefObject && apRefObject != pPlayer && pDeck) {
            if (apRefObject->IsMobileObject()) {
                pMobile = static_cast<MobileObject*>(apRefObject);
            }
            else {
                BGSTalkingActivator* pActivator = dynamic_cast<BGSTalkingActivator*>(apRefObject->GetObjectReference());
                pOpponent = pActivator ? pActivator->pTempRef : nullptr;
            }
            if (pMobile) {
                if (pMobile->IsActor()) {
                    pOpponent = static_cast<Actor*>(pMobile);
                }
                else if (pMobile->IsTalkingActivator()) {
                    // The talking activator reference's base object is the BGSTalkingActivator.
                    pOpponent = static_cast<BGSTalkingActivator*>(pMobile->GetTalkingActivator()->GetObjectReference())
                                    ->pTempRef;
                }
            }
            CaravanMenu::Create(pOpponent, pDeck, iDiffLevel, fBetPercentage, false);
        }
    }
    return true;
}

bool Script::IsPlayerInRegion(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESRegion* pRegion;
    if (Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pRegion
        )) {
        afReturnValue = 0.0;
        if (pRegion) {
            TESRegionList* pNextList;
            for (TESRegionList* pList = pPlayer->GetAllOccupiedRegions(); pList && pList->m_item; pList = pNextList) {
                if (pList->m_item->iFormID == pRegion->iFormID) {
                    afReturnValue = 1.0;
                    if (Script::bSystemOutput) {
                        Interface::PrintLine("IsPlayerInRegion >> %0.f", afReturnValue);
                    }
                    return true;
                }
                // Walks the nodes as region lists: each next node is downcast to the list around it.
                BSSimpleList<TESRegion*>* pNext = pList->m_pkNext;
                pNextList = static_cast<TESRegionList*>(pNext);
            }
        }
    }
    else {
        Warning("Invalid Parameter used in script IsPlayerInRegion");
    }
    if (Script::bSystemOutput) {
        Interface::PrintLine("IsPlayerInRegion >> %0.f", afReturnValue);
    }
    return true;
}

bool Script::ShowComputersInterface(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject && apRefObject->GetObjectReference()->GetFormType() == TERM_ID) {
        Interface::QueueMenuCreate(
            Interface::OPEN_COMPUTERS,
            apRefObject,
            nullptr,
            nullptr,
            Interface::CMM_STANDARD,
            false
        );
    }
    else {
        Interface::QueueMenuCreate(
            Interface::OPEN_COMPUTERS,
            nullptr,
            nullptr,
            nullptr,
            Interface::CMM_STANDARD,
            false
        );
    }
    return true;
}

bool Script::GetSandmanFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetSandmanConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetCannibalFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetCannibalConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::StartSandmanFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pTarget = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTarget
        )) {
        return false;
    }
    if (apRefObject->IsActor() && pTarget && pTarget->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        BaseProcess* pProcess1 = pActor->GetCurrentAIProcess();
        BaseProcess* pProcess2 = pActor->GetCurrentAIProcess();
        BaseProcess* pProcess3 = pActor->GetCurrentAIProcess();
        // The arguments are evaluated right to left: the index, then the marker, then the
        // furniture.
        uint32_t uiIndex = pProcess1->GetCurrentFurnitureIndex();
        FurnitureMark* pMarker = pProcess2->GetFurnitureMarker();
        TESObjectREFR* pFurniture = pProcess3->GetCurrentFurniture();
        pTarget->InitiateSandmanPackage(pActor, pFurniture, pMarker, static_cast<uint8_t>(uiIndex));
    }

    return true;
}

bool Script::StartCannibalFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pTarget = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTarget
        )) {
        return false;
    }
    if (apRefObject->IsActor() && pTarget && pTarget->IsActor()) {
        pTarget->InitiateCannibalPackage(static_cast<Actor*>(apRefObject));
    }

    return true;
}

bool Script::SetGhostFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iGhost = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iGhost
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        pActor->SetGhost(iGhost > 0);
    }
    if (Script::bSystemOutput) {
        Interface::PrintLine("SetGhost >> %d", iGhost);
    }
    return true;
}

bool Script::EquipItemFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* pItem = nullptr;
    int32_t iNoUnequip = 0;
    int32_t iHideMessage = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pItem,
            &iNoUnequip,
            &iHideMessage
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        bool bNoUnequip = iNoUnequip > 0;
        if (pItem) {
            if (pActor) {
                InventoryChanges* pChanges = pActor->m_Extra.GetContainerChanges();
                ItemChange* pItemChange = nullptr;
                uint8_t bEquip = 1;
                if (pChanges) {
                    pItemChange = pChanges->GetInventoryItem(static_cast<TESBoundObject*>(pItem), 0);
                    if (pItemChange) {
                        // The list's head, item and next getters are trivial accessors.
                        BSSimpleList<ExtraDataList*>* pNode = pItemChange->pExtraObjectList;
                        while (pNode && pNode->GetItem()) {
                            ExtraDataList* pList = pNode->GetItem();
                            if (pList && pList->ExtraDataList::GetWorn(false)) {
                                pList->ExtraDataList::SetCanNotWear(bNoUnequip);
                                bEquip = 0;
                                break;
                            }
                            pNode = pNode->GetNext();
                        }
                        delete pItemChange;
                    }
                }
                if (bEquip) {
                    pActor->QueueEquipObject(static_cast<TESBoundObject*>(pItem), 1, nullptr, true, bNoUnequip, true);
                    if (pActor == pPlayer && !iHideMessage) {
                        BSStringT<char> kMessage;
                        const char* pEquipText = sEquipItemOnPlayer.String();
                        kMessage.SPrintF("%s %s.", TESFullName::GetFullName(pItem), pEquipText);
                        Interface::ShowMessage(
                            kMessage.pString,
                            0,
                            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds",
                            nullptr,
                            2.0f,
                            false
                        );
                        pPlayer->UpdatePlayer3d();
                        pPlayer->pCurrentProcess->SetShouldRefreshMagicShadersEffects(true, false, false);
                        Interface::UpdateInventory();
                    }
                }
            }
        }
        else {
            Warning("SCRIPTS: EquipItem in script '%s' failed to generate an item.", apScript->GetFormEditorID());
        }
    }
    return true;
}

bool Script::UnEquipItemFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESBoundObject* pItem = nullptr;
    int32_t iNoEquip = 0;
    int32_t iHideMessage = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pItem,
            &iNoEquip,
            &iHideMessage
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        bool bNoEquip = iNoEquip > 0;
        if (pItem) {
            if (pActor) {
                InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(apRefObject);
                if (pChanges) {
                    ExtraDataList* pWornExtra = pChanges->WearingObject(pItem, false);
                    if (pWornExtra) {
                        pWornExtra->SetCanNotWear(false);
                        pActor->QueueUnEquipObject(pItem, 1, nullptr, true, bNoEquip, true);
                        if (pActor == pPlayer && !iHideMessage) {
                            BSStringT<char> kMessage;
                            const char* pUnequipText = sUnequipItemOnPlayer.String();
                            kMessage.SPrintF("%s %s.", TESFullName::GetFullName(pItem), pUnequipText);
                            Interface::ShowMessage(
                                kMessage.pString,
                                0,
                                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds",
                                nullptr,
                                2.0f,
                                false
                            );
                            pPlayer->UpdatePlayer3d();
                            pPlayer->pCurrentProcess->SetShouldRefreshMagicShadersEffects(true, false, false);
                            Interface::UpdateInventory();
                        }
                    }
                }
            }
        }
        else {
            InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(apRefObject);
            if (pChanges) {
                ItemChange* pItemChange = pChanges->GetInventoryItem(pItem, 0);
                if (pItemChange) {
                    // The list's head and item getters are trivial accessors.
                    if (pItemChange->pExtraObjectList) {
                        if (pItemChange->pExtraObjectList->GetItem()) {
                            pItemChange->pExtraObjectList->GetItem()->SetCanNotWear(bNoEquip);
                        }
                        else {
                            ExtraDataList* pExtraList;
                            pExtraList = new ExtraDataList;
                            pExtraList->SetCanNotWear(bNoEquip);
                            pItemChange->pExtraObjectList->AddHead(pExtraList);
                        }
                    }
                    else {
                        ExtraDataList* pExtraList;
                        BSSimpleList<ExtraDataList*>* pExtraLists = new BSSimpleList<ExtraDataList*>;
                        pItemChange->pExtraObjectList = pExtraLists;
                        pExtraList = new ExtraDataList;
                        pExtraList->SetCanNotWear(bNoEquip);
                        pItemChange->pExtraObjectList->AddHead(pExtraList);
                    }
                    if (pItemChange) {
                        delete pItemChange;
                    }
                }
            }
        }
    }
    return true;
}

bool Script::SetClassFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESClass* pClass = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pClass
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor && pClass) {
            TESBoundObject* pBaseObject = pActor->GetObjectReference();
            TESNPC* pNPC = dynamic_cast<TESNPC*>(pBaseObject);
            if (pNPC) {
                pNPC->SetClass(pClass);
                pNPC->InitValues(false);
            }
        }
    }
    return true;
}

bool Script::ResetCellFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectCELL* pCell = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pCell
        )) {
        return false;
    }
    if (pCell) {
        pCell->SetDetachTime(0xFFFFFFFFu, false);
    }
    return true;
}

bool Script::ModifyWaterShaderShallowColor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iRed;
    int32_t iGreen;
    int32_t iBlue;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iRed,
            &iGreen,
            &iBlue
        )) {
        return false;
    }
    WaterShaderProperty* pWaterProperty = nullptr;
    if (apRefObject) {
        NiNode* p3D = apRefObject->Get3D();
        NiNode* pChild = static_cast<NiNode*>(p3D->GetAt(0));
        pWaterProperty = static_cast<WaterShaderProperty*>(pChild->GetProperty(3));
    }
    if (iRed >= 0 && iGreen >= 0 && iBlue >= 0 && iRed <= 255 && iGreen <= 255 && iBlue <= 255) {
        if (pWaterProperty) {
            float fB = static_cast<double>(iBlue) / 255.0;
            float fG = static_cast<double>(iGreen) / 255.0;
            float fR = static_cast<double>(iRed) / 255.0;
            pWaterProperty->pShallowColor = NiColorA(fR, fG, fB, 1.0f);
        }
        else {
            float fB = static_cast<double>(iBlue) / 255.0;
            float fG = static_cast<double>(iGreen) / 255.0;
            float fR = static_cast<double>(iRed) / 255.0;
            NiColorA kDefaultColor(fR, fG, fB, 1.0f);
            WaterShader::kShallowColor = kDefaultColor;
        }
    }
    return true;
}

bool Script::ModifyWaterShaderDeepColor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iRed;
    int32_t iGreen;
    int32_t iBlue;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iRed,
            &iGreen,
            &iBlue
        )) {
        return false;
    }
    WaterShaderProperty* pWaterProperty = nullptr;
    if (apRefObject) {
        NiNode* p3D = apRefObject->Get3D();
        NiNode* pChild = static_cast<NiNode*>(p3D->GetAt(0));
        pWaterProperty = static_cast<WaterShaderProperty*>(pChild->GetProperty(3));
    }
    if (iRed >= 0 && iGreen >= 0 && iBlue >= 0 && iRed <= 255 && iGreen <= 255 && iBlue <= 255) {
        if (pWaterProperty) {
            float fB = static_cast<double>(iBlue) / 255.0;
            float fG = static_cast<double>(iGreen) / 255.0;
            float fR = static_cast<double>(iRed) / 255.0;
            pWaterProperty->pDeepColor = NiColorA(fR, fG, fB, 1.0f);
        }
        else {
            float fB = static_cast<double>(iBlue) / 255.0;
            float fG = static_cast<double>(iGreen) / 255.0;
            float fR = static_cast<double>(iRed) / 255.0;
            NiColorA kDefaultColor(fR, fG, fB, 1.0f);
            WaterShader::kDeepColor = kDefaultColor;
        }
    }
    return true;
}

bool Script::ModifyWaterShaderReflectionColor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iRed;
    int32_t iGreen;
    int32_t iBlue;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iRed,
            &iGreen,
            &iBlue
        )) {
        return false;
    }
    WaterShaderProperty* pWaterProperty = nullptr;
    if (apRefObject) {
        NiNode* p3D = apRefObject->Get3D();
        NiNode* pChild = static_cast<NiNode*>(p3D->GetAt(0));
        pWaterProperty = static_cast<WaterShaderProperty*>(pChild->GetProperty(3));
    }
    if (iRed >= 0 && iGreen >= 0 && iBlue >= 0 && iRed <= 255 && iGreen <= 255 && iBlue <= 255) {
        if (pWaterProperty) {
            float fB = static_cast<double>(iBlue) / 255.0;
            float fG = static_cast<double>(iGreen) / 255.0;
            float fR = static_cast<double>(iRed) / 255.0;
            pWaterProperty->pReflectionColor = NiColorA(fR, fG, fB, 1.0f);
        }
        else {
            float fB = static_cast<double>(iBlue) / 255.0;
            float fG = static_cast<double>(iGreen) / 255.0;
            float fR = static_cast<double>(iRed) / 255.0;
            WaterShader::kReflectionColor = NiColorA(fR, fG, fB, 1.0f);
        }
    }
    return true;
}

bool Script::DamageObjectFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fDamage = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fDamage
        )) {
        return false;
    }
    if (!apRefObject) {
        return false;
    }
    if (iNumHWThreads.Int() <= 1) {
        Script::DamageObjectBase(apRefObject, fDamage);
    }
    else {
        TaskQueueInterface::GetInstance()->QueueScriptFunctionCall(0x116Cu, apRefObject, fDamage);
    }
    return true;
}

bool Script::ResetAllTerminals(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    PlayerCharacter::DebugResetAllTerminals();
    return true;
}

bool Script::ExitGame(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* pForm = TESForm::GetFormByNumericID(0xED568u);
    BGSPerk* pPerk = dynamic_cast<BGSPerk*>(pForm);
    float fValue = static_cast<float>((pPlayer->GetPerkRank(pPerk, false) != 0) + 1);
    pPlayCredits->SetValue(fValue);
    Interface::CloseConsole();
    StartMenu::ChooseMainMenu();
    return true;
}

bool Script::ShowWarningFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cMessage[518];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &cMessage[2]
        )) {
        return false;
    }
    BSStringT<char> kString;
    return Script::ParseString(
        apParameters,
        apCompiledParams,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &afReturnValue,
        &auiOffset,
        &cMessage[2],
        &kString
    );
}

// How deep EnterTrigger runs the reference's script inside itself on this thread.
static thread_local int32_t iEnterTriggerRecursionDepth;

bool Script::TriggerEnterFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* pActionRef = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pActionRef
        )) {
        return false;
    }
    if (apRefObject) {
        Script::InitActionList(apRefObject, &apRefObject->m_Extra);
        Script::SetActionFlag(pActionRef, &apRefObject->m_Extra, SCRIPT_ACTION_ONTRIGGERENTER);
        Script::SetActionFlag(pActionRef, &apRefObject->m_Extra, SCRIPT_ACTION_ONTRIGGER);
        apRefObject->SetActionRef(static_cast<TESObjectREFR*>(pActionRef));
        if (iEnterTriggerRecursionDepth < 5) {
            ++iEnterTriggerRecursionDepth;
            apRefObject->RunScript();
            --iEnterTriggerRecursionDepth;
        }
    }
    return true;
}

bool Script::SetConsoleOutputFileFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cFileName[272];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cFileName
        )) {
        return false;
    }
    cFileName[271] = 0;
    if (cFileName[0] && cFileName[0] != 48) {
        MenuConsole::Instance(true)->SetOutputFilename(cFileName);
        if (Script::bSystemOutput) {
            Interface::PrintLine("SetConsoleOutputFilename >> '%s'", cFileName);
        }
    }
    else {
        if (Script::bSystemOutput) {
            Interface::PrintLine("SetConsoleOutputFilename >> OUTPUT_DISABLED");
        }
        MenuConsole::Instance(true)->SetOutputFilename(nullptr);
    }
    return true;
}

bool Script::TestRaceSexTerminalFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Interface::CreateRaceSexMenu(RSM_CONTEXT_NONE);
    return true;
}

bool Script::SetPlayerTeammateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiTeammate = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiTeammate
        )) {
        return false;
    }
    if (apRefObject && apRefObject->IsActor()) {
        static_cast<Actor*>(apRefObject)->Actor::SetPlayerTeammate(uiTeammate != 0);
    }

    return true;
}

bool Script::OpenTeammateContainerFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiForce;
    if (apRefObject && apRefObject->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        uiForce = 0;
        Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiForce
        );
        if (pActor->IsPlayerTeammate() || uiForce) {
            Interface::QueueMenuCreate(
                Interface::OPEN_CONTAINER,
                apRefObject,
                nullptr,
                nullptr,
                Interface::CMM_TEAMMATE,
                false
            );
        }
    }

    return true;
}

bool Script::ClearFactionPlayerEnemyFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction
        )) {
        return false;
    }
    pFaction->SetPCAnEnemy(false);
    return true;
}

bool Script::ResetQuestFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* pQuest = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest
        )) {
        return false;
    }
    if (pQuest) {
        pQuest->ResetQuest();
    }
    return true;
}

// The quest's script delay, and its change flag CHANGE_QUEST_SCRIPT_DELAY (4) for the save game.
bool Script::SetQuestDelayFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* pQuest = nullptr;
    float fDelay = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest,
            &fDelay
        )) {
        return false;
    }
    if (pQuest) {
        pQuest->SetScriptProcessingDelay(fDelay);
        pQuest->AddChange(CHANGE_QUEST_SCRIPT_DELAY);
    }
    return true;
}

bool Script::ClearDestructionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSDestructibleObjectForm::ClearDestruction(apRefObject);
    return true;
}

bool Script::FireWeaponFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pForm
        )) {
        return false;
    }
    if (apRefObject) {
        if (pForm && pForm->GetFormType() == WEAP_ID) {
            TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pForm);
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                TaskQueueInterface::GetInstance()->QueueWeaponFire(pWeapon, apRefObject);
            }
            else {
                pWeapon->Fire(apRefObject);
            }
        }
        else {
            Warning(
                "SCRIPTS: FireWeapon in script '%s' called with non-weapon parameter.",
                apScript->GetFormEditorID()
            );
        }
    }
    return true;
}

bool Script::SetWeaponHealthPercFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fPercent = 1.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fPercent
        )) {
        return false;
    }
    if (apRefObject && apRefObject->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        if (pActor->GetCurrentAIProcess()) {
            BaseProcess* pProc = pActor->GetCurrentAIProcess();
            ItemChange* pWeaponChange = pProc->GetCurrentWeapon();
            if (pWeaponChange) {
                fPercent = fPercent / 100.0;
                bool bModActive = pWeaponChange->HasModEffectActive(10);
                float fFormHealth = static_cast<float>(
                    static_cast<TESObjectWEAP*>(pWeaponChange->pContainerObj)->TESObjectWEAP::GetFormHealth(bModActive)
                );
                float fHealth = fFormHealth * fPercent;
                float fDelta = fHealth - pWeaponChange->GetItemHealth(false);
                if (fDelta > 0.0) {
                    ExtraDataList* pExtraList = pWeaponChange->pExtraObjectList->m_item;
                    InventoryChanges* pChanges = pActor->m_Extra.GetContainerChanges();
                    float fMaxHealth = fFormHealth - 1.0;
                    float fNewHealth = BSMIN(fHealth, fMaxHealth);
                    pWeaponChange->SetItemHealth(fNewHealth, pChanges, pExtraList, true);
                }
                else {
                    pActor->DamageEquipment(pWeaponChange, -fDelta, true);
                }
            }
        }
    }

    return true;
}

bool Script::ModWeaponHealthPercFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fPercent = 1.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fPercent
        )) {
        return false;
    }
    if (apRefObject && apRefObject->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        if (pActor->GetCurrentAIProcess()) {
            BaseProcess* pProc = pActor->GetCurrentAIProcess();
            ItemChange* pWeaponChange = pProc->GetCurrentWeapon();
            if (pWeaponChange) {
                double dItemHealth = pWeaponChange->GetItemHealth(true);
                fPercent = dItemHealth + fPercent;
                fPercent = fPercent / 100.0;
                if (fPercent < 0.0) {
                    fPercent = 0.0f;
                }
                float fFormHealth = static_cast<float>(TESHealthForm::GetFormHealth(pWeaponChange->pContainerObj));
                float fHealth = fFormHealth * fPercent;
                float fDelta = fHealth - pWeaponChange->GetItemHealth(false);
                if (fDelta > 0.0) {
                    ExtraDataList* pExtraList = pWeaponChange->pExtraObjectList->m_item;
                    InventoryChanges* pChanges = pActor->m_Extra.GetContainerChanges();
                    pWeaponChange->SetItemHealth(fHealth, pChanges, pExtraList, true);
                }
                else {
                    pActor->DamageEquipment(pWeaponChange, -fDelta, true);
                }
            }
        }
    }

    return true;
}

bool Script::SetRumbleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fLeftMotor = 0.0f;
    float fRightMotor = 0.0f;
    float fDuration = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fLeftMotor,
            &fRightMotor,
            &fDuration
        )) {
        return false;
    }
    Controls* pControls = appMain->GetControls();
    if (pControls) {
        pControls->Rumble(fLeftMotor, fRightMotor, static_cast<int32_t>(fDuration * 1000.0), 0, CONSTANT, false, true);
    }
    return true;
}

bool Script::SetNoActivationSound(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESSound* pSound = nullptr;
    if (Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pSound
        ) &&
        pSound) {
        TESSound* pPrecacheSound = pSound;
        const char* pFilename = pSound->GetFilename();
        BSAudio::QInstance().PrecacheSound(pFilename, 0x20000121u, pPrecacheSound);
        pPrecacheSound = pSound;
        pFilename = pSound->GetFilename();
        BSAudio::QInstance().PrecacheSound(pFilename, 0x20000121u, pPrecacheSound);
        pPrecacheSound = pSound;
        pFilename = pSound->GetFilename();
        BSAudio::QInstance().PrecacheSound(pFilename, 0x20000121u, pPrecacheSound);
        FalloutAudio::SetNoActivationSound(pSound);
    }

    return true;
}

bool Script::AbsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fValue = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fValue
        )) {
        return false;
    }
    afReturnValue = std::fabs(fValue);
    return true;
}

bool Script::EnableWeaponModFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iModSlot = 0;
    TESObjectREFR* pWeapon = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pWeapon,
            &iModSlot
        )) {
        return false;
    }
    if (apRefObject->IsActor() && iModSlot > 0 && iModSlot <= 3) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        // The weapon's first instance's extra data.
        ItemChange* pChange = pActor->GetCurrentAIProcess()->GetCurrentWeapon();
        ExtraDataList* pList = pChange->pExtraObjectList->m_item;
        pList->SetWeaponModSlot(1 << (iModSlot - 1));
    }
    return true;
}

bool Script::AddReputationFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiType = 0;
    int32_t iValue = 0;
    TESReputation* pReputation = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pReputation,
            &uiType,
            &iValue
        )) {
        return false;
    }
    if (uiType > 1) {
        return false;
    }
    pReputation->AddReputation(static_cast<ReputationType>(uiType), iValue);
    return true;
}

bool Script::AddReputationExactFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iType = 0;
    float fValue = 0.0f;
    TESReputation* pReputation = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pReputation,
            &iType,
            &fValue
        )) {
        return false;
    }
    if (iType < 0 || iType > 1) {
        return false;
    }
    pReputation->AddReputationExact(static_cast<ReputationType>(iType), fValue);
    return true;
}

bool Script::RemoveReputationFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiType = 0;
    int32_t iValue = 0;
    TESReputation* pReputation = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pReputation,
            &uiType,
            &iValue
        )) {
        return false;
    }
    if (uiType > 1) {
        return false;
    }
    pReputation->RemoveReputation(static_cast<ReputationType>(uiType), iValue);
    return true;
}

bool Script::RemoveReputationExactFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iType = 0;
    float fValue = 0.0f;
    TESReputation* pReputation = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pReputation,
            &iType,
            &fValue
        )) {
        return false;
    }
    if (iType < 0 || iType > 1) {
        return false;
    }
    pReputation->RemoveReputationExact(static_cast<ReputationType>(iType), fValue);
    return true;
}

bool Script::SetReputationFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiType = 0;
    float fValue = 0.0f;
    TESReputation* pReputation = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pReputation,
            &uiType,
            &fValue
        )) {
        return false;
    }
    if (uiType > 1) {
        return false;
    }
    pReputation->SetReputation(uiType, fValue);
    return true;
}

bool Script::DetonatePlacedExplosivesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        ProcessArray* pMobArray = &ReferenceProcessLists.MobProcessArray;
        for (uint32_t uiIndex = pMobArray->GetProcessLevelHead(PROCESS_TYPE_HIGH);
             uiIndex < pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH);
             ++uiIndex) {
            Projectile* pProjectile = static_cast<Projectile*>(pMobArray->GetItem(uiIndex));
            if ((!pProjectile || !pProjectile->IsStillLoading()) && pProjectile->IsProjectile()) {
                TESActorBase* pOwner;
                if ((pProjectile->GetShooter() == apRefObject ||
                     (pOwner = static_cast<TESActorBase*>(pProjectile->GetOwner()),
                      pOwner == apRefObject->GetTemplateActorBase())) &&
                    pProjectile->GetProjectileBase()->GetHasExplosionDetonates()) {
                    pProjectile->SetFlag(Projectile::DETONATE, true);
                }
            }
        }
    }
    return true;
}

bool Cmd_IsLoadingMenuEnabled(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = static_cast<double>(Interface::IsInGameLoadingMenuOpen());
    if (Script::bSystemOutput) {
        Interface::PrintLine("IsLoadingMenuEnabled >> %0.f", afReturnValue);
    }
    return true;
}

bool Cmd_IsActorRefEssential(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    if (apRefObject->IsActor()) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        afReturnValue = static_cast<double>(pActor->GetRefEssential());
        if (Script::bSystemOutput) {
            Interface::PrintLine("IsActorRefEssential >> %0.f", afReturnValue);
        }
    }
    else if (Script::bSystemOutput) {
        Interface::PrintLine("IsActorRefEssential >> Invalid actor reference ");
    }
    return true;
}

bool Cmd_SetActorRefEssential(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iEssential;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iEssential
        )) {
        return false;
    }
    if (apRefObject->IsActor()) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        pActor->SetActorRefEssential(iEssential > 0);
    }
    else if (Script::bSystemOutput) {
        Interface::PrintLine("SetActorRefEssential >> Invalid actor reference ");
    }
    return true;
}

bool Cmd_IsNoHumanGoreExe(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    if (Script::bSystemOutput) {
        Interface::PrintLine("IsNoHumanGoreExe >> %0.f", afReturnValue);
    }
    return true;
}

bool Cmd_IncrementScriptedChallenge(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESChallenge* pChallenge;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pChallenge
        )) {
        return false;
    }
    TESChallengeManager::IncrementScriptedChallenge(pChallenge, 1);
    return true;
}

bool Cmd_PersistentRefCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!TESForm::pAllForms) {
        return true;
    }
    TESForm* pForm = nullptr;
    NiTMapItem<uint32_t, TESForm*>* pPos = TESForm::pAllForms->GetFirstPos();
    uint32_t uiFormID = 0;
    uint32_t uiCount = 0;
    afReturnValue = 0.0;
    while (pPos) {
        TESForm::pAllForms->GetNext(pPos, uiFormID, pForm);
        if (pForm && pForm->IsReference() && (pForm->iFormFlags & TESForm::kFormFlags_IsRefPersistent) != 0) {
            ++uiCount;
        }
    }
    afReturnValue = static_cast<double>(uiCount);
    if (Script::bSystemOutput) {
        Interface::PrintLine("PersistentRefCount >> %u", uiCount);
    }
    return true;
}

bool Cmd_AlwaysShowActorSubtitles(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    // The parsed value is tested with a signed compare.
    int32_t iShow;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iShow
        )) {
        return false;
    }
    if (apRefObject->IsActor()) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        pActor->SetAlwaysShowSubtitles(iShow > 0);
    }
    else if (Script::bSystemOutput) {
        Interface::PrintLine("AlwaysShowActorSubtitles >> Invalid actor reference ");
    }
    return true;
}

bool Script::ModifyWaterShader(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    LARGE_INTEGER kEndCount;
    LARGE_INTEGER kFrequency;
    LARGE_INTEGER kStartCount;
    char cSetting[516];
    float fValue = 0.0f;
    if (Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cSetting,
            &fValue
        )) {
        WaterShaderProperty* pWaterProperty = nullptr;
        if (apRefObject) {
            NiNode* pWaterGeometry = static_cast<NiNode*>(
                TES::pInstance->GetWaterSystem()->GetWaterGeometry(apRefObject)
            );
            pWaterProperty = static_cast<WaterShaderProperty*>(pWaterGeometry->GetProperty(3));
        }
        if (BSstricmp(cSetting, "velocity") &&
            BSstricmp(cSetting, "direction") &&
            BSstricmp(cSetting, "amplitude") &&
            BSstricmp(cSetting, "frequency")) {
            if (BSstricmp(cSetting, "reflectamt")) {
                if (BSstricmp(cSetting, "distort")) {
                    if (BSstricmp(cSetting, "fresnel")) {
                        if (BSstricmp(cSetting, "opacity")) {
                            if (BSstricmp(cSetting, "blend")) {
                                if (BSstricmp(cSetting, "noise")) {
                                    if (BSstricmp(cSetting, "off")) {
                                        if (BSstricmp(cSetting, "displaceforce")) {
                                            if (BSstricmp(cSetting, "displacevelocity")) {
                                                if (BSstricmp(cSetting, "displacefalloff")) {
                                                    if (BSstricmp(cSetting, "displacedampener")) {
                                                        if (BSstricmp(cSetting, "rainforce")) {
                                                            if (BSstricmp(cSetting, "rainvelocity")) {
                                                                if (BSstricmp(cSetting, "rainfalloff")) {
                                                                    if (BSstricmp(cSetting, "rainsize")) {
                                                                        if (BSstricmp(cSetting, "reflect")) {
                                                                            if (BSstricmp(cSetting, "refract")) {
                                                                                if (BSstricmp(cSetting, "depth")) {
                                                                                    if (BSstricmp(cSetting, "fog")) {
                                                                                        if (BSstricmp(
                                                                                                cSetting,
                                                                                                "lod"
                                                                                            )) {
                                                                                            if (!BSstricmp(
                                                                                                    cSetting,
                                                                                                    "autowa"
                                                                                                    "te"
                                                                                                    "r"
                                                                                                )) {
                                                                                                char cMessage[256];
                                                                                                double
                                                                                                    dTicksPerMicrosecond =
                                                                                                        1000.0;
                                                                                                kStartCount.u
                                                                                                    .LowPart = 0;
                                                                                                QueryPerformanceFrequency(
                                                                                                    &kFrequency
                                                                                                );
                                                                                                dTicksPerMicrosecond =
                                                                                                    static_cast<double>(
                                                                                                        kFrequency
                                                                                                            .QuadPart
                                                                                                    ) *
                                                                                                    1e-06;
                                                                                                QueryPerformanceCounter(
                                                                                                    &kStartCount
                                                                                                );
                                                                                                TESObjectCELL* pCell =
                                                                                                    TES::pInstance
                                                                                                        ->GetCurrentCell();
                                                                                                BGSAutoWater::
                                                                                                    GeneratePlaceableWaterForCell(
                                                                                                        pCell
                                                                                                    );
                                                                                                QueryPerformanceCounter(
                                                                                                    &kEndCount
                                                                                                );
                                                                                                double dMicroseconds =
                                                                                                    static_cast<double>(
                                                                                                        kEndCount
                                                                                                            .QuadPart -
                                                                                                        kStartCount
                                                                                                            .QuadPart
                                                                                                    ) /
                                                                                                    dTicksPerMicrosecond;
                                                                                                int64_t iTicks =
                                                                                                    kEndCount.QuadPart -
                                                                                                    kStartCount
                                                                                                        .QuadPart;
                                                                                                pCell =
                                                                                                    TES::pInstance
                                                                                                        ->GetCurrentCell();
                                                                                                // The
                                                                                                // 64-bit
                                                                                                // conversion
                                                                                                // is
                                                                                                // spelled
                                                                                                // %lld, the
                                                                                                // standard
                                                                                                // length,
                                                                                                // rather
                                                                                                // than
                                                                                                // MSVC's
                                                                                                // own
                                                                                                // %I64d.
                                                                                                BSsprintf(
                                                                                                    cMessage,
                                                                                                    255,
                                                                                                    "Genera"
                                                                                                    "tePlac"
                                                                                                    "eableW"
                                                                                                    "aterFo"
                                                                                                    "rCell("
                                                                                                    ") for "
                                                                                                    "cell "
                                                                                                    "%s: "
                                                                                                    "Ticks:"
                                                                                                    " %lld,"
                                                                                                    " Micro"
                                                                                                    "second"
                                                                                                    "s: %f",
                                                                                                    pCell
                                                                                                        ->GetCellName(),
                                                                                                    iTicks,
                                                                                                    dMicroseconds
                                                                                                );
                                                                                                InitializationFlow(
                                                                                                    cMessage,
                                                                                                    0
                                                                                                );
                                                                                                if (
                                                                                                    !TESWaterSystem::
                                                                                                        GetWaterEnabled()
                                                                                                ) {
                                                                                                    TES::pInstance
                                                                                                        ->GetWaterSystem()
                                                                                                        ->EnableWaterSystem();
                                                                                                }
                                                                                            }
                                                                                        }
                                                                                        else {
                                                                                            if (
                                                                                                TES::pInstance
                                                                                                    ->GetWaterSystem()
                                                                                            ) {
                                                                                                TES::pInstance
                                                                                                    ->GetWaterSystem()
                                                                                                    ->DisplayWater(
                                                                                                        true,
                                                                                                        true,
                                                                                                        true
                                                                                                    );
                                                                                            }
                                                                                            Interface::PrintLine(
                                                                                                "Toggle"
                                                                                                " LOD "
                                                                                                "Wate"
                                                                                                "r"
                                                                                            );
                                                                                        }
                                                                                    }
                                                                                    else if (
                                                                                        fValue >= 0.0 && fValue <= 100.0
                                                                                    ) {
                                                                                        if (pWaterProperty) {
                                                                                            pWaterProperty
                                                                                                ->fFogAmount = fValue /
                                                                                                100.0;
                                                                                            Interface::PrintLine(
                                                                                                "set "
                                                                                                "fog "
                                                                                                "amount"
                                                                                                " to "
                                                                                                "%d",
                                                                                                static_cast<int32_t>(
                                                                                                    fValue
                                                                                                )
                                                                                            );
                                                                                            pWaterProperty
                                                                                                ->bUpdateConstants =
                                                                                                false;
                                                                                        }
                                                                                        else {
                                                                                            WaterShader::pFogColor
                                                                                                .a = fValue / 100.0;
                                                                                            Interface::PrintLine(
                                                                                                "set "
                                                                                                "fog "
                                                                                                "amount"
                                                                                                " to "
                                                                                                "%d",
                                                                                                static_cast<int32_t>(
                                                                                                    fValue
                                                                                                )
                                                                                            );
                                                                                            g_bIsUseModdedWaterShaders =
                                                                                                1;
                                                                                        }
                                                                                    }
                                                                                }
                                                                                else if (pWaterProperty) {
                                                                                    if (pWaterProperty->bDepth) {
                                                                                        pWaterProperty->bDepth = false;
                                                                                        Interface::PrintLine(
                                                                                            "Toggle"
                                                                                            " Depth"
                                                                                            " : "
                                                                                            "OFF"
                                                                                        );
                                                                                    }
                                                                                    else {
                                                                                        pWaterProperty->bDepth = true;
                                                                                        Interface::PrintLine(
                                                                                            "Toggle"
                                                                                            " Depth"
                                                                                            " : "
                                                                                            "ON"
                                                                                        );
                                                                                    }
                                                                                }
                                                                                else if (
                                                                                    TESWaterSystem::bUseWaterDepth
                                                                                        .Bool()
                                                                                ) {
                                                                                    TESWaterSystem::
                                                                                        bUseWaterDepth = false;
                                                                                    Interface::PrintLine(
                                                                                        "Toggle"
                                                                                        " Globa"
                                                                                        "l "
                                                                                        "Water "
                                                                                        "Depth "
                                                                                        ": "
                                                                                        "OFF"
                                                                                    );
                                                                                }
                                                                                else {
                                                                                    TESWaterSystem::
                                                                                        bUseWaterDepth = true;
                                                                                    Interface::PrintLine(
                                                                                        "Toggle"
                                                                                        " Globa"
                                                                                        "l "
                                                                                        "Water "
                                                                                        "Depth "
                                                                                        ": ON"
                                                                                    );
                                                                                }
                                                                            }
                                                                            else if (pWaterProperty) {
                                                                                if (pWaterProperty->bRefractions) {
                                                                                    pWaterProperty
                                                                                        ->bRefractions = false;
                                                                                    Interface::PrintLine(
                                                                                        "Toggle "
                                                                                        "Refraction"
                                                                                        "s : OFF"
                                                                                    );
                                                                                }
                                                                                else {
                                                                                    pWaterProperty->bReflections = true;
                                                                                    Interface::PrintLine(
                                                                                        "Toggle "
                                                                                        "Refraction"
                                                                                        "s : ON"
                                                                                    );
                                                                                }
                                                                            }
                                                                            else if (
                                                                                TESWaterSystem::bUseWaterRefractions
                                                                                    .Bool()
                                                                            ) {
                                                                                TESWaterSystem::
                                                                                    bUseWaterRefractions = false;
                                                                                Interface::PrintLine(
                                                                                    "Toggle "
                                                                                    "Global "
                                                                                    "Water "
                                                                                    "Refraction"
                                                                                    "s : OFF"
                                                                                );
                                                                            }
                                                                            else {
                                                                                TESWaterSystem::
                                                                                    bUseWaterRefractions = true;
                                                                                Interface::PrintLine(
                                                                                    "Toggle "
                                                                                    "Global "
                                                                                    "Water "
                                                                                    "Refraction"
                                                                                    "s : ON"
                                                                                );
                                                                            }
                                                                        }
                                                                        else if (pWaterProperty) {
                                                                            if (pWaterProperty->bReflections) {
                                                                                pWaterProperty->bReflections = false;
                                                                                Interface::PrintLine(
                                                                                    "Toggle "
                                                                                    "Reflections : "
                                                                                    "OFF"
                                                                                );
                                                                            }
                                                                            else {
                                                                                pWaterProperty->bReflections = true;
                                                                                Interface::PrintLine(
                                                                                    "Toggle "
                                                                                    "Reflections : ON"
                                                                                );
                                                                            }
                                                                        }
                                                                        else if (
                                                                            TESWaterSystem::bUseWaterReflections.Bool()
                                                                        ) {
                                                                            TESWaterSystem::
                                                                                bUseWaterReflections = false;
                                                                            Interface::PrintLine(
                                                                                "Toggle Global "
                                                                                "Water Reflections "
                                                                                ": OFF"
                                                                            );
                                                                        }
                                                                        else {
                                                                            TESWaterSystem::bUseWaterReflections = true;
                                                                            Interface::PrintLine(
                                                                                "Toggle Global "
                                                                                "Water Reflections "
                                                                                ": ON"
                                                                            );
                                                                        }
                                                                    }
                                                                    else if (fValue >= 0.0 && fValue <= 1.0) {
                                                                        ImageSpaceEffectWaterDisplacement::RainVariables
                                                                            .a = fValue;
                                                                        g_bIsUseModdedWaterShaders = 1;
                                                                    }
                                                                }
                                                                else if (fValue >= 0.0 && fValue <= 1.0) {
                                                                    ImageSpaceEffectWaterDisplacement::RainVariables
                                                                        .b = fValue;
                                                                    g_bIsUseModdedWaterShaders = 1;
                                                                }
                                                            }
                                                            else if (fValue >= 0.0 && fValue <= 1.0) {
                                                                ImageSpaceEffectWaterDisplacement::RainVariables
                                                                    .g = fValue;
                                                                g_bIsUseModdedWaterShaders = 1;
                                                            }
                                                        }
                                                        else if (fValue >= 0.0 && fValue <= 1.0) {
                                                            ImageSpaceEffectWaterDisplacement::RainVariables.r = fValue;
                                                            g_bIsUseModdedWaterShaders = 1;
                                                        }
                                                    }
                                                    else if (fValue >= 0.0 && fValue <= 20.0) {
                                                        g_fWaterDisplaceDampener = fValue;
                                                        g_bIsUseModdedWaterShaders = 1;
                                                    }
                                                }
                                                else if (fValue >= 0.0 && fValue <= 1.0) {
                                                    ImageSpaceEffectWaterDisplacement::WadingVariables.b = fValue;
                                                    g_bIsUseModdedWaterShaders = 1;
                                                }
                                            }
                                            else if (fValue >= 0.0 && fValue <= 1.0) {
                                                ImageSpaceEffectWaterDisplacement::WadingVariables.g = fValue;
                                                g_bIsUseModdedWaterShaders = 1;
                                            }
                                        }
                                        else if (fValue >= 0.0 && fValue <= 1.0) {
                                            ImageSpaceEffectWaterDisplacement::WadingVariables.r = fValue;
                                            g_bIsUseModdedWaterShaders = 1;
                                        }
                                    }
                                    else if (pWaterProperty) {
                                        pWaterProperty->bUpdateConstants = true;
                                    }
                                    else {
                                        g_bIsUseModdedWaterShaders = 0;
                                    }
                                }
                                else if (!(fValue >= 0.0) || !(fValue <= 100.0)) {
                                    Interface::PrintLine("usage : mws noise (0.0 - 100.0)", fValue);
                                }
                                else if (pWaterProperty) {
                                    pWaterProperty->fNoiseScale = fValue;
                                    Interface::PrintLine("set noise scale to %f", fValue);
                                    pWaterProperty->bUpdateConstants = false;
                                }
                                else {
                                    ImageSpaceEffectNoise::fNoiseScale = fValue;
                                    Interface::PrintLine("set noise to %f", fValue);
                                    g_bIsUseModdedWaterShaders = 1;
                                }
                            }
                        }
                        else if (!(fValue >= 0.0) || !(fValue <= 100.0)) {
                            Interface::PrintLine("usage : mws opacity (0.0 - 100.0)", fValue);
                        }
                        else if (pWaterProperty) {
                            pWaterProperty->pVarAmounts.z = fValue / 100.0;
                            Interface::PrintLine("set water opacity to %d", static_cast<int32_t>(fValue));
                            pWaterProperty->bUpdateConstants = false;
                        }
                        else {
                            WaterShader::pVarAmounts.b = fValue / 100.0;
                            Interface::PrintLine("set water opacity to %d", static_cast<int32_t>(fValue));
                            g_bIsUseModdedWaterShaders = 1;
                        }
                    }
                    else if (!(fValue >= 0.0) || !(fValue <= 1.0)) {
                        Interface::PrintLine("usage : mws fresnel (0.0 - 1.0)", fValue);
                    }
                    else if (pWaterProperty) {
                        pWaterProperty->fFresnelAmount = fValue;
                        Interface::PrintLine("set fresnel term to %f", fValue);
                        pWaterProperty->bUpdateConstants = false;
                    }
                    else {
                        g_fWaterFresnelTerm = fValue;
                        Interface::PrintLine("set fresnel term to %f", fValue);
                        g_bIsUseModdedWaterShaders = 1;
                    }
                }
                else if (pWaterProperty) {
                    pWaterProperty->pVarAmounts.w = fValue;
                    Interface::PrintLine("set water distortion amount to %f", fValue);
                    pWaterProperty->bUpdateConstants = false;
                }
                else {
                    WaterShader::pVarAmounts.a = fValue;
                    Interface::PrintLine("set water distortion amount to %f", fValue);
                    g_bIsUseModdedWaterShaders = 1;
                }
            }
            else if (!(fValue >= 0.0) || !(fValue <= 1.0)) {
                Interface::PrintLine("usage : mws reflectamt (0.0 - 1.0)", fValue);
            }
            else if (pWaterProperty) {
                pWaterProperty->pVarAmounts.y = fValue;
                Interface::PrintLine("set water reflectivity amount to %f", fValue);
                pWaterProperty->bUpdateConstants = false;
            }
            else {
                WaterShader::pVarAmounts.g = fValue;
                Interface::PrintLine("set water reflectivity amount to %f", fValue);
                g_bIsUseModdedWaterShaders = 1;
            }
        }

        return true;
    }

    Interface::PrintLine("reflectamt, fresnel, opacity, speed, noise, reflect, refract, lod");
    return false;
}

bool Script::EssentailDeathReloadFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cMessage[516];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cMessage
        )) {
        return false;
    }
    if (pSaveLoadGameOLD->GetMostRecentSaveGame()) {
        Interface::CreateMessageMenu(
            cMessage,
            static_cast<const char*>(nullptr),
            static_cast<const char*>(nullptr),
            &PlayerCharacter::ForceReload,
            1,
            0,
            0.0f,
            0.0f,
            static_cast<const char*>(sOk),
            nullptr
        );
    }
    else {
        Interface::CreateMessageMenu(
            cMessage,
            static_cast<const char*>(nullptr),
            static_cast<const char*>(nullptr),
            &PlayerCharacter::ForceRestart,
            1,
            0,
            0.0f,
            0.0f,
            static_cast<const char*>(sOk),
            nullptr
        );
    }
    return true;
}

bool Script::OutputMemContexts(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Interface::PrintLine("OutputMemContexts is no longer supported.  Use Sherlock or OutputMemStats for context info.");
    return true;
}

// The texture's memory size for the dump. Always returns 0.
uint32_t GetTextureDumpSize(NiTexture* apTexture) {
    return 0;
}

bool Script::OutputTextureUseMap(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cFileName[512];
    std::memset(cFileName, 0, sizeof(cFileName));
    BSstrcpy(cFileName, 512, "TextureUseMap.txt");
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cFileName
        )) {
        BSstrcpy(cFileName, 512, "TextureUseMap.txt");
    }
    BGSTextureUseMap kTexUseMap;
    if (apRefObject && apRefObject->Get3D()) {
        kTexUseMap.CollectTexUseDataRecursive(apRefObject->Get3D(), true);
    }
    else {
        kTexUseMap.CollectSceneTexUseData(true);
    }
    if (kTexUseMap.WriteToFile(cFileName)) {
        Interface::PrintLine("map written to file '%s'", cFileName);
    }
    else {
        Interface::PrintLine("FAILED to write to '%s'", cFileName);
    }
    BSStringT<char> kString;
    uint32_t uiSize = kTexUseMap.GetTotalTextureSize();
    if (uiSize <= 0x100000) {
        if (uiSize <= 1024) {
            kString.SPrintF("%d bytes", uiSize);
        }
        else {
            uiSize >>= 10;
            kString.SPrintF("%d KB", uiSize);
        }
    }
    else {
        uiSize >>= 20;
        kString.SPrintF("%d MB", uiSize);
    }
    kString += " textures in use.";
    Interface::PrintLine(static_cast<const char*>(kString.pString));
    return true;
}

bool Script::TestAllCellsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iCount = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iCount
        )) {
        return false;
    }
    if (!iCount) {
        iCount = -1;
    }
    TES::pInstance->TestAllCells(iCount);
    const char* pState = TES::pInstance->CellTestRunning() ? "running" : "stopped";
    Interface::PrintLine("TestAllCells %s", pState);
    return true;
}

bool Script::RenderTestCellFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer->GetParentCell()) {
        TESObjectCELL::RenderTestCell(pPlayer->GetParentCell(), nullptr, true);
        Interface::PrintLine("RenderTestCell complete. Check warnings file for more info.");
    }
    else {
        Interface::PrintLine("RenderTestCell failed: no cell");
    }
    return true;
}

bool Script::RenderTestHereFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer->GetParentCell()) {
        TESObjectCELL::RenderTestCell(pPlayer->GetParentCell(), nullptr, false);
        Interface::PrintLine("RenderTestCell complete. Check warnings file for more info.");
    }
    else {
        Interface::PrintLine("RenderTestCell failed: no cell");
    }
    return true;
}

bool Script::ToggleConversationStatsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint8_t isConversationsStatsEnabled = TESTopic::GetShowConversationStats();
    TESTopic::SetShowConversationStats(!isConversationsStatsEnabled);
    if (TESTopic::GetShowConversationStats()) {
        Interface::PrintLine("Conversation stats %s", "shown.");
    }
    else {
        Interface::PrintLine("Conversation stats %s", "hidden.");
    }
    return true;
}

bool Script::ToggleCollisionGeometryFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TES::pInstance->ToggleCollisionGeometry();
    return true;
}

bool Script::ToggleProjectileDebugFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ToggleProjectileDebug();
    if (Script::bSystemOutput) {
        Interface::PrintLine("Debug lines/shapes now show for projectiles.");
        Interface::PrintLine("    -Yellow line for targeting.");
        Interface::PrintLine("    -Red diamond for spawn point.");
        Interface::PrintLine("    -Blue diamond for non-supersonic/non-hit-scan projectile sound start.");
        Interface::PrintLine("    -Teal diamond for near miss sound from supersonic.");
    }
    return true;
}

void ToggleProjectileDebug() {
    bIsProjectileDebug = !bIsProjectileDebug;
}

bool Script::SetTargetRefractionFire(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iPeriod;
    float fAmount;
    if (BSShaderManager::IsRefractionEnabled()) {
        fAmount = 0.0f;
        iPeriod = 0;
        if (!Script::ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &fAmount,
                &iPeriod
            )) {
            return false;
        }
        if (!dynamic_cast<Actor*>(apRefObject)) {
            float fClamped = BSMIN(fAmount, 1.0f);
            fAmount = BSMAX(fClamped, 0.0);
            bool bEnable = fAmount > 0.0;
            NiNode* p3D = apRefObject->Get3D();
            if (p3D) {
                BSShaderPPLightingProperty::SetRefractionRecurse(
                    p3D,
                    bEnable,
                    fAmount,
                    1,
                    static_cast<float>(iPeriod),
                    0
                );
                if (Script::bSystemOutput) {
                    Interface::PrintLine(
                        "%s refraction fire has been set to %f, period of %d",
                        apRefObject->GetFullName(),
                        fAmount,
                        iPeriod
                    );
                }
            }
        }
    }
    return true;
}

bool Script::ToggleLiteBriteFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint8_t bIsLiteBrite = BSShaderManager::IsLiteBrite();
    BSShaderManager::SetIsLiteBrite(bIsLiteBrite == 0);
    if (Script::bSystemOutput) {
        const char* pState = BSShaderManager::IsLiteBrite() ? "On" : "Off";
        Interface::PrintLine("Lite Brite -> %s", pState);
    }
    return true;
}

bool Script::SetDebugTextFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cPage[512];
    int32_t iValue;
    std::memset(cPage, 0, sizeof(cPage));
    iValue = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cPage,
            &iValue
        )) {
        return false;
    }
    if (cPage[0]) {
        char cFileName[264];
        int32_t iPageValue = iValue;
        DebugTextManager::GetSingleton()->SetScreenDebugPage(cPage, iPageValue);
        BSstrcpy(cFileName, 260, BSUtilities::GetPersonalPath());
        BSstrcat(cFileName, 260, "Custom.ini");
        platform::IniWriteString("Display", "sDebugText", cPage, cFileName);
    }
    else {
        DebugTextManager::GetSingleton()->OutputDebugPageNames(&Interface::PrintLine);
    }
    return true;
}

bool Script::ShowFullScenegraphFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    NiTreeCtrl::SetCallback(&TreeCtrlCallback);
    NiTreeCtrl* pTreeCtrl = new NiTreeCtrl(
        appMain->hInstance,
        appMain->hWnd,
        Main::GetWorldSceneGraph(),
        "Fallout World Scene Graph",
        0x80000000u,
        0x80000000u,
        800,
        600
    );
    if (apRefObject) {
        pTreeCtrl->BuildTree(apRefObject->Get3D());
    }
    pTreeCtrl->CreateTree(Main::MenuRootNode(), "Fallout Interface Scene Graph");
    pTreeCtrl->CreateTree(Main::MenuRootNode3D(), "Fallout Interface 3D Object Scene Graph");
    if (BSShaderManager::GetShadowSceneNode(1)) {
        pTreeCtrl->CreateTree(BSShaderManager::GetShadowSceneNode(1), "Fallout Rendered Menu 3D Scene Graph");
    }
    if (ScreenSplatter::IsActiveSplatter()) {
        pTreeCtrl->CreateTree(ScreenSplatter::GetRoot(), "Fallout Screen Splatter Scene Graph");
    }
    return true;
}

bool Script::SetTerminalMenuPosFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fX = 0.0f;
    float fY = 0.0f;
    float fScale = 1.0f;
    char cExtra[4];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fX,
            &fY,
            &fScale,
            cExtra
        )) {
        return false;
    }
    SetTerminalMenuPos(fX, fY, fScale);
    return true;
}

bool Script::MovePipboyKnobFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fAmount;
    uint32_t uiKnob;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiKnob,
            &fAmount
        )) {
        return false;
    }
    float fMoveAmount = fAmount;
    FOPipboyManager::Knobs eKnob = static_cast<FOPipboyManager::Knobs>(uiKnob);
    Interface::GetPipboy()->MoveKnob(eKnob, fMoveAmount, 0.0005f, true);
    return true;
}

bool Script::ToggleTestLight(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fFire = -1.0f;
    float fX0 = -1.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fFire,
            &fX0
        )) {
        return false;
    }
    bTestLightOn = !(fFire <= 0.0) || !(fX0 <= 0.0) || !bTestLightOn;
    return true;
}

bool Script::ToggleCombatAI(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bool bOn = CombatController::ToggleCombatAI();
    if (Script::bSystemOutput) {
        const char* pState = bOn ? "On" : "Off";
        Interface::PrintLine("All Combat AI processing is %s", pState);
    }
    return true;
}

bool Script::ShowHeadTrackTarget(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        TESObjectREFR* pTarget = pActor->GetExtra()->GetHeadTrackTargetExtra();
        if (Script::bSystemOutput) {
            if (pTarget) {
                const char* pTargetName = pTarget->GetFullName();
                const char* pActorName = pActor->GetFullName();
                Interface::PrintLine("%s set to look at %s", pActorName, pTargetName);
            }
            else {
                Interface::PrintLine("%s set to not look at anyone", pActor->GetFullName());
            }
        }
    }
    return true;
}

bool Script::ToggleHighProcess(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ReferenceProcessLists.bProcessHigh = !ReferenceProcessLists.bProcessHigh;
    if (Script::bSystemOutput) {
        const char* pState = ReferenceProcessLists.bProcessHigh ? "On" : "Off";
        Interface::PrintLine("AI Processing for actors in high is  %s", pState);
    }
    if (!ReferenceProcessLists.bProcessHigh) {
        ReferenceProcessLists.TurnOffMovementinHigh();
    }
    return true;
}

bool Script::ToggleLowProcess(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ReferenceProcessLists.bProcessLow = !ReferenceProcessLists.bProcessLow;
    if (Script::bSystemOutput) {
        const char* pState = ReferenceProcessLists.bProcessLow ? "On" : "Off";
        Interface::PrintLine("AI Processing for actors in Low is  %s", pState);
    }
    return true;
}

bool Script::ToggleMiddleLowProcess(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ReferenceProcessLists.bProcessMLow = !ReferenceProcessLists.bProcessMLow;
    if (Script::bSystemOutput) {
        const char* pState = ReferenceProcessLists.bProcessMLow ? "On" : "Off";
        Interface::PrintLine("AI Processing for actors in Middle Low is  %s", pState);
    }
    return true;
}

bool Script::ToggleMiddleHighProcess(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ReferenceProcessLists.bProcessMHigh = !ReferenceProcessLists.bProcessMHigh;
    if (Script::bSystemOutput) {
        const char* pState = ReferenceProcessLists.bProcessMHigh ? "On" : "Off";
        Interface::PrintLine("AI Processing for actors in Middle High is  %s", pState);
    }
    return true;
}

bool Script::SpeakSoundFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iEmotion;
    char cSound[516];
    Actor* pActor;
    char cSoundCopy[512];
    int32_t iEmotionValue = 50;
    char* pSound = nullptr;
    BSSoundHandle kSoundHandle;
    BSStringT<char> kSoundPath;
    iEmotion = 0;
    if (Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cSound,
            &iEmotion,
            &iEmotionValue
        ) &&
        (pSound = cSound, (BSstrcpy(cSoundCopy, 512, cSound), pSound)) &&
        apRefObject &&
        (pActor = dynamic_cast<Actor*>(apRefObject)) != nullptr &&
        pActor->pCurrentProcess &&
        pActor->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
        if (iEmotion < 0) {
            iEmotion = 0;
        }
        if (iEmotion >= 8) {
            iEmotion = 0;
        }
        if (iEmotionValue < 0) {
            iEmotionValue = 0;
        }
        if (iEmotionValue > 100) {
            iEmotionValue = 100;
        }
        kSoundPath.SPrintF("%s", pSound);
        float fDuration = pActor->SpeakSoundFunction(
            kSoundPath.pString,
            kSoundHandle,
            static_cast<DIALOGUE_EMOTION>(iEmotion),
            iEmotionValue,
            0,
            nullptr,
            nullptr,
            nullptr,
            false,
            false,
            false,
            true,
            true
        );
        pActor->pCurrentProcess->SetGreetingFlag(true);
        pActor->pCurrentProcess->SetSoundDelay(fDuration);
        if (Script::bSystemOutput) {
            Interface::PrintLine("The NPC will speak the sound now.");
        }
        return true;
    }

    return false;
}

bool Script::HelpFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint8_t cExtra[4];
    uint32_t i;
    char cFilter[516];
    cFilter[0] = 0;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        cFilter,
        cExtra
    );
    Interface::PrintLine("----CONSOLE COMMANDS--------------------");
    for (i = 0; i < 205; ++i) {
        if (!cFilter[0] ||
            BSstristr(scriptConsole[i].pHelpString, cFilter) ||
            BSstristr(scriptConsole[i].pFunctionName, cFilter) ||
            BSstristr(scriptConsole[i].pShortName, cFilter)) {
            if (scriptConsole[i].pHelpString && BSstrlen(scriptConsole[i].pHelpString)) {
                if (scriptConsole[i].pShortName && BSstrlen(scriptConsole[i].pShortName)) {
                    Interface::PrintLine(
                        "%s (%s) -> %s",
                        scriptConsole[i].pFunctionName,
                        scriptConsole[i].pShortName,
                        scriptConsole[i].pHelpString
                    );
                }
                else {
                    Interface::PrintLine("%s -> %s", scriptConsole[i].pFunctionName, scriptConsole[i].pHelpString);
                }
            }
            else if (scriptConsole[i].pShortName && BSstrlen(scriptConsole[i].pShortName)) {
                Interface::PrintLine("%s (%s)", scriptConsole[i].pFunctionName, scriptConsole[i].pShortName);
            }
            else {
                Interface::PrintLine("%s", scriptConsole[i].pFunctionName);
            }
        }
    }
    Interface::PrintLine("----SCRIPT FUNCTIONS--------------------");
    for (i = 0; i < 640; ++i) {
        if (!cFilter[0] ||
            BSstristr(scriptFunctions[i].pHelpString, cFilter) ||
            BSstristr(scriptFunctions[i].pFunctionName, cFilter) ||
            BSstristr(scriptFunctions[i].pShortName, cFilter)) {
            if (scriptFunctions[i].pHelpString && BSstrlen(scriptFunctions[i].pHelpString)) {
                if (scriptFunctions[i].pShortName && BSstrlen(scriptFunctions[i].pShortName)) {
                    Interface::PrintLine(
                        "%s (%s) -> %s",
                        scriptFunctions[i].pFunctionName,
                        scriptFunctions[i].pShortName,
                        scriptFunctions[i].pHelpString
                    );
                }
                else {
                    Interface::PrintLine("%s -> %s", scriptFunctions[i].pFunctionName, scriptFunctions[i].pHelpString);
                }
            }
            else if (scriptFunctions[i].pShortName && BSstrlen(scriptFunctions[i].pShortName)) {
                Interface::PrintLine("%s (%s)", scriptFunctions[i].pFunctionName, scriptFunctions[i].pShortName);
            }
            else {
                Interface::PrintLine("%s", scriptFunctions[i].pFunctionName);
            }
        }
    }
    if (cFilter[0]) {
        uint32_t uiIndex;
        Interface::PrintLine("----GAME SETTINGS--------------------");
        NiTPrimitiveArray<char*> kGameSettings(0, 1);
        SettingT<GameSettingCollection>::GetCollection()->GetViewerStrings(&kGameSettings);
        for (uiIndex = 0; uiIndex < kGameSettings.m_usESize; ++uiIndex) {
            char* pSetting = kGameSettings[uiIndex];
            if (pSetting) {
                if (uiIndex && BSstristr(pSetting, cFilter)) {
                    Interface::PrintLine("%s", pSetting);
                }

                ourFree(pSetting);
            }
        }
        Interface::PrintLine("----INI SETTINGS--------------------");
        NiTPrimitiveArray<char*> kINISettings(0, 1);
        SettingT<INISettingCollection>::GetCollection()->GetViewerStrings(&kINISettings);
        for (uiIndex = 0; uiIndex < kINISettings.m_usESize; ++uiIndex) {
            char* pSetting = kINISettings[uiIndex];
            if (pSetting) {
                if (uiIndex && BSstristr(pSetting, cFilter)) {
                    Interface::PrintLine("%s", pSetting);
                }

                ourFree(pSetting);
            }
        }
    }
    return true;
}

bool Script::DisableNavMesh(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cID[512];
    uint32_t uiID;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cID
        )) {
        return false;
    }
    uiID = 0;
    std::sscanf(cID, "%x", &uiID);
    if (uiID) {
        if (apScript && apScript->GetFile(-1)) {
            TESFile* pFile = apScript->GetFile(-1);
            TESForm::AddCompileIndex(uiID, pFile);
        }

        uint32_t uiNavMeshID = uiID;
        NavMeshObstacleManager::GetInstance()->DisableNavMesh(uiNavMeshID);
    }
    return true;
}

bool Script::EnableNavMesh(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cID[512];
    uint32_t uiID;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cID
        )) {
        return false;
    }
    uiID = 0;
    std::sscanf(cID, "%x", &uiID);
    if (uiID) {
        if (apScript && apScript->GetFile(-1)) {
            TESFile* pFile = apScript->GetFile(-1);
            TESForm::AddCompileIndex(uiID, pFile);
        }

        uint32_t uiNavMeshID = uiID;
        NavMeshObstacleManager::GetInstance()->EnableNavMesh(uiNavMeshID);
    }
    return true;
}

bool Script::CenterOnCellFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cCellName[516];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cCellName
        )) {
        return false;
    }
    Interface::CloseConsole();
    pPlayer->CenterOnCell(cCellName, nullptr);
    return true;
}

bool Script::CenterOnExteriorFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iY = 0;
    int32_t iX = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iX,
            &iY
        )) {
        return false;
    }
    TESWorldSpace* pWorldSpace = TES::pInstance->pWorldSpace;
    if (!pWorldSpace) {
        pWorldSpace = static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item);
    }
    if (!pWorldSpace) {
        return false;
    }
    if (pExteriorCellLoader) {
        pExteriorCellLoader->CancelAllCellLoads();
    }
    TESObjectCELL* pCell = pWorldSpace->GetCellFromCellCoord(iX, iY);
    if (!pCell) {
        pCell = pWorldSpace->LoadCell(iX, iY);
    }
    if (!pCell) {
        pCell = pDataHandler->NewCell(nullptr, iX, iY, pWorldSpace);
    }
    if (pCell) {
        Interface::CloseConsole();
        pPlayer->CenterOnCell(nullptr, pCell);
    }
    pPlayer->bForceQuestTargetUpdate = true;
    return true;
}

bool Script::CenterOnWorldFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iY = 0;
    TESWorldSpace* pWorldSpace = nullptr;
    int32_t iX = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pWorldSpace,
            &iX,
            &iY
        )) {
        return false;
    }
    if (!pWorldSpace) {
        return false;
    }
    if (pExteriorCellLoader) {
        pExteriorCellLoader->CancelAllCellLoads();
    }
    TESObjectCELL* pCell = pWorldSpace->GetCellFromCellCoord(iX, iY);
    if (!pCell) {
        pCell = pWorldSpace->LoadCell(iX, iY);
    }
    if (!pCell) {
        pCell = pDataHandler->NewCell(nullptr, iX, iY, pWorldSpace);
    }
    if (pCell) {
        Interface::CloseConsole();
        pPlayer->CenterOnCell(nullptr, pCell);
    }
    pPlayer->bForceQuestTargetUpdate = true;
    return true;
}

bool Script::SetSizeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fSize = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fSize
        )) {
        return false;
    }
    MobileObject* pMobile = dynamic_cast<Actor*>(apRefObject);
    if (pMobile) {
        bhkCharacterController* pCharController = pMobile->GetCharController();
        if (pCharController) {
            pCharController->SetSize(fSize);
        }
    }
    return true;
}

bool Script::KillActorFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pKiller = nullptr;
    CAUSE_OF_DEATH eCause = CAUSE_OF_DEATH_NONE;
    BGSBodyPart::LIMB_ENUM eLimb = BGSBodyPart::LIMB_NONE;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &pKiller,
        &eLimb,
        &eCause
    );
    PlayerCharacter* pActor = static_cast<PlayerCharacter*>(dynamic_cast<Actor*>(apRefObject));
    if (pActor) {
        if (pActor->IsInvulnerable()) {
            return true;
        }
        if (pActor == pPlayer && pActor->IsChild(false)) {
            eLimb = BGSBodyPart::LIMB_NONE;
        }
        bool bQueue = AILinearTaskThreadManager::ShouldQueue3DTask();
        if (pKiller == pPlayer) {
            int32_t iLevel = pActor->GetLevel();
            bool bCreature = pActor->IsCreature();
            float fBaseXP = static_cast<float>(
                ExperiencePoints::GetExperiencePoints(static_cast<ExperiencePoints::Activity>(!bCreature), iLevel)
            );
            GamePlayFormulas::DifficultyLevel eDifficulty = pPlayer->GetGameDifficulty();
            float fAdjustedXP = GamePlayFormulas::AdjustExperiencePoints(fBaseXP, eDifficulty);
            double dXP = std::floorf(fAdjustedXP);
            pPlayer->RewardExperience(static_cast<int32_t>(dXP));
        }
        if (pActor->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
            pActor->SetLifeState(ACTOR_LIFE_STATE_ALIVE);
        }
        if (bQueue) {
            DISMEMBER_DATA* pDismemberData = nullptr;
            if (eLimb != BGSBodyPart::LIMB_NONE) {
                pDismemberData = static_cast<DISMEMBER_DATA*>(ourMalloc(sizeof(DISMEMBER_DATA)));
                pDismemberData->iCause = eCause;
                pDismemberData->iLimb = eLimb;
                pDismemberData->pKiller = pKiller;
            }
            float* pfDelay = static_cast<float*>(ourMalloc(sizeof(float)));
            *pfDelay = 0.0f;
            pActor->pMyKiller = pKiller;
            TaskQueueInterface::GetInstance()->QueueActorKill(pActor, pfDelay, pDismemberData);
        }
        else {
            pActor->Kill(pKiller, 0.0f);
            if (eLimb != BGSBodyPart::LIMB_NONE) {
                pActor->ScriptDismember(eCause, eLimb, pKiller);
            }
        }
    }
    return true;
}

bool Script::KillAllActorsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pKiller = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pKiller
        )) {
        return false;
    }
    for (uint32_t uiIndex = 0;
         uiIndex < ReferenceProcessLists.GetProcessArray()->GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++uiIndex) {
        Actor* pActor = static_cast<Actor*>(ReferenceProcessLists.GetProcessArray()->GetItem(uiIndex));
        if (pActor && pActor->IsActor()) {
            pActor->Kill(pKiller, 0.0);
        }
    }
    return true;
}

bool Script::ResurrectActorFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiAnimate;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    uiAnimate = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiAnimate
        )) {
        return false;
    }
    if (pActor) {
        ReferenceProcessLists.kUpdateLock.Lock(nullptr);
        bool bAttach3D = pActor->TESObjectREFR::Get3D() != nullptr;
        pActor->Resurrect(true, bAttach3D, uiAnimate == 1);
        ReferenceProcessLists.kUpdateLock.Unlock();
    }
    return true;
}

bool Script::GetFatiguePercentageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetFatiguePercentageConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetHealthPercentageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetHealthPercentageConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetBaseActorValueFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ActorValue::Index eActorValue = ActorValue::AGGRESSION;
    if (Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eActorValue
        )) {
        return Script::GetBaseActorValueConditionFunction(
            static_cast<Actor*>(apRefObject),
            eActorValue,
            0,
            &afReturnValue
        );
    }

    return false;
}

bool Script::GetCauseofDeathFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetCauseofDeathConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::FriendDispositionBoostFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject && apRefObject->IsActor()) {
        int32_t iDisposition = static_cast<Actor*>(apRefObject)->GetDisposition(pPlayer, nullptr);
        if (iDisposition < iDialogueDispositionFriendValue.Int()) {
            float fBoost = static_cast<float>(iDialogueDispositionFriendValue.Int() - iDisposition);
            static_cast<Actor*>(apRefObject)->ModifyDispositionTowardActor(pPlayer, fBoost);
        }
    }

    return true;
}

bool Script::GetGameSettingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cSettingName[512];
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cSettingName
        )) {
        return false;
    }
    GameSettingCollection* pCollection = SettingT<GameSettingCollection>::GetCollection();
    Setting* pSetting = pCollection->GetPtr(cSettingName);
    if (pSetting) {
        int32_t iType = pSetting->GetType();
        switch (iType) {
            case 3: {
                afReturnValue = static_cast<double>(pSetting->Int());
                if (Script::bSystemOutput) {
                    int32_t iValue = pSetting->Int();
                    Interface::PrintLine("GameSetting %s >> %i", pSetting->Key(), iValue);
                }
                break;
            }
            case 5: {
                afReturnValue = pSetting->Float();
                if (Script::bSystemOutput) {
                    float* pValue = &pSetting->Float();
                    Interface::PrintLine("GameSetting %s >> %.2f", pSetting->Key(), *pValue);
                }
                break;
            }
            case 6: {
                if (Script::bSystemOutput) {
                    const char* pString = pSetting->String();
                    Interface::PrintLine("GameSetting %s >> '%s'", pSetting->Key(), pString);
                }
                break;
            }
            default: {
                if (Script::bSystemOutput) {
                    Interface::PrintLine("GameSetting %s >> UNKNOWN TYPE", pSetting->Key());
                }
                break;
            }
        }
    }
    else if (Script::bSystemOutput) {
        Interface::PrintLine("GameSetting %s >> NOT FOUND", cSettingName, pSetting->String());
    }
    return true;
}

bool Script::SetGameSettingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cSettingName[512];
    char cValue[512];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cSettingName,
            cValue
        )) {
        return false;
    }
    GameSettingCollection* pCollection = SettingT<GameSettingCollection>::GetCollection();
    SettingT<GameSettingCollection>* pSetting = static_cast<SettingT<GameSettingCollection>*>(
        pCollection->GetPtr(cSettingName)
    );
    if (pSetting) {
        int32_t iType = pSetting->GetType();
        switch (iType) {
            case 3: {
                int32_t iValue = std::atoi(cValue);
                *pSetting = iValue;
                if (Script::bSystemOutput) {
                    iValue = pSetting->Int();
                    Interface::PrintLine("GameSetting %s >> %i", pSetting->Key(), iValue);
                }
                break;
            }
            case 5: {
                float fValue = std::atof(cValue);
                *pSetting = fValue;
                if (Script::bSystemOutput) {
                    float* pValue = &pSetting->Float();
                    Interface::PrintLine("GameSetting %s >> %.2f", pSetting->Key(), *pValue);
                }
                break;
            }
            case 6: {
                *pSetting = static_cast<const char*>(cValue);
                if (Script::bSystemOutput) {
                    const char* pString = pSetting->String();
                    Interface::PrintLine("GameSetting %s >> '%s'", pSetting->Key(), pString);
                }
                break;
            }
            default: {
                Interface::PrintLine("GameSetting %s >> UNKNOWN TYPE", cSettingName);
                break;
            }
        }
    }
    else {
        Interface::PrintLine("GameSetting %s >> NOT FOUND", cSettingName);
    }
    return true;
}

bool Script::GetINISettingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cSettingName[512];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cSettingName
        )) {
        return false;
    }
    INISettingCollection* pCollection = SettingT<INISettingCollection>::GetCollection();
    Setting* pSetting = pCollection->GetPtr(cSettingName);
    if (!pSetting) {
        pSetting = SettingT<INIPrefSettingCollection>::GetCollection()->GetPtr(cSettingName);
    }
    if (pSetting) {
        switch (pSetting->GetType()) {
            case 0: {
                Interface::PrintLine("INISetting %s >> %i", cSettingName, pSetting->Bool());
                break;
            }
            case 3: {
                Interface::PrintLine("INISetting %s >> %i", cSettingName, pSetting->Int());
                break;
            }
            case 5: {
                Interface::PrintLine("INISetting %s >> %.2f", cSettingName, pSetting->Float());
                break;
            }
            case 6: {
                Interface::PrintLine("INISetting %s >> '%s'", cSettingName, pSetting->Int());
                break;
            }
            default: {
                Interface::PrintLine("INISetting %s >> UNKNOWN TYPE", cSettingName);
                break;
            }
        }
    }
    else {
        Interface::PrintLine("INISetting %s >> NOT FOUND", cSettingName);
    }
    return true;
}

bool Script::SetINISettingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cSettingName[516];
    char cValue[516];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cSettingName,
            cValue
        )) {
        return false;
    }
    INISettingCollection* pCollection = SettingT<INISettingCollection>::GetCollection();
    SettingT<GameSettingCollection>* pSetting = static_cast<SettingT<GameSettingCollection>*>(
        pCollection->GetPtr(cSettingName)
    );
    if (!pSetting) {
        INIPrefSettingCollection* pPrefCollection = SettingT<INIPrefSettingCollection>::GetCollection();
        pSetting = static_cast<SettingT<GameSettingCollection>*>(pPrefCollection->GetPtr(cSettingName));
    }
    if (pSetting) {
        switch (pSetting->GetType()) {
            case 0: {
                int32_t iValue = std::atoi(cValue);
                *pSetting = iValue;
                if (Script::bSystemOutput) {
                    Interface::PrintLine("INISetting %s >> %i", cSettingName, pSetting->Bool());
                }
                break;
            }
            case 3: {
                int32_t iValue = std::atoi(cValue);
                *pSetting = iValue;
                if (Script::bSystemOutput) {
                    Interface::PrintLine("INISetting %s >> %i", cSettingName, pSetting->Int());
                }
                break;
            }
            case 5: {
                float fValue = std::atof(cValue);
                *pSetting = fValue;
                if (Script::bSystemOutput) {
                    Interface::PrintLine("INISetting %s >> %.2f", cSettingName, pSetting->Float());
                }
                break;
            }
            case 6: {
                *pSetting = static_cast<const char*>(cValue);
                if (Script::bSystemOutput) {
                    Interface::PrintLine("INISetting %s >> '%s'", cSettingName, pSetting->Int());
                }
                break;
            }
            default: {
                Interface::PrintLine("INISetting %s >> UNKNOWN TYPE", cSettingName);
                break;
            }
        }
    }
    else {
        Interface::PrintLine("INISetting %s >> NOT FOUND", cSettingName);
    }
    return true;
}

bool Script::FadeSFXFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fSeconds = 1.0f;
    if (Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fSeconds
        )) {
        uint32_t uiMilliseconds = 1000 * static_cast<int64_t>(fSeconds);
        BSAudio::QInstance().MuteType(
            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER |
                BSGameSound::SOUND_FLAG_IS_2D_RADIUS | BSGameSound::SOUND_FLAG_BEAM_EMITTER,
            uiMilliseconds,
            0x1770u
        );
    }
    return true;
}

bool Script::SetLocationSpecificLoadScreensOnlyFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiOnly = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiOnly
        )) {
        return false;
    }
    if (uiOnly) {
        SetIsLocationSpecificLoadScreensOnly(true);
    }
    else {
        SetIsLocationSpecificLoadScreensOnly(false);
    }
    return true;
}

bool SetIsLocationSpecificLoadScreensOnly(bool abOnly) {
    LoadingMenu::bLocationSpecificLoadScreensOnly = abOnly;
    return abOnly;
}

bool Script::ShowQuestsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BSSimpleList<TESQuest*>* pNode = pDataHandler->GetQuestList();
    while (pNode && pNode->m_item) {
        TESQuest* pQuest = pNode->m_item;
        pNode = pNode->m_pkNext;
        if (pQuest) {
            const char* pState = pQuest->GetEnabled() ? "On" : "Off";
            uint32_t uiStage = pQuest->GetCurrentStage();
            Interface::PrintLine("%s (%s) -- Stage %d", pQuest->GetEditorName(), pState, uiStage);
        }
    }
    return true;
}

bool Script::Show1stPersonFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer->Is3rdPerson()) {
        if (pPlayer->Get3D(false)->GetAppCulled()) {
            pPlayer->Force3rdPerson(false);
            if (Script::bSystemOutput) {
                Interface::PrintLine("Normal 1st person mode.");
            }
        }
        else if (pPlayer->Get3D(true)->GetAppCulled()) {
            pPlayer->Get3D(true)->SetAppCulled(false);
            if (Script::bSystemOutput) {
                Interface::PrintLine("Showing 1st and 3rd person models.");
            }
        }
        else {
            pPlayer->Get3D(true)->SetAppCulled(true);
            if (Script::bSystemOutput) {
                Interface::PrintLine("Normal 3rd person mode.");
            }
        }
    }
    else {
        pPlayer->Force3rdPerson(true);
        if (Script::bSystemOutput) {
            Interface::PrintLine("Showing 1st person model in 3rd person camera.");
        }
    }
    return true;
}

bool Script::LoadGame(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iOutputStats = 0;
    char cFileName[516];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cFileName,
            &iOutputStats
        )) {
        return false;
    }
    pSaveLoadManager->LoadGame(cFileName, -1, iOutputStats != 0, true);
    return true;
}

bool Script::Autosave(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (bAllowScriptedAutosave.Bool()) {
        pSaveLoadManager->SetAutoSaveTimerTo1();
    }
    return true;
}

bool Script::ForceSave(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (bAllowScriptedForceSave.Bool() && pSaveLoadManager->CanSave(-1)) {
        pSaveLoadManager->SetForceSaveTimer();
    }
    return true;
}

bool Script::ToggleCellNode(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectCELL::CELLNODE eNode = TESObjectCELL::CN_NONE;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eNode
        )) {
        return false;
    }
    if (eNode != TESObjectCELL::CN_NONE) {
        const char* pNodeName = nullptr;
        switch (eNode) {
            case TESObjectCELL::CN_ACTOR: {
                pNodeName = "Actor";
                break;
            }
            case TESObjectCELL::CN_MARKER: {
                pNodeName = "Marker";
                break;
            }
            case TESObjectCELL::CN_LAND: {
                pNodeName = "Land";
                break;
            }
            case TESObjectCELL::CN_STATIC: {
                pNodeName = "Static";
                break;
            }
            case TESObjectCELL::CN_DYNAMIC: {
                pNodeName = "Dynamic";
                break;
            }
            case TESObjectCELL::CN_OCCLUSION_PLANE: {
                pNodeName = "MultiBound";
                break;
            }
            case TESObjectCELL::CN_PORTAL: {
                pNodeName = "Water";
                break;
            }
            default: {
                break;
            }
        }
        if (pNodeName) {
            // The node index is the same value as a cull bit (CN_x == CCN_x for 0-6) and ShowCellNode's int; the value
            // is passed unchanged to both.
            bool bCulled = TESObjectCELL::IsNodeCulled(static_cast<TESObjectCELL::CELL_CULL_NODE>(eNode));
            TES::pInstance->ShowCellNode(static_cast<int32_t>(eNode), bCulled, false);
            if (eNode == TESObjectCELL::CN_PORTAL && TES::pInstance->GetWaterSystem()) {
                TES::pInstance->GetWaterSystem()->DisplayWater(bCulled, false, false);
            }

            const char* pState = bCulled ? "DISPLAYED" : "CULLED";
            Interface::PrintLine("Cell %s nodes now %s.", pNodeName, pState);
        }
        else {
            Interface::PrintLine("Invalid node index. (0-6).");
        }
    }
    return true;
}

bool Script::DumpTexturePalette(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cFileName[512];
    cFileName[0] = 0;
    if (apParameters &&
        !Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cFileName
        )) {
        return false;
    }
    if (!cFileName[0]) {
        TESObjectCELL* pInterior = TES::pInstance->pInteriorCell;
        if (pInterior) {
            BSsprintf(cFileName, 512, "%s-INT-%s.txt", "TexDump", pInterior->GetFormEditorID());
        }
        else {
            TESWorldSpace* pWorldSpace = TES::pInstance->pWorldSpace;
            if (pWorldSpace) {
                int32_t iGridY = TES::pInstance->iCurrentGridY;
                BSsprintf(
                    cFileName,
                    512,
                    "%s-%s.%i.%i.txt",
                    "TexDump",
                    pWorldSpace->GetFormEditorID(),
                    TES::pInstance->iCurrentGridX,
                    iGridY
                );
            }
            else {
                BSsprintf(cFileName, 512, "%s-UNKNOWN.txt", "TexDump");
            }
        }
    }
    if (!cFileName[0]) {
        ourStrcpy(cFileName, "D:\\TexDump.txt");
    }
    MessageHandler::IncDisableWarningCount(true);
    Warning("TEXTURES: <<< DUMPTEXTUREPALETTE results");
    BSFile kFile(cFileName, NiFile::WRITE_ONLY, 0x4000u, false);
    kFile.BSFile::Open(false, false);
    NiPointer<NiTexture> aspTextures[2048];
    ourMemset(aspTextures, 0, sizeof(aspTextures));
    uint32_t uiTotalSize = 0;
    int32_t iTextureCount = 0;
    // The textures with renderer data, sorted by size, largest first.
    for (NiTexture* pTexture = NiTexture::GetListHead(); pTexture; pTexture = pTexture->m_pkNext) {
        ++iTextureCount;
        if (pTexture->m_pkRendererData) {
            uint32_t uiSize = GetTextureDumpSize(pTexture);
            uiTotalSize += uiSize;
            int32_t iSlot = -1;
            for (int32_t i = 0; i < 2048 && iSlot == -1; ++i) {
                if (aspTextures[i]) {
                    uint32_t uiSlotSize = GetTextureDumpSize(aspTextures[i]);
                    if (uiSlotSize < uiSize) {
                        iSlot = i;
                    }
                }
                else {
                    iSlot = i;
                }
            }
            if (iSlot != -1) {
                for (int32_t j = 2047; j > iSlot; --j) {
                    aspTextures[j] = aspTextures[j - 1];
                }
                aspTextures[iSlot] = pTexture;
            }
        }
    }
    Warning("TEXTURES: Total Textures = %d @ %.2f Mb", iTextureCount, static_cast<double>(uiTotalSize) / 1048576.0);
    for (int32_t k = 0; k < 2048; ++k) {
        if (aspTextures[k]) {
            char cName[264];
            char cLine[512];
            char cRendered = 0;
            if (NiObject::IsKindOf_0(&NiSourceTexture::ms_RTTI, aspTextures[k])) {
                NiSourceTexture* pSourceTexture = static_cast<NiSourceTexture*>(aspTextures[k].m_pObject);
                const char* pName = pSourceTexture->GetFilename().GetHandleString();
                if (!pName || !*pName) {
                    pName = aspTextures[k]->GetName().GetHandleString();
                }
                if (pName && *pName) {
                    BSstrcpy(cName, 260, pName);
                }
                else {
                    BSstrcpy(cName, 260, "UNNAMED SOURCE TEXTURE");
                }
            }
            else {
                cRendered = 1;
                const char* pName = aspTextures[k]->GetName().GetHandleString();
                if (pName && *pName) {
                    BSsprintf(cName, 260, "RENDERED_TEXTURE (%s)", pName);
                }
                else {
                    BSsprintf(cName, 260, "RENDERED_TEXTURE (none)");
                }
            }
            BSstrlwr(cName, 260);
            const char* pCategory = "SYSTEM";
            if (!cRendered) {
                if (std::strstr(cName, "\\armor\\") ||
                    std::strstr(cName, "\\creatures\\") ||
                    std::strstr(cName, "\\characters\\") ||
                    std::strstr(cName, "\\pipboy3000\\") ||
                    std::strstr(cName, "\\weapons\\") ||
                    std::strstr(cName, "\\decals\\") ||
                    std::strstr(cName, "\\projectiles\\")) {
                    pCategory = "ACTOR";
                }
                else if (std::strstr(cName, "\\fonts\\") || std::strstr(cName, "\\interface\\")) {
                    pCategory = "MENUS";
                }
                else if (std::strstr(cName, "\\effects\\") || std::strstr(cName, "\\gore\\")) {
                    pCategory = "EFFECTS";
                }
                else if (std::strstr(cName, "\\sky\\")) {
                    pCategory = "SKY";
                }
                else if (std::strstr(cName, "\\lod\\")) {
                    pCategory = "LOD";
                }
                else if (std::strstr(cName, "\\water\\")) {
                    pCategory = "WATER";
                }
                else {
                    pCategory = "MISCREF";
                }
            }
            uint32_t uiSize = GetTextureDumpSize(aspTextures[k]);
            const NiPixelFormat* pFormat = &aspTextures[k]->m_pkRendererData->m_kPixelFormat;
            _D3DFORMAT eFormat = NiDX9PixelFormat::DetermineD3DFormat(*pFormat);
            const char* pFormatString = NiDX9SystemDesc::GetD3D9FormatString(eFormat);
            const char* pcClassName = aspTextures[k]->GetRTTI()->GetName();
            const char* pQuality = aspTextures[k]->QState() == PS_DEGRADED ? "L" : "H";
            NiTexture* pTexture = aspTextures[k];
            int32_t iRefCount = aspTextures[k]->GetRefCount() - 1;
            uint32_t uiSizeKB = uiSize >> 10;
            uint32_t uiHeight = pTexture->GetHeight();
            uint32_t uiWidth = pTexture->GetWidth();
            BSsprintf(
                cLine,
                512,
                "TEXTURES: %d:\t%s\t%s\t%dx%d\t%d\t%s\trefcount %d\t%s\t%s\r\n",
                k + 1,
                pCategory,
                cName,
                uiWidth,
                uiHeight,
                uiSizeKB,
                pFormatString,
                iRefCount,
                pQuality,
                pcClassName
            );
            Warning(cLine);
            kFile.BSFile::WriteF(cLine, std::strlen(cLine) + 1);
        }
    }
    kFile.Close();
    Warning("TEXTURES: >>> DUMPTEXTUREPALETTE results");
    MessageHandler::IncDisableWarningCount(false);
    return true;
}

bool Script::GetItemCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pItem = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pItem
           ) &&
        Script::GetItemCountConditionFunction(apRefObject, pItem, nullptr, afReturnValue);
}

bool Script::SetShowQuestItemsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iShow = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iShow
        )) {
        return false;
    }
    pPlayer->bShowQuestItemsInInventory = iShow > 0;
    return true;
}

bool Script::ResetHealthFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject && apRefObject->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        float fHealth = pActor->GetActorFloatValue(ActorValue::HEALTH);
        float fPermanentHealth = pActor->GetPermanentActorFloatValue(ActorValue::HEALTH);
        float fAmount = fPermanentHealth - fHealth;
        pActor->RestoreActorValue(ActorValue::HEALTH, fAmount);
        pActor->RestoreActorValue(ActorValue::LEFT_MOBILITY_CONDITION, 999.0f);
        pActor->RestoreActorValue(ActorValue::RIGHT_MOBILITY_CONDITION, 999.0f);
        pActor->RestoreActorValue(ActorValue::LEFT_ATTACK_CONDITION, 999.0f);
        pActor->RestoreActorValue(ActorValue::RIGHT_ATTACK_CONDITION, 999.0f);
        pActor->RestoreActorValue(ActorValue::PERCEPTION_CONDITION, 999.0f);
        pActor->RestoreActorValue(ActorValue::ENDURANCE_CONDITION, 999.0f);
        pActor->RestoreActorValue(ActorValue::BRAIN_CONDITION, 999.0f);
    }

    return true;
}

bool Script::GetEquippedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pItem = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pItem
           ) &&
        Script::GetEquippedConditionFunction(apRefObject, pItem, nullptr, afReturnValue);
}

bool Script::GetInWorldspace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pWorldSpace = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pWorldSpace
           ) &&
        Script::GetInWorldspaceConditionFunction(apRefObject, pWorldSpace, nullptr, afReturnValue);
}

bool Script::SetCellPublicFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iPublic = 0;
    TESObjectCELL* pCell = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pCell,
            &iPublic
        )) {
        return false;
    }
    if (pCell) {
        pCell->SetPublic(iPublic > 0);
    }
    return true;
}

bool Script::GetInSameCell(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pRef = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pRef
           ) &&
        Script::GetInSameCellConditionFunction(apRefObject, pRef, nullptr, afReturnValue);
}

bool Script::GetIsClass(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pClass = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pClass
           ) &&
        Script::GetIsClassConditionFunction(apRefObject, pClass, nullptr, afReturnValue);
}

bool Script::SetChargenValue(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiInCharGen = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiInCharGen
        )) {
        return false;
    }
    if (uiInCharGen) {
        pPlayer->SetInCharGen(true);
    }
    else {
        pPlayer->SetInCharGen(false);
    }
    return true;
}

bool Cmd_GetInCharGen(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    if (pPlayer) {
        afReturnValue = pPlayer->GetInChargen();
        if (Script::bSystemOutput) {
            Interface::PrintLine("GetChargenValue >> %i", static_cast<int32_t>(afReturnValue));
        }
    }
    return true;
}

bool Script::GetIsRace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pRace = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pRace
           ) &&
        Script::GetIsRaceConditionFunction(apRefObject, pRace, nullptr, afReturnValue);
}

bool Script::GetIsCreatureType(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pType = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pType
           ) &&
        Script::GetIsCreatureTypeConditionFunction(apRefObject, pType, nullptr, afReturnValue);
}

bool Script::SayToFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iUseSubtitles = 0;
    int32_t iNoTargetLook = 0;
    Actor* pTarget = nullptr;
    TESForm* pTopic = nullptr;
    afReturnValue = 0.0;
    bool bForceSubtitles = false;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTarget,
            &pTopic,
            &iUseSubtitles,
            &iNoTargetLook
        )) {
        return false;
    }
    if (iUseSubtitles > 0) {
        bForceSubtitles = true;
    }
    int32_t iLipDistance = iSpeakSoundLipDistance.Int();
    iSpeakSoundLipDistance = std::numeric_limits<int32_t>::max();
    Actor* pSpeaker = nullptr;
    if (apRefObject->IsActor()) {
        pSpeaker = static_cast<Actor*>(apRefObject);
    }
    if (pSpeaker && pTarget) {
        pSpeaker->StopCurrentDialogue();
        if (pSpeaker->pCurrentProcess && !pSpeaker->IsDead(true) && pTarget->pCurrentProcess) {
            pSpeaker->pCurrentProcess->SetWaitingforLipFile(false);
            pSpeaker->pCurrentProcess->ClearGreetingInfoData();
            if (pSpeaker->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
                if (!iNoTargetLook) {
                    pTarget->pCurrentProcess->ClearScriptHeadTrackTarget(false);
                    pTarget->pCurrentProcess->SetActionHeadTrackTarget(pSpeaker);
                    pTarget->SetDialogueItemTarget(pSpeaker);
                }
                pSpeaker->pCurrentProcess->ClearScriptHeadTrackTarget(false);
                pSpeaker->pCurrentProcess->SetActionHeadTrackTarget(pTarget);
            }
            if (pPlayer->IsSleepingorResting() ||
                pSpeaker->pCurrentProcess->eLevel ||
                (pSpeaker->GetEssential() && pSpeaker->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN)) {
                DialogueItem* pItem = static_cast<TESTopic*>(pTopic)
                                          ->TESTopic::CreateDialogueItem(pSpeaker, pTarget, nullptr, nullptr, true);
                if (pItem) {
                    pItem->FirstResponse();
                    pItem->GetCurrentResponse();
                    pItem->RunResult(TIRS_BEGIN);
                    pItem->RunResult(TIRS_END);
                    Script::SetActionFlag(pTopic, &pSpeaker->m_Extra, SCRIPT_ACTION_ONSAYTODONE);
                    delete pItem;
                }
            }
            else {
                pSpeaker->SetDialogueItemTarget(pTarget);
                pSpeaker->pCurrentProcess
                    ->ProcessGreet(pSpeaker, static_cast<TESTopic*>(pTopic), bForceSubtitles, false, true, true);
            }
        }
    }
    else if (
        apRefObject &&
        (apRefObject->Get3D() ||
         (apRefObject->IsMobileObject() && static_cast<MobileObject*>(apRefObject)->GetTalkingActivator()->Get3D()))
    ) {
        apRefObject->SetSayToTopic(static_cast<TESTopic*>(pTopic));
        DialogueItem* pItem = static_cast<TESTopic*>(pTopic)
                                  ->TESTopic::CreateDialogueItem(apRefObject, nullptr, nullptr, nullptr, false);
        if (pItem) {
            apRefObject->SetSayToTopicInfo(pItem->pInfo);
        }
        BSSoundHandle kVoice = apRefObject->VoiceSoundFunction(
            static_cast<TESTopic*>(pTopic),
            pTarget,
            false,
            false,
            false,
            false,
            bForceSubtitles
        );
        // The callback's context is untyped: the speaker's form ID is passed in it.
        kVoice.SetCompletionCallback(
            &TESObjectREFR::SayToCallBack,
            reinterpret_cast<void*>(static_cast<uintptr_t>(apRefObject->iFormID))
        );
    }
    iSpeakSoundLipDistance = iLipDistance;
    return true;
}

bool Script::ResetAIFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && !pActor->GetDisabled() && !pActor->GetDelete() && !pActor->IsPc()) {
        pActor->EvaluatePackage(false, true);
    }

    return true;
}

bool Script::EvaluatePackageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && !pActor->GetDisabled() && !pActor->GetDelete() && !pActor->IsPc()) {
        pActor->EvaluatePackage(false, false);
    }

    return true;
}

bool Script::AddTopicFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESTopic* pTopic = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTopic
        )) {
        return false;
    }
    pPlayer->AddTopic(pTopic, true, false);
    return true;
}

bool Script::SetAlertFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iAlert = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iAlert
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor) {
            pActor->SetAlert(iAlert > 0);
        }
    }
    return true;
}

bool Script::GetAlertFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetAlertConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetPlantedExplosiveFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetPlantedExplosiveConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::CompleteAllObjectivesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* pQuest = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest
        )) {
        return false;
    }
    pQuest->SetAllObjectivesComplete(1);
    return true;
}

bool Script::LookFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pTarget = nullptr;
    int32_t iPathLook = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTarget,
            &iPathLook
        )) {
        return false;
    }
    if (apRefObject && pTarget) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor && pActor->pCurrentProcess) {
            if (iPathLook) {
                NiPoint3 kTargetPos = pTarget->GetLocationOnReference();
                pActor->SetLookAtTarget(kTargetPos);
                pActor->pCurrentProcess->SetPathLookAtTarget(pTarget);
                pTarget->SetTargeted(true);
            }
            else {
                pActor->pCurrentProcess->SetScriptHeadTrackTarget(pTarget);
                pActor->m_Extra.SetHeadTrackTargetExtra(pTarget);
            }
            pActor->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        }
    }

    return true;
}

bool Script::StopLookFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor) {
            pActor->TESObjectREFR::GetExtra()->SetHeadTrackTargetExtra(nullptr);
            if (pActor->MobileObject::GetCurrentAIProcess()) {
                pActor->MobileObject::GetCurrentAIProcess()->ClearScriptHeadTrackTarget(true);
                pActor->ClearLookAtTarget();
                pActor->MobileObject::GetCurrentAIProcess()->SetPathLookAtTarget(nullptr);
            }
        }
    }
    return true;
}

bool Script::SetSkyParamFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fSunlightDimmer = 0.0f;
    float fLumRamp = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fSunlightDimmer,
            &fLumRamp
        )) {
        return false;
    }
    BSShaderManager::fSunlightDimmer = std::fabs(fSunlightDimmer);
    SkyShader::pLUMRamp[0] = std::fabs(fLumRamp);
    return true;
}

char Script::OnAddFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONADD, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnSellFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONSELL, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnDropFunction(
    SCRIPT_PARAMETER* paramInfo,
    void* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            static_cast<const char*>(scriptData),
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONDROP, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnActivateFunction(
    SCRIPT_PARAMETER* paramInfo,
    void* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    void* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm;
    *afReturnValue = 0.0;
    pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            static_cast<const char*>(scriptData),
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            static_cast<ScriptLocals*>(eventList),
            &pForm
        )) {
        return 0;
    }
    if (thisObj) {
        if (thisObj->GetAction(OBJECT_ACTION_ACTIVATE)) {
            *afReturnValue = 1.0;
        }
        thisObj->ClearAction(OBJECT_ACTION_USE_DEFAULT);
        thisObj->ClearAction(OBJECT_ACTION_ACTIVATE);
    }
    return 1;
}

char Script::OnNPCActivateFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    *afReturnValue = 0.0;
    Actor* pActor = nullptr;
    // GetActorCause is called through the vtable each time rather than cached.
    if (thisObj->GetActorCause() && thisObj->GetActorCause()->IsValid()) {
        pActor = thisObj->GetActorCause()->pActor;
    }

    if (thisObj && !pActor) {
        *afReturnValue = 1.0;
    }

    return 1;
}

char Script::OnFireFunction(
    int32_t aiParameters,
    int32_t aiCompiledParams,
    TESObjectREFR* apRefObject,
    int32_t aiContainer,
    int32_t aiScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    *afReturnValue = 0.0;
    Script::CheckFlagFunction(SCRIPT_ACTION_ONFIRE, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Script::OnOpenFunction(
    int32_t aiParameters,
    int32_t aiCompiledParams,
    TESObjectREFR* apRefObject,
    int32_t aiContainer,
    int32_t aiScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    *afReturnValue = 0.0;
    Script::CheckFlagFunction(SCRIPT_ACTION_ONOPEN, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Script::OnCloseFunction(
    int32_t aiParameters,
    int32_t aiCompiledParams,
    TESObjectREFR* apRefObject,
    int32_t aiContainer,
    int32_t aiScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    *afReturnValue = 0.0;
    Script::CheckFlagFunction(SCRIPT_ACTION_ONCLOSE, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Script::OnGrabFunction(
    int32_t aiParameters,
    int32_t aiCompiledParams,
    TESObjectREFR* apRefObject,
    int32_t aiContainer,
    int32_t aiScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    *afReturnValue = 0.0;
    Script::CheckFlagFunction(SCRIPT_ACTION_ONGRAB, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Script::OnReleaseFunction(
    int32_t aiParameters,
    int32_t aiCompiledParams,
    TESObjectREFR* apRefObject,
    int32_t aiContainer,
    int32_t aiScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    *afReturnValue = 0.0;
    Script::CheckFlagFunction(SCRIPT_ACTION_ONRELEASE, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Script::OnDestructionStageChangeFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    // The stage parameter, 0x80000000 when the script gives none.
    uint32_t uiStage;
    *afReturnValue = 0.0;
    uiStage = 0x80000000u;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &uiStage
        )) {
        return 0;
    }
    // 0x200000 is the OnDestructionStageChange action flag. The result is otherwise unused.
    Script::CheckFlagFunction(SCRIPT_ACTION_ONDESTRUCTIONSTAGECHANGE, thisObj, nullptr, eventList, *afReturnValue);
    return 1;
}

char Script::OnHitFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONHIT, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnHitWithFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    if (pForm && pForm->GetFormType() == FLST_ID) {
        // A form list: each bound object in it is tested until one sets the result.
        BSSimpleList<TESForm*>* pIter = static_cast<BGSListForm*>(pForm)->GetFormList();
        do {
            if (!pIter) {
                break;
            }
            if (pIter->IsEmpty()) {
                break;
            }
            TESForm* pItem = pIter->GetItem();
            pIter = pIter->GetNext();
            TESForm* pActionForm = nullptr;
            if (pItem->IsBoundObject()) {
                pActionForm = pItem;
            }
            Script::CheckFlagFunction(SCRIPT_ACTION_ONHITWITH, thisObj, pActionForm, eventList, *afReturnValue);
        }
        while (0.0 == *afReturnValue);
    }
    else {
        Script::CheckFlagFunction(SCRIPT_ACTION_ONHITWITH, thisObj, pForm, eventList, *afReturnValue);
    }
    return 1;
}

char Script::OnDeathFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONDEATH, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnStartCombatFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONSTARTCOMBAT, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnMurderFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONMURDER, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnCombatEndFunction(
    int32_t aiParameters,
    int32_t aiCompiledParams,
    TESObjectREFR* apRefObject,
    int32_t aiContainer,
    int32_t aiScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    Script::CheckFlagFunction(SCRIPT_ACTION_ONCOMBATEND, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

// Ignores ParseParameters' result.
char Script::OnSayToDoneFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    Script::ParseParameters(
        paramInfo,
        scriptData,
        *opcodeOffsetPtr,
        thisObj,
        containingObj,
        scriptObj,
        eventList,
        &pForm
    );
    // 0x40000 is the SayToDone action flag.
    Script::CheckFlagFunction(SCRIPT_ACTION_ONSAYTODONE, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnPackageStartFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONPACKAGESTART, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnPackageEndFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONPACKAGEEND, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnPackageChangeFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONPACKAGECHANGE, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnLoadFunction(
    int32_t aiParameters,
    int32_t aiCompiledParams,
    TESObjectREFR* apRefObject,
    int32_t aiContainer,
    int32_t aiScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    Script::CheckFlagFunction(SCRIPT_ACTION_ONLOAD, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Script::OnResetFunction(
    int32_t aiParameters,
    int32_t aiCompiledParams,
    TESObjectREFR* apRefObject,
    int32_t aiContainer,
    int32_t aiScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    Script::CheckFlagFunction(SCRIPT_ACTION_ONRESET, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Script::OnMagicEffectHitFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONMAGICEFFECTHIT, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnTriggerFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONTRIGGER, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnTriggerEnterFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONTRIGGERENTER, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnTriggerLeaveFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONTRIGGERLEAVE, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

bool Script::EnablePlayerControlsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiFlags = 0;
    int32_t iMovement = 1;
    int32_t iPipboy = 1;
    int32_t iFighting = 1;
    int32_t iPOVSwitch = 1;
    int32_t iLooking = 1;
    int32_t iRollOver = 1;
    int32_t iSneaking = 1;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iMovement,
            &iPipboy,
            &iFighting,
            &iPOVSwitch,
            &iLooking,
            &iRollOver,
            &iSneaking
        )) {
        return false;
    }
    if (iMovement) {
        uiFlags |= PlayerCharacter::DPC_MOVEMENT;
    }
    if (iPipboy) {
        uiFlags |= PlayerCharacter::DPC_PIPBOY;
    }
    if (iFighting) {
        uiFlags |= PlayerCharacter::DPC_FIGHTING;
    }
    if (iPOVSwitch) {
        uiFlags |= PlayerCharacter::DPC_POV_SWITCH;
    }
    if (iLooking) {
        uiFlags |= PlayerCharacter::DPC_LOOKING;
    }
    if (iRollOver) {
        uiFlags |= PlayerCharacter::DPC_ROLL_OVER;
    }
    if (iSneaking) {
        uiFlags |= PlayerCharacter::DPC_SNEAKING;
    }
    pPlayer->SetControlsDisabled(false, static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags));
    return true;
}

bool Script::DisablePlayerControlsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iSneaking = 0;
    int32_t iPOVSwitch = 0;
    int32_t iPipboy = 1;
    uint32_t uiMovement = 1;
    int32_t iLooking = 0;
    int32_t iRollOver = 0;
    int32_t iFighting = 1;
    PlayerCharacter::DISABLE_PLAYER_CONTROLS uiFlags = PlayerCharacter::DPC_NONE;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiMovement,
            &iPipboy,
            &iFighting,
            &iPOVSwitch,
            &iLooking,
            &iRollOver,
            &iSneaking
        )) {
        return false;
    }
    if (uiMovement) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_MOVEMENT);
    }
    if (iPipboy) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_PIPBOY);
    }
    if (iFighting) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_FIGHTING);
    }
    if (iPOVSwitch) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_POV_SWITCH);
    }
    if (iLooking) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_LOOKING);
    }
    if (iRollOver) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_ROLL_OVER);
    }
    if (iSneaking) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_SNEAKING);
    }
    pPlayer->SetControlsDisabled(true, uiFlags);
    return true;
}

bool Script::GetPlayerControlsDisabledFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iSneaking = 1;
    int32_t iPOVSwitch = 1;
    int32_t iPipboy = 1;
    uint32_t uiFlags = 0;
    uint32_t uiMovement = 1;
    int32_t iLooking = 1;
    int32_t iRollOver = 1;
    int32_t iFighting = 1;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiMovement,
            &iPipboy,
            &iFighting,
            &iPOVSwitch,
            &iLooking,
            &iRollOver,
            &iSneaking
        )) {
        return false;
    }
    if (uiMovement) {
        uiFlags |= PlayerCharacter::DPC_MOVEMENT;
    }
    if (iPipboy) {
        uiFlags |= PlayerCharacter::DPC_PIPBOY;
    }
    if (iFighting) {
        uiFlags |= PlayerCharacter::DPC_FIGHTING;
    }
    if (iPOVSwitch) {
        uiFlags |= PlayerCharacter::DPC_POV_SWITCH;
    }
    if (iLooking) {
        uiFlags |= PlayerCharacter::DPC_LOOKING;
    }
    if (iRollOver) {
        uiFlags |= PlayerCharacter::DPC_ROLL_OVER;
    }
    if (iSneaking) {
        uiFlags |= PlayerCharacter::DPC_SNEAKING;
    }
    // The flags go in the condition function's first parameter slot, where
    // Script::GetPlayerControlsDisabledConditionFunction reads them.
    Script::GetPlayerControlsDisabledConditionFunction(
        nullptr,
        reinterpret_cast<void*>(static_cast<uintptr_t>(uiFlags)),
        nullptr,
        afReturnValue
    );
    return true;
}

bool Script::PickIdleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && pActor->GetCurrentAIProcess()) {
        TESIdleForm* pIdleForm = pIdleManager->GetIdleToPlay(pActor, pActor->GetCurrentAIProcess()->GetTarget());
        if (pIdleForm) {
            Animation* pAnimation = pActor->GetAnimation();
            if (!pAnimation) {
                return true;
            }
            bool bPlaying = !pAnimation->SpecialIdleDonePlaying();
            if (pAnimation->spAnimIdleQueued.m_pObject && pAnimation->spAnimIdleQueued->pIdleForm == pIdleForm) {
                return true;
            }
            if (bPlaying &&
                pAnimation->spAnimIdle.m_pObject &&
                (pAnimation->spAnimIdle->eType != AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE ||
                 (pAnimation->spAnimIdle->GetSequence() &&
                  pAnimation->spAnimIdle->GetSequence()->m_eCycleType == NiControllerSequence::CYCLE_LOOP))) {
                bPlaying = false;
            }

            if (!bPlaying) {
                if (pActor->GetCurrentAIProcess()) {
                    pActor->GetCurrentAIProcess()->SetCurrentProcessIdle(pIdleForm);
                    pActor->GetCurrentAIProcess()->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_PICK_IDLE_CALL);
                }
                if (Script::bSystemOutput) {
                    const char* pModel = pIdleForm->GetModel();
                    int32_t iFormID = pIdleForm->iFormID;
                    Interface::PrintLine(
                        "Picked Idle '%s' (%08X) file: %s",
                        pIdleForm->GetFormEditorID(),
                        iFormID,
                        pModel
                    );
                }
            }
        }
    }

    return true;
}

bool Script::PlayIdleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cIdleName[512];
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cIdleName
        )) {
        return false;
    }
    if (pActor && pActor->pCurrentProcess) {
        TESForm* pForm = TESForm::GetFormByEditorID(cIdleName);
        TESIdleForm* pIdleForm = dynamic_cast<TESIdleForm*>(pForm);
        if (pIdleForm) {
            Animation* pAnimation = pActor->GetAnimation();
            if (!pAnimation) {
                return true;
            }
            bool bPlaying = !pAnimation->SpecialIdleDonePlaying();
            if (pAnimation->spAnimIdleQueued.m_pObject && pAnimation->spAnimIdleQueued->pIdleForm == pIdleForm) {
                return true;
            }
            if (bPlaying &&
                pAnimation->spAnimIdle.m_pObject &&
                (pAnimation->spAnimIdle->eType != AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE ||
                 (pAnimation->spAnimIdle->GetSequence() &&
                  pAnimation->spAnimIdle->GetSequence()->m_eCycleType == NiControllerSequence::CYCLE_LOOP))) {
                bPlaying = false;
            }

            if (!bPlaying) {
                if (pActor->pCurrentProcess) {
                    pActor->pCurrentProcess->SetCurrentProcessIdle(pIdleForm);
                    pActor->pCurrentProcess->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_PICK_IDLE_CALL);
                }
                if (Script::bSystemOutput) {
                    const char* pModel = pIdleForm->GetModel();
                    int32_t iFormID = pIdleForm->iFormID;
                    Interface::PrintLine(
                        "PlayIdle found '%s' (%08X) file: %s",
                        pIdleForm->GetFormEditorID(),
                        iFormID,
                        pModel
                    );
                }
            }
        }
    }

    return true;
}

bool Script::IsWeaponInListFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pList = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pList
           ) &&
        Script::IsWeaponInListConditionFunction(apRefObject, pList, nullptr, afReturnValue);
}

bool Script::IsFacingUpFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return !apRefObject || Script::IsFacingUpConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::IsLeftUpFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::IsLeftUpConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::GetKnockedStateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::GetKnockedStateConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::IsWeaponSkillTypeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pSkill = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pSkill
        )) {
        return false;
    }
    if (apRefObject) {
        return Script::IsWeaponSkillTypeConditionFunction(apRefObject, pSkill, nullptr, afReturnValue);
    }
    return true;
}

bool Script::GetCurrentAIPackageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::GetCurrentAIPackageConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::IsWaitingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::IsWaitingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::IsIdlePlayingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::IsIdlePlayingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::ShowQuestTargetsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer->GetActiveQuest()) {
        Interface::PrintLine("Active quest: %s", pPlayer->GetActiveQuest()->GetEditorName());
        // pIter holds the list head and then each next node.
        BSSimpleList<TESQuestTarget*>* pIter = pPlayer->GetCurrentTargetList();
        if (pIter) {
            Interface::PrintLine("%d current targets", pIter->ItemsInList());
        }
        else {
            Interface::PrintLine("No current targets");
        }
        int32_t iTarget = 1;
        while (pIter && pIter->GetItem()) {
            TESQuestTarget* pTarget = pIter->GetItem();
            pIter = pIter->GetNext();
            BSStringT<char> kRefText;
            BSStringT<char> kDoorText;
            kDoorText.Set("Same cell/exterior", 0);
            TESObjectREFR* pRef = pTarget->GetReference(false);
            TESObjectREFR* pCarrier = pTarget->GetReference(true);
            if (pCarrier && pRef) {
                if (pCarrier == pRef) {
                    int32_t iFormID = pRef->iFormID;
                    kRefText.SPrintF("%s (%08X)", pRef->GetFormEditorID(), iFormID);
                }
                else {
                    int32_t iRefFormID = pRef->iFormID;
                    const char* pRefName = pRef->GetFormEditorID();
                    int32_t iCarrierFormID = pCarrier->iFormID;
                    kRefText.SPrintF(
                        "%s (%08X) (carrying %s (%08X))",
                        pCarrier->GetFormEditorID(),
                        iCarrierFormID,
                        pRefName,
                        iRefFormID
                    );
                }
            }

            TESObjectREFR* pDoorRef = pTarget->QTrackingRef();
            if (pDoorRef && pDoorRef->GetObjectReference()->GetFormType() == DOOR_ID) {
                int32_t iDoorFormID = pDoorRef->iFormID;
                kDoorText.SPrintF("%s (%08X)", pDoorRef->GetFormEditorID(), iDoorFormID);
            }

            Interface::PrintLine(
                "Target %d:  Reference: %s, load door: %s",
                iTarget++,
                kRefText.pString,
                kDoorText.pString
            );
        }
    }
    else {
        Interface::PrintLine("No active quest");
    }
    return true;
}

// The CRT takes the strings as unsigned char bytes: a signedness change of the same bytes.
char* mbsnbcpy_(char* Destination, const char* Source, uint32_t Count) {
    return reinterpret_cast<char*>(
        _mbsnbcpy(reinterpret_cast<uint8_t*>(Destination), reinterpret_cast<const uint8_t*>(Source), Count)
    );
}

bool Script::ShowFullQuestLogFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* pQuest = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest
        )) {
        return false;
    }
    BSSimpleList<TESQuestStageItem*> kQuestLog;
    TESQuest::GetQuestLog(&kQuestLog, pQuest, false, false, false);
    BSSimpleList<TESQuestStageItem*>* pIter = &kQuestLog;
    while (pIter && pIter->m_item) {
        char cLogEntry[260];
        TESQuestStageItem* pItem = pIter->m_item;
        TESQuest* pOwner = pItem->m_pOwner;
        pIter = pIter->m_pkNext;
        ourMemset(cLogEntry, 0, sizeof(cLogEntry));
        mbsnbcpy_(cLogEntry, pItem->GetLogEntry(pOwner), 259u);
        Date* pDate = pItem->m_pLogDate;
        Interface::PrintLine("------------------------------------------------");
        // Reads the year, then the month's name, then the day.
        uint16_t usYear = pDate->sYear;
        const char* pMonthName = pGSMonth[pDate->GetMonth()]->String();
        Interface::PrintLine("%d of %s, %d", pDate->GetDay(), pMonthName, usYear);
        Interface::PrintLine("%s", pItem->m_pOwner->TESFullName::GetFullName());
        Interface::PrintLine("%s", cLogEntry);
    }
    return true;
}

bool Script::SetPCFactionMurderFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction = nullptr;
    int32_t iMurder = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction,
            &iMurder
        )) {
        return false;
    }
    if (iMurder) {
        pFaction->SetPCMurder(true);
    }
    else {
        pFaction->SetPCMurder(false);
    }
    return true;
}

bool Script::SetPlayerEnemyofFactionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction = nullptr;
    int32_t iEnemy = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction,
            &iEnemy
        )) {
        return false;
    }
    if (iEnemy) {
        pFaction->SetPCAnEnemy(true);
    }
    else {
        pFaction->SetPCAnEnemy(false);
    }
    return true;
}

bool Script::SetPCFactionAttackFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction = nullptr;
    int32_t iAttack = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction,
            &iAttack
        )) {
        return false;
    }
    if (iAttack) {
        pFaction->SetPCAttack(true);
    }
    else {
        pFaction->SetPCAttack(false);
    }
    return true;
}

bool Script::GetSelfFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    uint32_t uiFormID = 0;
    if (apRefObject) {
        uint8_t ucFormType;
        if (apRefObject->GetRefPersists() ||
            (ucFormType = apRefObject->GetObjectReference()->GetFormType(),
             !TESContainer::ContainerCanHoldType(ucFormType))) {
            uiFormID = apRefObject->iFormID;
            Script::PutNumericIDInDouble(uiFormID, afReturnValue);
        }
    }
    if (Script::bSystemOutput) {
        Interface::PrintLine("GetSelf >> (%08x)", uiFormID);
    }
    return true;
}

bool Script::GetCombatTargetFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    uint32_t uiFormID = 0;
    if (pActor) {
        Actor* pTarget = pActor->GetCombatTarget();
        if (pTarget) {
            uiFormID = pTarget->iFormID;
            Script::PutNumericIDInDouble(uiFormID, afReturnValue);
        }
    }
    if (Script::bSystemOutput) {
        Interface::PrintLine("GetCombatTarget >> (%08x)", uiFormID);
    }
    return true;
}

bool Script::GetPackageTargetFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    uint32_t uiFormID = 0;
    if (pActor) {
        TESObjectREFR* pTarget = pActor->GetCurrentPackageTarget();
        if (pTarget) {
            uiFormID = pTarget->TESForm::GetFormID();
            Script::PutNumericIDInDouble(uiFormID, afReturnValue);
        }
    }
    if (Script::bSystemOutput) {
        Interface::PrintLine("GetPackageTarget >> (%08x)", uiFormID);
    }
    return true;
}

bool Script::GetContainerFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    uint32_t uiFormID = 0;
    if (apRefObject && apContainer) {
        uiFormID = apContainer->TESForm::GetFormID();
        Script::PutNumericIDInDouble(uiFormID, afReturnValue);
    }

    if (Script::bSystemOutput) {
        Interface::PrintLine("GetContainer >>(%08x)", uiFormID);
    }
    return true;
}

bool Script::GetParentRefFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    uint32_t uiFormID = 0;
    if (apRefObject && apRefObject->GetEnableStateParent()) {
        TESObjectREFR* pParent = apRefObject->GetEnableStateParent();
        uiFormID = pParent->GetFormID();
        Script::PutNumericIDInDouble(uiFormID, afReturnValue);
    }

    if (Script::bSystemOutput) {
        Interface::PrintLine("GetParentRef >> (%08x)", uiFormID);
    }
    return true;
}

bool Script::GetLinkedRefFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    uint32_t uiFormID = 0;
    if (apRefObject && apRefObject->GetLinkedRef()) {
        TESObjectREFR* pLinkedRef = apRefObject->GetLinkedRef();
        uiFormID = pLinkedRef->GetFormID();
        Script::PutNumericIDInDouble(uiFormID, afReturnValue);
    }

    if (Script::bSystemOutput) {
        Interface::PrintLine("GetLinkedRef >> (%08x)", uiFormID);
    }
    return true;
}

bool Script::GetForceRun(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && pActor->GetForceRun()) {
        afReturnValue = 1.0;
    }

    if (Script::bSystemOutput) {
        Interface::PrintLine("GetForceRun >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::SetForceRun(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        afReturnValue = 1.0;
        int32_t iForceRun = 0;
        if (!Script::ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &iForceRun
            )) {
            return false;
        }
        pActor->bForceRun = iForceRun != 0;
        if (Script::bSystemOutput) {
            Interface::PrintLine("SetForceRun >> %0.2f", iForceRun);
        }
    }
    return true;
}

bool Script::GetForceSneak(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && pActor->bForceSneak) {
        afReturnValue = 1.0;
    }

    if (Script::bSystemOutput) {
        Interface::PrintLine("GetForceSneak >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::SetForceSneak(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        afReturnValue = 1.0;
        int32_t iForceSneak = 0;
        if (!Script::ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &iForceSneak
            )) {
            return false;
        }
        if (iForceSneak) {
            pActor->SetForceSneak(true);
        }
        else {
            pActor->SetForceSneak(false);
        }
        if (Script::bSystemOutput) {
            Interface::PrintLine("SetForceSneak >> %0.2f", iForceSneak);
        }
    }
    return true;
}

bool Script::AdvanceLevelFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer) {
        uint16_t usLevel = pPlayer->GetLevel();
        pPlayer->GetActorBase()->SetLevel(usLevel + 1);
        Interface::CreateLevelUpMenu();
    }
    return true;
}

bool Script::HasMagicEffectFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pEffect = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pEffect
           ) &&
        Script::HasMagicEffectConditionFunction(apRefObject, pEffect, nullptr, afReturnValue);
}

bool Script::IsSpellTargetFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pSpell = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pSpell
           ) &&
        Script::IsSpellTargetConditionFunction(apRefObject, pSpell, nullptr, afReturnValue);
}

bool Script::GetSpellUsageNumberFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pSpell = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pSpell
           ) &&
        Script::GetSpellUsageNumberConditionFunction(apRefObject, pSpell, nullptr, afReturnValue);
}

bool Script::GetVATSModeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetVATSModeConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetVATSTargetHeightFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetVATSTargetHeightConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::SetDefaultOpenFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiOpen = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiOpen
        )) {
        return false;
    }
    if (apRefObject) {
        bool bWasOpenByDefault = apRefObject->GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
        if (uiOpen) {
            apRefObject->SetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
        }
        else {
            apRefObject->ClearAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
        }
        bool bOpenByDefault = apRefObject->GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
        apRefObject->m_Extra.RemoveSavedAnimation();
        apRefObject->m_Extra.RemoveLastFinishedSequence();
        if (bWasOpenByDefault != bOpenByDefault) {
            BGSChangeFlags kFlags(CHANGE_OBJECT_OPEN_DEFAULT_STATE);
            if (pSaveLoadGame->GetChange(apRefObject, kFlags)) {
                apRefObject->RemoveChange(CHANGE_OBJECT_OPEN_DEFAULT_STATE);
            }
            else {
                apRefObject->AddChange(CHANGE_OBJECT_OPEN_DEFAULT_STATE);
            }
        }
        BGSOpenCloseForm::SetOpenState(apRefObject, bOpenByDefault, true);
    }
    if (Script::bSystemOutput) {
        Interface::PrintLine("SetDefaultOpen >> %0.2f", uiOpen);
    }
    return true;
}

bool Script::ShowRaceMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer) {
        // The flag is tested as a raw byte: read as a bool, clang passes the byte xor 1.
        if (*reinterpret_cast<const uint8_t*>(&Script::bSystemOutput)) {
            Interface::CreateRaceSexMenu(RSM_CONTEXT_NONE);
        }
        else {
            Interface::CreateRaceSexMenu(RSM_CONTEXT_CHARGEN);
        }
    }
    return true;
}

bool Script::ShowBarberMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer) {
        Interface::CreateRaceSexMenu(RSM_CONTEXT_BARBER);
    }
    return true;
}

bool Script::ShowPlasticSurgeonMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer) {
        Interface::CreateRaceSexMenu(RSM_CONTEXT_PLASTIC_SURGEON);
    }
    return true;
}

bool Script::ShowNameMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer) {
        return Script::GetPlayerName(
            apParameters,
            apCompiledParams,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            afReturnValue,
            auiOffset
        );
    }

    return true;
}

bool Script::ShowBarterMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iMode = 0;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &iMode
    );
    if (apRefObject && apRefObject != pPlayer) {
        TESObjectREFR* pMobileRef = nullptr;
        TESObjectREFR* pRef = nullptr;
        if (apRefObject->IsMobileObject()) {
            pMobileRef = apRefObject;
        }
        else {
            BGSTalkingActivator* pTalkingActivator = dynamic_cast<BGSTalkingActivator*>(
                apRefObject->GetObjectReference()
            );
            pRef = pTalkingActivator ? pTalkingActivator->pTempRef : nullptr;
        }
        if (pMobileRef) {
            if (pMobileRef->IsActor()) {
                pRef = pMobileRef;
            }
            else {
                MobileObject* pMobile = static_cast<MobileObject*>(pMobileRef);
                if (pMobile->bTalkingActivator) {
                    // The talking activator's base object is read as a BGSTalkingActivator without a type check.
                    pRef = static_cast<BGSTalkingActivator*>(pMobile->pTalkingActivator->GetObjectReference())
                               ->pTempRef;
                }
            }
        }
        if (pRef) {
            Interface::CreateBarterMenu(pRef, iMode);
        }
    }

    return true;
}

bool Script::GetRestrainedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetRestrainedConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::ForceFleeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pLocationRef = nullptr;
    TESObjectCELL* pCell = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pCell,
            &pLocationRef
        )) {
        return false;
    }
    if (apRefObject == pPlayer) {
        return true;
    }
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor && pActor->GetCurrentAIProcess()) {
            CombatController* pCombatController = pActor->GetCombatController();
            if (pCombatController) {
                pCombatController->ForceFlee(pCombatController->pTarget, 0.0f);
                BaseProcess* pProcess = pActor->GetCurrentAIProcess();
                FleePackage* pFlee = dynamic_cast<FleePackage*>(pProcess->GetPackageThatIsRunning());
                if (pFlee) {
                    PackageLocation* pLocation = pFlee->pPackLoc;
                    if (pLocationRef) {
                        pLocation->SetLocReference(pLocationRef);
                        pFlee->SetEvaluatePoint(false);
                    }
                    else if (pCell) {
                        pLocation->SetLocCell(pCell);
                        pFlee->SetEvaluatePoint(false);
                    }
                }
            }
            else {
                pActor->InitiateFlee(nullptr, false, false, true, pCell, pLocationRef, -1.0f, -1.0f);
            }
        }
    }
    return true;
}

bool Script::ForceTakeCoverFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pTarget = nullptr;
    // The duration is an integer parameter.
    int32_t iDuration = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTarget,
            &iDuration
        )) {
        return false;
    }
    if (apRefObject == pPlayer) {
        return true;
    }
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor && pActor->GetCurrentAIProcess()) {
            CombatController* pCombatController = pActor->GetCombatController();
            if (pCombatController) {
                pCombatController->ForceTakeCover(pTarget, static_cast<float>(iDuration));
            }
        }
    }
    return true;
}

bool Script::GetOffersServiceNowFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetOffersServiceConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::ModBarterGoldFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iGold = 0;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor &&
        !Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iGold
        )) {
        return false;
    }
    if (Script::bSystemOutput) {
        Interface::PrintLine("Actor Barter Gold is  %d ", pActor->GetBarterGold());
    }
    return true;
}

// pNPC's barter gold is read for the line even when the reference's base is no NPC.
bool Script::SetBarterGoldFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iGold = 0;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        TESNPC* pNPC = dynamic_cast<TESNPC*>(pActor->GetObjectReference());
        if (pNPC) {
            if (!Script::ParseParameters(
                    apParameters,
                    apCompiledParams,
                    auiOffset,
                    apRefObject,
                    apContainer,
                    apScript,
                    apScriptLocals,
                    &iGold
                )) {
                return false;
            }
            pNPC->SetBarterGold(static_cast<uint16_t>(iGold));
        }
        if (Script::bSystemOutput) {
            Interface::PrintLine(" Actor's base barter gold is  %d ", pNPC->GetBarterGold());
        }
    }
    return true;
}

bool Script::GetBarterGoldFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetBarterGoldConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::SetLevelFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iLevel = 0;
    int32_t iMatchesPCLevel = 0;
    int32_t iCalcLevelMin = -1;
    int32_t iCalcLevelMax = -1;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        TESActorBase* pActorBase = pActor->GetActorBase();
        if (!Script::ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &iLevel,
                &iMatchesPCLevel,
                &iCalcLevelMin,
                &iCalcLevelMax
            )) {
            return false;
        }
        if (pActor->IsPc()) {
            CharacterProgression* pProgression = pPlayer->GetCharacterProgressionInfo();
            pProgression->ForceLevel(iLevel);
        }
        else {
            pActorBase->SetLevel(iLevel);
            if (iMatchesPCLevel) {
                pActorBase->SetMatchesPCLevel(true);
            }
            if (iCalcLevelMin != -1) {
                pActorBase->SetCalcLevelMin(iCalcLevelMin);
                pActorBase->SetMatchesPCLevel(true);
            }
            if (iCalcLevelMax != -1) {
                pActorBase->SetCalcLevelMax(iCalcLevelMax);
                pActorBase->SetMatchesPCLevel(true);
            }
            TESContainer* pContainer = pActor->HasContainer();
            InventoryChanges* pInvChanges = InventoryChanges::GetInventoryChanges(pActor);
            if (pContainer && pInvChanges) {
                pInvChanges->RemoveAllObjectsWorn(false, nullptr);
                pInvChanges->RemoveDefaultLeveledItems();
                pInvChanges->InitLeveledItems();
                TESNPC* pNPC = dynamic_cast<TESNPC*>(pActorBase);
                TESCreature* pCreature = dynamic_cast<TESCreature*>(pActorBase);
                if (pNPC) {
                    pNPC->InitDefaultWorn(pActor, true, true, false, true);
                }
                else if (pCreature) {
                    pCreature->InitDefaultWorn(pActor, true, true, true);
                }
            }

            if (pActor->IsNpc()) {
                TESNPC* pNPC = dynamic_cast<TESNPC*>(pActor->GetObjectReference());
                if (pNPC) {
                    pNPC->InitValues(false);
                }
            }
        }
    }
    return true;
}

bool Script::IsXBoxFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsXboxConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::IsPS3Function(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsPS3ConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::IsPCFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsPCConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::IsActorUsingATorchFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsActorUsingATorchConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::ModPCAttributeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ActorValue::Index eAttribute = ActorValue::AGGRESSION;
    int32_t iMod = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eAttribute,
            &iMod
        )) {
        return false;
    }
    if (ActorValue::IsSPECIAL(eAttribute)) {
        int32_t iBase = pPlayer->GetBaseActorValue(eAttribute);
        pPlayer->SetActorBaseValue(eAttribute, iMod + iBase);
    }
    return true;
}

bool Script::DamageWeaponFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fDamage = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fDamage
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (!pActor) {
        return false;
    }
    BaseProcess* pProcess = pActor->MobileObject::GetCurrentAIProcess();
    ItemChange* pWeapon = pProcess->GetCurrentWeapon();
    if (pWeapon) {
        pActor->DamageEquipment(pWeapon, fDamage, true);
    }
    return true;
}

bool Script::GetWeaponHealthFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bool bResult = true;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        BaseProcess* pProcess = pActor->MobileObject::GetCurrentAIProcess();
        ItemChange* pWeapon = pProcess->GetCurrentWeapon();
        if (pWeapon) {
            Interface::PrintLine("%.2f/100.0", pWeapon->GetItemHealth(false));
        }
        else {
            Interface::PrintLine("%s has no weapon equipped.", pActor->GetFullName());
        }
    }
    else {
        bResult = false;
    }
    return bResult;
}

bool Script::SetCellOwnershipFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* pOwner = nullptr;
    TESObjectCELL* pCell = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pCell,
            &pOwner
        )) {
        return false;
    }
    if (!pOwner) {
        pOwner = pPlayer->GetObjectReference();
    }
    if (pCell) {
        pCell->SetCellOwnership(pOwner);
    }
    return true;
}

bool Script::SetCellFullNameFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectCELL* pCell = nullptr;
    BGSMessage* pMessage = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pCell,
            &pMessage
        )) {
        return false;
    }
    if (pCell && pMessage) {
        pCell->TESFullName::SetFullName(pMessage->TESFullName::GetFullName());
        pCell->AddChange(CHANGE_CELL_FULLNAME);
    }

    return true;
}

bool Script::SetActorFullNameFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSMessage* pMessage = nullptr;
    if (!apRefObject) {
        return true;
    }
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pMessage
        )) {
        return false;
    }
    if (pMessage) {
        if (apRefObject->IsActor()) {
            Actor* pActor = static_cast<Actor*>(apRefObject);
            // Reads the message's name before it fetches the actor base.
            const char* pName = pMessage->TESFullName::GetFullName();
            pActor->GetActorBase()->TESFullName::SetFullName(pName);
            pActor->GetActorBase()->AddChange(CHANGE_ACTOR_BASE_FULLNAME);
        }
        else if (apRefObject->GetObjectReference()->GetFormType() == TACT_ID) {
            BGSTalkingActivator* pActivator = static_cast<BGSTalkingActivator*>(apRefObject->GetObjectReference());
            pActivator->TESFullName::SetFullName(pMessage->TESFullName::GetFullName());
            pActivator->AddChange(CHANGE_BASE_OBJECT_FULLNAME);
        }
    }
    return true;
}

bool Script::IsRunningFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::IsRunningConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    return true;
}

bool Script::GetFriendHitFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::GetFriendHitConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    return true;
}

bool Script::IsInCombatFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::IsInCombatConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    return true;
}

bool Script::SetPackageDurationFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fDuration = 0.0f;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fDuration
        )) {
        return false;
    }
    if (pActor) {
        if (pActor->GetCurrentAIProcess()) {
            pActor->GetCurrentAIProcess()->SetPackageEndTimerValue(fDuration);
        }
        if (Script::bSystemOutput) {
            Interface::PrintLine("Package duration is set to  %.02f ", fDuration);
        }
    }
    return true;
}

bool Script::PlayMagicShader(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESEffectShader* pShader = nullptr;
    float fDuration = -1.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pShader,
            &fDuration
        )) {
        return false;
    }
    if (!apRefObject) {
        apRefObject = pPlayer;
    }
    TESObjectCELL* pCell = apRefObject->GetParentCell();
    if (!TES::pInstance->IsCellLoaded(pCell, false)) {
        return true;
    }
    if (!apRefObject->Get3D()) {
        return true;
    }
    AutoMemContext kMemContext(
        MEM_CONTEXT_TES_ACTOR,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScriptFunctions.cpp",
        0x409C
    );
    MagicShaderHitEffect* pShaderEffect = new MagicShaderHitEffect(apRefObject, pShader, fDuration);
    BSTempEffect* pEffect = pShaderEffect;
    if (pShaderEffect->Init()) {
        ReferenceProcessLists.AddTempEffect(pEffect);
        if (Script::bSystemOutput) {
            if (apRefObject->GetFullName()) {
                Interface::PrintLine("Shader effect has been applied to %s", apRefObject->GetFullName());
            }
            else {
                Interface::PrintLine("Shader effect has been applied to reference");
            }
        }
    }
    else {
        if (pEffect) {
            delete pEffect;
        }
        if (Script::bSystemOutput) {
            if (apRefObject->GetFullName()) {
                Interface::PrintLine("Shader effect initialization failed for %s", apRefObject->GetFullName());
            }
            else {
                Interface::PrintLine("Shader effect initialization failed for reference");
            }
        }
    }
    return true;
}

bool Script::PlayMagicEffect(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    EffectSetting* pEffectSetting = nullptr;
    float fTime = -1.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pEffectSetting,
            &fTime
        )) {
        return false;
    }
    if (!apRefObject) {
        apRefObject = pPlayer;
    }
    TESObjectCELL* pCell = apRefObject->GetParentCell();
    if (!TES::pInstance->IsCellLoaded(pCell, false)) {
        return true;
    }
    if (apRefObject->Get3D()) {
        MagicHitEffect* pEffect = nullptr;
        if (pEffectSetting && pEffectSetting->GetModelLength()) {
            pEffect = new MagicModelHitEffect(apRefObject, pEffectSetting->GetModel(), fTime);
        }

        if (pEffect && pEffect->Init()) {
            ReferenceProcessLists.AddTempEffect(pEffect);
            if (Script::bSystemOutput) {
                if (apRefObject->GetFullName()) {
                    Interface::PrintLine("Visual effect has been applied to %s", apRefObject->GetFullName());
                }
                else {
                    Interface::PrintLine("Visual effect has been applied to reference");
                }
            }
        }
        else {
            if (pEffect) {
                delete pEffect;
            }
            if (Script::bSystemOutput) {
                if (apRefObject->GetFullName()) {
                    Interface::PrintLine("Visual effect initialization failed for %s", apRefObject->GetFullName());
                }
                else {
                    Interface::PrintLine("Visual effect initialization failed for reference");
                }
            }
        }
        return true;
    }

    NiPoint3 kAngle = apRefObject->GetAngleOnReference();
    NiPoint3 kLocation = apRefObject->GetLocationOnReference();
    float fY = NiCos(-kAngle.z);
    float fX = -NiSin(-kAngle.z);
    NiPoint3 kDirection(fX, fY, 0.0f);
    // The effect's model is read without testing the parsed effect for null on this path.
    const char* pModel = pEffectSetting->GetModel();
    BSTempEffectParticle* pParticle = BSTempEffectParticle::Spawn(
        apRefObject->GetParentCell(),
        1.0f,
        pModel,
        kDirection,
        kLocation,
        1.0f,
        7u,
        nullptr
    );
    pParticle->PlaySequence("SpecialIdle_HitEffect");
    if (Script::bSystemOutput) {
        if (apRefObject->GetFullName()) {
            Interface::PrintLine("Visual effect has been applied to %s", apRefObject->GetFullName());
        }
        else {
            Interface::PrintLine("Visual effect has been applied to reference");
        }
    }
    return true;
}

bool Script::StopMagicShader(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESEffectShader* pShader = nullptr;
    float fDuration = -1.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pShader,
            &fDuration
        )) {
        return false;
    }
    if (!apRefObject) {
        apRefObject = pPlayer;
    }
    ReferenceProcessLists.FinishMagicShaderHitEffect(apRefObject, pShader);
    if (apRefObject->GetFullName()) {
        Interface::PrintLine("Shader effect has been removed from %s", apRefObject->GetFullName());
    }
    else {
        Interface::PrintLine("Shader effect has been removed from reference");
    }
    return true;
}

// The command's parameters are an EffectSetting (a form id the parser resolves) and a float; the effect setting's
// TESModel (the base subobject at +0x18) gives the model path passed to FinishMagicModelHitEffect.
bool Script::StopMagicEffect(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    EffectSetting* pEffectSetting = nullptr;
    float fValue = -1.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pEffectSetting,
            &fValue
        )) {
        return false;
    }
    if (!apRefObject) {
        apRefObject = pPlayer;
    }
    if (pEffectSetting && pEffectSetting->GetModelLength()) {
        // Calls TESModel's virtual GetModel on the base subobject and passes its result to the callee's char*
        // parameter.
        ReferenceProcessLists.FinishMagicModelHitEffect(apRefObject, const_cast<char*>(pEffectSetting->GetModel()));
    }

    if (Script::bSystemOutput) {
        if (apRefObject->GetFullName()) {
            Interface::PrintLine("Visual effect has been removed from %s", apRefObject->GetFullName());
        }
        else {
            Interface::PrintLine("Visual effect has been removed from reference");
        }
    }
    return true;
}

bool Script::PreloadMagicEffect(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    EffectSetting* pEffectSetting = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pEffectSetting
        )) {
        return false;
    }
    if (pEffectSetting && pEffectSetting->GetModelLength()) {
        NiPointer<Model> spModel;
        if (!pModelLoader->FindModel(pEffectSetting->GetModel(), spModel)) {
            pModelLoader->QueueModel(
                pEffectSetting->GetModel(),
                IO_TASK_PRIORITY_VERY_LOW,
                nullptr,
                LOD_MULT_NONE,
                true,
                false,
                false
            );
        }
    }

    return true;
}

bool Script::SetIgnoreFriendlyHitsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iIgnore = -1;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iIgnore
        )) {
        return false;
    }
    if (iIgnore != -1) {
        static_cast<Actor*>(apRefObject)->Actor::SetIgnoreFriendlyHits(iIgnore != 0);
        apRefObject->AddChange(CHANGE_FORM_FLAGS);
    }
    return true;
}

bool Script::GetIgnoreFriendlyHitsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bool bIgnoreFriendlyHits = Script::GetIgnoreFriendlyHitsConditionFunction(
        apRefObject,
        nullptr,
        nullptr,
        afReturnValue
    );
    if (Script::bSystemOutput) {
        if (0.0 == afReturnValue) {
            Interface::PrintLine("%s", "Actor counts friendly hits");
        }
        else {
            Interface::PrintLine("%s", "Actor ignores friendly hits");
        }
    }
    return bIgnoreFriendlyHits;
}

bool Script::SetItemValueFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iValue = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iValue
        )) {
        return false;
    }
    if (apRefObject) {
        TESBoundObject* pObject = apRefObject->GetObjectReference();
        if (pObject) {
            TESValueForm* pValueForm = dynamic_cast<TESValueForm*>(pObject);
            if (pValueForm) {
                pValueForm->SetFormValue(iValue);
                if (Script::bSystemOutput) {
                    Interface::PrintLine("%s has been set to a VALUE of %i", apRefObject->GetFullName(), iValue);
                }
                BarterMenu::ForceBarterUpdate(false);
            }
        }
    }
    return true;
}

bool Script::SetActorRefractionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fRefraction = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fRefraction
        )) {
        return false;
    }
    if (!apRefObject) {
        apRefObject = pPlayer;
    }
    float fCapped = BSMIN(fRefraction, 10.0f);
    fRefraction = BSMAX(fCapped, 0.0f);
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && pActor->pCurrentProcess) {
        BaseProcess* pProcess = pActor->pCurrentProcess;
        pProcess->SetScriptRefractionPower(fRefraction);
        if (BSShaderManager::IsRefractionEnabled()) {
            float fChameleonMagnitude = BSMAX(static_cast<float>(0.0), 0.0);
            float fChameleon = BSMIN(1.0f, fChameleonMagnitude);
            if (static_cast<float>(0.0) > 0.0 && pActor == pPlayer) {
                pActor->SetAlpha(1.0f);
                pActor->SetRefraction(true, 0.05f);
            }
            else if (!(fChameleon > 0.0)) {
                if (!(fRefraction > 0.0)) {
                    pActor->SetRefraction(false, 0.0f);
                    pActor->UpdateAlpha();
                }
                else {
                    pActor->SetRefraction(true, fRefraction);
                }
            }
            else {
                pActor->SetAlpha(1.0f);
                float fRatio = 1.0 - fChameleon / 100.0;
                float fMaxRefraction = MagicSystem::GetChameleonMaxRefraction();
                float fMinRefraction = MagicSystem::GetChameleonMinRefraction();
                float fChameleonRefraction = FLerp(fMinRefraction, fMaxRefraction, 0.0f, 1.0f, fRatio);
                pActor->SetRefraction(true, fChameleonRefraction);
            }
        }
        if (Script::bSystemOutput) {
            Interface::PrintLine("%s refraction has been set to %f", apRefObject->GetFullName(), fRefraction);
        }
    }

    return true;
}

bool Script::RunCellTest(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiTest = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiTest
        )) {
        return false;
    }
    TES::pInstance->RunCellTest(uiTest);
    return true;
}

bool Script::StartAllQuests(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    for (BSSimpleList<TESQuest*>* pNode = pDataHandler->GetQuestList(); pNode && !pNode->IsEmpty();
         pNode = pNode->m_pkNext) {
        TESQuest* pQuest = pNode->m_item;
        pQuest->SetEnabled(1u);
    }
    if (Script::bSystemOutput) {
        Interface::PrintLine("All Quests Enabled.", afReturnValue);
    }
    return true;
}

bool Script::SetGamma(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fGamma = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fGamma
        )) {
        return false;
    }
    Renderer::SetGamma(fGamma);
    return true;
}

bool Script::SetDepthOfField(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fAniso = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fAniso
        )) {
        return false;
    }
    int32_t iAniso = static_cast<int32_t>(fAniso);
    int32_t iMaxAniso;
    if (iAniso < 2) {
        iMaxAniso = 1;
    }
    else if (iAniso < 4) {
        iMaxAniso = 2;
    }
    else if (iAniso < 8) {
        iMaxAniso = 4;
    }
    else if (iAniso < 12) {
        iMaxAniso = 8;
    }
    else if (iAniso < 16) {
        iMaxAniso = 12;
    }
    else {
        iMaxAniso = 16;
    }
    if (Script::bSystemOutput) {
        Interface::PrintLine("Maxaniso set to %d", iMaxAniso);
    }
    NiRenderer::iMaxAnisotropy = iMaxAniso;
    return true;
}

bool Script::SetTalkingActivatorActorFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pRef = nullptr;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &pRef
    );
    if (apRefObject && apRefObject->GetObjectReference()->GetFormType() == TACT_ID) {
        BGSTalkingActivator* pActivator = static_cast<BGSTalkingActivator*>(apRefObject->GetObjectReference());
        pActivator->SetTempRef(pRef);
    }

    return true;
}

bool Script::AddPerkFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSPerk* pPerk = nullptr;
    int32_t iAlt = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pPerk,
            &iAlt
        )) {
        return false;
    }
    bool bAlt = iAlt != 0;
    if (pPerk && apRefObject->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        uint8_t ucRank = pActor->GetPerkRank(pPerk, bAlt);
        if (pPerk->GetNumRanks() <= ucRank) {
            afReturnValue = static_cast<double>(ucRank);
        }
        else {
            pActor->AddPerk(pPerk, ucRank + 1, bAlt);
            if (Script::bSystemOutput) {
                const char* pActorName = pActor->GetFullName();
                Interface::PrintLine(
                    "Added Perk %s to %s with rank %i",
                    pPerk->TESFullName::GetFullName(),
                    pActorName,
                    ucRank + 1
                );
            }
            afReturnValue = static_cast<double>(ucRank + 1);
        }
    }

    return true;
}

bool Script::RemovePerkFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSPerk* pPerk = nullptr;
    int32_t iAlt = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pPerk,
            &iAlt
        )) {
        return false;
    }
    bool bAlt = iAlt != 0;
    if (pPerk && apRefObject->IsActor()) {
        static_cast<Actor*>(apRefObject)->RemovePerk(pPerk, bAlt);
    }

    return true;
}

bool Script::HasPerkFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pPerk = nullptr;
    void* pParam2 = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pPerk,
               &pParam2
           ) &&
        Script::HasPerkConditionFunction(apRefObject, pPerk, pParam2, afReturnValue);
}

bool Script::SendSherlockDebugPage(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char acPageName[0x800];
    *reinterpret_cast<uint32_t*>(acPageName) = 0;
    std::memset(&acPageName[4], 0, sizeof(acPageName) - 4);
    if (Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            acPageName
        )) {
        if (Interface::IsDebugTextVisible() || *reinterpret_cast<uint32_t*>(acPageName)) {
            const char* pPage;
            if (!Interface::IsDebugTextVisible() || *reinterpret_cast<uint32_t*>(acPageName)) {
                pPage = acPageName;
            }
            else {
                pPage = DebugTextManager::GetSingleton()->GetScreenDebugPage();
            }
            if (!apRefObject) {
                apRefObject = pPlayer;
            }
            DebugTextManager::GetSingleton()->AddTrackedDebugPage(pPage, apRefObject);
        }
        else {
            Interface::PrintLine("You must supply a debug page name if debug text is not showing");
        }
    }
    return true;
}

bool Script::RewardExperiencePointsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiXP = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiXP
        )) {
        return false;
    }
    pPlayer->RewardExperience(uiXP);
    return true;
}

bool Script::RewardKarmaFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iKarma = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iKarma
        )) {
        return false;
    }
    pPlayer->RewardKarma(iKarma);
    return true;
}

bool Script::ShowRepairMenuFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject && apRefObject->IsActor()) {
        Interface::CreateRepairServicesMenu(static_cast<Actor*>(apRefObject));
    }

    return true;
}

bool Script::AddNoteFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSNote* pNote = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pNote
        )) {
        return false;
    }
    if (pNote) {
        pPlayer->AddNote(pNote, Script::bSystemOutput);
    }
    return true;
}

bool Script::RemoveNoteFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSNote* pNote = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pNote
        )) {
        return false;
    }
    if (pNote) {
        pPlayer->RemoveNote(pNote);
    }
    if (Script::bSystemOutput) {
        Interface::PrintLine("Removed Note %s from Player ", pNote->GetFullName());
    }
    return true;
}

bool Script::AddFactionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction = nullptr;
    int32_t iRank = -1;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction,
            &iRank
        )) {
        return false;
    }
    if (pFaction && apRefObject->IsActor()) {
        static_cast<Actor*>(apRefObject)->Actor::AddFaction(pFaction, iRank);
    }

    return true;
}

bool Script::RemoveFactionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction
        )) {
        return false;
    }
    if (pFaction && apRefObject->IsActor()) {
        static_cast<Actor*>(apRefObject)->Actor::RemoveFaction(pFaction);
    }

    return true;
}

bool Script::SetDispositionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = nullptr;
    if (apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }
    Actor* pTarget = nullptr;
    int32_t iDisposition = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTarget,
            &iDisposition
        )) {
        return false;
    }
    if (pTarget && pActor) {
        int32_t iCurrent = pActor->GetDisposition(pTarget, nullptr);
        pActor->ModifyDispositionTowardActor(pTarget, static_cast<float>(iDisposition - iCurrent));
    }

    return true;
}

bool Script::GetHitLocationFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::GetHitLocationConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::bSystemOutput) {
        Interface::PrintLine("GetHitLocation >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetLastHitCriticalFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::GetLastHitCriticalConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::bSystemOutput) {
        Interface::PrintLine("GetLastHitCritical >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsPC1stPersonFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::IsPC1stPersonConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::bSystemOutput) {
        Interface::PrintLine("IsPC1stPerson >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::SetCombatActionCostFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cAction[516];
    float fCost = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cAction,
            &fCost
        )) {
        return false;
    }
    char bFound = SetCombatActionCost(cAction, fCost);
    if (Script::bSystemOutput) {
        if (bFound) {
            Interface::PrintLine("%s >> %0.2f", cAction, fCost);
        }
        else {
            Interface::PrintLine("%s not found", cAction);
        }
    }
    return true;
}

bool Script::ToggleCombatDebugFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cOption[516];
    ourStrcpy(cOption, "");
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cOption
        )) {
        return false;
    }
    if (!std::strlen(cOption)) {
        // No option: turn every combat debug display off when any is on, else turn them all on.
        uint8_t bAnyOn = bDebugCombatCoverReservations.Bool() ||
            bDebugCombatThreats.Bool() ||
            bDebugCombatGroups.Bool() ||
            bDebugCombatGroups2.Bool() ||
            bDebugCombatTargets.Bool() ||
            bDebugCombatCoverSearch.Bool() ||
            bDebugCombatSearch.Bool() ||
            bDebugCombatCoverSearch2.Bool() ||
            bDebugCombatCoverSearchText.Bool() ||
            bDebugCombatCoverReservationsText.Bool() ||
            bDebugCombatAttackerText.Bool() ||
            bDebugCombatGuardRadius.Bool() ||
            bDebugCombatGroupClusters.Bool() ||
            bDebugCombatUnreachableLocations.Bool() ||
            bDebugCombatDetectionEvents.Bool() ||
            bDebugCombatProjectileLOS.Bool() ||
            bDebugCombatArea.Bool();
        bDebugCombatCoverReservations = !bAnyOn;
        bDebugCombatThreats = !bAnyOn;
        bDebugCombatGroups = !bAnyOn;
        bDebugCombatGroups2 = !bAnyOn;
        bDebugCombatTargets = !bAnyOn;
        bDebugCombatCoverSearch = !bAnyOn;
        bDebugCombatCoverSearch2 = !bAnyOn;
        bDebugCombatSearch = !bAnyOn;
        bDebugCombatCoverSearchText = !bAnyOn;
        bDebugCombatCoverReservationsText = !bAnyOn;
        bDebugCombatAttackerText = !bAnyOn;
        bDebugCombatGuardRadius = !bAnyOn;
        bDebugCombatGroupClusters = !bAnyOn;
        bDebugCombatUnreachableLocations = !bAnyOn;
        bDebugCombatDetectionEvents = !bAnyOn;
        bDebugCombatProjectileLOS = !bAnyOn;
        bDebugCombatArea = !bAnyOn;
        if (Script::bSystemOutput) {
            Interface::PrintLine("All Combat debugging turned %s", bAnyOn ? "Off" : "On");
        }
    }
    else if (!BSstricmp(cOption, "cover")) {
        bDebugCombatCoverReservations = !bDebugCombatCoverReservations.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine(
                "Combat Cover Reservations debugging turned %s",
                bDebugCombatCoverReservations.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "covertext")) {
        bDebugCombatCoverReservationsText = !bDebugCombatCoverReservationsText.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine(
                "Combat Cover Reservations debug text turned %s",
                bDebugCombatCoverReservationsText.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "cover2") || !BSstricmp(cOption, "coversearch")) {
        bDebugCombatCoverSearch = !bDebugCombatCoverSearch.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine(
                "Combat Cover Search debugging turned %s",
                bDebugCombatCoverSearch.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "cover2text") || !BSstricmp(cOption, "coversearchtext")) {
        bDebugCombatCoverSearchText = !bDebugCombatCoverSearchText.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine(
                "Combat Cover Search debug text turned %s",
                bDebugCombatCoverSearchText.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "cover3") || !BSstricmp(cOption, "coversearch2")) {
        bDebugCombatCoverSearch2 = !bDebugCombatCoverSearch2.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine(
                "Combat Secondary Cover Search debugging turned %s",
                bDebugCombatCoverSearch2.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "threats")) {
        bDebugCombatThreats = !bDebugCombatThreats.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine("Combat Threat debugging turned %s", bDebugCombatThreats.Bool() ? "On" : "Off");
        }
    }
    else if (!BSstricmp(cOption, "groups")) {
        bDebugCombatGroups = !bDebugCombatGroups.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine("Combat Group debugging turned %s", bDebugCombatGroups.Bool() ? "On" : "Off");
        }
    }
    else if (!BSstricmp(cOption, "groups2")) {
        bDebugCombatGroups2 = !bDebugCombatGroups2.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine("Combat Group 2 debugging turned %s", bDebugCombatGroups2.Bool() ? "On" : "Off");
        }
    }
    else if (!BSstricmp(cOption, "targets")) {
        bDebugCombatTargets = !bDebugCombatTargets.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine("Combat Target debugging turned %s", bDebugCombatTargets.Bool() ? "On" : "Off");
        }
    }
    else if (!BSstricmp(cOption, "search")) {
        bDebugCombatSearch = !bDebugCombatSearch.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine("Combat Search debugging turned %s", bDebugCombatSearch.Bool() ? "On" : "Off");
        }
    }
    else if (!BSstricmp(cOption, "guard")) {
        bDebugCombatGuardRadius = !bDebugCombatGuardRadius.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine(
                "Combat Guard Radius debugging turned %s",
                bDebugCombatGuardRadius.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "cluster") || !BSstricmp(cOption, "clusters")) {
        bDebugCombatGroupClusters = !bDebugCombatGroupClusters.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine(
                "Combat Group Cluster debugging turned %s",
                bDebugCombatGroupClusters.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "text")) {
        bDebugCombatAttackerText = !bDebugCombatAttackerText.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine(
                "Combat Attacker Text debugging turned %s",
                bDebugCombatAttackerText.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "color")) {
        bDebugCombatTextColorDark = !bDebugCombatTextColorDark.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine(
                "Combat Debug Text color changed to %s",
                bDebugCombatTextColorDark.Bool() ? "Dark" : "Light"
            );
        }
    }
    else if (!BSstricmp(cOption, "size")) {
        // Subtracts 0.1f as a double and rounds the result to a float before storing it.
        float fSize = fDebugCombatTextSize.Float() - 0.10000000149011612;
        fDebugCombatTextSize = fSize;
        if (fDebugCombatTextSize.Float() <= 0.05000000074505806) {
            fDebugCombatTextSize = 1.0f;
        }
        if (Script::bSystemOutput) {
            Interface::PrintLine("Combat Debug Text size changed to %.2f", fDebugCombatTextSize.Float());
        }
    }
    else if (!BSstricmp(cOption, "unreach") || !BSstricmp(cOption, "unreachable")) {
        bDebugCombatUnreachableLocations = !bDebugCombatUnreachableLocations.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine(
                "Combat Unreachable Locations debugging turned %s",
                bDebugCombatUnreachableLocations.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "events")) {
        bDebugCombatDetectionEvents = !bDebugCombatDetectionEvents.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine(
                "Combat Detection Events debugging turned %s",
                bDebugCombatDetectionEvents.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "plos")) {
        bDebugCombatProjectileLOS = !bDebugCombatProjectileLOS.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine(
                "Combat Projectile LOS debugging turned %s",
                bDebugCombatProjectileLOS.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "area")) {
        bDebugCombatArea = !bDebugCombatArea.Bool();
        if (Script::bSystemOutput) {
            Interface::PrintLine("Combat Area debugging turned %s", bDebugCombatArea.Bool() ? "On" : "Off");
        }
    }
    else if (Script::bSystemOutput) {
        Interface::PrintLine("Unknown option '%s'", cOption);
    }
    return true;
}

bool Script::SetActorsAIFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiProcessing;
    if (apRefObject->IsActor()) {
        if (!Script::ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &uiProcessing
            )) {
            return false;
        }
        TESObjectREFR* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor) {
            if (uiProcessing) {
                pActor[1].m_Extra.iFlags[8] = 1;
            }
            else {
                pActor[1].m_Extra.iFlags[8] = 0;
            }
            if (Script::bSystemOutput) {
                const char* pState = pActor[1].m_Extra.iFlags[8] ? "On" : "Off";
                Interface::PrintLine("%s processing is  %s", pActor->GetFullName(), pState);
            }
        }
    }
    return true;
}

bool Script::ClearOwnershipFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        apRefObject->TESObjectREFR::GetExtra()->RemoveOwnership();
        apRefObject->AddChange(CHANGE_REFR_EXTRA_OWNERSHIP);
    }
    return true;
}

bool Script::ShowInventoryFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        Actor* pActor = nullptr;
        if (apRefObject->IsActor()) {
            pActor = static_cast<Actor*>(apRefObject);
        }
        int32_t iCount = apRefObject->GetInventoryCount(false, true);
        if (iCount > 0) {
            const char* pRefName;
            if (apRefObject->GetFormEditorIDLength()) {
                pRefName = apRefObject->GetFormEditorID();
            }
            else {
                pRefName = apRefObject->GetFullName();
            }
            Interface::PrintLine("%s (%08X) has %i items:", pRefName, apRefObject->iFormID, iCount);
            // The index compare is unsigned.
            for (uint32_t i = 0; i < static_cast<uint32_t>(iCount); ++i) {
                ItemChange* pItem = apRefObject->GetInventoryItem(i, false);
                if (pItem && pItem->pContainerObj) {
                    TESBoundObject* pObject = pItem->pContainerObj;
                    int32_t iFormType = pObject->GetFormType();
                    if (iFormType == 24) {
                        const char* pName;
                        const char* pWorn = pItem->GetWorn(false) ? " - Worn" : "";
                        if (pObject->GetFormEditorIDLength()) {
                            pName = pObject->GetFormEditorID();
                        }
                        else {
                            pName = TESFullName::GetFullName(pObject);
                        }
                        double dHealthPercent = pItem->GetItemHealth(true);
                        double dHealth = pItem->GetItemHealth(false);
                        Interface::PrintLine(
                            "%d - %s (%08X) (%.0f,%0.2f%%)%s",
                            pItem->iNumber,
                            pName,
                            pObject->iFormID,
                            dHealth,
                            dHealthPercent,
                            pWorn
                        );
                    }
                    else if (iFormType == 40) {
                        const char* pName;
                        const char* pCannotEquip;
                        if (!pActor || pActor->CanUseWeapon(static_cast<TESObjectWEAP*>(pObject), true)) {
                            pCannotEquip = "";
                        }
                        else {
                            pCannotEquip = " [CANNOT EQUIP]";
                        }
                        const char* pWorn = pItem->GetWorn(false) ? " - Worn" : "";
                        if (pObject->GetFormEditorIDLength()) {
                            pName = pObject->GetFormEditorID();
                        }
                        else {
                            pName = TESFullName::GetFullName(pObject);
                        }
                        double dHealthPercent = pItem->GetItemHealth(true);
                        double dHealth = pItem->GetItemHealth(false);
                        Interface::PrintLine(
                            "%d - %s (%08X) (%.0f,%0.2f%%)%s%s",
                            pItem->iNumber,
                            pName,
                            pObject->iFormID,
                            dHealth,
                            dHealthPercent,
                            pWorn,
                            pCannotEquip
                        );
                    }
                    else {
                        const char* pName;
                        const char* pWorn = pItem->GetWorn(false) ? " - Worn" : "";
                        if (pObject->GetFormEditorIDLength()) {
                            pName = pObject->GetFormEditorID();
                        }
                        else {
                            pName = TESFullName::GetFullName(pObject);
                        }
                        Interface::PrintLine("%d - %s (%08X)%s", pItem->iNumber, pName, pObject->iFormID, pWorn);
                    }
                    BGSBipedModelList* pModelList = BGSBipedModelList::GetFormAsBipedModelList(pObject);
                    if (pModelList) {
                        BGSListForm* pListForm = pModelList->pListForm;
                        if (pListForm) {
                            BSSimpleList<TESForm*>* pIter = &pListForm->ListOfForms;
                            const char* pListName;
                            if (pListForm->GetFormEditorIDLength()) {
                                pListName = pListForm->GetFormEditorID();
                            }
                            else {
                                pListName = TESFullName::GetFullName(pListForm);
                            }
                            Interface::PrintLine("  === %s (%08X) ===", pListName, pListForm->iFormID);
                            while (pIter) {
                                TESForm* pForm = pIter->m_item;
                                const char* pFormName;
                                if (pForm->GetFormEditorIDLength()) {
                                    pFormName = pForm->GetFormEditorID();
                                }
                                else {
                                    pFormName = TESFullName::GetFullName(pForm);
                                }
                                Interface::PrintLine("  ---> %s (%08X)", pFormName, pForm->iFormID);
                                pIter = pIter->m_pkNext;
                            }
                        }
                    }
                }

                if (pItem) {
                    delete pItem;
                }
            }
        }
    }
    return true;
}

bool Script::ShowCharGenMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iSPECIALPoints = 0;
    int32_t iSkillPoints = 0;
    int32_t iTagSkills = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iSPECIALPoints,
            &iSkillPoints,
            &iTagSkills
        )) {
        return false;
    }
    if (iSPECIALPoints > 0) {
        Interface::CreateCharGenMenu(0, iSPECIALPoints, false, true);
    }
    else if (iTagSkills > 0) {
        Interface::CreateCharGenMenu(1, iTagSkills, false, true);
    }
    return true;
}

bool Script::ShowCharGenMenuModValues(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iSPECIALPoints = 0;
    int32_t iSkillPoints = 0;
    int32_t iTagSkills = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iSPECIALPoints,
            &iSkillPoints,
            &iTagSkills
        )) {
        return false;
    }
    if (iSPECIALPoints > 0) {
        Interface::CreateCharGenMenu(0, iSPECIALPoints, true, true);
    }
    else if (iTagSkills > 0) {
        Interface::CreateCharGenMenu(1, iTagSkills, true, true);
    }
    return true;
}

bool Script::SetSPECIALPoints(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iMaxPoints = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iMaxPoints
        )) {
        return false;
    }
    Interface::CreateCharGenMenu(0, iMaxPoints, false, true);
    return true;
}

bool Script::AddSPECIALPoints(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iMaxPoints = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iMaxPoints
        )) {
        return false;
    }
    Interface::CreateCharGenMenu(0, iMaxPoints, true, true);
    return true;
}

bool Script::SetTagSkills(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iMaxPoints = 0;
    int32_t iShowTaggedSkills = 1;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iMaxPoints,
            &iShowTaggedSkills
        )) {
        return false;
    }
    Interface::CreateCharGenMenu(1, iMaxPoints, false, iShowTaggedSkills == 1);
    return true;
}

bool Script::AddTagSkills(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iMaxPoints = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iMaxPoints
        )) {
        return false;
    }
    Interface::CreateCharGenMenu(1, iMaxPoints, true, true);
    return true;
}

bool Script::ShowLockpickMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iLockLevel = 3;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &iLockLevel
    );
    Interface::CreateLockPickMenu(nullptr, iLockLevel - 1, false);
    return true;
}

bool Script::ShowLockpickMenuDebug(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iLockLevel = 3;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &iLockLevel
    );
    Interface::CreateLockPickMenu(nullptr, iLockLevel - 1, true);
    return true;
}

bool Script::ResetPerformanceTimers(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    InitializationFlow();
    return true;
}

bool Script::ToggleDecalDebug(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iOption = -1;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iOption
        )) {
        return false;
    }
    if (iOption != -1) {
        switch (iOption) {
            case 0: {
                if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_WIREFRAME_CLIPPING_PLANES) != 0) {
                    BGSDecalManager::iDecalDebugFlags &= ~BGSDecalManager::DEBUG_WIREFRAME_CLIPPING_PLANES;
                    Interface::PrintLine("Wireframe Decal Clipping Planes : DISABLED");
                }
                else {
                    BGSDecalManager::iDecalDebugFlags |= BGSDecalManager::DEBUG_WIREFRAME_CLIPPING_PLANES;
                    Interface::PrintLine("Wireframe Decal Clipping Planes : ENABLED");
                }
                break;
            }
            case 1: {
                if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_CLIPPING_PLANES) != 0) {
                    BGSDecalManager::iDecalDebugFlags &= ~BGSDecalManager::DEBUG_CLIPPING_PLANES;
                    Interface::PrintLine("Decal Clipping Planes : DISABLED");
                }
                else {
                    BGSDecalManager::iDecalDebugFlags |= BGSDecalManager::DEBUG_CLIPPING_PLANES;
                    Interface::PrintLine("Decal Clipping Planes : ENABLED");
                }
                break;
            }
            case 2: {
                if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_QUERY_CAMERA_DIRECTION) != 0) {
                    BGSDecalManager::iDecalDebugFlags &= ~BGSDecalManager::DEBUG_QUERY_CAMERA_DIRECTION;
                    Interface::PrintLine("Occlusion query camera direction : DISABLED");
                }
                else {
                    BGSDecalManager::iDecalDebugFlags |= BGSDecalManager::DEBUG_QUERY_CAMERA_DIRECTION;
                    Interface::PrintLine("Occlusion query camera direction : ENABLED");
                }
                break;
            }
            case 3: {
                if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_DECAL_TRANSFORM) != 0) {
                    BGSDecalManager::iDecalDebugFlags &= ~BGSDecalManager::DEBUG_DECAL_TRANSFORM;
                    Interface::PrintLine("Decal Transform : DISABLED");
                }
                else {
                    BGSDecalManager::iDecalDebugFlags |= BGSDecalManager::DEBUG_DECAL_TRANSFORM;
                    Interface::PrintLine("Decal Transform : ENABLED");
                }
                break;
            }
            case 4: {
                if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_FAILED_DECALS) != 0) {
                    BGSDecalManager::iDecalDebugFlags &= ~BGSDecalManager::DEBUG_FAILED_DECALS;
                    Interface::PrintLine("Failed Decals : DISABLED");
                }
                else {
                    BGSDecalManager::iDecalDebugFlags |= BGSDecalManager::DEBUG_FAILED_DECALS;
                    Interface::PrintLine("Failed Decals : ENABLED");
                }
                break;
            }
            case 5: {
                if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_WIREFRAME_DECALS) != 0) {
                    BGSDecalManager::iDecalDebugFlags &= ~BGSDecalManager::DEBUG_WIREFRAME_DECALS;
                    g_enableWireframeDecals = 0;
                    Interface::PrintLine("Wireframe Decals : DISABLED");
                }
                else {
                    BGSDecalManager::iDecalDebugFlags |= BGSDecalManager::DEBUG_WIREFRAME_DECALS;
                    g_enableWireframeDecals = 1;
                    Interface::PrintLine("Wireframe Decals : ENABLED");
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    return true;
}

bool Script::SetImageSpaceModifiersEnable(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iEnable = -1;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iEnable
        )) {
        return false;
    }
    if (!iEnable) {
        TESImageSpaceModifier::SetEnable(false);
    }
    if (iEnable == 1) {
        TESImageSpaceModifier::SetEnable(true);
    }
    return true;
}

bool Script::GetIsMovingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::GetIsMovingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::bSystemOutput) {
        Interface::PrintLine("IsMoving >> %0.0f", afReturnValue);
    }
    return true;
}

bool Script::GetIsTurningFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::GetIsTurningConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::bSystemOutput) {
        Interface::PrintLine("IsTurning >> %0.0f", afReturnValue);
    }
    return true;
}

bool Script::GetAnimActionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::GetAnimActionConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::bSystemOutput) {
        Interface::PrintLine("GetAnimAction >> %0.0f", afReturnValue);
    }
    return true;
}

bool Script::UseWeaponFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESBoundObject* pWeapon = nullptr;
    TESObjectREFR* pLocRef = nullptr;
    TESObjectREFR* pTargetRef = nullptr;
    int32_t iAttackTimes = 0;
    int32_t iCrouch = 1;
    int32_t iHoldFire = 1;
    int32_t iAlwaysHit = 0;
    int32_t iDoNoDamage = 0;
    TESObjectREFR* pTargetLocRef = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pWeapon,
            &pLocRef,
            &pTargetRef,
            &iAttackTimes,
            &iCrouch,
            &iHoldFire,
            &iAlwaysHit,
            &iDoNoDamage,
            &pTargetLocRef
        )) {
        return false;
    }
    if (apRefObject->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        pActor->StopCombat(nullptr);
        pActor->InitiateUseWeaponPackage(
            pWeapon,
            pLocRef,
            pTargetRef,
            iAttackTimes,
            pTargetLocRef,
            iCrouch != 0,
            iHoldFire != 0,
            iAlwaysHit != 0,
            iDoNoDamage != 0
        );
    }
    return true;
}

bool Script::EvaluateSpellConditionsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    MagicTarget* pTarget = apRefObject->GetMagicTarget();
    if (pTarget) {
        pTarget->EvaluateSpellConditions();
    }
    return true;
}

bool Script::ClearScreenBlood(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ScreenSplatter::Clear();
    return true;
}

bool Script::TriggerScreenSplatter(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iCount = 0;
    float fDuration = 0.0f;
    float fSizeMult = 0.0f;
    float fOpacityMult = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iCount,
            &fDuration,
            &fSizeMult,
            &fOpacityMult
        )) {
        return false;
    }
    ScreenCustomSplatter::Activate(
        static_cast<uint32_t>(iCount),
        fDuration,
        SCREENSCUSTOMPLATTER_BLOOD,
        fSizeMult,
        fOpacityMult
    );
    return true;
}

bool Script::SetScreenSplatterFade(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fFadeStart = 0.0f;
    // The Integer parameter (SCRIPT_PARAM_INT) parses to 4 bytes, so iNoFade is given 4 bytes. Only the low byte is
    // tested against 0 before it is passed on.
    int32_t iNoFade;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fFadeStart,
            &iNoFade
        )) {
        return false;
    }
    SetCustomSplatterFade(fFadeStart, static_cast<uint8_t>(iNoFade) != 0);
    return true;
}

bool Script::SetScreenSplatterTexture(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cTexture[256];
    // The Integer parameter (SCRIPT_PARAM_INT) parses to 4 bytes, so iValue is 4 bytes wide. Only the low byte is
    // passed on.
    int32_t iValue;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cTexture,
            &iValue
        )) {
        return false;
    }
    ScreenCustomSplatter::SetCustomSplatterTexture(cTexture, static_cast<uint8_t>(iValue));
    return true;
}

bool Script::ClearScreenSplatter(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    CustomBloodSplatter();
    return true;
}

bool Script::AgeRaceFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiSteps = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiSteps
        )) {
        return false;
    }
    if (uiSteps && apRefObject && apRefObject->IsNpc()) {
        TESNPC* pNPC = static_cast<TESNPC*>(apRefObject->GetObjectReference());
        TESRace* pOriginalRace = pNPC->pFormRace;
        TESRace* pRace = pOriginalRace;
        while (uiSteps) {
            if ((uiSteps & 0x80000000u) == 0) {
                if (pRace->GetOlderRace()) {
                    pRace = pRace->GetOlderRace();
                    --uiSteps;
                }
                else {
                    uiSteps = 0;
                }
            }
            else if (pRace->GetYoungerRace()) {
                pRace = pRace->GetYoungerRace();
                ++uiSteps;
            }
            else {
                uiSteps = 0;
            }
        }
        if (pRace != pOriginalRace) {
            pNPC->SwitchRace(pRace, apRefObject->IsActor() ? static_cast<Actor*>(apRefObject) : nullptr);
            static_cast<Character*>(apRefObject)->Character::Reset3D();
        }
    }

    return true;
}

bool Script::GetVATSBackAreaFreeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetVATSBackAreaFreeConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::GetVATSFrontAreaFreeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetVATSFrontAreaFreeConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

void Set(char abDump) {
    bDumpNextFrame = abDump;
}

bool Script::GetVATSRightTargetVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetVATSRightTargetVisibleConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::GetVATSLeftTargetVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetVATSLeftTargetVisibleConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::GetVATSBackTargetVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetVATSBackTargetVisibleConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::GetVATSFrontTargetVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetVATSFrontTargetVisibleConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::SetCritStageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ACTOR_CRITICAL_STAGE eStage = ACS_NONE;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eStage
        )) {
        return false;
    }
    if (apRefObject && apRefObject->IsActor()) {
        static_cast<Actor*>(apRefObject)->Actor::SetCriticalStage(eStage);
    }

    return true;
}

bool Script::IsInCritStageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pStage = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pStage
           ) &&
        Script::IsInCritStageConditionFunction(apRefObject, pStage, nullptr, afReturnValue);
}

bool Script::SinFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fX = 0.0f;
    int32_t iInverse = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fX,
            &iInverse
        )) {
        return false;
    }
    if (iInverse) {
        if (fX < -1.0) {
            fX = -1.0f;
        }
        if (fX > 1.0) {
            fX = 1.0f;
        }
        afReturnValue = std::asin(fX) * 57.2957763671875;
    }
    else {
        float fRadians = fX * 0.01745329238474369;
        afReturnValue = std::sin(fRadians);
    }
    return true;
}

bool Script::CosFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fX = 0.0f;
    int32_t iInverse = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fX,
            &iInverse
        )) {
        return false;
    }
    if (iInverse) {
        if (fX < -1.0) {
            fX = -1.0f;
        }
        if (fX > 1.0) {
            fX = 1.0f;
        }
        afReturnValue = std::acos(fX) * 57.2957763671875;
    }
    else {
        float fRadians = fX * 0.01745329238474369;
        afReturnValue = std::cos(fRadians);
    }
    return true;
}

bool Script::TanFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fX = 0.0f;
    int32_t iInverse = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fX,
            &iInverse
        )) {
        return false;
    }
    if (iInverse) {
        afReturnValue = std::atan(fX) * 57.2957763671875;
    }
    else {
        float fRadians = fX * 0.01745329238474369;
        afReturnValue = std::tan(fRadians);
    }
    return true;
}

bool Script::SqrtFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fX = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fX
        )) {
        return false;
    }
    afReturnValue = std::sqrt(fX);
    return true;
}

bool Script::LogFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fX = 0.0f;
    float fBase = 2.7182817f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fX,
            &fBase
        )) {
        return false;
    }
    if (fBase == 1.0) {
        afReturnValue = 0.0;
    }
    else {
        double dLogX = std::log(fX);
        afReturnValue = dLogX / std::log(fBase);
    }
    return true;
}

bool Script::ShowQuestStagesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* pQuest = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest
        )) {
        return false;
    }
    BSStringT<char> kStages;
    if (Script::bSystemOutput) {
        for (BSSimpleList<TESQuestStage*>* pStageIter = &pQuest->m_listStages; pStageIter && pStageIter->m_item;
             pStageIter = pStageIter->m_pkNext) {
            char cBuffer[32];
            TESQuestStage* pStage = pStageIter->m_item;
            BSsprintf(cBuffer, 31, "Stage %d: %d ", pStage->m_Data.ucIndex, pStage->m_Data.bIsDone != 0);
            kStages = cBuffer;
            // Tests the item list's second link rather than its first item.
            if (pStage->listStageItems.m_pkNext) {
                kStages += " (";
                for (BSSimpleList<TESQuestStageItem*>* pItemIter = &pStage->listStageItems;
                     pItemIter && pItemIter->m_item;
                     pItemIter = pItemIter->m_pkNext) {
                    TESQuestStageItem* pItem = pItemIter->m_item;
                    BSsprintf(cBuffer, 31, "Item %d: %d", pItem->ucIndex, pItem->m_pLogDate != nullptr);
                    kStages += cBuffer;
                    if (pItemIter->m_pkNext) {
                        kStages += ", ";
                    }
                }
                kStages += ")";
            }
            BSStringT<char> kLine(kStages);
            Interface::PrintLine(kLine);
            // Natively C++ passes PrintLine a copy of this raw-storage local, so the local is destroyed here.
        }
    }
    return true;
}

bool Script::SetTaskThreadUpdateSleepFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iSleep = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iSleep
        )) {
        return false;
    }
    BSTaskThread::SetUpdateSleepValue(iSleep);
    Interface::PrintLine("BSTaskThread::sUpdateSleepValue set to %d ms", iSleep);
    return true;
}

bool Script::AddFormToFormListFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSListForm* pList = nullptr;
    TESForm* pForm = nullptr;
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pList,
            &pForm
        )) {
        return false;
    }
    if (pForm && pList) {
        pList->AddForm(pForm);
    }

    return true;
}

bool Script::AddItemToLeveledListFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESLevItem* pList = nullptr;
    TESForm* pForm = nullptr;
    int32_t iLevel = 0;
    int32_t iCount = 0;
    float fHealth = 100.0f;
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pList,
            &pForm,
            &iLevel,
            &iCount,
            &fHealth
        )) {
        return false;
    }
    if (pList && pForm && iCount) {
        ContainerItemExtra* pExtra;
        fHealth = fHealth / 100.0;
        pExtra = new ContainerItemExtra;
        pExtra->AdjustHealthPercent(fHealth);
        static_cast<TESLeveledList*>(pList)->AddScriptAddedLeveledObject(
            pList,
            static_cast<uint16_t>(iLevel),
            static_cast<uint16_t>(iCount),
            pForm,
            pExtra
        );
    }

    return true;
}

bool Script::RemapWaterTypeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* pWater = nullptr;
    TESForm* pRemapWater = nullptr;
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pWater,
            &pRemapWater
        )) {
        return false;
    }
    if (!pWater || pWater->GetFormType() != WATR_ID) {
        Warning("Invalid water type for water to be remapped.");
        pWater = nullptr;
    }
    if (!pRemapWater || pRemapWater->GetFormType() != WATR_ID) {
        Warning("Invalid water type for water to remap to.");
        pRemapWater = nullptr;
    }
    if (pWater && pRemapWater) {
        static_cast<TESWaterForm*>(pWater)->TESWaterForm::SetRemappedWaterForm(static_cast<TESWaterForm*>(pRemapWater));
    }

    return true;
}

bool Script::IsHardcoreFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsHardcoreConditionalFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::SetHardcoreFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    // The parsed integer, tested signed against the two settings.
    int32_t iHardcore = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iHardcore
        )) {
        return false;
    }
    if (pPlayer && iHardcore >= GamePlayFormulas::HARDCORE_OFF && iHardcore <= GamePlayFormulas::HARDCORE_ON) {
        pPlayer->SetHardcore(static_cast<GamePlayFormulas::HardcoreSetting>(iHardcore), false);
        if (iHardcore != GamePlayFormulas::HARDCORE_ON) {
            pPlayer->ClearsHardcoreFlag();
        }
        return true;
    }

    Warning("Attempting to set hardcore on an invalid player!");
    return false;
}

bool Script::ObjectUnderReticleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    CFilter kGroupFilter;
    alignas(16) CFilter kPickFilter;
    CFilter kPlayerFilter;
    alignas(16) NiPoint3 kRow;
    NiMatrix3 kRotation;
    alignas(16) NiMatrix3 kCameraRotation;
    float fDistance;
    alignas(16) float afParam[3];
    afReturnValue = 0.0;
    afParam[2] = 0.0f;
    afParam[1] = std::bit_cast<float>(static_cast<uint32_t>(1));
    afParam[0] = 0.0f;
    fDistance = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            afParam,
            &fDistance
        )) {
        return false;
    }
    fDistance = BSMIN(fDistance, 50000.0f);
    bhkPickData kPickData;
    NiPoint3 kTo(0.0f, fDistance, 0.0f);
    NiPoint3 kCamPos = pPlayer->GetLocationOnReference();
    NiAVObject* pCamera = Main::GetWorldSceneGraph()->GetCamera();
    kCameraRotation = pCamera->GetWorldTransform().m_Rotate;
    // Each camera row goes into the pick rotation with its components rotated: GetRow writes its first, second and
    // third entries to y, z and x.
    kCameraRotation.GetRow(0, kRow.y, kRow.z, kRow.x);
    kRotation.SetRow(0, kRow);
    kCameraRotation.GetRow(1, kRow.y, kRow.z, kRow.x);
    kRotation.SetRow(1, kRow);
    kCameraRotation.GetRow(2, kRow.y, kRow.z, kRow.x);
    kRotation.SetRow(2, kRow);
    kCamPos = *pPlayer->GetCameraPosition();
    kTo = kRotation * kTo + kCamPos;
    pPlayer->GetCollisionFilter(&kPlayerFilter.iFilter);
    kPickFilter.iFilter = 0;
    kPickFilter.SetLayer(L_PROJECTILE);
    pPlayer->GetCollisionFilter(&kGroupFilter.iFilter);
    uint32_t uiGroup = kGroupFilter.GetGroup();
    kPickFilter.SetGroup(uiGroup);
    kPickData.SetFilter(kPickFilter);
    kPickData.NiSetFrom(kCamPos);
    kPickData.NiSetTo(kTo);
    NiNode* pPicked = static_cast<NiNode*>(TES::pInstance->Pick(kPickData));
    TESObjectREFR* pRef = pPicked ? TESObjectREFR::FindReferenceFor3D(pPicked) : nullptr;
    if (pRef) {
        uint32_t uiID = pRef->iFormID;
        Script::PutNumericIDInDouble(uiID, afReturnValue);
    }
    return true;
}

bool Script::GetDLCCountFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            0
        )) {
        char cMessage[68];
        BSSimpleList<BSContentPackage*>* pValidDLC = &BSSystemUtility::QInstance().QDLCUtility().m_validDLC;
        if (pValidDLC) {
            // The count is converted to double as unsigned.
            afReturnValue = static_cast<double>(static_cast<uint32_t>(pValidDLC->ItemsInList()));
        }
        else {
            afReturnValue = 0.0;
        }
        // The double is truncated to a 64-bit integer and only its low 32 bits are passed.
        BSsprintf(
            cMessage,
            64,
            "There are %u DLC packages available",
            static_cast<uint32_t>(static_cast<int64_t>(afReturnValue))
        );
        Interface::PrintLine(cMessage);
        return true;
    }

    afReturnValue = 0.0;
    return false;
}

// The IsDLCInstalled script command: looks for a valid DLC package whose file name matches the parameter.
bool Script::IsDLCInstalledFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char acName[512];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            acName
        )) {
        afReturnValue = 0.0;
        return false;
    }
    char acBuffer[200];
    BSSimpleList<BSContentPackage*>* pNode = &BSSystemUtility::QInstance().QDLCUtility().m_validDLC;
    BSsprintf(acBuffer, 200, "Checking for DLC package '%s'", acName);
    while (pNode) {
        BSContentPackage* pPackage = pNode->m_item;
        if (pPackage) {
            BSsprintf(acBuffer, 200, "     Examining '%S' [%s]", pPackage->DisplayName(), pPackage->FileName());
            size_t stConverted = 0;
            rt::wcstombs_s(&stConverted, acBuffer, 200, pPackage->DisplayName(), 200);
            if (!BSstricmp(acBuffer, acName)) {
                afReturnValue = 1.0;
                return true;
            }
        }
        pNode = pNode->GetNext();
    }
    afReturnValue = 0.0;
    return true;
}

bool Cmd_GetContainerInventoryCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        afReturnValue = static_cast<double>(apRefObject->GetInventoryCount(false, true));
        if (Script::bSystemOutput) {
            Interface::PrintLine("GetContainerInventoryCount >> %i", static_cast<int32_t>(afReturnValue));
        }
    }
    return true;
}

bool Cmd_SystemSave(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (bAllowScriptedAutosave.Bool()) {
        pSaveLoadManager->SetShouldSystemSave();
    }
    return true;
}

MobDialogueFlagResetter::MobDialogueFlagResetter() : MobIterOperator() {}

void MobDialogueFlagResetter::Operate(MobileObject* apMob) {
    if (apMob->IsActor()) {
        BaseProcess* pProcess = apMob->pCurrentProcess;
        if (pProcess) {
            pProcess->SetGreetingFlag(false);
        }
    }
    apMob->m_Extra.RemoveExtra(115);
}

bool ActorCause::IsValid() {
    return pActor != nullptr;
}

bool BGSProjectile::GetHasExplosionDetonates() {
    return IsFlagSet(EXPLOSION_DETONATES);
}

const char* BSFaceGenManager::GetFaceGenExpressionText(BS_FACEGEN_EXPRESSION aeWhich) {
    return pFaceGenExpressionText[aeWhich];
}

const char* BSFaceGenManager::GetFaceGenModifierText(BS_FACEGEN_MODIFIER aeWhich) {
    return pFaceGenModifierText[aeWhich];
}

const char* BSFaceGenManager::GetFaceGenPhonemeText(BS_FACEGEN_PHONEME aeWhich) {
    return pFaceGenPhonemeText[aeWhich];
}

void PathingRequest::SetDestination(const PathingLocation& aDestination) {
    Destination = aDestination;
}

void Renderer::SetDoCanopyShadowPass(bool abVal) {
    Renderer::bDoCanopyShadowPass = abVal;
}

void Script::SetScaleBase(TESObjectREFR* apRefObject, float afValue) {
    if (apRefObject) {
        apRefObject->SetScale(afValue);
        apRefObject->SetAltered(true);
    }
}

void Script::DamageObjectBase(TESObjectREFR* apRef, float afValue) {
    apRef->DamageObject(afValue, true);
}

void Script::MatchFaceGeometryBase(TESObjectREFR* apRef, TESObjectREFR* apMatchActor, int32_t aiValue) {
    if (apRef && apMatchActor) {
        float fVal = static_cast<double>(aiValue) / 100.0;
        TESNPC* pNPCToMatch = dynamic_cast<TESNPC*>(apMatchActor->GetObjectReference());
        TESNPC* pNPCToChange = dynamic_cast<TESNPC*>(apRef->GetObjectReference());
        FR2MatrixVTC<float> kBlendedCoord[2][2];
        FR2MatrixVTC<float> kChargenOffset[2][2];
        BSSimpleArray<TESNPC*, 1024> kPresetHeads;
        TESActorBase::SEX eSex = pNPCToMatch->GetSex();
        TESRace* pMatchRace = pNPCToMatch->GetRace();
        TESNPC::FillCharGenPresetList(pMatchRace, eSex, &kPresetHeads);
        TESNPC* pPresetNPC = nullptr;
        if (kPresetHeads.GetAt(0)) {
            pPresetNPC = kPresetHeads.GetAt(0);
        }
        FR2MatrixVTC<float>(*pCoord)[2][2] = pNPCToChange->pAlternateFaceOffsetCoord;
        if (pCoord) {
            delete[] pCoord;
            pCoord = nullptr;
            pNPCToChange->pAlternateFaceOffsetCoord = nullptr;
        }
        FR2MatrixVTC<float>(*pCoordToChange)[2][2] = pNPCToChange->GetOffsetFaceCoord();
        FR2MatrixVTC<float>(*pCoordToMatch)[2][2] = pNPCToMatch->GetOffsetFaceCoord();
        FR2MatrixVTC<float>(*pPresetOffsetCoord)[2][2] = pPresetNPC->GetOffsetFaceCoord();
        BSFaceGenManager::OffsetFaceGenCoord(pPresetOffsetCoord, pCoordToMatch, &kChargenOffset, false);
        float fValOldGeo = BSFaceGenManager::GetFaceCoordAttribute(*pCoordToChange, FANATT_AGE, FANTYPE_GEO);
        BSFaceGenManager::InitFaceGenCoord(&kBlendedCoord);
        BSFaceGenManager::ScaleFaceCoord(fVal, &kChargenOffset, true);
        BSFaceGenManager::MergeFaceGenCoord(pCoordToChange, &kChargenOffset, &kBlendedCoord, true, 0.0f);
        BSFaceGenManager::SetFaceCoordAttribute(kBlendedCoord, FANATT_AGE, FANTYPE_GEO, fValOldGeo);
        pCoord = new FR2MatrixVTC<float>[1][2][2];
        BSFaceGenManager::InitFaceGenCoord(pCoord);
        TESRace* pChangeRace = pNPCToChange->GetRace();
        BSFaceGenManager::OffsetFaceGenCoord(
            pChangeRace->GetMeanFaceCoord(TESActorBase::SEX_MALE),
            &kBlendedCoord,
            pCoord,
            false
        );
        BSFaceGenManager::CopyFaceGenCoord(pCoordToChange, pCoord, false, true);
        pNPCToChange->pAlternateFaceOffsetCoord = pCoord;
        pNPCToChange->AddChange(CHANGE_NPC_FACE);
        if (apRef->Get3D()) {
            static_cast<Character*>(apRef)->Character::Reset3D();
        }
    }
}

bool Script::ParseString(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue,
    uint32_t* auiOffset,
    const char* apText,
    BSStringT<char>* apOut
) {
    double dArgs[10];
    // The compiled script data stores the argument count as a 16-bit word.
    int16_t sArgCount = *reinterpret_cast<const uint16_t*>(&apCompiledParams[*auiOffset]);
    *auiOffset += 2;
    ourMemset(dArgs, 0, sizeof(dArgs));
    for (int16_t i = 0; i < sArgCount; ++i) {
        dArgs[i] = 0.0;
        if (!Script::GetValue(
                dArgs[i],
                apCompiledParams,
                *auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                true
            )) {
            return false;
        }
    }
    apOut->SPrintF(
        apText,
        dArgs[0],
        dArgs[1],
        dArgs[2],
        dArgs[3],
        dArgs[4],
        dArgs[5],
        dArgs[6],
        dArgs[7],
        dArgs[8],
        dArgs[9]
    );
    return true;
}

// Prints the HDR image space parameters to the console. Every float is passed as a double, including the blur pass
// count its format prints with %d, and the blur radius has no argument.
bool Script::PrintHDRParamFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ImageSpaceManager* pManager = BSShaderManager::GetImageSpaceManager();
    Interface::PrintLine<>("Current HDR Params:");
    Interface::PrintLine<>("SISG:");
    Interface::PrintLine("    iNumBlurpasses: %d fBlurRadius: %f", pManager->GetNumBlurPasses());
    double dBrightScale = pManager->GetHDRBrightScale();
    double dBrightClamp = pManager->GetHDRBrightClamp();
    Interface::PrintLine("    fBrightClamp: %f fBrightScale: %f", dBrightClamp, dBrightScale);
    Interface::PrintLine<>("SSP:");
    Interface::PrintLine(
        "    fSunlightDimmer: %f Lum Ramp: %f",
        static_cast<double>(BSShaderManager::fSunlightDimmer),
        static_cast<double>(SkyShader::pLUMRamp[0])
    );
    Interface::PrintLine<>("SHP:");
    double dEmissiveHDRMult = pManager->GetEmissiveHDRMult();
    double dEyeAdaptSpeed = pManager->GetHDREyeAdaptSpeed();
    Interface::PrintLine("    fEyeAdaptSpeed: %f fEmissiveHDRMult: %f", dEyeAdaptSpeed, dEmissiveHDRMult);
    Interface::PrintLine(
        "    fTreeDimmer: %f fGrassDimmer: %f",
        static_cast<double>(BSShaderManager::fTreeDimmer),
        static_cast<double>(BSShaderManager::fGrassDimmer)
    );
    double dTargetLUM = pManager->GetHDRTargetLUM();
    double dUpperLUMClamp = pManager->GetHDRUpperLUMClamp();
    Interface::PrintLine("    fUpperLUMClamp: %f fTargetLUM: %f", dUpperLUMClamp, dTargetLUM);
    return true;
}

// Makes the parsed package the actor's current one: ends its dialogue, swaps out the running package with the
// script events, records where the package starts and refiles a middle-high process.
bool Script::AddScriptPackageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESPackage* pPackage = nullptr;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pPackage
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && pActor->IsinDialogue()) {
        pActor->EndDialogue();
    }
    if (!pActor || !pPackage || !pActor->GetCurrentAIProcess()) {
        return true;
    }
    TESPackage* pOldPackage = pActor->GetCurrentAIProcess()->GetCurrentPackage();
    if (pOldPackage) {
        SetActionFlag(pOldPackage, pActor->GetExtra(), SCRIPT_ACTION_ONPACKAGECHANGE);
        pActor->GetCurrentAIProcess()->ChangePackage(pActor, pOldPackage);
    }
    pActor->GetCurrentAIProcess()->SetPackageDoneOnce(false);
    SetActionFlag(pPackage, pActor->GetExtra(), SCRIPT_ACTION_ONPACKAGESTART);
    pActor->GetCurrentAIProcess()->StartNewPackage(pActor, pPackage);
    if (pPackage->GetProcedureType() == PACKAGE_PROCEDURE_NONE) {
        pPackage->CalculateProcedureType(pActor->GetLinkedRef());
    }
    float fZAngle = pActor->GetAngleOnReference().z;
    const NiPoint3* pLocation = &pActor->GetLocationOnReference();
    TESObjectCELL* pCell = pActor->GetParentCell();
    pActor->SetPackageStartLocation(pActor->GetWorldSpace(), pCell, *pLocation, fZAngle);
    pPackage->SetScriptPackage(true);
    pActor->PutCreatedPackage(pPackage, false, false);
    if (pActor->GetCurrentAIProcess() && pActor->GetCurrentAIProcess()->GetProcessLevel() != PROCESS_TYPE_HIGH) {
        float fTime = static_cast<float>(appCalendar.GetHour() - 1.0);
        pActor->GetCurrentAIProcess()->SetLastTimeProcessed(fTime);
        if (pActor->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH) {
            ReferenceProcessLists.RemoveReference(pActor, pActor->GetCurrentAIProcess()->GetProcessLevel());
            ReferenceProcessLists
                .AddReference(pActor, pActor->GetCurrentAIProcess()->GetProcessLevel(), false, false, nullptr);
        }
    }
    else if (pActor == pPlayer) {
        pPlayer->SetAIControlledPackage(true);
    }
    return true;
}

// How deep Activate runs inside itself on this thread.
static thread_local uint32_t uiActivateRecursionDepth;

bool Script::ActivateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    MobileObject* pActivator = nullptr;
    int32_t iRunOnActivateBlock = 0;
    if (pPlayer == apRefObject) {
        Warning("SCRIPTS: Never have the player character activate something in a script very Bad");
        return false;
    }
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pActivator,
            &iRunOnActivateBlock
        )) {
        return false;
    }
    if (apRefObject) {
        if (uiActivateRecursionDepth > 5) {
            return true;
        }
        ++uiActivateRecursionDepth;
        if (!pActivator) {
            pActivator = static_cast<MobileObject*>(apRefObject->GetActionRef());
        }
        if (pActivator && !pActivator->GetDisabled()) {
            if (iRunOnActivateBlock) {
                apRefObject->Activate(pActivator, false, nullptr, 1);
            }
            else {
                bool bUseDefault = apRefObject->GetAction(OBJECT_ACTION_USE_DEFAULT);
                if (!bUseDefault) {
                    apRefObject->SetAction(OBJECT_ACTION_USE_DEFAULT);
                }
                apRefObject->SetAction(OBJECT_ACTION_ACTIVATE);
                apRefObject->Activate(pActivator, false, nullptr, 1);
                apRefObject->ClearAction(OBJECT_ACTION_ACTIVATE);
                if (!bUseDefault) {
                    apRefObject->ClearAction(OBJECT_ACTION_USE_DEFAULT);
                }
            }
        }

        --uiActivateRecursionDepth;
    }
    return true;
}

bool Script::DuplicateNPCStatsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pSource = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pSource
        )) {
        return false;
    }
    if (apRefObject && apRefObject->IsActor() && pSource) {
        TESNPC* pTarget = dynamic_cast<TESNPC*>(apRefObject->GetObjectReference());
        TESNPC* pSourceNPC = dynamic_cast<TESNPC*>(pSource->GetObjectReference());
        if (pTarget && pSourceNPC) {
            pTarget->SetClass(pSourceNPC->GetClass());
            pTarget->SetLevel(pSourceNPC->GetLevel());
            pTarget->InitValues(false);
        }
    }
    return true;
}

bool Script::GetDisposition(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetDispositionConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::GetPCFactionMurderFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pFaction = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction
        )) {
        return false;
    }
    Script::GetPCFactionMurderConditionFunction(apRefObject, pFaction, nullptr, afReturnValue);
    return true;
}

bool Script::GetPlayerEnemyofFactionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pFaction = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction
        )) {
        return false;
    }
    Script::GetPlayerEnemyofFactionConditionFunction(apRefObject, pFaction, nullptr, afReturnValue);
    return true;
}

bool Script::GetPCFactionAttackFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pFaction = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction
        )) {
        return false;
    }
    Script::GetPCFactionAttackConditionFunction(apRefObject, pFaction, nullptr, afReturnValue);
    return true;
}

bool Script::IsPlayerActionActiveFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pAction = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pAction
           ) &&
        Script::IsPlayerActionActiveConditionFunction(apRefObject, pAction, nullptr, afReturnValue);
}

bool Script::GetTalkingActivatorActorFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pActor = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pActor
           ) &&
        Script::GetTalkingActivatorActorConditionFunction(apRefObject, pActor, nullptr, afReturnValue);
}

bool Script::GetHasNoteFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pNote = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pNote
           ) &&
        Script::GetHasNoteConditionFunction(apRefObject, pNote, nullptr, afReturnValue);
}

bool Script::ExistsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pRef = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pRef
           ) &&
        Script::ExistsConditionFunction(apRefObject, pRef, nullptr, afReturnValue);
}

void Script::MoveToFunctionBase(TESObjectREFR* apMoveRef, TESObjectREFR* apTargetRef, float afX, float afY, float afZ) {
    if ((apMoveRef != pPlayer || !pPlayer->IsDead(false)) && apMoveRef && apTargetRef) {
        NiPoint3 kPosition;
        bool bActor = apMoveRef->IsActor();
        if (bActor) {
            Actor* pAct = static_cast<Actor*>(apMoveRef);
            ReferenceProcessLists.StopCombatandAlarmonActor(static_cast<Actor*>(apMoveRef), false);
            BaseProcess* pCurProc = pAct->GetCurrentAIProcess();
            if (pCurProc && pCurProc->GetSitSleepState()) {
                pAct->GetOutofFurnitureQuick();
            }
        }
        TESObjectCELL* pCell = apTargetRef->GetParentCell();
        TESWorldSpace* pWorld = apTargetRef->GetWorldSpace();
        NiPoint3 kAngle = apTargetRef->GetAngleOnReference();
        const NiPoint3* pLocation = &apTargetRef->GetLocationOnReference();
        kPosition.x = pLocation->x;
        kPosition.y = pLocation->y;
        float fZ = pLocation->z;
        kPosition.x = kPosition.x + afX;
        kPosition.y = kPosition.y + afY;
        kPosition.z = fZ + afZ;
        if (apMoveRef == pPlayer) {
            PLAYER_TARGET_LOC* pLoc;
            pLoc = new PLAYER_TARGET_LOC;
            pLoc->Location = kPosition;
            pLoc->Angle.z = kAngle.z;
            pLoc->Angle.x = kAngle.x;
            pLoc->bResetWeather = true;
            if (pWorld) {
                pLoc->pWorld = pWorld;
            }
            else if (pCell && pCell->GetInterior()) {
                pLoc->pInterior = pCell;
            }
            pLoc->pFurnitureRef = apTargetRef;
            pPlayer->RequestPositionPlayer(pLoc);
            Interface::CloseConsole();
        }
        else {
            bool bMob = apMoveRef->IsMobileObject();
            apMoveRef->SetLocationOnReference(kPosition);
            if (pCell && TES::pInstance->IsCellLoaded(pCell, false)) {
                apMoveRef->SetAngleOnReferenceZ(kAngle.z);
                apMoveRef->SetAngleOnReferenceX(0.0f);
                NiAVObject* p3D = apMoveRef->Get3D();
                if (p3D) {
                    p3D->SetTranslate(apMoveRef->GetLocationOnReference());
                    p3D->SetRotate(apMoveRef->GetOrientation());
                    bhkNiCollisionObject::ResetSim(p3D, true);
                    NiUpdateData kUpdateData(0.0f, false, false);
                    p3D->Update(kUpdateData);
                }
            }
            else if (apMoveRef != pPlayer && bMob) {
                apMoveRef->SetAngleOnReferenceX(std::numeric_limits<float>::max());
            }
            TESObjectREFR::MoveRefToNewSpace(apMoveRef, pCell, pWorld);
            if (apMoveRef->GetParentCell()) {
                TESObjectCELL* pParentCell = apMoveRef->GetParentCell();
                if (TES::pInstance->IsCellLoaded(pParentCell, false)) {
                    apMoveRef->InitHavok();
                }
            }
            if (bMob) {
                bhkCharacterController* pCon = static_cast<MobileObject*>(apMoveRef)->MobileObject::GetCharController();
                if (pCon) {
                    pCon->SetPosition(kPosition);
                }
            }
            if (bActor) {
                Actor* pActor = static_cast<Actor*>(apMoveRef);
                if (pActor->GetCurrentAIProcess()) {
                    BaseProcess* pProcess = pActor->pCurrentProcess;
                    pProcess->ComputeLastTimeProcessed();
                    pActor->StopMoving();
                    pProcess = pActor->pCurrentProcess;
                    pProcess->CheckforNewPackage(pActor, true);
                    pActor->FadeIn();
                    pActor->UpdateAlpha();
                }
                else {
                    Warning("AI: Actor being moved is Disabled he has no process");
                }
            }
        }
        pPlayer->CheckForQuestTargetUpdate(apMoveRef);
    }
}

// Moves the reference to the parsed marker plus an offset, directly on one thread or as a queued script function
// call; a reference that cannot be moved gets a warning unless the script was created in game.
bool Script::MoveToFunction(
    SCRIPT_PARAMETER* apParameters,
    char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue,
    uint32_t* auiOffset
) {
    TESObjectREFR* pTarget = nullptr;
    float fX = 0.0f;
    float fY = 0.0f;
    float fZ = 0.0f;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            *auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTarget,
            &fX,
            &fY,
            &fZ
        )) {
        return false;
    }
    if (apRefObject && !apRefObject->CanBeMoved()) {
        if (bSystemOutput) {
            Interface::PrintLine("%s cannot be moved.", apRefObject->GetFormEditorID());
        }
        if (!pDataHandler->IsFormIDCreated(apScript->GetFormID())) {
            uint32_t uiRefID = apRefObject->GetFormID();
            const char* pRefName = apRefObject->GetFormEditorID();
            uint32_t uiScriptID = apScript->GetFormID();
            const char* pScriptName = apScript->GetFormEditorID();
            Warning(
                "SCRIPTS: Script '%s' (%08X) is attempting to move reference '%s' (%08X), but it cannot be moved",
                pScriptName,
                uiScriptID,
                pRefName,
                uiRefID
            );
        }
        return true;
    }
    if (pTarget) {
        if (static_cast<int32_t>(iNumHWThreads.UInt()) <= 1) {
            MoveToFunctionBase(apRefObject, pTarget, fX, fY, fZ);
        }
        else {
            TaskQueueInterface::GetInstance()->QueueScriptFunctionCall(0x109E, apRefObject, pTarget, fX, fY, fZ);
        }
    }
    return true;
}

// MoveToMarkerFunction, but an actor with a high process fades out on the way.
bool Script::MoveToFadeFunction(
    SCRIPT_PARAMETER* apParameters,
    char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue,
    uint32_t* auiOffset
) {
    TESObjectREFR* pTarget = nullptr;
    float fX = 0.0f;
    float fY = 0.0f;
    float fZ = 0.0f;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            *auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTarget,
            &fX,
            &fY,
            &fZ
        )) {
        return false;
    }
    if (apRefObject && !apRefObject->CanBeMoved()) {
        if (bSystemOutput) {
            Interface::PrintLine("%s cannot be moved.", apRefObject->GetFormEditorID());
        }
        if (!pDataHandler->IsFormIDCreated(apScript->GetFormID())) {
            uint32_t uiRefID = apRefObject->GetFormID();
            const char* pRefName = apRefObject->GetFormEditorID();
            uint32_t uiScriptID = apScript->GetFormID();
            const char* pScriptName = apScript->GetFormEditorID();
            Warning(
                "SCRIPTS: Script '%s' (%08X) is attempting to move reference '%s' (%08X), but it cannot be moved",
                pScriptName,
                uiScriptID,
                pRefName,
                uiRefID
            );
        }
        return true;
    }
    if (!pTarget) {
        return true;
    }
    if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
        TaskQueueInterface::GetInstance()->QueueScriptFunctionCall(0x124F, apRefObject, pTarget, fX, fY, fZ);
    }
    else if (apRefObject->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        if (pActor->GetCurrentAIProcess()) {
            if (pActor->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
                HighProcess* pProcess = static_cast<HighProcess*>(pActor->GetCurrentAIProcess());
                pProcess->FadeOutAndMove(pActor, pTarget, fX, fY, fZ);
            }
            else {
                MoveToFunctionBase(apRefObject, pTarget, fX, fY, fZ);
            }
        }
        else {
            MoveToFunctionBase(apRefObject, pTarget, fX, fY, fZ);
        }
    }
    else {
        MoveToFunctionBase(apRefObject, pTarget, fX, fY, fZ);
    }
    return true;
}

// Puts the actor's current follow, accompany or escort package into its waiting state; an escort also restarts
// its procedure at index 2.
bool Script::WaitFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESPackage* pPackage = nullptr;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pPackage
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (!pActor || !pPackage) {
        return true;
    }
    if (pActor->GetPackageSetAsPcurrent() == pPackage || pActor->GetExtra()->GetPackageExtra() == pPackage) {
        if (pPackage->GetPackType() == PACKAGE_TYPE_ESCORT) {
            pPackage->SetWaiting(true);
            pActor->GetCurrentAIProcess()->SetProcedureIndexRunning(2);
        }
        else if (pPackage->GetPackType() == PACKAGE_TYPE_ACCOMPANY || pPackage->GetPackType() == PACKAGE_TYPE_FOLLOW) {
            pPackage->SetWaiting(true);
        }
        else {
            Warning("PACKAGES: Package %s is not a follow or an escort. ", pPackage->GetFormEditorID());
        }
    }
    else {
        const char* pName = pActor->GetFullName();
        Warning("PACKAGES: Package %s is not  %s current package", pPackage->GetFormEditorID(), pName);
    }
    return true;
}

// Opens the SPECIAL book with the parsed points, or the default 35 when none parse; nothing without a player.
bool Script::ShowSPECIALBookMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer) {
        int32_t iPoints = 0;
        if (ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &iPoints
            )) {
            SPECIALBookMenu::Create(iPoints);
        }
        else {
            SPECIALBookMenu::Create35();
        }
    }
    return true;
}

// Opens the love tester with the parsed points, or its default when none parse; nothing without a player.
bool Script::ShowLoveTesterMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer) {
        int32_t iPoints = 0;
        if (ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &iPoints
            )) {
            LoveTesterMenu::Create(iPoints);
        }
        else {
            LoveTesterMenu::Create();
        }
    }
    return true;
}

// Turns the HUD's automatic objective display on for a positive parameter, off otherwise.
bool Script::AutoDisplayObjectives(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer) {
        int32_t iValue = 0;
        if (ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &iValue
            )) {
            HUDMainMenu::ForceObjectiveReminders(iValue > 0);
        }
    }
    return true;
}

// The parsed container is only read after ParseParameters succeeded.
bool Script::SetMerchantContainer(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pContainer;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pContainer
        )) {
        return false;
    }
    apRefObject->SetMerchantContainer(pContainer);
    return true;
}

// Places the parsed form near the reference with the parsed health percent and returns the placed reference's
// form ID.
bool Script::PlaceAtMeHealthPercentFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    TESForm* pForm = nullptr;
    float fHealthPercent = 1.0f;
    int32_t iCount = 1;
    int32_t iDistance = 0;
    int32_t iDirection = 0;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pForm,
            &fHealthPercent,
            &iCount,
            &iDistance,
            &iDirection
        )) {
        return false;
    }
    TESObjectREFR* pPlaced = PlaceAtMeFunctionBase(
        apRefObject,
        static_cast<TESObject*>(pForm),
        iCount,
        iDistance,
        iDirection,
        fHealthPercent
    );
    if (pPlaced) {
        uint32_t uiFormID = pPlaced->GetFormID();
        PutNumericIDInDouble(uiFormID, afReturnValue);
    }
    return true;
}

// Shows (a nonzero third parameter, once) or hides the parsed objective of an uncompleted quest; a missing quest or
// objective is a warning. Takes the form ID before the editor name for each warning, and on the missing quest path
// still calls both on the null quest.
bool Script::SetObjectiveDisplayedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* pQuest;
    uint32_t uiIndex;
    int32_t iDisplayed;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest,
            &uiIndex,
            &iDisplayed
        )) {
        return false;
    }
    if (pQuest) {
        BGSQuestObjective* pObjective = pQuest->GetObjective(uiIndex);
        if (pObjective) {
            if (!pQuest->GetCompleted()) {
                if (iDisplayed) {
                    if (!pObjective->GetDisplayed()) {
                        pObjective->SetState(QOS_DISPLAYED);
                    }
                }
                else {
                    pObjective->SetState(QOS_DORMANT);
                }
            }
        }
        else {
            uint32_t uiFormID = pQuest->GetFormID();
            const char* pName = pQuest->GetEditorName();
            Warning("Script error: objective %d does not exist in quest %s (%08X)", uiIndex, pName, uiFormID);
        }
    }
    else {
        uint32_t uiFormID = pQuest->GetFormID();
        const char* pName = pQuest->GetEditorName();
        Warning("Script error: quest %s (%08X) does not exist.", pName, uiFormID);
    }
    return true;
}

// Fills the file name with "ArchiveProfile.txt", parses the parameters into it and appends ".txt" when the parsed
// name does not already contain it. Outputs the archive profile stats to that file and reports the outcome.
bool Script::OutputArchiveProfile(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cFileName[512];
    BSstrcpy(cFileName, 512, "ArchiveProfile.txt");
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cFileName
        )) {
        BSstrcpy(cFileName, 512, "ArchiveProfile.txt");
    }
    if (!std::strstr(cFileName, ".txt")) {
        BSstrcat(cFileName, 512, ".txt");
    }
    if (ArchiveManager::OutputProfileStats(cFileName)) {
        Interface::PrintLine<>("Outputting Archive profile to file %s", cFileName);
    }
    else {
        Interface::PrintLine<>("Archive profiling is not enabled");
    }
    return true;
}

// Clamps the parsed alpha to 0..1 and applies it to the reference's AI process, then updates the actor's alpha. The AI
// process is queried twice: once in the guard and once for the setter.
bool Script::SetActorAlpha(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fAlpha = 0.0f;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fAlpha
        )) {
        return false;
    }
    if (!apRefObject) {
        apRefObject = pPlayer;
    }
    PlayerCharacter* pActor = static_cast<PlayerCharacter*>(apRefObject);
    if (!pActor->IsActor() || !pActor->GetCurrentAIProcess()) {
        return true;
    }
    fAlpha = BSMIN(1.0f, BSMAX(0.0f, fAlpha));
    BaseProcess* pProcess = pActor->GetCurrentAIProcess();
    pProcess->SetAlphaMult(fAlpha);
    pActor->Actor::UpdateAlpha();
    return true;
}

// Sets the rigid body mass directly on one thread, or queues the script function call when more than one hardware
// thread is in use.
bool Script::SetRigidBodyMass(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fMass = 0.0f;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fMass
        )) {
        return false;
    }
    if (!apRefObject) {
        return false;
    }
    if (static_cast<int32_t>(iNumHWThreads.UInt()) <= 1) {
        SetRigidBodyMassBase(apRefObject, fMass);
    }
    else {
        TaskQueueInterface::GetInstance()->QueueScriptFunctionCall(4438, apRefObject, fMass);
    }
    return true;
}

// Sets the rigid body mass of apRefObject's physics body.
void Script::SetRigidBodyMassBase(TESObjectREFR* apRefObject, float afValue) {
    bhkWorldNS::ObjectRecData kRecData;
    kRecData.pWorld = nullptr;
    kRecData.eAction = bhkWorldNS::OA_TOTAL;
    kRecData.bRecurse = true;
    kRecData.uDataA[0].f = afValue;
    bhkWorld::DoObjects(apRefObject->Get3D(), kRecData, &SetRBMass);
}

// Reports the reference's full name to the console when the thread's system output is on, then deletes it. Returns true
// only when there was no reference to delete.
bool Script::DeleteReference(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (bSystemOutput) {
        if (apRefObject) {
            Interface::PrintLine<>("Deleting reference %s", apRefObject->GetFullName());
        }
        else {
            Interface::PrintLine<>("No reference to flush");
        }
    }
    if (!apRefObject) {
        return true;
    }
    delete apRefObject;
    return false;
}

// Flips the shadow-casting bit of object type 0..6 in BSShaderManager's mask and echoes the new state to the console
// when the thread's system output is on.
bool Script::ToggleCastShadows(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iObjectType = 0;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iObjectType
        )) {
        return false;
    }
    if (iObjectType >= 0 && iObjectType <= 6) {
        if ((BSShaderManager::uiObjectTypeShadowMask & (1u << iObjectType)) != 0) {
            BSShaderManager::uiObjectTypeShadowMask &= ~(1u << iObjectType);
        }
        else {
            BSShaderManager::uiObjectTypeShadowMask |= 1u << iObjectType;
        }
        if (!bSystemOutput) {
            return true;
        }
        static const char* apObjectTypeNames[7] =
            {"Undetermined", "Architecture", "Furniture", "Actors", "Items", "Misc", "Other"};
        const char* pName = apObjectTypeNames[iObjectType];
        if ((BSShaderManager::uiObjectTypeShadowMask & (1u << iObjectType)) != 0) {
            Interface::PrintLine<>("Shadows %s: %s", pName, "On");
        }
        else {
            Interface::PrintLine<>("Shadows %s: %s", pName, "Off");
        }
        return true;
    }
    if (bSystemOutput) {
        Interface::PrintLine<>("Shadow object type must be [0,6]");
    }
    return true;
}

// Parses a type word, an index and an integer value (hundredths). "expression", "phoneme" and "modifier" print the
// names of their values without an index, else set the indexed one (the others keep their current values); "coord"
// stops (-1) or starts (1) cycling face coordinate iValue, or rebuilds an NPC's head (0); "reset" resets the morphs.
// The by-value copy handed to Interface::PrintLine is released by the callee.
bool Script::ModifyFaceGen(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char acType[512];
    acType[0] = 0;
    std::memset(&acType[1], 0, sizeof(acType) - 1);
    int32_t iIndex = -1;
    int32_t iValue = 0;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            acType,
            &iIndex,
            &iValue
        )) {
        return false;
    }
    if (!acType[0]) {
        Interface::PrintLine<>("expression");
        return true;
    }
    BSFaceGenAnimationData* pData = apRefObject ? apRefObject->GetFaceAnimationData(nullptr) : nullptr;
    if (!pData) {
        return true;
    }
    if (!BSstricmp(acType, "expression")) {
        if (iIndex == -1) {
            alignas(4) char kListMem[sizeof(BSStringT<char>)];
            BSStringT<char>* pList = ::new (kListMem) BSStringT<char>;
            for (int32_t i = 0; i < BS_FG_EXPRESSION_COUNT; ++i) {
                char acLine[512];
                if (i == BS_FG_EXPRESSION_COMBAT_ANGRY) {
                    BSsprintf(
                        acLine,
                        0x200,
                        "%d. %s",
                        i,
                        BSFaceGenManager::GetFaceGenExpressionText(static_cast<BS_FACEGEN_EXPRESSION>(i))
                    );
                }
                else {
                    BSsprintf(
                        acLine,
                        0x200,
                        "%d. %s, ",
                        i,
                        BSFaceGenManager::GetFaceGenExpressionText(static_cast<BS_FACEGEN_EXPRESSION>(i))
                    );
                }
                *pList += acLine;
            }
            // PrintLine takes its string by value: the call makes the copy and PrintLine destroys it.
            Interface::PrintLine(*pList);
            pList->~BSStringT<char>();
        }
        else {
            float fValue = iValue / 100.0;
            pData->SetExpressionTarget(static_cast<BS_FACEGEN_EXPRESSION>(iIndex), fValue);
        }
    }
    else if (!BSstricmp(acType, "phoneme")) {
        if (iIndex == -1) {
            alignas(4) char kListMem[sizeof(BSStringT<char>)];
            BSStringT<char>* pList = ::new (kListMem) BSStringT<char>;
            for (int32_t i = 0; i < BS_FG_PHONEME_COUNT; ++i) {
                char acLine[512];
                if (i == BS_FG_PHONEME_W) {
                    BSsprintf(
                        acLine,
                        0x200,
                        "%d. %s",
                        i,
                        BSFaceGenManager::GetFaceGenPhonemeText(static_cast<BS_FACEGEN_PHONEME>(i))
                    );
                }
                else {
                    BSsprintf(
                        acLine,
                        0x200,
                        "%d. %s, ",
                        i,
                        BSFaceGenManager::GetFaceGenPhonemeText(static_cast<BS_FACEGEN_PHONEME>(i))
                    );
                }
                *pList += acLine;
            }
            // PrintLine takes its string by value: the call makes the copy and PrintLine destroys it.
            Interface::PrintLine(*pList);
            pList->~BSStringT<char>();
        }
        else {
            float afPhonemes[BS_FG_PHONEME_COUNT];
            ourMemset(afPhonemes, 0, sizeof(afPhonemes));
            for (int32_t i = 0; i < BS_FG_PHONEME_COUNT; ++i) {
                if (i == iIndex) {
                    afPhonemes[iIndex] = iValue / 100.0;
                }
                else {
                    afPhonemes[i] = pData->GetPhonemeValue(static_cast<BS_FACEGEN_PHONEME>(i));
                }
            }
            pData->AddPhonemeKeyframe(afPhonemes, 1.0f);
        }
    }
    else if (!BSstricmp(acType, "modifier")) {
        if (iIndex == -1) {
            alignas(4) char kListMem[sizeof(BSStringT<char>)];
            BSStringT<char>* pList = ::new (kListMem) BSStringT<char>;
            for (int32_t i = 0; i < BS_FG_MODIFIER_COUNT; ++i) {
                char acLine[512];
                if (i == BS_FG_MODIFIER_HEAD_YAW) {
                    BSsprintf(
                        acLine,
                        0x200,
                        "%d. %s",
                        i,
                        BSFaceGenManager::GetFaceGenModifierText(static_cast<BS_FACEGEN_MODIFIER>(i))
                    );
                }
                else {
                    BSsprintf(
                        acLine,
                        0x200,
                        "%d. %s, ",
                        i,
                        BSFaceGenManager::GetFaceGenModifierText(static_cast<BS_FACEGEN_MODIFIER>(i))
                    );
                }
                *pList += acLine;
            }
            // PrintLine takes its string by value: the call makes the copy and PrintLine destroys it.
            Interface::PrintLine(*pList);
            pList->~BSStringT<char>();
        }
        else {
            BSFaceGenManager::SetUseProceduralEyeBlinking(false);
            float afModifiers[BS_FG_MODIFIER_COUNT];
            ourMemset(afModifiers, 0, sizeof(afModifiers));
            for (int32_t i = 0; i < BS_FG_MODIFIER_COUNT; ++i) {
                if (i == iIndex) {
                    afModifiers[iIndex] = iValue / 100.0;
                }
                else {
                    afModifiers[i] = pData->GetModifierValue(static_cast<BS_FACEGEN_MODIFIER>(i));
                }
            }
            pData->AddModifiersKeyframe(afModifiers, 1.0f);
        }
    }
    else if (!BSstricmp(acType, "coord")) {
        switch (iIndex) {
            case -1: {
                BSFaceGenNiNode* pNode = apRefObject->GetFaceNodeSkinned(nullptr);
                pNode->pActor = static_cast<Actor*>(apRefObject);
                pNode->bCycleFaceGenCoord = false;
                pNode->SetForceBaseMorphs(false);
                pNode->SetDisableMorphUpdates(false);
                break;
            }
            case 0:
                if (apRefObject && apRefObject->IsNpc()) {
                    Actor* pActor = static_cast<Actor*>(apRefObject);
                    TESNPC* pNPC = static_cast<TESNPC*>(apRefObject->GetObjectReference());
                    NiNode* p3D = apRefObject->Get3D();
                    NiNode* pRoot = p3D ? p3D->IsNode() : nullptr;
                    if (pRoot) {
                        NiDefaultAVObjectPalette* pPalette = nullptr;
                        if (apRefObject->GetAnimation() && apRefObject->GetAnimation()->GetControllerManager()) {
                            pPalette = static_cast<NiDefaultAVObjectPalette*>(
                                apRefObject->GetAnimation()->GetControllerManager()->GetObjectPalette()
                            );
                        }
                        BSFaceGenNiNode* pNode = apRefObject->GetFaceNodeSkinned(nullptr);
                        if (pNode && pNode->GetParent()) {
                            RecurseAndRemoveObjectsFromPalette(pNode, pPalette);
                            pActor->GetCurrentAIProcess()->SetFaceSkinnedNode(nullptr);
                            pNode->GetParent()->DetachChild(pNode);
                        }
                        pNode = apRefObject->GetFaceNodeBiped(nullptr);
                        if (pNode && pNode->GetParent()) {
                            RecurseAndRemoveObjectsFromPalette(pNode, pPalette);
                            pActor->GetCurrentAIProcess()->SetFaceNode(nullptr);
                            pActor->GetCurrentAIProcess()->SetHeadAnims(nullptr);
                            pNode->GetParent()->DetachChild(pNode);
                        }
                    }
                    pActor->GetCurrentAIProcess()->ClearFaceAnimationData();
                    pNPC->ClearHead();
                    BSFaceGenNiNode* pSkinned = nullptr;
                    BSFaceGenNiNode* pBiped = pSkinned;
                    pNPC->InitHead(pBiped, pSkinned);
                    pNPC->AttachHead(apRefObject, apRefObject->GetBiped(), pBiped, pSkinned);
                    alignas(FaceGenNpcData) char kNpcDataMem[sizeof(FaceGenNpcData)];
                    FaceGenNpcData* pNpcData = ::new (kNpcDataMem) FaceGenNpcData;
                    pNPC->GetRace()->GetFaceGenData(pNPC, *pNpcData, TESActorBase::SEX_MALE, false);
                    NiUpdateData kUpdateData(0.0f, false, false);
                    p3D->NiAVObject::Update(kUpdateData);
                    pNpcData->~FaceGenNpcData();
                }
                break;
            case 1: {
                BSFaceGenNiNode* pNode = apRefObject->GetFaceNodeSkinned(nullptr);
                pNode->pActor = static_cast<Actor*>(apRefObject);
                pNode->bCycleFaceGenCoord = true;
                pNode->SetForceBaseMorphs(true);
                pNode->SetDisableMorphUpdates(true);
                BSFaceGenManager::iCycleCtrlID = iValue;
                break;
            }
        }
    }
    else if (!BSstricmp(acType, "reset")) {
        pData->ResetCurrentMorphs(1.0f, true, true, true, true, false);
        BSFaceGenManager::SetUseProceduralEyeBlinking(true);
    }
    return true;
}

// Reports whether the parsed code has already been redeemed on the signed-in profile and echoes the result to the
// console.
bool Script::IsCodeRedeemedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cCode[0x100];
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cCode
        )) {
        return false;
    }
    const bool bRedeemed = XGamerProfile::Instance()->IsCodeRedeemed(cCode);
    char cMessage[200];
    BSsprintf(cMessage, 200, "IsCodeRedeemed [%s] = %s", cCode, bRedeemed ? "Yes" : "No");
    Interface::PrintLine<>(cMessage);
    afReturnValue = bRedeemed ? 1.0 : 0.0;
    return bRedeemed;
}

// Redeems the parsed code on the signed-in profile and reports the result through the return value slot.
bool Script::RedeemCodeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cCode[0x100];
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cCode
        )) {
        return false;
    }
    const uint8_t ucResult = static_cast<uint8_t>(XGamerProfile::Instance()->RedeemCode(cCode));
    const bool bResult = ucResult != 0;
    afReturnValue = bResult ? 1.0 : 0.0;
    return bResult;
}

// The parsed integer is only read after ParseParameters succeeded.
bool Script::SetRadioToDialogueOnlyModeFunction(
    SCRIPT_PARAMETER* apParameters,
    char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue,
    uint32_t* auiOffset
) {
    int32_t iDialogueOnly;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            *auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iDialogueOnly
        )) {
        return false;
    }
    if (iDialogueOnly == 1) {
        FalloutRadio::SetIsDialogueOnlyRadio(true);
    }
    else {
        FalloutRadio::SetIsDialogueOnlyRadio(false);
    }
    return true;
}

// Console command: clamps each given falloff value (-1 keeps it unchanged), turns the angles (degrees, at most 90)
// into cosines and sets the data on the no-lighting shaders under the reference's 3D.
bool Script::SetTargetFalloff(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        // start angle, stop angle, start opacity, stop opacity
        NiColorA kFalloff = NiColorA::WHITE * -1.0f;
        if (!Script::ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &kFalloff.r,
                &kFalloff.g,
                &kFalloff.b,
                &kFalloff.a
            )) {
            return false;
        }
        if (kFalloff.r != -1.0) {
            kFalloff.r = BSMAX(BSMIN(kFalloff.r, 90.0f), 0.0f);
            float fRadians = static_cast<float>(kFalloff.r * 0.01745329238474369);
            kFalloff.r = NiCos(fRadians);
        }
        if (kFalloff.g != -1.0) {
            kFalloff.g = BSMAX(BSMIN(kFalloff.g, 90.0f), 0.0f);
            float fRadians = static_cast<float>(kFalloff.g * 0.01745329238474369);
            kFalloff.g = NiCos(fRadians);
        }
        if (kFalloff.b != -1.0) {
            kFalloff.b = BSMAX(BSMIN(kFalloff.b, 1.0f), 0.0f);
        }
        if (kFalloff.a != -1.0) {
            kFalloff.a = BSMAX(BSMIN(kFalloff.a, 1.0f), 0.0f);
        }
        TESObjectREFR* pRef = apRefObject;
        NiNode* p3D = pRef->Get3D();
        if (p3D) {
            BSShaderNoLightingProperty::SetFalloffRecurse(p3D, kFalloff);
        }
    }
    else {
        if (VerboseOutput()) {
            Interface::PrintLine("No Reference Selected");
        }
    }
    return true;
}

bool Script::SetUFOCamSpeedMult(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fMult;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fMult
        )) {
        return false;
    }
    fUFOCamSpeedMult.Float() = fMult;
    return true;
}

// The by-value copy of the string handed to Interface::PrintLine is released by the callee.
bool Script::SetTreeMipmapBias(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fLODBias = BSShaderManager::fLODTreeMipMapLODBias;
    float fLocalBias = BSShaderManager::fLocalTreeMipMapLODBias;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fLocalBias,
            &fLODBias
        )) {
        return false;
    }
    float fOldLOD = BSShaderManager::fLODTreeMipMapLODBias;
    float fOldLocal = BSShaderManager::fLocalTreeMipMapLODBias;
    alignas(4) char kStringMem[sizeof(BSStringT<char>)];
    BSStringT<char>* pString = ::new (kStringMem) BSStringT<char>;
    pString->SPrintF(
        "Old values:   [ Local Trees: %f | LOD Trees: %f ]",
        static_cast<double>(fOldLocal),
        static_cast<double>(fOldLOD)
    );
    // The by-value argument's copy, which the call makes and the callee destroys.
    Interface::PrintLine(*pString);
    BSShaderManager::fLODTreeMipMapLODBias = fLODBias;
    BSShaderManager::fLocalTreeMipMapLODBias = fLocalBias;
    pString->SPrintF(
        "New values:   [ Local Trees: %f | LOD Trees: %f ]",
        static_cast<double>(fLocalBias),
        static_cast<double>(fLODBias)
    );
    Interface::PrintLine(*pString);
    pString->~BSStringT<char>();
    return true;
}

// Projects a blood (actors) or stone impact decal, of the player's weapon or else the default organic one, from the
// camera onto the reference: skinned or actor targets get a projected decal, others a simple one. Without a reference
// it only looks the current cell up and drops it.
bool Script::AddDecal(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectCELL* pParentCell = nullptr;
    NiCamera* pCamera = Main::GetWorldSceneGraph()->GetCamera();
    DECAL_CREATION_DATA kDecal;
    kDecal.pOrigin = pCamera->GetWorldTranslate();
    if (apRefObject) {
        pParentCell = apRefObject->GetParentCell();
        if (apRefObject->Get3D()) {
            NiNode* p3D = apRefObject->Get3D();
            BGSImpactData* pImpact = nullptr;
            TESObjectWEAP* pWeapon = pPlayer->GetCurrentWeapon();
            if (apRefObject->IsActor()) {
                Actor* pActor = dynamic_cast<Actor*>(apRefObject);
                int32_t iMaterial;
                if (pActor->IsPowerBodyArmorWorn()) {
                    iMaterial = BGSImpactDataSet::IMP_MAT_METAL;
                }
                else {
                    TESActorBaseData* pBaseData = pActor->GetActorBase();
                    iMaterial = pBaseData->GetBloodImpactMaterial();
                }
                BGSImpactDataSet* pSet = pWeapon ? pWeapon->GetImpactDataSet() : nullptr;
                if (pSet) {
                    pImpact = pSet->GetImpactData(static_cast<BGSImpactDataSet::IMPACT_MATERIAL_TYPE>(iMaterial));
                }
                else {
                    pImpact = nullptr;
                }
            }
            else {
                HavokMaterialType eMaterial = BHK_MATERIAL_STONE;
                pImpact = pWeapon ? pWeapon->GetImpactDataForMaterial(eMaterial) : nullptr;
            }
            BGSTextureSet* pTexSet = pImpact ? pImpact->GetDecalTextureSet() : nullptr;
            if (!pTexSet) {
                pImpact = BGSImpactDataSet::GetDefaultImpactDataSet()->GetImpactDataForMaterial(
                    BGSImpactDataSet::IMP_MAT_ORGANIC
                );
                pTexSet = pImpact ? pImpact->GetDecalTextureSet() : nullptr;
            }
            kDecal.pTexSet = pTexSet;
            if (apRefObject->IsActor()) {
                for (int32_t i = 0; i < 15; ++i) {
                    kDecal.iBodyParts |= 1 << (i + 1);
                }
            }
            kDecal.iIndex = -1;
            kDecal.fRotation = RandomFloatTwoPi();
            kDecal.cSubTex = static_cast<char>(std::floor(RandomFloat(0.0f, 4.0f)));
            float fMaxWidth = pImpact->GetDecalMaxWidth();
            float fMinWidth = static_cast<float>(pImpact->GetMinWidth());
            float fSize = RandomFloat(fMinWidth, fMaxWidth);
            kDecal.fDecalHeight = fSize;
            kDecal.fDecalWidth = kDecal.fDecalHeight;
            kDecal.fDecalDepth = 1000.0f;
            kDecal.fDecalEpsilon = 90.0f;
            if (static_cast<NiGeometry*>(static_cast<NiAVObject*>(p3D))->GetSkinInstance() || apRefObject->IsActor()) {
                kDecal.pDirection = pCamera->GetWorldDirection();
                kDecal.pObjRefr = apRefObject;
                pParentCell->AddDecal(kDecal, BGSDecalManager::DECAL_TYPE_PROJECTED, true);
            }
            else {
                kDecal.pDirection = -pCamera->GetWorldDirection();
                kDecal.pAVObj = apRefObject->Get3D();
                pParentCell->AddDecal(kDecal, BGSDecalManager::DECAL_TYPE_SIMPLE, true);
            }
        }
    }
    else if (TES::pInstance->GetInterior()) {
        pParentCell = TES::pInstance->GetCurrentCell();
    }
    else {
        pParentCell = pPlayer->GetParentCell();
    }
    return true;
}

bool Script::GetPosFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cAxis;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &cAxis
        )) {
        return false;
    }
    return Script::GetPosConditionFunction(
        apRefObject,
        reinterpret_cast<void*>(static_cast<intptr_t>(static_cast<int8_t>(cAxis))),
        nullptr,
        afReturnValue
    );
}

bool Script::GetStartingPosFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cAxis;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &cAxis
        )) {
        return false;
    }
    return Script::GetStartingPosConditionFunction(
        apRefObject,
        reinterpret_cast<void*>(static_cast<intptr_t>(static_cast<int8_t>(cAxis))),
        nullptr,
        afReturnValue
    );
}

bool Script::GetAngleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cAxis;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &cAxis
        )) {
        return false;
    }
    return Script::GetAngleConditionFunction(
        apRefObject,
        reinterpret_cast<void*>(static_cast<intptr_t>(static_cast<int8_t>(cAxis))),
        nullptr,
        afReturnValue
    );
}

bool Script::GetStartingAngleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cAxis;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &cAxis
        )) {
        return false;
    }
    return Script::GetStartingAngleConditionFunction(
        apRefObject,
        reinterpret_cast<void*>(static_cast<intptr_t>(static_cast<int8_t>(cAxis))),
        nullptr,
        afReturnValue
    );
}

bool Script::GetIsSex(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iSex;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iSex
        )) {
        return false;
    }
    return Script::GetIsSexConditionFunction(
        apRefObject,
        reinterpret_cast<void*>(static_cast<intptr_t>(iSex)),
        nullptr,
        afReturnValue
    );
}

bool Script::GetPCIsSex(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iSex;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iSex
        )) {
        return false;
    }
    return Script::GetIsSexConditionFunction(
        pPlayer,
        reinterpret_cast<void*>(static_cast<intptr_t>(iSex)),
        nullptr,
        afReturnValue
    );
}

bool Script::GetIsUsedItemType(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint8_t ucType;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &ucType
        )) {
        return false;
    }
    return Script::GetIsUsedItemTypeConditionFunction(
        apRefObject,
        reinterpret_cast<void*>(static_cast<uintptr_t>(ucType)),
        nullptr,
        afReturnValue
    );
}

bool Script::PlayMusicFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    // The Music parameter (SCRIPT_PARAM_MUSIC) parses to a form pointer: 8 bytes (4 in the exe).
    BGSMusicType* pMusicType = nullptr;
    if (Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pMusicType
        )) {
        FalloutAudioMedia::MediaOpen(pMusicType);
        FalloutAudioMedia::MediaPlay();
        FalloutAudioMedia::Update();
    }
    return true;
}

bool Script::PlaceAtMeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    TESForm* pObject = nullptr;
    int32_t iCount = 1;
    int32_t iDistance = 0;
    int32_t iDirection = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pObject,
            &iCount,
            &iDistance,
            &iDirection
        )) {
        return false;
    }
    TESObjectREFR* pPlaced = Script::PlaceAtMeFunctionBase(
        apRefObject,
        static_cast<TESObject*>(pObject),
        iCount,
        iDistance,
        iDirection,
        1.0f
    );
    if (pPlaced) {
        uint32_t uiFormID = pPlaced->GetFormID();
        Script::PutNumericIDInDouble(uiFormID, afReturnValue);
    }
    return true;
}

TESObjectREFR* Script::PlaceAtMeFunctionBase(
    TESObjectREFR* apPlaceAtRef,
    TESObject* apObject,
    int32_t aiCount,
    int32_t aiDistance,
    int32_t aiDirection,
    float afHealthPerc
) {
    alignas(16) NiPoint3 kPoint;
    NiPoint3 kDirection;
    NiMatrix3 kRotMatrix;
    alignas(16) NiPoint3 kPos;
    NiPoint3 kRotation;
    TESObjectREFR* pPlacedRef = nullptr;
    if (apPlaceAtRef && apObject) {
        alignas(16) NiPoint3 kPointArray[9];
        kRotation = apPlaceAtRef->GetAngleOnReference();
        kPos = apPlaceAtRef->GetLocationOnReference();
        if (aiDistance) {
            NiAVObject* pObj3D = apPlaceAtRef->Get3D();
            if (pObj3D) {
                alignas(16) NiPoint3 kMoveVec(0.0f, 0.0f, 0.0f);
                kRotMatrix = pObj3D->m_kLocal.m_Rotate;
                kPos = pObj3D->GetTranslate();
                if (aiDirection < 2 || aiDirection > 3) {
                    kMoveVec.x = kRotMatrix.GetEntry(0, 1u);
                    kMoveVec.y = kRotMatrix.GetEntry(1u, 1u);
                    kMoveVec.z = kRotMatrix.GetEntry(2u, 1u);
                }
                else {
                    kMoveVec.x = kRotMatrix.GetEntry(0, 0);
                    kMoveVec.y = kRotMatrix.GetEntry(1u, 0);
                    kMoveVec.z = kRotMatrix.GetEntry(2u, 0);
                }
                kMoveVec *= static_cast<float>(aiDistance);
                if (aiDirection <= 0 || aiDirection > 2) {
                    kPos += kMoveVec;
                }
                else {
                    kPos -= kMoveVec;
                }
            }
        }
        kPointArray[0] = kPos;
        int32_t iIndex = 1;
        float fAngle = RandomFloat0To1() * 0.7853981852531433;
        for (int32_t i = 0; i < 8; ++i) {
            float fSin = NiSin(fAngle);
            float fCos = NiCos(fAngle);
            NiPoint3* pDirection = ::new (&kDirection) NiPoint3(fCos, fSin, 0.0f);
            NiPoint3* pPoint = ::new (&kPoint) NiPoint3(kPos + *pDirection * 100.0f);
            kPointArray[iIndex++] = *pPoint;
            fAngle = fAngle + 0.7853981852531433;
        }
        if (apObject->GetFormType() == LVLC_ID || apObject->GetFormType() == LVLN_ID) {
            uint32_t uiCalcLevel = apPlaceAtRef->GetCalcLevel(false);
            alignas(16) TESContainer kTempCont;
            // Casts to the leveled creature or the leveled character by form type; both reach their TESLeveledList base
            // without a null test.
            if (apObject->GetFormType() == LVLC_ID) {
                static_cast<TESLevCreature*>(apObject)->CalculateCurrentFormList(uiCalcLevel, aiCount, &kTempCont, 0);
            }
            else {
                static_cast<TESLevCharacter*>(apObject)->CalculateCurrentFormList(uiCalcLevel, aiCount, &kTempCont, 0);
            }
            for (BSSimpleList<ContainerObject*>* pContList = &kTempCont.objectList; pContList && !pContList->IsEmpty();
                 pContList = pContList->m_pkNext) {
                ContainerObject* pCO = pContList->m_item;
                if (pCO && pCO->pObj) {
                    while (pCO->i_count > 0) {
                        TESWorldSpace* pWorld = apPlaceAtRef->GetWorldSpace();
                        TESObjectCELL* pCell = apPlaceAtRef->GetParentCell();
                        pPlacedRef = pDataHandler->CreateReferenceAtLocation(
                            pCO->pObj,
                            kPos,
                            kRotation,
                            pCell,
                            pWorld,
                            nullptr,
                            nullptr,
                            nullptr
                        );
                        if (pPlacedRef && pPlacedRef->IsActor()) {
                            Actor* pActor = static_cast<Actor*>(pPlacedRef);
                            pActor->SetAlpha(0.0f);
                            pActor->FadeIn();
                        }

                        --pCO->i_count;
                    }
                }
            }
        }
        else if (apObject->IsBoundObject()) {
            TESBoundObject* pBoundObj = static_cast<TESBoundObject*>(apObject);
            TESHealthForm* pHealthForm = TESHealthForm::GetFormAsHealthForm(apObject);
            const NiPoint3* pLocation = &apPlaceAtRef->GetLocationOnReference();
            bool bOutsideLoadedArea = !TES::pInstance->IsCoordInLoadedArea(*pLocation);
            iIndex = 0;
            for (int32_t j = 0; j < aiCount; ++j) {
                if (iIndex && !bOutsideLoadedArea && TES::pInstance->IsCoordInLoadedArea(kPointArray[iIndex])) {
                    bhkPickData kPickData;
                    CFilter kFilter(0u);
                    kFilter.SetLayer(L_PATHINGPICK);
                    kFilter.SetGroup(3u);
                    kPickData.SetFilter(kFilter);
                    alignas(16) NiPoint3 kFrom = apPlaceAtRef->GetLocationOnReference();
                    kFrom.z = kFrom.z + 64.0;
                    NiPoint3 kTo = kPointArray[iIndex];
                    kTo.z = kTo.z + 64.0;
                    kPickData.NiSetFrom(kFrom);
                    kPickData.NiSetTo(kTo);
                    TES::pInstance->Pick(kPickData);
                    hkBool bHit = kPickData.hasHit();
                    if (bHit) {
                        NiPoint3 kDelta = kTo - kFrom;
                        kDelta.Unitize();
                        kPos = kTo - kDelta * pBoundObj->GetBoundSize();
                    }
                    else {
                        kPos = kTo;
                    }
                    kPos.z = apPlaceAtRef->GetLocationOnReference().z;
                }
                else {
                    kPos = apPlaceAtRef->GetLocationOnReference();
                }
                TESWorldSpace* pWorld = apPlaceAtRef->GetWorldSpace();
                TESObjectCELL* pCell = apPlaceAtRef->GetParentCell();
                pPlacedRef = pDataHandler->CreateReferenceAtLocation(
                    pBoundObj,
                    kPos,
                    kRotation,
                    pCell,
                    pWorld,
                    nullptr,
                    nullptr,
                    nullptr
                );
                if (!pPlacedRef) {
                    return nullptr;
                }
                if (pPlacedRef->IsActor()) {
                    Actor* pActor = static_cast<Actor*>(pPlacedRef);
                    pActor->SetAlpha(0.0f);
                    pActor->FadeIn();
                }
                if (pHealthForm) {
                    float fHealth = static_cast<float>(pHealthForm->GetFormHealth());
                    pPlacedRef->SetHealth(fHealth * afHealthPerc);
                }
                if (++iIndex == 9) {
                    iIndex = 0;
                }
            }
        }
    }
    return pPlacedRef;
}

bool Script::SetAudioMultiThreading(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iOn;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iOn
        )) {
        return false;
    }
    if (iOn && !BSAudio::QInstance().bMultiThread) {
        BSAudio::QInstance().SetMultiThreaded(true);
    }
    else if (!iOn && BSAudio::QInstance().bMultiThread) {
        BSAudio::QInstance().SetMultiThreaded(false);
    }
    return true;
}

bool Script::EnableLoadingMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iEnable;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iEnable
        )) {
        afReturnValue = 0.0;
        return false;
    }
    if (iEnable > 0) {
        TES::pInstance->ShowLoadingMenu(true, nullptr, true);
    }
    else {
        TES::pInstance->ShowLoadingMenu(false, nullptr, true);
    }
    return true;
}

bool Script::PrintNPCDialog(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ReferenceProcessLists.PrintActorsInDialog();
    return true;
}

bool Script::ToggleMotionBlur(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!apRefObject) {
        return true;
    }
    NiNode* p3D = apRefObject->Get3D();
    if (p3D) {
        BSShaderProperty::ToggleMotionBlurRecurse(p3D);
    }
    return true;
}

bool Script::TogglePrimitivesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TES::pInstance->TogglePrimitives();
    return true;
}

bool Script::ToggleScriptsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    SetProcessScripts(!GetProcessScripts());
    Interface::PrintLine("Script processing %s", GetProcessScripts() ? "enabled." : "disabled.");
    return true;
}

bool Script::ToggleTrees(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BSTreeManager::SetTreesEnabled(!BSTreeManager::GetTreesEnabled());
    Interface::PrintLine("All trees are now %s.", BSTreeManager::GetTreesEnabled() ? "NOT CULLED" : "CULLED");
    return true;
}

bool Script::MenuModeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iMenuMode = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iMenuMode
        )) {
        return false;
    }
    // The condition function's parameter slots are untyped: the menu mode goes in as the slot's integer value.
    return Script::GetMenuModeConditionFunction(
        nullptr,
        reinterpret_cast<void*>(static_cast<intptr_t>(iMenuMode)),
        nullptr,
        afReturnValue
    );
}

// Showing the grass updates it at once around the world camera (looking along the zero vector, update angle 0);
// hiding it releases the shared static maps.
bool Script::ToggleGrassFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    NiNode* pGrassNode = TESTerrainLODManager::GetGrassNode();
    if (!pGrassNode) {
        return true;
    }
    bool bIsDisabled = !static_cast<uint8_t>(pGrassNode->GetAppCulled());
    pGrassNode->SetAppCulled(bIsDisabled);
    bIsToggleGrassUpdate = !bIsDisabled;
    if (bIsDisabled) {
        TallGrassShaderProperty::ClearStaticMaps();
    }
    else {
        NiCamera* pCamera = Main::GetWorldSceneGraph()->GetCamera();
        if (pCamera) {
            TESTerrainLODManager::UpdateGrass(pCamera->GetWorldTranslate(), NiPoint3::ZERO, 0.0f);
        }
    }
    Interface::PrintLine("Grass Display %s", bIsDisabled ? "Disabled." : "Enabled.");
    return true;
}

bool Script::ShowFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fValue = 0.0f;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fValue
        )) {
        return false;
    }
    Interface::PrintLine("Value = %0.4f", fValue);
    return true;
}

bool Script::IsTrespassingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::IsTrespassingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }
    return true;
}

// The condition function's result is dropped: the opcode always succeeds.
bool Script::GetAmountStolenSoldFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::GetAmountStolenSoldConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    return true;
}

bool Script::GetGroupMemberCountFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetGroupMemberCountConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetPlayerTeammateCountFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetPlayerTeammateCountConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::SetCriticalStageFunction(
    SCRIPT_PARAMETER* apParameters,
    char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue,
    uint32_t* auiOffset
) {
    ACTOR_CRITICAL_STAGE eStage = ACS_NONE;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            *auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eStage
        )) {
        return false;
    }
    if (apRefObject && apRefObject->IsActor()) {
        static_cast<Actor*>(apRefObject)->SetCriticalStage(eStage);
    }
    return true;
}

bool Script::GameModeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (Interface::IsInMenuMode() || Main::QFaderToBlackVisible()) {
        afReturnValue = 0.0;
    }
    else {
        afReturnValue = 1.0;
    }
    return true;
}

char Script::ScriptEffectStartFunction(
    SCRIPT_PARAMETER* apParameters,
    char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double* afReturnValue,
    uint32_t* auiOffset
) {
    *afReturnValue = 0.0;
    if (apScriptLocals != nullptr &&
        apScriptLocals->m_pScriptEffectData != nullptr &&
        apScriptLocals->m_pScriptEffectData->m_bScriptEffectStart) {
        *afReturnValue = 1.0;
    }
    return 1;
}

// Both fields of view are clamped by the float BSMIN; an unordered value is kept as parsed by the zero
// test and goes through 1 / -v. Main::GetWorldSceneGraph is called after the SetCameraFOV arguments are pushed.
bool Script::SetCameraFOV(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fMainFOV = 0.0f;
    float fViewModelFOV = 0.0f;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fMainFOV,
            &fViewModelFOV
        )) {
        return false;
    }
    fMainFOV = BSMIN(fMainFOV, 160.0f);
    if (fMainFOV == 0.0) {
        fMainFOV = 75.0f;
    }
    fViewModelFOV = BSMIN(fViewModelFOV, 160.0f);
    if (fViewModelFOV == 0.0) {
        fViewModelFOV = 75.0f;
    }
    double dWorldFOV;
    if (fMainFOV > 0.0) {
        dWorldFOV = fMainFOV;
    }
    else {
        dWorldFOV = 1.0 / -fMainFOV;
    }
    float fWorldFOV = static_cast<float>(dWorldFOV);
    double d1stPersonFOV;
    if (fViewModelFOV > 0.0) {
        d1stPersonFOV = fViewModelFOV;
    }
    else {
        d1stPersonFOV = 1.0 / -fViewModelFOV;
    }
    float f1stPersonFOV = static_cast<float>(d1stPersonFOV);
    Main::GetWorldSceneGraph()->SetCameraFOV(fWorldFOV, false, nullptr, false);
    BSShaderManager::SetFOV(fWorldFOV);
    pPlayer->SetWorldFOV(fWorldFOV);
    pPlayer->Set1stPersonFOV(f1stPersonFOV);
    BSSceneGraph::fDefaultWorldFOV = fWorldFOV;
    BSSceneGraph::fDefault1stPersonFOV = f1stPersonFOV;
    return true;
}

void Script::CheckFlagFunction(
    uint32_t aiFlag,
    TESObjectREFR* apRefObject,
    TESForm* apActionForm,
    ScriptLocals* apScriptLocals,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject != nullptr && apScriptLocals != nullptr && apScriptLocals->CheckActionFlag(apActionForm, aiFlag)) {
        afReturnValue = 1.0;
    }
}

// The handle the audio system builds is copied into the local handle; both are destroyed.
bool Script::PlaySoundFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESSound* pSound = nullptr;
    int32_t iSystemSound = 0;
    BSSoundHandle kHandle;
    if (ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pSound,
            &iSystemSound
        )) {
        uint32_t uiFlags = iSystemSound <= 0 ? BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN
                                             : BSGameSound::SOUND_FLAG_SYSTEM_SOUND;
        uiFlags |= BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IS_2D;
        uint32_t uiFormID = pSound->GetFormID();
        kHandle = BSAudio::QInstance().GetSoundHandleByNumericID(uiFormID, uiFlags);
        kHandle.Play(false);
    }
    return true;
}

bool Script::TestCode(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fNumber = 0.0f;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fNumber
        )) {
        return false;
    }
    float fTNumber = 0.0f;
    float fMNumber = 0.0f;
    BGSSceneInfo::CollectSceneDataForLoadedArea(fTNumber, fMNumber, true, false);
    fMNumber = static_cast<float>(fMNumber / 1048576.0);
    Interface::PrintLine("M# for loaded area = %.0f MB", fMNumber);
    return true;
}

bool Script::QuitGame(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Interface::PrintLine<>("Bye.");
    appMain->QuitGame();
    return true;
}

bool Script::PurgeCellBuffers(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    MemoryLevelManager::MEM_FREE_DATA kFreeData;
    MemoryLevelManager::BeginMemoryFree(&kFreeData, true, true, true);
    TES::pInstance->ClearAllCells(true, false);
    MemoryLevelManager::FreeReleasedObjects(kFreeData.bLockedFaceGen);
    MemoryLevelManager::EndMemoryFree(kFreeData);
    return true;
}

bool Script::SetAllVisible(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iVisible = 0;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iVisible
        )) {
        return false;
    }
    Actor::bAllPointsVisible = iVisible != 0;
    return true;
}

bool Script::SetNoAvoidance(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iNoAvoidance = 0;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iNoAvoidance
        )) {
        return false;
    }
    bhkAvoidBox::bNoAvoidance = iNoAvoidance != 0;
    return true;
}

bool Script::SetEmitterParticleMax(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iMax = 0;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iMax
        )) {
        return false;
    }
    if (iMax < 0 || iMax > 0xFFFF) {
        return false;
    }
    NiPSysEmitterCtlr::SetParticlesLimit(static_cast<uint16_t>(iMax));
    return true;
}

// Returns what the disable or enable call returned.
bool Script::ToggleWaterSystem(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TES::pInstance->GetWaterSystem();
    if (TESWaterSystem::GetWaterEnabled()) {
        Interface::PrintLine<>("Water System Off");
        return TES::pInstance->GetWaterSystem()->DisableWaterSystem(true, false);
    }
    Interface::PrintLine<>("Water System On");
    return TES::pInstance->GetWaterSystem()->EnableWaterSystem();
}

// Sets the scene's complexity and the no-avoidance flag together.
bool Script::SetSceneIsComplex(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iComplex = 0;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iComplex
        )) {
        return false;
    }
    if (iComplex) {
        Main::SetSceneIsComplex(true);
        bhkAvoidBox::bNoAvoidance = true;
    }
    else {
        Main::SetSceneIsComplex(false);
        bhkAvoidBox::bNoAvoidance = false;
    }
    return true;
}

bool Script::ToggleDecalRendering(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (appMain->QDrawWorldAccum()->GetRenderDecals()) {
        Interface::PrintLine<>("Decal Rendering Off");
        appMain->QDrawWorldAccum()->SetRenderDecals(false);
    }
    else {
        Interface::PrintLine<>("Decal Rendering On");
        appMain->QDrawWorldAccum()->SetRenderDecals(true);
    }
    return true;
}

// Opens the tutorial menu on the parsed message.
bool Script::ShowTutorialMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (ReturnTrue()) {
        BGSMessage* pMessage = nullptr;
        ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pMessage
        );
        TutorialMenu::Create(pMessage, false);
    }
    return true;
}

// Without a reference the global grab IK setting flips; with an actor that has a ragdoll controller, that
// controller's grab IK flips. The console line reads the new state back.
bool Script::ToggleGrabIK(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!apRefObject) {
        // Flips the whole setting byte (xor 1), not a 0/1 value.
        bhkRagdollController::SetGrabIK(bhkRagdollController::QGrabIK() ^ 1);
        if (bSystemOutput) {
            const char* pState = bhkRagdollController::QGrabIK() ? "On" : "Off";
            Interface::PrintLine("GrabIK %s", pState);
        }
        return true;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && pActor->pRagdollController) {
        pActor->pRagdollController->SetGrabIKEnable(!pActor->pRagdollController->QGrabIKEnable());
        if (bSystemOutput) {
            // The state text is chosen before the editor ID call.
            const char* pState = pActor->pRagdollController->QGrabIKEnable() ? "On" : "Off";
            Interface::PrintLine("GrabIK on REF %s %s", apRefObject->GetFormEditorID(), pState);
        }
    }
    return true;
}

// Without a reference the global look IK setting flips; with an actor that has a ragdoll controller, that
// controller's look IK flips. The console line reads the new state back.
bool Script::ToggleLookIK(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!apRefObject) {
        // Flips the whole setting byte (xor 1), not a 0/1 value.
        bhkRagdollController::SetLookIK(bhkRagdollController::QLookIK() ^ 1);
        if (bSystemOutput) {
            const char* pState = bhkRagdollController::QLookIK() ? "On" : "Off";
            Interface::PrintLine("LookIK %s", pState);
        }
        return true;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && pActor->pRagdollController) {
        pActor->pRagdollController->SetLookIKEnable(!pActor->pRagdollController->QLookIKEnable());
        if (bSystemOutput) {
            // The state text is chosen before the editor ID call.
            const char* pState = pActor->pRagdollController->QLookIKEnable() ? "On" : "Off";
            Interface::PrintLine("LookIK on REF %s %s", apRefObject->GetFormEditorID(), pState);
        }
    }
    return true;
}

bool Script::ToggleRagdollAnim(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bhkRagdollController::SetRagdollAnim(!bhkRagdollController::QRagdollAnim());
    if (bSystemOutput) {
        if (bhkRagdollController::QRagdollAnim()) {
            Interface::PrintLine("Ragdoll Animation %s", "On");
        }
        else {
            Interface::PrintLine("Ragdoll Animation %s", "Off");
        }
    }
    return true;
}

// With more than one hardware thread the scale is queued as script function 4412 (0x113C, SetScale) on the task
// queue, the float passed as a double; otherwise it is set at once.
bool Script::SetScaleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fScale = 0.0f;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fScale
        )) {
        return false;
    }
    if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
        TaskQueueInterface::GetInstance()->QueueScriptFunctionCall(0x113C, apRefObject, fScale);
    }
    else {
        SetScaleBase(apRefObject, fScale);
    }
    return true;
}

bool Script::SetGlobalTimeMultiplierFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fMult = 1.0f;
    int32_t iIgnoreMult = 0;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fMult,
            &iIgnoreMult
        )) {
        return false;
    }
    appTimer.SetGlobalTimeMultiplier(fMult, true);
    bool bIgnore = iIgnoreMult != 0;
    BSAudioManager::QInstance().SetIgnoreGlobalTimeMultiplier(bIgnore);
    if (bSystemOutput) {
        Interface::PrintLine("Global Time Multiplier >> '%0.2f'", fMult);
    }
    return true;
}

// Plays only when apRefObject has 3D, but reports success whenever the parameters parse.
bool Script::PlaySound3DFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cResult = 0;
    TESSound* pSound = nullptr;
    BSSoundHandle kHandle;
    if (ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pSound
        )) {
        if (apRefObject != nullptr && apRefObject->Get3D() != nullptr) {
            uint32_t uiFormID = pSound->GetFormID();
            kHandle = BSAudio::QInstance().GetSoundHandleByNumericID(
                uiFormID,
                BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT
            );
            const NiPoint3* pLocation = &apRefObject->GetLocationOnReference();
            kHandle.SetPosition(pLocation->x, pLocation->y, pLocation->z);
            kHandle.SetObjectToFollow(apRefObject->Get3D());
            kHandle.Play(false);
        }
        cResult = 1;
    }
    return cResult;
}

bool Script::GetWalkSpeedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject == nullptr) {
        return false;
    }
    return GetWalkSpeedConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::ShowHackingMiniGame(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject != nullptr && apRefObject->GetObjectReference()->GetFormType() == TERM_ID) {
        Interface::QueueMenuCreate(
            Interface::OPEN_HACKING,
            apRefObject,
            nullptr,
            nullptr,
            Interface::CMM_STANDARD,
            false
        );
    }
    else {
        Interface::QueueMenuCreate(Interface::OPEN_HACKING, nullptr, nullptr, nullptr, Interface::CMM_STANDARD, false);
    }
    return true;
}

bool Script::TriggerScreenBlood(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiCount = 0;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiCount
        )) {
        return false;
    }
    ScreenSplatter::Activate(uiCount, 0.0f, SCREENSPLATTER_BLOOD);
    return true;
}

bool Script::GetPlayerName(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Interface::CloseConsole();
    Interface::QueueMenuCreate(
        Interface::OPEN_PLAYER_NAME_ENTRY,
        pPlayer,
        nullptr,
        nullptr,
        Interface::CMM_STANDARD,
        false
    );
    return true;
}

bool Script::ToggleShadowVolumesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint8_t bVisible = !BSShaderManager::GetVisibleShadowVolumes();
    BSShaderManager::SetVisibleShadowVolumes(bVisible);
    return true;
}

void Script::HandleMessageMenu() {
    Script::m_cButtonValue = Interface::GetMessageMenuResult();
}

bool Script::ToggleFullScreenMotionBlur(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ReturnTrue();
    return true;
}

bool Script::GetButtonPressedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t* pOwner;
    if (!apRefObject || apRefObject->TESForm::GetTemporary()) {
        if (!apScript) {
            return false;
        }
        pOwner = reinterpret_cast<int32_t*>(static_cast<uintptr_t>(apScript->TESForm::GetFormID()));
    }
    else {
        pOwner = reinterpret_cast<int32_t*>(static_cast<uintptr_t>(apRefObject->TESForm::GetFormID()));
    }
    if (Script::m_cButtonValue > -1 &&
        pOwner == reinterpret_cast<int32_t*>(static_cast<uintptr_t>(Script::m_iButtonOwner))) {
        afReturnValue = Script::m_cButtonValue;
        Script::m_cButtonValue = -1;
        Script::m_iButtonOwner = 0;
    }
    else {
        afReturnValue = -1.0;
    }
    return true;
}

bool Script::ToggleCharControllerShapeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!apRefObject) {
        return true;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (!pActor) {
        return true;
    }
    bhkCharacterController* pCharController = pActor->MobileObject::GetCharController();
    if (!pCharController) {
        return true;
    }
    int32_t iShapeType = pCharController->QShapeType();
    if (iShapeType == 0) {
        pCharController->SetShapeType(bhkCharacterController::ST_CONVEX);
    }
    else {
        if (iShapeType == 1) {
            iShapeType = 0;
        }
        pCharController->SetShapeType(static_cast<bhkCharacterController::SHAPE_TYPES>(iShapeType));
    }
    return true;
}

bool Script::SkipAnimFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!apRefObject) {
        return true;
    }
    if (!apRefObject->GetAnimation()) {
        return true;
    }
    apRefObject->GetAnimation()->SkipUpdate(AGS_WHOLE_BODY);
    return true;
}

bool Script::GetTalkedToPC(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetTalkedToPCConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::ForceRadioStationUpdateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    FalloutRadio::Update(true);
    return true;
}

bool Script::RevertWorld(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    pSaveLoadGame->RevertCurrentChanges(nullptr, nullptr);
    return true;
}

bool Script::ToggleRefractionDebug(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (BSShaderManager::IsRefractionEnabled()) {
        BSShaderAccumulator::ToggleRefractionDebugTexture();
    }
    return true;
}

bool Script::DumpMenuPackingInfo(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Interface::DumpMenuPackingInfo();
    return true;
}

bool Script::GetCurrentAIProcedureFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::GetCurrentAIProcedureConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::ToggleWaterRadius(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TES::pInstance->TES::GetWaterSystem();
    if (TESWaterSystem::GetWaterEnabled()) {
        TESWaterSystem::bDisplayWaterSoundPlacement = true;
    }
    return true;
}

bool Script::Reset3DStateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!apRefObject) {
        return true;
    }
    apRefObject->ResetAnimation_Old();
    apRefObject->TESObjectREFR::GetExtra()->RemoveSavedAnimation();
    apRefObject->TESObjectREFR::GetExtra()->RemoveSavedHavokData();
    return true;
}

bool Script::ClearActorsFactionsPlayerEnemyFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!apRefObject) {
        return true;
    }
    if (apRefObject->IsActor()) {
        static_cast<Actor*>(apRefObject)->RemoveEnemyFlagFromAllFactions();
    }
    return true;
}

bool Script::GetActorCrimePlayerEnemyFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetActorCrimePlayerEnemyConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetActorsInHighFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetActorsInHighConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::ShowRenderPasses(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BSShaderAccumulator::bRequestRenderPassesScenegraph = true;
    return true;
}

bool Script::TriggerLODApocalypseFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    pPlayer->SetMegatonDestroyed(true);
    if (TES::pInstance->GetWorldSpace()) {
        TESWorldSpace* pWorldSpace = TES::pInstance->GetWorldSpace();
        if (pWorldSpace->GetTerrainManager()) {
            TES::pInstance->GetWorldSpace()->GetTerrainManager()->TriggerApocalypse();
        }
    }
    return true;
}

bool Script::ToggleVerboseFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int8_t bIsVerboseMessages = CombatController::GetShowVerboseMessages();
    CombatController::SetShowVerboseMessages(bIsVerboseMessages == 0);
    if (CombatController::GetShowVerboseMessages()) {
        Interface::PrintLine("Verbose messages %s", "shown.");
    }
    else {
        Interface::PrintLine("Verbose messages %s", "hidden.");
    }
    return true;
}

bool Script::ToggleDemigodModeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int8_t bDemigodeMode = PlayerCharacter::IsDemigodMode();
    PlayerCharacter::SetDemigodMode(bDemigodeMode == 0);
    PlayerCharacter::SetGodMode(bDemigodeMode == 0);
    if (PlayerCharacter::IsDemigodMode()) {
        Interface::PrintLine("Demigod Mode %s", "enabled.");
    }
    else {
        Interface::PrintLine("Demigod Mode %s", "disabled.");
    }
    return true;
}

bool Script::ToggleSafeZoneFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Interface::ToggleSafeZone(2);
    return true;
}

bool Script::TestDegradeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iReachable = 0;
    return Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &iReachable
    );
}

bool Script::HasLoaded3DFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::HasLoaded3DConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetKillingBlowLimbFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetKillingBlowLimbConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::HasFriendDispositionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::HasFriendDispositionConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetScaleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetScaleConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::ScriptEffectUpdateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    if (!apScript) {
        return true;
    }
    if (!apScript->GetHeader()) {
        return true;
    }
    if (apScript->GetHeader()->bIsMagicEffectScript) {
        afReturnValue = 1.0;
    }
    return true;
}

bool Script::ScriptEffectElapsedTimeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    if (apScriptLocals && apScriptLocals->m_pScriptEffectData) {
        afReturnValue = apScriptLocals->m_pScriptEffectData->m_fSecondsElapsed;
    }

    return true;
}

bool Script::GetDisease(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetDiseaseConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetVampire(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetVampireConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetClothingValue(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetClothingValueConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetDead(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetDeadConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetGold(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetGoldConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetSleeping(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetSleepingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetSitting(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetSittingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetFurnitureMarkerID(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetFurnitureMarkerIDConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetAlarmed(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetAlarmedConditionFunction(static_cast<Actor*>(apRefObject), nullptr, nullptr, afReturnValue);
}

bool Script::GetIsPleasant(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetIsPleasantConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetIsRaining(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetIsRainingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetIsSnowing(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetIsSnowingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetWindSpeedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetWindSpeedConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetIsCreature(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetIsCreatureConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetIsChild(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetIsChildConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetUsedItemLevelFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetUsedItemLevelConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetUsedItemActivateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetUsedItemActivateConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetRandomPercent(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetRandomPercentConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetLevel(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetLevelConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetArmorRating(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetArmorRatingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::IsGuard(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::IsGuardConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return false;
}

bool Script::IsTalkingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::IsTalkingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return false;
}

bool Script::GetTrespassWarningLevelFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::GetTrespassWarningLevelConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::IsInMyOwnedCellFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::IsInMyOwnedCellConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::IsContinuingPackagePCNearFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::IsContinuingPackagePCNearConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::CanHaveFlamesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::CanHaveFlamesConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::HasFlamesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::HasFlamesConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::GetOpenStateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::GetOpenStateConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::GetDayOfWeekFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetDayOfWeekConditionFunction(nullptr, nullptr, nullptr, afReturnValue);
}

bool Script::IsPCSleepingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsPCSleepingConditionFunction(nullptr, nullptr, nullptr, afReturnValue);
}

bool Script::IsPCAMurdererFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsPCAMurdererConditionFunction(nullptr, nullptr, nullptr, afReturnValue);
}

bool Script::IsSwimming(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::IsSwimmingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return false;
}

bool Script::GetPlayerSleepHours(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = pPlayer->GetSleepHours();
    return true;
}

bool Script::GetDestroyedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::GetDestroyedConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    return true;
}

bool Script::IsActorEvilFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsActorEvilConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::IsActorVictimFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsActorVictimConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetIdleDoneOnceFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetIdleDoneOnceConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetNoRumorsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int8_t bNoRumors = Script::GetNoRumorsConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (bSystemOutput) {
        Interface::PrintLine("GetNoRumors >> %0.2f", afReturnValue);
    }
    return bNoRumors;
}

bool Script::GetWhichServiceFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int8_t bResult = Script::GetWhichServiceConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (bSystemOutput) {
        Interface::PrintLine("GetWhichSerivce >> %0.2f", afReturnValue);
    }
    return bResult;
}

bool Script::IsActorRidingHorseFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int8_t bIsRidingHorse = Script::IsActorRidingHorseConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (bSystemOutput) {
        Interface::PrintLine("Is actor riding horse >> %0.2f", afReturnValue);
    }
    return bIsRidingHorse;
}

bool Script::IsInDangerousWaterFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int8_t bResult = Script::IsInDangerousWaterConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (bSystemOutput) {
        Interface::PrintLine("Actor is in lava >> %0.2f", afReturnValue);
    }
    return bResult;
}

bool Script::GetLastPlayerActionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetLastPlayerActionConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetBroadcastState(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = FalloutRadio::GetBroadcastState(apRefObject) ? 1.0 : 0.0;
    if (bSystemOutput) {
        Interface::PrintLine("GetBroadcastState >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::SetBroadcastState(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iState = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iState
        )) {
        return false;
    }
    FalloutRadio::SetBroadcastState(apRefObject, iState > 0);
    return true;
}

bool Script::IsActorTalkingThroughActivatorFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsActorTalkingThroughActivatorConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::RemoveMerchantContainer(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    apRefObject->SetMerchantContainer(nullptr);
    return true;
}

bool Script::ClearLocationMarkersFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer->TESObjectREFR::GetParentCell()) {
        TESObjectCELL* pParentCell = pPlayer->TESObjectREFR::GetParentCell();
        pParentCell->ClearLocationMarkers();
    }
    return true;
}

bool Script::GetDialogueEmotionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetDialogueEmotionConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetDialogueEmotionValueFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetDialogueEmotionValueConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::IsWaterObjectFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsWaterObjectConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetActorFactionPlayerEnemyFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetActorFactionPlayerEnemyConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetPlayerGrabbedRefFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    uint32_t uiFormID = 0;
    if (pPlayer->GetGrabObject()) {
        TESObjectREFR* pGrabObject = pPlayer->GetGrabObject();
        uiFormID = pGrabObject->TESForm::GetFormID();
        Script::PutNumericIDInDouble(uiFormID, afReturnValue);
    }
    if (bSystemOutput) {
        Interface::PrintLine("GetPlayerGrabbedRef >> (%08x)", uiFormID);
    }
    return true;
}

bool Script::GetDestructionStageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetDestructionStageConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetRadiationLevelFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetRadiationLevelConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetMapMarkerVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetMapMarkerVisibleConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetWeaponHealthPercFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetWeaponHealthPercConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::ClearNoActivationSound(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    FalloutAudio::ClearNoActivationSound();
    return true;
}

bool Script::IsGreetingPlayerFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsGreetingPlayerConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetIgnoreCrimeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetIgnoreCrimeConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::ResetPipboyManagerFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    FOPipboyManager* pPipboyManager = Interface::GetPipboy();
    if (pPipboyManager) {
        pPipboyManager->SetPipBoyManagerReset(true);
    }
    return true;
}

bool Script::ForceTerminalBackFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TileMenu* pTileMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(Interface::eComputersMenu));
    ComputersMenu* pMenu = nullptr;
    if (pTileMenu) {
        pMenu = static_cast<ComputersMenu*>(pTileMenu->Tile::GetMenu());
    }
    if (pMenu) {
        pMenu->DisplayPreviousTerminal();
    }
    return true;
}

bool Script::RadioShutOffFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    FalloutRadio::PipboyRadioEnable(false);
    return true;
}

bool Script::GetCurrentTimeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetCurrentTimeConditionFunction(nullptr, nullptr, nullptr, afReturnValue);
}

bool Script::GetUnconsciousFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetUnconsciousConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::ResetInventoryFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        apRefObject->ResetInventory(false);
    }
    return true;
}

bool Script::AddFlamesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject && apRefObject->Get3D()) {
        NiNode* p3D = apRefObject->Get3D();
        NiNode* pNode = p3D ? p3D->IsNode() : nullptr;
        afReturnValue = TESObjectREFR::AddAddonNodes(pNode);
        if (0.0 != afReturnValue) {
            pNode->UpdateProperties();
            NiUpdateData kUpdateData(0.0, true, false);
            pNode->Update(kUpdateData);
        }
        if (bSystemOutput) {
            Interface::PrintLine("AddFlames >> %0.2f", afReturnValue);
        }
    }

    return true;
}

bool Script::PlayBinkFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int8_t cFileName[512];
    int32_t iAllowInterruption = 0;
    int32_t bFadeAudio = 1;
    int32_t iSkippable = 1;
    int32_t iLetterBoxed = 1;
    uint8_t bMultiThreadedAudio = BSAudio::QInstance().bMultiThread;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cFileName,
            &iAllowInterruption,
            &bFadeAudio,
            &iSkippable,
            &iLetterBoxed
        )) {
        return false;
    }
    if (bFadeAudio) {
        BSAudio::QInstance().Mute(0, 0x1770u);
    }
    else if (!bMultiThreadedAudio) {
        BSAudio::QInstance().SetMultiThreaded(true);
    }
    pIOManager->Pause();
    pMoviePlayer->Play(
        static_cast<const char*>(static_cast<const void*>(cFileName)),
        iAllowInterruption != 0,
        false,
        false,
        false,
        iLetterBoxed != 0,
        false,
        bFadeAudio != 0,
        iSkippable != 0
    );
    pIOManager->Resume();
    if (bFadeAudio) {
        BSAudio::QInstance().UnMute(0);
    }
    else if (!bMultiThreadedAudio) {
        BSAudio::QInstance().SetMultiThreaded(false);
    }
    return true;
}

bool Script::StopCombatAlarmOnActorFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        PlayerCharacter* pActor = static_cast<PlayerCharacter*>(dynamic_cast<Actor*>(apRefObject));
        if (pActor) {
            ReferenceProcessLists.StopCombatandAlarmonActor(pActor, false);
            pActor->StopCombat(nullptr);
        }
    }
    return true;
}

bool Script::GetArmorRatingUpperFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return GetArmorRatingUpperConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::FreezeRendererAccumulation(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Interface::PrintLine("Deprecated, use TFC 2 instead");
    return true;
}

void Script::SetAngleBase(TESObjectREFR* apRefObject, char acAxis, float afValue) {
    if (apRefObject) {
        float fRadians = static_cast<float>(afValue * 0.01745329238474369);
        switch (acAxis) {
            case 88: {
                apRefObject->SetAngleOnReferenceX(fRadians);
                break;
            }
            case 89: {
                apRefObject->SetAngleOnReferenceY(fRadians);
                break;
            }
            case 90: {
                apRefObject->SetAngleOnReferenceZ(fRadians);
                break;
            }
        }
        NiNode* p3D = apRefObject->Get3D();
        if (p3D) {
            p3D->SetRotate(apRefObject->GetOrientation());
            bhkNiCollisionObject::ResetSim(p3D, true);
            if (!apRefObject->GetAnimation()) {
                NiUpdateData kUpdateData(0.0, false, false);
                p3D->Update(kUpdateData);
            }
        }
    }
}

bool Script::GetGroupTargetCountFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetGroupTargetCountConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::ReloadCurrentClimate(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Sky* pSky = Sky::GetInstance();
    TESClimate* pClimate = pSky->Sky::GetCurrentClimate();
    TESFile* pClimateFile = pClimate->GetFile(-1);
    if (pClimateFile) {
        TESFile* pFile = pClimateFile->GetThreadSafeFile();
        pFile->OpenTES(NiFile::READ_ONLY, false);
        pFile->FindForm(pClimate);
        pClimate->ClearData();
        pClimate->Load(pFile);
        pSky = Sky::GetInstance();
        pSky->SetCurrentClimate(pClimate, true);
    }
    return true;
}

bool Script::ForceCloseFile(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int8_t cFileName[516];
    cFileName[0] = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cFileName
        )) {
        return false;
    }
    if (cFileName[0]) {
        TESFile* pListFile = pDataHandler->GetListFile(static_cast<const char*>(static_cast<const void*>(cFileName)));
        if (pListFile) {
            pDataHandler->SetIsClosingFile(true);
            pListFile->CloseTES();
            pDataHandler->SetIsClosingFile(false);
            if (bSystemOutput) {
                Interface::PrintLine("Closed file '%s'.", cFileName);
            }
        }
        else if (bSystemOutput) {
            Interface::PrintLine("ERR: Could not find file '%s'.", cFileName);
        }
    }
    else if (bSystemOutput) {
        Interface::PrintLine("ERR: No Filename.");
    }
    return true;
}

bool Script::GetSecondsPassedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return GetSecondsPassedConditionFunction(nullptr, apScript, nullptr, afReturnValue);
}

bool Script::StopCombatFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject && apRefObject->IsActor()) {
        if (static_cast<Actor*>(apRefObject)->bInCombat && static_cast<Actor*>(apRefObject)->GetCombatController()) {
            static_cast<Actor*>(apRefObject)->GetCombatController()->ForceStopCombat();
        }

        if (apRefObject->TESObjectREFR::GetExtra()) {
            apRefObject->TESObjectREFR::GetExtra()->RemovePlayerCrimeListExtra(static_cast<Actor*>(apRefObject));
        }
    }

    return true;
}

bool Script::RemoveFlamesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject && apRefObject->Get3D()) {
        NiNode* p3D = apRefObject->Get3D();
        if (p3D) {
            afReturnValue = TESObjectREFR::RemoveAddonNodes(p3D->IsNode());
        }
        if (bSystemOutput) {
            Interface::PrintLine("RemoveFlames >> %0.2f", afReturnValue);
        }
    }

    return true;
}

bool Script::RevokeCodeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int8_t cCode[260];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cCode
        )) {
        return false;
    }
    XGamerProfile::Instance()->RevokeCode(static_cast<const char*>(static_cast<const void*>(cCode)));
    return true;
}

void Script::SavePendingReferences(BGSSaveGameBuffer* apBuffer) {
    Script::ScriptRefListCrit.Enter(nullptr);
    uint32_t uiCount = 0;
    uint32_t uiPosition = apBuffer->StartVariableSizedValue();
    for (BSSimpleList<TESObjectREFR*>* pNode = &Script::ReferencesToEnable; pNode; pNode = pNode->m_pkNext) {
        TESForm* pForm = pNode->m_item;
        if (pForm) {
            apBuffer->SaveFormID(pForm, static_cast<uint32_t>(0));
            ++uiCount;
        }
    }
    apBuffer->SaveVariableSizedValue(uiCount, uiPosition);
    uiCount = 0;
    uiPosition = apBuffer->StartVariableSizedValue();
    for (BSSimpleList<TESObjectREFR*>* pNode = &Script::ReferencesToDisable; pNode; pNode = pNode->m_pkNext) {
        TESForm* pForm = pNode->m_item;
        if (pForm) {
            apBuffer->SaveFormID(pForm, static_cast<uint32_t>(0));
            ++uiCount;
        }
    }
    apBuffer->SaveVariableSizedValue(uiCount, uiPosition);
    uiCount = 0;
    uiPosition = apBuffer->StartVariableSizedValue();
    for (BSSimpleList<TESObjectREFR*>* pNode = &Script::ReferencesToDelete; pNode; pNode = pNode->m_pkNext) {
        TESForm* pForm = pNode->m_item;
        if (pForm) {
            apBuffer->SaveFormID(pForm, static_cast<uint32_t>(0));
            ++uiCount;
        }
    }
    apBuffer->SaveVariableSizedValue(uiCount, uiPosition);
    uiCount = 0;
    uiPosition = apBuffer->StartVariableSizedValue();
    for (BSSimpleList<TESObjectREFR*>* pNode = &Script::ReferencesToFadeDisable; pNode; pNode = pNode->m_pkNext) {
        TESForm* pForm = pNode->m_item;
        if (pForm) {
            apBuffer->SaveFormID(pForm, static_cast<uint32_t>(0));
            ++uiCount;
        }
    }
    apBuffer->SaveVariableSizedValue(uiCount, uiPosition);
    Script::ScriptRefListCrit.Leave();
}

bool Script::ToggleOcclusion(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BSShaderManager::bOcclusionQuery = !BSShaderManager::bOcclusionQuery;
    const char* pState = BSShaderManager::bOcclusionQuery ? "on" : "off";
    Interface::PrintLine("Occlusion Query : %s", pState);
    return true;
}

bool Script::GetActorValueInfoFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iIndex = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iIndex
        )) {
        return false;
    }
    if (bSystemOutput &&
        apRefObject &&
        apRefObject->IsActor() &&
        iIndex >= 0 &&
        iIndex < ActorValue::ACTOR_VALUE_COUNT) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        ActorValue::Index eIndex = static_cast<ActorValue::Index>(iIndex);
        // The results kept are rounded to float; the modifiers are passed on unrounded, as doubles.
        float fComputedBase = pActor->GetBaseActorFloatValue(eIndex);
        const char* pFullName = pActor->GetFullName();
        Interface::PrintLine("GetActorValueInfo: %s on %s", ActorValue::GetActorValueScriptName(eIndex), pFullName);
        Interface::PrintLine(
            "...Current Value: %.2f Computed Base: %.2f ",
            pActor->GetActorFloatValue(eIndex),
            fComputedBase
        );
        float fReferenceBase = pActor->GetActorBase()->GetActorFloatValue(eIndex);
        float fDerivedValue = 0.0f;
        bool bIsDerived = ActorValue::GetDerivedActorValue(
            static_cast<ActorValueOwner*>(pActor),
            eIndex,
            fDerivedValue
        );
        bool bOverride = false;
        float fOverride = pActor->GetBaseValueOverride(eIndex, bOverride);
        Interface::PrintLine("...Base Value components:");
        const char* pBaseNote;
        if (pActor->IsAutoCalc()) {
            pBaseNote = "(auto-calculated)";
        }
        else if (pActor->IsCreature()) {
            pBaseNote = "(creature)";
        }
        else {
            pBaseNote = "";
        }
        Interface::PrintLine("......Reference Base Value: %.2f %s", fReferenceBase, pBaseNote);
        if (bIsDerived) {
            if ((!pActor->IsPc() &&
                 !pActor->IsAutoCalc() &&
                 ActorValue::IsFlagSet(eIndex, ActorValue::DERIVED_IF_AUTOCALC)) ||
                pActor->IsCreature()) {
                Interface::PrintLine("......Derived Value: %.2f %s", fDerivedValue, "(Ignored)");
            }
            else {
                Interface::PrintLine("......Derived Value: %.2f %s", fDerivedValue, "");
            }
        }
        if (bOverride) {
            Interface::PrintLine("......SetAV override: %.2f", fOverride);
        }
        // Gets the damage and permanent modifiers before the temporary one.
        double dDamage = pActor->GetDamageModifier(eIndex);
        double dPermanent = pActor->GetPermanentModifier(eIndex);
        Interface::PrintLine(
            "...Modifiers: Temp: %.2f Perm: %.2f Damage: %.2f",
            pActor->GetTemporaryModifier(eIndex),
            dPermanent,
            dDamage
        );
        if (pActor->IsPc()) {
            Interface::PrintLine("...Level-up Value: %.2f", pPlayer->GetPermanentActorFloatValue(eIndex));
        }
    }

    return true;
}

bool Script::SetPosFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cAxis;
    float fPos = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &cAxis,
            &fPos
        )) {
        return false;
    }
    if (!apRefObject || apRefObject->CanBeMoved()) {
        if (iNumHWThreads.Int() <= 1) {
            Script::SetPosBase(apRefObject, cAxis, fPos);
        }
        else {
            TaskQueueInterface::GetInstance()->QueueScriptFunctionCall(0x1007u, apRefObject, cAxis, fPos);
        }
        return true;
    }

    if (bSystemOutput) {
        Interface::PrintLine("%s cannot be moved.", apRefObject->GetFormEditorID());
    }
    if (!pDataHandler->IsFormIDCreated(apScript->iFormID)) {
        int32_t iRefFormID = apRefObject->iFormID;
        const char* pRefName = apRefObject->GetFormEditorID();
        int32_t iScriptFormID = apScript->iFormID;
        Warning(
            "SCRIPTS: Script '%s' (%08X) is attempting to move reference '%s' (%08X), but it cannot be moved",
            apScript->GetFormEditorID(),
            iScriptFormID,
            pRefName,
            iRefFormID
        );
    }
    return true;
}

void Script::SetPosBase(TESObjectREFR* apRefObject, char acAxis, float afValue) {
    NiPoint3 pos;
    if (apRefObject) {
        pos = apRefObject->GetLocationOnReference();
        switch (acAxis) {
            case 88: {
                pos.x = afValue;
                break;
            }
            case 89: {
                pos.y = afValue;
                break;
            }
            case 90: {
                pos.z = afValue;
                break;
            }
        }
        apRefObject->SetLocationOnReference(pos);
        MobileObject* pMob = dynamic_cast<MobileObject*>(apRefObject);
        if (pMob) {
            bhkCharacterController* pCon = pMob->GetCharController();
            if (pCon && !pCon->QIsFlying()) {
                pCon->SetPosition(pos);
            }
        }
        NiAVObject* obj3D = apRefObject->Get3D();
        if (obj3D) {
            obj3D->SetTranslate(pos);
            bhkNiCollisionObject::ResetSim(obj3D, true);
            NiUpdateData kUpdateData(0.0f, false, false);
            obj3D->Update(kUpdateData);
        }
    }
}

bool Script::ShowVarsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cShowGlobals = 1;
    if (apRefObject) {
        if (apRefObject->GetScriptPointer()) {
            Script* pOwnerScript = apRefObject->GetScriptPointer();
            ScriptLocals* pOwnerLocals = apRefObject->GetScriptVariables();
            if (pOwnerScript->GetVariableList()->IsEmpty()) {
                Interface::PrintLine("No variables in script %s", pOwnerScript->GetFormEditorID());
            }
            else {
                BSSimpleList<ScriptVariable*>* pList = pOwnerScript->GetVariableList();
                while (pOwnerLocals && pList && pList->m_item) {
                    ScriptVariable* pVar = pList->m_item;
                    pList = pList->m_pkNext;
                    if (pOwnerLocals->IsReferenceVariable(pVar->data.uiID)) {
                        uint32_t uiID = 0;
                        double dVal = pOwnerLocals->GetVariable(pVar->data.uiID, nullptr);
                        Script::GetNumericIDFromDouble(uiID, dVal);
                        char* pVarName = pVar->cName.pString;
                        Interface::PrintLine("%s->%s = (%08X)", pOwnerScript->GetFormEditorID(), pVarName, uiID);
                    }
                    else if (pVar->data.bIsInteger) {
                        int32_t iValue = static_cast<int32_t>(pOwnerLocals->GetVariable(pVar->data.uiID, nullptr));
                        char* pVarName = pVar->cName.pString;
                        Interface::PrintLine("%s->%s = %d", pOwnerScript->GetFormEditorID(), pVarName, iValue);
                    }
                    else {
                        double dValue = pOwnerLocals->GetVariable(pVar->data.uiID, nullptr);
                        char* pVarName = pVar->cName.pString;
                        Interface::PrintLine("%s->%s = %0.4f", pOwnerScript->GetFormEditorID(), pVarName, dValue);
                    }
                }
            }
            cShowGlobals = 0;
        }
        if (apRefObject->HasContainer()) {
            InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(apRefObject);
            int32_t iCount = pInventory->GetCount(true);
            for (int32_t i = 0; i < iCount; ++i) {
                ItemChange* pItem = pInventory->GetItembyIndex(i);
                if (pItem && pItem->GetScript()) {
                    Script* pOwnerScript = pItem->GetScript();
                    ScriptLocals* pOwnerLocals = pItem->GetScriptLocals();
                    if (pOwnerScript->GetVariableList()->IsEmpty()) {
                        Interface::PrintLine("No variables in script %s", pOwnerScript->GetFormEditorID());
                    }
                    else {
                        BSSimpleList<ScriptVariable*>* pList = pOwnerScript->GetVariableList();
                        while (pOwnerLocals && pList && pList->m_item) {
                            ScriptVariable* pVar = pList->m_item;
                            pList = pList->m_pkNext;
                            if (pOwnerLocals->IsReferenceVariable(pVar->data.uiID)) {
                                uint32_t uiID = 0;
                                double dVal = pOwnerLocals->GetVariable(pVar->data.uiID, nullptr);
                                Script::GetNumericIDFromDouble(uiID, dVal);
                                char* pVarName = pVar->cName.pString;
                                Interface::PrintLine(
                                    "%s->%s = (%08X)",
                                    pOwnerScript->GetFormEditorID(),
                                    pVarName,
                                    uiID
                                );
                            }
                            else if (pVar->data.bIsInteger) {
                                int32_t iValue = static_cast<int32_t>(
                                    pOwnerLocals->GetVariable(pVar->data.uiID, nullptr)
                                );
                                char* pVarName = pVar->cName.pString;
                                Interface::PrintLine("%s->%s = %d", pOwnerScript->GetFormEditorID(), pVarName, iValue);
                            }
                            else {
                                double dValue = pOwnerLocals->GetVariable(pVar->data.uiID, nullptr);
                                char* pVarName = pVar->cName.pString;
                                Interface::PrintLine(
                                    "%s->%s = %0.4f",
                                    pOwnerScript->GetFormEditorID(),
                                    pVarName,
                                    dValue
                                );
                            }
                        }
                    }
                    cShowGlobals = 0;
                }
            }
        }
    }
    if (cShowGlobals) {
        BSSimpleList<TESGlobal*>* pGlobalList = pDataHandler->GetGlobalList();
        if (pGlobalList && !pGlobalList->IsEmpty()) {
            Interface::PrintLine("--- Global Variables -----------------------------");
            while (pGlobalList) {
                TESGlobal* pGlobal = pGlobalList->m_item;
                if (pGlobal) {
                    char cType = pGlobal->GetType();
                    switch (cType) {
                        case 102: {
                            double dValue = pGlobal->fValue;
                            Interface::PrintLine("%s = %0.4f", pGlobal->GetFormEditorID(), dValue);
                            break;
                        }
                        case 108: {
                            int32_t iValue = static_cast<int32_t>(pGlobal->fValue);
                            Interface::PrintLine("%s = %d", pGlobal->GetFormEditorID(), iValue);
                            break;
                        }
                        case 115: {
                            int32_t iValue = static_cast<int16_t>(static_cast<int32_t>(pGlobal->fValue));
                            Interface::PrintLine("%s = %d", pGlobal->GetFormEditorID(), iValue);
                            break;
                        }
                    }
                }
                pGlobalList = pGlobalList->m_pkNext;
            }
        }
    }
    return true;
}

bool Script::StartConversation(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pTarget = nullptr;
    TESTopic* pTopic = nullptr;
    TESObjectREFR* pLocationSpeaker = nullptr;
    TESObjectREFR* pLocationTarget = nullptr;
    int32_t bHeadTrack = 1;
    int32_t bMove = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTarget,
            &pTopic,
            &pLocationSpeaker,
            &pLocationTarget,
            &bHeadTrack,
            &bMove
        )) {
        return false;
    }
    if (apRefObject && pTarget && !apRefObject->GetDisabled() && !pTarget->GetDisabled()) {
        PackageLocation* pLoc1 = nullptr;
        PackageLocation* pLoc2 = nullptr;
        PackageLocation tempFirstLoc;
        PackageLocation tempSecondLoc;
        if (pLocationSpeaker) {
            pLoc1 = &tempFirstLoc;
            tempFirstLoc.SetLocReference(pLocationSpeaker);
        }
        if (pLocationTarget) {
            pLoc2 = &tempSecondLoc;
            tempSecondLoc.SetLocReference(pLocationTarget);
        }
        MobileObject* pActor = nullptr;
        if (apRefObject->IsActor()) {
            pActor = static_cast<MobileObject*>(apRefObject);
        }
        MobileObject* pTargetChar = nullptr;
        if (pTarget->IsActor()) {
            pTargetChar = static_cast<MobileObject*>(pTarget);
        }
        else {
            pTargetChar = pTarget->AddTalkingActivatorMobileObjectREFR();
        }
        if (!pActor ||
            !pTargetChar ||
            pActor->GetDisabled() ||
            pTargetChar->GetDisabled() ||
            pActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
            Script::IsPendingFadeDisabledReference(pActor) ||
            Script::IsPendingFadeDisabledReference(pTargetChar)) {
            apRefObject->Activate(pTarget, false, nullptr, 1);
        }
        else if (pActor == pPlayer) {
            pTargetChar->Activate(pPlayer, false, nullptr, 1);
        }
        else if (pActor->pCurrentProcess && pTargetChar->pCurrentProcess) {
            pActor
                ->InitiateDialogue(pTargetChar, pLoc1, pLoc2, bHeadTrack == 1, bMove == 1, false, pTopic, false, false);
            DialoguePackage* pPack = static_cast<DialoguePackage*>(pActor->GetCurrentPackage());
            if (pPack && pPack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                pPlayer->pAIConversationRunning = pPack;
            }
        }
    }

    return true;
}

bool Script::CompleteAllQuestStages(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bool bWasOn = Script::GetProcessScripts();
    Script::SetProcessScripts(false);
    for (BSSimpleList<TESQuest*>* pList = pDataHandler->GetQuestList(); pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        TESQuest* pQuest = pList->m_item;
        for (BSSimpleList<TESQuestStage*>* pStageList = &pQuest->m_listStages; pStageList && !pStageList->IsEmpty();
             pStageList = pStageList->m_pkNext) {
            pStageList->m_item->SetDone(pQuest, true);
        }
        for (BSSimpleList<BGSQuestObjective*>* pObjList = pQuest->GetObjectiveList(); pObjList && !pObjList->IsEmpty();
             pObjList = pObjList->m_pkNext) {
            BGSQuestObjective* pObj = pObjList->m_item;
            pObj->SetState(QOS_DISPLAYED);
            pObj->SetState(QOS_COMPLETED_DISPLAYED);
        }
    }
    Script::SetProcessScripts(bWasOn);
    if (bSystemOutput) {
        Interface::PrintLine("All Quest Stages Completed.", afReturnValue);
    }
    return true;
}

bool Script::SetObjectiveCompletedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iCompleted;
    uint32_t uiIndex;
    TESQuest* pQuest;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest,
            &uiIndex,
            &iCompleted
        )) {
        return false;
    }
    if (pQuest) {
        BGSQuestObjective* pObjective = pQuest->GetObjective(uiIndex);
        if (pObjective) {
            if (!pQuest->GetCompleted()) {
                if (iCompleted) {
                    if (!pObjective->GetCompleted()) {
                        pObjective->SetState(pObjective->GetDisplayed() ? QOS_COMPLETED_DISPLAYED : QOS_COMPLETED);
                    }
                }
                else {
                    pObjective->SetState(QOS_DORMANT);
                }
            }
        }
        else {
            uint32_t uiFormID = pQuest->iFormID;
            const char* pName = pQuest->GetEditorName();
            Warning("Script error: objective %d does not exist in quest %s (%08X)", uiIndex, pName, uiFormID);
        }
    }
    else {
        // Reads the form ID and calls the editor name getter through the null quest, deliberately.
        uint32_t uiFormID = pQuest->iFormID;
        Warning("Script error: quest %s (%08X) does not exist.", pQuest->GetEditorName(), uiFormID);
    }
    return true;
}

bool Script::SetMPSParticleMax(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiMax = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiMax
        )) {
        return false;
    }
    if (uiMax >= 0x10000) {
        return false;
    }
    BSPSysMultiTargetEmitterCtlr::SetMaxParticlesPerEmitter(uiMax);
    return true;
}

bool Script::GetDistanceFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetDistanceConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::GetInZoneFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pZone = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pZone
           ) &&
        Script::GetInZoneConditionFunction(apRefObject, pZone, nullptr, afReturnValue);
}

bool Script::AddItemFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* pWeap = nullptr;
    int32_t iCount = 0;
    int32_t iSilent = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pWeap,
            &iCount,
            &iSilent
        )) {
        return false;
    }
    if (apRefObject) {
        uint32_t uiCalcLevel = apRefObject->GetCalcLevel(false);
        TESContainer tempCont;
        TESLevItem* pLevItem = nullptr;
        BGSListForm* pFormList = nullptr;
        TESBoundObject* pObject = nullptr;
        bool bWeap = false;
        int32_t iFormType = pWeap->GetFormType();
        if (iFormType == LVLI_ID) {
            pLevItem = static_cast<TESLevItem*>(pWeap);
        }
        else if (iFormType == FLST_ID) {
            pFormList = static_cast<BGSListForm*>(pWeap);
        }
        else {
            if (iFormType == WEAP_ID) {
                bWeap = true;
            }
            if (pWeap->IsBoundObject()) {
                pObject = static_cast<TESBoundObject*>(pWeap);
            }
        }
        if (pLevItem) {
            pLevItem->CalculateCurrentFormList(uiCalcLevel, iCount, &tempCont, 0);
        }
        else if (pFormList && iCount) {
            BSSimpleList<TESForm*>* pList = &pFormList->ListOfForms;
            while (pList && !pList->IsEmpty()) {
                TESForm* pAddForm = pList->m_item;
                pList = pList->m_pkNext;
                if (pAddForm->IsBoundObject()) {
                    tempCont.AddObject(static_cast<TESObject*>(pAddForm), iCount, nullptr);
                }
            }
        }
        else if (pObject && iCount) {
            tempCont.AddObject(pObject, iCount, nullptr);
        }
        else {
            Warning("SCRIPTS: AddItem in script '%s' failed to generate an item.", apScript->GetFormEditorID());
        }
        if (bWeap && static_cast<TESObjectWEAP*>(pWeap)->TESObjectWEAP::GetWeaponType() < WEAPON_ONE_HAND_GRENADE) {
            tempCont.ApplyHealthPercentToContainer(0.999f);
        }
        tempCont.AddContainerToReference(apRefObject, iSilent == 1);
        if (apRefObject->IsActor() && apRefObject != pPlayer && apRefObject->IsNpc()) {
            BipedAnim* pBiped = apRefObject->GetBiped();
            static_cast<TESNPC*>(apRefObject->GetObjectReference())->TESNPC::InitParts(apRefObject, pBiped, true);
        }
        return true;
    }
    return true;
}

bool Script::RemoveItemFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* pFormList = nullptr;
    TESBoundObject* pObject = nullptr;
    int32_t iCount = 0;
    int32_t iSilent = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFormList,
            &iCount,
            &iSilent
        )) {
        return false;
    }
    if (apRefObject && pFormList) {
        BSSimpleList<TESForm*> removeList;
        if (pFormList->IsBoundObject()) {
            pObject = static_cast<TESBoundObject*>(pFormList);
        }
        if (pFormList->GetFormType() == LVLI_ID) {
            TESLevItem* pLeveled = static_cast<TESLevItem*>(pFormList);
            if (pFormList) {
                InventoryChanges* pChanges = apRefObject->m_Extra.GetContainerChanges();
                if (pChanges) {
                    pObject = pChanges->GetLeveledCalculatedBaseObject(pLeveled);
                }
            }
        }
        if (pObject) {
            TESForm* pRemoveForm = pObject;
            removeList.AddHead(pRemoveForm);
        }
        else if (pFormList->GetFormType() == FLST_ID) {
            BSSimpleList<TESForm*>* pList = &static_cast<BGSListForm*>(pFormList)->ListOfForms;
            while (pList && !pList->IsEmpty()) {
                TESForm* pRemoveForm = pList->m_item;
                pList = pList->m_pkNext;
                if (pRemoveForm->IsBoundObject()) {
                    removeList.AddHead(pRemoveForm);
                }
            }
        }
        while (!removeList.IsEmpty()) {
            pObject = static_cast<TESBoundObject*>(removeList.m_item);
            removeList.RemoveHead();
            int32_t iRemoveCount = iCount;
            Actor* pActor = dynamic_cast<Actor*>(apRefObject);
            if (pActor) {
                double dActualCount = static_cast<double>(iRemoveCount);
                Script::GetItemCountConditionFunction(apRefObject, pObject, nullptr, dActualCount);
                if (static_cast<double>(iRemoveCount) > dActualCount) {
                    iRemoveCount = static_cast<int32_t>(dActualCount);
                }
                ExtraDataList* pExtra = nullptr;
                if (pActor->WearingObject(pObject)) {
                    InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(apRefObject);
                    if (pInventory) {
                        pExtra = pInventory->WearingObject(pObject, false);
                        if (pExtra) {
                            pExtra->SetCanNotWear(false);
                        }
                    }
                }
                if (iRemoveCount > 0) {
                    bool bWorn = pActor->WearingObject(pObject);
                    if (bWorn) {
                        pActor->Dispel(pObject, true);
                    }
                    pActor->RemoveItem(
                        pObject,
                        pExtra,
                        iRemoveCount,
                        false,
                        false,
                        nullptr,
                        nullptr,
                        nullptr,
                        true,
                        false
                    );
                    if (pActor == pPlayer) {
                        if (!iSilent) {
                            BSStringT<char> kString;
                            if (iRemoveCount <= 1) {
                                const char* pRemovedText = sRemoveItemfromInventory.String();
                                const char* pName = TESFullName::GetFullName(pObject);
                                kString.SPrintF("%s %s", pName, pRemovedText);
                            }
                            else {
                                const char* pRemovedText = sRemoveItemfromInventory.String();
                                const char* pPlural = sPlural.String();
                                const char* pName = TESFullName::GetFullName(pObject);
                                kString.SPrintF("%i %s%s %s", iRemoveCount, pName, pPlural, pRemovedText);
                            }
                            switch (pObject->GetFormType()) {
                                case ARMO_ID: {
                                    const char* pSoundName = iSilent
                                        ? nullptr
                                        : pPlayer->GetPickUpSoundName(pObject, false, false);
                                    Interface::ShowMessage(
                                        kString.pString,
                                        0,
                                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds",
                                        pSoundName,
                                        2.0f,
                                        false
                                    );
                                    break;
                                }
                                case BOOK_ID:
                                case LIGH_ID:
                                case MISC_ID:
                                case WEAP_ID:
                                case AMMO_ID:
                                case KEYM_ID:
                                case ALCH_ID:
                                case COBJ_ID:
                                case IMOD_ID:
                                case CHIP_ID:
                                case CCRD_ID:
                                case CMNY_ID: {
                                    const char* pSoundName = iSilent
                                        ? nullptr
                                        : pPlayer->GetPickUpSoundName(pObject, false, false);
                                    Interface::ShowMessage(
                                        kString.pString,
                                        0,
                                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds",
                                        pSoundName,
                                        2.0f,
                                        false
                                    );
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                        Interface::UpdateInventory();
                    }
                    else if (bWorn && pActor->pCurrentProcess && pActor->Get3D()) {
                        pActor->pCurrentProcess->ProcessRemoveWorn(pActor, false);
                    }
                }
            }
            else {
                apRefObject
                    ->RemoveItem(pObject, nullptr, iRemoveCount, false, false, nullptr, nullptr, nullptr, true, false);
            }
        }
    }

    return true;
}

bool Script::ToggleDialogSubtitle(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iShow = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iShow
        )) {
        return false;
    }
    ReferenceProcessLists.bShowSubtitle = iShow != 0;
    return true;
}

// The exe's cleanup-only C++ EH frame is left out.
bool Script::OutputMemStats(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char input[512];
    char cSuffix[268];
    input[0] = 0;
    std::memset(input + 1, 0, 511);
    if (Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            input
        )) {
        BSstrcpy(cSuffix, 260, input);
        BSsprintf(input, 512, "MemStats%s.mhd", cSuffix);
    }
    else {
        BSstrcpy(input, 512, "OutputMemStats.mhd");
        // Empty here (a departure): without a parameter nothing writes the suffix, and the exe formats it unset into
        // the model and texture dump names.
        cSuffix[0] = '\0';
    }
    MemoryManagerFile file(input);
    MemoryManager::Instance().OutputDebugInfo(&file, false);
    BSsprintf(input, 512, "ModelDump%s.txt", cSuffix);
    BSFile outfile(input, NiFile::WRITE_ONLY, 0x4000, false);
    outfile.BSFile::Open(false, false);
    if (pModelLoader) {
        pModelLoader->OutputModelMapContents(&outfile, false);
    }
    if (BSFaceGenManager::GetModelCache()) {
        BSFaceGenModelMap* pModelCache = BSFaceGenManager::GetModelCache();
        pModelCache->OutputModelMapContents(&outfile);
    }
    outfile.Close();
    BSsprintf(input, 512, "TextureDump%s.txt", cSuffix);
    BSFile outfile2(input, NiFile::WRITE_ONLY, 0x4000, false);
    outfile2.BSFile::Open(false, false);
    NiPointer<NiTexture> pLargestTex[2048];
    ourMemset(pLargestTex, 0, sizeof(pLargestTex));
    // The textures with renderer data, sorted by size, largest first.
    for (NiTexture* pTex = NiTexture::GetListHead(); pTex; pTex = pTex->m_pkNext) {
        if (pTex->m_pkRendererData) {
            uint32_t uiTexSize = GetTextureDumpSize(pTex);
            int32_t idx = -1;
            for (int32_t i = 0; i < 2048 && idx == -1; ++i) {
                if (pLargestTex[i].m_pObject) {
                    uint32_t uiCompSize = GetTextureDumpSize(pLargestTex[i].m_pObject);
                    if (uiCompSize < uiTexSize) {
                        idx = i;
                    }
                }
                else {
                    idx = i;
                }
            }
            if (idx != -1) {
                for (int32_t i = 2047; i > idx; --i) {
                    pLargestTex[i] = pLargestTex[i - 1];
                }
                pLargestTex[idx] = pTex;
            }
        }
    }
    for (int32_t i = 0; i < 2048; ++i) {
        if (pLargestTex[i].m_pObject) {
            char cTexName[264];
            char cLine[512];
            bool bSystemTex = false;
            if (NiObject::IsKindOf_0(&NiSourceTexture::ms_RTTI, pLargestTex[i].m_pObject)) {
                NiSourceTexture* pSrcTex = static_cast<NiSourceTexture*>(pLargestTex[i].m_pObject);
                const char* pTexName = pSrcTex->GetFilename().GetHandleString();
                if (!pTexName || !*pTexName) {
                    pTexName = pLargestTex[i]->m_kName.GetHandleString();
                }
                if (pTexName && *pTexName) {
                    BSstrcpy(cTexName, 260, pTexName);
                }
                else {
                    BSstrcpy(cTexName, 260, "UNNAMED SOURCE TEXTURE");
                }
            }
            else {
                bSystemTex = true;
                const char* pTexName = pLargestTex[i]->m_kName.GetHandleString();
                if (pTexName && *pTexName) {
                    BSsprintf(cTexName, 260, "RENDERED_TEXTURE (%s)", pTexName);
                }
                else {
                    BSsprintf(cTexName, 260, "RENDERED_TEXTURE (none)");
                }
            }
            BSstrlwr(cTexName, 260);
            const char* pCatText = "SYSTEM";
            if (!bSystemTex) {
                if (std::strstr(cTexName, "\\armor\\") ||
                    std::strstr(cTexName, "\\creatures\\") ||
                    std::strstr(cTexName, "\\characters\\") ||
                    std::strstr(cTexName, "\\pipboy3000\\") ||
                    std::strstr(cTexName, "\\weapons\\") ||
                    std::strstr(cTexName, "\\decals\\") ||
                    std::strstr(cTexName, "\\projectiles\\")) {
                    pCatText = "ACTOR";
                }
                else if (std::strstr(cTexName, "\\fonts\\") || std::strstr(cTexName, "\\interface\\")) {
                    pCatText = "MENUS";
                }
                else if (std::strstr(cTexName, "\\effects\\") || std::strstr(cTexName, "\\gore\\")) {
                    pCatText = "EFFECTS";
                }
                else if (std::strstr(cTexName, "\\sky\\")) {
                    pCatText = "SKY";
                }
                else if (std::strstr(cTexName, "\\lod\\")) {
                    pCatText = "LOD";
                }
                else if (std::strstr(cTexName, "\\water\\")) {
                    pCatText = "WATER";
                }
                else {
                    pCatText = "MISCREF";
                }
            }
            uint32_t uiTexSize = GetTextureDumpSize(pLargestTex[i].m_pObject);
            _D3DFORMAT eFormat = NiDX9PixelFormat::DetermineD3DFormat(pLargestTex[i]->m_pkRendererData->m_kPixelFormat);
            const char* pFormat = NiDX9SystemDesc::GetD3D9FormatString(eFormat);
            NiTexture* pRttiTex = pLargestTex[i].m_pObject;
            const char* pRttiName = pRttiTex->GetRTTI()->m_pcName;
            const char* pQuality = pLargestTex[i]->QState() == PS_DEGRADED ? "L" : "H";
            NiTexture* pHeightTex = pLargestTex[i].m_pObject;
            NiTexture* pWidthTex = pLargestTex[i].m_pObject;
            int32_t iRefCount = pLargestTex[i]->m_uiRefCount - 1;
            uint32_t uiSizeKB = uiTexSize >> 10;
            int32_t iHeight = pHeightTex->GetHeight();
            int32_t iWidth = pWidthTex->GetWidth();
            BSsprintf(
                cLine,
                512,
                "TEXTURES: %d:\t%s\t%s\t%dx%d\t%d\t%s\trefcount %d\t%s\t%s\r\n",
                i + 1,
                pCatText,
                cTexName,
                iWidth,
                iHeight,
                uiSizeKB,
                pFormat,
                iRefCount,
                pQuality,
                pRttiName
            );
            Warning(cLine);
            size_t stLen = std::strlen(cLine);
            outfile2.BSFile::WriteF(cLine, stLen + 1);
        }
    }
    outfile2.Close();
    return true;
}

bool Script::OutputMemStatsFullWalk(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cFileName[516];
    std::memset(cFileName, 0, 512);
    BSstrncpy(cFileName, "OutputMemStats.mhd", 511);
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cFileName
        )) {
        BSstrncpy(cFileName, "OutputMemStats.mhd", 511);
    }
    MemoryManagerFile file(cFileName);
    MemoryManager::Instance().OutputDebugInfo(&file, true);
    return true;
}

bool Script::ToggleMapMarkersFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iTurnOn = 1;
    int32_t iEnableTravel = 1;
    int32_t iShowAll = 1;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iTurnOn,
            &iEnableTravel,
            &iShowAll
        )) {
        return false;
    }
    BSSimpleList<TESWorldSpace*>* pWorldList = pDataHandler->GetWorldSpaceList();
    while (pWorldList && !pWorldList->IsEmpty()) {
        TESWorldSpace* pWorld = pWorldList->m_item;
        pWorldList = pWorldList->m_pkNext;
        BSSimpleList<TESObjectREFR*>* pMarkerList = pWorld->BuildMapMarkerList(false);
        if (pMarkerList) {
            while (!pMarkerList->IsEmpty()) {
                TESObjectREFR* pRef = pMarkerList->m_item;
                pMarkerList->RemoveHead();
                MapMarkerData* pData = pRef->GetMapMarkerData();
                if (!iShowAll || !pData->GetHidden()) {
                    if (iTurnOn) {
                        pData->SetIsVisible(1);
                        if (!pData->GetTravelLoc()) {
                            pData->SetTravelLoc(iEnableTravel != 0);
                        }
                    }
                    else {
                        pData->SetIsVisible(0);
                        pData->SetTravelLoc(false);
                    }
                    pRef->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
                }
            }
            delete pMarkerList;
        }
    }
    if (bSystemOutput) {
        if (iTurnOn <= 0) {
            Interface::PrintLine("All map markers %s", "hidden.");
        }
        else {
            Interface::PrintLine("All map markers %s", "shown.");
        }
    }
    return true;
}

bool Script::ToggleGodModeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bool bGodMode = PlayerCharacter::IsGodMode();
    PlayerCharacter::SetGodMode(!bGodMode);
    if (!PlayerCharacter::IsGodMode() && PlayerCharacter::IsDemigodMode()) {
        PlayerCharacter::SetDemigodMode(false);
        Interface::PrintLine("Demigod Mode disabled");
    }
    const char* pState = PlayerCharacter::IsGodMode() ? "enabled." : "disabled.";
    Interface::PrintLine("God Mode %s", pState);
    return true;
}

bool Script::ToggleEmotionsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bool bUseEmotions = BSFaceGenManager::GetUseEmotions();
    BSFaceGenManager::SetUseEmotions(!bUseEmotions);
    const char* pState = BSFaceGenManager::GetUseEmotions() ? "Enabled." : "Disabled.";
    Interface::PrintLine("NPC Facial Emotions %s", pState);
    return true;
}

bool Script::RemoveImageSpaceModifier(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESImageSpaceModifier* pImageSpace = nullptr;
    float fStrength = 1.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pImageSpace,
            &fStrength
        )) {
        return false;
    }
    ImageSpaceModifierInstanceForm::Stop(pImageSpace);
    return true;
}

bool Script::TestLocalMap(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iFog = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iFog
        )) {
        return false;
    }
    NiNode* pNode = new NiNode(0);
    NiPoint3 playerLoc = pPlayer->GetLocationOnReference();
    playerLoc.z = playerLoc.z + 10.0;
    int32_t iTriSize = 64;
    int32_t iTileSide = 16;
    int32_t iTileCount = 256;
    int32_t iTileSize = 4;
    int32_t iSeenSize = 17;
    uint32_t uiArraySize = 289;
    int32_t iMapSide = uGridsToLoad.Int();
    int32_t iMapArraySize = iMapSide * iMapSide;
    int32_t iMapSize = iMapSide * iTriSize;
    float fPosSize = SeenData::GetSeenPosSize();
    for (int32_t iTri = 0; iTri < iMapArraySize; ++iTri) {
        // The division by the setting's value is unsigned.
        int32_t iSectX = static_cast<uint32_t>(iTri) % static_cast<uint32_t>(uGridsToLoad.Int());
        int32_t iSectY = static_cast<uint32_t>(iTri) / static_cast<uint32_t>(uGridsToLoad.Int());
        NiPointer<NiTexture> spMapTex(static_cast<NiTexture*>(nullptr));
        TESObjectCELL* pSeenCell = nullptr;
        NiPoint3 sectOffset = NiPoint3::ZERO;
        if (pPlayer->GetInterior()) {
            pSeenCell = pPlayer->GetParentCell();
            if (pSeenCell) {
                NiPoint3 playerIntLoc = playerLoc;
                pSeenCell->AdjustCoordForNorthRotation(playerLoc, playerIntLoc, true);
                int32_t iSeenX = (FloatToInt(playerIntLoc.x) - 2048) >> 12;
                int32_t iSeenY = (FloatToInt(playerIntLoc.y) - 2048) >> 12;
                int32_t iIntSectionX = iSectX + iSeenX - (iMapSide >> 1);
                int32_t iIntSectionY = iSectY + iSeenY - (iMapSide >> 1);
                sectOffset = NiPoint3(
                    static_cast<float>((iIntSectionX << 12) + 2048),
                    static_cast<float>((iIntSectionY << 12) + 2048),
                    0.0f
                );
                pSeenCell->GetIntSeenSection(iIntSectionX, iIntSectionY, false);
                pSeenCell->GetInteriorLocalMapTexture(iIntSectionX, iIntSectionY, spMapTex);
            }
        }
        else {
            TESWorldSpace* pWorld = pPlayer->GetWorldSpace();
            int32_t iCellX = FloatToInt(playerLoc.x) >> 12;
            int32_t iCellY = FloatToInt(playerLoc.y) >> 12;
            int32_t iCellSectionX = iSectX + iCellX - (iMapSide >> 1);
            int32_t iCellSectionY = iSectY + iCellY - (iMapSide >> 1);
            sectOffset = NiPoint3(
                static_cast<float>(iCellSectionX << 12),
                static_cast<float>(iCellSectionY << 12),
                0.0f
            );
            if (pWorld) {
                pSeenCell = pWorld->GetCellFromCellCoord(iCellSectionX, iCellSectionY);
            }
            if (pSeenCell) {
                pSeenCell->GetSeenData();
                pSeenCell->GetExteriorLocalMapTexture(spMapTex);
            }
        }
        NiPoint3* pVerts = new NiPoint3[uiArraySize];
        NiPoint3* pNorm = new NiPoint3[uiArraySize];
        NiPoint2* pTex = new NiPoint2[uiArraySize];
        NiColorA* pColor = new NiColorA[uiArraySize];
        uint16_t* pTriList = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t) * iTileCount));
        int32_t index = 0;
        int32_t x;
        int32_t y;
        for (y = 0; y < iSeenSize; ++y) {
            for (x = 0; x < iSeenSize; ++x) {
                float fVertY = static_cast<double>(iTileSize * y) - static_cast<double>(iTriSize) / 2.0;
                float fVertX = static_cast<double>(iTileSize * x) - static_cast<double>(iTriSize) / 2.0;
                pVerts[index++] = NiPoint3(fVertX, fVertY, 0.0f);
            }
        }
        index = 0;
        for (y = 0; y < iSeenSize; ++y) {
            for (x = 0; x < iSeenSize; ++x) {
                pNorm[index] = NiPoint3(0.0f, 0.0f, 1.0f);
                float fV = 1.0 - static_cast<double>(y) / static_cast<double>(iSeenSize);
                float fU = static_cast<double>(x) / static_cast<double>(iSeenSize);
                pTex[index].x = fU;
                pTex[index].y = fV;
                if (iFog) {
                    float fWorldY = static_cast<double>(y) * fPosSize;
                    float fWorldX = static_cast<double>(x) * fPosSize;
                    NiPoint3 worldCoord(fWorldX, fWorldY, 0.0f);
                    worldCoord += sectOffset;
                    float fSeenVal = static_cast<float>(TESObjectCELL::GetSeenValue(worldCoord));
                    float fShade = fSeenVal / 4.0;
                    pColor[index].r = fShade;
                    pColor[index].g = fShade;
                    pColor[index].b = fShade;
                    pColor[index].a = 0.0f;
                }
                else {
                    pColor[index] = NiColorA::WHITE;
                }
                ++index;
            }
        }
        index = 0;
        for (y = 0; y < iTileSide; ++y) {
            for (x = 0; x < iTileSide; ++x) {
                uint16_t usLastVertex;
                if (x % 2 != y % 2) {
                    pTriList[index++] = x + iSeenSize * (y + 1);
                    pTriList[index++] = x + iSeenSize * y;
                    pTriList[index++] = iSeenSize * y + x + 1;
                    pTriList[index++] = iSeenSize * y + x + 1;
                    pTriList[index++] = iSeenSize * (y + 1) + x + 1;
                    usLastVertex = x + iSeenSize * (y + 1);
                }
                else {
                    pTriList[index++] = iSeenSize * (y + 1) + x + 1;
                    pTriList[index++] = x + iSeenSize * (y + 1);
                    pTriList[index++] = x + iSeenSize * y;
                    pTriList[index++] = x + iSeenSize * y;
                    pTriList[index++] = iSeenSize * y + x + 1;
                    usLastVertex = iSeenSize * (y + 1) + x + 1;
                }
                pTriList[index++] = usLastVertex;
            }
        }
        NiTriShape* pTri = new NiTriShape(
            uiArraySize,
            pVerts,
            pNorm,
            pColor,
            pTex,
            1,
            NiGeometryData::NBT_METHOD_NONE,
            2 * iTileCount,
            pTriList
        );
        pNode->AttachChild(pTri, 1);
        float fOffsetY = static_cast<double>(iSectY * iTriSize) + static_cast<double>(iTriSize) / 2.0 -
            static_cast<double>(iMapSize) / 2.0;
        float fOffsetX = static_cast<double>(iSectX * iTriSize) + static_cast<double>(iTriSize) / 2.0 -
            static_cast<double>(iMapSize) / 2.0;
        pTri->SetTranslate(fOffsetX, fOffsetY, 0.0f);
        if (spMapTex.m_pObject) {
            NiTexturingProperty* pTexProp = new NiTexturingProperty;
            pTexProp->NiTexturingProperty::SetBaseTexture(spMapTex.m_pObject);
            pTexProp->NiTexturingProperty::SetBaseClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
            pTexProp->SetApplyMode(NiTexturingProperty::APPLY_MODULATE);
            pTri->AttachProperty(pTexProp);
            spMapTex = static_cast<NiTexture*>(nullptr);
        }
    }
    NiPoint3 playerPos = pPlayer->GetLocationOnReference();
    NiPoint3 mapWorldCenter;
    if (pPlayer->GetInterior()) {
        NiPoint3 playerIntLoc;
        const NiPoint3& kPlayerLoc = pPlayer->GetLocationOnReference();
        pPlayer->GetParentCell()->AdjustCoordForNorthRotation(kPlayerLoc, playerIntLoc, true);
        int32_t iSectX = (FloatToInt(playerIntLoc.x) - 2048) >> 12;
        int32_t iSectY = (FloatToInt(playerIntLoc.y) - 2048) >> 12;
        mapWorldCenter = NiPoint3(
            static_cast<float>((iSectX << 12) + 4096),
            static_cast<float>((iSectY << 12) + 4096),
            0.0f
        );
        playerPos = playerIntLoc;
    }
    else {
        int32_t iSectX = FloatToInt(pPlayer->GetLocationOnReference().x) >> 12;
        int32_t iSectY = FloatToInt(pPlayer->GetLocationOnReference().y) >> 12;
        mapWorldCenter = NiPoint3(
            static_cast<float>((iSectX << 12) + 2048),
            static_cast<float>((iSectY << 12) + 2048),
            0.0f
        );
    }
    playerPos.z = 0.0f;
    NiPoint3 playerOffset = playerPos - mapWorldCenter;
    float fMapRatio = static_cast<double>(iTriSize) / 4096.0;
    playerOffset.x = playerOffset.x * fMapRatio;
    playerOffset.y = playerOffset.y * fMapRatio;
    playerOffset.z = 0.0f;
    NiPoint3 pointN(0.0f, 1.5f, 1.0f);
    NiPoint3 pointSE(0.5f, -0.5f, 1.0f);
    NiPoint3 pointSW(-0.5f, -0.5f, 1.0f);
    NiColorA kColor(1.0f, 0.0f, 0.0f, 0.0f);
    NiTriShape* pMark = MakeTriangle(pointN, pointSE, pointSW, kColor, true);
    pMark->SetTranslate(playerOffset);
    NiMatrix3 markRot = NiMatrix3::IDENTITY;
    double dNorthRotation = pPlayer->GetParentCell()->GetNorthRotation();
    float fAngle = pPlayer->GetAngleOnReference().z + dNorthRotation;
    markRot.MakeZRotation(fAngle);
    pMark->SetRotate(markRot);
    pNode->AttachChild(pMark, 1);
    pNode->SetTranslate(playerLoc);
    NiUpdateData kUpdateData(0.0f, false, false);
    pNode->Update(kUpdateData);
    pNode->UpdateProperties();
    TES::pInstance->AddTempDebugObject(pNode, 30.0f);
    return true;
}

bool Script::ToggleMagicStatsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bool bShowMagicStats = MagicSystem::GetShowMagicStats();
    MagicSystem::SetShowMagicStats(!bShowMagicStats);
    const char* pState = MagicSystem::GetShowMagicStats() ? "shown." : "hidden.";
    Interface::PrintLine("Magic stats %s", pState);
    return true;
}

bool Script::SaveWorldFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cFileName[516];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cFileName
        )) {
        return false;
    }
    TES::pInstance->SaveWorld(cFileName);
    return true;
}

bool Script::ToggleMenusFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bMenusOn ^= 1u;
    Interface::SetMenuSystemVisible(bMenusOn);
    if (bSystemOutput) {
        const char* pState = bMenusOn ? "On" : "Off";
        Interface::PrintLine("Menus -> %s", pState);
    }
    return true;
}

bool Script::ToggleLeavesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bool bLeavesEnabled = BSTreeManager::AreLeavesVisible();
    BSTreeManager::SetLeavesVisible(!bLeavesEnabled);
    if (bSystemOutput) {
        const char* pState = BSTreeManager::AreLeavesVisible() ? "On" : "Off";
        Interface::PrintLine("Leaves -> %s", pState);
    }
    return true;
}

bool Script::ToggleDetectionStats(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ReferenceProcessLists.bShowDetectionStats = !ReferenceProcessLists.bShowDetectionStats;
    Actor* pActor = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pActor
        )) {
        return false;
    }
    ReferenceProcessLists.pStatdetect = pActor;
    if (bSystemOutput) {
        const char* pState = ReferenceProcessLists.bShowDetectionStats ? "On" : "Off";
        Interface::PrintLine("AI Detection stats printing is  %s", pState);
    }
    return true;
}

bool Script::SetTargetRefraction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (BSShaderManager::IsRefractionEnabled()) {
        float fAmount = 0.0f;
        if (!Script::ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &fAmount
            )) {
            return false;
        }
        if (!apRefObject) {
            apRefObject = pPlayer;
        }
        float fClamped = BSMIN(fAmount, 1.0f);
        fAmount = BSMAX(fClamped, 0.0);
        bool bEnabled = fAmount > 0.0;
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor) {
            pActor->SetRefraction(bEnabled, fAmount);
        }
        else {
            NiAVObject* pObj3D = apRefObject->Get3D();
            if (pObj3D) {
                BSShaderPPLightingProperty::SetRefractionRecurse(pObj3D, 0, 0.0f, 0, 0.0f, 0);
                BSShaderPPLightingProperty::SetRefractionRecurse(pObj3D, 0, 0.0f, 1, 0.0f, 0);
                BSShaderPPLightingProperty::SetRefractionRecurse(pObj3D, bEnabled, fAmount, 0, 0.0f, 0);
            }
        }
        if (bSystemOutput) {
            Interface::PrintLine("%s refraction has been set to %f", apRefObject->GetFullName(), fAmount);
        }
    }
    return true;
}

bool Script::SetMotionBlur(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fBlur = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fBlur
        )) {
        return false;
    }
    BSShaderManager::GetImageSpaceManager()->SetMotionBlurConst(fBlur);
    return true;
}

bool Script::SetVelFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cAxis;
    float fPos = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &cAxis,
            &fPos
        )) {
        return false;
    }
    if (apRefObject) {
        NiPoint3 pos(0.0f, 0.0f, 0.0f);
        switch (cAxis) {
            case 88: {
                pos.x = fPos;
                break;
            }
            case 89: {
                pos.y = fPos;
                break;
            }
            case 90: {
                pos.z = fPos;
                break;
            }
        }
        TESHavokUtilities::AddVelocity(apRefObject->Get3D(), pos, true);
    }
    return true;
}

bool Script::ShowViewerStringsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject && apRefObject->Get3D()) {
        NiNode* pNode = apRefObject->Get3D();
        NiTPrimitiveArray<char*>* pStrings = ::new NiTPrimitiveArray<char*>(0, 1);
        if (pNode && pNode->IsNode()) {
            NiNode* pParent = pNode;
            pNode = nullptr;
            uint32_t uiChild = 0;
            for (uint32_t i = pParent->GetArrayCount(); uiChild < i && !pNode; i = pParent->GetArrayCount()) {
                pNode = static_cast<NiNode*>(pParent->GetAt(uiChild++));
            }
        }

        if (!pNode) {
            Interface::PrintLine("Node with no children supplied as reference...");
            return true;
        }
        pNode->GetViewerStrings(pStrings);
        for (uint32_t uiIndex = 0; uiIndex < pStrings->m_usSize; ++uiIndex) {
            Interface::PrintLine(static_cast<const char*>((*pStrings)[uiIndex]));
        }
        pStrings->RemoveAll();
        if (pNode->GetCollisionObject()) {
            NiObject* pCollision = NiObject::DynamicCastStatic(
                &bhkCollisionObject::ms_RTTI,
                pNode->GetCollisionObject()
            );
            if (pCollision) {
                bhkRigidBody* pBody = static_cast<bhkCollisionObject*>(pCollision)->GetRigidBody();
                pBody->GetViewerStrings(pStrings);
                for (uint32_t j = 0; j < pStrings->m_usSize; ++j) {
                    Interface::PrintLine(static_cast<const char*>((*pStrings)[j]));
                }
                pStrings->RemoveAll();
            }
        }
        if (pStrings) {
            delete pStrings;
        }
    }
    else {
        Interface::PrintLine("Must supply a valid reference to show viewer string.");
    }
    return true;
}

bool Script::ToggleLODLandFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cType[516];
    cType[0] = 0;
    if (apParameters &&
        !Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cType
        )) {
        return false;
    }
    BGSTerrainManager* pTerrainManager = TES::pInstance->pWorldSpace->GetTerrainManager();
    if (!pTerrainManager) {
        return true;
    }
    char cObjects = 0;
    char cTrees = 0;
    char cLand = 0;
    char cWater = 0;
    if (!cType[0] || !BSstrcmp(cType, "all")) {
        Interface::PrintLine("Toggling all LOD.");
        cObjects = 1;
        cTrees = 1;
        cLand = 1;
        cWater = 1;
    }
    if (!BSstrcmp(cType, "water")) {
        Interface::PrintLine("Toggling water.");
        cWater = 1;
    }
    if (!BSstrcmp(cType, "land")) {
        Interface::PrintLine("Toggling land.");
        cLand = 1;
    }
    if (!BSstrcmp(cType, "trees")) {
        Interface::PrintLine("Toggling trees.");
        cTrees = 1;
    }
    if (!BSstrcmp(cType, "objects") || !BSstrcmp(cType, "buildings")) {
        Interface::PrintLine("Toggling objects.");
        cObjects = 1;
    }
    if (cWater || cLand || cTrees || cObjects) {
        if (pTerrainManager->GetTreeNode() && cTrees) {
            bool bCull = !pTerrainManager->GetTreeNode()->GetAppCulled();
            pTerrainManager->GetTreeNode()->SetAppCulled(bCull);
        }

        if (pTerrainManager->GetLODLandRoot() && cLand) {
            bool bCull = !pTerrainManager->GetLODLandRoot()->GetAppCulled();
            pTerrainManager->GetLODLandRoot()->SetAppCulled(bCull);
        }

        if (pTerrainManager->GetLODObjectRoot() && cObjects) {
            bool bCull = !pTerrainManager->GetLODObjectRoot()->GetAppCulled();
            pTerrainManager->GetLODObjectRoot()->SetAppCulled(bCull);
        }

        if (pTerrainManager->GetWaterLODRoot() && cWater) {
            bool bAppCulled = pTerrainManager->GetWaterLODRoot()->GetAppCulled();
            pTerrainManager->GetWaterLODRoot()->SetAppCulled(!bAppCulled);
        }

        return true;
    }

    Interface::PrintLine("Invalid parameter.  Please specify land, objects, trees, water, or all.");
    return true;
}

bool Script::SetLODObjectDistanceFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float f0 = 0.0f;
    float f1 = 0.0f;
    if (apParameters &&
        !Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &f0,
            &f1
        )) {
        return false;
    }
    BGSTerrainManager* pm = TES::pInstance->pWorldSpace->GetTerrainManager();
    if (!pm) {
        return true;
    }
    fBlockLoadDistanceLow = f0;
    fBlockLoadDistance = f1;
    NiPoint3 updatePos = NiPoint3::ZERO;
    const NiPoint3* pPos;
    if (appMain->bFlyCamera) {
        pPos = &Main::GetWorldSceneGraph()->GetCameraRoot()->GetTranslate();
    }
    else {
        pPos = &pPlayer->GetLocationOnReference();
    }
    updatePos = *pPos;
    pm->Update(pPlayer->GetLocationOnReference(), BGSTerrainManager::UPDATE_LAND_AND_BLOCKS);
    return true;
}

bool Script::HighlightBodyPartFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iParam3;
    char cParam2[4];
    char cParam4[4];
    float fLimb = 15.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fLimb,
            cParam2,
            &iParam3,
            cParam4
        )) {
        return false;
    }
    if (apRefObject && apRefObject->Get3D()) {
        BGSBodyPart::LIMB_ENUM eLimb = static_cast<BGSBodyPart::LIMB_ENUM>(static_cast<int32_t>(fLimb));
        Interface::GetVATSEffect()->SetTarget(apRefObject, eLimb, false);
        FOVATSEffectManager::VATS_EFFECT_MODE eMode = FOVATSEffectManager::SINGLE_TARGET;
        Interface::GetVATSEffect()->SetEffectMode(eMode);
    }

    return true;
}

bool Script::HighlightAdditionalReferenceFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cParam3[4];
    char cParam2[4];
    char cParam4[4];
    float fParam1 = 15.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fParam1,
            cParam2,
            cParam3,
            cParam4
        )) {
        return false;
    }
    if (apRefObject && apRefObject->Get3D()) {
        Interface::GetVATSEffect()->SetMultiTarget(apRefObject);
        Interface::GetVATSEffect()->SetSelectedMultiTarget(apRefObject);
        FOVATSEffectManager::VATS_EFFECT_MODE eMode = FOVATSEffectManager::MULTIPLE_TARGETS;
        Interface::GetVATSEffect()->SetEffectMode(eMode);
    }

    return true;
}

bool Script::DeactivateAllHighlightsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cParam3[4];
    char cParam2[4];
    char cParam4[4];
    float fParam1 = 15.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fParam1,
            cParam2,
            cParam3,
            cParam4
        )) {
        return false;
    }
    if (apRefObject && apRefObject->Get3D()) {
        Interface::GetVATSEffect()->SetTarget(nullptr, BGSBodyPart::LIMB_TORSO, false);
        Interface::GetVATSEffect()->ClearMultiTargets();
        FOVATSEffectManager::VATS_EFFECT_MODE eMode = FOVATSEffectManager::SINGLE_TARGET;
        Interface::GetVATSEffect()->SetEffectMode(eMode);
    }

    return true;
}

bool Script::AddOcclusionPlane(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    NiPoint2 kSize(-1.0f, -1.0f);
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &kSize.y,
            &kSize
        )) {
        return false;
    }
    if (kSize.x == 0.0 || kSize.y == 0.0) {
        BSShaderManager::GetShadowSceneNode(0)->GetPortalGraph()->RemoveAllOccluders();
        return true;
    }

    NiMatrix3 kCameraFunk;
    if (kSize.x <= 0.0) {
        kSize.x = 50.0f;
    }
    if (kSize.y <= 0.0) {
        kSize.y = 50.0f;
    }
    NiPoint3 kPos(BSShaderManager::GetShadowSceneNode(0)->GetCameraPos());
    NiMatrix3 kCameraRot = Main::GetWorldSceneGraph()->GetCamera()->GetWorldTransform().m_Rotate;
    kCameraFunk.MakeZRotation(1.5707963f);
    kCameraRot = kCameraRot * kCameraFunk;
    NiPointer<BSOcclusionPlane> spOccluder(new BSOcclusionPlane(&kPos, &kSize, &kCameraRot));
    BSShaderManager::GetShadowSceneNode(0)->GetPortalGraph()->AddOccluder(spOccluder.m_pObject);
    return true;
}

bool Script::SetGlobalRadialBlur(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fRampUp;
    float fStrength;
    float fDownStart;
    float fRampDown;
    float fStart;
    bool bRet = true;
    if (g_radialBlurIMOD) {
        ImageSpaceModifierInstance::Stop(g_radialBlurIMOD);
        g_radialBlurIMOD = nullptr;
    }
    if (Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fStrength,
            &fRampUp,
            &fStart,
            &fRampDown,
            &fDownStart
        )) {
        g_radialBlurIMOD = ImageSpaceModifierInstanceRB::Trigger(
            fStrength,
            fRampUp,
            fStart,
            fRampDown,
            fDownStart,
            NiPoint2(0.5f, 0.5f),
            0.0f
        );
        if (g_radialBlurIMOD) {
            g_radialBlurIMOD->SetPermanent(true);
        }
    }
    else {
        if (bSystemOutput) {
            Interface::PrintLine("SetGlobalRadialBlur strength rampup start rampdown downstart");
        }
        return false;
    }
    return bRet;
}

bool Script::ToggleAI(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pActor
        )) {
        return false;
    }
    if (!pActor) {
        pActor = dynamic_cast<Actor*>(apRefObject);
    }
    if (pActor) {
        // Tests the whole byte, not a bool's low bit: any non-zero byte reads as on.
        pActor->bProcessMe = static_cast<uint8_t>(pActor->bProcessMe) == 0;
        pActor->EndMovement();
        if (bSystemOutput) {
            const char* pState = pActor->bProcessMe ? "On" : "Off";
            Interface::PrintLine("%s processing is  %s", pActor->GetFullName(), pState);
        }
    }
    else {
        // Tests the whole byte here too.
        ReferenceProcessLists.bRunScheduals = static_cast<uint8_t>(ReferenceProcessLists.bRunScheduals) == 0;
        if (bSystemOutput) {
            const char* pState = ReferenceProcessLists.bRunScheduals ? "On" : "Off";
            Interface::PrintLine("All AI Processing is  %s", pState);
        }
        if (!ReferenceProcessLists.bRunScheduals) {
            ReferenceProcessLists.TurnOffMovementinHigh();
        }
    }
    return true;
}

bool Script::ToggleFlyCam(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iFreezeTime = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iFreezeTime
        )) {
        return false;
    }
    bool bIsFlyCam = false;
    if (iFreezeTime != 4) {
        if ((iFreezeTime >= 1 && iFreezeTime <= 2) || iFreezeTime == 5) {
            bIsFlyCam = pPlayer->ToggleFlyCam(true);
        }
        else {
            bIsFlyCam = pPlayer->ToggleFlyCam(false);
        }
    }
    if (bIsFlyCam && iFreezeTime >= 1) {
        BSShaderProperty::PreviousTransform::SetFreeze(true);
    }
    else {
        BSShaderProperty::PreviousTransform::SetFreeze(false);
    }
    return true;
}

bool Script::ToggleNavMeshFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t eMode = 4;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eMode
        )) {
        return false;
    }
    if (eMode > 6) {
        eMode = 4;
    }
    bool bIsDraw = NavMeshRender::GetInstance()->GetDraw();
    if (eMode == 4) {
        eMode = !bIsDraw;
    }
    if (eMode == 5) {
        NavMeshRender::bDrawCover = !NavMeshRender::bDrawCover;
        if (bIsDraw) {
            eMode = NavMeshRender::iNavMeshDrawMode;
            NavMeshRender::GetInstance()->SetDraw(false);
            bIsDraw = false;
        }
        else {
            eMode = 0;
        }
    }
    if (eMode == 6) {
        NavMeshRender::bDrawConnections = !NavMeshRender::bDrawConnections;
        if (bIsDraw) {
            eMode = NavMeshRender::iNavMeshDrawMode;
            NavMeshRender::GetInstance()->SetDraw(false);
            bIsDraw = false;
        }
        eMode = 3;
    }
    if (eMode) {
        NavMeshRender::bDrawTriangles = true;
        if (eMode == 2) {
            NavMeshRender::GetInstance()->SetTransparentOverLayMode(true);
        }
        else if (eMode == 1 || eMode == 3) {
            NavMeshRender::GetInstance()->SetTransparentOverLayMode(false);
        }
        NavMeshRender::iNavMeshDrawMode = eMode;
        if (!bIsDraw) {
            TESObjectCELL* pCurrentCell = TES::pInstance->GetCurrentCell();
            if (pCurrentCell) {
                if (pCurrentCell->GetInterior()) {
                    NavMeshRender::GetInstance()->AddDrawOnlyNavMeshes(pCurrentCell, false);
                }
                else {
                    int32_t iRadius = (uGridsToLoad.Int() - 1) / 2;
                    for (int32_t iX = pCurrentCell->GetDataX() - iRadius; iX <= iRadius + pCurrentCell->GetDataX();
                         ++iX) {
                        for (int32_t iY = pCurrentCell->GetDataY() - iRadius; iY <= iRadius + pCurrentCell->GetDataY();
                             ++iY) {
                            TESObjectCELL* pCell = pCurrentCell->GetWorldSpace()->GetCellFromCellCoord(iX, iY);
                            NavMeshRender::GetInstance()->AddDrawOnlyNavMeshes(pCell, false);
                        }
                    }
                }
            }
            NavMeshRender::GetInstance()->SetDraw(true);
        }
    }
    else {
        NavMeshRender::bDrawTriangles = false;
        NavMeshRender::GetInstance()->SetDraw(false);
    }
    return true;
}

bool Script::PickRefByIDFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pPickRef = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pPickRef
        )) {
        return false;
    }
    if (pPickRef) {
        int32_t iFormType = pPickRef->GetFormType();
        if (iFormType >= REFR_ID && (iFormType <= PFLA_ID || iFormType == PCBE_ID)) {
            BSStringT<char> kText;
            int32_t iFormID = pPickRef->iFormID;
            kText.SPrintF("\"%s\" (%08x)", pPickRef->GetFullName(), iFormID);
            int32_t iFont = iDebugTextFont.Int() - 1;
            int32_t iY = static_cast<int32_t>(FontManager::Instance()->GetFont(iFont)->GetBaseLine() + 3.0) - 5;
            float fY = static_cast<float>(iY);
            DebugText::Instance(true)->Print(kText.pString, 640.0f, fY, 2, -1, -1.0f, 0, nullptr);
            Interface::SetTargetREFR(pPickRef);
        }
    }
    return true;
}

bool Script::GetActorValueFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pActorValue = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pActorValue
           ) &&
        Script::GetActorValueConditionFunction(apRefObject, pActorValue, nullptr, afReturnValue);
}

bool Script::GetPermanentActorValueFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pActorValue = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pActorValue
           ) &&
        Script::GetPermanentActorValueConditionFunction(apRefObject, pActorValue, nullptr, afReturnValue);
}

bool Script::SetActorValueFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ActorValue::Index eIndex = ActorValue::AGGRESSION;
    int32_t iValue = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eIndex,
            &iValue
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        if (ActorValue::IsFlagSet(eIndex, ActorValue::NO_SCRIPT_MODAV)) {
            if (bSystemOutput) {
                Interface::PrintLine(
                    "Actor Value '%s' cannot be modified in scripts or the console.",
                    ActorValue::GetActorValueScriptName(eIndex)
                );
            }
        }
        else {
            pActor->SetActorValue(eIndex, iValue);
        }
    }
    return true;
}

bool Script::ModActorValueFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ActorValue::Index eIndex = ActorValue::AGGRESSION;
    int32_t iValue = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eIndex,
            &iValue
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        if (ActorValue::IsFlagSet(eIndex, ActorValue::NO_SCRIPT_MODAV)) {
            if (bSystemOutput) {
                Interface::PrintLine(
                    "Actor Value '%s' cannot be modified in scripts or the console.",
                    ActorValue::GetActorValueScriptName(eIndex)
                );
            }
        }
        else {
            // Slot 234 (+0x3A8): the int32_t overload.
            pActor->PermanentModActorValue(eIndex, iValue, static_cast<Actor*>(nullptr));
        }
    }
    return true;
}

bool Script::RestoreActorValueFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ActorValue::Index eIndex = ActorValue::AGGRESSION;
    float fValue = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eIndex,
            &fValue
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        if (ActorValue::IsFlagSet(eIndex, ActorValue::NO_SCRIPT_MODAV)) {
            if (bSystemOutput) {
                Interface::PrintLine(
                    "Actor Value '%s' cannot be modified in scripts or the console.",
                    ActorValue::GetActorValueScriptName(eIndex)
                );
            }
        }
        else {
            float fAmount = ActorValue::IsFlagSet(eIndex, ActorValue::DAMAGE_IS_POSITIVE) ? -fValue : std::fabs(fValue);
            pActor->RestoreActorValue(eIndex, fAmount);
        }
    }
    return true;
}

bool Script::ForceActorValueFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ActorValue::Index eIndex = ActorValue::AGGRESSION;
    int32_t iValue = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eIndex,
            &iValue
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        if (ActorValue::IsFlagSet(eIndex, ActorValue::NO_SCRIPT_MODAV)) {
            if (bSystemOutput) {
                Interface::PrintLine(
                    "Actor Value '%s' cannot be modified in scripts or the console.",
                    ActorValue::GetActorValueScriptName(eIndex)
                );
            }
        }
        else {
            double dValue = static_cast<double>(iValue);
            float fDelta = dValue - pActor->GetActorFloatValue(eIndex);
            pActor->PermanentModActorValue(eIndex, fDelta, nullptr);
        }
    }
    return true;
}

bool Script::IsLimbGoneFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iLimb = 0;
    int32_t iLastLimb = -1000;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iLimb,
            &iLastLimb
        )) {
        return false;
    }
    if (iLastLimb <= 0) {
        return Script::IsLimbGoneConditionFunction(
            apRefObject,
            reinterpret_cast<void*>(static_cast<intptr_t>(iLimb)),
            nullptr,
            afReturnValue
        );
    }
    if (apRefObject && apRefObject->IsActor()) {
        for (int32_t i = iLimb; i <= iLastLimb; ++i) {
            if (apRefObject->GetDismembered(static_cast<BGSBodyPart::LIMB_ENUM>(i))) {
                afReturnValue = 1.0;
                return true;
            }
        }
    }
    return true;
}

bool Script::SetLightingPassesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char input[516];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            input
        )) {
        return false;
    }
    char* pCurrent = input;
    uint16_t passes = 0;
    // Tests pCurrent, the copy of the buffer's address.
    if (pCurrent && *pCurrent == 49) {
        passes |= 1u;
        BSShaderManager::SetRenderMode(0);
        BSShaderManager::SetDebugMode(BSShaderManager::BSSM_RENDER_NORMAL);
        if (BSstrlen(input) == 1) {
            passes |= 2u;
            passes |= 4u;
            passes |= 8u;
            passes |= BSShaderManager::GetShaderLevel(false) < BSShaderManager::BSSM_SV_2_0 ? 0 : 32;
        }
    }
    else if (pCurrent && *pCurrent == 50) {
        passes = 1;
        BSShaderManager::SetDebugMode(BSShaderManager::BSSM_RENDER_NUMLIGHTS);
    }
    else if (pCurrent && *pCurrent == 51) {
        passes = 1;
        BSShaderManager::SetDebugMode(BSShaderManager::BSSM_RENDER_NUMPASSES);
    }
    else if (pCurrent && *pCurrent == 52) {
        passes |= 1u;
        BSShaderManager::SetRenderMode(3u);
        BSShaderManager::SetDebugMode(BSShaderManager::BSSM_RENDER_NORMAL);
        if (BSstrlen(input) == 1) {
            passes |= 2u;
            passes |= 4u;
            passes |= 8u;
            passes |= BSShaderManager::GetShaderLevel(false) < BSShaderManager::BSSM_SV_2_0 ? 0 : 32;
        }
    }
    else if (pCurrent && *pCurrent == 53) {}
    else if (!_strnicmp("tex", pCurrent, 3u)) {
        passes = 1;
        BSShaderManager::SetDebugMode(BSShaderManager::BSSM_RENDER_TEXUSAGE);
    }
    else if (!_strnicmp("degrade", pCurrent, 7u)) {
        passes = 1;
        BSShaderManager::SetDebugMode(BSShaderManager::BSSM_RENDER_DEGRADE);
    }
    else if (!_strnicmp("sh", pCurrent, 2u)) {
        if (pCurrent[2]) {
            int32_t iMode = pCurrent[2] - 48;
            if (pCurrent[2] == 49 || pCurrent[2] == 50) {
                iMode = 3;
            }
            int32_t iClampedMode = BSMIN(3, iMode);
            BSShaderManager::eShadowMode = static_cast<BSShaderManager::etShadowMode>(BSMAX(0, iClampedMode));
            BSShaderManager::ReloadShaders();
            passes = BSShaderManager::GetEnabledPasses() | 128;
        }
        else {
            bool bToggle = BSShaderManager::GetShadows();
            BSShaderManager::EnableShadows(!bToggle);
            passes = BSShaderManager::GetEnabledPasses();
            if ((passes & 128) != 0) {
                passes &= ~128u;
            }
            else {
                passes |= 128u;
            }
        }
    }
    else if (!_strnicmp("sc", pCurrent, 2u)) {
        BSShaderManager::bLightScissor = !BSShaderManager::bLightScissor;
        passes = BSShaderManager::GetEnabledPasses();
    }
    else if (!_strnicmp("psh", pCurrent, 2u)) {
        if ((BSShaderManager::GetEnabledPasses() & BSShaderManager::BSSM_ENABLED_PROJECTED_SHADOW) != 0) {
            Renderer::SetDoCanopyShadowPass(false);
            BSShaderManager::SetProjectedShadowMaskTexture(nullptr);
            uint16_t usPasses = BSShaderManager::GetEnabledPasses() & ~BSShaderManager::BSSM_ENABLED_PROJECTED_SHADOW;
            BSShaderManager::SetEnabledPasses(usPasses);
        }
        else {
            Renderer::SetDoCanopyShadowPass(true);
            TES::pInstance->pGridCellA->RecreateCanopyShadowMaskTexture();
            uint16_t usPasses = BSShaderManager::GetEnabledPasses() | BSShaderManager::BSSM_ENABLED_PROJECTED_SHADOW;
            BSShaderManager::SetEnabledPasses(usPasses);
        }
        return true;
    }
    else if (*pCurrent == 116) {
        BSShaderManager::iMaxLandTextures = std::atoi(pCurrent + 1);
    }
    else if (_strnicmp("dsh", pCurrent, 3u)) {
        if (!_strnicmp("opt", pCurrent, 3u)) {
            BSShaderManager::bUseOptPasses = !BSShaderManager::bUseOptPasses;
            passes = BSShaderManager::GetEnabledPasses() | 256;
        }
        else if (!_strnicmp("alpha", pCurrent, 5u)) {
            return true;
        }
    }
    else {
        if (apRefObject) {
            ShadowSceneNode* pssn = BSShaderManager::GetShadowSceneNode(0);
            for (ShadowSceneLight* pssl = pssn->GetFirstShadowLight(); pssl; pssl = pssn->GetNextShadowLight()) {
                NiNode* pShadowScene = static_cast<NiNode*>(pssl->GetShadowScene());
                if (pShadowScene == apRefObject->Get3D()) {
                    pssl->ShowDebugTexture(!pssl->bShowDebugTexture);
                }
            }
        }
        return true;
    }
    if (++pCurrent && *pCurrent == 49) {
        passes |= 2u;
    }

    if (++pCurrent && *pCurrent == 49) {
        passes |= 4u;
    }

    if (++pCurrent && *pCurrent == 49) {
        passes |= 8u;
    }

    if (BSShaderManager::GetShaderLevel(false) >= BSShaderManager::BSSM_SV_2_0 && (!++pCurrent || *pCurrent != 48)) {
        passes |= 32u;
    }
    BSShaderManager::SetEnabledPasses(passes);
    return true;
}

bool Script::SetFogDepthFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float iDistNear = 0.0f;
    float iDistFar = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iDistNear,
            &iDistFar
        )) {
        return false;
    }
    BSFogProperty* pBSFog = TES::pInstance->pSky->pAtmosphere->GetFog();
    if (pBSFog) {
        bool bUpdateDist = iDistNear == 0.0f && iDistFar == 0.0f;
        TES::pInstance->pSky->pAtmosphere->SetUpdateFogDistance(bUpdateDist);
        float fDistFar = static_cast<float>(std::bit_cast<int32_t>(iDistFar));
        float fDistNear = static_cast<float>(std::bit_cast<int32_t>(iDistNear));
        pBSFog->SetDistance(fDistNear, fDistFar);
    }
    return true;
}

bool Script::SetClipDistFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    NiFrustum frustum;
    float fClipDistance = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fClipDistance
        )) {
        return false;
    }
    bool bUpdateFogDistance = false;
    if (fClipDistance <= 0.0) {
        fClipDistance = 163840.0f;
        bUpdateFogDistance = true;
    }
    else if (fClipDistance > 163840.0) {
        fClipDistance = 163840.0f;
    }
    NiCamera* pCamera = Main::GetWorldSceneGraph()->GetCamera();
    if (pCamera) {
        frustum = pCamera->GetViewFrustum();
        if (frustum.m_fFar != fClipDistance) {
            frustum.m_fFar = fClipDistance;
            float fMaxFarNearRatio = fClipDistance / frustum.m_fNear;
            pCamera->SetMaxFarNearRatio(fMaxFarNearRatio);
            pCamera->SetViewFrustum(frustum);
            TES::pInstance->pSky->pAtmosphere->SetUpdateFogDistance(bUpdateFogDistance);
        }
    }
    return true;
}

bool Script::SetHUDGlowConstants(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cParam4[4];
    float fBlurRadius = fBlurRadiusHUD.Float();
    float fBlurIntensity = fBlurIntensityHUD.Float();
    float fScanlineFrequency = fScanlineFrequencyHUD.Float();
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fBlurRadius,
            &fBlurIntensity,
            &fScanlineFrequency,
            cParam4
        )) {
        return false;
    }
    Interface::GetHUDEffects()->SetBlurRadiusHUD(fBlurRadius);
    Interface::GetHUDEffects()->SetBlurIntensityHUD(fBlurIntensity);
    Interface::GetHUDEffects()->SetScanlineFrequencyHUD(fScanlineFrequency);
    return true;
}

bool Script::HairTint(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iR = 0;
    int32_t iG = 0;
    int32_t iB = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iR,
            &iG,
            &iB
        )) {
        return false;
    }
    float fB = static_cast<double>(iB) / 256.0;
    float fG = static_cast<double>(iG) / 256.0;
    float fR = static_cast<double>(iR) / 256.0;
    BSShaderLightingProperty::kHairTint = NiColorA(fR, fG, fB, 1.0f);
    return true;
}

bool Script::RotateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cAxis;
    int32_t iSpeed = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &cAxis,
            &iSpeed
        )) {
        return false;
    }
    if (apRefObject) {
        NiAVObject* obj3D = apRefObject->Get3D();
        if (obj3D) {
            float fAngle = 0.0f;
            switch (cAxis) {
                case 88: {
                    fAngle = apRefObject->GetAngleOnReference().x;
                    break;
                }
                case 89: {
                    fAngle = apRefObject->GetAngleOnReference().y;
                    break;
                }
                case 90: {
                    fAngle = apRefObject->GetAngleOnReference().z;
                    break;
                }
            }
            double dSpeed = static_cast<double>(iSpeed) * 0.01745329238474369;
            for (fAngle = appTimer.fDelta * dSpeed + fAngle; fAngle >= 6.2831854820251465;
                 fAngle = fAngle - 6.2831854820251465) {
                ;
            }
            while (fAngle < 0.0) {
                fAngle = fAngle + 6.2831854820251465;
            }
            switch (cAxis) {
                case 88: {
                    apRefObject->SetAngleOnReferenceX(fAngle);
                    break;
                }
                case 89: {
                    apRefObject->SetAngleOnReferenceY(fAngle);
                    break;
                }
                case 90: {
                    apRefObject->SetAngleOnReferenceZ(fAngle);
                    break;
                }
            }
            obj3D->SetRotate(apRefObject->GetOrientation());
            if (!apRefObject->GetAnimation()) {
                NiUpdateData kUpdateData(0.0f, false, false);
                obj3D->Update(kUpdateData);
            }
        }
    }
    return true;
}

bool Script::SetAngleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cAxis;
    float fDegrees = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &cAxis,
            &fDegrees
        )) {
        return false;
    }
    if (!apRefObject || apRefObject->CanBeMoved()) {
        if (iNumHWThreads.Int() <= 1) {
            Script::SetAngleBase(apRefObject, cAxis, fDegrees);
        }
        else {
            TaskQueueInterface::GetInstance()->QueueScriptFunctionCall(0x1009u, apRefObject, cAxis, fDegrees);
        }
        return true;
    }

    if (bSystemOutput) {
        Interface::PrintLine("%s cannot be moved.", apRefObject->GetFormEditorID());
    }
    if (!pDataHandler->IsFormIDCreated(apScript->iFormID)) {
        int32_t iRefFormID = apRefObject->iFormID;
        const char* pRefName = apRefObject->GetFormEditorID();
        int32_t iScriptFormID = apScript->iFormID;
        Warning(
            "SCRIPTS: Script '%s' (%08X) is attempting to move reference '%s' (%08X), but it cannot be moved",
            apScript->GetFormEditorID(),
            iScriptFormID,
            pRefName,
            iRefFormID
        );
    }
    return true;
}

bool Script::PlayGroupFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ANIM_GROUP_ENUM eAnimGroup;
    int32_t iFlags = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eAnimGroup,
            &iFlags
        )) {
        return false;
    }
    if (apRefObject) {
        apRefObject->AddChange(CHANGE_REFR_ANIMATION);
        if (apRefObject->GetAnimation() && apRefObject->IsActor()) {
            Actor* pActor = dynamic_cast<Actor*>(apRefObject);
            ACTION_FLAGS eFlags = static_cast<ACTION_FLAGS>(iFlags);
            uint16_t usAnimGroup = pActor->GetAnimGroup(eAnimGroup, nullptr, false, nullptr);
            apRefObject->GetAnimation()->PlayGroup(usAnimGroup, eFlags, -1, AGS_NONE);
            if (eAnimGroup) {
                BSAnimGroupSequence* pSequence = apRefObject->GetAnimation()->GetCurrentSequence(AGS_MOVEMENT);
                pActor->SetAnimAction(ANIM_ACTION_FORCE_SCRIPT_ANIM, pSequence);
            }
            else if (pActor->GetAnimAction() == ANIM_ACTION_FORCE_SCRIPT_ANIM) {
                pActor->SetAnimAction(ANIM_ACTION_NONE, nullptr);
            }
        }
        else if (eAnimGroup != ANIM_GROUP_NONE) {
            NiAVObject* pObj = apRefObject->Get3D();
            NiNode* obj3D = pObj ? pObj->IsNode() : nullptr;
            if (obj3D && obj3D->GetAtChecked(0)) {
                NiObject* pController = obj3D->GetAt(0)->GetControllers();
                NiControllerManager* pManager = static_cast<NiControllerManager*>(
                    NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pController)
                );
                if (!pManager) {
                    pController = obj3D->GetControllers();
                    pManager = static_cast<NiControllerManager*>(
                        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pController)
                    );
                }
                if (pManager) {
                    NiControllerSequence* pSeq = pManager->GetSequenceByName(animGroupInfo[eAnimGroup].pName);
                    if (pSeq) {
                        for (uint32_t n = 0; n < pManager->GetSequenceCount(); ++n) {
                            NiControllerSequence* pDisableSeq = pManager->GetSequenceAt(n);
                            const char* pIdleName = animGroupInfo[0].pName;
                            const char* pSeqName = pDisableSeq->m_kName.GetHandleString();
                            if (BSstricmp(pSeqName, pIdleName)) {
                                pManager->DeactivateSequence(pDisableSeq, 0.0f);
                            }
                        }
                        if (iFlags) {
                            float fEaseInTime = static_cast<double>(iFlags) * 0.10000000149011612;
                            pManager->ActivateSequence(pSeq, 0, true, 1.0f, fEaseInTime, nullptr);
                        }
                        else {
                            pManager->ActivateSequence(pSeq, 0, false, 1.0f, 0.0f, nullptr);
                        }
                        pSeq->SetOffset(-std::numeric_limits<float>::max());
                        pManager->SetActive(true);
                    }
                    else {
                        int32_t iRefFormID = apRefObject->iFormID;
                        const char* pRefName = apRefObject->GetFormEditorID();
                        Warning(
                            "SCRIPT: '%s' (PlayGroup Error) Sequence '%s' not found for '%s' (%08X).",
                            apScript->GetFormEditorID(),
                            animGroupInfo[eAnimGroup].pName,
                            pRefName,
                            iRefFormID
                        );
                    }
                }
                else {
                    int32_t iRefFormID = apRefObject->iFormID;
                    const char* pRefName = apRefObject->GetFormEditorID();
                    Warning(
                        "SCRIPT: '%s' (PlayGroup Error) No NiControllerManager found for '%s' (%08X).",
                        apScript->GetFormEditorID(),
                        pRefName,
                        iRefFormID
                    );
                }
            }
            else {
                int32_t iRefFormID = apRefObject->iFormID;
                const char* pRefName = apRefObject->GetFormEditorID();
                Warning(
                    "SCRIPT: '%s' (PlayGroup Error) No 3d or controllers found for '%s' (%08X).",
                    apScript->GetFormEditorID(),
                    pRefName,
                    iRefFormID
                );
            }
        }
        apRefObject->SetAltered(true);
    }
    return true;
}

bool Script::LoopGroupFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    // The AnimGroup parameter (SCRIPT_PARAM_ANIM_GROUP) parses to 4 bytes; only its low word is passed on.
    ANIM_GROUP_ENUM eAnimGroup;
    int32_t iLoops = 0;
    ACTION_FLAGS eFlags = ACTION_NORMAL;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eAnimGroup,
            &iLoops,
            &eFlags
        )) {
        return false;
    }
    if (apRefObject && apRefObject->GetAnimation()) {
        apRefObject->GetAnimation()->PlayGroup(static_cast<uint16_t>(eAnimGroup), eFlags, iLoops, AGS_NONE);
        apRefObject->SetAltered(true);
    }

    return true;
}

bool Script::IsAnimPlayingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t uiGroup[2];
    uiGroup[0] = 255;
    uiGroup[1] = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            uiGroup
        )) {
        return false;
    }
    afReturnValue = 0.0;
    if (apRefObject) {
        if (apRefObject->GetAnimation()) {
            Animation* pAnim = apRefObject->GetAnimation();
            if (uiGroup[0] == 255) {
                // Counts the sections in a signed int that is passed as the section.
                for (int32_t n = AGS_IDLE; n < ANIM_GROUP_SECTION_COUNT; ++n) {
                    if (pAnim->GetCurrentSequence(static_cast<ANIM_GROUP_SECTION>(n))) {
                        afReturnValue = 1.0;
                    }
                }
            }
            else if (pAnim->GetCurrentSequence(animGroupInfo[uiGroup[0]].eSectionLayer)) {
                afReturnValue = 1.0;
            }
        }
        else {
            NiAVObject* pAV = apRefObject->Get3D();
            NiNode* pNode = pAV ? pAV->IsNode() : nullptr;
            if (pNode && pNode->GetControllers()) {
                NiControllerManager* pManager = pNode->GetControllers()->IsNiControllerManager();
                if (pManager) {
                    if (uiGroup[0] == 255) {
                        for (int32_t n = 0; n < pManager->GetSequenceCount(); ++n) {
                            NiControllerSequence* pSeq = pManager->GetSequenceAt(static_cast<uint32_t>(n));
                            if (pSeq && pSeq->GetState()) {
                                afReturnValue = 1.0;
                            }
                        }
                    }
                    else {
                        NiControllerSequence* pSeq = pManager->GetSequenceByName(animGroupInfo[uiGroup[0]].pName);
                        if (pSeq && pSeq->GetState()) {
                            afReturnValue = 1.0;
                        }
                    }
                }
            }
        }
        if (bSystemOutput) {
            Interface::PrintLine("IsAnimPlaying -> %1.0f", afReturnValue);
        }
    }
    return true;
}

bool Script::StartCombatFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pTarget = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTarget
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pAttacker = dynamic_cast<Actor*>(apRefObject);
        if (pAttacker &&
            !pAttacker->IsDead(false) &&
            pTarget &&
            !pTarget->IsDead(false) &&
            pAttacker->pCurrentProcess &&
            pTarget->pCurrentProcess) {
            if (pAttacker->IsinDialogue() && pTarget != pPlayer) {
                Warning(
                    "COMBAT: %s is in conversation so will not go into combat. Do not put startcombat in the "
                    "dialogue results",
                    pAttacker->GetFullName()
                );
                return true;
            }
            if (pTarget == pPlayer && pPlayer->IsSleepingorResting()) {
                pPlayer->SetSleeping(0, true);
                ReferenceProcessLists.RemoveActorFromTempChangeList(pAttacker);
            }
            bool bAgressor = !pAttacker->IsActoraGuard();
            if ((!pAttacker->IsInCombat() ||
                 (pAttacker->GetCombatController() &&
                  !pAttacker->GetCombatController()->IsActoraCombatTarget(pTarget))) &&
                pAttacker->pCurrentProcess) {
                pAttacker->pCurrentProcess->EnterCombat(
                    pAttacker,
                    pTarget,
                    true,
                    true,
                    nullptr,
                    false,
                    false,
                    bAgressor,
                    0,
                    true,
                    false,
                    true,
                    nullptr
                );
            }

            if (!bAgressor) {
                pPlayer->SetPlayerResistingArrest(true);
                pAttacker->pCurrentProcess->GuardCallforHelp(pAttacker, pTarget);
            }
            if (pAttacker->GetCombatController()) {
                if (pAttacker->pCurrentProcess->GetCurrentPackageSpell()) {
                    MagicItem* pSpell = pAttacker->pCurrentProcess->GetCurrentPackageSpell();
                    pAttacker->CastSpell(pSpell, pAttacker, false);
                }
            }
        }
    }
    return true;
}

bool Script::AddSpellFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    SpellItem* pSpell = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pSpell
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pTarget = dynamic_cast<Actor*>(apRefObject);
        if (pTarget) {
            if (pTarget->AddSpell(pSpell)) {
                afReturnValue = 1.0;
                const char* pName = pTarget->GetFullName();
                Interface::PrintLine("Actor Effect '%s' added to %s", pSpell->GetFullName(), pName);
                if (pTarget == pPlayer) {
                    BSStringT<char> kMessage;
                    const char* pAddedText = sAddItemtoSpellList.String();
                    kMessage.SPrintF("%s %s", pSpell->GetFullName(), pAddedText);
                    BSStringT<char> kIcon;
                    kIcon.SPrintF("%s", "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
                    Interface::ShowMessage(kMessage.pString, 0, kIcon.pString, nullptr, 2.0f, false);
                }
            }
            else {
                const char* pName = pTarget->GetFullName();
                Interface::PrintLine("Actor Effect '%s' not added to %s", pSpell->GetFullName(), pName);
            }
        }
    }
    return true;
}

bool Script::StreamMusicFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cFileName[516];
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        cFileName
    );
    return true;
}

// Prints the low-level path from the player to the reference to the console and shows a magenta line from the
// player to the first door (or to the reference) for 30 seconds.
// The exe's cleanup-only C++ EH frame is left out.
bool Script::CalcLowPathToPointFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    using NavMeshInfoArray_t = BSSimpleArray<const NavMeshInfo*, 1024>;
    using RefArray_t = BSSimpleArray<TESObjectREFR*, 1024>;
    int32_t iIgnoreLocks = 0;
    int32_t iAllowDisabledDoors = 0;
    int32_t iIgnoreMinUse = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iIgnoreLocks,
            &iAllowDisabledDoors,
            &iIgnoreMinUse
        )) {
        return false;
    }
    TeleportPath kPath;
    ActorLockData kActorData(pPlayer);
    PathingLocation kStart(pPlayer);
    PathingLocation kDest(apRefObject);
    if (Pathing::BuildLowPath(
            kStart,
            kDest,
            &kPath,
            &kActorData,
            static_cast<Pathing::DOOR_LOCK_DISPOSITION>(iIgnoreLocks != 0)
        )) {
        int32_t iStep;
        for (iStep = 0; iStep < kPath.Doors.iSize; ++iStep) {
            if (kPath.Spaces.GetAt(iStep).bWorldSpace) {
                TESWorldSpace* pWorldSpace = kPath.Spaces.GetAt(iStep).pWorldSpace;
                int32_t iFormID = kPath.Spaces.GetAt(iStep).pWorldSpace->iFormID;
                Interface::PrintLine("Worldspace %s (%08x)", pWorldSpace->GetFormEditorID(), iFormID);
            }
            else {
                TESObjectCELL* pInteriorCell = kPath.Spaces.GetAt(iStep).pInteriorCell;
                int32_t iFormID = kPath.Spaces.GetAt(iStep).pInteriorCell->iFormID;
                Interface::PrintLine("Interior cell %s (%08x)", pInteriorCell->GetFormEditorID(), iFormID);
            }
            PathingLocation kFrom(
                iStep ? PathingLocation(
                            &kPath.Doors.GetAt(iStep - 1).DoorTeleportLocation,
                            kPath.Spaces.GetAt(iStep).pInteriorCell,
                            kPath.Spaces.GetAt(iStep).pWorldSpace
                        )
                      : PathingLocation(pPlayer)
            );
            TESWorldSpace* pToWorldSpace = kPath.Spaces.GetAt(iStep).pWorldSpace;
            TESObjectCELL* pToCell = kPath.Spaces.GetAt(iStep).pInteriorCell;
            const NiPoint3* pToPos = &kPath.Doors.GetAt(iStep).pDoor->GetLinkedDoorTeleportPosition();
            PathingLocation kTo(pToPos, pToCell, pToWorldSpace);
            char cFound = 1;
            if (kFrom.ResolveNavMeshInfo(nullptr)) {
                if (kTo.ResolveNavMeshInfo(nullptr)) {
                    NavMeshInfoSearch kSearch;
                    NavMeshInfoArray_t kNavMeshInfos;
                    RefArray_t kDoors;
                    Pathing::DOOR_LOCK_DISPOSITION eLockDisposition = static_cast<Pathing::DOOR_LOCK_DISPOSITION>(
                        iIgnoreLocks != 0
                    );
                    const NavMeshInfo* pToInfo = kTo.GetBestNavMeshInfo();
                    NiPoint3 kToPos(kTo.GetLocation());
                    const NavMeshInfo* pFromInfo = kFrom.GetBestNavMeshInfo();
                    if (kSearch.Pathfind(
                            &kActorData,
                            kFrom.GetLocation(),
                            pFromInfo,
                            kToPos,
                            pToInfo,
                            &kNavMeshInfos,
                            &kDoors,
                            eLockDisposition
                        )) {
                        for (uint32_t uiIndex = 0; uiIndex < kDoors.iSize; ++uiIndex) {
                            if (kDoors.GetAt(uiIndex)) {
                                cFound = 0;
                            }
                        }
                    }
                    else {
                        cFound = 0;
                    }
                }
                else {
                    cFound = 0;
                }
            }
            else {
                cFound = 0;
            }
            if (!cFound) {
                if (iStep) {
                    TESObjectREFR* pDoor = kPath.Doors.GetAt(iStep).pDoor;
                    TESObjectREFR* pPrevDoor = kPath.Doors.GetAt(iStep - 1).pDoor;
                    int32_t iDoorFormID = kPath.Doors.GetAt(iStep).pDoor->iFormID;
                    const char* pDoorName = pDoor->GetFormEditorID();
                    int32_t iPrevDoorFormID = kPath.Doors.GetAt(iStep - 1).pDoor->iFormID;
                    Interface::PrintLine(
                        "Could not find navmesh path from Door %s (0x%08x) to Door %s (0x%08x)",
                        pPrevDoor->GetFormEditorID(),
                        iPrevDoorFormID,
                        pDoorName,
                        iDoorFormID
                    );
                }
                else {
                    TESObjectREFR* pDoor = kPath.Doors.GetAt(0).pDoor;
                    int32_t iDoorFormID = kPath.Doors.GetAt(iStep).pDoor->iFormID;
                    Interface::PrintLine(
                        "Could not find navmesh path from the Start to Door %s (0x%08x)",
                        pDoor->GetFormEditorID(),
                        iDoorFormID
                    );
                }
                Interface::PrintLine("Make sure that the navmeshes on the way are finalized and connected");
            }
            NiPoint3 kTravel;
            if (iStep) {
                const NiPoint3& kDoorPos = kPath.Doors.GetAt(iStep).pDoor->GetLocationOnReference();
                kTravel = kPath.Doors.GetAt(iStep - 1).DoorTeleportLocation - kDoorPos;
            }
            else {
                const NiPoint3& kDoorPos = kPath.Doors.GetAt(0).pDoor->GetLocationOnReference();
                kTravel = kPath.Start - kDoorPos;
            }
            float fTravel = kTravel.Length();
            Interface::PrintLine("Travel %f units", fTravel);
            TESObjectREFR* pDoor = kPath.Doors.GetAt(iStep).pDoor;
            DoorTeleportData* pTeleport = pDoor->GetTeleport();
            BSStringT<char> kFlags;
            if (pDoor->GetLock() && pDoor->GetLock()->IsLocked()) {
                kFlags += "-Locked";
            }
            if (pDoor->GetDisabled() || pTeleport->pLinkedDoor->GetDisabled()) {
                kFlags += "-Disabled";
            }
            if (static_cast<TESObjectDOOR*>(pDoor->GetObjectReference())->TESObjectDOOR::IsMinimalUse() ||
                static_cast<TESObjectDOOR*>(pTeleport->pLinkedDoor->GetObjectReference())
                    ->TESObjectDOOR::IsMinimalUse()) {
                kFlags += "-MinUse";
            }
            TESObjectREFR* pActivateDoor = kPath.Doors.GetAt(iStep).pDoor;
            const char* pFlags = kFlags.String();
            int32_t iFormID = kPath.Doors.GetAt(iStep).pDoor->iFormID;
            Interface::PrintLine("Activate Door %s (%08x) (%s)", pActivateDoor->GetFormEditorID(), iFormID, pFlags);
        }
        if (kPath.Spaces.GetAt(iStep).bWorldSpace) {
            TESWorldSpace* pWorldSpace = kPath.Spaces.GetAt(iStep).pWorldSpace;
            int32_t iFormID = kPath.Spaces.GetAt(iStep).pWorldSpace->iFormID;
            Interface::PrintLine("Worldspace %s (%08x)", pWorldSpace->GetFormEditorID(), iFormID);
        }
        else {
            TESObjectCELL* pInteriorCell = kPath.Spaces.GetAt(iStep).pInteriorCell;
            int32_t iFormID = kPath.Spaces.GetAt(iStep).pInteriorCell->iFormID;
            Interface::PrintLine("Interior cell %s (%08x)", pInteriorCell->GetFormEditorID(), iFormID);
        }
        PathingLocation kFrom(
            iStep ? PathingLocation(
                        &kPath.Doors.GetAt(iStep - 1).DoorTeleportLocation,
                        kPath.Spaces.GetAt(iStep).pInteriorCell,
                        kPath.Spaces.GetAt(iStep).pWorldSpace
                    )
                  : PathingLocation(pPlayer)
        );
        PathingLocation kGoal(apRefObject);
        char cFound = 1;
        if (kFrom.ResolveNavMeshInfo(nullptr)) {
            if (kGoal.ResolveNavMeshInfo(nullptr)) {
                NavMeshInfoSearch kSearch;
                NavMeshInfoArray_t kNavMeshInfos;
                RefArray_t kDoors;
                Pathing::DOOR_LOCK_DISPOSITION eLockDisposition = static_cast<Pathing::DOOR_LOCK_DISPOSITION>(
                    iIgnoreLocks != 0
                );
                const NavMeshInfo* pGoalInfo = kGoal.GetBestNavMeshInfo();
                NiPoint3 kGoalPos(kGoal.GetLocation());
                const NavMeshInfo* pFromInfo = kFrom.GetBestNavMeshInfo();
                if (!kSearch.Pathfind(
                        &kActorData,
                        kFrom.GetLocation(),
                        pFromInfo,
                        kGoalPos,
                        pGoalInfo,
                        &kNavMeshInfos,
                        &kDoors,
                        eLockDisposition
                    )) {
                    cFound = 0;
                }
            }
            else {
                cFound = 0;
            }
        }
        else {
            cFound = 0;
        }
        if (!cFound) {
            if (iStep) {
                // Names the last door (a departure): the exe passes the format alone, so its %s and %08x read
                // arguments nothing wrote. The door is the one the path leaves from, as in the door-to-door message.
                TESObjectREFR* pPrevDoor = kPath.Doors.GetAt(iStep - 1).pDoor;
                Interface::PrintLine(
                    "Could not find navmesh path from Door %s (0x%08x) to Goal",
                    pPrevDoor->GetFormEditorID(),
                    pPrevDoor->iFormID
                );
            }
            else {
                Interface::PrintLine("Could not find navmesh path from the Start to Goal");
            }
            Interface::PrintLine("Make sure that the navmeshes on the way are finalized and connected");
        }
        float fGoalTravel;
        if (kPath.Doors.IsEmpty()) {
            fGoalTravel = (kPath.Start - kPath.End).Length();
        }
        else {
            fGoalTravel = (kPath.Doors.GetAt(iStep - 1).DoorTeleportLocation - kPath.End).Length();
        }
        Interface::PrintLine("Travel %f units", fGoalTravel);
        float fTotalDistance = kPath.ComputeLength();
        TESGlobal* pTimeScale = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(58u));
        double dCurrentSpeed = pPlayer->GetCurrentSpeed();
        float fHours = fTotalDistance / dCurrentSpeed;
        fHours = fHours / 3600.0;
        double dTimeScale = pTimeScale->fValue;
        fHours = dTimeScale * fHours;
        Interface::PrintLine("--Total distance: %.0f units.", fTotalDistance);
        Interface::PrintLine("--Estimated Travel Time: %.2f game hours.", fHours);
        if (kPath.Doors.IsEmpty()) {
            const NiPoint3& kRefPos = apRefObject->GetLocationOnReference();
            const NiPoint3& kPlayerPos = pPlayer->GetLocationOnReference();
            NiAVObject* pLine = MakeLine(kPlayerPos, NiColorA::Magenta, kRefPos, NiColorA::Magenta, true);
            TES::pInstance->AddTempDebugObject(pLine, 30.0f);
        }
        else {
            const NiPoint3& kDoorPos = kPath.Doors.GetAt(0).pDoor->GetLocationOnReference();
            const NiPoint3& kPlayerPos = pPlayer->GetLocationOnReference();
            NiAVObject* pLine = MakeLine(kPlayerPos, NiColorA::Magenta, kDoorPos, NiColorA::Magenta, true);
            TES::pInstance->AddTempDebugObject(pLine, 30.0f);
        }
    }
    else {
        Interface::PrintLine("No Path found.");
    }
    return true;
}

bool Script::ShowQuestVarsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* pQuest = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest
        )) {
        return false;
    }
    if (pQuest) {
        if (pQuest->pFormScript) {
            Script* pOwnerScript = pQuest->pFormScript;
            ScriptLocals* pOwnerLocals = pQuest->GetScriptLocals();
            if (pOwnerScript->GetVariableList()->IsEmpty()) {
                Interface::PrintLine("No script variables on quest %s", pQuest->GetEditorName());
            }
            else {
                BSSimpleList<ScriptVariable*>* pList = pOwnerScript->GetVariableList();
                while (pOwnerLocals && pList && pList->m_item) {
                    ScriptVariable* pVar = pList->m_item;
                    pList = pList->m_pkNext;
                    if (pOwnerLocals->IsReferenceVariable(pVar->data.uiID)) {
                        uint32_t uiID = 0;
                        double dVal = pOwnerLocals->GetVariable(pVar->data.uiID, nullptr);
                        Script::GetNumericIDFromDouble(uiID, dVal);
                        const char* pVarName = pVar->cName.String();
                        Interface::PrintLine("%s->%s = (%08X)", pOwnerScript->GetFormEditorID(), pVarName, uiID);
                    }
                    else if (static_cast<uint8_t>(pVar->data.bIsInteger)) {
                        // Tests the whole byte; a bool read would test bit 0 only.
                        int32_t iValue = static_cast<int32_t>(pOwnerLocals->GetVariable(pVar->data.uiID, nullptr));
                        const char* pVarName = pVar->cName.String();
                        Interface::PrintLine("%s->%s = %d", pOwnerScript->GetFormEditorID(), pVarName, iValue);
                    }
                    else {
                        double dValue = pOwnerLocals->GetVariable(pVar->data.uiID, nullptr);
                        const char* pVarName = pVar->cName.String();
                        Interface::PrintLine("%s->%s = %0.4f", pOwnerScript->GetFormEditorID(), pVarName, dValue);
                    }
                }
            }
        }
        Interface::PrintLine("--- Quest state -----------------------------");
        const char* pRunning = pQuest->GetEnabled() ? "Yes" : "No";
        Interface::PrintLine("Running?       %s", pRunning);
        Interface::PrintLine("Current stage: %d", pQuest->GetCurrentStage());
        Interface::PrintLine("Priority:      %d", pQuest->GetPriority());
    }
    return true;
}

bool Script::ShowAnimFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject && apRefObject->GetAnimation() && apRefObject->IsActor()) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor) {
            Interface::PrintLine("--- Actor Variables -----------------------------");
            if (pActor->GetCurrentProcessType() != PROCESS_TYPE_NONE) {
                Interface::PrintLine("Process Level: %s", pProcessTypeString[pActor->GetCurrentProcessType()]);
            }
            BaseProcess* pProcess = pActor->pCurrentProcess;
            int32_t iWeaponDrawn = pProcess->GetWeaponDrawn();
            Interface::PrintLine(
                "Wants Weapon Drawn %d, Weapon Drawn %d",
                pProcess->GetWantWeaponDrawn(),
                iWeaponDrawn
            );
            if (pActor->GetAnimAction() != ANIM_ACTION_NONE) {
                Interface::PrintLine("Animation Action: %s", pAnimationActionStrings[pActor->GetAnimAction()]);
            }
            Interface::PrintLine("Life State: %s", LifeStateString[pActor->GetLifeState()]);
            Interface::PrintLine("Sit/Sleep State: %s", SleepStateString[pActor->pCurrentProcess->GetSitSleepState()]);
            Interface::PrintLine("Knock State: %s", KnockStateString[pActor->pCurrentProcess->GetKnockState()]);
            const char* pHasRagDoll = pActor->HasRagDoll() ? "Yes" : "No";
            Interface::PrintLine("Has RagDoll: %s", pHasRagDoll);
            if (pActor->IsInCombat() && pActor->GetCombatTarget()) {
                int32_t iFormID = pActor->GetCombatTarget()->iFormID;
                Interface::PrintLine("In Combat with \"%s\" (%08x)", pActor->GetCombatTarget()->GetFullName(), iFormID);
            }

            if (pActor->GetCurrentPackageTarget()) {
                int32_t iFormID = pActor->GetCurrentPackageTarget()->iFormID;
                Interface::PrintLine(
                    "Current package target \"%s\" (%08x)",
                    pActor->GetCurrentPackageTarget()->GetFullName(),
                    iFormID
                );
            }
        }
        Animation* pAnimation = apRefObject->GetAnimation();
        if (pAnimation) {
            Interface::PrintLine("--- Animation -------------------------------");
            Interface::PrintLine("Anims Loading: %d", pAnimation->GetAnimsLoadingCount());
            for (ANIM_GROUP_SECTION eSection = AGS_IDLE; eSection < ANIM_GROUP_SECTION_COUNT;
                 eSection = static_cast<ANIM_GROUP_SECTION>(eSection + 1)) {
                if (pAnimation->GetCurrentSequence(eSection)) {
                    uint16_t eGroup = pAnimation->GetGroup(eSection);
                    const char* pName = animGroupInfo[TESAnimGroup::GetType(eGroup)].pName;
                    const char* pWeapon = animWeaponInfo[TESAnimGroup::GetWeapon(eGroup)];
                    Interface::PrintLine(
                        "%s -> %s/%s/%s",
                        animGroupSectionNames[eSection],
                        animMoveInfo[TESAnimGroup::GetMove(eGroup)],
                        pWeapon,
                        pName
                    );
                }
            }
            if (pAnimation->spAnimIdle.m_pObject) {
                Interface::PrintLine("IdleAnim: %s", pAnimation->spAnimIdle->pIdleForm->GetFormEditorID());
            }
            if (pAnimation->spAnimIdleQueued.m_pObject) {
                Interface::PrintLine("IdleAnim Queued: %s", pAnimation->spAnimIdleQueued->pIdleForm->GetFormEditorID());
            }
        }
    }

    return true;
}

bool Script::EnableFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t bFadeIn = 1;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &bFadeIn
        )) {
        return false;
    }
    if (!bFadeIn) {
        apRefObject->SetDisableFade(true);
    }
    if (apRefObject) {
        if (apRefObject->GetEnableStateParent()) {
            if (apScript && apScript->iFormID) {
                const char* pName = apRefObject->GetFormEditorIDLength() ? apRefObject->GetFormEditorID()
                                                                         : apRefObject->GetFullName();
                const char* pScriptName = apScript->GetFormEditorID();
                int32_t iScriptFormID = apScript->iFormID;
                Warning(
                    "SCRIPTS: Enable is being called on reference %08X %s in script %08X %s even though it has an "
                    "enable state parent.  This is not valid behavior and will be ignored.",
                    apRefObject->iFormID,
                    pName,
                    iScriptFormID,
                    pScriptName
                );
            }
            else {
                const char* pName = apRefObject->GetFormEditorIDLength() ? apRefObject->GetFormEditorID()
                                                                         : apRefObject->GetFullName();
                const char* pScriptName = apScript->GetFormEditorID();
                int32_t iScriptFormID = apScript->iFormID;
                Warning(
                    "SCRIPTS: Enable is being called on reference %08X %s in a results script even though it has an "
                    "enable state parent.  This is not valid behavior and will be ignored.",
                    apRefObject->iFormID,
                    pName,
                    iScriptFormID,
                    pScriptName
                );
            }
            return true;
        }
        BSFadeNode* pFade;
        NiAVObject* pAV;
        Script::RemoveDelayedScriptActionReference(apRefObject);
        if (apRefObject->GetDisabled()) {
            Script::AddPendingEnabledReference(apRefObject);
        }
        pAV = apRefObject->Get3D();
        pFade = pAV ? pAV->IsFadeNode() : nullptr;
        if (pFade && pFade->GetMultType() == LOD_MULT_INVISIBLE) {
            pFade->eMultType = TES::GetLODMult(apRefObject->GetObjectReference());
        }

        if (apRefObject->GetIsInvisibleLODMultType()) {
            apRefObject->SetIsInvisibleLODMultType(false);
        }
    }
    apScriptLocals->m_cFlags &= ~ScriptLocals::FLAG_DISABLED;
    return true;
}

bool Script::DisableFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t bFadeOut = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &bFadeOut
        )) {
        return false;
    }
    if (apRefObject && !apRefObject->GetDisabled() && !apRefObject->GetTemporary()) {
        if (apRefObject->GetEnableStateParent()) {
            if (apScript && apScript->iFormID) {
                const char* pName = apRefObject->GetFormEditorIDLength() ? apRefObject->GetFormEditorID()
                                                                         : apRefObject->GetFullName();
                const char* pScriptName = apScript->GetFormEditorID();
                int32_t iScriptFormID = apScript->iFormID;
                Warning(
                    "SCRIPTS: Disable is being called on reference %08X %s in script %08X %s even though it has an "
                    "enable state parent.  This is not valid behavior and will be ignored.",
                    apRefObject->iFormID,
                    pName,
                    iScriptFormID,
                    pScriptName
                );
            }
            else {
                const char* pName = apRefObject->GetFormEditorIDLength() ? apRefObject->GetFormEditorID()
                                                                         : apRefObject->GetFullName();
                const char* pScriptName = apScript->GetFormEditorID();
                int32_t iScriptFormID = apScript->iFormID;
                Warning(
                    "SCRIPTS: Disable is being called on reference %08X %s in a results script even though it has an "
                    "enable state parent.  This is not valid behavior and will be ignored.",
                    apRefObject->iFormID,
                    pName,
                    iScriptFormID,
                    pScriptName
                );
            }
            return true;
        }
        if (bFadeOut > 0) {
            NiNode* pRootNode = apRefObject->Get3D();
            BSFadeNode* pFadeNode = pRootNode ? pRootNode->IsFadeNode() : nullptr;
            if (pFadeNode) {
                pFadeNode->ForceLODMultType(LOD_MULT_INVISIBLE);
            }
            if (!apRefObject->GetIsInvisibleLODMultType()) {
                apRefObject->SetIsInvisibleLODMultType(true);
            }
        }
        Script::AddPendingDisabledReference(apRefObject, bFadeOut != 0);
    }
    apScriptLocals->m_cFlags |= ScriptLocals::FLAG_DISABLED;
    return true;
}

bool Script::SaveGame(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iOutputStats = 0;
    char cFileName[516];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cFileName,
            &iOutputStats
        )) {
        return false;
    }
    pSaveLoadManager->SaveGame(cFileName, -1, iOutputStats != 0);
    return true;
}

bool Script::CopySaveGames(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cOption[516];
    ourStrcpy(cOption, "");
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cOption
        )) {
        return false;
    }
    if (pSaveLoadManager) {
        if (BSstricmp(cOption, "ms")) {
            pSaveLoadManager->CopySaveGames(false);
        }
        else {
            pSaveLoadManager->CopySaveGames(true);
        }
    }
    return true;
}

bool Script::DumpModelMap(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cFileName[265];
    char cSuffix[512];
    cSuffix[0] = 0;
    if (apParameters &&
        !Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cSuffix
        )) {
        return false;
    }
    MessageHandler::IncDisableWarningCount(true);
    Warning("MODELS: <<< DUMPMODELMAP results");
    BSsprintf(&cFileName[1], 260, "ModelDump%s.txt", cSuffix);
    BSFile outfile(&cFileName[1], NiFile::WRITE_ONLY, 0x4000u, false);
    outfile.BSFile::Open(false, false);
    if (pModelLoader) {
        pModelLoader->OutputModelMapContents(&outfile, false);
    }
    if (BSFaceGenManager::GetModelCache()) {
        BSFaceGenManager::GetModelCache()->OutputModelMapContents(&outfile);
    }
    outfile.Close();
    Warning("MODELS: >>> DUMPMODELMAP results");
    MessageHandler::IncDisableWarningCount(false);
    return true;
}

bool Script::SexChange(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iSex = -1;
    int32_t iUseDefaultPreset = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iSex,
            &iUseDefaultPreset
        )) {
        return false;
    }
    Character* pActor = pPlayer;
    if (apRefObject && apRefObject->IsNpc()) {
        pActor = static_cast<Character*>(apRefObject);
    }

    if (pActor) {
        TESActorBase::SEX eCurrentSex = pActor->GetSex();
        if (eCurrentSex == iSex) {
            return true;
        }
        TESNPC* pNPC = static_cast<TESNPC*>(pActor->GetObjectReference());
        TESActorBase::SEX eSex = TESActorBase::SEX_MALE;
        if (iSex == -1) {
            if (pActor->GetSex() == TESActorBase::SEX_MALE) {
                eSex = TESActorBase::SEX_FEMALE;
            }
        }
        else if (iSex == 1) {
            eSex = TESActorBase::SEX_FEMALE;
        }
        pNPC->SetFlagBit(TESActorBaseData::BASE_FEMALE, eSex == TESActorBase::SEX_FEMALE, true);
        if (pActor->IsPc() && iUseDefaultPreset) {
            using presets_t = BSSimpleArray<TESNPC*, 1024>;
            presets_t presets;
            TESActorBase::SEX eNPCSex = pNPC->GetSex();
            TESRace* pRace = pNPC->GetRace();
            TESNPC::FillCharGenPresetList(pRace, eNPCSex, &presets);
            pNPC->CopyHeadData(presets.GetAt(0));
        }

        pActor->Reset3D();
        if (bSystemOutput) {
            BSStringT<char> message;
            char* pSexName = sexName[eSex];
            int32_t iFormID = pActor->iFormID;
            message.SPrintF("\"%s\" (%08x) is now %s", pActor->GetFullName(), iFormID, pSexName);
            Interface::PrintLine(static_cast<const char*>(message.pString));
        }
    }
    return true;
}

bool Script::IsCurrentFurnitureRef(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pFurnitureRef = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pFurnitureRef
           ) &&
        Script::IsCurrentFurnitureRefConditionFunction(apRefObject, pFurnitureRef, nullptr, afReturnValue);
}

bool Script::IsCurrentFurnitureObj(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pFurniture = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pFurniture
           ) &&
        Script::IsCurrentFurnitureObjConditionFunction(apRefObject, pFurniture, nullptr, afReturnValue);
}

bool Script::GetTalkedToPCParam(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pActor = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pActor
           ) &&
        Script::GetTalkedToPCConditionFunction(nullptr, pActor, nullptr, afReturnValue);
}

bool Script::SetStageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    TESQuest* pQuest = nullptr;
    int32_t iStage = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest,
            &iStage
        )) {
        return false;
    }
    if (pQuest && pQuest->SetStageDone(iStage)) {
        afReturnValue = 1.0;
    }

    return true;
}

bool Script::GetIsCurrentWeatherFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pWeather = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pWeather
           ) &&
        Script::GetIsCurrentWeatherConditionFunction(apRefObject, pWeather, nullptr, afReturnValue);
}

bool Script::GetLockLevelFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetLockLevelConditionFunction(apRefObject, apScriptLocals, nullptr, afReturnValue);
}

bool Script::GetIsVoiceTypeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pVoiceType = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pVoiceType
           ) &&
        Script::GetIsVoiceTypeConditionFunction(apRefObject, pVoiceType, nullptr, afReturnValue);
}

bool Script::SameFaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::SameFactionConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::SameRace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::SameRaceConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::SameSex(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::SameSexConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::SameRaceAsPC(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::SameRaceConditionFunction(apRefObject, pPlayer, nullptr, afReturnValue);
}

bool Script::SameSexAsPC(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::SameSexConditionFunction(apRefObject, pPlayer, nullptr, afReturnValue);
}

bool Script::GetQuestCompleted(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pQuest = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pQuest
           ) &&
        Script::GetQuestCompletedConditionFunction(apRefObject, pQuest, nullptr, afReturnValue);
}

bool Script::GetStage(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pQuest = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pQuest
           ) &&
        Script::GetStageConditionFunction(apRefObject, pQuest, nullptr, afReturnValue);
}

bool Script::GetStageDone(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pQuest = nullptr;
    void* pStage = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pQuest,
               &pStage
           ) &&
        Script::GetStageDoneConditionFunction(apRefObject, pQuest, pStage, afReturnValue);
}

bool Script::GetFactionRankDifference(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pFaction = nullptr;
    void* pActor = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pFaction,
               &pActor
           ) &&
        Script::GetFactionRankDifferenceConditionFunction(apRefObject, pFaction, pActor, afReturnValue);
}

bool Script::GetInFaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pFaction = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pFaction
           ) &&
        Script::GetInFactionConditionFunction(apRefObject, pFaction, nullptr, afReturnValue);
}

bool Script::GetPCIsClass(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pClass = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pClass
           ) &&
        Script::GetIsClassConditionFunction(pPlayer, pClass, nullptr, afReturnValue);
}

bool Script::GetPCIsRace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pRace = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pRace
           ) &&
        Script::GetIsRaceConditionFunction(pPlayer, pRace, nullptr, afReturnValue);
}

bool Script::GetGlobalValue(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pGlobal = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pGlobal
           ) &&
        Script::GetGlobalValueConditionFunction(apRefObject, pGlobal, nullptr, afReturnValue);
}

bool Script::GetShouldAttack(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetShouldAttackConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::GetInCell(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pCell = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pCell
           ) &&
        Script::GetInCellConditionFunction(apRefObject, pCell, nullptr, afReturnValue);
}

bool Script::GetInCellParam(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pRef = nullptr;
    void* pCell = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pCell,
               &pRef
           ) &&
        Script::GetInCellConditionFunction(apRefObject, pCell, pRef, afReturnValue);
}

bool Script::GetPCInFaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pFaction = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pFaction
           ) &&
        Script::GetInFactionConditionFunction(pPlayer, pFaction, nullptr, afReturnValue);
}

bool Script::GetIsID(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pObject = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pObject
           ) &&
        Script::GetIsIDConditionFunction(apRefObject, pObject, nullptr, afReturnValue);
}

bool Script::IsInListFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pList = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pList
           ) &&
        Script::IsInListConditionFunction(apRefObject, pList, nullptr, afReturnValue);
}

bool Script::GetIsUsedItem(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pItem = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pItem
           ) &&
        Script::GetIsUsedItemConditionFunction(apRefObject, pItem, nullptr, afReturnValue);
}

bool Script::GetIsRef(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pRef = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pRef
           ) &&
        Script::GetIsRefConditionFunction(apRefObject, pRef, nullptr, afReturnValue);
}

bool Script::GetFactionRank(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pFaction = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pFaction
           ) &&
        Script::GetFactionRankConditionFunction(apRefObject, pFaction, nullptr, afReturnValue);
}

bool Script::SetFactionRank(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction = nullptr;
    int32_t iRank = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction,
            &iRank
        )) {
        return false;
    }
    afReturnValue = 0.0;
    Actor* pAct = dynamic_cast<Actor*>(apRefObject);
    if (!pAct) {
        return true;
    }
    if (pAct && pFaction) {
        pAct->SetFactionRank(pFaction, iRank);
    }

    return true;
}

bool Script::ModDisposition(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pTarget = nullptr;
    int32_t iValue = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTarget,
            &iValue
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (!pActor || !pTarget) {
        return false;
    }
    pActor->ModifyDispositionTowardActor(pTarget, static_cast<float>(iValue));
    return Script::GetDispositionConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::GetDeadCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pActorBase = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pActorBase
           ) &&
        Script::GetDeadCountConditionFunction(apRefObject, pActorBase, nullptr, afReturnValue);
}

bool Script::ShowMap(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cShowMessage = 0;
    TESObjectREFR* pMarker = nullptr;
    int32_t iEnableTravel = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pMarker,
            &iEnableTravel
        )) {
        return false;
    }
    if (pMarker && pMarker->GetMapMarkerData()) {
        if (!pMarker->GetMapMarkerData()->GetVisible()) {
            cShowMessage = 1;
        }
        pMarker->GetMapMarkerData()->SetIsVisible(1);
        pMarker->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        if (iEnableTravel) {
            if (!pMarker->GetMapMarkerData()->GetTravelLoc()) {
                cShowMessage = 1;
            }
            pMarker->GetMapMarkerData()->SetTravelLoc(true);
        }
    }

    if (cShowMessage) {
        Interface::ShowMessage(
            sMapMarkerAdded.String(),
            0,
            "Interface\\Icons\\Message Icons\\glow_message_map.dds",
            "UIPopUpMapMarkerAdded",
            2.0f,
            false
        );
    }
    afReturnValue = 1.0;
    return true;
}

// SayToCallBack is passed as the callback, with the reference's form ID as the callback's context.
bool Script::SayFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    TESTopic* pTopic = nullptr;
    int32_t iSubtitle = 0;
    TESActorBase* pNpc = nullptr;
    int32_t iPCHead = 0;
    int32_t iAlwaysPlay = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTopic,
            &iSubtitle,
            &pNpc,
            &iPCHead,
            &iAlwaysPlay
        )) {
        return false;
    }
    bool bPlayerHead = iPCHead != 0;
    bool bForceSub = iSubtitle > 0;
    bool bAlwaysPlay = iAlwaysPlay > 0;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    int32_t iOldLipDistance = iSpeakSoundLipDistance.Int();
    iSpeakSoundLipDistance = INT32_MAX;
    if (pTopic) {
        if (!pActor) {
            if (apRefObject && pNpc) {
                TESObjectREFR* pTempActor = TESObjectREFR::CreateReference(ACHR_ID, false);
                pTempActor->SetTemporary();
                pTempActor->SetObjectReference(pNpc);
                pTempActor->SetLocationOnReference(apRefObject->GetLocationOnReference());
                apRefObject->SetSayToTopic(pTopic);
                DialogueItem* pVoice = pTopic->CreateDialogueItem(pTempActor, nullptr, nullptr, nullptr, false);
                if (pVoice) {
                    apRefObject->SetSayToTopicInfo(pVoice->pInfo);
                }
                BSSoundHandle kSoundHand = pTempActor->VoiceSoundFunction(
                    pTopic,
                    static_cast<Actor*>(pTempActor),
                    bPlayerHead,
                    bAlwaysPlay,
                    false,
                    false,
                    bForceSub
                );
                kSoundHand.SetCompletionCallback(
                    &TESObjectREFR::SayToCallBack,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(apRefObject->iFormID))
                );
                if (pTempActor) {
                    delete pTempActor;
                }
            }
            else if (
                apRefObject->Get3D() ||
                (apRefObject->IsMobileObject() && static_cast<MobileObject*>(apRefObject)->pTalkingActivator->Get3D())
            ) {
                apRefObject->SetSayToTopic(pTopic);
                DialogueItem* pVoice = pTopic->CreateDialogueItem(apRefObject, nullptr, nullptr, nullptr, false);
                if (pVoice) {
                    apRefObject->SetSayToTopicInfo(pVoice->pInfo);
                }
                BSSoundHandle kSoundHand = apRefObject->VoiceSoundFunction(
                    pTopic,
                    nullptr,
                    bPlayerHead,
                    bAlwaysPlay,
                    false,
                    false,
                    bForceSub
                );
                kSoundHand.SetCompletionCallback(
                    &TESObjectREFR::SayToCallBack,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(apRefObject->iFormID))
                );
            }
        }
        else if (!pActor->pCurrentProcess || pActor->IsDead(true)) {
            if (pActor->pCurrentProcess) {
                Script::SetActionFlag(pTopic, &pActor->m_Extra, SCRIPT_ACTION_ONSAYTODONE);
                pActor->pCurrentProcess->SetDoingSayTo(false);
            }
        }
        else {
            pActor->fSayToFailVoiceTimer = -1.0f;
            pActor->pCurrentProcess->SetWaitingforLipFile(false);
            pActor->pCurrentProcess->ClearDialogTarget();
            if (pActor->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
                TESObjectREFR* pCurrentScriptTarget;
                pCurrentScriptTarget = pActor->pCurrentProcess->GetCurrentHeadTrackTarget(
                    BaseProcess::HEAD_TRACK_TYPE_SCRIPT
                );
                pActor->pCurrentProcess->ClearScriptHeadTrackTarget(false);
                pActor->pCurrentProcess->SetActionHeadTrackTarget(pCurrentScriptTarget);
                pActor->pCurrentProcess->ClearGreetingInfoData();
            }
            else if (iPCHead) {
                pActor->SetSayToTopic(pTopic);
                DialogueItem* pVoice = pTopic->CreateDialogueItem(pActor, nullptr, nullptr, nullptr, false);
                if (pVoice) {
                    apRefObject->SetSayToTopicInfo(pVoice->pInfo);
                }
                BSSoundHandle
                    kSoundHand = pActor->VoiceSoundFunctionEx(pTopic, pActor, bPlayerHead, bAlwaysPlay, 0, false);
                kSoundHand.SetCompletionCallback(
                    &TESObjectREFR::SayToCallBack,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(apRefObject->iFormID))
                );
                return true;
            }
            if (pPlayer->IsSleepingorResting() ||
                pActor->pCurrentProcess->eLevel ||
                (pActor->GetEssential() && pActor->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN)) {
                DialogueItem* pVoice = pTopic->CreateDialogueItem(pActor, nullptr, nullptr, nullptr, true);
                if (pVoice) {
                    pVoice->FirstResponse();
                    pVoice->RunResult(TIRS_BEGIN);
                    pVoice->RunResult(TIRS_END);
                    Script::SetActionFlag(pTopic, &pActor->m_Extra, SCRIPT_ACTION_ONSAYTODONE);
                    delete pVoice;
                }
            }
            else {
                pActor->pCurrentProcess->ProcessGreet(pActor, pTopic, bForceSub, false, true, true);
            }
        }
    }
    iSpeakSoundLipDistance = iOldLipDistance;
    return true;
}

bool Script::PlayVATSCamerasFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cName[512];
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cName
        )) {
        return false;
    }
    TESForm* pForm = TESForm::GetFormByEditorID(cName);
    pForceCameraPath = dynamic_cast<BGSCameraPath*>(pForm);
    if (bSystemOutput) {
        if (pForceCameraPath) {
            Interface::PrintLine("PlayVATSCameras '%s' queued for next VATS playback.", cName);
        }
        else {
            Interface::PrintLine("PlayVATSCameras '%s' not found.", cName);
        }
    }
    return true;
}

bool Script::ShowQuestLogFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiShowCompleted = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiShowCompleted
        )) {
        return false;
    }
    bool bShowCompleted = uiShowCompleted != 0;
    BSSimpleList<TESQuestStageItem*> kQuestLog;
    TESQuest::GetQuestLog(&kQuestLog, nullptr, bShowCompleted, false, false);
    BSSimpleList<TESQuestStageItem*>* pIter = &kQuestLog;
    while (pIter && pIter->GetItem()) {
        TESQuestStageItem* pItem = pIter->GetItem();
        TESQuest* pQuest = pItem->m_pOwner;
        pIter = pIter->GetNext();
        char cLogEntry[260];
        ourMemset(cLogEntry, 0, sizeof(cLogEntry));
        mbsnbcpy_(cLogEntry, pItem->GetLogEntry(pQuest), sizeof(cLogEntry) - 1);
        Date* pDate = pItem->m_pLogDate;
        Interface::PrintLine("------------------------------------------------");
        uint16_t usYear = pDate->GetYear();
        const char* pMonthName = pGSMonth[pDate->GetMonth()]->String();
        uint16_t usDay = pDate->GetDay();
        Interface::PrintLine("%d of %s, %d", usDay, pMonthName, usYear);
        Interface::PrintLine("%s", pItem->m_pOwner->TESFullName::GetFullName());
        Interface::PrintLine("%s", cLogEntry);
    }
    return true;
}

bool Script::LockFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!apRefObject) {
        return false;
    }
    int32_t iCount = 0;
    int32_t iLockAsOwner = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iCount,
            &iLockAsOwner
        )) {
        return false;
    }
    if (apRefObject->GetAction(OBJECT_ACTION_OPEN)) {
        BGSOpenCloseForm::SetOpenState(apRefObject, false, true);
    }
    LockData* pLock = apRefObject->GetLock();
    if (!pLock) {
        if (apRefObject->GetObjectReference()->GetFormType() == TERM_ID) {
            LOCK_LEVEL eLockLevel;
            BGSTerminal* pTerminal = static_cast<BGSTerminal*>(apRefObject->GetObjectReference());
            if (iCount >= 0) {
                if (iCount < 6) {
                    eLockLevel = static_cast<LOCK_LEVEL>(iCount);
                }
                else {
                    eLockLevel = LL_IMPOSSIBLE;
                }
            }
            else {
                eLockLevel = LL_VERYEASY;
            }
            pTerminal->SetRawDifficulty(apRefObject, eLockLevel);
            TESObjectREFR::SetTerminalUnlocked(apRefObject, 0);
            iCount = eLockLevel;
        }
        else {
            pLock = apRefObject->AddLock();
        }
    }
    if (pLock) {
        if (iCount) {
            pLock->cLockLevel = iCount;
        }
        pLock->SetLocked(true);
        apRefObject->AddLockChange();
        if (iLockAsOwner > 0) {
            DoorTeleportData* pTeleport = apRefObject->GetTeleport();
            if (pTeleport) {
                pTeleport->SetConnectedCellsPublic(apRefObject, false);
            }
        }
        apRefObject->SetAltered(true);
    }
    if (bSystemOutput) {
        Interface::PrintLine("Locked %s with lock level %d", apRefObject->GetFullName(), iCount);
    }
    return true;
}

bool Script::UnlockFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!apRefObject) {
        return false;
    }
    int32_t iUnlockAsOwner = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iUnlockAsOwner
        )) {
        return false;
    }
    LockData* pLock = apRefObject->GetLock();
    if (pLock) {
        pLock->SetLocked(false);
        apRefObject->AddLockChange();
        if (iUnlockAsOwner > 0) {
            DoorTeleportData* pTeleport = apRefObject->GetTeleport();
            if (pTeleport) {
                pTeleport->SetConnectedCellsPublic(apRefObject, true);
            }
        }
        apRefObject->SetAltered(true);
    }
    else if (apRefObject->GetObjectReference()->GetFormType() == TERM_ID) {
        static_cast<BGSTerminal*>(apRefObject->GetObjectReference())->BGSTerminal::SetUnlocked(apRefObject);
        BGSTerminal::SetLockedOut(apRefObject, 0);
    }
    if (bSystemOutput) {
        Interface::PrintLine("Unlocked %s ", apRefObject->GetFullName());
    }
    return true;
}

bool Script::GetCrimeKnownFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    uint32_t uiCrimeType = 0;
    Actor* pCriminal = nullptr;
    Actor* pVictim = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiCrimeType,
            &pCriminal,
            &pVictim
        )) {
        return false;
    }
    PackageNS::CRIME_TYPE eCrimeType = static_cast<PackageNS::CRIME_TYPE>(uiCrimeType);
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (ReferenceProcessLists.IsCrimeinGlobalCrimeList(pVictim, pCriminal, pActor, eCrimeType, nullptr, -1)) {
        afReturnValue = 1.0;
    }
    return true;
}

// The crime type goes in the condition function's untyped second parameter slot as the integer itself (the slot
// holds a form or a value by condition), so the integer is converted to the pointer-sized slot.
bool Script::GetCrimeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    int32_t iCrime = -1;
    void* pActor = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pActor,
            &iCrime
        )) {
        return false;
    }
    if (apRefObject) {
        return Script::GetCrimeConditionFunction(
            apRefObject,
            pActor,
            reinterpret_cast<void*>(static_cast<intptr_t>(iCrime)),
            afReturnValue
        );
    }
    return true;
}

bool Script::MoveToQuestTargetFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iTargetNumber = 1;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iTargetNumber
        )) {
        return false;
    }
    TESObjectREFR* pTargetRef = nullptr;
    if (pPlayer->GetActiveQuest()) {
        BSSimpleList<TESQuestTarget*>* pList = pPlayer->GetCurrentTargetList();
        if (!pList) {
            Interface::PrintLine("No current targets");
        }
        for (int32_t i = 0; pList && pList->m_item && i < iTargetNumber; ++i) {
            TESQuestTarget* pTarget = pList->m_item;
            pList = pList->m_pkNext;
            pTargetRef = pTarget->GetReference(true);
        }
    }
    else {
        Interface::PrintLine("No active quest");
    }
    if (pTargetRef) {
        Script::MoveToFunctionBase(pPlayer, pTargetRef, 0.0f, 0.0f, 0.0f);
    }
    return true;
}

bool Script::WakeUpPCFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iTime = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iTime
        )) {
        return false;
    }
    if (pPlayer->IsSleeping()) {
        if (iTime < 0) {
            iTime = 0;
        }
        pPlayer->SetSleeping(iTime, true);
        if (!iTime) {
            Interface::CloseSleepMenu();
        }
    }
    return true;
}

bool Script::SetCombatStyleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESCombatStyle* pCombatStyle = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pCombatStyle
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor) {
            pActor->SetCombatStyle(pCombatStyle);
            if (pActor->IsPlayerTeammate()) {
                if (pActor->IsCreature()) {
                    static_cast<TESCreature*>(pActor->GetObjectReference())
                        ->TESCreature::InitDefaultWorn(pActor, true, true, true);
                }
                else {
                    TESNPC* pNPC = static_cast<TESNPC*>(pActor->GetObjectReference());
                    pNPC->InitParts(pActor, pActor->GetBiped(), true);
                }
            }
        }
    }
    return true;
}

bool Script::SelectPlayerSpellFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* pParamForm = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pParamForm
        )) {
        return false;
    }
    if (pParamForm) {
        SpellItem* pSpell = dynamic_cast<SpellItem*>(pParamForm);
        TESObjectBOOK* pBook = dynamic_cast<TESObjectBOOK*>(pParamForm);
        if (pBook) {
            pPlayer->SetSelectedScroll(pBook);
        }
        else if (pSpell) {
            pPlayer->SetSelectedSpell(pSpell);
        }
        else {
            pPlayer->SetSelectedSpell(nullptr);
        }
        if (bSystemOutput) {
            const char* pName = pSpell ? pSpell->GetFullName() : "NONE";
            Interface::PrintLine("Player Spell set to %s", pName);
        }
    }
    return true;
}

bool Script::SetEssentialFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    TESActorBase* pActorBase = nullptr;
    int32_t iEssential = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pActorBase,
            &iEssential
        )) {
        return false;
    }
    if (pActorBase) {
        pActorBase->SetEssential(iEssential > 0);
    }
    return true;
}

bool Script::SetQuestObjectFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    TESObject* pObject = nullptr;
    int32_t iQuestObject = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pObject,
            &iQuestObject
        )) {
        return false;
    }
    if (pObject) {
        pObject->SetQuestObject(iQuestObject > 0);
    }
    return true;
}

bool Script::GetDetectionLevelFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetDetectionLevelConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::StopWaitingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESPackage* pPackage = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pPackage
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && pPackage) {
        if (pActor->GetPackageSetAsPcurrent() == pPackage || pActor->m_Extra.GetPackageExtra() == pPackage) {
            if (pPackage->GetPackType() == PACKAGE_TYPE_ESCORT) {
                pPackage->SetWaiting(false);
                pActor->pCurrentProcess->SetProcedureIndexRunning(3);
            }
            else if (
                pPackage->GetPackType() == PACKAGE_TYPE_ACCOMPANY || pPackage->GetPackType() == PACKAGE_TYPE_FOLLOW
            ) {
                int32_t iNumberOfFollowers;
                TESObjectREFR* pTarget = pActor->pCurrentProcess->GetTarget();
                if (dynamic_cast<Actor*>(pTarget) != pPlayer ||
                    pPlayer->IsPlayerFollower(pActor) ||
                    (iNumberOfFollowers = pPlayer->GetNumberOfFollowers(),
                     iNumberOfFollowers <= iNumberActorsAllowedToFollowPlayer.Int())) {
                    pPackage->SetWaiting(false);
                }
                else {
                    Interface::ShowMessage(sNoMoreFollowers.String(), 0, nullptr, nullptr, 2.0f, false);
                }
            }
            else {
                Warning("PACKAGES: Package %s is not a follow or an escort. ", pPackage->GetFormEditorID());
            }
        }
        else {
            const char* pName = pActor->GetFullName();
            Warning("PACKAGES: Package %s is not  %s current package", pPackage->GetFormEditorID(), pName);
        }
    }

    return true;
}

bool Script::SetPlayerSleepHours(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iHours = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iHours
        )) {
        return false;
    }
    pPlayer->SetSleeping(iHours, true);
    return true;
}

bool Script::ModAmountStolenSoldFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iAmount = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iAmount
        )) {
        return false;
    }
    pPlayer->ModAmountStolenSold(iAmount);
    return true;
}

bool Script::GetPCExpelledFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pFaction = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction
        )) {
        return false;
    }
    Script::GetPCExpelledConditionFunction(apRefObject, pFaction, nullptr, afReturnValue);
    return true;
}

bool Script::SetPCExpelledFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction = nullptr;
    int32_t iExpelled = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pFaction,
            &iExpelled
        )) {
        return false;
    }
    if (iExpelled) {
        pFaction->SetPCExpelled(true);
        TESObjectCELL* pCell = pPlayer->GetParentCell();
        if (pCell) {
            pCell->GetOwner();
        }
    }
    else {
        pFaction->SetPCExpelled(false);
    }
    return true;
}

bool Script::SetDestroyedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiDestroyed = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiDestroyed
        )) {
        return false;
    }
    if (apRefObject) {
        if (uiDestroyed) {
            apRefObject->SetDestroyed(true);
        }
        else {
            apRefObject->SetDestroyed(false);
        }
    }
    return true;
}

bool Script::SetSecuritronExpression(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cPersonality[516];
    char cMood[516];
    TESObjectREFR* pSecuritron = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pSecuritron,
            cPersonality,
            cMood
        )) {
        return false;
    }
    if (pSecuritron) {
        NiAVObject* pMesh = pSecuritron->Get3D();
        if (pMesh) {
            ExtraSecuritronFace* pFaceExtra = static_cast<ExtraSecuritronFace*>(pSecuritron->GetExtraData(143));
            if (!pFaceExtra) {
                pFaceExtra = new ExtraSecuritronFace;
                pSecuritron->m_Extra.AddExtra(pFaceExtra);
            }
            if (pFaceExtra) {
                pFaceExtra->SetPersonality(cPersonality);
                pFaceExtra->SetMood(cMood);
                pFaceExtra->ApplyFace(pMesh);
                pSecuritron->AddChange(CHANGE_REFR_ANIMATION);
                pSecuritron->AddChange(-0x5BF9E7C0);
            }
        }
    }
    return true;
}

bool Script::SwapTextureOnRef(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cGeometryName[512];
    char cTextureName[512];
    char path[260];
    TESObjectREFR* pRef = nullptr;
    BSStringT<char> kGeometryName;
    BSStringT<char> kTextureName;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pRef,
            cGeometryName,
            cTextureName
        )) {
        return false;
    }
    kGeometryName = cGeometryName;
    kTextureName = cTextureName;
    if (!kGeometryName.GetLength() || !kTextureName.GetLength()) {
        return false;
    }
    if (pRef) {
        NiAVObject* pMesh = pRef->Get3D();
        if (pMesh) {
            NiAVObject* pGeom = pMesh->GetObjectByName(cGeometryName);
            if (pGeom) {
                char cTexturePath[264];
                NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
                BSsprintf(cTexturePath, 260, "Textures\\%s.dds", kTextureName.String());
                SwapPlatformLanguageTexturePath(cTexturePath, path, 260u);
                TES::pInstance->CreateTextureImage(path, spTexture, false, false);
                int32_t iIsLighting = 0;
                NiProperty* pProperty = pGeom->GetProperty(3);
                if (NiObject::DynamicCastStatic(&NiShadeProperty::ms_RTTI, pProperty)) {
                    pProperty = pGeom->GetProperty(3);
                    NiShadeProperty* pShade = static_cast<NiShadeProperty*>(
                        NiObject::DynamicCastStatic(&NiShadeProperty::ms_RTTI, pProperty)
                    );
                    if (pShade->GetShaderPropertyType() >= 8) {
                        pProperty = pGeom->GetProperty(3);
                        pShade = static_cast<NiShadeProperty*>(
                            NiObject::DynamicCastStatic(&NiShadeProperty::ms_RTTI, pProperty)
                        );
                        if (pShade->GetShaderPropertyType() <= 12) {
                            iIsLighting = true;
                        }
                    }
                }
                BSShaderPPLightingProperty* pLightShader;
                if (iIsLighting) {
                    pProperty = pGeom->GetProperty(3);
                    pLightShader = static_cast<BSShaderPPLightingProperty*>(
                        NiObject::DynamicCastStatic(&NiShadeProperty::ms_RTTI, pProperty)
                    );
                }
                else {
                    pLightShader = nullptr;
                }
                if (pLightShader) {
                    pLightShader->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                }
                int32_t iType = NiShadeProperty::GetType();
                pProperty = pGeom->GetProperty(iType);
                BSShaderNoLightingProperty* pNoLightShader = static_cast<BSShaderNoLightingProperty*>(
                    NiObject::DynamicCastStatic(&BSShaderNoLightingProperty::ms_RTTI, pProperty)
                );
                if (pNoLightShader) {
                    pNoLightShader->SetBaseMap(spTexture.m_pObject);
                }
            }
        }
    }
    return true;
}

bool Script::SetUnconsciousFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiUnconscious = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiUnconscious
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor) {
            if (uiUnconscious) {
                pActor->StopCombat(nullptr);
                if (!pActor->IsUnConscious() && pActor->pCurrentProcess) {
                    pActor->pCurrentProcess->AddPostAnimationAction(BaseProcess::POSTANIM_FREEUP_SPECIAL_IDLE);
                    pActor->pCurrentProcess->SetCurrentProcessIdle(nullptr);
                }

                pActor->SetUnConscious(true);
                pActor->EndMovement();
                if (pActor->pCurrentProcess) {
                    pActor->pCurrentProcess->SetIdleTimer(-1.0f);
                }
            }
            else {
                if (pActor->IsUnConscious() && pActor->pCurrentProcess) {
                    pActor->pCurrentProcess->AddPostAnimationAction(BaseProcess::POSTANIM_FREEUP_SPECIAL_IDLE);
                    pActor->pCurrentProcess->SetCurrentProcessIdle(nullptr);
                }

                pActor->SetUnConscious(false);
            }
        }
    }
    return true;
}

bool Script::SetRestrainedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiRestrained = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiRestrained
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor) {
            if (uiRestrained) {
                pActor->SetRestrained(true);
                pActor->EndMovement();
            }
            else {
                pActor->SetRestrained(false);
            }
        }
    }
    return true;
}

bool Script::SetCanPCFastTravelFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t bCanFastTravel = 0;
    int32_t bCanWait = 1;
    int32_t bKeepOnCellChange = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &bCanFastTravel,
            &bCanWait,
            &bKeepOnCellChange
        )) {
        return false;
    }
    if (bCanFastTravel) {
        pPlayer->SetCanFastTravel(true);
        pPlayer->SetCanWait(true);
        // Tests bKeepOnCellChange against 1, not against 0.
        if (bKeepOnCellChange == 1) {
            pPlayer->SetDisableFastTravelResetsOnCellChange(1);
        }
        else {
            pPlayer->SetDisableFastTravelResetsOnCellChange(0);
        }
    }
    else {
        pPlayer->SetCanFastTravel(false);
        if (bCanWait) {
            pPlayer->SetCanWait(true);
        }
        else {
            pPlayer->SetCanWait(false);
        }
        if (bKeepOnCellChange == 1) {
            pPlayer->SetDisableFastTravelResetsOnCellChange(1);
        }
    }
    return true;
}

bool Script::ModPCMiscStat(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    MiscStatManager::MiscStatID eStat = MiscStatManager::MS_QUESTS_COMPLETED;
    int32_t iAmount = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eStat,
            &iAmount
        )) {
        return false;
    }
    MiscStatManager::ModVal(eStat, iAmount);
    return true;
}

bool Script::GetPCMiscStat(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pStat = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pStat
        )) {
        return false;
    }
    Script::GetPCMiscStatConditionFunction(apRefObject, pStat, nullptr, afReturnValue);
    return true;
}

bool Script::SetNoRumorsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iBool = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iBool
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        bool bNoRumors = iBool != 0;
        int32_t iNoRumors = iBool != 0;
        if (iNoRumors == pActor->GetActorBase()->GetNoRumors()) {
            pActor->m_Extra.ClearRumorsExtra();
        }
        else {
            pActor->m_Extra.SetNoRumors(bNoRumors);
        }
        pActor->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    }
    return true;
}

bool Script::TriggerHitShaderFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fStrength = 1.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fStrength
        )) {
        return false;
    }
    ImageSpaceModifierInstanceForm::Trigger(TESImageSpaceModifier::GetGetHit(), fStrength, nullptr);
    return true;
}

bool Script::AddAchievement(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiAchievement = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiAchievement
        )) {
        return false;
    }
    if (!g_bHasPlayerUsedConsole) {
        BSSteamAwardsSystemUtility::QInstance()->Unlock(uiAchievement);
    }
    return true;
}

bool Script::FlushPersistantActors(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iMode = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iMode
        )) {
        return false;
    }
    ReferenceProcessLists.FlushNonPersistActors(iMode);
    if (bSystemOutput) {
        Interface::PrintLine("Deleted all non persistent actors in high process.");
    }
    return true;
}

bool Script::ToggleFogOfWar(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cFogOfWar = Interface::GetFogOfWarEnabled();
    Interface::SetFogOfWarEnabled(cFogOfWar == 0);
    if (bSystemOutput) {
        const char* pState = Interface::GetFogOfWarEnabled() ? "ENABLED" : "DISABLED";
        Interface::PrintLine("Fog of war - %s.", pState);
    }
    return true;
}

bool Script::OutputLocalMapPictures(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ImageSpaceEffectMap::bSaveMode = true;
    MapMenu::ClearCount128();
    for (uint32_t i = 0; i < 25; ++i) {
        MapMenu::RenderNextGridToDisk();
    }
    ImageSpaceEffectMap::bSaveMode = false;
    return true;
}

bool Script::WasteMemory(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fAmount = 0.0f;
    return Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &fAmount
    );
}

bool Script::SendCrimeAlarmFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pActor
        )) {
        return false;
    }
    Actor* pSender = dynamic_cast<Actor*>(apRefObject);
    if (pActor && pSender) {
        pActor->TrespassAlarm(pSender, pSender->GetObjectReference(), -1);
    }

    return true;
}

bool Script::ForceCombatGroupStrategyFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cStrategy[516];
    ourStrcpy(cStrategy, "");
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cStrategy
        )) {
        return false;
    }
    if (apRefObject->IsActor()) {
        CombatController* pController = static_cast<Actor*>(apRefObject)->GetCombatController();
        if (pController) {
            CombatGroup* pGroup = pController->GetCombatGroup();
            if (pGroup->GetMemberCount() >= 2) {
                if (std::strlen(cStrategy)) {
                    CombatGroupStrategy* pStrategy = CombatGroupStrategy::LookupByName(cStrategy);
                    if (pStrategy) {
                        pGroup->ForceGroupStrategy(pStrategy);
                        if (bSystemOutput) {
                            Interface::PrintLine("Group strategy set to: %s", pStrategy->GetName());
                        }
                    }
                    else if (bSystemOutput) {
                        Interface::PrintLine("Unknown strategy '%s'", cStrategy);
                    }
                }
                else {
                    pGroup->ForceGroupStrategy(nullptr);
                    if (bSystemOutput) {
                        Interface::PrintLine("Group strategy cleared");
                    }
                }
                return true;
            }

            Interface::PrintLine("The combat group has only one member");
            return true;
        }

        const char* pName = apRefObject->GetFormEditorIDLength() ? apRefObject->GetFormEditorID()
                                                                 : apRefObject->GetFullName();
        Interface::PrintLine("%s is not in combat", pName);
        return true;
    }

    const char* pName = apRefObject->GetFormEditorIDLength() ? apRefObject->GetFormEditorID()
                                                             : apRefObject->GetFullName();
    Interface::PrintLine("%s is not an actor", pName);
    return true;
}

bool Script::PushActorAwayFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = nullptr;
    int32_t iDamage = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pActor,
            &iDamage
        )) {
        return false;
    }
    if (pActor->IsActor()) {
        float fSpeed = CombatFormulas::GetKnockbackSpeed(pActor, iDamage);
        BaseProcess* pProcess = pActor->GetCurrentAIProcess();
        if (pProcess && pProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
            if (apRefObject == pPlayer) {
                ItemChange* pWeaponItem = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
                TESBoundObject* pWeapon = pWeaponItem ? pWeaponItem->pContainerObj : nullptr;
                TESChallengeManager::UpdateChallenge(
                    CHALLENGE_TYPE_HIT,
                    1,
                    pActor->GetObjectReference(),
                    pWeapon,
                    0xFFFF,
                    2,
                    0
                );
            }
            pProcess->KnockExplosion(pActor, apRefObject->GetLocationOnReference(), fSpeed);
        }
    }
    else if (apScript) {
        Warning(
            "SCRIPTS: PushActorAway in script '%s' is attempting to push a non-actor reference.",
            apScript->GetFormEditorID()
        );
    }
    return true;
}

bool Script::GetObjectiveCompletedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iVal;
    int32_t iIndex;
    TESQuest* pQuest;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest,
            &iIndex,
            &iVal
        )) {
        return false;
    }
    if (pQuest) {
        BGSQuestObjective* pObjective = pQuest->GetObjective(static_cast<uint32_t>(iIndex));
        if (pObjective) {
            afReturnValue = pObjective->GetCompleted() ? 1.0 : 0.0;
            if (bSystemOutput) {
                Interface::PrintLine("GetObjectiveCompleted >> %0.2f", afReturnValue);
            }
        }
    }
    return true;
}

bool Script::GetObjectiveDisplayedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iVal;
    int32_t iIndex;
    TESQuest* pQuest;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest,
            &iIndex,
            &iVal
        )) {
        return false;
    }
    if (pQuest) {
        BGSQuestObjective* pObjective = pQuest->GetObjective(static_cast<uint32_t>(iIndex));
        if (pObjective) {
            afReturnValue = pObjective->GetDisplayed() ? 1.0 : 0.0;
            if (bSystemOutput) {
                Interface::PrintLine("GetObjectiveDisplayed >> %0.2f", afReturnValue);
            }
        }
    }
    return true;
}

bool Script::PipboyRadio(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cCommand[524];
    TESObjectREFR* pTarg = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            cCommand,
            &pTarg
        )) {
        return false;
    }
    cCommand[523] = cCommand[0] == 49;
    bool bZero = cCommand[0] == 48;
    if (cCommand[0] != 49 && BSstricmp(cCommand, "enable") && BSstricmp(cCommand, "on")) {
        if (!bZero && BSstricmp(cCommand, "disable") && BSstricmp(cCommand, "off")) {
            if (!BSstricmp(cCommand, "tune")) {
                FalloutRadio::TunePipboyToStation(pTarg, true);
            }
        }
        else {
            FalloutRadio::PipboyRadioEnable(false);
        }
    }
    else {
        FalloutRadio::PipboyRadioEnable(true);
        FalloutRadio::TunePipboyToStation(pTarg, true);
    }
    return true;
}

bool Script::SetNPCRadio(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pStation;
    uint32_t uiEnable[2];
    pStation = nullptr;
    uiEnable[1] = 0;
    uiEnable[0] = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            uiEnable,
            &pStation
        )) {
        return false;
    }
    if (!apRefObject || !apRefObject->IsActor() || !pStation) {
        return false;
    }
    if (uiEnable[0] == 1) {
        FalloutRadio::EnableNPCRadio(apRefObject, pStation);
    }
    else if (!uiEnable[0]) {
        FalloutRadio::DisableNPCRadio(apRefObject);
    }
    return true;
}

bool Script::StartRadioConversation(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESTopic* pTopic = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTopic
        )) {
        return false;
    }
    FalloutRadio::StartRadioConversation(apRefObject, pTopic);
    return true;
}

bool Script::PlaceLocationMarkerFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char cName[516];
    float z;
    int32_t iColor;
    float x;
    float y;
    // Zeroes the first byte and memsets the next 511, which is the `= {0}` of a 512-byte buffer.
    cName[0] = 0;
    std::memset(&cName[1], 0, 511);
    iColor = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &x,
            &y,
            &z,
            cName,
            &iColor
        )) {
        return false;
    }
    if (pPlayer->GetParentCell()) {
        NiPoint3 location(x, y, z);
        if (std::strlen(cName)) {
            const NiColorA* pColor = &BSTestObjects::GetColor(iColor);
            pPlayer->GetParentCell()->AddLocationMarker(location, cName, pColor);
        }
        else {
            const NiColorA* pColor = &BSTestObjects::GetColor(iColor);
            pPlayer->GetParentCell()->AddLocationMarker(location, nullptr, pColor);
        }
    }
    return true;
}

bool Script::GetIsObjectTypeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint8_t ucFormType[2];
    ucFormType[1] = 1;
    ucFormType[0] = 0;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               ucFormType
           ) &&
        Script::GetIsObjectTypeConditionFunction(
               apRefObject,
               reinterpret_cast<void*>(static_cast<uintptr_t>(ucFormType[0])),
               nullptr,
               afReturnValue
        );
}

bool Script::MarkForDeleteFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject && !apRefObject->GetDelete() && apRefObject != pPlayer) {
        apRefObject->SetDelete(true);
        if (!apRefObject->GetFile(0)) {
            Script::ReferencesToDelete.AddHead(apRefObject);
        }
    }

    return true;
}

bool Script::SetPlayerTagSkillFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ActorValue::Index eSkill = ActorValue::VALUE_NONE;
    int32_t iIndex = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eSkill,
            &iIndex
        )) {
        return false;
    }
    if (ActorValue::IsSkill(eSkill) && iIndex >= 0 && iIndex < TESClass::GetTagSkillCount()) {
        pPlayer->GetClass()->SetMajorSkill(iIndex, eSkill);
    }
    return true;
}

bool Script::IsPlayerTagSkillFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ActorValue::Index eSkill = ActorValue::VALUE_NONE;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &eSkill
        )) {
        return false;
    }
    afReturnValue = 0.0;
    if (ActorValue::IsSkill(eSkill)) {
        if (pPlayer->GetClass()->IsTagSkill(eSkill)) {
            afReturnValue = 1.0;
        }
    }
    if (bSystemOutput) {
        Interface::PrintLine("IsPlayerTagSkill >> %.0f", afReturnValue);
    }
    return true;
}

bool Script::IsPlayerGrabbedRefFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pRef = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pRef
           ) &&
        Script::IsPlayerGrabbedRefConditionFunction(apRefObject, pRef, nullptr, afReturnValue);
}

bool Script::PlaceLeveledActorAtMeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    TESActorBase* pActorBase = nullptr;
    int32_t iLevelMod = 4;
    BGSEncounterZone* pForceZone = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pActorBase,
            &iLevelMod,
            &pForceZone
        )) {
        return false;
    }
    if (pActorBase && pActorBase->IsActorBase() && apRefObject) {
        TESActorBase* pBase = pActorBase;
        Actor* pPlacedRef = nullptr;
        int32_t iFormType = pActorBase->GetFormType();
        if (iFormType == NPC__ID) {
            pPlacedRef = new Character;
        }
        else if (iFormType == CREA_ID) {
            pPlacedRef = new Creature;
        }
        if (pPlacedRef) {
            if (pForceZone) {
                pPlacedRef->SetEncounterZone(pForceZone);
            }
            pPlacedRef->m_Extra.SetLevCreaModifier(iLevelMod);
            pPlacedRef->AddChange(CHANGE_REFR_EXTRA_CREATED_ONLY);
            pPlacedRef->SetParentCell(apRefObject->GetParentCell());
            pPlacedRef->SetObjectReference(pBase);
            if (pBase->GetUsesLeveledTemplate()) {
                pBase->CalcTemplateForRef(pPlacedRef);
            }
            TESObjectCELL* pInterior = apRefObject->GetParentCell();
            if (pInterior && !pInterior->GetInterior()) {
                pInterior = nullptr;
            }

            TESWorldSpace* pWorld = apRefObject->GetWorldSpace();
            const NiPoint3* pAngle = &apRefObject->GetAngleOnReference();
            const NiPoint3* pLocation = &apRefObject->GetLocationOnReference();
            pDataHandler->CreateReferenceAtLocation(
                pPlacedRef->GetObjectReference(),
                *pLocation,
                *pAngle,
                pInterior,
                pWorld,
                pPlacedRef,
                nullptr,
                nullptr
            );
            pPlacedRef->SetStillLoading(false);
            uint32_t uiID = pPlacedRef->iFormID;
            Script::PutNumericIDInDouble(uiID, afReturnValue);
        }
    }

    return true;
}

bool Script::ForceActiveQuestFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* pQuest = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pQuest
        )) {
        return false;
    }
    if (pQuest) {
        pPlayer->SetActiveQuest(pQuest);
    }
    return true;
}

bool Script::MatchFaceGeometryFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pMatchActor = nullptr;
    int32_t iVal = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pMatchActor,
            &iVal
        )) {
        return false;
    }
    if (iNumHWThreads.Int() <= 1) {
        Script::MatchFaceGeometryBase(apRefObject, pMatchActor, iVal);
    }
    else {
        TaskQueueInterface::GetInstance()->QueueScriptFunctionCall(0x11DFu, apRefObject, pMatchActor, iVal);
    }
    return true;
}

bool Script::GetIsUsedItemEquipTypeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iEquipType = -1;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &iEquipType
           ) &&
        Script::GetIsUsedItemEquipTypeConditionFunction(
               apRefObject,
               reinterpret_cast<void*>(static_cast<intptr_t>(iEquipType)),
               nullptr,
               afReturnValue
        );
}

bool Script::AddItemHealthPercentFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* pItem = nullptr;
    int32_t iCount = 0;
    float fHealthPercent = 0.0f;
    int32_t iSilent = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pItem,
            &iCount,
            &fHealthPercent,
            &iSilent
        )) {
        return false;
    }
    if (apRefObject) {
        uint32_t uiLevel = apRefObject->GetCalcLevel(false);
        TESContainer kContainer;
        TESLevItem* pLevItem = nullptr;
        BGSListForm* pListForm = nullptr;
        TESBoundObject* pBoundObject = nullptr;
        int32_t iFormType = pItem->GetFormType();
        if (iFormType == LVLI_ID) {
            pLevItem = static_cast<TESLevItem*>(pItem);
        }
        else if (iFormType == FLST_ID) {
            pListForm = static_cast<BGSListForm*>(pItem);
        }
        else if (pItem->IsBoundObject()) {
            pBoundObject = static_cast<TESBoundObject*>(pItem);
        }
        if (pLevItem) {
            pLevItem->CalculateCurrentFormList(uiLevel, iCount, &kContainer, 0);
        }
        else if (pListForm && iCount) {
            BSSimpleList<TESForm*>* pIter = pListForm->GetFormList();
            while (pIter && !pIter->IsEmpty()) {
                TESForm* pForm = pIter->GetItem();
                pIter = pIter->GetNext();
                if (pForm->IsBoundObject()) {
                    kContainer.AddObject(static_cast<TESBoundObject*>(pForm), iCount, nullptr);
                }
            }
        }
        else if (pBoundObject && iCount) {
            kContainer.AddObject(pBoundObject, iCount, nullptr);
        }
        else {
            Warning("SCRIPTS: AddItem in script '%s' failed to generate an item.", apScript->GetFormEditorID());
        }
        kContainer.ApplyHealthPercentToContainer(fHealthPercent);
        kContainer.AddContainerToReference(apRefObject, iSilent == 1);
    }
    return true;
}

bool Script::SendAssaultAlarmFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    PlayerCharacter* pTarget = nullptr;
    TESFaction* pFaction = nullptr;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &pTarget,
        &pFaction
    );
    if (apRefObject->IsActor()) {
        if (pTarget || pFaction) {
            if (pTarget && pTarget != pPlayer && pTarget->pCurrentProcess) {
                pTarget->AttackAlarm(pPlayer, false, true);
                pTarget->pCurrentProcess->EnterCombat(
                    pTarget,
                    pPlayer,
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
            }
            else if (pFaction) {
                Actor* pFactionActor = ReferenceProcessLists.GetActorRefInHigh(pFaction, true, true);
                if (pFactionActor) {
                    if (pFactionActor->IsCreature()) {
                        if (pFactionActor->GetOwner()) {
                            Actor* pOwnerActor;
                            TESForm* pOwner = pFactionActor->GetOwner();
                            if (pOwner->GetFormType() == FACT_ID) {
                                pOwnerActor = ReferenceProcessLists
                                                  .GetActorRefInHigh(static_cast<TESFaction*>(pOwner), false, true);
                            }
                            else {
                                pOwnerActor = ReferenceProcessLists.GetActorRefInHigh(
                                    static_cast<TESNPC*>(pOwner),
                                    false
                                );
                            }
                            if (pOwnerActor) {
                                pOwnerActor->AttackAlarm(pPlayer, false, true);
                                pFactionActor->pCurrentProcess->EnterCombat(
                                    pFactionActor,
                                    pPlayer,
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
                            }
                        }
                    }
                    else {
                        pFactionActor->AttackAlarm(pPlayer, false, true);
                        pFactionActor->pCurrentProcess->EnterCombat(
                            pFactionActor,
                            pPlayer,
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
                    }
                }
            }
        }
        else {
            Actor* pActor = static_cast<Actor*>(apRefObject);
            pActor->Actor::AttackAlarm(pPlayer, false, true);
            pActor->GetCurrentAIProcess()->EnterCombat(
                pActor,
                pPlayer,
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
        }
    }
    return true;
}

bool Script::GetThreatRatioFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetThreatRatioConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::MatchRaceFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pActor
        )) {
        return false;
    }
    if (pActor && pActor->IsNpc() && apRefObject && apRefObject->IsNpc()) {
        TESNPC* pNPC = static_cast<TESNPC*>(apRefObject->GetObjectReference());
        TESNPC* pNewNPC = static_cast<TESNPC*>(pActor->GetObjectReference());
        TESRace* pRace = pNPC->pFormRace;
        TESRace* pNewRace = pNewNPC->pFormRace;
        if (pNewRace != pRace) {
            uint32_t uiLevel = 0;
            while (pRace->GetYoungerRace()) {
                pRace = pRace->GetYoungerRace();
                ++uiLevel;
            }
            while (pNewRace->GetYoungerRace()) {
                pNewRace = pNewRace->GetYoungerRace();
            }
            while (pNewRace->GetOlderRace() && uiLevel) {
                pNewRace = pNewRace->GetOlderRace();
                --uiLevel;
            }
            BSFaceGenManager::ClearBodyTexturesFromPalette(pNPC);
            pNPC->SwitchRace(pNewRace, nullptr);
            static_cast<Character*>(apRefObject)->Character::Reset3D();
        }
    }

    return true;
}

bool Script::SetPCYoungFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiYoung = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiYoung
        )) {
        return false;
    }
    pPlayer->SetPCYoung(uiYoung != 0);
    return true;
}

bool Script::SetPCToddlerFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiToddler = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiToddler
        )) {
        return false;
    }
    pPlayer->SetPCToddler(uiToddler != 0);
    return true;
}

bool Script::GetConcussedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pParam1 = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pParam1
           ) &&
        Script::GetConcussedConditionFunction(apRefObject, pParam1, nullptr, afReturnValue);
}

bool Script::SetVATSTargetFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iVal = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iVal
        )) {
        return false;
    }
    if (apRefObject->GetDestructible()) {
        bool bWantToTarget = iVal != 0;
        bool bTargetable = false;
        BGSDestructibleObjectForm* pDestruct = BGSDestructibleObjectForm::GetDestructionForm(
            apRefObject->GetObjectReference()
        );
        if (pDestruct) {
            bTargetable = pDestruct->GetTargetableInVATS();
        }
        if (bTargetable == bWantToTarget) {
            apRefObject->SetVATSTargetOverride(false);
        }
        else {
            apRefObject->SetVATSTargetOverride(true);
        }
    }
    return true;
}

bool Script::SetZoneRespawnsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSEncounterZone* pZone = nullptr;
    int32_t iRespawns = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pZone,
            &iRespawns
        )) {
        return false;
    }
    if (pZone) {
        pZone->SetNeverReset(iRespawns == 0);
        pZone->AddChange(CHANGE_ENCOUNTER_ZONE_FLAGS);
    }
    return true;
}

bool Script::CreateDetectionEventFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        TESObjectREFR* pRef = nullptr;
        int32_t iSound = 0;
        int32_t iEvent = 3;
        if (!Script::ParseParameters(
                apParameters,
                apCompiledParams,
                auiOffset,
                apRefObject,
                apContainer,
                apScript,
                apScriptLocals,
                &pRef,
                &iSound,
                &iEvent
            )) {
            return false;
        }
        if (pRef && pRef->IsActor()) {
            Actor* pActor = static_cast<Actor*>(pRef);
            if (pActor->GetCurrentAIProcess()) {
                BaseProcess* pProc = pActor->GetCurrentAIProcess();
                pProc->SetActorsDetectionEvent(
                    pActor,
                    apRefObject->GetLocationOnReference(),
                    iSound,
                    iEvent,
                    apRefObject
                );
            }
        }
    }
    return true;
}

bool Script::IgnoreCrimeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iIgnoreCrime = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iIgnoreCrime
        )) {
        return false;
    }
    if (apRefObject && apRefObject->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        if (iIgnoreCrime <= 0) {
            pActor->SetIgnoreCrime(false);
        }
        else {
            pActor->SetIgnoreCrime(true);
        }
    }

    return true;
}

bool Script::IsCombatTargetFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::IsCombatTargetConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

// Places an ash pile (the second one for a parsed 2, else the first) at the actor: a pick straight down from 32 units
// above it to 256 below puts the pile on the hit point, turned to the hit's normal, or on stairs to the navmesh's
// normal there; the pile and the actor then point at each other through their ash pile extra data. On stairs the normal
// comes from Pathing::GetNavmeshNormalForLocation, which returns early without writing it when the location has no
// navmesh triangle, and it is still passed by value to BSUtilities::GetRotationZToVectorTranspose, an uninitialised
// read kept deliberately.
bool Script::AttachAshPileFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    CFilter fltr;
    NiPoint3 kLocation;
    int32_t iParam = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iParam
        )) {
        return false;
    }
    if (apRefObject && apRefObject->IsActor()) {
        NiPoint3 kTo;
        alignas(16) NiPoint3 kFrom;
        kLocation = apRefObject->GetLocationOnReference();
        NiPoint3 kAngle(0.0f, 0.0f, apRefObject->GetAngleOnReference().z);
        char cHit = 0;
        // Static, water, terrain, ground and invisible wall.
        uint32_t uiMultiLayer = 0x8022802;
        bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1, uiMultiLayer);
        bhkPickData kPick;
        fltr.iFilter = 0;
        fltr.SetLayer(L_CUSTOMPICK1);
        uint32_t uiSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
        fltr.SetGroup(uiSystemGroup);
        kPick.SetFilter(fltr);
        kFrom.x = kLocation.x;
        kFrom.y = kLocation.y;
        kFrom.z = kLocation.z + 32.0;
        kTo.x = kLocation.x;
        kTo.y = kLocation.y;
        kTo.z = kLocation.z - 256.0;
        kPick.NiSetFrom(kFrom);
        kPick.NiSetTo(kTo);
        TES::pInstance->Pick(kPick);
        hkBool bHasHit = kPick.hasHit();
        if (bHasHit) {
            NiPoint3 kNormal;
            const hkpShape* pShape = kPick.m_rootCollidable->m_shape;
            bhkShape* pBhkShape = bhkShape::Getbhk(pShape);
            bool bStairs = pBhkShape->GetMaterialVarType(MVT_STAIRS);
            if (!bStairs) {
                const hkpShape* pChildShape = pShape;
                const hkpShape* pParentShape = nullptr;
                uint32_t uiShapeKey = -1;
                hkLocalBuffer<char[512]> kShapeBuffer(8, nullptr);
                for (int32_t i = 0; i < 8; ++i) {
                    if (kPick.m_shapeKeys[i] == -1) {
                        break;
                    }
                    if (pChildShape->getType() == HK_SHAPE_MOPP && (++i >= 8 || kPick.m_shapeKeys[i] == -1)) {
                        break;
                    }
                    uiShapeKey = kPick.m_shapeKeys[i];
                    pParentShape = pChildShape;
                    const hkpShapeContainer* pContainer = pChildShape->getContainer();
                    pChildShape = pContainer->getChildShape(kPick.m_shapeKeys[i], kShapeBuffer[i]);
                }
                bhkShape* pParentBhkShape = bhkShape::Getbhk(pParentShape);
                if (pParentBhkShape) {
                    bStairs = pParentBhkShape->GetSubMaterialVarType(uiShapeKey, MVT_STAIRS);
                }
            }
            if (bStairs) {
                PathingLocation kPathLocation(apRefObject);
                Pathing::GetNavmeshNormalForLocation(kPathLocation, kNormal);
            }
            else {
                HK2NI(kNormal, kPick.m_normal);
            }
            alignas(16) NiMatrix3 kRotation = BSUtilities::GetRotationZToVectorTranspose(kNormal);
            kRotation.ToEulerAnglesXYZ(kAngle.x, kAngle.y, kAngle.z);
            cHit = 1;
            kPick.NiCalcHitPoint(kLocation);
        }
        TESObjectACTI* pPileForm = iParam == 2 ? pAshPile2 : pAshPile1;
        TESWorldSpace* pWorld = apRefObject->GetWorldSpace();
        TESObjectCELL* pCell = apRefObject->GetParentCell();
        TESObjectREFR* pPile = pDataHandler->CreateReferenceAtLocation(
            pPileForm,
            kLocation,
            kAngle,
            pCell,
            pWorld,
            nullptr,
            nullptr,
            nullptr
        );
        pPile->SetDroppedRef(cHit == 0);
        pPile->SetDisableFade(true);
        pPile->SetRefPersists(true);
        pPile->m_Extra.SetAshPileRef(apRefObject);
        apRefObject->m_Extra.SetAshPileRef(pPile);
        pPile->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        apRefObject->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    }

    return true;
}

bool Script::SetMinimalUseFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiMinimalUse = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiMinimalUse
        )) {
        return false;
    }
    if (apRefObject &&
        apRefObject->GetObjectReference() &&
        apRefObject->GetObjectReference()->GetFormType() == DOOR_ID) {
        static_cast<TESObjectDOOR*>(apRefObject->GetObjectReference())->TESObjectDOOR::SetMinimalUse(uiMinimalUse != 0);
        pPlayer->CheckForQuestTargetUpdate(pPlayer);
    }
    else {
        Warning("SCRIPTS: SetMinimalUse must be called on a door reference.", apScript->GetFormEditorID());
    }
    return true;
}

bool Script::SetPCCanUsePowerArmorFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiCanUse = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &uiCanUse
        )) {
        return false;
    }
    pPlayer->SetPCCanUsePowerArmor(uiCanUse != 0);
    return true;
}

bool Script::GetReputationThresholdFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t uiThreshold;
    TESReputation* pReputation = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pReputation,
            &uiThreshold
        )) {
        return false;
    }
    if (uiThreshold > 2) {
        return false;
    }
    afReturnValue = static_cast<double>(pReputation->GetReputationThreshold(uiThreshold));
    return true;
}

bool Script::SetLastExtDoorActivated(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pDoor = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pDoor
        )) {
        return false;
    }
    pPlayer->pLastExtDoorActivated = pDoor;
    if (pDoor && pDoor->GetWorldSpace()) {
        Interface::SetRootMapSpace(pDoor->GetWorldSpace());
    }

    return true;
}

bool Script::SetTaskThreadSleepFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iSleepValue = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iSleepValue
        )) {
        return false;
    }
    BSTaskThread::SetPauseSleepValue(iSleepValue);
    Interface::PrintLine("BSTaskThread::sPauseSleepValue set to %d ms", iSleepValue);
    return true;
}

bool Script::IsImageSpaceActiveFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESImageSpaceModifier* pImageSpace = nullptr;
    float fStrength = 1.0f;
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pImageSpace,
            &fStrength
        )) {
        return false;
    }
    // The list, its items and its links are read through trivial accessors.
    BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* pNode = &TES::pInstance->listActiveImageSpaceModifiers;
    while (pNode) {
        NiPointer<ImageSpaceModifierInstance> spInstance(pNode->m_item);
        if (spInstance.m_pObject) {
            ImageSpaceModifierInstanceForm* pForm = spInstance->IsForm();
            if (pForm && pForm->pImageSpaceMod == pImageSpace) {
                afReturnValue = 1.0;
                break;
            }
        }
        pNode = pNode->m_pkNext;
    }
    if (0.0 == afReturnValue) {
        Interface::PrintLine("ImageSpace is not active ");
    }
    else {
        Interface::PrintLine("ImageSpace is active ");
    }
    return true;
}

bool Script::ResetXPFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals
    );
    if (pPlayer) {
        float fXP = pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS);
        pPlayer->PermanentModActorValue(ActorValue::EXPERIENCE_POINTS, -fXP, static_cast<Actor*>(nullptr));
        return true;
    }

    Warning("Attempting to reset xp on an invalid player!");
    return false;
}

bool Script::PlaceAtReticleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    TESForm* pObject = nullptr;
    int32_t iCount = 1;
    alignas(16) float fUnused = 0.0f;
    float fDistance = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pObject,
            &iCount,
            &fUnused,
            &fDistance
        )) {
        return false;
    }
    fDistance = BSMIN(fDistance, 50000.0f);
    bhkPickData kPick;
    NiPoint3 kTo(0.0f, fDistance, 0.0f);
    NiPoint3 kFrom = pPlayer->GetLocationOnReference();
    NiAVObject* pCamera = Main::GetWorldSceneGraph()->GetCamera();
    alignas(16) NiMatrix3 kCameraRotate = pCamera->GetWorldTransform().m_Rotate;
    // The camera's rows with their components rotated (y, z, x into x, y, z): NiMatrix3's and NiPoint3's empty
    // constructors are trivial.
    NiMatrix3 kRotate;
    alignas(16) NiPoint3 kRow;
    kCameraRotate.GetRow(0, kRow.y, kRow.z, kRow.x);
    kRotate.SetRow(0, kRow);
    kCameraRotate.GetRow(1, kRow.y, kRow.z, kRow.x);
    kRotate.SetRow(1, kRow);
    kCameraRotate.GetRow(2, kRow.y, kRow.z, kRow.x);
    kRotate.SetRow(2, kRow);
    kFrom = *pPlayer->GetCameraPosition();
    kTo = kRotate * kTo + kFrom;
    CFilter kUnusedFilter;
    pPlayer->GetCollisionFilter(&kUnusedFilter.iFilter);
    alignas(16) CFilter kFilter;
    kFilter.iFilter = 0;
    kFilter.SetLayer(L_PROJECTILE);
    CFilter kPlayerFilter;
    pPlayer->GetCollisionFilter(&kPlayerFilter.iFilter);
    kFilter.SetGroup(kPlayerFilter.GetGroup());
    kPick.SetFilter(kFilter);
    kPick.NiSetFrom(kFrom);
    kPick.NiSetTo(kTo);
    TES::pInstance->Pick(kPick);
    NiPoint3 kHitPoint;
    kPick.NiCalcHitPoint(kHitPoint);
    alignas(16) NiPoint3 kLocation = pPlayer->GetLocationOnReference();
    if (pObject->IsBoundObject()) {
        TESBoundObject* pBoundObject = static_cast<TESBoundObject*>(pObject);
        TESHealthForm* pHealthForm = TESHealthForm::GetFormAsHealthForm(pObject);
        if (TES::pInstance->IsCoordInLoadedArea(kHitPoint)) {
            kLocation = kHitPoint;
            afReturnValue = 1.0;
            // The arguments are evaluated last to first.
            TESWorldSpace* pWorldSpace = pPlayer->GetWorldSpace();
            TESObjectCELL* pParentCell = pPlayer->GetParentCell();
            const NiPoint3* pAngle = &pPlayer->GetAngleOnReference();
            TESObjectREFR* pRef = pDataHandler->CreateReferenceAtLocation(
                pBoundObject,
                kLocation,
                *pAngle,
                pParentCell,
                pWorldSpace,
                nullptr,
                nullptr,
                nullptr
            );
            if (!pRef) {
                return false;
            }
            if (pRef->IsActor()) {
                Actor* pActor = static_cast<Actor*>(pRef);
                pActor->SetAlpha(0.0f);
                pActor->FadeIn();
            }
            if (pHealthForm) {
                float fHealth = static_cast<float>(pHealthForm->GetFormHealth());
                fHealth = fHealth * 100.0;
                pRef->SetHealth(fHealth);
            }
        }
    }
    return true;
}

bool Script::ShowSleepWaitMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iSleep;
    int32_t iPassTimeOnly = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iSleep,
            &iPassTimeOnly
        )) {
        return false;
    }
    // Both parsed values are tested against 1.
    if (iPassTimeOnly == 1) {
        pPlayer->CanPassTime();
    }
    else {
        Interface::CreateSleepMenu(iSleep == 1);
    }
    return true;
}

bool Script::SaveIniFiles(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char (*pIniFile)[260] = &SettingT<INISettingCollection>::GetCollection()->pSettingFile;
    INISettingCollection* pIniCollection = SettingT<INISettingCollection>::GetCollection();
    pIniCollection->SaveAll(reinterpret_cast<char*>(pIniFile));
    char (*pPrefFile)[260] = &SettingT<INIPrefSettingCollection>::GetCollection()->pSettingFile;
    INIPrefSettingCollection* pPrefCollection = SettingT<INIPrefSettingCollection>::GetCollection();
    pPrefCollection->SaveAll(reinterpret_cast<char*>(pPrefFile));
    return true;
}

bool Script::ReloadCurrentWeather(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESWeather* pWeather = Sky::GetInstance()->GetCurrentWeather();
    TESFile* pFile = pWeather->GetFile(-1);
    if (pFile) {
        TESFile* pThreadSafeFile = pFile->GetThreadSafeFile();
        pThreadSafeFile->OpenTES(NiFile::READ_ONLY, false);
        pThreadSafeFile->FindForm(pWeather);
        pWeather->ClearData();
        pWeather->Load(pThreadSafeFile);
    }
    return true;
}

bool Script::TestSeenData(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    NiNode* pNode;
    NiMatrix3 kRot;
    pNode = new NiNode(0);
    TESObjectCELL* pCell = pPlayer->GetParentCell();
    if (pCell->GetInterior()) {
        SeenData* pData = pCell->GetSeenData();
        if (pData) {
            NiPoint3 kCoord(static_cast<float>(0.0), static_cast<float>(0.0), pPlayer->GetLocationOnReference().z);
            pData->DrawSeenData(pNode, kCoord, true);
            float fRot = pCell->GetNorthRotation() * -1.0;
            if (fRot != 0.0) {
                kRot = NiMatrix3::IDENTITY;
                kRot.MakeZRotation(fRot);
                pNode->SetRotate(kRot);
            }
        }
    }
    else {
        // The grid count is compared unsigned.
        for (int32_t iX = 0; static_cast<uint32_t>(iX) < uGridsToLoad.UInt(); ++iX) {
            for (int32_t iY = 0; static_cast<uint32_t>(iY) < uGridsToLoad.UInt(); ++iY) {
                TESObjectCELL* pGridCell = TES::pInstance->GetGridCell(iX, iY)->pCell;
                SeenData* pData = pGridCell ? pGridCell->GetSeenData() : nullptr;
                if (pData) {
                    float fZ = pPlayer->GetLocationOnReference().z;
                    float fY = static_cast<float>(pGridCell->GetDataY() << 12);
                    float fX = static_cast<float>(pGridCell->GetDataX() << 12);
                    NiPoint3 kCoord(fX, fY, fZ);
                    pData->DrawSeenData(pNode, kCoord, true);
                }
            }
        }
    }
    NiUpdateData kUpdateData(0.0f, false, false);
    pNode->Update(kUpdateData);
    pNode->UpdateProperties();
    TES::pInstance->AddTempDebugObject(pNode, 30.0f);
    return true;
}

bool Script::ToggleFullHelp(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Interface::ToggleFullHelp();
    const char* pState = Interface::GetFullHelp() ? "shown." : "hidden.";
    Interface::PrintLine("Toggle Full Help %s", pState);
    return true;
}

bool Script::ToggleSkyFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    s_ToggleSky ^= 1u;
    if (TES::pInstance) {
        if (s_ToggleSky) {
            TES::pInstance->pSky->SetMode(Sky::SM_NONE);
        }
        else if (TES::pInstance->pInteriorCell) {
            if (TES::pInstance->pInteriorCell->GetFakeExterior()) {
                TES::pInstance->pSky->SetMode(Sky::SM_FAKEEXTERIOR);
            }
            else {
                TES::pInstance->pSky->SetMode(Sky::SM_INTERIOR);
            }
        }
        else {
            TES::pInstance->pSky->SetMode(Sky::SM_FULL);
        }
    }
    if (bSystemOutput) {
        const char* pState = s_ToggleSky ? "Off" : "On";
        Interface::PrintLine("Sky -> %s", pState);
    }
    return true;
}

bool Script::ToggleCollisionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        bool bNoCollision = !apRefObject->GetNoCollision();
        apRefObject->SetNoCollision(bNoCollision);
        if (bSystemOutput) {
            const char* pState = bNoCollision ? "Off" : "On";
            Interface::PrintLine("Ref '%s' Collision -> %s", apRefObject->GetFullName(), pState);
        }
    }
    else {
        TES::pInstance->ToggleCollision();
        if (bSystemOutput) {
            const char* pState = TES::QCollisionDisableFlag() ? "Off" : "On";
            Interface::PrintLine("Collision -> %s", pState);
        }
    }
    return true;
}

bool Script::ToggleWireframeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    g_bIsToggleWireframe ^= 1u;
    Main::GetWorldSceneGraph();
    ShadowSceneNode* pScene = BSShaderManager::GetShadowSceneNode(0);
    pScene->ToggleWireframe();
    if (bSystemOutput) {
        const char* pState = g_bIsToggleWireframe ? "On" : "Off";
        Interface::PrintLine("Wireframe -> %s", pState);
    }
    return true;
}

bool Script::ToggleFootIK(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor && pActor->pRagdollController) {
            bool bFootIK = pActor->pRagdollController->QFootIKEnable();
            pActor->pRagdollController->SetFootIKEnable(!bFootIK);
            if (bSystemOutput) {
                const char* pState = pActor->pRagdollController->QFootIKEnable() ? "On" : "Off";
                Interface::PrintLine("FootIK on REF %s %s", apRefObject->GetFormEditorID(), pState);
            }
        }
    }
    else {
        bool bFootIK = bhkRagdollController::QFootIK();
        bhkRagdollController::SetFootIK(!bFootIK);
        if (bSystemOutput) {
            const char* pState = bhkRagdollController::QFootIK() ? "On" : "Off";
            Interface::PrintLine("FootIK %s", pState);
        }
    }
    return true;
}

bool Script::ToggleDebugTextFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Interface::ToggleDebugTextVisible();
    if (bSystemOutput) {
        const char* pState = Interface::IsDebugTextVisible() ? "On" : "Off";
        Interface::PrintLine("Debug Text -> %s", pState);
    }
    return true;
}

bool Script::ShowScenegraphFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    NiTreeCtrl::SetCallback(&TreeCtrlCallback);
    if (apRefObject && apRefObject->Get3D()) {
        const char* pName = apRefObject->GetFullName();
        if (apRefObject == pPlayer) {
            NiTreeCtrl* pWindow = new NiTreeCtrl(
                appMain->hInstance,
                appMain->hWnd,
                apRefObject->Get3D(),
                pPlayer->Is3rdPerson() ? "Player 3rd Person" : "Player 1st Person",
                0x80000000u,
                0x80000000u,
                800,
                600
            );
            const char* pTreeName = pPlayer->Is3rdPerson() ? "Player 1st Person" : "Player 3rd Person";
            NiAVObject* pPlayer3D = pPlayer->Get3D(pPlayer->Is3rdPerson());
            pWindow->CreateTree(pPlayer3D, pTreeName);
            if (pPlayer->GetInventory3D()) {
                pWindow->CreateTree(pPlayer->GetInventory3D(), "Player Inventory Menu");
            }
        }
        else {
            new NiTreeCtrl(
                appMain->hInstance,
                appMain->hWnd,
                apRefObject->Get3D(),
                pName,
                0x80000000u,
                0x80000000u,
                800,
                600
            );
        }
    }
    else {
        NiTreeCtrl* pWindow = new NiTreeCtrl(
            appMain->hInstance,
            appMain->hWnd,
            Main::GetWorldSceneGraph(),
            "Fallout World Scene Graph",
            0x80000000u,
            0x80000000u,
            800,
            600
        );
        pWindow->CreateTree(Main::MenuRootNode(), "Fallout Interface Scene Graph");
        pWindow->CreateTree(Main::MenuRootNode3D(), "Fallout Interface 3D Object Scene Graph");
        if (BSShaderManager::GetShadowSceneNode(1)) {
            pWindow->CreateTree(BSShaderManager::GetShadowSceneNode(1), "Fallout Menu Object 3D Scene Graph");
        }
        if (BSShaderManager::GetShadowSceneNode(3)) {
            pWindow->CreateTree(BSShaderManager::GetShadowSceneNode(3), "Fallout Menu Player 3D Scene Graph");
        }
        if (ScreenSplatter::IsActiveSplatter()) {
            pWindow->CreateTree(ScreenSplatter::GetRoot(), "Fallout Screen Splatter Scene Graph");
        }
    }
    return true;
}

bool Script::ToggleEOFImageSpace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ImageSpaceManager::bEOFEnabled = !ImageSpaceManager::bEOFEnabled;
    return true;
}

bool Script::ToggleAiDetection(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ReferenceProcessLists.bRunDetection = !ReferenceProcessLists.bRunDetection;
    if (bSystemOutput) {
        const char* pState = ReferenceProcessLists.bRunDetection ? "On" : "Off";
        Interface::PrintLine("AI Detection is  %s", pState);
    }
    return true;
}

bool Script::ToggleActorAI(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        pActor->bProcessMe = !pActor->bProcessMe;
        if (bSystemOutput) {
            const char* pState = pActor->bProcessMe ? "On" : "Off";
            Interface::PrintLine("%s processing is  %s", pActor->GetFullName(), pState);
        }
    }
    return true;
}

bool Script::IsActorsAIOff(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        afReturnValue = pActor->bProcessMe ? 0.0 : 1.0;
        if (bSystemOutput) {
            const char* pState = pActor->bProcessMe ? "On" : "Off";
            Interface::PrintLine("%s processing is  %s", pActor->GetFullName(), pState);
        }
    }
    return true;
}

bool Script::ToggleAISchedules(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ReferenceProcessLists.bProcessSche = !ReferenceProcessLists.bProcessSche;
    if (bSystemOutput) {
        const char* pState = ReferenceProcessLists.bProcessSche ? "On" : "Off";
        Interface::PrintLine("AI Processing of Actor's Editor Schedules is  %s", pState);
    }
    return true;
}

bool Script::TogglePathingInfoFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (g_bIsShowPathingInfo) {
        g_bIsShowPathingInfo = false;
    }
    else {
        g_bIsShowPathingInfo = true;
        if (!Interface::IsDebugTextVisible()) {
            Interface::ToggleDebugTextVisible();
        }
        DebugTextManager::GetSingleton()->SetScreenDebugPage("Pathing", 0);
    }
    return true;
}

bool Script::ToggleGrassUpdateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bIsToggleGrassUpdate = !bIsToggleGrassUpdate;
    return true;
}

bool Script::SetAtStartFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        NiNode* p3D = apRefObject->Get3D();
        if (p3D) {
            NiMatrix3 kRot;
            NiMatrix3 kRotate = NiMatrix3::IDENTITY;
            apRefObject->SetStartingPosition(NiPoint3::ZERO);
            NiPoint3 kAngle = apRefObject->GetStartingAngle();
            kRot.MakeXRotation(kAngle.x);
            kRotate = kRot * kRotate;
            kAngle = apRefObject->GetStartingAngle();
            kRot.MakeYRotation(kAngle.y);
            kRotate = kRot * kRotate;
            kAngle = apRefObject->GetStartingAngle();
            kRot.MakeZRotation(kAngle.z);
            kRotate = kRot * kRotate;
            p3D->SetTranslate(apRefObject->GetStartingLocation());
            p3D->SetRotate(kRotate);
            // apRefObject is tested again here.
            if (!apRefObject || !apRefObject->GetAnimation()) {
                NiUpdateData arData(0.0f, false, false);
                p3D->Update(arData);
            }
        }
    }
    return true;
}

bool Script::PlayExplosionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        NiAVObject* pObj = apRefObject->Get3D();
        NiNode* pObj3D = pObj ? pObj->IsNode() : nullptr;
        if (pObj3D && pObj3D->GetControllers()) {
            NiTimeController* pController = pObj3D->GetControllers();
            NiControllerManager* pManager = nullptr;
            while (pController) {
                if (NiObject::IsKindOf_0(&NiControllerManager::ms_RTTI, pController)) {
                    pManager = static_cast<NiControllerManager*>(pController);
                    break;
                }
                pController = pController->GetNext();
            }
            if (pManager) {
                NiControllerSequence* pSeq = pManager->GetSequenceByName(FixedStrings::QSpecialIdle_AreaEffect());
                if (pSeq) {
                    for (uint32_t n = 0; n < pManager->GetSequenceCount(); ++n) {
                        NiControllerSequence* pDisableSeq = pManager->GetSequenceAt(n);
                        const char* pIdleName = animGroupInfo[0].pName;
                        const char* pSeqName = pDisableSeq->m_kName.GetHandleString();
                        if (BSstricmp(pSeqName, pIdleName)) {
                            pManager->DeactivateSequence(pDisableSeq, 0.0f);
                        }
                    }
                    pManager->ActivateSequence(pSeq, 0, false, 1.0f, 0.0f, nullptr);
                    pSeq->SetOffset(-std::numeric_limits<float>::max());
                    pManager->SetActive(true);
                }
                else {
                    int32_t iFormID = apRefObject->iFormID;
                    Warning(
                        "ANIMATION: (PlayExplosion Error) Sequence '%s' not found for '%s' (%08X).",
                        "SpecialIdle_AreaEffect",
                        apRefObject->GetFullName(),
                        iFormID
                    );
                }
            }
            else {
                int32_t iFormID = apRefObject->iFormID;
                Warning(
                    "ANIMATION: (PlayExplosion Error) No NiControllerManager found for '%s' (%08X).",
                    apRefObject->GetFullName(),
                    iFormID
                );
            }
        }
        else {
            int32_t iFormID = apRefObject->iFormID;
            Warning(
                "ANIMATION: (PlayExplosion Error) No 3d or controllers found for '%s' (%08X).",
                apRefObject->GetFullName(),
                iFormID
            );
        }
        apRefObject->SetAltered(true);
    }
    return true;
}

bool Script::ScriptEffectFinishFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    if (apScriptLocals &&
        apScriptLocals->m_pScriptEffectData &&
        apScriptLocals->m_pScriptEffectData->m_bScriptEffectFinish) {
        afReturnValue = 1.0;
    }

    return true;
}

bool Script::GetIsCloudy(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetIsCloudyConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetAttacked(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetAttackedConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::RemoveScriptPackageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        pActor->StopCombat(nullptr);
        TESPackage* pPackage = pActor->GetPackageSetAsPcurrent();
        if (pPackage) {
            if (pPackage->GetIsCreated()) {
                Script::SetActionFlag(pActor->m_Extra.GetPackageExtra(), &pActor->m_Extra, SCRIPT_ACTION_ONPACKAGEEND);
                pActor->m_Extra.RemovePackageExtra();
            }
            else {
                Script::SetActionFlag(pPackage, &pActor->m_Extra, SCRIPT_ACTION_ONPACKAGEEND);
                pActor->pCurrentProcess->ClearCurrentPackage();
                pActor->pCurrentProcess->CheckforNewPackage(pActor, true);
            }
        }
        if (pActor == pPlayer) {
            pPlayer->SetAIControlledPackage(false);
            pPlayer->GetAnimation(true)->SpecialIdleFree(true, false);
            pPlayer->GetAnimation(false)->SpecialIdleFree(true, false);
        }
    }
    return true;
}

bool Script::GetActionRefFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    uint32_t uiID = 0;
    if (apRefObject && apRefObject->GetActionRef()) {
        uiID = apRefObject->GetActionRef()->GetFormID();
        Script::PutNumericIDInDouble(uiID, afReturnValue);
    }

    if (bSystemOutput) {
        Interface::PrintLine("GetActionRef >> (%08x)", uiID);
    }
    return true;
}

bool Script::ResetFallDamageTimerFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && pActor->GetCharController()) {
        pActor->GetCharController()->QFallStartHeight();
    }

    return true;
}

bool Script::IsPlayersLastRiddenHorseFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bool bRetVal = Script::IsPlayersLastRiddenHorseConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (bSystemOutput) {
        Interface::PrintLine("Actor players last ridden horse >> %0.2f", afReturnValue);
    }
    return bRetVal;
}

bool Script::ClearAdaptedLight(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (BSShaderManager::bHDR) {
        ImageSpaceEffectHDR::ClearAdaptedLight();
    }
    return true;
}

bool Script::ToggleVATSLight(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!pVATS.GetVATSLight()) {
        return true;
    }
    if (pVATS.GetVATSLight()->GetLight()->GetAppCulled()) {
        Interface::PrintLine("VATS Lighting On");
        pVATS.GetVATSLight()->GetLight()->SetAppCulled(false);
        ShadowSceneLight* pVATSLight = pVATS.GetVATSLight();
        BSShaderManager::GetShadowSceneNode(0)->UpdateLight(pVATSLight);
    }
    else {
        Interface::PrintLine("VATS Lighting Off");
        pVATS.GetVATSLight()->GetLight()->SetAppCulled(true);
    }
    return true;
}

bool Script::ResetDialogueFlags(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    MobDialogueFlagResetter kResetter;
    ReferenceProcessLists.IterateMobs(kResetter);
    pPlayer->ResetPlayerGreetFlag();
    for (TESTopic* pTopic : *pDataHandler->GetTopicList()) {
        if (pTopic) {
            for (QUEST_INFO* pQuestInfo : *&pTopic->m_listQuestInfo) {
                if (pQuestInfo) {
                    TopicInfoArray* pInfoArray = &pQuestInfo->infoArray;
                    uint32_t uiMaxSize = pInfoArray->m_uiSize;
                    for (uint32_t i = 0; i < uiMaxSize; ++i) {
                        TESTopicInfo* pTopicInfo = pInfoArray->GetAt(i);
                        if (pTopicInfo) {
                            pTopicInfo->ResetSaidOnceFlags();
                        }
                    }
                }
            }
        }
    }
    DialogMenu::DebugResetSaidOnceFlags();
    return true;
}

bool Script::ToggleDepthBiasFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BSShaderManager::bUseDepthBias = !BSShaderManager::bUseDepthBias;
    const char* pState = BSShaderManager::bUseDepthBias ? "ON" : "OFF";
    Interface::PrintLine("Depth Bias is now %s!", pState);
    return true;
}

bool Script::DisableAllMinesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    for (uint32_t i = 0; i < ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMobile = ReferenceProcessLists.MobProcessArray.GetItem(i);
        Projectile* pProjectile = nullptr;
        if (pMobile && pMobile->IsProjectile()) {
            pProjectile = static_cast<Projectile*>(pMobile);
            if (pProjectile && pProjectile->IsMine()) {
                pProjectile->TurnOff(pPlayer, true);
            }
        }
    }
    return true;
}

bool Script::KillQuestUpdates(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (HUDMainMenu::pme) {
        HUDMainMenu::pme->KillQuestUpdates();
    }
    return true;
}

bool Script::GetHeadingAngleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pTarget
        )) {
        return false;
    }
    if (apRefObject && pTarget) {
        return Script::GetHeadingAngleConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
    }
    return true;
}

bool Script::IsWeaponOutFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return !apRefObject || Script::IsWeaponOutConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetDefaultOpenFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (!Script::GetDefaultOpenConditionFunction(apRefObject, nullptr, nullptr, afReturnValue)) {
        return false;
    }
    if (bSystemOutput) {
        Interface::PrintLine("GetDefaultOpen >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsTimePassingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsTimePassingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetCellOwnershipFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    void* pCell = nullptr;
    TESForm* pOwner = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pCell,
            &pOwner
        )) {
        return false;
    }
    if (!pOwner) {
        pOwner = pPlayer->GetObjectReference();
    }
    Script::GetCellOwnershipConditionFunction(apRefObject, pCell, pOwner, afReturnValue);
    return true;
}

bool Script::IsSneakingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::IsSneakingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    return true;
}

bool Script::IsInInteriorFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::IsInInteriorConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    return true;
}

bool Script::GetKillerFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pKiller = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pKiller
           ) &&
        Script::GetKillerConditionFunction(apRefObject, pKiller, nullptr, afReturnValue);
}

bool Script::GetKillerObjectFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pObject = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pObject
           ) &&
        Script::GetKillerObjectConditionFunction(apRefObject, pObject, nullptr, afReturnValue);
}

bool Script::GetFactionCombatReactionFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pFaction = nullptr;
    TESForm* pOtherFaction = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pFaction,
               &pOtherFaction
           ) &&
        Script::GetFactionCombatReactionConditionFunction(apRefObject, pFaction, pOtherFaction, afReturnValue);
}

bool Script::GetFactionRelationFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* pActor = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pActor
        )) {
        return false;
    }
    if (apRefObject && pActor) {
        return Script::GetFactionRelationConditionFunction(apRefObject, pActor, nullptr, afReturnValue);
    }
    return true;
}

bool Script::IsLastPlayedIdleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pIdle = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pIdle
        )) {
        return false;
    }
    if (apRefObject && pIdle) {
        return Script::IsLastPlayedIdleConditionFunction(apRefObject, pIdle, nullptr, afReturnValue);
    }
    return true;
}

bool Script::GetIsAlignmentFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESActorBaseData::ALIGNMENT eAlignment = TESActorBaseData::ALIGNMENT_NEUTRAL;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &eAlignment
           ) &&
        Script::GetIsAlignmentConditionFunction(
            apRefObject, reinterpret_cast<void*>(static_cast<uintptr_t>(eAlignment)), nullptr, afReturnValue
        );
}

bool Script::GetVATSRightAreaFreeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetVATSRightAreaFreeConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::GetVATSLeftAreaFreeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pTarget = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pTarget
           ) &&
        Script::GetVATSLeftAreaFreeConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Script::GetReputationFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iRepType;
    TESReputation* pReputation = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pReputation,
            &iRepType
        )) {
        return false;
    }
    if (iRepType < 0 || iRepType > 1) {
        return false;
    }
    afReturnValue = pReputation->GetReputationValue(static_cast<ReputationType>(iRepType));
    return true;
}

bool Script::GetReputationPctFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iRepType;
    TESReputation* pReputation = nullptr;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pReputation,
            &iRepType
        )) {
        return false;
    }
    if (iRepType < 0 || iRepType > 1) {
        return false;
    }
    afReturnValue = pReputation->GetReputationPct(static_cast<ReputationType>(iRepType));
    return true;
}

bool Script::ChallengeLockedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESChallenge* pChallenge = nullptr;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &pChallenge
    );
    if (!pChallenge) {
        return false;
    }
    afReturnValue = static_cast<double>(pChallenge->GetUnlocked());
    return true;
}

bool Script::GetChallengeCompletedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESChallenge* pChallenge = nullptr;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &pChallenge
    );
    if (!pChallenge) {
        return false;
    }
    int32_t iCompleted = pChallenge->GetCompleted() || pChallenge->GetCompletedRecurring();
    afReturnValue = static_cast<double>(iCompleted);
    return true;
}

TESClimate* Sky::GetCurrentClimate() const {
    return pCurrentClimate;
}

void Sky::ReleaseWeatherOverride() {
    pOverrideWeather = nullptr;
}

BSSimpleList<BGSQuestObjective*>* TESQuest::GetObjectiveList() {
    return &m_listObjectives;
}

void TESQuest::SetScriptProcessingDelay(float afVal) {
    data.fQuestDelayTime = afVal;
}

BSSimpleList<TESQuestStage*>* TESQuest::GetStageList() {
    return &m_listStages;
}

Date* TESQuestStageItem::GetLogDate() {
    return m_pLogDate;
}

TESQuest* TESQuestStageItem::GetOwner() {
    return m_pOwner;
}

uint8_t TESQuestStage::GetIndex() {
    return m_Data.ucIndex;
}

BSSimpleList<TESQuestStageItem*>* TESQuestStage::GetStageItemList() {
    return &listStageItems;
}

TESObjectREFR* TESQuestTarget::QTrackingRef() {
    if (m_TargetPath.Doors.iSize) {
        return m_TargetPath.Doors.GetAt(0).pDoor;
    }

    return m_pTargetRef;
}

// The female mean face coordinates for a nonzero sex, else the male ones.
FR2MatrixVTC<float> (*TESRace::GetMeanFaceCoord(TESActorBase::SEX aeSex))[2][2] {
    if (aeSex) {
        return &MeanFaceCoordFemale;
    }
    return &MeanFaceCoordMale;
}

bool Actor::GetForceRun() {
    return bForceRun;
}

void Projectile::SetFlag(Projectile::ProjectileFlags aeFlag, bool abValue) {
    if (abValue) {
        iFlags |= aeFlag;
    }
    else {
        iFlags &= ~aeFlag;
    }
}

Actor* CombatController::GetTarget() {
    return pTarget;
}

void CombatController::ForceStopCombat() {
    bStopCombat = true;
}

NiFixedString& FixedStrings::QSpecialIdle_AreaEffect() {
    return *FixedStrings::SpecialIdle_AreaEffect;
}

void PlayerCharacter::SetCanWait(bool abFlag) {
    bCanWait = abFlag;
}

MobIterOperator::MobIterOperator() : eType(PROCESS_TYPE_HIGH) {}

void FalloutAudio::SetNoActivationSound(TESSound* apNoActivateSnd) {
    pNoActivationSound = apNoActivateSnd;
}

void FalloutAudio::ClearNoActivationSound() {
    pNoActivationSound = nullptr;
}

const char* DebugTextManager::GetScreenDebugPage() {
    return RegisteredPages[iScreenPage].pName;
}

void FOPipboyManager::SetPipBoyManagerReset(bool abFlag) {
    this->bResetPipboyManager = abFlag;
}

void Main::SetSceneIsComplex(bool abComplex) {
    bSceneIsComplex = abComplex;
}

bool Main::QFaderToBlackVisible() {
    return bFaderToBlackVisible;
}

bool Script::ToggleBordersFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bBordersOn ^= 1u;
    if (!TES::pInstance->TES::GetInterior()) {
        TES::pInstance->ShowLandBorders(bBordersOn);
    }
    if (bSystemOutput) {
        if (bBordersOn) {
            Interface::PrintLine("Borders -> %s", "On");
        }
        else {
            Interface::PrintLine("Borders -> %s", "Off");
        }
    }
    return true;
}

bool Script::ReloadShaders(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BSShaderManager::ShaderErrorCallback pOldCallback = BSShaderManager::GetShaderErrorCallback();
    BSShaderManager::SetShaderErrorCallback(&ConsoleShaderError);
    BSShaderManager::ReloadShaders();
    BSShaderManager::SetShaderErrorCallback(pOldCallback);
    return true;
}

bool Script::RefreshINIFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int8_t cPrefINIPath[264];
    int8_t cCustomINIPath[264];
    int8_t cINIPath[268];
    if (pAppININame) {
        const char* pPersonalPath = BSUtilities::GetPersonalPath();
        BSstrcpy(static_cast<char*>(static_cast<void*>(cINIPath)), 0x104u, pPersonalPath);
        BSstrcat(static_cast<char*>(static_cast<void*>(cINIPath)), 0x104u, pAppININame);
        INISettingCollection* pCollection = SettingT<INISettingCollection>::GetCollection();
        pCollection->ReadSettingsFromFile(static_cast<const char*>(static_cast<const void*>(cINIPath)));
    }
    if (pAppCustomININame) {
        const char* pPersonalPath = BSUtilities::GetPersonalPath();
        BSstrcpy(static_cast<char*>(static_cast<void*>(cCustomINIPath)), 0x104u, pPersonalPath);
        BSstrcat(static_cast<char*>(static_cast<void*>(cCustomINIPath)), 0x104u, pAppCustomININame);
        INISettingCollection* pCollection = SettingT<INISettingCollection>::GetCollection();
        pCollection->ReadSettingsFromFile(static_cast<const char*>(static_cast<const void*>(cCustomINIPath)));
    }
    if (pAppPrefININame) {
        const char* pPersonalPath = BSUtilities::GetPersonalPath();
        BSstrcpy(static_cast<char*>(static_cast<void*>(cPrefINIPath)), 0x104u, pPersonalPath);
        BSstrcat(static_cast<char*>(static_cast<void*>(cPrefINIPath)), 0x104u, pAppPrefININame);
        INIPrefSettingCollection* pCollection = SettingT<INIPrefSettingCollection>::GetCollection();
        pCollection->ReadSettingsFromFile(static_cast<const char*>(static_cast<const void*>(cPrefINIPath)));
    }
    Interface::PrintLine("The in-game settings have been refreshed from the Fallout.ini file.");
    return true;
}

bool Script::DebugCombatPlanFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject->IsActor()) {
        CombatController* pCombatController = static_cast<Actor*>(apRefObject)->GetCombatController();
        if (pCombatController) {
            pCombatController->DebugCombatPlan();
        }
    }
    return true;
}

bool Script::DumpNiUpdatesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Set(1);
    Interface::CloseConsole();
    return true;
}

bool Script::MessageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSMessage* pMessage = nullptr;
    void* pUnused = nullptr;
    if (!ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pMessage,
            &pUnused
        )) {
        return false;
    }
    if (!pMessage) {
        return false;
    }

    alignas(BSStringT<char>) uint8_t aTextStorage[sizeof(BSStringT<char>)];
    BSStringT<char>* pText = ::new (aTextStorage) BSStringT<char>;
    // The chunk is 'DESC' as a little-endian constant.
    TESDescription* pDescription = pMessage;
    const char* pDescriptionText = pDescription->GetDescription(nullptr, DESC_ID);
    if (!ParseString(
            apParameters,
            apCompiledParams,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &afReturnValue,
            &auiOffset,
            pDescriptionText,
            pText
        )) {
        pText->~BSStringT<char>();
        return false;
    }

    ClearButtonPressed();
    if (apRefObject && !apRefObject->GetTemporary()) {
        m_iButtonOwner = apRefObject->GetFormID();
    }
    else if (apScript) {
        m_iButtonOwner = apScript->GetFormID();
    }
    else {
        pText->~BSStringT<char>();
        return false;
    }

    int32_t iButton = 0;
    BSSimpleList<MESSAGEBOX_BUTTON*>* pNode = pMessage->GetButtonList();
    bool bNoButtonShown = true;
    const char* apButtons[10];
    ourMemset(apButtons, 0, sizeof(apButtons));
    while (pNode && pNode->GetItem() && iButton < 10) {
        bool bDispositionFailure;
        if (pNode->GetItem()->Conditions.IsTrueFull(pPlayer, nullptr, &bDispositionFailure, false)) {
            apButtons[iButton] = pNode->GetItem()->Text.String();
            bNoButtonShown = false;
        }
        else {
            apButtons[iButton] = " ";
        }
        ++iButton;
        pNode = pNode->GetNext();
    }
    if (bNoButtonShown) {
        apButtons[0] = "OK";
    }

    if (pMessage->GetIsMessageBox()) {
        TESFullName* pFullName = pMessage;
        const char* pTitle = pFullName->GetFullName();
        // The text is passed through an identity function, which returns it unchanged.
        const char* pMessageText = NavMeshInfo::GetCellOrCoords(const_cast<char*>(pText->String()), 0);
        Interface::CreateMessageMenu(
            pMessageText,
            nullptr,
            pTitle,
            &Script::HandleMessageMenu,
            0,
            0x17,
            0.0f,
            0.0f,
            apButtons[0],
            apButtons[1],
            apButtons[2],
            apButtons[3],
            apButtons[4],
            apButtons[5],
            apButtons[6],
            apButtons[7],
            apButtons[8],
            apButtons[9],
            static_cast<const char*>(nullptr)
        );
    }
    else if (pText->GetLength()) {
        const char* pIconName;
        if (pMessage->GetIcon()) {
            pIconName = pMessage->GetIcon()->GetTextureName();
        }
        else {
            pIconName = nullptr;
        }
        // Widens the display time to a 64-bit integer before rounding it to float.
        float fSeconds = static_cast<float>(static_cast<int64_t>(static_cast<uint64_t>(pMessage->GetDisplayTime())));
        const char* pMessageText = NavMeshInfo::GetCellOrCoords(const_cast<char*>(pText->String()), 0);
        Interface::ShowMessage(pMessageText, 0, pIconName, nullptr, fSeconds, false);
    }
    pText->~BSStringT<char>();
    return true;
}

bool Script::GetWeaponAnimTypeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::GetWeaponAnimTypeConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::SetOpenStateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iOpen = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iOpen
        )) {
        return false;
    }
    if (apRefObject) {
        if (iOpen) {
            if (BGSOpenCloseForm::GetOpenState(apRefObject) > BGSOpenCloseForm::OBJECT_OPENING) {
                apRefObject->Activate(nullptr, false, nullptr, 1);
            }
            apRefObject->SetLastFinishedSequence("Open");
        }
        else {
            if (BGSOpenCloseForm::GetOpenState(apRefObject) < BGSOpenCloseForm::OBJECT_CLOSED) {
                apRefObject->Activate(nullptr, false, nullptr, 1);
            }
            apRefObject->SetLastFinishedSequence("Close");
        }
        ExtraDataList* pExtra = apRefObject->TESObjectREFR::GetExtra();
        pExtra->RemoveSavedAnimation();
    }
    return true;
}

bool Script::TriggerHUDShudderFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fUnused;
    float fIntensity = 500.0f;
    float fDuration = 0.10000000149011612f;
    float fFrequency = 0.05000000074505806f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fIntensity,
            &fDuration,
            &fFrequency,
            &fUnused
        )) {
        return false;
    }
    FOHUDEffectManager* pHUDEffects = Interface::GetHUDEffects();
    pHUDEffects->TriggerShudder(fIntensity, fDuration, fFrequency);
    return true;
}

bool Script::GetOwnerLastTargetFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    afReturnValue = 0.0;
    if (apContainer && apContainer->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apContainer);
        HitData* pLastTargetInfo = pActor->GetLastAttackHitData();
        if (pLastTargetInfo && pLastTargetInfo->pTarget) {
            uint32_t uiFormID = pLastTargetInfo->pTarget->TESForm::GetFormID();
            Script::PutNumericIDInDouble(uiFormID, afReturnValue);
        }
    }

    return true;
}

// Opens the recipe menu for the calling reference's vendor: the actor itself, or the speaker of a talking activator
// (the reference's own base object, or the one its mobile object talks through).
bool Script::ShowRecipeMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* pCategory = nullptr;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &pCategory
    );
    if (apRefObject) {
        MobileObject* pMobile = nullptr;
        TESObjectREFR* pVendor = nullptr;
        if (apRefObject->IsMobileObject()) {
            pMobile = static_cast<MobileObject*>(apRefObject);
        }
        else {
            BGSTalkingActivator* pActivator = dynamic_cast<BGSTalkingActivator*>(apRefObject->GetObjectReference());
            pVendor = pActivator ? pActivator->GetTempRef() : nullptr;
        }
        if (pMobile) {
            if (pMobile->IsActor()) {
                pVendor = pMobile;
            }
            else if (pMobile->IsTalkingActivator()) {
                // Takes the talking activator's base object as a BGSTalkingActivator without a check.
                pVendor = static_cast<BGSTalkingActivator*>(pMobile->GetTalkingActivator()->GetObjectReference())
                              ->GetTempRef();
            }
        }
        if (pVendor) {
            Interface::CreateRecipeMenu(pVendor, pCategory);
        }
    }
    return true;
}

// The winnings level (0-4, quarters of the casino's maximum winnings) the player has reached at a casino.
bool Script::GetCasinoWinningsLevel(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    // The casino's slot and the zeroed slot after it, which ParseParameters is passed with it (call 1's frame bytes).
    TESCasino* apCasino[2];
    // apCasino[0] starts null (a departure): ParseParameters can return before writing it (a null parameter list,
    // a reference code other than 'r'), and the exe then tests it unset.
    apCasino[0] = nullptr;
    apCasino[1] = nullptr;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &apCasino[0]
    );
    TESCasino* pCasino = apCasino[0];
    if (!pCasino) {
        return false;
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
    return true;
}

// Sets the player's winnings level (0-4) at a casino and its winnings to that many quarters of the casino's maximum.
bool Script::SetCasinoWinningsLevel(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    // The casino's slot and the zeroed slot after it, which ParseParameters is passed with it (call 1's frame bytes).
    TESCasino* apCasino[2];
    int32_t iLevel;
    // apCasino[0] starts null (a departure): ParseParameters can return before writing it (a null parameter list,
    // a reference code other than 'r'), and the exe then tests it unset.
    apCasino[0] = nullptr;
    apCasino[1] = nullptr;
    iLevel = 0;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &apCasino[0],
        &iLevel
    );
    TESCasino* pCasino = apCasino[0];
    if (!pCasino) {
        return false;
    }
    for (BSSimpleList<CasinoData*>* pNode = pPlayer->GetCasinoDataList(); pNode && pNode->m_item;
         pNode = pNode->m_pkNext) {
        if (pNode->m_item->casinoID == pCasino->GetFormID() && iLevel >= 0 && iLevel <= 4) {
            int32_t iMaxWinnings = pCasino->data.iMaxWinnings;
            pNode->m_item->iWinningLevel = static_cast<int16_t>(iLevel);
            int32_t iTotalWinnings = static_cast<int32_t>(
                static_cast<double>(iLevel) * 0.25 * static_cast<double>(iMaxWinnings)
            );
            pNode->m_item->iTotalWinnings = iTotalWinnings;
        }
    }
    return true;
}

bool Script::UnlockChallengeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESChallenge* pChallenge = nullptr;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &pChallenge
    );
    if (!pChallenge) {
        return false;
    }
    TESChallengeManager::UnlockChallenge(pChallenge);
    return true;
}

bool Script::DisableHardcoreTracking(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (pPlayer) {
        pPlayer->ClearsHardcoreFlag();
    }
    return true;
}

bool Script::IsAlwaysHardcoreFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::IsAlwaysHardcoreConditionalFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::RemoveRecurringFromChallengeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESChallenge* pChallenge = nullptr;
    Script::ParseParameters(
        apParameters,
        apCompiledParams,
        auiOffset,
        apRefObject,
        apContainer,
        apScript,
        apScriptLocals,
        &pChallenge
    );
    if (!pChallenge) {
        return false;
    }
    pChallenge->RemoveRecurringFromChallenge(1);
    return true;
}

char Script::ActorOnEquipFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONEQUIP, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnEquipFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONEQUIP, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::ActorOnUnequipFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONUNEQUIP, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

char Script::OnUnequipFunction(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* pForm = nullptr;
    if (!Script::ParseParameters(
            paramInfo,
            scriptData,
            *opcodeOffsetPtr,
            thisObj,
            containingObj,
            scriptObj,
            eventList,
            &pForm
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONUNEQUIP, thisObj, pForm, eventList, *afReturnValue);
    return 1;
}

bool Script::BeginTrace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::CalcPathToPointFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::CastFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::DebugMilesPS3Function(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::DisableLinkedPathPointsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::DumpMasterFileSeekData(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::EnableLinkedPathPointsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::EvalActorTextures(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ExportBetaComment(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ForceFileCacheFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ForceRSXCrashFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::GetIsClassDefaultFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::GetIsMyClassDefaultFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::IgnoreRenderPass(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ModPCSkillFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::RunMemoryPass(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::SetCasinoCheatLevel(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::SetHDRParamFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::SetImageSpaceGlowParamFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ShowClassMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ShowSpellmakingMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ShowSurgeryMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::StartMasterFileSeekData(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ToggleActorMover(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ToggleBoundVisGeometry(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ToggleHDRDebugFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ToggleMultibound(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::TogglePathlinesFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ToggleSPURenderBatchFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ToggleSPUTransformUpdateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ToggleScreenBloodTest(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::TrapUpdateFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::SetDepthOfFieldRange(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (Script::bSystemOutput) {
        Interface::PrintLine("This function is no longer supported");
    }
    return true;
}

bool Script::SetTargetDOF(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (Script::bSystemOutput) {
        Interface::PrintLine("This function is no longer supported");
    }
    return true;
}

bool Script::IsShieldOutFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return !apRefObject || Script::IsShieldOutConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::IsTorchOutFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return !apRefObject || Script::IsTorchOutConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetPersuasionNumberFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetPersuasionNumberConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetTotalPersuasionNumberFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return Script::GetTotalPersuasionNumberConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Script::GetPlayerHasLastRiddenHorseFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::GetPlayerHasLastRiddenHorseConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    return true;
}

bool Script::IsHorseStolenFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Script::IsHorseStolenConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    return true;
}

// The leveled list the script names is the TESLeveledList base of a leveled creature; the form it adds is the
// leveled list's owner as well.
bool Script::AddCreatureToLeveledListFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESLevCreature* pList = nullptr;
    TESForm* pForm = nullptr;
    int32_t iLevel = 0;
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pList,
            &pForm,
            &iLevel
        )) {
        return false;
    }
    if (pList && pForm) {
        static_cast<TESLeveledList*>(pList)
            ->AddScriptAddedLeveledObject(pList, static_cast<uint16_t>(iLevel), 1, pForm, nullptr);
    }

    return true;
}

// The leveled list the script names is the TESLeveledList base of a leveled actor; the form it adds is the
// leveled list's owner as well.
bool Script::AddNPCToLeveledListFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESLevCharacter* pList = nullptr;
    TESForm* pForm = nullptr;
    int32_t iLevel = 0;
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pList,
            &pForm,
            &iLevel
        )) {
        return false;
    }
    if (pList && pForm) {
        static_cast<TESLeveledList*>(pList)
            ->AddScriptAddedLeveledObject(pList, static_cast<uint16_t>(iLevel), 1, pForm, nullptr);
    }

    return true;
}

bool Script::PrintTrackedCellsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::ClearTrackedCellsFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}

bool Script::EnableCellTrackingFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return true;
}
