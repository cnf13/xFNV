#include "Fallout Shared/TES.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWindListener.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldM.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSCompoundFrustum.h"
#include "BSMain/BSMasterParticleSystem.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSMultiBoundShape.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSParallelTaskManager.h"
#include "BSMain/BSParticleSystemManager.h"
#include "BSMain/BSPortal.h"
#include "BSMain/BSPortalGraph.h"
#include "BSMain/BSPortalSharedNode.h"
#include "BSMain/BSRangeNode.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTempNodeManager.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSWindModifier.h"
#include "BSMain/IOManager.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/RendererSettingCollection.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMain/SettingCollectionMap.h"
#include "BSMenu/TileMenu.h"
#include "BSMovie/MoviePlayer.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/DistantLODShaderProperty.h"
#include "BSShader/ParticleShader.h"
#include "BSShader/ParticleShaderProperty.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/BSFileCache.h"
#include "BSSystem/BSSystemFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSAddonNode.h"
#include "Fallout Shared/BGSAddonNodeSoundHandleExtra.h"
#include "Fallout Shared/BGSAutoWater.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSSceneInfo.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BSCustomSplatterExtraData.h"
#include "Fallout Shared/BSSplatterExtraData.h"
#include "Fallout Shared/BSTaskThread.h"
#include "Fallout Shared/BSTempNode.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExteriorCellLoader.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenBaseMorphExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenModelExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenMorphData.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridArray.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/Havok/HavokError.h"
#include "Fallout Shared/Havok/HavokStream.h"
#include "Fallout Shared/Havok/HavokStreambufFactory.h"
#include "Fallout Shared/Havok/LoadedAreaBound.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
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
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
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
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESLandTexture.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectTREE.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESTerrainLODManager.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeather.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TempEffects/BSTempEffectSimpleDecal.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/Projectile.h"
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
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/LoveTesterMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/MemoryLevelManager.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "SDK/MSVC/time.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include <system_error>

namespace bhkConvert {} // namespace bhkConvert

// Set while TES::ReloadGridCells runs, so a reentrant call returns at once.
static bool bReloading;

// Set while TES::RunScripts runs, so a reentrant call returns at once.
static bool bScriptsRunning;

// The tick count a scripted loading menu closes at, 0 while none is pending.
static uint32_t uiCloseLoadingMenuTime;
static IO_TASK_PRIORITY elastPriority{};

// Statics of TES::GetCellPriority: the cell it last worked out a priority for, and that priority.
static TESObjectCELL* plastCell;

// Used by TES::SetupTemporaryParticleCache: set once the cache is filled.
static bool bdoOnce;

// Held around the managed-node update when Havok runs on its own thread.
static BSCriticalSection HavokUpdateCrit;

