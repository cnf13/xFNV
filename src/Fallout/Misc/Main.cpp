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
#include "BSCore/RTTI.h"
#include "BSCore/ScrapHeap.h"
#include "BSCore/TLSData.h"
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
#include "BSMain/SettingCollection.h"
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
#include "BSShader/ImageSpaceEffectFullScreenBlur.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
#include "Gamebryo2.2/CoreLibs/NiSystem/NiStaticDataManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/Ini.h"
#include "platform/Paths.h"
#include "platform/System.h"

#include <SDL3/SDL.h>
#include <cstdlib>
#include <filesystem>
#include <stdint.h>
#include <string>
#include <thread>

// The texture palette's upgrade and degrade distances and their defaults (PDB: Main.cpp's file statics).
static float fUpgradeTexturesDistance = 2500.0f;
static float fDegradeTexturesDistance = 3500.0f;
static float fUpgradeTexturesDistanceDefault = -1.0f;
static float fDegradeTexturesDistanceDefault = -1.0f;

// Not named in the PDB or IDA; name chosen here. 0x3A bytes no code of the exe reads or writes, between
// bOffsetViewmodelLights and appMain.
using MainUnusedBytesA = uint8_t[0x3A];
static MainUnusedBytesA data_11DE9D2{};

// Not named in the PDB or IDA; name chosen here. 0x18 bytes no code of the exe reads or writes, between
// iIdleSectionCount and this file's iTotalBSTaskCount copy.
using MainUnusedBytesB = uint8_t[0x18];
static MainUnusedBytesB data_11DEA48{};

// Main::OnIdle_FixActorBones's static bfixNextTime.
static bool bfixNextTime;

// Not named in the PDB or IDA; name chosen here. 4 bytes no code of the exe reads or writes, between iMainValue
// and BSTimer.cpp's appTimer.
using MainUnusedBytesC = uint8_t[4];
static MainUnusedBytesC data_11F6390{};

// INI and game settings read by the OnIdle steps and Main_ShaderInit.
static SettingT<INISettingCollection> bUseHardDriveCache("bUseHardDriveCache:General", false);
static SettingT<INISettingCollection> bDisplayLODLandINI("bDisplayLODLand:LOD", true);
static SettingT<INISettingCollection> fLocalTreeMipMapLODBiasINI("fLocalTreeMipMapLODBias:SpeedTree", -0.25f);
static SettingT<INISettingCollection> fLODTreeMipMapLODBiasINI("fLODTreeMipMapLODBias:SpeedTree", -0.75f);

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
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
SettingT<INISettingCollection> gs_bUseArchives_Archive("bUseArchives:Archive", true);
SettingT<INISettingCollection> bAlwaysActive("bAlwaysActive:General", false);
SettingT<INISettingCollection> bCRTMemoryChecks("bCRTMemoryChecks:General", false);
SettingT<INISettingCollection> gs_sEssentialFileCacheList_General("sEssentialFileCacheList:General",
	"Data\\FalloutNV.esm, Data\\Fallout - Misc.bsa, Data\\Fallout - Sound.bsa, Data\\final_master_xml.dat, "
	"Data\\Shaders\\shaderpackage.sdp, Data\\Music\\Battle\\*, Data\\Music\\Battle\\Finale\\*, Data\\Music\\Base\\*, "
	"Data\\Music\\Dungeon\\*, Data\\Music\\Endgame\\*, Data\\Music\\Explore\\*, Data\\Music\\Public\\*, "
	"Data\\Music\\Special\\*, Data\\Music\\Tension\\*, Data\\Music\\TranquilityLane\\*");
SettingT<INISettingCollection> gs_sUnessentialFileCacheList_General("sUnessentialFileCacheList:General",
	"Data\\Fallout - Meshes.bsa");
SettingT<INISettingCollection> gs_sArchiveList_Archive("sArchiveList:Archive",
	"Fallout - Textures.bsa, Fallout - Meshes.bsa, Oblivion - Voices1.bsa");
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

// Not named in the PDB; IDA's label is g_bUseMyGamesDirectory:General; name chosen here.
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
SettingT<INISettingCollection> gs_iFPS_CameraPath("iFPS:CameraPath", 60);
SettingT<INISettingCollection> gs_sDirectoryName_CameraPath("sDirectoryName:CameraPath", "TestCameraPath");
SettingT<INISettingCollection> gs_iTake_CameraPath("iTake:CameraPath", 0);
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
static SettingT<INISettingCollection> sCopyProtectionTitle("sCopyProtectionTitle:CopyProtectionStrings",
	"CD-ROM Drive Not Found");
static SettingT<INISettingCollection> sCopyProtectionMessage("sCopyProtectionMessage:CopyProtectionStrings",
	"Unable to find a CD-ROM/DVD drive on this computer.");
static SettingT<INISettingCollection> sCopyProtectionTitle2("sCopyProtectionTitle2:CopyProtectionStrings",
	"Fallout Disc Not Found");
static SettingT<INISettingCollection> sCopyProtectionMessage2("sCopyProtectionMessage2:CopyProtectionStrings",
	"Insert the Fallout Disc.");
SettingT<INISettingCollection> bUseBackgroundFileLoader("bUseBackgroundFileLoader:BackgroundLoad", false);
NiPointer<NiRefObject> spcameraPath;
SettingT<INISettingCollection> gs_bCheckRuntimeCollisions_Archive("bCheckRuntimeCollisions:Archive", false);
SettingT<INISettingCollection> gs_iRetainDirectoryStringTable_Archive("iRetainDirectoryStringTable:Archive", 1);
SettingT<INISettingCollection> gs_iRetainFilenameStringTable_Archive("iRetainFilenameStringTable:Archive", 1);
SettingT<INISettingCollection> gs_iRetainFilenameOffsetTable_Archive("iRetainFilenameOffsetTable:Archive", 1);
SettingT<INISettingCollection> gs_sInvalidationFile_Archive("sInvalidationFile:Archive", "ArchiveInvalidation.txt");
SettingT<INISettingCollection> gs_bInvalidateOlderFiles_Archive("bInvalidateOlderFiles:Archive", true);
SettingT<INISettingCollection> gs_iFileLogging_MESSAGES("iFileLogging:MESSAGES", 0);
SettingT<INISettingCollection> sDebugText("sDebugText:Display", "VATS");
SettingT<INISettingCollection> bShowMenuTextureUse("bShowMenuTextureUse:Display", true);
SettingT<INISettingCollection> iDebugTextTopBottomOffset("iDebugTextTopBottomOffset:Display", 10);
SettingT<INISettingCollection> iDebugTextLeftRightOffset("iDebugTextLeftRightOffset:Display", 10);
SettingT<INISettingCollection> bUseSunbeams("bUseSunbeams:Display", false);
SettingT<INISettingCollection> fScopeScissorAmount("fScopeScissorAmount:Display", 0.3f);
SettingT<RegSettingCollection> gs_sBasePath("sBasePath", ".\\");
SettingT<INISettingCollection> bBlockMessageBoxes("bBlockMessageBoxes:MESSAGES", false);
SettingT<INISettingCollection> bUseWindowsMessageBox("bUseWindowsMessageBox:MESSAGES", false);
static BSPrecisionTimer UpdateTexturesTraverseTimer;

// A copy of NiPoint3::ZERO made at start-up.
static NiPoint3 kZeroPointCopy(NiPoint3_ZERO);
static SettingT<INISettingCollection> fINIUpgradeTexturesDistance("fUpgradeTexturesDistance:General", 1500.0f);
static SettingT<INISettingCollection> fINIDegradeTexturesDistance("fDegradeTexturesDistance:General", 2500.0f);
SettingT<INISettingCollection> gs_bAllowYesToAll_MESSAGES("bAllowYesToAll:MESSAGES", true);
SettingT<INISettingCollection> bSkipProgramFlows_MESSAGES("bSkipProgramFlows:MESSAGES", true);
SettingT<INISettingCollection> gs_bSkipInitializationFlows_MESSAGES("bSkipInitializationFlows:MESSAGES", true);

// The program's window, which WinMain creates; appMain->hWnd holds its
// native handle.
SDL_Window* pSDLMainWindow;

// An SDL window's native handle, as the game keeps it (Main::hWnd,
// the renderer's device window): the Win32 window where SDL has one; elsewhere the SDL window itself (the property's
// default), which dxvk-native takes through the untyped HWND.
static HWND__* GetNativeWindow_SDL(SDL_Window* apWindow) {
	return static_cast<HWND__*>(SDL_GetPointerProperty(SDL_GetWindowProperties(apWindow),
		SDL_PROP_WINDOW_WIN32_HWND_POINTER, apWindow));
}

// Not named in the PDB or IDA; name chosen here. The window's event handler, standing for the exe's window procedure
// WinProc (0x86A0A0): per SDL event, what WinProc does for the window message the event stands for, with the same calls
// in the same order. Nothing stands for the messages WinProc only answers (WM_SIZE, WM_ERASEBKGND,
// WM_IME_SETCONTEXT); WM_SYSKEYDOWN's swallowing of the system keys is WinMain's SDL_HINT_WINDOWS_CLOSE_ON_ALT_F4, and
// WM_SYSCOMMAND's refusal of the screen saver (with a 360 controller) SDL's own (it keeps the screen saver off while it
// runs).
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
			appMain->SetActive(1);
			Controls* Controls_ = appMain->GetControls();
			Controls_->SetMouseBackgroundStatus(0);
		}
		break;
	}
	// WM_NCACTIVATE with wParam 0, then WM_ACTIVATE with WA_INACTIVE: the window lost the focus.
	case SDL_EVENT_WINDOW_FOCUS_LOST: {
		bWindowDeactivated = 1;
		if (appMain) {
			if (appMain->bGameActive) {
				if (appMain->bOnIdle) {
					if (!Interface::IsMenuQueuedToOpen() && !Menu::GetMenuByClass(1013) &&
						!Menu::GetMenuByClass(1007)) {
						Interface::QueueMenuCreate(Interface::OPEN_PAUSE, 0, 0, 0, Interface::CMM_STANDARD, 0);
					}
				}
				if (appMain) {
					if (!spcameraPath.m_pObject) {
						appMain->SetActive(0);
					}
				}
			}
		}
		break;
	}
	// WM_MOUSEMOVE: the cursor's position in the window (the background-mouse menus).
	case SDL_EVENT_MOUSE_MOTION: {
		appMain->HandleMouseMove(static_cast<int32_t>(kEvent.motion.x), static_cast<int32_t>(kEvent.motion.y));
		break;
	}
	// WM_NCHITTEST: while Controls exist, the cursor is hidden over the window's client area and shown elsewhere.
	case SDL_EVENT_WINDOW_MOUSE_ENTER:
	case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
		if (appMain) {
			if (appMain->GetControls()) {
				if (kEvent.type == SDL_EVENT_WINDOW_MOUSE_ENTER) {
					if (bIsCursorVisible) {
						SDL_HideCursor();
						bIsCursorVisible = 0;
					}
				} else {
					if (!bIsCursorVisible) {
						SDL_ShowCursor();
						bIsCursorVisible = 1;
					}
				}
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
	default:
		break;
	}
}

int32_t WINAPI WinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, char* lpCmdLine, int32_t nShowCmd) {
	FNVD_INLINED_LOCAL(AutoMemContext, apMemory, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(MemoryManagerFile, apFile, std::nullopt, std::nullopt);
	if (!platform::SingleInstance("FalloutNVGameMutex")) {
		platform::ShowMessageBox("There is already an instance of Fallout: New Vegas running", "Error", 0);
		return 0;
	} else {
		if (platform::RemoteSession()) {
			platform::ShowMessageBox("This application cannot be run from remote desktop.", "Error", 0);
			return 0;
		} else {
			char pPath[264];
			bool v86 = 1;
			bool y = gs_bSkipInitializationFlows_MESSAGES.Bool();
			bool x = bSkipProgramFlows_MESSAGES.Bool();
			bool* v5 = &gs_bAllowYesToAll_MESSAGES.Bool();
			MessageHandler::Init(1, *v5, x, y, &MessageBoxA_0);
			MessageHandler::SetWarningFileName("Warnings.txt");
			MessageHandler::SetProgramFlowFileName("ProgramFlow.txt");
			MessageHandler::SetProgrammerOutputFileName("ProgrammerOutput.txt");
			MessageHandler::SetCrashTraceFileName("StackTrace.txt");
			// GetSystemInfo's dwNumberOfProcessors: the logical processors.
			int32_t* v6 = &iNumHWThreads.Int();
			*v6 = static_cast<int32_t>(std::thread::hardware_concurrency());
			TESMain_LoadIniFile(pPath);
			if (iNumHWThreads.Int() == 1) {
				iNumHWThreads.Int() = 2;
			}
			// The exe ends here when FindWindowA finds another instance's window of class pAppClassName, after
			// bringing it to the front (SetForegroundWindow). SDL cannot reach another process's window; the
			// single-instance check above already ends a second instance.
			{
				BSWin32TaskletManager* v71;
				char File[264];
				FillUserNameGlobal(pAppClassName);
				BSSystemUtility::QInstance().pEventNotificationCallback = &SystemUtilityEventNotificationCallback;
				BSSystemUtility* v7 = &BSSystemUtility::QInstance();
				v7->GetMsgDialogUtility()->pIdleCallback = &SystemUtilityDialogIdleCallback;
				NiStaticDataManager::Init(0);
				BSMain::InitSDM();
				FixedStrings::InitSDM();
				Menu::InitSDM();
				BSShaderSDM::InitSDM();
				TES::InitSDM();
				SetMemContext(MEM_CONTEXT_UNKNOWN);
				BSFadeNode::SetMaxAlphaAccessFunc(&BSFadeNode::GetLinkedActorAlpha);
				BSSoundHandle::SetAddMovingSoundCallback(&BSFadeNode::AddMovingSoundCallback);
				NiSourceTexture::SetDestroyAppDataFlag(1);
				BSStream::InitLoaders();
				BSSearchPath::InitCallback();
				::new (&apMemory) AutoMemContext(MEM_CONTEXT_HAVOK, 1,
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 2272);
				FOCollisionListener::Instance();
				apMemory.Leave();
				apMemory.Enter(MEM_CONTEXT_GAMEBRIO, 1,
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 2278);
				BSTexturePalette::Init();
				apMemory.Leave();
				InitSteamAPI();
				// The command line's arguments, the program first (the exe's CommandLineToArgvW(GetCommandLineW())
				// compared with _wcsicmp; the same for the ASCII "nocloud").
				for (const std::string& kArgument : platform::CommandLine()) {
					if (kArgument[0] == '-') {
						if (!_stricmp(kArgument.c_str() + 1, "nocloud")) {
							SteamAPICallbacks* Singleton = SteamAPICallbacks::GetSingleton();
							Singleton->SetNoCloud(0);
						}
					}
				}
				RegSettingCollection* Collection = SettingT<RegSettingCollection>::GetCollection();
				Collection->ReadSettingsFromFile("Software\\Bethesda Softworks\\Fallout");
				INISettingCollection* v11 = SettingT<INISettingCollection>::GetCollection();
				v11->ReadSettingsFromFile(pPath);
				const char* PersonalPath = BSUtilities::GetPersonalPath();
				BSstrcpy(pPath, 260, PersonalPath);
				BSstrcat(pPath, 260, pAppININame);
				INIPrefSettingCollection* v13 = SettingT<INIPrefSettingCollection>::GetCollection();
				v13->ReadSettingsFromFile(pPath);
				const char* v14 = BSUtilities::GetPersonalPath();
				BSstrcpy(pPath, 260, v14);
				BSstrcat(pPath, 260, pAppPrefININame);
				RendererSettingCollection* v15 = SettingT<RendererSettingCollection>::GetCollection();
				v15->ReadSettingsFromFile(pPath);
				INIPrefSettingCollection* v16 = SettingT<INIPrefSettingCollection>::GetCollection();
				v16->ReadSettingsFromFile(pPath);
				if (iNumHWThreads.Int() == 1) {
					iNumHWThreads.Int() = 2;
				}
				if (CreateDisplayDriver()) {
					BSShaderManager::Shutdown(eTrue);
					v71 = static_cast<BSWin32TaskletManager*>(&BSTaskletManager::QInstance());
					v71->ShutdownAllTasks();
					GetInstalledPathRegistryKey(File, 260u);
					BSstrcat(File, 260, "FalloutNVLauncher.exe");
					// The exe tests ShellExecuteA's result against 0, which only its out-of-memory failure gives (its
					// other failures are codes 2-32): the game ends after the launcher's start whether it started.
					platform::Open(File);
					apMemory.~AutoMemContext();
					return 0;
				} else {
					FaderManager* v69;
					Main* v70;
					const char* sUnessentialFileCacheList = gs_sUnessentialFileCacheList_General.String();
					const char* ya = gs_sEssentialFileCacheList_General.String();
					BSFileCache* v17 = &BSFileCache::QInstance();
					v17->SetFileCacheList(ya, sUnessentialFileCacheList);
					FileFinder* v74 = static_cast<FileFinder*>(ourMalloc(sizeof(FileFinder)));
					if (v74) {
						int32_t yb = gs_iFileLogging_MESSAGES.Int();
						const char* v18 = gs_sBasePath.String();
						::new (v74) FileFinder(v18, yb, 0);
					}
					FileFinder::AddPath_0("DATA");
					bool yc = gs_bSkipInitializationFlows_MESSAGES.Bool();
					bool xa = bSkipProgramFlows_MESSAGES.Bool();
					bool* v19 = &gs_bAllowYesToAll_MESSAGES.Bool();
					MessageHandler::Init(v86, *v19, xa, yc, &MessageBoxA_0);
					MessageHandler::SetWarningFileName("Warnings.txt");
					MessageHandler::SetProgramFlowFileName("ProgramFlow.txt");
					MessageHandler::SetProgrammerOutputFileName("ProgrammerOutput.txt");
					MessageHandler::SetCrashTraceFileName("StackTrace.txt");
					apMemory.Enter(MEM_CONTEXT_FILESYSTEM, 1,
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 2450);
					InitializationFlow();
					BSFileCache* v20 = &BSFileCache::QInstance();
					v20->ForcePauseFileCaching();
					apMemory.Leave();
					apMemory.Enter(MEM_CONTEXT_ARCHIVES, 1,
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 2460);
					SetupArchiveSettings_AndLoadBSAs();
					apMemory.Leave();
					// The window is SDL's, and SDL reads the input: its video and gamepad parts. The window as the
					// exe's (RegisterClassA, AdjustWindowRect, CreateWindowExA): the exe's icon (resource 101), a 320x240
					// client area at 0,0, shown; Alt+F4 does not close it (the exe's WinProc swallows WM_SYSKEYDOWN).
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
					HWND__* window_1 = GetNativeWindow_SDL(pSDLMainWindow);
					Main* v73 = static_cast<Main*>(ourMalloc(sizeof(Main)));
					if (v73) {
						v70 = ::new (v73) Main(window_1, hInstance);
					} else {
						v70 = 0;
					}
					appMain = v70;
					apMemory.Enter(MEM_CONTEXT_INTERFACE, 1,
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 2552);
					FaderManager* v72 = static_cast<FaderManager*>(ourMalloc(sizeof(FaderManager)));
					if (v72) {
						v69 = ::new (v72) FaderManager;
					} else {
						v69 = 0;
					}
					pFaderManager = v69;
					apMemory.Leave();
					InitializationFlow("Initializing Renderer...");
					appMain->InitRenderer();
					NiTexture::bLoadDirectToRenderer = 1;
					NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
					Renderer_->LockRenderer();
					NiDX9Renderer* v22 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
					v22->AddLostDeviceNotificationFunc(&Main::LostDeviceCallback, 0);
					NiDX9Renderer* v23 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
					v23->AddResetNotificationFunc(&Main::ResetDeviceCallback, 0);
					Main_ShaderInit();
					appMain->InitSceneGraph();
					Interface::Init(1);
					BSFaceGenManager::InitializeSystem();
					appMain->SetActive(1);
					NavMeshRender::InitSingleton();
					Main_ImagespaceInit();
					NiDX9Renderer* v24 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
					v24->UnlockRenderer();
					BGSMoviePlayer::Create();
					const char* v25 = gs_sIntroSequence_General.String();
					pMoviePlayer->StartSequence(v25, 0, -1, 1, 1, 0.0f);
					apMemory.Leave();
					SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
					appMain->InitTES(WorldSceneGraph);
					SceneGraph* v27 = Main::GetWorldSceneGraph();
					BSFaceGenManager::SetCamera(v27->GetCamera());
					SceneGraph* v28 = Main::GetWorldSceneGraph();
					BSTreeManager::SetCamera(v28->GetCamera());
					if (iNumHWThreads.Int() > 1) {
						AILinearTaskThreadManager* v29 = AILinearTaskThreadManager::QInstance();
						v29->CreateThreads();
					}
					Pathing::Init();
					PathManager* v30 = PathManager::QInstance();
					v30->Initialize();
					if (bUseObstacleAvoidance.Bool()) {
						NavMeshObstacleManager* Instance = NavMeshObstacleManager::GetInstance();
						Instance->Initialize();
					}
					ScreenSplatter::Init();
					ScreenCustomSplatter::Init();
					int32_t v89 = 0;
					char* a2 = 0;
					NiPoint3 initialPos(2048.0f, 2048.0f, 128.0f);
					TESWorldSpace* sUnessentialFileCacheLista =
						static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item);
					TES::pInstance->SetWorldSpace(sUnessentialFileCacheLista);
					pPlayer->Preload1stPersonAnimsFileList();
					pSaveLoadGameOLD->m_bAllowChanges = 1;
					InitializationFlow("Initializing Actor Locations...");
					ReferenceProcessLists.InitPackageLocations();
					pDataHandler->UpdateAutoCalcNPCs();
					pSaveLoadGame->SetGlobalAllowChanges(1);
					InitializationFlow("Loading initial area...");
					Main::InitStartCell(a2, &initialPos);
					InitializationFlow("Placing player...");
					Main::InitPlayerStart(initialPos);
					Interface::InitSecond();
					appMain->bOnIdle = 1;
					ArchiveManager::SetStartupFinished();
					if (pPlayer->GetParentCell()) {
						pMoviePlayer->WaitForSequenceToEnd();
					} else {
						pDataHandler->SetMasterFileLargeBuffer(-1);
						InitializationFlow();
						Interface::CreateStartMenu();
						pMoviePlayer->WaitForSequenceToEnd();
						InitializationFlow();
					}
					pIOManager->LoadAllQueuedPriorities();
					InitializationFlow("Begin Idle loop...");
					bool v99 = 0;
					while (1) {
						// The pending window events through WinProc_SDL (the exe's PeekMessage, TranslateMessage and
						// DispatchMessage to WinProc); the focus test is SDL's input focus (GetActiveWindow).
						SDL_Event kEvent;
						while (SDL_PollEvent(&kEvent)) {
							WinProc_SDL(kEvent);
						}
						if ((SDL_GetWindowFlags(pSDLMainWindow) & SDL_WINDOW_INPUT_FOCUS) ||
							(TES::pInstance->CellTestRunning() || (spcameraPath.m_pObject || bAlwaysActive.Bool()))) {
							NiDX9Renderer* v32 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
							IDirect3DDevice9* D3DDevice = v32->GetD3DDevice();
							int32_t v79 = D3DDevice->TestCooperativeLevel();
							if (v79 >= 0) {
								appMain->OnIdle();
							} else {
								NiDX9Renderer* v33 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
								v33->BeginOffScreenFrame();
								NiDX9Renderer* v34 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
								v34->EndOffScreenFrame();
								Sleep(50u);
							}
							if (v99) {
								v99 = 0;
								BSAudio* v35 = &BSAudio::QInstance();
								v35->UnPauseSynchTimer();
								FalloutAudioMedia::MediaMenuUnPause();
								FalloutRadio::Update(0);
								BSAudio* v36 = &BSAudio::QInstance();
								v36->Update(1);
								// The exe re-fits a full-screen window here, as in ResetDeviceCallback.
							}
							if (!v99 && appMain->oneMore && v79 >= 0) {
								LoadingMenu::ResumeBackgroundThread();
								appMain->oneMore = 0;
							}
						} else {
							if (!v99) {
								v99 = 1;
								if (!LoadingMenuThread::GetByte49()) {
									uint32_t CurrentThreadId = GetCurrentThreadId();
									if (CurrentThreadId == appMain->uiThreadID) {
										LoadingMenu::SuspendBackgroundThread();
										appMain->oneMore = 1;
									}
								}
								BSAudio* v38 = &BSAudio::QInstance();
								v38->PauseSynchTimer();
								FalloutAudioMedia::MediaMenuPause();
								FalloutRadio::Update(0);
								BSAudio* v39 = &BSAudio::QInstance();
								v39->Update(1);
								BSShaderAccumulator* v40 = appMain->QDrawWorldAccum();
								v40->ClearSunQueries();
								BGSDecalManager* v41 = BGSDecalManager::GetSingleton();
								v41->ClearPendingQueries();
								FOVATSEffectManager* VATSEffect = Interface::GetVATSEffect();
								VATSEffect->FreeOcclusionQueries();
								// The exe re-fits a full-screen window here (AdjustWindowRectEx, SetWindowPos); the
								// full-screen window is the device's (Direct3D 9's, dxvk's SDL backend).
							}
							Sleep(50u);
						}
						if (appMain->bQuitGame) {
							break;
						}
						if (appMain->GameReset()) {
							appMain->PerformGameReset(1);
						}
					}
					appMain->bOnIdle = 0;
					DestroySteamAPICallbacks();
					Interface::HideMenus();
					Interface::CloseAllSupportMenus(0);
					if (Main::spMenuBGTexture.m_pObject) {
						BSRenderedTexture* yd = Main::spMenuBGTexture.m_pObject;
						BSTextureManager* TextureManager = BSShaderManager::TextureManagerPtr();
						TextureManager->ReturnRenderedTexture(yd);
						Main::spMenuBGTexture.operator=(static_cast<BSRenderedTexture*>(0));
					}
					if (Main::spInterfaceFXTex.m_pObject) {
						BSRenderedTexture* ye = Main::spInterfaceFXTex.m_pObject;
						BSTextureManager* v44 = BSShaderManager::TextureManagerPtr();
						v44->ReturnRenderedTexture(ye);
						Main::spInterfaceFXTex.operator=(static_cast<BSRenderedTexture*>(0));
					}
					if (Main::spVATSTex.m_pObject) {
						BSRenderedTexture* yf = Main::spVATSTex.m_pObject;
						BSTextureManager* v45 = BSShaderManager::TextureManagerPtr();
						v45->ReturnRenderedTexture(yf);
						Main::spVATSTex.operator=(static_cast<BSRenderedTexture*>(0));
					}
					TaskQueueInterface::SetActive(0);
					if (NiParallelUpdateTaskManager::Get()) {
						NiParallelUpdateTaskManager::Shutdown();
					}
					pModelLoader->CancelAllFiles();
					if (iNumHWThreads.Int() > 1) {
						AILinearTaskThreadManager* v46 = AILinearTaskThreadManager::QInstance();
						v46->DestroyThreads();
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
					BSAudio* v67 = &BSAudio::QInstance();
					v67->Shutdown();
					InitializationFlow();
					BGSDecalManager::Destroy_();
					DecalCaster::ClearInstance();
					HitCaster::ClearInstance();
					if (bUseObstacleAvoidance.Bool()) {
						NavMeshObstacleManager* v47 = NavMeshObstacleManager::GetInstance();
						v47->Release();
					}
					if (TES::pInstance) {
						delete TES::pInstance;
					}
					BGSTerrainManager::DeallocateStaticNodes();
					PathManager* v48 = PathManager::QInstance();
					v48->Release();
					BSFaceGenManager::DestroySystem();
					CombatManager::Destroy();
					BSWin32TaskletManager* v66 = static_cast<BSWin32TaskletManager*>(&BSTaskletManager::QInstance());
					v66->ShutdownAllTasks();
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
					appMain = 0;
					BSFileCache* v65 = &BSFileCache::QInstance();
					v65->Shutdown();
					char* yg = SettingT<INIPrefSettingCollection>::GetCollection()->pSettingFile;
					INIPrefSettingCollection* v49 = SettingT<INIPrefSettingCollection>::GetCollection();
					v49->SaveAll(yg);
					NiDX9Renderer* v50 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
					v50->RemoveResetNotificationFunc(&Main::ResetDeviceCallback);
					NiDX9Renderer* v51 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
					v51->RemoveLostDeviceNotificationFunc(&Main::LostDeviceCallback);
					NavMeshRender::Destroy();
					BSTexturePalette::Kill();
					BSShaderSDM::KillSDM();
					BSShaderManager::Shutdown(eTrue);
					FileFinder::Close();
					TES::KillSDM();
					BSMain::KillSDM();
					FixedStrings::KillSDM();
					Menu::KillSDM();
					NiStaticDataManager::Shutdown();
					::new (&apFile) MemoryManagerFile("ExitData.mhd");
					MemoryManager* v52 = &MemoryManager::Instance();
					v52->OutputDebugInfo(&apFile, 0);
					apFile.~MemoryManagerFile();
					apMemory.~AutoMemContext();
					return 0;
				}
			}
		}
	}
}

HWND__* GetActiveWindow_SDL() {
	if (!pSDLMainWindow || SDL_GetKeyboardFocus() != pSDLMainWindow) {
		return nullptr;
	}
	return GetNativeWindow_SDL(pSDLMainWindow);
}

void Main_ShaderInit() {
	InitializationFlow("Initializing Shader System...");
	BSShaderManager::SetShaderErrorCallback(nullptr);
	BSShaderManager::SetTimerFunc(&fnvd::callFunction<&BSShaderManager::CalculateTime>);
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
	} else {
		SkyShader::SetSunBrightness(Renderer::GetBlurSunBrightness());
		SkyShader::SetSkyBrightness(Renderer::GetBlurSkyBrightness());
		ImageSpaceEffectBlur::fSpecularMult = Renderer::GetBlurSpecularMult();
		ImageSpaceEffectBlur::fEmmisiveMult = Renderer::GetBlurEmmisiveMult();
	}
	float fMotionBlur = ::fMotionBlur.Float();
	ImageSpaceManager* pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
	static_cast<ImageSpaceEffectMotionBlur*>(pImageSpaceManager->GetEffect(ImageSpaceManager::EFFECT_MOTION_BLUR))
		->SetFullScreenBlurConst(fMotionBlur);
}

