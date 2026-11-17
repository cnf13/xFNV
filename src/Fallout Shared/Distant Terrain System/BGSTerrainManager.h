#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BGSDistantTreeBlock;
class BGSTerrainChunk;
class BGSTerrainNode;
class NiCamera;
class TESObjectREFR;
class TESWorldSpace;

// The distant terrain (LOD land, water and objects) of a worldspace. Only some of its members are declared.
class BGSTerrainManager {
public:
    // What Update refreshes.
    enum LOD_UPDATE_TYPE : int32_t {
        UPDATE_LAND_AND_BLOCKS = 1,
        UPDATE_BLOCK_VISIBILITY = 2,
        UPDATE_WATER_VISIBILITY = 4,
        UPDATE_TREES = 8,
        UPDATE_ALL = 15,
    };

    BGSTerrainManager(TESWorldSpace* apWorld);
    ~BGSTerrainManager();

    // Switches the whole node tree to the apocalypse LOD.
    void TriggerApocalypse();
    // The tree block holding aPoint, or null when there is no root node.
    BGSDistantTreeBlock* GetTreeBlockForPoint(const NiPoint3& aPoint);
    // Drops every queued hide of apTreeRef.
    void ReleaseTree(TESObjectREFR* apTreeRef);
    // Shows the LOD tree standing in for apTreeRef again.
    void ShowTree(TESObjectREFR* apTreeRef);
    NiNode* GetLODLandRoot();
    NiNode* GetLODObjectRoot();
    BGSTerrainChunk* GetChunkForPoint(const NiPoint3& aPoint);
    // Updates the parts aWhatToUpdate of the distant terrain around aViewPosition.
    void Update(const NiPoint3& aViewPosition, LOD_UPDATE_TYPE aWhatToUpdate);
    // Updates the LOD morphing for aViewPosition.
    void UpdateMorphParameters(const NiPoint3& aViewPosition);
    // Unloads all the distant terrain.
    void UnloadAll();
    // Hides the LOD tree standing in for apTreeRef, queued when abQueue.
    void HideTree(TESObjectREFR* apTreeRef, bool abQueue);
    // Asks for the distant terrain to update next frame.
    void SetNeedsImmediateUpdate(bool abUpdate);
    // Updates the multibound visibility of the distant terrain for apCamera.
    void UpdateMultiBoundVisibility(NiCamera* apCamera);
    // Resets the cull results of the terrain chunks and distant object blocks.
    void ResetMultiBoundResults();
    bool HasRootNode();
    NiTexture* GetDiffuseTextureForPoint(const NiPoint3& aPoint);
    NiTexture* GetNormalTextureForPoint(const NiPoint3& aPoint);
    TESWorldSpace* GetWorld();
    NiNode* GetLODRoot() {
        return spLODRoot;
    }
    // spLODWaterRoot.
    NiNode* GetWaterLODRoot() {
        return spLODWaterRoot;
    }
    NiNode* GetTreeNode();
    void DestroyNodeAndChildren();

    // Turns the LOD water processing on or off.
    static void EnableLODWaterProcessing(bool abEnable);
    static void DisableBlockVisibility();
    static void EnableBlockVisibility();
    static uint32_t GetTreeLevel();
    static bool GetEnabled();
    static void ShowMinimumDetail();
    static void ShowNormalDetail();
    static BGSTerrainManager* GetActiveManager();
    static bool GetCurrentWorldHasLOD();
    static void operator delete(void* apMemory);
    static void SetTreeNode(NiNode* apTreeNode);
    static void DeallocateStaticNodes();

    TESWorldSpace* pWorld;
    BGSTerrainNode* pRootNode;
    NiPointer<NiNode> spLODRoot;
    NiPointer<NiNode> spLODWaterRoot;
    // The cell bounds the constructor reads from the world's .dlodsettings, as two x, y pairs.
    int16_t sMinCellX;
    int16_t sMinCellY;
    int16_t sMaxCellX;
    int16_t sMaxCellY;
    uint32_t iMaxLevel;
    uint32_t iMinLevel;
    uint32_t iRootLevel;
    uint32_t iBlockLevel;
    bool bNeedsImmediateUpdate;
    bool bHasLOD;
    uint8_t padding2A[0x2];
    // The trees whose LOD HideTree queued.
    BSSimpleArray<TESObjectREFR*> TreesQueuedToHide;

    static inline BGSTerrainManager* pActiveManager;
    static NiPointer<NiNode> spRootLandLODNode;
    static NiPointer<NiNode> spRootObjectLODNode;
    static NiPointer<NiNode> spTreeNode;
    static inline bool bEnabled = true;
};

// The game settings BGSTerrainManager.cpp defines.
extern SettingT<INISettingCollection> bUseDistantTrees;
extern SettingT<INISettingCollection> bUseDistantObjectBlocks;
extern SettingT<INISettingCollection> bUseNewTerrainSystem;
