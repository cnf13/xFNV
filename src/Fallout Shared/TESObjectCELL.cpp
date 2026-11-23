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
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace HavokWorldProps {} // namespace HavokWorldProps

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
    float fDX = afX1 - afX2;
    float fDY = afY1 - afY2;
    float fDXSqr = fDX * fDX;
    float fDYSqr = fDY * fDY;
    float fDistSqr = fDXSqr + fDYSqr;
    return std::sqrt(fDistSqr);
}

void SyncFromHavok(NiNode* apNode) {
    for (uint32_t i = 0; i < apNode->GetArrayCount(); ++i) {
        NiNode* pChild = static_cast<NiNode*>(apNode->GetAt(i));
        if (pChild) {
            Actor* pActorNode = static_cast<Actor*>(TESObjectREFR::FindReferenceFor3D(pChild));
            if (pActorNode &&
                pActorNode->IsActor() &&
                pActorNode->pRagdollController &&
                pActorNode->pRagdollController->GetRagdollAnimEnable() &&
                !pActorNode->pRagdollController->bUsingMappers &&
                !static_cast<uint8_t>(pChild->GetNotVisible()) &&
                !pActorNode->IsInDialoguewithPlayer()) {
                bhkNiCollisionObject::Synchronize(pChild, bhkNiCollisionObject::SYNC_FROMHAVOK);
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

TESObjectCELL::QUEUED_ATTACH* TESObjectCELL::QUEUED_ATTACH::Copy(const TESObjectCELL::QUEUED_ATTACH* apSource) {
    spAttachChild.SetPointer(&apSource->spAttachChild);
    spAttachNode.SetPointer(&apSource->spAttachNode);
    bFirstAvail = apSource->bFirstAvail;
    return this;
}

TESObjectCELL::QUEUED_ATTACH& TESObjectCELL::QUEUED_ATTACH::operator=(const TESObjectCELL::QUEUED_ATTACH& __that) {
    spAttachChild = __that.spAttachChild;
    spAttachNode = __that.spAttachNode;
    bFirstAvail = __that.bFirstAvail;
    return *this;
}

TESObjectCELL::TESObjectCELL()
  : cCellFlags(0), cCellGameFlags(0), cCellState(0), pCellData(nullptr), pCellLand(nullptr),
    fWaterHeight(std::numeric_limits<float>::max()), bAutoWaterLoaded(false), pNavMeshes(nullptr),
    iCriticalQueuedRefCount(0), iQueuedRefCount(0), sNumRefsWithVisibleDistant(0), sNumLoadedRefsWithVisibleDistant(0),
    spLightMarkerNode(nullptr), spSoundMarkerNode(nullptr), spMultiBoundNode(nullptr), iTempDataOffset(0),
    pLoadedData(nullptr), fLodFadeInPercent(0.0f), bLODFadingIn(false), bFadedIn(false), bFadingToHighDetail(false),
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
    ExtraData.RemoveNonPersistentCellData();
    GetInterior();
    ourFree(pCellData);
    pCellData = nullptr;
    SetLightingTemplate(nullptr);
    iLightingTemplateInheritanceFlags = 0;
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
                // A 16-byte aligned stack buffer sized by the chunk, but the read is capped at 0x200 bytes.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
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
                    SetWaterHeight(std::numeric_limits<float>::max());
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
                    char* pTextureName = static_cast<char*>(XFNV_ALLOCA16(uiSize));
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
                    pImpactSwap = new ImpactSwap;
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
    FORM kGroupData;
    if (GetDelete()) {
        return false;
    }
    bool bChanged = false;
    CellRefLockEnter();
    if (apFile->GetMaster() || GetAltered()) {
        bChanged = true;
    }
    else {
        BSSimpleList<TESObjectREFR*>* pIter = GetRefList();
        while (pIter && !pIter->IsEmpty()) {
            TESForm* pRef = pIter->m_item;
            pIter = pIter->m_pkNext;
            TESFile* pFile = pRef->GetFile(-1);
            if (pFile == apFile || !pFile || pRef->GetAltered()) {
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
    kGroupData.form = formEnumString[2].uiFormString;
    kGroupData.iFormID = 6;
    kGroupData.flags = GetFormID();
    kGroupData.length = 0;
    kGroupData.iVersionControl = 0;
    apFile->StartGroup(&kGroupData);
    if (!apFile->GetMaster() || GetFile(0) == apFile) {
        SaveReferences(apFile);
    }
    return true;
}

void TESObjectCELL::Save() {
    uint32_t uiID;
    StartForm();
    TESFullName::Save();
    TESForm::AddChunk(DATA_ID, cCellFlags);
    if (GetInterior()) {
        INTERIOR_DATA* pInteriorData = GetInteriorData();
        if (TESForm::QEndianSwapOnSave()) {
            pInteriorData->Endian();
        }
        TESForm::__AddChunkData(XCLL_ID, pInteriorData, 44u);
        if (TESForm::QEndianSwapOnSave()) {
            pInteriorData->Endian();
        }
        ImpactSwap* pSwap = GetExtra()->GetImpactSwap();
        if (pSwap) {
            pSwap->Save();
        }
    }
    else {
        EXTERIOR_DATA* pExteriorData = GetExteriorData();
        if (TESForm::QEndianSwapOnSave()) {
            pExteriorData->Endian();
        }
        TESForm::__AddChunkData(XCLC_ID, pExteriorData, 12u);
        if (TESForm::QEndianSwapOnSave()) {
            pExteriorData->Endian();
        }
    }
    if (GetLightingTemplate()) {
        uiID = GetLightingTemplate()->iFormID;
    }
    else {
        uiID = 0;
    }
    TESForm::AddChunk(LTMP_ID, uiID);
    TESForm::AddChunk(LNAM_ID, iLightingTemplateInheritanceFlags);
    TESForm::AddChunk(XCLW_ID, std::bit_cast<uint32_t>(fWaterHeight));
    TESForm::AddChunkArray(XNAM_ID, WaterNoiseTexture.GetTextureName(), WaterNoiseTexture.GetTextureNameLength() + 1);
    ExtraData.Save();
    CloseForm();
}

bool TESObjectCELL::SaveEdit(TESFile* apFile) {
    FORM kGroupData;
    Save();
    apFile->AddTESForm(this);
    kGroupData.form = formEnumString[2].uiFormString;
    kGroupData.iFormID = 6;
    kGroupData.flags = GetFormID();
    kGroupData.length = 0;
    kGroupData.iVersionControl = 0;
    apFile->StartGroup(&kGroupData);
    SaveReferences(apFile);
    return true;
}

bool TESObjectCELL::SavesBefore(TESForm* apForm) {
    bool bRetVal = false;
    switch (apForm->GetFormType()) {
        case CELL_ID: {
            TESObjectCELL* pOtherCell = dynamic_cast<TESObjectCELL*>(apForm);
            if (pOtherCell) {
                if (!GetInterior() || pOtherCell->GetInterior()) {
                    int32_t iInterior = GetInterior();
                    if (iInterior == pOtherCell->GetInterior()) {
                        bool bCheckBlocks = true;
                        if (!GetInterior()) {
                            TESWorldSpace* pWorld = GetWorldSpace();
                            TESWorldSpace* pOtherWorld = pOtherCell->GetWorldSpace();
                            if (pWorld == pOtherWorld) {
                                int32_t iPersistentCell = GetPersistentCell();
                                if (iPersistentCell != pOtherCell->GetPersistentCell()) {
                                    bRetVal = GetPersistentCell();
                                    bCheckBlocks = false;
                                }
                            }
                            else {
                                bRetVal = pWorld->SavesBefore(pOtherWorld);
                                bCheckBlocks = false;
                            }
                        }
                        if (bCheckBlocks) {
                            uint32_t uiBlockKey = GetGroupBlockKey();
                            if (uiBlockKey >= pOtherCell->GetGroupBlockKey()) {
                                uiBlockKey = GetGroupBlockKey();
                                if (uiBlockKey == pOtherCell->GetGroupBlockKey()) {
                                    uint32_t uiSubBlockKey = GetGroupSubBlockKey();
                                    if (uiSubBlockKey >= pOtherCell->GetGroupSubBlockKey()) {
                                        uiSubBlockKey = GetGroupSubBlockKey();
                                        if (uiSubBlockKey == pOtherCell->GetGroupSubBlockKey()) {
                                            uint32_t uiID = iFormID;
                                            if (uiID < pOtherCell->iFormID) {
                                                bRetVal = true;
                                            }
                                        }
                                    }
                                    else {
                                        bRetVal = true;
                                    }
                                }
                            }
                            else {
                                bRetVal = true;
                            }
                        }
                    }
                }
                else {
                    bRetVal = true;
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
            TESChildCell* pCellChild = dynamic_cast<TESChildCell*>(apForm);
            if (pCellChild->GetSaveParentCell() == this) {
                bRetVal = true;
            }
            else {
                bRetVal = SavesBefore(pCellChild->GetSaveParentCell());
            }
            break;
        }
        case WRLD_ID: {
            if (GetInterior()) {
                bRetVal = true;
            }
            else {
                TESWorldSpace* pWorld = GetWorldSpace();
                bRetVal = pWorld->SavesBefore(apForm);
            }
            break;
        }
        default: {
            bRetVal = TESForm::SavesBefore(apForm);
            break;
        }
    }
    return bRetVal;
}

bool TESObjectCELL::SavesBefore(FORM* apGroupFORM) {
    bool bRetVal = false;
    if (apGroupFORM && apGroupFORM->form == formEnumString[2].uiFormString) {
        switch (apGroupFORM->iFormID) {
            case 0u: {
                bRetVal = TESForm::SavesBefore(apGroupFORM);
                break;
            }
            case 1u: {
                if (GetInterior()) {
                    bRetVal = true;
                }
                else {
                    TESForm* pForm = TESForm::GetFormByNumericID(apGroupFORM->flags);
                    TESWorldSpace* pWorld = dynamic_cast<TESWorldSpace*>(pForm);
                    if (pWorld) {
                        bRetVal = SavesBefore(pWorld);
                    }
                }
                break;
            }
            case 2u: {
                if (GetInterior()) {
                    bRetVal = GetGroupBlockKey() < apGroupFORM->flags;
                }
                break;
            }
            case 3u: {
                if (GetInterior()) {
                    bRetVal = GetGroupSubBlockKey() < apGroupFORM->flags;
                }
                break;
            }
            case 4u: {
                bRetVal = GetInterior() || GetPersistentCell() || GetGroupBlockKey() < apGroupFORM->flags;
                break;
            }
            case 5u: {
                bRetVal = GetInterior() || GetPersistentCell() || GetGroupSubBlockKey() < apGroupFORM->flags;
                break;
            }
            case 6u:
            case 8u:
            case 9u: {
                TESForm* pForm = TESForm::GetFormByNumericID(apGroupFORM->flags);
                TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(pForm);
                if (pCell) {
                    bRetVal = SavesBefore(pCell);
                }
                break;
            }
            case 7u: {
                bRetVal = true;
                break;
            }
            default: {
                break;
            }
        }
    }

    return bRetVal;
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
        NavMeshArray* pNewNavMeshes = new NavMeshArray;
        pCell->SetNavMeshArray(pNewNavMeshes);
        alignas(NavMeshPtr) uint8_t aCopyStorage[sizeof(NavMeshPtr)];
        NavMeshPtr& spCopy = *reinterpret_cast<NavMeshPtr*>(aCopyStorage);
        ::new (&spCopy) NavMeshPtr;
        for (uint32_t i = 0; i < pNavMeshes->GetNavMeshCount(); ++i) {
            TESForm* pNavMesh = pNavMeshes->GetNavMeshByIndex(i);
            if (pNavMesh && !pNavMesh->GetDelete()) {
                NavMesh* pNewNavMesh = dynamic_cast<NavMesh*>(pNavMesh->CreateDuplicateForm(false, apCopyMap));
                if (pNewNavMesh) {
                    pNewNavMesh->pParentCell = pCell;
                    spCopy = NavMeshPtr::MakeNavMeshPtr(pNewNavMesh);
                    // SWEEP-INLINE 0x42fa20
                    pCell->pNavMeshes->AddNavMesh(spCopy);
                }
            }
        }
        spCopy = nullptr;
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
    int16_t sDetachSize = 0;
    if ((aiFlags & CHANGE_CELL_DETACHTIME) != 0) {
        sDetachSize = 4;
    }
    uint16_t usSize = TESForm::GetSaveSize(aiFlags) + sDetachSize;
    uint16_t usBaseSize = usSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        usSize += 6;
    }
    if ((aiFlags & CHANGE_CELL_FLAGS) != 0) {
        ++usSize;
    }
    if ((aiFlags & CHANGE_CELL_SEENDATA) != 0) {
        SeenData* pSeenData = GetSeenData();
        usSize += pSeenData->GetSaveSize(false);
    }
    if ((aiFlags & CHANGE_CELL_FULLNAME) != 0) {
        usSize += BSstrlen(GetFullName()) + 1;
    }
    if ((aiFlags & CHANGE_CELL_OWNERSHIP) != 0) {
        usSize += 4;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                usSize - usBaseSize,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                0x3D51,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize - usBaseSize,
                0x3D51,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp"
            );
        }
    }
    return usSize;
}

// Writes the cell's changes in the old save format: the detach time, TESForm's data, the block header and size, the
// cell flags, the seen data, the name and the owner, then the block's size into its slot.
void TESObjectCELL::SaveGame(uint32_t aiFlags) {
    uint32_t uiOwnerID;
    uint8_t cLength;
    uint8_t cTempCellFlags;
    uint32_t uiBlockHeader;
    uint16_t usBlockSize;
    int32_t iDetachTime;
    if ((aiFlags & CHANGE_CELL_DETACHTIME) != 0) {
        iDetachTime = GetDetachTime();
        SaveGameDataOLD(&iDetachTime, sizeof(iDetachTime));
    }
    TESForm::SaveGame(aiFlags);
    usBlockSize = 0;
    uint16_t* pBlockSize = nullptr;
    uint8_t* pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uiBlockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&uiBlockHeader, sizeof(uiBlockHeader));
        pBlockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    if ((aiFlags & CHANGE_CELL_FLAGS) != 0) {
        cTempCellFlags = cCellFlags & (kCellFlag_Public | kCellFlag_TempPublic);
        cTempCellFlags |= cCellGameFlags;
        SaveGameDataOLD(&cTempCellFlags, sizeof(cTempCellFlags));
    }
    if ((aiFlags & CHANGE_CELL_SEENDATA) != 0) {
        SeenData* pSeenData = GetSeenData();
        pSeenData->SaveGame(false);
    }
    if ((aiFlags & CHANGE_CELL_FULLNAME) != 0) {
        const char* pFullName = GetFullName();
        cLength = BSstrlen(pFullName);
        SaveGameDataOLD(&cLength, sizeof(cLength));
        if (cLength) {
            SaveGameDataOLD(pFullName, cLength);
        }
    }
    if ((aiFlags & CHANGE_CELL_OWNERSHIP) != 0) {
        TESForm* pOwner = GetOwner();
        uiOwnerID = 0;
        if (pOwner) {
            uiOwnerID = pOwner->iFormID;
        }
        SaveNumericID(&uiOwnerID, 4);
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pBufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            uint32_t iFlags = pHeader->iFlags;
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                pBufferPointer - pTempStartPosition,
                pHeader->iFormID,
                pEditorID,
                iFlags,
                15750,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                pBufferPointer - pTempStartPosition,
                15750,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block's size is the bytes written since the size slot, compared and subtracted as addresses.
        uintptr_t uiBufferPos = reinterpret_cast<uintptr_t>(pSaveLoadGameOLD->m_pBuffer);
        uintptr_t uiBlockSizePos = reinterpret_cast<uintptr_t>(pBlockSize);
        if (uiBufferPos > uiBlockSizePos + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
                15750
            );
        }
        *pBlockSize = static_cast<uint16_t>(uiBufferPos - uiBlockSizePos);
    }
}

void TESObjectCELL::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t cTempCellFlags;
    TESForm::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_CELL_FLAGS)) {
        cTempCellFlags = cCellFlags & (kCellFlag_Public | kCellFlag_TempPublic);
        cTempCellFlags |= cCellGameFlags;
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cTempCellFlags), sizeof(cTempCellFlags), 0);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_CELL_SEENDATA)) {
        SeenData* pSeenData = GetSeenData();
        pSeenData->SaveGame(apSaveGameBuffer);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_CELL_FULLNAME)) {
        const char* pFullName = GetFullName();
        apSaveGameBuffer->SaveString(pFullName, 0);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_CELL_OWNERSHIP)) {
        TESForm* pOwner = GetOwner();
        apSaveGameBuffer->SaveFormID(pOwner, 0u);
    }
}

// Reads the cell's changes from an old-format save: the detach time (before or after TESForm's data, by save version),
// the block header and size, the cell flags, the seen data, the name and the owner; then warns when the read ran past
// or short of the block. LoadGameDataOLD reads raw save bytes into the locals, hence the double* casts.
void TESObjectCELL::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x5A && (aiFlags & CHANGE_CELL_DETACHTIME) != 0) {
        int32_t iDetachTime;
        LoadGameDataOLD(reinterpret_cast<double*>(&iDetachTime), sizeof(iDetachTime));
        GetExtra()->SetDetachTime(iDetachTime);
    }
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    uint16_t usBlockSize = 0;
    char* pBlockStart = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiBlockMarker;
        pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockMarker, sizeof(uiBlockMarker));
        if (uiBlockMarker != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
                    15773,
                    pHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pHeader->cVersion,
                    pHeader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
                    15773,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    if ((aiFlags & CHANGE_CELL_FLAGS) != 0) {
        uint8_t cFlags;
        LoadGameDataOLD(reinterpret_cast<double*>(&cFlags), sizeof(cFlags));
        cCellFlags = (cCellFlags & ~(kCellFlag_Public | kCellFlag_TempPublic)) |
            (cFlags & (kCellFlag_Public | kCellFlag_TempPublic));
        cCellGameFlags = cFlags & ~(kCellFlag_Public | kCellFlag_TempPublic);
    }
    if ((aiFlags & CHANGE_CELL_SEENDATA) != 0) {
        SeenData* pSeenData = GetSeenData();
        if (!pSeenData) {
            if (GetInterior()) {
                IntSeenData* pIntSeenData = new IntSeenData(0, 0);
                pSeenData = pIntSeenData;
            }
            else {
                SeenData* pNewSeenData = new SeenData;
                pSeenData = pNewSeenData;
            }
            GetExtra()->SetSeenData(pSeenData);
        }
        pSeenData->LoadGame(static_cast<uint16_t>(0xFFFF));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 0x5A && (aiFlags & CHANGE_CELL_DETACHTIME) != 0) {
        int32_t iDetachTime;
        LoadGameDataOLD(reinterpret_cast<double*>(&iDetachTime), sizeof(iDetachTime));
        GetExtra()->SetDetachTime(iDetachTime);
    }
    if ((aiFlags & CHANGE_CELL_FULLNAME) != 0) {
        char acName[0x104];
        ourMemset(acName, 0, 0x104);
        uint8_t cLength;
        LoadGameDataOLD(reinterpret_cast<double*>(&cLength), sizeof(cLength));
        if (cLength) {
            LoadGameDataOLD(reinterpret_cast<double*>(acName), cLength);
        }
        SetFullName(acName);
    }
    if ((aiFlags & CHANGE_CELL_OWNERSHIP) != 0) {
        uint32_t uiOwnerID;
        LoadNumericID(&uiOwnerID, 4);
        // The loaded owner ID is passed straight on as the owner.
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
                    15829,
                    pHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pHeader->cVersion,
                    pHeader->iFlags
                );
            }
            else if (pPosition < pBlockStart + usBlockSize) {
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>((pBlockStart + usBlockSize) - pPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
                    15829,
                    pHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pHeader->cVersion,
                    pHeader->iFlags
                );
            }
        }
        else if (pPosition > pBlockStart + usBlockSize) {
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pPosition - (pBlockStart + usBlockSize)),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
                15829,
                pSaveLoadGameOLD->GetCurrentVersion()
            );
        }
        else if (pPosition < pBlockStart + usBlockSize) {
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>((pBlockStart + usBlockSize) - pPosition),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
                15829,
                pSaveLoadGameOLD->GetCurrentVersion()
            );
        }
    }
}

void TESObjectCELL::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint32_t uiOwnerID;
    uint8_t cTempCellFlags;
    TESForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_CELL_FLAGS)) {
        cTempCellFlags = 0;
        apLoadGameBuffer->LoadData(&cTempCellFlags, sizeof(cTempCellFlags));
        cCellFlags = (cTempCellFlags & (kCellFlag_Public | kCellFlag_TempPublic)) |
            (cCellFlags & ~(kCellFlag_Public | kCellFlag_TempPublic));
        cCellGameFlags = cTempCellFlags & ~(kCellFlag_Public | kCellFlag_TempPublic);
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_CELL_SEENDATA)) {
        SeenData* pSeenData = GetSeenData();
        if (!pSeenData) {
            SeenData* pNewSeenData;
            if (GetInterior()) {
                pNewSeenData = new IntSeenData(0, 0);
            }
            else {
                pNewSeenData = new SeenData;
            }
            pSeenData = pNewSeenData;
            GetExtra()->SetSeenData(pNewSeenData);
        }
        pSeenData->LoadGame(apLoadGameBuffer);
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_CELL_FULLNAME)) {
        char acName[268];
        apLoadGameBuffer->LoadString(acName);
        SetFullName(acName);
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_CELL_OWNERSHIP)) {
        // The owner's form ID is kept in the ownership extra until InitLoadGame resolves it.
        uiOwnerID = 0;
        apLoadGameBuffer->LoadFormID(uiOwnerID);
        SetCellOwnership(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiOwnerID)));
    }
}