BSMsgDialogSystemUtility::USER_RESPONSE SystemUtilityDialogIdleCallback() {
	if (!NiRenderer::GetRenderer()) {
		return BSMsgDialogSystemUtility::USER_RESPONSE_NONE;
	}
	NiDX9Renderer* renderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
	renderer->DoMinimalFlip();
	return BSMsgDialogSystemUtility::USER_RESPONSE_NONE;
}

// ExitProcess ends the process without the CRT's atexit handlers and static destructors: std::_Exit. The exe's code
// after the call (a Yes-to-all, as the import is not marked noreturn) never runs.
void MessageMenuCallback() {
	char MessageMenuResult = Interface::GetMessageMenuResult();
	if (MessageMenuResult == 1 || MessageMenuResult == 3) {
		InitializationFlow("Warning Response was to Exit the Process");
		if (platform::DebuggerPresent()) {
			__builtin_debugtrap();
		}
		std::_Exit(0);
	} else if (MessageMenuResult == 2) {
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

int32_t TESMain_LoadIniFile(char* lpFileName) {
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
		char pszPath[264];
		std::error_code kError;
		*lpFileName = 0;
		// CSIDL_LOCAL_APPDATA (0x1C), then CSIDL_PERSONAL (5).
		BSstrcpy(pszPath, 260, platform::UserFolder(platform::UserFolderKind::LocalAppData).c_str());
		BSstrcat(pszPath, 260, "\\FalloutNV\\");
		std::filesystem::create_directory(platform::NativePath(pszPath), kError);
		BSstrcpy(BSUtilities::GetLocalAppDataPath(), 260, pszPath);
		BSstrcpy(pszPath, 260, platform::UserFolder(platform::UserFolderKind::Documents).c_str());
		BSstrcat(pszPath, 260, "\\My Games\\");
		std::filesystem::create_directory(platform::NativePath(pszPath), kError);
		BSstrcat(pszPath, 260, "FalloutNV\\");
		std::filesystem::create_directory(platform::NativePath(pszPath), kError);
		BSstrcpy(BSUtilities::GetPersonalPath(), 260, pszPath);
	} else {
		BSstrcpy(BSUtilities::GetLocalAppDataPath(), 260, ".\\");
		BSstrcpy(BSUtilities::GetPersonalPath(), 260, ".\\");
	}
	BSstrcpy(lpFileName, 260, BSUtilities::GetPersonalPath());
	BSstrcat(lpFileName, 260, pAppININame);
	std::error_code kError;
	std::filesystem::remove(platform::NativePath(MessageHandler::GetWarningFileName()), kError);
	std::filesystem::remove(platform::NativePath(MessageHandler::GetProgramFlowFileName()), kError);
	std::filesystem::remove(platform::NativePath(MessageHandler::GetProgrammerOutputFileName()), kError);
	int32_t result = rt::_access(lpFileName, 0);
	if (result == -1) {
		char ExistingFileName[264];
		BSstrcpy(ExistingFileName, 260, "Fallout_default.ini");
		// CopyFileA with bFailIfExists: the copy's success as the BOOL.
		return std::filesystem::copy_file(platform::NativePath(ExistingFileName), platform::NativePath(lpFileName),
			std::filesystem::copy_options::none, kError);
	}
	return result;
}

void SetupArchiveSettings_AndLoadBSAs() {
	const char* v4 = gs_sArchiveList_Archive.String();
	const char* v3 = sLocalMasterPath.String();
	bool* v0 = &gs_bUseArchives_Archive.Bool();
	ArchiveManager::Init(*v0, v3, v4, 0x200000u);
	bool* v1 = &gs_bCheckRuntimeCollisions_Archive.Bool();
	ArchiveManager::SetCheckRuntimeCollisions(*v1);
	ArchiveManager::SetRetainTables(gs_iRetainDirectoryStringTable_Archive.Int(),
		gs_iRetainFilenameStringTable_Archive.Int(), gs_iRetainFilenameOffsetTable_Archive.Int());
	const char* v5 = gs_sInvalidationFile_Archive.String();
	bool* v2 = &gs_bInvalidateOlderFiles_Archive.Bool();
	ArchiveManager::SetInvalidation(*v2, v5);
	ArchiveManager::OpenMasterArchives();
}

void SystemUtilityEventNotificationCallback(BSSystemUtility::EVENT_NOTIFICATION aeEventNotification) {
	if (aeEventNotification == BSSystemUtility::EVENT_NOTIFICATION_USER_REQUEST_EXIT) {
		if (NiRenderer::GetRenderer()) {
			NiRenderer* Renderer_ = NiRenderer::GetRenderer();
		}
		BGSMoviePlayer::Destroy();
		if (pIOManager) {
			pIOManager->Pause();
		}
		pModelLoader->SetExiting();
		pIOManager->SetThreadExiting();
		pIOManager->Resume();
		BSFileCache* v2 = &BSFileCache::QInstance();
		v2->ForcePauseAndWaitForIdle();
		BSFileCache* v5 = &BSFileCache::QInstance();
		v5->Shutdown();
		FalloutAudioMedia::MediaClose();
		FalloutAudioMedia::Shutdown();
		BSAudio* v4 = &BSAudio::QInstance();
		v4->Shutdown();
		LoadingMenu::SetThreadExiting();
		LoadingMenu::KillThread();
		BSTaskletManager* v3 = &BSTaskletManager::QInstance();
		v3->ShutdownAllTasks();
		if (pIOManager) {
			pIOManager->Pause();
		}
	}
}

int32_t MessageBoxA_0(const char* apText, const char* apCaption, uint32_t aiType) {
	int32_t v15 = 6;
	if (!bBlockMessageBoxes.Bool()) {
		const char* yes;
		const char* no;
		const char* v12;
		appTimer.Disable();
		uint8_t v14 = aiType & 15;
		if (!MessageBoxA_bOnlyOnce || (!appMain || !appMain->GetControls())) {
			if (!Renderer::GetFullScreen() || !Renderer::GetHWND()) {
				if (appMain) {
					uint32_t CurrentThreadId = GetCurrentThreadId();
					if (CurrentThreadId != appMain->uiThreadID) {
						if (appMain->GetControls()) {
							Controls* Controls_ = appMain->GetControls();
							if (!Controls_->IsMouseInBackground()) {
								Controls* v9 = appMain->GetControls();
								v9->SetMouseBackgroundStatus(1);
							}
						}
					}
				}
				v15 = platform::ShowMessageBox(apText, apCaption, aiType);
			}
			goto LABEL_40;
		}
		MessageBoxA_bOnlyOnce = 0;
		yes = 0;
		no = 0;
		v12 = 0;
		if (v14 == 3) {
			v12 = gs_sYesToAllText.String();
		} else {
			if (v14 != 4) {
			LABEL_14:;
				if (v14 == 2 ||
					(!v14 ||
						!Interface::CreateMessageMenu(static_cast<const char*>(apText), static_cast<const char*>(0),
							static_cast<const char*>(0), &MessageMenuCallback, 0, MessageHandler::QWarningContext(),
							0.0f, 0.0f, yes, no, v12, nullptr))) {
					if (Renderer::GetFullScreen() && Renderer::GetHWND()) {
						if (v14 == 2) {
							v15 = 5;
						} else {
							if ((aiType & 15) > 2 && (aiType & 15) <= 4) {
								v15 = 6;
							}
						}
					} else {
						if (appMain) {
							uint32_t v4 = GetCurrentThreadId();
							if (v4 != appMain->uiThreadID) {
								if (appMain->GetControls()) {
									Controls* v5 = appMain->GetControls();
									if (!v5->IsMouseInBackground()) {
										Controls* v6 = appMain->GetControls();
										v6->SetMouseBackgroundStatus(1);
									}
								}
							}
						}
						v15 = platform::ShowMessageBox(apText, apCaption, aiType);
					}
				}
				MessageBoxA_bOnlyOnce = 1;
			LABEL_40:;
				appTimer.Enable();
				return v15;
			}
		}
		yes = gs_sYesText.String();
		no = gs_sNoText.String();
		goto LABEL_14;
	}
	if ((aiType & 15) == 2) {
		return 5;
	} else {
		return 6;
	}
}

Main::Main(HWND__* ahWnd, HINSTANCE__* ahInstance)
	: bQuitGame(false), bResetGame(false), bGameActive(false), bOnIdle(false), bReloadContent(false), bFlyCamera(false),
	  bFreezeTime(false), hWnd(ahWnd), hInstance(ahInstance), uiThreadID(BSGetCurrentThreadID()),
	  hThreadHandle(nullptr), kTaskQueue(&kPackedTaskHeap, &fnvd::callFunction<&TaskQueueInterface::TaskUnpackFunc>),
	  kSecondaryTaskQueue(&kSecondaryPackedTaskHeap, &fnvd::callFunction<&TaskQueueInterface::TaskUnpackFunc>),
	  bRenderingMenuBG(false) {
	Main::spInterfaceFXTex = nullptr;
	spDrawWorldAccum = new BSShaderAccumulator(99, 1u, 759u);
	spDraw1stPersonAccum = new BSShaderAccumulator(99, 1u, 759u);
	spDraw1stPersonAccum->SetRenderMode(BSShaderManager::BSSM_RENDER_FIRST_PERSON);
	spDraw1stPersonAccum->b1stPerson = 1;
	spDraw1stPersonDepthAccum = new BSShaderAccumulator(99, 1u, 759u);
	spDraw1stPersonDepthAccum->SetRenderMode(BSShaderManager::BSSM_RENDER_FIRST_PERSON);
	spDraw1stPersonDepthAccum->b1stPerson = 1;
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
		Controls::InitializeInstance(ahInstance);
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
	// The exe calls GetTexture even for a null texture; GetTexture returns null for a null this.
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
	// The exe sets each callback through BSAudio's one-field setters (0x50F9C0, 0x7037C0, 0x61CC40, 0x86CF00,
	// 0x5F4BB0, 0x8D7DC0, folds named after other classes), on a fresh QInstance each time.
	BSAudio::QInstance().pfnIDCallback = &FalloutAudio::ResolveSoundID;
	BSAudio::QInstance().pfnNameCallback = &FalloutAudio::ResolveSoundName;
	BSAudio::QInstance().pfnRandomFilenameCallback = &TESSound::PickRandomFilename;
	BSAudio::QInstance().pfnSettingsCallback = &fnvd::callFunction<&FalloutAudio::ApplySettingsFromMasterFile>;
	BSAudio::QInstance().pfnSynchPausedCallback = &FalloutRadio::RadioSynchPaused;
	BSAudio::QInstance().pfnSynchUnPausedCallback = &FalloutRadio::RadioSynchUnPaused;
	BSAudio* pAudio = &BSAudio::QInstance();
	pAudio->SetLoopPointCallback(&FalloutAudio::GetLoopPoints);
	BSAudio* pAudio2 = &BSAudio::QInstance();
	pAudio2->Init(hWnd);
	FalloutAudioMedia::Startup();
}

// The exe's cleanup-only C++ EH frame is left out.
void Main::InitTES(NiNode* pScene) {
	PlayerCharacter* pPlayer;
	TES* v10;
	NiPoint3 dst;
	SettingT<INISettingCollection>* pTestFiles[10];
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, tempMemContext, &AutoMemContext::Destruct, MEM_CONTEXT_GAME_OVERHEAD, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 3663);
	tempMemContext.Enter(MEM_CONTEXT_GAME_OVERHEAD, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 3665);
	Main::pSky = Sky::GetInstance();
	Main::InitSky();
	NiNode* pObjects = Main::ObjectLODNode();
	NiNode* pLands = Main::LandLODNode();
	if (pLands) {
		NiNode* pLands_2;
		Main::pSky->GetLight()->AttachAffectedNode(pLands);
		NiAVObject* pchild = pLands->GetAtChecked(0);
		if (pchild) {
			pLands_2 = pchild->IsNode();
		} else {
			pLands_2 = 0;
		}
		pLands = pLands_2;
	}
	InitializationFlow("Initializing TES...");
	TES* v16 = static_cast<TES*>(ourMalloc(sizeof(TES)));
	if (v16) {
		v10 = ::new (v16) TES("DATA\\", pObjects, pLands, Main::pSky, Main::pWaterLOD);
	} else {
		v10 = 0;
	}
	TES::pInstance = v10;
	tempMemContext.Leave();
	TES::pInstance->InitializeWater();
	InitializationFlow("Initializing TreeManager...");
	BSTreeManager::Create(0);
	InitializationFlow("Activating Tasklet Threads...");
	BSTaskletManager* v2 = &BSTaskletManager::QInstance();
	v2->SignalInstanceAvailable();
	BGSLoadedReferenceCollection::Create();
	CombatManager::Create();
	tempMemContext.Enter(MEM_CONTEXT_WORLD_SG, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
		3717);
	pScene->UpdateProperties();
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData, nullptr, 0.0f, 0, 0);
	pScene->Update(UpdateData);
	tempMemContext.Leave();
	bool bFilesChecked = 0;
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
			const char* v3 = pTestFiles[n]->String();
			if (std::strlen(v3)) {
				const char* pluginsTxt = pTestFiles[n]->String();
				TESFile* pFile = pDataHandler->GetListFile(pluginsTxt);
				if (pFile) {
					pFile->SetChecked(1);
					bFilesChecked = 1;
				}
			}
		}
	}
	char* pluginsTxta = Plugins_txt;
	char* LocalAppDataPath = BSUtilities::GetLocalAppDataPath();
	if (LoadPluginsFromFile(LocalAppDataPath, pluginsTxta)) {
		bFilesChecked = 1;
	}
	if (!bFilesChecked) {
		TESFile* pFile_2 = pDataHandler->GetListFile("Fallout.esm");
		if (pFile_2) {
			pFile_2->SetChecked(1);
		}
	}
	tempMemContext.Enter(MEM_CONTEXT_PLAYER, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
		3787);
	PlayerCharacter* v14 = static_cast<PlayerCharacter*>(ourMalloc(sizeof(PlayerCharacter)));
	if (v14) {
		pPlayer = ::new (v14) PlayerCharacter;
	} else {
		pPlayer = 0;
	}
	::pPlayer = pPlayer;
	pPlayer->SetFormID(20u, 1);
	tempMemContext.Leave();
	InitializationFlow("Loading Files...");
	pDataHandler->CompileFiles(0);
	TESChallengeManager::LoadChallenges();
	TES::pInstance->PreloadAddonNodes();
	TES::pInstance->PreloadDefaultModels();
	if (TESWaterSystem::bUseWater.Bool()) {
		TES::pInstance->PreloadPlaceableWaterNoiseTextures();
	}
	TES::pInstance->PreloadForms();
	BSTempEffectSimpleDecal::PreloadDecalData();
	InitializationFlow("Initializing Player...");
	tempMemContext.Enter(MEM_CONTEXT_PLAYER, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp",
		3840);
	TESForm* FormByNumericID = TESForm::GetFormByNumericID(7u);
	TESNPC* pNPC = dynamic_cast<TESNPC*>(FormByNumericID);
	::pPlayer->SetObjectReference(pNPC);
	NiPoint3* v6 = ::new (&dst) NiPoint3(2048.0f, 2048.0f, 0.0f);
	::pPlayer->SetPosition(*v6);
	::pPlayer->SetRotation(NiPoint3_ZERO);
	::pPlayer->InitItem();
	::pPlayer->CastPermanentMagic(0);
	if (!::pPlayer->GetActorValue(ActorValue::HEALTH)) {
		InitializationFlow("ERROR: Health value is 0 on the Player. Fix '%s' with the editor.",
			&pNPC->GetFile(0)->m_Filename);
	}
	InitializationFlow("Initializing Scripts...");
	pDataHandler->FixAllScriptIDs();
	// The exe tests the setting's unsigned value with signed compares (jle 0, jge 200).
	if (static_cast<int32_t>(iFPSClamp.operator uint32_t()) > 0 &&
		static_cast<int32_t>(iFPSClamp.operator uint32_t()) < 200) {
		// The PDB's aiClamp is unsigned; the exe passes Int()'s dword as it is.
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

// The exe's cleanup-only C++ EH frame is left out.
void Main::InitSceneGraph() {
	BSParticleSystemManager* BSParticleSystemManager_;
	NiNode* ObjectLODRoot;
	NiNode* plodtreenode;
	NiNode* WaterLOD;
	NiNode* plodnode_2;
	NiNode* plodnode;
	BSClearZNode* BSClearZNode_;
	NiNode* v28;
	BSMultiBoundNode* Sky_;
	BSFogProperty* pFogProp;
	int32_t v33;
	ShadowSceneNode* pshadowSceneNode;
	SceneGraph* pSceneGraph;
	FNVD_INLINED_LOCAL(NiFixedString, v41, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v44, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v47, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v50, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v53, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v56, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v59, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v62, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, tempMemContext, &AutoMemContext::Destruct, MEM_CONTEXT_WORLD_SG, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 3916);
	SceneGraph* v72 = static_cast<SceneGraph*>(NiMemObject::operator new(sizeof(SceneGraph)));
	if (v72) {
		pSceneGraph = ::new (v72) SceneGraph("World", 0, nullptr);
	} else {
		pSceneGraph = 0;
	}
	NiRefObject* v73 = (NiRefObject*)pSceneGraph;
	Main::spWorldRoot.operator=(static_cast<BSSceneGraph*>(pSceneGraph));
	ShadowSceneNode* v70 = static_cast<ShadowSceneNode*>(NiMemObject::operator new(sizeof(ShadowSceneNode)));
	if (v70) {
		pshadowSceneNode = ::new (v70) ShadowSceneNode;
	} else {
		pshadowSceneNode = 0;
	}
	NiDX9Renderer* NIRenderer = Renderer::GetNIRenderer();
	BSShaderManager::SetRenderer(NIRenderer);
	BSShaderManager::SetDefaultShaderPath("data\\shaders\\");
	BSShaderManager::Initialize();
	Main::spWorldRoot.m_pObject->GetCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
	pshadowSceneNode->SetSceneGraphIndex(0);
	BSShaderManager::SetShadowSceneNode(0, pshadowSceneNode);
	spDrawWorldAccum.m_pObject->SetShadowSceneNode(pshadowSceneNode);
	spDraw1stPersonAccum.m_pObject->SetShadowSceneNode(pshadowSceneNode);
	spDraw1stPersonDepthAccum.m_pObject->SetShadowSceneNode(pshadowSceneNode);
	spScreenSplatterAccum.m_pObject->SetShadowSceneNode(pshadowSceneNode);
	ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
	spRenderedMenuAccum.m_pObject->SetShadowSceneNode(ShadowSceneNode_);
	BSShaderManager::SetSeparateAmbientLight(0);
	BSShaderManager::SetSeparateSpecularColor(0);
	int32_t ilightingPasses = Renderer::QDoAmbientPass();
	ilightingPasses |= !Renderer::QDoDiffusePass() ? 0 : 2;
	ilightingPasses |= !Renderer::QDoTexturePass() ? 0 : 4;
	ilightingPasses |= (!Renderer::QDoSpecularPass() ? 0 : 8) | 16;
	if (Renderer::QDoCanopyShadowPass() && BSShaderManager::GetShaderLevel(0) >= BSShaderManager::BSSM_SV_2_0) {
		v33 = 32;
	} else {
		v33 = 0;
	}
	ilightingPasses |= v33;
	BSShaderManager::SetEnabledPasses(ilightingPasses);
	int32_t iShadowCasters = Renderer::QDoActorShadows() ? 8 : 0;
	iShadowCasters |= Renderer::QDoStaticAndArchShadows() ? 2 : 0;
	BSShaderManager::uiObjectTypeShadowMask = iShadowCasters;
	BSShaderManager::SetObjectTypeCallback(&TESObjectREFR::GetLightingObjectType);
	ShadowLightShader::fWaterDepthRange = 0.0f;
	BSShaderManager::spMainCamera.operator=(Main::spWorldRoot.m_pObject->GetCamera());
	SceneGraph* v32 = static_cast<SceneGraph*>(Main::spWorldRoot.m_pObject);
	v32->AttachChild(pshadowSceneNode, 1);
	BSFogProperty* v68 = static_cast<BSFogProperty*>(NiMemObject::operator new(sizeof(BSFogProperty)));
	if (v68) {
		pFogProp = ::new (v68) BSFogProperty;
	} else {
		pFogProp = 0;
	}
	NiRefObject* v69 = (NiRefObject*)pFogProp;
	Main::spFog.operator=(pFogProp);
	Main::spFog.m_pObject->SetFog(1);
	Main::spFog.m_pObject->SetDepth(1.0f);
	BSShaderManager::SetFogProperty(0, Main::spFog.m_pObject);
	BSMultiBoundNode* v66 = static_cast<BSMultiBoundNode*>(NiMemObject::operator new(sizeof(BSMultiBoundNode)));
	if (v66) {
		Sky_ = ::new (v66) BSMultiBoundNode;
	} else {
		Sky_ = 0;
	}
	NiRefObject* v67 = (NiRefObject*)Sky_;
	Main::spSkyRoot.operator=(static_cast<NiNode*>(Sky_));
	static_cast<BSMultiBoundNode*>(Main::spSkyRoot.m_pObject)
		->BSMultiBoundNode::SetCullingMode(BSCullingProcess::BSCP_CULL_ALLPASS);
	Main::spSkyRoot.m_pObject->SetFixedBound(1);
	NiFixedString* kName = ::new (static_cast<void*>(&apString)) NiFixedString("Sky");
	Main::spSkyRoot.m_pObject->SetName(*kName);
	apString.~NiFixedString();
	NiNode* a4 = Main::spSkyRoot.m_pObject;
	pshadowSceneNode->SetAt(0, a4);
	NiNode* v63 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
	if (v63) {
		v28 = ::new (v63) NiNode(0);
	} else {
		v28 = 0;
	}
	NiRefObject* v64 = (NiRefObject*)v28;
	Main::spWeatherRoot.operator=(v28);
	Main::spWeatherRoot.m_pObject->SetAlwaysDraw(1);
	Main::spWeatherRoot.m_pObject->SetFixedBound(1);
	NiFixedString* v27 = ::new (static_cast<void*>(&v62)) NiFixedString("Weather");
	Main::spWeatherRoot.m_pObject->SetName(*v27);
	v62.~NiFixedString();
	NiNode* a4a = Main::spWeatherRoot.m_pObject;
	pshadowSceneNode->SetAt(1u, a4a);
	Main::CreateMenuRootNode();
	Main::CreateMenuRootNode3D();
	BSClearZNode* v60 = static_cast<BSClearZNode*>(NiMemObject::operator new(sizeof(BSClearZNode)));
	if (v60) {
		BSClearZNode_ = ::new (v60) BSClearZNode;
	} else {
		BSClearZNode_ = 0;
	}
	Main::pLandLOD = (NiNode*)BSClearZNode_;
	NiFixedString* LODRoot = ::new (static_cast<void*>(&v59)) NiFixedString("LODRoot");
	Main::pLandLOD->SetName(*LODRoot);
	v59.~NiFixedString();
	pshadowSceneNode->SetAt(2u, Main::pLandLOD);
	NiNode* v57 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
	if (v57) {
		plodnode = ::new (v57) NiNode(0);
	} else {
		plodnode = 0;
	}
	NiNode* plodnode_3 = plodnode;
	plodnode->SetAlwaysDraw(1);
	plodnode_3->SetFixedBound(1);
	NiFixedString* name = ::new (static_cast<void*>(&v56)) NiFixedString("LandLOD");
	plodnode_3->SetName(*name);
	v56.~NiFixedString();
	Main::pLandLOD->AttachChild(plodnode_3, 1);
	BSShaderManager::fLODLandDrop = fLODLandDropAmount.Float();
	NiNode* v54 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
	if (v54) {
		plodnode_2 = ::new (v54) NiNode(0);
	} else {
		plodnode_2 = 0;
	}
	plodnode_3 = plodnode_2;
	plodnode_2->SetAlwaysDraw(1);
	plodnode_3->SetFixedBound(1);
	NiFixedString* v21 = ::new (static_cast<void*>(&v53)) NiFixedString("DistantRefLOD");
	plodnode_3->SetName(*v21);
	v53.~NiFixedString();
	Main::pLandLOD->AttachChild(plodnode_3, 1);
	NiNode* v51 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
	if (v51) {
		WaterLOD = ::new (v51) NiNode(0);
	} else {
		WaterLOD = 0;
	}
	Main::pWaterLOD = WaterLOD;
	WaterLOD->SetAlwaysDraw(1);
	Main::pWaterLOD->SetFixedBound(1);
	NiFixedString* v19 = ::new (static_cast<void*>(&v50)) NiFixedString("WaterLOD");
	Main::pWaterLOD->SetName(*v19);
	v50.~NiFixedString();
	Main::pLandLOD->AttachChild(Main::pWaterLOD, 1);
	NiNode* v48 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
	if (v48) {
		plodtreenode = ::new (v48) NiNode(0);
	} else {
		plodtreenode = 0;
	}
	plodtreenode->SetAlwaysDraw(1);
	plodtreenode->SetFixedBound(1);
	NiFixedString* v17 = ::new (static_cast<void*>(&v47)) NiFixedString("LOD Trees");
	plodtreenode->SetName(*v17);
	v47.~NiFixedString();
	plodnode_3->AttachChild(plodtreenode, 1);
	DistantLODShaderProperty::SetLODTreeParent(plodtreenode);
	BGSTerrainManager::SetTreeNode(plodtreenode);
	NiNode* v45 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
	if (v45) {
		ObjectLODRoot = ::new (v45) NiNode(0);
	} else {
		ObjectLODRoot = 0;
	}
	Main::pObjectLOD = ObjectLODRoot;
	ObjectLODRoot->SetAlwaysDraw(1);
	Main::pObjectLOD->SetFixedBound(1);
	NiFixedString* v15 = ::new (static_cast<void*>(&v44)) NiFixedString("ObjectLODRoot");
	Main::pObjectLOD->SetName(*v15);
	v44.~NiFixedString();
	pshadowSceneNode->SetAt(3u, Main::pObjectLOD);
	BSParticleSystemManager* v42 =
		static_cast<BSParticleSystemManager*>(NiMemObject::operator new(sizeof(BSParticleSystemManager)));
	if (v42) {
		BSParticleSystemManager_ = ::new (v42) BSParticleSystemManager;
	} else {
		BSParticleSystemManager_ = 0;
	}
	NiRefObject* v43 = (NiRefObject*)BSParticleSystemManager_;
	Main::spParticleSystemManager.operator=(BSParticleSystemManager_);
	NiFixedString* v13 = ::new (static_cast<void*>(&v41)) NiFixedString("Master Particle Systems");
	Main::spParticleSystemManager.m_pObject->SetName(*v13);
	v41.~NiFixedString();
	BSParticleSystemManager* a4b = Main::spParticleSystemManager.m_pObject;
	pshadowSceneNode->SetAt(5u, a4b);
	NiNode* CameraRoot = Main::spWorldRoot.m_pObject->GetCameraRoot();
	CameraRoot->SetTranslate(0.0f, 0.0f, 128.0f);
	Main::spWorldRoot.m_pObject->UpdateProperties();
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData, nullptr, 0.0f, 0, 0);
	Main::spWorldRoot.m_pObject->Update(UpdateData);
	BSShaderAccumulator* a4c = BSShaderManager::GetAccumulator();
	NiRenderer::GetRenderer()->SetSorter(a4c);
	Main::bStaticMenuBG = Renderer::QDOStaticMenuBackground();
	if (!Main::spMenuBGGeometry.m_pObject) {
		NiTexturingProperty* ptexprop;
		NiMaterialProperty* pmatprop;
		NiScreenElements* pScreenElements;
		NiScreenElements* v40 = static_cast<NiScreenElements*>(NiMemObject::operator new(sizeof(NiScreenElements)));
		if (v40) {
			NiScreenElementsData* NiScreenElementsData_;
			NiScreenElementsData* v39 =
				static_cast<NiScreenElementsData*>(NiMemObject::operator new(sizeof(NiScreenElementsData)));
			if (v39) {
				NiScreenElementsData_ = ::new (v39) NiScreenElementsData(0, 0, 1, 1, 1, 4, 1, 2, 1);
			} else {
				NiScreenElementsData_ = 0;
			}
			pScreenElements = ::new (v40) NiScreenElements(NiScreenElementsData_);
		} else {
			pScreenElements = 0;
		}
		Main::spMenuBGGeometry.operator=(pScreenElements);
		Main::spMenuBGGeometry.m_pObject->Insert(4, 0, 0);
		Main::spMenuBGGeometry.m_pObject->SetRectangle(0, 0.0f, 0.0f, 1.0f, 1.0f);
		Main::spMenuBGGeometry.m_pObject->UpdateBound();
		Main::spMenuBGGeometry.m_pObject->SetTextures(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
		NiMaterialProperty* v38 =
			static_cast<NiMaterialProperty*>(NiMemObject::operator new(sizeof(NiMaterialProperty)));
		if (v38) {
			pmatprop = ::new (v38) NiMaterialProperty;
		} else {
			pmatprop = 0;
		}
		pmatprop->SetEmittance(NiColor_WHITE);
		Main::spMenuBGGeometry.m_pObject->AttachProperty(pmatprop);
		NiTexturingProperty* v37 =
			static_cast<NiTexturingProperty*>(NiMemObject::operator new(sizeof(NiTexturingProperty)));
		if (v37) {
			ptexprop = ::new (v37) NiTexturingProperty;
		} else {
			ptexprop = 0;
		}
		NiRenderedTexture* Texture = Main::spMenuBGTexture.m_pObject->GetTexture(0);
		ptexprop->SetBaseTexture(Texture);
		ptexprop->SetBaseFilterMode(NiTexturingProperty::FILTER_NEAREST);
		ptexprop->SetBaseClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
		Main::spMenuBGGeometry.m_pObject->AttachProperty(ptexprop);
		Main::spMenuBGGeometry.m_pObject->UpdateProperties();
		FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData_2, nullptr, 0.0f, 0, 0);
		Main::spMenuBGGeometry.m_pObject->Update(UpdateData_2);
	}
	TESTerrainLODManager::InitGrass();
}

