#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

class BSTreeModel;
class BSTreeNode;
class BSXFlags;
class NiAlphaProperty;
class NiCamera;
class NiMaterialProperty;
class NiSourceTexture;
class NiTexture;
class TES;
class TESObjectREFR;
class TESObjectTREE;

template <class Key, class Data>
class LockFreeMap;
template <class TKEY, class TVAL>
class NiTPointerMap;

// Sets the shader wind matrix auiMatrixIndex, of four, to the rotation by Yaw and Pitch; 0 for an index out of range.
char SetWindMatrix(uint32_t auiMatrixIndex, float Yaw, float Pitch);

// The SpeedTree forest: the loaded tree models, their shared properties, and the canopy shadow.
class BSTreeManager {
public:
    BSTreeManager();
    ~BSTreeManager();

    // Drops apRef from the trees loaded in the background.
    void RemoveBackgroundLoadedTree(TESObjectREFR* apRef);
    // Creates the tree node for apReference from apTree.
    BSTreeNode* CreateTreeForReference(TESObjectREFR* apTreeRefr, TESObjectTREE* apTreeObj);
    // Keeps apNode as the tree apRef loaded in the background.
    void AddBackgroundLoadedTree(TESObjectREFR* apRef, BSTreeNode* apTreeNode);
    NiPointer<BSTreeModel>* GetModelArray(TESObjectTREE* apTree);
    void GetModel(TESObjectTREE* apTree, uint32_t aiSeed, NiPointer<BSTreeModel>& aspBSTreeModel);
    void UpdateWindMatrices();

    static void SetCamera(NiCamera* apCamera);
    // The camera the tree LOD sorts against (pWorldCamera).
    static NiCamera* GetCamera();
    static BSTreeManager* GetInstance(bool abCreate);
    // Builds the singleton; an existing one is destroyed and rebuilt only when
    // abRecreateIfFound is set.
    static void Create(bool abRecreateIfFound);
    // Frees the canopy shadow texture, so the next weather or grid reload recreates it.
    static void ClearCanopyShadow();
    // Empties the tree model cache.
    static void ClearModelCache(bool abClearManagerOnlyHeldOnly);
    // Whether trees are shown: the tree setting is on and the manager shows them.
    static bool GetTreesEnabled();
    // Whether tree leaves are shown: trees are enabled and the manager shows leaves.
    static bool AreLeavesVisible();
    static bool GetUseCanopyShadows();
    static bool CanTreesLoad();
    static float GetForceCurveScalar();
    static float GetForceBranchDimming();
    static float GetForceLeafDimming();
    static float GetForceMinimumBudAngle();
    static float GetForceMaximumBudAngle();
    static float GetNearDistanceBase();
    static float GetFarDistanceBase();
    static float GetSizeConversion();
    // The tree near distance: the base distance scaled by the tree LOD multiplier.
    static float GetNearDistance();
    // The tree far distance: the base distance scaled by the tree LOD multiplier.
    static float GetFarDistance();
    // Creates the canopy shadow texture of the current worldspace.
    static NiTexture* GetCanopyShadow();
    // Updates the tree LOD and wind against apCamera.
    static void Update(NiCamera* apCamera, bool abMenuMode);
    static void SetForceFullLOD(bool abForce);
    static bool GetForceFullLOD();
    static void SetTreesEnabled(bool abEnabled);
    static BSXFlags* GetHavokFlags();
    static NiAlphaProperty* GetDistantBillboardAlphaProperty();
    static void SetLeavesVisible(bool abShowLeaves);
    // The deleting destructor frees with ourFree.
    static void operator delete(void* apMemory);
    static void Destroy();

    // The loaded tree models by tree form.
    NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>* pModelMap;
    NiPointer<NiSourceTexture> spCanopyShadow;
    NiPointer<NiMaterialProperty> spBillboardMatProp;
    NiPointer<NiAlphaProperty> spDistantAlphaProp;
    NiPointer<BSXFlags> spHavokFlags;
    float fWindSpeed;
    bool bShowTrees;
    bool bShowLeaves;
    bool bForceCompute;
    bool m_bForceFullLOD;
    // The trees loaded in the background, by reference.
    LockFreeMap<TESObjectREFR*, BSTreeNode*>* pBackgroundLoadedTreeMap;

    // The camera the tree LOD sorts against.
    static inline NiCamera* pWorldCamera;
    // Whether the trees cast canopy shadows.
    static inline bool bUseCanopyShadows = true;
    static BSCriticalSection kModelCriticalSection;
    // How fast the leaves rock ("fLeafRockTimeScale", 2.0).
    static SettingT<GameSettingCollection> fLeafRockTimeScale;
    static SettingT<GameSettingCollection> fTreeWindVariance;
    // "bForceFullLOD:SpeedTree".
    static SettingT<INISettingCollection> bForceFullLOD;
    static SettingT<INISettingCollection> fTreeLODExponent;
    static SettingT<INISettingCollection> fTreeForceLLA;

protected:
    // These are constructed with SettingT<GameSettingCollection>'s float constructor.
    static SettingT<GameSettingCollection> fTreeNearDistanceBase;
    static SettingT<GameSettingCollection> fTreeFarDistanceBase;
    static SettingT<GameSettingCollection> fTreeSizeConversion;
    static SettingT<GameSettingCollection> fLeafRustleTimeScale;
    static SettingT<GameSettingCollection> fLeafRockSpeedSwayInfluence;
    static SettingT<GameSettingCollection> fLeafRustleSpeedSwayInfluence;
    static SettingT<GameSettingCollection> fLeafRockAmountSwayInfluence;
    static SettingT<GameSettingCollection> fLeafRustleAmountSwayInfluence;
    static SettingT<INISettingCollection> bEnableTrees;
    static SettingT<INISettingCollection> fTreeForceCS;
    static SettingT<INISettingCollection> fTreeForceBranchDimming;
    static SettingT<INISettingCollection> fTreeForceLeafDimming;
    static SettingT<INISettingCollection> fTreeForceMinBudAngle;
    static SettingT<INISettingCollection> fTreeForceMaxBudAngle;
    static SettingT<INISettingCollection> iCanopyShadowScale;
    static SettingT<INISettingCollection> fCanopyShadowGrassMult;
};

// The tables BSTreeManager::UpdateWindMatrices reads and writes: a table of eight floats and a table of four floats.
using LeafSwayBaseTable = float[8];
using LeafSwayPhaseTable = float[4];

inline BSTreeManager* pTreeManager;

inline bool BSTreeManager::CanTreesLoad() {
    return BSTreeManager::bEnableTrees.Bool();
}

inline float BSTreeManager::GetForceCurveScalar() {
    return BSTreeManager::fTreeForceCS.Float();
}

inline float BSTreeManager::GetForceBranchDimming() {
    return BSTreeManager::fTreeForceBranchDimming.Float();
}

inline float BSTreeManager::GetForceLeafDimming() {
    return BSTreeManager::fTreeForceLeafDimming.Float();
}

inline float BSTreeManager::GetForceMinimumBudAngle() {
    return BSTreeManager::fTreeForceMinBudAngle.Float();
}

inline float BSTreeManager::GetForceMaximumBudAngle() {
    return BSTreeManager::fTreeForceMaxBudAngle.Float();
}

inline float BSTreeManager::GetNearDistanceBase() {
    return BSTreeManager::fTreeNearDistanceBase.Float();
}

inline float BSTreeManager::GetFarDistanceBase() {
    return BSTreeManager::fTreeFarDistanceBase.Float();
}

inline float BSTreeManager::GetSizeConversion() {
    return BSTreeManager::fTreeSizeConversion.Float();
}