// The owner is stored as its form ID until the load resolves it.
void TESObjectCELL::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    TESForm::InitLoadGame(aiFlags, aiOldFlags);
    if (aiFlags & CHANGE_CELL_OWNERSHIP) {
        TESForm* pOwner = GetOwner();
        if (pOwner) {
            SetCellOwnership(TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pOwner))));
        }
    }
}

// The owner's form ID, which LoadGame keeps in the ownership extra, is resolved to the form.
void TESObjectCELL::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_CELL_OWNERSHIP)) {
        TESForm* pOwner = GetOwner();
        if (pOwner) {
            pOwner = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pOwner)));
            SetCellOwnership(pOwner);
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
// data. The Warning arguments go through locals so they are evaluated right to left.
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
                "Cell name %s for cell %s (%08X) is too long. The maximum length allowed is 33 characters.",
                pFullName,
                pEditorID,
                iFormID
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
                    pEditorID,
                    iFormID
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
                    pEditorID,
                    iFormID,
                    iX,
                    iY,
                    pWorldEditorID,
                    iWorldFormID
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
                    pEditorID,
                    iFormID,
                    iX,
                    iY
                );
            }
        }
        if (GetLightingTemplate() != nullptr) {
            // Until now the template pointer holds the file's form ID.
            uint32_t iTemplateID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetLightingTemplate()));
            TESForm::AddCompileIndex(iTemplateID, GetFile(-1));
            BGSLightingTemplate* pTemplate = dynamic_cast<BGSLightingTemplate*>(
                TESForm::GetFormByNumericID(iTemplateID)
            );
            SetLightingTemplate(pTemplate);
            if (pTemplate == nullptr && iTemplateID != 0) {
                uint32_t iFormID = GetFormID();
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Unable to find lighting template (%08X) for cell '%s' (%08X).",
                    iTemplateID,
                    pEditorID,
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
            NavMeshPtr spNavMesh = GetNavMeshArray()->GetNavMeshByIndex(i);
            spNavMesh->InitItem();
            pSaveLoadGame->LoadForm(static_cast<NavMesh*>(spNavMesh));
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
// prints them.
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
            "'%s' (%08X) (%i, %i) in WorldSpace '%s' (%08X)",
            pEditorID,
            uiFormID,
            iX,
            iY,
            pWorldEditorID,
            uiWorldFormID
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
            INTERIOR_DATA* pInteriorData = GetInteriorData();
            INTERIOR_DATA* pCopyInteriorData = pCopy->GetInteriorData();
            if (pInteriorData && pCopyInteriorData) {
                ourMemcpy(pInteriorData, pCopyInteriorData, sizeof(INTERIOR_DATA));
            }
        }
        else {
            EXTERIOR_DATA* pExteriorData = GetExteriorData();
            EXTERIOR_DATA* pCopyExteriorData = pCopy->GetExteriorData();
            if (pExteriorData && pCopyExteriorData) {
                ourMemcpy(pExteriorData, pCopyExteriorData, sizeof(EXTERIOR_DATA));
            }
        }
        SetLightingTemplate(pCopy->GetLightingTemplate());
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
    int32_t iCellFlags = cCellFlags;
    if (iCellFlags != pComp->cCellFlags) {
        return true;
    }
    if (iLightingTemplateInheritanceFlags != pComp->iLightingTemplateInheritanceFlags) {
        return true;
    }
    BGSLightingTemplate* pTemplate = GetLightingTemplate();
    if (pTemplate != pComp->GetLightingTemplate()) {
        return true;
    }
    if (pComp->fWaterHeight != fWaterHeight) {
        return true;
    }
    if (GetInterior()) {
        INTERIOR_DATA* pInteriorData = GetInteriorData();
        INTERIOR_DATA* pCompInteriorData = pComp->GetInteriorData();
        if (pInteriorData &&
            pCompInteriorData &&
            std::memcmp(pInteriorData, pCompInteriorData, sizeof(INTERIOR_DATA))) {
            return true;
        }
    }
    else {
        EXTERIOR_DATA* pExteriorData = GetExteriorData();
        EXTERIOR_DATA* pCompExteriorData = pComp->GetExteriorData();
        if (pExteriorData &&
            pCompExteriorData &&
            std::memcmp(pExteriorData, pCompExteriorData, sizeof(EXTERIOR_DATA))) {
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
    bool bRetVal = false;
    if (apGroupFORM && apGroupFORM->form == formEnumString[2].uiFormString) {
        TESWorldSpace* pWorld = GetWorldSpace();
        switch (apGroupFORM->iFormID) {
            case 0u: {
                if (abAllowParentGroups) {
                    if (GetInterior()) {
                        bRetVal = TESForm::BelongsInGroup(apGroupFORM, abAllowParentGroups, abCurrentOnly);
                    }
                    else {
                        bRetVal = pWorld->BelongsInGroup(apGroupFORM, abAllowParentGroups, abCurrentOnly);
                    }
                }
                break;
            }
            case 1u: {
                if (!GetInterior() &&
                    pWorld->FormIDMatchesWithoutIndex(apGroupFORM->flags) &&
                    (abAllowParentGroups || GetPersistentCell())) {
                    bRetVal = true;
                }
                break;
            }
            case 2u:
            case 4u: {
                if (!GetPersistentCell() && abAllowParentGroups) {
                    bRetVal = apGroupFORM->flags == GetGroupBlockKey();
                }
                break;
            }
            case 3u:
            case 5u: {
                if (!GetPersistentCell()) {
                    bRetVal = apGroupFORM->flags == GetGroupSubBlockKey();
                }
                break;
            }
            default: {
                break;
            }
        }
    }

    return bRetVal;
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
                else if (apParentGroup->GroupData.flags == formEnumString[57].uiFormString) {
                    apOutGroupFORM->form = formEnumString[2].uiFormString;
                    apOutGroupFORM->iFormID = 2;
                    apOutGroupFORM->flags = GetGroupBlockKey();
                    apOutGroupFORM->length = 0;
                    apOutGroupFORM->iVersionControl = 0;
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
        else if (apParentGroup) {
            uint32_t uiParentType = apParentGroup->GroupData.iFormID;
            if (uiParentType) {
                if (uiParentType == 1) {
                    TESWorldSpace* pWorld = GetWorldSpace();
                    if (apParentGroup->GroupData.flags == pWorld->GetFormID() && !GetPersistentCell()) {
                        apOutGroupFORM->form = formEnumString[2].uiFormString;
                        apOutGroupFORM->iFormID = 4;
                        apOutGroupFORM->flags = GetGroupBlockKey();
                        apOutGroupFORM->length = 0;
                        apOutGroupFORM->iVersionControl = 0;
                    }
                }
                else if (
                    uiParentType == 4 && apParentGroup->GroupData.flags == GetGroupBlockKey() && !GetPersistentCell()
                ) {
                    apOutGroupFORM->form = formEnumString[2].uiFormString;
                    apOutGroupFORM->iFormID = 5;
                    apOutGroupFORM->flags = GetGroupSubBlockKey();
                    apOutGroupFORM->length = 0;
                    apOutGroupFORM->iVersionControl = 0;
                }
            }
            else if (apParentGroup->GroupData.flags == formEnumString[65].uiFormString) {
                apOutGroupFORM->form = formEnumString[2].uiFormString;
                apOutGroupFORM->iFormID = 1;
                TESWorldSpace* pWorld = GetWorldSpace();
                apOutGroupFORM->flags = pWorld->GetFormID();
                apOutGroupFORM->length = 0;
                apOutGroupFORM->iVersionControl = 0;
            }
        }
    }
}

bool TESObjectCELL::IsFormTypeChild(uint8_t aeType) {
    return TESObjectCELL::IsFormTypeCellChild(aeType);
}

// GetExtraEditorID is called a second time for the result it returns.
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
    bool bFlag = false;
    TESForm* pOwner = GetOwner();
    if (pOwner) {
        if (!apActor->IsNpc()) {
            return false;
        }
        TESNPC* pNPC = dynamic_cast<TESNPC*>(pOwner);
        if (pNPC) {
            return pNPC == apActor->GetObjectReference();
        }

        TESFaction* pFaction = dynamic_cast<TESFaction*>(pOwner);
        if (pFaction) {
            int32_t iRank = GetOwnershipRank();
            TESNPC* pActNPC = dynamic_cast<TESNPC*>(apActor->GetObjectReference());
            return pActNPC->GetFactionRank(pFaction, apActor == pPlayer) >= iRank;
        }
    }
    return bFlag;
}

void TESObjectCELL::ClearLocationMarkers() {
    const char* pcName;
    NiAVObject* pChild;
    uint32_t i;
    NiNode* pMarkerNode = static_cast<NiNode*>(GetMarkerNode());
    if (pMarkerNode) {
        for (i = 0; i < pMarkerNode->GetArrayCount(); ++i) {
            pChild = pMarkerNode->GetAt(i);
            if (pChild) {
                pcName = pChild->GetName().GetHandleString();
                if (pcName && !BSstrcmp(pcName, "CellLocationMarker")) {
                    pMarkerNode->DetachChildAt(i);
                }
            }
        }
    }
}

// The release build drops the debug marker, so this does nothing.
void TESObjectCELL::AddLocationMarker(const NiPoint3&, const char*, const NiColorA*) {}

void TESObjectCELL::AdjustCoordForNorthRotation(const NiPoint3& aStartCoord, NiPoint3& aOutCoord, bool abClockwise) {
    float fSign;
    NiMatrix3 kRotation;
    if (abClockwise) {
        fSign = -1.0f;
    }
    else {
        fSign = 1.0;
    }
    float fAngle = GetNorthRotation() * fSign;
    if (fAngle == 0.0) {
        aOutCoord = aStartCoord;
    }
    else {
        NiPoint3 kRotated;
        kRotation = NiMatrix3::IDENTITY;
        kRotation.MakeZRotation(fAngle);
        kRotated = aStartCoord * kRotation;
        aOutCoord = kRotated;
    }
}

void TESObjectCELL::AddMapMarkersToList(BSSimpleList<TESObjectREFR*>* apList) {
    if (apList) {
        CellRefLockEnter();
        for (TESObjectREFR* pRef : *GetRefList()) {
            if (pRef && pRef->GetObjectReference() == pMapMarker && pRef->GetMapMarkerData()) {
                apList->AddHead(pRef);
            }
        }
        CellRefLockLeave();
    }
}

void TESObjectCELL::UpdateManagedNodeTransforms() {
    NiTMapItem<TESObjectREFR*, NiNode*>* pPos;
    NiUpdateData kUpdateData(0.0f, false, false);
    NiNode* pDynamicNode = static_cast<NiNode*>(GetDynamicNode());
    if (pDynamicNode) {
        pDynamicNode->UpdateTransformAndBounds(kUpdateData);
    }
    if (pLoadedData) {
        pPos = pLoadedData->AnimatedRefMap.GetFirstPos();
        while (pPos) {
            TESObjectREFR* pRef = nullptr;
            NiNode* pNode = nullptr;
            pLoadedData->AnimatedRefMap.GetNext(pPos, pRef, pNode);
            if (pNode) {
                if (pNode->m_pkParent != pDynamicNode) {
                    pNode->UpdateTransformAndBounds(kUpdateData);
                }
            }
        }
    }
}

void TESObjectCELL::ShowPrimitives(bool abEnable) {
    for (BSSimpleList<TESObjectREFR*>* pList = &listReferences; pList && pList->m_item; pList = pList->m_pkNext) {
        pList->m_item->ShowPrimitive(abEnable);
    }
}

void TESObjectCELL::SetCellOwnership(TESForm* apOwner) {
    GetExtra()->SetOwnership(apOwner);
    AddChange(CHANGE_CELL_OWNERSHIP);
}