// One frame of the game loop: the menu and console state, the player, saves, timers and controls, the texture,
// fader and splatter updates, the world clock, the AI lists (on the main thread with one hardware thread, else
// started on the AI threads around the render), the visibility culling, the swap and the post-frame work. Tab+Alt
// held skips the frame. The exe keeps the alignment of the player's karma and remaps it (above evil: very good to 0,
// else 2) into a local it never reads; src only makes the call.
void Main::OnIdle() {
	UpdateTextures(fUpdateTexturesOnIdleTime);
	// The exe's GetAsyncKeyState(VK_TAB) and GetAsyncKeyState(VK_MENU), each held: SDL's keyboard state (VK_MENU is
	// either Alt key).
	const bool* pbKeys = SDL_GetKeyboardState(nullptr);
	if (pbKeys[SDL_SCANCODE_TAB] && (pbKeys[SDL_SCANCODE_LALT] || pbKeys[SDL_SCANCODE_RALT]))
		return;
	BSSystemUtility* pSystemUtility = &BSSystemUtility::QInstance();
	pSystemUtility->Update();
	bInMenuMode = Interface::IsInMenuMode() || Interface::IsPipBoyOpening();
	bInDialog = Interface::InDialog();
	bFaderToBlackVisible = pFaderManager->IsFaderVisible(FADER_TO_BLACK);
	bInConsole = Interface::IsConsoleVisible();
	bool bOtherRenderedMenu =
		Interface::GetCurrentRenderedMenu() && Interface::GetCurrentRenderedMenu() != Interface::GetPipboy();
	// The exe sign-extends the mode byte; 3 is the menu-entering mode.
	if (static_cast<int8_t>(InterfaceManager::Instance()->GetCurrentMode()) == 3 && !bOtherRenderedMenu)
		MemoryLevelManager::RunNonDestructiveFree();
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
	if (!bInMenuMode)
		BSFadeNode::IncFadeCounter();
	OnIdle_UpdateTimer();
	OnIdle_PollControls();
	pIOManager->UpdateQueue();
	if (!StartMenu::CreditsMenuIsActive()) {
		if (StartMenu::MainMenuIsActive())
			LoadingMenu::SuspendBackgroundThread();
		else if (Interface::IsInGameLoadingMenuOpen())
			TES::pInstance->ShowLoadingMenu(false, nullptr, false);
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
	if (NiParallelUpdateTaskManager::Get())
		NiParallelUpdateTaskManager::BeginUpdate();
	if (!bInMenuMode && !bFreezeTime) {
		if (TES::pInstance->CellTestRunning() && !TES::pInstance->CellTestRunning2())
			TES::pInstance->TestAllCells(0);
		BSSceneGraph* pWorldRoot = spWorldRoot.operator->();
		float fViewSeconds = appTimer.GetSeconds();
		pWorldRoot->SetViewDistanceBasedOnFrameRate(fViewSeconds);
		appCalendar.Update(appTimer.GetSeconds());
		if (iNumHWThreads.UInt() == 1)
			TES::pInstance->RunAnimations();
	}
	ReferenceProcessLists.kUpdateLock.Lock(nullptr);
	ReferenceProcessLists.RunActorScripts();
	if (iNumHWThreads.UInt() != 1) {
		if ((!bInMenuMode || bFaderToBlackVisible) && !bInConsole && !bFreezeTime) {
			ReferenceProcessLists.UpdateRadiationList();
			ReferenceProcessLists.ChangeProcessLevelTempList();
			ReferenceProcessLists.UpdateFollowerTempList();
		}
	} else if ((!bInMenuMode || bFaderToBlackVisible) && !bInConsole && !bFreezeTime) {
		ReferenceProcessLists.UpdateRadiationList();
		// The exe calls the empty function 0x8D0600 here with 0.0f and 0.
		ReferenceProcessLists.ChangeProcessLevelTempList();
		ReferenceProcessLists.UpdateFollowerTempList();
	}
	ReferenceProcessLists.kUpdateLock.Unlock();
	MemoryLevelManager::QInstance()->CheckMemoryLevel(true);
	if (BSTexturePalette::IsPartiallyPurged())
		BSTexturePalette::PurgeUnusedTextures();
	GarbageCollector::Update();
	GarbageCollector::ClearTempEffects();
	bool bTreesPaused = bInMenuMode || bFreezeTime;
	NiCamera* pTreesCamera = WorldRootCamera();
	BSTreeManager::Update(pTreesCamera, bTreesPaused);
	OnIdle_UpdateCurrentGridCell();
	if (iNumHWThreads.UInt() == 1)
		OnIdle_DoInterfaceIdle();
	ForceUpdateTextures();
	BGSDecalManager::GetSingleton()->UpdateDecals();
	WorldRootCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
	if (pVATS.GetMode() != VATS::VATS_PLAYBACK) {
		float fWorldFOV = pPlayer->GetWorldFOV();
		GetWorldSceneGraph()->SetCameraFOV(fWorldFOV, false, nullptr, true);
	}
	BSShaderManager::SetFOV(pPlayer->GetWorldFOV());
	NiCamera* pSceneCamera = spWorldRoot.operator->()->GetCamera();
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
		} else {
			ReferenceProcessLists.TaskManagerAI.StartTasksDuringRendering();
		}
	}
	OnIdle_UpdateAnimationsAndEffects();
	if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1 && !bRunThreads) {
		OnIdle_DoInterfaceIdle();
		if (Interface::IsInMenuMode())
			Interface::LastMinuteUpdate();
	}
	if (!bFreezeTime)
		PathManager::QInstance()->Update();
	NavMeshRender::GetInstance()->Update();
	if (iNumHWThreads.UInt() == 1) {
		if (bUseObstacleAvoidance.Bool())
			NavMeshObstacleManager::GetInstance()->Update();
		pCombatManager->Update(bInMenuMode || bFaderToBlackVisible || bFreezeTime);
	}
	if (NiParallelUpdateTaskManager::IsActive())
		NiParallelUpdateTaskManager::EndUpdate();
	// The exe calls UpdateSleeping with whatever ecx holds; the body never reads this, so src passes the menu.
	if (Interface::GetTopMenuID() == Interface::eSleepWaitMenu && SleepWaitMenu::pMe->UpdateSleeping())
		RenderMenuBackground();
	CheckWithinMultiBoundTask::DoAttachments();
	BSShaderManager::ProcessAllQueuedLights();
	// Actor value 23 is the karma.
	int32_t iKarma = static_cast<ActorValueOwner*>(pPlayer)->GetActorValue(static_cast<ActorValue::Index>(23));
	TESActorBaseData::GetAlignmentForKarma(static_cast<float>(iKarma));
	Swap();
	PostSwapProcess();
	if (Renderer::bDoResize) {
		Renderer::DoResize();
		Renderer::bDoResize = false;
	}
	if ((!bInMenuMode || bFaderToBlackVisible) && !bInConsole && !bFreezeTime) {
		if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1)
			AILinearTaskThreadManager::GetSingletonPtr()->WaitForThreads();
		else
			ReferenceProcessLists.TaskManagerAI.WaitForTasksDuringRendering();
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
		// The exe compares the zero-extended byte with 256, which never matches.
		if (static_cast<uint32_t>(ucSortFreeBlocksIndex) == 0x100u)
			ucSortFreeBlocksIndex = 0;
	}
}

void Main::OnIdle_ScaleLOD() {
	if (GetSceneIsComplex()) {
		BSFadeNode::SetLODFadeOutObjectMult(fLODFadeOutObjectMultComplex.Float());
		BSFadeNode::SetLODFadeOutItemMult(fLODFadeOutItemMultComplex.Float());
		BSFadeNode::SetLODFadeOutActorMult(fLODFadeOutActorMultComplex.Float());
	} else if (TES::pInstance->GetWorldSpace() &&
		TES::pInstance->GetWorldSpace()->GetParentWorld(TESWorldSpace::PU_LOD)) {
		BSFadeNode::SetLODFadeOutObjectMult(fLODFadeOutObjectMultCity.Float());
		BSFadeNode::SetLODFadeOutItemMult(fLODFadeOutItemMultCity.Float());
		BSFadeNode::SetLODFadeOutActorMult(fLODFadeOutActorMultCity.Float());
	} else if (pPlayer && pPlayer->GetParentCell() && pPlayer->GetParentCell()->GetInterior()) {
		BSFadeNode::SetLODFadeOutObjectMult(fLODFadeOutObjectMultInterior.Float());
		BSFadeNode::SetLODFadeOutItemMult(fLODFadeOutItemMultInterior.Float());
		BSFadeNode::SetLODFadeOutActorMult(fLODFadeOutActorMultInterior.Float());
	} else {
		BSFadeNode::SetLODFadeOutObjectMult(1.0f);
		BSFadeNode::SetLODFadeOutItemMult(1.0f);
		BSFadeNode::SetLODFadeOutActorMult(1.0f);
	}
}

// bfixNextTime is the function's static (the PDB's S_LDATA32); the exe calls MiddleHighProcess::ForceBoneScale (empty
// in this build) on every high process with 3D and on the player's process.
void Main::OnIdle_FixActorBones() {
	if (!bInMenuMode) {
		bfixNextTime = true;
		return;
	}
	if (bfixNextTime) {
		ProcessArray* pMobArray = ReferenceProcessLists.GetProcessArray();
		for (uint32_t i = 0; i < pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
			MobileObject* pmob = pMobArray->GetItem(i);
			if (pmob && pmob->GetCurrentProcessType() == PROCESS_TYPE_HIGH && pmob->Get3D())
				static_cast<MiddleHighProcess*>(pmob->GetCurrentAIProcess())->ForceBoneScale();
		}
		static_cast<MiddleHighProcess*>(pPlayer->GetCurrentAIProcess())->ForceBoneScale();
	}
	bfixNextTime = false;
}

void Main::OnIdle_UpdateTimer() {
	appTimer.SetUseGlobalTimeMultiplierTarget(bChangeTimeMultSlowly.Bool());
	if (!bInMenuMode)
		BSTimer::UpdateGlobalTimeMultiplier();
	appTimer.Update();
	bhkWorld::fTauRatio = fHavokTauRatio.Float();
	// The exe evaluates the arguments right to left.
	bool bSimulateEveryFrame = pVATS.GetMode() == VATS::VATS_PLAYBACK;
	bool bQuickSimulate = GetSceneIsComplex();
	float fDeltaTime = appTimer.GetSeconds();
	bhkWorld::SetDeltaTime(fDeltaTime, bQuickSimulate, bSimulateEveryFrame);
	BSShaderManager::UpdateTimers();
	double fAnimTime;
	if (bInMenuMode) {
		fAnimTime = 0.0;
	} else {
		double fSeconds = appTimer.GetSeconds();
		fAnimTime = fAnimationMult.Float() * fSeconds;
	}
	fFrameAnimTime = static_cast<float>(fAnimTime);
}

void Main::OnIdle_PollControls() {
	Controls::GetInstance()->UpdateValues();
	float fTime = appTimer.GetRealTimeSeconds();
	Controls::GetInstance()->Poll(fTime);
	if (!bFlyCamera && !pPlayer->IsDead(false) &&
		((pVATS.GetMode() == VATS::VATS_PLAYBACK && !pPlayer->IsInSlowMo()) || Interface::IsPipBoyOpening()) &&
		!Interface::IsMenuIDVisible(Interface::eMessageMenu, 0) && !StartMenu::PauseMenuIsActive())
		Controls::GetInstance()->ClearUserActions();
}

void Main::OnIdle_HandleMenuBackground() {
	if ((!Main::bInMenuMode) ||
		(Interface::IsTopMenuID(Interface::eDialogMenu) ||
			(StartMenu::MainMenuIsActive() ||
				(Interface::IsMenuIDVisible(Interface::eVATSMenu, static_cast<Interface::FADE_STATE>(11)) ||
					(Interface::IsMenuIDVisible(Interface::eMessageMenu, static_cast<Interface::FADE_STATE>(11)) ||
						(Interface::IsInPipboyMenu() ||
							(Interface::IsConsoleVisible() || (Main::bMenuBGReady || (!Main::bStaticMenuBG))))))))) {
		if ((((((((((Main::bMenuBGReady &&
						((!Main::bInMenuMode) ||
							(StartMenu::MainMenuIsActive() ||
								((Interface::IsConsoleVisible() && (!Interface::IsInRenderedMenu())) ||
									(Interface::IsTopMenuID(Interface::eDialogMenu) ||
										Interface::IsTopMenuID(Interface::eVATSMenu)))))) &&
					   (!pFaderManager->IsFaderActive(FADER_TO_BLACK))) &&
					  (!Interface::IsMenuIDVisible(Interface::eSurgeryMenu, 0))) &&
					 (!Interface::IsMenuIDVisible(Interface::eLockPickMenu, 0))) &&
					(!Interface::IsMenuIDVisible(Interface::eSPECIALBookMenu, 0))) &&
				   (!Interface::IsMenuIDVisible(Interface::eLoveTesterMenu, 0))) &&
				  (!Interface::IsMenuIDVisible(Interface::eSlotMachineMenu, 0))) &&
				 (!Interface::IsMenuIDVisible(Interface::eBlackJackMenu, 0))) &&
				(!Interface::IsMenuIDVisible(Interface::eRouletteMenu, 0))) &&
			(!Interface::IsMenuIDVisible(Interface::eCaravanMenu, 0))) {
			KillMenuBGTexture();
		}
	} else {
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
		if (pSpeaker && pSpeaker->IsActor())
			static_cast<Actor*>(pSpeaker)->UpdateNonRenderSafeDialogueUpdate(0.0f);
	}
	ReferenceProcessLists.UpdateHighListImpacts(0.0f);
	if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1)
		ReferenceProcessLists.ParallelUpdateAnimationScenegraph();
	else
		ReferenceProcessLists.UpdateAnimationScenegraph();
	ReferenceProcessLists.ReconcileDetectionDataForActor();
	ReferenceProcessLists.UpdateNonRenderSafeStuff(appTimer.GetSeconds());
	FOPipboyManager* pPipboy = Interface::GetPipboy();
	if (pPipboy->GetPipBoyManagerReset())
		pPipboy->ResetPipBoyManager();
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
	if (iNumHWThreads.UInt() == 1)
		ReferenceProcessLists.ChangeProcessLevelTempList();
	if ((!bInMenuMode || bFaderToBlackVisible) && !bInConsole && !bFreezeTime) {
		if (iNumHWThreads.UInt() == 1) {
			ReferenceProcessLists.TaskManagerAI.UpdateTasks();
			pCombatManager->UpdateCombatants();
		}
		ReferenceProcessLists.UpdateProcessLists();
	} else if (bInMenuMode || bFreezeTime) {
		pPlayer->UpdateAutoAimActor();
	}
}

// The exe reads the player's position through TESObjectREFR::GetLocationOnReference called non-virtually (0x436AA0,
// folded with BSPortalGraph::GetMBRoomAccumList).
void Main::OnIdle_UpdatePlayer() {
	if (pPlayer->HandlePositionPlayerRequest())
		return;
	if (bInMenuMode && !Interface::IsPipBoyOpening()) {
		pPlayer->ForceGrenadeHold();
		return;
	}
	if (bFlyCamera) {
		if (pPlayer->Get3D())
			pPlayer->UpdateFlyCamera(appTimer.GetSeconds(), bFreezeTime);
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
	if (!pCell || pCell->GetInterior() || pCell->IsPointInCellExterior(kPos))
		return;
	TESObjectCELL* pNewCell = pDataHandler->GetCellFromWorldCoord(kPos.x, kPos.y, pCell->GetWorldSpace(), true);
	if (!pNewCell)
		return;
	if (!pNewCell->IsLoaded() && !pNewCell->GetAttached()) {
		TES::pInstance->UpdateCurrentGridCell(&kPos, true);
		if (TES::pInstance->CellTestRunning())
			TES::pInstance->ShowLoadingMenu(false, nullptr, false);
	}
	FalloutAudio::DisableRegionSoundUpdates(true);
	pPlayer->SetAcousticSpaceTransition(true);
	pNewCell->AddReference(pPlayer, false);
	FalloutAudio::SetNextAcousticSpace(pNewCell->GetAcousticSpace());
	pPlayer->SetAcousticSpaceTransition(false);
	FalloutAudio::DisableRegionSoundUpdates(false);
	BSShaderAccumulator* pAccumulator = BSShaderManager::GetAccumulator();
	if (pAccumulator)
		pAccumulator->ClearAllBoundVolumes();
}

// The exe reads the player's position through TESObjectREFR::GetLocationOnReference called non-virtually (0x436AA0,
// folded with BSPortalGraph::GetMBRoomAccumList). The two world space lookups' results are unused in the exe too.
void Main::OnIdle_UpdateCurrentGridCell() {
	if (!bInMenuMode && !Interface::bIsLoadingNewGame && !bFreezeTime) {
		TES::pInstance->UpdateCurrentGridCell(pPlayer->QLocationOnReference(), true);
		if (TES::pInstance->CellTestRunning() && !TES::pInstance->GetInterior()) {
			pPlayer->GetWorldSpace();
			TES::pInstance->GetWorldSpace();
		}
	}
}

// Both branches of the exe's thread-count test call UpdateParallel the same way.
void Main::OnIdle_UpdateAnimationsAndEffects() {
	if ((!bInMenuMode || bInDialog || bFaderToBlackVisible) && !bFreezeTime) {
		if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1)
			ReferenceProcessLists.UpdateTempEffectsParallel(appTimer.GetSeconds());
		else
			ReferenceProcessLists.UpdateTempEffects(appTimer.GetSeconds());
		if (iNumHWThreads.UInt() == 1 && !bInMenuMode)
			TES::pInstance->UpdateCellAnimations(fFrameAnimTime);
		else
			TES::pInstance->UpdateCellMainThread();
		NiUpdateData kUpdateData;
		kUpdateData.~NiUpdateData();
		::new (&kUpdateData) NiUpdateData(fFrameAnimTime, true, false);
		if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1)
			GetMasterParticleSystemManager()->UpdateParallel(kUpdateData);
		else
			GetMasterParticleSystemManager()->UpdateParallel(kUpdateData);
	}
}

