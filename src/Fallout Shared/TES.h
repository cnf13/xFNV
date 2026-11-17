#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkWorld.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class BSCloningProcess;
enum ENUM_LOD_MULT : uint32_t;
struct GridCell;
class GridCellArray;
class GridDistantArray;
enum HavokMaterialType : int32_t;
enum IO_TASK_PRIORITY : int32_t;
class LoadedAreaBound;
class NavMeshInfoMap;
class NiCamera;
class NiCloningProcess;
class NiDirectionalLight;
class NiFogProperty;
class NiGeometry;
class NiPick;
class NiQuaternion;
class QueuedFile;
class QueuedReference;
class Sky;
class TESActorBase;
class TESLandTexture;
class TESObject;
class TESObjectREFR;
class TESRegion;
class TESWaterSystem;
class TESWorldSpace;
class bhkPickData;
class hkQsTransform;

// Gives the geometry of the clone apObject new geometry data.
void MakeNewGeomData(NiAVObject* apObject);
// The printf-style printer to a named file that TestAllCells and TESDataHandler::CheckModels write their reports
// through. The shipped exe compiles it empty: it reads no argument and returns 0 (cdecl, C-variadic, 0xC3C220).
int32_t PrintToFile(const char* apFileName, const char* apFormat, ...);
// Converts the Havok length afhk to game units.
float HK2NI(float afhk);
// Converts the Havok vector arhkV to game units into arNiP and returns it.
NiPoint3& HK2NI(NiPoint3& arNiP, const hkVector4& arhkV);
// Converts the Havok vector arhkV to game units into arNiP without the world scale and returns it.
NiPoint3& HK2NINS(NiPoint3& arNiP, const hkVector4& arhkV);
// Converts the Havok transform arhkTran into arNiTran and returns it; the scale is left as it
// was.
NiTransform& HK2NI(NiTransform& arNiTran, const hkTransform& arhkTran);
// Converts the game-unit point arNiP to Havok units into arhkV and returns it (cdecl, 0x4A3E00; the body is in
// BSHavok/bhkRagdollController.cpp).
hkVector4& NI2HK(hkVector4& arhkV, const NiPoint3& arNiP);
// Copies the point arNiP into arhkV without the Havok unit scale and returns arhkV.
hkVector4& NI2HKNS(hkVector4& arhkV, const NiPoint3& arNiP);
// Converts the Havok rotation arhkRot into arNiRot and returns it.
NiMatrix3& HK2NI(NiMatrix3& arNiRot, const hkRotation& arhkRot);
// Converts the game transform arNiTran to Havok units into arhkTran and returns it.
hkTransform& NI2HK(hkTransform& arhkTran, const NiTransform& arNiTran);
// Converts the game rotation arNiRot to Havok units into arhkRot and returns it.
hkRotation& NI2HK(hkRotation& arhkRot, const NiMatrix3& arNiRot);
// Converts the game transform arNiQ to Havok units into arhkQ and returns it.
hkQsTransform& NI2HK(hkQsTransform& arhkQ, const NiTransform& arNiQ);
// Converts the game-unit length afValue to Havok units, returned in ST0 (cdecl, 0x4A3E90).
float NI2HK(float afValue);
// Converts the game rotation arNiQ to a Havok quaternion into arhkQ
// and returns it.
// Inferred type: arhkQ: hkQuaternion* from 0x8bd5f5 (&bhkRagdollController+0xc0 (this of
// bhkRagdollController::SetHeadOffset)), its members used at 0x561510
hkQuaternion& NI2HKMIGRATION_QUATERNION(hkQuaternion& arhkQ, const NiQuaternion& arNiQ);
void Init();

// How many times an actor kind has died, kept per base form (the PDB's DeadCountObject). The exe stores the
// TESActorBase the form ID resolves to (TES::LoadGame) and compares it with TES::GetDeadCount's argument.
struct DeathCount {
	TESActorBase* pActorBase;
	uint16_t usCount;
	uint8_t padding06[2];
};

