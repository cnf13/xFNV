#include "Fallout Shared/BGSSceneInfo.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSParticleSystemManager.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSValueNode.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSAddonNode.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTextureUseMap.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TEX_USER_DATA.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
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
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/string.h"

#include <stdint.h>

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
BSSimpleList<void*> BGSSceneInfo::CellInfoList;
BSSimpleList<void*> BGSSceneInfo::CellChildInfoList;
BGSSceneInfo::TextureTracker_t BGSSceneInfo::TextureTracker(37);
BSSimpleList<uint32_t> BGSSceneInfo::Animated3DList;
BGSSceneInfo::SceneDataAddonNodes_t BGSSceneInfo::SceneDataAddonNodes;
BGSSceneInfo::ActorMap_t BGSSceneInfo::ActorMap(37);
BGSSceneInfo::WorstOffenderMap_t BGSSceneInfo::WorstOffenderMap(37);
BGSSceneInfo::LightGeomMap_t BGSSceneInfo::LightGeomMap(37);
static SettingT<INISettingCollection> uRefCountBudgetCap("uRefCount:BudgetCaps", 700u);
static SettingT<INISettingCollection> uActorRefCountBudgetCap("uActorRefCount:BudgetCaps", 20u);
static SettingT<INISettingCollection> uActiveRefCountBudgetCap("uActiveRefCount:BudgetCaps", 100u);
static SettingT<INISettingCollection> uAnimatedObjectsCountBudgetCap("uAnimatedObjectsCount:BudgetCaps", 50u);
static SettingT<INISettingCollection> uParticlesCountBudgetCap("uParticlesCount:BudgetCaps", 5000u);
static SettingT<INISettingCollection> uEmittersCountBudgetCap("uEmittersCount:BudgetCaps", 50u);
static SettingT<INISettingCollection> uGeometryCountBudgetCap("uGeometryCount:BudgetCaps", 1000u);
static SettingT<INISettingCollection> uTriangleCountBudgetCap("uTriangleCount:BudgetCaps", 100000u);
static SettingT<INISettingCollection> uWaterCountBudgetCap("uWaterCount:BudgetCaps", 10u);
static SettingT<INISettingCollection> uGeometryMemoryBudgetCap("uGeometryMemory:BudgetCaps", 5242880u);
static SettingT<INISettingCollection> uTextureMemoryBudgetCap("uTextureMemory:BudgetCaps", 20971520u);
static SettingT<INISettingCollection> uWaterMemoryBudgetCap("uWaterMemory:BudgetCaps", 5242880u);
static SettingT<INISettingCollection> uLightCountBudgetCap("uLightCount:BudgetCaps", 10u);
SettingT<INISettingCollection> gs_uLightExcessGeometry_BudgetCaps("uLightExcessGeometry:BudgetCaps", 100u);
static SettingT<INISettingCollection> uDecalCountBudgetCap("uDecalCount:BudgetCaps", 500u);
static SettingT<INISettingCollection> uHavokTriCountBudgetCap("uHavokTriCount:BudgetCaps", 5000u);
static SettingT<INISettingCollection> uRefCountBudgetCapInterior("uRefCountInterior:BudgetCaps", 1000u);
static SettingT<INISettingCollection> uActorRefCountBudgetCapInterior("uActorRefCountInterior:BudgetCaps", 20u);
static SettingT<INISettingCollection> uActiveRefCountBudgetCapInterior("uActiveRefCountInterior:BudgetCaps", 100u);
static SettingT<INISettingCollection> uAnimatedObjectsCountBudgetCapInterior("uAnimatedObjectsCountInterior:BudgetCaps",
	50u);
static SettingT<INISettingCollection> uParticlesCountBudgetCapInterior("uParticlesCountInterior:BudgetCaps", 5000u);
static SettingT<INISettingCollection> uEmittersCountBudgetCapInterior("uEmittersCountInterior:BudgetCaps", 50u);
static SettingT<INISettingCollection> uGeometryCountBudgetCapInterior("uGeometryCountInterior:BudgetCaps", 1000u);
static SettingT<INISettingCollection> uTriangleCountBudgetCapInterior("uTriangleCountInterior:BudgetCaps", 100000u);
static SettingT<INISettingCollection> uWaterCountBudgetCapInterior("uWaterCountInterior:BudgetCaps", 10u);
static SettingT<INISettingCollection> uGeometryMemoryBudgetCapInterior("uGeometryMemoryInterior:BudgetCaps", 10485760u);
static SettingT<INISettingCollection> uTextureMemoryBudgetCapInterior("uTextureMemoryInterior:BudgetCaps", 104857600u);
static SettingT<INISettingCollection> uWaterMemoryBudgetCapInterior("uWaterMemoryInterior:BudgetCaps", 10485760u);
static SettingT<INISettingCollection> uLightCountBudgetCapInterior("uLightCountInterior:BudgetCaps", 10u);
static SettingT<INISettingCollection> uLightExcessGeometryBudgetCapInterior("uLightExcessGeometryInterior:BudgetCaps",
	5u);