void Main::OnIdle_DoInterfaceIdle() {
	Interface::PreIdleStuff();
	Interface::Idle();
	Interface::PostIdleStuff();
}

// The exe's cleanup-only C++ EH frame is left out.
void Main::OnIdle_UpdateImageSpace(bool abAdvanceTime) {
	ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
	ImageSpaceManager_->ResetToBaseParameterData();
	ImageSpaceManager* v3 = BSShaderManager::GetImageSpaceManager();
	v3->BeginParamModifiers();
	if (abAdvanceTime) {
		float afAmount = appTimer.fDelta;
		TESImageSpaceModifier::AdvanceCurrentTime(afAmount);
	}
	BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* plistmodifiers =
		&TES::pInstance->listActiveImageSpaceModifiers;
	int32_t iweatherCount = 0;
	int32_t iactiveWeatherCount = 0;
	while (plistmodifiers) {
		ImageSpaceModifierInstance* pimagespacemodinstance = plistmodifiers->m_item.m_pObject;
		if (!pimagespacemodinstance) {
			goto LABEL_14;
		}
		if (pimagespacemodinstance->bIsWeather) {
			++iweatherCount;
		}
		if (pimagespacemodinstance->IsExpired()) {
			NiPointer<ImageSpaceModifierInstance> spInstance(pimagespacemodinstance);
			plistmodifiers->Remove(spInstance);
		} else {
			if (TESImageSpaceModifier::GetEnable()) {
				pimagespacemodinstance->Apply();
				if (pimagespacemodinstance->fStrength > 0.0 && pimagespacemodinstance->bIsWeather) {
					++iactiveWeatherCount;
				}
			}
		LABEL_14:;
			plistmodifiers = plistmodifiers->m_pkNext;
			if (iweatherCount == 4) {
				if (!iactiveWeatherCount) {
					ImageSpaceManager* v4 = BSShaderManager::GetImageSpaceManager();
					v4->ResetWeatherData();
				}
				ImageSpaceManager* v5 = BSShaderManager::GetImageSpaceManager();
				v5->EndWeatherModifiers();
				iweatherCount = 0;
			}
		}
	}
	ImageSpaceManager* v6 = BSShaderManager::GetImageSpaceManager();
	v6->EndParamModifiers();
	ImageSpaceManager* v7 = BSShaderManager::GetImageSpaceManager();
	v7->ApplyCurrentParameterData(0);
}

// The exe tests the shared `return true` stub 0x5D4A40 (ReturnTrue) for the debug texture, comparing its byte with 1.
void Main::Swap() {
	char PathName[268];
	NiRenderer* pr = NiRenderer::GetRenderer();
	static_cast<NiDX9Renderer*>(pr)->NiDX9Renderer::LockRenderer();
	if (Interface::bStartPipboyRenderBackground) {
		Interface::bStartPipboyRenderBackground = 0;
		NiAVObject* v1 = pPlayer->Get3D(1);
		bool b1cull = v1->GetAppCulled();
		NiAVObject* v2 = pPlayer->Get3D(0);
		bool b3cull = v2->GetAppCulled();
		NiAVObject* v3 = pPlayer->Get3D(1);
		v3->SetAppCulled(1);
		NiAVObject* v4 = pPlayer->Get3D(0);
		v4->SetAppCulled(1);
		RenderMenuBackground();
		NiAVObject* v5 = pPlayer->Get3D(1);
		v5->SetAppCulled(b1cull);
		NiAVObject* v6 = pPlayer->Get3D(0);
		v6->SetAppCulled(b3cull);
	}
	NiRenderer* Renderer_ = NiRenderer::GetRenderer();
	Renderer_->GetSorter();
	InitializationFlow();
	MTRenderingSystem::QInstance()->StartThreads();
	if (Renderer::bGammaDirty) {
		Renderer::UpdateGamma();
	}
	if (pFaderManager->IsFaderVisible(FADER_TO_BLACK) || pFaderManager->IsFaderVisible(FADER_TO_WHITE)) {
		if (!iBlackFaderFrames) {
			TES::pInstance->ShowLoadingMenu(0, 0, 0);
			InitializationFlow();
			ReferenceProcessLists.SkipHighListFadeIn();
		}
		fBlackFaderTime = appTimer.GetRealTimeSeconds() + fBlackFaderTime;
		++iBlackFaderFrames;
		if (fBlackFaderTime >= 1.0 && iBlackFaderFrames >= 10) {
			pFaderManager->RemoveFader(FADER_TO_BLACK, 0);
			pFaderManager->RemoveFader(FADER_TO_WHITE, 0);
			pSaveLoadGameOLD->SetLoading(0);
			if (pSaveLoadGameOLD->GetLoading()) {
				pSaveLoadGameOLD->SetLoading(0);
			}
			KillMenuBGTexture();
		}
	} else {
		fBlackFaderTime = 0.0f;
		iBlackFaderFrames = 0;
	}
	BSRenderedTexture* p3DInterfaceTex = 0;
	if (Main::bMenuBGReady ||
		(StartMenu::MainMenuIsActive() || (!pPlayer->Get3D() || pFaderManager->IsFadingIn(FADER_TO_BLACK)))) {
		if ((!Interface::IsInRenderedMenu() && Interface::IsInMenuMode()) ||
			Interface::IsMenuIDVisible(Interface::eStartMenu, 0)) {
			for (int32_t i = 1; i < 23; ++i) {
				MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, i);
				MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, i);
			}
			UpdateOffscreenInterface();
			if (Interface::IsMenuIDVisible(Interface::eSurgeryMenu, 0) ||
				(Interface::IsMenuIDVisible(Interface::eLockPickMenu, 0) ||
					(Interface::IsMenuIDVisible(Interface::eSPECIALBookMenu, 0) ||
						(Interface::IsMenuIDVisible(Interface::eLoveTesterMenu, 0) ||
							(Interface::IsMenuIDVisible(Interface::eSlotMachineMenu, 0) ||
								(Interface::IsMenuIDVisible(Interface::eBlackJackMenu, 0) ||
									(Interface::IsMenuIDVisible(Interface::eRouletteMenu, 0) ||
										Interface::IsMenuIDVisible(Interface::eCaravanMenu, 0)))))))) {
				DrawWorldMenu3D(0);
			} else {
				if (Main::spMenuBGTexture.m_pObject) {
					bool MultiSampleEnabled = Renderer::GetMultiSampleEnabled();
					ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
					DrawWorld_CopyMenuBG(ImageSpaceManager_, static_cast<NiDX9Renderer*>(pr), MultiSampleEnabled, 0);
				}
			}
		} else {
			if (pPlayer->Get3D()) {
				bool v8 = Interface::IsInRenderedMenu();
				DrawWorld(0, v8, 0);
			}
		}
	} else {
		if (!TES::pInstance->pInteriorCell) {
			if (*TES::pInstance->pWorldSpace->GetCanopyShadowTextureName()) {
				if (TES::pInstance->pGridCellA) {
					if (GridCellArray::GetRecreateCanopyShadowMaskTexture()) {
						TES::pInstance->pGridCellA->RecreateCanopyShadowMaskTexture();
					}
				}
			}
		}
		DrawWorld(0, 0, 0);
	}
	if (ReturnTrue() == true) {
		BSShaderManager::RenderDebugTexture();
	}
	Interface::DrawHUDMenu3D();
	Interface::Click(p3DInterfaceTex, Main::spInterfaceFXTex.m_pObject);
	MTRenderingSystem::QInstance()->WaitForThreads();
	if (Main::spInterfaceFXTex.m_pObject) {
		BSRenderedTexture* v15 = Main::spInterfaceFXTex.m_pObject;
		BSTextureManager* TextureManager = BSShaderManager::TextureManagerPtr();
		TextureManager->ReturnRenderedTexture(v15);
		Main::spInterfaceFXTex.operator=(static_cast<BSRenderedTexture*>(0));
	}
	Controls* Instance = Controls::GetInstance();
	if (Instance->GetUserAction(Controls::UACTN_SCREENSHOT, 1)) {
		Controls* v12 = Controls::GetInstance();
		if (v12->GetKeyboardButton(157, 0) || bMultiScreenshotQueued) {
			bMultiScreenshotQueued = !bMultiScreenshotQueued;
			if (bMultiScreenshotQueued) {
				iIdleSectionCount = 0;
				gs_iTake_CameraPath.operator=(gs_iTake_CameraPath.Int() + 1);
				BSsprintf(PathName, 260, "%s%03d", gs_sDirectoryName_CameraPath.String(), gs_iTake_CameraPath.Int());
				std::error_code kError;
				std::filesystem::create_directory(platform::NativePath(PathName), kError);
				appTimer.ClampAt(static_cast<uint32_t>(gs_iFPS_CameraPath.Int()));
			} else {
				appTimer.ClampAt(0);
			}
		} else {
			ScreenShot::TakeScreenShot(0);
		}
	}
	NiObjectGarbage::Update();
	if (pr->GetFrameState()) {
		BSRenderedTexture::Stop();
	}
	if (p3DInterfaceTex) {
		BSRenderedTexture* v16 = p3DInterfaceTex;
		BSTextureManager* v13 = BSShaderManager::TextureManagerPtr();
		v13->ReturnRenderedTexture(v16);
		p3DInterfaceTex = 0;
	}
	Main::spWorldRoot.m_pObject->SetCameraScene();
	static_cast<NiDX9Renderer*>(pr)->NiDX9Renderer::UnlockRenderer();
	if (iNumHWThreads.Int() > 1) {
		AILinearTaskThreadManager::SetMainRendering(0);
	}
}

void Main::PostSwapProcess() {
	OnIdle_UpdateAudio();
	OnIdle_UpdateProcessLists();
	if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1)
		TESObjectCELL::RunQueuedParticleUpdates();
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
	// The exe builds the by-value argument before it fetches the scene node.
	NiPoint3 kEyePosition = GetWorldSceneGraph()->GetCamera()->GetWorldTranslate();
	BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->SetEyePosition(kEyePosition);
	if (abRenderedMenuMode && !Interface::IsMenuIDVisible(Interface::eStartMenu, 0))
		DrawWorldRenderedMenu(apDestination, bWireframe);
	else if (bWireframe)
		DrawWorldWireframe(apDestination, abSkipFirstPerson);
	else
		DrawWorldStandard(apDestination, abSkipFirstPerson);
}

void Main::DrawWorldRenderedMenu(BSRenderedTexture* apDestination, bool bwireframe) {
	NiGeometry* prenderedMenuGeom;
	ImageSpaceManager* pimagespacemanager = BSShaderManager::GetImageSpaceManager();
	NiDX9Renderer* prenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
	bool bmultiSampleEnabled = Renderer::GetMultiSampleEnabled();
	if (Interface::GetCurrentRenderedMenu()) {
		prenderedMenuGeom = Interface::GetCurrentRenderedMenu()->GetScreenGeometry();
	} else {
		prenderedMenuGeom = 0;
	}
	DrawWorld_WaitForParallelUpdate();
	for (int32_t i = 1; i < 17; ++i) {
		MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, i);
		MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, i);
	}
	DrawWorld_UpdateOffscreenBuffers();
	BSRenderedTexture* apRenderTexture =
		DrawWorld_InitRenderTexture(prenderer, apDestination, bwireframe, bmultiSampleEnabled);
	DrawWorld_CopyMenuBG(pimagespacemanager, prenderer, bmultiSampleEnabled, apDestination);
	if (iNumHWThreads.Int() > 1) {
		AILinearTaskThreadManager::SetMainRendering(0);
	}
	MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 21);
	MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 21);
	if (Interface::IsInPipboyMenu()) {
		NiCamera* pcamera = Main::WorldRootCamera();
		BSCullingProcess* pculler = Main::GetWorldSceneGraph()->pCuller;
		pculler->SetAccumulator(spDraw1stPersonAccum.m_pObject);
		pculler->m_pkCamera = pcamera;
		ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
		ShadowSceneNode_->PreOnVisible(*pculler);
		DrawWorld_Setup1stPerson();
		DrawWorld_MTRender1stPerson(prenderer, prenderedMenuGeom, 0, apRenderTexture);
	} else {
		MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 22);
		MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 22);
	}
	DrawWorld_RenderedMenuScene();
	if (!BSShaderManager::GetImageSpaceEffects() || bwireframe) {
		if (apDestination) {
			DrawWorld_StopRenderTarget(bmultiSampleEnabled, apDestination);
		}
	} else {
		DrawWorld_ImageSpace(bmultiSampleEnabled, apRenderTexture, prenderer, apDestination);
		if (prenderedMenuGeom) {
			DrawWorld_RenderedMenu(prenderedMenuGeom, prenderer);
		}
	}
	DrawWorld_End(apRenderTexture, pimagespacemanager, prenderer, 0);
}

void Main::DrawWorldWireframe(BSRenderedTexture* apDestination, bool abSkipFirstPerson) {
	NiDX9Renderer* prenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
	ImageSpaceManager* pimagespacemanager = BSShaderManager::GetImageSpaceManager();
	NiAVObject* v3 = pPlayer->Get3D(1);
	bool b1stPersonCulled = v3->GetAppCulled();
	bool bmultiSampleEnabled = Renderer::GetMultiSampleEnabled();
	BSRenderState::SetWireframeEnable(1, BSRS_LOCK);
	DrawWorld_WaitForParallelUpdate();
	Sun* psun = DrawWorld_Init();
	DrawWorld_UpdateWater();
	bool bissuesVATSQuery = DrawWorld_VATSOcclusionQuery();
	DrawWorld_UpdateOffscreenBuffers();
	BSRenderedTexture* apRenderTexture = DrawWorld_InitRenderTexture(prenderer, apDestination, 0, bmultiSampleEnabled);
	bool bdo1stPerson = !b1stPersonCulled &&
		(!abSkipFirstPerson && (!Interface::GetVATSEffect()->GetTarget() && !DialogMenu::IsDialogActive()));
	DrawWorld_DrawScene(psun, bdo1stPerson, 1, bissuesVATSQuery);
	if (bdo1stPerson) {
		DrawWorld_Setup1stPerson();
		DrawWorld_RestartRenderTexture(apRenderTexture, bmultiSampleEnabled, 0);
		DrawWorld_MTRender1stPerson(prenderer, 0, psun, apRenderTexture);
	} else {
		MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 22);
		MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 22);
	}
	if (apDestination) {
		DrawWorld_StopRenderTarget(bmultiSampleEnabled, apDestination);
	}
	BSRenderState::RestoreWireframeEnable(BSRS_LOCK);
	if (BSShaderManager::GetImageSpaceEffects()) {
		DrawWorld_ImageSpace(bmultiSampleEnabled, apRenderTexture, prenderer, apDestination);
	} else {
		if (apDestination) {
			DrawWorld_StopRenderTarget(bmultiSampleEnabled, apDestination);
		}
	}
	DrawWorld_End(apRenderTexture, pimagespacemanager, prenderer, 0);
}

void Main::DrawWorldStandard(BSRenderedTexture* apDestination, bool abSkipFirstPerson) {
	NiPoint2 v14;
	NiRect<float> pkR;
	NiDX9Renderer* prenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
	ImageSpaceManager* pimagespacemanager = BSShaderManager::GetImageSpaceManager();
	NiAVObject* v3 = pPlayer->Get3D(1);
	bool b1stPersonCulled = v3->GetAppCulled();
	bool bmultiSampleEnabled = Renderer::GetMultiSampleEnabled();
	DrawWorld_WaitForParallelUpdate();
	Sun* psun = DrawWorld_Init();
	DrawWorld_UpdateWater();
	bool bissuesVATSQuery = DrawWorld_VATSOcclusionQuery();
	DrawWorld_UpdateOffscreenBuffers();
	BSRenderedTexture* apRenderTexture = DrawWorld_InitRenderTexture(prenderer, apDestination, 0, bmultiSampleEnabled);
	DrawWorld_VATS(bissuesVATSQuery);
	DrawWorld_RestartRenderTexture(apRenderTexture, bmultiSampleEnabled, 5u);
	bool bdo1stPerson = !b1stPersonCulled &&
		(!abSkipFirstPerson && (!Interface::GetVATSEffect()->GetTarget() && !DialogMenu::IsDialogActive()));
	bool bglareocclude = 0;
	if (bdo1stPerson) {
		if (psun) {
			bglareocclude = psun->GetSunGlareNode()->GetAppCulled();
			psun->GetSunGlareNode()->SetAppCulled(1);
		}
	}
	bool useScopeScissoring = !IsEyefinity();
	bool bgunscope = Interface::IsGunScopeVisible();
	if (bgunscope) {
		uint32_t uiw = prenderer->NiRenderer::GetScreenWidth();
		uint32_t uih = prenderer->NiRenderer::GetScreenHeight();
		float* v4 = &fScopeScissorAmount.Float();
		float fhalfamt = Interface::GetAspectRatioRatio() * *v4 * 0.5;
		float fx0 = static_cast<double>(uiw) * fhalfamt;
		float fx1 = static_cast<double>(uiw) - fx0;
		if (useScopeScissoring) {
			BSShaderManager::EnableFullScreenScissor(static_cast<int64_t>(fx0), 0, static_cast<int64_t>(fx1), uih);
		}
	}
	DrawWorld_DrawScene(psun, bdo1stPerson, 0, bissuesVATSQuery);
	if (bdo1stPerson) {
		if (psun) {
			psun->GetSunGlareNode()->SetAppCulled(bglareocclude);
		}
	}
	ImageSpaceEffectDepthOfField* pdofeffect = static_cast<ImageSpaceEffectDepthOfField*>(pimagespacemanager
			->GetEffect(ImageSpaceManager::EFFECT_DEPTH_OF_FIELD));
	bool bisdepthoffieldactive = pdofeffect->DoDepth();
	bool bUseSunbeams = pimagespacemanager->IsEffectActive(ImageSpaceManager::EFFECT_SUNBEAMS) &&
		(!TES::pInstance->pInteriorCell && ::bUseSunbeams.Bool());
	bool bsunbeamsactive = bUseSunbeams;
	bool bdovolumetricfog = 0;
	bool bdovelocity = 0;
	bool b1stpersonwait = 0;
	if (bisdepthoffieldactive && bdo1stPerson) {
		b1stpersonwait = 1;
		DrawWorld_Add1stPersonToDepthBuffer(apRenderTexture);
		DrawWorld_RestartRenderTexture(apRenderTexture, bmultiSampleEnabled, 0);
	}
	if (BSShaderManager::IsRefractionEnabled()) {
		if (spDrawWorldAccum.m_pObject->GetRefractivePassCount()) {
			DrawWorld_Refraction(spDrawWorldAccum.m_pObject, apRenderTexture);
		}
	}
	if (bdo1stPerson) {
		DrawWorld_Setup1stPerson();
		DrawWorld_RestartRenderTexture(apRenderTexture, bmultiSampleEnabled, 0);
		if (!b1stpersonwait) {
			MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 22);
		}
		DrawWorld_MTRender1stPerson(prenderer, 0, psun, apRenderTexture);
	} else {
		MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 22);
		MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 22);
	}
	if (bisdepthoffieldactive) {
		DrawWorld_DoDepth(spDrawWorldAccum.m_pObject, apRenderTexture);
	}
	if (bsunbeamsactive && !TES::pInstance->pInteriorCell) {
		if (Main::pSky) {
			if (Main::pSky->pSun) {
				Sun* pSun = Main::pSky->pSun;
				NiPoint3 sunPos = pSun->GetRoot()->GetWorldTranslate();
				float sunz = 0.0f;
				float suny = 0.0f;
				float sunx = 0.0f;
				SceneGraph* psg = Main::GetWorldSceneGraph();
				bool btrans = psg->GetCamera()->WorldPtToScreenPt3(sunPos, sunx, suny, sunz, 1e-05f);
				NiPoint2* pSunPos = ::new (&v14) NiPoint2(sunx, suny);
				ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
				ImageSpaceManager_->SetSunPosition(*pSunPos);
			}
		}
	}
	if (BSShaderManager::GetImageSpaceEffects()) {
		DrawWorld_ImageSpace(bmultiSampleEnabled, apRenderTexture, prenderer, apDestination);
		if ((ScreenCustomSplatter::IsActiveSplatter() || ScreenSplatter::IsActiveSplatter()) &&
			!Interface::IsInPipboyMenu()) {
			DrawWorld_ScreenSplatter(prenderer);
		}
	} else {
		if (apDestination) {
			DrawWorld_StopRenderTarget(bmultiSampleEnabled, apDestination);
		}
	}
	if (bgunscope) {
		if (useScopeScissoring) {
			BSShaderManager::DisableFullScreenScissor();
		}
		if (IsEyefinity()) {
			NiRenderer::GetRenderer()->SetBackgroundColor(NiColorA_BLACK);
			::new (&pkR) NiColorA(0.0f, 0.0f, 0.0f, 0.0f);
			pkR.m_left = 0.0f;
			pkR.m_right = 0.33f;
			pkR.m_top = 1.0f;
			pkR.m_bottom = 0.0f;
			NiRenderer* Renderer_ = NiRenderer::GetRenderer();
			Renderer_->ClearBuffer(&pkR, 1u);
			pkR.m_left = 0.66f;
			pkR.m_right = 1.0f;
			pkR.m_top = 1.0f;
			pkR.m_bottom = 0.0f;
			NiRenderer* v7 = NiRenderer::GetRenderer();
			v7->ClearBuffer(&pkR, 1u);
		}
	}
	DrawWorld_End(apRenderTexture, pimagespacemanager, prenderer, psun);
}

void Main::DrawWorld_CopyMenuBG(ImageSpaceManager* apImagespaceManager, NiDX9Renderer* apRenderer,
	bool abMultiSampleEnabled, BSRenderedTexture* apDestination) {
	BSRenderedTexture* v5;
	BSRenderedTexture* vCondition1;
	vCondition1 = (&Main::spMenuBGTexture)->operator->();
	if (vCondition1) {
		BSRenderState::SetColorWriteEnable(7u, BSRS_LOCK);
		BSRenderState::SetAlphaBlendEnable(0, BSRS_LOCK);
		v5 = (&Main::spMenuBGTexture)->operator->();
		apImagespaceManager->RenderEffect(ImageSpaceManager::EFFECT_SHADER_COPY, apRenderer, v5,
			(!abMultiSampleEnabled) ? apDestination : (nullptr), nullptr, 1);
		BSRenderState::RestoreAlphaBlendEnable(BSRS_LOCK);
		BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
	}
}

void Main::DrawWorld_StopRenderTarget(bool abMultiSampleEnabled, BSRenderedTexture* apDestination) {
	BSRenderedTexture::StopOffscreen();
}

// The exe's cleanup-only C++ EH frame is left out.
void Main::DrawWorld_End(BSRenderedTexture* apRenderTexture, ImageSpaceManager* apImagespaceManager,
	NiDX9Renderer* apRenderer, Sun* apSun) {
	SceneGraph* apSceneGraph = Main::GetWorldSceneGraph();
	if (apSun) {
		apSun->StopSunOcclusion();
	}
	BSShaderProperty::PreviousTransform::SaveCamera(apSceneGraph->GetCamera());
	if (apRenderTexture) {
		BSTextureManager* TextureManager = BSShaderManager::TextureManagerPtr();
		TextureManager->ReturnRenderedTexture(apRenderTexture);
	}
	spDrawWorldAccum.m_pObject->ClearRenderedGeometryCache();
	spScreenSplatterAccum.m_pObject->ClearBloodSplatter();
	BSShaderManager::ReturnSceneDepthTexture();
	BSShaderManager::ReturnVelocityBuffers();
	ImageSpaceEffectDepthOfField* Effect = static_cast<ImageSpaceEffectDepthOfField*>(apImagespaceManager
			->GetEffect(ImageSpaceManager::EFFECT_DEPTH_OF_FIELD));
	Effect->ReturnMaskTexture();
	if (BSShaderManager::bSLIMode) {
		// The exe calls the HDR effect's Shutdown (0xBAD4A0, ClearAdaptedLight's body) with no this set up: the body
		// ignores it.
		ImageSpaceEffectHDR::ClearAdaptedLight();
	}
	NiDX9Renderer* Renderer_ = BSShaderManager::GetRenderer();
	IDirect3DDevice9* pD3DDevice = Renderer_->GetD3DDevice();
	pD3DDevice->SetIndices(0);
	pD3DDevice->SetStreamSource(0, 0, 0, 0);
	if (NiParallelUpdateTaskManager::Get()) {
		NiParallelUpdateTaskManager::SignalRenderSema();
	}
	if (BSShaderAccumulator::spRenderPassesScenegraph.m_pObject) {
		NiTreeCtrl* apMemory = static_cast<NiTreeCtrl*>(ourMalloc(sizeof(NiTreeCtrl)));
		if (apMemory) {
			::new (apMemory) NiTreeCtrl(hInstance, hWnd, BSShaderAccumulator::spRenderPassesScenegraph.m_pObject,
				"Render Passes", 0x80000000u, 0x80000000u, 800, 600);
		}
		BSShaderAccumulator::spRenderPassesScenegraph.operator=(static_cast<NiNode*>(0));
	}
	BSBatchRenderer::EndPass();
}

