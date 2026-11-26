#include "Fallout/Misc/Main.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSCore/BSPrecisionTimer.h"
#include "BSCore/BSSemaphore.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/MemoryPool.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSCore/ScrapHeap.h"
#include "BSCore/Xenon/BSThread_Xenon.h"
#include "BSDevices/Controls.h"
#include "BSDevices/XPrefSettings.h"
#include "BSHavok/bhkAvoidBox.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSBound.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSMain.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSMultiBoundSphere.h"
#include "BSMain/BSParticleSystemManager.h"
#include "BSMain/BSPortal.h"
#include "BSMain/BSPortalGraph.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/IOManager.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSMain/RendererSettingCollection.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSMain/XRegSetting.h"
#include "BSMenu/DebugText.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSClearZNode.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSFadeNodeCuller.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderBloodSplatter.h"
#include "BSShader/BSShaderBloodSplatterProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderSDM.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/DistantLODShaderProperty.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectBlur.h"
#include "BSShader/ImageSpaceEffectDepthOfField.h"
#include "BSShader/ImageSpaceEffectHDR.h"
#include "BSShader/ImageSpaceEffectMotionBlur.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/MTRenderingSystem.h"
#include "BSShader/ParticleShader.h"
#include "BSShader/ParticleShaderProperty.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SkyShader.h"
#include "BSShader/WaterShader.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFileCache.h"
#include "BSSystem/BSPackedTask.h"
#include "BSSystem/BSPackedTaskQueue.h"
#include "BSSystem/BSSearchPath.h"
#include "BSSystem/BSTMessageQueue.h"
#include "BSSystem/BSTaskletManager.h"
#include "BSSystem/FileFinder.h"
#include "BSSystem/Win32/BSWin32TaskletManager.h"
#include "BSSystemUtilities/BSMsgDialogSystemUtility.h"
#include "BSSystemUtilities/BSSystemUtility.h"
#include "BSSystemUtilities/SteamAPICallbacks.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CheckWithinMultiBoundTask.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/Havok/HitCaster.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/Sky/Sun.h"
#include "Fallout Shared/SleepManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESChallengeManager.h"
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
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TempEffects/BSTempEffectSimpleDecal.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/AITaskManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/HighActorCuller.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/FOVATSEffectManager.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/CharGenMenu.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/LoveTesterMenu.h"
#include "Fallout/Interface/Menus/SPECIALBookMenu.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/BGSMoviePlayer.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/MemoryLevelManager.h"
#include "Fallout/Misc/MemoryManagerFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Fallout/Misc/ScreenShot.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParallelUpdateTaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElementsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVisibleArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiStaticDataManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/Ini.h"
#include "platform/Paths.h"
#include "platform/System.h"
#include "xfnv/Compiler.h"

#include <SDL3/SDL.h>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

// Main::OnIdle_FixActorBones's static bfixNextTime.
static bool bfixNextTime;

// INI and game settings read by the OnIdle steps and Main_ShaderInit.
static SettingT<INISettingCollection> bUseHardDriveCache("bUseHardDriveCache:General", false);
static SettingT<INISettingCollection> bDisplayLODLandINI("bDisplayLODLand:LOD", true);
static SettingT<INISettingCollection> fLocalTreeMipMapLODBiasINI("fLocalTreeMipMapLODBias:SpeedTree", -0.25f);
static SettingT<INISettingCollection> fLODTreeMipMapLODBiasINI("fLODTreeMipMapLODBias:SpeedTree", -0.75f);

// The game settings this file defines, with their IDs and values, in initialization order.
NiPointer<BSSceneGraph> Main::spWorldRoot;
NiPointer<NiNode> Main::spMenuRoot;
NiPointer<NiNode> Main::sp3DMenuRoot;
NiPointer<NiNode> Main::spSkyRoot;
NiPointer<NiNode> Main::spWeatherRoot;
NiPointer<BSParticleSystemManager> Main::spParticleSystemManager;
NiPointer<BSFogProperty> Main::spFog;
NiPointer<BSRenderedTexture> Main::spMenuBGTexture;
NiPointer<BSRenderedTexture> Main::spInterfaceFXTex;
NiPointer<BSRenderedTexture> Main::spVATSTex;
NiPointer<NiScreenElements> Main::spMenuBGGeometry;
SettingT<INISettingCollection> bUseArchives("bUseArchives:Archive", true);
SettingT<INISettingCollection> bAlwaysActive("bAlwaysActive:General", false);
SettingT<INISettingCollection> bCRTMemoryChecks("bCRTMemoryChecks:General", false);
SettingT<INISettingCollection> sEssentialFileCacheList(
    "sEssentialFileCacheList:General",
    "Data\\FalloutNV.esm, Data\\Fallout - Misc.bsa, Data\\Fallout - Sound.bsa, Data\\final_master_xml.dat, "
    "Data\\Shaders\\shaderpackage.sdp, Data\\Music\\Battle\\*, Data\\Music\\Battle\\Finale\\*, Data\\Music\\Base\\*, "
    "Data\\Music\\Dungeon\\*, Data\\Music\\Endgame\\*, Data\\Music\\Explore\\*, Data\\Music\\Public\\*, "
    "Data\\Music\\Special\\*, Data\\Music\\Tension\\*, Data\\Music\\TranquilityLane\\*"
);
SettingT<INISettingCollection>
    sUnessentialFileCacheList("sUnessentialFileCacheList:General", "Data\\Fallout - Meshes.bsa");
SettingT<INISettingCollection>
    sMasterArchiveList("sArchiveList:Archive", "Fallout - Textures.bsa, Fallout - Meshes.bsa, Oblivion - Voices1.bsa");
SettingT<INISettingCollection> sTestFile1("sTestFile1:General", "FalloutNV.esm");
SettingT<INISettingCollection> sTestFile2("sTestFile2:General", "");
SettingT<INISettingCollection> sTestFile3("sTestFile3:General", "");
SettingT<INISettingCollection> sTestFile4("sTestFile4:General", "");
SettingT<INISettingCollection> sTestFile5("sTestFile5:General", "");
SettingT<INISettingCollection> sTestFile6("sTestFile6:General", "");
SettingT<INISettingCollection> sTestFile7("sTestFile7:General", "");
SettingT<INISettingCollection> sTestFile8("sTestFile8:General", "");
SettingT<INISettingCollection> sTestFile9("sTestFile9:General", "");
SettingT<INISettingCollection> sTestFile10("sTestFile10:General", "");
SettingT<INISettingCollection> sStartingCell("sStartingCell:General", "");

static SettingT<INISettingCollection> bUseMyGamesDirectory("bUseMyGamesDirectory:General", true);
SettingT<INISettingCollection> sStartingWorld("sStartingWorld:General", "");
SettingT<INISettingCollection> sStartingCellX("sStartingCellX:General", "");
SettingT<INISettingCollection> sStartingCellY("sStartingCellY:General", "");
SettingT<INISettingCollection> bRunVTuneTest("bRunVTuneTest:General", false);
SettingT<INISettingCollection> iFPSClamp("iFPSClamp:General", 0);
SettingT<INISettingCollection> bChangeTimeMultSlowly("bChangeTimeMultSlowly:General", true);
SettingT<INISettingCollection> bUseThreadedParticleSystem("bUseThreadedParticleSystem:General", false);
SettingT<INISettingCollection> bUseThreadedMorpher("bUseThreadedMorpher:General", false);
SettingT<INISettingCollection> bUseThreadedTempEffects("bUseThreadedTempEffects:General", false);
SettingT<INISettingCollection> bDisplayBoundingVolumes("bDisplayBoundingVolumes:General", false);
SettingT<INISettingCollection> bEnableBoundingVolumeOcclusion("bEnableBoundingVolumeOcclusion:General", false);
static SettingT<INISettingCollection> sCameraPathNif("sNif:CameraPath", "Test\\CameraPath.nif");
SettingT<INISettingCollection> iCameraPathFPS("iFPS:CameraPath", 60);
SettingT<INISettingCollection> sCameraPathDir("sDirectoryName:CameraPath", "TestCameraPath");
SettingT<INISettingCollection> iCameraPathTake("iTake:CameraPath", 0);
SettingT<INISettingCollection> bPreCullActors("bPreCullActors:General", true);
SettingT<INISettingCollection> fLODLandVerticalBias("fLODLandVerticalBias:LOD", 0.0f);
SettingT<INISettingCollection> fLODLandDropAmount("fLODLandDropAmount:LOD", 230.0f);
SettingT<INISettingCollection> fMenuBGBlurRadius("fMenuBGBlurRadius:Interface", 0.01f);
SettingT<INISettingCollection> fLODFadeOutObjectMultComplex("fLODFadeOutObjectMultComplex:LOD", 1.0f);
SettingT<INISettingCollection> fLODFadeOutItemMultComplex("fLODFadeOutItemMultComplex:LOD", 1.0f);
SettingT<INISettingCollection> fLODFadeOutActorMultComplex("fLODFadeOutActorMultComplex:LOD", 1.0f);
SettingT<INISettingCollection> fLODFadeOutObjectMultCity("fLODFadeOutObjectMultCity:LOD", 1.0f);
SettingT<INISettingCollection> fLODFadeOutItemMultCity("fLODFadeOutItemMultCity:LOD", 1.0f);
SettingT<INISettingCollection> fLODFadeOutActorMultCity("fLODFadeOutActorMultCity:LOD", 1.0f);
SettingT<INISettingCollection> fLODFadeOutObjectMultInterior("fLODFadeOutObjectMultInterior:LOD", 1.0f);
SettingT<INISettingCollection> fLODFadeOutItemMultInterior("fLODFadeOutItemMultInterior:LOD", 1.0f);
SettingT<INISettingCollection> fLODFadeOutActorMultInterior("fLODFadeOutActorMultInterior:LOD", 1.0f);

// The copy protection's message box texts.
static SettingT<INISettingCollection>
    sCopyProtectionTitle("sCopyProtectionTitle:CopyProtectionStrings", "CD-ROM Drive Not Found");
static SettingT<INISettingCollection> sCopyProtectionMessage(
    "sCopyProtectionMessage:CopyProtectionStrings",
    "Unable to find a CD-ROM/DVD drive on this computer."
);
static SettingT<INISettingCollection>
    sCopyProtectionTitle2("sCopyProtectionTitle2:CopyProtectionStrings", "Fallout Disc Not Found");
static SettingT<INISettingCollection>
    sCopyProtectionMessage2("sCopyProtectionMessage2:CopyProtectionStrings", "Insert the Fallout Disc.");
SettingT<INISettingCollection> bUseBackgroundFileLoader("bUseBackgroundFileLoader:BackgroundLoad", false);
NiPointer<NiRefObject> spcameraPath;
SettingT<INISettingCollection> bCheckRuntimeCollisions("bCheckRuntimeCollisions:Archive", false);
SettingT<INISettingCollection> iRetainDirectoryStringTable("iRetainDirectoryStringTable:Archive", 1);
SettingT<INISettingCollection> iRetainFilenameStringTable("iRetainFilenameStringTable:Archive", 1);
SettingT<INISettingCollection> iRetainFilenameOffsetTable("iRetainFilenameOffsetTable:Archive", 1);
SettingT<INISettingCollection> sArchiveInvalidationFile("sInvalidationFile:Archive", "ArchiveInvalidation.txt");
SettingT<INISettingCollection> bInvalidateOlderFiles("bInvalidateOlderFiles:Archive", true);
SettingT<INISettingCollection> iFileLogging("iFileLogging:MESSAGES", 0);
SettingT<INISettingCollection> sDebugText("sDebugText:Display", "VATS");
SettingT<INISettingCollection> bShowMenuTextureUse("bShowMenuTextureUse:Display", true);
SettingT<INISettingCollection> iDebugTextTopBottomOffset("iDebugTextTopBottomOffset:Display", 10);
SettingT<INISettingCollection> iDebugTextLeftRightOffset("iDebugTextLeftRightOffset:Display", 10);
SettingT<INISettingCollection> bUseSunbeams("bUseSunbeams:Display", false);
SettingT<INISettingCollection> fScopeScissorAmount("fScopeScissorAmount:Display", 0.3f);
SettingT<RegSettingCollection> sBasePathS("sBasePath", ".\\");
SettingT<INISettingCollection> bBlockMessageBoxes("bBlockMessageBoxes:MESSAGES", false);
SettingT<INISettingCollection> bUseWindowsMessageBox("bUseWindowsMessageBox:MESSAGES", false);
static BSPrecisionTimer UpdateTexturesTraverseTimer;
static SettingT<INISettingCollection> fINIUpgradeTexturesDistance("fUpgradeTexturesDistance:General", 1500.0f);
static SettingT<INISettingCollection> fINIDegradeTexturesDistance("fDegradeTexturesDistance:General", 2500.0f);
SettingT<INISettingCollection> bAllowYesToAll("bAllowYesToAll:MESSAGES", true);
SettingT<INISettingCollection> bSkipProgramFlows("bSkipProgramFlows:MESSAGES", true);
SettingT<INISettingCollection> bSkipInitializationFlows("bSkipInitializationFlows:MESSAGES", true);

// The program's window, which WinMain creates; appMain->hWnd holds its
// native handle.
SDL_Window* pSDLMainWindow;

// An SDL window's native handle, as the game keeps it (Main::hWnd,
// the renderer's device window): the Win32 window where SDL has one; elsewhere the SDL window itself (the property's
// default), which dxvk-native takes through the untyped HWND.
static HWND__* GetNativeWindow_SDL(SDL_Window* apWindow) {
    return static_cast<HWND__*>(
        SDL_GetPointerProperty(SDL_GetWindowProperties(apWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, apWindow)
    );
}

// The window's event handler, standing for the game's window procedure WinProc: per SDL event, it does what WinProc
// does for the window message the event stands for, with the same calls in the same order. Nothing stands for the
// messages WinProc only answers (WM_SIZE, WM_ERASEBKGND, WM_IME_SETCONTEXT). WM_SYSKEYDOWN's swallowing of the system
// keys is handled by WinMain's SDL_HINT_WINDOWS_CLOSE_ON_ALT_F4, and WM_SYSCOMMAND's refusal of the screen saver (with
// a 360 controller) is SDL's own behaviour (it keeps the screen saver off while it runs).
static void WinProc_SDL(const SDL_Event& kEvent) {
    switch (kEvent.type) {
        // WM_DESTROY: the window closed.
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        case SDL_EVENT_QUIT: {
            if (appMain) {
                appMain->QuitGame();
            }
            break;
        }
        // WM_ACTIVATE, activated (WA_ACTIVE or WA_CLICKACTIVE, not minimized).
        case SDL_EVENT_WINDOW_FOCUS_GAINED: {
            if (appMain) {
                appMain->SetActive(true);
                appMain->GetControls()->SetMouseBackgroundStatus(false);
            }
            break;
        }
        // WM_NCACTIVATE with wParam 0, then WM_ACTIVATE with WA_INACTIVE: the window lost the focus.
        case SDL_EVENT_WINDOW_FOCUS_LOST: {
            bWindowDeactivated = true;
            if (appMain && appMain->bGameActive) {
                if (appMain->bOnIdle &&
                    !Interface::IsMenuQueuedToOpen() &&
                    !Menu::GetMenuByClass(1013) &&
                    !Menu::GetMenuByClass(1007)) {
                    Interface::QueueMenuCreate(
                        Interface::OPEN_PAUSE,
                        nullptr,
                        nullptr,
                        nullptr,
                        Interface::CMM_STANDARD,
                        false
                    );
                }

                if (appMain && !spcameraPath.m_pObject) {
                    appMain->SetActive(false);
                }
            }

            break;
        }
        // WM_MOUSEMOVE: the cursor's position in the client area's pixels (the background-mouse menus), which are the
        // screen's. SDL's position is in the window's units, scaled here to the screen's size: the two differ once
        // the window manager resizes the window or the display is scaled.
        case SDL_EVENT_MOUSE_MOTION: {
            // In double, so that equal sizes give SDL's position back exactly.
            double dX = kEvent.motion.x;
            double dY = kEvent.motion.y;
            int32_t iWindowWidth = 0;
            int32_t iWindowHeight = 0;
            if (SDL_GetWindowSize(pSDLMainWindow, &iWindowWidth, &iWindowHeight) &&
                iWindowWidth > 0 &&
                iWindowHeight > 0) {
                dX = dX * Renderer::GetScreenWidth() / iWindowWidth;
                dY = dY * Renderer::GetScreenHeight() / iWindowHeight;
            }
            appMain->HandleMouseMove(static_cast<int32_t>(dX), static_cast<int32_t>(dY));
            break;
        }
        // WM_NCHITTEST: while Controls exist, the cursor is hidden over the window's client area and shown elsewhere.
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
            if (appMain && appMain->GetControls()) {
                if (kEvent.type == SDL_EVENT_WINDOW_MOUSE_ENTER) {
                    if (bIsCursorVisible) {
                        SDL_HideCursor();
                        bIsCursorVisible = false;
                    }
                }
                else if (!bIsCursorVisible) {
                    SDL_ShowCursor();
                    bIsCursorVisible = true;
                }
            }

            break;
        }
        // WM_WINDOWPOSCHANGED: the window's new position.
        case SDL_EVENT_WINDOW_MOVED: {
            if (Renderer::GetNIRenderer()) {
                Renderer::Reposition(kEvent.window.data1, kEvent.window.data2);
            }
            break;
        }
        default: break;
    }
}

int32_t WINAPI WinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, char* lpCmdLine, int32_t nShowCmd) {
    if (!platform::SingleInstance("FalloutNVGameMutex")) {
        platform::ShowMessageBox("There is already an instance of Fallout: New Vegas running", "Error", 0);
        return 0;
    }

    if (platform::RemoteSession()) {
        platform::ShowMessageBox("This application cannot be run from remote desktop.", "Error", 0);
        return 0;
    }

    char acPath[264];
    bool bDisableWarningMessageBox = true;
    bool bSkipInitializationFlows = ::bSkipInitializationFlows.Bool();
    bool bSkipProgramFlows = ::bSkipProgramFlows.Bool();
    MessageHandler::Init(true, bAllowYesToAll.Bool(), bSkipProgramFlows, bSkipInitializationFlows, &MessageBoxCallback);
    MessageHandler::SetWarningFileName("Warnings.txt");
    MessageHandler::SetProgramFlowFileName("ProgramFlow.txt");
    MessageHandler::SetProgrammerOutputFileName("ProgrammerOutput.txt");
    MessageHandler::SetCrashTraceFileName("StackTrace.txt");
    // GetSystemInfo's dwNumberOfProcessors: the logical processors.
    int32_t* pNumHWThreads = &iNumHWThreads.Int();
    *pNumHWThreads = static_cast<int32_t>(std::thread::hardware_concurrency());
    Main_LoadIniFile(acPath);
    if (iNumHWThreads.Int() == 1) {
        iNumHWThreads.Int() = 2;
    }
    // The game ends here when FindWindowA finds another instance's window of class pAppClassName, after
    // bringing it to the front (SetForegroundWindow). SDL cannot reach another process's window; the
    // single-instance check above already ends a second instance.
    {
        // Starts empty (a departure): the exe leaves it uninitialised, so an unset install path gives the bare launcher
        // name here.
        char acLauncherPath[264] = {};
        FillUserNameGlobal(pAppClassName);
        BSSystemUtility::QInstance().pEventNotificationCallback = &SystemUtilityEventNotificationCallback;
        BSSystemUtility::QInstance().GetMsgDialogUtility()->pIdleCallback = &SystemUtilityDialogIdleCallback;
        NiStaticDataManager::Init(nullptr);
        BSMain::InitSDM();
        FixedStrings::InitSDM();
        Menu::InitSDM();
        BSShaderSDM::InitSDM();
        TES::InitSDM();
        SetMemContext(MEM_CONTEXT_UNKNOWN);
        BSFadeNode::SetMaxAlphaAccessFunc(&BSFadeNode::GetLinkedActorAlpha);
        BSSoundHandle::SetAddMovingSoundCallback(&BSFadeNode::AddMovingSoundCallback);
        NiSourceTexture::SetDestroyAppDataFlag(true);
        BSStream::InitLoaders();
        BSSearchPath::InitCallback();
        AutoMemContext kMemContext(
            MEM_CONTEXT_HAVOK,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
            2272
        );
        FOCollisionListener::Instance();
        kMemContext.Leave();
        kMemContext
            .Enter(MEM_CONTEXT_GAMEBRIO, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 2278);
        BSTexturePalette::Init();
        kMemContext.Leave();
        InitSteamAPI();
        // The command line's arguments, the program first.
        for (const std::string& kArgument : platform::CommandLine()) {
            if (kArgument[0] == '-' && !_stricmp(kArgument.c_str() + 1, "nocloud")) {
                SteamAPICallbacks::GetSingleton()->SetNoCloud(false);
            }
        }
        SettingT<RegSettingCollection>::GetCollection()->ReadSettingsFromFile("Software\\Bethesda Softworks\\Fallout");
        SettingT<INISettingCollection>::GetCollection()->ReadSettingsFromFile(acPath);
        BSstrcpy(acPath, 260, BSUtilities::GetPersonalPath());
        BSstrcat(acPath, 260, pAppININame);
        SettingT<INIPrefSettingCollection>::GetCollection()->ReadSettingsFromFile(acPath);
        BSstrcpy(acPath, 260, BSUtilities::GetPersonalPath());
        BSstrcat(acPath, 260, pAppPrefININame);
        SettingT<RendererSettingCollection>::GetCollection()->ReadSettingsFromFile(acPath);
        SettingT<INIPrefSettingCollection>::GetCollection()->ReadSettingsFromFile(acPath);
        if (iNumHWThreads.Int() == 1) {
            iNumHWThreads.Int() = 2;
        }
        if (CreateDisplayDriver()) {
            Renderer::Kill(true);
            BSWin32TaskletManager* pTaskletManager = static_cast<BSWin32TaskletManager*>(
                &BSTaskletManager::QInstance()
            );
            pTaskletManager->ShutdownAllTasks();
            GetInstalledPathRegistryKey(acLauncherPath, 260u);
            BSstrcat(acLauncherPath, 260, "FalloutNVLauncher.exe");
            // The game ends after starting the launcher whether or not the launcher started.
            platform::Open(acLauncherPath);
            return 0;
        }

        const char* pUnessentialFileCacheList = sUnessentialFileCacheList.String();
        const char* pEssentialFileCacheList = sEssentialFileCacheList.String();
        BSFileCache::QInstance().SetFileCacheList(pEssentialFileCacheList, pUnessentialFileCacheList);
        new FileFinder(sBasePathS.String(), iFileLogging.Int(), false);
        FileFinder::AddPath_0("DATA");
        bSkipInitializationFlows = ::bSkipInitializationFlows.Bool();
        bSkipProgramFlows = ::bSkipProgramFlows.Bool();
        MessageHandler::Init(
            bDisableWarningMessageBox,
            bAllowYesToAll.Bool(),
            bSkipProgramFlows,
            bSkipInitializationFlows,
            &MessageBoxCallback
        );
        MessageHandler::SetWarningFileName("Warnings.txt");
        MessageHandler::SetProgramFlowFileName("ProgramFlow.txt");
        MessageHandler::SetProgrammerOutputFileName("ProgrammerOutput.txt");
        MessageHandler::SetCrashTraceFileName("StackTrace.txt");
        kMemContext.Enter(
            MEM_CONTEXT_FILESYSTEM,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
            2450
        );
        InitializationFlow();
        BSFileCache::QInstance().ForcePauseFileCaching();
        kMemContext.Leave();
        kMemContext
            .Enter(MEM_CONTEXT_ARCHIVES, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 2460);
        SetupArchiveSettings_AndLoadBSAs();
        kMemContext.Leave();
        // The window is SDL's, and SDL reads the input: its video and gamepad parts. The window matches the
        // game's: icon resource 101, a 320x240 client area at 0,0, shown; Alt+F4 does not close it, as the
        // game's window procedure swallows WM_SYSKEYDOWN.
        SDL_SetHint(SDL_HINT_WINDOWS_INTRESOURCE_ICON, "101");
        SDL_SetHint(SDL_HINT_WINDOWS_CLOSE_ON_ALT_F4, "0");
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
#if defined(_WIN32)
        pSDLMainWindow = SDL_CreateWindow(pAppClassName, 320, 240, 0);
#else
        // Linux: created hidden, and shown at its final size by Renderer::Init, so that the 320x240 window
        // does not stand on the desktop for the seconds before the device takes it.
        pSDLMainWindow = SDL_CreateWindow(pAppClassName, 320, 240, SDL_WINDOW_HIDDEN);
#endif
        SDL_SetWindowPosition(pSDLMainWindow, 0, 0);
        // A display that goes away (the compositor or session ending) is a quit request, as the window's
        // close is, so the game ends through its quit path, not Xlib's exit().
        platform::QuitOnDisplayLoss(pSDLMainWindow);
        HWND__* pWindow = GetNativeWindow_SDL(pSDLMainWindow);
        appMain = new Main(pWindow, hInstance);
        kMemContext.Enter(
            MEM_CONTEXT_INTERFACE,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
            2552
        );
        pFaderManager = new FaderManager;
        kMemContext.Leave();
        InitializationFlow("Initializing Renderer...");
        appMain->InitRenderer();
        NiTexture::bLoadDirectToRenderer = true;
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->LockRenderer();
        pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->AddLostDeviceNotificationFunc(&Main::LostDeviceCallback, nullptr);
        pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->AddResetNotificationFunc(&Main::ResetDeviceCallback, nullptr);
        Main_ShaderInit();
        appMain->InitSceneGraph();
        Interface::Init(true);
        BSFaceGenManager::InitializeSystem();
        appMain->SetActive(true);
        NavMeshRender::InitSingleton();
        Main_ImagespaceInit();
        pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->UnlockRenderer();
        BGSMoviePlayer::Create();
        const char* pIntroSequence = BGSMoviePlayer::sIntroSequence.String();
        pMoviePlayer->StartSequence(pIntroSequence, false, -1, true, true, 0.0f);
        kMemContext.Leave();
        appMain->InitTES(Main::GetWorldSceneGraph());
        BSFaceGenManager::SetCamera(Main::GetWorldSceneGraph()->GetCamera());
        BSTreeManager::SetCamera(Main::GetWorldSceneGraph()->GetCamera());
        if (iNumHWThreads.Int() > 1) {
            AILinearTaskThreadManager::QInstance()->CreateThreads();
        }
        Pathing::Init();
        PathManager::QInstance()->Initialize();
        if (bUseObstacleAvoidance.Bool()) {
            NavMeshObstacleManager::GetInstance()->Initialize();
        }
        ScreenSplatter::Init();
        ScreenCustomSplatter::Init();
        char* pStartCellName = nullptr;
        NiPoint3 kInitialPos(2048.0f, 2048.0f, 128.0f);
        TESWorldSpace* pWorldSpace = static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item);
        TES::pInstance->SetWorldSpace(pWorldSpace);
        pPlayer->Preload1stPersonAnimsFileList();
        pSaveLoadGameOLD->m_bAllowChanges = true;
        InitializationFlow("Initializing Actor Locations...");
        ReferenceProcessLists.InitPackageLocations();
        pDataHandler->UpdateAutoCalcNPCs();
        pSaveLoadGame->SetGlobalAllowChanges(true);
        InitializationFlow("Loading initial area...");
        Main_InitStartCell(pStartCellName, &kInitialPos);
        InitializationFlow("Placing player...");
        Main::InitPlayerStart(kInitialPos);
        Interface::InitSecond();
        appMain->bOnIdle = true;
        ArchiveManager::SetStartupFinished();
        if (pPlayer->GetParentCell()) {
            pMoviePlayer->WaitForSequenceToEnd();
        }
        else {
            pDataHandler->SetMasterFileLargeBuffer(-1);
            InitializationFlow();
            Interface::CreateStartMenu();
            pMoviePlayer->WaitForSequenceToEnd();
            InitializationFlow();
        }
        pIOManager->LoadAllQueuedPriorities();
        InitializationFlow("Begin Idle loop...");
        bool bInactive = false;
        while (true) {
            // The pending window events go through WinProc_SDL; the focus test is SDL's input focus.
            SDL_Event kEvent;
            while (SDL_PollEvent(&kEvent)) {
                WinProc_SDL(kEvent);
            }
            if ((SDL_GetWindowFlags(pSDLMainWindow) & SDL_WINDOW_INPUT_FOCUS) ||
                (TES::pInstance->CellTestRunning() || (spcameraPath.m_pObject || bAlwaysActive.Bool()))) {
                pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                int32_t iCooperativeLevel = pRenderer->GetD3DDevice()->TestCooperativeLevel();
                if (iCooperativeLevel >= 0) {
                    appMain->OnIdle();
                }
                else {
                    pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                    pRenderer->BeginOffScreenFrame();
                    pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                    pRenderer->EndOffScreenFrame();
                    Sleep(50u);
                }
                if (bInactive) {
                    bInactive = false;
                    BSAudio::QInstance().UnPauseSynchTimer();
                    FalloutAudioMedia::MediaMenuUnPause();
                    FalloutRadio::Update(false);
                    BSAudio::QInstance().Update(true);
                }
                if (!bInactive && appMain->oneMore && iCooperativeLevel >= 0) {
                    LoadingMenu::ResumeBackgroundThread();
                    appMain->oneMore = false;
                }
            }
            else {
                if (!bInactive) {
                    bInactive = true;
                    if (!LoadingMenuThread::GetByte49()) {
                        uint32_t uiCurrentThreadID = GetCurrentThreadId();
                        if (uiCurrentThreadID == appMain->uiThreadID) {
                            LoadingMenu::SuspendBackgroundThread();
                            appMain->oneMore = true;
                        }
                    }
                    BSAudio::QInstance().PauseSynchTimer();
                    FalloutAudioMedia::MediaMenuPause();
                    FalloutRadio::Update(false);
                    BSAudio::QInstance().Update(true);
                    appMain->QDrawWorldAccum()->ClearSunQueries();
                    BGSDecalManager::GetSingleton()->ClearPendingQueries();
                    Interface::GetVATSEffect()->FreeOcclusionQueries();
                    // A full-screen window is not re-fitted here; the full-screen window is the device's
                    // (Direct3D 9's, dxvk's SDL backend).
                }
                Sleep(50u);
            }
            if (appMain->bQuitGame) {
                break;
            }
            if (appMain->GameReset()) {
                appMain->PerformGameReset(true);
            }
        }
        appMain->bOnIdle = false;
        DestroySteamAPICallbacks();
        Interface::HideMenus();
        Interface::CloseAllSupportMenus(false);
        if (Main::spMenuBGTexture.m_pObject) {
            BSRenderedTexture* pTexture = Main::spMenuBGTexture.m_pObject;
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            pTextureManager->ReturnRenderedTexture(pTexture);
            Main::spMenuBGTexture = static_cast<BSRenderedTexture*>(nullptr);
        }
        if (Main::spInterfaceFXTex.m_pObject) {
            BSRenderedTexture* pTexture = Main::spInterfaceFXTex.m_pObject;
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            pTextureManager->ReturnRenderedTexture(pTexture);
            Main::spInterfaceFXTex = static_cast<BSRenderedTexture*>(nullptr);
        }
        if (Main::spVATSTex.m_pObject) {
            BSRenderedTexture* pTexture = Main::spVATSTex.m_pObject;
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            pTextureManager->ReturnRenderedTexture(pTexture);
            Main::spVATSTex = static_cast<BSRenderedTexture*>(nullptr);
        }
        TaskQueueInterface::SetActive(false);
        if (NiParallelUpdateTaskManager::Get()) {
            NiParallelUpdateTaskManager::Shutdown();
        }
        pModelLoader->CancelAllFiles();
        if (iNumHWThreads.Int() > 1) {
            AILinearTaskThreadManager::QInstance()->DestroyThreads();
        }
        BGSMoviePlayer::Destroy();
        Main::DestroyInterfaceSceneGraph();
        Main::DestroyInterface3DSceneGraph();
        Interface::Free();
        if (TES::pInstance->pSky) {
            TES::pInstance->pSky->Destroy();
        }
        ScreenSplatter::Shutdown();
        ScreenCustomSplatter::Shutdown();
        FORenderedTerminal::ReleaseStaticGeometry();
        FalloutRadio::Shutdown();
        FalloutAudioMedia::MediaClose();
        FalloutAudioMedia::Shutdown();
        BSAudio::QInstance().Shutdown();
        InitializationFlow();
        BGSDecalManager::Destroy_();
        DecalCaster::ClearInstance();
        HitCaster::ClearInstance();
        if (bUseObstacleAvoidance.Bool()) {
            NavMeshObstacleManager::GetInstance()->Release();
        }
        if (TES::pInstance) {
            delete TES::pInstance;
        }
        BGSTerrainManager::DeallocateStaticNodes();
        PathManager::QInstance()->Release();
        BSFaceGenManager::DestroySystem();
        CombatManager::Destroy();
        BSWin32TaskletManager* pTaskletManager = static_cast<BSWin32TaskletManager*>(&BSTaskletManager::QInstance());
        pTaskletManager->ShutdownAllTasks();
        BSTreeManager::Destroy();
        BGSLoadedReferenceCollection::Destroy();
        EffectSettingCollection::Reset();
        ImpactMixer::DeInitialize();
        if (pFaderManager) {
            delete pFaderManager;
        }
        if (appMain) {
            delete appMain;
        }
        appMain = nullptr;
        BSFileCache::QInstance().Shutdown();
        char* pSettingFile = SettingT<INIPrefSettingCollection>::GetCollection()->pSettingFile;
        SettingT<INIPrefSettingCollection>::GetCollection()->SaveAll(pSettingFile);
        pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->RemoveResetNotificationFunc(&Main::ResetDeviceCallback);
        pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->RemoveLostDeviceNotificationFunc(&Main::LostDeviceCallback);
        NavMeshRender::Destroy();
        BSTexturePalette::Kill();
        BSShaderSDM::KillSDM();
        Renderer::Kill(true);
        FileFinder::Close();
        TES::KillSDM();
        BSMain::KillSDM();
        FixedStrings::KillSDM();
        Menu::KillSDM();
        NiStaticDataManager::Shutdown();
        MemoryManagerFile kFile("ExitData.mhd");
        MemoryManager::Instance().OutputDebugInfo(&kFile, false);

        return 0;
    }
}

