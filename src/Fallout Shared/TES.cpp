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
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkConvert.h"
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
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BSCustomSplatterExtraData.h"
#include "Fallout Shared/BSSplatterExtraData.h"
#include "Fallout Shared/BSTaskThread.h"
#include "Fallout Shared/BSTempNode.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExteriorCellLoader.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenBaseMorphExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenModelExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenMorphData.h"
#include "Fallout Shared/FOScriptFunctions.h"
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
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
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
#include "Fallout Shared/TempEffects/BSTempEffectSimpleDecal.h"
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
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FORenderedTerminal.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
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
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "SDK/MSVC/time.h"

#include <cstring>
#include <filesystem>
#include <stdint.h>
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

using data_11C3F0C_t = uint32_t[6];
static data_11C3F0C_t data_11C3F0C{};

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
SettingT<INISettingCollection> sFailureMessage("sFailureMessage:LANGUAGE",
	"No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sSysUtil_Retry("sSysUtil_Retry:LANGUAGE",
	"No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sSysUtil_AutoSaveWarning("sSysUtil_AutoSaveWarning:LANGUAGE",
	"No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sSysUtil_GameDataInsufficientSpace("sSysUtil_GameDataInsufficientSpace:LANGUAGE",
	"No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sSysUtil_GameDataCorrupt("sSysUtil_GameDataCorrupt:LANGUAGE",
	"No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sSysUtil_SaveDataCreateNew("sSysUtil_SaveDataCreateNew:LANGUAGE",
	"No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sSysUtil_SaveDataInsufficientSpace("sSysUtil_SaveDataInsufficientSpace:LANGUAGE",
	"No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sSysUtil_DiscEject("sSysUtil_DiscEject:LANGUAGE",
	"No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> uExteriorCellBuffer("uExterior Cell Buffer:General",
	uGridsToLoad.UInt() * uGridsToLoad.UInt() + uGridsToLoad.UInt() + uGridsToLoad.UInt() + 1);
SettingT<INISettingCollection> gs_bShowLoadingAreaMessage_General("bShowLoadingAreaMessage:General", false);
SettingT<INISettingCollection>
	gs_bSelectivePurgeUnusedOnFastTravel_BackgroundLoad("bSelectivePurgeUnusedOnFastTravel:BackgroundLoad", false);