void Main::DrawWorld_UpdateOffscreenBuffers() {
	if (BSShaderManager::GetShadows() && Main::GetSceneIsComplex()) {
		bSavedShadowState = BSShaderManager::GetShadows();
		BSShaderManager::EnableShadows(0);
		ShadowSceneNode* pssnode = BSShaderManager::GetShadowSceneNode(0);
		pssnode->ReleaseAllShadowMaps();
		NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
		BSTextureManager* TextureManager = BSShaderManager::TextureManagerPtr();
		TextureManager->SetFrustumShadowTextureCount(pRenderer, 0);
	} else {
		if (bSavedShadowState && !Main::GetSceneIsComplex()) {
			int32_t iMaxActorShadows;
			bSavedShadowState = 0;
			BSShaderManager::EnableShadows(1);
			if (TES::pInstance->pInteriorCell) {
				iMaxActorShadows = Renderer::iActorShadowCountInt.Int();
			} else {
				iMaxActorShadows = Renderer::iActorShadowCountExt.Int();
			}
			NiDX9Renderer* pRenderera = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
			BSTextureManager* v5 = BSShaderManager::TextureManagerPtr();
			v5->SetFrustumShadowTextureCount(pRenderera, static_cast<uint32_t>(iMaxActorShadows));
		}
	}
	if (BSShaderManager::GetShadows() && Renderer::QDoShadows()) {
		int32_t* v6;
		BSShaderManager::ResetNextAvailableShadowMapAccumulator();
		ShadowSceneNode* pssnode_2 = BSShaderManager::GetShadowSceneNode(0);
		pssnode_2->DisableAllShadowLights();
		if (TES::pInstance->pInteriorCell) {
			v6 = &Renderer::iActorShadowCountInt.Int();
			int32_t v7 = *v6;
		} else {
			v6 = &Renderer::iActorShadowCountExt.Int();
			int32_t v8 = *v6;
		}
		int32_t imaxActorShadows_3 = *v6;
		NiDX9Renderer* pRendererb = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
		BSTextureManager* v9 = BSShaderManager::TextureManagerPtr();
		v9->SetFrustumShadowTextureCount(pRendererb, static_cast<uint32_t>(imaxActorShadows_3));
		if (imaxActorShadows_3) {
			ShadowSceneLight* pplayerShadow = 0;
			int32_t icount = 0;
			if ((pPlayer->Is3rdPerson() || pPlayer->IsVanityMode()) && !pPlayer->IsRefractive()) {
				NiAVObject* v10 = pPlayer->Get3D(0);
				pplayerShadow = pssnode_2->AddShadowLight(v10);
				++icount;
			}
			ReferenceProcessLists.kNearbyActorLock.Lock(0);
			Actor** pcloseActorsA = ReferenceProcessLists.pActorsCloseToPlayerA;
			int32_t inumCloseActors = ReferenceProcessLists.iActorsCloseToPlayer;
			for (int32_t icurCloseActor = 0; icurCloseActor < inumCloseActors && icount < imaxActorShadows_3;
				++icurCloseActor) {
				Actor* pactor = pcloseActorsA[icurCloseActor];
				if (pactor->Get3D()) {
					if (pactor->Get3D()->m_pkParent) {
						if (pactor->HasShadow() && pactor->GetSitSleepState() != SIT_SLEEP_IS_SITTING &&
							!pactor->IsRefractive()) {
							NiPoint3* pBound;
							const NiFixedString* pExtraBoundTag = &BSBound::Tag();
							NiNode* v11 = pactor->Get3D();
							BSBound* pbound = static_cast<BSBound*>(v11->GetExtraData(*pExtraBoundTag));
							if (pbound) {
								pBound = &pbound->Extents;
							} else {
								pBound = &NiPoint3_ZERO;
							}
							float x = pBound->x;
							float y = pBound->y;
							float z = pBound->z;
							float pRendererc = BSMAX(x, y);
							float fbradius = BSMAX(pRendererc, z);
							NiNode* v12 = pactor->Get3D();
							const NiBound* WorldBound = &v12->GetWorldBound();
							if (WorldBound->m_fRadius < fbradius * 3.0) {
								NiNode* v14 = pactor->Get3D();
								pssnode_2->AddShadowLight(v14);
								++icount;
							}
						}
					}
				}
			}
			ReferenceProcessLists.kNearbyActorLock.Unlock();
			pssnode_2->SortShadowLights(pplayerShadow);
			NiAVObject* p1stPerson3d = pPlayer->Get3D(1);
			if (p1stPerson3d) {
				SceneGraph* psg = Main::GetWorldSceneGraph();
				BSCullingProcess* pculler = psg->pCuller;
				pculler->m_pkCamera = psg->GetCamera();
				const NiFrustum* ViewFrustum = &psg->GetCamera()->GetViewFrustum();
				pculler->SetFrustum(*ViewFrustum);
				bool b1stcull = p1stPerson3d->GetAppCulled();
				p1stPerson3d->SetAppCulled(1);
				NiAVObject* pgnode = (NiAVObject*)TESTerrainLODManager::GetGrassNode();
				BSFadeNode::SetFadeEnabled(0);
				bool bgrasscull = 0;
				if (pgnode) {
					bgrasscull = pgnode->GetAppCulled();
					if (!Renderer::bShadowsOnGrass.Bool()) {
						pgnode->SetAppCulled(1);
					}
				}
				ShadowSceneLight* psslight = pssnode_2->GetFirstShadowLight();
				int32_t ienabledShadows = 0;
				while (psslight) {
					if (psslight->GetLight()) {
						if (psslight->GetShadowScene()) {
							NiLight* v19;
							NiAVObject* v20;
							NiAVObject* v21;
							NiBound kbound;
							NiPoint3 kShadowCasterPos;
							if (!psslight->IsFadedOut()) {
								NiLight* Light = psslight->GetLight();
								if (Light->GetAppCulled()) {
									NiAVObject* ShadowScene = psslight->GetShadowScene();
									if (ShadowScene->GetAppCulled()) {
										psslight->SetFadeTarget(0.0f, 1);
									} else {
										if (ienabledShadows < imaxActorShadows_3) {
											NiLight* v18 = psslight->GetLight();
											v18->SetAppCulled(0);
										}
										psslight->SetFadeTarget(0.0f, 0);
									}
								}
							}
							if (ienabledShadows >= imaxActorShadows_3 ||
								((v19 = psslight->GetLight(), v19->GetAppCulled()) ||
									((v20 = psslight->GetShadowScene(), v20->GetAppCulled()) ||
										((v21 = psslight->GetShadowScene(),
											 (kbound = v21->GetWorldBound(),
												 (kShadowCasterPos = psslight->GetShadowScene()->GetWorldTranslate(),
													 (psslight->ComputeWeightedShadowLight(0, kShadowCasterPos),
														 (psslight->TestFrustumCull(*pculler),
															 psslight->GetFrustumCull() == 255))))) ||
											!(psslight->fShadowLODDimmer >= 0.05))))) {
								NiLight* v26 = psslight->GetLight();
								v26->SetAppCulled(1);
							} else {
								++ienabledShadows;
								FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData, nullptr, 0.0f, 0, 0);
								NiLight* v22 = psslight->GetLight();
								v22->Update(UpdateData);
								psslight->AccumulateGeometry(pculler, ienabledShadows - 1);
								psslight->InsertUnusedFence();
								ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
								BSPortalGraph* ppgraph = ShadowSceneNode_->GetPortalGraph();
								ShadowSceneLight* pExtraBoundTaga = psslight;
								ShadowSceneNode* v24 = BSShaderManager::GetShadowSceneNode(0);
								v24->UpdateQueuedLight(pExtraBoundTaga);
								if (Renderer::bActorSelfShadowing.Bool()) {
									NiAVObject* v25 = psslight->GetShadowScene();
									psslight->AddObject(v25);
								}
								psslight->RemoveUnusedGeom();
							}
						}
					}
					psslight = pssnode_2->GetNextShadowLight();
				}
				while (ienabledShadows + 17 < 21) {
					MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, ienabledShadows + 17);
					++ienabledShadows;
				}
				if (pgnode) {
					pgnode->SetAppCulled(bgrasscull);
				}
				BSFadeNode::SetFadeEnabled(1);
				pssnode_2->UpdateDynamicShadows(*pculler);
				p1stPerson3d->SetAppCulled(b1stcull);
			}
		} else {
			pssnode_2->RemoveAllShadowLights();
		}
	} else {
		for (int32_t i = 17; i < 21; ++i) {
			MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, i);
			MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, i);
		}
	}
	UpdateOffscreenInterface();
}

void Main::KillMenuBGTexture() {
	BSTextureManager* textureManager;
	ImageSpaceManager* imageSpaceManager;
	ImageSpaceManager* v3;
	BSRenderedTexture* v4;
	BSRenderedTexture* vCondition1;
	vCondition1 = (&Main::spMenuBGTexture)->operator->();
	if (vCondition1) {
		v4 = (&Main::spMenuBGTexture)->operator->();
		textureManager = BSShaderManager::TextureManagerPtr();
		textureManager->ReturnRenderedTexture(v4);
		(&Main::spMenuBGTexture)->operator=(nullptr);
		imageSpaceManager = BSShaderManager::GetImageSpaceManager();
		imageSpaceManager->SetMenuBGBlendValue(0.0);
		v3 = BSShaderManager::GetImageSpaceManager();
		v3->SetMenuBGBlendEnabled(0);
	}
	Main::bMenuBGReady = (bool)0;
}

// The exe builds the viewport rects with NiColorA's constructor (0x414430). It reads the rendered menu's screen
// parent through FORenderedMenu::GetParentNodeToRender called non-virtually (0x5585E0, folded with
// Animation::GetSceneRoot). AutoMemContext as in UpdateOffscreenInterface.
void Main::RenderMenuBackground() {
	alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
	AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
	::new (aucContext) AutoMemContext(MEM_CONTEXT_INTERFACE, true,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 7193);
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
	if (spMenuBGTexture.operator->()) {
		BSRenderedTexture* pOldTexture = spMenuBGTexture.operator->();
		BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(pOldTexture);
	}
	NiRenderer* pRenderer = NiRenderer::GetRenderer();
	BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
	spMenuBGTexture = pTextureManager->BorrowRenderedTexture(static_cast<NiDX9Renderer*>(pRenderer),
		BSTextureManager::TEX_MAIN_MENUBG, 0, nullptr, 0);
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
	} else if (BSShaderManager::bLetterbox) {
		WorldRootCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
	}
	TESForm* pImageSpaceMod = InterfaceManager::Instance()->GetCurrentImagespaceMod();
	ImageSpaceModifierInstanceForm::Stop(TESImageSpaceModifier::GetGetHit());
	if (pImageSpaceMod) {
		ImageSpaceModifierInstanceForm::Trigger(static_cast<TESImageSpaceModifier*>(pImageSpaceMod), 1.0f, nullptr);
		OnIdle_UpdateImageSpace(false);
	}
	bool bSkipFirstPerson = Interface::IsInPipboyMenu();
	if (Interface::IsMenuIDVisible(Interface::eStartMenu, 0))
		bSkipFirstPerson = false;
	DrawWorld(spMenuBGTexture.operator->(), false, bSkipFirstPerson);
	if (Interface::IsMenuIDVisible(Interface::eRaceSexMenu, 0) && StartMenu::PauseMenuIsActive()) {
		alignas(BSCullingProcess) uint8_t aCullerStorage[sizeof(BSCullingProcess)];
		BSCullingProcess* pCuller = ::new (aCullerStorage) BSCullingProcess(nullptr);
		NiCamera* pCamera = Interface::GetCurrentRenderedMenu()->GetCamera();
		pCuller->SetCamera(pCamera);
		pCuller->SetAccumulator(spRenderedMenuAccum.operator->());
		ShadowSceneNode* pShadowSceneNode =
			BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D);
		spRenderedMenuAccum->SetShadowSceneNode(pShadowSceneNode);
		FORenderedMenu* pRenderedMenu = Interface::GetCurrentRenderedMenu();
		NiAVObject* pScreenParent;
		pScreenParent = pRenderedMenu->FORenderedMenu::GetParentNodeToRender();
		BSShaderUtil::AccumulateScene(pCamera, pScreenParent, *pCuller);
		BSShaderUtil::RenderScene(pCamera, spRenderedMenuAccum.operator->(), false);
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

// The exe calls the out-of-line AutoMemContext constructor and destructor on a stack context and ignores the
// constructor's result.
void Main::UpdateOffscreenInterface() {
	if (Interface::IsInMenuMode() && (bUsePipboyMode.Bool() || bUseImageSpaceMenuFX.Bool()) &&
		Interface::IsInRenderedMenu()) {
		alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
		AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
		::new (aucContext) AutoMemContext(MEM_CONTEXT_INTERFACE, true,
			"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 6878);
		bool bInPipboy = Interface::IsInPipboyMenu();
		if (Interface::IsInMenuMode())
			Interface::LastMinuteUpdate();
		if (!spInterfaceFXTex.operator->()) {
			NiRenderer* pRenderer = NiRenderer::GetRenderer();
			BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
			spInterfaceFXTex = pTextureManager->BorrowRenderedTexture(static_cast<NiDX9Renderer*>(pRenderer),
				BSTextureManager::TEX_INTERFACE_RENDEREDMENU, 0, nullptr, 0);
		}
		if (!bInPipboy)
			Interface::GetPipboy()->SetScreenTexture(nullptr);
		bool bMenuOpen = Interface::IsCurrentRenderedMenuTopmost() || Menu::GetMenuByClass(Interface::eMessageMenu) ||
			Menu::GetMenuByClass(Interface::eQuantityMenu) || Menu::GetMenuByClass(Interface::eTutorialMenu);
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
			pMenu->Draw(spInterfaceFXTex.operator->(), NiRenderer::CLEAR_BACKBUFFER, nullptr);
			if (bHideMessage)
				pMessageModel->SetAppCulled(bWasCulled);
		}
		pContext->~AutoMemContext();
	} else if (Interface::IsInMenuMode()) {
		Interface::LastMinuteUpdate();
	}
}

// The exe tests the setting's whole byte (movzx; test) and then stores 1: read as bool, clang ors the raw byte into
// abValue.
void Main::SetActive(bool abValue) {
	if (static_cast<uint8_t>(bAlwaysActive.Bool()))
		abValue = true;
	if (Controls::GetInstance()) {
		Controls::GetInstance()->ClearKeystrokes();
		Controls::GetInstance()->UpdateValues();
		Controls::GetInstance()->ClearUserActions();
	}
	if (abValue)
		appTimer.Enable();
	else
		appTimer.Disable();
	bGameActive = abValue;
}

// The exe fetches the water system once before SetIsUnderWater without using it, which src keeps.
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
		} else {
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
		if (NiParallelUpdateTaskManager::IsActive())
			NiParallelUpdateTaskManager::EndUpdate();
		NiParallelUpdateTaskManager::WaitRenderSema(3);
	}
}

// The exe keeps only the low byte of GetMultiSampleEnabled's result.
void Main::DrawWorldMenu3D(BSRenderedTexture* apDestination) {
	NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
	ImageSpaceManager* pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
	bool bIsMSAA = static_cast<uint8_t>(Renderer::GetMultiSampleEnabled());
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
	} else if (apDestination) {
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
	spScreenSplatterAccum.operator->()->RenderScreenSplatter();
	ScreenSplatter::bResetCameraPos = bResetCameraPos;
}

