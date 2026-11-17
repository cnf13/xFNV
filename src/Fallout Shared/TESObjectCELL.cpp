#include "Fallout Shared/TESObjectCELL.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSMap.h"
#include "BSCore/BSScrapMap.h"
#include "BSCore/BSSemaphore.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/ScrapHeap.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkEntityListener.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkTrapListener.h"
#include "BSHavok/bhkWindListener.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldM.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundOBB.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPortal.h"
#include "BSMain/BSPortalGraph.h"
#include "BSMain/BSPortalSharedNode.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/IOManager.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/BSSystemDir.h"
#include "BSSystem/BSSystemFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSAcousticSpace.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BGSLightingTemplate.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/BGSPrimitive.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRagdoll.h"
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
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSAcousticSpaceListener.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/CellMopp.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/Havok/FOIslandActivationListener.h"
#include "Fallout Shared/Havok/TESArrowTargetListener.h"
#include "Fallout Shared/Havok/TESTrapListener.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/Havok/TESWindListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshArray.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionDataGrass.h"
#include "Fallout Shared/Region System/TESRegionDataList.h"
#include "Fallout Shared/Region System/TESRegionDataManager.h"
#include "Fallout Shared/Region System/TESRegionDataObjects.h"
#include "Fallout Shared/Region System/TESRegionGrassObject.h"
#include "Fallout Shared/Region System/TESRegionGrassObjectList.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Region System/TESRegionObject.h"
#include "Fallout Shared/Region System/TESRegionObjectList.h"
#include "Fallout Shared/Region System/TESRegionPointList.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/SpeedTree/BSTreeModel.h"
#include "Fallout Shared/SpeedTree/BSTreeNode.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpace.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESLandTexture.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTerrainLODManager.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
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
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Fallout/Misc/SeenData.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiGeomMorpherController.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9DynamicTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <cmath>
#include <cstdint>

namespace HavokWorldProps {} // namespace HavokWorldProps

static float data_118B6A8 = 65536.0f;
static float data_118B6AC = 65536.0f;
static float data_118B6B0 = 10000.0f;
static BGSMusicType* ptypeLast;

// GetMusicType's guarded static position (its guard is lastloc_guard).
static NiPoint3 lastloc{};
static uint32_t lastloc_guard;

// The game settings this file defines, with their IDs and values, in the game's initialization order.
TESObjectCELL::MultiboundTestMapType TESObjectCELL::MultiboundTestMap(37);
TESObjectCELL::QueuedChildAttachMapType TESObjectCELL::QueuedChildAttachMap(37);
SettingT<GameSettingCollection> TESObjectCELL::fSeenDataUpdateRadius("fSeenDataUpdateRadius", 1024.0f);
BSSimpleList<TESObjectREFR*> TESObjectCELL::AutoWaterReflectList;
BSSimpleList<TESObjectREFR*> TESObjectCELL::AutoWaterRefractList;
BSSpinLock TESObjectCELL::QueuedParticleLock;
NiPointer<bhkWorldM> TESObjectCELL::spExteriorWorld;
SettingT<INISettingCollection> bUseMultibounds("bUseMultibounds:General", true);
SettingT<INISettingCollection> bDefaultCOCPlacement("bDefaultCOCPlacement:General", false);
SettingT<GameSettingCollection> TESObjectCELL::sDefaultCellName("sDefaultCellName", "Wilderness");
SettingT<GameSettingCollection> gs_iHoursToRespawnCell("iHoursToRespawnCell", 72);
TESObjectCELL::QueuedParticleUpdateList TESObjectCELL::QueuedParticleUpdates;

using QueuedChildAttachMap_t = TESObjectCELL::QueuedChildAttachMapType;

void* ConstructQueuedAttachMapItem(void* apMemory, [[maybe_unused]] void* apEdx) {
    ::new (apMemory) NiTMapItem<NiAVObject*, TESObjectCELL::QUEUED_ATTACH>;
    return apMemory;
}

float GetDistanceBetweenPoints(float afX1, float afY1, float afX2, float afY2) {
    float v9 = afX1 - afX2;
    float v7 = afY1 - afY2;
    float v10 = v9 * v9;
    float v8 = v7 * v7;
    float afX = v10 + v8;
    return std::sqrt(afX);
}

void SyncFromHavok(NiNode* arg0) {
    for (uint32_t a1 = 0; a1 < arg0->GetArrayCount(); ++a1) {
        NiNode* v2 = static_cast<NiNode*>(arg0->GetAt(a1));
        if (v2) {
            Actor* pActorNode = static_cast<Actor*>(TESObjectREFR::FindReferenceFor3D(v2));
            if ((pActorNode) && (pActorNode->IsActor()) && (pActorNode->pRagdollController) &&
                (pActorNode->pRagdollController->GetRagdollAnimEnable() &&
                 !pActorNode->pRagdollController->bUsingMappers && !static_cast<uint8_t>(v2->GetNotVisible()) &&
                 !pActorNode->IsInDialoguewithPlayer())) {
                bhkNiCollisionObject::Synchronize(v2, bhkNiCollisionObject::SYNC_FROMHAVOK);
            }
        }
    }
}

INTERIOR_DATA::INTERIOR_DATA()
  : iAmbient(0), iDirectional(0), iFog(0), fFogNear(0.0f), fFogFar(0.0f), iDirectionalXY(0), iDirectionalZ(0),
    fDirectionalFade(1.0f), fClipDist(0.0f), fFogPower(1.0f), iInteriorOffset(0) {}

EXTERIOR_DATA::EXTERIOR_DATA() : iCellX(0), iCellY(0), ucLandHideFlags(0) {}

LoadedCellData::LoadedCellData()
  : spCell3D(nullptr), AnimatedRefMap(37), EmittanceSourceRefMap(37), EmittanceLightRefMap(37), MultiboundRefMap(37) {}

LoadedCellData::~LoadedCellData() {}

TESObjectCELL::RENDER_DATA::RENDER_DATA()
  : iGeometryCount(1200), iTriangleCount(800000), iNewRenderPassCount(100), iActiveLights(15), iTotalTime(0xFFFFFFFFu),
    iAccumulateTime(8) {}

TESObjectCELL::RENDER_TEST_DATA::RENDER_TEST_DATA()
  : pFailureCallbackFunc(&TESObjectCELL::DefaultRenderTestCallback), pFailureCallbackData(nullptr) {}

TESObjectCELL::RENDER_FAILURE_DATA::RENDER_FAILURE_DATA() {}

TESObjectCELL::QUEUED_ATTACH::QUEUED_ATTACH([[maybe_unused]] int32_t aiVal)
  : spAttachChild(nullptr), spAttachNode(nullptr), bFirstAvail(false) {}

TESObjectCELL::QUEUED_ATTACH* TESObjectCELL::QUEUED_ATTACH::Copy(const TESObjectCELL::QUEUED_ATTACH* a2) {
    (&spAttachChild)->SetPointer(&a2->spAttachChild);
    (&spAttachNode)->SetPointer(&a2->spAttachNode);
    bFirstAvail = (bool)((uint8_t)a2->bFirstAvail);
    return this;
}

TESObjectCELL::QUEUED_ATTACH& TESObjectCELL::QUEUED_ATTACH::operator=(const TESObjectCELL::QUEUED_ATTACH& __that) {
    spAttachChild.operator=(__that.spAttachChild);
    spAttachNode.operator=(__that.spAttachNode);
    bFirstAvail = __that.bFirstAvail;
    return *this;
}

TESObjectCELL::TESObjectCELL()
  : cCellFlags(0), cCellGameFlags(0), cCellState(0), pCellData(nullptr), pCellLand(nullptr),
    fWaterHeight(3.4028235e+38f), bAutoWaterLoaded(false), pNavMeshes(nullptr), iCriticalQueuedRefCount(0),
    iQueuedRefCount(0), sNumRefsWithVisibleDistant(0), sNumLoadedRefsWithVisibleDistant(0), spLightMarkerNode(nullptr),
    spSoundMarkerNode(nullptr), spMultiBoundNode(nullptr), iTempDataOffset(0), pLoadedData(nullptr),
    fLodFadeInPercent(0.0f), bLODFadingIn(false), bFadedIn(false), bFadingToHighDetail(false),
    bFadingToLowDetail(false), bDisplayHighDetail(false), bCellDetached(false), bUpdateTerrain(false),
    spPortalGraph(nullptr), pLightingTemplate(nullptr), iLightingTemplateInheritanceFlags(159) {
    cFormType = 57;
    SetInitialized(true);
    WaterNoiseTexture.InitializeDataComponent();
}

TESObjectCELL::~TESObjectCELL() {
    ClearPortalGraph();
    if (!GetTemporary()) {
        Destroy();
        if (pCellLand) {
            delete pCellLand;
            pCellLand = nullptr;
        }
        if (pNavMeshes) {
            delete pNavMeshes;
            pNavMeshes = nullptr;
        }
        TESRegionList* pRegionList = ExtraData.GetRegionList();
        if (pRegionList) {
            pRegionList->Clear();
            delete pRegionList;
        }
    }
    if (!pDataHandler->GetClearingData()) {
        TESWorldSpace* pworld = GetWorldSpace();
        if (pworld) {
            pworld->ReleaseCell(this);
        }
    }
    FreeLoadedData();
    TESObjectCELL::ClearOptimizationDataForCell(this);
    TESObjectCELL::ClearData();
}

void TESObjectCELL::ClearData() {
    (&ExtraData)->RemoveNonPersistentCellData();
    GetInterior();
    ourFree(pCellData);
    pCellData = nullptr;
    SetLightingTemplate(nullptr);
    iLightingTemplateInheritanceFlags = (uint32_t)0;
}

// Reads a CELL record: its flags and cell data, water, names, lighting and the reference-style extra data chunks. The
// lighting template's form ID is kept in the pointer until InitItem resolves it. When the editor ID chunk is empty
// (malformed data), the editor ID buffer is initialised to "" (one zeroed byte, allocated only for an empty chunk).
bool TESObjectCELL::Load(TESFile* apFile) {
    // Compares only the low byte of the record type.
    if (static_cast<uint8_t>(apFile->GetTESForm()) != CELL_ID) {
        return false;
    }
    uint32_t uiOffset = 0;
    if (apFile->GetMaster()) {
        uiOffset = apFile->GetOffset();
    }
    TESForm::LoadForm(apFile);
    do {
        switch (apFile->GetTESChunk()) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer sized by the chunk, but the read is capped at 0x200 bytes, as the game
                // does.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pEditorID = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pEditorID[0] = '\0';
                apFile->GetChunkData(pEditorID, 0x200);
                SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case DATA_ID:
                if (apFile->GetChunkSize() == 4) {
                    uint32_t uiFlags = 0;
                    apFile->GetChunkData(uiFlags);
                    cCellFlags = static_cast<uint8_t>(uiFlags);
                }
                else {
                    apFile->GetChunkData(&cCellFlags, 1);
                }
                if ((cCellFlags & kCellFlag_Interior) == 0) {
                    cCellFlags |= kCellFlag_HasWater;
                }
                if ((cCellFlags & kCellFlag_Interior) != 0 && (cCellFlags & kCellFlag_BehavesLikeExterior) != 0) {
                    // FLT_MAX.
                    SetWaterHeight(3.4028234663852886e+38f);
                }
                if (!pDataHandler->GetSaveLoadGame()) {
                    cCellFlags &= ~kCellFlag_TempPublic;
                }
                CreateCellData();
                if (GetInterior() && uiOffset != 0) {
                    SetInteriorOffset(uiOffset);
                }
                break;
            case XCLC_ID: {
                EXTERIOR_DATA* pExterior = GetExteriorData();
                if (pExterior != nullptr) {
                    if (apFile->QEndian() && apFile->GetChunkSize() != sizeof(EXTERIOR_DATA)) {
                        pExterior->Endian();
                    }
                    apFile->GetChunkData(pExterior, sizeof(EXTERIOR_DATA));
                    if (apFile->QEndian()) {
                        pExterior->Endian();
                    }
                }
                break;
            }
            case XCLL_ID: {
                INTERIOR_DATA* pInterior = GetInteriorData();
                if (pInterior != nullptr) {
                    if (apFile->QEndian() && apFile->GetChunkSize() != sizeof(INTERIOR_DATA)) {
                        pInterior->Endian();
                    }
                    apFile->GetChunkData(pInterior, sizeof(INTERIOR_DATA));
                    if (apFile->QEndian()) {
                        pInterior->Endian();
                    }
                }
                break;
            }
            case XCLW_ID: apFile->GetChunkData(fWaterHeight); break;
            case XNAM_ID: {
                uint32_t uiSize = apFile->GetChunkSize();
                if (uiSize != 0) {
                    // 16-byte aligned.
                    char* pTextureName = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
                    apFile->GetChunkData(pTextureName, uiSize);
                    WaterNoiseTexture.SetTextureName(pTextureName);
                }
                break;
            }
            case LTMP_ID: {
                uint32_t uiTemplateID = 0;
                apFile->GetChunkData(uiTemplateID);
                pLightingTemplate = reinterpret_cast<BGSLightingTemplate*>(static_cast<uintptr_t>(uiTemplateID));
                break;
            }
            case LNAM_ID: apFile->GetChunkData(iLightingTemplateInheritanceFlags); break;
            case IMPF_ID:
            case IMPS_ID: {
                ImpactSwap* pImpactSwap = GetExtra()->GetImpactSwap();
                if (pImpactSwap == nullptr) {
                    void* pMemory = ourMalloc(sizeof(ImpactSwap));
                    pImpactSwap = pMemory != nullptr ? ::new (pMemory) ImpactSwap : nullptr;
                    GetExtra()->SetImpactSwap(pImpactSwap);
                }
                pImpactSwap->Load(apFile);
                break;
            }
            case AMRK_ID:
            case MMRK_ID:
            case RNAM_ID:
            case XACR_ID:
            case XACT_ID:
            case XAMC_ID:
            case XAMT_ID:
            case XAPD_ID:
            case XAPR_ID:
            case XATO_ID:
            case XCAS_ID:
            case XCCM_ID:
            case XCET_ID:
            case XCHG_ID:
            case XCIM_ID:
            case XCLM_ID:
            case XLCR_ID:
            case XCLR_ID:
            case XCMO_ID:
            case XCMT_ID:
            case XCNT_ID:
            case XCWT_ID:
            case XDCR_ID:
            case XEDL_ID:
            case XEMI_ID:
            case XESP_ID:
            case XEZN_ID:
            case XGLB_ID:
            case XHLP_ID:
            case XHLT_ID:
            case XHRS_ID:
            case XIBS_ID:
            case XLCM_ID:
            case XLKR_ID:
            case XLMB_ID:
            case XLOC_ID:
            case XLOD_ID:
            case XLTW_ID:
            case XMBO_ID:
            case XMBP_ID:
            case XMBR_ID:
            case XMRC_ID:
            case XMRK_ID:
            case XNDP_ID:
            case XNVP_ID:
            case XOCP_ID:
            case XORD_ID:
            case XOWN_ID:
            case XPCI_ID:
            case XPOD_ID:
            case XPPA_ID:
            case XPRD_ID:
            case XPRM_ID:
            case XPSL_ID:
            case XPSN_ID:
            case XPTL_ID:
            case XPWR_ID:
            case XRAD_ID:
            case XRDO_ID:
            case XRDS_ID:
            case XRGB_ID:
            case XRGD_ID:
            case XRMR_ID:
            case XRNK_ID:
            case XROO_ID:
            case XRTM_ID:
            case XSED_ID:
            case XSOL_ID:
            case XSRD_ID:
            case XSRF_ID:
            case XTEL_ID:
            case XTIM_ID:
            case XTRG_ID:
            case XTRI_ID:
            case XUSE_ID:
            case XWLT_ID: ExtraData.Load(apFile, this); break;
            default: break;
        }
    }
    while (apFile->NextChunk());
    SetInitialized(false);
    return true;
}

bool TESObjectCELL::Save(TESFile* apFile) {
    FORM apGroupData;
    if (GetDelete()) {
        return false;
    }
    bool bChanged = false;
    CellRefLockEnter();
    if (apFile->GetMaster() || GetAltered()) {
        bChanged = true;
    }
    else {
        BSSimpleList<TESObjectREFR*>* ecx0a = GetRefList();
        while (ecx0a && !ecx0a->IsEmpty()) {
            TESForm* v7 = ecx0a->m_item;
            ecx0a = ecx0a->m_pkNext;
            TESFile* File = v7->GetFile(-1);
            if (File == apFile || (!File || v7->GetAltered())) {
                bChanged = true;
                break;
            }
        }
    }
    CellRefLockLeave();
    if (!bChanged) {
        return false;
    }
    Save();
    apFile->AddTESForm(this);
    apGroupData.form = formEnumString[2].uiFormString;
    apGroupData.iFormID = 6;
    apGroupData.flags = GetFormID();
    apGroupData.length = 0;
    apGroupData.iVersionControl = 0;
    apFile->StartGroup(&apGroupData);
    if (!apFile->GetMaster() || GetFile(0) == apFile) {
        SaveReferences(apFile);
    }
    return true;
}

void TESObjectCELL::Save() {
    uint32_t iid;
    StartForm();
    TESFullName::Save();
    TESForm::AddChunk(DATA_ID, cCellFlags);
    if (GetInterior()) {
        INTERIOR_DATA* pdata = GetInteriorData();
        if (TESForm::QEndianSwapOnSave()) {
            pdata->Endian();
        }
        TESForm::__AddChunkData(XCLL_ID, pdata, 44u);
        if (TESForm::QEndianSwapOnSave()) {
            pdata->Endian();
        }
        ImpactSwap* pswap = GetExtra()->GetImpactSwap();
        if (pswap) {
            pswap->Save();
        }
    }
    else {
        EXTERIOR_DATA* pdata_2 = GetExteriorData();
        if (TESForm::QEndianSwapOnSave()) {
            pdata_2->Endian();
        }
        TESForm::__AddChunkData(XCLC_ID, pdata_2, 12u);
        if (TESForm::QEndianSwapOnSave()) {
            pdata_2->Endian();
        }
    }
    if (GetLightingTemplate()) {
        BGSLightingTemplate* LightingTemplate = GetLightingTemplate();
        iid = LightingTemplate->iFormID;
    }
    else {
        iid = 0;
    }
    TESForm::AddChunk(LTMP_ID, iid);
    TESForm::AddChunk(LNAM_ID, iLightingTemplateInheritanceFlags);
    TESForm::AddChunk(XCLW_ID, std::bit_cast<uint32_t>(fWaterHeight));
    TESForm::AddChunkArray(XNAM_ID, WaterNoiseTexture.GetTextureName(), WaterNoiseTexture.GetTextureNameLength() + 1);
    ExtraData.Save();
    CloseForm();
}

bool TESObjectCELL::SaveEdit(TESFile* apFile) {
    FORM apGroupData;
    Save();
    apFile->AddTESForm(this);
    apGroupData.form = formEnumString[2].uiFormString;
    apGroupData.iFormID = 6;
    apGroupData.flags = GetFormID();
    apGroupData.length = 0;
    apGroupData.iVersionControl = 0;
    apFile->StartGroup(&apGroupData);
    SaveReferences(apFile);
    return true;
}

bool TESObjectCELL::SavesBefore(TESForm* apForm) {
    bool bretval = false;
    switch (apForm->GetFormType()) {
        case CELL_ID: {
            TESObjectCELL* pothercell = dynamic_cast<TESObjectCELL*>(apForm);
            if (pothercell) {
                if (!GetInterior() || pothercell->GetInterior()) {
                    int32_t Interior = GetInterior();
                    if (Interior == pothercell->GetInterior()) {
                        bool bcheckblocks = true;
                        if (!GetInterior()) {
                            TESWorldSpace* pworld_2 = GetWorldSpace();
                            TESWorldSpace* potherworld = pothercell->GetWorldSpace();
                            if (pworld_2 == potherworld) {
                                int32_t PersistentCell = GetPersistentCell();
                                if (PersistentCell != pothercell->GetPersistentCell()) {
                                    bretval = GetPersistentCell();
                                    bcheckblocks = false;
                                }
                            }
                            else {
                                bretval = pworld_2->SavesBefore(potherworld);
                                bcheckblocks = false;
                            }
                        }
                        if (bcheckblocks) {
                            uint32_t GroupBlockKey = GetGroupBlockKey();
                            if (GroupBlockKey >= pothercell->GetGroupBlockKey()) {
                                uint32_t v6 = GetGroupBlockKey();
                                if (v6 == pothercell->GetGroupBlockKey()) {
                                    uint32_t GroupSubBlockKey = GetGroupSubBlockKey();
                                    if (GroupSubBlockKey >= pothercell->GetGroupSubBlockKey()) {
                                        uint32_t v8 = GetGroupSubBlockKey();
                                        if (v8 == pothercell->GetGroupSubBlockKey()) {
                                            uint32_t CastingType = iFormID;
                                            if (CastingType < pothercell->iFormID) {
                                                bretval = true;
                                            }
                                        }
                                    }
                                    else {
                                        bretval = true;
                                    }
                                }
                            }
                            else {
                                bretval = true;
                            }
                        }
                    }
                }
                else {
                    bretval = true;
                }
            }
            break;
        }
        case REFR_ID:
        case ACHR_ID:
        case ACRE_ID:
        case PMIS_ID:
        case PGRE_ID:
        case PBEA_ID:
        case PFLA_ID:
        case LAND_ID:
        case NAVM_ID:
        case PCBE_ID: {
            TESChildCell* pcellchild = dynamic_cast<TESChildCell*>(apForm);
            if (pcellchild->GetSaveParentCell() == this) {
                bretval = true;
            }
            else {
                TESObjectCELL* v2 = pcellchild->GetSaveParentCell();
                bretval = SavesBefore(v2);
            }
            break;
        }
        case WRLD_ID: {
            if (GetInterior()) {
                bretval = true;
            }
            else {
                TESWorldSpace* pworld = GetWorldSpace();
                bretval = pworld->SavesBefore(apForm);
            }
            break;
        }
        default: {
            bretval = TESForm::SavesBefore(apForm);
            break;
        }
    }
    return bretval;
}

bool TESObjectCELL::SavesBefore(FORM* apGroupFORM) {
    bool bretval = false;
    if ((apGroupFORM) && (apGroupFORM->form == formEnumString[2].uiFormString)) {
        uint32_t form = apGroupFORM->form;
        switch (apGroupFORM->iFormID) {
            case 0u: {
                bretval = TESForm::SavesBefore(apGroupFORM);
                break;
            }
            case 1u: {
                if (GetInterior()) {
                    bretval = true;
                }
                else {
                    TESForm* FormByNumericID = TESForm::GetFormByNumericID(apGroupFORM->flags);
                    TESWorldSpace* pworld = dynamic_cast<TESWorldSpace*>(FormByNumericID);
                    if (pworld) {
                        bretval = SavesBefore(pworld);
                    }
                }
                break;
            }
            case 2u: {
                if (GetInterior()) {
                    bretval = GetGroupBlockKey() < apGroupFORM->flags;
                }
                break;
            }
            case 3u: {
                if (GetInterior()) {
                    bretval = GetGroupSubBlockKey() < apGroupFORM->flags;
                }
                break;
            }
            case 4u: {
                bretval = GetInterior() || (GetPersistentCell() || GetGroupBlockKey() < apGroupFORM->flags);
                break;
            }
            case 5u: {
                bretval = GetInterior() || (GetPersistentCell() || GetGroupSubBlockKey() < apGroupFORM->flags);
                break;
            }
            case 6u:
            case 8u:
            case 9u: {
                TESForm* v3 = TESForm::GetFormByNumericID(apGroupFORM->flags);
                TESObjectCELL* pcell = dynamic_cast<TESObjectCELL*>(v3);
                if (pcell) {
                    bretval = SavesBefore(pcell);
                }
                break;
            }
            case 7u: {
                bretval = true;
                break;
            }
            default: {
                return bretval;
            }
        }
    }

    return bretval;
}

// An exterior cell's copy: the base form's duplicate, then copies of its land, its live navmeshes and every
// non-temporary reference it owns (persistent references only from an interior or persistent cell). An interior cell is
// not duplicated.
TESForm* TESObjectCELL::CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) {
    TESObjectCELL* pCell = nullptr;
    if (GetInterior()) {
        return pCell;
    }
    pCell = dynamic_cast<TESObjectCELL*>(TESForm::CreateDuplicateForm(false, apCopyMap));
    TESForm* pLand = pCellLand;
    if (pLand) {
        TESObjectLAND* pNewLand = dynamic_cast<TESObjectLAND*>(pLand->CreateDuplicateForm(false, apCopyMap));
        if (pNewLand) {
            pCell->SetLand(pNewLand);
            pNewLand->SetCell(pCell);
            TESForm* pNewLandForm = pNewLand;
            pNewLandForm->SetAltered(true);
        }
    }
    if (pNavMeshes) {
        void* pArrayMemory = ourMalloc(sizeof(NavMeshArray));
        NavMeshArray* pNewNavMeshes = pArrayMemory ? ::new (pArrayMemory) NavMeshArray : nullptr;
        pCell->SetNavMeshArray(pNewNavMeshes);
        alignas(NavMeshPtr) uint8_t aCopyStorage[sizeof(NavMeshPtr)];
        NavMeshPtr& spCopy = *reinterpret_cast<NavMeshPtr*>(aCopyStorage);
        ::new (&spCopy) NavMeshPtr;
        for (uint32_t i = 0; i < pNavMeshes->GetNavMeshCount(); ++i) {
            alignas(NavMeshPtr) uint8_t aNavMeshStorage[sizeof(NavMeshPtr)];
            NavMeshPtr& spNavMesh = *reinterpret_cast<NavMeshPtr*>(aNavMeshStorage);
            TESForm* pNavMesh = pNavMeshes->GetNavMeshByIndex(&spNavMesh, i)->operator->();
            spNavMesh.~NavMeshPtr();
            if (pNavMesh && !pNavMesh->GetDelete()) {
                NavMesh* pNewNavMesh = dynamic_cast<NavMesh*>(pNavMesh->CreateDuplicateForm(false, apCopyMap));
                if (pNewNavMesh) {
                    pNewNavMesh->pParentCell = pCell;
                    alignas(NavMeshPtr) uint8_t aMadeStorage[sizeof(NavMeshPtr)];
                    NavMeshPtr& spMade = *reinterpret_cast<NavMeshPtr*>(aMadeStorage);
                    spCopy = *NavMeshPtr::MakeNavMeshPtr(&spMade, pNewNavMesh);
                    spMade.~NavMeshPtr();
                    // SWEEP-INLINE 0x42fa20
                    pCell->pNavMeshes->AddNavMesh(spCopy);
                }
            }
        }
        spCopy.~NavMeshPtr();
    }
    CellRefLockEnter();
    alignas(BSSimpleList<TESObjectREFR*>) uint8_t aListStorage[sizeof(BSSimpleList<TESObjectREFR*>)];
    BSSimpleList<TESObjectREFR*>& kRefs = *reinterpret_cast<BSSimpleList<TESObjectREFR*>*>(aListStorage);
    ::new (&kRefs) BSSimpleList<TESObjectREFR*>;
    for (BSSimpleList<TESObjectREFR*>* pNode = GetRefList(); pNode && !pNode->IsEmpty();) {
        TESObjectREFR* pRef = pNode->GetItem();
        if (!pRef->GetTemporary() && (GetInterior() || GetPersistentCell() || !pRef->GetRefPersists())) {
            kRefs.AddHead(pRef);
        }
        pNode = pNode->GetNext();
    }
    CellRefLockLeave();
    for (BSSimpleList<TESObjectREFR*>* pNode = &kRefs; pNode && !pNode->IsEmpty();) {
        TESForm* pRefForm = pNode->GetItem();
        TESObjectREFR* pNewRef = dynamic_cast<TESObjectREFR*>(pRefForm->CreateDuplicateForm(false, apCopyMap));
        if (pNewRef) {
            pNewRef->SetRefPersists(pCell->GetPersistentCell());
            pCell->AddReference(pNewRef, false);
            TESForm* pNewRefForm = pNewRef;
            pNewRefForm->SetAltered(true);
        }
        pNode = pNode->GetNext();
    }
    kRefs.RemoveAll();
    kRefs.~BSSimpleList<TESObjectREFR*>();
    return pCell;
}

uint16_t TESObjectCELL::GetSaveSize(uint32_t aiFlags) {
    int16_t v8 = 0;
    if ((aiFlags & CHANGE_CELL_DETACHTIME) != 0) {
        v8 = 4;
    }
    uint16_t isize = TESForm::GetSaveSize(aiFlags) + v8;
    uint16_t v7 = isize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        isize += 6;
    }
    if ((aiFlags & 2) != 0) {
        ++isize;
    }
    if ((aiFlags & CHANGE_CELL_SEENDATA) != 0) {
        SeenData* SeenData_ = GetSeenData();
        isize += SeenData_->GetSaveSize(false);
    }
    if ((aiFlags & 4) != 0) {
        isize += BSstrlen(GetFullName()) + 1;
    }
    if ((aiFlags & 8) != 0) {
        isize += 4;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize - v7,
                pheader->iFormID, pform->GetFormEditorID(), pheader->iFlags, 0x3D51,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize - v7, 0x3D51,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp"
            );
        }
    }
    return isize;
}

// Writes the cell's changes in the old save format: the detach time, TESForm's data, the block header and size, the
// cell flags, the seen data, the name and the owner, then the block's size into its slot.
void TESObjectCELL::SaveGame(uint32_t aiFlags) {
    uint32_t iownerID;
    uint8_t clength;
    uint8_t ctempCellFlags;
    uint32_t iblockHeader;
    uint16_t iblockSize;
    int32_t idetachTime;
    if ((aiFlags & CHANGE_CELL_DETACHTIME) != 0) {
        idetachTime = GetDetachTime();
        SaveGameDataOLD(&idetachTime, 4);
    }
    TESForm::SaveGame(aiFlags);
    iblockSize = 0;
    uint16_t* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    if ((aiFlags & 2) != 0) {
        ctempCellFlags = cCellFlags & 96;
        ctempCellFlags |= cCellGameFlags;
        SaveGameDataOLD(&ctempCellFlags, 1);
    }
    if ((aiFlags & CHANGE_CELL_SEENDATA) != 0) {
        SeenData* SeenData_ = GetSeenData();
        SeenData_->SaveGame(false);
    }
    if ((aiFlags & 4) != 0) {
        const char* pfullname = GetFullName();
        clength = BSstrlen(pfullname);
        SaveGameDataOLD(&clength, 1);
        if (clength) {
            SaveGameDataOLD(pfullname, clength);
        }
    }
    if ((aiFlags & 8) != 0) {
        TESForm* powner = GetOwner();
        iownerID = 0;
        if (powner) {
            iownerID = powner->iFormID;
        }
        SaveNumericID(&iownerID, 4);
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v2 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                BufferPointer - itempStartPosition, pheader->iFormID, v2, iFlags, 15750,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s", BufferPointer - itempStartPosition, 15750,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block's size is the bytes written since the size slot, compared and subtracted as addresses.
        uintptr_t uiBufferPos = reinterpret_cast<uintptr_t>(pSaveLoadGameOLD->m_pBuffer);
        uintptr_t uiBlockSizePos = reinterpret_cast<uintptr_t>(pblockSize);
        if (uiBufferPos > uiBlockSizePos + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 15750
            );
        }
        *pblockSize = static_cast<uint16_t>(uiBufferPos - uiBlockSizePos);
    }
}

void TESObjectCELL::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t ctempCellFlags;
    TESForm::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(2)) {
        ctempCellFlags = cCellFlags & 96;
        ctempCellFlags |= cCellGameFlags;
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&ctempCellFlags), 1u, 0);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_CELL_SEENDATA)) {
        SeenData* pSeenData = GetSeenData();
        pSeenData->SaveGame(apSaveGameBuffer);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(4)) {
        const char* pfullname = GetFullName();
        apSaveGameBuffer->SaveString(pfullname, 0);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(8)) {
        TESForm* powner = GetOwner();
        apSaveGameBuffer->SaveFormID(powner, 0u);
    }
}

// Reads the cell's changes from an old-format save: the detach time (before or after TESForm's data, by save version),
// the block header and size, the cell flags, the seen data, the name and the owner; then warns when the read ran past
// or short of the block. LoadGameDataOLD reads raw save bytes into the locals, hence the double* casts.
void TESObjectCELL::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x5A && (aiFlags & 0x40000000) != 0) {
        int32_t iDetachTime;
        LoadGameDataOLD(reinterpret_cast<double*>(&iDetachTime), 4);
        GetExtra()->SetDetachTime(iDetachTime);
    }
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    uint16_t usBlockSize = 0;
    char* pBlockStart = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiBlockMarker;
        pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockMarker, 4);
        if (uiBlockMarker != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 15773,
                    pHeader->iFormID, pForm->GetFormEditorID(), pHeader->cVersion, pHeader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 15773,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, 2);
    }
    if ((aiFlags & 2) != 0) {
        uint8_t cFlags;
        LoadGameDataOLD(reinterpret_cast<double*>(&cFlags), 1);
        cCellFlags = (cCellFlags & 0x9F) | (cFlags & 0x60);
        cCellGameFlags = cFlags & 0x9F;
    }
    if ((aiFlags & 0x80000000) != 0) {
        SeenData* pSeenData = GetSeenData();
        if (!pSeenData) {
            if (GetInterior()) {
                void* pMemory = ourMalloc(sizeof(IntSeenData));
                IntSeenData* pIntSeenData = nullptr;
                if (pMemory) {
                    pIntSeenData = ::new (pMemory) IntSeenData(0, 0);
                }
                pSeenData = pIntSeenData;
            }
            else {
                void* pMemory = ourMalloc(sizeof(SeenData));
                SeenData* pNewSeenData = nullptr;
                if (pMemory) {
                    pNewSeenData = ::new (pMemory) SeenData;
                }
                pSeenData = pNewSeenData;
            }
            GetExtra()->SetSeenData(pSeenData);
        }
        pSeenData->LoadGame(static_cast<uint16_t>(0xFFFF));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 0x5A && (aiFlags & 0x40000000) != 0) {
        int32_t iDetachTime;
        LoadGameDataOLD(reinterpret_cast<double*>(&iDetachTime), 4);
        GetExtra()->SetDetachTime(iDetachTime);
    }
    if ((aiFlags & 4) != 0) {
        char acName[0x104];
        ourMemset(acName, 0, 0x104);
        uint8_t cLength;
        LoadGameDataOLD(reinterpret_cast<double*>(&cLength), 1);
        if (cLength) {
            LoadGameDataOLD(reinterpret_cast<double*>(acName), cLength);
        }
        SetFullName(acName);
    }
    if ((aiFlags & 8) != 0) {
        uint32_t uiOwnerID;
        LoadNumericID(&uiOwnerID, 4);
        // The loaded owner ID is passed straight on as the owner, as the game does.
        SetCellOwnership(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiOwnerID)));
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pPosition = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pPosition > pBlockStart + usBlockSize) {
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pPosition - (pBlockStart + usBlockSize)),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 15829,
                    pHeader->iFormID, pForm->GetFormEditorID(), pHeader->cVersion, pHeader->iFlags
                );
            }
            else if (pPosition < pBlockStart + usBlockSize) {
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>((pBlockStart + usBlockSize) - pPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 15829,
                    pHeader->iFormID, pForm->GetFormEditorID(), pHeader->cVersion, pHeader->iFlags
                );
            }
        }
        else if (pPosition > pBlockStart + usBlockSize) {
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pPosition - (pBlockStart + usBlockSize)),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 15829,
                pSaveLoadGameOLD->GetCurrentVersion()
            );
        }
        else if (pPosition < pBlockStart + usBlockSize) {
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>((pBlockStart + usBlockSize) - pPosition),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 15829,
                pSaveLoadGameOLD->GetCurrentVersion()
            );
        }
    }
}

void TESObjectCELL::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint32_t uiOwnerID;
    uint8_t ctempCellFlags;
    TESForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(2)) {
        ctempCellFlags = 0;
        apLoadGameBuffer->LoadData(&ctempCellFlags, 1u);
        cCellFlags = (ctempCellFlags & 96) | (cCellFlags & 159);
        cCellGameFlags = ctempCellFlags & 159;
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_CELL_SEENDATA)) {
        SeenData* pseen = GetSeenData();
        if (!pseen) {
            SeenData* pseen_3;
            if (GetInterior()) {
                SeenData* pseen_2;
                void* apMemory = ourMalloc(sizeof(IntSeenData));
                if (apMemory) {
                    pseen_2 = (SeenData*)::new (apMemory) IntSeenData(0, 0);
                }
                else {
                    pseen_2 = nullptr;
                }
                pseen_3 = pseen_2;
            }
            else {
                SeenData* pseen_4;
                SeenData* v8 = static_cast<SeenData*>(ourMalloc(sizeof(SeenData)));
                if (v8) {
                    pseen_4 = ::new (v8) SeenData;
                }
                else {
                    pseen_4 = nullptr;
                }
                pseen_3 = pseen_4;
            }
            pseen = pseen_3;
            GetExtra()->SetSeenData(pseen_3);
        }
        pseen->LoadGame(apLoadGameBuffer);
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(4)) {
        char apModel[268];
        apLoadGameBuffer->LoadString(apModel);
        SetFullName(apModel);
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(8)) {
        // The owner's form ID is kept in the ownership extra until InitLoadGame resolves it.
        uiOwnerID = 0;
        apLoadGameBuffer->LoadFormID(uiOwnerID);
        SetCellOwnership(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiOwnerID)));
    }
}

// The owner is stored as its form ID until the load resolves it.
void TESObjectCELL::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    TESForm::InitLoadGame(aiFlags, aiOldFlags);
    if (aiFlags & 8) {
        TESForm* pOwner = GetOwner();
        if (pOwner) {
            SetCellOwnership(TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pOwner))));
        }
    }
}

// The owner's form ID, which LoadGame keeps in the ownership extra, is resolved to the form.
void TESObjectCELL::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(8)) {
        TESForm* powner = GetOwner();
        if (powner) {
            powner = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(powner)));
            SetCellOwnership(powner);
        }
    }
}

// Reverts the cell's game changes: its game flags while the save game is not reverting, its seen data and its detach
// time when those changed.
void TESObjectCELL::Revert(uint32_t aiFlags) {
    TESForm::Revert(aiFlags);
    if (pSaveLoadGameOLD->IsUnk048Zero()) {
        cCellGameFlags = 0;
    }
    if (aiFlags & CHANGE_CELL_SEENDATA) {
        ClearSeenData();
    }
    if (aiFlags & CHANGE_CELL_DETACHTIME) {
        SetDetachTime(0, false);
    }
}

// The same for the new save system: the game flags always, then the seen data and detach time when the old change
// flags of the load buffer have them.
void TESObjectCELL::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::Revert(apLoadGameBuffer);
    cCellGameFlags = 0;
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_CELL_SEENDATA)) {
        ClearSeenData();
    }
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_CELL_DETACHTIME)) {
        SetDetachTime(0, false);
    }
}

// Seeks a master file to the cell's record (an interior's offset, an exterior's world space search) and checks that the
// file now sits on this cell's record.
bool TESObjectCELL::FindInFileFast(TESFile* apFile) {
    if (apFile == nullptr || !apFile->GetMaster()) {
        return false;
    }
    if (GetInterior()) {
        uint32_t iOffset = GetInteriorOffset();
        if (iOffset != 0) {
            apFile->SetOffset(iOffset);
        }
    }
    else {
        TESWorldSpace* pWorld = GetWorldSpace();
        if (pWorld != nullptr) {
            int32_t iY = GetDataY();
            int32_t iX = GetDataX();
            pWorld->FindCellInFile(apFile, iX, iY);
        }
    }
    if (apFile->GetTESForm() != CELL_ID) {
        return false;
    }
    uint32_t iFileID = apFile->GetID();
    return iFileID == GetFormID();
}

// Initialises the cell once, with warnings counted apart and reported in one message, and resolves its lighting
// template's form ID; then initialises its references and navmeshes and, during a save load, applies the save's cell
// data. The Warning arguments go through locals so they are evaluated right to left, as the game does.
void TESObjectCELL::InitItem() {
    if (!GetInitialized()) {
        uint32_t uiWarningCount = MessageHandler::QWarningCount();
        MessageHandler::IncDisableWarningCount(true);
        MessageHandler::SetWarningCount(0);
        ExtraData.InitItem(this);
        if (!GetInterior()) {
            TESFullName::SetFullName(nullptr);
        }
        SetInitialized(true);
        if (TESFullName::GetFullName() != nullptr && std::strlen(TESFullName::GetFullName()) > 33) {
            uint32_t iFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            const char* pFullName = TESFullName::GetFullName();
            Warning(
                "Cell name %s for cell %s (%08X) is too long. The maximum length allowed is 33 characters.", pFullName,
                pEditorID, iFormID
            );
        }
        MessageHandler::IncDisableWarningCount(false);
        uint32_t uiNewWarningCount = MessageHandler::QWarningCount();
        MessageHandler::SetWarningCount(uiWarningCount);
        if (uiNewWarningCount != 0) {
            TESWorldSpace* pWorld = GetWorldSpace();
            if (GetInterior()) {
                uint32_t iFormID = GetFormID();
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "Errors were encountered during InitItem for interior cell:\n\nCell '%s' (%08X)\n\n"
                    "Check Warnings for details.",
                    pEditorID, iFormID
                );
            }
            else if (pWorld != nullptr) {
                uint32_t iWorldFormID = pWorld->GetFormID();
                const char* pWorldEditorID = pWorld->GetFormEditorID();
                int32_t iY = GetDataY();
                int32_t iX = GetDataX();
                uint32_t iFormID = GetFormID();
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "Errors were encountered during InitItem for exterior cell:\n\nCell '%s' (%08X) at (%i, %i) in "
                    "world %s (%08X)\n\nCheck Warnings for details.",
                    pEditorID, iFormID, iX, iY, pWorldEditorID, iWorldFormID
                );
            }
            else {
                int32_t iY = GetDataY();
                int32_t iX = GetDataX();
                uint32_t iFormID = GetFormID();
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "Errors were encountered during InitItem for exterior cell:\n\nCell '%s' (%08X) at (%i, %i) in "
                    "UNKNOWN world\n\nCheck Warnings for details.",
                    pEditorID, iFormID, iX, iY
                );
            }
        }
        if (GetLightingTemplate() != nullptr) {
            // Until now the template pointer holds the file's form ID.
            uint32_t iTemplateID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetLightingTemplate()));
            TESForm::AddCompileIndex(iTemplateID, GetFile(-1));
            BGSLightingTemplate* pTemplate =
                dynamic_cast<BGSLightingTemplate*>(TESForm::GetFormByNumericID(iTemplateID));
            SetLightingTemplate(pTemplate);
            if (pTemplate == nullptr && iTemplateID != 0) {
                uint32_t iFormID = GetFormID();
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Unable to find lighting template (%08X) for cell '%s' (%08X).", iTemplateID, pEditorID,
                    iFormID
                );
            }
        }
    }

    bool bOldAllowChanges = pSaveLoadGame->SetThreadAllowChanges(false);
    // A reference that InitItem moves to another cell leaves the list; the walk then resumes after the last kept
    // node, or from the head.
    BSSimpleList<TESObjectREFR*>* pNode = GetRefList();
    BSSimpleList<TESObjectREFR*>* pPrevNode = nullptr;
    while (pNode != nullptr && !pNode->IsEmpty()) {
        TESObjectREFR* pRef = pNode->GetItem();
        if (!pRef->GetInitialized()) {
            pRef->InitItem();
            pSaveLoadGameOLD->LoadForm(pRef);
            pSaveLoadGame->LoadForm(pRef);
        }
        if (GetInterior() && pRef->GetObjectReference() == pNorthMarker) {
            float fNorthRotation = static_cast<float>(pRef->GetAngleOnReference().z * -1.0);
            GetExtra()->SetNorthRotation(fNorthRotation);
        }
        if (GetPersistentCell() || pRef->GetParentCell() == this) {
            pPrevNode = pNode;
            pNode = pNode->GetNext();
        }
        else if (pPrevNode != nullptr) {
            pNode = pPrevNode->GetNext();
        }
        else {
            pNode = GetRefList();
        }
    }
    if (GetNavMeshArray() != nullptr) {
        for (uint32_t i = 0; i < GetNavMeshArray()->GetNavMeshCount(); ++i) {
            // GetNavMeshByIndex constructs its result in place.
            alignas(NavMeshPtr) uint8_t aNavMeshStorage[sizeof(NavMeshPtr)];
            NavMeshPtr& spNavMesh = *reinterpret_cast<NavMeshPtr*>(aNavMeshStorage);
            GetNavMeshArray()->GetNavMeshByIndex(&spNavMesh, i);
            spNavMesh->InitItem();
            pSaveLoadGame->LoadForm(static_cast<NavMesh*>(spNavMesh));
            spNavMesh.~NavMeshPtr();
        }
    }
    pSaveLoadGame->SetThreadAllowChanges(bOldAllowChanges);

    if (pSaveLoadGameOLD->GetLoading()) {
        pSaveLoadGameOLD->LoadCell(this);
        if (!pSaveLoadGameOLD->GetLoading() && pSaveLoadGameOLD->GetInitArrayExists()) {
            bool bOldLoading = pSaveLoadGameOLD->GetLoading();
            pSaveLoadGameOLD->SetLoading(true);
            pSaveLoadGameOLD->InitForms(nullptr, nullptr, false);
            pSaveLoadGameOLD->LoadQueuedData(false);
            pSaveLoadGameOLD->SetLoading(bOldLoading);
        }
    }
}

// The cell's editor ID and form ID, with its grid position and worldspace when it is exterior. The locals keep the
// game's call order (each form ID before its editor ID). The invalid-worldspace format is passed X and Y but never
// prints them, as the game does.
void TESObjectCELL::GetFormDetailedString(BSStringT<char>& aString) {
    if (GetInterior()) {
        uint32_t uiFormID = GetFormID();
        const char* pEditorID = GetFormEditorID();
        aString.SPrintF("'%s' (%08X)", pEditorID, uiFormID);
        return;
    }
    TESForm* pWorldSpace = GetWorldSpace();
    if (pWorldSpace) {
        uint32_t uiWorldFormID = pWorldSpace->GetFormID();
        const char* pWorldEditorID = pWorldSpace->GetFormEditorID();
        int32_t iY = GetDataY();
        int32_t iX = GetDataX();
        uint32_t uiFormID = GetFormID();
        const char* pEditorID = GetFormEditorID();
        aString.SPrintF(
            "'%s' (%08X) (%i, %i) in WorldSpace '%s' (%08X)", pEditorID, uiFormID, iX, iY, pWorldEditorID, uiWorldFormID
        );
        return;
    }
    int32_t iY = GetDataY();
    int32_t iX = GetDataX();
    uint32_t uiFormID = GetFormID();
    const char* pEditorID = GetFormEditorID();
    aString.SPrintF("'%s' (%08X) Invalid WorldSpace", pEditorID, uiFormID, iX, iY);
}

// An exterior cell that becomes altered alters its world space too.
void TESObjectCELL::SetAltered(bool abAltered) {
    TESForm::SetAltered(abAltered);
    if (!GetTemporary() && GetAltered() && !GetInterior()) {
        TESWorldSpace* pWorld = GetWorldSpace();
        if (pWorld != nullptr) {
            pWorld->SetAltered(true);
        }
    }
}

void TESObjectCELL::Copy(TESForm* apCopy) {
    TESObjectCELL* pCopy = dynamic_cast<TESObjectCELL*>(apCopy);
    if (pCopy) {
        CopyAllComponents(pCopy);
        GetExtra()->CopyList(pCopy->GetExtra());
        GetInterior();
        ourFree(pCellData);
        pCellData = nullptr;
        cCellFlags = pCopy->cCellFlags;
        fWaterHeight = pCopy->fWaterHeight;
        CreateCellData();
        if (GetInterior()) {
            INTERIOR_DATA* pdata = GetInteriorData();
            INTERIOR_DATA* pcopydata = pCopy->GetInteriorData();
            if ((pdata) && (pcopydata)) {
                ourMemcpy(pdata, pcopydata, sizeof(INTERIOR_DATA));
            }
        }
        else {
            EXTERIOR_DATA* pdata_2 = GetExteriorData();
            EXTERIOR_DATA* pcopydata_2 = pCopy->GetExteriorData();
            if ((pdata_2) && (pcopydata_2)) {
                ourMemcpy(pdata_2, pcopydata_2, sizeof(EXTERIOR_DATA));
            }
        }
        BGSLightingTemplate* LightingTemplate = pCopy->GetLightingTemplate();
        SetLightingTemplate(LightingTemplate);
        iLightingTemplateInheritanceFlags = pCopy->iLightingTemplateInheritanceFlags;
        WaterNoiseTexture.CopyComponent(&pCopy->WaterNoiseTexture);
    }
}

bool TESObjectCELL::Compare(TESForm* apCompare) {
    TESObjectCELL* pComp = dynamic_cast<TESObjectCELL*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    int32_t v3 = cCellFlags;
    if (v3 != pComp->cCellFlags) {
        return true;
    }
    if (iLightingTemplateInheritanceFlags != pComp->iLightingTemplateInheritanceFlags) {
        return true;
    }
    BGSLightingTemplate* LightingTemplate = GetLightingTemplate();
    if (LightingTemplate != pComp->GetLightingTemplate()) {
        return true;
    }
    if (pComp->fWaterHeight != fWaterHeight) {
        return true;
    }
    if (GetInterior()) {
        INTERIOR_DATA* pdata = GetInteriorData();
        INTERIOR_DATA* pcompdata = pComp->GetInteriorData();
        if (pdata && (pcompdata && std::memcmp(pdata, pcompdata, sizeof(INTERIOR_DATA)))) {
            return true;
        }
    }
    else {
        EXTERIOR_DATA* pdata_2 = GetExteriorData();
        EXTERIOR_DATA* pcompdata_2 = pComp->GetExteriorData();
        if (pdata_2 && (pcompdata_2 && std::memcmp(pdata_2, pcompdata_2, sizeof(EXTERIOR_DATA)))) {
            return true;
        }
    }
    // The texture's CompareComponent is called virtually; a call on the member itself would be devirtualised, so it
    // goes through a base pointer.
    BaseFormComponent* pWaterNoiseTexture = &WaterNoiseTexture;
    return GetExtra()->CompareList(pComp->GetExtra()) ||
        pWaterNoiseTexture->CompareComponent(&pComp->WaterNoiseTexture);
}

bool TESObjectCELL::BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) {
    bool v7 = false;
    if ((apGroupFORM) && (apGroupFORM->form == formEnumString[2].uiFormString)) {
        TESWorldSpace* WorldSpace = GetWorldSpace();
        switch (apGroupFORM->iFormID) {
            case 0u: {
                if (abAllowParentGroups) {
                    if (GetInterior()) {
                        v7 = TESForm::BelongsInGroup(apGroupFORM, abAllowParentGroups, abCurrentOnly);
                    }
                    else {
                        v7 = WorldSpace->BelongsInGroup(apGroupFORM, abAllowParentGroups, abCurrentOnly);
                    }
                }
                break;
            }
            case 1u: {
                if (!GetInterior() && WorldSpace->FormIDMatchesWithoutIndex(apGroupFORM->flags) &&
                    (abAllowParentGroups || GetPersistentCell())) {
                    v7 = true;
                }
                break;
            }
            case 2u:
            case 4u: {
                if (!GetPersistentCell() && abAllowParentGroups) {
                    v7 = apGroupFORM->flags == GetGroupBlockKey();
                }
                break;
            }
            case 3u:
            case 5u: {
                if (!GetPersistentCell()) {
                    v7 = apGroupFORM->flags == GetGroupSubBlockKey();
                }
                break;
            }
            default: {
                return v7;
            }
        }
    }

    return v7;
}

void TESObjectCELL::CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup) {
    if (apOutGroupFORM) {
        apOutGroupFORM->form = 0;
        if (GetInterior()) {
            if (apParentGroup) {
                uint32_t iFormID = apParentGroup->GroupData.iFormID;
                if (iFormID) {
                    if (iFormID == 2 && apParentGroup->GroupData.flags == GetGroupBlockKey()) {
                        apOutGroupFORM->form = formEnumString[2].uiFormString;
                        apOutGroupFORM->iFormID = 3;
                        apOutGroupFORM->flags = GetGroupSubBlockKey();
                        apOutGroupFORM->length = 0;
                        apOutGroupFORM->iVersionControl = 0;
                    }
                }
                else {
                    if (apParentGroup->GroupData.flags == formEnumString[57].uiFormString) {
                        apOutGroupFORM->form = formEnumString[2].uiFormString;
                        apOutGroupFORM->iFormID = 2;
                        apOutGroupFORM->flags = GetGroupBlockKey();
                        apOutGroupFORM->length = 0;
                        apOutGroupFORM->iVersionControl = 0;
                    }
                }
            }
            else {
                apOutGroupFORM->form = formEnumString[2].uiFormString;
                apOutGroupFORM->iFormID = 0;
                apOutGroupFORM->flags = formEnumString[57].uiFormString;
                apOutGroupFORM->length = 0;
                apOutGroupFORM->iVersionControl = 0;
            }
        }
        else {
            if (apParentGroup) {
                uint32_t v6 = apParentGroup->GroupData.iFormID;
                if (v6) {
                    if (v6 == 1) {
                        TESWorldSpace* WorldSpace = GetWorldSpace();
                        if (apParentGroup->GroupData.flags == WorldSpace->GetFormID() && !GetPersistentCell()) {
                            apOutGroupFORM->form = formEnumString[2].uiFormString;
                            apOutGroupFORM->iFormID = 4;
                            apOutGroupFORM->flags = GetGroupBlockKey();
                            apOutGroupFORM->length = 0;
                            apOutGroupFORM->iVersionControl = 0;
                        }
                    }
                    else {
                        if (v6 == 4 && apParentGroup->GroupData.flags == GetGroupBlockKey() && !GetPersistentCell()) {
                            apOutGroupFORM->form = formEnumString[2].uiFormString;
                            apOutGroupFORM->iFormID = 5;
                            apOutGroupFORM->flags = GetGroupSubBlockKey();
                            apOutGroupFORM->length = 0;
                            apOutGroupFORM->iVersionControl = 0;
                        }
                    }
                }
                else {
                    if (apParentGroup->GroupData.flags == formEnumString[65].uiFormString) {
                        apOutGroupFORM->form = formEnumString[2].uiFormString;
                        apOutGroupFORM->iFormID = 1;
                        TESWorldSpace* v3 = GetWorldSpace();
                        apOutGroupFORM->flags = v3->GetFormID();
                        apOutGroupFORM->length = 0;
                        apOutGroupFORM->iVersionControl = 0;
                    }
                }
            }
        }
    }
}

bool TESObjectCELL::IsFormTypeChild(uint8_t aeType) {
    return TESObjectCELL::IsFormTypeCellChild(aeType);
}

// GetExtraEditorID is called a second time for the result it returns, as the game does.
const char* TESObjectCELL::GetFormEditorID() {
    if (ExtraData.GetExtraEditorID() != nullptr) {
        return ExtraData.GetExtraEditorID();
    }
    if (GetInterior()) {
        return GetCellName();
    }
    return sDefaultCellName.String();
}

bool TESObjectCELL::SetFormEditorID(const char* apEditorID) {
    if (BSstrlen(apEditorID) != 0) {
        ExtraData.SetEditorID(apEditorID);
    }
    return true;
}

bool TESObjectCELL::IsActorCellOwner(Actor* apActor) {
    bool bflag = false;
    TESForm* powner = GetOwner();
    if (powner) {
        if (!apActor->IsNpc()) {
            return false;
        }
        TESNPC* pnpc = dynamic_cast<TESNPC*>(powner);
        if (pnpc) {
            return pnpc == apActor->GetObjectReference();
        }

        TESFaction* pfaction = dynamic_cast<TESFaction*>(powner);
        if (pfaction) {
            int32_t irank = GetOwnershipRank();
            TESNPC* pactnpc = dynamic_cast<TESNPC*>(apActor->GetObjectReference());
            return pactnpc->GetFactionRank(pfaction, apActor == pPlayer) >= irank;
        }
    }
    return bflag;
}

void TESObjectCELL::ClearLocationMarkers() {
    const NiFixedString* name;
    const char* apString1;
    NiAVObject* v3;
    uint32_t a1;
    NiNode* markerNode = static_cast<NiNode*>(GetMarkerNode());
    if (markerNode) {
        for (a1 = 0; a1 < ((uint32_t)((int32_t)markerNode->GetArrayCount())); ++a1) {
            v3 = markerNode->GetAt(a1);
            if (v3) {
                name = &v3->GetName();
                apString1 = name->GetHandleString();
                if ((apString1) && (!BSstrcmp(apString1, "CellLocationMarker"))) {
                    markerNode->DetachChildAt(a1);
                }
            }
        }
    }
}

// The release build drops the debug marker, so this does nothing.
void TESObjectCELL::AddLocationMarker(const NiPoint3&, const char*, const NiColorA*) {}

void TESObjectCELL::AdjustCoordForNorthRotation(const NiPoint3& aStartCoord, NiPoint3& aOutCoord, bool abClockwise) {
    float v4;
    NiMatrix3 rotationMatrix;
    if (abClockwise) {
        v4 = -1.0f;
    }
    else {
        v4 = 1.0;
    }
    float angle = GetNorthRotation() * v4;
    if (angle == 0.0) {
        aOutCoord = aStartCoord;
    }
    else {
        NiPoint3 result;
        rotationMatrix = NiMatrix3_IDENTITY;
        rotationMatrix.MakeZRotation(angle);
        aOutCoord = (result = aStartCoord * rotationMatrix);
    }
}

void TESObjectCELL::AddMapMarkersToList(BSSimpleList<TESObjectREFR*>* apList) {
    if (apList) {
        CellRefLockEnter();
        for (TESObjectREFR* a2 : *GetRefList()) {
            if ((a2) && (a2->GetObjectReference() == pMapMarker) && (a2->GetMapMarkerData())) {
                apList->AddHead(a2);
            }
        }
        CellRefLockLeave();
    }
}

void TESObjectCELL::UpdateManagedNodeTransforms() {
    NiTMapItem<TESObjectREFR*, NiNode*>* a2;
    NiUpdateData apMemory(0.0f, false, false);
    NiNode* DynamicNode = static_cast<NiNode*>(GetDynamicNode());
    if (DynamicNode) {
        DynamicNode->UpdateTransformAndBounds(apMemory);
    }
    if (pLoadedData) {
        a2 = pLoadedData->AnimatedRefMap.GetFirstPos();
        while (a2) {
            TESObjectREFR* a3 = nullptr;
            NiNode* a4 = nullptr;
            pLoadedData->AnimatedRefMap.GetNext(a2, a3, a4);
            if (a4) {
                NiNode* Parent = a4->m_pkParent;
                if (Parent != DynamicNode) {
                    a4->UpdateTransformAndBounds(apMemory);
                }
            }
        }
    }
}

void TESObjectCELL::ShowPrimitives(bool abEnable) {
    for (BSSimpleList<TESObjectREFR*>* plist = &listReferences; plist && plist->m_item; plist = plist->m_pkNext) {
        TESObjectREFR** v2 = &plist->m_item;
        (*v2)->ShowPrimitive(abEnable);
    }
}

int32_t TESObjectCELL::SetCellOwnership(TESForm* apOwner) {
    int32_t result;
    ExtraDataList* Extra = GetExtra();
    Extra->SetOwnership(apOwner);
    AddChange(8);
    return result;
}

bool TESObjectCELL::PickWaterDistance(NiPick* apPick, const NiPoint3& aOrigin, const NiPoint3& aDir, float afDistance) {
    bool bhitsomething = false;
    if (apPick) {
        for (BSSimpleList<TESObjectREFR*>* plist = GetWaterRefList(); plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            TESObjectREFR* pref = plist->m_item;
            if (pref->Get3D()) {
                NiNode* v7 = pref->Get3D();
                if (PickDistanceRecursive(apPick, aOrigin, aDir, afDistance, v7)) {
                    NiPoint3 waterhit = *&(&apPick->m_pickResults)->GetAt(0)->m_intersect;
                    if (static_cast<double>(afDistance) > (waterhit - aOrigin).Length()) {
                        return true;
                    }
                }
            }
        }
    }
    return bhitsomething;
}

void TESObjectCELL::FreeLoadedData() {
    FNVD_INLINED_LOCAL(NiPointer<BSMultiBoundNode>, spnode, std::nullopt, std::nullopt);
    NiTMapItem<TESObjectREFR*, NiPointer<BSMultiBoundNode>>* iter;
    if (pLoadedData) {
        pLoadedData->ScriptedRefs.RemoveAll();
        pLoadedData->ActivatingRefs.RemoveAll();
        iter = pLoadedData->MultiboundRefMap.GetFirstPos();
        while (iter) {
            ::new (&spnode) NiPointer<ActorPathingMessageQueue>(static_cast<ActorPathingMessageQueue*>(nullptr));
            TESObjectREFR* pref = nullptr;
            pLoadedData->MultiboundRefMap.GetNext(iter, pref, spnode);
            if ((pref) && (spnode.m_pObject) && (spnode.m_pObject->QMultiBound()->m_uiRefCount == 2)) {
                pref->FreeMultiBound();
            }

            spnode.~NiPointer<BSMultiBoundNode>();
        }
        if (pLoadedData->spCell3D.m_pObject) {
            ClearPortalGraph();
            NiNode* pparent = pLoadedData->spCell3D.m_pObject->m_pkParent;
            if (pparent) {
                pparent->DetachChild(pLoadedData->spCell3D.m_pObject);
            }
        }
        if (pLoadedData) {
            delete pLoadedData;
        }
        pLoadedData = nullptr;
    }
}

void TESObjectCELL::SkipVisibleDistantFade() {
    BSFadeNode* pFadeNode;
    NiNode* pNode;
    PlayerCharacter* pRef;
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* ecx0 = GetRefList();
    while (ecx0) {
        pRef = static_cast<PlayerCharacter*>(ecx0->m_item);
        ecx0 = ecx0->m_pkNext;
        if ((pRef) && (pRef != pPlayer)) {
            pNode = pRef->TESObjectREFR::Get3D();
            if (pNode) {
                pFadeNode = pNode->IsFadeNode();
                if ((pFadeNode) &&
                    ((((int32_t)pFadeNode->GetMultType()) == 6) || (pFadeNode->GetBoundRadius() > 599.9000244140625))) {
                    pFadeNode->TurnFadeNodeOn();
                    BSShaderUtil::RecursiveSetPropertyFadeAlpha(pFadeNode, 1.0);
                }
            }
        }
    }
    fLodFadeInPercent = TESObjectCELL::fLodFadeTime;
    bDisplayHighDetail = true;
    bFadingToHighDetail = false;
    bFadingToLowDetail = false;
    bUpdateTerrain = true;
    CellRefLockLeave();
}

void TESObjectCELL::ForceLODFadeInLoadedCells() {
    for (uint32_t i = 0; i < TES::pInstance->GetExteriorBufferSize(); ++i) {
        TESObjectCELL* pCell = TES::pInstance->GetExteriorBufferCellByIndex(i);
        if (pCell) {
            pCell->SkipVisibleDistantFade();
        }
    }
}

void TESObjectCELL::SetDetachTime() {
    SetDetachTime(appCalendar.GetHoursPassed(), false);
}

// Walks the multibound node's children as NiNodes; the switch on aeAsProxyForNode has only CCN_DYNAMIC.
void TESObjectCELL::CullChildrenOfMultibounds(bool abCull, CELL_CULL_NODE aeAsProxyForNode) {
    NiNode* pMultiBoundNode = static_cast<NiNode*>(GetChildNode(CN_MULTIBOUND));
    if (!pMultiBoundNode) {
        return;
    }
    for (uint32_t i = 0; i < pMultiBoundNode->GetArrayCount(); ++i) {
        NiNode* pBound = static_cast<NiNode*>(pMultiBoundNode->GetAt(i));
        if (!pBound) {
            continue;
        }
        NiNode* pBoundNode = pBound;
        for (uint32_t j = 0; j < pBoundNode->GetArrayCount(); ++j) {
            NiAVObject* pChild = pBoundNode->GetAt(j);
            if (!pChild) {
                continue;
            }
            TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pChild);
            if (!pRef) {
                continue;
            }
            switch (aeAsProxyForNode) {
                case CCN_DYNAMIC: {
                    TESBoundObject* pBase = pRef->GetObjectReference();
                    if (pBase->GetFormType() != LIGH_ID && pBase != pPlaneMarker) {
                        pRef->Get3D()->SetAppCulled(abCull);
                    }
                    break;
                }
                default: break;
            }
        }
    }
}

void TESObjectCELL::AddReferences(TESWorldSpace* apWorld) {
    if ((apWorld) && (GetPersistentCell())) {
        CellRefLockEnter();
        for (TESObjectREFR* v12 : *GetRefList()) {
            if (v12) {
                const NiPoint3* v2 = &v12->GetLocationOnReference();
                int32_t aiX = FloatToInt(v2->x) >> 12;
                const NiPoint3* v3 = &v12->GetLocationOnReference();
                int32_t aiY = FloatToInt(v3->y) >> 12;
                TESObjectCELL* CellFromCellCoord = apWorld->GetCellFromCellCoord(aiX, aiY);
                if (CellFromCellCoord) {
                    CellFromCellCoord->AddReference(v12, false);
                }
                else {
                    int32_t someFloat = v12->GetFormID();
                    const char* v6 = v12->GetFormEditorID();
                    int32_t CastingType = GetFormID();
                    const char* v4 = apWorld->GetFormEditorID();
                    Warning(
                        "CELLS: Could not find cell (%i, %i) in world '%s' (%08X) to add reference '%s' (%08X) "
                        "to.",
                        aiX, aiY, v4, CastingType, v6, someFloat
                    );
                }
            }
        }
        CellRefLockLeave();
    }
}

ExtraDataList* TESObjectCELL::GetExtra() {
    return &ExtraData;
}

TESObjectCELL::CELL_STATE TESObjectCELL::GetState() const {
    return static_cast<CELL_STATE>(cCellState);
}

void TESObjectCELL::SetState(CELL_STATE aeState) {
    cCellState = static_cast<uint8_t>(aeState);
}

bool TESObjectCELL::GetUnloading() {
    return (bool)((uint8_t)(((int32_t)GetState()) == 1));
}

bool TESObjectCELL::GetCantWaitHere() {
    TESWorldSpace* worldSpace;
    if (GetInterior()) {
        if ((((int32_t)TESForm::GetFormFlags()) & 524288) != 0) {
            return true;
        }
    }
    else {
        worldSpace = GetWorldSpace();
        if (worldSpace) {
            return (bool)((uint8_t)worldSpace->GetCantWaitHere());
        }
    }
    return false;
}

void TESObjectCELL::SetLowestProcessToMiddleLow(bool abFlag) {
    ExtraProcessMiddleLow* pExtra = static_cast<ExtraProcessMiddleLow*>(ExtraData.GetExtraData(9));
    if (!pExtra && abFlag) {
        ExtraProcessMiddleLow* pExtra_2;
        ExtraProcessMiddleLow* apMemory = static_cast<ExtraProcessMiddleLow*>(ourMalloc(sizeof(ExtraProcessMiddleLow)));
        if (apMemory) {
            pExtra_2 = ::new (apMemory) ExtraProcessMiddleLow;
        }
        else {
            pExtra_2 = nullptr;
        }
        pExtra = pExtra_2;
        ExtraData.AddExtra(pExtra_2);
        CellRefLockEnter();
        BSSimpleList<TESObjectREFR*>* apList = GetRefList();
        while (apList && apList->m_item) {
            MobileObject* pRef = dynamic_cast<MobileObject*>(apList->m_item);
            apList = apList->m_pkNext;
            if ((pRef) && (!pRef->GetDisabled()) && (pRef->pCurrentProcess) &&
                (pRef->GetCurrentProcessType() == PROCESS_TYPE_LOW)) {
                ReferenceProcessLists.AddActorToTempChangeList(pRef);
            }
        }
        CellRefLockLeave();
    }
    if (pExtra) {
        int32_t v2;
        if (abFlag) {
            v2 = pExtra->iRefCount + 1;
        }
        else {
            v2 = pExtra->iRefCount - 1;
        }
        pExtra->iRefCount = v2;
        if (!pExtra->iRefCount) {
            ExtraData.RemoveExtra(pExtra, true);
        }
    }
}

bool TESObjectCELL::IsLoaded() const {
    return GetState() == CS_LOADED;
}

bool TESObjectCELL::IsLoading() const {
    return GetState() == CS_LOADING;
}

bool TESObjectCELL::GetAttached() const {
    return GetState() == CS_ATTACHED;
}

bool TESObjectCELL::IsAttaching() const {
    return GetState() == CS_ATTACHING;
}

int32_t TESObjectCELL::GetCriticalQueuedRefCount() const {
    return iCriticalQueuedRefCount;
}

void TESObjectCELL::IncQueuedRefCount() {
    NiAtomicIncrement(iQueuedRefCount);
}

void TESObjectCELL::IncCriticalQueuedRefCount() {
    NiAtomicIncrement(iCriticalQueuedRefCount);
}

// The counter never drops below zero.
void TESObjectCELL::DecQueuedRefCount() {
    NiAtomicDecrement(iQueuedRefCount);
    if (iQueuedRefCount < 0) {
        iQueuedRefCount = 0;
    }
}

void TESObjectCELL::DecCriticalQueuedRefCount() {
    NiAtomicDecrement(iCriticalQueuedRefCount);
    if (iCriticalQueuedRefCount < 0) {
        iCriticalQueuedRefCount = 0;
    }
}

bool TESObjectCELL::GetLandHeight(const NiPoint3& arPos, float& arHeight) {
    uint8_t v4 = 0;
    TESObjectLAND* land = GetLand();
    if (land) {
        return (bool)((uint8_t)land->GetLandHeight(arPos, arHeight));
    }
    arHeight = (float)0.0;
    return (bool)v4;
}

// Returns a NiNode; NiNode::GetAt yields a NiAVObject, so the pointer is cast.
NiNode* TESObjectCELL::GetLandNode(uint32_t aiQuad) {
    NiNode* childNode = GetChildNode(TESObjectCELL::CN_LAND);
    if (childNode) {
        return static_cast<NiNode*>(childNode->GetAt(aiQuad));
    }

    return nullptr;
}

void TESObjectCELL::UpdateRefSounds() {
    FNVD_INLINED_LOCAL(BSSoundHandle, SoundHandle, std::nullopt, std::nullopt);
    CellRefLockEnter();
    if (GetAttached()) {
        for (TESObjectREFR* pobj : GetRefList()->ItemsUntilNull()) {
            if ((pobj) &&
                (!pobj->GetDisabled() && !pobj->GetDestroyed() && pobj->HasRandomSound() &&
                 (!pobj->IsCreature() || !pobj->IsDead(false)))) {
                TESSound* pSound = pobj->pRandomSound;
                if (pSound) {
                    float v5 = static_cast<double>(pSound->GetRandomPercentChance()) / 200.0;
                    if ((RandomBool(v5)) && (*pSound->GetFilename())) {
                        uint32_t iMax = 100 * pSound->GetMaxDistance();
                        float fdist = pPlayer->GetDistanceFromReference(pobj, false, false);
                        if (iMax > static_cast<uint32_t>(static_cast<int64_t>(fdist))) {
                            uint32_t a3 = pSound->iFormID;
                            BSAudio* v1 = &BSAudio::QInstance();
                            v1->GetSoundHandleByNumericID(
                                &SoundHandle, a3,
                                BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                    BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER
                            );
                            if (pobj->Get3D()) {
                                NiNode* v2 = pobj->Get3D();
                                SoundHandle.SetObjectToFollow(v2);
                            }
                            const NiPoint3* v3 = &pobj->GetLocationOnReference();
                            SoundHandle.SetPosition(v3->x, v3->y, v3->z);
                            SoundHandle.SetPriority(192u);
                            SoundHandle.Play(false);
                        }
                    }
                }
            }
        }
    }
    CellRefLockLeave();
}

// Shows or hides the Havok debug display of the cell's world and returns the new state; false when the cell has no
// world.
bool TESObjectCELL::ToggleCollisionGeometry() {
    bool bDisplay = false;
    bhkWorld* pWorld = GetbhkWorld();
    if (pWorld) {
        bDisplay = !pWorld->QDebugDisplay();
        SetCollisionGeometryDisplay(bDisplay);
    }
    return bDisplay;
}

void TESObjectCELL::UpdateSounds() {
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* ecx0 = GetRefList();
    while (ecx0) {
        TESObjectREFR* th_s = ecx0->m_item;
        ecx0 = ecx0->m_pkNext;
        if ((th_s) && (th_s->CanHaveSound())) {
            bool ShouldPlaySound = th_s->ShouldPlaySound();
            th_s->AttachSound(ShouldPlaySound);
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::QueueReferences(bool abHighPriorityOnly) {
    TESObjectREFR* pRefr;
    BSSimpleList<TESObjectREFR*>* ecx0;
    BSSimpleList<TESObjectREFR*>* ecx0a;
    CellRefLockEnter();
    int32_t eCellPriority = (int32_t)TES::pInstance->GetCellPriority(this, nullptr);
    for (ecx0 = GetRefList(); ecx0; ecx0 = ecx0->m_pkNext) {
        pRefr = ecx0->m_item;
        if ((pRefr) &&
            ((static_cast<const void*>(pRefr->TESObjectREFR::GetObjectReference()) ==
              static_cast<const void*>(pRoomMarker)) ||
             (static_cast<const void*>(pRefr->TESObjectREFR::GetObjectReference()) ==
              static_cast<const void*>(pBoundMarker)))) {
            if (pRefr->Get3D() && (!pRefr->HasTemp3D())) {
                if (pRefr->GetDisabled() || pRefr->GetDelete()) {
                    pRefr->Set3D(nullptr, true);
                }
            }
            else {
                if (((!pRefr->GetDisabled()) && (!pRefr->GetDelete())) &&
                    ((!abHighPriorityOnly) || pRefr->Is3DHighPriority())) {
                    pModelLoader->QueueReference(pRefr, static_cast<IO_TASK_PRIORITY>(eCellPriority), true);
                }
            }
        }
    }
    for (ecx0a = GetRefList(); ecx0a; ecx0a = ecx0a->m_pkNext) {
        pRefr = ecx0a->m_item;
        if ((pRefr) &&
            ((static_cast<const void*>(pRefr->TESObjectREFR::GetObjectReference()) !=
              static_cast<const void*>(pRoomMarker)) &&
             (static_cast<const void*>(pRefr->TESObjectREFR::GetObjectReference()) !=
              static_cast<const void*>(pBoundMarker)))) {
            if (pRefr->Get3D() && (!pRefr->HasTemp3D())) {
                if (pRefr->GetDisabled() || pRefr->GetDelete()) {
                    pRefr->Set3D(nullptr, false);
                }
            }
            else {
                if (((!pRefr->GetDisabled()) && (!pRefr->GetDelete())) &&
                    ((!abHighPriorityOnly) || pRefr->Is3DHighPriority())) {
                    pModelLoader->QueueReference(pRefr, static_cast<IO_TASK_PRIORITY>(eCellPriority), true);
                }
            }
        }
    }
    CellRefLockLeave();
}

// Walks the cell's references and turns each fade node back on with full alpha.
void TESObjectCELL::UpdateFadeNodes() {
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* pNode = GetRefList();
    while (pNode != nullptr) {
        TESObjectREFR* pRef = pNode->GetItem();
        if (pRef != nullptr) {
            NiNode* pRef3D = pRef->TESObjectREFR::Get3D();
            if (pRef3D != nullptr && pRef3D->IsFadeNode()) {
                static_cast<BSFadeNode*>(pRef3D)->TurnFadeNodeOn();
                BSShaderUtil::RecursiveSetPropertyFadeAlpha(pRef3D, 1.0f);
            }
        }
        pNode = pNode->GetNext();
    }
    CellRefLockLeave();
}

void TESObjectCELL::SetCollisionGeometryDisplay(bool abTF) {
    bhkWorld* pWorld = GetbhkWorld();
    NiNode* p3D = Get3D();
    if (pWorld && p3D) {
        pWorld->SetDebugDisplay(p3D, abTF);
        SetActorCollisionGeometry(abTF);
        PrepareObjectWithNoLightingRecurse(p3D);
    }
}

void TESObjectCELL::SetActorCollisionGeometry(bool abTF) {
    NiNode* pactors = static_cast<NiNode*>(GetActorNode());
    uint32_t iactors = pactors->GetArrayCount();
    while (iactors--) {
        NiAVObject* pactorAV = pactors->GetAt(iactors);
        MobileObject* pmob = static_cast<MobileObject*>(TESObjectREFR::FindReferenceFor3D(pactorAV));
        if (pmob) {
            pmob->SetDisplayGeometry(abTF);
        }
    }
    NiNode* pmb = GetChildNode(TESObjectCELL::CN_MULTIBOUND);
    uint32_t ui = 0;
    for (uint32_t i = pmb->GetArrayCount(); ui < i; i = pmb->GetArrayCount()) {
        NiAVObject* pavobj = pmb->GetAt(ui);
        if ((pavobj) && (pavobj->IsMultiBoundNode() || NiObject::IsExactKindOf(&BSMultiBoundRoom::ms_RTTI, pavobj))) {
            BSMultiBoundNode* pmbnode = static_cast<BSMultiBoundNode*>(pavobj);
            int32_t j_2 = 0;
            for (uint32_t j = static_cast<NiNode*>(pavobj)->NiNode::GetArrayCount(); j_2 < j;
                 j = pmbnode->GetArrayCount()) {
                NiAVObject* pavobj_2 = pmbnode->GetAt(static_cast<uint32_t>(j_2));
                if (pavobj_2) {
                    TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pavobj_2);
                    if (pref->IsActor()) {
                        // The same slot (+0x2F0) as the actor loop's call above: MobileObject's.
                        static_cast<MobileObject*>(pref)->SetDisplayGeometry(abTF);
                    }
                }
                ++j_2;
            }
        }

        ++ui;
    }
}

int32_t TESObjectCELL::GetReferenceCount() {
    return listReferences.ItemsInList();
}

BSSimpleList<TESObjectREFR*>* TESObjectCELL::GetRefList() {
    return &listReferences;
}

// Hands every carryable item the cell's references hold to apBagRef, the default loot bag: a light is judged by its
// own flag, everything else by the container rules. The list walk uses BSSimpleList's inline GetItem/GetNext.
void TESObjectCELL::FillLootBag(TESObjectREFR* apBagRef) {
    if (apBagRef == nullptr || apBagRef->GetObjectReference() != pDefLootBag) {
        return;
    }
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* pNode = GetRefList();
    while (pNode != nullptr) {
        TESObjectREFR* pRef = pNode->GetItem();
        if (pRef != nullptr && !pRef->GetDelete() && !pRef->GetDisabled()) {
            uint8_t ucFormType = pRef->GetObjectReference()->GetFormType();
            bool bCanCarry = false;
            if (ucFormType == LIGH_ID) {
                bCanCarry = static_cast<TESObjectLIGH*>(pRef->GetObjectReference())->GetCanCarry();
            }
            else {
                bCanCarry = TESContainer::ContainerCanHoldType(ucFormType);
            }
            if (bCanCarry) {
                int16_t sCount = pRef->GetExtra()->GetCount();
                apBagRef->AddReferenceItem(pRef, sCount, false, false);
                pRef->MarkAsPickedUp();
            }
        }
        pNode = pNode->GetNext();
    }
    CellRefLockLeave();
}

// Adds apRef to the cell's reference list. A persistent cell only files it with its worldspace; otherwise it moves from
// its old cell, joins the loaded lists and has its 3D attached or queued, and exterior references join the auto water
// lists.
void TESObjectCELL::AddReference(TESObjectREFR* apRef, bool abOnTop) {
    if (apRef == nullptr || apRef->GetObjectReference() == nullptr) {
        return;
    }
    if (apRef->GetVisibleDistant() && !apRef->GetDisabled()) {
        ++sNumRefsWithVisibleDistant;
    }
    if (GetPersistentCell()) {
        CellRefLockEnter();
        GetRefList()->AddHead(apRef);
        CellRefLockLeave();
        apRef->GetExtra()->SetPersistentCell(this);
        TESWorldSpace* pWorldSpace = GetWorldSpace();
        if (pWorldSpace != nullptr) {
            pWorldSpace->AddToPersistentRefData(apRef);
        }
        if (!pDataHandler->GetSaveLoad()) {
            SetAltered(true);
        }
        return;
    }

    TESObjectCELL* pOldCell = apRef->GetParentCell();
    if (pOldCell != nullptr) {
        pOldCell->RemoveReference(apRef);
    }
    CellRefLockEnter();
    GetRefList()->AddHead(apRef);
    apRef->SetParentCell(this);
    CellRefLockLeave();
    if (GetAttached()) {
        pLoadedReferenceCollection->AddReference(apRef);
    }
    if (pLoadedData != nullptr) {
        if ((!apRef->IsActor() || apRef->GetDisabled()) && apRef->HasScript()) {
            AddScriptedRef(apRef);
        }
        if (apRef->IsActivatingChildren()) {
            AddActivatingRef(apRef);
        }
    }
    if (!apRef->GetDisabled() && apRef->Get3D() != nullptr && dynamic_cast<MobileObject*>(apRef) == nullptr) {
        apRef->InitHavok();
    }
    if (!pDataHandler->GetSaveLoad() && !apRef->GetDisabledOrDeleted()) {
        NiNode* pRoot = apRef->Get3D();
        if (TES::pInstance->IsCellLoaded(this, false)) {
            bool bCellNotLoading = false;
            if (pRoot != nullptr) {
                AttachReference3D(apRef, abOnTop);
            }
            else {
                if (!apRef->IsActor()) {
                    bCellNotLoading = true;
                }
                else if (!pPlayer->IsSleepingorResting() && !IsLoading()) {
                    bCellNotLoading = true;
                }
                if (bCellNotLoading) {
                    if ((apRef->IsActor()) && (!pSaveLoadGameOLD->GetLoading() || pSaveLoadGameOLD->GetLoading()))
                    // pSaveLoadGameOLD is asked twice whether it is loading, and the actor is moved either way, as
                    // the game does.
                    {
                        if (GetAttached() || IsAttaching()) {
                            static_cast<MobileObject*>(apRef)->MovetoHigh();
                        }
                        else {
                            static_cast<MobileObject*>(apRef)->MovetoMiddleHigh();
                        }
                    }

                    pModelLoader->QueueReference(apRef, TES::pInstance->GetCellPriority(this, nullptr), false);
                }
            }
            if (!bCellNotLoading && apRef->IsActor()) {
                ReferenceProcessLists.AddActorToTempChangeList(static_cast<Actor*>(apRef));
            }
        }
        else {
            QueuedChildAttachMap.RemoveAt(apRef->Get3D());
            if (apRef->IsActor() && !pPlayer->IsSleepingorResting() && !pSaveLoadGameOLD->GetLoading()) {
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    TaskQueueInterface::GetInstance()->QueueSet3DNull(apRef);
                }
                else {
                    apRef->Set3D(nullptr, false);
                }
            }
        }
    }
    apRef->AddLight(false);
    if (!apRef->GetParentCell()->GetInterior()) {
        if (apRef->GetReflectedByAutoWater() && !AutoWaterReflectList.IsInList(apRef)) {
            AutoWaterReflectList.AddHead(apRef);
        }
        if (apRef->GetRefractedByAutoWater() && !AutoWaterRefractList.IsInList(apRef)) {
            AutoWaterRefractList.AddHead(apRef);
        }
    }
}

// Unfiles apRef: its visible-distant count, encounter zone, the reference list, the loaded collection and the loaded
// lists, then either its persistent cell or its parent cell and the occlusion planes linked to it.
void TESObjectCELL::RemoveReference(TESObjectREFR* apRef) {
    if (apRef == nullptr) {
        return;
    }
    if (apRef->GetVisibleDistant()) {
        --sNumRefsWithVisibleDistant;
    }
    if (apRef->GetEncounterZone() == nullptr && !pDataHandler->GetSaveLoad()) {
        if (GetEncounterZone() != nullptr) {
            apRef->SetEncounterZone(GetEncounterZone());
        }
        else {
            apRef->SetEncounterZone(BGSEncounterZone::GetNoZone());
        }
    }
    CellRefLockEnter();
    GetRefList()->Remove(apRef);
    CellRefLockLeave();
    if (GetAttached() || IsAttaching()) {
        pLoadedReferenceCollection->RemoveReference(apRef);
    }
    if (pLoadedData != nullptr) {
        bool bReconstructing = pSaveLoadGame->GetReconstructingForms();
        if (bReconstructing || apRef->HasScript()) {
            RemoveScriptedRef(apRef);
        }
        if (apRef->IsActivatingChildren()) {
            RemoveActivatingRef(apRef);
        }
        RemoveAnimatedRef(apRef);
        RemoveEmittanceRef(apRef);
    }
    if (GetPersistentCell()) {
        apRef->GetExtra()->SetPersistentCell(nullptr);
        TESWorldSpace* pWorld = GetWorldSpace();
        if (pWorld != nullptr) {
            pWorld->RemoveFromPersistentRefData(apRef);
        }
        return;
    }
    apRef->SetParentCell(nullptr);
    if (apRef->GetObjectReference() == static_cast<TESForm*>(pPlaneMarker)) {
        OcclusionPlaneLinkedRefData* pLinkedData = apRef->GetExtra()->GetOcclusionPlaneLinkedRefData();
        if (pLinkedData != nullptr) {
            for (int32_t i = 0; i < 4; ++i) {
                if (pLinkedData->pLinkedRefs[i] == nullptr) {
                    continue;
                }
                OcclusionPlaneLinkedRefData* pOtherData =
                    pLinkedData->pLinkedRefs[i]->GetExtra()->GetOcclusionPlaneLinkedRefData();
                if (pOtherData == nullptr) {
                    continue;
                }
                for (int32_t j = 0; j < 4; ++j) {
                    if (pOtherData->pLinkedRefs[j] == apRef) {
                        pOtherData->pLinkedRefs[j] = nullptr;
                    }
                }
            }
        }
    }
    if (!apRef->GetTemporary() && !apRef->GetRefPersists() && !pDataHandler->GetClearingData()) {
        SetAltered(true);
    }
}

void TESObjectCELL::AddReferenceSimple(TESObjectREFR* apRef) {
    if ((apRef) && (apRef->GetObjectReference())) {
        if (GetPersistentCell()) {
            CellRefLockEnter();
            BSSimpleList<TESObjectREFR*>* RefList = GetRefList();
            RefList->AddHead(apRef);
            CellRefLockLeave();
            ExtraDataList* Extra = apRef->GetExtra();
            Extra->SetPersistentCell(this);
            if (!pDataHandler->GetSaveLoad()) {
                SetAltered(true);
            }
        }
        else {
            TESObjectCELL* ParentCell = apRef->GetParentCell();
            if (ParentCell) {
                ParentCell->RemoveReference(apRef);
            }
            CellRefLockEnter();
            BSSimpleList<TESObjectREFR*>* v4 = GetRefList();
            v4->AddHead(apRef);
            apRef->SetParentCell(this);
            CellRefLockLeave();
            if (!apRef->GetTemporary() && !apRef->GetRefPersists() && !pDataHandler->GetSaveLoad()) {
                SetAltered(true);
            }
            apRef->AddLight(false);
        }
    }
}

void TESObjectCELL::SetPersistentCell(bool abVal) {
    uint32_t uiFlags = GetFormFlags();
    if (abVal) {
        uiFlags |= TESForm::kFormFlags_IsRefPersistent;
    }
    else {
        uiFlags &= ~TESForm::kFormFlags_IsRefPersistent;
    }
    SetFormFlags(uiFlags);
}

void TESObjectCELL::BuildTeleportDoorList(BSSimpleArray<TESObjectREFR*>& arOutArray) {
    CellRefLockEnter();
    for (BSSimpleList<TESObjectREFR*>* pNode = GetRefList(); pNode != nullptr; pNode = pNode->GetNext()) {
        TESObjectREFR* pRef = pNode->GetItem();
        if (pRef == nullptr || pRef->GetDisabled() || pRef->GetDelete()) {
            continue;
        }
        TESBoundObject* pBase = pRef->GetObjectReference();
        if (pBase->GetFormType() == DOOR_ID && pBase != static_cast<TESForm*>(pJailMarker) &&
            pRef->GetTeleport() != nullptr) {
            arOutArray.Add(pRef);
        }
    }
    CellRefLockLeave();
}

// Walks the loaded cell's scripted references (all references when unloaded and bIgnore3D), then lets the activating
// references update their children, dropping those that are done. bIgnoreActors is unused.
bool TESObjectCELL::RunScripts(bool bIgnore3D, [[maybe_unused]] bool bIgnoreActors) {
    bool bRanScript = false;
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* pNode = nullptr;
    if (pLoadedData != nullptr) {
        pNode = &pLoadedData->ScriptedRefs;
    }
    else if (bIgnore3D) {
        pNode = GetRefList();
    }
    while (pNode != nullptr) {
        TESObjectREFR* pRef = pNode->GetItem();
        pNode = pNode->GetNext();
        if ((pRef != nullptr && (bIgnore3D || pRef->Get3D_Body() != nullptr || pRef->GetDisabled())) &&
            (pRef->RunScript() && !bIgnore3D)) {
            bRanScript = true;
            break;
        }
    }
    if (pLoadedData != nullptr && !bRanScript && !Interface::IsInMenuMode()) {
        BSSimpleList<TESObjectREFR*>* pActivating = &pLoadedData->ActivatingRefs;
        BSSimpleList<TESObjectREFR*>* pPrevious = nullptr;
        while (pActivating != nullptr && !pActivating->IsEmpty()) {
            TESObjectREFR* pRef = pActivating->GetItem();
            if (pRef->UpdateChildActivates()) {
                pPrevious = pActivating;
                pActivating = pActivating->GetNext();
            }
            else if (pPrevious != nullptr) {
                pPrevious->Remove(pRef);
                pActivating = pPrevious->GetNext();
            }
            else {
                pActivating->RemoveHead();
            }
        }
    }
    CellRefLockLeave();
    return bRanScript;
}

// A radius of FLT_MAX disables its test; the second test is skipped when it repeats the first.
bool TESObjectCELL::EnumReferencesCloseToPoint(
    const NiPoint3& aPoint1,
    float afRadius1,
    const NiPoint3& aPoint2,
    float afRadius2,
    bool (*apfnEnumFunc)(TESObjectREFR*, void*),
    void* aiEnumData
) {
    if (apfnEnumFunc == nullptr) {
        return false;
    }
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* pNode = GetRefList();
    while (pNode != nullptr && !pNode->IsEmpty()) {
        TESObjectREFR* pRef = pNode->GetItem();
        pNode = pNode->GetNext();
        if (afRadius1 != 3.4028234663852886e+38 &&
            NiPoint3::IsDistanceLongerThanRadius(&pRef->GetLocationOnReference(), &aPoint1, afRadius1) >= 0) {
            continue;
        }
        if (afRadius2 != 3.4028234663852886e+38 && (afRadius1 != afRadius2 || !(aPoint1 == aPoint2)) &&
            NiPoint3::IsDistanceLongerThanRadius(&pRef->GetLocationOnReference(), &aPoint2, afRadius2) >= 0) {
            continue;
        }
        if (apfnEnumFunc(pRef, aiEnumData)) {
            CellRefLockLeave();
            return false;
        }
    }
    CellRefLockLeave();
    return true;
}

void TESObjectCELL::CellRefLockEnter() {
    kSpinLock.Lock("TESObjectCELL::CellRefLockEnter()");
}

void TESObjectCELL::CellRefLockLeave() {
    kSpinLock.Unlock();
}

// Builds the cell's root node on first load: the fixed-bound, always-drawn child nodes the cell's 3D is sorted into
// (each culled per IsNodeCulled; node 1 holds the light and sound marker nodes, node 2 four multibound land nodes, and
// nodes 7 and 8 are attached twice, deliberately, as the game does), then the loaded data, multibounds, Havok and the
// save's changes. Every load makes sure the portal graph has its shared portal geometry root. The AutoMemContext is
// constructed and destroyed explicitly in local storage, and the constructor's result is ignored.
NiNode* TESObjectCELL::Load3D() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_CELLS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 3460
    );
    NiNode* pnode = Get3D();
    if (!pnode) {
        SetState(CS_LOADING);
        bCellDetached = false;
        void* pMemory = NiMemObject::operator new(sizeof(NiNode));
        pnode = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pnode->SetTranslate(NiPoint3_ZERO);
        pnode->SetRotate(NiMatrix3_IDENTITY);
        pnode->SetFixedBound(true);
        pnode->SetAlwaysDraw(true);

        pMemory = NiMemObject::operator new(sizeof(NiNode));
        NiNode* pnew = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_ACTOR));
        pnode->AttachChild(pnew, true);

        pMemory = NiMemObject::operator new(sizeof(NiNode));
        pnew = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetPickOff(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_MARKER));
        pnode->AttachChild(pnew, true);
        NiNode* pmarkernode = pnew;

        pMemory = NiMemObject::operator new(sizeof(NiNode));
        pnew = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_LIGHT_MARKER));
        pmarkernode->AttachChild(pnew, true);
        spLightMarkerNode = pnew;

        pMemory = NiMemObject::operator new(sizeof(NiNode));
        pnew = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_SOUND_MARKER));
        pmarkernode->AttachChild(pnew, true);
        spSoundMarkerNode = pnew;

        pMemory = NiMemObject::operator new(sizeof(NiNode));
        pnew = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_LAND));
        pnode->AttachChild(pnew, true);
        for (int32_t i = 0; i < 4; ++i) {
            void* pLandMemory = NiMemObject::operator new(sizeof(BSMultiBoundNode));
            BSMultiBoundNode* plandnode = pLandMemory ? ::new (pLandMemory) BSMultiBoundNode : nullptr;
            plandnode->SetCullingMode(BSCullingProcess::BSCP_CULL_ALLPASS);
            void* pBoundMemory = NiMemObject::operator new(sizeof(BSMultiBound));
            BSMultiBound* pmultibound = pBoundMemory ? ::new (pBoundMemory) BSMultiBound : nullptr;
            void* pShapeMemory = NiMemObject::operator new(sizeof(BSMultiBoundAABB));
            BSMultiBoundAABB* pmbaabb = pShapeMemory ? ::new (pShapeMemory) BSMultiBoundAABB : nullptr;
            pmultibound->SetShape(pmbaabb);
            plandnode->SetMultiBound(pmultibound);
            pnew->AttachChild(plandnode, true);
        }

        pMemory = NiMemObject::operator new(sizeof(NiNode));
        pnew = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_STATIC));
        pnode->AttachChild(pnew, true);

        // Node 4 is never culled; IsNodeCulled is not asked for it.
        pMemory = NiMemObject::operator new(sizeof(NiNode));
        pnew = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(false);
        pnode->AttachChild(pnew, true);

        pMemory = NiMemObject::operator new(sizeof(NiNode));
        pnew = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_OCCLUSION_PLANE));
        pnode->AttachChild(pnew, true);

        pMemory = NiMemObject::operator new(sizeof(NiNode));
        pnew = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_PORTAL));
        pnode->AttachChild(pnew, true);

        pMemory = NiMemObject::operator new(sizeof(NiNode));
        pnew = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_MULTIBOUND));
        pnode->AttachChild(pnew, true);
        pnode->AttachChild(pnew, true);

        pMemory = NiMemObject::operator new(sizeof(NiNode));
        pnew = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_COLLISION));
        pnode->AttachChild(pnew, true);
        pnode->AttachChild(pnew, true);

        NiUpdateData UpdateData;
        UpdateData.~NiUpdateData();
        ::new (&UpdateData) NiUpdateData(0.0f, false, false);
        pnode->Update(UpdateData);
        CreateLoadedData();
        pLoadedData->spCell3D = pnode;
        AttachMultiBoundNodes();
        InitHavok();
        if (!pSaveLoadGame->GetSaveGameLoading()) {
            bool brestoreAllowChanges = pSaveLoadGame->SetThreadAllowChanges(false);
            pSaveLoadGame->LoadCell(this);
            if (!pSaveLoadGame->GetDeferInitForms()) {
                pSaveLoadGame->InitForms(false);
            }
            pSaveLoadGame->SetThreadAllowChanges(brestoreAllowChanges);
        }
        SetState(CS_LOADED);
    }
    BSPortalGraph* pportalgraph = CreatePortalGraph();
    if (!pportalgraph->GetRootNode()) {
        void* pMemory = NiMemObject::operator new(sizeof(NiNode));
        NiNode* ppnr = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        NiFixedString kName("Shared Portal Geometry");
        ppnr->SetName(kName);
        pportalgraph->SetPortalNodeRoot(ppnr);
    }
    pContext->~AutoMemContext();
    return pnode;
}

// A cell without water, and an interior cell, is at -FLT_MAX. A cell whose height is still FLT_MAX takes its
// worldspace's default height; a NaN height is kept.
float TESObjectCELL::GetWaterHeight() {
    if (!HasWater() || GetInterior()) {
        return -3.4028234663852886e+38f;
    }
    if (fWaterHeight != 3.4028234663852886e+38) {
        return fWaterHeight;
    }
    TESWorldSpace* pWorld = GetWorldSpace();
    if (pWorld != nullptr) {
        return pWorld->GetDefaultWaterHeight();
    }
    return 0.0f;
}

// The water height at arPos: the cell's own water level, raised to the top of any water reference whose multibound
// contains arPos (tested at the bound's centre height). Interiors without water report -FLT_MAX and false. The bounds,
// angle and the copied rotation are read but unused, as the game does.
bool TESObjectCELL::GetWaterHeight(const NiPoint3& arPos, float& afHeight) {
    bool bResult = false;
    if (GetInterior() && !HasWater()) {
        afHeight = -3.4028235e+38f;
        return false;
    }
    afHeight = GetWaterHeight();
    NiPoint3 kPos = arPos;
    LoadedCellData* pLoadedData = GetLoadedData();
    if (pLoadedData) {
        // The condition and the item read go through GetItem, the step through GetNext.
        for (BSSimpleList<TESObjectREFR*>* pIter = &pLoadedData->WaterRefs; pIter && pIter->GetItem();
             pIter = pIter->GetNext()) {
            TESObjectREFR* pWaterRef = pIter->GetItem();
            [[maybe_unused]] NiPoint3 kMin = pWaterRef->GetBoundMin();
            [[maybe_unused]] NiPoint3 kMax = pWaterRef->GetBoundMax();
            [[maybe_unused]] NiPoint3 kRotation = pWaterRef->GetAngleOnReference();
            NiAVObject* pChildNode = pWaterRef->Get3D()->IsNode()->GetAt(0);
            BSMultiBoundNode* pMultiBoundNode = pChildNode->IsMultiBoundNode();
            if (!pMultiBoundNode) {
                continue;
            }
            BSMultiBoundShape* pMultiBoundShape = pMultiBoundNode->QMultiBound()->QShape();
            if (pMultiBoundShape->QType() == BSMultiBoundShape::BS_SHAPE_AABB) {
                kPos.z = static_cast<BSMultiBoundAABB*>(pMultiBoundShape)->QCenter().z;
            }
            if (pMultiBoundShape->QType() == BSMultiBoundShape::BS_SHAPE_OBB) {
                kPos.z = static_cast<BSMultiBoundOBB*>(pMultiBoundShape)->QCenter().z;
            }
            if (pMultiBoundNode->QPointWithin(kPos)) {
                float z = pWaterRef->GetLocationOnReference().z;
                if (afHeight < z) {
                    afHeight = z;
                }
                bResult = true;
            }
        }
    }
    return bResult;
}

// Stores afWaterHeight, or FLT_MAX ("use the world space's") when it equals the world space's default water height
// within 0.001.
void TESObjectCELL::SetWaterHeight(float afWaterHeight) {
    TESWorldSpace* pWorldSpace = GetWorldSpace();
    if (pWorldSpace && FloatEQ(pWorldSpace->GetDefaultWaterHeight(), afWaterHeight, 0.001f)) {
        fWaterHeight = 3.4028235e+38f;
    }
    else {
        fWaterHeight = afWaterHeight;
    }
}

// Hangs apRef's 3D in the cell node its kind belongs under: actors, markers (rooms, portals and occlusion planes build
// their portal-graph objects on the way), lights, sounds, projectiles, then the reference's multibound node or the
// static node. Animated references (trees, flickering lights, animated BSX 3D) join the animated list. Placeable water
// joins the water system; the player's two 3D roots are attached directly, anything else through the cell's attach
// (queued on the task thread when it runs) and then faded in. abOnTop is unused. TESObjectREFR::Get3D is called
// non-virtually at the entry and after the attach, and virtually for the BSX flags.
void TESObjectCELL::AttachReference3D(TESObjectREFR* apRef, [[maybe_unused]] bool abOnTop) {
    bool bDisableFade = false;
    if (!apRef || !apRef->TESObjectREFR::Get3D()) {
        return;
    }
    if (apRef->GetObjectReference()->GetLODTree()) {
        TESWorldSpace* pworld = apRef->GetWorldSpace();
        if (pworld) {
            pworld->GetTerrainManager()->HideTree(apRef, true);
            pworld->GetTerrainManager()->SetNeedsImmediateUpdate(true);
        }
    }
    if (apRef->GetObjectReference() && apRef->GetObjectReference()->GetFormType() == PWAT_ID) {
        TES::pInstance->IncrementPlaceableWaterCount();
        if (GetbhkWorld()->QWaterListener()) {
            GetbhkWorld()->QWaterListener()->SetEnabled(true);
        }
        if (GetInterior() && !HasWater()) {
            const char* pEditorID = GetFormEditorID();
            uint32_t iCellID = GetFormID();
            Warning(
                "Trying to add placeable water reference %08X to waterless cell %08X '%s'. This will cause a "
                "crash.\n",
                apRef->GetFormID(), iCellID, pEditorID
            );
            if (!TESWaterSystem::bUseWater.Bool()) {
                return;
            }
        }
    }

    NiNode* pAttach = nullptr;
    if (apRef->IsActor()) {
        pAttach = static_cast<NiNode*>(GetChildNode(CN_ACTOR));
    }
    else if (apRef->GetObjectReference() == pBoundMarker) {
        pAttach = static_cast<NiNode*>(GetChildNode(CN_MARKER));
    }
    else if (apRef->GetObjectReference() == pRoomMarker) {
        if (apRef->GetExtra()->GetRoomIsMaster()) {
            if (!apRef->GetExtra()->GetRoom()) {
                BSMultiBoundRoom* proom = apRef->GetMultiBoundRoom();
                if (!proom) {
                    NiPoint3 kangle = apRef->GetAngleOnReference();
                    BGSPrimitive* pprim = apRef->GetExtra()->GetPrimitive();
                    BSMultiBoundShape* pshape = pprim->MakeMultiBoundShape(kangle);
                    void* pMemory = NiMemObject::operator new(sizeof(BSMultiBoundRoom));
                    BSMultiBoundRoom* pnewroom = pMemory ? ::new (pMemory) BSMultiBoundRoom : nullptr;
                    void* pBoundMemory = NiMemObject::operator new(sizeof(BSMultiBound));
                    BSMultiBound* pbound = pBoundMemory ? ::new (pBoundMemory) BSMultiBound : nullptr;
                    pnewroom->SetMultiBound(pbound);
                    pbound->SetShape(pshape);
                    pbound->SetCenter(apRef->GetLocationOnReference());
                    apRef->GetExtra()->SetRoom(pnewroom);
                }
            }
            BSMultiBoundRoom* proom = apRef->GetExtra()->GetRoom();
            BSMultiBoundShape* pshape = proom->QMultiBound()->QShape();
            pshape->SetCenter(apRef->GetLocationOnReference());
            BSPortalGraph* pportalgraph = GetPortalGraph();
            pportalgraph->AddRoom(proom);
        }
        pAttach = static_cast<NiNode*>(GetChildNode(CN_MARKER));
    }
    else if (apRef->GetObjectReference() == pPortalMarker && !apRef->GetTemporary()) {
        if (!apRef->GetExtra()->GetPortal()) {
            BGSPrimitive* pprim = apRef->GetExtra()->GetPrimitive();
            // A portal marker's primitive is a plane; MakePortal is called on it.
            BSPortal* pportal = static_cast<BGSPrimitivePlane*>(pprim)->MakePortal();
            if (pportal) {
                pportal->SetCenter(apRef->GetLocationOnReference());
                pportal->SetRotation(apRef->GetOrientation());
                apRef->GetExtra()->SetPortal(pportal);
                void* pMemory = NiMemObject::operator new(sizeof(BSPortalSharedNode));
                BSPortalSharedNode* pportalnode = pMemory ? ::new (pMemory) BSPortalSharedNode(pportal) : nullptr;
                NiFixedString kName("Portal Shared Geometry");
                pportalnode->SetName(kName);
                pportal->SetSharedGeometryNode(pportalnode);
                BSPortalGraph* pportalgraph = GetPortalGraph();
                pportalgraph->GetRootNode()->AttachChild(pportalnode, true);
                PortalLinkedRefData* pdata = apRef->GetExtra()->GetPortalLinkedRefData();
                if (pdata) {
                    for (int32_t i = 0; i < 2; ++i) {
                        if (pdata && pdata->pLinkedRefs[i]) {
                            BSMultiBoundRoom* proom = pdata->pLinkedRefs[i]->GetMultiBoundRoom();
                            if (!proom) {
                                proom = pdata->pLinkedRefs[i]->GetExtra()->GetMasterRoom()->GetMultiBoundRoom();
                            }
                            if (proom) {
                                proom->QPortalList().AddTail(pportal);
                                if (i == 0) {
                                    pportal->SetMultiBoundRoomLeft(proom);
                                }
                                else {
                                    pportal->SetMultiBoundRoomRight(proom);
                                }
                            }
                        }
                        else {
                            BSPortalGraph* pportalgraph = GetPortalGraph();
                            pportalgraph->AddPortal(pportal);
                        }
                    }
                }
            }
        }
        pAttach = static_cast<NiNode*>(GetChildNode(CN_PORTAL));
    }
    else if (apRef->GetObjectReference() == pPlaneMarker) {
        pAttach = static_cast<NiNode*>(GetChildNode(CN_OCCLUSION_PLANE));
        BSOcclusionPlane* pplane = apRef->GetExtra()->GetOcclusionPlane();
        if (!pplane) {
            void* pMemory = NiMemObject::operator new(sizeof(BSOcclusionPlane));
            pplane = pMemory ? ::new (pMemory) BSOcclusionPlane : nullptr;
            apRef->GetExtra()->SetOcclusionPlane(pplane);
        }
        pplane->SetRotation(apRef->GetOrientation());
        pplane->SetCenter(apRef->GetLocationOnReference());
        BGSPrimitive* pprim = apRef->GetExtra()->GetPrimitive();
        if (pprim) {
            NiPoint3 kradii3 = pprim->GetRadii();
            NiPoint2 kradii2(kradii3.x, kradii3.z);
            pplane->SetHalfExtents(kradii2);
        }
    }
    else if (apRef->GetObjectReference() == pCollisionMarker) {
        pAttach = static_cast<NiNode*>(GetChildNode(CN_COLLISION));
    }
    else if (apRef->IsMarker() && apRef->GetObjectReference()->GetFormType() == LIGH_ID) {
        pAttach = LightMarkerNode();
        if (!static_cast<TESObjectLIGH*>(apRef->GetObjectReference())->GetNoFlicker()) {
            AddAnimatedRef(apRef);
        }
    }
    else if (apRef->IsMarker() || apRef->GetExtra()->GetPrimitive()) {
        if (apRef->GetObjectReference()->GetFormType() == ASPC_ID) {
            pAttach = SoundMarkerNode();
        }
        else {
            pAttach = static_cast<NiNode*>(GetChildNode(CN_MARKER));
        }
    }
    else if (apRef->GetObjectReference()->GetFormType() == SOUN_ID) {
        pAttach = SoundMarkerNode();
    }
    else {
        if (apRef->GetObjectReference()->GetFormType() == TREE_ID) {
            AddAnimatedRef(apRef);
        }
        else if (apRef->IsProjectile()) {
            pAttach = static_cast<NiNode*>(GetChildNode(CN_DYNAMIC));
        }
        else if (
            apRef->GetObjectReference() && apRef->GetObjectReference()->GetFormType() == LIGH_ID &&
            !static_cast<TESObjectLIGH*>(apRef->GetObjectReference())->GetNoFlicker()
        ) {
            AddAnimatedRef(apRef);
        }
        else {
            // Fetches the tag before the virtual Get3D.
            const NiFixedString* pTag = &BSXFlags::Tag();
            NiNode* p3D = apRef->Get3D();
            BSXFlags* pflags = static_cast<BSXFlags*>(p3D->GetExtraData(*pTag));
            if (pflags && pflags->QAnimated() && TESObjectREFR::HasTimeControllers(apRef->Get3D())) {
                bDisableFade = true;
                AddAnimatedRef(apRef);
            }
        }
        if (!pAttach && bUseMultibounds) {
            TESObjectREFR* pboundref = apRef->GetExtra()->GetMultiBoundRef();
            if (pboundref) {
                pAttach = apRef->GetParentCell()->GetMultiBoundNode(pboundref);
            }
        }
        if (!pAttach) {
            pAttach = static_cast<NiNode*>(GetChildNode(CN_STATIC));
        }
    }

    if (apRef->CheckUsesExternalEmittance()) {
        AddEmittanceRef(apRef);
    }
    if (apRef->GetObjectReference()->GetFormType() == PWAT_ID) {
        AddWaterRef(apRef);
        TES::pInstance->GetWaterSystem()->AddPlaceableWater(apRef);
    }
    if (!pAttach) {
        return;
    }
    if (apRef == pPlayer) {
        pAttach->AttachChild(pPlayer->Get3D(false), true);
        pPlayer->Get3D(false)->UpdateProperties();
        BSFadeNode* pfadenode = pPlayer->Get3D(false)->IsFadeNode();
        if (pfadenode) {
            pfadenode->TurnFadeNodeOn();
        }
        pAttach->AttachChild(pPlayer->Get3D(true), true);
        pPlayer->Get3D(true)->UpdateProperties();
        return;
    }
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        QueueAttachChild(apRef, pAttach);
    }
    else {
        PerformCellNodeAttach(apRef, pAttach);
    }
    BSFadeNode* pfadenode = apRef->TESObjectREFR::Get3D()->IsFadeNode();
    if (!pfadenode) {
        return;
    }
    if (apRef->GetObjectReference()->GetLODTree()) {
        pfadenode->SetIgnoreFade(true);
    }
    else if (apRef->GetVisibleDistant()) {
        pfadenode->ForceLODMultType(LOD_MULT_DISTANTLOD);
    }
    else if (apRef->GetIsImposter()) {
        pfadenode->ForceLODMultType(LOD_MULT_DIST_OVERRIDE);
        // FLT_MAX.
        pfadenode->SetRange(3.40282347e+38f, 3.40282347e+38f);
    }
    if (Interface::GetLoadingMenuVisible() || pSaveLoadGame->GetSaveGameLoading()) {
        pfadenode->TurnFadeNodeOn();
        BSShaderUtil::RecursiveSetPropertyFadeAlpha(pfadenode, 1.0f);
        fLodFadeInPercent = fLodFadeTime;
        bFadingToHighDetail = false;
        bFadingToLowDetail = false;
        bDisplayHighDetail = true;
    }
    else {
        pfadenode->CheckFadeRadius(Main::GetWorldSceneGraph()->GetCamera(), bDisableFade);
    }
    if (sNumLoadedRefsWithVisibleDistant > 0 && All3DWithVisibleDistantLoaded() && ShouldDisplayHighDetail()) {
        FadeToHighDetail();
    }
}

// Attaches apRef's 3D to apParent, or detaches it from its parent, under the cell lock. Unless it went into the
// dynamic node or a multibound node, it is tested against the multibounds; then its lighting is updated.
void TESObjectCELL::PerformCellNodeAttach(TESObjectREFR* apRef, NiNode* apParent) {
    if (apRef == nullptr || apRef->Get3D_Body() == nullptr) {
        return;
    }
    CellRefLockEnter();
    NiNode* p3D = apRef->Get3D_Body();
    if (apParent != nullptr) {
        apParent->AttachChild(p3D, true);
    }
    else if (p3D->GetParent() != nullptr) {
        NiNode* pOldParent = p3D->GetParent();
        pOldParent->DetachChild(p3D);
    }
    if (apParent == nullptr || (apParent != GetDynamicNode() && apParent->IsMultiBoundNode() == nullptr)) {
        TestObjectAgainstMultibounds(apRef, true, false);
    }
    NiNode* pRef3D = apRef->Get3D();
    ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
    pShadowSceneNode->UpdateObjectLighting(pRef3D, false);
    CellRefLockLeave();
}

// Darkens the cell's 64x64 canopy shadow mask in a disc of radius aiRadius (in units, 64 per texel) around aLocation;
// texels past the cell's edge go to the eight neighbouring cells' masks through PlaceCanopyShadowInAdjacentCell. The
// loop counters are compared with the unsigned bounds and the texel ranges with signed compares, in the order written
// here. The texture is locked with IDirect3DTexture9::LockRect.
void TESObjectCELL::PlaceCanopyShadow(NiPoint3 aLocation, uint32_t aiRadius, float afFalloff) {
    if (g_bNoCanopy || GetInterior()) {
        return;
    }
    TESWorldSpace* pWorldSpace = GetWorldSpace();
    if (!*pWorldSpace->GetCanopyShadowTextureName()) {
        return;
    }
    if (BSTreeManager::GetUseCanopyShadows()) {
        BSTreeManager::GetCanopyShadow();
    }
    NiRenderedTexture* pTex = nullptr;
    _D3DLOCKED_RECT* pPixData = nullptr;
    _D3DLOCKED_RECT* pPixDataN = nullptr;
    _D3DLOCKED_RECT* pPixDataS = nullptr;
    _D3DLOCKED_RECT* pPixDataE = nullptr;
    _D3DLOCKED_RECT* pPixDataW = nullptr;
    _D3DLOCKED_RECT* pPixDataNE = nullptr;
    _D3DLOCKED_RECT* pPixDataNW = nullptr;
    _D3DLOCKED_RECT* pPixDataSE = nullptr;
    _D3DLOCKED_RECT* pPixDataSW = nullptr;
    GetExtra()->GetCanopyShadowMask(pTex, pPixData);
    if (!aiRadius || aiRadius > 1024) {
        aiRadius = 300;
    }
    if (!pTex) {
        TESObjectCELL::CreateCanopyShadowMaskForCell(this, pTex, pPixData);
    }
    COORD_DATA kCData;
    if (!GetLand() || !GetLand()->GetCoordData(&kCData, aLocation, false)) {
        return;
    }
    float fTreeX = kCData.Cellpoint.x / 64.0;
    float fTreeY = kCData.Cellpoint.y / 64.0;
    aiRadius >>= 6;
    if (!pTex) {
        return;
    }
    static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->LockRenderer();
    if (!pPixData->pBits) {
        NiDX9DynamicTextureData* pTexData = static_cast<NiDX9DynamicTextureData*>(pTex->m_pkRendererData);
        if (pTexData) {
            IDirect3DTexture9* pD3DTexture = static_cast<IDirect3DTexture9*>(pTexData->GetD3DTexture());
            pD3DTexture->LockRect(0, pPixData, nullptr, 0);
        }
    }
    uint8_t* pData = static_cast<uint8_t*>(pPixData->pBits);
    int32_t iHeight = pTex->GetHeight();
    int32_t iTreeX = static_cast<int32_t>(fTreeX);
    int32_t iTreeY = static_cast<int32_t>(fTreeY);
    for (int32_t iY = iTreeY - aiRadius; static_cast<uint32_t>(iY) < iTreeY + aiRadius; ++iY) {
        for (int32_t iX = iTreeX - aiRadius; static_cast<uint32_t>(iX) < iTreeX + aiRadius; ++iX) {
            float fDistSq = (fTreeX - static_cast<double>(iX)) * (fTreeX - static_cast<double>(iX)) +
                (fTreeY - static_cast<double>(iY)) * (fTreeY - static_cast<double>(iY));
            float fDist = std::sqrt(fDistSq);
            if (fDist <= static_cast<double>(aiRadius)) {
                int32_t iIndex = iX + iHeight * iY;
                double fRandom = RandomFloat(0.0f, 55.0f);
                uint8_t ucValue = static_cast<int32_t>(255.0 - fRandom - fDist / static_cast<double>(aiRadius) * 60.0);
                if (iY < 64 && iY >= 0 && iX < 64 && iX >= 0) {
                    if (pData[iIndex]) {
                        pData[iIndex] = 0xFF;
                    }
                    else {
                        pData[iIndex] = ucValue;
                    }
                }
                else if (iY >= 64 && iX < 0) {
                    iIndex = (iY - 64) * iHeight + iX + 64;
                    int32_t iDataY = GetDataY();
                    int32_t iDataX = GetDataX();
                    PlaceCanopyShadowInAdjacentCell(iDataX - 1, iDataY + 1, pPixDataNW, iIndex, ucValue);
                }
                else if (iY >= 64 && iX >= 64) {
                    iIndex = (iY - 64) * iHeight + iX - 64;
                    int32_t iDataY = GetDataY();
                    int32_t iDataX = GetDataX();
                    PlaceCanopyShadowInAdjacentCell(iDataX + 1, iDataY + 1, pPixDataNE, iIndex, ucValue);
                }
                else if (iY < 0 && iX < 0) {
                    iIndex = (iY + 64) * iHeight + iX + 64;
                    int32_t iDataY = GetDataY();
                    int32_t iDataX = GetDataX();
                    PlaceCanopyShadowInAdjacentCell(iDataX - 1, iDataY - 1, pPixDataSW, iIndex, ucValue);
                }
                else if (iY < 0 && iX >= 64) {
                    iIndex = (iY + 64) * iHeight + iX - 64;
                    int32_t iDataY = GetDataY();
                    int32_t iDataX = GetDataX();
                    PlaceCanopyShadowInAdjacentCell(iDataX + 1, iDataY - 1, pPixDataSE, iIndex, ucValue);
                }
                else if (iY >= 64) {
                    iIndex = (iY - 64) * iHeight + iX;
                    int32_t iDataY = GetDataY();
                    int32_t iDataX = GetDataX();
                    PlaceCanopyShadowInAdjacentCell(iDataX, iDataY + 1, pPixDataN, iIndex, ucValue);
                }
                else if (iY < 0) {
                    iIndex = (iY + 64) * iHeight + iX;
                    int32_t iDataY = GetDataY();
                    int32_t iDataX = GetDataX();
                    PlaceCanopyShadowInAdjacentCell(iDataX, iDataY - 1, pPixDataS, iIndex, ucValue);
                }
                else if (iX >= 64) {
                    iIndex = iY * iHeight + iX - 64;
                    int32_t iDataY = GetDataY();
                    int32_t iDataX = GetDataX();
                    PlaceCanopyShadowInAdjacentCell(iDataX + 1, iDataY, pPixDataE, iIndex, ucValue);
                }
                else if (iX < 0) {
                    iIndex = iY * iHeight + iX + 64;
                    int32_t iDataY = GetDataY();
                    int32_t iDataX = GetDataX();
                    PlaceCanopyShadowInAdjacentCell(iDataX - 1, iDataY, pPixDataW, iIndex, ucValue);
                }
            }
        }
    }
    static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->UnlockRenderer();
}

// Both kinds of old record are freed with ourFree.
void TESObjectCELL::CreateCellData() {
    if (pCellData != nullptr) {
        if (GetInterior()) {
            ourFree(pCellData);
        }
        else {
            ourFree(pCellData);
        }
        pCellData = nullptr;
    }
    if (GetInterior()) {
        void* pMemory = ourMalloc(sizeof(INTERIOR_DATA));
        INTERIOR_DATA* pData = pMemory ? ::new (pMemory) INTERIOR_DATA : nullptr;
        pCellData = pData;
    }
    else {
        void* pMemory = ourMalloc(sizeof(EXTERIOR_DATA));
        EXTERIOR_DATA* pData = pMemory ? ::new (pMemory) EXTERIOR_DATA : nullptr;
        pCellData = pData;
    }
}

uint32_t TESObjectCELL::GetDataAmbient() {
    INTERIOR_DATA* pData = GetInteriorData();
    if (pData == nullptr) {
        return 0;
    }
    if (GetInheritFlag(kInherit_AmbientColor) && GetLightingTemplate() != nullptr) {
        return GetLightingTemplate()->GetDataAmbient();
    }
    return pData->iAmbient;
}

// Each channel byte is divided by the double 255.0 and stored as a float.
void TESObjectCELL::GetDataAmbientColor(NiColor& acolColor) {
    uint32_t iColor = GetDataAmbient();
    acolColor.r = static_cast<float>(static_cast<int32_t>(iColor & 0xFF) / 255.0);
    acolColor.g = static_cast<float>(static_cast<int32_t>((iColor >> 8) & 0xFF) / 255.0);
    acolColor.b = static_cast<float>(static_cast<int32_t>((iColor >> 16) & 0xFF) / 255.0);
}

uint32_t TESObjectCELL::GetDataDirectional() {
    INTERIOR_DATA* pData = GetInteriorData();
    if (pData == nullptr) {
        return 0;
    }
    if (GetInheritFlag(kInherit_DirectionalColor) && GetLightingTemplate() != nullptr) {
        return GetLightingTemplate()->GetDataDirectional();
    }
    return pData->iDirectional;
}

void TESObjectCELL::GetDataDirectionalColor(NiColor& acolColor) {
    uint32_t iColor = GetDataDirectional();
    acolColor.r = static_cast<float>(static_cast<int32_t>(iColor & 0xFF) / 255.0);
    acolColor.g = static_cast<float>(static_cast<int32_t>((iColor >> 8) & 0xFF) / 255.0);
    acolColor.b = static_cast<float>(static_cast<int32_t>((iColor >> 16) & 0xFF) / 255.0);
}

uint32_t TESObjectCELL::GetDataDirectionalXY() {
    INTERIOR_DATA* pData = GetInteriorData();
    if (pData == nullptr) {
        return 0;
    }
    if (GetInheritFlag(kInherit_DirectionalRotation) && GetLightingTemplate() != nullptr) {
        return GetLightingTemplate()->GetDataDirectionalXY();
    }
    return pData->iDirectionalXY;
}

uint32_t TESObjectCELL::GetDataDirectionalZ() {
    INTERIOR_DATA* pData = GetInteriorData();
    if (pData == nullptr) {
        return 0;
    }
    if (GetInheritFlag(kInherit_DirectionalRotation) && GetLightingTemplate() != nullptr) {
        return GetLightingTemplate()->GetDataDirectionalZ();
    }
    return pData->iDirectionalZ;
}

uint32_t TESObjectCELL::GetDataFog() {
    INTERIOR_DATA* pData = GetInteriorData();
    if (pData == nullptr) {
        return 0;
    }
    if (GetInheritFlag(kInherit_FogColor) && GetLightingTemplate() != nullptr) {
        return GetLightingTemplate()->GetDataFog();
    }
    return pData->iFog;
}

void TESObjectCELL::GetDataFogColor(NiColor& acolColor) {
    uint32_t iColor = GetDataFog();
    acolColor.r = static_cast<float>(static_cast<int32_t>(iColor & 0xFF) / 255.0);
    acolColor.g = static_cast<float>(static_cast<int32_t>((iColor >> 8) & 0xFF) / 255.0);
    acolColor.b = static_cast<float>(static_cast<int32_t>((iColor >> 16) & 0xFF) / 255.0);
}

float TESObjectCELL::GetDataFogNearPlane() {
    INTERIOR_DATA* pData = GetInteriorData();
    if (pData == nullptr) {
        return 0.0f;
    }
    if (GetInheritFlag(kInherit_FogNear) && GetLightingTemplate() != nullptr) {
        return GetLightingTemplate()->GetDataFogNearPlane();
    }
    return pData->fFogNear;
}

float TESObjectCELL::GetDataFogFarPlane() {
    INTERIOR_DATA* pData = GetInteriorData();
    if (pData == nullptr) {
        return 0.0f;
    }
    if (GetInheritFlag(kInherit_FogFar) && GetLightingTemplate() != nullptr) {
        return GetLightingTemplate()->GetDataFogFarPlane();
    }
    return pData->fFogFar;
}

// Without interior data the fog power is 1.
float TESObjectCELL::GetDataFogPower() {
    INTERIOR_DATA* pData = GetInteriorData();
    if (pData == nullptr) {
        return 1.0f;
    }
    if (GetInheritFlag(kInherit_FogPower) && GetLightingTemplate() != nullptr) {
        return GetLightingTemplate()->GetDataFogPower();
    }
    return pData->fFogPower;
}

float TESObjectCELL::GetDataClipDist() {
    INTERIOR_DATA* pData = GetInteriorData();
    if (pData == nullptr) {
        return 0.0f;
    }
    if (GetInheritFlag(kInherit_ClipDistance) && GetLightingTemplate() != nullptr) {
        return GetLightingTemplate()->GetDataClipDist();
    }
    return pData->fClipDist;
}

// Files the cell's references: multibound references in MultiboundRefMap, scripted non-actors and references that
// activate children in their lists. Deleted references count only while a save game loads. Multibound nodes are then
// parented to their multibound reference's node, and the worldspace's overlapping multibound references are added.
void TESObjectCELL::CreateLoadedData() {
    if (pLoadedData == nullptr) {
        void* pMemory = ourMalloc(sizeof(LoadedCellData));
        LoadedCellData* pData = pMemory ? ::new (pMemory) LoadedCellData : nullptr;
        pLoadedData = pData;
    }
    CellRefLockEnter();
    alignas(BSSimpleList<TESObjectREFR*>) uint8_t aMultiBoundStorage[sizeof(BSSimpleList<TESObjectREFR*>)];
    BSSimpleList<TESObjectREFR*>& kMultiBoundRefs = *::new (aMultiBoundStorage) BSSimpleList<TESObjectREFR*>;
    BSSimpleList<TESObjectREFR*>* pIter = GetRefList();
    while (pIter != nullptr && !pIter->IsEmpty()) {
        TESObjectREFR* pRef = pIter->GetItem();
        pIter = pIter->GetNext();
        if (pRef->GetDelete() && !pSaveLoadGame->GetSaveGameLoading()) {
            continue;
        }
        if (pRef->IsMultiBoundRef()) {
            AddMultiBoundRef(pRef);
            if (pRef->GetMultiBoundRef() != nullptr) {
                kMultiBoundRefs.AddHead(pRef);
            }
        }
        else {
            if (!pRef->IsActor() && pRef->HasScript()) {
                pLoadedData->ScriptedRefs.AddHead(pRef);
            }
            if (pRef->IsActivatingChildren()) {
                pLoadedData->ActivatingRefs.AddHead(pRef);
            }
        }
    }
    CellRefLockLeave();
    while (!kMultiBoundRefs.IsEmpty()) {
        TESObjectREFR* pRef = kMultiBoundRefs.GetItem();
        kMultiBoundRefs.RemoveHead();
        TESObjectREFR* pMultiBoundRef = pRef->GetMultiBoundRef();
        NiNode* pNode = GetMultiBoundNode(pRef);
        NiNode* pParentNode = GetMultiBoundNode(pMultiBoundRef);
        if (pNode != pParentNode) {
            pParentNode->AttachChild(pNode, true);
        }
    }
    TESWorldSpace* pWorld = GetWorldSpace();
    if (pWorld != nullptr) {
        BSSimpleList<TESObjectREFR*>* pOverlap = pWorld->GetOverlappedMultiBoundRefs(this);
        while (pOverlap != nullptr && !pOverlap->IsEmpty()) {
            AddMultiBoundRef(pOverlap->GetItem());
            pOverlap = pOverlap->GetNext();
        }
    }
    kMultiBoundRefs.~BSSimpleList<TESObjectREFR*>();
}

// References whose base object's bound is larger than 3000 also animate from afar.
void TESObjectCELL::AddAnimatedRef(TESObjectREFR* apRef) {
    if (pLoadedData == nullptr || apRef == nullptr || apRef->Get3D() == nullptr) {
        return;
    }
    NiNode* p3D = apRef->Get3D();
    pLoadedData->AnimatedRefMap.SetAt(apRef, p3D->IsNode());
    if (apRef->GetObjectReference()->GetBoundSize() > 3000.0) {
        pLoadedData->LargeAnimatedRefs.AddHead(apRef);
    }
}

// A light reference is filed by itself. Any other reference gets its own materials, lit by the emittance colour of
// its emittance source, else of the player's current region.
void TESObjectCELL::AddEmittanceRef(TESObjectREFR* apRef) {
    if (pLoadedData == nullptr || apRef == nullptr || apRef->Get3D() == nullptr) {
        return;
    }
    NiNode* p3D = apRef->Get3D();
    if (apRef->GetObjectReference()->GetFormType() == LIGH_ID) {
        pLoadedData->EmittanceLightRefMap.SetAt(apRef, p3D->IsNode());
        return;
    }
    MakeUniqueMaterial_Recurse(p3D);
    p3D->UpdateProperties();
    TESRegion* pSource = static_cast<TESRegion*>(apRef->GetEmittanceSource());
    if (pSource == nullptr) {
        pSource = pPlayer->GetOccupiedRegion();
    }
    if (pSource == nullptr) {
        return;
    }
    NiColor* pColor = pSource->GetEmittanceColor();
    if (pColor == nullptr) {
        return;
    }
    // Fetches the material property and does not use it, as the game does.
    p3D->GetProperty(NiProperty::MATERIAL);
    BSShaderManager::SetExternalEmittanceRecurse(p3D, pColor);
    pLoadedData->EmittanceSourceRefMap.SetAt(pSource, apRef);
}

void TESObjectCELL::AddWaterRef(TESObjectREFR* apRef) {
    if (pLoadedData) {
        (&pLoadedData->WaterRefs)->AddHead(apRef);
    }
}

bool TESObjectCELL::All3DWithVisibleDistantLoaded() {
    return (bool)((uint8_t)(((int32_t)((int16_t)sNumRefsWithVisibleDistant)) <=
                            ((int32_t)sNumLoadedRefsWithVisibleDistant)));
}

bool TESObjectCELL::ShouldDisplayHighDetail() {
    return (((int32_t)GetState()) != 0);
}

void TESObjectCELL::FadeToHighDetail() {
    if (!bFadingToHighDetail) {
        bFadingToHighDetail = true;
        bFadingToLowDetail = false;
        fLodFadeInPercent = (float)0.0;
    }
}

// Returns the node it filed in the loaded data's MultiboundRefMap. The node goes to SetAt as a by-value NiPointer.
BSMultiBoundNode* TESObjectCELL::AddMultiBoundRef(TESObjectREFR* apRef) {
    NiPoint3 kAngle;
    if (!pLoadedData || (!apRef || !apRef->IsMultiBoundRef())) {
        return nullptr;
    }
    BSMultiBoundNode* pMBNode = (BSMultiBoundNode*)(&apRef->m_Extra)->GetRoom();
    BSMultiBound* pMB = nullptr;
    if (!pMBNode) {
        TESBoundObject* pObjectReference = apRef->GetObjectReference();
        if (pObjectReference != pRoomMarker) {
            BSMultiBoundNode* pNewNode;
            void* pMemory = NiMemObject::operator new(sizeof(BSMultiBoundNode));
            if (pMemory) {
                pNewNode = ::new (pMemory) BSMultiBoundNode;
            }
            else {
                pNewNode = nullptr;
            }
            pMBNode = pNewNode;
            pMB = apRef->GetMultiBound();
            pNewNode->SetMultiBound(pMB);
            goto LABEL_15;
        }
        pMBNode = (BSMultiBoundNode*)apRef->GetMultiBoundRoom();
        if (!pMBNode) {
            return nullptr;
        }
    }
    pMB = pMBNode->QMultiBound();
LABEL_15:;
    BSMultiBoundShape* pShape = pMB->QShape();
    if (!pShape) {
        BGSPrimitive* pPrimitive = (&apRef->m_Extra)->GetPrimitive();
        if (pPrimitive) {
            kAngle = apRef->GetAngleOnReference();
            pShape = pPrimitive->MakeMultiBoundShape(kAngle);
            pMB->SetShape(pShape);
        }
    }
    if (pShape) {
        const NiPoint3* pLocation = &apRef->GetLocationOnReference();
        pShape->SetCenter(*pLocation);
        if (pShape->QType() == BSMultiBoundShape::BS_SHAPE_OBB) {
            static_cast<BSMultiBoundOBB*>(pShape)->BSMultiBoundOBB::SetAxis(apRef->GetOrientation());
        }
        pMBNode->SetMultiBound(pMB);
        this->pLoadedData->MultiboundRefMap.SetAt(apRef, NiPointer<BSMultiBoundNode>(pMBNode));
    }
    pMBNode->UpdateWorldBound();
    return pMBNode;
}

// The reference's multibound node: the one the loaded data's map holds, or a fresh one filed there.
BSMultiBoundNode* TESObjectCELL::GetMultiBoundNode(TESObjectREFR* apRef) {
    if (pLoadedData == nullptr || apRef == nullptr || !apRef->IsMultiBoundRef()) {
        return nullptr;
    }
    NiPointer<BSMultiBoundNode> spNode(nullptr);
    if (!pLoadedData->MultiboundRefMap.GetAt(apRef, spNode) || spNode.operator->() == nullptr) {
        spNode = AddMultiBoundRef(apRef);
    }
    return spNode.operator->();
}

void TESObjectCELL::AttachMultiBoundNodes() {
    NiNode* pMultiBoundRoot = static_cast<NiNode*>(GetChildNode(CN_MULTIBOUND));
    if (pMultiBoundRoot == nullptr || pLoadedData == nullptr) {
        return;
    }
    NiTMapItem<TESObjectREFR*, NiPointer<BSMultiBoundNode>>* pPos = pLoadedData->MultiboundRefMap.GetFirstPos();
    while (pPos != nullptr) {
        NiPointer<BSMultiBoundNode> spNode;
        TESObjectREFR* pRef = nullptr;
        pLoadedData->MultiboundRefMap.GetNext(pPos, pRef, spNode);
        if (spNode.m_pObject != nullptr && spNode.m_pObject->GetParent() == nullptr) {
            pMultiBoundRoot->AttachChild(spNode.m_pObject, true);
        }
    }
}

// An interior cell makes its own portal graph; an exterior cell shares its worldspace's.
BSPortalGraph* TESObjectCELL::CreatePortalGraph() {
    if (spPortalGraph.m_pObject == nullptr) {
        if (GetInterior()) {
            void* pMemory = NiMemObject::operator new(sizeof(BSPortalGraph));
            BSPortalGraph* pGraph = pMemory ? ::new (pMemory) BSPortalGraph : nullptr;
            spPortalGraph = pGraph;
        }
        else {
            spPortalGraph = GetWorldSpace()->GetPortalGraph();
        }
    }
    return spPortalGraph.m_pObject;
}

TESForm* TESObjectCELL::GetOwner() {
    TESForm* pOwner = GetExtra()->GetOwner();
    if (pOwner == nullptr) {
        BGSEncounterZone* pZone = GetEncounterZone();
        if (pZone != nullptr && pZone != BGSEncounterZone::GetNoZone()) {
            pOwner = pZone->QOwnerForm();
        }
    }
    return pOwner;
}

// Interior cells have no region list. The new list does not own its regions.
TESRegionList* TESObjectCELL::GetRegionList(bool abCreateIfNotFound) {
    if (GetInterior()) {
        return nullptr;
    }
    TESRegionList* pList = ExtraData.GetRegionList();
    if (pList == nullptr && abCreateIfNotFound) {
        void* pMemory = ourMalloc(sizeof(TESRegionList));
        TESRegionList* pNewList = pMemory ? ::new (pMemory) TESRegionList(false) : nullptr;
        pList = pNewList;
        ExtraData.SetRegionList(pList);
    }
    return pList;
}

// The music type extra, else (exterior only) the worldspace's, which is also kept in a static. apLoc is unused. The
// guarded static position is initialised and never read.
BGSMusicType* TESObjectCELL::GetMusicType(NiPoint3* apLoc) {
    CellRefLockEnter();
    if (!(lastloc_guard & 1)) {
        lastloc_guard |= 1;
        lastloc = NiPoint3_ZERO;
    }
    BGSMusicType* ptypeRegion = nullptr;
    BGSMusicType* ptypeCell = GetExtra()->GetMusicType();
    if (ptypeCell != nullptr || GetInterior()) {
        CellRefLockLeave();
        return ptypeCell;
    }
    if (ptypeRegion == nullptr && !GetInterior()) {
        TESWorldSpace* pworld = GetWorldSpace();
        if (pworld != nullptr) {
            ptypeRegion = pworld->GetMusicType();
            ptypeLast = ptypeRegion;
        }
    }
    CellRefLockLeave();
    return ptypeRegion;
}

// An interior cell has a climate only when it behaves like an exterior.
TESClimate* TESObjectCELL::GetClimate() {
    if (GetInterior()) {
        if (!GetFakeExterior()) {
            return nullptr;
        }
        return GetExtra()->GetClimate();
    }
    return GetWorldSpace()->GetClimate();
}

// A guard never trespasses. In an owned, non-public cell without an ownership global, an NPC trespasses when the
// owner is another NPC base, or a faction in which its rank is below the cell's ownership rank. The NPC cast of
// the actor's base is not checked for null.
bool TESObjectCELL::CheckifTrespassing(Actor* apActor) {
    bool bTrespassing = false;
    if (apActor->IsActoraGuard()) {
        return false;
    }
    TESForm* pOwner = GetOwner();
    if (pOwner == nullptr || GetPublicState()) {
        return bTrespassing;
    }
    if (GetOwnershipGlobal() != nullptr || !apActor->IsNpc()) {
        return bTrespassing;
    }
    TESNPC* pOwnerNPC = dynamic_cast<TESNPC*>(pOwner);
    if (pOwnerNPC != nullptr) {
        if (static_cast<void*>(pOwnerNPC) != static_cast<void*>(apActor->GetObjectReference())) {
            bTrespassing = true;
        }
    }
    else {
        TESFaction* pFaction = dynamic_cast<TESFaction*>(pOwner);
        if (pFaction != nullptr) {
            int32_t iCellRank = GetOwnershipRank();
            TESNPC* pNPC = dynamic_cast<TESNPC*>(apActor->GetObjectReference());
            if (pNPC->GetFactionRank(pFaction, apActor == pPlayer) < iCellRank) {
                bTrespassing = true;
            }
        }
    }
    return bTrespassing;
}

// The default image spaces are the forms 0x160 (interior) and 0x161 (exterior).
TESImageSpace* TESObjectCELL::GetUsableImageSpace() {
    TESImageSpace* pImageSpace = GetImageSpace();
    if (pImageSpace == nullptr && !GetInterior() && GetWorldSpace() != nullptr) {
        pImageSpace = GetWorldSpace()->GetImageSpace();
    }
    if (pImageSpace == nullptr && GetInterior() && !GetFakeExterior()) {
        return dynamic_cast<TESImageSpace*>(TESForm::GetFormByNumericID(0x160));
    }
    if (pImageSpace == nullptr) {
        pImageSpace = dynamic_cast<TESImageSpace*>(TESForm::GetFormByNumericID(0x161));
    }
    return pImageSpace;
}

// A reference's 3D (the player's current 3D) is attached under the first multibound of the cell that bounds it, or
// handed to the room tests; a multibound that only holds its centre queues a CheckWithinMultiBoundTask. Failing those,
// a single portal the 3D touches takes it, several make it always-rendered, and otherwise it goes under the actor or
// static node. Actors, creatures, levelled actors, projectiles and explosions only take part with abForceRoomTest. The
// null NiPointers around DetachAlwaysRenderChild are unused locals.
void TESObjectCELL::TestObjectAgainstMultibounds(TESObjectREFR* apRef, bool abForceRoomTest, bool abMultipleRoomTest) {
    if (apRef == nullptr || apRef->Get3D() == nullptr) {
        return;
    }
    if (apRef->GetObjectReference() == pPortalMarker || apRef->GetObjectReference() == pRoomMarker) {
        return;
    }
    if (apRef->Get3D()->GetParent() == GetDynamicNode()) {
        return;
    }
    bool bfindmultibound = false;
    [[maybe_unused]] bool broomsonly = false;
    if (bUseMultibounds && apRef != nullptr && apRef->GetExtra()->GetMultiBoundRef() == nullptr) {
        switch (apRef->GetObjectReference()->GetFormType()) {
            case NPC__ID:
            case CREA_ID:
            case LVLC_ID:
            case LVLN_ID:
            case PROJ_ID:
            case EXPL_ID: break;
            default: bfindmultibound = true; break;
        }
    }
    if (bfindmultibound || abForceRoomTest) {
        NiPointer<NiAVObject> spref3d(apRef->Get3D());
        if (apRef == pPlayer) {
            NiAVObject* pCurrent3D = pPlayer->GetCurrent3D();
            spref3d = pCurrent3D;
        }
        bool bfoundattach = false;
        bool battached = false;
        bool btest = false;
        alignas(BSScrapMap<TESObjectREFR*, bool>)
            uint8_t aMultiboundsExaminedMapStorage[sizeof(BSScrapMap<TESObjectREFR*, bool>)];
        BSScrapMap<TESObjectREFR*, bool>* pMultiboundsExaminedMap =
            ::new (aMultiboundsExaminedMapStorage) BSScrapMap<TESObjectREFR*, bool>(37, false);
        TESObjectREFR* pcurb = apRef->GetExtra()->GetMultiBoundRef();
        if (pcurb != nullptr && !abMultipleRoomTest) {
            pMultiboundsExaminedMap->SetAt(pcurb, true);
            bfoundattach = TestObjectAgainstRoom(apRef, spref3d.operator->(), pcurb, btest, abMultipleRoomTest);
            if (!bfoundattach) {
                // Every portal of the room is walked, even after a room took the 3D.
                for (BSSimpleList<TESObjectREFR*>* pportalList = pcurb->GetExtra()->GetRoomLinkedPortalRefList();
                     pportalList != nullptr; pportalList = pportalList->GetNext()) {
                    TESObjectREFR* pportalRef = pportalList->GetItem();
                    if (pportalRef == nullptr) {
                        continue;
                    }
                    TESObjectREFR* ptestRef = pportalRef->GetExtra()->GetPortalLinkedRoomRef(0);
                    if (ptestRef != nullptr) {
                        pMultiboundsExaminedMap->SetAt(ptestRef, true);
                        bfoundattach =
                            TestObjectAgainstRoom(apRef, spref3d.operator->(), ptestRef, btest, abMultipleRoomTest);
                    }
                    if (bfoundattach) {
                        continue;
                    }
                    ptestRef = pportalRef->GetExtra()->GetPortalLinkedRoomRef(1);
                    if (ptestRef != nullptr) {
                        bfoundattach =
                            TestObjectAgainstRoom(apRef, spref3d.operator->(), ptestRef, btest, abMultipleRoomTest);
                        pMultiboundsExaminedMap->SetAt(ptestRef, true);
                    }
                }
            }
        }
        NiTMapItem<TESObjectREFR*, NiPointer<BSMultiBoundNode>>* iter = pLoadedData->MultiboundRefMap.GetFirstPos();
        while (iter != nullptr && !bfoundattach) {
            NiPointer<BSMultiBoundNode> spnode(nullptr);
            TESObjectREFR* pboundref = nullptr;
            pLoadedData->MultiboundRefMap.GetNext(iter, pboundref, spnode);
            if (pboundref == nullptr || spnode.operator->() == nullptr ||
                pMultiboundsExaminedMap->GetAt(pboundref, battached)) {
                continue;
            }
            if (!bfindmultibound && abForceRoomTest && spnode.operator->()->GetMultiBoundRoom() == nullptr) {
                continue;
            }
            BSMultiBound* pbound = spnode.operator->()->QMultiBound();
            if (pbound == nullptr) {
                continue;
            }
            if (apRef->QBoundInsideMultibound(pbound) && !abForceRoomTest) {
                NiNode* pboundnode = apRef->GetParentCell()->GetMultiBoundNode(pboundref);
                if (pboundnode != nullptr) {
                    pboundnode->AttachChild(apRef->Get3D(), true);
                    bfoundattach = true;
                    apRef->SetMultiBoundRef(pboundref);
                }
            }
            else if (spnode.operator->()->GetMultiBoundRoom() != nullptr) {
                if (TestObjectAgainstRoom(apRef, spref3d.operator->(), pboundref, btest, abMultipleRoomTest)) {
                    bfoundattach = true;
                    break;
                }
            }
            else if (apRef->QCenterInsideMultibound(pbound)) {
                MultiboundTestMap.SetAt(apRef, pboundref);
                NiAVObject* pref3d = apRef->Get3D();
                void* pMemory = ourMalloc(sizeof(CheckWithinMultiBoundTask));
                CheckWithinMultiBoundTask* ptask = nullptr;
                if (pMemory != nullptr) {
                    ptask = ::new (pMemory) CheckWithinMultiBoundTask(pboundref, pref3d);
                }
                pIOManager->AddTask(ptask);
                BSPortalGraph* pportalgraph = GetPortalGraph();
                NiPointer<NiAVObject> pobject(nullptr);
                pportalgraph->DetachAlwaysRenderChild(apRef->Get3D());
            }
        }
        if (!bfoundattach && !btest) {
            BSPortalGraph* pportalgraph = CreatePortalGraph();
            NiTListItem<BSPortal*>* kportaliter = pportalgraph->QPortalList().GetHeadPos();
            BSPortal* pattachportal = nullptr;
            bool bportalshared = false;
            bool bmultipleportals = false;
            bool badd = false;
            NiAVObject* apRef3D = apRef->Get3D();
            while (kportaliter != nullptr && !bmultipleportals) {
                BSPortal* pportal = pportalgraph->QPortalList().GetNext(kportaliter);
                bportalshared = pportal->TestIntersection(apRef3D->GetWorldBound());
                if (bportalshared) {
                    if (badd) {
                        pattachportal = nullptr;
                        bmultipleportals = true;
                    }
                    else {
                        badd = true;
                        pattachportal = pportal;
                    }
                }
            }
            if (pattachportal != nullptr) {
                NiPointer<NiAVObject> pobject(nullptr);
                pportalgraph->DetachAlwaysRenderChild(apRef3D);
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    QueueAttachChild(apRef, pattachportal->QSharedGeometryNode());
                }
                else {
                    pattachportal->QSharedGeometryNode()->AttachChild(apRef3D, true);
                }
                bfoundattach = true;
            }
            else if (bmultipleportals) {
                // The actor branch is dead: an actor never gets past the first test.
                if (apRef3D->GetParent() != GetDynamicNode() && !apRef->IsActor()) {
                    if (apRef->IsActor()) {
                        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                            QueueAttachChild(apRef, static_cast<NiNode*>(GetActorNode()));
                        }
                        else {
                            static_cast<NiNode*>(GetActorNode())->AttachChild(apRef3D, true);
                        }
                    }
                    else {
                        GetPortalGraph()->AttachAlwaysRenderChild(apRef3D);
                        if (apRef3D->GetParent() != nullptr) {
                            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                                QueueAttachChild(apRef, nullptr);
                            }
                            else {
                                apRef3D->GetParent()->DetachChild(apRef3D);
                            }
                        }
                    }
                }
                bfoundattach = true;
            }
        }
        if (!bfoundattach && !btest) {
            if (apRef->IsActor()) {
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    QueueAttachChild(apRef, static_cast<NiNode*>(GetActorNode()));
                }
                else {
                    static_cast<NiNode*>(GetActorNode())->AttachChild(apRef->Get3D(), true);
                }
            }
            else if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                QueueAttachChild(apRef, static_cast<NiNode*>(GetStaticNode()));
            }
            else {
                static_cast<NiNode*>(GetStaticNode())->AttachChild(apRef->Get3D(), true);
            }
        }
        pMultiboundsExaminedMap->~BSScrapMap<TESObjectREFR*, bool>();
    }
    if (apRef->GetExtra()->GetOcclusionPlane() != nullptr && apRef->GetExtra()->GetMultiBoundRef() == nullptr) {
        GetPortalGraph()->AddOccluder(apRef->GetExtra()->GetOcclusionPlane());
    }
}

bool TESObjectCELL::TestObjectAgainstRoom(
    TESObjectREFR* apRef,
    NiAVObject* apRef3D,
    TESObjectREFR* apBoundRef,
    bool& abFoundRoom,
    bool abMultipleRoomTest
) {
    NiPoint3 kBoundCenter = (&apRef3D->GetWorldBound())->m_kCenter;
    bool bOcclusionPlane = false;
    if ((&apRef->m_Extra)->GetOcclusionPlane()) {
        kBoundCenter = (&apRef->m_Extra)->GetOcclusionPlane()->kCenter;
        bOcclusionPlane = true;
    }
    NiPointer<BSMultiBoundNode> spNode(static_cast<BSMultiBoundNode*>(nullptr));
    pLoadedData->MultiboundRefMap.GetAt(apBoundRef, spNode);
    if (!spNode.m_pObject) {
        goto LABEL_76;
    }
    if (!spNode.m_pObject->GetMultiBoundRoom()) {
        goto LABEL_76;
    }
    if (!abMultipleRoomTest || (&GetPortalGraph()->lPortalList)->IsEmpty()) {
        const NiBound* pWorldBound;
        BSMultiBoundNode* pNodeB;
        BSMultiBoundRoom* pNodeA;
        if (!spNode.m_pObject ||
            ((pNodeA = static_cast<BSMultiBoundRoom*>(spNode.m_pObject), !pNodeA->QPointWithin(kBoundCenter)) &&
             (!abMultipleRoomTest ||
              (pNodeB = spNode.m_pObject,
               (pWorldBound = &apRef3D->GetWorldBound(),
                pNodeB->CheckBound(*pWorldBound) == BSMultiBoundShape::BS_INTERSECT_NONE))))) {
        LABEL_76:;
            return false;
        }
        apRef->SetMultiBoundRef(apBoundRef);
        if (abMultipleRoomTest && abFoundRoom) {
            if (apRef->IsActor()) {
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    NiNode* pActorNode = static_cast<NiNode*>(GetActorNode());
                    TESObjectCELL::QueueAttachChild(apRef, pActorNode);
                }
                else {
                    NiNode* pActorNode = static_cast<NiNode*>(GetActorNode());
                    pActorNode->AttachChild(apRef3D, 1);
                }
            }
            else {
                BSPortalGraph* pPortalGraph = GetPortalGraph();
                pPortalGraph->AttachAlwaysRenderChild(apRef3D);
                if (apRef3D->m_pkParent) {
                    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                        TESObjectCELL::QueueAttachChild(apRef, nullptr);
                    }
                    else {
                        NiNode* pParent = apRef3D->m_pkParent;
                        pParent->DetachChild(apRef3D);
                    }
                }
            }
            return true;
        }

        BSMultiBoundNode* pNode = spNode.m_pObject;
        BSMultiBoundRoom* pRoom = pNode->GetMultiBoundRoom();
        if (bOcclusionPlane) {
            BSOcclusionPlane* pOcclusionPlane = (&apRef->m_Extra)->GetOcclusionPlane();
            pRoom->AddOccluder(pOcclusionPlane);
            return true;
        }

        bool bMultiplePortals = false;
        bool bPortalShared = false;
        BSPortal* pSharedPortal = nullptr;
        if ((&pRoom->QPortalList())->m_uiCount) {
            NiFrustumPlanes kBoxPlanes;
            [[maybe_unused]] NiTransform kWorldTransform = apRef3D->GetWorldTransform();
            bool bUseFrustum = false;
            NiTListItem<BSPortal*>* pPos = (&pRoom->QPortalList())->m_pkHead;
            while (pPos) {
                BSPortal* pPortal = (&pRoom->QPortalList())->GetNext(pPos);
                if (pPortal) {
                    bool bAdd = false;
                    if (bUseFrustum) {
                        bAdd = pPortal->WithinFrustum(kBoxPlanes, false, NiPoint3_ZERO);
                    }
                    else {
                        const NiBound* pBound = &apRef3D->GetWorldBound();
                        bAdd = pPortal->TestIntersection(*pBound);
                    }
                    if (bAdd) {
                        if (bPortalShared) {
                            bMultiplePortals = true;
                            NiNode* pParent = apRef3D->m_pkParent;
                            if (pParent != GetDynamicNode() && !apRef->IsActor()) {
                                if (apRef->IsActor()) {
                                    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                                        NiNode* pActorNode = static_cast<NiNode*>(GetActorNode());
                                        TESObjectCELL::QueueAttachChild(apRef, pActorNode);
                                    }
                                    else {
                                        NiNode* pActorNode = static_cast<NiNode*>(GetActorNode());
                                        pActorNode->AttachChild(apRef3D, 1);
                                    }
                                }
                                else {
                                    BSPortalGraph* pPortalGraph = GetPortalGraph();
                                    pPortalGraph->AttachAlwaysRenderChild(apRef3D);
                                    if (apRef3D->m_pkParent) {
                                        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                                            TESObjectCELL::QueueAttachChild(apRef, nullptr);
                                        }
                                        else {
                                            NiNode* pOldParent = apRef3D->m_pkParent;
                                            pOldParent->DetachChild(apRef3D);
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        pSharedPortal = pPortal;
                        bPortalShared = true;
                    }
                }
            }
            if (bPortalShared && !bMultiplePortals) {
                BSPortalSharedNode* pParent = static_cast<BSPortalSharedNode*>(apRef->Get3D()->m_pkParent);
                if (pParent != pSharedPortal->BSPortal::QSharedGeometryNode()) {
                    BSPortalGraph* pPortalGraph = GetPortalGraph();
                    NiPointer<NiAVObject> spObject(static_cast<NiAVObject*>(nullptr));
                    pPortalGraph->DetachAlwaysRenderChild(apRef3D);
                    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                        BSPortalSharedNode* pShared = pSharedPortal->BSPortal::QSharedGeometryNode();
                        TESObjectCELL::QueueAttachChild(apRef, pShared);
                    }
                    else {
                        BSPortalSharedNode* pShared = pSharedPortal->BSPortal::QSharedGeometryNode();
                        pShared->AttachChild(apRef3D, 1);
                    }
                }
            }
        }
        if (!bPortalShared) {
            BSMultiBoundNode* pParent = static_cast<BSMultiBoundNode*>(apRef3D->m_pkParent);
            if (pParent != spNode.m_pObject) {
                BSPortalGraph* pPortalGraph = GetPortalGraph();
                NiPointer<NiAVObject> spObject(static_cast<NiAVObject*>(nullptr));
                pPortalGraph->DetachAlwaysRenderChild(apRef3D);
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    TESObjectCELL::QueueAttachChild(apRef, spNode.m_pObject);
                }
                else {
                    BSMultiBoundNode* pRoomNode = spNode.m_pObject;
                    pRoomNode->AttachChild(apRef3D, 1);
                }
            }
        }
        abFoundRoom = true;
        return !(abMultipleRoomTest && !bMultiplePortals);
    }

    if (apRef->IsActor()) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            NiNode* pActorNode = static_cast<NiNode*>(GetActorNode());
            TESObjectCELL::QueueAttachChild(apRef, pActorNode);
        }
        else {
            NiNode* pActorNode = static_cast<NiNode*>(GetActorNode());
            pActorNode->AttachChild(apRef3D, 1);
        }
    }
    else {
        BSPortalGraph* pPortalGraph = GetPortalGraph();
        pPortalGraph->AttachAlwaysRenderChild(apRef3D);
        if (apRef3D->m_pkParent) {
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                TESObjectCELL::QueueAttachChild(apRef, nullptr);
            }
            else {
                NiNode* pParent = apRef3D->m_pkParent;
                pParent->DetachChild(apRef3D);
            }
        }
    }
    abFoundRoom = true;
    return true;
}

void TESObjectCELL::AddDecal(
    DECAL_CREATION_DATA& arDecalData,
    BGSDecalManager::DECAL_TYPE aeDecalType,
    bool abForceAdd
) {
    if (!(&arDecalData)->pParentCell) {
        (&arDecalData)->pParentCell = this;
    }
    BGSDecalManager* singleton = BGSDecalManager::GetSingleton();
    singleton->AddDecal(arDecalData, aeDecalType, abForceAdd);
}

void TESObjectCELL::AddAudioMarkersToList(BSSimpleList<TESObjectREFR*>* apAudioList) {
    if (apAudioList == nullptr) {
        return;
    }
    CellRefLockEnter();
    for (BSSimpleList<TESObjectREFR*>* pNode = GetRefList(); pNode != nullptr; pNode = pNode->GetNext()) {
        TESObjectREFR* pRef = pNode->GetItem();
        if (pRef != nullptr && pRef->GetObjectReference() == static_cast<TESForm*>(pAudioMarker) &&
            pRef->GetExtraAudioMarker() != nullptr) {
            apAudioList->AddHead(pRef);
        }
    }
    CellRefLockLeave();
}

// Nothing happens while the data is cleared or a save loads.
void TESObjectCELL::AttachSounds(bool abAttach) {
    if (pDataHandler->GetClearingData() || pSaveLoadGame->GetSaveGameLoading()) {
        return;
    }
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* pNode = GetRefList();
    while (pNode != nullptr) {
        TESObjectREFR* pRef = pNode->GetItem();
        pNode = pNode->GetNext();
        if (pRef != nullptr && pRef->CanHaveSound() && (!abAttach || pRef->ShouldPlaySound())) {
            pRef->AttachSound(abAttach);
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::AttachLights(bool abAttach) {
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* pNode = GetRefList();
    while (pNode != nullptr) {
        TESObjectREFR* pRef = pNode->GetItem();
        pNode = pNode->GetNext();
        if (pRef != nullptr) {
            if (abAttach) {
                pRef->AddLight(false);
            }
            else {
                pRef->RemoveLight(false);
            }
        }
    }
    CellRefLockLeave();
}

// Loads the cell's 3D, runs its scripts and recalculates its leveled actors. A buffered cell only updates its root and
// references; otherwise the reference list is walked twice, multibound markers (room and bound markers) in the first
// pass and every other reference in the second, and each enabled, undeleted reference is moved to high process, queued
// for its model and attached when it already has 3D (automatic placeable water never is). The pass counter advances as
// soon as a pass reads its last node, so that node's reference is filtered by the next pass, deliberately, as the game
// does. Both save-game getters always return false.
void TESObjectCELL::AttachModels(bool abSetupMopp) {
    NiNode* pRootNode = Load3D();
    RunScripts(false, false);
    CalcLeveledCreatures(HandleExpiredReferences());
    if (TES::pInstance->IsBuffered(this)) {
        if (pRootNode) {
            NiUpdateData kUpdateData;
            kUpdateData.~NiUpdateData();
            ::new (&kUpdateData) NiUpdateData(0.0f, false, false);
            pRootNode->Update(kUpdateData);
        }
        UpdateReferences3DandProcess();
    }
    else {
        CellRefLockEnter();
        IO_TASK_PRIORITY ePriority = TES::pInstance->GetCellPriority(this, nullptr);
        BSSimpleList<TESObjectREFR*>* pIter = GetRefList();
        int32_t iPass = 0;
        while (iPass < 2 && pIter) {
            TESObjectREFR* pRef = pIter->GetItem();
            pIter = pIter->GetNext();
            if (!pIter && ++iPass < 2) {
                pIter = GetRefList();
            }
            if (!pRef) {
                continue;
            }
            bool bMultiBound = pRef->GetObjectReference() &&
                (pRef->GetObjectReference() == pRoomMarker || pRef->GetObjectReference() == pBoundMarker);
            if (iPass == 0 && !bMultiBound) {
                continue;
            }
            if (iPass == 1 && bMultiBound) {
                continue;
            }
            if (!pSaveLoadGame->GetSaveGameLoading()) {
                Script::InitActionList(pRef, pRef->GetExtra());
                Script::SetActionFlag(pRef, pRef->GetExtra(), SCRIPT_ACTION_ONLOAD);
            }
            if (pRef->GetDisabled() || pRef->GetDelete()) {
                continue;
            }
            if (pRef->IsActor() && (!pSaveLoadGameOLD->GetLoading() || pSaveLoadGameOLD->GetLoading())) {
                Actor* pActor = dynamic_cast<Actor*>(pRef);
                if (pActor) {
                    pActor->MovetoHigh();
                    ReferenceProcessLists.RemoveActorFromTempChangeList(pActor);
                }
            }
            else if (pRef->IsProjectile()) {
                static_cast<MobileObject*>(pRef)->ChangeProcessLevel();
            }
            pModelLoader->QueueReference(pRef, ePriority, false);
            if (pRef->Get3D()) {
                bool bAttach = true;
                if (pRef->GetObjectReference()->GetFormType() == PWAT_ID) {
                    bAttach = !static_cast<BGSPlaceableWater*>(pRef->GetObjectReference())->IsAutomaticWater();
                }
                if (bAttach) {
                    AttachReference3D(pRef, false);
                }
            }
        }
        CellRefLockLeave();
    }
    if (abSetupMopp) {
        SetupMopp();
    }
    SetState(CS_ATTACHED);
}

void TESObjectCELL::UpdateReferences3DandProcess() {
    ChangeProcessLevel(false);
    CellRefLockEnter();
    for (TESObjectREFR* pRef : *GetRefList()) {
        if (pRef) {
            pRef->UpdateReference3D();
            if ((pRef->GetObjectReference()->TESForm::GetHasSpokenFlag()) && (pRef->Get3D()) && (GetWorldSpace())) {
                TESWorldSpace* WorldSpace = GetWorldSpace();
                BGSTerrainManager* TerrainManager = WorldSpace->GetTerrainManager();
                TerrainManager->HideTree(pRef, true);
                TESWorldSpace* v3 = GetWorldSpace();
                BGSTerrainManager* v4 = v3->GetTerrainManager();
                v4->SetNeedsImmediateUpdate(true);
            }
        }
    }
    CellRefLockLeave();
}

// The centre of the bounding box of the cell's references, lights, ammo and actors left out. A reference that lies
// beyond 30000 units, or more than the Havok world size (HK2NI(3250)) from the running centre, goes back to its
// starting position, or to the COC placement when that is still bad, and is then left out too. A NaN distance counts as
// inside the Havok world: the test is "HK2NI < distance".
void TESObjectCELL::CalcRefCenterPoint(NiPoint3& aOutCenter) {
    aOutCenter = NiPoint3_ZERO;
    CellRefLockEnter();
    int32_t irefcount = 0;
    NiPoint3 minLocation(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f);
    NiPoint3 maxLocation(1.1754944e-38f, 1.1754944e-38f, 1.1754944e-38f);
    for (BSSimpleList<TESObjectREFR*>* plist = GetRefList(); plist != nullptr; plist = plist->GetNext()) {
        TESObjectREFR* pref = plist->GetItem();
        if (pref == nullptr || pref->GetObjectReference() == nullptr) {
            continue;
        }
        bool bprocess = true;
        int32_t iFormType = pref->GetObjectReference()->GetFormType();
        if (iFormType == LIGH_ID || (iFormType > AMMO_ID && iFormType <= LVLN_ID)) {
            bprocess = false;
        }
        else {
            bool boutsideBroadphase = false;
            if (irefcount != 0) {
                NiPoint3 tempCenter = (minLocation + maxLocation) / 2.0f;
                NiPoint3 diff;
                diff = tempCenter - pref->GetLocationOnReference();
                bool bOutside = true;
                float fX = std::fabs(diff.x);
                if (!(HK2NI(3250.0f) < fX)) {
                    float fY = std::fabs(diff.y);
                    if (!(HK2NI(3250.0f) < fY)) {
                        float fZ = std::fabs(diff.z);
                        if (!(HK2NI(3250.0f) < fZ)) {
                            bOutside = false;
                        }
                    }
                }
                boutsideBroadphase = bOutside;
            }
            if (std::fabs(pref->GetLocationOnReference().x) > 30000.0 ||
                std::fabs(pref->GetLocationOnReference().y) > 30000.0 || boutsideBroadphase) {
                bool bbad = true;
                if (pref->GetExtra()->GetExtraData(EXTRA_STARTINGPOSITION)) {
                    uint32_t uiFormID = pref->GetFormID();
                    const char* pEditorID = pref->GetFormEditorID();
                    Warning(
                        "Ref '%s' (%08X) is at invalid position. Attempting to return to start pos.", pEditorID,
                        uiFormID
                    );
                    NiPoint3 kStart = pref->GetStartingLocation();
                    pref->SetLocationOnReference(kStart);
                    if (boutsideBroadphase) {
                        NiPoint3 tempCenter = (minLocation + maxLocation) / 2.0f;
                        NiPoint3 diff;
                        diff = tempCenter - pref->GetLocationOnReference();
                        bool bOutside = true;
                        float fX = std::fabs(diff.x);
                        if (!(HK2NI(3250.0f) < fX)) {
                            float fY = std::fabs(diff.y);
                            if (!(HK2NI(3250.0f) < fY)) {
                                float fZ = std::fabs(diff.z);
                                if (!(HK2NI(3250.0f) < fZ)) {
                                    bOutside = false;
                                }
                            }
                        }
                        boutsideBroadphase = bOutside;
                    }
                    if (std::fabs(pref->GetLocationOnReference().x) < 30000.0 &&
                        std::fabs(pref->GetLocationOnReference().y) < 30000.0 && !boutsideBroadphase) {
                        bbad = false;
                    }
                }
                if (bbad) {
                    uint32_t uiFormID = pref->GetFormID();
                    const char* pEditorID = pref->GetFormEditorID();
                    Warning(
                        "Ref '%s' (%08X) is at invalid position. Attempting to move to safe place.", pEditorID, uiFormID
                    );
                    NiPoint3 pos;
                    NiPoint3 rot;
                    GetCOCPlacementInfo(pos, rot);
                    pref->SetLocationOnReference(pos);
                    bprocess = false;
                }
            }
        }
        if (bprocess) {
            minLocation.x = BSMIN(minLocation.x, pref->GetLocationOnReference().x);
            minLocation.y = BSMIN(minLocation.y, pref->GetLocationOnReference().y);
            minLocation.z = BSMIN(minLocation.z, pref->GetLocationOnReference().z);
            maxLocation.x = BSMAX(maxLocation.x, pref->GetLocationOnReference().x);
            maxLocation.y = BSMAX(maxLocation.y, pref->GetLocationOnReference().y);
            maxLocation.z = BSMAX(maxLocation.z, pref->GetLocationOnReference().z);
            ++irefcount;
        }
    }
    if (irefcount > 0) {
        NiPoint3 extents = maxLocation - minLocation;
        if (extents.x > HK2NI(3250.0f) || extents.y > HK2NI(3250.0f) || extents.z > HK2NI(3250.0f)) {
            Warning(
                "HAVOK ERROR: Interior is too large for the havok world! World must be expanded or cell must be "
                "made smaller. Cell: '%s' (%08X)\nCell Extents:\t x:%f y:%f z:%f\nWorld Size:\t\t %f",
                GetFormEditorID(), GetFormID(), static_cast<double>(extents.x), static_cast<double>(extents.y),
                static_cast<double>(extents.z), static_cast<double>(HK2NI(3250.0f))
            );
        }
        aOutCenter = (minLocation + maxLocation) / 2.0f;
    }
    CellRefLockLeave();
}

void TESObjectCELL::AddFurnitureToList(BSSimpleList<TESObjectREFR*>* apFurnList) {
    if (apFurnList == nullptr) {
        return;
    }
    CellRefLockEnter();
    for (BSSimpleList<TESObjectREFR*>* pNode = GetRefList(); pNode != nullptr; pNode = pNode->GetNext()) {
        TESObjectREFR* pRef = pNode->GetItem();
        if (pRef != nullptr && pRef->IsFurniture() && !pRef->GetDelete() && !pRef->GetDisabled()) {
            apFurnList->AddHead(pRef);
        }
    }
    CellRefLockLeave();
}

// The centre is (X * 4096 + 2048, Y * 4096 + 2048, 0), each coordinate rounded to a float; Y is read first.
void TESObjectCELL::GetCOCPlacementInfo(NiPoint3& aOutPos, NiPoint3& aOutRot) {
    float fY = static_cast<float>((GetDataY() << 12) + 0x800);
    float fX = static_cast<float>((GetDataX() << 12) + 0x800);
    aOutPos = NiPoint3(fX, fY, 0.0f);
    aOutRot = NiPoint3_ZERO;
    if (bDefaultCOCPlacement.Bool() && !GetInterior()) {
        return;
    }
    LoadAllTempData();
    DetermineCOCPlacement(aOutPos, aOutRot);
}

// Picks where a center-on-cell lands: the first COC heading marker, else XMarker heading (both with their rotation),
// else XMarker, else the arrival spot of the first door leading here, else the first static, else the last reference;
// in an exterior each must lie inside the cell. An exterior placement is then lifted onto the land or the water,
// whichever is higher.
void TESObjectCELL::DetermineCOCPlacement(NiPoint3& aOutPos, NiPoint3& aOutRot) {
    CellRefLockEnter();
    TESObjectREFR* pXref = nullptr;
    TESObjectREFR* pXHeadref = nullptr;
    TESObjectREFR* pCOCref = nullptr;
    TESObjectREFR* pSTATref = nullptr;
    TESObjectREFR* plastref = nullptr;
    bool bfoundteleport = false;
    for (BSSimpleList<TESObjectREFR*>* plist = GetRefList(); plist && !plist->IsEmpty(); plist = plist->GetNext()) {
        TESObjectREFR* ptestref = plist->GetItem();
        if (ptestref->GetDelete()) {
            continue;
        }
        if ((!pCOCref && ptestref->GetObjectReference() == pCOCMarkerHeading) &&
            (GetInterior() || IsPointInCellExterior(ptestref->GetLocationOnReference()))) {
            pCOCref = ptestref;
        }

        if ((!pXref && ptestref->GetObjectReference() == pXMarker) &&
            (GetInterior() || IsPointInCellExterior(ptestref->GetLocationOnReference()))) {
            pXref = ptestref;
        }

        if ((!pXHeadref && ptestref->GetObjectReference() == pXMarkerHeading) &&
            (GetInterior() || IsPointInCellExterior(ptestref->GetLocationOnReference()))) {
            pXHeadref = ptestref;
        }

        if (!bfoundteleport) {
            DoorTeleportData* ptel = ptestref->GetTeleport();
            if (ptel && ptel->GetLinkedDoor()) {
                ptel = ptel->GetLinkedDoor()->GetTeleport();
                if (ptel) {
                    NiPoint3 telpos = ptel->GetTeleportPosition();
                    if (GetInterior() || IsPointInCellExterior(telpos)) {
                        aOutPos = telpos;
                        bfoundteleport = true;
                    }
                }
            }
        }
        if ((!pSTATref && ptestref->GetObjectReference()->GetFormType() == STAT_ID) &&
            (GetInterior() || IsPointInCellExterior(ptestref->GetLocationOnReference()))) {
            pSTATref = ptestref;
        }

        if (GetInterior() || IsPointInCellExterior(ptestref->GetLocationOnReference())) {
            plastref = ptestref;
        }
    }
    CellRefLockLeave();
    if (pCOCref) {
        aOutPos = pCOCref->GetLocationOnReference();
        aOutRot = pCOCref->GetAngleOnReference();
    }
    else if (pXHeadref) {
        aOutPos = pXHeadref->GetLocationOnReference();
        aOutRot = pXHeadref->GetAngleOnReference();
    }
    else if (pXref) {
        aOutPos = pXref->GetLocationOnReference();
    }
    else if (!bfoundteleport) {
        if (pSTATref) {
            aOutPos = pSTATref->GetLocationOnReference();
        }
        else if (plastref) {
            aOutPos = plastref->GetLocationOnReference();
        }
    }
    if (!GetInterior()) {
        TESObjectLAND* pland = GetLand();
        if (pland) {
            float flandheight = 0.0f;
            pland->GetLandHeight(aOutPos, flandheight);
            float fwaterheight = GetWaterHeight();
            if (fwaterheight > flandheight) {
                flandheight = fwaterheight;
            }
            if (aOutPos.z < flandheight) {
                aOutPos.z = flandheight;
            }
        }
    }
}

uint32_t TESObjectCELL::GetInteriorOffset() {
    uint32_t iOffset = 0;
    if (GetInterior()) {
        INTERIOR_DATA* pData = GetInteriorData();
        if (pData != nullptr) {
            iOffset = pData->iInteriorOffset;
        }
    }
    return iOffset;
}

// Resets the cell's references once it has been detached long enough. A cell never detached (detach time 0) only
// resets the inventories of persistent containers and of persistent actors due a reset. Otherwise (-1 forces it) a
// reset deletes non-quest references created at run time, resurrects respawning actors at their editor spot, removes
// dead non-respawning ones with their ash piles, and resets containers, flora and deleted food ingredients whose
// encounter zone resets, flagging each for its OnReset block. Returns whether the cell's zone reset.
bool TESObjectCELL::HandleExpiredReferences() {
    if (pSaveLoadGame->GetSaveGameLoading()) {
        return false;
    }
    uint32_t uiHoursPassed = appCalendar.GetHoursPassed();
    bool bShouldReset = false;
    bool bDeleteCreatedRefs = false;
    int32_t iDetachTime = GetDetachTime();
    if (iDetachTime == 0) {
        CellRefLockEnter();
        BSSimpleList<TESObjectREFR*>* pNode = GetRefList();
        while (pNode != nullptr && !pNode->IsEmpty()) {
            TESObjectREFR* pRef = pNode->GetItem();
            pNode = pNode->GetNext();
            uint8_t eType = pRef->GetObjectReference()->GetFormType();
            if (eType == CONT_ID) {
                if (pRef->GetRefPersists()) {
                    pRef->ResetInventory(!pRef->GetRespawn());
                }
            }
            else if (
                eType > AMMO_ID && eType <= CREA_ID && pRef->GetRefPersists() &&
                static_cast<Actor*>(pRef)->ShouldReset(uiHoursPassed)
            ) {
                pRef->ResetInventory(!pRef->GetRespawn());
            }
        }
        CellRefLockLeave();
        return true;
    }
    bool bCanRespawn = false;
    BGSEncounterZone* pCellZone = GetEncounterZone();
    if (iDetachTime == -1) {
        bShouldReset = true;
        bDeleteCreatedRefs = true;
        bCanRespawn = true;
    }
    else {
        // The compare is unsigned.
        if (uiHoursPassed - iDetachTime > GetHoursToRespawnCell()) {
            bCanRespawn = true;
        }
        if (pCellZone != nullptr) {
            if (pCellZone->ShouldReset()) {
                bShouldReset = pCellZone->ShouldCellRecalc(this);
            }
            else {
                bShouldReset = false;
            }
        }
        else {
            bShouldReset = bCanRespawn;
        }
    }
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* pNode = GetRefList();
    while (pNode != nullptr && !pNode->IsEmpty()) {
        TESObjectREFR* pRef = pNode->GetItem();
        pNode = pNode->GetNext();
        if (bDeleteCreatedRefs && pDataHandler->IsFormIDCreated(pRef->GetFormID())) {
            if (!pRef->IsQuestObject() && !pRef->HasQuestObject()) {
                pRef->SetDelete(true);
            }
            continue;
        }
        Actor* pActor = nullptr;
        if (pRef->IsActor()) {
            pActor = static_cast<Actor*>(pRef);
        }
        if (pActor != nullptr) {
            if (!pActor->ShouldReset(uiHoursPassed)) {
                continue;
            }
        }
        else {
            if (bCanRespawn && pRef->GetDestructible()) {
                BGSDestructibleObjectForm* pDestructible =
                    BGSDestructibleObjectForm::GetDestructionForm(pRef->GetObjectReference());
                pDestructible->ApplyTerminalDamage(pRef);
                if (pRef->GetExtra()->GetModelSwap() != nullptr) {
                    pRef->SetResetDestruct(true);
                }
            }
            BGSEncounterZone* pRefZone = pRef->GetEncounterZone();
            if (pRefZone == pCellZone) {
                if (!bShouldReset) {
                    continue;
                }
            }
            else if (pRefZone != nullptr && !pRefZone->ShouldReset()) {
                continue;
            }
        }
        if (pActor != nullptr) {
            if (pActor->GetRespawn()) {
                pActor->Resurrect(true, false, false);
                BSSimpleList<TESObjectREFR*>* pDroppedList = pActor->GetExtra()->GetDroppedItemList();
                if (pDroppedList != nullptr) {
                    while (pDroppedList != nullptr && !pDroppedList->IsEmpty()) {
                        TESObjectREFR* pDropped = pDroppedList->GetItem();
                        pDropped->GetExtra()->SetItemDropper(nullptr);
                        if (!pDropped->IsQuestObject()) {
                            pDropped->MarkAsDeleted();
                        }
                        pDroppedList->RemoveHead();
                    }
                    pActor->GetExtra()->RemoveDroppedItemList();
                }
                NiPoint3 kStartingPos = pActor->GetStartingLocation();
                bool bResetPosition = false;
                if (pActor->GetEditorLocationInteriorCell() == this) {
                    bResetPosition = true;
                }
                else if (pActor->GetEditorLocationWorld() == GetWorldSpace() && IsPointInCellExterior(kStartingPos)) {
                    bResetPosition = true;
                }
                if (bResetPosition) {
                    pActor->SetLocationOnReference(kStartingPos);
                    pActor->SetAngleOnReference(pActor->GetStartingAngle());
                }
            }
            else if (pActor->IsDead(false)) {
                if (!pActor->IsQuestObject() && !pActor->HasQuestObject()) {
                    pActor->MarkAsDeleted();
                    TESObjectREFR* pAshPileRef = pActor->GetExtra()->GetAshPileRef();
                    if (pAshPileRef != nullptr) {
                        pAshPileRef->MarkAsDeleted();
                    }
                }
                BSSimpleList<TESObjectREFR*>* pDroppedList = pActor->GetExtra()->GetDroppedItemList();
                if (pDroppedList != nullptr) {
                    while (pDroppedList != nullptr && !pDroppedList->IsEmpty()) {
                        TESObjectREFR* pDropped = pDroppedList->GetItem();
                        pDropped->GetExtra()->SetItemDropper(nullptr);
                        if (!pDropped->IsQuestObject()) {
                            pDropped->MarkAsDeleted();
                        }
                        pDroppedList->RemoveHead();
                    }
                    pActor->GetExtra()->RemoveDroppedItemList();
                }
            }
        }
        else {
            switch (pRef->GetObjectReference()->GetFormType()) {
                case CONT_ID:
                    if (pRef->GetRespawn()) {
                        pRef->ResetInventory(false);
                        if (pRef->GetExtra()->GetContainerChanges() != nullptr &&
                            !pRef->GetExtra()->GetContainerChanges()->IsEmpty()) {
                            pRef->Lock();
                        }
                    }
                    break;
                case INGR_ID:
                    if (pRef->GetDelete() && !pRef->GetPersistFlagValue() && !pRef->IsQuestObject()) {
                        IngredientItem* pIngredient = dynamic_cast<IngredientItem*>(pRef->GetObjectReference());
                        // IsFood is called through the EffectItemList subobject.
                        if (pIngredient != nullptr && static_cast<EffectItemList*>(pIngredient)->IsFood()) {
                            pRef->SetDelete(false);
                        }
                    }
                    break;
                case FLOR_ID: pRef->SetEmpty(false); break;
            }
        }
        Script::InitActionList(pRef, pRef->GetExtra());
        // the OnReset event flag
        Script::SetActionFlag(pRef, pRef->GetExtra(), SCRIPT_ACTION_ONRESET);
    }
    CellRefLockLeave();
    SetDetachTime(0, false);
    return bShouldReset;
}

// Loads the cell's temporary references from every file that defines it, seeking each file either to the cell's stored
// temp-data offset (only for files without masters) or to the cell's record. A file that is missing or not a master
// fails the load only for an exterior cell that is not past loading. On success the cell is marked as having its temp
// data and is reinitialised. The save-game getter always returns false and the setter does nothing.
bool TESObjectCELL::LoadAllTempData() {
    if (GetHasTempData()) {
        return true;
    }
    bool bLoaded = true;
    // The index compare is unsigned.
    uint32_t uiFileCount = GetFileCount();
    for (uint32_t i = 0; i < uiFileCount; ++i) {
        TESFile* pFile = GetFile(i)->GetThreadSafeFile();
        if (pFile && pFile->GetMaster()) {
            bool bOffsetSet = false;
            uint32_t uiOffset = pFile->GetMasterList()->IsEmpty() ? GetTempDataOffset() : 0;
            if (uiOffset) {
                if (pFile->SetOffset(uiOffset)) {
                    bOffsetSet = true;
                }
            }
            else if (pFile->FindForm(this)) {
                bOffsetSet = true;
            }
            if (!bOffsetSet) {
                bLoaded = false;
            }
            else if (!LoadTempDataFromFile(pFile)) {
                bLoaded = false;
            }
        }
        else if (static_cast<int32_t>(GetState()) <= CS_LOADING && !GetInterior()) {
            // The state compare is signed.
            bLoaded = false;
        }
    }
    if (!bLoaded) {
        return bLoaded;
    }
    SetHasTempData(true);
    bool bOldLoading = pSaveLoadGameOLD->GetLoading();
    pSaveLoadGameOLD->SetLoading(!bOldLoading);
    InitItem();
    pSaveLoadGameOLD->SetLoading(bOldLoading);
    return bLoaded;
}

// Loads the cell's temporary references from a master file positioned on the cell's record (or on its children
// group): steps past the cell children group (6) and the persistent group (8) into the temporary group (9), then
// constructs each child record whose form is not loaded, changed in the save or a temporary ID of a plugin. A group
// header is a FORM whose iFormID holds the group type and flags the group's label (the cell's form ID).
bool TESObjectCELL::LoadTempDataFromFile(TESFile* apFile) {
    if (apFile == nullptr) {
        return false;
    }
    if (!apFile->GetMaster()) {
        return true;
    }
    FORM* pCurrentForm = apFile->GetFormData();
    if (pCurrentForm->form == formEnumString[CELL_ID].uiFormString) {
        if (apFile->GetID() != GetFormID()) {
            return false;
        }
        apFile->NextForm(true);
        pCurrentForm = apFile->GetFormData();
        if (!IsFormCellChild(pCurrentForm)) {
            return true;
        }
        if (pCurrentForm->form != formEnumString[GRUP_ID].uiFormString || pCurrentForm->iFormID != 6) {
            return false;
        }
        apFile->NextForm(true);
        pCurrentForm = apFile->GetFormData();
        if (pCurrentForm->form != formEnumString[GRUP_ID].uiFormString) {
            return false;
        }
        if (pCurrentForm->iFormID == 8) {
            apFile->NextGroup();
            pCurrentForm = apFile->GetFormData();
            if (!IsFormCellChild(pCurrentForm)) {
                return true;
            }
        }
    }
    if (pCurrentForm->form != formEnumString[GRUP_ID].uiFormString) {
        return false;
    }
    if (pCurrentForm->iFormID != 9) {
        return false;
    }
    if (apFile->NextForm(true)) {
        pCurrentForm = apFile->GetFormData();
    }
    else {
        pCurrentForm = nullptr;
    }
    pLastLoadedCell = this;
    pDataHandler->SetSaveLoad(true);
    if (pCurrentForm != nullptr && pCurrentForm->form == formEnumString[GRUP_ID].uiFormString &&
        pCurrentForm->iFormID == 9) {
        if (apFile->NextForm(true)) {
            pCurrentForm = apFile->GetFormData();
        }
        else {
            pCurrentForm = nullptr;
        }
    }
    SetLoadingTempData(true);
    bool bOldAllowChanges = pSaveLoadGame->SetThreadAllowChanges(false);
    bool bLoaded = true;
    while (pCurrentForm != nullptr && IsFormTypeCellChild(TESForm::GetFormTypeFromFormString(pCurrentForm->form))) {
        uint32_t uiFormID = apFile->GetID();
        if (TESFile::GetTempIDCanLoadFromFile(uiFormID, apFile)) {
            TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
            bool bChangedCells = false;
            if (!pSaveLoadGame->GetSaveGameLoading()) {
                bChangedCells = pSaveLoadGame->HasReferenceChangedCells(uiFormID);
            }
            bool bTempID = false;
            if ((apFile->GetMaster()) &&
                (!apFile->GetMasterList()->IsEmpty() && (apFile->GetFormData()->flags & 0x20) != 0)) {
                bTempID = true;
            }

            if (pForm == nullptr && !bChangedCells && !bTempID &&
                !pDataHandler->ConstructObject(apFile, false, pForm, false)) {
                bLoaded = false;
            }
        }
        if (apFile->NextForm(true)) {
            pCurrentForm = apFile->GetFormData();
        }
        else {
            pCurrentForm = nullptr;
        }
        if (pCurrentForm != nullptr && pCurrentForm->form == formEnumString[GRUP_ID].uiFormString &&
            pCurrentForm->iFormID == 9 && pCurrentForm->flags == GetFormID()) {
            if (apFile->NextForm(true)) {
                pCurrentForm = apFile->GetFormData();
            }
            else {
                pCurrentForm = nullptr;
            }
        }
    }
    SetLoadingTempData(false);
    pSaveLoadGame->SetThreadAllowChanges(bOldAllowChanges);
    pLastLoadedCell = nullptr;
    pDataHandler->SetSaveLoad(false);
    return bLoaded;
}

// Swaps each live leveled actor whose reset time has passed back to its original leveled base, deleting the
// generated base, then recalculates the template of any actor whose base uses a leveled template. abAllowRecalc is
// unused.
void TESObjectCELL::CalcLeveledCreatures([[maybe_unused]] bool abAllowRecalc) {
    CellRefLockEnter();
    uint32_t icurrenttime = appCalendar.GetHoursPassed();
    BSSimpleList<TESObjectREFR*>* plist = GetRefList();
    while (plist && !plist->IsEmpty()) {
        TESObjectREFR* pref = plist->GetItem();
        plist = plist->GetNext();
        if (!pref->GetObjectReference()->IsActorBase() || pref->GetDelete() || pref->IsDead(false)) {
            continue;
        }
        Actor* pactor = static_cast<Actor*>(pref);
        TESActorBase* pbase = static_cast<TESActorBase*>(pref->GetObjectReference());
        TESActorBase* porigbase = static_cast<TESActorBase*>(pref->GetExtra()->GetLevCreaOriginalBase());
        if (porigbase && porigbase != pbase) {
            if (!pactor->ShouldReset(icurrenttime)) {
                continue;
            }
            pref->Set3D(nullptr, false);
            pref->SetObjectReference(porigbase);
            pref->SetLeveledCreature(nullptr, nullptr);
            delete pbase;
            pbase = porigbase;
        }
        if (pbase->GetUsesLeveledTemplate()) {
            pbase->CalcTemplateForRef(pref);
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::UpdateManagedNodes(float afTime, bool abLargeOnly) {
    Sky::COLOR_BLEND colorblend_2;
    Sky::COLOR_BLEND colorblend;
    if (GetInterior() && GetAttached()) {
        bhkWorld* pw = GetbhkWorld();
        if (pw) {
            pw->Update();
        }
    }
    bool bparallel = AILinearTaskThreadManager::ShouldQueue3DTask();
    NiNode* pdynamic = static_cast<NiNode*>(GetDynamicNode());
    if ((pdynamic) && (!abLargeOnly)) {
        NiUpdateData UpdateData(afTime, false, bparallel);
        pdynamic->UpdateSelectedDownwardPass(UpdateData, 0);
        if (iNumHWThreads.Int() > 1) {
            TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
            Instance->QueueCellTransformChildTask(pdynamic);
            if (bparallel && UpdateData.bFoundParticles) {
                TESObjectCELL::QueueParticleUpdate(pdynamic);
            }
            if (bparallel && UpdateData.bFoundMorphController) {
                TaskQueueInterface* v4 = TaskQueueInterface::GetInstance();
                v4->QueueMorphControllerUpdate(pdynamic);
            }
        }
    }

    if (sNumLoadedRefsWithVisibleDistant > 0 && All3DWithVisibleDistantLoaded() && ShouldDisplayHighDetail()) {
        FadeToHighDetail();
    }
    if ((bUpdateTerrain) && (GetWorldSpace()) && (GetState() == TESObjectCELL::CS_ATTACHED)) {
        bDisplayHighDetail = true;
        bFadingToHighDetail = false;
        bFadingToLowDetail = false;
        if (GetWorldSpace()) {
            TESWorldSpace* WorldSpace = GetWorldSpace();
            WorldSpace->GetTerrainManager()->SetNeedsImmediateUpdate(true);
        }
        bUpdateTerrain = false;
    }

    if (bFadingToHighDetail) {
        if (fLodFadeInPercent >= TESObjectCELL::fLodFadeThreshold * TESObjectCELL::fLodFadeTime) {
            if ((GetWorldSpace()) && (!bDisplayHighDetail)) {
                TESWorldSpace* v6 = GetWorldSpace();
                v6->GetTerrainManager()->SetNeedsImmediateUpdate(true);
            }

            bDisplayHighDetail = true;
            bFadingToHighDetail = false;
            bFadingToLowDetail = false;
        }
        fLodFadeInPercent = appTimer.fDelta + fLodFadeInPercent;
    }
    if (pLoadedData) {
        if (abLargeOnly) {
            BSSimpleList<TESObjectREFR*>* plist = &pLoadedData->LargeAnimatedRefs;
            while (plist && !plist->IsEmpty()) {
                TESObjectREFR* pref = plist->m_item;
                NiNode* pnode = pref->Get3D();
                plist = plist->m_pkNext;
                if (pnode) {
                    NiUpdateData UpdateData_2(afTime, false, bparallel);
                    pnode->UpdateSelectedDownwardPass(UpdateData_2, 0);
                    if (UpdateData_2.bSceneGraphChange) {
                        NiNode* v17 = pnode;
                        ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
                        ShadowSceneNode_->UpdateObjectLighting(v17, true);
                    }
                    if (iNumHWThreads.Int() > 1) {
                        NiNode* addFlash_1b = pnode;
                        TaskQueueInterface* v8 = TaskQueueInterface::GetInstance();
                        v8->QueueCellTransformChildTask(addFlash_1b);
                        if (bparallel && UpdateData_2.bFoundParticles) {
                            TESObjectCELL::QueueParticleUpdate(pnode);
                        }
                        if (bparallel && UpdateData_2.bFoundMorphController) {
                            NiNode* addFlash_1c = pnode;
                            TaskQueueInterface* v9 = TaskQueueInterface::GetInstance();
                            v9->QueueMorphControllerUpdate(addFlash_1c);
                        }
                    }
                }
            }
        }
        else {
            NiTMapItem<TESObjectREFR*, NiNode*>* pAnimatedPos = pLoadedData->AnimatedRefMap.GetFirstPos();
            while (pAnimatedPos) {
                TESObjectREFR* pref_2 = nullptr;
                NiNode* pnode_2 = nullptr;
                pLoadedData->AnimatedRefMap.GetNext(pAnimatedPos, pref_2, pnode_2);
                if (pnode_2) {
                    NiNode* Parent = pnode_2->m_pkParent;
                    if (Parent != pdynamic) {
                        NiCamera* Camera;
                        if (pref_2->GetObjectReference()->GetFormType() != TREE_ID ||
                            (Camera = BSTreeManager::GetCamera(), NodeInFrustum(pnode_2, Camera))) {
                            NiUpdateData UpdateData_3(afTime, false, bparallel);
                            pnode_2->UpdateSelectedDownwardPass(UpdateData_3, 0);
                            if (UpdateData_3.bSceneGraphChange) {
                                NiNode* v18 = pnode_2;
                                ShadowSceneNode* v12 = BSShaderManager::GetShadowSceneNode(0);
                                v12->UpdateObjectLighting(v18, true);
                            }
                            if (iNumHWThreads.Int() > 1) {
                                NiNode* addFlash_1d = pnode_2;
                                TaskQueueInterface* v13 = TaskQueueInterface::GetInstance();
                                v13->QueueCellTransformChildTask(addFlash_1d);
                                if (bparallel && UpdateData_3.bFoundParticles) {
                                    TESObjectCELL::QueueParticleUpdate(pnode_2);
                                }
                                if (bparallel && UpdateData_3.bFoundMorphController) {
                                    NiNode* addFlash_1e = pnode_2;
                                    TaskQueueInterface* v14 = TaskQueueInterface::GetInstance();
                                    v14->QueueMorphControllerUpdate(addFlash_1e);
                                }
                            }
                        }
                    }
                }
            }
            NiTMapItem<TESForm*, TESObjectREFR*>* pSourcePos = pLoadedData->EmittanceSourceRefMap.GetFirstPos();
            while (pSourcePos) {
                TESForm* psource = nullptr;
                TESObjectREFR* pref_3 = nullptr;
                pLoadedData->EmittanceSourceRefMap.GetNext(pSourcePos, psource, pref_3);
                if (psource) {
                    NiColor* emitcolor = psource->GetEmittanceColor();
                    int32_t eTypeID = psource->GetFormType();
                    if (eTypeID == LIGH_ID) {
                        uint32_t ilightColor = static_cast<TESObjectLIGH*>(psource)->GetColor();
                        float fscale = 0.003921569f;
                        emitcolor->r = static_cast<double>(static_cast<uint8_t>(ilightColor)) * 0.003921569f;
                        emitcolor->g = static_cast<double>(static_cast<uint8_t>(ilightColor >> 8)) * fscale;
                        emitcolor->b = static_cast<double>(static_cast<uint8_t>(ilightColor >> 16)) * fscale;
                    }
                    else {
                        if (eTypeID == REGN_ID) {
                            TESWeather* pweather = static_cast<TESRegion*>(psource)->GetCurrentWeather();
                            if (!pweather) {
                                TESForm* FormByNumericID = TESForm::GetFormByNumericID(350u);
                                pweather = dynamic_cast<TESWeather*>(FormByNumericID);
                            }
                            if (pweather) {
                                TESWeather::ColorTime aeTime2;
                                TESWeather::ColorTime aeTime1;
                                Sky* psky = Sky::GetInstance();
                                psky->FillColorBlend(colorblend, pweather, 1.0f, aeTime1, aeTime2);
                                psky->FillColorBlendColors(
                                    colorblend, pweather, nullptr, TESWeather::SUNLIGHT, aeTime1, aeTime2
                                );
                                double v27 = psky->GetFlash();
                                float fflash = fWeatherFlashDirectional.Float() * v27;
                                psky->SetColor(*emitcolor, &colorblend, fflash);
                            }
                        }
                    }
                }
            }
            NiTMapItem<TESObjectREFR*, NiNode*>* pLightPos = pLoadedData->EmittanceLightRefMap.GetFirstPos();
            while (pLightPos) {
                TESObjectREFR* pref_4 = nullptr;
                NiNode* pnode_3 = nullptr;
                pLoadedData->EmittanceLightRefMap.GetNext(pLightPos, pref_4, pnode_3);
                if (pnode_3) {
                    NiColor emitcolor_2 = NiColor_WHITE;
                    TESForm* psource_2 = pref_4->GetEmittanceSource();
                    if (!psource_2) {
                        psource_2 = (TESForm*)pPlayer->GetOccupiedRegion();
                    }
                    if (psource_2) {
                        int32_t FormType = psource_2->GetFormType();
                        if (FormType == LIGH_ID) {
                            uint32_t ilightColor_2 = static_cast<TESObjectLIGH*>(psource_2)->GetColor();
                            float fscale_2 = 0.003921569f;
                            emitcolor_2.r = static_cast<double>(static_cast<uint8_t>(ilightColor_2)) * fscale_2;
                            emitcolor_2.g = static_cast<double>(static_cast<uint8_t>(ilightColor_2 >> 8)) * fscale_2;
                            emitcolor_2.b = static_cast<double>(static_cast<uint8_t>(ilightColor_2 >> 16)) * fscale_2;
                        }
                        else {
                            if (FormType == REGN_ID) {
                                TESWeather* pweather_2 = static_cast<TESRegion*>(psource_2)->GetCurrentWeather();
                                if (!pweather_2) {
                                    TESForm* v16 = TESForm::GetFormByNumericID(350u);
                                    pweather_2 = dynamic_cast<TESWeather*>(v16);
                                }
                                if (pweather_2) {
                                    TESWeather::ColorTime aeTime2_2;
                                    TESWeather::ColorTime aeTime1_2;
                                    Sky* psky_2 = Sky::GetInstance();
                                    psky_2->FillColorBlend(colorblend_2, pweather_2, 1.0f, aeTime1_2, aeTime2_2);
                                    psky_2->FillColorBlendColors(
                                        colorblend_2, pweather_2, nullptr, TESWeather::SUNLIGHT, aeTime1_2, aeTime2_2
                                    );
                                    double flash = psky_2->GetFlash();
                                    float fflash_2 = fWeatherFlashDirectional.Float() * flash;
                                    psky_2->SetColor(emitcolor_2, &colorblend_2, fflash_2);
                                }
                            }
                        }
                    }
                    if (pref_4->GetObjectReference()->GetFormType() == LIGH_ID) {
                        uint32_t lightColor = static_cast<TESObjectLIGH*>(pref_4->GetObjectReference())->GetColor();
                        float fscale_3 = 0.003921569f;
                        emitcolor_2.r =
                            static_cast<double>(static_cast<uint8_t>(lightColor)) * fscale_3 * emitcolor_2.r;
                        emitcolor_2.g =
                            static_cast<double>(static_cast<uint8_t>(lightColor >> 8)) * fscale_3 * emitcolor_2.g;
                        emitcolor_2.b =
                            static_cast<double>(static_cast<uint8_t>(lightColor >> 16)) * fscale_3 * emitcolor_2.b;
                        LightRefData* pthelight = pref_4->GetLight();
                        if ((pthelight) && (pthelight->spLight.m_pObject)) {
                            pthelight->spLight.m_pObject->SetDiffuseColor(emitcolor_2);
                        }
                    }
                }
            }
        }
    }
}

BGSAcousticSpace* TESObjectCELL::GetAcousticSpace() {
    ExtraDataList* extraDataList = GetExtra();
    return extraDataList->GetAcousticSpace();
}

bool TESObjectCELL::IsPointInCellExterior(const NiPoint3& aPt) {
    if (GetInterior()) {
        return false;
    }
    int32_t v4 = FloatToInt(aPt.x);
    int32_t v5 = FloatToInt(aPt.y);
    return (bool)((uint8_t)(((v4 >> 12) == GetDataX()) && ((v5 >> 12) == GetDataY())));
}

// Loads the cell's 3D and hangs it under apRoot, then brings the cell's world online: Havok (the exterior's MOPP, the
// collision and visual-debugger switches), lights, addon nodes, sounds, the debug navmeshes, the loaded reference
// collection and the obstacles, and finally the portal graph: the shadow scene node takes it, a graph with no
// always-rendered children gets the dynamic, actor and shadow-scene child 5 nodes, the portal node takes the graph's
// root, and the static, actor, land and marker nodes become unbound nodes.
void TESObjectCELL::AttachToWorld(NiNode* apRoot) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_CELLS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 14685
    );
    NiNode* pnode = Load3D();
    SetState(CS_ATTACHING);
    apRoot->AttachChild(pnode, true);
    bhkWorld* pworld = GetbhkWorld();
    if (pworld) {
        if (!GetInterior()) {
            SetupMopp();
        }
        pworld->SetEnabled(!TES::QCollisionDisableFlag());
        pworld->SetVisualDebugger(bhkWorld::IsHavokDebug());
    }
    AttachLights(true);
    ProcessAddonNodesForRefs(true);
    AttachSounds(true);
    if (NavMeshRender::GetInstance()->GetDraw()) {
        NavMeshRender::GetInstance()->AddDrawOnlyNavMeshes(this, true);
    }
    AddReferencesToLoadedReferenceCollection();
    AddObstaclesAndDoors();
    NavMeshObstacleManager::GetInstance()->QueueUpdateAllObstacle();
    BSPortalGraph* pgraph = GetPortalGraph();
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->SetPortalGraph(pgraph);
    if (!pgraph->GetAlwaysRenderArrayCount()) {
        pgraph->AttachAlwaysRenderChild(GetDynamicNode());
        pgraph->AttachAlwaysRenderChild(GetActorNode());
        pgraph->AttachAlwaysRenderChild(
            BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->GetAtChecked(5)
        );
    }
    // GetChildNode and the unnamed node getters return NiAVObject*, so their results are cast to NiNode*.
    NiNode* pportalnode = static_cast<NiNode*>(GetChildNode(CN_PORTAL));
    pportalnode->AttachChild(pgraph->GetRootNode(), true);
    pgraph->AddUnboundNode(static_cast<NiNode*>(GetStaticNode()));
    pgraph->AddUnboundNode(static_cast<NiNode*>(GetActorNode()));
    pgraph->AddUnboundNode(static_cast<NiNode*>(GetChildNode(CN_LAND)));
    pgraph->AddUnboundNode(static_cast<NiNode*>(GetMarkerNode()));
    bCellDetached = false;
    pContext->~AutoMemContext();
}

// Builds an interior cell's havok world when it has none (the cinfo with the game's gravity, then the listeners, the
// origin at the references' centre, a first step and the enable flags), then the cell's MOPP when it has none.
void TESObjectCELL::InitHavok() {
    bhkWorld* pw = GetbhkWorld();
    if (this->GetInterior() && !pw) {
        NiPoint3 org;
        TESArrowTargetListener* v16;
        BGSAcousticSpaceListener* v22;
        TESTrapListener* v26;
        bhkWorld* pw_2;
        AutoMemContext tempMemContext(
            MEM_CONTEXT_HAVOK, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 14911
        );
        bhkWorldCinfo hkwInfo;
        hkwInfo.m_gravity = hkVector4(0.0f, 0.0f, HavokWorldProps::fGravitySC, 0.0f);
        hkwInfo.setBroadPhaseWorldSize(3250.0f);
        hkwInfo.m_collisionTolerance = 1.0f;
        bhkWorld* v30 = static_cast<bhkWorld*>(bhkRagdollController::operator new(sizeof(bhkWorld)));
        if (v30) {
            pw_2 = ::new (v30) bhkWorld(hkwInfo);
        }
        else {
            pw_2 = nullptr;
        }
        pw = pw_2;
        pw_2->MarkForWrite();
        TESTrapListener* v27 = static_cast<TESTrapListener*>(ourMalloc(sizeof(TESTrapListener)));
        if (v27) {
            v26 = ::new (v27) TESTrapListener;
        }
        else {
            v26 = nullptr;
        }
        pw->AddTrapListener(v26);
        BGSAcousticSpaceListener* v23 =
            static_cast<BGSAcousticSpaceListener*>(ourMalloc(sizeof(BGSAcousticSpaceListener)));
        if (v23) {
            v22 = ::new (v23) BGSAcousticSpaceListener;
        }
        else {
            v22 = nullptr;
        }
        pw->AddAcousticSpaceListener(v22);
        if (this->HasWater()) {
            TESWaterListener* v19;
            TESWaterListener* v20 = static_cast<TESWaterListener*>(ourMalloc(sizeof(TESWaterListener)));
            if (v20) {
                float a5 = this->GetWaterHeight();
                v19 = ::new (v20) TESWaterListener(a5);
            }
            else {
                v19 = nullptr;
            }
            pw->AddWaterListener(v19);
        }
        pw->AddContactListener(FOCollisionListener::Instance());
        TESArrowTargetListener* v17 = static_cast<TESArrowTargetListener*>(ourMalloc(sizeof(TESArrowTargetListener)));
        if (v17) {
            v16 = ::new (v17) TESArrowTargetListener;
        }
        else {
            v16 = nullptr;
        }
        pw->AddEntityListener(v16);
        if (!TESObjectCELL::pIslandListener) {
            FOIslandActivationListener* v13;
            FOIslandActivationListener* v14 =
                static_cast<FOIslandActivationListener*>(ourMalloc(sizeof(FOIslandActivationListener)));
            if (v14) {
                v13 = ::new (v14) FOIslandActivationListener;
            }
            else {
                v13 = nullptr;
            }
            TESObjectCELL::pIslandListener = v13;
        }
        ahkpWorld* v3 = pw->operator*();
        TESObjectCELL::pIslandListener->AddToWorld(v3);
        this->CalcRefCenterPoint(org);
        pw->SetOrigin(org);
        bool IsHavokDebug = bhkWorld::IsHavokDebug();
        pw->SetVisualDebugger(IsHavokDebug);
        this->ExtraData.SetbhkWorld(pw);
        pw->MarkForWrite();
        pw->Update();
        bool v5 = TES::QCollisionDisableFlag();
        pw->SetEnabled(!v5);
    }
    CellMopp* pcm = this->GetCellMopp();
    if (!pcm) {
        CellMopp* pcm_2;
        AutoMemContext tempMemContext_2(
            MEM_CONTEXT_HAVOK, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 14993
        );
        CellMopp* v8 = static_cast<CellMopp*>(NiMemObject::operator new(sizeof(CellMopp)));
        if (v8) {
            pcm_2 = ::new (v8) CellMopp;
        }
        else {
            pcm_2 = nullptr;
        }
        pcm = pcm_2;
        this->ExtraData.SetCellMopp(pcm_2);
    }
}

// Builds the cell's MOPP collision into its Havok world, centred on the cell (4096 units a side). AutoMemContext as
// in AttachToWorld.
void TESObjectCELL::SetupMopp() {
    bhkWorld* pWorld = GetbhkWorld();
    int32_t iX = GetDataX();
    int32_t iY = GetDataY();
    CellMopp* pCellMopp = GetCellMopp();
    if (pWorld && pCellMopp) {
        alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
        AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
        ::new (aucContext) AutoMemContext(
            MEM_CONTEXT_HAVOK, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 15014
        );
        NiTransform kTransform;
        kTransform.~NiTransform();
        ::new (&kTransform) NiTransform;
        kTransform.m_fScale = 1.0f;
        kTransform.m_Rotate = NiMatrix3_IDENTITY;
        kTransform.m_Translate.x = static_cast<float>((iX << 12) + 2048.0);
        kTransform.m_Translate.y = static_cast<float>((iY << 12) + 2048.0);
        kTransform.m_Translate.z = 0.0f;
        pCellMopp->Make(pWorld, kTransform.m_Translate, NiQuaternion_IDENTITY);
        pContext->~AutoMemContext();
    }
}

void TESObjectCELL::RunAnimations() {
    if (!GetAttached() || pLoadedData == nullptr) {
        return;
    }
    NiTMapItem<TESObjectREFR*, NiNode*>* pPos = pLoadedData->AnimatedRefMap.GetFirstPos();
    while (pPos != nullptr) {
        TESObjectREFR* pRef = nullptr;
        NiNode* pRoot = nullptr;
        pLoadedData->AnimatedRefMap.GetNext(pPos, pRef, pRoot);
        if (!pRef->IsActor() && pRef->GetObjectReference() != nullptr &&
            pRef->GetObjectReference()->GetFormType() != TREE_ID) {
            pRef->UpdateAnimation();
        }
    }
}

float TESObjectCELL::GetNorthRotation() {
    float fRotation = 0.0f;
    if (GetInterior()) {
        fRotation = GetExtra()->GetNorthRotation();
    }
    return fRotation;
}

void TESObjectCELL::AddReferencesToLoadedReferenceCollection() {
    CellRefLockEnter();
    for (BSSimpleList<TESObjectREFR*>* pNode = GetRefList(); pNode != nullptr; pNode = pNode->GetNext()) {
        TESObjectREFR* pRef = pNode->GetItem();
        if (pRef != nullptr) {
            pLoadedReferenceCollection->AddReference(pRef);
        }
    }
    CellRefLockLeave();
}

// Registers every obstacle reference with the obstacle manager; open hinged doors are added as obstacles and closed
// doors go through OnDoorClose.
void TESObjectCELL::AddObstaclesAndDoors() {
    CellRefLockEnter();
    for (BSSimpleList<TESObjectREFR*>* pNode = GetRefList(); pNode != nullptr; pNode = pNode->GetNext()) {
        TESObjectREFR* pRef = pNode->GetItem();
        if (pRef == nullptr) {
            continue;
        }
        if (pRef->IsObstacle()) {
            NavMeshObstacleManager::GetInstance()->AddObstacleForReference(pRef);
        }
        if (pRef->GetObjectReference()->GetFormType() == DOOR_ID) {
            if (BGSOpenCloseForm::GetOpenState(pRef) == BGSOpenCloseForm::OBJECT_OPEN) {
                if (!static_cast<TESObjectDOOR*>(pRef->GetObjectReference())->IsSlidingDoor()) {
                    NavMeshObstacleManager::GetInstance()->AddObstacleForReference(pRef);
                }
            }
            else {
                NavMeshObstacleManager::GetInstance()->OnDoorClose(pRef);
            }
        }
    }
    CellRefLockLeave();
}

// Only references whose loaded data has flag 2 set and that have 3D take part.
void TESObjectCELL::ProcessAddonNodesForRefs(bool abAttach) {
    CellRefLockEnter();
    if (abAttach) {
        BSSimpleList<TESObjectREFR*>* pNode = GetRefList();
        while (pNode != nullptr) {
            TESObjectREFR* pRef = pNode->GetItem();
            pNode = pNode->GetNext();
            if (pRef != nullptr && pRef->GetLoadedDataFlag(2) && pRef->Get3D() != nullptr) {
                TESObjectREFR::AddMasterParticleAddonNodes(pRef->Get3D()->IsNode());
            }
        }
    }
    else {
        BSSimpleList<TESObjectREFR*>* pNode = GetRefList();
        while (pNode != nullptr) {
            TESObjectREFR* pRef = pNode->GetItem();
            pNode = pNode->GetNext();
            if (pRef != nullptr && pRef->GetLoadedDataFlag(2) && pRef->Get3D() != nullptr) {
                TESObjectREFR::RemoveMasterParticleAddonNodes(pRef->Get3D()->IsNode());
            }
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::ActivateAllSubmergedReferencesIslands() {
    for (BSSimpleList<TESObjectREFR*>* pNode = GetRefList(); pNode != nullptr && pNode->GetItem() != nullptr;
         pNode = pNode->GetNext()) {
        TESObjectREFR* pRef = pNode->GetItem();
        if (!pRef->IsActor() && pRef->GetObjectReference()->GetFormType() != DOOR_ID && pRef->GetLODTree() &&
            Get3D() != nullptr) {
            bhkWorld::Activate(Get3D(), true, true, true);
        }
    }
}

void TESObjectCELL::RemoveScriptedRef(TESObjectREFR* apRef) {
    if (pLoadedData) {
        (&pLoadedData->ScriptedRefs)->Remove(apRef);
    }
}

void TESObjectCELL::RemoveActivatingRef(TESObjectREFR* apRef) {
    if (pLoadedData) {
        pLoadedData->ActivatingRefs.Remove(apRef);
    }
    apRef->RemoveChange(CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN);
}

void TESObjectCELL::RemoveAnimatedRef(TESObjectREFR* apRef) {
    if (pLoadedData) {
        pLoadedData->AnimatedRefMap.RemoveAt(apRef);
        pLoadedData->LargeAnimatedRefs.Remove(apRef);
    }
}

void TESObjectCELL::RemoveEmittanceRef(TESObjectREFR* apRef) {
    if (pLoadedData) {
        if (apRef->GetObjectReference()->GetFormType() == LIGH_ID) {
            pLoadedData->EmittanceLightRefMap.RemoveAt(apRef);
        }
        else {
            TESForm* psource = apRef->GetEmittanceSource();
            if (psource) {
                pLoadedData->EmittanceSourceRefMap.RemoveAt(psource);
            }
        }
    }
}

void TESObjectCELL::AddScriptedRef(TESObjectREFR* apRef) {
    if (pLoadedData) {
        pLoadedData->ScriptedRefs.AddHead(apRef);
    }
}

void TESObjectCELL::AddActivatingRef(TESObjectREFR* apRef) {
    if (pLoadedData) {
        BSSimpleList<TESObjectREFR*>* plist = &pLoadedData->ActivatingRefs;
        BSSimpleList<TESObjectREFR*>* plastlist = nullptr;
        while (plist && !plist->IsEmpty()) {
            TESObjectREFR** v2 = &plist->m_item;
            if (*v2 == apRef) {
                return;
            }
            plastlist = plist;
            plist = plist->m_pkNext;
        }
        apRef->AddChange(CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN);
        if (plastlist) {
            plastlist->AddTail(apRef);
        }
        else {
            pLoadedData->ActivatingRefs.AddHead(apRef);
        }
    }
}

// The cell's root node, or null while no 3D is loaded.
NiNode* TESObjectCELL::Get3D() const {
    if (pLoadedData != nullptr) {
        return pLoadedData->spCell3D.m_pObject;
    }
    return nullptr;
}

// Returns a NiNode; NiNode::GetAt yields a NiAVObject, so the pointer is cast.
NiNode* TESObjectCELL::GetChildNode(CELLNODE aeIndex) const {
    NiNode* pCell3D = Get3D();
    if (pCell3D != nullptr) {
        return static_cast<NiNode*>(pCell3D->GetAt(aeIndex));
    }
    return nullptr;
}

NiNode* TESObjectCELL::GetMarkerNode() const {
    return GetChildNode(CN_MARKER);
}

// Calls GetChildNode with no null test of its own.
NiAVObject* TESObjectCELL::GetDynamicNode() const {
    return GetChildNode(CN_DYNAMIC);
}

NiAVObject* TESObjectCELL::GetActorNode() const {
    return GetChildNode(CN_ACTOR);
}

NiAVObject* TESObjectCELL::GetStaticNode() const {
    return GetChildNode(CN_STATIC);
}

NiNode* TESObjectCELL::LightMarkerNode() const {
    return spLightMarkerNode.m_pObject;
}

NiNode* TESObjectCELL::SoundMarkerNode() {
    return this->spSoundMarkerNode.operator->();
}

BSPortalGraph* TESObjectCELL::GetPortalGraph() const {
    return spPortalGraph.m_pObject;
}

TESObjectREFR* TESObjectCELL::GetFirstRefr() {
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* objectList = GetRefList();
    TESObjectREFR* v4 = objectList->m_item;
    CellRefLockLeave();
    return v4;
}

bool TESObjectCELL::GetPersistentCell() {
    return (GetFormFlags() & TESForm::kFormFlags_IsRefPersistent) != 0;
}

bool TESObjectCELL::GetInterior() const {
    return (cCellFlags & kCellFlag_Interior) != 0;
}

int32_t TESObjectCELL::GetCanopyShadowMask(NiRenderedTexture*& apOutTex, _D3DLOCKED_RECT*& apOutRect) {
    if (GetInterior()) {
        return 0;
    }

    return GetExtra()->GetCanopyShadowMask(apOutTex, apOutRect);
}

NiAVObject* TESObjectCELL::Pick(bhkPickData& arResult) {
    CFilter* CollisionFilter;
    CFilter a2;
    bhkWorld* pw = GetbhkWorld();
    if (!pw || !pw->PickObject(arResult)) {
        return nullptr;
    }
    hkpRigidBody* pbody = arResult.GethkRigidBody();
    bhkRigidBody* pbhkBody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pbody));
    if (pbhkBody && (CollisionFilter = pbhkBody->GetCollisionFilter(&a2), CollisionFilter->GetLayer() == L_GROUND)) {
        return Get3D();
    }

    return arResult.GetNiAVObject();
}

bool TESObjectCELL::HasWater() const {
    return (cCellFlags & kCellFlag_HasWater) != 0;
}

bool TESObjectCELL::GetPublic() const {
    return (cCellFlags & kCellFlag_Public) != 0;
}

bool TESObjectCELL::GetTempPublic() const {
    return (cCellFlags & kCellFlag_TempPublic) != 0;
}

bool TESObjectCELL::GetPublicState() {
    return GetPublic() || GetTempPublic();
}

bool TESObjectCELL::GetFakeExterior() const {
    return (cCellFlags & kCellFlag_BehavesLikeExterior) != 0;
}

bool TESObjectCELL::GetHasTempData() const {
    return (cCellFlags & kCellFlag_HasTempData) != 0;
}

void TESObjectCELL::SetHasTempData(bool abVal) {
    if (abVal) {
        cCellFlags |= kCellFlag_HasTempData;
    }
    else {
        cCellFlags &= static_cast<uint8_t>(~kCellFlag_HasTempData);
    }
}

LoadedCellData* TESObjectCELL::GetLoadedData() {
    return pLoadedData;
}

// Changing the flag also marks the cell as changed (change flag 2).
void TESObjectCELL::SetTempPublic(bool abVal) {
    if (abVal) {
        cCellFlags |= kCellFlag_TempPublic;
    }
    else {
        cCellFlags &= static_cast<uint8_t>(~kCellFlag_TempPublic);
    }
    AddChange(2);
}

TESGlobal* TESObjectCELL::GetOwnershipGlobal() {
    return GetExtra()->GetGlobal();
}

// The ownership rank, or 0 when the cell has none.
int32_t TESObjectCELL::GetOwnershipRank() {
    int32_t iRank = GetExtra()->GetRank();
    if (iRank == -1) {
        return 0;
    }
    return iRank;
}

uint32_t TESObjectCELL::GetDetachTime() {
    return GetExtra()->GetDetachTime();
}

void TESObjectCELL::SetDetachTime(uint32_t aiTime, bool abForce) {
    if (!pSaveLoadGame->GetSaveGameLoading() || (!aiTime || abForce)) {
        GetExtra()->SetDetachTime(aiTime);
    }
    if (aiTime) {
        AddChange(CHANGE_CELL_DETACHTIME);
        if (!GetInterior()) {
            int32_t x = GetDataX();
            int32_t y = GetDataY();
            if (x > 127 || (x < -128 || (y > 127 || y < -128))) {
                AddChange(CHANGE_CELL_EXTERIOR_SHORT);
            }
            else {
                AddChange(CHANGE_CELL_EXTERIOR_CHAR);
            }
        }
    }
    else {
        RemoveChange(CHANGE_CELL_DETACHTIME | CHANGE_CELL_EXTERIOR_CHAR | CHANGE_CELL_EXTERIOR_SHORT);
    }
}

void TESObjectCELL::ClearSeenData() {
    GetExtra()->SetSeenData(nullptr);
    cCellGameFlags &= ~1;
    RemoveChange(CHANGE_CELL_SEENDATA);
}

// The change flag 2 (the cell's flags) has no name in CHANGE_TYPE.
void TESObjectCELL::SetFullySeen(bool abVal) {
    if (abVal) {
        cCellGameFlags |= 1;
    }
    else {
        cCellGameFlags &= ~1;
    }
    AddChange(2);
}

// Steps to the next node before testing the item and does not test the node for emptiness, as the game does.
uint32_t TESObjectCELL::GetLightCount() {
    CellRefLockEnter();
    uint32_t iCount = 0;
    BSSimpleList<TESObjectREFR*>* pIter = GetRefList();
    while (pIter) {
        TESObjectREFR* pRef = pIter->GetItem();
        pIter = pIter->GetNext();
        if (pRef && pRef->GetObjectReference()->GetFormType() == LIGH_ID) {
            ++iCount;
        }
    }
    CellRefLockLeave();
    return iCount;
}

TESObjectREFR* TESObjectCELL::GetLootBag() {
    TESObjectREFR* pLootBag = nullptr;
    CellRefLockEnter();
    for (BSSimpleList<TESObjectREFR*>* pIter = GetRefList(); pIter; pIter = pIter->GetNext()) {
        TESObjectREFR* pRef = pIter->GetItem();
        if (pRef && pRef->GetObjectReference() == pDefLootBag) {
            pLootBag = pRef;
            break;
        }
    }
    CellRefLockLeave();
    return pLootBag;
}

TESImageSpace* TESObjectCELL::GetImageSpace() {
    return GetExtra()->GetImageSpace();
}

// Only interior cells carry an impact swap of their own.
ImpactSwap* TESObjectCELL::GetImpactSwap() {
    if (GetInterior()) {
        return GetExtra()->GetImpactSwap();
    }
    return nullptr;
}

// The cell's own encounter zone, or its worldspace's.
BGSEncounterZone* TESObjectCELL::GetEncounterZone() {
    BGSEncounterZone* pZone = GetExtra()->GetEncounterZone();
    if (pZone != nullptr) {
        return pZone;
    }
    // GetWorldSpace is called again for the zone rather than keeping the first result, as the game does.
    if (GetWorldSpace() != nullptr) {
        return GetWorldSpace()->GetEncounterZone();
    }
    return nullptr;
}

CellMopp* TESObjectCELL::GetCellMopp() {
    return ExtraData.GetCellMopp();
}

bool TESObjectCELL::GetLowestProcessMiddleLow() {
    return ExtraData.GetExtraData(EXTRA_CELL_MID_LOW_PROCESS) != nullptr;
}

const char* TESObjectCELL::GetWaterNoiseTextureName() const {
    return WaterNoiseTexture.GetTextureName();
}

BGSLightingTemplate* TESObjectCELL::GetLightingTemplate() const {
    return pLightingTemplate;
}

void TESObjectCELL::SetLightingTemplate(BGSLightingTemplate* apTemplate) {
    pLightingTemplate = apTemplate;
}

bool TESObjectCELL::GetInheritFlag(uint32_t aeFlag) const {
    return (iLightingTemplateInheritanceFlags & aeFlag) != 0;
}

EXTERIOR_DATA* TESObjectCELL::GetExteriorData() const {
    if (GetInterior()) {
        return nullptr;
    }
    return static_cast<EXTERIOR_DATA*>(pCellData);
}

INTERIOR_DATA* TESObjectCELL::GetInteriorData() const {
    if (!GetInterior()) {
        return nullptr;
    }
    return static_cast<INTERIOR_DATA*>(pCellData);
}

int32_t TESObjectCELL::GetDataX() const {
    EXTERIOR_DATA* pData = GetExteriorData();
    if (pData != nullptr) {
        return pData->iCellX;
    }
    return 0;
}

int32_t TESObjectCELL::GetDataY() const {
    EXTERIOR_DATA* pData = GetExteriorData();
    if (pData != nullptr) {
        return pData->iCellY;
    }
    return 0;
}

void TESObjectCELL::SetInteriorOffset(uint32_t aiOffset) {
    if (GetInterior()) {
        INTERIOR_DATA* pData = GetInteriorData();
        if (pData != nullptr) {
            pData->iInteriorOffset = aiOffset;
        }
    }
}

// Interior cells have no worldspace.
TESWorldSpace* TESObjectCELL::GetWorldSpace() const {
    if (GetInterior()) {
        return nullptr;
    }
    return pWorldSpace;
}

void TESObjectCELL::SetWorldSpace(TESWorldSpace* apWorldSpace) {
    if (!GetInterior()) {
        pWorldSpace = apWorldSpace;
    }
}

uint32_t TESObjectCELL::GetTempDataOffset() const {
    if (GetInterior()) {
        return iTempDataOffset;
    }
    return 0;
}

void TESObjectCELL::SetTempDataOffset(uint32_t aiOffset) {
    if (GetInterior()) {
        pWorldSpace = (TESWorldSpace*)aiOffset;
    }
}

void TESObjectCELL::SetNavMeshArray(NavMeshArray* apNavMeshArray) {
    if (pNavMeshes != nullptr) {
        delete pNavMeshes;
    }
    pNavMeshes = apNavMeshArray;
}

NavMeshArray* TESObjectCELL::GetNavMeshArray() {
    return pNavMeshes;
}

TESObjectLAND* TESObjectCELL::GetLand() {
    if (GetInterior() || GetPersistentCell()) {
        return nullptr;
    }
    if (!pCellLand) {
        TESObjectLAND* v2;
        AutoMemContext apMemory(
            MEM_CONTEXT_LAND, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 4022
        );
        TESObjectLAND* v4 = static_cast<TESObjectLAND*>(ourMalloc(sizeof(TESObjectLAND)));
        if (v4) {
            v2 = ::new (v4) TESObjectLAND;
        }
        else {
            v2 = nullptr;
        }
        pCellLand = v2;
        pCellLand->SetCell(this);
    }
    return pCellLand;
}

float TESObjectCELL::GetScaleBasedOnLocation(
    TESRegionPoint* apTest,
    REGION_DATA_ID aID,
    TESRegionObjectBase* apObjectBase
) {
    BSSimpleList<TESRegion*>* pcurrent;
    TESRegionList* RegionList = GetRegionList(true);
    if (RegionList) {
        pcurrent = static_cast<BSSimpleList<TESRegion*>*>(RegionList);
    }
    else {
        pcurrent = nullptr;
    }
    BSSimpleList<TESRegion*>* pcurrent_2 = pcurrent;
    if (GetInterior() || !pcurrent) {
        return 0.0f;
    }
    int32_t icurrentOverride = -1;
    float fcurrentScale = 3.4028235e+38f;
    while (pcurrent_2 && pcurrent_2->m_item) {
        float fscaleRegion;
        TESRegionData* pdata;
        if (!pcurrent_2->m_item->GetDelete() &&
            ((fscaleRegion = 0.0f, (pdata = pcurrent_2->m_item->pDataList->Find(aID)) != nullptr) &&
             pdata->Validate())) {
            BSSimpleList<TESRegionPointList*>* pcurrentRPL;
            if (pdata->bOverride) {
                if (pdata->cPriority <= icurrentOverride) {
                    goto LABEL_16;
                }
            }
            else {
                if (icurrentOverride >= 0) {
                    goto LABEL_44;
                }
            }
            for (pcurrentRPL = pcurrent_2->m_item->pPointLists;
                 pcurrentRPL && (pcurrentRPL->m_item && !(fscaleRegion > 0.0)); pcurrentRPL = pcurrentRPL->m_pkNext) {
                float a2 = static_cast<float>(pcurrentRPL->m_item->iDistanceInsideAtMax);
                float ftemp = pcurrentRPL->m_item->GetScaleBasedOnLocation(apTest, a2);
                if (ftemp > 1.0) {
                    ftemp = 1.0f;
                }
                fscaleRegion = BSMAX(fscaleRegion, ftemp);
            }
            if (fscaleRegion <= 0.0) {
            LABEL_44:;
                pcurrent_2 = pcurrent_2->m_pkNext;
            }
            else {
                if (pdata->bOverride) {
                    fcurrentScale = 3.4028235e+38f;
                    icurrentOverride = pdata->cPriority;
                }
                if (pdata->GetID() == REGION_DATA_OBJECTS_ID &&
                    (apObjectBase && (apObjectBase->GetObject() && !apObjectBase->GetID()))) {
                    TESRegionDataManager* RegionDataManager = pDataHandler->GetRegionDataManager();
                    TESRegionDataObjects* pdataObjects = RegionDataManager->IsRegionDataObjects(pdata);
                    TESRegionObjectList* v5 = pdataObjects->GetObjectList();
                    // An objects entry (GetID 0) is a TESRegionObject.
                    if (v5->IsInListTOL(static_cast<TESRegionObject*>(apObjectBase), true, false)) {
                        goto LABEL_45;
                    }
                    goto LABEL_44;
                }
                if ((pdata->GetID() == REGION_DATA_GRASS_ID) && (apObjectBase) && (apObjectBase->GetObject()) &&
                    (apObjectBase->GetID() == 1)) {
                    int32_t aiParentFind;
                    TESRegionDataManager* v14 = pDataHandler->GetRegionDataManager();
                    TESRegionDataGrass* pdataGrass = v14->IsRegionDataGrass(pdata);
                    // A grass entry (GetID 1) is a TESRegionGrassObject.
                    TESRegionGrassObject* pgrassObject = static_cast<TESRegionGrassObject*>(apObjectBase);
                    if (pgrassObject->GetParentTexture()) {
                        TESForm* v6 = pgrassObject->GetParentTexture();
                        aiParentFind = v6->iFormID;
                    }
                    else {
                        aiParentFind = -1;
                    }
                    TESForm* v7 = pgrassObject->GetObject();
                    uint32_t a1 = v7->iFormID;
                    TESRegionGrassObjectList* v8 = pdataGrass->GetObjectList();
                    if (!v8->Find(a1, aiParentFind)) {
                        goto LABEL_44;
                    }
                }

            LABEL_45:;
                while (pcurrentRPL && pcurrentRPL->m_item) {
                    float a2a = static_cast<float>(pcurrentRPL->m_item->iDistanceInsideAtMax);
                    float ftemp_2 = pcurrentRPL->m_item->GetScaleBasedOnLocation(apTest, a2a);
                    if (ftemp_2 > 1.0) {
                        ftemp_2 = 1.0f;
                    }
                    fscaleRegion = BSMAX(fscaleRegion, ftemp_2);
                    pcurrentRPL = pcurrentRPL->m_pkNext;
                }
                fcurrentScale = BSMIN(fcurrentScale, fscaleRegion);
                pcurrent_2 = pcurrent_2->m_pkNext;
            }
        }
        else {
        LABEL_16:;
            pcurrent_2 = pcurrent_2->m_pkNext;
        }
    }
    if (fcurrentScale < 0.0) {
        fcurrentScale = 0.0f;
    }
    if (fcurrentScale > 1.0) {
        return 0.0f;
    }
    return fcurrentScale;
}

void TESObjectCELL::SetLand(TESObjectLAND* apLand) {
    if (!GetInterior() && pCellLand != apLand) {
        if (pCellLand) {
            delete pCellLand;
        }
        pCellLand = apLand;
    }
}

void TESObjectCELL::Detach(bool abDetachHavok) {
    CellRefLockEnter();
    TESObjectCELL::CELL_STATE estate = GetState();
    if (estate == TESObjectCELL::CS_ATTACHED || estate == TESObjectCELL::CS_ATTACHING) {
        SetState(TESObjectCELL::CS_DETACHING);
        FadeToLowDetail();
        pModelLoader->CancelReferencesForCell(this);
        TES::pInstance->SetUpdateGridString();
        AttachLights(false);
        if (GetInterior()) {
            TES::pInstance->AddToInteriorBuffer(this);
        }
        else {
            TES::pInstance->MoveToEndOfExteriorBuffer(this);
        }
        if (abDetachHavok) {
            DetachHavok();
        }
        ProcessAddonNodesForRefs(false);
        NiNode* proot = Load3D();
        NiNode* Root = TES::pInstance->pObjRoot;
        Root->DetachChild(proot);
        RemoveReferencesFromLoadedReferenceCollection();
        RemoveNavmeshObstacles();
        SetState(TESObjectCELL::CS_LOADED);
        bCellDetached = true;
        if (!pSaveLoadManager->IsLoadingGame()) {
            ChangeProcessLevel(true);
        }
        SetDetachTime();
        AttachSounds(false);
        NavMeshRender* Instance = NavMeshRender::GetInstance();
        Instance->RemoveCell(this);
        uint32_t CastingType = iFormID;
        if (pDataHandler->IsFormIDCreated(CastingType)) {
            pSaveLoadGame->RemoveChanges(this);
        }
        TES::pInstance->SetUpdateGridString();
        BSPortalGraph* pportalgraph = GetPortalGraph();
        NiNode* StaticNode = static_cast<NiNode*>(GetStaticNode());
        pportalgraph->RemoveUnboundNode(StaticNode);
        NiNode* ActorNode = static_cast<NiNode*>(GetActorNode());
        pportalgraph->RemoveUnboundNode(ActorNode);
        NiNode* ChildNode = GetChildNode(TESObjectCELL::CN_LAND);
        pportalgraph->RemoveUnboundNode(ChildNode);
        NiNode* MarkerNode = GetMarkerNode();
        pportalgraph->RemoveUnboundNode(MarkerNode);
    }
    CellRefLockLeave();
}

bool TESObjectCELL::IsRefListLocked() {
    return kSpinLock.IsLocked();
}

void TESObjectCELL::SetHasWater(bool abNewbool) {
    uint8_t ucCellFlags;
    uint8_t v3;
    if (abNewbool) {
        cCellFlags |= 2u;
        ucCellFlags = cCellFlags;
    }
    else {
        cCellFlags &= (~2u);
        v3 = cCellFlags;
    }
    uint8_t v4 = cCellFlags;
}

uint32_t TESObjectCELL::GetGroupBlockKey() {
    if (GetInterior()) {
        return (uint32_t)((TESForm::GetFormID() & 0xFFFFFFull) % 10);
    }
    int32_t dataY = GetDataY();
    int32_t dataX = GetDataX();
    return TESObjectCELL::CalcExtGroupBlockKey(dataX, dataY);
}

uint32_t TESObjectCELL::GetGroupSubBlockKey() {
    if (GetInterior()) {
        return (uint32_t)(((TESForm::GetFormID() & 0xFFFFFFull) % 100) / 10);
    }
    int32_t dataY = GetDataY();
    int32_t dataX = GetDataX();
    return TESObjectCELL::CalcExtGroupSubBlockKey(dataX, dataY);
}

void TESObjectCELL::SetInterior(bool abVal) {
    if (abVal) {
        cCellFlags |= 1u;
    }
    else {
        cCellFlags &= (~1u);
    }
}

bool TESObjectCELL::CantTravelFrom() {
    uint8_t v3 = (uint8_t)((cCellFlags & 4) != 0);
    if (GetInterior()) {
        v3 = (uint8_t)(!v3);
    }
    if (v3) {
        return true;
    }
    TESWorldSpace* worldSpace = GetWorldSpace();
    return (bool)((uint8_t)(worldSpace && worldSpace->CantTravelFrom()));
}

bool TESObjectCELL::GetHideLand(uint32_t aiQuad) {
    EXTERIOR_DATA* pData = GetExteriorData();
    return (pData && ((((int32_t)(1 << aiQuad)) & ((int8_t)pData->ucLandHideFlags)) != 0));
}

void TESObjectCELL::SetDataCoord(int32_t aiX, int32_t aiY) {
    EXTERIOR_DATA* exteriorData;
    if (!GetInterior()) {
        exteriorData = GetExteriorData();
        if (exteriorData) {
            exteriorData->iCellX = aiX;
            exteriorData->iCellY = aiY;
        }
    }
}

BSSimpleList<TESObjectREFR*>* TESObjectCELL::GetWaterRefList() {
    if (pLoadedData) {
        return &pLoadedData->WaterRefs;
    }

    return (BSSimpleList<TESObjectREFR*>*)nullptr;
}

void TESObjectCELL::ClearQueuedRefCount() {
    iQueuedRefCount = 0;
}

void TESObjectCELL::ClearCriticalQueuedRefCount() {
    iCriticalQueuedRefCount = 0;
}

void TESObjectCELL::SetImageSpace(TESImageSpace* apImageSpace) {
    ExtraDataList* extraDataList = GetExtra();
    extraDataList->SetImageSpace(apImageSpace);
}

float TESObjectCELL::GetDistanceFromPoint(const NiPoint3& aPos) {
    float v11 = (float)(double)(GetDataX() << 12);
    float v10 = (float)(v11 + 4096.0);
    float v8 = (float)(double)(GetDataY() << 12);
    float v9 = (float)(v8 + 4096.0);
    if (v11 > ((double)(&aPos)->x)) {
        if (v9 < ((double)(&aPos)->y)) {
            return GetDistanceBetweenPoints((&aPos)->x, (&aPos)->y, v11, v9);
        }

        if (!(v8 > ((double)(&aPos)->y))) {
            return (v11 - (&aPos)->x);
        }

        return GetDistanceBetweenPoints((&aPos)->x, (&aPos)->y, v11, v8);
    }

    if (!(v10 < ((double)(&aPos)->x))) {
        if (v9 < ((double)(&aPos)->y)) {
            return ((&aPos)->y - v9);
        }

        if (!(v8 > ((double)(&aPos)->y))) {
            return 0.0;
        }

        return (v8 - (&aPos)->y);
    }

    if (!(v9 < ((double)(&aPos)->y))) {
        if (!(v8 > ((double)(&aPos)->y))) {
            return (v11 - (&aPos)->x);
        }

        return GetDistanceBetweenPoints((&aPos)->x, (&aPos)->y, v10, v8);
    }

    return GetDistanceBetweenPoints((&aPos)->x, (&aPos)->y, v10, v9);
}

bool TESObjectCELL::GetFullySeen() {
    return ((cCellGameFlags & 1) != 0);
}

bool TESObjectCELL::HasReferences() {
    return (bool)((uint8_t)(!(&listReferences)->IsEmpty()));
}

bool TESObjectCELL::IsOffLimits() {
    return ((((int32_t)TESForm::GetFormFlags()) & 131072) != 0);
}

bool TESObjectCELL::All3DWithVisibleDistantFadingIn() {
    return (bool)((uint8_t)bDisplayHighDetail);
}

SeenData* TESObjectCELL::GetSeenData() {
    ExtraDataList* extra = GetExtra();
    return static_cast<SeenData*>(extra->GetSeenData());
}

void TESObjectCELL::SetAutoWaterLoaded(bool abVal) {
    bAutoWaterLoaded = (bool)((int8_t)abVal);
}

void TESObjectCELL::Clear3D() {
    ahkpWorld* v1;
    bhkWorld* v3 = (&ExtraData)->GetbhkWorld();
    if ((v3) && (TESObjectCELL::pIslandListener)) {
        v1 = v3->operator*();
        TESObjectCELL::pIslandListener->RemoveFromWorld(v1);
    }

    SetState(TESObjectCELL::CS_UNLOADING);
    FreeLoadedData();
    (&ExtraData)->SetbhkWorld(nullptr);
    ClearQueuedRefCount();
    ClearCriticalQueuedRefCount();
    SetState(TESObjectCELL::CS_NOTLOADED);
    bCellDetached = false;
}

bool TESObjectCELL::PickWorldDistance(
    NiPick* apPick,
    const NiPoint3& aOrigin,
    const NiPoint3& aDir,
    float afDistance,
    char acNodeFlags
) {
    NiNode* childNode;
    NiNode* v8;
    NiNode* v9;
    NiNode* v10;
    NiNode* v11;
    NiNode* v12;
    uint8_t v14 = 0;
    if ((apPick) && (acNodeFlags)) {
        if ((acNodeFlags & 1) != 0) {
            childNode = GetChildNode(TESObjectCELL::CN_ACTOR);
            v14 = (uint8_t)(((int8_t)PickDistance(apPick, aOrigin, aDir, afDistance, childNode, false)) != 0);
        }
        if ((acNodeFlags & 2) != 0) {
            v8 = GetChildNode(TESObjectCELL::CN_MARKER);
            if (PickDistance(apPick, aOrigin, aDir, afDistance, v8, false)) {
                v14 = 1;
            }
        }
        if ((acNodeFlags & 4) != 0) {
            v9 = GetChildNode(TESObjectCELL::CN_LAND);
            if (PickDistance(apPick, aOrigin, aDir, afDistance, v9, false)) {
                v14 = 1;
            }
        }
        if ((acNodeFlags & 8) != 0) {
            v10 = GetChildNode(TESObjectCELL::CN_STATIC);
            if (PickDistance(apPick, aOrigin, aDir, afDistance, v10, false)) {
                v14 = 1;
            }
        }
        if ((acNodeFlags & 16) != 0) {
            v11 = GetChildNode(TESObjectCELL::CN_DYNAMIC);
            if (PickDistance(apPick, aOrigin, aDir, afDistance, v11, false)) {
                v14 = 1;
            }
        }
        if (((acNodeFlags & 8) != 0) || ((acNodeFlags & 16) != 0)) {
            v12 = GetChildNode(TESObjectCELL::CN_MULTIBOUND);
            if (PickDistance(apPick, aOrigin, aDir, afDistance, v12, false)) {
                return true;
            }
        }
    }

    return (bool)v14;
}

void TESObjectCELL::ChangeProcessLevel(bool abChangeLevel) {
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* RefList = GetRefList();
    ReferenceProcessLists.ChangeProcessLevel(RefList, abChangeLevel);
    CellRefLockLeave();
}

void TESObjectCELL::ClearReference3D() {
    BSFadeNode* v2;
    NiNode* v3;
    PlayerCharacter* v4;
    CellRefLockEnter();
    ClearPortalGraph();
    BSSimpleList<TESObjectREFR*>* ecx0 = GetRefList();
    while (ecx0) {
        v4 = static_cast<PlayerCharacter*>(ecx0->m_item);
        ecx0 = ecx0->m_pkNext;
        if ((v4) && (v4 != pPlayer)) {
            v3 = v4->TESObjectREFR::Get3D();
            if (v3) {
                v2 = v3->IsFadeNode();
                if (v2) {
                    v2->TurnFadeNodeOff();
                }
            }
            v4->Set3D(nullptr, false);
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::UnLoadAllTempData() {
    FNVD_INLINED_LOCAL(NavMeshPtr, apResult, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NavMeshPtr, apCell, std::nullopt, std::nullopt);
    TESObjectREFR* pref;
    CellRefLockEnter();
    this->sNumRefsWithVisibleDistant = 0;
    this->sNumLoadedRefsWithVisibleDistant = 0;
    bool bAllowChanges = pSaveLoadGame->SetThreadAllowChanges(false);
    this->RemoveNavmeshObstacles();
    BSSimpleList<TESObjectREFR*>* pcurrent = this->GetRefList();
    BSSimpleList<TESObjectREFR*>* pprevious = nullptr;
    while (pcurrent && !pcurrent->IsEmpty()) {
        TESFile* File;
        pref = pcurrent->m_item;
        if (pref->GetRefPersists() || (pref->GetFile(-1) && (File = pref->GetFile(-1), !File->GetMaster()))) {
            pprevious = pcurrent;
            pcurrent = pcurrent->m_pkNext;
        }
        else {
            if (pprevious) {
                pprevious->Remove(pref);
                pcurrent = pprevious->m_pkNext;
            }
            else {
                pcurrent->RemoveHead();
            }
            pLoadedReferenceCollection->RemoveReference(pref);
            if (pref->IsActor()) {
                Actor* v17 = static_cast<Actor*>(pref);
                if (static_cast<MobileObject*>(pref)->MobileObject::GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
                    v17->SetLastSeenTime();
                }
            }
            pSaveLoadGameOLD->UnloadForm(pref);
            pSaveLoadGame->UnloadForm(pref, false);
            char v18 = 1;
            if (pSaveLoadGame->GetSaveGameLoading()) {
                BGSChangeFlags changeFlags(8);
                if (pSaveLoadGame->GetOldChange(pref, changeFlags) && pref->MoveToEditorLocation(this)) {
                    v18 = 0;
                }
            }
            if ((pref->GetObjectReference()) && (pref->GetObjectReference()->GetFormType() == PWAT_ID)) {
                BGSPlaceableWater* ObjectReference = static_cast<BGSPlaceableWater*>(pref->GetObjectReference());
                if (ObjectReference->IsAutomaticWater()) {
                    this->RemovePlaceableWaterRef(pref);
                }
            }

            if ((v18) && (pref != pPlayer)) {
                GarbageCollector::Add(pref);
            }
        }
    }
    pSaveLoadGame->SetThreadAllowChanges(bAllowChanges);
    this->CellRefLockLeave();
    TESForm* Land = (TESForm*)this->GetLand();
    if (Land) {
        TESFile* v15 = Land->GetFile(-1);
        if (!v15 || v15->GetMaster()) {
            this->SetLand(nullptr);
            Land = nullptr;
        }
    }
    NavMeshArray* pNavMeshes = this->GetNavMeshArray();
    if (pNavMeshes) {
        AutoMemContext apMemory(
            MEM_CONTEXT_NAVMESH, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
            0x2769
        );
        for (uint32_t aiNavMeshIndex = 0;; ++aiNavMeshIndex) {
            uint32_t NavMeshCount = pNavMeshes->GetNavMeshCount();
            if (aiNavMeshIndex >= NavMeshCount) {
                break;
            }
            NavMeshPtr* NavMeshByIndex = pNavMeshes->GetNavMeshByIndex(&apResult, aiNavMeshIndex);
            TESForm* Object = NavMeshByIndex->m_pObject;
            TESFile* v12 = Object->GetFile(-1);
            FNVD_DESTRUCT(NiPointer<NavMesh>)(&apResult);
            if (!v12 || v12->GetMaster()) {
                pNavMeshes->GetNavMeshByIndex(&apCell, aiNavMeshIndex);
                pSaveLoadGame->UnloadForm(apCell.operator NavMesh*(), false);
                pNavMeshes->RemoveNavMeshByIndex(aiNavMeshIndex--);
                FNVD_DESTRUCT(NiPointer<NavMesh>)(&apCell);
            }
        }
    }
    this->SetHasTempData(false);
}

void TESObjectCELL::ClearPortalGraph() {
    if (spPortalGraph.m_pObject) {
        if (GetInterior()) {
            ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
            BSPortalGraph* PortalGraph = ShadowSceneNode_->GetPortalGraph();
            if (PortalGraph == spPortalGraph.m_pObject) {
                ShadowSceneNode* v3 = BSShaderManager::GetShadowSceneNode(0);
                v3->SetPortalGraph(nullptr);
            }
        }
        spPortalGraph.operator=(static_cast<BSPortalGraph*>(nullptr));
        if (GetWorldSpace()) {
            TESWorldSpace* WorldSpace = GetWorldSpace();
            WorldSpace->ClearPortalGraph();
        }
    }
}

TESWaterForm* TESObjectCELL::GetWaterType() {
    TESWorldSpace* worldSpace;
    ExtraDataList* extra = GetExtra();
    TESWaterForm* waterType = extra->GetWaterType();
    if (!waterType) {
        worldSpace = GetWorldSpace();
        if (worldSpace) {
            waterType = worldSpace->GetWaterType();
        }
    }
    if (!waterType) {
        waterType = pDefaultWater;
    }
    if ((waterType) && (TESForm::GetInitialized()) && (waterType->pRemappedWaterForm)) {
        return waterType->pRemappedWaterForm;
    }

    return waterType;
}

TESObjectREFR* TESObjectCELL::CreateLootBagAtRef(TESObjectREFR* apRef) {
    TESObjectREFR* v4;
    NiPoint3 ecx0;
    NiPoint3 ecx0_12;
    ecx0_12;
    int8_t v10 = 0;
    if (apRef && (apRef->TESObjectREFR::GetParentCell() == this)) {
        ecx0_12 = apRef->GetLocationOnReference();
        v10 = 1;
    }
    else {
        if (pPlayer->TESObjectREFR::GetParentCell() == this) {
            ecx0_12 = pPlayer->GetLocationOnReference();
            v10 = 1;
        }
    }
    if (!v10) {
        ecx0;
        DetermineCOCPlacement(ecx0_12, ecx0);
    }
    void* apMemory = ourMalloc(sizeof(TESObjectREFR));
    int32_t v11 = 0;
    if (apMemory) {
        v4 = ::new (apMemory) TESObjectREFR;
    }
    else {
        v4 = nullptr;
    }
    v11 = (-1);
    TESObjectREFR* apRef_2 = v4;
    v4->SetLocationOnReference(ecx0_12);
    apRef_2->SetObjectReference(pDefLootBag);
    TESBoundObject* objectReference = pPlayer->TESObjectREFR::GetObjectReference();
    apRef_2->SetOwner(objectReference);
    FillLootBag(apRef_2);
    AddReference(apRef_2, false);
    return apRef_2;
}

IntSeenData* TESObjectCELL::GetIntSeenSection(int32_t aiSectionX, int32_t aiSectionY, bool abCreate) {
    SeenData* seenData;
    ExtraDataList* extra;
    IntSeenData* v8;
    void* apMemory;
    IntSeenData* v12;
    IntSeenData* v10 = nullptr;
    if (GetInterior()) {
        seenData = GetSeenData();
        v10 = dynamic_cast<IntSeenData*>(seenData);
        v12 = nullptr;
        while (v10 && ((v10->cSectionX != aiSectionX) || (v10->cSectionY != aiSectionY))) {
            v12 = v10;
            v10 = v10->pNextSection;
        }
        if ((!v10) && abCreate) {
            if (v12) {
                apMemory = ourMalloc(sizeof(IntSeenData));
                if (apMemory) {
                    v8 = ::new (apMemory) IntSeenData(aiSectionX, aiSectionY);
                }
                else {
                    v8 = nullptr;
                }
                v12->pNextSection = v8;
                return v12->pNextSection;
            }

            v10 = static_cast<IntSeenData*>(ourMalloc(sizeof(IntSeenData)));
            if (v10) {
                v10 = ::new (v10) IntSeenData(aiSectionX, aiSectionY);
            }
            else {
                v10 = nullptr;
            }
            extra = GetExtra();
            extra->SetSeenData(v10);
            AddChange(0x80000000u);
        }
    }
    return v10;
}

void TESObjectCELL::RemovePlaceableWaterRef(TESObjectREFR* a2) {
    BSSimpleList<TESObjectREFR*>* ecx0;
    TESObjectREFR* v3;
    for (ecx0 = GetRefList(); ecx0 && ecx0->m_item; ecx0 = ecx0->m_pkNext) {
        v3 = ecx0->m_item;
        if (v3->GetRelevantWaterType() == a2) {
            v3->SetRelevantWaterType(nullptr);
        }
    }
    if (pPlayer->GetRelevantWaterType() == a2) {
        pPlayer->SetRelevantWaterType(nullptr);
    }
}

bool TESObjectCELL::IsActorBaseCellOwner(TESActorBase* apActorBase) {
    TESNPC* v4;
    int32_t extraRank;
    TESFaction* apFaction;
    TESNPC* v7;
    int8_t v8 = 0;
    TESForm* owner = GetOwner();
    if (owner) {
        v7 = dynamic_cast<TESNPC*>(owner);
        if (v7) {
            return (bool)((uint8_t)(static_cast<const void*>(v7) == static_cast<const void*>(apActorBase)));
        }

        apFaction = dynamic_cast<TESFaction*>(owner);
        if (apFaction) {
            extraRank = GetOwnershipRank();
            v4 = dynamic_cast<TESNPC*>(apActorBase);
            if (v4) {
                return (bool)((uint8_t)(static_cast<TESActorBaseData*>(v4)->GetFactionRank(apFaction, false) >=
                                        extraRank));
            }
        }
    }
    return (bool)v8;
}

void TESObjectCELL::RemoveWaterRef(TESObjectREFR* apRef) {
    if (pLoadedData) {
        (&pLoadedData->WaterRefs)->Remove(apRef);
    }
}

void TESObjectCELL::RemoveReferencesFromLoadedReferenceCollection() {
    TESBoundObject* pObject;
    BGSTerrainManager* v2;
    BGSTerrainManager* terrainManager;
    BGSTerrainManager* v4;
    TESWorldSpace* worldSpace;
    TESObjectREFR* apRef;
    BSSimpleList<TESObjectREFR*>* ecx0;
    CellRefLockEnter();
    for (ecx0 = GetRefList(); ecx0; ecx0 = ecx0->m_pkNext) {
        apRef = ecx0->m_item;
        if (apRef) {
            pLoadedReferenceCollection->RemoveReference(apRef);
            if ((!TES::pInstance->TES::GetInterior()) && (TES::pInstance->GetWorldSpace())) {
                pObject = apRef->TESObjectREFR::GetObjectReference();
                if (pObject->TESForm::GetHasSpokenFlag()) {
                    worldSpace = apRef->GetWorldSpace();
                    if (worldSpace) {
                        if (apRef->GetDisabled()) {
                            terrainManager = worldSpace->GetTerrainManager();
                            terrainManager->HideTree(apRef, true);
                        }
                        else {
                            v2 = worldSpace->GetTerrainManager();
                            v2->ShowTree(apRef);
                        }
                        v4 = worldSpace->GetTerrainManager();
                        v4->SetNeedsImmediateUpdate(true);
                    }
                }
            }
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::BuildLocalMapDoorList(BSSimpleList<TESObjectREFR*>* apOutList) {
    if (apOutList) {
        CellRefLockEnter();
        for (TESObjectREFR* a2 : *GetRefList()) {
            if ((a2) && (!a2->GetDisabled() && !a2->GetDelete())) {
                TESObjectDOOR* ObjectReference = static_cast<TESObjectDOOR*>(a2->GetObjectReference());
                if (ObjectReference->GetFormType() == 28 && ObjectReference != pJailMarker) {
                    TESObjectDOOR* v3 = static_cast<TESObjectDOOR*>(a2->GetObjectReference());
                    if (a2->GetTeleport() || v3->IsRandomDoor()) {
                        apOutList->AddHead(a2);
                    }
                }
            }
        }
        CellRefLockLeave();
    }
}

void TESObjectCELL::GetExteriorLocalMapTexture(NiPointer<NiTexture>& aspTexture) {
    FNVD_INLINED_LOCAL(NiPointer<BSRenderedTexture>, sprt, std::nullopt, std::nullopt);
    NiPointer<NiTexture> sprettex(static_cast<NiTexture*>(nullptr));
    const char* v5 = "GetExteriorLocalMapTexture";
    if (!GetInterior()) {
        ::new (&sprt) NiPointer<ActorPathingMessageQueue>(static_cast<ActorPathingMessageQueue*>(nullptr));
        TakeExteriorLocalMapPicture(sprt);
        sprettex.operator=(static_cast<NiTexture*>(sprt.m_pObject->GetTexture(0)));
        sprt.m_pObject->ClearRenderedTextures();
        sprt.~NiPointer<BSRenderedTexture>();
    }
    aspTexture.operator=(sprettex);
}

void TESObjectCELL::GetInteriorLocalMapTexture(
    int32_t aiSectionX,
    int32_t aiSectionY,
    NiPointer<NiTexture>& aspTexture
) {
    NiPointer<NiTexture> sprettex(static_cast<NiTexture*>(nullptr));
    const char* v7 = "GetInteriorLocalMapTexture";
    if (GetInterior()) {
        NiPointer<BSRenderedTexture> sprt(static_cast<BSRenderedTexture*>(nullptr));
        TakeInteriorLocalMapPicture(aiSectionX, aiSectionY, sprt);
        sprettex.operator=(static_cast<NiTexture*>(sprt.m_pObject->GetTexture(0)));
        sprt.m_pObject->ClearRenderedTextures();
    }
    aspTexture.operator=(sprettex);
}

void TESObjectCELL::SaveReferences(TESFile* apFile) {
    TESObjectREFR* pref;
    NiTPrimitiveArray<TESObjectREFR*> saverefs(0, 1);
    saverefs.SetSize(50u);
    saverefs.m_usGrowBy = 50;
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* plist = GetRefList();
    while (plist && !plist->IsEmpty()) {
        pref = plist->m_item;
        plist = plist->m_pkNext;
        bool bsaveref = !pref->GetTemporary();
        if (pref->GetDelete()) {
            TESFile* pfile = pref->GetFile(0);
            if ((pfile) && (!pfile->GetMaster())) {
                bsaveref = false;
            }
        }
        if (apFile != pref->GetFile(-1) && !pref->GetAltered()) {
            bsaveref = false;
        }
        if (!GetInterior() && pref->GetRefPersists() && !GetPersistentCell()) {
            bsaveref = false;
        }
        if (bsaveref) {
            saverefs.AddFirstEmpty(pref);
        }
    }
    CellRefLockLeave();
    saverefs.Compact();
    int32_t isize = saverefs.m_usESize;
    for (int32_t n = 0; n < isize; ++n) {
        TESObjectREFR* pref_2 = saverefs.operator[](n);
        for (int32_t m = n + 1; m < isize; ++m) {
            TESObjectREFR* pref2 = saverefs.operator[](m);
            if (pref2->SavesBefore(pref_2)) {
                saverefs.SetAt(n, pref2);
                saverefs.SetAt(m, pref_2);
                pref_2 = pref2;
            }
        }
    }
    for (int32_t i = 0; i < isize; ++i) {
        TESObjectREFR* pref_3 = saverefs.operator[](i);
        if (pref_3) {
            pref_3->TESForm::Save(apFile);
        }
    }
}

// The mask of the cell at (aiX, aiY): locked on first use, then the byte at aiIndex set to aiValue, or to 0xFF where it
// is already set. The D3D texture is locked with IDirect3DTexture9::LockRect, as CreateCanopyShadowMaskForCell does.
void TESObjectCELL::PlaceCanopyShadowInAdjacentCell(
    int32_t aiX,
    int32_t aiY,
    _D3DLOCKED_RECT*& apRect,
    int32_t aiIndex,
    int32_t aiValue
) {
    NiRenderedTexture* pTex = nullptr;
    if (!apRect) {
        TESWorldSpace* pWorldSpace = GetWorldSpace();
        TESObjectCELL* pTmpCell = pDataHandler->GetCellFromCellCoord(aiX, aiY, pWorldSpace, false);
        if (!pTmpCell) {
            return;
        }
        pTmpCell->GetExtra()->GetCanopyShadowMask(pTex, apRect);
        if (!pTex) {
            TESObjectCELL::CreateCanopyShadowMaskForCell(pTmpCell, pTex, apRect);
        }
    }
    if (apRect) {
        if (!apRect->pBits) {
            NiDX9DynamicTextureData* pTexData = static_cast<NiDX9DynamicTextureData*>(pTex->m_pkRendererData);
            IDirect3DTexture9* pD3DTexture = static_cast<IDirect3DTexture9*>(pTexData->GetD3DTexture());
            pD3DTexture->LockRect(0, apRect, nullptr, 0);
        }
        uint8_t* pTmpData = static_cast<uint8_t*>(apRect->pBits);
        if (pTmpData[aiIndex]) {
            pTmpData[aiIndex] = 0xFF;
        }
        else {
            pTmpData[aiIndex] = static_cast<uint8_t>(aiValue);
        }
    }
}

void TESObjectCELL::ProcessActors() {
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* RefList = GetRefList();
    ReferenceProcessLists.ProcessActors(RefList);
    CellRefLockLeave();
}

void TESObjectCELL::FillBarterContainer(TESObjectREFR* apActor, InventoryChanges* apBarterContainer) {
    if ((apBarterContainer) && (apActor)) {
        TESObjectREFR* pmerchant = (&apActor->m_Extra)->GetMerchantContainer();
        CellRefLockEnter();
        for (TESObjectREFR*& rpItem : GetRefList()->ItemsUntilNull()) {
            InventoryChanges* pActorlist = nullptr;
            TESObjectREFR* pref = rpItem;
            ExtraDataList* pExtra = nullptr;
            if ((!pref->GetDisabled() && !pref->GetDelete()) && (pref->GetOwner()) &&
                (pref->IsAnOwner(static_cast<Actor*>(apActor), false) && pref != pmerchant)) {
                switch (pref->GetObjectReference()->GetFormType()) {
                    case ARMO_ID:
                    case BOOK_ID:
                    case CLOT_ID:
                    case INGR_ID:
                    case LIGH_ID:
                    case MISC_ID:
                    case WEAP_ID:
                    case AMMO_ID:
                    case KEYM_ID:
                    case ALCH_ID:
                    case COBJ_ID:
                    case IMOD_ID:
                    case CHIP_ID:
                    case CMNY_ID: {
                        goto LABEL_15;
                    }
                    case CONT_ID: {
                        pActorlist = (&pref->m_Extra)->GetContainerChanges();
                        if (pActorlist) {
                            pActorlist->CreateCopyofList(apBarterContainer, pref, false);
                        }
                        break;
                    }
                    case CCRD_ID: {
                        ExtraDataList* pExtra_2;
                        ItemChange* pitem;
                        // This address (&pref->m_Extra) is tested although it is never null, as the game
                        // does.
                        (&pref->m_Extra)->SetCount(1);
                    LABEL_15:;
                        ItemChange* apMemory = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                        if (apMemory) {
                            pitem = ::new (apMemory) ItemChange(pref->GetObjectReference(), 0);
                        }
                        else {
                            pitem = nullptr;
                        }
                        if (!pitem->pExtraObjectList) {
                            BSSimpleList<ExtraDataList*>* v5;
                            BSSimpleList<ExtraDataList*>* v9 = static_cast<BSSimpleList<ExtraDataList*>*>(
                                ourMalloc(sizeof(BSSimpleList<ExtraDataList*>))
                            );
                            if (v9) {
                                v5 = ::new (v9) BSSimpleList<ExtraDataList*>;
                            }
                            else {
                                v5 = nullptr;
                            }
                            pitem->pExtraObjectList = v5;
                        }
                        ExtraDataList* v8 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                        if (v8) {
                            pExtra_2 = ::new (v8) ExtraDataList;
                        }
                        else {
                            pExtra_2 = nullptr;
                        }
                        pExtra = pExtra_2;
                        ExtraDataList* Extra = &pref->m_Extra;
                        pExtra->DuplicateExtraListForContainer(Extra);
                        pExtra->SetOriginalReferenceExtra(pref);
                        pExtra->RemoveOwnership();
                        pitem->pExtraObjectList->AddHead(pExtra);
                        pitem->iNumber = (&pref->m_Extra)->GetCount();
                        apBarterContainer->AddChangeItem(pitem, true);
                        break;
                    }
                    default: {
                        continue;
                    }
                }
            }
        }
        CellRefLockLeave();
    }
}

void TESObjectCELL::SaveGameTest() {
    CellRefLockEnter();
    for (TESObjectREFR* pref : *GetRefList()) {
        if ((pref) && (pref != pPlayer)) {
            Actor* pactor = dynamic_cast<Actor*>(pref);
            InventoryChanges* pinventory = (&pref->m_Extra)->GetContainerChanges();
            if (pinventory) {
                pinventory->RemoveAllItems(pref, pPlayer, false, false, true, false, -1, nullptr);
            }
            if ((pactor) && (!pactor->GetDisabled() && !pactor->GetDelete())) {
                pactor->ModifyDispositionTowardActor(pPlayer, 10.0f);
            }

            if ((pactor) &&
                (!pactor->GetEssential() && !pactor->GetDisabled() && !pactor->GetDelete() && !pactor->IsDead(false))) {
                pactor->StartCombat(pPlayer, nullptr, false, false, false, 0, true, nullptr);
                pactor->Kill(nullptr, 0.0f);
            }

            if (pref->GetLock()) {
                pref->UnLock();
            }
            TESObjectCELL::UpdateSeenDataForPlayer();
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::UpdateLights() {
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* ecx0 = GetRefList();
    while (ecx0) {
        TESObjectREFR* v6 = ecx0->m_item;
        ecx0 = ecx0->m_pkNext;
        if (v6) {
            TESBoundObject* ObjectReference = v6->GetObjectReference();
            if (ObjectReference->GetFormType() == 30) {
                ExtraDataList* Extra = v6->GetExtra();
                LightRefData* a2 = Extra->GetLight();
                if (a2) {
                    TESObjectLIGH* v3 = static_cast<TESObjectLIGH*>(v6->GetObjectReference());
                    v3->Update(a2, nullptr);
                }
            }
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::Destroy() {
    CellRefLockEnter();
    RemoveNavmeshObstacles();
    sNumRefsWithVisibleDistant = 0;
    for (BSSimpleList<TESObjectREFR*>* ecx0 = GetRefList(); ecx0 && !ecx0->IsEmpty(); ecx0 = GetRefList()) {
        PlayerCharacter* a2 = static_cast<PlayerCharacter*>(ecx0->m_item);
        if (a2 == pPlayer) {
            GetRefList()->RemoveHead();
            a2->SetParentCell(nullptr);
        }
        else {
            bool v2 = false;
            if ((GetPersistentCell()) && (pDataHandler)) {
                v2 = !pDataHandler->GetClearingData();
            }

            if (!a2->GetRefPersists() || (GetInterior() || v2)) {
                if ((a2->GetObjectReference()) &&
                    (a2->GetObjectReference()->GetFormType() == PWAT_ID &&
                     static_cast<BGSPlaceableWater*>(a2->GetObjectReference())
                         ->BGSPlaceableWater::IsAutomaticWater())) {
                    RemovePlaceableWaterRef(a2);
                }

                GarbageCollector::Add(a2);
            }
            else {
                if (GetPersistentCell()) {
                    if (a2) {
                        delete a2;
                    }
                }
                else {
                    a2->SetParentCell(nullptr);
                }
            }
            GetRefList()->RemoveHead();
        }
    }
    CellRefLockLeave();
}

TESObjectREFR* TESObjectCELL::SearchForm(TESBoundObject* a1, char a2) {
    if (!a1) {
        return nullptr;
    }
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* ecx0 = GetRefList();
    while (ecx0 && !ecx0->IsEmpty()) {
        TESObjectREFR* v6 = ecx0->m_item;
        TESBoundObject* ObjectReference = v6->GetObjectReference();
        ecx0 = ecx0->m_pkNext;
        if (ObjectReference && ((!a2 || !v6->GetDelete()) && ObjectReference == a1)) {
            CellRefLockLeave();
            return v6;
        }
    }
    CellRefLockLeave();
    return nullptr;
}

uint32_t TESObjectCELL::FindTeleportDoorsForLocation(
    const NiPoint3& arLocation,
    float afRadius,
    BSSimpleArray<TESObjectREFR*, 1024>* apRetDoors
) {
    if (!apRetDoors) {
        return 0;
    }
    float v12 = afRadius * afRadius;
    CellRefLockEnter();
    for (TESObjectREFR* a2 : *GetRefList()) {
        if ((a2) && (!a2->GetDisabledOrDeleted())) {
            TESForm* ObjectReference = a2->GetObjectReference();
            if ((ObjectReference->GetFormType() == 28 && ObjectReference != pJailMarker) && (a2->GetTeleport())) {
                NiPoint3 result;
                const NiPoint3* v5 = &a2->GetLocationOnReference();
                result = arLocation - *v5;
                double v6 = result.SqrLength();
                if (v12 > v6) {
                    apRetDoors->Add(a2);
                }
            }
        }
    }
    CellRefLockLeave();
    return apRetDoors->iSize;
}

void TESObjectCELL::TakeLocalMapPicture(NiCamera* apCamera, NiPointer<BSRenderedTexture>& aspRenderedTexture) {
    FNVD_INLINED_LOCAL(NiColorA, blackAlpha, std::nullopt, std::nullopt);
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LOCALMAP, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 0x2251
    );
    NiPointer<BSShaderAccumulator> spoldAccum(BSShaderManager::GetAccumulator());
    BSCullingProcess culler(nullptr);
    NiPointer<BSRenderedTexture> sprendTextColor(static_cast<BSRenderedTexture*>(nullptr));
    NiPointer<BSRenderedTexture> sprendTextShader(static_cast<BSRenderedTexture*>(nullptr));
    NiPointer<BSRenderedTexture> sprendTextNormals(static_cast<BSRenderedTexture*>(nullptr));
    NiDX9Renderer* Renderer_ = BSShaderManager::GetRenderer();
    BSTextureManager* TextureManager = BSShaderManager::TextureManagerPtr();
    sprendTextNormals.operator=(
        TextureManager->BorrowRenderedTexture(Renderer_, BSTextureManager::TEX_LOCALMAP_NORMALS, 0, nullptr, 0)
    );
    NiDX9Renderer* fR = BSShaderManager::GetRenderer();
    BSTextureManager* v4 = BSShaderManager::TextureManagerPtr();
    sprendTextShader.operator=(v4->CreateRenderedTexture(fR, BSTextureManager::TEX_LOCALMAP_FINAL, nullptr, 0));
    if ((sprendTextShader.m_pObject) && (sprendTextNormals.m_pObject)) {
        void* pCustomAccumulator;
        NiColorA oldColor(0.0f, 0.0f, 0.0f, 0.0f);
        NiDX9Renderer* v22 = BSShaderManager::GetRenderer();
        v22->GetBackgroundColor(oldColor);
        NiDX9Renderer* v5 = BSShaderManager::GetRenderer();
        v5->SetBackgroundColorAlpha(*::new (&blackAlpha) NiColorA(0.0f, 0.0f, 0.0f, 1.0f));
        ShadowSceneNode* pscene = BSShaderManager::GetShadowSceneNode(0);
        bool bwasLightUpdateDisabled = pscene->bDisableLightUpdate;
        pscene->bDisableLightUpdate = true;
        bool boutsideFrame = BSRenderedTexture::QOutsideFrame();
        NiNode* pwaternode = TESWaterSystem::GetWaterRoot();
        bool bwaterculled = false;
        bool btreesenabled = true;
        if (pwaternode) {
            bwaterculled = pwaternode->GetAppCulled();
            pwaternode->SetAppCulled(true);
        }
        btreesenabled = BSTreeManager::GetTreesEnabled();
        BSTreeManager::SetTreesEnabled(false);
        if (boutsideFrame) {
            NiRenderTargetGroup* Group = sprendTextNormals.m_pObject->GetGroup();
            BSRenderedTexture::StartOffscreen(7u, Group);
        }
        else {
            BSRenderedTexture::BeginTexture(sprendTextNormals.m_pObject, 7u);
        }
        BSShaderAccumulator* v24 =
            static_cast<BSShaderAccumulator*>(NiMemObject::operator new(sizeof(BSShaderAccumulator)));
        if (v24) {
            pCustomAccumulator = ::new (v24) BSShaderAccumulator(99, 1u, 759u);
        }
        else {
            pCustomAccumulator = nullptr;
        }
        NiPointer<BSShaderAccumulator> spaccum(static_cast<BSShaderAccumulator*>(pCustomAccumulator));
        spaccum.m_pObject->SetShadowSceneNode(pscene);
        culler.SetAccumulator(spaccum.m_pObject);
        spaccum.m_pObject->SetRenderMode(BSShaderManager::BSSM_RENDER_LOCALMAP);
        culler.PushCullMode(BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS);
        BSShaderUtil::AccumulateScene(apCamera, pscene, culler);
        culler.PopCullMode();
        if (GetChildNode(TESObjectCELL::CN_PORTAL)) {
            culler.PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
            ShadowSceneNode* pscene_3 = static_cast<ShadowSceneNode*>(GetChildNode(TESObjectCELL::CN_PORTAL));
            BSShaderUtil::AccumulateScene(apCamera, pscene_3, culler);
            culler.PopCullMode();
        }
        BSShaderUtil::RenderScene(apCamera, spaccum.m_pObject, false);
        culler.SetAccumulator(nullptr);
        spaccum.operator=(static_cast<BSShaderAccumulator*>(nullptr));
        if (boutsideFrame) {
            BSRenderedTexture::StopOffscreen();
        }
        else {
            BSRenderedTexture::End();
        }
        BSTreeManager::SetTreesEnabled(btreesenabled);
        if (pwaternode) {
            pwaternode->SetAppCulled(bwaterculled);
        }
        if (boutsideFrame) {
            NiRenderTargetGroup* v7 = sprendTextShader.m_pObject->GetGroup();
            BSRenderedTexture::StartOffscreen(7u, v7);
        }
        else {
            BSRenderedTexture::BeginTexture(sprendTextShader.m_pObject, 7u);
        }
        ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
        ImageSpaceEffect* peffect = ImageSpaceManager_->GetEffect(ImageSpaceManager::EFFECT_MAP);
        BSRenderedTexture* v17 = sprendTextShader.m_pObject;
        BSRenderedTexture* fRa = sprendTextNormals.m_pObject;
        NiDX9Renderer* v14 = BSShaderManager::GetRenderer();
        ImageSpaceManager* v9 = BSShaderManager::GetImageSpaceManager();
        v9->RenderEffect(peffect, v14, fRa, v17, nullptr, 0);
        if (boutsideFrame) {
            BSRenderedTexture::StopOffscreen();
        }
        else {
            BSRenderedTexture::End();
        }
        pscene->bDisableLightUpdate = bwasLightUpdateDisabled;
        BSShaderAccumulator* Accumulator = BSShaderManager::GetAccumulator();
        Accumulator->bRenderNoShader = true;
        NiDX9Renderer* v20 = BSShaderManager::GetRenderer();
        v20->SetBackgroundColorAlpha(oldColor);
    }

    BSRenderedTexture* v19 = sprendTextNormals.m_pObject;
    BSTextureManager* v11 = BSShaderManager::TextureManagerPtr();
    v11->ReturnRenderedTexture(v19);
    aspRenderedTexture.operator=(sprendTextShader);
    BSShaderManager::SetAccumulator(spoldAccum.m_pObject);
}

void TESObjectCELL::TakeExteriorLocalMapPicture(NiPointer<BSRenderedTexture>& aspRenderedTexture) {
    NiPoint2 result;
    if (GetInterior()) {
        aspRenderedTexture.operator=(static_cast<BSRenderedTexture*>(nullptr));
    }
    else {
        NiNode* v10;
        NiCamera* pCellCamera;
        NiMatrix3 camrot;
        NiPoint2 heightextents;
        NiNode* pscene = BSShaderManager::GetShadowSceneNode(0);
        NiPoint3 campos = NiPoint3_ZERO;
        camrot.FromEulerAnglesXYZ(-1.5707964f, -1.5707964f, 1.5707964f);
        NiFrustum frustum(false);
        frustum.m_fNear = 100.0f;
        frustum.m_bOrtho = true;
        TESObjectLAND* Land = GetLand();
        NiPoint2* MinMaxLandHeight = ::new (&result) NiPoint2(Land->GetMinMaxLandHeight());
        float x = MinMaxLandHeight->x;
        float y = MinMaxLandHeight->y;
        heightextents.x = x;
        heightextents.y = y;
        int32_t iHalfMapWorldDim = 2176;
        campos.x = static_cast<float>((GetDataX() << 12) + 2176);
        campos.y = static_cast<double>((GetDataY() << 12) + 2176) - 32.0;
        campos.z = heightextents.y + 20000.0;
        frustum.m_fLeft = -2176.0f;
        frustum.m_fRight = 2176.0f;
        frustum.m_fTop = 2176.0f;
        frustum.m_fBottom = -2176.0f;
        frustum.m_fFar = campos.z - heightextents.x + 10.0;
        NiCamera* apMemory = static_cast<NiCamera*>(NiMemObject::operator new(sizeof(NiCamera)));
        if (apMemory) {
            pCellCamera = ::new (apMemory) NiCamera;
        }
        else {
            pCellCamera = nullptr;
        }
        NiPointer<NiCamera> spcamera(pCellCamera);
        NiNode* v13 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
        if (v13) {
            v10 = ::new (v13) NiNode(0);
        }
        else {
            v10 = nullptr;
        }
        NiPointer<NiNode> spcameraNode(v10);
        NiNode* pscene_2 = spcameraNode.m_pObject;
        pscene_2->AttachChild(spcamera.m_pObject, 1);
        spcameraNode.m_pObject->SetTranslate(campos);
        NiNode* fYAngle = spcameraNode.m_pObject;
        pscene->AttachChild(fYAngle, 1);
        spcamera.m_pObject->SetRotate(camrot);
        spcamera.m_pObject->SetViewFrustum(frustum);
        NiUpdateData UpdateData1(0.0f, false, false);
        spcameraNode.m_pObject->Update(UpdateData1);
        NiUpdateData UpdateData2(0.0f, false, false);
        spcamera.m_pObject->Update(UpdateData2);
        BSTreeManager::SetForceFullLOD(true);
        BSTreeManager::Update(spcamera.m_pObject, true);
        UpdateTrees();
        bool bactorculled = false;
        bool bmarkerculled = false;
        bool blandculled = false;
        NiNode* pactornode = GetChildNode(TESObjectCELL::CN_ACTOR);
        if (pactornode) {
            bactorculled = pactornode->GetAppCulled();
            pactornode->SetAppCulled(true);
        }
        NiNode* pmarkernode = GetChildNode(TESObjectCELL::CN_MARKER);
        if (pmarkernode) {
            bmarkerculled = pmarkernode->GetAppCulled();
            pmarkernode->SetAppCulled(true);
        }
        NiNode* plandnode = GetChildNode(TESObjectCELL::CN_LAND);
        if (plandnode) {
            blandculled = plandnode->GetAppCulled();
            plandnode->SetAppCulled(true);
        }
        NiNode* pskynode = Sky::GetInstance()->GetRoot();
        bool bskyculled = false;
        if (pskynode) {
            bskyculled = pskynode->GetAppCulled();
            pskynode->SetAppCulled(true);
        }
        NiNode* pweathernode = Main::WeatherRootNode();
        bool bweatherculled = false;
        if (pskynode) {
            bweatherculled = pweathernode->GetAppCulled();
            pweathernode->SetAppCulled(true);
        }
        bool bgrassculled = false;
        NiAVObject* pgnode = TESTerrainLODManager::GetGrassNode();
        if (pgnode) {
            bgrassculled = pgnode->GetAppCulled();
            pgnode->SetAppCulled(true);
        }
        bool bfadeenabled = BSFadeNode::IsFadeEnabled();
        BSFadeNode::SetFadeEnabled(false);
        NiPointer<BSRenderedTexture> sptexture(static_cast<BSRenderedTexture*>(nullptr));
        TakeLocalMapPicture(spcamera.m_pObject, sptexture);
        BSFadeNode::SetFadeEnabled(bfadeenabled);
        if (pgnode) {
            pgnode->SetAppCulled(bgrassculled);
        }
        if (pweathernode) {
            pweathernode->SetAppCulled(bweatherculled);
        }
        if (pskynode) {
            pskynode->SetAppCulled(bskyculled);
        }
        if (pactornode) {
            pactornode->SetAppCulled(bactorculled);
        }
        if (pmarkernode) {
            pmarkernode->SetAppCulled(bmarkerculled);
        }
        if (plandnode) {
            plandnode->SetAppCulled(blandculled);
        }
        NiCamera* v6 = Main::WorldRootCamera();
        BSTreeManager::Update(v6, true);
        BSTreeManager::SetForceFullLOD(false);
        UpdateTrees();
        NiNode* fZAngle = spcameraNode.m_pObject;
        pscene->DetachChild(fZAngle);
        spcameraNode.operator=(static_cast<NiNode*>(nullptr));
        spcamera.operator=(static_cast<NiCamera*>(nullptr));
        aspRenderedTexture.operator=(sptexture);
    }
}

void TESObjectCELL::TakeInteriorLocalMapPicture(
    int32_t aiSectionX,
    int32_t aiSectionY,
    NiPointer<BSRenderedTexture>& aspRenderedTexture
) {
    NiMatrix3 camrot_2;
    NiMatrix3 rotmat;
    NiBound cellbound;
    NiMatrix3 camrot;
    if (GetInterior() && Get3D()) {
        NiNode* v9;
        void* v10;
        NiNode* pscene = (NiNode*)BSShaderManager::GetShadowSceneNode(0);
        NiPoint3 campos = NiPoint3_ZERO;
        float v12 = 3.1415927f / 2.0;
        float fXAngle = -3.1415927f / 2.0;
        camrot.FromEulerAnglesXYZ(fXAngle, fXAngle, v12);
        NiFrustum frustum(false);
        frustum.m_fNear = 100.0f;
        frustum.m_bOrtho = true;
        NiNode* v4 = Get3D();
        cellbound = v4->GetWorldBound();
        NiPoint3* Center = &(&cellbound)->m_kCenter;
        float ecx0 = Center->z - cellbound.m_fRadius;
        NiPoint3* v6 = &(&cellbound)->m_kCenter;
        float v37 = cellbound.m_fRadius + v6->z;
        int32_t iHalfMapWorldDim = 2176;
        campos.x = static_cast<float>((aiSectionX << 12) + 0x1080);
        campos.y = static_cast<float>((aiSectionY << 12) + 0x1080);
        campos.z = v37 + 40000.0;
        AdjustCoordForNorthRotation(campos, campos, false);
        frustum.m_fLeft = -2176.0f;
        frustum.m_fRight = 2176.0f;
        frustum.m_fTop = 2176.0f;
        frustum.m_fBottom = -2176.0f;
        frustum.m_fFar = campos.z - ecx0 + 10000.0;
        void* apMemory = NiMemObject::operator new(sizeof(NiCamera));
        if (apMemory) {
            v10 = (void*)::new (apMemory) NiCamera;
        }
        else {
            v10 = nullptr;
        }
        NiRefObject* v18 = static_cast<NiRefObject*>(v10);
        NiPointer<NiCamera> spcamera(static_cast<NiCamera*>(v10));
        NiNode* v15 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
        if (v15) {
            v9 = ::new (v15) NiNode(0);
        }
        else {
            v9 = nullptr;
        }
        NiRefObject* v16 = (NiRefObject*)v9;
        NiPointer<NiNode> spcameraNode(v9);
        NiNode* v8 = spcameraNode.m_pObject;
        v8->AttachChild(spcamera.m_pObject, 1);
        spcameraNode.m_pObject->SetTranslate(campos);
        pscene->AttachChild(spcameraNode.m_pObject, 1);
        float fnorthrotate = 0.0f;
        TESObjectREFR* pnorthmarkerref = SearchForm(pNorthMarker, 1);
        if (pnorthmarkerref) {
            fnorthrotate = (&pnorthmarkerref->GetAngleOnReference())->z;
        }
        if (fnorthrotate != 0.0) {
            rotmat = NiMatrix3_IDENTITY;
            rotmat.MakeZRotation(fnorthrotate);
            camrot = *::new (&camrot_2) NiMatrix3(rotmat * camrot);
        }
        spcamera.m_pObject->SetRotate(camrot);
        spcamera.m_pObject->SetViewFrustum(frustum);
        NiUpdateData UpdateData1(0.0f, false, false);
        spcameraNode.m_pObject->Update(UpdateData1);
        NiUpdateData UpdateData2(0.0f, false, false);
        spcamera.m_pObject->Update(UpdateData2);
        bool bactorculled = false;
        bool bmarkerculled = false;
        NiNode* pactornode = GetChildNode(TESObjectCELL::CN_ACTOR);
        if (pactornode) {
            bactorculled = pactornode->GetAppCulled();
            pactornode->SetAppCulled(true);
        }
        NiNode* pmarkernode = GetChildNode(TESObjectCELL::CN_MARKER);
        if (pmarkernode) {
            bmarkerculled = pmarkernode->GetAppCulled();
            pmarkernode->SetAppCulled(true);
        }
        NiNode* pportalnode = GetChildNode(TESObjectCELL::CN_PORTAL);
        NiNode* pwaternode = TESWaterSystem::GetWaterRoot();
        bool bwaterculled = false;
        if (pwaternode) {
            bwaterculled = pwaternode->GetAppCulled();
            pwaternode->SetAppCulled(true);
        }
        bool bfadeenabled = BSFadeNode::IsFadeEnabled();
        BSFadeNode::SetFadeEnabled(false);
        NiPointer<BSRenderedTexture> sptexture(static_cast<BSRenderedTexture*>(nullptr));
        TakeLocalMapPicture(spcamera.m_pObject, sptexture);
        BSFadeNode::SetFadeEnabled(bfadeenabled);
        if (pwaternode) {
            pwaternode->SetAppCulled(bwaterculled);
        }
        if (pactornode) {
            pactornode->SetAppCulled(bactorculled);
        }
        if (pmarkernode) {
            pmarkernode->SetAppCulled(bmarkerculled);
        }
        NiNode* angle = spcameraNode.m_pObject;
        pscene->DetachChild(angle);
        spcameraNode.operator=(static_cast<NiNode*>(nullptr));
        spcamera.operator=(static_cast<NiCamera*>(nullptr));
        aspRenderedTexture.operator=(sptexture);
    }
    else {
        aspRenderedTexture.operator=(static_cast<BSRenderedTexture*>(nullptr));
    }
}

void TESObjectCELL::DetachHavok() {
    if (GetInterior()) {
        bhkWorld* pw = GetbhkWorld();
        if (pw) {
            pw->SetEnabled(false);
            pw->SetVisualDebugger(false);
            if (pw->pAcousticSpaceListener) {
                BGSAcousticSpaceListener* pasl = pw->pAcousticSpaceListener;
                pasl->DequeueAllRemovals(true);
                pasl->bBufferRemovalOperations = false;
            }
        }
    }
    else {
        CellMopp* pcm = GetCellMopp();
        if (pcm) {
            pcm->Remove();
        }
        CellRefLockEnter();
        BSSimpleList<TESObjectREFR*>* plist = GetRefList();
        while (plist) {
            TESObjectREFR* pobj = plist->m_item;
            plist = plist->m_pkNext;
            if (pobj) {
                pobj->DetachHavok();
            }
        }
        bhkWorld* pw_2 = GetbhkWorld();
        if (pw_2) {
            pw_2->pAcousticSpaceListener->bBufferRemovalOperations = false;
        }
        if (pSaveLoadManager->IsLoadingGame()) {
            pw_2->ProcessBatchRemovals(false);
        }
        CellRefLockLeave();
    }
}

void TESObjectCELL::UpdateSeenData(const NiPoint3& aCoord) {
    NiPoint3 v34;
    NiPoint3 v35;
    NiPoint3 v36;
    NiPoint3 v37;
    NiPoint3 dst;
    if (GetInterior()) {
        NiPoint3 shiftcoord = aCoord;
        if (GetNorthRotation() == 0.0) {
            IntSeenData* pdata_2[4];
            NiPoint3 pstart_2[4];
            rt::eh_vector_constructor_iterator(pstart_2, sizeof(NiPoint3), 4, &emptyPointConstructor);
            ourMemset(pdata_2, 0, sizeof(pdata_2));
            ourMemset(pstart_2, 0, sizeof(pstart_2));
            int32_t isectionx_2 = (FloatToInt(shiftcoord.x) - 2048) >> 12;
            int32_t isectiony_2 = (FloatToInt(shiftcoord.y) - 2048) >> 12;
            pdata_2[0] = GetIntSeenSection(isectionx_2, isectiony_2, true);
            pstart_2[0] = *::new (&v37) NiPoint3(
                static_cast<float>((isectionx_2 << 12) + 2048), static_cast<float>((isectiony_2 << 12) + 2048),
                shiftcoord.z
            );
            NiPoint2 sectionmin(
                static_cast<float>((isectionx_2 << 12) + 2048), static_cast<float>((isectiony_2 << 12) + 2048)
            );
            NiPoint2 sectionmax(
                static_cast<float>(((isectionx_2 + 0x1000) << 12) + 2048),
                static_cast<float>(((isectiony_2 + 0x1000) << 12) + 2048)
            );
            int32_t ixshift = 0;
            int32_t iyshift = 0;
            float* v4 = &TESObjectCELL::fSeenDataUpdateRadius.Float();
            if (!(sectionmin.x >= shiftcoord.x - *v4)) {
                float* v5 = &TESObjectCELL::fSeenDataUpdateRadius.Float();
                if (sectionmax.x <= shiftcoord.x + *v5) {
                    ixshift = 1;
                }
            }
            else {
                ixshift = -1;
            }
            float* v6 = &TESObjectCELL::fSeenDataUpdateRadius.Float();
            if (!(sectionmin.y >= shiftcoord.y - *v6)) {
                float* v7 = &TESObjectCELL::fSeenDataUpdateRadius.Float();
                if (sectionmax.y <= shiftcoord.y + *v7) {
                    iyshift = 1;
                }
            }
            else {
                iyshift = -1;
            }
            int32_t iindex_2 = 1;
            if (ixshift) {
                NiPoint3* v8 = ::new (&v36) NiPoint3(
                    static_cast<float>(((ixshift + isectionx_2) << 12) + 2048),
                    static_cast<float>((isectiony_2 << 12) + 2048), shiftcoord.z
                );
                pstart_2[iindex_2] = *v8;
                IntSeenData* IntSeenSection = GetIntSeenSection(ixshift + isectionx_2, isectiony_2, true);
                pdata_2[iindex_2++] = IntSeenSection;
            }
            if (iyshift) {
                NiPoint3* v10 = ::new (&v35) NiPoint3(
                    static_cast<float>((isectionx_2 << 12) + 2048),
                    static_cast<float>(((iyshift + isectiony_2) << 12) + 2048), shiftcoord.z
                );
                pstart_2[iindex_2] = *v10;
                IntSeenData* v11 = GetIntSeenSection(isectionx_2, iyshift + isectiony_2, true);
                pdata_2[iindex_2++] = v11;
            }
            if ((ixshift) && (iyshift)) {
                NiPoint3* v12 = ::new (&v34) NiPoint3(
                    static_cast<float>(((ixshift + isectionx_2) << 12) + 2048),
                    static_cast<float>(((iyshift + isectiony_2) << 12) + 2048), shiftcoord.z
                );
                pstart_2[iindex_2] = *v12;
                IntSeenData* v13 = GetIntSeenSection(ixshift + isectionx_2, iyshift + isectiony_2, true);
                pdata_2[iindex_2++] = v13;
            }

            for (int32_t i_2 = 0; i_2 < 4 && pdata_2[i_2]; ++i_2) {
                pdata_2[i_2]->UpdateSeenData(shiftcoord, pstart_2[i_2]);
            }
        }
        else {
            IntSeenData* pdata[9];
            NiPoint3 pstart[9];
            AdjustCoordForNorthRotation(aCoord, shiftcoord, true);
            rt::eh_vector_constructor_iterator(pstart, sizeof(NiPoint3), 9, &emptyPointConstructor);
            ourMemset(pdata, 0, sizeof(pdata));
            ourMemset(pstart, 0, sizeof(pstart));
            int32_t isectionx = (FloatToInt(shiftcoord.x) - 2048) >> 12;
            int32_t isectiony = (FloatToInt(shiftcoord.y) - 2048) >> 12;
            int32_t iindex = 0;
            for (int32_t idx = -1; idx <= 1; ++idx) {
                for (int32_t idy = -1; idy <= 1; ++idy) {
                    IntSeenData* v2 = GetIntSeenSection(idx + isectionx, idy + isectiony, true);
                    pdata[iindex] = v2;
                    NiPoint3* v3 = ::new (&dst) NiPoint3(
                        static_cast<float>(((idx + isectionx) << 12) + 2048),
                        static_cast<float>(((idy + isectiony) << 12) + 2048), shiftcoord.z
                    );
                    pstart[iindex++] = *v3;
                }
            }
            for (int32_t i = 0; i < 9 && pdata[i]; ++i) {
                pdata[i]->UpdateSeenData(shiftcoord, pstart[i]);
            }
        }
    }
    else {
        if (!GetFullySeen()) {
            SeenData* pdata_3 = GetSeenData();
            if (!pdata_3) {
                SeenData* pdata_4;
                SeenData* apMemory = static_cast<SeenData*>(ourMalloc(sizeof(SeenData)));
                if (apMemory) {
                    pdata_4 = ::new (apMemory) SeenData;
                }
                else {
                    pdata_4 = nullptr;
                }
                pdata_3 = pdata_4;
                GetExtra()->SetSeenData(pdata_4);
                AddChange(CHANGE_CELL_SEENDATA);
            }
            float zb = aCoord.z;
            float yg = static_cast<float>(GetDataY() << 12);
            float xe = static_cast<float>(GetDataX() << 12);
            NiPoint3 cellstart(xe, yg, zb);
            if (pdata_3->UpdateSeenData(aCoord, cellstart)) {
                ClearSeenData();
                SetFullySeen(true);
            }
        }
    }
}

void TESObjectCELL::UpdateTrees() {
    CellRefLockEnter();
    for (TESObjectREFR* v5 : *GetRefList()) {
        if ((v5) && (!v5->HasTemp3D())) {
            TESBoundObject* ObjectReference = v5->GetObjectReference();
            if ((ObjectReference->GetFormType() == 37) && (v5->Get3D())) {
                BSTreeNode* v4 = static_cast<BSTreeNode*>(v5->Get3D());
                BSTreeModel* v2 = v4->GetModel();
                v2->Update(v4);
            }
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::RemoveNavmeshObstacles() {
    CellRefLockEnter();
    for (TESObjectREFR* apRef : *GetRefList()) {
        if (apRef) {
            if (apRef->IsObstacle()) {
                NavMeshObstacleManager* Instance = NavMeshObstacleManager::GetInstance();
                Instance->RemoveObstacleForReference(apRef);
            }
            TESBoundObject* ObjectReference = apRef->GetObjectReference();
            if (ObjectReference->GetFormType() == 28) {
                NavMeshObstacleManager* v3 = NavMeshObstacleManager::GetInstance();
                v3->OnDoorRemove(apRef);
            }
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::FadeToLowDetail() {
    if (bFadingToLowDetail) {
        bFadingToLowDetail = true;
    }
    bFadingToHighDetail = false;
    fLodFadeInPercent = TESObjectCELL::fLodFadeTime;
    bDisplayHighDetail = false;
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* pList = GetRefList();
    while (pList) {
        TESObjectREFR* pRef = pList->m_item;
        pList = pList->m_pkNext;
        if ((pRef) && (pRef != pPlayer)) {
            NiAVObject* pNav = pRef->TESObjectREFR::Get3D();
            if (pNav) {
                BSFadeNode* pfadenode = pNav->IsFadeNode();
                if ((pfadenode) && (pfadenode->GetMultType() == LOD_MULT_DISTANTLOD)) {
                    pfadenode->TurnFadeNodeOff();
                }
            }
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::SetPublic(bool abVal) {
    if (abVal) {
        cCellFlags |= 32;
    }
    else {
        cCellFlags &= ~32;
    }
    AddChange(2);
}

bhkWorldM* TESObjectCELL::ExteriorWorld() {
    return spExteriorWorld.m_pObject;
}

bool TESObjectCELL::IsNodeCulled(CELL_CULL_NODE aiNode) {
    return (iNodeCullFlags & (1u << (aiNode & 31))) != 0;
}

// Packs a cell's grid position into one value: X in the high half, Y in the low half. Only the low 16 bits of each
// coordinate are used.
int32_t TESObjectCELL::GetCoord(int32_t aiX, int32_t aiY) {
    return static_cast<int32_t>(static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(aiX))) << 16) |
        static_cast<uint16_t>(aiY);
}

uint32_t TESObjectCELL::GetHoursToRespawnCell() {
    return (&gs_iHoursToRespawnCell)->UInt();
}

void TESObjectCELL::KillStatics() {
    if (pIslandListener) {
        bhkWorld* pWorld = spExteriorWorld.operator->();
        pIslandListener->RemoveFromWorld(pWorld->operator*());
        delete pIslandListener;
        pIslandListener = nullptr;
    }
    spExteriorWorld = nullptr;
    TESWaterListener::ClearExteriorListener();
}

TESObjectCELL* TESObjectCELL::GetLastWaterCell() {
    return TESWaterSystem::pLastWaterCell;
}

bool TESObjectCELL::FindLandDataInFile(TESFile* apFile) {
    if (!apFile) {
        return false;
    }
    if (apFile->GetFormData()->form != formEnumString[57].uiFormString) {
        return false;
    }
    if (!apFile->NextForm(true)) {
        return false;
    }
    FORM* apFORMa = apFile->GetFormData();
    if (!TESObjectCELL::IsFormCellChild(apFORMa)) {
        return false;
    }
    if (!apFile->NextForm(true)) {
        return false;
    }
    FORM* apFORM = apFile->GetFormData();
    if (!TESObjectCELL::IsFormCellChild(apFORM)) {
        return false;
    }
    if (apFORM->form != formEnumString[2].uiFormString) {
        return false;
    }
    if (apFORM->iFormID == 8) {
        apFile->NextGroup();
        apFORM = apFile->GetFormData();
        if (!TESObjectCELL::IsFormCellChild(apFORM)) {
            return false;
        }
    }
    if (apFORM->iFormID != 9) {
        return false;
    }
    apFile->NextForm(true);
    bool v5 = false;
    while (true) {
        uint8_t TESForm_ = apFile->GetTESForm();
        if (!TESObjectCELL::IsFormTypeCellChild(TESForm_)) {
            break;
        }
        if (apFile->GetTESForm() == LAND_ID) {
            return true;
        }
        apFile->NextForm(true);
    }
    return v5;
}

void TESObjectCELL::DefaultRenderTestCallback(RENDER_FAILURE_DATA* apFailureData, void* apParamData) {
    SaveRenderFailureData(apFailureData);
}

void TESObjectCELL::SaveRenderFailureData(TESObjectCELL::RENDER_FAILURE_DATA* apFailureData) {
    // The cell of the last saved render failure: SaveRenderFailureData, its only
    // user, compares it with and stores RENDER_FAILURE_DATA::pTestCell, and clears it.
    static TESObjectCELL* pLastFailedCell;
    if (apFailureData) {
        BSFile* poutfile_2;
        float fy;
        float fz;
        char FileName[264];
        char apDest[268];
        float fx;
        BSSystemDir::MakeDir("RTF");
        const char* v18 = ".\\RTF\\";
        if (apFailureData->pTestCell->GetInterior()) {
            BSsprintf(FileName, 260, "%sINT-%s.txt", v18, apFailureData->pTestCell->GetFormEditorID());
        }
        else {
            TESWorldSpace* pworld = apFailureData->pTestCell->GetWorldSpace();
            if (*apFailureData->pTestCell->TESForm::GetFormEditorID()) {
                const char* v6 = apFailureData->pTestCell->GetFormEditorID();
                int32_t DataY = apFailureData->pTestCell->GetDataY();
                int32_t DataX = apFailureData->pTestCell->GetDataX();
                BSsprintf(FileName, 260, "%sEXT-%s-(%i, %i)-%s.txt", v18, pworld->GetFormEditorID(), DataX, DataY, v6);
            }
            else {
                int32_t v7 = apFailureData->pTestCell->GetDataY();
                int32_t v5 = apFailureData->pTestCell->GetDataX();
                BSsprintf(FileName, 260, "%sEXT-%s-(%i, %i).txt", v18, pworld->GetFormEditorID(), v5, v7);
            }
        }
        if (pLastFailedCell == apFailureData->pTestCell) {
            BSFile* poutfile_3;
            BSFile* v10 = static_cast<BSFile*>(ourMalloc(sizeof(BSFile)));
            if (v10) {
                poutfile_3 = ::new (v10) BSFile(FileName, NiFile::APPEND_ONLY, 0x4000u, false);
            }
            else {
                poutfile_3 = nullptr;
            }
            poutfile_2 = poutfile_3;
            poutfile_3->Open(false, false);
        }
        else {
            BSFile* poutfile;
            BSSystemFile::DeleteFileA(FileName);
            BSFile* apMemory = static_cast<BSFile*>(ourMalloc(sizeof(BSFile)));
            if (apMemory) {
                poutfile = ::new (apMemory) BSFile(FileName, NiFile::WRITE_ONLY, 0x4000u, false);
            }
            else {
                poutfile = nullptr;
            }
            poutfile_2 = poutfile;
            poutfile->Open(false, false);
            poutfile_2->Seek(0, NiFile::ms_iSeekSet);
            BSsprintf(
                apDest, 260, "cell ID\tcamera pos\tcamera rot\tgeom count\ttri count\tnew pass count\tactive lights\r\n"
            );
            uint32_t v1 = BSstrlen(apDest);
            poutfile_2->WriteF(apDest, v1);
            pLastFailedCell = apFailureData->pTestCell;
        }
        apFailureData->TestRot.ToEulerAnglesXYZ(fx, fy, fz);
        BSsprintf(
            apDest, 260, "(%08X)\t(%.2f, %.2f, %.2f)\t(%.2f, %.2f, %.2f)\t%i\t%i\t%i\t%i\t%i\r\n",
            apFailureData->pTestCell->iFormID, apFailureData->TestPos.x, apFailureData->TestPos.y,
            apFailureData->TestPos.z, fx * 57.2957763671875, fy * 57.2957763671875, fz * 57.2957763671875,
            apFailureData->RenderData.iTotalTime, apFailureData->RenderData.iGeometryCount,
            apFailureData->RenderData.iTriangleCount, apFailureData->RenderData.iNewRenderPassCount,
            apFailureData->RenderData.iActiveLights
        );
        uint32_t v2 = BSstrlen(apDest);
        poutfile_2->WriteF(apDest, v2);
        if (poutfile_2) {
            delete poutfile_2;
        }
    }
    else {
        pLastFailedCell = nullptr;
    }
}

void TESObjectCELL::UpdateExteriorWorld(float afTime) {
    if (spExteriorWorld.operator->()) {
        bhkWorld* pWorld = spExteriorWorld.operator->();
        pWorld->Update();
    }
}

// Each block coordinate is narrowed to 16 bits before the call.
uint32_t TESObjectCELL::CalcExtGroupBlockKey(int32_t aiX, int32_t aiY) {
    uint16_t usX = uint16_t(aiX >> 5);
    uint16_t usY = uint16_t(aiY >> 5);
    return GetCoord(usX, usY);
}

// Each sub-block coordinate is narrowed to 16 bits before the call.
uint32_t TESObjectCELL::CalcExtGroupSubBlockKey(int32_t aiX, int32_t aiY) {
    uint16_t usX = uint16_t(aiX >> 3);
    uint16_t usY = uint16_t(aiY >> 3);
    return GetCoord(usX, usY);
}

// The seen-data update radius is capped at 2048 (the cap is compared as the double 2048.0 and stored as the float).
void TESObjectCELL::InitStatics() {
    NiPoint3 kCenter = NiPoint3_ZERO;
    iNodeCullFlags = 0;
    spExteriorWorld = CreateWorld(kCenter);
    if (fSeenDataUpdateRadius.Float() > 2048.0) {
        fSeenDataUpdateRadius = 2048.0f;
    }
}

// Builds the havok world of a cell centred on arCenter (z dropped): the cinfo with the game's gravity and sizes, then
// the listeners the game hangs off it, the island listener (made once), the origin, a first step and the enable flags.
// The cinfo and the memory context are destroyed at the end, cinfo first. Only x, y and z of kWorldSize and kBorderSize
// are set, and bhkWorldM's constructor copies both whole, w included, into the world; this uninitialised read is
// deliberate, as the game does.
bhkWorldM* TESObjectCELL::CreateWorld(NiPoint3& arCenter) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_HAVOK, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 15387
    );
    bhkWorldCinfo kInfo;
    bhkWorldCinfo* pInfo = &kInfo;
    pInfo->m_gravity = hkVector4(0.0f, 0.0f, HavokWorldProps::fGravitySC, 0.0f);
    pInfo->setBroadPhaseWorldSize(NI2HK(147456.0f));
    NiPoint3 kCenter;
    kCenter.x = arCenter.x;
    kCenter.y = arCenter.y;
    kCenter.z = arCenter.z;
    kCenter.z = 0.0f;
    pInfo->m_collisionTolerance = 1.0f;
    hkVector4 kWorldSize;
    kWorldSize.operator()(0) = NI2HK(262144.0f);
    kWorldSize.operator()(1) = NI2HK(262144.0f);
    kWorldSize.operator()(2) = NI2HK(262144.0f);
    hkVector4 kBorderSize;
    kBorderSize.operator()(0) = NI2HK(63488.0f);
    kBorderSize.operator()(1) = NI2HK(63488.0f);
    kBorderSize.operator()(2) = NI2HK(63488.0f);
    void* pWorldMemory = bhkRagdollController::operator new(sizeof(bhkWorldM));
    bhkWorldM* pWorld = nullptr;
    if (pWorldMemory) {
        pWorld = ::new (pWorldMemory) bhkWorldM(*pInfo, &kWorldSize, &kBorderSize);
    }
    (**pWorld)->markForWrite();
    void* pMemory = ourMalloc(sizeof(TESWindListener));
    TESWindListener* pWind = nullptr;
    if (pMemory) {
        pWind = ::new (pMemory) TESWindListener;
    }
    pWorld->AddEntityListener(pWind);
    pMemory = ourMalloc(sizeof(TESTrapListener));
    TESTrapListener* pTrap = nullptr;
    if (pMemory) {
        pTrap = ::new (pMemory) TESTrapListener;
    }
    pWorld->AddTrapListener(pTrap);
    pWorld->AddWaterListener(TESWaterListener::GetExteriorListener());
    pMemory = ourMalloc(sizeof(BGSAcousticSpaceListener));
    BGSAcousticSpaceListener* pAcoustic = nullptr;
    if (pMemory) {
        pAcoustic = ::new (pMemory) BGSAcousticSpaceListener;
    }
    pWorld->AddAcousticSpaceListener(pAcoustic);
    pWorld->AddContactListener(FOCollisionListener::Instance());
    pMemory = ourMalloc(sizeof(TESArrowTargetListener));
    TESArrowTargetListener* pArrow = nullptr;
    if (pMemory) {
        pArrow = ::new (pMemory) TESArrowTargetListener;
    }
    pWorld->AddEntityListener(pArrow);
    if (!pIslandListener) {
        pMemory = ourMalloc(sizeof(FOIslandActivationListener));
        FOIslandActivationListener* pIsland = nullptr;
        if (pMemory) {
            pIsland = ::new (pMemory) FOIslandActivationListener;
        }
        pIslandListener = pIsland;
    }
    pIslandListener->AddToWorld(**pWorld);
    pWorld->SetOrigin(kCenter);
    (**pWorld)->markForWrite();
    static_cast<bhkWorld*>(pWorld)->Update();
    pWorld->SetEnabled(!TES::QCollisionDisableFlag());
    pWorld->SetVisualDebugger(false);
    return pWorld;
}

// Moves the exterior Havok world to arCenter, charging the work to the Havok memory context. The guard is built in
// place on the stack and destroyed at the end.
void TESObjectCELL::MoveExteriorWorld(NiPoint3& arCenter) {
    alignas(AutoMemContext) uint8_t aucMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (aucMemContext) AutoMemContext(
        MEM_CONTEXT_HAVOK, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp", 15451
    );
    spExteriorWorld.operator->()->CheckForMove(arCenter);
    pMemContext->~AutoMemContext();
}

// The semaphore that guards the queued attachments, constructed on first use.
void TESObjectCELL::QueueAttachChild(TESObjectREFR* apRef, NiNode* apParent) {
    if ((apRef) && (apRef->TESObjectREFR::Get3D())) {
        static BSSemaphore QueuedAttachSema = 1;
        QueuedAttachSema.Wait();
        NiAVObject* pnewchild = apRef->TESObjectREFR::Get3D();
        if (pnewchild->m_pkParent == apParent) {
            TESObjectCELL::QueuedChildAttachMap.RemoveAt(pnewchild);
        }
        else {
            TESObjectCELL::QUEUED_ATTACH attachdata;
            attachdata.spAttachNode = apParent;
            attachdata.spAttachChild = pnewchild;
            // The map takes the entry by value: the copy constructor builds it in the argument slot with Copy,
            // and the callee destroys it.
            TESObjectCELL::QueuedChildAttachMap.SetAt(pnewchild, attachdata);
        }
        QueuedAttachSema.Signal();
    }
}

void TESObjectCELL::PerformQueuedChildAttaches() {
    NiTMapItem<NiAVObject*, QUEUED_ATTACH>* pPos = QueuedChildAttachMap.GetFirstPos();
    while (pPos != nullptr) {
        NiAVObject* pChild = nullptr;
        QUEUED_ATTACH kAttach;
        QueuedChildAttachMap.GetNext(pPos, pChild, kAttach);
        if (kAttach.spAttachChild.operator->() != nullptr &&
            kAttach.spAttachChild->GetParent() != kAttach.spAttachNode.operator->()) {
            TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(kAttach.spAttachChild.operator->());
            if (pRef != nullptr && pRef->GetParentCell() != nullptr) {
                pRef->GetParentCell()->PerformCellNodeAttach(pRef, kAttach.spAttachNode.operator->());
            }
        }
    }
    QueuedChildAttachMap.RemoveAll();
}

void TESObjectCELL::SetLoadingTempData(bool abVal) {
    GetThreadLoadingTempData() = abVal;
}

bool& TESObjectCELL::GetThreadLoadingTempData() {
    static thread_local bool bLoadingTempData = false;
    return bLoadingTempData;
}

// After a particle-only update, a fade node whose world bound changed is retested against its cell's multibounds. The
// head's pointer is copied into the local and released at the end of each pass.
void TESObjectCELL::RunQueuedParticleUpdates() {
    QueuedParticleLock.Lock(nullptr);
    while (!QueuedParticleUpdates.IsEmpty()) {
        FNVD_INLINED_LOCAL(NiPointer<NiAVObject>, spObject, std::nullopt, std::nullopt);
        spObject.SetPointer(&QueuedParticleUpdates.GetItem());
        QueuedParticleUpdates.RemoveHead();
        UpdateParticlesOnly(spObject.operator->());
        if (spObject.operator->() != nullptr && spObject->IsFadeNode() != nullptr && spObject->GetWorldBoundChange()) {
            TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(spObject.operator->());
            if (pRef != nullptr && pRef->GetParentCell() != nullptr) {
                pRef->GetParentCell()->TestObjectAgainstMultibounds(pRef, true, false);
                spObject->SetWorldBoundChange(false);
            }
        }
        spObject.~NiPointer<NiAVObject>();
    }
    QueuedParticleLock.Unlock();
}

// A group header (record type GRUP) belongs when its group type, read where a record keeps its form ID, is 6, 8 or
// 9; a record belongs when its form type does.
bool TESObjectCELL::IsFormCellChild(FORM* apFORM) {
    bool bChild = false;
    if (apFORM != nullptr) {
        if (apFORM->form == formEnumString[GRUP_ID].uiFormString) {
            uint32_t iGroupType = apFORM->iFormID;
            if (iGroupType == 6 || (iGroupType > 7 && iGroupType <= 9)) {
                bChild = true;
            }
        }
        else {
            bChild = IsFormTypeCellChild(TESForm::GetFormTypeFromFormString(apFORM->form));
        }
    }
    return bChild;
}

// Returns true for REFR_ID..PFLA_ID, LAND_ID, NAVM_ID and PCBE_ID.
bool TESObjectCELL::IsFormTypeCellChild(uint8_t aeType) {
    switch (aeType) {
        case REFR_ID:
        case ACHR_ID:
        case ACRE_ID:
        case PMIS_ID:
        case PGRE_ID:
        case PBEA_ID:
        case PFLA_ID:
        case LAND_ID:
        case NAVM_ID:
        case PCBE_ID: return true;
        default: return false;
    }
}

void TESObjectCELL::UpdateParticlesOnly(NiAVObject* ap3D) {
    if (ap3D) {
        NiNode* pnode = ap3D->IsNode();
        if (pnode) {
            uint32_t icount = pnode->GetArrayCount();
            for (uint32_t i = 0; i < icount; ++i) {
                NiAVObject* v1 = pnode->GetAt(i);
                TESObjectCELL::UpdateParticlesOnly(v1);
            }
        }
        else {
            NiParticles* ppart = ap3D->IsParticlesGeom();
            if (ppart) {
                NiUpdateData UpdateData(fCellAppTime, true, false);
                ppart->Update(UpdateData);
            }
        }
    }
}

// Queues ap3D for a particle-only update on the main thread, under the queue's lock.
void TESObjectCELL::QueueParticleUpdate(NiAVObject* ap3D) {
    QueuedParticleLock.Lock(nullptr);
    {
        NiPointer<NiAVObject> sp3D(ap3D);
        QueuedParticleUpdates.AddHead(sp3D);
    }
    QueuedParticleLock.Unlock();
}

const char* TESObjectCELL::GetDefaultCellName() {
    return (&TESObjectCELL::sDefaultCellName)->String();
}

void TESObjectCELL::ClearOptimizationDataForCell(TESObjectCELL* apCell) {
    if (apCell) {
        if (apCell == pLastCheckedPointCell) {
            pLastCheckedPointCell = nullptr;
        }
        if (TESObjectCELL::GetLastWaterCell() == apCell) {
            TESWaterSystem::SetLastWaterCell(nullptr);
        }
    }
}

float TESObjectCELL::GetSeenDataUpdateRadius() {
    return (&TESObjectCELL::fSeenDataUpdateRadius)->Setting::Float();
}

bool TESObjectCELL::GetLoadingTempData() {
    return (bool)((uint8_t)TESObjectCELL::GetThreadLoadingTempData());
}

bool TESObjectCELL::FindReferenceInFile(TESFile* apFile, uint32_t aiRefID) {
    if (!apFile) {
        return false;
    }
    if (apFile->GetFormData()->form != formEnumString[57].uiFormString) {
        return false;
    }
    if (!apFile->NextForm(true)) {
        return false;
    }
    FORM* pFORM = apFile->GetFormData();
    if (pFORM->form != formEnumString[2].uiFormString) {
        return false;
    }
    uint32_t imaxsize = pFORM->length - 24;
    if (!apFile->NextForm(true)) {
        return false;
    }
    FORM* pFORM_2 = apFile->GetFormData();
    uint32_t auiFormID = aiRefID & 0xFFFFFF;
    bool bfound = false;
    uint32_t icheckedsize = 0;
    while (!bfound && icheckedsize < imaxsize) {
        if (pFORM_2->form == formEnumString[2].uiFormString) {
            icheckedsize += 24;
        }
        else {
            if ((apFile->GetID() & 0xFFFFFF) == auiFormID) {
                bfound = true;
            }
            else {
                icheckedsize += pFORM_2->length + 24;
            }
        }
        if (!bfound) {
            if (!apFile->NextForm(true)) {
                return false;
            }
            pFORM_2 = apFile->GetFormData();
        }
    }
    return bfound;
}

void TESObjectCELL::UpdateMorphControllersOnly(NiAVObject* ap3D) {
    NiNode* v1;
    uint32_t a1;
    uint32_t arrayCount;
    NiUpdateData apMemory;
    NiGeomMorpherController* pGeoMorphController;
    NiNode* v6;
    if (ap3D) {
        pGeoMorphController =
            static_cast<NiGeomMorpherController*>(ap3D->GetController(&NiGeomMorpherController::ms_RTTI));
        if (pGeoMorphController) {
            apMemory.~NiUpdateData();
            ::new (&apMemory) NiUpdateData(fCellAppTime, true, false);
            pGeoMorphController->Update(apMemory);
        }
        v6 = ap3D->IsNode();
        if (v6) {
            arrayCount = v6->GetArrayCount();
            for (a1 = 0; a1 < arrayCount; ++a1) {
                v1 = static_cast<NiNode*>(v6->GetAt(a1));
                UpdateMorphControllersOnly(v1);
            }
        }
    }
}

int32_t TESObjectCELL::GetSeenValue(const NiPoint3& aCoord) {
    int32_t iretval = 0;
    TESObjectCELL* pcell = pPlayer->GetParentCell();
    if (pcell) {
        if (pcell->GetInterior()) {
            int32_t isectx = (FloatToInt(aCoord.x) - 2048) >> 12;
            int32_t isecty = (FloatToInt(aCoord.y) - 2048) >> 12;
            int32_t iposx = 0;
            int32_t iposy = 0;
            SeenData::GetPosForCoord(aCoord, true, iposx, iposy);
            SeenData* pseen = pcell->GetIntSeenSection(isectx, isecty, false);
            if ((pseen) && (pseen->IsSeenBitTrue(static_cast<uint32_t>(iposx), static_cast<uint32_t>(iposy)))) {
                ++iretval;
            }

            SeenData* pnorthseen = nullptr;
            bool bnorth = false;
            if (iposy == 15) {
                bnorth = true;
                pnorthseen = pcell->GetIntSeenSection(isectx, isecty + 1, false);
                if ((pnorthseen) && (pnorthseen->IsSeenBitTrue(static_cast<uint32_t>(iposx), 0))) {
                    ++iretval;
                }
            }
            else {
                if ((pseen) && (pseen->IsSeenBitTrue(static_cast<uint32_t>(iposx), iposy + 1))) {
                    ++iretval;
                }
            }
            SeenData* peastseen = nullptr;
            bool beast = false;
            if (iposx == 15) {
                beast = true;
                peastseen = pcell->GetIntSeenSection(isectx + 1, isecty, false);
                if ((peastseen) && (peastseen->IsSeenBitTrue(0, static_cast<uint32_t>(iposy)))) {
                    ++iretval;
                }
            }
            else {
                if ((pseen) && (pseen->IsSeenBitTrue(iposx + 1, static_cast<uint32_t>(iposy)))) {
                    ++iretval;
                }
            }
            if (bnorth && beast) {
                SeenData* pnortheastseen = pcell->GetIntSeenSection(isectx + 1, isecty + 1, false);
                if ((pnortheastseen) && (pnortheastseen->IsSeenBitTrue(0, 0))) {
                    ++iretval;
                }
            }
            else {
                if (bnorth) {
                    if ((pnorthseen) && (pnorthseen->IsSeenBitTrue(iposx + 1, 0))) {
                        ++iretval;
                    }
                }
                else {
                    if (beast) {
                        if ((peastseen) && (peastseen->IsSeenBitTrue(0, iposy + 1))) {
                            ++iretval;
                        }
                    }
                    else {
                        if ((pseen) && (pseen->IsSeenBitTrue(iposx + 1, iposy + 1))) {
                            ++iretval;
                        }
                    }
                }
            }
        }
        else {
            TESWorldSpace* pworld = pcell->GetWorldSpace();
            if (pworld) {
                SeenData* pseen_2;
                int32_t isectx_2 = FloatToInt(aCoord.x) >> 12;
                int32_t isecty_2 = FloatToInt(aCoord.y) >> 12;
                int32_t iposx_2 = 0;
                int32_t iposy_2 = 0;
                SeenData::GetPosForCoord(aCoord, false, iposx_2, iposy_2);
                TESObjectCELL* pcenterCell = pworld->GetCellFromCellCoord(isectx_2, isecty_2);
                if (pcenterCell) {
                    pseen_2 = pcenterCell->GetSeenData();
                }
                else {
                    pseen_2 = nullptr;
                }
                if ((pseen_2 &&
                     pseen_2->IsSeenBitTrue(static_cast<uint32_t>(iposx_2), static_cast<uint32_t>(iposy_2))) ||
                    (pcenterCell && pcenterCell->GetFullySeen())) {
                    ++iretval;
                }
                SeenData* pnorthseen_2 = nullptr;
                TESObjectCELL* pnorthCell = nullptr;
                bool bnorth_2 = false;
                if (iposy_2 == 15) {
                    SeenData* pnorthseen_3;
                    bnorth_2 = true;
                    pnorthCell = pworld->GetCellFromCellCoord(isectx_2, isecty_2 + 1);
                    if (pnorthCell) {
                        pnorthseen_3 = pnorthCell->GetSeenData();
                    }
                    else {
                        pnorthseen_3 = nullptr;
                    }
                    pnorthseen_2 = pnorthseen_3;
                    if ((pnorthseen_3 && pnorthseen_2->IsSeenBitTrue(static_cast<uint32_t>(iposx_2), 0)) ||
                        (pnorthCell && pnorthCell->GetFullySeen())) {
                        ++iretval;
                    }
                }
                else {
                    if ((pseen_2 && pseen_2->IsSeenBitTrue(static_cast<uint32_t>(iposx_2), iposy_2 + 1)) ||
                        (pcenterCell && pcenterCell->GetFullySeen())) {
                        ++iretval;
                    }
                }
                SeenData* peastseen_2 = nullptr;
                TESObjectCELL* peastCell = nullptr;
                bool beast_2 = false;
                if (iposx_2 == 15) {
                    SeenData* peastseen_3;
                    beast_2 = true;
                    peastCell = pworld->GetCellFromCellCoord(isectx_2 + 1, isecty_2);
                    if (peastCell) {
                        peastseen_3 = peastCell->GetSeenData();
                    }
                    else {
                        peastseen_3 = nullptr;
                    }
                    peastseen_2 = peastseen_3;
                    if ((peastseen_3 && peastseen_2->IsSeenBitTrue(0, static_cast<uint32_t>(iposy_2))) ||
                        (peastCell && peastCell->GetFullySeen())) {
                        ++iretval;
                    }
                }
                else {
                    if ((pseen_2 && pseen_2->IsSeenBitTrue(iposx_2 + 1, static_cast<uint32_t>(iposy_2))) ||
                        (pcenterCell && pcenterCell->GetFullySeen())) {
                        ++iretval;
                    }
                }
                if (bnorth_2 && beast_2) {
                    SeenData* pnortheastseen_2;
                    pcell = pworld->GetCellFromCellCoord(isectx_2 + 1, isecty_2 + 1);
                    if (pcell) {
                        pnortheastseen_2 = pcell->GetSeenData();
                    }
                    else {
                        pnortheastseen_2 = nullptr;
                    }
                    if ((pnortheastseen_2 && pnortheastseen_2->IsSeenBitTrue(0, 0)) ||
                        (pcell && pcell->GetFullySeen())) {
                        ++iretval;
                    }
                }
                else {
                    if (bnorth_2) {
                        if ((pnorthseen_2 && pnorthseen_2->IsSeenBitTrue(iposx_2 + 1, 0)) ||
                            (pnorthCell && pnorthCell->GetFullySeen())) {
                            ++iretval;
                        }
                    }
                    else {
                        if (beast_2) {
                            if ((peastseen_2 && peastseen_2->IsSeenBitTrue(0, iposy_2 + 1)) ||
                                (peastCell && peastCell->GetFullySeen())) {
                                ++iretval;
                            }
                        }
                        else {
                            if ((pseen_2 && pseen_2->IsSeenBitTrue(iposx_2 + 1, iposy_2 + 1)) ||
                                (pcenterCell && pcenterCell->GetFullySeen())) {
                                ++iretval;
                            }
                        }
                    }
                }
            }
        }
    }
    return iretval;
}

int32_t TESObjectCELL::CreateCanopyShadowMaskForCell(
    TESObjectCELL* apCell,
    NiRenderedTexture*& apOutTex,
    _D3DLOCKED_RECT*& apOutRect
) {
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    apOutTex = nullptr;
    if (!apCell || (apCell->GetInterior() || !Renderer::GetNIRenderer())) {
        return 0;
    }
    NiRenderedTexture::bStaticForceD3DFormat = true;
    NiRenderedTexture::iStaticForcedD3DFormat = D3DFMT_L8;
    NiRenderedTexture::bStaticRenderedTexture = false;
    NiRenderer* Renderer_ = NiRenderer::GetRenderer();
    NiFixedString* name = ::new (static_cast<void*>(&apString)) NiFixedString("Canopy Shadow Mask");
    apOutTex = NiRenderedTexture::Create(
        *name, 64u, 64u, Renderer_, NiRenderedTexture::ms_kDefaultPrefs, Ni2DBuffer::MULTISAMPLE_NONE, nullptr, 0
    );
    apString.~NiFixedString();
    NiRenderedTexture::bStaticForceD3DFormat = false;
    NiRenderedTexture::bStaticRenderedTexture = true;
    apCell->GetExtra()->SetCanopyShadowMask(1, apOutTex, apOutRect);
    NiDX9DynamicTextureData* ptexdata = static_cast<NiDX9DynamicTextureData*>(apOutTex->m_pkRendererData);
    IDirect3DTexture9* pd3dtexture = static_cast<IDirect3DTexture9*>(ptexdata->GetD3DTexture());
    pd3dtexture->LockRect(0, apOutRect, nullptr, 0);
    _D3DLOCKED_RECT* v4 = apOutRect;
    int32_t icount = v4->Pitch * apOutTex->GetHeight();
    uint8_t* pbits = static_cast<uint8_t*>(apOutRect->pBits);
    for (int32_t i = 0; i < icount; ++i) {
        pbits[i] = 0;
    }
    return 1;
}

TESObjectREFR* TESObjectCELL::PickRandomTeleportDoorInSpace(
    TESForm* apDoorSpace,
    TESForm* apTargetSpace,
    TESObjectREFR** appOutBackUp
) {
    TESObjectREFR* pretdoor = nullptr;
    if (apDoorSpace && (apTargetSpace && (appOutBackUp && apDoorSpace != apTargetSpace))) {
        uint32_t idoorcount = 0;
        BSSimpleList<TESObjectREFR*> doorlist;
        BSSimpleList<TESObjectREFR*>* preflist = nullptr;
        void* v11 = nullptr;
        TESObjectCELL* pdoorcell = dynamic_cast<TESObjectCELL*>(apDoorSpace);
        TESObjectCELL* plockcell = nullptr;
        if (pdoorcell) {
            preflist = pdoorcell->GetRefList();
            plockcell = pdoorcell;
        }
        else {
            TESWorldSpace* pdoorworld = dynamic_cast<TESWorldSpace*>(apDoorSpace);
            if ((pdoorworld) && (pdoorworld->pPersistentCell)) {
                preflist = pdoorworld->pPersistentCell->GetRefList();
                plockcell = pdoorworld->pPersistentCell;
            }
        }
        plockcell->CellRefLockEnter();
        while (preflist && !preflist->IsEmpty()) {
            TESObjectREFR* pref = preflist->m_item;
            if (pref->GetObjectReference()->GetFormType() == DOOR_ID && !pref->GetDelete() && !pref->GetDisabled() &&
                !pref->GetDestroyed()) {
                TESObjectDOOR* pdoor = dynamic_cast<TESObjectDOOR*>(pref->GetObjectReference());
                if ((pdoor) &&
                    (pdoor->IsRandomDoor() && pdoor->RandomlyTeleportsTo(apTargetSpace) && !pref->GetTeleport())) {
                    doorlist.AddHead(pref);
                    ++idoorcount;
                }
            }
            preflist = preflist->m_pkNext;
        }
        plockcell->CellRefLockLeave();
        if (idoorcount && !doorlist.IsEmpty()) {
            int32_t itry = RandomInt(0, static_cast<int32_t>(idoorcount));
            int32_t iindex = 0;
            for (preflist = &doorlist; preflist && !preflist->IsEmpty(); preflist = preflist->m_pkNext) {
                if (iindex == itry) {
                    pretdoor = preflist->m_item;
                    break;
                }
                ++iindex;
            }
            doorlist.RemoveAll();
        }
    }
    return pretdoor;
}

void TESObjectCELL::UpdateSeenDataForPlayer() {
    static NiPoint3 pt(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f);
    if (pPlayer) {
        NiPoint3 a1 = *pPlayer->QLocationOnReference();
        SeenData::ShiftCoordToSeenPos(a1);
        if (a1.operator!=(pt)) {
            pt = a1;
            TESObjectCELL* ParentCell = pPlayer->GetParentCell();
            if (ParentCell) {
                if (ParentCell->GetInterior()) {
                    ParentCell->UpdateSeenData(a1);
                }
                else {
                    TESObjectCELL* pCells[4];
                    ourMemset(pCells, 0, sizeof(pCells));
                    pCells[0] = ParentCell;
                    TESWorldSpace* WorldSpace = ParentCell->GetWorldSpace();
                    if (WorldSpace) {
                        int32_t aiX = FloatToInt(a1.x) >> 12;
                        int32_t aiY = FloatToInt(a1.y) >> 12;
                        NiPoint2 v12(static_cast<float>(aiX << 12), static_cast<float>(aiY << 12));
                        float v10 = static_cast<double>(aiY << 12) + 4096.0;
                        float v9 = static_cast<double>(aiX << 12) + 4096.0;
                        NiPoint2 v14(v9, v10);
                        int32_t v18 = 0;
                        int32_t v17 = 0;
                        float* v0 = &TESObjectCELL::fSeenDataUpdateRadius.Float();
                        if (!(v12.x >= a1.x - *v0)) {
                            float* v1 = &TESObjectCELL::fSeenDataUpdateRadius.Float();
                            if (v14.x <= a1.x + *v1) {
                                v18 = 1;
                            }
                        }
                        else {
                            v18 = -1;
                        }
                        float* v2 = &TESObjectCELL::fSeenDataUpdateRadius.Float();
                        if (!(v12.y >= a1.y - *v2)) {
                            float* v3 = &TESObjectCELL::fSeenDataUpdateRadius.Float();
                            if (v14.y <= a1.y + *v3) {
                                v17 = 1;
                            }
                        }
                        else {
                            v17 = -1;
                        }
                        int32_t v13 = 1;
                        if (v18) {
                            TESObjectCELL* CellFromCellCoord = WorldSpace->GetCellFromCellCoord(v18 + aiX, aiY);
                            pCells[v13++] = CellFromCellCoord;
                        }
                        if (v17) {
                            TESObjectCELL* v5 = WorldSpace->GetCellFromCellCoord(aiX, v17 + aiY);
                            pCells[v13++] = v5;
                        }
                        if ((v18) && (v17)) {
                            TESObjectCELL* v6 = WorldSpace->GetCellFromCellCoord(v18 + aiX, v17 + aiY);
                            pCells[v13++] = v6;
                        }
                    }
                    for (int32_t i = 0; i < 4 && pCells[i]; ++i) {
                        pCells[i]->UpdateSeenData(a1);
                    }
                }
            }
        }
    }
}

void TESObjectCELL::RenderTestCell(
    TESObjectCELL* apCell,
    TESObjectCELL::RENDER_TEST_DATA* apTestData,
    bool abtestEntireCell
) {
    FNVD_INLINED_LOCAL(NavMeshPtr, spnavmesh_2, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NavMeshPtr, spnavmesh, std::nullopt, std::nullopt);
    NiMatrix3 oldrot;
    TESObjectCELL::RENDER_TEST_DATA defaultdata;
    if ((apCell) && (apCell->Get3D())) {
        bool bsuccess = true;
        defaultdata.~RENDER_TEST_DATA();
        ::new (&defaultdata) TESObjectCELL::RENDER_TEST_DATA;
        if (!apTestData) {
            apTestData = &defaultdata;
        }
        SceneGraph* pworldroot = Main::GetWorldSceneGraph();
        NiPoint3 oldpos = pworldroot->GetCameraRoot()->GetTranslate();
        oldrot = (*&pworldroot->GetCameraRoot()->m_kLocal).m_Rotate;
        if (abtestEntireCell) {
            NavMeshArray* parray = apCell->GetNavMeshArray();
            if (parray && parray->GetNavMeshCount()) {
                uint32_t itotaltricount = 0;
                for (int32_t i = 0;; ++i) {
                    uint32_t NavMeshCount = parray->GetNavMeshCount();
                    if (i >= NavMeshCount) {
                        break;
                    }
                    parray->GetNavMeshByIndex(&spnavmesh, static_cast<uint32_t>(i));
                    uint32_t itricount = spnavmesh.m_pObject->GetTriangleCount();
                    itotaltricount += itricount;
                    FNVD_DESTRUCT(NiPointer<NavMesh>)(&spnavmesh);
                }
                Warning("Testing %i NavMesh triangles.", itotaltricount);
                for (int32_t i_2 = 0;; ++i_2) {
                    uint32_t v5 = parray->GetNavMeshCount();
                    if (i_2 >= v5) {
                        break;
                    }
                    parray->GetNavMeshByIndex(&spnavmesh_2, static_cast<uint32_t>(i_2));
                    if (spnavmesh_2.operator NavMesh*()) {
                        uint32_t itricount_2 = spnavmesh_2.m_pObject->GetTriangleCount();
                        for (uint32_t j = 0; j < itricount_2; ++j) {
                            NiPoint3 testpos = spnavmesh_2.m_pObject->GetCenter(j);
                            testpos.z = testpos.z + 118.0;
                            if (!TESObjectCELL::RenderTestPos(apCell, testpos, apTestData)) {
                                bsuccess = false;
                            }
                        }
                    }
                    FNVD_DESTRUCT(NiPointer<NavMesh>)(&spnavmesh_2);
                }
            }
            else {
                Warning("Testing %i reference positions.", apCell->GetRefList()->ItemsInList());
                BSSimpleList<TESObjectREFR*>* plist_2 = apCell->GetRefList();
                while (plist_2 && !plist_2->IsEmpty()) {
                    TESObjectREFR* pref = plist_2->m_item;
                    plist_2 = plist_2->m_pkNext;
                    NiPoint3 testpos_2 = pref->GetLocationOnReference();
                    if (pref->Get3D()) {
                        NiNode* v6 = pref->Get3D();
                        testpos_2 = (&v6->GetWorldBound())->m_kCenter;
                    }
                    if (!TESObjectCELL::RenderTestPos(apCell, testpos_2, apTestData)) {
                        bsuccess = false;
                    }
                }
            }
        }
        else {
            NiNode* v7 = pPlayer->Get3D();
            if (!TESObjectCELL::RenderTestPos(apCell, v7->GetWorldTranslate(), apTestData)) {
                bsuccess = false;
            }
        }
        NiNode* CameraRoot = pworldroot->GetCameraRoot();
        CameraRoot->SetTranslate(oldpos);
        NiNode* v9 = pworldroot->GetCameraRoot();
        v9->SetRotate(oldrot);
        NiUpdateData UpdateData(0.0f, false, false);
        NiNode* v10 = pworldroot->GetCameraRoot();
        v10->Update(UpdateData);
        if (!bsuccess) {
            Warning("!!!RenderTestCell failed!!! (see file in RTF folder for more info)");
        }
    }
}

// Multiplies the Z rotation by the X rotation into a local and tests the view from aTestPos with it, for three pitches
// and eight headings.
bool TESObjectCELL::RenderTestPos(
    TESObjectCELL* apCell,
    const NiPoint3& aTestPos,
    TESObjectCELL::RENDER_TEST_DATA* apTestData
) {
    bool bsuccess = true;
    int32_t ixangle = 45;
    for (int32_t i = 0; i < 3; ++i) {
        NiMatrix3 xrot;
        float v5 = static_cast<double>(ixangle) * 0.01745329238474369;
        xrot.MakeXRotation(v5);
        ixangle -= 45;
        int32_t izangle = 0;
        for (int32_t j = 0; j < 8; ++j) {
            NiMatrix3 zrot;
            float v4 = static_cast<double>(izangle) * 0.01745329238474369;
            zrot.MakeZRotation(v4);
            izangle += 45;
            NiMatrix3 kTestRot = zrot * xrot;
            if (!TESObjectCELL::RenderTestPosRot(apCell, aTestPos, kTestRot, apTestData)) {
                bsuccess = false;
            }
        }
    }
    return bsuccess;
}

bool TESObjectCELL::RenderTestPosRot(
    TESObjectCELL* apCell,
    const NiPoint3& aTestPos,
    const NiMatrix3& aTestRot,
    TESObjectCELL::RENDER_TEST_DATA* apTestData
) {
    TESObjectCELL::RENDER_FAILURE_DATA faildata;
    SceneGraph* pworldroot = Main::GetWorldSceneGraph();
    NiNode* CameraRoot = pworldroot->GetCameraRoot();
    CameraRoot->SetTranslate(aTestPos);
    NiNode* v5 = pworldroot->GetCameraRoot();
    v5->SetRotate(aTestRot);
    NiUpdateData UpdateData(0.0f, false, false);
    NiNode* v6 = pworldroot->GetCameraRoot();
    v6->Update(UpdateData);
    uint32_t istarttime = BSGetTickCount();
    appMain->Swap();
    uint32_t iignore = 0;
    faildata.~RENDER_FAILURE_DATA();
    ::new (&faildata) TESObjectCELL::RENDER_FAILURE_DATA;
    faildata.RenderData.iTotalTime = BSGetTickCount() - istarttime;
    NiRenderer* Renderer_ = NiRenderer::GetRenderer();
    static_cast<BSShaderAccumulator*>(Renderer_->GetSorter())
        ->BSShaderAccumulator::GetDebugStats(
            faildata.RenderData.iGeometryCount, iignore, faildata.RenderData.iTriangleCount, iignore, iignore, iignore,
            iignore, iignore, iignore, iignore, iignore
        );
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
    faildata.RenderData.iActiveLights = ShadowSceneNode_->GetActiveLightCount();
    bool bfailed = false;
    if (faildata.RenderData.iGeometryCount <= apTestData->Thresholds.iGeometryCount) {
        if (faildata.RenderData.iTriangleCount <= apTestData->Thresholds.iTriangleCount) {
            if (faildata.RenderData.iNewRenderPassCount <= apTestData->Thresholds.iNewRenderPassCount) {
                bfailed = faildata.RenderData.iActiveLights > apTestData->Thresholds.iActiveLights ||
                    (faildata.RenderData.iAccumulateTime > apTestData->Thresholds.iAccumulateTime ||
                     faildata.RenderData.iTotalTime > apTestData->Thresholds.iTotalTime);
            }
            else {
                bfailed = true;
            }
        }
        else {
            bfailed = true;
        }
    }
    else {
        bfailed = true;
    }
    if ((bfailed) && (apTestData->pFailureCallbackFunc)) {
        faildata.pTestCell = apCell;
        faildata.TestPos = aTestPos;
        faildata.TestRot = aTestRot;
        apTestData->pFailureCallbackFunc(&faildata, apTestData->pFailureCallbackData);
    }

    return !bfailed;
}

// The second guarded static, extcoord, is a NiPoint3 whose constructor is a trivial accessor: its guard is set and
// nothing else is written.
bool TESObjectCELL::EnumReferencesCloseToPointFromInterior(
    TESObjectCELL* apStartInt,
    const NiPoint3& aPoint1,
    float afRadius1,
    const NiPoint3& aPoint2,
    float afRadius2,
    bool (*apfnEnumFunc)(TESObjectREFR*, void*),
    void* aiEnumData
) {
    TESObjectCELL* ptelint;
    TESObjectREFR* pref;
    if (apStartInt) {
        apStartInt->CellRefLockEnter();
    }
    static NiTPointerList<TESObjectCELL*> checkedcells;
    bool bdone = false;
    if (!icallsdeep) {
        extcoord = NiPoint3_ZERO;
        pextworld = nullptr;
        fextdoordist = 3.4028235e+38f;
    }
    if ((apStartInt) && (apfnEnumFunc)) {
        ++icallsdeep;
        checkedcells.AddTail(apStartInt);
        NiTList<TESObjectREFR*> listTeleportDoors;
        BSSimpleList<TESObjectREFR*>* plist = apStartInt->GetRefList();
        while (plist && !bdone) {
            pref = plist->m_item;
            plist = plist->m_pkNext;
            if (pref) {
                const NiPoint3* v7;
                const NiPoint3* v18;
                float afRadius;
                if (afRadius1 == 3.4028234663852886e+38 ||
                    (afRadius = afRadius1,
                     (v18 = &aPoint1,
                      (v7 = &pref->GetLocationOnReference(),
                       NiPoint3::IsDistanceLongerThanRadius(v7, v18, afRadius) < 0)))) {
                    const NiPoint3* v8;
                    const NiPoint3* v19;
                    float afRadiusa;
                    if (afRadius2 == 3.4028234663852886e+38 ||
                        ((afRadius2 == afRadius1 && aPoint1.operator==(aPoint2)) ||
                         (afRadiusa = afRadius2,
                          (v19 = &aPoint2,
                           (v8 = &pref->GetLocationOnReference(),
                            NiPoint3::IsDistanceLongerThanRadius(v8, v19, afRadiusa) < 0))))) {
                        if (apfnEnumFunc(pref, aiEnumData)) {
                            bdone = true;
                        }
                        if (pref->GetObjectReference()->GetFormType() == DOOR_ID) {
                            TESObjectDOOR* ObjectReference = static_cast<TESObjectDOOR*>(pref->GetObjectReference());
                            if (ObjectReference != pJailMarker) {
                                DoorTeleportData* ptel = pref->GetTeleport();
                                if (ptel) {
                                    TESWorldSpace* ptelworld = ptel->GetTeleportWorldSpace();
                                    ptelint = ptel->GetTeleportCell();
                                    if ((ptelint) && (!ptelint->GetInterior())) {
                                        ptelint = nullptr;
                                    }

                                    if (ptelworld) {
                                        float fmindoordist = 3.4028235e+38f;
                                        if (afRadius1 != 3.4028234663852886e+38) {
                                            float fdoordist = afRadius1 - pref->GetDistanceFromPoint(aPoint1);
                                            if (fmindoordist > fdoordist) {
                                                fmindoordist = fdoordist;
                                            }
                                        }
                                        if (afRadius2 != 3.4028234663852886e+38 && afRadius1 != afRadius2) {
                                            float fdoordist_2 = afRadius2 - pref->GetDistanceFromPoint(aPoint2);
                                            if (fmindoordist > fdoordist_2) {
                                                fmindoordist = fdoordist_2;
                                            }
                                        }
                                        if (!pextworld || fextdoordist < fmindoordist) {
                                            fextdoordist = fmindoordist;
                                            extcoord = ptel->position;
                                            pextworld = ptelworld;
                                        }
                                    }
                                    else {
                                        if ((ptelint) && (!checkedcells.FindPos(ptelint, nullptr))) {
                                            listTeleportDoors.AddTail(pref);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        NiTListItem<TESObjectREFR*>* pos = listTeleportDoors.m_pkHead;
        while (pos && !bdone) {
            TESObjectREFR* pref_2 = listTeleportDoors.GetNext(pos);
            if (pref_2) {
                float fmindoordist_2 = 3.4028235e+38f;
                if (afRadius1 != 3.4028234663852886e+38) {
                    float fdoordist_3 = afRadius1 - pref_2->GetDistanceFromPoint(aPoint1);
                    if (fmindoordist_2 > fdoordist_3) {
                        fmindoordist_2 = fdoordist_3;
                    }
                }
                if (afRadius2 != 3.4028234663852886e+38 && afRadius1 != afRadius2) {
                    float fdoordist_4 = afRadius2 - pref_2->GetDistanceFromPoint(aPoint2);
                    if (fmindoordist_2 > fdoordist_4) {
                        fmindoordist_2 = fdoordist_4;
                    }
                }
                DoorTeleportData* ptel_2 = pref_2->GetTeleport();
                NiPoint3* TeleportPosition = &ptel_2->position;
                TESObjectCELL* ptelint_2 = ptel_2->GetTeleportCell();
                if (!TESObjectCELL::EnumReferencesCloseToPointFromInterior(
                        ptelint_2, *TeleportPosition, fmindoordist_2, aPoint2, 3.4028235e+38f, apfnEnumFunc, aiEnumData
                    )) {
                    bdone = true;
                }
            }
        }
        --icallsdeep;
    }

    if (!icallsdeep) {
        checkedcells.RemoveAll();
        if ((!bdone) && (pextworld)) {
            pextworld->EnumReferencesCloseToPoint(
                extcoord, fextdoordist, aPoint2, 3.4028235e+38f, apfnEnumFunc, aiEnumData
            );
            bdone = true;
        }
    }
    if (apStartInt) {
        apStartInt->CellRefLockLeave();
    }
    return !bdone;
}

void* ourMemalign(uint8_t acAlignment, size_t aSize) {
    MemoryManager* v2 = &MemoryManager::Instance();
    uint8_t* pmem = static_cast<uint8_t*>(v2->Allocate(aSize + acAlignment));
    // The padding is the alignment less the address's offset within it, from the address's low byte.
    uint8_t v5 = acAlignment - (static_cast<uint8_t>(reinterpret_cast<uintptr_t>(pmem)) & acAlignment - 1);
    pmem[v5 - 1] = v5;
    return (void*)&pmem[v5];
}

bool bhkRagdollController::QRagdollAnim() {
    return bhkRagdollController::bRagdollAnim.Bool();
}

// The threading-check mark is empty in the game's build, so MarkForWrite and UnMarkForWrite compile to one function.
void bhkWorld::MarkForWrite() {
    ahkpWorld* pw = operator*();
    if (pw) {
        pw->markForWrite();
    }
}

void bhkWorld::UnMarkForWrite() {
    ahkpWorld* pw = operator*();
    if (pw) {
        pw->unmarkForWrite();
    }
}

void bhkWorld::AddEntityListener(bhkEntityListener* apListener) {
    ahkpWorld* pw = operator*();
    if (pw) {
        MarkForWrite();
        pw->addEntityListener(apListener);
        UnMarkForWrite();
    }
    pEntityListenerA.pushBack(static_cast<hkpEntityListener*>(apListener));
}

void bhkWorld::AddContactListener(hkpContactListener* apListener) {
    ahkpWorld* pw = operator*();
    if (pw) {
        MarkForWrite();
        pw->addContactListener(apListener);
        UnMarkForWrite();
    }
}

// The listener hears entities through the entity listeners and phantoms through its hkpPhantomListener base.
void bhkWorld::AddTrapListener(bhkTrapListener* apListener) {
    AddEntityListener(apListener);
    ahkpWorld* pw = operator*();
    if (pw) {
        MarkForWrite();
        pw->addPhantomListener(apListener);
        UnMarkForWrite();
    }
    pTrapListener = apListener;
}

float BSFadeNode::GetBoundRadius() {
    return fBoundRadius;
}

TESForm* BGSEncounterZone::QOwnerForm() {
    return Data.pZoneOwner;
}

CheckWithinMultiBoundTask::CheckWithinMultiBoundTask(TESObjectREFR* apBoundRef, NiAVObject* ap3D)
  : IOTask(IO_TASK_PRIORITY_LOW), pBoundRef(apBoundRef), sp3D(ap3D), spBound(nullptr), bWithin(false) {
    spBound = apBoundRef->GetMultiBound();
}

CheckWithinMultiBoundTask::~CheckWithinMultiBoundTask() {}

void CheckWithinMultiBoundTask::operator delete(void* apMemory) {
    ourFree(apMemory);
}

TESWindListener::TESWindListener() {}

TESWindListener::~TESWindListener() {}

// A random value between minus and plus afValue.
float TESWindListener::GetRandom(float afSize) {
    return RandomFloat(-afSize, afSize);
}

void TESWindListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}

uint32_t NavMesh::GetTriangleCount() {
    return Triangles.GetSize();
}

NiPoint3 NavMesh::GetCenter(uint16_t apTriangle) {
    NavMeshTriangle* pTriangle = GetTriangle(apTriangle);
    NiPoint3 kCenter = GetVertex(pTriangle->GetVert(0))->Location;
    kCenter += GetVertex(pTriangle->GetVert(1))->Location;
    kCenter += GetVertex(pTriangle->GetVert(2))->Location;
    kCenter *= 0.333333f;
    return static_cast<const NiPoint3&>(kCenter);
}

TESWeather* TESRegion::GetCurrentWeather() {
    return pCurrentWeather;
}

hkVector4& NI2HKNS(hkVector4& arhkV, const NiPoint3& arNiP) {
    arhkV.operator()(0) = arNiP.x;
    arhkV.operator()(1) = arNiP.y;
    arhkV.operator()(2) = arNiP.z;
    arhkV.operator()(3) = 0.0;
    return arhkV;
}

bool TESForm::GetReflectedByAutoWater() {
    return ((iFormFlags & 268435456) != 0);
}

bool TESForm::GetRefractedByAutoWater() {
    return ((iFormFlags & 536870912) != 0);
}

bool TESObjectLIGH::GetNoFlicker() {
    return !GetFlicker() && !GetFlickerSlow() && !GetPulse() && !GetPulseSlow();
}

uint32_t TESObjectLIGH::GetColor() {
    return data.color;
}

bool TESObjectREFR::HasRandomSound() {
    TESSound::SOUND_DATA result;
    return pRandomSound && ((&(result = pRandomSound->GetData()))->iFlags & 2) != 0;
}

const char* TESWorldSpace::GetCanopyShadowTextureName() {
    return CanopyShadowTexture.GetTextureName();
}

// The constructor deliberately leaves the sphere unset and ignores both arguments, as the game does.
BSNiBound::BSNiBound(uint16_t asFlags, uint16_t asLevel) {}

// An object without a world bound gets a zeroed BSNiBound (flags 0x100, level 0), set through SetBound.
void NiAVObject::CreateBoundIfNone() {
    if (m_kWorldBound) {
        return;
    }
    void* pMemory = ourMalloc(sizeof(BSNiBound));
    BSNiBound* pkBound = pMemory ? ::new (pMemory) BSNiBound(0x100, 0) : nullptr;
    SetBound(pkBound);
    m_kWorldBound->SetCenterAndRadius(NiPoint3_ZERO, 0.0f);
}

NiAccumulator* NiRenderer::GetSorter() {
    return m_spAccum.operator->();
}