static SettingT<INISettingCollection> uDecalCountBudgetCapInterior("uDecalCountInterior:BudgetCaps", 500u);
static SettingT<INISettingCollection> uHavokTriCountBudgetCapInterior("uHavokTriCountInterior:BudgetCaps", 5000u);
static SettingT<INISettingCollection> fMsRefCountBudgetCap("fMsRefCount:BudgetCaps", 0.021f);
static SettingT<INISettingCollection> fMsActorRefCountBudgetCap("fMsActorRefCount:BudgetCaps", 0.245f);
static SettingT<INISettingCollection> fMsActiveRefCountBudgetCap("fMsActiveRefCount:BudgetCaps", 0.05f);
static SettingT<INISettingCollection> fMsAnimatedObjectsCountBudgetCap("fMsAnimatedObjectsCount:BudgetCaps", 0.05f);
static SettingT<INISettingCollection> fMsParticlesCountBudgetCap("fMsParticlesCount:BudgetCaps", 0.001f);
static SettingT<INISettingCollection> fMsEmittersCountBudgetCap("fMsEmittersCount:BudgetCaps", 0.01f);
static SettingT<INISettingCollection> fMsGeometryCountBudgetCap("fMsGeometryCount:BudgetCaps", 0.01f);
static SettingT<INISettingCollection> fMsTriangleCountBudgetCap("fMsTriangleCount:BudgetCaps", 0.0001f);
static SettingT<INISettingCollection> fMsWaterCountBudgetCap("fMsWaterCount:BudgetCaps", 0.1f);
static SettingT<INISettingCollection> fMsLightExcessGeometryBudgetCap("fMsLightExcessGeometry:BudgetCaps", 0.01f);
static SettingT<INISettingCollection> fMsLightCountBudgetCap("fMsLightCount:BudgetCaps", 0.01f);
static SettingT<INISettingCollection> fMsDecalCountBudgetCap("fMsDecalCount:BudgetCaps", 0.001f);
static SettingT<INISettingCollection> fMsHavokTriCountBudgetCap("fMsHavokTriCount:BudgetCaps", 0.001f);
SettingT<INISettingCollection> fMaxMsUsagePerFrame("fMaxMsUsagePerFrame:BudgetCaps", 28.0f);
SettingT<INISettingCollection> uLoadedAreaNonActorMemoryBudgetCap("uLoadedAreaNonActorMemoryBudgetCap:BudgetCaps",
	185597952u);
SettingT<INISettingCollection> uWastelandLODBudgetAdjustment("uWastelandLODBudgetAdjustment:BudgetCaps", 20971520u);
SettingT<INISettingCollection> uCityLODBudgetAdjustment("uCityLODBudgetAdjustment:BudgetCaps", 5242880u);
SettingT<INISettingCollection> uActorMemoryBudgetCap("uActorMemoryBudgetCap:BudgetCaps", 10485760u);
SettingT<INISettingCollection> bIncludeWaterInMNumberCalculations("bIncludeWaterInMNumberCalculations:BudgetCaps",
	true);