bool TESObjectCELL::PickWaterDistance(NiPick* apPick, const NiPoint3& aOrigin, const NiPoint3& aDir, float afDistance) {
    bool bhitsomething = false;
    if (apPick) {
        for (BSSimpleList<TESObjectREFR*>* plist = GetWaterRefList(); plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            TESObjectREFR* pref = plist->m_item;
            if (pref->Get3D()) {
                NiNode* p3D = pref->Get3D();
                if (PickDistanceRecursive(apPick, aOrigin, aDir, afDistance, p3D)) {
                    NiPoint3 waterhit = *&apPick->m_pickResults.GetAt(0)->m_intersect;
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
    NiTMapItem<TESObjectREFR*, NiPointer<BSMultiBoundNode>>* iter;
    if (pLoadedData) {
        pLoadedData->ScriptedRefs.RemoveAll();
        pLoadedData->ActivatingRefs.RemoveAll();
        iter = pLoadedData->MultiboundRefMap.GetFirstPos();
        while (iter) {
            NiPointer<BSMultiBoundNode> spnode(static_cast<BSMultiBoundNode*>(nullptr));
            TESObjectREFR* pref = nullptr;
            pLoadedData->MultiboundRefMap.GetNext(iter, pref, spnode);
            if (pref && spnode.m_pObject && (spnode->QMultiBound()->m_uiRefCount == 2)) {
                pref->FreeMultiBound();
            }
        }
        if (pLoadedData->spCell3D.m_pObject) {
            ClearPortalGraph();
            NiNode* pparent = pLoadedData->spCell3D->m_pkParent;
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
    BSSimpleList<TESObjectREFR*>* pIter = GetRefList();
    while (pIter) {
        pRef = static_cast<PlayerCharacter*>(pIter->m_item);
        pIter = pIter->m_pkNext;
        if (pRef && pRef != pPlayer) {
            pNode = pRef->TESObjectREFR::Get3D();
            if (pNode) {
                pFadeNode = pNode->IsFadeNode();
                if (pFadeNode &&
                    (pFadeNode->GetMultType() == LOD_MULT_DISTANTLOD ||
                     pFadeNode->GetBoundRadius() > 599.9000244140625)) {
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
    if (apWorld && GetPersistentCell()) {
        CellRefLockEnter();
        for (TESObjectREFR* pRef : *GetRefList()) {
            if (pRef) {
                int32_t iX = FloatToInt(pRef->GetLocationOnReference().x) >> 12;
                int32_t iY = FloatToInt(pRef->GetLocationOnReference().y) >> 12;
                TESObjectCELL* pCell = apWorld->GetCellFromCellCoord(iX, iY);
                if (pCell) {
                    pCell->AddReference(pRef, false);
                }
                else {
                    int32_t iRefID = pRef->GetFormID();
                    const char* pRefEditorID = pRef->GetFormEditorID();
                    int32_t iCellID = GetFormID();
                    const char* pWorldEditorID = apWorld->GetFormEditorID();
                    Warning(
                        "CELLS: Could not find cell (%i, %i) in world '%s' (%08X) to add reference '%s' (%08X) "
                        "to.",
                        iX,
                        iY,
                        pWorldEditorID,
                        iCellID,
                        pRefEditorID,
                        iRefID
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
    return GetState() == CS_UNLOADING;
}

bool TESObjectCELL::GetCantWaitHere() {
    TESWorldSpace* pWorld;
    if (GetInterior()) {
        if ((TESForm::GetFormFlags() & 524288) != 0) {
            return true;
        }
    }
    else {
        pWorld = GetWorldSpace();
        if (pWorld) {
            return pWorld->GetCantWaitHere();
        }
    }
    return false;
}

void TESObjectCELL::SetLowestProcessToMiddleLow(bool abFlag) {
    ExtraProcessMiddleLow* pExtra = static_cast<ExtraProcessMiddleLow*>(ExtraData.GetExtraData(9));
    if (!pExtra && abFlag) {
        ExtraProcessMiddleLow* pNewExtra;
        pNewExtra = new ExtraProcessMiddleLow;
        pExtra = pNewExtra;
        ExtraData.AddExtra(pNewExtra);
        CellRefLockEnter();
        BSSimpleList<TESObjectREFR*>* pList = GetRefList();
        while (pList && pList->m_item) {
            MobileObject* pRef = dynamic_cast<MobileObject*>(pList->m_item);
            pList = pList->m_pkNext;
            if (pRef &&
                !pRef->GetDisabled() &&
                pRef->pCurrentProcess &&
                pRef->GetCurrentProcessType() == PROCESS_TYPE_LOW) {
                ReferenceProcessLists.AddActorToTempChangeList(pRef);
            }
        }
        CellRefLockLeave();
    }
    if (pExtra) {
        int32_t iRefCount = abFlag ? pExtra->iRefCount + 1 : pExtra->iRefCount - 1;
        pExtra->iRefCount = iRefCount;
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
    TESObjectLAND* pLand = GetLand();
    if (pLand) {
        return pLand->GetLandHeight(arPos, arHeight);
    }
    arHeight = 0.0f;
    return false;
}

// Returns a NiNode; NiNode::GetAt yields a NiAVObject, so the pointer is cast.
NiNode* TESObjectCELL::GetLandNode(uint32_t aiQuad) {
    NiNode* pChildNode = GetChildNode(TESObjectCELL::CN_LAND);
    if (pChildNode) {
        return static_cast<NiNode*>(pChildNode->GetAt(aiQuad));
    }

    return nullptr;
}

void TESObjectCELL::UpdateRefSounds() {
    CellRefLockEnter();
    if (GetAttached()) {
        for (TESObjectREFR* pObj : GetRefList()->ItemsUntilNull()) {
            if (pObj &&
                !pObj->GetDisabled() &&
                !pObj->GetDestroyed() &&
                pObj->HasRandomSound() &&
                (!pObj->IsCreature() || !pObj->IsDead(false))) {
                TESSound* pSound = pObj->pRandomSound;
                if (pSound) {
                    float fChance = static_cast<double>(pSound->GetRandomPercentChance()) / 200.0;
                    if (RandomBool(fChance) && *pSound->GetFilename()) {
                        uint32_t iMax = 100 * pSound->GetMaxDistance();
                        float fDist = pPlayer->GetDistanceFromReference(pObj, false, false);
                        if (iMax > static_cast<uint32_t>(static_cast<int64_t>(fDist))) {
                            uint32_t uiSoundID = pSound->iFormID;
                            BSSoundHandle kSoundHandle = BSAudio::QInstance().GetSoundHandleByNumericID(
                                uiSoundID,
                                BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                    BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER
                            );
                            if (pObj->Get3D()) {
                                kSoundHandle.SetObjectToFollow(pObj->Get3D());
                            }
                            const NiPoint3& kPos = pObj->GetLocationOnReference();
                            kSoundHandle.SetPosition(kPos.x, kPos.y, kPos.z);
                            kSoundHandle.SetPriority(192u);
                            kSoundHandle.Play(false);
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
    BSSimpleList<TESObjectREFR*>* pIter = GetRefList();
    while (pIter) {
        TESObjectREFR* pRef = pIter->m_item;
        pIter = pIter->m_pkNext;
        if (pRef && pRef->CanHaveSound()) {
            pRef->AttachSound(pRef->ShouldPlaySound());
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::QueueReferences(bool abHighPriorityOnly) {
    TESObjectREFR* pRefr;
    BSSimpleList<TESObjectREFR*>* pIter;
    CellRefLockEnter();
    int32_t eCellPriority = TES::pInstance->GetCellPriority(this, nullptr);
    for (pIter = GetRefList(); pIter; pIter = pIter->m_pkNext) {
        pRefr = pIter->m_item;
        if (pRefr &&
            (static_cast<const void*>(pRefr->TESObjectREFR::GetObjectReference()) ==
                 static_cast<const void*>(pRoomMarker) ||
             static_cast<const void*>(pRefr->TESObjectREFR::GetObjectReference()) ==
                 static_cast<const void*>(pBoundMarker))) {
            if (pRefr->Get3D() && !pRefr->HasTemp3D()) {
                if (pRefr->GetDisabled() || pRefr->GetDelete()) {
                    pRefr->Set3D(nullptr, true);
                }
            }
            else {
                if (!pRefr->GetDisabled() &&
                    !pRefr->GetDelete() &&
                    (!abHighPriorityOnly || pRefr->Is3DHighPriority())) {
                    pModelLoader->QueueReference(pRefr, static_cast<IO_TASK_PRIORITY>(eCellPriority), true);
                }
            }
        }
    }
    for (pIter = GetRefList(); pIter; pIter = pIter->m_pkNext) {
        pRefr = pIter->m_item;
        if (pRefr &&
            static_cast<const void*>(pRefr->TESObjectREFR::GetObjectReference()) !=
                static_cast<const void*>(pRoomMarker) &&
            static_cast<const void*>(pRefr->TESObjectREFR::GetObjectReference()) !=
                static_cast<const void*>(pBoundMarker)) {
            if (pRefr->Get3D() && !pRefr->HasTemp3D()) {
                if (pRefr->GetDisabled() || pRefr->GetDelete()) {
                    pRefr->Set3D(nullptr, false);
                }
            }
            else {
                if (!pRefr->GetDisabled() &&
                    !pRefr->GetDelete() &&
                    (!abHighPriorityOnly || pRefr->Is3DHighPriority())) {
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
    NiNode* pActors = static_cast<NiNode*>(GetActorNode());
    uint32_t uiActors = pActors->GetArrayCount();
    while (uiActors--) {
        NiAVObject* pActorAV = pActors->GetAt(uiActors);
        MobileObject* pMob = static_cast<MobileObject*>(TESObjectREFR::FindReferenceFor3D(pActorAV));
        if (pMob) {
            pMob->SetDisplayGeometry(abTF);
        }
    }
    NiNode* pMB = GetChildNode(TESObjectCELL::CN_MULTIBOUND);
    uint32_t ui = 0;
    for (uint32_t i = pMB->GetArrayCount(); ui < i; i = pMB->GetArrayCount()) {
        NiAVObject* pAVObj = pMB->GetAt(ui);
        if (pAVObj && (pAVObj->IsMultiBoundNode() || NiObject::IsExactKindOf(&BSMultiBoundRoom::ms_RTTI, pAVObj))) {
            BSMultiBoundNode* pMBNode = static_cast<BSMultiBoundNode*>(pAVObj);
            int32_t iChild = 0;
            for (uint32_t j = static_cast<NiNode*>(pAVObj)->NiNode::GetArrayCount(); iChild < j;
                 j = pMBNode->GetArrayCount()) {
                NiAVObject* pChild = pMBNode->GetAt(static_cast<uint32_t>(iChild));
                if (pChild) {
                    TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pChild);
                    if (pRef->IsActor()) {
                        // The same slot (+0x2F0) as the actor loop's call above: MobileObject's.
                        static_cast<MobileObject*>(pRef)->SetDisplayGeometry(abTF);
                    }
                }
                ++iChild;
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
                    if (apRef->IsActor() && (!pSaveLoadGameOLD->GetLoading() || pSaveLoadGameOLD->GetLoading()))
                    // pSaveLoadGameOLD is asked twice whether it is loading, and the actor is moved either way.
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
                OcclusionPlaneLinkedRefData*
                    pOtherData = pLinkedData->pLinkedRefs[i]->GetExtra()->GetOcclusionPlaneLinkedRefData();
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
    if (apRef && apRef->GetObjectReference()) {
        if (GetPersistentCell()) {
            CellRefLockEnter();
            GetRefList()->AddHead(apRef);
            CellRefLockLeave();
            apRef->GetExtra()->SetPersistentCell(this);
            if (!pDataHandler->GetSaveLoad()) {
                SetAltered(true);
            }
        }
        else {
            TESObjectCELL* pParentCell = apRef->GetParentCell();
            if (pParentCell) {
                pParentCell->RemoveReference(apRef);
            }
            CellRefLockEnter();
            GetRefList()->AddHead(apRef);
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
        if (pBase->GetFormType() == DOOR_ID &&
            pBase != static_cast<TESForm*>(pJailMarker) &&
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
        if (afRadius2 != 3.4028234663852886e+38 &&
            (afRadius1 != afRadius2 || !(aPoint1 == aPoint2)) &&
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
// nodes 7 and 8 are attached twice, deliberately), then the loaded data, multibounds, Havok and the
// save's changes. Every load makes sure the portal graph has its shared portal geometry root. The AutoMemContext is
// constructed and destroyed explicitly in local storage, and the constructor's result is ignored.
NiNode* TESObjectCELL::Load3D() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_CELLS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
        3460
    );
    NiNode* pnode = Get3D();
    if (!pnode) {
        SetState(CS_LOADING);
        bCellDetached = false;
        pnode = new NiNode(0);
        pnode->SetTranslate(NiPoint3::ZERO);
        pnode->SetRotate(NiMatrix3::IDENTITY);
        pnode->SetFixedBound(true);
        pnode->SetAlwaysDraw(true);

        NiNode* pnew = new NiNode(0);
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_ACTOR));
        pnode->AttachChild(pnew, true);

        pnew = new NiNode(0);
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetPickOff(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_MARKER));
        pnode->AttachChild(pnew, true);
        NiNode* pmarkernode = pnew;

        pnew = new NiNode(0);
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_LIGHT_MARKER));
        pmarkernode->AttachChild(pnew, true);
        spLightMarkerNode = pnew;

        pnew = new NiNode(0);
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_SOUND_MARKER));
        pmarkernode->AttachChild(pnew, true);
        spSoundMarkerNode = pnew;

        pnew = new NiNode(0);
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_LAND));
        pnode->AttachChild(pnew, true);
        for (int32_t i = 0; i < 4; ++i) {
            BSMultiBoundNode* plandnode = new BSMultiBoundNode;
            plandnode->SetCullingMode(BSCullingProcess::BSCP_CULL_ALLPASS);
            BSMultiBound* pmultibound = new BSMultiBound;
            BSMultiBoundAABB* pmbaabb = new BSMultiBoundAABB;
            pmultibound->SetShape(pmbaabb);
            plandnode->SetMultiBound(pmultibound);
            pnew->AttachChild(plandnode, true);
        }

        pnew = new NiNode(0);
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_STATIC));
        pnode->AttachChild(pnew, true);

        // Node 4 is never culled; IsNodeCulled is not asked for it.
        pnew = new NiNode(0);
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(false);
        pnode->AttachChild(pnew, true);

        pnew = new NiNode(0);
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_OCCLUSION_PLANE));
        pnode->AttachChild(pnew, true);

        pnew = new NiNode(0);
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_PORTAL));
        pnode->AttachChild(pnew, true);

        pnew = new NiNode(0);
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_MULTIBOUND));
        pnode->AttachChild(pnew, true);
        pnode->AttachChild(pnew, true);

        pnew = new NiNode(0);
        pnew->SetAlwaysDraw(true);
        pnew->SetFixedBound(true);
        pnew->SetAppCulled(IsNodeCulled(CCN_COLLISION));
        pnode->AttachChild(pnew, true);
        pnode->AttachChild(pnew, true);

        NiUpdateData UpdateData(0.0f, false, false);
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
        NiNode* ppnr = new NiNode(0);
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
        return -std::numeric_limits<float>::max();
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
// angle and the copied rotation are read but unused.
bool TESObjectCELL::GetWaterHeight(const NiPoint3& arPos, float& afHeight) {
    bool bResult = false;
    if (GetInterior() && !HasWater()) {
        afHeight = -std::numeric_limits<float>::max();
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
            pWaterRef->GetBoundMin();
            pWaterRef->GetBoundMax();
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
        fWaterHeight = std::numeric_limits<float>::max();
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
                apRef->GetFormID(),
                iCellID,
                pEditorID
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
                    BSMultiBoundRoom* pnewroom = new BSMultiBoundRoom;
                    BSMultiBound* pbound = new BSMultiBound;
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
                BSPortalSharedNode* pportalnode = new BSPortalSharedNode(pportal);
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
            pplane = new BSOcclusionPlane;
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
            apRef->GetObjectReference() &&
            apRef->GetObjectReference()->GetFormType() == LIGH_ID &&
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
        pfadenode->SetRange(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
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
        INTERIOR_DATA* pData = new INTERIOR_DATA;
        pCellData = pData;
    }
    else {
        EXTERIOR_DATA* pData = new EXTERIOR_DATA;
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
        LoadedCellData* pData = new LoadedCellData;
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
    // Fetches the material property and does not use it.
    p3D->GetProperty(NiProperty::MATERIAL);
    BSShaderManager::SetExternalEmittanceRecurse(p3D, pColor);
    pLoadedData->EmittanceSourceRefMap.SetAt(pSource, apRef);
}

void TESObjectCELL::AddWaterRef(TESObjectREFR* apRef) {
    if (pLoadedData) {
        pLoadedData->WaterRefs.AddHead(apRef);
    }
}

bool TESObjectCELL::All3DWithVisibleDistantLoaded() {
    return static_cast<int16_t>(sNumRefsWithVisibleDistant) <= sNumLoadedRefsWithVisibleDistant;
}

bool TESObjectCELL::ShouldDisplayHighDetail() {
    return GetState() != CS_NOTLOADED;
}

void TESObjectCELL::FadeToHighDetail() {
    if (!bFadingToHighDetail) {
        bFadingToHighDetail = true;
        bFadingToLowDetail = false;
        fLodFadeInPercent = 0.0f;
    }
}

// Returns the node it filed in the loaded data's MultiboundRefMap. The node goes to SetAt as a by-value NiPointer.
BSMultiBoundNode* TESObjectCELL::AddMultiBoundRef(TESObjectREFR* apRef) {
    NiPoint3 kAngle;
    if (!pLoadedData || !apRef || !apRef->IsMultiBoundRef()) {
        return nullptr;
    }
    BSMultiBoundNode* pMBNode = apRef->m_Extra.GetRoom();
    BSMultiBound* pMB = nullptr;
    if (!pMBNode) {
        if (apRef->GetObjectReference() == pRoomMarker) {
            pMBNode = apRef->GetMultiBoundRoom();
            if (!pMBNode) {
                return nullptr;
            }
            pMB = pMBNode->QMultiBound();
        }
        else {
            BSMultiBoundNode* pNewNode;
            pNewNode = new BSMultiBoundNode;
            pMBNode = pNewNode;
            pMB = apRef->GetMultiBound();
            pNewNode->SetMultiBound(pMB);
        }
    }
    else {
        pMB = pMBNode->QMultiBound();
    }
    BSMultiBoundShape* pShape = pMB->QShape();
    if (!pShape) {
        BGSPrimitive* pPrimitive = apRef->m_Extra.GetPrimitive();
        if (pPrimitive) {
            kAngle = apRef->GetAngleOnReference();
            pShape = pPrimitive->MakeMultiBoundShape(kAngle);
            pMB->SetShape(pShape);
        }
    }
    if (pShape) {
        pShape->SetCenter(apRef->GetLocationOnReference());
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
    if (!pLoadedData->MultiboundRefMap.GetAt(apRef, spNode) || spNode == nullptr) {
        spNode = AddMultiBoundRef(apRef);
    }
    return spNode;
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
        if (spNode.m_pObject != nullptr && spNode->GetParent() == nullptr) {
            pMultiBoundRoot->AttachChild(spNode.m_pObject, true);
        }
    }
}

// An interior cell makes its own portal graph; an exterior cell shares its worldspace's.
BSPortalGraph* TESObjectCELL::CreatePortalGraph() {
    if (spPortalGraph.m_pObject == nullptr) {
        if (GetInterior()) {
            BSPortalGraph* pGraph = new BSPortalGraph;
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
        TESRegionList* pNewList = new TESRegionList(false);
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
        lastloc = NiPoint3::ZERO;
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
        BSScrapMap<TESObjectREFR*, bool>* pMultiboundsExaminedMap = ::new (aMultiboundsExaminedMapStorage)
            BSScrapMap<TESObjectREFR*, bool>(37, false);
        TESObjectREFR* pcurb = apRef->GetExtra()->GetMultiBoundRef();
        if (pcurb != nullptr && !abMultipleRoomTest) {
            pMultiboundsExaminedMap->SetAt(pcurb, true);
            bfoundattach = TestObjectAgainstRoom(apRef, spref3d, pcurb, btest, abMultipleRoomTest);
            if (!bfoundattach) {
                // Every portal of the room is walked, even after a room took the 3D.
                for (BSSimpleList<TESObjectREFR*>* pportalList = pcurb->GetExtra()->GetRoomLinkedPortalRefList();
                     pportalList != nullptr;
                     pportalList = pportalList->GetNext()) {
                    TESObjectREFR* pportalRef = pportalList->GetItem();
                    if (pportalRef == nullptr) {
                        continue;
                    }
                    TESObjectREFR* ptestRef = pportalRef->GetExtra()->GetPortalLinkedRoomRef(0);
                    if (ptestRef != nullptr) {
                        pMultiboundsExaminedMap->SetAt(ptestRef, true);
                        bfoundattach = TestObjectAgainstRoom(apRef, spref3d, ptestRef, btest, abMultipleRoomTest);
                    }
                    if (bfoundattach) {
                        continue;
                    }
                    ptestRef = pportalRef->GetExtra()->GetPortalLinkedRoomRef(1);
                    if (ptestRef != nullptr) {
                        bfoundattach = TestObjectAgainstRoom(apRef, spref3d, ptestRef, btest, abMultipleRoomTest);
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
            if (pboundref == nullptr || spnode == nullptr || pMultiboundsExaminedMap->GetAt(pboundref, battached)) {
                continue;
            }
            if (!bfindmultibound && abForceRoomTest && spnode->GetMultiBoundRoom() == nullptr) {
                continue;
            }
            BSMultiBound* pbound = spnode->QMultiBound();
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
            else if (spnode->GetMultiBoundRoom() != nullptr) {
                if (TestObjectAgainstRoom(apRef, spref3d, pboundref, btest, abMultipleRoomTest)) {
                    bfoundattach = true;
                    break;
                }
            }
            else if (apRef->QCenterInsideMultibound(pbound)) {
                MultiboundTestMap.SetAt(apRef, pboundref);
                NiAVObject* pref3d = apRef->Get3D();
                CheckWithinMultiBoundTask* ptask = new CheckWithinMultiBoundTask(pboundref, pref3d);
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
    NiPoint3 kBoundCenter = apRef3D->GetWorldBound().m_kCenter;
    bool bOcclusionPlane = false;
    if (apRef->m_Extra.GetOcclusionPlane()) {
        kBoundCenter = apRef->m_Extra.GetOcclusionPlane()->kCenter;
        bOcclusionPlane = true;
    }
    NiPointer<BSMultiBoundNode> spNode(static_cast<BSMultiBoundNode*>(nullptr));
    pLoadedData->MultiboundRefMap.GetAt(apBoundRef, spNode);
    if (!spNode.m_pObject) {
        return false;
    }
    if (!spNode->GetMultiBoundRoom()) {
        return false;
    }
    if (!abMultipleRoomTest || GetPortalGraph()->lPortalList.IsEmpty()) {
        if (!spNode.m_pObject ||
            (!static_cast<BSMultiBoundRoom*>(spNode.m_pObject)->QPointWithin(kBoundCenter) &&
             (!abMultipleRoomTest ||
              spNode->CheckBound(apRef3D->GetWorldBound()) == BSMultiBoundShape::BS_INTERSECT_NONE))) {
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
            BSOcclusionPlane* pOcclusionPlane = apRef->m_Extra.GetOcclusionPlane();
            pRoom->AddOccluder(pOcclusionPlane);
            return true;
        }

        bool bMultiplePortals = false;
        bool bPortalShared = false;
        BSPortal* pSharedPortal = nullptr;
        if (pRoom->QPortalList().m_uiCount) {
            NiFrustumPlanes kBoxPlanes;
            bool bUseFrustum = false;
            NiTListItem<BSPortal*>* pPos = pRoom->QPortalList().m_pkHead;
            while (pPos) {
                BSPortal* pPortal = pRoom->QPortalList().GetNext(pPos);
                if (pPortal) {
                    bool bAdd = false;
                    if (bUseFrustum) {
                        bAdd = pPortal->WithinFrustum(kBoxPlanes, false, NiPoint3::ZERO);
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
    if (!arDecalData.pParentCell) {
        arDecalData.pParentCell = this;
    }
    BGSDecalManager::GetSingleton()->AddDecal(arDecalData, aeDecalType, abForceAdd);
}

void TESObjectCELL::AddAudioMarkersToList(BSSimpleList<TESObjectREFR*>* apAudioList) {
    if (apAudioList == nullptr) {
        return;
    }
    CellRefLockEnter();
    for (BSSimpleList<TESObjectREFR*>* pNode = GetRefList(); pNode != nullptr; pNode = pNode->GetNext()) {
        TESObjectREFR* pRef = pNode->GetItem();
        if (pRef != nullptr &&
            pRef->GetObjectReference() == static_cast<TESForm*>(pAudioMarker) &&
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
// soon as a pass reads its last node, so that node's reference is filtered by the next pass, deliberately.
// Both save-game getters always return false.
void TESObjectCELL::AttachModels(bool abSetupMopp) {
    NiNode* pRootNode = Load3D();
    RunScripts(false, false);
    CalcLeveledCreatures(HandleExpiredReferences());
    if (TES::pInstance->IsBuffered(this)) {
        if (pRootNode) {
            NiUpdateData kUpdateData(0.0f, false, false);
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
            if (pRef->GetObjectReference()->TESForm::GetHasSpokenFlag() && pRef->Get3D() && GetWorldSpace()) {
                GetWorldSpace()->GetTerrainManager()->HideTree(pRef, true);
                GetWorldSpace()->GetTerrainManager()->SetNeedsImmediateUpdate(true);
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
    aOutCenter = NiPoint3::ZERO;
    CellRefLockEnter();
    int32_t irefcount = 0;
    NiPoint3 minLocation(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    );
    NiPoint3 maxLocation(
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::min()
    );
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
                std::fabs(pref->GetLocationOnReference().y) > 30000.0 ||
                boutsideBroadphase) {
                bool bbad = true;
                if (pref->GetExtra()->GetExtraData(EXTRA_STARTINGPOSITION)) {
                    uint32_t uiFormID = pref->GetFormID();
                    const char* pEditorID = pref->GetFormEditorID();
                    Warning(
                        "Ref '%s' (%08X) is at invalid position. Attempting to return to start pos.",
                        pEditorID,
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
                        std::fabs(pref->GetLocationOnReference().y) < 30000.0 &&
                        !boutsideBroadphase) {
                        bbad = false;
                    }
                }
                if (bbad) {
                    uint32_t uiFormID = pref->GetFormID();
                    const char* pEditorID = pref->GetFormEditorID();
                    Warning(
                        "Ref '%s' (%08X) is at invalid position. Attempting to move to safe place.",
                        pEditorID,
                        uiFormID
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
                GetFormEditorID(),
                GetFormID(),
                static_cast<double>(extents.x),
                static_cast<double>(extents.y),
                static_cast<double>(extents.z),
                static_cast<double>(HK2NI(3250.0f))
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
    aOutRot = NiPoint3::ZERO;
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
                eType > AMMO_ID &&
                eType <= CREA_ID &&
                pRef->GetRefPersists() &&
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
                BGSDestructibleObjectForm* pDestructible = BGSDestructibleObjectForm::GetDestructionForm(
                    pRef->GetObjectReference()
                );
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
    if (pCurrentForm != nullptr &&
        pCurrentForm->form == formEnumString[GRUP_ID].uiFormString &&
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
            if (apFile->GetMaster() &&
                (!apFile->GetMasterList()->IsEmpty() &&
                 (apFile->GetFormData()->flags & TESForm::kFormFlag_Deleted) != 0)) {
                bTempID = true;
            }

            if (pForm == nullptr &&
                !bChangedCells &&
                !bTempID &&
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
        if (pCurrentForm != nullptr &&
            pCurrentForm->form == formEnumString[GRUP_ID].uiFormString &&
            pCurrentForm->iFormID == 9 &&
            pCurrentForm->flags == GetFormID()) {
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
    Sky::COLOR_BLEND kColorBlend;
    if (GetInterior() && GetAttached()) {
        bhkWorld* pWorld = GetbhkWorld();
        if (pWorld) {
            pWorld->Update();
        }
    }
    bool bParallel = AILinearTaskThreadManager::ShouldQueue3DTask();
    NiNode* pDynamic = static_cast<NiNode*>(GetDynamicNode());
    if (pDynamic && !abLargeOnly) {
        NiUpdateData kUpdateData(afTime, false, bParallel);
        pDynamic->UpdateSelectedDownwardPass(kUpdateData, 0);
        if (iNumHWThreads.Int() > 1) {
            TaskQueueInterface::GetInstance()->QueueCellTransformChildTask(pDynamic);
            if (bParallel && kUpdateData.bFoundParticles) {
                TESObjectCELL::QueueParticleUpdate(pDynamic);
            }
            if (bParallel && kUpdateData.bFoundMorphController) {
                TaskQueueInterface::GetInstance()->QueueMorphControllerUpdate(pDynamic);
            }
        }
    }

    if (sNumLoadedRefsWithVisibleDistant > 0 && All3DWithVisibleDistantLoaded() && ShouldDisplayHighDetail()) {
        FadeToHighDetail();
    }
    if (bUpdateTerrain && GetWorldSpace() && GetState() == TESObjectCELL::CS_ATTACHED) {
        bDisplayHighDetail = true;
        bFadingToHighDetail = false;
        bFadingToLowDetail = false;
        if (GetWorldSpace()) {
            GetWorldSpace()->GetTerrainManager()->SetNeedsImmediateUpdate(true);
        }
        bUpdateTerrain = false;
    }

    if (bFadingToHighDetail) {
        if (fLodFadeInPercent >= TESObjectCELL::fLodFadeThreshold * TESObjectCELL::fLodFadeTime) {
            if (GetWorldSpace() && !bDisplayHighDetail) {
                GetWorldSpace()->GetTerrainManager()->SetNeedsImmediateUpdate(true);
            }

            bDisplayHighDetail = true;
            bFadingToHighDetail = false;
            bFadingToLowDetail = false;
        }
        fLodFadeInPercent = appTimer.fDelta + fLodFadeInPercent;
    }
    if (pLoadedData) {
        if (abLargeOnly) {
            BSSimpleList<TESObjectREFR*>* pList = &pLoadedData->LargeAnimatedRefs;
            while (pList && !pList->IsEmpty()) {
                TESObjectREFR* pRef = pList->m_item;
                NiNode* pNode = pRef->Get3D();
                pList = pList->m_pkNext;
                if (pNode) {
                    NiUpdateData kUpdateData(afTime, false, bParallel);
                    pNode->UpdateSelectedDownwardPass(kUpdateData, 0);
                    if (kUpdateData.bSceneGraphChange) {
                        BSShaderManager::GetShadowSceneNode(0)->UpdateObjectLighting(pNode, true);
                    }
                    if (iNumHWThreads.Int() > 1) {
                        TaskQueueInterface::GetInstance()->QueueCellTransformChildTask(pNode);
                        if (bParallel && kUpdateData.bFoundParticles) {
                            TESObjectCELL::QueueParticleUpdate(pNode);
                        }
                        if (bParallel && kUpdateData.bFoundMorphController) {
                            TaskQueueInterface::GetInstance()->QueueMorphControllerUpdate(pNode);
                        }
                    }
                }
            }
        }
        else {
            NiTMapItem<TESObjectREFR*, NiNode*>* pAnimatedPos = pLoadedData->AnimatedRefMap.GetFirstPos();
            while (pAnimatedPos) {
                TESObjectREFR* pRef = nullptr;
                NiNode* pNode = nullptr;
                pLoadedData->AnimatedRefMap.GetNext(pAnimatedPos, pRef, pNode);
                if (pNode) {
                    if (pNode->m_pkParent != pDynamic) {
                        if (pRef->GetObjectReference()->GetFormType() != TREE_ID ||
                            NodeInFrustum(pNode, BSTreeManager::GetCamera())) {
                            NiUpdateData kUpdateData(afTime, false, bParallel);
                            pNode->UpdateSelectedDownwardPass(kUpdateData, 0);
                            if (kUpdateData.bSceneGraphChange) {
                                BSShaderManager::GetShadowSceneNode(0)->UpdateObjectLighting(pNode, true);
                            }
                            if (iNumHWThreads.Int() > 1) {
                                TaskQueueInterface::GetInstance()->QueueCellTransformChildTask(pNode);
                                if (bParallel && kUpdateData.bFoundParticles) {
                                    TESObjectCELL::QueueParticleUpdate(pNode);
                                }
                                if (bParallel && kUpdateData.bFoundMorphController) {
                                    TaskQueueInterface::GetInstance()->QueueMorphControllerUpdate(pNode);
                                }
                            }
                        }
                    }
                }
            }
            NiTMapItem<TESForm*, TESObjectREFR*>* pSourcePos = pLoadedData->EmittanceSourceRefMap.GetFirstPos();
            while (pSourcePos) {
                TESForm* pSource = nullptr;
                TESObjectREFR* pRef = nullptr;
                pLoadedData->EmittanceSourceRefMap.GetNext(pSourcePos, pSource, pRef);
                if (pSource) {
                    NiColor* pEmitColor = pSource->GetEmittanceColor();
                    int32_t eTypeID = pSource->GetFormType();
                    if (eTypeID == LIGH_ID) {
                        uint32_t uiLightColor = static_cast<TESObjectLIGH*>(pSource)->GetColor();
                        float fScale = 0.003921569f;
                        pEmitColor->r = static_cast<double>(static_cast<uint8_t>(uiLightColor)) * 0.003921569f;
                        pEmitColor->g = static_cast<double>(static_cast<uint8_t>(uiLightColor >> 8)) * fScale;
                        pEmitColor->b = static_cast<double>(static_cast<uint8_t>(uiLightColor >> 16)) * fScale;
                    }
                    else if (eTypeID == REGN_ID) {
                        TESWeather* pWeather = static_cast<TESRegion*>(pSource)->GetCurrentWeather();
                        if (!pWeather) {
                            TESForm* pForm = TESForm::GetFormByNumericID(350u);
                            pWeather = dynamic_cast<TESWeather*>(pForm);
                        }
                        if (pWeather) {
                            TESWeather::ColorTime eTime2;
                            TESWeather::ColorTime eTime1;
                            Sky* pSky = Sky::GetInstance();
                            pSky->FillColorBlend(kColorBlend, pWeather, 1.0f, eTime1, eTime2);
                            pSky->FillColorBlendColors(
                                kColorBlend,
                                pWeather,
                                nullptr,
                                TESWeather::SUNLIGHT,
                                eTime1,
                                eTime2
                            );
                            double dSkyFlash = pSky->GetFlash();
                            float fFlash = fWeatherFlashDirectional.Float() * dSkyFlash;
                            pSky->SetColor(*pEmitColor, &kColorBlend, fFlash);
                        }
                    }
                }
            }
            NiTMapItem<TESObjectREFR*, NiNode*>* pLightPos = pLoadedData->EmittanceLightRefMap.GetFirstPos();
            while (pLightPos) {
                TESObjectREFR* pRef = nullptr;
                NiNode* pNode = nullptr;
                pLoadedData->EmittanceLightRefMap.GetNext(pLightPos, pRef, pNode);
                if (pNode) {
                    NiColor kEmitColor = NiColor::WHITE;
                    TESForm* pSource = pRef->GetEmittanceSource();
                    if (!pSource) {
                        pSource = pPlayer->GetOccupiedRegion();
                    }
                    if (pSource) {
                        int32_t eTypeID = pSource->GetFormType();
                        if (eTypeID == LIGH_ID) {
                            uint32_t uiLightColor = static_cast<TESObjectLIGH*>(pSource)->GetColor();
                            float fScale = 0.003921569f;
                            kEmitColor.r = static_cast<double>(static_cast<uint8_t>(uiLightColor)) * fScale;
                            kEmitColor.g = static_cast<double>(static_cast<uint8_t>(uiLightColor >> 8)) * fScale;
                            kEmitColor.b = static_cast<double>(static_cast<uint8_t>(uiLightColor >> 16)) * fScale;
                        }
                        else if (eTypeID == REGN_ID) {
                            TESWeather* pWeather = static_cast<TESRegion*>(pSource)->GetCurrentWeather();
                            if (!pWeather) {
                                TESForm* pForm = TESForm::GetFormByNumericID(350u);
                                pWeather = dynamic_cast<TESWeather*>(pForm);
                            }
                            if (pWeather) {
                                TESWeather::ColorTime eTime2;
                                TESWeather::ColorTime eTime1;
                                Sky* pSky = Sky::GetInstance();
                                pSky->FillColorBlend(kColorBlend, pWeather, 1.0f, eTime1, eTime2);
                                pSky->FillColorBlendColors(
                                    kColorBlend,
                                    pWeather,
                                    nullptr,
                                    TESWeather::SUNLIGHT,
                                    eTime1,
                                    eTime2
                                );
                                double dSkyFlash = pSky->GetFlash();
                                float fFlash = fWeatherFlashDirectional.Float() * dSkyFlash;
                                pSky->SetColor(kEmitColor, &kColorBlend, fFlash);
                            }
                        }
                    }
                    if (pRef->GetObjectReference()->GetFormType() == LIGH_ID) {
                        uint32_t uiLightColor = static_cast<TESObjectLIGH*>(pRef->GetObjectReference())->GetColor();
                        float fScale = 0.003921569f;
                        kEmitColor.r = static_cast<double>(static_cast<uint8_t>(uiLightColor)) * fScale * kEmitColor.r;
                        kEmitColor.g = static_cast<double>(static_cast<uint8_t>(uiLightColor >> 8)) * fScale *
                            kEmitColor.g;
                        kEmitColor.b = static_cast<double>(static_cast<uint8_t>(uiLightColor >> 16)) * fScale *
                            kEmitColor.b;
                        LightRefData* pTheLight = pRef->GetLight();
                        if (pTheLight && pTheLight->spLight.m_pObject) {
                            pTheLight->spLight->SetDiffuseColor(kEmitColor);
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
    int32_t iX = FloatToInt(aPt.x);
    int32_t iY = FloatToInt(aPt.y);
    return (iX >> 12) == GetDataX() && (iY >> 12) == GetDataY();
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
        MEM_CONTEXT_CELLS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
        14685
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
    bhkWorld* pWorld = GetbhkWorld();
    if (this->GetInterior() && !pWorld) {
        NiPoint3 kOrigin;
        TESArrowTargetListener* pArrowListener;
        BGSAcousticSpaceListener* pAcousticListener;
        TESTrapListener* pTrapListener;
        bhkWorld* pNewWorld;
        AutoMemContext kMemContext(
            MEM_CONTEXT_HAVOK,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
            14911
        );
        bhkWorldCinfo hkwInfo;
        hkwInfo.m_gravity = hkVector4(0.0f, 0.0f, HavokWorldProps::fGravitySC, 0.0f);
        hkwInfo.setBroadPhaseWorldSize(3250.0f);
        hkwInfo.m_collisionTolerance = 1.0f;
        pNewWorld = new bhkWorld(hkwInfo);
        pWorld = pNewWorld;
        pNewWorld->MarkForWrite();
        pTrapListener = new TESTrapListener;
        pWorld->AddTrapListener(pTrapListener);
        pAcousticListener = new BGSAcousticSpaceListener;
        pWorld->AddAcousticSpaceListener(pAcousticListener);
        if (this->HasWater()) {
            TESWaterListener* pWaterListener = new TESWaterListener(this->GetWaterHeight());
            pWorld->AddWaterListener(pWaterListener);
        }
        pWorld->AddContactListener(FOCollisionListener::Instance());
        pArrowListener = new TESArrowTargetListener;
        pWorld->AddEntityListener(pArrowListener);
        if (!TESObjectCELL::pIslandListener) {
            FOIslandActivationListener* pNewIslandListener = new FOIslandActivationListener;
            TESObjectCELL::pIslandListener = pNewIslandListener;
        }
        ahkpWorld* phkWorld = **pWorld;
        TESObjectCELL::pIslandListener->AddToWorld(phkWorld);
        this->CalcRefCenterPoint(kOrigin);
        pWorld->SetOrigin(kOrigin);
        pWorld->SetVisualDebugger(bhkWorld::IsHavokDebug());
        this->ExtraData.SetbhkWorld(pWorld);
        pWorld->MarkForWrite();
        pWorld->Update();
        pWorld->SetEnabled(!TES::QCollisionDisableFlag());
    }
    CellMopp* pCellMopp = this->GetCellMopp();
    if (!pCellMopp) {
        CellMopp* pNewMopp;
        AutoMemContext kMemContext(
            MEM_CONTEXT_HAVOK,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
            14993
        );
        pNewMopp = new CellMopp;
        pCellMopp = pNewMopp;
        this->ExtraData.SetCellMopp(pNewMopp);
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
            MEM_CONTEXT_HAVOK,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
            15014
        );
        NiTransform kTransform;
        kTransform.m_fScale = 1.0f;
        kTransform.m_Rotate = NiMatrix3::IDENTITY;
        kTransform.m_Translate.x = static_cast<float>((iX << 12) + 2048.0);
        kTransform.m_Translate.y = static_cast<float>((iY << 12) + 2048.0);
        kTransform.m_Translate.z = 0.0f;
        pCellMopp->Make(pWorld, kTransform.m_Translate, NiQuaternion::IDENTITY);
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
        if (!pRef->IsActor() &&
            pRef->GetObjectReference() != nullptr &&
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

void TESObjectCELL::ProcessAddonNodesForRefs(bool abAttach) {
    CellRefLockEnter();
    if (abAttach) {
        BSSimpleList<TESObjectREFR*>* pNode = GetRefList();
        while (pNode != nullptr) {
            TESObjectREFR* pRef = pNode->GetItem();
            pNode = pNode->GetNext();
            if (pRef != nullptr && pRef->GetLoadedDataFlag(LOADED_DATA_HAS_ADDON_FLAGS) && pRef->Get3D() != nullptr) {
                TESObjectREFR::AddMasterParticleAddonNodes(pRef->Get3D()->IsNode());
            }
        }
    }
    else {
        BSSimpleList<TESObjectREFR*>* pNode = GetRefList();
        while (pNode != nullptr) {
            TESObjectREFR* pRef = pNode->GetItem();
            pNode = pNode->GetNext();
            if (pRef != nullptr && pRef->GetLoadedDataFlag(LOADED_DATA_HAS_ADDON_FLAGS) && pRef->Get3D() != nullptr) {
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
        if (!pRef->IsActor() &&
            pRef->GetObjectReference()->GetFormType() != DOOR_ID &&
            pRef->GetLODTree() &&
            Get3D() != nullptr) {
            bhkWorld::Activate(Get3D(), true, true, true);
        }
    }
}

void TESObjectCELL::RemoveScriptedRef(TESObjectREFR* apRef) {
    if (pLoadedData) {
        pLoadedData->ScriptedRefs.Remove(apRef);
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
        BSSimpleList<TESObjectREFR*>* pList = &pLoadedData->ActivatingRefs;
        BSSimpleList<TESObjectREFR*>* pLastList = nullptr;
        while (pList && !pList->IsEmpty()) {
            if (pList->m_item == apRef) {
                return;
            }
            pLastList = pList;
            pList = pList->m_pkNext;
        }
        apRef->AddChange(CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN);
        if (pLastList) {
            pLastList->AddTail(apRef);
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
    return this->spSoundMarkerNode;
}

BSPortalGraph* TESObjectCELL::GetPortalGraph() const {
    return spPortalGraph.m_pObject;
}

TESObjectREFR* TESObjectCELL::GetFirstRefr() {
    CellRefLockEnter();
    TESObjectREFR* pRef = GetRefList()->m_item;
    CellRefLockLeave();
    return pRef;
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
    CFilter kFilter;
    bhkWorld* pWorld = GetbhkWorld();
    if (!pWorld || !pWorld->PickObject(arResult)) {
        return nullptr;
    }
    hkpRigidBody* pBody = arResult.GethkRigidBody();
    bhkRigidBody* pbhkBody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pBody));
    if (pbhkBody && pbhkBody->GetCollisionFilter(&kFilter)->GetLayer() == L_GROUND) {
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

void TESObjectCELL::SetTempPublic(bool abVal) {
    if (abVal) {
        cCellFlags |= kCellFlag_TempPublic;
    }
    else {
        cCellFlags &= static_cast<uint8_t>(~kCellFlag_TempPublic);
    }
    AddChange(CHANGE_CELL_FLAGS);
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
    cCellGameFlags &= ~kCellGameFlag_FullySeen;
    RemoveChange(CHANGE_CELL_SEENDATA);
}

void TESObjectCELL::SetFullySeen(bool abVal) {
    if (abVal) {
        cCellGameFlags |= kCellGameFlag_FullySeen;
    }
    else {
        cCellGameFlags &= ~kCellGameFlag_FullySeen;
    }
    AddChange(CHANGE_CELL_FLAGS);
}

// Steps to the next node before testing the item and does not test the node for emptiness.
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
    // GetWorldSpace is called again for the zone rather than keeping the first result.
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
        pWorldSpace = reinterpret_cast<TESWorldSpace*>(static_cast<uintptr_t>(aiOffset));
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
        TESObjectLAND* pLand;
        AutoMemContext kMemContext(
            MEM_CONTEXT_LAND,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
            4022
        );
        pLand = new TESObjectLAND;
        pCellLand = pLand;
        pCellLand->SetCell(this);
    }
    return pCellLand;
}

float TESObjectCELL::GetScaleBasedOnLocation(
    TESRegionPoint* apTest,
    REGION_DATA_ID aID,
    TESRegionObjectBase* apObjectBase
) {
    TESRegionList* pRegionList = GetRegionList(true);
    BSSimpleList<TESRegion*>* pCurrent = static_cast<BSSimpleList<TESRegion*>*>(pRegionList);
    if (GetInterior() || !pCurrent) {
        return 0.0f;
    }
    int32_t iCurrentOverride = -1;
    float fCurrentScale = std::numeric_limits<float>::max();
    for (; pCurrent && pCurrent->m_item; pCurrent = pCurrent->m_pkNext) {
        float fScaleRegion = 0.0f;
        TESRegionData* pData;
        if (!pCurrent->m_item->GetDelete() &&
            (pData = pCurrent->m_item->pDataList->Find(aID)) != nullptr &&
            pData->Validate()) {
            BSSimpleList<TESRegionPointList*>* pCurrentRPL;
            if (pData->bOverride) {
                if (pData->cPriority <= iCurrentOverride) {
                    continue;
                }
            }
            else if (iCurrentOverride >= 0) {
                continue;
            }
            for (pCurrentRPL = pCurrent->m_item->pPointLists;
                 pCurrentRPL && pCurrentRPL->m_item && !(fScaleRegion > 0.0);
                 pCurrentRPL = pCurrentRPL->m_pkNext) {
                float fDistanceInsideAtMax = static_cast<float>(pCurrentRPL->m_item->iDistanceInsideAtMax);
                float fTemp = pCurrentRPL->m_item->GetScaleBasedOnLocation(apTest, fDistanceInsideAtMax);
                if (fTemp > 1.0) {
                    fTemp = 1.0f;
                }
                fScaleRegion = BSMAX(fScaleRegion, fTemp);
            }
            if (!(fScaleRegion <= 0.0)) {
                if (pData->bOverride) {
                    fCurrentScale = std::numeric_limits<float>::max();
                    iCurrentOverride = pData->cPriority;
                }
                if (pData->GetID() == REGION_DATA_OBJECTS_ID &&
                    apObjectBase &&
                    apObjectBase->GetObject() &&
                    !apObjectBase->GetID()) {
                    TESRegionDataManager* pManager = pDataHandler->GetRegionDataManager();
                    TESRegionDataObjects* pDataObjects = pManager->IsRegionDataObjects(pData);
                    TESRegionObjectList* pObjectList = pDataObjects->GetObjectList();
                    // An objects entry (GetID 0) is a TESRegionObject.
                    if (!pObjectList->IsInListTOL(static_cast<TESRegionObject*>(apObjectBase), true, false)) {
                        continue;
                    }
                }
                else if (
                    pData->GetID() == REGION_DATA_GRASS_ID &&
                    apObjectBase &&
                    apObjectBase->GetObject() &&
                    apObjectBase->GetID() == 1
                ) {
                    int32_t iParentFind;
                    TESRegionDataManager* pManager = pDataHandler->GetRegionDataManager();
                    TESRegionDataGrass* pDataGrass = pManager->IsRegionDataGrass(pData);
                    // A grass entry (GetID 1) is a TESRegionGrassObject.
                    TESRegionGrassObject* pGrassObject = static_cast<TESRegionGrassObject*>(apObjectBase);
                    if (pGrassObject->GetParentTexture()) {
                        iParentFind = pGrassObject->GetParentTexture()->iFormID;
                    }
                    else {
                        iParentFind = -1;
                    }
                    uint32_t uiObjectID = pGrassObject->GetObject()->iFormID;
                    if (!pDataGrass->GetObjectList()->Find(uiObjectID, iParentFind)) {
                        continue;
                    }
                }
                while (pCurrentRPL && pCurrentRPL->m_item) {
                    float fDistanceInsideAtMax = static_cast<float>(pCurrentRPL->m_item->iDistanceInsideAtMax);
                    float fTemp = pCurrentRPL->m_item->GetScaleBasedOnLocation(apTest, fDistanceInsideAtMax);
                    if (fTemp > 1.0) {
                        fTemp = 1.0f;
                    }
                    fScaleRegion = BSMAX(fScaleRegion, fTemp);
                    pCurrentRPL = pCurrentRPL->m_pkNext;
                }
                fCurrentScale = BSMIN(fCurrentScale, fScaleRegion);
            }
        }
    }
    if (fCurrentScale < 0.0) {
        fCurrentScale = 0.0f;
    }
    if (fCurrentScale > 1.0) {
        return 0.0f;
    }
    return fCurrentScale;
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
        NavMeshRender::GetInstance()->RemoveCell(this);
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
    if (abNewbool) {
        cCellFlags |= kCellFlag_HasWater;
    }
    else {
        cCellFlags &= ~kCellFlag_HasWater;
    }
}

uint32_t TESObjectCELL::GetGroupBlockKey() {
    if (GetInterior()) {
        return static_cast<uint32_t>((TESForm::GetFormID() & 0xFFFFFFull) % 10);
    }
    int32_t dataY = GetDataY();
    int32_t dataX = GetDataX();
    return TESObjectCELL::CalcExtGroupBlockKey(dataX, dataY);
}

uint32_t TESObjectCELL::GetGroupSubBlockKey() {
    if (GetInterior()) {
        return static_cast<uint32_t>(((TESForm::GetFormID() & 0xFFFFFFull) % 100) / 10);
    }
    int32_t dataY = GetDataY();
    int32_t dataX = GetDataX();
    return TESObjectCELL::CalcExtGroupSubBlockKey(dataX, dataY);
}

void TESObjectCELL::SetInterior(bool abVal) {
    if (abVal) {
        cCellFlags |= kCellFlag_Interior;
    }
    else {
        cCellFlags &= ~kCellFlag_Interior;
    }
}

bool TESObjectCELL::CantTravelFrom() {
    uint8_t ucCantTravel = (cCellFlags & kCellFlag_CantTravelFrom) != 0;
    if (GetInterior()) {
        ucCantTravel = !ucCantTravel;
    }
    if (ucCantTravel) {
        return true;
    }
    TESWorldSpace* pWorld = GetWorldSpace();
    return pWorld && pWorld->CantTravelFrom();
}

bool TESObjectCELL::GetHideLand(uint32_t aiQuad) {
    EXTERIOR_DATA* pData = GetExteriorData();
    return (pData && (((1 << aiQuad) & static_cast<int8_t>(pData->ucLandHideFlags)) != 0));
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

    return nullptr;
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
    float fMinX = static_cast<float>(static_cast<double>(GetDataX() << 12));
    float fMaxX = static_cast<float>(fMinX + 4096.0);
    float fMinY = static_cast<float>(static_cast<double>(GetDataY() << 12));
    float fMaxY = static_cast<float>(fMinY + 4096.0);
    if (fMinX > static_cast<double>(aPos.x)) {
        if (fMaxY < static_cast<double>(aPos.y)) {
            return GetDistanceBetweenPoints(aPos.x, aPos.y, fMinX, fMaxY);
        }

        if (fMinY > static_cast<double>(aPos.y)) {
            return GetDistanceBetweenPoints(aPos.x, aPos.y, fMinX, fMinY);
        }

        return fMinX - aPos.x;
    }

    if (fMaxX < static_cast<double>(aPos.x)) {
        if (fMaxY < static_cast<double>(aPos.y)) {
            return GetDistanceBetweenPoints(aPos.x, aPos.y, fMaxX, fMaxY);
        }

        if (fMinY > static_cast<double>(aPos.y)) {
            return GetDistanceBetweenPoints(aPos.x, aPos.y, fMaxX, fMinY);
        }

        return fMinX - aPos.x;
    }

    if (fMaxY < static_cast<double>(aPos.y)) {
        return aPos.y - fMaxY;
    }

    if (fMinY > static_cast<double>(aPos.y)) {
        return fMinY - aPos.y;
    }

    return 0.0;
}

bool TESObjectCELL::GetFullySeen() {
    return ((cCellGameFlags & kCellGameFlag_FullySeen) != 0);
}

bool TESObjectCELL::HasReferences() {
    return !listReferences.IsEmpty();
}

bool TESObjectCELL::IsOffLimits() {
    return ((TESForm::GetFormFlags() & 131072) != 0);
}

bool TESObjectCELL::All3DWithVisibleDistantFadingIn() {
    return bDisplayHighDetail;
}

SeenData* TESObjectCELL::GetSeenData() {
    ExtraDataList* extra = GetExtra();
    return static_cast<SeenData*>(extra->GetSeenData());
}

void TESObjectCELL::SetAutoWaterLoaded(bool abVal) {
    bAutoWaterLoaded = abVal;
}

void TESObjectCELL::Clear3D() {
    ahkpWorld* phkWorld;
    bhkWorld* pWorld = ExtraData.GetbhkWorld();
    if (pWorld && TESObjectCELL::pIslandListener) {
        phkWorld = **pWorld;
        TESObjectCELL::pIslandListener->RemoveFromWorld(phkWorld);
    }

    SetState(TESObjectCELL::CS_UNLOADING);
    FreeLoadedData();
    ExtraData.SetbhkWorld(nullptr);
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
    NiNode* pNode;
    uint8_t ucHit = 0;
    if (apPick && acNodeFlags) {
        if ((acNodeFlags & kPickNode_Actor) != 0) {
            pNode = GetChildNode(TESObjectCELL::CN_ACTOR);
            ucHit = PickDistance(apPick, aOrigin, aDir, afDistance, pNode, false);
        }
        if ((acNodeFlags & kPickNode_Marker) != 0) {
            pNode = GetChildNode(TESObjectCELL::CN_MARKER);
            if (PickDistance(apPick, aOrigin, aDir, afDistance, pNode, false)) {
                ucHit = 1;
            }
        }
        if ((acNodeFlags & kPickNode_Land) != 0) {
            pNode = GetChildNode(TESObjectCELL::CN_LAND);
            if (PickDistance(apPick, aOrigin, aDir, afDistance, pNode, false)) {
                ucHit = 1;
            }
        }
        if ((acNodeFlags & kPickNode_Static) != 0) {
            pNode = GetChildNode(TESObjectCELL::CN_STATIC);
            if (PickDistance(apPick, aOrigin, aDir, afDistance, pNode, false)) {
                ucHit = 1;
            }
        }
        if ((acNodeFlags & kPickNode_Dynamic) != 0) {
            pNode = GetChildNode(TESObjectCELL::CN_DYNAMIC);
            if (PickDistance(apPick, aOrigin, aDir, afDistance, pNode, false)) {
                ucHit = 1;
            }
        }
        if ((acNodeFlags & kPickNode_Static) != 0 || (acNodeFlags & kPickNode_Dynamic) != 0) {
            pNode = GetChildNode(TESObjectCELL::CN_MULTIBOUND);
            if (PickDistance(apPick, aOrigin, aDir, afDistance, pNode, false)) {
                return true;
            }
        }
    }

    return ucHit;
}

void TESObjectCELL::ChangeProcessLevel(bool abChangeLevel) {
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* RefList = GetRefList();
    ReferenceProcessLists.ChangeProcessLevel(RefList, abChangeLevel);
    CellRefLockLeave();
}

void TESObjectCELL::ClearReference3D() {
    BSFadeNode* pFadeNode;
    NiNode* p3D;
    PlayerCharacter* pRef;
    CellRefLockEnter();
    ClearPortalGraph();
    BSSimpleList<TESObjectREFR*>* pIter = GetRefList();
    while (pIter) {
        pRef = static_cast<PlayerCharacter*>(pIter->m_item);
        pIter = pIter->m_pkNext;
        if (pRef && pRef != pPlayer) {
            p3D = pRef->TESObjectREFR::Get3D();
            if (p3D) {
                pFadeNode = p3D->IsFadeNode();
                if (pFadeNode) {
                    pFadeNode->TurnFadeNodeOff();
                }
            }
            pRef->Set3D(nullptr, false);
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::UnLoadAllTempData() {
    TESObjectREFR* pRef;
    CellRefLockEnter();
    this->sNumRefsWithVisibleDistant = 0;
    this->sNumLoadedRefsWithVisibleDistant = 0;
    bool bAllowChanges = pSaveLoadGame->SetThreadAllowChanges(false);
    this->RemoveNavmeshObstacles();
    BSSimpleList<TESObjectREFR*>* pCurrent = this->GetRefList();
    BSSimpleList<TESObjectREFR*>* pPrevious = nullptr;
    while (pCurrent && !pCurrent->IsEmpty()) {
        pRef = pCurrent->m_item;
        if (pRef->GetRefPersists() || (pRef->GetFile(-1) && !pRef->GetFile(-1)->GetMaster())) {
            pPrevious = pCurrent;
            pCurrent = pCurrent->m_pkNext;
        }
        else {
            if (pPrevious) {
                pPrevious->Remove(pRef);
                pCurrent = pPrevious->m_pkNext;
            }
            else {
                pCurrent->RemoveHead();
            }
            pLoadedReferenceCollection->RemoveReference(pRef);
            if (pRef->IsActor()) {
                Actor* pActor = static_cast<Actor*>(pRef);
                if (static_cast<MobileObject*>(pRef)->MobileObject::GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
                    pActor->SetLastSeenTime();
                }
            }
            pSaveLoadGameOLD->UnloadForm(pRef);
            pSaveLoadGame->UnloadForm(pRef, false);
            char cCollect = 1;
            if (pSaveLoadGame->GetSaveGameLoading()) {
                BGSChangeFlags kChangeFlags(CHANGE_REFR_CELL_CHANGED);
                if (pSaveLoadGame->GetOldChange(pRef, kChangeFlags) && pRef->MoveToEditorLocation(this)) {
                    cCollect = 0;
                }
            }
            if (pRef->GetObjectReference() && pRef->GetObjectReference()->GetFormType() == PWAT_ID) {
                BGSPlaceableWater* pWater = static_cast<BGSPlaceableWater*>(pRef->GetObjectReference());
                if (pWater->IsAutomaticWater()) {
                    this->RemovePlaceableWaterRef(pRef);
                }
            }

            if (cCollect && pRef != pPlayer) {
                GarbageCollector::Add(pRef);
            }
        }
    }
    pSaveLoadGame->SetThreadAllowChanges(bAllowChanges);
    this->CellRefLockLeave();
    TESForm* pLand = this->GetLand();
    if (pLand) {
        TESFile* pFile = pLand->GetFile(-1);
        if (!pFile || pFile->GetMaster()) {
            this->SetLand(nullptr);
            pLand = nullptr;
        }
    }
    NavMeshArray* pNavMeshes = this->GetNavMeshArray();
    if (pNavMeshes) {
        AutoMemContext kMemContext(
            MEM_CONTEXT_NAVMESH,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
            0x2769
        );
        for (uint32_t uiNavMeshIndex = 0; uiNavMeshIndex < pNavMeshes->GetNavMeshCount(); ++uiNavMeshIndex) {
            TESFile* pFile = pNavMeshes->GetNavMeshByIndex(uiNavMeshIndex)->GetFile(-1);
            if (!pFile || pFile->GetMaster()) {
                NavMeshPtr spNavMesh = pNavMeshes->GetNavMeshByIndex(uiNavMeshIndex);
                pSaveLoadGame->UnloadForm(spNavMesh, false);
                pNavMeshes->RemoveNavMeshByIndex(uiNavMeshIndex--);
            }
        }
    }
    this->SetHasTempData(false);
}

void TESObjectCELL::ClearPortalGraph() {
    if (spPortalGraph.m_pObject) {
        if (GetInterior()) {
            BSPortalGraph* pPortalGraph = BSShaderManager::GetShadowSceneNode(0)->GetPortalGraph();
            if (pPortalGraph == spPortalGraph.m_pObject) {
                BSShaderManager::GetShadowSceneNode(0)->SetPortalGraph(nullptr);
            }
        }
        spPortalGraph = static_cast<BSPortalGraph*>(nullptr);
        if (GetWorldSpace()) {
            GetWorldSpace()->ClearPortalGraph();
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
    if (waterType && TESForm::GetInitialized() && waterType->pRemappedWaterForm) {
        return waterType->pRemappedWaterForm;
    }

    return waterType;
}

TESObjectREFR* TESObjectCELL::CreateLootBagAtRef(TESObjectREFR* apRef) {
    TESObjectREFR* pBag;
    NiPoint3 kAngle;
    NiPoint3 kPosition;
    int8_t cHavePosition = 0;
    if (apRef && apRef->TESObjectREFR::GetParentCell() == this) {
        kPosition = apRef->GetLocationOnReference();
        cHavePosition = 1;
    }
    else if (pPlayer->TESObjectREFR::GetParentCell() == this) {
        kPosition = pPlayer->GetLocationOnReference();
        cHavePosition = 1;
    }
    if (!cHavePosition) {
        DetermineCOCPlacement(kPosition, kAngle);
    }
    pBag = new TESObjectREFR;
    pBag->SetLocationOnReference(kPosition);
    pBag->SetObjectReference(pDefLootBag);
    pBag->SetOwner(pPlayer->TESObjectREFR::GetObjectReference());
    FillLootBag(pBag);
    AddReference(pBag, false);
    return pBag;
}

IntSeenData* TESObjectCELL::GetIntSeenSection(int32_t aiSectionX, int32_t aiSectionY, bool abCreate) {
    SeenData* pSeenData;
    IntSeenData* pNewSection;
    IntSeenData* pPrevSection;
    IntSeenData* pSection = nullptr;
    if (GetInterior()) {
        pSeenData = GetSeenData();
        pSection = dynamic_cast<IntSeenData*>(pSeenData);
        pPrevSection = nullptr;
        while (pSection && (pSection->cSectionX != aiSectionX || pSection->cSectionY != aiSectionY)) {
            pPrevSection = pSection;
            pSection = pSection->pNextSection;
        }
        if (!pSection && abCreate) {
            if (pPrevSection) {
                pNewSection = new IntSeenData(aiSectionX, aiSectionY);
                pPrevSection->pNextSection = pNewSection;
                return pPrevSection->pNextSection;
            }

            pSection = new IntSeenData(aiSectionX, aiSectionY);
            GetExtra()->SetSeenData(pSection);
            AddChange(CHANGE_CELL_SEENDATA);
        }
    }
    return pSection;
}

void TESObjectCELL::RemovePlaceableWaterRef(TESObjectREFR* apWaterRef) {
    BSSimpleList<TESObjectREFR*>* pIter;
    TESObjectREFR* pRef;
    for (pIter = GetRefList(); pIter && pIter->m_item; pIter = pIter->m_pkNext) {
        pRef = pIter->m_item;
        if (pRef->GetRelevantWaterType() == apWaterRef) {
            pRef->SetRelevantWaterType(nullptr);
        }
    }
    if (pPlayer->GetRelevantWaterType() == apWaterRef) {
        pPlayer->SetRelevantWaterType(nullptr);
    }
}

bool TESObjectCELL::IsActorBaseCellOwner(TESActorBase* apActorBase) {
    TESNPC* pBaseNPC;
    int32_t iRank;
    TESFaction* pFaction;
    TESNPC* pNPC;
    TESForm* pOwner = GetOwner();
    if (pOwner) {
        pNPC = dynamic_cast<TESNPC*>(pOwner);
        if (pNPC) {
            return static_cast<const void*>(pNPC) == static_cast<const void*>(apActorBase);
        }

        pFaction = dynamic_cast<TESFaction*>(pOwner);
        if (pFaction) {
            iRank = GetOwnershipRank();
            pBaseNPC = dynamic_cast<TESNPC*>(apActorBase);
            if (pBaseNPC) {
                return static_cast<TESActorBaseData*>(pBaseNPC)->GetFactionRank(pFaction, false) >= iRank;
            }
        }
    }
    return false;
}

void TESObjectCELL::RemoveWaterRef(TESObjectREFR* apRef) {
    if (pLoadedData) {
        pLoadedData->WaterRefs.Remove(apRef);
    }
}

void TESObjectCELL::RemoveReferencesFromLoadedReferenceCollection() {
    TESBoundObject* pObject;
    TESWorldSpace* pWorld;
    TESObjectREFR* pRef;
    BSSimpleList<TESObjectREFR*>* pIter;
    CellRefLockEnter();
    for (pIter = GetRefList(); pIter; pIter = pIter->m_pkNext) {
        pRef = pIter->m_item;
        if (pRef) {
            pLoadedReferenceCollection->RemoveReference(pRef);
            if (!TES::pInstance->TES::GetInterior() && TES::pInstance->GetWorldSpace()) {
                pObject = pRef->TESObjectREFR::GetObjectReference();
                if (pObject->TESForm::GetHasSpokenFlag()) {
                    pWorld = pRef->GetWorldSpace();
                    if (pWorld) {
                        if (pRef->GetDisabled()) {
                            pWorld->GetTerrainManager()->HideTree(pRef, true);
                        }
                        else {
                            pWorld->GetTerrainManager()->ShowTree(pRef);
                        }
                        pWorld->GetTerrainManager()->SetNeedsImmediateUpdate(true);
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
        for (TESObjectREFR* pRef : *GetRefList()) {
            if (pRef && !pRef->GetDisabled() && !pRef->GetDelete()) {
                TESObjectDOOR* pDoor = static_cast<TESObjectDOOR*>(pRef->GetObjectReference());
                if (pDoor->GetFormType() == 28 && pDoor != pJailMarker) {
                    TESObjectDOOR* pBaseDoor = static_cast<TESObjectDOOR*>(pRef->GetObjectReference());
                    if (pRef->GetTeleport() || pBaseDoor->IsRandomDoor()) {
                        apOutList->AddHead(pRef);
                    }
                }
            }
        }
        CellRefLockLeave();
    }
}

void TESObjectCELL::GetExteriorLocalMapTexture(NiPointer<NiTexture>& aspTexture) {
    NiPointer<NiTexture> spRetTex(static_cast<NiTexture*>(nullptr));
    if (!GetInterior()) {
        NiPointer<BSRenderedTexture> spRT(static_cast<BSRenderedTexture*>(nullptr));
        TakeExteriorLocalMapPicture(spRT);
        spRetTex = static_cast<NiTexture*>(spRT->GetTexture(0));
        spRT->ClearRenderedTextures();
    }
    aspTexture = spRetTex;
}

void TESObjectCELL::GetInteriorLocalMapTexture(
    int32_t aiSectionX,
    int32_t aiSectionY,
    NiPointer<NiTexture>& aspTexture
) {
    NiPointer<NiTexture> spRetTex(static_cast<NiTexture*>(nullptr));
    if (GetInterior()) {
        NiPointer<BSRenderedTexture> spRT(static_cast<BSRenderedTexture*>(nullptr));
        TakeInteriorLocalMapPicture(aiSectionX, aiSectionY, spRT);
        spRetTex = static_cast<NiTexture*>(spRT->GetTexture(0));
        spRT->ClearRenderedTextures();
    }
    aspTexture = spRetTex;
}

void TESObjectCELL::SaveReferences(TESFile* apFile) {
    TESObjectREFR* pRef;
    NiTPrimitiveArray<TESObjectREFR*> kSaveRefs(0, 1);
    kSaveRefs.SetSize(50u);
    kSaveRefs.m_usGrowBy = 50;
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* pList = GetRefList();
    while (pList && !pList->IsEmpty()) {
        pRef = pList->m_item;
        pList = pList->m_pkNext;
        bool bSaveRef = !pRef->GetTemporary();
        if (pRef->GetDelete()) {
            TESFile* pFile = pRef->GetFile(0);
            if (pFile && !pFile->GetMaster()) {
                bSaveRef = false;
            }
        }
        if (apFile != pRef->GetFile(-1) && !pRef->GetAltered()) {
            bSaveRef = false;
        }
        if (!GetInterior() && pRef->GetRefPersists() && !GetPersistentCell()) {
            bSaveRef = false;
        }
        if (bSaveRef) {
            kSaveRefs.AddFirstEmpty(pRef);
        }
    }
    CellRefLockLeave();
    kSaveRefs.Compact();
    int32_t iSize = kSaveRefs.m_usESize;
    for (int32_t n = 0; n < iSize; ++n) {
        TESObjectREFR* pFirst = kSaveRefs[n];
        for (int32_t m = n + 1; m < iSize; ++m) {
            TESObjectREFR* pSecond = kSaveRefs[m];
            if (pSecond->SavesBefore(pFirst)) {
                kSaveRefs.SetAt(n, pSecond);
                kSaveRefs.SetAt(m, pFirst);
                pFirst = pSecond;
            }
        }
    }
    for (int32_t i = 0; i < iSize; ++i) {
        pRef = kSaveRefs[i];
        if (pRef) {
            pRef->TESForm::Save(apFile);
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
    if (apBarterContainer && apActor) {
        TESObjectREFR* pMerchant = apActor->m_Extra.GetMerchantContainer();
        CellRefLockEnter();
        for (TESObjectREFR*& rpItem : GetRefList()->ItemsUntilNull()) {
            InventoryChanges* pActorList = nullptr;
            TESObjectREFR* pRef = rpItem;
            ExtraDataList* pExtra = nullptr;
            if (!pRef->GetDisabled() &&
                !pRef->GetDelete() &&
                pRef->GetOwner() &&
                pRef->IsAnOwner(static_cast<Actor*>(apActor), false) &&
                pRef != pMerchant) {
                switch (pRef->GetObjectReference()->GetFormType()) {
                    case CCRD_ID: {
                        // This address (&pRef->m_Extra) is tested although it is never null.
                        pRef->m_Extra.SetCount(1);
                        [[fallthrough]];
                    }
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
                        ExtraDataList* pNewExtra;
                        ItemChange* pItem;
                        pItem = new ItemChange(pRef->GetObjectReference(), 0);
                        if (!pItem->pExtraObjectList) {
                            BSSimpleList<ExtraDataList*>* pList = new BSSimpleList<ExtraDataList*>;
                            pItem->pExtraObjectList = pList;
                        }
                        pNewExtra = new ExtraDataList;
                        pExtra = pNewExtra;
                        pExtra->DuplicateExtraListForContainer(&pRef->m_Extra);
                        pExtra->SetOriginalReferenceExtra(pRef);
                        pExtra->RemoveOwnership();
                        pItem->pExtraObjectList->AddHead(pExtra);
                        pItem->iNumber = pRef->m_Extra.GetCount();
                        apBarterContainer->AddChangeItem(pItem, true);
                        break;
                    }
                    case CONT_ID: {
                        pActorList = pRef->m_Extra.GetContainerChanges();
                        if (pActorList) {
                            pActorList->CreateCopyofList(apBarterContainer, pRef, false);
                        }
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
        if (pref && (pref != pPlayer)) {
            Actor* pactor = dynamic_cast<Actor*>(pref);
            InventoryChanges* pinventory = pref->m_Extra.GetContainerChanges();
            if (pinventory) {
                pinventory->RemoveAllItems(pref, pPlayer, false, false, true, false, -1, nullptr);
            }
            if (pactor && (!pactor->GetDisabled() && !pactor->GetDelete())) {
                pactor->ModifyDispositionTowardActor(pPlayer, 10.0f);
            }

            if (pactor &&
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
    BSSimpleList<TESObjectREFR*>* pIter = GetRefList();
    while (pIter) {
        TESObjectREFR* pRef = pIter->m_item;
        pIter = pIter->m_pkNext;
        if (pRef) {
            if (pRef->GetObjectReference()->GetFormType() == 30) {
                LightRefData* pLight = pRef->GetExtra()->GetLight();
                if (pLight) {
                    TESObjectLIGH* pLightBase = static_cast<TESObjectLIGH*>(pRef->GetObjectReference());
                    pLightBase->Update(pLight, nullptr);
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
    for (BSSimpleList<TESObjectREFR*>* pList = GetRefList(); pList && !pList->IsEmpty(); pList = GetRefList()) {
        PlayerCharacter* pRef = static_cast<PlayerCharacter*>(pList->m_item);
        if (pRef == pPlayer) {
            GetRefList()->RemoveHead();
            pRef->SetParentCell(nullptr);
        }
        else {
            bool bNotClearing = false;
            if (GetPersistentCell() && pDataHandler) {
                bNotClearing = !pDataHandler->GetClearingData();
            }

            if (!pRef->GetRefPersists() || GetInterior() || bNotClearing) {
                if (pRef->GetObjectReference() &&
                    pRef->GetObjectReference()->GetFormType() == PWAT_ID &&
                    static_cast<BGSPlaceableWater*>(pRef->GetObjectReference())
                        ->BGSPlaceableWater::IsAutomaticWater()) {
                    RemovePlaceableWaterRef(pRef);
                }

                GarbageCollector::Add(pRef);
            }
            else if (GetPersistentCell()) {
                if (pRef) {
                    delete pRef;
                }
            }
            else {
                pRef->SetParentCell(nullptr);
            }
            GetRefList()->RemoveHead();
        }
    }
    CellRefLockLeave();
}

TESObjectREFR* TESObjectCELL::SearchForm(TESBoundObject* apObject, char acSkipDeleted) {
    if (!apObject) {
        return nullptr;
    }
    CellRefLockEnter();
    BSSimpleList<TESObjectREFR*>* pIter = GetRefList();
    while (pIter && !pIter->IsEmpty()) {
        TESObjectREFR* pRef = pIter->m_item;
        TESBoundObject* pObject = pRef->GetObjectReference();
        pIter = pIter->m_pkNext;
        if (pObject && (!acSkipDeleted || !pRef->GetDelete()) && pObject == apObject) {
            CellRefLockLeave();
            return pRef;
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
    float fRadiusSqr = afRadius * afRadius;
    CellRefLockEnter();
    for (TESObjectREFR* pRef : *GetRefList()) {
        if (pRef && !pRef->GetDisabledOrDeleted()) {
            TESForm* pObject = pRef->GetObjectReference();
            if (pObject->GetFormType() == 28 && pObject != pJailMarker && pRef->GetTeleport()) {
                NiPoint3 kDelta;
                kDelta = arLocation - pRef->GetLocationOnReference();
                double dDistSqr = kDelta.SqrLength();
                if (fRadiusSqr > dDistSqr) {
                    apRetDoors->Add(pRef);
                }
            }
        }
    }
    CellRefLockLeave();
    return apRetDoors->iSize;
}

void TESObjectCELL::TakeLocalMapPicture(NiCamera* apCamera, NiPointer<BSRenderedTexture>& aspRenderedTexture) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_LOCALMAP,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
        0x2251
    );
    NiPointer<BSShaderAccumulator> spOldAccum(BSShaderManager::GetAccumulator());
    BSCullingProcess kCuller(nullptr);
    NiPointer<BSRenderedTexture> spRendTextColor(static_cast<BSRenderedTexture*>(nullptr));
    NiPointer<BSRenderedTexture> spRendTextShader(static_cast<BSRenderedTexture*>(nullptr));
    NiPointer<BSRenderedTexture> spRendTextNormals(static_cast<BSRenderedTexture*>(nullptr));
    NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
    BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
    spRendTextNormals = pTextureManager
                            ->BorrowRenderedTexture(pRenderer, BSTextureManager::TEX_LOCALMAP_NORMALS, 0, nullptr, 0);
    pRenderer = BSShaderManager::GetRenderer();
    pTextureManager = BSShaderManager::TextureManagerPtr();
    spRendTextShader = pTextureManager
                           ->CreateRenderedTexture(pRenderer, BSTextureManager::TEX_LOCALMAP_FINAL, nullptr, 0);
    if (spRendTextShader.m_pObject && spRendTextNormals.m_pObject) {
        void* pCustomAccumulator;
        NiColorA kOldColor(0.0f, 0.0f, 0.0f, 0.0f);
        BSShaderManager::GetRenderer()->GetBackgroundColor(kOldColor);
        BSShaderManager::GetRenderer()->SetBackgroundColorAlpha(NiColorA(0.0f, 0.0f, 0.0f, 1.0f));
        ShadowSceneNode* pScene = BSShaderManager::GetShadowSceneNode(0);
        bool bWasLightUpdateDisabled = pScene->bDisableLightUpdate;
        pScene->bDisableLightUpdate = true;
        bool bOutsideFrame = BSRenderedTexture::QOutsideFrame();
        NiNode* pWaterNode = TESWaterSystem::GetWaterRoot();
        bool bWaterCulled = false;
        bool bTreesEnabled = true;
        if (pWaterNode) {
            bWaterCulled = pWaterNode->GetAppCulled();
            pWaterNode->SetAppCulled(true);
        }
        bTreesEnabled = BSTreeManager::GetTreesEnabled();
        BSTreeManager::SetTreesEnabled(false);
        if (bOutsideFrame) {
            BSRenderedTexture::StartOffscreen(7u, spRendTextNormals->GetGroup());
        }
        else {
            BSRenderedTexture::BeginTexture(spRendTextNormals.m_pObject, 7u);
        }
        pCustomAccumulator = new BSShaderAccumulator(99, 1u, 759u);
        NiPointer<BSShaderAccumulator> spAccum(static_cast<BSShaderAccumulator*>(pCustomAccumulator));
        spAccum->SetShadowSceneNode(pScene);
        kCuller.SetAccumulator(spAccum.m_pObject);
        spAccum->SetRenderMode(BSShaderManager::BSSM_RENDER_LOCALMAP);
        kCuller.PushCullMode(BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS);
        BSShaderUtil::AccumulateScene(apCamera, pScene, kCuller);
        kCuller.PopCullMode();
        if (GetChildNode(TESObjectCELL::CN_PORTAL)) {
            kCuller.PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
            ShadowSceneNode* pPortalScene = static_cast<ShadowSceneNode*>(GetChildNode(TESObjectCELL::CN_PORTAL));
            BSShaderUtil::AccumulateScene(apCamera, pPortalScene, kCuller);
            kCuller.PopCullMode();
        }
        BSShaderUtil::RenderScene(apCamera, spAccum.m_pObject, false);
        kCuller.SetAccumulator(nullptr);
        spAccum = static_cast<BSShaderAccumulator*>(nullptr);
        if (bOutsideFrame) {
            BSRenderedTexture::StopOffscreen();
        }
        else {
            BSRenderedTexture::End();
        }
        BSTreeManager::SetTreesEnabled(bTreesEnabled);
        if (pWaterNode) {
            pWaterNode->SetAppCulled(bWaterCulled);
        }
        if (bOutsideFrame) {
            BSRenderedTexture::StartOffscreen(7u, spRendTextShader->GetGroup());
        }
        else {
            BSRenderedTexture::BeginTexture(spRendTextShader.m_pObject, 7u);
        }
        ImageSpaceManager* pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
        ImageSpaceEffect* pEffect = pImageSpaceManager->GetEffect(ImageSpaceManager::EFFECT_MAP);
        BSRenderedTexture* pShaderTexture = spRendTextShader.m_pObject;
        BSRenderedTexture* pNormalsTexture = spRendTextNormals.m_pObject;
        pRenderer = BSShaderManager::GetRenderer();
        pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
        pImageSpaceManager->RenderEffect(pEffect, pRenderer, pNormalsTexture, pShaderTexture, nullptr, 0);
        if (bOutsideFrame) {
            BSRenderedTexture::StopOffscreen();
        }
        else {
            BSRenderedTexture::End();
        }
        pScene->bDisableLightUpdate = bWasLightUpdateDisabled;
        BSShaderManager::GetAccumulator()->bRenderNoShader = true;
        BSShaderManager::GetRenderer()->SetBackgroundColorAlpha(kOldColor);
    }

    BSRenderedTexture* pNormals = spRendTextNormals.m_pObject;
    BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(pNormals);
    aspRenderedTexture = spRendTextShader;
    BSShaderManager::SetAccumulator(spOldAccum.m_pObject);
}

void TESObjectCELL::TakeExteriorLocalMapPicture(NiPointer<BSRenderedTexture>& aspRenderedTexture) {
    NiPoint2 kMinMax;
    if (GetInterior()) {
        aspRenderedTexture = static_cast<BSRenderedTexture*>(nullptr);
    }
    else {
        NiNode* pCameraNode;
        NiCamera* pCellCamera;
        NiMatrix3 kCamRot;
        NiPoint2 kHeightExtents;
        NiNode* pScene = BSShaderManager::GetShadowSceneNode(0);
        NiPoint3 kCamPos = NiPoint3::ZERO;
        kCamRot.FromEulerAnglesXYZ(-1.5707964f, -1.5707964f, 1.5707964f);
        NiFrustum kFrustum(false);
        kFrustum.m_fNear = 100.0f;
        kFrustum.m_bOrtho = true;
        TESObjectLAND* pLand = GetLand();
        NiPoint2* pMinMax = ::new (&kMinMax) NiPoint2(pLand->GetMinMaxLandHeight());
        kHeightExtents.x = pMinMax->x;
        kHeightExtents.y = pMinMax->y;
        kCamPos.x = static_cast<float>((GetDataX() << 12) + 2176);
        kCamPos.y = static_cast<double>((GetDataY() << 12) + 2176) - 32.0;
        kCamPos.z = kHeightExtents.y + 20000.0;
        kFrustum.m_fLeft = -2176.0f;
        kFrustum.m_fRight = 2176.0f;
        kFrustum.m_fTop = 2176.0f;
        kFrustum.m_fBottom = -2176.0f;
        kFrustum.m_fFar = kCamPos.z - kHeightExtents.x + 10.0;
        pCellCamera = new NiCamera;
        NiPointer<NiCamera> spCamera(pCellCamera);
        pCameraNode = new NiNode(0);
        NiPointer<NiNode> spCameraNode(pCameraNode);
        spCameraNode->AttachChild(spCamera.m_pObject, 1);
        spCameraNode->SetTranslate(kCamPos);
        pScene->AttachChild(spCameraNode.m_pObject, 1);
        spCamera->SetRotate(kCamRot);
        spCamera->SetViewFrustum(kFrustum);
        NiUpdateData kUpdateData1(0.0f, false, false);
        spCameraNode->Update(kUpdateData1);
        NiUpdateData kUpdateData2(0.0f, false, false);
        spCamera->Update(kUpdateData2);
        BSTreeManager::SetForceFullLOD(true);
        BSTreeManager::Update(spCamera.m_pObject, true);
        UpdateTrees();
        bool bActorCulled = false;
        bool bMarkerCulled = false;
        bool bLandCulled = false;
        NiNode* pActorNode = GetChildNode(TESObjectCELL::CN_ACTOR);
        if (pActorNode) {
            bActorCulled = pActorNode->GetAppCulled();
            pActorNode->SetAppCulled(true);
        }
        NiNode* pMarkerNode = GetChildNode(TESObjectCELL::CN_MARKER);
        if (pMarkerNode) {
            bMarkerCulled = pMarkerNode->GetAppCulled();
            pMarkerNode->SetAppCulled(true);
        }
        NiNode* pLandNode = GetChildNode(TESObjectCELL::CN_LAND);
        if (pLandNode) {
            bLandCulled = pLandNode->GetAppCulled();
            pLandNode->SetAppCulled(true);
        }
        NiNode* pSkyNode = Sky::GetInstance()->GetRoot();
        bool bSkyCulled = false;
        if (pSkyNode) {
            bSkyCulled = pSkyNode->GetAppCulled();
            pSkyNode->SetAppCulled(true);
        }
        NiNode* pWeatherNode = Main::WeatherRootNode();
        bool bWeatherCulled = false;
        if (pSkyNode) {
            bWeatherCulled = pWeatherNode->GetAppCulled();
            pWeatherNode->SetAppCulled(true);
        }
        bool bGrassCulled = false;
        NiAVObject* pGNode = TESTerrainLODManager::GetGrassNode();
        if (pGNode) {
            bGrassCulled = pGNode->GetAppCulled();
            pGNode->SetAppCulled(true);
        }
        bool bFadeEnabled = BSFadeNode::IsFadeEnabled();
        BSFadeNode::SetFadeEnabled(false);
        NiPointer<BSRenderedTexture> spTexture(static_cast<BSRenderedTexture*>(nullptr));
        TakeLocalMapPicture(spCamera.m_pObject, spTexture);
        BSFadeNode::SetFadeEnabled(bFadeEnabled);
        if (pGNode) {
            pGNode->SetAppCulled(bGrassCulled);
        }
        if (pWeatherNode) {
            pWeatherNode->SetAppCulled(bWeatherCulled);
        }
        if (pSkyNode) {
            pSkyNode->SetAppCulled(bSkyCulled);
        }
        if (pActorNode) {
            pActorNode->SetAppCulled(bActorCulled);
        }
        if (pMarkerNode) {
            pMarkerNode->SetAppCulled(bMarkerCulled);
        }
        if (pLandNode) {
            pLandNode->SetAppCulled(bLandCulled);
        }
        BSTreeManager::Update(Main::WorldRootCamera(), true);
        BSTreeManager::SetForceFullLOD(false);
        UpdateTrees();
        pScene->DetachChild(spCameraNode.m_pObject);
        spCameraNode = static_cast<NiNode*>(nullptr);
        spCamera = static_cast<NiCamera*>(nullptr);
        aspRenderedTexture = spTexture;
    }
}

void TESObjectCELL::TakeInteriorLocalMapPicture(
    int32_t aiSectionX,
    int32_t aiSectionY,
    NiPointer<BSRenderedTexture>& aspRenderedTexture
) {
    NiMatrix3 kRotated;
    NiMatrix3 kRotMat;
    NiBound kCellBound;
    NiMatrix3 kCamRot;
    if (GetInterior() && Get3D()) {
        NiNode* pCameraNode;
        void* pCamera;
        NiNode* pScene = BSShaderManager::GetShadowSceneNode(0);
        NiPoint3 kCamPos = NiPoint3::ZERO;
        float fZAngle = 3.1415927f / 2.0;
        float fXAngle = -3.1415927f / 2.0;
        kCamRot.FromEulerAnglesXYZ(fXAngle, fXAngle, fZAngle);
        NiFrustum kFrustum(false);
        kFrustum.m_fNear = 100.0f;
        kFrustum.m_bOrtho = true;
        kCellBound = Get3D()->GetWorldBound();
        float fMinZ = kCellBound.m_kCenter.z - kCellBound.m_fRadius;
        float fMaxZ = kCellBound.m_fRadius + kCellBound.m_kCenter.z;
        kCamPos.x = static_cast<float>((aiSectionX << 12) + 0x1080);
        kCamPos.y = static_cast<float>((aiSectionY << 12) + 0x1080);
        kCamPos.z = fMaxZ + 40000.0;
        AdjustCoordForNorthRotation(kCamPos, kCamPos, false);
        kFrustum.m_fLeft = -2176.0f;
        kFrustum.m_fRight = 2176.0f;
        kFrustum.m_fTop = 2176.0f;
        kFrustum.m_fBottom = -2176.0f;
        kFrustum.m_fFar = kCamPos.z - fMinZ + 10000.0;
        pCamera = new NiCamera;
        NiPointer<NiCamera> spCamera(static_cast<NiCamera*>(pCamera));
        pCameraNode = new NiNode(0);
        NiPointer<NiNode> spCameraNode(pCameraNode);
        spCameraNode->AttachChild(spCamera.m_pObject, 1);
        spCameraNode->SetTranslate(kCamPos);
        pScene->AttachChild(spCameraNode.m_pObject, 1);
        float fNorthRotate = 0.0f;
        TESObjectREFR* pNorthMarkerRef = SearchForm(pNorthMarker, 1);
        if (pNorthMarkerRef) {
            fNorthRotate = pNorthMarkerRef->GetAngleOnReference().z;
        }
        if (fNorthRotate != 0.0) {
            kRotMat = NiMatrix3::IDENTITY;
            kRotMat.MakeZRotation(fNorthRotate);
            kCamRot = *::new (&kRotated) NiMatrix3(kRotMat * kCamRot);
        }
        spCamera->SetRotate(kCamRot);
        spCamera->SetViewFrustum(kFrustum);
        NiUpdateData kUpdateData1(0.0f, false, false);
        spCameraNode->Update(kUpdateData1);
        NiUpdateData kUpdateData2(0.0f, false, false);
        spCamera->Update(kUpdateData2);
        bool bActorCulled = false;
        bool bMarkerCulled = false;
        NiNode* pActorNode = GetChildNode(TESObjectCELL::CN_ACTOR);
        if (pActorNode) {
            bActorCulled = pActorNode->GetAppCulled();
            pActorNode->SetAppCulled(true);
        }
        NiNode* pMarkerNode = GetChildNode(TESObjectCELL::CN_MARKER);
        if (pMarkerNode) {
            bMarkerCulled = pMarkerNode->GetAppCulled();
            pMarkerNode->SetAppCulled(true);
        }
        NiNode* pWaterNode = TESWaterSystem::GetWaterRoot();
        bool bWaterCulled = false;
        if (pWaterNode) {
            bWaterCulled = pWaterNode->GetAppCulled();
            pWaterNode->SetAppCulled(true);
        }
        bool bFadeEnabled = BSFadeNode::IsFadeEnabled();
        BSFadeNode::SetFadeEnabled(false);
        NiPointer<BSRenderedTexture> spTexture(static_cast<BSRenderedTexture*>(nullptr));
        TakeLocalMapPicture(spCamera.m_pObject, spTexture);
        BSFadeNode::SetFadeEnabled(bFadeEnabled);
        if (pWaterNode) {
            pWaterNode->SetAppCulled(bWaterCulled);
        }
        if (pActorNode) {
            pActorNode->SetAppCulled(bActorCulled);
        }
        if (pMarkerNode) {
            pMarkerNode->SetAppCulled(bMarkerCulled);
        }
        pScene->DetachChild(spCameraNode.m_pObject);
        spCameraNode = static_cast<NiNode*>(nullptr);
        spCamera = static_cast<NiCamera*>(nullptr);
        aspRenderedTexture = spTexture;
    }
    else {
        aspRenderedTexture = static_cast<BSRenderedTexture*>(nullptr);
    }
}

void TESObjectCELL::DetachHavok() {
    if (GetInterior()) {
        bhkWorld* pWorld = GetbhkWorld();
        if (pWorld) {
            pWorld->SetEnabled(false);
            pWorld->SetVisualDebugger(false);
            if (pWorld->pAcousticSpaceListener) {
                BGSAcousticSpaceListener* pListener = pWorld->pAcousticSpaceListener;
                pListener->DequeueAllRemovals(true);
                pListener->bBufferRemovalOperations = false;
            }
        }
    }
    else {
        CellMopp* pCellMopp = GetCellMopp();
        if (pCellMopp) {
            pCellMopp->Remove();
        }
        CellRefLockEnter();
        BSSimpleList<TESObjectREFR*>* pList = GetRefList();
        while (pList) {
            TESObjectREFR* pObj = pList->m_item;
            pList = pList->m_pkNext;
            if (pObj) {
                pObj->DetachHavok();
            }
        }
        bhkWorld* pWorld = GetbhkWorld();
        if (pWorld) {
            pWorld->pAcousticSpaceListener->bBufferRemovalOperations = false;
        }
        if (pSaveLoadManager->IsLoadingGame()) {
            pWorld->ProcessBatchRemovals(false);
        }
        CellRefLockLeave();
    }
}

void TESObjectCELL::UpdateSeenData(const NiPoint3& aCoord) {
    NiPoint3 kStartXY;
    NiPoint3 kStartY;
    NiPoint3 kStartX;
    NiPoint3 kStart0;
    NiPoint3 kStart;
    if (GetInterior()) {
        NiPoint3 kShiftCoord = aCoord;
        if (GetNorthRotation() == 0.0) {
            IntSeenData* pData[4];
            NiPoint3 pStart[4];
            ourMemset(pData, 0, sizeof(pData));
            ourMemset(pStart, 0, sizeof(pStart));
            int32_t iSectionX = (FloatToInt(kShiftCoord.x) - 2048) >> 12;
            int32_t iSectionY = (FloatToInt(kShiftCoord.y) - 2048) >> 12;
            pData[0] = GetIntSeenSection(iSectionX, iSectionY, true);
            pStart[0] = *::new (&kStart0) NiPoint3(
                static_cast<float>((iSectionX << 12) + 2048),
                static_cast<float>((iSectionY << 12) + 2048),
                kShiftCoord.z
            );
            NiPoint2 kSectionMin(
                static_cast<float>((iSectionX << 12) + 2048),
                static_cast<float>((iSectionY << 12) + 2048)
            );
            NiPoint2 kSectionMax(
                static_cast<float>(((iSectionX + 0x1000) << 12) + 2048),
                static_cast<float>(((iSectionY + 0x1000) << 12) + 2048)
            );
            int32_t iXShift = 0;
            int32_t iYShift = 0;
            if (kSectionMin.x >= kShiftCoord.x - TESObjectCELL::fSeenDataUpdateRadius.Float()) {
                iXShift = -1;
            }
            else if (kSectionMax.x <= kShiftCoord.x + TESObjectCELL::fSeenDataUpdateRadius.Float()) {
                iXShift = 1;
            }
            if (kSectionMin.y >= kShiftCoord.y - TESObjectCELL::fSeenDataUpdateRadius.Float()) {
                iYShift = -1;
            }
            else if (kSectionMax.y <= kShiftCoord.y + TESObjectCELL::fSeenDataUpdateRadius.Float()) {
                iYShift = 1;
            }
            int32_t iIndex = 1;
            if (iXShift) {
                pStart[iIndex] = *::new (&kStartX) NiPoint3(
                    static_cast<float>(((iXShift + iSectionX) << 12) + 2048),
                    static_cast<float>((iSectionY << 12) + 2048),
                    kShiftCoord.z
                );
                pData[iIndex++] = GetIntSeenSection(iXShift + iSectionX, iSectionY, true);
            }
            if (iYShift) {
                pStart[iIndex] = *::new (&kStartY) NiPoint3(
                    static_cast<float>((iSectionX << 12) + 2048),
                    static_cast<float>(((iYShift + iSectionY) << 12) + 2048),
                    kShiftCoord.z
                );
                pData[iIndex++] = GetIntSeenSection(iSectionX, iYShift + iSectionY, true);
            }
            if (iXShift && iYShift) {
                pStart[iIndex] = *::new (&kStartXY) NiPoint3(
                    static_cast<float>(((iXShift + iSectionX) << 12) + 2048),
                    static_cast<float>(((iYShift + iSectionY) << 12) + 2048),
                    kShiftCoord.z
                );
                pData[iIndex++] = GetIntSeenSection(iXShift + iSectionX, iYShift + iSectionY, true);
            }

            for (int32_t i = 0; i < 4 && pData[i]; ++i) {
                pData[i]->UpdateSeenData(kShiftCoord, pStart[i]);
            }
        }
        else {
            IntSeenData* pData[9];
            NiPoint3 pStart[9];
            AdjustCoordForNorthRotation(aCoord, kShiftCoord, true);
            ourMemset(pData, 0, sizeof(pData));
            ourMemset(pStart, 0, sizeof(pStart));
            int32_t iSectionX = (FloatToInt(kShiftCoord.x) - 2048) >> 12;
            int32_t iSectionY = (FloatToInt(kShiftCoord.y) - 2048) >> 12;
            int32_t iIndex = 0;
            for (int32_t idx = -1; idx <= 1; ++idx) {
                for (int32_t idy = -1; idy <= 1; ++idy) {
                    pData[iIndex] = GetIntSeenSection(idx + iSectionX, idy + iSectionY, true);
                    pStart[iIndex++] = *::new (&kStart) NiPoint3(
                        static_cast<float>(((idx + iSectionX) << 12) + 2048),
                        static_cast<float>(((idy + iSectionY) << 12) + 2048),
                        kShiftCoord.z
                    );
                }
            }
            for (int32_t i = 0; i < 9 && pData[i]; ++i) {
                pData[i]->UpdateSeenData(kShiftCoord, pStart[i]);
            }
        }
    }
    else if (!GetFullySeen()) {
        SeenData* pSeenData = GetSeenData();
        if (!pSeenData) {
            SeenData* pNewSeenData;
            pNewSeenData = new SeenData;
            pSeenData = pNewSeenData;
            GetExtra()->SetSeenData(pNewSeenData);
            AddChange(CHANGE_CELL_SEENDATA);
        }
        float fZ = aCoord.z;
        float fY = static_cast<float>(GetDataY() << 12);
        float fX = static_cast<float>(GetDataX() << 12);
        NiPoint3 kCellStart(fX, fY, fZ);
        if (pSeenData->UpdateSeenData(aCoord, kCellStart)) {
            ClearSeenData();
            SetFullySeen(true);
        }
    }
}

void TESObjectCELL::UpdateTrees() {
    CellRefLockEnter();
    for (TESObjectREFR* pRef : *GetRefList()) {
        if (pRef && !pRef->HasTemp3D()) {
            if (pRef->GetObjectReference()->GetFormType() == 37 && pRef->Get3D()) {
                BSTreeNode* pTreeNode = static_cast<BSTreeNode*>(pRef->Get3D());
                pTreeNode->GetModel()->Update(pTreeNode);
            }
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::RemoveNavmeshObstacles() {
    CellRefLockEnter();
    for (TESObjectREFR* pRef : *GetRefList()) {
        if (pRef) {
            if (pRef->IsObstacle()) {
                NavMeshObstacleManager::GetInstance()->RemoveObstacleForReference(pRef);
            }
            if (pRef->GetObjectReference()->GetFormType() == 28) {
                NavMeshObstacleManager::GetInstance()->OnDoorRemove(pRef);
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
        if (pRef && (pRef != pPlayer)) {
            NiAVObject* pNav = pRef->TESObjectREFR::Get3D();
            if (pNav) {
                BSFadeNode* pfadenode = pNav->IsFadeNode();
                if (pfadenode && (pfadenode->GetMultType() == LOD_MULT_DISTANTLOD)) {
                    pfadenode->TurnFadeNodeOff();
                }
            }
        }
    }
    CellRefLockLeave();
}

void TESObjectCELL::SetPublic(bool abVal) {
    if (abVal) {
        cCellFlags |= kCellFlag_Public;
    }
    else {
        cCellFlags &= ~kCellFlag_Public;
    }
    AddChange(CHANGE_CELL_FLAGS);
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
    return gs_iHoursToRespawnCell.UInt();
}

void TESObjectCELL::KillStatics() {
    if (pIslandListener) {
        bhkWorld* pWorld = spExteriorWorld;
        pIslandListener->RemoveFromWorld(**pWorld);
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
    FORM* pForm = apFile->GetFormData();
    if (!TESObjectCELL::IsFormCellChild(pForm)) {
        return false;
    }
    if (!apFile->NextForm(true)) {
        return false;
    }
    pForm = apFile->GetFormData();
    if (!TESObjectCELL::IsFormCellChild(pForm)) {
        return false;
    }
    if (pForm->form != formEnumString[2].uiFormString) {
        return false;
    }
    if (pForm->iFormID == 8) {
        apFile->NextGroup();
        pForm = apFile->GetFormData();
        if (!TESObjectCELL::IsFormCellChild(pForm)) {
            return false;
        }
    }
    if (pForm->iFormID != 9) {
        return false;
    }
    apFile->NextForm(true);
    bool bRetVal = false;
    while (true) {
        if (!TESObjectCELL::IsFormTypeCellChild(apFile->GetTESForm())) {
            break;
        }
        if (apFile->GetTESForm() == LAND_ID) {
            return true;
        }
        apFile->NextForm(true);
    }
    return bRetVal;
}

void TESObjectCELL::DefaultRenderTestCallback(RENDER_FAILURE_DATA* apFailureData, void* apParamData) {
    SaveRenderFailureData(apFailureData);
}

void TESObjectCELL::SaveRenderFailureData(TESObjectCELL::RENDER_FAILURE_DATA* apFailureData) {
    // The cell of the last saved render failure: SaveRenderFailureData, its only
    // user, compares it with and stores RENDER_FAILURE_DATA::pTestCell, and clears it.
    static TESObjectCELL* pLastFailedCell;
    if (apFailureData) {
        BSFile* pOutFile;
        float fY;
        float fZ;
        char acFileName[264];
        char acLine[268];
        float fX;
        BSSystemDir::MakeDir("RTF");
        const char* pcDir = ".\\RTF\\";
        if (apFailureData->pTestCell->GetInterior()) {
            BSsprintf(acFileName, 260, "%sINT-%s.txt", pcDir, apFailureData->pTestCell->GetFormEditorID());
        }
        else {
            TESWorldSpace* pWorld = apFailureData->pTestCell->GetWorldSpace();
            if (*apFailureData->pTestCell->TESForm::GetFormEditorID()) {
                const char* pcEditorID = apFailureData->pTestCell->GetFormEditorID();
                int32_t iDataY = apFailureData->pTestCell->GetDataY();
                int32_t iDataX = apFailureData->pTestCell->GetDataX();
                BSsprintf(
                    acFileName,
                    260,
                    "%sEXT-%s-(%i, %i)-%s.txt",
                    pcDir,
                    pWorld->GetFormEditorID(),
                    iDataX,
                    iDataY,
                    pcEditorID
                );
            }
            else {
                int32_t iDataY = apFailureData->pTestCell->GetDataY();
                int32_t iDataX = apFailureData->pTestCell->GetDataX();
                BSsprintf(acFileName, 260, "%sEXT-%s-(%i, %i).txt", pcDir, pWorld->GetFormEditorID(), iDataX, iDataY);
            }
        }
        if (pLastFailedCell == apFailureData->pTestCell) {
            BSFile* pNewFile;
            pNewFile = new BSFile(acFileName, NiFile::APPEND_ONLY, 0x4000u, false);
            pOutFile = pNewFile;
            pNewFile->Open(false, false);
        }
        else {
            BSFile* pNewFile;
            BSSystemFile::DeleteFileA(acFileName);
            pNewFile = new BSFile(acFileName, NiFile::WRITE_ONLY, 0x4000u, false);
            pOutFile = pNewFile;
            pNewFile->Open(false, false);
            pOutFile->Seek(0, NiFile::ms_iSeekSet);
            BSsprintf(
                acLine,
                260,
                "cell ID\tcamera pos\tcamera rot\tgeom count\ttri count\tnew pass count\tactive lights\r\n"
            );
            pOutFile->WriteF(acLine, BSstrlen(acLine));
            pLastFailedCell = apFailureData->pTestCell;
        }
        apFailureData->TestRot.ToEulerAnglesXYZ(fX, fY, fZ);
        BSsprintf(
            acLine,
            260,
            "(%08X)\t(%.2f, %.2f, %.2f)\t(%.2f, %.2f, %.2f)\t%i\t%i\t%i\t%i\t%i\r\n",
            apFailureData->pTestCell->iFormID,
            apFailureData->TestPos.x,
            apFailureData->TestPos.y,
            apFailureData->TestPos.z,
            fX * 57.2957763671875,
            fY * 57.2957763671875,
            fZ * 57.2957763671875,
            apFailureData->RenderData.iTotalTime,
            apFailureData->RenderData.iGeometryCount,
            apFailureData->RenderData.iTriangleCount,
            apFailureData->RenderData.iNewRenderPassCount,
            apFailureData->RenderData.iActiveLights
        );
        pOutFile->WriteF(acLine, BSstrlen(acLine));
        if (pOutFile) {
            delete pOutFile;
        }
    }
    else {
        pLastFailedCell = nullptr;
    }
}

void TESObjectCELL::UpdateExteriorWorld(float afTime) {
    if (spExteriorWorld) {
        bhkWorld* pWorld = spExteriorWorld;
        pWorld->Update();
    }
}

// Each block coordinate is narrowed to 16 bits before the call.
uint32_t TESObjectCELL::CalcExtGroupBlockKey(int32_t aiX, int32_t aiY) {
    uint16_t usX = static_cast<uint16_t>(aiX >> 5);
    uint16_t usY = static_cast<uint16_t>(aiY >> 5);
    return GetCoord(usX, usY);
}

// Each sub-block coordinate is narrowed to 16 bits before the call.
uint32_t TESObjectCELL::CalcExtGroupSubBlockKey(int32_t aiX, int32_t aiY) {
    uint16_t usX = static_cast<uint16_t>(aiX >> 3);
    uint16_t usY = static_cast<uint16_t>(aiY >> 3);
    return GetCoord(usX, usY);
}

// The seen-data update radius is capped at 2048 (the cap is compared as the double 2048.0 and stored as the float).
void TESObjectCELL::InitStatics() {
    NiPoint3 kCenter = NiPoint3::ZERO;
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
// deliberate.
bhkWorldM* TESObjectCELL::CreateWorld(NiPoint3& arCenter) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_HAVOK,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
        15387
    );
    bhkWorldCinfo kInfo;
    bhkWorldCinfo* pInfo = &kInfo;
    pInfo->m_gravity = hkVector4(0.0f, 0.0f, HavokWorldProps::fGravitySC, 0.0f);
    pInfo->setBroadPhaseWorldSize(NI2HK(147456.0f));
    NiPoint3 kCenter = arCenter;
    kCenter.z = 0.0f;
    pInfo->m_collisionTolerance = 1.0f;
    hkVector4 kWorldSize;
    kWorldSize(0) = NI2HK(262144.0f);
    kWorldSize(1) = NI2HK(262144.0f);
    kWorldSize(2) = NI2HK(262144.0f);
    hkVector4 kBorderSize;
    kBorderSize(0) = NI2HK(63488.0f);
    kBorderSize(1) = NI2HK(63488.0f);
    kBorderSize(2) = NI2HK(63488.0f);
    bhkWorldM* pWorld = new bhkWorldM(*pInfo, &kWorldSize, &kBorderSize);
    (**pWorld)->markForWrite();
    TESWindListener* pWind = new TESWindListener;
    pWorld->AddEntityListener(pWind);
    TESTrapListener* pTrap = new TESTrapListener;
    pWorld->AddTrapListener(pTrap);
    pWorld->AddWaterListener(TESWaterListener::GetExteriorListener());
    BGSAcousticSpaceListener* pAcoustic = new BGSAcousticSpaceListener;
    pWorld->AddAcousticSpaceListener(pAcoustic);
    pWorld->AddContactListener(FOCollisionListener::Instance());
    TESArrowTargetListener* pArrow = new TESArrowTargetListener;
    pWorld->AddEntityListener(pArrow);
    if (!pIslandListener) {
        FOIslandActivationListener* pIsland = new FOIslandActivationListener;
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
        MEM_CONTEXT_HAVOK,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectCELL.cpp",
        15451
    );
    spExteriorWorld->CheckForMove(arCenter);
    pMemContext->~AutoMemContext();
}

// The semaphore that guards the queued attachments, constructed on first use.
void TESObjectCELL::QueueAttachChild(TESObjectREFR* apRef, NiNode* apParent) {
    if (apRef && apRef->TESObjectREFR::Get3D()) {
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
        if (kAttach.spAttachChild != nullptr && kAttach.spAttachChild->GetParent() != kAttach.spAttachNode) {
            TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(kAttach.spAttachChild);
            if (pRef != nullptr && pRef->GetParentCell() != nullptr) {
                pRef->GetParentCell()->PerformCellNodeAttach(pRef, kAttach.spAttachNode);
            }
        }
    }
    QueuedChildAttachMap.RemoveAll();
}

void TESObjectCELL::SetLoadingTempData(bool abVal) {
    bLoadingTempData = abVal;
}

// After a particle-only update, a fade node whose world bound changed is retested against its cell's multibounds. The
// head's pointer is copied into the local and released at the end of each pass.
void TESObjectCELL::RunQueuedParticleUpdates() {
    QueuedParticleLock.Lock(nullptr);
    while (!QueuedParticleUpdates.IsEmpty()) {
        NiPointer<NiAVObject> spObject(QueuedParticleUpdates.GetItem());
        QueuedParticleUpdates.RemoveHead();
        UpdateParticlesOnly(spObject);
        if (spObject != nullptr && spObject->IsFadeNode() != nullptr && spObject->GetWorldBoundChange()) {
            TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(spObject);
            if (pRef != nullptr && pRef->GetParentCell() != nullptr) {
                pRef->GetParentCell()->TestObjectAgainstMultibounds(pRef, true, false);
                spObject->SetWorldBoundChange(false);
            }
        }
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
        NiNode* pNode = ap3D->IsNode();
        if (pNode) {
            uint32_t uiCount = pNode->GetArrayCount();
            for (uint32_t i = 0; i < uiCount; ++i) {
                TESObjectCELL::UpdateParticlesOnly(pNode->GetAt(i));
            }
        }
        else {
            NiParticles* pPart = ap3D->IsParticlesGeom();
            if (pPart) {
                NiUpdateData kUpdateData(fCellAppTime, true, false);
                pPart->Update(kUpdateData);
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
    return TESObjectCELL::sDefaultCellName.String();
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
    return TESObjectCELL::fSeenDataUpdateRadius.Setting::Float();
}

bool TESObjectCELL::GetLoadingTempData() {
    return bLoadingTempData;
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
    uint32_t uiMaxSize = pFORM->length - 24;
    if (!apFile->NextForm(true)) {
        return false;
    }
    pFORM = apFile->GetFormData();
    uint32_t uiFormID = aiRefID & 0xFFFFFF;
    bool bFound = false;
    uint32_t uiCheckedSize = 0;
    while (!bFound && uiCheckedSize < uiMaxSize) {
        if (pFORM->form == formEnumString[2].uiFormString) {
            uiCheckedSize += 24;
        }
        else if ((apFile->GetID() & 0xFFFFFF) == uiFormID) {
            bFound = true;
        }
        else {
            uiCheckedSize += pFORM->length + 24;
        }
        if (!bFound) {
            if (!apFile->NextForm(true)) {
                return false;
            }
            pFORM = apFile->GetFormData();
        }
    }
    return bFound;
}

void TESObjectCELL::UpdateMorphControllersOnly(NiAVObject* ap3D) {
    NiNode* pChild;
    uint32_t i;
    uint32_t uiArrayCount;
    NiGeomMorpherController* pGeoMorphController;
    NiNode* pNode;
    if (ap3D) {
        pGeoMorphController = static_cast<NiGeomMorpherController*>(
            ap3D->GetController(&NiGeomMorpherController::ms_RTTI)
        );
        if (pGeoMorphController) {
            NiUpdateData kUpdateData(fCellAppTime, true, false);
            pGeoMorphController->Update(kUpdateData);
        }
        pNode = ap3D->IsNode();
        if (pNode) {
            uiArrayCount = pNode->GetArrayCount();
            for (i = 0; i < uiArrayCount; ++i) {
                pChild = static_cast<NiNode*>(pNode->GetAt(i));
                UpdateMorphControllersOnly(pChild);
            }
        }
    }
}

int32_t TESObjectCELL::GetSeenValue(const NiPoint3& aCoord) {
    int32_t iRetVal = 0;
    TESObjectCELL* pCell = pPlayer->GetParentCell();
    if (pCell) {
        if (pCell->GetInterior()) {
            int32_t iSectX = (FloatToInt(aCoord.x) - 2048) >> 12;
            int32_t iSectY = (FloatToInt(aCoord.y) - 2048) >> 12;
            int32_t iPosX = 0;
            int32_t iPosY = 0;
            SeenData::GetPosForCoord(aCoord, true, iPosX, iPosY);
            SeenData* pSeen = pCell->GetIntSeenSection(iSectX, iSectY, false);
            if (pSeen && pSeen->IsSeenBitTrue(static_cast<uint32_t>(iPosX), static_cast<uint32_t>(iPosY))) {
                ++iRetVal;
            }

            SeenData* pNorthSeen = nullptr;
            bool bNorth = false;
            if (iPosY == 15) {
                bNorth = true;
                pNorthSeen = pCell->GetIntSeenSection(iSectX, iSectY + 1, false);
                if (pNorthSeen && pNorthSeen->IsSeenBitTrue(static_cast<uint32_t>(iPosX), 0)) {
                    ++iRetVal;
                }
            }
            else if (pSeen && pSeen->IsSeenBitTrue(static_cast<uint32_t>(iPosX), iPosY + 1)) {
                ++iRetVal;
            }
            SeenData* pEastSeen = nullptr;
            bool bEast = false;
            if (iPosX == 15) {
                bEast = true;
                pEastSeen = pCell->GetIntSeenSection(iSectX + 1, iSectY, false);
                if (pEastSeen && pEastSeen->IsSeenBitTrue(0, static_cast<uint32_t>(iPosY))) {
                    ++iRetVal;
                }
            }
            else if (pSeen && pSeen->IsSeenBitTrue(iPosX + 1, static_cast<uint32_t>(iPosY))) {
                ++iRetVal;
            }
            if (bNorth && bEast) {
                SeenData* pNorthEastSeen = pCell->GetIntSeenSection(iSectX + 1, iSectY + 1, false);
                if (pNorthEastSeen && pNorthEastSeen->IsSeenBitTrue(0, 0)) {
                    ++iRetVal;
                }
            }
            else if (bNorth) {
                if (pNorthSeen && pNorthSeen->IsSeenBitTrue(iPosX + 1, 0)) {
                    ++iRetVal;
                }
            }
            else if (bEast) {
                if (pEastSeen && pEastSeen->IsSeenBitTrue(0, iPosY + 1)) {
                    ++iRetVal;
                }
            }
            else if (pSeen && pSeen->IsSeenBitTrue(iPosX + 1, iPosY + 1)) {
                ++iRetVal;
            }
        }
        else {
            TESWorldSpace* pWorld = pCell->GetWorldSpace();
            if (pWorld) {
                int32_t iSectX = FloatToInt(aCoord.x) >> 12;
                int32_t iSectY = FloatToInt(aCoord.y) >> 12;
                int32_t iPosX = 0;
                int32_t iPosY = 0;
                SeenData::GetPosForCoord(aCoord, false, iPosX, iPosY);
                TESObjectCELL* pCenterCell = pWorld->GetCellFromCellCoord(iSectX, iSectY);
                SeenData* pSeen = pCenterCell ? pCenterCell->GetSeenData() : nullptr;
                if ((pSeen && pSeen->IsSeenBitTrue(static_cast<uint32_t>(iPosX), static_cast<uint32_t>(iPosY))) ||
                    (pCenterCell && pCenterCell->GetFullySeen())) {
                    ++iRetVal;
                }
                SeenData* pNorthSeen = nullptr;
                TESObjectCELL* pNorthCell = nullptr;
                bool bNorth = false;
                if (iPosY == 15) {
                    bNorth = true;
                    pNorthCell = pWorld->GetCellFromCellCoord(iSectX, iSectY + 1);
                    pNorthSeen = pNorthCell ? pNorthCell->GetSeenData() : nullptr;
                    if ((pNorthSeen && pNorthSeen->IsSeenBitTrue(static_cast<uint32_t>(iPosX), 0)) ||
                        (pNorthCell && pNorthCell->GetFullySeen())) {
                        ++iRetVal;
                    }
                }
                else if (
                    (pSeen && pSeen->IsSeenBitTrue(static_cast<uint32_t>(iPosX), iPosY + 1)) ||
                    (pCenterCell && pCenterCell->GetFullySeen())
                ) {
                    ++iRetVal;
                }
                SeenData* pEastSeen = nullptr;
                TESObjectCELL* pEastCell = nullptr;
                bool bEast = false;
                if (iPosX == 15) {
                    bEast = true;
                    pEastCell = pWorld->GetCellFromCellCoord(iSectX + 1, iSectY);
                    pEastSeen = pEastCell ? pEastCell->GetSeenData() : nullptr;
                    if ((pEastSeen && pEastSeen->IsSeenBitTrue(0, static_cast<uint32_t>(iPosY))) ||
                        (pEastCell && pEastCell->GetFullySeen())) {
                        ++iRetVal;
                    }
                }
                else if (
                    (pSeen && pSeen->IsSeenBitTrue(iPosX + 1, static_cast<uint32_t>(iPosY))) ||
                    (pCenterCell && pCenterCell->GetFullySeen())
                ) {
                    ++iRetVal;
                }
                if (bNorth && bEast) {
                    pCell = pWorld->GetCellFromCellCoord(iSectX + 1, iSectY + 1);
                    SeenData* pNorthEastSeen = pCell ? pCell->GetSeenData() : nullptr;
                    if ((pNorthEastSeen && pNorthEastSeen->IsSeenBitTrue(0, 0)) || (pCell && pCell->GetFullySeen())) {
                        ++iRetVal;
                    }
                }
                else if (bNorth) {
                    if ((pNorthSeen && pNorthSeen->IsSeenBitTrue(iPosX + 1, 0)) ||
                        (pNorthCell && pNorthCell->GetFullySeen())) {
                        ++iRetVal;
                    }
                }
                else if (bEast) {
                    if ((pEastSeen && pEastSeen->IsSeenBitTrue(0, iPosY + 1)) ||
                        (pEastCell && pEastCell->GetFullySeen())) {
                        ++iRetVal;
                    }
                }
                else if (
                    (pSeen && pSeen->IsSeenBitTrue(iPosX + 1, iPosY + 1)) ||
                    (pCenterCell && pCenterCell->GetFullySeen())
                ) {
                    ++iRetVal;
                }
            }
        }
    }
    return iRetVal;
}

int32_t TESObjectCELL::CreateCanopyShadowMaskForCell(
    TESObjectCELL* apCell,
    NiRenderedTexture*& apOutTex,
    _D3DLOCKED_RECT*& apOutRect
) {
    apOutTex = nullptr;
    if (!apCell || apCell->GetInterior() || !Renderer::GetNIRenderer()) {
        return 0;
    }
    NiRenderedTexture::bStaticForceD3DFormat = true;
    NiRenderedTexture::iStaticForcedD3DFormat = D3DFMT_L8;
    NiRenderedTexture::bStaticRenderedTexture = false;
    NiRenderer* pRenderer = NiRenderer::GetRenderer();
    apOutTex = NiRenderedTexture::Create(
        "Canopy Shadow Mask",
        64u,
        64u,
        pRenderer,
        NiRenderedTexture::ms_kDefaultPrefs,
        Ni2DBuffer::MULTISAMPLE_NONE,
        nullptr,
        0
    );
    NiRenderedTexture::bStaticForceD3DFormat = false;
    NiRenderedTexture::bStaticRenderedTexture = true;
    apCell->GetExtra()->SetCanopyShadowMask(1, apOutTex, apOutRect);
    NiDX9DynamicTextureData* ptexdata = static_cast<NiDX9DynamicTextureData*>(apOutTex->m_pkRendererData);
    IDirect3DTexture9* pd3dtexture = static_cast<IDirect3DTexture9*>(ptexdata->GetD3DTexture());
    pd3dtexture->LockRect(0, apOutRect, nullptr, 0);
    _D3DLOCKED_RECT* pRect = apOutRect;
    int32_t icount = pRect->Pitch * apOutTex->GetHeight();
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
    if (apDoorSpace && apTargetSpace && appOutBackUp && apDoorSpace != apTargetSpace) {
        uint32_t idoorcount = 0;
        BSSimpleList<TESObjectREFR*> doorlist;
        BSSimpleList<TESObjectREFR*>* preflist = nullptr;
        TESObjectCELL* pdoorcell = dynamic_cast<TESObjectCELL*>(apDoorSpace);
        TESObjectCELL* plockcell = nullptr;
        if (pdoorcell) {
            preflist = pdoorcell->GetRefList();
            plockcell = pdoorcell;
        }
        else {
            TESWorldSpace* pdoorworld = dynamic_cast<TESWorldSpace*>(apDoorSpace);
            if (pdoorworld && pdoorworld->pPersistentCell) {
                preflist = pdoorworld->pPersistentCell->GetRefList();
                plockcell = pdoorworld->pPersistentCell;
            }
        }
        plockcell->CellRefLockEnter();
        while (preflist && !preflist->IsEmpty()) {
            TESObjectREFR* pref = preflist->m_item;
            if (pref->GetObjectReference()->GetFormType() == DOOR_ID &&
                !pref->GetDelete() &&
                !pref->GetDisabled() &&
                !pref->GetDestroyed()) {
                TESObjectDOOR* pdoor = dynamic_cast<TESObjectDOOR*>(pref->GetObjectReference());
                if (pdoor &&
                    pdoor->IsRandomDoor() &&
                    pdoor->RandomlyTeleportsTo(apTargetSpace) &&
                    !pref->GetTeleport()) {
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
    static NiPoint3 pt(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    );
    if (pPlayer) {
        NiPoint3 kPos = *pPlayer->QLocationOnReference();
        SeenData::ShiftCoordToSeenPos(kPos);
        if (kPos != pt) {
            pt = kPos;
            TESObjectCELL* pParentCell = pPlayer->GetParentCell();
            if (pParentCell) {
                if (pParentCell->GetInterior()) {
                    pParentCell->UpdateSeenData(kPos);
                }
                else {
                    TESObjectCELL* pCells[4];
                    ourMemset(pCells, 0, sizeof(pCells));
                    pCells[0] = pParentCell;
                    TESWorldSpace* pWorld = pParentCell->GetWorldSpace();
                    if (pWorld) {
                        int32_t iX = FloatToInt(kPos.x) >> 12;
                        int32_t iY = FloatToInt(kPos.y) >> 12;
                        NiPoint2 kCellMin(static_cast<float>(iX << 12), static_cast<float>(iY << 12));
                        float fMaxY = static_cast<double>(iY << 12) + 4096.0;
                        float fMaxX = static_cast<double>(iX << 12) + 4096.0;
                        NiPoint2 kCellMax(fMaxX, fMaxY);
                        int32_t iXShift = 0;
                        int32_t iYShift = 0;
                        if (kCellMin.x >= kPos.x - TESObjectCELL::fSeenDataUpdateRadius.Float()) {
                            iXShift = -1;
                        }
                        else if (kCellMax.x <= kPos.x + TESObjectCELL::fSeenDataUpdateRadius.Float()) {
                            iXShift = 1;
                        }
                        if (kCellMin.y >= kPos.y - TESObjectCELL::fSeenDataUpdateRadius.Float()) {
                            iYShift = -1;
                        }
                        else if (kCellMax.y <= kPos.y + TESObjectCELL::fSeenDataUpdateRadius.Float()) {
                            iYShift = 1;
                        }
                        int32_t iIndex = 1;
                        if (iXShift) {
                            pCells[iIndex++] = pWorld->GetCellFromCellCoord(iXShift + iX, iY);
                        }
                        if (iYShift) {
                            pCells[iIndex++] = pWorld->GetCellFromCellCoord(iX, iYShift + iY);
                        }
                        if (iXShift && iYShift) {
                            pCells[iIndex++] = pWorld->GetCellFromCellCoord(iXShift + iX, iYShift + iY);
                        }
                    }
                    for (int32_t i = 0; i < 4 && pCells[i]; ++i) {
                        pCells[i]->UpdateSeenData(kPos);
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
    NiMatrix3 kOldRot;
    if (apCell && apCell->Get3D()) {
        bool bSuccess = true;
        TESObjectCELL::RENDER_TEST_DATA kDefaultData;
        if (!apTestData) {
            apTestData = &kDefaultData;
        }
        SceneGraph* pWorldRoot = Main::GetWorldSceneGraph();
        NiPoint3 kOldPos = pWorldRoot->GetCameraRoot()->GetTranslate();
        kOldRot = (*&pWorldRoot->GetCameraRoot()->m_kLocal).m_Rotate;
        if (abtestEntireCell) {
            NavMeshArray* pArray = apCell->GetNavMeshArray();
            if (pArray && pArray->GetNavMeshCount()) {
                uint32_t uiTotalTriCount = 0;
                for (int32_t i = 0; i < pArray->GetNavMeshCount(); ++i) {
                    NavMeshPtr spNavMesh = pArray->GetNavMeshByIndex(static_cast<uint32_t>(i));
                    uiTotalTriCount += spNavMesh->GetTriangleCount();
                }
                Warning("Testing %i NavMesh triangles.", uiTotalTriCount);
                for (int32_t i = 0; i < pArray->GetNavMeshCount(); ++i) {
                    NavMeshPtr spNavMesh = pArray->GetNavMeshByIndex(static_cast<uint32_t>(i));
                    if (spNavMesh) {
                        uint32_t uiTriCount = spNavMesh->GetTriangleCount();
                        for (uint32_t j = 0; j < uiTriCount; ++j) {
                            NiPoint3 kTestPos = spNavMesh->GetCenter(j);
                            kTestPos.z = kTestPos.z + 118.0;
                            if (!TESObjectCELL::RenderTestPos(apCell, kTestPos, apTestData)) {
                                bSuccess = false;
                            }
                        }
                    }
                }
            }
            else {
                Warning("Testing %i reference positions.", apCell->GetRefList()->ItemsInList());
                BSSimpleList<TESObjectREFR*>* pList = apCell->GetRefList();
                while (pList && !pList->IsEmpty()) {
                    TESObjectREFR* pRef = pList->m_item;
                    pList = pList->m_pkNext;
                    NiPoint3 kTestPos = pRef->GetLocationOnReference();
                    if (pRef->Get3D()) {
                        kTestPos = pRef->Get3D()->GetWorldBound().m_kCenter;
                    }
                    if (!TESObjectCELL::RenderTestPos(apCell, kTestPos, apTestData)) {
                        bSuccess = false;
                    }
                }
            }
        }
        else {
            if (!TESObjectCELL::RenderTestPos(apCell, pPlayer->Get3D()->GetWorldTranslate(), apTestData)) {
                bSuccess = false;
            }
        }
        pWorldRoot->GetCameraRoot()->SetTranslate(kOldPos);
        pWorldRoot->GetCameraRoot()->SetRotate(kOldRot);
        NiUpdateData kUpdateData(0.0f, false, false);
        pWorldRoot->GetCameraRoot()->Update(kUpdateData);
        if (!bSuccess) {
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
    bool bSuccess = true;
    int32_t iXAngle = 45;
    for (int32_t i = 0; i < 3; ++i) {
        NiMatrix3 kXRot;
        float fXAngle = static_cast<double>(iXAngle) * 0.01745329238474369;
        kXRot.MakeXRotation(fXAngle);
        iXAngle -= 45;
        int32_t iZAngle = 0;
        for (int32_t j = 0; j < 8; ++j) {
            NiMatrix3 kZRot;
            float fZAngle = static_cast<double>(iZAngle) * 0.01745329238474369;
            kZRot.MakeZRotation(fZAngle);
            iZAngle += 45;
            NiMatrix3 kTestRot = kZRot * kXRot;
            if (!TESObjectCELL::RenderTestPosRot(apCell, aTestPos, kTestRot, apTestData)) {
                bSuccess = false;
            }
        }
    }
    return bSuccess;
}

bool TESObjectCELL::RenderTestPosRot(
    TESObjectCELL* apCell,
    const NiPoint3& aTestPos,
    const NiMatrix3& aTestRot,
    TESObjectCELL::RENDER_TEST_DATA* apTestData
) {
    SceneGraph* pWorldRoot = Main::GetWorldSceneGraph();
    pWorldRoot->GetCameraRoot()->SetTranslate(aTestPos);
    pWorldRoot->GetCameraRoot()->SetRotate(aTestRot);
    NiUpdateData kUpdateData(0.0f, false, false);
    pWorldRoot->GetCameraRoot()->Update(kUpdateData);
    uint32_t uiStartTime = BSGetTickCount();
    appMain->Swap();
    uint32_t uiIgnore = 0;
    TESObjectCELL::RENDER_FAILURE_DATA kFailData;
    kFailData.RenderData.iTotalTime = BSGetTickCount() - uiStartTime;
    static_cast<BSShaderAccumulator*>(NiRenderer::GetRenderer()->GetSorter())
        ->BSShaderAccumulator::GetDebugStats(
            kFailData.RenderData.iGeometryCount,
            uiIgnore,
            kFailData.RenderData.iTriangleCount,
            uiIgnore,
            uiIgnore,
            uiIgnore,
            uiIgnore,
            uiIgnore,
            uiIgnore,
            uiIgnore,
            uiIgnore
        );
    kFailData.RenderData.iActiveLights = BSShaderManager::GetShadowSceneNode(0)->GetActiveLightCount();
    bool bFailed = false;
    if (kFailData.RenderData.iGeometryCount <= apTestData->Thresholds.iGeometryCount) {
        if (kFailData.RenderData.iTriangleCount <= apTestData->Thresholds.iTriangleCount) {
            if (kFailData.RenderData.iNewRenderPassCount <= apTestData->Thresholds.iNewRenderPassCount) {
                bFailed = kFailData.RenderData.iActiveLights > apTestData->Thresholds.iActiveLights ||
                    kFailData.RenderData.iAccumulateTime > apTestData->Thresholds.iAccumulateTime ||
                    kFailData.RenderData.iTotalTime > apTestData->Thresholds.iTotalTime;
            }
            else {
                bFailed = true;
            }
        }
        else {
            bFailed = true;
        }
    }
    else {
        bFailed = true;
    }
    if (bFailed && apTestData->pFailureCallbackFunc) {
        kFailData.pTestCell = apCell;
        kFailData.TestPos = aTestPos;
        kFailData.TestRot = aTestRot;
        apTestData->pFailureCallbackFunc(&kFailData, apTestData->pFailureCallbackData);
    }

    return !bFailed;
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
    TESObjectCELL* pTelInt;
    TESObjectREFR* pRef;
    if (apStartInt) {
        apStartInt->CellRefLockEnter();
    }
    static NiTPointerList<TESObjectCELL*> kCheckedCells;
    bool bDone = false;
    if (!icallsdeep) {
        extcoord = NiPoint3::ZERO;
        pextworld = nullptr;
        fextdoordist = std::numeric_limits<float>::max();
    }
    if (apStartInt && apfnEnumFunc) {
        ++icallsdeep;
        kCheckedCells.AddTail(apStartInt);
        NiTList<TESObjectREFR*> listTeleportDoors;
        BSSimpleList<TESObjectREFR*>* pList = apStartInt->GetRefList();
        while (pList && !bDone) {
            pRef = pList->m_item;
            pList = pList->m_pkNext;
            if (pRef) {
                if (afRadius1 == 3.4028234663852886e+38 ||
                    NiPoint3::IsDistanceLongerThanRadius(&pRef->GetLocationOnReference(), &aPoint1, afRadius1) < 0) {
                    if (afRadius2 == 3.4028234663852886e+38 ||
                        (afRadius2 == afRadius1 && aPoint1 == aPoint2) ||
                        NiPoint3::IsDistanceLongerThanRadius(&pRef->GetLocationOnReference(), &aPoint2, afRadius2) <
                            0) {
                        if (apfnEnumFunc(pRef, aiEnumData)) {
                            bDone = true;
                        }
                        if (pRef->GetObjectReference()->GetFormType() == DOOR_ID) {
                            TESObjectDOOR* pDoor = static_cast<TESObjectDOOR*>(pRef->GetObjectReference());
                            if (pDoor != pJailMarker) {
                                DoorTeleportData* pTel = pRef->GetTeleport();
                                if (pTel) {
                                    TESWorldSpace* pTelWorld = pTel->GetTeleportWorldSpace();
                                    pTelInt = pTel->GetTeleportCell();
                                    if (pTelInt && !pTelInt->GetInterior()) {
                                        pTelInt = nullptr;
                                    }

                                    if (pTelWorld) {
                                        float fMinDoorDist = std::numeric_limits<float>::max();
                                        if (afRadius1 != 3.4028234663852886e+38) {
                                            float fDoorDist = afRadius1 - pRef->GetDistanceFromPoint(aPoint1);
                                            if (fMinDoorDist > fDoorDist) {
                                                fMinDoorDist = fDoorDist;
                                            }
                                        }
                                        if (afRadius2 != 3.4028234663852886e+38 && afRadius1 != afRadius2) {
                                            float fDoorDist = afRadius2 - pRef->GetDistanceFromPoint(aPoint2);
                                            if (fMinDoorDist > fDoorDist) {
                                                fMinDoorDist = fDoorDist;
                                            }
                                        }
                                        if (!pextworld || fextdoordist < fMinDoorDist) {
                                            fextdoordist = fMinDoorDist;
                                            extcoord = pTel->position;
                                            pextworld = pTelWorld;
                                        }
                                    }
                                    else if (pTelInt && !kCheckedCells.FindPos(pTelInt, nullptr)) {
                                        listTeleportDoors.AddTail(pRef);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        NiTListItem<TESObjectREFR*>* pPos = listTeleportDoors.m_pkHead;
        while (pPos && !bDone) {
            TESObjectREFR* pDoorRef = listTeleportDoors.GetNext(pPos);
            if (pDoorRef) {
                float fMinDoorDist = std::numeric_limits<float>::max();
                if (afRadius1 != 3.4028234663852886e+38) {
                    float fDoorDist = afRadius1 - pDoorRef->GetDistanceFromPoint(aPoint1);
                    if (fMinDoorDist > fDoorDist) {
                        fMinDoorDist = fDoorDist;
                    }
                }
                if (afRadius2 != 3.4028234663852886e+38 && afRadius1 != afRadius2) {
                    float fDoorDist = afRadius2 - pDoorRef->GetDistanceFromPoint(aPoint2);
                    if (fMinDoorDist > fDoorDist) {
                        fMinDoorDist = fDoorDist;
                    }
                }
                DoorTeleportData* pTel = pDoorRef->GetTeleport();
                pTelInt = pTel->GetTeleportCell();
                if (!TESObjectCELL::EnumReferencesCloseToPointFromInterior(
                        pTelInt,
                        pTel->position,
                        fMinDoorDist,
                        aPoint2,
                        std::numeric_limits<float>::max(),
                        apfnEnumFunc,
                        aiEnumData
                    )) {
                    bDone = true;
                }
            }
        }
        --icallsdeep;
    }

    if (!icallsdeep) {
        kCheckedCells.RemoveAll();
        if (!bDone && pextworld) {
            pextworld->EnumReferencesCloseToPoint(
                extcoord,
                fextdoordist,
                aPoint2,
                std::numeric_limits<float>::max(),
                apfnEnumFunc,
                aiEnumData
            );
            bDone = true;
        }
    }
    if (apStartInt) {
        apStartInt->CellRefLockLeave();
    }
    return !bDone;
}

bool bhkRagdollController::QRagdollAnim() {
    return bhkRagdollController::bRagdollAnim.Bool();
}

// The threading-check mark is empty in the game's build, so MarkForWrite and UnMarkForWrite compile to one function.
void bhkWorld::MarkForWrite() {
    ahkpWorld* pw = **this;
    if (pw) {
        pw->markForWrite();
    }
}

void bhkWorld::UnMarkForWrite() {
    ahkpWorld* pw = **this;
    if (pw) {
        pw->unmarkForWrite();
    }
}

void bhkWorld::AddEntityListener(bhkEntityListener* apListener) {
    ahkpWorld* pw = **this;
    if (pw) {
        MarkForWrite();
        pw->addEntityListener(apListener);
        UnMarkForWrite();
    }
    pEntityListenerA.pushBack(static_cast<hkpEntityListener*>(apListener));
}

void bhkWorld::AddContactListener(hkpContactListener* apListener) {
    ahkpWorld* pw = **this;
    if (pw) {
        MarkForWrite();
        pw->addContactListener(apListener);
        UnMarkForWrite();
    }
}

// The listener hears entities through the entity listeners and phantoms through its hkpPhantomListener base.
void bhkWorld::AddTrapListener(bhkTrapListener* apListener) {
    AddEntityListener(apListener);
    ahkpWorld* pw = **this;
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
    arhkV(0) = arNiP.x;
    arhkV(1) = arNiP.y;
    arhkV(2) = arNiP.z;
    arhkV(3) = 0.0;
    return arhkV;
}

bool TESForm::GetReflectedByAutoWater() {
    return ((iFormFlags & kFormFlags_ReflectedByAutoWater) != 0);
}

bool TESForm::GetRefractedByAutoWater() {
    return ((iFormFlags & kFormFlags_RefractedByAutoWater) != 0);
}

bool TESObjectLIGH::GetNoFlicker() {
    return !GetFlicker() && !GetFlickerSlow() && !GetPulse() && !GetPulseSlow();
}

uint32_t TESObjectLIGH::GetColor() {
    return data.color;
}

bool TESObjectREFR::HasRandomSound() {
    TESSound::SOUND_DATA kData;
    return pRandomSound && ((kData = pRandomSound->GetData()).iFlags & TESSound::FLAG_PLAY_AT_RANDOM) != 0;
}

const char* TESWorldSpace::GetCanopyShadowTextureName() {
    return CanopyShadowTexture.GetTextureName();
}

// The constructor deliberately leaves the sphere unset and ignores both arguments.
BSNiBound::BSNiBound(uint16_t asFlags, uint16_t asLevel) {}

// An object without a world bound gets a zeroed BSNiBound (flags 0x100, level 0), set through SetBound.
void NiAVObject::CreateBoundIfNone() {
    if (m_kWorldBound) {
        return;
    }
    BSNiBound* pkBound = ::new BSNiBound(0x100, 0);
    SetBound(pkBound);
    m_kWorldBound->SetCenterAndRadius(NiPoint3::ZERO, 0.0f);
}

NiAccumulator* NiRenderer::GetSorter() {
    return m_spAccum;
}