// Which cells TES::InitModelsToLoad counts: those not attached to the grid yet (preload), or those with no grid cell.
enum MODEL_COUNT_TYPE : int32_t {
	MODEL_COUNT_PRELOAD = 0,
	MODEL_COUNT_NO_GRID_CELL = 1,
};

// The world: the loaded exterior grid or the current interior cell, with the water, sky and per-cell bookkeeping.
class TES {
public:
	// Indexed [x][y] per grid cell (0x45C840). PDB: TES::pLandPassMaskA, pLandNormalMaskA, const bool[5][5]; the exe
	// copies an entry into a bool without normalising it.
	using LandPassMask = const bool[5][5];

	// A cached particle model and its clones, in the singly linked list headed by pParticleCacheHead.
	class ParticleObjectCache {
	public:
		// The exe's constructor (thiscall, ret 0, 0x45AB30).
		ParticleObjectCache();
		~ParticleObjectCache();

		ParticleObjectCache* QNext() { return pNext; }
		void SetNext(ParticleObjectCache* apNext) { pNext = apNext; }

		// The exe's deleting destructor frees with ourFree (0x45AE20).
		static void operator delete(void* apMemory);

		NiPointer<NiAVObject> spModel;
		// the model's clones, an array of three
		NiPointer<NiAVObject>* pspClones;
		ParticleObjectCache* pNext;
	};

	// The exe's constructor (thiscall, ret 0x14, 0x44FB20). PDB: char* apPathname; the exe
	// only passes it to TESDataHandler::BuildFileList (const char*), and Main::InitTES passes "DATA\\".
	TES(const char* apPathname, NiNode* apRoot, NiNode* apLODLandRoot, Sky* apSky, NiNode* apLODWaterRoot);
	~TES();

	// The exe's TES vtable (0x10178B4) has this one slot and no destructor: WinMain calls the
	// deleting destructor (0x86BEE0) directly.
	virtual bool GetMapNameForLocation(BSStringT<char>& aMapName, NiPoint3 aWorldLoc, TESWorldSpace* apWorldSpace);