HWND__* GetActiveWindow_SDL() {
    // GetActiveWindow answers for the calling thread's windows, and only the main thread has one; SDL's focus is also
    // the main thread's to read.
    if (!SDL_IsMainThread() || !pSDLMainWindow || SDL_GetKeyboardFocus() != pSDLMainWindow) {
        return nullptr;
    }
    return GetNativeWindow_SDL(pSDLMainWindow);
}

float ShaderTimer(int32_t iDelta) {
    switch (iDelta) {
        case 3: {
            return appTimer.GetSeconds();
        }
        case 2: {
            if (Interface::IsInMenuMode()) {
                return 0.0;
            }

            return static_cast<float>(appTimer.BSTimer::GetTime() / 1000.0);
        }
        case 1: {
            if (!Interface::IsInMenuMode()) {
                return appTimer.GetSeconds();
            }
            return 0.0;
        }
        default: {
            if (iDelta) {
                return 0.0;
            }

            return static_cast<float>((appCalendar.GetHour() * 60.0) * 60.0);
        }
    }
}

void Main_ShaderInit() {
    InitializationFlow("Initializing Shader System...");
    BSShaderManager::SetShaderErrorCallback(nullptr);
    BSShaderManager::SetTimerFunc(ShaderTimer);
    BSShaderManager::SetUseHardDriveCache(bUseHardDriveCache.Bool());
    BSShaderManager::bDisplayLODLand = bDisplayLODLandINI.Bool();
    BSShaderManager::fLocalTreeMipMapLODBias = static_cast<float>(fLocalTreeMipMapLODBiasINI);
    BSShaderManager::fLODTreeMipMapLODBias = static_cast<float>(fLODTreeMipMapLODBiasINI);
}

// The blur multipliers and the sky shader's sun and sky brightness for HDR or not, then the motion blur setting.
void Main_ImagespaceInit() {
    if (BSShaderManager::bHDR) {
        ImageSpaceEffectBlur::fSpecularMult = Renderer::GetBlurSpecularMultHDR();
        ImageSpaceEffectBlur::fEmmisiveMult = Renderer::GetBlurEmmisiveMultHDR();
        SkyShader::SetSunBrightness(Renderer::GetBlurSunBrightnessHDR());
        SkyShader::SetSkyBrightness(Renderer::GetBlurSkyBrightnessHDR());
    }
    else {
        SkyShader::SetSunBrightness(Renderer::GetBlurSunBrightness());
        SkyShader::SetSkyBrightness(Renderer::GetBlurSkyBrightness());
        ImageSpaceEffectBlur::fSpecularMult = Renderer::GetBlurSpecularMult();
        ImageSpaceEffectBlur::fEmmisiveMult = Renderer::GetBlurEmmisiveMult();
    }
    float fMotionBlur = ::fMotionBlur.Float();
    static_cast<ImageSpaceEffectMotionBlur*>(
        BSShaderManager::GetImageSpaceManager()->GetEffect(ImageSpaceManager::EFFECT_MOTION_BLUR)
    )
        ->SetFullScreenBlurConst(fMotionBlur);
}

BSMsgDialogSystemUtility::USER_RESPONSE SystemUtilityDialogIdleCallback() {
    if (!NiRenderer::GetRenderer()) {
        return BSMsgDialogSystemUtility::USER_RESPONSE_NONE;
    }
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    pRenderer->DoMinimalFlip();
    return BSMsgDialogSystemUtility::USER_RESPONSE_NONE;
}

// ExitProcess ends the process without the CRT's atexit handlers and static destructors: std::_Exit.
void MessageMenuCallback() {
    char cMessageMenuResult = Interface::GetMessageMenuResult();
    if (cMessageMenuResult == 1 || cMessageMenuResult == 3) {
        InitializationFlow("Warning Response was to Exit the Process");
        if (platform::DebuggerPresent()) {
            xfnv::DebugTrap();
        }
        std::_Exit(0);
    }
    if (cMessageMenuResult == 2) {
        Interface::MessageMenuYesToAll();
    }
}

// The registry's Installed Path (platform::InstallFolder) into apPath, at most auiSize bytes with its NUL. apPath is
// left as it is when the value is missing or does not fit, as RegQueryValueExA leaves it.
void GetInstalledPathRegistryKey(char* apPath, uint32_t auiSize) {
    const std::string kFolder = platform::InstallFolder();
    if (!kFolder.empty() && kFolder.size() < auiSize) {
        BSstrcpy(apPath, auiSize, kFolder.c_str());
    }
}

int32_t Main_LoadIniFile(char* lpFileName) {
    bool bUseMyGames = true;
    BSsprintf(lpFileName, 260, ".\\%s", pAppCustomININame);
    if (!platform::IniGetInt(lpFileName, "General", "bUseMyGamesDirectory", 1)) {
        bUseMyGames = false;
    }
    BSsprintf(lpFileName, 260, ".\\%s", pAppININame);
    if (bUseMyGames && !platform::IniGetInt(lpFileName, "General", "bUseMyGamesDirectory", 1)) {
        bUseMyGames = false;
    }
    if (bUseMyGames) {
        char acPath[264];
        std::error_code kError;
        *lpFileName = 0;
        // CSIDL_LOCAL_APPDATA (0x1C), then CSIDL_PERSONAL (5).
        BSstrcpy(acPath, 260, platform::UserFolder(platform::UserFolderKind::LocalAppData).c_str());
        BSstrcat(acPath, 260, "\\FalloutNV\\");
        std::filesystem::create_directory(platform::NativePath(acPath), kError);
        BSstrcpy(BSUtilities::GetLocalAppDataPath(), 260, acPath);
        BSstrcpy(acPath, 260, platform::UserFolder(platform::UserFolderKind::Documents).c_str());
        BSstrcat(acPath, 260, "\\My Games\\");
        std::filesystem::create_directory(platform::NativePath(acPath), kError);
        BSstrcat(acPath, 260, "FalloutNV\\");
        std::filesystem::create_directory(platform::NativePath(acPath), kError);
        BSstrcpy(BSUtilities::GetPersonalPath(), 260, acPath);
    }
    else {
        BSstrcpy(BSUtilities::GetLocalAppDataPath(), 260, ".\\");
        BSstrcpy(BSUtilities::GetPersonalPath(), 260, ".\\");
    }
    BSstrcpy(lpFileName, 260, BSUtilities::GetPersonalPath());
    BSstrcat(lpFileName, 260, pAppININame);
    std::error_code kError;
    std::filesystem::remove(platform::NativePath(MessageHandler::GetWarningFileName()), kError);
    std::filesystem::remove(platform::NativePath(MessageHandler::GetProgramFlowFileName()), kError);
    std::filesystem::remove(platform::NativePath(MessageHandler::GetProgrammerOutputFileName()), kError);
    int32_t iResult = rt::_access(lpFileName, 0);
    if (iResult == -1) {
        char acDefaultFileName[264];
        BSstrcpy(acDefaultFileName, 260, "Fallout_default.ini");
        // CopyFileA with bFailIfExists: the copy's success as the BOOL.
        return std::filesystem::copy_file(
            platform::NativePath(acDefaultFileName),
            platform::NativePath(lpFileName),
            std::filesystem::copy_options::none,
            kError
        );
    }
    return iResult;
}

void SetupArchiveSettings_AndLoadBSAs() {
    const char* pArchiveList = sMasterArchiveList.String();
    const char* pMasterPath = sLocalMasterPath.String();
    ArchiveManager::Init(bUseArchives.Bool(), pMasterPath, pArchiveList, 0x200000u);

    ArchiveManager::SetCheckRuntimeCollisions(bCheckRuntimeCollisions.Bool());
    ArchiveManager::SetRetainTables(
        iRetainDirectoryStringTable.Int(),
        iRetainFilenameStringTable.Int(),
        iRetainFilenameOffsetTable.Int()
    );
    const char* pInvalidationFile = sArchiveInvalidationFile.String();
    ArchiveManager::SetInvalidation(bInvalidateOlderFiles.Bool(), pInvalidationFile);
    ArchiveManager::OpenMasterArchives();
}

void SystemUtilityEventNotificationCallback(BSSystemUtility::EVENT_NOTIFICATION aeEventNotification) {
    if (aeEventNotification == BSSystemUtility::EVENT_NOTIFICATION_USER_REQUEST_EXIT) {
        BGSMoviePlayer::Destroy();
        if (pIOManager) {
            pIOManager->Pause();
        }
        pModelLoader->SetExiting();
        pIOManager->SetThreadExiting();
        pIOManager->Resume();
        BSFileCache::QInstance().ForcePauseAndWaitForIdle();
        BSFileCache::QInstance().Shutdown();
        FalloutAudioMedia::MediaClose();
        FalloutAudioMedia::Shutdown();
        BSAudio::QInstance().Shutdown();
        LoadingMenu::SetThreadExiting();
        LoadingMenu::KillThread();
        BSTaskletManager::QInstance().ShutdownAllTasks();
        if (pIOManager) {
            pIOManager->Pause();
        }
    }
}

int32_t MessageBoxCallback(const char* apText, const char* apCaption, uint32_t aiType) {
    int32_t iResult = 6;
    if (!bBlockMessageBoxes.Bool()) {
        const char* pYesText;
        const char* pNoText;
        const char* pYesToAllText;
        appTimer.Disable();
        uint8_t cType = aiType & 15;
        if (!bMessageBoxOnlyOnce || (!appMain || !appMain->GetControls())) {
            if (!Renderer::GetFullScreen() || !Renderer::GetHWND()) {
                if (appMain) {
                    uint32_t uiCurrentThreadID = GetCurrentThreadId();
                    if (uiCurrentThreadID != appMain->uiThreadID && appMain->GetControls()) {
                        if (!appMain->GetControls()->IsMouseInBackground()) {
                            appMain->GetControls()->SetMouseBackgroundStatus(true);
                        }
                    }
                }
                iResult = platform::ShowMessageBox(apText, apCaption, aiType);
            }
        }
        else {
            bMessageBoxOnlyOnce = false;
            pYesText = nullptr;
            pNoText = nullptr;
            pYesToAllText = nullptr;
            switch (cType) {
                case 3: {
                    pYesToAllText = ::pYesToAllText.String();
                    [[fallthrough]];
                }
                case 4: {
                    pYesText = ::pYesText.String();
                    pNoText = ::pNoText.String();
                    break;
                }
                default: {
                    break;
                }
            }
            if (cType == 2 ||
                (!cType ||
                 !Interface::CreateMessageMenu(
                     apText,
                     static_cast<const char*>(nullptr),
                     static_cast<const char*>(nullptr),
                     &MessageMenuCallback,
                     0,
                     MessageHandler::QWarningContext(),
                     0.0f,
                     0.0f,
                     pYesText,
                     pNoText,
                     pYesToAllText,
                     nullptr
                 ))) {
                if (Renderer::GetFullScreen() && Renderer::GetHWND()) {
                    if (cType == 2) {
                        iResult = 5;
                    }
                    else if ((aiType & 15) > 2 && (aiType & 15) <= 4) {
                        iResult = 6;
                    }
                }
                else {
                    if (appMain) {
                        uint32_t uiCurrentThreadID = GetCurrentThreadId();
                        if (uiCurrentThreadID != appMain->uiThreadID && appMain->GetControls()) {
                            if (!appMain->GetControls()->IsMouseInBackground()) {
                                appMain->GetControls()->SetMouseBackgroundStatus(true);
                            }
                        }
                    }
                    iResult = platform::ShowMessageBox(apText, apCaption, aiType);
                }
            }
            bMessageBoxOnlyOnce = true;
        }
        appTimer.Enable();
        return iResult;
    }
    if ((aiType & 15) == 2) {
        return 5;
    }

    return 6;
}

Main::Main(HWND__* ahWnd, HINSTANCE__* ahInstance)
  : bQuitGame(false), bResetGame(false), bGameActive(false), bOnIdle(false), bReloadContent(false), bFlyCamera(false),
    bFreezeTime(false), hWnd(ahWnd), hInstance(ahInstance), uiThreadID(BSGetCurrentThreadID()), hThreadHandle(nullptr),
    kTaskQueue(&kPackedTaskHeap, [](BSPackedTask* apTask) { TaskQueueInterface::TaskUnpackFunc(*apTask); }),
    kSecondaryTaskQueue(
        &kSecondaryPackedTaskHeap,
        [](BSPackedTask* apTask) { TaskQueueInterface::TaskUnpackFunc(*apTask); }
    ),
    bRenderingMenuBG(false) {
    Main::spInterfaceFXTex = nullptr;
    spDrawWorldAccum = new BSShaderAccumulator(99, 1u, 759u);
    spDraw1stPersonAccum = new BSShaderAccumulator(99, 1u, 759u);
    spDraw1stPersonAccum->SetRenderMode(BSShaderManager::BSSM_RENDER_FIRST_PERSON);
    spDraw1stPersonAccum->b1stPerson = true;
    spDraw1stPersonDepthAccum = new BSShaderAccumulator(99, 1u, 759u);
    spDraw1stPersonDepthAccum->SetRenderMode(BSShaderManager::BSSM_RENDER_FIRST_PERSON);
    spDraw1stPersonDepthAccum->b1stPerson = true;
    spScreenSplatterAccum = new BSShaderAccumulator(100, 1u, 4u);
    spScreenSplatterAccum->SetRenderMode(BSShaderManager::BSSM_RENDER_SCREEN_SPLATTER);
    spRenderedMenuAccum = new BSShaderAccumulator(99, 1u, 759u);
    sp1stPersonAdjustedCamera = new NiCamera;
    if (iNumHWThreads.Int() > 1) {
        TaskQueueInterface::Instantiate(kTaskQueue, kSecondaryTaskQueue);
        kTaskQueue.ThreadBeginInput();
        kSecondaryTaskQueue.ThreadBeginInput();
    }
    SetThreadName(uiThreadID, "Main");
    {
        AutoMemContext kMemContext(MEM_CONTEXT_INTERFACE);
        Main_InitControls(ahInstance);
        Controls::GetInstance()->InitControlsFromIni();
    }
    {
        AutoMemContext kMemContext(MEM_CONTEXT_AUDIO);
        Main_AudioInit();
    }
}

Main::~Main() {
    ArchiveManager::CloseAll();
    Controls::GetInstance()->SaveControls();
    Main::pLandLOD = nullptr;
    Main::pObjectLOD = nullptr;
    Main::pWaterLOD = nullptr;
    Controls::DeleteInstance();
    Main::spWorldRoot = nullptr;
    NavMeshRender::spRootDrawNode = nullptr;
    Main::spSkyRoot = nullptr;
    Main::spWeatherRoot = nullptr;
    Main::spParticleSystemManager = nullptr;
    Main::spFog = nullptr;
    Main::spMenuBGTexture = nullptr;
    Main::spMenuBGGeometry = nullptr;
    MessageHandler::Close();
    for (uint32_t i = 0; i < 4; ++i) {
        delete pDeviceNamesA[i];
    }
    for (uint32_t i = 0; i < 238; ++i) {
        delete psKeyboard[i];
    }
    for (uint32_t i = 0; i < 9; ++i) {
        delete psMouse[i];
    }
    for (uint32_t i = 0; i < 8; ++i) {
        delete psJoystick[i];
    }
    for (uint32_t i = 0; i < 28; ++i) {
        delete psUserActions[i];
    }
    for (uint32_t i = 0; i < 14; ++i) {
        delete psWeaponTypes[i];
    }
    for (uint32_t i = 0; i < 20; ++i) {
        delete psTargetTypes[i];
    }
    for (uint32_t i = 0; i < 34; ++i) {
        delete psXBoxGamepadText[i];
    }
    for (uint32_t i = 0; i < 34; ++i) {
        delete g_PS3ButtonSettingNames[i];
    }
    if (iNumHWThreads.Int() > 1) {
        kTaskQueue.ThreadEndInput();
        kSecondaryTaskQueue.ThreadEndInput();
        TaskQueueInterface::Shutdown();
    }
}

void Main::ForceUpdateTextures() {}

bool Main::UpdateTextures(float afTime) {
    return false;
}

bool Main::QQuit() const {
    return bQuitGame;
}

bool Main::GetFlyCamera() const {
    return bFlyCamera;
}

bool Main::GetOnIdle() const {
    return bOnIdle;
}

BSShaderAccumulator* Main::QDrawWorldAccum() const {
    return spDrawWorldAccum;
}

NiRenderedTexture* Main::GetMenuBGTexture() {
    // GetTexture is called even for a null texture; GetTexture returns null for a null this.
    return spMenuBGTexture->GetTexture(0);
}

Controls* Main::GetControls() {
    return Controls::GetInstance();
}

uint32_t Main::GetThread() {
    return uiThreadID;
}

void Main::Main_AudioInit() {
    BSWin32Audio::Create();
    BSAudio::QInstance().pfnIDCallback = &FalloutAudio::ResolveSoundID;
    BSAudio::QInstance().pfnNameCallback = &FalloutAudio::ResolveSoundName;
    BSAudio::QInstance().pfnRandomFilenameCallback = &TESSound::PickRandomFilename;
    BSAudio::QInstance().pfnSettingsCallback =
        [](BSSoundHandle* apSoundHandle, void* apSettingsContext, uint32_t* apOutFlags) {
            return FalloutAudio::ApplySettingsFromMasterFile(*apSoundHandle, apSettingsContext, apOutFlags);
        };
    BSAudio::QInstance().pfnSynchPausedCallback = &FalloutRadio::RadioSynchPaused;
    BSAudio::QInstance().pfnSynchUnPausedCallback = &FalloutRadio::RadioSynchUnPaused;
    BSAudio::QInstance().SetLoopPointCallback(&FalloutAudio::GetLoopPoints);
    BSAudio::QInstance().Init(hWnd);

    FalloutAudioMedia::Startup();
}

