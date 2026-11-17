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

// TES::FreeTexture: releases the named texture, if it is not still in use. The body is in BSTreeManager.cpp.
bool TES_FreeTexture(TES* apTES, const char* apPath, uint32_t aiUser);
char sub_BB2FC0(uint32_t a1, float Yaw, float Pitch);

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
    static void SetTreesEnabled(bool a1);
    static BSXFlags* GetHavokFlags();
    static NiAlphaProperty* GetDistantBillboardAlphaProperty();
    static void SetLeavesVisible(bool abShowLeaves);
    // The deleting destructor frees with ourFree.
    static void operator delete(void* apMemory);
    static void ClearCanopyShadow_004F3310();
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
};

// The tables BSTreeManager::UpdateWindMatrices reads and writes: a table of eight floats and a table of four floats.
using LeafSwayBaseTable = float[8];
using LeafSwayPhaseTable = float[4];

inline BSTreeManager* pTreeManager;

extern SettingT<INISettingCollection> gs_fCanopyShadowGrassMult_SpeedTree;

extern SettingT<INISettingCollection> gs_iCanopyShadowScale_SpeedTree;

// The value getters of BSTreeManager's tree settings (bEnableTrees, fTreeForceCS, ...).
namespace SpeedTree {
    namespace bEnableTrees {
        bool Get();
    }
    namespace fTreeForceCS {
        float Get();
    }
    namespace fTreeForceBranchDimming {
        float Get();
    }
    namespace fTreeForceLeafDimming {
        float Get();
    }
    namespace fTreeForceMinBudAngle {
        float Get();
    }
    namespace fTreeForceMaxBudAngle {
        float Get();
    }
    // Clamps afValue to [0, 1] (NaN gives 0) and stores it as the grass's canopy shadow multiplier.
    namespace fCanopyShadowGrassMult {
        void Set(float afValue);
    }
} // namespace SpeedTree

// Getters of the tree settings BSTreeManager::fTreeNearDistanceBase and BSTreeManager::fTreeSizeConversion.
namespace GameSetting {
    namespace fTreeNearDistanceBase {
        float Get();
    }
    namespace fTreeFarDistanceBase {
        float Get();
    }
    namespace fTreeSizeConversion {
        float Get();
    }
} // namespace GameSetting