	// Visits every cell to test it.
	void RunCellTest(uint32_t aiOptions);
	TESLandTexture* GetLandTexture(const NiPoint3& aCoord);
	// The Havok material of the land texture at aCoord, or 2 where there is none.
	HavokMaterialType GetLandMaterialType(const NiPoint3& aCoord);
	void InitVars();
	BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* GetActiveImageSpaceModifiers();
	TESObjectCELL* GetInterior() const;
	// Flips bPrimitivesOn and shows or hides the primitives of the loaded cells.
	void TogglePrimitives();
	void SetInterior(TESObjectCELL* apCell);
	TESWorldSpace* GetWorldSpace() const;
	void ClearWorldSpace();
	TESWaterSystem* GetWaterSystem() const;
	void IncrementPlaceableWaterCount();
	BSSimpleList<TESObjectREFR*>* GetBedChairList();
	void ClearBedChairList();
	bool GetAllowUnusedPurge() const;
	TESObjectCELL* GetInteriorBufferCellByIndex(uint32_t aiIndex) const;
	GridCell* GetGridCell(int32_t aix, int32_t aiy);
	bool CellTestRunning() const;
	int16_t GetDeadCount(TESActorBase* apObject);
	bool IsCellLoaded(TESObjectCELL* apCell, bool abIgnorebuffered);
	bool IsInteriorBuffered(TESObjectCELL* apCell);
	TESObjectCELL* GetCurrentCell();
	TESObjectCELL* GetGridCellCell(int32_t aiCellX, int32_t aiCellY);
	// Picks the world along the ray, up to afDistance; acNodeFlags selects the node kinds.
	bool PickWorldDistance(NiPick* apPick, const NiPoint3& aOrigin, const NiPoint3& aDir, float afDistance,
		char acNodeFlags);
	// Picks only water along the ray, up to afDistance. In an interior the exe returns
	// TESObjectCELL::PickWaterDistance's byte unchanged, so the result is a char rather than a bool.
	bool PickWaterDistance(NiPick* apPick, const NiPoint3& aOrigin, const NiPoint3& aDir, float afDistance);
	// Unloads the loaded cells (only the buffered ones when abBufferedonly).
	void ClearAllCells(bool abBufferedonly, bool abDoNotSetInterior);
	// Frees the textures nothing uses.
	void CleanUpUnusedTextures(bool abWaitForCompletion);
	// The IO priority of loads for apCell (from apPos when it is not null).
	IO_TASK_PRIORITY GetCellPriority(TESObjectCELL* apCell, NiPoint3* apPos);
	// The navmesh info map, created on first use unless a save is loading.
	NavMeshInfoMap* GetNavMeshInfoMap();
	// Replaces the navmesh info map.
	void SetNavMeshInfoMap(NavMeshInfoMap* apNavMeshInfoMap);
	// Loads a texture file into aspSourceTexture.
	void CreateTextureImage(const char* apfilename, NiPointer<NiSourceTexture>& aspSourceTexture, bool abNoFileOK,
		bool abArchiveOnly);
	// The water height at arPos, from apCell (or the cell there when it is null) and its neighbours.
	float GetWaterHeight(const NiPoint3& arPos, TESObjectCELL* apCell);
	void InitializeWater();
	void CullGridCells(bool abDisable, NiPoint3* apPos);
	void CleanTemp3D(TESObjectREFR* apRef);
	bool CanAttach3D(TESObjectREFR* apRef);
	void PreCloneMove(TESObjectREFR* apRef);
	// Attaches apRef's 3D to apCell and runs its load-time setup; abMoved skips the pre-clone move.
	void AttachModel(TESObjectREFR* apRef, TESObjectCELL* apCell, QueuedReference* apQueuedReference, bool abMoved);
	// Re-centres the loaded grid on apPos.
	int32_t UpdateCurrentGridCell(const NiPoint3* apPos, bool abAllowPartialRemovals);
	bool IsBuffered(TESObjectCELL* apCell);
	void UpdateCellAnimations(float afTime);
	void UpdateCellMainThread();
	void LockHavokUpdateMT(bool abLock);
	void DetachGridCells();
	void AddToBuffer(TESObjectCELL* apCell);
	void AddToExteriorBuffer(TESObjectCELL* apCell);
	void SortExteriorBuffer();
	void SetUpdateGridString();
	void ClearDeadCountList();
	void AddToInteriorBuffer(TESObjectCELL* apCell);
	uint32_t FreeInteriorBuffer(bool abBufferedonly);
	bool IsExteriorBuffered(TESObjectCELL* apCell);
	uint32_t FreeExteriorBuffer(bool abBufferedonly, TESWorldSpace* apWorldSpace);
	void SetAtmosphere(TESObjectCELL* apCell);
	void RunScripts();
	void RunAnimations();
	NiAVObject* CreateDeepCopySameTextures(NiAVObject* apObject, BSCloningProcess& arCloneData);
	// Counts the references of the unloaded cells of the uGridsToLoad grid around the current grid cell (skipping
	// the cells aeType excludes) and shows the loading menu when there are any.
	void InitModelsToLoad(MODEL_COUNT_TYPE aeType);
	bool ShowLoadingMenu(bool abShow, TESForm* apLocation, bool abScripted);
	void SetWorldSpace(TESWorldSpace* apWorldSpace);
	// Reloads the grid around apPos: a full reload rebuilds the distant terrain, grid, textures and grass and fades
	// to black; otherwise only the grid cells are recentred and loaded.
	void ReloadGridCells(const NiPoint3* apPos, bool abFullreload);
	// Loads the grid cells around the current centre.
	void GridArrayLoad();
	// Loads the interior apNewInterior with the player at apPos.
	void LoadInterior(TESObjectCELL* apNewInterior, const NiPoint3* apPos);
	// Leaves the interior for the exterior around apPos.
	void LeaveInterior(NiPoint3* apPos);
	// Fills the temporary particle cache.
	void SetupTemporaryParticleCache();
	// The grid cell array (pGridCellA).
	GridCellArray* GetGridCellA() { return pGridCellA; }
	// iPlaceableWaterCount; the exe's shared one-field getter 0x4036B0 (thiscall, ret 0).
	uint32_t QPlaceableWaterCount();
	// The object root node (pObjRoot); the exe's shared one-field getter 0x84E3A0 (thiscall,
	// ret 0).
	NiNode* GetRoot();
	void CreateFurnitureList();
	// Preload and cache the shared models, forms and textures (thiscall, ret 0: 0x459F10,
	// 0x45A1E0, 0x45A370, 0x45A600).
	void PreloadAddonNodes();
	void PreloadForms();
	void PreloadDefaultModels();
	// Releases the four default particle models and the blood-decal preload texture.
	void UnloadDefaultModels();
	void PreloadPlaceableWaterNoiseTextures();
	// Clones apObject twice and caches the model and its clones, unless it is cached already.
	void AddTemporaryParticleObjectToCache(NiAVObject* apObject);
	void ActivateAllSubmergedReferencesIslands();
	// Culls the multibound nodes of the loaded cells, the grass and the water, then the rooms and portals, for
	// apCamera.
	void UpdateMultiBoundVisibility(NiCamera* apCamera);
	// Resets the cull results UpdateMultiBoundVisibility set, the land's when abLandscape and the rooms' when abRooms.
	void ResetAllMultiBoundNodes(bool abLandscape, bool abRooms);
	// Runs the cell test over every cell.
	void TestAllCells(int32_t aiControl);