void Main::InitTES(NiNode* pScene) {
    NiPoint3 kStartPos;
    SettingT<INISettingCollection>* pTestFiles[10];
    AutoMemContext kTempMemContext(
        MEM_CONTEXT_GAME_OVERHEAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
        3663
    );
    kTempMemContext.Enter(
        MEM_CONTEXT_GAME_OVERHEAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
        3665
    );
    Main::pSky = Sky::GetInstance();
    Main::InitSky();
    NiNode* pObjects = Main::ObjectLODNode();
    NiNode* pLands = Main::LandLODNode();
    if (pLands) {
        Main::pSky->GetLight()->AttachAffectedNode(pLands);
        NiAVObject* pChild = pLands->GetAtChecked(0);
        pLands = pChild ? pChild->IsNode() : nullptr;
    }
    InitializationFlow("Initializing TES...");
    TES* pTES = new TES("DATA\\", pObjects, pLands, Main::pSky, Main::pWaterLOD);
    TES::pInstance = pTES;
    kTempMemContext.Leave();
    TES::pInstance->InitializeWater();
    InitializationFlow("Initializing TreeManager...");
    BSTreeManager::Create(false);
    InitializationFlow("Activating Tasklet Threads...");
    BSTaskletManager::QInstance().SignalInstanceAvailable();
    BGSLoadedReferenceCollection::Create();
    CombatManager::Create();
    kTempMemContext
        .Enter(MEM_CONTEXT_WORLD_SG, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 3717);
    pScene->UpdateProperties();
    NiUpdateData kUpdateData(0.0f, false, false);
    pScene->Update(kUpdateData);
    kTempMemContext.Leave();
    bool bFilesChecked = false;
    pTestFiles[0] = &sTestFile1;
    pTestFiles[1] = &sTestFile2;
    pTestFiles[2] = &sTestFile3;
    pTestFiles[3] = &sTestFile4;
    pTestFiles[4] = &sTestFile5;
    pTestFiles[5] = &sTestFile6;
    pTestFiles[6] = &sTestFile7;
    pTestFiles[7] = &sTestFile8;
    pTestFiles[8] = &sTestFile9;
    pTestFiles[9] = &sTestFile10;
    for (int32_t n = 0; n < 10; ++n) {
        if (pTestFiles[n]->String()) {
            if (std::strlen(pTestFiles[n]->String())) {
                const char* pFileName = pTestFiles[n]->String();
                TESFile* pFile = pDataHandler->GetListFile(pFileName);
                if (pFile) {
                    pFile->SetChecked(true);
                    bFilesChecked = true;
                }
            }
        }
    }
    if (LoadPluginsFromFile(BSUtilities::GetLocalAppDataPath(), Plugins_txt)) {
        bFilesChecked = true;
    }
    if (!bFilesChecked) {
        TESFile* pFile = pDataHandler->GetListFile("Fallout.esm");
        if (pFile) {
            pFile->SetChecked(true);
        }
    }
    kTempMemContext
        .Enter(MEM_CONTEXT_PLAYER, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 3787);
    PlayerCharacter* pNewPlayer = new PlayerCharacter;
    pPlayer = pNewPlayer;
    pNewPlayer->SetFormID(20u, true);
    kTempMemContext.Leave();
    InitializationFlow("Loading Files...");
    pDataHandler->CompileFiles(false);
    TESChallengeManager::LoadChallenges();
    TES::pInstance->PreloadAddonNodes();
    TES::pInstance->PreloadDefaultModels();
    if (TESWaterSystem::bUseWater.Bool()) {
        TES::pInstance->PreloadPlaceableWaterNoiseTextures();
    }
    TES::pInstance->PreloadForms();
    BSTempEffectSimpleDecal::PreloadDecalData();
    InitializationFlow("Initializing Player...");
    kTempMemContext
        .Enter(MEM_CONTEXT_PLAYER, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 3840);
    TESForm* pForm = TESForm::GetFormByNumericID(7u);
    TESNPC* pNPC = dynamic_cast<TESNPC*>(pForm);
    pPlayer->SetObjectReference(pNPC);
    ::new (&kStartPos) NiPoint3(2048.0f, 2048.0f, 0.0f);
    pPlayer->SetPosition(kStartPos);

    pPlayer->SetRotation(NiPoint3::ZERO);
    pPlayer->InitItem();
    pPlayer->CastPermanentMagic(false);
    if (!pPlayer->GetActorValue(ActorValue::HEALTH)) {
        InitializationFlow(
            "ERROR: Health value is 0 on the Player. Fix '%s' with the editor.",
            &pNPC->GetFile(0)->m_Filename
        );
    }
    InitializationFlow("Initializing Scripts...");
    pDataHandler->FixAllScriptIDs();
    // The setting's unsigned value is tested with signed compares.
    if (static_cast<int32_t>(static_cast<uint32_t>(iFPSClamp)) > 0 &&
        static_cast<int32_t>(static_cast<uint32_t>(iFPSClamp)) < 200) {
        // ClampAt's aiClamp is unsigned; Int()'s value is passed as it is.
        appTimer.ClampAt(static_cast<uint32_t>(iFPSClamp.Int()));
    }
}

// ExitProcess ends the process without the CRT's atexit handlers and static destructors: std::_Exit.
void Main::InitRenderer() {
    if (!Renderer::Init(hWnd, hInstance)) {
        char acMessage[512];
        std::sprintf(acMessage, "Failed to initialize renderer.\n%s", Renderer::GetReason());
        platform::ShowMessageBox(acMessage, "Fallout", 0);
        std::_Exit(0);
    }
}

void Main::InitSceneGraph() {
    BSParticleSystemManager* pParticleSystemManager;
    NiNode* pObjectLODRoot;
    NiNode* pLODTreeNode;
    NiNode* pWaterLODNode;
    NiNode* pLODNode;
    NiNode* pWeatherRoot;
    BSMultiBoundNode* pSkyRoot;
    BSFogProperty* pFogProp;
    int32_t iCanopyShadowPass;
    ShadowSceneNode* pShadowSceneNode;
    SceneGraph* pSceneGraph;
    AutoMemContext kTempMemContext(
        MEM_CONTEXT_WORLD_SG,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
        3916
    );
    pSceneGraph = new SceneGraph("World", false, nullptr);
    Main::spWorldRoot = static_cast<BSSceneGraph*>(pSceneGraph);
    pShadowSceneNode = new ShadowSceneNode;
    BSShaderManager::SetRenderer(Renderer::GetNIRenderer());
    BSShaderManager::SetDefaultShaderPath("data\\shaders\\");
    BSShaderManager::Initialize();
    Main::spWorldRoot->GetCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
    pShadowSceneNode->SetSceneGraphIndex(0);
    BSShaderManager::SetShadowSceneNode(0, pShadowSceneNode);
    spDrawWorldAccum->SetShadowSceneNode(pShadowSceneNode);
    spDraw1stPersonAccum->SetShadowSceneNode(pShadowSceneNode);
    spDraw1stPersonDepthAccum->SetShadowSceneNode(pShadowSceneNode);
    spScreenSplatterAccum->SetShadowSceneNode(pShadowSceneNode);
    ShadowSceneNode* pMenuShadowSceneNode = BSShaderManager::GetShadowSceneNode(1);
    spRenderedMenuAccum->SetShadowSceneNode(pMenuShadowSceneNode);
    BSShaderManager::SetSeparateAmbientLight(false);
    BSShaderManager::SetSeparateSpecularColor(false);
    int32_t iLightingPasses = Renderer::QDoAmbientPass();
    iLightingPasses |= !Renderer::QDoDiffusePass() ? 0 : 2;
    iLightingPasses |= !Renderer::QDoTexturePass() ? 0 : 4;
    iLightingPasses |= (!Renderer::QDoSpecularPass() ? 0 : 8) | 16;
    if (Renderer::QDoCanopyShadowPass() && BSShaderManager::GetShaderLevel(false) >= BSShaderManager::BSSM_SV_2_0) {
        iCanopyShadowPass = 32;
    }
    else {
        iCanopyShadowPass = 0;
    }
    iLightingPasses |= iCanopyShadowPass;
    BSShaderManager::SetEnabledPasses(iLightingPasses);
    int32_t iShadowCasters = Renderer::QDoActorShadows() ? 8 : 0;
    iShadowCasters |= Renderer::QDoStaticAndArchShadows() ? 2 : 0;
    BSShaderManager::uiObjectTypeShadowMask = iShadowCasters;
    BSShaderManager::SetObjectTypeCallback(&TESObjectREFR::GetLightingObjectType);
    ShadowLightShader::fWaterDepthRange = 0.0f;
    BSShaderManager::spMainCamera = Main::spWorldRoot->GetCamera();
    static_cast<SceneGraph*>(Main::spWorldRoot.m_pObject)->AttachChild(pShadowSceneNode, 1);
    pFogProp = new BSFogProperty;
    Main::spFog = pFogProp;
    Main::spFog->SetFog(true);
    Main::spFog->SetDepth(1.0f);
    BSShaderManager::SetFogProperty(0, Main::spFog.m_pObject);
    pSkyRoot = new BSMultiBoundNode;
    Main::spSkyRoot = static_cast<NiNode*>(pSkyRoot);
    static_cast<BSMultiBoundNode*>(Main::spSkyRoot.m_pObject)
        ->BSMultiBoundNode::SetCullingMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    Main::spSkyRoot->SetFixedBound(true);
    Main::spSkyRoot->SetName("Sky");
    pShadowSceneNode->SetAt(0, Main::spSkyRoot.m_pObject);
    pWeatherRoot = new NiNode(0);
    Main::spWeatherRoot = pWeatherRoot;
    Main::spWeatherRoot->SetAlwaysDraw(true);
    Main::spWeatherRoot->SetFixedBound(true);
    Main::spWeatherRoot->SetName("Weather");
    pShadowSceneNode->SetAt(1u, Main::spWeatherRoot.m_pObject);
    Main::CreateMenuRootNode();
    Main::CreateMenuRootNode3D();
    Main::pLandLOD = new BSClearZNode;
    Main::pLandLOD->SetName("LODRoot");
    pShadowSceneNode->SetAt(2u, Main::pLandLOD);
    pLODNode = new NiNode(0);
    pLODNode->SetAlwaysDraw(true);
    pLODNode->SetFixedBound(true);
    pLODNode->SetName("LandLOD");
    Main::pLandLOD->AttachChild(pLODNode, 1);
    BSShaderManager::fLODLandDrop = fLODLandDropAmount.Float();
    pLODNode = new NiNode(0);
    pLODNode->SetAlwaysDraw(true);
    pLODNode->SetFixedBound(true);
    pLODNode->SetName("DistantRefLOD");
    Main::pLandLOD->AttachChild(pLODNode, 1);
    pWaterLODNode = new NiNode(0);
    Main::pWaterLOD = pWaterLODNode;
    pWaterLODNode->SetAlwaysDraw(true);
    Main::pWaterLOD->SetFixedBound(true);
    Main::pWaterLOD->SetName("WaterLOD");
    Main::pLandLOD->AttachChild(Main::pWaterLOD, 1);
    pLODTreeNode = new NiNode(0);
    pLODTreeNode->SetAlwaysDraw(true);
    pLODTreeNode->SetFixedBound(true);
    pLODTreeNode->SetName("LOD Trees");
    pLODNode->AttachChild(pLODTreeNode, 1);
    DistantLODShaderProperty::SetLODTreeParent(pLODTreeNode);
    BGSTerrainManager::SetTreeNode(pLODTreeNode);
    pObjectLODRoot = new NiNode(0);
    Main::pObjectLOD = pObjectLODRoot;
    pObjectLODRoot->SetAlwaysDraw(true);
    Main::pObjectLOD->SetFixedBound(true);
    Main::pObjectLOD->SetName("ObjectLODRoot");
    pShadowSceneNode->SetAt(3u, Main::pObjectLOD);
    pParticleSystemManager = new BSParticleSystemManager;
    Main::spParticleSystemManager = pParticleSystemManager;
    Main::spParticleSystemManager->SetName("Master Particle Systems");
    pShadowSceneNode->SetAt(5u, Main::spParticleSystemManager.m_pObject);
    Main::spWorldRoot->GetCameraRoot()->SetTranslate(0.0f, 0.0f, 128.0f);
    Main::spWorldRoot->UpdateProperties();
    NiUpdateData kUpdateData(0.0f, false, false);
    Main::spWorldRoot->Update(kUpdateData);
    BSShaderAccumulator* pAccumulator = BSShaderManager::GetAccumulator();
    NiRenderer::GetRenderer()->SetSorter(pAccumulator);
    Main::bStaticMenuBG = Renderer::QDOStaticMenuBackground();
    if (!Main::spMenuBGGeometry.m_pObject) {
        NiTexturingProperty* pTexProp;
        NiMaterialProperty* pMatProp;
        NiScreenElements* pScreenElements;
        pScreenElements = new NiScreenElements(new NiScreenElementsData(false, false, 1, 1, 1, 4, 1, 2, 1));
        Main::spMenuBGGeometry = pScreenElements;
        Main::spMenuBGGeometry->Insert(4, 0, nullptr);
        Main::spMenuBGGeometry->SetRectangle(0, 0.0f, 0.0f, 1.0f, 1.0f);
        Main::spMenuBGGeometry->UpdateBound();
        Main::spMenuBGGeometry->SetTextures(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
        pMatProp = new NiMaterialProperty;
        pMatProp->SetEmittance(NiColor::WHITE);
        Main::spMenuBGGeometry->AttachProperty(pMatProp);
        pTexProp = new NiTexturingProperty;
        pTexProp->SetBaseTexture(Main::spMenuBGTexture->GetTexture(0));
        pTexProp->SetBaseFilterMode(NiTexturingProperty::FILTER_NEAREST);
        pTexProp->SetBaseClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
        Main::spMenuBGGeometry->AttachProperty(pTexProp);
        Main::spMenuBGGeometry->UpdateProperties();
        NiUpdateData kMenuUpdateData(0.0f, false, false);
        Main::spMenuBGGeometry->Update(kMenuUpdateData);
    }
    TESTerrainLODManager::InitGrass();
}

// One frame of the game loop: the menu and console state, the player, saves, timers and controls, the texture, fader
// and splatter updates, the world clock, the AI lists (on the main thread with one hardware thread, else started on the
// AI threads around the render), the visibility culling, the swap and the post-frame work. Tab+Alt held skips the
// frame. The player's karma alignment is fetched but its result is not used.
void Main::OnIdle() {
    UpdateTextures(fUpdateTexturesOnIdleTime);
    // Returns while Tab and either Alt key are both held, read from SDL's keyboard state.
    const bool* pbKeys = SDL_GetKeyboardState(nullptr);
    if (pbKeys[SDL_SCANCODE_TAB] && (pbKeys[SDL_SCANCODE_LALT] || pbKeys[SDL_SCANCODE_RALT])) {
        return;
    }
    BSSystemUtility::QInstance().Update();
    bInMenuMode = Interface::IsInMenuMode() || Interface::IsPipBoyOpening();
    bInDialog = Interface::InDialog();
    bFaderToBlackVisible = pFaderManager->IsFaderVisible(FADER_TO_BLACK);
    bInConsole = Interface::IsConsoleVisible();
    bool bOtherRenderedMenu = Interface::GetCurrentRenderedMenu() &&
        Interface::GetCurrentRenderedMenu() != Interface::GetPipboy();
    // The mode byte is sign-extended; 3 is the menu-entering mode.
    if (static_cast<int8_t>(InterfaceManager::Instance()->GetCurrentMode()) == 3 && !bOtherRenderedMenu) {
        MemoryLevelManager::RunNonDestructiveFree();
    }
    OnIdle_UpdatePlayer();
    SteamAPICallbacks::GetSingleton()->RunCallbacks();
    OnIdle_UpdateImageSpace(!bInMenuMode && !bFreezeTime);
    bInMenuMode = Interface::IsInMenuMode() || Interface::IsPipBoyOpening();
    bInDialog = Interface::InDialog();
    bFaderToBlackVisible = pFaderManager->IsFaderVisible(FADER_TO_BLACK);
    bInConsole = Interface::IsConsoleVisible();
    if (bprofilePathing) {
        const NiPoint3* pLocation = &pPlayer->GetLocationOnReference();
        TESObjectCELL* pCell = pPlayer->GetParentCell();
        Pathing::ProfilePathing(pCell, *pLocation, 2, 100);
    }
    pSaveLoadManager->UpdateQueuedSaves();
    bhkWorld::ResetBatchProcessed();
    bhkRagdollController::ResetFootCastTimer();
    OnIdle_FixActorBones();
    ForceUpdateTextures();
    SleepManager::RunSleep();
    ++iFrameCounter;
    iHeadTrackingActorUpdateCount = 0;
    if (!bInMenuMode) {
        BSFadeNode::IncFadeCounter();
    }
    OnIdle_UpdateTimer();
    OnIdle_PollControls();
    pIOManager->UpdateQueue();
    if (!StartMenu::CreditsMenuIsActive()) {
        if (StartMenu::MainMenuIsActive()) {
            LoadingMenu::SuspendBackgroundThread();
        }
        else if (Interface::IsInGameLoadingMenuOpen()) {
            TES::pInstance->ShowLoadingMenu(false, nullptr, false);
        }
    }
    OnIdle_ScaleLOD();
    bool bTexturesMenuMode = bInMenuMode;
    NiCamera* pTexturesCamera = WorldRootCamera();
    BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
    pTextureManager->Update(pTexturesCamera, bTexturesMenuMode);
    OnIdle_HandleMenuBackground();
    pFaderManager->UpdateFaders(true);
    if (!bInMenuMode && !bFreezeTime) {
        ScreenSplatter::Update();
        ScreenCustomSplatter::Update();
    }
    if (NiParallelUpdateTaskManager::Get()) {
        NiParallelUpdateTaskManager::BeginUpdate();
    }
    if (!bInMenuMode && !bFreezeTime) {
        if (TES::pInstance->CellTestRunning() && !TES::pInstance->CellTestRunning2()) {
            TES::pInstance->TestAllCells(0);
        }
        BSSceneGraph* pWorldRoot = spWorldRoot;
        float fViewSeconds = appTimer.GetSeconds();
        pWorldRoot->SetViewDistanceBasedOnFrameRate(fViewSeconds);
        appCalendar.Update(appTimer.GetSeconds());
        if (iNumHWThreads.UInt() == 1) {
            TES::pInstance->RunAnimations();
        }
    }
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    ReferenceProcessLists.RunActorScripts();
    if (iNumHWThreads.UInt() != 1) {
        if ((!bInMenuMode || bFaderToBlackVisible) && !bInConsole && !bFreezeTime) {
            ReferenceProcessLists.UpdateRadiationList();
            ReferenceProcessLists.ChangeProcessLevelTempList();
            ReferenceProcessLists.UpdateFollowerTempList();
        }
    }
    else if ((!bInMenuMode || bFaderToBlackVisible) && !bInConsole && !bFreezeTime) {
        ReferenceProcessLists.UpdateRadiationList();
        ReferenceProcessLists.ChangeProcessLevelTempList();
        ReferenceProcessLists.UpdateFollowerTempList();
    }
    ReferenceProcessLists.kUpdateLock.Unlock();
    MemoryLevelManager::QInstance()->CheckMemoryLevel(true);
    if (BSTexturePalette::IsPartiallyPurged()) {
        BSTexturePalette::PurgeUnusedTextures();
    }
    GarbageCollector::Update();
    GarbageCollector::ClearTempEffects();
    bool bTreesPaused = bInMenuMode || bFreezeTime;
    NiCamera* pTreesCamera = WorldRootCamera();
    BSTreeManager::Update(pTreesCamera, bTreesPaused);
    OnIdle_UpdateCurrentGridCell();
    if (iNumHWThreads.UInt() == 1) {
        OnIdle_DoInterfaceIdle();
    }
    ForceUpdateTextures();
    BGSDecalManager::GetSingleton()->UpdateDecals();
    WorldRootCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
    if (pVATS.GetMode() != VATS::VATS_PLAYBACK) {
        float fWorldFOV = pPlayer->GetWorldFOV();
        GetWorldSceneGraph()->SetCameraFOV(fWorldFOV, false, nullptr, true);
    }
    BSShaderManager::SetFOV(pPlayer->GetWorldFOV());
    NiCamera* pSceneCamera = spWorldRoot->GetCamera();
    NiCamera* pCullCamera = nullptr;
    TES::pInstance->ResetAllMultiBoundNodes(true, true);
    NiCamera* pOcclusionCamera = pCullCamera ? pCullCamera : pSceneCamera;
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)
        ->UpdateOcclusionPlaneVisibility(pOcclusionCamera);
    TES::pInstance->UpdateMultiBoundVisibility(pCullCamera ? pCullCamera : pSceneCamera);
    bool bRunThreads = (!bInMenuMode || bFaderToBlackVisible) && !bInConsole && !bFreezeTime;
    // AILinearTaskThreadManager::GetSingleton returns the manager's address as an integer.
    if (bRunThreads) {
        if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
            AILinearTaskThreadManager::SetMainRendering(true);
            AILinearTaskThreadManager::GetSingletonPtr()->StartThreads();
        }
        else {
            ReferenceProcessLists.TaskManagerAI.StartTasksDuringRendering();
        }
    }
    OnIdle_UpdateAnimationsAndEffects();
    if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1 && !bRunThreads) {
        OnIdle_DoInterfaceIdle();
        if (Interface::IsInMenuMode()) {
            Interface::LastMinuteUpdate();
        }
    }
    if (!bFreezeTime) {
        PathManager::QInstance()->Update();
    }
    NavMeshRender::GetInstance()->Update();
    if (iNumHWThreads.UInt() == 1) {
        if (bUseObstacleAvoidance.Bool()) {
            NavMeshObstacleManager::GetInstance()->Update();
        }
        pCombatManager->Update(bInMenuMode || bFaderToBlackVisible || bFreezeTime);
    }
    if (NiParallelUpdateTaskManager::IsActive()) {
        NiParallelUpdateTaskManager::EndUpdate();
    }
    if (Interface::GetTopMenuID() == Interface::eSleepWaitMenu && Interface::UpdateSleeping()) {
        RenderMenuBackground();
    }
    CheckWithinMultiBoundTask::DoAttachments();
    BSShaderManager::ProcessAllQueuedLights();
    int32_t iKarma = static_cast<ActorValueOwner*>(pPlayer)->GetActorValue(ActorValue::KARMA);
    TESActorBaseData::GetAlignmentForKarma(static_cast<float>(iKarma));
    Swap();
    PostSwapProcess();
    if (Renderer::bDoResize) {
        Renderer::DoResize();
        Renderer::bDoResize = false;
    }
    if ((!bInMenuMode || bFaderToBlackVisible) && !bInConsole && !bFreezeTime) {
        if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
            AILinearTaskThreadManager::GetSingletonPtr()->WaitForThreads();
        }
        else {
            ReferenceProcessLists.TaskManagerAI.WaitForTasksDuringRendering();
        }
    }
    UpdateNonRenderSafeAITasks();
    OnIdle_PostThreadsProcess();
    if (GetOpenConsole()) {
        SetOpenConsole(false);
        Interface::OpenConsole();
    }
    Script::ClearOptimizations();
    ScriptLocals::ClearOptimizations();
    fSortFreeBlocksTimer = appTimer.GetSeconds() + fSortFreeBlocksTimer;
    if (Interface::IsInMenuMode() || (pPlayer && pPlayer->IsVanityMode()) || fSortFreeBlocksTimer >= 45.0) {
        fSortFreeBlocksTimer = 0.0f;
        int32_t iIndex = ucSortFreeBlocksIndex;
        ucSortFreeBlocksIndex = ucSortFreeBlocksIndex + 1;
        MemoryPool::SortFreeBlocks(iIndex);
        // Compares the zero-extended byte with 256, which never matches.
        if (static_cast<uint32_t>(ucSortFreeBlocksIndex) == 0x100u) {
            ucSortFreeBlocksIndex = 0;
        }
    }
}

void Main::OnIdle_ScaleLOD() {
    if (GetSceneIsComplex()) {
        BSFadeNode::SetLODFadeOutObjectMult(fLODFadeOutObjectMultComplex.Float());
        BSFadeNode::SetLODFadeOutItemMult(fLODFadeOutItemMultComplex.Float());
        BSFadeNode::SetLODFadeOutActorMult(fLODFadeOutActorMultComplex.Float());
    }
    else if (
        TES::pInstance->GetWorldSpace() && TES::pInstance->GetWorldSpace()->GetParentWorld(TESWorldSpace::PU_LOD)
    ) {
        BSFadeNode::SetLODFadeOutObjectMult(fLODFadeOutObjectMultCity.Float());
        BSFadeNode::SetLODFadeOutItemMult(fLODFadeOutItemMultCity.Float());
        BSFadeNode::SetLODFadeOutActorMult(fLODFadeOutActorMultCity.Float());
    }
    else if (pPlayer && pPlayer->GetParentCell() && pPlayer->GetParentCell()->GetInterior()) {
        BSFadeNode::SetLODFadeOutObjectMult(fLODFadeOutObjectMultInterior.Float());
        BSFadeNode::SetLODFadeOutItemMult(fLODFadeOutItemMultInterior.Float());
        BSFadeNode::SetLODFadeOutActorMult(fLODFadeOutActorMultInterior.Float());
    }
    else {
        BSFadeNode::SetLODFadeOutObjectMult(1.0f);
        BSFadeNode::SetLODFadeOutItemMult(1.0f);
        BSFadeNode::SetLODFadeOutActorMult(1.0f);
    }
}

// bfixNextTime is the function's static. Calls MiddleHighProcess::ForceBoneScale (empty in this build) on every high
// process with 3D and on the player's process.
void Main::OnIdle_FixActorBones() {
    if (!bInMenuMode) {
        bfixNextTime = true;
        return;
    }
    if (bfixNextTime) {
        ProcessArray* pMobArray = ReferenceProcessLists.GetProcessArray();
        for (uint32_t i = 0; i < pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
            MobileObject* pMob = pMobArray->GetItem(i);
            if (pMob && pMob->GetCurrentProcessType() == PROCESS_TYPE_HIGH && pMob->Get3D()) {
                static_cast<MiddleHighProcess*>(pMob->GetCurrentAIProcess())->ForceBoneScale();
            }
        }
        static_cast<MiddleHighProcess*>(pPlayer->GetCurrentAIProcess())->ForceBoneScale();
    }
    bfixNextTime = false;
}