// The profiling timer around UpdateMultiBoundVisibility.
static BSPerformanceTimer TimerUpdateMultibound("Renderer", "Update Multibound Visibility", true, true);

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bPreemptivelyUnloadCells("bPreemptivelyUnloadCells:General", false);
SettingT<INISettingCollection> TES::uInteriorCellBufferGeneral("uInterior Cell Buffer:General", 3u);
SettingT<INISettingCollection> iNumHWThreads("iNumHWThreads:General", 1);
static SettingT<INISettingCollection> sEnvReduceColor("sEnvReduceColor:Weather", "255,255,255,255");
static SettingT<INISettingCollection> sLerpCloseColor("sLerpCloseColor:Weather", "255,255,255,255");
static SettingT<INISettingCollection> sBumpFadeColor("sBumpFadeColor:Weather", "255,255,255,255");
static SettingT<INISettingCollection> fAlphReduce("fAlphaReduce:Weather", 1.0f);
SettingT<INISettingCollection> gs_iUpdateType_HAVOK("iUpdateType:HAVOK", 0);
SettingT<INISettingCollection> gs_fMoveLimitMass_HAVOK("fMoveLimitMass:HAVOK", 95.0f);
SettingT<INISettingCollection> bAddBipedWhenKeyframed_HAVOK("bAddBipedWhenKeyframed:HAVOK", true);
SettingT<INISettingCollection> gs_fMaxPickTime_HAVOK("fMaxPickTime:HAVOK", 0.004f);
SettingT<INISettingCollection> gs_fMaxPickTimeVATS_HAVOK("fMaxPickTimeVATS:HAVOK", 0.04f);
static SettingT<INISettingCollection> fMaxPickTimeDebug("fMaxPickTimeDebug:HAVOK", 0.06f);
static SettingT<INISettingCollection> fMaxPickTimeDebugVATS("fMaxPickTimeDebugVATS:HAVOK", 0.6f);
SettingT<INISettingCollection> gs_iEntityBatchRemoveRate_HAVOK("iEntityBatchRemoveRate:HAVOK", 100);
SettingT<INISettingCollection> gs_fChaseDeltaMult_HAVOK("fChaseDeltaMult:HAVOK", 0.05f);
SettingT<GameSettingCollection> gs_fDebrisMinExtent("fDebrisMinExtent", 2.0f);
SettingT<GameSettingCollection> gs_fDebrisMaxVelocity("fDebrisMaxVelocity", 25.0f);
SettingT<INISettingCollection> gs_fDistanceMultiplier_LOD("fDistanceMultiplier:LOD", 1.2f);
SettingT<INISettingCollection> gs_fFadeOutThreshold_LOD("fFadeOutThreshold:LOD", 0.3f);
SettingT<INISettingCollection> gs_fFadeInThreshold_LOD("fFadeInThreshold:LOD", 0.7f);
SettingT<INISettingCollection> gs_fFadeInTime_LOD("fFadeInTime:LOD", 1.2f);
SettingT<INISettingCollection> gs_fFadeOutTime_LOD("fFadeOutTime:LOD", 1.2f);
SettingT<INISettingCollection> gs_bCheckPurgedTextureList_General("bCheckPurgedTextureList:General", false);
SettingT<INISettingCollection> gs_bQueueWarnings_General("bQueueWarnings:General", false);
SettingT<INISettingCollection> bUseMultiThreadedTrees("bUseMultiThreadedTrees:BackgroundLoad", true);
SettingT<GameSettingCollection> fFadeToBlackFadeSeconds("fFadeToBlackFadeSeconds", 0.3f);
SettingT<GameSettingCollection> fCharacterControllerMultipleStepSpeed("fCharacterControllerMultipleStepSpeed", 25.0f);
SettingT<INISettingCollection> sLanguage("sLanguage:General", "ENGLISH");
SettingT<INISettingCollection>
    sFailureMessage("sFailureMessage:LANGUAGE", "No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection>
    sSysUtil_Retry("sSysUtil_Retry:LANGUAGE", "No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection>
    sSysUtil_AutoSaveWarning("sSysUtil_AutoSaveWarning:LANGUAGE", "No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sSysUtil_GameDataInsufficientSpace(
    "sSysUtil_GameDataInsufficientSpace:LANGUAGE",
    "No Default string.  Must be loaded from INIFile"
);
SettingT<INISettingCollection>
    sSysUtil_GameDataCorrupt("sSysUtil_GameDataCorrupt:LANGUAGE", "No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sSysUtil_SaveDataCreateNew(
    "sSysUtil_SaveDataCreateNew:LANGUAGE",
    "No Default string.  Must be loaded from INIFile"
);
SettingT<INISettingCollection> sSysUtil_SaveDataInsufficientSpace(
    "sSysUtil_SaveDataInsufficientSpace:LANGUAGE",
    "No Default string.  Must be loaded from INIFile"
);
SettingT<INISettingCollection>
    sSysUtil_DiscEject("sSysUtil_DiscEject:LANGUAGE", "No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> gs_bShowLoadingAreaMessage_General("bShowLoadingAreaMessage:General", false);
SettingT<INISettingCollection>
    gs_bSelectivePurgeUnusedOnFastTravel_BackgroundLoad("bSelectivePurgeUnusedOnFastTravel:BackgroundLoad", false);
SettingT<INISettingCollection>
    sSysUtil_DirtyDisc("sSysUtil_DirtyDisc:LANGUAGE", "No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sThumbstick("sThumbstick:LANGUAGE", "No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sDPad("sDPad:LANGUAGE", "No Default string.  Must be loaded from INIFile");
SettingT<RendererSettingCollection> fLODFadeOutMultActors("fLODFadeOutMultActors:LOD", 2.5f);
SettingT<RendererSettingCollection> fLODFadeOutMultItems("fLODFadeOutMultItems:LOD", 3.0f);
SettingT<RendererSettingCollection> fLODFadeOutMultObjects("fLODFadeOutMultObjects:LOD", 5.0f);

// The shared model-data getter's class (TESActorBaseData) has its field at another offset than m_usVertices with
// 8-byte pointers, so the field is read directly.
uint16_t NiGeometry::GetVertexCount() {
    return this->m_spModelData->m_usVertices;
}

void MakeNewGeomData(NiAVObject* apObject) {
    NiPointer<NiObject> spTempObject(nullptr);
    if (apObject) {
        NiGeometry* pGeom = apObject->IsGeometry();
        if (pGeom) {
            NiPointer<NiGeometryData> spData(pGeom->GetModelData());
            if (spData.m_pObject) {
                if (spData->GetConsistency() || !spData->m_pkVertex) {
                    pGeom->SetModelData(spData.m_pObject);
                }
                else {
                    spData->CreateDeepCopy(spTempObject);
                    spData = static_cast<NiGeometryData*>(spTempObject.m_pObject);
                    pGeom->SetModelData(spData.m_pObject);
                }
            }
        }
        else {
            NiNode* pNode = apObject->IsNode();
            if (pNode) {
                uint32_t I = pNode->GetArrayCount();
                while (I--) {
                    MakeNewGeomData(pNode->GetAt(I));
                }
            }
        }
    }
}

// Reads no argument and returns 0, as the game's file printer does.
int32_t PrintToFile(const char* apFileName, const char* apFormat, ...) {
    return 0;
}

float HK2NI(float afhk) {
    float fResult = bhkConvert::fHk2BSScaleSC * afhk;
    return fResult;
}

NiTransform& HK2NI(NiTransform& arNiTran, const hkTransform& arhkTran) {
    HK2NI(arNiTran.m_Rotate, arhkTran.m_rotation);
    HK2NI(arNiTran.m_Translate, arhkTran.getTranslation());
    return arNiTran;
}

hkQuaternion& NI2HKMIGRATION_QUATERNION(hkQuaternion& arhkQ, const NiQuaternion& arNiQ) {
    float fX = arNiQ.m_fX;
    arhkQ.m_vec(0) = fX;
    float fY = arNiQ.m_fY;
    arhkQ.m_vec(1) = fY;
    float fZ = arNiQ.m_fZ;
    arhkQ.m_vec(2) = fZ;
    float fW = arNiQ.m_fW;
    arhkQ.m_vec(3) = fW;
    return arhkQ;
}

void Init() {
    char cRootDir[268];
    AutoMemContext kMemContext(
        MEM_CONTEXT_HAVOK,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Havok\\HavokStream.cpp",
        172
    );
    bhkWorld::InitSystem();
    hkMonitorStream::init();
    HavokError* pErrorMemory = static_cast<HavokError*>(hkReferencedObject::newObject(sizeof(HavokError)));
    HavokError* pError = pErrorMemory ? ::new (pErrorMemory) HavokError : nullptr;
    hkSingleton<hkError>::replaceInstance(pError);
    HavokStreambufFactory* pFactoryMemory = static_cast<HavokStreambufFactory*>(
        hkReferencedObject::newObject(sizeof(HavokStreambufFactory))
    );
    HavokStreambufFactory* pFactory = pFactoryMemory ? ::new (pFactoryMemory) HavokStreambufFactory : nullptr;
    hkSingleton<hkFileSystem>::replaceInstance(pFactory);
    // GetCurrentDirectoryA(260, cRootDir): the buffer is left as it is when the folder is unknown or does not fit.
    std::error_code kError;
    const std::string kCurrentDirectory = std::filesystem::current_path(kError).string();
    if (!kError && kCurrentDirectory.size() < 260) {
        std::strcpy(cRootDir, kCurrentDirectory.c_str());
    }
    pError->SetRootDir(cRootDir);
    bhkWorldCinfo::SetSimulationType(gs_iSimType_HAVOK.Int());
}

TES::ParticleObjectCache::ParticleObjectCache() : spModel(nullptr), pspClones(nullptr), pNext(nullptr) {}

TES::ParticleObjectCache::~ParticleObjectCache() {
    delete[] pspClones;
}

void TES::ParticleObjectCache::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// Every field starts zeroed through its initialiser before the stores below.
TES::TES(const char* apPathname, NiNode* apRoot, NiNode* apLODLandRoot, Sky* apSky, NiNode* apLODWaterRoot)
  : pGridDistantA(nullptr), pGridCellA(nullptr), pObjRoot(nullptr), pObjLandRoot(nullptr), pObjLODWaterRoot(nullptr),
    pTempNodeManager(nullptr), pObjLight(nullptr), pObjFog(nullptr), iCurrentGridX(0), iCurrentGridY(0),
    iCurrentQueuedX(0), iCurrentQueuedY(0), pInteriorCell(nullptr), pInteriorBuffer(nullptr), pExteriorBuffer(nullptr),
    iTempInteriorBufferSize(0), iTempExteriorBufferSize(0), iSaveGridX(0), iSaveGridY(0), bCollisionBoxes(false),
    bRunningCellTests(false), bRunningCellTests2(false), bRunningSubmergedObjectTest(false),
    pfnTACCallbackFunc(nullptr), pTACCallbackData(nullptr), pTACRegionFilter(nullptr), bShowLANDborders(false),
    pWaterSystem(nullptr), pSky(nullptr), iTotalToLoad(0), iLoaded(0), bDisablePercentageUpdate(false),
    bUpdateGridString(false), fCell_delta_x(0.0f), fCell_delta_y(0.0f), pWorldSpace(nullptr),
    pParticleCacheHead(nullptr), bFadeWhenLoading(false), bAllowUnusedPurge(false), iPlaceableWaterCount(0),
    pNavMeshInfoMap(nullptr) {
    AutoMemContext kMemContext(MEM_CONTEXT_GAME_OVERHEAD);
    NiObjectNET::SetDefaultCopyType(NiObjectNET::COPY_EXACT);
    Init();
    bhkWorld::SetUpdateType(gs_iUpdateType_HAVOK);
    bhkWorld::SetRecFunction(bhkWorldNS::OA_ADD, &TESObjectREFR::FilteredAddObject);
    bhkWorld::SetAddBipedWhenKeyframed(bAddBipedWhenKeyframed_HAVOK.Bool());
    bhkWorld::SetDebrisMaxVelocity(gs_fDebrisMaxVelocity.Float());
    bhkWorld::SetDebrisMinExtent(gs_fDebrisMinExtent.Float());
    bhkWorld::SetChaseDeltaMult(gs_fChaseDeltaMult_HAVOK.Float());
    bhkWorld::SetMaxPickTimeNormal(gs_fMaxPickTime_HAVOK.Float());
    bhkWorld::SetMaxPickTimeVATS(gs_fMaxPickTimeVATS_HAVOK.Float());
    bhkWorld::SetEntityBatchRemoveRate(static_cast<uint32_t>(gs_iEntityBatchRemoveRate_HAVOK.Int()));
    bhkCharacterListener::SetMoveLimitMass(gs_fMoveLimitMass_HAVOK);
    bhkCharacterController::SetMultipleStepSpeed(
        fCharacterControllerMultipleStepSpeed.Float() * fCharacterControllerMultipleStepSpeed.Float()
    );
    bhkCharacterController::SetMinHurtfulMass(fPhysicsDamage1Mass.Float());
    TES::InitFadeOutMultipliers();
    BSFadeNode::SetFadeOutMultiplier(LOD_MULT_DISTANTLOD, 0.0f);
    BSFadeNode::SetFadeOutMultiplier(LOD_MULT_ACTOR_BODY_PART, -1.0f);
    BSFadeNode::SetFadeOutThreshold(gs_fFadeOutThreshold_LOD.Float());
    BSFadeNode::SetFadeInThreshold(gs_fFadeInThreshold_LOD.Float());
    BSFadeNode::SetFadeInTime(gs_fFadeInTime_LOD.Float());
    BSFadeNode::SetFadeOutTime(gs_fFadeOutTime_LOD.Float());
    BSFadeNode::SetDistanceMultiplier(gs_fDistanceMultiplier_LOD.Float());
    MessageHandler::SetWarningQueueing(gs_bQueueWarnings_General.Bool());
    BSTexturePalette::SetCheckPurgedList(gs_bCheckPurgedTextureList_General.Bool());
    bhkNiCollisionObject::SetTransChangeCallback(&TESObjectREFR::TransChangeCallback);
    bhkNiCollisionObject::SetDebugDisplayCallback(&TESObjectREFR::DebugDisplayCallback);
    pGridCellA = new GridCellArray();
    pGridCellA->Init();
    {
        AutoMemContext kLoadersContext(MEM_CONTEXT_LOADERS);
        ExteriorCellLoader::Initialize();
        pIOManager = new IOManager();
        pModelLoader = new ModelLoader();
    }
    if (iNumHWThreads.Int() > 1) {
        pParallelTaskManager = new BSParallelTaskManager();
    }
    pDataHandler = new TESDataHandler();
    pDataHandler->BuildFileList(apPathname);
    pSaveLoadGameOLD = new TESSaveLoadGame();
    BGSSaveLoadGame::Create();
    BGSSaveLoadManager::Create();
    int32_t iGrids = uGridsToLoad.Int();
    if (static_cast<uint32_t>(uExteriorCellBuffer) < static_cast<uint32_t>(iGrids * iGrids + iGrids + iGrids + 1)) {
        uExteriorCellBuffer = iGrids + iGrids * iGrids + iGrids + 1;
    }
    if (!static_cast<uint32_t>(TES::uInteriorCellBufferGeneral)) {
        TES::uInteriorCellBufferGeneral = 1;
    }
    pInteriorBuffer = static_cast<TESObjectCELL**>(
        ourMalloc(TES::uInteriorCellBufferGeneral.UInt() * sizeof(TESObjectCELL*))
    );
    pExteriorBuffer = static_cast<TESObjectCELL**>(ourMalloc(uExteriorCellBuffer.UInt() * sizeof(TESObjectCELL*)));
    for (uint32_t n = 0; n < static_cast<uint32_t>(TES::uInteriorCellBufferGeneral); ++n) {
        pInteriorBuffer[n] = nullptr;
    }
    for (uint32_t n = 0; n < static_cast<uint32_t>(uExteriorCellBuffer); ++n) {
        pExteriorBuffer[n] = nullptr;
    }
    InitVars();
    pObjRoot = apRoot;
    pTempNodeManager = new BSTempNodeManager();
    if (pObjRoot) {
        pObjRoot->AttachChild(pTempNodeManager, 1);
    }
    pObjLandRoot = apLODLandRoot;
    pObjLODWaterRoot = apLODWaterRoot;
    pSky = apSky;
    pObjLight = pSky ? pSky->GetLight() : nullptr;
    pObjFog = pSky ? pSky->GetFog() : nullptr;
    bUpdateGridString = false;
    pWorldSpace = nullptr;
    pParticleCacheHead = nullptr;
    bFadeWhenLoading = true;
    if (uGridsArea < static_cast<uint32_t>(uGridsToLoad.Int() * uGridsToLoad.Int())) {
        uGridsArea = uGridsToLoad.Int() * uGridsToLoad.Int();
    }
    bShowLANDborders = false;
    bRunningCellTests = false;
    bRunningCellTests2 = false;
    bAllowUnusedPurge = true;
    pNavMeshInfoMap = nullptr;
    BSRangeNode::SetVisibilityChangedCB(&TES::RangedNodeVisibilityChangedCB);
    spLoadedAreaBound = new LoadedAreaBound(20.0f, 600.0f, 1000.0f, 0.3f);
    pTACRegionFilter = nullptr;
}

TES::~TES() {
    BSRangeNode::SetVisibilityChangedCB(nullptr);
    pDataHandler->SetIsClosingFile(true);
    appCalendar.ClearPointers();
    ExteriorCellLoader::Release();
    pPlayer->Set3D(nullptr, false);
    pSaveLoadGameOLD->m_bAllowChanges = false;
    pObjFog = nullptr;
    pObjLight = nullptr;
    pSky = nullptr;
    ReferenceProcessLists.UnloadAllActorPermanentMagic();
    ClearAllCells(false, false);
    ClearBedChairList();
    ClearDeadCountList();
    BSFaceGenManager::ClearModelCache();
    BSTreeManager::ClearModelCache(false);
    CleanupTemporaryParticleCache();
    UnloadDefaultModels();
    UnloadAddonNodes();
    UnPreloadForms();
    BSTempEffectSimpleDecal::UnloadDecalData();
    MiscStatManager::ReleaseMemory();
    pVATS.ClearData();
    pDataHandler->ClearData();
    FreeWater();
    pDataHandler->SetIsClosingFile(true);
    NiObjectGarbage::ClearAll(false);
    delete pModelLoader;
    pModelLoader = nullptr;
    delete pIOManager;
    pIOManager = nullptr;
    delete pParallelTaskManager;
    pParallelTaskManager = nullptr;
    delete pGridCellA;
    delete pSaveLoadGameOLD;
    pSaveLoadGameOLD = nullptr;
    BGSSaveLoadGame::Destroy();
    BGSSaveLoadManager::Destroy();
    spLoadedAreaBound = nullptr;
    delete pDataHandler;
    pDataHandler = nullptr;
    ourFree(pInteriorBuffer);
    ourFree(pExteriorBuffer);
    listLastLoadedExteriors.RemoveAll();
    TESWaterSystem::ClearWaterRoot();
    for (int32_t i = 0; i < 4; ++i) {
        if (BSShaderManager::GetShadowSceneNode(i)) {
            BSShaderManager::GetShadowSceneNode(i)->RemoveAllLights();
            BSShaderManager::ProcessAllQueuedLights();
        }
    }
    KillHavokSystem();
    listActiveImageSpaceModifiers.RemoveAll();
}

bool TES::GetMapNameForLocation(BSStringT<char>& aMapName, NiPoint3 aWorldLoc, TESWorldSpace* apWorldSpace) {
    TESWorldSpace* pWorldSpace = apWorldSpace ? apWorldSpace : GetWorldSpace();
    return pWorldSpace && pWorldSpace->GetMapNameForLocation(aMapName, aWorldLoc);
}

void TES::RunCellTest(uint32_t aiOptions) {
    bRunningCellTests = true;
    PlayerCharacter::SetGodMode(true);
    InitializationFlow("Running Cell Test");
    Interface::EmergencyCloseAllMenusAndBreakStuff();
    uint32_t uiCellTestStartTime = BSGetTickCount();
    for (BSSimpleList<TESWorldSpace*>* pList = pDataHandler->GetWorldSpaceList(); pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        TESWorldSpace* pWorld = pList->m_item;
        if (pWorld) {
            SetWorldSpace(pWorld);
            int32_t iMinX = static_cast<int32_t>(pWorld->GetMinX()) >> 12;
            int32_t iMinY = static_cast<int32_t>(pWorld->MinimumCoords.Y) >> 12;
            int32_t iMaxX = static_cast<int32_t>(pWorld->MaximumCoords.X) >> 12;
            int32_t iMaxY = static_cast<int32_t>(pWorld->MaximumCoords.Y) >> 12;
            const char* pWorldSpaceName = pWorld->GetFullName();
            if (!pWorldSpaceName || !BSstrlen(pWorldSpaceName)) {
                pWorldSpaceName = pWorld->GetFormEditorID();
            }
            InitializationFlow(
                "Starting world %08X %s with bounds (%i,%i) to (%i,%i)",
                pWorld->iFormID,
                pWorldSpaceName,
                iMinX,
                iMinY,
                iMaxX,
                iMaxY
            );
            for (int32_t x = iMinX; x < iMaxX; x += uGridsToLoad.Int()) {
                for (int32_t y = iMinY; y < iMaxY; y += uGridsToLoad.Int()) {
                    TESObjectCELL* pCell = pWorld->GetCellFromCellCoord(x, y);
                    if (!pCell) {
                        pCell = pWorld->LoadCell(x, y);
                    }
                    if (pCell) {
                        TestCell(pCell, aiOptions, uiCellTestStartTime);
                    }
                }
            }
        }
    }
    pDataHandler->SortInteriorCells();
    uint32_t uiInteriorCount = pDataHandler->GetInteriorCellCount();
    for (uint32_t i = 0; i < uiInteriorCount; ++i) {
        TESObjectCELL* pCell = pDataHandler->GetInteriorCellByIndex(i);
        TestCell(pCell, aiOptions, uiCellTestStartTime);
    }
    bRunningCellTests2 = false;
}

TESLandTexture* TES::GetLandTexture(const NiPoint3& aCoord) {
    TESLandTexture* pRetTex = nullptr;
    int32_t ix = FloatToInt(aCoord.x);
    int32_t iy = FloatToInt(aCoord.y);
    if (!(ix % 0x1000) && aCoord.x < static_cast<double>(ix)) {
        --ix;
    }
    if (!(iy % 0x1000) && aCoord.y < static_cast<double>(iy)) {
        --iy;
    }
    if (ix - ix % 128 == iGetLandTexLastX) {
        return pGetLandTexLastTex;
    }
    TESObjectCELL* pTestCell = GetGridCellCell(ix >> 12, iy >> 12);
    if (pTestCell) {
        TESObjectLAND* pLand = pTestCell->GetLand();
        pRetTex = pLand->GetMainTexture(aCoord);
        if (pRetTex) {
            pGetLandTexLastTex = pRetTex;
            iGetLandTexLastX = ix - ix % 128;
            iGetLandTexLastY = iy - iy % 128;
        }
    }
    return pRetTex;
}

HavokMaterialType TES::GetLandMaterialType(const NiPoint3& aCoord) {
    TESLandTexture* pLandTexture = GetLandTexture(aCoord);
    if (pLandTexture) {
        return pLandTexture->GetHavokMaterialType();
    }
    return BHK_MATERIAL_DIRT;
}

void TES::InitVars() {
    // 0x7FFFFFFF marks "no cell"
    iCurrentGridX = 0x7FFFFFFF;
    iCurrentGridY = 0x7FFFFFFF;
    iCurrentQueuedX = 0x7FFFFFFF;
    iCurrentQueuedY = 0x7FFFFFFF;
    iSaveGridX = 0x7FFFFFFF;
    iSaveGridY = 0x7FFFFFFF;
}

BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* TES::GetActiveImageSpaceModifiers() {
    return &listActiveImageSpaceModifiers;
}

TESObjectCELL* TES::GetInterior() const {
    return pInteriorCell;
}

void TES::TogglePrimitives() {
    TESObjectCELL* pCell = pInteriorCell;
    bool* toggle = &TESObjectREFR::bPrimitivesOn.Bool();
    // Tests the whole stored byte: any nonzero byte toggles to 0, not only 1.
    *toggle = static_cast<uint8_t>(TESObjectREFR::bPrimitivesOn.Bool()) == 0;
    bool bShow = *toggle;
    if (pCell) {
        pCell->ShowPrimitives(bShow);
    }
    else {
        pGridCellA->ShowPrimitives(bShow);
    }
}

void TES::SetInterior(TESObjectCELL* apCell) {
    pInteriorCell = apCell;
}

TESWorldSpace* TES::GetWorldSpace() const {
    return pWorldSpace;
}

void TES::ClearWorldSpace() {
    pWorldSpace = nullptr;
}

TESWaterSystem* TES::GetWaterSystem() const {
    return pWaterSystem;
}

void TES::IncrementPlaceableWaterCount() {
    ++iPlaceableWaterCount;
}

BSSimpleList<TESObjectREFR*>* TES::GetBedChairList() {
    return &ListofBedsAndChairs;
}

void TES::ClearBedChairList() {
    ListofBedsAndChairs.RemoveAll();
}

bool TES::GetAllowUnusedPurge() const {
    return bAllowUnusedPurge;
}

TESObjectCELL* TES::GetInteriorBufferCellByIndex(uint32_t aiIndex) const {
    return pInteriorBuffer[aiIndex];
}

GridCell* TES::GetGridCell(int32_t aix, int32_t aiy) {
    return pGridCellA->Get(aix, aiy);
}

// Tests each flag's whole byte and returns 0 or 1, so the bytes are read as integers.
bool TES::CellTestRunning() const {
    return reinterpret_cast<const uint8_t&>(bRunningCellTests) != 0 ||
        reinterpret_cast<const uint8_t&>(bRunningCellTests2) != 0;
}

int16_t TES::GetDeadCount(TESActorBase* apObject) {
    for (BSSimpleList<DeathCount*>* node = &DeadCount; node && node->m_item; node = node->m_pkNext) {
        if (node->m_item->pActorBase == apObject) {
            return node->m_item->usCount;
        }
    }

    return 0;
}

bool TES::IsCellLoaded(TESObjectCELL* apCell, bool abIgnorebuffered) {
    if (!apCell) {
        return false;
    }

    switch (apCell->GetState()) {
        // states that only count when buffered cells are not ignored
        case 2:
        case 3:
        case 4: return !abIgnorebuffered;
        case 5:
        case 6: return true;
        default: return false;
    }
}

bool TES::IsInteriorBuffered(TESObjectCELL* apCell) {
    for (uint32_t index = 0; index < static_cast<uint32_t>(uInteriorCellBufferGeneral); ++index) {
        if (pInteriorBuffer[index] == apCell) {
            return true;
        }
    }

    return false;
}

TESObjectCELL* TES::GetCurrentCell() {
    if (GetInterior()) {
        return GetInterior();
    }

    if (iCurrentGridX == 0x7FFFFFFF || iCurrentGridY == 0x7FFFFFFF) {
        return nullptr;
    }

    return GetGridCellCell(iCurrentGridX, iCurrentGridY);
}

TESObjectCELL* TES::GetGridCellCell(int32_t aiCellX, int32_t aiCellY) {
    int32_t iX = static_cast<int32_t>(aiCellX - (iCurrentGridX - (uGridsToLoad.UInt() >> 1)));
    int32_t iY = static_cast<int32_t>(aiCellY - (iCurrentGridY - (uGridsToLoad.UInt() >> 1)));
    TESObjectCELL* pCell = nullptr;
    if (static_cast<uint32_t>(iX) < uGridsToLoad.UInt() &&
        static_cast<uint32_t>(iY) < uGridsToLoad.UInt() &&
        iX >= 0 &&
        iY >= 0) {
        pCell = pGridCellA->Get(iX, iY)->pCell;
    }
    if (!pCell && GetWorldSpace()) {
        return pDataHandler->GetCellFromCellCoord(aiCellX, aiCellY, GetWorldSpace(), false);
    }

    return pCell;
}

bool TES::PickWorldDistance(
    NiPick* apPick,
    const NiPoint3& aOrigin,
    const NiPoint3& aDir,
    float afDistance,
    char acNodeFlags
) {
    uint8_t cPicked = 0;
    if (apPick && acNodeFlags) {
        apPick->SetTarget(nullptr);
        apPick->PickObjects(NiPoint3::ZERO, NiPoint3::ZERO, false);
        if (TES::pInstance->TES::GetInterior()) {
            TESObjectCELL* pInterior = TES::pInstance->TES::GetInterior();
            return pInterior->PickWorldDistance(apPick, aOrigin, aDir, afDistance, acNodeFlags);
        }

        for (uint32_t uiX = 0; uiX < uGridsToLoad.UInt(); ++uiX) {
            for (uint32_t uiY = 0; uiY < uGridsToLoad.UInt(); ++uiY) {
                TESObjectCELL* pCell = GetGridCell(uiX, uiY)->pCell;
                if (pCell && pCell->PickWorldDistance(apPick, aOrigin, aDir, afDistance, acNodeFlags)) {
                    cPicked = 1;
                }
            }
        }
    }

    return cPicked != 0;
}

bool TES::PickWaterDistance(NiPick* apPick, const NiPoint3& aOrigin, const NiPoint3& aDir, float afDistance) {
    char cPicked = 0;
    if (apPick) {
        if (TES::pInstance->TES::GetInterior()) {
            TESObjectCELL* pInteriorCell = TES::pInstance->TES::GetInterior();
            return pInteriorCell->PickWaterDistance(apPick, aOrigin, aDir, afDistance);
        }

        for (uint32_t i = 0; i < uGridsToLoad.UInt(); ++i) {
            for (uint32_t j = 0; j < uGridsToLoad.UInt(); ++j) {
                TESObjectCELL* pCell = GetGridCell(i, j)->pCell;
                if (pCell && pCell->PickWaterDistance(apPick, aOrigin, aDir, afDistance)) {
                    cPicked = 1;
                }
            }
        }
    }
    return cPicked;
}

// Empties the last loaded exteriors, detaches the grid (unless only buffered cells go and there is no interior),
// frees the buffers and, unless only buffered cells go, the current interior.
void TES::ClearAllCells(bool abBufferedonly, bool abDoNotSetInterior) {
    GetLastLoadedExteriors()->RemoveAll();
    if (!abBufferedonly || GetInterior()) {
        DetachGridCells();
    }

    BSAudio::QInstance().EndAllMovingSounds();
    FreeExteriorBuffer(abBufferedonly, nullptr);
    if (GetInterior() && !abBufferedonly && !IsInteriorBuffered(GetInterior())) {
        pDataHandler->UnloadCell(GetInterior());
    }

    FreeInteriorBuffer(abBufferedonly);
    if (!abBufferedonly && !abDoNotSetInterior) {
        SetInterior(nullptr);
    }

    bFadeWhenLoading = true;
}

// Purges the unused textures (when purging is allowed or abWaitForCompletion), with the palette locked and the tree
// model cache cleared before and after.
void TES::CleanUpUnusedTextures(bool abWaitForCompletion) {
    if (!Renderer::QCanResume()) {
        return;
    }

    BSTexturePalette::Lock();
    BSTreeManager::ClearModelCache(true);
    if (GetAllowUnusedPurge() || abWaitForCompletion) {
        BSTexturePalette::PurgeUnusedTextures();
    }

    BSTreeManager::ClearModelCache(true);
    BSTexturePalette::Unlock();
}

// The priority of loads for apCell, from its grid distance to the current cell (or to apPos): 20 or nearer is very
// high, anything farther mid. The answer is cached for the last cell asked about.
IO_TASK_PRIORITY TES::GetCellPriority(TESObjectCELL* apCell, NiPoint3* apPos) {
    if (plastCell == apCell) {
        return elastPriority;
    }

    int32_t iGridX;
    int32_t iGridY;
    if (apCell->GetInterior()) {
        iGridX = static_cast<int32_t>(uGridsToLoad.UInt() >> 1);
        iGridY = static_cast<int32_t>(uGridsToLoad.UInt() >> 1);
    }
    else if (apPos) {
        int32_t iDataX = apCell->GetDataX();
        int32_t iPosX = FloatToInt(apPos->x) >> 12;
        iGridX = iDataX - (iPosX - static_cast<int32_t>(uGridsToLoad.UInt() >> 1));
        int32_t iDataY = apCell->GetDataY();
        int32_t iPosY = FloatToInt(apPos->y) >> 12;
        iGridY = iDataY - (iPosY - static_cast<int32_t>(uGridsToLoad.UInt() >> 1));
    }
    else {
        int32_t iDataX = apCell->GetDataX();
        iGridX = iDataX - (iCurrentGridX - static_cast<int32_t>(uGridsToLoad.UInt() >> 1));
        int32_t iDataY = apCell->GetDataY();
        iGridY = iDataY - (iCurrentGridY - static_cast<int32_t>(uGridsToLoad.UInt() >> 1));
    }

    int32_t iOffsetX = iGridX - static_cast<int32_t>(uGridsToLoad.UInt() >> 1);
    int32_t iOffsetY = iGridY - static_cast<int32_t>(uGridsToLoad.UInt() >> 1);
    uint8_t cDistance;
    if (iOffsetX > iOffsetY) {
        cDistance = static_cast<uint8_t>(std::abs(iOffsetX) * 10 + 10);
    }
    else {
        cDistance = static_cast<uint8_t>(std::abs(iOffsetY) * 10 + 10);
    }

    if (cDistance <= 20) {
        cDistance = 20;
    }
    else {
        cDistance = 40;
    }

    switch (cDistance) {
        case 10: elastPriority = IO_TASK_PRIORITY_CRITICAL; break;
        case 20: elastPriority = IO_TASK_PRIORITY_VERY_HIGH; break;
        case 30: elastPriority = IO_TASK_PRIORITY_HIGH; break;
        case 40: elastPriority = IO_TASK_PRIORITY_MID; break;
        case 50: elastPriority = IO_TASK_PRIORITY_LOW; break;
        default: break;
    }

    plastCell = apCell;
    return elastPriority;
}

NavMeshInfoMap* TES::GetNavMeshInfoMap() {
    if (!pNavMeshInfoMap && pDataHandler && !pDataHandler->GetSaveLoad()) {
        pNavMeshInfoMap = new NavMeshInfoMap;
    }

    return pNavMeshInfoMap;
}

// Replacing an existing map warns when the new one is not null, and deletes the old one.
void TES::SetNavMeshInfoMap(NavMeshInfoMap* apNavMeshInfoMap) {
    if (pNavMeshInfoMap) {
        if (apNavMeshInfoMap) {
            Warning("AI: Multiple NavMeshInfoMaps loaded");
        }

        delete pNavMeshInfoMap;
    }

    pNavMeshInfoMap = apNavMeshInfoMap;
}

// Loads apfilename into aspSourceTexture: from the texture palette when it is there, else from the file (searched in
// the texture archives only when abArchiveOnly), registering the new texture in the palette. A missing file is fine
// with abNoFileOK; a texture that could not be created warns otherwise. The palette fills aspSourceTexture in place as
// a NiPointer<NiTexture>.
void TES::CreateTextureImage(
    const char* apfilename,
    NiPointer<NiSourceTexture>& aspSourceTexture,
    bool abNoFileOK,
    bool abArchiveOnly
) {
    FileFinder::LOOKIN_FLAGS eLookIn = FileFinder::SKIP_NONE;
    // Every archive type the finder knows; 0xFFFF is passed rather than ARCHIVE_TYPE_ALL.
    int32_t iArchiveType = 0xFFFF;
    aspSourceTexture = nullptr;
    BSTexturePalette::GetTexture(apfilename, reinterpret_cast<NiPointer<NiTexture>&>(aspSourceTexture));
    if (aspSourceTexture.m_pObject) {
        return;
    }
    alignas(NiPointer<NiRefObject>) uint8_t aucUnused[sizeof(NiPointer<NiRefObject>)];
    NiPointer<NiRefObject>* pspUnused = ::new (aucUnused) NiPointer<NiRefObject>(nullptr);

    if (abArchiveOnly) {
        eLookIn = static_cast<FileFinder::LOOKIN_FLAGS>(FileFinder::SKIP_CWD | FileFinder::SKIP_PATHS);
        // the texture archives
        iArchiveType = 2;
    }

    if (!FileFinder::Locate(apfilename, nullptr, eLookIn, iArchiveType) && abNoFileOK) {
        pspUnused->~NiPointer<NiRefObject>();
        return;
    }

    NiFixedString kFileName(apfilename);
    aspSourceTexture = NiSourceTexture::Create(kFileName, NiSourceTexture::ms_kDefaultPrefs, true, false);
    if (aspSourceTexture.m_pObject) {
        BSTexturePalette::SetTexture(aspSourceTexture.m_pObject, nullptr);
    }
    else if (!abNoFileOK) {
        Warning("MODELS: TES::CreateTextureImage unable to create image for \"%s\".\r\n", apfilename);
    }
    pspUnused->~NiPointer<NiRefObject>();
}

bool TES::RemoveTextureImage(const char* apPath, uint32_t aiUser) {
    NiPointer<NiTexture> spTexture(nullptr);
    BSTexturePalette::GetTexture(apPath, spTexture);
    if (spTexture.m_pObject && spTexture->m_uiRefCount <= aiUser + 2) {
        BSTexturePalette::RemoveTexture(spTexture.m_pObject);
        return true;
    }

    return false;
}

// The water height at arPos: apCell's (or, with no cell, the interior's, else the worldspace cell's there) when the
// cell has water at arPos, else the first neighbouring cell that does, else the cell's own height. Outside any loaded
// cell it is the worldspace's LOD water height; an interior cell without water at arPos has none (-FLT_MAX). The branch
// for a null cell before the neighbour search cannot be reached, because every path that reaches it has set apCell
// non-null; it is kept deliberately, as the game has it.
float TES::GetWaterHeight(const NiPoint3& arPos, TESObjectCELL* apCell) {
    // -FLT_MAX.
    constexpr float kNoWater = -std::numeric_limits<float>::max();
    float fHeight = kNoWater;
    if (apCell) {
        float fCellHeight;
        if (apCell->GetWaterHeight(arPos, fCellHeight)) {
            return fCellHeight;
        }

        if (apCell->GetInterior()) {
            return kNoWater;
        }

        fHeight = apCell->GetWaterHeight();
    }
    else if (GetInterior()) {
        float fInteriorHeight;
        GetInterior()->GetWaterHeight(arPos, fInteriorHeight);
        return fInteriorHeight;
    }
    else {
        apCell = pInstance->GetWorldSpace()->GetCellFromWorldCoord(arPos);
        if (!apCell) {
            return pInstance->GetWorldSpace()->GetLODWaterHeight();
        }

        fHeight = apCell->GetWaterHeight();
    }

    int32_t iCellX;
    int32_t iCellY;
    if (apCell) {
        iCellX = apCell->GetDataX();
        iCellY = apCell->GetDataY();
    }
    else {
        iCellX = pInstance->GetCurrentGridX();
        iCellY = pInstance->GetCurrentGridY();
    }

    float fNeighbourHeight = 0.0f;
    for (int32_t iX = iCellX - 1; iX < iCellX + 2; ++iX) {
        for (int32_t iY = iCellY - 1; iY < iCellY + 2; ++iY) {
            TESObjectCELL* pCell = pInstance->GetWorldSpace()->GetCellFromCellCoord(iX, iY);
            if (pCell && pCell != apCell && pCell->GetWaterHeight(arPos, fNeighbourHeight)) {
                return fNeighbourHeight;
            }
        }
    }

    return fHeight;
}

// Constructs and destroys an AutoMemContext on the stack and ignores the constructor's result.
void TES::InitializeWater() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext)
        AutoMemContext(MEM_CONTEXT_WATER, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 646);
    pWaterSystem = new TESWaterSystem;
    pContext->~AutoMemContext();
}

// Detaches (abDisable) or reattaches the loaded grid cells, batching the Havok world's removals or adds, then culls
// or shows the land root's parent and updates the land root. apPos is unused.
void TES::CullGridCells(bool abDisable, [[maybe_unused]] NiPoint3* apPos) {
    if (!pDataHandler) {
        return;
    }

    if (TESObjectCELL::ExteriorWorld()) {
        if (abDisable) {
            TESObjectCELL::ExteriorWorld()->StartBatchingRemovals();
        }
        else {
            TESObjectCELL::ExteriorWorld()->StartBatchingAdds();
        }
    }

    for (uint32_t uiX = 0; uiX < uGridsToLoad.UInt(); ++uiX) {
        for (uint32_t uiY = 0; uiY < uGridsToLoad.UInt(); ++uiY) {
            TESObjectCELL* pCell = pGridCellA->Get(uiX, uiY)->pCell;
            if (abDisable) {
                if (pCell && pCell->GetAttached()) {
                    pCell->Detach(false);
                }
            }
            else if (pCell && pCell->IsLoaded()) {
                pGridCellA->AttachToWorld(pObjRoot, uiX, uiY);
                pCell->AttachModels(false);
            }
        }
    }

    if (TESObjectCELL::ExteriorWorld()) {
        if (abDisable) {
            TESObjectCELL::ExteriorWorld()->StopBatchingRemovals(false);
        }
        else {
            TESObjectCELL::ExteriorWorld()->StopBatchingAdds(false);
        }
    }

    if (!abDisable) {
        ReferenceProcessLists.HandleInvalidPositions();
    }

    NiNode* pParent = pObjLandRoot->GetParent();
    if (pParent) {
        pParent->SetAppCulled(abDisable);
        BGSTerrainManager::EnableLODWaterProcessing(!abDisable);
    }

    NiUpdateData kUpdateData(0.0f, false, false);
    pObjLandRoot->Update(kUpdateData);
}

// Drops the temporary 3D of a visible-distant reference.
void TES::CleanTemp3D(TESObjectREFR* apRef) {
    if (apRef->GetVisibleDistant() && apRef->Get3D() && apRef->HasTemp3D()) {
        NiNode* pParent = apRef->Get3D()->GetParent();
        if (pParent) {
            pParent->DetachChild(apRef->Get3D());
        }

        apRef->Set3DSimple(nullptr);
        apRef->SetHasTemp3D(false);
    }
}

// A reference may get 3D unless it is deleted or disabled. The base form's type is read first and never used.
bool TES::CanAttach3D(TESObjectREFR* apRef) {
    apRef->GetObjectReference()->GetFormType();
    return !apRef->GetDelete() && !apRef->GetDisabled();
}

// An actor about to be cloned and moved gets its process level re-evaluated and leaves the temp change list.
void TES::PreCloneMove(TESObjectREFR* apRef) {
    if (apRef->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRef);
        if (pActor) {
            pActor->ChangeProcessLevel();
            ReferenceProcessLists.RemoveActorFromTempChangeList(pActor);
        }
    }
}

// Attaches apRef's 3D (apQueuedReference's clone when it has one, else loaded now) to apCell and the shadow scene, and
// runs the reference's load-time setup: particle addon nodes, lighting, the OnLoad script event, the tree canopy shadow
// and the actor's alpha and scene-graph id. Projectiles and explosions only run their own setup. A levelled creature or
// character's 3D is only detached from its parent. The action flag 0x1000 is OnLoad and the loaded-data flag 2 is "can
// have flames".
void TES::AttachModel(TESObjectREFR* apRef, TESObjectCELL* apCell, QueuedReference* apQueuedReference, bool abMoved) {
    if (!apRef) {
        return;
    }

    if (apRef->IsProjectile()) {
        Projectile* pProj = static_cast<Projectile*>(apRef);
        pProj->Initialize();
        return;
    }

    if (apRef->IsExplosion()) {
        Explosion* pExplosion = static_cast<Explosion*>(apRef);
        pExplosion->Initialize();
        return;
    }

    CleanTemp3D(apRef);
    uint8_t type = apRef->GetObjectReference()->GetFormType();
    if (!CanAttach3D(apRef)) {
        return;
    }

    if (!abMoved) {
        PreCloneMove(apRef);
    }

    NiAVObject* pObj = nullptr;
    if (apQueuedReference && apQueuedReference->GetCloned3D()) {
        pObj = apQueuedReference->GetCloned3D();
        if (!apRef->TESObjectREFR::Get3D()) {
            apRef->Set3DSimple(pObj);
        }
    }
    else {
        pObj = apRef->Load3D(false);
    }

    NiNode* pModel = static_cast<NiNode*>(pObj);
    if (apQueuedReference) {
        apQueuedReference->FinishAttach();
    }

    if (pModel) {
        if (type == LVLC_ID || type == LVLN_ID) {
            if (pModel->GetParent()) {
                NiNode* pParent = pModel->GetParent();
                pParent->DetachChild(pModel);
            }
        }
        else {
            apCell->Load3D();
            BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->AddObject(pModel);
            if (apRef->IsActor()) {
                BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->AddShadowLight(pModel);
            }

            if (apRef->GetVisibleDistant()) {
                apCell->IncLoadedRefsWithVisibleDistantCount();
            }

            bool bAttach = true;
            if (apRef->GetObjectReference()->GetFormType() == PWAT_ID) {
                BGSPlaceableWater* pWat = static_cast<BGSPlaceableWater*>(apRef->GetObjectReference());
                if (pWat->IsAutomaticWater()) {
                    bAttach = false;
                }
            }

            if (bAttach) {
                apCell->AttachReference3D(apRef, false);
                apRef->UpdateReference3D();
            }

            if (apRef->GetDestructible() && apRef->QUnderwater()) {
                TESObjectREFR::RemoveMasterParticleAddonNodes(pModel);
            }
            else if (
                apRef->GetLoadedDataFlag(LOADED_DATA_HAS_ADDON_FLAGS) &&
                (apCell->GetAttached() || apCell->IsAttaching())
            ) {
                TESObjectREFR::AddMasterParticleAddonNodes(pModel);
            }

            if (apRef->IsActor()) {
                Actor* pActor = static_cast<Actor*>(apRef);
                if (pActor) {
                    pActor->UpdateCurrentDebugGeometry();
                }
            }

            if (type == LIGH_ID) {
                const NiPoint3& kAngle = apRef->GetAngleOnReference();
                TESObjectLIGH* pLight = static_cast<TESObjectLIGH*>(apRef->GetObjectReference());
                pLight->FixGravity(kAngle);
            }
            else {
                BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)
                    ->UpdateObjectLighting(pModel, false);
            }
        }
    }

    apRef->InitScript();
    if (!pSaveLoadGame->GetSaveGameLoading()) {
        Script::InitActionList(apRef, apRef->GetExtra());
        Script::SetActionFlag(apRef, apRef->GetExtra(), SCRIPT_ACTION_ONLOAD);
    }

    // The canopy shadow's radius is the shadow radius times the reference's scale, converted to unsigned through a
    // 64-bit integer.
    if (type == TREE_ID) {
        TESObjectTREE* pTree = dynamic_cast<TESObjectTREE*>(apRef->GetObjectReference());
        if (pTree && BSTreeManager::GetUseCanopyShadows()) {
            NiPoint2 kBillboardSize = pTree->GetBillboardSize();
            if (kBillboardSize.x > 200.0 && kBillboardSize.y > 200.0) {
                double dShadowRadius = pTree->GetShadowRadius();
                uint32_t uiRadius = static_cast<uint32_t>(static_cast<int64_t>(apRef->GetScale() * dShadowRadius));
                apCell->PlaceCanopyShadow(apRef->GetLocationOnReference(), uiRadius, 1.0f);
            }
        }
    }

    if (apRef->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRef);
        pActor->UpdateAlpha();
        if (apRef != pPlayer) {
            uint32_t uiID = apRef->GetFormID();
            SetReferenceIDOnScenegraph(pActor->TESObjectREFR::Get3D(), uiID);
        }
    }
}