SCENE_INFO_INFOArray pSceneInfoInfo{
	{"References", &uRefCountBudgetCap, &uRefCountBudgetCapInterior, &fMsRefCountBudgetCap, false, false},
	{"Actors", &uActorRefCountBudgetCap, &uActorRefCountBudgetCapInterior, &fMsActorRefCountBudgetCap, false, false},
	{"Active Objects", &uActiveRefCountBudgetCap, &uActiveRefCountBudgetCapInterior, &fMsActiveRefCountBudgetCap, false,
		false},
	{"Unique Anims", &uAnimatedObjectsCountBudgetCap, &uAnimatedObjectsCountBudgetCapInterior,
		&fMsAnimatedObjectsCountBudgetCap, false, false},
	{"Particles", &uParticlesCountBudgetCap, &uParticlesCountBudgetCapInterior, &fMsParticlesCountBudgetCap, false,
		false},
	{"Emitters", &uEmittersCountBudgetCap, &uEmittersCountBudgetCapInterior, &fMsEmittersCountBudgetCap, false, false},
	{"Geometry Objects", &uGeometryCountBudgetCap, &uGeometryCountBudgetCapInterior, &fMsGeometryCountBudgetCap, false,
		false},
	{"Triangles", &uTriangleCountBudgetCap, &uTriangleCountBudgetCapInterior, &fMsTriangleCountBudgetCap, false, false},
	{"Water Objects", &uWaterCountBudgetCap, &uWaterCountBudgetCapInterior, &fMsWaterCountBudgetCap, false, false},
	{"Geometry Memory", &uGeometryMemoryBudgetCap, &uGeometryMemoryBudgetCapInterior, nullptr, true, false},
	{"Texture Memory", &uTextureMemoryBudgetCap, &uTextureMemoryBudgetCapInterior, nullptr, true, false},
	{"Water Memory", &uWaterMemoryBudgetCap, &uWaterMemoryBudgetCapInterior, nullptr, true, false},
	{"Lights", &uLightCountBudgetCap, &uLightCountBudgetCapInterior, &fMsLightCountBudgetCap, false, false},
	{"Lights w/Excess Geom", &gs_uLightExcessGeometry_BudgetCaps, &uLightExcessGeometryBudgetCapInterior,
		&fMsLightExcessGeometryBudgetCap, false, false},
	{"Decal Count", &uDecalCountBudgetCap, &uDecalCountBudgetCapInterior, &fMsDecalCountBudgetCap, false, false},
	{"Havok Tri Count", &uHavokTriCountBudgetCap, &uHavokTriCountBudgetCapInterior, &fMsHavokTriCountBudgetCap, false,
		false}};

// The triangles of apShape: one for a triangle, the sum over the children of a list, MOPP, convex list or triangle
// collection, ten for any other shape. The exe gives each container case its own copy of the loop and of the
// 16-aligned 512-byte child shape buffer.
int32_t GetTriangleCountOfShape(const hkpShape* apShape) {
	if (!apShape) {
		return 0;
	}
	int32_t iCount = 0;
	switch (apShape->getType()) {
	case HK_SHAPE_TRIANGLE:
		return 1;
	case HK_SHAPE_LIST:
	case HK_SHAPE_MOPP:
	case HK_SHAPE_CONVEX_LIST:
	case HK_SHAPE_TRIANGLE_COLLECTION: {
		const hkpShapeContainer* pContainer = apShape->getContainer();
		// 0xFFFFFFFF is Havok's HK_INVALID_SHAPE_KEY, the end of the keys.
		for (uint32_t uiKey = pContainer->getFirstKey(); uiKey != 0xFFFFFFFFu; uiKey = pContainer->getNextKey(uiKey)) {
			alignas(16) char acBuffer[512];
			const hkpShape* pChild = pContainer->getChildShape(uiKey, acBuffer);
			if (!pChild) {
				break;
			}
			iCount += GetTriangleCountOfShape(pChild);
		}
		return iCount;
	}
	default:
		return 10;
	}
}

SCENE_INFO_DATA_STRUCT::SCENE_INFO_DATA_STRUCT() : pData{} {}

// SceneData's constructor zeroes it, which the exe does again here.
BGSSceneInfo::BGSSceneInfo() : pAssociatedForm(nullptr) {}

BGSSceneInfo::~BGSSceneInfo() {}

uint32_t BGSSceneInfo::GetSceneData(SCENE_INFO_DATA aeIndex) {
	if (aeIndex < 0 || aeIndex >= SID_COUNT) {
		return (uint32_t)0;
	} else {
		return SceneData.pData[aeIndex];
	}
}

// The exe rounds the product to float before returning it.
float BGSSceneInfo::GetMsUsage(SCENE_INFO_DATA aeIndex) {
	if (aeIndex < 0 || aeIndex >= 16 || !pSceneInfoInfo[aeIndex].pSceneInfoMsCount) {
		return 0.0f;
	}
	double dData = GetSceneData(aeIndex);
	float fUsage = static_cast<float>(pSceneInfoInfo[aeIndex].pSceneInfoMsCount->Float() * dData);
	return fUsage;
}

void BGSSceneInfo::CollectSceneDataFromAddonNodes() {
	SCENE_INFO_DATA_STRUCT sdis;
	uint32_t iaddonNodeCount = pDataHandler->GetAddonNodeCount();
	for (uint32_t i = 0; i < iaddonNodeCount; ++i) {
		sdis.~SCENE_INFO_DATA_STRUCT();
		::new (&sdis) SCENE_INFO_DATA_STRUCT;
		BGSSceneInfo::SceneDataAddonNodes.Insert(i, sdis);
		BGSAddonNode* paddonNode = pDataHandler->GetAddonNode(i);
		if (paddonNode) {
			bool bmaster = 0;
			if (paddonNode->GetModelLength()) {
				char modelPath[268];
				BSstrcpy(modelPath, 260, "Meshes\\");
				const char* v1 = paddonNode->GetModel();
				BSstrcat(modelPath, 260, v1);
				NiAVObject* pav = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
				if (pav) {
					if (!BSParticleSystemManager::IsValidMasterParticleSystem(pav)) {
						SCENE_INFO_DATA_STRUCT* v3 = &BGSSceneInfo::SceneDataAddonNodes.GetAt(i);
						NiAVObject* v2 = static_cast<NiAVObject*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pav));
						CollectSceneDataRecursive(v2, v3, 0, 0);
					}
					pModelLoader->ReleaseFile(modelPath);
				}
			}
		}
	}
}