void Main::OnIdle_UpdateTimer() {
    appTimer.SetUseGlobalTimeMultiplierTarget(bChangeTimeMultSlowly.Bool());
    if (!bInMenuMode) {
        BSTimer::UpdateGlobalTimeMultiplier();
    }
    appTimer.Update();
    bhkWorld::fTauRatio = fHavokTauRatio.Float();
    // The arguments are evaluated right to left.
    bool bSimulateEveryFrame = pVATS.GetMode() == VATS::VATS_PLAYBACK;
    bool bQuickSimulate = GetSceneIsComplex();
    float fDeltaTime = appTimer.GetSeconds();
    bhkWorld::SetDeltaTime(fDeltaTime, bQuickSimulate, bSimulateEveryFrame);
    BSShaderManager::UpdateTimers();
    double fAnimTime;
    if (bInMenuMode) {
        fAnimTime = 0.0;
    }
    else {
        double fSeconds = appTimer.GetSeconds();
        fAnimTime = fAnimationMult.Float() * fSeconds;
    }
    fFrameAnimTime = static_cast<float>(fAnimTime);
}

void Main::OnIdle_PollControls() {
    Controls::GetInstance()->UpdateValues();
    float fTime = appTimer.GetRealTimeSeconds();
    Controls::GetInstance()->Poll(fTime);
    if (!bFlyCamera &&
        !pPlayer->IsDead(false) &&
        ((pVATS.GetMode() == VATS::VATS_PLAYBACK && !pPlayer->IsInSlowMo()) || Interface::IsPipBoyOpening()) &&
        !Interface::IsMenuIDVisible(Interface::eMessageMenu, 0) &&
        !StartMenu::PauseMenuIsActive()) {
        Controls::GetInstance()->ClearUserActions();
    }
}

void Main::OnIdle_HandleMenuBackground() {
    if (!Main::bInMenuMode ||
        Interface::IsTopMenuID(Interface::eDialogMenu) ||
        StartMenu::MainMenuIsActive() ||
        Interface::IsMenuIDVisible(Interface::eVATSMenu, Interface::FADE_NOT_HIDDEN) ||
        Interface::IsMenuIDVisible(Interface::eMessageMenu, Interface::FADE_NOT_HIDDEN) ||
        Interface::IsInPipboyMenu() ||
        Interface::IsConsoleVisible() ||
        Main::bMenuBGReady ||
        !Main::bStaticMenuBG) {
        if (Main::bMenuBGReady &&
            (!Main::bInMenuMode ||
             StartMenu::MainMenuIsActive() ||
             (Interface::IsConsoleVisible() && !Interface::IsInRenderedMenu()) ||
             Interface::IsTopMenuID(Interface::eDialogMenu) ||
             Interface::IsTopMenuID(Interface::eVATSMenu)) &&
            !pFaderManager->IsFaderActive(FADER_TO_BLACK) &&
            !Interface::IsMenuIDVisible(Interface::eSurgeryMenu, 0) &&
            !Interface::IsMenuIDVisible(Interface::eLockPickMenu, 0) &&
            !Interface::IsMenuIDVisible(Interface::eSPECIALBookMenu, 0) &&
            !Interface::IsMenuIDVisible(Interface::eLoveTesterMenu, 0) &&
            !Interface::IsMenuIDVisible(Interface::eSlotMachineMenu, 0) &&
            !Interface::IsMenuIDVisible(Interface::eBlackJackMenu, 0) &&
            !Interface::IsMenuIDVisible(Interface::eRouletteMenu, 0) &&
            !Interface::IsMenuIDVisible(Interface::eCaravanMenu, 0)) {
            KillMenuBGTexture();
        }
    }
    else {
        RenderMenuBackground();
    }
}

void Main::OnIdle_UpdateAudio() {
    BSAudio::QInstance().Update(true);
    FalloutRadio::Update(false);
    FalloutAudioMedia::Update();
    FalloutAudio::UpdateRegionSounds();
}

void Main::RunScripts() {
    bhkAvoidBox::bNoAvoidance = false;
    Actor::bAllPointsVisible = false;
    bSceneIsComplex = false;
    Script::ClearCachedValues();
    TES::pInstance->RunScripts();
}

void Main::UpdateNonRenderSafeAITasks() {
    if (Interface::InDialog() && !Interface::IsConsoleVisible() && !DialogMenu::GetPaused()) {
        TESObjectREFR* pSpeaker = DialogMenu::GetDialogSpeaker();
        if (pSpeaker && pSpeaker->IsActor()) {
            static_cast<Actor*>(pSpeaker)->UpdateNonRenderSafeDialogueUpdate(0.0f);
        }
    }
    ReferenceProcessLists.UpdateHighListImpacts(0.0f);
    if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
        ReferenceProcessLists.ParallelUpdateAnimationScenegraph();
    }
    else {
        ReferenceProcessLists.UpdateAnimationScenegraph();
    }
    ReferenceProcessLists.ReconcileDetectionDataForActor();
    ReferenceProcessLists.UpdateNonRenderSafeStuff(appTimer.GetSeconds());
    FOPipboyManager* pPipboy = Interface::GetPipboy();
    if (pPipboy->GetPipBoyManagerReset()) {
        pPipboy->ResetPipBoyManager();
    }
    if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
        TESObjectCELL::PerformQueuedChildAttaches();
        TaskQueueInterface::GetInstance()->ThreadEndInput();
        TaskQueueInterface::GetInstance()->ProcessQueue();
        TESObjectCELL::RunQueuedParticleUpdates();
        BGSDestructibleObjectForm::SetLastUpdateTime(fCellAppTime);
        TaskQueueInterface::GetInstance()->ThreadBeginInput();
    }
    Interface::SetPickLength(iActivatePickLength.UInt());
    Interface::RefreshActivateREFR();
    pCombatManager->UpdateMainThread(bInMenuMode || bFaderToBlackVisible || bFreezeTime);
}

void Main::OnIdle_UpdateProcessLists() {
    if (iNumHWThreads.UInt() == 1) {
        ReferenceProcessLists.ChangeProcessLevelTempList();
    }
    if ((!bInMenuMode || bFaderToBlackVisible) && !bInConsole && !bFreezeTime) {
        if (iNumHWThreads.UInt() == 1) {
            ReferenceProcessLists.TaskManagerAI.UpdateTasks();
            pCombatManager->UpdateCombatants();
        }
        ReferenceProcessLists.UpdateProcessLists();
    }
    else if (bInMenuMode || bFreezeTime) {
        pPlayer->UpdateAutoAimActor();
    }
}

void Main::OnIdle_UpdatePlayer() {
    if (pPlayer->HandlePositionPlayerRequest()) {
        return;
    }
    if (bInMenuMode && !Interface::IsPipBoyOpening()) {
        pPlayer->ForceGrenadeHold();
        return;
    }
    if (bFlyCamera) {
        if (pPlayer->Get3D()) {
            pPlayer->UpdateFlyCamera(appTimer.GetSeconds(), bFreezeTime);
        }
        return;
    }
    if (pPlayer->Get3D()) {
        ++cSkipThreadQueue;
        double fSeconds = appTimer.GetSeconds();
        float fDelta = pVATS.GetPlayerUpdateMult() * fSeconds;
        pPlayer->Update(fDelta);
        --cSkipThreadQueue;
    }
    TESObjectCELL* pCell = pPlayer->GetParentCell();
    NiPoint3 kPos = *pPlayer->QLocationOnReference();
    if (!pCell || pCell->GetInterior() || pCell->IsPointInCellExterior(kPos)) {
        return;
    }
    TESObjectCELL* pNewCell = pDataHandler->GetCellFromWorldCoord(kPos.x, kPos.y, pCell->GetWorldSpace(), true);
    if (!pNewCell) {
        return;
    }
    if (!pNewCell->IsLoaded() && !pNewCell->GetAttached()) {
        TES::pInstance->UpdateCurrentGridCell(&kPos, true);
        if (TES::pInstance->CellTestRunning()) {
            TES::pInstance->ShowLoadingMenu(false, nullptr, false);
        }
    }
    FalloutAudio::DisableRegionSoundUpdates(true);
    pPlayer->SetAcousticSpaceTransition(true);
    pNewCell->AddReference(pPlayer, false);
    FalloutAudio::SetNextAcousticSpace(pNewCell->GetAcousticSpace());
    pPlayer->SetAcousticSpaceTransition(false);
    FalloutAudio::DisableRegionSoundUpdates(false);
    BSShaderAccumulator* pAccumulator = BSShaderManager::GetAccumulator();
    if (pAccumulator) {
        pAccumulator->ClearAllBoundVolumes();
    }
}

// The player's position is read through GetLocationOnReference called non-virtually. The results of the two world space
// lookups are unused.
void Main::OnIdle_UpdateCurrentGridCell() {
    if (!bInMenuMode && !Interface::bIsLoadingNewGame && !bFreezeTime) {
        TES::pInstance->UpdateCurrentGridCell(pPlayer->QLocationOnReference(), true);
        if (TES::pInstance->CellTestRunning() && !TES::pInstance->GetInterior()) {
            pPlayer->GetWorldSpace();
            TES::pInstance->GetWorldSpace();
        }
    }
}

void Main::OnIdle_UpdateAnimationsAndEffects() {
    if ((!bInMenuMode || bInDialog || bFaderToBlackVisible) && !bFreezeTime) {
        if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
            ReferenceProcessLists.UpdateTempEffectsParallel(appTimer.GetSeconds());
        }
        else {
            ReferenceProcessLists.UpdateTempEffects(appTimer.GetSeconds());
        }
        if (iNumHWThreads.UInt() == 1 && !bInMenuMode) {
            TES::pInstance->UpdateCellAnimations(fFrameAnimTime);
        }
        else {
            TES::pInstance->UpdateCellMainThread();
        }
        NiUpdateData kUpdateData(fFrameAnimTime, true, false);
        if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
            GetMasterParticleSystemManager()->UpdateParallel(kUpdateData);
        }
        else {
            GetMasterParticleSystemManager()->UpdateParallel(kUpdateData);
        }
    }
}

void Main::OnIdle_DoInterfaceIdle() {
    Interface::PreIdleStuff();
    Interface::Idle();
    Interface::PostIdleStuff();
}

void Main::OnIdle_UpdateImageSpace(bool abAdvanceTime) {
    BSShaderManager::GetImageSpaceManager()->ResetToBaseParameterData();
    BSShaderManager::GetImageSpaceManager()->BeginParamModifiers();
    if (abAdvanceTime) {
        float fAmount = appTimer.fDelta;
        TESImageSpaceModifier::AdvanceCurrentTime(fAmount);
    }
    BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* pListModifiers = &TES::pInstance
                                                                               ->listActiveImageSpaceModifiers;
    int32_t iWeatherCount = 0;
    int32_t iActiveWeatherCount = 0;
    while (pListModifiers) {
        ImageSpaceModifierInstance* pImageSpaceModInstance = pListModifiers->m_item.m_pObject;
        if (pImageSpaceModInstance) {
            if (pImageSpaceModInstance->bIsWeather) {
                ++iWeatherCount;
            }
            if (pImageSpaceModInstance->IsExpired()) {
                NiPointer<ImageSpaceModifierInstance> spInstance(pImageSpaceModInstance);
                pListModifiers->Remove(spInstance);
                continue;
            }
            if (TESImageSpaceModifier::GetEnable()) {
                pImageSpaceModInstance->Apply();
                if (pImageSpaceModInstance->fStrength > 0.0 && pImageSpaceModInstance->bIsWeather) {
                    ++iActiveWeatherCount;
                }
            }
        }
        pListModifiers = pListModifiers->m_pkNext;
        if (iWeatherCount == 4) {
            if (!iActiveWeatherCount) {
                BSShaderManager::GetImageSpaceManager()->ResetWeatherData();
            }
            BSShaderManager::GetImageSpaceManager()->EndWeatherModifiers();
            iWeatherCount = 0;
        }
    }
    BSShaderManager::GetImageSpaceManager()->EndParamModifiers();
    BSShaderManager::GetImageSpaceManager()->ApplyCurrentParameterData(false);
}

// The debug texture check tests a shared stub that always returns true (ReturnTrue), comparing its byte with 1.
void Main::Swap() {
    char acPathName[268];
    NiRenderer* pRenderer = NiRenderer::GetRenderer();
    static_cast<NiDX9Renderer*>(pRenderer)->NiDX9Renderer::LockRenderer();
    if (Interface::bStartPipboyRenderBackground) {
        Interface::bStartPipboyRenderBackground = false;
        bool b1Cull = pPlayer->Get3D(true)->GetAppCulled();
        bool b3Cull = pPlayer->Get3D(false)->GetAppCulled();
        pPlayer->Get3D(true)->SetAppCulled(true);
        pPlayer->Get3D(false)->SetAppCulled(true);
        RenderMenuBackground();
        pPlayer->Get3D(true)->SetAppCulled(b1Cull);
        pPlayer->Get3D(false)->SetAppCulled(b3Cull);
    }
    NiRenderer::GetRenderer()->GetSorter();

    InitializationFlow();
    MTRenderingSystem::QInstance()->StartThreads();
    if (Renderer::bGammaDirty) {
        Renderer::UpdateGamma();
    }
    if (pFaderManager->IsFaderVisible(FADER_TO_BLACK) || pFaderManager->IsFaderVisible(FADER_TO_WHITE)) {
        if (!iBlackFaderFrames) {
            TES::pInstance->ShowLoadingMenu(false, nullptr, false);
            InitializationFlow();
            ReferenceProcessLists.SkipHighListFadeIn();
        }
        fBlackFaderTime = appTimer.GetRealTimeSeconds() + fBlackFaderTime;
        ++iBlackFaderFrames;
        if (fBlackFaderTime >= 1.0 && iBlackFaderFrames >= 10) {
            pFaderManager->RemoveFader(FADER_TO_BLACK, false);
            pFaderManager->RemoveFader(FADER_TO_WHITE, false);
            pSaveLoadGameOLD->SetLoading(false);
            if (pSaveLoadGameOLD->GetLoading()) {
                pSaveLoadGameOLD->SetLoading(false);
            }
            KillMenuBGTexture();
        }
    }
    else {
        fBlackFaderTime = 0.0f;
        iBlackFaderFrames = 0;
    }
    BSRenderedTexture* p3DInterfaceTex = nullptr;
    if (Main::bMenuBGReady ||
        StartMenu::MainMenuIsActive() ||
        !pPlayer->Get3D() ||
        pFaderManager->IsFadingIn(FADER_TO_BLACK)) {
        if ((!Interface::IsInRenderedMenu() && Interface::IsInMenuMode()) ||
            Interface::IsMenuIDVisible(Interface::eStartMenu, 0)) {
            for (int32_t i = 1; i < 23; ++i) {
                MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, i);
                MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, i);
            }
            UpdateOffscreenInterface();
            if (Interface::IsMenuIDVisible(Interface::eSurgeryMenu, 0) ||
                Interface::IsMenuIDVisible(Interface::eLockPickMenu, 0) ||
                Interface::IsMenuIDVisible(Interface::eSPECIALBookMenu, 0) ||
                Interface::IsMenuIDVisible(Interface::eLoveTesterMenu, 0) ||
                Interface::IsMenuIDVisible(Interface::eSlotMachineMenu, 0) ||
                Interface::IsMenuIDVisible(Interface::eBlackJackMenu, 0) ||
                Interface::IsMenuIDVisible(Interface::eRouletteMenu, 0) ||
                Interface::IsMenuIDVisible(Interface::eCaravanMenu, 0)) {
                DrawWorldMenu3D(nullptr);
            }
            else if (Main::spMenuBGTexture.m_pObject) {
                bool bMultiSampleEnabled = Renderer::GetMultiSampleEnabled();
                DrawWorld_CopyMenuBG(
                    BSShaderManager::GetImageSpaceManager(),
                    static_cast<NiDX9Renderer*>(pRenderer),
                    bMultiSampleEnabled,
                    nullptr
                );
            }
        }
        else if (pPlayer->Get3D()) {
            DrawWorld(nullptr, Interface::IsInRenderedMenu(), false);
        }
    }
    else {
        if (!TES::pInstance->pInteriorCell &&
            *TES::pInstance->pWorldSpace->GetCanopyShadowTextureName() &&
            TES::pInstance->pGridCellA &&
            GridCellArray::GetRecreateCanopyShadowMaskTexture()) {
            TES::pInstance->pGridCellA->RecreateCanopyShadowMaskTexture();
        }

        DrawWorld(nullptr, false, false);
    }
    if (ReturnTrue()) {
        BSShaderManager::RenderDebugTexture();
    }
    Interface::DrawHUDMenu3D();
    Interface::Click(p3DInterfaceTex, Main::spInterfaceFXTex.m_pObject);
    MTRenderingSystem::QInstance()->WaitForThreads();
    if (Main::spInterfaceFXTex.m_pObject) {
        BSRenderedTexture* pTexture = Main::spInterfaceFXTex.m_pObject;
        BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
        pTextureManager->ReturnRenderedTexture(pTexture);
        Main::spInterfaceFXTex = static_cast<BSRenderedTexture*>(nullptr);
    }
    if (Controls::GetInstance()->GetUserAction(Controls::UACTN_SCREENSHOT, 1)) {
        if (Controls::GetInstance()->GetKeyboardButton(157, 0) || bMultiScreenshotQueued) {
            bMultiScreenshotQueued = !bMultiScreenshotQueued;
            if (bMultiScreenshotQueued) {
                iIdleSectionCount = 0;
                iCameraPathTake = iCameraPathTake.Int() + 1;
                BSsprintf(acPathName, 260, "%s%03d", sCameraPathDir.String(), iCameraPathTake.Int());
                std::error_code kError;
                std::filesystem::create_directory(platform::NativePath(acPathName), kError);
                appTimer.ClampAt(static_cast<uint32_t>(iCameraPathFPS.Int()));
            }
            else {
                appTimer.ClampAt(0);
            }
        }
        else {
            ScreenShot::TakeScreenShot(nullptr);
        }
    }
    NiObjectGarbage::Update();
    if (pRenderer->GetFrameState()) {
        BSRenderedTexture::Stop();
    }
    if (p3DInterfaceTex) {
        BSRenderedTexture* pTexture = p3DInterfaceTex;
        BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
        pTextureManager->ReturnRenderedTexture(pTexture);
        p3DInterfaceTex = nullptr;
    }
    Main::spWorldRoot->SetCameraScene();
    static_cast<NiDX9Renderer*>(pRenderer)->NiDX9Renderer::UnlockRenderer();
    if (iNumHWThreads.Int() > 1) {
        AILinearTaskThreadManager::SetMainRendering(false);
    }
}

void Main::PostSwapProcess() {
    OnIdle_UpdateAudio();
    OnIdle_UpdateProcessLists();
    if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
        TESObjectCELL::RunQueuedParticleUpdates();
    }
    RunScripts();
}

void Main::OnIdle_PostThreadsProcess() {
    Script::RunDelayedScriptActionsOnReferences();
    if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
        TaskQueueInterface::GetInstance()->ThreadEndSecondaryInput();
        TaskQueueInterface::GetInstance()->ProcessSecondaryQueue();
        TaskQueueInterface::GetInstance()->ThreadBeginSecondaryInput();
    }
    Interface::HandleQueuedMenuOpen();
    InterfaceManager::Instance()->UpdateQueuedMenuClosings();
    DebugText::Instance(true)->HandleQueuedPrints();
}

void Main::DrawWorld(BSRenderedTexture* apDestination, bool abRenderedMenuMode, bool abSkipFirstPerson) {
    bool bWireframe = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->QWireframe();
    // The by-value argument is built before the scene node is fetched.
    NiPoint3 kEyePosition = GetWorldSceneGraph()->GetCamera()->GetWorldTranslate();
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->SetEyePosition(kEyePosition);
    if (abRenderedMenuMode && !Interface::IsMenuIDVisible(Interface::eStartMenu, 0)) {
        DrawWorldRenderedMenu(apDestination, bWireframe);
    }
    else if (bWireframe) {
        DrawWorldWireframe(apDestination, abSkipFirstPerson);
    }
    else {
        DrawWorldStandard(apDestination, abSkipFirstPerson);
    }
}

void Main::DrawWorldRenderedMenu(BSRenderedTexture* apDestination, bool bwireframe) {
    NiGeometry* pRenderedMenuGeom;
    ImageSpaceManager* pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    bool bMultiSampleEnabled = Renderer::GetMultiSampleEnabled();
    if (Interface::GetCurrentRenderedMenu()) {
        pRenderedMenuGeom = Interface::GetCurrentRenderedMenu()->GetScreenGeometry();
    }
    else {
        pRenderedMenuGeom = nullptr;
    }
    DrawWorld_WaitForParallelUpdate();
    for (int32_t i = 1; i < 17; ++i) {
        MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, i);
        MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, i);
    }
    DrawWorld_UpdateOffscreenBuffers();
    BSRenderedTexture*
        pRenderTexture = DrawWorld_InitRenderTexture(pRenderer, apDestination, bwireframe, bMultiSampleEnabled);
    DrawWorld_CopyMenuBG(pImageSpaceManager, pRenderer, bMultiSampleEnabled, apDestination);
    if (iNumHWThreads.Int() > 1) {
        AILinearTaskThreadManager::SetMainRendering(false);
    }
    MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 21);
    MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 21);
    if (Interface::IsInPipboyMenu()) {
        NiCamera* pCamera = Main::WorldRootCamera();
        BSCullingProcess* pCuller = Main::GetWorldSceneGraph()->pCuller;
        pCuller->SetAccumulator(spDraw1stPersonAccum.m_pObject);
        pCuller->m_pkCamera = pCamera;
        BSShaderManager::GetShadowSceneNode(0)->PreOnVisible(*pCuller);
        DrawWorld_Setup1stPerson();
        DrawWorld_MTRender1stPerson(pRenderer, pRenderedMenuGeom, nullptr, pRenderTexture);
    }
    else {
        MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 22);
        MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 22);
    }
    DrawWorld_RenderedMenuScene();
    if (!BSShaderManager::GetImageSpaceEffects() || bwireframe) {
        if (apDestination) {
            DrawWorld_StopRenderTarget(bMultiSampleEnabled, apDestination);
        }
    }
    else {
        DrawWorld_ImageSpace(bMultiSampleEnabled, pRenderTexture, pRenderer, apDestination);
        if (pRenderedMenuGeom) {
            DrawWorld_RenderedMenu(pRenderedMenuGeom, pRenderer);
        }
    }
    DrawWorld_End(pRenderTexture, pImageSpaceManager, pRenderer, nullptr);
}

void Main::DrawWorldWireframe(BSRenderedTexture* apDestination, bool abSkipFirstPerson) {
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    ImageSpaceManager* pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
    bool b1stPersonCulled = pPlayer->Get3D(true)->GetAppCulled();
    bool bMultiSampleEnabled = Renderer::GetMultiSampleEnabled();
    BSRenderState::SetWireframeEnable(true, BSRS_LOCK);
    DrawWorld_WaitForParallelUpdate();
    Sun* pSun = DrawWorld_Init();
    DrawWorld_UpdateWater();
    bool bIssuesVATSQuery = DrawWorld_VATSOcclusionQuery();
    DrawWorld_UpdateOffscreenBuffers();
    BSRenderedTexture*
        pRenderTexture = DrawWorld_InitRenderTexture(pRenderer, apDestination, false, bMultiSampleEnabled);
    bool bDo1stPerson = !b1stPersonCulled &&
        !abSkipFirstPerson &&
        !Interface::GetVATSEffect()->GetTarget() &&
        !DialogMenu::IsDialogActive();
    DrawWorld_DrawScene(pSun, bDo1stPerson, true, bIssuesVATSQuery);
    if (bDo1stPerson) {
        DrawWorld_Setup1stPerson();
        DrawWorld_RestartRenderTexture(pRenderTexture, bMultiSampleEnabled, 0);
        DrawWorld_MTRender1stPerson(pRenderer, nullptr, pSun, pRenderTexture);
    }
    else {
        MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 22);
        MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 22);
    }
    if (apDestination) {
        DrawWorld_StopRenderTarget(bMultiSampleEnabled, apDestination);
    }
    BSRenderState::RestoreWireframeEnable(BSRS_LOCK);
    if (BSShaderManager::GetImageSpaceEffects()) {
        DrawWorld_ImageSpace(bMultiSampleEnabled, pRenderTexture, pRenderer, apDestination);
    }
    else if (apDestination) {
        DrawWorld_StopRenderTarget(bMultiSampleEnabled, apDestination);
    }
    DrawWorld_End(pRenderTexture, pImageSpaceManager, pRenderer, nullptr);
}