int32_t TES::UpdateCurrentGridCell(const NiPoint3* apPos, bool abAllowPartialRemovals) {
    NiPoint2 kCenter;
    listLastLoadedExteriors.RemoveAll();
    HideFarLandNormalMaps();
    if (TES::bLODFadeInProgress && !BSShaderManager::bLODFadeInProgress && !pInteriorCell) {
        TES::bLODFadeInProgress = false;
        HideUnneededLandPasses();
    }
    if (pSaveLoadGame->GetSaveGameLoading()) {
        spLoadedAreaBound->Clear();
    }
    GridCellArray* pGridCells = pGridCellA;
    TESObjectCELL* pCurrentCell = GetCurrentCell();
    spLoadedAreaBound->Update(pCurrentCell, pGridCells);
    if (!pDataHandler) {
        return 0;
    }
    if (this->GetCurrentGridX() == 0x7FFFFFFF || this->GetCurrentGridY() == 0x7FFFFFFF) {
        if (apPos) {
            iCurrentGridX = FloatToInt(apPos->x) >> 12;
            iCurrentGridY = FloatToInt(apPos->y) >> 12;
            iCurrentQueuedX = this->GetCurrentGridX();
            iCurrentQueuedY = this->GetCurrentGridY();
        }
        ShowLoadingMenu(true, pWorldSpace, false);
        InitGridCells(apPos, false);
        return 1;
    }

    int32_t iPosX = FloatToInt(apPos->x);
    int32_t iPosY = FloatToInt(apPos->y);
    int32_t iOffsetX = iPosX - ((this->GetCurrentGridX() << 12) + 2048);
    int32_t iOffsetY = iPosY - ((this->GetCurrentGridY() << 12) + 2048);
    float fLoadDistance = 4096.0f;
    if (uGridsToLoad.Int() == 3) {
        fLoadDistance = 3072.0f;
    }
    fCell_delta_x = fLoadDistance - static_cast<double>(std::abs(iOffsetX));
    fCell_delta_y = fLoadDistance - static_cast<double>(std::abs(iOffsetY));
    bool bSaveGameLoading = pSaveLoadGame->GetSaveGameLoading();
    if (!(fCell_delta_x > 0.0) || !(fCell_delta_y > 0.0) || bSaveGameLoading) {
        BSTaskThread::SetUpdateSleepEnabled(false);
        bool bMenuDisplayed = Interface::GetLoadingMenuVisible();
        if (!bMenuDisplayed && pExteriorCellLoader->QueuedCellCount() && !pSaveLoadGame->GetSaveGameLoading()) {
            ShowLoadingMessage("Loading Area (queued cells)...");
            bMenuDisplayed = true;
        }

        int32_t iNewX = iPosX >> 12;
        int32_t iNewY = iPosY >> 12;
        if (pSaveLoadGame->GetSaveGameLoading() && pSaveLoadGame->GetPositioningPlayerCharacter()) {
            iNewX = this->GetCurrentGridX();
            iNewY = this->GetCurrentGridY();
        }
        int32_t iDeltaX = iNewX - this->GetCurrentGridX();
        int32_t iDeltaY = iNewY - this->GetCurrentGridY();
        iCurrentGridX = iNewX;
        iCurrentGridY = iNewY;
        iCurrentQueuedX = this->GetCurrentGridX();
        iCurrentQueuedY = this->GetCurrentGridY();
        BSShaderManager::kOldGridArrayCenter = BSShaderManager::kGridArrayCenter;
        float fCenterY = (static_cast<double>(this->GetCurrentGridY()) + 0.5) * 4096.0;
        float fCenterX = (static_cast<double>(this->GetCurrentGridX()) + 0.5) * 4096.0;
        ::new (&kCenter) NiPoint2(fCenterX, fCenterY);
        float fY = kCenter.y;
        BSShaderManager::kGridArrayCenter.x = kCenter.x;
        BSShaderManager::kGridArrayCenter.y = fY;
        if (BSShaderManager::kOldGridArrayCenter == NiPoint2::ZERO || std::abs(iDeltaX) > 1 || std::abs(iDeltaY) > 1) {
            BSShaderManager::kOldGridArrayCenter = BSShaderManager::kGridArrayCenter;
        }
        BSShaderManager::fGriddArrayLerpStart = BSShaderManager::GetTimer(false);
        TES::bLODFadeInProgress = true;
        BSShaderManager::bLODFadeInProgress = true;
        HideFarLandNormalMaps();
        HideUnneededLandPasses();
        pExteriorCellLoader->PostProcessCompletedTasks();
        if (pExteriorCellLoader->TryCancelTasks()) {
            pExteriorCellLoader->WaitForTasks();
        }
        if (!pInteriorCell || bSaveGameLoading) {
            if (bPreemptivelyUnloadCells.Bool() && FreeInteriorBuffer(true)) {
                CleanUpUnusedTextures(false);
            }

            if (!pWorldSpace) {
                SetWorldSpace(static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item));
            }
            bool bMoveImmediate = false;
            if (std::abs(iDeltaX) > 1 || std::abs(iDeltaY) > 1) {
                bMoveImmediate = true;
            }
            if (TESObjectCELL::ExteriorWorld()) {
                if (bMoveImmediate || !abAllowPartialRemovals) {
                    TESObjectCELL::ExteriorWorld()->StopBatchingAdds(false);
                    TESObjectCELL::ExteriorWorld()->StopBatchingRemovals(false);
                }
                TESObjectCELL::ExteriorWorld()->StartBatchingRemovals();
            }
            pGridCellA->SetCenter(iNewX, iNewY);
            if (TESObjectCELL::ExteriorWorld()) {
                if (bMoveImmediate) {
                    TESObjectCELL::ExteriorWorld()->StopBatchingRemovals(false);
                }
                else {
                    TESObjectCELL::ExteriorWorld()->StopBatchingRemovals(abAllowPartialRemovals);
                }
            }
            if (!bMenuDisplayed && !AllCellsInGridLoaded() && !pSaveLoadGame->GetSaveGameLoading()) {
                ShowLoadingMessage("Loading Area (unloaded cells)...");
                bMenuDisplayed = true;
            }
            if (std::abs(iDeltaX) > 1 || std::abs(iDeltaY) > 1) {
                InitModelsToLoad(MODEL_COUNT_PRELOAD);
            }
            if (TESObjectCELL::ExteriorWorld()) {
                TESObjectCELL::ExteriorWorld()->StartBatchingAdds();
            }
            GridArrayLoad();
            SetUpdateGridString();
            if (TESObjectCELL::ExteriorWorld()) {
                if (bMoveImmediate) {
                    TESObjectCELL::ExteriorWorld()->StopBatchingAdds(false);
                }
                else {
                    TESObjectCELL::ExteriorWorld()->StopBatchingAdds(abAllowPartialRemovals);
                }
            }
        }
        CleanUpUnusedTextures(false);
        TESWaterListener::RefreshExteriorWaterHeights();
        if (!pInteriorCell) {
            const NiPoint3& kPlayerPos = pPlayer->GetLocationOnReference();
            BGSTerrainManager* pTerrainManager = pWorldSpace->GetTerrainManager();
            pTerrainManager->Update(kPlayerPos, BGSTerrainManager::UPDATE_ALL);
        }
        BSTaskThread::SetUpdateSleepEnabled(true);
        return 1;
    }

    if (bBackgroundCellLoads.Bool() && !pInteriorCell) {
        int32_t iCellX = iPosX >> 12;
        int32_t iCellY = iPosY >> 12;
        if (iCurrentQueuedX != iPosX >> 12 || iCurrentQueuedY != iCellY) {
            if (bPreemptivelyUnloadCells.Bool() && FreeInteriorBuffer(true)) {
                CleanUpUnusedTextures(false);
            }

            int32_t iHalfGrid = uGridsToLoad.UInt() >> 1;
            int32_t iQueueX = iCurrentQueuedX;
            int32_t iDeltaX = iCellX - iCurrentQueuedX;
            if (iDeltaX) {
                iQueueX += iDeltaX * (iHalfGrid + 1);
                for (int32_t iY = -iHalfGrid; iY <= iHalfGrid; ++iY) {
                    pExteriorCellLoader->QueueCellLoad(pWorldSpace, iQueueX, iCurrentQueuedY + iY);
                }
            }
            int32_t iQueueY = iCurrentQueuedY;
            int32_t iDeltaY = iCellY - iCurrentQueuedY;
            if (iDeltaY) {
                iQueueY += iDeltaY * (iHalfGrid + 1);
                for (int32_t iX = -iHalfGrid; iX <= iHalfGrid; ++iX) {
                    pExteriorCellLoader->QueueCellLoad(pWorldSpace, iCurrentQueuedX + iX, iQueueY);
                }
            }
            if (iDeltaX && iDeltaY) {
                pExteriorCellLoader->QueueCellLoad(pWorldSpace, iQueueX, iQueueY);
            }
        }
        iCurrentQueuedX = iCellX;
        iCurrentQueuedY = iCellY;
    }
    return 0;
}

bool TES::IsBuffered(TESObjectCELL* apCell) {
    if (!apCell) {
        return false;
    }

    if (apCell->GetInterior()) {
        return IsInteriorBuffered(apCell);
    }

    return IsExteriorBuffered(apCell);
}

// The per-frame cell update: the wind (none indoors), the cell animation clock, the managed nodes under the Havok
// lock, the parallel tasks, the collision damage, and with one hardware thread the sky and the main-thread part.
void TES::UpdateCellAnimations(float afTime) {
    TESObjectCELL* pInterior = GetInterior();
    if (pInterior) {
        BSWindModifier::SetWind(0.0f, 0.0f);
        bhkWindListener::SetWind(0.0f, 0.0f);
    }
    else {
        float fWindSpeed = pSky->GetWindSpeed();
        float fWindAngle = pSky->GetWindAngle();
        BSWindModifier::SetWind(fWindSpeed, fWindAngle);
        bhkWindListener::SetWind(fWindSpeed, fWindAngle);
    }

    fCellAppTime += afTime;
    LockHavokUpdateMT(true);
    if (pInterior) {
        pInterior->UpdateManagedNodes(fCellAppTime, false);
    }
    else {
        pGridCellA->UpdateManagedNodes(fCellAppTime);
    }

    LockHavokUpdateMT(false);
    if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
        TaskQueueInterface::GetInstance()->QueueParallelTaskProcessing();
    }

    FOCollisionListener::Instance()->DealObjectDamage();
    if (iNumHWThreads.UInt() == 1) {
        float fSkyTime = Interface::InDialog() ? appTimer.GetSeconds() * fAnimationMult.Float() : afTime;
        pSky->Update(fSkyTime);
    }

    if (iNumHWThreads.UInt() == 1) {
        UpdateCellMainThread();
    }

    ReferenceProcessLists.PostProcessProjectiles();
}

// The main-thread part of the cell update: the sky, then the temporary nodes at the cell animation time.
void TES::UpdateCellMainThread() {
    float fSkyTime = Interface::InDialog() ? appTimer.GetSeconds() * fAnimationMult.Float() : Main::QFrameAnimTime();
    pSky->Update(fSkyTime);
    NiUpdateData kUpdateData(fCellAppTime, false, false);
    GetTempNodeManager()->Update(kUpdateData);
}

// Takes or releases the Havok update lock, which only matters with more than one hardware thread.
void TES::LockHavokUpdateMT(bool abLock) {
    if (iNumHWThreads.UInt() != 1) {
        if (abLock) {
            HavokUpdateCrit.Enter(nullptr);
        }
        else {
            HavokUpdateCrit.Leave();
        }
    }
}

void TES::DetachGridCells() {
    if (!pDataHandler) {
        return;
    }

    pGridCellA->DetachAll();
    BSShaderManager::SetProjectedShadowMaskTexture(nullptr);
    GridCellArray::SetRecreateCanopyShadowMaskTexture(true);
}

void TES::AddToBuffer(TESObjectCELL* apCell) {
    if (apCell) {
        if (apCell->GetInterior()) {
            AddToInteriorBuffer(apCell);
        }
        else {
            AddToExteriorBuffer(apCell);
        }
    }
}

void TES::AddToExteriorBuffer(TESObjectCELL* apCell) {
    SortExteriorBuffer();
    uint8_t ucAttached = apCell->GetAttached() || apCell->IsAttaching();
    int8_t cFromStart = static_cast<int8_t>(ucAttached);
    if (!pExteriorBuffer[uGridsToLoad.Int() * uGridsToLoad.Int() - 1]) {
        cFromStart = 1;
    }
    TESObjectCELL* pMoveCell = apCell;
    uint32_t uiStart = 0;
    if (!cFromStart) {
        uint32_t uiBufferSize = static_cast<uint32_t>(uExteriorCellBuffer);
        if (uiBufferSize > uGridsToLoad.Int() * uGridsToLoad.Int()) {
            uiStart = uGridsToLoad.Int() * uGridsToLoad.Int();
        }
    }
    for (uint32_t i = uiStart; i < static_cast<uint32_t>(uExteriorCellBuffer); ++i) {
        TESObjectCELL* pBufferCell = pExteriorBuffer[i];
        pExteriorBuffer[i] = pMoveCell;
        if (pBufferCell == apCell) {
            return;
        }
        pMoveCell = pBufferCell;
    }
    if (pMoveCell) {
        pDataHandler->UnloadCell(pMoveCell);
        SetUpdateGridString();
    }
}

// Moves the queued cells that lie inside the loaded grid ahead of the cells outside it in the exterior buffer.
void TES::SortExteriorBuffer() {
    // Only X is compared: the matching Y test would compare iCurrentQueuedY with itself (always equal)
    // rather than with a current grid Y, so it is left out.
    if (iCurrentGridX == iCurrentQueuedX) {
        return;
    }
    uint32_t uiStart = uGridsToLoad.Int() * uGridsToLoad.Int();
    // The grid count is halved as an unsigned value and the half grid is compared as a signed int.
    int32_t iHalfGrid = static_cast<int32_t>(static_cast<uint32_t>(uGridsToLoad.Int()) >> 1);
    uint32_t uiLastOutside = 0;
    for (uint32_t i = uiStart; i < uExteriorCellBuffer.UInt() - 1; ++i) {
        TESObjectCELL* pCell = pExteriorBuffer[i];
        if (!pCell) {
            break;
        }
        int32_t iDataX = pCell->GetDataX();
        int32_t iDataY = pCell->GetDataY();
        int32_t iDeltaX = iCurrentQueuedX - iDataX;
        int32_t iDeltaY = iCurrentQueuedY - iDataY;
        bool bInGrid = false;
        if (iDeltaX >= -iHalfGrid && iDeltaX <= iHalfGrid && iDeltaY >= -iHalfGrid && iDeltaY <= iHalfGrid) {
            bInGrid = true;
        }
        if (bInGrid && uiLastOutside) {
            std::memmove(
                &pExteriorBuffer[uiLastOutside + 1],
                &pExteriorBuffer[uiLastOutside],
                sizeof(TESObjectCELL*) * (i - uiLastOutside)
            );
            pExteriorBuffer[uiLastOutside] = pCell;
            i = uiLastOutside + 1;
            uiLastOutside = 0;
        }
        else {
            uiLastOutside = i;
        }
    }
}

void TES::SetUpdateGridString() {
    bUpdateGridString = true;
}

void TES::ClearDeadCountList() {
    for (DeathCount*& rpItem : DeadCount.ItemsUntilNull()) {
        ourFree(rpItem);
    }
    DeadCount.RemoveAll();
}

// Moves apCell to the front of the interior buffer. A cell not yet buffered pushes the others back; the one pushed
// off the end is unloaded.
void TES::AddToInteriorBuffer(TESObjectCELL* apCell) {
    if (IsInteriorBuffered(apCell)) {
        for (uint32_t uiIndex = 0; uiIndex < static_cast<uint32_t>(uInteriorCellBufferGeneral); ++uiIndex) {
            if (pInteriorBuffer[uiIndex] == apCell) {
                for (; uiIndex != 0; --uiIndex) {
                    pInteriorBuffer[uiIndex] = pInteriorBuffer[uiIndex - 1];
                }

                pInteriorBuffer[0] = apCell;
                break;
            }
        }
    }
    else {
        int32_t iLast = static_cast<int32_t>(static_cast<uint32_t>(uInteriorCellBufferGeneral) - 1);
        for (int32_t iIndex = iLast; iIndex >= 0; --iIndex) {
            if (iIndex == iLast && pInteriorBuffer[iIndex]) {
                pDataHandler->UnloadCell(pInteriorBuffer[iIndex]);
            }

            if (iIndex != 0) {
                pInteriorBuffer[iIndex] = pInteriorBuffer[iIndex - 1];
            }
            else {
                pInteriorBuffer[iIndex] = nullptr;
            }
        }

        pInteriorBuffer[0] = apCell;
    }
}

// Unloads the buffered interiors (with abBufferedonly, all but the current one) whose reference lists are not
// locked; returns how many it unloaded.
uint32_t TES::FreeInteriorBuffer(bool abBufferedonly) {
    uint32_t iFreed = 0;
    for (uint32_t uiIndex = 0; uiIndex < static_cast<uint32_t>(uInteriorCellBufferGeneral); ++uiIndex) {
        if (!pInteriorBuffer[uiIndex]) {
            continue;
        }

        if (abBufferedonly && GetInterior() && pInteriorBuffer[uiIndex] == GetInterior()) {
            continue;
        }

        if (pInteriorBuffer[uiIndex]->IsRefListLocked()) {
            continue;
        }

        pDataHandler->UnloadCell(pInteriorBuffer[uiIndex]);
        pInteriorBuffer[uiIndex] = nullptr;
        ++iFreed;
    }

    return iFreed;
}

bool TES::IsExteriorBuffered(TESObjectCELL* apCell) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(uExteriorCellBuffer); ++i) {
        if (pExteriorBuffer[i] == apCell) {
            return true;
        }
    }
    return false;
}

// Unloads the buffered exteriors (with abBufferedonly, only those not loaded; with apWorldSpace, only its cells)
// whose reference lists are not locked; an empty buffer also unloads the worldspace's distant terrain. Returns how
// many cells it unloaded.
uint32_t TES::FreeExteriorBuffer(bool abBufferedonly, TESWorldSpace* apWorldSpace) {
    uint32_t iFreed = 0;
    for (uint32_t uiIndex = 0; uiIndex < static_cast<uint32_t>(uExteriorCellBuffer); ++uiIndex) {
        if (!pExteriorBuffer[uiIndex]) {
            continue;
        }

        if (abBufferedonly && IsCellLoaded(pExteriorBuffer[uiIndex], true)) {
            continue;
        }

        if (pExteriorBuffer[uiIndex]->IsRefListLocked()) {
            continue;
        }

        if (apWorldSpace && pExteriorBuffer[uiIndex]->GetWorldSpace() != apWorldSpace) {
            continue;
        }

        pDataHandler->UnloadCell(pExteriorBuffer[uiIndex]);
        pExteriorBuffer[uiIndex] = nullptr;
        ++iFreed;
    }

    if (!pExteriorBuffer[0] && GetWorldSpace() && GetWorldSpace()->GetTerrainManager()) {
        GetWorldSpace()->GetTerrainManager()->UnloadAll();
    }

    return iFreed;
}

// Picks the sky mode for apCell: interior, fake exterior, or full sky outdoors and with no cell. A sky with no mode
// is left alone.
void TES::SetAtmosphere(TESObjectCELL* apCell) {
    if (GetSky()->GetMode() == Sky::SM_NONE) {
        return;
    }

    if (apCell) {
        if (apCell->GetInterior()) {
            if (apCell->GetFakeExterior()) {
                GetSky()->SetMode(Sky::SM_FAKEEXTERIOR);
            }
            else {
                GetSky()->SetMode(Sky::SM_INTERIOR);
            }
        }
        else {
            GetSky()->SetMode(Sky::SM_FULL);
        }
    }
    else {
        GetSky()->SetMode(Sky::SM_FULL);
    }
}

// Runs the running quests' scripts, then the scripts of the interior or of the attached grid cells (stopping at the
// first cell whose scripts report true), then the player's, under ProcessLists' update lock. A reentrant call returns
// at once.
void TES::RunScripts() {
    if (bScriptsRunning || !Script::GetProcessScripts()) {
        return;
    }

    bScriptsRunning = true;
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    TESQuest* pQuest = nullptr;
    for (auto* pNode = pDataHandler->GetQuestList(); pNode;) {
        pQuest = static_cast<TESQuest*>(pNode->m_item);
        pNode = pNode->m_pkNext;
        if (pQuest && pQuest->GetEnabled()) {
            pQuest->RunScript();
        }
    }

    if (GetInterior()) {
        GetInterior()->RunScripts(false, false);
    }
    else {
        bool bRan = false;
        for (uint32_t uiX = 0; uiX < uGridsToLoad.UInt(); ++uiX) {
            for (uint32_t uiY = 0; uiY < uGridsToLoad.UInt(); ++uiY) {
                TESObjectCELL* pCell = pGridCellA->Get(uiX, uiY)->pCell;
                if (pCell && pCell->GetAttached() && pCell->RunScripts(false, false)) {
                    bRan = true;
                    break;
                }
            }

            if (bRan) {
                break;
            }
        }
    }

    pPlayer->RunScript();
    ReferenceProcessLists.kUpdateLock.Unlock();
    bScriptsRunning = false;
}