// Adds apNode's subtree to apData (this object's own scene data when null). A node named FurnitureMarker is skipped
// whole, and an actor's node keeps its children out unless abTrackActors; both return before the closing
// UnloadAddonNodes call the exe makes on every other path.
void BGSSceneInfo::CollectSceneDataRecursive(NiAVObject* ap3D, SCENE_INFO_DATA_STRUCT* pStructToFill,
	bool abTrackActors, NiAVObject* apParent) {
	uint32_t* psceneDataToFill;
	if (pStructToFill) {
		psceneDataToFill = pStructToFill->pData;
	} else {
		psceneDataToFill = SceneData.pData;
	}
	if (ap3D) {
		bool bignore = false;
		bool bfound = false;
		NiNode* pnode = ap3D->IsNode();
		if (pnode) {
			if (pnode->m_kName.GetHandleString()) {
				const char* pname = pnode->m_kName.GetHandleString();
				if (!_strnicmp(pname, "FurnitureMarker", 15u)) {
					return;
				}
			}
			if (NiObject::IsKindOf_0(&BSValueNode::ms_RTTI, pnode)) {
				BSValueNode* pvalueNode =
					static_cast<BSValueNode*>(NiObject::DynamicCastStatic(&BSValueNode::ms_RTTI, pnode));
				int32_t ival = pvalueNode->QValue();
				// The exe compares the value with both counts signed (jge).
				if (ival >= 0 && ival < static_cast<int32_t>(pDataHandler->GetAddonNodeCount()) &&
					ival < static_cast<int32_t>(BGSSceneInfo::SceneDataAddonNodes.iSize)) {
					for (int32_t i = 0; i < SID_COUNT; ++i) {
						switch (i) {
						case SID_REFERENCE_COUNT:
						case SID_ACTOR_REF_COUNT:
						case SID_ACTIVE_REF_COUNT:
						case SID_UNIQUE_ANIM_COUNT:
							break;
						default:
							psceneDataToFill[i] += BGSSceneInfo::SceneDataAddonNodes.GetAt(ival).pData[i];
							break;
						}
					}
				}
			}
			bool bisActor = false;
			NiNode* pfadeNode = nullptr;
			if (pnode->IsFadeNode()) {
				pfadeNode = pnode;
			}
			if (pfadeNode) {
				TESObjectREFR* pref = static_cast<BSFadeNode*>(pfadeNode)->BSFadeNode::GetLinkedObj();
				if (pref && !pref->IsMarker() && (!bfound || !bignore)) {
					++psceneDataToFill[SID_REFERENCE_COUNT];
					if (TESObjectREFR::HasTimeControllers(ap3D)) {
						++psceneDataToFill[SID_ACTIVE_REF_COUNT];
						uint32_t uiFormID = pref->GetObjectReference()->GetFormID();
						if (!BGSSceneInfo::Animated3DList.IsInList(uiFormID)) {
							++psceneDataToFill[SID_UNIQUE_ANIM_COUNT];
							uint32_t uiNewFormID = pref->GetObjectReference()->GetFormID();
							BGSSceneInfo::Animated3DList.AddTail(uiNewFormID);
						}
					}
					for (BSSimpleList<REF_DECAL_DATA*>* plist = pref->m_Extra.GetDecalRefs(); plist && plist->m_item;
						plist = plist->m_pkNext) {
						++psceneDataToFill[SID_DECAL_COUNT];
					}
					int32_t iformType = pref->GetObjectReference()->GetFormType();
					if (iformType >= NPC__ID && iformType <= LVLN_ID) {
						++psceneDataToFill[SID_ACTOR_REF_COUNT];
						bisActor = true;
					}
					bhkNiCollisionObject* pcol = bhkNiCollisionObject::GetbhkNiCollisionObject(ap3D);
					if (pcol) {
						bhkWorldObject* pwo = pcol->GetWorldObject();
						const hkpCollidable* pcoll = pwo ? pwo->GetCollidable() : nullptr;
						const hkpShape* pshape = pcoll ? pcoll->getShape() : nullptr;
						if (pshape) {
							psceneDataToFill[SID_HAVOK_FACE_COUNT] += GetTriangleCountOfShape(pshape);
						}
					}
				}
			}
			if (bisActor && !abTrackActors) {
				return;
			}
			if (!bfound || !bignore) {
				uint32_t icount = pnode->GetArrayCount();
				for (uint32_t i = 0; i < icount; ++i) {
					NiAVObject* pchild = pnode->GetAt(i);
					CollectSceneDataRecursive(pchild, pStructToFill, 0, pnode);
				}
			}
		} else {
			NiGeometry* pgeom = ap3D->IsGeometry();
			if (pgeom) {
				if (!bfound || !bignore) {
					++psceneDataToFill[SID_GEOMETRY_COUNT];
					NiShadeProperty* pprop = static_cast<NiShadeProperty*>(pgeom->GetProperty(3));
					BSShaderProperty* pbssprop;
					if (pprop) {
						pbssprop = pprop->iShaderPropertyType != -1 ? static_cast<BSShaderProperty*>(pprop) : nullptr;
					} else {
						pbssprop = nullptr;
					}
					if (pbssprop) {
						uint32_t* kparam = &psceneDataToFill[SID_TEXTURE_MEM];
						pbssprop->GetTextureUse(&BGSSceneInfo::TexUseCB, &kparam);
						if ((apParent && NiObject::IsKindOf_0(&BGSDecalNode::ms_RTTI, apParent)) ||
							pbssprop->GetFlag(26u) || pbssprop->GetFlag(27u)) {
							++psceneDataToFill[SID_DECAL_COUNT];
						}
					}
					bool bisPPLighting;
					if (pprop) {
						bisPPLighting = pprop->iShaderPropertyType >= 8 && pprop->iShaderPropertyType <= 12;
					} else {
						bisPPLighting = false;
					}
					BSShaderPPLightingProperty* ppplprop =
						bisPPLighting ? static_cast<BSShaderPPLightingProperty*>(pprop) : nullptr;
					if (ppplprop) {
						// The exe stores the count and never reads it.
						ppplprop->GetNumberofActiveLights();
						void* piter;
						for (ShadowSceneLight* plight = ppplprop->GetFirstActiveLight(piter); plight;
							plight = ppplprop->GetNextActiveLight(piter)) {
							int32_t icount = 0;
							BGSSceneInfo::LightGeomMap.GetAt(plight, icount);
							BGSSceneInfo::LightGeomMap.SetAt(plight, ++icount);
							if (icount == gs_uLightExcessGeometry_BudgetCaps.Int()) {
								++psceneDataToFill[SID_LIGHT_COUNT_EXCESS_GEOM];
							}
						}
					}
					NiTriBasedGeom* ptrigeom = pgeom->IsTriBasedGeom();
					if (ptrigeom) {
						psceneDataToFill[SID_TRIANGLE_COUNT] += ptrigeom->GetActiveTriangleCount();
						uint32_t itotalsize = 0;
						uint32_t ivertcount = ptrigeom->GetActiveVertexCount();
						if (ptrigeom->GetVertices()) {
							itotalsize += 12 * ivertcount;
						}
						if (ptrigeom->GetColors()) {
							itotalsize += 16 * ivertcount;
						}
						if (ptrigeom->GetNormals()) {
							itotalsize += 12 * ivertcount;
						}
						if (ptrigeom->GetTextures()) {
							itotalsize += 8 * ivertcount;
						}
						psceneDataToFill[SID_GEOMETRY_MEM] += itotalsize;
					}
					if (pgeom->GetModelData()) {
						NiAdditionalGeometryData* pAGD = pgeom->GetModelData()->GetAdditionalGeometryData();
						for (uint32_t i = 0; pAGD && i < pAGD->GetSize(); ++i) {
							uint32_t iblockSize = 0;
							pAGD->GetDataBlock(i, iblockSize);
							psceneDataToFill[SID_GEOMETRY_MEM] += iblockSize;
						}
					}
				}
			} else {
				NiLight* pnilight = static_cast<NiLight*>(NiObject::DynamicCastStatic(&NiLight::ms_RTTI, ap3D));
				if (pnilight) {
					ShadowSceneNode* pssnode = BSShaderManager::GetShadowSceneNode(0);
					if (pssnode && pssnode->GetLight(pnilight)) {
						++psceneDataToFill[SID_LIGHT_COUNT];
					}
				}
			}
			NiParticleSystem* pniParticleSystem =
				static_cast<NiParticleSystem*>(NiObject::DynamicCastStatic(&NiParticleSystem::ms_RTTI, ap3D));
			if (pniParticleSystem) {
				++psceneDataToFill[SID_EMITTERS_COUNT];
				psceneDataToFill[SID_PARTICLES_COUNT] += pniParticleSystem->GetMaxNumParticles();
			}
		}
	}
	TES::pInstance->UnloadAddonNodes();
}

