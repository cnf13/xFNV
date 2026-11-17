#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

#include <stdint.h>

class ShadowSceneLight;
class TESObjectCELL;
class TESObjectREFR;

// apShape is the Havok shape; CollectSceneDataRecursive passes hkpCollidable's m_shape.
int32_t GetTriangleCountOfShape(const hkpShape* apShape);

// The kinds of scene data, indexes into SCENE_INFO_DATA_STRUCT::pData and pSceneInfoInfo.
enum SCENE_INFO_DATA : int32_t {
	SID_REFERENCE_COUNT = 0,
	SID_ACTOR_REF_COUNT = 1,
	SID_ACTIVE_REF_COUNT = 2,
	SID_UNIQUE_ANIM_COUNT = 3,
	SID_PARTICLES_COUNT = 4,
	SID_EMITTERS_COUNT = 5,
	SID_GEOMETRY_COUNT = 6,
	SID_TRIANGLE_COUNT = 7,
	SID_WATER_COUNT = 8,
	SID_GEOMETRY_MEM = 9,
	SID_TEXTURE_MEM = 10,
	SID_WATER_MEM = 11,
	SID_LIGHT_COUNT = 12,
	SID_LIGHT_COUNT_EXCESS_GEOM = 13,
	SID_DECAL_COUNT = 14,
	SID_HAVOK_FACE_COUNT = 15,
	SID_COUNT = 16,
};

// One scene data record: sixteen words.
class SCENE_INFO_DATA_STRUCT {
public:
	SCENE_INFO_DATA_STRUCT();

	// Counts despite the name: CollectSceneDataRecursive increments them and adds triangle counts to them.
	uint32_t pData[16];
};

// The settings that name, budget and time one kind of scene data.
struct SCENE_INFO_INFO {
	const char* pSceneInfoName;
	SettingT<INISettingCollection>* pSceneInfoThreshhold;
	SettingT<INISettingCollection>* pSceneInfoThreshholdInterior;
	SettingT<INISettingCollection>* pSceneInfoMsCount;
	bool bMemoryRelated;
	bool bTimeRelated;
	uint8_t padding12[2];
};

class BGSSceneInfo {
public:
	using ReferenceIgnoreMap_t = NiTMap<TESObjectREFR*, bool>;
	// CollectSceneDataRecursive keys it by the light.
	using LightGeomMap_t = NiTMap<ShadowSceneLight*, int32_t>;
	using TextureTracker_t = NiTMap<NiTexture*, bool>;
	using SceneDataAddonNodes_t = BSSimpleArray<SCENE_INFO_DATA_STRUCT, 1024>;
	// The ActorMap is emptied in CollectSceneDataFromNode.
	using ActorMap_t = NiTMap<TESActorBase*, bool>;
	using WorstOffenderMap_t = NiTMap<int32_t, int32_t>;

	BGSSceneInfo();
	virtual ~BGSSceneInfo();

	// The scene data value at aeIndex, 0 outside 0-15; the compare is signed.
	uint32_t GetSceneData(SCENE_INFO_DATA aeIndex);
	// The milliseconds the scene data at aeIndex costs: its value times its ms-count setting; 0 outside 0-15 or
	// without that setting.
	float GetMsUsage(SCENE_INFO_DATA aeIndex);
	void CollectSceneDataFromAddonNodes();
	void CollectSceneDataRecursive(NiAVObject* ap3D, SCENE_INFO_DATA_STRUCT* pStructToFill, bool abTrackActors,
		NiAVObject* apParent);
	void CollectWaterData(TESObjectCELL* pCell);
	void CollectSceneDataFromNode(NiNode* apNode, bool abIncludeAddonNodes, bool abTrackActors);

	static bool GetAggregateInstances();
	// Totals the texture and memory figures of the loaded area's scene data into afOutTNumber and afOutMNumber.
	static void CollectSceneDataForLoadedArea(float& afOutTNumber, float& afOutMNumber, bool abCollectTextures,
		bool abIncludeAddonNodes);
	static uint32_t GetActorMemoryBudgetCap();
	// The loaded area's non-actor memory budget cap: uLoadedAreaNonActorMemoryBudgetCap.
	static uint32_t GetLoadedAreaNonActorMemoryBudgetCap();
	// Formats afMnumber as its percentage of the actor or the loaded area budget cap, with the figure in bytes, KB or
	// MB.
	static void BuildMNumberString(float afMnumber, BSStringT<char>& aOutText, bool abForActor);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void ReloadBudgetCaps();
	static void TexUseCB(NiTexture* apTexture, uint32_t aulMemorySize, void* apParam);

	SCENE_INFO_DATA_STRUCT SceneData;
	TESForm* pAssociatedForm;

	static inline bool bAggregateInstances = true;
	static inline bool bIncludeAddons = true;
	static inline ReferenceIgnoreMap_t* pReferenceIgnoreMap;
	static LightGeomMap_t LightGeomMap;
	static BSSimpleList<uint32_t> Animated3DList;
	static TextureTracker_t TextureTracker;
	static SceneDataAddonNodes_t SceneDataAddonNodes;
	static BSSimpleList<void*> CellInfoList;
	static BSSimpleList<void*> CellChildInfoList;
	static ActorMap_t ActorMap;
	static WorstOffenderMap_t WorstOffenderMap;
};

// One entry per kind of scene data; the first 16 are indexed.
using SCENE_INFO_INFOArray = SCENE_INFO_INFO[16];
extern SCENE_INFO_INFOArray pSceneInfoInfo;

// The game settings BGSSceneInfo.cpp defines.
extern SettingT<INISettingCollection> bIncludeWaterInMNumberCalculations;
extern SettingT<INISettingCollection> uActorMemoryBudgetCap;
extern SettingT<INISettingCollection> uWastelandLODBudgetAdjustment;
extern SettingT<INISettingCollection> uLoadedAreaNonActorMemoryBudgetCap;
extern SettingT<INISettingCollection> fMaxMsUsagePerFrame;
extern SettingT<INISettingCollection> uCityLODBudgetAdjustment;

extern SettingT<INISettingCollection> gs_uLightExcessGeometry_BudgetCaps;