void TES::RunAnimations() {
    Animation::ResetCloneCount();
    if (GetInterior()) {
        GetInterior()->RunAnimations();
    }
    else {
        pGridCellA->RunAnimations();
    }
}

// Clones apObject through arCloneData and gives the clone's geometry its own data; null for no object.
NiAVObject* TES::CreateDeepCopySameTextures(NiAVObject* apObject, BSCloningProcess& arCloneData) {
    NiAVObject* pClone = nullptr;
    if (apObject) {
        pClone = static_cast<NiAVObject*>(apObject->Clone(arCloneData));
        MakeNewGeomData(pClone);
    }

    return pClone;
}

// The grid size and the coordinates are compared unsigned, and the corner is the current grid cell less half the size.
void TES::InitModelsToLoad(MODEL_COUNT_TYPE aeType) {
    int32_t iRefCount = 0;
    int32_t iStartX = iCurrentGridX - (uGridsToLoad.UInt() >> 1);
    int32_t iStartY = iCurrentGridY - (uGridsToLoad.UInt() >> 1);
    for (uint32_t x = 0; x < uGridsToLoad.UInt(); ++x) {
        for (uint32_t y = 0; y < uGridsToLoad.UInt(); ++y) {
            switch (aeType) {
                case MODEL_COUNT_PRELOAD:
                    if (pGridCellA->GetAttached(x, y)) {
                        continue;
                    }
                    break;
                case MODEL_COUNT_NO_GRID_CELL:
                    if (pGridCellA->Get(x, y)) {
                        continue;
                    }
                    break;
            }
            if (GetWorldSpace()) {
                TESObjectCELL* pCell = pWorldSpace->LoadCell(iStartX + x, iStartY + y);
                if (!pCell) {
                    TESWorldSpace* pWorld = GetWorldSpace();
                    pCell = pDataHandler->GetCellFromCellCoord(iStartX + x, iStartY + y, pWorld, true);
                }
                if (pCell && !pCell->IsLoaded()) {
                    iRefCount += pCell->GetReferenceCount();
                }
            }
        }
    }
    if (iRefCount) {
        ShowLoadingMenu(true, pWorldSpace, false);
    }
}

// Shows the loading menu for apLocation (a worldspace, a cell, or a reference; an exterior cell becomes its
// worldspace), or hides it. Showing returns true once a menu was created; a menu already up only takes the new
// worldspace. Hiding closes a scripted menu at once when abScripted and otherwise five seconds after the first
// request. The cell and worldspace the switch works out are not used further; the menu gets apLocation.
bool TES::ShowLoadingMenu(bool abShow, TESForm* apLocation, bool abScripted) {
    if (abShow) {
        if (Interface::GetLoadingMenuVisible() && !LoadingMenu::QResetFlagged()) {
            LoadingMenu::SetWorldspaceAndChangedFlag(static_cast<TESWorldSpace*>(apLocation));
            return false;
        }

        if (CellTestRunning()) {
            return false;
        }

        pSky->StopPlayingAllWeatherSounds();
        BSAudio::QInstance().EndAllSoundsOfType(BSGameSound::SOUND_FLAG_VOICE | BSGameSound::SOUND_FLAG_LOOP);
        TESObjectCELL* pCell = nullptr;
        if (apLocation) {
            switch (apLocation->GetFormType()) {
                case WRLD_ID: break;
                case CELL_ID:
                    pCell = static_cast<TESObjectCELL*>(apLocation);
                    if (pCell && !pCell->GetInterior()) {
                        apLocation = pCell->GetWorldSpace();
                        pCell = nullptr;
                    }
                    break;
                case REFR_ID:
                case ACHR_ID:
                case ACRE_ID:
                case PMIS_ID:
                case PGRE_ID:
                case PBEA_ID:
                case PFLA_ID:
                case PCBE_ID:
                    pCell = static_cast<TESObjectREFR*>(apLocation)->GetParentCell();
                    if (pCell && !pCell->GetInterior()) {
                        pCell = nullptr;
                    }
                    else if (!pCell) {
                        static_cast<TESObjectREFR*>(apLocation)->GetWorldSpace();
                    }
                    break;
                default: break;
            }
        }

        Interface::CreateLoadingMenu(static_cast<TESWorldSpace*>(apLocation), abScripted);
        BSFileCache::QInstance().ResumeFileCaching();
        BSFileCache::QInstance().WaitDuringLoadingScreen();
        BSFileCache::QInstance().PauseFileCaching();
        return true;
    }

    if (!LoadingMenu::IsInScriptMode() || (LoadingMenu::IsInScriptMode() && abScripted)) {
        uiCloseLoadingMenuTime = 0;
        LoadingMenu::SetScriptMode(false);
        Interface::CloseLoadingMenu();
        if (HUDMainMenu::GetMenuMode() != HUDMainMenu::HMM_MOVIE) {
            HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_RUN_AND_GUN);
        }
    }
    else {
        if (uiCloseLoadingMenuTime == 0) {
            uiCloseLoadingMenuTime = BSGetTickCount() + 5000;
        }

        if (BSGetTickCount() > uiCloseLoadingMenuTime) {
            uiCloseLoadingMenuTime = 0;
            LoadingMenu::SetScriptMode(false);
            Interface::CloseLoadingMenu();
            if (HUDMainMenu::GetMenuMode() != HUDMainMenu::HMM_MOVIE) {
                HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_RUN_AND_GUN);
            }
        }
    }

    return false;
}

// Makes apWorldSpace current: optionally frees the buffered cells first, then sets up its encounter zone, impact swap,
// grid, map and water noise. The noise texture is held in a NiPointer. AutoMemContext as in InitializeWater.
void TES::SetWorldSpace(TESWorldSpace* apWorldSpace) {
    if (!apWorldSpace || pWorldSpace == apWorldSpace) {
        return;
    }

    if (bPreemptivelyUnloadCells.Bool()) {
        if (GetInterior()) {
            DetachGridCells();
        }

        int32_t iFreed = 0;
        iFreed = FreeExteriorBuffer(true, pWorldSpace) + iFreed;
        if (!GetInterior()) {
            iFreed = FreeInteriorBuffer(true) + iFreed;
        }

        if (iFreed) {
            CleanUpUnusedTextures(false);
        }
    }

    pWorldSpace = apWorldSpace;
    BGSEncounterZone* pEncounterZone = pWorldSpace->GetEncounterZone();
    if (pEncounterZone) {
        pEncounterZone->DetermineLevel();
    }

    BGSImpactDataSet::ApplyImpactSwap(pWorldSpace->QImpactSwap());
    DetachGridCells();
    InitVars();
    Interface::PreloadWorldMap();
    Interface::SetRootMapSpace(apWorldSpace);
    if (*apWorldSpace->GetWaterNoiseTexturePath()) {
        // AutoMemContext as in InitializeWater; the texture pointer is released before the context.
        alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
        AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
        ::new (aucContext) AutoMemContext(
            MEM_CONTEXT_WATER,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp",
            5969
        );
        {
            NiPointer<NiSourceTexture> spNoiseTexture(nullptr);
            pInstance->CreateTextureImage(apWorldSpace->GetWaterNoiseTexturePath(), spNoiseTexture, true, true);
            pInstance->GetWaterSystem()->SetWaterNoiseTexture(spNoiseTexture);
        }
        pContext->~AutoMemContext();
    }
    else {
        pInstance->GetWaterSystem()->SetWaterNoiseTexture(nullptr);
    }

    BSShaderManager::SetUseNoiseMap(!apWorldSpace->GetNoLODNoise());
}

void TES::ReloadGridCells(const NiPoint3* apPos, bool abFullreload) {
    if (!pDataHandler || GetInterior()) {
        return;
    }
    if (bReloading) {
        return;
    }
    bReloading = true;
    if (abFullreload) {
        pGridCellA->DetachAll();
        GridCellArray::SetRecreateCanopyShadowMaskTexture(true);
        BSShaderManager::SetProjectedShadowMaskTexture(nullptr);
        pWorldSpace->GetTerrainManager()->UnloadAll();
        // Takes the player's position before it fetches the terrain manager again.
        const NiPoint3* pPlayerPos = &pPlayer->GetLocationOnReference();
        pWorldSpace->GetTerrainManager()->Update(*pPlayerPos, BGSTerrainManager::UPDATE_ALL);
        pGridCellA->SetCenter(iCurrentGridX, iCurrentGridY);
        FreeExteriorBuffer(false, nullptr);
        CleanUpUnusedTextures(false);
        InitModelsToLoad(MODEL_COUNT_PRELOAD);
        GridArrayLoad();
        pIOManager->LoadQueuedPriority(IO_TASK_PRIORITY_LOW);
        NiPoint3 kPos;
        ::new (&kPos) NiPoint3(apPos->x, apPos->y, 0.0f);
        GetLandHeight(*apPos, kPos.z);
        TallGrassShaderProperty::SetUseBackgroundLoading(false);
        TESTerrainLODManager::UpdateGrass(kPos, NiPoint3::UNIT_Y, 1.0f);
        TallGrassShaderProperty::SetUseBackgroundLoading(true);
        if (!CellTestRunning()) {
            pFaderManager->CreateFader(FADER_TO_BLACK, fFadeToBlackFadeSeconds.Float(), false);
        }
        CreateFurnitureList();
        ReferenceProcessLists.PlaceActorsinBedsOrChairs();
        ReferenceProcessLists.AnimateDeadActors();
    }
    else {
        pGridCellA->SetCenter(iCurrentGridX, iCurrentGridY);
        InitModelsToLoad(MODEL_COUNT_NO_GRID_CELL);
        GridArrayLoad();
    }
    RunScripts();
    NiUpdateData kUpdateData(0.0f, false, false);
    pObjLandRoot->Update(kUpdateData);
    CleanUpUnusedTextures(false);
    pDataHandler->CloseAllTES();
    bReloading = false;
}

void TES::GridArrayLoad() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_CELLS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp",
        2262
    );
    int32_t iCellX = iCurrentGridX - (uGridsToLoad.UInt() >> 1);
    int32_t iCellY = iCurrentGridY - (uGridsToLoad.UInt() >> 1);
    bool bDelayedLandLoad = Interface::GetLoadingMenuVisible();
    bool bShowLoadingBar = Interface::GetLoadingMenuVisible();
    pSaveLoadGameOLD->SetLoading(true);
    pSaveLoadGame->SetDeferInitForms(true);
    if (TESObjectCELL::ExteriorWorld()) {
        TESObjectCELL::ExteriorWorld()->StartBatchingAdds();
    }
    for (int32_t x = 0; x < uGridsToLoad.UInt(); ++x) {
        for (int32_t y = 0; y < uGridsToLoad.UInt(); ++y) {
            LoadGridCell(x, y, x + iCellX, y + iCellY);
            TESObjectCELL* pCell = pDataHandler->GetCellFromCellCoord(x + iCellX, y + iCellY, pWorldSpace, false);
            if (TES::pInstance->IsBuffered(pCell)) {
                AddToBuffer(pCell);
            }
            if (bShowLoadingBar || (pSaveLoadGameOLD->GetLoading() && pSaveLoadGameOLD->GetLoading())) {
                if (pSaveLoadGameOLD->GetLoading() && pSaveLoadGameOLD->GetLoading()) {
                    pSaveLoadGameOLD->StartLoadingSequence();
                }
            }
        }
    }
    if (TESObjectCELL::ExteriorWorld()) {
        TESObjectCELL::ExteriorWorld()->StopBatchingAdds(false);
    }
    pSaveLoadGameOLD->SetLoading(false);
    if (pSaveLoadGameOLD->GetInitArrayExists()) {
        bool bRestore = pSaveLoadGameOLD->GetLoading();
        bool bRestore2 = pSaveLoadGameOLD->GetLoading();
        pSaveLoadGameOLD->SetLoading(true);
        pSaveLoadGameOLD->SetLoading(true);
        pSaveLoadGameOLD->InitForms(nullptr, nullptr, false);
        pSaveLoadGameOLD->LoadQueuedData(false);
        pSaveLoadGameOLD->SetLoading(bRestore2);
        pSaveLoadGameOLD->SetLoading(bRestore);
    }
    pSaveLoadGame->SetDeferInitForms(false);
    if (!pSaveLoadGame->GetSaveGameLoading()) {
        pSaveLoadGame->InitForms(false);
    }
    if (TESObjectCELL::ExteriorWorld()) {
        TESObjectCELL::ExteriorWorld()->StartBatchingAdds();
    }
    // The counters are compared with the setting unsigned.
    for (int32_t x = 0; static_cast<uint32_t>(x) < uGridsToLoad.UInt(); ++x) {
        for (int32_t y = 0; static_cast<uint32_t>(y) < uGridsToLoad.UInt(); ++y) {
            if (pWorldSpace) {
                TESObjectCELL* pCell = pDataHandler->GetCellFromCellCoord(x + iCellX, y + iCellY, pWorldSpace, false);
                if (pCell) {
                    if (pCell->GetState() == TESObjectCELL::CS_ATTACHING) {
                        pCell->AttachModels(!bDelayedLandLoad);
                        AddToBuffer(pCell);
                        NiNode* pRoot = pCell->Load3D();
                        pRoot->UpdateProperties();
                        if (!pRoot->GetChildCount()) {
                            NiUpdateData UpdateData(0.0f, false, false);
                            pRoot->Update(UpdateData);
                        }
                    }
                    listLastLoadedExteriors.AddHead(pCell);
                }
            }
        }
    }
    if (TESObjectCELL::ExteriorWorld()) {
        TESObjectCELL::ExteriorWorld()->StopBatchingAdds(false);
    }
    BSShaderManager::SetProjectedShadowMaskTexture(nullptr);
    GridCellArray::SetRecreateCanopyShadowMaskTexture(true);
}

void TES::LoadInterior(TESObjectCELL* apNewInterior, const NiPoint3* apPos) {
    if (!CellTestRunning()) {
        appMain->RenderMenuBackground();
    }
    TESObjectCELL::ExteriorWorld()->SetEnabled(false);
    TESObjectCELL::ExteriorWorld()->SetVisualDebugger(false);
    TESObjectCELL* pint = pInteriorCell;
    if (pint == apNewInterior) {
        UpdateCurrentGridCell(apPos, false);
    }
    else {
        if (BSShaderManager::GetAccumulator()) {
            BSShaderManager::GetAccumulator()->ClearRenderPasses();
        }
        if (bPreemptivelyUnloadCells.Bool()) {
            if (pint) {
                DetachGridCells();
                if (FreeExteriorBuffer(true, nullptr)) {
                    CleanUpUnusedTextures(false);
                }
            }
            else if (FreeExteriorBuffer(true, nullptr)) {
                CleanUpUnusedTextures(false);
            }
        }
        if (!pSaveLoadGame->GetSaveGameLoading() &&
            !Interface::IsMenuIDVisible(Interface::eLoadingMenu, 0) &&
            !pMoviePlayer->GetPlayingSequence()) {
            appMain->Swap();
        }
        BGSEncounterZone* pZone = apNewInterior->GetEncounterZone();
        if (pZone) {
            pZone->DetermineLevel();
        }
        BGSImpactDataSet::ApplyImpactSwap(apNewInterior->GetImpactSwap());
        BSShaderManager::GetShadowSceneNode(0)->SetPortalGraph(nullptr);
        BSShaderManager::GetShadowSceneNode(0)->ProcessQueuedLights();
        BSShaderManager::GetShadowSceneNode(0)->RemoveAllLights();
        bool bWasInterior = false;
        if (pint) {
            bWasInterior = true;
            pint->Detach(true);
            BSShaderManager::ClearAllShaderPassStages();
            NiAVObject* pPlayer3D = pPlayer->Get3D(false);
            BSShaderManager::GetShadowSceneNode(0)->AddShadowLight(pPlayer3D);
        }
        if (!IsInteriorBuffered(apNewInterior)) {
            TESObjectCELL* pOldInt = GetInteriorBufferCellByIndex(TES::uInteriorCellBufferGeneral.UInt() - 1);
            if (pOldInt) {
                RemoveCellFromInteriorBuffer(pOldInt);
            }
        }
        pint = apNewInterior;
        SetInterior(apNewInterior);
        BSShaderManager::SetInterior(true);
        if (apNewInterior) {
            if (iSaveGridX == 0x7FFFFFFF && !bWasInterior) {
                iSaveGridX = this->GetCurrentGridX();
                iSaveGridY = this->GetCurrentGridY();
                CullGridCells(true, nullptr);
            }
            iCurrentGridX = FloatToInt(apPos->x) >> 12;
            iCurrentGridY = FloatToInt(apPos->y) >> 12;
            pint->LoadAllTempData();
            bool bLoadingSaveGame = pSaveLoadGame->GetSaveGameLoading();
            if (pint->GetReferenceCount() && !pint->IsLoaded()) {
                ShowLoadingMenu(true, pint, false);
            }

            pint->AttachToWorld(pObjRoot);
            bhkWorld* pw = pint->GetbhkWorld();
            if (spLoadedAreaBound.m_pObject) {
                spLoadedAreaBound->Clear();
            }
            if (pw) {
                pw->StartBatchingAdds();
            }
            pint->AttachModels(true);
            if (pModelLoader->IsFileQueuedForCell(pint)) {
                ShowLoadingMenu(true, pint, false);
            }
            SetAtmosphere(pint);
            if (!bLoadingSaveGame) {
                pIOManager->LoadQueuedPriority(IO_TASK_PRIORITY_VERY_HIGH);
            }
            if (pw) {
                pw->StopBatchingAdds(false);
            }
            TES::pInstance->ActivateAllSubmergedReferencesIslands();
            if (!CellTestRunning() && !bLoadingSaveGame && pPlayer->Get3D()) {
                pFaderManager->CreateFader(FADER_TO_BLACK, 0.0f, false);
            }

            if (!bLoadingSaveGame) {
                ReferenceProcessLists.HandleInvalidPositions();
                CreateFurnitureList();
                ReferenceProcessLists.PlaceActorsinBedsOrChairs();
                ReferenceProcessLists.AnimateDeadActors();
            }
            CleanUpUnusedTextures(false);
            pDataHandler->CloseAllTES();
            NiNode* pRoot = pint->Load3D();
            pRoot->UpdateProperties();
            if (!pRoot->GetChildCount()) {
                NiUpdateData UpdateData(0.0f, false, false);
                pRoot->Update(UpdateData);
            }
            pint->AttachLights(true);
            AddToInteriorBuffer(apNewInterior);
        }
        if (!bWasInterior) {
            BSShaderAccumulator* pAccum = BSShaderManager::GetAccumulator();
            if (pAccum) {
                pAccum->ClearAllQueries();
            }
            TallGrassShaderProperty::ClearStaticMaps();
            pGridCellA->ClearCanopyShadowMaskTexture();
        }
        if (CellTestRunning()) {
            ShowLoadingMenu(false, nullptr, false);
        }
        CleanUpUnusedTextures(false);
        Interface::SetEnemyActor(nullptr);
        if (*pint->GetWaterNoiseTextureName()) {
            AutoMemContext tempMemContext(
                MEM_CONTEXT_WATER,
                true,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp",
                3188
            );
            NiPointer<NiSourceTexture> spSourceTexture(static_cast<NiSourceTexture*>(nullptr));
            const char* pTextureName = pint->GetWaterNoiseTextureName();
            TES::pInstance->CreateTextureImage(pTextureName, spSourceTexture, true, false);
            TES::pInstance->GetWaterSystem()->SetWaterNoiseTexture(spSourceTexture.m_pObject);
        }
        else {
            TES::pInstance->GetWaterSystem()->SetWaterNoiseTexture(nullptr);
        }
        if (!pSaveLoadGame->GetSaveGameLoading()) {
            NavMeshObstacleManager::GetInstance()->ForceUpdateAllObstacles();
        }
    }
}

void TES::LeaveInterior(NiPoint3* apPos) {
    NiPoint2 kCenter;
    TESWorldSpace* pWorld = pWorldSpace;
    if (pWorld) {
        pWorld->GetTerrainManager()->Update(*apPos, BGSTerrainManager::UPDATE_ALL);
        pWorld->GetTerrainManager()->UpdateMorphParameters(*apPos);
    }
    BSShaderManager::SetInterior(false);
    if (pWorldSpace) {
        BGSImpactDataSet::ApplyImpactSwap(pWorldSpace->pImpactSwap);
    }
    DistantLODShaderProperty::SetDistantLODCulled(false);
    TESObjectCELL::ExteriorWorld()->SetEnabled(true);
    TESObjectCELL* pint = pInteriorCell;
    if (pint) {
        BSPortalGraph* pPortalGraph = BSShaderManager::GetShadowSceneNode(0)->GetPortalGraph();
        if (pPortalGraph == pint->GetPortalGraph()) {
            BSShaderManager::GetShadowSceneNode(0)->SetPortalGraph(nullptr);
        }
        pint->Detach(true);
        if (IsInteriorBuffered(pint)) {
            pint->ChangeProcessLevel(true);
        }
        else {
            pDataHandler->UnloadCell(pint);
        }
        SetInterior(nullptr);
        if (!pSaveLoadGame->GetSaveGameLoading()) {
            if (iSaveGridX == 0x7FFFFFFF) {
                if (!GetLandGeometry(*apPos)) {
                    iCurrentGridX = 0x7FFFFFFF;
                    iCurrentGridY = 0x7FFFFFFF;
                    iCurrentQueuedX = iCurrentGridX;
                    iCurrentQueuedY = iCurrentGridY;
                }
            }
            else {
                if (*pExteriorBuffer) {
                    iCurrentGridX = iSaveGridX;
                    iCurrentGridY = iSaveGridY;
                    CullGridCells(false, apPos);
                }
                else {
                    iCurrentGridX = 0x7FFFFFFF;
                    iCurrentGridY = 0x7FFFFFFF;
                    iCurrentQueuedX = iCurrentGridX;
                    iCurrentQueuedY = iCurrentGridY;
                }
                iSaveGridX = 0x7FFFFFFF;
                iSaveGridY = 0x7FFFFFFF;
            }
        }
        BSShaderManager::GetShadowSceneNode(0)->RemoveAllLights();
        NiAVObject* pPlayer3D = pPlayer->Get3D(false);
        BSShaderManager::GetShadowSceneNode(0)->AddShadowLight(pPlayer3D);
    }
    if (pObjLandRoot) {
        NiNode* pLODRoot = pObjLandRoot->m_pkParent;
        if (pLODRoot) {
            pLODRoot->SetAppCulled(false);
        }
        BGSTerrainManager::EnableLODWaterProcessing(true);
    }
    TESObjectCELL::ExteriorWorld();
    bool bHavokDebug = bhkWorld::IsHavokDebug();
    TESObjectCELL::ExteriorWorld()->SetVisualDebugger(bHavokDebug);
    bool bLoadingSaveGame = pSaveLoadGame->GetSaveGameLoading();
    bool bLoadingSaveGameOLD = pSaveLoadGameOLD->GetLoading();
    UpdateCurrentGridCell(apPos, !bLoadingSaveGameOLD);
    TESObjectCELL* pCurrentCell = GetGridCellCell(iCurrentGridX, iCurrentGridY);
    if (pModelLoader->IsFileQueuedForCell(pCurrentCell)) {
        ShowLoadingMenu(true, pCurrentCell, false);
    }
    if (TESObjectCELL::ExteriorWorld()) {
        TESObjectCELL::ExteriorWorld()->StartBatchingAdds();
    }
    if (!bLoadingSaveGame) {
        pIOManager->LoadQueuedPriority(IO_TASK_PRIORITY_LOW);
    }
    pGridCellA->AttachAllLand3D();
    HideFarLandNormalMaps();
    HideUnneededLandPasses();
    if (TESObjectCELL::ExteriorWorld()) {
        TESObjectCELL::ExteriorWorld()->StopBatchingAdds(false);
    }
    ActivateAllSubmergedReferencesIslands();
    NiPoint3 kGrassPos(apPos->x, apPos->y, 0.0f);
    GetLandHeight(*apPos, kGrassPos.z);
    TallGrassShaderProperty::SetUseBackgroundLoading(false);
    TESTerrainLODManager::UpdateGrass(kGrassPos, NiPoint3::UNIT_Y, 1.0f);
    TallGrassShaderProperty::SetUseBackgroundLoading(true);
    if (!CellTestRunning() && !bLoadingSaveGame) {
        pFaderManager->CreateFader(FADER_TO_BLACK, fFadeToBlackFadeSeconds.Float(), true);
    }
    if (!bLoadingSaveGame) {
        ReferenceProcessLists.HandleInvalidPositions();
        CreateFurnitureList();
        ReferenceProcessLists.PlaceActorsinBedsOrChairs();
        ReferenceProcessLists.AnimateDeadActors();
    }
    if (pint && !pint->GetFakeExterior()) {
        pSky->ActivateWeatherSounds(pSky->pCurrentWeather);
        pSky->ActivateWeatherSounds(pSky->pLastWeather);
    }

    UpdateGridLights();
    SetAtmosphere(nullptr);
    pObjRoot->UpdateProperties();
    Interface::SetEnemyActor(nullptr);
    if (CellTestRunning()) {
        ShowLoadingMenu(false, nullptr, false);
    }
    CleanUpUnusedTextures(false);
    if (pWorldSpace && *pWorldSpace->GetWaterNoiseTexturePath()) {
        AutoMemContext tempMemContext(
            MEM_CONTEXT_WATER,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp",
            3434
        );
        NiPointer<NiSourceTexture> spSourceTexture(nullptr);
        const char* pTexturePath = pWorldSpace->GetWaterNoiseTexturePath();
        TES::pInstance->CreateTextureImage(pTexturePath, spSourceTexture, true, false);
        TES::pInstance->GetWaterSystem()->SetWaterNoiseTexture(spSourceTexture.m_pObject);
    }
    else {
        TES::pInstance->GetWaterSystem()->SetWaterNoiseTexture(nullptr);
    }
    TESWaterSystem::bForceWaterUpdate = true;
    float fCenterY = (static_cast<double>(iCurrentGridY) + 0.5) * 4096.0;
    float fCenterX = (static_cast<double>(iCurrentGridX) + 0.5) * 4096.0;
    ::new (&kCenter) NiPoint2(fCenterX, fCenterY);
    float fX = kCenter.x;
    float fY = kCenter.y;
    BSShaderManager::kGridArrayCenter.x = fX;
    BSShaderManager::kGridArrayCenter.y = fY;
    BSShaderManager::kOldGridArrayCenter.x = fX;
    BSShaderManager::kOldGridArrayCenter.y = fY;
    BSShaderManager::fGriddArrayLerpStart = BSShaderManager::GetTimer(false);
    TES::bLODFadeInProgress = false;
    BSShaderManager::bLODFadeInProgress = false;
    BSShaderManager::bInLODWorld = pWorldSpace->GetTerrainManager()->HasRootNode();
    if (!pSaveLoadGame->GetSaveGameLoading()) {
        NavMeshObstacleManager::GetInstance()->ForceUpdateAllObstacles();
    }
}