	Sky* GetSky() { return pSky; }
	// The temporary-node manager (0x9611E0, a shared one-field getter).
	NiNode* GetTempNodeManager();
	BSSimpleList<TESObjectCELL*>* GetLastLoadedExteriors() { return &listLastLoadedExteriors; }
	int32_t GetCurrentGridX() { return iCurrentGridX; }
	int32_t GetCurrentGridY() { return iCurrentGridY; }
	// Cancels the exterior cell loader's queued loads.
	void ClearCellLoaderTasks();
	// The land height under acoord into afHeight; false when no loaded land is there.
	bool GetLandHeight(const NiPoint3& acoord, float& afHeight);
	// Casts arResult through the Havok world and returns the 3D it hit, or null.
	NiAVObject* Pick(bhkPickData& arResult);
	// Picks through the cell the player is in when abHavok is set, else through the scene graph (PickNI).
	NiAVObject* Pick(bhkPickData& arResult, bool abHavok);
	// Picks arResult through the scene graph.
	NiAVObject* PickNI(bhkPickData& arResult);
	// Moves the player to apCell for the cell test, logs the load times, advances the game two minutes and, for
	// test modes 4 and 5, runs the save/load test over every cell.
	void TestCell(TESObjectCELL* apCell, uint32_t aiOptions, uint32_t aiTestStartTime);
	void DecrementPlaceableWaterCount();
	// Finishes loading the world's save data.
	void FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// Reverts the world's save data.
	void Revert(BGSLoadGameBuffer* apLoadGameBuffer);
	// Shows apObject in the world for afTime seconds.
	void AddTempDebugObject(NiAVObject* apObject, float afTime);
	// The land geometry at acoord.
	NiGeometry* GetLandGeometry(const NiPoint3& acoord);
	bool IsCoordInLoadedArea(const NiPoint3& aCoord);
	TESObjectCELL* GetCellForPoint(NiPoint3 aLocation);
	void UpdateRefSounds();
	void ToggleCollisionGeometry();
	void ShowLandBorders(bool abShow);
	bool GetLandNormal(const NiPoint3& aCoord, NiPoint3& aNormal, NiPoint3& aFaceNormal);
	TESObjectCELL* GetExteriorBufferCellByIndex(int32_t a2);
	// The number of buffered exterior cells, uExteriorCellBuffer.
	uint32_t GetExteriorBufferSize();
	void SetAllowUnusedPurge(bool abAllow);
	// Sets (abFlag false) or clears the skip-normal-maps flag of the land shader of each quad of the grid cell. The
	// exe tests the whole byte of abFlag, so it is a uint8_t rather than a bool.
	void SetHideLandNormalMapForGridCell(int32_t aiGridX, int32_t aiGridY, bool abFlag);
	// Deletes the water system.
	void FreeWater();
	// Releases the preloaded addon node files.
	void UnloadAddonNodes();
	// Releases the preloaded form files.
	void UnPreloadForms();
	void SetHideLandBlendPassForGridCell(int32_t aiGridX, int32_t aiGridY, bool abFlag);
	bool GetShowLandBorders();
	bool CellTestRunning2();
	void RemoveCellFromInteriorBuffer(TESObjectCELL* apCell);
	bool GetLandColor(const NiPoint3& acoord, NiColorA& acolor);
	void QueueReferencesForAttachedCells();
	void UpdateFadeNodesForAttachedCells();
	void InitDistantTextureBlending();
	void UpdateGridLights();
	void UpdateDeferredCellAnimationTransforms();
	void ToggleCollision();
	void GetLoadedAreaRect(tagRECT& aOutRect, float afOffsetInside);
	bool AllCellsInGridLoaded();
	GridCell* FindFirstGridCellWithWater();
	void ShowCellNode(int32_t aiNode, bool abShow, bool abActiveCellsOnly);
	void LoadGridCell(int32_t aiGridX, int32_t a2, int32_t a3, int32_t a4);
	int32_t GetFaceCount(NiNode* apRoot, bool abCountculled);
	void InitGridCells(const NiPoint3* apPos, bool abFullreload);
	bool ClearOldestBufferedCell();
	void MoveToEndOfExteriorBuffer(TESObjectCELL* apCell);
	void SaveWorld(const char* apFileName);
	bool& ShowLoadingMessage(const char* apText);
	void AddDeadCount(TESActorBase* apObject, int16_t ascount);
	uint16_t GetSaveSize();
	void SaveGameTES();
	// Inferred type: a2: BGSSaveGameBuffer* from 0x84bd26 (parameter 1 of BGSSaveLoadGlobalData::SaveGlobalData)
	void SaveGameBGS(BGSSaveGameBuffer* a2);
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	uint32_t GetUnusedCachedParticleObject(NiAVObject* apModel, NiPointer<NiAVObject>& arInstance);
	void CleanupTemporaryParticleCache();
	void HideUnneededLandPasses();
	void HideFarLandNormalMaps();