// Counts the placeable water planes in pCell into scene data 8 (each reference once) and adds the vertex memory of
// the water references' geometry in pCell to scene data 11. The exe's cleanup-only C++ EH frame is left out. The
// exe also counts, per group, the planes whose cell the cell map lacks, and clears a local when pCell is in the cell
// map; it reads neither, so neither is written here (the lookup itself is kept). Each group loop takes the next
// position, then checks the current one again before using it.
void BGSSceneInfo::CollectWaterData(TESObjectCELL* pCell) {
	NiTPointerList<PlaceableWaterGroup*>* pGroupList = TES::pInstance->GetWaterSystem()->GetPlaceableWaterGroupList();
	using CellMap_t = NiTMap<TESObjectCELL*, bool>;
	FNVD_CONSTRUCTED_LOCAL(CellMap_t, kCells, &CellMap_t::Destruct, 37u);
	using RefMap_t = NiTMap<TESObjectREFR*, bool>;
	FNVD_CONSTRUCTED_LOCAL(RefMap_t, kRefs, &RefMap_t::Destruct, 37u);
	NiTListItem<PlaceableWaterGroup*>* kGroupIter = pGroupList->GetHeadPos();
	NiTListItem<PlaceableWaterGroup*>* kNextGroup = nullptr;
	while (kGroupIter) {
		kNextGroup = pGroupList->GetNextPos(kGroupIter);
		if (kGroupIter) {
			PlaceableWaterGroup* pGroup = pGroupList->Get(kGroupIter);
			if (pGroup) {
				NiTListItem<TESObjectREFR*>* kIter = pGroup->PlaceableWaterList.GetHeadPos();
				NiTListItem<TESObjectREFR*>* kNext = nullptr;
				bool bVal = false;
				while (kIter) {
					kNext = pGroup->PlaceableWaterList.GetNextPos(kIter);
					TESObjectREFR* pPlane = pGroup->PlaceableWaterList.Get(kIter);
					kCells.GetAt(pPlane->TESObjectREFR::GetParentCell(), bVal);
					if (!kRefs.GetAt(pPlane, bVal) && pPlane->TESObjectREFR::GetParentCell() == pCell) {
						++SceneData.pData[8];
						kRefs.SetAt(pPlane, true);
					}
					kCells.SetAt(pPlane->TESObjectREFR::GetParentCell(), true);
					kIter = kNext;
				}
				kCells.GetAt(pCell, bVal);
			}
		}
		kCells.RemoveAll_1();
		kGroupIter = kNextGroup;
	}
	BSSimpleList<TESObjectREFR*>* pIter = pCell->GetWaterRefList();
	while (pIter && pIter->GetItem()) {
		TESObjectREFR* pRef = pIter->GetItem();
		NiNode* p3D = pRef->Get3D();
		if (p3D && p3D->IsFadeNode()) {
			NiTriBasedGeom* pGeom = p3D->GetAt(0)->IsTriBasedGeom();
			if (pGeom) {
				int32_t iSize = 0;
				int32_t iVertexCount = pGeom->GetActiveVertexCount();
				if (pGeom->GetVertices()) {
					iSize += 12 * iVertexCount;
				}
				if (pGeom->GetColors()) {
					iSize += 16 * iVertexCount;
				}
				if (pGeom->GetNormals()) {
					iSize += 12 * iVertexCount;
				}
				if (pGeom->GetTextures()) {
					iSize += 8 * iVertexCount;
				}
				SceneData.pData[11] += iSize;
			}
		}
		pIter = pIter->GetNext();
	}
	kRefs.RemoveAll_1();
}

