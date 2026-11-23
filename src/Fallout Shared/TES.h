#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"

#include <stdint.h>

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class bhkPickData;
class BSCloningProcess;
struct GridCell;
class GridCellArray;
class GridDistantArray;
class hkQsTransform;
class LoadedAreaBound;
class NavMeshInfoMap;
class NiCamera;
class NiCloningProcess;
class NiDirectionalLight;
class NiFogProperty;
class NiGeometry;
class NiPick;
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

enum ENUM_LOD_MULT : uint32_t;
enum HavokMaterialType : int32_t;
enum IO_TASK_PRIORITY : int32_t;

// Gives the geometry of the clone apObject new geometry data.
void MakeNewGeomData(NiAVObject* apObject);
// The printf-style printer to a named file that TestAllCells and TESDataHandler::CheckModels write their reports
// through. It is empty, as in the shipped game: it reads no argument and returns 0.
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
// Converts the game-unit point arNiP to Havok units into arhkV and returns it (the body is in
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
// Converts the game-unit length afValue to Havok units.
float NI2HK(float afValue);
// Copies the game quaternion arNiQ into arhkQ, component by component, and returns arhkQ.
inline hkQuaternion& NI2HK(hkQuaternion& arhkQ, const NiQuaternion& arNiQ) {
    arhkQ.m_vec(0) = arNiQ.m_fX;
    arhkQ.m_vec(1) = arNiQ.m_fY;
    arhkQ.m_vec(2) = arNiQ.m_fZ;
    arhkQ.m_vec(3) = arNiQ.m_fW;
    return arhkQ;
}
// Converts the game rotation arNiQ to a Havok quaternion into arhkQ and returns it.
hkQuaternion& NI2HKMIGRATION_QUATERNION(hkQuaternion& arhkQ, const NiQuaternion& arNiQ);
void Init();

// How many times an actor kind has died, kept per base form. Stores the TESActorBase the form ID resolves to
// (TES::LoadGame) and compares it with TES::GetDeadCount's argument.
struct DeathCount {
    TESActorBase* pActorBase;
    uint16_t usCount;
};

// Which cells TES::InitModelsToLoad counts: those not attached to the grid yet (preload), or those with no grid cell.
enum MODEL_COUNT_TYPE : int32_t {
    MODEL_COUNT_PRELOAD = 0,
    MODEL_COUNT_NO_GRID_CELL = 1,
};

// The world: the loaded exterior grid or the current interior cell, with the water, sky and per-cell bookkeeping.
class TES {
public:
    // Indexed [x][y] per grid cell.
    using LandPassMask = const bool[5][5];

    // A cached particle model and its clones, in the singly linked list headed by pParticleCacheHead.
    class ParticleObjectCache {
    public:
        ParticleObjectCache();
        ~ParticleObjectCache();

        ParticleObjectCache* QNext() {
            return pNext;
        }
        void SetNext(ParticleObjectCache* apNext) {
            pNext = apNext;
        }

        // The deleting destructor frees with ourFree.
        static void operator delete(void* apMemory);

        NiPointer<NiAVObject> spModel;
        // the model's clones, an array of three
        NiPointer<NiAVObject>* pspClones;
        ParticleObjectCache* pNext;
    };

    // apPathname is const char* because it is only passed to TESDataHandler::BuildFileList (const char*); Main::InitTES
    // passes "DATA\\".
    TES(const char* apPathname, NiNode* apRoot, NiNode* apLODLandRoot, Sky* apSky, NiNode* apLODWaterRoot);
    ~TES();

    // TES has this one virtual function and no virtual destructor; WinMain calls the deleting destructor directly.
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
    bool PickWorldDistance(
        NiPick* apPick,
        const NiPoint3& aOrigin,
        const NiPoint3& aDir,
        float afDistance,
        char acNodeFlags
    );
    // Picks only water along the ray, up to afDistance.
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
    void CreateTextureImage(
        const char* apfilename,
        NiPointer<NiSourceTexture>& aspSourceTexture,
        bool abNoFileOK,
        bool abArchiveOnly
    );
    // Releases the texture apPath from the palette when at most aiUser users besides the palette hold it.
    bool RemoveTextureImage(const char* apPath, uint32_t aiUser);
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
    GridCellArray* GetGridCellA() {
        return pGridCellA;
    }
    uint32_t QPlaceableWaterCount();
    // Returns the object root node (pObjRoot).
    NiNode* GetRoot();
    void CreateFurnitureList();
    // Preload and cache the shared models, forms and textures.
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