	static bool QCollisionDisableFlag();
	// The fade distance class of apObj's 3D.
	static ENUM_LOD_MULT GetLODMult(TESObject* apObj);
	// The exe's deleting destructor frees with ourFree (0x86BEE0).
	static void operator delete(void* apMemory);

	static void InitFadeOutMultipliers();
	static void SetCollisionDisableFlag(bool abTF);
	static void RangedNodeVisibilityChangedCB(NiNode* apNode, bool abVisible);
	static bool GetSelectivePurgeUnusedOnFastTravel();
	static void InitSDM();
	static void KillSDM();

	GridDistantArray* pGridDistantA;
	GridCellArray* pGridCellA;
	NiNode* pObjRoot;
	NiNode* pObjLandRoot;
	NiNode* pObjLODWaterRoot;
	// the temporary-node manager (the PDB's BSTempNodeManager, a node; src has no class for it yet)
	NiNode* pTempNodeManager;
	NiDirectionalLight* pObjLight;
	NiFogProperty* pObjFog;
	int32_t iCurrentGridX;
	int32_t iCurrentGridY;
	int32_t iCurrentQueuedX;
	int32_t iCurrentQueuedY;
	TESObjectCELL* pInteriorCell;
	// the buffered interior cells, as many as gs_uInteriorCellBuffer_General
	TESObjectCELL** pInteriorBuffer;
	// the buffered exterior cells, as many as gs_uExteriorCellBuffer_General
	TESObjectCELL** pExteriorBuffer;
	uint32_t iTempInteriorBufferSize;
	uint32_t iTempExteriorBufferSize;
	int32_t iSaveGridX;
	int32_t iSaveGridY;
	bool bCollisionBoxes;
	bool bRunningCellTests;
	bool bRunningCellTests2;
	bool bRunningSubmergedObjectTest;
	void (*pfnTACCallbackFunc)(TESObjectCELL* apCell, void* apData);
	// the data pfnTACCallbackFunc is given
	void* pTACCallbackData;
	TESRegion* pTACRegionFilter;
	bool bShowLANDborders;
	uint8_t padding61[3];
	TESWaterSystem* pWaterSystem;
	Sky* pSky;
	BSSimpleList<NiPointer<ImageSpaceModifierInstance>> listActiveImageSpaceModifiers;
	uint32_t iTotalToLoad;
	uint32_t iLoaded;
	bool bDisablePercentageUpdate;
	bool bUpdateGridString;
	uint8_t padding7E[2];
	float fCell_delta_x;
	float fCell_delta_y;
	TESWorldSpace* pWorldSpace;
	BSSimpleList<TESObjectCELL*> listLastLoadedExteriors;
	BSSimpleList<TESObjectREFR*> ListofBedsAndChairs;
	BSSimpleList<DeathCount*> DeadCount;
	NiPointer<QueuedFile> spPreloadedAddonNodes;
	NiPointer<NiSourceTexture> BloodDecalPreload1;
	NiPointer<QueuedFile> spPreloadedForms;
	ParticleObjectCache* pParticleCacheHead;
	bool bFadeWhenLoading;
	bool bAllowUnusedPurge;
	uint8_t paddingB6[2];
	int32_t iPlaceableWaterCount;
	NavMeshInfoMap* pNavMeshInfoMap;
	NiPointer<LoadedAreaBound> spLoadedAreaBound;