void BGSSceneInfo::CollectSceneDataFromNode(NiNode* apNode, bool abIncludeAddonNodes, bool abTrackActors) {
	BGSSceneInfo::ActorMap.RemoveAll();
	BGSSceneInfo::LightGeomMap.RemoveAll();
	if (abIncludeAddonNodes && BGSSceneInfo::bIncludeAddons && !BGSSceneInfo::SceneDataAddonNodes.iSize) {
		CollectSceneDataFromAddonNodes();
	}
	ourMemset(&SceneData, 0, sizeof(SceneData));
	CollectSceneDataRecursive(apNode, &SceneData, abTrackActors, 0);
	if (BGSSceneInfo::GetAggregateInstances()) {
		BGSSceneInfo::TextureTracker.RemoveAll();
	}
}

bool BGSSceneInfo::GetAggregateInstances() {
	return (bool)((int8_t)BGSSceneInfo::bAggregateInstances);
}

// The exe's cleanup-only C++ EH frame is left out.
void BGSSceneInfo::CollectSceneDataForLoadedArea(float& afOutTNumber, float& afOutMNumber, bool abCollectTextures,
	bool abIncludeAddonNodes) {
	FNVD_INLINED_LOCAL(BSSimpleList<BGSSceneInfo*>, cellinfolist, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BGSTextureUseMap, textureusemap, std::nullopt, std::nullopt);
	afOutTNumber = 0.0f;
	afOutMNumber = 0.0f;
	BGSSceneInfo::ReloadBudgetCaps();
	::new (&textureusemap) BGSTextureUseMap;
	::new (&cellinfolist) BSSimpleList<BGSSceneInfo*>;
	if (TES::pInstance->pInteriorCell) {
		BGSSceneInfo* pinfo;
		TESObjectCELL* pcell = TES::pInstance->pInteriorCell;
		BGSSceneInfo* v11 = static_cast<BGSSceneInfo*>(ourMalloc(sizeof(BGSSceneInfo)));
		if (v11) {
			pinfo = ::new (v11) BGSSceneInfo;
		} else {
			pinfo = 0;
		}
		BGSSceneInfo* pinfo_2 = pinfo;
		pinfo->pAssociatedForm = pcell;
		NiNode* v4 = pcell->Get3D();
		pinfo->CollectSceneDataFromNode(v4, abIncludeAddonNodes, 0);
		pinfo->CollectWaterData(pcell);
		NiNode* v5 = pcell->Get3D();
		textureusemap.CollectTexUseData(v5, pcell, 0);
		cellinfolist.AddHead(pinfo_2);
	} else {
		// The exe reads the setting through Setting::Int (0x43D4D0, UInt's fold) and halves it with shr.
		uint32_t icenter = uGridsToLoad.UInt() >> 1;
		uint32_t imin = icenter - BSMIN(static_cast<int32_t>(icenter), 2);
		uint32_t imax = icenter + BSMIN(static_cast<int32_t>(icenter), 2);
		for (uint32_t ix = imin; ix <= imax; ++ix) {
			for (uint32_t iy = imin; iy <= imax; ++iy) {
				GridCell* pgridcell = TES::pInstance->GetGridCell(static_cast<int32_t>(ix), static_cast<int32_t>(iy));
				if (pgridcell->pCell) {
					BGSSceneInfo* pinfo_3;
					TESObjectCELL* pcell__pgridcell = pgridcell->pCell;
					BGSSceneInfo* v10 = static_cast<BGSSceneInfo*>(ourMalloc(sizeof(BGSSceneInfo)));
					if (v10) {
						pinfo_3 = ::new (v10) BGSSceneInfo;
					} else {
						pinfo_3 = 0;
					}
					BGSSceneInfo* pinfo_4 = pinfo_3;
					pinfo_3->pAssociatedForm = pcell__pgridcell;
					NiNode* v6 = pcell__pgridcell->Get3D();
					pinfo_4->CollectSceneDataFromNode(v6, abIncludeAddonNodes, 0);
					NiNode* v7 = pcell__pgridcell->Get3D();
					textureusemap.CollectTexUseData(v7, pcell__pgridcell, 0);
					pinfo_4->CollectWaterData(pcell__pgridcell);
					cellinfolist.AddHead(pinfo_4);
				}
			}
		}
	}
	while (!cellinfolist.IsEmpty()) {
		BGSSceneInfo* pinfo_5 = (&cellinfolist)->m_item;
		cellinfolist.RemoveHead();
		afOutMNumber = static_cast<double>(pinfo_5->GetSceneData(SID_GEOMETRY_MEM)) + afOutMNumber;
		for (SCENE_INFO_DATA i = SID_REFERENCE_COUNT; i < SID_COUNT; i = static_cast<SCENE_INFO_DATA>(i + 1)) {
			if (!pSceneInfoInfo[i].bMemoryRelated) {
				afOutTNumber = pinfo_5->GetMsUsage(i) + afOutTNumber;
			}
		}
		if (pinfo_5) {
			delete pinfo_5;
		}
	}
	if (abCollectTextures) {
		afOutMNumber = static_cast<double>(textureusemap.GetTotalTextureSize()) + afOutMNumber;
	}
	BGSSceneInfo::TextureTracker.RemoveAll();
	cellinfolist.~BSSimpleList<BGSSceneInfo*>();
	textureusemap.~BGSTextureUseMap();
}

