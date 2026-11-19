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
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSDevices/Controls.h"
#include "BSDevices/XPrefSettings.h"
#include "BSDevices/Xenon/XGamerProfile.h"
#include "BSHavok/bhkAvoidBox.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPickData.h"
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
#include "BSMain/BSTestObjects.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/IOManager.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMain/SettingCollectionMap.h"
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
#include "Fallout Shared/BGSImpactData.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
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
#include "Fallout Shared/BSTaskThread.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
#include "Reconstruction.hpp"
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
#include <xmmintrin.h>

using TESModelArray = NiTArray<TESModel*, NiTMallocInterface<TESModel*>>;

void SetRBMass(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkRigidBody* v3 =
        static_cast<bhkRigidBody*>(NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, apCollision->GetWorldObject()));
    if (v3) {
        v3->SetMass(arData.uDataA[0u].f);
        hkpMaxSizeMotion* StoredDynamicMotion = static_cast<hkpMaxSizeMotion*>(v3->GetStoredDynamicMotion());
        if (StoredDynamicMotion) {
            StoredDynamicMotion->setMass(arData.uDataA[0u].f);
        }
    }
}

// Prints the console's shader error text, through NavMeshInfo::GetCellOrCoords (it returns its first argument).
// BSShaderManager's error callback: its caller passes both arguments, but only the text is read.
void ConsoleShaderError(const char* apErrorString, const NiAVObject* apObject) {
    // The identity's owner, NavMeshInfo::GetCellOrCoords, takes a char*; it only returns the text.
    Interface::PrintLine<>(NavMeshInfo::GetCellOrCoords(const_cast<char*>(apErrorString), 0));
}

bool Cmd_GetCurrentWeatherPercent_Execute(
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

bool Cmd_GetDisabled_Execute(
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

bool au_re_Cmd_GetIsLockBroken(
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

bool Cmd_RemoveFromAllFactions(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ExtraDataList* extra;
    ExtraDataList* v4;
    ExtraDataList* v5;
    TESActorBase* actorBase;
    ExtraFactionChanges* factionChangesExtra;
    if ((apRefObject) && (apRefObject->IsActor())) {
        apRefObject->AddChange(0x80000000u);
        extra = apRefObject->TESObjectREFR::GetExtra();
        factionChangesExtra = extra->GetFactionChangesExtra();
        if (!factionChangesExtra) {
            v4 = apRefObject->TESObjectREFR::GetExtra();
            v4->AddFactionChangesExtra();
            v5 = apRefObject->TESObjectREFR::GetExtra();
            factionChangesExtra = v5->GetFactionChangesExtra();
        }
        actorBase = static_cast<Actor*>(apRefObject)->GetActorBase();
        factionChangesExtra->ExpelFromAllFactions(actorBase);
    }

    return true;
}

bool au_re_Cmd_GetXPForNextLevel(
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

bool Cmd_IsActorDetected_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    const char* v7;
    const char* fullName;
    Actor* apActor = dynamic_cast<Actor*>(apRefObject);
    if (apActor) {
        afReturnValue = (double)(&ReferenceProcessLists)->IsActorDetected(apActor);
        if (Script::GetThreadSystemOutput()) {
            if (0.0 == afReturnValue) {
                fullName = apActor->GetFullName();
                Interface::PrintLine(" %s is not detected", fullName);
            }
            else {
                v7 = apActor->GetFullName();
                Interface::PrintLine(" %s is detected", v7);
            }
        }
    }
    return true;
}

bool Cmd_ShowTraitSelectMenu(
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

bool Cmd_ShowTraitMenu(
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

bool Cmd_GetIsGhost_Execute(
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

bool au_re_Cmd_GetMinorCrimeCount(
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

bool Cmd_DrawSkeleton(
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
    uint8_t v10;
    uint8_t v11;
    Actor* pActor;
    uint8_t v13;
    int32_t a8 = 0;
    int32_t v15 = 0;
    int32_t v16 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v15,
            &v16
        )) {
        return false;
    }
    if (apRefObject) {
        pActor = dynamic_cast<Actor*>(apRefObject);
        v10 = (uint8_t)(a8 != 0);
        v13 = (uint8_t)(v15 != 0);
        v11 = (uint8_t)(v16 != 0);
        if ((pActor) && (pActor->pRagdollController)) {
            pNode = pActor->TESObjectREFR::Get3D();
            if (v10 || (v13 || v11)) {
                pActor->SetNonLinesAlpha(pNode, 0x1p-1f);
            }
            else {
                pActor->SetNonLinesAlpha(pNode, 1.0);
            }
            pActor->pRagdollController->ShowDebugLines(v10, v13, v11);
        }
    }
    return true;
}

bool au_re_Cmd_IsGoreDisabled(
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

bool au_re_Cmd_SetArgTo0AndReturnTrue_0(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return ::Cmd_SetArgTo0AndReturnTrue(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Cmd_GetTimeDead_Execute(
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
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("time dead >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_ToggleCombatStats(
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

bool Cmd_AddCardToPlayer(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESBoundObject* objectReference;
    const char* v9;
    TESCaravanCard* v11;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals
        )) {
        Warning("Invalid Parameter used in script AddCardToPlayer");
    }
    if ((apRefObject) && (apRefObject->TESObjectREFR::GetObjectReference())) {
        objectReference = apRefObject->TESObjectREFR::GetObjectReference();
        v11 = dynamic_cast<TESCaravanCard*>(objectReference);
        if (v11) {
            pPlayer->AddCaravanCard(v11);
        }
        else {
            v9 = apRefObject->GetFormEditorID();
            Warning("%s is not a valid CaravanCard object", v9);
        }
    }

    return true;
}

bool Cmd_GetPlayerTeammate_Execute(
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

bool Cmd_ShowWhoDetectsPlayer(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    (&ReferenceProcessLists)->ProcessLists::UpdateEmptyStep(1, (-1));
    Interface::PrintLine("Detection list printed");
    return true;
}

bool Cmd_DispelAllSpells(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* v4;
    if (apRefObject) {
        v4 = dynamic_cast<Actor*>(apRefObject);
        if (v4) {
            static_cast<MagicTarget*>(v4)->RemoveEffects(false);
        }
    }
    return true;
}

bool au_re_Cmd_GetLocked(
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

bool au_re_Cmd_SameFactionAsPC(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return ::Cmd_SameFaction(apRefObject, pPlayer, nullptr, afReturnValue);
}

bool au_re_Cmd_GetIsPlayableRace(
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

bool Cmd_IsActor_Execute(
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
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsActor >> %0.2f", afReturnValue);
    }
    return true;
}

bool au_re_Cmd_GetMajorCrimeCount(
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

bool au_re_Cmd_GetActorAggroRadiusViolated(
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

bool GetForceHitReactionFunction(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals
        )) {
        return false;
    }
    if (apRefObject->IsActor()) {
        afReturnValue = (double)static_cast<Actor*>(apRefObject)->GetForceHitReaction();
    }
    return true;
}

bool Cmd_AddFaceAnimNote(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BSFaceGenAnimationData* v9;
    int8_t a8[516];
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    if (apRefObject) {
        v9 = apRefObject->GetFaceAnimationData(nullptr);
        if (v9) {
            v9->AddAnimationDataFromNote(static_cast<const char*>(static_cast<const void*>(a8)));
        }
    }
    return true;
}

bool au_re_Cmd_HasBeenEaten(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return (int8_t)Script::HasBeenEatenConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Cmd_IsEssential_Execute(
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
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsEssential >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_IsPlayerMovingIntoNewSpace_Execute(
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
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Player is moving to new area >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_DropMe_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if ((!apRefObject) || (!apContainer)) {
        return true;
    }
    TESBoundObject* objectReference = apRefObject->TESObjectREFR::GetObjectReference();
    apContainer->RemoveItem(objectReference, nullptr, 1, false, true, nullptr, nullptr, nullptr, true, false);
    pPlayer->CheckForQuestTargetUpdate(apRefObject);
    if (Interface::IsInMenuMode()) {
        Interface::UpdateInventory();
    }
    return false;
}

bool Cmd_DeprecatedDOF(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("This function is no longer supported");
    }
    return true;
}

bool Cmd_CloseAllMenus(
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

bool Cmd_RemoveMe_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (!apRefObject || !apContainer) {
        return true;
    }
    TESObjectREFR* v11 = dynamic_cast<Actor*>(apContainer);
    ExtraDataList* v10 = nullptr;
    if ((v11) && (v11->WearingObject(apRefObject->GetObjectReference()))) {
        InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(apContainer);
        if (InventoryChanges_) {
            v10 = InventoryChanges_->WearingObject(apRefObject->GetObjectReference(), false);
        }
    }

    apContainer->RemoveItem(apRefObject->GetObjectReference(), v10, 1, false, false, a8, nullptr, nullptr, true, false);
    pPlayer->CheckForQuestTargetUpdate(apRefObject);
    if (apContainer == pPlayer) {
        Interface::UpdateInventory();
    }
    return false;
}

bool Cmd_DuplicateAllItems_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* a8;
    int32_t v11;
    a8 = nullptr;
    v11 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v11
        )) {
        return false;
    }
    bool v12 = v11 != 0;
    if (apRefObject) {
        // &apRefObject->m_Extra is never null.
        InventoryChanges* ContainerChanges = (&apRefObject->m_Extra)->GetContainerChanges();
        if (ContainerChanges) {
            ContainerChanges->DuplicateAllItems(apRefObject, a8);
        }
    }
    return true;
}

bool Cmd_RemoveAllTypedItems(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* a8;
    int32_t v11;
    int32_t aiFormType;
    int32_t v13;
    BGSListForm* apDoNotRemoveList;
    a8 = nullptr;
    v11 = 0;
    v13 = 0;
    aiFormType = -1;
    apDoNotRemoveList = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v11,
            &v13, &aiFormType, &apDoNotRemoveList
        )) {
        return false;
    }
    bool v15 = v11 != 0;
    if (apRefObject) {
        // &apRefObject->m_Extra is never null.
        InventoryChanges* ContainerChanges = (&apRefObject->m_Extra)->GetContainerChanges();
        if (ContainerChanges) {
            ContainerChanges->RemoveAllTypedItems(
                apRefObject, a8, false, v15, false, v13 != 0, aiFormType, apDoNotRemoveList
            );
            pPlayer->CheckForQuestTargetUpdate(apRefObject);
        }
    }
    return true;
}

bool Cmd_RemoveAllItems(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* a8;
    int32_t v11;
    int32_t v12;
    a8 = nullptr;
    v11 = 0;
    v12 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v11,
            &v12
        )) {
        return false;
    }
    bool v13 = v11 != 0;
    if (apRefObject) {
        // &apRefObject->m_Extra is never null.
        InventoryChanges* ContainerChanges = (&apRefObject->m_Extra)->GetContainerChanges();
        if (ContainerChanges) {
            ContainerChanges->RemoveAllItems(apRefObject, a8, false, v13, false, v12 != 0, -1, nullptr);
            pPlayer->CheckForQuestTargetUpdate(apRefObject);
        }
    }
    return true;
}

bool Cmd_IsOwner_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESBoundObject* a8;
    afReturnValue = 0.0;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (!a8) {
        a8 = pPlayer->GetObjectReference();
    }
    Cmd_IsOwner_Eval(apRefObject, a8, nullptr, afReturnValue);
    return true;
}

bool Cmd_SetOwnership(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESBoundObject* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (!a8) {
        a8 = pPlayer->GetObjectReference();
    }
    if (apRefObject) {
        (&apRefObject->m_Extra)->SetOwnership(a8);
        apRefObject->AddChange(64);
    }
    return true;
}

bool Cmd_Drop_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v9;
    TESBoundObject* a8;
    a8 = nullptr;
    v9 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v9
        )) {
        return false;
    }
    if (apRefObject) {
        apRefObject->RemoveItem(a8, nullptr, v9, false, true, nullptr, nullptr, nullptr, true, false);
    }
    return true;
}

bool Cmd_IsActionRef(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* a8;
    afReturnValue = 0.0;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if ((apRefObject) && (a8)) {
        TESObjectREFR* ActionRef = apRefObject->GetActionRef();
        if (a8 == ActionRef) {
            afReturnValue = 1.0;
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("IsActionRef >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_PlayerSpellBook(
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
        if ((pNode->m_item) &&
            (static_cast<MagicItem*>(static_cast<SpellItem*>(pNode->m_item))->GetSpellType() == MagicSystem::SPELL ||
             static_cast<MagicItem*>(static_cast<SpellItem*>(pNode->m_item))->GetSpellType() == MagicSystem::POWER ||
             static_cast<MagicItem*>(static_cast<SpellItem*>(pNode->m_item))->GetSpellType() ==
                 MagicSystem::LESSER_POWER) &&
            (pPlayer->AddSpell(static_cast<SpellItem*>(pNode->m_item)))) {
            ++iCount;
        }

        pNode = pNode->m_pkNext;
    }
    Interface::PrintLine("%d spells added to Player Character", iCount);
    return true;
}

bool Cmd_PrintAiList(
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

bool Cmd_SetWeather(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v10;
    TESWeather* a8;
    a8 = nullptr;
    v10 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v10
        )) {
        return false;
    }
    Sky* Instance = Sky::GetInstance();
    if (Instance->pCurrentWeather) {
        if (v10) {
            Instance->pOverrideWeather = a8;
        }
        else {
            Instance->pDefaultWeather = a8;
            pPlayer->SetOccupiedRegion(nullptr);
        }
        if (!GetThreadTLSData()->bSystemOutput) {
            Instance->SetAcceleration(true);
        }
    }
    else {
        Instance->ForceWeather(a8, v10 != 0);
    }
    return true;
}

bool Cmd_ForceWeather(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v12;
    TESWeather* a8;
    a8 = nullptr;
    v12 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v12
        )) {
        return false;
    }
    bool v11 = v12 != 0;
    TESWeather* v10 = a8;
    Sky* Instance = Sky::GetInstance();
    Instance->ForceWeather(v10, v11);
    return true;
}

bool Cmd_ReleaseWeatherOverride(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Sky* Instance = Sky::GetInstance();
    Instance->ReleaseWeatherOverride();
    return true;
}

bool Cmd_ApplyImageSpaceModifier(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESImageSpaceModifier* a8;
    float afStrength;
    a8 = nullptr;
    afStrength = 1.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
            &afStrength
        )) {
        return false;
    }
    ImageSpaceModifierInstanceForm::Trigger(a8, afStrength, nullptr);
    return true;
}

bool Cmd_SetCellImageSpace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectCELL* a8;
    TESImageSpace* apImageSpace;
    apImageSpace = nullptr;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
            &apImageSpace
        )) {
        return false;
    }
    a8->SetImageSpace(apImageSpace);
    return true;
}

bool Cmd_SetImageSpace(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESImageSpace* pImageSpace;
    pImageSpace = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pImageSpace
        )) {
        return false;
    }
    if (pImageSpace) {
        ImageSpaceParameterData* pData = &pImageSpace->Data;
        ImageSpaceManager* pManager = BSShaderManager::GetImageSpaceManager();
        pManager->SetActiveImageSpaceParams(pData);
    }
    else {
        ImageSpaceManager* pManager = BSShaderManager::GetImageSpaceManager();
        pManager->SetActiveImageSpaceParams(nullptr);
    }
    ImageSpaceManager* pManager = BSShaderManager::GetImageSpaceManager();
    pManager->ResetToBaseParameterData();
    return true;
}

bool Cmd_ModScale(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float a8;
    a8 = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject) {
        double Scale = apRefObject->GetScale();
        a8 = Scale + a8;
        if (!(a8 > 2.0)) {
            if (a8 < 0.5) {
                a8 = 0.5f;
            }
        }
        else {
            a8 = 2.0f;
        }
        Script::SetScaleBase(apRefObject, a8);
    }
    return true;
}

bool Cmd_GetLineOfSight(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pTarget;
    pTarget = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pTarget
           ) &&
        Script::GetLOSConditionFunction(apRefObject, pTarget, nullptr, afReturnValue);
}

bool Cmd_RemoveSpell(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    SpellItem* pSpell;
    pSpell = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pSpell
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

bool Cmd_CastImmediateOnSelf(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    MagicItem* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject) {
        if (!a8 || a8->GetSpellType() == MagicSystem::ABILITY) {
            return true;
        }
        MagicCaster* v9 = apRefObject->GetMagicCaster();
        MagicTarget* v10 = apRefObject->GetMagicTarget();
        v9->InterruptCast();
        TESObjectCELL* a1 = apRefObject->GetParentCell();
        if ((a1) && (TES::pInstance->IsCellLoaded(a1, true) && TES::pInstance->IsCellLoaded(a1, false))) {
            v9->CastSpellImmediate(a8, false, v10, 1.0, false);
        }
    }
    return true;
}

bool Cmd_Dispel(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    MagicItem* a8;
    int32_t v11;
    a8 = nullptr;
    v11 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v11
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* v9 = dynamic_cast<Actor*>(apRefObject);
        if (v9) {
            v9->Dispel(a8, nullptr, nullptr);
            if (a8->HasScript(apScript)) {
                return false;
            }
        }
    }
    return true;
}

bool Cmd_DisableAllActors(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* v11 = dynamic_cast<Actor*>(apRefObject);
    for (uint32_t uiIndex = 0;
         uiIndex < (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_LOW); ++uiIndex) {
        MobileObject* Item = (&ReferenceProcessLists.MobProcessArray)->GetItem(uiIndex);
        if ((Item) && (Item->IsActor() && Item != v11 && !Item->GetDisabled())) {
            Script::AddPendingDisabledReference(Item, true);
            apScriptLocals->m_cFlags |= 1;
        }
    }
    return true;
}

bool Cmd_GetIsFormType(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[516];
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    int32_t FormType = apRefObject->GetObjectReference()->GetFormType();
    if (!std::strcmp(formEnumString[FormType].pFormString, a8)) {
        afReturnValue = 1.0;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Get Is Form Type >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_GetDetected_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetDetectedConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool Cmd_GetQuestRunning_0(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetQuestRunningConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool Cmd_StartQuest(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8) {
        a8->SetEnabled(1);
    }
    return true;
}

bool Cmd_StopQuest(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8) {
        a8->SetEnabled(0);
    }
    return true;
}

bool Cmd_CompleteQuest(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8) {
        a8->SetCompleted(true);
    }
    return true;
}

bool Cmd_ModFactionRank_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* a8;
    int32_t v12;
    a8 = nullptr;
    v12 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v12
        )) {
        return false;
    }
    afReturnValue = 0.0;
    Actor* v10 = dynamic_cast<Actor*>(apRefObject);
    if (!v10) {
        return true;
    }
    TESActorBase* ActorBase = v10->GetActorBase();
    if ((v10) && (a8)) {
        v10->ModFactionRank(a8, v12, v10 == pPlayer);
    }

    return true;
}

bool Cmd_GetFactionReaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* a8;
    TESFaction* pFaction;
    afReturnValue = 0.0;
    a8 = nullptr;
    pFaction = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
            &pFaction
        )) {
        return false;
    }
    if ((a8) && (pFaction)) {
        afReturnValue = static_cast<double>(a8->GetFactionReaction(pFaction));
    }

    if (GetThreadTLSData()->bSystemOutput) {
        double v10 = afReturnValue;
        const char* FullName = pFaction->GetFullName();
        Interface::PrintLine("%.20s reaction to %.20s is %.1f", a8->GetFullName(), FullName, v10);
    }
    return true;
}

bool Cmd_SetFactionReaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* a8;
    TESFaction* pFaction;
    int32_t reaction;
    a8 = nullptr;
    pFaction = nullptr;
    reaction = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
            &pFaction, &reaction
        )) {
        return false;
    }
    afReturnValue = 0.0;
    if ((pFaction) && (a8)) {
        a8->SetFactionReaction_0(pFaction, reaction);
    }

    return true;
}

bool Cmd_SetAlly(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* a8;
    TESForm* a2;
    int32_t v11;
    int32_t v12;
    a8 = nullptr;
    a2 = nullptr;
    v12 = 0;
    v11 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &a2,
            &v12, &v11
        )) {
        return false;
    }
    afReturnValue = 0.0;
    if ((a2) && (a8)) {
        if (v12) {
            a8->SetFactionReaction(static_cast<TESFaction*>(a2), 3);
        }
        else {
            a8->SetFactionReaction(static_cast<TESFaction*>(a2), 2);
        }
        if (v11) {
            static_cast<TESFaction*>(a2)->TESFaction::SetFactionReaction(a8, 3);
        }
        else {
            static_cast<TESFaction*>(a2)->TESFaction::SetFactionReaction(a8, 2);
        }
    }

    return true;
}

bool Cmd_SetEnemy(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* a8;
    TESForm* a2;
    int32_t v11;
    int32_t v12;
    a8 = nullptr;
    a2 = nullptr;
    v12 = 0;
    v11 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &a2,
            &v12, &v11
        )) {
        return false;
    }
    afReturnValue = 0.0;
    if ((a2) && (a8)) {
        if (v12) {
            a8->SetFactionReaction(static_cast<TESFaction*>(a2), 0);
        }
        else {
            a8->SetFactionReaction(static_cast<TESFaction*>(a2), 1);
        }
        if (v11) {
            static_cast<TESFaction*>(a2)->TESFaction::SetFactionReaction(a8, 0);
        }
        else {
            static_cast<TESFaction*>(a2)->TESFaction::SetFactionReaction(a8, 1);
        }
    }

    return true;
}

bool Cmd_ModFactionReaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* pFaction;
    TESFaction* pOtherFaction;
    int32_t iReaction;
    pFaction = nullptr;
    pOtherFaction = nullptr;
    iReaction = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pFaction,
            &pOtherFaction, &iReaction
        )) {
        return false;
    }
    afReturnValue = 0.0;
    if ((pOtherFaction) && (pFaction)) {
        pFaction->ModFactionReaction(pOtherFaction, iReaction);
    }

    return true;
}

bool Cmd_ShowPivot(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    NiPoint3 v31;
    NiPoint3 v32;
    NiPoint3 v33;
    NiPoint3 v34;
    NiPoint3 v35;
    NiPoint3 dst;
    if (apRefObject) {
        NiLines* v27;
        NiPoint3* pkVertex;
        NiColorA* pkColor;
        float v50 = 10.0f;
        if (apRefObject->Get3D()) {
            NiNode* v8 = apRefObject->Get3D();
            const NiBound* WorldBound = &v8->GetWorldBound();
            float v48 = WorldBound->m_fRadius * 0.5;
            if (v50 > static_cast<double>(v48)) {
                v50 = v48;
            }
        }
        void* ptr = ourMalloc(6 * sizeof(NiPoint3));
        if (ptr) {
            rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), 6, &emptyPointConstructor);
            pkVertex = static_cast<NiPoint3*>(ptr);
        }
        else {
            pkVertex = nullptr;
        }
        void* v44 = ourMalloc(6 * sizeof(NiColorA));
        if (v44) {
            pkColor = ::new (v44) NiColorA[6];
        }
        else {
            pkColor = nullptr;
        }
        bool* v43 = static_cast<bool*>(ourMalloc(6 * sizeof(bool)));
        *v43 = true;
        v43[1] = false;
        v43[2] = true;
        v43[3] = false;
        v43[4] = true;
        v43[5] = false;
        NiColorA apMemory(1.0f, 1.0f, 0.0f, 1.0f);
        NiColorA* v10 = &apMemory;
        *pkColor = *v10;
        NiColorA v41(1.0f, 1.0f, 0.0f, 1.0f);
        NiColorA* v11 = &v41;
        pkColor[1] = *v11;
        NiColorA v40(1.0f, 1.0f, 0.0f, 1.0f);
        NiColorA* v12 = &v40;
        pkColor[2] = *v12;
        NiColorA v39(1.0f, 1.0f, 0.0f, 1.0f);
        NiColorA* v13 = &v39;
        pkColor[3] = *v13;
        NiColorA v38(1.0f, 1.0f, 0.0f, 1.0f);
        NiColorA* v14 = &v38;
        pkColor[4] = *v14;
        NiColorA v37(1.0f, 1.0f, 0.0f, 1.0f);
        NiColorA* v15 = &v37;
        pkColor[5] = *v15;
        NiPoint3* v16 = ::new (&dst) NiPoint3(-v50, 0.0f, 0.0f);
        *pkVertex = *v16;
        NiPoint3* v17 = ::new (&v35) NiPoint3(v50, 0.0f, 0.0f);
        pkVertex[1] = *v17;
        NiPoint3* v18 = ::new (&v34) NiPoint3(0.0f, -v50, 0.0f);
        pkVertex[2] = *v18;
        NiPoint3* v19 = ::new (&v33) NiPoint3(0.0f, v50, 0.0f);
        pkVertex[3] = *v19;
        NiPoint3* v20 = ::new (&v32) NiPoint3(0.0f, 0.0f, -v50);
        pkVertex[4] = *v20;
        NiPoint3* v21 = ::new (&v31) NiPoint3(0.0f, 0.0f, v50);
        pkVertex[5] = *v21;
        NiLines* v30 = static_cast<NiLines*>(NiMemObject::operator new(sizeof(NiLines)));
        if (v30) {
            v27 = ::new (v30) NiLines(
                6, pkVertex, pkColor, nullptr, 1, NiGeometryData::NBT_METHOD_NONE, reinterpret_cast<uint8_t*>(v43)
            );
        }
        else {
            v27 = nullptr;
        }
        const NiPoint3* v22 = &apRefObject->GetLocationOnReference();
        v27->SetTranslate(*v22);
        TES::pInstance->AddTempDebugObject(v27, 20.0f);
    }
    return true;
}

bool Cmd_GetIsCurrentPackage_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject && a8) {
        return Script::GetIsCurrentPackageConditionFunction(apRefObject, a8, nullptr, afReturnValue);
    }
    return true;
}

bool Cmd_ShowSlotMachineMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESCasino* a8;
    int32_t aiMaxBet;
    int32_t aiMinBet;
    int32_t aiMinWinnings;
    if (pPlayer) {
        aiMinBet = 0;
        aiMaxBet = 0;
        aiMinWinnings = 0;
        if (Script::ParseParameters(
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
                &aiMinBet, &aiMaxBet, &aiMinWinnings
            )) {
            if (a8) {
                SlotMachineMenu::Create(a8, aiMinBet, aiMaxBet, aiMinWinnings, false);
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

bool Cmd_ShowBlackJackMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESCasino* a8;
    int32_t aiMaxBet;
    int32_t aiMinBet;
    int32_t v12;
    if (pPlayer) {
        aiMinBet = 0;
        aiMaxBet = 0;
        v12 = 0;
        if (Script::ParseParameters(
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
                &aiMinBet, &aiMaxBet, &v12
            )) {
            if (a8) {
                BlackJackMenu::Create(a8, aiMinBet, aiMaxBet, 0, false);
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

bool Cmd_ShowRouletteMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESCasino* a8;
    int32_t aiMaxBet;
    int32_t aiMinBet;
    int32_t aiMinWinnings;
    if (pPlayer) {
        aiMinBet = 0;
        aiMaxBet = 0;
        aiMinWinnings = 0;
        if (Script::ParseParameters(
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
                &aiMinBet, &aiMaxBet, &aiMinWinnings
            )) {
            if (a8) {
                RouletteMenu::Create(a8, aiMinBet, aiMaxBet, aiMinWinnings, false);
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

bool Cmd_ShowCaravanMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESCaravanDeck* a8;
    int32_t aiDiffLevel;
    float afBetPercentage;
    if (pPlayer) {
        a8 = nullptr;
        MobileObject* pMobile = nullptr;
        Actor* arg0 = nullptr;
        aiDiffLevel = 0;
        afBetPercentage = 0.0f;
        if (!Script::ParseParameters(
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
                &aiDiffLevel, &afBetPercentage
            )) {
            Warning("Invalid Parameters used in script ShowCaravanMenu ");
            return true;
        }
        if ((apRefObject) && (apRefObject != pPlayer) && (a8)) {
            if (apRefObject->IsMobileObject()) {
                pMobile = static_cast<MobileObject*>(apRefObject);
            }
            else {
                BGSTalkingActivator* pActivator = dynamic_cast<BGSTalkingActivator*>(apRefObject->GetObjectReference());
                arg0 = pActivator ? pActivator->pTempRef : nullptr;
            }
            if (pMobile) {
                if (pMobile->IsActor()) {
                    arg0 = static_cast<Actor*>(pMobile);
                }
                else if (pMobile->IsTalkingActivator()) {
                    // The talking activator reference's base object is the BGSTalkingActivator.
                    arg0 = static_cast<BGSTalkingActivator*>(pMobile->GetTalkingActivator()->GetObjectReference())
                               ->pTempRef;
                }
            }
            CaravanMenu::Create(arg0, a8, aiDiffLevel, afBetPercentage, false);
        }
    }
    return true;
}

bool Cmd_IsPlayerInRegion(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pRegion
        )) {
        afReturnValue = 0.0;
        if (pRegion) {
            TESRegionList* p_m_pkNext;
            for (TESRegionList* i = pPlayer->GetAllOccupiedRegions(); i && i->m_item; i = p_m_pkNext) {
                if (i->m_item->iFormID == pRegion->iFormID) {
                    afReturnValue = 1.0;
                    if (GetThreadTLSData()->bSystemOutput) {
                        Interface::PrintLine("IsPlayerInRegion >> %0.f", afReturnValue);
                    }
                    return true;
                }
                // Walks the nodes as region lists: each next node is downcast to the list around it.
                BSSimpleList<TESRegion*>* pNext = i->m_pkNext;
                if (pNext) {
                    p_m_pkNext = static_cast<TESRegionList*>(pNext);
                }
                else {
                    p_m_pkNext = nullptr;
                }
            }
        }
    }
    else {
        Warning("Invalid Parameter used in script IsPlayerInRegion");
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("IsPlayerInRegion >> %0.f", afReturnValue);
    }
    return true;
}

bool Cmd_ShowComputersInterface(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESBoundObject* ObjectReference;
    if (apRefObject && (ObjectReference = apRefObject->GetObjectReference(), ObjectReference->GetFormType() == 23)) {
        Interface::QueueMenuCreate(
            Interface::OPEN_COMPUTERS, apRefObject, nullptr, nullptr, Interface::CMM_STANDARD, false
        );
    }
    else {
        Interface::QueueMenuCreate(
            Interface::OPEN_COMPUTERS, nullptr, nullptr, nullptr, Interface::CMM_STANDARD, false
        );
    }
    return true;
}

bool GetSandmanFunction(
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

bool GetCannibalFunction(
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

bool Cmd_StartMisterSandMan(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pTarget
        )) {
        return false;
    }
    if ((apRefObject->IsActor()) && (pTarget) && (pTarget->IsActor())) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        BaseProcess* pProcess1 = pActor->GetCurrentAIProcess();
        BaseProcess* pProcess2 = pActor->GetCurrentAIProcess();
        BaseProcess* pProcess3 = pActor->GetCurrentAIProcess();
        // The arguments are evaluated right to left, as the game does: the index, then the marker, then the
        // furniture.
        uint32_t uiIndex = pProcess1->GetCurrentFurnitureIndex();
        FurnitureMark* pMarker = pProcess2->GetFurnitureMarker();
        TESObjectREFR* pFurniture = pProcess3->GetCurrentFurniture();
        pTarget->InitiateSandmanPackage(pActor, pFurniture, pMarker, static_cast<uint8_t>(uiIndex));
    }

    return true;
}

bool Cmd_StartCannibal(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pTarget
        )) {
        return false;
    }
    if ((apRefObject->IsActor()) && (pTarget) && (pTarget->IsActor())) {
        pTarget->InitiateCannibalPackage(static_cast<Actor*>(apRefObject));
    }

    return true;
}

bool Cmd_SetGhost_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    Actor* v10 = dynamic_cast<Actor*>(apRefObject);
    if (v10) {
        v10->SetGhost(a8 > 0);
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("SetGhost >> %d", a8);
    }
    return true;
}

bool Cmd_EquipItem_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v20;
    TESForm* item;
    int32_t v22;
    item = nullptr;
    v20 = 0;
    v22 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &item, &v20,
            &v22
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* v18 = dynamic_cast<Actor*>(apRefObject);
        bool v19 = v20 > 0;
        if (item) {
            if (v18) {
                InventoryChanges* ContainerChanges = (&v18->m_Extra)->GetContainerChanges();
                ItemChange* InventoryItem = nullptr;
                uint8_t bEquip = 1;
                if (ContainerChanges) {
                    InventoryItem = ContainerChanges->GetInventoryItem(static_cast<TESBoundObject*>(item), 0);
                    if (InventoryItem) {
                        // The list's head, item and next getters are trivial accessors.
                        BSSimpleList<ExtraDataList*>* pNode = InventoryItem->pExtraObjectList;
                        while (pNode && pNode->GetItem()) {
                            ExtraDataList* pList = pNode->GetItem();
                            if (pList && pList->ExtraDataList::GetWorn(false)) {
                                pList->ExtraDataList::SetCanNotWear(v19);
                                bEquip = 0;
                                break;
                            }
                            pNode = pNode->GetNext();
                        }
                        delete InventoryItem;
                    }
                }
                if (bEquip) {
                    v18->QueueEquipObject(static_cast<TESBoundObject*>(item), 1, nullptr, true, v19, true);
                    if (v18 == pPlayer && !v22) {
                        BSStringT<char> v11;
                        const char* v9 = sEquipItemOnPlayer.String();
                        v11.SPrintF("%s %s.", TESFullName::GetFullName(item), v9);
                        Interface::ShowMessage(
                            v11.pString, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds",
                            nullptr, 2.0f, false
                        );
                        pPlayer->UpdatePlayer3d();
                        BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
                        CurrentAIProcess->SetShouldRefreshMagicShadersEffects(true, false, false);
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

bool Cmd_UnequipItem_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ExtraDataList* item;
    ExtraDataList* a2;
    int32_t v26;
    TESBoundObject* a8;
    int32_t v28;
    a8 = nullptr;
    v26 = 0;
    v28 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v26,
            &v28
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* v24 = dynamic_cast<Actor*>(apRefObject);
        bool v25 = v26 > 0;
        if (a8) {
            if (v24) {
                InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(apRefObject);
                if (InventoryChanges_) {
                    ExtraDataList* v22 = InventoryChanges_->WearingObject(a8, false);
                    if (v22) {
                        v22->SetCanNotWear(false);
                        v24->QueueUnEquipObject(a8, 1, nullptr, true, v25, true);
                        if (v24 == pPlayer && !v28) {
                            BSStringT<char> apString;
                            const char* v9 = sUnequipItemOnPlayer.String();
                            apString.SPrintF("%s %s.", TESFullName::GetFullName(a8), v9);
                            Interface::ShowMessage(
                                apString.pString, 0,
                                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds", nullptr, 2.0f,
                                false
                            );
                            pPlayer->UpdatePlayer3d();
                            BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
                            CurrentAIProcess->SetShouldRefreshMagicShadersEffects(true, false, false);
                            Interface::UpdateInventory();
                        }
                    }
                }
            }
        }
        else {
            InventoryChanges* v20 = InventoryChanges::GetInventoryChanges(apRefObject);
            if (v20) {
                ItemChange* InventoryItem = v20->GetInventoryItem(a8, 0);
                if (InventoryItem) {
                    // The list's head and item getters are trivial accessors.
                    if (InventoryItem->pExtraObjectList) {
                        if (InventoryItem->pExtraObjectList->GetItem()) {
                            InventoryItem->pExtraObjectList->GetItem()->SetCanNotWear(v25);
                        }
                        else {
                            ExtraDataList* v12;
                            ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                            if (apMemory) {
                                v12 = ::new (apMemory) ExtraDataList;
                            }
                            else {
                                v12 = nullptr;
                            }
                            a2 = v12;
                            a2->SetCanNotWear(v25);
                            InventoryItem->pExtraObjectList->AddHead(a2);
                        }
                    }
                    else {
                        ExtraDataList* v10;
                        BSSimpleList<ExtraDataList*>* v11;
                        BSSimpleList<ExtraDataList*>* v15 =
                            static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
                        if (v15) {
                            v11 = ::new (v15) BSSimpleList<ExtraDataList*>;
                        }
                        else {
                            v11 = nullptr;
                        }
                        InventoryItem->pExtraObjectList = v11;
                        ExtraDataList* v14 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                        if (v14) {
                            v10 = ::new (v14) ExtraDataList;
                        }
                        else {
                            v10 = nullptr;
                        }
                        item = v10;
                        item->SetCanNotWear(v25);
                        InventoryItem->pExtraObjectList->AddHead(item);
                    }
                    if (InventoryItem) {
                        delete InventoryItem;
                    }
                }
            }
        }
    }
    return true;
}

bool Cmd_SetClass_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESClass* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* v11 = dynamic_cast<Actor*>(apRefObject);
        if ((v11) && (a8)) {
            TESBoundObject* ObjectReference = v11->GetObjectReference();
            TESNPC* v10 = dynamic_cast<TESNPC*>(ObjectReference);
            if (v10) {
                v10->SetClass(a8);
                v10->InitValues(false);
            }
        }
    }
    return true;
}

bool Cmd_ResetInterior(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectCELL* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8) {
        a8->SetDetachTime(0xFFFFFFFFu, false);
    }
    return true;
}

bool Cmd_WaterShallowColor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    int32_t v22;
    int32_t v23;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v22,
            &v23
        )) {
        return false;
    }
    WaterShaderProperty* v24 = nullptr;
    if (apRefObject) {
        NiNode* v18 = apRefObject->Get3D();
        NiNode* v19 = static_cast<NiNode*>(v18->GetAt(0));
        WaterShaderProperty* Property = static_cast<WaterShaderProperty*>(v19->GetProperty(3));
        v24 = Property;
    }
    if (a8 >= 0 && v22 >= 0 && v23 >= 0 && a8 <= 255 && v22 <= 255 && v23 <= 255) {
        if (v24) {
            float v15 = static_cast<double>(v23) / 255.0;
            float v14 = static_cast<double>(v22) / 255.0;
            float v13 = static_cast<double>(a8) / 255.0;
            NiColorA apMemory(v13, v14, v15, 1.0f);
            NiColorA* v9 = &apMemory;
            v24->pShallowColor = *v9;
        }
        else {
            float fB = static_cast<double>(v23) / 255.0;
            float fG = static_cast<double>(v22) / 255.0;
            float fR = static_cast<double>(a8) / 255.0;
            NiColorA kDefaultColor(fR, fG, fB, 1.0f);
            NiColorA* v10 = &kDefaultColor;
            WaterShader::kShallowColor = *v10;
        }
    }
    return true;
}

bool Cmd_WaterDeepColor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    int32_t v22;
    int32_t v23;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v22,
            &v23
        )) {
        return false;
    }
    WaterShaderProperty* v24 = nullptr;
    if (apRefObject) {
        NiNode* v18 = apRefObject->Get3D();
        NiNode* v19 = static_cast<NiNode*>(v18->GetAt(0));
        WaterShaderProperty* Property = static_cast<WaterShaderProperty*>(v19->GetProperty(3));
        v24 = Property;
    }
    if (a8 >= 0 && v22 >= 0 && v23 >= 0 && a8 <= 255 && v22 <= 255 && v23 <= 255) {
        if (v24) {
            float v15 = static_cast<double>(v23) / 255.0;
            float v14 = static_cast<double>(v22) / 255.0;
            float v13 = static_cast<double>(a8) / 255.0;
            NiColorA apMemory(v13, v14, v15, 1.0f);
            NiColorA* v9 = &apMemory;
            v24->pDeepColor = *v9;
        }
        else {
            float fB = static_cast<double>(v23) / 255.0;
            float fG = static_cast<double>(v22) / 255.0;
            float fR = static_cast<double>(a8) / 255.0;
            NiColorA kDefaultColor(fR, fG, fB, 1.0f);
            NiColorA* v10 = &kDefaultColor;
            WaterShader::kDeepColor = *v10;
        }
    }
    return true;
}

bool Cmd_WaterReflectionColor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    int32_t v22;
    int32_t v23;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v22,
            &v23
        )) {
        return false;
    }
    WaterShaderProperty* v24 = nullptr;
    if (apRefObject) {
        NiNode* v18 = apRefObject->Get3D();
        NiNode* v19 = static_cast<NiNode*>(v18->GetAt(0));
        WaterShaderProperty* Property = static_cast<WaterShaderProperty*>(v19->GetProperty(3));
        v24 = Property;
    }
    if (a8 >= 0 && v22 >= 0 && v23 >= 0 && a8 <= 255 && v22 <= 255 && v23 <= 255) {
        if (v24) {
            float v15 = static_cast<double>(v23) / 255.0;
            float v14 = static_cast<double>(v22) / 255.0;
            float v13 = static_cast<double>(a8) / 255.0;
            NiColorA apMemory(v13, v14, v15, 1.0f);
            NiColorA* v9 = &apMemory;
            v24->pReflectionColor = *v9;
        }
        else {
            float v12 = static_cast<double>(v23) / 255.0;
            float v11 = static_cast<double>(v22) / 255.0;
            float v10 = static_cast<double>(a8) / 255.0;
            WaterShader::kReflectionColor = NiColorA(v10, v11, v12, 1.0f);
        }
    }
    return true;
}

bool Cmd_DamageObject(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float a8;
    a8 = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (!apRefObject) {
        return false;
    }
    if (iNumHWThreads.Int() <= 1) {
        Script::DamageObjectBase(apRefObject, a8);
    }
    else {
        TaskQueueInterface::GetInstance()->QueueScriptFunctionCall(0x116Cu, apRefObject, a8);
    }
    return true;
}

bool Cmd_ResetAllTerminals(
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

bool Cmd_ExitGame_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(0xED568u);
    BGSPerk* v11 = dynamic_cast<BGSPerk*>(FormByNumericID);
    float afValue = static_cast<float>((pPlayer->GetPerkRank(v11, false) != 0) + 1);
    pPlayCredits->SetValue(afValue);
    Interface::CloseConsole();
    StartMenu::ChooseMainMenu();
    return true;
}

bool Cmd_ShowWarning_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char result[518];
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &result[2]
        )) {
        return false;
    }
    BSStringT<char> out;
    return Script::ParseString(
        apParameters, apCompiledParams, apRefObject, apContainer, apScript, apScriptLocals, &afReturnValue, &auiOffset,
        &result[2], &out
    );
}

bool Cmd_EnterTrigger_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* a1;
    a1 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a1
        )) {
        return false;
    }
    if (apRefObject) {
        Script::InitActionList(apRefObject, &apRefObject->m_Extra);
        ExtraDataList* Extra = &apRefObject->m_Extra;
        Script::SetActionFlag(a1, Extra, SCRIPT_ACTION_ONTRIGGERENTER);
        ExtraDataList* v10 = &apRefObject->m_Extra;
        Script::SetActionFlag(a1, v10, SCRIPT_ACTION_ONTRIGGER);
        apRefObject->SetActionRef(static_cast<TESObjectREFR*>(a1));
        if (GetThreadTLSData()->unk290 < 5) {
            GetThreadTLSData()->unk290 = GetThreadTLSData()->unk290 + 1;
            apRefObject->RunScript();
            GetThreadTLSData()->unk290 = GetThreadTLSData()->unk290 - 1;
        }
    }
    return true;
}

bool Cmd_SetConsoleOutputFile_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[272];
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    a8[271] = 0;
    if (a8[0] && a8[0] != 48) {
        MenuConsole* v10 = MenuConsole::Instance(true);
        v10->SetOutputFilename(a8);
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("SetConsoleOutputFilename >> '%s'", a8);
        }
    }
    else {
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("SetConsoleOutputFilename >> OUTPUT_DISABLED");
        }
        MenuConsole* v9 = MenuConsole::Instance(true);
        v9->SetOutputFilename(nullptr);
    }
    return true;
}

bool Cmd_TestRaceSexTerimnal(
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

bool Cmd_SetPlayerTeammate_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if ((apRefObject) && (apRefObject->IsActor())) {
        static_cast<Actor*>(apRefObject)->Actor::SetPlayerTeammate(a8 != 0);
    }

    return true;
}

bool Cmd_OpenTeammateContainer(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a8;
    if ((apRefObject) && (apRefObject->IsActor())) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        a8 = 0;
        Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        );
        if (pActor->IsPlayerTeammate() || a8) {
            Interface::QueueMenuCreate(
                Interface::OPEN_CONTAINER, apRefObject, nullptr, nullptr, Interface::CMM_TEAMMATE, false
            );
        }
    }

    return true;
}

bool Cmd_ClearFactionPlayerEnemyFlag(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    a8->SetPCAnEnemy(false);
    return true;
}

bool Cmd_ResetQuest(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8) {
        a8->ResetQuest();
    }
    return true;
}

// The quest's script delay, and its change flag CHANGE_QUEST_SCRIPT_DELAY (4) for the save game.
bool Cmd_SetQuestDelay(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* pquest = nullptr;
    float fdelay = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pquest,
            &fdelay
        )) {
        return false;
    }
    if (pquest) {
        pquest->SetScriptProcessingDelay(fdelay);
        pquest->AddChange(4);
    }
    return true;
}

bool Cmd_ClearDestruction(
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

bool Cmd_FireWeapon(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* v12;
    v12 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &v12
        )) {
        return false;
    }
    if (apRefObject) {
        if (v12 && v12->GetFormType() == 40) {
            TESObjectWEAP* apWeapon = static_cast<TESObjectWEAP*>(v12);
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                Instance->QueueWeaponFire(apWeapon, apRefObject);
            }
            else {
                apWeapon->Fire(apRefObject);
            }
        }
        else {
            const char* v10 = apScript->GetFormEditorID();
            Warning("SCRIPTS: FireWeapon in script '%s' called with non-weapon parameter.", v10);
        }
    }
    return true;
}

bool Cmd_SetWeaponHealthPerc(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float a8;
    a8 = 1.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if ((apRefObject) && (apRefObject->IsActor())) {
        Actor* pactor = static_cast<Actor*>(apRefObject);
        if (pactor->GetCurrentAIProcess()) {
            BaseProcess* pProc = pactor->GetCurrentAIProcess();
            ItemChange* v19 = pProc->GetCurrentWeapon();
            if (v19) {
                a8 = a8 / 100.0;
                bool HasModEffectActive_0 = v19->HasModEffectActive(10);
                float FormHealth = static_cast<float>(
                    static_cast<TESObjectWEAP*>(v19->pContainerObj)->TESObjectWEAP::GetFormHealth(HasModEffectActive_0)
                );
                float afA = FormHealth * a8;
                float v15 = afA - v19->GetItemHealth(false);
                if (!(v15 > 0.0)) {
                    pactor->DamageEquipment(v19, -v15, true);
                }
                else {
                    ExtraDataList* m_item = v19->pExtraObjectList->m_item;
                    InventoryChanges* ContainerChanges = pactor->m_Extra.GetContainerChanges();
                    float v13 = FormHealth - 1.0;
                    float arg0 = BSMIN(afA, v13);
                    v19->SetItemHealth(arg0, ContainerChanges, m_item, true);
                }
            }
        }
    }

    return true;
}

bool Cmd_ModWeaponHealthPerc(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float a8;
    a8 = 1.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if ((apRefObject) && (apRefObject->IsActor())) {
        Actor* pactor = static_cast<Actor*>(apRefObject);
        if (pactor->GetCurrentAIProcess()) {
            BaseProcess* pProc = pactor->GetCurrentAIProcess();
            ItemChange* v17 = pProc->GetCurrentWeapon();
            if (v17) {
                double ItemHealth = v17->GetItemHealth(true);
                a8 = ItemHealth + a8;
                a8 = a8 / 100.0;
                if (a8 < 0.0) {
                    a8 = 0.0f;
                }
                float FormHealth = static_cast<float>(TESHealthForm::GetFormHealth(v17->pContainerObj));
                float v16 = FormHealth * a8;
                float v14 = v16 - v17->GetItemHealth(false);
                if (!(v14 > 0.0)) {
                    pactor->DamageEquipment(v17, -v14, true);
                }
                else {
                    ExtraDataList* m_item = v17->pExtraObjectList->m_item;
                    InventoryChanges* ContainerChanges = pactor->m_Extra.GetContainerChanges();
                    v17->SetItemHealth(v16, ContainerChanges, m_item, true);
                }
            }
        }
    }

    return true;
}

bool Cmd_SetRumble(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float a8;
    float v10;
    float v12;
    a8 = 0.0f;
    v10 = 0.0f;
    v12 = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v10,
            &v12
        )) {
        return false;
    }
    Controls* Controls_ = appMain->GetControls();
    if (Controls_) {
        Controls_->Rumble(a8, v10, static_cast<int32_t>(v12 * 1000.0), 0, CONSTANT, false, true);
    }
    return true;
}

bool Cmd_SetNoActivationSound(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESSound* a1;
    a1 = nullptr;
    if ((Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a1
        )) &&
        (a1)) {
        TESSound* v15 = a1;
        const char* Filename = a1->GetFilename();
        BSAudio* v8 = &BSAudio::QInstance();
        v8->PrecacheSound(Filename, 0x20000121u, v15);
        TESSound* v16 = a1;
        const char* v13 = a1->GetFilename();
        BSAudio* v9 = &BSAudio::QInstance();
        v9->PrecacheSound(v13, 0x20000121u, v16);
        TESSound* v17 = a1;
        const char* v14 = a1->GetFilename();
        BSAudio* v10 = &BSAudio::QInstance();
        v10->PrecacheSound(v14, 0x20000121u, v17);
        FalloutAudio::SetNoActivationSound(a1);
    }

    return true;
}

bool Cmd_Abs(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float a8;
    a8 = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    afReturnValue = std::fabs(a8);
    return true;
}

bool Cmd_EnableWeaponMod(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pWeapon,
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

bool Cmd_AddReputation(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a1;
    int32_t a2;
    TESReputation* a8;
    a1 = 0;
    a2 = 0;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &a1, &a2
        )) {
        return false;
    }
    if (a1 > 1) {
        return false;
    }
    a8->AddReputation(static_cast<ReputationType>(a1), a2);
    return true;
}

bool Cmd_AddReputationExact(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a2;
    float v10;
    TESReputation* a8;
    a2 = 0;
    v10 = 0.0f;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &a2,
            &v10
        )) {
        return false;
    }
    if (a2 < 0 || a2 > 1) {
        return false;
    }
    a8->AddReputationExact(static_cast<ReputationType>(a2), v10);
    return true;
}

bool Cmd_RemoveReputation(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a1;
    int32_t a2;
    TESReputation* a8;
    a1 = 0;
    a2 = 0;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &a1, &a2
        )) {
        return false;
    }
    if (a1 > 1) {
        return false;
    }
    a8->RemoveReputation(static_cast<ReputationType>(a1), a2);
    return true;
}

bool Cmd_RemoveReputationExact(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a2;
    float v10;
    TESReputation* a8;
    a2 = 0;
    v10 = 0.0f;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &a2,
            &v10
        )) {
        return false;
    }
    if (a2 < 0 || a2 > 1) {
        return false;
    }
    a8->RemoveReputationExact(static_cast<ReputationType>(a2), v10);
    return true;
}

bool Cmd_SetReputation(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a2;
    float a3;
    TESReputation* a8;
    a2 = 0;
    a3 = 0.0f;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &a2, &a3
        )) {
        return false;
    }
    if (a2 > 1) {
        return false;
    }
    a8->SetReputation(a2, a3);
    return true;
}

bool Cmd_DetonatePlacedExplosives(
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
        ProcessArray* ProcessArray_ = &ReferenceProcessLists.MobProcessArray;
        for (uint32_t uiIndex = ProcessArray_->GetProcessLevelHead(PROCESS_TYPE_HIGH);
             uiIndex < ProcessArray_->GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
            Projectile* Item = static_cast<Projectile*>(ProcessArray_->GetItem(uiIndex));
            if ((!Item || !Item->IsStillLoading()) && Item->IsProjectile()) {
                TESActorBase* Owner;
                if ((Item->GetShooter() == apRefObject ||
                     (Owner = static_cast<TESActorBase*>(Item->GetOwner()),
                      Owner == apRefObject->GetTemplateActorBase())) &&
                    (Item->GetProjectileBase()->GetHasExplosionDetonates())) {
                    Item->SetFlag(Projectile::DETONATE, true);
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
    if (GetThreadTLSData()->bSystemOutput) {
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
        Actor* v9 = dynamic_cast<Actor*>(apRefObject);
        afReturnValue = static_cast<double>(v9->GetRefEssential());
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("IsActorRefEssential >> %0.f", afReturnValue);
        }
    }
    else {
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("IsActorRefEssential >> Invalid actor reference ");
        }
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
    int32_t a8;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject->IsActor()) {
        Actor* v9 = dynamic_cast<Actor*>(apRefObject);
        v9->SetActorRefEssential(a8 > 0);
    }
    else {
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("SetActorRefEssential >> Invalid actor reference ");
        }
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
    if (GetThreadTLSData()->bSystemOutput) {
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
    TESChallenge* a8;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    TESChallengeManager::IncrementScriptedChallenge(a8, 1);
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
    TESForm* a4 = nullptr;
    NiTMapItem<uint32_t, TESForm*>* a2 = TESForm::pAllForms->GetFirstPos();
    uint32_t a3 = 0;
    uint32_t v12 = 0;
    afReturnValue = 0.0;
    while (a2) {
        TESForm::pAllForms->GetNext(a2, a3, a4);
        if ((a4) && (a4->IsReference() && (a4->iFormFlags & 1024) != 0)) {
            ++v12;
        }
    }
    afReturnValue = static_cast<double>(v12);
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("PersistentRefCount >> %u", v12);
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
    int32_t a8;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject->IsActor()) {
        Actor* v9 = dynamic_cast<Actor*>(apRefObject);
        v9->SetAlwaysShowSubtitles(a8 > 0);
    }
    else {
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("AlwaysShowActorSubtitles >> Invalid actor reference ");
        }
    }
    return true;
}

bool Cmd_ModWaterShader(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    LARGE_INTEGER v13;
    LARGE_INTEGER Frequency;
    LARGE_INTEGER PerformanceCount;
    char apString1[516];
    float v21;
    v21 = 0.0f;
    if (Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, apString1,
            &v21
        )) {
        WaterShaderProperty* v22 = nullptr;
        if (apRefObject) {
            NiNode* WaterGeometry =
                static_cast<NiNode*>(TES::pInstance->GetWaterSystem()->GetWaterGeometry(apRefObject));
            WaterShaderProperty* Property = static_cast<WaterShaderProperty*>(WaterGeometry->GetProperty(3));
            v22 = Property;
        }
        if ((BSstricmp(apString1, "velocity")) && (BSstricmp(apString1, "direction")) &&
            (BSstricmp(apString1, "amplitude")) && (BSstricmp(apString1, "frequency"))) {
            if (BSstricmp(apString1, "reflectamt")) {
                if (BSstricmp(apString1, "distort")) {
                    if (BSstricmp(apString1, "fresnel")) {
                        if (BSstricmp(apString1, "opacity")) {
                            if (BSstricmp(apString1, "blend")) {
                                if (BSstricmp(apString1, "noise")) {
                                    if (BSstricmp(apString1, "off")) {
                                        if (BSstricmp(apString1, "displaceforce")) {
                                            if (BSstricmp(apString1, "displacevelocity")) {
                                                if (BSstricmp(apString1, "displacefalloff")) {
                                                    if (BSstricmp(apString1, "displacedampener")) {
                                                        if (BSstricmp(apString1, "rainforce")) {
                                                            if (BSstricmp(apString1, "rainvelocity")) {
                                                                if (BSstricmp(apString1, "rainfalloff")) {
                                                                    if (BSstricmp(apString1, "rainsize")) {
                                                                        if (BSstricmp(apString1, "reflect")) {
                                                                            if (BSstricmp(apString1, "refract")) {
                                                                                if (BSstricmp(apString1, "depth")) {
                                                                                    if (BSstricmp(apString1, "fog")) {
                                                                                        if (BSstricmp(
                                                                                                apString1, "lod"
                                                                                            )) {
                                                                                            if (!BSstricmp(
                                                                                                    apString1,
                                                                                                    "autowa"
                                                                                                    "te"
                                                                                                    "r"
                                                                                                )) {
                                                                                                char a[256];
                                                                                                double v17 = 1000.0;
                                                                                                PerformanceCount.u
                                                                                                    .LowPart = 0;
                                                                                                QueryPerformanceFrequency(
                                                                                                    &Frequency
                                                                                                );
                                                                                                v17 =
                                                                                                    static_cast<double>(
                                                                                                        Frequency
                                                                                                            .QuadPart
                                                                                                    ) *
                                                                                                    1e-06;
                                                                                                QueryPerformanceCounter(
                                                                                                    &PerformanceCount
                                                                                                );
                                                                                                TESObjectCELL* CurrentCell =
                                                                                                    TES::pInstance
                                                                                                        ->GetCurrentCell();
                                                                                                BGSAutoWater::
                                                                                                    GeneratePlaceableWaterForCell(
                                                                                                        CurrentCell
                                                                                                    );
                                                                                                QueryPerformanceCounter(
                                                                                                    &v13
                                                                                                );
                                                                                                double microSeconds =
                                                                                                    static_cast<double>(
                                                                                                        v13.QuadPart -
                                                                                                        PerformanceCount
                                                                                                            .QuadPart
                                                                                                    ) /
                                                                                                    v17;
                                                                                                int64_t v11 =
                                                                                                    v13.QuadPart -
                                                                                                    PerformanceCount
                                                                                                        .QuadPart;
                                                                                                TESObjectCELL* v10 =
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
                                                                                                    a, 255,
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
                                                                                                    v10->GetCellName(),
                                                                                                    v11, microSeconds
                                                                                                );
                                                                                                InitializationFlow(
                                                                                                    a, 0
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
                                                                                                        true, true, true
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
                                                                                    else {
                                                                                        if (v21 >= 0.0 &&
                                                                                            v21 <= 100.0) {
                                                                                            if (v22) {
                                                                                                v22->fFogAmount =
                                                                                                    v21 / 100.0;
                                                                                                Interface::PrintLine(
                                                                                                    "set "
                                                                                                    "fog "
                                                                                                    "amount"
                                                                                                    " to "
                                                                                                    "%d",
                                                                                                    static_cast<
                                                                                                        int32_t>(v21)
                                                                                                );
                                                                                                v22->bUpdateConstants =
                                                                                                    false;
                                                                                            }
                                                                                            else {
                                                                                                WaterShader::pFogColor
                                                                                                    .a = v21 / 100.0;
                                                                                                Interface::PrintLine(
                                                                                                    "set "
                                                                                                    "fog "
                                                                                                    "amount"
                                                                                                    " to "
                                                                                                    "%d",
                                                                                                    static_cast<
                                                                                                        int32_t>(v21)
                                                                                                );
                                                                                                g_bIsUseModdedWaterShaders =
                                                                                                    1;
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                                                                                else {
                                                                                    if (v22) {
                                                                                        if (v22->bDepth) {
                                                                                            v22->bDepth = false;
                                                                                            Interface::PrintLine(
                                                                                                "Toggle"
                                                                                                " Depth"
                                                                                                " : "
                                                                                                "OFF"
                                                                                            );
                                                                                        }
                                                                                        else {
                                                                                            v22->bDepth = true;
                                                                                            Interface::PrintLine(
                                                                                                "Toggle"
                                                                                                " Depth"
                                                                                                " : "
                                                                                                "ON"
                                                                                            );
                                                                                        }
                                                                                    }
                                                                                    else {
                                                                                        if (TESWaterSystem::
                                                                                                bUseWaterDepth.Bool()) {
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
                                                                                }
                                                                            }
                                                                            else {
                                                                                if (v22) {
                                                                                    if (v22->bRefractions) {
                                                                                        v22->bRefractions = false;
                                                                                        Interface::PrintLine(
                                                                                            "Toggle "
                                                                                            "Refraction"
                                                                                            "s : OFF"
                                                                                        );
                                                                                    }
                                                                                    else {
                                                                                        v22->bReflections = true;
                                                                                        Interface::PrintLine(
                                                                                            "Toggle "
                                                                                            "Refraction"
                                                                                            "s : ON"
                                                                                        );
                                                                                    }
                                                                                }
                                                                                else {
                                                                                    if (TESWaterSystem::
                                                                                            bUseWaterRefractions
                                                                                                .Bool()) {
                                                                                        TESWaterSystem::
                                                                                            bUseWaterRefractions =
                                                                                                false;
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
                                                                            }
                                                                        }
                                                                        else {
                                                                            if (v22) {
                                                                                if (v22->bReflections) {
                                                                                    v22->bReflections = false;
                                                                                    Interface::PrintLine(
                                                                                        "Toggle "
                                                                                        "Reflections : "
                                                                                        "OFF"
                                                                                    );
                                                                                }
                                                                                else {
                                                                                    v22->bReflections = true;
                                                                                    Interface::PrintLine(
                                                                                        "Toggle "
                                                                                        "Reflections : ON"
                                                                                    );
                                                                                }
                                                                            }
                                                                            else {
                                                                                if (TESWaterSystem::bUseWaterReflections
                                                                                        .Bool()) {
                                                                                    TESWaterSystem::
                                                                                        bUseWaterReflections = false;
                                                                                    Interface::PrintLine(
                                                                                        "Toggle Global "
                                                                                        "Water Reflections "
                                                                                        ": OFF"
                                                                                    );
                                                                                }
                                                                                else {
                                                                                    TESWaterSystem::
                                                                                        bUseWaterReflections = true;
                                                                                    Interface::PrintLine(
                                                                                        "Toggle Global "
                                                                                        "Water Reflections "
                                                                                        ": ON"
                                                                                    );
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                    else {
                                                                        if (v21 >= 0.0 && v21 <= 1.0) {
                                                                            ImageSpaceEffectWaterDisplacement::
                                                                                RainVariables.a = v21;
                                                                            g_bIsUseModdedWaterShaders = 1;
                                                                        }
                                                                    }
                                                                }
                                                                else {
                                                                    if (v21 >= 0.0 && v21 <= 1.0) {
                                                                        ImageSpaceEffectWaterDisplacement::RainVariables
                                                                            .b = v21;
                                                                        g_bIsUseModdedWaterShaders = 1;
                                                                    }
                                                                }
                                                            }
                                                            else {
                                                                if (v21 >= 0.0 && v21 <= 1.0) {
                                                                    ImageSpaceEffectWaterDisplacement::RainVariables.g =
                                                                        v21;
                                                                    g_bIsUseModdedWaterShaders = 1;
                                                                }
                                                            }
                                                        }
                                                        else {
                                                            if (v21 >= 0.0 && v21 <= 1.0) {
                                                                ImageSpaceEffectWaterDisplacement::RainVariables.r =
                                                                    v21;
                                                                g_bIsUseModdedWaterShaders = 1;
                                                            }
                                                        }
                                                    }
                                                    else {
                                                        if (v21 >= 0.0 && v21 <= 20.0) {
                                                            g_fWaterDisplaceDampener = v21;
                                                            g_bIsUseModdedWaterShaders = 1;
                                                        }
                                                    }
                                                }
                                                else {
                                                    if (v21 >= 0.0 && v21 <= 1.0) {
                                                        ImageSpaceEffectWaterDisplacement::WadingVariables.b = v21;
                                                        g_bIsUseModdedWaterShaders = 1;
                                                    }
                                                }
                                            }
                                            else {
                                                if (v21 >= 0.0 && v21 <= 1.0) {
                                                    ImageSpaceEffectWaterDisplacement::WadingVariables.g = v21;
                                                    g_bIsUseModdedWaterShaders = 1;
                                                }
                                            }
                                        }
                                        else {
                                            if (v21 >= 0.0 && v21 <= 1.0) {
                                                ImageSpaceEffectWaterDisplacement::WadingVariables.r = v21;
                                                g_bIsUseModdedWaterShaders = 1;
                                            }
                                        }
                                    }
                                    else {
                                        if (v22) {
                                            v22->bUpdateConstants = true;
                                        }
                                        else {
                                            g_bIsUseModdedWaterShaders = 0;
                                        }
                                    }
                                }
                                else {
                                    if (!(v21 >= 0.0) || !(v21 <= 100.0)) {
                                        Interface::PrintLine("usage : mws noise (0.0 - 100.0)", v21);
                                    }
                                    else {
                                        if (v22) {
                                            v22->fNoiseScale = v21;
                                            Interface::PrintLine("set noise scale to %f", v21);
                                            v22->bUpdateConstants = false;
                                        }
                                        else {
                                            ImageSpaceEffectNoise::fNoiseScale = v21;
                                            Interface::PrintLine("set noise to %f", v21);
                                            g_bIsUseModdedWaterShaders = 1;
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            if (!(v21 >= 0.0) || !(v21 <= 100.0)) {
                                Interface::PrintLine("usage : mws opacity (0.0 - 100.0)", v21);
                            }
                            else {
                                if (v22) {
                                    v22->pVarAmounts.z = v21 / 100.0;
                                    Interface::PrintLine("set water opacity to %d", static_cast<int32_t>(v21));
                                    v22->bUpdateConstants = false;
                                }
                                else {
                                    WaterShader::pVarAmounts.b = v21 / 100.0;
                                    Interface::PrintLine("set water opacity to %d", static_cast<int32_t>(v21));
                                    g_bIsUseModdedWaterShaders = 1;
                                }
                            }
                        }
                    }
                    else {
                        if (!(v21 >= 0.0) || !(v21 <= 1.0)) {
                            Interface::PrintLine("usage : mws fresnel (0.0 - 1.0)", v21);
                        }
                        else {
                            if (v22) {
                                v22->fFresnelAmount = v21;
                                Interface::PrintLine("set fresnel term to %f", v21);
                                v22->bUpdateConstants = false;
                            }
                            else {
                                g_fWaterFresnelTerm = v21;
                                Interface::PrintLine("set fresnel term to %f", v21);
                                g_bIsUseModdedWaterShaders = 1;
                            }
                        }
                    }
                }
                else {
                    if (v22) {
                        v22->pVarAmounts.w = v21;
                        Interface::PrintLine("set water distortion amount to %f", v21);
                        v22->bUpdateConstants = false;
                    }
                    else {
                        WaterShader::pVarAmounts.a = v21;
                        Interface::PrintLine("set water distortion amount to %f", v21);
                        g_bIsUseModdedWaterShaders = 1;
                    }
                }
            }
            else {
                if (!(v21 >= 0.0) || !(v21 <= 1.0)) {
                    Interface::PrintLine("usage : mws reflectamt (0.0 - 1.0)", v21);
                }
                else {
                    if (v22) {
                        v22->pVarAmounts.y = v21;
                        Interface::PrintLine("set water reflectivity amount to %f", v21);
                        v22->bUpdateConstants = false;
                    }
                    else {
                        WaterShader::pVarAmounts.g = v21;
                        Interface::PrintLine("set water reflectivity amount to %f", v21);
                        g_bIsUseModdedWaterShaders = 1;
                    }
                }
            }
        }

        return true;
    }

    Interface::PrintLine("reflectamt, fresnel, opacity, speed, noise, reflect, refract, lod");
    return false;
}

bool Cmd_EssentialDeathReload_Parse(
    const uint16_t asParamCount,
    const SCRIPT_PARAMETER* apParameters,
    SCRIPT_LINE* apScriptLine,
    ScriptCompileData* apCompileData
) {
    return Cmd_NonDefault_Parse(asParamCount, apParameters, apScriptLine, apCompileData, true, false);
}

bool Cmd_ShowMessage_Parse(
    const uint16_t asParamCount,
    const SCRIPT_PARAMETER* apParameters,
    SCRIPT_LINE* apScriptLine,
    ScriptCompileData* apCompileData
) {
    return Cmd_NonDefault_Parse(asParamCount, apParameters, apScriptLine, apCompileData, false, false);
}

bool Cmd_ShowWarning_Parse(
    const uint16_t asParamCount,
    const SCRIPT_PARAMETER* apParameters,
    SCRIPT_LINE* apScriptLine,
    ScriptCompileData* apCompileData
) {
    return Cmd_NonDefault_Parse(asParamCount, apParameters, apScriptLine, apCompileData, false, true);
}

// The compiled line is raw bytecode: its 16-bit and 32-bit values are written into the output byte buffer unaligned.
bool Cmd_NonDefault_Parse(
    const uint16_t asParamCount,
    const SCRIPT_PARAMETER* apParameters,
    SCRIPT_LINE* apScriptLine,
    ScriptCompileData* apCompileData,
    bool a5,
    bool a6
) {
    apScriptLine->uiOffset = 0;
    uint16_t usOne = 1;
    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = usOne;
    apScriptLine->uiOutputSize += 2;
    SCRIPT_WORD apWord;
    BGSMessage* pMessage = nullptr;
    int32_t aCount = 0;
    if (a6) {
        aCount = ScriptCompiler::GetNextWord(
            apCompileData, &apWord, apScriptLine->pLine, apScriptLine->uiOffset, false, false
        );
        if (!aCount) {
            ScriptCompiler::ScriptError(apCompileData, "No message text.");
            return false;
        }
        *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) =
            static_cast<uint16_t>(aCount);
        apScriptLine->uiOutputSize += 2;
        ourMemcpy(&apScriptLine->pOutput[apScriptLine->uiOutputSize], apWord.pText, aCount);
        apScriptLine->uiOutputSize += aCount;
    }
    else {
        SCRIPT_WORD String;
        aCount = ScriptCompiler::GetNextWord(
            apCompileData, &String, apScriptLine->pLine, apScriptLine->uiOffset, false, false
        );
        if (!aCount) {
            ScriptCompiler::ScriptError(
                apCompileData, "Missing parameter %s.\r\nCompiled script not saved!", apParameters->pParamName
            );
            return false;
        }
        if (String.uiRefObjectIndex || String.cVariableType) {
            ScriptCompiler::ScriptError(
                apCompileData, "Parameter %s may not be a variable.\r\nCompiled script not saved!",
                apParameters->pParamName
            );
            return false;
        }
        if (!apCompileData->GetReferencedObject(&String, false, false) || !String.uiRefObjectIndex) {
            ScriptCompiler::ScriptError(
                apCompileData, "Item '%s' not found for parameter %s.\r\nCompiled script not saved!", String.pText,
                apParameters->pParamName
            );
            return false;
        }
        pMessage = dynamic_cast<BGSMessage*>(String.pForm);
        if (!String.uiVariableID && !pMessage) {
            ScriptCompiler::ScriptError(
                apCompileData, "Invalid message '%s' for parameter %s.\r\nCompiled script not saved!", String.pText,
                apParameters->pParamName
            );
            return false;
        }
        char* pOutput = apScriptLine->pOutput;
        pOutput[apScriptLine->uiOutputSize++] = 'r';
        *reinterpret_cast<uint16_t*>(&pOutput[apScriptLine->uiOutputSize]) =
            static_cast<uint16_t>(String.uiRefObjectIndex);
        apScriptLine->uiOutputSize += 2;
    }
    // The message text, scanned for its '%' format specifiers ("%%" is a literal percent sign).
    const char* pText = nullptr;
    if (a6) {
        pText = apWord.pText;
    }
    else {
        const char* pDescription;
        if (pMessage) {
            pDescription = static_cast<TESDescription*>(pMessage)->GetDescription(nullptr, DESC_ID);
        }
        else {
            pDescription = nullptr;
        }
        pText = pDescription;
    }
    int32_t v20 = 0;
    while (pText && *pText) {
        if (*pText == '%') {
            if (pText[1] == '%') {
                ++pText;
            }
            else {
                ++v20;
            }
        }
        ++pText;
    }
    if (v20 >= 10) {
        ScriptCompiler::ScriptError(apCompileData, "Max variables of %d exceeded.", 10);
        return false;
    }
    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = static_cast<uint16_t>(v20);
    apScriptLine->uiOutputSize += 2;
    char* v17 = apScriptLine->pOutput;
    for (int32_t i = 0; i < v20; ++i) {
        SCRIPT_WORD apMemory;
        aCount = ScriptCompiler::GetNextWord(
            apCompileData, &apMemory, apScriptLine->pLine, apScriptLine->uiOffset, true, false
        );
        if (!aCount) {
            ScriptCompiler::ScriptError(
                apCompileData, "Too few variables in MessageBox parameters; expected %d, found %d.", v20, i
            );
            return false;
        }
        if (!apMemory.cVariableType) {
            ScriptCompiler::ScriptError(
                apCompileData, "Unknown variable '%s' in MessageBox parameters.", apMemory.pText
            );
            return false;
        }
        if (apMemory.uiRefObjectIndex) {
            if (apMemory.cVariableType == 'G') {
                v17[apScriptLine->uiOutputSize] = 'G';
            }
            else {
                v17[apScriptLine->uiOutputSize] = 'r';
            }
            *reinterpret_cast<uint16_t*>(&v17[++apScriptLine->uiOutputSize]) =
                static_cast<uint16_t>(apMemory.uiRefObjectIndex);
            apScriptLine->uiOutputSize += 2;
        }
        if (apMemory.cVariableType != 'G') {
            if (!apMemory.uiVariableID) {
                ScriptCompiler::ScriptError(
                    apCompileData, "Unknown compiler error in MessageBoxCompile.  Failed to parse variable '%s'.",
                    apMemory.pText
                );
                return false;
            }
            v17[apScriptLine->uiOutputSize++] = apMemory.cVariableType;
            *reinterpret_cast<uint16_t*>(&v17[apScriptLine->uiOutputSize]) =
                static_cast<uint16_t>(apMemory.uiVariableID);
            apScriptLine->uiOutputSize += 2;
        }
    }
    if (!a6) {
        if (a5) {
            SCRIPT_WORD aButtons[10];
            int32_t v12 = 0;
            do {
                aCount = ScriptCompiler::GetNextWord(
                    apCompileData, &aButtons[v12], apScriptLine->pLine, apScriptLine->uiOffset, false, false
                );
                if (aCount <= 0) {
                    break;
                }
                ++v12;
            }
            while (v12 < 10);
            *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) =
                static_cast<uint16_t>(v12);
            apScriptLine->uiOutputSize += 2;
            for (int32_t j = 0; j < v12; ++j) {
                *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = 1;
                apScriptLine->uiOutputSize += 2;
                aCount = BSstrlen(aButtons[j].pText);
                *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) =
                    static_cast<uint16_t>(aCount);
                apScriptLine->uiOutputSize += 2;
                ourMemcpy(&apScriptLine->pOutput[apScriptLine->uiOutputSize], aButtons[j].pText, aCount);
                apScriptLine->uiOutputSize += aCount;
            }
        }
        else {
            int32_t v8 = 0;
            SCRIPT_WORD apNextWord;
            aCount = ScriptCompiler::GetNextWord(
                apCompileData, &apNextWord, apScriptLine->pLine, apScriptLine->uiOffset, false, false
            );
            if (aCount > 0) {
                if (!IsStringInteger(apNextWord.pText)) {
                    ScriptCompiler::ScriptError(
                        apCompileData, "Message time must be an integer.\r\nCompiled script not saved!"
                    );
                    return false;
                }
                v8 = std::atol(apNextWord.pText);
            }
            *reinterpret_cast<uint32_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = v8;
            apScriptLine->uiOutputSize += 4;
        }
    }
    return true;
}

bool Cmd_EssentialDeathReload_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[516];
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    if (pSaveLoadGameOLD->GetMostRecentSaveGame()) {
        Interface::CreateMessageMenu(
            a8, static_cast<const char*>(nullptr), static_cast<const char*>(nullptr), &PlayerCharacter::ForceReload, 1,
            0, 0.0f, 0.0f, sOk.operator const char*(), nullptr
        );
    }
    else {
        Interface::CreateMessageMenu(
            a8, static_cast<const char*>(nullptr), static_cast<const char*>(nullptr), &PlayerCharacter::ForceRestart, 1,
            0, 0.0f, 0.0f, sOk.operator const char*(), nullptr
        );
    }
    return true;
}

bool Cmd_PrintDeprecatedReturnTrue(
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

// The texture's memory size for the dump. Always returns 0, as the game does.
uint32_t GetTextureDumpSize(NiTexture* apTexture) {
    return 0;
}

bool Cmd_OutputTextureUseMap(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char apFileName[512];
    std::memset(apFileName, 0, sizeof(apFileName));
    BSstrcpy(apFileName, 512, "TextureUseMap.txt");
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, apFileName
        )) {
        BSstrcpy(apFileName, 512, "TextureUseMap.txt");
    }
    BGSTextureUseMap kTexUseMap;
    if (apRefObject && apRefObject->Get3D()) {
        NiNode* v8 = apRefObject->Get3D();
        kTexUseMap.CollectTexUseDataRecursive(v8, true);
    }
    else {
        kTexUseMap.CollectSceneTexUseData(true);
    }
    if (kTexUseMap.WriteToFile(apFileName)) {
        Interface::PrintLine("map written to file '%s'", apFileName);
    }
    else {
        Interface::PrintLine("FAILED to write to '%s'", apFileName);
    }
    BSStringT<char> string;
    uint32_t size = kTexUseMap.GetTotalTextureSize();
    if (size <= 0x100000) {
        if (size <= 1024) {
            string.SPrintF("%d bytes", size);
        }
        else {
            size >>= 10;
            string.SPrintF("%d KB", size);
        }
    }
    else {
        size >>= 20;
        string.SPrintF("%d MB", size);
    }
    string.operator+=(" textures in use.");
    Interface::PrintLine(static_cast<const char*>(string.pString));
    return true;
}

bool Cmd_TestAllCells(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    const char* v9;
    int32_t arg0;
    arg0 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &arg0
        )) {
        return false;
    }
    if (!arg0) {
        arg0 = -1;
    }
    TES::pInstance->TestAllCells(arg0);
    if (TES::pInstance->CellTestRunning()) {
        v9 = "running";
    }
    else {
        v9 = "stopped";
    }
    Interface::PrintLine("TestAllCells %s", v9);
    return true;
}

bool Cmd_RenderTestCell(
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
        TESObjectCELL* ParentCell = pPlayer->GetParentCell();
        TESObjectCELL::RenderTestCell(ParentCell, nullptr, true);
        Interface::PrintLine("RenderTestCell complete. Check warnings file for more info.");
    }
    else {
        Interface::PrintLine("RenderTestCell failed: no cell");
    }
    return true;
}

bool Cmd_RenderTestHere(
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
        TESObjectCELL* ParentCell = pPlayer->GetParentCell();
        TESObjectCELL::RenderTestCell(ParentCell, nullptr, false);
        Interface::PrintLine("RenderTestCell complete. Check warnings file for more info.");
    }
    else {
        Interface::PrintLine("RenderTestCell failed: no cell");
    }
    return true;
}

bool Cmd_ToggleConversations(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint8_t isConversationsStatsEnabled = (uint8_t)TESTopic::GetShowConversationStats();
    TESTopic::SetShowConversationStats(!isConversationsStatsEnabled);
    if (TESTopic::GetShowConversationStats()) {
        Interface::PrintLine("Conversation stats %s", "shown.");
    }
    else {
        Interface::PrintLine("Conversation stats %s", "hidden.");
    }
    return true;
}

bool Cmd_ToggleCollisionGeometry(
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

bool Cmd_ToggleProjectileDebug(
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
    if (GetThreadTLSData()->bSystemOutput) {
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

bool Cmd_SetTargetRefractionFire(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v13;
    float a8;
    if (BSShaderManager::IsRefractionEnabled()) {
        a8 = 0.0f;
        v13 = 0;
        if (!Script::ParseParameters(
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v13
            )) {
            return false;
        }
        if (!dynamic_cast<Actor*>(apRefObject)) {
            float a1_4 = BSMIN(a8, 1.0f);
            a8 = BSMAX(a1_4, 0.0);
            bool v11 = a8 > 0.0;
            NiNode* v12 = apRefObject->Get3D();
            if (v12) {
                BSShaderPPLightingProperty::SetRefractionRecurse(v12, v11, a8, 1, static_cast<float>(v13), 0);
                if (GetThreadTLSData()->bSystemOutput) {
                    Interface::PrintLine(
                        "%s refraction fire has been set to %f, period of %d", apRefObject->GetFullName(), a8, v13
                    );
                }
            }
        }
    }
    return true;
}

bool Cmd_ToggleLiteBrite(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint8_t IsLiteBrite = BSShaderManager::IsLiteBrite();
    BSShaderManager::SetIsLiteBrite(IsLiteBrite == 0);
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v1;
        if (BSShaderManager::IsLiteBrite()) {
            v1 = "On";
        }
        else {
            v1 = "Off";
        }
        Interface::PrintLine("Lite Brite -> %s", v1);
    }
    return true;
}

bool Cmd_SetDebugText(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char String[512];
    int32_t aiValue;
    std::memset(String, 0, sizeof(String));
    aiValue = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, String,
            &aiValue
        )) {
        return false;
    }
    if (String[0]) {
        char FileName[264];
        int32_t v12 = aiValue;
        DebugTextManager* Singleton = DebugTextManager::GetSingleton();
        Singleton->SetScreenDebugPage(String, v12);
        const char* PersonalPath = BSUtilities::GetPersonalPath();
        BSstrcpy(FileName, 260, PersonalPath);
        BSstrcat(FileName, 260, "Custom.ini");
        platform::IniWriteString("Display", "sDebugText", String, FileName);
    }
    else {
        DebugTextManager* v9 = DebugTextManager::GetSingleton();
        v9->OutputDebugPageNames(&Interface::PrintLine);
    }
    return true;
}

bool Cmd_ShowFullScenegraph(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    NiTreeCtrl* v16;
    NiTreeCtrl::SetCallback(&TreeCtrlCallback);
    NiTreeCtrl* apMemory = static_cast<NiTreeCtrl*>(ourMalloc(sizeof(NiTreeCtrl)));
    if (apMemory) {
        SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
        HWND__* FormFlags = appMain->hWnd;
        v16 = ::new (apMemory) NiTreeCtrl(
            appMain->hInstance, FormFlags, WorldSceneGraph, "Fallout World Scene Graph", 0x80000000u, 0x80000000u, 800,
            600
        );
    }
    else {
        v16 = nullptr;
    }
    if (apRefObject) {
        NiNode* v8 = apRefObject->Get3D();
        v16->BuildTree(v8);
    }
    SceneGraph* v9 = Main::MenuRootNode();
    v16->CreateTree(v9, "Fallout Interface Scene Graph");
    SceneGraph* v10 = Main::MenuRootNode3D();
    v16->CreateTree(v10, "Fallout Interface 3D Object Scene Graph");
    if (BSShaderManager::GetShadowSceneNode(1)) {
        ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
        v16->CreateTree(ShadowSceneNode_, "Fallout Rendered Menu 3D Scene Graph");
    }
    if (ScreenSplatter::IsActiveSplatter()) {
        NiNode* Root = ScreenSplatter::GetRoot();
        v16->CreateTree(Root, "Fallout Screen Splatter Scene Graph");
    }
    return true;
}

bool Cmd_SetTerminalMenuPos(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float afScale;
    float afY;
    char v11[4];
    float a8;
    a8 = 0.0f;
    afY = 0.0f;
    afScale = 1.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &afY,
            &afScale, v11
        )) {
        return false;
    }
    SetTerminalMenuPos(a8, afY, afScale);
    return true;
}

bool Cmd_MovePipboyKnob(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float v12;
    uint32_t a8;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v12
        )) {
        return false;
    }
    float afAmount = v12;
    FOPipboyManager::Knobs v10 = static_cast<FOPipboyManager::Knobs>(a8);
    FOPipboyManager* Pipboy = Interface::GetPipboy();
    Pipboy->MoveKnob(v10, afAmount, 0.0005f, true);
    return true;
}

bool Cmd_ToggleTestLight(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float v14;
    float a8;
    a8 = -1.0f;
    v14 = -1.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v14
        )) {
        return false;
    }
    bTestLightOn = !(a8 <= 0.0) || !(v14 <= 0.0) || !bTestLightOn;
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
    NiPoint3 cameraPos = ShadowSceneNode_->GetCameraPos();
    float x0 = v14;
    float fire = a8;
    NiPoint3 v17 = cameraPos;
    int32_t v11 = bTestLightOn;
    ShadowSceneNode* v10 = BSShaderManager::GetShadowSceneNode(0);
    return true;
}

bool Cmd_ToggleCombatAI(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bool v2 = ToggleCombatAI();
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v1;
        if (v2) {
            v1 = "On";
        }
        else {
            v1 = "Off";
        }
        Interface::PrintLine("All Combat AI processing is %s", v1);
    }
    return true;
}

bool ToggleCombatAI() {
    CombatController::bProcessCombat = !CombatController::bProcessCombat;
    return CombatController::bProcessCombat;
}

bool Cmd_ShowHeadTrackTarget(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* v9 = dynamic_cast<Actor*>(apRefObject);
    if (v9) {
        ExtraDataList* Extra = v9->GetExtra();
        TESObjectREFR* HeadTrackTargetExtra = Extra->GetHeadTrackTargetExtra();
        if (GetThreadTLSData()->bSystemOutput) {
            if (HeadTrackTargetExtra) {
                const char* FullName = HeadTrackTargetExtra->GetFullName();
                const char* v4 = v9->GetFullName();
                Interface::PrintLine("%s set to look at %s", v4, FullName);
            }
            else {
                const char* v5 = v9->GetFullName();
                Interface::PrintLine("%s set to not look at anyone", v5);
            }
        }
    }
    return true;
}

bool Cmd_ToggleHighProcess(
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
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v1;
        if (ReferenceProcessLists.bProcessHigh) {
            v1 = "On";
        }
        else {
            v1 = "Off";
        }
        Interface::PrintLine("AI Processing for actors in high is  %s", v1);
    }
    if (!ReferenceProcessLists.bProcessHigh) {
        ReferenceProcessLists.TurnOffMovementinHigh();
    }
    return true;
}

bool Cmd_ToggleLowProcess(
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
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v1;
        if (ReferenceProcessLists.bProcessLow) {
            v1 = "On";
        }
        else {
            v1 = "Off";
        }
        Interface::PrintLine("AI Processing for actors in Low is  %s", v1);
    }
    return true;
}

bool Cmd_ToggleMiddleLowProcess(
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
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v1;
        if (ReferenceProcessLists.bProcessMLow) {
            v1 = "On";
        }
        else {
            v1 = "Off";
        }
        Interface::PrintLine("AI Processing for actors in Middle Low is  %s", v1);
    }
    return true;
}

bool Cmd_ToggleMiddleHighProcess(
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
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v1;
        if (ReferenceProcessLists.bProcessMHigh) {
            v1 = "On";
        }
        else {
            v1 = "Off";
        }
        Interface::PrintLine("AI Processing for actors in Middle High is  %s", v1);
    }
    return true;
}

bool Cmd_SpeakSound(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v16;
    char a8[516];
    Actor* v19;
    char Dst[512];
    int32_t v22;
    char* Src = nullptr;
    BSSoundHandle th_s;
    BSStringT<char> v15;
    v16 = 0;
    v22 = 50;
    if (Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8, &v16,
            &v22
        ) &&
        ((Src = a8, (BSstrcpy(Dst, 512, a8), Src)) &&
         (apRefObject &&
          ((v19 = dynamic_cast<Actor*>(apRefObject)) != nullptr &&
           (v19->pCurrentProcess && v19->GetCurrentProcessType() == PROCESS_TYPE_HIGH))))) {
        if (v16 < 0) {
            v16 = 0;
        }
        if (v16 >= 8) {
            v16 = 0;
        }
        if (v22 < 0) {
            v22 = 0;
        }
        if (v22 > 100) {
            v22 = 100;
        }
        v15.SPrintF("%s", Src);
        uint32_t v11 = v22;
        DIALOGUE_EMOTION v10 = static_cast<DIALOGUE_EMOTION>(v16);
        char* v9 = v15.pString;
        float v20 =
            v19->SpeakSoundFunction(v9, th_s, v10, v11, 0, nullptr, nullptr, nullptr, false, false, false, true, true);
        BaseProcess* CurrentAIProcess = v19->pCurrentProcess;
        CurrentAIProcess->SetGreetingFlag(true);
        BaseProcess* v12 = v19->pCurrentProcess;
        v12->SetSoundDelay(v20);
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("The NPC will speak the sound now.");
        }
        return true;
    }

    return false;
}

bool Cmd_Help(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint8_t v18[4];
    uint32_t i;
    char a8[516];
    a8[0] = 0;
    Script::ParseParameters(
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8, v18
    );
    Interface::PrintLine("----CONSOLE COMMANDS--------------------");
    for (i = 0; i < 205; ++i) {
        if (!a8[0] || BSstristr(scriptConsole[i].pHelpString, a8) || BSstristr(scriptConsole[i].pFunctionName, a8) ||
            BSstristr(scriptConsole[i].pShortName, a8)) {
            if (scriptConsole[i].pHelpString && BSstrlen(scriptConsole[i].pHelpString)) {
                if (scriptConsole[i].pShortName && BSstrlen(scriptConsole[i].pShortName)) {
                    Interface::PrintLine(
                        "%s (%s) -> %s", scriptConsole[i].pFunctionName, scriptConsole[i].pShortName,
                        scriptConsole[i].pHelpString
                    );
                }
                else {
                    Interface::PrintLine("%s -> %s", scriptConsole[i].pFunctionName, scriptConsole[i].pHelpString);
                }
            }
            else {
                if (scriptConsole[i].pShortName && BSstrlen(scriptConsole[i].pShortName)) {
                    Interface::PrintLine("%s (%s)", scriptConsole[i].pFunctionName, scriptConsole[i].pShortName);
                }
                else {
                    Interface::PrintLine("%s", scriptConsole[i].pFunctionName);
                }
            }
        }
    }
    Interface::PrintLine("----SCRIPT FUNCTIONS--------------------");
    for (i = 0; i < 640; ++i) {
        if (!a8[0] || BSstristr(scriptFunctions[i].pHelpString, a8) ||
            BSstristr(scriptFunctions[i].pFunctionName, a8) || BSstristr(scriptFunctions[i].pShortName, a8)) {
            if (scriptFunctions[i].pHelpString && BSstrlen(scriptFunctions[i].pHelpString)) {
                if (scriptFunctions[i].pShortName && BSstrlen(scriptFunctions[i].pShortName)) {
                    Interface::PrintLine(
                        "%s (%s) -> %s", scriptFunctions[i].pFunctionName, scriptFunctions[i].pShortName,
                        scriptFunctions[i].pHelpString
                    );
                }
                else {
                    Interface::PrintLine("%s -> %s", scriptFunctions[i].pFunctionName, scriptFunctions[i].pHelpString);
                }
            }
            else {
                if (scriptFunctions[i].pShortName && BSstrlen(scriptFunctions[i].pShortName)) {
                    Interface::PrintLine("%s (%s)", scriptFunctions[i].pFunctionName, scriptFunctions[i].pShortName);
                }
                else {
                    Interface::PrintLine("%s", scriptFunctions[i].pFunctionName);
                }
            }
        }
    }
    if (a8[0]) {
        uint32_t uiIndex;
        Interface::PrintLine("----GAME SETTINGS--------------------");
        NiTPrimitiveArray<char*> v16(0, 1);
        GameSettingCollection* Collection = SettingT<GameSettingCollection>::GetCollection();
        Collection->GetViewerStrings(&v16);
        for (uiIndex = 0;; ++uiIndex) {
            uint32_t EffectiveSize = v16.m_usESize;
            if (uiIndex >= EffectiveSize) {
                break;
            }
            char* apString1 = v16[uiIndex];
            if (apString1) {
                if ((uiIndex) && (BSstristr(apString1, a8))) {
                    Interface::PrintLine("%s", apString1);
                }

                ourFree(apString1);
            }
        }
        Interface::PrintLine("----INI SETTINGS--------------------");
        NiTPrimitiveArray<char*> v15(0, 1);
        INISettingCollection* v11 = SettingT<INISettingCollection>::GetCollection();
        v11->GetViewerStrings(&v15);
        for (uiIndex = 0;; ++uiIndex) {
            uint32_t v9 = v15.m_usESize;
            if (uiIndex >= v9) {
                break;
            }
            char* v13 = v15[uiIndex];
            if (v13) {
                if ((uiIndex) && (BSstristr(v13, a8))) {
                    Interface::PrintLine("%s", v13);
                }

                ourFree(v13);
            }
        }
    }
    return true;
}

bool Cmd_DisableNavMesh_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[512];
    uint32_t aiID;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    aiID = 0;
    std::sscanf(a8, "%x", &aiID);
    if (aiID) {
        if ((apScript) && (apScript->GetFile(-1))) {
            TESFile* File = apScript->GetFile(-1);
            TESForm::AddCompileIndex(aiID, File);
        }

        uint32_t v11 = aiID;
        NavMeshObstacleManager* Instance = NavMeshObstacleManager::GetInstance();
        Instance->DisableNavMesh(v11);
    }
    return true;
}

bool Cmd_EnableNavmesh_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[512];
    uint32_t aiID;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    aiID = 0;
    std::sscanf(a8, "%x", &aiID);
    if (aiID) {
        if ((apScript) && (apScript->GetFile(-1))) {
            TESFile* File = apScript->GetFile(-1);
            TESForm::AddCompileIndex(aiID, File);
        }

        uint32_t v11 = aiID;
        NavMeshObstacleManager* Instance = NavMeshObstacleManager::GetInstance();
        Instance->EnableNavMesh(v11);
    }
    return true;
}

bool Cmd_CenterOnCell_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[516];
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    Interface::CloseConsole();
    pPlayer->CenterOnCell(a8, nullptr);
    return true;
}

bool Cmd_CenterOnExterior(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t aiY;
    int32_t a8;
    a8 = 0;
    aiY = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &aiY
        )) {
        return false;
    }
    TESWorldSpace* apWorldSpace = TES::pInstance->pWorldSpace;
    if (!apWorldSpace) {
        apWorldSpace = static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item);
    }
    if (!apWorldSpace) {
        return false;
    }
    if (pExteriorCellLoader) {
        pExteriorCellLoader->CancelAllCellLoads();
    }
    TESObjectCELL* apCellIn = apWorldSpace->GetCellFromCellCoord(a8, aiY);
    if (!apCellIn) {
        apCellIn = apWorldSpace->LoadCell(a8, aiY);
    }
    if (!apCellIn) {
        apCellIn = pDataHandler->NewCell(nullptr, a8, aiY, apWorldSpace);
    }
    if (apCellIn) {
        Interface::CloseConsole();
        pPlayer->CenterOnCell(nullptr, apCellIn);
    }
    pPlayer->bForceQuestTargetUpdate = true;
    return true;
}

bool Cmd_CenterOnWorld(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t aiY;
    TESWorldSpace* a8;
    int32_t aiX;
    a8 = nullptr;
    aiX = 0;
    aiY = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &aiX,
            &aiY
        )) {
        return false;
    }
    if (!a8) {
        return false;
    }
    if (pExteriorCellLoader) {
        pExteriorCellLoader->CancelAllCellLoads();
    }
    TESObjectCELL* apCellIn = a8->GetCellFromCellCoord(aiX, aiY);
    if (!apCellIn) {
        apCellIn = a8->LoadCell(aiX, aiY);
    }
    if (!apCellIn) {
        apCellIn = pDataHandler->NewCell(nullptr, aiX, aiY, a8);
    }
    if (apCellIn) {
        Interface::CloseConsole();
        pPlayer->CenterOnCell(nullptr, apCellIn);
    }
    pPlayer->bForceQuestTargetUpdate = true;
    return true;
}

bool Cmd_SetSize(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float a8;
    a8 = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    MobileObject* v10 = dynamic_cast<Actor*>(apRefObject);
    if (v10) {
        bhkCharacterController* CharController = v10->GetCharController();
        if (CharController) {
            CharController->SetSize(a8);
        }
    }
    return true;
}

bool Cmd_KillActor_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* killer;
    CAUSE_OF_DEATH a2;
    BGSBodyPart::LIMB_ENUM a3;
    killer = nullptr;
    a3 = BGSBodyPart::LIMB_NONE;
    a2 = CAUSE_OF_DEATH_NONE;
    Script::ParseParameters(
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &killer, &a3, &a2
    );
    PlayerCharacter* a1 = static_cast<PlayerCharacter*>(dynamic_cast<Actor*>(apRefObject));
    if (a1) {
        if (a1->IsInvulnerable()) {
            return true;
        }
        if (a1 == pPlayer && a1->IsChild(false)) {
            a3 = BGSBodyPart::LIMB_NONE;
        }
        bool ShouldQueue3DTask = AILinearTaskThreadManager::ShouldQueue3DTask();
        if (killer == pPlayer) {
            int32_t X = a1->GetLevel();
            bool v9 = a1->IsCreature();
            float afXP = static_cast<float>(
                ExperiencePoints::GetExperiencePoints(static_cast<ExperiencePoints::Activity>(!v9), X)
            );
            GamePlayFormulas::DifficultyLevel GameDifficulty = pPlayer->GetGameDifficulty();
            float v17 = GamePlayFormulas::AdjustExperiencePoints(afXP, GameDifficulty);
            double v11 = std::floorf(v17);
            pPlayer->RewardExperience(static_cast<int32_t>(v11));
        }
        if (a1->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
            a1->SetLifeState(ACTOR_LIFE_STATE_ALIVE);
        }
        if (ShouldQueue3DTask) {
            DISMEMBER_DATA* apDismemberData = nullptr;
            if (a3 != BGSBodyPart::LIMB_NONE) {
                apDismemberData = static_cast<DISMEMBER_DATA*>(ourMalloc(sizeof(DISMEMBER_DATA)));
                apDismemberData->iCause = a2;
                apDismemberData->iLimb = a3;
                apDismemberData->pKiller = killer;
            }
            float* v14 = static_cast<float*>(ourMalloc(sizeof(float)));
            *v14 = 0.0f;
            a1->pMyKiller = killer;
            TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
            Instance->QueueActorKill(a1, v14, apDismemberData);
        }
        else {
            a1->Kill(killer, 0.0f);
            if (a3 != BGSBodyPart::LIMB_NONE) {
                a1->ScriptDismember(a2, a3, killer);
            }
        }
    }
    return true;
}

bool Cmd_KillAllActors(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* killer;
    killer = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &killer
        )) {
        return false;
    }
    for (uint32_t uiIndex = 0;; ++uiIndex) {
        ProcessArray* ActorPackageThatIsRunning = ReferenceProcessLists.GetProcessArray();
        if (uiIndex >= ActorPackageThatIsRunning->GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
            break;
        }
        ProcessArray* v10 = ReferenceProcessLists.GetProcessArray();
        Actor* v11 = static_cast<Actor*>(v10->GetItem(uiIndex));
        if ((v11) && (v11->IsActor())) {
            v11->Kill(killer, 0.0);
        }
    }
    return true;
}

bool Cmd_ResurrectActor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a8;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (pActor) {
        ReferenceProcessLists.kUpdateLock.Lock(nullptr);
        bool bAttach3D = pActor->TESObjectREFR::Get3D() != nullptr;
        pActor->Resurrect(true, bAttach3D, a8 == 1);
        ReferenceProcessLists.kUpdateLock.Unlock();
    }
    return true;
}

bool GetFatiguePercentageFunction(
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

bool Cmd_GetHealthPercentage_Execute(
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

bool Cmd_GetBaseActorValue_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ActorValue::Index a8;
    a8 = ActorValue::AGGRESSION;
    if (Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return Script::GetBaseActorValue_Eval(static_cast<Actor*>(apRefObject), a8, 0, &afReturnValue);
    }

    return false;
}

bool Cmd_GetCauseOfDeath_Eval(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return (int8_t)Script::GetCauseofDeathConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Cmd_FriendDispositionBoost(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if ((apRefObject) && (apRefObject->IsActor())) {
        int32_t v5 = static_cast<Actor*>(apRefObject)->GetDisposition(pPlayer, nullptr);
        if (v5 < iDialogueDispositionFriendValue.Int()) {
            float v4 = static_cast<float>(iDialogueDispositionFriendValue.Int() - v5);
            static_cast<Actor*>(apRefObject)->ModifyDispositionTowardActor(pPlayer, v4);
        }
    }

    return true;
}

bool Cmd_GetGameSetting(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[512];
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    GameSettingCollection* Collection = SettingT<GameSettingCollection>::GetCollection();
    Setting* Ptr = Collection->GetPtr(a8);
    if (Ptr) {
        int32_t type = Ptr->GetType();
        switch (type) {
            case 3: {
                afReturnValue = static_cast<double>(Ptr->Int());
                if (GetThreadTLSData()->bSystemOutput) {
                    int32_t v12 = Ptr->Int();
                    Interface::PrintLine("GameSetting %s >> %i", Ptr->Key(), v12);
                }
                break;
            }
            case 5: {
                afReturnValue = Ptr->Float();
                if (GetThreadTLSData()->bSystemOutput) {
                    float* v10 = &Ptr->Float();
                    Interface::PrintLine("GameSetting %s >> %.2f", Ptr->Key(), *v10);
                }
                break;
            }
            case 6: {
                if (GetThreadTLSData()->bSystemOutput) {
                    const char* v11 = Ptr->String();
                    Interface::PrintLine("GameSetting %s >> '%s'", Ptr->Key(), v11);
                }
                break;
            }
            default: {
                if (GetThreadTLSData()->bSystemOutput) {
                    Interface::PrintLine("GameSetting %s >> UNKNOWN TYPE", Ptr->Key());
                }
                break;
            }
        }
    }
    else {
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("GameSetting %s >> NOT FOUND", a8, Ptr->String());
        }
    }
    return true;
}

bool Cmd_SetGameSetting(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[512];
    char String[512];
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8, String
        )) {
        return false;
    }
    GameSettingCollection* Collection = SettingT<GameSettingCollection>::GetCollection();
    SettingT<GameSettingCollection>* Ptr = static_cast<SettingT<GameSettingCollection>*>(Collection->GetPtr(a8));
    if (Ptr) {
        int32_t Type = Ptr->GetType();
        switch (Type) {
            case 3: {
                int32_t afValue = std::atoi(String);
                Ptr->operator=(afValue);
                if (GetThreadTLSData()->bSystemOutput) {
                    int32_t afValuea = Ptr->Int();
                    Interface::PrintLine("GameSetting %s >> %i", Ptr->Key(), afValuea);
                }
                break;
            }
            case 5: {
                float v14 = std::atof(String);
                Ptr->operator=(v14);
                if (GetThreadTLSData()->bSystemOutput) {
                    float* v10 = &Ptr->Float();
                    Interface::PrintLine("GameSetting %s >> %.2f", Ptr->Key(), *v10);
                }
                break;
            }
            case 6: {
                Ptr->operator=(static_cast<const char*>(String));
                if (GetThreadTLSData()->bSystemOutput) {
                    const char* v11 = Ptr->String();
                    Interface::PrintLine("GameSetting %s >> '%s'", Ptr->Key(), v11);
                }
                break;
            }
            default: {
                Interface::PrintLine("GameSetting %s >> UNKNOWN TYPE", a8);
                break;
            }
        }
    }
    else {
        Interface::PrintLine("GameSetting %s >> NOT FOUND", a8);
    }
    return true;
}

bool Cmd_GetIniSetting_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[512];
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    INISettingCollection* Collection = SettingT<INISettingCollection>::GetCollection();
    Setting* Ptr = Collection->GetPtr(a8);
    if (!Ptr) {
        INIPrefSettingCollection* v10 = SettingT<INIPrefSettingCollection>::GetCollection();
        Ptr = v10->GetPtr(a8);
    }
    if (Ptr) {
        switch (Ptr->GetType()) {
            case 0: {
                Interface::PrintLine("INISetting %s >> %i", a8, Ptr->Bool());
                break;
            }
            case 3: {
                Interface::PrintLine("INISetting %s >> %i", a8, Ptr->Int());
                break;
            }
            case 5: {
                Interface::PrintLine("INISetting %s >> %.2f", a8, Ptr->Float());
                break;
            }
            case 6: {
                Interface::PrintLine("INISetting %s >> '%s'", a8, Ptr->Int());
                break;
            }
            default: {
                Interface::PrintLine("INISetting %s >> UNKNOWN TYPE", a8);
                break;
            }
        }
    }
    else {
        Interface::PrintLine("INISetting %s >> NOT FOUND", a8);
    }
    return true;
}

bool Cmd_SetINISetting(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[516];
    char String[516];
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8, String
        )) {
        return false;
    }
    INISettingCollection* Collection = SettingT<INISettingCollection>::GetCollection();
    SettingT<GameSettingCollection>* Ptr = static_cast<SettingT<GameSettingCollection>*>(Collection->GetPtr(a8));
    if (!Ptr) {
        INIPrefSettingCollection* v10 = SettingT<INIPrefSettingCollection>::GetCollection();
        Ptr = static_cast<SettingT<GameSettingCollection>*>(v10->GetPtr(a8));
    }
    if (Ptr) {
        switch (Ptr->GetType()) {
            case 0: {
                int32_t afValue = std::atoi(String);
                Ptr->operator=(afValue);
                if (GetThreadTLSData()->bSystemOutput) {
                    Interface::PrintLine("INISetting %s >> %i", a8, Ptr->Bool());
                }
                break;
            }
            case 3: {
                int32_t afValuea = std::atoi(String);
                Ptr->operator=(afValuea);
                if (GetThreadTLSData()->bSystemOutput) {
                    Interface::PrintLine("INISetting %s >> %i", a8, Ptr->Int());
                }
                break;
            }
            case 5: {
                float v13 = std::atof(String);
                Ptr->operator=(v13);
                if (GetThreadTLSData()->bSystemOutput) {
                    Interface::PrintLine("INISetting %s >> %.2f", a8, Ptr->Float());
                }
                break;
            }
            case 6: {
                Ptr->operator=(static_cast<const char*>(String));
                if (GetThreadTLSData()->bSystemOutput) {
                    Interface::PrintLine("INISetting %s >> '%s'", a8, Ptr->Int());
                }
                break;
            }
            default: {
                Interface::PrintLine("INISetting %s >> UNKNOWN TYPE", a8);
                break;
            }
        }
    }
    else {
        Interface::PrintLine("INISetting %s >> NOT FOUND", a8);
    }
    return true;
}

bool Cmd_FadeSFX(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float a8;
    a8 = 1.0f;
    if (Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        uint32_t v10 = 1000 * static_cast<int64_t>(a8);
        BSAudio* v8 = &BSAudio::QInstance();
        v8->MuteType(
            (BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER |
             BSGameSound::SOUND_FLAG_IS_2D_RADIUS | BSGameSound::SOUND_FLAG_BEAM_EMITTER),
            v10, 0x1770u
        );
    }
    return true;
}

bool Cmd_SetLocationSpecificLoadScreensOnly(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8) {
        SetIsLocationSpecificLoadScreensOnly(true);
    }
    else {
        SetIsLocationSpecificLoadScreensOnly(false);
    }
    return true;
}

bool SetIsLocationSpecificLoadScreensOnly(bool a1) {
    LoadingMenu::bLocationSpecificLoadScreensOnly = a1;
    return a1;
}

bool Cmd_ShowQuests(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BSSimpleList<TESQuest*>* ecx0 = pDataHandler->GetQuestList();
    while (ecx0 && ecx0->m_item) {
        TESQuest* v3 = ecx0->m_item;
        ecx0 = ecx0->m_pkNext;
        if (v3) {
            const char* v2;
            if (v3->GetEnabled()) {
                v2 = "On";
            }
            else {
                v2 = "Off";
            }
            uint32_t CurrentStage = v3->GetCurrentStage();
            Interface::PrintLine("%s (%s) -- Stage %d", v3->GetEditorName(), v2, CurrentStage);
        }
    }
    return true;
}

bool Cmd_Show1stPerson(
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
        NiAVObject* v0 = pPlayer->Get3D(false);
        if (v0->GetAppCulled()) {
            pPlayer->Force3rdPerson(false);
            if (GetThreadTLSData()->bSystemOutput) {
                Interface::PrintLine("Normal 1st person mode.");
            }
        }
        else {
            NiAVObject* v1 = pPlayer->Get3D(true);
            if (v1->GetAppCulled()) {
                NiAVObject* v3 = pPlayer->Get3D(true);
                v3->SetAppCulled(false);
                if (GetThreadTLSData()->bSystemOutput) {
                    Interface::PrintLine("Showing 1st and 3rd person models.");
                }
            }
            else {
                NiAVObject* v2 = pPlayer->Get3D(true);
                v2->SetAppCulled(true);
                if (GetThreadTLSData()->bSystemOutput) {
                    Interface::PrintLine("Normal 3rd person mode.");
                }
            }
        }
    }
    else {
        pPlayer->Force3rdPerson(true);
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("Showing 1st person model in 3rd person camera.");
        }
    }
    return true;
}

bool Cmd_LoadGame_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v9;
    char a2[516];
    v9 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a2, &v9
        )) {
        return false;
    }
    pSaveLoadManager->LoadGame(a2, -1, v9 != 0, true);
    return true;
}

bool Cmd_Autosave(
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

bool Cmd_ForceSave(
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

bool Cmd_ToggleCellNode(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectCELL::CELLNODE a8;
    a8 = TESObjectCELL::CN_NONE;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8 != TESObjectCELL::CN_NONE) {
        const char* v11 = nullptr;
        switch (a8) {
            case TESObjectCELL::CN_ACTOR: {
                v11 = "Actor";
                break;
            }
            case TESObjectCELL::CN_MARKER: {
                v11 = "Marker";
                break;
            }
            case TESObjectCELL::CN_LAND: {
                v11 = "Land";
                break;
            }
            case TESObjectCELL::CN_STATIC: {
                v11 = "Static";
                break;
            }
            case TESObjectCELL::CN_DYNAMIC: {
                v11 = "Dynamic";
                break;
            }
            case TESObjectCELL::CN_OCCLUSION_PLANE: {
                v11 = "MultiBound";
                break;
            }
            case TESObjectCELL::CN_PORTAL: {
                v11 = "Water";
                break;
            }
            default: {
                break;
            }
        }
        if (v11) {
            const char* v9;
            // The node index is the same value as a cull bit (CN_x == CCN_x for 0-6) and ShowCellNode's int; the value
            // is passed unchanged to both.
            bool IsNodeCulled = TESObjectCELL::IsNodeCulled(static_cast<TESObjectCELL::CELL_CULL_NODE>(a8));
            TES::pInstance->ShowCellNode(static_cast<int32_t>(a8), IsNodeCulled, false);
            if ((a8 == TESObjectCELL::CN_PORTAL) && (TES::pInstance->GetWaterSystem())) {
                TES::pInstance->GetWaterSystem()->DisplayWater(IsNodeCulled, false, false);
            }

            if (IsNodeCulled) {
                v9 = "DISPLAYED";
            }
            else {
                v9 = "CULLED";
            }
            Interface::PrintLine("Cell %s nodes now %s.", v11, v9);
        }
        else {
            Interface::PrintLine("Invalid node index. (0-6).");
        }
    }
    return true;
}

bool Cmd_DumpTexturePalette(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char Destination[512];
    Destination[0] = 0;
    if (apParameters &&
        !Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, Destination
        )) {
        return false;
    }
    if (!Destination[0]) {
        TESObjectCELL* Interior = TES::pInstance->pInteriorCell;
        if (Interior) {
            BSsprintf(Destination, 512, "%s-INT-%s.txt", "TexDump", Interior->GetFormEditorID());
        }
        else {
            TESWorldSpace* WorldSpace = TES::pInstance->pWorldSpace;
            if (WorldSpace) {
                int32_t ProcessLevel = TES::pInstance->iCurrentGridY;
                BSsprintf(
                    Destination, 512, "%s-%s.%i.%i.txt", "TexDump", WorldSpace->GetFormEditorID(),
                    TES::pInstance->iCurrentGridX, ProcessLevel
                );
            }
            else {
                BSsprintf(Destination, 512, "%s-UNKNOWN.txt", "TexDump");
            }
        }
    }
    if (!Destination[0]) {
        ourStrcpy(Destination, "D:\\TexDump.txt");
    }
    MessageHandler::IncDisableWarningCount(true);
    Warning("TEXTURES: <<< DUMPTEXTUREPALETTE results");
    BSFile apMemory(Destination, NiFile::WRITE_ONLY, 0x4000u, false);
    apMemory.BSFile::Open(false, false);
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
            const char* v32;
            char Str[264];
            char apDest[512];
            char v36 = 0;
            if (NiObject::IsKindOf_0(&NiSourceTexture::ms_RTTI, aspTextures[k])) {
                NiSourceTexture* v34 = static_cast<NiSourceTexture*>(aspTextures[k].m_pObject);
                const NiFixedString* v10 = &v34->GetFilename();
                const char* Src = v10->GetHandleString();
                if (!Src || !*Src) {
                    Src = aspTextures[k]->GetName().GetHandleString();
                }
                if (Src && *Src) {
                    BSstrcpy(Str, 260, Src);
                }
                else {
                    BSstrcpy(Str, 260, "UNNAMED SOURCE TEXTURE");
                }
            }
            else {
                v36 = 1;
                const char* result_1 = aspTextures[k]->GetName().GetHandleString();
                if (result_1 && *result_1) {
                    BSsprintf(Str, 260, "RENDERED_TEXTURE (%s)", result_1);
                }
                else {
                    BSsprintf(Str, 260, "RENDERED_TEXTURE (none)");
                }
            }
            BSstrlwr(Str, 260);
            const char* v37 = "SYSTEM";
            if (!v36) {
                if (std::strstr(Str, "\\armor\\") ||
                    (std::strstr(Str, "\\creatures\\") ||
                     (std::strstr(Str, "\\characters\\") ||
                      (std::strstr(Str, "\\pipboy3000\\") ||
                       (std::strstr(Str, "\\weapons\\") ||
                        (std::strstr(Str, "\\decals\\") || std::strstr(Str, "\\projectiles\\"))))))) {
                    v37 = "ACTOR";
                }
                else {
                    if (std::strstr(Str, "\\fonts\\") || std::strstr(Str, "\\interface\\")) {
                        v37 = "MENUS";
                    }
                    else {
                        if (std::strstr(Str, "\\effects\\") || std::strstr(Str, "\\gore\\")) {
                            v37 = "EFFECTS";
                        }
                        else {
                            if (std::strstr(Str, "\\sky\\")) {
                                v37 = "SKY";
                            }
                            else {
                                if (std::strstr(Str, "\\lod\\")) {
                                    v37 = "LOD";
                                }
                                else {
                                    if (std::strstr(Str, "\\water\\")) {
                                        v37 = "WATER";
                                    }
                                    else {
                                        v37 = "MISCREF";
                                    }
                                }
                            }
                        }
                    }
                }
            }
            uint32_t v40 = GetTextureDumpSize(aspTextures[k]);
            const NiPixelFormat* pFormat = &aspTextures[k]->m_pkRendererData->m_kPixelFormat;
            _D3DFORMAT aeD3DFmt = NiDX9PixelFormat::DetermineD3DFormat(*pFormat);
            const char* D3D9FormatString = NiDX9SystemDesc::GetD3D9FormatString(aeD3DFmt);
            const char* pcClassName = aspTextures[k]->GetRTTI()->GetName();
            if (aspTextures[k]->QState() == PS_DEGRADED) {
                v32 = "L";
            }
            else {
                v32 = "H";
            }
            NiTexture* v31 = aspTextures[k];
            NiTexture* v30 = aspTextures[k];
            int32_t v27 = aspTextures[k]->GetRefCount() - 1;
            uint32_t v25 = v40 >> 10;
            uint32_t v24 = v31->GetHeight();
            uint32_t v23 = v30->GetWidth();
            BSsprintf(
                apDest, 512, "TEXTURES: %d:\t%s\t%s\t%dx%d\t%d\t%s\trefcount %d\t%s\t%s\r\n", k + 1, v37, Str, v23, v24,
                v25, D3D9FormatString, v27, v32, pcClassName
            );
            Warning(apDest);
            size_t v22 = std::strlen(apDest);
            apMemory.BSFile::WriteF(apDest, v22 + 1);
        }
    }
    apMemory.Close();
    Warning("TEXTURES: >>> DUMPTEXTUREPALETTE results");
    MessageHandler::IncDisableWarningCount(false);
    return true;
}

bool Cmd_GetItemCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetItemCountConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool Cmd_SetShowQuestItems(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    pPlayer->bShowQuestItemsInInventory = a8 > 0;
    return true;
}

bool Cmd_ResetHealth(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if ((apRefObject) && (apRefObject->IsActor())) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        float v5 = pActor->GetActorFloatValue(ActorValue::HEALTH);
        float v4 = pActor->GetPermanentActorFloatValue(ActorValue::HEALTH);
        float afAmount = v4 - v5;
        pActor->RestoreActorValue(ActorValue::HEALTH, afAmount);
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

bool Cmd_GetEquipped(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetEquippedConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool GetInWorldspaceFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_GetInWorldspace(apRefObject, a8, nullptr, afReturnValue);
}

bool Cmd_SetCellPublicFlag(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v9;
    TESObjectCELL* a8;
    a8 = nullptr;
    v9 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v9
        )) {
        return false;
    }
    if (a8) {
        a8->SetPublic(v9 > 0);
    }
    return true;
}

bool GetInSameCellFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetInSameCellConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool GetIsClassFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_GetIsClass(apRefObject, a8, nullptr, afReturnValue);
}

bool Cmd_SetInChargen(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8) {
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
        afReturnValue = (double)pPlayer->GetInChargen();
        if (Script::GetThreadSystemOutput()) {
            Interface::PrintLine("GetChargenValue >> %i", (int32_t)afReturnValue);
        }
    }
    return true;
}

bool GetIsRaceFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetIsRaceConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool GetIsCreatureTypeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_GetIsCreatureType(apRefObject, a8, nullptr, afReturnValue);
}

bool Cmd_SayTo_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iUseSubtitles;
    int32_t iNoTargetLook;
    Actor* pTarget;
    TESForm* pTopic;
    afReturnValue = 0.0;
    pTarget = nullptr;
    pTopic = nullptr;
    iUseSubtitles = 0;
    iNoTargetLook = 0;
    bool bForceSubtitles = false;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pTarget,
            &pTopic, &iUseSubtitles, &iNoTargetLook
        )) {
        return false;
    }
    if (iUseSubtitles > 0) {
        bForceSubtitles = true;
    }
    int32_t aiValue = iSpeakSoundLipDistance.Int();
    iSpeakSoundLipDistance.operator=(0x7FFFFFFF);
    Actor* pSpeaker = nullptr;
    if (apRefObject->IsActor()) {
        pSpeaker = static_cast<Actor*>(apRefObject);
    }
    if (pSpeaker && pTarget) {
        pSpeaker->StopCurrentDialogue();
        if ((pSpeaker->pCurrentProcess) && (!pSpeaker->IsDead(true)) && (pTarget->pCurrentProcess)) {
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
                (pSpeaker->pCurrentProcess->eLevel ||
                 (pSpeaker->GetEssential() && pSpeaker->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN))) {
                DialogueItem* pItem = static_cast<TESTopic*>(pTopic)->TESTopic::CreateDialogueItem(
                    pSpeaker, pTarget, nullptr, nullptr, true
                );
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
                pSpeaker->pCurrentProcess->ProcessGreet(
                    pSpeaker, static_cast<TESTopic*>(pTopic), bForceSubtitles, false, true, true
                );
            }
        }
    }
    else {
        if ((apRefObject) &&
            (apRefObject->Get3D() ||
             (apRefObject->IsMobileObject() &&
              static_cast<MobileObject*>(apRefObject)->GetTalkingActivator()->Get3D()))) {
            apRefObject->SetSayToTopic(static_cast<TESTopic*>(pTopic));
            DialogueItem* pItem = static_cast<TESTopic*>(pTopic)->TESTopic::CreateDialogueItem(
                apRefObject, nullptr, nullptr, nullptr, false
            );
            if (pItem) {
                apRefObject->SetSayToTopicInfo(pItem->pInfo);
            }
            BSSoundHandle kVoice = apRefObject->VoiceSoundFunction(
                static_cast<TESTopic*>(pTopic), pTarget, false, false, false, false, bForceSubtitles
            );
            // The callback's context is untyped: the speaker's form ID is passed in it.
            kVoice.SetCompletionCallback(
                &TESObjectREFR::SayToCallBack, reinterpret_cast<void*>(static_cast<uintptr_t>(apRefObject->iFormID))
            );
        }
    }
    iSpeakSoundLipDistance.operator=(aiValue);
    return true;
}

bool Cmd_ResetAI(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* v4 = dynamic_cast<Actor*>(apRefObject);
    if ((v4) && (((!v4->GetDisabled()) && (!v4->GetDelete())) && (!v4->IsPc()))) {
        v4->EvaluatePackage(false, true);
    }

    return true;
}

bool Cmd_EvaluatePackage_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* v4 = dynamic_cast<Actor*>(apRefObject);
    if ((v4) && (((!v4->GetDisabled()) && (!v4->GetDelete())) && (!v4->IsPc()))) {
        v4->EvaluatePackage(false, false);
    }

    return true;
}

bool Cmd_AddTopic(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESTopic* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    pPlayer->AddTopic(a8, true, false);
    return true;
}

bool Cmd_SetAlert(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* v9 = dynamic_cast<Actor*>(apRefObject);
        if (v9) {
            v9->SetAlert(a8 > 0);
        }
    }
    return true;
}

bool au_re_Cmd_GetIsAlerted(
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

bool GetPlantedExplosiveFunction(
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

bool Cmd_CompleteAllObjectives(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    a8->SetAllObjectivesComplete(1);
    return true;
}

bool Cmd_Look(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v12;
    Actor* a8;
    a8 = nullptr;
    v12 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v12
        )) {
        return false;
    }
    if ((apRefObject) && (a8)) {
        Actor* v11 = dynamic_cast<Actor*>(apRefObject);
        if ((v11) && (v11->pCurrentProcess)) {
            if (v12) {
                NiPoint3 v14 = a8->GetLocationOnReference();
                v11->SetLookAtTarget(v14);
                BaseProcess* CurrentAIProcess = v11->pCurrentProcess;
                CurrentAIProcess->SetPathLookAtTarget(a8);
                a8->SetTargeted(true);
            }
            else {
                BaseProcess* v10 = v11->pCurrentProcess;
                v10->SetScriptHeadTrackTarget(a8);
                (&v11->m_Extra)->SetHeadTrackTargetExtra(a8);
            }
            v11->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        }
    }

    return true;
}

bool Cmd_StopLook_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ExtraDataList* extra;
    BaseProcess* v5;
    BaseProcess* currentAIProcess;
    Actor* v7;
    if (apRefObject) {
        v7 = dynamic_cast<Actor*>(apRefObject);
        if (v7) {
            extra = v7->TESObjectREFR::GetExtra();
            extra->SetHeadTrackTargetExtra(nullptr);
            if (v7->MobileObject::GetCurrentAIProcess()) {
                currentAIProcess = v7->MobileObject::GetCurrentAIProcess();
                currentAIProcess->ClearScriptHeadTrackTarget(true);
                v7->ClearLookAtTarget();
                v5 = v7->MobileObject::GetCurrentAIProcess();
                v5->SetPathLookAtTarget(nullptr);
            }
        }
    }
    return true;
}

bool Cmd_SetSkyParam(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float v9;
    float a8;
    a8 = 0.0f;
    v9 = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v9
        )) {
        return false;
    }
    BSShaderManager::fSunlightDimmer = std::fabs(a8);
    SkyShader::pLUMRamp[0] = std::fabs(v9);
    return true;
}

char Event_OnAdd(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONADD, thisObj, a8, eventList, *afReturnValue);
    return 1;
}

char Event_OnSell(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* a7,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONSELL, thisObj, a8, eventList, *a7);
    return 1;
}

char Event_OnEquip(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONEQUIP, thisObj, a8, eventList, *afReturnValue);
    return 1;
}

char Event_OnUnEquip(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONUNEQUIP, thisObj, a8, eventList, *afReturnValue);
    return 1;
}

char Event_OnDrop(
    SCRIPT_PARAMETER* paramInfo,
    void* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, static_cast<const char*>(scriptData), *opcodeOffsetPtr, thisObj, containingObj, scriptObj,
            eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONDROP, thisObj, a8, eventList, *afReturnValue);
    return 1;
}

char Cmd_OnActivate_Execute(
    SCRIPT_PARAMETER* paramInfo,
    void* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    void* eventList,
    double* a7,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    *a7 = 0.0;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, static_cast<const char*>(scriptData), *opcodeOffsetPtr, thisObj, containingObj, scriptObj,
            static_cast<ScriptLocals*>(eventList), &a8
        )) {
        return 0;
    }
    if (thisObj) {
        if (thisObj->GetAction(OBJECT_ACTION_ACTIVATE)) {
            *a7 = 1.0;
        }
        thisObj->ClearAction(OBJECT_ACTION_USE_DEFAULT);
        thisObj->ClearAction(OBJECT_ACTION_ACTIVATE);
    }
    return 1;
}

char Cmd_OnNPCActivate_Execute(
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
    // GetActorCause is called through the vtable each time rather than cached, as the game does.
    if ((thisObj->GetActorCause()) && (thisObj->GetActorCause()->IsValid())) {
        pActor = thisObj->GetActorCause()->pActor;
    }

    if ((thisObj) && (!pActor)) {
        *afReturnValue = 1.0;
    }

    return 1;
}

char Event_OnFire(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    *afReturnValue = 0.0;
    Script::CheckFlagFunction(SCRIPT_ACTION_ONFIRE, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Event_OnOpen(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    *afReturnValue = 0.0;
    Script::CheckFlagFunction(SCRIPT_ACTION_ONOPEN, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Event_OnClose(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    *afReturnValue = 0.0;
    Script::CheckFlagFunction(SCRIPT_ACTION_ONCLOSE, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Event_OnGrab(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    *afReturnValue = 0.0;
    Script::CheckFlagFunction(SCRIPT_ACTION_ONGRAB, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Event_OnRelease(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    *afReturnValue = 0.0;
    Script::CheckFlagFunction(SCRIPT_ACTION_ONRELEASE, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Event_OnDestructionStageChange(
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
    uint32_t a8;
    *afReturnValue = 0.0;
    a8 = 0x80000000u;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    // 0x200000 is the OnDestructionStageChange action flag. The result is otherwise unused.
    Script::CheckFlagFunction(SCRIPT_ACTION_ONDESTRUCTIONSTAGECHANGE, thisObj, nullptr, eventList, *afReturnValue);
    return 1;
}

char Event_OnHit(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* a7,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONHIT, thisObj, a8, eventList, *a7);
    return 1;
}

char Event_OnHitWith(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* v12;
    v12 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &v12
        )) {
        return 0;
    }
    if (v12 && v12->GetFormType() == FLST_ID) {
        // A form list: each bound object in it is tested until one sets the result.
        BSSimpleList<TESForm*>* pIter = static_cast<BGSListForm*>(v12)->GetFormList();
        do {
            if (!pIter) {
                break;
            }
            if (pIter->IsEmpty()) {
                break;
            }
            TESForm* v10 = pIter->GetItem();
            pIter = pIter->GetNext();
            TESForm* apActionForm = nullptr;
            if (v10->IsBoundObject()) {
                apActionForm = v10;
            }
            Script::CheckFlagFunction(SCRIPT_ACTION_ONHITWITH, thisObj, apActionForm, eventList, *afReturnValue);
        }
        while (0.0 == *afReturnValue);
    }
    else {
        Script::CheckFlagFunction(SCRIPT_ACTION_ONHITWITH, thisObj, v12, eventList, *afReturnValue);
    }
    return 1;
}

char Event_OnDeath(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONDEATH, thisObj, a8, eventList, *afReturnValue);
    return 1;
}

char Event_OnStartCombat(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* a7,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONSTARTCOMBAT, thisObj, a8, eventList, *a7);
    return 1;
}

char Event_OnMurder(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONMURDER, thisObj, a8, eventList, *afReturnValue);
    return 1;
}

char Event_OnCombatEnd(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    Script::CheckFlagFunction(SCRIPT_ACTION_ONCOMBATEND, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

// Ignores ParseParameters' result, as the game does.
char Event_SayToDone(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    Script::ParseParameters(paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8);
    // 0x40000 is the SayToDone action flag.
    Script::CheckFlagFunction(SCRIPT_ACTION_ONSAYTODONE, thisObj, a8, eventList, *afReturnValue);
    return 1;
}

char Event_OnPackageStart(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONPACKAGESTART, thisObj, a8, eventList, *afReturnValue);
    return 1;
}

char Event_OnPackageDone(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONPACKAGEEND, thisObj, a8, eventList, *afReturnValue);
    return 1;
}

char Event_OnPackageChange(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONPACKAGECHANGE, thisObj, a8, eventList, *afReturnValue);
    return 1;
}

char Event_OnLoad(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    Script::CheckFlagFunction(SCRIPT_ACTION_ONLOAD, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Event_OnReset(
    int32_t a1,
    int32_t a2,
    TESObjectREFR* apRefObject,
    int32_t a4,
    int32_t a5,
    ScriptLocals* apScriptLocals,
    double* afReturnValue
) {
    Script::CheckFlagFunction(SCRIPT_ACTION_ONRESET, apRefObject, nullptr, apScriptLocals, *afReturnValue);
    return 1;
}

char Event_OnMagicEffectHit(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONMAGICEFFECTHIT, thisObj, a8, eventList, *afReturnValue);
    return 1;
}

char Event_OnTrigger(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* a7,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONTRIGGER, thisObj, a8, eventList, *a7);
    return 1;
}

char Event_OnTriggerEnter(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* a7,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONTRIGGERENTER, thisObj, a8, eventList, *a7);
    return 1;
}

char Event_OnTriggerLeave(
    SCRIPT_PARAMETER* paramInfo,
    char* scriptData,
    TESObjectREFR* thisObj,
    TESObjectREFR* containingObj,
    Script* scriptObj,
    ScriptLocals* eventList,
    double* afReturnValue,
    uint32_t* opcodeOffsetPtr
) {
    TESForm* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            paramInfo, scriptData, *opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList, &a8
        )) {
        return 0;
    }
    Script::CheckFlagFunction(SCRIPT_ACTION_ONTRIGGERLEAVE, thisObj, a8, eventList, *afReturnValue);
    return 1;
}

bool Cmd_EnablePlayerControls_Execute(
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
    int32_t a8 = 1;
    int32_t v11 = 1;
    int32_t v16 = 1;
    int32_t v10 = 1;
    int32_t v14 = 1;
    int32_t v15 = 1;
    int32_t v9 = 1;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v11,
            &v16, &v10, &v14, &v15, &v9
        )) {
        return false;
    }
    if (a8) {
        uiFlags |= 1u;
    }
    if (v11) {
        uiFlags |= 4u;
    }
    if (v16) {
        uiFlags |= 8u;
    }
    if (v10) {
        uiFlags |= 0x10u;
    }
    if (v14) {
        uiFlags |= 2u;
    }
    if (v15) {
        uiFlags |= 0x20u;
    }
    if (v9) {
        uiFlags |= 0x40u;
    }
    pPlayer->SetControlsDisabled(false, static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags));
    return true;
}

bool Cmd_DisablePlayerControls_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v9;
    int32_t v10;
    int32_t v11;
    uint32_t a8;
    int32_t v14;
    int32_t v15;
    int32_t v16;
    PlayerCharacter::DISABLE_PLAYER_CONTROLS uiFlags = PlayerCharacter::DPC_NONE;
    a8 = 1;
    v11 = 1;
    v16 = 1;
    v10 = 0;
    v14 = 0;
    v15 = 0;
    v9 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v11,
            &v16, &v10, &v14, &v15, &v9
        )) {
        return false;
    }
    if (a8) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_MOVEMENT);
    }
    if (v11) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_PIPBOY);
    }
    if (v16) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_FIGHTING);
    }
    if (v10) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_POV_SWITCH);
    }
    if (v14) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_LOOKING);
    }
    if (v15) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_ROLL_OVER);
    }
    if (v9) {
        uiFlags = static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(uiFlags | PlayerCharacter::DPC_SNEAKING);
    }
    pPlayer->SetControlsDisabled(true, uiFlags);
    return true;
}

bool GetPlayerControlsDisabledFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v9;
    int32_t v10;
    int32_t v11;
    uint32_t uiFlags;
    uint32_t a8;
    int32_t v14;
    int32_t v15;
    int32_t v16;
    uiFlags = 0;
    a8 = 1;
    v11 = 1;
    v16 = 1;
    v10 = 1;
    v14 = 1;
    v15 = 1;
    v9 = 1;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v11,
            &v16, &v10, &v14, &v15, &v9
        )) {
        return false;
    }
    if (a8) {
        uiFlags |= 1u;
    }
    if (v11) {
        uiFlags |= 4u;
    }
    if (v16) {
        uiFlags |= 8u;
    }
    if (v10) {
        uiFlags |= 16u;
    }
    if (v14) {
        uiFlags |= 2u;
    }
    if (v15) {
        uiFlags |= 32u;
    }
    if (v9) {
        uiFlags |= 64u;
    }
    // The flags go in the condition function's first parameter slot, where Cmd_GetPlayerControlsDisabled reads them.
    Cmd_GetPlayerControlsDisabled(
        nullptr, reinterpret_cast<void*>(static_cast<uintptr_t>(uiFlags)), nullptr, afReturnValue
    );
    return true;
}

bool Cmd_PickIdle(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* v18 = dynamic_cast<Actor*>(apRefObject);
    if ((v18) && (v18->GetCurrentAIProcess())) {
        BaseProcess* CurrentAIProcess = v18->GetCurrentAIProcess();
        TESIdleForm* IdleToPlay = pIdleManager->GetIdleToPlay(v18, CurrentAIProcess->GetTarget());
        if (IdleToPlay) {
            Animation* v16 = v18->GetAnimation();
            if (!v16) {
                return true;
            }
            bool v15 = !v16->SpecialIdleDonePlaying();
            if (v16->spAnimIdleQueued.m_pObject && v16->spAnimIdleQueued.m_pObject->pIdleForm == IdleToPlay) {
                return true;
            }
            if ((v15) && (v16->spAnimIdle.m_pObject) &&
                (v16->spAnimIdle.m_pObject->eType != AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE ||
                 (v16->spAnimIdle.m_pObject->GetSequence() &&
                  v16->spAnimIdle.m_pObject->GetSequence()->m_eCycleType == NiControllerSequence::CYCLE_LOOP))) {
                v15 = false;
            }

            if (!v15) {
                if (v18->GetCurrentAIProcess()) {
                    v18->GetCurrentAIProcess()->SetCurrentProcessIdle(IdleToPlay);
                    v18->GetCurrentAIProcess()->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_PICK_IDLE_CALL);
                }
                if (GetThreadTLSData()->bSystemOutput) {
                    const char* v11 = IdleToPlay->GetModel();
                    int32_t CastingType = IdleToPlay->iFormID;
                    Interface::PrintLine(
                        "Picked Idle '%s' (%08X) file: %s", IdleToPlay->GetFormEditorID(), CastingType, v11
                    );
                }
            }
        }
    }

    return true;
}

bool Cmd_PlayIdle(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[512];
    Actor* v18 = dynamic_cast<Actor*>(apRefObject);
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    if ((v18) && (v18->pCurrentProcess)) {
        TESForm* FormByEditorID = TESForm::GetFormByEditorID(a8);
        TESIdleForm* pIdleForm = dynamic_cast<TESIdleForm*>(FormByEditorID);
        if (pIdleForm) {
            Animation* pAnimation = v18->GetAnimation();
            if (!pAnimation) {
                return true;
            }
            bool bPlaying = !pAnimation->SpecialIdleDonePlaying();
            if (pAnimation->spAnimIdleQueued.m_pObject &&
                pAnimation->spAnimIdleQueued.m_pObject->pIdleForm == pIdleForm) {
                return true;
            }
            if ((bPlaying) && (pAnimation->spAnimIdle.m_pObject) &&
                (pAnimation->spAnimIdle.m_pObject->eType != AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE ||
                 (pAnimation->spAnimIdle.m_pObject->GetSequence() &&
                  pAnimation->spAnimIdle.m_pObject->GetSequence()->m_eCycleType == NiControllerSequence::CYCLE_LOOP))) {
                bPlaying = false;
            }

            if (!bPlaying) {
                if (v18->pCurrentProcess) {
                    BaseProcess* CurrentAIProcess = v18->pCurrentProcess;
                    CurrentAIProcess->SetCurrentProcessIdle(pIdleForm);
                    BaseProcess* v11 = v18->pCurrentProcess;
                    v11->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_PICK_IDLE_CALL);
                }
                if (GetThreadTLSData()->bSystemOutput) {
                    const char* v10 = pIdleForm->GetModel();
                    int32_t CastingType = pIdleForm->iFormID;
                    Interface::PrintLine(
                        "PlayIdle found '%s' (%08X) file: %s", pIdleForm->GetFormEditorID(), CastingType, v10
                    );
                }
            }
        }
    }

    return true;
}

bool IsWeaponInListFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_IsWeaponInList(apRefObject, a8, nullptr, afReturnValue);
}

bool IsTorchOutFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return !apRefObject || Cmd_SetArgTo0AndReturnTrue(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Cmd_IsFacingUp_Execute(
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

bool Cmd_IsLeftUp_Execute(
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

bool au_re_Cmd_GetKnockedState(
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

bool IsWeaponSkillTypeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject) {
        return Cmd_IsWeaponSkillType(apRefObject, a8, nullptr, afReturnValue);
    }
    return true;
}

bool Cmd_GetCurrentAIPackage_Execute(
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

bool au_re_Cmd_IsWaiting(
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

bool au_re_Cmd_IsIdlePlaying(
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
        return (int8_t)Script::IsIdlePlayingConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Cmd_ShowQuestTargets_Execute(
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
        TESQuest* ActiveQuest = pPlayer->GetActiveQuest();
        Interface::PrintLine("Active quest: %s", ActiveQuest->GetEditorName());
        // pIter holds the list head and then each next node.
        BSSimpleList<TESQuestTarget*>* pIter = pPlayer->GetCurrentTargetList();
        if (pIter) {
            Interface::PrintLine("%d current targets", pIter->ItemsInList());
        }
        else {
            Interface::PrintLine("No current targets");
        }
        int32_t v25 = 1;
        while (pIter && pIter->GetItem()) {
            TESQuestTarget* v20 = pIter->GetItem();
            pIter = pIter->GetNext();
            BSStringT<char> v21;
            BSStringT<char> apString;
            apString.Set("Same cell/exterior", 0);
            TESObjectREFR* Reference = v20->GetReference(false);
            TESObjectREFR* v18 = v20->GetReference(true);
            if ((v18) && (Reference)) {
                if (v18 == Reference) {
                    int32_t CastingType = Reference->iFormID;
                    v21.SPrintF("%s (%08X)", Reference->GetFormEditorID(), CastingType);
                }
                else {
                    int32_t v13 = Reference->iFormID;
                    const char* v10 = Reference->GetFormEditorID();
                    int32_t v9 = v18->iFormID;
                    v21.SPrintF("%s (%08X) (carrying %s (%08X))", v18->GetFormEditorID(), v9, v10, v13);
                }
            }

            TESObjectREFR* v22 = v20->QTrackingRef();
            if ((v22) && (v22->GetObjectReference()->GetFormType() == DOOR_ID)) {
                int32_t v14 = v22->iFormID;
                apString.SPrintF("%s (%08X)", v22->GetFormEditorID(), v14);
            }

            char* v15 = apString.pString;
            char* v11 = v21.pString;
            Interface::PrintLine("Target %d:  Reference: %s, load door: %s", v25++, v11, v15);
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

bool Cmd_ShowFullQuestLog(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    BSSimpleList<TESQuestStageItem*> kQuestLog;
    TESQuest::GetQuestLog(&kQuestLog, a8, false, false, false);
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

bool Cmd_SetPCFactionMurder(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* a8;
    int32_t v10;
    a8 = nullptr;
    v10 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v10
        )) {
        return false;
    }
    if (v10) {
        a8->SetPCMurder(true);
    }
    else {
        a8->SetPCMurder(false);
    }
    return true;
}

bool Cmd_SetPCEnemyofFaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* a8;
    int32_t v10;
    a8 = nullptr;
    v10 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v10
        )) {
        return false;
    }
    if (v10) {
        a8->SetPCAnEnemy(true);
    }
    else {
        a8->SetPCAnEnemy(false);
    }
    return true;
}

bool Cmd_SetPCFactionAttack(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* a8;
    int32_t v10;
    a8 = nullptr;
    v10 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v10
        )) {
        return false;
    }
    if (v10) {
        a8->SetPCAttack(true);
    }
    else {
        a8->SetPCAttack(false);
    }
    return true;
}

bool Cmd_GetSelf_Execute(
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
    uint32_t apID = 0;
    if (apRefObject) {
        uint8_t FormType;
        if (apRefObject->GetRefPersists() ||
            (FormType = apRefObject->GetObjectReference()->GetFormType(),
             !TESContainer::ContainerCanHoldType(FormType))) {
            apID = apRefObject->iFormID;
            Script::PutNumericIDInDouble(apID, afReturnValue);
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetSelf >> (%08x)", apID);
    }
    return true;
}

bool Cmd_GetCombatTarget_Execute(
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
    Actor* v10 = dynamic_cast<Actor*>(apRefObject);
    uint32_t apID = 0;
    if (v10) {
        Actor* v9 = v10->GetCombatTarget();
        if (v9) {
            apID = v9->iFormID;
            Script::PutNumericIDInDouble(apID, afReturnValue);
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetCombatTarget >> (%08x)", apID);
    }
    return true;
}

bool Cmd_GetPackageTarget_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* currentPackageTarget;
    afReturnValue = 0.0;
    Actor* v9 = dynamic_cast<Actor*>(apRefObject);
    uint32_t apID = 0;
    if (v9) {
        currentPackageTarget = v9->GetCurrentPackageTarget();
        if (currentPackageTarget) {
            apID = currentPackageTarget->TESForm::GetFormID();
            Script::PutNumericIDInDouble(apID, afReturnValue);
        }
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetPackageTarget >> (%08x)", apID);
    }
    return true;
}

bool Cmd_GetContainer(
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
    uint32_t apID = 0;
    if ((apRefObject) && (apContainer)) {
        apID = apContainer->TESForm::GetFormID();
        Script::PutNumericIDInDouble(apID, afReturnValue);
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetContainer >>(%08x)", apID);
    }
    return true;
}

bool Cmd_GetParentRef(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t apID;
    afReturnValue = 0.0;
    apID = 0;
    if ((apRefObject) && (apRefObject->GetEnableStateParent())) {
        TESObjectREFR* EnableStateParent = apRefObject->GetEnableStateParent();
        apID = EnableStateParent->GetFormID();
        Script::PutNumericIDInDouble(apID, afReturnValue);
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetParentRef >> (%08x)", apID);
    }
    return true;
}

bool Cmd_GetLinkedRef(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t apID;
    afReturnValue = 0.0;
    apID = 0;
    if ((apRefObject) && (apRefObject->GetLinkedRef())) {
        TESObjectREFR* LinkedRef = apRefObject->GetLinkedRef();
        apID = LinkedRef->GetFormID();
        Script::PutNumericIDInDouble(apID, afReturnValue);
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetLinkedRef >> (%08x)", apID);
    }
    return true;
}

bool Cmd_GetForceRun(
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
    Actor* v8 = dynamic_cast<Actor*>(apRefObject);
    if ((v8) && (v8->GetForceRun())) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetForceRun >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_SetForceRun(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    Actor* v10 = dynamic_cast<Actor*>(apRefObject);
    if (v10) {
        afReturnValue = 1.0;
        a8 = 0;
        if (!Script::ParseParameters(
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
            )) {
            return false;
        }
        v10->bForceRun = a8 != 0;
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("SetForceRun >> %0.2f", a8);
        }
    }
    return true;
}

bool Cmd_GetForceSneak(
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
    if ((pActor) && (pActor->bForceSneak)) {
        afReturnValue = 1.0;
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetForceSneak >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_SetForceSneak(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    Actor* v10 = dynamic_cast<Actor*>(apRefObject);
    if (v10) {
        afReturnValue = 1.0;
        a8 = 0;
        if (!Script::ParseParameters(
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
            )) {
            return false;
        }
        if (a8) {
            v10->SetForceSneak(true);
        }
        else {
            v10->SetForceSneak(false);
        }
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("SetForceSneak >> %0.2f", a8);
        }
    }
    return true;
}

bool Cmd_AdvLevel_Execute(
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
        uint16_t Level = pPlayer->GetLevel();
        pPlayer->GetActorBase()->SetLevel(Level + 1);
        LevelUpMenu::Create();
    }
    return true;
}

bool Cmd_HasMagicEffect_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::HasMagicEffectConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool IsSpellTargetFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::IsSpellTargetConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool GetSpellUsageNumberFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetSpellUsageNumberConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool au_re_Cmd_GetVATSMode(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return (int8_t)Script::GetVATSModeConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool GetVATSTargetHeightFunction(
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

bool Cmd_SetDefaultOpen(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject) {
        bool bWasOpenByDefault = apRefObject->GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
        if (a8) {
            apRefObject->SetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
        }
        else {
            apRefObject->ClearAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
        }
        bool bOpenByDefault = apRefObject->GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
        apRefObject->m_Extra.RemoveSavedAnimation();
        apRefObject->m_Extra.RemoveLastFinishedSequence();
        if (bWasOpenByDefault != bOpenByDefault) {
            BGSChangeFlags kFlags;
            kFlags.~BGSChangeFlags();
            ::new (&kFlags) BGSChangeFlags(CHANGE_OBJECT_OPEN_DEFAULT_STATE);
            if (pSaveLoadGame->GetChange(apRefObject, kFlags)) {
                apRefObject->RemoveChange(CHANGE_OBJECT_OPEN_DEFAULT_STATE);
            }
            else {
                apRefObject->AddChange(CHANGE_OBJECT_OPEN_DEFAULT_STATE);
            }
        }
        BGSOpenCloseForm::SetOpenState(apRefObject, bOpenByDefault, true);
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("SetDefaultOpen >> %0.2f", a8);
    }
    return true;
}

bool Cmd_ShowRaceMenu_Execute(
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
        if (*reinterpret_cast<const uint8_t*>(&GetThreadTLSData()->bSystemOutput)) {
            Interface::CreateRaceSexMenu(RSM_CONTEXT_NONE);
        }
        else {
            Interface::CreateRaceSexMenu(RSM_CONTEXT_CHARGEN);
        }
    }
    return true;
}

bool Cmd_ShowBarberMenu(
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

bool Cmd_ShowPlasticSurgeonMenu(
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

bool Cmd_ShowNameMenu(
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
        return (int8_t)Script::GetPlayerName(
            apParameters, apCompiledParams, apRefObject, apContainer, apScript, apScriptLocals, afReturnValue, auiOffset
        );
    }

    return true;
}

bool Cmd_ShowBarterMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    a8 = 0;
    Script::ParseParameters(
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
    );
    if ((apRefObject) && (apRefObject != pPlayer)) {
        TESObjectREFR* v11 = nullptr;
        TESObjectREFR* apRef = nullptr;
        if (apRefObject->IsMobileObject()) {
            v11 = apRefObject;
        }
        else {
            TESObjectREFR* CloseSound;
            BGSTalkingActivator* v10 = dynamic_cast<BGSTalkingActivator*>(apRefObject->GetObjectReference());
            if (v10) {
                CloseSound = v10->pTempRef;
            }
            else {
                CloseSound = nullptr;
            }
            apRef = CloseSound;
        }
        if (v11) {
            if (v11->IsActor()) {
                apRef = v11;
            }
            else {
                MobileObject* pMobile = static_cast<MobileObject*>(v11);
                if (pMobile->bTalkingActivator) {
                    // The talking activator's base object is read as a BGSTalkingActivator without a type check, as
                    // the game does.
                    apRef =
                        static_cast<BGSTalkingActivator*>(pMobile->pTalkingActivator->GetObjectReference())->pTempRef;
                }
            }
        }
        if (apRef) {
            ShowBarterMenu_(apRef, a8);
        }
    }

    return true;
}

bool GetRestrainedFunction(
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

bool Cmd_ForceFlee(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectCELL* pCell;
    TESObjectREFR* a2;
    a2 = nullptr;
    pCell = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pCell, &a2
        )) {
        return false;
    }
    if (apRefObject == pPlayer) {
        return true;
    }
    if (apRefObject) {
        Actor* v14 = dynamic_cast<Actor*>(apRefObject);
        if ((v14) && (v14->GetCurrentAIProcess())) {
            CombatController* pCombatController = v14->GetCombatController();
            if (pCombatController) {
                pCombatController->ForceFlee(pCombatController->pTarget, 0.0f);
                BaseProcess* CurrentAIProcess = v14->GetCurrentAIProcess();
                FleePackage* v12 = dynamic_cast<FleePackage*>(CurrentAIProcess->GetPackageThatIsRunning());
                if (v12) {
                    PackageLocation* Location_ = v12->pPackLoc;
                    if (a2) {
                        Location_->SetLocReference(a2);
                        v12->SetEvaluatePoint(false);
                    }
                    else {
                        if (pCell) {
                            Location_->SetLocCell(pCell);
                            v12->SetEvaluatePoint(false);
                        }
                    }
                }
            }
            else {
                v14->InitiateFlee(nullptr, false, false, true, pCell, a2, -1.0f, -1.0f);
            }
        }
    }
    return true;
}

bool Cmd_ForceTakeCover(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* a8;
    // The duration is an integer parameter.
    int32_t v13;
    a8 = nullptr;
    v13 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v13
        )) {
        return false;
    }
    if (apRefObject == pPlayer) {
        return true;
    }
    if (apRefObject) {
        Actor* v11 = dynamic_cast<Actor*>(apRefObject);
        if ((v11) && (v11->GetCurrentAIProcess())) {
            CombatController* v10 = v11->GetCombatController();
            if (v10) {
                v10->ForceTakeCover(a8, static_cast<float>(v13));
            }
        }
    }
    return true;
}

bool au_re_Cmd_GetOffersServicesNow(
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

bool Cmd_ModBarterGold(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    a8 = 0;
    Actor* v10 = dynamic_cast<Actor*>(apRefObject);
    if (v10 &&
        !Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Actor Barter Gold is  %d ", v10->GetBarterGold());
    }
    return true;
}

// pNPC's barter gold is read for the line even when the reference's base is no NPC, as the game does.
bool Cmd_SetBarterGold(
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
                    apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals,
                    &iGold
                )) {
                return false;
            }
            pNPC->SetBarterGold(static_cast<uint16_t>(iGold));
        }
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(" Actor's base barter gold is  %d ", pNPC->GetBarterGold());
        }
    }
    return true;
}

bool au_re_Cmd_GetBarterGold(
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

bool Cmd_SetLevel_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v16;
    int32_t v17;
    int32_t v18;
    int32_t a8;
    a8 = 0;
    v18 = 0;
    v17 = -1;
    v16 = -1;
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        TESActorBase* ActorBase = pActor->GetActorBase();
        if (!Script::ParseParameters(
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
                &v18, &v17, &v16
            )) {
            return false;
        }
        if (pActor->IsPc()) {
            CharacterProgression* CharacterProgressionInfo = pPlayer->GetCharacterProgressionInfo();
            CharacterProgressionInfo->ForceLevel(a8);
        }
        else {
            ActorBase->SetLevel(a8);
            if (v18) {
                ActorBase->SetMatchesPCLevel(true);
            }
            if (v17 != -1) {
                ActorBase->SetCalcLevelMin(v17);
                ActorBase->SetMatchesPCLevel(true);
            }
            if (v16 != -1) {
                ActorBase->SetCalcLevelMax(v16);
                ActorBase->SetMatchesPCLevel(true);
            }
            TESContainer* pContainer = pActor->HasContainer();
            InventoryChanges* pInvChanges = InventoryChanges::GetInventoryChanges(pActor);
            if ((pContainer) && (pInvChanges)) {
                pInvChanges->RemoveAllObjectsWorn(false, nullptr);
                pInvChanges->RemoveDefaultLeveledItems();
                pInvChanges->InitLeveledItems();
                TESNPC* v11 = dynamic_cast<TESNPC*>(ActorBase);
                TESCreature* v10 = dynamic_cast<TESCreature*>(ActorBase);
                if (v11) {
                    v11->InitDefaultWorn(pActor, true, true, false, true);
                }
                else {
                    if (v10) {
                        v10->InitDefaultWorn(pActor, true, true, true);
                    }
                }
            }

            if (pActor->IsNpc()) {
                TESNPC* v9 = dynamic_cast<TESNPC*>(pActor->GetObjectReference());
                if (v9) {
                    v9->InitValues(false);
                }
            }
        }
    }
    return true;
}

bool au_re_Cmd_IsXBox(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return (int8_t)Script::IsXboxConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool au_re_IsPS3(
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

bool au_re_Cmd_IsPC(
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

bool IsActorUsingATorchFunction(
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

bool Cmd_ModPCAttribute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ActorValue::Index a8;
    int32_t v11;
    a8 = ActorValue::AGGRESSION;
    v11 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v11
        )) {
        return false;
    }
    if (ActorValue::IsSPECIAL(a8)) {
        int32_t v9 = pPlayer->GetBaseActorValue(a8);
        pPlayer->SetActorBaseValue(a8, v11 + v9);
    }
    return true;
}

bool Cmd_DamageWeapon(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fDamage
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

bool Cmd_GetWeaponHealth(
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

bool Cmd_SetCellOwnership(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectCELL* a8;
    TESForm* apOwner;
    apOwner = nullptr;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &apOwner
        )) {
        return false;
    }
    if (!apOwner) {
        apOwner = pPlayer->GetObjectReference();
    }
    if (a8) {
        a8->SetCellOwnership(apOwner);
    }
    return true;
}

bool au_re_Cmd_SetArgTo0AndReturnTrue_1(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ::Cmd_SetArgTo0AndReturnTrue(apRefObject, nullptr, nullptr, afReturnValue);
    return true;
}

bool Cmd_SetCellFullName(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pCell,
            &pMessage
        )) {
        return false;
    }
    if ((pCell) && (pMessage)) {
        pCell->TESFullName::SetFullName(pMessage->TESFullName::GetFullName());
        pCell->AddChange(CHANGE_CELL_FULLNAME);
    }

    return true;
}

bool Cmd_SetActorFullName(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pMessage
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
        else {
            if (apRefObject->GetObjectReference()->GetFormType() == TACT_ID) {
                BGSTalkingActivator* pActivator = static_cast<BGSTalkingActivator*>(apRefObject->GetObjectReference());
                pActivator->TESFullName::SetFullName(pMessage->TESFullName::GetFullName());
                pActivator->AddChange(CHANGE_BASE_OBJECT_FULLNAME);
            }
        }
    }
    return true;
}

bool au_re_Cmd_GetIsActorRunning(
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

bool au_re_Cmd_GetFriendHit(
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

bool Cmd_IsInCombat_Execute(
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

bool Cmd_SetPackDuration_Execute(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fDuration
        )) {
        return false;
    }
    if (pActor) {
        if (pActor->GetCurrentAIProcess()) {
            pActor->GetCurrentAIProcess()->SetPackageEndTimerValue(fDuration);
        }
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("Package duration is set to  %.02f ", fDuration);
        }
    }
    return true;
}

bool Cmd_PlayMagicShaderVisuals_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    MagicShaderHitEffect* v9;
    float duration;
    TESEffectShader* TESEffectShader_;
    TESEffectShader_ = nullptr;
    duration = -1.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals,
            &TESEffectShader_, &duration
        )) {
        return false;
    }
    if (!apRefObject) {
        apRefObject = pPlayer;
    }
    TESObjectCELL* cell = apRefObject->GetParentCell();
    if (!TES::pInstance->IsCellLoaded(cell, false)) {
        return true;
    }
    if (!apRefObject->Get3D()) {
        return true;
    }
    AutoMemContext apMemory(
        MEM_CONTEXT_TES_ACTOR, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScriptFunctions.cpp",
        0x409C
    );
    BSTempEffect* apEffect = nullptr;
    MagicShaderHitEffect* v10 =
        static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
    if (v10) {
        v9 = ::new (v10) MagicShaderHitEffect(apRefObject, TESEffectShader_, duration);
    }
    else {
        v9 = nullptr;
    }
    apEffect = v9;
    if (v9->Init()) {
        ReferenceProcessLists.AddTempEffect(apEffect);
        if (GetThreadTLSData()->bSystemOutput) {
            if (apRefObject->GetFullName()) {
                Interface::PrintLine("Shader effect has been applied to %s", apRefObject->GetFullName());
            }
            else {
                Interface::PrintLine("Shader effect has been applied to reference");
            }
        }
    }
    else {
        if (apEffect) {
            delete apEffect;
        }
        if (GetThreadTLSData()->bSystemOutput) {
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

bool Cmd_PlayMagicEffectVisuals(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fTime;
    EffectSetting* pEffectSetting;
    pEffectSetting = nullptr;
    fTime = -1.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals,
            &pEffectSetting, &fTime
        )) {
        return false;
    }
    if (!apRefObject) {
        apRefObject = pPlayer;
    }
    TESObjectCELL* a1 = apRefObject->GetParentCell();
    if (!TES::pInstance->IsCellLoaded(a1, false)) {
        return true;
    }
    if (apRefObject->Get3D()) {
        MagicHitEffect* pEffect = nullptr;
        if ((pEffectSetting) && (pEffectSetting->GetModelLength())) {
            MagicModelHitEffect* v19;
            MagicModelHitEffect* apMemory =
                static_cast<MagicModelHitEffect*>(NiMemObject::operator new(sizeof(MagicModelHitEffect)));
            if (apMemory) {
                float z = fTime;
                const char* yb = pEffectSetting->GetModel();
                v19 = ::new (apMemory) MagicModelHitEffect(apRefObject, yb, z);
            }
            else {
                v19 = nullptr;
            }
            pEffect = v19;
        }

        if (pEffect && pEffect->Init()) {
            ReferenceProcessLists.AddTempEffect(pEffect);
            if (GetThreadTLSData()->bSystemOutput) {
                if (apRefObject->GetFullName()) {
                    const char* FullName = apRefObject->GetFullName();
                    Interface::PrintLine("Visual effect has been applied to %s", FullName);
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
            if (GetThreadTLSData()->bSystemOutput) {
                if (apRefObject->GetFullName()) {
                    const char* v12 = apRefObject->GetFullName();
                    Interface::PrintLine("Visual effect initialization failed for %s", v12);
                }
                else {
                    Interface::PrintLine("Visual effect initialization failed for reference");
                }
            }
        }
        return true;
    }

    NiPoint3 v29 = apRefObject->GetAngleOnReference();
    NiPoint3 v30 = apRefObject->GetLocationOnReference();
    float ya = NiCos(-v29.z);
    float xa = -NiSin(-v29.z);
    NiPoint3 dst(xa, ya, 0.0f);
    NiPoint3 v28 = dst;
    // The effect's model is read without testing the parsed effect for null on this path, as the game does.
    const char* v9 = pEffectSetting->GetModel();
    BSTempEffectParticle* v22 =
        BSTempEffectParticle::Spawn(apRefObject->GetParentCell(), 1.0f, v9, v28, v30, 1.0f, 7u, nullptr);
    v22->PlaySequence("SpecialIdle_HitEffect");
    if (GetThreadTLSData()->bSystemOutput) {
        if (apRefObject->GetFullName()) {
            const char* v10 = apRefObject->GetFullName();
            Interface::PrintLine("Visual effect has been applied to %s", v10);
        }
        else {
            Interface::PrintLine("Visual effect has been applied to reference");
        }
    }
    return true;
}

bool Cmd_StopMagicShaderVisuals(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float v9;
    TESEffectShader* a8;
    a8 = nullptr;
    v9 = -1.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v9
        )) {
        return false;
    }
    if (!apRefObject) {
        apRefObject = pPlayer;
    }
    ReferenceProcessLists.FinishMagicShaderHitEffect(apRefObject, a8);
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
bool Cmd_StopMagicEffectVisuals(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals,
            &pEffectSetting, &fValue
        )) {
        return false;
    }
    if (!apRefObject) {
        apRefObject = pPlayer;
    }
    if ((pEffectSetting) && (pEffectSetting->GetModelLength())) {
        // Calls TESModel's virtual GetModel on the base subobject and passes its result to the callee's char*
        // parameter.
        ReferenceProcessLists.FinishMagicModelHitEffect(apRefObject, const_cast<char*>(pEffectSetting->GetModel()));
    }

    if (GetThreadTLSData()->bSystemOutput) {
        if (apRefObject->GetFullName()) {
            Interface::PrintLine("Visual effect has been removed from %s", apRefObject->GetFullName());
        }
        else {
            Interface::PrintLine("Visual effect has been removed from reference");
        }
    }
    return true;
}

bool Cmd_PreloadMagicEffect(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    EffectSetting* pEffectSetting;
    pEffectSetting = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals,
            &pEffectSetting
        )) {
        return false;
    }
    if ((pEffectSetting) && (pEffectSetting->GetModelLength())) {
        NiPointer<Model> spModel;
        if (!pModelLoader->FindModel(pEffectSetting->GetModel(), spModel)) {
            pModelLoader->QueueModel(
                pEffectSetting->GetModel(), IO_TASK_PRIORITY_VERY_LOW, nullptr, LOD_MULT_NONE, true, false, false
            );
        }
    }

    return true;
}

bool Cmd_SetIgnoreFriendlyHits(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    a8 = -1;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8 != -1) {
        static_cast<Actor*>(apRefObject)->Actor::SetIgnoreFriendlyHits(a8 != 0);
        apRefObject->AddChange(1);
    }
    return true;
}

bool Cmd_GetIgnoreFriendlyHits_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bool ignoreFriendlyHits = Cmd_GetIgnoreFriendlyHits(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::GetThreadSystemOutput()) {
        if (0.0 == afReturnValue) {
            Interface::PrintLine("%s", "Actor counts friendly hits");
        }
        else {
            Interface::PrintLine("%s", "Actor ignores friendly hits");
        }
    }
    return ignoreFriendlyHits;
}

bool Cmd_SetItemValue(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    TESBoundObject* ObjectReference = nullptr;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject) {
        ObjectReference = apRefObject->GetObjectReference();
        if (ObjectReference) {
            TESValueForm* v9 = dynamic_cast<TESValueForm*>(ObjectReference);
            if (v9) {
                v9->SetFormValue(a8);
                if (GetThreadTLSData()->bSystemOutput) {
                    Interface::PrintLine("%s has been set to a VALUE of %i", apRefObject->GetFullName(), a8);
                }
                BarterMenu::ForceBarterUpdate(false);
            }
        }
    }
    return true;
}

bool Cmd_SetActorRefraction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float a8;
    a8 = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (!apRefObject) {
        apRefObject = pPlayer;
    }
    float a1 = BSMIN(a8, 10.0f);
    a8 = BSMAX(a1, 0.0f);
    Actor* v18 = dynamic_cast<Actor*>(apRefObject);
    if ((v18) && (v18->pCurrentProcess)) {
        BaseProcess* CurrentAIProcess = v18->pCurrentProcess;
        CurrentAIProcess->SetScriptRefractionPower(a8);
        if (BSShaderManager::IsRefractionEnabled()) {
            float a2 = BSMAX(static_cast<float>(0.0), 0.0);
            float v16 = BSMIN(1.0f, a2);
            if (static_cast<float>(0.0) > 0.0 && v18 == pPlayer) {
                v18->SetAlpha(1.0f);
                v18->SetRefraction(true, 0.05f);
            }
            else {
                if (!(v16 > 0.0)) {
                    if (!(a8 > 0.0)) {
                        v18->SetRefraction(false, 0.0f);
                        v18->UpdateAlpha();
                    }
                    else {
                        v18->SetRefraction(true, a8);
                    }
                }
                else {
                    v18->SetAlpha(1.0f);
                    float v13 = 1.0 - v16 / 100.0;
                    float fChameleonMaxRefraction_4 = MagicSystem::GetChameleonMaxRefraction();
                    float fChameleonMaxRefraction = MagicSystem::GetChameleonMinRefraction();
                    float v15 = FLerp(fChameleonMaxRefraction, fChameleonMaxRefraction_4, 0.0f, 1.0f, v13);
                    v18->SetRefraction(true, v15);
                }
            }
        }
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("%s refraction has been set to %f", apRefObject->GetFullName(), a8);
        }
    }

    return true;
}

bool Cmd_RunCellTest(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    TES::pInstance->RunCellTest(a8);
    return true;
}

bool Cmd_StartAllQuests(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    for (BSSimpleList<TESQuest*>* ecx0 = pDataHandler->GetQuestList(); ecx0 && !ecx0->IsEmpty();
         ecx0 = ecx0->m_pkNext) {
        TESQuest* v8 = ecx0->m_item;
        v8->SetEnabled(1u);
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("All Quests Enabled.", afReturnValue);
    }
    return true;
}

bool Cmd_SetsNewGammaRamp_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float a8;
    a8 = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    Renderer::SetGamma(a8);
    return true;
}

bool Cmd_SetMaxAniso(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v10;
    float a8;
    a8 = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    int32_t v9 = static_cast<int32_t>(a8);
    if (v9 >= 2) {
        if (v9 >= 4) {
            if (v9 >= 8) {
                if (v9 >= 12) {
                    if (v9 >= 16) {
                        v10 = 16;
                    }
                    else {
                        v10 = 12;
                    }
                }
                else {
                    v10 = 8;
                }
            }
            else {
                v10 = 4;
            }
        }
        else {
            v10 = 2;
        }
    }
    else {
        v10 = 1;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Maxaniso set to %d", v10);
    }
    NiRenderer::iMaxAnisotropy = v10;
    return true;
}

bool Cmd_SetTalkingActivatorActor(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* a8;
    a8 = nullptr;
    Script::ParseParameters(
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
    );
    if ((apRefObject) && (apRefObject->GetObjectReference()->GetFormType() == TACT_ID)) {
        BGSTalkingActivator* ObjectReference = static_cast<BGSTalkingActivator*>(apRefObject->GetObjectReference());
        ObjectReference->SetTempRef(a8);
    }

    return true;
}

bool Cmd_AddPerk_Execute(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pPerk, &iAlt
        )) {
        return false;
    }
    bool bAlt = iAlt != 0;
    if ((pPerk) && (apRefObject->IsActor())) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        uint8_t ucRank = pActor->GetPerkRank(pPerk, bAlt);
        if (pPerk->GetNumRanks() <= ucRank) {
            afReturnValue = static_cast<double>(ucRank);
        }
        else {
            pActor->AddPerk(pPerk, ucRank + 1, bAlt);
            if (GetThreadTLSData()->bSystemOutput) {
                const char* FullName = pActor->GetFullName();
                Interface::PrintLine(
                    "Added Perk %s to %s with rank %i", pPerk->TESFullName::GetFullName(), FullName, ucRank + 1
                );
            }
            afReturnValue = static_cast<double>(ucRank + 1);
        }
    }

    return true;
}

bool Cmd_RemovePerk(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSPerk* pperk;
    int32_t v11;
    pperk = nullptr;
    v11 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pperk, &v11
        )) {
        return false;
    }
    bool v10 = v11 != 0;
    if ((pperk) && (apRefObject->IsActor())) {
        static_cast<Actor*>(apRefObject)->RemovePerk(pperk, v10);
    }

    return true;
}

bool Cmd_HasPerk_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    void* aiParam2;
    a8 = nullptr;
    aiParam2 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
               &aiParam2
           ) &&
        Script::HasPerkConditionFunction(apRefObject, a8, aiParam2, afReturnValue);
}

bool Cmd_SendSherlockDebugText(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, acPageName
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

bool Cmd_RewardXP(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    pPlayer->RewardExperience(a8);
    return true;
}

bool Cmd_RewardKarma(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    pPlayer->RewardKarma(a8);
    return true;
}

bool Cmd_ShowRepairMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if ((apRefObject) && (apRefObject->IsActor())) {
        Interface::CreateRepairServicesMenu(static_cast<Actor*>(apRefObject));
    }

    return true;
}

bool Cmd_AddNote(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSNote* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8) {
        pPlayer->AddNote(a8, GetThreadTLSData()->bSystemOutput);
    }
    return true;
}

bool Cmd_RemoveNote(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSNote* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8) {
        pPlayer->RemoveNote(a8);
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Removed Note %s from Player ", a8->GetFullName());
    }
    return true;
}

bool Cmd_AddToFaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* a8;
    int32_t aiRank;
    a8 = nullptr;
    aiRank = -1;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &aiRank
        )) {
        return false;
    }
    if ((a8) && (apRefObject->IsActor())) {
        static_cast<Actor*>(apRefObject)->Actor::AddFaction(a8, aiRank);
    }

    return true;
}

bool Cmd_RemoveFromFaction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESFaction* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if ((a8) && (apRefObject->IsActor())) {
        static_cast<Actor*>(apRefObject)->Actor::RemoveFaction(a8);
    }

    return true;
}

bool Cmd_SetDisposition(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Actor* a8;
    int32_t v12;
    Actor* v13 = nullptr;
    if (apRefObject->IsActor()) {
        v13 = static_cast<Actor*>(apRefObject);
    }
    a8 = nullptr;
    v12 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v12
        )) {
        return false;
    }
    if ((a8) && (v13)) {
        int32_t v10 = v13->GetDisposition(a8, nullptr);
        v13->ModifyDispositionTowardActor(a8, static_cast<float>(v12 - v10));
    }

    return true;
}

bool Cmd_GetHitLocation_Execute(
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
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetHitLocation >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_GetLastHitCritical_Execute(
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
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetLastHitCritical >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_IsPC1stPerson_Execute(
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
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("IsPC1stPerson >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_SetCombatActionCost(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[516];
    float afCost;
    afCost = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8, &afCost
        )) {
        return false;
    }
    char v11 = SetCombatActionCost(a8, afCost);
    if (GetThreadTLSData()->bSystemOutput) {
        if (v11) {
            Interface::PrintLine("%s >> %0.2f", a8, afCost);
        }
        else {
            Interface::PrintLine("%s not found", a8);
        }
    }
    return true;
}

bool Cmd_ToggleCombatDebug(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, cOption
        )) {
        return false;
    }
    if (!std::strlen(cOption)) {
        // No option: turn every combat debug display off when any is on, else turn them all on.
        uint8_t bAnyOn = bDebugCombatCoverReservations.Bool() || bDebugCombatThreats.Bool() ||
            bDebugCombatGroups.Bool() || bDebugCombatGroups2.Bool() || bDebugCombatTargets.Bool() ||
            bDebugCombatCoverSearch.Bool() || bDebugCombatSearch.Bool() || bDebugCombatCoverSearch2.Bool() ||
            bDebugCombatCoverSearchText.Bool() || bDebugCombatCoverReservationsText.Bool() ||
            bDebugCombatAttackerText.Bool() || bDebugCombatGuardRadius.Bool() || bDebugCombatGroupClusters.Bool() ||
            bDebugCombatUnreachableLocations.Bool() || bDebugCombatDetectionEvents.Bool() ||
            bDebugCombatProjectileLOS.Bool() || bDebugCombatArea.Bool();
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
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("All Combat debugging turned %s", bAnyOn ? "Off" : "On");
        }
    }
    else if (!BSstricmp(cOption, "cover")) {
        bDebugCombatCoverReservations = !bDebugCombatCoverReservations.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "Combat Cover Reservations debugging turned %s", bDebugCombatCoverReservations.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "covertext")) {
        bDebugCombatCoverReservationsText = !bDebugCombatCoverReservationsText.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "Combat Cover Reservations debug text turned %s",
                bDebugCombatCoverReservationsText.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "cover2") || !BSstricmp(cOption, "coversearch")) {
        bDebugCombatCoverSearch = !bDebugCombatCoverSearch.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "Combat Cover Search debugging turned %s", bDebugCombatCoverSearch.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "cover2text") || !BSstricmp(cOption, "coversearchtext")) {
        bDebugCombatCoverSearchText = !bDebugCombatCoverSearchText.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "Combat Cover Search debug text turned %s", bDebugCombatCoverSearchText.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "cover3") || !BSstricmp(cOption, "coversearch2")) {
        bDebugCombatCoverSearch2 = !bDebugCombatCoverSearch2.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "Combat Secondary Cover Search debugging turned %s", bDebugCombatCoverSearch2.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "threats")) {
        bDebugCombatThreats = !bDebugCombatThreats.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("Combat Threat debugging turned %s", bDebugCombatThreats.Bool() ? "On" : "Off");
        }
    }
    else if (!BSstricmp(cOption, "groups")) {
        bDebugCombatGroups = !bDebugCombatGroups.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("Combat Group debugging turned %s", bDebugCombatGroups.Bool() ? "On" : "Off");
        }
    }
    else if (!BSstricmp(cOption, "groups2")) {
        bDebugCombatGroups2 = !bDebugCombatGroups2.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("Combat Group 2 debugging turned %s", bDebugCombatGroups2.Bool() ? "On" : "Off");
        }
    }
    else if (!BSstricmp(cOption, "targets")) {
        bDebugCombatTargets = !bDebugCombatTargets.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("Combat Target debugging turned %s", bDebugCombatTargets.Bool() ? "On" : "Off");
        }
    }
    else if (!BSstricmp(cOption, "search")) {
        bDebugCombatSearch = !bDebugCombatSearch.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("Combat Search debugging turned %s", bDebugCombatSearch.Bool() ? "On" : "Off");
        }
    }
    else if (!BSstricmp(cOption, "guard")) {
        bDebugCombatGuardRadius = !bDebugCombatGuardRadius.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "Combat Guard Radius debugging turned %s", bDebugCombatGuardRadius.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "cluster") || !BSstricmp(cOption, "clusters")) {
        bDebugCombatGroupClusters = !bDebugCombatGroupClusters.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "Combat Group Cluster debugging turned %s", bDebugCombatGroupClusters.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "text")) {
        bDebugCombatAttackerText = !bDebugCombatAttackerText.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "Combat Attacker Text debugging turned %s", bDebugCombatAttackerText.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "color")) {
        bDebugCombatTextColorDark = !bDebugCombatTextColorDark.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "Combat Debug Text color changed to %s", bDebugCombatTextColorDark.Bool() ? "Dark" : "Light"
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
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("Combat Debug Text size changed to %.2f", fDebugCombatTextSize.Float());
        }
    }
    else if (!BSstricmp(cOption, "unreach") || !BSstricmp(cOption, "unreachable")) {
        bDebugCombatUnreachableLocations = !bDebugCombatUnreachableLocations.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "Combat Unreachable Locations debugging turned %s",
                bDebugCombatUnreachableLocations.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "events")) {
        bDebugCombatDetectionEvents = !bDebugCombatDetectionEvents.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "Combat Detection Events debugging turned %s", bDebugCombatDetectionEvents.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "plos")) {
        bDebugCombatProjectileLOS = !bDebugCombatProjectileLOS.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "Combat Projectile LOS debugging turned %s", bDebugCombatProjectileLOS.Bool() ? "On" : "Off"
            );
        }
    }
    else if (!BSstricmp(cOption, "area")) {
        bDebugCombatArea = !bDebugCombatArea.Bool();
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("Combat Area debugging turned %s", bDebugCombatArea.Bool() ? "On" : "Off");
        }
    }
    else if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Unknown option '%s'", cOption);
    }
    return true;
}

bool Cmd_SetActorsAI(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a8;
    if (apRefObject->IsActor()) {
        if (!Script::ParseParameters(
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
            )) {
            return false;
        }
        TESObjectREFR* v10 = dynamic_cast<Actor*>(apRefObject);
        if (v10) {
            if (a8) {
                v10[1].m_Extra.iFlags[8] = 1;
            }
            else {
                v10[1].m_Extra.iFlags[8] = 0;
            }
            if (GetThreadTLSData()->bSystemOutput) {
                const char* v9;
                if (v10[1].m_Extra.iFlags[8]) {
                    v9 = "On";
                }
                else {
                    v9 = "Off";
                }
                Interface::PrintLine("%s processing is  %s", v10->GetFullName(), v9);
            }
        }
    }
    return true;
}

bool Cmd_ClearOwnership(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ExtraDataList* extra;
    if (apRefObject) {
        extra = apRefObject->TESObjectREFR::GetExtra();
        extra->RemoveOwnership();
        apRefObject->AddChange(0x40u);
    }
    return true;
}

bool Cmd_ShowInventory(
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
                ItemChange* pItem = apRefObject->GetInventoryItem_005754A0(i, false);
                if ((pItem) && (pItem->pContainerObj)) {
                    TESBoundObject* pObject = pItem->pContainerObj;
                    int32_t iFormType = pObject->GetFormType();
                    if (iFormType == 24) {
                        const char* pName;
                        const char* pWorn;
                        if (pItem->GetWorn(false)) {
                            pWorn = " - Worn";
                        }
                        else {
                            pWorn = "";
                        }
                        if (pObject->GetFormEditorIDLength()) {
                            pName = pObject->GetFormEditorID();
                        }
                        else {
                            pName = TESFullName::GetFullName(pObject);
                        }
                        double fHealthPercent = pItem->GetItemHealth(true);
                        double fHealth = pItem->GetItemHealth(false);
                        Interface::PrintLine(
                            "%d - %s (%08X) (%.0f,%0.2f%%)%s", pItem->iNumber, pName, pObject->iFormID, fHealth,
                            fHealthPercent, pWorn
                        );
                    }
                    else if (iFormType == 40) {
                        const char* pName;
                        const char* pWorn;
                        const char* pCannotEquip;
                        if (!pActor || pActor->CanUseWeapon(static_cast<TESObjectWEAP*>(pObject), true)) {
                            pCannotEquip = "";
                        }
                        else {
                            pCannotEquip = " [CANNOT EQUIP]";
                        }
                        if (pItem->GetWorn(false)) {
                            pWorn = " - Worn";
                        }
                        else {
                            pWorn = "";
                        }
                        if (pObject->GetFormEditorIDLength()) {
                            pName = pObject->GetFormEditorID();
                        }
                        else {
                            pName = TESFullName::GetFullName(pObject);
                        }
                        double fHealthPercent = pItem->GetItemHealth(true);
                        double fHealth = pItem->GetItemHealth(false);
                        Interface::PrintLine(
                            "%d - %s (%08X) (%.0f,%0.2f%%)%s%s", pItem->iNumber, pName, pObject->iFormID, fHealth,
                            fHealthPercent, pWorn, pCannotEquip
                        );
                    }
                    else {
                        const char* pName;
                        const char* pWorn;
                        if (pItem->GetWorn(false)) {
                            pWorn = " - Worn";
                        }
                        else {
                            pWorn = "";
                        }
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

bool Cmd_ShowChargenMenuParams(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v9;
    int32_t aiMaxPoints;
    int32_t a8;
    bool v11 = true;
    a8 = 0;
    v9 = 0;
    aiMaxPoints = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v9,
            &aiMaxPoints
        )) {
        return false;
    }
    if (a8 <= 0) {
        if (aiMaxPoints > 0) {
            CharGenMenu::Create_(1, aiMaxPoints, false, true);
        }
    }
    else {
        CharGenMenu::Create_(0, a8, false, true);
    }
    return v11;
}

bool Cmd_ShowChargenMenuModValues(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v9;
    int32_t aiMaxPoints;
    int32_t a8;
    bool v11 = true;
    a8 = 0;
    v9 = 0;
    aiMaxPoints = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v9,
            &aiMaxPoints
        )) {
        return false;
    }
    if (a8 <= 0) {
        if (aiMaxPoints > 0) {
            CharGenMenu::Create_(1, aiMaxPoints, true, true);
        }
    }
    else {
        CharGenMenu::Create_(0, a8, true, true);
    }
    return v11;
}

bool Cmd_SetSPECIALPoints(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    bool v10 = true;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    CharGenMenu::Create_(0, a8, false, true);
    return v10;
}

bool Cmd_AddSPECIALPoints(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    bool v10 = true;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    CharGenMenu::Create_(0, a8, true, true);
    return v10;
}

bool Cmd_SetTagSkills(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    int32_t v11;
    bool v10 = true;
    a8 = 0;
    v11 = 1;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v11
        )) {
        return false;
    }
    CharGenMenu::Create_(1, a8, false, v11 == 1);
    return v10;
}

bool Cmd_AddTagSkills(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    bool v10 = true;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    CharGenMenu::Create_(1, a8, true, true);
    return v10;
}

bool Cmd_ShowLockpickMenu(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    a8 = 3;
    Script::ParseParameters(
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
    );
    _ShowLockPickMenu(nullptr, a8 - 1, false);
    return true;
}

bool Cmd_ShowLockpickMenuDebug(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    a8 = 3;
    Script::ParseParameters(
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
    );
    _ShowLockPickMenu(nullptr, a8 - 1, true);
    return true;
}

bool Cmd_ResetPerformanceTimers(
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

bool Cmd_ToggleDebugDecal(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    a8 = -1;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8 != -1) {
        switch (a8) {
            case 0: {
                if ((BGSDecalManager::iDecalDebugFlags & 1) != 0) {
                    BGSDecalManager::iDecalDebugFlags &= ~1u;
                    Interface::PrintLine("Wireframe Decal Clipping Planes : DISABLED");
                }
                else {
                    BGSDecalManager::iDecalDebugFlags |= 1u;
                    Interface::PrintLine("Wireframe Decal Clipping Planes : ENABLED");
                }
                break;
            }
            case 1: {
                if ((BGSDecalManager::iDecalDebugFlags & 2) != 0) {
                    BGSDecalManager::iDecalDebugFlags &= ~2u;
                    Interface::PrintLine("Decal Clipping Planes : DISABLED");
                }
                else {
                    BGSDecalManager::iDecalDebugFlags |= 2u;
                    Interface::PrintLine("Decal Clipping Planes : ENABLED");
                }
                break;
            }
            case 2: {
                if ((BGSDecalManager::iDecalDebugFlags & 4) != 0) {
                    BGSDecalManager::iDecalDebugFlags &= ~4u;
                    Interface::PrintLine("Occlusion query camera direction : DISABLED");
                }
                else {
                    BGSDecalManager::iDecalDebugFlags |= 4u;
                    Interface::PrintLine("Occlusion query camera direction : ENABLED");
                }
                break;
            }
            case 3: {
                if ((BGSDecalManager::iDecalDebugFlags & 8) != 0) {
                    BGSDecalManager::iDecalDebugFlags &= ~8u;
                    Interface::PrintLine("Decal Transform : DISABLED");
                }
                else {
                    BGSDecalManager::iDecalDebugFlags |= 8u;
                    Interface::PrintLine("Decal Transform : ENABLED");
                }
                break;
            }
            case 4: {
                if ((BGSDecalManager::iDecalDebugFlags & 16) != 0) {
                    BGSDecalManager::iDecalDebugFlags &= ~16u;
                    Interface::PrintLine("Failed Decals : DISABLED");
                }
                else {
                    BGSDecalManager::iDecalDebugFlags |= 16u;
                    Interface::PrintLine("Failed Decals : ENABLED");
                }
                break;
            }
            case 5: {
                if ((BGSDecalManager::iDecalDebugFlags & 32) != 0) {
                    BGSDecalManager::iDecalDebugFlags &= ~32u;
                    g_enableWireframeDecals = 0;
                    Interface::PrintLine("Wireframe Decals : DISABLED");
                }
                else {
                    BGSDecalManager::iDecalDebugFlags |= 32u;
                    g_enableWireframeDecals = 1;
                    Interface::PrintLine("Wireframe Decals : ENABLED");
                }
                break;
            }
            default: {
                return true;
            }
        }
    }
    return true;
}

bool Cmd_SetImageSpaceModifiersEnable(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8;
    a8 = -1;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (!a8) {
        TESImageSpaceModifier::SetEnable(false);
    }
    if (a8 == 1) {
        TESImageSpaceModifier::SetEnable(true);
    }
    return true;
}

bool Cmd_IsMoving(
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
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsMoving >> %0.0f", afReturnValue);
    }
    return true;
}

bool Cmd_IsTurning(
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
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsTurning >> %0.0f", afReturnValue);
    }
    return true;
}

bool Cmd_GetAnimAction_Execute(
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
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetAnimAction >> %0.0f", afReturnValue);
    }
    return true;
}

bool Cmd_UseWeapon(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pWeapon,
            &pLocRef, &pTargetRef, &iAttackTimes, &iCrouch, &iHoldFire, &iAlwaysHit, &iDoNoDamage, &pTargetLocRef
        )) {
        return false;
    }
    if (apRefObject->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        pActor->StopCombat(nullptr);
        pActor->InitiateUseWeaponPackage(
            pWeapon, pLocRef, pTargetRef, iAttackTimes, pTargetLocRef, iCrouch != 0, iHoldFire != 0, iAlwaysHit != 0,
            iDoNoDamage != 0
        );
    }
    return true;
}

bool Cmd_EvaluateSpellConditions(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    MagicTarget* v4 = apRefObject->GetMagicTarget();
    if (v4) {
        v4->EvaluateSpellConditions();
    }
    return true;
}

bool Cmd_ClearScreenBlood(
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

bool Cmd_TriggerScreenSplatter(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t uiCount = 0;
    float fDuration = (float)0.0;
    float fSizeMult = (float)0.0;
    float fOpacityMult = (float)0.0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &uiCount,
            &fDuration, &fSizeMult, &fOpacityMult
        )) {
        return false;
    }
    ScreenCustomSplatter::Activate((uint32_t)uiCount, fDuration, SCREENSCUSTOMPLATTER_BLOOD, fSizeMult, fOpacityMult);
    return true;
}

bool Cmd_SetScreenSplatterFade(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float fFadeStart;
    // The Integer parameter (SCRIPT_PARAM_INT) parses to 4 bytes, so iNoFade is given 4 bytes. Only the low byte is
    // tested against 0 before it is passed on.
    int32_t iNoFade;
    fFadeStart = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fFadeStart,
            &iNoFade
        )) {
        return false;
    }
    SetCustomSplatterFade(fFadeStart, static_cast<uint8_t>(iNoFade) != 0);
    return true;
}

bool Cmd_SetScreenSplatterTexture(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[256];
    // The Integer parameter (SCRIPT_PARAM_INT) parses to 4 bytes, so iValue is 4 bytes wide. Only the low byte is
    // passed on.
    int32_t iValue;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8, &iValue
        )) {
        return false;
    }
    ScreenCustomSplatter::SetCustomSplatterTexture(a8, static_cast<uint8_t>(iValue));
    return true;
}

bool Cmd_ClearScreenSplatter(
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

bool Cmd_AgeRace_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if ((a8) && (apRefObject) && (apRefObject->IsNpc())) {
        TESNPC* ObjectReference = static_cast<TESNPC*>(apRefObject->GetObjectReference());
        TESRace* FormRace = ObjectReference->pFormRace;
        TESRace* apRace = FormRace;
        while (a8) {
            if ((a8 & 0x80000000u) == 0) {
                if (apRace->GetOlderRace()) {
                    apRace = apRace->GetOlderRace();
                    --a8;
                }
                else {
                    a8 = 0;
                }
            }
            else {
                if (apRace->GetYoungerRace()) {
                    apRace = apRace->GetYoungerRace();
                    ++a8;
                }
                else {
                    a8 = 0;
                }
            }
        }
        if (apRace != FormRace) {
            bool v9 = apRefObject->IsActor();
            ObjectReference->SwitchRace(apRace, v9 ? static_cast<Actor*>(apRefObject) : nullptr);
            static_cast<Character*>(apRefObject)->Character::Reset3D();
        }
    }

    return true;
}

bool GetVATSBackAreaFreeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetVATSBackAreaFreeConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool GetVATSFrontAreaFreeFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetVATSFrontAreaFreeConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

void Set(char a1) {
    bDumpNextFrame = a1;
}

bool GetVATSRightTargetVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetVATSRightTargetVisibleConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool GetVATSLeftTargetVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetVATSLeftTargetVisibleConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool GetVATSBackTargetVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetVATSBackTargetVisibleConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool GetVATSFrontTargetVisibleFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetVATSFrontTargetVisibleConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

bool Cmd_SetCriticalStage(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ACTOR_CRITICAL_STAGE a8;
    a8 = ACS_NONE;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if ((apRefObject) && (apRefObject->IsActor())) {
        static_cast<Actor*>(apRefObject)->Actor::SetCriticalStage(a8);
    }

    return true;
}

bool IsInCriticalStageFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_IsInCriticalStage(apRefObject, a8, nullptr, afReturnValue);
}

bool Cmd_Sin(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v10;
    float a8;
    a8 = 0.0f;
    v10 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v10
        )) {
        return false;
    }
    if (v10) {
        if (a8 < -1.0) {
            a8 = -1.0f;
        }
        if (a8 > 1.0) {
            a8 = 1.0f;
        }
        afReturnValue = std::asin(a8) * 57.2957763671875;
    }
    else {
        float afX = a8 * 0.01745329238474369;
        afReturnValue = std::sin(afX);
    }
    return true;
}

bool Cmd_Cos(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v10;
    float a8;
    a8 = 0.0f;
    v10 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v10
        )) {
        return false;
    }
    if (v10) {
        if (a8 < -1.0) {
            a8 = -1.0f;
        }
        if (a8 > 1.0) {
            a8 = 1.0f;
        }
        afReturnValue = std::acos(a8) * 57.2957763671875;
    }
    else {
        float afX = a8 * 0.01745329238474369;
        afReturnValue = std::cos(afX);
    }
    return true;
}

bool Cmd_Tan(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v10;
    float a8;
    a8 = 0.0f;
    v10 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v10
        )) {
        return false;
    }
    if (v10) {
        afReturnValue = std::atan(a8) * 57.2957763671875;
    }
    else {
        float v9 = a8 * 0.01745329238474369;
        afReturnValue = std::tan(v9);
    }
    return true;
}

bool Cmd_Sqrt(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float a8;
    a8 = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    afReturnValue = std::sqrt(a8);
    return true;
}

bool Cmd_Log(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float afX;
    float a8;
    a8 = 0.0f;
    afX = 2.7182817f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &afX
        )) {
        return false;
    }
    if (afX == 1.0) {
        afReturnValue = 0.0;
    }
    else {
        double v9 = std::log(a8);
        afReturnValue = v9 / std::log(afX);
    }
    return true;
}

bool Cmd_ShowQuestStages(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESQuest* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    BSStringT<char> kStages;
    if (GetThreadTLSData()->bSystemOutput) {
        for (BSSimpleList<TESQuestStage*>* pStageIter = &a8->m_listStages; pStageIter && pStageIter->m_item;
             pStageIter = pStageIter->m_pkNext) {
            char cBuffer[32];
            TESQuestStage* pStage = pStageIter->m_item;
            BSsprintf(cBuffer, 31, "Stage %d: %d ", pStage->m_Data.ucIndex, pStage->m_Data.bIsDone != 0);
            kStages.operator=(cBuffer);
            // Tests the item list's second link rather than its first item, as the game does.
            if (pStage->listStageItems.m_pkNext) {
                kStages += " (";
                for (BSSimpleList<TESQuestStageItem*>* pItemIter = &pStage->listStageItems;
                     pItemIter && pItemIter->m_item; pItemIter = pItemIter->m_pkNext) {
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

bool Cmd_SetTaskThreadUpdateSleep(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a1;
    a1 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a1
        )) {
        return false;
    }
    BSTaskThread::SetUpdateSleepValue(a1);
    Interface::PrintLine("BSTaskThread::sUpdateSleepValue set to %d ms", a1);
    return true;
}

bool Cmd_AddFormToFormList_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSListForm* a8;
    TESForm* a2;
    a8 = nullptr;
    a2 = nullptr;
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &a2
        )) {
        return false;
    }
    if ((a2) && (a8)) {
        a8->AddForm(a2);
    }

    return true;
}

// The leveled list the script names is the TESLeveledList base of a leveled actor (at +0x30); the form it adds is
// the leveled list's owner as well.
bool Cmd_AddNPCToLeveledList(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESLevCharacter* pList;
    TESForm* pForm;
    int32_t iLevel;
    pList = nullptr;
    pForm = nullptr;
    iLevel = 0;
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pList,
            &pForm, &iLevel
        )) {
        return false;
    }
    if ((pList) && (pForm)) {
        static_cast<TESLeveledList*>(pList)->AddScriptAddedLeveledObject(
            pList, static_cast<uint16_t>(iLevel), 1, pForm, nullptr
        );
    }

    return true;
}

bool Cmd_AddItemToLeveledList(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t iCount;
    float fHealth;
    TESLevItem* pList;
    TESForm* pForm;
    int32_t iLevel;
    pList = nullptr;
    pForm = nullptr;
    iLevel = 0;
    iCount = 0;
    fHealth = 100.0f;
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pList,
            &pForm, &iLevel, &iCount, &fHealth
        )) {
        return false;
    }
    if ((pList) && (pForm) && (iCount)) {
        ContainerItemExtra* pExtra;
        fHealth = fHealth / 100.0;
        ContainerItemExtra* apMemory = static_cast<ContainerItemExtra*>(ourMalloc(sizeof(ContainerItemExtra)));
        if (apMemory) {
            pExtra = ::new (apMemory) ContainerItemExtra;
        }
        else {
            pExtra = nullptr;
        }
        pExtra->AdjustHealthPercent(fHealth);
        static_cast<TESLeveledList*>(pList)->AddScriptAddedLeveledObject(
            pList, static_cast<uint16_t>(iLevel), static_cast<uint16_t>(iCount), pForm, pExtra
        );
    }

    return true;
}

bool Cmd_RemapWaterType(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESForm* v9;
    TESForm* a2;
    v9 = nullptr;
    a2 = nullptr;
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &v9, &a2
        )) {
        return false;
    }
    if (!v9 || v9->GetFormType() != 78) {
        Warning("Invalid water type for water to be remapped.");
        v9 = nullptr;
    }
    if (!a2 || a2->GetFormType() != 78) {
        Warning("Invalid water type for water to remap to.");
        a2 = nullptr;
    }
    if ((v9) && (a2)) {
        static_cast<TESWaterForm*>(v9)->TESWaterForm::SetRemappedWaterForm(static_cast<TESWaterForm*>(a2));
    }

    return true;
}

bool Cmd_IsHardcore_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    return (int8_t)Script::IsHardcoreConditionalFunction(apRefObject, nullptr, nullptr, afReturnValue);
}

bool Cmd_SetHardcore_Execute(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iHardcore
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

bool Cmd_ObjectUnderReticle_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESObjectREFR* pRef;
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, afParam,
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
    kCameraRotation.GetRow_005DF560(0, &kRow.y, &kRow.z, &kRow.x);
    kRotation.SetRow_005DF510(0, &kRow);
    kCameraRotation.GetRow_005DF560(1, &kRow.y, &kRow.z, &kRow.x);
    kRotation.SetRow_005DF510(1, &kRow);
    kCameraRotation.GetRow_005DF560(2, &kRow.y, &kRow.z, &kRow.x);
    kRotation.SetRow_005DF510(2, &kRow);
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
    if (pPicked) {
        pRef = TESObjectREFR::FindReferenceFor3D(pPicked);
    }
    else {
        pRef = nullptr;
    }
    if (pRef) {
        uint32_t uiID = pRef->iFormID;
        Script::PutNumericIDInDouble(uiID, afReturnValue);
    }
    return true;
}

bool Cmd_GetDLCCount(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, 0
        )) {
        char Format[68];
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
            Format, 64, "There are %u DLC packages available",
            static_cast<uint32_t>(static_cast<int64_t>(afReturnValue))
        );
        Interface::PrintLine(Format);
        return true;
    }

    afReturnValue = 0.0;
    return false;
}

// The IsDLCInstalled script command: looks for a valid DLC package whose file name matches the parameter.
bool Cmd_IsDLCInstalled(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, acName
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
        if (GetThreadTLSData()->bSystemOutput) {
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
    if ((&bAllowScriptedAutosave)->Bool()) {
        pSaveLoadManager->SetShouldSystemSave();
    }
    return true;
}

int32_t hkGetByteOffset(const void* base, const void* pntr) {
    return static_cast<int32_t>(static_cast<const char*>(pntr) - static_cast<const char*>(base));
}

MobDialogueFlagResetter::MobDialogueFlagResetter() : MobIterOperator() {}

void MobDialogueFlagResetter::Operate(MobileObject* apMob) {
    if (apMob->IsActor()) {
        BaseProcess* CurrentAIProcess = apMob->pCurrentProcess;
        if (CurrentAIProcess) {
            CurrentAIProcess->SetGreetingFlag(false);
        }
    }
    (&apMob->m_Extra)->RemoveExtra(115);
}

void BSAudioManager::SetIgnoreGlobalTimeMultiplier(bool bIgnore) {
    bIgnoreGlobalTimeMultiplier = (bool)((uint8_t)bIgnore);
}

// The next system group, wrapping within 16 bits and skipping the reserved groups 0..9 (restarts at 10).
uint32_t bhkCollisionFilter::GetNewSystemGroup() {
    uiNextGroup = static_cast<uint16_t>(uiNextGroup + 1);
    if (!uiNextGroup) {
        uiNextGroup = 10;
    }
    return uiNextGroup;
}

// The hit point: the ray's start interpolated towards its end by the hit fraction.
void bhkPickData::CalcHitPoint(hkVector4& arPoint) {
    alignas(16) hkSimdReal kFraction;
    kFraction.~hkSimdReal();
    ::new (&kFraction) hkSimdReal(m_hitFraction);
    arPoint.setInterpolate4(m_from, m_to, kFraction);
}

void bhkRagdollController::SetLookIKEnable(bool abEnable) {
    bool v2 = bInitLookIK && abEnable;
    bEnableLookIK = v2;
}

void bhkRagdollController::SetFootIKEnable(bool abEnable) {
    bool v2 = bInitFootIK && abEnable;
    bEnableFootIK = v2;
}

void bhkRagdollController::SetGrabIKEnable(bool abEnable) {
    bool v2 = bInitGrabIK && abEnable;
    bEnableGrabIK = v2;
}

bool bhkRagdollController::QGrabIK() {
    return bhkRagdollController::bGrabIK.Bool();
}

void bhkRagdollController::SetGrabIK(bool abEnable) {
    bhkRagdollController::bGrabIK.Bool() = abEnable;
}

void bhkRagdollController::SetLookIK(bool abEnable) {
    bhkRagdollController::bLookIK.Bool() = abEnable;
}

void bhkRagdollController::SetFootIK(bool abEnable) {
    bhkRagdollController::bFootIK.Bool() = abEnable;
}

bool bhkShape::GetMaterialVarType(MATERIAL_VARIANT_TYPE aeType) {
    return (static_cast<int32_t>(eMaterial) & aeType) != 0;
}

Font* FontManager::GetFont(int32_t a) {
    return pFont[a];
}

const char* Tile::GetTypeName() {
    return "TILE";
}

const char* TileMenu::GetTypeName() {
    return "MENU";
}

const char* TileRect::GetTypeName() {
    return "RECT";
}

bool BSShaderAccumulator::GetRenderDecals() {
    return bRenderDecals;
}

void BSShaderAccumulator::SetRenderDecals(bool abRenderDecals) {
    bRenderDecals = abRenderDecals;
}

void BSShaderAccumulator::ToggleRefractionDebugTexture() {
    BSShaderAccumulator::bShowRefractionDebugTexture = !BSShaderAccumulator::bShowRefractionDebugTexture;
}

void BSShaderManager::SetDebugMode(BSShaderManager::etRenderMode aeDebugMode) {
    usDebugMode = aeDebugMode;
}

ImageSpaceEffectHDR::ImageSpaceEffectHDR() : pfData(pfDefaultHDRData) {
    pTextures.SetSize(3u);
}

// The thirteen shader parameters are the effect's: each is deleted and its slot emptied.
ImageSpaceEffectHDR::~ImageSpaceEffectHDR() {
    for (uint32_t i = 0; i < 13; ++i) {
        if (pEffectParamList.m_pBase[i].m_pObject) {
            delete pEffectParamList.m_pBase[i].m_pObject;
            pEffectParamList.SetAt(i, nullptr);
        }
    }
}

float ImageSpaceEffectHDR::GetBrightScale() {
    return pfData[6];
}

float ImageSpaceEffectHDR::GetEyeAdaptSpeed() {
    return pfData[0];
}

float ImageSpaceEffectHDR::GetTargetLUM() {
    return pfData[4];
}

float ImageSpaceEffectHDR::GetUpperLUMClamp() {
    return pfData[5];
}

// The HDR getters read one float of the data record each: eye adapt speed [0], blur passes [1], emissive multiplier
// [3], target luminance [4], upper luminance clamp [5], bright scale [6], bright clamp [7].
float ImageSpaceEffectHDR::GetEmissiveHDRMult() {
    return pfData[3];
}

float ImageSpaceEffectHDR::GetNumBlurPasses() {
    return pfData[1];
}

float ImageSpaceEffectHDR::GetBrightClamp() {
    return pfData[7];
}

void ShadowSceneNode::ToggleWireframe() {
    bWireframe = !bWireframe;
}

bool ActorCause::IsValid() {
    return pActor != nullptr;
}

bool BGSProjectile::GetHasExplosionDetonates() {
    return IsFlagSet(EXPLOSION_DETONATES);
}

void BSTaskThread::SetUpdateSleepValue(int32_t a1) {
    BSTaskThread::sUpdateSleepValue = a1;
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
    if ((apRef) && (apMatchActor)) {
        FR2MatrixVTC<float>(*pNewCoord)[2][2];
        float fval = static_cast<double>(aiValue) / 100.0;
        TESNPC* pnpcToMatch = dynamic_cast<TESNPC*>(apMatchActor->GetObjectReference());
        TESNPC* pnpcToChange = dynamic_cast<TESNPC*>(apRef->GetObjectReference());
        FR2MatrixVTC<float> blendedcoord[2][2];
        FR2MatrixVTC<float> chargenoffset[2][2];
        BSSimpleArray<TESNPC*, 1024> xPresetHeads;
        TESActorBase::SEX Sex = pnpcToMatch->GetSex();
        TESRace* Race = pnpcToMatch->GetRace();
        TESNPC::FillCharGenPresetList(Race, Sex, &xPresetHeads);
        TESNPC* ppresetNPC = nullptr;
        if (xPresetHeads.GetAt(0)) {
            ppresetNPC = xPresetHeads.GetAt(0);
        }
        FR2MatrixVTC<float>(*pcoord)[2][2] = pnpcToChange->pAlternateFaceOffsetCoord;
        if (pcoord) {
            (*pcoord)[0][0].DeleteArray(3u);
            pcoord = nullptr;
            pnpcToChange->pAlternateFaceOffsetCoord = nullptr;
        }
        FR2MatrixVTC<float>(*coordToChange)[2][2] = pnpcToChange->GetOffsetFaceCoord();
        FR2MatrixVTC<float>(*coordToMatch)[2][2] = pnpcToMatch->GetOffsetFaceCoord();
        FR2MatrixVTC<float>(*presetOffsetCoord)[2][2] = ppresetNPC->GetOffsetFaceCoord();
        BSFaceGenManager::OffsetFaceGenCoord(presetOffsetCoord, coordToMatch, &chargenoffset, false);
        float fvalOldGeo = BSFaceGenManager::GetFaceCoordAttribute(*coordToChange, FANATT_AGE, FANTYPE_GEO);
        BSFaceGenManager::InitFaceGenCoord(&blendedcoord);
        BSFaceGenManager::ScaleFaceCoord(fval, &chargenoffset, true);
        BSFaceGenManager::MergeFaceGenCoord(coordToChange, &chargenoffset, &blendedcoord, true, 0.0f);
        BSFaceGenManager::SetFaceCoordAttribute(blendedcoord, FANATT_AGE, FANTYPE_GEO, fvalOldGeo);
        // new FR2MatrixVTC<float>[2][2]: new[]'s cookie, the element count, is written by hand in the size_t before
        // the elements.
        size_t* pCookie = static_cast<size_t*>(ourMalloc(sizeof(size_t) + sizeof(FR2MatrixVTC<float>[2][2])));
        if (pCookie) {
            *pCookie = 4;
            pNewCoord = ::new (static_cast<void*>(&pCookie[1])) FR2MatrixVTC<float>[1][2][2];
        }
        else {
            pNewCoord = nullptr;
        }
        pcoord = pNewCoord;
        BSFaceGenManager::InitFaceGenCoord(pcoord);
        TESRace* v4 = pnpcToChange->GetRace();
        BSFaceGenManager::OffsetFaceGenCoord(
            v4->GetMeanFaceCoord(TESActorBase::SEX_MALE), &blendedcoord, pcoord, false
        );
        BSFaceGenManager::CopyFaceGenCoord(coordToChange, pcoord, false, true);
        pnpcToChange->pAlternateFaceOffsetCoord = pcoord;
        pnpcToChange->AddChange(2048);
        if (apRef->Get3D()) {
            static_cast<Character*>(apRef)->Character::Reset3D();
        }
    }
}

bool Script::GetBaseActorValue_Eval(Actor* apActor, ActorValue::Index aeActorValue, int32_t aiArg2, double* apResult) {
    Actor* v5 = nullptr;
    if ((apActor) && (apActor->IsActor())) {
        v5 = apActor;
    }

    if (v5) {
        *apResult = static_cast<double>(v5->GetBaseActorValue(aeActorValue));
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "GetBaseActorValue: %s >> %0.2f", ActorValue::GetActorValueScriptName(aeActorValue), *apResult
            );
        }
    }
    return true;
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
    double apDest[10];
    // The compiled script data stores the argument count as a 16-bit word.
    int16_t v12 = *reinterpret_cast<const uint16_t*>(&apCompiledParams[*auiOffset]);
    *auiOffset += 2;
    ourMemset(apDest, 0, sizeof(apDest));
    for (int16_t i = 0; i < v12; ++i) {
        apDest[i] = 0.0;
        if (!Script::GetValue(
                apDest[i], apCompiledParams, *auiOffset, apRefObject, apContainer, apScript, apScriptLocals, true
            )) {
            return false;
        }
    }
    apOut->SPrintF(
        apText, apDest[0], apDest[1], apDest[2], apDest[3], apDest[4], apDest[5], apDest[6], apDest[7], apDest[8],
        apDest[9]
    );
    return true;
}

// Prints the HDR image space parameters to the console. Every float is passed as a double, including the blur pass
// count its format prints with %d, and the blur radius has no argument, as the game does.
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
    Interface::PrintLine("    iNumBlurpasses: %d fBlurRadius: %f", static_cast<double>(pManager->GetNumBlurPasses()));
    double fBrightScale = pManager->GetHDRBrightScale();
    double fBrightClamp = pManager->GetHDRBrightClamp();
    Interface::PrintLine("    fBrightClamp: %f fBrightScale: %f", fBrightClamp, fBrightScale);
    Interface::PrintLine<>("SSP:");
    Interface::PrintLine(
        "    fSunlightDimmer: %f Lum Ramp: %f", static_cast<double>(BSShaderManager::fSunlightDimmer),
        static_cast<double>(SkyShader::pLUMRamp[0])
    );
    Interface::PrintLine<>("SHP:");
    double fEmissiveHDRMult = pManager->GetEmissiveHDRMult();
    double fEyeAdaptSpeed = pManager->GetHDREyeAdaptSpeed();
    Interface::PrintLine("    fEyeAdaptSpeed: %f fEmissiveHDRMult: %f", fEyeAdaptSpeed, fEmissiveHDRMult);
    Interface::PrintLine(
        "    fTreeDimmer: %f fGrassDimmer: %f", static_cast<double>(BSShaderManager::fTreeDimmer),
        static_cast<double>(BSShaderManager::fGrassDimmer)
    );
    double fTargetLUM = pManager->GetHDRTargetLUM();
    double fUpperLUMClamp = pManager->GetHDRUpperLUMClamp();
    Interface::PrintLine("    fUpperLUMClamp: %f fTargetLUM: %f", fUpperLUMClamp, fTargetLUM);
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pPackage
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
        // 0x800 is the OnPackageChange script event.
        SetActionFlag(pOldPackage, pActor->GetExtra(), 0x800);
        pActor->GetCurrentAIProcess()->ChangePackage(pActor, pOldPackage);
    }
    pActor->GetCurrentAIProcess()->SetPackageDoneOnce(false);
    // 0x200 is the OnPackageStart script event.
    SetActionFlag(pPackage, pActor->GetExtra(), 0x200);
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
            ReferenceProcessLists.AddReference(
                pActor, pActor->GetCurrentAIProcess()->GetProcessLevel(), false, false, nullptr
            );
        }
    }
    else if (pActor == pPlayer) {
        pPlayer->SetAIControlledPackage(true);
    }
    return true;
}

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
    int32_t v10;
    MobileObject* apActivator;
    apActivator = nullptr;
    v10 = 0;
    if (pPlayer == apRefObject) {
        Warning("SCRIPTS: Never have the player character activate something in a script very Bad");
        return false;
    }
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &apActivator,
            &v10
        )) {
        return false;
    }
    if (apRefObject) {
        if (GetThreadTLSData()->uiActivateRecursionDepth > 5) {
            return true;
        }
        GetThreadTLSData()->uiActivateRecursionDepth = GetThreadTLSData()->uiActivateRecursionDepth + 1;
        if (!apActivator) {
            apActivator = static_cast<MobileObject*>(apRefObject->GetActionRef());
        }
        if ((apActivator) && (!apActivator->GetDisabled())) {
            if (v10) {
                apRefObject->Activate(apActivator, false, nullptr, 1);
            }
            else {
                bool Action = apRefObject->GetAction(OBJECT_ACTION_USE_DEFAULT);
                if (!Action) {
                    apRefObject->SetAction(OBJECT_ACTION_USE_DEFAULT);
                }
                apRefObject->SetAction(OBJECT_ACTION_ACTIVATE);
                apRefObject->Activate(apActivator, false, nullptr, 1);
                apRefObject->ClearAction(OBJECT_ACTION_ACTIVATE);
                if (!Action) {
                    apRefObject->ClearAction(OBJECT_ACTION_USE_DEFAULT);
                }
            }
        }

        GetThreadTLSData()->uiActivateRecursionDepth = GetThreadTLSData()->uiActivateRecursionDepth - 1;
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pSource
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
    TESObjectREFR* a2;
    a2 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a2
           ) &&
        Script::GetDispositionConditionFunction(apRefObject, a2, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    Cmd_GetPCFactionMurder(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    Cmd_GetPCEnemyofFaction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    Cmd_GetPCFactionAttack(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::IsPlayerActionActiveConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetTalkingActivatorActorConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetHasNoteConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_Exists(apRefObject, a8, nullptr, afReturnValue);
}

void Script::MoveToFunctionBase(TESObjectREFR* apMoveRef, TESObjectREFR* apTargetRef, float afX, float afY, float afZ) {
    if ((apMoveRef != pPlayer || !pPlayer->IsDead(false)) && (apMoveRef) && (apTargetRef)) {
        NiPoint3 position;
        bool bactor = apMoveRef->IsActor();
        if (bactor) {
            Actor* pact = static_cast<Actor*>(apMoveRef);
            ReferenceProcessLists.StopCombatandAlarmonActor(static_cast<Actor*>(apMoveRef), false);
            BaseProcess* pcurProc = pact->GetCurrentAIProcess();
            if ((pcurProc) && (pcurProc->GetSitSleepState())) {
                pact->GetOutofFurnitureQuick();
            }
        }
        TESObjectCELL* pCell = apTargetRef->GetParentCell();
        TESWorldSpace* pWorld = apTargetRef->GetWorldSpace();
        NiPoint3 afz = apTargetRef->GetAngleOnReference();
        const NiPoint3* v5 = &apTargetRef->GetLocationOnReference();
        position.x = v5->x;
        position.y = v5->y;
        float z = v5->z;
        position.x = position.x + afX;
        position.y = position.y + afY;
        position.z = z + afZ;
        if (apMoveRef == pPlayer) {
            PLAYER_TARGET_LOC* ploc;
            void* apMemory = ourMalloc(sizeof(PLAYER_TARGET_LOC));
            if (apMemory) {
                ploc = ::new (apMemory) PLAYER_TARGET_LOC;
            }
            else {
                ploc = nullptr;
            }
            ploc->Location = position;
            ploc->Angle.z = afz.z;
            ploc->Angle.x = afz.x;
            ploc->bResetWeather = true;
            if (pWorld) {
                ploc->pWorld = pWorld;
            }
            else {
                if ((pCell) && (pCell->GetInterior())) {
                    ploc->pInterior = pCell;
                }
            }
            ploc->pFurnitureRef = apTargetRef;
            pPlayer->RequestPositionPlayer(ploc);
            Interface::CloseConsole();
        }
        else {
            bool bmob = apMoveRef->IsMobileObject();
            apMoveRef->SetLocationOnReference(position);
            if (pCell && TES::pInstance->IsCellLoaded(pCell, false)) {
                apMoveRef->SetAngleOnReferenceZ(afz.z);
                apMoveRef->SetAngleOnReferenceX(0.0f);
                NiAVObject* p3D = (NiAVObject*)apMoveRef->Get3D();
                if (p3D) {
                    const NiPoint3* v7 = &apMoveRef->GetLocationOnReference();
                    p3D->SetTranslate(*v7);
                    p3D->SetRotate(apMoveRef->GetOrientation());
                    bhkNiCollisionObject::ResetSim(p3D, true);
                    NiUpdateData UpdateData(0.0f, false, false);
                    p3D->Update(UpdateData);
                }
            }
            else {
                if (apMoveRef != pPlayer && bmob) {
                    apMoveRef->SetAngleOnReferenceX(3.4028235e+38f);
                }
            }
            TESObjectREFR::MoveRefToNewSpace(apMoveRef, pCell, pWorld);
            if (apMoveRef->GetParentCell()) {
                TESObjectCELL* ParentCell = apMoveRef->GetParentCell();
                if (TES::pInstance->IsCellLoaded(ParentCell, false)) {
                    apMoveRef->InitHavok();
                }
            }
            if (bmob) {
                MobileObject* pmob = static_cast<MobileObject*>(apMoveRef);
                bhkCharacterController* pcon = static_cast<MobileObject*>(apMoveRef)->MobileObject::GetCharController();
                if (pcon) {
                    pcon->SetPosition(position);
                }
            }
            if (bactor) {
                Actor* pactor = static_cast<Actor*>(apMoveRef);
                if (pactor->GetCurrentAIProcess()) {
                    BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
                    CurrentAIProcess->ComputeLastTimeProcessed();
                    pactor->StopMoving();
                    BaseProcess* v10 = pactor->pCurrentProcess;
                    v10->CheckforNewPackage(pactor, true);
                    pactor->FadeIn();
                    pactor->UpdateAlpha();
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
            apParameters, apCompiledParams, *auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pTarget,
            &fX, &fY, &fZ
        )) {
        return false;
    }
    if (apRefObject && !apRefObject->CanBeMoved()) {
        if (GetThreadSystemOutput()) {
            Interface::PrintLine("%s cannot be moved.", apRefObject->GetFormEditorID());
        }
        if (!pDataHandler->IsFormIDCreated(apScript->GetFormID())) {
            uint32_t uiRefID = apRefObject->GetFormID();
            const char* pRefName = apRefObject->GetFormEditorID();
            uint32_t uiScriptID = apScript->GetFormID();
            const char* pScriptName = apScript->GetFormEditorID();
            Warning(
                "SCRIPTS: Script '%s' (%08X) is attempting to move reference '%s' (%08X), but it cannot be moved",
                pScriptName, uiScriptID, pRefName, uiRefID
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
            apParameters, apCompiledParams, *auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pTarget,
            &fX, &fY, &fZ
        )) {
        return false;
    }
    if (apRefObject && !apRefObject->CanBeMoved()) {
        if (GetThreadSystemOutput()) {
            Interface::PrintLine("%s cannot be moved.", apRefObject->GetFormEditorID());
        }
        if (!pDataHandler->IsFormIDCreated(apScript->GetFormID())) {
            uint32_t uiRefID = apRefObject->GetFormID();
            const char* pRefName = apRefObject->GetFormEditorID();
            uint32_t uiScriptID = apScript->GetFormID();
            const char* pScriptName = apScript->GetFormEditorID();
            Warning(
                "SCRIPTS: Script '%s' (%08X) is attempting to move reference '%s' (%08X), but it cannot be moved",
                pScriptName, uiScriptID, pRefName, uiRefID
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pPackage
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
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iPoints
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
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iPoints
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
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iValue
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pContainer
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pForm,
            &fHealthPercent, &iCount, &iDistance, &iDirection
        )) {
        return false;
    }
    TESObjectREFR* pPlaced = PlaceAtMeFunctionBase(
        apRefObject, static_cast<TESObject*>(pForm), iCount, iDistance, iDirection, fHealthPercent
    );
    if (pPlaced) {
        uint32_t uiFormID = pPlaced->GetFormID();
        PutNumericIDInDouble(uiFormID, afReturnValue);
    }
    return true;
}

// Shows (a nonzero third parameter, once) or hides the parsed objective of an uncompleted quest; a missing quest or
// objective is a warning. Takes the form ID before the editor name for each warning, and on the missing quest path
// still calls both on the null quest, as the game does.
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pQuest,
            &uiIndex, &iDisplayed
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, cFileName
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fAlpha
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fMass
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
    bhkWorldNS::ObjectRecData recData;
    recData.pWorld = nullptr;
    recData.eAction = bhkWorldNS::OA_TOTAL;
    recData.bRecurse = true;
    recData.uDataA[0].f = afValue;
    NiNode* v2 = apRefObject->Get3D();
    bhkWorld::DoObjects(v2, recData, &SetRBMass);
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
    if (GetThreadSystemOutput()) {
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iObjectType
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
        if (!GetThreadSystemOutput()) {
            return true;
        }
        static const char* apObjectTypeNames[7] = {"Undetermined", "Architecture", "Furniture", "Actors",
                                                   "Items",        "Misc",         "Other"};
        const char* pName = apObjectTypeNames[iObjectType];
        if ((BSShaderManager::uiObjectTypeShadowMask & (1u << iObjectType)) != 0) {
            Interface::PrintLine<>("Shadows %s: %s", pName, "On");
        }
        else {
            Interface::PrintLine<>("Shadows %s: %s", pName, "Off");
        }
        return true;
    }
    if (GetThreadSystemOutput()) {
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, acType,
            &iIndex, &iValue
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
                        acLine, 0x200, "%d. %s", i,
                        BSFaceGenManager::GetFaceGenExpressionText(static_cast<BS_FACEGEN_EXPRESSION>(i))
                    );
                }
                else {
                    BSsprintf(
                        acLine, 0x200, "%d. %s, ", i,
                        BSFaceGenManager::GetFaceGenExpressionText(static_cast<BS_FACEGEN_EXPRESSION>(i))
                    );
                }
                *pList += acLine;
            }
            // SWEEP-INLINE 0x4047F0: the by-value argument's copy, which the call makes and the callee destroys.
            Interface::PrintLine(*pList);
            // SWEEP-INLINE 0x4037D0
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
                        acLine, 0x200, "%d. %s", i,
                        BSFaceGenManager::GetFaceGenPhonemeText(static_cast<BS_FACEGEN_PHONEME>(i))
                    );
                }
                else {
                    BSsprintf(
                        acLine, 0x200, "%d. %s, ", i,
                        BSFaceGenManager::GetFaceGenPhonemeText(static_cast<BS_FACEGEN_PHONEME>(i))
                    );
                }
                *pList += acLine;
            }
            // SWEEP-INLINE 0x4047F0: the by-value argument's copy, which the call makes and the callee destroys.
            Interface::PrintLine(*pList);
            // SWEEP-INLINE 0x4037D0
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
                        acLine, 0x200, "%d. %s", i,
                        BSFaceGenManager::GetFaceGenModifierText(static_cast<BS_FACEGEN_MODIFIER>(i))
                    );
                }
                else {
                    BSsprintf(
                        acLine, 0x200, "%d. %s, ", i,
                        BSFaceGenManager::GetFaceGenModifierText(static_cast<BS_FACEGEN_MODIFIER>(i))
                    );
                }
                *pList += acLine;
            }
            // SWEEP-INLINE 0x4047F0: the by-value argument's copy, which the call makes and the callee destroys.
            Interface::PrintLine(*pList);
            // SWEEP-INLINE 0x4037D0
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
                    NiUpdateData kUpdateData;
                    kUpdateData.~NiUpdateData();
                    ::new (&kUpdateData) NiUpdateData(0.0f, false, false);
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, cCode
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, cCode
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
            apParameters, apCompiledParams, *auiOffset, apRefObject, apContainer, apScript, apScriptLocals,
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
        NiColorA kFalloff = NiColorA_WHITE * -1.0f;
        if (!Script::ParseParameters(
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals,
                &kFalloff.r, &kFalloff.g, &kFalloff.b, &kFalloff.a
            )) {
            return false;
        }
        if (kFalloff.r != -1.0) {
            kFalloff.r = BSMAX(BSMIN(kFalloff.r, 90.0f), 0.0f);
            float fRadians = (float)(kFalloff.r * 0.01745329238474369);
            kFalloff.r = NiCos(fRadians);
        }
        if (kFalloff.g != -1.0) {
            kFalloff.g = BSMAX(BSMIN(kFalloff.g, 90.0f), 0.0f);
            float fRadians = (float)(kFalloff.g * 0.01745329238474369);
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
        // EXE-INLINED none: the thread's system-output flag read
        if (Script::GetThreadSystemOutput()) {
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fMult
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fLocalBias,
            &fLODBias
        )) {
        return false;
    }
    float fOldLOD = BSShaderManager::fLODTreeMipMapLODBias;
    float fOldLocal = BSShaderManager::fLocalTreeMipMapLODBias;
    alignas(4) char kStringMem[sizeof(BSStringT<char>)];
    BSStringT<char>* pString = ::new (kStringMem) BSStringT<char>;
    pString->SPrintF("Old values:   [ Local Trees: %f | LOD Trees: %f ]", (double)fOldLocal, (double)fOldLOD);
    // The by-value argument's copy, which the call makes and the callee destroys.
    Interface::PrintLine(*pString);
    BSShaderManager::fLODTreeMipMapLODBias = fLODBias;
    BSShaderManager::fLocalTreeMipMapLODBias = fLocalBias;
    pString->SPrintF("New values:   [ Local Trees: %f | LOD Trees: %f ]", (double)fLocalBias, (double)fLODBias);
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
    kDecal.~DECAL_CREATION_DATA();
    ::new (&kDecal) DECAL_CREATION_DATA;
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
                pImpact = pSet ? pSet->GetImpactData((BGSImpactDataSet::IMPACT_MATERIAL_TYPE)iMaterial) : nullptr;
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
            kDecal.cSubTex = (char)std::floor(RandomFloat(0.0f, 4.0f));
            float fMaxWidth = pImpact->GetDecalMaxWidth();
            float fMinWidth = (float)pImpact->GetMinWidth();
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &cAxis
        )) {
        return false;
    }
    return Script::GetPosConditionFunction(
        apRefObject, reinterpret_cast<void*>(static_cast<intptr_t>((int8_t)cAxis)), nullptr, afReturnValue
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &cAxis
        )) {
        return false;
    }
    return Script::GetStartingPosConditionFunction(
        apRefObject, reinterpret_cast<void*>(static_cast<intptr_t>((int8_t)cAxis)), nullptr, afReturnValue
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &cAxis
        )) {
        return false;
    }
    return Script::GetAngleConditionFunction(
        apRefObject, reinterpret_cast<void*>(static_cast<intptr_t>((int8_t)cAxis)), nullptr, afReturnValue
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &cAxis
        )) {
        return false;
    }
    return Script::GetStartingAngleConditionFunction(
        apRefObject, reinterpret_cast<void*>(static_cast<intptr_t>((int8_t)cAxis)), nullptr, afReturnValue
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iSex
        )) {
        return false;
    }
    return Script::GetIsSexConditionFunction(
        apRefObject, reinterpret_cast<void*>(static_cast<intptr_t>(iSex)), nullptr, afReturnValue
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iSex
        )) {
        return false;
    }
    return Script::GetIsSexConditionFunction(
        pPlayer, reinterpret_cast<void*>(static_cast<intptr_t>(iSex)), nullptr, afReturnValue
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &ucType
        )) {
        return false;
    }
    return Script::GetIsUsedItemTypeConditionFunction(
        apRefObject, reinterpret_cast<void*>(static_cast<uintptr_t>(ucType)), nullptr, afReturnValue
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
    // The Music parameter (SCRIPT_PARAM_MUSIC) parses to a form pointer: 8 bytes on x64.
    BGSMusicType* pMusicType = nullptr;
    if (Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pMusicType
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pObject,
            &iCount, &iDistance, &iDirection
        )) {
        return false;
    }
    TESObjectREFR* pPlaced = Script::PlaceAtMeFunctionBase(
        apRefObject, static_cast<TESObject*>(pObject), iCount, iDistance, iDirection, 1.0f
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
    double v10;
    alignas(16) NiPoint3 v44;
    NiPoint3 v45;
    NiMatrix3 rotMatrix;
    alignas(16) NiPoint3 pos;
    NiPoint3 rotation;
    int32_t v64;
    int32_t* v59 = &v64;
    TESObjectREFR* pplacedref = nullptr;
    if (apPlaceAtRef && apObject) {
        alignas(16) NiPoint3 pointArray[9];
        rotation = apPlaceAtRef->GetAngleOnReference();
        pos = apPlaceAtRef->GetLocationOnReference();
        if (aiDistance) {
            NiAVObject* obj3D = (NiAVObject*)apPlaceAtRef->Get3D();
            if (obj3D) {
                alignas(16) NiPoint3 moveVec(0.0f, 0.0f, 0.0f);
                rotMatrix = (*&obj3D->m_kLocal).m_Rotate;
                pos = obj3D->GetTranslate();
                if (aiDirection < 2 || aiDirection > 3) {
                    moveVec.x = rotMatrix.GetEntry(0, 1u);
                    moveVec.y = rotMatrix.GetEntry(1u, 1u);
                    moveVec.z = rotMatrix.GetEntry(2u, 1u);
                }
                else {
                    moveVec.x = rotMatrix.GetEntry(0, 0);
                    moveVec.y = rotMatrix.GetEntry(1u, 0);
                    moveVec.z = rotMatrix.GetEntry(2u, 0);
                }
                moveVec.operator*=(static_cast<float>(aiDistance));
                if (aiDirection <= 0 || aiDirection > 2) {
                    pos.operator+=(moveVec);
                }
                else {
                    pos.operator-=(moveVec);
                }
            }
        }
        rt::eh_vector_constructor_iterator(pointArray, sizeof(NiPoint3), 9, &emptyPointConstructor);
        pointArray[0] = pos;
        int32_t index = 1;
        float ang = RandomFloat0To1() * 0.7853981852531433;
        for (int32_t i = 0; i < 8; ++i) {
            float X_4 = NiSin(ang);
            float X = NiCos(ang);
            NiPoint3* v6 = ::new (&v45) NiPoint3(X, X_4, 0.0f);
            NiPoint3* v8 = ::new (&v44) NiPoint3(pos + *v6 * 100.0f);
            pointArray[index++] = *v8;
            ang = ang + 0.7853981852531433;
        }
        if (apObject->GetFormType() == LVLC_ID || apObject->GetFormType() == LVLN_ID) {
            uint32_t icalclevel = apPlaceAtRef->GetCalcLevel(false);
            alignas(16) TESContainer tempcont;
            // Casts to the leveled creature or the leveled character by form type; both reach their TESLeveledList base
            // without a null test.
            if (apObject->GetFormType() == LVLC_ID) {
                static_cast<TESLevCreature*>(apObject)->CalculateCurrentFormList(icalclevel, aiCount, &tempcont, 0);
            }
            else {
                static_cast<TESLevCharacter*>(apObject)->CalculateCurrentFormList(icalclevel, aiCount, &tempcont, 0);
            }
            for (BSSimpleList<ContainerObject*>* pcontlist = &tempcont.objectList; pcontlist && !pcontlist->IsEmpty();
                 pcontlist = pcontlist->m_pkNext) {
                ContainerObject* pCO = pcontlist->m_item;
                if ((pCO) && (pCO->pObj)) {
                    while (pCO->i_count > 0) {
                        TESWorldSpace* X_4a = apPlaceAtRef->GetWorldSpace();
                        TESObjectCELL* Xa = apPlaceAtRef->GetParentCell();
                        pplacedref = pDataHandler->CreateReferenceAtLocation(
                            pCO->pObj, pos, rotation, Xa, X_4a, nullptr, nullptr, nullptr
                        );
                        if ((pplacedref) && (pplacedref->IsActor())) {
                            Actor* pActor = static_cast<Actor*>(pplacedref);
                            pActor->SetAlpha(0.0f);
                            pActor->FadeIn();
                        }

                        --pCO->i_count;
                    }
                }
            }
        }
        else {
            if (apObject->IsBoundObject()) {
                TESBoundObject* pboundobj = static_cast<TESBoundObject*>(apObject);
                TESHealthForm* phealthform = TESHealthForm::GetFormAsHealthForm(apObject);
                const NiPoint3* v9 = &apPlaceAtRef->GetLocationOnReference();
                bool v34 = !TES::pInstance->IsCoordInLoadedArea(*v9);
                index = 0;
                for (int32_t j = 0; j < aiCount; ++j) {
                    if (index && (!v34 && TES::pInstance->IsCoordInLoadedArea(pointArray[index]))) {
                        bhkPickData v32;
                        CFilter fltr(0u);
                        fltr.SetLayer(L_PATHINGPICK);
                        fltr.SetGroup(3u);
                        v32.SetFilter(fltr);
                        alignas(16) NiPoint3 from = apPlaceAtRef->GetLocationOnReference();
                        from.z = from.z + 64.0;
                        NiPoint3 to = pointArray[index];
                        v10 = to.z + 64.0;
                        to.z = v10;
                        v32.NiSetFrom(from);
                        v32.NiSetTo(to);
                        TES::pInstance->Pick(v32);
                        hkBool v28 = v32.hasHit();
                        if (v28.operator bool()) {
                            NiPoint3 delta = to - from;
                            delta.Unitize();
                            pos = to - delta * pboundobj->GetBoundSize();
                        }
                        else {
                            pos = to;
                        }
                        pos.z = (&apPlaceAtRef->GetLocationOnReference())->z;
                    }
                    else {
                        pos = apPlaceAtRef->GetLocationOnReference();
                    }
                    TESWorldSpace* X_4b = apPlaceAtRef->GetWorldSpace();
                    TESObjectCELL* Xb = apPlaceAtRef->GetParentCell();
                    pplacedref = pDataHandler->CreateReferenceAtLocation(
                        pboundobj, pos, rotation, Xb, X_4b, nullptr, nullptr, nullptr
                    );
                    if (!pplacedref) {
                        return nullptr;
                    }
                    if (pplacedref->IsActor()) {
                        Actor* pActor_2 = static_cast<Actor*>(pplacedref);
                        pActor_2->SetAlpha(0.0f);
                        pActor_2->FadeIn();
                    }
                    if (phealthform) {
                        float fhealth = static_cast<float>(phealthform->GetFormHealth());
                        float v23 = fhealth * afHealthPerc;
                        pplacedref->SetHealth(v23);
                    }
                    if (++index == 9) {
                        index = 0;
                    }
                }
            }
        }
    }
    return pplacedref;
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iOn
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iEnable
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iMenuMode
        )) {
        return false;
    }
    // The condition function's parameter slots are untyped: the menu mode goes in as the slot's integer value.
    return Script::GetMenuModeConditionFunction(
        nullptr, reinterpret_cast<void*>(static_cast<intptr_t>(iMenuMode)), nullptr, afReturnValue
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
            TESTerrainLODManager::UpdateGrass(pCamera->GetWorldTranslate(), NiPoint3_ZERO, 0.0f);
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fValue
        )) {
        return false;
    }
    Interface::PrintLine("Value = %0.4f", static_cast<double>(fValue));
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
            apParameters, apCompiledParams, *auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &eStage
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

char Script::ScriptEffectStart(
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
    if (apScriptLocals != nullptr && apScriptLocals->m_pScriptEffectData != nullptr &&
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fMainFOV,
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pSound,
            &iSystemSound
        )) {
        uint32_t uiFlags =
            iSystemSound <= 0 ? BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN : BSGameSound::SOUND_FLAG_SYSTEM_SOUND;
        uiFlags |= BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IS_2D;
        uint32_t uiFormID = pSound->GetFormID();
        BSAudio* pAudio = &BSAudio::QInstance();
        kHandle = pAudio->GetSoundHandleByNumericID(uiFormID, uiFlags);
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fNumber
        )) {
        return false;
    }
    float fTNumber = 0.0f;
    float fMNumber = 0.0f;
    BGSSceneInfo::CollectSceneDataForLoadedArea(fTNumber, fMNumber, true, false);
    fMNumber = static_cast<float>(fMNumber / 1048576.0);
    Interface::PrintLine("M# for loaded area = %.0f MB", static_cast<double>(fMNumber));
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iVisible
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iNoAvoidance
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iMax
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iComplex
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pMessage
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
        if (GetThreadSystemOutput()) {
            const char* pState = bhkRagdollController::QGrabIK() ? "On" : "Off";
            Interface::PrintLine("GrabIK %s", pState);
        }
        return true;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && pActor->pRagdollController) {
        pActor->pRagdollController->SetGrabIKEnable(!pActor->pRagdollController->QGrabIKEnable());
        if (GetThreadSystemOutput()) {
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
        if (GetThreadSystemOutput()) {
            const char* pState = bhkRagdollController::QLookIK() ? "On" : "Off";
            Interface::PrintLine("LookIK %s", pState);
        }
        return true;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor && pActor->pRagdollController) {
        pActor->pRagdollController->SetLookIKEnable(!pActor->pRagdollController->QLookIKEnable());
        if (GetThreadSystemOutput()) {
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
    if (GetThreadSystemOutput()) {
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fScale
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fMult,
            &iIgnoreMult
        )) {
        return false;
    }
    appTimer.SetGlobalTimeMultiplier(fMult, true);
    bool bIgnore = iIgnoreMult != 0;
    BSAudioManager::QInstance().SetIgnoreGlobalTimeMultiplier(bIgnore);
    if (GetThreadSystemOutput()) {
        Interface::PrintLine("Global Time Multiplier >> '%0.2f'", static_cast<double>(fMult));
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pSound
        )) {
        if (apRefObject != nullptr && apRefObject->Get3D() != nullptr) {
            uint32_t uiFormID = pSound->GetFormID();
            BSAudio* pAudio = &BSAudio::QInstance();
            kHandle = pAudio->GetSoundHandleByNumericID(
                uiFormID, BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT
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
            Interface::OPEN_HACKING, apRefObject, nullptr, nullptr, Interface::CMM_STANDARD, false
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &uiCount
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
        Interface::OPEN_PLAYER_NAME_ENTRY, pPlayer, nullptr, nullptr, Interface::CMM_STANDARD, false
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
    uint8_t v9 = (uint8_t)(!BSShaderManager::GetVisibleShadowVolumes());
    BSShaderManager::SetVisibleShadowVolumes(v9);
    return true;
}

void Script::HandleMessageMenu() {
    Script::m_cButtonValue = (int8_t)((int32_t)Interface::GetMessageMenuResult());
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
    int32_t* v8;
    if ((!apRefObject) || apRefObject->TESForm::GetTemporary()) {
        if (!apScript) {
            return false;
        }
        v8 = reinterpret_cast<int32_t*>(static_cast<uintptr_t>(apScript->TESForm::GetFormID()));
    }
    else {
        v8 = reinterpret_cast<int32_t*>(static_cast<uintptr_t>(apRefObject->TESForm::GetFormID()));
    }
    if ((((int32_t)Script::m_cButtonValue) > (-1)) &&
        (v8 == reinterpret_cast<int32_t*>(static_cast<uintptr_t>(Script::m_iButtonOwner)))) {
        afReturnValue = (double)Script::m_cButtonValue;
        Script::m_cButtonValue = (-1);
        Script::m_iButtonOwner = 0;
    }
    else {
        afReturnValue = (-1.0);
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
    Actor* v6 = dynamic_cast<Actor*>(apRefObject);
    if (!v6) {
        return true;
    }
    void* charController = v6->MobileObject::GetCharController();
    if (!charController) {
        return true;
    }
    int32_t shapeType = (int32_t)static_cast<bhkCharacterController*>(charController)->QShapeType();
    if (shapeType == 0) {
        static_cast<bhkCharacterController*>(charController)->SetShapeType(bhkCharacterController::ST_CONVEX);
    }
    else {
        if (shapeType == 1) {
            shapeType = 0;
        }
        static_cast<bhkCharacterController*>(charController)
            ->SetShapeType(static_cast<bhkCharacterController::SHAPE_TYPES>(shapeType));
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
    Animation* v3 = apRefObject->GetAnimation();
    v3->SkipUpdate(AGS_WHOLE_BODY);
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
    ExtraDataList* extraDataList = apRefObject->TESObjectREFR::GetExtra();
    extraDataList->RemoveSavedAnimation();
    ExtraDataList* v4 = apRefObject->TESObjectREFR::GetExtra();
    v4->RemoveSavedHavokData();
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
    TESWorldSpace* worldSpace;
    TESWorldSpace* v1;
    BGSTerrainManager* terrainManager;
    pPlayer->SetMegatonDestroyed(true);
    if (TES::pInstance->GetWorldSpace()) {
        worldSpace = TES::pInstance->GetWorldSpace();
        if (worldSpace->GetTerrainManager()) {
            v1 = TES::pInstance->GetWorldSpace();
            terrainManager = v1->GetTerrainManager();
            terrainManager->TriggerApocalypse();
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
    int8_t isVerboseMessages = (int8_t)CombatController::GetShowVerboseMessages();
    CombatController::SetShowVerboseMessages(isVerboseMessages == 0);
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
    int8_t demigodeMode = (int8_t)PlayerCharacter::IsDemigodMode();
    PlayerCharacter::SetDemigodMode(demigodeMode == 0);
    PlayerCharacter::SetGodMode(demigodeMode == 0);
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

bool Script::SetAllReachable(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t a8 = 0;
    return (bool)((uint8_t)Script::ParseParameters(
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
    ));
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
    if ((apScriptLocals) && (apScriptLocals->m_pScriptEffectData)) {
        afReturnValue = (double)apScriptLocals->m_pScriptEffectData->m_fSecondsElapsed;
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
    return (bool)((uint8_t)Script::GetAlarmedConditionFunction(
        static_cast<Actor*>(apRefObject), nullptr, nullptr, afReturnValue
    ));
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
    afReturnValue = (double)pPlayer->GetSleepHours();
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
    int8_t noRumors = Script::GetNoRumors(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetNoRumors >> %0.2f", afReturnValue);
    }
    return (bool)noRumors;
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
    int8_t v8 = Script::GetWhichServiceConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetWhichSerivce >> %0.2f", afReturnValue);
    }
    return (bool)v8;
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
    int8_t isRidingHorse = Script::IsActorRidingHorseConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Is actor riding horse >> %0.2f", afReturnValue);
    }
    return (bool)isRidingHorse;
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
    int8_t v8 = Script::IsInDangerousWaterConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Actor is in lava >> %0.2f", afReturnValue);
    }
    return (bool)v8;
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
    double v8;
    if (FalloutRadio::GetBroadcastState(apRefObject)) {
        v8 = 1.0;
    }
    else {
        v8 = 0.0;
    }
    afReturnValue = v8;
    if (Script::GetThreadSystemOutput()) {
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
    int32_t a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    FalloutRadio::SetBroadcastState(apRefObject, a8 > 0);
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
    TESObjectCELL* parentCell;
    if (pPlayer->TESObjectREFR::GetParentCell()) {
        parentCell = pPlayer->TESObjectREFR::GetParentCell();
        parentCell->ClearLocationMarkers();
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
    TESObjectREFR* grabObject;
    afReturnValue = 0.0;
    uint32_t v9 = 0;
    if (pPlayer->GetGrabObject()) {
        grabObject = pPlayer->GetGrabObject();
        v9 = grabObject->TESForm::GetFormID();
        Script::PutNumericIDInDouble(v9, afReturnValue);
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetPlayerGrabbedRef >> (%08x)", v9);
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
    return Script::GetRadiationLevel(apRefObject, nullptr, nullptr, afReturnValue);
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
    FOPipboyManager* pipboyManager = Interface::GetPipboy();
    if (pipboyManager) {
        pipboyManager->SetPipBoyManagerReset(true);
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
    ComputersMenu* menu;
    TileMenu* menuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(1057));
    if (menuByClass) {
        menu = static_cast<ComputersMenu*>(menuByClass->Tile::GetMenu());
    }
    else {
        menu = nullptr;
    }
    if (menu) {
        menu->DisplayPreviousTerminal();
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
    return (bool)((int8_t)Script::GetCurrentTimeConditionFunction(nullptr, nullptr, nullptr, afReturnValue));
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
    return (bool)((int8_t)Script::GetUnconsciousConditionFunction(apRefObject, nullptr, nullptr, afReturnValue));
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
    NiUpdateData a2;
    NiNode* v9;
    NiNode* apNode;
    if ((apRefObject) && (apRefObject->Get3D())) {
        v9 = apRefObject->Get3D();
        apNode = nullptr;
        if (v9) {
            apNode = v9->IsNode();
        }
        afReturnValue = (double)TESObjectREFR::AddAddonNodes(apNode);
        if (0.0 != afReturnValue) {
            apNode->UpdateProperties();
            a2.~NiUpdateData();
            ::new (&a2) NiUpdateData(0.0, true, false);
            apNode->Update(a2);
        }
        if (Script::GetThreadSystemOutput()) {
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
    BSAudio* v10;
    BSAudio* v11;
    BSAudio* v12;
    BSAudio* v13;
    int8_t source[512];
    int32_t v16 = 0;
    int32_t bFadeAudio = 1;
    int32_t v17 = 1;
    int32_t v15 = 1;
    BSAudio* singleton = &BSAudio::QInstance();
    uint8_t bMultiThreadedAudio = (uint8_t)singleton->bMultiThread;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, source, &v16,
            &bFadeAudio, &v17, &v15
        )) {
        return false;
    }
    if (bFadeAudio) {
        v10 = &BSAudio::QInstance();
        v10->Mute(0, 0x1770u);
    }
    else {
        if (!bMultiThreadedAudio) {
            v11 = &BSAudio::QInstance();
            v11->SetMultiThreaded(true);
        }
    }
    pIOManager->Pause();
    pMoviePlayer->Play(
        static_cast<const char*>(static_cast<const void*>(source)), v16 != 0, false, false, false, v15 != 0, false,
        bFadeAudio != 0, v17 != 0
    );
    pIOManager->Resume();
    if (bFadeAudio) {
        v12 = &BSAudio::QInstance();
        v12->UnMute(0);
    }
    else {
        if (!bMultiThreadedAudio) {
            v13 = &BSAudio::QInstance();
            v13->SetMultiThreaded(false);
        }
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
    PlayerCharacter* a1;
    if (apRefObject) {
        a1 = static_cast<PlayerCharacter*>(dynamic_cast<Actor*>(apRefObject));
        if (a1) {
            (&ReferenceProcessLists)->StopCombatandAlarmonActor(a1, false);
            a1->StopCombat(nullptr);
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
    NiMatrix3* orientation;
    NiMatrix3 result;
    NiUpdateData a2;
    NiNode* v6;
    float afX;
    if (apRefObject) {
        afX = (float)(afValue * 0.01745329238474369);
        switch (acAxis) {
            case 88: {
                apRefObject->SetAngleOnReferenceX(afX);
                break;
            }
            case 89: {
                apRefObject->SetAngleOnReferenceY(afX);
                break;
            }
            case 90: {
                apRefObject->SetAngleOnReferenceZ(afX);
                break;
            }
        }
        v6 = apRefObject->Get3D();
        if (v6) {
            orientation = &(result = apRefObject->GetOrientation());
            v6->SetRotate(*orientation);
            bhkNiCollisionObject::ResetSim(v6, true);
            if (!apRefObject->GetAnimation()) {
                a2.~NiUpdateData();
                ::new (&a2) NiUpdateData(0.0, false, false);
                v6->Update(a2);
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
    return (bool)((int8_t)Script::GetGroupTargetCountConditionFunction(apRefObject, nullptr, nullptr, afReturnValue));
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
    TESFile* pFile;
    Sky* pSky = Sky::GetInstance();
    TESClimate* pClimate = pSky->Sky::GetCurrentClimate();
    TESFile* file = pClimate->GetFile((-1));
    if (file) {
        pFile = file->GetThreadSafeFile();
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
    TESFile* listFile;
    int8_t a8[516];
    a8[0] = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    if (a8[0]) {
        listFile = pDataHandler->GetListFile(static_cast<const char*>(static_cast<const void*>(a8)));
        if (listFile) {
            pDataHandler->SetIsClosingFile(true);
            listFile->CloseTES();
            pDataHandler->SetIsClosingFile(false);
            if (Script::GetThreadSystemOutput()) {
                Interface::PrintLine("Closed file '%s'.", a8);
            }
        }
        else {
            if (Script::GetThreadSystemOutput()) {
                Interface::PrintLine("ERR: Could not find file '%s'.", a8);
            }
        }
    }
    else {
        if (Script::GetThreadSystemOutput()) {
            Interface::PrintLine("ERR: No Filename.");
        }
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
    CombatController* v3;
    ExtraDataList* extra;
    if ((apRefObject) && (apRefObject->IsActor())) {
        if ((static_cast<Actor*>(apRefObject)->bInCombat) &&
            (static_cast<Actor*>(apRefObject)->GetCombatController())) {
            v3 = static_cast<Actor*>(apRefObject)->GetCombatController();
            v3->ForceStopCombat();
        }

        if (apRefObject->TESObjectREFR::GetExtra()) {
            extra = apRefObject->TESObjectREFR::GetExtra();
            extra->RemovePlayerCrimeListExtra(static_cast<Actor*>(apRefObject));
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
    NiNode* v7;
    NiNode* v9;
    if ((apRefObject) && (apRefObject->Get3D())) {
        v9 = apRefObject->Get3D();
        if (v9) {
            v7 = v9->IsNode();
            afReturnValue = (double)TESObjectREFR::RemoveAddonNodes(v7);
        }
        if (Script::GetThreadSystemOutput()) {
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
    int8_t a8[260];
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    XGamerProfile* singleton = XGamerProfile::Instance();
    singleton->RevokeCode(static_cast<const char*>(static_cast<const void*>(a8)));
    return true;
}

void Script::SavePendingReferences(BGSSaveGameBuffer* a1) {
    Script::ScriptRefListCrit.Enter(nullptr);
    uint32_t aiVariableValue = 0;
    uint32_t aiPosition = a1->StartVariableSizedValue();
    for (BSSimpleList<TESObjectREFR*>* ecx0 = &Script::ReferencesToEnable; ecx0; ecx0 = ecx0->m_pkNext) {
        TESForm* apForm = ecx0->m_item;
        if (apForm) {
            a1->SaveFormID(apForm, static_cast<uint32_t>(0));
            ++aiVariableValue;
        }
    }
    a1->SaveVariableSizedValue(aiVariableValue, aiPosition);
    uint32_t v12 = 0;
    uint32_t started = a1->StartVariableSizedValue();
    for (BSSimpleList<TESObjectREFR*>* i = &Script::ReferencesToDisable; i; i = i->m_pkNext) {
        TESForm* v9 = i->m_item;
        if (v9) {
            a1->SaveFormID(v9, static_cast<uint32_t>(0));
            ++v12;
        }
    }
    a1->SaveVariableSizedValue(v12, started);
    uint32_t v8 = 0;
    uint32_t v7 = a1->StartVariableSizedValue();
    for (BSSimpleList<TESObjectREFR*>* j = &Script::ReferencesToDelete; j; j = j->m_pkNext) {
        TESForm* v5 = j->m_item;
        if (v5) {
            a1->SaveFormID(v5, static_cast<uint32_t>(0));
            ++v8;
        }
    }
    a1->SaveVariableSizedValue(v8, v7);
    uint32_t v4 = 0;
    uint32_t v3 = a1->StartVariableSizedValue();
    for (BSSimpleList<TESObjectREFR*>* k = &Script::ReferencesToFadeDisable; k; k = k->m_pkNext) {
        TESForm* v1 = k->m_item;
        if (v1) {
            a1->SaveFormID(v1, static_cast<uint32_t>(0));
            ++v4;
        }
    }
    a1->SaveVariableSizedValue(v4, v3);
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
    const char* v1;
    BSShaderManager::bOcclusionQuery = !BSShaderManager::bOcclusionQuery;
    if (BSShaderManager::bOcclusionQuery) {
        v1 = "on";
    }
    else {
        v1 = "off";
    }
    Interface::PrintLine("Occlusion Query : %s", v1);
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
    int32_t iIndex;
    iIndex = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iIndex
        )) {
        return false;
    }
    if ((GetThreadTLSData()->bSystemOutput) && (apRefObject) &&
        (apRefObject->IsActor() && iIndex >= 0 && iIndex < ActorValue::ACTOR_VALUE_COUNT)) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        ActorValue::Index eIndex = static_cast<ActorValue::Index>(iIndex);
        // The results kept are rounded to float; the modifiers are passed on unrounded, as doubles.
        float fComputedBase = pActor->GetBaseActorFloatValue(eIndex);
        const char* pFullName = pActor->GetFullName();
        Interface::PrintLine("GetActorValueInfo: %s on %s", ActorValue::GetActorValueScriptName(eIndex), pFullName);
        Interface::PrintLine(
            "...Current Value: %.2f Computed Base: %.2f ", pActor->GetActorFloatValue(eIndex), fComputedBase
        );
        float fReferenceBase = pActor->GetActorBase()->GetActorFloatValue(eIndex);
        float fDerivedValue = 0.0f;
        bool bIsDerived =
            ActorValue::GetDerivedActorValue(static_cast<ActorValueOwner*>(pActor), eIndex, fDerivedValue);
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
            if ((!pActor->IsPc() && !pActor->IsAutoCalc() &&
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
            "...Modifiers: Temp: %.2f Perm: %.2f Damage: %.2f", pActor->GetTemporaryModifier(eIndex), dPermanent,
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
    float fPos;
    fPos = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &cAxis, &fPos
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

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("%s cannot be moved.", apRefObject->GetFormEditorID());
    }
    uint32_t CastingType = apScript->iFormID;
    if (!pDataHandler->IsFormIDCreated(CastingType)) {
        int32_t afValue_4 = apRefObject->iFormID;
        const char* afValue = apRefObject->GetFormEditorID();
        int32_t v10 = apScript->iFormID;
        Warning(
            "SCRIPTS: Script '%s' (%08X) is attempting to move reference '%s' (%08X), but it cannot be moved",
            apScript->GetFormEditorID(), v10, afValue, afValue_4
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
        MobileObject* pmob = dynamic_cast<MobileObject*>(apRefObject);
        if (pmob) {
            bhkCharacterController* pcon = pmob->GetCharController();
            if ((pcon) && (!pcon->QIsFlying())) {
                pcon->SetPosition(pos);
            }
        }
        NiAVObject* obj3D = apRefObject->Get3D();
        if (obj3D) {
            obj3D->SetTranslate(pos);
            bhkNiCollisionObject::ResetSim(obj3D, true);
            NiUpdateData UpdateData(0.0f, false, false);
            obj3D->Update(UpdateData);
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
    char v39 = 1;
    if (apRefObject) {
        if (apRefObject->GetScriptPointer()) {
            Script* ownerScript = apRefObject->GetScriptPointer();
            ScriptLocals* ownerLocals = apRefObject->GetScriptVariables();
            if (ownerScript->GetVariableList()->IsEmpty()) {
                Interface::PrintLine("No variables in script %s", ownerScript->GetFormEditorID());
            }
            else {
                BSSimpleList<ScriptVariable*>* pList = ownerScript->GetVariableList();
                while (ownerLocals && (pList && pList->m_item)) {
                    ScriptVariable* pVar = pList->m_item;
                    pList = pList->m_pkNext;
                    if (ownerLocals->IsReferenceVariable(pVar->data.uiID)) {
                        uint32_t iid = 0;
                        double fval = ownerLocals->GetVariable(pVar->data.uiID, nullptr);
                        Script::GetNumericIDFromDouble(iid, fval);
                        uint32_t v13 = iid;
                        char* v6 = pVar->cName.pString;
                        Interface::PrintLine("%s->%s = (%08X)", ownerScript->GetFormEditorID(), v6, v13);
                    }
                    else {
                        if (pVar->data.bIsInteger) {
                            int32_t Variable = static_cast<int32_t>(ownerLocals->GetVariable(pVar->data.uiID, nullptr));
                            char* v7 = pVar->cName.pString;
                            Interface::PrintLine("%s->%s = %d", ownerScript->GetFormEditorID(), v7, Variable);
                        }
                        else {
                            double v8 = ownerLocals->GetVariable(pVar->data.uiID, nullptr);
                            char* v4 = pVar->cName.pString;
                            Interface::PrintLine("%s->%s = %0.4f", ownerScript->GetFormEditorID(), v4, v8);
                        }
                    }
                }
            }
            v39 = 0;
        }
        if (apRefObject->HasContainer()) {
            InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(apRefObject);
            int32_t iCount = pinventory->GetCount(true);
            for (int32_t i = 0; i < iCount; ++i) {
                ItemChange* pitem = pinventory->GetItembyIndex(i);
                if ((pitem) && (pitem->GetScript())) {
                    Script* ownerScript_2 = pitem->GetScript();
                    ScriptLocals* ownerLocals_2 = pitem->GetScriptLocals();
                    if (ownerScript_2->GetVariableList()->IsEmpty()) {
                        Interface::PrintLine("No variables in script %s", ownerScript_2->GetFormEditorID());
                    }
                    else {
                        BSSimpleList<ScriptVariable*>* pList_2 = ownerScript_2->GetVariableList();
                        while (ownerLocals_2 && (pList_2 && pList_2->m_item)) {
                            ScriptVariable* pVar_2 = pList_2->m_item;
                            pList_2 = pList_2->m_pkNext;
                            if (ownerLocals_2->IsReferenceVariable(pVar_2->data.uiID)) {
                                uint32_t iid_2 = 0;
                                double fval_2 = ownerLocals_2->GetVariable(pVar_2->data.uiID, nullptr);
                                Script::GetNumericIDFromDouble(iid_2, fval_2);
                                uint32_t v15 = iid_2;
                                char* v9 = pVar_2->cName.pString;
                                Interface::PrintLine("%s->%s = (%08X)", ownerScript_2->GetFormEditorID(), v9, v15);
                            }
                            else {
                                if (pVar_2->data.bIsInteger) {
                                    int32_t v16 =
                                        static_cast<int32_t>(ownerLocals_2->GetVariable(pVar_2->data.uiID, nullptr));
                                    char* v10 = pVar_2->cName.pString;
                                    Interface::PrintLine("%s->%s = %d", ownerScript_2->GetFormEditorID(), v10, v16);
                                }
                                else {
                                    double v11 = ownerLocals_2->GetVariable(pVar_2->data.uiID, nullptr);
                                    char* v5 = pVar_2->cName.pString;
                                    Interface::PrintLine("%s->%s = %0.4f", ownerScript_2->GetFormEditorID(), v5, v11);
                                }
                            }
                        }
                    }
                    v39 = 0;
                }
            }
        }
    }
    if (v39) {
        BSSimpleList<TESGlobal*>* pList_3 = pDataHandler->GetGlobalList();
        if ((pList_3) && (!pList_3->IsEmpty())) {
            Interface::PrintLine("--- Global Variables -----------------------------");
            while (pList_3) {
                TESGlobal* pGlobal = pList_3->m_item;
                if (pGlobal) {
                    char Type = pGlobal->GetType();
                    switch (Type) {
                        case 102: {
                            double v12 = pGlobal->fValue;
                            Interface::PrintLine("%s = %0.4f", pGlobal->GetFormEditorID(), v12);
                            break;
                        }
                        case 108: {
                            int32_t v18 = static_cast<int32_t>(pGlobal->fValue);
                            Interface::PrintLine("%s = %d", pGlobal->GetFormEditorID(), v18);
                            break;
                        }
                        case 115: {
                            int32_t v17 = static_cast<int16_t>(static_cast<int32_t>(pGlobal->fValue));
                            Interface::PrintLine("%s = %d", pGlobal->GetFormEditorID(), v17);
                            break;
                        }
                    }
                }
                pList_3 = pList_3->m_pkNext;
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
    TESObjectREFR* pLocationTarget;
    TESObjectREFR* ptarget;
    TESTopic* ptopic;
    int32_t bHeadTrack;
    int32_t bMove;
    TESObjectREFR* pLocationSpeaker;
    ptarget = nullptr;
    ptopic = nullptr;
    pLocationSpeaker = nullptr;
    pLocationTarget = nullptr;
    bHeadTrack = 1;
    bMove = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &ptarget,
            &ptopic, &pLocationSpeaker, &pLocationTarget, &bHeadTrack, &bMove
        )) {
        return false;
    }
    if ((apRefObject) && (ptarget) && (!apRefObject->GetDisabled() && !ptarget->GetDisabled())) {
        PackageLocation* ploc1 = nullptr;
        PackageLocation* ploc2 = nullptr;
        PackageLocation tempFirstLoc;
        PackageLocation tempSecondLoc;
        if (pLocationSpeaker) {
            ploc1 = &tempFirstLoc;
            tempFirstLoc.SetLocReference(pLocationSpeaker);
        }
        if (pLocationTarget) {
            ploc2 = &tempSecondLoc;
            tempSecondLoc.SetLocReference(pLocationTarget);
        }
        MobileObject* pactor = nullptr;
        if (apRefObject->IsActor()) {
            pactor = static_cast<MobileObject*>(apRefObject);
        }
        MobileObject* ptargetchar = nullptr;
        if (ptarget->IsActor()) {
            ptargetchar = static_cast<MobileObject*>(ptarget);
        }
        else {
            ptargetchar = ptarget->AddTalkingActivatorMobileObjectREFR();
        }
        if (!pactor ||
            (!ptargetchar ||
             (pactor->GetDisabled() ||
              (ptargetchar->GetDisabled() ||
               (pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
                (Script::IsPendingFadeDisabledReference(pactor) ||
                 Script::IsPendingFadeDisabledReference(ptargetchar))))))) {
            apRefObject->Activate(ptarget, false, nullptr, 1);
        }
        else {
            if (pactor == pPlayer) {
                ptargetchar->Activate(pPlayer, false, nullptr, 1);
            }
            else {
                if ((pactor->pCurrentProcess) && (ptargetchar->pCurrentProcess)) {
                    pactor->InitiateDialogue(
                        ptargetchar, ploc1, ploc2, bHeadTrack == 1, bMove == 1, false, ptopic, false, false
                    );
                    DialoguePackage* ppack = static_cast<DialoguePackage*>(pactor->GetCurrentPackage());
                    if ((ppack) && (ppack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE)) {
                        pPlayer->pAIConversationRunning = ppack;
                    }
                }
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
    bool bwason = Script::GetProcessScripts();
    Script::SetProcessScripts(false);
    for (BSSimpleList<TESQuest*>* plist = pDataHandler->GetQuestList(); plist && !plist->IsEmpty();
         plist = plist->m_pkNext) {
        TESQuest* pquest = plist->m_item;
        for (BSSimpleList<TESQuestStage*>* pstagelist = &pquest->m_listStages; pstagelist && !pstagelist->IsEmpty();
             pstagelist = pstagelist->m_pkNext) {
            TESQuestStage** v8 = &pstagelist->m_item;
            (*v8)->SetDone(pquest, true);
        }
        for (BSSimpleList<BGSQuestObjective*>* pobjlist = pquest->GetObjectiveList(); pobjlist && !pobjlist->IsEmpty();
             pobjlist = pobjlist->m_pkNext) {
            BGSQuestObjective* pobj = pobjlist->m_item;
            pobj->SetState(QOS_DISPLAYED);
            pobj->SetState(QOS_COMPLETED_DISPLAYED);
        }
    }
    Script::SetProcessScripts(bwason);
    if (GetThreadTLSData()->bSystemOutput) {
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pQuest,
            &uiIndex, &iCompleted
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
        // Reads the form ID and calls the editor name getter through the null quest, deliberately, as the game does.
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
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8 >= 0x10000) {
        return false;
    }
    BSPSysMultiTargetEmitterCtlr::SetMaxParticlesPerEmitter(a8);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetDistanceConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetInZoneConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    int32_t iCount;
    int32_t iSilent;
    TESForm* pWeap;
    pWeap = nullptr;
    iCount = 0;
    iSilent = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pWeap,
            &iCount, &iSilent
        )) {
        return false;
    }
    if (apRefObject) {
        uint32_t icalclevel = apRefObject->GetCalcLevel(false);
        TESContainer tempcont;
        TESLevItem* plevitem = nullptr;
        BGSListForm* pformlist = nullptr;
        TESBoundObject* pobject = nullptr;
        bool bWeap = false;
        int32_t FormType = pWeap->GetFormType();
        if (FormType != 40) {
            if (FormType == 52) {
                plevitem = static_cast<TESLevItem*>(pWeap);
            }
            else {
                if (FormType != 85) {
                LABEL_11:;
                    if (pWeap->IsBoundObject()) {
                        pobject = static_cast<TESBoundObject*>(pWeap);
                    }
                    goto LABEL_13;
                }
                pformlist = static_cast<BGSListForm*>(pWeap);
            }
        LABEL_13:;
            if (plevitem) {
                plevitem->CalculateCurrentFormList(icalclevel, iCount, &tempcont, 0);
            }
            else {
                if (pformlist && iCount) {
                    BSSimpleList<TESForm*>* plist = &pformlist->ListOfForms;
                    while (plist && !plist->IsEmpty()) {
                        TESForm* paddform = plist->m_item;
                        plist = plist->m_pkNext;
                        if (paddform->IsBoundObject()) {
                            tempcont.AddObject(static_cast<TESObject*>(paddform), iCount, nullptr);
                        }
                    }
                }
                else {
                    if (pobject && iCount) {
                        tempcont.AddObject(pobject, iCount, nullptr);
                    }
                    else {
                        Warning(
                            "SCRIPTS: AddItem in script '%s' failed to generate an item.", apScript->GetFormEditorID()
                        );
                    }
                }
            }
            if (bWeap && static_cast<TESObjectWEAP*>(pWeap)->TESObjectWEAP::GetWeaponType() < WEAPON_ONE_HAND_GRENADE) {
                tempcont.ApplyHealthPercentToContainer(0.999f);
            }
            tempcont.AddContainerToReference(apRefObject, iSilent == 1);
            if (apRefObject->IsActor() && apRefObject != pPlayer && apRefObject->IsNpc()) {
                BipedAnim* v9 = apRefObject->GetBiped();
                static_cast<TESNPC*>(apRefObject->GetObjectReference())->TESNPC::InitParts(apRefObject, v9, true);
            }
            return true;
        }
        bWeap = true;
        goto LABEL_11;
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
    int32_t icount;
    int32_t isilent;
    TESForm* pformlist;
    pformlist = nullptr;
    TESBoundObject* pobject = nullptr;
    icount = 0;
    isilent = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pformlist,
            &icount, &isilent
        )) {
        return false;
    }
    if ((apRefObject) && (pformlist)) {
        BSSimpleList<TESForm*> removelist;
        if (pformlist->IsBoundObject()) {
            pobject = static_cast<TESBoundObject*>(pformlist);
        }
        if (pformlist->GetFormType() == LVLI_ID) {
            TESLevItem* pleveled = static_cast<TESLevItem*>(pformlist);
            if (pformlist) {
                InventoryChanges* pchanges = (&apRefObject->m_Extra)->GetContainerChanges();
                if (pchanges) {
                    pobject = pchanges->GetLeveledCalculatedBaseObject(pleveled);
                }
            }
        }
        if (pobject) {
            TESForm* premoveform = pobject;
            removelist.AddHead(premoveform);
        }
        else {
            if (pformlist->GetFormType() == FLST_ID) {
                TESForm* v27 = pformlist;
                BSSimpleList<TESForm*>* plist = &static_cast<BGSListForm*>(pformlist)->ListOfForms;
                while (plist && !plist->IsEmpty()) {
                    TESForm* premoveform_2 = plist->m_item;
                    plist = plist->m_pkNext;
                    if (premoveform_2->IsBoundObject()) {
                        removelist.AddHead(premoveform_2);
                    }
                }
            }
        }
        while (!removelist.IsEmpty()) {
            pobject = static_cast<TESBoundObject*>((&removelist)->m_item);
            removelist.RemoveHead();
            int32_t iremovecount = icount;
            Actor* pactor = dynamic_cast<Actor*>(apRefObject);
            if (pactor) {
                double iActualCount = static_cast<double>(iremovecount);
                Script::GetItemCountConditionFunction(apRefObject, pobject, nullptr, iActualCount);
                if (static_cast<double>(iremovecount) > iActualCount) {
                    iremovecount = static_cast<int32_t>(iActualCount);
                }
                ExtraDataList* pextra = nullptr;
                if (pactor->WearingObject(pobject)) {
                    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(apRefObject);
                    if (pinventory) {
                        pextra = pinventory->WearingObject(pobject, false);
                        if (pextra) {
                            pextra->SetCanNotWear(false);
                        }
                    }
                }
                if (iremovecount > 0) {
                    bool bworn = pactor->WearingObject(pobject);
                    if (bworn) {
                        pactor->Dispel(pobject, true);
                    }
                    pactor->RemoveItem(
                        pobject, pextra, iremovecount, false, false, nullptr, nullptr, nullptr, true, false
                    );
                    if (pactor == pPlayer) {
                        if (!isilent) {
                            BSStringT<char> pstring;
                            if (iremovecount <= 1) {
                                const char* v13 = sRemoveItemfromInventory.String();
                                const char* FullName = TESFullName::GetFullName(pobject);
                                pstring.SPrintF("%s %s", FullName, v13);
                            }
                            else {
                                const char* v12 = sRemoveItemfromInventory.String();
                                const char* time = sPlural.String();
                                const char* v9 = TESFullName::GetFullName(pobject);
                                pstring.SPrintF("%i %s%s %s", iremovecount, v9, time, v12);
                            }
                            switch (pobject->GetFormType()) {
                                case ARMO_ID: {
                                    const char* apSoundName;
                                    if (isilent) {
                                        apSoundName = nullptr;
                                    }
                                    else {
                                        apSoundName = pPlayer->GetPickUpSoundName(pobject, false, false);
                                    }
                                    Interface::ShowMessage(
                                        pstring.pString, 0,
                                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds",
                                        apSoundName, 2.0f, false
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
                                    const char* soundName;
                                    if (isilent) {
                                        soundName = nullptr;
                                    }
                                    else {
                                        soundName = pPlayer->GetPickUpSoundName(pobject, false, false);
                                    }
                                    Interface::ShowMessage(
                                        pstring.pString, 0,
                                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds", soundName,
                                        2.0f, false
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
                    else {
                        if ((bworn) && (pactor->pCurrentProcess) && (pactor->Get3D())) {
                            BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
                            CurrentAIProcess->ProcessRemoveWorn(pactor, false);
                        }
                    }
                }
            }
            else {
                apRefObject->RemoveItem(
                    pobject, nullptr, iremovecount, false, false, nullptr, nullptr, nullptr, true, false
                );
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
    int32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    ReferenceProcessLists.bShowSubtitle = a8 != 0;
    return true;
}

// KEEP: when ParseParameters fails the exe never writes apString1 ([ebp-0x11C]), yet formats it into the model and
// texture dump names (BSsprintf at 0x5B5F7A and 0x5B600C): an uninitialised read.
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
    char apString1[268];
    input[0] = 0;
    std::memset(input + 1, 0, 511);
    if (Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, input
        )) {
        BSstrcpy(apString1, 260, input);
        BSsprintf(input, 512, "MemStats%s.mhd", apString1);
    }
    else {
        BSstrcpy(input, 512, "OutputMemStats.mhd");
    }
    MemoryManagerFile file(input);
    MemoryManager* pManager = &MemoryManager::Instance();
    pManager->OutputDebugInfo(&file, false);
    BSsprintf(input, 512, "ModelDump%s.txt", apString1);
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
    BSsprintf(input, 512, "TextureDump%s.txt", apString1);
    BSFile outfile2(input, NiFile::WRITE_ONLY, 0x4000, false);
    outfile2.BSFile::Open(false, false);
    NiPointer<NiTexture> pLargestTex[2048];
    ourMemset(pLargestTex, 0, sizeof(pLargestTex));
    uint32_t ultotalMemory = 0;
    int32_t itexcount = 0;
    // The textures with renderer data, sorted by size, largest first.
    for (NiTexture* pTex = NiTexture::GetListHead(); pTex; pTex = pTex->m_pkNext) {
        ++itexcount;
        if (pTex->m_pkRendererData) {
            uint32_t itexsize = GetTextureDumpSize(pTex);
            ultotalMemory += itexsize;
            int32_t idx = -1;
            for (int32_t i = 0; i < 2048 && idx == -1; ++i) {
                if (pLargestTex[i].m_pObject) {
                    uint32_t icompsize = GetTextureDumpSize(pLargestTex[i].m_pObject);
                    if (icompsize < itexsize) {
                        idx = i;
                    }
                }
                else {
                    idx = i;
                }
            }
            if (idx != -1) {
                for (int32_t i_2 = 2047; i_2 > idx; --i_2) {
                    pLargestTex[i_2].operator=(pLargestTex[i_2 - 1]);
                }
                pLargestTex[idx].operator=(pTex);
            }
        }
    }
    for (int32_t i_3 = 0; i_3 < 2048; ++i_3) {
        if (pLargestTex[i_3].m_pObject) {
            const char* pquality;
            char Str[264];
            char buf[512];
            bool bsystemtex = false;
            if (NiObject::IsKindOf_0(&NiSourceTexture::ms_RTTI, pLargestTex[i_3].m_pObject)) {
                NiSourceTexture* psrctex = static_cast<NiSourceTexture*>(pLargestTex[i_3].m_pObject);
                const NiFixedString* pfilename = &psrctex->GetFilename();
                const char* ptexname = pfilename->GetHandleString();
                if (!ptexname || !*ptexname) {
                    ptexname = (&pLargestTex[i_3].m_pObject->m_kName)->GetHandleString();
                }
                if (ptexname && *ptexname) {
                    BSstrcpy(Str, 260, ptexname);
                }
                else {
                    BSstrcpy(Str, 260, "UNNAMED SOURCE TEXTURE");
                }
            }
            else {
                bsystemtex = true;
                const char* ptexname_2 = (&pLargestTex[i_3].m_pObject->m_kName)->GetHandleString();
                if (ptexname_2 && *ptexname_2) {
                    BSsprintf(Str, 260, "RENDERED_TEXTURE (%s)", ptexname_2);
                }
                else {
                    BSsprintf(Str, 260, "RENDERED_TEXTURE (none)");
                }
            }
            BSstrlwr(Str, 260);
            const char* pcattext = "SYSTEM";
            if (!bsystemtex) {
                if (std::strstr(Str, "\\armor\\") ||
                    (std::strstr(Str, "\\creatures\\") ||
                     (std::strstr(Str, "\\characters\\") ||
                      (std::strstr(Str, "\\pipboy3000\\") ||
                       (std::strstr(Str, "\\weapons\\") ||
                        (std::strstr(Str, "\\decals\\") || std::strstr(Str, "\\projectiles\\"))))))) {
                    pcattext = "ACTOR";
                }
                else {
                    if (std::strstr(Str, "\\fonts\\") || std::strstr(Str, "\\interface\\")) {
                        pcattext = "MENUS";
                    }
                    else {
                        if (std::strstr(Str, "\\effects\\") || std::strstr(Str, "\\gore\\")) {
                            pcattext = "EFFECTS";
                        }
                        else {
                            if (std::strstr(Str, "\\sky\\")) {
                                pcattext = "SKY";
                            }
                            else {
                                if (std::strstr(Str, "\\lod\\")) {
                                    pcattext = "LOD";
                                }
                                else {
                                    if (std::strstr(Str, "\\water\\")) {
                                        pcattext = "WATER";
                                    }
                                    else {
                                        pcattext = "MISCREF";
                                    }
                                }
                            }
                        }
                    }
                }
            }
            uint32_t itexsize_2 = GetTextureDumpSize(pLargestTex[i_3].m_pObject);
            _D3DFORMAT eFormat =
                NiDX9PixelFormat::DetermineD3DFormat(*&pLargestTex[i_3].m_pObject->m_pkRendererData->m_kPixelFormat);
            const char* pfmt = NiDX9SystemDesc::GetD3D9FormatString(eFormat);
            NiTexture* pRttiTex = pLargestTex[i_3].m_pObject;
            const char* prtti = pRttiTex->GetRTTI()->m_pcName;
            if (pLargestTex[i_3].m_pObject->QState() == PS_DEGRADED) {
                pquality = "L";
            }
            else {
                pquality = "H";
            }
            NiTexture* pHeightTex = pLargestTex[i_3].m_pObject;
            NiTexture* pWidthTex = pLargestTex[i_3].m_pObject;
            int32_t irefcount = pLargestTex[i_3].m_pObject->m_uiRefCount - 1;
            uint32_t uisizekb = itexsize_2 >> 10;
            int32_t iheight = pHeightTex->GetHeight();
            int32_t iwidth = pWidthTex->GetWidth();
            BSsprintf(
                buf, 512, "TEXTURES: %d:\t%s\t%s\t%dx%d\t%d\t%s\trefcount %d\t%s\t%s\r\n", i_3 + 1, pcattext, Str,
                iwidth, iheight, uisizekb, pfmt, irefcount, pquality, prtti
            );
            Warning(buf);
            size_t stlen = std::strlen(buf);
            outfile2.BSFile::WriteF(buf, stlen + 1);
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
    char a8[516];
    std::memset(a8, 0, 512);
    BSstrncpy(a8, "OutputMemStats.mhd", 511);
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        BSstrncpy(a8, "OutputMemStats.mhd", 511);
    }
    MemoryManagerFile result_1(a8);
    MemoryManager* v8 = &MemoryManager::Instance();
    v8->OutputDebugInfo(&result_1, true);
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
    int32_t ienabletravel;
    int32_t iturnon;
    int32_t ishowall;
    iturnon = 1;
    ienabletravel = 1;
    ishowall = 1;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iturnon,
            &ienabletravel, &ishowall
        )) {
        return false;
    }
    BSSimpleList<TESWorldSpace*>* pworldlist = pDataHandler->GetWorldSpaceList();
    while (pworldlist && !pworldlist->IsEmpty()) {
        TESWorldSpace* pworld = pworldlist->m_item;
        pworldlist = pworldlist->m_pkNext;
        BSSimpleList<TESObjectREFR*>* pmarkerlist = pworld->BuildMapMarkerList(false);
        if (pmarkerlist) {
            while (!pmarkerlist->IsEmpty()) {
                TESObjectREFR* pref = pmarkerlist->m_item;
                pmarkerlist->RemoveHead();
                MapMarkerData* pdata = pref->GetMapMarkerData();
                if (!ishowall || !pdata->GetHidden()) {
                    if (iturnon) {
                        pdata->SetIsVisible(1);
                        if (!pdata->GetTravelLoc()) {
                            pdata->SetTravelLoc(ienabletravel != 0);
                        }
                    }
                    else {
                        pdata->SetIsVisible(0);
                        pdata->SetTravelLoc(false);
                    }
                    pref->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
                }
            }
            delete pmarkerlist;
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        if (iturnon <= 0) {
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
    const char* v10;
    bool IsGodMode = PlayerCharacter::IsGodMode();
    PlayerCharacter::SetGodMode(!IsGodMode);
    if (!PlayerCharacter::IsGodMode() && PlayerCharacter::IsDemigodMode()) {
        PlayerCharacter::SetDemigodMode(false);
        Interface::PrintLine("Demigod Mode disabled");
    }
    if (PlayerCharacter::IsGodMode()) {
        v10 = "enabled.";
    }
    else {
        v10 = "disabled.";
    }
    Interface::PrintLine("God Mode %s", v10);
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
    const char* v2;
    bool UseEmotions = BSFaceGenManager::GetUseEmotions();
    BSFaceGenManager::SetUseEmotions(!UseEmotions);
    if (BSFaceGenManager::GetUseEmotions()) {
        v2 = "Enabled.";
    }
    else {
        v2 = "Disabled.";
    }
    Interface::PrintLine("NPC Facial Emotions %s", v2);
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
    TESImageSpaceModifier* a8;
    float v10;
    a8 = nullptr;
    v10 = 1.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v10
        )) {
        return false;
    }
    ImageSpaceModifierInstanceForm::Stop(a8);
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
    NiNode* pnode;
    NiPoint3 v53;
    NiPoint3 v54;
    NiPoint2 v60;
    NiPoint3 v61;
    NiPoint3 v62;
    NiPoint3 v75;
    NiPoint3 dst;
    NiPoint3 mapworldcenter;
    NiMatrix3 markrot;
    int32_t ifog;
    ifog = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &ifog
        )) {
        return false;
    }
    void* apMemory = NiMemObject::operator new(sizeof(NiNode));
    if (apMemory) {
        pnode = ::new (apMemory) NiNode(0);
    }
    else {
        pnode = nullptr;
    }
    NiPoint3 playerloc = pPlayer->GetLocationOnReference();
    playerloc.z = playerloc.z + 10.0;
    int32_t itrisize = 64;
    int32_t itileside = 16;
    int32_t itilecount = 256;
    int32_t itilesize = 4;
    int32_t iseensize = 17;
    uint32_t iarraysize = 289;
    int32_t imapside = uGridsToLoad.Int();
    int32_t imaparraysize = imapside * imapside;
    int32_t imapsize = imapside * itrisize;
    float fpossize = SeenData::GetSeenPosSize();
    for (int32_t itri = 0; itri < imaparraysize; ++itri) {
        NiTriShape* ptri;
        NiColorA* pcolor;
        NiPoint2* ptex;
        NiPoint3* pnorm;
        NiPoint3* pverts;
        int32_t x_2;
        int32_t y_2;
        // The division by the setting's value is unsigned.
        int32_t* v9 = &uGridsToLoad.Int();
        int32_t isectx = static_cast<uint32_t>(itri) % static_cast<uint32_t>(*v9);
        int32_t* v10 = &uGridsToLoad.Int();
        int32_t isecty = static_cast<uint32_t>(itri) / static_cast<uint32_t>(*v10);
        NiPointer<NiTexture> spmaptex(static_cast<NiTexture*>(nullptr));
        SeenData* pseendata = nullptr;
        TESObjectCELL* pseencell = nullptr;
        NiPoint3 sectoffset = NiPoint3_ZERO;
        if (pPlayer->GetInterior()) {
            pseencell = pPlayer->GetParentCell();
            if (pseencell) {
                NiPoint3 playerintloc = playerloc;
                pseencell->AdjustCoordForNorthRotation(playerloc, playerintloc, true);
                int32_t iseenx = (FloatToInt(playerintloc.x) - 2048) >> 12;
                int32_t iseeny = (FloatToInt(playerintloc.y) - 2048) >> 12;
                int32_t iintsectionx = isectx + iseenx - (imapside >> 1);
                int32_t iintsectiony = isecty + iseeny - (imapside >> 1);
                sectoffset = *::new (&dst) NiPoint3(
                    static_cast<float>((iintsectionx << 12) + 2048), static_cast<float>((iintsectiony << 12) + 2048),
                    0.0f
                );
                pseendata = pseencell->GetIntSeenSection(iintsectionx, iintsectiony, false);
                pseencell->GetInteriorLocalMapTexture(iintsectionx, iintsectiony, spmaptex);
            }
        }
        else {
            TESWorldSpace* pworld = pPlayer->GetWorldSpace();
            int32_t icellx = FloatToInt(playerloc.x) >> 12;
            int32_t icelly = FloatToInt(playerloc.y) >> 12;
            int32_t icellsectionx = isectx + icellx - (imapside >> 1);
            int32_t icellsectiony = isecty + icelly - (imapside >> 1);
            sectoffset =
                *::new (&v75)
                    NiPoint3(static_cast<float>(icellsectionx << 12), static_cast<float>(icellsectiony << 12), 0.0f);
            if (pworld) {
                pseencell = pworld->GetCellFromCellCoord(icellsectionx, icellsectiony);
            }
            if (pseencell) {
                pseendata = pseencell->GetSeenData();
                pseencell->GetExteriorLocalMapTexture(spmaptex);
            }
        }
        int32_t iarraysize_2 = iarraysize;
        void* ptr = ourMalloc(iarraysize <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * iarraysize : SIZE_MAX);
        if (ptr) {
            rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), iarraysize_2, &emptyPointConstructor);
            pverts = static_cast<NiPoint3*>(ptr);
        }
        else {
            pverts = nullptr;
        }
        NiPoint3* v73 = pverts;
        NiPoint3* pverts_2 = pverts;
        int32_t iarraysize_3 = iarraysize;
        void* v69 = ourMalloc(iarraysize <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * iarraysize : SIZE_MAX);
        if (v69) {
            rt::eh_vector_constructor_iterator(v69, sizeof(NiPoint3), iarraysize_3, &emptyPointConstructor);
            pnorm = static_cast<NiPoint3*>(v69);
        }
        else {
            pnorm = nullptr;
        }
        NiPoint3* v70 = pnorm;
        NiPoint3* pnorm_2 = pnorm;
        int32_t iarraysize_4 = iarraysize;
        void* v66 = ourMalloc(iarraysize <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * iarraysize : SIZE_MAX);
        if (v66) {
            rt::eh_vector_constructor_iterator(v66, sizeof(NiPoint2), iarraysize_4, &emptyPointConstructor);
            ptex = static_cast<NiPoint2*>(v66);
        }
        else {
            ptex = nullptr;
        }
        NiPoint2* v67 = ptex;
        NiPoint2* ptex_2 = ptex;
        int32_t iarraysize_5 = iarraysize;
        void* v63 = ourMalloc(iarraysize <= SIZE_MAX / sizeof(NiColorA) ? sizeof(NiColorA) * iarraysize : SIZE_MAX);
        if (v63) {
            pcolor = ::new (v63) NiColorA[iarraysize_5];
        }
        else {
            pcolor = nullptr;
        }
        NiColorA* a1 = pcolor;
        NiColorA* pcolor_2 = pcolor;
        uint16_t* ptrilist = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t) * itilecount));
        int32_t index = 0;
        for (y_2 = 0; y_2 < iseensize; ++y_2) {
            for (x_2 = 0; x_2 < iseensize; ++x_2) {
                float v46 = static_cast<double>(itilesize * y_2) - static_cast<double>(itrisize) / 2.0;
                float v45 = static_cast<double>(itilesize * x_2) - static_cast<double>(itrisize) / 2.0;
                NiPoint3* v11 = ::new (&v62) NiPoint3(v45, v46, 0.0f);
                pverts_2[index++] = *v11;
            }
        }
        index = 0;
        for (y_2 = 0; y_2 < iseensize; ++y_2) {
            for (x_2 = 0; x_2 < iseensize; ++x_2) {
                NiPoint3* v12 = ::new (&v61) NiPoint3(0.0f, 0.0f, 1.0f);
                pnorm_2[index] = *v12;
                float v44 = 1.0 - static_cast<double>(y_2) / static_cast<double>(iseensize);
                float v43 = static_cast<double>(x_2) / static_cast<double>(iseensize);
                NiPoint2* v13 = ::new (&v60) NiPoint2(v43, v44);
                float x = v13->x;
                float y = v13->y;
                int32_t v16 = index;
                NiPoint2* v17 = ptex_2;
                ptex_2[index].x = x;
                v17[v16].y = y;
                if (ifog) {
                    float v42 = static_cast<double>(y_2) * fpossize;
                    float v41 = static_cast<double>(x_2) * fpossize;
                    NiPoint3 worldcoord(v41, v42, 0.0f);
                    worldcoord.operator+=(sectoffset);
                    float fseenval = static_cast<float>(TESObjectCELL::GetSeenValue(worldcoord));
                    float v40 = fseenval / 4.0;
                    NiColorA kSeenColor(v40, v40, v40, 0.0f);
                    NiColorA* v18 = &kSeenColor;
                    NiColorA* v19 = &pcolor_2[index];
                    v19->r = v18->r;
                    v19->g = v18->g;
                    v19->b = v18->b;
                    v19->a = v18->a;
                }
                else {
                    pcolor_2[index] = NiColorA_WHITE;
                }
                ++index;
            }
        }
        index = 0;
        for (y_2 = 0; y_2 < itileside; ++y_2) {
            for (x_2 = 0; x_2 < itileside; ++x_2) {
                uint16_t v20;
                if (x_2 % 2 != y_2 % 2) {
                    ptrilist[index++] = x_2 + iseensize * (y_2 + 1);
                    ptrilist[index++] = x_2 + iseensize * y_2;
                    ptrilist[index++] = iseensize * y_2 + x_2 + 1;
                    ptrilist[index++] = iseensize * y_2 + x_2 + 1;
                    ptrilist[index++] = iseensize * (y_2 + 1) + x_2 + 1;
                    uint16_t ival = x_2 + iseensize * (y_2 + 1);
                    v20 = ival;
                }
                else {
                    ptrilist[index++] = iseensize * (y_2 + 1) + x_2 + 1;
                    ptrilist[index++] = x_2 + iseensize * (y_2 + 1);
                    ptrilist[index++] = x_2 + iseensize * y_2;
                    ptrilist[index++] = x_2 + iseensize * y_2;
                    ptrilist[index++] = iseensize * y_2 + x_2 + 1;
                    uint16_t ival_2 = iseensize * (y_2 + 1) + x_2 + 1;
                    v20 = ival_2;
                }
                ptrilist[index++] = v20;
            }
        }
        NiTriShape* v57 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
        if (v57) {
            ptri = ::new (v57) NiTriShape(
                iarraysize, pverts_2, pnorm_2, pcolor_2, ptex_2, 1, NiGeometryData::NBT_METHOD_NONE, 2 * itilecount,
                ptrilist
            );
        }
        else {
            ptri = nullptr;
        }
        NiTriShape* v58 = ptri;
        NiTriShape* ptri_2 = ptri;
        pnode->AttachChild(ptri, 1);
        float v38 = static_cast<double>(isecty * itrisize) + static_cast<double>(itrisize) / 2.0 -
            static_cast<double>(imapsize) / 2.0;
        float v37 = static_cast<double>(isectx * itrisize) + static_cast<double>(itrisize) / 2.0 -
            static_cast<double>(imapsize) / 2.0;
        ptri_2->SetTranslate(v37, v38, 0.0f);
        if (spmaptex.m_pObject) {
            NiTexturingProperty* ptexprop;
            NiTexturingProperty* v55 =
                static_cast<NiTexturingProperty*>(NiMemObject::operator new(sizeof(NiTexturingProperty)));
            if (v55) {
                ptexprop = ::new (v55) NiTexturingProperty;
            }
            else {
                ptexprop = nullptr;
            }
            NiTexturingProperty* v56 = ptexprop;
            NiProperty* ptexprop_2 = ptexprop;
            NiTexture* a4 = spmaptex.m_pObject;
            static_cast<NiTexturingProperty*>(ptexprop_2)->NiTexturingProperty::SetBaseTexture(a4);
            static_cast<NiTexturingProperty*>(ptexprop_2)
                ->NiTexturingProperty::SetBaseClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
            static_cast<NiTexturingProperty*>(ptexprop_2)->SetApplyMode(NiTexturingProperty::APPLY_MODULATE);
            ptri_2->AttachProperty(ptexprop_2);
            spmaptex.operator=(static_cast<NiTexture*>(nullptr));
        }
    }
    NiPoint3 playerpos = pPlayer->GetLocationOnReference();
    if (pPlayer->GetInterior()) {
        NiPoint3 playerintloc_2;
        const NiPoint3* v21 = &pPlayer->GetLocationOnReference();
        pPlayer->GetParentCell()->AdjustCoordForNorthRotation(*v21, playerintloc_2, true);
        int32_t isectx_2 = (FloatToInt(playerintloc_2.x) - 2048) >> 12;
        int32_t isecty_2 = (FloatToInt(playerintloc_2.y) - 2048) >> 12;
        mapworldcenter = *::new (&v54) NiPoint3(
            static_cast<float>((isectx_2 << 12) + 4096), static_cast<float>((isecty_2 << 12) + 4096), 0.0f
        );
        playerpos = playerintloc_2;
    }
    else {
        const NiPoint3* v22 = &pPlayer->GetLocationOnReference();
        int32_t isectx_3 = FloatToInt(v22->x) >> 12;
        const NiPoint3* v23 = &pPlayer->GetLocationOnReference();
        int32_t isecty_3 = FloatToInt(v23->y) >> 12;
        mapworldcenter = *::new (&v53) NiPoint3(
            static_cast<float>((isectx_3 << 12) + 2048), static_cast<float>((isecty_3 << 12) + 2048), 0.0f
        );
    }
    playerpos.z = 0.0f;
    NiPoint3 playeroffset = playerpos - mapworldcenter;
    float fmapratio = static_cast<double>(itrisize) / 4096.0;
    playeroffset.x = playeroffset.x * fmapratio;
    playeroffset.y = playeroffset.y * fmapratio;
    playeroffset.z = 0.0f;
    NiPoint3 pointn(0.0f, 1.5f, 1.0f);
    NiPoint3 pointse(0.5f, -0.5f, 1.0f);
    NiPoint3 pointsw(-0.5f, -0.5f, 1.0f);
    NiColorA kColor(1.0f, 0.0f, 0.0f, 0.0f);
    NiColorA* a3d = &kColor;
    NiTriShape* pmark = MakeTriangle(pointn, pointse, pointsw, *a3d, true);
    pmark->SetTranslate(playeroffset);
    markrot = NiMatrix3_IDENTITY;
    double NorthRotation = pPlayer->GetParentCell()->GetNorthRotation();
    float v35 = (&pPlayer->GetAngleOnReference())->z + NorthRotation;
    markrot.MakeZRotation(v35);
    pmark->SetRotate(markrot);
    pnode->AttachChild(pmark, 1);
    pnode->SetTranslate(playerloc);
    NiUpdateData UpdateData(0.0f, false, false);
    pnode->Update(UpdateData);
    pnode->UpdateProperties();
    TES::pInstance->AddTempDebugObject(pnode, 30.0f);
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
    const char* v10;
    bool ShowMagicStats = MagicSystem::GetShowMagicStats();
    MagicSystem::SetShowMagicStats(!ShowMagicStats);
    if (MagicSystem::GetShowMagicStats()) {
        v10 = "shown.";
    }
    else {
        v10 = "hidden.";
    }
    Interface::PrintLine("Magic stats %s", v10);
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
    char a8[516];
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    TES::pInstance->SaveWorld(a8);
    return true;
}

bool Script::ToggleMenus(
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
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v1;
        if (bMenusOn) {
            v1 = "On";
        }
        else {
            v1 = "Off";
        }
        Interface::PrintLine("Menus -> %s", v1);
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
    bool LeavesEnabled = BSTreeManager::AreLeavesVisible();
    BSTreeManager::SetLeavesVisible(!LeavesEnabled);
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v10;
        if (BSTreeManager::AreLeavesVisible()) {
            v10 = "On";
        }
        else {
            v10 = "Off";
        }
        Interface::PrintLine("Leaves -> %s", v10);
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
    Actor* a8;
    ReferenceProcessLists.bShowDetectionStats = !ReferenceProcessLists.bShowDetectionStats;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    ReferenceProcessLists.pStatdetect = a8;
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v9;
        if (ReferenceProcessLists.bShowDetectionStats) {
            v9 = "On";
        }
        else {
            v9 = "Off";
        }
        Interface::PrintLine("AI Detection stats printing is  %s", v9);
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
    float famount;
    if (BSShaderManager::IsRefractionEnabled()) {
        famount = 0.0f;
        if (!Script::ParseParameters(
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &famount
            )) {
            return false;
        }
        if (!apRefObject) {
            apRefObject = pPlayer;
        }
        float a1 = BSMIN(famount, 1.0f);
        famount = BSMAX(a1, 0.0);
        bool benabled = famount > 0.0;
        Actor* pactor = dynamic_cast<Actor*>(apRefObject);
        if (pactor) {
            pactor->SetRefraction(benabled, famount);
        }
        else {
            NiAVObject* pobj3D = apRefObject->Get3D();
            if (pobj3D) {
                BSShaderPPLightingProperty::SetRefractionRecurse(pobj3D, 0, 0.0f, 0, 0.0f, 0);
                BSShaderPPLightingProperty::SetRefractionRecurse(pobj3D, 0, 0.0f, 1, 0.0f, 0);
                BSShaderPPLightingProperty::SetRefractionRecurse(pobj3D, benabled, famount, 0, 0.0f, 0);
            }
        }
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("%s refraction has been set to %f", apRefObject->GetFullName(), famount);
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
    float a8;
    a8 = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    float afConst = a8;
    ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
    ImageSpaceManager_->SetMotionBlurConst(afConst);
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
    float fPos;
    fPos = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &cAxis, &fPos
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
        NiNode* v9 = apRefObject->Get3D();
        TESHavokUtilities::AddVelocity(v9, pos, true);
    }
    return true;
}

bool Script::ShowViewerStrings(
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
        NiTPrimitiveArray<char*>* v11;
        NiNode* v18 = apRefObject->Get3D();
        NiTPrimitiveArray<char*>* apMemory =
            static_cast<NiTPrimitiveArray<char*>*>(ourMalloc(sizeof(NiTPrimitiveArray<char*>)));
        if (apMemory) {
            v11 = ::new (apMemory) NiTPrimitiveArray<char*>(0, 1);
        }
        else {
            v11 = nullptr;
        }
        if ((v18) && (v18->IsNode())) {
            NiNode* v17 = v18;
            v18 = nullptr;
            uint32_t a1 = 0;
            for (uint32_t i = v17->GetArrayCount(); a1 < i && !v18; i = v17->GetArrayCount()) {
                v18 = static_cast<NiNode*>(v17->GetAt(a1++));
            }
        }

        if (!v18) {
            Interface::PrintLine("Node with no children supplied as reference...");
            return true;
        }
        v18->GetViewerStrings(v11);
        for (uint32_t uiIndex = 0; uiIndex < v11->m_usSize; ++uiIndex) {
            Interface::PrintLine(static_cast<const char*>(v11->operator[](uiIndex)));
        }
        v11->RemoveAll();
        if (v18->GetCollisionObject()) {
            NiObject* v14 = NiObject::DynamicCastStatic(&bhkCollisionObject::ms_RTTI, v18->GetCollisionObject());
            if (v14) {
                bhkRigidBody* pBody = static_cast<bhkCollisionObject*>(v14)->GetRigidBody();
                pBody->GetViewerStrings(v11);
                for (uint32_t j = 0; j < v11->m_usSize; ++j) {
                    Interface::PrintLine(static_cast<const char*>(v11->operator[](j)));
                }
                v11->RemoveAll();
            }
        }
        if (v11) {
            delete v11;
        }
    }
    else {
        Interface::PrintLine("Must supply a valid reference to show viewer string.");
    }
    return true;
}

bool Script::ToggleLODLand(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[516];
    a8[0] = 0;
    if (apParameters &&
        !Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    BGSTerrainManager* terrainManager = TES::pInstance->pWorldSpace->GetTerrainManager();
    if (!terrainManager) {
        return true;
    }
    char v23 = 0;
    char v21 = 0;
    char v20 = 0;
    char v19 = 0;
    if (!a8[0] || !BSstrcmp(a8, "all")) {
        Interface::PrintLine("Toggling all LOD.");
        v23 = 1;
        v21 = 1;
        v20 = 1;
        v19 = 1;
    }
    if (!BSstrcmp(a8, "water")) {
        Interface::PrintLine("Toggling water.");
        v19 = 1;
    }
    if (!BSstrcmp(a8, "land")) {
        Interface::PrintLine("Toggling land.");
        v20 = 1;
    }
    if (!BSstrcmp(a8, "trees")) {
        Interface::PrintLine("Toggling trees.");
        v21 = 1;
    }
    if (!BSstrcmp(a8, "objects") || !BSstrcmp(a8, "buildings")) {
        Interface::PrintLine("Toggling objects.");
        v23 = 1;
    }
    if (v19 || (v20 || (v21 || v23))) {
        if ((terrainManager->GetTreeNode()) && (v21)) {
            NiNode* TreeNode = terrainManager->GetTreeNode();
            bool v16 = !TreeNode->GetAppCulled();
            NiNode* v10 = terrainManager->GetTreeNode();
            v10->SetAppCulled(v16);
        }

        if ((terrainManager->GetLODLandRoot()) && (v20)) {
            NiNode* LODLandRoot = terrainManager->GetLODLandRoot();
            bool v17 = !LODLandRoot->GetAppCulled();
            NiNode* v12 = terrainManager->GetLODLandRoot();
            v12->SetAppCulled(v17);
        }

        if ((terrainManager->GetLODObjectRoot()) && (v23)) {
            NiNode* LODObjectRoot = terrainManager->GetLODObjectRoot();
            bool v18 = !LODObjectRoot->GetAppCulled();
            NiNode* v14 = terrainManager->GetLODObjectRoot();
            v14->SetAppCulled(v18);
        }

        if ((terrainManager->GetWaterLODRoot()) && (v19)) {
            bool AppCulled = terrainManager->GetWaterLODRoot()->GetAppCulled();
            terrainManager->GetWaterLODRoot()->SetAppCulled(!AppCulled);
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
    const NiPoint3* Translate;
    float f1;
    float f0;
    f0 = 0.0f;
    f1 = 0.0f;
    if (apParameters &&
        !Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &f0, &f1
        )) {
        return false;
    }
    BGSTerrainManager* pm = TES::pInstance->pWorldSpace->GetTerrainManager();
    if (!pm) {
        return true;
    }
    fBlockLoadDistanceLow.operator=(f0);
    fBlockLoadDistance.operator=(f1);
    NiPoint3 updatePos = NiPoint3_ZERO;
    if (appMain->bFlyCamera) {
        SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
        Translate = &WorldSceneGraph->GetCameraRoot()->GetTranslate();
    }
    else {
        Translate = &pPlayer->GetLocationOnReference();
    }
    updatePos = *Translate;
    const NiPoint3* v11 = &pPlayer->GetLocationOnReference();
    pm->Update(*v11, BGSTerrainManager::UPDATE_LAND_AND_BLOCKS);
    return true;
}

bool Script::HighlightBodyPart(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v13;
    char v14[4];
    char v15[4];
    float a8;
    a8 = 15.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, v14,
            &v13, v15
        )) {
        return false;
    }
    if ((apRefObject) && (apRefObject->Get3D())) {
        BGSBodyPart::LIMB_ENUM v11 = static_cast<BGSBodyPart::LIMB_ENUM>(static_cast<int32_t>(a8));
        FOVATSEffectManager* VATSEffect = Interface::GetVATSEffect();
        VATSEffect->SetTarget(apRefObject, v11, false);
        FOVATSEffectManager::VATS_EFFECT_MODE a2 = FOVATSEffectManager::SINGLE_TARGET;
        FOVATSEffectManager* v10 = Interface::GetVATSEffect();
        v10->SetEffectMode(a2);
    }

    return true;
}

bool Script::HighlightAdditionalReference(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char v13[4];
    char v14[4];
    char v15[4];
    float a8;
    a8 = 15.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, v14,
            v13, v15
        )) {
        return false;
    }
    if ((apRefObject) && (apRefObject->Get3D())) {
        FOVATSEffectManager* VATSEffect = Interface::GetVATSEffect();
        VATSEffect->SetMultiTarget(apRefObject);
        FOVATSEffectManager* v10 = Interface::GetVATSEffect();
        v10->SetSelectedMultiTarget(apRefObject);
        FOVATSEffectManager::VATS_EFFECT_MODE a2 = FOVATSEffectManager::MULTIPLE_TARGETS;
        FOVATSEffectManager* v11 = Interface::GetVATSEffect();
        v11->SetEffectMode(a2);
    }

    return true;
}

bool Script::DeactivateAllHighlights(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char v13[4];
    char v14[4];
    char v15[4];
    float a8;
    a8 = 15.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, v14,
            v13, v15
        )) {
        return false;
    }
    if ((apRefObject) && (apRefObject->Get3D())) {
        FOVATSEffectManager* VATSEffect = Interface::GetVATSEffect();
        VATSEffect->SetTarget(nullptr, BGSBodyPart::LIMB_TORSO, false);
        FOVATSEffectManager* v10 = Interface::GetVATSEffect();
        v10->ClearMultiTargets();
        FOVATSEffectManager::VATS_EFFECT_MODE a2 = FOVATSEffectManager::SINGLE_TARGET;
        FOVATSEffectManager* v11 = Interface::GetVATSEffect();
        v11->SetEffectMode(a2);
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
    NiMatrix3 kcamerafunk_2;
    NiPoint3 kpos;
    NiMatrix3 kcamerarot;
    NiPoint2 ksize(-1.0f, -1.0f);
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &ksize.y,
            &ksize
        )) {
        return false;
    }
    if (ksize.x == 0.0 || ksize.y == 0.0) {
        ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
        BSPortalGraph* PortalGraph = ShadowSceneNode_->GetPortalGraph();
        PortalGraph->RemoveAllOccluders();
        return true;
    }

    BSOcclusionPlane* v15;
    NiMatrix3 kcamerafunk;
    if (ksize.x <= 0.0) {
        ksize.x = 50.0f;
    }
    if (ksize.y <= 0.0) {
        ksize.y = 50.0f;
    }
    ShadowSceneNode* v11 = BSShaderManager::GetShadowSceneNode(0);
    ::new (&kpos) NiPoint3(v11->GetCameraPos());
    kcamerarot = Main::GetWorldSceneGraph()->GetCamera()->GetWorldTransform().m_Rotate;
    kcamerafunk.MakeZRotation(1.5707963f);
    kcamerarot = *::new (&kcamerafunk_2) NiMatrix3(kcamerarot * kcamerafunk);
    BSOcclusionPlane* apMemory = static_cast<BSOcclusionPlane*>(NiMemObject::operator new(sizeof(BSOcclusionPlane)));
    if (apMemory) {
        v15 = ::new (apMemory) BSOcclusionPlane(&kpos, &ksize, &kcamerarot);
    }
    else {
        v15 = nullptr;
    }
    NiPointer<BSOcclusionPlane> spoccluder(v15);
    BSOcclusionPlane* angle = spoccluder.m_pObject;
    ShadowSceneNode* v12 = BSShaderManager::GetShadowSceneNode(0);
    BSPortalGraph* v13 = v12->GetPortalGraph();
    v13->AddOccluder(angle);
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
    NiPoint2 v10;
    float frampup;
    float fstrength;
    float fdownstart;
    float frampdown;
    float fstart;
    bool bret = true;
    if (g_radialBlurIMOD) {
        ImageSpaceModifierInstance::Stop(g_radialBlurIMOD);
        g_radialBlurIMOD = nullptr;
    }
    if (Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &fstrength,
            &frampup, &fstart, &frampdown, &fdownstart
        )) {
        NiPoint2* v8 = ::new (&v10) NiPoint2(0.5f, 0.5f);
        g_radialBlurIMOD = ImageSpaceModifierInstanceRB::Trigger_0052A440(
            fstrength, frampup, fstart, frampdown, fdownstart, *v8, 0.0f
        );
        if (g_radialBlurIMOD) {
            g_radialBlurIMOD->SetPermanent(true);
        }
    }
    else {
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("SetGlobalRadialBlur strength rampup start rampdown downstart");
        }
        return false;
    }
    return bret;
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
    Actor* pActor;
    pActor = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pActor
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
        if (GetThreadTLSData()->bSystemOutput) {
            const char* v10;
            if (pActor->bProcessMe) {
                v10 = "On";
            }
            else {
                v10 = "Off";
            }
            Interface::PrintLine("%s processing is  %s", pActor->GetFullName(), v10);
        }
    }
    else {
        // Tests the whole byte here too.
        ReferenceProcessLists.bRunScheduals = static_cast<uint8_t>(ReferenceProcessLists.bRunScheduals) == 0;
        if (GetThreadTLSData()->bSystemOutput) {
            const char* v9;
            if (ReferenceProcessLists.bRunScheduals) {
                v9 = "On";
            }
            else {
                v9 = "Off";
            }
            Interface::PrintLine("All AI Processing is  %s", v9);
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
    int32_t ifreezeTime;
    ifreezeTime = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &ifreezeTime
        )) {
        return false;
    }
    bool v11 = false;
    if (ifreezeTime != 4) {
        bool bisFlyCam;
        if ((ifreezeTime >= 1 && ifreezeTime <= 2) || ifreezeTime == 5) {
            bisFlyCam = pPlayer->ToggleFlyCam(true);
        }
        else {
            bisFlyCam = pPlayer->ToggleFlyCam(false);
        }
        v11 = bisFlyCam;
    }
    if (v11 && ifreezeTime >= 1) {
        BSShaderProperty::PreviousTransform::SetFreeze(true);
    }
    else {
        BSShaderProperty::PreviousTransform::SetFreeze(false);
    }
    return true;
}

bool Script::ToggleNavMesh(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t eMode;
    eMode = 4;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &eMode
        )) {
        return false;
    }
    if (eMode > 6) {
        eMode = 4;
    }
    NavMeshRender* Instance = NavMeshRender::GetInstance();
    bool bIsDraw = Instance->GetDraw();
    if (eMode == 4) {
        eMode = !bIsDraw;
    }
    if (eMode == 5) {
        NavMeshRender::bDrawCover = !NavMeshRender::bDrawCover;
        if (bIsDraw) {
            eMode = NavMeshRender::iNavMeshDrawMode;
            NavMeshRender* v10 = NavMeshRender::GetInstance();
            v10->SetDraw(false);
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
            NavMeshRender* v11 = NavMeshRender::GetInstance();
            v11->SetDraw(false);
            bIsDraw = false;
        }
        eMode = 3;
    }
    if (eMode) {
        NavMeshRender::bDrawTriangles = true;
        if (eMode == 2) {
            NavMeshRender* v12 = NavMeshRender::GetInstance();
            v12->SetTransparentOverLayMode(true);
        }
        else {
            if (eMode == 1 || eMode == 3) {
                NavMeshRender* v13 = NavMeshRender::GetInstance();
                v13->SetTransparentOverLayMode(false);
            }
        }
        NavMeshRender::iNavMeshDrawMode = eMode;
        if (!bIsDraw) {
            TESObjectCELL* CurrentCell = TES::pInstance->GetCurrentCell();
            if (CurrentCell) {
                if (CurrentCell->GetInterior()) {
                    NavMeshRender* v16 = NavMeshRender::GetInstance();
                    v16->AddDrawOnlyNavMeshes(CurrentCell, false);
                }
                else {
                    int32_t v22 = (uGridsToLoad.Int() - 1) / 2;
                    for (int32_t aiX = CurrentCell->GetDataX() - v22; aiX <= v22 + CurrentCell->GetDataX(); ++aiX) {
                        for (int32_t aiY = CurrentCell->GetDataY() - v22; aiY <= v22 + CurrentCell->GetDataY(); ++aiY) {
                            TESWorldSpace* WorldSpace = CurrentCell->GetWorldSpace();
                            TESObjectCELL* apCell = WorldSpace->GetCellFromCellCoord(aiX, aiY);
                            NavMeshRender* v15 = NavMeshRender::GetInstance();
                            v15->AddDrawOnlyNavMeshes(apCell, false);
                        }
                    }
                }
            }
            NavMeshRender* v17 = NavMeshRender::GetInstance();
            v17->SetDraw(true);
        }
    }
    else {
        NavMeshRender::bDrawTriangles = false;
        NavMeshRender* v18 = NavMeshRender::GetInstance();
        v18->SetDraw(false);
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
    TESObjectREFR* pPickRef;
    pPickRef = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pPickRef
        )) {
        return false;
    }
    if (pPickRef) {
        int32_t FormType = pPickRef->GetFormType();
        if (FormType >= 58 && (FormType <= 64 || FormType == 105)) {
            BSStringT<char> v16;
            int32_t CastingType = pPickRef->iFormID;
            v16.SPrintF("\"%s\" (%08x)", pPickRef->GetFullName(), CastingType);
            int32_t v14 = iDebugTextFont.Int() - 1;
            FontManager* Singleton = FontManager::Instance();
            int32_t v17 = static_cast<int32_t>(Singleton->GetFont(v14)->GetBaseLine() + 3.0) - 5;
            float a4 = static_cast<float>(v17);
            char* v11 = v16.pString;
            DebugText* v10 = DebugText::Instance(true);
            v10->Print(v11, 640.0f, a4, 2, -1, -1.0f, 0, nullptr);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetActorValueConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetPermanentActorValueConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    ActorValue::Index a8;
    int32_t v11;
    a8 = ActorValue::AGGRESSION;
    v11 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v11
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        if (ActorValue::IsFlagSet(a8, ActorValue::NO_SCRIPT_MODAV)) {
            if (GetThreadTLSData()->bSystemOutput) {
                Interface::PrintLine(
                    "Actor Value '%s' cannot be modified in scripts or the console.",
                    ActorValue::GetActorValueScriptName(a8)
                );
            }
        }
        else {
            pActor->SetActorValue(a8, v11);
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &eIndex,
            &iValue
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        if (ActorValue::IsFlagSet(eIndex, ActorValue::NO_SCRIPT_MODAV)) {
            if (GetThreadTLSData()->bSystemOutput) {
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
    ActorValue::Index a8;
    float v12;
    a8 = ActorValue::AGGRESSION;
    v12 = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v12
        )) {
        return false;
    }
    Actor* v10 = dynamic_cast<Actor*>(apRefObject);
    if (v10) {
        if (ActorValue::IsFlagSet(a8, ActorValue::NO_SCRIPT_MODAV)) {
            if (GetThreadTLSData()->bSystemOutput) {
                Interface::PrintLine(
                    "Actor Value '%s' cannot be modified in scripts or the console.",
                    ActorValue::GetActorValueScriptName(a8)
                );
            }
        }
        else {
            float v9;
            if (ActorValue::IsFlagSet(a8, ActorValue::DAMAGE_IS_POSITIVE)) {
                v9 = -v12;
            }
            else {
                v9 = std::fabs(v12);
            }
            v10->RestoreActorValue(a8, v9);
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
    ActorValue::Index a8;
    int32_t v13;
    a8 = ActorValue::AGGRESSION;
    v13 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v13
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        if (ActorValue::IsFlagSet(a8, ActorValue::NO_SCRIPT_MODAV)) {
            if (GetThreadTLSData()->bSystemOutput) {
                Interface::PrintLine(
                    "Actor Value '%s' cannot be modified in scripts or the console.",
                    ActorValue::GetActorValueScriptName(a8)
                );
            }
        }
        else {
            double v9 = static_cast<double>(v13);
            float v10 = v9 - pActor->GetActorFloatValue(a8);
            pActor->PermanentModActorValue(a8, v10, nullptr);
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iLimb,
            &iLastLimb
        )) {
        return false;
    }
    if (iLastLimb <= 0) {
        return Script::IsLimbGoneConditionFunction(
            apRefObject, reinterpret_cast<void*>(static_cast<intptr_t>(iLimb)), nullptr, afReturnValue
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, input
        )) {
        return false;
    }
    char* pcurrent = input;
    uint16_t passes = 0;
    // Tests pcurrent, the copy of the buffer's address.
    if (pcurrent && *pcurrent == 49) {
        passes |= 1u;
        BSShaderManager::SetRenderMode(0);
        BSShaderManager::SetDebugMode(BSShaderManager::BSSM_RENDER_NORMAL);
        if (BSstrlen(input) == 1) {
            passes |= 2u;
            passes |= 4u;
            passes |= 8u;
            passes |= BSShaderManager::GetShaderLevel(false) < BSShaderManager::BSSM_SV_2_0 ? 0 : 32;
        }
        goto LABEL_55;
    }
    if (pcurrent && *pcurrent == 50) {
        passes = 1;
        BSShaderManager::SetDebugMode(BSShaderManager::BSSM_RENDER_NUMLIGHTS);
    LABEL_55:;
        if ((++pcurrent) && (*pcurrent == 49)) {
            passes |= 2u;
        }

        if ((++pcurrent) && (*pcurrent == 49)) {
            passes |= 4u;
        }

        if ((++pcurrent) && (*pcurrent == 49)) {
            passes |= 8u;
        }

        if (BSShaderManager::GetShaderLevel(false) >= BSShaderManager::BSSM_SV_2_0 &&
            (!++pcurrent || *pcurrent != 48)) {
            passes |= 32u;
        }
        BSShaderManager::SetEnabledPasses(passes);
        return true;
    }
    if (pcurrent && *pcurrent == 51) {
        passes = 1;
        BSShaderManager::SetDebugMode(BSShaderManager::BSSM_RENDER_NUMPASSES);
        goto LABEL_55;
    }
    if (pcurrent && *pcurrent == 52) {
        passes |= 1u;
        BSShaderManager::SetRenderMode(3u);
        BSShaderManager::SetDebugMode(BSShaderManager::BSSM_RENDER_NORMAL);
        if (BSstrlen(input) == 1) {
            passes |= 2u;
            passes |= 4u;
            passes |= 8u;
            passes |= BSShaderManager::GetShaderLevel(false) < BSShaderManager::BSSM_SV_2_0 ? 0 : 32;
        }
        goto LABEL_55;
    }
    if (pcurrent && *pcurrent == 53) {
        goto LABEL_55;
    }
    if (!_strnicmp("tex", pcurrent, 3u)) {
        passes = 1;
        BSShaderManager::SetDebugMode(BSShaderManager::BSSM_RENDER_TEXUSAGE);
        goto LABEL_55;
    }
    if (!_strnicmp("degrade", pcurrent, 7u)) {
        passes = 1;
        BSShaderManager::SetDebugMode(BSShaderManager::BSSM_RENDER_DEGRADE);
        goto LABEL_55;
    }
    if (!_strnicmp("sh", pcurrent, 2u)) {
        if (pcurrent[2]) {
            int32_t imode = pcurrent[2] - 48;
            if (pcurrent[2] == 49 || pcurrent[2] == 50) {
                imode = 3;
            }
            int32_t v9 = BSMIN(3, imode);
            BSShaderManager::eShadowMode = static_cast<BSShaderManager::etShadowMode>(BSMAX(0, v9));
            BSShaderManager::ReloadShaders();
            passes = BSShaderManager::GetEnabledPasses() | 128;
        }
        else {
            bool toggle = BSShaderManager::GetShadows();
            BSShaderManager::EnableShadows(!toggle);
            passes = BSShaderManager::GetEnabledPasses();
            if ((passes & 128) != 0) {
                passes &= ~128u;
            }
            else {
                passes |= 128u;
            }
        }
        goto LABEL_55;
    }
    if (!_strnicmp("sc", pcurrent, 2u)) {
        BSShaderManager::bLightScissor = !BSShaderManager::bLightScissor;
        passes = BSShaderManager::GetEnabledPasses();
        goto LABEL_55;
    }
    if (!_strnicmp("psh", pcurrent, 2u)) {
        if ((BSShaderManager::GetEnabledPasses() & 32) != 0) {
            Renderer::SetDoCanopyShadowPass(false);
            BSShaderManager::SetProjectedShadowMaskTexture(nullptr);
            uint16_t passes_3 = BSShaderManager::GetEnabledPasses() & 0xFFDF;
            BSShaderManager::SetEnabledPasses(passes_3);
        }
        else {
            Renderer::SetDoCanopyShadowPass(true);
            TES::pInstance->pGridCellA->RecreateCanopyShadowMaskTexture();
            uint16_t passes_2 = BSShaderManager::GetEnabledPasses() | 32;
            BSShaderManager::SetEnabledPasses(passes_2);
        }
        return true;
    }

    if (*pcurrent == 116) {
        BSShaderManager::iMaxLandTextures = std::atoi(pcurrent + 1);
        goto LABEL_55;
    }
    if (_strnicmp("dsh", pcurrent, 3u)) {
        if (!_strnicmp("opt", pcurrent, 3u)) {
            BSShaderManager::bUseOptPasses = !BSShaderManager::bUseOptPasses;
            passes = BSShaderManager::GetEnabledPasses() | 256;
        }
        else {
            if (!_strnicmp("alpha", pcurrent, 5u)) {
                return true;
            }
        }
        goto LABEL_55;
    }
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
    float iDistFar;
    float iDistNear;
    iDistNear = 0.0f;
    iDistFar = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iDistNear,
            &iDistFar
        )) {
        return false;
    }
    BSFogProperty* pbsfog = TES::pInstance->pSky->pAtmosphere->GetFog();
    if (pbsfog) {
        bool bupdatedist = iDistNear == 0.0f && iDistFar == 0.0f;
        TES::pInstance->pSky->pAtmosphere->SetUpdateFogDistance(bupdatedist);
        float fDistFar = static_cast<float>(std::bit_cast<int32_t>(iDistFar));
        float fDistNear = static_cast<float>(std::bit_cast<int32_t>(iDistNear));
        pbsfog->SetDistance(fDistNear, fDistFar);
    }
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
    NiFrustum frustum;
    float fClipDistance;
    fClipDistance = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals,
            &fClipDistance
        )) {
        return false;
    }
    bool bUpdateFogDistance = false;
    if (!(fClipDistance <= 0.0)) {
        if (fClipDistance > 163840.0) {
            fClipDistance = 163840.0f;
        }
    }
    else {
        fClipDistance = 163840.0f;
        bUpdateFogDistance = true;
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
    float c2;
    float c1;
    char c3[4];
    float a8;
    a8 = fBlurRadiusHUD.Float();
    c1 = fBlurIntensityHUD.Float();
    c2 = fScanlineFrequencyHUD.Float();
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &c1,
            &c2, c3
        )) {
        return false;
    }
    float a2 = a8;
    FOHUDEffectManager* HUDEffects = Interface::GetHUDEffects();
    HUDEffects->SetBlurRadiusHUD(a2);
    Interface::GetHUDEffects()->SetBlurIntensityHUD(c1);
    float a2a = c2;
    FOHUDEffectManager* v10 = Interface::GetHUDEffects();
    v10->SetScanlineFrequencyHUD(a2a);
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
    int32_t iG;
    int32_t iR;
    int32_t iB;
    iR = 0;
    iG = 0;
    iB = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iR, &iG, &iB
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
    int32_t iSpeed;
    iSpeed = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &cAxis,
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
                    fAngle = (&apRefObject->GetAngleOnReference())->x;
                    break;
                }
                case 89: {
                    fAngle = (&apRefObject->GetAngleOnReference())->y;
                    break;
                }
                case 90: {
                    fAngle = (&apRefObject->GetAngleOnReference())->z;
                    break;
                }
            }
            double v10 = static_cast<double>(iSpeed) * 0.01745329238474369;
            for (fAngle = appTimer.fDelta * v10 + fAngle; fAngle >= 6.2831854820251465;
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
                NiUpdateData UpdateData(0.0f, false, false);
                obj3D->Update(UpdateData);
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
    float fDegrees;
    fDegrees = 0.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &cAxis,
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

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("%s cannot be moved.", apRefObject->GetFormEditorID());
    }
    uint32_t CastingType = apScript->iFormID;
    if (!pDataHandler->IsFormIDCreated(CastingType)) {
        int32_t afValue_4 = apRefObject->iFormID;
        const char* afValue = apRefObject->GetFormEditorID();
        int32_t v10 = apScript->iFormID;
        Warning(
            "SCRIPTS: Script '%s' (%08X) is attempting to move reference '%s' (%08X), but it cannot be moved",
            apScript->GetFormEditorID(), v10, afValue, afValue_4
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
    int32_t aeFlags;
    aeFlags = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &eAnimGroup,
            &aeFlags
        )) {
        return false;
    }
    if (apRefObject) {
        apRefObject->AddChange(CHANGE_REFR_ANIMATION);
        if (apRefObject->GetAnimation() && apRefObject->IsActor()) {
            Actor* pactor = dynamic_cast<Actor*>(apRefObject);
            ACTION_FLAGS a4 = static_cast<ACTION_FLAGS>(aeFlags);
            uint16_t AnimGroup = pactor->GetAnimGroup(eAnimGroup, nullptr, false, nullptr);
            Animation* v9 = apRefObject->GetAnimation();
            v9->PlayGroup(AnimGroup, a4, -1, AGS_NONE);
            if (eAnimGroup) {
                Animation* v10 = apRefObject->GetAnimation();
                BSAnimGroupSequence* CurrentSequence = v10->GetCurrentSequence(AGS_MOVEMENT);
                pactor->SetAnimAction(ANIM_ACTION_FORCE_SCRIPT_ANIM, CurrentSequence);
            }
            else {
                if (pactor->GetAnimAction() == ANIM_ACTION_FORCE_SCRIPT_ANIM) {
                    pactor->SetAnimAction(ANIM_ACTION_NONE, nullptr);
                }
            }
        }
        else {
            if (eAnimGroup != ANIM_GROUP_NONE) {
                NiAVObject* pobj = apRefObject->Get3D();
                NiNode* obj3D = nullptr;
                if (pobj) {
                    obj3D = pobj->IsNode();
                }
                if (obj3D && obj3D->GetAtChecked(0)) {
                    NiObject* po = obj3D->GetAt(0)->GetControllers();
                    NiControllerManager* pmanager = static_cast<NiControllerManager*>(
                        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po)
                    );
                    if (!pmanager) {
                        po = obj3D->GetControllers();
                        pmanager = static_cast<NiControllerManager*>(
                            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po)
                        );
                    }
                    if (pmanager) {
                        NiControllerSequence* pseq = pmanager->GetSequenceByName(animGroupInfo[eAnimGroup].pName);
                        if (pseq) {
                            for (uint32_t n = 0;; ++n) {
                                uint32_t SequenceCount = pmanager->GetSequenceCount();
                                if (n >= SequenceCount) {
                                    break;
                                }
                                NiControllerSequence* pdisableSeq = pmanager->GetSequenceAt(n);
                                const char* fEaseOutTime = animGroupInfo[0].pName;
                                const char* HandleString = (&pdisableSeq->m_kName)->GetHandleString();
                                if (BSstricmp(HandleString, fEaseOutTime)) {
                                    pmanager->DeactivateSequence(pdisableSeq, 0.0f);
                                }
                            }
                            if (aeFlags) {
                                float v24 = static_cast<double>(aeFlags) * 0.10000000149011612;
                                pmanager->ActivateSequence(pseq, 0, true, 1.0f, v24, nullptr);
                            }
                            else {
                                pmanager->ActivateSequence(pseq, 0, false, 1.0f, 0.0f, nullptr);
                            }
                            pseq->SetOffset(-3.4028235e+38f);
                            pmanager->SetActive(true);
                        }
                        else {
                            int32_t fEaseOutTimeb = apRefObject->iFormID;
                            const char* a5 = apRefObject->GetFormEditorID();
                            Warning(
                                "SCRIPT: '%s' (PlayGroup Error) Sequence '%s' not found for '%s' (%08X).",
                                apScript->GetFormEditorID(), animGroupInfo[eAnimGroup].pName, a5, fEaseOutTimeb
                            );
                        }
                    }
                    else {
                        int32_t fEaseOutTimec = apRefObject->iFormID;
                        const char* a5a = apRefObject->GetFormEditorID();
                        Warning(
                            "SCRIPT: '%s' (PlayGroup Error) No NiControllerManager found for '%s' (%08X).",
                            apScript->GetFormEditorID(), a5a, fEaseOutTimec
                        );
                    }
                }
                else {
                    int32_t fEaseOutTimed = apRefObject->iFormID;
                    const char* a5b = apRefObject->GetFormEditorID();
                    Warning(
                        "SCRIPT: '%s' (PlayGroup Error) No 3d or controllers found for '%s' (%08X).",
                        apScript->GetFormEditorID(), a5b, fEaseOutTimed
                    );
                }
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
    int32_t iLoops;
    // The AnimGroup parameter (SCRIPT_PARAM_ANIM_GROUP) parses to 4 bytes; only its low word is passed on.
    ANIM_GROUP_ENUM eAnimGroup;
    ACTION_FLAGS eFlags;
    iLoops = 0;
    eFlags = static_cast<ACTION_FLAGS>(0);
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &eAnimGroup,
            &iLoops, &eFlags
        )) {
        return false;
    }
    if ((apRefObject) && (apRefObject->GetAnimation())) {
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, uiGroup
        )) {
        return false;
    }
    afReturnValue = 0.0;
    if (apRefObject) {
        if (apRefObject->GetAnimation()) {
            Animation* panim = apRefObject->GetAnimation();
            if (uiGroup[0] == 255) {
                // Counts the sections in a signed int that is passed as the section.
                for (int32_t n = AGS_IDLE; n < ANIM_GROUP_SECTION_COUNT; ++n) {
                    if (panim->GetCurrentSequence(static_cast<ANIM_GROUP_SECTION>(n))) {
                        afReturnValue = 1.0;
                    }
                }
            }
            else {
                if (panim->GetCurrentSequence(animGroupInfo[uiGroup[0]].eSectionLayer)) {
                    afReturnValue = 1.0;
                }
            }
        }
        else {
            NiNode* pnode;
            NiAVObject* pav = (NiAVObject*)apRefObject->Get3D();
            if (pav) {
                pnode = pav->IsNode();
            }
            else {
                pnode = nullptr;
            }
            if ((pnode) && (pnode->GetControllers())) {
                NiObject* po = (NiObject*)pnode->GetControllers();
                NiControllerManager* pmanager = po->IsNiControllerManager();
                if (pmanager) {
                    if (uiGroup[0] == 255) {
                        for (int32_t n_2 = 0; n_2 < pmanager->GetSequenceCount(); ++n_2) {
                            NiControllerSequence* pseq = pmanager->GetSequenceAt(static_cast<uint32_t>(n_2));
                            if ((pseq) && (pseq->GetState())) {
                                afReturnValue = 1.0;
                            }
                        }
                    }
                    else {
                        NiControllerSequence* pseq_2 = pmanager->GetSequenceByName(animGroupInfo[uiGroup[0]].pName);
                        if ((pseq_2) && (pseq_2->GetState())) {
                            afReturnValue = 1.0;
                        }
                    }
                }
            }
        }
        if (GetThreadTLSData()->bSystemOutput) {
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
    Actor* pTarget;
    pTarget = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pTarget
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pAttacker = dynamic_cast<Actor*>(apRefObject);
        if ((pAttacker) &&
            (!pAttacker->IsDead(false) &&
             (pTarget && (!pTarget->IsDead(false) && (pAttacker->pCurrentProcess && pTarget->pCurrentProcess))))) {
            CombatController* v9;
            Actor* v11;
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
            bool bagressor = !pAttacker->IsActoraGuard();
            if ((!pAttacker->IsInCombat() ||
                 (pAttacker->GetCombatController() &&
                  (v11 = pTarget, (v9 = pAttacker->GetCombatController(), !v9->IsActoraCombatTarget(v11))))) &&
                (pAttacker->pCurrentProcess)) {
                BaseProcess* pProcess = pAttacker->pCurrentProcess;
                pProcess->EnterCombat(
                    pAttacker, pTarget, true, true, nullptr, false, false, bagressor, 0, true, false, true, nullptr
                );
            }

            if (!bagressor) {
                pPlayer->SetPlayerResistingArrest(true);
                BaseProcess* CurrentAIProcess = pAttacker->pCurrentProcess;
                CurrentAIProcess->GuardCallforHelp(pAttacker, pTarget);
            }
            if (pAttacker->GetCombatController()) {
                BaseProcess* v13 = pAttacker->pCurrentProcess;
                if (v13->GetCurrentPackageSpell()) {
                    BaseProcess* v12 = pAttacker->pCurrentProcess;
                    MagicItem* v10 = v12->GetCurrentPackageSpell();
                    pAttacker->CastSpell(v10, pAttacker, false);
                }
            }
        }
    }
    return true;
}

bool Script::AddSpell_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    SpellItem* pSpell;
    pSpell = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pSpell
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pTarget = dynamic_cast<Actor*>(apRefObject);
        if (pTarget) {
            if (pTarget->AddSpell(pSpell)) {
                afReturnValue = 1.0;
                const char* FullName = pTarget->GetFullName();
                Interface::PrintLine("Actor Effect '%s' added to %s", pSpell->GetFullName(), FullName);
                if (pTarget == pPlayer) {
                    BSStringT<char> v12;
                    const char* v10 = sAddItemtoSpellList.String();
                    v12.SPrintF("%s %s", pSpell->GetFullName(), v10);
                    BSStringT<char> v13;
                    v13.SPrintF("%s", "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
                    Interface::ShowMessage(v12.pString, 0, v13.pString, nullptr, 2.0f, false);
                }
            }
            else {
                const char* v11 = pTarget->GetFullName();
                Interface::PrintLine("Actor Effect '%s' not added to %s", pSpell->GetFullName(), v11);
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
    char a8[516];
    Script::ParseParameters(
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
    );
    return true;
}

// Prints the low-level path from the player to the reference to the console and shows a magenta line from the
// player to the first door (or to the reference) for 30 seconds.
// KEEP: the "to Goal" failure message's format takes a door name and form ID, but the exe passes the format alone
// (0x5C318B: push 0x103A96C; call PrintLine), so PrintLine reads two arguments from stack the exe never wrote, which
// src cannot reproduce. The exe's cleanup-only C++ EH frame is left out.
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
    NiPoint3 v81;
    NiPoint3 v85;
    NiPoint3 v86;
    NiPoint3 result;
    using apNavMeshInfoArray_t = BSSimpleArray<const NavMeshInfo*, 1024>;
    using a8_t = BSSimpleArray<TESObjectREFR*, 1024>;
    using a7_t = BSSimpleArray<const NavMeshInfo*, 1024>;
    using apArray_t = BSSimpleArray<TESObjectREFR*, 1024>;
    int32_t v118;
    int32_t v121;
    int32_t v122;
    v121 = 0;
    v118 = 0;
    v122 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &v121, &v118,
            &v122
        )) {
        return false;
    }
    TeleportPath kPath;
    ActorLockData apActorData(pPlayer);
    PathingLocation kStart(pPlayer);
    PathingLocation kDest(apRefObject);
    if (Pathing::BuildLowPath(
            kStart, kDest, &kPath, &apActorData, static_cast<Pathing::DOOR_LOCK_DISPOSITION>(v121 != 0)
        )) {
        float v110;
        int32_t a2;
        for (a2 = 0;; ++a2) {
            NiPoint3* v18;
            uint32_t FormFlags = kPath.Doors.iSize;
            if (a2 >= FormFlags) {
                break;
            }
            if ((&kPath.Spaces.GetAt(a2))->bWorldSpace) {
                TESWorldSpace* pWorldSpace = (&kPath.Spaces.GetAt(a2))->pWorldSpace;
                int32_t afTime_4 = (&kPath.Spaces.GetAt(a2))->pWorldSpace->iFormID;
                const char* v10 = pWorldSpace->GetFormEditorID();
                Interface::PrintLine("Worldspace %s (%08x)", v10, afTime_4);
            }
            else {
                TESObjectCELL* pInteriorCell = (&kPath.Spaces.GetAt(a2))->pInteriorCell;
                int32_t afTime_4a = (&kPath.Spaces.GetAt(a2))->pInteriorCell->iFormID;
                const char* v11 = pInteriorCell->GetFormEditorID();
                Interface::PrintLine("Interior cell %s (%08x)", v11, afTime_4a);
            }
            PathingLocation v101(
                a2 ? PathingLocation(
                         &(&kPath.Doors.GetAt(a2 - 1))->DoorTeleportLocation, (&kPath.Spaces.GetAt(a2))->pInteriorCell,
                         (&kPath.Spaces.GetAt(a2))->pWorldSpace
                     )
                   : PathingLocation(pPlayer)
            );
            TESWorldSpace* afTime_4c = (&kPath.Spaces.GetAt(a2))->pWorldSpace;
            TESObjectCELL* afTimea = (&kPath.Spaces.GetAt(a2))->pInteriorCell;
            const NiPoint3* LinkedDoorTeleportPosition =
                &(&kPath.Doors.GetAt(a2))->pDoor->GetLinkedDoorTeleportPosition();
            PathingLocation v105(LinkedDoorTeleportPosition, afTimea, afTime_4c);
            char v102 = 1;
            if (v101.ResolveNavMeshInfo(nullptr)) {
                if (v105.ResolveNavMeshInfo(nullptr)) {
                    NavMeshInfoSearch apThis;
                    a7_t a7;
                    apArray_t apArray;
                    Pathing::DOOR_LOCK_DISPOSITION afTime_4d = static_cast<Pathing::DOOR_LOCK_DISPOSITION>(v121 != 0);
                    const NavMeshInfo* BestNavMeshInfo = v105.GetBestNavMeshInfo();
                    NiPoint3* Location_ = ::new (&result) NiPoint3(v105.GetLocation());
                    const NavMeshInfo* v31 = v101.GetBestNavMeshInfo();
                    if (apThis.Pathfind(
                            &apActorData, v101.GetLocation(), v31, *Location_, BestNavMeshInfo, &a7, &apArray, afTime_4d
                        )) {
                        for (uint32_t uiIndex = 0;; ++uiIndex) {
                            uint32_t v14 = apArray.iSize;
                            if (uiIndex >= v14) {
                                break;
                            }
                            if (apArray.GetAt(uiIndex)) {
                                v102 = 0;
                            }
                        }
                    }
                    else {
                        v102 = 0;
                    }
                }
                else {
                    v102 = 0;
                }
            }
            else {
                v102 = 0;
            }
            if (!v102) {
                if (a2) {
                    TESObjectREFR* pDoor = (&kPath.Doors.GetAt(a2))->pDoor;
                    TESObjectREFR* v69 = (&kPath.Doors.GetAt(a2 - 1))->pDoor;
                    int32_t afTime_4e = (&kPath.Doors.GetAt(a2))->pDoor->iFormID;
                    const char* afTimeb = pDoor->GetFormEditorID();
                    int32_t CastingType = (&kPath.Doors.GetAt(a2 - 1))->pDoor->iFormID;
                    const char* v15 = v69->GetFormEditorID();
                    Interface::PrintLine(
                        "Could not find navmesh path from Door %s (0x%08x) to Door %s (0x%08x)", v15, CastingType,
                        afTimeb, afTime_4e
                    );
                }
                else {
                    TESObjectREFR* v68 = (&kPath.Doors.GetAt(0))->pDoor;
                    int32_t afTime_4f = (&kPath.Doors.GetAt(a2))->pDoor->iFormID;
                    const char* v16 = v68->GetFormEditorID();
                    Interface::PrintLine(
                        "Could not find navmesh path from the Start to Door %s (0x%08x)", v16, afTime_4f
                    );
                }
                Interface::PrintLine("Make sure that the navmeshes on the way are finalized and connected");
            }
            float v103 = 0.0f;
            if (a2) {
                TESObjectREFR* v67 = (&kPath.Doors.GetAt(a2))->pDoor;
                const NiPoint3* v17 = &v67->GetLocationOnReference();
                v18 = ::new (&v86) NiPoint3((&kPath.Doors.GetAt(a2 - 1))->DoorTeleportLocation - *v17);
            }
            else {
                TESObjectREFR* v66 = (&kPath.Doors.GetAt(0))->pDoor;
                const NiPoint3* v19 = &v66->GetLocationOnReference();
                v18 = ::new (&v85) NiPoint3(kPath.Start - *v19);
            }
            v103 = v18->Length();
            Interface::PrintLine("Travel %f units", v103);
            TESObjectREFR* v107 = (&kPath.Doors.GetAt(a2))->pDoor;
            DoorTeleportData* Teleport = v107->GetTeleport();
            BSStringT<char> v106;
            if (v107->GetLock()) {
                LockData* Lock = v107->GetLock();
                if (Lock->IsLocked()) {
                    v106.operator+=("-Locked");
                }
            }
            if (v107->GetDisabled() || Teleport->pLinkedDoor->GetDisabled()) {
                v106.operator+=("-Disabled");
            }
            if (static_cast<TESObjectDOOR*>(v107->GetObjectReference())->TESObjectDOOR::IsMinimalUse() ||
                static_cast<TESObjectDOOR*>(Teleport->pLinkedDoor->GetObjectReference())
                    ->TESObjectDOOR::IsMinimalUse()) {
                v106.operator+=("-MinUse");
            }
            TESObjectREFR* v65 = (&kPath.Doors.GetAt(a2))->pDoor;
            const char* afTime_4g = v106.String();
            int32_t afTimec = (&kPath.Doors.GetAt(a2))->pDoor->iFormID;
            const char* v21 = v65->GetFormEditorID();
            Interface::PrintLine("Activate Door %s (%08x) (%s)", v21, afTimec, afTime_4g);
        }
        if ((&kPath.Spaces.GetAt(a2))->bWorldSpace) {
            TESWorldSpace* v64 = (&kPath.Spaces.GetAt(a2))->pWorldSpace;
            int32_t afTime_4h = (&kPath.Spaces.GetAt(a2))->pWorldSpace->iFormID;
            const char* v22 = v64->GetFormEditorID();
            Interface::PrintLine("Worldspace %s (%08x)", v22, afTime_4h);
        }
        else {
            TESObjectCELL* v63 = (&kPath.Spaces.GetAt(a2))->pInteriorCell;
            int32_t afTime_4i = (&kPath.Spaces.GetAt(a2))->pInteriorCell->iFormID;
            const char* v23 = v63->GetFormEditorID();
            Interface::PrintLine("Interior cell %s (%08x)", v23, afTime_4i);
        }
        PathingLocation v108(
            a2 ? PathingLocation(
                     &(&kPath.Doors.GetAt(a2 - 1))->DoorTeleportLocation, (&kPath.Spaces.GetAt(a2))->pInteriorCell,
                     (&kPath.Spaces.GetAt(a2))->pWorldSpace
                 )
               : PathingLocation(pPlayer)
        );
        PathingLocation v111(apRefObject);
        char v109 = 1;
        if (v108.ResolveNavMeshInfo(nullptr)) {
            if (v111.ResolveNavMeshInfo(nullptr)) {
                NavMeshInfoSearch v96;
                apNavMeshInfoArray_t apNavMeshInfoArray;
                a8_t a8;
                Pathing::DOOR_LOCK_DISPOSITION afTime_4k = static_cast<Pathing::DOOR_LOCK_DISPOSITION>(v121 != 0);
                const NavMeshInfo* v36 = v111.GetBestNavMeshInfo();
                NiPoint3* v34 = ::new (&v81) NiPoint3(v111.GetLocation());
                const NavMeshInfo* v32 = v108.GetBestNavMeshInfo();
                if (!v96.Pathfind(
                        &apActorData, v108.GetLocation(), v32, *v34, v36, &apNavMeshInfoArray, &a8, afTime_4k
                    )) {
                    v109 = 0;
                }
            }
            else {
                v109 = 0;
            }
        }
        else {
            v109 = 0;
        }
        if (!v109) {
            if (a2) {
                // No argument is passed for the format's %s and %08x, deliberately, as the game does.
                Interface::PrintLine("Could not find navmesh path from Door %s (0x%08x) to Goal");
            }
            else {
                Interface::PrintLine("Could not find navmesh path from the Start to Goal");
            }
            Interface::PrintLine("Make sure that the navmeshes on the way are finalized and connected");
        }
        if (kPath.Doors.IsEmpty()) {
            v110 = (kPath.Start - kPath.End).Length();
        }
        else {
            v110 = ((&kPath.Doors.GetAt(a2 - 1))->DoorTeleportLocation - kPath.End).Length();
        }
        Interface::PrintLine("Travel %f units", v110);
        float v112 = kPath.ComputeLength();
        TESGlobal* FormByNumericID = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(58u));
        double CurrentSpeed = pPlayer->GetCurrentSpeed();
        float v114 = v112 / CurrentSpeed;
        v114 = v114 / 3600.0;
        double v28 = FormByNumericID->fValue;
        v114 = v28 * v114;
        Interface::PrintLine("--Total distance: %.0f units.", v112);
        Interface::PrintLine("--Estimated Travel Time: %.2f game hours.", v114);
        if (kPath.Doors.IsEmpty()) {
            const NiPoint3* v39 = &apRefObject->GetLocationOnReference();
            const NiPoint3* v30 = &pPlayer->GetLocationOnReference();
            NiAVObject* Line = (NiAVObject*)MakeLine(*v30, NiColorA_Magenta, *v39, NiColorA_Magenta, true);
            TES::pInstance->AddTempDebugObject(Line, 30.0f);
        }
        else {
            TESObjectREFR* v59 = (&kPath.Doors.GetAt(0))->pDoor;
            const NiPoint3* v38 = &v59->GetLocationOnReference();
            const NiPoint3* v29 = &pPlayer->GetLocationOnReference();
            NiAVObject* apObject = (NiAVObject*)MakeLine(*v29, NiColorA_Magenta, *v38, NiColorA_Magenta, true);
            TES::pInstance->AddTempDebugObject(apObject, 30.0f);
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
    TESQuest* pQuest;
    pQuest = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pQuest
        )) {
        return false;
    }
    if (pQuest) {
        const char* v15;
        if (pQuest->pFormScript) {
            Script* ownerScript = pQuest->pFormScript;
            ScriptLocals* ownerLocals = pQuest->GetScriptLocals();
            if (ownerScript->GetVariableList()->IsEmpty()) {
                Interface::PrintLine("No script variables on quest %s", pQuest->GetEditorName());
            }
            else {
                BSSimpleList<ScriptVariable*>* pList_2 = ownerScript->GetVariableList();
                while (ownerLocals && (pList_2 && pList_2->m_item)) {
                    ScriptVariable* pVar = pList_2->m_item;
                    pList_2 = pList_2->m_pkNext;
                    if (ownerLocals->IsReferenceVariable(pVar->data.uiID)) {
                        uint32_t iid = 0;
                        double fval = ownerLocals->GetVariable(pVar->data.uiID, nullptr);
                        Script::GetNumericIDFromDouble(iid, fval);
                        uint32_t v13 = iid;
                        const char* v10 = pVar->cName.String();
                        Interface::PrintLine("%s->%s = (%08X)", ownerScript->GetFormEditorID(), v10, v13);
                    }
                    else {
                        // Tests the whole byte; a bool read would test bit 0 only.
                        if (static_cast<uint8_t>(pVar->data.bIsInteger)) {
                            int32_t Variable = static_cast<int32_t>(ownerLocals->GetVariable(pVar->data.uiID, nullptr));
                            const char* v11 = pVar->cName.String();
                            Interface::PrintLine("%s->%s = %d", ownerScript->GetFormEditorID(), v11, Variable);
                        }
                        else {
                            double v12 = ownerLocals->GetVariable(pVar->data.uiID, nullptr);
                            const char* v9 = pVar->cName.String();
                            Interface::PrintLine("%s->%s = %0.4f", ownerScript->GetFormEditorID(), v9, v12);
                        }
                    }
                }
            }
        }
        Interface::PrintLine("--- Quest state -----------------------------");
        if (pQuest->GetEnabled()) {
            v15 = "Yes";
        }
        else {
            v15 = "No";
        }
        Interface::PrintLine("Running?       %s", v15);
        Interface::PrintLine("Current stage: %d", pQuest->GetCurrentStage());
        Interface::PrintLine("Priority:      %d", pQuest->GetPriority());
    }
    return true;
}

bool Script::ShowAnim(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if ((apRefObject) && (apRefObject->GetAnimation()) && (apRefObject->IsActor())) {
        Actor* pActor = dynamic_cast<Actor*>(apRefObject);
        if (pActor) {
            const char* v20;
            Interface::PrintLine("--- Actor Variables -----------------------------");
            if (pActor->GetCurrentProcessType() != PROCESS_TYPE_NONE) {
                Interface::PrintLine("Process Level: %s", pProcessTypeString[pActor->GetCurrentProcessType()]);
            }
            BaseProcess* CurrentAIProcess = pActor->pCurrentProcess;
            BaseProcess* v23 = pActor->pCurrentProcess;
            int32_t v14 = CurrentAIProcess->GetWeaponDrawn();
            Interface::PrintLine("Wants Weapon Drawn %d, Weapon Drawn %d", v23->GetWantWeaponDrawn(), v14);
            if (pActor->GetAnimAction() != ANIM_ACTION_NONE) {
                Interface::PrintLine("Animation Action: %s", pAnimationActionStrings[pActor->GetAnimAction()]);
            }
            Interface::PrintLine("Life State: %s", LifeStateString[pActor->GetLifeState()]);
            BaseProcess* v22 = pActor->pCurrentProcess;
            Interface::PrintLine("Sit/Sleep State: %s", SleepStateString[v22->GetSitSleepState()]);
            BaseProcess* v21 = pActor->pCurrentProcess;
            Interface::PrintLine("Knock State: %s", KnockStateString[v21->GetKnockState()]);
            if (pActor->HasRagDoll()) {
                v20 = "Yes";
            }
            else {
                v20 = "No";
            }
            Interface::PrintLine("Has RagDoll: %s", v20);
            if ((pActor->IsInCombat()) && (pActor->GetCombatTarget())) {
                Actor* v8 = pActor->GetCombatTarget();
                int32_t CastingType = v8->iFormID;
                Actor* v9 = pActor->GetCombatTarget();
                Interface::PrintLine("In Combat with \"%s\" (%08x)", v9->GetFullName(), CastingType);
            }

            if (pActor->GetCurrentPackageTarget()) {
                TESObjectREFR* CurrentPackageTarget = pActor->GetCurrentPackageTarget();
                int32_t v16 = CurrentPackageTarget->iFormID;
                TESObjectREFR* v11 = pActor->GetCurrentPackageTarget();
                Interface::PrintLine("Current package target \"%s\" (%08x)", v11->GetFullName(), v16);
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
                    const char* v13 = animWeaponInfo[TESAnimGroup::GetWeapon(eGroup)];
                    Interface::PrintLine(
                        "%s -> %s/%s/%s", animGroupSectionNames[eSection], animMoveInfo[TESAnimGroup::GetMove(eGroup)],
                        v13, pName
                    );
                }
            }
            if (pAnimation->spAnimIdle.m_pObject) {
                TESIdleForm* IdleForm = pAnimation->spAnimIdle.m_pObject->pIdleForm;
                Interface::PrintLine("IdleAnim: %s", IdleForm->GetFormEditorID());
            }
            if (pAnimation->spAnimIdleQueued.m_pObject) {
                TESIdleForm* v18 = pAnimation->spAnimIdleQueued.m_pObject->pIdleForm;
                Interface::PrintLine("IdleAnim Queued: %s", v18->GetFormEditorID());
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
    int32_t bFadeIn;
    bFadeIn = 1;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &bFadeIn
        )) {
        return false;
    }
    if (!bFadeIn) {
        apRefObject->SetDisableFade(true);
    }
    if (!apRefObject) {
        goto LABEL_28;
    }
    if (!apRefObject->GetEnableStateParent()) {
        BSFadeNode* pfade;
        NiAVObject* pav;
        Script::RemoveDelayedScriptActionReference(apRefObject);
        if (apRefObject->GetDisabled()) {
            Script::AddPendingEnabledReference(apRefObject);
        }
        pav = apRefObject->Get3D();
        if (pav) {
            pfade = pav->IsFadeNode();
        }
        else {
            pfade = nullptr;
        }
        if ((pfade) && (pfade->GetMultType() == LOD_MULT_INVISIBLE)) {
            pfade->eMultType = TES::GetLODMult(apRefObject->GetObjectReference());
        }

        if (apRefObject->GetIsInvisibleLODMultType()) {
            apRefObject->SetIsInvisibleLODMultType(false);
        }
    LABEL_28:;
        apScriptLocals->m_cFlags &= ~1;
        return true;
    }
    if (apScript && apScript->iFormID) {
        const char* FullName;
        if (apRefObject->GetFormEditorIDLength()) {
            FullName = apRefObject->GetFormEditorID();
        }
        else {
            FullName = apRefObject->GetFullName();
        }
        const char* v11 = apScript->GetFormEditorID();
        int32_t CastingType = apScript->iFormID;
        Warning(
            "SCRIPTS: Enable is being called on reference %08X %s in script %08X %s even though it has an enable "
            "state parent.  This is not valid behavior and will be ignored.",
            apRefObject->iFormID, FullName, CastingType, v11
        );
    }
    else {
        const char* v14;
        if (apRefObject->GetFormEditorIDLength()) {
            v14 = apRefObject->GetFormEditorID();
        }
        else {
            v14 = apRefObject->GetFullName();
        }
        const char* v12 = apScript->GetFormEditorID();
        int32_t v10 = apScript->iFormID;
        Warning(
            "SCRIPTS: Enable is being called on reference %08X %s in a results script even though it has an enable "
            "state parent.  This is not valid behavior and will be ignored.",
            apRefObject->iFormID, v14, v10, v12
        );
    }
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
    int32_t bFadeOut;
    bFadeOut = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &bFadeOut
        )) {
        return false;
    }
    if (!apRefObject || (apRefObject->GetDisabled() || apRefObject->GetTemporary())) {
        goto LABEL_27;
    }
    if (!apRefObject->GetEnableStateParent()) {
        if (bFadeOut > 0) {
            BSFadeNode* pFadeNode;
            NiNode* pRootNode = apRefObject->Get3D();
            if (pRootNode) {
                pFadeNode = pRootNode->IsFadeNode();
            }
            else {
                pFadeNode = nullptr;
            }
            if (pFadeNode) {
                pFadeNode->ForceLODMultType(LOD_MULT_INVISIBLE);
            }
            if (!apRefObject->GetIsInvisibleLODMultType()) {
                apRefObject->SetIsInvisibleLODMultType(true);
            }
        }
        Script::AddPendingDisabledReference(apRefObject, bFadeOut != 0);
    LABEL_27:;
        apScriptLocals->m_cFlags |= 1;
        return true;
    }
    if (apScript && apScript->iFormID) {
        const char* FullName;
        if (apRefObject->GetFormEditorIDLength()) {
            FullName = apRefObject->GetFormEditorID();
        }
        else {
            FullName = apRefObject->GetFullName();
        }
        const char* v11 = apScript->GetFormEditorID();
        int32_t CastingType = apScript->iFormID;
        Warning(
            "SCRIPTS: Disable is being called on reference %08X %s in script %08X %s even though it has an enable "
            "state parent.  This is not valid behavior and will be ignored.",
            apRefObject->iFormID, FullName, CastingType, v11
        );
    }
    else {
        const char* pName;
        if (apRefObject->GetFormEditorIDLength()) {
            pName = apRefObject->GetFormEditorID();
        }
        else {
            pName = apRefObject->GetFullName();
        }
        const char* v12 = apScript->GetFormEditorID();
        int32_t v10 = apScript->iFormID;
        Warning(
            "SCRIPTS: Disable is being called on reference %08X %s in a results script even though it has an "
            "enable state parent.  This is not valid behavior and will be ignored.",
            apRefObject->iFormID, pName, v10, v12
        );
    }
    return true;
}

bool Script::SaveGame_Execute(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v9;
    char a8[516];
    v9 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8, &v9
        )) {
        return false;
    }
    pSaveLoadManager->SaveGame(a8, -1, v9 != 0);
    return true;
}

bool Script::CopySaves(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char Destination[516];
    ourStrcpy(Destination, "");
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, Destination
        )) {
        return false;
    }
    if (pSaveLoadManager) {
        if (BSstricmp(Destination, "ms")) {
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
    char result[265];
    char a8[512];
    a8[0] = 0;
    if (apParameters &&
        !Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    MessageHandler::IncDisableWarningCount(true);
    Warning("MODELS: <<< DUMPMODELMAP results");
    BSsprintf(&result[1], 260, "ModelDump%s.txt", a8);
    BSFile outfile(&result[1], NiFile::WRITE_ONLY, 0x4000u, false);
    outfile.BSFile::Open(false, false);
    if (pModelLoader) {
        pModelLoader->OutputModelMapContents(&outfile, false);
    }
    if (BSFaceGenManager::GetModelCache()) {
        BSFaceGenModelMap* ModelCache = BSFaceGenManager::GetModelCache();
        ModelCache->OutputModelMapContents(&outfile);
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
    int32_t isex;
    int32_t iuseDefaultPreset;
    isex = -1;
    iuseDefaultPreset = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &isex,
            &iuseDefaultPreset
        )) {
        return false;
    }
    Character* pactor = (Character*)pPlayer;
    if ((apRefObject) && (apRefObject->IsNpc())) {
        pactor = static_cast<Character*>(apRefObject);
    }

    if (pactor) {
        TESActorBase::SEX Sex = pactor->GetSex();
        if (Sex == isex) {
            return true;
        }
        TESNPC* pnpc = static_cast<TESNPC*>(pactor->GetObjectReference());
        TESActorBase::SEX esex = TESActorBase::SEX_MALE;
        NiAVObject* pobj = (NiAVObject*)pactor->Get3D();
        NiNode* proot = nullptr;
        if (pobj) {
            proot = pobj->IsNode();
        }
        if (isex == -1) {
            if (pactor->GetSex() == TESActorBase::SEX_MALE) {
                esex = TESActorBase::SEX_FEMALE;
            }
        }
        else {
            if (isex == 1) {
                esex = TESActorBase::SEX_FEMALE;
            }
        }
        pnpc->SetFlagBit(1, esex == TESActorBase::SEX_FEMALE, true);
        if ((pactor->IsPc()) && (iuseDefaultPreset)) {
            using presets_t = BSSimpleArray<TESNPC*, 1024>;
            presets_t presets;
            TESActorBase::SEX v11 = pnpc->GetSex();
            TESRace* Race = pnpc->GetRace();
            TESNPC::FillCharGenPresetList(Race, v11, &presets);
            TESNPC* v13 = presets.GetAt(0);
            pnpc->CopyHeadData(v13);
        }

        pactor->Reset3D();
        if (GetThreadTLSData()->bSystemOutput) {
            BSStringT<char> message;
            char* v14 = sexName[esex];
            int32_t CastingType = pactor->iFormID;
            message.SPrintF("\"%s\" (%08x) is now %s", pactor->GetFullName(), CastingType, v14);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::IsCurrentFurnitureRefConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::IsCurrentFurnitureObjConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetTalkedToPCConditionFunction(nullptr, a8, nullptr, afReturnValue);
}

bool Script::SetStage(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    int32_t v9;
    TESQuest* a8;
    afReturnValue = 0.0;
    a8 = nullptr;
    v9 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v9
        )) {
        return false;
    }
    if ((a8) && (a8->SetStageDone(v9))) {
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_GetIsCurrentWeather(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetIsVoiceTypeConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_SameFaction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_SameRace(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_SameSex(apRefObject, a8, nullptr, afReturnValue);
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
    return Cmd_SameRace(apRefObject, pPlayer, nullptr, afReturnValue);
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
    return Cmd_SameSex(apRefObject, pPlayer, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_GetQuestCompleted(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetStageConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* apParam2;
    void* a8;
    a8 = nullptr;
    apParam2 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
               &apParam2
           ) &&
        Script::GetStageDoneConditionFunction(apRefObject, a8, apParam2, afReturnValue);
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
    void* a8;
    void* apParam2;
    a8 = nullptr;
    apParam2 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
               &apParam2
           ) &&
        Script::GetFactionRankDifferenceConditionFunction(apRefObject, a8, apParam2, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetInFactionConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_GetIsClass(pPlayer, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetIsRaceConditionFunction(pPlayer, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetGlobalValueConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetShouldAttackConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetInCellConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    void* apParam2;
    apParam2 = nullptr;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
               &apParam2
           ) &&
        Script::GetInCellConditionFunction(apRefObject, a8, apParam2, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetInFactionConditionFunction(pPlayer, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetIsIDConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::IsInListConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetIsUsedItemConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetIsRefConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetFactionRankConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    int32_t iRank;
    TESFaction* pFaction;
    pFaction = nullptr;
    iRank = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pFaction,
            &iRank
        )) {
        return false;
    }
    afReturnValue = 0.0;
    Actor* pact = dynamic_cast<Actor*>(apRefObject);
    if (!pact) {
        return true;
    }
    TESActorBase* pactorbase = pact->GetActorBase();
    if ((pact) && (pFaction)) {
        pact->SetFactionRank(pFaction, iRank);
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
    Actor* ptarget;
    int32_t iValue;
    ptarget = nullptr;
    iValue = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &ptarget,
            &iValue
        )) {
        return false;
    }
    Actor* pactor = dynamic_cast<Actor*>(apRefObject);
    if (!pactor || !ptarget) {
        return false;
    }
    pactor->ModifyDispositionTowardActor(ptarget, static_cast<float>(iValue));
    return Script::GetDispositionConditionFunction(apRefObject, ptarget, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetDeadCountConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    TESObjectREFR* a8;
    int32_t v15;
    char v16 = 0;
    a8 = nullptr;
    v15 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v15
        )) {
        return false;
    }
    if ((a8) && (a8->GetMapMarkerData())) {
        MapMarkerData* MapMarkerData_ = a8->GetMapMarkerData();
        if (!MapMarkerData_->GetVisible()) {
            v16 = 1;
        }
        MapMarkerData* v10 = a8->GetMapMarkerData();
        v10->SetIsVisible(1);
        a8->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        if (v15) {
            MapMarkerData* v11 = a8->GetMapMarkerData();
            if (!v11->GetTravelLoc()) {
                v16 = 1;
            }
            MapMarkerData* v12 = a8->GetMapMarkerData();
            v12->SetTravelLoc(true);
        }
    }

    if (v16) {
        const char* v13 = sMapMarkerAdded.String();
        Interface::ShowMessage(
            v13, 0, "Interface\\Icons\\Message Icons\\glow_message_map.dds", "UIPopUpMapMarkerAdded", 2.0f, false
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
    BaseProcess* v22;
    BaseProcess* v23;
    int32_t isubtitle;
    int32_t iPCHead;
    int32_t iAlwaysPlay;
    TESActorBase* pNpc;
    TESTopic* ptopic;
    afReturnValue = 0.0;
    ptopic = nullptr;
    isubtitle = 0;
    pNpc = nullptr;
    iPCHead = 0;
    iAlwaysPlay = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &ptopic,
            &isubtitle, &pNpc, &iPCHead, &iAlwaysPlay
        )) {
        return false;
    }
    bool bplayerhead = iPCHead != 0;
    bool bforcesub = isubtitle > 0;
    bool balwaysplay = iAlwaysPlay > 0;
    Actor* pactor = dynamic_cast<Actor*>(apRefObject);
    int32_t ioldLipDistance = iSpeakSoundLipDistance.Int();
    iSpeakSoundLipDistance.operator=(INT32_MAX);
    if (!ptopic) {
        goto LABEL_37;
    }
    if (!pactor) {
        if (apRefObject && pNpc) {
            TESObjectREFR* ptempactor = TESObjectREFR::CreateReference(59, false);
            ptempactor->SetTemporary();
            ptempactor->SetObjectReference(pNpc);
            const NiPoint3* v10 = &apRefObject->GetLocationOnReference();
            ptempactor->SetLocationOnReference(*v10);
            apRefObject->SetSayToTopic(ptopic);
            DialogueItem* pVoice_3 = ptopic->CreateDialogueItem(ptempactor, nullptr, nullptr, nullptr, false);
            if (pVoice_3) {
                apRefObject->SetSayToTopicInfo(pVoice_3->pInfo);
            }
            BSSoundHandle aSoundHand_2 = ptempactor->VoiceSoundFunction(
                ptopic, static_cast<Actor*>(ptempactor), bplayerhead, balwaysplay, false, false, bforcesub
            );
            aSoundHand_2.SetCompletionCallback(
                &TESObjectREFR::SayToCallBack, reinterpret_cast<void*>(static_cast<uintptr_t>(apRefObject->iFormID))
            );
            if (ptempactor) {
                delete ptempactor;
            }
        }
        else {
            TESObjectREFR* Unk6C;
            if (apRefObject->Get3D() ||
                (apRefObject->IsMobileObject() &&
                 (Unk6C = static_cast<MobileObject*>(apRefObject)->pTalkingActivator, Unk6C->Get3D()))) {
                apRefObject->SetSayToTopic(ptopic);
                DialogueItem* pVoice_4 = ptopic->CreateDialogueItem(apRefObject, nullptr, nullptr, nullptr, false);
                if (pVoice_4) {
                    apRefObject->SetSayToTopicInfo(pVoice_4->pInfo);
                }
                BSSoundHandle aSoundHand_3 =
                    apRefObject->VoiceSoundFunction(ptopic, nullptr, bplayerhead, balwaysplay, false, false, bforcesub);
                aSoundHand_3.SetCompletionCallback(
                    &TESObjectREFR::SayToCallBack, reinterpret_cast<void*>(static_cast<uintptr_t>(apRefObject->iFormID))
                );
            }
        }
        goto LABEL_37;
    }
    if (!pactor->pCurrentProcess || pactor->IsDead(true)) {
        if (pactor->pCurrentProcess) {
            ExtraDataList* Extra = &pactor->m_Extra;
            Script::SetActionFlag(ptopic, Extra, SCRIPT_ACTION_ONSAYTODONE);
            BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
            CurrentAIProcess->SetDoingSayTo(false);
        }
        goto LABEL_37;
    }
    pactor->fSayToFailVoiceTimer = -1.0f;
    v23 = pactor->pCurrentProcess;
    v23->SetWaitingforLipFile(false);
    v22 = pactor->pCurrentProcess;
    v22->ClearDialogTarget();
    if (pactor->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
        BaseProcess* v18;
        BaseProcess* v19;
        BaseProcess* v20;
        BaseProcess* v21;
        TESObjectREFR* pcurrentScriptTarget;
        v21 = pactor->pCurrentProcess;
        pcurrentScriptTarget = v21->GetCurrentHeadTrackTarget(BaseProcess::HEAD_TRACK_TYPE_SCRIPT);
        v20 = pactor->pCurrentProcess;
        v20->ClearScriptHeadTrackTarget(false);
        v19 = pactor->pCurrentProcess;
        v19->SetActionHeadTrackTarget(pcurrentScriptTarget);
        v18 = pactor->pCurrentProcess;
        v18->ClearGreetingInfoData();
    LABEL_13:;
        if (pPlayer->IsSleepingorResting() ||
            (pactor->pCurrentProcess->eLevel ||
             (pactor->GetEssential() && pactor->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN))) {
            DialogueItem* pVoice_2 = ptopic->CreateDialogueItem(pactor, nullptr, nullptr, nullptr, true);
            if (pVoice_2) {
                pVoice_2->FirstResponse();
                DialogueResponse* pResponse = pVoice_2->GetCurrentResponse();
                pVoice_2->RunResult(TIRS_BEGIN);
                pVoice_2->RunResult(TIRS_END);
                ExtraDataList* v13 = &pactor->m_Extra;
                Script::SetActionFlag(ptopic, v13, SCRIPT_ACTION_ONSAYTODONE);
                delete pVoice_2;
            }
        }
        else {
            BaseProcess* v17 = pactor->pCurrentProcess;
            v17->ProcessGreet(pactor, ptopic, bforcesub, false, true, true);
        }
    LABEL_37:;
        iSpeakSoundLipDistance.operator=(ioldLipDistance);
        return true;
    }
    if (!iPCHead) {
        goto LABEL_13;
    }
    pactor->SetSayToTopic(ptopic);
    DialogueItem* pVoice = ptopic->CreateDialogueItem(pactor, nullptr, nullptr, nullptr, false);
    if (pVoice) {
        apRefObject->SetSayToTopicInfo(pVoice->pInfo);
    }
    BSSoundHandle aSoundHand = pactor->VoiceSoundFunctionEx(ptopic, pactor, bplayerhead, balwaysplay, 0, false);
    aSoundHand.SetCompletionCallback(
        &TESObjectREFR::SayToCallBack, reinterpret_cast<void*>(static_cast<uintptr_t>(apRefObject->iFormID))
    );
    return true;
}

bool Script::PlayVATSCameras(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char a8[512];
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8
        )) {
        return false;
    }
    TESForm* FormByEditorID = TESForm::GetFormByEditorID(a8);
    pForceCameraPath = dynamic_cast<BGSCameraPath*>(FormByEditorID);
    if (GetThreadTLSData()->bSystemOutput) {
        if (pForceCameraPath) {
            Interface::PrintLine("PlayVATSCameras '%s' queued for next VATS playback.", a8);
        }
        else {
            Interface::PrintLine("PlayVATSCameras '%s' not found.", a8);
        }
    }
    return true;
}

bool Script::ShowQuestLog(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    bool bShowCompleted = a8 != 0;
    BSSimpleList<TESQuestStageItem*> kQuestLog;
    TESQuest::GetQuestLog(&kQuestLog, nullptr, bShowCompleted, false, false);
    BSSimpleList<TESQuestStageItem*>* pIter = &kQuestLog;
    while (pIter && pIter->GetItem()) {
        TESQuestStageItem* pItem = pIter->GetItem();
        TESQuest* pQuest = pItem->m_pOwner;
        pIter = pIter->GetNext();
        char acLogEntry[260];
        ourMemset(acLogEntry, 0, sizeof(acLogEntry));
        mbsnbcpy_(acLogEntry, pItem->GetLogEntry(pQuest), sizeof(acLogEntry) - 1);
        Date* pDate = pItem->m_pLogDate;
        Interface::PrintLine("------------------------------------------------");
        uint16_t usYear = pDate->GetYear();
        const char* pMonthName = pGSMonth[pDate->GetMonth()]->String();
        uint16_t usDay = pDate->GetDay();
        Interface::PrintLine("%d of %s, %d", usDay, pMonthName, usYear);
        Interface::PrintLine("%s", pItem->m_pOwner->TESFullName::GetFullName());
        Interface::PrintLine("%s", acLogEntry);
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
    int32_t iCount;
    int32_t iLockAsOwner;
    if (!apRefObject) {
        return false;
    }
    iCount = 0;
    iLockAsOwner = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iCount,
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
            LOCK_LEVEL ell;
            BGSTerminal* pterminal_2 = static_cast<BGSTerminal*>(apRefObject->GetObjectReference());
            if (iCount >= 0) {
                if (iCount < 6) {
                    ell = static_cast<LOCK_LEVEL>(iCount);
                }
                else {
                    ell = LL_IMPOSSIBLE;
                }
            }
            else {
                ell = LL_VERYEASY;
            }
            pterminal_2->SetRawDifficulty(apRefObject, ell);
            TESObjectREFR::SetTerminalUnlocked(apRefObject, 0);
            iCount = ell;
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
            DoorTeleportData* ptel = apRefObject->GetTeleport();
            if (ptel) {
                ptel->SetConnectedCellsPublic(apRefObject, false);
            }
        }
        apRefObject->SetAltered(true);
    }
    if (GetThreadTLSData()->bSystemOutput) {
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
    int32_t iUnlockAsOwner;
    if (!apRefObject) {
        return false;
    }
    iUnlockAsOwner = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals,
            &iUnlockAsOwner
        )) {
        return false;
    }
    LockData* pLock = apRefObject->GetLock();
    if (pLock) {
        pLock->SetLocked(false);
        apRefObject->AddLockChange();
        if (iUnlockAsOwner > 0) {
            DoorTeleportData* ptel = apRefObject->GetTeleport();
            if (ptel) {
                ptel->SetConnectedCellsPublic(apRefObject, true);
            }
        }
        apRefObject->SetAltered(true);
    }
    else {
        if (apRefObject->GetObjectReference()->GetFormType() == TERM_ID) {
            static_cast<BGSTerminal*>(apRefObject->GetObjectReference())->BGSTerminal::SetUnlocked(apRefObject);
            BGSTerminal::SetLockedOut(apRefObject, 0);
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
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
    Actor* pCriminal;
    Actor* pVictim;
    uint32_t a8;
    afReturnValue = 0.0;
    a8 = 0;
    pCriminal = nullptr;
    pVictim = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
            &pCriminal, &pVictim
        )) {
        return false;
    }
    PackageNS::CRIME_TYPE v10 = static_cast<PackageNS::CRIME_TYPE>(a8);
    Actor* v9 = dynamic_cast<Actor*>(apRefObject);
    if (ReferenceProcessLists.IsCrimeinGlobalCrimeList(pVictim, pCriminal, v9, v10, nullptr, -1)) {
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
    void* a8;
    int32_t iCrime;
    afReturnValue = 0.0;
    iCrime = -1;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &iCrime
        )) {
        return false;
    }
    if (apRefObject) {
        return Script::GetCrimeConditionFunction(
            apRefObject, a8, reinterpret_cast<void*>(static_cast<intptr_t>(iCrime)), afReturnValue
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
    int32_t a8;
    a8 = 1;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    TESObjectREFR* apTargetRef = nullptr;
    if (pPlayer->GetActiveQuest()) {
        BSSimpleList<TESQuestTarget*>* ecx0 = pPlayer->GetCurrentTargetList();
        if (!ecx0) {
            Interface::PrintLine("No current targets");
        }
        for (int32_t i = 0; ecx0 && (ecx0->m_item && i < a8); ++i) {
            TESQuestTarget* v9 = ecx0->m_item;
            ecx0 = ecx0->m_pkNext;
            apTargetRef = v9->GetReference(true);
        }
    }
    else {
        Interface::PrintLine("No active quest");
    }
    if (apTargetRef) {
        Script::MoveToFunctionBase(pPlayer, apTargetRef, 0.0f, 0.0f, 0.0f);
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
    int32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (pPlayer->IsSleeping()) {
        if (a8 < 0) {
            a8 = 0;
        }
        pPlayer->SetSleeping(a8, true);
        if (!a8) {
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
    TESCombatStyle* pcombatStyle;
    pcombatStyle = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pcombatStyle
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pactor = dynamic_cast<Actor*>(apRefObject);
        if (pactor) {
            pactor->SetCombatStyle(pcombatStyle);
            if (pactor->IsPlayerTeammate()) {
                if (pactor->IsCreature()) {
                    static_cast<TESCreature*>(pactor->GetObjectReference())
                        ->TESCreature::InitDefaultWorn(pactor, true, true, true);
                }
                else {
                    TESNPC* pnpc = static_cast<TESNPC*>(pactor->GetObjectReference());
                    BipedAnim* v9 = pactor->GetBiped();
                    pnpc->InitParts(pactor, v9, true);
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
    TESForm* pparamForm;
    pparamForm = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pparamForm
        )) {
        return false;
    }
    if (pparamForm) {
        SpellItem* pspell = dynamic_cast<SpellItem*>(pparamForm);
        TESObjectBOOK* pbook = dynamic_cast<TESObjectBOOK*>(pparamForm);
        if (pbook) {
            pPlayer->SetSelectedScroll(pbook);
        }
        else {
            if (pspell) {
                pPlayer->SetSelectedSpell(pspell);
            }
            else {
                pPlayer->SetSelectedSpell(nullptr);
            }
        }
        if (GetThreadTLSData()->bSystemOutput) {
            const char* FullName;
            if (pspell) {
                FullName = pspell->GetFullName();
            }
            else {
                FullName = "NONE";
            }
            Interface::PrintLine("Player Spell set to %s", FullName);
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
    TESActorBase* pactorBase;
    int32_t v10;
    afReturnValue = 0.0;
    pactorBase = nullptr;
    v10 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pactorBase,
            &v10
        )) {
        return false;
    }
    if (pactorBase) {
        pactorBase->SetEssential(v10 > 0);
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
    TESObject* pobject;
    int32_t v10;
    afReturnValue = 0.0;
    pobject = nullptr;
    v10 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pobject,
            &v10
        )) {
        return false;
    }
    if (pobject) {
        pobject->SetQuestObject(v10 > 0);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetDetectionLevelConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    TESPackage* pPackage;
    pPackage = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pPackage
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if ((pActor) && (pPackage)) {
        TESPackage* PackageExtra;
        TESPackage* ActiveStablePackage = pActor->GetPackageSetAsPcurrent();
        if (ActiveStablePackage == pPackage ||
            (PackageExtra = (&pActor->m_Extra)->GetPackageExtra(), PackageExtra == pPackage)) {
            if (pPackage->GetPackType() == PACKAGE_TYPE_ESCORT) {
                pPackage->SetWaiting(false);
                BaseProcess* CurrentAIProcess = pActor->pCurrentProcess;
                CurrentAIProcess->SetProcedureIndexRunning(3);
            }
            else {
                if (pPackage->GetPackType() == PACKAGE_TYPE_ACCOMPANY ||
                    pPackage->GetPackType() == PACKAGE_TYPE_FOLLOW) {
                    int32_t NumberOfFollowers;
                    BaseProcess* v15 = pActor->pCurrentProcess;
                    TESObjectREFR* v11 = v15->GetTarget();
                    if (dynamic_cast<Actor*>(v11) != pPlayer ||
                        (pPlayer->IsPlayerFollower(pActor) ||
                         (NumberOfFollowers = pPlayer->GetNumberOfFollowers(),
                          NumberOfFollowers <= iNumberActorsAllowedToFollowPlayer.Int()))) {
                        pPackage->SetWaiting(false);
                    }
                    else {
                        const char* v13 = sNoMoreFollowers.String();
                        Interface::ShowMessage(v13, 0, nullptr, nullptr, 2.0f, false);
                    }
                }
                else {
                    Warning("PACKAGES: Package %s is not a follow or an escort. ", pPackage->GetFormEditorID());
                }
            }
        }
        else {
            const char* FullName = pActor->GetFullName();
            Warning("PACKAGES: Package %s is not  %s current package", pPackage->GetFormEditorID(), FullName);
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
    int32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    pPlayer->SetSleeping(a8, true);
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
    int32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    pPlayer->ModAmountStolenSold(a8);
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
    void* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    Cmd_GetPCExpelled(apRefObject, a8, nullptr, afReturnValue);
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
    TESFaction* a8;
    int32_t v11;
    a8 = nullptr;
    v11 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v11
        )) {
        return false;
    }
    if (v11) {
        a8->SetPCExpelled(true);
        TESObjectCELL* pcell = pPlayer->GetParentCell();
        if (pcell) {
            pcell->GetOwner();
        }
    }
    else {
        a8->SetPCExpelled(false);
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
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject) {
        if (a8) {
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
    TESObjectREFR* pSecuritron;
    char asPersonality[516];
    char asMood[516];
    pSecuritron = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pSecuritron,
            asPersonality, asMood
        )) {
        return false;
    }
    if (pSecuritron) {
        NiAVObject* pMesh = (NiAVObject*)pSecuritron->Get3D();
        if (pMesh) {
            ExtraSecuritronFace* pFaceExtra = static_cast<ExtraSecuritronFace*>(pSecuritron->GetExtraData(143));
            if (!pFaceExtra) {
                ExtraSecuritronFace* pFaceExtra_2;
                ExtraSecuritronFace* apMemory =
                    static_cast<ExtraSecuritronFace*>(ourMalloc(sizeof(ExtraSecuritronFace)));
                if (apMemory) {
                    pFaceExtra_2 = ::new (apMemory) ExtraSecuritronFace;
                }
                else {
                    pFaceExtra_2 = nullptr;
                }
                pFaceExtra = pFaceExtra_2;
                (&pSecuritron->m_Extra)->AddExtra(pFaceExtra_2);
            }
            if (pFaceExtra) {
                pFaceExtra->SetPersonality(asPersonality);
                pFaceExtra->SetMood(asMood);
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
    TESObjectREFR* pRef;
    char GeometryName[512];
    char TextureName[512];
    char path[260];
    pRef = nullptr;
    BSStringT<char> BSGeometryName;
    BSStringT<char> BSTextureName;
    if (Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pRef,
            GeometryName, TextureName
        ) &&
        ((&BSGeometryName.operator=(GeometryName),
          (&BSTextureName.operator=(TextureName), BSGeometryName.GetLength())) &&
         BSTextureName.GetLength())) {
        if (pRef) {
            NiAVObject* pMesh = (NiAVObject*)pRef->Get3D();
            if (pMesh) {
                NiAVObject* pGeom = pMesh->GetObjectByName(GeometryName);
                if (pGeom) {
                    BSShaderPPLightingProperty* pLightShader;
                    int32_t v21;
                    char apDest[264];
                    NiPointer<NiSourceTexture> TexPtr(static_cast<NiSourceTexture*>(nullptr));
                    BSsprintf(apDest, 260, "Textures\\%s.dds", BSTextureName.String());
                    SwapPlatformLanguageTexturePath(apDest, path, 260u);
                    TES::pInstance->CreateTextureImage(path, TexPtr, false, false);
                    NiProperty* Property = pGeom->GetProperty(3);
                    if (NiObject::DynamicCastStatic(&NiShadeProperty::ms_RTTI, Property)) {
                        NiProperty* v10 = pGeom->GetProperty(3);
                        NiShadeProperty* v11 =
                            static_cast<NiShadeProperty*>(NiObject::DynamicCastStatic(&NiShadeProperty::ms_RTTI, v10));
                        int32_t v20 = false;
                        if (v11->GetShaderPropertyType() >= 8) {
                            NiProperty* v12 = pGeom->GetProperty(3);
                            NiShadeProperty* v13 = static_cast<NiShadeProperty*>(
                                NiObject::DynamicCastStatic(&NiShadeProperty::ms_RTTI, v12)
                            );
                            if (v13->GetShaderPropertyType() <= 12) {
                                v20 = true;
                            }
                        }
                        v21 = v20;
                    }
                    else {
                        v21 = 0;
                    }
                    if (v21) {
                        NiProperty* v14 = pGeom->GetProperty(3);
                        pLightShader = static_cast<BSShaderPPLightingProperty*>(
                            NiObject::DynamicCastStatic(&NiShadeProperty::ms_RTTI, v14)
                        );
                    }
                    else {
                        pLightShader = nullptr;
                    }
                    if (pLightShader) {
                        NiSourceTexture* v17 = TexPtr.m_pObject;
                        pLightShader->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v17);
                    }
                    int32_t Type = NiShadeProperty::GetType();
                    NiProperty* v16 = pGeom->GetProperty(Type);
                    BSShaderNoLightingProperty* pNoLightShader = static_cast<BSShaderNoLightingProperty*>(
                        NiObject::DynamicCastStatic(&BSShaderNoLightingProperty::ms_RTTI, v16)
                    );
                    if (pNoLightShader) {
                        NiSourceTexture* v18 = TexPtr.m_pObject;
                        pNoLightShader->SetBaseMap(v18);
                    }
                }
            }
        }
        return true;
    }

    return false;
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
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pactor = dynamic_cast<Actor*>(apRefObject);
        if (pactor) {
            if (a8) {
                pactor->StopCombat(nullptr);
                if ((!pactor->IsUnConscious()) && (pactor->pCurrentProcess)) {
                    BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
                    CurrentAIProcess->AddPostAnimationAction(BaseProcess::POSTANIM_FREEUP_SPECIAL_IDLE);
                    BaseProcess* v12 = pactor->pCurrentProcess;
                    v12->SetCurrentProcessIdle(nullptr);
                }

                pactor->SetUnConscious(true);
                pactor->EndMovement();
                if (pactor->pCurrentProcess) {
                    BaseProcess* v11 = pactor->pCurrentProcess;
                    v11->SetIdleTimer(-1.0f);
                }
            }
            else {
                if ((pactor->IsUnConscious()) && (pactor->pCurrentProcess)) {
                    BaseProcess* v10 = pactor->pCurrentProcess;
                    v10->AddPostAnimationAction(BaseProcess::POSTANIM_FREEUP_SPECIAL_IDLE);
                    BaseProcess* v9 = pactor->pCurrentProcess;
                    v9->SetCurrentProcessIdle(nullptr);
                }

                pactor->SetUnConscious(false);
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
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject) {
        Actor* pactor = dynamic_cast<Actor*>(apRefObject);
        if (pactor) {
            if (a8) {
                pactor->SetRestrained(true);
                pactor->EndMovement();
            }
            else {
                pactor->SetRestrained(false);
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
    int32_t bCanFastTravel;
    int32_t bKeepOnCellChange;
    int32_t bCanWait;
    bCanFastTravel = 0;
    bCanWait = 1;
    bKeepOnCellChange = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals,
            &bCanFastTravel, &bCanWait, &bKeepOnCellChange
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
    MiscStatManager::MiscStatID a8;
    int32_t iamount;
    a8 = MiscStatManager::MS_QUESTS_COMPLETED;
    iamount = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &iamount
        )) {
        return false;
    }
    MiscStatManager::ModVal(a8, iamount);
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
    void* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    Cmd_GetPCMiscStat(apRefObject, a8, nullptr, afReturnValue);
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
    int32_t ibool;
    ibool = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &ibool
        )) {
        return false;
    }
    Actor* pactor = dynamic_cast<Actor*>(apRefObject);
    if (pactor) {
        bool bnorumors = ibool != 0;
        int32_t v9 = ibool != 0;
        if (v9 == pactor->GetActorBase()->GetNoRumors()) {
            (&pactor->m_Extra)->ClearRumorsExtra();
        }
        else {
            (&pactor->m_Extra)->SetNoRumors(bnorumors);
        }
        pactor->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
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
    float a8;
    a8 = 1.0f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    float afStrength = a8;
    TESImageSpaceModifier* Hit = TESImageSpaceModifier::GetGetHit();
    ImageSpaceModifierInstanceForm::Trigger(Hit, afStrength, nullptr);
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
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (!g_bHasPlayerUsedConsole) {
        int32_t v10 = a8;
        BSSteamAwardsSystemUtility* v9 = BSSteamAwardsSystemUtility::QInstance();
        v9->Unlock(v10);
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
    int32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    ReferenceProcessLists.FlushNonPersistActors(a8);
    if (GetThreadTLSData()->bSystemOutput) {
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
    char IsMapMenuFogOfWar = GetIsMapMenuFogOfWar();
    ToggleMapFogOfWar(IsMapMenuFogOfWar == 0);
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v10;
        if (GetIsMapMenuFogOfWar()) {
            v10 = "ENABLED";
        }
        else {
            v10 = "DISABLED";
        }
        Interface::PrintLine("Fog of war - %s.", v10);
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
    float a8;
    a8 = 0.0f;
    return Script::ParseParameters(
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
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
    Actor* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    Actor* pSender = dynamic_cast<Actor*>(apRefObject);
    if ((a8) && (pSender)) {
        TESBoundObject* ObjectReference = pSender->GetObjectReference();
        a8->TrespassAlarm(pSender, ObjectReference, -1);
    }

    return true;
}

bool Script::ForceGroupStrategy(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    char Destination[516];
    ourStrcpy(Destination, "");
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, Destination
        )) {
        return false;
    }
    if (apRefObject->IsActor()) {
        CombatController* pController = static_cast<Actor*>(apRefObject)->GetCombatController();
        if (pController) {
            CombatGroup* CombatGroup_ = pController->GetCombatGroup();
            if (CombatGroup_->GetMemberCount() >= 2) {
                if (std::strlen(Destination)) {
                    CombatGroupStrategy* apStrategy = CombatGroupStrategy::LookupByName(Destination);
                    if (apStrategy) {
                        CombatGroup_->ForceGroupStrategy(apStrategy);
                        if (GetThreadTLSData()->bSystemOutput) {
                            Interface::PrintLine("Group strategy set to: %s", apStrategy->GetName());
                        }
                    }
                    else {
                        if (GetThreadTLSData()->bSystemOutput) {
                            Interface::PrintLine("Unknown strategy '%s'", Destination);
                        }
                    }
                }
                else {
                    CombatGroup_->ForceGroupStrategy(nullptr);
                    if (GetThreadTLSData()->bSystemOutput) {
                        Interface::PrintLine("Group strategy cleared");
                    }
                }
                return true;
            }

            Interface::PrintLine("The combat group has only one member");
            return true;
        }

        const char* FullName;
        if (apRefObject->GetFormEditorIDLength()) {
            FullName = apRefObject->GetFormEditorID();
        }
        else {
            FullName = apRefObject->GetFullName();
        }
        Interface::PrintLine("%s is not in combat", FullName);
        return true;
    }

    const char* v10;
    if (apRefObject->GetFormEditorIDLength()) {
        v10 = apRefObject->GetFormEditorID();
    }
    else {
        v10 = apRefObject->GetFullName();
    }
    Interface::PrintLine("%s is not an actor", v10);
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
    Actor* a8;
    int32_t aiDamage;
    a8 = nullptr;
    aiDamage = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
            &aiDamage
        )) {
        return false;
    }
    if (a8->IsActor()) {
        Actor* v13 = a8;
        float fSpeed = CombatFormulas::GetKnockbackSpeed(v13, aiDamage);
        BaseProcess* pProcess = v13->GetCurrentAIProcess();
        if (pProcess && pProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
            if (apRefObject == pPlayer) {
                TESBoundObject* pWeapon;
                ItemChange* v12 = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
                if (v12) {
                    pWeapon = v12->pContainerObj;
                }
                else {
                    pWeapon = nullptr;
                }
                TESChallengeManager::UpdateChallenge(
                    CHALLENGE_TYPE_HIT, 1, v13->GetObjectReference(), pWeapon, 0xFFFF, 2, 0
                );
            }
            pProcess->KnockExplosion(v13, apRefObject->GetLocationOnReference(), fSpeed);
        }
    }
    else {
        if (apScript) {
            Warning(
                "SCRIPTS: PushActorAway in script '%s' is attempting to push a non-actor reference.",
                apScript->GetFormEditorID()
            );
        }
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
    int32_t ival;
    int32_t iindex;
    TESQuest* pquest;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pquest,
            &iindex, &ival
        )) {
        return false;
    }
    if (pquest) {
        BGSQuestObjective* pobjective = pquest->GetObjective(static_cast<uint32_t>(iindex));
        if (pobjective) {
            double v9;
            if (pobjective->GetCompleted()) {
                v9 = 1.0;
            }
            else {
                v9 = 0.0;
            }
            afReturnValue = v9;
            if (GetThreadTLSData()->bSystemOutput) {
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
    int32_t ival;
    int32_t iindex;
    TESQuest* pquest;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pquest,
            &iindex, &ival
        )) {
        return false;
    }
    if (pquest) {
        BGSQuestObjective* pobjective = pquest->GetObjective(static_cast<uint32_t>(iindex));
        if (pobjective) {
            double v9;
            if (pobjective->GetDisplayed()) {
                v9 = 1.0;
            }
            else {
                v9 = 0.0;
            }
            afReturnValue = v9;
            if (GetThreadTLSData()->bSystemOutput) {
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
    char a8[524];
    TESObjectREFR* pTarg;
    pTarg = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8, &pTarg
        )) {
        return false;
    }
    a8[523] = a8[0] == 49;
    bool v9 = a8[0] == 48;
    if (a8[0] != 49 && (BSstricmp(a8, "enable") && BSstricmp(a8, "on"))) {
        if (!v9 && (BSstricmp(a8, "disable") && BSstricmp(a8, "off"))) {
            if (!BSstricmp(a8, "tune")) {
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
    TESObjectREFR* pstation;
    uint32_t a8[2];
    pstation = nullptr;
    a8[1] = 0;
    a8[0] = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, a8, &pstation
        )) {
        return false;
    }
    if (!apRefObject || (!apRefObject->IsActor() || !pstation)) {
        return false;
    }
    if (a8[0] == 1) {
        FalloutRadio::EnableNPCRadio(apRefObject, pstation);
    }
    else {
        if (!a8[0]) {
            FalloutRadio::DisableNPCRadio(apRefObject);
        }
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
    TESTopic* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    FalloutRadio::StartRadioConversation(apRefObject, a8);
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
    char Str[516];
    float z_2;
    int32_t icolor;
    float x;
    float y;
    // Zeroes the first byte and memsets the next 511, which is the `= {0}` of a 512-byte buffer.
    Str[0] = 0;
    std::memset(&Str[1], 0, 511);
    icolor = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &x, &y, &z_2,
            Str, &icolor
        )) {
        return false;
    }
    if (pPlayer->GetParentCell()) {
        NiPoint3 location(x, y, z_2);
        if (std::strlen(Str)) {
            const NiColorA* pColor = &BSTestObjects::GetColor(icolor);
            pPlayer->GetParentCell()->AddLocationMarker(location, Str, pColor);
        }
        else {
            const NiColorA* pColor = &BSTestObjects::GetColor(icolor);
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
    uint8_t formtype[2];
    formtype[1] = 1;
    formtype[0] = 0;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, formtype
           ) &&
        Script::GetIsObjectTypeConditionFunction(
               apRefObject, reinterpret_cast<void*>(static_cast<uintptr_t>(formtype[0])), nullptr, afReturnValue
        );
}

bool Script::MarkForDelete(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if ((apRefObject) && (!apRefObject->GetDelete() && apRefObject != pPlayer)) {
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
    ActorValue::Index a8;
    int32_t a2;
    a8 = ActorValue::VALUE_NONE;
    a2 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &a2
        )) {
        return false;
    }
    if (ActorValue::IsSkill(a8) && a2 >= 0 && a2 < TESClass::GetTagSkillCount()) {
        ActorValue::Index v11 = a8;
        int32_t v10 = a2;
        TESClass* Class = pPlayer->GetClass();
        Class->SetMajorSkill(v10, v11);
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
    ActorValue::Index a8;
    a8 = ActorValue::VALUE_NONE;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    afReturnValue = 0.0;
    if (ActorValue::IsSkill(a8)) {
        ActorValue::Index v10 = a8;
        TESClass* Class = pPlayer->GetClass();
        if (Class->IsTagSkill(v10)) {
            afReturnValue = 1.0;
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::IsPlayerGrabbedRefConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    int32_t aiVal;
    TESActorBase* a8;
    BGSEncounterZone* pforcezone;
    afReturnValue = 0.0;
    a8 = nullptr;
    aiVal = 4;
    pforcezone = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &aiVal,
            &pforcezone
        )) {
        return false;
    }
    if ((a8) && (a8->IsActorBase()) && (apRefObject)) {
        TESActorBase* pbase = a8;
        Actor* pplacedref = nullptr;
        int32_t FormType = a8->GetFormType();
        if (FormType == 42) {
            Actor* pplacedref_2;
            Character* apMemory = static_cast<Character*>(ourMalloc(sizeof(Character)));
            if (apMemory) {
                pplacedref_2 = ::new (apMemory) Character;
            }
            else {
                pplacedref_2 = nullptr;
            }
            pplacedref = pplacedref_2;
        }
        else {
            if (FormType == 43) {
                Actor* pplacedref_3;
                Creature* v15 = static_cast<Creature*>(ourMalloc(sizeof(Creature)));
                if (v15) {
                    pplacedref_3 = ::new (v15) Creature;
                }
                else {
                    pplacedref_3 = nullptr;
                }
                pplacedref = pplacedref_3;
            }
        }
        if (pplacedref) {
            if (pforcezone) {
                pplacedref->SetEncounterZone(pforcezone);
            }
            (&pplacedref->m_Extra)->SetLevCreaModifier(aiVal);
            pplacedref->AddChange(CHANGE_REFR_EXTRA_CREATED_ONLY);
            pplacedref->SetParentCell(apRefObject->GetParentCell());
            pplacedref->SetObjectReference(pbase);
            if (pbase->GetUsesLeveledTemplate()) {
                pbase->CalcTemplateForRef(pplacedref);
            }
            TESObjectCELL* pint_2 = apRefObject->GetParentCell();
            if ((pint_2) && (!pint_2->GetInterior())) {
                pint_2 = nullptr;
            }

            TESWorldSpace* pworld = apRefObject->GetWorldSpace();
            const NiPoint3* AngleOnReference = &apRefObject->GetAngleOnReference();
            const NiPoint3* v10 = &apRefObject->GetLocationOnReference();
            TESBoundObject* ObjectReference = pplacedref->GetObjectReference();
            pDataHandler->CreateReferenceAtLocation(
                ObjectReference, *v10, *AngleOnReference, pint_2, pworld, pplacedref, nullptr, nullptr
            );
            pplacedref->SetStillLoading(false);
            uint32_t iid = pplacedref->iFormID;
            Script::PutNumericIDInDouble(iid, afReturnValue);
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
    TESQuest* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (a8) {
        pPlayer->SetActiveQuest(a8);
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
    int32_t ival;
    TESObjectREFR* a8;
    a8 = nullptr;
    ival = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &ival
        )) {
        return false;
    }
    if (iNumHWThreads.Int() <= 1) {
        Script::MatchFaceGeometryBase(apRefObject, a8, ival);
    }
    else {
        TaskQueueInterface::GetInstance()->QueueScriptFunctionCall(0x11DFu, apRefObject, a8, ival);
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
    int32_t iEquipType;
    iEquipType = -1;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals,
               &iEquipType
           ) &&
        Script::GetIsUsedItemEquipTypeConditionFunction(
               apRefObject, reinterpret_cast<void*>(static_cast<intptr_t>(iEquipType)), nullptr, afReturnValue
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
    int32_t aiCount;
    float v18;
    int32_t v19;
    TESForm* v20;
    v20 = nullptr;
    aiCount = 0;
    v18 = 0.0f;
    v19 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &v20,
            &aiCount, &v18, &v19
        )) {
        return false;
    }
    if (apRefObject) {
        uint32_t CalcLevel = apRefObject->GetCalcLevel(false);
        TESContainer kContainer;
        TESLevItem* pLevItem = nullptr;
        BGSListForm* pListForm = nullptr;
        TESBoundObject* v12 = nullptr;
        int32_t FormType = v20->GetFormType();
        if (FormType == LVLI_ID) {
            pLevItem = static_cast<TESLevItem*>(v20);
        }
        else {
            if (FormType == FLST_ID) {
                pListForm = static_cast<BGSListForm*>(v20);
            }
            else {
                if (v20->IsBoundObject()) {
                    v12 = static_cast<TESBoundObject*>(v20);
                }
            }
        }
        if (pLevItem) {
            pLevItem->CalculateCurrentFormList(CalcLevel, aiCount, &kContainer, 0);
        }
        else {
            if (pListForm && aiCount) {
                BSSimpleList<TESForm*>* pIter = pListForm->GetFormList();
                while (pIter && !pIter->IsEmpty()) {
                    TESForm* pForm = pIter->GetItem();
                    pIter = pIter->GetNext();
                    if (pForm->IsBoundObject()) {
                        kContainer.AddObject(static_cast<TESBoundObject*>(pForm), aiCount, nullptr);
                    }
                }
            }
            else {
                if (v12 && aiCount) {
                    kContainer.AddObject(v12, aiCount, nullptr);
                }
                else {
                    Warning("SCRIPTS: AddItem in script '%s' failed to generate an item.", apScript->GetFormEditorID());
                }
            }
        }
        kContainer.ApplyHealthPercentToContainer(v18);
        kContainer.AddContainerToReference(apRefObject, v19 == 1);
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
    TESFaction* apFaction;
    PlayerCharacter* a8;
    TES* v17 = nullptr;
    a8 = nullptr;
    apFaction = nullptr;
    Script::ParseParameters(
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &apFaction
    );
    if (apRefObject->IsActor()) {
        PlayerCharacter* v18 = a8;
        if (a8 || apFaction) {
            if (v18 && (v18 != pPlayer && v18->pCurrentProcess)) {
                v18->AttackAlarm(pPlayer, false, true);
                BaseProcess* CurrentAIProcess = v18->pCurrentProcess;
                CurrentAIProcess->EnterCombat(
                    v18, pPlayer, true, false, nullptr, false, false, false, 0, false, false, true, nullptr
                );
            }
            else {
                if (apFaction) {
                    Actor* ActorRefInHigh = ReferenceProcessLists.GetActorRefInHigh(apFaction, true, true);
                    if (ActorRefInHigh) {
                        if (ActorRefInHigh->IsCreature()) {
                            if (ActorRefInHigh->GetOwner()) {
                                Actor* ActorRefInHigh_0;
                                TESForm* Owner = ActorRefInHigh->GetOwner();
                                if (Owner->GetFormType() == FACT_ID) {
                                    ActorRefInHigh_0 = ReferenceProcessLists.GetActorRefInHigh(
                                        static_cast<TESFaction*>(Owner), false, true
                                    );
                                }
                                else {
                                    ActorRefInHigh_0 =
                                        ReferenceProcessLists.GetActorRefInHigh(static_cast<TESNPC*>(Owner), false);
                                }
                                if (ActorRefInHigh_0) {
                                    ActorRefInHigh_0->AttackAlarm(pPlayer, false, true);
                                    BaseProcess* v10 = ActorRefInHigh->pCurrentProcess;
                                    v10->EnterCombat(
                                        ActorRefInHigh, pPlayer, true, false, nullptr, false, false, false, 0, false,
                                        false, true, nullptr
                                    );
                                }
                            }
                        }
                        else {
                            ActorRefInHigh->AttackAlarm(pPlayer, false, true);
                            BaseProcess* v9 = ActorRefInHigh->pCurrentProcess;
                            v9->EnterCombat(
                                ActorRefInHigh, pPlayer, true, false, nullptr, false, false, false, 0, false, false,
                                true, nullptr
                            );
                        }
                    }
                }
            }
        }
        else {
            Actor* pActor = static_cast<Actor*>(apRefObject);
            pActor->Actor::AttackAlarm(pPlayer, false, true);
            pActor->GetCurrentAIProcess()->EnterCombat(
                pActor, pPlayer, true, false, nullptr, false, false, false, 0, false, false, true, nullptr
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetThreatRatioConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    Actor* pactor = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pactor
        )) {
        return false;
    }
    if ((pactor) && (pactor->IsNpc()) && (apRefObject) && (apRefObject->IsNpc())) {
        TESNPC* pnpc = static_cast<TESNPC*>(apRefObject->GetObjectReference());
        TESNPC* pnewnpc = static_cast<TESNPC*>(pactor->GetObjectReference());
        TESRace* prace = pnpc->pFormRace;
        TESRace* pnewrace = pnewnpc->pFormRace;
        if (pnewrace != prace) {
            uint32_t ilevel = 0;
            while (prace->GetYoungerRace()) {
                prace = prace->GetYoungerRace();
                ++ilevel;
            }
            while (pnewrace->GetYoungerRace()) {
                pnewrace = pnewrace->GetYoungerRace();
            }
            while (pnewrace->GetOlderRace() && ilevel) {
                pnewrace = pnewrace->GetOlderRace();
                --ilevel;
            }
            BSFaceGenManager::ClearBodyTexturesFromPalette(pnpc);
            pnpc->SwitchRace(pnewrace, nullptr);
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
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    pPlayer->SetPCYoung(a8 != 0);
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
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    pPlayer->SetPCToddler(a8 != 0);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_GetConcussed(apRefObject, a8, nullptr, afReturnValue);
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
    int32_t ival;
    ival = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &ival
        )) {
        return false;
    }
    if (apRefObject->GetDestructible()) {
        bool bwanttotarget = ival != 0;
        bool btargetable = false;
        BGSDestructibleObjectForm* pdestruct =
            BGSDestructibleObjectForm::GetDestructionForm(apRefObject->GetObjectReference());
        if (pdestruct) {
            btargetable = pdestruct->GetTargetableInVATS();
        }
        if (btargetable == bwanttotarget) {
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
    BGSEncounterZone* pzone;
    int32_t v10;
    pzone = nullptr;
    v10 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pzone, &v10
        )) {
        return false;
    }
    if (pzone) {
        pzone->SetNeverReset(v10 == 0);
        pzone->AddChange(2);
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
        TESObjectREFR* pref = nullptr;
        int32_t isound = 0;
        int32_t ievent = 3;
        if (!Script::ParseParameters(
                apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pref,
                &isound, &ievent
            )) {
            return false;
        }
        if ((pref) && (pref->IsActor())) {
            Actor* pactor = static_cast<Actor*>(pref);
            if (pactor->GetCurrentAIProcess()) {
                BaseProcess* pProc = pactor->GetCurrentAIProcess();
                pProc->SetActorsDetectionEvent(
                    pactor, apRefObject->GetLocationOnReference(), isound, ievent, apRefObject
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
    int32_t iignorecrime = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iignorecrime
        )) {
        return false;
    }
    if ((apRefObject) && (apRefObject->IsActor())) {
        Actor* pactor = static_cast<Actor*>(apRefObject);
        if (iignorecrime <= 0) {
            pactor->SetIgnoreCrime(false);
        }
        else {
            pactor->SetIgnoreCrime(true);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::IsCombatTargetConditionFunction(apRefObject, a8, nullptr, afReturnValue);
}

// Places an ash pile (the second one for a parsed 2, else the first) at the actor: a pick straight down from 32 units
// above it to 256 below puts the pile on the hit point, turned to the hit's normal, or on stairs to the navmesh's
// normal there; the pile and the actor then point at each other through their ash pile extra data. On stairs the normal
// comes from Pathing::GetNavmeshNormalForLocation, which returns early without writing it when the location has no
// navmesh triangle, and it is still passed by value to BSUtilities::GetRotationZToVectorTranspose, an uninitialised
// read kept deliberately as the game does.
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
    hkLocalBuffer<char[512]> v23;
    CFilter fltr;
    NiPoint3 v38;
    int32_t iparam = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iparam
        )) {
        return false;
    }
    if ((apRefObject) && (apRefObject->IsActor())) {
        TESObjectACTI* v16;
        NiPoint3 v31;
        alignas(16) NiPoint3 v32;
        NiNode* v39 = apRefObject->Get3D();
        v38 = apRefObject->GetLocationOnReference();
        NiPoint3 v37(0.0f, 0.0f, (&apRefObject->GetAngleOnReference())->z);
        char v36 = 0;
        // Static, water, terrain, ground and invisible wall.
        uint32_t uimultiLayer = 0x8022802;
        bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1, uimultiLayer);
        bhkPickData v34;
        fltr.iFilter = 0;
        fltr.SetLayer(L_CUSTOMPICK1);
        uint32_t NewSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
        fltr.SetGroup(NewSystemGroup);
        v34.SetFilter(fltr);
        v32.x = v38.x;
        v32.y = v38.y;
        v32.z = v38.z + 32.0;
        v31.x = v38.x;
        v31.y = v38.y;
        v31.z = v38.z - 256.0;
        v34.NiSetFrom(v32);
        v34.NiSetTo(v31);
        TES::pInstance->Pick(v34);
        hkBool v30 = v34.hasHit();
        if (v30.operator bool()) {
            NiPoint3 v19;
            const hkpShape* Shape = v34.m_rootCollidable->m_shape;
            bhkShape* v27 = bhkShape::Getbhk(Shape);
            bool MaterialVarType = v27->GetMaterialVarType(MVT_STAIRS);
            if (!MaterialVarType) {
                const hkpShape* v26 = Shape;
                const hkpShape* v25 = nullptr;
                uint32_t v24 = -1;
                v23.~hkLocalBuffer<char[512]>();
                ::new (&v23) hkLocalBuffer<char[512]>(8, nullptr);
                for (int32_t i = 0; i < 8; ++i) {
                    if (v34.m_shapeKeys[i] == -1) {
                        break;
                    }
                    if (v26->getType() == HK_SHAPE_MOPP && (++i >= 8 || v34.m_shapeKeys[i] == -1)) {
                        break;
                    }
                    v24 = v34.m_shapeKeys[i];
                    v25 = v26;
                    const hkpShapeContainer* v21 = v26->getContainer();
                    char (*v12)[512] = &v23.operator[](i);
                    v26 = v21->getChildShape(v34.m_shapeKeys[i], *v12);
                }
                bhkShape* v20 = bhkShape::Getbhk(v25);
                if (v20) {
                    MaterialVarType = v20->GetSubMaterialVarType(v24, MVT_STAIRS);
                }
            }
            if (MaterialVarType) {
                PathingLocation v18(apRefObject);
                Pathing::GetNavmeshNormalForLocation(v18, v19);
            }
            else {
                HK2NI(v19, v34.m_normal);
            }
            alignas(16) NiMatrix3 v17 = BSUtilities::GetRotationZToVectorTranspose(v19);
            v17.ToEulerAnglesXYZ(v37.x, v37.y, v37.z);
            v36 = 1;
            v34.NiCalcHitPoint(v38);
        }
        if (iparam == 2) {
            v16 = pAshPile2;
        }
        else {
            v16 = pAshPile1;
        }
        TESWorldSpace* WorldSpace = apRefObject->GetWorldSpace();
        TESObjectCELL* ParentCell = apRefObject->GetParentCell();
        TESObjectREFR* v15 =
            pDataHandler->CreateReferenceAtLocation(v16, v38, v37, ParentCell, WorldSpace, nullptr, nullptr, nullptr);
        v15->SetDroppedRef(v36 == 0);
        v15->SetDisableFade(true);
        v15->SetRefPersists(true);
        (&v15->m_Extra)->SetAshPileRef(apRefObject);
        (&apRefObject->m_Extra)->SetAshPileRef(v15);
        v15->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
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
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject &&
        (apRefObject->GetObjectReference() && apRefObject->GetObjectReference()->GetFormType() == DOOR_ID)) {
        static_cast<TESObjectDOOR*>(apRefObject->GetObjectReference())->TESObjectDOOR::SetMinimalUse(a8 != 0);
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
    uint32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    bool v10 = a8 != 0;
    pPlayer->SetPCCanUsePowerArmor(a8 != 0);
    return true;
}

bool Script::GetReputationThreshold(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    uint32_t v9;
    TESReputation* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v9
        )) {
        return false;
    }
    if (v9 > 2) {
        return false;
    }
    afReturnValue = static_cast<double>(a8->GetReputationThreshold(v9));
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
    TESObjectREFR* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    pPlayer->pLastExtDoorActivated = a8;
    if ((a8) && (a8->GetWorldSpace())) {
        TESWorldSpace* WorldSpace = a8->GetWorldSpace();
        Interface::SetRootMapSpace(WorldSpace);
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
    int32_t a8;
    a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    BSTaskThread::SetPauseSleepValue(a8);
    Interface::PrintLine("BSTaskThread::sPauseSleepValue set to %d ms", a8);
    return true;
}

bool Script::IsImageSpaceActive(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    TESImageSpaceModifier* pImageSpace;
    float fStrength;
    pImageSpace = nullptr;
    fStrength = 1.0f;
    afReturnValue = 0.0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pImageSpace,
            &fStrength
        )) {
        return false;
    }
    // The list, its items and its links are read through trivial accessors.
    BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* pNode = &TES::pInstance->listActiveImageSpaceModifiers;
    while (pNode) {
        NiPointer<ImageSpaceModifierInstance> spInstance(pNode->m_item);
        if (spInstance.m_pObject) {
            ImageSpaceModifierInstanceForm* pForm = spInstance.m_pObject->IsForm();
            if ((pForm) && (pForm->pImageSpaceMod == pImageSpace)) {
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

bool Script::ResetXP_Execute(
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
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals
    );
    if (pPlayer) {
        float v10 = pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS);
        pPlayer->PermanentModActorValue(ActorValue::EXPERIENCE_POINTS, (-v10), static_cast<Actor*>(nullptr));
        return true;
    }

    Warning("Attempting to reset xp on an invalid player!");
    return false;
}

bool Script::PlaceAtReticle(
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
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pObject,
            &iCount, &fUnused, &fDistance
        )) {
        return false;
    }
    fDistance = BSMIN(fDistance, 50000.0f);
    bhkPickData kPick;
    NiPoint3 kTo(0.0f, fDistance, 0.0f);
    NiPoint3 kFrom = pPlayer->GetLocationOnReference();
    NiAVObject* pCamera = (NiAVObject*)Main::GetWorldSceneGraph()->GetCamera();
    alignas(16) NiMatrix3 kCameraRotate = pCamera->GetWorldTransform().m_Rotate;
    // The camera's rows with their components rotated (y, z, x into x, y, z): NiMatrix3's and NiPoint3's empty
    // constructors are trivial.
    NiMatrix3 kRotate;
    alignas(16) NiPoint3 kRow;
    kCameraRotate.GetRow_005DF560(0, &kRow.y, &kRow.z, &kRow.x);
    kRotate.SetRow_005DF510(0, &kRow);
    kCameraRotate.GetRow_005DF560(1, &kRow.y, &kRow.z, &kRow.x);
    kRotate.SetRow_005DF510(1, &kRow);
    kCameraRotate.GetRow_005DF560(2, &kRow.y, &kRow.z, &kRow.x);
    kRotate.SetRow_005DF510(2, &kRow);
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
                pBoundObject, kLocation, *pAngle, pParentCell, pWorldSpace, nullptr, nullptr, nullptr
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
    int32_t iPassTimeOnly;
    iPassTimeOnly = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &iSleep,
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
    char (*ActorPackageThatIsRunning)[260] = &SettingT<INISettingCollection>::GetCollection()->pSettingFile;
    INISettingCollection* Collection = SettingT<INISettingCollection>::GetCollection();
    Collection->SaveAll(reinterpret_cast<char*>(ActorPackageThatIsRunning));
    char (*v12)[260] = &SettingT<INIPrefSettingCollection>::GetCollection()->pSettingFile;
    INIPrefSettingCollection* v9 = SettingT<INIPrefSettingCollection>::GetCollection();
    v9->SaveAll(reinterpret_cast<char*>(v12));
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
    Sky* Instance = Sky::GetInstance();
    TESWeather* Thread = Instance->GetCurrentWeather();
    TESFile* File = Thread->GetFile(-1);
    if (File) {
        TESFile* ThreadSafeFile = File->GetThreadSafeFile();
        ThreadSafeFile->OpenTES(NiFile::READ_ONLY, false);
        ThreadSafeFile->FindForm(Thread);
        Thread->ClearData();
        Thread->Load(ThreadSafeFile);
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
    NiNode* pnode;
    NiMatrix3 rot;
    NiNode* apMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
    if (apMemory) {
        pnode = ::new (apMemory) NiNode(0);
    }
    else {
        pnode = nullptr;
    }
    TESObjectCELL* pcell = pPlayer->GetParentCell();
    if (pcell->GetInterior()) {
        SeenData* pdata = pcell->GetSeenData();
        if (pdata) {
            const NiPoint3* v8 = &pPlayer->GetLocationOnReference();
            NiPoint3 coord(static_cast<float>(0.0), static_cast<float>(0.0), v8->z);
            pdata->DrawSeenData(pnode, coord, true);
            float frot = pcell->GetNorthRotation() * -1.0;
            if (frot != 0.0) {
                rot = NiMatrix3_IDENTITY;
                rot.MakeZRotation(frot);
                pnode->SetRotate(rot);
            }
        }
    }
    else {
        for (int32_t x_2 = 0;; ++x_2) {
            // The grid count is compared unsigned.
            if (static_cast<uint32_t>(x_2) >= uGridsToLoad.UInt()) {
                break;
            }
            for (int32_t y_2 = 0;; ++y_2) {
                SeenData* pdata_2;
                if (static_cast<uint32_t>(y_2) >= uGridsToLoad.UInt()) {
                    break;
                }
                TESObjectCELL* pcell_2 = TES::pInstance->GetGridCell(x_2, y_2)->pCell;
                if (pcell_2) {
                    pdata_2 = pcell_2->GetSeenData();
                }
                else {
                    pdata_2 = nullptr;
                }
                SeenData* pdata_3 = pdata_2;
                if (pdata_2) {
                    float a2 = pPlayer->GetLocationOnReference().z;
                    float y = static_cast<float>(pcell_2->GetDataY() << 12);
                    float x = static_cast<float>(pcell_2->GetDataX() << 12);
                    NiPoint3 coord_2(x, y, a2);
                    pdata_3->DrawSeenData(pnode, coord_2, true);
                }
            }
        }
    }
    NiUpdateData UpdateData(0.0f, false, false);
    pnode->Update(UpdateData);
    pnode->UpdateProperties();
    TES::pInstance->AddTempDebugObject(pnode, 30.0f);
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
    const char* v9;
    ::ToggleFullHelp();
    if (GetIsFullHelp()) {
        v9 = "shown.";
    }
    else {
        v9 = "hidden.";
    }
    Interface::PrintLine("Toggle Full Help %s", v9);
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
        else {
            if (TES::pInstance->pInteriorCell) {
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
    }
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v9;
        if (s_ToggleSky) {
            v9 = "Off";
        }
        else {
            v9 = "On";
        }
        Interface::PrintLine("Sky -> %s", v9);
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
        bool v11 = !apRefObject->GetNoCollision();
        apRefObject->SetNoCollision(v11);
        if (GetThreadTLSData()->bSystemOutput) {
            const char* v10;
            if (v11) {
                v10 = "Off";
            }
            else {
                v10 = "On";
            }
            Interface::PrintLine("Ref '%s' Collision -> %s", apRefObject->GetFullName(), v10);
        }
    }
    else {
        TES::pInstance->ToggleCollision();
        if (GetThreadTLSData()->bSystemOutput) {
            const char* v9;
            if (TES::QCollisionDisableFlag()) {
                v9 = "Off";
            }
            else {
                v9 = "On";
            }
            Interface::PrintLine("Collision -> %s", v9);
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
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v9;
        if (g_bIsToggleWireframe) {
            v9 = "On";
        }
        else {
            v9 = "Off";
        }
        Interface::PrintLine("Wireframe -> %s", v9);
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
        Actor* pactor = dynamic_cast<Actor*>(apRefObject);
        if ((pactor) && (pactor->pRagdollController)) {
            bool v9 = pactor->pRagdollController->QFootIKEnable();
            pactor->pRagdollController->SetFootIKEnable(!v9);
            if (GetThreadTLSData()->bSystemOutput) {
                const char* v11;
                if (pactor->pRagdollController->QFootIKEnable()) {
                    v11 = "On";
                }
                else {
                    v11 = "Off";
                }
                Interface::PrintLine("FootIK on REF %s %s", apRefObject->GetFormEditorID(), v11);
            }
        }
    }
    else {
        bool v8 = bhkRagdollController::QFootIK();
        bhkRagdollController::SetFootIK(!v8);
        if (GetThreadTLSData()->bSystemOutput) {
            const char* v12;
            if (bhkRagdollController::QFootIK()) {
                v12 = "On";
            }
            else {
                v12 = "Off";
            }
            Interface::PrintLine("FootIK %s", v12);
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
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v9;
        if (Interface::IsDebugTextVisible()) {
            v9 = "On";
        }
        else {
            v9 = "Off";
        }
        Interface::PrintLine("Debug Text -> %s", v9);
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
        const char* FullName = apRefObject->GetFullName();
        if (apRefObject == pPlayer) {
            const char* a3;
            NiTreeCtrl* pWindow;
            NiTreeCtrl* apMemory = static_cast<NiTreeCtrl*>(ourMalloc(sizeof(NiTreeCtrl)));
            if (apMemory) {
                const char* windowName;
                if (pPlayer->Is3rdPerson()) {
                    windowName = "Player 3rd Person";
                }
                else {
                    windowName = "Player 1st Person";
                }
                NiNode* v20 = apRefObject->Get3D();
                HWND__* FormFlags = appMain->hWnd;
                pWindow = ::new (apMemory)
                    NiTreeCtrl(appMain->hInstance, FormFlags, v20, windowName, 0x80000000u, 0x80000000u, 800, 600);
            }
            else {
                pWindow = nullptr;
            }
            if (pPlayer->Is3rdPerson()) {
                a3 = "Player 1st Person";
            }
            else {
                a3 = "Player 3rd Person";
            }
            bool v8 = pPlayer->Is3rdPerson();
            NiAVObject* v9 = pPlayer->Get3D(v8);
            pWindow->CreateTree(v9, a3);
            if (pPlayer->GetInventory3D()) {
                NiNode* Inventory3D = pPlayer->GetInventory3D();
                pWindow->CreateTree(Inventory3D, "Player Inventory Menu");
            }
        }
        else {
            NiTreeCtrl* v28 = static_cast<NiTreeCtrl*>(ourMalloc(sizeof(NiTreeCtrl)));
            if (v28) {
                NiNode* v21 = apRefObject->Get3D();
                HWND__* v18 = appMain->hWnd;
                ::new (v28) NiTreeCtrl(appMain->hInstance, v18, v21, FullName, 0x80000000u, 0x80000000u, 800, 600);
            }
        }
    }
    else {
        NiTreeCtrl* v23;
        NiTreeCtrl* v27 = static_cast<NiTreeCtrl*>(ourMalloc(sizeof(NiTreeCtrl)));
        if (v27) {
            SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
            HWND__* v19 = appMain->hWnd;
            v23 = ::new (v27) NiTreeCtrl(
                appMain->hInstance, v19, WorldSceneGraph, "Fallout World Scene Graph", 0x80000000u, 0x80000000u, 800,
                600
            );
        }
        else {
            v23 = nullptr;
        }
        SceneGraph* v11 = Main::MenuRootNode();
        v23->CreateTree(v11, "Fallout Interface Scene Graph");
        SceneGraph* v12 = Main::MenuRootNode3D();
        v23->CreateTree(v12, "Fallout Interface 3D Object Scene Graph");
        if (BSShaderManager::GetShadowSceneNode(1)) {
            ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
            v23->CreateTree(ShadowSceneNode_, "Fallout Menu Object 3D Scene Graph");
        }
        if (BSShaderManager::GetShadowSceneNode(3)) {
            ShadowSceneNode* v14 = BSShaderManager::GetShadowSceneNode(3);
            v23->CreateTree(v14, "Fallout Menu Player 3D Scene Graph");
        }
        if (ScreenSplatter::IsActiveSplatter()) {
            NiNode* Root = ScreenSplatter::GetRoot();
            v23->CreateTree(Root, "Fallout Screen Splatter Scene Graph");
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
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v9;
        if (ReferenceProcessLists.bRunDetection) {
            v9 = "On";
        }
        else {
            v9 = "Off";
        }
        Interface::PrintLine("AI Detection is  %s", v9);
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
        if (GetThreadTLSData()->bSystemOutput) {
            const char* v9;
            if (pActor->bProcessMe) {
                v9 = "On";
            }
            else {
                v9 = "Off";
            }
            Interface::PrintLine("%s processing is  %s", pActor->GetFullName(), v9);
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
        if (pActor->bProcessMe) {
            afReturnValue = 0.0;
        }
        else {
            afReturnValue = 1.0;
        }
        if (GetThreadTLSData()->bSystemOutput) {
            const char* v9;
            if (pActor->bProcessMe) {
                v9 = "On";
            }
            else {
                v9 = "Off";
            }
            Interface::PrintLine("%s processing is  %s", pActor->GetFullName(), v9);
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
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v9;
        if (ReferenceProcessLists.bProcessSche) {
            v9 = "On";
        }
        else {
            v9 = "Off";
        }
        Interface::PrintLine("AI Processing of Actor's Editor Schedules is  %s", v9);
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
        DebugTextManager* Singleton = DebugTextManager::GetSingleton();
        Singleton->SetScreenDebugPage("Pathing", 0);
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
    NiPoint3 v13;
    NiMatrix3 v14;
    NiPoint3 v15;
    NiMatrix3 v16;
    NiPoint3 v17;
    NiMatrix3 result;
    NiPoint3 v19;
    NiMatrix3 v21;
    if (apRefObject) {
        NiNode* v23 = apRefObject->Get3D();
        if (v23) {
            NiMatrix3 ecx0;
            v21 = NiMatrix3_IDENTITY;
            apRefObject->SetStartingPosition(NiPoint3_ZERO);
            NiPoint3* v8 = &(v19 = apRefObject->GetStartingAngle());
            ecx0.MakeXRotation(v8->x);
            v21 = (result = ecx0 * v21);
            NiPoint3* v9 = &(v17 = apRefObject->GetStartingAngle());
            ecx0.MakeYRotation(v9->y);
            v21 = (v16 = ecx0 * v21);
            NiPoint3* v10 = &(v15 = apRefObject->GetStartingAngle());
            ecx0.MakeZRotation(v10->z);
            v21 = (v14 = ecx0 * v21);
            NiPoint3* v11 = &(v13 = apRefObject->GetStartingLocation());
            v23->SetTranslate(*v11);
            v23->SetRotate(v21);
            // apRefObject is tested again here, as the game does.
            if (!apRefObject || !apRefObject->GetAnimation()) {
                NiUpdateData arData(0.0f, false, false);
                v23->Update(arData);
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
        NiAVObject* pobj = apRefObject->Get3D();
        NiNode* obj3D = nullptr;
        if (pobj) {
            obj3D = pobj->IsNode();
        }
        if (obj3D && obj3D->GetControllers()) {
            NiTimeController* po = obj3D->GetControllers();
            NiControllerManager* pmanager = nullptr;
            while (po) {
                if (NiObject::IsKindOf_0(&NiControllerManager::ms_RTTI, po)) {
                    pmanager = static_cast<NiControllerManager*>(po);
                    break;
                }
                po = po->GetNext();
            }
            if (pmanager) {
                NiFixedString* v8 = &FixedStrings::QSpecialIdle_AreaEffect();
                NiControllerSequence* pseq = pmanager->GetSequenceByName(*v8);
                if (pseq) {
                    for (uint32_t n = 0; n < pmanager->GetSequenceCount(); ++n) {
                        NiControllerSequence* pdisableSeq = pmanager->GetSequenceAt(n);
                        const char* fEaseOutTime = animGroupInfo[0].pName;
                        const char* HandleString = (&pdisableSeq->m_kName)->GetHandleString();
                        if (BSstricmp(HandleString, fEaseOutTime)) {
                            pmanager->DeactivateSequence(pdisableSeq, 0.0f);
                        }
                    }
                    pmanager->ActivateSequence(pseq, 0, false, 1.0f, 0.0f, nullptr);
                    pseq->SetOffset(-3.4028235e+38f);
                    pmanager->SetActive(true);
                }
                else {
                    int32_t fEaseOutTimeb = apRefObject->iFormID;
                    Warning(
                        "ANIMATION: (PlayExplosion Error) Sequence '%s' not found for '%s' (%08X).",
                        "SpecialIdle_AreaEffect", apRefObject->GetFullName(), fEaseOutTimeb
                    );
                }
            }
            else {
                int32_t fEaseOutTimec = apRefObject->iFormID;
                Warning(
                    "ANIMATION: (PlayExplosion Error) No NiControllerManager found for '%s' (%08X).",
                    apRefObject->GetFullName(), fEaseOutTimec
                );
            }
        }
        else {
            int32_t fEaseOutTimed = apRefObject->iFormID;
            Warning(
                "ANIMATION: (PlayExplosion Error) No 3d or controllers found for '%s' (%08X).",
                apRefObject->GetFullName(), fEaseOutTimed
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
    if ((apScriptLocals) && (apScriptLocals->m_pScriptEffectData) &&
        (apScriptLocals->m_pScriptEffectData->m_bScriptEffectFinish)) {
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
                Script::SetActionFlag(
                    (&pActor->m_Extra)->GetPackageExtra(), &pActor->m_Extra, SCRIPT_ACTION_ONPACKAGEEND
                );
                (&pActor->m_Extra)->RemovePackageExtra();
            }
            else {
                Script::SetActionFlag(pPackage, &pActor->m_Extra, SCRIPT_ACTION_ONPACKAGEEND);
                BaseProcess* CurrentAIProcess = pActor->pCurrentProcess;
                CurrentAIProcess->ClearCurrentPackage();
                BaseProcess* v11 = pActor->pCurrentProcess;
                v11->CheckforNewPackage(pActor, true);
            }
        }
        if (pActor == pPlayer) {
            pPlayer->SetAIControlledPackage(false);
            Animation* Animation_0 = pPlayer->GetAnimation(true);
            Animation_0->SpecialIdleFree(true, false);
            Animation* v9 = pPlayer->GetAnimation(false);
            v9->SpecialIdleFree(true, false);
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
    uint32_t apID;
    afReturnValue = 0.0;
    apID = 0;
    if ((apRefObject) && (apRefObject->GetActionRef())) {
        TESObjectREFR* ActionRef = apRefObject->GetActionRef();
        apID = ActionRef->GetFormID();
        Script::PutNumericIDInDouble(apID, afReturnValue);
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetActionRef >> (%08x)", apID);
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
    Actor* pactor = dynamic_cast<Actor*>(apRefObject);
    if ((pactor) && (pactor->GetCharController())) {
        bhkCharacterController* CharController = pactor->GetCharController();
        CharController->QFallStartHeight();
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
    bool bretval = Cmd_SetArgTo0AndReturnTrue(apRefObject, nullptr, nullptr, afReturnValue);
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Actor players last ridden horse >> %0.2f", afReturnValue);
    }
    return bretval;
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
    ShadowSceneLight* VATSLight = pVATS.GetVATSLight();
    NiLight* Light = VATSLight->GetLight();
    if (Light->GetAppCulled()) {
        Interface::PrintLine("VATS Lighting On");
        ShadowSceneLight* v10 = pVATS.GetVATSLight();
        NiLight* v11 = v10->GetLight();
        v11->SetAppCulled(false);
        ShadowSceneLight* v16 = pVATS.GetVATSLight();
        ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
        ShadowSceneNode_->UpdateLight(v16);
    }
    else {
        Interface::PrintLine("VATS Lighting Off");
        ShadowSceneLight* v14 = pVATS.GetVATSLight();
        NiLight* v15 = v14->GetLight();
        v15->SetAppCulled(true);
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
    MobDialogueFlagResetter mobDialogueFlagResetter;
    mobDialogueFlagResetter.~MobDialogueFlagResetter();
    ::new (&mobDialogueFlagResetter) MobDialogueFlagResetter;
    ReferenceProcessLists.IterateMobs(mobDialogueFlagResetter);
    pPlayer->ResetPlayerGreetFlag();
    TESTopic* pInfoRefusalTopic = TESTopic::GetTopic(DT_MISCELLANEOUS, 12);
    TESTopic* pHelloTopic = TESTopic::GetTopic(DT_CONVERSATION, 0);
    bool bfoundInfoRefusal = false;
    for (TESTopic* ptopic : *pDataHandler->GetTopicList()) {
        if (ptopic) {
            if (ptopic == pInfoRefusalTopic) {
                bfoundInfoRefusal = true;
            }
            for (QUEST_INFO* pQuestInfo : *&ptopic->m_listQuestInfo) {
                if (pQuestInfo) {
                    TopicInfoArray* pInfoArray = (&pQuestInfo->infoArray);
                    uint32_t maxSize = pInfoArray->m_uiSize;
                    for (uint32_t i = 0; i < maxSize; ++i) {
                        TESTopicInfo* pti = pInfoArray->GetAt(i);
                        if (pti) {
                            pti->ResetSaidOnceFlags();
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
    const char* v9;
    BSShaderManager::bUseDepthBias = !BSShaderManager::bUseDepthBias;
    if (BSShaderManager::bUseDepthBias) {
        v9 = "ON";
    }
    else {
        v9 = "OFF";
    }
    Interface::PrintLine("Depth Bias is now %s!", v9);
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
    for (uint32_t i = 0; i < (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* Item = (&ReferenceProcessLists.MobProcessArray)->GetItem(i);
        Projectile* pProjectile = nullptr;
        if (Item && Item->IsProjectile()) {
            pProjectile = static_cast<Projectile*>(Item);
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
    void* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject && a8) {
        return Script::GetHeadingAngleConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    if (!Cmd_GetDefaultOpen(apRefObject, nullptr, nullptr, afReturnValue)) {
        return false;
    }
    if (GetThreadTLSData()->bSystemOutput) {
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
    void* a8;
    TESForm* a3;
    afReturnValue = 0.0;
    a8 = nullptr;
    a3 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &a3
        )) {
        return false;
    }
    if (!a3) {
        a3 = pPlayer->GetObjectReference();
    }
    Cmd_IsCellOwner(apRefObject, a8, a3, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetKillerConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetKillerObjectConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    TESForm* apParam2;
    a8 = nullptr;
    apParam2 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8,
               &apParam2
           ) &&
        Script::GetFactionCombatReactionConditionFunction(apRefObject, a8, apParam2, afReturnValue);
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
    Actor* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject && a8) {
        return Cmd_GetFactionRelation(apRefObject, a8, nullptr, afReturnValue);
    }
    return true;
}

bool Script::IsLastIdlePlayedFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject && a8) {
        return Cmd_IsLastIdlePlayed(apRefObject, a8, nullptr, afReturnValue);
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
    TESActorBaseData::ALIGNMENT a8;
    a8 = TESActorBaseData::ALIGNMENT_NEUTRAL;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Cmd_GetIsAlignment(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetVATSRightAreaFreeConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    void* a8;
    a8 = nullptr;
    return Script::ParseParameters(
               apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
           ) &&
        Script::GetVATSLeftAreaFreeConditionFunction(apRefObject, a8, nullptr, afReturnValue);
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
    int32_t RepType;
    TESReputation* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &RepType
        )) {
        return false;
    }
    if (RepType < 0 || RepType > 1) {
        return false;
    }
    afReturnValue = a8->GetReputationValue(static_cast<ReputationType>(RepType));
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
    int32_t RepType;
    TESReputation* a8;
    a8 = nullptr;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &RepType
        )) {
        return false;
    }
    if (RepType < 0 || RepType > 1) {
        return false;
    }
    afReturnValue = a8->GetReputationPct(static_cast<ReputationType>(RepType));
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
    TESChallenge* a8;
    a8 = nullptr;
    Script::ParseParameters(
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
    );
    if (!a8) {
        return false;
    }
    afReturnValue = static_cast<double>(a8->GetUnlocked());
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
    TESChallenge* a8;
    a8 = nullptr;
    Script::ParseParameters(
        apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
    );
    if (!a8) {
        return false;
    }
    int32_t v9 = a8->GetCompleted() || a8->GetCompletedRecurring();
    afReturnValue = static_cast<double>(v9);
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
        return (&m_TargetPath.Doors.GetAt(0))->pDoor;
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

const NiPixelFormat* NiTexture::RendererData::GetPixelFormat() {
    return &m_kPixelFormat;
}

void NiTexturingProperty::Map::SetTexture(NiTexture* pkTexture) {
    m_spTexture = pkTexture;
}

// Allocate the base map only when its slot is empty, then call the texture setter.
void NiTexturingProperty::SetBaseTexture(NiTexture* pkTexture) {
    Map* pkMap = m_kMaps[BASE_INDEX];
    if (!pkMap) {
        void* pvMemory = ourMalloc(sizeof(Map));
        pkMap = pvMemory ? ::new (pvMemory) Map : nullptr;
        m_kMaps.SetAt(BASE_INDEX, pkMap);
    }
    pkMap->SetTexture(pkTexture);
}

// Returns the bytes from m_next to m_end, reading m_end first.
int32_t hkStackMemory::getFreeStackSize() {
    void* pEnd = m_end;
    char* pNext = m_next;
    return hkGetByteOffset(pNext, pEnd);
}

// Checks the free size (the assert's call, the result unused), then moves m_next past the block.
void* hkStackMemory::allocateStack(int32_t numBytes, const char* what) {
    getFreeStackSize();
    char* pCurrent = m_next;
    m_next = hkAddByteOffset(pCurrent, numBytes);
    return pCurrent;
}

void hkStackMemory::deallocateStack(void* p) {
    m_next.operator=(static_cast<char*>(p));
}

void hkStackMemory::initMemory(void* p, int32_t size) {
    m_start.m_storage = p;
    m_next.operator=(static_cast<char*>(p));
    m_end.m_storage = hkAddByteOffset<char>(static_cast<char*>(p), size);
}