    Sky* GetSky() {
        return pSky;
    }
    NiNode* GetTempNodeManager();
    BSSimpleList<TESObjectCELL*>* GetLastLoadedExteriors() {
        return &listLastLoadedExteriors;
    }
    int32_t GetCurrentGridX() {
        return iCurrentGridX;
    }
    int32_t GetCurrentGridY() {
        return iCurrentGridY;
    }
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
    TESObjectCELL* GetExteriorBufferCellByIndex(int32_t aiIndex);
    // The number of buffered exterior cells, uExteriorCellBuffer.
    uint32_t GetExteriorBufferSize();
    void SetAllowUnusedPurge(bool abAllow);
    // Sets (abFlag false) or clears the skip-normal-maps flag of the land shader of each quad of the grid cell.
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
    void LoadGridCell(int32_t aiGridX, int32_t aiGridY, int32_t aiCellX, int32_t aiCellY);
    int32_t GetFaceCount(NiNode* apRoot, bool abCountculled);
    void InitGridCells(const NiPoint3* apPos, bool abFullreload);
    bool ClearOldestBufferedCell();
    void MoveToEndOfExteriorBuffer(TESObjectCELL* apCell);
    void SaveWorld(const char* apFileName);
    bool& ShowLoadingMessage(const char* apText);
    void AddDeadCount(TESActorBase* apObject, int16_t ascount);
    uint16_t GetSaveSize();
    void SaveGameTES();
    void SaveGameBGS(BGSSaveGameBuffer* apSaveGameBuffer);
    void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
    uint32_t GetUnusedCachedParticleObject(NiAVObject* apModel, NiPointer<NiAVObject>& arInstance);
    void CleanupTemporaryParticleCache();
    void HideUnneededLandPasses();
    void HideFarLandNormalMaps();

    static bool QCollisionDisableFlag();
    // The fade distance class of apObj's 3D.
    static ENUM_LOD_MULT GetLODMult(TESObject* apObj);
    // The deleting destructor frees with ourFree.
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
    // The temporary-node manager (BSTempNodeManager, a node; there is no class for it here yet).
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
    TESWaterSystem* pWaterSystem;
    Sky* pSky;
    BSSimpleList<NiPointer<ImageSpaceModifierInstance>> listActiveImageSpaceModifiers;
    uint32_t iTotalToLoad;
    uint32_t iLoaded;
    bool bDisablePercentageUpdate;
    bool bUpdateGridString;
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
    int32_t iPlaceableWaterCount;
    NavMeshInfoMap* pNavMeshInfoMap;
    NiPointer<LoadedAreaBound> spLoadedAreaBound;

    static inline LandPassMask kLandNormalPassMask{
        {true, true, true, true, true},
        {true, true, true, true, true},
        {true, true, true, true, true},
        {true, true, true, true, true},
        {true, true, true, true, true}
    };
    static inline bool bCollisionDisable;
    static inline bool bLODFadeInProgress;
    // Passed to bhkWorld::DoObjectRec as its record function, which takes the record data by reference.
    static inline LandPassMask
        kLandBlendPassMask{{}, {false, false, true}, {false, true, true, true}, {false, false, true}};

    // The gs_uInteriorCellBuffer_General INI setting: how many interior cells stay buffered.
    static SettingT<INISettingCollection> uInteriorCellBufferGeneral;

    // The game's one TES (the loaded world: the grid of cells around the player, the interior, the worldspace).
    static inline TES* pInstance;
};

// How many exterior cells stay buffered (gs_uExteriorCellBuffer_General), an INI setting.
extern SettingT<INISettingCollection> uExteriorCellBuffer;

// The time the cell animations have run for.
inline float fCellAppTime;

// The game settings TES.cpp defines.
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

// TES::TestAllCells' cursor in the world space list: set from TESDataHandler::GetWorldSpaceList, its item compared with
// a TESWorldSpace*, advanced to the next node.
inline BSSimpleList<TESWorldSpace*>* pCurrentWorldSpace;
inline char bbaseObjectsChecked;

// GetLandTexture's last texture found; GetLandTexture stores and returns it.
inline TESLandTexture* pGetLandTexLastTex;

extern SettingT<INISettingCollection> gs_iUpdateType_HAVOK;

extern SettingT<INISettingCollection> gs_iEntityBatchRemoveRate_HAVOK;

extern SettingT<INISettingCollection> gs_fFadeOutTime_LOD;

extern SettingT<GameSettingCollection> gs_fDebrisMaxVelocity;

extern SettingT<INISettingCollection> gs_bCheckPurgedTextureList_General;

extern SettingT<INISettingCollection> gs_fFadeInThreshold_LOD;

extern SettingT<INISettingCollection> gs_fChaseDeltaMult_HAVOK;

// Whether changing worldspace frees the buffered cells first.
extern SettingT<INISettingCollection> bPreemptivelyUnloadCells;

extern SettingT<INISettingCollection> gs_fMoveLimitMass_HAVOK;

extern SettingT<INISettingCollection> gs_fDistanceMultiplier_LOD;

extern SettingT<INISettingCollection> bAddBipedWhenKeyframed_HAVOK;

extern SettingT<GameSettingCollection> gs_fDebrisMinExtent;

extern SettingT<INISettingCollection> gs_fFadeInTime_LOD;

extern SettingT<INISettingCollection> gs_fFadeOutThreshold_LOD;

extern SettingT<INISettingCollection> gs_fMaxPickTime_HAVOK;

extern SettingT<INISettingCollection> gs_bQueueWarnings_General;

extern SettingT<INISettingCollection> gs_fMaxPickTimeVATS_HAVOK;

// The cell test's report file name.
inline const char* TestAllCells_xls = "TestAllCells.xls";

// The control value the cell test was started with.
inline uint32_t TAC_Mode;

extern SettingT<INISettingCollection> gs_iSimType_HAVOK;