uint32_t BGSSceneInfo::GetActorMemoryBudgetCap() {
	return (&uActorMemoryBudgetCap)->UInt();
}

uint32_t BGSSceneInfo::GetLoadedAreaNonActorMemoryBudgetCap() {
	uint32_t ival = uLoadedAreaNonActorMemoryBudgetCap.Int();
	if (!TES::pInstance->pInteriorCell) {
		TESWorldSpace* pLODworld;
		for (pLODworld = TES::pInstance->pWorldSpace; pLODworld && pLODworld->GetParentWorld(TESWorldSpace::PU_LOD);
			pLODworld = pLODworld->GetParentWorld(TESWorldSpace::PU_LOD)) {
			;
		}
		if (pLODworld) {
			int32_t LODBudgetAdjustment;
			if (pLODworld->iFormID == ACRE_ID) {
				LODBudgetAdjustment = uWastelandLODBudgetAdjustment.Int();
			} else {
				LODBudgetAdjustment = uCityLODBudgetAdjustment.Int();
			}
			ival -= LODBudgetAdjustment;
		}
	}
	return ival;
}

// Formats afMnumber as its percentage of the budget cap. The exe converts the cap with fild qword and rounds it to a
// float (fstp dword) before using it: the conversion goes through double so that clang rounds it too (a direct
// uint32_t-to-float conversion stays unrounded in an x87 register).
void BGSSceneInfo::BuildMNumberString(float afMnumber, BSStringT<char>& aOutText, bool abForActor) {
	float fmax;
	if (abForActor) {
		fmax = static_cast<float>(static_cast<double>(BGSSceneInfo::GetActorMemoryBudgetCap()));
	} else {
		fmax = static_cast<float>(static_cast<double>(BGSSceneInfo::GetLoadedAreaNonActorMemoryBudgetCap()));
	}
	float fpercent = afMnumber * 100.0 / fmax;
	float v4 = fmax / 1048576.0;
	if (!(afMnumber > 1048576.0)) {
		if (!(afMnumber > 1024.0)) {
			aOutText.SPrintF("%.2f%% (%.0f bytes / %.2f MB)", fpercent, afMnumber, v4);
		} else {
			float afMnumberb = afMnumber / 1024.0;
			aOutText.SPrintF("%.2f%% (%.2f KB / %.2f MB)", fpercent, afMnumberb, v4);
		}
	} else {
		float afMnumbera = afMnumber / 1048576.0;
		aOutText.SPrintF("%.2f%% (%.2f MB / %.2f MB)", fpercent, afMnumbera, v4);
	}
}

