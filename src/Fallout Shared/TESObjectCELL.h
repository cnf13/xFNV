#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

#include <stdint.h>

class Actor;
class BGSEncounterZone;
class BGSLightingTemplate;
class BGSMusicType;
class bhkPickData;
class bhkWorldM;
class BSMultiBoundNode;
class BSPortalGraph;
class CellMopp;
class FOIslandActivationListener;
class IntSeenData;
class InventoryChanges;
class NavMeshArray;
class NiCamera;
class NiPick;
class SeenData;
class TESBoundObject;
class TESClimate;
class TESGlobal;
class TESImageSpace;
class TESObjectCELL;
class TESObjectLAND;
class TESObjectREFR;
class TESRegionList;
class TESRegionObjectBase;
class TESRegionPoint;
class TESWaterForm;
class TESWorldSpace;

enum REGION_DATA_ID : int32_t;

// Constructs the child attach map's node in apMemory and
// returns apMemory; the map's node allocation calls it.
void* ConstructQueuedAttachMapItem(void* apMemory, [[maybe_unused]] void* apEdx);
float GetDistanceBetweenPoints(float afX1, float afY1, float afX2, float afY2);
void SyncFromHavok(NiNode* arg0);

// The lighting and ownership record of an interior cell.
struct INTERIOR_DATA {
    INTERIOR_DATA();

    // Byte-swaps the record read from a big-endian file.
    void Endian();

    uint32_t iAmbient;
    uint32_t iDirectional;
    uint32_t iFog;
    float fFogNear;
    float fFogFar;
    uint32_t iDirectionalXY;
    uint32_t iDirectionalZ;
    float fDirectionalFade;
    float fClipDist;
    float fFogPower;
    uint32_t iInteriorOffset;
};

// The grid position of an exterior cell.
struct EXTERIOR_DATA {
    EXTERIOR_DATA();

    // Byte-swaps the grid position read from a big-endian file.
    void Endian();

    int32_t iCellX;
    int32_t iCellY;
    uint8_t ucLandHideFlags;
    uint8_t padding09[3];
};

// The 3D and bookkeeping a cell holds while it is loaded.
struct LoadedCellData {
    LoadedCellData();
    ~LoadedCellData();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    NiPointer<NiNode> spCell3D;
    // the references whose bound is large enough to animate from afar
    BSSimpleList<TESObjectREFR*> LargeAnimatedRefs;
    // animated references and their root nodes
    NiTMap<TESObjectREFR*, NiNode*> AnimatedRefMap;
    // emittance sources (lights, regions) and the reference that uses each
    NiTMap<TESForm*, TESObjectREFR*> EmittanceSourceRefMap;
    // light references and their root nodes
    NiTMap<TESObjectREFR*, NiNode*> EmittanceLightRefMap;
    // multibound references and their multibound nodes
    NiTMap<TESObjectREFR*, NiPointer<BSMultiBoundNode>> MultiboundRefMap;
    BSSimpleList<TESObjectREFR*> ScriptedRefs;
    BSSimpleList<TESObjectREFR*> ActivatingRefs;
    BSSimpleList<TESObjectREFR*> WaterRefs;
};

// A cell: an interior room or one square of a worldspace's exterior grid.
class TESObjectCELL : public TESForm, public TESFullName {
public:
    // The render statistics of a cell: what it drew and how long it took.
    struct RENDER_DATA {
        RENDER_DATA();

        uint32_t iGeometryCount;
        uint32_t iTriangleCount;
        uint32_t iNewRenderPassCount;
        uint32_t iActiveLights;
        uint32_t iTotalTime;
        uint32_t iAccumulateTime;
    };

    struct RENDER_FAILURE_DATA;

    // The limits a cell render test checks RENDER_DATA against, and what to call when it fails.
    struct RENDER_TEST_DATA {
        RENDER_TEST_DATA();

        RENDER_DATA Thresholds;
        void (*pFailureCallbackFunc)(RENDER_FAILURE_DATA* apFailureData, void* apCallbackData);
        void* pFailureCallbackData;
    };

    // What a failed cell render test hands the callback: the statistics and the cell and view that failed.
    struct RENDER_FAILURE_DATA {
        RENDER_FAILURE_DATA();

        RENDER_DATA RenderData;
        TESObjectCELL* pTestCell;
        NiPoint3 TestPos;
        NiMatrix3 TestRot;
    };

    enum CELL_STATE : uint32_t {
        CS_NOTLOADED = 0x0,
        CS_UNLOADING = 0x1,
        CS_LOADING = 0x2,
        CS_LOADED = 0x3,
        CS_DETACHING = 0x4,
        CS_ATTACHING = 0x5,
        CS_ATTACHED = 0x6,
    };

