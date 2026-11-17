#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSMain/Setting.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

#include <stdint.h>

class Actor;
class BSMultiBoundNode;
class NiTriShape;
class NiTriShapeData;
class TESObjectCELL;
class TESObjectREFR;
class TESWaterForm;
class TESWorldSpace;

// The larger (smaller) of afA and adB, adB when they are equal or unordered; the water update clamps the sun intensity
// with them.
double MaxOrEq(float afA, double adB);
double MinOrEq(float afA, double adB);

// The water planes sharing one height and water type, with the references in them. Only the leading members are
// declared.
class PlaceableWaterGroup {
public:
	PlaceableWaterGroup();
	~PlaceableWaterGroup();

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	TESWaterForm* pWaterType;
	NiPlane ReflectWaterPlane;
	NiPlane RefractWaterPlane;
	NiTPointerList<TESObjectREFR*> PlaceableWaterList;
	// The references in the group's water; actors go to ActorsInWaterList instead
	// (TESWaterSystem::UpdateSubmergedReference).
	NiTPointerList<TESObjectREFR*> ObjectInWaterList;
	NiTPointerList<Actor*> ActorsInWaterList;
	// The groups that share this group's reflection textures.
	NiTPointerList<PlaceableWaterGroup*> SharedReflectionGroups;
	NiPointer<BSRenderedTexture> spGroupReflectionMap;
	NiPointer<NiRefObject> spWadingWaterGeometry;
	bool bGroupAtWorldSpaceWaterHeight;
	bool bRenderGroup;
	bool bRenderDepth;
	bool bRenderGroupReflections;
	bool bRenderSilhouetteReflections;
	uint8_t padding61[3];
	// The static and dynamic objects the group reflects.
	NiTPointerList<NiAVObject*> StaticReflectiveObjects;
	NiTPointerList<NiAVObject*> DynamicReflectiveObjects;
	// TESWaterSystem::UpdatePlaceableWater empties both StaticDepthObjects and DynamicDepthObjects.
	NiTPointerList<NiAVObject*> StaticDepthObjects;
	NiTPointerList<NiAVObject*> DynamicDepthObjects;
	NiPointer<BSShaderAccumulator> spGroupReflectionSorter;
	NiPointer<BSShaderAccumulator> spDepthSorter;
	uint32_t iReflectionThreadStage;
	uint32_t iDepthThreadStage;
	NiPointer<NiCamera> spReflectionCamera;
	NiPointer<NiCamera> spDepthCamera;
	uint32_t iStencilBitMask;

	// Whether the world (sky) reflections must render this frame; the water update clears both.
	static inline bool bRenderWorldReflections;
	static inline bool bRenderSkyReflections;
	// The sky reflection pass (TESWaterSystem::MTRenderSkyReflectionTexture): its thread stage, sorter, texture and
	// camera.
	static inline uint32_t iSkyReflectionThreadStage;
	// The world reflection pass's thread stage (TESWaterSystem::UpdatePlaceableWater).
	static inline uint32_t iWorldReflectionThreadStage;
	// The shared depth map and wading-water height map the water groups render into.
	static NiPointer<BSRenderedTexture> spWadingWaterHeightMap;
	static NiPointer<BSRenderedTexture> spDepthMap;
	static NiPointer<BSShaderAccumulator> spSkyReflectionSorter;
	static NiPointer<BSRenderedTexture> spSkyReflectionMap;
	static NiPointer<NiCamera> spSkyReflectionCamera;
	static NiPointer<BSRenderedTexture> spWorldReflectionMap;
	static NiTPointerList<TESObjectREFR*> ExplosionsList;
	static NiTPointerList<NiAVObject*> StaticSkyReflectiveObjects;
	static NiTPointerList<NiAVObject*> DynamicSkyReflectiveObjects;
};

// The water system: the root water node, the water textures it renders with, and the camera/player state the
// water shader needs. One instance is created per savegame load; most accessors are static because the state
// they read is process-global.
class TESWaterSystem {
public:
	TESWaterSystem();
	~TESWaterSystem();