// The exe builds the viewport rect with NiColorA's constructor (0x414430). AutoMemContext as in
// UpdateOffscreenInterface.
BSRenderedTexture* Main::DrawWorld_InitRenderTexture(NiDX9Renderer* apRenderer, BSRenderedTexture* apDestination,
	bool abWireframe, bool abMultiSampleEnabled) {
	BSRenderedTexture* pRenderTexture = nullptr;
	if (BSShaderManager::GetImageSpaceEffects() && !abWireframe) {
		alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
		AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
		::new (aucContext) AutoMemContext(MEM_CONTEXT_IMAGE_SPACE, true,
			"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 7789);
		pRenderTexture = BSShaderManager::TextureManagerPtr()->BorrowRenderedTexture(apRenderer,
			BSTextureManager::TEX_MAIN_IMAGESPACE, 0, nullptr, 0);
		pRenderTexture->UsePreRestoreGroup();
		BSShaderManager::pCurrentRenderTarget = pRenderTexture;
		// The exe calls NiRenderer's screen size getters (0x4DEE10, 0x4DEE70), not NiDX9Renderer's.
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
	} else {
		alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
		AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
		::new (aucContext) AutoMemContext(MEM_CONTEXT_SHADERS, true,
			"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 7812);
		if (apDestination) {
			if (abMultiSampleEnabled)
				BSRenderedTexture::StartOffscreen(7, apRenderer->GetDefaultRenderTargetGroup());
			else
				BSRenderedTexture::StartOffscreen(7, apDestination->GetGroup());
		} else {
			BSRenderedTexture::Start(7, nullptr);
		}
		pContext->~AutoMemContext();
	}
	return pRenderTexture;
}

// The exe passes each NiPointer::operator-> result (0x559450) before it fetches the VATS effect manager.
bool Main::DrawWorld_VATSOcclusionQuery() {
	bool bIssued = false;
	if (Interface::GetVATSEffect()->GetTarget()) {
		if (!spVATSTex.operator->()) {
			NiRenderer* pRenderer = NiRenderer::GetRenderer();
			BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
			spVATSTex = pTextureManager->BorrowRenderedTexture(static_cast<NiDX9Renderer*>(pRenderer),
				BSTextureManager::TEX_INTERFACE_VATS, 0, nullptr, 0);
		}
		if (!Interface::GetVATSEffect()->GetEffectMode() && spVATSTex.operator->() &&
			Interface::GetVATSEffect()->GetTarget() && Interface::GetVATSEffect()->OcclusionEnabled()) {
			BSRenderedTexture* pTexture = spVATSTex.operator->();
			Interface::GetVATSEffect()->DrawTargetToTexture(pTexture, FOVATSEffectManager::VATS_OCCLUSION_QUERY_NONE,
				true, FOVATSEffectManager::VATS_DRAW_SELECTED_PARTS, NiRenderer::CLEAR_ALL, false);
			pTexture = spVATSTex.operator->();
			Interface::GetVATSEffect()->DrawTargetToTexture(pTexture, FOVATSEffectManager::VATS_OCCLUSION_QUERY_FULL,
				true, FOVATSEffectManager::VATS_DRAW_SELECTED_PARTS, NiRenderer::CLEAR_BACKBUFFER, true);
			bIssued = true;
		}
	}
	return bIssued;
}

// abIssuedVATSQuery is not read. The exe passes each NiPointer::operator-> result (0x559450) before it fetches the
// VATS effect manager.
void Main::DrawWorld_VATS([[maybe_unused]] bool abIssuedVATSQuery) {
	if (Interface::GetVATSEffect()->GetTarget()) {
		BSRenderedTexture::StopOffscreen();
		BSRenderState::SetColorWriteEnable(0, BSRS_LOCK);
		BSRenderedTexture* pTexture = spVATSTex.operator->();
		Interface::GetVATSEffect()->DrawTargetToTexture(pTexture, FOVATSEffectManager::VATS_OCCLUSION_QUERY_NONE, false,
			FOVATSEffectManager::VATS_DRAW_ENTIRE_TARGET, NiRenderer::CLEAR_ALL, false);
		BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
		pTexture = spVATSTex.operator->();
		Interface::GetVATSEffect()->DrawTargetToTexture(pTexture, FOVATSEffectManager::VATS_OCCLUSION_QUERY_NONE, false,
			FOVATSEffectManager::VATS_DRAW_SELECTED_PARTS, NiRenderer::CLEAR_BACKBUFFER, true);
	}
}

// The exe fetches the image-space manager and the renderer here without using them; src keeps both calls.
// abMultiSampleEnabled is not read.
void Main::DrawWorld_RestartRenderTexture(BSRenderedTexture* apRenderTexture,
	[[maybe_unused]] bool abMultiSampleEnabled, uint32_t auiClearMode) {
	if (BSRenderedTexture::QOutsideFrame()) {
		BSShaderManager::GetImageSpaceManager();
		BSShaderManager::GetRenderer();
		BSRenderedTexture::StartOffscreen(auiClearMode, apRenderTexture->GetGroup());
	}
}

// The exe builds the viewport rect with NiColorA's constructor (0x414430). It passes each NiPointer::operator->
// result (0x559450) before it fetches the callee's object.
// abMultiSampleEnabled is not read. AutoMemContext as in UpdateOffscreenInterface.
void Main::DrawWorld_ImageSpace([[maybe_unused]] bool abMultiSampleEnabled, BSRenderedTexture* apRenderTexture,
	NiDX9Renderer* apRenderer, BSRenderedTexture* apDestination) {
	alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
	AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
	::new (aucContext) AutoMemContext(MEM_CONTEXT_IMAGE_SPACE, true,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 9058);
	if (!BSRenderedTexture::QOutsideFrame())
		BSRenderedTexture::StopOffscreen();
	if (!Interface::IsInPipboyMenu() && !Interface::IsInMenuMode() && !Interface::GetLoadingMenuVisible() &&
		!pSaveLoadGame->GetSaveGameLoading() && !pSaveLoadGame->GetSaveGameSaving() && !bRenderingMenuBG)
		Interface::Accumulate();
	if (spVATSTex.operator->()) {
		BSRenderedTexture* pMask = spVATSTex.operator->();
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
	if (spVATSTex.operator->()) {
		Interface::GetVATSEffect()->ShutdownEffect();
		BSRenderedTexture* pMask = spVATSTex.operator->();
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
	bQuitGame = (bool)1;
}

void Main::SetFreezeTime(bool abFreezeTime) {
	bFreezeTime = (bool)((uint8_t)abFreezeTime);
}

void Main::SetFlyCamera(bool abFlyCamera) {
	bFlyCamera = (bool)((uint8_t)abFlyCamera);
}

void Main::ResetGame() {
	bResetGame = (bool)1;
}

void Main::SetGame() {
	bResetGame = (bool)0;
	bReloadContent = (bool)0;
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
				if (acLine[uiLength - 1] == '\n')
					acLine[uiLength - 1] = '\0';
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
	// The window has the input focus (the exe: GetActiveWindow() is hWnd).
	if (SDL_GetKeyboardFocus() != pSDLMainWindow) {
		return 0;
	}
	if (!Interface::IsInMenuMode()) {
		return 0;
	}
	Controls* Instance = Controls::GetInstance();
	if (!Instance->IsMouseInBackground()) {
		return 0;
	}
	if (Interface::IsTopMenuID(Interface::eSurgeryMenu)) {
		Interface::SetSurgeryCursorPos(x, y);
	} else {
		float afY = static_cast<double>(y) / static_cast<double>(Renderer::GetScreenHeight());
		float afX = static_cast<double>(x) / static_cast<double>(Renderer::GetScreenWidth());
		Interface::SetCursorPos(afX, afY);
	}
	return 1;
}

// The exe's cleanup-only C++ EH frame is left out. The viewport is built with NiColorA's constructor (0x414430).
Sun* Main::DrawWorld_Init() {
	NiRect<float> kViewport;
	SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
	WorldSceneGraph->GetCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
	if (pVATS.eMode != VATS::VATS_PLAYBACK) {
		float fWorldFOV = pPlayer->GetWorldFOV();
		SceneGraph* v2 = Main::GetWorldSceneGraph();
		v2->SetCameraFOV(fWorldFOV, 0, 0, 1);
	}
	float fA = pPlayer->GetWorldFOV();
	BSShaderManager::SetFOV(fA);
	if (bRenderingMenuBG) {
		::new (&kViewport) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
		SceneGraph* v3 = Main::GetWorldSceneGraph();
		v3->GetCamera()->SetViewPort(kViewport);
	}
	NiCamera* pcamera = Main::spWorldRoot.m_pObject->GetCamera();
	NiCamera* pviscamera = 0;
	if (bPreCullActors.Bool()) {
		// The PDB's function static cullHA, guarded by bit 1 of cullHA_guard; the exe calls HighActorCuller's
		// constructor. The hybrid binds the name to the exe's object at its address, untyped memory there, as
		// FNVD_FUNCTION_STATIC does.
		static HighActorCuller cullHA;
		if (pviscamera) {
			cullHA.Run(pviscamera);
		} else {
			cullHA.Run(pcamera);
		}
	}
	NiNode* CameraRoot = Main::spWorldRoot.m_pObject->GetCameraRoot();
	Main::spSkyRoot.m_pObject->SetTranslate(CameraRoot->GetTranslate());
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, SkyUpdateData, nullptr, 0.0f, 0, 0);
	Main::spSkyRoot.m_pObject->Update(SkyUpdateData);
	NiNode* v5 = Main::spWorldRoot.m_pObject->GetCameraRoot();
	Main::spWeatherRoot.m_pObject->SetTranslate(v5->GetTranslate());
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, WeatherUpdateData, nullptr, 0.0f, 0, 0);
	Main::spWeatherRoot.m_pObject->Update(WeatherUpdateData);
	if (BSShaderAccumulator::bRequestRenderPassesScenegraph) {
		BSShaderAccumulator::bRequestRenderPassesScenegraph = 0;
		BSShaderAccumulator::bShowRenderPassesScenegraph = 1;
	}
	bool* v6 = &bEnableBoundingVolumeOcclusion.Bool();
	BSShaderAccumulator::SetRunBoundingSphereTests(*v6, 1);
	bool* v7 = &bDisplayBoundingVolumes.Bool();
	BSShaderAccumulator::SetDisplayBoundingSpheres(*v7);
	BSShaderManager::bDepthRestored = 0;
	Sun* psun = 0;
	if (TES::pInstance->pSky) {
		if (TES::pInstance->pSky->pSun) {
			if (TES::pInstance->pSky->eMode == Sky::SM_FULL || TES::pInstance->pSky->eMode == Sky::SM_FAKEEXTERIOR) {
				return TES::pInstance->pSky->pSun;
			}
		}
	}
	return psun;
}

// The exe's cleanup-only C++ EH frame is left out.
void Main::DrawWorld_DrawScene(Sun* psun, int32_t abDo1stPerson, bool abWireframe, bool abIssueVATSPartialQuery) {
	FNVD_INLINED_LOCAL(BSCullingProcess, kcullproc, std::nullopt, std::nullopt);
	NiTPointerList<NiAVObject*> lAccumOne;
	NiTPointerList<NiAVObject*> lAccumTwo;
	NiTPointerList<NiAVObject*> lAccumThree;
	SceneGraph* pscenegraph = Main::GetWorldSceneGraph();
	BSCullingProcess* pculler = pscenegraph->pCuller;
	ImageSpaceEffectRadialBlur::pWorldCamera = pscenegraph->GetCamera();
	if (psun) {
		psun->StartSunOcclusion();
	}
	NiAVObject* v5 = pPlayer->Get3D(1);
	bool b1stcull = v5->GetAppCulled();
	NiAVObject* v6 = pPlayer->Get3D(1);
	v6->SetAppCulled(1);
	spDrawWorldAccum.m_pObject->bWorldGeometry = !abWireframe;
	ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
	ImageSpaceEffectDepthOfField* pdofeffect = static_cast<ImageSpaceEffectDepthOfField*>(ImageSpaceManager_
			->GetEffect(ImageSpaceManager::EFFECT_DEPTH_OF_FIELD));
	bool bRenderDOF = !abWireframe && pdofeffect->DoDepth();
	spDrawWorldAccum.m_pObject->bDoDepthPasses = bRenderDOF;
	bool v61 = !abWireframe && pdofeffect->DoDepth();
	spDraw1stPersonAccum.m_pObject->bDoDepthPasses = v61;
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, tempMemContext, &AutoMemContext::Destruct, MEM_CONTEXT_GAMEBRIO, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 0x1EB7);
	NiCamera* paccumcamera = 0;
	paccumcamera = pscenegraph->GetCamera();
	pculler->m_pkCamera = paccumcamera;
	const NiFrustum* ViewFrustum = &paccumcamera->GetViewFrustum();
	pculler->SetFrustum(*ViewFrustum);
	BSShaderAccumulator* v49 = spDrawWorldAccum.m_pObject;
	pculler->SetAccumulator(v49);
	if (BSShaderManager::bMTRendering) {
		::new (&lAccumOne) NiTPointerList<BSAnimGroupSequence*>;
		::new (&lAccumTwo) NiTPointerList<BSAnimGroupSequence*>;
		::new (&lAccumThree) NiTPointerList<BSAnimGroupSequence*>;
		NiNode* pssn = (NiNode*)BSShaderManager::GetShadowSceneNode(0);
		NiAVObject* pavobj = 0;
		NiNode* pnode = 0;
		pavobj = pssn->GetAtChecked(0);
		if (pavobj) {
			if (!pavobj->GetAppCulled()) {
				lAccumThree.AddTail(pavobj);
			}
		}
		pavobj = pssn->GetAtChecked(1u);
		if (pavobj) {
			if (!pavobj->GetAppCulled()) {
				lAccumThree.AddTail(pavobj);
			}
		}
		pavobj = pssn->GetAtChecked(2u);
		if (pavobj) {
			if (!pavobj->GetAppCulled()) {
				pnode = pavobj->IsNode();
			}
		}
		if (pnode) {
			pavobj = pnode->GetAtChecked(0);
			if (pavobj) {
				if (!pavobj->GetAppCulled()) {
					lAccumOne.AddTail(pavobj);
				}
			}
			pavobj = pnode->GetAtChecked(1u);
			if (pavobj) {
				if (!pavobj->GetAppCulled()) {
					lAccumOne.AddTail(pavobj);
				}
			}
			pavobj = pnode->GetAtChecked(2u);
			if (pavobj) {
				if (!pavobj->GetAppCulled()) {
					lAccumOne.AddTail(pavobj);
				}
			}
		}
		pavobj = pssn->GetAtChecked(3u);
		pnode = pavobj->IsNode();
		if (TES::pInstance->pInteriorCell) {
			NiTListItem<NiPointer<BSMultiBoundRoom>>* HeadPos;
			NiTPointerList<NiPointer<BSMultiBoundRoom>>* MBRoomAccumList;
			ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
			if (ShadowSceneNode_->HasVisibleUnboundSpace()) {
				pavobj = pnode->GetAtChecked(0);
				if (pavobj) {
					if (!pavobj->GetAppCulled()) {
						lAccumTwo.AddTail(pavobj);
					}
				}
				pavobj = pnode->GetAtChecked(1u);
				if (pavobj) {
					if (!pavobj->GetAppCulled()) {
						lAccumOne.AddTail(pavobj);
					}
				}
				pavobj = pnode->GetAtChecked(2u);
				if (pavobj) {
					pnode = pavobj->IsNode();
					if (pnode) {
						if (!pnode->GetAppCulled()) {
							for (int32_t i_2 = 0;; ++i_2) {
								uint32_t ArrayCount = pnode->GetArrayCount();
								if (i_2 >= ArrayCount) {
									break;
								}
								pavobj = pnode->GetAt(i_2);
								if (pavobj) {
									if (i_2 != 4) {
										if (i_2) {
											NiNode* pchnode_2 = pavobj->IsNode();
											if (pchnode_2) {
												if (!pchnode_2->GetAppCulled()) {
													for (int32_t j_2 = i_2 == 6;; ++j_2) {
														uint32_t v14 = pchnode_2->GetArrayCount();
														if (j_2 >= v14) {
															break;
														}
														NiAVObject* pchobj_2 = pchnode_2->GetAt(j_2);
														if (pchobj_2) {
															if (!pchobj_2->GetAppCulled()) {
																if (j_2 % 2) {
																	lAccumOne.AddTail(pchobj_2);
																} else {
																	lAccumTwo.AddTail(pchobj_2);
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
					}
				}
			} else {
				pavobj = pnode->GetAtChecked(2u);
				if (pavobj) {
					pnode = pavobj->IsNode();
					if (pnode) {
						if (!pnode->GetAppCulled()) {
							pavobj = pnode->GetAtChecked(7u);
							if (pavobj) {
								pnode = pavobj->IsNode();
								if (pnode) {
									for (int32_t j_3 = 0;; ++j_3) {
										uint32_t v15 = pnode->GetArrayCount();
										if (j_3 >= v15) {
											break;
										}
										NiAVObject* pchobj_3 = pnode->GetAt(j_3);
										if (pchobj_3) {
											if (!pchobj_3->GetAppCulled()) {
												if (j_3 % 2) {
													lAccumOne.AddTail(pchobj_3);
												} else {
													lAccumTwo.AddTail(pchobj_3);
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
			ShadowSceneNode* v16 = BSShaderManager::GetShadowSceneNode(0);
			BSPortalGraph* pportalgraph = v16->GetPortalGraph();
			if (pportalgraph) {
				MBRoomAccumList = pportalgraph->GetMBRoomAccumList();
			} else {
				MBRoomAccumList = 0;
			}
			if (MBRoomAccumList) {
				HeadPos = MBRoomAccumList->m_pkHead;
			} else {
				HeadPos = 0;
			}
			NiTListItem<NiPointer<BSMultiBoundRoom>>* kRoomIter = HeadPos;
			int32_t ispcount = 0;
			while (kRoomIter) {
				BSMultiBoundRoom* pRoom = pportalgraph->GetMBRoomAccumList()->GetNext(kRoomIter).m_pObject;
				if (pRoom) {
					if (!pRoom->kPortalList.IsEmpty()) {
						NiTListItem<BSPortal*>* kPortalIter = pRoom->kPortalList.m_pkHead;
						while (kPortalIter) {
							BSPortal* pportal = pRoom->kPortalList.GetNext(kPortalIter);
							if (pportal) {
								BSPortalSharedNode* v20 = pportal->QSharedGeometryNode();
								if (pculler->AddShared(v20)) {
									BSPortalSharedNode* spnode = pportal->QSharedGeometryNode();
									if (spnode) {
										if (pportal->pMultiBoundRoom[0]) {
											if (pportal->pMultiBoundRoom[1]) {
												BSPortal* v50 = pportal;
												NiCamera* Camera = pculler->m_pkCamera;
												ShadowSceneNode* v21 = BSShaderManager::GetShadowSceneNode(0);
												v21->BuildSharedCompoundFrustum(Camera, v50);
											}
										}
										if (ispcount % 2) {
											NiAVObject* v67 = spnode;
											lAccumOne.AddTail(v67);
										} else {
											NiAVObject* v66 = spnode;
											lAccumTwo.AddTail(v66);
										}
										++ispcount;
									}
								}
							}
						}
					}
				}
			}
			ShadowSceneNode* v22 = BSShaderManager::GetShadowSceneNode(0);
			if (v22->HasVisibleUnboundSpace()) {
				NiTListItem<BSPortal*>* kiter = pportalgraph->QPortalList().m_pkHead;
				while (kiter) {
					BSPortal* pportal_2 = pportalgraph->QPortalList().GetNext(kiter);
					if (pportal_2) {
						BSPortalSharedNode* v23 = pportal_2->QSharedGeometryNode();
						if (pculler->AddShared(v23)) {
							BSPortalSharedNode* spnode_2 = pportal_2->QSharedGeometryNode();
							if (spnode_2) {
								if (ispcount % 2) {
									NiAVObject* v65 = spnode_2;
									lAccumOne.AddTail(v65);
								} else {
									NiAVObject* v64 = spnode_2;
									lAccumTwo.AddTail(v64);
								}
								++ispcount;
							}
						}
					}
				}
			}
			TESObjectCELL* CurrentCell = TES::pInstance->GetCurrentCell();
			BSPortalGraph* pbsPortalGraph = CurrentCell->GetPortalGraph();
			if (pbsPortalGraph) {
				for (uint32_t i_3 = 0;; ++i_3) {
					uint32_t AlwaysRenderArrayCount = pbsPortalGraph->GetAlwaysRenderArrayCount();
					if (i_3 >= AlwaysRenderArrayCount) {
						break;
					}
					NiAVObject* pkChild = pbsPortalGraph->GetAlwaysRenderAt(i_3);
					if (pkChild) {
						lAccumThree.AddTail(pkChild);
					}
				}
			}
		} else {
			if (pnode) {
				if (!pnode->GetAppCulled()) {
					for (int32_t i = 0;; ++i) {
						uint32_t v9 = pnode->GetArrayCount();
						if (i >= v9) {
							break;
						}
						pavobj = pnode->GetAt(i);
						if (pavobj) {
							NiNode* pcnode = pavobj->IsNode();
							if (pcnode) {
								if (!pcnode->GetAppCulled()) {
									for (int32_t j = 0;; ++j) {
										uint32_t v10 = pcnode->GetArrayCount();
										if (j >= v10) {
											break;
										}
										NiAVObject* pchobj = pcnode->GetAt(j);
										if (pchobj) {
											NiNode* pchnode = pchobj->IsNode();
											if (pchnode) {
												if (!pchnode->GetAppCulled()) {
													for (int32_t k = j == 6;; ++k) {
														uint32_t v11 = pchnode->GetArrayCount();
														if (k >= v11) {
															break;
														}
														NiAVObject* pkGrandChild = pchnode->GetAt(k);
														if (pkGrandChild) {
															if (!pkGrandChild->NiAVObject::GetAppCulled()) {
																if (i % 2) {
																	lAccumOne.AddTail(pkGrandChild);
																} else {
																	lAccumTwo.AddTail(pkGrandChild);
																}
															}
														}
													}
												}
											} else {
												if (i % 2) {
													lAccumOne.AddTail(pchobj);
												} else {
													lAccumTwo.AddTail(pchobj);
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
		pavobj = pssn->GetAtChecked(4u);
		if (pavobj) {
			if (!pavobj->GetAppCulled()) {
				lAccumThree.AddTail(pavobj);
			}
		}
		pavobj = pssn->GetAtChecked(5u);
		if (pavobj) {
			if (!pavobj->GetAppCulled()) {
				lAccumThree.AddTail(pavobj);
			}
		}
		pavobj = pssn->GetAtChecked(6u);
		if (pavobj) {
			if (!pavobj->GetAppCulled()) {
				lAccumOne.AddTail(pavobj);
			}
		}
		for (int32_t i_4 = 7;; ++i_4) {
			uint32_t v26 = pssn->GetArrayCount();
			if (i_4 >= v26) {
				break;
			}
			pavobj = pssn->GetAt(i_4);
			if (pavobj) {
				if (!pavobj->GetAppCulled()) {
					if (i_4 % 2) {
						lAccumOne.AddTail(pavobj);
					} else {
						lAccumTwo.AddTail(pavobj);
					}
				}
			}
		}
		pculler->m_pkCamera = paccumcamera;
		ShadowSceneNode* v27 = BSShaderManager::GetShadowSceneNode(0);
		v27->PreOnVisible(*pculler);
		ShadowSceneNode* v28 = BSShaderManager::GetShadowSceneNode(0);
		// The exe tests this address (&v28->kCompoundFrustum), which is never null.
		ShadowSceneNode* v29 = BSShaderManager::GetShadowSceneNode(0);
		BSCompoundFrustum* EffectShaderList = &v29->kCompoundFrustum;
		EffectShaderList->PrethreadOpList();
		BSShaderAccumulator* v58 = spDrawWorldAccum.m_pObject;
		v58->StartAccumulating(paccumcamera);
		BSShaderManager::SetAccumulator(spDrawWorldAccum.m_pObject);
		NiRenderer::GetRenderer()->SetSorter(spDrawWorldAccum.m_pObject);
		BSShaderAccumulator* v41 = spDrawWorldAccum.m_pObject;
		ShadowSceneNode* v31 = BSShaderManager::GetShadowSceneNode(0);
		BSCompoundFrustum* v32 = &v31->kCompoundFrustum;
		MTRenderingSystem::QInstance()->AddAccumTask(paccumcamera, v32, 0, &lAccumOne, 0, v41,
			BSCullingProcess::BSCP_CULL_NORMAL, 21, 1);
		MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 21);
		GetThreadTLSData()->iBatchRendererIndex = 0;
		::new (&kcullproc) BSCullingProcess(nullptr);
		kcullproc.m_pkCamera = paccumcamera;
		kcullproc.SetAccumulator(spDrawWorldAccum.m_pObject);
		ShadowSceneNode* v33 = BSShaderManager::GetShadowSceneNode(0);
		kcullproc.pCompoundFrustum = &v33->kCompoundFrustum;
		BSShaderUtil::AccumulateSceneList(paccumcamera, lAccumTwo, kcullproc);
		kcullproc.pCompoundFrustum = 0;
		BSShaderUtil::AccumulateSceneList(paccumcamera, lAccumThree, kcullproc);
		MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 21);
		lAccumOne.RemoveAll();
		lAccumTwo.RemoveAll();
		kcullproc.~BSCullingProcess();
	} else {
		BSShaderUtil::AccumulateScene(paccumcamera, pscenegraph, *pculler);
	}
	BSShaderAccumulator* v43 = spDrawWorldAccum.m_pObject;
	BSShaderUtil::RenderScenePreResolveDepth(pscenegraph->GetCamera(), v43, 1);
	if (abIssueVATSPartialQuery) {
		BSRenderState::SetColorWriteEnable(0, BSRS_LOCK);
		FOVATSEffectManager* VATSEffect = Interface::GetVATSEffect();
		VATSEffect->DrawTargetToTexture(0, FOVATSEffectManager::VATS_OCCLUSION_QUERY_PARTIAL, 1,
			FOVATSEffectManager::VATS_DRAW_SELECTED_PARTS, NiRenderer::CLEAR_NONE, 0);
		BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
	}
	BSShaderAccumulator* v44 = spDrawWorldAccum.m_pObject;
	BSShaderUtil::RenderScenePostResolveDepth(pscenegraph->GetCamera(), v44, 1);
	if (BSShaderManager::bMTRendering) {
		ShadowSceneNode* v35 = BSShaderManager::GetShadowSceneNode(0);
	}
	pculler->SetAccumulator(0);
	tempMemContext.Leave();
	spDrawWorldAccum.m_pObject->bWorldGeometry = 0;
	spDrawWorldAccum.m_pObject->bDoDepthPasses = 0;
	BSShaderBloodSplatterProperty::ResetShapeCount();
	NiDX9Renderer* apRenderer = BSShaderManager::GetRenderer();
	BSCullingProcess* apCuller = Main::GetWorldSceneGraph()->pCuller;
	apCuller->SetCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
	BSShaderBloodSplatter::ComputeScreenSpaceLightLoc(Main::spWorldRoot.m_pObject->GetCamera());
	apCuller->m_pkCamera = ScreenSplatter::GetCamera();
	BSShaderAccumulator* v53 = spScreenSplatterAccum.m_pObject;
	apCuller->SetAccumulator(v53);
	BSCullingProcess* v54 = apCuller;
	NiNode* Root = ScreenSplatter::GetRoot();
	NiCamera* v36 = ScreenSplatter::GetCamera();
	BSShaderUtil::AccumulateScene(v36, Root, *v54);
	BSShaderAccumulator* v46 = spScreenSplatterAccum.m_pObject;
	NiCamera* v37 = ScreenSplatter::GetCamera();
	BSShaderUtil::RenderScene(v37, v46, 0);
	pculler->SetAccumulator(0);
	apCuller->SetCullMode(BSCullingProcess::BSCP_CULL_NORMAL);
	BSShaderBloodSplatterProperty::ResetShapeCount();
	apRenderer = BSShaderManager::GetRenderer();
	apCuller = Main::GetWorldSceneGraph()->pCuller;
	apCuller->SetCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
	BSShaderBloodSplatter::ComputeScreenSpaceLightLoc(Main::spWorldRoot.m_pObject->GetCamera());
	apCuller->m_pkCamera = ScreenCustomSplatter::GetCamera();
	BSShaderAccumulator* v55 = spScreenSplatterAccum.m_pObject;
	apCuller->SetAccumulator(v55);
	NiNode* CurrentRoot = ScreenCustomSplatter::GetCurrentRoot();
	NiCamera* v38 = ScreenCustomSplatter::GetCamera();
	BSShaderUtil::AccumulateScene(v38, CurrentRoot, *apCuller);
	BSShaderAccumulator* v48 = spScreenSplatterAccum.m_pObject;
	NiCamera* v39 = ScreenCustomSplatter::GetCamera();
	BSShaderUtil::RenderScene(v39, v48, 0);
	pculler->SetAccumulator(0);
	apCuller->SetCullMode(BSCullingProcess::BSCP_CULL_NORMAL);
	if (iNumHWThreads.Int() > 1) {
		AILinearTaskThreadManager::SetMainRendering(0);
	}
	if (BSShaderManager::bFakeFullScreenMotionBlur) {
		ImageSpaceEffectFullScreenBlur::CheckCamera(pscenegraph->GetCamera());
	}
	if (psun) {
		psun->CalculateSunGlare(1u, spDrawWorldAccum.m_pObject, Main::spWorldRoot.m_pObject->GetCamera());
	}
	BSShaderAccumulator::SetRunBoundingSphereTests(0, 0);
	NiAVObject* v40 = pPlayer->Get3D(1);
	v40->SetAppCulled(b1stcull);
}

void Main::DrawWorld_Add1stPersonToDepthBuffer(BSRenderedTexture* apRenderTexture) {
	ImageSpaceManager* pimagespacemanager = BSShaderManager::GetImageSpaceManager();
	NiDX9Renderer* prenderer = BSShaderManager::GetRenderer();
	NiNode* p1stPersonNode = static_cast<NiNode*>(pPlayer->Get3D(1));
	SceneGraph* pscenegraph = Main::GetWorldSceneGraph();
	BSCullingProcess* pculler = pscenegraph->pCuller;
	bool bIronSights = pPlayer->GetIronSights();
	BSRenderedTexture* pDOFMask = 0;
	FNVD_CONSTRUCTED_LOCAL(NiColorA, savedBackground, nullptr, 0.0f, 0.0f, 0.0f, 0.0f);
	if (bIronSights) {
		ImageSpaceEffectDepthOfField* Effect = static_cast<ImageSpaceEffectDepthOfField*>(pimagespacemanager
				->GetEffect(ImageSpaceManager::EFFECT_DEPTH_OF_FIELD));
		BSRenderedTexture* p1stpersonBuffer = Effect->BorrowMaskTexture();
		BSRenderedTexture::StopOffscreen();
		prenderer->GetBackgroundColor(savedBackground);
		prenderer->SetBackgroundColor(NiColor_BLACK);
		NiRenderTargetGroup* Group = p1stpersonBuffer->GetGroup();
		BSRenderedTexture::StartOffscreen(1u, Group);
	} else {
		BSRenderState::SetColorWriteEnable(0, BSRS_LOCK);
	}
	bool bwas1stPersonCulled = p1stPersonNode->GetAppCulled();
	p1stPersonNode->SetAppCulled(0);
	float f1stPersonFOV = pPlayer->Get1stPersonFOV();
	pscenegraph->SetCameraFOV(f1stPersonFOV, 0, 0, 0);
	float fA = pPlayer->Get1stPersonFOV();
	BSShaderManager::SetFOV(fA);
	BSShaderAccumulator* fAa = spDraw1stPersonDepthAccum.m_pObject;
	pculler->SetAccumulator(fAa);
	BSShaderUtil::AccumulateScene(pscenegraph->GetCamera(), p1stPersonNode, *pculler);
	BSShaderAccumulator* fB = spDraw1stPersonDepthAccum.m_pObject;
	BSShaderUtil::RenderScene(pscenegraph->GetCamera(), fB, 0);
	pculler->SetAccumulator(0);
	float f1stPersonFOVa = pPlayer->GetWorldFOV();
	pscenegraph->SetCameraFOV(f1stPersonFOVa, 0, 0, 0);
	float fAb = pPlayer->GetWorldFOV();
	BSShaderManager::SetFOV(fAb);
	p1stPersonNode->SetAppCulled(bwas1stPersonCulled);
	if (bIronSights) {
		prenderer->SetBackgroundColorAlpha(savedBackground);
		BSRenderedTexture::StopOffscreen();
	} else {
		BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void Main::DrawWorld_Refraction(BSShaderAccumulator* apShaderAccum, BSRenderedTexture* apRenderTexture) {
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, tempMemContext, &AutoMemContext::Destruct, MEM_CONTEXT_IMAGE_SPACE, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 0x20D5);
	SceneGraph* pscene = Main::GetWorldSceneGraph();
	apShaderAccum->RenderNormals(pscene->GetCamera(), apRenderTexture);
}

// The exe's cleanup-only C++ EH frame is left out.
void Main::DrawWorld_Setup1stPerson() {
	NiAVObject* v4;
	NiAVObject* v5;
	NiAVObject* v6;
	FOPipboyManager* CurrentRenderedMenu;
	NiTransform v30;
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, context, &AutoMemContext::Destruct, MEM_CONTEXT_PLAYER, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 0x21B1);
	SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
	NiCamera* pRootCamera = Main::WorldRootCamera();
	NiNode* pPlayerNode = static_cast<NiNode*>(pPlayer->Get3D(1));
	pPlayerNode->SetAppCulled(0);
	if (BSShaderManager::bLetterbox) {
		// Renderer slots 51 and 50 (GetCurrentRenderTargetGroup, GetDefaultRenderTargetGroup): the exe fetches the
		// renderer twice, calls the default group's slot on the second fetch first, then the current one's.
		NiRenderer* pRenderer = NiRenderer::GetRenderer();
		bool bDefaultTarget = true;
		if (pRenderer->GetCurrentRenderTargetGroup()) {
			NiRenderer* pCurrentRenderer = NiRenderer::GetRenderer();
			NiRenderer* pDefaultRenderer = NiRenderer::GetRenderer();
			NiRenderTargetGroup* pDefaultGroup = pDefaultRenderer->GetDefaultRenderTargetGroup();
			bDefaultTarget = pCurrentRenderer->GetCurrentRenderTargetGroup() == pDefaultGroup;
		}
		if (bDefaultTarget) {
			NiCamera* v2 = Main::WorldRootCamera();
			v2->SetViewPort(BSShaderManager::kFramebufferViewport);
		}
	}
	float f1stPersonFOV = pPlayer->Get1stPersonFOV();
	pSceneGraph->SetCameraFOV(f1stPersonFOV, 0, 0, 0);
	NiAVObject* v3 = pPlayer->Get3D(0);
	if (((v3->GetAppCulled() || (v4 = pPlayer->Get3D(1), !v4->GetAppCulled())) &&
			((v5 = pPlayer->Get3D(0), -v5->GetAppCulled() != -1) || (v6 = pPlayer->Get3D(1), v6->GetAppCulled()))) ||
		appMain->bFlyCamera) {
		Main::bOffsetViewmodelLights = 0;
	} else {
		NiPointLight* v18;
		NiPointLight* light;
		NiPoint3 invertedPos = -pPlayerNode->GetWorldTranslate();
		FNVD_CONSTRUCTED_LOCAL(NiUpdateData, arData, nullptr, 0.0f, 0, 0);
		Main::bOffsetViewmodelLights = 1;
		float fMaxFarNearRatio = pRootCamera->m_fMaxFarNearRatio;
		sp1stPersonAdjustedCamera.m_pObject->SetMaxFarNearRatio(fMaxFarNearRatio);
		const NiFrustum* ViewFrustum = &pRootCamera->GetViewFrustum();
		sp1stPersonAdjustedCamera.m_pObject->SetViewFrustum(*ViewFrustum);
		sp1stPersonAdjustedCamera.m_pObject->SetViewPort(pRootCamera->GetViewPort());
		v30 = pRootCamera->GetWorldTransform();
		sp1stPersonAdjustedCamera.m_pObject->SetWorldTranslate(p1stPersonCamera->GetWorldTranslate() + invertedPos);
		sp1stPersonAdjustedCamera.m_pObject->SetWorldRotate(v30.m_Rotate);
		if (pPlayer->GetWeaponSightingNode()) {
			if (!pPlayer->IsReloading() && !pVATS.IsInVats()) {
				NiAVObject* WeaponSightingNode = pPlayer->GetWeaponSightingNode();
				sp1stPersonAdjustedCamera.m_pObject->SetWorldTranslate(WeaponSightingNode->GetWorldTranslate() +
					invertedPos);
			}
		}
		NiPoint3 kOrgPos = pPlayerNode->GetTranslate();
		pPlayerNode->SetTranslate(NiPoint3_ZERO);
		pPlayerNode->Update(arData);
		pPlayerNode->SetTranslate(kOrgPos);
		if (pPlayer->IsPipboyActive()) {
			light = Interface::GetPipboy()->GetMenuLight();
			v18 = light;
		} else {
			v18 = 0;
		}
		light = v18;
		pPlayerNode->SetAlwaysDraw(1);
		if (light) {
			light->SetWorldTranslate(light->GetWorldTranslate() - invertedPos);
		}
		NiPoint3 worldTranslate = sp1stPersonAdjustedCamera.m_pObject->GetWorldTranslate();
		ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
		ShadowSceneNode_->SetEyePosition(worldTranslate);
		NiPoint3 worldTranslatea = invertedPos;
		ShadowSceneNode* v13 = BSShaderManager::GetShadowSceneNode(0);
		v13->SetLightingOffset(worldTranslatea);
	}
	if (!Interface::GetCurrentRenderedMenu() ||
		(CurrentRenderedMenu = static_cast<FOPipboyManager*>(Interface::GetCurrentRenderedMenu()),
			CurrentRenderedMenu == Interface::GetPipboy())) {
		BSShaderAccumulator* f1stPersonFOVa = spDraw1stPersonAccum.m_pObject;
		MTRenderingSystem::QInstance()->AddAccumTask(sp1stPersonAdjustedCamera.m_pObject, 0, pPlayerNode, 0, 0,
			f1stPersonFOVa, BSCullingProcess::BSCP_CULL_NORMAL, 22, 0);
	}
	MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 22);
}

// The exe's cleanup-only C++ EH frame is left out.
void Main::DrawWorld_MTRender1stPerson(NiDX9Renderer* apRenderer, NiGeometry* apRenderedMenuGeom, Sun* apSun,
	BSRenderedTexture* apRenderTexture) {
	FOPipboyManager* v20;
	FNVD_INLINED_LOCAL(NiPointer<BSShaderAccumulator>, sptempaccum_4, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiPointer<BSShaderAccumulator>, sptempaccum_2, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiPointer<BSShaderAccumulator>, sptempaccum, std::nullopt, std::nullopt);
	SceneGraph* apSceneGraph = Main::GetWorldSceneGraph();
	BSCullingProcess* apCuller = apSceneGraph->pCuller;
	NiAVObject* ap1stPerson3d = pPlayer->Get3D(1);
	ap1stPerson3d->SetAppCulled(0);
	NiCamera* pcamera = apSceneGraph->GetCamera();
	NiNode* p1stPersonNode = static_cast<NiNode*>(ap1stPerson3d);
	MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 22);
	spDraw1stPersonAccum.m_pObject->bDoDepthPasses = 0;
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, tempMemContext, &AutoMemContext::Destruct, MEM_CONTEXT_PLAYER, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 0x221F);
	apRenderer->ClearBuffer(0, 4u);
	// Letterboxed while rendering to the default target group (or none): the framebuffer's viewport.
	NiRenderer* pCurrentRenderer;
	NiRenderer* pDefaultRenderer;
	NiRenderTargetGroup* pDefaultGroup;
	if (BSShaderManager::bLetterbox &&
		(!NiRenderer::GetRenderer()->GetCurrentRenderTargetGroup() ||
			(pCurrentRenderer = NiRenderer::GetRenderer(), pDefaultRenderer = NiRenderer::GetRenderer(),
				pDefaultGroup = pDefaultRenderer->GetDefaultRenderTargetGroup(),
				pCurrentRenderer->GetCurrentRenderTargetGroup() == pDefaultGroup))) {
		sp1stPersonAdjustedCamera.m_pObject->SetViewPort(BSShaderManager::kFramebufferViewport);
	} else {
		// The exe builds the viewport with NiColorA's constructor (0x414430): left, right, top, bottom.
		NiRect<float> kViewport;
		::new (&kViewport) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
		sp1stPersonAdjustedCamera.m_pObject->SetViewPort(kViewport);
	}
	float _1stPersonFOV = pPlayer->Get1stPersonFOV();
	apSceneGraph->SetCameraFOV(_1stPersonFOV, 0, 0, 0);
	float pNewViewport = pPlayer->Get1stPersonFOV();
	BSShaderManager::SetFOV(pNewViewport);
	const NiFrustum* ViewFrustum = &pcamera->GetViewFrustum();
	sp1stPersonAdjustedCamera.m_pObject->SetViewFrustum(*ViewFrustum);
	if (apRenderedMenuGeom) {
		FORenderedMenu* CurrentRenderedMenu = Interface::GetCurrentRenderedMenu();
		CurrentRenderedMenu->SetScreenTexture(0);
		apRenderedMenuGeom->SetAppCulled(0);
		if (Interface::IsInRenderedMenu()) {
			void* pNewAccumulator;
			if (Interface::IsInPipboyMenu() && pPlayer->IsRefractive()) {
				FORenderedMenu* v9 = Interface::GetCurrentRenderedMenu();
				v9->GetScreenGeometry()->SetAppCulled(1);
			}
			BSShaderAccumulator* v58 =
				static_cast<BSShaderAccumulator*>(NiMemObject::operator new(sizeof(BSShaderAccumulator)));
			if (v58) {
				pNewAccumulator = (void*)::new (v58) BSShaderAccumulator(99, 1u, 759u);
			} else {
				pNewAccumulator = 0;
			}
			NiRefObject* v59 = static_cast<NiRefObject*>(pNewAccumulator);
			::new (&sptempaccum) NiPointer<BSShaderAccumulator>(static_cast<BSShaderAccumulator*>(pNewAccumulator));
			apCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
			int32_t pNewViewporta = NiAlphaProperty::GetType();
			FORenderedMenu* v10 = Interface::GetCurrentRenderedMenu();
			NiAlphaProperty* prenderedMenuAlpha =
				static_cast<NiAlphaProperty*>(v10->GetScreenGeometry()->GetProperty(pNewViewporta));
			if (prenderedMenuAlpha) {
				prenderedMenuAlpha->SetAlphaBlending(0);
			}
			FORenderedMenu* v11 = Interface::GetCurrentRenderedMenu();
			v11->SetScreenTexture(0);
			BSShaderAccumulator* pNewViewportb = sptempaccum.m_pObject;
			apCuller->SetAccumulator(pNewViewportb);
			if (Interface::IsInPipboyMenu() && pPlayer->IsRefractive()) {
				FOPipboyManager* Pipboy = Interface::GetPipboy();
				NiAVObject* ButtonGlow = Pipboy->GetButtonGlow(0);
				BSShaderUtil::AccumulateScene(sp1stPersonAdjustedCamera.m_pObject, ButtonGlow, *apCuller);
				FOPipboyManager* v14 = Interface::GetPipboy();
				NiAVObject* v15 = v14->GetButtonGlow(1u);
				BSShaderUtil::AccumulateScene(sp1stPersonAdjustedCamera.m_pObject, v15, *apCuller);
				FOPipboyManager* v16 = Interface::GetPipboy();
				NiAVObject* v17 = v16->GetButtonGlow(2u);
				BSShaderUtil::AccumulateScene(sp1stPersonAdjustedCamera.m_pObject, v17, *apCuller);
			}
			FORenderedMenu* v18 = Interface::GetCurrentRenderedMenu();
			BSShaderUtil::AccumulateScene(sp1stPersonAdjustedCamera.m_pObject, v18->GetScreenGeometry(), *apCuller);
			BSShaderUtil::RenderScene(sp1stPersonAdjustedCamera.m_pObject, sptempaccum.m_pObject, 0);
			apCuller->SetAccumulator(0);
			sptempaccum.operator=(static_cast<BSShaderAccumulator*>(0));
			apCuller->PopCullMode();
			if (Interface::IsInPipboyMenu() && pPlayer->IsRefractive()) {
				FORenderedMenu* v19 = Interface::GetCurrentRenderedMenu();
				v19->GetScreenGeometry()->SetAppCulled(0);
			}
			sptempaccum.~NiPointer<BSShaderAccumulator>();
		}
	}
	if (!Interface::GetCurrentRenderedMenu() ||
		(v20 = static_cast<FOPipboyManager*>(Interface::GetCurrentRenderedMenu()), v20 == Interface::GetPipboy())) {
		if (!Interface::IsInPipboyMenu() || !pPlayer->IsRefractive()) {
			BSRenderState::SetColorWriteEnable(15u, BSRS_LOCK);
			MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 22);
			BSShaderUtil::RenderScene(sp1stPersonAdjustedCamera.m_pObject, spDraw1stPersonAccum.m_pObject, 0);
			BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
			BSSimpleList<NiPointer<BSTempEffect>>* pTempParticleList = &ReferenceProcessLists.GlobalTempEffectList;
			if (pTempParticleList) {
				if (!pTempParticleList->IsEmpty()) {
					NiPoint3 ISNodeDiff(0.0f, 0.0f, 0.0f);
					if (pPlayer->GetWeaponSightingNode()) {
						if (!pPlayer->IsReloading() && !pVATS.IsInVats()) {
							const NiPoint3* pNewViewportg = &p1stPersonCamera->GetWorldTranslate();
							NiAVObject* WeaponSightingNode = pPlayer->GetWeaponSightingNode();
							ISNodeDiff = WeaponSightingNode->GetWorldTranslate() - *pNewViewportg;
						}
					}
					SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
					NiCamera* pcamera_2 = WorldSceneGraph->GetCamera();
					BSCullingProcess* apCullingProcess = WorldSceneGraph->pCuller;
					sptempaccum_2.SetPointer(&spDrawWorldAccum);
					NiPoint3 cameraPos = pcamera_2->GetWorldTranslate();
					pcamera_2->SetWorldTranslate(cameraPos + ISNodeDiff);
					BSShaderAccumulator* pNewViewporth = sptempaccum_2.m_pObject;
					apCullingProcess->SetAccumulator(pNewViewporth);
					sptempaccum_2.m_pObject->b1stPerson = 1;
					bool bShouldRender = 0;
					// The exe walks the list with the same local, taking each node's item before moving on.
					while (pTempParticleList) {
						BSTempEffect* pEffect = pTempParticleList->m_item.m_pObject;
						pTempParticleList = pTempParticleList->m_pkNext;
						if (pEffect) {
							if (pEffect->IsFirstPerson()) {
								bShouldRender = 1;
								NiAVObject* v23 = pEffect->Get3D();
								v23->SetAppCulled(0);
								NiAVObject* v24 = pEffect->Get3D();
								BSShaderUtil::AccumulateScene(pcamera_2, v24, *apCullingProcess);
								NiAVObject* v25 = pEffect->Get3D();
								v25->SetAppCulled(1);
							}
						}
					}
					if (bShouldRender) {
						BSShaderAccumulator* pGlowNode0 = sptempaccum_2.m_pObject;
						BSShaderUtil::RenderScene(pcamera_2, pGlowNode0, 0);
					}
					apCullingProcess->SetAccumulator(0);
					sptempaccum_2.m_pObject->b1stPerson = 0;
					sptempaccum_2.operator=(static_cast<BSShaderAccumulator*>(0));
					pcamera_2->SetWorldTranslate(cameraPos);
					sptempaccum_2.~NiPointer<BSShaderAccumulator>();
				}
			}
		}
	}
	if (BSShaderManager::IsRefractionEnabled()) {
		FNVD_CONSTRUCTED_LOCAL(AutoMemContext, tempMemContext_2, &AutoMemContext::Destruct, MEM_CONTEXT_IMAGE_SPACE, 1,
			"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 0x22B5);
		if (Interface::IsInPipboyMenu() && BSShaderManager::IsRefractionEnabled() && pPlayer->IsRefractive()) {
			BSRenderedTexture::StopOffscreen();
			NiRenderTargetGroup* Group = apRenderTexture->GetGroup();
			BSRenderedTexture::StartOffscreen(6u, Group);
			if (BSShaderManager::bLetterbox) {
				NiCamera* v27 = Main::WorldRootCamera();
				v27->SetViewPort(BSShaderManager::kFramebufferViewport);
			}
		}
		SceneGraph* pscene = Main::GetWorldSceneGraph();
		spDraw1stPersonAccum.m_pObject->RenderNormals(sp1stPersonAdjustedCamera.m_pObject, apRenderTexture);
		DrawWorld_RestartRenderTexture(apRenderTexture, 0, 0);
	}
	if (Main::bOffsetViewmodelLights) {
		FNVD_CONSTRUCTED_LOCAL(NiUpdateData, kupdate, nullptr, 0.0f, 0, 0);
		p1stPersonNode->Update(kupdate);
		p1stPersonNode->SetAlwaysDraw(0);
		NiPoint3 a2 = pcamera->m_pkParent->GetTranslate();
		ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
		ShadowSceneNode_->SetEyePosition(a2);
		NiPoint3 a2a = NiPoint3_ZERO;
		ShadowSceneNode* v29 = BSShaderManager::GetShadowSceneNode(0);
		v29->SetLightingOffset(a2a);
	}
	if (pVATS.eMode != VATS::VATS_PLAYBACK) {
		float _1stPersonFOVa = pPlayer->GetWorldFOV();
		SceneGraph* v30 = Main::GetWorldSceneGraph();
		v30->SetCameraFOV(_1stPersonFOVa, 0, 0, 0);
	}
	float pNewViewportj = pPlayer->GetWorldFOV();
	BSShaderManager::SetFOV(pNewViewportj);
	if (apSun) {
		if (!apSun->GetSunGlareNode()->GetAppCulled()) {
			bool bcull = apSun->GetSunGlareNode()->GetAppCulled();
			apSun->GetSunGlareNode()->SetAppCulled(0);
			SceneGraph* apSceneGraph_2 = Main::GetWorldSceneGraph();
			NiCamera* pcamera_3 = apSceneGraph_2->GetCamera();
			BSCullingProcess* apCuller_2 = apSceneGraph_2->pCuller;
			sptempaccum_4.SetPointer(&spDrawWorldAccum);
			BSShaderAccumulator* pNewViewportk = sptempaccum_4.m_pObject;
			apCuller_2->SetAccumulator(pNewViewportk);
			sptempaccum_4.m_pObject->b1stPerson = 1;
			NiBillboardNode* pGlowNode0a = apSun->GetSunGlareNode();
			BSShaderUtil::AccumulateScene(pcamera_3, pGlowNode0a, *apCuller_2);
			BSShaderAccumulator* pGlowNode0b = sptempaccum_4.m_pObject;
			BSShaderUtil::RenderScene(pcamera_3, pGlowNode0b, 0);
			apCuller_2->SetAccumulator(0);
			sptempaccum_4.m_pObject->b1stPerson = 0;
			sptempaccum_4.operator=(static_cast<BSShaderAccumulator*>(0));
			apSun->GetSunGlareNode()->SetAppCulled(bcull);
			sptempaccum_4.~NiPointer<BSShaderAccumulator>();
		}
	}
	if (apSun) {
		apSun->CalculateSunGlare(2u, spDrawWorldAccum.m_pObject, Main::spWorldRoot.m_pObject->GetCamera());
	}
	float _1stPersonFOVb = pPlayer->Get1stPersonFOV();
	apSceneGraph->SetCameraFOV(_1stPersonFOVb, 0, 0, 0);
	float pNewViewportm = pPlayer->Get1stPersonFOV();
	BSShaderManager::SetFOV(pNewViewportm);
}

void Main::DrawWorld_RenderedMenuScene() {
	if (!Main::spVATSTex.m_pObject) {
		if (Interface::GetCurrentRenderedMenu()) {
			FOPipboyManager* CurrentRenderedMenu = static_cast<FOPipboyManager*>(Interface::GetCurrentRenderedMenu());
			if (CurrentRenderedMenu != Interface::GetPipboy()) {
				NiCamera* v18;
				SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
				BSCullingProcess* cullingProcess = WorldSceneGraph->pCuller;
				cullingProcess->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
				FORenderedMenu* v2 = Interface::GetCurrentRenderedMenu();
				if (v2->GetCamera()) {
					FORenderedMenu* v3 = Interface::GetCurrentRenderedMenu();
					v18 = v3->GetCamera();
				} else {
					v18 = WorldSceneGraph->GetCamera();
				}
				cullingProcess->m_pkCamera = v18;
				cullingProcess->SetAccumulator(spRenderedMenuAccum.m_pObject);
				ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
				spRenderedMenuAccum.m_pObject->SetShadowSceneNode(ShadowSceneNode_);
				// Letterboxed while rendering to the default target group (or none): the framebuffer's viewport.
				NiRenderer* pCurrentRenderer;
				NiRenderer* pDefaultRenderer;
				NiRenderTargetGroup* pDefaultGroup;
				if (BSShaderManager::bLetterbox &&
					(!NiRenderer::GetRenderer()->GetCurrentRenderTargetGroup() ||
						(pCurrentRenderer = NiRenderer::GetRenderer(), pDefaultRenderer = NiRenderer::GetRenderer(),
							pDefaultGroup = pDefaultRenderer->GetDefaultRenderTargetGroup(),
							pCurrentRenderer->GetCurrentRenderTargetGroup() == pDefaultGroup))) {
					v18->SetViewPort(BSShaderManager::kFramebufferViewport);
				} else {
					// The exe builds the viewport with NiColorA's constructor (0x414430): left, right, top, bottom.
					NiRect<float> kViewport;
					::new (&kViewport) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
					v18->SetViewPort(kViewport);
				}
				FORenderedMenu* v14 = Interface::GetCurrentRenderedMenu();
				NiAVObject* v7 = v14->GetParentNodeToRender();
				BSShaderUtil::AccumulateScene(v18, v7, *cullingProcess);
				BSShaderManager::SetRenderMode(15u);
				BSRenderState::SetZWriteEnable(0, BSRS_LOCK);
				BSRenderState::SetColorWriteEnable(8u, BSRS_LOCK);
				BSShaderAccumulator* fB = spRenderedMenuAccum.m_pObject;
				BSShaderUtil::RenderScene(v18, fB, 0);
				BSShaderManager::SetRenderMode(0);
				BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
				BSRenderState::RestoreZWriteEnable(BSRS_LOCK);
				FORenderedMenu* v13 = Interface::GetCurrentRenderedMenu();
				NiAVObject* v8 = v13->GetParentNodeToRender();
				BSShaderUtil::AccumulateScene(v18, v8, *cullingProcess);
				BSShaderAccumulator* fBa = spRenderedMenuAccum.m_pObject;
				BSShaderUtil::RenderScene(v18, fBa, 0);
				cullingProcess->SetAccumulator(0);
				cullingProcess->PopCullMode();
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void Main::DrawWorld_DoDepth(BSShaderAccumulator* apShaderAccum, BSRenderedTexture* apRenderTexture) {
	FNVD_INLINED_LOCAL(AutoMemContext, tempMemContext, std::nullopt, std::nullopt);
	SceneGraph* apSceneGraph = Main::GetWorldSceneGraph();
	if (pVATS.eMode != VATS::VATS_PLAYBACK) {
		float fWorldFOV = pPlayer->GetWorldFOV();
		apSceneGraph->SetCameraFOV(fWorldFOV, 0, 0, 0);
	}
	float afFOV = pPlayer->GetWorldFOV();
	BSShaderManager::SetFOV(afFOV);
	FNVD_CONSTRUCTED_LOCAL(NiCloningProcess, kClone, &NiCloningProcess::Destruct, 257u);
	NiCamera* Camera = apSceneGraph->GetCamera();
	NiCamera* p1stpersoncam = static_cast<NiCamera*>(Camera->CreateClone(kClone));
	if (pVATS.eMode != VATS::VATS_PLAYBACK) {
		float fWorldFOVa = pPlayer->Get1stPersonFOV();
		apSceneGraph->SetCameraFOV(fWorldFOVa, 0, p1stpersoncam, 0);
	}
	::new (&tempMemContext) AutoMemContext(MEM_CONTEXT_IMAGE_SPACE, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 0x234C);
	apShaderAccum->SetupDepth(apSceneGraph->GetCamera(), p1stpersoncam, apRenderTexture);
	if (p1stpersoncam) {
		delete p1stpersoncam;
	}
	tempMemContext.~AutoMemContext();
}

int32_t Main::DrawWorld_RenderedMenu(void* apPipboyGeom, NiDX9Renderer* apRenderer) {
	NiCamera* pcam_3;
	NiCamera* pcam_2;
	float v42;
	float a5;
	uint32_t a4;
	NiTransform p1stnodetransform;
	SceneGraph* apSceneGraph = Main::GetWorldSceneGraph();
	BSCullingProcess* apCuller = apSceneGraph->pCuller;
	FORenderedMenu* CurrentRenderedMenu = Interface::GetCurrentRenderedMenu();
	NiCamera* pcam = CurrentRenderedMenu->GetCamera();
	apRenderer->ClearBuffer(0, 4u);
	NiNode* p1stPersonNode =
		static_cast<NiNode*>(Interface::GetCurrentRenderedMenu()->FORenderedMenu::GetParentNodeToRender());
	apCuller->SetCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, kupdate, nullptr, 0.0f, 0, 0);
	FNVD_CONSTRUCTED_LOCAL(NiTransform, kcamtransform, nullptr);
	// The exe keeps GetAlwaysDraw's low byte (al) and passes it back to SetAlwaysDraw zero-extended.
	uint8_t balwaysdraw = static_cast<uint8_t>(p1stPersonNode->GetAlwaysDraw());
	if (pcam) {
		pcam_2 = pcam;
	} else {
		pcam_2 = apSceneGraph->GetCamera();
	}
	if (pPlayer->IsPipboyActive()) {
		p1stnodetransform = *&p1stPersonNode->m_kLocal;
		p1stPersonNode->SetTranslate(NiPoint3_ZERO);
		p1stPersonNode->Update(kupdate);
		p1stPersonNode->SetTranslate(p1stnodetransform.m_Translate);
		kcamtransform.MakeIdentity();
		if (pcam_2) {
			if (pcam_2->m_pkParent) {
				kcamtransform = *&pcam_2->m_pkParent->m_kLocal;
				const NiPoint3* camera1st_Translate_8 = &p1stPersonCamera->GetWorldTranslate();
				pcam_2->m_pkParent->SetTranslate(*camera1st_Translate_8);
				pcam_2->m_pkParent->Update(kupdate);
			}
		}
		NiPoint3 camera1st_Translate = -p1stPersonNode->GetWorldTranslate();
		ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
		ShadowSceneNode_->SetLightingOffset(camera1st_Translate);
	}
	if (Interface::IsInPipboyMenu()) {
		a4 = 0;
		Controls* Controls_ = GetControls();
		char v23 = Controls_->GetMouseButton(0, 2) > 0;
		Controls* v6 = GetControls();
		char v22 = v6->GetMouseButton(0, 1) > 0;
		FOPipboyManager* Pipboy = Interface::GetPipboy();
		uint8_t v45 = Pipboy->CheckOrangeButtonsClicked(v22, v23, &v42, &a5, &a4);
		Interface::SetQueuedPipboyTab(v45, v42, a5, a4);
	}
	if (pcam) {
		pcam_3 = pcam;
	} else {
		pcam_3 = apSceneGraph->GetCamera();
	}
	pcam = pcam_3;
	if (!pcam_3) {
		pcam = sp1stPersonAdjustedCamera.m_pObject;
	}
	NiRect<float> v55 = pcam->GetViewPort();
	if (BSShaderManager::bLetterbox) {
		NiRenderer* pCurrentRenderer = NiRenderer::GetRenderer();
		NiRenderer* pDefaultRenderer = NiRenderer::GetRenderer();
		NiRenderTargetGroup* pDefaultGroup = pDefaultRenderer->GetDefaultRenderTargetGroup();
		if (pCurrentRenderer->GetCurrentRenderTargetGroup() == pDefaultGroup) {
			pcam->SetViewPort(BSShaderManager::kFramebufferViewport);
		}
	}
	BSRenderState::SetColorWriteEnable(0, BSRS_LOCK);
	apCuller->m_pkCamera = pcam;
	BSShaderAccumulator* camera1st_Translate_8a = spDraw1stPersonAccum.m_pObject;
	apCuller->SetAccumulator(camera1st_Translate_8a);
	ShadowSceneNode* v9 = BSShaderManager::GetShadowSceneNode(0);
	spDraw1stPersonAccum.m_pObject->SetShadowSceneNode(v9);
	BSShaderUtil::AccumulateScene(pcam, p1stPersonNode, *apCuller);
	BSShaderAccumulator* camera1st_Translate_4 = spDraw1stPersonAccum.m_pObject;
	BSShaderUtil::RenderScene(pcam, camera1st_Translate_4, 0);
	spDraw1stPersonAccum.m_pObject->ClearGroupPasses(6);
	apCuller->SetAccumulator(0);
	BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
	int32_t camera1st_Translate_8b = NiAlphaProperty::GetType();
	FORenderedMenu* v10 = Interface::GetCurrentRenderedMenu();
	NiAlphaProperty* ppipboyAlpha =
		static_cast<NiAlphaProperty*>(v10->GetScreenGeometry()->GetProperty(camera1st_Translate_8b));
	if (ppipboyAlpha) {
		ppipboyAlpha->SetAlphaBlending(1);
		ppipboyAlpha->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);
		ppipboyAlpha->SetDestBlendMode(NiAlphaProperty::ALPHA_ONE);
		FORenderedMenu* v11 = Interface::GetCurrentRenderedMenu();
		v11->GetScreenGeometry()->UpdateProperties();
	}
	NiRenderedTexture* camera1st_Translate_8c = Main::spInterfaceFXTex.m_pObject->GetTexture(0);
	FORenderedMenu* v12 = Interface::GetCurrentRenderedMenu();
	v12->SetScreenTexture(camera1st_Translate_8c);
	apCuller->m_pkCamera = pcam;
	BSShaderAccumulator* camera1st_Translate_8d = spRenderedMenuAccum.m_pObject;
	apCuller->SetAccumulator(camera1st_Translate_8d);
	BSRenderState::SetZWriteEnable(0, BSRS_LOCK);
	if (pPlayer->IsRefractive() && Interface::IsInPipboyMenu()) {
		FOPipboyManager* v13 = Interface::GetPipboy();
		NiAVObject* ButtonGlow = v13->GetButtonGlow(0);
		BSShaderUtil::AccumulateScene(pcam, ButtonGlow, *apCuller);
		FOPipboyManager* v15 = Interface::GetPipboy();
		NiAVObject* v16 = v15->GetButtonGlow(1u);
		BSShaderUtil::AccumulateScene(pcam, v16, *apCuller);
		FOPipboyManager* v17 = Interface::GetPipboy();
		NiAVObject* v18 = v17->GetButtonGlow(2u);
		BSShaderUtil::AccumulateScene(pcam, v18, *apCuller);
		BSRenderState::SetZEnable(0, BSRS_LOCK);
	}
	NiGeometry* camera1st_Translate_4a = Interface::GetCurrentRenderedMenu()->GetScreenGeometry();
	BSShaderUtil::AccumulateScene(pcam, camera1st_Translate_4a, *apCuller);
	BSShaderAccumulator* camera1st_Translate_4b = spRenderedMenuAccum.m_pObject;
	BSShaderUtil::RenderScene(pcam, camera1st_Translate_4b, 0);
	BSRenderState::RestoreZWriteEnable(BSRS_LOCK);
	if (pPlayer->IsRefractive() && Interface::IsInPipboyMenu()) {
		BSRenderState::RestoreZEnable(BSRS_LOCK);
	}
	apCuller->SetAccumulator(0);
	FORenderedMenu* v19 = Interface::GetCurrentRenderedMenu();
	v19->SetScreenTexture(0);
	if (ppipboyAlpha) {
		ppipboyAlpha->SetAlphaBlending(0);
	}
	pcam->SetViewPort(v55);
	int32_t IsPipboyActive = pPlayer->IsPipboyActive();
	if (static_cast<uint8_t>(IsPipboyActive)) {
		p1stPersonNode->Update(kupdate);
		p1stPersonNode->SetAlwaysDraw(balwaysdraw);
		if (pcam_2) {
			if (pcam_2->m_pkParent) {
				pcam_2->m_pkParent->SetTranslate(kcamtransform.m_Translate);
				pcam_2->m_pkParent->Update(kupdate);
			}
		}
		NiPoint3 camera1st_Translatea = NiPoint3_ZERO;
		ShadowSceneNode* v21 = BSShaderManager::GetShadowSceneNode(0);
		v21->SetLightingOffset(camera1st_Translatea);
	}
	apCuller->SetCullMode(BSCullingProcess::BSCP_CULL_NORMAL);
	return IsPipboyActive;
}

// Resets the game world: stops the player's sounds, detaches both of the player's 3D roots, clears the loaded cells,
// obstacles and garbage, reverts every change and reinitialises, optionally opening the start menu.
void Main::PerformGameReset(bool abCreateMainMenu) {
	NiNode* pnode_2;
	NiNode* pnode;
	bool bonIdle = bOnIdle;
	bOnIdle = 0;
	pPlayer->StopHeartbeatSound();
	bool bresetAllowChanges = pSaveLoadGame->SetGlobalAllowChanges(0);
	BSAudio* v2 = &BSAudio::QInstance();
	// Every sound flag: all sounds end.
	v2->EndAllSoundsOfType(0xFFFFFFFFu);
	FalloutRadio::PipboyRadioEnable(0);
	if (pPlayer->GetParentCell()) {
		pPlayer->GetParentCell()->RemoveReference(pPlayer);
	}
	NiAVObject* p3D = pPlayer->Get3D(1);
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
	NiAVObject* p3D_2 = pPlayer->Get3D(0);
	if (p3D_2) {
		pnode_2 = p3D_2->IsNode();
	} else {
		pnode_2 = 0;
	}
	if (pnode_2) {
		if (pnode_2->m_pkParent) {
			NiNode* v5 = pnode_2->m_pkParent;
			v5->DetachChild(p3D_2);
		}
	}
	TES::pInstance->ClearAllCells(0, 0);
	// INT32_MAX: no current grid cell, as TES's constructor sets it.
	TES::pInstance->iCurrentGridX = INT32_MAX;
	TES::pInstance->iCurrentGridY = INT32_MAX;
	NavMeshObstacleManager* Instance = NavMeshObstacleManager::GetInstance();
	Instance->ClearAllObstacles();
	GarbageCollector::ClearAll(0);
	NiObjectGarbage::ClearAll(0);
	pPlayer->Set3D(0, 0);
	pSaveLoadGame->RevertAllChanges();
	ReferenceProcessLists.InitPackageLocations();
	pDataHandler->UpdateAutoCalcNPCs();
	BSShaderAccumulator* Accumulator = BSShaderManager::GetAccumulator();
	Accumulator->ClearAllQueries();
	InitializationFlow();
	Interface::CloseAllSupportMenus(0);
	TES::pInstance->CleanupTemporaryParticleCache();
	if (abCreateMainMenu) {
		Interface::CreateStartMenu();
	}
	pSaveLoadGame->SetGlobalAllowChanges(bresetAllowChanges);
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
	void* pMemory = NiMemObject::operator new(sizeof(SceneGraph));
	SceneGraph* pSceneGraph = pMemory ? ::new (pMemory) SceneGraph("Menu", true, nullptr) : nullptr;
	spMenuRoot = pSceneGraph;
}

void Main::CreateMenuRootNode3D() {
	void* pMemory = NiMemObject::operator new(sizeof(SceneGraph));
	SceneGraph* pSceneGraph = pMemory ? ::new (pMemory) SceneGraph("3DMenu", false, nullptr) : nullptr;
	sp3DMenuRoot = pSceneGraph;
}

// AutoMemContext as in UpdateOffscreenInterface.
void Main::InitSky() {
	alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
	AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
	::new (aucContext)
		AutoMemContext(MEM_CONTEXT_SKY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 9917);
	InitializationFlow("Initializing Sky...");
	BSFogProperty* pFog = spFog.operator->();
	NiNode* pSkyRoot = spSkyRoot.operator->();
	pSky->Initialize(static_cast<BSMultiBoundNode*>(pSkyRoot), pFog);
	NiNode* pWeatherRoot = spWeatherRoot.operator->();
	pWeatherRoot->AttachChild(pSky->GetSun()->GetSunGlareNode(), true);
	NiDirectionalLight* pSunLight = pSky->GetLight();
	BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->SetSunLight(pSunLight);
	spSkyRoot->UpdateProperties();
	bool bSunbeams = bUseSunbeams.Bool();
	BSShaderManager::GetImageSpaceManager()->SetUseSunbeams(bSunbeams);
	if (bUseSunbeams.Bool()) {
		void* pMemory = NiMemObject::operator new(sizeof(NiStencilProperty));
		NiStencilProperty* pStencil = pMemory ? ::new (pMemory) NiStencilProperty : nullptr;
		pStencil->SetStencilOn(true);
		pStencil->SetStencilPassAction(NiStencilProperty::ACTION_REPLACE);
		pStencil->SetStencilReference(0xFF);
		spSkyRoot->AttachProperty(pStencil);
	}
	NiUpdateData kSkyUpdate;
	kSkyUpdate.~NiUpdateData();
	::new (&kSkyUpdate) NiUpdateData(0.0f, false, false);
	spSkyRoot->Update(kSkyUpdate);
	spWeatherRoot->UpdateProperties();
	NiUpdateData kWeatherUpdate;
	kWeatherUpdate.~NiUpdateData();
	::new (&kWeatherUpdate) NiUpdateData(0.0f, false, false);
	spWeatherRoot->Update(kWeatherUpdate);
	BSShaderManager::PrepareObject(spSkyRoot.operator->(), false, false);
	BSShaderManager::PrepareObject(pSky->GetSun()->GetSunGlareNode(), false, false);
	pContext->~AutoMemContext();
}

void Main::ClearDebugTextRef() {
	Main::pDebugTextReference = nullptr;
}

void Main::SetOpenConsole(bool abValue) {
	Main::bOpenConsole = (bool)((int8_t)abValue);
}

void Main::DestroyInterfaceSceneGraph() {
	Main::spMenuRoot.operator=(static_cast<NiNode*>(0));
}

void Main::DestroyInterface3DSceneGraph() {
	Main::sp3DMenuRoot.operator=(static_cast<NiNode*>(0));
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
		BSShaderAccumulator* Accumulator = BSShaderManager::GetAccumulator();
		if (Accumulator) {
			Accumulator->ClearAllQueries();
		}
		int32_t CastingType = TES::pInstance->pGridCellA->iDimension;
		for (int32_t aiX = 0; aiX < CastingType; ++aiX) {
			for (int32_t aiY = 0; aiY < CastingType; ++aiY) {
				TESObjectCELL* pCell = TES::pInstance->GetGridCell(aiX, aiY)->pCell;
				if (pCell) {
					_D3DLOCKED_RECT* apOutRect;
					NiRenderedTexture* apOutTex;
					pCell->GetCanopyShadowMask(apOutTex, apOutRect);
					if (apOutTex) {
						if (apOutRect) {
							if (apOutRect->pBits) {
								apOutRect->pBits = 0;
							}
						}
					}
				}
			}
		}
		appMain->KillMenuBGTexture();
		// The exe re-fits a full-screen window here (GetWindowLongA, AdjustWindowRectEx, SetWindowPos, with the
		// adjusted rectangle's bottom as y and top - bottom, negative, as the height). A full-screen window is the
		// device's (Direct3D 9's on Windows, dxvk's SDL backend elsewhere), so it is left as it is.
	} else {
		// The exe calls the HDR effect's Shutdown (0xBAD4A0, ClearAdaptedLight's body) with no this set up: the body
		// ignores it.
		if (BSShaderManager::bHDR) {
			ImageSpaceEffectHDR::ClearAdaptedLight();
		}
		if (TES::pInstance->GetWaterSystem()) {
			if (TESWaterSystem::GetWaterEnabled()) {
				TES::pInstance->GetWaterSystem()->DisableWaterSystem(1, 0);
				TES::pInstance->GetWaterSystem()->EnableWaterSystem();
			}
		}
	}
	return 1;
}

bool Main::LostDeviceCallback([[maybe_unused]] void* apData) {
	return true;
}

void Main::InitStartCell(char* pCellName, NiPoint3* a2) {
	const char* v2;
	int32_t y;
	int32_t x;
	TESObjectCELL* cell = 0;
	char v28 = 1;
	char v29 = 1;
	if ((pCellName && BSstrlen(pCellName)) || (v2 = sStartingCell.String(), std::strlen(v2))) {
		if (pCellName && BSstrlen(pCellName)) {
			cell = pDataHandler->GetCellByEditorID(pCellName);
		} else {
			const char* v3 = sStartingCell.String();
			const char* v4 = v3;
			cell = pDataHandler->GetCellByEditorID(v4);
		}
		if (!cell) {
			x = 0;
			y = 0;
			const char* v5 = sStartingCell.String();
			const char* v6 = v5;
			TESWorldSpace* apWorldSpace = pDataHandler->GetExtCellDataFromFileByEditorID(v6, x, y);
			if (apWorldSpace) {
				TES::pInstance->SetWorldSpace(apWorldSpace);
				a2->x = static_cast<double>(x << 12) + 2048.0;
				a2->y = static_cast<double>(y << 12) + 2048.0;
				a2->z = 0.0f;
				TES::pInstance->LeaveInterior(a2);
				cell = pDataHandler->GetCellFromCellCoord(x, y, apWorldSpace, 0);
				v29 = 0;
			}
		}
	} else {
		const char* v8;
		const char* v7 = sStartingCellX.String();
		if (std::strlen(v7) && (v8 = sStartingCellY.String(), std::strlen(v8))) {
			const char* v9 = sStartingCellX.String();
			const char* v10 = v9;
			int32_t aiX = std::atoi(v10);
			const char* v11 = sStartingCellY.String();
			const char* v12 = v11;
			int32_t aiY = std::atoi(v12);
			TESWorldSpace* v20 = static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item);
			for (BSSimpleList<TESWorldSpace*>* ecx0 = pDataHandler->GetWorldSpaceList(); ecx0 && !ecx0->IsEmpty();
				ecx0 = ecx0->m_pkNext) {
				TESWorldSpace* v19 = ecx0->m_item;
				const char* v16 = sStartingWorld.String();
				const char* v13 = v19->GetFormEditorID();
				if (!BSstricmp(v13, v16)) {
					v20 = v19;
					break;
				}
			}
			if (v20) {
				cell = v20->LoadCell(aiX, aiY);
				if (!cell) {
					cell = pDataHandler->GetCellFromCellCoord(aiX, aiY, v20, 1);
				}
			}
		} else {
			v28 = 0;
			v29 = 0;
		}
	}
	if (v29) {
		if (cell) {
			if (cell->GetInterior()) {
				TES::pInstance->LoadInterior(cell, a2);
			} else {
				TESWorldSpace* WorldSpace = cell->GetWorldSpace();
				TES::pInstance->SetWorldSpace(WorldSpace);
				a2->x = static_cast<double>(cell->GetDataX() << 12) + 2048.0;
				a2->y = static_cast<double>(cell->GetDataY() << 12) + 2048.0;
				a2->z = 0.0f;
				TES::pInstance->LeaveInterior(a2);
			}
			v29 = 0;
		} else {
			if (v28) {
				if (sStartingCell.String() && *sStartingCell.String()) {
					Warning("CELLS: Could not find starting cell '%s'.", sStartingCell.String());
				} else {
					if (sStartingCellX.String() &&
						(sStartingCellY.String() && (*sStartingCellX.String() && *sStartingCellY.String()))) {
						if (sStartingWorld.String() && *sStartingWorld.String()) {
							const char* v17 = sStartingWorld.String();
							const char* v15 = sStartingCellY.String();
							Warning("CELLS: Could not find starting cell (%s, %s) in worldspace '%s'.",
								sStartingCellX.String(), v15, v17);
						} else {
							const char* v18 = sStartingCellY.String();
							Warning("CELLS: Could not find starting cell (%s, %s) in default Tamriel worldspace",
								sStartingCellX.String(), v18);
						}
					} else {
						Warning("CELLS: Could not find starting cell for INI data.");
					}
				}
			}
		}
		pDataHandler->SetMasterFileLargeBuffer(-1);
		if (v29) {
			TES::pInstance->InitGridCells(a2, 1);
			TES::pInstance->pGridCellA->AttachAllLand3D();
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void Main::InitPlayerStart(NiPoint3 position) {
	NiMatrix3 result;
	TESObjectCELL* pCell = TES::pInstance->pInteriorCell;
	NiPoint3 rotation = NiPoint3_ZERO;
	if (!pCell) {
		int32_t v10 = uGridsToLoad.UInt() >> 1;
		int32_t v9 = uGridsToLoad.UInt() >> 1;
		pCell = TES::pInstance->GetGridCell(v9, v10)->pCell;
	}
	if (pCell) {
		pCell->GetCOCPlacementInfo(position, rotation);
	}
	pPlayer->SetHeading(rotation.z);
	pPlayer->SetPosition(position);
	if (pCell) {
		pCell->AddReference(pPlayer, 0);
		pIOManager->LoadAllQueuedPriorities();
		FNVD_CONSTRUCTED_LOCAL(AutoMemContext, apMemory, &AutoMemContext::Destruct, MEM_CONTEXT_PLAYER, 1,
			"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\Main.cpp", 0x2612);
		NiAVObject* v1 = pPlayer->Get3D(0);
		v1->UpdateProperties();
		NiAVObject* v11 = pPlayer->Get3D(0);
		ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
		ShadowSceneNode_->AddShadowLight(v11);
	}
	pPlayer->InitScript();
	SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
	NiNode* CameraRoot = WorldSceneGraph->GetCameraRoot();
	CameraRoot->SetTranslate(position);
	NiMatrix3* Orientation = ::new (&result) NiMatrix3(pPlayer->GetOrientation());
	SceneGraph* v5 = Main::GetWorldSceneGraph();
	NiNode* v6 = v5->GetCameraRoot();
	v6->SetRotate(*Orientation);
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, a2, nullptr, 0.0f, 0, 0);
	SceneGraph* v7 = Main::GetWorldSceneGraph();
	NiNode* v8 = v7->GetCameraRoot();
	v8->Update(a2);
}

void Main::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// The PDB places Controls::DeleteInstance in this file.
void Controls::DeleteInstance() {
	if (pInstance) {
		Controls* pControls = pInstance;
		if (pControls)
			delete pControls;
	}
	pInstance = nullptr;
}

void MessageHandler::Close() {
	if (MessageHandler::pHandler) {
		if (!MessageHandler::pHandler->bStatic) {
			if (MessageHandler::pHandler) {
				delete MessageHandler::pHandler;
			}
		}
	}
}

int32_t MessageHandler::QWarningContext() {
	return iMainValue;
}

RegSettingCollection::RegSettingCollection() {}

BSFadeNodeCuller::BSFadeNodeCuller(NiVisibleArray* apVisSet) : NiCullingProcess(apVisSet) {}

BSFadeNodeCuller::~BSFadeNodeCuller() {}

// The exe's deleting destructor (0x872F00) frees through the engine's CRT.
void BSFadeNodeCuller::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// The width of rendered texture auiIndex through its virtual GetWidth (slot 37), 0 when it is missing. The exe calls
// the shared out-of-line NiPointer::operator-> for the test and again for the call.
uint32_t BSRenderedTexture::GetWidth(int32_t aiIndex) {
	if (!spTexture[aiIndex].operator->()) {
		return 0;
	}
	NiRenderedTexture* pTexture = spTexture[aiIndex].operator->();
	return pTexture->GetWidth();
}

// The height of rendered texture auiIndex through its virtual GetHeight (slot 38), 0 when it is missing.
uint32_t BSRenderedTexture::GetHeight(int32_t aiIndex) {
	if (!spTexture[aiIndex].operator->()) {
		return 0;
	}
	NiRenderedTexture* pTexture = spTexture[aiIndex].operator->();
	return pTexture->GetHeight();
}

// Copies the pre-restore group into the first target group when one exists.
void BSRenderedTexture::UsePreRestoreGroup() {
	if (spPreRestoreGroup.operator->()) {
		// The exe calls the out-of-line NiPointer assignment at 0x6E5CC0.
		spGroup[0] = spPreRestoreGroup;
	}
}

void ImageSpaceEffectFullScreenBlur::CheckCamera(NiCamera* apCam) {
	if (apCam) {
		NiPoint3 v9;
		NiPoint3 result;
		NiPoint3 v12;
		NiPoint3 v8 = apCam->GetWorldTranslate();
		result = apCam->GetWorldDirection();
		v12 = v8 - ImageSpaceEffectFullScreenBlur::kCamLoc;
		v9 = result - ImageSpaceEffectFullScreenBlur::kCamDir;
		float z = v12.z / ImageSpaceEffectFullScreenBlur::kCamLoc.z;
		float y = v12.y / ImageSpaceEffectFullScreenBlur::kCamLoc.y;
		float x = v12.x / ImageSpaceEffectFullScreenBlur::kCamLoc.x;
		NiPoint3 dst(x, y, z);
		float v3 = v9.z / ImageSpaceEffectFullScreenBlur::kCamDir.z;
		float v2 = v9.y / ImageSpaceEffectFullScreenBlur::kCamDir.y;
		float v1 = v9.x / ImageSpaceEffectFullScreenBlur::kCamDir.x;
		NiPoint3 v10(v1, v2, v3);
		ImageSpaceEffectFullScreenBlur::bMoving =
			dst.x > 0.01 || (dst.y > 0.01 || (dst.z > 0.01 || (v10.x > 0.01 || (v10.y > 0.01 || v10.z > 0.01))));
		ImageSpaceEffectFullScreenBlur::kCamLoc = v8;
		ImageSpaceEffectFullScreenBlur::kCamDir = result;
	} else {
		ImageSpaceEffectFullScreenBlur::kCamDir = NiPoint3_UNIT_ALL;
		ImageSpaceEffectFullScreenBlur::bMoving = 0;
	}
}

void BSFileCache::ForcePauseFileCaching() {
	if (eFileCacheState == FILE_CACHING_IN_PROGRESS || eFileCacheState == FILE_CACHING_PAUSED) {
		eFileCacheState = FILE_CACHING_FORCE_PAUSED;
	}
}

// The queue's semaphore (0, 100) is built by BSTCommonScrapHeapMessageQueue's own constructor.
BSPackedTaskQueue::BSPackedTaskQueue(ScrapHeap* apScrapHeap, UnpackFunc apUnpackFunc)
	: Queue(apScrapHeap), pUnpackFunc(apUnpackFunc), bProcessing(false) {}

void BSPackedTaskQueue::ThreadBeginInput() {
	(&Queue.semaphore)->Signal();
}

void BSPackedTaskQueue::ThreadEndInput() {
	(&Queue.semaphore)->Wait();
}

bool BSPackedTaskQueue::AddTask(BSPackedTask* apMessage) {
	// The exe calls Push through the queue's vtable (slot 1).
	return static_cast<BSTMessageQueue*>(&Queue)->Push(apMessage);
}

void FileFinder::AddPath_0(const char* apPath) {
	if (FileFinder::pFinder) {
		FileFinder::pFinder->AddPath(apPath);
	}
}

void FileFinder::Close() {
	if (FileFinder::pFinder) {
		delete FileFinder::pFinder;
	}
}

// The PDB places BGSDestructibleObjectForm::SetLastUpdateTime in this file.
void BGSDestructibleObjectForm::SetLastUpdateTime(float afTime) {
	fLastTime = afTime;
}

void BGSTerrainManager::SetTreeNode(NiNode* apTreeNode) {
	BGSTerrainManager::spTreeNode.operator=(apTreeNode);
}

void BGSTerrainManager::DeallocateStaticNodes() {
	BGSTerrainManager::spTreeNode.operator=(static_cast<NiNode*>(0));
	BGSTerrainManager::spRootLandLODNode.operator=(static_cast<NiNode*>(0));
	BGSTerrainManager::spRootObjectLODNode.operator=(static_cast<NiNode*>(0));
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

// The PDB places FOPipboyManager::GetButtonGlow in this file. The exe calls NiPointer's operator-> out of line.
NiAVObject* FOPipboyManager::GetButtonGlow(uint32_t aiButton) {
	return spButtonGlow[aiButton].operator->();
}

// The PDB places FOPipboyManager::GetPipBoyManagerReset in this file.
bool FOPipboyManager::GetPipBoyManagerReset() {
	return this->bResetPipboyManager;
}

void NiRenderer::SetSorter(NiAccumulator* pkAccum) {
	m_spAccum = pkAccum;
}

bool NiScreenElements::SetTextures(int32_t iPolygon, uint16_t usSet, float fLeft, float fTop, float fRight,
	float fBottom) {
	NiScreenElementsData* ModelData = static_cast<NiScreenElementsData*>(GetModelData());
	return ModelData->SetTextures(iPolygon, usSet, fLeft, fTop, fRight, fBottom);
}

NiScreenElementsData* NiScreenElements::Data() {
	return static_cast<NiScreenElementsData*>(m_spModelData.operator->());
}

void NiSourceTexture::SetDestroyAppDataFlag(bool bDestroy) {
	NiSourceTexture::ms_bDestroyAppData = bDestroy;
}

void ShadowSceneLight::SetClipSpacePlanes(float* apfClipPlanes) {
	if (apfClipPlanes) {
		rt::memcpy(pfClipSpacePlanes, apfClipPlanes, sizeof(pfClipSpacePlanes));
		bClipSpacePlanesReady = true;
	} else {
		bClipSpacePlanesReady = false;
	}
}