void Main::DrawWorldStandard(BSRenderedTexture* apDestination, bool abSkipFirstPerson) {
    NiPoint2 kSunScreenPos;
    NiRect<float> pkR;
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    ImageSpaceManager* pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
    bool b1stPersonCulled = pPlayer->Get3D(true)->GetAppCulled();
    bool bMultiSampleEnabled = Renderer::GetMultiSampleEnabled();
    DrawWorld_WaitForParallelUpdate();

    Sun* psun = DrawWorld_Init();
    DrawWorld_UpdateWater();
    bool bIssuesVATSQuery = DrawWorld_VATSOcclusionQuery();
    DrawWorld_UpdateOffscreenBuffers();
    BSRenderedTexture*
        pRenderTexture = DrawWorld_InitRenderTexture(pRenderer, apDestination, false, bMultiSampleEnabled);
    DrawWorld_VATS(bIssuesVATSQuery);
    DrawWorld_RestartRenderTexture(pRenderTexture, bMultiSampleEnabled, 5u);
    bool bDo1stPerson = !b1stPersonCulled &&
        !abSkipFirstPerson &&
        !Interface::GetVATSEffect()->GetTarget() &&
        !DialogMenu::IsDialogActive();
    bool bGlareOcclude = false;
    if (bDo1stPerson && psun) {
        bGlareOcclude = psun->GetSunGlareNode()->GetAppCulled();
        psun->GetSunGlareNode()->SetAppCulled(true);
    }

    bool bUseScopeScissoring = !IsEyefinity();
    bool bGunScope = Interface::IsGunScopeVisible();
    if (bGunScope) {
        uint32_t uiW = pRenderer->NiRenderer::GetScreenWidth();
        uint32_t uiH = pRenderer->NiRenderer::GetScreenHeight();
        float fHalfAmt = Interface::GetAspectRatioRatio() * fScopeScissorAmount.Float() * 0.5;
        float fX0 = static_cast<double>(uiW) * fHalfAmt;
        float fX1 = static_cast<double>(uiW) - fX0;
        if (bUseScopeScissoring) {
            BSShaderManager::EnableFullScreenScissor(static_cast<int64_t>(fX0), 0, static_cast<int64_t>(fX1), uiH);
        }
    }
    DrawWorld_DrawScene(psun, bDo1stPerson, false, bIssuesVATSQuery);
    if (bDo1stPerson && psun) {
        psun->GetSunGlareNode()->SetAppCulled(bGlareOcclude);
    }

    ImageSpaceEffectDepthOfField* pDOFEffect = static_cast<ImageSpaceEffectDepthOfField*>(
        pImageSpaceManager->GetEffect(ImageSpaceManager::EFFECT_DEPTH_OF_FIELD)
    );
    bool bIsDepthOfFieldActive = pDOFEffect->DoDepth();
    bool bSunbeamsActive = pImageSpaceManager->IsEffectActive(ImageSpaceManager::EFFECT_SUNBEAMS) &&
        !TES::pInstance->pInteriorCell &&
        bUseSunbeams.Bool();
    bool b1stPersonWait = false;
    if (bIsDepthOfFieldActive && bDo1stPerson) {
        b1stPersonWait = true;
        DrawWorld_Add1stPersonToDepthBuffer(pRenderTexture);
        DrawWorld_RestartRenderTexture(pRenderTexture, bMultiSampleEnabled, 0);
    }
    if (BSShaderManager::IsRefractionEnabled() && spDrawWorldAccum->GetRefractivePassCount()) {
        DrawWorld_Refraction(spDrawWorldAccum.m_pObject, pRenderTexture);
    }

    if (bDo1stPerson) {
        DrawWorld_Setup1stPerson();
        DrawWorld_RestartRenderTexture(pRenderTexture, bMultiSampleEnabled, 0);
        if (!b1stPersonWait) {
            MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 22);
        }
        DrawWorld_MTRender1stPerson(pRenderer, nullptr, psun, pRenderTexture);
    }
    else {
        MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 22);
        MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 22);
    }
    if (bIsDepthOfFieldActive) {
        DrawWorld_DoDepth(spDrawWorldAccum.m_pObject, pRenderTexture);
    }
    if (bSunbeamsActive && !TES::pInstance->pInteriorCell && Main::pSky && Main::pSky->pSun) {
        Sun* pSun = Main::pSky->pSun;
        NiPoint3 kSunPos = pSun->GetRoot()->GetWorldTranslate();

        float fSunZ = 0.0f;
        float fSunY = 0.0f;
        float fSunX = 0.0f;
        Main::GetWorldSceneGraph()->GetCamera()->WorldPtToScreenPt3(kSunPos, fSunX, fSunY, fSunZ, 1e-05f);
        ::new (&kSunScreenPos) NiPoint2(fSunX, fSunY);
        BSShaderManager::GetImageSpaceManager()->SetSunPosition(kSunScreenPos);
    }

    if (BSShaderManager::GetImageSpaceEffects()) {
        DrawWorld_ImageSpace(bMultiSampleEnabled, pRenderTexture, pRenderer, apDestination);
        if ((ScreenCustomSplatter::IsActiveSplatter() || ScreenSplatter::IsActiveSplatter()) &&
            !Interface::IsInPipboyMenu()) {
            DrawWorld_ScreenSplatter(pRenderer);
        }
    }
    else if (apDestination) {
        DrawWorld_StopRenderTarget(bMultiSampleEnabled, apDestination);
    }
    if (bGunScope) {
        if (bUseScopeScissoring) {
            BSShaderManager::DisableFullScreenScissor();
        }
        if (IsEyefinity()) {
            NiRenderer::GetRenderer()->SetBackgroundColor(NiColorA::BLACK);
            ::new (&pkR) NiColorA(0.0f, 0.0f, 0.0f, 0.0f);
            pkR.m_left = 0.0f;
            pkR.m_right = 0.33f;
            pkR.m_top = 1.0f;
            pkR.m_bottom = 0.0f;
            NiRenderer::GetRenderer()->ClearBuffer(&pkR, 1u);
            pkR.m_left = 0.66f;
            pkR.m_right = 1.0f;
            pkR.m_top = 1.0f;
            pkR.m_bottom = 0.0f;
            NiRenderer::GetRenderer()->ClearBuffer(&pkR, 1u);
        }
    }
    DrawWorld_End(pRenderTexture, pImageSpaceManager, pRenderer, psun);
}

void Main::DrawWorld_CopyMenuBG(
    ImageSpaceManager* apImagespaceManager,
    NiDX9Renderer* apRenderer,
    bool abMultiSampleEnabled,
    BSRenderedTexture* apDestination
) {
    BSRenderedTexture* pMenuBGTexture = Main::spMenuBGTexture;
    if (pMenuBGTexture) {
        BSRenderState::SetColorWriteEnable(7u, BSRS_LOCK);
        BSRenderState::SetAlphaBlendEnable(false, BSRS_LOCK);
        pMenuBGTexture = Main::spMenuBGTexture;
        apImagespaceManager->RenderEffect(
            ImageSpaceManager::EFFECT_SHADER_COPY,
            apRenderer,
            pMenuBGTexture,
            !abMultiSampleEnabled ? apDestination : nullptr,
            nullptr,
            1
        );
        BSRenderState::RestoreAlphaBlendEnable(BSRS_LOCK);
        BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
    }
}

void Main::DrawWorld_StopRenderTarget(bool abMultiSampleEnabled, BSRenderedTexture* apDestination) {
    BSRenderedTexture::StopOffscreen();
}

void Main::DrawWorld_End(
    BSRenderedTexture* apRenderTexture,
    ImageSpaceManager* apImagespaceManager,
    NiDX9Renderer* apRenderer,
    Sun* apSun
) {
    SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
    if (apSun) {
        apSun->StopSunOcclusion();
    }
    BSShaderProperty::PreviousTransform::SaveCamera(pSceneGraph->GetCamera());
    if (apRenderTexture) {
        BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(apRenderTexture);
    }
    spDrawWorldAccum->ClearRenderedGeometryCache();
    spScreenSplatterAccum->ClearBloodSplatter();
    BSShaderManager::ReturnSceneDepthTexture();
    BSShaderManager::ReturnVelocityBuffers();
    ImageSpaceEffectDepthOfField* pDOFEffect = static_cast<ImageSpaceEffectDepthOfField*>(
        apImagespaceManager->GetEffect(ImageSpaceManager::EFFECT_DEPTH_OF_FIELD)
    );
    pDOFEffect->ReturnMaskTexture();
    if (BSShaderManager::bSLIMode) {
        // The HDR effect's Shutdown is called with no object: its body ignores this.
        ImageSpaceEffectHDR::ClearAdaptedLight();
    }
    IDirect3DDevice9* pD3DDevice = BSShaderManager::GetRenderer()->GetD3DDevice();
    pD3DDevice->SetIndices(nullptr);
    pD3DDevice->SetStreamSource(0, nullptr, 0, 0);
    if (NiParallelUpdateTaskManager::Get()) {
        NiParallelUpdateTaskManager::SignalRenderSema();
    }
    if (BSShaderAccumulator::spRenderPassesScenegraph.m_pObject) {
        new NiTreeCtrl(
            hInstance,
            hWnd,
            BSShaderAccumulator::spRenderPassesScenegraph.m_pObject,
            "Render Passes",
            0x80000000u,
            0x80000000u,
            800,
            600
        );
        BSShaderAccumulator::spRenderPassesScenegraph = static_cast<NiNode*>(nullptr);
    }
    BSBatchRenderer::EndPass();
}

void Main::DrawWorld_UpdateOffscreenBuffers() {
    if (BSShaderManager::GetShadows() && Main::GetSceneIsComplex()) {
        bSavedShadowState = BSShaderManager::GetShadows();
        BSShaderManager::EnableShadows(false);
        ShadowSceneNode* pSSNode = BSShaderManager::GetShadowSceneNode(0);
        pSSNode->ReleaseAllShadowMaps();
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        BSShaderManager::TextureManagerPtr()->SetFrustumShadowTextureCount(pRenderer, 0);
    }
    else if (bSavedShadowState && !Main::GetSceneIsComplex()) {
        int32_t iMaxActorShadows;
        bSavedShadowState = false;
        BSShaderManager::EnableShadows(true);
        if (TES::pInstance->pInteriorCell) {
            iMaxActorShadows = Renderer::iActorShadowCountInt.Int();
        }
        else {
            iMaxActorShadows = Renderer::iActorShadowCountExt.Int();
        }
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        BSShaderManager::TextureManagerPtr()->SetFrustumShadowTextureCount(
            pRenderer,
            static_cast<uint32_t>(iMaxActorShadows)
        );
    }
    if (BSShaderManager::GetShadows() && Renderer::QDoShadows()) {
        BSShaderManager::ResetNextAvailableShadowMapAccumulator();
        ShadowSceneNode* pSSNode = BSShaderManager::GetShadowSceneNode(0);
        pSSNode->DisableAllShadowLights();
        int32_t iMaxActorShadows = TES::pInstance->pInteriorCell ? Renderer::iActorShadowCountInt.Int()
                                                                 : Renderer::iActorShadowCountExt.Int();
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        BSShaderManager::TextureManagerPtr()->SetFrustumShadowTextureCount(
            pRenderer,
            static_cast<uint32_t>(iMaxActorShadows)
        );
        if (iMaxActorShadows) {
            ShadowSceneLight* pPlayerShadow = nullptr;
            int32_t iCount = 0;
            if ((pPlayer->Is3rdPerson() || pPlayer->IsVanityMode()) && !pPlayer->IsRefractive()) {
                pPlayerShadow = pSSNode->AddShadowLight(pPlayer->Get3D(false));
                ++iCount;
            }
            ReferenceProcessLists.kNearbyActorLock.Lock(nullptr);
            Actor** pCloseActorsA = ReferenceProcessLists.pActorsCloseToPlayerA;
            int32_t iNumCloseActors = ReferenceProcessLists.iActorsCloseToPlayer;
            for (int32_t iCurCloseActor = 0; iCurCloseActor < iNumCloseActors && iCount < iMaxActorShadows;
                 ++iCurCloseActor) {
                Actor* pActor = pCloseActorsA[iCurCloseActor];
                if (pActor->Get3D() &&
                    pActor->Get3D()->m_pkParent &&
                    pActor->HasShadow() &&
                    pActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING &&
                    !pActor->IsRefractive()) {
                    const NiFixedString* pExtraBoundTag = &BSBound::Tag();
                    BSBound* pBound = static_cast<BSBound*>(pActor->Get3D()->GetExtraData(*pExtraBoundTag));
                    NiPoint3* pExtents = pBound ? &pBound->Extents : &NiPoint3::ZERO;
                    float fX = pExtents->x;
                    float fY = pExtents->y;
                    float fZ = pExtents->z;
                    float fMaxXY = BSMAX(fX, fY);
                    float fBRadius = BSMAX(fMaxXY, fZ);
                    if (pActor->Get3D()->GetWorldBound().m_fRadius < fBRadius * 3.0) {
                        pSSNode->AddShadowLight(pActor->Get3D());
                        ++iCount;
                    }
                }
            }
            ReferenceProcessLists.kNearbyActorLock.Unlock();
            pSSNode->SortShadowLights(pPlayerShadow);
            NiAVObject* p1stPerson3D = pPlayer->Get3D(true);
            if (p1stPerson3D) {
                SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
                BSCullingProcess* pCuller = pSceneGraph->pCuller;
                pCuller->m_pkCamera = pSceneGraph->GetCamera();
                pCuller->SetFrustum(pSceneGraph->GetCamera()->GetViewFrustum());
                bool b1stCull = p1stPerson3D->GetAppCulled();
                p1stPerson3D->SetAppCulled(true);
                NiAVObject* pGNode = TESTerrainLODManager::GetGrassNode();
                BSFadeNode::SetFadeEnabled(false);
                bool bGrassCull = false;
                if (pGNode) {
                    bGrassCull = pGNode->GetAppCulled();
                    if (!Renderer::bShadowsOnGrass.Bool()) {
                        pGNode->SetAppCulled(true);
                    }
                }
                ShadowSceneLight* pSSLight = pSSNode->GetFirstShadowLight();
                int32_t iEnabledShadows = 0;
                while (pSSLight) {
                    if (pSSLight->GetLight() && pSSLight->GetShadowScene()) {
                        NiBound kBound;
                        NiPoint3 kShadowCasterPos;
                        if (!pSSLight->IsFadedOut() && pSSLight->GetLight()->GetAppCulled()) {
                            if (pSSLight->GetShadowScene()->GetAppCulled()) {
                                pSSLight->SetFadeTarget(0.0f, true);
                            }
                            else {
                                if (iEnabledShadows < iMaxActorShadows) {
                                    pSSLight->GetLight()->SetAppCulled(false);
                                }
                                pSSLight->SetFadeTarget(0.0f, false);
                            }
                        }
                        if (iEnabledShadows >= iMaxActorShadows ||
                            pSSLight->GetLight()->GetAppCulled() ||
                            pSSLight->GetShadowScene()->GetAppCulled() ||
                            (kBound = pSSLight->GetShadowScene()->GetWorldBound(),
                             (kShadowCasterPos = pSSLight->GetShadowScene()->GetWorldTranslate(),
                              (pSSLight->ComputeWeightedShadowLight(nullptr, kShadowCasterPos),
                               (pSSLight->TestFrustumCull(*pCuller), pSSLight->GetFrustumCull() == 255)))) ||
                            !(pSSLight->fShadowLODDimmer >= 0.05)) {
                            pSSLight->GetLight()->SetAppCulled(true);
                        }
                        else {
                            ++iEnabledShadows;
                            NiUpdateData kUpdateData(0.0f, false, false);
                            pSSLight->GetLight()->Update(kUpdateData);
                            pSSLight->AccumulateGeometry(pCuller, iEnabledShadows - 1);
                            pSSLight->InsertUnusedFence();
                            BSShaderManager::GetShadowSceneNode(0)->UpdateQueuedLight(pSSLight);
                            if (Renderer::bActorSelfShadowing.Bool()) {
                                pSSLight->AddObject(pSSLight->GetShadowScene());
                            }
                            pSSLight->RemoveUnusedGeom();
                        }
                    }

                    pSSLight = pSSNode->GetNextShadowLight();
                }
                while (iEnabledShadows + 17 < 21) {
                    MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, iEnabledShadows + 17);
                    ++iEnabledShadows;
                }
                if (pGNode) {
                    pGNode->SetAppCulled(bGrassCull);
                }
                BSFadeNode::SetFadeEnabled(true);
                pSSNode->UpdateDynamicShadows(*pCuller);
                p1stPerson3D->SetAppCulled(b1stCull);
            }
        }
        else {
            pSSNode->RemoveAllShadowLights();
        }
    }
    else {
        for (int32_t i = 17; i < 21; ++i) {
            MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, i);
            MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, i);
        }
    }
    UpdateOffscreenInterface();
}

void Main::KillMenuBGTexture() {
    BSRenderedTexture* pMenuBGTexture = Main::spMenuBGTexture;
    if (pMenuBGTexture) {
        pMenuBGTexture = Main::spMenuBGTexture;
        BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(pMenuBGTexture);
        Main::spMenuBGTexture = nullptr;
        BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendValue(0.0);
        BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendEnabled(false);
    }

    Main::bMenuBGReady = false;
}

// Builds the viewport rects with NiColorA's constructor. Reads the rendered menu's screen parent through
// FORenderedMenu::GetParentNodeToRender called non-virtually. AutoMemContext as in UpdateOffscreenInterface.
void Main::RenderMenuBackground() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
        7193
    );
    if (!pPlayer->GetParentCell() || !pPlayer->Get3D()) {
        pContext->~AutoMemContext();
        return;
    }
    bool bEOFEnabled = ImageSpaceManager::bEOFEnabled;
    ImageSpaceManager::bEOFEnabled = true;
    bRenderingMenuBG = true;
    static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->LockRenderer();
    Tile::Lock();
    MTRenderingSystem::QInstance()->StartThreads();
    if (spMenuBGTexture) {
        BSRenderedTexture* pOldTexture = spMenuBGTexture;
        BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(pOldTexture);
    }
    NiRenderer* pRenderer = NiRenderer::GetRenderer();
    BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
    spMenuBGTexture = pTextureManager->BorrowRenderedTexture(
        static_cast<NiDX9Renderer*>(pRenderer),
        BSTextureManager::TEX_MAIN_MENUBG,
        0,
        nullptr,
        0
    );
    NiRenderTargetGroup* pTarget = NiRenderer::GetRenderer()->GetDefaultRenderTargetGroup();
    uint32_t uiTextureSize = spMenuBGTexture->GetWidth(0);
    bool bResized = pTarget->GetWidth(0) != uiTextureSize;
    if (!bResized) {
        pTarget = NiRenderer::GetRenderer()->GetDefaultRenderTargetGroup();
        uiTextureSize = spMenuBGTexture->GetHeight(0);
        bResized = pTarget->GetHeight(0) != uiTextureSize;
    }
    if (bResized) {
        pTarget = NiRenderer::GetRenderer()->GetDefaultRenderTargetGroup();
        double dTextureWidth = static_cast<double>(spMenuBGTexture->GetWidth(0));
        float fWidthRatio = static_cast<float>(static_cast<double>(pTarget->GetWidth(0)) / dTextureWidth);
        float fRight = BSMIN(fWidthRatio, 1.0f);
        pTarget = NiRenderer::GetRenderer()->GetDefaultRenderTargetGroup();
        double dTextureHeight = static_cast<double>(spMenuBGTexture->GetHeight(0));
        float fHeightRatio = static_cast<float>(static_cast<double>(pTarget->GetHeight(0)) / dTextureHeight);
        float fTop = BSMIN(fHeightRatio, 1.0f);
        NiRect<float> kPort;
        ::new (&kPort) NiColorA(0.0f, fRight, fTop, 0.0f);
        WorldRootCamera()->SetViewPort(kPort);
    }
    else if (BSShaderManager::bLetterbox) {
        WorldRootCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
    }
    TESForm* pImageSpaceMod = InterfaceManager::Instance()->GetCurrentImagespaceMod();
    ImageSpaceModifierInstanceForm::Stop(TESImageSpaceModifier::GetGetHit());
    if (pImageSpaceMod) {
        ImageSpaceModifierInstanceForm::Trigger(static_cast<TESImageSpaceModifier*>(pImageSpaceMod), 1.0f, nullptr);
        OnIdle_UpdateImageSpace(false);
    }
    bool bSkipFirstPerson = Interface::IsInPipboyMenu();
    if (Interface::IsMenuIDVisible(Interface::eStartMenu, 0)) {
        bSkipFirstPerson = false;
    }
    DrawWorld(spMenuBGTexture, false, bSkipFirstPerson);
    if (Interface::IsMenuIDVisible(Interface::eRaceSexMenu, 0) && StartMenu::PauseMenuIsActive()) {
        alignas(BSCullingProcess) uint8_t aCullerStorage[sizeof(BSCullingProcess)];
        BSCullingProcess* pCuller = ::new (aCullerStorage) BSCullingProcess(nullptr);
        NiCamera* pCamera = Interface::GetCurrentRenderedMenu()->GetCamera();
        pCuller->SetCamera(pCamera);
        pCuller->SetAccumulator(spRenderedMenuAccum);
        ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(
            BSShaderManager::BSSM_SSN_MENU_OBJECT_3D
        );
        spRenderedMenuAccum->SetShadowSceneNode(pShadowSceneNode);
        FORenderedMenu* pRenderedMenu = Interface::GetCurrentRenderedMenu();
        NiAVObject* pScreenParent;
        pScreenParent = pRenderedMenu->FORenderedMenu::GetParentNodeToRender();
        BSShaderUtil::AccumulateScene(pCamera, pScreenParent, *pCuller);
        BSShaderUtil::RenderScene(pCamera, spRenderedMenuAccum, false);
        pCuller->SetAccumulator(nullptr);
        pCuller->~BSCullingProcess();
    }
    if (pImageSpaceMod) {
        ImageSpaceModifierInstanceForm::Stop(static_cast<TESImageSpaceModifier*>(pImageSpaceMod));
        OnIdle_UpdateImageSpace(false);
    }
    MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 0x17);
    MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 0x17);
    MTRenderingSystem::QInstance()->WaitForThreads();
    bMenuBGReady = true;
    NiRect<float> kPort;
    ::new (&kPort) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    WorldRootCamera()->SetViewPort(kPort);
    Tile::Unlock();
    static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->UnlockRenderer();
    bRenderingMenuBG = false;
    ImageSpaceManager::bEOFEnabled = bEOFEnabled;
    pContext->~AutoMemContext();
}

// An AutoMemContext is constructed and destroyed on a stack context, and the constructor's result is ignored.
void Main::UpdateOffscreenInterface() {
    if (Interface::IsInMenuMode() &&
        (bUsePipboyMode.Bool() || bUseImageSpaceMenuFX.Bool()) &&
        Interface::IsInRenderedMenu()) {
        alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
        AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
        ::new (aucContext) AutoMemContext(
            MEM_CONTEXT_INTERFACE,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
            6878
        );
        bool bInPipboy = Interface::IsInPipboyMenu();
        if (Interface::IsInMenuMode()) {
            Interface::LastMinuteUpdate();
        }
        if (!spInterfaceFXTex) {
            NiRenderer* pRenderer = NiRenderer::GetRenderer();
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            spInterfaceFXTex = pTextureManager->BorrowRenderedTexture(
                static_cast<NiDX9Renderer*>(pRenderer),
                BSTextureManager::TEX_INTERFACE_RENDEREDMENU,
                0,
                nullptr,
                0
            );
        }
        if (!bInPipboy) {
            Interface::GetPipboy()->SetScreenTexture(nullptr);
        }
        bool bMenuOpen = Interface::IsCurrentRenderedMenuTopmost() ||
            Menu::GetMenuByClass(Interface::eMessageMenu) ||
            Menu::GetMenuByClass(Interface::eQuantityMenu) ||
            Menu::GetMenuByClass(Interface::eTutorialMenu);
        if (Interface::IsInRenderedMenu() && bMenuOpen) {
            bool bHideMessage = bInPipboy && Interface::IsMenuIDVisible(Interface::eMessageMenu, 0);
            bool bWasCulled = false;
            NiNode* pMessageModel = nullptr;
            if (bHideMessage) {
                pMessageModel = Menu::GetMenuByClass(Interface::eMessageMenu)->GetModel();
                bWasCulled = pMessageModel->GetAppCulled();
                pMessageModel->SetAppCulled(true);
            }
            FORenderedMenu* pMenu = Interface::GetCurrentRenderedMenu();
            pMenu->Draw(spInterfaceFXTex, NiRenderer::CLEAR_BACKBUFFER, nullptr);
            if (bHideMessage) {
                pMessageModel->SetAppCulled(bWasCulled);
            }
        }
        pContext->~AutoMemContext();
    }
    else if (Interface::IsInMenuMode()) {
        Interface::LastMinuteUpdate();
    }
}

// Tests the setting's whole byte and then stores 1; read as bool, clang ors the raw byte into abValue.
void Main::SetActive(bool abValue) {
    if (static_cast<uint8_t>(bAlwaysActive.Bool())) {
        abValue = true;
    }
    if (Controls::GetInstance()) {
        Controls::GetInstance()->ClearKeystrokes();
        Controls::GetInstance()->UpdateValues();
        Controls::GetInstance()->ClearUserActions();
    }
    if (abValue) {
        appTimer.Enable();
    }
    else {
        appTimer.Disable();
    }
    bGameActive = abValue;
}

// The water system is fetched once before SetIsUnderWater without being used.
void Main::DrawWorld_UpdateWater() {
    if (TESWaterSystem::bUseWater.Bool()) {
        int32_t iMaxAnisotropy = NiRenderer::iMaxAnisotropy;
        NiRenderer::iMaxAnisotropy = 1;
        if (!TESObjectCELL::IsNodeCulled(TESObjectCELL::CCN_WATER) &&
            (!TES::pInstance->GetInterior() || TES::pInstance->GetInterior()->HasWater())) {
            ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
            bool bDisableLightUpdate = pShadowSceneNode->GetDisableLightUpdate();
            pShadowSceneNode->SetDisableLightUpdate(true);
            NiCamera* pCamera = WorldRootCamera();
            TES::pInstance->GetWaterSystem()->UpdateWaterSystem(pCamera, pShadowSceneNode);
            pShadowSceneNode->SetDisableLightUpdate(bDisableLightUpdate);
        }
        else {
            WaterShader::bUnderWater = false;
            TES::pInstance->GetWaterSystem()->iAccumulationCount = 0;
            appMain->QDrawWorldAccum()->SetUnderWater(false);
            SkyShader::bUnderWater = false;
            TES::pInstance->GetWaterSystem();
            TESWaterSystem::SetPlayerUnderwater(false);
        }
        NiRenderer::iMaxAnisotropy = iMaxAnisotropy;
    }
    for (int32_t i = TES::pInstance->GetWaterSystem()->iAccumulationCount + 1; i < 0x11; ++i) {
        MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, i);
        MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, i);
    }
}

void Main::DrawWorld_WaitForParallelUpdate() {
    if (NiParallelUpdateTaskManager::Get()) {
        if (NiParallelUpdateTaskManager::IsActive()) {
            NiParallelUpdateTaskManager::EndUpdate();
        }
        NiParallelUpdateTaskManager::WaitRenderSema(3);
    }
}