void TES::SetupTemporaryParticleCache() {
    if (!bdoOnce) {
        AutoMemContext tempMemContext(
            MEM_CONTEXT_TES_ACTOR,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp",
            0x1C66
        );
        bdoOnce = true;
        if (*sDismemberParticleDefault.String()) {
            const char* pFileName = sDismemberParticleDefault.String();
            NiNode* pParticles = pModelLoader->LoadFile(pFileName, LOD_MULT_NONE, true, false, false, false);
            AddTemporaryParticleObjectToCache(pParticles);
        }
        if (*sDismemberRobotParticleDefault.String()) {
            const char* pFileName = sDismemberRobotParticleDefault.String();
            NiNode* pParticles = pModelLoader->LoadFile(pFileName, LOD_MULT_NONE, true, false, false, false);
            AddTemporaryParticleObjectToCache(pParticles);
        }
        if (*sSplashParticles.String()) {
            const char* pFileName = sSplashParticles.String();
            NiNode* pParticles = pModelLoader->LoadFile(pFileName, LOD_MULT_NONE, true, false, false, false);
            AddTemporaryParticleObjectToCache(pParticles);
        }
        if (*sBloodParticleMeleeDefault.String()) {
            const char* pFileName = sBloodParticleMeleeDefault.String();
            NiNode* pParticles = pModelLoader->LoadFile(pFileName, LOD_MULT_NONE, true, false, false, false);
            AddTemporaryParticleObjectToCache(pParticles);
        }
        if (*sImpactParticleWoodDefault.String()) {
            const char* pFileName = sImpactParticleWoodDefault.String();
            NiNode* pParticles = pModelLoader->LoadFile(pFileName, LOD_MULT_NONE, true, false, false, false);
            AddTemporaryParticleObjectToCache(pParticles);
        }
        if (*sImpactParticleMetalDefault.String()) {
            const char* pFileName = sImpactParticleMetalDefault.String();
            NiNode* pParticles = pModelLoader->LoadFile(pFileName, LOD_MULT_NONE, true, false, false, false);
            AddTemporaryParticleObjectToCache(pParticles);
        }
        if (*sImpactParticleConcreteDefault.String()) {
            const char* pFileName = sImpactParticleConcreteDefault.String();
            NiNode* pParticles = pModelLoader->LoadFile(pFileName, LOD_MULT_NONE, true, false, false, false);
            AddTemporaryParticleObjectToCache(pParticles);
        }
        if (*sBloodParticleDefault.String()) {
            const char* pFileName = sBloodParticleDefault.String();
            NiNode* pParticles = pModelLoader->LoadFile(pFileName, LOD_MULT_NONE, true, false, false, false);
            AddTemporaryParticleObjectToCache(pParticles);
        }
    }
}

uint32_t TES::QPlaceableWaterCount() {
    return static_cast<uint32_t>(iPlaceableWaterCount);
}

NiNode* TES::GetRoot() {
    return pObjRoot;
}

// Collects the beds and chairs of the interior, or of every grid cell, into ListofBedsAndChairs. The grid cells are
// not null-checked.
void TES::CreateFurnitureList() {
    if (pInteriorCell) {
        pInteriorCell->AddFurnitureToList(&ListofBedsAndChairs);
        return;
    }

    for (uint32_t uiX = 0; uiX < uGridsToLoad.UInt(); ++uiX) {
        for (uint32_t uiY = 0; uiY < uGridsToLoad.UInt(); ++uiY) {
            TESObjectCELL* pCell = pGridCellA->Get(uiX, uiY)->pCell;
            pCell->AddFurnitureToList(&ListofBedsAndChairs);
        }
    }
}

// Loads each master particle system addon node now (registering it with the particle system manager, then dropping the
// load's reference) and queues every other addon node's model under spPreloadedAddonNodes. AutoMemContext as in
// InitializeWater.
void TES::PreloadAddonNodes() {
    if (spPreloadedAddonNodes) {
        return;
    }

    // AutoMemContext as in InitializeWater.
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_EFFECTS_OVERHEAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp",
        7054
    );
    spPreloadedAddonNodes = new QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
    uint32_t uiAddonNodeCount = pDataHandler->GetAddonNodeCount();
    for (uint32_t i = 0; i < uiAddonNodeCount; ++i) {
        BGSAddonNode* pAddonNode = pDataHandler->GetAddonNode(i);
        if (!pAddonNode) {
            continue;
        }

        if (pAddonNode->IsMasterParticleSystem()) {
            TESModel* pModel = pAddonNode;
            NiNode* pav = pModelLoader->LoadFile(pModel->GetModel(), LOD_MULT_NONE, true, false, false, false);
            if (pav) {
                BSMasterParticleSystem* pMasterPSys = static_cast<BSMasterParticleSystem*>(pav);
                pMasterPSys->SetEditorIndex(pAddonNode->GetIndex());
                pMasterPSys->SetIndex(pAddonNode->GetMasterParticleSystemIndex());
                pMasterPSys->SetMaxParticlesPerFrame(pAddonNode->GetMasterParticleCap());
                Main::GetMasterParticleSystemManager()->AddMasterParticleSystem(pMasterPSys);
            }

            pModelLoader->ReleaseModel(pModel->GetModel(), true, 1);
        }
        else {
            pModelLoader->QueueModel(
                pAddonNode,
                IO_TASK_PRIORITY_VERY_LOW,
                spPreloadedAddonNodes,
                LOD_MULT_NONE,
                true,
                false,
                false
            );
        }
    }
    pContext->~AutoMemContext();
}

// Queues the files of every body part data form and of the fists under spPreloadedForms. AutoMemContext as in
// InitializeWater.
void TES::PreloadForms() {
    if (spPreloadedForms) {
        return;
    }

    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_EFFECTS_OVERHEAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp",
        7120
    );
    spPreloadedForms = new QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
    BSSimpleList<BGSBodyPartData*>* pIter = pDataHandler->GetPartDataList();
    while (pIter && !pIter->IsEmpty()) {
        BGSBodyPartData* pPartData = static_cast<BGSBodyPartData*>(pIter->GetItem());
        pIter = pIter->GetNext();
        if (pPartData) {
            BGSPreloadable* pPreloadable = pPartData;
            pPreloadable->QueueFiles(IO_TASK_PRIORITY_VERY_LOW, spPreloadedForms);
        }
    }

    if (pFists) {
        pFists->QueueFiles(IO_TASK_PRIORITY_VERY_LOW, spPreloadedForms, false);
    }
    pContext->~AutoMemContext();
}

void TES::PreloadDefaultModels() {
    char path[260];
    AutoMemContext tempMemContext(
        MEM_CONTEXT_EFFECTS_OVERHEAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp",
        7152
    );
    if (*sRagdollDataDefault.String()) {
        const char* pFileName = sRagdollDataDefault.String();
        pModelLoader->QueueModel(pFileName, IO_TASK_PRIORITY_VERY_LOW, nullptr, LOD_MULT_NONE, true, true, false);
    }
    if (*sBloodParticleDefault.String()) {
        const char* pFileName = sBloodParticleDefault.String();
        pModelLoader->QueueModel(pFileName, IO_TASK_PRIORITY_VERY_LOW, nullptr, LOD_MULT_NONE, true, true, false);
    }
    if (*sSplashParticles.String()) {
        const char* pFileName = sSplashParticles.String();
        pModelLoader->QueueModel(pFileName, IO_TASK_PRIORITY_VERY_LOW, nullptr, LOD_MULT_NONE, true, true, false);
    }
    if (*sExplosionSplashParticles.String()) {
        const char* pFileName = sExplosionSplashParticles.String();
        pModelLoader->QueueModel(pFileName, IO_TASK_PRIORITY_VERY_LOW, nullptr, LOD_MULT_NONE, true, true, false);
    }
    if (*sBloodTextureDefault.String()) {
        BSsprintf(path, 260, "%s\\%s\\%s", "Data", "Textures", sBloodTextureDefault.String());
        CreateTextureImage(path, BloodDecalPreload1, false, false);
    }
}

// Releases the four default particle models the settings name (each setting is queried twice), then
// the blood-decal preload texture.
void TES::UnloadDefaultModels() {
    if (*sRagdollDataDefault.String()) {
        pModelLoader->ReleaseFile(sRagdollDataDefault.String());
    }
    if (*sBloodParticleDefault.String()) {
        pModelLoader->ReleaseFile(sBloodParticleDefault.String());
    }
    if (*sSplashParticles.String()) {
        pModelLoader->ReleaseFile(sSplashParticles.String());
    }
    if (*sExplosionSplashParticles.String()) {
        pModelLoader->ReleaseFile(sExplosionSplashParticles.String());
    }
    BloodDecalPreload1 = nullptr;
}

// Unless bUsePerWorldSpaceWaterNoise is set, loads each water form's noise texture and hands it to the form. The water
// system getter's result is unused, and the list's items are not null-checked. The AutoMemContext
// constructor's result is ignored.
void TES::PreloadPlaceableWaterNoiseTextures() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (
        aucContext
    ) AutoMemContext(MEM_CONTEXT_WATER, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 7207);
    GetWaterSystem();
    if (TESWaterSystem::bUsePerWorldSpaceWaterNoise.Bool()) {
        pContext->~AutoMemContext();
        return;
    }

    for (BSSimpleList<TESWaterForm*>* pIter = pDataHandler->GetWaterList(); pIter && !pIter->IsEmpty();
         pIter = pIter->GetNext()) {
        TESWaterForm* pWater = static_cast<TESWaterForm*>(pIter->GetItem());
        if (pWater->GetNoiseTextureNameLength()) {
            NiPointer<NiSourceTexture> spTexture(nullptr);
            pInstance->CreateTextureImage(pWater->GetNoiseTextureName(), spTexture, true, false);
            pWater->SetNoiseTexture(spTexture);
        }
    }
    pContext->~AutoMemContext();
}

// Caches apObject with two clones of it at the head of pParticleCacheHead, unless the list already holds it. The
// clones' array is built as new[] of three NiPointers. The null allocation is not checked for.
void TES::AddTemporaryParticleObjectToCache(NiAVObject* apObject) {
    if (!apObject) {
        return;
    }

    for (ParticleObjectCache* pCache = pParticleCacheHead; pCache; pCache = pCache->QNext()) {
        if (pCache->spModel == apObject) {
            return;
        }
    }

    ParticleObjectCache* pNew = new ParticleObjectCache;
    pNew->spModel = apObject;
    pNew->SetNext(pParticleCacheHead);
    pParticleCacheHead = pNew;

    pNew->pspClones = new NiPointer<NiAVObject>[3];
    pNew->pspClones[0] = apObject;
    for (int32_t i = 1; i < 3; ++i) {
        pNew->pspClones[i] = static_cast<NiAVObject*>(apObject->Clone());
    }
}

// Activates the Havok islands of the submerged references of the interior, or of every loaded grid cell.
void TES::ActivateAllSubmergedReferencesIslands() {
    if (pInteriorCell) {
        pInteriorCell->ActivateAllSubmergedReferencesIslands();
        return;
    }

    for (uint32_t uiX = 0; uiX < uGridsToLoad.UInt(); ++uiX) {
        for (uint32_t uiY = 0; uiY < uGridsToLoad.UInt(); ++uiY) {
            TESObjectCELL* pCell = pGridCellA->Get(uiX, uiY)->pCell;
            if (pCell) {
                pCell->ActivateAllSubmergedReferencesIslands();
            }
        }
    }
}

// Culls the multibound nodes of the loaded cells, the grass, the placeable and LOD water against apCamera, then walks
// the rooms and portals from the room holding the frustum's far end. Each water loop takes the next position, then
// checks the current one again before using it. UpdateRoomAndPortalVisibility takes the culler's camera and planes as
// const in the game; this declaration drops the const.
void TES::UpdateMultiBoundVisibility(NiCamera* apCamera) {
    TimerUpdateMultibound.Start();
    alignas(NiCullingProcess) uint8_t aCullerStorage[sizeof(NiCullingProcess)];
    NiCullingProcess& kCuller = *::new (aCullerStorage) NiCullingProcess(nullptr);
    kCuller.SetCamera(apCamera);
    kCuller.SetFrustum(apCamera->GetViewFrustum());
    NiPoint3 kCameraPos = apCamera->GetWorldTranslate();
    ShadowSceneNode* pSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
    if (pInteriorCell) {
        NiNode* pMultiboundRoot = static_cast<NiNode*>(pInteriorCell->GetChildNode(TESObjectCELL::CN_MULTIBOUND));
        if (pMultiboundRoot) {
            for (uint32_t i = 0; i < pMultiboundRoot->GetArrayCount(); ++i) {
                BSMultiBoundNode* pMBNode = static_cast<BSMultiBoundNode*>(pMultiboundRoot->GetAt(i));
                if (pMBNode && !pMBNode->GetMultiBoundRoom()) {
                    pSceneNode->UpdateMultiboundVisibility(pMBNode, kCameraPos, kCuller);
                }
            }
        }
    }
    else {
        BGSTerrainManager* pTerrainMgr = GetWorldSpace() ? GetWorldSpace()->GetTerrainManager() : nullptr;
        if (pTerrainMgr) {
            pTerrainMgr->UpdateMultiBoundVisibility(apCamera);
        }

        for (uint32_t x = 0; x < uGridsToLoad.UInt(); ++x) {
            for (uint32_t y = 0; y < uGridsToLoad.UInt(); ++y) {
                TESObjectCELL* pCell = pGridCellA->Get(x, y)->pCell;
                if (!pCell) {
                    continue;
                }

                NiNode* pMultiboundRoot = static_cast<NiNode*>(pCell->GetChildNode(TESObjectCELL::CN_MULTIBOUND));
                if (pMultiboundRoot) {
                    for (uint32_t j = 0; j < pMultiboundRoot->GetArrayCount(); ++j) {
                        NiAVObject* pMBNode = pMultiboundRoot->GetAt(j);
                        if (pMBNode) {
                            pSceneNode->UpdateMultiboundVisibility(
                                static_cast<BSMultiBoundNode*>(pMBNode),
                                kCameraPos,
                                kCuller
                            );
                        }
                    }
                }

                // The land node is not null-checked.
                pMultiboundRoot = static_cast<NiNode*>(pCell->GetChildNode(TESObjectCELL::CN_LAND));
                for (uint32_t k = 0; k < pMultiboundRoot->GetArrayCount(); ++k) {
                    NiAVObject* pMBNode = pMultiboundRoot->GetAt(k);
                    if (pMBNode) {
                        pSceneNode
                            ->UpdateMultiboundVisibility(static_cast<BSMultiBoundNode*>(pMBNode), kCameraPos, kCuller);
                    }
                }
            }
        }
    }

    NiAVObject* pPotentialGrassRoot = pSceneNode->GetAtChecked(6);
    NiNode* pGrassNode = pPotentialGrassRoot ? pPotentialGrassRoot->IsNode() : nullptr;
    if (pGrassNode) {
        for (uint32_t m = 0; m < pGrassNode->GetArrayCount(); ++m) {
            NiAVObject* pGrass = pGrassNode->GetAt(m);
            if (pGrass) {
                pSceneNode->UpdateMultiboundVisibility(static_cast<BSMultiBoundNode*>(pGrass), kCameraPos, kCuller);
            }
        }
    }

    if (GetWaterSystem()) {
        PlaceableWaterGroup* pGroup = nullptr;
        NiTListItem<PlaceableWaterGroup*>* kGroupIter = GetWaterSystem()->GetPlaceableWaterGroupList()->GetHeadPos();
        while (kGroupIter) {
            NiTListItem<PlaceableWaterGroup*>* kNextGroup = GetWaterSystem()->GetPlaceableWaterGroupList()->GetNextPos(
                kGroupIter
            );
            if (kGroupIter) {
                pGroup = GetWaterSystem()->GetPlaceableWaterGroupList()->Get(kGroupIter);
                NiTListItem<TESObjectREFR*>* kIter = pGroup->PlaceableWaterList.GetHeadPos();
                while (kIter) {
                    NiTListItem<TESObjectREFR*>* kNext = pGroup->PlaceableWaterList.GetNextPos(kIter);
                    if (kIter) {
                        TESObjectREFR* pPlane = pGroup->PlaceableWaterList.Get(kIter);
                        pPlane->GetObjectReference();
                        NiNode* pWaterNode = pPlane->Get3D();
                        if (NiObject::IsExactKindOf(&BSMultiBoundNode::ms_RTTI, pWaterNode->GetAt(0))) {
                            pSceneNode->UpdateMultiboundVisibility(
                                static_cast<BSMultiBoundNode*>(pWaterNode->GetAt(0)),
                                kCameraPos,
                                kCuller
                            );
                        }
                    }
                    kIter = kNext;
                }
            }
            kGroupIter = kNextGroup;
        }

        pGroup = GetWaterSystem()->GetLODWaterGroup();
        if (pGroup) {
            NiTListItem<TESObjectREFR*>* kIter = pGroup->PlaceableWaterList.GetHeadPos();
            while (kIter) {
                NiTListItem<TESObjectREFR*>* kNext = pGroup->PlaceableWaterList.GetNextPos(kIter);
                if (kIter) {
                    TESObjectREFR* pPlane = pGroup->PlaceableWaterList.Get(kIter);
                    pPlane->GetObjectReference();
                    NiNode* pWaterNode = pPlane->Get3D();
                    if (NiObject::IsExactKindOf(&BSMultiBoundNode::ms_RTTI, pWaterNode->GetAt(0))) {
                        pSceneNode->UpdateMultiboundVisibility(
                            static_cast<BSMultiBoundNode*>(pWaterNode->GetAt(0)),
                            kCameraPos,
                            kCuller
                        );
                    }
                }
                kIter = kNext;
            }
        }
    }

    // The scene node is fetched again for each portal graph lookup, but not stored.
    if (BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->GetPortalGraph()) {
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->GetPortalGraph()->ResetRoomAccumList();
        {
            NiPointer<BSMultiBoundRoom> spPortalRoom(
                BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->GetPortalGraph()->QRoomRoot()
            );
            float fNear = apCamera->GetViewFrustum().m_fNear;
            NiPoint3 kMaxViewPos = apCamera->GetWorldDirection() * fNear;
            NiPoint3 kFrustumEndPos = apCamera->GetWorldTranslate() + kMaxViewPos;
            if (spPortalRoom) {
                BSMultiBoundRoom* pRoom = spPortalRoom;
                if (pRoom->QPointWithin(kFrustumEndPos)) {
                    const NiFrustumPlanes& kPlanes = kCuller.GetFrustumPlanes();
                    const NiCamera* pCullerCam = kCuller.GetCamera();
                    pRoom = spPortalRoom;
                    pSceneNode->UpdateRoomAndPortalVisibility(pRoom, pCullerCam, kPlanes, true);
                }
                else {
                    spPortalRoom = nullptr;
                }
            }

            bool bAddIntersection = true;
            if (!spPortalRoom) {
                const NiTPointerList<NiPointer<BSMultiBoundRoom>>& kMultiBoundList = pSceneNode->GetPortalGraph()
                                                                                         ->QRooms();
                if (!kMultiBoundList.IsEmpty()) {
                    NiTListItem<NiPointer<BSMultiBoundRoom>>* kIter = kMultiBoundList.GetHeadPos();
                    while (kIter) {
                        NiPointer<BSMultiBoundRoom>& spRoom = kMultiBoundList.GetNext(kIter);
                        BSMultiBoundRoom* pRoom = spRoom;
                        if (pRoom && pRoom->QPointWithin(kFrustumEndPos)) {
                            const NiFrustumPlanes& kPlanes = kCuller.GetFrustumPlanes();
                            const NiCamera* pCullerCam = kCuller.GetCamera();
                            pSceneNode->UpdateRoomAndPortalVisibility(pRoom, pCullerCam, kPlanes, bAddIntersection);
                            if (!spPortalRoom) {
                                spPortalRoom = pRoom;
                                bAddIntersection = false;
                            }
                        }
                    }
                }
            }

            if (!spPortalRoom) {
                const NiFrustumPlanes& kPlanes = kCuller.GetFrustumPlanes();
                const NiCamera* pCullerCam = kCuller.GetCamera();
                pSceneNode->UpdateRoomAndPortalVisibility(nullptr, pCullerCam, kPlanes, bAddIntersection);
            }

            BSMultiBoundRoom* pRoomRoot = spPortalRoom;
            pSceneNode->GetPortalGraph()->SetRoomRoot(pRoomRoot);
        }
    }

    TimerUpdateMultibound.Stop();
    kCuller.~NiCullingProcess();
}