	static inline LandPassMask kLandNormalPassMask{{true, true, true, true, true}, {true, true, true, true, true},
		{true, true, true, true, true}, {true, true, true, true, true}, {true, true, true, true, true}};
	static inline bool bCollisionDisable;
	static inline bool bLODFadeInProgress;
	// Not named in the PDB or IDA; name chosen here. The exe passes it to bhkWorld::DoObjectRec as its record function,
	// which takes the record data by reference.
	static inline LandPassMask kLandBlendPassMask{{}, {false, false, true}, {false, true, true, true},
		{false, false, true}};

	// The gs_uInteriorCellBuffer_General setting: how many interior cells stay buffered. The exe builds it with
	// 0x45CFE0 (0xF382DF) and destroys it with 0x44F4F0 (0xFC8088): an INI setting.
	static SettingT<INISettingCollection> uInteriorCellBufferGeneral;

	// The game's one TES (the loaded world: the grid of cells around the player, the interior, the worldspace).
	static inline TES* pInstance;
};

// How many exterior cells stay buffered (gs_uExteriorCellBuffer_General). The exe builds it with 0x45CFE0 (0xF38346)
// and destroys it with 0x44F4F0 (0xFC8098): an INI setting.
extern SettingT<INISettingCollection> uExteriorCellBuffer;

// The time the cell animations have run for.
inline float fCellAppTime;
inline constexpr fnvd::Vtable TES_vftable{};