// Only the low byte of GetMultiSampleEnabled's result is kept.
void Main::DrawWorldMenu3D(BSRenderedTexture* apDestination) {
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    ImageSpaceManager* pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
    bool bIsMSAA = Renderer::GetMultiSampleEnabled();
    DrawWorld_WaitForParallelUpdate();
    BSRenderedTexture* pRenderTexture = DrawWorld_InitRenderTexture(pRenderer, apDestination, false, bIsMSAA);
    DrawWorld_CopyMenuBG(pImageSpaceManager, pRenderer, bIsMSAA, apDestination);
    if (Interface::IsMenuIDVisible(Interface::eSurgeryMenu, 0)) {
        Interface::DrawSurgeryMenu3D();
    }
    if (Interface::IsMenuIDVisible(Interface::eLockPickMenu, 0)) {
        Interface::DrawLockpickMenu3D();
    }
    if (Interface::IsMenuIDVisible(Interface::eSlotMachineMenu, 0)) {
        Interface::DrawSlotMachineMenu3D();
    }
    if (Interface::IsMenuIDVisible(Interface::eBlackJackMenu, 0)) {
        Interface::DrawBlackJackMenu3D();
    }
    if (Interface::IsMenuIDVisible(Interface::eRouletteMenu, 0)) {
        Interface::DrawRouletteMenu3D();
    }
    if (Interface::IsMenuIDVisible(Interface::eCaravanMenu, 0)) {
        Interface::DrawCaravanMenu3D();
    }
    if (Interface::IsMenuIDVisible(Interface::eLoveTesterMenu, 0)) {
        LoveTesterMenu::Draw3DElements();
    }
    if (BSShaderManager::GetImageSpaceEffects()) {
        DrawWorld_ImageSpace(bIsMSAA, pRenderTexture, pRenderer, apDestination);
    }
    else if (apDestination) {
        DrawWorld_StopRenderTarget(bIsMSAA, apDestination);
    }
    DrawWorld_End(pRenderTexture, pImageSpaceManager, pRenderer, nullptr);
    if (Interface::IsMenuIDVisible(Interface::eSPECIALBookMenu, 0)) {
        SPECIALBookMenu::Draw3DElements();
    }
}

void Main::DrawWorld_DoVolumetricFog(BSShaderAccumulator* apShaderAccum, BSRenderedTexture* apRenderTexture) {
    BSSceneGraph* pWorldRoot = GetWorldSceneGraph();
    apShaderAccum->RenderVolumetricFog(pWorldRoot->GetCamera(), apRenderTexture);
}

void Main::DrawWorld_ScreenSplatter(NiDX9Renderer* apRenderer) {
    bool bResetCameraPos = ScreenSplatter::bResetCameraPos;
    ScreenSplatter::bResetCameraPos = false;
    apRenderer->SetCameraData(ScreenSplatter::GetCamera());
    spScreenSplatterAccum->RenderScreenSplatter();
    ScreenSplatter::bResetCameraPos = bResetCameraPos;
}

BSRenderedTexture* Main::DrawWorld_InitRenderTexture(
    NiDX9Renderer* apRenderer,
    BSRenderedTexture* apDestination,
    bool abWireframe,
    bool abMultiSampleEnabled
) {
    BSRenderedTexture* pRenderTexture = nullptr;
    if (BSShaderManager::GetImageSpaceEffects() && !abWireframe) {
        alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
        AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
        ::new (aucContext) AutoMemContext(
            MEM_CONTEXT_IMAGE_SPACE,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
            7789
        );
        pRenderTexture = BSShaderManager::TextureManagerPtr()
                             ->BorrowRenderedTexture(apRenderer, BSTextureManager::TEX_MAIN_IMAGESPACE, 0, nullptr, 0);
        pRenderTexture->UsePreRestoreGroup();
        BSShaderManager::pCurrentRenderTarget = pRenderTexture;
        // Calls NiRenderer's screen size getters, not NiDX9Renderer's.
        NiRenderer* pRenderer = apRenderer;
        uint32_t uiScreenWidth = pRenderer->GetScreenWidth();
        bool bResized = uiScreenWidth != pRenderTexture->GetWidth(0);
        if (!bResized) {
            uint32_t uiScreenHeight = pRenderer->GetScreenHeight();
            bResized = uiScreenHeight != pRenderTexture->GetHeight(0);
        }
        if (bResized) {
            double dWidth = static_cast<double>(pRenderer->GetScreenWidth()) * 1.05;
            float fRight = static_cast<float>(dMin(dWidth / static_cast<double>(pRenderTexture->GetWidth(0)), 1.0f));
            double dHeight = static_cast<double>(pRenderer->GetScreenHeight()) * 1.05;
            float fTop = static_cast<float>(dMin(dHeight / static_cast<double>(pRenderTexture->GetHeight(0)), 1.0f));
            NiRect<float> kPort;
            ::new (&kPort) NiColorA(0.0f, fRight, fTop, 0.0f);
            WorldRootCamera()->SetViewPort(kPort);
        }
        BSRenderedTexture::StartOffscreen(7, pRenderTexture->GetGroup());
        pContext->~AutoMemContext();
    }
    else {
        alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
        AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
        ::new (aucContext) AutoMemContext(
            MEM_CONTEXT_SHADERS,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
            7812
        );
        if (apDestination) {
            if (abMultiSampleEnabled) {
                BSRenderedTexture::StartOffscreen(7, apRenderer->GetDefaultRenderTargetGroup());
            }
            else {
                BSRenderedTexture::StartOffscreen(7, apDestination->GetGroup());
            }
        }
        else {
            BSRenderedTexture::Start(7, nullptr);
        }
        pContext->~AutoMemContext();
    }
    return pRenderTexture;
}

bool Main::DrawWorld_VATSOcclusionQuery() {
    bool bIssued = false;
    if (Interface::GetVATSEffect()->GetTarget()) {
        if (!spVATSTex) {
            NiRenderer* pRenderer = NiRenderer::GetRenderer();
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            spVATSTex = pTextureManager->BorrowRenderedTexture(
                static_cast<NiDX9Renderer*>(pRenderer),
                BSTextureManager::TEX_INTERFACE_VATS,
                0,
                nullptr,
                0
            );
        }
        if (!Interface::GetVATSEffect()->GetEffectMode() &&
            spVATSTex &&
            Interface::GetVATSEffect()->GetTarget() &&
            Interface::GetVATSEffect()->OcclusionEnabled()) {
            BSRenderedTexture* pTexture = spVATSTex;
            Interface::GetVATSEffect()->DrawTargetToTexture(
                pTexture,
                FOVATSEffectManager::VATS_OCCLUSION_QUERY_NONE,
                true,
                FOVATSEffectManager::VATS_DRAW_SELECTED_PARTS,
                NiRenderer::CLEAR_ALL,
                false
            );
            pTexture = spVATSTex;
            Interface::GetVATSEffect()->DrawTargetToTexture(
                pTexture,
                FOVATSEffectManager::VATS_OCCLUSION_QUERY_FULL,
                true,
                FOVATSEffectManager::VATS_DRAW_SELECTED_PARTS,
                NiRenderer::CLEAR_BACKBUFFER,
                true
            );
            bIssued = true;
        }
    }
    return bIssued;
}

// abIssuedVATSQuery is not read.
void Main::DrawWorld_VATS([[maybe_unused]] bool abIssuedVATSQuery) {
    if (Interface::GetVATSEffect()->GetTarget()) {
        BSRenderedTexture::StopOffscreen();
        BSRenderState::SetColorWriteEnable(0, BSRS_LOCK);
        BSRenderedTexture* pTexture = spVATSTex;
        Interface::GetVATSEffect()->DrawTargetToTexture(
            pTexture,
            FOVATSEffectManager::VATS_OCCLUSION_QUERY_NONE,
            false,
            FOVATSEffectManager::VATS_DRAW_ENTIRE_TARGET,
            NiRenderer::CLEAR_ALL,
            false
        );
        BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
        pTexture = spVATSTex;
        Interface::GetVATSEffect()->DrawTargetToTexture(
            pTexture,
            FOVATSEffectManager::VATS_OCCLUSION_QUERY_NONE,
            false,
            FOVATSEffectManager::VATS_DRAW_SELECTED_PARTS,
            NiRenderer::CLEAR_BACKBUFFER,
            true
        );
    }
}

// The image-space manager and the renderer are fetched here without being used. abMultiSampleEnabled
// is not read.
void Main::DrawWorld_RestartRenderTexture(
    BSRenderedTexture* apRenderTexture,
    [[maybe_unused]] bool abMultiSampleEnabled,
    uint32_t auiClearMode
) {
    if (BSRenderedTexture::QOutsideFrame()) {
        BSShaderManager::GetImageSpaceManager();
        BSShaderManager::GetRenderer();
        BSRenderedTexture::StartOffscreen(auiClearMode, apRenderTexture->GetGroup());
    }
}

// abMultiSampleEnabled is not read. AutoMemContext is handled as in UpdateOffscreenInterface.
void Main::DrawWorld_ImageSpace(
    [[maybe_unused]] bool abMultiSampleEnabled,
    BSRenderedTexture* apRenderTexture,
    NiDX9Renderer* apRenderer,
    BSRenderedTexture* apDestination
) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_IMAGE_SPACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
        9058
    );
    if (!BSRenderedTexture::QOutsideFrame()) {
        BSRenderedTexture::StopOffscreen();
    }
    if (!Interface::IsInPipboyMenu() &&
        !Interface::IsInMenuMode() &&
        !Interface::GetLoadingMenuVisible() &&
        !pSaveLoadGame->GetSaveGameLoading() &&
        !pSaveLoadGame->GetSaveGameSaving() &&
        !bRenderingMenuBG) {
        Interface::Accumulate();
    }
    if (spVATSTex) {
        BSRenderedTexture* pMask = spVATSTex;
        Interface::GetVATSEffect()->PrepareEffect(apRenderTexture, pMask, nullptr, false);
    }
    bool bMenuFade = Interface::IsInRenderedMenu() && !bRenderingMenuBG && bMenuBGReady;
    if (bMenuFade) {
        ImageSpaceEffectHDR::bUseAlphaMask = true;
        FORenderedMenu* pMenu = Interface::GetCurrentRenderedMenu();
        float fFadeAmount = pMenu->GetFadeAmount();
        ImageSpaceEffectHDR::fAlpha = fFadeAmount;
    }
    BSShaderManager::spCamera = GetWorldSceneGraph()->GetCamera();
    BSShaderManager::RenderImageSpaceEffects(apRenderer, apRenderTexture, apDestination);
    if (spVATSTex) {
        Interface::GetVATSEffect()->ShutdownEffect();
        BSRenderedTexture* pMask = spVATSTex;
        BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(pMask);
        spVATSTex = nullptr;
    }
    if (bMenuFade) {
        ImageSpaceEffectHDR::bUseAlphaMask = false;
        ImageSpaceEffectHDR::fAlpha = 1.0f;
    }
    NiRect<float> kPort;
    ::new (&kPort) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    WorldRootCamera()->SetViewPort(kPort);
    pContext->~AutoMemContext();
}

void Main::QuitGame() {
    bQuitGame = true;
}

void Main::SetFreezeTime(bool abFreezeTime) {
    bFreezeTime = abFreezeTime;
}

void Main::SetFlyCamera(bool abFlyCamera) {
    bFlyCamera = abFlyCamera;
}

void Main::ResetGame() {
    bResetGame = true;
}

void Main::SetGame() {
    bResetGame = false;
    bReloadContent = false;
}

bool Main::LoadPluginsFromFile(const char* pPath, const char* pFilename) {
    bool bFound = false;
    char acFileName[0x104];
    char acLine[0x200];
    BSstrcpy(acFileName, 0x104, pPath);
    BSstrcat(acFileName, 0x104, pFilename);
    std::FILE* pFile = rt::fopen(acFileName, "r");
    if (pFile) {
        while (rt::fgets(acLine, 0x200, pFile)) {
            uint32_t uiLength = BSstrlen(acLine);
            if (acLine[0] != '#' && uiLength > 1) {
                if (acLine[uiLength - 1] == '\n') {
                    acLine[uiLength - 1] = '\0';
                }
                TESFile* pPlugin = pDataHandler->GetListFile(acLine);
                if (pPlugin) {
                    pPlugin->SetChecked(true);
                    bFound = true;
                }
            }
        }
        std::fclose(pFile);
    }
    return bFound;
}

char Main::HandleMouseMove(int32_t x, int32_t y) {
    // The window has the input focus.
    if (SDL_GetKeyboardFocus() != pSDLMainWindow) {
        return 0;
    }
    if (!Interface::IsInMenuMode()) {
        return 0;
    }
    if (!Controls::GetInstance()->IsMouseInBackground()) {
        return 0;
    }
    if (Interface::IsTopMenuID(Interface::eSurgeryMenu)) {
        Interface::SetSurgeryCursorPos(x, y);
    }
    else {
        float fY = static_cast<double>(y) / static_cast<double>(Renderer::GetScreenHeight());
        float fX = static_cast<double>(x) / static_cast<double>(Renderer::GetScreenWidth());
        Interface::SetCursorPos(fX, fY);
    }
    return 1;
}

// The viewport is built with NiColorA's constructor.
Sun* Main::DrawWorld_Init() {
    NiRect<float> kViewport;
    Main::GetWorldSceneGraph()->GetCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
    if (pVATS.eMode != VATS::VATS_PLAYBACK) {
        float fWorldFOV = pPlayer->GetWorldFOV();
        Main::GetWorldSceneGraph()->SetCameraFOV(fWorldFOV, false, nullptr, true);
    }
    BSShaderManager::SetFOV(pPlayer->GetWorldFOV());
    if (bRenderingMenuBG) {
        ::new (&kViewport) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
        Main::GetWorldSceneGraph()->GetCamera()->SetViewPort(kViewport);
    }
    NiCamera* pCamera = Main::spWorldRoot->GetCamera();
    NiCamera* pVisCamera = nullptr;
    if (bPreCullActors.Bool()) {
        static HighActorCuller cullHA;
        if (pVisCamera) {
            cullHA.Run(pVisCamera);
        }
        else {
            cullHA.Run(pCamera);
        }
    }
    NiNode* pCameraRoot = Main::spWorldRoot->GetCameraRoot();
    Main::spSkyRoot->SetTranslate(pCameraRoot->GetTranslate());
    NiUpdateData kSkyUpdateData(0.0f, false, false);
    Main::spSkyRoot->Update(kSkyUpdateData);
    pCameraRoot = Main::spWorldRoot->GetCameraRoot();
    Main::spWeatherRoot->SetTranslate(pCameraRoot->GetTranslate());
    NiUpdateData kWeatherUpdateData(0.0f, false, false);
    Main::spWeatherRoot->Update(kWeatherUpdateData);
    if (BSShaderAccumulator::bRequestRenderPassesScenegraph) {
        BSShaderAccumulator::bRequestRenderPassesScenegraph = false;
        BSShaderAccumulator::bShowRenderPassesScenegraph = true;
    }
    BSShaderAccumulator::SetRunBoundingSphereTests(bEnableBoundingVolumeOcclusion.Bool(), true);
    BSShaderAccumulator::SetDisplayBoundingSpheres(bDisplayBoundingVolumes.Bool());
    BSShaderManager::bDepthRestored = false;
    Sun* pSun = nullptr;
    if (TES::pInstance->pSky &&
        TES::pInstance->pSky->pSun &&
        (TES::pInstance->pSky->eMode == Sky::SM_FULL || TES::pInstance->pSky->eMode == Sky::SM_FAKEEXTERIOR)) {
        return TES::pInstance->pSky->pSun;
    }

    return pSun;
}