// Resets the cull results that UpdateMultiBoundVisibility set: the multibound nodes of the loaded cells (and their land
// when abLandscape), the grass, the portal graph's occluders, its rooms with their portals and occluders when abRooms,
// and the placeable and LOD water. Each water loop takes the next position, then checks the current one again before
// using it.
void TES::ResetAllMultiBoundNodes(bool abLandscape, bool abRooms) {
    if (pInteriorCell) {
        NiNode* pMultiboundRoot = static_cast<NiNode*>(pInteriorCell->GetChildNode(TESObjectCELL::CN_MULTIBOUND));
        if (pMultiboundRoot) {
            for (uint32_t i = 0; i < pMultiboundRoot->GetArrayCount(); ++i) {
                BSMultiBoundNode* pMBNode = static_cast<BSMultiBoundNode*>(pMultiboundRoot->GetAt(i));
                if (pMBNode && !pMBNode->GetMultiBoundRoom()) {
                    pMBNode->QMultiBound()->QShape()->ResetCullResult();
                }
            }
        }
    }
    else {
        BGSTerrainManager* pTerrainManager = GetWorldSpace() ? GetWorldSpace()->GetTerrainManager() : nullptr;
        if (pTerrainManager) {
            pTerrainManager->ResetMultiBoundResults();
        }

        for (uint32_t x = 0; x < uGridsToLoad.UInt(); ++x) {
            for (uint32_t y = 0; y < uGridsToLoad.UInt(); ++y) {
                TESObjectCELL* pCell = pGridCellA->Get(x, y)->pCell;
                if (!pCell) {
                    continue;
                }

                NiNode* pMultiboundRoot = static_cast<NiNode*>(pCell->GetChildNode(TESObjectCELL::CN_MULTIBOUND));
                if (pMultiboundRoot) {
                    for (uint32_t i = 0; i < pMultiboundRoot->GetArrayCount(); ++i) {
                        NiAVObject* pMBNode = pMultiboundRoot->GetAt(i);
                        if (pMBNode) {
                            pMBNode->UpdateWorldBound();
                            static_cast<BSMultiBoundNode*>(pMBNode)->QMultiBound()->QShape()->ResetCullResult();
                        }
                    }
                }

                if (abLandscape) {
                    // The land node is not null-checked.
                    pMultiboundRoot = static_cast<NiNode*>(pCell->GetChildNode(TESObjectCELL::CN_LAND));
                    for (uint32_t j = 0; j < pMultiboundRoot->GetArrayCount(); ++j) {
                        NiAVObject* pMBNode = pMultiboundRoot->GetAt(j);
                        if (pMBNode) {
                            static_cast<BSMultiBoundNode*>(pMBNode)->QMultiBound()->QShape()->ResetCullResult();
                        }
                    }
                }
            }
        }
    }

    NiAVObject* pPotentialGrassRoot = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)
                                          ->GetAtChecked(6);
    NiNode* pGrassNode = pPotentialGrassRoot ? pPotentialGrassRoot->IsNode() : nullptr;
    if (pGrassNode) {
        for (uint32_t k = 0; k < pGrassNode->GetArrayCount(); ++k) {
            NiAVObject* pGrass = pGrassNode->GetAt(k);
            if (pGrass) {
                static_cast<BSMultiBoundNode*>(pGrass)->QMultiBound()->QShape()->ResetCullResult();
            }
        }
    }

    if (BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->GetPortalGraph()) {
        NiTListItem<BSOcclusionPlane*>* kOccluderIter = BSShaderManager::GetShadowSceneNode(
                                                            BSShaderManager::BSSM_SSN_WORLD
        )
                                                            ->GetPortalGraph()
                                                            ->QOccluderList()
                                                            .GetHeadPos();
        while (kOccluderIter) {
            BSOcclusionPlane* pOccluder = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)
                                              ->GetPortalGraph()
                                              ->QOccluderList()
                                              .GetNext(kOccluderIter);
            if (pOccluder) {
                pOccluder->ResetCullResult();
            }
        }

        const NiTPointerList<NiPointer<BSMultiBoundRoom>>&
            kRooms = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->GetPortalGraph()->QRooms();
        if (!kRooms.IsEmpty() && abRooms) {
            NiTListItem<NiPointer<BSMultiBoundRoom>>* kRoomIter = kRooms.GetHeadPos();
            while (kRoomIter) {
                NiPointer<BSMultiBoundRoom> spRoom(kRooms.GetNext(kRoomIter));
                if (spRoom) {
                    spRoom->UpdateWorldBound();
                    spRoom->SetCullResult(BSMultiBoundShape::BS_CULL_OCCLUDED);
                    spRoom->SetCompoundFrustum(nullptr);
                    NiTListItem<BSPortal*>* kPortalIter = spRoom->QPortalList().GetHeadPos();
                    while (kPortalIter) {
                        BSPortal* pPortal = spRoom->QPortalList().GetNext(kPortalIter);
                        if (pPortal) {
                            pPortal->ResetCullResult();
                            if (pPortal->QSharedGeometryNode()->QCompoundFrustum()) {
                                pPortal->QSharedGeometryNode()->QCompoundFrustum()->SetCamera(nullptr);
                            }
                        }
                    }

                    NiTListItem<BSOcclusionPlane*>* kRoomOccluderIter = spRoom->GetOccluders().GetHeadPos();
                    while (kRoomOccluderIter) {
                        BSOcclusionPlane* pOccluder = spRoom->GetOccluders().GetNext(kRoomOccluderIter);
                        if (pOccluder) {
                            pOccluder->ResetCullResult();
                        }
                    }
                }
            }
        }
    }

    if (GetWaterSystem()) {
        PlaceableWaterGroup* pGroup = nullptr;
        NiTListItem<PlaceableWaterGroup*>* kGroupIter = GetWaterSystem()->GetPlaceableWaterGroupList()->GetHeadPos();
        while (kGroupIter) {
            NiTListItem<PlaceableWaterGroup*>* kNextGroup = GetWaterSystem()->GetPlaceableWaterGroupList()->GetNextPos(
                kGroupIter
            );
            if (kGroupIter) {
                pGroup = GetWaterSystem()->GetPlaceableWaterGroupList()->Get(kGroupIter);
                NiTListItem<TESObjectREFR*>* kIter = pGroup->PlaceableWaterList.GetHeadPos();
                while (kIter) {
                    NiTListItem<TESObjectREFR*>* kNext = pGroup->PlaceableWaterList.GetNextPos(kIter);
                    if (kIter) {
                        TESObjectREFR* pPlane = pGroup->PlaceableWaterList.Get(kIter);
                        pPlane->GetObjectReference();
                        NiNode* pWaterNode = pPlane->Get3D();
                        if (NiObject::IsExactKindOf(&BSMultiBoundNode::ms_RTTI, pWaterNode->GetAt(0))) {
                            BSMultiBoundNode* pMBNode = static_cast<BSMultiBoundNode*>(pWaterNode->GetAt(0));
                            pMBNode->QMultiBound()->QShape()->ResetCullResult();
                        }
                    }
                    kIter = kNext;
                }
            }
            kGroupIter = kNextGroup;
        }

        pGroup = GetWaterSystem()->GetLODWaterGroup();
        if (pGroup) {
            NiTListItem<TESObjectREFR*>* kIter = pGroup->PlaceableWaterList.GetHeadPos();
            while (kIter) {
                NiTListItem<TESObjectREFR*>* kNext = pGroup->PlaceableWaterList.GetNextPos(kIter);
                if (kIter) {
                    TESObjectREFR* pPlane = pGroup->PlaceableWaterList.Get(kIter);
                    pPlane->GetObjectReference();
                    NiNode* pWaterNode = pPlane->Get3D();
                    if (NiObject::IsExactKindOf(&BSMultiBoundNode::ms_RTTI, pWaterNode->GetAt(0))) {
                        BSMultiBoundNode* pMBNode = static_cast<BSMultiBoundNode*>(pWaterNode->GetAt(0));
                        pMBNode->QMultiBound()->QShape()->ResetCullResult();
                    }
                }
                kIter = kNext;
            }
        }
    }
}

void TES::TestAllCells(int32_t aiControl) {
    static char bDiagnostics = 1;
    NiPoint3 kIgnored;
    NiPoint3 kPos;
    MemoryLevelManager::MEM_FREE_DATA kFreeData;
    if (aiControl == 4 || aiControl == 5 || aiControl == 9) {
        bbaseObjectsChecked = 1;
    }
    if (aiControl >= 6 && aiControl <= 7) {
        aiControl -= 5;
        bbaseObjectsChecked = 1;
        // The three-parameter function's address is stored as it is; C++ converts between function pointer types only
        // with reinterpret_cast.
        // TESObjectCELL::RenderTestCell takes three parameters but is cast to the two-parameter cell callback type.
        pfnTACCallbackFunc = reinterpret_cast<decltype(pfnTACCallbackFunc)>(&TESObjectCELL::RenderTestCell);
    }
    if (aiControl == -1) {
        bRunningCellTests = false;
        MessageHandler::IncDisableWarningCount(false);
        pTACRegionFilter = nullptr;
    }
    else if (aiControl) {
        TAC_Mode = aiControl;
        bRunningCellTests = true;
        MessageHandler::IncDisableWarningCount(true);
        BSSystemFile::DeleteFileA(TestAllCells_xls);
        if (!bbaseObjectsChecked) {
            Warning("CELLS: \r\n\r\n**************** Base Object Models ****************");
            pDataHandler->CheckModels(false);
            Warning("CELLS: \r\n\r\n**************** Base Object Icons/Textures ****************");
            pDataHandler->CheckIcons();
            bbaseObjectsChecked = 1;
        }
        if ((aiControl == 1 || aiControl == 4 || aiControl == 5 || aiControl == 9) &&
            !pCurrentWorldSpace &&
            !iinteriorCellCount) {
            pCurrentWorldSpace = pDataHandler->GetWorldSpaceList();
            bnewWorldNextLoop = true;
            pDataHandler->SortInteriorCells();
            iinteriorCellCount = pDataHandler->GetInteriorCellCount();
            iinteriorCellIndex = 0;
        }
        else if (aiControl == 3) {
            if (!pCurrentWorldSpace || pCurrentWorldSpace->m_item != pWorldSpace) {
                for (pCurrentWorldSpace = pDataHandler->GetWorldSpaceList(); pCurrentWorldSpace;
                     pCurrentWorldSpace = pCurrentWorldSpace->m_pkNext) {
                    TESWorldSpace* pws = pCurrentWorldSpace->m_item;
                    if (pws && pws == pWorldSpace) {
                        break;
                    }
                }
            }
            if (pCurrentWorldSpace) {
                bnewWorldNextLoop = true;
                pDataHandler->SortInteriorCells();
                iinteriorCellCount = pDataHandler->GetInteriorCellCount();
                iinteriorCellIndex = 0;
            }
        }
        else if (aiControl == 2) {
            if (pCurrentWorldSpace) {
                pCurrentWorldSpace = nullptr;
                bnewWorldNextLoop = false;
            }
            if (!iinteriorCellCount) {
                pDataHandler->SortInteriorCells();
                iinteriorCellCount = pDataHandler->GetInteriorCellCount();
                iinteriorCellIndex = 0;
            }
        }
    }
    bool bRepeat = true;
    while (bRepeat) {
        bRepeat = false;
        if (bRunningCellTests) {
            bRepeat = aiControl == 9;
            TESObjectCELL* pCell = nullptr;
            int32_t iStart = rt::clock();
            // The game sends a small relative mouse move here (SendInput; dx 2 when imouseDelta is positive, else -1),
            // which keeps the machine from going idle during the test. It is not done here because SDL has no input
            // injection, and SDL keeps the screen saver off itself.
            imouseDelta = -imouseDelta;
            if (bnewWorldNextLoop) {
                if (pCurrentWorldSpace) {
                    TESWorldSpace* pTestWorld;
                    for (pTestWorld = pCurrentWorldSpace->m_item; pTestWorld; pTestWorld = pCurrentWorldSpace->m_item) {
                        const char* pEditorID = pTestWorld->GetFormEditorID();
                        if (BSstrnicmp(pEditorID, "test", 4u)) {
                            break;
                        }
                        pCurrentWorldSpace = pCurrentWorldSpace->m_pkNext;
                        if (!pCurrentWorldSpace) {
                            pTestWorld = nullptr;
                            break;
                        }
                    }
                    if (pTestWorld) {
                        SetWorldSpace(pTestWorld);
                        iworldCellX = static_cast<int32_t>(pWorldSpace->GetMinX()) >> 12;
                        iworldMinY = static_cast<int32_t>(pWorldSpace->MinimumCoords.Y) >> 12;
                        int32_t imaxX = static_cast<int32_t>(pWorldSpace->MaximumCoords.X) >> 12;
                        int32_t imaxY = static_cast<int32_t>(pWorldSpace->MaximumCoords.Y) >> 12;
                        int32_t iMinY = iworldMinY;
                        int32_t iMinX = iworldCellX;
                        const char* pEditorID = pWorldSpace->GetFormEditorID();
                        Warning(
                            "CELLS: \r\n\r\n**************** %s (%d,%d) - (%d,%d) ****************",
                            pEditorID,
                            iMinX,
                            iMinY,
                            imaxX,
                            imaxY
                        );
                    }
                }
                bnewWorldNextLoop = false;
            }
            char cCellFound = 0;
            bool bFinishedInts = false;
            while (!cCellFound && !bnewWorldNextLoop && !bFinishedInts) {
                if (pCurrentWorldSpace) {
                    pCell = pWorldSpace->LoadCell(iworldCellX, iworldMinY);
                    if (pCell) {
                        AddToExteriorBuffer(pCell);
                    }
                    ++iworldMinY;
                    if (iworldMinY > static_cast<int32_t>(pWorldSpace->MaximumCoords.Y) >> 12) {
                        iworldMinY = static_cast<int32_t>(pWorldSpace->MinimumCoords.Y) >> 12;
                        ++iworldCellX;
                        if (iworldCellX > static_cast<int32_t>(pWorldSpace->MaximumCoords.X) >> 12) {
                            pCurrentWorldSpace = pCurrentWorldSpace->m_pkNext;
                            if (pCurrentWorldSpace) {
                                bnewWorldNextLoop = true;
                            }
                        }
                    }
                }
                else {
                    const char* pEditorID;
                    if (!iinteriorCellIndex) {
                        Warning("CELLS: \r\n\r\n**************** Interiors ****************");
                    }
                    do {
                        pCell = pDataHandler->GetInteriorCellByIndex(iinteriorCellIndex);
                        ++iinteriorCellIndex;
                        if (!pCell) {
                            break;
                        }
                        pEditorID = pCell->GetFormEditorID();
                    }
                    while (!BSstrnicmp(pEditorID, "test", 4u));
                    if (!pCell) {
                        bFinishedInts = true;
                    }
                }
                if (pCell && !pCell->GetDelete() && pCell->GetFirstRefr()) {
                    cCellFound = 1;
                }
            }
            if (pCell && cCellFound) {
                TESRegionList* pRegionList = pCell->GetRegionList(true);
                if (pTACRegionFilter && (!pRegionList || !pRegionList->IsInList(pTACRegionFilter))) {
                    cCellFound = 0;
                }
            }
            else if (bnewWorldNextLoop && pPlayer->GetWorldSpace() != pWorldSpace) {
                bRepeat = true;
            }
            if (cCellFound) {
                appCalendar.Update(10.0f);
                kPos = NiPoint3::ZERO;
                if (!pCell->GetInterior()) {
                    float fCenterY = static_cast<double>(pCell->GetDataY() << 12) + 2048.0;
                    float fCenterX = static_cast<double>(pCell->GetDataX() << 12) + 2048.0;
                    kPos = *::new (&kIgnored) NiPoint3(fCenterX, fCenterY, 0.0f);
                }
                if (pCell->GetInterior()) {
                    NiPoint3 kIgnoredAngle;
                    pCell->LoadAllTempData();
                    pCell->GetCOCPlacementInfo(kPos, kIgnoredAngle);
                    LoadInterior(pCell, &kPos);
                }
                else if (pInteriorCell) {
                    float fLandHeight;
                    LeaveInterior(&kPos);
                    TESObjectLAND* pLand = pCell->GetLand();
                    pLand->GetLandHeight(kPos, fLandHeight);
                    if (fLandHeight >= 0.0) {
                        kPos.z = fLandHeight;
                    }
                }
                else {
                    float fLandHeight;
                    UpdateCurrentGridCell(&kPos, false);
                    TESObjectLAND* pLand = pCell->GetLand();
                    pLand->GetLandHeight(kPos, fLandHeight);
                    if (fLandHeight >= 0.0) {
                        kPos.z = fLandHeight;
                    }
                }
                PLAYER_TARGET_LOC* pLoc = new PLAYER_TARGET_LOC;
                pLoc->Location = kPos;
                pLoc->Angle = pPlayer->GetAngleOnReference();
                pLoc->bResetWeather = true;
                if (pCell->GetInterior()) {
                    pLoc->pInterior = pCell;
                }
                else {
                    pLoc->pWorld = pCell->GetWorldSpace();
                }
                pPlayer->RequestPositionPlayer(pLoc);
                pPlayer->HandlePositionPlayerRequest();
                pIOManager->LoadAllQueuedPriorities();
                int32_t iStop = rt::clock();
                float fTime = static_cast<double>(iStop - iStart) / 1000.0;
                MemoryLevelManager::BeginMemoryFree(&kFreeData, true, true, true);
                TES::pInstance->ClearAllCells(true, false);
                MemoryLevelManager::FreeReleasedObjects(kFreeData.bLockedFaceGen);
                MemoryLevelManager::EndMemoryFree(kFreeData);
                if (bDiagnostics) {
                    float fTnumber = 0.0f;
                    float fMnumber = 0.0f;
                    BSStringT<char> textMnum;
                    BGSSceneInfo::CollectSceneDataForLoadedArea(fTnumber, fMnumber, true, true);
                    BGSSceneInfo::BuildMNumberString(fMnumber, textMnum, false);
                    bool bPortaled = false;
                    BSPortalGraph* pPortalGraph = pCell->GetPortalGraph();
                    if (pPortalGraph && !pPortalGraph->QRooms().IsEmpty()) {
                        bPortaled = true;
                    }

                    uint32_t uiTexMemory = 0;
                    for (NiTexture* pTex = NiTexture::GetListHead(); pTex; pTex = pTex->m_pkNext) {
                        if (pTex->m_pkRendererData) {
                            uiTexMemory += GetTextureDumpSize(pTex);
                        }
                    }
                    uiTexMemory >>= 20;
                    uint32_t uiHighActors = ReferenceProcessLists.CountHighActors();
                    if (pCell->GetInterior()) {
                        uint32_t uiLightCount = pCell->GetLightCount();
                        const char* pEditorID = pCell->GetFormEditorID();
                        Warning(
                            "CELLS: Cell \"%s\" (Interior) M#: %s T#: %.0f Time: %.1f Lights: %d Portals: %s "
                            "TexTotal: %i MB Actors: %i",
                            pEditorID,
                            textMnum.pString,
                            fTnumber,
                            fTime,
                            uiLightCount,
                            bPortaled ? "Y" : "N",
                            uiTexMemory,
                            uiHighActors
                        );
                        uiLightCount = pCell->GetLightCount();
                        pEditorID = pCell->GetFormEditorID();
                        PrintToFile(
                            TestAllCells_xls,
                            "%s\tInterior\t%s\t%.0f\t%.1f\t%d\t%s\t%d\t%i\r\n",
                            pEditorID,
                            textMnum.pString,
                            fTnumber,
                            fTime,
                            uiLightCount,
                            bPortaled ? "Y" : "N",
                            uiTexMemory,
                            uiHighActors
                        );
                    }
                    else {
                        int32_t iDataY = pCell->GetDataY();
                        int32_t iDataX = pCell->GetDataX();
                        const char* pEditorID = pCell->GetFormEditorID();
                        Warning(
                            "CELLS: Cell \"%s\" (%d, %d) M#: %s T#: %.0f Time: %.1f Portals: %s TexTotal: %i MB "
                            "Actors: %i",
                            pEditorID,
                            iDataX,
                            iDataY,
                            textMnum.pString,
                            fTnumber,
                            fTime,
                            bPortaled ? "Y" : "N",
                            uiTexMemory,
                            uiHighActors
                        );
                        iDataY = pCell->GetDataY();
                        iDataX = pCell->GetDataX();
                        pEditorID = pCell->GetFormEditorID();
                        PrintToFile(
                            TestAllCells_xls,
                            "%s\t%d, %d\t%s\t%.0f\t%.1f\t%s\t%d\t%i\r\n",
                            pEditorID,
                            iDataX,
                            iDataY,
                            textMnum.pString,
                            fTnumber,
                            fTime,
                            bPortaled ? "Y" : "N",
                            uiTexMemory,
                            uiHighActors
                        );
                    }
                }
                if (pfnTACCallbackFunc) {
                    pfnTACCallbackFunc(pCell, pTACCallbackData);
                }
                if (TAC_Mode == 4 || TAC_Mode == 5) {
                    pSaveLoadGameOLD->TestAllCells(TAC_Mode);
                }
            }
            if (iinteriorCellIndex >= iinteriorCellCount) {
                iinteriorCellCount = 0;
                bRunningCellTests = false;
                MessageHandler::IncDisableWarningCount(false);
                pTACRegionFilter = nullptr;
            }
        }
    }
}

NiNode* TES::GetTempNodeManager() {
    return pTempNodeManager;
}

void TES::ClearCellLoaderTasks() {
    pExteriorCellLoader->CancelAllCellLoads();
}

bool TES::GetLandHeight(const NiPoint3& acoord, float& afHeight) {
    int32_t ix = FloatToInt(acoord.x);
    int32_t iy = FloatToInt(acoord.y);
    if (!(ix % 4096) && acoord.x < static_cast<double>(ix)) {
        --ix;
    }
    if (!(iy % 4096) && acoord.y < static_cast<double>(iy)) {
        --iy;
    }
    afHeight = -2048.0f;
    TESObjectCELL* pCell = GetGridCellCell(ix >> 12, iy >> 12);
    if (!pCell || !TES::pInstance->IsCellLoaded(pCell, false)) {
        return false;
    }
    TESObjectLAND* pLand = pCell->GetLand();
    return pLand->GetLandHeight(acoord, afHeight);
}

NiAVObject* TES::Pick(bhkPickData& arResult) {
    return Pick(arResult, true);
}

// Reads uGridsToLoad twice, the y coordinate first.
NiAVObject* TES::Pick(bhkPickData& arResult, bool abHavok) {
    if (!abHavok) {
        return PickNI(arResult);
    }
    TESObjectCELL* pCell = GetInterior();
    if (!pCell) {
        int32_t iY = uGridsToLoad.UInt() >> 1;
        int32_t iX = uGridsToLoad.UInt() >> 1;
        pCell = pGridCellA->Get(iX, iY)->pCell;
    }
    if (pCell) {
        return pCell->Pick(arResult);
    }
    return nullptr;
}

NiAVObject* TES::PickNI(bhkPickData& arResult) {
    NiPoint3 kOrigin;
    NiPoint3 kPick;
    arResult.GetFrom(kOrigin);
    arResult.GetTo(kPick);
    NiPoint3 kDirection = kPick - kOrigin;
    NiPick kPicker(0, 8u);
    kPicker.SetTarget(pObjRoot);
    kPicker.SetObserveAppCullFlag(true);
    NiAVObject* pObject = nullptr;
    if (kPicker.PickObjects(kOrigin, kDirection, false)) {
        NiPick::Record* pPickRecord = kPicker.m_pickResults.GetAt(0);
        pObject = pPickRecord->GetAVObject();
    }
    return pObject;
}