    // The children of the cell's root node.
    enum CELLNODE : uint32_t {
        CN_ACTOR = 0x0,
        CN_MARKER = 0x1,
        CN_LAND = 0x2,
        CN_STATIC = 0x3,
        CN_DYNAMIC = 0x4,
        CN_OCCLUSION_PLANE = 0x5,
        CN_PORTAL = 0x6,
        CN_MULTIBOUND = 0x7,
        CN_COLLISION = 0x8,
        CN_LIGHT_MARKER = 0x9,
        CN_SOUND_MARKER = 0xA,
        CN_WATER_WADE = 0xB,
        // No node: the default value stored and tested.
        CN_NONE = 0xFFFFFFFF,
    };

    // The bits of iNodeCullFlags: one per child node of the cell's root, CCN_WATER standing for the wade water.
    enum CELL_CULL_NODE : int32_t {
        CCN_ACTOR = 0,
        CCN_MARKER = 1,
        CCN_LAND = 2,
        CCN_STATIC = 3,
        CCN_DYNAMIC = 4,
        CCN_OCCLUSION_PLANE = 5,
        CCN_PORTAL = 6,
        CCN_MULTIBOUND = 7,
        CCN_COLLISION = 8,
        CCN_LIGHT_MARKER = 9,
        CCN_SOUND_MARKER = 10,
        CCN_WATER = 11,
    };

    // The bits of iLightingTemplateInheritanceFlags: the values the cell takes from its lighting template.
    enum LightingInheritFlag : uint32_t {
        kInherit_AmbientColor = 0x1,
        kInherit_DirectionalColor = 0x2,
        kInherit_FogColor = 0x4,
        kInherit_FogNear = 0x8,
        kInherit_FogFar = 0x10,
        kInherit_DirectionalRotation = 0x20,
        kInherit_DirectionalFade = 0x40,
        kInherit_ClipDistance = 0x80,
        kInherit_FogPower = 0x100,
    };

    // The bits of cCellFlags.
    enum CellFlag : uint8_t {
        kCellFlag_Interior = 0x01,
        kCellFlag_HasWater = 0x02,
        kCellFlag_HasTempData = 0x10,
        kCellFlag_Public = 0x20,
        kCellFlag_TempPublic = 0x40,
        kCellFlag_BehavesLikeExterior = 0x80,
    };

    // A child 3D waiting to be attached to its parent node on the main thread.
    struct QUEUED_ATTACH {
        QUEUED_ATTACH() = default;
        // Builds the by-value arguments of QueueAttachChild and the map's SetAt, through Copy.
        QUEUED_ATTACH(const QUEUED_ATTACH& arOther) {
            Copy(&arOther);
        }
        // Sets both pointers to null and clears the flag; aiVal is not read.
        QUEUED_ATTACH(int32_t aiVal);

        // Copy constructor: copies a2's pointers without releasing this one's, and its flag.
        TESObjectCELL::QUEUED_ATTACH* Copy(const TESObjectCELL::QUEUED_ATTACH* a2);
        // The copy assignment the map templates call.
        QUEUED_ATTACH& operator=(const QUEUED_ATTACH& __that);

        NiPointer<NiAVObject> spAttachChild;
        NiPointer<NiNode> spAttachNode;
        bool bFirstAvail = true;
    };

    using QueuedChildAttachMapType = NiTMap<NiAVObject*, QUEUED_ATTACH>;
    using QueuedParticleUpdateList = BSSimpleList<NiPointer<NiAVObject>>;
    using MultiboundTestMapType = NiTMap<TESObjectREFR*, TESObjectREFR*>;

    TESObjectCELL();
    ~TESObjectCELL() override;