// The game settings TES.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> sFailureMessage;
extern SettingT<INISettingCollection> sSysUtil_GameDataInsufficientSpace;
extern SettingT<INISettingCollection> sSysUtil_SaveDataInsufficientSpace;
extern SettingT<INISettingCollection> sLanguage;
extern SettingT<INISettingCollection> sSysUtil_GameDataCorrupt;
extern SettingT<GameSettingCollection> fCharacterControllerMultipleStepSpeed;
extern SettingT<RendererSettingCollection> fLODFadeOutMultItems;
extern SettingT<INISettingCollection> bUseMultiThreadedTrees;
extern SettingT<INISettingCollection> sSysUtil_DiscEject;
extern SettingT<RendererSettingCollection> fLODFadeOutMultObjects;
extern SettingT<INISettingCollection> sDPad;
extern SettingT<INISettingCollection> sSysUtil_SaveDataCreateNew;
extern SettingT<INISettingCollection> sSysUtil_Retry;
extern SettingT<INISettingCollection> sSysUtil_DirtyDisc;
extern SettingT<GameSettingCollection> fFadeToBlackFadeSeconds;
extern SettingT<INISettingCollection> sThumbstick;
extern SettingT<INISettingCollection> iNumHWThreads;
extern SettingT<INISettingCollection> sSysUtil_AutoSaveWarning;
extern SettingT<RendererSettingCollection> fLODFadeOutMultActors;
inline int32_t imouseDelta = 1;
inline int32_t iGetLandTexLastY = 2147483647;
inline int32_t iGetLandTexLastX = 2147483647;
inline int32_t uGridsArea;
inline int32_t iworldMinY;
inline int32_t iworldCellX;
inline uint32_t iinteriorCellIndex;
inline bool bnewWorldNextLoop;
inline int32_t iinteriorCellCount;

// IDA name; not in the PDB. TES::TestAllCells' cursor in the world space list: set from
// TESDataHandler::GetWorldSpaceList (PDB: BSSimpleList<TESWorldSpace*>*), its item (+0) compared with a
// TESWorldSpace*, advanced to the next node (+4). IDA's type NiPlane* fits none of these accesses.
inline BSSimpleList<TESWorldSpace*>* pCurrentWorldSpace;
inline char bbaseObjectsChecked;

// IDA name; not in the PDB. GetLandTexture's last texture found (0x457720 stores and returns it).
inline TESLandTexture* pGetLandTexLastTex;

// INISettingCollection: 0xf3848a calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_iUpdateType_HAVOK;

// INISettingCollection: 0xf385da calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_iEntityBatchRemoveRate_HAVOK;

// INISettingCollection: 0xf38762 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fFadeOutTime_LOD;

// GameSettingCollection: 0xf38672 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fDebrisMaxVelocity;

// INISettingCollection: 0xf3878a calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bCheckPurgedTextureList_General;

// INISettingCollection: 0xf38702 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fFadeInThreshold_LOD;

// INISettingCollection: 0xf38612 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fChaseDeltaMult_HAVOK;

// Whether changing worldspace frees the buffered cells first.
// INISettingCollection: 0xf382aa calls 0x45cf30.
extern SettingT<INISettingCollection> bPreemptivelyUnloadCells;

// INISettingCollection: 0xf384c2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fMoveLimitMass_HAVOK;

// INISettingCollection: 0xf386a2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fDistanceMultiplier_LOD;

// INISettingCollection: 0xf384ea calls 0x45cf30.
extern SettingT<INISettingCollection> bAddBipedWhenKeyframed_HAVOK;

// GameSettingCollection: 0xf38642 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fDebrisMinExtent;

// INISettingCollection: 0xf38732 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fFadeInTime_LOD;

// INISettingCollection: 0xf386d2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fFadeOutThreshold_LOD;

// INISettingCollection: 0xf38522 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fMaxPickTime_HAVOK;

// INISettingCollection: 0xf387ba calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bQueueWarnings_General;

// INISettingCollection: 0xf38552 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fMaxPickTimeVATS_HAVOK;

// IDA: TestAllCells.xls; not in the PDB. The cell test's report file name.
inline const char* TestAllCells_xls = "TestAllCells.xls";

// IDA: TAC::Mode; not in the PDB. The control value the cell test was started with.
inline uint32_t TAC_Mode;

// INISettingCollection: 0xf558fa calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_iSimType_HAVOK;