SettingT<INISettingCollection> sSysUtil_DirtyDisc("sSysUtil_DirtyDisc:LANGUAGE",
	"No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sThumbstick("sThumbstick:LANGUAGE", "No Default string.  Must be loaded from INIFile");
SettingT<INISettingCollection> sDPad("sDPad:LANGUAGE", "No Default string.  Must be loaded from INIFile");
SettingT<RendererSettingCollection> fLODFadeOutMultActors("fLODFadeOutMultActors:LOD", 2.5f);
SettingT<RendererSettingCollection> fLODFadeOutMultItems("fLODFadeOutMultItems:LOD", 3.0f);
SettingT<RendererSettingCollection> fLODFadeOutMultObjects("fLODFadeOutMultObjects:LOD", 5.0f);

// On x64 the shared model-data getter's class (TESActorBaseData) has its field at another offset than m_usVertices, so
// the field is read directly.
uint16_t NiGeometry::GetVertexCount() {
#if FNVD_64BIT
	return this->m_spModelData.operator->()->m_usVertices;
#else
	return static_cast<TESActorBaseData*>(static_cast<void*>(this->m_spModelData.operator->()))
		->TESActorBaseData::GetFatigue();
#endif
}

void MakeNewGeomData(NiAVObject* apObject) {
	NiPointer<NiObject> spTempObject(0);
	NiTriStrips* pGeometry = 0;
	if (apObject) {
		NiGeometry* pgeom = apObject->IsGeometry();
		if (pgeom) {
			NiPointer<NiGeometryData> spdata(pgeom->GetModelData());
			if (spdata.m_pObject) {
				if (spdata.m_pObject->GetConsistency() || !spdata.m_pObject->m_pkVertex) {
					NiGeometryData* v4 = spdata.m_pObject;
					pgeom->SetModelData(v4);
				} else {
					spdata.m_pObject->CreateDeepCopy(spTempObject);
					spdata.operator=(static_cast<NiGeometryData*>(spTempObject.m_pObject));
					NiGeometryData* v3 = spdata.m_pObject;
					pgeom->SetModelData(v3);
				}
			}
		} else {
			NiNode* pnode = apObject->IsNode();
			if (pnode) {
				uint32_t I = pnode->GetArrayCount();
				while (I--) {
					NiAVObject* v2 = pnode->GetAt(I);
					MakeNewGeomData(v2);
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
	arhkQ.m_vec.operator()(0) = fX;
	float fY = arNiQ.m_fY;
	arhkQ.m_vec.operator()(1) = fY;
	float fZ = arNiQ.m_fZ;
	arhkQ.m_vec.operator()(2) = fZ;
	float fW = arNiQ.m_fW;
	arhkQ.m_vec.operator()(3) = fW;
	return arhkQ;
}

void Init() {
	HavokStreambufFactory* v0;
	HavokError* v1;
	char Buffer[268];
	AutoMemContext apMemory(MEM_CONTEXT_HAVOK, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Havok\\HavokStream.cpp", 172);
	bhkWorld::InitSystem();
	hkMonitorStream::init();
	HavokError* v3 = static_cast<HavokError*>(hkReferencedObject::newObject(sizeof(HavokError)));
	if (v3) {
		v1 = ::new (v3) HavokError;
	} else {
		v1 = 0;
	}
	hkSingleton<hkError>::replaceInstance(v1);
	HavokStreambufFactory* v2 =
		static_cast<HavokStreambufFactory*>(hkReferencedObject::newObject(sizeof(HavokStreambufFactory)));
	if (v2) {
		v0 = ::new (v2) HavokStreambufFactory;
	} else {
		v0 = 0;
	}
	hkSingleton<hkFileSystem>::replaceInstance(v0);
	// GetCurrentDirectoryA(260, Buffer): the buffer is left as it is when the folder is unknown or does not fit.
	std::error_code kError;
	const std::string kCurrentDirectory = std::filesystem::current_path(kError).string();
	if (!kError && kCurrentDirectory.size() < 260) {
		std::strcpy(Buffer, kCurrentDirectory.c_str());
	}
	v1->SetRootDir(Buffer);
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
	  iCurrentQueuedX(0), iCurrentQueuedY(0), pInteriorCell(nullptr), pInteriorBuffer(nullptr),
	  pExteriorBuffer(nullptr), iTempInteriorBufferSize(0), iTempExteriorBufferSize(0), iSaveGridX(0), iSaveGridY(0),
	  bCollisionBoxes(false), bRunningCellTests(false), bRunningCellTests2(false), bRunningSubmergedObjectTest(false),
	  pfnTACCallbackFunc(nullptr), pTACCallbackData(nullptr), pTACRegionFilter(nullptr), bShowLANDborders(false),
	  padding61{}, pWaterSystem(nullptr), pSky(nullptr), iTotalToLoad(0), iLoaded(0), bDisablePercentageUpdate(false),
	  bUpdateGridString(false), padding7E{}, fCell_delta_x(0.0f), fCell_delta_y(0.0f), pWorldSpace(nullptr),
	  pParticleCacheHead(nullptr), bFadeWhenLoading(false), bAllowUnusedPurge(false), paddingB6{},
	  iPlaceableWaterCount(0), pNavMeshInfoMap(nullptr) {
	AutoMemContext kMemContext(MEM_CONTEXT_GAME_OVERHEAD);
	NiObjectNET::SetDefaultCopyType(NiObjectNET::COPY_EXACT);
	Init();
	bhkWorld::SetUpdateType(gs_iUpdateType_HAVOK.operator uint32_t());
	bhkWorld::SetRecFunction(bhkWorldNS::OA_ADD, &TESObjectREFR::FilteredAddObject);
	bhkWorld::SetAddBipedWhenKeyframed(bAddBipedWhenKeyframed_HAVOK.Bool());
	bhkWorld::SetDebrisMaxVelocity(gs_fDebrisMaxVelocity.Float());
	bhkWorld::SetDebrisMinExtent(gs_fDebrisMinExtent.Float());
	bhkWorld::SetChaseDeltaMult(gs_fChaseDeltaMult_HAVOK.Float());
	bhkWorld::SetMaxPickTimeNormal(gs_fMaxPickTime_HAVOK.Float());
	bhkWorld::SetMaxPickTimeVATS(gs_fMaxPickTimeVATS_HAVOK.Float());
	bhkWorld::SetEntityBatchRemoveRate(static_cast<uint32_t>(gs_iEntityBatchRemoveRate_HAVOK.Int()));
	bhkCharacterListener::SetMoveLimitMass(gs_fMoveLimitMass_HAVOK.operator float());
	bhkCharacterController::SetMultipleStepSpeed(fCharacterControllerMultipleStepSpeed.Float() *
		fCharacterControllerMultipleStepSpeed.Float());
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
	if (uExteriorCellBuffer.operator uint32_t() < static_cast<uint32_t>(iGrids * iGrids + iGrids + iGrids + 1)) {
		uExteriorCellBuffer.operator=(iGrids + iGrids * iGrids + iGrids + 1);
	}
	if (!TES::uInteriorCellBufferGeneral.operator uint32_t()) {
		TES::uInteriorCellBufferGeneral.operator=(1);
	}
	pInteriorBuffer =
		static_cast<TESObjectCELL**>(ourMalloc(TES::uInteriorCellBufferGeneral.UInt() * sizeof(TESObjectCELL*)));
	pExteriorBuffer = static_cast<TESObjectCELL**>(ourMalloc(uExteriorCellBuffer.UInt() * sizeof(TESObjectCELL*)));
	for (uint32_t n = 0; n < TES::uInteriorCellBufferGeneral.operator uint32_t(); ++n) {
		pInteriorBuffer[n] = nullptr;
	}
	for (uint32_t n = 0; n < uExteriorCellBuffer.operator uint32_t(); ++n) {
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
	ClearAllCells(0, 0);
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
	NiObjectGarbage::ClearAll(0);
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
	TESWorldSpace* pWorldSpace;
	if (apWorldSpace) {
		pWorldSpace = apWorldSpace;
	} else {
		pWorldSpace = GetWorldSpace();
	}
	return pWorldSpace && pWorldSpace->GetMapNameForLocation(aMapName, aWorldLoc);
}

void TES::RunCellTest(uint32_t aiOptions) {
	bRunningCellTests = 1;
	PlayerCharacter::SetGodMode(1);
	InitializationFlow("Running Cell Test");
	Interface::EmergencyCloseAllMenusAndBreakStuff();
	uint32_t icellTestStartTime = BSGetTickCount();
	for (BSSimpleList<TESWorldSpace*>* plist = pDataHandler->GetWorldSpaceList(); plist && !plist->IsEmpty();
		plist = plist->m_pkNext) {
		TESWorldSpace* pworld = plist->m_item;
		if (pworld) {
			SetWorldSpace(pworld);
			int32_t iminX = static_cast<int32_t>(pworld->GetMinX()) >> 12;
			int32_t iminY = static_cast<int32_t>(pworld->MinimumCoords.Y) >> 12;
			int32_t imaxX = static_cast<int32_t>(pworld->MaximumCoords.X) >> 12;
			int32_t imaxY = static_cast<int32_t>(pworld->MaximumCoords.Y) >> 12;
			const char* pworldSpaceName = pworld->GetFullName();
			if (!pworldSpaceName || !BSstrlen(pworldSpaceName)) {
				pworldSpaceName = pworld->GetFormEditorID();
			}
			InitializationFlow("Starting world %08X %s with bounds (%i,%i) to (%i,%i)", pworld->iFormID,
				pworldSpaceName, iminX, iminY, imaxX, imaxY);
			for (int32_t x = iminX; x < imaxX; x += uGridsToLoad.Int()) {
				for (int32_t y = iminY; y < imaxY; y += uGridsToLoad.Int()) {
					TESObjectCELL* pcell = pworld->GetCellFromCellCoord(x, y);
					if (!pcell) {
						pcell = pworld->LoadCell(x, y);
					}
					if (pcell) {
						TestCell(pcell, aiOptions, icellTestStartTime);
					}
				}
			}
		}
	}
	pDataHandler->SortInteriorCells();
	uint32_t iinteriorCount = pDataHandler->GetInteriorCellCount();
	for (uint32_t i = 0; i < iinteriorCount; ++i) {
		TESObjectCELL* pcell_2 = pDataHandler->GetInteriorCellByIndex(i);
		TestCell(pcell_2, aiOptions, icellTestStartTime);
	}
	bRunningCellTests2 = 0;
}

TESLandTexture* TES::GetLandTexture(const NiPoint3& aCoord) {
	TESLandTexture* prettex = 0;
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
	TESObjectCELL* ptestcell = GetGridCellCell(ix >> 12, iy >> 12);
	if (ptestcell) {
		TESObjectLAND* Land = ptestcell->GetLand();
		prettex = Land->GetMainTexture(aCoord);
		if (prettex) {
			pGetLandTexLastTex = prettex;
			iGetLandTexLastX = ix - ix % 128;
			iGetLandTexLastY = iy - iy % 128;
		}
	}
	return prettex;
}

HavokMaterialType TES::GetLandMaterialType(const NiPoint3& aCoord) {
	int32_t v4 = 2;
	TESLandTexture* landTexture = GetLandTexture(aCoord);
	if (landTexture) {
		return static_cast<HavokMaterialType>((int32_t)landTexture->GetHavokMaterialType());
	}
	return static_cast<HavokMaterialType>(v4);
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
	TESObjectCELL* pcell = pInteriorCell;
	bool* toggle = &TESObjectREFR::bPrimitivesOn.Bool();
	// Tests the whole stored byte: any nonzero byte toggles to 0, not only 1.
	*toggle = static_cast<uint8_t>(TESObjectREFR::bPrimitivesOn.Bool()) == 0;
	bool bshow = *toggle;
	if (pcell) {
		pcell->ShowPrimitives(bshow);
	} else {
		pGridCellA->ShowPrimitives(bshow);
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
	case 4:
		return !abIgnorebuffered;
	case 5:
	case 6:
		return true;
	default:
		return false;
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
	TESWorldSpace* worldSpace;
	int32_t iX = (int32_t)(aiCellX - (iCurrentGridX - (((&uGridsToLoad)->UInt()) >> 1)));
	int32_t iY = (int32_t)(aiCellY - (iCurrentGridY - (((&uGridsToLoad)->UInt()) >> 1)));
	TESObjectCELL* pCell = nullptr;
	if ((((((uint32_t)iX) < ((&uGridsToLoad)->UInt())) && (((uint32_t)iY) < ((&uGridsToLoad)->UInt()))) && (iX >= 0)) &&
		(iY >= 0)) {
		pCell = pGridCellA->Get(iX, iY)->pCell;
	}
	if (!pCell) {
		if (GetWorldSpace()) {
			worldSpace = GetWorldSpace();
			return pDataHandler->GetCellFromCellCoord(aiCellX, aiCellY, worldSpace, 0);
		}
	}
	return pCell;
}

bool TES::PickWorldDistance(NiPick* apPick, const NiPoint3& aOrigin, const NiPoint3& aDir, float afDistance,
	char acNodeFlags) {
	TESObjectCELL* interior;
	TESObjectCELL* pCell;
	uint32_t aiY;
	uint32_t aiX;
	uint8_t v12 = 0;
	if (apPick) {
		if ((uint8_t)acNodeFlags) {
			apPick->SetTarget(nullptr);
			apPick->PickObjects(NiPoint3_ZERO, NiPoint3_ZERO, 0);
			if (TES::pInstance->TES::GetInterior()) {
				interior = TES::pInstance->TES::GetInterior();
				return (bool)((uint8_t)interior->PickWorldDistance(apPick, aOrigin, aDir, afDistance, acNodeFlags));
			} else {
				for (aiX = 0; aiX < ((&uGridsToLoad)->UInt()); ++aiX) {
					for (aiY = 0; aiY < ((&uGridsToLoad)->UInt()); ++aiY) {
						pCell = GetGridCell(aiX, aiY)->pCell;
						if (pCell) {
							if (pCell->PickWorldDistance(apPick, aOrigin, aDir, afDistance, acNodeFlags)) {
								v12 = 1;
							}
						}
					}
				}
			}
		}
	}
	return (bool)v12;
}

bool TES::PickWaterDistance(NiPick* apPick, const NiPoint3& aOrigin, const NiPoint3& aDir, float afDistance) {
	TESObjectCELL* pInteriorCell;
	TESObjectCELL* pCell;
	uint32_t j;
	uint32_t i;
	char v11 = 0;
	if (apPick) {
		if (TES::pInstance->TES::GetInterior()) {
			pInteriorCell = TES::pInstance->TES::GetInterior();
			return pInteriorCell->PickWaterDistance(apPick, aOrigin, aDir, afDistance);
		} else {
			for (i = 0; i < ((&uGridsToLoad)->UInt()); ++i) {
				for (j = 0; j < ((&uGridsToLoad)->UInt()); ++j) {
					pCell = GetGridCell(i, j)->pCell;
					if (pCell) {
						if (pCell->PickWaterDistance(apPick, aOrigin, aDir, afDistance)) {
							v11 = 1;
						}
					}
				}
			}
		}
	}
	return v11;
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
	} else if (apPos) {
		int32_t iDataX = apCell->GetDataX();
		int32_t iPosX = FloatToInt(apPos->x) >> 12;
		iGridX = iDataX - (iPosX - static_cast<int32_t>(uGridsToLoad.UInt() >> 1));
		int32_t iDataY = apCell->GetDataY();
		int32_t iPosY = FloatToInt(apPos->y) >> 12;
		iGridY = iDataY - (iPosY - static_cast<int32_t>(uGridsToLoad.UInt() >> 1));
	} else {
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
	} else {
		cDistance = static_cast<uint8_t>(std::abs(iOffsetY) * 10 + 10);
	}

	if (cDistance <= 20) {
		cDistance = 20;
	} else {
		cDistance = 40;
	}

	switch (cDistance) {
	case 10:
		elastPriority = IO_TASK_PRIORITY_CRITICAL;
		break;
	case 20:
		elastPriority = IO_TASK_PRIORITY_VERY_HIGH;
		break;
	case 30:
		elastPriority = IO_TASK_PRIORITY_HIGH;
		break;
	case 40:
		elastPriority = IO_TASK_PRIORITY_MID;
		break;
	case 50:
		elastPriority = IO_TASK_PRIORITY_LOW;
		break;
	default:
		break;
	}

	plastCell = apCell;
	return elastPriority;
}

NavMeshInfoMap* TES::GetNavMeshInfoMap() {
	if (!pNavMeshInfoMap && pDataHandler && !pDataHandler->GetSaveLoad()) {
		void* pMemory = ourMalloc(sizeof(NavMeshInfoMap));
		pNavMeshInfoMap = pMemory ? ::new (pMemory) NavMeshInfoMap : nullptr;
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
void TES::CreateTextureImage(const char* apfilename, NiPointer<NiSourceTexture>& aspSourceTexture, bool abNoFileOK,
	bool abArchiveOnly) {
	FileFinder::LOOKIN_FLAGS eLookIn = FileFinder::SKIP_NONE;
	// Every archive type the finder knows; 0xFFFF is passed rather than ARCHIVE_TYPE_ALL, as the game does.
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
	} else if (!abNoFileOK) {
		Warning("MODELS: TES::CreateTextureImage unable to create image for \"%s\".\r\n", apfilename);
	}
	pspUnused->~NiPointer<NiRefObject>();
}

// The water height at arPos: apCell's (or, with no cell, the interior's, else the worldspace cell's there) when the
// cell has water at arPos, else the first neighbouring cell that does, else the cell's own height. Outside any loaded
// cell it is the worldspace's LOD water height; an interior cell without water at arPos has none (-FLT_MAX). The branch
// for a null cell before the neighbour search cannot be reached, because every path that reaches it has set apCell
// non-null; it is kept deliberately, as the game has it.
float TES::GetWaterHeight(const NiPoint3& arPos, TESObjectCELL* apCell) {
	// -FLT_MAX.
	constexpr float kNoWater = -3.40282346638528859812e+38f;
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
	} else if (GetInterior()) {
		float fInteriorHeight;
		GetInterior()->GetWaterHeight(arPos, fInteriorHeight);
		return fInteriorHeight;
	} else {
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
	} else {
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

// Constructs and destroys an AutoMemContext on the stack and ignores the constructor's result, as the game does.
void TES::InitializeWater() {
	alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
	AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
	::new (aucContext)
		AutoMemContext(MEM_CONTEXT_WATER, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 646);
	void* pMemory = ourMalloc(sizeof(TESWaterSystem));
	pWaterSystem = pMemory ? ::new (pMemory) TESWaterSystem : nullptr;
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
		} else {
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
			} else if (pCell && pCell->IsLoaded()) {
				pGridCellA->AttachToWorld(pObjRoot, uiX, uiY);
				pCell->AttachModels(false);
			}
		}
	}

	if (TESObjectCELL::ExteriorWorld()) {
		if (abDisable) {
			TESObjectCELL::ExteriorWorld()->StopBatchingRemovals(false);
		} else {
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

	NiUpdateData kUpdateData;
	kUpdateData.~NiUpdateData();
	::new (&kUpdateData) NiUpdateData(0.0f, false, false);
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

// A reference may get 3D unless it is deleted or disabled. The base form's type is read first and never used, as the
// game does.
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
		Projectile* pproj = static_cast<Projectile*>(apRef);
		pproj->Initialize();
		return;
	}

	if (apRef->IsExplosion()) {
		Explosion* pexplosion = static_cast<Explosion*>(apRef);
		pexplosion->Initialize();
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
	} else {
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
		} else {
			apCell->Load3D();
			BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->AddObject(pModel);
			if (apRef->IsActor()) {
				BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->AddShadowLight(pModel);
			}

			if (apRef->GetVisibleDistant()) {
				apCell->IncLoadedRefsWithVisibleDistantCount();
			}

			bool battach = true;
			if (apRef->GetObjectReference()->GetFormType() == PWAT_ID) {
				BGSPlaceableWater* pwat = static_cast<BGSPlaceableWater*>(apRef->GetObjectReference());
				if (pwat->IsAutomaticWater()) {
					battach = false;
				}
			}

			if (battach) {
				apCell->AttachReference3D(apRef, false);
				apRef->UpdateReference3D();
			}

			if (apRef->GetDestructible() && apRef->QUnderwater()) {
				TESObjectREFR::RemoveMasterParticleAddonNodes(pModel);
			} else if (apRef->GetLoadedDataFlag(2) && (apCell->GetAttached() || apCell->IsAttaching())) {
				TESObjectREFR::AddMasterParticleAddonNodes(pModel);
			}

			if (apRef->IsActor()) {
				Actor* pactor = static_cast<Actor*>(apRef);
				if (pactor) {
					pactor->UpdateCurrentDebugGeometry();
				}
			}

			if (type == LIGH_ID) {
				const NiPoint3& kAngle = apRef->GetAngleOnReference();
				TESObjectLIGH* pLight = static_cast<TESObjectLIGH*>(apRef->GetObjectReference());
				pLight->FixGravity(kAngle);
			} else {
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
		TESObjectTREE* ptree = dynamic_cast<TESObjectTREE*>(apRef->GetObjectReference());
		if (ptree && BSTreeManager::GetUseCanopyShadows()) {
			NiPoint2 kbillboardsize = ptree->GetBillboardSize();
			if (kbillboardsize.x > 200.0 && kbillboardsize.y > 200.0) {
				double fShadowRadius = ptree->GetShadowRadius();
				uint32_t uiRadius = static_cast<uint32_t>(static_cast<int64_t>(apRef->GetScale() * fShadowRadius));
				apCell->PlaceCanopyShadow(apRef->GetLocationOnReference(), uiRadius, 1.0f);
			}
		}
	}

	if (apRef->IsActor()) {
		Actor* pactor = static_cast<Actor*>(apRef);
		pactor->UpdateAlpha();
		if (apRef != pPlayer) {
			uint32_t uiID = apRef->GetFormID();
			SetReferenceIDOnScenegraph(pactor->TESObjectREFR::Get3D(), uiID);
		}
	}
}

int32_t TES::UpdateCurrentGridCell(const NiPoint3* apPos, bool abAllowPartialRemovals) {
	NiPoint2 v27;
	(&listLastLoadedExteriors)->RemoveAll();
	HideFarLandNormalMaps();
	if (TES::bLODFadeInProgress && !BSShaderManager::bLODFadeInProgress && !pInteriorCell) {
		TES::bLODFadeInProgress = 0;
		HideUnneededLandPasses();
	}
	if (pSaveLoadGame->GetSaveGameLoading()) {
		spLoadedAreaBound.m_pObject->Clear();
	}
	GridCellArray* someFloat = pGridCellA;
	TESObjectCELL* CurrentCell = GetCurrentCell();
	spLoadedAreaBound.m_pObject->Update(CurrentCell, someFloat);
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
		ShowLoadingMenu(1, pWorldSpace, 0);
		InitGridCells(apPos, 0);
		return 1;
	} else {
		int32_t x = FloatToInt(apPos->x);
		int32_t y_2 = FloatToInt(apPos->y);
		int32_t Number = x - ((this->GetCurrentGridX() << 12) + 2048);
		int32_t v41 = y_2 - ((this->GetCurrentGridY() << 12) + 2048);
		float floadDistance = 4096.0f;
		if (uGridsToLoad.Int() == 3) {
			floadDistance = 3072.0f;
		}
		fCell_delta_x = floadDistance - static_cast<double>(std::abs(Number));
		fCell_delta_y = floadDistance - static_cast<double>(std::abs(v41));
		bool bsavegameLoading = pSaveLoadGame->GetSaveGameLoading();
		if (!(fCell_delta_x > 0.0) || (!(fCell_delta_y > 0.0) || bsavegameLoading)) {
			BSTaskThread::SetUpdateSleepEnabled(0);
			bool bmenudisplayed = Interface::GetLoadingMenuVisible();
			if (!bmenudisplayed) {
				if (pExteriorCellLoader->QueuedCellCount()) {
					if (!pSaveLoadGame->GetSaveGameLoading()) {
						ShowLoadingMessage("Loading Area (queued cells)...");
						bmenudisplayed = 1;
					}
				}
			}
			int32_t inewX = x >> 12;
			int32_t inewY = y_2 >> 12;
			if (pSaveLoadGame->GetSaveGameLoading() && pSaveLoadGame->GetPositioningPlayerCharacter()) {
				inewX = this->GetCurrentGridX();
				inewY = this->GetCurrentGridY();
			}
			int32_t ideltaX = inewX - this->GetCurrentGridX();
			int32_t ideltaY = inewY - this->GetCurrentGridY();
			iCurrentGridX = inewX;
			iCurrentGridY = inewY;
			iCurrentQueuedX = this->GetCurrentGridX();
			iCurrentQueuedY = this->GetCurrentGridY();
			BSShaderManager::kOldGridArrayCenter = BSShaderManager::kGridArrayCenter;
			float v25 = (static_cast<double>(this->GetCurrentGridY()) + 0.5) * 4096.0;
			float v24 = (static_cast<double>(this->GetCurrentGridX()) + 0.5) * 4096.0;
			NiPoint2* v5 = ::new (&v27) NiPoint2(v24, v25);
			float y = v5->y;
			BSShaderManager::kGridArrayCenter.x = v5->x;
			BSShaderManager::kGridArrayCenter.y = y;
			if (BSShaderManager::kOldGridArrayCenter.operator==(NiPoint2::ZERO) ||
				(std::abs(ideltaX) > 1 || std::abs(ideltaY) > 1)) {
				BSShaderManager::kOldGridArrayCenter = BSShaderManager::kGridArrayCenter;
			}
			BSShaderManager::fGriddArrayLerpStart = BSShaderManager::GetTimer(0);
			TES::bLODFadeInProgress = 1;
			BSShaderManager::bLODFadeInProgress = 1;
			HideFarLandNormalMaps();
			HideUnneededLandPasses();
			pExteriorCellLoader->PostProcessCompletedTasks();
			if (pExteriorCellLoader->TryCancelTasks()) {
				pExteriorCellLoader->WaitForTasks();
			}
			if (!pInteriorCell || bsavegameLoading) {
				if (bPreemptivelyUnloadCells.Bool()) {
					if (FreeInteriorBuffer(1)) {
						CleanUpUnusedTextures(0);
					}
				}
				if (!pWorldSpace) {
					SetWorldSpace(static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item));
				}
				bool bmoveImmediate = 0;
				if (std::abs(ideltaX) > 1 || std::abs(ideltaY) > 1) {
					bmoveImmediate = 1;
				}
				if (TESObjectCELL::ExteriorWorld()) {
					if (bmoveImmediate || !abAllowPartialRemovals) {
						bhkWorldM* ExteriorbhkWorld = TESObjectCELL::ExteriorWorld();
						ExteriorbhkWorld->StopBatchingAdds(0);
						bhkWorldM* v8 = TESObjectCELL::ExteriorWorld();
						v8->StopBatchingRemovals(0);
					}
					bhkWorldM* v9 = TESObjectCELL::ExteriorWorld();
					v9->StartBatchingRemovals();
				}
				pGridCellA->SetCenter(inewX, inewY);
				if (TESObjectCELL::ExteriorWorld()) {
					if (!bmenudisplayed && (!bmoveImmediate || !pExteriorCellLoader->QueuedCellCount())) {
						bhkWorldM* v10 = TESObjectCELL::ExteriorWorld();
					}
					if (bmoveImmediate) {
						bhkWorldM* v11 = TESObjectCELL::ExteriorWorld();
						v11->StopBatchingRemovals(0);
					} else {
						bhkWorldM* v12 = TESObjectCELL::ExteriorWorld();
						v12->StopBatchingRemovals(abAllowPartialRemovals);
					}
				}
				if (!bmenudisplayed && !AllCellsInGridLoaded() && !pSaveLoadGame->GetSaveGameLoading()) {
					ShowLoadingMessage("Loading Area (unloaded cells)...");
					bmenudisplayed = 1;
				}
				if (std::abs(ideltaX) > 1 || std::abs(ideltaY) > 1) {
					InitModelsToLoad(MODEL_COUNT_PRELOAD);
				}
				if (TESObjectCELL::ExteriorWorld()) {
					bhkWorldM* v13 = TESObjectCELL::ExteriorWorld();
					v13->StartBatchingAdds();
				}
				GridArrayLoad();
				SetUpdateGridString();
				if (TESObjectCELL::ExteriorWorld()) {
					if (bmoveImmediate) {
						bhkWorldM* v14 = TESObjectCELL::ExteriorWorld();
						v14->StopBatchingAdds(0);
					} else {
						bhkWorldM* v15 = TESObjectCELL::ExteriorWorld();
						v15->StopBatchingAdds(abAllowPartialRemovals);
					}
				}
			}
			CleanUpUnusedTextures(0);
			TESWaterListener::RefreshExteriorWaterHeights();
			if (!pInteriorCell) {
				const NiPoint3* a1a = &pPlayer->GetLocationOnReference();
				BGSTerrainManager* TerrainManager = pWorldSpace->GetTerrainManager();
				TerrainManager->Update(*a1a, BGSTerrainManager::UPDATE_ALL);
			}
			BSTaskThread::SetUpdateSleepEnabled(1);
			return 1;
		} else {
			if (bBackgroundCellLoads.Bool() && !pInteriorCell) {
				int32_t icellx = x >> 12;
				int32_t icelly = y_2 >> 12;
				if (iCurrentQueuedX != x >> 12 || iCurrentQueuedY != icelly) {
					if (bPreemptivelyUnloadCells.Bool()) {
						if (FreeInteriorBuffer(1)) {
							CleanUpUnusedTextures(0);
						}
					}
					int32_t ihalfgrid = uGridsToLoad.UInt() >> 1;
					int32_t iqueuex = iCurrentQueuedX;
					int32_t ideltax = icellx - iCurrentQueuedX;
					if (ideltax) {
						iqueuex += ideltax * (ihalfgrid + 1);
						for (int32_t iy = -ihalfgrid; iy <= ihalfgrid; ++iy) {
							int32_t someFloata = iCurrentQueuedY + iy;
							TESWorldSpace* WorldSpace = pWorldSpace;
							pExteriorCellLoader->QueueCellLoad(WorldSpace, iqueuex, someFloata);
						}
					}
					int32_t iqueuey = iCurrentQueuedY;
					int32_t ideltay = icelly - iCurrentQueuedY;
					if (ideltay) {
						iqueuey += ideltay * (ihalfgrid + 1);
						for (int32_t ix = -ihalfgrid; ix <= ihalfgrid; ++ix) {
							int32_t a1 = iCurrentQueuedX + ix;
							TESWorldSpace* v18 = pWorldSpace;
							pExteriorCellLoader->QueueCellLoad(v18, a1, iqueuey);
						}
					}
					if (ideltax) {
						if (ideltay) {
							TESWorldSpace* v19 = pWorldSpace;
							pExteriorCellLoader->QueueCellLoad(v19, iqueuex, iqueuey);
						}
					}
				}
				iCurrentQueuedX = icellx;
				iCurrentQueuedY = icelly;
			}
			return 0;
		}
	}
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
	} else {
		float fWindSpeed = pSky->GetWindSpeed();
		float fWindAngle = pSky->GetWindAngle();
		BSWindModifier::SetWind(fWindSpeed, fWindAngle);
		bhkWindListener::SetWind(fWindSpeed, fWindAngle);
	}

	fCellAppTime += afTime;
	LockHavokUpdateMT(true);
	if (pInterior) {
		pInterior->UpdateManagedNodes(fCellAppTime, false);
	} else {
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
	NiUpdateData kUpdateData;
	kUpdateData.~NiUpdateData();
	::new (&kUpdateData) NiUpdateData(fCellAppTime, false, false);
	GetTempNodeManager()->Update(kUpdateData);
}

// Takes or releases the Havok update lock, which only matters with more than one hardware thread.
void TES::LockHavokUpdateMT(bool abLock) {
	if (iNumHWThreads.UInt() != 1) {
		if (abLock) {
			HavokUpdateCrit.Enter(0);
		} else {
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
		} else {
			AddToExteriorBuffer(apCell);
		}
	}
}

void TES::AddToExteriorBuffer(TESObjectCELL* apCell) {
	uint32_t iniValue;
	int32_t* v4;
	int32_t* v5;
	uint32_t v6;
	TESObjectCELL* v9;
	uint32_t i;
	SortExteriorBuffer();
	uint8_t v7 = (uint8_t)(apCell->GetAttached() || apCell->IsAttaching());
	int8_t v11 = (int8_t)v7;
	int32_t* intValue = &(&uGridsToLoad)->Int();
	if (!pExteriorBuffer[(((&uGridsToLoad)->Int()) * (*intValue)) - 1]) {
		v11 = 1;
	}
	TESObjectCELL* v10 = apCell;
	if (v11 ||
		((iniValue = (&uExteriorCellBuffer)->operator uint32_t()),
			((v4 = &(&uGridsToLoad)->Int()), (iniValue <= (((&uGridsToLoad)->Int()) * (*v4)))))) {
		v6 = 0;
	} else {
		v5 = &(&uGridsToLoad)->Int();
		v6 = ((&uGridsToLoad)->Int()) * (*v5);
	}
	for (i = v6; i < ((uint32_t)((int32_t)(&uExteriorCellBuffer)->operator uint32_t())); ++i) {
		v9 = pExteriorBuffer[i];
		pExteriorBuffer[i] = v10;
		if (v9 == apCell) {
			return;
		}
		v10 = v9;
	}
	if (v10) {
		pDataHandler->UnloadCell(v10);
		SetUpdateGridString();
	}
}

// Moves the queued cells that lie inside the loaded grid ahead of the cells outside it in the exterior buffer.
void TES::SortExteriorBuffer() {
	// Only X is compared, as the game does: the original also compares iCurrentQueuedY with itself (a bug, always
	// equal), not with a current grid Y.
	if (iCurrentGridX == iCurrentQueuedX) {
		return;
	}
	int32_t* pGrids = &uGridsToLoad.Int();
	uint32_t uiStart = *pGrids * uGridsToLoad.Int();
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
			std::memmove(&pExteriorBuffer[uiLastOutside + 1], &pExteriorBuffer[uiLastOutside],
				sizeof(TESObjectCELL*) * (i - uiLastOutside));
			pExteriorBuffer[uiLastOutside] = pCell;
			i = uiLastOutside + 1;
			uiLastOutside = 0;
		} else {
			uiLastOutside = i;
		}
	}
}

void TES::SetUpdateGridString() {
	bUpdateGridString = (bool)1;
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
	} else {
		int32_t iLast = static_cast<int32_t>(static_cast<uint32_t>(uInteriorCellBufferGeneral) - 1);
		for (int32_t iIndex = iLast; iIndex >= 0; --iIndex) {
			if (iIndex == iLast && pInteriorBuffer[iIndex]) {
				pDataHandler->UnloadCell(pInteriorBuffer[iIndex]);
			}

			if (iIndex != 0) {
				pInteriorBuffer[iIndex] = pInteriorBuffer[iIndex - 1];
			} else {
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
	uint32_t i;
	for (i = 0; i < (&uExteriorCellBuffer)->operator uint32_t(); ++i) {
		if (pExteriorBuffer[i] == apCell) {
			return (bool)1;
		}
	}
	return (bool)0;
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
			} else {
				GetSky()->SetMode(Sky::SM_INTERIOR);
			}
		} else {
			GetSky()->SetMode(Sky::SM_FULL);
		}
	} else {
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
	} else {
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
	} else {
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
		[[maybe_unused]] TESWorldSpace* pWorldSpace = nullptr;
		if (apLocation) {
			switch (apLocation->GetFormType()) {
			case WRLD_ID:
				pWorldSpace = static_cast<TESWorldSpace*>(apLocation);
				break;
			case CELL_ID:
				pCell = static_cast<TESObjectCELL*>(apLocation);
				if (pCell && !pCell->GetInterior()) {
					pWorldSpace = pCell->GetWorldSpace();
					apLocation = pWorldSpace;
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
					pWorldSpace = pCell->GetWorldSpace();
					pCell = nullptr;
				} else if (!pCell) {
					pWorldSpace = static_cast<TESObjectREFR*>(apLocation)->GetWorldSpace();
				}
				break;
			default:
				break;
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
	} else {
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
		::new (aucContext) AutoMemContext(MEM_CONTEXT_WATER, true,
			"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 5969);
		{
			NiPointer<NiSourceTexture> spNoiseTexture(nullptr);
			pInstance->CreateTextureImage(apWorldSpace->GetWaterNoiseTexturePath(), spNoiseTexture, true, true);
			pInstance->GetWaterSystem()->SetWaterNoiseTexture(spNoiseTexture.operator->());
		}
		pContext->~AutoMemContext();
	} else {
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
		TESTerrainLODManager::UpdateGrass(kPos, NiPoint3_UNIT_Y, 1.0f);
		TallGrassShaderProperty::SetUseBackgroundLoading(true);
		if (!CellTestRunning()) {
			pFaderManager->CreateFader(FADER_TO_BLACK, fFadeToBlackFadeSeconds.Float(), false);
		}
		CreateFurnitureList();
		ReferenceProcessLists.PlaceActorsinBedsOrChairs();
		ReferenceProcessLists.AnimateDeadActors();
	} else {
		pGridCellA->SetCenter(iCurrentGridX, iCurrentGridY);
		InitModelsToLoad(MODEL_COUNT_NO_GRID_CELL);
		GridArrayLoad();
	}
	RunScripts();
	NiUpdateData kUpdateData;
	kUpdateData.~NiUpdateData();
	::new (&kUpdateData) NiUpdateData(0.0f, false, false);
	pObjLandRoot->Update(kUpdateData);
	CleanUpUnusedTextures(false);
	pDataHandler->CloseAllTES();
	bReloading = false;
}

void TES::GridArrayLoad() {
	AutoMemContext tempMemContext(MEM_CONTEXT_CELLS, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 2262);
	int32_t icellX = iCurrentGridX - (uGridsToLoad.UInt() >> 1);
	int32_t icellY = iCurrentGridY - (uGridsToLoad.UInt() >> 1);
	int32_t icellsLoaded = 0;
	bool bdelayedlandload = Interface::GetLoadingMenuVisible();
	bool bshowLoadingBar = Interface::GetLoadingMenuVisible();
	pSaveLoadGameOLD->SetLoading(1);
	pSaveLoadGame->SetDeferInitForms(1);
	if (TESObjectCELL::ExteriorWorld()) {
		bhkWorldM* ExteriorbhkWorld = TESObjectCELL::ExteriorWorld();
		ExteriorbhkWorld->StartBatchingAdds();
	}
	for (int32_t x = 0; x < uGridsToLoad.UInt(); ++x) {
		for (int32_t y = 0; y < uGridsToLoad.UInt(); ++y) {
			LoadGridCell(x, y, x + icellX, y + icellY);
			TESWorldSpace* WorldSpace = pWorldSpace;
			TESObjectCELL* pcell = pDataHandler->GetCellFromCellCoord(x + icellX, y + icellY, WorldSpace, 0);
			if (TES::pInstance->IsBuffered(pcell)) {
				AddToBuffer(pcell);
			}
			if (bshowLoadingBar || (pSaveLoadGameOLD->GetLoading() && pSaveLoadGameOLD->GetLoading())) {
				double v10 = static_cast<double>(++icellsLoaded);
				int32_t* v2 = &uGridsToLoad.Int();
				// The cell count is converted as unsigned.
				float fpercent = v10 / static_cast<double>(static_cast<uint32_t>(uGridsToLoad.Int() * *v2)) * 100.0;
				if (pSaveLoadGameOLD->GetLoading() && pSaveLoadGameOLD->GetLoading()) {
					pSaveLoadGameOLD->StartLoadingSequence();
				}
			}
		}
	}
	if (TESObjectCELL::ExteriorWorld()) {
		bhkWorldM* v3 = TESObjectCELL::ExteriorWorld();
		v3->StopBatchingAdds(0);
	}
	pSaveLoadGameOLD->SetLoading(0);
	if (pSaveLoadGameOLD->GetInitArrayExists()) {
		bool brestore = pSaveLoadGameOLD->GetLoading();
		bool brestore2 = pSaveLoadGameOLD->GetLoading();
		pSaveLoadGameOLD->SetLoading(1);
		pSaveLoadGameOLD->SetLoading(1);
		pSaveLoadGameOLD->InitForms(0, 0, 0);
		pSaveLoadGameOLD->LoadQueuedData(0);
		pSaveLoadGameOLD->SetLoading(brestore2);
		pSaveLoadGameOLD->SetLoading(brestore);
	}
	pSaveLoadGame->SetDeferInitForms(0);
	if (!pSaveLoadGame->GetSaveGameLoading()) {
		pSaveLoadGame->InitForms(0);
	}
	if (TESObjectCELL::ExteriorWorld()) {
		bhkWorldM* v4 = TESObjectCELL::ExteriorWorld();
		v4->StartBatchingAdds();
	}
	// The counters are compared with the setting unsigned.
	for (int32_t x_2 = 0; static_cast<uint32_t>(x_2) < uGridsToLoad.UInt(); ++x_2) {
		for (int32_t y_2 = 0; static_cast<uint32_t>(y_2) < uGridsToLoad.UInt(); ++y_2) {
			if (pWorldSpace) {
				TESWorldSpace* v9 = pWorldSpace;
				TESObjectCELL* pcell_2 = pDataHandler->GetCellFromCellCoord(x_2 + icellX, y_2 + icellY, v9, 0);
				if (pcell_2) {
					if (pcell_2->GetState() == TESObjectCELL::CS_ATTACHING) {
						pcell_2->AttachModels(!bdelayedlandload);
						AddToBuffer(pcell_2);
						NiNode* pRoot = pcell_2->Load3D();
						pRoot->UpdateProperties();
						if (!pRoot->GetChildCount()) {
							NiUpdateData UpdateData(0.0f, 0, 0);
							pRoot->Update(UpdateData);
						}
					}
					(&listLastLoadedExteriors)->AddHead(pcell_2);
				}
			}
		}
	}
	if (TESObjectCELL::ExteriorWorld()) {
		bhkWorldM* v7 = TESObjectCELL::ExteriorWorld();
		v7->StopBatchingAdds(0);
	}
	BSShaderManager::SetProjectedShadowMaskTexture(0);
	GridCellArray::SetRecreateCanopyShadowMaskTexture(1);
}

void TES::LoadInterior(TESObjectCELL* apNewInterior, const NiPoint3* apPos) {
	if (!CellTestRunning()) {
		appMain->RenderMenuBackground();
	}
	bhkWorldM* ExteriorbhkWorld = TESObjectCELL::ExteriorWorld();
	ExteriorbhkWorld->SetEnabled(0);
	bhkWorldM* v4 = TESObjectCELL::ExteriorWorld();
	v4->SetVisualDebugger(0);
	TESObjectCELL* pint = pInteriorCell;
	if (pint == apNewInterior) {
		UpdateCurrentGridCell(apPos, 0);
	} else {
		if (BSShaderManager::GetAccumulator()) {
			BSShaderAccumulator* Accumulator = BSShaderManager::GetAccumulator();
			Accumulator->ClearRenderPasses();
		}
		if (bPreemptivelyUnloadCells.Bool()) {
			if (pint) {
				DetachGridCells();
				if (FreeExteriorBuffer(1, 0)) {
					CleanUpUnusedTextures(0);
				}
			} else {
				if (FreeExteriorBuffer(1, 0)) {
					CleanUpUnusedTextures(0);
				}
			}
		}
		if (!pSaveLoadGame->GetSaveGameLoading() && !Interface::IsMenuIDVisible(Interface::eLoadingMenu, 0) &&
			!pMoviePlayer->GetPlayingSequence()) {
			appMain->Swap();
		}
		BGSEncounterZone* pzone = apNewInterior->GetEncounterZone();
		if (pzone) {
			pzone->DetermineLevel();
		}
		ImpactSwap* ImpactSwap_ = apNewInterior->GetImpactSwap();
		BGSImpactDataSet::ApplyImpactSwap(ImpactSwap_);
		ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
		ShadowSceneNode_->SetPortalGraph(0);
		ShadowSceneNode* v8 = BSShaderManager::GetShadowSceneNode(0);
		v8->ProcessQueuedLights();
		ShadowSceneNode* v9 = BSShaderManager::GetShadowSceneNode(0);
		v9->RemoveAllLights();
		bool wasInterior = 0;
		if (pint) {
			wasInterior = 1;
			pint->Detach(1);
			BSShaderManager::ClearAllShaderPassStages();
			NiAVObject* someFloat = pPlayer->Get3D(0);
			ShadowSceneNode* v10 = BSShaderManager::GetShadowSceneNode(0);
			v10->AddShadowLight(someFloat);
		}
		if (!IsInteriorBuffered(apNewInterior)) {
			TESObjectCELL* poldint = GetInteriorBufferCellByIndex(TES::uInteriorCellBufferGeneral.UInt() - 1);
			if (poldint) {
				RemoveCellFromInteriorBuffer(poldint);
			}
		}
		pint = apNewInterior;
		SetInterior(apNewInterior);
		BSShaderManager::SetInterior(1);
		if (apNewInterior) {
			if (iSaveGridX == 0x7FFFFFFF && !wasInterior) {
				iSaveGridX = this->GetCurrentGridX();
				iSaveGridY = this->GetCurrentGridY();
				CullGridCells(1, 0);
			}
			iCurrentGridX = FloatToInt(apPos->x) >> 12;
			iCurrentGridY = FloatToInt(apPos->y) >> 12;
			pint->LoadAllTempData();
			bool bloadingSaveGame = pSaveLoadGame->GetSaveGameLoading();
			bool bloadingSaveGameOLD = pSaveLoadGameOLD->GetLoading();
			if (pint->GetReferenceCount()) {
				if (!pint->IsLoaded()) {
					ShowLoadingMenu(1, pint, 0);
				}
			}
			pint->AttachToWorld(pObjRoot);
			bhkWorld* pw = pint->GetbhkWorld();
			if (spLoadedAreaBound.m_pObject) {
				spLoadedAreaBound.m_pObject->Clear();
			}
			if (pw) {
				pw->StartBatchingAdds();
			}
			pint->AttachModels(1);
			if (pModelLoader->IsFileQueuedForCell(pint)) {
				ShowLoadingMenu(1, pint, 0);
			}
			SetAtmosphere(pint);
			if (!bloadingSaveGame) {
				pIOManager->LoadQueuedPriority(IO_TASK_PRIORITY_VERY_HIGH);
			}
			if (pw) {
				pw->StopBatchingAdds(0);
			}
			TES::pInstance->ActivateAllSubmergedReferencesIslands();
			if (!CellTestRunning() && !bloadingSaveGame) {
				if (pPlayer->Get3D()) {
					pFaderManager->CreateFader(FADER_TO_BLACK, 0.0f, 0);
				}
			}
			if (!bloadingSaveGame) {
				ReferenceProcessLists.HandleInvalidPositions();
				CreateFurnitureList();
				ReferenceProcessLists.PlaceActorsinBedsOrChairs();
				ReferenceProcessLists.AnimateDeadActors();
			}
			CleanUpUnusedTextures(0);
			pDataHandler->CloseAllTES();
			NiNode* pRoot = pint->Load3D();
			pRoot->UpdateProperties();
			if (!pRoot->GetChildCount()) {
				NiUpdateData UpdateData(0.0f, 0, 0);
				pRoot->Update(UpdateData);
			}
			pint->AttachLights(1);
			AddToInteriorBuffer(apNewInterior);
		}
		if (!wasInterior) {
			BSShaderAccumulator* paccum = BSShaderManager::GetAccumulator();
			if (paccum) {
				paccum->ClearAllQueries();
			}
			TallGrassShaderProperty::ClearStaticMaps();
			pGridCellA->ClearCanopyShadowMaskTexture();
		}
		if (CellTestRunning()) {
			ShowLoadingMenu(0, 0, 0);
		}
		CleanUpUnusedTextures(0);
		Interface::SetEnemyActor(0);
		if (*pint->GetWaterNoiseTextureName()) {
			AutoMemContext tempMemContext(MEM_CONTEXT_WATER, 1,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 3188);
			NiPointer<NiSourceTexture> spSourceTexture(static_cast<NiSourceTexture*>(0));
			const char* WaterNoiseTextureName = pint->GetWaterNoiseTextureName();
			TES::pInstance->CreateTextureImage(WaterNoiseTextureName, spSourceTexture, 1, 0);
			NiSourceTexture* someFloata = spSourceTexture.m_pObject;
			TES::pInstance->GetWaterSystem()->SetWaterNoiseTexture(someFloata);
		} else {
			TES::pInstance->GetWaterSystem()->SetWaterNoiseTexture(0);
		}
		if (!pSaveLoadGame->GetSaveGameLoading()) {
			NavMeshObstacleManager* Instance = NavMeshObstacleManager::GetInstance();
			Instance->ForceUpdateAllObstacles();
		}
	}
}

void TES::LeaveInterior(NiPoint3* apPos) {
	NiPoint2 v26;
	TESWorldSpace* pWorld = pWorldSpace;
	if (pWorld) {
		BGSTerrainManager* TerrainManager = pWorld->GetTerrainManager();
		TerrainManager->Update(*apPos, BGSTerrainManager::UPDATE_ALL);
		BGSTerrainManager* v3 = pWorld->GetTerrainManager();
		v3->UpdateMorphParameters(*apPos);
	}
	BSShaderManager::SetInterior(0);
	if (pWorldSpace) {
		BGSImpactDataSet::ApplyImpactSwap(pWorldSpace->pImpactSwap);
	}
	DistantLODShaderProperty::SetDistantLODCulled(0);
	bhkWorldM* ExteriorbhkWorld = TESObjectCELL::ExteriorWorld();
	ExteriorbhkWorld->SetEnabled(1);
	TESObjectCELL* pint = pInteriorCell;
	if (pint) {
		ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
		BSPortalGraph* PortalGraph = ShadowSceneNode_->GetPortalGraph();
		if (PortalGraph == pint->GetPortalGraph()) {
			ShadowSceneNode* v7 = BSShaderManager::GetShadowSceneNode(0);
			v7->SetPortalGraph(0);
		}
		pint->Detach(1);
		if (IsInteriorBuffered(pint)) {
			pint->ChangeProcessLevel(1);
		} else {
			pDataHandler->UnloadCell(pint);
		}
		SetInterior(0);
		if (!pSaveLoadGame->GetSaveGameLoading()) {
			if (iSaveGridX == 0x7FFFFFFF) {
				if (!GetLandGeometry(*apPos)) {
					iCurrentGridX = 0x7FFFFFFF;
					iCurrentGridY = 0x7FFFFFFF;
					iCurrentQueuedX = iCurrentGridX;
					iCurrentQueuedY = iCurrentGridY;
				}
			} else {
				if (*pExteriorBuffer) {
					iCurrentGridX = iSaveGridX;
					iCurrentGridY = iSaveGridY;
					CullGridCells(0, apPos);
				} else {
					iCurrentGridX = 0x7FFFFFFF;
					iCurrentGridY = 0x7FFFFFFF;
					iCurrentQueuedX = iCurrentGridX;
					iCurrentQueuedY = iCurrentGridY;
				}
				iSaveGridX = 0x7FFFFFFF;
				iSaveGridY = 0x7FFFFFFF;
			}
		}
		ShadowSceneNode* v8 = BSShaderManager::GetShadowSceneNode(0);
		v8->RemoveAllLights();
		NiAVObject* z = pPlayer->Get3D(0);
		ShadowSceneNode* v9 = BSShaderManager::GetShadowSceneNode(0);
		v9->AddShadowLight(z);
	}
	if (pObjLandRoot) {
		NiNode* pLODroot = pObjLandRoot->m_pkParent;
		if (pLODroot) {
			pLODroot->SetAppCulled(0);
		}
		BGSTerrainManager::EnableLODWaterProcessing(1);
	}
	TESObjectCELL::ExteriorWorld();
	bool za = bhkWorld::IsHavokDebug();
	bhkWorldM* v10 = TESObjectCELL::ExteriorWorld();
	v10->SetVisualDebugger(za);
	bool bloadingSaveGame = pSaveLoadGame->GetSaveGameLoading();
	bool bloadingSaveGameOLD = pSaveLoadGameOLD->GetLoading();
	UpdateCurrentGridCell(apPos, !bloadingSaveGameOLD);
	TESObjectCELL* pcurrentCell = GetGridCellCell(iCurrentGridX, iCurrentGridY);
	if (pModelLoader->IsFileQueuedForCell(pcurrentCell)) {
		ShowLoadingMenu(1, pcurrentCell, 0);
	}
	if (TESObjectCELL::ExteriorWorld()) {
		bhkWorldM* v11 = TESObjectCELL::ExteriorWorld();
		v11->StartBatchingAdds();
	}
	if (!bloadingSaveGame) {
		pIOManager->LoadQueuedPriority(IO_TASK_PRIORITY_LOW);
	}
	pGridCellA->AttachAllLand3D();
	HideFarLandNormalMaps();
	HideUnneededLandPasses();
	if (TESObjectCELL::ExteriorWorld()) {
		bhkWorldM* v12 = TESObjectCELL::ExteriorWorld();
		v12->StopBatchingAdds(0);
	}
	ActivateAllSubmergedReferencesIslands();
	NiPoint3 kGrassPos(apPos->x, apPos->y, 0.0f);
	GetLandHeight(*apPos, kGrassPos.z);
	TallGrassShaderProperty::SetUseBackgroundLoading(0);
	TESTerrainLODManager::UpdateGrass(kGrassPos, NiPoint3_UNIT_Y, 1.0f);
	TallGrassShaderProperty::SetUseBackgroundLoading(1);
	if (!CellTestRunning() && !bloadingSaveGame) {
		float* v13 = &fFadeToBlackFadeSeconds.Float();
		pFaderManager->CreateFader(FADER_TO_BLACK, *v13, 1);
	}
	if (!bloadingSaveGame) {
		ReferenceProcessLists.HandleInvalidPositions();
		CreateFurnitureList();
		ReferenceProcessLists.PlaceActorsinBedsOrChairs();
		ReferenceProcessLists.AnimateDeadActors();
	}
	if (pint) {
		if (!pint->GetFakeExterior()) {
			pSky->ActivateWeatherSounds(pSky->pCurrentWeather);
			pSky->ActivateWeatherSounds(pSky->pLastWeather);
		}
	}
	UpdateGridLights();
	SetAtmosphere(0);
	pObjRoot->UpdateProperties();
	Interface::SetEnemyActor(0);
	if (CellTestRunning()) {
		ShowLoadingMenu(0, 0, 0);
	}
	CleanUpUnusedTextures(0);
	if (pWorldSpace && *pWorldSpace->GetWaterNoiseTexturePath()) {
		AutoMemContext tempMemContext(MEM_CONTEXT_WATER, 1,
			"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 3434);
		NiPointer<NiSourceTexture> spSourceTexture(nullptr);
		const char* WaterNoiseTexturePath = pWorldSpace->GetWaterNoiseTexturePath();
		TES::pInstance->CreateTextureImage(WaterNoiseTexturePath, spSourceTexture, 1, 0);
		NiSourceTexture* zb = spSourceTexture.m_pObject;
		TES::pInstance->GetWaterSystem()->SetWaterNoiseTexture(zb);
	} else {
		TES::pInstance->GetWaterSystem()->SetWaterNoiseTexture(0);
	}
	TESWaterSystem::bForceWaterUpdate = 1;
	float y = (static_cast<double>(iCurrentGridY) + 0.5) * 4096.0;
	float v23 = (static_cast<double>(iCurrentGridX) + 0.5) * 4096.0;
	NiPoint2* v15 = ::new (&v26) NiPoint2(v23, y);
	float x = v15->x;
	float v17 = v15->y;
	BSShaderManager::kGridArrayCenter.x = x;
	BSShaderManager::kGridArrayCenter.y = v17;
	BSShaderManager::kOldGridArrayCenter.x = x;
	BSShaderManager::kOldGridArrayCenter.y = v17;
	BSShaderManager::fGriddArrayLerpStart = BSShaderManager::GetTimer(false);
	TES::bLODFadeInProgress = 0;
	BSShaderManager::bLODFadeInProgress = 0;
	BGSTerrainManager* v18 = pWorldSpace->GetTerrainManager();
	BSShaderManager::bInLODWorld = v18->HasRootNode();
	if (!pSaveLoadGame->GetSaveGameLoading()) {
		NavMeshObstacleManager* Instance = NavMeshObstacleManager::GetInstance();
		Instance->ForceUpdateAllObstacles();
	}
}

void TES::SetupTemporaryParticleCache() {
	if (!bdoOnce) {
		AutoMemContext tempMemContext(MEM_CONTEXT_TES_ACTOR, 1,
			"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 0x1C66);
		bdoOnce = 1;
		if (*sDismemberParticleDefault.String()) {
			const char* v1 = sDismemberParticleDefault.String();
			NiNode* File = pModelLoader->LoadFile(v1, LOD_MULT_NONE, 1, 0, 0, 0);
			AddTemporaryParticleObjectToCache(File);
		}
		if (*sDismemberRobotParticleDefault.String()) {
			const char* v3 = sDismemberRobotParticleDefault.String();
			NiNode* v4 = pModelLoader->LoadFile(v3, LOD_MULT_NONE, 1, 0, 0, 0);
			AddTemporaryParticleObjectToCache(v4);
		}
		if (*sSplashParticles.String()) {
			const char* v5 = sSplashParticles.String();
			NiNode* v6 = pModelLoader->LoadFile(v5, LOD_MULT_NONE, 1, 0, 0, 0);
			AddTemporaryParticleObjectToCache(v6);
		}
		if (*sBloodParticleMeleeDefault.String()) {
			const char* v7 = sBloodParticleMeleeDefault.String();
			NiNode* v8 = pModelLoader->LoadFile(v7, LOD_MULT_NONE, 1, 0, 0, 0);
			AddTemporaryParticleObjectToCache(v8);
		}
		if (*sImpactParticleWoodDefault.String()) {
			const char* v9 = sImpactParticleWoodDefault.String();
			NiNode* v10 = pModelLoader->LoadFile(v9, LOD_MULT_NONE, 1, 0, 0, 0);
			AddTemporaryParticleObjectToCache(v10);
		}
		if (*sImpactParticleMetalDefault.String()) {
			const char* v11 = sImpactParticleMetalDefault.String();
			NiNode* v12 = pModelLoader->LoadFile(v11, LOD_MULT_NONE, 1, 0, 0, 0);
			AddTemporaryParticleObjectToCache(v12);
		}
		if (*sImpactParticleConcreteDefault.String()) {
			const char* v13 = sImpactParticleConcreteDefault.String();
			NiNode* v14 = pModelLoader->LoadFile(v13, LOD_MULT_NONE, 1, 0, 0, 0);
			AddTemporaryParticleObjectToCache(v14);
		}
		if (*sBloodParticleDefault.String()) {
			const char* v15 = sBloodParticleDefault.String();
			NiNode* v16 = pModelLoader->LoadFile(v15, LOD_MULT_NONE, 1, 0, 0, 0);
			AddTemporaryParticleObjectToCache(v16);
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
	if (spPreloadedAddonNodes.operator->()) {
		return;
	}

	// AutoMemContext as in InitializeWater.
	alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
	AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
	::new (aucContext) AutoMemContext(MEM_CONTEXT_EFFECTS_OVERHEAD, true,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 7054);
	void* pMemory = ourMalloc(sizeof(QueuedFile));
	spPreloadedAddonNodes = pMemory ? ::new (pMemory) QueuedFile(IO_TASK_PRIORITY_VERY_LOW) : nullptr;
	uint32_t iaddonNodeCount = pDataHandler->GetAddonNodeCount();
	for (uint32_t i = 0; i < iaddonNodeCount; ++i) {
		BGSAddonNode* paddonNode = pDataHandler->GetAddonNode(i);
		if (!paddonNode) {
			continue;
		}

		if (paddonNode->IsMasterParticleSystem()) {
			TESModel* pModel = paddonNode;
			NiNode* pav = pModelLoader->LoadFile(pModel->GetModel(), LOD_MULT_NONE, true, false, false, false);
			if (pav) {
				BSMasterParticleSystem* pmasterPSys = static_cast<BSMasterParticleSystem*>(pav);
				pmasterPSys->SetEditorIndex(paddonNode->GetIndex());
				pmasterPSys->SetIndex(paddonNode->GetMasterParticleSystemIndex());
				pmasterPSys->SetMaxParticlesPerFrame(paddonNode->GetMasterParticleCap());
				Main::GetMasterParticleSystemManager()->AddMasterParticleSystem(pmasterPSys);
			}

			pModelLoader->ReleaseModel(pModel->GetModel(), true, 1);
		} else {
			pModelLoader->QueueModel(paddonNode, IO_TASK_PRIORITY_VERY_LOW, spPreloadedAddonNodes.operator->(),
				LOD_MULT_NONE, true, false, false);
		}
	}
	pContext->~AutoMemContext();
}

// Queues the files of every body part data form and of the fists under spPreloadedForms. AutoMemContext as in
// InitializeWater.
void TES::PreloadForms() {
	if (spPreloadedForms.operator->()) {
		return;
	}

	alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
	AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
	::new (aucContext) AutoMemContext(MEM_CONTEXT_EFFECTS_OVERHEAD, true,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 7120);
	void* pMemory = ourMalloc(sizeof(QueuedFile));
	spPreloadedForms = pMemory ? ::new (pMemory) QueuedFile(IO_TASK_PRIORITY_VERY_LOW) : nullptr;
	BSSimpleList<BGSBodyPartData*>* pIter = pDataHandler->GetPartDataList();
	while (pIter && !pIter->IsEmpty()) {
		BGSBodyPartData* pPartData = static_cast<BGSBodyPartData*>(pIter->GetItem());
		pIter = pIter->GetNext();
		if (pPartData) {
			BGSPreloadable* pPreloadable = pPartData;
			pPreloadable->QueueFiles(IO_TASK_PRIORITY_VERY_LOW, spPreloadedForms.operator->());
		}
	}

	if (pFists) {
		pFists->QueueFiles(IO_TASK_PRIORITY_VERY_LOW, spPreloadedForms.operator->(), false);
	}
	pContext->~AutoMemContext();
}

void TES::PreloadDefaultModels() {
	char path[260];
	AutoMemContext tempMemContext(MEM_CONTEXT_EFFECTS_OVERHEAD, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 7152);
	if (*sRagdollDataDefault.String()) {
		const char* v1 = sRagdollDataDefault.String();
		pModelLoader->QueueModel(v1, IO_TASK_PRIORITY_VERY_LOW, 0, LOD_MULT_NONE, 1, 1, 0);
	}
	if (*sBloodParticleDefault.String()) {
		const char* v2 = sBloodParticleDefault.String();
		pModelLoader->QueueModel(v2, IO_TASK_PRIORITY_VERY_LOW, 0, LOD_MULT_NONE, 1, 1, 0);
	}
	if (*sSplashParticles.String()) {
		const char* v3 = sSplashParticles.String();
		pModelLoader->QueueModel(v3, IO_TASK_PRIORITY_VERY_LOW, 0, LOD_MULT_NONE, 1, 1, 0);
	}
	if (*sExplosionSplashParticles.String()) {
		const char* v4 = sExplosionSplashParticles.String();
		pModelLoader->QueueModel(v4, IO_TASK_PRIORITY_VERY_LOW, 0, LOD_MULT_NONE, 1, 1, 0);
	}
	if (*sBloodTextureDefault.String()) {
		BSsprintf(path, 260, "%s\\%s\\%s", "Data", "Textures", sBloodTextureDefault.String());
		CreateTextureImage(path, BloodDecalPreload1, 0, 0);
	}
}

// Releases the four default particle models the settings name (each setting is queried twice, as the game does), then
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
// system getter's result is unused, and the list's items are not null-checked, as the game does. The AutoMemContext
// constructor's result is ignored.
void TES::PreloadPlaceableWaterNoiseTextures() {
	alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
	AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
	::new (aucContext) AutoMemContext(MEM_CONTEXT_WATER, true,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 7207);
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
			pWater->SetNoiseTexture(spTexture.operator->());
		}
	}
	pContext->~AutoMemContext();
}

// Caches apObject with two clones of it at the head of pParticleCacheHead, unless the list already holds it. The
// clones' array is built as new[] of three NiPointers. The null allocation is not checked for, as the game does.
void TES::AddTemporaryParticleObjectToCache(NiAVObject* apObject) {
	if (!apObject) {
		return;
	}

	for (ParticleObjectCache* pcache = pParticleCacheHead; pcache; pcache = pcache->QNext()) {
		if (pcache->spModel.operator==(apObject)) {
			return;
		}
	}

	void* pMemory = ourMalloc(sizeof(ParticleObjectCache));
	ParticleObjectCache* pnew = pMemory ? ::new (pMemory) ParticleObjectCache : nullptr;
	pnew->spModel = apObject;
	pnew->SetNext(pParticleCacheHead);
	pParticleCacheHead = pnew;

	size_t* pArray = static_cast<size_t*>(ourMalloc(4 * sizeof(size_t)));
	NiPointer<NiAVObject>* pspClones = nullptr;
	if (pArray) {
		pArray[0] = 3;
		pspClones = reinterpret_cast<NiPointer<NiAVObject>*>(pArray + 1);
		rt::eh_vector_constructor_iterator(pspClones, sizeof(NiPointer<NiAVObject>), 3,
			&fnvd::constructElement<NiPointer<NiRefObject>>, &fnvd::destructElement<NiPointer<NiRefObject>>);
	}

	pnew->pspClones = pspClones;
	pnew->pspClones[0] = apObject;
	for (int32_t i = 1; i < 3; ++i) {
		pnew->pspClones[i] = static_cast<NiAVObject*>(apObject->Clone());
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
			for (uint32_t i = 0; i < static_cast<uint32_t>(pMultiboundRoot->GetArrayCount()); ++i) {
				BSMultiBoundNode* pMBNode = static_cast<BSMultiBoundNode*>(pMultiboundRoot->GetAt(i));
				if (pMBNode && !pMBNode->GetMultiBoundRoom()) {
					pSceneNode->UpdateMultiboundVisibility(pMBNode, kCameraPos, kCuller);
				}
			}
		}
	} else {
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
					for (uint32_t j = 0; j < static_cast<uint32_t>(pMultiboundRoot->GetArrayCount()); ++j) {
						NiAVObject* pMBNode = pMultiboundRoot->GetAt(j);
						if (pMBNode) {
							pSceneNode->UpdateMultiboundVisibility(static_cast<BSMultiBoundNode*>(pMBNode), kCameraPos,
								kCuller);
						}
					}
				}

				// The land node is not null-checked.
				pMultiboundRoot = static_cast<NiNode*>(pCell->GetChildNode(TESObjectCELL::CN_LAND));
				for (uint32_t k = 0; k < static_cast<uint32_t>(pMultiboundRoot->GetArrayCount()); ++k) {
					NiAVObject* pMBNode = pMultiboundRoot->GetAt(k);
					if (pMBNode) {
						pSceneNode->UpdateMultiboundVisibility(static_cast<BSMultiBoundNode*>(pMBNode), kCameraPos,
							kCuller);
					}
				}
			}
		}
	}

	NiAVObject* pPotentialGrassRoot = pSceneNode->GetAtChecked(6);
	NiNode* pGrassNode = pPotentialGrassRoot ? pPotentialGrassRoot->IsNode() : nullptr;
	if (pGrassNode) {
		for (uint32_t m = 0; m < static_cast<uint32_t>(pGrassNode->GetArrayCount()); ++m) {
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
			NiTListItem<PlaceableWaterGroup*>* kNextGroup =
				GetWaterSystem()->GetPlaceableWaterGroupList()->GetNextPos(kGroupIter);
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
							pSceneNode->UpdateMultiboundVisibility(static_cast<BSMultiBoundNode*>(pWaterNode->GetAt(0)),
								kCameraPos, kCuller);
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
						pSceneNode->UpdateMultiboundVisibility(static_cast<BSMultiBoundNode*>(pWaterNode->GetAt(0)),
							kCameraPos, kCuller);
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
			NiPointer<BSMultiBoundRoom>
				spPortalRoom(BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)
						->GetPortalGraph()
						->QRoomRoot());
			float fNear = apCamera->GetViewFrustum().m_fNear;
			NiPoint3 kMaxViewPos = apCamera->GetWorldDirection() * fNear;
			NiPoint3 kFrustumEndPos = apCamera->GetWorldTranslate() + kMaxViewPos;
			if (spPortalRoom.operator->()) {
				BSMultiBoundRoom* pRoom = spPortalRoom.operator->();
				if (pRoom->QPointWithin(kFrustumEndPos)) {
					const NiFrustumPlanes& kPlanes = kCuller.GetFrustumPlanes();
					const NiCamera* pCullerCam = kCuller.GetCamera();
					pRoom = spPortalRoom.operator->();
					pSceneNode->UpdateRoomAndPortalVisibility(pRoom, pCullerCam, kPlanes, true);
				} else {
					spPortalRoom = nullptr;
				}
			}

			bool bAddIntersection = true;
			if (!spPortalRoom.operator->()) {
				const NiTPointerList<NiPointer<BSMultiBoundRoom>>& kMultiBoundList =
					pSceneNode->GetPortalGraph()->QRooms();
				if (!kMultiBoundList.IsEmpty()) {
					NiTListItem<NiPointer<BSMultiBoundRoom>>* kIter = kMultiBoundList.GetHeadPos();
					while (kIter) {
						NiPointer<BSMultiBoundRoom>& spRoom = kMultiBoundList.GetNext(kIter);
						BSMultiBoundRoom* pRoom = spRoom.operator->();
						if (pRoom && pRoom->QPointWithin(kFrustumEndPos)) {
							const NiFrustumPlanes& kPlanes = kCuller.GetFrustumPlanes();
							const NiCamera* pCullerCam = kCuller.GetCamera();
							pSceneNode->UpdateRoomAndPortalVisibility(pRoom, pCullerCam, kPlanes, bAddIntersection);
							if (!spPortalRoom.operator->()) {
								spPortalRoom = pRoom;
								bAddIntersection = false;
							}
						}
					}
				}
			}

			if (!spPortalRoom.operator->()) {
				const NiFrustumPlanes& kPlanes = kCuller.GetFrustumPlanes();
				const NiCamera* pCullerCam = kCuller.GetCamera();
				pSceneNode->UpdateRoomAndPortalVisibility(nullptr, pCullerCam, kPlanes, bAddIntersection);
			}

			BSMultiBoundRoom* pRoomRoot = spPortalRoom.operator->();
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
			for (uint32_t i = 0; i < static_cast<uint32_t>(pMultiboundRoot->GetArrayCount()); ++i) {
				BSMultiBoundNode* pMBNode = static_cast<BSMultiBoundNode*>(pMultiboundRoot->GetAt(i));
				if (pMBNode && !pMBNode->GetMultiBoundRoom()) {
					pMBNode->QMultiBound()->QShape()->ResetCullResult();
				}
			}
		}
	} else {
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
					for (uint32_t i = 0; i < static_cast<uint32_t>(pMultiboundRoot->GetArrayCount()); ++i) {
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
					for (uint32_t j = 0; j < static_cast<uint32_t>(pMultiboundRoot->GetArrayCount()); ++j) {
						NiAVObject* pMBNode = pMultiboundRoot->GetAt(j);
						if (pMBNode) {
							static_cast<BSMultiBoundNode*>(pMBNode)->QMultiBound()->QShape()->ResetCullResult();
						}
					}
				}
			}
		}
	}

	NiAVObject* pPotentialGrassRoot =
		BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->GetAtChecked(6);
	NiNode* pGrassNode = pPotentialGrassRoot ? pPotentialGrassRoot->IsNode() : nullptr;
	if (pGrassNode) {
		for (uint32_t k = 0; k < static_cast<uint32_t>(pGrassNode->GetArrayCount()); ++k) {
			NiAVObject* pGrass = pGrassNode->GetAt(k);
			if (pGrass) {
				static_cast<BSMultiBoundNode*>(pGrass)->QMultiBound()->QShape()->ResetCullResult();
			}
		}
	}

	if (BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->GetPortalGraph()) {
		NiTListItem<BSOcclusionPlane*>* kOccluderIter =
			BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)
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

		const NiTPointerList<NiPointer<BSMultiBoundRoom>>& kRooms =
			BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->GetPortalGraph()->QRooms();
		if (!kRooms.IsEmpty() && abRooms) {
			NiTListItem<NiPointer<BSMultiBoundRoom>>* kRoomIter = kRooms.GetHeadPos();
			while (kRoomIter) {
				NiPointer<BSMultiBoundRoom> spRoom(kRooms.GetNext(kRoomIter));
				if (spRoom.operator->()) {
					spRoom.operator->()->UpdateWorldBound();
					spRoom.operator->()->SetCullResult(BSMultiBoundShape::BS_CULL_OCCLUDED);
					spRoom.operator->()->SetCompoundFrustum(nullptr);
					NiTListItem<BSPortal*>* kPortalIter = spRoom.operator->()->QPortalList().GetHeadPos();
					while (kPortalIter) {
						BSPortal* pPortal = spRoom.operator->()->QPortalList().GetNext(kPortalIter);
						if (pPortal) {
							pPortal->ResetCullResult();
							if (pPortal->QSharedGeometryNode()->QCompoundFrustum()) {
								pPortal->QSharedGeometryNode()->QCompoundFrustum()->SetCamera(nullptr);
							}
						}
					}

					NiTListItem<BSOcclusionPlane*>* kRoomOccluderIter =
						spRoom.operator->()->GetOccluders().GetHeadPos();
					while (kRoomOccluderIter) {
						BSOcclusionPlane* pOccluder = spRoom.operator->()->GetOccluders().GetNext(kRoomOccluderIter);
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
			NiTListItem<PlaceableWaterGroup*>* kNextGroup =
				GetWaterSystem()->GetPlaceableWaterGroupList()->GetNextPos(kGroupIter);
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
	static char bdiagnostics = 1;
	static int32_t iexteriorCellCount;
	NiPoint3 ignored;
	NiPoint3 pos;
	MemoryLevelManager::MEM_FREE_DATA freedata;
	if (aiControl == 4 || (aiControl == 5 || aiControl == 9)) {
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
		bRunningCellTests = 0;
		MessageHandler::IncDisableWarningCount(0);
		pTACRegionFilter = 0;
	} else {
		if (aiControl) {
			TAC_Mode = aiControl;
			bRunningCellTests = 1;
			MessageHandler::IncDisableWarningCount(1);
			BSSystemFile::DeleteFileA(TestAllCells_xls);
			if (!bbaseObjectsChecked) {
				Warning("CELLS: \r\n\r\n**************** Base Object Models ****************");
				pDataHandler->CheckModels(0);
				Warning("CELLS: \r\n\r\n**************** Base Object Icons/Textures ****************");
				pDataHandler->CheckIcons();
				bbaseObjectsChecked = 1;
			}
			if ((aiControl == 1 || (aiControl == 4 || (aiControl == 5 || aiControl == 9))) &&
				(!pCurrentWorldSpace && !iinteriorCellCount)) {
				pCurrentWorldSpace = pDataHandler->GetWorldSpaceList();
				bnewWorldNextLoop = 1;
				pDataHandler->SortInteriorCells();
				iinteriorCellCount = pDataHandler->GetInteriorCellCount();
				iinteriorCellIndex = 0;
			} else {
				if (aiControl == 3) {
					if (!pCurrentWorldSpace || pCurrentWorldSpace->m_item != pWorldSpace) {
						for (pCurrentWorldSpace = pDataHandler->GetWorldSpaceList(); pCurrentWorldSpace;
							pCurrentWorldSpace = pCurrentWorldSpace->m_pkNext) {
							TESWorldSpace* pws = pCurrentWorldSpace->m_item;
							if (pws) {
								if (pws == pWorldSpace) {
									break;
								}
							}
						}
					}
					if (pCurrentWorldSpace) {
						bnewWorldNextLoop = 1;
						pDataHandler->SortInteriorCells();
						iinteriorCellCount = pDataHandler->GetInteriorCellCount();
						iinteriorCellIndex = 0;
					}
				} else {
					if (aiControl == 2) {
						char v82 = 0;
						if (pCurrentWorldSpace) {
							pCurrentWorldSpace = 0;
							bnewWorldNextLoop = 0;
							v82 = 1;
						}
						if (!iinteriorCellCount) {
							pDataHandler->SortInteriorCells();
							iinteriorCellCount = pDataHandler->GetInteriorCellCount();
							iinteriorCellIndex = 0;
							v82 = 1;
						}
					}
				}
			}
		}
	}
	bool v84 = 1;
	while (v84) {
		v84 = 0;
		if (bRunningCellTests) {
			v84 = aiControl == 9;
			TESObjectCELL* pcell = 0;
			int32_t start = rt::clock();
			// The game sends a small relative mouse move here (SendInput; dx 2 when imouseDelta is positive, else -1),
			// which keeps the machine from going idle during the test. It is not done here because SDL has no input
			// injection, and SDL keeps the screen saver off itself.
			imouseDelta = -imouseDelta;
			if (bnewWorldNextLoop) {
				if (pCurrentWorldSpace) {
					TESWorldSpace* ptestworld;
				LABEL_45:;
					for (ptestworld = pCurrentWorldSpace->m_item; ptestworld; ptestworld = 0) {
						const char* v2 = ptestworld->GetFormEditorID();
						if (BSstrnicmp(v2, "test", 4u)) {
							break;
						}
						pCurrentWorldSpace = pCurrentWorldSpace->m_pkNext;
						if (pCurrentWorldSpace) {
							goto LABEL_45;
						}
					}
					if (ptestworld) {
						SetWorldSpace(ptestworld);
						iworldCellX = static_cast<int32_t>(pWorldSpace->GetMinX()) >> 12;
						iworldMinY = static_cast<int32_t>(pWorldSpace->MinimumCoords.Y) >> 12;
						int32_t imaxX = static_cast<int32_t>(pWorldSpace->MaximumCoords.X) >> 12;
						int32_t imaxY = static_cast<int32_t>(pWorldSpace->MaximumCoords.Y) >> 12;
						int32_t z = imaxY;
						int32_t y = imaxX;
						int32_t x = iworldMinY;
						int32_t v30 = iworldCellX;
						const char* v3 = pWorldSpace->GetFormEditorID();
						Warning("CELLS: \r\n\r\n**************** %s (%d,%d) - (%d,%d) ****************", v3, v30, x, y,
							z);
					}
				}
				bnewWorldNextLoop = 0;
			}
			char v81 = 0;
			bool bfinishedints = 0;
			while (!v81 && (!bnewWorldNextLoop && !bfinishedints)) {
				if (pCurrentWorldSpace) {
					pcell = pWorldSpace->LoadCell(iworldCellX, iworldMinY);
					if (pcell) {
						AddToExteriorBuffer(pcell);
					}
					++iworldMinY;
					if (iworldMinY > static_cast<int32_t>(pWorldSpace->MaximumCoords.Y) >> 12) {
						iworldMinY = static_cast<int32_t>(pWorldSpace->MinimumCoords.Y) >> 12;
						++iworldCellX;
						if (iworldCellX > static_cast<int32_t>(pWorldSpace->MaximumCoords.X) >> 12) {
							pCurrentWorldSpace = pCurrentWorldSpace->m_pkNext;
							if (pCurrentWorldSpace) {
								bnewWorldNextLoop = 1;
							}
						}
					}
				} else {
					const char* v4;
					if (!iinteriorCellIndex) {
						Warning("CELLS: \r\n\r\n**************** Interiors ****************");
					}
					do {
						pcell = pDataHandler->GetInteriorCellByIndex(iinteriorCellIndex);
						++iinteriorCellIndex;
						if (!pcell) {
							break;
						}
						v4 = pcell->GetFormEditorID();
					} while (!BSstrnicmp(v4, "test", 4u));
					if (!pcell) {
						bfinishedints = 1;
					}
				}
				if (pcell) {
					if (!pcell->GetDelete()) {
						if (pcell->GetFirstRefr()) {
							v81 = 1;
						}
					}
				}
			}
			if (pcell && v81) {
				TESRegionList* pregionList = pcell->GetRegionList(1);
				if (pTACRegionFilter) {
					if (!pregionList || !pregionList->IsInList(pTACRegionFilter)) {
						v81 = 0;
					}
				}
			} else {
				if (bnewWorldNextLoop && pPlayer->GetWorldSpace() != pWorldSpace) {
					v84 = 1;
				}
			}
			if (v81) {
				PLAYER_TARGET_LOC* ploc;
				appCalendar.Update(10.0f);
				pos = NiPoint3_ZERO;
				if (!pcell->GetInterior()) {
					float v50 = static_cast<double>(pcell->GetDataY() << 12) + 2048.0;
					float v49 = static_cast<double>(pcell->GetDataX() << 12) + 2048.0;
					pos = *::new (&ignored) NiPoint3(v49, v50, 0.0f);
				}
				if (pcell->GetInterior()) {
					NiPoint3 ignored_2;
					pcell->LoadAllTempData();
					pcell->GetCOCPlacementInfo(pos, ignored_2);
					LoadInterior(pcell, &pos);
				} else {
					if (pInteriorCell) {
						float z_2;
						LeaveInterior(&pos);
						TESObjectLAND* Land = pcell->GetLand();
						Land->GetLandHeight(pos, z_2);
						if (z_2 >= 0.0) {
							pos.z = z_2;
						}
					} else {
						float z_3;
						UpdateCurrentGridCell(&pos, 0);
						TESObjectLAND* v6 = pcell->GetLand();
						v6->GetLandHeight(pos, z_3);
						if (z_3 >= 0.0) {
							pos.z = z_3;
						}
					}
				}
				PLAYER_TARGET_LOC* apMemory = static_cast<PLAYER_TARGET_LOC*>(ourMalloc(sizeof(PLAYER_TARGET_LOC)));
				if (apMemory) {
					ploc = ::new (apMemory) PLAYER_TARGET_LOC;
				} else {
					ploc = 0;
				}
				PLAYER_TARGET_LOC* ploc_2 = ploc;
				ploc->Location = pos;
				const NiPoint3* AngleOnReference = &pPlayer->GetAngleOnReference();
				ploc_2->Angle = *AngleOnReference;
				ploc_2->bResetWeather = 1;
				if (pcell->GetInterior()) {
					ploc_2->pInterior = pcell;
				} else {
					TESWorldSpace* WorldSpace = pcell->GetWorldSpace();
					ploc_2->pWorld = WorldSpace;
				}
				pPlayer->RequestPositionPlayer(ploc_2);
				pPlayer->HandlePositionPlayerRequest();
				pIOManager->LoadAllQueuedPriorities();
				int32_t stop = rt::clock();
				float time = static_cast<double>(stop - start) / 1000.0;
				MemoryLevelManager::BeginMemoryFree(&freedata, 1, 1, 1);
				TES::pInstance->ClearAllCells(1, 0);
				MemoryLevelManager::FreeReleasedObjects(freedata.bLockedFaceGen);
				MemoryLevelManager::EndMemoryFree(freedata);
				if (bdiagnostics) {
					float fTnumber = 0.0f;
					float fMnumber = 0.0f;
					BSStringT<char> textMnum;
					BGSSceneInfo::CollectSceneDataForLoadedArea(fTnumber, fMnumber, 1, 1);
					BGSSceneInfo::BuildMNumberString(fMnumber, textMnum, 0);
					bool bportaled = 0;
					BSPortalGraph* pportalgraph = pcell->GetPortalGraph();
					if (pportalgraph) {
						if (!pportalgraph->QRooms().IsEmpty()) {
							bportaled = 1;
						}
					}
					uint32_t ultexmemory = 0;
					for (NiTexture* ptex = NiTexture::GetListHead(); ptex; ptex = ptex->m_pkNext) {
						if (ptex->m_pkRendererData) {
							ultexmemory += GetTextureDumpSize(ptex);
						}
					}
					ultexmemory >>= 20;
					int32_t imemLeftHeap = 0;
					void* imemLeftPhysical = 0;
					int32_t imemRemaining = 0;
					uint32_t ihighActors = ReferenceProcessLists.CountHighActors();
					if (pcell->GetInterior()) {
						const char* v46;
						const char* v47;
						if (bportaled) {
							v47 = "Y";
						} else {
							v47 = "N";
						}
						uint32_t za = ihighActors;
						uint32_t ya = ultexmemory;
						uint32_t LightCount = pcell->GetLightCount();
						double fTime = time;
						double v20 = fTnumber;
						char* v16 = textMnum.pString;
						const char* v10 = pcell->GetFormEditorID();
						Warning("CELLS: Cell \"%s\" (Interior) M#: %s T#: %.0f Time: %.1f Lights: %d Portals: %s "
								"TexTotal: %i MB Actors: %i",
							v10, v16, v20, fTime, LightCount, v47, ya, za);
						if (bportaled) {
							v46 = "Y";
						} else {
							v46 = "N";
						}
						uint32_t zb = ihighActors;
						uint32_t yb = ultexmemory;
						uint32_t v32 = pcell->GetLightCount();
						double fTimea = time;
						double v21 = fTnumber;
						char* v17 = textMnum.pString;
						const char* v11 = pcell->GetFormEditorID();
						PrintToFile(TestAllCells_xls, "%s\tInterior\t%s\t%.0f\t%.1f\t%d\t%s\t%d\t%i\r\n", v11, v17, v21,
							fTimea, v32, v46, yb, zb);
					} else {
						const char* v44;
						const char* v45;
						++iexteriorCellCount;
						if (bportaled) {
							v45 = "Y";
						} else {
							v45 = "N";
						}
						uint32_t zc = ihighActors;
						uint32_t yc = ultexmemory;
						double fTime_4 = time;
						double v24 = fTnumber;
						char* v22 = textMnum.pString;
						int32_t DataY = pcell->GetDataY();
						int32_t DataX = pcell->GetDataX();
						const char* v12 = pcell->GetFormEditorID();
						Warning("CELLS: Cell \"%s\" (%d, %d) M#: %s T#: %.0f Time: %.1f Portals: %s TexTotal: %i MB "
								"Actors: %i",
							v12, DataX, DataY, v22, v24, fTime_4, v45, yc, zc);
						if (bportaled) {
							v44 = "Y";
						} else {
							v44 = "N";
						}
						uint32_t zd = ihighActors;
						uint32_t yd = ultexmemory;
						double fTime_4a = time;
						double v25 = fTnumber;
						char* v23 = textMnum.pString;
						int32_t v19 = pcell->GetDataY();
						int32_t v15 = pcell->GetDataX();
						const char* v13 = pcell->GetFormEditorID();
						PrintToFile(TestAllCells_xls, "%s\t%d, %d\t%s\t%.0f\t%.1f\t%s\t%d\t%i\r\n", v13, v15, v19, v23,
							v25, fTime_4a, v44, yd, zd);
					}
				}
				if (pfnTACCallbackFunc) {
					pfnTACCallbackFunc(pcell, pTACCallbackData);
				}
				if (TAC_Mode == 4 || TAC_Mode == 5) {
					pSaveLoadGameOLD->TestAllCells(TAC_Mode);
				}
			}
			if (iinteriorCellIndex >= iinteriorCellCount) {
				iinteriorCellCount = 0;
				bRunningCellTests = 0;
				MessageHandler::IncDisableWarningCount(0);
				pTACRegionFilter = 0;
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
	int32_t v6 = FloatToInt((&acoord)->x);
	int32_t v7 = FloatToInt((&acoord)->y);
	if ((!(v6 % 4096)) && ((&acoord)->x < ((double)v6))) {
		--v6;
	}
	if ((!(v7 % 4096)) && ((&acoord)->y < ((double)v7))) {
		--v7;
	}
	afHeight = (-0x1p+11f);
	TESObjectCELL* a1 = GetGridCellCell(v6 >> 12, v7 >> 12);
	if ((!a1) || (!TES::pInstance->IsCellLoaded(a1, 0))) {
		return (bool)0;
	}
	TESObjectLAND* land = a1->GetLand();
	return (bool)((uint8_t)land->GetLandHeight(acoord, afHeight));
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
	NiPoint3 direction = kPick - kOrigin;
	NiPick kPicker(0, 8u);
	kPicker.SetTarget(pObjRoot);
	kPicker.SetObserveAppCullFlag(1);
	NiAVObject* pObject = 0;
	if (kPicker.PickObjects(kOrigin, direction, 0)) {
		NiPick::Results* pickresults = &kPicker.m_pickResults;
		NiPick::Record* ppickrecord = pickresults->GetAt(0);
		pObject = ppickrecord->GetAVObject();
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
	} else {
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
		InitializationFlow("Interior cell finished loading in %.02f seconds.  Total test time: %02i:%02i:%02i",
			fLoadTime, iHours, iMinutes, iSeconds);
	} else {
		double fLoadTime = static_cast<double>(iLoadEnd - iLoadStart) / 1000.0;
		int32_t iY = apCell->GetDataY();
		int32_t iX = apCell->GetDataX();
		InitializationFlow("Exterior cell (%i,%i) finished loading in %.02f seconds.  Total test time: %02i:%02i:%02i",
			iX, iY, fLoadTime, iHours, iMinutes, iSeconds);
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
	int32_t j;
	int32_t i;
	for (i = 0; i < ((int32_t)((&TES::uInteriorCellBufferGeneral)->UInt())); ++i) {
		if (pInteriorBuffer[i]) {
			if (TES::pInstance->IsCellLoaded(pInteriorBuffer[i], 1)) {
				pInteriorBuffer[i]->UpdateSounds();
			}
		}
	}
	for (j = 0; j < ((int32_t)((&uExteriorCellBuffer)->UInt())); ++j) {
		if (pExteriorBuffer[j]) {
			if (TES::pInstance->IsCellLoaded(pExteriorBuffer[j], 1)) {
				pExteriorBuffer[j]->UpdateSounds();
			}
		}
	}
	LoadedAreaBound* v2 = spLoadedAreaBound.m_pObject;
	v2->Clear();
}

void TES::Revert(BGSLoadGameBuffer* apLoadGameBuffer) {
	ClearDeadCountList();
	iSaveGridX = 2147483647;
	iSaveGridY = 2147483647;
}

void TES::AddTempDebugObject(NiAVObject* apObject, float afTime) {
	if (apObject) {
		BSTempNode* pNode;
		BSTempNode* apMemory = static_cast<BSTempNode*>(NiMemObject::operator new(sizeof(BSTempNode)));
		if (apMemory) {
			pNode = ::new (apMemory) BSTempNode(afTime);
		} else {
			pNode = 0;
		}
		pNode->AttachChild(apObject, 1);
		NiNode* TempNodeManager = pTempNodeManager;
		TempNodeManager->AttachChild(pNode, 1);
		pNode->UpdateProperties();
	}
}

NiGeometry* TES::GetLandGeometry(const NiPoint3& acoord) {
	int32_t icellX = FloatToInt(acoord.x);
	int32_t icellY = FloatToInt(acoord.y);
	if (!(icellX % 0x1000) && acoord.x < static_cast<double>(icellX)) {
		--icellX;
	}
	if (!(icellY % 0x1000) && acoord.y < static_cast<double>(icellY)) {
		--icellY;
	}
	TESObjectCELL* ptestcell = GetGridCellCell(icellX >> 12, icellY >> 12);
	if (!ptestcell || !TES::pInstance->IsCellLoaded(ptestcell, 0)) {
		return 0;
	}
	TESObjectLAND* Land = ptestcell->GetLand();
	return Land->GetGeometry(acoord);
}

bool TES::IsCoordInLoadedArea(const NiPoint3& aCoord) {
	if (TES::GetInterior()) {
		return (bool)1;
	}
	int32_t v4 = FloatToInt((&aCoord)->x) >> 12;
	int32_t v6 = FloatToInt((&aCoord)->y) >> 12;
	int32_t v7 = (int32_t)(iCurrentGridX - (((&uGridsToLoad)->UInt()) >> 1));
	int32_t v5 = (int32_t)(iCurrentGridY - (((&uGridsToLoad)->UInt()) >> 1));
	return (bool)((uint8_t)((v4 >= v7) &&
		((v4 < ((int32_t)(((&uGridsToLoad)->UInt()) + v7))) &&
			((v6 >= v5) && (v6 < ((int32_t)(((&uGridsToLoad)->UInt()) + v5)))))));
}

TESObjectCELL* TES::GetCellForPoint(NiPoint3 aLocation) {
	if (TES::GetInterior()) {
		return TES::GetInterior();
	}
	TESObjectCELL* a1 = GetGridCellCell(((int32_t)aLocation.x) >> 12, ((int32_t)aLocation.y) >> 12);
	if (a1 && TES::pInstance->IsCellLoaded(a1, 1)) {
		return a1;
	} else {
		return nullptr;
	}
}

void TES::UpdateRefSounds() {
	TESObjectCELL* interior;
	if (TES::GetInterior()) {
		interior = TES::GetInterior();
		interior->UpdateRefSounds();
	} else {
		pGridCellA->UpdateRefSounds();
	}
}

void TES::ToggleCollisionGeometry() {
	TESObjectCELL* interior = TES::GetInterior();
	if (interior) {
		interior->ToggleCollisionGeometry();
	} else {
		pGridCellA->ToggleCollisionGeometry();
	}
}

void TES::ShowLandBorders(bool abShow) {
	if (((uint8_t)bShowLANDborders) != ((uint8_t)abShow)) {
		bShowLANDborders = (bool)((uint8_t)abShow);
		if (pDataHandler) {
			if (!TES::GetInterior()) {
				pGridCellA->ShowLandBorders(abShow);
			}
		}
	}
}

bool TES::GetLandNormal(const NiPoint3& aCoord, NiPoint3& aNormal, NiPoint3& aFaceNormal) {
	int32_t v7 = FloatToInt((&aCoord)->x);
	int32_t v8 = FloatToInt((&aCoord)->y);
	if ((!(v7 % 4096)) && ((&aCoord)->x < ((double)v7))) {
		--v7;
	}
	if ((!(v8 % 4096)) && ((&aCoord)->y < ((double)v8))) {
		--v8;
	}
	aNormal = NiPoint3_ZERO;
	(&aNormal)->z = (float)1.0;
	TESObjectCELL* cell = GetGridCellCell(v7 >> 12, v8 >> 12);
	if ((!cell) || (!TES::pInstance->IsCellLoaded(cell, 0))) {
		return (bool)0;
	}
	TESObjectLAND* land = cell->GetLand();
	return (bool)((int8_t)land->GetLandNormal(aCoord, aNormal, aFaceNormal));
}

TESObjectCELL* TES::GetExteriorBufferCellByIndex(int32_t a2) {
	return pExteriorBuffer[a2];
}

uint32_t TES::GetExteriorBufferSize() {
	return uExteriorCellBuffer.UInt();
}

void TES::SetAllowUnusedPurge(bool abAllow) {
	bAllowUnusedPurge = (bool)((uint8_t)abAllow);
}

void TES::SetHideLandNormalMapForGridCell(int32_t aiGridX, int32_t aiGridY, bool abFlag) {
	GridCell* pGridCell = pGridCellA->Get(aiGridX, aiGridY);
	if (pGridCell && pGridCell->pCell) {
		for (int32_t i = 0; i < 4; ++i) {
			NiAVObject* pLandNode = pGridCell->pCell->GetLandNode(i);
			NiGeometry* pLandGeo;
			if (static_cast<NiNode*>(pLandNode)->GetAtChecked(0)) {
				NiAVObject* pChild = static_cast<NiNode*>(pLandNode)->GetAt(0);
				pLandGeo = pChild->IsGeometry();
			} else {
				pLandGeo = nullptr;
			}
			if (pLandGeo) {
				int32_t iType = NiShadeProperty::GetType();
				NiProperty* pProperty = pLandGeo->NiAVObject::GetProperty(iType);
				static_cast<BSShaderProperty*>(pProperty)->SetFlag(BSShaderProperty::BSSP_FLAG_SKIP_NORMAL_MAPS,
					!abFlag);
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
	int32_t type;
	NiGeometry* v5;
	NiAVObject* v6;
	NiProperty* a1;
	NiAVObject* landNode;
	int32_t i;
	GridCell* v10 = pGridCellA->Get(aiGridX, aiGridY);
	if (v10) {
		if (v10->pCell) {
			for (i = 0; i < 4; ++i) {
				landNode = v10->pCell->GetLandNode(i);
				if (static_cast<NiNode*>(landNode)->GetAtChecked(0)) {
					v6 = static_cast<NiNode*>(landNode)->GetAt(0);
					v5 = v6->IsGeometry();
				} else {
					v5 = nullptr;
				}
				if (v5) {
					type = NiShadeProperty::GetType();
					a1 = v5->NiAVObject::GetProperty(type);
					static_cast<BSShaderProperty*>(a1)->SetFlag(BSShaderProperty::BSSP_FLAG_SKIP_LANDLO_PASS, abFlag);
				}
			}
		}
	}
}

bool TES::GetShowLandBorders() {
	return (bool)((uint8_t)bShowLANDborders);
}

bool TES::CellTestRunning2() {
	return (bool)((uint8_t)bRunningCellTests2);
}

void TES::RemoveCellFromInteriorBuffer(TESObjectCELL* apCell) {
	uint32_t i;
	for (i = 0; i < ((&TES::uInteriorCellBufferGeneral)->UInt()); ++i) {
		if (apCell == pInteriorBuffer[i]) {
			pDataHandler->UnloadCell(apCell);
			pInteriorBuffer[i] = nullptr;
			break;
		}
	}
	while (i < (((&TES::uInteriorCellBufferGeneral)->UInt()) - 1)) {
		pInteriorBuffer[i] = pInteriorBuffer[i + 1];
		++i;
	}
	pInteriorBuffer[((&TES::uInteriorCellBufferGeneral)->UInt()) - 1] = nullptr;
	SetUpdateGridString();
}

bool TES::GetLandColor(const NiPoint3& acoord, NiColorA& acolor) {
	int32_t v6 = FloatToInt((&acoord)->x);
	int32_t v7 = FloatToInt((&acoord)->y);
	if ((!(v6 % 4096)) && ((&acoord)->x < ((double)v6))) {
		--v6;
	}
	if ((!(v7 % 4096)) && ((&acoord)->y < ((double)v7))) {
		--v7;
	}
	TESObjectCELL* cell = GetGridCellCell(v6 >> 12, v7 >> 12);
	if ((!cell) || (!TES::pInstance->IsCellLoaded(cell, 0))) {
		return (bool)0;
	}
	TESObjectLAND* land = cell->GetLand();
	return (bool)((int8_t)land->GetLandColor(acoord, acolor));
}

void TES::QueueReferencesForAttachedCells() {
	TESObjectCELL* interior;
	TESObjectCELL* v2;
	TESObjectCELL* pCell;
	uint32_t y;
	uint32_t x;
	if (pDataHandler) {
		if (TES::GetInterior()) {
			interior = TES::GetInterior();
			if (interior->GetAttached()) {
				v2 = TES::GetInterior();
				v2->QueueReferences(0);
			}
		}
		for (x = 0; x < ((&uGridsToLoad)->UInt()); ++x) {
			for (y = 0; y < ((&uGridsToLoad)->UInt()); ++y) {
				pCell = pGridCellA->Get(x, y)->pCell;
				if (pCell) {
					if (pCell->GetAttached()) {
						pCell->QueueReferences(0);
					}
				}
			}
		}
	}
}

void TES::UpdateFadeNodesForAttachedCells() {
	TESObjectCELL* interior;
	TESObjectCELL* v2;
	TESObjectCELL* pCell;
	uint32_t y;
	uint32_t x;
	if (pDataHandler) {
		if (TES::GetInterior()) {
			interior = TES::GetInterior();
			if (interior->GetAttached()) {
				v2 = TES::GetInterior();
				v2->UpdateFadeNodes();
			}
		}
		for (x = 0; x < ((&uGridsToLoad)->UInt()); ++x) {
			for (y = 0; y < ((&uGridsToLoad)->UInt()); ++y) {
				pCell = pGridCellA->Get(x, y)->pCell;
				if (pCell) {
					if (pCell->GetAttached()) {
						pCell->UpdateFadeNodes();
					}
				}
			}
		}
	}
}

void TES::InitDistantTextureBlending() {
	TESObjectLAND* land;
	uint32_t y;
	uint32_t x;
	TESObjectCELL* cell;
	for (x = 0; x < ((&uGridsToLoad)->UInt()); ++x) {
		for (y = 0; y < ((&uGridsToLoad)->UInt()); ++y) {
			cell = pGridCellA->Get(x, y)->pCell;
			if (cell) {
				land = cell->GetLand();
			} else {
				land = nullptr;
			}
			if (cell) {
				if (land) {
					if (land->HasVertices()) {
						land = cell->GetLand();
						land->InitDistantTextureBlending();
					}
				}
			}
		}
	}
}

void TES::UpdateGridLights() {
	TESObjectCELL* pCell;
	uint32_t y;
	uint32_t x;
	for (x = 0; x < ((&uGridsToLoad)->UInt()); ++x) {
		for (y = 0; y < ((&uGridsToLoad)->UInt()); ++y) {
			pCell = pGridCellA->Get(x, y)->pCell;
			if (pCell) {
				if (pCell->GetAttached()) {
					pCell->AttachLights(1);
				}
			}
		}
	}
	ShadowSceneNode* shadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
	shadowSceneNode->UpdateLighting(1, 1);
}

void TES::UpdateDeferredCellAnimationTransforms() {
	TESObjectCELL* interior = TES::GetInterior();
	if (interior) {
		interior->UpdateManagedNodeTransforms();
	} else {
		pGridCellA->UpdateManagedNodeTransforms();
	}
}

void TES::ToggleCollision() {
	bhkWorldM* exteriorbhkWorld;
	int8_t v2 = (int8_t)(((int8_t)TES::QCollisionDisableFlag()) == 0);
	TES::SetCollisionDisableFlag(v2);
	TESObjectCELL* interior = TES::pInstance->TES::GetInterior();
	if (interior) {
		exteriorbhkWorld = static_cast<bhkWorldM*>(interior->GetbhkWorld());
	} else {
		exteriorbhkWorld = TESObjectCELL::ExteriorWorld();
	}
	if (exteriorbhkWorld) {
		exteriorbhkWorld->bhkWorld::SetEnabled(v2 == 0);
	}
	bhkRagdollController::SetRagdollAnim(v2 == 0);
}

void TES::GetLoadedAreaRect(tagRECT& aOutRect, float afOffsetInside) {
	uint32_t v6 = iCurrentGridX - (((&uGridsToLoad)->UInt()) >> 1);
	uint32_t v4 = iCurrentGridY - (((&uGridsToLoad)->UInt()) >> 1);
	uint32_t v7 = iCurrentGridX + (((&uGridsToLoad)->UInt()) >> 1);
	uint32_t v5 = iCurrentGridY + (((&uGridsToLoad)->UInt()) >> 1);
	(&aOutRect)->left = (int32_t)(((double)((int32_t)(v6 << 12))) + afOffsetInside);
	(&aOutRect)->bottom = (int32_t)(((double)((int32_t)(v4 << 12))) + afOffsetInside);
	(&aOutRect)->right = (int32_t)(((double)((int32_t)((v7 << 12) + 4096))) - afOffsetInside);
	(&aOutRect)->top = (int32_t)(((double)((int32_t)((v5 << 12) + 4096))) - afOffsetInside);
}

bool TES::AllCellsInGridLoaded() {
	TESWorldSpace* worldSpace;
	TESObjectCELL* apCell;
	GridCell* v5;
	uint32_t y;
	uint32_t x;
	if (!TES::pInstance->GetWorldSpace()) {
		return (bool)0;
	}
	uint32_t v8 = iCurrentGridX - (((&uGridsToLoad)->UInt()) >> 1);
	uint32_t v9 = iCurrentGridY - (((&uGridsToLoad)->UInt()) >> 1);
	for (x = 0; x < ((&uGridsToLoad)->UInt()); ++x) {
		for (y = 0; y < ((&uGridsToLoad)->UInt()); ++y) {
			v5 = pGridCellA->Get(x, y);
			if (v5) {
				apCell = v5->pCell;
				if (!v5->pCell) {
					worldSpace = TES::pInstance->GetWorldSpace();
					apCell = worldSpace->GetCellFromCellCoord(x + v8, y + v9);
				}
				if ((!apCell) || (!IsExteriorBuffered(apCell))) {
					return (bool)0;
				}
			}
		}
	}
	return (bool)1;
}

GridCell* TES::FindFirstGridCellWithWater() {
	uint32_t k;
	GridCell* v6;
	uint32_t j;
	uint32_t v9;
	uint32_t v10;
	uint32_t y;
	GridCell* v12;
	uint32_t i;
	GridCell* pCell = nullptr;
	uint32_t _2grids = ((&uGridsToLoad)->UInt()) >> 1;
	for (i = 0; i < _2grids; ++i) {
		if (i) {
			y = i + _2grids;
			v10 = i + _2grids;
			v9 = _2grids - i;
			for (j = _2grids - i; j <= v10; ++j) {
				v6 = pGridCellA->Get(j, y);
				if (v6 && (v6->pCell && v6->pCell->HasWater())) {
					pCell = v6;
					break;
				}
				v6 = pGridCellA->Get(j, v9);
				if (v6 && (v6->pCell && v6->pCell->HasWater())) {
					pCell = v6;
					break;
				}
			}
			if (!pCell) {
				for (k = v9 + 1; k < y; ++k) {
					v6 = pGridCellA->Get(_2grids - i, k);
					if (v6 && (v6->pCell && v6->pCell->HasWater())) {
						pCell = v6;
						break;
					}
					v6 = pGridCellA->Get(v10, k);
					if (v6 && (v6->pCell && v6->pCell->HasWater())) {
						pCell = v6;
						break;
					}
				}
			}
		} else {
			v12 = pGridCellA->Get(_2grids, _2grids);
			if (v12) {
				if (v12->pCell) {
					if (v12->pCell->HasWater()) {
						pCell = v12;
					}
				}
			}
		}
		if (pCell) {
			break;
		}
	}
	return pCell;
}

void TES::ShowCellNode(int32_t aiNode, bool abShow, bool abActiveCellsOnly) {
	TESObjectCELL* interior;
	NiNode* waterRoot;
	TESObjectCELL* v6;
	int32_t v7;
	NiAVObject* v9;
	TESObjectCELL* pCell;
	uint32_t aiY;
	uint32_t aiX;
	NiAVObject* v13;
	TESObjectCELL* v14;
	int32_t j;
	NiAVObject* childNode;
	int32_t i;
	TESObjectCELL::SetNodeCulled(static_cast<TESObjectCELL::CELL_CULL_NODE>(aiNode), !abShow);
	if (aiNode == 11) {
		if ((!TES::pInstance->TES::GetInterior()) ||
			((interior = TES::pInstance->TES::GetInterior()), (!interior->HasWater()))) {
			waterRoot = TESWaterSystem::GetWaterRoot();
			waterRoot->SetAppCulled(!abShow);
		}
	} else {
		if (abActiveCellsOnly) {
			v7 = 0;
		} else {
			v7 = (int32_t)((&TES::uInteriorCellBufferGeneral)->UInt());
		}
		for (i = (-1); i < v7; ++i) {
			if (i == ((-1))) {
				v6 = TES::GetInterior();
			} else {
				v6 = pInteriorBuffer[i];
			}
			if (v6) {
				if (((uint32_t)aiNode) <= 8) {
					childNode = v6->GetChildNode(static_cast<TESObjectCELL::CELLNODE>(aiNode));
					if (childNode) {
						childNode->SetAppCulled(!abShow);
					}
					v6->CullChildrenOfMultibounds(!abShow, static_cast<TESObjectCELL::CELL_CULL_NODE>(aiNode));
				}
			}
		}
		if (abActiveCellsOnly) {
			for (aiX = 0; aiX < ((&uGridsToLoad)->UInt()); ++aiX) {
				for (aiY = 0; aiY < ((&uGridsToLoad)->UInt()); ++aiY) {
					pCell = TES::pInstance->GetGridCell(aiX, aiY)->pCell;
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
							v9 = pCell->GetChildNode(static_cast<TESObjectCELL::CELLNODE>(aiNode));
							if (v9) {
								v9->SetAppCulled(!abShow);
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
		} else {
			for (j = 0; j < ((int32_t)((&uExteriorCellBuffer)->UInt())); ++j) {
				v14 = pExteriorBuffer[j];
				if (v14) {
					switch (aiNode) {
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 7:
					case 8: {
						v13 = v14->GetChildNode(static_cast<TESObjectCELL::CELLNODE>(aiNode));
						if (v13) {
							v13->SetAppCulled(!abShow);
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

void TES::LoadGridCell(int32_t aiGridX, int32_t a2, int32_t a3, int32_t a4) {
	if (pWorldSpace) {
		TESObjectCELL* pcell = pWorldSpace->LoadCell(a3, a4);
		if (!pcell) {
			TESWorldSpace* WorldSpace = pWorldSpace;
			pcell = pDataHandler->GetCellFromCellCoord(a3, a4, WorldSpace, 1);
			pWorldSpace->AssignPersistentRefsToCell(pcell);
			pcell->SetHasTempData(1);
		}
		if (pGridCellA->GetAttached(static_cast<uint32_t>(aiGridX), static_cast<uint32_t>(a2))) {
			AddToBuffer(pcell);
		} else {
			char v15[268];
			int32_t CastingType = pcell->iFormID;
			int32_t DataY = pcell->GetDataY();
			int32_t DataX = pcell->GetDataX();
			std::sprintf(v15, "Loading cell...%s (%i, %i) (%08X)", pcell->GetCellName(), DataX, DataY, CastingType);
			pGridCellA->Set(static_cast<uint32_t>(aiGridX), static_cast<uint32_t>(a2), pcell);
			pGridCellA->AttachToWorld(pObjRoot, static_cast<uint32_t>(aiGridX), static_cast<uint32_t>(a2));
			TESObjectLAND* pLand = pcell->GetLand();
			if (!pLand->LoadVertices(1)) {
				pLand->UpdateMesh(0, 0);
			}
			TESObjectLAND* pLand_2 = pcell->GetLand();
			NiPoint2 extents = pLand_2->GetMinMaxLandHeight();
			double WaterHeight_0 = pcell->GetWaterHeight();
			pcell->SetHasWater(extents.x < WaterHeight_0);
			if (TESWaterSystem::bUseWater.Bool() && pcell->HasWater() && !pcell->bAutoWaterLoaded) {
				BGSAutoWater::GeneratePlaceableWaterForCell(pcell);
			}
			pLand->DrawLandBorder(bShowLANDborders);
		}
	}
}

int32_t TES::GetFaceCount(NiNode* apRoot, bool abCountculled) {
	int32_t icount = 0;
	if (!apRoot) {
		return icount;
	}
	for (uint32_t n = 0;; ++n) {
		uint32_t ArrayCount = apRoot->GetArrayCount();
		if (n >= ArrayCount) {
			break;
		}
		NiPointer<NiAVObject> child(apRoot->GetAt(n));
		if (child.m_pObject) {
			NiNode* v8 = static_cast<NiNode*>(child.m_pObject);
			if (v8->IsNode()) {
				int32_t FaceCount = GetFaceCount(static_cast<NiNode*>(child.m_pObject), abCountculled);
				icount += FaceCount;
			} else {
				NiNode* v7 = static_cast<NiNode*>(child.m_pObject);
				if (v7->IsTriBasedGeom()) {
					NiTriBasedGeom* triGeom = static_cast<NiTriBasedGeom*>(child.m_pObject);
					if (abCountculled || !triGeom->GetAppCulled()) {
						uint16_t TriCount = triGeom->GetTriCount();
						icount += TriCount;
					}
				}
			}
		}
	}
	return icount;
}

void TES::InitGridCells(const NiPoint3* apPos, bool abFullreload) {
	bool bsavegameLoading = pSaveLoadGameOLD->GetLoading();
	if ((pDataHandler && (!pInteriorCell && !pDataHandler->GetSaveLoad())) || bsavegameLoading) {
		int32_t ioldgridx = this->GetCurrentGridX();
		int32_t ioldgridy = this->GetCurrentGridY();
		if (!pWorldSpace) {
			SetWorldSpace(static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item));
		}
		if (apPos) {
			iCurrentGridX = FloatToInt(apPos->x) >> 12;
			iCurrentGridY = FloatToInt(apPos->y) >> 12;
		} else {
			iCurrentGridX = ioldgridx;
			iCurrentGridY = ioldgridy;
		}
		iCurrentQueuedX = this->GetCurrentGridX();
		iCurrentQueuedY = this->GetCurrentGridY();
		ReloadGridCells(apPos, abFullreload);
		TESWaterListener::RefreshExteriorWaterHeights();
	}
}

bool TES::ClearOldestBufferedCell() {
	TESObjectCELL* v15 = 0;
	if (GetInterior()) {
		int32_t CastingType = pGridCellA->GetDimension();
		uint32_t v11 = pGridCellA->GetDimension() * CastingType;
		for (uint32_t i = uExteriorCellBuffer.UInt() - 1; i >= v11; --i) {
			if (pExteriorBuffer[i] &&
				(!IsCellLoaded(pExteriorBuffer[i], 1) && !pExteriorBuffer[i]->IsRefListLocked())) {
				v15 = pExteriorBuffer[i];
				pExteriorBuffer[i] = 0;
				break;
			}
		}
		if (!v15) {
			for (int32_t j = TES::uInteriorCellBufferGeneral.UInt() - 1; j >= 0; --j) {
				if (this->pInteriorBuffer[j]) {
					TESObjectCELL** pInteriorBuffer = this->pInteriorBuffer;
					if (pInteriorBuffer[j] != GetInterior()) {
						v15 = this->pInteriorBuffer[j];
						this->pInteriorBuffer[j] = 0;
						break;
					}
				}
			}
		}
		if (!v15) {
			for (uint32_t k = 0; k < v11; ++k) {
				if (pExteriorBuffer[k] &&
					(!IsCellLoaded(pExteriorBuffer[k], 1) && !pExteriorBuffer[k]->IsRefListLocked())) {
					v15 = pExteriorBuffer[k];
					pExteriorBuffer[k] = 0;
					DetachGridCells();
					break;
				}
			}
		}
	} else {
		for (int32_t m = TES::uInteriorCellBufferGeneral.UInt() - 1; m >= 0; --m) {
			if (this->pInteriorBuffer[m]) {
				v15 = this->pInteriorBuffer[m];
				this->pInteriorBuffer[m] = 0;
				break;
			}
		}
		if (!v15) {
			SortExteriorBuffer();
			int32_t v1 = pGridCellA->GetDimension();
			uint32_t v13 = pGridCellA->GetDimension() * v1;
			for (uint32_t n = uExteriorCellBuffer.UInt() - 1; n >= v13; --n) {
				if (pExteriorBuffer[n]) {
					if (!IsCellLoaded(pExteriorBuffer[n], 1) && !pExteriorBuffer[n]->IsRefListLocked()) {
						v15 = pExteriorBuffer[n];
						pExteriorBuffer[n] = 0;
					}
					break;
				}
			}
		}
	}
	if (!v15) {
		return 0;
	}
	pDataHandler->UnloadCell(v15);
	if (!*pExteriorBuffer) {
		if (GetWorldSpace()) {
			TESWorldSpace* WorldSpace = GetWorldSpace();
			BGSTerrainManager* TerrainManager = WorldSpace->GetTerrainManager();
			TerrainManager->UnloadAll();
		}
	}
	return 1;
}

void TES::MoveToEndOfExteriorBuffer(TESObjectCELL* apCell) {
	if (apCell) {
		int32_t v4 = 0;
		int32_t uExteriorCellBuffer = ::uExteriorCellBuffer.operator uint32_t() - 1;
		while (v4 < uExteriorCellBuffer) {
			if (apCell == pExteriorBuffer[v4]) {
				while (v4 < uExteriorCellBuffer) {
					TESObjectCELL* v3 = pExteriorBuffer[v4 + 1];
					if (!v3 || !v3->GetAttached()) {
						break;
					}
					pExteriorBuffer[v4++] = v3;
				}
				pExteriorBuffer[v4] = apCell;
				return;
			}
			if (!pExteriorBuffer[v4]) {
				return;
			}
			++v4;
		}
	}
}

void TES::SaveWorld(const char* apFileName) {
	if (apFileName) {
		bhkWorldM* ExteriorbhkWorld;
		TESObjectCELL* Interior = TES::pInstance->GetInterior();
		if (Interior) {
			ExteriorbhkWorld = static_cast<bhkWorldM*>(Interior->GetbhkWorld());
		} else {
			ExteriorbhkWorld = TESObjectCELL::ExteriorWorld();
		}
		if (ExteriorbhkWorld) {
			char Dst[268];
			BSstrcpy(Dst, 260, apFileName);
			BSstrcat(Dst, 260, ".hkx");
			ExteriorbhkWorld->SaveSnapshot(Dst);
		}
	}
}

// Returns a reference to the setting's value; apText is unused.
bool& TES::ShowLoadingMessage(const char* apText) {
	return gs_bShowLoadingAreaMessage_General.Bool();
}

void TES::AddDeadCount(TESActorBase* apObject, int16_t ascount) {
	for (DeathCount* pdead : DeadCount.ItemsUntilNull()) {
		if (pdead->pActorBase == apObject) {
			pdead->usCount += ascount;
			return;
		}
	}
	DeathCount* pdead_2 = static_cast<DeathCount*>(ourMalloc(sizeof(DeathCount)));
	pdead_2->pActorBase = apObject;
	pdead_2->usCount = ascount;
	DeadCount.AddHead(pdead_2);
}

uint16_t TES::GetSaveSize() {
	int16_t v5 = 0;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		v5 = 6;
	}
	uint16_t isize = 6 * DeadCount.ItemsInList() + (v5 + 4);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 50u) {
		isize += 4;
	}
	if (bDebugSaveBuffer.Bool()) {
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
			InitializationFlow("GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				isize, pheader->iFormID, pform->GetFormEditorID(), pheader->iFlags, 6274,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp");
		} else {
			InitializationFlow("GetSaveSize(): %-5i ending at line %i in file %s", isize, 6274,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp");
		}
	}
	return isize;
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
		pSaveLoadGameOLD->SaveGameDataOLD(&uiTag, 4u);
		pBlockSize = pSaveLoadGameOLD->m_pBuffer;
		pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, 2u);
	}
	BSSimpleList<DeathCount*>* pList = &DeadCount;
	int32_t iCount = pList->ItemsInList();
	pSaveLoadGameOLD->SaveGameDataOLD(&iCount, 4u);
	while (pList) {
		DeathCount* pDead = pList->GetItem();
		if (pDead) {
			uint32_t uiFormID = 0;
			if (pDead->pActorBase) {
				uiFormID = pDead->pActorBase->iFormID;
			}
			pSaveLoadGameOLD->SaveNumericID(&uiFormID, 4u);
			pSaveLoadGameOLD->SaveGameDataOLD(&pDead->usCount, 2u);
		}
		pList = pList->GetNext();
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 50u) {
		pSaveLoadGameOLD->SaveGameDataOLD(&fCellAppTime, 4u);
	}
	if (bDebugSaveBuffer.Bool()) {
		char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
		SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pHeader) {
			TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
			uint32_t iFlags = pHeader->iFlags;
			const char* pEditorID = pForm->GetFormEditorID();
			InitializationFlow("SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				static_cast<int32_t>(pEnd - pStart), pHeader->iFormID, pEditorID, iFlags, 6369,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp");
		} else {
			InitializationFlow("SaveGame(): %-5i ending at line %i in file %s", static_cast<int32_t>(pEnd - pStart),
				6369, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp");
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		char* pBlockEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
		char* pBlockStart = static_cast<char*>(pBlockSize);
		if (pBlockEnd > pBlockStart + 0xFFFF) {
			Warning("SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TES.cpp", 6369);
		}
		*static_cast<uint16_t*>(pBlockSize) = static_cast<uint16_t>(pBlockEnd - pBlockStart);
	}
}

void TES::SaveGameBGS(BGSSaveGameBuffer* a2) {
	uint32_t apData;
	uint32_t aiVariableValue = 0;
	uint32_t aiPosition = a2->StartVariableSizedValue();
	for (DeathCount* m_item : DeadCount) {
		if (m_item) {
			a2->SaveFormID(m_item->pActorBase, 0);
			a2->SaveDataEndian(&m_item->usCount, 2u, 0);
			++aiVariableValue;
		}
	}
	a2->SaveVariableSizedValue(aiVariableValue, aiPosition);
	apData = uGridsToLoad.Int();
	a2->SaveDataEndian(&apData, 4u, 0);
	for (uint32_t x = 0; x < apData; ++x) {
		for (uint32_t y = 0; y < apData; ++y) {
			uint32_t v2;
			uint32_t CastingType;
			TESObjectCELL* pCell = pGridCellA->Get(x, y)->pCell;
			if (pCell && (v2 = pCell->iFormID, pDataHandler->IsFormIDCreated(v2))) {
				CastingType = pCell->iFormID;
			} else {
				CastingType = 0;
			}
			a2->SaveFormID(CastingType, 0);
		}
	}
}

void TES::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	uint32_t igridSize;
	uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
	for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
		DeathCount* pdeadCount = static_cast<DeathCount*>(ourMalloc(sizeof(DeathCount)));
		uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
		TESActorBase* v4 = dynamic_cast<TESActorBase*>(FormByNumericID);
		pdeadCount->pActorBase = v4;
		apLoadGameBuffer->LoadData(&pdeadCount->usCount, 2u);
		DeadCount.AddHead(pdeadCount);
	}
	igridSize = 0;
	apLoadGameBuffer->LoadData(&igridSize, 4u);
	for (uint32_t x = 0; x < igridSize; ++x) {
		for (uint32_t y = 0; y < igridSize; ++y) {
			TESObjectCELL* pcell = pGridCellA->Get(x, y)->pCell;
			uint32_t icellID = 0;
			apLoadGameBuffer->LoadFormID(icellID);
			if (pcell) {
				if (icellID) {
					uint32_t CastingType = pcell->iFormID;
					if (pDataHandler->IsFormIDCreated(CastingType)) {
						pSaveLoadGame->ClearFormID(icellID);
						pcell->SetFormID(icellID, 1);
					}
				}
			}
		}
	}
}

uint32_t TES::GetUnusedCachedParticleObject(NiAVObject* apModel, NiPointer<NiAVObject>& arInstance) {
	TES::ParticleObjectCache* pcache;
	arInstance.operator=(static_cast<NiAVObject*>(0));
	uint32_t iresult = 0;
	for (pcache = pParticleCacheHead; pcache; pcache = pcache->pNext) {
		if (pcache->spModel.operator==(apModel)) {
			iresult = 1;
			break;
		}
	}
	if (pcache && pcache->pspClones) {
		for (int32_t i = 0; i < 3; ++i) {
			if (pcache->pspClones[i].m_pObject && !pcache->pspClones[i].m_pObject->m_pkParent) {
				arInstance.operator=(pcache->pspClones[i]);
				return 2;
			}
		}
	}
	return iresult;
}

void TES::CleanupTemporaryParticleCache() {
	TES::ParticleObjectCache* v10;
	for (TES::ParticleObjectCache* i = pParticleCacheHead; i; i = v10) {
		v10 = i->pNext;
		delete i;
	}
	pParticleCacheHead = 0;
	if (*sBloodParticleDefault.String()) {
		const char* v1 = sBloodParticleDefault.String();
		pModelLoader->ReleaseFile(v1);
	}
	if (*sSplashParticles.String()) {
		const char* v2 = sSplashParticles.String();
		pModelLoader->ReleaseFile(v2);
	}
	if (*sBloodParticleMeleeDefault.String()) {
		const char* v3 = sBloodParticleMeleeDefault.String();
		pModelLoader->ReleaseFile(v3);
	}
	if (*sImpactParticleMetalDefault.String()) {
		const char* v4 = sImpactParticleMetalDefault.String();
		pModelLoader->ReleaseFile(v4);
	}
	if (*sImpactParticleConcreteDefault.String()) {
		const char* v5 = sImpactParticleConcreteDefault.String();
		pModelLoader->ReleaseFile(v5);
	}
	if (*sImpactParticleWoodDefault.String()) {
		const char* v6 = sImpactParticleWoodDefault.String();
		pModelLoader->ReleaseFile(v6);
	}
	if (*sDismemberParticleDefault.String()) {
		const char* v7 = sDismemberParticleDefault.String();
		pModelLoader->ReleaseFile(v7);
	}
	if (*sDismemberRobotParticleDefault.String()) {
		const char* v8 = sDismemberRobotParticleDefault.String();
		pModelLoader->ReleaseFile(v8);
	}
}

void TES::HideUnneededLandPasses() {
	if (uGridsToLoad.Int() == 5) {
		for (int32_t x = 0; x < pGridCellA->iDimension; ++x) {
			for (int32_t y = 0; y < pGridCellA->iDimension; ++y) {
				bool bsetting = 0;
				if (TES::bLODFadeInProgress) {
					if (x == 2) {
						bsetting = y == 2;
					}
				} else {
					bsetting = TES::kLandBlendPassMask[x][y];
				}
				SetHideLandBlendPassForGridCell(x, y, bsetting);
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
	case CMNY_ID:
		return LOD_MULT_ITEMS;
	case NPC__ID:
	case CREA_ID:
		return LOD_MULT_ACTORS;
	default:
		return LOD_MULT_OBJECTS;
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
	TES::bCollisionDisable = (bool)((uint8_t)abTF);
}

void TES::RangedNodeVisibilityChangedCB(NiNode* apNode, bool abVisible) {
	if (abVisible) {
		TESObjectREFR::AddMasterParticleAddonNodes(apNode);
	} else {
		TESObjectREFR::RemoveMasterParticleAddonNodes(apNode);
	}
}

bool TES::GetSelectivePurgeUnusedOnFastTravel() {
	return (bool)((uint8_t)(&gs_bSelectivePurgeUnusedOnFastTravel_BackgroundLoad)->operator bool());
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

BSPerformanceTimer::BSPerformanceTimer(const char* apSystem, const char* apName, bool abDisplayPerFrame,
	bool abGlobalReset) {}

void BSPerformanceTimer::Start() {}

void BSPerformanceTimer::Stop() {}

// m_to and kLength are reached by name rather than as neighbours of m_from, so the 64-bit layouts reach them too.
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
	return spSharedGeometry.operator->();
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
	return (bool)((int8_t)BSTreeManager::bUseCanopyShadows);
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
	} else {
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