// The elapsed ticks are divided as unsigned values, and the load time is widened to double as an unsigned value.
void TES::TestCell(TESObjectCELL* apCell, uint32_t aiOptions, uint32_t aiTestStartTime) {
    if (!apCell) {
        return;
    }
    if (apCell->GetInterior()) {
        const char* pName = static_cast<TESFullName*>(apCell)->GetFullName();
        uint32_t iFormID = apCell->GetFormID();
        InitializationFlow("Moving to interior cell %08X %s", iFormID, pName);
    }
    else {
        const char* pName = static_cast<TESFullName*>(apCell)->GetFullName();
        int32_t iY = apCell->GetDataY();
        int32_t iX = apCell->GetDataX();
        uint32_t iFormID = apCell->GetFormID();
        InitializationFlow("Moving to exterior cell %08X (%i,%i) %s", iFormID, iX, iY, pName);
    }
    uint32_t iLoadStart = BSGetTickCount();
    pPlayer->CenterOnCell(nullptr, apCell);
    pIOManager->LoadAllQueuedPriorities();
    uint32_t iLoadEnd = BSGetTickCount();
    uint32_t iElapsed = BSGetTickCount() - aiTestStartTime;
    uint32_t iHours = iElapsed / 3600000;
    iElapsed -= iHours * 3600000;
    uint32_t iMinutes = iElapsed / 60000;
    iElapsed -= iMinutes * 60000;
    uint32_t iSeconds = iElapsed / 1000;
    if (apCell->GetInterior()) {
        double fLoadTime = static_cast<double>(iLoadEnd - iLoadStart) / 1000.0;
        InitializationFlow(
            "Interior cell finished loading in %.02f seconds.  Total test time: %02i:%02i:%02i",
            fLoadTime,
            iHours,
            iMinutes,
            iSeconds
        );
    }
    else {
        double fLoadTime = static_cast<double>(iLoadEnd - iLoadStart) / 1000.0;
        int32_t iY = apCell->GetDataY();
        int32_t iX = apCell->GetDataX();
        InitializationFlow(
            "Exterior cell (%i,%i) finished loading in %.02f seconds.  Total test time: %02i:%02i:%02i",
            iX,
            iY,
            fLoadTime,
            iHours,
            iMinutes,
            iSeconds
        );
    }
    appCalendar.Update(120.0f);
    ReferenceProcessLists.ChangeProcessLevelTempList();
    ReferenceProcessLists.UpdateProcessLists();
    ReferenceProcessLists.ChangeProcessLevelTempList();
    if (aiOptions >= 4 && aiOptions <= 5) {
        pSaveLoadGameOLD->TestAllCells(aiOptions);
    }
}

void TES::DecrementPlaceableWaterCount() {
    --iPlaceableWaterCount;
}

void TES::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    for (int32_t i = 0; i < static_cast<int32_t>(TES::uInteriorCellBufferGeneral.UInt()); ++i) {
        if (pInteriorBuffer[i] && TES::pInstance->IsCellLoaded(pInteriorBuffer[i], true)) {
            pInteriorBuffer[i]->UpdateSounds();
        }
    }
    for (int32_t j = 0; j < static_cast<int32_t>(uExteriorCellBuffer.UInt()); ++j) {
        if (pExteriorBuffer[j] && TES::pInstance->IsCellLoaded(pExteriorBuffer[j], true)) {
            pExteriorBuffer[j]->UpdateSounds();
        }
    }
    spLoadedAreaBound->Clear();
}

void TES::Revert(BGSLoadGameBuffer* apLoadGameBuffer) {
    ClearDeadCountList();
    iSaveGridX = 2147483647;
    iSaveGridY = 2147483647;
}

void TES::AddTempDebugObject(NiAVObject* apObject, float afTime) {
    if (apObject) {
        BSTempNode* pNode = new BSTempNode(afTime);
        pNode->AttachChild(apObject, 1);
        pTempNodeManager->AttachChild(pNode, 1);
        pNode->UpdateProperties();
    }
}

NiGeometry* TES::GetLandGeometry(const NiPoint3& acoord) {
    int32_t iCellX = FloatToInt(acoord.x);
    int32_t iCellY = FloatToInt(acoord.y);
    if (!(iCellX % 0x1000) && acoord.x < static_cast<double>(iCellX)) {
        --iCellX;
    }
    if (!(iCellY % 0x1000) && acoord.y < static_cast<double>(iCellY)) {
        --iCellY;
    }
    TESObjectCELL* pTestCell = GetGridCellCell(iCellX >> 12, iCellY >> 12);
    if (!pTestCell || !TES::pInstance->IsCellLoaded(pTestCell, false)) {
        return nullptr;
    }
    TESObjectLAND* pLand = pTestCell->GetLand();
    return pLand->GetGeometry(acoord);
}

bool TES::IsCoordInLoadedArea(const NiPoint3& aCoord) {
    if (TES::GetInterior()) {
        return true;
    }
    int32_t iCellX = FloatToInt(aCoord.x) >> 12;
    int32_t iCellY = FloatToInt(aCoord.y) >> 12;
    int32_t iMinX = static_cast<int32_t>(iCurrentGridX - (uGridsToLoad.UInt() >> 1));
    int32_t iMinY = static_cast<int32_t>(iCurrentGridY - (uGridsToLoad.UInt() >> 1));
    return iCellX >= iMinX &&
        iCellX < static_cast<int32_t>(uGridsToLoad.UInt() + iMinX) &&
        iCellY >= iMinY &&
        iCellY < static_cast<int32_t>(uGridsToLoad.UInt() + iMinY);
}

TESObjectCELL* TES::GetCellForPoint(NiPoint3 aLocation) {
    if (TES::GetInterior()) {
        return TES::GetInterior();
    }
    TESObjectCELL* pCell = GetGridCellCell(
        static_cast<int32_t>(aLocation.x) >> 12,
        static_cast<int32_t>(aLocation.y) >> 12
    );
    if (pCell && TES::pInstance->IsCellLoaded(pCell, true)) {
        return pCell;
    }

    return nullptr;
}

void TES::UpdateRefSounds() {
    if (TES::GetInterior()) {
        TES::GetInterior()->UpdateRefSounds();
    }
    else {
        pGridCellA->UpdateRefSounds();
    }
}

void TES::ToggleCollisionGeometry() {
    TESObjectCELL* pInterior = TES::GetInterior();
    if (pInterior) {
        pInterior->ToggleCollisionGeometry();
    }
    else {
        pGridCellA->ToggleCollisionGeometry();
    }
}

void TES::ShowLandBorders(bool abShow) {
    if (bShowLANDborders != abShow) {
        bShowLANDborders = abShow;
        if (pDataHandler && !TES::GetInterior()) {
            pGridCellA->ShowLandBorders(abShow);
        }
    }
}

bool TES::GetLandNormal(const NiPoint3& aCoord, NiPoint3& aNormal, NiPoint3& aFaceNormal) {
    int32_t ix = FloatToInt(aCoord.x);
    int32_t iy = FloatToInt(aCoord.y);
    if (!(ix % 4096) && aCoord.x < static_cast<double>(ix)) {
        --ix;
    }
    if (!(iy % 4096) && aCoord.y < static_cast<double>(iy)) {
        --iy;
    }
    aNormal = NiPoint3::ZERO;
    aNormal.z = 1.0f;
    TESObjectCELL* pCell = GetGridCellCell(ix >> 12, iy >> 12);
    if (!pCell || !TES::pInstance->IsCellLoaded(pCell, false)) {
        return false;
    }
    TESObjectLAND* pLand = pCell->GetLand();
    return pLand->GetLandNormal(aCoord, aNormal, aFaceNormal);
}

TESObjectCELL* TES::GetExteriorBufferCellByIndex(int32_t aiIndex) {
    return pExteriorBuffer[aiIndex];
}

uint32_t TES::GetExteriorBufferSize() {
    return uExteriorCellBuffer.UInt();
}

void TES::SetAllowUnusedPurge(bool abAllow) {
    bAllowUnusedPurge = abAllow;
}

void TES::SetHideLandNormalMapForGridCell(int32_t aiGridX, int32_t aiGridY, bool abFlag) {
    GridCell* pGridCell = pGridCellA->Get(aiGridX, aiGridY);
    if (pGridCell && pGridCell->pCell) {
        for (int32_t i = 0; i < 4; ++i) {
            NiAVObject* pLandNode = pGridCell->pCell->GetLandNode(i);
            NiGeometry* pLandGeo = nullptr;
            if (static_cast<NiNode*>(pLandNode)->GetAtChecked(0)) {
                NiAVObject* pChild = static_cast<NiNode*>(pLandNode)->GetAt(0);
                pLandGeo = pChild->IsGeometry();
            }
            if (pLandGeo) {
                int32_t iType = NiShadeProperty::GetType();
                NiProperty* pProperty = pLandGeo->NiAVObject::GetProperty(iType);
                static_cast<BSShaderProperty*>(pProperty)->SetFlag(
                    BSShaderProperty::BSSP_FLAG_SKIP_NORMAL_MAPS,
                    !abFlag
                );
            }
        }
    }
}

void TES::FreeWater() {
    TESWaterSystem* pWater = pWaterSystem;
    if (pWater) {
        delete pWater;
    }
    pWaterSystem = nullptr;
}

// Releases the preloaded addon node files.
void TES::UnloadAddonNodes() {
    spPreloadedAddonNodes = nullptr;
}

// Releases the preloaded form files.
void TES::UnPreloadForms() {
    spPreloadedForms = nullptr;
}

void TES::SetHideLandBlendPassForGridCell(int32_t aiGridX, int32_t aiGridY, bool abFlag) {
    GridCell* pGridCell = pGridCellA->Get(aiGridX, aiGridY);
    if (pGridCell && pGridCell->pCell) {
        for (int32_t i = 0; i < 4; ++i) {
            NiAVObject* pLandNode = pGridCell->pCell->GetLandNode(i);
            NiGeometry* pLandGeo = nullptr;
            if (static_cast<NiNode*>(pLandNode)->GetAtChecked(0)) {
                NiAVObject* pChild = static_cast<NiNode*>(pLandNode)->GetAt(0);
                pLandGeo = pChild->IsGeometry();
            }
            if (pLandGeo) {
                int32_t iType = NiShadeProperty::GetType();
                NiProperty* pProperty = pLandGeo->NiAVObject::GetProperty(iType);
                static_cast<BSShaderProperty*>(pProperty)->SetFlag(
                    BSShaderProperty::BSSP_FLAG_SKIP_LANDLO_PASS,
                    abFlag
                );
            }
        }
    }
}

bool TES::GetShowLandBorders() {
    return bShowLANDborders;
}

bool TES::CellTestRunning2() {
    return bRunningCellTests2;
}

void TES::RemoveCellFromInteriorBuffer(TESObjectCELL* apCell) {
    uint32_t i;
    for (i = 0; i < TES::uInteriorCellBufferGeneral.UInt(); ++i) {
        if (apCell == pInteriorBuffer[i]) {
            pDataHandler->UnloadCell(apCell);
            pInteriorBuffer[i] = nullptr;
            break;
        }
    }
    while (i < TES::uInteriorCellBufferGeneral.UInt() - 1) {
        pInteriorBuffer[i] = pInteriorBuffer[i + 1];
        ++i;
    }
    pInteriorBuffer[TES::uInteriorCellBufferGeneral.UInt() - 1] = nullptr;
    SetUpdateGridString();
}

bool TES::GetLandColor(const NiPoint3& acoord, NiColorA& acolor) {
    int32_t ix = FloatToInt(acoord.x);
    int32_t iy = FloatToInt(acoord.y);
    if (!(ix % 4096) && acoord.x < static_cast<double>(ix)) {
        --ix;
    }
    if (!(iy % 4096) && acoord.y < static_cast<double>(iy)) {
        --iy;
    }
    TESObjectCELL* pCell = GetGridCellCell(ix >> 12, iy >> 12);
    if (!pCell || !TES::pInstance->IsCellLoaded(pCell, false)) {
        return false;
    }
    TESObjectLAND* pLand = pCell->GetLand();
    return pLand->GetLandColor(acoord, acolor);
}

void TES::QueueReferencesForAttachedCells() {
    if (pDataHandler) {
        if (TES::GetInterior()) {
            if (TES::GetInterior()->GetAttached()) {
                TES::GetInterior()->QueueReferences(false);
            }
        }
        for (uint32_t x = 0; x < uGridsToLoad.UInt(); ++x) {
            for (uint32_t y = 0; y < uGridsToLoad.UInt(); ++y) {
                TESObjectCELL* pCell = pGridCellA->Get(x, y)->pCell;
                if (pCell && pCell->GetAttached()) {
                    pCell->QueueReferences(false);
                }
            }
        }
    }
}

void TES::UpdateFadeNodesForAttachedCells() {
    if (pDataHandler) {
        if (TES::GetInterior()) {
            if (TES::GetInterior()->GetAttached()) {
                TES::GetInterior()->UpdateFadeNodes();
            }
        }
        for (uint32_t x = 0; x < uGridsToLoad.UInt(); ++x) {
            for (uint32_t y = 0; y < uGridsToLoad.UInt(); ++y) {
                TESObjectCELL* pCell = pGridCellA->Get(x, y)->pCell;
                if (pCell && pCell->GetAttached()) {
                    pCell->UpdateFadeNodes();
                }
            }
        }
    }
}

void TES::InitDistantTextureBlending() {
    for (uint32_t x = 0; x < uGridsToLoad.UInt(); ++x) {
        for (uint32_t y = 0; y < uGridsToLoad.UInt(); ++y) {
            TESObjectCELL* pCell = pGridCellA->Get(x, y)->pCell;
            TESObjectLAND* pLand = pCell ? pCell->GetLand() : nullptr;
            if (pCell && pLand && pLand->HasVertices()) {
                pLand = pCell->GetLand();
                pLand->InitDistantTextureBlending();
            }
        }
    }
}

void TES::UpdateGridLights() {
    for (uint32_t x = 0; x < uGridsToLoad.UInt(); ++x) {
        for (uint32_t y = 0; y < uGridsToLoad.UInt(); ++y) {
            TESObjectCELL* pCell = pGridCellA->Get(x, y)->pCell;
            if (pCell && pCell->GetAttached()) {
                pCell->AttachLights(true);
            }
        }
    }
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->UpdateLighting(true, true);
}

void TES::UpdateDeferredCellAnimationTransforms() {
    TESObjectCELL* pInterior = TES::GetInterior();
    if (pInterior) {
        pInterior->UpdateManagedNodeTransforms();
    }
    else {
        pGridCellA->UpdateManagedNodeTransforms();
    }
}

void TES::ToggleCollision() {
    int8_t cDisable = !TES::QCollisionDisableFlag();
    TES::SetCollisionDisableFlag(cDisable);
    TESObjectCELL* pInterior = TES::pInstance->TES::GetInterior();
    bhkWorldM* pWorld;
    if (pInterior) {
        pWorld = static_cast<bhkWorldM*>(pInterior->GetbhkWorld());
    }
    else {
        pWorld = TESObjectCELL::ExteriorWorld();
    }
    if (pWorld) {
        pWorld->bhkWorld::SetEnabled(cDisable == 0);
    }
    bhkRagdollController::SetRagdollAnim(cDisable == 0);
}

void TES::GetLoadedAreaRect(tagRECT& aOutRect, float afOffsetInside) {
    uint32_t uiMinX = iCurrentGridX - (uGridsToLoad.UInt() >> 1);
    uint32_t uiMinY = iCurrentGridY - (uGridsToLoad.UInt() >> 1);
    uint32_t uiMaxX = iCurrentGridX + (uGridsToLoad.UInt() >> 1);
    uint32_t uiMaxY = iCurrentGridY + (uGridsToLoad.UInt() >> 1);
    aOutRect.left = static_cast<int32_t>(static_cast<double>(static_cast<int32_t>(uiMinX << 12)) + afOffsetInside);
    aOutRect.bottom = static_cast<int32_t>(static_cast<double>(static_cast<int32_t>(uiMinY << 12)) + afOffsetInside);
    aOutRect.right = static_cast<int32_t>(
        static_cast<double>(static_cast<int32_t>((uiMaxX << 12) + 4096)) - afOffsetInside
    );
    aOutRect.top = static_cast<int32_t>(
        static_cast<double>(static_cast<int32_t>((uiMaxY << 12) + 4096)) - afOffsetInside
    );
}

bool TES::AllCellsInGridLoaded() {
    if (!TES::pInstance->GetWorldSpace()) {
        return false;
    }
    uint32_t uiMinX = iCurrentGridX - (uGridsToLoad.UInt() >> 1);
    uint32_t uiMinY = iCurrentGridY - (uGridsToLoad.UInt() >> 1);
    for (uint32_t x = 0; x < uGridsToLoad.UInt(); ++x) {
        for (uint32_t y = 0; y < uGridsToLoad.UInt(); ++y) {
            GridCell* pGridCell = pGridCellA->Get(x, y);
            if (pGridCell) {
                TESObjectCELL* pCell = pGridCell->pCell;
                if (!pGridCell->pCell) {
                    pCell = TES::pInstance->GetWorldSpace()->GetCellFromCellCoord(x + uiMinX, y + uiMinY);
                }
                if (!pCell || !IsExteriorBuffered(pCell)) {
                    return false;
                }
            }
        }
    }
    return true;
}

GridCell* TES::FindFirstGridCellWithWater() {
    GridCell* pCell = nullptr;
    uint32_t uiHalfGrid = uGridsToLoad.UInt() >> 1;
    for (uint32_t i = 0; i < uiHalfGrid; ++i) {
        if (i) {
            uint32_t uiMaxY = i + uiHalfGrid;
            uint32_t uiMaxX = i + uiHalfGrid;
            uint32_t uiMinY = uiHalfGrid - i;
            for (uint32_t j = uiHalfGrid - i; j <= uiMaxX; ++j) {
                GridCell* pGridCell = pGridCellA->Get(j, uiMaxY);
                if (pGridCell && pGridCell->pCell && pGridCell->pCell->HasWater()) {
                    pCell = pGridCell;
                    break;
                }
                pGridCell = pGridCellA->Get(j, uiMinY);
                if (pGridCell && pGridCell->pCell && pGridCell->pCell->HasWater()) {
                    pCell = pGridCell;
                    break;
                }
            }
            if (!pCell) {
                for (uint32_t k = uiMinY + 1; k < uiMaxY; ++k) {
                    GridCell* pGridCell = pGridCellA->Get(uiHalfGrid - i, k);
                    if (pGridCell && pGridCell->pCell && pGridCell->pCell->HasWater()) {
                        pCell = pGridCell;
                        break;
                    }
                    pGridCell = pGridCellA->Get(uiMaxX, k);
                    if (pGridCell && pGridCell->pCell && pGridCell->pCell->HasWater()) {
                        pCell = pGridCell;
                        break;
                    }
                }
            }
        }
        else {
            GridCell* pGridCell = pGridCellA->Get(uiHalfGrid, uiHalfGrid);
            if (pGridCell && pGridCell->pCell && pGridCell->pCell->HasWater()) {
                pCell = pGridCell;
            }
        }
        if (pCell) {
            break;
        }
    }
    return pCell;
}

void TES::ShowCellNode(int32_t aiNode, bool abShow, bool abActiveCellsOnly) {
    TESObjectCELL::SetNodeCulled(static_cast<TESObjectCELL::CELL_CULL_NODE>(aiNode), !abShow);
    if (aiNode == 11) {
        if (!TES::pInstance->TES::GetInterior() || !TES::pInstance->TES::GetInterior()->HasWater()) {
            TESWaterSystem::GetWaterRoot()->SetAppCulled(!abShow);
        }
    }
    else {
        int32_t iInteriorCount;
        if (abActiveCellsOnly) {
            iInteriorCount = 0;
        }
        else {
            iInteriorCount = static_cast<int32_t>(TES::uInteriorCellBufferGeneral.UInt());
        }
        for (int32_t i = -1; i < iInteriorCount; ++i) {
            TESObjectCELL* pInterior = i == -1 ? TES::GetInterior() : pInteriorBuffer[i];
            if (pInterior && static_cast<uint32_t>(aiNode) <= 8) {
                NiAVObject* pChildNode = pInterior->GetChildNode(static_cast<TESObjectCELL::CELLNODE>(aiNode));
                if (pChildNode) {
                    pChildNode->SetAppCulled(!abShow);
                }
                pInterior->CullChildrenOfMultibounds(!abShow, static_cast<TESObjectCELL::CELL_CULL_NODE>(aiNode));
            }
        }
        if (abActiveCellsOnly) {
            for (uint32_t uiX = 0; uiX < uGridsToLoad.UInt(); ++uiX) {
                for (uint32_t uiY = 0; uiY < uGridsToLoad.UInt(); ++uiY) {
                    TESObjectCELL* pCell = TES::pInstance->GetGridCell(uiX, uiY)->pCell;
                    if (pCell) {
                        switch (aiNode) {
                            case 0:
                            case 1:
                            case 2:
                            case 3:
                            case 4:
                            case 5:
                            case 7:
                            case 8: {
                                NiAVObject* pChildNode = pCell->GetChildNode(
                                    static_cast<TESObjectCELL::CELLNODE>(aiNode)
                                );
                                if (pChildNode) {
                                    pChildNode->SetAppCulled(!abShow);
                                }
                                break;
                            }
                            default: {
                                continue;
                            }
                        }
                    }
                }
            }
        }
        else {
            for (int32_t j = 0; j < static_cast<int32_t>(uExteriorCellBuffer.UInt()); ++j) {
                TESObjectCELL* pCell = pExteriorBuffer[j];
                if (pCell) {
                    switch (aiNode) {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                        case 7:
                        case 8: {
                            NiAVObject* pChildNode = pCell->GetChildNode(static_cast<TESObjectCELL::CELLNODE>(aiNode));
                            if (pChildNode) {
                                pChildNode->SetAppCulled(!abShow);
                            }
                            break;
                        }
                        default: {
                            continue;
                        }
                    }
                }
            }
        }
    }
}

void TES::LoadGridCell(int32_t aiGridX, int32_t aiGridY, int32_t aiCellX, int32_t aiCellY) {
    if (pWorldSpace) {
        TESObjectCELL* pCell = pWorldSpace->LoadCell(aiCellX, aiCellY);
        if (!pCell) {
            pCell = pDataHandler->GetCellFromCellCoord(aiCellX, aiCellY, pWorldSpace, true);
            pWorldSpace->AssignPersistentRefsToCell(pCell);
            pCell->SetHasTempData(true);
        }
        if (pGridCellA->GetAttached(static_cast<uint32_t>(aiGridX), static_cast<uint32_t>(aiGridY))) {
            AddToBuffer(pCell);
        }
        else {
            char cMessage[268];
            int32_t iFormID = pCell->iFormID;
            int32_t iDataY = pCell->GetDataY();
            int32_t iDataX = pCell->GetDataX();
            std::sprintf(cMessage, "Loading cell...%s (%i, %i) (%08X)", pCell->GetCellName(), iDataX, iDataY, iFormID);
            pGridCellA->Set(static_cast<uint32_t>(aiGridX), static_cast<uint32_t>(aiGridY), pCell);
            pGridCellA->AttachToWorld(pObjRoot, static_cast<uint32_t>(aiGridX), static_cast<uint32_t>(aiGridY));
            TESObjectLAND* pLand = pCell->GetLand();
            if (!pLand->LoadVertices(true)) {
                pLand->UpdateMesh(false, nullptr);
            }
            NiPoint2 kExtents = pCell->GetLand()->GetMinMaxLandHeight();
            double dWaterHeight = pCell->GetWaterHeight();
            pCell->SetHasWater(kExtents.x < dWaterHeight);
            if (TESWaterSystem::bUseWater.Bool() && pCell->HasWater() && !pCell->bAutoWaterLoaded) {
                BGSAutoWater::GeneratePlaceableWaterForCell(pCell);
            }
            pLand->DrawLandBorder(bShowLANDborders);
        }
    }
}