void Main::DrawWorld_DrawScene(Sun* psun, int32_t abDo1stPerson, bool abWireframe, bool abIssueVATSPartialQuery) {
    NiTPointerList<NiAVObject*> lAccumOne;
    NiTPointerList<NiAVObject*> lAccumTwo;
    NiTPointerList<NiAVObject*> lAccumThree;
    SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
    BSCullingProcess* pCuller = pSceneGraph->pCuller;
    ImageSpaceEffectRadialBlur::pWorldCamera = pSceneGraph->GetCamera();
    if (psun) {
        psun->StartSunOcclusion();
    }
    bool b1stCull = pPlayer->Get3D(true)->GetAppCulled();
    pPlayer->Get3D(true)->SetAppCulled(true);
    spDrawWorldAccum->bWorldGeometry = !abWireframe;
    ImageSpaceEffectDepthOfField* pDOFEffect = static_cast<ImageSpaceEffectDepthOfField*>(
        BSShaderManager::GetImageSpaceManager()->GetEffect(ImageSpaceManager::EFFECT_DEPTH_OF_FIELD)
    );
    bool bRenderDOF = !abWireframe && pDOFEffect->DoDepth();
    spDrawWorldAccum->bDoDepthPasses = bRenderDOF;
    spDraw1stPersonAccum->bDoDepthPasses = !abWireframe && pDOFEffect->DoDepth();
    AutoMemContext kTempMemContext(
        MEM_CONTEXT_GAMEBRIO,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
        0x1EB7
    );
    NiCamera* pAccumCamera = pSceneGraph->GetCamera();
    pCuller->m_pkCamera = pAccumCamera;
    pCuller->SetFrustum(pAccumCamera->GetViewFrustum());
    pCuller->SetAccumulator(spDrawWorldAccum.m_pObject);
    if (BSShaderManager::bMTRendering) {
        ::new (&lAccumOne) NiTPointerList<BSAnimGroupSequence*>;
        ::new (&lAccumTwo) NiTPointerList<BSAnimGroupSequence*>;
        ::new (&lAccumThree) NiTPointerList<BSAnimGroupSequence*>;
        NiNode* pSSN = BSShaderManager::GetShadowSceneNode(0);
        NiAVObject* pAVObj = nullptr;
        NiNode* pNode = nullptr;
        pAVObj = pSSN->GetAtChecked(0);
        if (pAVObj && !pAVObj->GetAppCulled()) {
            lAccumThree.AddTail(pAVObj);
        }

        pAVObj = pSSN->GetAtChecked(1u);
        if (pAVObj && !pAVObj->GetAppCulled()) {
            lAccumThree.AddTail(pAVObj);
        }

        pAVObj = pSSN->GetAtChecked(2u);
        if (pAVObj && !pAVObj->GetAppCulled()) {
            pNode = pAVObj->IsNode();
        }

        if (pNode) {
            pAVObj = pNode->GetAtChecked(0);
            if (pAVObj && !pAVObj->GetAppCulled()) {
                lAccumOne.AddTail(pAVObj);
            }

            pAVObj = pNode->GetAtChecked(1u);
            if (pAVObj && !pAVObj->GetAppCulled()) {
                lAccumOne.AddTail(pAVObj);
            }

            pAVObj = pNode->GetAtChecked(2u);
            if (pAVObj && !pAVObj->GetAppCulled()) {
                lAccumOne.AddTail(pAVObj);
            }
        }
        pAVObj = pSSN->GetAtChecked(3u);
        pNode = pAVObj->IsNode();
        if (TES::pInstance->pInteriorCell) {
            if (BSShaderManager::GetShadowSceneNode(0)->HasVisibleUnboundSpace()) {
                pAVObj = pNode->GetAtChecked(0);
                if (pAVObj && !pAVObj->GetAppCulled()) {
                    lAccumTwo.AddTail(pAVObj);
                }

                pAVObj = pNode->GetAtChecked(1u);
                if (pAVObj && !pAVObj->GetAppCulled()) {
                    lAccumOne.AddTail(pAVObj);
                }

                pAVObj = pNode->GetAtChecked(2u);
                if (pAVObj) {
                    pNode = pAVObj->IsNode();
                    if (pNode && !pNode->GetAppCulled()) {
                        for (int32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                            pAVObj = pNode->GetAt(i);
                            if (pAVObj && i != 4 && i) {
                                NiNode* pChNode = pAVObj->IsNode();
                                if (pChNode && !pChNode->GetAppCulled()) {
                                    for (int32_t j = i == 6; j < pChNode->GetArrayCount(); ++j) {
                                        NiAVObject* pChObj = pChNode->GetAt(j);
                                        if (pChObj && !pChObj->GetAppCulled()) {
                                            if (j % 2) {
                                                lAccumOne.AddTail(pChObj);
                                            }
                                            else {
                                                lAccumTwo.AddTail(pChObj);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else {
                pAVObj = pNode->GetAtChecked(2u);
                if (pAVObj) {
                    pNode = pAVObj->IsNode();
                    if (pNode && !pNode->GetAppCulled()) {
                        pAVObj = pNode->GetAtChecked(7u);
                        if (pAVObj) {
                            pNode = pAVObj->IsNode();
                            if (pNode) {
                                for (int32_t j = 0; j < pNode->GetArrayCount(); ++j) {
                                    NiAVObject* pChObj = pNode->GetAt(j);
                                    if (pChObj && !pChObj->GetAppCulled()) {
                                        if (j % 2) {
                                            lAccumOne.AddTail(pChObj);
                                        }
                                        else {
                                            lAccumTwo.AddTail(pChObj);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            BSPortalGraph* pPortalGraph = BSShaderManager::GetShadowSceneNode(0)->GetPortalGraph();
            NiTPointerList<NiPointer<BSMultiBoundRoom>>* pMBRoomAccumList = pPortalGraph
                ? pPortalGraph->GetMBRoomAccumList()
                : nullptr;
            NiTListItem<NiPointer<BSMultiBoundRoom>>* kRoomIter = pMBRoomAccumList ? pMBRoomAccumList->m_pkHead
                                                                                   : nullptr;
            int32_t iSPCount = 0;
            while (kRoomIter) {
                BSMultiBoundRoom* pRoom = pPortalGraph->GetMBRoomAccumList()->GetNext(kRoomIter).m_pObject;
                if (pRoom && !pRoom->kPortalList.IsEmpty()) {
                    NiTListItem<BSPortal*>* kPortalIter = pRoom->kPortalList.m_pkHead;
                    while (kPortalIter) {
                        BSPortal* pPortal = pRoom->kPortalList.GetNext(kPortalIter);
                        if (pPortal) {
                            if (pCuller->AddShared(pPortal->QSharedGeometryNode())) {
                                BSPortalSharedNode* pSharedNode = pPortal->QSharedGeometryNode();
                                if (pSharedNode) {
                                    if (pPortal->pMultiBoundRoom[0] && pPortal->pMultiBoundRoom[1]) {
                                        NiCamera* pCamera = pCuller->m_pkCamera;
                                        BSShaderManager::GetShadowSceneNode(0)->BuildSharedCompoundFrustum(
                                            pCamera,
                                            pPortal
                                        );
                                    }

                                    if (iSPCount % 2) {
                                        lAccumOne.AddTail(pSharedNode);
                                    }
                                    else {
                                        lAccumTwo.AddTail(pSharedNode);
                                    }
                                    ++iSPCount;
                                }
                            }
                        }
                    }
                }
            }
            if (BSShaderManager::GetShadowSceneNode(0)->HasVisibleUnboundSpace()) {
                NiTListItem<BSPortal*>* kIter = pPortalGraph->QPortalList().m_pkHead;
                while (kIter) {
                    BSPortal* pPortal = pPortalGraph->QPortalList().GetNext(kIter);
                    if (pPortal) {
                        if (pCuller->AddShared(pPortal->QSharedGeometryNode())) {
                            BSPortalSharedNode* pSharedNode = pPortal->QSharedGeometryNode();
                            if (pSharedNode) {
                                if (iSPCount % 2) {
                                    lAccumOne.AddTail(pSharedNode);
                                }
                                else {
                                    lAccumTwo.AddTail(pSharedNode);
                                }
                                ++iSPCount;
                            }
                        }
                    }
                }
            }
            BSPortalGraph* pBSPortalGraph = TES::pInstance->GetCurrentCell()->GetPortalGraph();
            if (pBSPortalGraph) {
                for (uint32_t i = 0; i < pBSPortalGraph->GetAlwaysRenderArrayCount(); ++i) {
                    NiAVObject* pkChild = pBSPortalGraph->GetAlwaysRenderAt(i);
                    if (pkChild) {
                        lAccumThree.AddTail(pkChild);
                    }
                }
            }
        }
        else if (pNode && !pNode->GetAppCulled()) {
            for (int32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                pAVObj = pNode->GetAt(i);
                if (pAVObj) {
                    NiNode* pCNode = pAVObj->IsNode();
                    if (pCNode && !pCNode->GetAppCulled()) {
                        for (int32_t j = 0; j < pCNode->GetArrayCount(); ++j) {
                            NiAVObject* pChObj = pCNode->GetAt(j);
                            if (pChObj) {
                                NiNode* pChNode = pChObj->IsNode();
                                if (pChNode) {
                                    if (!pChNode->GetAppCulled()) {
                                        for (int32_t k = j == 6; k < pChNode->GetArrayCount(); ++k) {
                                            NiAVObject* pkGrandChild = pChNode->GetAt(k);
                                            if (pkGrandChild && !pkGrandChild->NiAVObject::GetAppCulled()) {
                                                if (i % 2) {
                                                    lAccumOne.AddTail(pkGrandChild);
                                                }
                                                else {
                                                    lAccumTwo.AddTail(pkGrandChild);
                                                }
                                            }
                                        }
                                    }
                                }
                                else {
                                    if (i % 2) {
                                        lAccumOne.AddTail(pChObj);
                                    }
                                    else {
                                        lAccumTwo.AddTail(pChObj);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        pAVObj = pSSN->GetAtChecked(4u);
        if (pAVObj && !pAVObj->GetAppCulled()) {
            lAccumThree.AddTail(pAVObj);
        }

        pAVObj = pSSN->GetAtChecked(5u);
        if (pAVObj && !pAVObj->GetAppCulled()) {
            lAccumThree.AddTail(pAVObj);
        }

        pAVObj = pSSN->GetAtChecked(6u);
        if (pAVObj && !pAVObj->GetAppCulled()) {
            lAccumOne.AddTail(pAVObj);
        }

        for (int32_t i = 7; i < pSSN->GetArrayCount(); ++i) {
            pAVObj = pSSN->GetAt(i);
            if (pAVObj && !pAVObj->GetAppCulled()) {
                if (i % 2) {
                    lAccumOne.AddTail(pAVObj);
                }
                else {
                    lAccumTwo.AddTail(pAVObj);
                }
            }
        }
        pCuller->m_pkCamera = pAccumCamera;
        BSShaderManager::GetShadowSceneNode(0)->PreOnVisible(*pCuller);
        // &pFrustumNode->kCompoundFrustum is an address inside the node and is never null.
        ShadowSceneNode* pFrustumNode = BSShaderManager::GetShadowSceneNode(0);
        BSCompoundFrustum* pCompoundFrustum = &pFrustumNode->kCompoundFrustum;
        pCompoundFrustum->PrethreadOpList();
        spDrawWorldAccum->StartAccumulating(pAccumCamera);
        BSShaderManager::SetAccumulator(spDrawWorldAccum.m_pObject);
        NiRenderer::GetRenderer()->SetSorter(spDrawWorldAccum.m_pObject);
        BSShaderAccumulator* pWorldAccum = spDrawWorldAccum.m_pObject;
        pCompoundFrustum = &BSShaderManager::GetShadowSceneNode(0)->kCompoundFrustum;
        MTRenderingSystem::QInstance()->AddAccumTask(
            pAccumCamera,
            pCompoundFrustum,
            nullptr,
            &lAccumOne,
            nullptr,
            pWorldAccum,
            BSCullingProcess::BSCP_CULL_NORMAL,
            21,
            1
        );
        MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 21);
        BSShaderAccumulator::iBatchRendererIndex = 0;
        BSCullingProcess kCullProc(nullptr);
        kCullProc.m_pkCamera = pAccumCamera;
        kCullProc.SetAccumulator(spDrawWorldAccum.m_pObject);
        kCullProc.pCompoundFrustum = &BSShaderManager::GetShadowSceneNode(0)->kCompoundFrustum;
        BSShaderUtil::AccumulateSceneList(pAccumCamera, lAccumTwo, kCullProc);
        kCullProc.pCompoundFrustum = nullptr;
        BSShaderUtil::AccumulateSceneList(pAccumCamera, lAccumThree, kCullProc);
        MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 21);
        lAccumOne.RemoveAll();
        lAccumTwo.RemoveAll();
    }
    else {
        BSShaderUtil::AccumulateScene(pAccumCamera, pSceneGraph, *pCuller);
    }
    BSShaderAccumulator* pWorldAccum = spDrawWorldAccum.m_pObject;
    BSShaderUtil::RenderScenePreResolveDepth(pSceneGraph->GetCamera(), pWorldAccum, true);
    if (abIssueVATSPartialQuery) {
        BSRenderState::SetColorWriteEnable(0, BSRS_LOCK);
        Interface::GetVATSEffect()->DrawTargetToTexture(
            nullptr,
            FOVATSEffectManager::VATS_OCCLUSION_QUERY_PARTIAL,
            true,
            FOVATSEffectManager::VATS_DRAW_SELECTED_PARTS,
            NiRenderer::CLEAR_NONE,
            false
        );
        BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
    }
    pWorldAccum = spDrawWorldAccum.m_pObject;
    BSShaderUtil::RenderScenePostResolveDepth(pSceneGraph->GetCamera(), pWorldAccum, true);
    pCuller->SetAccumulator(nullptr);
    kTempMemContext.Leave();
    spDrawWorldAccum->bWorldGeometry = false;
    spDrawWorldAccum->bDoDepthPasses = false;
    BSShaderBloodSplatterProperty::ResetShapeCount();
    BSCullingProcess* pSplatterCuller = Main::GetWorldSceneGraph()->pCuller;
    pSplatterCuller->SetCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    BSShaderBloodSplatter::ComputeScreenSpaceLightLoc(Main::spWorldRoot->GetCamera());
    pSplatterCuller->m_pkCamera = ScreenSplatter::GetCamera();
    pSplatterCuller->SetAccumulator(spScreenSplatterAccum.m_pObject);
    NiNode* pRoot = ScreenSplatter::GetRoot();
    NiCamera* pCamera = ScreenSplatter::GetCamera();
    BSShaderUtil::AccumulateScene(pCamera, pRoot, *pSplatterCuller);
    BSShaderAccumulator* pSplatterAccum = spScreenSplatterAccum.m_pObject;
    BSShaderUtil::RenderScene(ScreenSplatter::GetCamera(), pSplatterAccum, false);
    pCuller->SetAccumulator(nullptr);
    pSplatterCuller->SetCullMode(BSCullingProcess::BSCP_CULL_NORMAL);
    BSShaderBloodSplatterProperty::ResetShapeCount();
    pSplatterCuller = Main::GetWorldSceneGraph()->pCuller;
    pSplatterCuller->SetCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    BSShaderBloodSplatter::ComputeScreenSpaceLightLoc(Main::spWorldRoot->GetCamera());
    pSplatterCuller->m_pkCamera = ScreenCustomSplatter::GetCamera();
    pSplatterCuller->SetAccumulator(spScreenSplatterAccum.m_pObject);
    pRoot = ScreenCustomSplatter::GetCurrentRoot();
    pCamera = ScreenCustomSplatter::GetCamera();
    BSShaderUtil::AccumulateScene(pCamera, pRoot, *pSplatterCuller);
    pSplatterAccum = spScreenSplatterAccum.m_pObject;
    BSShaderUtil::RenderScene(ScreenCustomSplatter::GetCamera(), pSplatterAccum, false);
    pCuller->SetAccumulator(nullptr);
    pSplatterCuller->SetCullMode(BSCullingProcess::BSCP_CULL_NORMAL);
    if (iNumHWThreads.Int() > 1) {
        AILinearTaskThreadManager::SetMainRendering(false);
    }
    if (BSShaderManager::bFakeFullScreenMotionBlur) {
        ImageSpaceEffectFullScreenBlur::CheckCamera(pSceneGraph->GetCamera());
    }
    if (psun) {
        psun->CalculateSunGlare(1u, spDrawWorldAccum.m_pObject, Main::spWorldRoot->GetCamera());
    }
    BSShaderAccumulator::SetRunBoundingSphereTests(false, false);
    pPlayer->Get3D(true)->SetAppCulled(b1stCull);
}

void Main::DrawWorld_Add1stPersonToDepthBuffer(BSRenderedTexture* apRenderTexture) {
    ImageSpaceManager* pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
    NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
    NiNode* p1stPersonNode = static_cast<NiNode*>(pPlayer->Get3D(true));
    SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
    BSCullingProcess* pCuller = pSceneGraph->pCuller;
    bool bIronSights = pPlayer->GetIronSights();
    NiColorA kSavedBackground(0.0f, 0.0f, 0.0f, 0.0f);
    if (bIronSights) {
        ImageSpaceEffectDepthOfField* pDOFEffect = static_cast<ImageSpaceEffectDepthOfField*>(
            pImageSpaceManager->GetEffect(ImageSpaceManager::EFFECT_DEPTH_OF_FIELD)
        );
        BSRenderedTexture* p1stPersonBuffer = pDOFEffect->BorrowMaskTexture();
        BSRenderedTexture::StopOffscreen();
        pRenderer->GetBackgroundColor(kSavedBackground);
        pRenderer->SetBackgroundColor(NiColor::BLACK);
        BSRenderedTexture::StartOffscreen(1u, p1stPersonBuffer->GetGroup());
    }
    else {
        BSRenderState::SetColorWriteEnable(0, BSRS_LOCK);
    }
    bool bWas1stPersonCulled = p1stPersonNode->GetAppCulled();
    p1stPersonNode->SetAppCulled(false);
    float f1stPersonFOV = pPlayer->Get1stPersonFOV();
    pSceneGraph->SetCameraFOV(f1stPersonFOV, false, nullptr, false);
    BSShaderManager::SetFOV(pPlayer->Get1stPersonFOV());
    pCuller->SetAccumulator(spDraw1stPersonDepthAccum.m_pObject);
    BSShaderUtil::AccumulateScene(pSceneGraph->GetCamera(), p1stPersonNode, *pCuller);
    BSShaderAccumulator* pDepthAccum = spDraw1stPersonDepthAccum.m_pObject;
    BSShaderUtil::RenderScene(pSceneGraph->GetCamera(), pDepthAccum, false);
    pCuller->SetAccumulator(nullptr);
    float fWorldFOV = pPlayer->GetWorldFOV();
    pSceneGraph->SetCameraFOV(fWorldFOV, false, nullptr, false);
    BSShaderManager::SetFOV(pPlayer->GetWorldFOV());
    p1stPersonNode->SetAppCulled(bWas1stPersonCulled);
    if (bIronSights) {
        pRenderer->SetBackgroundColorAlpha(kSavedBackground);
        BSRenderedTexture::StopOffscreen();
    }
    else {
        BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
    }
}

void Main::DrawWorld_Refraction(BSShaderAccumulator* apShaderAccum, BSRenderedTexture* apRenderTexture) {
    AutoMemContext kTempMemContext(
        MEM_CONTEXT_IMAGE_SPACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
        0x20D5
    );
    apShaderAccum->RenderNormals(Main::GetWorldSceneGraph()->GetCamera(), apRenderTexture);
}

void Main::DrawWorld_Setup1stPerson() {
    FOPipboyManager* pRenderedMenu;
    NiTransform kCameraTransform;
    AutoMemContext
        context(MEM_CONTEXT_PLAYER, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 0x21B1);
    SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
    NiCamera* pRootCamera = Main::WorldRootCamera();
    NiNode* pPlayerNode = static_cast<NiNode*>(pPlayer->Get3D(true));
    pPlayerNode->SetAppCulled(false);
    if (BSShaderManager::bLetterbox) {
        NiRenderer* pRenderer = NiRenderer::GetRenderer();
        bool bDefaultTarget = true;
        if (pRenderer->GetCurrentRenderTargetGroup()) {
            NiRenderer* pCurrentRenderer = NiRenderer::GetRenderer();
            NiRenderTargetGroup* pDefaultGroup = NiRenderer::GetRenderer()->GetDefaultRenderTargetGroup();
            bDefaultTarget = pCurrentRenderer->GetCurrentRenderTargetGroup() == pDefaultGroup;
        }
        if (bDefaultTarget) {
            Main::WorldRootCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
        }
    }
    float f1stPersonFOV = pPlayer->Get1stPersonFOV();
    pSceneGraph->SetCameraFOV(f1stPersonFOV, false, nullptr, false);
    if (((pPlayer->Get3D(false)->GetAppCulled() || !pPlayer->Get3D(true)->GetAppCulled()) &&
         (!pPlayer->Get3D(false)->GetAppCulled() || pPlayer->Get3D(true)->GetAppCulled())) ||
        appMain->bFlyCamera) {
        Main::bOffsetViewmodelLights = false;
    }
    else {
        NiPointLight* pLight;
        NiPoint3 kInvertedPos = -pPlayerNode->GetWorldTranslate();
        NiUpdateData kUpdateData(0.0f, false, false);
        Main::bOffsetViewmodelLights = true;
        float fMaxFarNearRatio = pRootCamera->m_fMaxFarNearRatio;
        sp1stPersonAdjustedCamera->SetMaxFarNearRatio(fMaxFarNearRatio);
        const NiFrustum* pViewFrustum = &pRootCamera->GetViewFrustum();
        sp1stPersonAdjustedCamera->SetViewFrustum(*pViewFrustum);
        sp1stPersonAdjustedCamera->SetViewPort(pRootCamera->GetViewPort());
        kCameraTransform = pRootCamera->GetWorldTransform();
        sp1stPersonAdjustedCamera->SetWorldTranslate(p1stPersonCamera->GetWorldTranslate() + kInvertedPos);
        sp1stPersonAdjustedCamera->SetWorldRotate(kCameraTransform.m_Rotate);
        if (pPlayer->GetWeaponSightingNode() && !pPlayer->IsReloading() && !pVATS.IsInVats()) {
            NiAVObject* pWeaponSightingNode = pPlayer->GetWeaponSightingNode();
            sp1stPersonAdjustedCamera->SetWorldTranslate(pWeaponSightingNode->GetWorldTranslate() + kInvertedPos);
        }

        NiPoint3 kOrgPos = pPlayerNode->GetTranslate();
        pPlayerNode->SetTranslate(NiPoint3::ZERO);
        pPlayerNode->Update(kUpdateData);
        pPlayerNode->SetTranslate(kOrgPos);
        if (pPlayer->IsPipboyActive()) {
            pLight = Interface::GetPipboy()->GetMenuLight();
        }
        else {
            pLight = nullptr;
        }
        pPlayerNode->SetAlwaysDraw(true);
        if (pLight) {
            pLight->SetWorldTranslate(pLight->GetWorldTranslate() - kInvertedPos);
        }
        NiPoint3 kEyePosition = sp1stPersonAdjustedCamera->GetWorldTranslate();
        BSShaderManager::GetShadowSceneNode(0)->SetEyePosition(kEyePosition);
        NiPoint3 kLightingOffset = kInvertedPos;
        BSShaderManager::GetShadowSceneNode(0)->SetLightingOffset(kLightingOffset);
    }
    if (!Interface::GetCurrentRenderedMenu() ||
        (pRenderedMenu = static_cast<FOPipboyManager*>(Interface::GetCurrentRenderedMenu()),
         pRenderedMenu == Interface::GetPipboy())) {
        BSShaderAccumulator* p1stPersonAccum = spDraw1stPersonAccum.m_pObject;
        MTRenderingSystem::QInstance()->AddAccumTask(
            sp1stPersonAdjustedCamera.m_pObject,
            nullptr,
            pPlayerNode,
            nullptr,
            nullptr,
            p1stPersonAccum,
            BSCullingProcess::BSCP_CULL_NORMAL,
            22,
            0
        );
    }

    MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 22);
}

void Main::DrawWorld_MTRender1stPerson(
    NiDX9Renderer* apRenderer,
    NiGeometry* apRenderedMenuGeom,
    Sun* apSun,
    BSRenderedTexture* apRenderTexture
) {
    FOPipboyManager* pRenderedMenu;
    SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
    BSCullingProcess* pCuller = pSceneGraph->pCuller;
    NiAVObject* p1stPerson3D = pPlayer->Get3D(true);
    p1stPerson3D->SetAppCulled(false);
    NiCamera* pCamera = pSceneGraph->GetCamera();
    NiNode* p1stPersonNode = static_cast<NiNode*>(p1stPerson3D);
    MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 22);
    spDraw1stPersonAccum->bDoDepthPasses = false;
    AutoMemContext kTempMemContext(
        MEM_CONTEXT_PLAYER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
        0x221F
    );
    apRenderer->ClearBuffer(nullptr, 4u);
    // Letterboxed while rendering to the default target group (or none): the framebuffer's viewport.
    NiRenderer* pCurrentRenderer;
    NiRenderer* pDefaultRenderer;
    NiRenderTargetGroup* pDefaultGroup;
    if (BSShaderManager::bLetterbox &&
        (!NiRenderer::GetRenderer()->GetCurrentRenderTargetGroup() ||
         (pCurrentRenderer = NiRenderer::GetRenderer(),
          pDefaultRenderer = NiRenderer::GetRenderer(),
          pDefaultGroup = pDefaultRenderer->GetDefaultRenderTargetGroup(),
          pCurrentRenderer->GetCurrentRenderTargetGroup() == pDefaultGroup))) {
        sp1stPersonAdjustedCamera->SetViewPort(BSShaderManager::kFramebufferViewport);
    }
    else {
        // The viewport is built with NiColorA's constructor: left, right, top, bottom.
        NiRect<float> kViewport;
        ::new (&kViewport) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
        sp1stPersonAdjustedCamera->SetViewPort(kViewport);
    }
    float f1stPersonFOV = pPlayer->Get1stPersonFOV();
    pSceneGraph->SetCameraFOV(f1stPersonFOV, false, nullptr, false);
    BSShaderManager::SetFOV(pPlayer->Get1stPersonFOV());
    const NiFrustum* pViewFrustum = &pCamera->GetViewFrustum();
    sp1stPersonAdjustedCamera->SetViewFrustum(*pViewFrustum);
    if (apRenderedMenuGeom) {
        Interface::GetCurrentRenderedMenu()->SetScreenTexture(nullptr);
        apRenderedMenuGeom->SetAppCulled(false);
        if (Interface::IsInRenderedMenu()) {
            void* pNewAccumulator;
            if (Interface::IsInPipboyMenu() && pPlayer->IsRefractive()) {
                Interface::GetCurrentRenderedMenu()->GetScreenGeometry()->SetAppCulled(true);
            }
            pNewAccumulator = new BSShaderAccumulator(99, 1u, 759u);
            NiPointer<BSShaderAccumulator> spTempAccum(static_cast<BSShaderAccumulator*>(pNewAccumulator));
            pCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
            int32_t iAlphaType = NiAlphaProperty::GetType();
            NiAlphaProperty* pRenderedMenuAlpha = static_cast<NiAlphaProperty*>(
                Interface::GetCurrentRenderedMenu()->GetScreenGeometry()->GetProperty(iAlphaType)
            );
            if (pRenderedMenuAlpha) {
                pRenderedMenuAlpha->SetAlphaBlending(false);
            }
            Interface::GetCurrentRenderedMenu()->SetScreenTexture(nullptr);
            pCuller->SetAccumulator(spTempAccum.m_pObject);
            if (Interface::IsInPipboyMenu() && pPlayer->IsRefractive()) {
                NiAVObject* pButtonGlow = Interface::GetPipboy()->GetButtonGlow(0);
                BSShaderUtil::AccumulateScene(sp1stPersonAdjustedCamera.m_pObject, pButtonGlow, *pCuller);
                pButtonGlow = Interface::GetPipboy()->GetButtonGlow(1u);
                BSShaderUtil::AccumulateScene(sp1stPersonAdjustedCamera.m_pObject, pButtonGlow, *pCuller);
                pButtonGlow = Interface::GetPipboy()->GetButtonGlow(2u);
                BSShaderUtil::AccumulateScene(sp1stPersonAdjustedCamera.m_pObject, pButtonGlow, *pCuller);
            }
            FORenderedMenu* pMenu = Interface::GetCurrentRenderedMenu();
            BSShaderUtil::AccumulateScene(sp1stPersonAdjustedCamera.m_pObject, pMenu->GetScreenGeometry(), *pCuller);
            BSShaderUtil::RenderScene(sp1stPersonAdjustedCamera.m_pObject, spTempAccum.m_pObject, false);
            pCuller->SetAccumulator(nullptr);
            spTempAccum = static_cast<BSShaderAccumulator*>(nullptr);
            pCuller->PopCullMode();
            if (Interface::IsInPipboyMenu() && pPlayer->IsRefractive()) {
                Interface::GetCurrentRenderedMenu()->GetScreenGeometry()->SetAppCulled(false);
            }
        }
    }
    if ((!Interface::GetCurrentRenderedMenu() ||
         (pRenderedMenu = static_cast<FOPipboyManager*>(Interface::GetCurrentRenderedMenu()),
          pRenderedMenu == Interface::GetPipboy())) &&
        (!Interface::IsInPipboyMenu() || !pPlayer->IsRefractive())) {
        BSRenderState::SetColorWriteEnable(15u, BSRS_LOCK);
        MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 22);
        BSShaderUtil::RenderScene(sp1stPersonAdjustedCamera.m_pObject, spDraw1stPersonAccum.m_pObject, false);
        BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
        BSSimpleList<NiPointer<BSTempEffect>>* pTempParticleList = &ReferenceProcessLists.GlobalTempEffectList;
        if (pTempParticleList && !pTempParticleList->IsEmpty()) {
            NiPoint3 kISNodeDiff(0.0f, 0.0f, 0.0f);
            if (pPlayer->GetWeaponSightingNode() && !pPlayer->IsReloading() && !pVATS.IsInVats()) {
                const NiPoint3* p1stPersonCameraPos = &p1stPersonCamera->GetWorldTranslate();
                NiAVObject* pWeaponSightingNode = pPlayer->GetWeaponSightingNode();
                kISNodeDiff = pWeaponSightingNode->GetWorldTranslate() - *p1stPersonCameraPos;
            }

            SceneGraph* pWorldSceneGraph = Main::GetWorldSceneGraph();
            NiCamera* pWorldCamera = pWorldSceneGraph->GetCamera();
            BSCullingProcess* pCullingProcess = pWorldSceneGraph->pCuller;
            NiPointer<BSShaderAccumulator> spTempAccum(spDrawWorldAccum);
            NiPoint3 kCameraPos = pWorldCamera->GetWorldTranslate();
            pWorldCamera->SetWorldTranslate(kCameraPos + kISNodeDiff);
            pCullingProcess->SetAccumulator(spTempAccum.m_pObject);
            spTempAccum->b1stPerson = true;
            bool bShouldRender = false;
            // Walks the list with the same local, taking each node's item before moving on.
            while (pTempParticleList) {
                BSTempEffect* pEffect = pTempParticleList->m_item.m_pObject;
                pTempParticleList = pTempParticleList->m_pkNext;
                if (pEffect && pEffect->IsFirstPerson()) {
                    bShouldRender = true;
                    pEffect->Get3D()->SetAppCulled(false);
                    BSShaderUtil::AccumulateScene(pWorldCamera, pEffect->Get3D(), *pCullingProcess);
                    pEffect->Get3D()->SetAppCulled(true);
                }
            }
            if (bShouldRender) {
                BSShaderUtil::RenderScene(pWorldCamera, spTempAccum.m_pObject, false);
            }
            pCullingProcess->SetAccumulator(nullptr);
            spTempAccum->b1stPerson = false;
            spTempAccum = static_cast<BSShaderAccumulator*>(nullptr);
            pWorldCamera->SetWorldTranslate(kCameraPos);
        }
    }

    if (BSShaderManager::IsRefractionEnabled()) {
        AutoMemContext kImageSpaceMemContext(
            MEM_CONTEXT_IMAGE_SPACE,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
            0x22B5
        );
        if (Interface::IsInPipboyMenu() && BSShaderManager::IsRefractionEnabled() && pPlayer->IsRefractive()) {
            BSRenderedTexture::StopOffscreen();
            BSRenderedTexture::StartOffscreen(6u, apRenderTexture->GetGroup());
            if (BSShaderManager::bLetterbox) {
                Main::WorldRootCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
            }
        }
        spDraw1stPersonAccum->RenderNormals(sp1stPersonAdjustedCamera.m_pObject, apRenderTexture);
        DrawWorld_RestartRenderTexture(apRenderTexture, false, 0);
    }
    if (Main::bOffsetViewmodelLights) {
        NiUpdateData kUpdate(0.0f, false, false);
        p1stPersonNode->Update(kUpdate);
        p1stPersonNode->SetAlwaysDraw(false);
        NiPoint3 kEyePosition = pCamera->m_pkParent->GetTranslate();
        BSShaderManager::GetShadowSceneNode(0)->SetEyePosition(kEyePosition);
        NiPoint3 kLightingOffset = NiPoint3::ZERO;
        BSShaderManager::GetShadowSceneNode(0)->SetLightingOffset(kLightingOffset);
    }
    if (pVATS.eMode != VATS::VATS_PLAYBACK) {
        float fWorldFOV = pPlayer->GetWorldFOV();
        Main::GetWorldSceneGraph()->SetCameraFOV(fWorldFOV, false, nullptr, false);
    }
    BSShaderManager::SetFOV(pPlayer->GetWorldFOV());
    if (apSun && !apSun->GetSunGlareNode()->GetAppCulled()) {
        bool bCull = apSun->GetSunGlareNode()->GetAppCulled();
        apSun->GetSunGlareNode()->SetAppCulled(false);
        SceneGraph* pWorldSceneGraph = Main::GetWorldSceneGraph();
        NiCamera* pWorldCamera = pWorldSceneGraph->GetCamera();
        BSCullingProcess* pCullingProcess = pWorldSceneGraph->pCuller;
        NiPointer<BSShaderAccumulator> spTempAccum(spDrawWorldAccum);
        pCullingProcess->SetAccumulator(spTempAccum.m_pObject);
        spTempAccum->b1stPerson = true;
        BSShaderUtil::AccumulateScene(pWorldCamera, apSun->GetSunGlareNode(), *pCullingProcess);
        BSShaderUtil::RenderScene(pWorldCamera, spTempAccum.m_pObject, false);
        pCullingProcess->SetAccumulator(nullptr);
        spTempAccum->b1stPerson = false;
        spTempAccum = static_cast<BSShaderAccumulator*>(nullptr);
        apSun->GetSunGlareNode()->SetAppCulled(bCull);
    }

    if (apSun) {
        apSun->CalculateSunGlare(2u, spDrawWorldAccum.m_pObject, Main::spWorldRoot->GetCamera());
    }
    f1stPersonFOV = pPlayer->Get1stPersonFOV();
    pSceneGraph->SetCameraFOV(f1stPersonFOV, false, nullptr, false);
    BSShaderManager::SetFOV(pPlayer->Get1stPersonFOV());
}

void Main::DrawWorld_RenderedMenuScene() {
    if (!Main::spVATSTex.m_pObject && Interface::GetCurrentRenderedMenu()) {
        FOPipboyManager* pMenu = static_cast<FOPipboyManager*>(Interface::GetCurrentRenderedMenu());
        if (pMenu != Interface::GetPipboy()) {
            NiCamera* pCamera;
            SceneGraph* pWorldSceneGraph = Main::GetWorldSceneGraph();
            BSCullingProcess* pCullingProcess = pWorldSceneGraph->pCuller;
            pCullingProcess->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
            if (Interface::GetCurrentRenderedMenu()->GetCamera()) {
                pCamera = Interface::GetCurrentRenderedMenu()->GetCamera();
            }
            else {
                pCamera = pWorldSceneGraph->GetCamera();
            }
            pCullingProcess->m_pkCamera = pCamera;
            pCullingProcess->SetAccumulator(spRenderedMenuAccum.m_pObject);
            ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
            spRenderedMenuAccum->SetShadowSceneNode(pShadowSceneNode);
            // Letterboxed while rendering to the default target group (or none): the framebuffer's viewport.
            NiRenderer* pCurrentRenderer;
            NiRenderer* pDefaultRenderer;
            NiRenderTargetGroup* pDefaultGroup;
            if (BSShaderManager::bLetterbox &&
                (!NiRenderer::GetRenderer()->GetCurrentRenderTargetGroup() ||
                 (pCurrentRenderer = NiRenderer::GetRenderer(),
                  pDefaultRenderer = NiRenderer::GetRenderer(),
                  pDefaultGroup = pDefaultRenderer->GetDefaultRenderTargetGroup(),
                  pCurrentRenderer->GetCurrentRenderTargetGroup() == pDefaultGroup))) {
                pCamera->SetViewPort(BSShaderManager::kFramebufferViewport);
            }
            else {
                // The viewport is built with NiColorA's constructor: left, right, top, bottom.
                NiRect<float> kViewport;
                ::new (&kViewport) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
                pCamera->SetViewPort(kViewport);
            }
            BSShaderUtil::AccumulateScene(
                pCamera,
                Interface::GetCurrentRenderedMenu()->GetParentNodeToRender(),
                *pCullingProcess
            );
            BSShaderManager::SetRenderMode(15u);
            BSRenderState::SetZWriteEnable(false, BSRS_LOCK);
            BSRenderState::SetColorWriteEnable(8u, BSRS_LOCK);
            BSShaderUtil::RenderScene(pCamera, spRenderedMenuAccum.m_pObject, false);
            BSShaderManager::SetRenderMode(0);
            BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
            BSRenderState::RestoreZWriteEnable(BSRS_LOCK);
            BSShaderUtil::AccumulateScene(
                pCamera,
                Interface::GetCurrentRenderedMenu()->GetParentNodeToRender(),
                *pCullingProcess
            );
            BSShaderUtil::RenderScene(pCamera, spRenderedMenuAccum.m_pObject, false);
            pCullingProcess->SetAccumulator(nullptr);
            pCullingProcess->PopCullMode();
        }
    }
}

void Main::DrawWorld_DoDepth(BSShaderAccumulator* apShaderAccum, BSRenderedTexture* apRenderTexture) {
    SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
    if (pVATS.eMode != VATS::VATS_PLAYBACK) {
        float fWorldFOV = pPlayer->GetWorldFOV();
        pSceneGraph->SetCameraFOV(fWorldFOV, false, nullptr, false);
    }
    BSShaderManager::SetFOV(pPlayer->GetWorldFOV());
    NiCloningProcess kClone(257u);
    NiCamera* p1stPersonCam = static_cast<NiCamera*>(pSceneGraph->GetCamera()->CreateClone(kClone));
    if (pVATS.eMode != VATS::VATS_PLAYBACK) {
        float f1stPersonFOV = pPlayer->Get1stPersonFOV();
        pSceneGraph->SetCameraFOV(f1stPersonFOV, false, p1stPersonCam, false);
    }
    AutoMemContext kTempMemContext(
        MEM_CONTEXT_IMAGE_SPACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
        0x234C
    );
    apShaderAccum->SetupDepth(pSceneGraph->GetCamera(), p1stPersonCam, apRenderTexture);
    if (p1stPersonCam) {
        delete p1stPersonCam;
    }
}

int32_t Main::DrawWorld_RenderedMenu(void* apPipboyGeom, NiDX9Renderer* apRenderer) {
    float fMouseX;
    float fMouseY;
    uint32_t uiMenuToSet;
    NiTransform k1stNodeTransform;
    SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
    BSCullingProcess* pCuller = pSceneGraph->pCuller;
    NiCamera* pCam = Interface::GetCurrentRenderedMenu()->GetCamera();
    apRenderer->ClearBuffer(nullptr, 4u);
    NiNode* p1stPersonNode = static_cast<NiNode*>(
        Interface::GetCurrentRenderedMenu()->FORenderedMenu::GetParentNodeToRender()
    );
    pCuller->SetCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    NiUpdateData kUpdate(0.0f, false, false);
    NiTransform kCamTransform;
    // Keeps only the low byte of GetAlwaysDraw's result and passes it back to SetAlwaysDraw zero-extended.
    uint8_t bAlwaysDraw = static_cast<uint8_t>(p1stPersonNode->GetAlwaysDraw());
    NiCamera* pPipboyCam = pCam ? pCam : pSceneGraph->GetCamera();
    if (pPlayer->IsPipboyActive()) {
        k1stNodeTransform = *&p1stPersonNode->m_kLocal;
        p1stPersonNode->SetTranslate(NiPoint3::ZERO);
        p1stPersonNode->Update(kUpdate);
        p1stPersonNode->SetTranslate(k1stNodeTransform.m_Translate);
        kCamTransform.MakeIdentity();
        if (pPipboyCam && pPipboyCam->m_pkParent) {
            kCamTransform = *&pPipboyCam->m_pkParent->m_kLocal;
            const NiPoint3* p1stPersonCameraPos = &p1stPersonCamera->GetWorldTranslate();
            pPipboyCam->m_pkParent->SetTranslate(*p1stPersonCameraPos);
            pPipboyCam->m_pkParent->Update(kUpdate);
        }

        NiPoint3 kLightingOffset = -p1stPersonNode->GetWorldTranslate();
        BSShaderManager::GetShadowSceneNode(0)->SetLightingOffset(kLightingOffset);
    }
    if (Interface::IsInPipboyMenu()) {
        uiMenuToSet = 0;
        char cLeftDepressed = GetControls()->GetMouseButton(0, 2) > 0;
        char cLeftPressed = GetControls()->GetMouseButton(0, 1) > 0;
        uint8_t ucOverPipBoyScreen = Interface::GetPipboy()->CheckOrangeButtonsClicked(
            cLeftPressed,
            cLeftDepressed,
            &fMouseX,
            &fMouseY,
            &uiMenuToSet
        );
        Interface::SetPipboyPickResults(ucOverPipBoyScreen, fMouseX, fMouseY, uiMenuToSet);
    }
    if (!pCam) {
        pCam = pSceneGraph->GetCamera();
    }

    if (!pCam) {
        pCam = sp1stPersonAdjustedCamera.m_pObject;
    }
    NiRect<float> kViewPort = pCam->GetViewPort();
    if (BSShaderManager::bLetterbox) {
        NiRenderer* pCurrentRenderer = NiRenderer::GetRenderer();
        NiRenderTargetGroup* pDefaultGroup = NiRenderer::GetRenderer()->GetDefaultRenderTargetGroup();
        if (pCurrentRenderer->GetCurrentRenderTargetGroup() == pDefaultGroup) {
            pCam->SetViewPort(BSShaderManager::kFramebufferViewport);
        }
    }
    BSRenderState::SetColorWriteEnable(0, BSRS_LOCK);
    pCuller->m_pkCamera = pCam;
    pCuller->SetAccumulator(spDraw1stPersonAccum.m_pObject);
    ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
    spDraw1stPersonAccum->SetShadowSceneNode(pShadowSceneNode);
    BSShaderUtil::AccumulateScene(pCam, p1stPersonNode, *pCuller);
    BSShaderUtil::RenderScene(pCam, spDraw1stPersonAccum.m_pObject, false);
    spDraw1stPersonAccum->ClearGroupPasses(6);
    pCuller->SetAccumulator(nullptr);
    BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
    int32_t iAlphaType = NiAlphaProperty::GetType();
    NiAlphaProperty* pPipboyAlpha = static_cast<NiAlphaProperty*>(
        Interface::GetCurrentRenderedMenu()->GetScreenGeometry()->GetProperty(iAlphaType)
    );
    if (pPipboyAlpha) {
        pPipboyAlpha->SetAlphaBlending(true);
        pPipboyAlpha->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);
        pPipboyAlpha->SetDestBlendMode(NiAlphaProperty::ALPHA_ONE);
        Interface::GetCurrentRenderedMenu()->GetScreenGeometry()->UpdateProperties();
    }
    NiRenderedTexture* pInterfaceTexture = Main::spInterfaceFXTex->GetTexture(0);
    Interface::GetCurrentRenderedMenu()->SetScreenTexture(pInterfaceTexture);
    pCuller->m_pkCamera = pCam;
    pCuller->SetAccumulator(spRenderedMenuAccum.m_pObject);
    BSRenderState::SetZWriteEnable(false, BSRS_LOCK);
    if (pPlayer->IsRefractive() && Interface::IsInPipboyMenu()) {
        BSShaderUtil::AccumulateScene(pCam, Interface::GetPipboy()->GetButtonGlow(0), *pCuller);
        BSShaderUtil::AccumulateScene(pCam, Interface::GetPipboy()->GetButtonGlow(1u), *pCuller);
        BSShaderUtil::AccumulateScene(pCam, Interface::GetPipboy()->GetButtonGlow(2u), *pCuller);
        BSRenderState::SetZEnable(false, BSRS_LOCK);
    }
    BSShaderUtil::AccumulateScene(pCam, Interface::GetCurrentRenderedMenu()->GetScreenGeometry(), *pCuller);
    BSShaderUtil::RenderScene(pCam, spRenderedMenuAccum.m_pObject, false);
    BSRenderState::RestoreZWriteEnable(BSRS_LOCK);
    if (pPlayer->IsRefractive() && Interface::IsInPipboyMenu()) {
        BSRenderState::RestoreZEnable(BSRS_LOCK);
    }
    pCuller->SetAccumulator(nullptr);
    Interface::GetCurrentRenderedMenu()->SetScreenTexture(nullptr);
    if (pPipboyAlpha) {
        pPipboyAlpha->SetAlphaBlending(false);
    }
    pCam->SetViewPort(kViewPort);
    int32_t iPipboyActive = pPlayer->IsPipboyActive();
    if (static_cast<uint8_t>(iPipboyActive)) {
        p1stPersonNode->Update(kUpdate);
        p1stPersonNode->SetAlwaysDraw(bAlwaysDraw);
        if (pPipboyCam && pPipboyCam->m_pkParent) {
            pPipboyCam->m_pkParent->SetTranslate(kCamTransform.m_Translate);
            pPipboyCam->m_pkParent->Update(kUpdate);
        }

        NiPoint3 kLightingOffset = NiPoint3::ZERO;
        BSShaderManager::GetShadowSceneNode(0)->SetLightingOffset(kLightingOffset);
    }
    pCuller->SetCullMode(BSCullingProcess::BSCP_CULL_NORMAL);
    return iPipboyActive;
}

// Resets the game world: stops the player's sounds, detaches both of the player's 3D roots, clears the loaded cells,
// obstacles and garbage, reverts every change and reinitialises, optionally opening the start menu.
void Main::PerformGameReset(bool abCreateMainMenu) {
    bool bonIdle = bOnIdle;
    bOnIdle = false;
    pPlayer->StopHeartbeatSound();
    bool bResetAllowChanges = pSaveLoadGame->SetGlobalAllowChanges(false);
    // Every sound flag: all sounds end.
    BSAudio::QInstance().EndAllSoundsOfType(0xFFFFFFFFu);
    FalloutRadio::PipboyRadioEnable(false);
    if (pPlayer->GetParentCell()) {
        pPlayer->GetParentCell()->RemoveReference(pPlayer);
    }
    NiAVObject* p3D = pPlayer->Get3D(true);
    NiNode* pNode = p3D ? p3D->IsNode() : nullptr;
    if (pNode && pNode->m_pkParent) {
        pNode->m_pkParent->DetachChild(p3D);
    }

    p3D = pPlayer->Get3D(false);
    pNode = p3D ? p3D->IsNode() : nullptr;
    if (pNode && pNode->m_pkParent) {
        pNode->m_pkParent->DetachChild(p3D);
    }

    TES::pInstance->ClearAllCells(false, false);
    // INT32_MAX: no current grid cell, as TES's constructor sets it.
    TES::pInstance->iCurrentGridX = INT32_MAX;
    TES::pInstance->iCurrentGridY = INT32_MAX;
    NavMeshObstacleManager::GetInstance()->ClearAllObstacles();
    GarbageCollector::ClearAll(false);
    NiObjectGarbage::ClearAll(false);
    pPlayer->Set3D(nullptr, false);
    pSaveLoadGame->RevertAllChanges();
    ReferenceProcessLists.InitPackageLocations();
    pDataHandler->UpdateAutoCalcNPCs();
    BSShaderManager::GetAccumulator()->ClearAllQueries();
    InitializationFlow();
    Interface::CloseAllSupportMenus(false);
    TES::pInstance->CleanupTemporaryParticleCache();
    if (abCreateMainMenu) {
        Interface::CreateStartMenu();
    }
    pSaveLoadGame->SetGlobalAllowChanges(bResetAllowChanges);
    appMain->SetGame();
    appMain->bOnIdle = bonIdle;
}

TESObjectREFR* Main::GetDebugTextRef() {
    return pDebugTextReference;
}

bool Main::GetSceneIsComplex() {
    return bSceneIsComplex;
}

bool Main::GetOpenConsole() {
    return bOpenConsole;
}

NiNode* Main::LandLODNode() {
    return pLandLOD;
}

NiNode* Main::ObjectLODNode() {
    return pObjectLOD;
}

float Main::QFrameAnimTime() {
    return fFrameAnimTime;
}

BSParticleSystemManager* Main::GetMasterParticleSystemManager() {
    return spParticleSystemManager;
}

SceneGraph* Main::GetWorldSceneGraph() {
    return static_cast<SceneGraph*>(spWorldRoot.m_pObject);
}

NiCamera* Main::WorldRootCamera() {
    return spWorldRoot->GetCamera();
}

NiNode* Main::WeatherRootNode() {
    return spWeatherRoot;
}

SceneGraph* Main::MenuRootNode() {
    return static_cast<SceneGraph*>(spMenuRoot.m_pObject);
}

SceneGraph* Main::MenuRootNode3D() {
    return static_cast<SceneGraph*>(sp3DMenuRoot.m_pObject);
}

void Main::CreateMenuRootNode() {
    SceneGraph* pSceneGraph = new SceneGraph("Menu", true, nullptr);
    spMenuRoot = pSceneGraph;
}

void Main::CreateMenuRootNode3D() {
    SceneGraph* pSceneGraph = new SceneGraph("3DMenu", false, nullptr);
    sp3DMenuRoot = pSceneGraph;
}

// AutoMemContext as in UpdateOffscreenInterface.
void Main::InitSky() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext)
        AutoMemContext(MEM_CONTEXT_SKY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 9917);
    InitializationFlow("Initializing Sky...");
    BSFogProperty* pFog = spFog;
    NiNode* pSkyRoot = spSkyRoot;
    pSky->Initialize(static_cast<BSMultiBoundNode*>(pSkyRoot), pFog);
    NiNode* pWeatherRoot = spWeatherRoot;
    pWeatherRoot->AttachChild(pSky->GetSun()->GetSunGlareNode(), true);
    NiDirectionalLight* pSunLight = pSky->GetLight();
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->SetSunLight(pSunLight);
    spSkyRoot->UpdateProperties();
    bool bSunbeams = bUseSunbeams.Bool();
    BSShaderManager::GetImageSpaceManager()->SetUseSunbeams(bSunbeams);
    if (bUseSunbeams.Bool()) {
        NiStencilProperty* pStencil = new NiStencilProperty;
        pStencil->SetStencilOn(true);
        pStencil->SetStencilPassAction(NiStencilProperty::ACTION_REPLACE);
        pStencil->SetStencilReference(0xFF);
        spSkyRoot->AttachProperty(pStencil);
    }
    NiUpdateData kSkyUpdate(0.0f, false, false);
    spSkyRoot->Update(kSkyUpdate);
    spWeatherRoot->UpdateProperties();
    NiUpdateData kWeatherUpdate(0.0f, false, false);
    spWeatherRoot->Update(kWeatherUpdate);
    BSShaderManager::PrepareObject(spSkyRoot, false, false);
    BSShaderManager::PrepareObject(pSky->GetSun()->GetSunGlareNode(), false, false);
    pContext->~AutoMemContext();
}

void Main::ClearDebugTextRef() {
    Main::pDebugTextReference = nullptr;
}

void Main::SetOpenConsole(bool abValue) {
    Main::bOpenConsole = abValue;
}

void Main::DestroyInterfaceSceneGraph() {
    Main::spMenuRoot = static_cast<NiNode*>(nullptr);
}

void Main::DestroyInterface3DSceneGraph() {
    Main::sp3DMenuRoot = static_cast<NiNode*>(nullptr);
}

bool Main::ResetDeviceCallback(bool abBeforeReset, [[maybe_unused]] void* apData) {
    if (abBeforeReset) {
        while (!LoadingMenuThread::GetByte49()) {
            BSSleep(5u);
        }
        if (pMoviePlayer->GetPlayingSequence()) {
            pMoviePlayer->StopSequence();
            pMoviePlayer->WaitForSequenceToEnd();
        }
        if (static_cast<BGSMoviePlayer*>(pMoviePlayer)->BGSMoviePlayer::IsPlaying()) {
            pMoviePlayer->Stop();
        }
        BSShaderAccumulator* pAccumulator = BSShaderManager::GetAccumulator();
        if (pAccumulator) {
            pAccumulator->ClearAllQueries();
        }
        int32_t iDimension = TES::pInstance->pGridCellA->iDimension;
        for (int32_t iX = 0; iX < iDimension; ++iX) {
            for (int32_t iY = 0; iY < iDimension; ++iY) {
                TESObjectCELL* pCell = TES::pInstance->GetGridCell(iX, iY)->pCell;
                if (pCell) {
                    _D3DLOCKED_RECT* pOutRect;
                    NiRenderedTexture* pOutTex;
                    pCell->GetCanopyShadowMask(pOutTex, pOutRect);
                    if (pOutTex && pOutRect && pOutRect->pBits) {
                        pOutRect->pBits = nullptr;
                    }
                }
            }
        }
        appMain->KillMenuBGTexture();
        // A full-screen window is not re-fitted here: it belongs to the device (Direct3D 9's on Windows, dxvk's SDL
        // backend elsewhere), so it is left as it is.
    }
    else {
        // The HDR effect's Shutdown is called with no object: its body ignores this.
        if (BSShaderManager::bHDR) {
            ImageSpaceEffectHDR::ClearAdaptedLight();
        }
        if (TES::pInstance->GetWaterSystem() && TESWaterSystem::GetWaterEnabled()) {
            TES::pInstance->GetWaterSystem()->DisableWaterSystem(true, false);
            TES::pInstance->GetWaterSystem()->EnableWaterSystem();
        }
    }
    return true;
}

bool Main::LostDeviceCallback([[maybe_unused]] void* apData) {
    return true;
}

void Main_InitStartCell(char* pCellName, NiPoint3* apPosition) {
    int32_t iY;
    int32_t iX;
    TESObjectCELL* pCell = nullptr;
    char cStartSpecified = 1;
    char cNotPlaced = 1;
    if ((pCellName && BSstrlen(pCellName)) || std::strlen(sStartingCell.String())) {
        if (pCellName && BSstrlen(pCellName)) {
            pCell = pDataHandler->GetCellByEditorID(pCellName);
        }
        else {
            const char* pStartingCell = sStartingCell.String();
            pCell = pDataHandler->GetCellByEditorID(pStartingCell);
        }
        if (!pCell) {
            iX = 0;
            iY = 0;
            const char* pStartingCell = sStartingCell.String();
            TESWorldSpace* pWorldSpace = pDataHandler->GetExtCellDataFromFileByEditorID(pStartingCell, iX, iY);
            if (pWorldSpace) {
                TES::pInstance->SetWorldSpace(pWorldSpace);
                apPosition->x = static_cast<double>(iX << 12) + 2048.0;
                apPosition->y = static_cast<double>(iY << 12) + 2048.0;
                apPosition->z = 0.0f;
                TES::pInstance->LeaveInterior(apPosition);
                pCell = pDataHandler->GetCellFromCellCoord(iX, iY, pWorldSpace, false);
                cNotPlaced = 0;
            }
        }
    }
    else if (std::strlen(sStartingCellX.String()) && std::strlen(sStartingCellY.String())) {
        int32_t iCellX = std::atoi(sStartingCellX.String());
        int32_t iCellY = std::atoi(sStartingCellY.String());
        TESWorldSpace* pWorldSpace = static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item);
        for (BSSimpleList<TESWorldSpace*>* pIter = pDataHandler->GetWorldSpaceList(); pIter && !pIter->IsEmpty();
             pIter = pIter->m_pkNext) {
            TESWorldSpace* pWorld = pIter->m_item;
            const char* pStartingWorld = sStartingWorld.String();
            if (!BSstricmp(pWorld->GetFormEditorID(), pStartingWorld)) {
                pWorldSpace = pWorld;
                break;
            }
        }
        if (pWorldSpace) {
            pCell = pWorldSpace->LoadCell(iCellX, iCellY);
            if (!pCell) {
                pCell = pDataHandler->GetCellFromCellCoord(iCellX, iCellY, pWorldSpace, true);
            }
        }
    }
    else {
        cStartSpecified = 0;
        cNotPlaced = 0;
    }
    if (cNotPlaced) {
        if (pCell) {
            if (pCell->GetInterior()) {
                TES::pInstance->LoadInterior(pCell, apPosition);
            }
            else {
                TESWorldSpace* pWorldSpace = pCell->GetWorldSpace();
                TES::pInstance->SetWorldSpace(pWorldSpace);
                apPosition->x = static_cast<double>(pCell->GetDataX() << 12) + 2048.0;
                apPosition->y = static_cast<double>(pCell->GetDataY() << 12) + 2048.0;
                apPosition->z = 0.0f;
                TES::pInstance->LeaveInterior(apPosition);
            }
            cNotPlaced = 0;
        }
        else if (cStartSpecified) {
            if (sStartingCell.String() && *sStartingCell.String()) {
                Warning("CELLS: Could not find starting cell '%s'.", sStartingCell.String());
            }
            else if (
                sStartingCellX.String() &&
                sStartingCellY.String() &&
                *sStartingCellX.String() &&
                *sStartingCellY.String()
            ) {
                if (sStartingWorld.String() && *sStartingWorld.String()) {
                    const char* pStartingWorld = sStartingWorld.String();
                    const char* pStartingCellY = sStartingCellY.String();
                    Warning(
                        "CELLS: Could not find starting cell (%s, %s) in worldspace '%s'.",
                        sStartingCellX.String(),
                        pStartingCellY,
                        pStartingWorld
                    );
                }
                else {
                    const char* pStartingCellY = sStartingCellY.String();
                    Warning(
                        "CELLS: Could not find starting cell (%s, %s) in default Tamriel worldspace",
                        sStartingCellX.String(),
                        pStartingCellY
                    );
                }
            }
            else {
                Warning("CELLS: Could not find starting cell for INI data.");
            }
        }
        pDataHandler->SetMasterFileLargeBuffer(-1);
        if (cNotPlaced) {
            TES::pInstance->InitGridCells(apPosition, true);
            TES::pInstance->pGridCellA->AttachAllLand3D();
        }
    }
}

void Main::InitPlayerStart(NiPoint3 position) {
    NiMatrix3 kOrientation;
    TESObjectCELL* pCell = TES::pInstance->pInteriorCell;
    NiPoint3 kRotation = NiPoint3::ZERO;
    if (!pCell) {
        int32_t iGridY = uGridsToLoad.UInt() >> 1;
        int32_t iGridX = uGridsToLoad.UInt() >> 1;
        pCell = TES::pInstance->GetGridCell(iGridX, iGridY)->pCell;
    }
    if (pCell) {
        pCell->GetCOCPlacementInfo(position, kRotation);
    }
    pPlayer->SetHeading(kRotation.z);
    pPlayer->SetPosition(position);
    if (pCell) {
        pCell->AddReference(pPlayer, false);
        pIOManager->LoadAllQueuedPriorities();
        AutoMemContext kMemContext(
            MEM_CONTEXT_PLAYER,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
            0x2612
        );
        pPlayer->Get3D(false)->UpdateProperties();
        NiAVObject* p3D = pPlayer->Get3D(false);
        BSShaderManager::GetShadowSceneNode(0)->AddShadowLight(p3D);
    }
    pPlayer->InitScript();
    Main::GetWorldSceneGraph()->GetCameraRoot()->SetTranslate(position);
    ::new (&kOrientation) NiMatrix3(pPlayer->GetOrientation());
    Main::GetWorldSceneGraph()->GetCameraRoot()->SetRotate(kOrientation);
    NiUpdateData kUpdateData(0.0f, false, false);
    Main::GetWorldSceneGraph()->GetCameraRoot()->Update(kUpdateData);
}

void Main::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BGSDestructibleObjectForm::SetLastUpdateTime(float afTime) {
    fLastTime = afTime;
}

void BGSTerrainManager::SetTreeNode(NiNode* apTreeNode) {
    BGSTerrainManager::spTreeNode = apTreeNode;
}

void BGSTerrainManager::DeallocateStaticNodes() {
    BGSTerrainManager::spTreeNode = static_cast<NiNode*>(nullptr);
    BGSTerrainManager::spRootLandLODNode = static_cast<NiNode*>(nullptr);
    BGSTerrainManager::spRootObjectLODNode = static_cast<NiNode*>(nullptr);
}

void BSFaceGenManager::SetCamera(NiCamera* apCamera) {
    pWorldCamera = apCamera;
}

bool TESImageSpaceModifier::GetEnable() {
    return bEnable;
}

HighActorCuller::HighActorCuller() : Culler(nullptr), spMBNode(nullptr), spMB(nullptr), spMBSphere(nullptr) {}

HighActorCuller::~HighActorCuller() {
    spMBSphere = nullptr;
    spMB = nullptr;
    spMBNode = nullptr;
}

void MobileObject::SetRotation(const NiPoint3& arRot) {
    SetAngleOnReference(arRot);
}

NiAVObject* FOPipboyManager::GetButtonGlow(uint32_t aiButton) {
    return spButtonGlow[aiButton];
}

bool FOPipboyManager::GetPipBoyManagerReset() {
    return this->bResetPipboyManager;
}

void Main_InitControls(HINSTANCE__* hinst) {
    Controls* oSInputGlobals;
    if (Controls::pInstance) {
        delete Controls::pInstance;
    }
    oSInputGlobals = new Controls(hinst);
    Controls::pInstance = oSInputGlobals;
}