void BGSSceneInfo::ReloadBudgetCaps() {
	INISettingCollection* v0;
	INISettingCollection* collection;
	int32_t i;
	for (i = 0; i < 16; ++i) {
		collection = SettingT<INISettingCollection>::GetCollection();
		collection->ReadSetting(*pSceneInfoInfo[i].pSceneInfoThreshhold);
		v0 = SettingT<INISettingCollection>::GetCollection();
		v0->ReadSetting(*pSceneInfoInfo[i].pSceneInfoThreshholdInterior);
	}
}

void BGSSceneInfo::TexUseCB(NiTexture* apTexture, uint32_t aulMemorySize, void* apParam) {
	bool bchecked = 0;
	if (!BGSSceneInfo::TextureTracker.GetAt(apTexture, bchecked) || !bchecked) {
		**static_cast<uint32_t**>(apParam) += aulMemorySize;
		BGSSceneInfo::TextureTracker.SetAt(apTexture, 1);
	}
}

// spTexture[aiTextureID] through NiPointer's out-of-line operator->; a null this reads a temporary null pointer
// instead (null). The exe's cleanup-only EH frame around the temporary is left out.
NiRenderedTexture* BSRenderedTexture::GetTexture(int32_t aiIndex) {
	if (!this) {
		NiPointer<NiRenderedTexture> spTemp;
		NiRenderedTexture* pTexture = spTemp.operator->();
		return pTexture;
	}
	return spTexture[aiIndex].operator->();
}

BGSTextureUseMap::BGSTextureUseMap() : UserMap(37u) {}

NiPoint3* NiGeometry::GetNormals() {
	return m_spModelData.operator->()->GetNormals();
}

NiColorA* NiGeometry::GetColors() {
	return m_spModelData.operator->()->GetColors();
}

NiPoint2* NiGeometry::GetTextures() {
	NiGeometryData* v1 = m_spModelData.operator->();
	return v1->m_pkTexture;
}

NiPoint3* NiGeometryData::GetNormals() const {
	return m_pkNormal;
}

hkpShapeType hkpShape::getType() const {
	return m_type;
}