int32_t TES::GetFaceCount(NiNode* apRoot, bool abCountculled) {
    int32_t iCount = 0;
    if (!apRoot) {
        return iCount;
    }
    for (uint32_t n = 0; n < apRoot->GetArrayCount(); ++n) {
        NiPointer<NiAVObject> spChild(apRoot->GetAt(n));
        if (spChild.m_pObject) {
            NiNode* pChildNode = static_cast<NiNode*>(spChild.m_pObject);
            if (pChildNode->IsNode()) {
                iCount += GetFaceCount(static_cast<NiNode*>(spChild.m_pObject), abCountculled);
            }
            else if (pChildNode->IsTriBasedGeom()) {
                NiTriBasedGeom* pTriGeom = static_cast<NiTriBasedGeom*>(spChild.m_pObject);
                if (abCountculled || !pTriGeom->GetAppCulled()) {
                    iCount += pTriGeom->GetTriCount();
                }
            }
        }
    }
    return iCount;
}

void TES::InitGridCells(const NiPoint3* apPos, bool abFullreload) {
    bool bSaveGameLoading = pSaveLoadGameOLD->GetLoading();
    if ((pDataHandler && !pInteriorCell && !pDataHandler->GetSaveLoad()) || bSaveGameLoading) {
        int32_t iOldGridX = this->GetCurrentGridX();
        int32_t iOldGridY = this->GetCurrentGridY();
        if (!pWorldSpace) {
            SetWorldSpace(static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item));
        }
        if (apPos) {
            iCurrentGridX = FloatToInt(apPos->x) >> 12;
            iCurrentGridY = FloatToInt(apPos->y) >> 12;
        }
        else {
            iCurrentGridX = iOldGridX;
            iCurrentGridY = iOldGridY;
        }
        iCurrentQueuedX = this->GetCurrentGridX();
        iCurrentQueuedY = this->GetCurrentGridY();
        ReloadGridCells(apPos, abFullreload);
        TESWaterListener::RefreshExteriorWaterHeights();
    }
}

bool TES::ClearOldestBufferedCell() {
    TESObjectCELL* pOldestCell = nullptr;
    if (GetInterior()) {
        int32_t iDimension = pGridCellA->GetDimension();
        uint32_t uiGridArea = pGridCellA->GetDimension() * iDimension;
        for (uint32_t i = uExteriorCellBuffer.UInt() - 1; i >= uiGridArea; --i) {
            if (pExteriorBuffer[i] &&
                !IsCellLoaded(pExteriorBuffer[i], true) &&
                !pExteriorBuffer[i]->IsRefListLocked()) {
                pOldestCell = pExteriorBuffer[i];
                pExteriorBuffer[i] = nullptr;
                break;
            }
        }
        if (!pOldestCell) {
            for (int32_t j = TES::uInteriorCellBufferGeneral.UInt() - 1; j >= 0; --j) {
                if (this->pInteriorBuffer[j]) {
                    if (this->pInteriorBuffer[j] != GetInterior()) {
                        pOldestCell = this->pInteriorBuffer[j];
                        this->pInteriorBuffer[j] = nullptr;
                        break;
                    }
                }
            }
        }
        if (!pOldestCell) {
            for (uint32_t k = 0; k < uiGridArea; ++k) {
                if (pExteriorBuffer[k] &&
                    !IsCellLoaded(pExteriorBuffer[k], true) &&
                    !pExteriorBuffer[k]->IsRefListLocked()) {
                    pOldestCell = pExteriorBuffer[k];
                    pExteriorBuffer[k] = nullptr;
                    DetachGridCells();
                    break;
                }
            }
        }
    }
    else {
        for (int32_t m = TES::uInteriorCellBufferGeneral.UInt() - 1; m >= 0; --m) {
            if (this->pInteriorBuffer[m]) {
                pOldestCell = this->pInteriorBuffer[m];
                this->pInteriorBuffer[m] = nullptr;
                break;
            }
        }
        if (!pOldestCell) {
            SortExteriorBuffer();
            int32_t iDimension = pGridCellA->GetDimension();
            uint32_t uiGridArea = pGridCellA->GetDimension() * iDimension;
            for (uint32_t n = uExteriorCellBuffer.UInt() - 1; n >= uiGridArea; --n) {
                if (pExteriorBuffer[n]) {
                    if (!IsCellLoaded(pExteriorBuffer[n], true) && !pExteriorBuffer[n]->IsRefListLocked()) {
                        pOldestCell = pExteriorBuffer[n];
                        pExteriorBuffer[n] = nullptr;
                    }
                    break;
                }
            }
        }
    }
    if (!pOldestCell) {
        return false;
    }
    pDataHandler->UnloadCell(pOldestCell);
    if (!*pExteriorBuffer && GetWorldSpace()) {
        GetWorldSpace()->GetTerrainManager()->UnloadAll();
    }

    return true;
}

void TES::MoveToEndOfExteriorBuffer(TESObjectCELL* apCell) {
    if (apCell) {
        int32_t iIndex = 0;
        int32_t iLast = static_cast<uint32_t>(uExteriorCellBuffer) - 1;
        while (iIndex < iLast) {
            if (apCell == pExteriorBuffer[iIndex]) {
                while (iIndex < iLast) {
                    TESObjectCELL* pNextCell = pExteriorBuffer[iIndex + 1];
                    if (!pNextCell || !pNextCell->GetAttached()) {
                        break;
                    }
                    pExteriorBuffer[iIndex++] = pNextCell;
                }
                pExteriorBuffer[iIndex] = apCell;
                return;
            }
            if (!pExteriorBuffer[iIndex]) {
                return;
            }
            ++iIndex;
        }
    }
}

void TES::SaveWorld(const char* apFileName) {
    if (apFileName) {
        bhkWorldM* pWorld;
        TESObjectCELL* pInterior = TES::pInstance->GetInterior();
        if (pInterior) {
            pWorld = static_cast<bhkWorldM*>(pInterior->GetbhkWorld());
        }
        else {
            pWorld = TESObjectCELL::ExteriorWorld();
        }
        if (pWorld) {
            char cFileName[268];
            BSstrcpy(cFileName, 260, apFileName);
            BSstrcat(cFileName, 260, ".hkx");
            pWorld->SaveSnapshot(cFileName);
        }
    }
}

// Returns a reference to the setting's value; apText is unused.
bool& TES::ShowLoadingMessage(const char* apText) {
    return gs_bShowLoadingAreaMessage_General.Bool();
}

void TES::AddDeadCount(TESActorBase* apObject, int16_t ascount) {
    for (DeathCount* pDead : DeadCount.ItemsUntilNull()) {
        if (pDead->pActorBase == apObject) {
            pDead->usCount += ascount;
            return;
        }
    }
    DeathCount* pDead = static_cast<DeathCount*>(ourMalloc(sizeof(DeathCount)));
    pDead->pActorBase = apObject;
    pDead->usCount = ascount;
    DeadCount.AddHead(pDead);
}

uint16_t TES::GetSaveSize() {
    int16_t sBlockHeaderSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        sBlockHeaderSize = 6;
    }
    uint16_t usSize = 6 * DeadCount.ItemsInList() + (sBlockHeaderSize + 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 50u) {
        usSize += 4;
    }
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
                6274,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize,
                6274,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp"
            );
        }
    }
    return usSize;
}

// Writes the death counts and the cell animation time into the old save buffer; when the save uses blocks, they
// go in a 'BLOK' block whose 16-bit size is filled in at the end.
void TES::SaveGameTES() {
    uint16_t usBlockSize = 0;
    void* pBlockSize = nullptr;
    char* pStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiTag = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&uiTag, sizeof(uiTag));
        pBlockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    BSSimpleList<DeathCount*>* pList = &DeadCount;
    int32_t iCount = pList->ItemsInList();
    pSaveLoadGameOLD->SaveGameDataOLD(&iCount, sizeof(iCount));
    while (pList) {
        DeathCount* pDead = pList->GetItem();
        if (pDead) {
            uint32_t uiFormID = 0;
            if (pDead->pActorBase) {
                uiFormID = pDead->pActorBase->iFormID;
            }
            pSaveLoadGameOLD->SaveNumericID(&uiFormID, 4u);
            pSaveLoadGameOLD->SaveGameDataOLD(&pDead->usCount, sizeof(pDead->usCount));
        }
        pList = pList->GetNext();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 50u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fCellAppTime, sizeof(fCellAppTime));
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
                static_cast<int32_t>(pEnd - pStart),
                pHeader->iFormID,
                pEditorID,
                iFlags,
                6369,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart),
                6369,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pBlockEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        char* pBlockStart = static_cast<char*>(pBlockSize);
        if (pBlockEnd > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp",
                6369
            );
        }
        *static_cast<uint16_t*>(pBlockSize) = static_cast<uint16_t>(pBlockEnd - pBlockStart);
    }
}

void TES::SaveGameBGS(BGSSaveGameBuffer* apSaveGameBuffer) {
    uint32_t uiDeadCount = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (DeathCount* pDead : DeadCount) {
        if (pDead) {
            apSaveGameBuffer->SaveFormID(pDead->pActorBase, 0);
            apSaveGameBuffer->SaveDataEndian(&pDead->usCount, sizeof(pDead->usCount), 0);
            ++uiDeadCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiDeadCount, uiPosition);
    uint32_t uiGridSize = uGridsToLoad.Int();
    apSaveGameBuffer->SaveDataEndian(&uiGridSize, sizeof(uiGridSize), 0);
    for (uint32_t x = 0; x < uiGridSize; ++x) {
        for (uint32_t y = 0; y < uiGridSize; ++y) {
            TESObjectCELL* pCell = pGridCellA->Get(x, y)->pCell;
            uint32_t uiCellID = 0;
            if (pCell && pDataHandler->IsFormIDCreated(pCell->iFormID)) {
                uiCellID = pCell->iFormID;
            }
            apSaveGameBuffer->SaveFormID(uiCellID, 0);
        }
    }
}

void TES::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    uint32_t uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t uiLoop = 0; uiLoop < uiSaveGameCount; ++uiLoop) {
        DeathCount* pDeadCount = static_cast<DeathCount*>(ourMalloc(sizeof(DeathCount)));
        uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
        TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
        pDeadCount->pActorBase = dynamic_cast<TESActorBase*>(pForm);
        apLoadGameBuffer->LoadData(&pDeadCount->usCount, sizeof(pDeadCount->usCount));
        DeadCount.AddHead(pDeadCount);
    }
    uint32_t uiGridSize = 0;
    apLoadGameBuffer->LoadData(&uiGridSize, sizeof(uiGridSize));
    for (uint32_t x = 0; x < uiGridSize; ++x) {
        for (uint32_t y = 0; y < uiGridSize; ++y) {
            TESObjectCELL* pCell = pGridCellA->Get(x, y)->pCell;
            uint32_t uiCellID = 0;
            apLoadGameBuffer->LoadFormID(uiCellID);
            if (pCell && uiCellID) {
                if (pDataHandler->IsFormIDCreated(pCell->iFormID)) {
                    pSaveLoadGame->ClearFormID(uiCellID);
                    pCell->SetFormID(uiCellID, true);
                }
            }
        }
    }
}

uint32_t TES::GetUnusedCachedParticleObject(NiAVObject* apModel, NiPointer<NiAVObject>& arInstance) {
    TES::ParticleObjectCache* pCache;
    arInstance = static_cast<NiAVObject*>(nullptr);
    uint32_t uiResult = 0;
    for (pCache = pParticleCacheHead; pCache; pCache = pCache->pNext) {
        if (pCache->spModel == apModel) {
            uiResult = 1;
            break;
        }
    }
    if (pCache && pCache->pspClones) {
        for (int32_t i = 0; i < 3; ++i) {
            if (pCache->pspClones[i].m_pObject && !pCache->pspClones[i]->m_pkParent) {
                arInstance = pCache->pspClones[i];
                return 2;
            }
        }
    }
    return uiResult;
}

void TES::CleanupTemporaryParticleCache() {
    TES::ParticleObjectCache* pNext;
    for (TES::ParticleObjectCache* pCache = pParticleCacheHead; pCache; pCache = pNext) {
        pNext = pCache->pNext;
        delete pCache;
    }
    pParticleCacheHead = nullptr;
    if (*sBloodParticleDefault.String()) {
        const char* pFileName = sBloodParticleDefault.String();
        pModelLoader->ReleaseFile(pFileName);
    }
    if (*sSplashParticles.String()) {
        const char* pFileName = sSplashParticles.String();
        pModelLoader->ReleaseFile(pFileName);
    }
    if (*sBloodParticleMeleeDefault.String()) {
        const char* pFileName = sBloodParticleMeleeDefault.String();
        pModelLoader->ReleaseFile(pFileName);
    }
    if (*sImpactParticleMetalDefault.String()) {
        const char* pFileName = sImpactParticleMetalDefault.String();
        pModelLoader->ReleaseFile(pFileName);
    }
    if (*sImpactParticleConcreteDefault.String()) {
        const char* pFileName = sImpactParticleConcreteDefault.String();
        pModelLoader->ReleaseFile(pFileName);
    }
    if (*sImpactParticleWoodDefault.String()) {
        const char* pFileName = sImpactParticleWoodDefault.String();
        pModelLoader->ReleaseFile(pFileName);
    }
    if (*sDismemberParticleDefault.String()) {
        const char* pFileName = sDismemberParticleDefault.String();
        pModelLoader->ReleaseFile(pFileName);
    }
    if (*sDismemberRobotParticleDefault.String()) {
        const char* pFileName = sDismemberRobotParticleDefault.String();
        pModelLoader->ReleaseFile(pFileName);
    }
}

void TES::HideUnneededLandPasses() {
    if (uGridsToLoad.Int() == 5) {
        for (int32_t x = 0; x < pGridCellA->iDimension; ++x) {
            for (int32_t y = 0; y < pGridCellA->iDimension; ++y) {
                bool bSetting = false;
                if (TES::bLODFadeInProgress) {
                    if (x == 2) {
                        bSetting = y == 2;
                    }
                }
                else {
                    bSetting = TES::kLandBlendPassMask[x][y];
                }
                SetHideLandBlendPassForGridCell(x, y, bSetting);
            }
        }
    }
}

void TES::HideFarLandNormalMaps() {
    if (uGridsToLoad.UInt() == 5) {
        for (int32_t x = 0; x < pGridCellA->iDimension; ++x) {
            for (int32_t y = 0; y < pGridCellA->iDimension; ++y) {
                SetHideLandNormalMapForGridCell(x, y, TES::kLandNormalPassMask[x][y]);
            }
        }
    }
}

bool TES::QCollisionDisableFlag() {
    return bCollisionDisable;
}

// Items and actors take their own fade multiplier class; every other form takes the object class.
ENUM_LOD_MULT TES::GetLODMult(TESObject* apObj) {
    switch (apObj->GetFormType()) {
        case ARMO_ID:
        case BOOK_ID:
        case CLOT_ID:
        case INGR_ID:
        case MISC_ID:
        case WEAP_ID:
        case AMMO_ID:
        case KEYM_ID:
        case ALCH_ID:
        case COBJ_ID:
        case IMOD_ID:
        case CHIP_ID:
        case CCRD_ID:
        case CMNY_ID: return LOD_MULT_ITEMS;
        case NPC__ID:
        case CREA_ID: return LOD_MULT_ACTORS;
        default: return LOD_MULT_OBJECTS;
    }
}

void TES::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// Pushes the LOD fade settings into BSFadeNode's per-class multipliers; every class but items and actors takes the
// object multiplier.
void TES::InitFadeOutMultipliers() {
    BSFadeNode::SetFadeOutMultiplier(LOD_MULT_ITEMS, fLODFadeOutMultItems.Float());
    BSFadeNode::SetFadeOutMultiplier(LOD_MULT_ACTORS, fLODFadeOutMultActors.Float());
    float fObjectMult = fLODFadeOutMultObjects.Float();
    BSFadeNode::SetFadeOutMultiplier(LOD_MULT_NONE, fObjectMult);
    BSFadeNode::SetFadeOutMultiplier(LOD_MULT_OBJECTS, fObjectMult);
    BSFadeNode::SetFadeOutMultiplier(LOD_MULT_TREES, fObjectMult);
    BSFadeNode::SetFadeOutMultiplier(LOD_MULT_LANDSCAPE, fObjectMult);
    BSFadeNode::SetFadeOutMultiplier(LOD_MULT_RENDEREDMENU, fObjectMult);
    BSFadeNode::SetFadeOutMultiplier(LOD_MULT_DIST_OVERRIDE, fObjectMult);
}

void TES::SetCollisionDisableFlag(bool abTF) {
    TES::bCollisionDisable = abTF;
}

void TES::RangedNodeVisibilityChangedCB(NiNode* apNode, bool abVisible) {
    if (abVisible) {
        TESObjectREFR::AddMasterParticleAddonNodes(apNode);
    }
    else {
        TESObjectREFR::RemoveMasterParticleAddonNodes(apNode);
    }
}

bool TES::GetSelectivePurgeUnusedOnFastTravel() {
    return gs_bSelectivePurgeUnusedOnFastTravel_BackgroundLoad;
}

void TES::InitSDM() {
    BSFaceGenAnimationData::InitTag();
    BSFaceGenModelExtraData::InitTag();
    BSFaceGenBaseMorphExtraData::InitSDM();
    BGSAddonNodeSoundHandleExtra::InitTag();
    BSSplatterExtraData::InitTag();
    BSCustomSplatterExtraData::InitTag();
}

void TES::KillSDM() {
    BSFaceGenAnimationData::KillSDM();
    BSFaceGenModelExtraData::KillSDM();
    BSFaceGenBaseMorphExtraData::KillSDM();
    BGSAddonNodeSoundHandleExtra::KillSDM();
    BSSplatterExtraData::KillSDM();
}

void BSAudio::EndAllMovingSounds() {}

void BSCriticalSection::Enter(const char* apName) {
    Lock(apName);
}

void BSCriticalSection::Leave() {
    Unlock();
}

BSPerformanceTimer::BSPerformanceTimer(
    const char* apSystem,
    const char* apName,
    bool abDisplayPerFrame,
    bool abGlobalReset
) {}

void BSPerformanceTimer::Start() {}

void BSPerformanceTimer::Stop() {}

// m_to and kLength are reached by name rather than as neighbours of m_from, as the layout is not the exe's.
void bhkPickData::CalcTo() {
    if (QLength()) {
        m_to.setAdd4(m_from, hkLength);
    }
}

void bhkPickData::GetTo(NiPoint3& arTo) {
    HK2NI(arTo, GethkTo());
}

bool bhkPickData::QLength() {
    return hkLength.equals3(hkVector4::getZero(), 0.001f) == 0;
}

hkVector4* bhkPickData::GethkFrom() {
    return &m_from;
}

void bhkPickData::GetFrom(NiPoint3& arFrom) {
    HK2NI(arFrom, *GethkFrom());
}

void bhkRagdollController::SetRagdollAnim(bool abEnable) {
    bhkRagdollController::bRagdollAnim.Bool() = abEnable;
}

const NiPoint3& BSMultiBoundAABB::QCenter() const {
    return Center;
}

NiTPointerList<NiPointer<BSMultiBound>>& BSMultiBoundRoom::QJoinedMultiBoundList() {
    return kJoinedMultiBoundList;
}

const NiTPointerList<BSOcclusionPlane*>& BSMultiBoundRoom::GetOccluders() const {
    return lOccluderList;
}

void BSOcclusionPlane::ResetCullResult() {
    kCullResult = BSMultiBoundShape::BS_CULL_UNTESTED;
}

BSPortalSharedNode* BSPortal::QSharedGeometryNode() {
    return spSharedGeometry;
}

const NiTPointerList<NiPointer<BSMultiBoundRoom>>& BSPortalGraph::QRooms() const {
    return lMultiBoundRoomList;
}

BSMultiBoundRoom* BSPortalGraph::QRoomRoot() {
    return static_cast<BSMultiBoundRoom*>(spMultiBoundRoomRoot.m_pObject);
}

NiTPointerList<BSOcclusionPlane*>& BSPortalGraph::QOccluderList() {
    return lOccluderList;
}

BSCompoundFrustum* BSPortalSharedNode::QCompoundFrustum() {
    return pCompoundFrustum;
}

void BSRangeNode::SetVisibilityChangedCB(void (*apVisibilityChangedCB)(NiNode*, bool)) {
    BSRangeNode::pVisibilityChangedCB = apVisibilityChangedCB;
}

float BSShaderManager::GetTimer(bool bDelta) {
    return fTimerValues[bDelta];
}

// Returns the configured particle cap for a master particle system.
uint16_t BGSAddonNode::GetMasterParticleCap() {
    return Data.usMasterParticleCap;
}

BSTempNode::BSTempNode(float afTime) : NiNode(0), fTime(afTime) {}

BSTempNode::~BSTempNode() {}

uint32_t GridArray::GetDimension() {
    return iDimension;
}

void ModelLoader::ReleaseFile(const char* apModelFileName) {
    ReleaseModel(apModelFileName, true, 1);
}

float Sky::GetWindSpeed() {
    return fWindSpeed;
}

float Sky::GetWindAngle() {
    return fWindAngle;
}

TESWeather* Sky::GetCurrentWeather() {
    return pCurrentWeather;
}

TESWeather* Sky::GetLastWeather() {
    return pLastWeather;
}

bool BSTreeManager::GetUseCanopyShadows() {
    return BSTreeManager::bUseCanopyShadows;
}

bool TESObjectLAND::HasVertices() {
    return pLoadedData && pLoadedData->ppVertices;
}

// An interior cell has its own Havok world in its extra data; exterior cells share the exterior world.
bhkWorld* TESObjectCELL::GetbhkWorld() {
    if (GetInterior()) {
        return ExtraData.GetbhkWorld();
    }
    return ExteriorWorld();
}

void TESObjectCELL::IncLoadedRefsWithVisibleDistantCount() {
    ++sNumLoadedRefsWithVisibleDistant;
}

// Returns the cell's base editor ID: calls TESForm::GetFormEditorID directly, not through the vtable.
const char* TESObjectCELL::GetCellName() {
    return TESForm::GetFormEditorID();
}

void TESObjectCELL::SetNodeCulled(CELL_CULL_NODE aiNode, bool abVal) {
    if (abVal) {
        iNodeCullFlags |= 1u << aiNode;
    }
    else {
        iNodeCullFlags &= ~(1u << aiNode);
    }
}

const NiPoint2& TESObjectTREE::GetBillboardSize() {
    return BillboardSize;
}

QUEST_DATA* TESQuest::GetData() {
    return &data;
}

NiTPointerList<PlaceableWaterGroup*>* TESWaterSystem::GetPlaceableWaterGroupList() {
    return &PlaceableWaterGroupList;
}

PlaceableWaterGroup* TESWaterSystem::GetLODWaterGroup() {
    return pLODWaterGroup;
}

float TESWorldSpace::GetLODWaterHeight() {
    return fLODWaterHeight;
}

NiAVObject* NiPick::Record::GetAVObject() {
    return m_spObject;
}

NiPick::Record* NiPick::Results::GetAt(uint32_t uiIndex) const {
    return NiTPrimitiveArray<Record*>::GetAt(uiIndex);
}

void NiPick::SetTarget(NiAVObject* pObject) {
    m_spRoot = pObject;
}

const NiCamera* NiCullingProcess::GetCamera() const {
    return m_pkCamera;
}

const NiFrustumPlanes& NiCullingProcess::GetFrustumPlanes() const {
    return m_kPlanes;
}

uint16_t NiGeometryData::GetVertexCount() const {
    return m_usVertices;
}

NiPoint3* NiGeometryData::GetVertices() const {
    return m_pkVertex;
}

void NiNode::RemoveAllChildren() {
    m_kChildren.RemoveAll();
}