    void ClearData() override;
    bool Load(TESFile* apFile) override;
    bool Save(TESFile* apFile) override;
    void Save() override;
    bool SaveEdit(TESFile* apFile) override;
    bool SavesBefore(TESForm* apForm) override;
    bool SavesBefore(FORM* apGroupFORM) override;
    TESForm* CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) override;
    uint16_t GetSaveSize(uint32_t aiFlags) override;
    void SaveGame(uint32_t aiFlags) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void Revert(uint32_t aiFlags) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    bool FindInFileFast(TESFile* apFile) override;
    void InitItem() override;
    void GetFormDetailedString(BSStringT<char>& aString) override;
    void SetAltered(bool abAltered) override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    bool BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) override;
    void CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup) override;
    bool IsParentForm() override {
        return true;
    }
    bool IsFormTypeChild(uint8_t aeType) override;
    const char* GetFormEditorID() override;
    bool SetFormEditorID(const char* apEditorID) override;

    // Whether apActor owns the cell.
    bool IsActorCellOwner(Actor* apActor);
    // Drops the cell's location markers.
    void ClearLocationMarkers();
    // A debug marker at arLocation, named apName, drawn in apColor.
    void AddLocationMarker(const NiPoint3& arLocation, const char* apName, const NiColorA* apColor);
    // Rotates aStartCoord about the cell's north marker into aOutCoord (clockwise with
    // abClockwise; the two may be the same point).
    void AdjustCoordForNorthRotation(const NiPoint3& aStartCoord, NiPoint3& aOutCoord, bool abClockwise);
    // Fills apList with the cell's map marker references.
    void AddMapMarkersToList(BSSimpleList<TESObjectREFR*>* apList);
    void UpdateManagedNodeTransforms();
    // Shows or hides the primitives of the cell's references.
    void ShowPrimitives(bool abEnable);
    // Sets the cell's owner.
    int32_t SetCellOwnership(TESForm* apOwner);
    bool PickWaterDistance(NiPick* apPick, const NiPoint3& aOrigin, const NiPoint3& aDir, float afDistance);
    void FreeLoadedData();
    void SkipVisibleDistantFade();
    // Runs SkipVisibleDistantFade on every buffered exterior cell.
    void ForceLODFadeInLoadedCells();
    // Stores the current hour as the detach time, without forcing it.
    void SetDetachTime();
    // Culls or shows the 3D of the references under the multibound node's children, for CCN_DYNAMIC: lights and the
    // plane marker excepted.
    void CullChildrenOfMultibounds(bool abCull, CELL_CULL_NODE aeAsProxyForNode);
    void AddReferences(TESWorldSpace* apWorld);

    ExtraDataList* GetExtra();
    CELL_STATE GetState() const;
    void SetState(CELL_STATE aeState);
    // Whether GetState() is CS_UNLOADING.
    bool GetUnloading();
    bool GetCantWaitHere();
    // Lowers the processes of the cell's actors to middle low at most.
    void SetLowestProcessToMiddleLow(bool abFlag);
    bool IsLoaded() const;
    bool IsLoading() const;
    bool GetAttached() const;
    bool IsAttaching() const;

    int32_t GetCriticalQueuedRefCount() const;
    void IncQueuedRefCount();
    void IncCriticalQueuedRefCount();
    void DecQueuedRefCount();
    void DecCriticalQueuedRefCount();
    // The land height under arPos into arHeight; false off the cell's land.
    bool GetLandHeight(const NiPoint3& arPos, float& arHeight);

    NiNode* GetLandNode(uint32_t aiQuad);
    void UpdateRefSounds();
    bool ToggleCollisionGeometry();
    void UpdateSounds();
    void QueueReferences(bool abHighPriorityOnly);
    // Turns the fade nodes of the cell's references back on with full alpha.
    void UpdateFadeNodes();
    // Shows or hides the Havok debug display of the cell's 3D and its actors' collision geometry.
    void SetCollisionGeometryDisplay(bool abTF);
    // Shows or hides the actors' collision geometry.
    void SetActorCollisionGeometry(bool abTF);
    int32_t GetReferenceCount();
    BSSimpleList<TESObjectREFR*>* GetRefList();
    // Hands every carryable item the cell's references hold to apBagRef, the default loot bag.
    void FillLootBag(TESObjectREFR* apBagRef);
    void AddReference(TESObjectREFR* apRef, bool abOnTop);
    void RemoveReference(TESObjectREFR* apRef);
    // Moves apRef into the cell's reference list (a persistent cell keeps it as its
    // persistent ref) and marks the cell altered as needed.
    void AddReferenceSimple(TESObjectREFR* apRef);
    // Sets or clears the persistent form flag (0x400). Tests the whole byte; bool would assume 0/1.
    void SetPersistentCell(bool abVal);
    // Adds the cell's enabled teleport doors, the jail marker's excepted, to arOutArray.
    void BuildTeleportDoorList(BSSimpleArray<TESObjectREFR*>& arOutArray);
    bhkWorld* GetbhkWorld();
    // Runs the scripts of the cell's references; true when the loaded scripted list was walked and a script ran
    // under bIgnore3D false.
    bool RunScripts(bool bIgnore3D, bool bIgnoreActors);
    // Calls apfnEnumFunc for the references near either point; false when apfnEnumFunc is null or asks to stop.
    bool EnumReferencesCloseToPoint(
        const NiPoint3& aPoint1,
        float afRadius1,
        const NiPoint3& aPoint2,
        float afRadius2,
        bool (*apfnEnumFunc)(TESObjectREFR*, void*),
        void* aiEnumData
    );
    // Locks and unlocks kSpinLock, which guards the cell's references and 3D.
    void CellRefLockEnter();
    void CellRefLockLeave();
    // Builds the cell's root node and its children.
    NiNode* Load3D();
    // The water height of the cell itself.
    float GetWaterHeight();
    // The water height at arPos, from the cell's water references; afHeight gets the cell's height first.
    bool GetWaterHeight(const NiPoint3& arPos, float& afHeight);
    // Sets the cell's water height; FLT_MAX when it equals the world space's default.
    void SetWaterHeight(float afWaterHeight);
    void AttachReference3D(TESObjectREFR* apRef, bool abOnTop);
    void PerformCellNodeAttach(TESObjectREFR* apRef, NiNode* apParent);
    // Counts one more loaded reference with visible-when-distant 3D.
    void IncLoadedRefsWithVisibleDistantCount();
    // Places a tree's canopy shadow of radius aiRadius at aLocation.
    void PlaceCanopyShadow(NiPoint3 aLocation, uint32_t aiRadius, float afFalloff);
    // Replaces pCellData with a fresh INTERIOR_DATA or EXTERIOR_DATA.
    void CreateCellData();
    // The interior lighting values; each comes from the lighting template when the cell inherits it.
    uint32_t GetDataAmbient();
    void GetDataAmbientColor(NiColor& acolColor);
    uint32_t GetDataDirectional();
    void GetDataDirectionalColor(NiColor& acolColor);
    uint32_t GetDataDirectionalXY();
    uint32_t GetDataDirectionalZ();
    uint32_t GetDataFog();
    void GetDataFogColor(NiColor& acolColor);
    float GetDataFogNearPlane();
    float GetDataFogFarPlane();
    float GetDataFogPower();
    float GetDataClipDist();
    // Creates pLoadedData and files the cell's references in its lists.
    void CreateLoadedData();
    void AddAnimatedRef(TESObjectREFR* apRef);
    void AddEmittanceRef(TESObjectREFR* apRef);
    // Files placeable water apRef in the loaded data's water list.
    void AddWaterRef(TESObjectREFR* apRef);
    // Whether every reference with visible-distant 3D has loaded, and whether the cell should
    // show its high detail.
    bool All3DWithVisibleDistantLoaded();
    bool ShouldDisplayHighDetail();
    void FadeToHighDetail();
    // Files a multibound reference in MultiboundRefMap and returns its node.
    BSMultiBoundNode* AddMultiBoundRef(TESObjectREFR* apRef);
    // Drops one from the count of the cell's visible-distant references.
    void DecRefsWithVisibleDistantCount();
    // The multibound node of apRef from the cell's map, else a fresh one filed there.
    BSMultiBoundNode* GetMultiBoundNode(TESObjectREFR* apRef);
    // Attaches the multibound nodes that have no parent to the cell's multibound node.
    void AttachMultiBoundNodes();
    BSPortalGraph* CreatePortalGraph();
    // The cell's owner, else its encounter zone's.
    TESForm* GetOwner();
    // The region list extra; with abCreateIfNotFound an exterior cell gets an empty one.
    TESRegionList* GetRegionList(bool abCreateIfNotFound);
    BGSMusicType* GetMusicType(NiPoint3* apLoc);
    TESClimate* GetClimate();
    // Whether apActor trespasses by being in the cell.
    bool CheckifTrespassing(Actor* apActor);
    // The image space, else the worldspace's, else the default interior or exterior one.
    TESImageSpace* GetUsableImageSpace();
    // Tests apRef's 3D against the cell's multibounds and rooms and moves it into the one that holds it.
    void TestObjectAgainstMultibounds(TESObjectREFR* apRef, bool abForceRoomTest, bool abMultipleRoomTest);
    // Tests apRef's 3D apRef3D against the room of apBoundRef and attaches it there when it is inside; abFoundRoom
    // is set when a room held it.
    bool TestObjectAgainstRoom(
        TESObjectREFR* apRef,
        NiAVObject* apRef3D,
        TESObjectREFR* apBoundRef,
        bool& abFoundRoom,
        bool abMultipleRoomTest
    );
    // Makes this cell the decal's parent cell when it has none, then hands the decal to the
    // decal manager.
    void AddDecal(DECAL_CREATION_DATA& arDecalData, BGSDecalManager::DECAL_TYPE aeDecalType, bool abForceAdd);
    void AddAudioMarkersToList(BSSimpleList<TESObjectREFR*>* apAudioList);
    void AttachSounds(bool abAttach);
    void AttachLights(bool abAttach);
    // Loads the cell's 3D, then attaches or queues its references (multibound markers first) and enters CS_ATTACHED.
    void AttachModels(bool abSetupMopp);
    void UpdateReferences3DandProcess();
    // The centre of the bounding box of the cell's references, moving those at invalid positions.
    void CalcRefCenterPoint(NiPoint3& aOutCenter);
    // The cell's enabled, undeleted furniture references, added to apFurnList.
    void AddFurnitureToList(BSSimpleList<TESObjectREFR*>* apFurnList);
    // Fills the coc destination: the exterior cell's centre and no rotation, then, unless bDefaultCOCPlacement
    // holds for an exterior cell, the placement DetermineCOCPlacement finds after loading the temp data.
    void GetCOCPlacementInfo(NiPoint3& aOutPos, NiPoint3& aOutRot);
    void DetermineCOCPlacement(NiPoint3& aOutPos, NiPoint3& aOutRot);
    // The record offset of an interior cell; 0 for an exterior one.
    uint32_t GetInteriorOffset();
    bool HandleExpiredReferences();
    bool LoadAllTempData();
    bool LoadTempDataFromFile(TESFile* apFile);
    // Returns leveled creatures due for a reset to their leveled base and recalculates their templates. AttachModels
    // passes HandleExpiredReferences's result as abAllowRecalc; the body never reads it.
    void CalcLeveledCreatures(bool abAllowRecalc);
    // Steps the cell's Havok world and updates its animated nodes, emittance colors and LOD fade.
    void UpdateManagedNodes(float afTime, bool abLargeOnly);
    BGSAcousticSpace* GetAcousticSpace();
    bool IsPointInCellExterior(const NiPoint3& aPt);
    void AttachToWorld(NiNode* apRoot);
    // Builds an interior cell's Havok world and the cell's MOPP where missing.
    void InitHavok();
    void SetupMopp();
    // Updates the animation of the attached cell's animated references, actors and trees excepted.
    void RunAnimations();
    // The north rotation of an interior cell; 0 for an exterior one.
    float GetNorthRotation();
    void AddReferencesToLoadedReferenceCollection();
    // Registers the cell's obstacle references and doors with the obstacle manager.
    void AddObstaclesAndDoors();
    // Adds (abAttach) or removes the master particle addon nodes of the cell's loaded references.
    void ProcessAddonNodesForRefs(bool abAttach);
    void ActivateAllSubmergedReferencesIslands();
    void RemoveScriptedRef(TESObjectREFR* apRef);
    void RemoveActivatingRef(TESObjectREFR* apRef);
    void RemoveAnimatedRef(TESObjectREFR* apRef);
    void RemoveEmittanceRef(TESObjectREFR* apRef);
    // Files apRef in the loaded data's ScriptedRefs.
    void AddScriptedRef(TESObjectREFR* apRef);
    // Files apRef in the loaded data's ActivatingRefs.
    void AddActivatingRef(TESObjectREFR* apRef);

    NiNode* Get3D() const;
    NiNode* GetChildNode(CELLNODE aeIndex) const;
    // The marker node is the cell's CN_MARKER child node (GetChildNode, a NiNode), and its callers call NiNode virtuals
    // (AttachChild) on it.
    NiNode* GetMarkerNode() const;
    NiAVObject* GetDynamicNode() const;
    NiAVObject* GetActorNode() const;
    NiAVObject* GetStaticNode() const;
    NiNode* LightMarkerNode() const;
    // Returns spSoundMarkerNode.
    NiNode* SoundMarkerNode();
    BSPortalGraph* GetPortalGraph() const;

    // The first reference in the cell's object list, read under the reference lock.
    TESObjectREFR* GetFirstRefr();

    bool GetPersistentCell();
    bool GetInterior() const;
    // The canopy shadow mask's state (an ExtraCellCanopyShadowMask::CELL_STATE; CS_NONE for
    // an interior), its texture in *apOutTex and its locked rect in *apOutRect.
    int32_t GetCanopyShadowMask(NiRenderedTexture*& apOutTex, _D3DLOCKED_RECT*& apOutRect);
    // Picks arResult through the cell's Havok world and returns the 3D it hit, or null.
    NiAVObject* Pick(bhkPickData& arResult);
    bool HasWater() const;
    bool GetPublic() const;
    bool GetTempPublic() const;
    bool GetPublicState();
    bool GetFakeExterior() const;
    bool GetHasTempData() const;
    void SetHasTempData(bool abVal);
    LoadedCellData* GetLoadedData();
    void SetTempPublic(bool abVal);

    TESGlobal* GetOwnershipGlobal();
    int32_t GetOwnershipRank();
    uint32_t GetDetachTime();
    // Stores the hour the cell was detached.
    void SetDetachTime(uint32_t aiTime, bool abForce);
    // Clears the cell's seen data and its fully-seen flag.
    void ClearSeenData();
    // Sets or clears the fully-seen flag. Tests the whole byte; bool would assume 0/1.
    void SetFullySeen(bool abVal);
    // The number of light references in the cell, counted under the reference lock.
    uint32_t GetLightCount();
    // The cell's loot bag reference (base object pDefLootBag), or null.
    TESObjectREFR* GetLootBag();
    TESImageSpace* GetImageSpace();
    ImpactSwap* GetImpactSwap();
    BGSEncounterZone* GetEncounterZone();
    CellMopp* GetCellMopp();
    bool GetLowestProcessMiddleLow();
    const char* GetWaterNoiseTextureName() const;
    const char* GetCellName();

    BGSLightingTemplate* GetLightingTemplate() const;
    void SetLightingTemplate(BGSLightingTemplate* apTemplate);
    bool GetInheritFlag(uint32_t aeFlag) const;

    EXTERIOR_DATA* GetExteriorData() const;
    INTERIOR_DATA* GetInteriorData() const;
    int32_t GetDataX() const;
    int32_t GetDataY() const;
    void SetInteriorOffset(uint32_t aiOffset);

    TESWorldSpace* GetWorldSpace() const;
    void SetWorldSpace(TESWorldSpace* apWorldSpace);
    // an interior cell keeps its temp data offset in the slot an exterior cell keeps its worldspace in
    uint32_t GetTempDataOffset() const;
    // Sets the temp data offset of an interior cell.
    void SetTempDataOffset(uint32_t aiOffset);
    void SetNavMeshArray(NavMeshArray* apNavMeshArray);
    NavMeshArray* GetNavMeshArray();
    // The cell's land, or null.
    TESObjectLAND* GetLand();
    // The scale the regions over apTest give apObjectBase's aID data.
    float GetScaleBasedOnLocation(TESRegionPoint* apTest, REGION_DATA_ID aID, TESRegionObjectBase* apObjectBase);
    void SetLand(TESObjectLAND* apLand);
    // Detaches the cell's 3D (and its Havok world when abDetachHavok).
    void Detach(bool abDetachHavok);
    // Whether the reference list is locked.
    bool IsRefListLocked();
    void SetHasWater(bool abNewbool);
    uint32_t GetGroupBlockKey();
    uint32_t GetGroupSubBlockKey();
    void SetInterior(bool abVal);
    bool CantTravelFrom();
    bool GetHideLand(uint32_t aiQuad);
    void SetDataCoord(int32_t aiX, int32_t aiY);
    BSSimpleList<TESObjectREFR*>* GetWaterRefList();
    void ClearQueuedRefCount();
    void ClearCriticalQueuedRefCount();
    void SetImageSpace(TESImageSpace* apImageSpace);
    float GetDistanceFromPoint(const NiPoint3& aPos);
    bool GetFullySeen();
    bool HasReferences();
    bool IsOffLimits();
    bool All3DWithVisibleDistantFadingIn();
    SeenData* GetSeenData();
    void SetAutoWaterLoaded(bool abVal);
    void Clear3D();
    bool PickWorldDistance(
        NiPick* apPick,
        const NiPoint3& aOrigin,
        const NiPoint3& aDir,
        float afDistance,
        char acNodeFlags
    );
    void ChangeProcessLevel(bool abChangeLevel);
    void ClearReference3D();
    void UnLoadAllTempData();
    void ClearPortalGraph();
    TESWaterForm* GetWaterType();
    TESObjectREFR* CreateLootBagAtRef(TESObjectREFR* apRef);
    IntSeenData* GetIntSeenSection(int32_t aiSectionX, int32_t aiSectionY, bool abCreate);
    void RemovePlaceableWaterRef(TESObjectREFR* a2);
    bool IsActorBaseCellOwner(TESActorBase* apActorBase);
    void RemoveWaterRef(TESObjectREFR* apRef);
    void RemoveReferencesFromLoadedReferenceCollection();
    void BuildLocalMapDoorList(BSSimpleList<TESObjectREFR*>* apOutList);
    void GetExteriorLocalMapTexture(NiPointer<NiTexture>& aspTexture);
    void GetInteriorLocalMapTexture(int32_t aiSectionX, int32_t aiSectionY, NiPointer<NiTexture>& aspTexture);
    void SaveReferences(TESFile* apFile);
    // Sets one byte of the canopy shadow mask of the cell at (aiX, aiY).
    void PlaceCanopyShadowInAdjacentCell(
        int32_t aiX,
        int32_t aiY,
        _D3DLOCKED_RECT*& apRect,
        int32_t aiIndex,
        int32_t aiValue
    );
    void ProcessActors();
    void FillBarterContainer(TESObjectREFR* apActor, InventoryChanges* apBarterContainer);
    void SaveGameTest();
    void UpdateLights();
    void Destroy();
    TESObjectREFR* SearchForm(TESBoundObject* a1, char a2);
    uint32_t FindTeleportDoorsForLocation(
        const NiPoint3& arLocation,
        float afRadius,
        BSSimpleArray<TESObjectREFR*, 1024>* apRetDoors
    );
    void TakeLocalMapPicture(NiCamera* apCamera, NiPointer<BSRenderedTexture>& aspRenderedTexture);
    void TakeExteriorLocalMapPicture(NiPointer<BSRenderedTexture>& aspRenderedTexture);
    void TakeInteriorLocalMapPicture(
        int32_t aiSectionX,
        int32_t aiSectionY,
        NiPointer<BSRenderedTexture>& aspRenderedTexture
    );
    void DetachHavok();
    void UpdateSeenData(const NiPoint3& aCoord);
    void UpdateTrees();
    void RemoveNavmeshObstacles();
    void FadeToLowDetail();
    void SetPublic(bool abVal);

    static bhkWorldM* ExteriorWorld();
    static bool IsNodeCulled(CELL_CULL_NODE aiNode);
    // Sets or clears node aiNode's bit in iNodeCullFlags.
    static void SetNodeCulled(CELL_CULL_NODE aiNode, bool abVal);
    // Only the low 16 bits of aiX and aiY are read; callers pass the full ints.
    static int32_t GetCoord(int32_t aiX, int32_t aiY);
    // Whether apRef is a key of MultiboundTestMap.
    static bool IsRefInMultiboundTestMap(TESObjectREFR* apRef);
    // The iHoursToRespawnCell game setting.
    static uint32_t GetHoursToRespawnCell();
    // Removes the island listener from the exterior world, deletes it, and releases the exterior world.
    static void KillStatics();
    // TESWaterSystem::pLastWaterCell.
    static TESObjectCELL* GetLastWaterCell();
    static bool FindLandDataInFile(TESFile* apFile);
    // The default render test failure callback: records apFailureData; apParamData is unused.
    static void DefaultRenderTestCallback(RENDER_FAILURE_DATA* apFailureData, void* apParamData);
    // Records a failed render test.
    static void SaveRenderFailureData(RENDER_FAILURE_DATA* apFailureData);
    // Steps the exterior Havok world; afTime is unused.
    static void UpdateExteriorWorld(float afTime);
    // Computes a block key.
    static uint32_t CalcExtGroupBlockKey(int32_t aiX, int32_t aiY);
    // Computes a sub-block key.
    static uint32_t CalcExtGroupSubBlockKey(int32_t aiX, int32_t aiY);
    // Creates the cells' shared statics: the exterior Havok world and the seen-data radius cap.
    static void InitStatics();
    // Creates the exterior Havok world centred on arCenter.
    static bhkWorldM* CreateWorld(NiPoint3& arCenter);
    // Moves the exterior Havok world to arCenter, charging the work to the Havok memory context.
    static void MoveExteriorWorld(NiPoint3& arCenter);
    // Queues apRef's 3D to be attached to apParent on the main thread.
    static void QueueAttachChild(TESObjectREFR* apRef, NiNode* apParent);
    // Attaches the queued children whose parent changed, then empties the queue.
    static void PerformQueuedChildAttaches();
    // Sets the calling thread's bLoadingTempData.
    static void SetLoadingTempData(bool abVal);
    // The calling thread's bLoadingTempData, a thread-local static.
    static bool& GetThreadLoadingTempData();
    // Runs the queued particle-only updates of the main thread.
    static void RunQueuedParticleUpdates();
    // Whether a record header of the file belongs in a cell's children group.
    static bool IsFormCellChild(FORM* apFORM);
    // Whether forms of the type belong in a cell's children group.
    static bool IsFormTypeCellChild(uint8_t aeType);
    // Updates only the particle systems under ap3D.
    static void UpdateParticlesOnly(NiAVObject* ap3D);
    // Queues ap3D for a particle-only update on the main thread.
    static void QueueParticleUpdate(NiAVObject* ap3D);
    static BSSimpleList<TESObjectREFR*>* GetAutoWaterReflectList() {
        return &TESObjectCELL::AutoWaterReflectList;
    }
    static BSSimpleList<TESObjectREFR*>* GetAutoWaterRefractList() {
        return &TESObjectCELL::AutoWaterRefractList;
    }
    static const char* GetDefaultCellName();
    static void ClearOptimizationDataForCell(TESObjectCELL* apCell);
    static float GetSeenDataUpdateRadius();
    static bool GetLoadingTempData();
    static bool FindReferenceInFile(TESFile* apFile, uint32_t aiRefID);
    static void UpdateMorphControllersOnly(NiAVObject* ap3D);
    static int32_t GetSeenValue(const NiPoint3& aCoord);
    static int32_t
    CreateCanopyShadowMaskForCell(TESObjectCELL* apCell, NiRenderedTexture*& apOutTex, _D3DLOCKED_RECT*& apOutRect);
    static TESObjectREFR*
    PickRandomTeleportDoorInSpace(TESForm* apDoorSpace, TESForm* apTargetSpace, TESObjectREFR** appOutBackUp);
    static void UpdateSeenDataForPlayer();
    static void
    RenderTestCell(TESObjectCELL* apCell, TESObjectCELL::RENDER_TEST_DATA* apTestData, bool abtestEntireCell);
    static bool
    RenderTestPos(TESObjectCELL* apCell, const NiPoint3& aTestPos, TESObjectCELL::RENDER_TEST_DATA* apTestData);
    static bool RenderTestPosRot(
        TESObjectCELL* apCell,
        const NiPoint3& aTestPos,
        const NiMatrix3& aTestRot,
        TESObjectCELL::RENDER_TEST_DATA* apTestData
    );
    static bool EnumReferencesCloseToPointFromInterior(
        TESObjectCELL* apStartInt,
        const NiPoint3& aPoint1,
        float afRadius1,
        const NiPoint3& aPoint2,
        float afRadius2,
        bool (*apfnEnumFunc)(TESObjectREFR*, void*),
        void* aiEnumData
    );

    uint8_t cCellFlags;
    uint8_t cCellGameFlags;
    uint8_t cCellState;
    uint8_t padding27;
    ExtraDataList ExtraData;
    // EXTERIOR_DATA for an exterior cell, INTERIOR_DATA for an interior one
    void* pCellData;
    TESObjectLAND* pCellLand;
    float fWaterHeight;
    bool bAutoWaterLoaded;
    uint8_t padding55[3];
    TESTexture WaterNoiseTexture;
    NavMeshArray* pNavMeshes;
    // the nav mesh and other unreconstructed bookkeeping up to the spin lock
    uint8_t padding68[0x18];
    BSSpinLock kSpinLock;
    int32_t iCriticalQueuedRefCount;
    int32_t iQueuedRefCount;
    uint16_t sNumRefsWithVisibleDistant;
    int16_t sNumLoadedRefsWithVisibleDistant;
    BSSimpleList<TESObjectREFR*> listReferences;
    NiPointer<NiNode> spLightMarkerNode;
    NiPointer<NiNode> spSoundMarkerNode;
    NiPointer<NiNode> spMultiBoundNode;

    union {
        TESWorldSpace* pWorldSpace;
        uint32_t iTempDataOffset;
    };

    LoadedCellData* pLoadedData;
    float fLodFadeInPercent;
    bool bLODFadingIn;
    bool bFadedIn;
    bool bFadingToHighDetail;
    bool bFadingToLowDetail;
    bool bDisplayHighDetail;
    bool bCellDetached;
    bool bUpdateTerrain;
    uint8_t paddingD3;
    NiPointer<BSPortalGraph> spPortalGraph;
    BGSLightingTemplate* pLightingTemplate;
    uint32_t iLightingTemplateInheritanceFlags;

    static inline uint32_t iNodeCullFlags;
    // The seconds a cell takes to fade from its LOD to its high detail.
    static inline float fLodFadeTime = 1.2f;
    // The share of fLodFadeTime after which a fading cell shows its high detail. Its only read is in
    // UpdateManagedNodes.
    static inline float fLodFadeThreshold = 1.0f;
    static inline float fLodFadeInThreshold = 1.0f;
    static inline float fLodFadeOutThreshold = 65536.0f;
    static inline float fLodHighDetailDistance = 65536.0f;
    static NiPointer<bhkWorldM> spExteriorWorld;
    // The default name of exterior cells.
    static SettingT<GameSettingCollection> sDefaultCellName;
    // The cap of the seen-data update radius.
    static SettingT<GameSettingCollection> fSeenDataUpdateRadius;
    // The child attaches queued by QueueAttachChild, by child.
    static QueuedChildAttachMapType QueuedChildAttachMap;
    // The references waiting on a CheckWithinMultiBoundTask, with the multibound reference each is tested against.
    static MultiboundTestMapType MultiboundTestMap;
    // The listener that is told when the havok islands of a cell wake and sleep.
    static inline FOIslandActivationListener* pIslandListener;
    // The 3D whose particles RunQueuedParticleUpdates updates, and its lock.
    static QueuedParticleUpdateList QueuedParticleUpdates;
    static BSSpinLock QueuedParticleLock;
    // The exterior references reflected and refracted by the auto water.
    static BSSimpleList<TESObjectREFR*> AutoWaterReflectList;
    static BSSimpleList<TESObjectREFR*> AutoWaterRefractList;
};

inline TESObjectCELL* pLastCheckedPointCell;

extern SettingT<INISettingCollection> bDefaultCOCPlacement;
extern SettingT<INISettingCollection> bUseMultibounds;
inline float fextdoordist = 3.4028235e+38f;
inline char g_bNoCanopy;
inline TESWorldSpace* pextworld;
inline int32_t icallsdeep;
inline NiPoint3 extcoord{};