	// Removes apRefr from the submerged references (the actor list for an actor) of
	// apWaterGroup. Its caller passes any reference: the body tests IsActor.
	void RemoveSubmergedReferenceFromGroup(TESObjectREFR* apRefr, PlaceableWaterGroup* apWaterGroup);
	// The water geometry under apWaterRef's 3D (the first child's first child, one level
	// deeper for automatic water), or null.
	NiAVObject* GetWaterGeometry(TESObjectREFR* apWaterRef);
	// Shows or hides the water.
	void DisplayWater(bool abVisible, bool abLOD, bool abInverseCull);
	// The water group of apWaterRef, looked up from apLookupRefr at afHeight.
	PlaceableWaterGroup* FindWaterGroup(TESObjectREFR* apWaterRef, TESObjectREFR* apObjectRef, float afWaterHeight);
	// Builds a water quad mesh of afMeshSizeWidth by afMeshSizeHeight.
	NiTriShapeData* CreateQuadData(float afMeshSizeWidth, float afMeshSizeHeight, uint32_t auiTextureSize,
		uint32_t auiTile, bool abNormals, bool abColors);
	// A square quad mesh: meshSize by meshSize.
	NiTriShapeData* CreateQuadData(float meshSize, uint32_t textureSize, uint32_t tile, bool bNormals, bool bColors);
	NiTPointerList<PlaceableWaterGroup*>* GetPlaceableWaterGroupList();
	PlaceableWaterGroup* GetLODWaterGroup();
	void SetWaterNoiseTexture(NiTexture* apTexture);
	// Updates the water planes, reflections and refractions for apCamera; apNode is unused.
	void UpdateWaterSystem(NiCamera* apCamera, NiNode* apNode);
	// Updates the LOD water for apCamera.
	void UpdateLODWater(NiCamera* apCamera, bool abForceUpdate);
	// Updates the placeable water groups for apCamera.
	void UpdatePlaceableWater(NiCamera* apCamera, bool abForceUpdate);
	// Whether the actors in water need an update.
	bool CheckUpdateActorsInWater();
	// Updates the actors' water displacements.
	void UpdateActorsInWater();
	// Steps the water displacement simulation.
	void UpdateWaterDisplacementSimulation();
	// Updates the water sounds.
	void UpdateWaterSounds();
	// The near-water sound settings (uNearWaterRadius, uNearWaterPoints, fNearWaterIndoorTolerance and
	// fNearWaterOutdoorTolerance in the Water section).
	uint32_t GetNearWaterRadius();
	uint32_t GetNearWaterPoints();
	float GetNearWaterIndoorTol();
	float GetNearWaterOutdoorTol();
	// Removes apObjectRef from the water group of apWaterRef, or of the cell water at
	// afWaterHeight when apWaterRef is null.
	void RemoveTESObjectFromWaterGroup(TESObjectREFR* apWaterRef, TESObjectREFR* apObjectRef, float afWaterHeight);
	// Removes apObjectRef from every automatic water group.
	void RemoveTESObjectFromAllAutoWaterGroups(TESObjectREFR* apObjectRef);
	// Adds apRef to the water group at apPos with rotation apRot; false when water is off.
	bool AddWaterRef(TESObjectREFR* apRef, NiPoint3* apPos, NiMatrix3* apRot);
	// Adds placeable water apWaterRef to the water groups at its world position and rotation.
	bool AddPlaceableWater(TESObjectREFR* apWaterRef);
	bool EnableWaterSystem();
	// Turns the water system off.
	bool DisableWaterSystem(bool abFreeMemory, bool abFinal);
	bool IsWaterNodeVisible(TESObjectREFR* apWaterRef, NiCamera* apCamera);
	// Removes apRef from the water group at apPos with rotation apRot; true when it was found.
	bool RemovePlaceableWater(TESObjectREFR* apRef, NiPoint3* apPos, NiMatrix3* apRot);
	void RemoveWaterRefFromWaterSystem(TESObjectREFR* apRef);
	// Updates the shader properties of apWaterGroup's water.
	void UpdateWaterShaderProperties(PlaceableWaterGroup* apWaterGroup);
	// Updates apWaterGroup's water noise.
	void UpdateWaterNoise(PlaceableWaterGroup* apWaterGroup);
	// Points apGroup's water shaders at its reflection textures.
	void UpdateReflectionTexturePointers(PlaceableWaterGroup* apGroup);
	// Clears apGroup's reflection texture pointers.
	void ClearReflectionTexturePointers(PlaceableWaterGroup* apGroup, bool abRemovingGroup, bool abRecurse);
	// Sets up apWaterGroup's reflection texture for apCamera.
	void MTSetupSelectiveReflectionTexture(NiCamera* apCamera, PlaceableWaterGroup* apWaterGroup, bool abLOD);
	// Sets up the world reflection texture for apCamera.
	void MTSetupWorldReflectionTexture(NiCamera* apCamera);
	// Sets up the sky reflection texture for apCamera.
	void MTSetupSkyReflectionTexture(NiCamera* apCamera);
	// Renders apWaterGroup's reflection texture.
	void MTRenderSelectiveReflectionTexture(PlaceableWaterGroup* apWaterGroup);
	// Renders the world reflection texture.
	void MTRenderWorldReflectionTexture();
	// Sets up apWaterGroup's depth texture for apCamera.
	void MTSetupSelectiveDepthTexture(NiCamera* apCamera, PlaceableWaterGroup* apWaterGroup);
	// Renders apWaterGroup's depth where the stencil matches auReadMask.
	void MTRenderSelectiveDepthTexture(PlaceableWaterGroup* apWaterGroup, uint16_t auReadMask);
	// Writes apWaterGroup's stencil bits auWriteMask from apCamera.
	void RenderWaterGroupStencil(NiCamera* apCamera, PlaceableWaterGroup* apWaterGroup, bool abClearAll,
		uint16_t auWriteMask);
	void AddTESObjectToWaterGroup(TESObjectREFR* apWaterRef, TESObjectREFR* apObjectRef, float afWaterHeight);
	void AddRipple(NiPoint3 arPosition, float afSize);

protected:
	// Renders the sky into the sky reflection texture, clipped at the LOD water plane.
	void MTRenderSkyReflectionTexture();
	// Sets user clip plane 0 to afData, a world-space plane, in clip space. aiPlane is unused. afData holds the plane's
	// four floats as a NiColorA (r, g, b, a = the plane's a, b, c, d).
	void SetClipPlane(NiColorA afData, int32_t aiPlane);

public:
	void UpdateWaterShaderProperties(PlaceableWaterGroup* apWaterGroup, TESObjectREFR* apRef);
	void GetWaterNoiseTexture(NiPointer<NiTexture>& aspTexture);
	TESObjectREFR* AddLODWater(NiGeometry* apLODWater, TESWorldSpace* apWorldSpace, NiNode* apWaterLODRoot,
		BSMultiBoundNode* apBound, bool abLODWaterHeight);
	void RemoveLODWater(TESObjectREFR* apLODWaterRef, TESWorldSpace* apWorldSpace, NiNode* apWaterLODRoot);
	void ResetStencilBitRefs();
	NiTriShape* CreateQuadShape(NiTriShapeData* data);
	// Builds the wading-water quad of a water group.
	NiAVObject* CreateWadingWaterGeometry(PlaceableWaterGroup* apGroup, NiNode* pParentNode, float fMeshSize);
	bool ActorWithinRangeOfWadingWater(PlaceableWaterGroup* apGroup);
	// Renders the placeable waters' stencil for the displacement pass.
	void RenderDisplacementWaterStencil();
	BSRenderedTexture* InitializeGreyTexture();

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);
	static NiNode* GetWaterRoot();
	// Removes the water root's children and releases the root.
	static void ClearWaterRoot();
	static bool GetPlayerUnderwater();
	static TESWaterForm* GetCurrentWaterType();
	static bool GetCameraInPlaceableWater();
	static void SetPlayerForceUnderwater(bool abVal);
	static void SetPlayerUnderwater(bool abIsUnderWater);
	static void SetCameraRelevantWaterHeight(float afVal);
	// Whether bUseWaterShader:Water is set.
	static bool GetUseWaterShader();
	static void SetLastWaterCell(TESObjectCELL* apCell);
	static bool GetWaterEnabled();
	static void SetCurrentWaterType(TESWaterForm* apType);
	static void SetCameraInPlaceableWater(bool abVal);
	// Returns fCameraRelevantWaterHeight, which is defined in Sky.cpp.
	static float GetCameraRelevantWaterHeight();
	static bool GetUseWaterDisplacements();

	int32_t iAccumulationCount;
	int32_t iGlobalGetWaterGeometryCount;
	// TESWaterSystem::DisableWaterSystem hands spWaterHeightMapTexture, spWaterNormalMapTexture and
	// spRainHeightMapTexture to BSTextureManager::ReturnRenderedTexture.
	NiPointer<BSRenderedTexture> spWaterHeightMapTexture;
	NiPointer<NiSourceTexture> spWaterHeightMapTextureFiltered;
	NiPointer<BSRenderedTexture> spWaterNormalMapTexture;
	NiPointer<BSRenderedTexture> spRainHeightMapTexture;
	// spRefractionDepthStencilBuffer: the water refraction depth/stencil buffer.
	NiPointer<NiSourceTexture> spRefractionDepthStencilBuffer;
	NiPointer<NiSourceTexture> spWaterNoiseTexture;
	// spWaterNormalMap: the combined normal map texture.
	NiPointer<NiSourceTexture> spWaterNormalMap;
	float fBlendTimer;
	NiPoint2 fLastDisplaceOffset;
	// The water type the system blends to, whether the blend runs, and how far it has come.
	TESWaterForm* pBlendToWaterType;
	bool bUpdateWaterBlend;
	uint8_t padding35[0x3];
	float fWaterTypeBlendAmount;
	NiTPointerList<PlaceableWaterGroup*> PlaceableWaterGroupList;
	// The LOD water's group.
	PlaceableWaterGroup* pLODWaterGroup;
	NiTPointerMap<TESObjectREFR*, TESObjectREFR*> ReflectionRefMap;
	NiTPointerMap<TESObjectREFR*, TESObjectREFR*> DepthRefMap;
	NiTPointerMap<TESWaterForm*, bool> WaterTypeUpdateMap;
	// Each reference in the water's wading state.
	NiTPointerMap<TESObjectREFR*, WadingWaterData*> WadingWaterMap;
	// The water's looping sound.
	BSSoundHandle WaterSound;
	float fTimeSinceLastRipplePlaced;
	// Whether the world reflection pass hid the player's 3rd-person 3D; the constructor clears it.
	bool bCull3rdPerson;
	uint8_t padding9D[3];

	// The root of the water scene graph, owned by the system.
	static NiPointer<NiNode> spWaterRoot;
	// The water type the shader currently blends from.
	static inline TESWaterForm* pCurrentWaterType;
	// Overrides the player's underwater state from scripts.
	static inline bool bForceUnderwater;
	// Whether the player is actually underwater this frame.
	static inline bool bUnderWater;
	// Whether the camera is inside placeable water geometry.
	static inline bool bCameraInPlaceableWater;
	// The water height the camera should render against.
	static inline float fCameraRelevantWaterHeight;
	// bUsePerWorldSpaceWaterNoise:Water: each water form loads its own noise texture instead of the shared one.
	static SettingT<INISettingCollection> bUsePerWorldSpaceWaterNoise;
	// bUseWater:Water: whether water is drawn at all.
	static SettingT<INISettingCollection> bUseWater;
	// bUseWaterShader:Water and bUseWaterDisplacements:Water: whether water draws with its shader and with displacement.
	static SettingT<INISettingCollection> bUseWaterShader;
	static SettingT<RendererSettingCollection> bUseWaterDisplacements;
	// bUseWaterHiRes:Water and bUseWaterReflections:Water (Renderer::Init writes them to RendererInfo.txt).
	static SettingT<INISettingCollection> bUseWaterHiRes;
	static SettingT<RendererSettingCollection> bUseWaterReflections;
	static inline bool bWaterEnabled = true;
	static inline TESObjectCELL* pLastWaterCell;
	// The world camera of the last water update.
	static inline NiCamera* pWorldCamera;
	static inline bool bDisplayWaterSoundPlacement;
	// Skips the next water update once.
	static inline bool bSkipNextUpdate;
	// Forces the next LOD and placeable water update.
	static inline bool bForceWaterUpdate = true;
	// The number of placeable water groups visible this frame.
	static inline int32_t iActiveWaterGroups;
	// The Water section's reflection, refraction and depth settings (bUseWaterReflections:Water and so on).
	static SettingT<RendererSettingCollection> bAutoWaterSilhouetteReflections;
	static SettingT<INISettingCollection> bForceLowDetailReflections;
	static SettingT<RendererSettingCollection> bUseWaterDepth;
	static SettingT<RendererSettingCollection> bForceHighDetailReflections;
	static SettingT<RendererSettingCollection> bUseWaterRefractions;
	// The near-water sound settings (uNearWaterRadius, uNearWaterPoints, fNearWaterIndoorTolerance and
	// fNearWaterOutdoorTolerance in the Water section).
	static SettingT<INISettingCollection> iNearWaterRadius;
	static SettingT<INISettingCollection> iNearWaterPoints;
	static SettingT<INISettingCollection> fNearWaterIndoorTol;
	static SettingT<INISettingCollection> fNearWaterOutdoorTol;
	static inline int32_t uiNumInteriorReflectionsAllowed;
	static inline int32_t uiNumExteriorReflectionsAllowed;
	static inline int32_t uiLODPlanesCount;
	static inline NiFixedString* pTag;
	static NiPointer<NiCamera> spWaterHeightCamera;
	static NiPointer<BSShaderAccumulator> spWaterHeightShaderAccumulator;
	// fDisplacementDampener is read by SetupGeometryConstants.
	static inline float fDisplacementDampener;
	static inline bool bIsRendering;
	static NiTPointerList<NiAVObject*> WaterLevelReflect_Empty;
	// Passed to MTRenderingSystem::AddAccumTask as a NiTPointerList<NiAVObject*>.
	static NiTPointerList<NiAVObject*> WaterLevelReflect_Nodes;
	static SettingT<INISettingCollection> fSurfaceTileSize;
	static SettingT<INISettingCollection> fTileTextureDivisor;
	static SettingT<INISettingCollection> bUseWaterHDR;
	static SettingT<INISettingCollection> fWadingWaterTextureRes;
	static SettingT<INISettingCollection> fExteriorWaterReflectionThreshold;
	static SettingT<INISettingCollection> fInteriorWaterReflectionThreshold;
	static SettingT<INISettingCollection> bUseWaterLOD;
	static SettingT<INISettingCollection> iAnimFPS;
	static SettingT<INISettingCollection> fNearWaterUnderFreq;
	static SettingT<INISettingCollection> fNearWaterUnderVolMult;
};
